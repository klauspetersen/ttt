//
// Created by kape on 12/3/15.
//

#include "sigrok_wrapper.h"
#include <glib.h>
#include <libsigrok-internal.h>
#include <string.h>
#include <malloc.h>
#include "hardware/saleae-logic16/protocol.h"
#include <poll.h>

static void sr_data_recv_cb(sr_wrap_packet_t *packet);

struct sr_dev_inst *sdiArr[SIGROK_WRAPPER_MAX_DEVICES];
struct sr_context *sr_ctx = NULL;

extern SR_PRIV struct sr_dev_driver saleae_logic16_driver_info;


void sigrok_init(struct sr_context **ctx) {
    struct sr_dev_driver *driver;
    struct sr_session *session;
    struct sr_dev_inst *sdi;
    struct drv_context *drvc;
    struct sr_channel;
    GSList *l;

    driver = &saleae_logic16_driver_info;

    sr_ctx = g_malloc0(sizeof(struct sr_context));
    libusb_init(&sr_ctx->libusb_ctx);
    sr_resource_set_hooks(sr_ctx, NULL, NULL, NULL, NULL);

    session = g_malloc0(sizeof(struct sr_session));
    session->ctx = ctx;

    drvc = g_malloc0(sizeof(struct drv_context));
    drvc->sr_ctx = sr_ctx;
    drvc->instances = NULL;
    driver->context = drvc;


    GSList *devices = driver->scan(driver, NULL);

    int i = 0;
    for (GSList *l = devices; l; l = l->next) {
        sdiArr[i] = (struct sr_dev_inst *) l->data;
        sdiArr[i]->id = i;
        sdiArr[i]->cb = sr_data_recv_cb;
        session->devs = g_slist_append(session->devs, sdiArr[i]);
        sdiArr[i]->session = session;
        sdiArr[i]->driver->dev_open(sdiArr[i]); /* Calls upload */
        i++;
    }

    session->main_context = g_main_context_ref_thread_default();

    /* Have all devices start acquisition. */
    for (l = session->devs; l; l = l->next) {
        sdi = l->data;
        sigrok_start(sdi, sdi); /* Calls upload bitstream */
    }

    /* Have all devices fire. */
    for (l = session->devs; l; l = l->next) {
        sdi = l->data;
        sdi->driver->dev_acquisition_trigger(sdi);
    }



    *ctx = sr_ctx;
#if 0
    GMainLoop *main_loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(main_loop);

#else
    int poll_ret;
    const struct libusb_pollfd **poll_usb;
    struct pollfd pollfd_array[20];
    struct timeval tv;
    int completed;
    int fdCnt;

    //get device handle and claim interface
    //also set up context

    tv.tv_usec = 0;
    poll_usb = libusb_get_pollfds(sr_ctx->libusb_ctx);

    for (fdCnt = 0; poll_usb[fdCnt] != NULL; fdCnt++) {
        pollfd_array[fdCnt].fd = poll_usb[fdCnt]->fd;
        pollfd_array[fdCnt].events = poll_usb[fdCnt]->events;
    }

    while(1) {
        poll_ret = poll(pollfd_array, fdCnt, 5000); // timeout of 5 seconds
        //so here poll wont trigger even though i know there is data ready to be received
        //i'm sending data from another thread to the usb device, and it replies but doesn't trigger poll
        if (poll_ret > 0) {
            //perform receive functions
            //printf("poll_ret > 0 %d\n", poll_ret);
            libusb_handle_events_timeout_completed(sr_ctx->libusb_ctx, &tv, &completed);
            //printf("Completed: %d\n", completed);
            completed = 0;
        }
        else {
            //printf("poll_ret = 0");
            //timeout occurs or pipe error
        }
    }
#endif
}

/** Custom GLib event source for libusb I/O.
 * @internal
 */
struct usb_source {
    GSource base;

    int64_t timeout_us;
    int64_t due_us;

    /* Needed to keep track of installed sources */
    struct sr_session *session;

    struct libusb_context *usb_ctx;
    GPtrArray *pollfds;
};

extern gboolean usb_source_prepare(GSource *source, int *timeout);
extern gboolean usb_source_check(GSource *source);
extern gboolean usb_source_dispatch(GSource *source, GSourceFunc callback, void *user_data);


int sigrok_start(const struct sr_dev_inst *sdi, void *cb_data) {
    struct sr_dev_driver *di = sdi->driver;
    struct dev_context *devc;
    struct drv_context *drvc;
    struct sr_usb_dev_inst *usb;
    struct libusb_transfer *transfer;
    unsigned int i;
    unsigned char *buf;
    size_t size;

    if (sdi->status != SR_ST_ACTIVE)
        return SR_ERR_DEV_CLOSED;

    drvc = di->context;
    devc = sdi->priv;
    usb = sdi->conn;

    sr_info("dev_acquisition_start");

    memset(devc->channel_data, 0, sizeof(devc->channel_data));

    devc->submitted_transfers = 0;

    logic16_setup_acquisition(sdi, devc->cur_samplerate);

    /* Allocate transfer buffers */
    devc->num_transfers = 100;
    devc->transfers = g_try_malloc0(sizeof(*devc->transfers) * devc->num_transfers);

    devc->ctx = drvc->sr_ctx;

    sr_info("usb_source_add");

    static GSourceFuncs usb_source_funcs = {
            .prepare  = &usb_source_prepare,
            .check    = &usb_source_check,
            .dispatch = &usb_source_dispatch,
            .finalize = NULL
    };

#if 0
    GSource *source = g_source_new(&usb_source_funcs, sizeof(struct usb_source));
    g_source_set_callback(source, (GSourceFunc)receive_data, cb_data, NULL);
    g_source_attach(source, sdi->session->main_context);
    g_source_unref(source);

    struct usb_source *usource = (struct usb_source *)source;
    usource->timeout_us = 1000 * (int64_t)1000;
    usource->due_us = 0;
    usource->session = sdi->session;
    usource->usb_ctx = devc->ctx->libusb_ctx;
    usource->pollfds = g_ptr_array_new_full(8, NULL);


    const struct libusb_pollfd **upollfds = libusb_get_pollfds(devc->ctx->libusb_ctx);
    for (const struct libusb_pollfd **upfd = upollfds; *upfd != NULL; upfd++) {
        GPollFD *pollfd = g_slice_new(GPollFD);
        pollfd->fd = (gintptr) ((*upfd)->fd);
        pollfd->events = (*upfd)->events;
        pollfd->revents = 0;

        g_ptr_array_add(usource->pollfds, pollfd);
        g_source_add_poll(&usource->base, pollfd);
    }
    free(upollfds);
#endif

    size = 160256;
    for (i = 0; i < devc->num_transfers; i++) {
        buf = g_try_malloc(size);

        sr_info("sdi id: %d", sdi->id);
        transfer = libusb_alloc_transfer(0);
        libusb_fill_bulk_transfer(transfer, usb->devhdl, 2 | LIBUSB_ENDPOINT_IN, buf, size, logic16_receive_transfer, (void *) sdi, 5000);
        libusb_submit_transfer(transfer);

        devc->transfers[i] = transfer;
        devc->submitted_transfers++;
    }
    return SR_OK;
}

static void sr_data_recv_cb(sr_wrap_packet_t *packet){
    for(int i=0; i< packet->size; i++){
        //printf("%s\n", byte2bin(((uint8_t*)packet->data)[i], buf));
        if(((uint8_t*)packet->data)[i] != 0) {
            printf("%d\n", ((uint8_t *) packet->data)[i]);
        }
    }
}


//
// Created by kape on 12/3/15.
//

#include "sigrok_wrapper.h"
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

    const struct libusb_pollfd **poll_usb;
    struct pollfd pollfd_array[20];
    struct timeval tv;
    int completed;
    int fdCnt;

    tv.tv_usec = 0;
    poll_usb = libusb_get_pollfds(sr_ctx->libusb_ctx);

    for (fdCnt = 0; poll_usb[fdCnt] != NULL; fdCnt++) {
        pollfd_array[fdCnt].fd = poll_usb[fdCnt]->fd;
        pollfd_array[fdCnt].events = poll_usb[fdCnt]->events;
    }

    while(1) {
        if (poll(pollfd_array, fdCnt, 5000)) {
            libusb_handle_events_timeout_completed(sr_ctx->libusb_ctx, &tv, &completed);
            if(completed != 0){
                printf("%d\n", completed);
                completed = 0;
            }
        }
    }
}


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
    devc->transfers = malloc(sizeof(*devc->transfers) * devc->num_transfers);

    devc->ctx = drvc->sr_ctx;

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


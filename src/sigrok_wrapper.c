//
// Created by kape on 12/3/15.
//

#include "sigrok_wrapper.h"
#include <glib.h>
#include <libsigrok-internal.h>
#include <string.h>
#include "hardware/saleae-logic16/protocol.h"

static void sr_data_recv_cb(sr_wrap_packet_t *packet);

struct sr_dev_inst *sdiArr[SIGROK_WRAPPER_MAX_DEVICES];
struct sr_context *sr_ctx = NULL;

extern SR_PRIV struct sr_dev_driver saleae_logic16_driver_info;

void sigrok_init(struct sr_context **ctx){
    struct sr_dev_driver *driver;
    struct sr_session *session;
    struct sr_context *context;
    struct sr_dev_inst *sdi;
    struct sr_channel;
    GSList *l;

    GMainLoop *main_loop;

    driver = &saleae_logic16_driver_info;

    context = g_malloc0(sizeof(struct sr_context));
    libusb_init(&context->libusb_ctx);
    sr_resource_set_hooks(context, NULL, NULL, NULL, NULL);
    sr_ctx = context;

    session = g_malloc0(sizeof(struct sr_session));
    session->ctx = ctx;
    session->event_sources = g_hash_table_new(NULL, NULL);

    driver->init(driver, sr_ctx);

    GSList *devices = sr_driver_scan(driver, NULL);

    int i=0;
    for (GSList *l = devices; l; l = l->next) {
        sdiArr[i] = (struct sr_dev_inst *)l->data;
        sdiArr[i]->id = i;
        sdiArr[i]->cb = sr_data_recv_cb;
        session->devs = g_slist_append(session->devs, sdiArr[i]);
        sdiArr[i]->session = session;
        sdiArr[i]->driver->dev_open(sdiArr[i]);
        i++;
    }


    session->main_context = g_main_context_ref_thread_default();
    g_main_context_acquire(session->main_context);
    g_main_context_release(session->main_context);


    /* Have all devices start acquisition. */
    for (l = session->devs; l; l = l->next) {
        sdi = l->data;
        sigrok_start(sdi, sdi);
    }

    /* Have all devices fire. */
    for (l = session->devs; l; l = l->next) {
        sdi = l->data;
        sdi->driver->dev_acquisition_trigger(sdi);
    }

    main_loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(main_loop);

    *ctx = sr_ctx;
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

    devc->sent_samples = 0;
    memset(devc->channel_data, 0, sizeof(devc->channel_data));

    devc->submitted_transfers = 0;

    logic16_setup_acquisition(sdi, devc->cur_samplerate);

    /* Allocate transfer buffers */
    devc->num_transfers = 100;
    devc->transfers = g_try_malloc0(sizeof(*devc->transfers) * devc->num_transfers);

    devc->ctx = drvc->sr_ctx;

    sr_info("usb_source_add");

    usb_source_add(sdi->session, devc->ctx, 1000, receive_data, (void *) sdi);

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
    //char buf[9];

    for(int i=0; i< packet->size; i++){
        //printf("%s\n", byte2bin(((uint8_t*)packet->data)[i], buf));
        if(((uint8_t*)packet->data)[i] != 0) {
            printf("%d\n", ((uint8_t *) packet->data)[i]);
        }
    }
}


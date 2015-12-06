//
// Created by kape on 12/3/15.
//

#include "sigrok_wrapper.h"
#include <glib.h>
#include <libsigrok-internal.h>

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
        sdi->driver->dev_acquisition_start(sdi, sdi);
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

static void sr_data_recv_cb(sr_wrap_packet_t *packet){
    //char buf[9];

    for(int i=0; i< packet->size; i++){
        //printf("%s\n", byte2bin(((uint8_t*)packet->data)[i], buf));
        if(((uint8_t*)packet->data)[i] != 0) {
            printf("%d\n", ((uint8_t *) packet->data)[i]);
        }
    }
}


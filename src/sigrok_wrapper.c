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

void sigrok_init(){
    struct sr_dev_driver *driver;
    struct sr_session *session;
    GMainLoop *main_loop;

    driver = &saleae_logic16_driver_info;

    if (sr_init(&sr_ctx) != SR_OK) {
        g_critical("Failed to initialize context.");
    }

    if(sr_session_new(sr_ctx, &session) != SR_OK){
        g_critical("Failed to initialize session.");
    }

    if (sr_driver_init(sr_ctx, driver) != SR_OK) {
        g_critical("Failed to initialize driver.");
    }

    GSList *devices = sr_driver_scan(driver, NULL);

    int i=0;
    for (GSList *l = devices; l; l = l->next) {
        sdiArr[i] = (struct sr_dev_inst *)l->data;
        sdiArr[i]->id = i;
        sdiArr[i]->cb = sr_data_recv_cb;
        if(sr_session_dev_add(session, sdiArr[i]) != SR_OK) {
            g_critical("Failed to add device to session.");
        }

        if(sr_dev_open(sdiArr[i]) != SR_OK){
            g_critical("Failed to open device");
        }
        i++;
    }

    main_loop = g_main_loop_new(NULL, FALSE);

    if(sr_session_start(session) != SR_OK){
        g_critical("Failed to start session.");
    }

    g_main_loop_run(main_loop);
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


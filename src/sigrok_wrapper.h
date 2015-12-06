//
// Created by kape on 12/3/15.
//

#ifndef TTT_SIGROK_WRAPPER_H
#define TTT_SIGROK_WRAPPER_H

#include <unistd.h>
#include <libusb.h>


#define SIGROK_WRAPPER_MAX_DEVICES 3

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int id;
    uint8_t *data;
    ssize_t size;
    void *ref;
} sr_wrap_packet_t;

typedef struct {
    struct libusb_transfer *transfer;
    sr_wrap_packet_t packet;
} sr_warp_transfer_t;


typedef void (*sr_callback_t)(sr_wrap_packet_t *packet);

#include "libsigrok-internal.h"

void sigrok_init(struct sr_context **ctx);



#ifdef __cplusplus
}
#endif

#endif //TTT_SIGROK_WRAPPER_H

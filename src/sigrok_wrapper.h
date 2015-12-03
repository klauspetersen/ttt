//
// Created by kape on 12/3/15.
//

#ifndef TTT_SIGROK_WRAPPER_H
#define TTT_SIGROK_WRAPPER_H

#include <unistd.h>

#define SIGROK_WRAPPER_MAX_DEVICES 3

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int id;
    void *data;
    ssize_t size;
} sr_wrap_packet_t;

typedef struct libusb_transfer{} sr_wrap_libusb_transfer_t;

typedef struct {
    sr_wrap_libusb_transfer_t transfer;
    sr_wrap_packet_t packet;
} sr_warp_transfer_t;


typedef void (*sr_callback_t)(sr_wrap_packet_t *packet);

void sigrok_init();

#ifdef __cplusplus
}
#endif

#endif //TTT_SIGROK_WRAPPER_H

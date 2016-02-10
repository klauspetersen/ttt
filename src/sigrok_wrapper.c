//
// Created by kape on 12/3/15.
//

#include "sigrok_wrapper.h"
#include <string.h>
#include <malloc.h>
#include "hardware/saleae-logic16/protocol.h"
#include <poll.h>
#include <stdlib.h>
#include <assert.h>

#define LOGIC16_VID        0x21a9
#define LOGIC16_PID        0x1001

#define USB_INTERFACE        0
#define USB_CONFIGURATION    1
#define FX2_FIRMWARE        "saleae-logic16-fx2.fw"
static void sr_data_recv_cb(sr_wrap_packet_t *packet);
static int dev_open(struct sr_dev_inst *sdi);
GSList *scan(struct sr_dev_driver *di);
int sigrok_start(const struct sr_dev_inst *sdi);
int usb_get_port_path(libusb_device *dev, char *path, int path_len);

struct sr_dev_inst *sdiArr[SIGROK_WRAPPER_MAX_DEVICES];
struct sr_context *sr_ctx = NULL;

struct sr_dev_driver saleae_logic16_driver_info;

//GSList *devs;

void sigrok_init(struct sr_context **ctx) {
    struct sr_dev_driver *driver;
    struct drv_context *drvc;

    driver = &saleae_logic16_driver_info;

    sr_ctx = g_malloc0(sizeof(struct sr_context));
    libusb_init(&sr_ctx->libusb_ctx);
    sr_resource_set_hooks(sr_ctx, NULL, NULL, NULL, NULL);

    drvc = g_malloc0(sizeof(struct drv_context));
    drvc->sr_ctx = sr_ctx;
    drvc->instances = NULL;
    driver->context = drvc;

    GSList *devices = scan(driver);

    int i = 0;
    for (GSList *l = devices; l; l = l->next) {
        sdiArr[i] = (struct sr_dev_inst *) l->data;
        sdiArr[i]->id = i;
        sdiArr[i]->cb = sr_data_recv_cb;
        dev_open(sdiArr[i]); /* Calls upload */
        i++;
    }

    /* Have all devices start acquisition. */
    for (GSList *l = devices; l; l = l->next) {
        sigrok_start(l->data); /* Calls upload bitstream */
    }

    /* Have all devices fire. */
    for (GSList *l = devices; l; l = l->next) {
        logic16_start_acquisition(l->data);
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


int sigrok_start(const struct sr_dev_inst *sdi) {
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
    devc = sdi->ctx;
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


GSList *scan(struct sr_dev_driver *di) {
    struct drv_context *drvc;
    struct dev_context *devc;
    struct sr_dev_inst *sdi;
    GSList *devices;
    struct libusb_device_descriptor des;
    libusb_device **devlist;
    unsigned int i;
    char connection_id[64];

    sr_info("Scanning for device.");

    drvc = di->context;

    /* Find all Logic16 devices and upload firmware to them. */
    devices = NULL;
    libusb_get_device_list(drvc->sr_ctx->libusb_ctx, &devlist);
    for (i = 0; devlist[i]; i++) {
        libusb_get_device_descriptor(devlist[i], &des);

        usb_get_port_path(devlist[i], connection_id, sizeof(connection_id));

        if (des.idVendor != LOGIC16_VID || des.idProduct != LOGIC16_PID)
            continue;

        sdi = g_malloc0(sizeof(struct sr_dev_inst));
        sdi->status = SR_ST_INITIALIZING;
        sdi->driver = di;
        sdi->connection_id = g_strdup(connection_id);

        devc = g_malloc0(sizeof(struct dev_context));
        devc->selected_voltage_range = VOLTAGE_RANGE_18_33_V;
        sdi->ctx = devc;
        drvc->instances = g_slist_append(drvc->instances, sdi);
        devices = g_slist_append(devices, sdi);

        if (ezusb_upload_firmware(drvc->sr_ctx, devlist[i], USB_CONFIGURATION, FX2_FIRMWARE) != SR_OK) {
            sr_err("Firmware upload failed.");
        }

        sdi->conn = g_malloc0(sizeof(struct sr_usb_dev_inst));
        sdi->conn->devhdl = NULL;
    }

    libusb_free_device_list(devlist, 1);

    return devices;
}


static int logic16_dev_open(struct sr_dev_inst *sdi) {
    struct sr_dev_driver *di;
    libusb_device **devlist;
    struct sr_usb_dev_inst *usb;
    struct libusb_device_descriptor des;
    struct drv_context *drvc;
    int ret, i, device_count;
    char connection_id[64];

    di = sdi->driver;
    drvc = di->context;
    usb = sdi->conn;

    if (sdi->status == SR_ST_ACTIVE) {
        sr_err("Device already in use");
        return SR_ERR;
    }

    device_count = libusb_get_device_list(drvc->sr_ctx->libusb_ctx, &devlist);

    sr_info("Device count: %d", device_count);

    for (size_t idx = 0; idx < device_count; ++idx) {
        struct libusb_device *device = devlist[idx];
        struct libusb_device_descriptor desc = {0};

        ret = libusb_get_device_descriptor(device, &desc);
        assert(ret == 0);

        sr_info("Vendor:Device = %04x:%04x\n", desc.idVendor, desc.idProduct);
    }

    if (device_count < 0) {
        sr_err("Failed to get device list: %s.", libusb_error_name(device_count));
        return SR_ERR;
    }

    for (i = 0; i < device_count; i++) {
        libusb_get_device_descriptor(devlist[i], &des);

        if (des.idVendor != LOGIC16_VID || des.idProduct != LOGIC16_PID) {
            sr_info("Vendor or PID did not match.");
            continue;
        }

        if ((sdi->status == SR_ST_INITIALIZING) || (sdi->status == SR_ST_INACTIVE)) {
            /*
             * Check device by its physical USB bus/port address.
             */
            usb_get_port_path(devlist[i], connection_id, sizeof(connection_id));
            if (strcmp(sdi->connection_id, connection_id)) {
                sr_info("Connection id did not match");
                /* This is not the one. */
                continue;
            } else {
                sr_info("Connection match: %s", sdi->connection_id);
            }
        }

        if ((ret = libusb_open(devlist[i], &usb->devhdl))) {
            sr_err("Failed to open device: %s.", libusb_error_name(ret));
            break;
        }

        ret = libusb_claim_interface(usb->devhdl, USB_INTERFACE);
        if (ret == LIBUSB_ERROR_BUSY) {
            sr_err("Unable to claim USB interface. Another program or driver has already claimed it.");
            break;
        } else if (ret == LIBUSB_ERROR_NO_DEVICE) {
            sr_err("Device has been disconnected.");
            break;
        } else if (ret != 0) {
            sr_err("Unable to claim interface: %s.", libusb_error_name(ret));
            break;
        }

        if (logic16_init_device(sdi) != SR_OK) {
            sr_err("Failed to init device.");
            break;
        }

        sdi->status = SR_ST_ACTIVE;
        sr_info("Opened device on %d.%d (logical) / %s (physical), interface %d.", libusb_get_bus_number(devlist[i]), libusb_get_device_address(devlist[i]), sdi->connection_id, USB_INTERFACE);
        break;
    }
    libusb_free_device_list(devlist, 1);

    if (sdi->status != SR_ST_ACTIVE) {
        sr_info("Device not in active state");
        if (usb->devhdl) {
            libusb_release_interface(usb->devhdl, USB_INTERFACE);
            libusb_close(usb->devhdl);
            usb->devhdl = NULL;
        }
        return SR_ERR;
    }

    return SR_OK;
}

static int dev_open(struct sr_dev_inst *sdi) {
    struct dev_context *devc;

    devc = sdi->ctx;

    sr_info("Waiting for device to reset.");
    while (1) {
        if (logic16_dev_open(sdi) == SR_OK) {
            sr_info("Device found.");
            break;
        }

        sr_info("No device found.");
        g_usleep(100 * 1000);
    }

    devc->cur_samplerate = SR_MHZ(16);
    sr_info("Samplerate set to %d", (uint32_t) devc->cur_samplerate);

    return SR_OK;
}


int usb_get_port_path(libusb_device *dev, char *path, int path_len){
    uint8_t port_numbers[8];
    int i, n, len;

/*
 * FreeBSD requires that devices prior to calling libusb_get_port_numbers()
 * have been opened with libusb_open().
 */
#ifdef __FreeBSD__
    struct libusb_device_handle *devh;
	if (libusb_open(dev, &devh) != 0)
		return SR_ERR;
#endif
    n = libusb_get_port_numbers(dev, port_numbers, sizeof(port_numbers));
#ifdef __FreeBSD__
    libusb_close(devh);
#endif

/* Workaround FreeBSD libusb_get_port_numbers() returning 0. */
#ifdef __FreeBSD__
    if (n == 0) {
		port_numbers[0] = libusb_get_device_address(dev);
		n = 1;
	}
#endif
    if (n < 1)
        return SR_ERR;

    len = snprintf(path, path_len, "usb/%d-%d",
                   libusb_get_bus_number(dev), port_numbers[0]);

    for (i = 1; i < n; i++)
        len += snprintf(path+len, path_len-len, ".%d", port_numbers[i]);

    return SR_OK;
}
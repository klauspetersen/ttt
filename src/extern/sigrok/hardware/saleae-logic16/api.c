/*
 * This file is part of the libsigrok project.
 *
 * Copyright (C) 2013 Marcus Comstedt <marcus@mc.pp.se>
 * Copyright (C) 2013 Bert Vermeulen <bert@biot.com>
 * Copyright (C) 2012 Joel Holdsworth <joel@airwebreathe.org.uk>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <glib.h>
#include <libusb.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include "libsigrok.h"
#include "libsigrok-internal.h"
#include "protocol.h"

#define LOGIC16_VID        0x21a9
#define LOGIC16_PID        0x1001

#define USB_INTERFACE        0
#define USB_CONFIGURATION    1
#define FX2_FIRMWARE        "saleae-logic16-fx2.fw"

SR_PRIV struct sr_dev_driver saleae_logic16_driver_info;

static const char *channel_names[] = {
        "0", "1", "2", "3", "4", "5", "6", "7", "8",
        "9", "10", "11", "12", "13", "14", "15",
};



static GSList *scan(struct sr_dev_driver *di, GSList *options) {
    struct drv_context *drvc;
    struct dev_context *devc;
    struct sr_dev_inst *sdi;
    GSList *devices, *conn_devices;
    struct libusb_device_descriptor des;
    libusb_device **devlist;
    unsigned int i, j;
    char connection_id[64];

    sr_info("Scanning for device.");

    drvc = di->context;

    conn_devices = NULL;

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
        sdi->vendor = g_strdup("Saleae");
        sdi->model = g_strdup("Logic16");
        sdi->driver = di;
        sdi->connection_id = g_strdup(connection_id);

        for (j = 0; j < ARRAY_SIZE(channel_names); j++) {
            if (j < 8) {
                sr_channel_new(sdi, j, SR_CHANNEL_LOGIC, TRUE, channel_names[j]);
            } else {
                sr_channel_new(sdi, j, SR_CHANNEL_LOGIC, FALSE, channel_names[j]);
            }
        }

        devc = g_malloc0(sizeof(struct dev_context));
        devc->selected_voltage_range = VOLTAGE_RANGE_18_33_V;
        sdi->priv = devc;
        drvc->instances = g_slist_append(drvc->instances, sdi);
        devices = g_slist_append(devices, sdi);

        if (ezusb_upload_firmware(drvc->sr_ctx, devlist[i], USB_CONFIGURATION, FX2_FIRMWARE) != SR_OK) {
            sr_err("Firmware upload failed.");
        }
        sdi->inst_type = SR_INST_USB;
        sdi->conn = sr_usb_dev_inst_new(libusb_get_bus_number(devlist[i]), 0xff, NULL);

    }

    libusb_free_device_list(devlist, 1);
    g_slist_free_full(conn_devices, (GDestroyNotify) sr_usb_dev_inst_free);

    return devices;
}

static GSList *dev_list(const struct sr_dev_driver *di) {
    return ((struct drv_context *) (di->context))->instances;
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

        if (!(ret = libusb_open(devlist[i], &usb->devhdl))) {
            if (usb->address == 0xff) {
                /*
                 * First time we touch this device after FW
                 * upload, so we don't know the address yet.
                 */
                usb->address = libusb_get_device_address(devlist[i]);
            }
        } else {
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
        sr_info("Opened device on %d.%d (logical) / %s (physical), interface %d.", usb->bus, usb->address,
                sdi->connection_id, USB_INTERFACE);
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

    devc = sdi->priv;

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

static int dev_acquisition_trigger(const struct sr_dev_inst *sdi) {
    return logic16_start_acquisition(sdi);
}

SR_PRIV struct sr_dev_driver saleae_logic16_driver_info = {
        .name = "saleae-logic16",
        .longname = "Saleae Logic16",
        .api_version = 1,
        .init = NULL,
        .cleanup = NULL,
        .scan = scan,
        .dev_list = dev_list,
        .dev_clear = NULL,
        .config_get = NULL,
        .config_set = NULL,
        .config_list = NULL,
        .dev_open = dev_open,
        .dev_close = NULL,
        //.dev_acquisition_start = dev_acquisition_start,
        .dev_acquisition_start = NULL,
        .dev_acquisition_trigger = dev_acquisition_trigger,
        .dev_acquisition_stop = NULL,
        .context = NULL,
};

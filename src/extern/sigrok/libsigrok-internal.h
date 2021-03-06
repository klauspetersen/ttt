/*
 * This file is part of the libsigrok project.
 *
 * Copyright (C) 2013 Bert Vermeulen <bert@biot.com>
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

/** @file
  * @internal
  */

#ifndef LIBSIGROK_LIBSIGROK_INTERNAL_H
#define LIBSIGROK_LIBSIGROK_INTERNAL_H

#include <stdarg.h>
#include <stdio.h>
#include <glib.h>
#include <libusb.h>
#include "libsigrok.h"

#include "sigrok_wrapper.h"

/**
 * @file
 *
 * libsigrok private header file, only to be used internally.
 */

/*--- Macros ----------------------------------------------------------------*/

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#endif

#ifndef ARRAY_AND_SIZE
#define ARRAY_AND_SIZE(a) (a), ARRAY_SIZE(a)
#endif


/* Portability fixes for FreeBSD. */
#ifdef __FreeBSD__
#define LIBUSB_CLASS_APPLICATION 0xfe
#define libusb_has_capability(x) 0
#define libusb_handle_events_timeout_completed(ctx, tv, c) \
	libusb_handle_events_timeout(ctx, tv)
#endif

/* Static definitions of structs ending with an all-zero entry are a
 * problem when compiling with -Wmissing-field-initializers: GCC
 * suppresses the warning only with { 0 }, clang wants { } */
#ifdef __clang__
#define ALL_ZERO { }
#else
#define ALL_ZERO { 0 }
#endif

struct sr_context {
	libusb_context *libusb_ctx;
	sr_resource_open_callback resource_open_cb;
	sr_resource_close_callback resource_close_cb;
	sr_resource_read_callback resource_read_cb;
	void *resource_cb_data;
};



/** USB device instance */
struct  sr_usb_dev_inst {
	/** libusb device handle */
	struct libusb_device_handle *devhdl;
};


/* Private driver context. */
struct drv_context {
	/** sigrok context */
	struct sr_context *sr_ctx;
	GSList *instances;
};

/*--- log.c -----------------------------------------------------------------*/

SR_PRIV int sr_log(int loglevel, const char *format, ...) G_GNUC_PRINTF(2, 3);

/* Message logging helpers with subsystem-specific prefix string. */
#define sr_spew(...)	sr_log(SR_LOG_SPEW, LOG_PREFIX ": " __VA_ARGS__)
#define sr_dbg(...)	sr_log(SR_LOG_DBG,  LOG_PREFIX ": " __VA_ARGS__)
#define sr_info(...)	sr_log(SR_LOG_INFO, LOG_PREFIX ": " __VA_ARGS__)
#define sr_warn(...)	sr_log(SR_LOG_WARN, LOG_PREFIX ": " __VA_ARGS__)
#define sr_err(...)	sr_log(SR_LOG_ERR,  LOG_PREFIX ": " __VA_ARGS__)

/*--- device.c --------------------------------------------------------------*/


/** Device instance data */
struct sr_dev_inst {
    /** Device id **/
    int id;
    /** Device callback */
    sr_callback_t cb;
	/** Device driver. */
	struct sr_dev_driver *driver;
	/** Device instance status. SR_ST_NOT_FOUND, etc. */
	int status;
	/** Connection string to uniquely identify devices. */
	char *connection_id;
	/** Device instance connection data (used?) */
	//void *conn;
	struct sr_usb_dev_inst *conn;
	/** Device instance private data (used?) */
	struct dev_context *ctx;
};

SR_PRIV int usb_get_port_path(libusb_device *dev, char *path, int path_len);

/*--- resource.c ------------------------------------------------------------*/

SR_PRIV int64_t sr_file_get_size(FILE *file);

SR_PRIV int sr_resource_open(struct sr_context *ctx,
		struct sr_resource *res, int type, const char *name)
		G_GNUC_WARN_UNUSED_RESULT;
SR_PRIV int sr_resource_close(struct sr_context *ctx,
		struct sr_resource *res);
SR_PRIV ssize_t sr_resource_read(struct sr_context *ctx,
		const struct sr_resource *res, void *buf, size_t count)
		G_GNUC_WARN_UNUSED_RESULT;
SR_PRIV void *sr_resource_load(struct sr_context *ctx, int type,
		const char *name, size_t *size, size_t max_size)
		G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;

/*--- hardware/ezusb.c ------------------------------------------------------*/

SR_PRIV int ezusb_reset(struct libusb_device_handle *hdl, int set_clear);
SR_PRIV int ezusb_install_firmware(struct sr_context *ctx, libusb_device_handle *hdl, const char *name);
SR_PRIV int ezusb_upload_firmware(struct sr_context *ctx, libusb_device *dev, int configuration, const char *name);


#endif

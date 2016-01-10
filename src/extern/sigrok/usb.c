/*
 * This file is part of the libsigrok project.
 *
 * Copyright (C) 2012 Uwe Hermann <uwe@hermann-uwe.de>
 * Copyright (C) 2012 Bert Vermeulen <bert@biot.com>
 * Copyright (C) 2015 Daniel Elstner <daniel.kitta@gmail.com>
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

#include <stdlib.h>
#include <glib.h>
#include <libusb.h>
#include "libsigrok.h"
#include "libsigrok-internal.h"


#define LOG_PREFIX "usb"


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

/** USB event source prepare() method.
 */
gboolean usb_source_prepare(GSource *source, int *timeout)
{
	int64_t now_us, usb_due_us;
	struct usb_source *usource;
	struct timeval usb_timeout;
	int remaining_ms;
	int ret;

	usource = (struct usb_source *)source;

	ret = libusb_get_next_timeout(usource->usb_ctx, &usb_timeout);
	if (G_UNLIKELY(ret < 0)) {
		sr_err("Failed to get libusb timeout: %s",
			libusb_error_name(ret));
	}
	now_us = g_source_get_time(source);

	if (usource->due_us == 0) {
		/* First-time initialization of the expiration time */
		usource->due_us = now_us + usource->timeout_us;
	}
	if (ret == 1) {
		usb_due_us = (int64_t)usb_timeout.tv_sec * G_USEC_PER_SEC
				+ usb_timeout.tv_usec + now_us;
		if (usb_due_us < usource->due_us)
			usource->due_us = usb_due_us;
	}
	if (usource->due_us != INT64_MAX)
		remaining_ms = (MAX(0, usource->due_us - now_us) + 999) / 1000;
	else
		remaining_ms = -1;

	*timeout = remaining_ms;

	return (remaining_ms == 0);
}

/** USB event source dispatch() method.
 */

gboolean usb_source_dispatch(GSource *source, GSourceFunc callback, void *user_data)
{
	struct usb_source *usource;
	GPollFD *pollfd;
	unsigned int revents;
	unsigned int i;
	gboolean keep;

	usource = (struct usb_source *)source;
	revents = 0;
	/*
	 * This is somewhat arbitrary, but drivers use revents to distinguish
	 * actual I/O from timeouts. When we remove the user timeout from the
	 * driver API, this will no longer be needed.
	 */
	for (i = 0; i < usource->pollfds->len; i++) {
		pollfd = g_ptr_array_index(usource->pollfds, i);
		revents |= pollfd->revents;
	}
	if (revents != 0)
		sr_spew("%s: revents 0x%.2X", __func__, revents);
	else/* SR_CONF_CONN takes one of these: */
		sr_spew("%s: timed out", __func__);

	if (!callback) {
		sr_err("Callback not set, cannot dispatch event.");
		return G_SOURCE_REMOVE;
	}
	keep = (*(sr_receive_data_callback)callback)(-1, revents, user_data);

	if (G_LIKELY(keep) && G_LIKELY(!g_source_is_destroyed(source))) {
		if (usource->timeout_us >= 0)
			usource->due_us = g_source_get_time(source)
					+ usource->timeout_us;
		else
			usource->due_us = INT64_MAX;
	}
	return keep;
}

/** USB event source check() method.
 */
gboolean usb_source_check(GSource *source)
{
    struct usb_source *usource;
    GPollFD *pollfd;
    unsigned int revents;
    unsigned int i;

    usource = (struct usb_source *)source;
    revents = 0;

    for (i = 0; i < usource->pollfds->len; i++) {
        pollfd = g_ptr_array_index(usource->pollfds, i);
        revents |= pollfd->revents;
    }
    return (revents != 0 || (usource->due_us != INT64_MAX
                             && usource->due_us <= g_source_get_time(source)));
}

SR_PRIV int usb_get_port_path(libusb_device *dev, char *path, int path_len)
{
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

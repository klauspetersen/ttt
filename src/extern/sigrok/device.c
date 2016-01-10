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

#include <stdio.h>
#include <glib.h>
#include "libsigrok.h"
#include "libsigrok-internal.h"

/** @cond PRIVATE */
#define LOG_PREFIX "device"
/** @endcond */

/**
 * @file
 *
 * Device handling in libsigrok.
 */

/**
 * @defgroup grp_devices Devices
 *
 * Device handling in libsigrok.
 *
 * @{
 */

/** @private
 *  Allocate and initialize new struct sr_channel and add to sdi.
 *  @param[in]  sdi The device instance the channel is connected to.
 *  @param[in]  index @copydoc sr_channel::index
 *  @param[in]  type @copydoc sr_channel::type
 *  @param[in]  enabled @copydoc sr_channel::enabled
 *  @param[in]  name @copydoc sr_channel::name
 *
 *  @return A new struct sr_channel*.
 */
SR_PRIV struct sr_channel *sr_channel_new(struct sr_dev_inst *sdi,
		int index, int type, gboolean enabled, const char *name)
{
	struct sr_channel *ch;

	ch = g_malloc0(sizeof(struct sr_channel));
	ch->sdi = sdi;
	ch->index = index;
	ch->type = type;
	ch->enabled = enabled;
	if (name)
		ch->name = g_strdup(name);

	sdi->channels = g_slist_append(sdi->channels, ch);

	return ch;
}


/** @private
 *  Allocate and init struct for USB device instance.
 *  @param[in]  bus @copydoc sr_usb_dev_inst::bus
 *  @param[in]  address @copydoc sr_usb_dev_inst::address
 *  @param[in]  hdl @copydoc sr_usb_dev_inst::devhdl
 *
 *  @retval other struct sr_usb_dev_inst * for USB device instance.
 */
SR_PRIV struct sr_usb_dev_inst *sr_usb_dev_inst_new(uint8_t bus,
			uint8_t address, struct libusb_device_handle *hdl)
{
	struct sr_usb_dev_inst *udi;

	udi = g_malloc0(sizeof(struct sr_usb_dev_inst));
	udi->bus = bus;
	udi->address = address;
	udi->devhdl = hdl;

	return udi;
}

/** @private
 *  Free struct * allocated by sr_usb_dev_inst().
 *  @param usb  struct* to free. Must not be NULL.
 */
SR_PRIV void sr_usb_dev_inst_free(struct sr_usb_dev_inst *usb)
{
	g_free(usb);
}

/*
 * This file is part of the libsigrok project.
 *
 * Copyright (C) 2013 Uwe Hermann <uwe@hermann-uwe.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

/** @file
  * Standard API helper functions.
  * @internal
  */

#include <glib.h>
#include "libsigrok.h"
#include "libsigrok-internal.h"

#define LOG_PREFIX "std"

/**
 * Standard sr_driver_init() API helper.
 *
 * This function can be used to simplify most driver's init() API callback.
 *
 * It creates a new 'struct drv_context' (drvc), assigns sr_ctx to it, and
 * then 'drvc' is assigned to the 'struct sr_dev_driver' (di) that is passed.
 *
 * @param sr_ctx The libsigrok context to assign.
 * @param di The driver instance to use.
 * @param[in] prefix A driver-specific prefix string used for log messages.
 *
 * @return SR_OK upon success, SR_ERR_ARG upon invalid arguments.
 */
SR_PRIV int std_init(struct sr_context *sr_ctx, struct sr_dev_driver *di,
		     const char *prefix)
{
	struct drv_context *drvc;

	if (!di) {
		sr_err("%s: Invalid driver, cannot initialize.", prefix);
		return SR_ERR_ARG;
	}

	drvc = g_malloc0(sizeof(struct drv_context));
	drvc->sr_ctx = sr_ctx;
	drvc->instances = NULL;
	di->context = drvc;

	return SR_OK;
}



/**
 * Standard driver dev_clear() helper.
 *
 * Clear driver, this means, close all instances.
 *
 * This function can be used to implement the dev_clear() driver API
 * callback. dev_close() is called before every sr_dev_inst is cleared.
 *
 * The only limitation is driver-specific device contexts (sdi->priv).
 * These are freed, but any dynamic allocation within structs stored
 * there cannot be freed.
 *
 * @param driver The driver which will have its instances released.
 * @param clear_private If not NULL, this points to a function called
 * with sdi->priv as argument. The function can then clear any device
 * instance-specific resources kept there. It must also clear the struct
 * pointed to by sdi->priv.
 *
 * @return SR_OK on success.
 */
SR_PRIV int std_dev_clear(const struct sr_dev_driver *driver,
		std_dev_clear_callback clear_private)
{
	struct drv_context *drvc;
	struct sr_dev_inst *sdi;
	GSList *l;
	int ret;

	if (!(drvc = driver->context))
		/* Driver was never initialized, nothing to do. */
		return SR_OK;

	ret = SR_OK;
	for (l = drvc->instances; l; l = l->next) {
		if (!(sdi = l->data)) {
			ret = SR_ERR_BUG;
			continue;
		}
		if (driver->dev_close)
			driver->dev_close(sdi);

		if (sdi->conn) {
			if (sdi->inst_type == SR_INST_USB)
				sr_usb_dev_inst_free(sdi->conn);
		}
		if (clear_private)
			/* The helper function is responsible for freeing
			 * its own sdi->priv! */
			clear_private(sdi->priv);
		else
			g_free(sdi->priv);

		sr_dev_inst_free(sdi);
	}

	g_slist_free(drvc->instances);
	drvc->instances = NULL;

	return ret;
}

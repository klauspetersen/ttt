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

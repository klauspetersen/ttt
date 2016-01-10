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

#include <glib.h>
#include "libsigrok.h"
#include "libsigrok-internal.h"

/** @cond PRIVATE */
#define LOG_PREFIX "hwdriver"
/** @endcond */


/**
 * Initialize a hardware driver.
 *
 * This usually involves memory allocations and variable initializations
 * within the driver, but _not_ scanning for attached devices.
 * The API call sr_driver_scan() is used for that.
 *
 * @param ctx A libsigrok context object allocated by a previous call to
 *            sr_init(). Must not be NULL.
 * @param driver The driver to initialize. This must be a pointer to one of
 *               the entries returned by sr_driver_list(). Must not be NULL.
 *
 * @retval SR_OK Success
 * @retval SR_ERR_ARG Invalid parameter(s).
 * @retval SR_ERR_BUG Internal errors.
 * @retval other Another negative error code upon other errors.
 *
 * @since 0.2.0
 */
SR_API int sr_driver_init(struct sr_context *ctx, struct sr_dev_driver *driver)
{
	int ret;

	if (!ctx) {
		sr_err("Invalid libsigrok context, can't initialize.");
		return SR_ERR_ARG;
	}

	if (!driver) {
		sr_err("Invalid driver, can't initialize.");
		return SR_ERR_ARG;
	}

	sr_spew("Initializing driver '%s'.", driver->name);
	if ((ret = driver->init(driver, ctx)) < 0)
		sr_err("Failed to initialize the driver: %d.", ret);

	return ret;
}

/** @} */

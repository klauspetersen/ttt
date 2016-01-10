/*
 * This file is part of the libsigrok project.
 *
 * Copyright (C) 2011-2012 Uwe Hermann <uwe@hermann-uwe.de>
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

#include <stdarg.h>
#include <stdio.h>
#include <glib/gprintf.h>
#include "libsigrok.h"
#include "libsigrok-internal.h"

#define LOG_PREFIX "log"

/**
 * @file
 *
 * Controlling the libsigrok message logging functionality.
 */

/**
 * @defgroup grp_logging Logging
 *
 * Controlling the libsigrok message logging functionality.
 *
 * @{
 */

/* Currently selected libsigrok loglevel. Default: SR_LOG_WARN. */
static int cur_loglevel = SR_LOG_DBG; /* Show errors+warnings per default. */

/* Function prototype. */
static int sr_logv(void *cb_data, int loglevel, const char *format,
		   va_list args);

/* Pointer to the currently selected log callback. Default: sr_logv(). */
static sr_log_callback sr_log_cb = sr_logv;

/*
 * Pointer to private data that can be passed to the log callback.
 * This can be used (for example) by C++ GUIs to pass a "this" pointer.
 */
static void *sr_log_cb_data = NULL;

/** @cond PRIVATE */
#define LOGLEVEL_TIMESTAMP SR_LOG_DBG
/** @endcond */
static int64_t sr_log_start_time = 0;

static int sr_logv(void *cb_data, int loglevel, const char *format, va_list args)
{
	uint64_t elapsed_us, minutes;
	unsigned int rest_us, seconds, microseconds;
	int ret;

	/* This specific log callback doesn't need the void pointer data. */
	(void)cb_data;

	/* Only output messages of at least the selected loglevel(s). */
	if (loglevel > cur_loglevel)
		return SR_OK;

	if (cur_loglevel >= LOGLEVEL_TIMESTAMP) {
		elapsed_us = g_get_monotonic_time() - sr_log_start_time;

		minutes = elapsed_us / G_TIME_SPAN_MINUTE;
		rest_us = elapsed_us % G_TIME_SPAN_MINUTE;
		seconds = rest_us / G_TIME_SPAN_SECOND;
		microseconds = rest_us % G_TIME_SPAN_SECOND;

		ret = g_fprintf(stderr, "sr: [%.2" PRIu64 ":%.2u.%.6u] ",
				minutes, seconds, microseconds);
	} else {
		ret = fputs("sr: ", stderr);
	}

	if (ret < 0 || g_vfprintf(stderr, format, args) < 0
			|| putc('\n', stderr) < 0)
		return SR_ERR;

	return SR_OK;
}

/** @private */
SR_PRIV int sr_log(int loglevel, const char *format, ...)
{
	int ret;
	va_list args;
        
	va_start(args, format);
	ret = sr_log_cb(sr_log_cb_data, loglevel, format, args);
	va_end(args);

	return ret;
}

/** @} */

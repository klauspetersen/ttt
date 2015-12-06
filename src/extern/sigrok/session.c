/*
 * This file is part of the libsigrok project.
 *
 * Copyright (C) 2010-2012 Bert Vermeulen <bert@biot.com>
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

#include <stdio.h>
#include <glib.h>
#include "libsigrok.h"
#include "libsigrok-internal.h"

/** @cond PRIVATE */
#define LOG_PREFIX "session"
/** @endcond */


/**
 * @file
 *
 * Creating, using, or destroying libsigrok sessions.
 */


/**
 * Create a new session.
 *
 * @param ctx         The context in which to create the new session.
 * @param new_session This will contain a pointer to the newly created
 *                    session if the return value is SR_OK, otherwise the value
 *                    is undefined and should not be used. Must not be NULL.
 *
 * @retval SR_OK Success.
 * @retval SR_ERR_ARG Invalid argument.
 *
 * @since 0.4.0
 */
SR_API int sr_session_new(struct sr_context *ctx,
		struct sr_session **new_session)
{
	struct sr_session *session;


	session = g_malloc0(sizeof(struct sr_session));

	session->ctx = ctx;

	/* To maintain API compatibility, we need a lookup table
	 * which maps poll_object IDs to GSource* pointers.
	 */
	session->event_sources = g_hash_table_new(NULL, NULL);

	*new_session = session;

	return SR_OK;
}


static unsigned int session_source_attach(struct sr_session *session,
		GSource *source)
{
	unsigned int id = 0;

	g_mutex_lock(&session->main_mutex);

	if (session->main_context)
		id = g_source_attach(source, session->main_context);
	else
		sr_err("Cannot add event source without main context.");

	g_mutex_unlock(&session->main_mutex);

	return id;
}



/**
 * Add an event source for a file descriptor.
 *
 * @param session The session to use. Must not be NULL.
 * @param key The key which identifies the event source.
 * @param source An event source object. Must not be NULL.
 *
 * @retval SR_OK Success.
 * @retval SR_ERR_ARG Invalid argument.
 * @retval SR_ERR_BUG Event source with @a key already installed.
 * @retval SR_ERR Other error.
 *
 * @private
 */
SR_PRIV int sr_session_source_add_internal(struct sr_session *session,
		void *key, GSource *source)
{
	/*
	 * This must not ever happen, since the source has already been
	 * created and its finalize() method will remove the key for the
	 * already installed source. (Well it would, if we did not have
	 * another sanity check there.)
	 */
	if (g_hash_table_contains(session->event_sources, key)) {
		sr_err("Event source with key %p already exists.", key);
		return SR_ERR_BUG;
	}
	g_hash_table_insert(session->event_sources, key, source);

	if (session_source_attach(session, source) == 0)
		return SR_ERR;

	return SR_OK;
}


/** @} */

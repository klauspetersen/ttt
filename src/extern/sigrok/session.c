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

	if (!new_session)
		return SR_ERR_ARG;

	session = g_malloc0(sizeof(struct sr_session));

	session->ctx = ctx;

	g_mutex_init(&session->main_mutex);

	/* To maintain API compatibility, we need a lookup table
	 * which maps poll_object IDs to GSource* pointers.
	 */
	session->event_sources = g_hash_table_new(NULL, NULL);

	*new_session = session;

	return SR_OK;
}

/**
 * Add a device instance to a session.
 *
 * @param session The session to add to. Must not be NULL.
 * @param sdi The device instance to add to a session. Must not
 *            be NULL. Also, sdi->driver and sdi->driver->dev_open must
 *            not be NULL.
 *
 * @retval SR_OK Success.
 * @retval SR_ERR_ARG Invalid argument.
 *
 * @since 0.4.0
 */
SR_API int sr_session_dev_add(struct sr_session *session, struct sr_dev_inst *sdi){
	if (!sdi) {
		sr_err("%s: sdi was NULL", __func__);
		return SR_ERR_ARG;
	}

	if (!session) {
		sr_err("%s: session was NULL", __func__);
		return SR_ERR_ARG;
	}

	/* If sdi->session is not NULL, the device is already in this or
	 * another session. */
	if (sdi->session) {
		sr_err("%s: already assigned to session", __func__);
		return SR_ERR_ARG;
	}

	/* If sdi->driver is NULL, this is a virtual device. */
	if (!sdi->driver) {
		/* Just add the device, don't run dev_open(). */
		session->devs = g_slist_append(session->devs, sdi);
		sdi->session = session;
		return SR_OK;
	}

	/* sdi->driver is non-NULL (i.e. we have a real device). */
	if (!sdi->driver->dev_open) {
		sr_err("%s: sdi->driver->dev_open was NULL", __func__);
		return SR_ERR_BUG;
	}

	session->devs = g_slist_append(session->devs, sdi);
	sdi->session = session;

	return SR_OK;
}


/** Set up the main context the session will be executing in.
 *
 * Must be called just before the session starts, by the thread which
 * will execute the session main loop. Once acquired, the main context
 * pointer is immutable for the duration of the session run.
 */
static int set_main_context(struct sr_session *session)
{
	GMainContext *main_context;

	g_mutex_lock(&session->main_mutex);

	/* May happen if sr_session_start() is called a second time
	 * while the session is still running.
	 */
	if (session->main_context) {
		sr_err("Main context already set.");

		g_mutex_unlock(&session->main_mutex);
		return SR_ERR;
	}
	main_context = g_main_context_ref_thread_default();
	/*
	 * Try to use an existing main context if possible, but only if we
	 * can make it owned by the current thread. Otherwise, create our
	 * own main context so that event source callbacks can execute in
	 * the session thread.
	 */
	if (g_main_context_acquire(main_context)) {
		g_main_context_release(main_context);

		sr_dbg("Using thread-default main context.");
	} else {
		g_main_context_unref(main_context);

		sr_dbg("Creating our own main context.");
		main_context = g_main_context_new();
	}
	session->main_context = main_context;

	g_mutex_unlock(&session->main_mutex);

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
 * Start a session.
 *
 * When this function returns with a status code indicating success, the
 * session is running. Use sr_session_stopped_callback_set() to receive
 * notification upon completion, or call sr_session_run() to block until
 * the session stops.
 *
 * Session events will be processed in the context of the current thread.
 * If a thread-default GLib main context has been set, and is not owned by
 * any other thread, it will be used. Otherwise, libsigrok will create its
 * own main context for the current thread.
 *
 * @param session The session to use. Must not be NULL.
 *
 * @retval SR_OK Success.
 * @retval SR_ERR_ARG Invalid session passed.
 * @retval SR_ERR Other error.
 *
 * @since 0.4.0
 */
SR_API int sr_session_start(struct sr_session *session)
{
	struct sr_dev_inst *sdi;
	struct sr_channel;
	GSList *l;
	int ret;

	set_main_context(session);

	sr_info("Starting.");

	/* Have all devices start acquisition. */
	for (l = session->devs; l; l = l->next) {
            sdi = l->data;
            ret = sdi->driver->dev_acquisition_start(sdi, sdi);
            if (ret != SR_OK) {
                sr_err("Could not start %s device %s acquisition.", sdi->driver->name, sdi->connection_id);
                break;
            } else {
                sr_info("Device started.");
            }
	}

	/* Have all devices fire. */
	for (l = session->devs; l; l = l->next) {
            sdi = l->data;
            ret = sdi->driver->dev_acquisition_trigger(sdi);
            if (ret != SR_OK) {
                sr_err("Could not fire %s device %s acquisition.", sdi->driver->name, sdi->connection_id);
                break;
            } else {
                sr_info("Device fired.");
            }
	}

	return SR_OK;
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

/*
 * This file is part of the libsigrok project.
 *
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

#include <errno.h>
#include <stdio.h>
#include <glib.h>
#include <glib/gstdio.h>
#include "libsigrok.h"
#include "libsigrok-internal.h"

/** @cond PRIVATE */
#define LOG_PREFIX "resource"
/** @endcond */

/**
 * @file
 *
 * Access to resource files.
 */

/** Retrieve the size of the open stream @a file.
 *
 * This function only works on seekable streams. However, the set of seekable
 * streams is generally congruent with the set of streams that have a size.
 * Code that needs to work with any type of stream (including pipes) should
 * require neither seekability nor advance knowledge of the size.
 * On failure, the return value is negative and errno is set.
 *
 * @param file An I/O stream opened in binary mode.
 * @return The size of @a file in bytes, or a negative value on failure.
 *
 * @private
 */
SR_PRIV int64_t sr_file_get_size(FILE *file)
{
	off_t filepos, filesize;

	/* ftello() and fseeko() are not standard C, but part of POSIX.1-2001.
	 * Thus, if these functions are available at all, they can reasonably
	 * be expected to also conform to POSIX semantics. In particular, this
	 * means that ftello() after fseeko(..., SEEK_END) has a defined result
	 * and can be used to get the size of a seekable stream.
	 * On Windows, the result is fully defined only for binary streams.
	 */
	filepos = ftello(file);
	if (filepos < 0)
		return -1;

	if (fseeko(file, 0, SEEK_END) < 0)
		return -1;

	filesize = ftello(file);
	if (filesize < 0)
		return -1;

	if (fseeko(file, filepos, SEEK_SET) < 0)
		return -1;

	return filesize;
}

static FILE *try_open_file(const char *datadir, const char *subdir,	const char *name){
	char *filename;
	FILE *file;

	filename = g_build_filename(datadir, subdir, name, NULL);
	file = g_fopen(filename, "rb");

	if (file)
		sr_info("Opened '%s'.", filename);
	else
		sr_spew("Attempt to open '%s' failed: %s",
			filename, g_strerror(errno));
	g_free(filename);

	return file;
}

int resource_open_default(struct sr_resource *res,	const char *name, void *cb_data){
	int64_t filesize;
#ifdef FIRMWARE_DIR
	const char *builtindir;
#endif
	const char *subdir;
	const char *const *datadirs;
	FILE *file;

	(void)cb_data;

	switch (res->type) {
	case SR_RESOURCE_FIRMWARE:
#ifdef FIRMWARE_DIR
		builtindir = FIRMWARE_DIR;
#endif
		subdir = "sigrok-firmware";
		break;
	default:
		sr_err("%s: unknown type %d.", __func__, res->type);
		return SR_ERR_ARG;
	}

	file = try_open_file(g_get_user_data_dir(), subdir, name);
	/*
	 * Scan the hard-coded directory before the system directories to
	 * avoid picking up possibly outdated files from a system install.
	 */
#ifdef FIRMWARE_DIR
	if (!file)
		file = try_open_file(builtindir, "", name);
#endif
	if (!file) {
		datadirs = g_get_system_data_dirs();
		while (*datadirs && !file)
			file = try_open_file(*datadirs++, subdir, name);
	}
	if (!file) {
		sr_err("Failed to locate '%s'.", name);
		return SR_ERR;
	}

	filesize = sr_file_get_size(file);
	if (filesize < 0) {
		sr_err("Failed to obtain size of '%s': %s",
			name, g_strerror(errno));
		fclose(file);
		return SR_ERR;
	}
	res->size = filesize;
	res->handle = file;

	return SR_OK;
}

int resource_close_default(struct sr_resource *res, void *cb_data){
	FILE *file;

	(void)cb_data;

	file = res->handle;
	if (!file) {
		sr_err("%s: invalid handle.", __func__);
		return SR_ERR_ARG;
	}

	if (fclose(file) < 0) {
		sr_err("Failed to close file: %s", g_strerror(errno));
		return SR_ERR;
	}
	res->handle = NULL;

	return SR_OK;
}

ssize_t resource_read_default(const struct sr_resource *res, void *buf, size_t count, void *cb_data){
	FILE *file;
	size_t n_read;

	(void)cb_data;

	file = res->handle;
	if (!file) {
		sr_err("%s: invalid handle.", __func__);
		return SR_ERR_ARG;
	}
	if (count > SSIZE_MAX) {
		sr_err("%s: count %zu too large.", __func__, count);
		return SR_ERR_ARG;
	}

	n_read = fread(buf, 1, count, file);

	if (n_read != count && ferror(file)) {
		sr_err("Failed to read resource file: %s", g_strerror(errno));
		return SR_ERR;
	}
	return n_read;
}

/**
 * Install resource access hooks.
 *
 * @param ctx libsigrok context. Must not be NULL.
 * @param open_cb Resource open callback, or NULL to unset.
 * @param close_cb Resource close callback, or NULL to unset.
 * @param read_cb Resource read callback, or NULL to unset.
 * @param cb_data User data pointer passed to callbacks.
 *
 * @retval SR_OK Success.
 * @retval SR_ERR_ARG Invalid argument.
 *
 * @since 0.4.0
 */
SR_API int sr_resource_set_hooks(struct sr_context *ctx,
		sr_resource_open_callback open_cb,
		sr_resource_close_callback close_cb,
		sr_resource_read_callback read_cb, void *cb_data)
{
	if (!ctx) {
		sr_err("%s: ctx was NULL.", __func__);
		return SR_ERR_ARG;
	}
	if (open_cb && close_cb && read_cb) {
		ctx->resource_open_cb  = open_cb;
		ctx->resource_close_cb = close_cb;
		ctx->resource_read_cb  = read_cb;
		ctx->resource_cb_data  = cb_data;
	} else if (!open_cb && !close_cb && !read_cb) {
		ctx->resource_open_cb  = &resource_open_default;
		ctx->resource_close_cb = &resource_close_default;
		ctx->resource_read_cb  = &resource_read_default;
		ctx->resource_cb_data  = ctx;
	} else {
		sr_err("%s: inconsistent callback pointers.", __func__);
		return SR_ERR_ARG;
	}
	return SR_OK;
}

/**
 * Open resource.
 *
 * @param ctx libsigrok context. Must not be NULL.
 * @param[out] res Resource descriptor to fill in. Must not be NULL.
 * @param type Resource type ID.
 * @param name Name of the resource. Must not be NULL.
 *
 * @retval SR_OK Success.
 * @retval SR_ERR_ARG Invalid argument.
 * @retval SR_ERR Other error.
 *
 * @private
 */
SR_PRIV int sr_resource_open(struct sr_context *ctx,
		struct sr_resource *res, int type, const char *name)
{
	int ret;

	res->size = 0;
	res->handle = NULL;
	res->type = type;

	ret = (*ctx->resource_open_cb)(res, name, ctx->resource_cb_data);

	if (ret != SR_OK)
		sr_err("Failed to open resource '%s'.", name);

	return ret;
}

/**
 * Close resource.
 *
 * @param ctx libsigrok context. Must not be NULL.
 * @param[inout] res Resource descriptor. Must not be NULL.
 *
 * @retval SR_OK Success.
 * @retval SR_ERR_ARG Invalid argument.
 * @retval SR_ERR Other error.
 *
 * @private
 */
SR_PRIV int sr_resource_close(struct sr_context *ctx, struct sr_resource *res)
{
	int ret;

	ret = (*ctx->resource_close_cb)(res, ctx->resource_cb_data);

	if (ret != SR_OK)
		sr_err("Failed to close resource.");

	return ret;
}

/**
 * Read resource data.
 *
 * @param ctx libsigrok context. Must not be NULL.
 * @param[in] res Resource descriptor. Must not be NULL.
 * @param[out] buf Buffer to store @a count bytes into. Must not be NULL.
 * @param count Number of bytes to read.
 *
 * @return The number of bytes actually read, or a negative value on error.
 * @retval SR_ERR_ARG Invalid argument.
 * @retval SR_ERR Other error.
 *
 * @private
 */
SR_PRIV ssize_t sr_resource_read(struct sr_context *ctx,
		const struct sr_resource *res, void *buf, size_t count)
{
	ssize_t n_read;

	n_read = (*ctx->resource_read_cb)(res, buf, count,
			ctx->resource_cb_data);
	if (n_read < 0)
		sr_err("Failed to read resource.");

	return n_read;
}

/**
 * Load a resource into memory.
 *
 * @param ctx libsigrok context. Must not be NULL.
 * @param type Resource type ID.
 * @param name Name of the resource. Must not be NULL.
 * @param[out] size Size in bytes of the returned buffer. Must not be NULL.
 * @param max_size Size limit. Error out if the resource is larger than this.
 *
 * @return A buffer containing the resource data, or NULL on failure. Must
 *  be freed by the caller using g_free().
 *
 * @private
 */
SR_PRIV void *sr_resource_load(struct sr_context *ctx, int type, const char *name, size_t *size, size_t max_size){
	struct sr_resource res;
	void *buf;
	size_t res_size;
	ssize_t n_read;

	if (sr_resource_open(ctx, &res, type, name) != SR_OK)
		return NULL;

	if (res.size > max_size) {
		sr_err("Size %" PRIu64 " of '%s' exceeds limit %zu.",
			res.size, name, max_size);
		sr_resource_close(ctx, &res);
		return NULL;
	}
	res_size = res.size;

	buf = g_try_malloc(res_size);
	if (!buf) {
		sr_err("Failed to allocate buffer for '%s'.", name);
		sr_resource_close(ctx, &res);
		return NULL;
	}

	n_read = sr_resource_read(ctx, &res, buf, res_size);
	sr_resource_close(ctx, &res);

	if (n_read < 0 || (size_t)n_read != res_size) {
		if (n_read >= 0)
			sr_err("Failed to read '%s': premature end of file.",
				name);
		g_free(buf);
		return NULL;
	}

	*size = res_size;
	return buf;
}

/** @} */

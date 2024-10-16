/*
 * FastRPC reverse tunnel
 *
 * Copyright (C) 2023 The Sensor Shell Contributors
 *
 * This file is part of sensh.
 *
 * Sensh is free software: you can redistribute it and/or modify
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <inttypes.h>
#include <libhexagonrpc/error.h>
#include <libhexagonrpc/fastrpc.h>
#include <libhexagonrpc/interfaces/remotectl.def>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "interfaces/adsp_listener.def"
#include "iobuffer.h"
#include "listener.h"

static int check_buf_sizes(const struct hrpc_method_def_interp3 *def,
			   uint8_t n_inbufs, uint8_t n_outbufs,
			   const struct fastrpc_io_buffer *inbufs)
{
	const uint32_t *prim_in = inbufs[0].p;
	uint8_t n_def_outbufs;
	size_t i, j = 1;
	size_t off;
	size_t n_prim_in;

	if (def->msg_id < 31)
		off = 0;
	else
		off = 1;

	for (i = 0; i < def->n_args; i++) {
		if (def->args[i] == HEXAGONRPC_DELIMITER)
			break;

		if (!def->args[i])
			continue;

		/*
		 * We can't check the size because the primary input buffer is
		 * too small, so skip checking and print an error later.
		 */
		if ((off + i) * sizeof(uint32_t) >= inbufs[0].s)
			continue;

		if (j >= n_inbufs) {
			fprintf(stderr, "listener: not enough input buffers\n");
			return 1;
		}

		if ((size_t) prim_in[off + i] * def->args[i] > inbufs[j].s) {
			fprintf(stderr, "listener: input buffer too small\n");
			return 1;
		}

		j++;
	}

	n_prim_in = i;
	if (def->args[i] == HEXAGONRPC_DELIMITER)
		i++;

	if (def->has_prim_out)
		n_def_outbufs = 1;
	else
		n_def_outbufs = 0;

	for (; i < def->n_args; i++) {
		if (def->args[i]) {
			n_def_outbufs++;
			n_prim_in++;
		}
	}

	if (n_def_outbufs != n_outbufs) {
		fprintf(stderr, "listener: wrong amount of output buffers\n");
		return 1;
	}

	if ((off + n_prim_in) * sizeof(uint32_t) > inbufs[0].s) {
		fprintf(stderr, "listener: primary input buffer too small\n");
		return 1;
	}

	return 0;
}

static struct fastrpc_io_buffer *allocate_outbufs(const struct hrpc_method_def_interp3 *def,
						  const uint32_t *prim_in)
{
	struct fastrpc_io_buffer *out;
	// This is only used when there is a delimiter.
	const uint32_t *size = NULL;
	size_t n_outbufs, n_prim_out = 0;
	size_t i;

	out = malloc(sizeof(struct fastrpc_io_buffer) * def->n_args);
	if (out == NULL)
		return NULL;

	if (def->has_prim_out)
		n_outbufs = 1;
	else
		n_outbufs = 0;

	for (i = 0; i < def->n_args; i++) {
		if (def->args[i] == HEXAGONRPC_DELIMITER) {
			size = &prim_in[i];
			i++;
			break;
		}
	}

	for (; i < def->n_args; i++) {
		if (def->args[i]) {
			out[n_outbufs].s = (size_t) def->args[i] * *size;
			out[n_outbufs].p = malloc(out[n_outbufs].s);
			if (out[n_outbufs].p == NULL && out[n_outbufs].s)
				goto err_free_outbufs;

			n_outbufs++;
			size = &size[1];
		} else {
			n_prim_out++;
		}
	}

	if (def->has_prim_out) {
		out[0].s = sizeof(uint32_t) * n_prim_out;
		out[0].p = malloc(out[0].s);
		if (out[0].p == NULL && out[0].s)
			goto err_free_outbufs;
	}

	return out;

err_free_outbufs:
	if (!def->has_prim_out)
		free(out[0].p);

	for (i = 1; i < n_outbufs; i++)
		free(out[i].p);

	free(out);
	return NULL;
}

static int return_for_next_invoke(int fd,
				  uint32_t result,
				  uint32_t *rctx,
				  uint32_t *handle,
				  uint32_t *sc,
				  const struct fastrpc_io_buffer *returned,
				  struct fastrpc_io_buffer **decoded)
{
	struct fastrpc_decoder_context *ctx;
	char inbufs[256];
	char *outbufs = NULL;
	uint32_t inbufs_len;
	uint32_t outbufs_len;
	int ret;

	outbufs_len = outbufs_calculate_size(REMOTE_SCALARS_OUTBUFS(*sc), returned);

	if (outbufs_len) {
		outbufs = malloc(outbufs_len);
		if (outbufs == NULL) {
			perror("Could not allocate encoded output buffer");
			return -1;
		}

		outbufs_encode(REMOTE_SCALARS_OUTBUFS(*sc), returned, outbufs);
	}

	ret = adsp_listener3_next2(fd,
				  *rctx, result,
				  outbufs_len, outbufs,
				  rctx, handle, sc,
				  &inbufs_len, 256, inbufs);
	if (ret) {
		if (ret == -1)
			perror("Could not fetch next FastRPC message");
		else
			fprintf(stderr, "Could not fetch next FastRPC message: %d\n", ret);

		goto err_free_outbufs;
	}

	if (inbufs_len > 256) {
		fprintf(stderr, "Large (>256B) input buffers aren't implemented\n");
		ret = -1;
		goto err_free_outbufs;
	}

	ctx = inbuf_decode_start(*sc);
	if (!ctx) {
		perror("Could not start decoding");
		ret = -1;
		goto err_free_outbufs;
	}

	ret = inbuf_decode(ctx, inbufs_len, inbufs);
	if (ret) {
		perror("Could not decode");
		goto err_free_outbufs;
	}

	if (!inbuf_decode_is_complete(ctx)) {
		fprintf(stderr, "Expected more input buffers\n");
		ret = -1;
		goto err_free_outbufs;
	}

	*decoded = inbuf_decode_finish(ctx);

err_free_outbufs:
	free(outbufs);
	return ret;
}

static int invoke_requested_procedure(size_t n_ifaces,
				      struct fastrpc_interface **ifaces,
				      uint32_t handle,
				      uint32_t sc,
			              uint32_t *result,
				      const struct fastrpc_io_buffer *decoded,
				      struct fastrpc_io_buffer **returned)
{
	const struct fastrpc_function_impl *impl;
	uint32_t method = REMOTE_SCALARS_METHOD(sc);
	int ret;

	if (method < 31)
		method = REMOTE_SCALARS_METHOD(sc);
	else if (decoded[0].s >= sizeof(uint32_t))
		method = *(const uint32_t *) decoded[0].p;
	else
		method = UINT32_MAX;

	if (sc & 0xff) {
		fprintf(stderr, "Handles are not supported, but got %u in, %u out\n",
				(sc & 0xf0) >> 4, sc & 0xf);
		*result = AEE_EBADPARM;
		return 1;
	}

	if (handle >= n_ifaces) {
		fprintf(stderr, "Unsupported handle: %u\n", handle);
		*result = AEE_EUNSUPPORTED;
		return 1;
	}

	if (method >= ifaces[handle]->n_procs) {
		fprintf(stderr, "Unsupported method: %u (%08x)\n", method, sc);
		*result = AEE_EUNSUPPORTED;
		return 1;
	}

	impl = &ifaces[handle]->procs[method];

	if (impl->def == NULL || impl->impl == NULL) {
		fprintf(stderr, "Unsupported method: %u (%08x)\n", method, sc);
		*result = AEE_EUNSUPPORTED;
		return 1;
	}

	ret = check_buf_sizes(impl->def, REMOTE_SCALARS_INBUFS(sc),
			      REMOTE_SCALARS_OUTBUFS(sc), decoded);
	if (ret) {
		*result = AEE_EBADPARM;
		return 1;
	}

	*returned = allocate_outbufs(impl->def, decoded[0].p);
	if (*returned == NULL && impl->def->n_args > 0) {
		perror("Could not allocate output buffers");
		*result = AEE_ENOMEMORY;
		return 1;
	}

	*result = impl->impl(ifaces[handle]->data, decoded, *returned);

	return 0;
}

int run_fastrpc_listener(int fd,
			 size_t n_ifaces,
			 struct fastrpc_interface **ifaces)
{
	struct fastrpc_io_buffer *decoded = NULL,
				 *returned = NULL;
	uint32_t result = 0xffffffff;
	uint32_t handle;
	uint32_t rctx = 0;
	uint32_t sc = REMOTE_SCALARS_MAKE(0, 0, 0);
	uint32_t n_outbufs = 0;
	int ret;

	ret = adsp_listener3_init2(fd);
	if (ret) {
		fprintf(stderr, "Could not initialize the listener: %u\n", ret);
		return ret;
	}

	while (!ret) {
		ret = return_for_next_invoke(fd,
					     result, &rctx, &handle, &sc,
					     returned, &decoded);
		if (ret)
			break;

		if (returned != NULL)
			iobuf_free(n_outbufs, returned);

		ret = invoke_requested_procedure(n_ifaces, ifaces,
						 handle, sc, &result,
						 decoded, &returned);
		if (ret)
			break;

		if (decoded != NULL)
			iobuf_free(REMOTE_SCALARS_INBUFS(sc), decoded);

		n_outbufs = REMOTE_SCALARS_OUTBUFS(sc);
	}

	return ret;
}

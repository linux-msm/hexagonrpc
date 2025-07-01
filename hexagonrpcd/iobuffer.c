/*
 * FastRPC reverse tunnel - argument decoder
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

#include <libhexagonrpc/hexagonrpc.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "iobuffer.h"

#define MIN(x, y) (((x) < (y)) ? (x) : (y))

static size_t consume_size(struct fastrpc_decoder_context *ctx,
			   size_t len, const char *buf)
{
	size_t segment;

	segment = MIN(len, 4 - ctx->size_off);
	memcpy(&((char *) &ctx->size)[ctx->size_off], buf, segment);
	ctx->size_off = (ctx->size_off + segment) % 4;
	ctx->align = (ctx->align + segment) & 0x7;

	return segment;
}

static int try_populate_inbuf(struct fastrpc_decoder_context *ctx)
{
	void *buf;

	// If we still need to process the size, don't do anything yet.
	if (ctx->size_off)
		return 0;

	buf = malloc(ctx->size);
	if (buf == NULL)
		return -1;

	ctx->inbufs[ctx->idx].s = ctx->size;
	ctx->inbufs[ctx->idx].p = buf;

	return 0;
}

static size_t consume_alignment(struct fastrpc_decoder_context *ctx, size_t len)
{
	size_t segment = 0;

	if (ctx->align) {
		segment = MIN(len, 8 - ctx->align);
		ctx->align = (ctx->align + segment) & 0x7;
	}

	return segment;
}

static size_t consume_buf(struct fastrpc_decoder_context *ctx,
			  size_t len, const void *buf)
{
	size_t segment;
	void *dest;

	dest = (void *) &((char *) ctx->inbufs[ctx->idx].p)[ctx->buf_off];

	segment = MIN(len, ctx->size - ctx->buf_off);
	memcpy(dest, buf, segment);

	if (ctx->buf_off + segment >= ctx->size) {
		ctx->size = 0;
		ctx->buf_off = 0;
		ctx->idx++;
	} else {
		ctx->buf_off = ctx->buf_off + segment;
	}

	ctx->align = (ctx->align + segment) & 0x7;

	return segment;
}

static size_t align_and_copy_outbuf(const struct fastrpc_io_buffer *outbuf,
				    void *dest,
				    off_t align)
{
	char *ptr = dest;
	size_t zero = 0;

	if (align) {
		zero = 8 - align;
		memset(ptr, 0, zero);
	}

	memcpy(&ptr[zero], outbuf->p, outbuf->s);

	return zero + outbuf->s;
}

void iobuf_free(size_t n_iobufs, struct fastrpc_io_buffer *iobufs)
{
	size_t i;

	for (i = 0; i < n_iobufs; i++)
		free(iobufs[i].p);

	free(iobufs);
}

struct fastrpc_decoder_context *inbuf_decode_start(uint32_t sc)
{
	struct fastrpc_decoder_context *ctx;

	ctx = malloc(sizeof(*ctx));
	if (ctx == NULL)
		return ctx;

	ctx->idx = 0;
	ctx->size = 0;
	ctx->n_inbufs = REMOTE_SCALARS_INBUFS(sc);

	ctx->size_off = 0;
	ctx->buf_off = 0;
	ctx->align = 0;

	ctx->inbufs = malloc(sizeof(*ctx->inbufs) * ctx->n_inbufs);
	if (ctx->inbufs == NULL && ctx->n_inbufs != 0)
		goto err;

	return ctx;

err:
	free(ctx);
	return NULL;
}

struct fastrpc_io_buffer *inbuf_decode_finish(struct fastrpc_decoder_context *ctx)
{
	struct fastrpc_io_buffer *inbufs = ctx->inbufs;

	free(ctx);

	return inbufs;
}

int inbuf_decode_is_complete(struct fastrpc_decoder_context *ctx)
{
	return ctx->idx >= ctx->n_inbufs;
}

int inbuf_decode(struct fastrpc_decoder_context *ctx, size_t len, const void *src)
{
	const char *buf = src;
	size_t off = 0;
	int ret = 0;

	while (off < len && ctx->idx < ctx->n_inbufs) {
		if (!ctx->size || ctx->size_off) {
			off += consume_size(ctx, len - off, &buf[off]);
			ret = try_populate_inbuf(ctx);
		} else if (ctx->align && !ctx->buf_off) {
			off += consume_alignment(ctx, len - off);
		} else {
			off += consume_buf(ctx, len - off, &buf[off]);
		}

		if (ret == -1)
			return ret;
	}

	return 0;
}

size_t outbufs_calculate_size(size_t n_outbufs, const struct fastrpc_io_buffer *outbufs)
{
	size_t i;
	size_t size = 0;

	for (i = 0; i < n_outbufs; i++) {
		size += 4;

		if (size & 0x7)
			size += 8 - (size & 0x7);
		size += outbufs[i].s;
	}

	return size;
}

void outbufs_encode(size_t n_outbufs, const struct fastrpc_io_buffer *outbufs,
		    void *dest)
{
	char *ptr = dest;
	off_t align = 0;
	size_t written;
	size_t i;

	for (i = 0; i < n_outbufs; i++) {
		*(uint32_t *) ptr = outbufs[i].s;
		ptr = &ptr[4];
		align = (align + 4) & 0x7;

		if (outbufs[i].s) {
			written = align_and_copy_outbuf(&outbufs[i], ptr, align);
			ptr = &ptr[written];
			align = (align + written) & 0x7;
		}
	}
}

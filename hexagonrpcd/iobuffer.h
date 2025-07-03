/*
 * FastRPC reverse tunnel I/O buffer encoder/decoder API
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

#ifndef IOBUFFER_H
#define IOBUFFER_H

#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>

struct fastrpc_io_buffer {
	size_t s;
	void *p;
};

struct fastrpc_decoder_context {
	struct fastrpc_io_buffer *inbufs;
	unsigned int n_inbufs;
	unsigned int idx;

	uint32_t size;
	uint32_t buf_off;

	unsigned int size_off;
	unsigned int align;
};

void iobuf_free(size_t n_iobufs, struct fastrpc_io_buffer *iobufs);

struct fastrpc_decoder_context *inbuf_decode_start(uint32_t sc);
struct fastrpc_io_buffer *inbuf_decode_finish(struct fastrpc_decoder_context *ctx);
int inbuf_decode_is_complete(struct fastrpc_decoder_context *ctx);
int inbuf_decode(struct fastrpc_decoder_context *ctx, size_t len, const void *src);

size_t outbufs_calculate_size(size_t n_outbufs, const struct fastrpc_io_buffer *outbufs);
void outbufs_encode(size_t n_outbufs, const struct fastrpc_io_buffer *outbufs,
		    void *dest);

#endif

/*
 * FastRPC reverse tunnel - tests for argument encoder/decoder
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
#include <stdlib.h>
#include <string.h>

#include "../hexagonrpcd/iobuffer.h"

static const unsigned char misaligned_iobufs[] = {
	/* inbuf 0 (4 bytes misaligned) */
	0x01, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x12,
	/* inbuf 1 (5 bytes misaligned) */
	0x0A, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00,
	0x02, 0x46, 0x8A, 0xCF, 0x13, 0x57, 0x9B, 0xDF, 0x04, 0x8C,
	/* inbuf 2 (6 bytes misaligned) */
	0x03, 0x00, 0x00, 0x00,
	0x00, 0x00,
	'A', 'B', 'C',
	/* inbuf 3 (7 bytes misaligned) */
	0x04, 0x00, 0x00, 0x00,
	0x00,
	'F', 'a', 's', 't',
	/* inbuf 4 (aligned) */
	0x05, 0x00, 0x00, 0x00,
	'S', 'l', 'o', 'w', '\0',
	/* inbuf 5 (1 byte misaligned) */
	0x06, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	'f', 'a', 's', 't', 'e', 'r',
	/* inbuf 6 (2 bytes misaligned) */
	0x07, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	'F', 'a', 's', 't', 'R', 'P', 'C',
	/* inbuf 7 (3 bytes misaligned) */
	0x02, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00,
	':', 'D',
};

static unsigned char misaligned_decoded0[] = { 0x12, };
static unsigned char misaligned_decoded1[] = {
	0x02, 0x46, 0x8A, 0xCF, 0x13, 0x57, 0x9B, 0xDF, 0x04, 0x8C,
};
static unsigned char misaligned_decoded2[] = { 'A', 'B', 'C', };
static unsigned char misaligned_decoded3[] = { 'F', 'a', 's', 't', };
static unsigned char misaligned_decoded4[] = { 'S', 'l', 'o', 'w', '\0', };
static unsigned char misaligned_decoded5[] = { 'f', 'a', 's', 't', 'e', 'r', };
static unsigned char misaligned_decoded6[] = { 'F', 'a', 's', 't', 'R', 'P', 'C', };
static unsigned char misaligned_decoded7[] = { ':', 'D', };

static const struct fastrpc_io_buffer misaligned_decoded[] = {
	{ .s =  1, .p = misaligned_decoded0, },
	{ .s = 10, .p = misaligned_decoded1, },
	{ .s =  3, .p = misaligned_decoded2, },
	{ .s =  4, .p = misaligned_decoded3, },
	{ .s =  5, .p = misaligned_decoded4, },
	{ .s =  6, .p = misaligned_decoded5, },
	{ .s =  7, .p = misaligned_decoded6, },
	{ .s =  2, .p = misaligned_decoded7, },
};

static int test_in_empty(void)
{
	struct fastrpc_decoder_context *ctx;
	struct fastrpc_io_buffer *bufs;
	int complete;

	ctx = inbuf_decode_start(REMOTE_SCALARS_MAKE(1, 0, 2));
	if (ctx == NULL)
		return 1;

	inbuf_decode(ctx, 0, NULL);

	complete = inbuf_decode_is_complete(ctx);
	if (!complete)
		return 1;

	bufs = inbuf_decode_finish(ctx);

	iobuf_free(0, bufs);

	return 0;
}

static int test_in_normal(void)
{
	struct fastrpc_decoder_context *ctx;
	struct fastrpc_io_buffer *bufs;
	int complete;
	size_t i;
	int ret;

	ctx = inbuf_decode_start(REMOTE_SCALARS_MAKE(1, 8, 2));
	if (ctx == NULL)
		return 1;

	complete = inbuf_decode_is_complete(ctx);
	if (complete)
		return 1;

	inbuf_decode(ctx, sizeof(misaligned_iobufs), misaligned_iobufs);

	complete = inbuf_decode_is_complete(ctx);
	if (!complete)
		return 1;

	bufs = inbuf_decode_finish(ctx);

	for (i = 0; i < 8; i++) {
		if (bufs[i].s != misaligned_decoded[i].s)
			return 1;

		ret = memcmp(bufs[i].p, misaligned_decoded[i].p, misaligned_decoded[i].s);
		if (ret)
			return 1;
	}

	iobuf_free(8, bufs);

	return 0;
}

static int test_in_misaligned(void)
{
	struct fastrpc_decoder_context *ctx;
	struct fastrpc_io_buffer *bufs;
	int complete;
	size_t i;
	int ret;

	ctx = inbuf_decode_start(REMOTE_SCALARS_MAKE(1, 8, 2));
	if (ctx == NULL)
		return 1;

	for (i = 0; i < sizeof(misaligned_iobufs); i++) {
		complete = inbuf_decode_is_complete(ctx);
		if (complete)
			return 1;

		inbuf_decode(ctx, 1, &misaligned_iobufs[i]);
	}

	complete = inbuf_decode_is_complete(ctx);
	if (!complete)
		return 1;

	bufs = inbuf_decode_finish(ctx);

	for (i = 0; i < 8; i++) {
		if (bufs[i].s != misaligned_decoded[i].s)
			return 1;

		ret = memcmp(bufs[i].p, misaligned_decoded[i].p, misaligned_decoded[i].s);
		if (ret)
			return 1;
	}

	iobuf_free(8, bufs);

	return 0;
}

static int test_out_empty(void)
{
	size_t size;

	size = outbufs_calculate_size(0, NULL);
	if (size != 0)
		return 1;

	outbufs_encode(0, NULL, NULL);

	return 0;
}

static int test_out_misaligned(void)
{
	size_t size;
	void *buf;
	int ret;

	size = outbufs_calculate_size(8, misaligned_decoded);
	if (size != sizeof(misaligned_iobufs))
		return 1;

	buf = malloc(size);
	if (buf == NULL)
		return 1;

	outbufs_encode(8, misaligned_decoded, buf);

	ret = memcmp(buf, misaligned_iobufs, sizeof(misaligned_iobufs));
	if (ret)
		return 1;

	free(buf);

	return 0;
}

int main(int argc, const char **argv)
{
	int ret;

	ret = test_in_empty();
	if (ret)
		return ret;

	ret = test_in_normal();
	if (ret)
		return ret;

	ret = test_in_misaligned();
	if (ret)
		return ret;

	ret = test_out_empty();
	if (ret)
		return ret;

	ret = test_out_misaligned();
	if (ret)
		return ret;

	return 0;
}

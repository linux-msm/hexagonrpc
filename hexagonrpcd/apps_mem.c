/*
 * FastRPC memory mapping interface implementation
 *
 * Copyright (C) 2024 The Sensor Shell Contributors
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <misc/fastrpc.h>
#include <sys/ioctl.h>

#include "apps_mem.h"
#include "interfaces/apps_mem.def"
#include "listener.h"

// See fastrpc.git/src/apps_mem_imp.c
#define ADSP_MMAP_HEAP_ADDR 4
#define ADSP_MMAP_REMOTE_HEAP_ADDR 8
#define ADSP_MMAP_ADD_PAGES   0x1000

struct apps_mem_ctx {
	int fd;
};

static uint32_t apps_mem_request_map64(void *data,
				       const struct fastrpc_io_buffer *inbufs,
				       struct fastrpc_io_buffer *outbufs)
{
	struct apps_mem_ctx *ctx = data;
	const struct {
		uint32_t heap_id;
		uint32_t lflags;
		uint32_t rflags;
		uint32_t padding;
		uint64_t vin;
		uint64_t len;
	} *first_in = inbufs[0].p;
	struct {
		uint64_t vapps;
		uint64_t vadsp;
	} *first_out = outbufs[0].p;
	struct fastrpc_req_mmap mmap;
	int ret;

	if (!(first_in->rflags & ADSP_MMAP_ADD_PAGES)) {
		fprintf(stderr, "Unsupported mmap operation: DMA buffer request\n");
		return AEE_EUNSUPPORTED;
	}

	mmap.fd = -1;
	mmap.flags = first_in->rflags;
	mmap.vaddrin = 0;
	mmap.size = first_in->len;

	ret = ioctl(ctx->fd, FASTRPC_IOCTL_MMAP, &mmap);
	if (ret) {
		fprintf(stderr, "Memory map failed: %s\n",
			hexagonrpc_strerror(ret));
		return ret;
	}

#if HEXAGONRPC_VERBOSE
	printf("mmap(%d, %x, %x, %lu, %lu) -> %" PRIx64 "\n",
	       first_in->heap_id,
	       first_in->lflags,
	       first_in->rflags,
	       first_in->vin,
	       first_in->len,
	       mmap.vaddrout);
#endif

	first_out->vapps = 0;
	first_out->vadsp = mmap.vaddrout & 0xFFFFFFFF;
	return 0;
}

struct fastrpc_interface *fastrpc_apps_mem_init(int fd)
{
	struct fastrpc_interface *iface;
	struct apps_mem_ctx *ctx;

	iface = malloc(sizeof(*iface));
	if (iface == NULL)
		return NULL;

	ctx = calloc(1, sizeof(*ctx));
	if (ctx == NULL)
		goto err_free_iface;

	memcpy(iface, &apps_mem_interface, sizeof(*iface));

	ctx->fd = fd;

	iface->data = ctx;

	return iface;

err_free_iface:
	free(iface);

	return NULL;
}

void fastrpc_apps_mem_deinit(struct fastrpc_interface *iface)
{
	free(iface->data);
	free(iface);
}

static const struct fastrpc_function_impl apps_mem_procs[] = {
	{ .def = NULL, .impl = NULL, },
	{ .def = NULL, .impl = NULL, },
	{ .def = &apps_mem_request_map64_def, .impl = apps_mem_request_map64, },
	{ .def = NULL, .impl = NULL, },
	{ .def = NULL, .impl = NULL, },
	{ .def = NULL, .impl = NULL, },
};

const struct fastrpc_interface apps_mem_interface = {
	.name = "apps_mem",
	.n_procs = 6,
	.procs = apps_mem_procs,
};

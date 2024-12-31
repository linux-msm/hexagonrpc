/*
 * FastRPC reverse tunnel - header file
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

#ifndef LISTENER_H
#define LISTENER_H

#include <libhexagonrpc/fastrpc.h>
#include <stddef.h>
#include <stdint.h>

#include "iobuffer.h"

struct fastrpc_function_impl {
	const struct fastrpc_function_def_interp2 *def;
	uint32_t (*impl)(void *data,
			 const struct fastrpc_io_buffer *inbufs,
			 struct fastrpc_io_buffer *outbufs);
};

struct fastrpc_interface {
	const char *name;
	void *data;
	uint8_t n_procs;
	const struct fastrpc_function_impl *procs;
};

extern const struct fastrpc_interface localctl_interface;

extern const struct fastrpc_interface apps_mem_interface;
extern const struct fastrpc_interface apps_std_interface;

int run_fastrpc_listener(int fd,
			 size_t n_ifaces,
			 struct fastrpc_interface **ifaces);

#endif

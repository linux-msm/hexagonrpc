/*
 * FastRPC argument marshalling - header files
 *
 * Copyright (C) 2025 HexagonRPC Contributors
 *
 * This file is part of HexagonRPC.
 *
 * HexagonRPC is free software: you can redistribute it and/or modify
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

#ifndef HEXAGONRPC_H
#define HEXAGONRPC_H

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

// See fastrpc.git/inc/remote.h
#define REMOTE_SCALARS_MAKEX(nAttr,nMethod,nIn,nOut,noIn,noOut) \
          ((((uint32_t)   (nAttr) &  0x7) << 29) | \
           (((uint32_t) (nMethod) & 0x1f) << 24) | \
           (((uint32_t)     (nIn) & 0xff) << 16) | \
           (((uint32_t)    (nOut) & 0xff) <<  8) | \
           (((uint32_t)    (noIn) & 0x0f) <<  4) | \
            ((uint32_t)   (noOut) & 0x0f))

#define REMOTE_SCALARS_MAKE(nMethod,nIn,nOut)  REMOTE_SCALARS_MAKEX(0,nMethod,nIn,nOut,0,0)

#define REMOTE_SCALARS_METHOD(sc) (((sc) >> 24) & 0x1f)
#define REMOTE_SCALARS_INBUFS(sc) (((sc) >> 16) & 0xff)
#define REMOTE_SCALARS_OUTBUFS(sc) (((sc) >> 8) & 0xff)

#define HRPC_ARRAY_SIZE(p) (sizeof(p) / sizeof(*(p)))

// Literal uint32_t or uint64_t (top-level use only)
#define HRPC_ARG_WORD 0
// Pointer to data of size d (usable in inner types)
#define HRPC_ARG_BLOB 1
// Pointer to inner type, with d indexing the inner_types array (top-level use only)
#define HRPC_ARG_TYPE 2
// Number of elements and pointer to sequence, element size d (usable in inner types)
#define HRPC_ARG_BLOB_SEQ 3
// Number of elements and pointer to sequence of inner type, with index d
#define HRPC_ARG_TYPE_SEQ 4
// Type 5 is reserved for sequence of inner type containing other sequences of inner types

// Corresponding argument types for output (top-level use only)
#define HRPC_ARG_OUT_BLOB 6
#define HRPC_ARG_OUT_TYPE 7
#define HRPC_ARG_OUT_BLOB_SEQ 8
#define HRPC_ARG_OUT_TYPE_SEQ 9

struct hrpc_arg_def_interp4 {
	// Type (field t) can be any HRPC_ARG_* macro
	uint32_t t;
	// Data depends on type
	uint32_t d;
};

struct hrpc_inner_type_def_interp4 {
	size_t s;
	const struct hrpc_arg_def_interp4 *p;
};

struct hrpc_method_def_interp4 {
	uint32_t msg_id;
	size_t n_args;
	const struct hrpc_arg_def_interp4 *args;
	size_t n_inner_types;
	const struct hrpc_inner_type_def_interp4 *inner_types;
};

int vhexagonrpc(const struct hrpc_method_def_interp4 *def,
		int fd, uint32_t handle, va_list list);
int hexagonrpc(const struct hrpc_method_def_interp4 *def,
	       int fd, uint32_t handle, ...);

#endif

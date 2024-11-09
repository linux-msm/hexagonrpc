/*
 * FastRPC interface method definition macros
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

#ifndef LIBHEXAGONRPC_INTERFACE_H
#define LIBHEXAGONRPC_INTERFACE_H

#include <libhexagonrpc/fastrpc.h>

/*
 * We want to declare method definitions as external by default so we only need
 * special flags when compiling the interfaces. Otherwise, everything that uses
 * the interfaces would need to define a macro.
 */
#if !HEXAGONRPC_BUILD_METHOD_DEFINITIONS

#define HEXAGONRPC_DEFINE_REMOTE_METHOD(mid, name,			\
					innums, inbufs,			\
					outnums, outbufs)		\
	extern const struct fastrpc_function_def_interp2 name##_def;

#define HEXAGONRPC_DEFINE_REMOTE_METHOD3_EMPTY(mid, name)	\
	extern const struct hrpc_method_def_interp3 name##_def;

#define HEXAGONRPC_DEFINE_REMOTE_METHOD3(mid, name, has_oprim, ...)	\
	HEXAGONRPC_DEFINE_REMOTE_METHOD3_EMPTY(mid, name)

#else /* HEXAGONRPC_BUILD_METHOD_DEFINITIONS */

#define HEXAGONRPC_DEFINE_REMOTE_METHOD(mid, name,			\
					innums, inbufs,			\
					outnums, outbufs)		\
	const struct fastrpc_function_def_interp2 name##_def = {	\
		.msg_id = mid,						\
		.in_nums = innums,					\
		.in_bufs = inbufs,					\
		.out_nums = outnums,					\
		.out_bufs = outbufs,					\
	};

#define HEXAGONRPC_DEFINE_REMOTE_METHOD3_EMPTY(mid, name)	\
	const struct hrpc_method_def_interp3 name##_def = {	\
		.msg_id = mid,					\
		.has_prim_out = false,				\
		.n_args = 0,					\
		.args = NULL,					\
	};

#define HEXAGONRPC_DEFINE_REMOTE_METHOD3(mid, name, has_oprim, ...)	\
	static const uint32_t name##_args[] = { __VA_ARGS__ };		\
	const struct hrpc_method_def_interp3 name##_def = {		\
		.msg_id = mid,						\
		.has_prim_out = has_oprim,				\
		.n_args = sizeof(name##_args) / sizeof(*name##_args),	\
		.args = name##_args,					\
	};

#endif /* HEXAGONRPC_BUILD_METHOD_DEFINITIONS */

#endif /* LIBHEXAGONRPC_INTERFACE_H */

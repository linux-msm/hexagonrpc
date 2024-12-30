/*
 * FastRPC API Replacement - context-based interface
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

#include <libhexagonrpc/fastrpc.h>

struct fastrpc_context *fastrpc_create_context(int fd, uint32_t handle)
{
	struct fastrpc_context *ctx;

	ctx = malloc(sizeof(*ctx));
	if (ctx == NULL)
		return NULL;

	ctx->fd = fd;
	ctx->handle = handle;

	return ctx;
}

int vfastrpc(const struct fastrpc_function_def_interp2 *def,
	     const struct fastrpc_context *ctx, va_list arg_list)
{
	return vfastrpc2(def, ctx->fd, ctx->handle, arg_list);
}

int fastrpc(const struct fastrpc_function_def_interp2 *def,
	    const struct fastrpc_context *ctx, ...)
{
	va_list arg_list;
	int ret;

	va_start(arg_list, ctx);
	ret = vfastrpc(def, ctx, arg_list);
	va_end(arg_list);

	return ret;
}

int vhexagonrpc(const struct hrpc_method_def_interp3 *def,
		const struct fastrpc_context *ctx, va_list arg_list)
{
	return vhexagonrpc2(def, ctx->fd, ctx->handle, arg_list);
}

int hexagonrpc(const struct hrpc_method_def_interp3 *def,
	       const struct fastrpc_context *ctx, ...)
{
	va_list arg_list;
	int ret;

	va_start(arg_list, ctx);
	ret = vhexagonrpc(def, ctx, arg_list);
	va_end(arg_list);

	return ret;
}

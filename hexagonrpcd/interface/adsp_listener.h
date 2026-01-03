/*
 * FastRPC reverse tunnel main interface - API
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

#ifndef INTERFACE_ADSP_LISTENER_H
#define INTERFACE_ADSP_LISTENER_H

#include <stdint.h>
#include <libhexagonrpc/hexagonrpc.h>

#define ADSP_LISTENER 3

extern struct hrpc_method_def_interp4 adsp_listener_init2_def;
static inline int adsp_listener_init2(int fd)
{
	return hexagonrpc(&adsp_listener_init2_def, fd, ADSP_LISTENER);
}

extern struct hrpc_method_def_interp4 adsp_listener_next2_def;
static inline int adsp_listener_next2(int fd,
				      uint32_t ret_rctx,
				      uint32_t ret_res,
				      uint32_t ret_outbufs_len, const char *ret_outbufs,
				      uint32_t *rctx,
				      uint32_t *handle,
				      uint32_t *sc,
				      uint32_t *inbufs_len,
				      uint32_t inbufs_size, char *inbufs)
{
	return hexagonrpc(&adsp_listener_next2_def, fd, ADSP_LISTENER,
			  ret_rctx, ret_res,
			  ret_outbufs_len, (const void *) ret_outbufs,
			  rctx, handle, sc, inbufs_len,
			  inbufs_size, (void *) inbufs);
}

#endif /* INTERFACE_ADSP_LISTENER_H */

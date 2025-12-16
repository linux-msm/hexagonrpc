/*
 * Remote processor control interface - API
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

#ifndef INTERFACE_REMOTECTL_H
#define INTERFACE_REMOTECTL_H

#include <stdint.h>
#include <libhexagonrpc/hexagonrpc.h>

#define REMOTECTL 0

extern struct hrpc_method_def_interp4 remotectl_open_def;
static inline int remotectl_open(int fd,
				 uint32_t n_str, const char *str,
				 uint32_t *handle,
				 uint32_t n_err, char *err,
				 uint32_t *err_valid_len)
{
	return hexagonrpc(&remotectl_open_def, fd, REMOTECTL,
			  n_str, (const void *) str,
			  (void *) handle, n_err, (void *) err, (void *) err_valid_len);
}

extern struct hrpc_method_def_interp4 remotectl_close_def;
static inline int remotectl_close(int fd,
				  uint32_t handle,
				  uint32_t n_err, char *err,
				  uint32_t *err_valid_len)
{
	return hexagonrpc(&remotectl_close_def, fd, REMOTECTL,
			  handle, n_err, (void *) err, (void *) err_valid_len);
}

#endif /* INTERFACE_REMOTECTL_H */

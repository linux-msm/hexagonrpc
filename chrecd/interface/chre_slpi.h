/*
 * Context Hub Runtime Environment interface - API
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

#ifndef INTERFACE_CHRE_SLPI_H
#define INTERFACE_CHRE_SLPI_H

#include <stdint.h>
#include <libhexagonrpc/hexagonrpc.h>

extern struct hrpc_method_def_interp4 chre_slpi_start_thread_def;
static inline int chre_slpi_start_thread(int fd, uint32_t hdl)
{
	return hexagonrpc(&chre_slpi_start_thread_def, fd, hdl);
}

extern struct hrpc_method_def_interp4 chre_slpi_wait_on_thread_exit_def;
static inline int chre_slpi_wait_on_thread_exit(int fd, uint32_t hdl)
{
	return hexagonrpc(&chre_slpi_wait_on_thread_exit_def, fd, hdl);
}


#endif /* INTERFACE_CHRE_SLPI_H */

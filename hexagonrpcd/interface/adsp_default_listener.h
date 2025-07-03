/*
 * FastRPC reverse tunnel registration interface - API
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

#ifndef INTERFACE_ADSP_DEFAULT_LISTENER_H
#define INTERFACE_ADSP_DEFAULT_LISTENER_H

#include <stdint.h>
#include <libhexagonrpc/hexagonrpc.h>

extern struct hrpc_method_def_interp4 adsp_default_listener_register_def;
static inline int adsp_default_listener_register(int fd, uint32_t hdl)
{
	return hexagonrpc(&adsp_default_listener_register_def, fd, hdl);
}

#endif /* INTERFACE_ADSP_DEFAULT_LISTENER_H */

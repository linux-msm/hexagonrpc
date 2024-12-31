/*
 * FastRPC operating system interface - context initialization
 *
 * Copyright (C) 2024 The Sensor Shell Contributors
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

#ifndef APPS_MEM_H
#define APPS_MEM_H

#include "listener.h"

struct fastrpc_interface *fastrpc_apps_mem_init();
void fastrpc_apps_mem_deinit(struct fastrpc_interface *iface);

#endif

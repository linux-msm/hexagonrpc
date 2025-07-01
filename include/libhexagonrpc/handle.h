/*
 * FastRPC handle utilities
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

#ifndef LIBHEXAGONRPC_HANDLE_H
#define LIBHEXAGONRPC_HANDLE_H

#include <stddef.h>
#include <stdint.h>

/*
 * Open a remote interface by name and get its handle.
 *
 * On success, returns 0 and sets the handle.
 * On failure, returns -1 and sets the err string.
 */
int hexagonrpc_open(int fd, const char *name, uint32_t *hdl, size_t n_err, char *err);

/*
 * Close a remote interface by its handle.
 */
void hexagonrpc_close(int fd, uint32_t hdl);

#endif /* LIBHEXAGONRPC_HANDLE_H */

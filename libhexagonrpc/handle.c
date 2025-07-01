/*
 * FastRPC remote interface handle management
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

#include <string.h>

#include <libhexagonrpc/error.h>
#include <libhexagonrpc/hexagonrpc.h>
#include <libhexagonrpc/session.h>
#include <libhexagonrpc/interfaces/remotectl.def>

int hexagonrpc_open(int fd, const char *name, uint32_t *hdl, size_t n_err, char *err)
{
	uint32_t dlerr;
	int ret;

	ret = fastrpc2(&remotectl_open_def, fd, REMOTECTL_HANDLE,
		       strlen(name) + 1, name, hdl,
		       n_err, err, &dlerr);
	if (ret) {
		strncpy(err, hexagonrpc_strerror(ret), n_err);
		err[n_err - 1] = '\0';
		return -1;
	}

	/*
	 * The error message is already in the buffer because the buffer was
	 * directly passed as an output buffer. Simply return at this point.
	 */
	if (dlerr)
		return -1;

	return 0;
}

void hexagonrpc_close(int fd, uint32_t hdl)
{
	uint32_t dlerr;

	fastrpc2(&remotectl_open_def, fd, REMOTECTL_HANDLE,
		 hdl, 0, NULL, &dlerr);
}

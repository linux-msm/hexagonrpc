/*
 * CHRE client daemon entry point
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

#include <errno.h>
#include <libhexagonrpc/fastrpc.h>
#include <libhexagonrpc/handle.h>
#include <libhexagonrpc/session.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "interfaces/chre_slpi.def"

static int chre_slpi_start_thread(int fd, uint32_t handle)
{
	return fastrpc2(&chre_slpi_start_thread_def, fd, handle);
}

static int chre_slpi_wait_on_thread_exit(int fd, uint32_t handle)
{
	return fastrpc2(&chre_slpi_wait_on_thread_exit_def, fd, handle);
}

int main()
{
	uint32_t hdl;
	char err[256];
	int fd, ret;

	fd = hexagonrpc_fd_from_env();
	if (fd == -1)
		return 1;

	ret = hexagonrpc_open(fd, "chre_slpi", &hdl, 256, err);
	if (ret) {
		fprintf(stderr, "Could not open CHRE remote interface: %s\n", err);
		return 1;
	}

	ret = chre_slpi_start_thread(fd, hdl);
	if (ret) {
		fprintf(stderr, "Could not start CHRE\n");
		goto err;
	}

	ret = chre_slpi_wait_on_thread_exit(fd, hdl);
	if (ret) {
		fprintf(stderr, "Could not wait for CHRE thread\n");
		goto err;
	}

err:
	hexagonrpc_close(fd, hdl);
}

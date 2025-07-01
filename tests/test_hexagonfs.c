/*
 * FastRPC reverse tunnel - tests for virtual filesystem
 *
 * Copyright (C) 2024 The HexagonRPC Contributors
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

#include <fcntl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../hexagonrpcd/hexagonfs.h"

static int test_mapped_seq_read(const char *path)
{
	struct hexagonfs_fd file = {
		.is_assigned = true,
		.up = NULL,
		.ops = &hexagonfs_mapped_ops,
	};
	int ret, fd;
	char *buf1, *buf2;

	buf1 = malloc(256);
	if (buf1 == NULL)
		return 1;

	buf2 = malloc(256);
	if (buf2 == NULL)
		return 1;

	fd = open(path, O_RDONLY);
	if (fd == -1)
		return 1;

	ret = hexagonfs_mapped_ops.from_dirent(path, false, &file.data);
	if (ret)
		return 1;

	memset(buf1, 0, 6);
	memset(buf2, 0, 6);

	read(fd, buf1, 1);
	read(fd, &buf1[1], 5);
	hexagonfs_mapped_ops.read(&file, 1, buf2);
	hexagonfs_mapped_ops.read(&file, 5, &buf2[1]);
	if (memcmp(buf1, buf2, 6))
		return 1;

	memset(buf1, 0, 27);
	memset(buf2, 0, 27);

	lseek(fd, 819 - 6, SEEK_CUR);
	read(fd, buf1, 27);
	hexagonfs_mapped_ops.seek(&file, 819 - 6, SEEK_CUR);
	hexagonfs_mapped_ops.read(&file, 27, buf2);
	if (memcmp(buf1, buf2, 27))
		return 1;

	hexagonfs_mapped_ops.close(file.data);

	close(fd);

	free(buf2);
	free(buf1);

	return 0;
}

int main(int argc, const char **argv)
{
	int ret;

	if (argc < 2)
		return 1;

	ret = test_mapped_seq_read(argv[1]);
	if (ret)
		return ret;

	return 0;
}

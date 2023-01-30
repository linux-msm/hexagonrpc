/*
 * HexagonFS mapped file/directory operations
 *
 * Copyright (C) 2023 Richard Acayan
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

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "hexagonfs.h"

struct mapped_ctx {
	int fd;
	DIR *dir;
};

static void mapped_close(void *fd_data)
{
	struct mapped_ctx *ctx = fd_data;

	if (ctx->dir != NULL)
		closedir(ctx->dir);
	else
		close(ctx->fd);

	free(ctx);
}

static int mapped_from_dirent(void *dirent_data, bool dir, void **fd_data)
{
	struct mapped_ctx *ctx;
	const char *name = dirent_data;
	int flags = O_RDONLY;
	int fd;
	int ret;

	ctx = malloc(sizeof(struct mapped_ctx));
	if (ctx == NULL)
		return -ENOMEM;

	if (dir)
		flags |= O_DIRECTORY;

	ctx->fd = open(name, flags);
	if (ctx->fd == -1) {
		ret = -errno;
		goto err;
	}

	ctx->dir = NULL;

	*fd_data = ctx;

	return 0;

err:
	free(ctx);
	return ret;
}

static int mapped_openat(struct hexagonfs_fd *dir,
			 const char *segment,
			 bool expect_dir,
			 struct hexagonfs_fd **out)
{
	struct mapped_ctx *dir_ctx = dir->data;
	struct hexagonfs_fd *fd;
	struct mapped_ctx *ctx;
	int flags = O_RDONLY;
	int ret;

	ctx = malloc(sizeof(struct mapped_ctx));
	if (ctx == NULL)
		return -ENOMEM;

	fd = malloc(sizeof(struct hexagonfs_fd));
	if (fd == NULL) {
		ret = -ENOMEM;
		goto err;
	}

	if (expect_dir)
		flags |= O_DIRECTORY;

	ctx->fd = openat(dir_ctx->fd, segment, flags);
	if (ctx->fd == -1) {
		ret = -errno;
		goto err_free_fd;
	}

	ctx->dir = NULL;

	fd->off = 0;
	fd->up = dir;
	fd->ops = &hexagonfs_mapped_ops;
	fd->data = ctx;

	*out = fd;

	return 0;

err_free_fd:
	free(fd);
err:
	free(ctx);
	return ret;
}

ssize_t mapped_read(struct hexagonfs_fd *fd, size_t size, void *out)
{
	struct mapped_ctx *ctx = fd->data;
	ssize_t ret;

	ret = read(ctx->fd, out, size);
	if (ret < 0)
		return -errno;

	return ret;
}

int mapped_readdir(struct hexagonfs_fd *fd, size_t size, char *out)
{
	struct mapped_ctx *ctx = fd->data;
	struct dirent *ent;

	if (ctx->dir == NULL) {
		ctx->dir = fdopendir(ctx->fd);
		if (ctx->dir == NULL)
			return -errno;

		ctx->fd = dirfd(ctx->dir);
		if (ctx->fd == -1)
			return -errno;
	}

	errno = 0;

	ent = readdir(ctx->dir);
	if (ent == NULL)
		return -errno;

	strncpy(out, ent->d_name, size);
	out[size - 1] = '\0';

	return 0;
}

static int mapped_seek(struct hexagonfs_fd *fd, off_t off, int whence)
{
	struct mapped_ctx *ctx = fd->data;
	int ret;

	ret = lseek(ctx->fd, off, whence);
	if (ret == -1)
		return -errno;

	return 0;
}

static int mapped_stat(struct hexagonfs_fd *fd, struct stat *stats)
{
	struct mapped_ctx *ctx = fd->data;
	struct stat phys;
	int ret;

	ret = fstat(ctx->fd, &phys);
	if (ret)
		return -errno;

	stats->st_size = phys.st_size;

	stats->st_dev = 0;
	stats->st_rdev = 0;

	stats->st_ino = 0;
	stats->st_nlink = 0;

	if (phys.st_mode & S_IFDIR) {
		stats->st_mode = S_IFDIR
			       | S_IRUSR | S_IXUSR
			       | S_IRGRP | S_IXGRP
			       | S_IROTH | S_IXOTH;
	} else {
		stats->st_mode = S_IRUSR | S_IRGRP | S_IROTH;
	}

	stats->st_atim.tv_sec = phys.st_atim.tv_sec;
	stats->st_atim.tv_nsec = phys.st_atim.tv_nsec;
	stats->st_ctim.tv_sec = phys.st_ctim.tv_sec;
	stats->st_ctim.tv_nsec = phys.st_ctim.tv_nsec;
	stats->st_mtim.tv_sec = phys.st_mtim.tv_sec;
	stats->st_mtim.tv_nsec = phys.st_mtim.tv_nsec;

	return 0;
}

struct hexagonfs_file_ops hexagonfs_mapped_ops = {
	.close = mapped_close,
	.from_dirent = mapped_from_dirent,
	.openat = mapped_openat,
	.read = mapped_read,
	.readdir = mapped_readdir,
	.seek = mapped_seek,
	.stat = mapped_stat,
};
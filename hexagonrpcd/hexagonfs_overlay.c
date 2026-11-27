// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * HexagonFS file operations for files served from the writable overlay.
 */
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "hexagonfs.h"
#include "overlay.h"

struct overlay_file {
	int fd;
};

static void ovl_close(void *fd_data)
{
	struct overlay_file *file = fd_data;

	close(file->fd);
	free(file);
}

static ssize_t ovl_read(struct hexagonfs_fd *fd, size_t size, void *out)
{
	struct overlay_file *file = fd->data;
	ssize_t ret;

	ret = read(file->fd, out, size);
	if (ret < 0)
		return -errno;

	return ret;
}

static ssize_t ovl_write(struct hexagonfs_fd *fd, size_t size,
			     const void *ptr)
{
	struct overlay_file *file = fd->data;
	const char *buf = ptr;
	size_t done = 0;
	ssize_t ret;

	while (done < size) {
		ret = write(file->fd, buf + done, size - done);
		if (ret < 0)
			return -errno;
		done += ret;
	}

	return done;
}

static int ovl_seek(struct hexagonfs_fd *fd, off_t off, int whence)
{
	struct overlay_file *file = fd->data;

	if (lseek(file->fd, off, whence) == (off_t) -1)
		return -errno;

	return 0;
}

static int ovl_stat(struct hexagonfs_fd *fd, struct stat *stats)
{
	struct overlay_file *file = fd->data;
	struct stat phys;
	int ret;

	ret = fstat(file->fd, &phys);
	if (ret)
		return -errno;

	stats->st_size = phys.st_size;

	stats->st_dev = 0;
	stats->st_rdev = 0;

	stats->st_ino = 0;
	stats->st_nlink = 0;

	stats->st_mode = S_IFREG | S_IRUSR | S_IWUSR
		       | S_IRGRP | S_IROTH;

	stats->st_atim.tv_sec = phys.st_atim.tv_sec;
	stats->st_atim.tv_nsec = phys.st_atim.tv_nsec;
	stats->st_ctim.tv_sec = phys.st_ctim.tv_sec;
	stats->st_ctim.tv_nsec = phys.st_ctim.tv_nsec;
	stats->st_mtim.tv_sec = phys.st_mtim.tv_sec;
	stats->st_mtim.tv_nsec = phys.st_mtim.tv_nsec;

	return 0;
}

static int ovl_fsync(struct hexagonfs_fd *fd)
{
	struct overlay_file *file = fd->data;

	if (fsync(file->fd) < 0)
		return -errno;

	return 0;
}

struct hexagonfs_file_ops hexagonfs_overlay_ops = {
	.close = ovl_close,
	.read = ovl_read,
	.write = ovl_write,
	.seek = ovl_seek,
	.stat = ovl_stat,
	.fsync = ovl_fsync,
};

int hexagonfs_overlay_open(const char *vpath, int flags,
			   struct hexagonfs_fd **out)
{
	struct hexagonfs_fd *fd;
	struct overlay_file *file;
	int ret;

	fd = calloc(1, sizeof(struct hexagonfs_fd));
	if (fd == NULL)
		return -ENOMEM;

	file = malloc(sizeof(struct overlay_file));
	if (file == NULL) {
		ret = -ENOMEM;
		goto err_free_fd;
	}

	ret = overlay_open(vpath, flags, 0644);
	if (ret < 0)
		goto err_free_file;

	file->fd = ret;

	fd->is_assigned = false;
	fd->up = NULL;
	fd->vpath = strdup(vpath);
	if (fd->vpath == NULL) {
		ret = -ENOMEM;
		goto err_close;
	}

	fd->ops = &hexagonfs_overlay_ops;
	fd->data = file;

	*out = fd;

	return 0;

err_close:
	close(file->fd);
err_free_file:
	free(file);
err_free_fd:
	free(fd);

	return ret;
}

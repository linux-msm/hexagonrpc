/*
 * Virtual read-only filesystem for Hexagon processors
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
#include <fcntl.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "hexagonfs.h"
#include "overlay.h"

static char *copy_segment_and_advance(const char *path,
				      bool *trailing_slash,
				      const char **next)
{
	const char *next_tmp;
	char *segment;
	size_t segment_len;

	next_tmp = strchr(path, '/');
	if (next_tmp == NULL) {
		next_tmp = &path[strlen(path)];
		*trailing_slash = false;
	} else {
		*trailing_slash = true;
	}

	segment_len = next_tmp - path;

	while (*next_tmp == '/')
		next_tmp++;

	segment = malloc(segment_len + 1);
	if (segment == NULL)
		return NULL;

	memcpy(segment, path, segment_len);
	segment[segment_len] = 0;

	*next = next_tmp;

	return segment;
}

static struct hexagonfs_fd *pop_dir(struct hexagonfs_fd *dir,
				    struct hexagonfs_fd *root)
{
	struct hexagonfs_fd *up;

	if (dir != root && dir->up != NULL) {
		up = dir->up;

		if (!dir->is_assigned) {
			dir->ops->close(dir->data);
			free(dir);
		}
	} else {
		up = dir;
	}

	return up;
}

static int allocate_file_number(struct hexagonfs_fd **fds,
				struct hexagonfs_fd *fd)
{
	size_t i;

	for (i = 0; i < HEXAGONFS_MAX_FD; i++) {
		if (fds[i] == NULL) {
			fd->is_assigned = true;
			fds[i] = fd;
			return i;
		}
	}

	return -EMFILE;
}

static void destroy_file_descriptor(struct hexagonfs_fd *fd)
{
	struct hexagonfs_fd *curr = fd;
	struct hexagonfs_fd *next;

	while (curr != NULL && !curr->is_assigned) {
		next = curr->up;
		curr->ops->close(curr->data);
		free(curr->vpath);
		free(curr);

		curr = next;
	}
}

int hexagonfs_set_overlay(const char *dir)
{
	return overlay_init(dir);
}

bool hexagonfs_overlay_enabled(void)
{
	return overlay_enabled();
}

int hexagonfs_open_root(struct hexagonfs_fd **fds, struct hexagonfs_dirent *root)
{
	struct hexagonfs_fd *fd;
	int ret;

	fd = calloc(1, sizeof(struct hexagonfs_fd));
	if (fd == NULL)
		return -ENOMEM;

	fd->is_assigned = false;
	fd->up = NULL;
	fd->ops = root->ops;
	fd->vpath = strdup("");
	if (fd->vpath == NULL) {
		ret = -ENOMEM;
		goto err_free_fd;
	}

	ret = root->ops->from_dirent(root->u.ptr, true, &fd->data);
	if (ret)
		goto err;

	ret = allocate_file_number(fds, fd);
	if (ret < 0)
		goto err;

	return ret;

err:
	free(fd->vpath);
err_free_fd:
	free(fd);
	return ret;
}

static int path_init(char *path, size_t size, struct hexagonfs_fd **fds,
		     int dirfd, const char *name)
{
	const char *base = "";

	if (*name != '/' && fds[dirfd] != NULL && fds[dirfd]->vpath != NULL)
		base = fds[dirfd]->vpath;

	if (snprintf(path, size, "%s", base) >= (int) size)
		return -ENAMETOOLONG;

	return 0;
}

static int path_append(char *path, size_t size, const char *segment)
{
	size_t len = strlen(path);

	if (snprintf(path + len, size - len, "/%s", segment)
	    >= (int) (size - len))
		return -ENAMETOOLONG;

	return 0;
}

static void path_pop(char *path)
{
	char *slash = strrchr(path, '/');

	if (slash != NULL)
		*slash = '\0';
}

/*
 * Files written by the DSP live in the overlay; if one of them is opened for
 * reading, serve the overlay copy instead of the read-only tree.
 */
static int open_maybe_overlaid(struct hexagonfs_fd *dir, const char *vpath,
			       const char *segment, bool expect_dir,
			       struct hexagonfs_fd **out)
{
	struct stat st;
	int ret;

	if (!expect_dir && overlay_enabled()
	    && overlay_stat(vpath, &st) == 0 && S_ISREG(st.st_mode)) {
		ret = hexagonfs_overlay_open(vpath, O_RDONLY, out);
		if (ret)
			return ret;

		(*out)->up = dir;

		return 0;
	}

	return dir->ops->openat(dir, segment, expect_dir, out);
}

static int walk_openat(struct hexagonfs_fd **fds, int rootfd, int dirfd,
		       const char *name)
{
	struct hexagonfs_fd *fd;
	char path[PATH_MAX];
	const char *curr = name;
	char *segment;
	bool expect_dir;
	int ret = 0;

	ret = path_init(path, sizeof(path), fds, dirfd, name);
	if (ret)
		return ret;

	if (*curr == '/') {
		fd = fds[rootfd];

		while (*curr == '/')
			curr++;
	} else {
		fd = fds[dirfd];
	}

	while (*curr != '\0' && !ret) {
		segment = copy_segment_and_advance(curr, &expect_dir, &curr);
		if (segment == NULL) {
			ret = -ENOMEM;
			goto err;
		}

		if (!strcmp(segment, ".")) {
			goto next;
		} else if (!strcmp(segment, "..")) {
			fd = pop_dir(fd, fds[rootfd]);
			path_pop(path);

			free(fd->vpath);
			fd->vpath = strdup(path);
			if (fd->vpath == NULL)
				ret = -ENOMEM;
		} else {
			ret = path_append(path, sizeof(path), segment);
			if (ret) {
				free(segment);
				goto err;
			}

			ret = open_maybe_overlaid(fd, path, segment, expect_dir,
						  &fd);
			if (!ret && fd->vpath == NULL) {
				fd->vpath = strdup(path);
				if (fd->vpath == NULL)
					ret = -ENOMEM;
			}
		}

	next:
		free(segment);
	}

	if (ret)
		goto err;

	ret = allocate_file_number(fds, fd);
	if (ret)
		goto err;

	return ret;

err:
	destroy_file_descriptor(fd);

	return ret;
}

int hexagonfs_openat(struct hexagonfs_fd **fds, int rootfd, int dirfd,
		     const char *name)
{
	return walk_openat(fds, rootfd, dirfd, name);
}

/*
 * Build the DSP-visible path for a name, so that the overlay can address the
 * file the same way the DSP does.
 */
static int overlay_vpath(struct hexagonfs_fd **fds, int dirfd, const char *name,
			 char *vpath, size_t size)
{
	const char *base = "";
	int ret;

	if (*name != '/' && fds[dirfd] != NULL && fds[dirfd]->vpath != NULL)
		base = fds[dirfd]->vpath;

	ret = snprintf(vpath, size, "%s/%s", base, name);
	if (ret < 0 || (size_t) ret >= size)
		return -ENAMETOOLONG;

	while (*vpath == '/')
		memmove(vpath, vpath + 1, strlen(vpath));

	return 0;
}

/*
 * Make sure the overlay holds a copy of the file, so the DSP can modify it
 * without touching the read-only tree.
 */
static int copy_up(struct hexagonfs_fd **fds, int rootfd, int dirfd,
		   const char *vpath)
{
	char buf[4096];
	ssize_t n;
	int src, dst, ret = 0;

	if (overlay_exists(vpath))
		return 0;

	src = hexagonfs_openat(fds, rootfd, dirfd, vpath);
	if (src < 0)
		return 0;	/* the DSP is creating a new file */

	dst = overlay_open(vpath, O_WRONLY | O_CREAT | O_EXCL, 0644);
	if (dst < 0) {
		hexagonfs_close(fds, src);
		return dst;
	}

	while ((n = hexagonfs_read(fds, src, sizeof(buf), buf)) > 0) {
		if (write(dst, buf, n) != n) {
			ret = -errno;
			break;
		}
	}

	close(dst);
	hexagonfs_close(fds, src);

	return ret;
}

int hexagonfs_openat_write(struct hexagonfs_fd **fds, int rootfd, int dirfd,
			   const char *name, int flags)
{
	struct hexagonfs_fd *fd;
	char vpath[PATH_MAX];
	int ret;

	if (!overlay_enabled())
		return -EROFS;

	ret = overlay_vpath(fds, dirfd, name, vpath, sizeof(vpath));
	if (ret)
		return ret;

	ret = copy_up(fds, rootfd, dirfd, vpath);
	if (ret)
		return ret;

	ret = hexagonfs_overlay_open(vpath, O_RDWR | O_CREAT | flags, &fd);
	if (ret)
		return ret;

	fd->up = fds[dirfd];

	ret = allocate_file_number(fds, fd);
	if (ret)
		destroy_file_descriptor(fd);

	return ret;
}

int hexagonfs_close(struct hexagonfs_fd **fds, int fileno)
{
	struct hexagonfs_fd *fd;

	if (fileno < 0 || fileno >= HEXAGONFS_MAX_FD)
		return -EBADF;

	fd = fds[fileno];
	if (fd == NULL || fd->ops == NULL)
		return -EBADF;

	fd->is_assigned = false;
	destroy_file_descriptor(fd);

	fds[fileno] = NULL;

	return 0;
}

int hexagonfs_lseek(struct hexagonfs_fd **fds, int fileno, off_t off, int whence)
{
	struct hexagonfs_fd *fd;

	if (fileno < 0 || fileno >= HEXAGONFS_MAX_FD)
		return -EBADF;

	fd = fds[fileno];
	if (fd == NULL)
		return -EBADF;

	if (fd->ops->seek == NULL)
		return -ENOSYS;

	return fd->ops->seek(fd, off, whence);
}

ssize_t hexagonfs_read(struct hexagonfs_fd **fds, int fileno, size_t size, void *ptr)
{
	struct hexagonfs_fd *fd;

	if (fileno < 0 || fileno >= HEXAGONFS_MAX_FD)
		return -EBADF;

	fd = fds[fileno];
	if (fd == NULL)
		return -EBADF;

	if (fd->ops->read == NULL)
		return -ENOSYS;

	return fd->ops->read(fd, size, ptr);
}

int hexagonfs_readdir(struct hexagonfs_fd **fds, int fileno, size_t ent_size, char *ent)
{
	struct hexagonfs_fd *fd;

	if (fileno < 0 || fileno >= HEXAGONFS_MAX_FD)
		return -EBADF;

	fd = fds[fileno];
	if (fd == NULL)
		return -EBADF;

	if (fd->ops->readdir == NULL)
		return -ENOSYS;

	return fd->ops->readdir(fd, ent_size, ent);
}

int hexagonfs_fstat(struct hexagonfs_fd **fds, int fileno, struct stat *stats)
{
	struct hexagonfs_fd *fd;

	if (fileno < 0 || fileno >= HEXAGONFS_MAX_FD)
		return -EBADF;

	fd = fds[fileno];
	if (fd == NULL)
		return -EBADF;

	if (fd->ops->stat == NULL)
		return -ENOSYS;

	return fd->ops->stat(fd, stats);
}

ssize_t hexagonfs_write(struct hexagonfs_fd **fds, int fileno, size_t size,
			const void *ptr)
{
	struct hexagonfs_fd *fd;

	if (fileno < 0 || fileno >= HEXAGONFS_MAX_FD)
		return -EBADF;

	fd = fds[fileno];
	if (fd == NULL)
		return -EBADF;

	if (fd->ops->write == NULL)
		return -ENOSYS;

	return fd->ops->write(fd, size, ptr);
}

int hexagonfs_fsync(struct hexagonfs_fd **fds, int fileno)
{
	struct hexagonfs_fd *fd;

	if (fileno < 0 || fileno >= HEXAGONFS_MAX_FD)
		return -EBADF;

	fd = fds[fileno];
	if (fd == NULL)
		return -EBADF;

	if (fd->ops->fsync == NULL)
		return 0;

	return fd->ops->fsync(fd);
}

int hexagonfs_remove(struct hexagonfs_fd **fds, int rootfd, int dirfd,
		     const char *name)
{
	int ret, fd;

	if (!overlay_enabled())
		return -EROFS;

	ret = overlay_remove(name);
	if (ret != -ENOENT)
		return ret;

	/* The file exists in the read-only tree only: refuse to delete it. */
	fd = hexagonfs_openat(fds, rootfd, dirfd, name);
	if (fd >= 0) {
		hexagonfs_close(fds, fd);
		return -EPERM;
	}

	return -ENOENT;
}

int hexagonfs_rename(struct hexagonfs_fd **fds, int rootfd, int dirfd,
		     const char *from, const char *to)
{
	(void) fds;
	(void) rootfd;
	(void) dirfd;

	if (!overlay_enabled())
		return -EROFS;

	return overlay_rename(from, to);
}

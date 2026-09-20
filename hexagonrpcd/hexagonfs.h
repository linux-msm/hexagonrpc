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

#ifndef HEXAGONFS_H
#define HEXAGONFS_H

#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>
#include <sys/stat.h>

#define HEXAGONFS_MAX_FD 256

struct hexagonfs_fd;

struct hexagonfs_file_ops {
	void (*close)(void *fd_data);
	int (*from_dirent)(const void *dirent_data, bool dir, void **fd_data);
	int (*openat)(struct hexagonfs_fd *dir,
		      const char *segment,
		      bool expect_dir,
		      struct hexagonfs_fd **out);
	int (*readdir)(struct hexagonfs_fd *fd, size_t size, char *out);
	ssize_t (*read)(struct hexagonfs_fd *fd, size_t size, void *ptr);
	int (*stat)(struct hexagonfs_fd *fd, struct stat *stats);
	int (*seek)(struct hexagonfs_fd *fd, off_t off, int whence);
	/* Writable backends only; NULL here is reported as ENOSYS. */
	int (*create)(struct hexagonfs_fd *dir,
		      const char *name,
		      int flags,
		      struct hexagonfs_fd **out);
	ssize_t (*write)(struct hexagonfs_fd *fd, size_t size, const void *ptr);
	int (*truncate)(struct hexagonfs_fd *fd, off_t len);
	int (*unlink)(struct hexagonfs_fd *dir, const char *name);
	int (*rename)(struct hexagonfs_fd *dir, const char *name,
		      struct hexagonfs_fd *newdir, const char *newname);
};

struct hexagonfs_dirent {
	const char *name;

	struct hexagonfs_file_ops *ops;
	union hexagonfs_dirent_data {
		void *ptr;
		struct hexagonfs_dirent **dir;
		const char *phys;
	} u;
};

struct hexagonfs_fd {
	bool is_assigned;
	struct hexagonfs_fd *up;
	void *data;

	struct hexagonfs_file_ops *ops;
};

extern struct hexagonfs_file_ops hexagonfs_mapped_ops;
extern struct hexagonfs_file_ops hexagonfs_mapped_or_empty_ops;
extern struct hexagonfs_file_ops hexagonfs_mapped_sysfs_ops;
extern struct hexagonfs_file_ops hexagonfs_plat_subtype_name_ops;
extern struct hexagonfs_file_ops hexagonfs_virt_dir_ops;

int hexagonfs_open_root(struct hexagonfs_fd **fds, struct hexagonfs_dirent *root);
int hexagonfs_openat(struct hexagonfs_fd **fds, int rootfd, int dirfd, const char *name);
int hexagonfs_close(struct hexagonfs_fd **fds, int fileno);

int hexagonfs_fstat(struct hexagonfs_fd **fds, int fileno, struct stat *stats);
int hexagonfs_lseek(struct hexagonfs_fd **fds, int fileno, off_t pos, int whence);
int hexagonfs_readdir(struct hexagonfs_fd **fds, int fileno, size_t size, char *name);
ssize_t hexagonfs_read(struct hexagonfs_fd **fds, int fileno, size_t size, void *ptr);
int hexagonfs_create(struct hexagonfs_fd **fds, int rootfd, int dirfd, const char *name, int flags);
ssize_t hexagonfs_write(struct hexagonfs_fd **fds, int fileno, size_t size, const void *ptr);
int hexagonfs_ftruncate(struct hexagonfs_fd **fds, int fileno, off_t len);
int hexagonfs_unlink(struct hexagonfs_fd **fds, int rootfd, int dirfd, const char *name);
int hexagonfs_rename(struct hexagonfs_fd **fds, int rootfd, int dirfd, const char *name, const char *newname);

#endif

/*
 * FastRPC operating system interface implementation
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
#include <libhexagonrpc/error.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "hexagonfs.h"
#include "interface/apps_std.h"
#include "iobuffer.h"
#include "listener.h"

struct apps_std_ctx {
	int rootfd;
	int adsp_avs_cfg_dirfd;
	int adsp_library_dirfd;
	struct hexagonfs_fd *fds[HEXAGONFS_MAX_FD];
};

static const int apps_std_whence_table[] = {
	SEEK_SET,
	SEEK_CUR,
	SEEK_END,
};

/*
 * This is a placeholder function used to complete any I/O operations.
 * File descriptors do not have a flush operation because their reads and
 * writes are blocking.
 */
static uint32_t apps_std_fflush(void *data,
				const struct fastrpc_io_buffer *inbufs,
				struct fastrpc_io_buffer *outbufs)
{
#ifdef HEXAGONRPC_VERBOSE
	uint32_t *fd = inbufs[0].p;

	printf("ignore fflush(%u)\n", *fd);
#endif

	memset(outbufs[0].p, 0, outbufs[0].s);

	return 0;
}

static uint32_t apps_std_fclose(void *data,
				const struct fastrpc_io_buffer *inbufs,
				struct fastrpc_io_buffer *outbufs)
{
	struct apps_std_ctx *ctx = data;
	const uint32_t *first_in = inbufs[0].p;
	int ret;

	ret = hexagonfs_close(ctx->fds, *first_in);
	if (ret) {
		fprintf(stderr, "Could not close: %s\n", strerror(-ret));
		return AEE_EFAILED;
	}

#ifdef HEXAGONRPC_VERBOSE
	printf("close(%u)\n", *first_in);
#endif

	return 0;
}

static uint32_t apps_std_fread(void *data,
			       const struct fastrpc_io_buffer *inbufs,
			       struct fastrpc_io_buffer *outbufs)
{
	struct apps_std_ctx *ctx = data;
	const struct {
		uint32_t fd;
		uint32_t buf_size;
	} *first_in = inbufs[0].p;
	struct {
		uint32_t written;
		uint32_t is_eof;
	} *first_out = outbufs[0].p;
	ssize_t ret;

	ret = hexagonfs_read(ctx->fds, first_in->fd,
			     first_in->buf_size, outbufs[1].p);
	if (ret < 0) {
		fprintf(stderr, "Could not read file: %s\n", strerror(-ret));
		return AEE_EFAILED;
	}

#ifdef HEXAGONRPC_VERBOSE
	printf("read(%u, %u) -> %ld\n", first_in->fd,
					first_in->buf_size,
					ret);
#endif

	first_out->written = ret;
	first_out->is_eof = first_out->written < first_in->buf_size;

	return 0;
}

static uint32_t apps_std_fwrite(void *data,
				const struct fastrpc_io_buffer *inbufs,
				struct fastrpc_io_buffer *outbufs)
{
	struct apps_std_ctx *ctx = data;
	const struct {
		uint32_t fd;
		uint32_t buf_size;
	} *first_in = inbufs[0].p;
	struct {
		uint32_t written;
		uint32_t is_eof;
	} *first_out = outbufs[0].p;
	ssize_t ret;

	ret = hexagonfs_write(ctx->fds, first_in->fd, first_in->buf_size,
			      inbufs[1].p);
	if (ret < 0) {
		/*
		 * Without a writable overlay there is nothing to write to; the
		 * firmware is happy as long as the write is accepted.
		 */
		if (ret == -ENOSYS) {
			first_out->written = first_in->buf_size;
			first_out->is_eof = 0;

			return 0;
		}

		fprintf(stderr, "Could not write file: %s\n", strerror(-ret));
		return AEE_EFAILED;
	}

#ifdef HEXAGONRPC_VERBOSE
	printf("write(%u, %u) -> %ld\n", first_in->fd, first_in->buf_size, ret);
#endif

	first_out->written = ret;
	first_out->is_eof = 0;

	return 0;
}

static uint32_t apps_std_fopen_fd(void *data,
				  const struct fastrpc_io_buffer *inbufs,
				  struct fastrpc_io_buffer *outbufs)
{
	struct apps_std_ctx *ctx = data;
	uint32_t *out = outbufs[0].p;
	int fd;

	fd = hexagonfs_openat(ctx->fds, ctx->rootfd, ctx->rootfd, inbufs[0].p);
	if (fd < 0) {
		fprintf(stderr, "Could not open %s: %s\n",
				(const char *) inbufs[0].p, strerror(errno));
		return AEE_EFAILED;
	}

	out[0] = fd;
	out[1] = 0;

	return 0;
}

static uint32_t apps_std_fclose_fd(void *data,
				   const struct fastrpc_io_buffer *inbufs,
				   struct fastrpc_io_buffer *outbufs)
{
	struct apps_std_ctx *ctx = data;
	const uint32_t *fd = inbufs[0].p;

	return hexagonfs_close(ctx->fds, *fd) ? AEE_EFAILED : 0;
}

static uint32_t apps_std_fopen_with_env_fd(void *data,
					   const struct fastrpc_io_buffer *inbufs,
					   struct fastrpc_io_buffer *outbufs)
{
	struct apps_std_ctx *ctx = data;
	uint32_t *out = outbufs[0].p;
	char rw_mode;
	int dirfd, fd;

	rw_mode = ((const char *) inbufs[3].p)[0];
	if (rw_mode == 'w' || rw_mode == 'a')
		fprintf(stderr, "Opening %s for writing (writes are discarded)\n",
				(const char *) inbufs[2].p);

	if (!strcmp(inbufs[0].p, "ADSP_LIBRARY_PATH")) {
		dirfd = ctx->adsp_library_dirfd;
	} else if (!strcmp(inbufs[0].p, "ADSP_AVS_CFG_PATH")) {
		dirfd = ctx->adsp_avs_cfg_dirfd;
	} else {
		fprintf(stderr, "Unknown search directory %s\n",
				(const char *) inbufs[0].p);
		return AEE_EBADPARM;
	}

	if (dirfd < 0)
		return AEE_EFAILED;

	fd = hexagonfs_openat(ctx->fds, ctx->rootfd, dirfd, inbufs[2].p);
	if (fd < 0) {
		fprintf(stderr, "Could not open %s: %s\n",
				(const char *) inbufs[2].p, strerror(errno));
		return AEE_EFAILED;
	}

	out[0] = fd;
	out[1] = 0;

	return 0;
}

static uint32_t apps_std_fseek(void *data,
			       const struct fastrpc_io_buffer *inbufs,
			       struct fastrpc_io_buffer *outbufs)
{
	struct apps_std_ctx *ctx = data;
	const struct {
		uint32_t fd;
		uint32_t pos;
		uint32_t whence;
	} *first_in = inbufs[0].p;
	int ret;
	int whence;

	whence = apps_std_whence_table[first_in->whence];

	ret = hexagonfs_lseek(ctx->fds, first_in->fd, first_in->pos, whence);
	if (ret) {
		fprintf(stderr, "Could not seek stream: %s\n", strerror(-ret));
		return AEE_EFAILED;
	}

#ifdef HEXAGONRPC_VERBOSE
	printf("lseek(%u, %d, %d)\n", first_in->fd,
				      first_in->pos,
				      first_in->whence);
#endif

	return 0;
}

static uint32_t apps_std_fopen_with_env(void *data,
					const struct fastrpc_io_buffer *inbufs,
					struct fastrpc_io_buffer *outbufs)
{
	struct apps_std_ctx *ctx = data;
	uint32_t *out = outbufs[0].p;
	char rw_mode;
	int dirfd, fd;

	// The name and environment variable must also be NULL-terminated
	if (((const char *) inbufs[1].p)[inbufs[1].s - 1] != 0
	 || ((const char *) inbufs[3].p)[inbufs[3].s - 1] != 0
	 || ((const char *) inbufs[4].p)[inbufs[4].s - 1] != 0)
		return AEE_EBADPARM;

	rw_mode = ((const char *) inbufs[4].p)[0];
	if (rw_mode == 'w' || rw_mode == 'a') {
		if (!hexagonfs_overlay_enabled())
			return AEE_EUNSUPPORTED;

		/*
		 * 'w' does not truncate the copied-up file: this firmware
		 * reopens the same file for writing several times while it
		 * builds its registry, so O_TRUNC loses everything written
		 * before (measured: the registry shrank and the accelerometer
		 * and proximity sensors disappeared from the sensor list).
		 */
		fd = hexagonfs_openat_write(ctx->fds, ctx->rootfd, ctx->rootfd,
					    inbufs[3].p,
					    rw_mode == 'a' ? O_APPEND : 0);
		if (fd < 0) {
			fprintf(stderr, "Could not open %s for writing: %s\n",
					(const char *) inbufs[3].p,
					strerror(-fd));
			return AEE_EFAILED;
		}

		*out = (uint32_t) fd;
		return 0;
	}

	if (!strcmp(inbufs[1].p, "ADSP_LIBRARY_PATH")) {
		dirfd = ctx->adsp_library_dirfd;
	} else if (!strcmp(inbufs[1].p, "ADSP_AVS_CFG_PATH")) {
		dirfd = ctx->adsp_avs_cfg_dirfd;
	} else {
		fprintf(stderr, "Unknown search directory %s\n",
				(const char *) inbufs[1].p);
		return AEE_EBADPARM;
	}

	if (dirfd < 0) {
		fprintf(stderr, "Could not open virtual %s: %s\n",
				(const char *) inbufs[1].p, strerror(-dirfd));
		return AEE_EFAILED;
	}

	fd = hexagonfs_openat(ctx->fds, ctx->rootfd, dirfd, inbufs[3].p);
	if (fd < 0) {
		fprintf(stderr, "Could not open %s: %s\n",
				(const char *) inbufs[3].p,
				strerror(errno));
		return AEE_EFAILED;
	}

#ifdef HEXAGONRPC_VERBOSE
	printf("openat($%s, %s, %c) -> %d\n", (const char *) inbufs[1].p,
					      (const char *) inbufs[3].p,
					      rw_mode,
					      fd);
#endif

	*out = fd;

	return 0;
}

static uint32_t apps_std_opendir(void *data,
				 const struct fastrpc_io_buffer *inbufs,
				 struct fastrpc_io_buffer *outbufs)
{
	struct apps_std_ctx *ctx = data;
	uint64_t *dir_out = outbufs[0].p;
	int ret;

	// The name must be NULL-terminated
	if (((const char *) inbufs[1].p)[inbufs[1].s - 1] != 0)
		return AEE_EBADPARM;

	ret = hexagonfs_openat(ctx->fds, ctx->rootfd, ctx->rootfd, inbufs[1].p);
	if (ret < 0) {
		fprintf(stderr, "Could not open %s: %s\n",
				(const char *) inbufs[1].p,
				strerror(-ret));
		return AEE_EFAILED;
	}

#ifdef HEXAGONRPC_VERBOSE
	printf("opendir(%s) -> %d\n", (const char *) inbufs[1].p, ret);
#endif

	*dir_out = ret;

	return 0;
}

static uint32_t apps_std_closedir(void *data,
				  const struct fastrpc_io_buffer *inbufs,
				  struct fastrpc_io_buffer *outbufs)
{
	struct apps_std_ctx *ctx = data;
	const uint64_t *dir = inbufs[0].p;
	int ret;

	ret = hexagonfs_close(ctx->fds, *dir);
	if (ret)
		return AEE_EFAILED;

#ifdef HEXAGONRPC_VERBOSE
	printf("closedir(%ld)\n", *dir);
#endif

	return 0;
}

static uint32_t apps_std_readdir(void *data,
				 const struct fastrpc_io_buffer *inbufs,
				 struct fastrpc_io_buffer *outbufs)
{
	struct apps_std_ctx *ctx = data;
	const uint64_t *dir = inbufs[0].p;
	struct {
		uint32_t inode;
		char name[255];
		uint32_t is_eof;
	} *first_out = outbufs[0].p;
	int ret;

	ret = hexagonfs_readdir(ctx->fds, *dir, 255, first_out->name);
	if (ret < 0) {
		fprintf(stderr, "Could not read from directory: %s\n",
				strerror(-ret));
		return AEE_EFAILED;
	}

#ifdef HEXAGONRPC_VERBOSE
	printf("readdir(%ld) -> %s\n", *dir, first_out->name);
#endif

	first_out->inode = 0;
	first_out->is_eof = (*first_out->name == '\0');

	return 0;
}

static uint32_t apps_std_stat(void *data,
			      const struct fastrpc_io_buffer *inbufs,
			      struct fastrpc_io_buffer *outbufs)
{
	struct apps_std_ctx *ctx = data;
	struct {
		uint64_t tsz; // Unknown purpose
		uint64_t dev;
		uint64_t ino;
		uint32_t mode;
		uint32_t nlink;
		uint64_t rdev;
		uint64_t size;
		int64_t atime;
		int64_t atimensec;
		int64_t mtime;
		int64_t mtimensec;
		int64_t ctime;
		int64_t ctimensec;
	} *first_out = outbufs[0].p;
	const char *pathname = inbufs[1].p;
	struct stat stats;
	int fd, ret;

	if (((const char *) inbufs[1].p)[inbufs[1].s - 1] != 0)
		return AEE_EBADPARM;

	fd = hexagonfs_openat(ctx->fds, ctx->rootfd, ctx->adsp_library_dirfd, pathname);
	if (fd < 0) {
		fprintf(stderr, "Could not open %s: %s\n",
				pathname, strerror(-fd));
		return AEE_EFAILED;
	}

	ret = hexagonfs_fstat(ctx->fds, fd, &stats);
	if (ret) {
		fprintf(stderr, "Could not stat %s: %s\n",
				pathname, strerror(-fd));
		return AEE_EFAILED;
	}

	hexagonfs_close(ctx->fds, fd);

#ifdef HEXAGONRPC_VERBOSE
	printf("stat(%s)\n", pathname);
#endif

	first_out->tsz = 0;

	first_out->dev = stats.st_dev;
	first_out->ino = stats.st_ino;
	first_out->mode = stats.st_mode;
	first_out->nlink = stats.st_nlink;
	first_out->rdev = stats.st_rdev;
	first_out->size = stats.st_size;
	first_out->atime = (int64_t) stats.st_atim.tv_sec;
	first_out->atimensec = (int64_t) stats.st_atim.tv_nsec;
	first_out->mtime = (int64_t) stats.st_mtim.tv_sec;
	first_out->mtimensec = (int64_t) stats.st_mtim.tv_nsec;
	first_out->ctime = (int64_t) stats.st_ctim.tv_nsec;
	first_out->ctimensec = (int64_t) stats.st_ctim.tv_nsec;

	return 0;
}

struct fastrpc_interface *fastrpc_apps_std_init(struct hexagonfs_dirent *root)
{
	struct fastrpc_interface *iface;
	struct apps_std_ctx *ctx;

	iface = malloc(sizeof(struct fastrpc_interface));
	if (iface == NULL)
		return NULL;

	ctx = calloc(1, sizeof(struct apps_std_ctx));
	if (ctx == NULL)
		goto err_free_iface;

	memcpy(iface, &apps_std_interface, sizeof(struct fastrpc_interface));

	ctx->rootfd = hexagonfs_open_root(ctx->fds, root);
	if (ctx->rootfd < 0)
		goto err_free_ctx;

	ctx->adsp_avs_cfg_dirfd = hexagonfs_openat(ctx->fds,
						   ctx->rootfd,
						   ctx->rootfd,
						   "/vendor/etc/acdbdata/");
	ctx->adsp_library_dirfd = hexagonfs_openat(ctx->fds,
						   ctx->rootfd,
						   ctx->rootfd,
						   "/usr/lib/qcom/adsp/");

	iface->data = ctx;

	return iface;

err_free_ctx:
	free(ctx);
err_free_iface:
	free(iface);

	return NULL;
}

void fastrpc_apps_std_deinit(struct fastrpc_interface *iface)
{
	struct apps_std_ctx *ctx = iface->data;
	int i;

	for (i = 0; i < HEXAGONFS_MAX_FD; i++) {
		if (ctx->fds[i] != NULL)
			hexagonfs_close(ctx->fds, i);
	}

	free(iface->data);
	free(iface);
}

static uint32_t apps_std_fsync(void *data,
			       const struct fastrpc_io_buffer *inbufs,
			       struct fastrpc_io_buffer *outbufs)
{
	struct apps_std_ctx *ctx = data;
	const uint32_t *fd = inbufs[0].p;

	return hexagonfs_fsync(ctx->fds, *fd) ? AEE_EFAILED : 0;
}

static uint32_t apps_std_fremove(void *data,
				 const struct fastrpc_io_buffer *inbufs,
				 struct fastrpc_io_buffer *outbufs)
{
	struct apps_std_ctx *ctx = data;
	const char *path = inbufs[0].p;

	if (hexagonfs_remove(ctx->fds, ctx->rootfd, ctx->rootfd, path))
		return AEE_EFAILED;

	return 0;
}

static uint32_t apps_std_frename(void *data,
				 const struct fastrpc_io_buffer *inbufs,
				 struct fastrpc_io_buffer *outbufs)
{
	struct apps_std_ctx *ctx = data;
	const char *from = inbufs[0].p;
	const char *to = inbufs[1].p;

	return hexagonfs_rename(ctx->fds, ctx->rootfd, ctx->rootfd, from, to)
		? AEE_EFAILED : 0;
}

static const struct fastrpc_function_impl apps_std_procs[] = {
	[2] = { .def = &apps_std_fflush_def, .impl = apps_std_fflush },
	[3] = { .def = &apps_std_fclose_def, .impl = apps_std_fclose },
	[4] = { .def = &apps_std_fread_def, .impl = apps_std_fread },
	[5] = { .def = &apps_std_fwrite_def, .impl = apps_std_fwrite },
	[9] = { .def = &apps_std_fseek_def, .impl = apps_std_fseek },
	[19] = { .def = &apps_std_fopen_with_env_def, .impl = apps_std_fopen_with_env },
	[23] = { .def = &apps_std_fsync_def, .impl = apps_std_fsync },
	[24] = { .def = &apps_std_fremove_def, .impl = apps_std_fremove },
	[26] = { .def = &apps_std_opendir_def, .impl = apps_std_opendir },
	[27] = { .def = &apps_std_closedir_def, .impl = apps_std_closedir },
	[28] = { .def = &apps_std_readdir_def, .impl = apps_std_readdir },
	[31] = { .def = &apps_std_stat_def, .impl = apps_std_stat },
	[33] = { .def = &apps_std_frename_def, .impl = apps_std_frename },
	[34] = { .def = &apps_std_fopen_fd_def, .impl = apps_std_fopen_fd },
	[35] = { .def = &apps_std_fclose_fd_def, .impl = apps_std_fclose_fd },
	[36] = { .def = &apps_std_fopen_with_env_fd_def, .impl = apps_std_fopen_with_env_fd },
};


const struct fastrpc_interface apps_std_interface = {
	.name = "apps_std",
	.n_procs = 37, /* = table length; indexes follow the AOSP apps_std IDL */
	.procs = apps_std_procs,
};

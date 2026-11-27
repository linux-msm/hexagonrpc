// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Writable overlay for hexagonrpcd.
 *
 * Paths are addressed exactly like the DSP addresses them: relative to the
 * root of the served tree, e.g. /persist/sensors/registry/sns_reg_version.
 * The same relative path is used below the overlay directory.
 */
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "overlay.h"

static char *overlay_root;

static const char *relative(const char *vpath)
{
	while (*vpath == '/')
		vpath++;

	return vpath;
}

int overlay_init(const char *dir)
{
	if (dir == NULL || dir[0] == '\0')
		return -EINVAL;

	overlay_root = realpath(dir, NULL);
	if (overlay_root == NULL) {
		if (mkdir(dir, 0755) < 0 && errno != EEXIST)
			return -errno;
		overlay_root = realpath(dir, NULL);
		if (overlay_root == NULL)
			return -errno;
	}

	return 0;
}

bool overlay_enabled(void)
{
	return overlay_root != NULL;
}

int overlay_phys_path(const char *vpath, char *buf, size_t size)
{
	int ret;

	if (!overlay_enabled())
		return -ENOSYS;

	ret = snprintf(buf, size, "%s/%s", overlay_root, relative(vpath));
	if (ret < 0 || (size_t) ret >= size)
		return -ENAMETOOLONG;

	return 0;
}

/* Create all parent directories of an overlay path, like mkdir -p. */
static int mkdir_parents(const char *vpath)
{
	char path[PATH_MAX];
	char *slash;
	int ret;

	ret = overlay_phys_path(vpath, path, sizeof(path));
	if (ret)
		return ret;

	for (slash = path; (slash = strchr(slash + 1, '/')); slash++) {
		*slash = '\0';
		if (mkdir(path, 0755) < 0 && errno != EEXIST) {
			*slash = '/';
			return -errno;
		}
		*slash = '/';
	}

	return 0;
}

int overlay_open(const char *vpath, int flags, mode_t mode)
{
	char path[PATH_MAX];
	int ret;

	if (flags & (O_CREAT | O_WRONLY | O_RDWR)) {
		ret = mkdir_parents(vpath);
		if (ret)
			return ret;
	}

	ret = overlay_phys_path(vpath, path, sizeof(path));
	if (ret)
		return ret;

	ret = open(path, flags, mode);
	if (ret < 0)
		return -errno;

	return ret;
}

bool overlay_exists(const char *vpath)
{
	char path[PATH_MAX];

	if (overlay_phys_path(vpath, path, sizeof(path)))
		return false;

	return access(path, F_OK) == 0;
}

int overlay_stat(const char *vpath, struct stat *st)
{
	char path[PATH_MAX];
	int ret;

	ret = overlay_phys_path(vpath, path, sizeof(path));
	if (ret)
		return ret;

	if (stat(path, st) < 0)
		return -errno;

	return 0;
}

int overlay_remove(const char *vpath)
{
	char path[PATH_MAX];
	int ret;

	ret = overlay_phys_path(vpath, path, sizeof(path));
	if (ret)
		return ret;

	if (unlink(path) < 0)
		return -errno;

	return 0;
}

int overlay_rename(const char *from, const char *to)
{
	char from_path[PATH_MAX];
	char to_path[PATH_MAX];
	int ret;

	ret = overlay_phys_path(from, from_path, sizeof(from_path));
	if (ret)
		return ret;

	ret = mkdir_parents(to);
	if (ret)
		return ret;

	ret = overlay_phys_path(to, to_path, sizeof(to_path));
	if (ret)
		return ret;

	if (rename(from_path, to_path) < 0)
		return -errno;

	return 0;
}

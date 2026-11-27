// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Tests for the writable overlay behind the HexagonFS.
 */
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "../hexagonrpcd/hexagonfs.h"

#define REGISTRY_PATH "/persist/sensors/registry/sns_reg_version"

static int write_file(const char *path, const char *content)
{
	int fd;
	ssize_t len = strlen(content);

	fd = open(path, O_WRONLY | O_CREAT, 0644);
	if (fd < 0)
		return -errno;

	if (write(fd, content, len) != len) {
		close(fd);
		return -EIO;
	}

	close(fd);

	return 0;
}

static int read_via(const char *root, const char *name, char *buf, size_t size)
{
	struct hexagonfs_fd *fds[HEXAGONFS_MAX_FD] = { 0 };
	struct hexagonfs_dirent dir = {
		.name = "root",
		.ops = &hexagonfs_mapped_ops,
		.u.phys = root,
	};
	ssize_t n;
	int fd, ret;

	ret = hexagonfs_open_root(fds, &dir);
	if (ret < 0)
		return ret;

	fd = hexagonfs_openat(fds, 0, 0, name);
	if (fd < 0) {
		hexagonfs_close(fds, 0);
		return fd;
	}

	n = hexagonfs_read(fds, fd, size - 1, buf);
	if (n < 0) {
		hexagonfs_close(fds, fd);
		hexagonfs_close(fds, 0);
		return n;
	}

	buf[n] = '\0';

	hexagonfs_close(fds, fd);
	hexagonfs_close(fds, 0);

	return 0;
}

static int test_copy_up_and_overlay_first(const char *root, const char *overlay)
{
	struct hexagonfs_fd *fds[HEXAGONFS_MAX_FD] = { 0 };
	struct hexagonfs_dirent dir = {
		.name = "root",
		.ops = &hexagonfs_mapped_ops,
		.u.phys = root,
	};
	char buf[64];
	int fd, ret;

	ret = hexagonfs_set_overlay(overlay);
	if (ret)
		return 1;

	ret = hexagonfs_open_root(fds, &dir);
	if (ret < 0)
		return 1;

	/* Opening for writing copies the read-only file into the overlay. */
	fd = hexagonfs_openat_write(fds, 0, 0, REGISTRY_PATH, 0);
	if (fd < 0)
		return 1;

	ret = hexagonfs_write(fds, fd, 9, "version=3");
	if (ret != 9)
		return 1;

	ret = hexagonfs_fsync(fds, fd);
	if (ret)
		return 1;

	hexagonfs_close(fds, fd);
	hexagonfs_close(fds, 0);

	/* Reads must see the overlay copy, not the read-only original. */
	ret = read_via(root, REGISTRY_PATH, buf, sizeof(buf));
	if (ret)
		return 1;

	if (strcmp(buf, "version=3"))
		return 1;

	return 0;
}

static int test_rename_and_remove(const char *root, const char *overlay)
{
	struct hexagonfs_fd *fds[HEXAGONFS_MAX_FD] = { 0 };
	struct hexagonfs_dirent dir = {
		.name = "root",
		.ops = &hexagonfs_mapped_ops,
		.u.phys = root,
	};
	char buf[64];
	int ret;

	ret = hexagonfs_set_overlay(overlay);
	if (ret)
		return 1;

	ret = hexagonfs_open_root(fds, &dir);
	if (ret < 0)
		return 1;

	/* Renaming a file that lives in the read-only tree is not allowed. */
	ret = hexagonfs_remove(fds, 0, 0, "/persist/sensors/sns_reg.conf");
	if (ret != -EPERM)
		return 1;

	ret = hexagonfs_rename(fds, 0, 0, "/persist/sensors/sns_reg.conf",
			       "/persist/sensors/renamed.conf");
	if (ret != -ENOENT)
		return 1;

	hexagonfs_close(fds, 0);

	/* A file in the overlay can be renamed and removed. */
	ret = read_via(root, REGISTRY_PATH, buf, sizeof(buf));
	if (ret)
		return 1;

	ret = hexagonfs_open_root(fds, &dir);
	if (ret < 0)
		return 1;

	ret = hexagonfs_rename(fds, 0, 0, REGISTRY_PATH,
			       "/persist/sensors/registry/moved");
	if (ret)
		return 1;

	/* The read-only original is still there, so it cannot be removed. */
	ret = hexagonfs_remove(fds, 0, 0, REGISTRY_PATH);
	if (ret != -EPERM)
		return 1;

	ret = hexagonfs_remove(fds, 0, 0, "/persist/sensors/registry/moved");
	if (ret)
		return 1;

	hexagonfs_close(fds, 0);

	return 0;
}

static void remove_tree(const char *path)
{
	char cmd[PATH_MAX + 16];

	snprintf(cmd, sizeof(cmd), "rm -rf '%s'", path);
	if (system(cmd) != 0)
		fprintf(stderr, "Could not remove %s\n", path);
}

int main(void)
{
	char root[] = "/tmp/test-overlay-root-XXXXXX";
	char overlay[] = "/tmp/test-overlay-dir-XXXXXX";
	char path[256];
	int ret = 1;

	if (mkdtemp(root) == NULL)
		return 1;

	if (mkdtemp(overlay) == NULL)
		return 1;

	snprintf(path, sizeof(path), "%s/persist", root);
	if (mkdir(path, 0755))
		return 1;

	snprintf(path, sizeof(path), "%s/persist/sensors", root);
	if (mkdir(path, 0755))
		return 1;

	snprintf(path, sizeof(path), "%s/persist/sensors/registry", root);
	if (mkdir(path, 0755))
		return 1;

	snprintf(path, sizeof(path), "%s%s", root, REGISTRY_PATH);
	if (write_file(path, "version=0"))
		return 1;

	snprintf(path, sizeof(path), "%s/persist/sensors/sns_reg.conf", root);
	if (write_file(path, "version=0"))
		return 1;

	if (test_copy_up_and_overlay_first(root, overlay))
		goto out;

	if (test_rename_and_remove(root, overlay))
		goto out;

	ret = 0;

out:
	remove_tree(root);
	remove_tree(overlay);

	return ret;
}

/*
 * FastRPC Example
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
#include <libhexagonrpc/fastrpc.h>
#include <libhexagonrpc/interfaces/remotectl.def>
#include <misc/fastrpc.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "aee_error.h"
#include "apps_mem.h"
#include "apps_std.h"
#include "hexagonfs.h"
#include "interfaces/adsp_default_listener.def"
#include "listener.h"
#include "localctl.h"
#include "rpcd_builder.h"

static int remotectl_open(int fd, char *name, struct fastrpc_context **ctx, void (*err_cb)(const char *err))
{
	uint32_t handle;
	int32_t dlret;
	char err[256];
	int ret;

	ret = fastrpc2(&remotectl_open_def, fd, REMOTECTL_HANDLE,
		       strlen(name) + 1, name,
		       &handle,
		       &dlret,
		       256, err);

	if (ret == -1) {
		err_cb(strerror(errno));
		return ret;
	}

	if (dlret == -5) {
		err_cb(err);
		return dlret;
	} else if (dlret) {
		err_cb(aee_strerror[dlret]);
		return dlret;
	}

	*ctx = fastrpc_create_context(fd, handle);

	return ret;
}

static int remotectl_close(struct fastrpc_context *ctx, void (*err_cb)(const char *err))
{
	uint32_t dlret;
	char err[256];
	int ret;

	ret = fastrpc2(&remotectl_close_def, ctx->fd, REMOTECTL_HANDLE,
		       ctx->handle,
		       &dlret,
		       256, err);

	if (ret == -1) {
		err_cb(strerror(errno));
		return ret;
	}

	if (dlret) {
		err_cb(aee_strerror[dlret]);
		return dlret;
	}

	fastrpc_destroy_context(ctx);

	return ret;
}

static int adsp_default_listener_register(struct fastrpc_context *ctx)
{
	return fastrpc(&adsp_default_listener_register_def, ctx);
}

static void remotectl_err(const char *err)
{
	fprintf(stderr, "Could not remotectl: %s\n", err);
}

static int register_fastrpc_listener(int fd)
{
	struct fastrpc_context *ctx;
	int ret;

	ret = remotectl_open(fd, "adsp_default_listener", &ctx, remotectl_err);
	if (ret)
		return 1;

	ret = adsp_default_listener_register(ctx);
	if (ret) {
		fprintf(stderr, "Could not register ADSP default listener\n");
		goto err;
	}

err:
	remotectl_close(ctx, remotectl_err);
	return ret;
}

static void print_usage(const char *argv0)
{
	printf("Usage: %s [options] -f DEVICE\n\n", argv0);
	printf("Server for FastRPC remote procedure calls from Qualcomm DSPs\n\n"
	       "Options:\n"
	       "\t-c SHELL\t\tCreate a new pd running the specified ELF\n"
	       "\t-d DSP\t\tDSP name (default: "")\n"
	       "\t-f DEVICE\tFastRPC device node to attach to\n"
	       "\t-p PROGRAM\tRun client program with shared file descriptor\n"
	       "\t-R DIR\t\tRoot directory of served files (default: /usr/share/qcom/)\n"
	       "\t-s\t\tAttach to sensorspd\n");
}

static int create_shell_pd(int fd, const char *create_shell)
{
	char *buf;
	int shellfd, ret;
	struct stat stats;
	struct fastrpc_alloc_dma_buf dmabuf;
	long page_size;
	struct fastrpc_init_create init = {
		.attrs = 0,
		.siglen = 0,
	};

	shellfd = open(create_shell, O_RDONLY);
	if (shellfd == -1) {
		fprintf(stderr, "Could not open %s: %s\n",
			create_shell, strerror(errno));
		return -1;
	}

	ret = fstat(shellfd, &stats);
	if (ret == -1)
		goto err_close_fd;

	page_size = sysconf(_SC_PAGE_SIZE);
	dmabuf.size = (stats.st_size + page_size - 1) & ~(page_size - 1);
	dmabuf.flags = 0;

	ret = ioctl(fd, FASTRPC_IOCTL_ALLOC_DMA_BUFF, &dmabuf);
	if (ret)
		goto err_close_fd;

	buf = mmap(NULL, dmabuf.size, PROT_WRITE, MAP_SHARED, dmabuf.fd, 0);
	if (buf == MAP_FAILED) {
		ret = -1;
		goto err_close_dmabuf;
	}

	read(shellfd, buf, stats.st_size);

	init.file = (__u64) buf;
	init.filefd = dmabuf.fd;
	init.filelen = stats.st_size;

	ret = ioctl(fd, FASTRPC_IOCTL_INIT_CREATE, &init);

	munmap(buf, dmabuf.size);
err_close_dmabuf:
	close(dmabuf.fd);
err_close_fd:
	close(shellfd);

	return ret;
}

static int setup_environment(int fd)
{
	char *buf;
	int ret;

	buf = malloc(256);
	if (buf == NULL) {
		perror("Could not format file descriptor");
		return 1;
	}

	snprintf(buf, 256, "%d", fd);
	buf[255] = '\0';

	ret = setenv("HEXAGONRPC_FD", buf, 1);

	free(buf);

	return ret;
}

static int terminate_clients(size_t n_pids, const pid_t *pids)
{
	size_t i;

	for (i = 0; i < n_pids; i++) {
		kill(pids[i], SIGTERM);
	}

	return 0;
}

static int start_clients(size_t n_progs, const char **progs, pid_t *pids)
{
	size_t i;

	for (i = 0; i < n_progs; i++) {
		pids[i] = fork();
		if (pids[i] == -1) {
			perror("Could not fork process");
			terminate_clients(i, pids);
			return 1;
		}

		if (pids[i] == 0) {
			execl("/usr/bin/env", "/usr/bin/env", progs[i], (const char *) NULL);
			exit(1);
		}
	}

	return 0;
}

static void *start_reverse_tunnel(int fd, const char *device_dir, const char *dsp)
{
	struct fastrpc_interface **ifaces;
	struct hexagonfs_dirent *root_dir;
	size_t n_ifaces = 3;
	int ret;

	ifaces = malloc(sizeof(struct fastrpc_interface) * n_ifaces);
	if (ifaces == NULL)
		return NULL;

	root_dir = construct_root_dir(device_dir, dsp);

	/*
	 * The apps_remotectl interface patiently waits for this function to
	 * fully populate the ifaces array as long as it receives a pointer to
	 * it.
	 */
	ifaces[REMOTECTL_HANDLE] = fastrpc_localctl_init(n_ifaces, ifaces);

	// Dynamic interfaces with no hardcoded handle
	ifaces[1] = fastrpc_apps_std_init(root_dir);
	ifaces[2] = fastrpc_apps_mem_init(fd);

	ret = register_fastrpc_listener(fd);
	if (ret)
		goto err;

	run_fastrpc_listener(fd, n_ifaces, ifaces);

	fastrpc_localctl_deinit(ifaces[REMOTECTL_HANDLE]);

	free(ifaces);

	return NULL;

err:
	free(ifaces);

	return NULL;
}

int main(int argc, char* argv[])
{
	char *fastrpc_node = NULL;
	const char *device_dir = "/usr/share/qcom/";
	const char *dsp = "";
	const char *create_shell = NULL;
	const char **progs;
	pid_t *pids;
	size_t n_progs = 0;
	int fd, ret, opt;
	bool attach_sns = false;

	progs = malloc(sizeof(const char *) * argc);
	if (progs == NULL) {
		perror("Could not list client programs");
		return 1;
	}

	pids = malloc(sizeof(pid_t) * argc);
	if (pids == NULL) {
		perror("Could not list client PIDs");
		goto err_free_progs;
	}

	while ((opt = getopt(argc, argv, "c:d:f:p:R:s")) != -1) {
		switch (opt) {
			case 'c':
				create_shell = optarg;
				break;
			case 'd':
				dsp = optarg;
				break;
			case 'f':
				fastrpc_node = optarg;
				break;
			case 'p':
				progs[n_progs] = optarg;
				n_progs++;
				break;
			case 'R':
				device_dir = optarg;
				break;
			case 's':
				attach_sns = true;
				break;
			default:
				print_usage(argv[0]);
				goto err_free_pids;
		}
	}

	if (!fastrpc_node) {
		print_usage(argv[0]);
		goto err_free_pids;
	}

	printf("Starting %s (%s) on %s\n", argv[0], attach_sns? "INIT_ATTACH_SNS": "INIT_ATTACH", fastrpc_node);

	fd = open(fastrpc_node, O_RDWR);
	if (fd < 0) {
		fprintf(stderr, "Could not open FastRPC node (%s): %s\n", fastrpc_node, strerror(errno));
		goto err_free_pids;
	}

	if (attach_sns)
		ret = ioctl(fd, FASTRPC_IOCTL_INIT_ATTACH_SNS, NULL);
	else if (create_shell != NULL)
		ret = create_shell_pd(fd, create_shell);
	else
		ret = ioctl(fd, FASTRPC_IOCTL_INIT_ATTACH, NULL);
	if (ret) {
		fprintf(stderr, "Could not attach to FastRPC node: %s\n", strerror(errno));
		goto err_close_dev;
	}

	ret = setup_environment(fd);
	if (ret) {
		perror("Could not setup environment variables");
		goto err_close_dev;
	}

	ret = start_clients(n_progs, progs, pids);
	if (ret)
		goto err_close_dev;

	start_reverse_tunnel(fd, device_dir, dsp);

	terminate_clients(n_progs, pids);

	close(fd);
	free(pids);
	free(progs);

	return 0;

err_close_dev:
	close(fd);
err_free_pids:
	free(pids);
err_free_progs:
	free(progs);
	return 4;
}

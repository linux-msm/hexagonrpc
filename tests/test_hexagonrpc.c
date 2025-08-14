/*
 * FastRPC reverse tunnel - tests for virtual filesystem
 *
 * Copyright (C) 2025 HexagonRPC Contributors
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

#include <libhexagonrpc/hexagonrpc.h>
#include <misc/fastrpc.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <unistd.h>

#ifdef __GLIBC__
typedef unsigned long int ioctl_operation;
#else
typedef int ioctl_operation;
#endif

struct expected_invoke_arg {
	size_t len;
	const void *ptr;
};

static const struct expected_invoke_arg *expected = NULL;
static uint32_t expected_sc = 0;

static const uint32_t no_args_sc = REMOTE_SCALARS_MAKE(0, 0, 0);

static const struct hrpc_method_def_interp4 no_args_def = {
	.msg_id = 0,
	.n_args = 0,
	.args = NULL,
	.n_inner_types = 0,
	.inner_types = NULL,
};

static const unsigned char scalar_arg_exp0[] = {
	0x67, 0x45, 0x23, 0x01,
	0x02, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x10, 0x32, 0x54, 0x76, 0x98, 0xba, 0xdc, 0xfe,
};

static const unsigned char scalar_arg_exp1[] = {
	'h', 'i',
};

static const struct expected_invoke_arg scalar_arg_exp[] = {
	{
		.len = sizeof(scalar_arg_exp0),
		.ptr = scalar_arg_exp0,
	},
	{
		.len = sizeof(scalar_arg_exp1),
		.ptr = scalar_arg_exp1,
	},
	{
		.len = 0,
		.ptr = NULL,
	},
};

static const uint32_t scalar_arg_sc = REMOTE_SCALARS_MAKE(0, 3, 0);

static const struct hrpc_arg_def_interp4 scalar_arg_args[] = {
	{ HRPC_ARG_WORD, sizeof(uint32_t) },
	{ HRPC_ARG_BLOB_SEQ, sizeof(char) },
	{ HRPC_ARG_BLOB_SEQ, sizeof(char) },
	{ HRPC_ARG_WORD, sizeof(uint64_t) },
};

static const struct hrpc_method_def_interp4 scalar_arg_def = {
	.msg_id = 0,
	.n_args = HRPC_ARRAY_SIZE(scalar_arg_args),
	.args = scalar_arg_args,
	.n_inner_types = 0,
	.inner_types = NULL,
};

static const uint32_t out_arg_sc = REMOTE_SCALARS_MAKE(1, 1, 3);

static const unsigned char out_arg_exp0[] = {
	0x02, 0x00, 0x00, 0x00,
	0x05, 0x00, 0x00, 0x00,
};

static const unsigned char out_arg_exp1[] = {
	0x00, 0x00, 0x00, 0x00,
};

static const unsigned char out_arg_exp2[] = {
	'h', 'i',
};

static const unsigned char out_arg_exp3[] = {
	'h', 'e', 'l', 'l', 'o',
};

static const struct expected_invoke_arg out_arg_exp[] = {
	{
		.len = sizeof(out_arg_exp0),
		.ptr = out_arg_exp0,
	},
	{
		.len = sizeof(out_arg_exp1),
		.ptr = out_arg_exp1,
	},
	{
		.len = sizeof(out_arg_exp2),
		.ptr = out_arg_exp2,
	},
	{
		.len = sizeof(out_arg_exp3),
		.ptr = out_arg_exp3,
	},
};

static const struct hrpc_arg_def_interp4 out_arg_args[] = {
	{ HRPC_ARG_OUT_BLOB, sizeof(uint32_t) },
	{ HRPC_ARG_OUT_BLOB_SEQ, sizeof(char) },
	{ HRPC_ARG_OUT_BLOB_SEQ, sizeof(char) },
};

static const struct hrpc_method_def_interp4 out_arg_def = {
	.msg_id = 1,
	.n_args = HRPC_ARRAY_SIZE(out_arg_args),
	.args = out_arg_args,
	.n_inner_types = 0,
	.inner_types = NULL,
};

static const uint32_t thirty_third_method_sc = REMOTE_SCALARS_MAKE(31, 3, 0);

static const unsigned char thirty_third_method_exp0[] = {
	0x20, 0x00, 0x00, 0x00,
	0x67, 0x45, 0x23, 0x01,
	0x02, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
};

static const unsigned char thirty_third_method_exp1[] = {
	'h', 'i',
};

static const struct expected_invoke_arg thirty_third_method_exp[] = {
	{
		.len = sizeof(thirty_third_method_exp0),
		.ptr = thirty_third_method_exp0,
	},
	{
		.len = sizeof(thirty_third_method_exp1),
		.ptr = thirty_third_method_exp1,
	},
	{
		.len = 0,
		.ptr = NULL,
	},
};

static const struct hrpc_arg_def_interp4 thirty_third_method_args[] = {
	{ HRPC_ARG_WORD, sizeof(uint32_t) },
	{ HRPC_ARG_BLOB_SEQ, sizeof(char) },
	{ HRPC_ARG_BLOB_SEQ, sizeof(char) },
};

static const struct hrpc_method_def_interp4 thirty_third_method_def = {
	.msg_id = 32,
	.n_args = HRPC_ARRAY_SIZE(thirty_third_method_args),
	.args = thirty_third_method_args,
	.n_inner_types = 0,
	.inner_types = NULL,
};

int ioctl(int fd, ioctl_operation op, ...)
{
	struct fastrpc_invoke *invoke;
	struct fastrpc_invoke_args *args;
	size_t i, n_inbufs, n_outbufs;
	void *data;
	va_list list;

	va_start(list, op);
	data = va_arg(list, void *);
	va_end(list);

	if (op != (ioctl_operation) FASTRPC_IOCTL_INVOKE) {
		return syscall(SYS_ioctl, fd, op, data);
	} else {
		invoke = data;

		if (fd != -1)
			return -1;

		if (invoke->handle != 0)
			return -1;

		if (invoke->sc != expected_sc)
			return -1;

		n_inbufs = REMOTE_SCALARS_INBUFS(invoke->sc);
		n_outbufs = REMOTE_SCALARS_OUTBUFS(invoke->sc);
		args = (struct fastrpc_invoke_args *) invoke->args;

		for (i = 0; i < n_inbufs; i++) {
			if (args[i].fd != -1 || args[i].attr != 0)
				return -1;

			if (args[i].length != expected[i].len)
				return -1;

			if (memcmp((void *) args[i].ptr,
				   expected[i].ptr,
				   args[i].length))
				return -1;
		}

		for (i = n_inbufs; i < n_inbufs + n_outbufs; i++) {
			if (args[i].fd != -1 || args[i].attr != 0)
				return -1;

			if (args[i].length < expected[i].len)
				return -1;

			memcpy((void *) args[i].ptr,
			       expected[i].ptr,
			       args[i].length);
		}

		return 0;
	}
}

int main(void)
{
	int ret, fd = -1;
	uint32_t hdl = 0;
	uint32_t word_out;
	char hi[2];
	char hello[5];

	expected_sc = no_args_sc;
	expected = NULL;
	ret = hexagonrpc(&no_args_def, fd, hdl);
	if (ret)
		return 1;

	expected_sc = scalar_arg_sc;
	expected = scalar_arg_exp;
	ret = hexagonrpc(&scalar_arg_def, fd, hdl,
			 (uint32_t) 0x01234567,
			 (uint32_t) 2, (const void *) "hi",
			 (uint32_t) 0, (const void *) NULL,
			 (uint64_t) 0xfedcba9876543210 );
	if (ret)
		return 1;

	expected_sc = out_arg_sc;
	expected = out_arg_exp;
	ret = hexagonrpc(&out_arg_def, fd, hdl,
			 (void *) &word_out,
			 (uint32_t) 2, (void *) hi,
			 (uint32_t) 5, (void *) hello);
	if (ret)
		return 1;

	expected_sc = thirty_third_method_sc;
	expected = thirty_third_method_exp;
	ret = hexagonrpc(&thirty_third_method_def, fd, hdl,
			 (uint32_t) 0x01234567,
			 (uint32_t) 2, (const void *) "hi",
			 (uint32_t) 0, (const void *) NULL);
	if (ret)
		return 1;

	return 0;
}

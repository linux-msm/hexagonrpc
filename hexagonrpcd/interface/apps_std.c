/*
 * FastRPC operating system interface - method definitions
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

#include <stdint.h>
#include <libhexagonrpc/hexagonrpc.h>

static struct hrpc_arg_def_interp4 apps_std_freopen_args[] = {
	{ HRPC_ARG_WORD, sizeof(uint32_t) },
	{ HRPC_ARG_OUT_BLOB, sizeof(uint32_t) },
	{ HRPC_ARG_OUT_BLOB_SEQ, sizeof(char) },
};

const struct hrpc_method_def_interp4 apps_std_freopen_def = {
	.msg_id = 1,
	.n_args = HRPC_ARRAY_SIZE(apps_std_freopen_args),
	.args = apps_std_freopen_args,
	.n_inner_types = 0,
	.inner_types = NULL,
};

static struct hrpc_arg_def_interp4 apps_std_fflush_args[] = {
	{ HRPC_ARG_WORD, sizeof(uint32_t) },
};

const struct hrpc_method_def_interp4 apps_std_fflush_def = {
	.msg_id = 2,
	.n_args = HRPC_ARRAY_SIZE(apps_std_fflush_args),
	.args = apps_std_fflush_args,
	.n_inner_types = 0,
	.inner_types = NULL,
};

static struct hrpc_arg_def_interp4 apps_std_fclose_args[] = {
	{ HRPC_ARG_WORD, sizeof(uint32_t) },
};

const struct hrpc_method_def_interp4 apps_std_fclose_def = {
	.msg_id = 3,
	.n_args = HRPC_ARRAY_SIZE(apps_std_fclose_args),
	.args = apps_std_fclose_args,
	.n_inner_types = 0,
	.inner_types = NULL,
};

static struct hrpc_arg_def_interp4 apps_std_fread_args[] = {
	{ HRPC_ARG_WORD, sizeof(uint32_t) },
	{ HRPC_ARG_OUT_BLOB, sizeof(uint32_t) },
	{ HRPC_ARG_OUT_BLOB, sizeof(uint32_t) },
	{ HRPC_ARG_OUT_BLOB_SEQ, sizeof(char) },
};

const struct hrpc_method_def_interp4 apps_std_fread_def = {
	.msg_id = 4,
	.n_args = HRPC_ARRAY_SIZE(apps_std_fread_args),
	.args = apps_std_fread_args,
	.n_inner_types = 0,
	.inner_types = NULL,
};

static struct hrpc_arg_def_interp4 apps_std_fseek_args[] = {
	{ HRPC_ARG_WORD, sizeof(uint32_t) },
	{ HRPC_ARG_WORD, sizeof(uint32_t) },
	{ HRPC_ARG_WORD, sizeof(uint32_t) },
};

const struct hrpc_method_def_interp4 apps_std_fseek_def = {
	.msg_id = 9,
	.n_args = HRPC_ARRAY_SIZE(apps_std_fseek_args),
	.args = apps_std_fseek_args,
	.n_inner_types = 0,
	.inner_types = NULL,
};

static struct hrpc_arg_def_interp4 apps_std_fopen_with_env_args[] = {
	{ HRPC_ARG_BLOB_SEQ, sizeof(char) },
	{ HRPC_ARG_BLOB_SEQ, sizeof(char) },
	{ HRPC_ARG_BLOB_SEQ, sizeof(char) },
	{ HRPC_ARG_BLOB_SEQ, sizeof(char) },
	{ HRPC_ARG_OUT_BLOB, sizeof(uint32_t) },
};

const struct hrpc_method_def_interp4 apps_std_fopen_with_env_def = {
	.msg_id = 19,
	.n_args = HRPC_ARRAY_SIZE(apps_std_fopen_with_env_args),
	.args = apps_std_fopen_with_env_args,
	.n_inner_types = 0,
	.inner_types = NULL,
};

static struct hrpc_arg_def_interp4 apps_std_opendir_args[] = {
	{ HRPC_ARG_BLOB_SEQ, sizeof(char) },
	{ HRPC_ARG_OUT_BLOB, sizeof(uint64_t) },
};

const struct hrpc_method_def_interp4 apps_std_opendir_def = {
	.msg_id = 26,
	.n_args = HRPC_ARRAY_SIZE(apps_std_opendir_args),
	.args = apps_std_opendir_args,
	.n_inner_types = 0,
	.inner_types = NULL,
};

static struct hrpc_arg_def_interp4 apps_std_closedir_args[] = {
	{ HRPC_ARG_WORD, sizeof(uint64_t) },
};

const struct hrpc_method_def_interp4 apps_std_closedir_def = {
	.msg_id = 27,
	.n_args = HRPC_ARRAY_SIZE(apps_std_closedir_args),
	.args = apps_std_closedir_args,
	.n_inner_types = 0,
	.inner_types = NULL,
};

static struct hrpc_arg_def_interp4 apps_std_readdir_args[] = {
	{ HRPC_ARG_WORD, sizeof(uint64_t) },
	{ HRPC_ARG_OUT_BLOB, sizeof(uint32_t) },
	{ HRPC_ARG_OUT_BLOB, sizeof(char) * 256 },
	{ HRPC_ARG_OUT_BLOB, sizeof(uint32_t) },
};

const struct hrpc_method_def_interp4 apps_std_readdir_def = {
	.msg_id = 28,
	.n_args = HRPC_ARRAY_SIZE(apps_std_readdir_args),
	.args = apps_std_readdir_args,
	.n_inner_types = 0,
	.inner_types = NULL,
};

static struct hrpc_arg_def_interp4 apps_std_mkdir_args[] = {
	{ HRPC_ARG_BLOB, sizeof(char) },
	{ HRPC_ARG_WORD, sizeof(uint32_t) },
};

const struct hrpc_method_def_interp4 apps_std_mkdir_def = {
	.msg_id = 29,
	.n_args = HRPC_ARRAY_SIZE(apps_std_mkdir_args),
	.args = apps_std_mkdir_args,
	.n_inner_types = 0,
	.inner_types = NULL,
};

static struct hrpc_arg_def_interp4 apps_std_stat_args[] = {
	{ HRPC_ARG_BLOB_SEQ, sizeof(char) },
	{ HRPC_ARG_OUT_BLOB, sizeof(uint64_t) },
	{ HRPC_ARG_OUT_BLOB, sizeof(uint64_t) },
	{ HRPC_ARG_OUT_BLOB, sizeof(uint64_t) },
	{ HRPC_ARG_OUT_BLOB, sizeof(uint32_t) },
	{ HRPC_ARG_OUT_BLOB, sizeof(uint32_t) },
	{ HRPC_ARG_OUT_BLOB, sizeof(uint64_t) },
	{ HRPC_ARG_OUT_BLOB, sizeof(uint64_t) },
	{ HRPC_ARG_OUT_BLOB, sizeof(uint64_t) },
	{ HRPC_ARG_OUT_BLOB, sizeof(uint64_t) },
	{ HRPC_ARG_OUT_BLOB, sizeof(uint64_t) },
	{ HRPC_ARG_OUT_BLOB, sizeof(uint64_t) },
	{ HRPC_ARG_OUT_BLOB, sizeof(uint64_t) },
	{ HRPC_ARG_OUT_BLOB, sizeof(uint64_t) },
};

const struct hrpc_method_def_interp4 apps_std_stat_def = {
	.msg_id = 31,
	.n_args = HRPC_ARRAY_SIZE(apps_std_stat_args),
	.args = apps_std_stat_args,
	.n_inner_types = 0,
	.inner_types = NULL,
};

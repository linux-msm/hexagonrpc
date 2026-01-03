/*
 * Remote processor control interface - method definitions
 *
 * Copyright (C) 2025 HexagonRPC Contributors
 *
 * This file is part of HexagonRPC.
 *
 * HexagonRPC is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
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

struct hrpc_arg_def_interp4 remotectl_open_args[] = {
	{ HRPC_ARG_BLOB_SEQ, sizeof(char) },
	{ HRPC_ARG_OUT_BLOB, sizeof(uint32_t) },
	{ HRPC_ARG_OUT_BLOB_SEQ, sizeof(char) },
	{ HRPC_ARG_OUT_BLOB, sizeof(uint32_t) },
};

const struct hrpc_method_def_interp4 remotectl_open_def = {
	.msg_id = 0,
	.n_args = HRPC_ARRAY_SIZE(remotectl_open_args),
	.args = remotectl_open_args,
	.n_inner_types = 0,
	.inner_types = NULL,
};

struct hrpc_arg_def_interp4 remotectl_close_args[] = {
	{ HRPC_ARG_WORD, sizeof(uint32_t) },
	{ HRPC_ARG_OUT_BLOB_SEQ, sizeof(char) },
	{ HRPC_ARG_OUT_BLOB, sizeof(uint32_t) },
};

const struct hrpc_method_def_interp4 remotectl_close_def = {
	.msg_id = 1,
	.n_args = HRPC_ARRAY_SIZE(remotectl_close_args),
	.args = remotectl_close_args,
	.n_inner_types = 0,
	.inner_types = NULL,
};

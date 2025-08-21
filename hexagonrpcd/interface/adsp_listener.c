/*
 * FastRPC reverse tunnel main interface - method definitions
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

const struct hrpc_method_def_interp4 adsp_listener_init2_def = {
	.msg_id = 3,
	.n_args = 0,
	.args = NULL,
	.n_inner_types = 0,
	.inner_types = NULL,
};

struct hrpc_arg_def_interp4 adsp_listener_next2_args[] = {
	{ HRPC_ARG_WORD, sizeof(uint32_t) },
	{ HRPC_ARG_WORD, sizeof(uint32_t) },
	{ HRPC_ARG_BLOB_SEQ, sizeof(char) },
	{ HRPC_ARG_OUT_BLOB, sizeof(uint32_t) },
	{ HRPC_ARG_OUT_BLOB, sizeof(uint32_t) },
	{ HRPC_ARG_OUT_BLOB, sizeof(uint32_t) },
	{ HRPC_ARG_OUT_BLOB, sizeof(uint32_t) },
	{ HRPC_ARG_OUT_BLOB_SEQ, sizeof(char) },
};

const struct hrpc_method_def_interp4 adsp_listener_next2_def = {
	.msg_id = 4,
	.n_args = HRPC_ARRAY_SIZE(adsp_listener_next2_args),
	.args = adsp_listener_next2_args,
	.n_inner_types = 0,
	.inner_types = NULL,
};

/*
 * FastRPC argument marshalling
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

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <misc/fastrpc.h>
#include <sys/ioctl.h>

#include <libhexagonrpc/hexagonrpc.h>

/*
 * Operations to be done for a stage of invoking.
 *
 * The hexagonrpc function has 5 stages that parse the method definition:
 * - count: count the I/O buffers in need of marshalling
 * - alloc: populate fastrpc_invoke_args with pointers, allocating when needed
 * - encode: marshal va_args into primary input buffer
 * - decode: demarshal primary output buffer into va_args buffers
 * - free: free any buffers allocated by the alloc stage
 *
 * These stages interpret the arguments in the exact same way and have a
 * similar control structure. To reduce code duplication, each stage can call
 * emit_args with their own stage_ops callbacks and state.
 */
struct stage_ops {
	void (*emit_prim_in)(size_t size, const void *ptr, void *data);
	void (*emit_prim_out)(size_t size, void *ptr, void *data);
	void (*emit_inbuf)(size_t size, const void *ptr, void *data);
	void (*emit_outbuf)(size_t size, void *ptr, void *data);

	int (*emit_type_seq_in)(const struct hrpc_method_def_interp4 *def,
				const struct hrpc_inner_type_def_interp4 *type,
				void *data, size_t n_inst, const void *inst);
	int (*emit_type_seq_out)(const struct hrpc_method_def_interp4 *def,
				 const struct hrpc_inner_type_def_interp4 *type,
				 void *data, size_t n_inst, void *inst);
};

// Used by count_ops
struct buf_count {
	uint8_t n_inbufs;
	uint8_t n_outbufs;
	uint32_t n_prim_in;
	uint32_t n_prim_out;
};

// Used by alloc_ops and free_ops
struct curr_buf {
	struct fastrpc_invoke_args *inbuf;
	struct fastrpc_invoke_args *outbuf;
};

// Used by encode_ops and decode_ops
struct curr_pos {
	struct fastrpc_invoke_args *buf;
	void *prim;
};

static const struct stage_ops count_ops;
static const struct stage_ops alloc_ops;
static const struct stage_ops encode_ops;
static const struct stage_ops decode_ops;
static const struct stage_ops free_ops;

static int emit_inner_type_in(const struct hrpc_inner_type_def_interp4 *type,
			      const struct stage_ops *ops, void *data,
			      const void **inst)
{
	uint32_t arg32;
	void *ptr;
	size_t i, size;

	for (i = 0; i < type->s; i++) {
		switch (type->p[i].t) {
			case HRPC_ARG_BLOB:
				size = type->p[i].d;
				ops->emit_prim_in(size, *inst, data);
				*inst = (char *) *inst + size;
				break;
			case HRPC_ARG_BLOB_SEQ:
				arg32 = *(const uint32_t *) *inst;
				*inst = (uint32_t *) *inst + 1;
				ptr = *(void **) *inst;
				*inst = (void **) *inst + 1;

				size = (size_t) type->p[i].d * arg32;

				ops->emit_prim_in(sizeof(arg32), &arg32, data);
				ops->emit_inbuf(size, ptr, data);

				break;
			// Not seen so far in public IDLs
			case HRPC_ARG_TYPE_SEQ:
				return -1;
			default:
				return -1;
		}
	}

	return 0;
}

static int emit_inner_type_out(const struct hrpc_inner_type_def_interp4 *type,
			       const struct stage_ops *ops, void *data,
			       void **inst)
{
	uint32_t arg32;
	void *ptr;
	size_t i, size;

	for (i = 0; i < type->s; i++) {
		switch (type->p[i].t) {
			case HRPC_ARG_BLOB:
				size = type->p[i].d;
				ops->emit_prim_out(size, *inst, data);
				*inst = (char *) *inst + size;
				break;
			case HRPC_ARG_BLOB_SEQ:
				arg32 = *(const uint32_t *) *inst;
				*inst = (uint32_t *) *inst + 1;
				ptr = *(void **) *inst;
				*inst = (void **) *inst + 1;

				size = (size_t) type->p[i].d * arg32;

				ops->emit_prim_in(sizeof(arg32), &arg32, data);
				ops->emit_outbuf(size, ptr, data);

				break;
			// Not seen so far in public IDLs
			case HRPC_ARG_TYPE_SEQ:
				return -1;
			default:
				return -1;
		}
	}

	return 0;
}

static int emit_args(const struct hrpc_method_def_interp4 *def,
		     const struct stage_ops *ops, void *data,
		     va_list list)
{
	const struct hrpc_inner_type_def_interp4 *inner_type;
	uint32_t arg32;
	uint64_t arg64;
	const void *iptr;
	void *optr;
	size_t i, size;
	int ret;

	for (i = 0; i < def->n_args; i++) {
		switch (def->args[i].t) {
			case HRPC_ARG_WORD:
				size = def->args[i].d;
				if (size == sizeof(uint32_t)) {
					arg32 = va_arg(list, uint32_t);
					ops->emit_prim_in(size, &arg32, data);
				} else if (size == sizeof(uint64_t)) {
					arg64 = va_arg(list, uint64_t);
					ops->emit_prim_in(size, &arg64, data);
				} else {
					return -1;
				}

				break;
			case HRPC_ARG_BLOB:
				size = def->args[i].d;
				iptr = va_arg(list, const void *);
				ops->emit_prim_in(size, iptr, data);
				break;
			case HRPC_ARG_TYPE:
				iptr = va_arg(list, const void *);
				inner_type = &def->inner_types[def->args[i].d];
				ret = emit_inner_type_in(inner_type,
							 ops, data, &iptr);
				if (ret)
					return ret;
				break;
			case HRPC_ARG_BLOB_SEQ:
				arg32 = va_arg(list, uint32_t);
				iptr = va_arg(list, const void *);
				size = (size_t) def->args[i].d * arg32;

				ops->emit_prim_in(sizeof(arg32), &arg32, data);
				ops->emit_inbuf(size, iptr, data);

				break;
			case HRPC_ARG_TYPE_SEQ:
				arg32 = va_arg(list, uint32_t);
				iptr = va_arg(list, const void *);
				ret = ops->emit_type_seq_in(def, inner_type,
							    data, arg32, iptr);
				if (ret)
					return ret;
				break;
			case HRPC_ARG_OUT_BLOB:
				optr = va_arg(list, void *);
				ops->emit_prim_out(def->args[i].d, optr, data);
				break;
			case HRPC_ARG_OUT_TYPE:
				optr = va_arg(list, void *);
				inner_type = &def->inner_types[def->args[i].d];
				ret = emit_inner_type_out(inner_type,
							  ops, data, &optr);
				if (ret)
					return ret;
				break;
			case HRPC_ARG_OUT_BLOB_SEQ:
				arg32 = va_arg(list, uint32_t);
				optr = va_arg(list, void *);
				size = (size_t) def->args[i].d * arg32;

				ops->emit_prim_in(sizeof(arg32), &arg32, data);
				ops->emit_outbuf(size, optr, data);

				break;
			case HRPC_ARG_OUT_TYPE_SEQ:
				arg32 = va_arg(list, uint32_t);
				optr = va_arg(list, void *);
				ret = ops->emit_type_seq_out(def, inner_type,
							     data, arg32, optr);
				if (ret)
					return ret;
				break;
			default:
				return -1;
		}
	}

	return 0;
}

static void emit_nop_in(size_t size, const void *ptr, void *data)
{
}

static void emit_nop_out(size_t size, void *ptr, void *data)
{
}

static void count_prim_in(size_t size, const void *ptr, void *data)
{
	((struct buf_count *) data)->n_prim_in += size;
}

static void count_prim_out(size_t size, void *ptr, void *data)
{
	((struct buf_count *) data)->n_prim_out += size;
}

static void count_inbuf(size_t size, const void *ptr, void *data)
{
	((struct buf_count *) data)->n_inbufs++;
}

static void count_outbuf(size_t size, void *ptr, void *data)
{
	((struct buf_count *) data)->n_outbufs++;
}

static int count_type_seq_in(const struct hrpc_method_def_interp4 *def,
			     const struct hrpc_inner_type_def_interp4 *type,
			     void *data, size_t n_inst, const void *inst)
{
	struct buf_count inner_count = { 0, 0, 0, 0 };
	struct buf_count *count = data;
	int ret = 0;

	if (n_inst) {
		ret = emit_inner_type_in(type,
					 &count_ops, &inner_count,
					 &inst);
		count->n_inbufs++;
		count->n_inbufs += inner_count.n_inbufs * n_inst;
	}

	return ret;
}

static int count_type_seq_out(const struct hrpc_method_def_interp4 *def,
			      const struct hrpc_inner_type_def_interp4 *type,
			      void *data, size_t n_inst, void *inst)
{
	struct buf_count inner_count = { 0, 0, 0, 0 };
	struct buf_count *count = data;
	int ret = 0;

	if (n_inst) {
		ret = emit_inner_type_out(type,
					  &count_ops, &inner_count,
					  &inst);
		count->n_inbufs++;
		count->n_outbufs += inner_count.n_outbufs * n_inst;
	}

	return ret;
}

static const struct stage_ops count_ops = {
	.emit_prim_in = count_prim_in,
	.emit_prim_out = count_prim_out,
	.emit_inbuf = count_inbuf,
	.emit_outbuf = count_outbuf,
	.emit_type_seq_in = count_type_seq_in,
	.emit_type_seq_out = count_type_seq_out,
};

static void alloc_inbuf(size_t size, const void *ptr, void *data)
{
	struct curr_buf *curr = data;

	curr->inbuf->length = size;
	curr->inbuf->ptr = (__u64) ptr;
	curr->inbuf++;
}

static void alloc_outbuf(size_t size, void *ptr, void *data)
{
	struct curr_buf *curr = data;

	curr->outbuf->length = size;
	curr->outbuf->ptr = (__u64) ptr;
	curr->outbuf++;
}

static int alloc_type_seq_in(const struct hrpc_method_def_interp4 *def,
			     const struct hrpc_inner_type_def_interp4 *type,
			     void *data, size_t n_inst, const void *inst)
{
	struct buf_count inner_count = { 0, 0, 0, 0 };
	struct curr_buf *curr = data;
	int ret = 0;
	size_t i;

	if (n_inst) {
		ret = emit_inner_type_in(type,
					 &count_ops, &inner_count,
					 &inst);
		if (ret)
			return ret;

		curr->inbuf->length = inner_count.n_prim_in * n_inst;
		curr->inbuf->ptr = (__u64) malloc(curr->inbuf->length);
		if (curr->inbuf->ptr == 0)
			return -1;
	}

	curr->inbuf++;

	for (i = 0; i < n_inst; i++) {
		ret = emit_inner_type_in(type, &alloc_ops, data, &inst);
		if (ret)
			return ret;
	}

	return ret;
}

static int alloc_type_seq_out(const struct hrpc_method_def_interp4 *def,
			      const struct hrpc_inner_type_def_interp4 *type,
			      void *data, size_t n_inst, void *inst)
{
	struct buf_count inner_count = { 0, 0, 0, 0 };
	struct curr_buf *curr = data;
	int ret = 0;
	size_t i;

	if (n_inst) {
		ret = emit_inner_type_out(type,
					  &count_ops, &inner_count,
					  &inst);
		if (ret)
			return ret;

		curr->inbuf->length = inner_count.n_prim_in * n_inst;
		curr->inbuf->ptr = (__u64) malloc(curr->inbuf->length);
		if (curr->inbuf->ptr == 0)
			return -1;

		curr->outbuf->length = inner_count.n_prim_out * n_inst;
		curr->outbuf->ptr = (__u64) malloc(curr->outbuf->length);
		if (curr->outbuf->ptr == 0)
			return -1;
	}

	curr->inbuf++;
	curr->outbuf++;

	for (i = 0; i < n_inst; i++) {
		ret = emit_inner_type_out(type, &alloc_ops, data, &inst);
		if (ret)
			return ret;
	}

	return 0;
}

static const struct stage_ops alloc_ops = {
	.emit_prim_in = emit_nop_in,
	.emit_prim_out = emit_nop_out,
	.emit_inbuf = alloc_inbuf,
	.emit_outbuf = alloc_outbuf,
	.emit_type_seq_in = alloc_type_seq_in,
	.emit_type_seq_out = alloc_type_seq_out,
};

static void encode_prim_in(size_t size, const void *ptr, void *data)
{
	struct curr_pos *pos = data;

	memcpy(pos->prim, ptr, size);
	pos->prim = (char *) pos->prim + size;
}

static void encode_inbuf(size_t size, const void *ptr, void *data)
{
	((struct curr_pos *) data)->buf++;
}

static int encode_type_seq_in(const struct hrpc_method_def_interp4 *def,
			      const struct hrpc_inner_type_def_interp4 *type,
			      void *data, size_t n_inst, const void *inst)
{
	struct curr_pos inner_pos;
	struct curr_pos *pos = data;
	int ret;
	size_t i;

	inner_pos.buf = pos->buf;
	inner_pos.prim = (void *) pos->buf->ptr;

	for (i = 0; i < n_inst; i++) {
		ret = emit_inner_type_in(type, &encode_ops, &inner_pos, &inst);
		if (ret)
			return ret;
	}

	pos->buf = inner_pos.buf;
	return 0;
}

static int encode_type_seq_out(const struct hrpc_method_def_interp4 *def,
			       const struct hrpc_inner_type_def_interp4 *type,
			       void *data, size_t n_inst, void *inst)
{
	struct curr_pos inner_pos;
	struct curr_pos *pos = data;
	int ret;
	size_t i;

	inner_pos.buf = pos->buf;
	inner_pos.prim = (void *) pos->buf->ptr;

	for (i = 0; i < n_inst; i++) {
		ret = emit_inner_type_out(type, &encode_ops, &inner_pos, &inst);
		if (ret)
			return ret;
	}

	pos->buf = inner_pos.buf;
	return 0;
}

static const struct stage_ops encode_ops = {
	.emit_prim_in = encode_prim_in,
	.emit_prim_out = emit_nop_out,
	.emit_inbuf = encode_inbuf,
	.emit_outbuf = emit_nop_out,
	.emit_type_seq_in = encode_type_seq_in,
	.emit_type_seq_out = encode_type_seq_out,
};

static void decode_prim_out(size_t size, void *ptr, void *data)
{
	struct curr_pos *pos = data;

	memcpy(ptr, pos->prim, size);
	pos->prim = (char *) pos->prim + size;
}

static void decode_outbuf(size_t size, void *ptr, void *data)
{
	((struct curr_pos *) data)->buf++;
}

static int decode_type_seq_in(const struct hrpc_method_def_interp4 *def,
			      const struct hrpc_inner_type_def_interp4 *type,
			      void *data, size_t n_inst, const void *inst)
{
	return 0;
}

static int decode_type_seq_out(const struct hrpc_method_def_interp4 *def,
			       const struct hrpc_inner_type_def_interp4 *type,
			       void *data, size_t n_inst, void *inst)
{
	struct curr_pos inner_pos;
	struct curr_pos *pos = data;
	int ret;
	size_t i;

	inner_pos.buf = pos->buf;
	inner_pos.prim = (void *) pos->buf->ptr;

	for (i = 0; i < n_inst; i++) {
		ret = emit_inner_type_out(type, &decode_ops, &inner_pos, &inst);
		if (ret)
			return ret;
	}

	pos->buf = inner_pos.buf;
	return 0;
}

static const struct stage_ops decode_ops = {
	.emit_prim_in = emit_nop_in,
	.emit_prim_out = decode_prim_out,
	.emit_inbuf = emit_nop_in,
	.emit_outbuf = decode_outbuf,
	.emit_type_seq_in = decode_type_seq_in,
	.emit_type_seq_out = decode_type_seq_out,
};

static void free_inbuf(size_t size, const void *ptr, void *data)
{
	((struct curr_buf *) data)->inbuf++;
}

static void free_outbuf(size_t size, void *ptr, void *data)
{
	((struct curr_buf *) data)->outbuf++;
}

static int free_type_seq_in(const struct hrpc_method_def_interp4 *def,
			    const struct hrpc_inner_type_def_interp4 *type,
			    void *data, size_t n_inst, const void *inst)
{
	struct curr_buf *curr = data;
	int ret = 0;
	size_t i;

	if (n_inst) {
		free((void *) curr->inbuf->ptr);
	}

	curr->inbuf++;

	for (i = 0; i < n_inst; i++) {
		ret = emit_inner_type_in(type, &free_ops, data, &inst);
		if (ret)
			return ret;
	}

	return 0;
}

static int free_type_seq_out(const struct hrpc_method_def_interp4 *def,
			     const struct hrpc_inner_type_def_interp4 *type,
			     void *data, size_t n_inst, void *inst)
{
	struct curr_buf *curr = data;
	int ret = 0;
	size_t i;

	if (n_inst) {
		free((void *) curr->inbuf->ptr);
		free((void *) curr->outbuf->ptr);
	}

	curr->inbuf++;
	curr->outbuf++;

	for (i = 0; i < n_inst; i++) {
		ret = emit_inner_type_out(type, &free_ops, data, &inst);
		if (ret)
			return ret;
	}

	return 0;
}

static const struct stage_ops free_ops = {
	.emit_prim_in = emit_nop_in,
	.emit_prim_out = emit_nop_out,
	.emit_inbuf = free_inbuf,
	.emit_outbuf = free_outbuf,
	.emit_type_seq_in = free_type_seq_in,
	.emit_type_seq_out = free_type_seq_out,
};

static int count_args(const struct hrpc_method_def_interp4 *def,
		      struct buf_count *count, va_list list)
{
	int ret;

	if (def->msg_id > 30)
		count->n_prim_in += 4;

	ret = emit_args(def, &count_ops, count, list);
	if (ret)
		return ret;

	if (count->n_prim_in)
		count->n_inbufs++;
	if (count->n_prim_out)
		count->n_outbufs++;

	return 0;
}

static void free_args(const struct hrpc_method_def_interp4 *def,
		      const struct buf_count *count,
		      struct fastrpc_invoke_args *args,
		      va_list list);

static struct fastrpc_invoke_args *alloc_args(const struct hrpc_method_def_interp4 *def,
					      const struct buf_count *count,
					      va_list list)
{
	struct curr_buf curr_buf = { NULL, NULL };
	struct fastrpc_invoke_args *args;
	size_t i;
	int ret = -1;
	va_list peek;

	args = malloc(sizeof(*args) * (count->n_inbufs + count->n_outbufs));
	if (args == NULL)
		return NULL;

	for (i = 0; i < count->n_inbufs + count->n_outbufs; i++) {
		args[i].fd = -1;
		args[i].attr = 0;
		args[i].ptr = 0;
	}

	curr_buf.inbuf = &args[0];
	curr_buf.outbuf = &args[count->n_inbufs];

	if (count->n_prim_in) {
		curr_buf.inbuf->length = count->n_prim_in;
		curr_buf.inbuf->ptr = (__u64) malloc(count->n_prim_in);
		if (curr_buf.inbuf->ptr == 0)
			goto err;
		curr_buf.inbuf++;
	}

	if (count->n_prim_out) {
		curr_buf.outbuf->length = count->n_prim_out;
		curr_buf.outbuf->ptr = (__u64) malloc(count->n_prim_out);
		if (curr_buf.outbuf->ptr == 0)
			goto err;
		curr_buf.outbuf++;
	}

	va_copy(peek, list);
	ret = emit_args(def, &alloc_ops, &curr_buf, peek);
	va_end(peek);
	if (ret)
		goto err;

	return args;

err:
	va_copy(peek, list);
	free_args(def, count, args, peek);
	va_end(peek);
	return NULL;
}

static int encode_args(const struct hrpc_method_def_interp4 *def,
		       const struct buf_count *count,
		       struct fastrpc_invoke_args *args,
		       va_list list)
{
	struct curr_pos curr_pos;

	curr_pos.buf = &args[0];
	if (count->n_prim_in) {
		curr_pos.prim = (void *) args[0].ptr;
		curr_pos.buf++;
	} else {
		curr_pos.prim = NULL;
	}

	if (def->msg_id > 30) {
		*(uint32_t *) curr_pos.prim = def->msg_id;
		curr_pos.prim = (uint32_t *) curr_pos.prim + 1;
	}

	return emit_args(def, &encode_ops, &curr_pos, list);
}

static void decode_args(const struct hrpc_method_def_interp4 *def,
			const struct buf_count *count,
			struct fastrpc_invoke_args *args,
			va_list list)
{
	struct curr_pos curr_pos;

	curr_pos.buf = &args[count->n_inbufs];
	if (count->n_prim_out) {
		curr_pos.prim = (void *) args[count->n_inbufs].ptr;
		curr_pos.buf++;
	} else {
		curr_pos.prim = NULL;
	}

	emit_args(def, &decode_ops, &curr_pos, list);
}

static void free_args(const struct hrpc_method_def_interp4 *def,
		      const struct buf_count *count,
		      struct fastrpc_invoke_args *args,
		      va_list list)
{
	struct curr_buf curr_buf = { NULL, NULL };

	curr_buf.inbuf = &args[0];
	curr_buf.outbuf = &args[count->n_inbufs];

	if (count->n_prim_in) {
		if (curr_buf.inbuf->ptr)
			free((void *) curr_buf.inbuf->ptr);
		curr_buf.inbuf++;
	}

	if (count->n_prim_out) {
		if (curr_buf.outbuf->ptr)
			free((void *) curr_buf.outbuf->ptr);
		curr_buf.outbuf++;
	}

	emit_args(def, &free_ops, &curr_buf, list);

	free(args);
}

/*
 * 1. validate def and count prim_in, prim_out sizes
 * 2. construct prim_in, sequences with nestings
 * 3. point inbufs to sequences, nested sequences
 * 4. return prim_out
 */
int vhexagonrpc(const struct hrpc_method_def_interp4 *def,
		int fd, uint32_t handle, va_list list)
{
	struct buf_count count = { 0, 0, 0, 0 };
	struct fastrpc_invoke invoke;
	struct fastrpc_invoke_args *args;
	va_list peek;
	int ret;

	va_copy(peek, list);
	ret = count_args(def, &count, peek);
	va_end(peek);
	if (ret)
		return ret;

	va_copy(peek, list);
	args = alloc_args(def, &count, peek);
	va_end(peek);
	if (args == NULL)
		return -1;

	va_copy(peek, list);
	ret = encode_args(def, &count, args, peek);
	va_end(peek);
	if (ret)
		goto err;

	invoke.args = (__u64) args;
	invoke.handle = handle;
	if (def->msg_id > 30)
		invoke.sc = REMOTE_SCALARS_MAKE(31,
						count.n_inbufs, count.n_outbufs);
	else
		invoke.sc = REMOTE_SCALARS_MAKE(def->msg_id,
						count.n_inbufs, count.n_outbufs);

	ret = ioctl(fd, FASTRPC_IOCTL_INVOKE, (__u64) &invoke);
	if (ret)
		goto err;

	va_copy(peek, list);
	decode_args(def, &count, args, peek);
	va_end(peek);

err:
	va_copy(peek, list);
	free_args(def, &count, args, peek);
	va_end(peek);
	return ret;
}

int hexagonrpc(const struct hrpc_method_def_interp4 *def,
	       int fd, uint32_t handle, ...)
{
	va_list list;
	int ret;

	va_start(list, handle);
	ret = vhexagonrpc(def, fd, handle, list);
	va_end(list);

	return ret;
}

#ifndef LISTENER_H
#define LISTENER_H

#include "fastrpc.h"
#include "iobuffer.h"

struct fastrpc_function_impl {
	const struct fastrpc_function_def_interp2 *def;
	uint32_t (*impl)(const struct fastrpc_io_buffer *inbufs,
			 struct fastrpc_io_buffer *outbufs);
};

struct fastrpc_interface {
	const char *name;
	uint8_t n_procs;
	struct fastrpc_function_impl procs[];
};

int run_fastrpc_listener(int fd);

#endif

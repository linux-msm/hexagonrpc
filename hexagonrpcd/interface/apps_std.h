/*
 * FastRPC operating system interface - API
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

#ifndef INTERFACE_APPS_STD_H
#define INTERFACE_APPS_STD_H

#include <libhexagonrpc/hexagonrpc.h>

extern struct hrpc_method_def_interp4 apps_std_freopen_def;
extern struct hrpc_method_def_interp4 apps_std_fflush_def;
extern struct hrpc_method_def_interp4 apps_std_fclose_def;
extern struct hrpc_method_def_interp4 apps_std_fread_def;
extern struct hrpc_method_def_interp4 apps_std_fseek_def;
extern struct hrpc_method_def_interp4 apps_std_fopen_with_env_def;
extern struct hrpc_method_def_interp4 apps_std_opendir_def;
extern struct hrpc_method_def_interp4 apps_std_closedir_def;
extern struct hrpc_method_def_interp4 apps_std_readdir_def;
extern struct hrpc_method_def_interp4 apps_std_mkdir_def;
extern struct hrpc_method_def_interp4 apps_std_stat_def;

#endif /* INTERFACE_APPS_STD_H */

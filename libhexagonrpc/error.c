/*
 * Imported FastRPC error messages
 *
 * Copyright (c) 2019, The Linux Foundation. All rights reserved.
 * Copyright (c) 2023, The Sensor Shell Contributors. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *    * Neither the name of The Linux Foundation nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <errno.h>
#include <string.h>

static const char *err_tab[] = {
	"No error",
	"General failure",
	"Insufficient RAM",
	"Specified class unsupported",
	"Version not supported",
	"Object already loaded",
	"Unable to load object/applet",
	"Unable to unload object/applet",
	"Alarm is pending",
	"Invalid time",
	"NULL class object",
	"Invalid metric specified",
	"App/Component Expired",
	"Invalid state",
	"Invalid parameter",
	"Invalid URL scheme",
	"Invalid item",
	"Invalid format",
	"Incomplete item",
	"Insufficient flash",
	"API is not supported",
	"Privileges are insufficient for this operation",
	"Unable to find specified resource",
	"Non re-entrant API re-entered",
	"API called in wrong task context",
	"App/Module left memory allocated when released.",
	"Operation is already in progress",
	"ADS mutual authorization failed",
	"Need service programming",
	"bad memory pointer",
	"heap corruption",
	"Context (system, interface, etc.) is idle",
	"Context (system, interface, etc.) is busy",
	"Invalid subscriber ID",
	"No type detected/found",
	"Need more data/info",
	"ADS Capabilities do not match those required for phone",
	"App failed to close properly",
	"Destination buffer given is too small",
	"No such name, port, socket or service exists or is valid",
	"ACK pending on application",
	"Not an owner authorized to perform the operation",
	"Current item is invalid",
	"Not allowed to perform the operation",
	"Invalid handle",
	"Out of handles",
	"Waitable call is interrupted",
	"No more items available -- reached end",
	"A CPU exception occurred",
	"Cannot change read-only object or parameter",
};

const char *hexagonrpc_strerror(int ret)
{
	if (ret == -1)
		return strerror(errno);
	else if (ret >= 0 && (unsigned int) ret < sizeof(err_tab) / sizeof(*err_tab))
		return err_tab[ret];

	return "Unknown error";
}

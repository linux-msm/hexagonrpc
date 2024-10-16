/*
 * Imported FastRPC error numbers
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

#ifndef LIBHEXAGONRPC_ERROR_H
#define LIBHEXAGONRPC_ERROR_H

#define AEE_SUCCESS		0
#define AEE_EFAILED		1
#define AEE_ENOMEMORY		2
#define AEE_ECLASSNOTSUPPORT	3
#define AEE_EVERSIONNOTSUPPORT	4
#define AEE_EALREADYLOADED	5
#define AEE_EUNABLETOLOAD	6
#define AEE_EUNABLETOUNLOAD	7
#define AEE_EALARMPENDING	8
#define AEE_EINVALIDTIME	9
#define AEE_EBADCLASS		10
#define AEE_EBADMETRIC		11
#define AEE_EEXPIRED		12
#define AEE_EBADSTATE		13
#define AEE_EBADPARM		14
#define AEE_ESCHEMENOTSUPPORTED	15
#define AEE_EBADITEM		16
#define AEE_EINVALIDFORMAT	17
#define AEE_EINCOMPLETEITEM	18
#define AEE_ENOPERSISTMEMORY	19
#define AEE_EUNSUPPORTED	20
#define AEE_EPRIVLEVEL		21
#define AEE_ERESOURCENOTFOUND	22
#define AEE_EREENTERED		23
#define AEE_EBADTASK		24
#define AEE_EALLOCATED		25
#define AEE_EALREADY		26
#define AEE_EADSAUTHBAD		27
#define AEE_ENEEDSERVICEPROG	28
#define AEE_EMEMPTR		29
#define AEE_EHEAP		30
#define AEE_EIDLE		31
#define AEE_EITEMBUSY		32
#define AEE_EBADSID		33
#define AEE_ENOTYPE		34
#define AEE_ENEEDMORE		35
#define AEE_EADSCAPS		36
#define AEE_EBADSHUTDOWN	37
#define AEE_EBUFFERTOOSMALL	38
#define AEE_ENOSUCH		39
#define AEE_EACKPENDING		40
#define AEE_ENOTOWNER		41
#define AEE_EINVALIDITEM	42
#define AEE_ENOTALLOWED		43
#define AEE_EBADHANDLE		44
#define AEE_EOUTOFHANDLES	45
#define AEE_EINTERRUPTED	46
#define AEE_ENOMORE		47
#define AEE_ECPUEXCEPTION	48
#define AEE_EREADONLY		49

const char *hexagonrpc_strerror(int ret);

#endif /* LIBHEXAGONRPC_ERROR_H */

/*
 * Copyright (c) 2003-2004 Apple Computer, Inc. All Rights Reserved.
 * 
 * @APPLE_LICENSE_HEADER_START@
 * 
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this
 * file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 * 
 * @APPLE_LICENSE_HEADER_END@
 */


/*
 *  SecBasePriv.h
 */
#ifndef _SECURITY_SECBASEPRIV_H_
#define _SECURITY_SECBASEPRIV_H_

#include <CoreFoundation/CFBase.h>
#include <Security/cssmtype.h>

#if defined(__cplusplus)
extern "C" {
#endif

const char *cssmErrorString(CSSM_RETURN error);

CFStringRef SecCopyErrorMessageString(OSStatus status, void *reserved);

#if defined(__cplusplus)
}
#endif

#endif /* !_SECURITY_SECBASEPRIV_H_ */


/*========================================================================*
 *                                                                        *
 *  Distributed by Whiteley Research Inc., Sunnyvale, California, USA     *
 *                       http://wrcad.com                                 *
 *  Copyright (C) 2017 Whiteley Research Inc., all rights reserved.       *
 *  Author: Stephen R. Whiteley, except as indicated.                     *
 *                                                                        *
 *  As fully as possible recognizing licensing terms and conditions       *
 *  imposed by earlier work from which this work was derived, if any,     *
 *  this work is released under the Apache License, Version 2.0 (the      *
 *  "License").  You may not use this file except in compliance with      *
 *  the License, and compliance with inherited licenses which are         *
 *  specified in a sub-header below this one if applicable.  A copy       *
 *  of the License is provided with this distribution, or you may         *
 *  obtain a copy of the License at                                       *
 *                                                                        *
 *        http://www.apache.org/licenses/LICENSE-2.0                      *
 *                                                                        *
 *  See the License for the specific language governing permissions       *
 *  and limitations under the License.                                    *
 *                                                                        *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,      *
 *   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES      *
 *   OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-        *
 *   INFRINGEMENT.  IN NO EVENT SHALL WHITELEY RESEARCH INCORPORATED      *
 *   OR STEPHEN R. WHITELEY BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER     *
 *   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,      *
 *   ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE       *
 *   USE OR OTHER DEALINGS IN THE SOFTWARE.                               *
 *                                                                        *
 *========================================================================*
 *                                                                        *
 * Memory Allocator Package                                               *
 *                                                                        *
 *========================================================================*
 $Id: malloc.c,v 1.28 2013/12/07 02:16:32 stevew Exp $
 *========================================================================*/

#ifdef __linux
#define _GNU_SOURCE
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <unistd.h>
#ifdef __linux
/*
This will force use of the mallinfo struct defined in the system malloc.h.
*/
#include <malloc.h>
#define HAVE_USR_INCLUDE_MALLOC_H
#endif


/*
// Exports that we use.
extern void *dlmalloc(size_t);
extern void *dlvalloc(size_t);
extern void *dlcalloc(size_t, size_t);
extern void *dlrealloc(void*, size_t);
extern void *dlmemalign(size_t, size_t);
extern void dlfree(void*);
extern struct mallinfo dlmallinfo();
*/

/*
//  The malloc included below is used by default in Xic, since it has
//  the advantage of being able to access most if not all available
//  system memory.  Typical malloc's get memory only from sbrk, which
//  gives up when it hits the address where shared libraries are
//  loaded (0x40000000 in Linux).  The mmap call can be used to map
//  another Gb or two of memory, if available.  This malloc uses mmap
//  after sbrk is exhausted.
*/

/* Add a "dl" prefix ahead of the symbol names. */
#define USE_DL_PREFIX

#define PROCEED_ON_ERROR 1
#define SRW_HACKS

#if defined(__FreeBSD__) || defined(__linux)
#include MALLOCFILE
#else
// This isn't used, avoids linker warnings about no symbols.
int NO_LOCAL_MALLOC = 1;
#endif



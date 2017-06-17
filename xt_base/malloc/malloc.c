
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2010 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *                                                                        *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,      *
 *   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES      *
 *   OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-        *
 *   INFRINGEMENT.  IN NO EVENT SHALL STEPHEN R. WHITELEY BE LIABLE       *
 *   FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION      *
 *   OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN           *
 *   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN         *
 *   THE SOFTWARE.                                                        *
 *                                                                        *
 * This library is available for non-commercial use under the terms of    *
 * the GNU Library General Public License as published by the Free        *
 * Software Foundation; either version 2 of the License, or (at your      *
 * option) any later version.                                             *
 *                                                                        *
 * A commercial license is available to those wishing to use this         *
 * library or a derivative work for commercial purposes.  Contact         *
 * Whiteley Research Inc., 456 Flora Vista Ave., Sunnyvale, CA 94086.     *
 * This provision will be enforced to the fullest extent possible under   *
 * law.                                                                   *
 *                                                                        *
 * This library is distributed in the hope that it will be useful,        *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU      *
 * Library General Public License for more details.                       *
 *                                                                        *
 * You should have received a copy of the GNU Library General Public      *
 * License along with this library; if not, write to the Free             *
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.     *
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



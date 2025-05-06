
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
 *               XicTools Integrated Circuit Design System                *
 *                                                                        *
 * Memory Allocator Package                                               *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>

#if defined(__arm64) || defined(__x86_64)
#include <execinfo.h>
#endif

#ifdef __linux
#ifndef __USE_GNU
#define __USE_GNU
#endif
#include <malloc.h>
#define HAVE_USR_INCLUDE_MALLOC_H
#endif

#include <dlfcn.h>
#include "local_malloc.h"

// Uncomment to enable allocation monitoring.  This now has very low
// overhead when not active so we keep it available by default.
#define ENABLE_MONITOR

// Uncomment for recursion testing, not good if multi-threads.
// #define MEM_CHECK_RECURSE

#ifndef __APPLE__
extern "C" {
#ifndef HAVE_USR_INCLUDE_MALLOC_H
#ifndef _MALLOC_H
    struct mallinfo {
        size_t arena;    // non-mmapped space allocated from system
        size_t ordblks;  // number of free chunks
        size_t smblks;   // always 0
        size_t hblks;    // always 0
        size_t hblkhd;   // space in mmapped regions
        size_t usmblks;  // maximum total allocated space
        size_t fsmblks;  // always 0
        size_t uordblks; // total allocated space
        size_t fordblks; // total free space
        size_t keepcost; // releasable (via malloc_trim) space
    };
#endif
#endif

    // malloc.c
    extern void *dlmalloc(size_t);
    extern void *dlvalloc(size_t);
    extern void *dlcalloc(size_t, size_t);
    extern void *dlrealloc(void*, size_t);
    extern void *dlmemalign(size_t, size_t);
    extern int dlposix_memalign(void**, size_t, size_t);
    extern void dlfree(void*);
    extern struct mallinfo dlmallinfo();
}
#endif


// Instantiated here only, never in application.
namespace { sMemory _memory_; }

mem_logfunc sMemory::mem_logfunc_ptr = 0;
sMemory *sMemory::mem_ptr = &_memory_;

// Report memory in use, bytes.
//
size_t
sMemory::in_use()
{
#ifdef __APPLE__
    malloc_statistics_t st;
    malloc_zone_statistics(0, &st);
    return (st.size_in_use);
#else
    if (mem_use_local_malloc) {
        struct mallinfo m = dlmallinfo();
        if (m.uordblks)
            return ((unsigned int)m.uordblks);
        return ((unsigned int)m.arena + (unsigned int)m.hblkhd);
    }
    else {
#ifdef __linux
        struct mallinfo m = mallinfo();
        if (m.uordblks)
            return ((unsigned int)m.uordblks);
        return ((unsigned int)m.arena + (unsigned int)m.hblkhd);
#endif
    }
#endif
    return (0);
}


void
sMemory::mem_init()
{
    if (mem_state != m_noinit)
        return;
    mem_state = m_pending;

#ifdef __APPLE__
    // Apple doesn't provide a means to replace the malloc functions,
    // but we can hook them.

    malloc_zone_t *mz = malloc_default_zone();
    mem_malloc_ptr =  mz->malloc;
    mem_valloc_ptr =  mz->valloc;
    mem_calloc_ptr =  mz->calloc;
    mem_realloc_ptr =  mz->realloc;
#ifdef HAVE_POSIX_MEMALIGN
    mem_memalign_ptr =  mz->memalign;
    mem_free_definite_size_ptr = mz->free_definite_size;
#endif
    mem_free_ptr = mz->free;

    mz->malloc = apple_foo::mz_malloc;
    mz->valloc = apple_foo::mz_valloc;
    mz->calloc = apple_foo::mz_calloc;
    mz->realloc = apple_foo::mz_realloc;
#ifdef HAVE_POSIX_MEMALIGN
    mz->memalign = apple_foo::mz_memalign;
    mz->free_definite_size = apple_foo::mz_free_definite_size;
#endif
    mz->free = apple_foo::mz_free;

#else

#if defined(__FreeBSD__) || defined(__linux)
    mem_use_local_malloc = (getenv("XT_LOCAL_MALLOC") != 0);
#endif
    if (mem_use_local_malloc) {
        // Use our private malloc and friends.  This may facilitate
        // debugging, but may also cause serious problems.  For
        // example, it is completely unstable in openSuSE-13.1.  Not
        // sure if it is due to multi-threading or an allocation
        // function that is not mapped.

        // Recent gtk2 releases use a "g_slice" memory manager which
        // seg faults when our internal memory manager is used.  This
        // seems to fix the problem in some cases.
        static char glibfix[] = "G_SLICE=always-malloc";
        putenv(glibfix);

        mem_malloc_ptr = dlmalloc;
        mem_valloc_ptr = dlvalloc;
        mem_calloc_ptr = dlcalloc;
        mem_realloc_ptr = dlrealloc;
        mem_memalign_ptr = dlmemalign;
        mem_posix_memalign_ptr = dlposix_memalign;
        mem_free_ptr = dlfree;
    }
    else {
        // Recent glibc dlsym calls calloc, but it seems ok if calloc
        // returns 0 in this case.  It is beyond stupid to use memory
        // allocation functions in dlsym.  When mem_state == m_pending,
        // the allocation functions will return 0.

        mem_calloc_ptr = (void*(*)(size_t, size_t))dlsym(RTLD_NEXT, "calloc");
        mem_malloc_ptr = (void*(*)(size_t))dlsym(RTLD_NEXT, "malloc");
        mem_valloc_ptr = (void*(*)(size_t))dlsym(RTLD_NEXT, "valloc");
        mem_realloc_ptr = (void*(*)(void*, size_t))dlsym(RTLD_NEXT, "realloc");
        mem_memalign_ptr = (void*(*)(size_t, size_t))dlsym(RTLD_NEXT,
            "memalign");
        mem_posix_memalign_ptr = (int(*)(void**, size_t, size_t))dlsym(
            RTLD_NEXT, "posix_memalign");
        mem_free_ptr = (void(*)(void*))dlsym(RTLD_NEXT, "free");
    }
#endif
    mem_state = m_running;

    if (!mem_calloc_ptr || !mem_malloc_ptr || !mem_valloc_ptr || 
            !mem_realloc_ptr ||
#ifndef __FreeBSD__
            !mem_memalign_ptr ||
#endif
#ifndef __APPLE__
            !mem_posix_memalign_ptr ||
#endif
            !mem_free_ptr) {
        fprintf(stderr, "Error: unresolved allocation pointer, fatal.\n");
        exit(1);
    }

#ifdef ENABLE_MONITOR
    {
        // If the environment variable is set, start monitor.
        const char *s = getenv("MMON_STARTUP");
        if (s) {
            int d = 1;
            if (isdigit(*s))
                d = atoi(s);
            if (d < 1 || d > 15)
                d = 1;
            mon_start(d);
            mem_mon_check_free = true;
        }
    }
#endif
}


// Called from the allocation code on error.  If fatal, then the
// allocation call is failing (returning 0) which will terminate the
// application.  In this case, clear the memory_busy flag so we can
// exit cleanly.  Otherwise, the flag will be cleared by the caller.
//
// This generates a stack backtrace and calls the user's error handler.
//
void
sMemory::mem_error(const char *string, void *chunk, int fatal)
{
    if (!mem_logfunc_ptr)
        return;

    unsigned int tmp = mem_busy;
    mem_busy = 0;
    memset(mem_stk, 0, MEM_ERR_DEPTH*sizeof(long));

#if defined(__arm64) || defined(__x86_64)
    void *vtmp[MEM_ERR_DEPTH+1];
    // The backtrace function can call malloc, mem_busy must be zeroed.
    int i = backtrace(vtmp, MEM_ERR_DEPTH+1) - 1;
    if (i > 0 && i <= MEM_ERR_DEPTH)
        memcpy(mem_stk, vtmp + 1, i*sizeof(long));

#else
    // Stack walk, gcc-specific.
    int i = 0;
    switch (1) {
    default:
        if (!__builtin_frame_address(1)) break;
        mem_stk[i++] = (long)__builtin_return_address(1);
        if (i >= MEM_ERR_DEPTH || !mem_stk[i-1]) break;
        if (!__builtin_frame_address(2)) break;
        mem_stk[i++] = (long)__builtin_return_address(2);
        if (i >= MEM_ERR_DEPTH || !mem_stk[i-1]) break;
        if (!__builtin_frame_address(3)) break;
        mem_stk[i++] = (long)__builtin_return_address(3);
        if (i >= MEM_ERR_DEPTH || !mem_stk[i-1]) break;
        if (!__builtin_frame_address(4)) break;
        mem_stk[i++] = (long)__builtin_return_address(4);
        if (i >= MEM_ERR_DEPTH || !mem_stk[i-1]) break;
        if (!__builtin_frame_address(5)) break;
        mem_stk[i++] = (long)__builtin_return_address(5);
        if (i >= MEM_ERR_DEPTH || !mem_stk[i-1]) break;
        if (!__builtin_frame_address(6)) break;
        mem_stk[i++] = (long)__builtin_return_address(6);
        if (i >= MEM_ERR_DEPTH || !mem_stk[i-1]) break;
        if (!__builtin_frame_address(7)) break;
        mem_stk[i++] = (long)__builtin_return_address(7);
        if (i >= MEM_ERR_DEPTH || !mem_stk[i-1]) break;
        if (!__builtin_frame_address(8)) break;
        mem_stk[i++] = (long)__builtin_return_address(8);
        if (i >= MEM_ERR_DEPTH || !mem_stk[i-1]) break;
        if (!__builtin_frame_address(9)) break;
        mem_stk[i++] = (long)__builtin_return_address(9);
        if (i >= MEM_ERR_DEPTH || !mem_stk[i-1]) break;
        if (!__builtin_frame_address(10)) break;
        mem_stk[i++] = (long)__builtin_return_address(10);
        if (i >= MEM_ERR_DEPTH || !mem_stk[i-1]) break;
        if (!__builtin_frame_address(11)) break;
        mem_stk[i++] = (long)__builtin_return_address(11);
        if (i >= MEM_ERR_DEPTH || !mem_stk[i-1]) break;
        if (!__builtin_frame_address(12)) break;
        mem_stk[i++] = (long)__builtin_return_address(12);
        if (i >= MEM_ERR_DEPTH || !mem_stk[i-1]) break;
        if (!__builtin_frame_address(13)) break;
        mem_stk[i++] = (long)__builtin_return_address(13);
        if (i >= MEM_ERR_DEPTH || !mem_stk[i-1]) break;
        if (!__builtin_frame_address(14)) break;
        mem_stk[i++] = (long)__builtin_return_address(14);
        if (i >= MEM_ERR_DEPTH || !mem_stk[i-1]) break;
        if (!__builtin_frame_address(15)) break;
        mem_stk[i++] = (long)__builtin_return_address(15);
    }
#endif
    (*mem_logfunc_ptr)(string, chunk, mem_stk, i);
    mem_busy = fatal ? 0 : tmp;
}


inline bool
sMemory::mem_init_check()
{
    if (mem_state == m_noinit)
        mem_init();
    if (mem_state == m_pending)
        return (false);
    return (true);
}


inline bool
sMemory::mem_recur_check(const char *str, int flags)
{
    if (mem_busy & flags) {
        Memory()->mem_error(str, 0, 1);
        return (false);
    }
    return (true);
}


#ifdef __APPLE__
void *
sMemory::mem_malloc(malloc_zone_t *mz, size_t size)
#else
inline void *
sMemory::mem_malloc(size_t size)
#endif
{
    if (!mem_init_check())
        return (0);
#ifdef MEM_CHECK_RECURSE
    if (!mem_recur_check("recursion error in malloc",
            MEM_IN_MALLOC | MEM_IN_FREE))
        return (0);
    mem_busy |= MEM_IN_MALLOC;
#endif
#ifdef __APPLE__
    void *v = (*mem_malloc_ptr)(mz, size);
#else
    void *v = (*mem_malloc_ptr)(size);
#endif
#ifdef MEM_CHECK_RECURSE
    mem_busy &= ~MEM_IN_MALLOC;
#endif

    if (!v || mem_fault_test) {
        mem_error("out of memory", 0, 1);
        return (0);
    }
#ifdef ENABLE_MONITOR
    mem_mon_alloc_hook(v);
#endif
    return (v);
}


#ifdef __APPLE__
void *
sMemory::mem_valloc(malloc_zone_t *mz, size_t size)
#else
inline void *
sMemory::mem_valloc(size_t size)
#endif
{
    if (!mem_init_check())
        return (0);
#ifdef MEM_CHECK_RECURSE
    if (!mem_recur_check("recursion error in valloc",
            MEM_IN_VALLOC | MEM_IN_FREE))
        return (0);
    mem_busy |= MEM_IN_VALLOC;
#endif
#ifdef __APPLE__
    void *v = (*mem_valloc_ptr)(mz, size);
#else
    void *v = (*mem_valloc_ptr)(size);
#endif
#ifdef MEM_CHECK_RECURSE
    mem_busy &= ~MEM_IN_VALLOC;
#endif

    if (!v) {
        mem_error("out of memory", 0, 1);
        return (0);
    }
#ifdef ENABLE_MONITOR
    mem_mon_alloc_hook(v);
#endif
    return (v);
}


#ifdef __APPLE__
void *
sMemory::mem_calloc(malloc_zone_t *mz, size_t n, size_t size)
#else
inline void *
sMemory::mem_calloc(size_t n, size_t size)
#endif
{
    if (!mem_init_check())
        return (0);
#ifdef MEM_CHECK_RECURSE
    if (!mem_recur_check("recursion error in calloc",
            MEM_IN_CALLOC | MEM_IN_FREE))
        return (0);
    mem_busy |= MEM_IN_CALLOC;
#endif
#ifdef __APPLE__
    void *v = (*mem_calloc_ptr)(mz, n, size);
#else
    void *v = (*mem_calloc_ptr)(n, size);
#endif
#ifdef MEM_CHECK_RECURSE
    mem_busy &= ~MEM_IN_CALLOC;
#endif

    if (!v) {
        mem_error("out of memory", 0, 1);
        return (0);
    }
#ifdef ENABLE_MONITOR
    mem_mon_alloc_hook(v);
#endif
    return (v);
}


#ifdef __APPLE__
void *
sMemory::mem_realloc(malloc_zone_t *mz, void *p, size_t size)
#else
inline void *
sMemory::mem_realloc(void *p, size_t size)
#endif
{
    if (!mem_init_check())
        return (0);
#ifdef MEM_CHECK_RECURSE
    if (!mem_recur_check("recursion error in realloc", -1))
        return (0);
    mem_busy |= MEM_IN_REALLOC;
#endif
#ifdef __APPLE__
    void *v = (*mem_realloc_ptr)(mz, p, size);
#else
    void *v = (*mem_realloc_ptr)(p, size);
#endif
#ifdef MEM_CHECK_RECURSE
    mem_busy &= ~MEM_IN_REALLOC;
#endif

    if (!v) {
        mem_error("out of memory", 0, 1);
        return (0);
    }
#ifdef ENABLE_MONITOR
    if (p != v) {
        mem_mon_free_hook(p);
        mem_mon_alloc_hook(v);
    }
#endif
    return (v);
}


#ifdef __APPLE__

#ifdef HAVE_POSIX_MEMALIGN
void *
sMemory::mem_memalign(malloc_zone_t *mz, size_t align, size_t size)
{
    if (!mem_init_check())
        return (0);
#ifdef MEM_CHECK_RECURSE
    if (!mem_recur_check("recursion error in memalign",
            MEM_IN_MEMALIGN | MEM_IN_FREE))
        return (0);
    mem_busy |= MEM_IN_MEMALIGN;
#endif
    void *v = (*mem_memalign_ptr)(mz, align, size);
#ifdef MEM_CHECK_RECURSE
    mem_busy &= ~MEM_IN_MEMALIGN;
#endif

    if (!v) {
        mem_error("out of memory", 0, 1);
        return (0);
    }
#ifdef ENABLE_MONITOR
    mem_mon_alloc_hook(v);
#endif
    return (v);
}


void
sMemory::mem_free_definite_size(malloc_zone_t *mz, void *p, size_t)
{
    mem_free(mz, p);
}
#endif

#else

inline void *
sMemory::mem_memalign(size_t align, size_t size)
{
    if (!mem_init_check())
        return (0);
#ifdef MEM_CHECK_RECURSE
    if (!mem_recur_check("recursion error in memalign",
            MEM_IN_MEMALIGN | MEM_IN_FREE))
        return (0);
    mem_busy |= MEM_IN_MEMALIGN;
#endif
    void *v = (*mem_memalign_ptr)(align, size);
#ifdef MEM_CHECK_RECURSE
    mem_busy &= ~MEM_IN_MEMALIGN;
#endif

    if (!v) {
        mem_error("out of memory", 0, 1);
        return (0);
    }
#ifdef ENABLE_MONITOR
    mem_mon_alloc_hook(v);
#endif
    return (v);
}


int
sMemory::mem_posix_memalign(void **p, size_t align, size_t size)
{
    if (!mem_init_check())
        return (0);
#ifdef MEM_CHECK_RECURSE
    if (!mem_recur_check("recursion error in posix_memalign",
            MEM_IN_POSIX_MEMALIGN | MEM_IN_FREE))
        return (EINVAL);
    mem_busy |= MEM_IN_POSIX_MEMALIGN;
#endif
    int ret = (*mem_posix_memalign_ptr)(p, align, size);
#ifdef MEM_CHECK_RECURSE
    mem_busy &= ~MEM_IN_POSIX_MEMALIGN;
#endif

    if (ret) {
        if (ret == ENOMEM)
            mem_error("out of memory", 0, 1);
        else if (ret == EINVAL)
            mem_error("alignment error", 0, 1);
        else
            mem_error("unknown error", 0, 1);
        return (ret);
    }
#ifdef ENABLE_MONITOR
    mem_mon_alloc_hook(*p);
#endif
    return (0);
}
#endif


#ifdef __APPLE__
void
sMemory::mem_free(malloc_zone_t *mz, void *p)
#else
inline void
sMemory::mem_free(void *p)
#endif
{
    if (mem_state == m_pending)
        return;

#ifdef ENABLE_MONITOR
    mem_mon_free_hook(p);
#endif

#ifdef MEM_CHECK_RECURSE
    if (mem_busy & MEM_FREE_MASK) {
        mem_error("recursion error in free", 0, 0);
        return;
    }
    mem_busy |= MEM_IN_FREE;
#endif
#ifdef __APPLE__
    (*mem_free_ptr)(mz, p);
#else
    (*mem_free_ptr)(p);
#endif
#ifdef MEM_CHECK_RECURSE
    mem_busy &= ~MEM_IN_FREE;
#endif

    if (mem_free_cb) {
        mem_free_cnt++;
        if (!(mem_free_cnt % MEM_FREE_CB_INCR))
            (*mem_free_cb)(mem_free_cnt);
    }
}


#ifdef __APPLE__
namespace apple_foo {
    void *
    mz_malloc(malloc_zone_t *mz, size_t size)
    {
        return (Memory()->mem_malloc(mz, size));
    }

    void *
    mz_valloc(malloc_zone_t *mz, size_t size)
    {
        return (Memory()->mem_valloc(mz, size));
    }

    void *
    mz_calloc(malloc_zone_t *mz, size_t n, size_t size)
    {
        return (Memory()->mem_calloc(mz, n, size));
    }

    void *
    mz_realloc(malloc_zone_t *mz, void *p, size_t size)
    {
        return (Memory()->mem_realloc(mz, p, size));
    }

#ifdef HAVE_POSIX_MEMALIGN
    void *
    mz_memalign(malloc_zone_t *mz, size_t align, size_t size)
    {
        return (Memory()->mem_memalign(mz, align, size));
    }

    void
    mz_free_definite_size(malloc_zone_t *mz, void *p, size_t size)
    {
        Memory()->mem_free_definite_size(mz, p, size);
    }
#endif

    void
    mz_free(malloc_zone_t *mz, void *p)
    {
        Memory()->mem_free(mz, p);
    }
}


#else
extern "C" {

void *
malloc(size_t size)
{
    return (Memory()->mem_malloc(size));
}

void *
valloc(size_t size)
{
    return (Memory()->mem_valloc(size));
}

void *
calloc(size_t n, size_t size)
{
    return (Memory()->mem_calloc(n, size));
}

void *
realloc(void *p, size_t size)
{
    return (Memory()->mem_realloc(p, size));
}

#ifndef __FreeBSD__
void *
memalign(size_t align, size_t size)
{
    return (Memory()->mem_memalign(align, size));
}
#endif

int
posix_memalign(void **p, size_t align, size_t size)
{
    return (Memory()->mem_posix_memalign(p, align, size));
}

void
free(void *p)
{
    Memory()->mem_free(p);
}

} // extern "C"
#endif

extern "C" {

// Export to malloc.c.
void
malloc_error(const char *msg, void *p)
{
    Memory()->mem_error(msg, p, 0);
}

} // extern "C"


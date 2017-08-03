
/*========================================================================*
 *                                                                        *
 *  Whiteley Research Inc, Sunnyvale, CA USA, http://wrcad.com            *
 *  Copyright (C) 2017 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *  Licensed under the Apache License, Version 2.0 (the "License");       *
 *  you may not use this file except in compliance with the License.      *
 *  You may obtain a copy of the License at                               *
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

#ifndef LOCAL_MALLOC_H
#define LOCAL_MALLOC_H

#ifdef __APPLE__
#include <malloc/malloc.h>
#include "config.h"
#endif


//
// A class for control and augmentation of the malloc/free sunctions.
// This provides hooks for statistics and monitoring, plus a complete
// malloc-family replacement when needed.
//
// It is not always possible to completely override the system malloc
// functions.  Generally, this is possible under Unix/Linux only. 
// Under OS X, it is possible to hook into but not completely override
// the system functions, since malloc called in a dylib will not be
// overridden by malloc provided in the application.  We can't replace
// malloc, but we can intercept the calls in the default zone.
//

#define MEM_IN_MALLOC           0x1
#define MEM_IN_CALLOC           0x2
#define MEM_IN_VALLOC           0x4
#define MEM_IN_REALLOC          0x8
#define MEM_IN_MEMALIGN         0x10
#define MEM_IN_POSIX_MEMALIGN   0x20
#define MEM_IN_FREE             0x40
#define MEM_FREE_MASK           0x77

#define MEM_ERR_DEPTH 10
#define MEM_FREE_CB_INCR 10000

extern "C" void malloc_error(const char*, void*);

typedef void(*mem_logfunc)(const char*, void*, long*, int);

inline struct sMemory *Memory();

#ifdef __APPLE__
namespace apple_foo {
    void *mz_malloc(malloc_zone_t*, size_t);
    void *mz_valloc(malloc_zone_t*, size_t);
    void *mz_calloc(malloc_zone_t*, size_t, size_t);
    void *mz_realloc(malloc_zone_t*, void*, size_t);
#ifdef HAVE_POSIX_MEMALIGN
    void *mz_memalign(malloc_zone_t*, size_t, size_t);
    void mz_free_definite_size(malloc_zone_t*, void*, size_t);
#endif
    void mz_free(malloc_zone_t*, void*);
}
#endif

struct sMemory
{
    enum m_state { m_noinit, m_pending, m_running };
    friend sMemory *Memory() { return (mem_ptr); }
    friend void malloc_error(const char*, void*);

    // No constructor, instantiated global static.

    size_t in_use();

    bool is_busy()                              { return (mem_busy); }
    void force_not_busy()                       { mem_busy = 0; }
    void set_memory_fault_test(bool b)          { mem_fault_test = b; }

    // Might be called before constructor.
    static void register_error_log(mem_logfunc logfunc)
                                             { mem_logfunc_ptr = logfunc; }

    // Freeing a large number of objects can take a while,
    // particularly if page swapping is going on.  Below is a hack to
    // get some user feedback.  The callback is called periodically
    // with the number of objects freed.

    void register_free_talk(void(*cb)(size_t))
        {
            if (cb)
                mem_free_cnt = 0;
            mem_free_cb = cb;
        }

    // Memory monitor functions, in monitor.cc.
    bool mon_start(int);
    bool mon_stop();
    int mon_count();
    bool mon_dump(const char*);

    int mon_depth()                             { return (mem_mon_depth); }

private:
    void mem_init();
    void mem_error(const char*, void*, int);

    inline bool mem_init_check();
    inline bool mem_recur_check(const char*, int);

public:
#ifdef __APPLE__
    void *mem_malloc(malloc_zone_t*, size_t);
    void *mem_valloc(malloc_zone_t*, size_t);
    void *mem_calloc(malloc_zone_t*, size_t, size_t);
    void *mem_realloc(malloc_zone_t*, void*, size_t);
#ifdef HAVE_POSIX_MEMALIGN
    void *mem_memalign(malloc_zone_t*, size_t, size_t);
    void mem_free_definite_size(malloc_zone_t*, void*, size_t);
#endif
    void mem_free(malloc_zone_t*, void*);
#else
    inline void *mem_malloc(size_t);
    inline void *mem_valloc(size_t);
    inline void *mem_calloc(size_t, size_t);
    inline void *mem_realloc(void*, size_t);
    inline void *mem_memalign(size_t, size_t);
    inline int mem_posix_memalign(void**, size_t, size_t);
    inline void mem_free(void*);
#endif

private:
    void mem_mon_alloc_hook(void *v)
        {
            if (mem_mon_on && v)
                mem_mon_alloc_hook_prv(v);
        }

    void mem_mon_free_hook(void *v)
        {
            if (mem_mon_on && v)
                mem_mon_free_hook_prv(v);
        }

    // Memory monitor functions, in monitor.cc.
    void mem_mon_enable(bool);
    void mem_mon_alloc_hook_prv(void*);
    void mem_mon_free_hook_prv(void*);

    m_state mem_state;
    unsigned int mem_busy;

#ifdef __APPLE__
    void *(*mem_malloc_ptr)(malloc_zone_t*, size_t);
    void *(*mem_valloc_ptr)(malloc_zone_t*, size_t);
    void *(*mem_calloc_ptr)(malloc_zone_t*, size_t, size_t);
    void *(*mem_realloc_ptr)(malloc_zone_t*, void*, size_t);
#ifdef HAVE_POSIX_MEMALIGN
    void *(*mem_memalign_ptr)(malloc_zone_t*, size_t, size_t);
    void (*mem_free_definite_size_ptr)(malloc_zone_t*, void*, size_t);
#endif
    void (*mem_free_ptr)(malloc_zone_t*, void*);
#else
    void *(*mem_malloc_ptr)(size_t);
    void *(*mem_valloc_ptr)(size_t);
    void *(*mem_calloc_ptr)(size_t, size_t);
    void *(*mem_realloc_ptr)(void*, size_t);
    void *(*mem_memalign_ptr)(size_t, size_t);
    int (*mem_posix_memalign_ptr)(void**, size_t, size_t);
    void (*mem_free_ptr)(void*);
#endif
    void (*mem_free_cb)(size_t);
    size_t mem_free_cnt;

    int mem_mon_depth;
    bool mem_fault_test;
    bool mem_mon_on;
    bool mem_mon_check_free;
    bool mem_use_local_malloc;
    struct mtable_t *mem_mon_tab;

    long mem_stk[MEM_ERR_DEPTH];

    static mem_logfunc mem_logfunc_ptr;
    static sMemory *mem_ptr;
};

#endif


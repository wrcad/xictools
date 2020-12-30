
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
 * Misc. Utilities Library                                                *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#include "threadpool.h"
#include <stdio.h>
#include <stdint.h>


//
// A general-purpose thread pool class implemented using pthreads.
//

// Define for original termination code, new code may be slightly
// faster.
// #define JOIN_WHEN_DONE

// #define DEBUG

cThreadPool::cThreadPool(int numthreads)
{
    tp_nthreads = numthreads;
    if (tp_nthreads < 1)
        tp_nthreads = 1;
    else if (tp_nthreads > 31)
        tp_nthreads = 31;
    tp_error = 0;
    tp_state = new sTPstate[tp_nthreads];
    tp_list = 0;
    tp_list_bak = 0;
    tp_list_end = 0;
    pthread_mutex_init(&tp_mtx, 0);
    pthread_cond_init(&tp_cnd, 0);
#ifdef JOIN_WHEN_DONE
    for (unsigned int i = 0; i < tp_nthreads; i++) {
        tp_state[i].s_tp = this;
        tp_state[i].s_tnum = i;
        int err = pthread_create(&tp_state[i].s_thr, 0, tp_thread_proc,
            &tp_state[i]);
        if (err) {
            fprintf(stderr, "pthread creation error %d!", err);
            tp_nthreads = i;
            break;
        }
    }
#else
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    for (unsigned int i = 0; i < tp_nthreads; i++) {
        tp_state[i].s_tp = this;
        tp_state[i].s_tnum = i;
        int err = pthread_create(&tp_state[i].s_thr, &attr, tp_thread_proc,
            &tp_state[i]);
        if (err) {
            fprintf(stderr, "pthread creation error %d!", err);
            tp_nthreads = i;
            break;
        }
    }
    pthread_attr_destroy(&attr);
#endif
}


cThreadPool::~cThreadPool()
{
    sTPjobList::destroy(tp_list_bak);
    for (unsigned int i = 0; i < tp_nthreads; i++) {
        tp_state[i].s_die = true;
        tp_state[i].s_ready = true;
    }
    pthread_cond_broadcast(&tp_cnd);
#ifdef JOIN_WHEN_DONE
    for (unsigned int i = 0; i < tp_nthreads; i++) {
        pthread_join(tp_state[i].s_thr, 0);
    }
#else
    // Make sure that we don't destroy the mutex, etc.  until the
    // threads are done.  Only Windows seems to need this.

again:
    for (unsigned int i = 0; i < tp_nthreads; i++) {
        volatile bool b = tp_state[i].s_ready;
        if (b)
            goto again;
    }
#endif
    pthread_mutex_destroy(&tp_mtx);
    pthread_cond_destroy(&tp_cnd);
    delete [] tp_state;
}


// Supply optional per-thread data, which will be passed as the first
// argument to the user's work procedure.
//
void
cThreadPool::setThreadData(sTPthreadData *data, unsigned int thread)
{
    if (thread >= tp_nthreads)
        return;
    tp_state[thread].s_data = data;
}


// Submit a job, to be assigned to a thread from the pool.  The first
// argument is the user's work procedure.  The second argument is user
// data to be passed to the work procedure.  The third argument is a
// destructor for the user data, which is called when the cThreadPool
// is destroyed or cleared.
//
void
cThreadPool::submit(TPthreadJob j, void *arg, TPdestroy destr)
{
    if (!tp_list_bak)
        tp_list_bak = tp_list_end = new sTPjobList(j, arg, destr);
    else {
        tp_list_end->jl_next = new sTPjobList(j, arg, destr);
        tp_list_end = tp_list_end->jl_next;
    }
}


// Run the queue.  The argument is user thread data, as passed to
// setThreadData, for the main thread.  The return value is nonzero on
// failure, the return from the user's work function.
//
int
cThreadPool::run(sTPthreadData *data)
{
    tp_list = tp_list_bak;
    tp_error = 0;
    for (unsigned int i = 0; i < tp_nthreads; i++) {
        tp_state[i].s_done = false;
        tp_state[i].s_ready = true;
    }
#ifdef DEBUG
    int cnt = 0;
    for (sTPjobList *j = tp_list; j; j = j->jl_next)
        cnt++;
    fprintf(stderr, "run %d\n", cnt);
#endif
    pthread_mutex_lock(&tp_mtx);
    pthread_cond_broadcast(&tp_cnd);
    pthread_mutex_unlock(&tp_mtx);

    for (;;) {
        sTPjobList *j = tp_list;
        if (!j)
            break;
        sTPjobList *n = j->jl_next;
        if (!__sync_bool_compare_and_swap((uintptr_t*)&tp_list,
                (uintptr_t)j, (uintptr_t)n))
            continue;
        if (!j->jl_job)
            continue;

        int error = (*j->jl_job)(data, j->jl_arg);
        if (error) {
            tp_error = error;
            goto again;
        }
    }

again:
    for (unsigned int i = 0; i < tp_nthreads; i++) {
        volatile bool b = tp_state[i].s_done;
        if (!b)
            goto again;
    }
    return (tp_error);
}


// Static function.
// The phtread work function.
//
void *
cThreadPool::tp_thread_proc(void *arg)
{
    sTPstate *st = (sTPstate*)arg;
    cThreadPool *tp = st->s_tp;
    for (;;) {
        pthread_mutex_lock(&tp->tp_mtx);
        while (!st->s_ready)
            pthread_cond_wait(&tp->tp_cnd, &tp->tp_mtx);
        pthread_mutex_unlock(&tp->tp_mtx);
        st->s_ready = false;
#ifdef DEBUG
        fprintf(stderr, "%d start\n", st->s_tnum);
#endif
        if (st->s_die)
            break;

        for (;;) {
            sTPjobList *j = tp->tp_list;
            if (!j)
                break;
            sTPjobList *n = j->jl_next;
            if (!__sync_bool_compare_and_swap((uintptr_t*)&tp->tp_list,
                    (uintptr_t)j, (uintptr_t)n))
                continue;
            if (!j->jl_job)
                continue;

            int error = (*j->jl_job)(st->s_data, j->jl_arg);
            if (error) {
                tp->tp_error = error;
                break;
            }
            if (tp->tp_error)
                break;
        }
#ifdef DEBUG
        fprintf(stderr, "%d done\n", st->s_tnum);
#endif
        st->s_done = true;
    }
    return (0);
}
// End of cThreadPool functions.


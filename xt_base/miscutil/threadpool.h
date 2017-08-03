
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
 * Misc. Utilities Library                                                *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <pthread.h>

//
// A generic thread pool implementation for pthreads.
//

// Base class for per-thread data container.  If the user needs to
// supply per-thread data, this should sub-classed.  The destructor
// will be called when the thread pool is destroyed.  A pointer to
// this, as well as a pointer to the user's per-job data will be
// passed to the user's work function.
//
struct sTPthreadData
{
    virtual ~sTPthreadData() { }
};


// The user's work procedure, user implements and submits, along with
// the pointer to pass as the second argument.  The function should
// return 0 on success, nonzero on error.  A nonzero return will halt
// all threads and the value will be returned from the run function.
//
typedef int(*TPthreadJob)(sTPthreadData*, void*);

// Function type of optional function to pass along with the
// submission, which will free the argument.  This will be invoked
// when the threadpool is destroyed or cleared, to free the user's
// data.
//
typedef void(*TPdestroy)(void*);


class cThreadPool
{
public:
    // The job queue.  The work is actually performed in the
    // user-supplied TPthreadJob.
    //
    struct sTPjobList
    {
        sTPjobList(TPthreadJob j, void *a, TPdestroy d)
            {
                jl_next = 0;
                jl_job = j;
                jl_arg = a;
                jl_destroy = d;
            }

        ~sTPjobList()
            {
                if (jl_destroy)
                    (*jl_destroy)(jl_arg);
            }

        static void destroy(const sTPjobList *j)
            {
                while (j) {
                    const sTPjobList *jx = j;
                    j = j->jl_next;
                    delete jx;
                }
            }

        sTPjobList  *jl_next;
        TPthreadJob jl_job;
        void        *jl_arg;
        TPdestroy   jl_destroy;
    };

    // Per-thread state variable container.
    //
    struct sTPstate
    {
        sTPstate()
            {
                s_tp = 0;
                s_data = 0;
                s_ready = false;
                s_done = false;
                s_die = false;
                s_tnum = 0;
            }

        ~sTPstate()
            {
                delete s_data;
            }

        cThreadPool     *s_tp;
        sTPthreadData   *s_data;
        pthread_t       s_thr;
        bool            s_ready;
        bool            s_done;
        bool            s_die;
        int             s_tnum;
    };

    cThreadPool(int);
    ~cThreadPool();

    // Usage:
    // Instantiate a cThreadPool.
    // Optionally call setThreadData to supply some data for each
    //  thread.
    // Call cThreadPool::submit to submit a number of jobs, each
    //  encapsulated as a TPthreadJob and arbitrary pointer argument.
    // Call cThreadPool::run, a zero return indicates success, nonzero
    //  as returned from a TPthreadJob indicates failure.  This can be
    //  called repeatedly.
    // Destroy the cThreadPool, or call clear, submit, and run, for
    //  another task.

    void setThreadData(sTPthreadData*, unsigned int);
    void submit(TPthreadJob, void*, TPdestroy = 0);
    int run(sTPthreadData*);

    unsigned int num_threads()    const { return (tp_nthreads); }

    void clear()
        {
            tp_list = 0;
            sTPjobList::destroy(tp_list_bak);
            tp_list_bak = 0;
            tp_list_end = 0;
        }

private:
    static void *tp_thread_proc(void*);

    unsigned int    tp_nthreads;
    int             tp_error;
    sTPstate        *tp_state;
    sTPjobList      *tp_list;
    sTPjobList      *tp_list_bak;
    sTPjobList      *tp_list_end;
    pthread_mutex_t tp_mtx;
    pthread_cond_t  tp_cnd;
};

#endif


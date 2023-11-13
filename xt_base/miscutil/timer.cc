
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

#include "config.h"
#include "timer.h"
#include "tvals.h"

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#ifdef USE_PTHREAD
#include <pthread.h>
#endif
#include <sys/time.h>
#include <time.h>

#ifdef WIN32
#include "msw.h"
#endif


//
// System time reference generator.  What we want is to asynchronously
// increment a time value.  Using pthreads is the logical approach,
// but the overhead seems to be unacceptable, i.e., operations take
// 1.5-2 times longer.  Instead we use setitimer and hope to catch any
// bad effects this produces, such as select(), etc. returning early.
//

namespace miscutil {

    cTimer *cTimer::instancePtr = 0;

    cTimer::cTimer()
    {
        if (instancePtr) {
            fprintf(stderr, "Singleton class cTimer already instantiated.\n");
            exit(1);
        }
        instancePtr = this;

        t_elapsed_time = 0;
        t_callback = 0;
        t_period = 0;
        t_started = false;
    }


#ifdef TIMER_TEST_NULL
    // Private static error exit.
    //
    void
    cTimer::on_null_ptr()
    {
        fprintf(stderr, "Singleton class cTimer used before instantiated.\n");
        exit(1);
    }
#endif


    // Start an asynchronous timer which increments the t_elapsed_time
    // parameter.  This can be polled, e.g., to determine when to check
    // for events.  The period is given in milliseconds.
    //
    void
    cTimer::start(int period)
    {
        if (t_started)
            return;
        t_started = true;
        if (period <= 0)
            return;
        t_period = period;
#ifdef WIN32
        _beginthread(timer_thread_cb, 0, 0);
#else
#ifdef USE_PTHREAD
        static pthread_t thread;
        pthread_create(&thread, &attr, timer_thread_cb, 0);
#else
        itimerval it;
        it.it_value.tv_sec = period/1000;
        it.it_value.tv_usec = (period % 1000)*1000;
        it.it_interval.tv_sec = period/1000;
        it.it_interval.tv_usec = (period % 1000)*1000;

        setitimer(ITIMER_REAL, &it, 0);
        signal(SIGALRM, alarm_hdlr);
#endif
#endif
    }


    // Statis function.
    // This is functionally separate from the main timer.
    //
    void
    cTimer::milli_sleep(int ms)
    {
#ifdef WIN32
        Sleep(ms);
#else
        fd_set fds;
        FD_ZERO(&fds);
        timeval to;
        to.tv_sec = ms/1000;
        to.tv_usec = (ms % 1000)*1000;

        // Have to decrease the timer interval on interrupt.
        unsigned int t0 = Tvals::millisec();
        for (;;) {
            // Back out of any SIGALRM handler, causes trouble in select.
            sig_t hdlr = signal(SIGALRM, SIG_IGN);
            int n = select(1, &fds, 0, 0, &to);
            signal(SIGALRM, hdlr);
            if (n >= 0)
                break;
            unsigned int t = Tvals::millisec();
            ms -= (t - t0);
            if (ms <= 0)
                break;
            t0 = t;
            to.tv_sec = ms/1000;
            to.tv_usec = (ms % 1000)*1000;
        }
#endif
    }


#ifdef WIN32

    void
    cTimer::timer_thread_cb(void*)
    {
        for (;;) {
            cTimer::self()->t_elapsed_time = Tvals::millisec();
            Sleep(cTimer::self()->t_period);
            if (cTimer::self()->t_callback)
                (*cTimer::self()->t_callback)();
        }
    }

#else
#ifdef USE_PTHREAD

    void *
    cTimer::timer_thread_cb(void*)
    {
        for (;;) {
            cTimer::self()->t_elapsed_time = Tvals::millisec();
            milli_sleep(cTimer::self()->t_period);
            if (cTimer::self()->t_callback)
                (*cTimer::self()->t_callback)();
        }
        return (0);
    }

#else

    void
    cTimer::alarm_hdlr(int)
    {
        signal(SIGALRM, alarm_hdlr);
        cTimer::self()->t_elapsed_time = Tvals::millisec();
        if (cTimer::self()->t_callback)
            (*cTimer::self()->t_callback)();
    }

#endif
#endif

}


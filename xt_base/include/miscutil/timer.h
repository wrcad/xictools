
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

#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>


// If defined, include test for use of main class before initialized.
//#define TIMER_TEST_NULL

namespace miscutil {

    class cTimer
    {
#ifdef TIMER_TEST_NULL
        static cTimer *ptr()
            {
                if (!instancePtr)
                    on_null_ptr();
                return (instancePtr);
            }

        static void on_null_ptr();
#endif

    public:
#ifdef TIMER_TEST_NULL
        static cTimer *self()       { return (cTimer::ptr()); }
#else
        static cTimer *self()       { return (instancePtr); }
#endif

        cTimer();
        void start(int);

        uint64_t elapsed_msec()             { return (t_elapsed_time); }

        bool check_interval(uint64_t &check_time)
            {
                if (t_elapsed_time > check_time) {
                    check_time = t_elapsed_time;
                    return (true);
                }
                return (false);
            }

        void register_callback(void(*cb)()) { t_callback = cb; }
        static void milli_sleep(int);

    private:
#ifdef WIN32
        static void timer_thread_cb(void*);
#else
#ifdef USE_PTHREAD
        static void *timer_thread_cb(void *);
#else
        static void alarm_hdlr(int);
#endif
#endif

        uint64_t t_elapsed_time;
        void(*t_callback)();
        int t_period;
        bool t_started;

        static cTimer *instancePtr;
    };
}
using namespace miscutil;

#endif


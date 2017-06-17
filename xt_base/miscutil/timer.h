
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2007 Whiteley Research Inc, all rights reserved.        *
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
 *                                                                        *
 *========================================================================*
 *                                                                        *
 * Misc. Utilities Library                                                *
 *                                                                        *
 *========================================================================*
 $Id: timer.h,v 1.6 2015/10/30 04:37:51 stevew Exp $
 *========================================================================*/

#ifndef TIMER_H
#define TIMER_H

// If defined, include test for use of main class before initialized.
//#define TIMER_TEST_NULL

inline class cTimer *Timer();

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
    friend inline cTimer *Timer()       { return (cTimer::ptr()); }
#else
    friend inline cTimer *Timer()       { return (instancePtr); }
#endif

    cTimer();
    void start(int);

    unsigned long elapsed_msec()        { return (t_elapsed_time); }

    bool check_interval(unsigned long &check_time)
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

    unsigned long t_elapsed_time;
    void(*t_callback)();
    int t_period;
    bool t_started;

    static cTimer *instancePtr;
};

#endif


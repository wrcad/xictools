
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
 * QtInterf Graphical Interface Library                                   *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#ifndef INTERVAL_TIMER_H
#define INTERVAL_TIMER_H

#include <QTimer>

namespace qtinterf
{
    class QTtimer: public QTimer
    {
        Q_OBJECT

    public:
        QTtimer(int(*)(void*), void*, QTtimer*, QObject*);
        ~QTtimer();

        // Manage external list of timers.
        QTtimer *nextTimer() { return next; }
        void setNextTimer(QTtimer *t) { next = t; }

        // Register the timer list head address.  The timer will
        // be removed from this list when destroyed.
        void register_list(QTtimer **l) { list = l; }

        // Start timer, nothing happens until this is called.
        void start(int ms) { msec = ms; QTimer::start(ms); }

        // Return unique id.
        int id() { return (timer_id); }

        // Set mode.  If unset (the default) the callback return is
        // ignored.  The timer will stop after timout, and can be
        // restarted by calling start.  If set, the callback will
        // restart the timer by returning true, or destroy the timer
        // by returning false.
        //
        void set_use_return(bool b) { use_cb_ret = b; }

    signals:
        void destroy(QTtimer*);

    private slots:
        void timeout_slot();

    private:
        int (*callback)(void*);
        void *arg;
        QTtimer *next;
        int timer_id;
        int msec;
        QTtimer **list;
        bool deleted;
        bool use_cb_ret;
    };
}

#endif

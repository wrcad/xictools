
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

#include "qttimer.h"


using namespace qtinterf;

QTtimer::QTtimer(int(*cb)(void*), void *a, QTtimer *n, QObject *p) : QTimer(p)
{
    // QT's timerId returns -1 unless the timer is active, so we need
    // our own id generator.
    static int id_cntr = 1;

    t_callback = cb;
    t_arg = a;
    t_next = n;
    t_msec = 0;
    t_list = 0;
    t_timer_id = id_cntr++;
    t_deleted = false;
    t_use_cb_ret = false;
    connect(this, &QTtimer::timeout, this, &QTtimer::timeout_slot);
}


QTtimer::~QTtimer()
{
    t_deleted = true;   // Make sure that no timeouts are delivered after
                        // the timer is destroyed.
    if (t_list) {
        QTtimer *tp = 0;
        for (QTtimer *t = *t_list; t; t = t->t_next) {
            if (t == this) {
                if (tp)
                    tp->t_next = t->t_next;
                else
                    *t_list = t->t_next;
                break;
            }
            tp = t;
        }
    }
    emit destroy(this);
}


void
QTtimer::timeout_slot()
{
    stop();
    if (!t_deleted && t_callback) {
        bool r = (*t_callback)(t_arg);
        if (t_use_cb_ret) {
            if (r)
                start(t_msec);
            else
                delete this;
        }
    }
}


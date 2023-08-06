
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

#include "qtidleproc.h"


using namespace qtinterf;

QTidleproc::QTidleproc() : QTimer(0)
{
    ip_idle_proc_list = 0;
    ip_idle_id_cnt = 1000;
    ip_running = false;
    connect(this, SIGNAL(timeout()), this, SLOT(run_slot()));
}


// Add an idle function callback.  The function will be called repeatedly
// until 0 is returned, at which point it will be removed from the list.
// An id for the callback is returned.
//
int
QTidleproc::add(int(*cb)(void*), void *arg)
{
    idle_procs *ip = new idle_procs(cb, arg);
    if (!ip_idle_proc_list)
        ip_idle_proc_list = ip;
    else {
        idle_procs *p = ip_idle_proc_list;
        while (p->next)
            p = p->next;
        p->next = ip;
    }
    ip_idle_proc_list->id = ip_idle_id_cnt++;
    if (!ip_running) {
        start();
        ip_running = true;
    }
    return (ip_idle_proc_list->id);
}


// Remove an idle function callback from the list.  The argument is
// the return value obtained when the callback was added.  Return
// true if a removal was done.
//
bool
QTidleproc::remove(int iid)
{
    idle_procs *p = 0;
    for (idle_procs *ip = ip_idle_proc_list; ip; ip = ip->next) {
        if (ip->id == iid) {
            if (p)
                p->next = ip->next;
            else
                ip_idle_proc_list = ip->next;
            delete ip;
            return (true);
        }
        p = ip;
    }
    return (false);
}


// Slot to run the idle queue.  The first callback is popped off and
// run.  If the callback returns true, the callback is appended to the
// end of the list, otherwise it is deleted.
//
void
QTidleproc::run_slot()
{
    if (ip_idle_proc_list) {
        idle_procs *ip = ip_idle_proc_list;
        ip_idle_proc_list = ip->next;
        ip->next = 0;

        int ret = (*ip->proc)(ip->arg);
        if (ret) {
            if (!ip_idle_proc_list)
                ip_idle_proc_list = ip;
            else {
                idle_procs *p = ip_idle_proc_list;
                while (p->next)
                    p = p->next;
                p->next = ip;
            }
        }
        else
            delete ip;
    }

    if (!ip_idle_proc_list) {
        stop();
        ip_running = false;
    }
}


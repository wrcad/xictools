
/*========================================================================*
 *                                                                        *
 *  Distributed by Whiteley Research Inc., Sunnyvale, California, USA     *
 *                       http://wrcad.com                                 *
 *  Copyright (C) 2018 Whiteley Research Inc., all rights reserved.       *
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
 * WRspice Circuit Simulation and Analysis Tool:  Device Library          *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#ifndef TIMELIST_H
#define TIMELIST_H

// Storage for history values.  WRspice used to use arrays which would
// grow in length to the total number of internal time points for each
// line.  This is very inefficient memory use, so instead we now use a
// doubly-linked list, based on the structures declared here.  This is
// still not great, should use some kind of circular buffer based on
// arrays for lower memory usage.


// The main class template.
//
template <class T>
struct timelist
{
    // Base class for the linked list element.  User should subclass this
    // adding their own data items.
    //
    struct telt
    {
        double time;
        T *next;
        T *prev;
    };

    timelist()
        {
            tl_head = 0;
            tl_tail = 0;
            tl_freelist = 0;
        }

    ~timelist()
        {
            while (tl_tail) {
                T *tx = tl_tail;
                tl_tail = tl_tail->next;
                delete tx;
            }
            while (tl_freelist) {
                T *tx = tl_freelist;
                tl_freelist = tl_freelist->next;
                delete tx;
            }
        }

    // Allocate and return a new list element for time.
    //
    T *link_new(double time)
        {
            if (tl_head && time <= tl_head->time)
                return (0);
            T *t = 0;
            if (tl_freelist) {
                t = tl_freelist;
                tl_freelist = tl_freelist->next;
            }
            if (!t)
                t = new T;
            t->time = time;
            t->next = 0;
            t->prev = tl_head;

            if (tl_head)
                tl_head->next = t;
            tl_head = t;
            if (!tl_tail)
                tl_tail = t;

            return (tl_head);
        }

    // Get rid of elements before, put them in the freelist for
    // recycling.
    //
    void free_tail(double time)
        {

            while (tl_tail->time < time) {
                T *tx = tl_tail->next;
                if (tx)
                    tx->prev = 0;
                tl_tail->next = tl_freelist;
                tl_freelist = tl_tail;
                tl_tail = tx;
                if (!tl_tail) {
                    tl_head = 0;
                    break;
                }
            }
        }

    T *tail()       { return (tl_tail); }
    T *head()       { return (tl_head); }

private:
    T *tl_tail;
    T *tl_head;
    T *tl_freelist;
};

#endif



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
 * Ginterf Graphical Interface Library                                    *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#ifndef SHMCTL_H_INCLUDED
#define SHMCTL_H_INCLUDED

#include "miscutil/symtab.h"


//
// An allocator and manager for SYSV shared memory segments used for
// the X shared memory extension when available.  This takes care of
// cleanup when the application terminates.
//

struct ShmCtl
{
    struct shm_el
    {
        unsigned long tab_key()         { return (key); }
        shm_el *tab_next()              { return (next); }
        void set_tab_next(shm_el *n)    { next = n; }
        shm_el *tgen_next(bool)         { return (next); }

        unsigned long key;
        shm_el *next;
        unsigned int id;
    };

    ShmCtl()
        {
            if (instance) {
                fprintf(stderr,
                    "Singleton class ShmCtl instantiated more than once.\n");
                exit (1);
            }
            shm_tab = 0;
            instance = this;
        }

    ~ShmCtl();

    static void *allocate(int*, unsigned int);
    static void deallocate(void*);

private:
    itable_t<shm_el> *shm_tab;
    static ShmCtl *instance;
};

#endif



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

#include "config.h"
#ifdef HAVE_SHMGET
#include <sys/ipc.h>
#include <sys/shm.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include "shmctl.h"
#include "graphics.h"


namespace { ShmCtl _shmctl_; }

ShmCtl *ShmCtl::instance = 0;

// Debugging.
//#define SHMDBG


// Destructor, deallocate any active segments.  This occurs on program
// termination.
//
ShmCtl::~ShmCtl()
{
#ifdef HAVE_SHMGET
    if (shm_tab) {
        tgen_t<shm_el> gen(shm_tab);
        shm_el *e;
        while ((e = gen.next()) != 0) {
#ifdef SHMDBG
printf("terminal dealloc shm segment id %d\n", e->id);
#endif
            shmdt((void*)e->key);
            shmctl(e->id, IPC_RMID, 0);
        }
    }
#endif
}


// Static function.
// Allocate a shared memory block if possible, returning the shmid in
// the argument.  If not using shared memory, return a heap allocation
// with id = 0.
//
void *
ShmCtl::allocate(int *retid, unsigned int sz)
{
#ifdef HAVE_SHMGET
//XXX    if (GRpkg::self()->UseSHM()) {
        if (!instance) {
            fprintf(stderr, "Class ShmCtl used before allocated.\n");
            exit (1);
        }
#ifdef SHMDBG
printf("allocating shm %d bytes\n", sz);
#endif
        int shmid = shmget(IPC_PRIVATE, sz, IPC_CREAT | 0777);
        if (shmid == -1) {
            // Some kind of error, can't use SHM.
            shmid = 0;
#ifdef SHMDBG
printf("failed, using heap\n");
#endif
            *retid = 0;
            return (new char[sz]);
        }
        *retid = shmid;
        if (!instance->shm_tab)
            instance->shm_tab = new itable_t<shm_el>;
        void *addr = (void*)shmat(shmid, 0, 0);
        shm_el *e = new shm_el;
        e->key = (unsigned long)addr;
        e->next = 0;
        e->id = shmid;
        instance->shm_tab->link(e);
        instance->shm_tab = instance->shm_tab->check_rehash();
        return (addr);
//XXX    }
#endif
#ifdef SHMDBG
printf("allocating normal %d bytes\n", sz);
#endif
    *retid = 0;
    return (new char[sz]);
}


// Static function.
// Deallocate or free a pointer returned by the allocate method.
//
void
ShmCtl::deallocate(void *addr)
{
#ifdef HAVE_SHMGET
    if (instance && instance->shm_tab) {
        if (!addr)
            return;
        shm_el *e = instance->shm_tab->find(addr);
        if (e) {
#ifdef SHMDBG
printf("dealloc shm segment id %d\n", e->id);
#endif
            shmdt(addr);
            shmctl(e->id, IPC_RMID, 0);
            instance->shm_tab->unlink(e);
            delete e;
            return;
        }
    }
#endif
#ifdef SHMDBG
printf("normal dallocation\n");
#endif
    delete [] (char*)addr;
}


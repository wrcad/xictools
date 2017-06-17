
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2010 Whiteley Research Inc, all rights reserved.        *
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
 *========================================================================*
 *                                                                        *
 * Ginterf Graphical Interface Library                                    *
 *                                                                        *
 *========================================================================*
 $Id: shmctl.cc,v 2.2 2010/06/05 17:47:06 stevew Exp $
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
    if (GRpkgIf()->UseSHM()) {
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
    }
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


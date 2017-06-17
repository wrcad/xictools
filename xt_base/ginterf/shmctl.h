
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
 $Id: shmctl.h,v 2.1 2010/05/23 18:04:53 stevew Exp $
 *========================================================================*/

#ifndef SHMCTL_H_INCLUDED
#define SHMCTL_H_INCLUDED

#include "symtab.h"


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


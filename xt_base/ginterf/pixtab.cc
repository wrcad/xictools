
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2005 Whiteley Research Inc, all rights reserved.        *
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
 $Id: pixtab.cc,v 2.5 2007/08/25 07:22:23 stevew Exp $
 *========================================================================*/

#include "pixtab.h"

using namespace ginterf;

//
// pixel number mapping functions
//

// Add a new map entry.  key is the application pixel, data is the
// mapped-to pixel index
//
void
ptab::add(int key, int data)
{
    int hkey = key % PTSIZE;
    pent *pp = 0;
    for (pent *p = tab[hkey]; p; pp = p, p = p->next) {
        if (p->key == key) {
            p->data = data;
            return;
        }
        if (p->key > key) {
            if (pp)
                pp->next = new pent(key, data, pp->next);
            else
                tab[hkey] = new pent(key, data, tab[hkey]);
            return;
        }
    }
    if (pp)
        pp->next = new pent(key, data, 0);
    else
        tab[hkey] = new pent(key, data, 0);
}


// Given the application pixel value, return the mapped-to value
//
int
ptab::get(int key)
{
    int hkey = key % PTSIZE;
    for (pent *p = tab[hkey]; p; p = p->next) {
        if (p->key == key)
            return (p->data);
        else if (p->key > key)
            break;
    }
    return (0);
}


ptab::~ptab()
{
    for (int i = 0; i < PTSIZE; i++) {
        pent *pn;
        for (pent *p = tab[i]; p; p = pn) {
            pn = p->next;
            delete p;
            p = pn;
        }
    }
}


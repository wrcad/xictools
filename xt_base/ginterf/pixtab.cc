
/*========================================================================*
 *                                                                        *
 *  Whiteley Research Inc, Sunnyvale, CA USA, http://wrcad.com            *
 *  Copyright (C) 2017 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *  Licensed under the Apache License, Version 2.0 (the "License");       *
 *  you may not use this file except in compliance with the License.      *
 *  You may obtain a copy of the License at                               *
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



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
 * Xic Integrated Circuit Layout and Schematic Editor                     *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#include "config.h"
#include "cd.h"
#include "cd_types.h"
#include "hashfunc.h"


//
// Comparison and hashing functions for objects.
//

// This does not consider properties.
//
bool
CDo::operator==(const CDo &od) const
{
    if (type() != od.type())
        return (false);
    if (ldesc() != od.ldesc())
        return (false);

#ifdef HAVE_COMPUTED_GOTO
    COMPUTED_JUMP(this)
#else
    CONDITIONAL_JUMP(this)
#endif

box:
    if (oBB() != od.oBB())
        return (false);
    return (true);

poly:
    {
        if (oBB() != od.oBB())
            return (false);
        return (((const CDpo*)this)->po_v_compare(((const CDpo*)&od)));
    }
    return (true);

wire:
    {
        if (oBB() != od.oBB())
            return (false);
        if (((const CDw*)this)->attributes() !=
                ((const CDw*)&od)->attributes())
            return (false);
        return (((const CDw*)this)->w_v_compare(((const CDw*)&od)));
    }
    return (true);

label:
    {
        if (oBB() != od.oBB())
            return (false);
        CDla *l1 = (CDla*)this, *l2 = (CDla*)&od;
        if (l1->xpos() != l2->xpos())
            return (false);
        if (l1->ypos() != l2->ypos())
            return (false);
        if (l1->width() != l2->width())
            return (false);
        if (l1->height() != l2->height())
            return (false);
        if (l1->xform() != l2->xform())
            return (false);
        // Don't bother to check the text, assume labels are the same
        // if they occupy exactly same space.
    }
    return (true);

inst:
    {
        CDc *c1 = (CDc*)this, *c2 = (CDc*)&od;
        if (c1->master() != c2->master() && c1->cellname() != c2->cellname())
            return (false);
        if (c1->attr() != c2->attr())
            return (false);
        if (c1->posX() != c2->posX())
            return (false);
        if (c1->posY() != c2->posY())
            return (false);
    }
    return (true);
}


unsigned int
CDo::hash() const
{
    unsigned int k = INCR_HASH_INIT;
    char c = type();
    k = incr_hash(k, &c);
    k = incr_hash(k, &e_children);
    k = incr_hash(k, &e_BB.left);
    k = incr_hash(k, &e_BB.bottom);
    k = incr_hash(k, &e_BB.right);
    k = incr_hash(k, &e_BB.top);

#ifdef HAVE_COMPUTED_GOTO
    COMPUTED_JUMP(this)
#else
    CONDITIONAL_JUMP(this)
#endif

box:
    return (k);

poly:
    k = ((CDpo*)this)->add_hash(k);
    return (k);

wire:
    k = ((CDw*)this)->add_hash(k);
    return (k);

label:
    k = ((CDla*)this)->add_hash(k);
    return (k);

inst:
    k = ((CDc*)this)->add_hash(k);
    return (k);
}
// End of CDo functions.


unsigned int
CDpo::add_hash(unsigned int k)
{
    Point *p = poPoints;
    int n = poNumpts;
    while (n--) {
        k = incr_hash(k, &p->x);
        k = incr_hash(k, &p->y);
        p++;
    }
    return (k);
}
// End of CDpo functions.


unsigned int
CDw::add_hash(unsigned int k)
{
    Point *p = wPoints;
    int n = wNumpts;
    while (n--) {
        k = incr_hash(k, &p->x);
        k = incr_hash(k, &p->y);
        p++;
    }
    k = incr_hash(k, &wAttributes);
    return (k);
}
// End of CDw functions.


unsigned int
CDla::add_hash(unsigned int k)
{
    k = incr_hash(k, &laXform);
    k = incr_hash(k, &laX);
    k = incr_hash(k, &laY);
    k = incr_hash(k, &laWidth);
    k = incr_hash(k, &laHeight);
    return (k);
}
// End of CDla functions.


unsigned int
CDc::add_hash(unsigned int k)
{
    k = incr_hash(k, &cAttr);
    // The cellname is in the string table, treat like a long value.
    unsigned long cn = (unsigned long)cellname();
    k = incr_hash(k, &cn);
    k = incr_hash(k, &cX);
    k = incr_hash(k, &cY);
    return (k);
}
// End of CDc functions.



/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2007 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *  WHITELEY RESEARCH INCORPORATED PROPRIETARY SOFTWARE                   *
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
 *                                                                        *
 * XIC Integrated Circuit Layout and Schematic Editor                     *
 *                                                                        *
 *========================================================================*
 $Id: cd_lists.cc,v 5.17 2014/02/09 00:24:53 stevew Exp $
 *========================================================================*/

#include "cd.h"
#include "cd_types.h"
#include "pathlist.h"
#include <algorithm>


namespace {
    // Alphabetic in cell name, descending in top, ascending in left.
    //
    bool instcmp(const CDc *c1, const CDc *c2)
    {
        const char *s1 = c1->cellname()->string();
        const char *s2 = c2->cellname()->string();
        if (s1 == s2) {
            const BBox &b1 = c1->oBB();
            const BBox &b2 = c2->oBB();
            if (b1.top > b2.top)
                return (true);
            if (b1.top < b2.top)
                return (false);
            return (b1.left < b2.left);
        }
        return (strcmp(s1, s2) < 0);
    }
}


// Sort the instance list.
//
void
CDcl::sort_instances()
{
    int cnt = 0;
    for (CDcl *cl = this; cl; cl = cl->next)
        cnt++;
    if (cnt < 2)
        return;
    const CDc **ary = new const CDc*[cnt];
    cnt = 0;
    for (CDcl *cl = this; cl; cl = cl->next)
        ary[cnt++] = cl->cdesc;
    std::sort(ary, ary + cnt, instcmp);
    cnt = 0;
    for (CDcl *cl = this; cl; cl = cl->next)
        cl->cdesc = ary[cnt++];
    delete [] ary;
}


// Unlink which from list, return a pointer to it if found.
//
CDcl *
CDcl::unlink(CDcl **list, const CDc *which)
{
    if (!list)
        return (0);
    CDcl *p = 0;
    for (CDcl *c = *list; c; p = c, c = c->next) {
        if (c->cdesc == which) {
            if (!p)
                *list = c->next;
            else
                p->next = c->next;
            return (c);
        }
    }
    return (0);
}


void
CDcl::free()
{
    CDcl *n;
    for (CDcl *c = this; c; c = n) {
        n = c->next;
        delete c;
    }
}
// End CDcl functions


bool
CDll::inlist(CDl *ld)
{
    for (CDll *l = this; l; l = l->next)
        if (l->ldesc == ld)
            return (true);
    return (false);
}


CDll *
CDll::unlink(CDll **list, CDl *which)
{
    if (!list)
        return (0);
    CDll *p = 0;
    for (CDll *l = *list; l; p = l, l = l->next) {
        if (l->ldesc == which) {
            if (!p)
                *list = l->next;
            else
                p->next = l->next;
            return (l);
        }
    }
    return (0);
}


void
CDll::free()
{
    CDll *n;
    for (CDll *l = this; l; l = n) {
        n = l->next;
        delete l;
    }
}
// End CDll functions


void
CDol::free()
{
    CDol *n;
    for (CDol *o = this; o; o = n) {
        n = o->next;
        delete o;
    }
}


// Compute the BB of the listed objects
//
void
CDol::computeBB(BBox *nBB) const
{
    *nBB = CDnullBB;
    for (const CDol *ol = this; ol; ol = ol->next) {
        if (ol->odesc)
            nBB->add(&ol->odesc->oBB());
    }
}


// Copy the list
//
CDol *
CDol::dup() const
{
    CDol *ol0 = 0, *ole = 0;
    for (const CDol *ol = this; ol; ol = ol->next) {
        if (!ol0)
            ol0 = ole = new CDol(ol->odesc, 0);
        else {
            ole->next = new CDol(ol->odesc, 0);
            ole = ole->next;
        }
    }
    return (ol0);
}
// End CDol functions


namespace {
    // Comparison function for properties
    //
    inline bool
    p_comp(const CDp *p1, const CDp *p2)
    {
        return (p1->value() < p2->value());
    }
}


// Sort a list of properties by increasing value
//
void
CDpl::sort()
{
    int cnt = 0;
    for (CDpl *p = this; p; p = p->next, cnt++) ;
    if (cnt < 2)
        return;
    CDp **aa = new CDp*[cnt];
    cnt = 0;
    for (CDpl *p = this; p; p = p->next, cnt++)
        aa[cnt] = p->pdesc;
    std::sort(aa, aa + cnt, p_comp);
    cnt = 0;
    for (CDpl *p = this; p; p = p->next, cnt++)
        p->pdesc = aa[cnt];
    delete [] aa;
}


void
CDpl::free()
{
    CDpl *n;
    for (CDpl *p = this; p; p = n) {
        n = p->next;
        delete p;
    }
}
// End CDpl functions


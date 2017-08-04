
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
 * Xic Integrated Circuit Layout and Schematic Editor                     *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#include "cd.h"
#include "cd_types.h"
#include "geo_ylist.h"
#include "timedbg.h"


// Return a list of zoids from objects on ld that overlap pBB.
//
Zlist *
CDs::getRawZlist(int maxdepth, const CDl *ld, const BBox *pBB,
    XIrt *retp) const
{
    if (!ld) {
        if (retp)
            *retp = XIbad;
        return (0);
    }
    if (retp)
        *retp = XIok;
    Zlist *head = 0;
    if (ld == CellLayer()) {
        CDg gdesc;
        gdesc.init_gen(this, ld, pBB);
        CDc *cdesc;
        while ((cdesc = (CDc*)gdesc.next()) != 0) {
            if (!cdesc->is_normal())
                continue;
            Zlist *zn = cdesc->toZlist();
            if (zn) {
                Zlist *zk = zn;
                while (zk->next)
                    zk = zk->next;
                zk->next = head;
                head = zn;
            }
        }
    }
    else {
        sPF gen(this, pBB, ld, maxdepth);
        CDo *odesc;
        while ((odesc = gen.next(false, false)) != 0) {
            Zlist *zn = odesc->toZlist();
            if (zn) {
                Zlist *zk = zn;
                while (zk->next)
                    zk = zk->next;
                zk->next = head;
                head = zn;
            }
            delete odesc;
        }
    }
    return (head);
}


// Return a zoid list representing the coverage of ld, to maxdepth in
// the hierarchy.  The list is clipped to zref.  The return status is
// provided in retp.
//
Zlist *
CDs::getZlist(int maxdepth, const CDl *ld, const Zlist *zref,
    XIrt *retp) const
{
    TimeDbgAccum ac("getZlist");

    if (!ld) {
        if (retp)
            *retp = XIbad;
        return (0);
    }
    if (retp)
        *retp = XIok;
    Zlist *z0 = 0;
    for (const Zlist *zl = zref; zl; zl = zl->next) {
        BBox tBB;
        zl->Z.BB(&tBB);
        bool manh = zl->Z.is_rect();
        bool noclip = (zl->Z.yu >= sBB.top && zl->Z.yl <= sBB.bottom &&
                    zl->Z.xll <= sBB.left && zl->Z.xul <= sBB.left &&
                    zl->Z.xlr >= sBB.right && zl->Z.xur >= sBB.right);

        if (ld == CellLayer()) {
            CDg gdesc;
            gdesc.init_gen(this, ld, &tBB);
            CDc *cdesc;
            while ((cdesc = (CDc*)gdesc.next()) != 0) {
                if (!cdesc->is_normal())
                    continue;
                Zlist *zx = cdesc->toZlist();
                if (!noclip && (!manh || !(cdesc->oBB() <= tBB)))
                    Zlist::zl_and(&zx, &zl->Z);
                if (zx) {
                    Zlist *zn = zx;
                    while (zn->next)
                        zn = zn->next;
                    zn->next = z0;
                    z0 = zx;
                }
            }
        }
        else {
            const bool nocopy = (maxdepth == 0);
            sPF gen(this, &tBB, ld, maxdepth);
            CDo *odesc;
            while ((odesc = gen.next(nocopy, false)) != 0) {
                Zlist *zx = odesc->toZlist();
                if (!noclip && (!manh || !(odesc->oBB() <= tBB)))
                    Zlist::zl_and(&zx, &zl->Z);
                if (zx) {
                    Zlist *zn = zx;
                    while (zn->next)
                        zn = zn->next;
                    zn->next = z0;
                    z0 = zx;
                }
                if (!nocopy)
                    delete odesc;
            }
        }
    }

    try {
        z0 = Zlist::repartition(z0);
        return (z0);
    }
    catch (XIrt ret) {
        if (retp)
            *retp = ret;
        return (0);
    }
}


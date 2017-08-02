
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

#include "cd.h"
#include "geo_ylist.h"
#include "timedbg.h"


//------------------------------------------------------------------------
// Zlist exports.  These encapsulate exception handling.
//------------------------------------------------------------------------

// Enable the old "safe clipping" mode for debugging.
//#define SCLDEBUG


// Static function.
// Check whether or not the lists intersect.  The lists are destroyed.
//
bool
Zlist::zl_intersect(Zlist *zl1, Zlist *zl2, bool touchok)
{
    if (!zl1 || !zl2)
        return (false);

    // Do simple case
    if (!zl1->next && !zl2->next) {
        bool ret = zl1->Z.intersect(&zl2->Z, touchok);
        Zlist::destroy(zl1);
        Zlist::destroy(zl2);
        return (ret);
    }

    Ylist *yl1 = new Ylist(zl1);
    Ylist *yl2 = new Ylist(zl2);

    bool ret = Ylist::intersect(yl1, yl2, touchok);
    Ylist::destroy(yl1);
    Ylist::destroy(yl2);
    return (ret);
}


// Static function.
// On success, return (zl1 | zl2) in zl1p, zl2 is consumed.
// On exception: *zl1p and zl2 are freed.
//
XIrt
Zlist::zl_or(Zlist **zl1p, Zlist *zl2)
{
    if (!zl2)
        return (XIok);
    if (!*zl1p) {
        *zl1p = zl2;
        return (XIok);
    }
    Zlist *zn = *zl1p;
    while (zn->next)
        zn = zn->next;
    zn->next = zl2;
    try {
        *zl1p = Zlist::repartition((*zl1p));
        return (XIok);
    }
    catch (XIrt ret) {
        *zl1p = 0;
        return (ret);
    }
}


// Static function.
// On success, return the self-intersecting areas of the list in *zp.
// On exception: *zp is freed.
//
XIrt
Zlist::zl_and(Zlist **zp)
{
    Zlist *zl = *zp;
    if (!zl || !zl->next) {
        Zlist::destroy(zl);
        *zp = 0;
        return (XIok);
    }
    Ylist *yl = new Ylist(zl);
    try {
        *zp = Ylist::clip_to_self(yl);
        return (XIok);
    }
    catch (XIrt ret) {
        *zp = 0;
        return (ret);
    }
}


// Static function.
// On success, return (zl1 & ZB) in zl1p.  No interrupt check.
//
XIrt
Zlist::zl_and(Zlist **zl1p, const Zoid *ZB)
{
    if (!*zl1p || !ZB) {
        Zlist::destroy((*zl1p));
        *zl1p = 0;
        return (XIok);
    }
    if (!(*zl1p)->next) {
        Zlist *zn = (*zl1p)->Z.clip_to(ZB);
        delete *zl1p;
        *zl1p = zn;
        return (XIok);
    }

    Ylist *y = new Ylist(*zl1p);
    *zl1p = Ylist::clip_to_zoid(y, ZB);
    Ylist::destroy(y);
    return (XIok);
}


// Static function.
// On success, return (zl1 & zl2) in zl1p, zl2 is freed.
// On exception: *zl1p and zl2 are freed.
//
XIrt
Zlist::zl_and(Zlist **zl1p, Zlist *zl2)
{
    TimeDbgAccum ac("zl_and");

    if (!*zl1p || !zl2) {
        Zlist::destroy((*zl1p));
        *zl1p = 0;
        Zlist::destroy(zl2);
        return (XIok);
    }

    // Do simple case
    if (!(*zl1p)->next && !zl2->next) {
        Zlist *zt = *zl1p;
        *zl1p = (zt->Z).clip_to(&zl2->Z);
        Zlist::destroy(zl2);
        Zlist::destroy(zt);
        return (XIok);
    }

    Ylist *yl1 = new Ylist(*zl1p);
    Ylist *yl2 = new Ylist(zl2);

#ifdef SCLDEBUG
    if (GEO()->useSclFuncs()) {
        try {
            *zl1p = yl1->scl_clip_to(yl2)->to_zlist();
            return (XIok);
        }
        catch (XIrt ret) {
            *zl1p = 0;
            return (ret);
        }
    }
#endif
    try {
        *zl1p = Ylist::clip_to_ylist(yl1, yl2);
        Ylist::destroy(yl1);
        Ylist::destroy(yl2);
        return (XIok);
    }
    catch (XIrt ret) {
        *zl1p = 0;
        Ylist::destroy(yl1);
        Ylist::destroy(yl2);
        return (ret);
    }
}


// Static function.
// On success, return (zl1 & yl2) in zl1p.
// On exception: *zl1p is freed.
//
XIrt
Zlist::zl_and(Zlist **zl1p, const Ylist *yl2)
{
    TimeDbgAccum ac("zl_and_y");

    if (!*zl1p || !yl2 || (!yl2->y_zlist && !yl2->next)) {
        Zlist::destroy((*zl1p));
        *zl1p = 0;
        return (XIok);
    }

    Ylist *yl1 = new Ylist(*zl1p);

#ifdef SCLDEBUG
    if (GEO()->useSclFuncs()) {
        try {
            *zl1p = yl1->scl_clip_to(yl2->copy())->to_zlist();
            return (XIok);
        }
        catch (XIrt ret) {
            *zl1p = 0;
            return (ret);
        }
    }
#endif
    try {
        *zl1p = Ylist::clip_to_ylist(yl1, yl2);
        Ylist::destroy(yl1);
        return (XIok);
    }
    catch (XIrt ret) {
        *zl1p = 0;
        Ylist::destroy(yl1);
        return (ret);
    }
}


// Static function.
// On success, return the areas of the list that do not overlap in *zp.
// On exception: *zp is freed.
//
XIrt
Zlist::zl_andnot(Zlist **zp)
{
    if (!*zp || !(*zp)->next)
        return (XIok);
    Ylist *yl = new Ylist(*zp);

#ifdef SCLDEBUG
    if (GEO()->useSclFuncs()) {
        try {
            *zp = yl->scl_clip_out()->to_zlist();
            return (XIok);
        }
        catch (XIrt ret) {
            *zp = 0;
            return (ret);
        }
    }
#endif
    try {
        *zp = Ylist::clip_out_self(yl);
        Ylist::destroy(yl);
        return (XIok);
    }
    catch (XIrt ret) {
        *zp = 0;
        Ylist::destroy(yl);
        return (ret);
    }
}


// Static function.
// On success, return (zl1 & !ZB) in zl1p.  No interrupt check.
//
XIrt
Zlist::zl_andnot(Zlist **zl1p, const Zoid *ZB)
{
    if (!*zl1p || !ZB)
        return (XIok);

    if (!(*zl1p)->next) {
        bool no_ovl;
        Zlist *zn = (*zl1p)->Z.clip_out(ZB, &no_ovl);
        if (zn) {
            delete *zl1p;
            *zl1p = zn;
            return (XIok);
        }
        if (no_ovl)
            return (XIok);
        delete *zl1p;
        *zl1p = 0;
        return (XIok);
    }

    Ylist *y = new Ylist(*zl1p);
    *zl1p = Ylist::to_zlist(Ylist::clip_out_zoid(y, ZB));
    return (XIok);
}


// Static function.
// On success, return (zl1 & !zl2) in zl1p, zl2 is freed.
// On exception: *zl1p and zl2 are freed;
//
XIrt
Zlist::zl_andnot(Zlist **zl1p, Zlist *zl2)
{
    TimeDbgAccum ac("zl_andnot");

    if (!*zl1p || !zl2) {
        Zlist::destroy(zl2);
        return (XIok);
    }
    if (!(*zl1p)->next && !zl2->next) {
        Zlist *zo = *zl1p;
        bool no_ovl;
        *zl1p = (zo->Z).clip_out(&zl2->Z, &no_ovl);
        if (no_ovl)
            *zl1p = zo;
        else
            Zlist::destroy(zo);
        Zlist::destroy(zl2);
        return (XIok);
    }

    Ylist *yl = new Ylist(*zl1p);
    if (!zl2->next) {
        *zl1p = Ylist::to_zlist(Ylist::clip_out_zoid(yl, &zl2->Z));
        return (XIok);
    }

    Ylist *yr = new Ylist(zl2);
#ifdef SCLDEBUG
    if (GEO()->useSclFuncs()) {
        try {
            *zl1p = yl->scl_clip_out(yr)->to_zlist();
            return (XIok);
        }
        catch (XIrt ret) {
            *zl1p = 0;
            return (ret);
        }
    }
#endif
    try {
        *zl1p = Ylist::clip_out_ylist(yl, yr);
        Ylist::destroy(yl);
        Ylist::destroy(yr);
        return (XIok);
    }
    catch (XIrt ret) {
        *zl1p = 0;
        Ylist::destroy(yl);
        Ylist::destroy(yr);
        return (ret);
    }
}


// Static function.
// On success, return (zl1 & !yr) in zl1p.
// On exception: *zl1p is freed;
//
XIrt
Zlist::zl_andnot(Zlist **zlp, const Ylist *yr)
{
    TimeDbgAccum ac("zl_andnot_y");

    if (!*zlp || !yr || (!yr->y_zlist && !yr->next))
        return (XIok);
    Ylist *yl = new Ylist(*zlp);

#ifdef SCLDEBUG
    if (GEO()->useSclFuncs()) {
        try {
            *zlp = yl->scl_clip_out(yr->copy())->to_zlist();
            return (XIok);
        }
        catch (XIrt ret) {
            *zlp = 0;
            return (ret);
        }
    }
#endif
    try {
        *zlp = Ylist::clip_out_ylist(yl, yr);
        Ylist::destroy(yl);
        return (XIok);
    }
    catch (XIrt ret) {
        *zlp = 0;
        Ylist::destroy(yl);
        return (ret);
    }
}


// Static function.
// On success, return (zl1 & !zl2) in zl1p, (zl2 & !zl1) in zl2p.
// On exception: *zl1p and *zl2p are freed;
//
XIrt
Zlist::zl_andnot2(Zlist **zl1p, Zlist **zl2p)
{
    TimeDbgAccum ac("zl_andnot2");

    if (!*zl1p || !*zl2p)
        return (XIok);
    Ylist *yl = new Ylist(*zl1p);
    Ylist *yr = new Ylist(*zl2p);
    Ylist::remove_common(&yl, &yr);

#ifdef SCLDEBUG
    if (GEO()->useSclFuncs()) {
        try {
            *zl1p = yl->scl_clip_out2(&yr)->to_zlist();
            *zl2p = yr->to_zlist();
            return (XIok);
        }
        catch (XIrt ret) {
            *zl1p = 0;
            *zl2p = 0;
            return (ret);
        }
    }
#endif
    try {
        *zl2p = Ylist::clip_out_ylist(yr, yl);
        *zl1p = Ylist::clip_out_ylist(yl, yr);
        Ylist::destroy(yl);
        Ylist::destroy(yr);
        return (XIok);
    }
    catch (XIrt ret) {
        *zl1p = 0;
        *zl2p = 0;
        Ylist::destroy(yl);
        Ylist::destroy(yr);
        return (ret);
    }
}


// Static function.
// Return the ex-or in zl1p.  Both sources are consumed.
//
XIrt
Zlist::zl_xor(Zlist **zl1p, Zlist *zl2)
{
    TimeDbgAccum ac("zl_xor");

#ifdef SCLDEBUG
    if (GEO()->useSclFuncs()) {
        Ylist *yl1 = new Ylist(*zl1p);
        Ylist *yl2 = new Ylist(zl2);
        try {
            *zl1p = yl1->scl_clip_xor(yl2)->to_zlist();
            return (XIok);
        }
        catch (XIrt ret) {
            *zl1p = 0;
            return (ret);
        }
    }
#endif
    Zlist *zl1 = *zl1p;
    XIrt ret = zl_andnot2(&zl1, &zl2);
    if (ret != XIok)
        return (ret);
    if (zl1) {
        if (zl2) {
            Zlist *ze = zl1;
            while (ze->next)
                ze = ze->next;
            ze->next = zl2;
        }
    }
    else
        zl1 = zl2;
    *zl1p = zl1;
    return (XIok);
}


// Static function.
// On succes, return bloated zoid list in *zp.
// On exception: *zp is freed.
//
XIrt
Zlist::zl_bloat(Zlist **zp, int delta, int mode)
{
    Zlist *zl = *zp;
    try {
        *zp = Zlist::bloat(zl, delta, mode);
        Zlist::destroy(zl);
        return (XIok);
    }
    catch (XIrt ret) {
        *zp = 0;
        Zlist::destroy(zl);
        return (ret);
    }
}


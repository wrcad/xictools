
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

#include "geo.h"
#include "geo_memmgr.h"
#include "geo_memmgr_cfg.h"


//-----------------------------------------------------------------------------
// GEOmmgr:
// Memory management for RTelem, Zlist, Ylist, Blist, BYlist.
//-----------------------------------------------------------------------------


cGEOmmgr *cGEOmmgr::instancePtr = 0;

cGEOmmgr::cGEOmmgr() :
    RTelem_db("RTelem"), Zlist_db("Zlist"), Ylist_db("Ylist"),
    Blist_db("Blist"), BYlist_db("BYlist"), GEOblock_db("GEOblock")
{
    if (instancePtr) {
        fprintf(stderr, "Singleton class cGEOmmgr already instantiated.\n");
        exit (1);
    }
    instancePtr = this;
}


// Function to print allocation statistics.
//
void
cGEOmmgr::stats(int *inuse, int *not_inuse)
{
    mmgrstat_t st;

    RTelem_db.stats(&st);
    st.print("RTelem", stdout);
    if (inuse)
        *inuse += st.inuse*st.strsize;
    if (not_inuse)
        *not_inuse += st.not_inuse*st.strsize;

    Zlist_db.stats(&st);
    st.print("Zlist", stdout);
    if (inuse)
        *inuse += st.inuse*st.strsize;
    if (not_inuse)
        *not_inuse += st.not_inuse*st.strsize;

    Ylist_db.stats(&st);
    st.print("Ylist", stdout);
    if (inuse)
        *inuse += st.inuse*st.strsize;
    if (not_inuse)
        *not_inuse += st.not_inuse*st.strsize;

    Blist_db.stats(&st);
    st.print("Blist", stdout);
    if (inuse)
        *inuse += st.inuse*st.strsize;
    if (not_inuse)
        *not_inuse += st.not_inuse*st.strsize;

    BYlist_db.stats(&st);
    st.print("BYlist", stdout);
    if (inuse)
        *inuse += st.inuse*st.strsize;
    if (not_inuse)
        *not_inuse += st.not_inuse*st.strsize;

    GEOblock_db.stats(&st);
    st.print("GEOblock", stdout);
    if (inuse)
        *inuse += st.inuse*st.strsize;
    if (not_inuse)
        *not_inuse += st.not_inuse*st.strsize;
}


//-----------------------------------------------------------------------------
// Allocation operators.

#ifdef GEO_USE_MANAGER

void *
RTelem::operator new(size_t)
    { return (GEOmmgr()->new_RTelem()); }

// This has to catch all derived classes.
//
void
RTelem::operator delete(void *p, size_t)
{
    if (!p)
        return;

    RTelem *e = (RTelem*)p;
    if (!e->e_type) {
        // Non-leaf.
        GEOmmgr()->del_RTelem(e);
        return;
    }
    int typecode = e->e_type - 'a';
    if (typecode >= 0 && typecode < 26) {
        if (e_delete_funcs[typecode])
            (*e_delete_funcs[typecode])(e);
    }
}

void *
Zlist::operator new(size_t)
    { return (GEOmmgr()->new_Zlist()); }

void
Zlist::operator delete(void *p, size_t)
    { if (p) GEOmmgr()->del_Zlist((Zlist*)p); }

void *
Ylist::operator new(size_t)
    { return (GEOmmgr()->new_Ylist()); }

void
Ylist::operator delete(void *p, size_t)
    { if (p) GEOmmgr()->del_Ylist((Ylist*)p); }

void *
Blist::operator new(size_t)
    { return (GEOmmgr()->new_Blist()); }

void
Blist::operator delete(void *p, size_t)
    { if (p) GEOmmgr()->del_Blist((Blist*)p); }

void *
BYlist::operator new(size_t)
    { return (GEOmmgr()->new_BYlist()); }

void
BYlist::operator delete(void *p, size_t)
    { if (p) GEOmmgr()->del_BYlist((BYlist*)p); }

void *
GEOblock::operator new(size_t)
    { return (GEOmmgr()->new_GEOblock()); }

void
GEOblock::operator delete(void *p, size_t)
    { if (p) GEOmmgr()->del_GEOblock((GEOblock*)p); }

#endif


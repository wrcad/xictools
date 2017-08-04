
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

#ifndef CD_INSTANCE_H
#define CD_INSTANCE_H

#include "cd_memmgr_cfg.h"


// Instance array parameters
struct CDap
{
    inline CDap(const CDc* = 0);

    CDap(unsigned int a, unsigned int b, int c, int d)
        {
            nx = a;
            ny = b;
            dx = c;
            dy = d;
        }

    bool operator==(const CDap &c) const
        {
            if (nx != c.nx || ny != c.ny)
                return (false);
            if (nx > 1 && dx != c.dx)
                return (false);
            if (ny > 1 && dy != c.dy)
                return (false);
            return (true);
        }

    bool operator!=(const CDap &c) const
        {
            if (nx != c.nx || ny != c.ny)
                return (true);
            if (nx > 1 && dx != c.dx)
                return (true);
            if (ny > 1 && dy != c.dy)
                return (true);
            return (false);
        }

    void scale(double s)
        {
            if (s != 1.0) {
                dx = mmRnd(dx*s);
                dy = mmRnd(dy*s);
            }
        }

    unsigned int nx, ny;
    int dx, dy;
};


// Cell instance attributes
struct CDattr : public CDxf
{
    inline CDattr(const CDc* = 0, const CDap* = 0);

    CDattr(const CDtx *tx, const CDap *ap)
        {
            if (tx) {
                magn = tx->magn;
                ax = tx->ax;
                ay = tx->ay;
                refly = tx->refly;
            }
            else {
                magn = 1.0;
                ax = 1;
                ay = 0;
                refly = false;
            }
            if (ap) {
                nx = ap->nx;
                ny = ap->ny;
                dx = ap->dx;
                dy = ap->dy;
            }
            else {
                nx = ny = 1;
                dx = dy = 0;
            }
        }

    bool operator==(const CDattr &c) const
        {
            if (nx != c.nx || ny != c.ny)
                return (false);
            if (ax != c.ax || ay != c.ay || refly != c.refly)
                return (false);
            if (magn != c.magn)
                return (false);
            if (nx > 1 && dx != c.dx)
                return (false);
            if (ny > 1 && dy != c.dy)
                return (false);
            return (true);
        }

    bool operator!=(const CDattr &c) const
        {
            if (nx != c.nx || ny != c.ny)
                return (true);
            if (ax != c.ax || ay != c.ay || refly != c.refly)
                return (true);
            if (magn != c.magn)
                return (true);
            if (nx > 1 && dx != c.dx)
                return (true);
            if (ny > 1 && dy != c.dy)
                return (true);
            return (false);
        }

    double magn;            // magnification
    unsigned int nx, ny;    // array cols/rows
    int dx, dy;             // array spacing
};


// Call desc
// In order to save a few bytes per instance, the CDattr database is
// used to save common copies of the instance transforms.  The
// transform is accessed with the cAttr member.
//
struct CDc : public CDo
{
    // ptable_t necessities
    CDc *ptab_next() const      { return(cPtabNext); }
    void set_ptab_next(CDc *c)  { cPtabNext = c; }
    CDc *tgen_next(bool) const  { return (cPtabNext); }

#ifdef CD_USE_MANAGER
    void *operator new(size_t);
#endif

    CDc(CDl *ld) : CDo(ld)
        {
            e_type = CDINSTANCE;
            cAttr = NULL_TICKET;
            cMaster = 0;
            cPtabNext = 0;
            cX = cY = 0;
        }

    // destruction
    void cleanup()
        {
            unlinkFromMaster(false);
            if (cMaster && !cMaster->hasUnlinked() &&
                    !cMaster->hasInstances()) {
                // this is the last of the linking objects
                cMaster->unlink();  // unlink from parent
                if (cMaster->celldesc())
                    cMaster->unlinkRef();
                delete cMaster;
            }
        }

    void get_tx(CDtx *tx) const
        {
            tx->tx = cX;
            tx->ty = cY;
            CDattr a;
            CD()->FindAttr(cAttr, &a);
            tx->magn = a.magn;
            tx->ax = a.ax;
            tx->ay = a.ay;
            tx->refly = a.refly;
        }

    void get_ap(CDap *ap) const
        {
            CDattr a;
            CD()->FindAttr(cAttr, &a);
            ap->nx = a.nx;
            ap->ny = a.ny;
            ap->dx = a.dx;
            ap->dy = a.dy;
        }

    void get_attr(CDattr *at)   const { CD()->FindAttr(cAttr, at); }

    void linkIntoMaster()
        {
            if (cMaster) {
                cMaster->unlinkCdesc(this, true);
                cMaster->linkCdesc(this, false);
            }
        }

    void unlinkFromMaster(bool addunl)
        {
            if (cMaster) {
                cMaster->unlinkCdesc(this, false);
                if (addunl)
                    cMaster->linkCdesc(this, true);
                else
                    cMaster->unlinkCdesc(this, true);
            }
        }

    void setBadBB()
        {
            e_BB.left = e_BB.right = posX();
            e_BB.bottom = e_BB.top = posY();
        }

    // This will return the symbolic rep if any, unless the argument
    // is true.
    CDs *masterCell(bool owner = false) const
        {
            if (!cMaster)
                return (0);
            CDs *sd = cMaster->celldesc();
            if (!sd || owner || !sd->isElectrical())
                return (sd);
            CDs *tsd = sd->symbolicRep(this);
            if (tsd)
                return (tsd);
            return (sd);
        }

    CDs *parent()   const { return (cMaster ? cMaster->parent() : 0); }

    CDcellName cellname() const
        {
            if (cMaster && cMaster->celldesc())
                return (cMaster->celldesc()->cellname());
            return (0);
        }

    bool isDevice() const
        {
            if (cMaster && cMaster->celldesc())
                return (cMaster->celldesc()->isDevice());
            return (false);
        }

    CDm *master()       const   { return (cMaster); }
    void setMaster(CDm *m)      { cMaster = m; }

    int posX()          const   { return (cX); }
    void setPosX(int x)         { cX = x; }
    int posY()          const   { return (cY); }
    void setPosY(int y)         { cY = y; }

    ticket_t attr()     const   { return (cAttr); }
    void setAttr(ticket_t t)    { cAttr = t; }

    // cd_hash.cc
    unsigned int add_hash(unsigned int);

    // cd_instance.cc
    void call(CallDesc*) const;
    void setTransform(const CDtf*, const CDap* = 0);
    void prptyAddStruct(bool = false);
    CDelecCellType elecCellType(const char** = 0);
    const char *getBaseName(const CDp_name* = 0) const;
    char *getInstName(unsigned int) const;
    bool nameOK(const char*) const;
    CDc *findElecDualOfPhys(int*, unsigned int, unsigned int) const;
    CDc *findPhysDualOfElec(int, unsigned int*, unsigned int*) const;
    void updateDeviceName(unsigned int);
    void updateTerminals(int*);
    void updateTermNames();
    bool updateBB();
    void addSymbChangeBB(BBox*);

private:
    ticket_t cAttr;     // Attributes ticket.
    CDc *cPtabNext;     // Private link for ptable_t.
    CDm *cMaster;       // Pointer to master list descriptor.
    int cX, cY;         // Origin.
};


// Iterator class for CDc contained in CDm.
//
struct CDc_gen : public tgen_t<CDc>
{
    CDc_gen(CDm *m, bool unl = false) :
        tgen_t<CDc>(unl ? (m ? m->unlinked() : 0) :
            (m ? m->instances() : 0), true) { }

    CDc *c_first() { return (next()); }
    CDc *c_next() { return (next()); }
};


// List element for CDc.
//
struct CDcl
{
#ifdef CD_USE_MANAGER
    void *operator new(size_t);
    void operator delete(void*, size_t);
#endif
    CDcl()
        {
            next = 0;
            cdesc = 0;
        }

    CDcl(const CDc *c, CDcl *n)
        {
            next = n;
            cdesc = c;
        }

    static void destroy(CDcl *cl)
        {
            while (cl) {
                CDcl *cx = cl;
                cl = cl->next;
                delete cx;
            }
        }

    // Unlink which from list, return a pointer to it if found.
    //
    static CDcl *unlink(CDcl **list, const CDc *which)
        {
            if (!list)
                return (0);
            CDcl *p = 0;
            for (CDcl *c = *list; c; c = c->next) {
                if (c->cdesc == which) {
                    if (!p)
                        *list = c->next;
                    else
                        p->next = c->next;
                    return (c);
                }
                p = c;
            }
            return (0);
        }

    // cd_instance.cc
    static void sort_instances(CDcl*);

    CDcl *next;
    const CDc *cdesc;
};

// A cdesc plus x-y indices for use in instance arrays, exported for
// use as a stack element.
//
struct CDcxy
{
    // No constructor/destructor.
    const CDc *cdesc;
    unsigned int xind, yind;
};

// List element for CDc, plus array element indices.  Convenience
// export.
//
struct CDclxy : public CDcxy
{
    CDclxy(const CDc *c, CDclxy *n = 0, int x = 0, int y = 0)
        {
            cdesc = c;
            xind = x;
            yind = y;
            next = n;
        }

    static void destroy(CDclxy *c)
        {
            while (c) {
                CDclxy *cx = c;
                c = c->next;
                delete cx;
            }
        }

    CDclxy *next;
};


// Deferred inlines

inline
CDtx::CDtx(const CDc *cdesc)
{
    if (cdesc)
        cdesc->get_tx(this);
    else
        clear();
}


inline
CDap::CDap(const CDc *cd)
{
    if (cd)
        cd->get_ap(this);
    else {
        nx = ny = 1;
        dx = dy = 0;
    }
}


inline
CDattr::CDattr(const CDc *cd, const CDap *ap)
{
    magn = 1.0;
    nx = ny = 1;
    dx = dy = 0;
    if (ap) {
        nx = ap->nx;
        ny = ap->ny;
        dx = ap->dx;
        dy = ap->dy;
    }
    ax = 1;
    ay = 0;
    refly = false;
    if (cd)
        cd->get_attr(this);
}

#endif


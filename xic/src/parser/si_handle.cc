
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

#include "config.h"
#include "cd.h"
#include "fio.h"
#include "fio_assemble.h"
#include "fio_oasis.h"
#include "si_parsenode.h"
#include "si_handle.h"
#include "si_parser.h"

#include <unistd.h>
#ifdef WIN32
#include <winsock2.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#endif


// The WIN32 defines are for the MINGW version of gcc running on Windows

#ifdef WIN32
#define CLOSESOCKET(x) (shutdown(x, SD_SEND), closesocket(x))
#else
#define CLOSESOCKET(x) ::close(x)
#endif

////////////////////////////////////////////////////////////////////////
//
// Handle Support
//
////////////////////////////////////////////////////////////////////////

// Table of open handles
//
SymTab *sHdl::HdlTable;

// The handle indexing starts with 1001, except for file descriptors
// which are indexed by the actual value
//
int sHdl::HdlIndex = 1000;


// Constructor: add to the symbol table.
//
sHdl::sHdl(void *d, int hd)
{
    type = HDLgeneric;
    data = d;
    if (!HdlTable)
        HdlTable = new SymTab(false, false);
    if (hd) {
        id = hd;
        HdlTable->add(hd, this, false);
    }
    else {
        id = ++HdlIndex;
        HdlTable->add(HdlIndex, this, false);
    }
}


// Destructor: remove from the symbol table.
//
sHdl::~sHdl()
{
    if (HdlTable)
        HdlTable->remove(id);
}


// Static function.
// Function to resolve a handle.
//
sHdl *
sHdl::get(int hd)
{
    if (!HdlTable)
        return (0);
    sHdl *hdl = (sHdl*)SymTab::get(HdlTable, hd);
    if (hdl == (sHdl*)ST_NIL)
        return (0);
    return (hdl);
}


// Static function.
// Yecch!  Search through the table for references to olddesc, which
// is being deleted, and replace with newdesc or remove.
//
void
sHdl::update(CDo *olddesc, CDo *newdesc)
{
    if (HdlTable) {
        SymTabGen gen(HdlTable);
        SymTabEnt *h;
        while ((h = gen.next()) != 0) {
            sHdl *hdl = (sHdl*)h->stData;
            if (hdl->type == HDLobject) {
                if (((sHdlObject*)hdl)->copies)
                    continue;
                CDol *ol = (CDol*)hdl->data;
                CDol *op = 0, *on;
                for ( ; ol; ol = on) {
                    on = ol->next;
                    if (ol->odesc == olddesc) {
                        if (newdesc)
                            ol->odesc = newdesc;
                        else {
                            if (!op)
                                hdl->data = on;
                            else
                                op->next = on;
                            delete ol;
                            continue;
                        }
                    }
                    op = ol;
                }
            }
            else if (hdl->type == HDLprpty) {
                CDo *odesc = ((sHdlPrpty*)hdl)->odesc;
                if (odesc == olddesc) {
                    ((sHdlPrpty*)hdl)->odesc = newdesc;
                    hdl->data = 0;
                    if (newdesc)
                        hdl->data = prp_list(newdesc->prpty_list());
                }
            }
        }
    }
}


// Static function.
// Yecch!  Search through the table for references to oldp, which
// is being deleted, and replace with newp or remove.
//
void
sHdl::update(CDo *odesc, CDp *oldp, CDp *newp)
{
    if (HdlTable) {
        SymTabGen gen(HdlTable);
        SymTabEnt *h;
        while ((h = gen.next()) != 0) {
            sHdl *hdl = (sHdl*)h->stData;
            if (hdl->type == HDLprpty) {
                if (odesc == ((sHdlPrpty*)hdl)->odesc) {
                    CDpl *pl = (CDpl*)hdl->data;
                    CDpl *pp = 0, *pn;
                    for ( ; pl; pl = pn) {
                        pn = pl->next;
                        if (pl->pdesc == oldp) {
                            if (newp)
                                pl->pdesc = newp;
                            else {
                                if (!pp)
                                    hdl->data = pn;
                                else
                                    pp->next = pn;
                                delete pl;
                                continue;
                            }
                        }
                        pp = pl;
                    }
                }
            }
        }
    }
}


// Static function.
// When the cell desc is cleared, delete all of the handles that would
// be affected.
//
void
sHdl::update(CDs *sd)
{
    if (HdlTable) {
        SymTabGen gen(HdlTable);
        SymTabEnt *h;
        while ((h = gen.next()) != 0) {
            sHdl *hdl = (sHdl*)h->stData;
            if (hdl->type == HDLobject) {
                if (((sHdlObject*)hdl)->sdesc == sd &&
                        !((sHdlObject*)hdl)->copies)
                    hdl->close(hdl->id);
            }
            else if (hdl->type == HDLprpty) {
                if (((sHdlPrpty*)hdl)->sdesc == sd)
                    hdl->close(hdl->id);
            }
            else if (hdl->type == HDLdevice || hdl->type == HDLdcontact ||
                    hdl->type == HDLsubckt || hdl->type == HDLscontact) {
                cGroupDesc *gd = ((sHdlDevice*)hdl)->gdesc;
                if (gd && SIparse()->ifGetCellFromGroupDesc(gd) == sd)
                    hdl->close(hdl->id);
            }
        }
    }
}


// Static function.
// When the group desc is cleared, delete all of the handles that would
// be affected.
//
void
sHdl::update(cGroupDesc *g)
{
    if (HdlTable) {
        SymTabGen gen(HdlTable);
        SymTabEnt *h;
        while ((h = gen.next()) != 0) {
            sHdl *hdl = (sHdl*)h->stData;
            if (hdl->type == HDLdevice || hdl->type == HDLdcontact ||
                    hdl->type == HDLsubckt || hdl->type == HDLscontact) {
                if (((sHdlDevice*)hdl)->gdesc == g)
                    hdl->close(hdl->id);
            }
        }
    }
}


// Static function.
// Close all handles that reference cells or cell data.
//
void
sHdl::update()
{
    if (HdlTable) {
        SymTabGen gen(HdlTable);
        SymTabEnt *h;
        while ((h = gen.next()) != 0) {
            sHdl *hdl = (sHdl*)h->stData;
            switch (hdl->type) {
            case HDLobject:
                if (!((sHdlObject*)hdl)->copies)
                    hdl->close(hdl->id);
                break;
            case HDLprpty:
            case HDLdcontact:
            case HDLsubckt:
            case HDLscontact:
            case HDLgen:
                hdl->close(hdl->id);
            default:
                break;
            }
        }
    }
}


// Static function.
// Function to clear and free the table.
//
void
sHdl::clear()
{
    if (HdlTable) {
        SymTab *st = HdlTable;
        HdlTable = 0;  // avoid calling remove() from ~sHdl()
        SymTabGen gen(st, true);
        SymTabEnt *h;
        while ((h = gen.next()) != 0) {
            sHdl *hdl = (sHdl*)h->stData;
            h->stData = 0;
            delete hdl;
            delete h;
        }
        delete st;
    }
}


// Static function.
// Convenience function to create a CDpl from a property list.
//
CDpl *
sHdl::prp_list(CDp *prpty_list)
{
    CDpl *p0 = 0, *pe = 0;
    for (CDp *p = prpty_list; p; p = p->next_prp()) {
        if (!p0)
            p0 = pe = new CDpl(p, 0);
        else {
            pe->next = new CDpl(p, 0);
            pe = pe->next;
        }
    }
    return (p0);
}


// Static function.
// Convenience function to create a CDol from a selection list.
//
CDol *
sHdl::sel_list(CDol *select_list)
{
    CDol *o0 = 0, *oe = 0;
    for (CDol *s = select_list; s; s = s->next) {
        if (!s->odesc)
            continue;
        if (!o0)
            o0 = oe = new CDol(s->odesc, 0);
        else {
            oe->next = new CDol(s->odesc, 0);
            oe = oe->next;
        }
    }
    return (o0);
}


// Static function.
// Create and return a hash table for uniqueness testing.
//
sHdlUniq *
sHdl::new_uniq(sHdl *hdl)
{
    sHdlUniq *hu = new sHdlUniq(hdl);
    if (!hu->active()) {
        delete hu;
        return (0);
    }
    return (hu);
}
// End of sHdl functions.


sHdlUniq::sHdlUniq(sHdl *hdl)
{
    hu_tab = 0;
    hu_type = HDLgeneric;
    hu_copies = false;
    if (hdl) {
        switch (hdl->type) {
        case HDLstring:
        case HDLobject:
        case HDLprpty:
        case HDLnode:
        case HDLterminal:
        case HDLdevice:
        case HDLdcontact:
        case HDLsubckt:
        case HDLscontact:
            hu_type = hdl->type;
            break;
        default:
            return;
        }
        hu_tab = new SymTab(false, false);
        if (hu_type == HDLstring) {
            stringlist *sl = (stringlist*)hdl->data;
            for (stringlist *s = sl; s; s = s->next)
                hu_tab->add(sl->string, 0, true);
        }
        else if (hu_type == HDLobject) {
            hu_copies = ((sHdlObject*)hdl)->copies;
            CDol *ol = (CDol*)hdl->data;
            for (CDol *o = ol; o; o = o->next)
                hu_tab->add((uintptr_t)o->odesc, 0, true);
        }
        else if (hu_type == HDLprpty) {
            CDpl *pl = (CDpl*)hdl->data;
            for (CDpl *p = pl; p; p = p->next)
                hu_tab->add((uintptr_t)p->pdesc, 0, true);
        }
        else {
            tlist<void> *t0 = (tlist<void>*)hdl->data;
            for (tlist<void>*t = t0; t; t = t->next)
                hu_tab->add((uintptr_t)t->elt, 0, true);
        }
    }
}


sHdlUniq::~sHdlUniq()
{
    delete hu_tab;
}


// Return true if the object associated with hdl is in the table.
//
bool
sHdlUniq::test(sHdl *hdl)
{
    if (!hdl || !hu_tab)
        return (false);
    switch (hdl->type) {
    case HDLstring:
    case HDLobject:
    case HDLprpty:
    case HDLnode:
    case HDLterminal:
    case HDLdevice:
    case HDLdcontact:
    case HDLsubckt:
    case HDLscontact:
        if (hu_type == hdl->type)
            break;
        // fallthrough
    default:
        return (false);
    }
    if (hu_type == HDLstring) {
        stringlist *sl = (stringlist*)hdl->data;
        if (sl && SymTab::get(hu_tab, sl->string) == 0)
            return (true);
    }
    else if (hu_type == HDLobject) {
        if (hu_copies == ((sHdlObject*)hdl)->copies) {
            CDol *ol = (CDol*)hdl->data;
            if (ol && SymTab::get(hu_tab, (uintptr_t)ol->odesc) == 0)
                return (true);
        }
    }
    else if (hu_type == HDLprpty) {
        CDpl *pl = (CDpl*)hdl->data;
        if (pl && SymTab::get(hu_tab, (uintptr_t)pl->pdesc) == 0)
            return (true);
    }
    else {
        tlist<void> *t0 = (tlist<void>*)hdl->data;
        if (t0 && SymTab::get(hu_tab, (uintptr_t)t0->elt) == 0)
            return (true);
    }
    return (false);
}
// End of sHdlUniq functions.


sHdlString::~sHdlString()
{
    stringlist::destroy((stringlist*)data);
}

// Iterator for strings: return the string (the string is *always* a copy
// and eventually needs to be freed.
//
void *
sHdlString::iterator()
{
    stringlist *s = (stringlist*)data;
    char *str = 0;
    if (s) {
        data = s->next;
        str = s->string;
        s->string = 0;
        delete s;
    }
    if (!data)
        delete this;
    return (str);
}
// End of sHdlString functions.


sHdlObject::~sHdlObject()
{
    CDol *ol = (CDol*)data;
    while (ol) {
        CDol *on = ol->next;
        if (copies)
            delete ol->odesc;
        delete ol;
        ol = on;
    }
    data = 0;
}

// Iterator for objects: return the object, which might be a copy and
// need to be freed later.
//
void *
sHdlObject::iterator()
{
    CDol *ol = (CDol*)data;
    CDo *obj = 0;
    if (ol) {
        data = ol->next;
        obj = ol->odesc;
        delete ol;
    }
    if (!data)
        delete this;
    return (obj);
}
// End of sHdlObject functions.


sHdlFd::~sHdlFd()
{
    delete [] (char*)data;
}

int
sHdlFd::close(int hd)
{
    int rt;
    if (fp)
        rt = pclose(fp);
    else if (data)
        rt = ::close(hd);
    else
        rt = CLOSESOCKET(hd);
    delete this;
    return (rt);
}
// End of sHdlFd functions.


sHdlPrpty::~sHdlPrpty()
{
    CDpl::destroy((CDpl*)data);
}

void *
sHdlPrpty::iterator()
{
    CDpl *prp = (CDpl*)data;
    CDp *prpty = 0;
    if (prp) {
        data = prp->next;
        prpty = prp->pdesc;
        delete prp;
    }
    if (!data)
        delete this;
    return (prpty);
}
// End of sHdlPrpty functions.


sHdlNode::~sHdlNode()
{
    tlist<CDp_nodeEx>::destroy((tlist<CDp_nodeEx>*)data);
}

void *
sHdlNode::iterator()
{
    tlist<CDp_nodeEx> *dv = (tlist<CDp_nodeEx>*)data;
    CDp_nodeEx *di = 0;
    if (dv) {
        data = dv->next;
        di = dv->elt;
        delete dv;
    }
    if (!data)
        delete this;
    return (di);
}
// End of sHdlNode functions.


sHdlTerminal::~sHdlTerminal()
{
    tlist<CDterm>::destroy((tlist<CDterm>*)data);
}

void *
sHdlTerminal::iterator()
{
    tlist<CDterm> *dv = (tlist<CDterm>*)data;
    CDterm *di = 0;
    if (dv) {
        data = dv->next;
        di = dv->elt;
        delete dv;
    }
    if (!data)
        delete this;
    return (di);
}
// End of sHdlTerminal functions.


sHdlDevice::~sHdlDevice()
{
    tlist<sDevInst>::destroy((tlist<sDevInst>*)data);
}

void *
sHdlDevice::iterator()
{
    tlist<sDevInst> *dv = (tlist<sDevInst>*)data;
    sDevInst *di = 0;
    if (dv) {
        data = dv->next;
        di = dv->elt;
        delete dv;
    }
    if (!data)
        delete this;
    return (di);
}
// End of sHdlDevice functions.


sHdlDevContact::~sHdlDevContact()
{
    tlist<sDevContactInst>::destroy((tlist<sDevContactInst>*)data);
}

void *
sHdlDevContact::iterator()
{
    tlist<sDevContactInst> *dv = (tlist<sDevContactInst>*)data;
    sDevContactInst *di = 0;
    if (dv) {
        data = dv->next;
        di = dv->elt;
        delete dv;
    }
    if (!data)
        delete this;
    return (di);
}
// End of sHdlDevContact functions.


sHdlSubckt::~sHdlSubckt()
{
    tlist<sSubcInst>::destroy((tlist<sSubcInst>*)data);
}

void *
sHdlSubckt::iterator()
{
    tlist<sSubcInst> *dv = (tlist<sSubcInst>*)data;
    sSubcInst *di = 0;
    if (dv) {
        data = dv->next;
        di = dv->elt;
        delete dv;
    }
    if (!data)
        delete this;
    return (di);
}
// End of sHdlSubckt functions.


sHdlSubcContact::~sHdlSubcContact()
{
    tlist<sSubcContactInst>::destroy((tlist<sSubcContactInst>*)data);
}

void *
sHdlSubcContact::iterator()
{
    tlist<sSubcContactInst> *dv = (tlist<sSubcContactInst>*)data;
    sSubcContactInst *di = 0;
    if (dv) {
        data = dv->next;
        di = dv->elt;
        delete dv;
    }
    if (!data)
        delete this;
    return (di);
}
// End of sHdlSubcContact functions.


sHdlGen::~sHdlGen()
{
    delete (sPF*)data; delete rec;
}

void *
sHdlGen::iterator()
{
    for (;;) {
        sPF *gen = (sPF*)data;
        if (gen) {
            CDo *od = gen->next((rec->depth == 0), false);
            if (od) {
                CDol *ol = new CDol(od, 0);
                sHdl *h = new sHdlObject(ol, sdesc, (rec->depth != 0));
                return ((void*)(uintptr_t)h->id);
            }
            CDl *ld = 0;
            while (!ld && rec->names) {
                ld = CDldb()->findLayer(rec->names->string, rec->mode);
                stringlist *sn = rec->names->next;
                delete [] rec->names->string;
                delete rec->names;
                rec->names = sn;
            }
            if (ld) {
                if (gen->reinit(sdesc, &rec->AOI, ld, rec->depth))
                    continue;
            }
        }
        break;
    }
    delete this;
    return (0);
}
// End of sHdlGen functions.


sHdlAjob::~sHdlAjob()
{
    delete (ajob_t*)data;
}


sHdlBstream::~sHdlBstream()
{
    delete (cv_incr_reader*)data;
}
// End of sHdlAjob functions.


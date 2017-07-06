
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
 $Id: cd_objmisc.cc,v 5.43 2016/06/17 19:08:40 stevew Exp $
 *========================================================================*/

#include "cd.h"
#include "cd_types.h"
#include "cd_propnum.h"
#include "cd_terminal.h"
#include "cd_netname.h"
#include "cd_hypertext.h"
#include "fio_gencif.h"


//
// Misc functions for CDo and derivatives.
//

FlagDef OdescFlags[] =
{
    { "MergeDeleted", CDmergeDeleted, false, "Deleted by object merging" },
    { "MergeCreated", CDmergeCreated, false, "Created by object merging" },
    { "NoDRC", CDnoDRC, true, "Skip DRC on this object" },
    { "Expand", CDexpand, false, "Show cell expanded" },
    { "InQueue", CDinqueue, false, "Object is in selection queue" },
    { "NoMerge", CDnoMerge, false, "Object will not be merged" },
    { "IsCopy", CDisCopy, false, "Object is a copy (not in database)" },
    { 0, 0, false, 0 }
};

//-----------------------------------------------------------------------------
// Box (base class for objects)
//

// When an object is in the RTree database, one should never
// change the ldesc or BB values.  However, with copies or
// on-stack temporaries, this is ok.
//
void
CDo::set_oBB(const BBox &tBB)
{
    if (!is_copy() && in_db()) {
        CD()->ifInfoMessage(IFMSG_POP_ERR,
    "Internal error: attempt to change bounding box of object in database.");
        return;
    }
    e_BB = tBB;
}


void
CDo::set_ldesc(CDl *ld)
{
    if (!is_copy() && in_db()) {
        CD()->ifInfoMessage(IFMSG_POP_ERR,
            "Internal error: attempt to change layer of object in database.");
        return;
    }
    e_children = (RTelem*)ld;
}


// Add a copy of pdesc to CDo, return pointer to copy.
//
CDp *
CDo::prptyAddCopy(CDp *pdesc)
{
    if (!pdesc)
        return (0);
    CDp *pcopy = pdesc->dup();
    if (!pcopy)
        return (0);

    // Remove existing properties that shouldn't be duplicated.
    if (type() == CDINSTANCE &&
            (pdesc->value() == XICP_PC || pdesc->value() == XICP_PC_PARAMS))
        prptyRemove(pdesc->value());

    link_prpty_list(pcopy);
    return (pcopy);
}


// Add a copy of the entire prpty list in pdesc to CDo.
//
void
CDo::prptyAddCopyList(CDp *pdesc)
{
    bool new_pc = false;
    bool new_pc_params = false;
    CDp *pp = 0, *p0 = 0;
    for ( ; pdesc; pdesc = pdesc->next_prp()) {
        if (prpty_reserved(pdesc->value()))
            continue;
        if (pdesc->value() == XICP_PC)
            new_pc = true;
        if (pdesc->value() == XICP_PC_PARAMS)
            new_pc_params = true;
        CDp *pcopy = pdesc->dup();
        if (!pcopy)
            continue;
        if (!pp)
            pp = p0 = pcopy;
        else {
            pp->set_next_prp(pcopy);
            pp = pp->next_prp();
        }
        pcopy->set_next_prp(0);
    }
    if (p0) {
        // Remove existing properties that shouldn't be duplicated.
        if (type() == CDINSTANCE) {
            if (new_pc)
                prptyRemove(XICP_PC);
            if (new_pc_params)
                prptyRemove(XICP_PC_PARAMS);
        }
        pp->set_next_prp(prpty_list());
        set_prpty_list(p0);
    }
}


// Unlink the CDp descriptor desc from the CDo property list.
//
CDp *
CDo::prptyUnlink(CDp *pdesc)
{
    CDp *pt1 = 0;
    for (CDp *pd = prpty_list(); pd; pt1 = pd, pd = pd->next_prp()) {
        if (pd == pdesc) {
            if (!pt1)
                set_prpty_list(pd->next_prp());
            else
                pt1->set_next_prp(pd->next_prp());
            pdesc->set_next_prp(0);
            return (pdesc);
        }
    }
    return (0);
}


// Remove all properties that match value from CDo.
//
void
CDo::prptyRemove(int value)
{
    CDp *pp = 0, *pn;
    for (CDp *p = prpty_list(); p; p = pn) {
        pn = p->next_prp();
        if (p->value() == value) {
            if (!pp)
                set_prpty_list(pn);
            else
                pp->set_next_prp(pn);
            delete p;
            continue;
        }
        pp = p;
    }
}


void
CDo::prptyFreeList()
{
    CDp *pd = prpty_list();
    set_prpty_list(0);
    while (pd) {
        CDp *px = pd;
        pd = pd->next_prp();
        delete px;
    }
}


// Remove links to this object determined from properties.  Called when
// object is deleted.
//
void
CDo::prptyClearInternal()
{
    CDp *pn;
    for (CDp *pd = prpty_list(); pd; pd = pn) {
        pn = pd->next_prp();
        // XICP_TERM_OSET marks the object as referenced by some terminal
        if (pd->value() == XICP_TERM_OSET)
            // this will delete pd
            ((CDp_oset*)pd)->term()->set_ref(0);
    }
}


namespace {
    bool
    get_pts(const char *s, Point **ptsp, int *nptsp)
    {
        *ptsp = 0;
        *nptsp = 0;
        int size = 1;
        Point *pts = 0;
        int cnt = 0;
        char *tok;
        while ((tok = lstring::gettok(&s)) != 0) {
            int n = cnt >> 1;
            if (n >= size || !pts) {
                Point *tpts = new Point[2*size];
                if (pts)
                    memcpy(tpts, pts, size*sizeof(Point));
                delete [] pts;
                pts = tpts;
                size += size;
            }
            int d;
            if (sscanf(tok, "%d", &d) != 1) {
                delete [] pts;
                delete [] tok;
                return (false);
            }
            if (cnt & 1)
                pts[n].y = d;
            else
                pts[n].x = d;
            delete [] tok;
            cnt++;
        }
        if (cnt & 1) {
            delete [] pts;
            return (false);
        }
        *ptsp = pts;
        *nptsp = cnt >> 1;
        return (true);
    }
}


// Static function.
// Create a box, polygon, or wire object (with copy flag set) on ld
// from the CIF statement string given.
//
CDo*
CDo::fromCifString(CDl *ld, const char *s)
{
    if (!s)
        return (0);
    while (isspace(*s))
        s++;

    if (*s == 'B') {
        s += 2;
        int w, h, x, y;
        if (sscanf(s, "%d %d %d %d;", &w, &h, &x, &y) != 4)
            return (0);
        BBox BB;
        BB.set_cif(x, y, w, h);
        if (BB.valid()) {
            CDo *od = new CDo(ld, &BB);
            od->set_copy(true);
            return (od);
        }
    }
    if (*s == 'P') {
        s += 2;
        Point *pts;
        int numpts;
        if (!get_pts(s, &pts, &numpts))
            return (0);
        Poly po(numpts, pts);
        if (po.valid()) {
            CDpo *od = new CDpo(ld, &po);
            od->set_copy(true);
            return (od);
        }
        delete [] pts;
    }
    if (*s == 'W') {
        int sty = isdigit(s[1]) ? s[1] - '0' : 2;
        if (sty > 2)
            sty = 2;
        s += 2;
        char *tok = lstring::gettok(&s);
        int wid;
        if (sscanf(tok, "%d", &wid) != 1) {
            delete [] tok;
            return (0);
        }
        delete [] tok;
        Point *pts;
        int numpts;
        if (!get_pts(s, &pts, &numpts))
            return (0);
        Wire w(wid, (WireStyle)sty, numpts, pts);
        CDw *od = new CDw(ld, &w);
        od->set_copy(true);
        return (od);
    }
    return (0);
}


//-----------------------------------------------------------------------------
// Wire
//

// Link the label to the wire.  A named wire will contribute a name
// for the containing net.  Scalar and vector names are supported.
//
bool
CDw::set_node_label(CDs *sdesc, CDla *label, bool noundo)
{
    if (!label) {
        Errs()->add_error("set_node_label: null label.");
        return (false);
    }
    if (!ldesc()->isWireActive()) {
        Errs()->add_error("set_node_label: wire not on active layer.");
        return (false);
    }

    hyList *h = label->label();
    char *string = hyList::string(h, HYcvPlain, true);

    CDnetex *netex;
    if (!CDnetex::parse(string, &netex) || !netex) {
        Errs()->add_error("set_node_label: empty label.");
        delete [] string;
        return (false);
    }
    delete [] string;

    if (noundo) {
        CDnetName nm;
        int beg, end;
        if (netex->is_scalar(&nm) ||
                (netex->is_simple(&nm, &beg, &end) && nm && beg == end)) {
            // A scalar or 1-bit connector.

            CDnetex::destroy(netex);
            prptyRemove(P_BNODE);
            CDp_node *pn = (CDp_node*)prpty(P_NODE);
            if (!pn) {
                pn = new CDp_node;
                link_prpty_list(pn);
            }
            pn->set_term_name(nm);
            if (pn->bound()) {
                Errs()->add_error(
                    "set_node_label: node label already bound.");
                return (false);
            }
            pn->bind(label);
            if (!label->link(0, this, 0)) {
                Errs()->add_error(
                    "set_node_label: failed to link label reference.");
                return (false);
            }
        }
        else {
            // A vector connector.

            prptyRemove(P_NODE);
            CDp_bnode *pb = (CDp_bnode*)prpty(P_BNODE);
            if (!pb) {
                pb = new CDp_bnode;
                link_prpty_list(pb);
            }
            if (pb->bound()) {
                Errs()->add_error(
                    "set_node_label: bus node label already bound.");
                CDnetex::destroy(netex);
                return (false);
            }
            pb->bind(label);
            if (!label->link(0, this, 0)) {
                Errs()->add_error(
                    "set_node_label: failed to link label reference.");
                CDnetex::destroy(netex);
                return (false);
            }
            pb->update_bundle(netex);
        }

    }
    else {
        // Unlink any existing node/bnode properties.
        CDp_bnode *pbo = (CDp_bnode*)prpty(P_BNODE);
        if (pbo)
            prptyUnlink(pbo);
        CDp_node *pno = (CDp_node*)prpty(P_NODE);
        if (pno)
            prptyUnlink(pno);

        CDnetName nm;
        int beg, end;
        if (netex->is_scalar(&nm) ||
                (netex->is_simple(&nm, &beg, &end) && nm && beg == end)) {
            // A scalar or 1-bit connector.
            CDnetex::destroy(netex);

            // Create, link, and bind a new node property.
            CDp_node *pn = new CDp_node;
            link_prpty_list(pn);
            pn->set_term_name(nm);
            pn->bind(label);
            bool ret = label->link(0, this, 0);

            // Unlink the new property and put back the old.  The undo
            // system will actually make the change.
            prptyUnlink(pn);
            if (pno && pbo) {
                // Shouldn't happen.
                delete pbo;
                pbo = 0;
            }
            CDp *oldp = pno;
            if (!oldp)
                oldp = pbo;
            if (oldp)
                link_prpty_list(oldp);
            if (!ret) {
                Errs()->add_error(
                    "set_node_label: failed to link label reference.");
                delete pn;
                return (false);
            }
            CD()->ifRecordPrptyChange(sdesc, this, oldp, pn);
        }
        else {
            // A vector connector.

            // Create, link, and bind a new bnode property.
            CDp_bnode *pb = new CDp_bnode;
            link_prpty_list(pb);
            pb->bind(label);
            bool ret = label->link(0, this, 0);
            pb->update_bundle(netex);

            // Unlink the new property and put back the old.  The undo
            // system will actually make the change.
            prptyUnlink(pb);
            if (pno && pbo) {
                // Shouldn't happen.
                delete pno;
                pno = 0;
            }
            CDp *oldp = pno;
            if (!oldp)
                oldp = pbo;
            if (oldp)
                link_prpty_list(oldp);
            if (!ret) {
                Errs()->add_error(
                    "set_node_label: failed to link label reference.");
                delete pb;
                return (false);
            }
            CD()->ifRecordPrptyChange(sdesc, this, oldp, pb);
        }
    }
    return (true);
}


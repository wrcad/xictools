
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
#include "cd_types.h"
#include "cd_hypertext.h"
#include "cd_propnum.h"
#include "cd_lgen.h"
#include "cd_objects.h"
#include "cd_cell.h"
#include "cd_instance.h"
#include "errorrec.h"


struct CDscriptOut
{
    CDscriptOut()
        {
            so_sdesc = 0;
            so_fp = 0;
            so_pfx = 0;
            so_newlyr = false;
        }

    bool write(const CDs*, FILE*, const char*);

private:
    bool writeBox(const CDo*);
    bool writePoly(const CDo*);
    bool writeWire(const CDo*);
    bool writeLabel(const CDo*);
    bool writeInst(const CDo*);
    CDp *cellProperties();
    CDp *objProperties(const CDo*);
    void getRefPt(const CDs *sdesc, const CDc*, int*, int*);

    const CDs *so_sdesc;
    FILE *so_fp;
    const char *so_pfx;
    bool so_newlyr;

};


// Write out the content of the cell in the form of script functions
// that will create the features when executed.  The pfx if not null
// or empty is prepended to non-pcell instance names.
//
bool
CDs::writeScript(FILE *fp, const char *pfx) const
{
    CDscriptOut so;
    return (so.write(this, fp, pfx));
}


bool
CDscriptOut::write(const CDs *sd, FILE *fp, const char *pfx)
{
    if (!sd) {
        Errs()->add_error("write: null cell descriptor");
        return (false);
    }
    if (!fp) {
        Errs()->add_error("write: null file pointer");
        return (false);
    }
    so_sdesc = sd;
    so_fp = fp;
    so_pfx = pfx;

    // Write cell properties
    CDp *p0 = cellProperties();
    while (p0) {
        fprintf(so_fp, "AddCellProperty(%d, \"%s\")\n", p0->value(),
            p0->string());
        CDp *px = p0;
        p0 = p0->next_prp();
        delete px;
    }
    fprintf(fp, "mkscr_sy = IsShowSymbolic()\n");
    fprintf(fp, "if (mkscr_sy != 0)\n    SetSymbolicFast(FALSE)\nend\n");

    CDl *ld;
    CDlgen lgen(so_sdesc->displayMode(), CDlgen::BotToTopWithCells);
    fprintf(so_fp, "mkscr_indx = GetCurLayerIndex()\n");
    while ((ld = lgen.next()) != 0) {
        so_newlyr = true;
        CDg gdesc;
        gdesc.init_gen(so_sdesc, ld);
        CDo *odesc;
        while ((odesc = gdesc.next()) != 0) {
            bool ret = true;
            if (odesc->type() == CDBOX)
                ret = writeBox(odesc);
            else if (odesc->type() == CDPOLYGON)
                ret = writePoly(odesc);
            else if (odesc->type() == CDWIRE)
                ret = writeWire(odesc);
            else if (odesc->type() == CDLABEL)
                ret = writeLabel(odesc);
            else if (odesc->type() == CDINSTANCE)
                ret = writeInst(odesc);
            if (!ret) {
                Errs()->add_error("writeScript: write to disk failed");
                return (false);
            }
        }
    }
    fprintf(so_fp, "if (mkscr_sy != 0)\n    SetSymbolicFast(TRUE)\nend\n");
    fprintf(so_fp, "SetCurLayerFast(mkscr_indx)\n");
    return (true);
}


bool
CDscriptOut::writeBox(const CDo *odesc)
{
    if (!odesc)
        return (false);
    CDl *ld = odesc->ldesc();
    if (so_newlyr) {
        fprintf(so_fp, "SetCurLayerFast(\"%s\")\n", ld->name());
        so_newlyr = false;
    }
    CDp *p0 = objProperties(odesc);
    int ret = 0;
    if (p0) {
        fprintf(so_fp,
            "mkscr_h = BoxH(%.5f, %.5f, %.5f, %.5f)\n",
            MICRONS(odesc->oBB().left), MICRONS(odesc->oBB().bottom),
            MICRONS(odesc->oBB().right), MICRONS(odesc->oBB().top));
        while (p0) {
            fprintf(so_fp, "PrptyAdd(mkscr_h, %d, \"%s\")\n",
                p0->value(), p0->string());
            CDp *px = p0;
            p0 = p0->next_prp();
            delete px;
        }
        ret = fprintf(so_fp, "Close(mkscr_h)\n");
    }
    else {
        ret = fprintf(so_fp, "Box(%.5f, %.5f, %.5f, %.5f)\n",
            MICRONS(odesc->oBB().left), MICRONS(odesc->oBB().bottom),
            MICRONS(odesc->oBB().right), MICRONS(odesc->oBB().top));
    }
    return (ret >= 0);
}


bool
CDscriptOut::writePoly(const CDo *odesc)
{
    if (!odesc)
        return (false);
    CDpo *p = (CDpo*)odesc;
    if (so_newlyr) {
        CDl *ld = odesc->ldesc();
        fprintf(so_fp, "SetCurLayerFast(\"%s\")\n", ld->name());
        so_newlyr = false;
    }

    int numpts = p->numpts();
    const Point *pts = p->points();

    fprintf(so_fp, "mkscr_ary[0] = \\\n  [");
    int j = 0;
    int ret = 0;
    for (int i = 0; i < numpts; i++) {
        j++;
        fprintf(so_fp, "%.5f,%.5f", MICRONS(pts[i].x), MICRONS(pts[i].y));
        if (i == numpts-1)
            fprintf(so_fp, "]\n");
        else if (j == 4) {
            fprintf(so_fp, ",\\\n   ");
            j = 0;
        }
        else
            fprintf(so_fp, ", ");
    }

    CDp *p0 = objProperties(odesc);
    if (p0) {
        fprintf(so_fp, "mkscr_h = PolygonH(%d, mkscr_ary)\n", numpts);
        while (p0) {
            fprintf(so_fp, "PrptyAdd(mkscr_h, %d, \"%s\")\n",
                p0->value(), p0->string());
            CDp *px = p0;
            p0 = p0->next_prp();
            delete px;
        }
        ret = fprintf(so_fp, "Close(mkscr_h)\n");
    }
    else
        ret = fprintf(so_fp, "Polygon(%d, mkscr_ary)\n", numpts);
    return (ret >= 0);
}


bool
CDscriptOut::writeWire(const CDo *odesc)
{
    if (!odesc)
        return (false);
    CDw *w = (CDw*)odesc;
    if (so_newlyr) {
        CDl *ld = odesc->ldesc();
        fprintf(so_fp, "SetCurLayerFast(\"%s\")\n", ld->name());
        so_newlyr = false;
    }

    int numpts = w->numpts();
    const Point *pts = w->points();

    fprintf(so_fp, "mkscr_ary[0] = \\\n  [");
    int j = 0;
    int ret = 0;
    for (int i = 0; i < numpts; i++) {
        j++;
        fprintf(so_fp, "%.5f,%.5f", MICRONS(pts[i].x), MICRONS(pts[i].y));
        if (i == numpts-1)
            fprintf(so_fp, "]\n");
        else if (j == 4) {
            fprintf(so_fp, ",\\\n   ");
            j = 0;
        }
        else
            fprintf(so_fp, ", ");
    }

    CDp *p0 = objProperties(odesc);
    if (p0) {
        fprintf(so_fp, "mkscr_h = WireH(%.5f, %d, mkscr_ary, %d)\n",
            MICRONS(w->wire_width()), numpts, w->wire_style());
        while (p0) {
            fprintf(so_fp, "PrptyAdd(mkscr_h, %d, \"%s\")\n",
                p0->value(), p0->string());
            CDp *px = p0;
            p0 = p0->next_prp();
            delete px;
        }
        ret = fprintf(so_fp, "Close(mkscr_h)\n");
    }
    else {
        ret = fprintf(so_fp, "Wire(%.5f, %d, mkscr_ary, %d)\n",
            MICRONS(w->wire_width()), numpts, w->wire_style());
    }
    return (ret >= 0);
}


bool
CDscriptOut::writeLabel(const CDo *odesc)
{
    if (!odesc)
        return (false);
    CDla *l = (CDla*)odesc;
    if (so_sdesc->isElectrical() && l->prpty(P_LABRF))
        return (true);
    if (so_newlyr) {
        CDl *ld = odesc->ldesc();
        fprintf(so_fp, "SetCurLayerFast(\"%s\")\n", ld->name());
        so_newlyr = false;
    }

    char *text = hyList::string(l->label(), HYcvPlain, false);
    int ret = 0;

    CDp *p0 = objProperties(odesc);
    if (p0) {
        fprintf(so_fp,
            "mkscr_h = LabelH(\"%s\", %.5f, %.5f, %.5f, 0, %d)\n",
            text, MICRONS(l->xpos()), MICRONS(l->ypos()),
            MICRONS(l->width()), l->xform());
        while (p0) {
            fprintf(so_fp, "PrptyAdd(mkscr_h, %d, \"%s\")\n",
                p0->value(), p0->string());
            CDp *px = p0;
            p0 = p0->next_prp();
            delete px;
        }
        ret = fprintf(so_fp, "Close(mkscr_h)\n");
    }
    else {
        ret = fprintf(so_fp,
            "Label(\"%s\", %.5f, %.5f, %.5f, 0, %d)\n",
            text, MICRONS(l->xpos()), MICRONS(l->ypos()),
            MICRONS(l->width()), l->xform());
    }
    delete [] text;
    return (ret >= 0);
}


bool
CDscriptOut::writeInst(const CDo *odesc)
{
    if (!odesc)
        return (false);
    CDc *c = (CDc*)odesc;
    CDs *msdesc = c->masterCell();
    if (!msdesc)
        return (true);
    if (so_newlyr)
        so_newlyr = false;

    CDap ap(c);
    CDtx tx(c);
    int xpos, ypos;
    getRefPt(msdesc, c, &xpos, &ypos);
    const BBox *pBB = msdesc->BB();
    const char *cn = Tstring(c->cellname());
    if (ap.nx > 1 || ap.ny > 1) {
        fprintf(so_fp, "mkscr_ary[0] = [%d, %d, %.5f, %.5f]\n",
            ap.nx, ap.ny, MICRONS(ap.dx - pBB->width()),
            MICRONS(ap.dy - pBB->height()));
    }

    int ret = 0;
    char *tfstring = tx.tfstring();
    CDp *p0 = objProperties(odesc);
    if (msdesc->isPCellSubMaster()) {
        if (msdesc->pcType() == CDpcXic) {
            // Xic native PCell
            CDp *pd = msdesc->prpty(XICP_PC);
            if (pd) {
                const char *pcname = pd->string();
                pd = msdesc->prpty(XICP_PC_PARAMS);
                const char *params = pd ? pd->string() : 0;
                fprintf(so_fp,
                    "PlaceSetPCellParams(0, %s, 0, \"%s\")\n",
                    pcname, params);
                cn = pcname;
            }
        }
        else {
            // OpenAccess PCell
//XXX fixme
        }

        // Instantiation should generate correct instance
        // properties (?)
        CDp::destroy(p0);

        ret = fprintf(so_fp,
            "Place(\"%s\", %.5f, %.5f, 0, %s, FALSE, FALSE, \"%s\")\n",
            cn, MICRONS(xpos), MICRONS(ypos),
            ap.nx > 1 || ap.ny > 1 ? "mkscr_ary" : "0", tfstring);
    }
    else {
        bool libdev = msdesc->isElectrical() && msdesc->isLibrary() &&
            msdesc->isDevice();

        if (!libdev) {
            if (so_pfx && *so_pfx)
                fprintf(so_fp, "TouchCell(%s + \"%s\", FALSE)\n", so_pfx, cn);
            else
                fprintf(so_fp, "TouchCell(\"%s\", FALSE)\n", cn);
        }
        if (!libdev && (so_pfx && *so_pfx)) {
            if (p0)
                fprintf(so_fp, "mkscr_h = PlaceH(%s + ", so_pfx);
            else
                fprintf(so_fp, "Place(%s + ", so_pfx);
        }
        else {
            if (p0)
                fprintf(so_fp, "mkscr_h = PlaceH(");
            else
                fprintf(so_fp, "Place(");
        }
        ret = fprintf(so_fp,
            "\"%s\", %.5f, %.5f, 0, %s, FALSE, FALSE, \"%s\")\n",
            cn, MICRONS(xpos), MICRONS(ypos),
            ap.nx > 1 || ap.ny > 1 ? "mkscr_ary" : "0", tfstring);
        bool hadp0 = p0;
        while (p0) {
            fprintf(so_fp, "PrptyAdd(mkscr_h, %d, \"%s\")\n", p0->value(),
                p0->string());
            CDp *px = p0;
            p0 = p0->next_prp();
            delete px;
        }
        if (hadp0)
            ret = fprintf(so_fp, "Close(mkscr_h)\n");
    }
    delete [] tfstring;
    return (ret >= 0);
}


// Return list of properties of sdesc, as strings.
CDp *
CDscriptOut::cellProperties()
{
    CDp *p0 = 0, *pe = 0;
    for (CDp *pd = so_sdesc->prptyList(); pd; pd = pd->next_prp()) {
        if (prpty_reserved(pd->value()))
            continue;
        char *s;
        if (pd->string(&s)) {
            if (strchr(s, '"') || strchr(s, '\n')) {
                sLstr lstr;
                for (const char *t = s; *t; t++) {
                    if (*t == '"') {
                        lstr.add_c('\\');
                        lstr.add_c(*t);
                    }
                    else if (*t == '\n') {
                        lstr.add_c('\\');
                        lstr.add_c('n');
                    }
                    else
                        lstr.add_c(*t);
                }
                delete [] s;
                s = lstr.string_trim();
            }
            CDp *pn = new CDp(s, pd->value());
            delete [] s;
            if (!p0)
                p0 = pe = pn;
            else {
                pe->set_next_prp(pn);
                pe = pe->next_prp();
            }
        }
    }
    return (p0);
}


// Return a list of properties to create in the new object.
//
CDp *
CDscriptOut::objProperties(const CDo *odesc)
{
    CDp *p0 = 0, *pe = 0;
    for (CDp *pd = odesc->prpty_list(); pd; pd = pd->next_prp()) {
        if (prpty_reserved(pd->value()))
            continue;

        if (so_sdesc->isElectrical()) {
            switch (pd->value()) {
            case P_MODEL:
            case P_VALUE:
            case P_PARAM:
            case P_OTHER:
            case P_NOPHYS:
            case P_SYMBLC:
                // There are handled directly.
                break;
            case P_NODE:
                // Grab the user-set name, skip if none.
                {
                    char *s;
                    if (pd->string(&s)) {
                        char *t = s;
                        char *tok = lstring::gettok(&t);
                        delete [] s;
                        t = strchr(tok, ':');
                        if (t) {
                            t++;
                            CDp *pn = new CDp(t, pd->value());
                            delete [] tok;
                            if (!p0)
                                p0 = pe = pn;
                            else {
                                pe->set_next_prp(pn);
                                pe = pe->next_prp();
                            }
                        }
                    }
                }
                // fall thru
            default:
                continue;
            }
        }

        char *s;
        if (pd->string(&s)) {
            if (strchr(s, '"')) {
                sLstr lstr;
                for (const char *t = s; *t; t++) {
                    if (*t == '"') {
                        lstr.add_c('\\');
                        lstr.add_c(*t);
                    }
                    else if (*t == '\n') {
                        lstr.add_c('\\');
                        lstr.add_c('n');
                    }
                    else
                        lstr.add_c(*t);
                }
                delete [] s;
                s = lstr.string_trim();
            }
            CDp *pn = new CDp(s, pd->value());
            delete [] s;
            if (!p0)
                p0 = pe = pn;
            else {
                pe->set_next_prp(pn);
                pe = pe->next_prp();
            }
        }
    }

    // Add the NODRC property if necessary
    if (!so_sdesc->isElectrical() && odesc->type() != CDINSTANCE &&
            odesc->type() != CDLABEL && (odesc->flags() & CDnoDRC)) {
        CDp *pn = new CDp("NODRC", XICP_NODRC);
        if (!p0)
            p0 = pe = pn;
        else {
            pe->set_next_prp(pn);
            pe = pe->next_prp();
        }
    }
    return (p0);
}


// The placement position is relative to the coordinate of the 0'th
// terminal for electrical mode subcircuits and devices.  We need to
// find this location and offset the placement accordingly.
//
void
CDscriptOut::getRefPt(const CDs *sdesc, const CDc *cdesc, int *x, int *y)
{
    if (so_sdesc->isElectrical()) {
        if (sdesc->owner())
            sdesc = sdesc->owner();

        // Index 0 is the reference point.
        CDp_snode *pn = (CDp_snode*)sdesc->prpty(P_NODE);
        for ( ; pn; pn = pn->next()) {
            if (pn->index() == 0) {
                if (sdesc->symbolicRep(cdesc)) {
                    if (!pn->get_pos(0, x, y))
                        continue;
                }
                else
                    pn->get_schem_pos(x, y);
                cTfmStack stk;
                stk.TPush();
                stk.TApplyTransform(cdesc);
                stk.TPoint(x, y);
                return;
            }
        }
    }
    *x = cdesc->posX();
    *y = cdesc->posY();
}


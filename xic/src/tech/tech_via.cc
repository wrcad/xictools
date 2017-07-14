
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2016 Whiteley Research Inc, all rights reserved.        *
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
 $Id: tech_via.cc,v 1.5 2017/04/16 20:28:21 stevew Exp $
 *========================================================================*/

#include "dsp.h"
#include "cd_celldb.h"
#include "cd_propnum.h"
#include "tech.h"
#include "tech_via.h"
#include "tech_kwords.h"
#include "si_parsenode.h"
#include <algorithm>


//
// Support for Standard Vias.
//

#define STV_DEBUG

// Link the sv content into the table.  This is called when reading
// tech data.  The sStdVia requires a name for linking.  If there is a
// name clash with an existing sStdVia, the existing struct is
// returned.  It is NOT possible to remove or overwrite a sStdVia in
// the table.
//
sStdVia *
cTech::AddStdVia(const sStdVia &sv)
{
    if (!sv.tab_name())
        return (0);
    if (!tc_std_vias) {
        tc_std_vias = new table_t<sStdVia>;
        if (tc_has_std_via)
            (*tc_has_std_via)(true);
    }
    sStdVia *ov = tc_std_vias->find(sv.tab_name());
    if (ov) {
#ifdef STV_DEBUG
        printf("AddStdVia: %s already exists, returning existing.\n",
            sv.tab_name());
#endif
        return (ov);
    }
    sStdVia *nv = new sStdVia(sv);

    // Make sure that nothing rogue is set.
    nv->clean_pre_insert();
#ifdef STV_DEBUG
        printf("AddStdVia: inserting new %s.\n", sv.tab_name());
#endif

    return (tc_std_vias->link(nv, true));
}


sStdVia *
cTech::FindStdVia(const char *svname)
{
    if (tc_std_vias && svname)
        return (tc_std_vias->find(svname));
    return (0);
}


// The argument is the XICP_STDVIA property string.  Return (create if
// necessary) the corresponding sub-master pointer.
//
CDs *
cTech::OpenViaSubMaster(const char *str)
{
    char *vname = lstring::gettok(&str);
    if (!vname) {
        Errs()->add_error("OpenViaSubMaster: bad string, no name given.");
        return (0);
    }
    // The first token is the standard via name.

    sStdVia *sv = tc_std_vias ? tc_std_vias->find(vname) : 0;
    if (!sv) {
        Errs()->add_error("OpenViaSubMaster: no standard via named \"%s\".",
            vname);
        delete [] vname;
        return (0);
    }
    delete [] vname;

    sStdVia rv(*sv);
    rv.parse(str);
#ifdef STV_DEBUG
    printf("OpenViaSubMaster: called for %s.\n", str);
#endif

    CDs *sd = 0;
    for (sStdVia *v = sv; v; v = v->variations()) {
        if (*v == rv) {
            sd = v->open();
#ifdef STV_DEBUG
            if (!sd)
                printf("  Found existing, but null cell pointer!\n");
            else {
                CDp *pd = sd->prpty(XICP_STDVIA);
                printf("  Found existing, cell=%s string=%s\n", 
                    sd->cellname()->string(), pd ? pd->string() : "null");
            }
#endif
            break;
        }
    }
    if (!sd) {
        sStdVia *nv = new sStdVia(rv);
        sv->add_variation(nv);
        sd = nv->open();
#ifdef STV_DEBUG
        if (!sd)
            printf("  New variation, but null cell pointer!\n");
        else {
            CDp *pd = sd->prpty(XICP_STDVIA);
            printf("  New variation, cell=%s string=%s\n",
                sd->cellname()->string(), pd ? pd->string() : "null");
        }
#endif
    }
    return (sd);
}


namespace {
    inline bool sv_comp(const sStdVia *v1, const sStdVia *v2)
    {
        return (strcmp(v1->tab_name(), v2->tab_name()) < 0);
    }
}


// Return a list of sStdVia struct pointers, sorted alpha by name. 
// These point to data in the tech database.  The return list should
// be freed by calling the free method.
//
sStdViaList *
cTech::StdViaList()
{
    sStdViaList *vl = 0;
    if (tc_std_vias && tc_std_vias->allocated()) {
        tgen_t<sStdVia> gen(tc_std_vias);
        const sStdVia *v;
        int cnt = 0;
        while ((v = gen.next()) != 0) {
            if (!v->via() || !v->bottom() || !v->top())
                continue;
            vl = new sStdViaList(v, vl);
            cnt++;
        }
        if (cnt < 2)
            return (vl);
        const sStdVia **ary = new const sStdVia*[cnt];
        cnt = 0;
        for (sStdViaList *sv = vl; sv; sv = sv->next)
            ary[cnt++] = sv->std_via;
        std::sort(ary, ary + cnt, sv_comp);
        cnt = 0;
        for (sStdViaList *sv = vl; sv; sv = sv->next)
            sv->std_via = ary[cnt++];
        delete [] ary;
    }
    return (vl);
}


// The cell database and cellname tables have been cleared.  Either
// blow away the standard vieas (clear == true), or recreate the
// cells.
//
void
cTech::StdViaReset(bool clear)
{
    if (tc_std_vias && tc_std_vias->allocated()) {
        tgen_t<sStdVia> gen(tc_std_vias);
        sStdVia *v;
        while ((v = gen.next()) != 0) {
            if (clear) {
                tc_std_vias->unlink(v);
                v->clear_variations();
                delete v;
            }
            else
                v->reset();
        }
        if (clear) {
            delete tc_std_vias;
            tc_std_vias = 0;
        }
    }
}
// End of cTech functions.


// Parse the string and set the fields accordingly.  The format is as
// generated by string() or an old format, with the name stripped.
//
bool
sStdVia::parse(const char *str)
{
    if (!str)
        return (true);
    const char *strbk = str;
    char *tok = lstring::gettok(&str);
    if (!tok)
        return (true);
    str = strbk;
    bool old_style = strchr(tok, ':');
    delete [] tok;

    if (old_style) {
        // Old format: keyword:x[,y] ...

        const char *s = str;
        char tbuf[32];
        const int maxlen=20;
        while (*s) {
            while (isspace(*s))
                s++;
            if (!*s)
                break;
            char *t = tbuf;
            int i = 0;
            while (i < maxlen) {
                if (*s == ':')
                    break;
                if (!isalnum(*s))
                    return (false);
                *t++ = *s++;
                i++;
            }
            if (i == maxlen)
                return (false);
            if (*s != ':')
                return (false);
            s++;
            tbuf[i] = 0;
            if (lstring::cieq(tbuf, "Layer1Enc")) {
                int x, y;
                if (sscanf(s, "%d,%d", &x, &y) != 2)
                    return (false);
                sv_bot_enc_x = x;
                sv_bot_enc_y = y;
            }
            else if (lstring::cieq(tbuf, "Layer2Enc")) {
                int x, y;
                if (sscanf(s, "%d,%d", &x, &y) != 2)
                    return (false);
                sv_top_enc_x = x;
                sv_top_enc_y = y;
            }
            else if (lstring::cieq(tbuf, "Implant1Enc")) {
                int x, y;
                if (sscanf(s, "%d,%d", &x, &y) != 2)
                    return (false);
                sv_imp1_enc_x = x;
                sv_imp1_enc_y = y;
            }
            else if (lstring::cieq(tbuf, "Implant2Enc")) {
                int x, y;
                if (sscanf(s, "%d,%d", &x, &y) != 2)
                    return (false);
                sv_imp2_enc_x = x;
                sv_imp2_enc_y = y;
            }
            else if (lstring::cieq(tbuf, "Layer1Offset")) {
                int x, y;
                if (sscanf(s, "%d,%d", &x, &y) != 2)
                    return (false);
                sv_bot_off_x = x;
                sv_bot_off_y = y;
            }
            else if (lstring::cieq(tbuf, "Layer2Offset")) {
                int x, y;
                if (sscanf(s, "%d,%d", &x, &y) != 2)
                    return (false);
                sv_top_off_x = x;
                sv_top_off_y = y;
            }
            else if (lstring::cieq(tbuf, "CutSpacing")) {
                int x, y;
                if (sscanf(s, "%d,%d", &x, &y) != 2)
                    return (false);
                sv_via_spa_x = x;
                sv_via_spa_y = y;
            }
            else if (lstring::cieq(tbuf, "OriginOffset")) {
                int x, y;
                if (sscanf(s, "%d,%d", &x, &y) != 2)
                    return (false);
                sv_org_off_x = x;
                sv_org_off_y = y;
            }
            else if (lstring::cieq(tbuf, "CutLayer")) {
                int x;
                if (sscanf(s, "%d", &x) != 1)
                    return (false);
                // We don't use this.
            }
            else if (lstring::cieq(tbuf, "CutColumns")) {
                int x;
                if (sscanf(s, "%d", &x) != 1)
                    return (false);
                sv_via_cols = x;
            }
            else if (lstring::cieq(tbuf, "CutRows")) {
                int x;
                if (sscanf(s, "%d", &x) != 1)
                    return (false);
                sv_via_rows = x;
            }
            else if (lstring::cieq(tbuf, "CutWidth")) {
                int x;
                if (sscanf(s, "%d", &x) != 1)
                    return (false);
                sv_via_wid = x;
            }
            else if (lstring::cieq(tbuf, "CutHeight")) {
                int x;
                if (sscanf(s, "%d", &x) != 1)
                    return (false);
                sv_via_hei = x;
            }
            else
                return (false);
            while (*s && !isspace(*s))
                s++;
        }
    }
    else {
        // New style.

        const char *s = str;
        while (*s) {
            while (isspace(*s))
                s++;
            if (!*s)
                break;
            int c = *s++;
            if (c < 'a' || c > 't')
                return (false);
            c -= 'a';
            const char *t = s;
            while (*s && !isspace(*s))
                s++;

            int *p = &sv_via_wid;
            if (c < 2) {
                double d;
                if (sscanf(t, "%lf", &d) != 1)
                    return (false);
                p[c] = INTERNAL_UNITS(d/1000);
                continue;
            }
            if (c < 4) {
                int i;
                if (sscanf(t, "%d", &i) != 1)
                    return (false);
                p[c] = i;
                continue;
            }
            if (c < 16) {
                double d;
                if (sscanf(t, "%lf", &d) != 1)
                    return (false);
                p[c] = INTERNAL_UNITS(d/1000);
                continue;
            }
            if (c == 16) {
                double d;
                if (sscanf(t, "%lf", &d) != 1)
                    return (false);
                sv_imp1_enc_x = INTERNAL_UNITS(d/1000);
                continue;
            }
            if (c == 17) {
                double d;
                if (sscanf(t, "%lf", &d) != 1)
                    return (false);
                sv_imp1_enc_y = INTERNAL_UNITS(d/1000);
                continue;
            }
            if (c == 18) {
                double d;
                if (sscanf(t, "%lf", &d) != 1)
                    return (false);
                sv_imp2_enc_x = INTERNAL_UNITS(d/1000);
                continue;
            }
            if (c == 19) {
                double d;
                if (sscanf(t, "%lf", &d) != 1)
                    return (false);
                sv_imp2_enc_y = INTERNAL_UNITS(d/1000);
                continue;
            }
            return (false);
        }
    }
    return (true);
}


// Return a string for the XICP_STDVIA property.  This property is
// applied to standard via sub-masters and instances.  The syntax is
//
//   name <code><num> <code><num> ...
//
// The name is the base standard via name as defined in the tech file.
// The <code> is a letter:
//
//   a  via_wid;
//   b  via_hei;
//   c  via_rows;
//   d  via_cols;
//   e  via_spa_x;
//   f  via_spa_y;
//   g  bot_enc_x;
//   h  bot_enc_y;
//   i  bot_off_x;
//   j  bot_off_y;
//   k  top_enc_x;
//   l  top_enc_y;
//   m  top_off_x;
//   n  top_off_y;
//   o  org_off_x;
//   p  org_off_y;
//   q  imp1_enc_x;
//   r  imp1_enc_y;
//   s  imp2_enc_x;
//   t  imp2_enc_y;
//
// The <num> is the non-standard value, in nanometers except for rows/cols.
// The print format is "%g".
//
char *
sStdVia::string() const
{
    sLstr lstr;
    if (sv_reference) {
        if (!sv_reference->sv_name)
            return (0);
        lstr.add(sv_reference->sv_name);
    }
    else {
        if (!sv_name)
            return (0);
        return (lstring::copy(sv_name));
    }
    unsigned int *i1 = (unsigned int*)&sv_via_wid;
    unsigned int *i2 = (unsigned int*)&sv_reference->sv_via_wid;
    int cnt = 0;
    while (i1 <= (unsigned int*)&sv_org_off_y) {
        if (*i1 != *i2) {
            if (cnt == 2 || cnt == 3) {
                lstr.add_c(' ');
                lstr.add_c('a' + cnt);
                lstr.add_i(*i1);
            }
            else {
                lstr.add_c(' ');
                lstr.add_c('a' + cnt);
                lstr.add_g(1000*MICRONS(*i1));
            }
        }
        i1++;
        i2++;
        cnt++;
    }
    if (sv_implant1 || sv_reference->sv_implant1) {
        i1 = (unsigned int*)&sv_imp1_enc_x;
        i2 = (unsigned int*)&sv_reference->sv_imp1_enc_x;
        while (i1 <= (unsigned int*)&sv_imp1_enc_y) {
            if (*i1 != *i2) {
                lstr.add_c(' ');
                lstr.add_c('a' + cnt);
                lstr.add_g(1000*MICRONS(*i1));
            }
            i1++;
            i2++;
            cnt++;
        }
        if (sv_implant2 || sv_reference->sv_implant2) {
            i1 = (unsigned int*)&sv_imp2_enc_x;
            i2 = (unsigned int*)&sv_reference->sv_imp2_enc_x;
            while (i1 <= (unsigned int*)&sv_imp2_enc_y) {
                if (*i1 != *i2) {
                    lstr.add_c(' ');
                    lstr.add_c('a' + cnt);
                    lstr.add_g(1000*MICRONS(*i1));
                }
                i1++;
                i2++;
                cnt++;
            }
        }
    }
    return (lstr.string_trim());
}


namespace {
    // Find an unused cell name.
    //
    CDcellName get_cellname(const char *pfx, bool trybare = false)
    {
        char buf[256];
        char *e = lstring::stpcpy(buf, pfx);
        CDcbin cbin;
        if (trybare) {
            if (!CDcdb()->findSymbol(buf, &cbin))
                return (CD()->CellNameTableAdd(buf));
        }
        for (int i = 1; ; i++) {
            sprintf(e, "_%d", i);
            if (!CDcdb()->findSymbol(buf, &cbin))
                return (CD()->CellNameTableAdd(buf));
        }
        return (0);
    }
}


// Return the CDs for the standard via, create if necessary.
//
CDs *
sStdVia::open()
{
    if (sv_sdesc)
        return (sv_sdesc);
    if (!sv_via || !sv_bot || !sv_top) {
        Errs()->add_error("sStdVia::open: missing layer descriptor.");
        return (0);
    }

    // Strip the white space out of the property string.  This will
    // produce a decently unique and well-defined cell name.
    char *sname = string();
    if (!sname) {
        Errs()->add_error("sStdVia::open: null string!");
        return (0);
    }
    char *s = sname;
    char *t = sname;
    while (*t) {
        if (!isspace(*t)) {
            if (*t == '-')
                *s++ = 'm';
            else if (*t == '.')
                *s++ = 'p';
            else
                *s++ = *t;
        }
        t++;
    }
    *s = 0;

    // The name shouldn't be in use, but one can't be sure.
    CDcellName cellname = get_cellname(sname, true);
    delete [] sname;

    if (!sv_name)
        sv_name = lstring::copy(cellname->string());

    CDs *sd = new CDs(cellname, Physical);
    CDcdb()->linkCell(sd);

    CDo *newb;
    int xo = sv_org_off_x;
    int yo = sv_org_off_y;
    int cw2 = sv_via_wid/2;
    int ch2 = sv_via_hei/2;

    // From OpenAccess, the origin should default to the center of the
    // lower-left cut.  Virtuoso uses the center of the cut array,
    // which we'll adapt here.

    int w = sv_via_cols*sv_via_wid + (sv_via_cols - 1)*sv_via_spa_x;
    int h = sv_via_rows*sv_via_hei + (sv_via_rows - 1)*sv_via_spa_y;
    xo += cw2 - w/2;
    yo += ch2 - h/2;

    for (int i = 0; i < sv_via_rows; i++) {
        int yb = yo - ch2 + i*(sv_via_hei + sv_via_spa_y);
        for (int j = 0; j < sv_via_cols; j++) {
            int xl = xo - cw2 + j*(sv_via_wid + sv_via_spa_x);
            BBox BB(xl, yb, xl + sv_via_wid, yb + sv_via_hei);
            sd->makeBox(sv_via, &BB, &newb, false);
        }
    }
    BBox cutBB(xo - cw2, yo - ch2,
        xo - cw2 + (sv_via_cols - 1)*(sv_via_wid + sv_via_spa_x) + sv_via_wid,
        yo - ch2 + (sv_via_rows - 1)*(sv_via_hei + sv_via_spa_y) + sv_via_hei);

    BBox l1BB(
        cutBB.left - sv_bot_enc_x + sv_bot_off_x,
        cutBB.bottom - sv_bot_enc_y + sv_bot_off_y,
        cutBB.right + sv_bot_enc_x + sv_bot_off_x,
        cutBB.top + sv_bot_enc_y + sv_bot_off_y);
    sd->makeBox(sv_bot, &l1BB, &newb, false);

    BBox l2BB(
        cutBB.left - sv_top_enc_x + sv_top_off_x,
        cutBB.bottom - sv_top_enc_y + sv_top_off_y,
        cutBB.right + sv_top_enc_x + sv_top_off_x,
        cutBB.top + sv_top_enc_y + sv_top_off_y);
    sd->makeBox(sv_top, &l2BB, &newb, false);

    if (sv_implant1) {
        BBox imp1BB(l1BB.left - sv_imp1_enc_x, l1BB.bottom - sv_imp1_enc_y,
            l1BB.right + sv_imp1_enc_x, l1BB.top + sv_imp1_enc_y);
        sd->makeBox(sv_implant1, &imp1BB, &newb, false);
        if (sv_implant2) {
            BBox imp2BB(l2BB.left - sv_imp2_enc_x, l2BB.bottom - sv_imp2_enc_y,
                l2BB.right + sv_imp2_enc_x, l2BB.top + sv_imp2_enc_y);
            sd->makeBox(sv_implant2, &imp2BB, &newb, false);
        }
    }

    // Add the STDVIA property, which will identify the sub-master as
    // such.  Also set the IMMUTABLE flag.
    char *pstring = string();
    sd->prptyAdd(XICP_STDVIA, pstring);
    delete [] pstring;
    sd->setImmutable(true);

    sv_sdesc = sd;
    return (sd);
}


// We know that this is a super-master, and the corresponding CDs has
// been destroyed.  Clear the variations, and recreate the cell.
//
void
sStdVia::reset()
{
    if (sv_reference)
        return;
    sv_sdesc = 0;
    clear_variations();
    open();
#ifdef STV_DEBUG
    printf("reset %s, %x\n", sv_name, CDcdb()->findCell(sv_name, Physical));
#endif
}


// Print a standard via description, for the tech file.
//
void
sStdVia::tech_print(FILE *fp) const
{
    if (!via() || !bottom() || !top())
        return;
    fprintf(fp, "%s %-16s %-10s %-10s %s \\\n", Tkw.StandardVia(),
        tab_name(), bottom()->name(), top()->name(), via()->name());
    char buf[256];
    sprintf(buf, "%g %g", MICRONS(via_wid()), MICRONS(via_hei()));
    fprintf(fp, "    %-12s", buf);
    sprintf(buf, "%d %d", via_rows(), via_cols());
    fprintf(fp, " %-5s", buf);
    sprintf(buf, "%g %g", MICRONS(via_spa_x()), MICRONS(via_spa_y()));
    fprintf(fp, " %-12s", buf);
    sprintf(buf, "%g %g", MICRONS(bot_enc_x()), MICRONS(bot_enc_y()));
    fprintf(fp, " %-12s", buf);
    sprintf(buf, "%g %g", MICRONS(top_enc_x()), MICRONS(top_enc_y()));
    fprintf(fp, " %s \\\n", buf);

    sprintf(buf, "%g %g", MICRONS(bot_off_x()), MICRONS(bot_off_y()));
    fprintf(fp, "    %-12s", buf);
    sprintf(buf, "%g %g", MICRONS(top_off_x()), MICRONS(top_off_y()));
    fprintf(fp, " %-12s", buf);
    sprintf(buf, "%g %g", MICRONS(org_off_x()), MICRONS(org_off_y()));
    fprintf(fp, " %s", buf);
    if (implant1()) {
        sprintf(buf, "%g %g", MICRONS(imp1_enc_x()), MICRONS(imp1_enc_y()));
        fprintf(fp, " \\\n    %-16s %12s", implant1()->name(), buf);
        if (implant2()) {
            sprintf(buf, "%g %g", MICRONS(imp2_enc_x()), MICRONS(imp2_enc_y()));
            fprintf(fp, "  %-16s %12s", implant2()->name(), buf);
        }
    }
    fprintf(fp, "\n");
}


namespace {
    bool get_dim(const char **str, int *ip)
    {
        char *tok = lstring::gettok(str);
        if (!tok)
            return (false);
        double d;
        int ret = sscanf(tok, "%lf", &d);
        delete [] tok;
        if (ret != 1)
            return (false);
        *ip = INTERNAL_UNITS(d);
        return (true);
    }


    bool get_int(const char **str, int *ip)
    {
        char *tok = lstring::gettok(str);
        if (!tok)
            return (false);
        int i;
        int ret = sscanf(tok, "%d", &i);
        delete [] tok;
        if (ret != 1)
            return (false);
        *ip = i;
        return (true);
    }


    char *errret(const char *rstr)
    {
        char buf[256];
        sprintf(buf, "error at token %s", rstr);
        return (lstring::copy(buf));
    }
}


// Static function.
//
// Techfile parser for standard via description.
//
TCret
sStdVia::tech_parse(const char *inbuf)
{
    const char *t = inbuf;
    char *svname = lstring::gettok(&t);
    if (!svname)
        return (errret("via_name"));

    char *botname = lstring::gettok(&t);
    if (!botname) {
        delete [] svname;
        return (errret("layer1_name"));
    }
    CDl *ldb = CDldb()->findLayer(botname);
    delete [] botname;
    if (!ldb) {
        delete [] svname;
        return (errret("layer1"));
    }

    char *topname = lstring::gettok(&t);
    if (!topname) {
        delete [] svname;
        return (errret("layer2_name"));
    }
    CDl *ldt = CDldb()->findLayer(topname);
    delete [] topname;
    if (!ldt) {
        delete [] svname;
        return (errret("layer2"));
    }

    char *vlname = lstring::gettok(&t);
    if (!vlname) {
        delete [] svname;
        return (errret("cut_layer_name"));
    }
    CDl *ldv = CDldb()->findLayer(vlname);
    delete [] vlname;
    if (!ldv) {
        delete [] svname;
        return (errret("cut_layer"));
    }

    sStdVia sv(svname, ldv, ldb, ldt);
    delete [] svname;

    int ival;
    if (!get_dim(&t, &ival))
        return (errret("cut_wid"));
    sv.set_via_wid(ival);
    if (!get_dim(&t, &ival))
        return (errret("cut_hei"));
    sv.set_via_hei(ival);

    if (!get_int(&t, &ival))
        return (errret("cut_rows"));
    sv.set_via_rows(ival);
    if (!get_int(&t, &ival))
        return (errret("cut_cols"));
    sv.set_via_cols(ival);

    if (!get_dim(&t, &ival))
        return (errret("cut_spa_x"));
    sv.set_via_spa_x(ival);
    if (!get_dim(&t, &ival))
        return (errret("cut_spa_y"));
    sv.set_via_spa_y(ival);

    if (!get_dim(&t, &ival))
        return (errret("enc1_x"));
    sv.set_bot_enc_x(ival);
    if (!get_dim(&t, &ival))
        return (errret("enc1_y"));
    sv.set_bot_enc_y(ival);

    if (!get_dim(&t, &ival))
        return (errret("enc2_x"));
    sv.set_top_enc_x(ival);
    if (!get_dim(&t, &ival))
        return (errret("enc2_y"));
    sv.set_top_enc_y(ival);

    if (!get_dim(&t, &ival))
        return (errret("off1_x"));
    sv.set_bot_off_x(ival);
    if (!get_dim(&t, &ival))
        return (errret("off1_y"));
    sv.set_bot_off_y(ival);

    if (!get_dim(&t, &ival))
        return (errret("off2_x"));
    sv.set_top_off_x(ival);
    if (!get_dim(&t, &ival))
        return (errret("off2_y"));
    sv.set_top_off_y(ival);

    if (!get_dim(&t, &ival))
        return (errret("org_off_x"));
    sv.set_org_off_x(ival);
    if (!get_dim(&t, &ival))
        return (errret("org_off_y"));
    sv.set_org_off_y(ival);

    char *tok = lstring::gettok(&t);
    if (tok) {
        CDl *ldi1 = CDldb()->findLayer(tok);
        delete [] tok;
        if (ldi1) {
            sv.set_implant1(ldi1);
            if (!get_dim(&t, &ival))
                return (errret("imp1_enc_x"));
            sv.set_imp1_enc_x(ival);
            if (!get_dim(&t, &ival))
                return (errret("imp1_enc_y"));
            sv.set_imp1_enc_y(ival);

            tok = lstring::gettok(&t);
            if (tok) {
                CDl *ldi2 = CDldb()->findLayer(tok);
                delete [] tok;
                if (ldi2) {
                    sv.set_implant2(ldi2);
                    if (!get_dim(&t, &ival))
                        return (errret("imp2_enc_x"));
                    sv.set_imp2_enc_x(ival);
                    if (!get_dim(&t, &ival))
                        return (errret("imp2_enc_y"));
                    sv.set_imp2_enc_y(ival);
                }
                else
                    return (errret("implant2"));
            }
        }
        else
            return (errret("implant1"));
    }
    Tech()->AddStdVia(sv);
    return (TCmatch);
}
// End of sStdVia functions.


sVia::sVia(char *vl1, char *vl2, ParseNode *t)
{
    v_next = 0;
    v_lname1 = vl1;
    v_lname2 = vl2;
    v_ld1 = 0;
    v_ld2 = 0;
    v_tree = t;
}


sVia::~sVia()
{
    delete [] v_lname1;
    delete [] v_lname2;
    ParseNode::destroy(v_tree);
}


CDl *
sVia::top_layer()
{
    CDl *ld1 = layer1();
    CDl *ld2 = layer2();
    if (!ld1 && !layername1()) // for Dielectric on substrate.
        return (ld2);
    if (!ld1 || !ld2)
        return (0);
    return (ld1->physIndex() > ld2->physIndex() ? ld1 : ld2);
}


CDl *
sVia::bottom_layer()
{
    CDl *ld1 = layer1();
    CDl *ld2 = layer2();
    if (!ld2 && !layername2())  // For Dielectric above all other layers.
        return (ld1);
    if (!ld1 || !ld2)
        return (0);
    return (ld1->physIndex() < ld2->physIndex() ? ld1 : ld2);
}


CDl *
sVia::getld1()
{
    if (v_lname1) {
        v_ld1 = CDldb()->findLayer(v_lname1, Physical);
        return (v_ld1);
    }
    return (0);
}


CDl *
sVia::getld2()
{
    if (v_lname2) {
        v_ld2 = CDldb()->findLayer(v_lname2, Physical);
        return (v_ld2);
    }
    return (0);
}
// End of sVia functions.


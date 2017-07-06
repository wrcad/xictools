
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2012 Whiteley Research Inc, all rights reserved.        *
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
 $Id: tech_drf_in.cc,v 1.9 2017/03/14 01:26:55 stevew Exp $
 *========================================================================*/

#include "dsp.h"
#include "dsp_window.h"
#include "dsp_layer.h"
#include "dsp_inlines.h"
#include "si_parsenode.h"
#include "si_lisp.h"
#include "tech.h"
#include "tech_drf_in.h"
#include "errorlog.h"
#include "main_variables.h"


//-----------------------------------------------------------------------------
// cTechDrfIn  Cadence DRF reader

cTechDrfIn *cTechDrfIn::instancePtr;
sDrfDeferred *cTechDrfIn::deferred;

cTechDrfIn::cTechDrfIn()
{
    if (instancePtr) {
        fprintf(stderr,
            "Singleton class cTechDrfIn is already instantiated.\n");
        exit(1);
    }
    instancePtr = this;

    c_display = 0;
    c_color_tab = 0;
    c_color_rtab = 0;
    c_stipple_tab = 0;
    c_stipple_rtab = 0;
    c_line_tab = 0;
    c_packet_tab = 0;
    c_packet_rtab = 0;
    c_badpkts = 0;
    c_badstips = 0;
    c_badclrs = 0;

    // Virtuoso
    register_node("drDefineDisplay",        drDefineDisplay);
    register_node("drDefineColor",          drDefineColor);
    register_node("drDefineStipple",        drDefineStipple);
    register_node("drDefineLineStyle",      drDefineLineStyle);
    register_node("drDefinePacket",         drDefinePacket);

    // Santana
    register_node("dispDefineDisplay",      drDefineDisplay);
    register_node("dispDefineColor",        drDefineColor);
    register_node("dispDefineStipple",      drDefineStipple);
    register_node("dispDefineLineStyle",    drDefineLineStyle);
    register_node("dispDefinePacket",       drDefinePacket);
}


cTechDrfIn::~cTechDrfIn()
{
    instancePtr = 0;

    delete [] c_display;
    SymTabGen cgen(c_color_tab, true);
    SymTabEnt *ent;
    while ((ent = cgen.next()) != 0) {
        delete (sDrfColor*)ent->stData;
        delete ent;
    }
    delete c_color_rtab;

    SymTabGen sgen(c_stipple_tab, true);
    while ((ent = sgen.next()) != 0) {
        delete (sDrfStipple*)ent->stData;
        delete ent;
    }
    delete c_stipple_rtab;

    SymTabGen lgen(c_line_tab, true);
    while ((ent = lgen.next()) != 0) {
        delete (sDrfLine*)ent->stData;
        delete ent;
    }

    SymTabGen pgen(c_packet_tab, true);
    while ((ent = pgen.next()) != 0) {
        delete (sDrfPacket*)ent->stData;
        delete ent;
    }
    delete c_packet_rtab;

    stringlist::destroy(c_badpkts);
    stringlist::destroy(c_badstips);
    stringlist::destroy(c_badclrs);
}


// Main entry to read a DRF file.
//
bool
cTechDrfIn::read(const char *fname, char **err)
{
    bool ret = readEvalLisp(fname, CDvdb()->getVariable(VA_LibPath), true, err);
    if (deferred) {
        // The tech data were loaded before DRF.

        while (deferred) {
            apply_packet(deferred->ldesc, deferred->packet);
            sDrfDeferred *dx = deferred;
            deferred = deferred->next;
            delete dx;
        }
        sLstr lstr;
        report_unresolved(lstr);
        if (lstr.string() && CDvdb()->getVariable(VA_DrfDebug))
            Log()->WarningLog("compat", lstr.string());
    }
    return (ret);
}


namespace {
    // Defaults for presentation attributes.
#define DEF_STIPPLE "none"
#define DEF_LSTYLE  "solid"
#define DEF_COLOR   "white"
#define DEF_PACKET  "defaultPacket"

    sDrfColor defaultColor(DEF_COLOR, 255, 255, 255, false);

    sDrfPacket defaultPacket(DEF_PACKET, DEF_STIPPLE, DEF_LSTYLE,
        DEF_COLOR, DEF_COLOR);
}

// #define DEBUG


// Apply the named packet attributes to the layer desc.
//
void
cTechDrfIn::apply_packet(CDl *ld, const char *pktname)
{
    // Fill outlines that we support.
    enum outl_type { outl_none, outl_thin, outl_thick, outl_dashed };

    const sDrfPacket *pk = find_packet(pktname);
    if (!pk) {
#ifdef DEBUG
        printf("pkt not found %s\n", pktname);
#endif
        c_badpkts = new stringlist(lstring::copy(pktname), c_badpkts);
        pk = find_packet(DEF_PACKET);
    }
    if (!pk)
        pk = &defaultPacket;

    const sDrfStipple *st = 0;
    bool solid = false;
    bool cut = false;
    if (!strcasecmp(pk->stipple_name(), "solid"))
        solid = true;
    else if (!strcasecmp(pk->stipple_name(), "none"))
        ;
    else if (!strcasecmp(pk->stipple_name(), "X"))
        // Should draw 'X' over boxes.
        cut = true;
    else {
        st = find_stipple(pk->stipple_name());
        if (!st) {
#ifdef DEBUG
            printf("stipple not found %s\n", pk->stipple_name());
#endif
            c_badstips = new stringlist(lstring::copy(pk->stipple_name()),
                c_badstips);
            st = find_stipple(DEF_STIPPLE);
        }
        // else none
    }

    const sDrfColor *fc = find_color(pk->fill_color());
    if (!fc) {
#ifdef DEBUG
        printf("color not found %s\n", pk->fill_color());
#endif
        c_badclrs = new stringlist(lstring::copy(pk->fill_color()),
            c_badclrs);
        fc = find_color(DEF_COLOR);
    }
    if (!fc)
        fc = &defaultColor;

    outl_type outl = outl_none;
    if (!strcasecmp(pk->linestyle_name(), "none"))
        ;
    else if (!strcasecmp(pk->linestyle_name(), "solid"))
        outl = outl_thin;
    else {
        const sDrfLine *ls = find_line(pk->linestyle_name());
        if (!ls) {
#ifdef DEBUG
            printf("linestyle not found %s\n", pk->linestyle_name());
#endif
            if (lstring::ciprefix("thick", pk->linestyle_name()))
                outl = outl_thick;
            else
                outl = outl_dashed;
        }
        else {
            if (ls->size() > 1)
                outl = outl_thick;
            else
                outl = outl_dashed;
        }
    }

    // We don't presently support a stipple with a textured outline.

    if (solid)
        outl = outl_none;
    if (outl == outl_dashed && st)
        outl = outl_thin;

    dsp_prm(ld)->set_red(fc->red());
    dsp_prm(ld)->set_green(fc->green());
    dsp_prm(ld)->set_blue(fc->blue());
    int pix = 0;
    DSPmainDraw(DefineColor(&pix, fc->red(), fc->green(), fc->blue()))
    dsp_prm(ld)->set_pixel(pix);

    ld->setFilled(false);
    ld->setOutlined(false);
    ld->setOutlinedFat(false);
    ld->setCut(false);

    if (solid || st)
        ld->setFilled(true);
    if (st) {
        DSPmainDraw(defineFillpattern(dsp_prm(ld)->fill(),
            st->nx(), st->ny(), st->map()))
    }
    if (!solid) {
        if (outl == outl_thick) {
            ld->setOutlined(true);
            ld->setOutlinedFat(true);
        }
        else if (outl == outl_dashed)
            ld->setOutlined(true);
        else if (outl == outl_thin) {
            if (st)
                ld->setOutlined(true);
        }
        if (cut)
            ld->setCut(true);
    }
}


// After loading DRF data, this will generate a report of unresolved
// identifiers, and clear the "bad" lists.
//
void
cTechDrfIn::report_unresolved(sLstr &lstr)
{
    if (c_badpkts) {
        lstr.add(
            "Packet names referenced in technology but not defined in drf:\n");
        SymTab tab(false, false);
        for (stringlist *sl = c_badpkts; sl; sl = sl->next) {
            if (tab.get(sl->string) == ST_NIL)
                tab.add(sl->string, 0, false);
        }
        stringlist *names = tab.names();
        stringlist::sort(names);
        for (stringlist *sl = names; sl; sl = sl->next) {
            lstr.add("    ");
            lstr.add(sl->string);
            lstr.add_c('\n');
        }
        stringlist::destroy(names);
        stringlist::destroy(c_badpkts);
        c_badpkts = 0;
    }
    if (c_badstips) {
        lstr.add(
            "Stipple names referenced in technology but not defined in drf:\n");
        SymTab tab(false, false);
        for (stringlist *sl = c_badstips; sl; sl = sl->next) {
            if (tab.get(sl->string) == ST_NIL)
                tab.add(sl->string, 0, false);
        }
        stringlist *names = tab.names();
        stringlist::sort(names);
        for (stringlist *sl = names; sl; sl = sl->next) {
            lstr.add("    ");
            lstr.add(sl->string);
            lstr.add_c('\n');
        }
        stringlist::destroy(names);
        stringlist::destroy(c_badstips);
        c_badstips = 0;
    }
    if (c_badclrs) {
        lstr.add(
            "Color names referenced in technology but not defined in drf:\n");
        SymTab tab(false, false);
        for (stringlist *sl = c_badclrs; sl; sl = sl->next) {
            if (tab.get(sl->string) == ST_NIL)
                tab.add(sl->string, 0, false);
        }
        stringlist *names = tab.names();
        stringlist::sort(names);
        for (stringlist *sl = names; sl; sl = sl->next) {
            lstr.add("    ");
            lstr.add(sl->string);
            lstr.add_c('\n');
        }
        stringlist::destroy(names);
        stringlist::destroy(c_badclrs);
        c_badclrs = 0;
    }
}


const sDrfColor *
cTechDrfIn::find_color(const char *name)
{
    if (c_color_tab && name) {
        const sDrfColor *c = (sDrfColor*)c_color_tab->get(name);
        if (c != (sDrfColor*)ST_NIL)
            return (c);
    }
    return (0);
}


const sDrfColor *
cTechDrfIn::find_color(int r, int g, int b)
{
    if (c_color_rtab) {
        char *nm = mk_color_name(r, g, b);
        const sDrfColor *c = (sDrfColor*)c_color_rtab->get(nm);
        delete [] nm;
        if (c != (sDrfColor*)ST_NIL)
            return (c);
    }
    return (0);
}


void
cTechDrfIn::add_color(sDrfColor *c)
{
    if (!c || !c->name())
        return;
    if (c_color_tab) {
        SymTabEnt *ent = c_color_tab->get_ent(c->name());
        if (ent) {
            if (ent->stData != (void*)c) {
                sDrfColor *oldc = (sDrfColor*)ent->stData;
                char *nm = mk_color_name(oldc->red(), oldc->green(),
                    oldc->blue());
                SymTabEnt *e2 = c_color_rtab->get_ent(nm);
                if (e2 && e2->stData == (void*)oldc)
                    c_color_rtab->remove(nm);
                delete [] nm;

                delete (sDrfColor*)ent->stData;
                ent->stTag = c->name();
                ent->stData = c;
                nm = mk_color_name(c->red(), c->green(), c->blue());
                if (c_color_rtab->get(nm) == ST_NIL)
                    c_color_rtab->add(nm, c, false);
                else
                    delete [] nm;
            }
            return;
        }
    }
    else {
        c_color_tab = new SymTab(false, false);
        c_color_rtab = new SymTab(true, false);
    }
    c_color_tab->add(c->name(), c, false);
    char *nm = mk_color_name(c->red(), c->green(), c->blue());
    if (c_color_rtab->get(nm) == ST_NIL)
        c_color_rtab->add(nm, c, false);
    else
        delete [] nm;
}


const sDrfStipple *
cTechDrfIn::find_stipple(const char *name)
{
    if (c_stipple_tab && name) {
        const sDrfStipple *s = (sDrfStipple*)c_stipple_tab->get(name);
        if (s != (sDrfStipple*)ST_NIL)
            return (s);
    }
    return (0);
}


const sDrfStipple *
cTechDrfIn::find_stipple(const GRfillType *fill)
{
    if (c_stipple_rtab) {
        unsigned char *map = fill->newBitmap();
        char *nm = mk_stp_str(fill->nX(), fill->nY(), map);
        delete [] map;
        const sDrfStipple *s = (sDrfStipple*)c_stipple_rtab->get(nm);
        delete [] nm;
        if (s != (sDrfStipple*)ST_NIL)
            return (s);
    }
    return (0);
}


void
cTechDrfIn::add_stipple(sDrfStipple *s)
{
    if (!s || !s->name())
        return;
    if (c_stipple_tab) {
        SymTabEnt *ent = c_stipple_tab->get_ent(s->name());
        if (ent) {
            if (ent->stData != (void*)s) {
                sDrfStipple *olds = (sDrfStipple*)ent->stData;
                char *nm = mk_stp_str(olds->nx(), olds->ny(), olds->map());
                SymTabEnt *e2 = c_stipple_rtab->get_ent(nm);
                if (e2 && e2->stData == (void*)olds)
                    c_stipple_rtab->remove(nm);
                delete [] nm;

                delete (sDrfStipple*)ent->stData;
                ent->stTag = s->name();
                ent->stData = s;
                nm = mk_stp_str(s->nx(), s->ny(), s->map());
                if (c_stipple_rtab->get(nm) == ST_NIL)
                    c_stipple_rtab->add(nm, s, false);
                else
                    delete [] nm;
            }
            return;
        }
    }
    else {
        c_stipple_tab = new SymTab(false, false);
        c_stipple_rtab = new SymTab(true, false);
    }
    c_stipple_tab->add(s->name(), s, false);
    char *nm = mk_stp_str(s->nx(), s->ny(), s->map());
    if (c_stipple_rtab->get(nm) == ST_NIL)
        c_stipple_rtab->add(nm, s, false);
    else
        delete [] nm;
}


const sDrfLine *
cTechDrfIn::find_line(const char *name)
{
    if (c_line_tab && name) {
        const sDrfLine *l = (sDrfLine*)c_line_tab->get(name);
        if (l != (sDrfLine*)ST_NIL)
            return (l);
    }
    return (0);
}


void
cTechDrfIn::add_line(sDrfLine *l)
{
    if (!l || !l->name())
        return;
    if (c_line_tab) {
        SymTabEnt *ent = c_line_tab->get_ent(l->name());
        if (ent) {
            if (ent->stData != (void*)l) {
                delete (sDrfLine*)ent->stData;
                ent->stTag = l->name();
                ent->stData = l;
            }
            return;
        }
    }
    else
        c_line_tab = new SymTab(false, false);
    c_line_tab->add(l->name(), l, false);
}


const sDrfPacket *
cTechDrfIn::find_packet(const char *name)
{
    if (c_packet_tab && name) {
        const sDrfPacket *p = (sDrfPacket*)c_packet_tab->get(name);
        if (p != (sDrfPacket*)ST_NIL)
            return (p);
    }
    return (0);
}


const sDrfPacket *
cTechDrfIn::find_packet(const char *color, const char *stipple,
    const char *line, const char *outline)
{
    if (c_packet_rtab) {
        char *nm = mk_packet_name(color, stipple, line, outline);
        const sDrfPacket *p = (sDrfPacket*)c_packet_rtab->get(nm);
        delete [] nm;
        if (p != (sDrfPacket*)ST_NIL)
            return (p);
    }
    return (0);
}


void
cTechDrfIn::add_packet(sDrfPacket *p)
{
    if (!p || !p->name())
        return;
    if (c_packet_tab) {
        SymTabEnt *ent = c_packet_tab->get_ent(p->name());
        if (ent) {
            if (ent->stData != (void*)p) {
                sDrfPacket *oldp = (sDrfPacket*)ent->stData;
                char *nm = mk_packet_name(oldp->fill_color(),
                    oldp->stipple_name(), oldp->linestyle_name(),
                    oldp->outline_color());
                SymTabEnt *e2 = c_packet_rtab->get_ent(nm);
                if (e2 && e2->stData == (void*)oldp)
                    c_packet_rtab->remove(nm);
                delete [] nm;

                delete (sDrfPacket*)ent->stData;
                ent->stTag = p->name();
                ent->stData = p;
                nm = mk_packet_name(p->fill_color(), p->stipple_name(),
                    p->linestyle_name(), p->outline_color());
                if (c_packet_rtab->get(nm) == ST_NIL)
                    c_packet_rtab->add(nm, p, false);
                else
                    delete [] nm;
            }
            return;
        }
    }
    else {
        c_packet_tab = new SymTab(false, false);
        c_packet_rtab = new SymTab(true, false);
    }
    c_packet_tab->add(p->name(), p, false);
    char *nm = mk_packet_name(p->fill_color(), p->stipple_name(),
        p->linestyle_name(), p->outline_color());
    if (c_packet_rtab->get(nm) == ST_NIL)
        c_packet_rtab->add(nm, p, false);
    else
        delete [] nm;
}


// Static function.
//
// Utility to create a color name from r/g/b values.
//
char *
cTechDrfIn::mk_color_name(int r, int g, int b)
{
    char buf[32];
    snprintf(buf, 32,  "c%02x%02x%02x", r, g, b);
    return (lstring::copy(buf));
}


// Static function.
//
char *
cTechDrfIn::mk_stp_str(unsigned int nx, unsigned int ny,
    const unsigned char *map)
{
    sLstr lstr;
    const unsigned char *a = map;
    int bpl = (nx + 7)/8;
    for (unsigned int i = 0; i < ny; i++) {
        unsigned int d = *a++;
        for (int j = 1; j < bpl; j++)
            d |= *a++ <<  8*j;

        unsigned int mask = 1;
        for (unsigned int j = 0; j < nx; j++) {
            lstr.add_c((d & mask) ? '1' : '0');
            mask <<= 1;
            if (j == nx - 1)
                lstr.add(")\n");
        }
    }
    return (lstr.string_trim());
}


// Static function.
//
// Construct a packet name of the form
//
//  color [ stipple ][ line ][ _[S][L][N][B] ]
//
// where
//
//   color
//
// specifies the fill color and/or the outline color in the following
// format:
//
//   [ fill ] [ outline ]
//
// At least one color name must be specified.  Specify colors
// according to the following rules:  When the fill and outline colors
// are different, specify the fill color followed by the outline
// color.  When the fill and outline colors are the same, specify the
// one color.  When the fill and outline colors are the same, but the
// outline color is blinking (as defined with drDefineColor), specify
// the one color when the outline color name is the same as the fill
// color name with a B appended (for example, white and whiteB).  If
// the outline color name is constructed in any other way (for
// example, white2), specify both colors (whitewhite2).
//
//   stipple
//
// Specifies a stipple pattern name.  This field is optional; no entry
// indicates "none", no stipple pattern.
//
//   line
//
// Specifies a line style name.  Default when the packet name does not
// have the N extension:  solid Default when the packet name does have
// the N extension:  none
//
//   _SLNB extension
//
// S indicates that the stipple pattern name is specified and the line
//   style name is the default (solid).
// SN indicates that the stipple pattern name is specified and the line
//   style name is none.
// L indicates that the line style name is specified and the stipple
//   pattern name is the default (none).
// N indicates that the line style name is none.
// B indicates that the outline color is a blinking color. Only the
//   outline color can be a blinking color; the fill color must be
//   nonblinking. (See "drDefineColor" for information on defining
//   colors.)
//
char *
cTechDrfIn::mk_packet_name(const char *color, const char *stipple,
    const char *line, const char *outline)
{
    if (!color || !*color)
        color = "white";
    if (!stipple || !*stipple)
        stipple = "none";
    if (!line || !*line)
        line = "solid";
    if (!outline || !*outline)
        outline = color;

    // No blinking.  In Xic the outline and fill are always the same
    // color, but we'll keep the defference here for possible future
    // support.

    sLstr lstr;
    lstr.add(color);
    if (outline != color && strcmp(outline, color))
        lstr.add(outline);

    bool has_stip = false;
    if (strcmp(stipple, "none")) {
        lstr.add(stipple);
        has_stip = true;
    }
    bool has_line = false;
    if (strcmp(line, "none") && strcmp(line, "solid")) {
        lstr.add(line);
        has_line = true;
    }
    if (has_stip && !has_line && !strcmp(line, "solid"))
        lstr.add("_S");
    else if (has_stip && !has_line && !strcmp(line, "none"))
        lstr.add("_SN");
    else if (!has_stip && has_line)
        lstr.add("_L");
    else if (!strcmp(line, "none"))
        lstr.add("_N");

    return (lstr.string_trim());
}


namespace {
    const char *compat = "compat";

    void err_rpt(const char *name, lispnode *p)
    {
        if (!p) {
            if (name)
                Log()->WarningLogV(compat, "Ignored bad node in %s.\n", name);
            else
                Log()->WarningLogV(compat, "Ignored bad node.\n");
        }
        else {
            sLstr lstr;
            lispnode::print(p, &lstr);
            if (name)
                Log()->WarningLogV(compat, "Ignored bad node in %s:\n\t%s\n",
                    name, lstr.string());
            else
                Log()->WarningLogV(compat, "Ignored bad node:\n\t%s\n",
                    lstr.string());
        }
    }
}


// Static function
bool
cTechDrfIn::drDefineDisplay(lispnode *p0, lispnode*, char **err)
{
    for (lispnode *p = p0->args; p; p = p->next) {
        lispnode n[1];
        int cnt = lispnode::eval_list(p->args, n, 1, err);
        if (cnt < 1) {
            err_rpt("drDefineDisplay", p);
            continue;
        }
        if (n[0].type != LN_STRING) {
            err_rpt("drDefineDisplay", p);
            continue;
        }
        if (!DrfIn()->display()) {
            DrfIn()->set_display(n[0].string);
            break;
        }
    }

    return (true);
}


// Static function
bool
cTechDrfIn::drDefineColor(lispnode *p0, lispnode*, char **err)
{
    for (lispnode *p = p0->args; p; p = p->next) {
        lispnode n[6];
        int cnt = lispnode::eval_list(p->args, n, 6, err);
        if (cnt < 5) {
            err_rpt("drDefineColor", p);
            continue;
        }
        if (n[0].type != LN_STRING || n[1].type != LN_STRING ||
                n[2].type != LN_NUMERIC || n[3].type != LN_NUMERIC ||
                n[4].type != LN_NUMERIC) {
            err_rpt("drDefineColor", p);
            continue;
        }
        char *dname = n[0].string;  // display name
        if (strcmp(dname, DrfIn()->display()))
            continue;
        char *cname = n[1].string;  // color name
        int r = (int)n[2].value;    // red
        int g = (int)n[3].value;    // green
        int b = (int)n[4].value;    // blue
        if (r < 0 || g < 0 || b < 0) {
            err_rpt("drDefineColor", p);
            continue;
        }
        bool blink = (cnt == 6 && !n[5].is_nil());
        if (r > 255 || g > 255 || b > 255) {
            err_rpt("drDefineColor", p);
            continue;
        }
        DrfIn()->add_color(new sDrfColor(cname, r, g, b, blink));
    }
    return (true);
}


// Static function
bool
cTechDrfIn::drDefineStipple(lispnode *p0, lispnode*, char **err)
{
    for (lispnode *p = p0->args; p; p = p->next) {
        lispnode n[3];
        int cnt = lispnode::eval_list(p->args, n, 3, err);
        if (cnt < 3) {
            err_rpt("drDefineStipple", 0);
            continue;
        }
        if (n[0].type != LN_STRING || n[1].type != LN_STRING ||
                n[2].type != LN_NODE) {
            err_rpt("drDefineStipple", 0);
            continue;
        }
        char *dname = n[0].string;    // display name
        if (strcmp(dname, DrfIn()->display()))
            continue;
        char *sname = n[1].string;    // stipple name
        if (!sname) {
            err_rpt("drDefineStipple", 0);
            continue;
        }

        int ny = n[2].arg_cnt();
        int nx = n[2].args->arg_cnt();
        int bpl = (nx + 7)/8;
        unsigned char *map = new unsigned char[ny*bpl];
        unsigned char *a = map;
        for (lispnode *q = n[2].args; q; q = q->next) {
            unsigned int d = 0;
            unsigned int mask = 0x1;
            for (lispnode *b = q->args; b; b = b->next) {
                if (b->value != 0)
                    d |= mask;
                mask <<= 1;
            }
            *a++ = d;
            for (int i = 1; i < bpl; i++)
                *a++ = d >> 8*i;
        }
        DrfIn()->add_stipple(new sDrfStipple(sname, nx, ny, map));
    }
    return (true);
}


// Static function
bool
cTechDrfIn::drDefineLineStyle(lispnode *p0, lispnode*, char **err)
{
    for (lispnode *p = p0->args; p; p = p->next) {
        lispnode n[4];
        int cnt = lispnode::eval_list(p->args, n, 4, err);
        if (cnt < 4) {
            err_rpt("drDefineLineStyle", p);
            continue;
        }
        if (n[0].type != LN_STRING || n[1].type != LN_STRING ||
                n[2].type != LN_NUMERIC) {
            err_rpt("drDefineLineStyle", p);
            continue;
        }
        if (n[3].type != LN_NODE && n[3].type != LN_NUMERIC) {
            // If the fourth argument has a form like "(1)" it is taken
            // as numeric.
            err_rpt("drDefineLineStyle", p);
            continue;
        }
        if (strcmp(n[0].string, DrfIn()->display()))
            continue;
        char *name = n[1].string;       // linestyle name
        if (!strcmp(name, "solid") || !strcmp(name, "none"))
            continue;
        int size = (int)n[2].value;     // line width
        if (n[3].type == LN_NUMERIC) {
            if (n[3].value != 0)
                DrfIn()->add_line(new sDrfLine(name, size, 1, 1));
            else
                DrfIn()->add_line(new sDrfLine(name, size, 1, 0));
        }
        else {
            int len = n[3].arg_cnt(); // pattern length;
            unsigned int d = 0;
            unsigned int mask = 1 << (len - 1);
            for (lispnode *q = n[3].args; q; q = q->next) {
                if (q->value != 0)
                    d |= mask;
                mask >>= 1;
            }
            DrfIn()->add_line(new sDrfLine(name, size, len, d));
        }
    }
    return (true);
}


// Static function
bool
cTechDrfIn::drDefinePacket(lispnode *p0, lispnode*, char **err)
{
    for (lispnode *p = p0->args; p; p = p->next) {
        lispnode n[6];
        int cnt = lispnode::eval_list(p->args, n, 6, err);
        if (cnt < 6) {
            err_rpt("drDefinePacket", p);
            continue;
        }
        if (n[0].type != LN_STRING || n[1].type != LN_STRING ||
                n[2].type != LN_STRING || n[3].type != LN_STRING ||
                n[4].type != LN_STRING || n[5].type != LN_STRING) {
            err_rpt("drDefinePacket", p);
            continue;
        }
        if (strcmp(n[0].string, DrfIn()->display()))
            continue;

        const char *name = n[1].string;     // packet name
        const char *sname = n[2].string;    // stipple name
        const char *lname = n[3].string;    // linestyle name
        const char *fc = n[4].string;       // fill color
        const char *oc = n[5].string;       // outline color
        DrfIn()->add_packet(new sDrfPacket(name, sname, lname, fc, oc));
    }
    return (true);
}


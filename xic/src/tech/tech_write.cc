
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2017 Whiteley Research Inc, all rights reserved.        *
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
 $Id: tech_write.cc,v 1.2 2017/03/16 05:19:40 stevew Exp $
 *========================================================================*/

#include "dsp.h"
#include "dsp_layer.h"
#include "fio.h"
#include "cd_lgen.h"
#include "tech.h"
#include "tech_kwords.h"
#include "tech_via.h"
#include "main_variables.h"
#include "si_macro.h"


//
// Functions to write a technology file.
//

// Print the contents of the technology file. The order is:
//   !Set and set lines
//   Macros
//   Paths
//   Electrical layers
//   User DRC rules
//   Physical layers
//   Device blocks
//   Scripts
//   Attributes
//   Print driver blocks
//
void
cTech::Print(FILE *techfp)
{
    const char *sep =
        "#--------------------------------------------------------------"
        "----------------\n";

    // Print header comments.
    CommentDump(techfp, 0, tBlkNone, 0, 0);
    fprintf(techfp, "\n");

    if (TechnologyName() && *TechnologyName())
        fprintf(techfp, "%s %s\n", Tkw.Technology(), TechnologyName());
    CommentDump(techfp, 0, tBlkNone, 0, Tkw.Technology());
    fprintf(techfp, "\n");

    // Print !Set lines.
    if (tc_bangset_lines) {
        fprintf(techfp, "# original !Set lines\n");
        for (stringlist *sl = tc_bangset_lines; sl; sl = sl->next) {
            fputs("# !Set ", techfp);
            fputs(sl->string, techfp);
            putc('\n', techfp);
        }
        putc('\n', techfp);
    }

    stringlist *sl0 = VarList();
    if (sl0) {
        fprintf(techfp, "# current !Set variable status\n");
        for (stringlist *sl = sl0; sl; sl = sl->next) {
            fputs("!Set ", techfp);
            fputs(sl->string, techfp);
            putc('\n', techfp);
        }
        stringlist::destroy(sl0);
    }

    CommentDump(techfp, 0, tBlkNone, 0, "!Set");
    if (tc_bangset_lines)
        fprintf(techfp, "\n");

    // Print the "set name = value" lines.
    if (tc_variable_tab && tc_variable_tab->allocated()) {
        stringlist *s0 = SymTab::names(tc_variable_tab);
        for (stringlist *sl = s0; sl; sl = sl->next) {
            const char *val =
                (const char*)SymTab::get(tc_variable_tab, sl->string);
            fprintf(techfp, "%s %s = %s\n", Tkw.Set(), sl->string, val);
        }
        CommentDump(techfp, 0, tBlkNone, 0, Tkw.Set());
        stringlist::destroy(s0);
        fprintf(techfp, "\n");
    }

    // Print Defines.
    if (tc_tech_macros) {
        tc_tech_macros->print(techfp, Tkw.Define(), false);
        CommentDump(techfp, 0, tBlkNone, 0, Tkw.Define());
        fprintf(techfp, "\n");
    }

    // Print paths.
    if (FIO()->PGetPath())
        fprintf(techfp, "%s %s\n", Tkw.Path(), FIO()->PGetPath());
    CommentDump(techfp, 0, tBlkNone, 0, Tkw.Path());
    if (CDvdb()->getVariable(VA_LibPath))
        fprintf(techfp, "%s %s\n", Tkw.LibPath(),
            CDvdb()->getVariable(VA_LibPath));
    CommentDump(techfp, 0, tBlkNone, 0, Tkw.LibPath());
    if (CDvdb()->getVariable(VA_HelpPath))
        fprintf(techfp, "%s %s\n", Tkw.HelpPath(),
            CDvdb()->getVariable(VA_HelpPath));
    CommentDump(techfp, 0, tBlkNone, 0, Tkw.HelpPath());
    if (CDvdb()->getVariable(VA_ScriptPath))
        fprintf(techfp, "%s %s\n", Tkw.ScriptPath(),
            CDvdb()->getVariable(VA_ScriptPath));
    CommentDump(techfp, 0, tBlkNone, 0, Tkw.ScriptPath());
    fprintf(techfp, "\n");

    // Print mapped layer names.
    string2list *s2 = CDldb()->listAliases();
    for (string2list *s = s2; s; s = s->next) {
        fprintf(techfp, "%-16s %-22s %s\n", Tkw.MapLayer(),
            s->string, s->value);
    }
    CommentDump(techfp, 0, tBlkNone, 0, Tkw.MapLayer());
    if (s2) {
        fprintf(techfp, "\n");
        string2list::destroy(s2);
    }

    // Print layer and purpose tables.
    stringnumlist *s0 = CDldb()->listOAlayerTab();
    for (stringnumlist *sl = s0; sl; sl = sl->next) {
        fprintf(techfp, "%-16s %-22s %d\n", Tkw.DefineLayer(),
            sl->string, sl->num);
    }
    CommentDump(techfp, 0, tBlkNone, 0, Tkw.DefineLayer());
    if (s0) {
        fprintf(techfp, "\n");
        stringnumlist::destroy(s0);
    }
    s0 = CDldb()->listOApurposeTab();
    for (stringnumlist *sl = s0; sl; sl = sl->next) {
        fprintf(techfp, "%-16s %-22s %d\n", Tkw.DefinePurpose(),
            sl->string, sl->num);
    }
    CommentDump(techfp, 0, tBlkNone, 0, Tkw.DefineLayer());
    if (s0) {
        fprintf(techfp, "\n");
        stringnumlist::destroy(s0);
    }

    // Print electrical layers.
    if (CDldb()->layersUsed(Electrical) > 1) {
        fputs(sep, techfp);
        fprintf(techfp, "# Electrical Layers\n\n");
        CDlgen lgen(Electrical);
        CDl *ld;
        while ((ld = lgen.next()) != 0) {
            if (ld->layerType() != CDLnormal)
                continue;
            PrintLayerBlock(techfp, 0, true, ld, Electrical);
            fprintf(techfp,"\n");
        }
        fputs(sep, techfp);
        fputs("\n", techfp);
    }

    // User-defined DRC rules.
    if (tc_print_user_rules)
        (*tc_print_user_rules)(techfp);

    // Print physical layers.
    if (CDldb()->layersUsed(Physical) > 1) {
        fputs(sep, techfp);
        fprintf(techfp, "# Physical Layers\n\n");
        CDlgen lgen(Physical);
        CDl *ld;
        while ((ld = lgen.next()) != 0) {
            if (ld->layerType() != CDLnormal)
                continue;
            PrintLayerBlock(techfp, 0, true, ld, Physical);
            fprintf(techfp, "\n");
        }
        fputs(sep, techfp);
        fputs("\n", techfp);
    }

    // Print Invalid layers
    if (CDldb()->invalidLayers()) {
        fputs(sep, techfp);
        fprintf(techfp, "# Invalid Layers\n\n");
        for (CDll *ll = CDldb()->invalidLayers(); ll; ll = ll->next) {
            if (ll->ldesc->layerType() != CDLnormal)
                continue;
            PrintLayerBlock(techfp, 0, true, ll->ldesc, Physical);
            fprintf(techfp, "\n");
        }
        fputs(sep, techfp);
        fputs("\n", techfp);
    }

    // Print derived layers.
    CDl **ary = CDldb()->listDerivedLayers();
    if (ary) {
        for (CDl **a = ary; *a; a++) {
            const char *mstr = 0;
            switch ((*a)->drvMode()) {
            case CLdefault:
                break;
            case CLsplitH:
                mstr = "split";
                break;
            case CLsplitV:
                mstr = "splitv";
                break;
            case CLjoin:
                mstr = "join";
                break;
            }
            if (mstr)
                fprintf(techfp, "%s %-14s %s %s\n", Tkw.DerivedLayer(),
                    (*a)->name(), mstr, (*a)->drvExpr());
            else
                fprintf(techfp, "%s %-14s %s\n", Tkw.DerivedLayer(),
                    (*a)->name(), (*a)->drvExpr());
        }
        delete [] ary;
        fputs("\n", techfp);
    }
    // End of layer definitions.

    // Print standard via definitions
    if (tc_std_vias && tc_std_vias->allocated()) {
        fputs(sep, techfp);
        fprintf(techfp, "# Standard Via Definitions\n\n");
        sStdViaList *vl = StdViaList();
        for (sStdViaList *sv = vl; sv; sv = sv->next)
            sv->std_via->tech_print(techfp);
        sStdViaList::destroy(vl);
        CommentDump(techfp, 0, tBlkNone, 0, Tkw.StandardVia());
        fprintf(techfp, "\n");
    }

    // Print Device blocks
    if (tc_print_devices)
        (*tc_print_devices)(techfp);

    // Print scripts
    if (tc_print_scripts)
        (*tc_print_scripts)(techfp);

    // Print attributes
    fputs(sep, techfp);
    fprintf(techfp, "# Misc. Parameters and Attributes\n\n");
    PrintAttributes(techfp);
    fputs("\n", techfp);
    fputs(sep, techfp);
    fputs("\n", techfp);

    // Print hard copy defaults
    fputs(sep, techfp);
    fprintf(techfp, "# Print Driver Blocks\n\n");
    PrintHardcopy(techfp);
    fputs(sep, techfp);
    fprintf(techfp, "# End of file\n\n");
}


void
cTech::PrintLayerBlock(FILE *fp, sLstr *lstr, bool cmts, const CDl *ld,
    DisplayMode mode)
{
    if (!ld)
        return;
    tBlkType tbt = mode == Physical ? tBlkPlyr : tBlkElyr;
    char buf[256];

    // LayerName
    PutStr(fp, lstr, mode == Physical ? Tkw.PhysLayer() : Tkw.ElecLayer());
    PutChr(fp, lstr, ' ');
    PutStr(fp, lstr, ld->name());
    PutChr(fp, lstr, '\n');
    if (cmts) {
        if (mode == Physical) {
            CommentDump(fp, lstr, tbt, ld->name(), Tkw.PhysLayerName());
            CommentDump(fp, lstr, tbt, ld->name(), Tkw.PhysLayer());
            CommentDump(fp, lstr, tbt, ld->name(), Tkw.LayerName());
            CommentDump(fp, lstr, tbt, ld->name(), Tkw.Layer());
        }
        else {
            CommentDump(fp, lstr, tbt, ld->name(), Tkw.ElecLayer());
            CommentDump(fp, lstr, tbt, ld->name(), Tkw.ElecLayerName());
        }
    }

    // LPP name
    if (ld->lppName()) {
        PutStr(fp, lstr, Tkw.LppName());
        PutChr(fp, lstr, ' ');
        PutStr(fp, lstr, ld->lppName());
        PutChr(fp, lstr, '\n');
    }
    if (cmts)
        CommentDump(fp, lstr, tbt, ld->lppName(), Tkw.LppName());

    // Description
    if (ld->description()) {
        PutStr(fp, lstr, Tkw.Description());
        PutChr(fp, lstr, ' ');
        PutStr(fp, lstr, ld->description());
        PutChr(fp, lstr, '\n');
    }
    if (cmts)
        CommentDump(fp, lstr, tbt, ld->description(), Tkw.Description());

    // Conversion parameters.
    PrintCvtLayerBlock(fp, lstr, cmts, ld, mode);

    DspLayerParams *lp = dsp_prm(ld);

    // RGB
    sprintf(buf, "%s %d %d %d\n", Tkw.kwRGB(),
        lp->red(), lp->green(), lp->blue());
    PutStr(fp, lstr, buf);
    if (cmts)
        CommentDump(fp, lstr, tbt, ld->name(), Tkw.kwRGB());

    // Filled
    PutStr(fp, lstr, Tkw.Filled());
    if (ld->isFilled() && !lp->fill()->hasMap())
        PutStr(fp, lstr, " y\n");  // solid
    else {
        if (!ld->isFilled())
            PutStr(fp, lstr, " n"); // empty
        else {
            PutStr(fp, lstr, " \\\n");
            unsigned char *map = lp->fill()->newBitmap();
            PrintPmap(fp, lstr, map, lp->fill()->nX(), lp->fill()->nY());
            delete [] map;
        }
        if (ld->isOutlined()) {
            if (ld->isOutlinedFat())
                PutStr(fp, lstr, " fat");
            else
                PutStr(fp, lstr, " outline");
        }
        if (ld->isCut())
            PutStr(fp, lstr, " cut\n");
        else
            PutChr(fp, lstr, '\n');
    }
    if (cmts)
        CommentDump(fp, lstr, tbt, ld->name(), Tkw.Filled());

    // Invisible
    if (ld->isRstInvisible()) {
        PutStr(fp, lstr, Tkw.Invisible());
        PutChr(fp, lstr, '\n');
    }
    if (cmts)
        CommentDump(fp, lstr, tbt, ld->name(), Tkw.Invisible());

    // NoSelect
    if (ld->isRstNoSelect()) {
        PutStr(fp, lstr, Tkw.NoSelect());
        PutChr(fp, lstr, '\n');
    }
    if (cmts)
        CommentDump(fp, lstr, tbt, ld->name(), Tkw.NoSelect());

    // WireActive
    if (ld->isWireActive() && strcmp(ld->name(), "SCED")) {
        PutStr(fp, lstr, Tkw.WireActive());
        PutChr(fp, lstr, '\n');
    }
    if (cmts)
        CommentDump(fp, lstr, tbt, ld->name(), Tkw.WireActive());

    // NoInstView
    if (ld->isNoInstView()) {
        PutStr(fp, lstr, Tkw.NoInstView());
        PutChr(fp, lstr, '\n');
    }
    if (cmts)
        CommentDump(fp, lstr, tbt, ld->name(), Tkw.NoInstView());

    // Invalid
    if (ld->isInvalid()) {
        PutStr(fp, lstr, Tkw.Invalid());
        PutChr(fp, lstr, '\n');
    }
    if (cmts)
        CommentDump(fp, lstr, tbt, ld->name(), Tkw.Invalid());

    // Blinkers?
    if (ld->isBlink()) {
        PutStr(fp, lstr, Tkw.Blink());
        PutChr(fp, lstr, '\n');
    }
    if (cmts)
        CommentDump(fp, lstr, tbt, ld->name(), Tkw.Blink());

    // WireWidth
    if (lp->wire_width() > 0) {
        sprintf(buf, "%s %.4f\n", Tkw.WireWidth(),
            MICRONS(lp->wire_width()));
        PutStr(fp, lstr, buf);
    }
    if (cmts)
        CommentDump(fp, lstr, tbt, ld->name(), Tkw.WireWidth());

    if (mode == Physical) {
        // CrossThick
        if (lp->xsect_thickness() > 0) {
            sprintf(buf, "%s %.4f\n", Tkw.CrossThick(),
                MICRONS(lp->xsect_thickness()));
            PutStr(fp, lstr, buf);
        }
        if (cmts)
            CommentDump(fp, lstr, tbt, ld->name(), Tkw.CrossThick());

        // Symbolic
        if (ld->isSymbolic()) {
            PutStr(fp, lstr, Tkw.Symbolic());
            PutChr(fp, lstr, '\n');
        }
        if (cmts)
            CommentDump(fp, lstr, tbt, ld->name(), Tkw.Symbolic());

        // NoMerge
        if (ld->isNoMerge()) {
            PutStr(fp, lstr, Tkw.NoMerge());
            PutChr(fp, lstr, '\n');
        }
        if (cmts)
            CommentDump(fp, lstr, tbt, ld->name(), Tkw.NoMerge());

        // Extraction support info.
        PrintExtLayerBlock(fp, lstr, cmts, ld);

        // Design rules
        if (tc_print_rules)
            (*tc_print_rules)(fp, lstr, cmts, ld);
    }
}


// Print the hex values of the bitmap to fp or lstr.
//
bool
cTech::PrintPmap(FILE *fp, sLstr *lstr, unsigned char *map, int nx, int ny)
{
    if (!map)
        return (false);
    bool no_pmap = CDvdb()->getVariable(VA_TechNoPrintPatMap);
    int bpl = (nx+7)/8;
    unsigned char *a = map;
    char buf[256];
    for (int i = 0; i < ny; i++) {
        unsigned int d = *a++;
        for (int j = 1; j < bpl; j++)
            d |= *a++ <<  8*j;
        char *bp = buf;
        if (no_pmap) {
            if (i == 0) {
                if ((nx != 8 && nx != 16) || (ny != 8 && ny != 16)) {
                    sprintf(bp, " x=%d y=%d\\\n", nx, ny);
                    while (*bp)
                        bp++;
                }
            }
            if (nx <= 8)
                sprintf(bp, " %02x", d);
            else if (nx <= 16)
                sprintf(bp, " %04x", d);
            else
                sprintf(bp, " %x", d);
            if ((i == 7 || i == 15 || i == 23) && i != ny-1) {
                while (*bp)
                    bp++;
                strcpy(bp, " \\\n");
            }
        }
        else {
            *bp++ = ' ';
            *bp++ = ' ';
            *bp++ = '|';
            unsigned int mask = 1;
            for (int j = 0; j < nx; j++) {
                *bp++ = (d & mask) ? '.' : ' ';
                mask <<= 1;
            }
            *bp++ = '|';
            *bp++ = ' ';
            *bp++ = ' ';
            if (i == ny-1)
                sprintf(bp, "(0x%x)", d);
            else
                sprintf(bp, "(0x%x) \\\n", d);
        }
        PutStr(fp, lstr, buf);
    }
    return (true);
}


namespace {
    bool match(const char *s1, const char *s2)
    {
        if (s1 && s2 && lstring::cieq(s1, s2))
            return (true);
        if (!s1 && !s2)
            return (true);
        return (false);
    }
}


void
cTech::CommentDump(FILE *fp, sLstr *lstr, tBlkType t, const char *blk,
    const char *kw, const char *subkw)
{
    if (!kw && !subkw && !blk && t == tBlkNone) {
        for (sTcomment *c = tc_comments; c; c = c->next)
            c->dumped = false;
    }
    const char *ckw = "Comment";
    for (sTcomment *c = tc_comments; c; c = c->next) {
        if (c->dumped)
            continue;
        sTcx *ccx = c->cx;
        if (t == ccx->type() || (t == tBlkNone && ccx->type() != tBlkNone &&
                ccx->type() != tBlkHcpy)) {
            if (!blk ||
                    (ccx->blkname() && lstring::cieq(blk, ccx->blkname()))) {
                if (match(kw, ccx->key()) &&
                        (!subkw || match(subkw, ccx->subkey()))) {
                    if (lstr) {
                        lstr->add("Comment ");
                        lstr->add(c->text);
                        lstr->add_c('\n');
                    }
                    else if (fp)
                        fprintf(fp, "Comment %s\n", c->text);
                    kw = ckw;
                    c->dumped = true;
                }
                else if (kw == ckw)
                    break;
            }
        }
    }
}


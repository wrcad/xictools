
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

#include "main.h"
#include "ext.h"
#include "ext_extract.h"
#include "ext_fc.h"
#include "ext_fh.h"
#include "ext_antenna.h"
#include "ext_net_dump.h"
#include "edit.h"
#include "sced.h"
#include "cd_celldb.h"
#include "cd_digest.h"
#include "dsp_layer.h"
#include "dsp_tkif.h"
#include "dsp_inlines.h"
#include "fio.h"
#include "fio_chd.h"
#include "tech_layer.h"
#include "promptline.h"
#include "errorlog.h"
#include "events.h"
#include "tech.h"
#include "miscutil/filestat.h"


//-----------------------------------------------------------------------------
// Extraction 'bang' commands
//

namespace {
    namespace ext_bangcmds {

        // Extract
        void antenna(const char*);
        void netext(const char*);
        void addcells(const char*);
        void find(const char*);
        void ptrms(const char*);
        void ushow(const char*);
        void fc(const char*);
        void fh(const char*);
        // Undocumented
        void updlabels(const char*);
        void updmeas(const char*);
    }
}


void
cExt::setupBangCmds()
{
    // Extract
    XM()->RegisterBangCmd("antenna", &ext_bangcmds::antenna);
    XM()->RegisterBangCmd("netext", &ext_bangcmds::netext);
    XM()->RegisterBangCmd("addcells", &ext_bangcmds::addcells);
    XM()->RegisterBangCmd("find", &ext_bangcmds::find);
    XM()->RegisterBangCmd("ptrms", &ext_bangcmds::ptrms);
    XM()->RegisterBangCmd("ushow", &ext_bangcmds::ushow);
    XM()->RegisterBangCmd("fc", &ext_bangcmds::fc);
    XM()->RegisterBangCmd("fh", &ext_bangcmds::fh);
    XM()->RegisterBangCmd("updlabels", &ext_bangcmds::updlabels);
    XM()->RegisterBangCmd("updmeas", &ext_bangcmds::updmeas);
}


//-----------------------------------------------------------------------------
// Extract

void
ext_bangcmds::antenna(const char *s)
{
    const char *usage =
        "Usage: !antenna [layer_name layer_min_ratio]... [min_ratio]";

    CDs *sdesc = CDcdb()->findCell(DSP()->CurCellName(), Physical);
    if (!sdesc) {
        PL()->ShowPrompt("No current physical cell!");
        return;
    }
    if (!EX()->extract(sdesc)) {
        PL()->ShowPrompt("Extraction failed!");
        return;
    }

    struct ant_lyr
    {
        ant_lyr(const CDl *ld, double r)
            { ldesc = ld; ratio = r; next = 0; }

        const CDl *ldesc;
        double ratio;
        ant_lyr *next;
    };
    ant_lyr *al0 = 0, *ale = 0;

    CDlgen lgen(Physical);
    CDl *ld;
    while ((ld = lgen.next()) != 0) {
        double aa = tech_prm(ld)->ant_ratio();
        if (aa > 0.0) {
            if (!al0)
                al0 = ale = new ant_lyr(ld, aa);
            else {
                ale->next = new ant_lyr(ld, aa);
                ale = ale->next;
            }
        }
    }

    double totlim = Tech()->AntennaTotal();
    bool limset = false;
    const char *sbak = s;
    char *tok = lstring::gettok(&s);
    if (!tok) {
        sLstr lstr;
        char buf[64];
        for (ant_lyr *a = al0; a; a = a->next) {
            snprintf(buf, sizeof(buf), " %s %g", a->ldesc->name(), a->ratio);
            lstr.add(buf);
        }
        if (totlim > 0.0) {
            snprintf(buf, sizeof(buf), " %g", totlim);
            lstr.add(buf);
        }
        if (lstr.length()) {
            char *in = PL()->EditPrompt("!antenna ", lstr.string());
            if (!in) {
                while (al0) {
                    ant_lyr *a = al0;
                    al0 = al0->next;
                    delete a;
                }
                PL()->ErasePrompt();
                return;
            }
            s = in;
        }
        else
            s = sbak;
    }
    else {
        s = sbak;
        delete [] tok;
    }
    while ((tok = lstring::gettok(&s)) != 0) {
        double d;
        if ((ld = CDldb()->findLayer(tok, Physical)) != 0) {
            char *tok2 = lstring::gettok(&s);
            if (tok2 && sscanf(tok2, "%lf", &d) == 1 && d >= 0.0) {
                bool found = false;
                for (ant_lyr *a = al0; a; a = a->next) {
                    if (a->ldesc == ld) {
                        a->ratio = d;
                        found = true;
                        break;
                    }
                }
                if (!found && d > 0.0) {
                    if (!al0)
                        al0 = ale = new ant_lyr(ld, d);
                    else {
                        ale->next = new ant_lyr(ld, d);
                        ale = ale->next;
                    }
                }
                delete [] tok;
                delete [] tok2;
                continue;
            }
            PL()->ShowPromptV("Error following \"%s\".", tok);
            delete [] tok;
            return;
        }
        if (sscanf(tok, "%lf", &d) == 1 && d >= 0.0) {
            if (limset) {
                PL()->ShowPromptV("Unexpected token \"%s\".", tok);
                delete [] tok;
                return;
            }
            limset = true;
            totlim = d;
            delete [] tok;
            continue;
        }
        if (ispunct(*tok))
            PL()->ShowPrompt(usage);
        else
            PL()->ShowPromptV("Unrecognized token \"%s\".", tok);
        delete [] tok;
        return;
    }

    ant_pathfinder apf;
    while (al0) {
        ant_lyr *a = al0;
        al0 = al0->next;
        apf.set_layer_lim(a->ldesc->name(), a->ratio);
        delete a;
    }
    apf.set_lim(totlim);

    char buf[256];
    snprintf(buf, sizeof(buf), "%s.antenna.log", Tstring(sdesc->cellname()));
    if (!filestat::create_bak(buf)) {
        PL()->ShowPrompt(
            "Can't backup existing log file in current directory.");
        return;
    }

    FILE *fp = fopen(buf, "w");
    if (!fp) {
        PL()->ShowPromptV(
            "Can't create log file \"%s\" in current directory.", buf);
        return;
    }
    apf.set_output(fp);

    fprintf(fp, "Antenna Report (generated by %s)\n", XM()->IdString());
    fprintf(fp, "Root Cell: %s\n", Tstring(sdesc->cellname()));
    for (const alimit_t *al = apf.specs(); al; al = al->next)
        fprintf(fp, "Layer: %s  MinRatio: %.6f\n", al->al_lname,
            al->al_max_ratio);
    fprintf(fp, "MinRatio: %.6e\n", apf.limit());
    fprintf(fp, "-----------------------------\n");

    DSPpkg::self()->SetWorking(true);

    bool ret = apf.find_antennae(sdesc);

    DSPpkg::self()->SetWorking(false);

    fprintf(fp, "-----------------------------\n");
    fprintf(fp, "End of Report.  Run returned %s.\n",
        ret ? "no errors" : "ERROR");
    if (!ret)
        fprintf(fp, "%s\n", Errs()->get_error());
    fclose(fp);

    const char *msg = ret ? "Antenna run complete, view file? " :
        "Antenna run complete (errors reported), view file? ";
    char *in = PL()->EditPrompt(msg, "y");
    in = lstring::strip_space(in);
    if (in && (*in == 'y' || *in == 'Y'))
        DSPmainWbag(PopUpFileBrowser(buf))
}


#define matching(s) lstring::cieq(tok, s)

void
ext_bangcmds::netext(const char *s)
{
    struct sgc
    {
        sgc() { filename = cellname = basename = 0; }
        ~sgc() { delete [] filename; delete [] cellname; delete [] basename; }

        char *filename;
        char *cellname;
        char *basename;
    };

    sgc gc;
    BBox AOI;
    bool bb_given = false;
    int gridsize = 0;
    unsigned int flags = EN_FLAT | EN_COMP | EN_EXTR;

    bool ok = true;
    char *tok;
    while ((tok = lstring::gettok(&s)) != 0) {
        if (matching("-b")) {
            delete [] tok;
            tok = 0;
            gc.basename = lstring::getqtok(&s);
            if (!gc.basename) {
                ok = false;
                break;
            }
        }
        else if (matching("-c")) {
            delete [] tok;
            tok = 0;
            gc.cellname = lstring::getqtok(&s);
            if (!gc.cellname) {
                ok = false;
                break;
            }
        }
        else if (matching("-f")) {
            delete [] tok;
            tok = 0;
            gc.filename = lstring::getqtok(&s);
            if (!gc.filename) {
                ok = false;
                break;
            }
        }
        else if (matching("-g")) {
            delete [] tok;
            tok = lstring::getqtok(&s);
            if (!tok) {
                ok = false;
                break;
            }
            double g;
            if (sscanf(tok, "%lf", &g) != 1) {
                delete [] tok;
                ok = false;
                break;
            }
            gridsize = INTERNAL_UNITS(g);
        }
        else if (matching("-w")) {
            delete [] tok;
            tok = lstring::getqtok(&s);
            if (!tok) {
                ok = false;
                break;
            }
            double l, b, r, t;
            if (sscanf(tok, "%lf,%lf,%lf,%lf", &l, &b, &r, &t) != 4) {
                delete [] tok;
                ok = false;
                break;
            }
            BBox BB(INTERNAL_UNITS(l), INTERNAL_UNITS(b), INTERNAL_UNITS(r),
                INTERNAL_UNITS(t));
            BB.fix();
            AOI = BB;
            bb_given = true;
        }
        else if (matching("-nf"))
            flags &= ~EN_FLAT;
        else if (matching("-nc"))
            flags &= ~EN_COMP;
        else if (matching("-ne"))
            flags &= ~EN_EXTR;
        else if (matching("-l"))
            flags &= ~EN_LFLT;
        else if (matching("-v"))
            flags |= EN_VIAS;
        else if (matching("-v+"))
            flags |= (EN_VIAS | EN_VTRE);
        else if (matching("-vs"))
            flags |= EN_VSTD;
        else if (matching("-k"))
            flags |= EN_KEEP;
        else if (matching("-s1"))
            flags |= EN_STP1;
        else if (matching("-s2"))
            flags |= EN_STP2;
        else {
            PL()->ShowPromptV("Unknown token %s.", tok);
            delete [] tok;
            ok = false;
            break;
        }
        delete [] tok;
    }
    if (!ok) {
        PL()->ShowPrompt("Argument list error!");
        return;
    }
    if (!gc.filename) {
        PL()->ShowPrompt("No filename given!");
        return;
    }
    if (!gc.basename)
        gc.basename = lstring::copy("netext");

    bool free_chd = false;
    cCHD *chd = CDchd()->chdRecall(gc.filename, false);
    if (!chd) {
        char *realname;
        FILE *fp = FIO()->POpen(gc.filename, "rb", &realname);
        if (!fp) {
            PL()->ShowPromptV("File %s could not be opened.", gc.filename);
            delete [] realname;
            return;
        }
        FileType ft = FIO()->GetFileType(fp);
        fclose(fp); 
        if (!FIO()->IsSupportedArchiveFormat(ft)) {
            sCHDin chd_in;
            if (!chd_in.check(realname)) {
                delete [] realname;
                PL()->ShowPromptV("File %s type not supported.", gc.filename);
                return;
            }
            chd = chd_in.read(realname, sCHDin::get_default_cgd_type());
            delete [] realname;
            if (!chd) {
                PL()->ShowPromptV("Error reading CHD file %s.", gc.filename);
                if (Errs()->has_error())
                    Log()->ErrorLog(mh::Processing, Errs()->get_error());
                Errs()->get_error();
                return;
            }
        }
        if (!chd)
            chd = FIO()->NewCHD(gc.filename, ft, Physical, 0);
        if (!chd) {
            PL()->ShowPrompt("Error opening CHD.");
            if (Errs()->has_error())
                Log()->ErrorLog(mh::Processing, Errs()->get_error());
            return;
        }
        free_chd = true;
    }

    cExtNets netext(chd, gc.cellname, gc.basename, flags);
    bool ret;
    if (bb_given)
        ret = netext.dump_nets(&AOI, 0, 0);
    else if (gridsize == 0)
        ret = netext.dump_nets(0, 0, 0);
    else {
        if (gridsize < INTERNAL_UNITS(1.0)) {
            PL()->ShowPrompt("Grid size too small.");
            if (free_chd)
                delete chd;
            return;
        }
        ret = netext.dump_nets_grid(gridsize);
    }
    if (free_chd)
        delete chd;
    if (ret)
        PL()->ShowPromptV("Done, %d nets extracted.", netext.net_count());
    else {
        PL()->ShowPrompt("Done, error returned.");
        if (Errs()->has_error())
            Log()->ErrorLog(mh::Processing, Errs()->get_error());
    }
}


void
ext_bangcmds::addcells(const char*)
{
    PL()->ErasePrompt();
    CDs *cursd = CurCell(true);
    if (cursd) {
        EV()->InitCallback();
        ED()->ulListCheck("!addc", cursd, false);
        cursd->addMissingInstances();
        if (ED()->ulCommitChanges()) {
            DSP()->MainWdesc()->CenterFullView();
            DSP()->RedisplayAll();
        }
    }
}


void
ext_bangcmds::find(const char *s)
{
    CDs *cursdp = CurCell(Physical);
    if (cursdp) {
        EX()->associate(cursdp);
        cGroupDesc *gd = cursdp->groups();
        if (gd)
            gd->parse_find_dev(s, (*s != 0));
    }
    else
        PL()->ErasePrompt();
}


void
ext_bangcmds::ptrms(const char *s)
{
    if (DSP()->CurCellName()) {
        bool do_labels = false, do_terms = false, do_recurs = false;
        while (*s) {
            if (*s == 'c' || *s == 'l')
                do_labels = true;
            else if (*s == 'd' || *s == 't')
                do_terms = true;
            else if (*s == 'r')
                do_recurs = true;
            s++;
        }
        if (!do_labels && !do_terms)
            PL()->ShowPrompt("Usage:  !ptrms l|t [r]");
        else {
            bool tmpt = DSP()->ShowTerminals();
            if (tmpt)
                DSP()->ShowTerminals(ERASE);
            CDcbin cbin(DSP()->CurCellName());
            EX()->reset(&cbin, do_labels, do_terms, do_recurs);
            if (tmpt)
                DSP()->ShowTerminals(DISPLAY);
        }
    }
    else
        PL()->ErasePrompt();
}


void
ext_bangcmds::ushow(const char *s)
{
    CDs *cursdp = CurCell(Physical);
    if (!cursdp) {
        PL()->ErasePrompt();
        return;
    }
    EX()->associate(cursdp);
    cGroupDesc *gd = cursdp->groups();

    if (!s || !*s)
        s = "gds";
    if (strchr(s, 'g') || strchr(s, 'G') || strchr(s, 'n') ||
            strchr(s, 'N')) {
        gd->select_unassoc_groups();
        gd->select_unassoc_nodes();
    }
    if (strchr(s, 'd') || strchr(s, 'D')) {
        gd->select_unassoc_pdevs();
        gd->select_unassoc_edevs();
    }
    if (strchr(s, 's') || strchr(s, 'S') || strchr(s, 'c') ||
            strchr(s, 'C')) {
        gd->select_unassoc_psubs();
        gd->select_unassoc_esubs();
    }
    PL()->ErasePrompt();
}


void
ext_bangcmds::fc(const char *s)
{
    char *kw = lstring::gettok(&s);
    FCif()->doCmd(kw, s);
    delete [] kw;
}


void
ext_bangcmds::fh(const char *s)
{
    char *kw = lstring::gettok(&s);
    FHif()->doCmd(kw, s);
    delete [] kw;
}


// These are undocumented, for debugging.

void
ext_bangcmds::updlabels(const char*)
{
    EX()->updateNetLabels();
    PL()->ErasePrompt();
}


void
ext_bangcmds::updmeas(const char*)
{
    EX()->saveMeasuresInProp();
    PL()->ErasePrompt();
}


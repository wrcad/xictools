
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

#include "fio.h"
#include "fio_cif.h"
#include "fio_chd.h"
#include "fio_library.h"
#include "fio_layermap.h"
#include "cd_propnum.h"
#include "cd_hypertext.h"
#include "cd_celldb.h"
#include "cd_digest.h"
#include "miscutil/timedbg.h"
#include "miscutil/filestat.h"
#include <ctype.h>
#include <dirent.h>


namespace {
    bool test_oldstyle(FILE*, bool*);
    Point *box_to_poly(BBox*, int, int);
}

#define UNDEF_PREFIX "Symbol"


// Determine if the file is CIF, if so return true, and fill in the
// pointers if given.  The ctype returns a code indicating the type of
// name extension used.  Skip to the first DS command, and look at the
// line following.
//
// CFnative:   XIC/KIC symbol (not cif!).  An XIC symbol has the
//             '9 cellname' BEFORE the DS command.
// CFigs:      An IGS symbol name follows a DS command as in
//             9 cellname;
// CFnca:      A Stanford/NCA symbol name follows a DS command as in
//             (cellname);
// CFicarus:   An Icarus symbol name follows a DS command as in
//             (9 cellname);
// CDsif:      A Sif symbol name follows a DS command as in
//             (Name: cellname);
// CFnone:     None of the above
//
// If issced is not null, perform further testing for compatibility.
// Set *issced true if the file is an old style SCED file.  Only
// CFnative files can be SCED files.
//
bool
cFIO::IsCIF(FILE *cfile, CFtype *ctype, bool *issced)
{
    if (cfile == 0)
        return (false);

    if (issced)
        *issced = false;

    char buf[512];
    bool native = false;
    bool firstc = true;
    CFtype type = CFnone;
    int c;
    while ((c = getc(cfile)) != EOF) {
        if (isspace(c) || c == ';')
            continue;
        if (firstc) {
            // ignore any "#!..." shell header
            if (c == '#') {
                c = getc(cfile);
                if (c != '!') {
                    rewind(cfile);
                    return (false);
                }
                while ((c = getc(cfile)) != EOF) {
                    if (c == '\n')
                        break;
                }
                c = getc(cfile);
                if (isspace(c))
                    continue;
            }
            if (c != '(' && c != '9' && c != '5' && c != 'D') {
                // file should start with one of these
                rewind(cfile);
                return (false);
            }
            if (c != '(')
                firstc = false;
        }
        if (c == '9') {
            // user extension line, test for native symbol
            if ((c = getc(cfile)) == EOF) {
                rewind(cfile);
                return (false);
            }
            // at least one space before symbol_name
            if (!isspace(c)) {
                while (((c = getc(cfile)) != EOF) && (c != ';')) ;
                continue;
            }
            while ((c = getc(cfile)) != EOF)
                if (!isspace(c)) break;
            while ((c = getc(cfile)) != EOF) {
                // should be 'symbol_name;' (no space)
                if (isspace(c))
                    break;
                if (c == ';') {
                    type = CFnative;
                    break;
                }
            }
            if (c != ';')
                while (((c = getc(cfile)) != EOF) && (c != ';')) ;
            continue;
        }
        if (c == '(') {
            // look for "(PHYSICAL)"
            int i = 0;
            int dep = 1;
            while ((c = getc(cfile)) != EOF) {
                if (c == '(')
                    dep++;
                else if (c == ')') {
                    dep--;
                    if (!dep)
                        break;
                }
                if (i < 511)
                    buf[i++] = c;
            }
            buf[i] = 0;
            if (firstc) {
                firstc = false;
                // The first line is a comment, check if library file.
                if (lstring::cimatch("library", buf)) {
                    rewind(cfile);
                    return (false);
                }
            }
            if (!strcmp(buf, "PHYSICAL"))
                native = true;
            continue;
        }
        if (c != 'D') {
            while (((c = getc(cfile)) != EOF) && (c != ';')) ;
            continue;
        }
        if ((c = getc(cfile)) == EOF) {
            rewind(cfile);
            return (false);
        }
        if (c != 'S') {
            while (((c = getc(cfile)) != EOF) && (c != ';')) ;
            continue;
        }

        // found a DS command
        break;
    }

    if (type == CFnone) {
        // Look at the following line and determine file type
        while (((c = getc(cfile)) != EOF) && (c != ';')) ;
        while ((c = getc(cfile)) != EOF) {
            if (!isspace(c))
                break;
        }
        if (c == EOF) {
            rewind(cfile);
            return (false);
        }

        if (c == '(') {
            // a comment line
            while ((c = getc(cfile)) != EOF) {
                if (!isspace(c))
                    break;
            }
            if (c == EOF) {
                rewind(cfile);
                return (false);
            }
            if (c == '9')
                // Icarus
                type = CFicarus;
            else {
                int token = 0;
                int cl = c;
                while ((c = getc(cfile)) != EOF) {
                    if (isspace(c)) {
                        if (!isspace(cl))
                            token++;
                        if (token > 1)
                            break;
                        cl = c;
                        continue;
                    }
                    if (c == ':') {
                        // Sif
                        type = CFsif;
                        break;
                    }
                    if (c == ')') {
                        // Stanford/NCA
                        type = CFnca;
                        break;
                    }
                    cl = c;
                }
            }
        }
        else if (c == '9') {
            // user extension line
            c = getc(cfile);
            if (c == EOF) {
                rewind(cfile);
                return (false);
            }
            if (isspace(c)) {
                while ((c = getc(cfile)) != EOF) {
                    if (!isspace(c))
                        break;
                }
                if (c == EOF) {
                    rewind(cfile);
                    return (false);
                }
                while ((c = getc(cfile)) != EOF) {
                    if (isspace(c) || c == ';')
                        break;
                }
                if (c == EOF) {
                    rewind(cfile);
                    return (false);
                }
                if (isspace(c)) {
                    while ((c = getc(cfile)) != EOF) {
                        if (!isspace(c))
                            break;
                    }
                    if (c == EOF) {
                        rewind(cfile);
                        return (false);
                    }
                }
                if (c == ';')
                    // IGS (CD default)
                    type = CFigs;
            }
        }
    }
    if (ctype)
        *ctype = type;

    if (!issced || type != CFnative || native) {
        rewind(cfile);
        return (true);
    }

    // Now see if the file is an old fashioned SCED file.  Set
    // issced if yes.

    if (c != ';')
        while (((c = getc(cfile)) != EOF) && (c != ';')) ;

    int sced_liklihood = 0;
    while ((c = getc(cfile)) != EOF) {
        if (isspace(c)) continue;
        int i;
        if (c == 'L') {
            // Layer declaration line, test for SCED layer
            i = 0;
            while (((c = getc(cfile)) != EOF) && (c != ';'))
                if (!isspace(c) && i < 4) {
                    buf[i] = c;
                    i++;
                }
            if (buf[0] == 'S' && buf[1] == 'C' && buf[2] == 'E' &&
                    buf[3] == 'D')
                *issced = true;
            rewind(cfile);
            return (true);
        }
        else if (c == '9') {
            // name declaration, look for lib cells
            i = 0;
            while (((c = getc(cfile)) != EOF) && (c != ';'))
                if (!isspace(c) && i < 4) {
                    buf[i] = c;
                    i++;
                }
            buf[i] = '\0';
            if (!strcmp(buf, "gnd") || !strcmp(buf, "vsrc") ||
                    !strcmp(buf, "isrc"))
                sced_liklihood += 2;
        }
        else if (c == '5') {
            // property extension, look for sced properties
            i = 0;
            while (((c = getc(cfile)) != EOF) && (c != ';'))
                if (!isspace(c) && i < 3) {
                    buf[i] = c;
                    i++;
                }
            buf[i] = '\0';
            i = atoi(buf);
            if (i == P_NAME || i == P_NODE)
                sced_liklihood++;
        }
        else
            while (((c = getc(cfile)) != EOF) && (c != ';')) ;
        if (sced_liklihood > 2) {
            *issced = true;
            break;
        }
    }
    rewind(cfile);
    return (true);
}


// The file pointer is known to point to natice cell data.  Return
// true if a "(PHYSICAL);" comment is found before DS.  This section
// is optional in device symbols.
//
bool
cFIO::HasPHYSICAL(FILE *cfile)
{
    if (cfile == 0)
        return (false);
    long posn = large_ftell(cfile);

    char buf[512];
    bool firstc = true;
    int c;
    while ((c = getc(cfile)) != EOF) {
        if (isspace(c) || c == ';')
            continue;
        if (firstc) {
            // ignore any "#!..." shell header
            if (c == '#') {
                c = getc(cfile);
                if (c != '!') {
                    large_fseek(cfile, posn, SEEK_SET);
                    return (false);
                }
                while ((c = getc(cfile)) != EOF) {
                    if (c == '\n')
                        break;
                }
                c = getc(cfile);
                if (isspace(c))
                    continue;
            }
            if (c != '(' && c != '9' && c != '5' && c != 'D') {
                // file should start with one of these
                large_fseek(cfile, posn, SEEK_SET);
                return (false);
            }
            if (c != '(')
                firstc = false;
        }
        if (c == '9') {
            // user extension line, test for native symbol
            if ((c = getc(cfile)) == EOF) {
                large_fseek(cfile, posn, SEEK_SET);
                return (false);
            }
            // at least one space before symbol_name
            if (!isspace(c)) {
                while (((c = getc(cfile)) != EOF) && (c != ';')) ;
                continue;
            }
            while ((c = getc(cfile)) != EOF)
                if (!isspace(c)) break;
            while ((c = getc(cfile)) != EOF) {
                // should be 'symbol_name;' (no space)
                if (isspace(c))
                    break;
                if (c == ';')
                    break;
            }
            if (c != ';')
                while (((c = getc(cfile)) != EOF) && (c != ';')) ;
            continue;
        }
        if (c == '(') {
            // look for "(PHYSICAL)"
            int i = 0;
            int dep = 1;
            while ((c = getc(cfile)) != EOF) {
                if (c == '(')
                    dep++;
                else if (c == ')') {
                    dep--;
                    if (!dep)
                        break;
                }
                if (i < 511)
                    buf[i++] = c;
            }
            buf[i] = 0;
            if (firstc) {
                firstc = false;
                // The first line is a comment, check if library file.
                if (lstring::cimatch("library", buf)) {
                    large_fseek(cfile, posn, SEEK_SET);
                    return (false);
                }
            }
            if (!strcmp(buf, "PHYSICAL")) {
                large_fseek(cfile, posn, SEEK_SET);
                return (true);
            }
            continue;
        }
        if (c != 'D') {
            while (((c = getc(cfile)) != EOF) && (c != ';')) ;
            continue;
        }
        if ((c = getc(cfile)) == EOF)
            break;
        if (c != 'S') {
            while (((c = getc(cfile)) != EOF) && (c != ';')) ;
            continue;
        }

        // found a DS command
        break;
    }
    large_fseek(cfile, posn, SEEK_SET);
    return (false);
}


// Read the CIF file cif_fname, the new cells will be be added to
// the database.  All conversions will be scaled by the value of
// scale.
//  Return values:
//    OIerror       unspecified error
//    OIok          file was read and new cells opened ok
//    OIaborted     user aborted
//
OItype
cFIO::DbFromCIF(const char *cif_fname, const FIOreadPrms *prms,
    stringlist **tlp, stringlist **tle)
{
    if (!prms)
        return (OIerror);

    Tdbg()->start_timing("cif_read");
    CIFstyle style_bak = fioCIF_Style;
    fioCIF_Style.set_def();

    cif_in *cif = new cif_in(prms->allow_layer_mapping());
    cif->set_show_progress(true);
    cif->set_no_test_empties(IsNoCheckEmpties());

    CD()->SetReading(true);
    cif->assign_alias(NewReadingAlias(prms->alias_mask()));
    cif->read_alias(cif_fname);
    bool ret = cif->setup_source(cif_fname);
    if (ret) {
        // The first pass sets the cell name extension type.
        cif->set_to_database();
        Tdbg()->start_timing("cif_read_phys_1");
        cif->set_update_style(true);
        cif->begin_log(Physical);
        ifPrintCvLog(IFLOG_INFO, "Building symbol table.");
        ret = cif->parse(Physical, true, 1.0);
        cif->set_update_style(false);
        Tdbg()->stop_timing("cif_read_phys_1");
    }
    if (ret && !CD()->IsNoElectrical() && cif->has_electrical()) {
        Tdbg()->start_timing("cif_read_elec_1");
        cif->begin_log(Electrical);
        ifPrintCvLog(IFLOG_INFO, "Building symbol table.");
        ret = cif->parse(Electrical, true, 1.0);
        Tdbg()->stop_timing("cif_read_elec_1");
    }
    if (ret) {
        // The second pass sets the layer and label extensions.
        Tdbg()->start_timing("cif_read_phys_2");
        cif->set_update_style(true);
        cif->begin_log(Physical);
        CD()->SetDeferInst(true);
        ret = cif->parse(Physical, false, prms->scale());
        CD()->SetDeferInst(false);
        Tdbg()->stop_timing("cif_read_phys_2");
        if (ret)
            ret = cif->mark_references(tlp);
        cif->end_log();
        cif->set_update_style(false);
    }
    if (ret && !CD()->IsNoElectrical() && cif->has_electrical()) {
        Tdbg()->start_timing("cif_read_elec_2");
        cif->begin_log(Electrical);
        CD()->SetDeferInst(true);
        bool lpc = CD()->EnableLabelPatchCache(true);
        ret = cif->parse(Electrical, false, prms->scale());
        CD()->EnableLabelPatchCache(lpc);
        CD()->SetDeferInst(false);
        Tdbg()->stop_timing("cif_read_elec_2");
        if (ret)
            ret = cif->mark_references(tle);
        cif->end_log();
    }
    if (ret)
        cif->mark_top(tlp, tle);
    CD()->SetReading(false);

    if (ret)
        cif->dump_alias(cif_fname);

    OItype oiret = ret ? OIok : cif->was_interrupted() ? OIaborted : OIerror;
    delete cif;
    fioCIF_LastReadStyle = fioCIF_Style;
    fioCIF_Style = style_bak;
    Tdbg()->stop_timing("cif_read");
    return (oiret);
}


// Read the CIF file cif_fname, performing translation.
//  Return values:
//    OIerror       unspecified error
//    OIok          file was read and new cells opened ok
//    OIaborted     user aborted
//
OItype
cFIO::FromCIF(const char *cif_fname, const FIOcvtPrms *prms,
    const char *chdcell)
{
    if (!prms) {
        Errs()->add_error("FromCIF: null destination pointer.");
        return (OIerror);
    }
    if (!prms->destination()) {
        Errs()->add_error("FromCIF:: no destination given!");
        return (OIerror);
    }

    cCHD *chd = CDchd()->chdRecall(cif_fname, false);
    if (chd) {
        if (chd->filetype() != Fcif) {
            Errs()->add_error("FromCIF:: CHD file type not CIF!");
            return (OIerror);
        }

        // We were given a CHD name, use it.
        cif_fname = chd->filename();
    }
    else {
        CFtype type = CFnone;
        FILE *fp = POpen(cif_fname, "rb");
        if (fp) {
            bool issced;
            if (!IsCIF(fp, &type, &issced)) {
                fclose(fp);
                Errs()->add_error("File %s has unknown dialect.",
                    cif_fname);
                ifInfoMessage(IFMSG_INFO, Errs()->get_error());
                return (OIerror);
            }
            fclose(fp);
        }
        else {
            Errs()->add_error("CIF file %s not found.", cif_fname);
            ifInfoMessage(IFMSG_INFO, Errs()->get_error());
            return (OIerror);
        }
        ifPrintCvLog(IFLOG_INFO, "Converting CIF file type: %d", type);
    }

    SetAllowPrptyStrip(true);
    if (chd || prms->use_window() || prms->flatten() ||
            (prms->ecf_level() != ECFnone)) {

        // Using windowing or other features requiring a cCHD
        // description.  This is restricted to physical-mode data.

        bool free_chd = false;
        if (!chd) {
            unsigned int mask = prms->alias_mask();
            if (prms->filetype() == Fgds)
                mask |= CVAL_GDS;
            FIOaliasTab *tab = NewTranslatingAlias(mask);
            if (tab)
                tab->read_alias(cif_fname);
            cvINFO info = cvINFOtotals;
            if (prms->ecf_level() == ECFall || prms->ecf_level() == ECFpre)
                info = cvINFOplpc;
            chd = NewCHD(cif_fname, Fcif, Physical, tab, info);
            delete tab;
            free_chd = true;
        }
        OItype oiret = OIerror;
        if (chd) {
            oiret = chd->translate_write(prms, chdcell);
            if (free_chd)
                delete chd;
        }
        SetAllowPrptyStrip(false);
        return (oiret);
    }

    cif_in *cif = new cif_in(prms->allow_layer_mapping());
    cif->set_show_progress(true);

    // Translating, directly streaming.  Skip electrical data if
    // StripForExport is set.

    const cv_alias_info *aif = prms->alias_info();
    if (aif)
        cif->assign_alias(new FIOaliasTab(true, false, aif));
    else {
        unsigned int mask = prms->alias_mask();
        if (prms->filetype() == Fgds)
            mask |= CVAL_GDS;
        cif->assign_alias(NewTranslatingAlias(mask));
    }
    cif->read_alias(cif_fname);

    bool ret = cif->setup_source(cif_fname);
    if (ret)
        ret = cif->setup_destination(prms->destination(), prms->filetype(),
            prms->to_cgd());

    if (ret) {
        cif->begin_log(Physical);
        ifPrintCvLog(IFLOG_INFO, "Building symbol table.");
        ret = cif->parse(Physical, true, 1.0);
    }
    if (ret && !fioStripForExport && !prms->to_cgd() &&
            !CD()->IsNoElectrical() && cif->has_electrical()) {
        cif->begin_log(Electrical);
        ifPrintCvLog(IFLOG_INFO, "Building symbol table.");
        ret = cif->parse(Electrical, true, 1.0);
    }
    if (ret) {
        cif->begin_log(Physical);
        ret = cif->parse(Physical, false, prms->scale());
        cif->end_log();
    }
    if (ret && !fioStripForExport && !prms->to_cgd() &&
            !CD()->IsNoElectrical() && cif->has_electrical()) {
        cif->begin_log(Electrical);
        ret = cif->parse(Electrical, false, prms->scale());
        cif->end_log();
    }

    if (ret)
        cif->dump_alias(cif_fname);

    OItype oiret = ret ? OIok : cif->was_interrupted() ? OIaborted : OIerror;
    delete cif;
    SetAllowPrptyStrip(false);
    return (oiret);
}


namespace {
    // Give each master a new name and cell pointer.  Transfer the master
    // references list to newsd.
    //
    void
    fix_master_refs(CDs *sdesc, CDs *newsd)
    {
        CDcellName name = newsd->cellname();
        CDm_gen gen(sdesc, GEN_MASTER_REFS);
        for (CDm *m = gen.m_first(); m; m = gen.m_next()) {
            CDs *parent = m->parent();
            if (!parent) {
                m->setCellname(name);
                m->setCelldesc(newsd);
                continue;
            }
            m->unlink();
            m->setCellname(name);
            m->setCelldesc(newsd);
            parent->linkMaster(m);
        }
        unsigned long mr = sdesc->masterRefs();
        sdesc->setMasterRefs(0);
        newsd->setMasterRefs(mr);
    }


    // Return true if the first instance has the standard via property.
    //
    bool is_std_via(CDs *sdesc)
    {
        if (!sdesc)
            return (false);
        if (sdesc->prpty(XICP_STDVIA))
            return (true);
        CDm_gen mgen(sdesc, GEN_MASTER_REFS);
        CDm *m = mgen.m_first();
        CDc_gen cgen(m);
        CDc *cd = cgen.c_first();
        return (cd && cd->prpty(XICP_STDVIA));
    }
}


OItype
cFIO::FromNative(const char *fullpath, CDcbin *cbret, double scale)
{
    if (cbret)
        cbret->reset();
    const char *topcell = lstring::strip_path(fullpath);

    // The path is stripped from fullpath and added to the
    // front or back of the search path.

    int flags = 0;
    char *tmp_path = 0;
    if (fullpath != topcell) {
        tmp_path = lstring::copy(PGetPath());
        flags = PSetReadPath(fullpath, 0);
    }

    // We need to carry the sub-master table along outside of the
    // cif_in, which gets destroyed after converting each cell.
    delete fioSubMasterTab;  // Should be null already.
    fioSubMasterTab = 0;

    bool lpc = CD()->EnableLabelPatchCache(true);
    OItype oiret = OIok;
    SymTab *tab = new SymTab(true, false);
    stringnumlist *s0 = new stringnumlist(lstring::copy(topcell), 1, 0);
    while (s0) {
        stringnumlist *inames = 0;
        if (SymTab::get(tab, s0->string) == ST_NIL) {
            tab->add(lstring::copy(s0->string), 0, false);

            CDcbin cbin;
            bool divert;
            fioUseSubMasterTab = true;
            oiret = OpenNative(s0->string, &cbin, scale, &divert);
            fioUseSubMasterTab = false;
            if (OIfailed(oiret)) {
                ifPrintCvLog(IFLOG_FATAL, "Failed to open %s.", s0->string);
                Errs()->add_error("FromNative: OpenNative failed.");
                stringnumlist::destroy(s0);
                s0 = 0;
                break;
            }
            if (divert) {
                tab->remove(s0->string);
                tab->add(lstring::copy(Tstring(cbin.cellname())), 0, false);

                // Add the association cellname -> archive path to a
                // hash table.  This will be used when writing to put
                // the path back as an instance name.

                if (!fioNativeImportTab)
                    fioNativeImportTab = new SymTab(true, true);
                SymTabEnt *h = SymTab::get_ent(
                    fioNativeImportTab, Tstring(cbin.cellname()));
                if (h) {
                    if (strcmp((const char*)h->stData, s0->string)) {
                        delete [] (char*)h->stData;
                        h->stData = lstring::copy(s0->string);
                    }
                }
                else {
                    fioNativeImportTab->add(
                        lstring::copy(Tstring(cbin.cellname())),
                        lstring::copy(s0->string), false);
                }
            }
            else {
                if (cbin.phys())
                    cbin.phys()->listSubcells(&inames, false);
                if (cbin.elec())
                    cbin.elec()->listSubcells(&inames, false);
            }
        }
        if (inames) {
            stringnumlist *n = inames;
            while (n->next)
                n = n->next;
            n->next = s0->next;
            s0->next = 0;
            stringnumlist::destroy(s0);
            s0 = inames;
        }
        else {
            stringnumlist *n = s0->next;
            s0->next = 0;
            stringnumlist::destroy(s0);
            s0 = n;
        }
    }
    delete fioSubMasterTab;
    fioSubMasterTab = 0;
    CD()->EnableLabelPatchCache(lpc);

    if (fullpath != topcell) {
        PSetReadPath(fullpath, flags);
        // If PSetReadPath has changed the path, notify the application.
        const char *npath = PGetPath();
        if ((npath && !tmp_path) || (!npath && tmp_path) ||
                (npath && tmp_path && strcmp(npath, tmp_path)))
            ifPathChange();
        delete [] tmp_path;
    }

    if (!OIfailed(oiret)) {
        SymTabGen tgen(tab);
        SymTabEnt *h;
        while ((h = tgen.next()) != 0) {
            const char *sname = h->stTag;
            CDcbin cbin;
            if (!CDcdb()->findSymbol(sname, &cbin)) {
                // This should have been created as the file was read.
                Errs()->add_error(
                "FromNative: internal error, cell %s not in database.",
                    sname);
                oiret = OIerror;
                break;
            }
            if (cbin.phys()) {
                SetSkipFixBB(true);
                bool ret = ifReopenSubMaster(cbin.phys());
                SetSkipFixBB(false);
                if (!ret) {
                    ifPrintCvLog(IFLOG_WARN,
                        "PCell evaluation failed for %s (%s):\n%s",
                        sname, "physical", Errs()->get_error());
                    cbin.phys()->setUnread(false);
                }
            }
            if (cbin.elec()) {
                SetSkipFixBB(true);
                bool ret = ifReopenSubMaster(cbin.elec());
                SetSkipFixBB(false);
                if (!ret) {
                    ifPrintCvLog(IFLOG_WARN,
                        "PCell evaluation failed for %s (%s):\n%s",
                        sname, "electrical", Errs()->get_error());
                    cbin.elec()->setUnread(false);
                }
            }

            if ((cbin.phys() && cbin.phys()->isUnread()) ||
                    (cbin.elec() && cbin.elec()->isUnread())) {
                CDcbin tcbin;
                FIO()->SetSkipFixBB(true);
                oiret = FIO()->OpenLibCell(0, sname, LIBdevice | LIBuser,
                    &tcbin);
                FIO()->SetSkipFixBB(false);
                if (OIfailed(oiret)) {
                    // An error or user abort.  OIok is returned whether
                    // or not the cell was actually found.
                    Errs()->add_error("OpenLibCell returned prematurely.");
                    break;
                }
                if (cbin.phys() && cbin.phys() == tcbin.phys())
                    cbin.phys()->setUnread(false);
                if (cbin.elec() && cbin.elec() == tcbin.elec())
                    cbin.elec()->setUnread(false);
            }
                
            int fl = 0;
            if (cbin.phys() && cbin.phys()->isUnread()) {
                fl |= 1;
                cbin.phys()->setUnread(false);
            }
            if (cbin.elec() && cbin.elec()->isUnread()) {
                fl |= 2;
                cbin.elec()->setUnread(false);
            }
            if (fl) {
                if (fl == 1 && is_std_via(cbin.phys())) {
                    ifPrintCvLog(IFLOG_WARN,
                        "Unsatisfied standard via reference to %s.", sname);
                }
                else {
                    const char *c = "physical and electrical";
                    if (fl == 1)
                        c = "physical";
                    else if (fl == 2)
                        c = "electrical";
                    ifPrintCvLog(IFLOG_WARN,
                        "Unsatisfied symbol reference (%s) to %s.", c, sname);
                }
            }
        }
    }

    CDcbin cbin;
    if (!OIfailed(oiret)) {
        if (!CDcdb()->findSymbol(topcell, &cbin))
            oiret = OIerror;
    }
    if (!OIfailed(oiret) && !IsSkipFixBB()) {
        if (!cbin.fixBBs())
            oiret = OIerror;
    }
    if (!OIfailed(oiret))
        *cbret = cbin;
    else {
        SymTabGen gen(tab);
        SymTabEnt *h;
        while ((h = gen.next()) != 0) {
            if (CDcdb()->findSymbol(h->stTag, &cbin)) {
                delete cbin.phys();
                delete cbin.elec();
            }
        }
    }
    delete tab;

    return (oiret);
}


// Open a native cell.  We can resolve native cell files and inlined
// cells in libraries.
//
// The divert argument is for support for opening a cell from an
// archive.  In this case, the spath, i.e., "9 spath;" in the cell
// file, may have a form like "/path/to/foo.gds mycell", where the
// first token is an archive file name or library name, and the second
// token, which need not exist, is the name of a cell from that
// source.  If we detect something like this, and the divert argument
// is not null, we will open the file so that the cell will be
// referenced by the cell being opened, and set divert to point to
// true.
//
OItype
cFIO::OpenNative(const char *spath, CDcbin *cbret, double scale, bool *divert)
{
    if (cbret)
        cbret->reset();
    if (divert)
        *divert = false;
    if (!spath) {
        Errs()->add_error("OpenNative: null cell name encountered.");
        return (OIerror);
    }

    const char *sname = lstring::strip_path(spath);
    if (!*sname) {
        Errs()->add_error("OpenNative: empty cell name encountered.");
        return (OIerror);
    }

    char *fname, *cname;
    if (divert) {
        const char *ss = spath;
        fname = lstring::gettok(&ss);
        cname = lstring::gettok(&ss);
    }
    else {
        fname = lstring::copy(spath);
        cname = 0;
    }
    GCarray<char*> gc_fname(fname);
    GCarray<char*> gc_cname(cname);

    // With divert, there should be an empty cell named spath created
    // to satisfy the instance reference, created by the reader.

    CDcbin cbin;
    bool exists = CDcdb()->findSymbol(sname, &cbin);
    if (!exists && sname != spath && divert &&
            CDcdb()->findSymbol(spath, &cbin))
        exists = true;

    OItype oiret = OIok;
    bool overwrite_phys = false;
    bool overwrite_elec = false;

    // Open the cell file or library.
    char *pathname = 0;
    sLibRef *libref = 0;
    sLib *libptr = 0;
    FILE *fp = POpen(fname, "rb", &pathname,
        LIBdevice | LIBuser | LIBnativeOnly, &libref, &libptr);

    if (!exists) {
        // If the cell doesn't exist, it is an error if we can't
        // resolve the given cell name/path.

        if (!fp) {
            Errs()->sys_error("open");
            Errs()->add_error("Can't open cell %s.", sname);
            oiret = OIerror;
        }
    }
    else {
        // If the cell exists and there is no native cell file
        // resolved, we're done.  Otherwise we consider whether or not
        // to overwrite.

        bool libdev = cbin.isDevice() && cbin.isLibrary();

        // If a cell exists and is empty, it will be overwritten
        // without confirmation.
        bool unread = cbin.isUnread() ||
            ((!cbin.phys() || cbin.phys()->isEmpty()) &&
            (!cbin.elec() || cbin.elec()->isEmpty()));
        if (unread && fp) {
            // Symbol was created to satisfy instance reference, is being
            // read in now.
            overwrite_phys = true;
            overwrite_elec = true;
        }
        if (unread || !libdev) {
            if (cbin.isLibrary() && (FIO()->IsReadingLibrary() ||
                    FIO()->IsNoOverwriteLibCells())) {
                // Skip reading into library cells.
                if (fp && !FIO()->IsReadingLibrary()) {
                    Errs()->add_error("OpenNative: "
                        "can't overwrite library cell %s already in memory.",
                        Tstring(cbin.cellname()));
                    return (OIerror);
                }
            }
            else {
                if (divert) {

                    // The following makes it possible to call an
                    // archive from a native cell file, by the archive
                    // file name, and to resolve non-inline library
                    // references.  The archive must have a single
                    // top-level cell, which will appear as the called
                    // reference.

                    // When we get here, there is already a cell in
                    // memory with the same name as the archive file. 
                    // We remove it, dup in the new cell info, and
                    // relink it.

                    bool open_as_import = false;
                    if (IsSupportedArchiveFormat(TypeExt(fname)))
                        open_as_import = true;
                    else if (!fp) {
                        if (cname) {
                            if (LookupLibCell(fname, cname, LIBuser, 0))
                                open_as_import = true;
                            else if (FIO()->ifOpenOA(fname, cname, 0))
                                open_as_import = true;
                        }
                        else {
                            if (LookupLibCell(0, fname, LIBuser, 0))
                                open_as_import = true;
                        }
                    }

                    if (open_as_import) {

                        // Pull out the cells with the hideous name.
                        CDcellName cn = CD()->CellNameTableFind(spath);
                        CDs *sdp = CDcdb()->removeCell(cn, Physical);
                        CDs *sde = CDcdb()->removeCell(cn, Electrical);
                        cbin.reset();

                        FIOreadPrms prms;
                        prms.set_scale(scale);
                        prms.set_alias_mask(
                            CVAL_AUTO_NAME | CVAL_CASE | CVAL_FILE);

                        // Open the reference.
                        oiret = OpenImport(fname, &prms, cname, 0, &cbin);
                        if (OIfailed(oiret)) {
                            ifPrintCvLog(IFLOG_FATAL, "Failed to open %s.",
                                spath);
                            Errs()->add_error(
                                "OpenNative: OpenImport failed.");
                        }
                        else {
                            if (sdp) {
                                if (cbin.phys())
                                    fix_master_refs(sdp, cbin.phys());
                                delete sdp;
                            }
                            if (sde) {
                                if (cbin.elec())
                                    fix_master_refs(sde, cbin.elec());
                                delete sde;
                            }
                        }

                        delete [] pathname;
                        *divert = true;
                        if (cbret)
                            *cbret = cbin;
                        return (oiret);
                    }
                }

                if (fp && !unread) {
                    mitem_t mi(sname);
                    if (cbin.phys() && cbin.phys()->isViaSubMaster()) {
                        mi.overwrite_phys = false;
                        mi.overwrite_elec = false;
                    }
                    else {
                        if (!IsNoOverwritePhys())
                            mi.overwrite_phys = true;
                        if (!IsNoOverwriteElec())
                            mi.overwrite_elec = true;
                        ifMergeControl(&mi);
                    }
                    if (mi.overwrite_phys) {
                        overwrite_phys = true;
                        if (cbin.phys()) {
                            cbin.phys()->setImmutable(false);
                            cbin.phys()->setLibrary(false);
                            cbin.phys()->clear(true);
                        }
                    }
                    if (mi.overwrite_elec) {
                        overwrite_elec = true;
                        if (cbin.elec()) {
                            cbin.elec()->setImmutable(false);
                            cbin.elec()->setLibrary(false);
                            cbin.elec()->clear(true);
                        }
                    }
                }
            }
        }
        if (!overwrite_phys && !overwrite_elec)
            oiret = OIold;
    }

    if (!OIfailed(oiret) && oiret != OIold) {
        if (!cbin.phys()) {
            cbin.setPhys(CDcdb()->insertCell(sname, Physical));
            overwrite_phys = true;
        }
        if (!cbin.elec() && !CD()->IsNoElectrical()) {
            cbin.setElec(CDcdb()->insertCell(sname, Electrical));
            overwrite_elec = true;
        }
        cbin.setFileType(Fnative);
        cbin.setFileName(0);
        if (!libref && pathname) {
            // Save the directory containing the cell (if known) in
            // the fileName field.  Note that we do this to both
            // physical and electrical cells, whether or not both are
            // read.  Same applies to fileType.

            char *t = lstring::strrdirsep(pathname);
            if (t) {
                // Strip cell name.
                *t = 0;
                cbin.setFileName(pathname);
            }
            // We're done with pathname.
            delete [] pathname;
            pathname = 0;
        }

        // Read and parse the structure if necessary.
        if (fp) {
            if (libptr) {
                cbin.setDevice(libptr->lib_type() == LIBdevice);
                cbin.setLibrary(true);
            }
            else if (FIO()->IsReadingLibrary())
                cbin.setLibrary(true);

            if (cbin.phys())
                cbin.phys()->setUnread(false);
            if (cbin.elec())
                cbin.elec()->setUnread(false);

            bool ret = true;
            CD()->SetReading(true);
            // No layer filter or aliasing reading native.
            cif_in *cif = new cif_in(false, CFnative);
            if (fioUseSubMasterTab)
                cif->set_submaster_tab(fioSubMasterTab);
            cif->set_to_database();
            if (!libptr || libptr->lib_type() != LIBdevice ||
                    HasPHYSICAL(fp)) {
                // LIBdevice has optional physical part.
                cif->setup_native(Tstring(cbin.cellname()), fp,
                    overwrite_phys ? cbin.phys() : 0);
                ret = cif->parse(Physical, false, scale);
            }
            if (ret && overwrite_elec) {
                cif->setup_native(Tstring(cbin.cellname()), fp,
                    cbin.elec());
                ret = cif->parse(Electrical, false, 1.0);
            }
            CD()->SetReading(false);

            // Delete empty cells.
            bool ep = cbin.phys()->isEmpty();
            bool ee = cbin.elec()->isEmpty();
            if (ee) {
                delete cbin.elec();
                cbin.setElec(0);
            }
            else if (ep) {
                delete cbin.phys();
                cbin.setPhys(0);
            }

            if (!ret)
                oiret = cif->was_interrupted() ? OIaborted : OIerror;
            if (fioUseSubMasterTab)
                fioSubMasterTab = cif->get_submaster_tab();
            delete cif;
            if (libptr || FIO()->IsReadingLibrary())
                cbin.setImmutable(true);
        }
    }
    delete [] pathname;

    if (fp)
        fclose(fp);

    if (cbret)
        *cbret = cbin;
    return (oiret);
}


// An alias file "aliases.alias" can be used in the same directory as
// the native cell files, for translation.
#define AliasFileName "aliases"

// Write all native cell files found in the dirpath directory into an
// archive file specified in the parms destination.
//
OItype
cFIO::TranslateDir(const char *dirpath, const FIOcvtPrms *prms)
{
    if (!prms) {
        Errs()->add_error("TranslateDir: null destination pointer.");
        return (OIerror);
    }
    if (!prms->destination()) {
        Errs()->add_error("TranslateDir: no destination given!");
        return (OIerror);
    }
    if (!dirpath || !*dirpath)
        dirpath = ".";
    if (!filestat::is_directory(dirpath)) {
        Errs()->add_error("TranslateDir: path is not a directory.");
        return (OIerror);
    }

    cif_in *cif = new cif_in(prms->allow_layer_mapping(), CFnative);

    const cv_alias_info *aif = prms->alias_info();
    if (aif)
        cif->assign_alias(new FIOaliasTab(true, false, aif));
    else {
        unsigned int mask = prms->alias_mask();
        if (prms->filetype() == Fgds)
            mask |= CVAL_GDS;
        cif->assign_alias(NewTranslatingAlias(mask));
    }
    char *tbf = new char[strlen(dirpath) + 32];
    char *e = lstring::stpcpy(tbf, dirpath);
    *e++ = '/';
    strcpy(e, AliasFileName);
    cif->read_alias(tbf);
    delete [] tbf;

    if (!cif->setup_destination(prms->destination(), prms->filetype(),
            prms->to_cgd())) {
        delete cif;
        Errs()->add_error("TranslateDir:: destination setup failed.");
        return (OIerror);
    }

    OItype oiret = OIok;
    cif->begin_log(Physical);
    SetAllowPrptyStrip(true);

    cif->set_no_open_lib(true);
    cif->set_no_end_lib(true);
    cv_out *out = cif->peek_backend();
    if (!out->open_library(Physical, 1.0)) {
        Errs()->add_error("TranslateDir:: open_library failed.");
        oiret = OIerror;
    }

    if (oiret == OIok) {
        DIR *wdir = opendir(dirpath);
        if (wdir) {
            char *path = new char[strlen(dirpath) + 256];
            strcpy(path, dirpath);
            char *t = path + strlen(path) - 1;
            if (!lstring::is_dirsep(*t)) {
                t++;
                *t++ = '/';
                *t = 0;
            }
            else
                t++;

            struct dirent *de;
            while ((de = readdir(wdir)) != 0) {
                if (!strcmp(de->d_name, "."))
                    continue;
                if (!strcmp(de->d_name, ".."))
                    continue;
                const char *tmp = strrchr(de->d_name, '.');
                if (tmp && lstring::cieq(tmp, ".bak"))
                    continue;
                if (LookupLibCell(0, de->d_name, LIBdevice, 0))
                    continue;
                strcpy(t, de->d_name);

                FILE *fp = fopen(path, "r");
                if (!fp)
                    continue;
                CFtype ctype;
                bool ret = (!IsCIF(fp, &ctype, 0) || ctype != CFnative);
                fclose(fp);
                if (ret)
                    continue;

                oiret = cif->translate_native_cell(path, Physical,
                    prms->scale());
                if (oiret != OIok) {
                    Errs()->add_error(
                        "TranslateDir:: translation of cell %s failed.",
                        de->d_name);
                    break;
                }
            }
            closedir(wdir);
            delete [] path;
        }
    }
    if (oiret == OIok && !out->write_endlib(0)) {
        Errs()->add_error("TranslateDir:: close_library failed.");
        oiret = OIerror;
    }
    cif->end_log();

    if (oiret == OIok && !fioStripForExport && !prms->to_cgd() &&
            !CD()->IsNoElectrical()) {
        cif->begin_log(Electrical);
        SetAllowPrptyStrip(true);

        cif->set_no_open_lib(true);
        cif->set_no_end_lib(true);
        out = cif->peek_backend();
        if (!out->open_library(Electrical, 1.0)) {
            Errs()->add_error("TranslateDir:: open_library failed.");
            oiret = OIerror;
        }

        if (oiret == OIok) {
            DIR *wdir = opendir(dirpath);
            if (wdir) {
                char *path = new char[strlen(dirpath) + 256];
                strcpy(path, dirpath);
                char *t = path + strlen(path) - 1;
                if (!lstring::is_dirsep(*t)) {
                    t++;
                    *t++ = '/';
                    *t = 0;
                }
                else
                    t++;

                struct dirent *de;
                while ((de = readdir(wdir)) != 0) {
                    if (!strcmp(de->d_name, "."))
                        continue;
                    if (!strcmp(de->d_name, ".."))
                        continue;
                    const char *tmp = strrchr(de->d_name, '.');
                    if (tmp && lstring::cieq(tmp, ".bak"))
                        continue;
                    if (LookupLibCell(0, de->d_name, LIBdevice, 0))
                        continue;
                    strcpy(t, de->d_name);

                    FILE *fp = fopen(path, "r");
                    if (!fp)
                        continue;
                    CFtype ctype;
                    bool ret = (!IsCIF(fp, &ctype, 0) || ctype != CFnative);
                    fclose(fp);
                    if (ret)
                        continue;

                    oiret = cif->translate_native_cell(path, Electrical, 1.0);
                    if (oiret != OIok) {
                        Errs()->add_error(
                            "TranslateDir:: translation of cell %s failed.",
                            de->d_name);
                        break;
                    }
                }
                closedir(wdir);
                delete [] path;
            }
        }
        if (oiret == OIok && !out->write_endlib(0)) {
            Errs()->add_error("TranslateDir:: close_library failed.");
            oiret = OIerror;
        }
        cif->end_log();
    }

    delete cif;
    SetAllowPrptyStrip(false);
    return (oiret);
}
// End of cFIO functions.


char *
cif_info::pr_records(FILE*)
{
    return (0);
}
// End of cif_info functions.


cif_in::cif_in(bool allow_layer_mapping, CFtype t) : cv_in(allow_layer_mapping)
{
    in_filetype = Fcif;
    in_action = cvOpenModeNone;
    in_resol_scale = 1.0;
    in_fp = 0;

    in_curlayer = 0;
    in_line_cnt = 0;
    in_cif_type = t;
    in_phys_map = 0;
    in_elec_map = 0;
    in_eos_tab = 0;

    in_phys_res_found = 0;
    in_elec_res_found = 0;

    in_native = false;
    in_found_data = false;
    in_found_resol = false;
    in_found_elec = false;
    in_found_phys = false;
    in_in_root = true;
    in_skip = false;
    in_no_create_layer = false;
    in_update_style = false;
    in_cnametype_set = false;
    in_layertype_set = false;
    in_labeltype_set = false;
    in_check_intersect = false;
    *in_cellname = 0;
    strcpy(in_curlayer_name, "####");
}


cif_in::~cif_in()
{
    if (in_fp && !in_native)
        fclose(in_fp);
    delete in_phys_map;
    delete in_elec_map;
    delete in_eos_tab;
}


// Set to native input mode.
//
void
cif_in::setup_native(const char *name, FILE *fp, CDs *sdesc)
{
    strcpy(in_cellname, lstring::strip_path(name));
    in_fp = fp;
    in_calldesc.name = in_cellname;
    in_sdesc = sdesc;
    in_native = true;
}


// Read the native cell in spath, for translation.  On the first pass
// (physical mode) save the ending offset in a table.  On the second
// pass (Electrical), start reading at this offset, which must exist.
//
OItype
cif_in::translate_native_cell(const char *spath, DisplayMode mode,
    double phys_scale)
{
    if (!spath) {
        Errs()->add_error("TranslateNative: null cell name encountered.");
        return (OIerror);
    }
    const char *sname = lstring::strip_path(spath);
    if (!*sname) {
        Errs()->add_error("TranslateNative: empty cell name encountered.");
        return (OIerror);
    }

    OItype oiret = OIok;
    sLibRef *libref = 0;
    sLib *libptr = 0;

    FILE *fp = FIO()->POpen(spath, "rb", 0,
        LIBdevice | LIBuser | LIBnativeOnly, &libref, &libptr);
    if (!fp) {
        Errs()->sys_error("open");
        Errs()->add_error("Can't open cell %s.", sname);
        oiret = OIerror;
    }

    bool ret = true;
    if (!libptr || libptr->lib_type() != LIBdevice) {
        setup_native(spath, fp, 0);
        ret = parse(mode, false, phys_scale);
    }

    if (!ret)
        oiret = was_interrupted() ? OIaborted : OIerror;

    fclose(fp);
    return (oiret);
}


//
// Setup functions
//

// Open the source file.
//
bool
cif_in::setup_source(const char *cif_fname, const cCHD*)
{
    in_fp = FIO()->POpen(cif_fname, "rb");
    if (!in_fp) {
        Errs()->sys_error("open");
        Errs()->add_error("Can't open CIF file %s.", cif_fname);
        return (false);
    }
    in_filename = lstring::copy(cif_fname);

    return (true);
}


// Set up the destination channel.
//
bool
cif_in::setup_destination(const char *destination, FileType ftype,
    bool to_cgd)
{
    if (destination) {
        in_out = FIO()->NewOutput(in_filename, destination, ftype, to_cgd);
        if (!in_out)
            return (false);
        in_own_in_out = true;
    }
    in_action = cvOpenModeTrans;
    return (true);
}


// Explicitly set the back-end processor, and reset a few things.
//
bool
cif_in::setup_backend(cv_out *out)
{
    if (in_fp)
        rewind(in_fp);
    in_bytes_read = 0;
    in_fb_incr = UFB_INCR;
    in_offset = 0;
    in_out = out;
    in_own_in_out = false;
    in_action = cvOpenModeTrans;
    return (true);
}


// Notes On Scaling
//
//    scale       (overall per-mode, from command)
//    RESOLUTION  (overall per-mode, from file)
//    A/B         per cell
//
//    initial:
//        in_ext_phys_scale = scale (physical)
//
//    in read_header()
//        in_resol_scale = in_ext_phys_scale*resolution; (physical)
//        in_resol_scale = resolution; (electrical)
//        in_scale = in_resol_scale;
//        if (physical)
//            in_phys_scale = in_scale
//
//    in a_symbol()
//        in_scale = in_resol_scale*(A/B)
//        in_scale = in_resol_scale when finished
//
//    hy_scale(): multiplies hypertext reference coords, need this for
//        electrical mode label test when electrical mode scale != 1
//    CDp::scale():
//        phys mode: grid
//        elec mode: lots of coords, phys term locations
//
//    Cif from Xic always has A/B = 1.
//    Only Cif from Xic requires property/label scaling.
//
//    scaling struct property:
//        elec scale = elec_resol_scale
//        phys scale = phys_resol_scale*scale*A/B
//
//    scaling instance property:
//        elec scale = elec_resol_scale
//        phys scale = phys_resol_scale*scale*A/B, A/B in parent
//
//   To handle A/B, would need to save per struct A/B somewhere.  This
//   is not done, since records that use A/B will never have properties
//   that require scaling.

// Set this to create standard via masters when listing a hierarchy,
// such as when creating a CHD.  Creating these at this point might
// avoid name uncertainty later.
//
#define VIAS_IN_LISTONLY

// Main entry for reading.  If sc is not 1.0, geometry will be scaled.
// If listonly is true, the symbol offsets will be saved in the name
// table, but there is no conversion.
//
bool
cif_in::parse(DisplayMode mode, bool listonly, double sc, bool save_bb,
    cvINFO)
{
    if (listonly && (in_action != cvOpenModeDb || in_native))
        return (true);
    in_mode = mode;
    if (in_mode == Physical) {
        in_ext_phys_scale = dfix(sc);
        rewind(in_fp);
    }

    // Note: info structs not supported.

    in_listonly = listonly;
    in_prpty_list = 0;
    in_found_data = false;
    in_found_resol = false;
    in_found_phys = false;
    in_found_elec = false;
    in_in_root = true;
    in_line_cnt = 0;

    GCarray<char*> GCtabname(0);
    char *tabname = 0;
    if (in_native) {
        // The setup_native function has been called, in_cellname
        // contains the unaliased name.
        tabname = lstring::copy(alias(in_cellname));
        GCtabname.set(tabname);

        if (in_mode == Electrical && tabname && in_eos_tab) {
            cif_eos_t *el = (cif_eos_t*)SymTab::get(in_eos_tab, tabname);
            if (el != (cif_eos_t*)ST_NIL) {
                in_line_cnt = el->e_lines;
                fseek(in_fp, el->e_offset, SEEK_SET);
            }
        }
    }
    in_cellname[0] = '\0';

    in_savebb = save_bb;
    in_ignore_text = in_mode == Physical && FIO()->IsNoReadLabels();
    in_header_read = false;

    bool brk;
    if (!read_header(&brk))
        return (false);

    if (in_action == cvOpenModeTrans && !brk && !in_cv_no_openlib) {
        if (!in_out->open_library(in_mode, 1.0))
            return (false);
    }

    while (!brk) {
        bool abrt;
        in_offset = large_ftell(in_fp);
        int c;
        if (!read_character(&c, &abrt, PSTRIP2, PFAILONEOF))
            return (false);
        if (abrt)
            return (false);

        if (c == 'D') {
            if (!read_character(&c, &abrt, PSTRIP2, PFAILONEOF))
                return (false);
            if (abrt)
                return (false);
            if (c == 'S') {
                if (!a_symbol())
                    return (false);
                if (!a_end_symbol())
                    return (false);
            }
            else if (c == 'D') {
                if (!a_delete_symbol())
                    return (false);
            }
        }
        else if (c == 'E') {
            c = getc(in_fp);
            if (c == 'n' || c == 'N') {
                in_bytes_read++;
                c = getc(in_fp);
                if (c == 'd' || c == 'D')
                    in_bytes_read++;
                else
                    ungetc(c, in_fp);
            }
            else
                ungetc(c, in_fp);
            if (in_action == cvOpenModeTrans && !in_cv_no_endlib) {
                if (!in_out->write_endlib(0))
                    return (false);
            }
            break;  // done
        }
        else if (!dispatch(c, &abrt)) {
            if (abrt)
                error(PERRETC, "syntax error");
            return (false);
        }
    }
    if (in_native && in_mode == Physical && tabname) {
        if (!in_eos_tab)
            in_eos_tab = new SymTab(true, false);
        cif_eos_t *el = in_eos_fct.new_element();
        el->e_offset = ftell(in_fp);
        el->e_lines = in_line_cnt;
        in_eos_tab->add(tabname, el, false);
        GCtabname.clear();
    }
    return (true);
}


//
// Entries for reading through CHD.
//

// Setup scaling and read the file header.
// Assumed here that electrical mode resolution is always 1000.
//
bool
cif_in::chd_read_header(double phys_scale)
{
    if (in_native)
        return (false);
    bool ret = true;
    if (in_mode == Physical) {
        in_ext_phys_scale = dfix(phys_scale);

        bool brk;
        ret = read_header(&brk) || brk;
    }
    else {
        in_resol_scale = 1.0;
        in_scale = 1.0;
        in_needs_mult = false;

        // not supported in electrical mode
        in_areafilt = false;
        in_flatmax = -1;
        in_clip = false;
    }
    return (ret);
}


// Access a cell in the file through the symbol reference pointer.
// This can be used for reading into memory or translation, and is not
// recursive.  The second arg is a return for the new cell opened in
// the database, if any.
//
bool
cif_in::chd_read_cell(symref_t *p, bool use_inst_list, CDs **sdret)
{
    if (sdret)
        *sdret = 0;

    if (FIO()->IsUseCellTab()) {
        if (in_mode == Physical && 
                ((in_action == cvOpenModeTrans) ||
                ((in_action == cvOpenModeDb) && in_flatten))) {

            // The AuxCellTab may contain cells to stream out from the main
            // database, overriding the cell data in the CHD.

            OItype oiret = chd_process_override_cell(p);
            if (oiret == OIerror)
                return (false);
            if (oiret == OInew)
                return (true);
        }
    }
    if (!p->get_defseen()) {
        CDs *sd = CDcdb()->findCell(p->get_name(), in_mode);
        if (sd && in_mode == Physical && 
                ((in_action == cvOpenModeTrans) ||
                ((in_action == cvOpenModeDb) && in_flatten))) {

            // The cell definition was not in the file.  This could mean
            // that the cell is a standard via or parameterized cell
            // sub-master, or is a native library cell.  In any case, it
            // should be in memory.

            if (sd && (sd->isViaSubMaster() || sd->isPCellSubMaster()))
                return (chd_output_cell(sd) == OIok);
        }
        if (sd && (in_action == cvOpenModeDb))
            return (true);
        FIO()->ifPrintCvLog(IFLOG_WARN, "Unresolved cell %s (%s).",
            DisplayModeNameLC(in_mode), Tstring(p->get_name()),
            sd ? "in memory" : "not found");
        return (true);
    }

    large_fseek(in_fp, p->get_offset(), SEEK_SET);

    CD()->InitCoincCheck();
    in_uselist = true;
    in_prpty_list = 0;
    in_cellname[0] = '\0';
    bool ret = true;

    cv_chd_state stbak;
    in_chd_state.push_state(p, use_inst_list ? get_sym_tab(in_mode) : 0,
        &stbak);

    // Skip labels when flattening subcells when flag set.
    bool bktxt = in_ignore_text;
    if (in_mode == Physical && in_flatten && in_transform > 0 &&
            FIO()->IsNoFlattenLabels())
        in_ignore_text = true;

    for (;;) {
        int c;
        bool abrt;
        in_offset = large_ftell(in_fp);
        if (!read_character(&c, &abrt, PSTRIP2, PFAILONEOF) || abrt) {
            ret = false;
            break;
        }
        if (c == 'D') {
            if (!read_character(&c, &abrt, PSTRIP2, PFAILONEOF) || abrt) {
                ret = false;
                break;
            }
            if (c == 'S') {
                if (!a_symbol()) {
                    ret = false;
                    break;
                }
                if (sdret)
                    *sdret = in_sdesc;
                if (!a_end_symbol()) {
                    ret = false;
                    break;
                }
                break;
            }
            error(PERRETC, "syntax error");
            ret = false;
            break;
        }
        if (!dispatch(c, &abrt)) {
            if (abrt)
                error(PERRETC, "syntax error");
            ret = false;
            break;
        }
    }
    in_ignore_text = bktxt;
    in_chd_state.pop_state(&stbak);

    if (!ret && in_out && in_out->was_interrupted())
        in_interrupted = true;

    return (ret);
}


cv_header_info *
cif_in::chd_get_header_info()
{
    if (!in_header_read)
        return (0);
    int res = in_phys_res_found;
    if (!res)
        res = 100;
    double u = 1e-6/res;
    cif_header_info *cif = new cif_header_info(u);
    cif->phys_res_found = in_phys_res_found;
    cif->elec_res_found = in_elec_res_found;
    in_header_read = false;
    return (cif);
}


void
cif_in::chd_set_header_info(cv_header_info *hinfo)
{
    if (!hinfo || in_header_read)
        return;
    cif_header_info *cif = static_cast<cif_header_info*>(hinfo);
    in_phys_res_found = cif->phys_res_found;
    in_elec_res_found = cif->elec_res_found;
    in_header_read = true;
}


//
// Misc. entries.
//

// Return true if electrical records present.
//
bool
cif_in::has_electrical()
{
    int c;
    while ((c = getc(in_fp)) != EOF) {
        // electrical part should start:
        // (ELECTRICAL);  (optiona)
        // 5 ... ;        (optional)
        // 9 cellname;    (mandatory)
        //
        if (c == '(' || c == '9' || c == '5')
            break;
        if (c == '\n')
            in_line_cnt++;
    }
    if (c == EOF) {
        clearerr(in_fp);
        return (false);
    }
    ungetc(c, in_fp);
    return (true);
}


// Check if the cell contains any geometry that would make it through
// any layer filtering, overlapping AOI if given.  Subcells are
// ignored here.
// Returns:
//   OIaborted
//   OIerror
//   OIok          has geometry
//   OIambiguous   no geometry
//
OItype
cif_in::has_geom(symref_t *p, const BBox *AOI)
{
    if (!p->get_defseen()) {
        // No cell definition in file, just ignore this.
        return (OIambiguous);
    }
    large_fseek(in_fp, p->get_offset(), SEEK_SET);

    in_uselist = true;
    in_prpty_list = 0;
    in_cellname[0] = '\0';

    for (;;) {
        int c;
        bool abrt;
        in_offset = large_ftell(in_fp);
        if (!read_character(&c, &abrt, PSTRIP2, PFAILONEOF))
            return (OIerror);
        if (abrt)
            return (OIaborted);
        if (c == 'D') {
            if (!read_character(&c, &abrt, PSTRIP2, PFAILONEOF))
                return (OIerror);
            if (abrt)
                return (OIaborted);
            if (c == 'S')
                break;
            error(PERRETC, "syntax error");
            return (OIerror);
        }
    }

    if (!in_cell_offset)
        in_cell_offset = in_offset;
    int sym_num;
    if (!read_integer(&sym_num))
        return (OIerror);
    int A = 1;
    int B = 1;
    // look for scaling factor
    int c;
    if (!look_ahead(&c, PSTRIP3))
        return (OIerror);
    if (isdigit(c)) {
        if (!read_point(&A, &B))
            return (OIerror);
    }
    if (!look_for_semi(&c))
        return (OIerror);

    in_in_root = false;
    in_curlayer = 0;

    if (!symbol_name(sym_num))
        return (OIerror);

    clear_properties();
    *in_cellname = '\0';

    BBox tmpBB = in_cBB;
    if (AOI) {
        in_cBB = *AOI;
        in_check_intersect = true;
    }

    in_no_create_layer = true;
    cvOpenMode atmp = in_action;
    in_action = cvOpenModeDb;
    OItype oiret = OIok;
    bool found = false;
    for (;;) {
        bool abrt;
        if (!read_character(&c, &abrt, PSTRIP2, PFAILONEOF)) {
            oiret = OIerror;
            break;
        }
        if (abrt) {
            oiret = OIaborted;
            break;
        }
        if (!dispatch(c, &abrt)) {
            if (abrt) {
                oiret = OIaborted;
                break;
            }
            oiret = OIerror;
            break;
        }
        if (c == 'P' || c == 'B' || c == 'W' || c == 'R') {
            if (!in_skip) {
                found = true;
                break;
            }
        }
    }
    if (oiret == OIok && !found)
        oiret = OIambiguous;

    in_action = atmp;
    in_no_create_layer = false;
    in_check_intersect = false;
    in_cBB = tmpBB;
    return (oiret);
}
// End of virtual overrides.


//
//---- Private Functions -----------
//

bool
cif_in::read_header(bool *brk)
{
    *brk = false;
    if (in_header_read) {
        if (in_mode == Physical) {
            int res = in_phys_res_found;
            if (!res)
                res = 100;
            else
                in_found_resol = true;
            in_resol_scale =
                dfix((in_ext_phys_scale*CDphysResolution)/res);
            in_scale = in_resol_scale;
            in_needs_mult = (in_scale != 0.0);
        }
        else {
            int res = in_elec_res_found;
            if (!res)
                res = 100;
            else
                in_found_resol = true;
            in_resol_scale =
                dfix(CDphysResolution/res);
            in_scale = in_resol_scale;
            in_needs_mult = (in_scale != 0.0);
        }
        return (true);
    }

    // skip past white space
    int c = getc(in_fp);
    in_bytes_read++;
    if (c == '\n')
        in_line_cnt++;
    while (isspace(c)) {
        c = getc(in_fp);
        in_bytes_read++;
        if (c == '\n')
            in_line_cnt++;
    }

    // strip off any "#!..." shell header
    if (c == '#') {
        c = getc(in_fp);
        if (c == '!') {
            while ((c = getc(in_fp)) != EOF) {
                if (c == '\n')
                    break;
            }
        }
        else
            ungetc(c, in_fp);
            // '#' would be stripped anyway
    }
    else if (c == '*') {
        // '*' to fix 2.5.38 error where spice comment might appear
        // after phys-mode record.
        if (in_mode == Electrical) {
            *brk = true;
            return (true);
        }
        ungetc(c, in_fp);
    }
    else if (c == EOF) {
        if (in_mode != Electrical)
            // don't fail on start of electrical, may be blank
            return (error(PERREOF, 0));
        *brk = true;
        return (true);
    }
    else
        ungetc(c, in_fp);

    bool firstc = true;
    for (;;) {
        bool abrt;
        in_offset = large_ftell(in_fp);
        read_character(&c, &abrt, PSTRIP2, PDONTFAILONEOF);
        if (abrt)
            return (false);

        if (c == '(') {
            if (!a_comment())
                return (false);
        }
        else if (c == ';')
            continue;
        else if (c == EOF) {
            if (in_mode != Electrical || !firstc)
                // don't fail on start of electrical, may be blank
                return (error(PERREOF, 0));
            *brk = true;
            return (true);
        }
        else {
            ungetc(c, in_fp);
            break;
        }
        firstc = false;
    }
    if (in_native) {
        if (in_mode == Physical) {
            if (in_found_elec) {
                if (in_found_phys)
                    return (error(PERRETC,
                        "conflicting physical/electrical tags"));
                // no physical data
                rewind(in_fp);
                in_line_cnt = 0;
                *brk = true;
                return (true);
            }
            if (!in_found_phys) {
                // old style file
                bool issced;
                if (!test_oldstyle(in_fp, &issced))
                    return (error(PERRETC,
                        "incorrect format, no physical stub"));
                if (issced) {
                    // done with physical (nothing there)
                    rewind(in_fp);
                    in_line_cnt = 0;
                    *brk = true;
                    return (true);
                }
                in_found_phys = true;
            }
        }
        else if (in_mode == Electrical) {
            if (in_found_phys)
                return (error(PERRETC,
                    "physical tag found in electrical data"));
            in_found_elec = true;
        }
    }
    if (!in_found_data) {
        in_found_data = true;
        if (!in_found_resol) {
            if (in_mode == Physical)
                in_resol_scale =
                    dfix((in_ext_phys_scale*CDphysResolution)/100);
            else
                in_resol_scale = dfix(CDelecResolution/100);
            in_scale = in_resol_scale;
            in_needs_mult = (in_scale != 0.0);
            FIO()->ifPrintCvLog(IFLOG_INFO, "Default resolution (100).");
        }
    }
    if (in_mode == Physical)
        in_phys_scale = in_scale;
    in_header_read = true;
    return (true);
}


bool
cif_in::dispatch(int key, bool *nogood)
{
    *nogood = false;
    if (key == '(')
        return (a_comment());
    if (key == ';')
        return (true);
    if (key == 'P')
        return (a_polygon());
    if (key == 'B')
        return (a_box());
    if (key == 'W')
        return (a_wire());
    if (key == 'L')
        return (a_layer());
    if (key == 'C')
        return (a_call());
    if (key == 'R')
        return (a_round_flash());
    if (isdigit(key))
        return (a_extension(key));
    *nogood = true;
    return (false);
}


// This function begins the parsing action for a symbol definition
// and performs all necessary initialization for the new symbol.
//
bool
cif_in::a_symbol()
{
    if (!in_cell_offset)
        in_cell_offset = in_offset;
    int sym_num;
    if (!read_integer(&sym_num))
        return (false);
    int A = 1;
    int B = 1;
    // look for scaling factor
    int c;
    if (!look_ahead(&c, PSTRIP3))
        return (false);
    if (isdigit(c)) {
        if (!read_point(&A, &B))
            return (false);
    }
    if (!look_for_semi(&c))
        return (false);

    in_in_root = false;
    in_curlayer = 0;
    CD()->InitCoincCheck();
    if (!symbol_name(sym_num))
        return (false);
    char unalias_name[256];
    strcpy(unalias_name, in_cellname);
    const char *sn = in_chd_state.symref() ?
        Tstring(in_chd_state.symref()->get_name()) : alias(in_cellname);
    if (sn != in_cellname)
        strcpy(in_cellname, sn);

    if (in_mode == Physical && !in_native) {
        in_scale = dfix((in_resol_scale*A)/B);
        in_needs_mult = (in_scale != 0.0);
    }
    if (in_action == cvOpenModeTrans) {
        if (!a_symbol_cvt(unalias_name, sym_num))
            return (false);
    }
    else {
        if (!a_symbol_db(unalias_name, sym_num))
            return (false);
    }

    clear_properties();
    *in_cellname = '\0';

    for (;;) {
        bool abrt;
        if (!read_character(&c, &abrt, PSTRIP2, PFAILONEOF))
            return (false);
        if (abrt)
            return (false);
        if (!dispatch(c, &abrt)) {
            if (abrt)
                break;
            return (false);
        }
    }
    if (c != 'D')
        return (error(PERRETC, "Can't understand next command"));
    bool abrt;
    if (!read_character(&c, &abrt, PSTRIP2, PFAILONEOF) || c != 'F')
        return (error(PERRETC, "Can't understand next command"));
    if (abrt)
        return (false);
    if (!look_for_semi(&c))
        return (false);

    return (true);
}


bool
cif_in::a_symbol_db(const char *unalias_name, int sym_num)
{
    if (in_native) {
        if (in_sdesc) {
            if (!*in_cellname)
                // bad, no name given
                return (error(PERRETC, "unnamed symbol"));
            FIO()->ifPrintCvLog(IFLOG_INFO, "Processing %s records of %s.",
                DisplayModeNameLC(in_mode), in_cellname);
            FIO()->ScalePrptyStrings(in_prpty_list, in_phys_scale, in_scale,
                in_mode);
            add_properties_db(in_sdesc, 0);
        }
    }
    else {
        in_sdesc = 0;
        symref_t *srf = 0;
        bool dup_sym = false;
        if (in_uselist) {
            if (in_chd_state.symref())
                srf = in_chd_state.symref();
            else {
                // CHD name might be aliased or not.
                srf = get_symref(in_cellname, in_mode);
                if (!srf)
                    srf = get_symref(unalias_name, in_mode);
                if (!srf) {
                    Errs()->add_error("Can't find %s in CHD.", in_cellname);
                    return (error(PERRCD, 0));
                }
            }
        }
        else {
            nametab_t *ntab = get_sym_tab(in_mode);
            srf = get_symref(in_cellname, in_mode);
            if (!srf) {
                // The srf is not in the name table, however it might be
                // in the numtab if it was already referenced without a
                // name.  Create a numtab if it doesn't exist.

                numtab_t *nutab = numtab();
                if (!nutab) {
                    nutab = new numtab_t;
                    set_numtab(nutab);
                }
                else
                    srf = nutab->get(sym_num);

                if (srf) {
                    // Found the srf in the numtab, set the name and
                    // add srf to the name table.

                    srf->set_name(CD()->CellNameTableAdd(in_cellname));
                    add_symref(srf, in_mode);
                }
                else {
                    // Not found in either table, create a new symref
                    // and add it to both tables.

                    ntab->new_symref(in_cellname, in_mode, &srf, true);
                    add_symref(srf, in_mode);
                    srf->set_num(sym_num);
                    nutab->add(srf);
                }
            }
            if (in_listonly) {
                if (srf && srf->get_defseen()) {
                    FIO()->ifPrintCvLog(IFLOG_WARN,
                        "Duplicate symbol definition for %s at offset %llu.",
                        in_cellname, (unsigned long long)in_offset);
                    dup_sym = true;
                }

                // Flush instance list, this will be rebuilt.
                srf->set_cmpr(0);
                srf->set_crefs(0);
            }
            in_symref = srf;
            in_cref_end = 0;  // used as end pointer for cref_t list
            srf->set_defseen(true);
            srf->set_num(sym_num);
            srf->set_offset(in_cell_offset);
        }

        if (in_listonly) {
            if (in_savebb)
                in_cBB = CDnullBB;
            clear_properties();
            return (true);
        }

        CDs *sd = CDcdb()->findCell(in_cellname, in_mode);
        if (!sd) {
            // doesn't already exist, open it
            sd = CDcdb()->insertCell(in_cellname, in_mode);
            sd->setFileType(Fcif);
            sd->setFileName(in_filename);
            if (in_lcheck || in_layer_alias || in_phys_scale != 1.0)
                sd->setAltered(true);
            in_sdesc = sd;
        }
        else if (sd->isLibrary() && (FIO()->IsReadingLibrary() ||
                FIO()->IsNoOverwriteLibCells())) {
            // Skip reading into library cells.
            if (!FIO()->IsReadingLibrary()) {
                FIO()->ifPrintCvLog(IFLOG_WARN,
                    "can't overwrite library cell %s already in memory.",
                    in_cellname);
            }
        }
        else if (sd->isLibrary() && sd->isDevice()) {
            const char *nname = handle_lib_clash(in_cellname);
            if (nname)
                strcpy(in_cellname, nname);
        }
        else if (in_mode == Physical) {
            bool reuse = false;
            if (sd->isUnread()) {
                sd->setUnread(false);
                reuse = true;
            }
            else {
                mitem_t mi(in_cellname);
                if (sd->isViaSubMaster()) {
                    mi.overwrite_phys = false;
                    mi.overwrite_elec = false;
                }
                else if (dup_sym) {
                    mi.overwrite_phys = true;
                    mi.overwrite_elec = true;
                }
                else {
                    if (!FIO()->IsNoOverwritePhys())
                        mi.overwrite_phys = true;
                    if (!FIO()->IsNoOverwriteElec())
                        mi.overwrite_elec = true;
                    FIO()->ifMergeControl(&mi);
                }
                if (mi.overwrite_phys) {
                    sd->setImmutable(false);
                    sd->setLibrary(false);
                    sd->clear(true);
                    reuse = true;
                }
                // Save the electrical overwrite flag, presence in the
                // table means that we have prompted for this cell.
                if (!in_over_tab)
                    in_over_tab = new SymTab(false, false);
                in_over_tab->add((unsigned long)sd->cellname(),
                    (void*)(long)mi.overwrite_elec, false);
                if (mi.overwrite_elec) {
                    // If overwriting electrical, clear the existing
                    // electrical cell (if any) here.  There may not
                    // be a matching electrical cell from the file, in
                    // which case it won't get cleared in the code
                    // below.
                    CDs *tsd = CDcdb()->findCell(sd->cellname(), Electrical);
                    if (tsd) {
                        tsd->setImmutable(false);
                        tsd->setLibrary(false);
                        tsd->clear(true);
                    }
                }
            }
            if (reuse) {
                sd->setFileType(Fcif);
                sd->setFileName(in_filename);
                if (in_lcheck || in_layer_alias || in_phys_scale != 1.0)
                    sd->setAltered(true);
                in_sdesc = sd;
            }
        }
        else {
            bool reuse = false;
            if (sd->isUnread()) {
                sd->setUnread(false);
                reuse = true;
            }
            else {
                void *xx;
                if (in_over_tab && (xx = SymTab::get(in_over_tab,
                        (unsigned long)sd->cellname())) != ST_NIL) {
                    // We already asked about overwriting.
                    if (xx) {
                        // User chose to overwrite the electrical
                        // part, the cell has already been cleared.
                        reuse = true;
                    }
                }
                else {
                    // Physical part of this cell (if any) has already
                    // been read, and there was no conflict.

                    mitem_t mi(in_cellname);
                    if (dup_sym) {
                        mi.overwrite_phys = true;
                        mi.overwrite_elec = true;
                    }
                    else {
                        if (!FIO()->IsNoOverwritePhys())
                            mi.overwrite_phys = true;
                        if (!FIO()->IsNoOverwriteElec())
                            mi.overwrite_elec = true;
                        FIO()->ifMergeControl(&mi);
                    }
                    if (mi.overwrite_phys) {
                        if (!get_symref(Tstring(sd->cellname()), Physical)) {
                            // The phys cell was not read from the
                            // current file.  If overwriting, clear
                            // existing phys cell.
                            CDs *tsd = CDcdb()->findCell(sd->cellname(),
                                Physical);
                            if (tsd) {
                                tsd->setImmutable(false);
                                tsd->setLibrary(false);
                                tsd->clear(true);
                            }
                        }
                    }
                    if (mi.overwrite_elec) {
                        sd->setImmutable(false);
                        sd->setLibrary(false);
                        sd->clear(true);
                        reuse = true;
                    }
                }
            }
            if (reuse) {
                sd->setFileType(Fcif);
                sd->setFileName(in_filename);
                if (in_lcheck || in_layer_alias || in_phys_scale != 1.0)
                    sd->setAltered(true);
                in_sdesc = sd;
            }
        }
        if (in_symref) {
            FIO()->ScalePrptyStrings(in_prpty_list, in_phys_scale,
                in_scale, in_mode);
            add_properties_db(in_sdesc, 0);
        }
        if (in_savebb)
            in_cBB = CDnullBB;
    }
    clear_properties();
    return (true);
}


bool
cif_in::a_symbol_cvt(const char*, int)
{
    bool ret = true;
    if (in_transform <= 0) {
        time_t now = time(0);
        struct tm *tgmt = gmtime(&now);
        ret = in_out->write_struct(in_cellname, tgmt, tgmt);
    }
    in_out->clear_property_queue();
    return (ret);
}


bool
cif_in::a_end_symbol()
{
    if (in_listonly) {
        if (in_savebb && in_symref)
            in_symref->set_bb(&in_cBB);
        if (in_cref_end) {
            nametab_t *ntab = get_sym_tab(in_mode);
            cref_t *c = ntab->find_cref(in_cref_end);
            c->set_last_cref(true);
            in_cref_end = 0;

            if (!ntab->cref_cmp_test(in_symref, in_cref_end, CRCMP_end))
                return (false);
        }
        in_symref = 0;
    }
    else if (in_action == cvOpenModeTrans) {
        if (in_transform <= 0) {
            if (!in_flatten) {
                if (!in_out->write_end_struct())
                    return (false);
            }
        }
    }
    else if (in_action == cvOpenModeDb && in_sdesc) {
        if (in_mode == Physical) {
            if (!in_native && !in_savebb && !in_no_test_empties &&
                    in_sdesc->isEmpty()) {
                FIO()->ifPrintCvLog(IFLOG_INFO,
                    "Symbol %s physical definition is empty at line %d.",
                    in_sdesc->cellname(), in_line_cnt);
            }
        }
        else {
            // This sets up the pointers for bound labels.
            in_sdesc->prptyPatchAll();
        }
    }
    if (in_sdesc && FIO()->IsReadingLibrary()) {
        in_sdesc->setImmutable(true);
        in_sdesc->setLibrary(true);
    }

    in_in_root = true;
    in_sdesc = 0;
    in_scale = in_resol_scale;
    in_needs_mult = (in_scale != 0.0);
    in_cell_offset = 0;
    return (true);
}


// We do not deal with definition deletes.
// It could be handled by using the symbol table to obtain the
// respective symbol numbers, and invoking CDClose on those cell
// definitions to be deleted.
//
// Ignore DD commands.
//
bool
cif_in::a_delete_symbol()
{
    int sym_num;
    if (!read_integer(&sym_num))
        return (false);
    int c;
    if (!look_for_semi(&c))
        return (false);
    char buf[64];
    sprintf(buf, "definition delete (DD) of Symbol %d - ignored", sym_num);
    warning(buf, false);
    return (true);
}


// Read the instance call lines and dispatch.
//
bool
cif_in::a_call()
{
    int sym_num = 0;
    if (!read_integer(&sym_num))
        return (false);

    in_tx.clear();
    for (;;) {
        int c = 0;
        bool abrt = false;
        if (!read_character(&c, &abrt, PSTRIP2, PFAILONEOF))
            return (false);
        if (abrt)
            return (false);
        int x=0, y=0;
        if (c == 'T') {
            if (!read_point(&x, &y))
                return (error(PERRETC, "Can't parse translation transform."));
            in_tx.add_transform(CDTRANSLATE, scale(x), scale(y));
        }
        else if (c == 'M') {
            if (!read_character(&c, &abrt, PSTRIP2, PFAILONEOF))
                return (false);
            if (abrt)
                return (false);
            if (c == 'X')
                in_tx.add_transform(CDMIRRORX, x, y);
            else if (c == 'Y')
                in_tx.add_transform(CDMIRRORY, x, y);
            else
                return (error(PERRETC, "Can't parse mirror transform."));
        }
        else if (c == 'R') {
            if (!read_point(&x, &y))
                return (error(PERRETC, "Can't parse rotation transform."));
            in_tx.add_transform(CDROTATE, x, y);
        }
        else if (c == ';')
            break;
        else
            return (error(PERRETC, "Can't parse transformation."));
    }
    if (in_listonly) {
        if (in_symref) {

            nametab_t *ntab = get_sym_tab(in_mode);
            cref_t *sr=0;
            ticket_t ctk = ntab->new_cref(&sr);
            symref_t *ptr = 0;

            if (*in_cellname) {
                const char *sn = alias(in_cellname);
                if (sn != in_cellname)
                    strcpy(in_cellname, sn);
#ifdef VIAS_IN_LISTONLY
                if (in_mode == Physical) {
                    CDcellName cname = CD()->CellNameTableAdd(in_cellname);
                    cname = check_sub_master(cname);
                    strcpy(in_cellname, Tstring(cname));
                }
#endif
                ptr = get_symref(in_cellname, in_mode);
                if (!ptr) {
                    // The symref was not found in the name table.  It
                    // might be in the number table if it was
                    // referenced earlier without a name.  Check for
                    // this, also create a numtab if necessary.

                    // The sym_num must be larger than 0 here, since 0
                    // is reserved for the top-level cell.  However,
                    // all sym nums may be 0 in CIF files, in which
                    // case the numtab_t will not be used.

                    numtab_t *nutab = 0;
                    if (sym_num > 0) {
                        nutab = numtab();
                        if (!nutab) {
                            nutab = new numtab_t;
                            set_numtab(nutab);
                        }
                        else
                            ptr = nutab->get(sym_num);
                    }
                    if (ptr) {
                        // Found the symref in the numtab, assign a name
                        // and add it to name table.

                        ptr->set_name(CD()->CellNameTableAdd(in_cellname));
                        add_symref(ptr, in_mode);
                    }
                    else {
                        // Not found in either table.  Create a new symref.
                        // and add it to both.

                        sr->set_refptr(
                            ntab->new_symref(in_cellname, in_mode, &ptr, true));
                        add_symref(ptr, in_mode);
                        if (sym_num > 0) {
                            ptr->set_num(sym_num);
                            nutab->add(ptr);
                        }
                    }
                }
            }
            else {
                // No symbol name available.  Check for symref in numtab,
                // create numtab if necessary.

                if (sym_num == 0)
                    return (error(PERRETC, "Unnamed reference to cell 0."));

                numtab_t *nutab = numtab();
                if (!nutab) {
                    nutab = new numtab_t;
                    set_numtab(nutab);
                }
                else
                    ptr = nutab->get(sym_num);

                if (!ptr) {
                    // Not found in numtab, create a new symref for
                    // the sym_num with an empty name, and add this to
                    // the numtab.  This will be added to the name
                    // table later.

                    ntab->new_symref("", in_mode, &ptr, true);
                    ptr->set_num(sym_num);
                    nutab->add(ptr);
                }
            }

            // We always have a nonzero ptr here, link it to the cref.
            sr->set_refptr(ptr->get_ticket());

            in_tx.add_mag(in_calldesc.magn);
            CDattr at(&in_tx, 0);
            sr->set_pos_x(in_tx.tx);
            sr->set_pos_y(in_tx.ty);
            if (in_calldesc.nx > 1 || in_calldesc.ny > 1) {
                at.nx = in_calldesc.nx;
                at.ny = in_calldesc.ny;
                at.dx = in_calldesc.dx;
                at.dy = in_calldesc.dy;
            }
            sr->set_tkt(CD()->RecordAttr(&at));

            // keep the list in order of appearance!
            if (!in_cref_end) {
                in_symref->set_crefs(ctk);
                if (!ntab->cref_cmp_test(in_symref, ctk, CRCMP_start))
                    return (false);
            }
            else {
                if (!ntab->cref_cmp_test(in_symref, ctk, CRCMP_check))
                    return (false);
            }
            in_cref_end = ctk;
        }
        clear_properties();
        *in_cellname = '\0';
        in_calldesc.nx = in_calldesc.ny = 1;
        in_calldesc.dx = in_calldesc.dy = 0;
        in_calldesc.magn = 1.0;
        return (true);
    }
    if (in_action == cvOpenModeTrans)
        return (a_call_cvt(sym_num));
    return (a_call_db(sym_num));
}


// Processing for cvOpenModeDb.
//
bool
cif_in::a_call_db(int sym_num)
{
    if (in_chd_state.gen()) {
        if (!in_sdesc || !in_chd_state.symref()) {
            clear_properties();
            reset_call();
            return (true);
        }
        nametab_t *ntab = get_sym_tab(in_mode);
        const cref_o_t *c = in_chd_state.gen()->next();
        if (!c) {
            Errs()->add_error(
                "Context instance list ended prematurely in %s.",
                in_chd_state.symref()->get_name());
            return (false);
        }
        symref_t *cp = ntab->find_symref(c->srfptr);
        if (!cp) {
            Errs()->add_error(
                "Unresolved symref back-pointer from %s.",
                in_chd_state.symref()->get_name());
            return (false);
        }
        if (cp->should_skip()) {
            clear_properties();
            reset_call();
            return (true);
        }
        tristate_t fst = in_chd_state.get_inst_use_flag();
        if (fst == ts_unset) {
            clear_properties();
            reset_call();
            return (true);
        }

        CDcellName cname = cp->get_name();
        CDattr at;
        if (!CD()->FindAttr(c->attr, &at)) {
            Errs()->add_error(
                "Unresolved transform ticket %d.", c->attr);
            return (false);
        }
        int x, y, dx, dy;
        if (in_mode == Physical) {
            double tscale = in_ext_phys_scale;
            x = mmRnd(c->tx*tscale);
            y = mmRnd(c->ty*tscale);
            dx = mmRnd(at.dx*tscale);
            dy = mmRnd(at.dy*tscale);
        }
        else {
            x = c->tx;
            y = c->ty;
            dx = at.dx;
            dy = at.dy;
        }
        CDtx tx(at.refly, at.ax, at.ay, x, y, at.magn);
        CDap ap(at.nx, at.ny, dx, dy);

        cname = check_sub_master(cname);
        CallDesc calldesc(cname, 0);

        CDc *newo;
        if (OIfailed(in_sdesc->makeCall(&calldesc, &tx, &ap, CDcallDb, &newo)))
            return (false);
        FIO()->ScalePrptyStrings(in_prpty_list, in_phys_scale, in_scale,
            in_mode);
        add_properties_db(in_sdesc, newo);
        clear_properties();
        reset_call();
        return (true);
    }

    if (! (in_ignore_inst || (in_action == cvOpenModeDb && in_in_root)) ) {

        if (in_sdesc) {

            if (!*in_cellname) {
                symref_t *ptr = 0;
                if (in_mode == Physical) {
                    if (!in_phys_map)
                        in_phys_map = new numtab_t(in_phys_sym_tab);
                    ptr = in_phys_map->get(sym_num);
                }
                else {
                    if (!in_elec_map)
                        in_elec_map = new numtab_t(in_elec_sym_tab);
                    ptr = in_elec_map->get(sym_num);
                }
                if (ptr)
                    strcpy(in_cellname, Tstring(ptr->get_name()));
                else {
                    char buf[256];
                    sprintf(buf, "reference to undefined symbol %d", sym_num);
                    warning(buf, false);
                    sprintf(in_cellname, "%s%d", UNDEF_PREFIX, sym_num);
                }
            }
            else {
                const char *sn = alias(in_cellname);
                if (sn != in_cellname)
                    strcpy(in_cellname, sn);
            }

            CDcellName cname = CD()->CellNameTableAdd(in_cellname);
            cname = check_sub_master(cname);
            CallDesc cd(cname, 0);

            // If we're reading into the database and scaling, we
            // don't want to scale library, standard cell, or pcell
            // sub-masters.  Instead, these are always read in
            // unscaled, and instance placements are scaled.  We
            // check/set this here.

            double tmag = in_calldesc.magn;
            if (in_mode == Physical && in_needs_mult &&
                    in_action == cvOpenModeDb) {
                CDs *sd = CDcdb()->findCell(cname, in_mode);
                if (sd && (sd->isViaSubMaster() || sd->isPCellSubMaster() ||
                        sd->isLibrary()))
                    tmag *= in_phys_scale;
                else if (in_chd_state.chd()) {
                    symref_t *sr = in_chd_state.chd()->findSymref(cname,
                        in_mode);
                    if (!sr || !sr->get_defseen() || !sr->get_offset())
                        tmag *= in_phys_scale;
                }
            }
            CDtx tx = in_tx;
            tx.magn = tmag;

            CDap ap(in_calldesc.nx, in_calldesc.ny, in_calldesc.dx,
                in_calldesc.dy);
            CDc *newo;
            if (OIfailed(in_sdesc->makeCall(&cd, &tx, &ap, CDcallDb, &newo)))
                return (error(PERRCD, 0));
            FIO()->ScalePrptyStrings(in_prpty_list, in_phys_scale, in_scale,
                in_mode == Physical ? Physical : Electrical);
            add_properties_db(in_sdesc, newo);

            if (in_symref) {
                // Add a symref for this instance if necessary.
                nametab_t *ntab = get_sym_tab(in_mode);
                symref_t *ptr = get_symref(Tstring(cname), in_mode);
                if (!ptr) {
                    ntab->new_symref(Tstring(cname), in_mode, &ptr, true);
                    add_symref(ptr, in_mode);
                }
            }
        }
    }
    clear_properties();
    *in_cellname = '\0';
    in_calldesc.nx = in_calldesc.ny = 1;
    in_calldesc.dx = in_calldesc.dy = 0;
    in_calldesc.magn = 1.0;
    return (true);
}


// Processing for cvOpenModeTrans.
//
bool
cif_in::a_call_cvt(int sym_num)
{
    if (in_chd_state.gen()) {
        if (!in_chd_state.symref()) {
            in_out->clear_property_queue();
            reset_call();
            return (true);
        }
        nametab_t *ntab = get_sym_tab(in_mode);
        const cref_o_t *c = in_chd_state.gen()->next();
        if (!c) {
            Errs()->add_error(
                "Context instance list ended prematurely in %s.",
                in_chd_state.symref()->get_name());
            return (false);
        }
        symref_t *cp = ntab->find_symref(c->srfptr);
        if (!cp) {
            Errs()->add_error(
                "Unresolved symref back-pointer from %s.",
                in_chd_state.symref()->get_name());
            return (false);
        }
        if (cp->should_skip()) {
            in_out->clear_property_queue();
            reset_call();
            return (true);
        }
        tristate_t fst = in_chd_state.get_inst_use_flag();
        if (fst == ts_unset) {
            in_out->clear_property_queue();
            return (true);
        }

        CDcellName cellname = cp->get_name();
        CDattr at;
        if (!CD()->FindAttr(c->attr, &at)) {
            Errs()->add_error(
                "Unresolved transform ticket %d.", c->attr);
            return (false);
        }
        int x, y, dx, dy;
        if (in_mode == Physical) {
            double tscale = in_ext_phys_scale;
            x = mmRnd(c->tx*tscale);
            y = mmRnd(c->ty*tscale);
            dx = mmRnd(at.dx*tscale);
            dy = mmRnd(at.dy*tscale);
        }
        else {
            x = c->tx;
            y = c->ty;
            dx = at.dx;
            dy = at.dy;
        }

        Instance inst;
        inst.magn = at.magn;
        inst.name = Tstring(cellname);
        inst.nx = at.nx;
        inst.ny = at.ny;
        inst.dx = dx;
        inst.dy = dy;
        inst.origin.set(x, y);
        inst.reflection = at.refly;
        inst.set_angle(at.ax, at.ay);

        if (!a_call_backend(&inst, cp, (fst = ts_set)))
            return (false);
        in_out->clear_property_queue();
        reset_call();
        return (true);
    }

    bool ret = true;
    if (!(in_ignore_inst || (in_in_root && sym_num == 0))) {
        // Skip the phony "C 0;" at the end of the file.

        if (!*in_cellname) {
            symref_t *ptr = 0;
            if (in_mode == Physical) {
                if (!in_phys_map)
                    in_phys_map = new numtab_t(in_phys_sym_tab);
                ptr = in_phys_map->get(sym_num);
            }
            else {
                if (!in_elec_map)
                    in_elec_map = new numtab_t(in_elec_sym_tab);
                ptr = in_elec_map->get(sym_num);
            }
            if (ptr)
                strcpy(in_cellname, Tstring(ptr->get_name()));
            else {
                char buf[256];
                sprintf(buf, "reference to undefined symbol %d", sym_num);
                warning(buf, false);
                sprintf(in_cellname, "%s%d", UNDEF_PREFIX, sym_num);
            }
        }
        else {
            const char *sn = alias(in_cellname);
            if (sn != in_cellname)
                strcpy(in_cellname, sn);
        }

        Instance inst;
        inst.magn = in_calldesc.magn;
        inst.name = in_cellname;
        inst.nx = in_calldesc.nx;
        inst.ny = in_calldesc.ny;
        inst.dx = in_calldesc.dx;
        inst.dy = in_calldesc.dy;
        inst.origin.set(in_tx.tx, in_tx.ty);
        inst.reflection = in_tx.refly;
        inst.set_angle(in_tx.ax, in_tx.ay);

        ret = a_call_backend(&inst, 0, false);
    }
    in_out->clear_property_queue();
    *in_cellname = '\0';
    in_calldesc.nx = in_calldesc.ny = 1;
    in_calldesc.dx = in_calldesc.dy = 0;
    in_calldesc.magn = 1.0;
    return (ret);
}


bool
cif_in::a_call_backend(Instance *inst, symref_t *p, bool no_area_test)
{
    bool ret = true;
    if (in_flatmax >= 0 && in_transform < in_flatmax) {
        // Flattening (not used)
        return (false);
    }
    else if (in_areafilt) {
        //
        // Area filtering
        //
        if (p) {
            //
            // Using CHD to reference cells.  If a vec_ctab was given,
            // use it to resolve the bounding box.
            //
            if (!no_area_test) {
                const BBox *tsBB;
                if (in_chd_state.ctab()) {
                    tsBB = in_chd_state.ctab()->resolve_BB(
                        &p, in_chd_state.ct_index());
                    if (!tsBB)
                        return (true);
                }
                else
                    tsBB = p->get_bb();

                if (!tsBB) {
                    Errs()->add_error("Bounding box for master %s not found.",
                        Tstring(p->get_name()));
                    return (false);
                }
                BBox sBB(*tsBB);
                if (in_mode == Physical)
                    sBB.scale(in_ext_phys_scale);

                no_area_test = inst->check_overlap(this, &sBB, &in_cBB);
            }
            if (no_area_test)
                ret = in_out->write_sref(inst);
        }
    }
    else
        ret = in_out->write_sref(inst);
    return (ret);
}


// Read the box lines and dispatch appropriately.
//
bool
cif_in::a_box()
{
    int xdir = 1;
    int ydir = 0;
    int width;
    if (!read_integer(&width))
        return (false);
    int height;
    if (!read_integer(&height))
        return (false);
    int x, y;
    if (!read_point(&x, &y))
        return (false);
    int c;
    if (!look_for_semi(&c))
        return (false);
    if (c != ';') {
        if (!read_point(&xdir, &ydir))
            return (false);
        if (!look_for_semi(&c))
            return (false);
        if (c != ';')
            return (error(PERRNOSEMI, 0));
    }

    BBox BB;
    BB.set_cif(x, y, width, height);
    if (in_needs_mult) {
        BB.left = scale(BB.left);
        BB.bottom = scale(BB.bottom);
        BB.right = scale(BB.right);
        BB.top = scale(BB.top);
    }

    bool ret;
    if (!ydir) {
        if (in_action == cvOpenModeTrans)
            ret = a_box_cvt(BB);
        else
            ret = a_box_db(BB);
    }
    else {
        // Convert rotated box to poly.  Only necessary for non-native
        // input.
        Point *points = box_to_poly(&BB, xdir, ydir);
        if (!points)
            return (false);
        Poly po(5, points);
        if (in_action == cvOpenModeTrans)
            ret = a_polygon_cvt(po);
        else
            ret = a_polygon_db(po);
        delete [] po.points;
    }
    return (ret);
}


// Process box, for cvOpenModeDb.
//
bool
cif_in::a_box_db(BBox &BB)
{
    if (in_action == cvOpenModeDb && in_in_root) {
        clear_properties();
        return (true);
    }
    if (in_check_intersect) {
        if (!BB.intersect(&in_cBB, false))
            in_skip = true;
    }
    else if (in_savebb || (in_sdesc && !in_skip)) {
        if (in_savebb)
            in_cBB.add(&BB);
        else if (!in_areafilt || BB.intersect(&in_cBB, false)) {
            CDo *newo;
            CDerrType err = in_sdesc->makeBox(in_curlayer, &BB, &newo);
            if (err != CDok) {
                clear_properties();
                if (err == CDbadBox) {
                    warning("bad box (ignored)", false);
                    return (true);
                }
                return (error(PERRCD, 0));
            }
            if (newo) {
                add_properties_db(in_sdesc, newo);
                if (FIO()->IsMergeInput() && in_mode == Physical) {
                    if (!in_sdesc->mergeBox(newo, false)) {
                        clear_properties();
                        return (error(PERRCD, 0));
                    }
                }
            }
        }
    }
    clear_properties();
    return (true);
}


// Process box, for cvOpenModeTrans.
//
bool
cif_in::a_box_cvt(BBox &BB)
{
    if (in_skip) {
        in_out->clear_property_queue();
        return (true);
    }
    bool ret = true;

    if (in_tf_list) {
        ts_reader tsr(in_tf_list);
        CDtx ttx;
        CDap ap;
        BBox BBbak = BB;

        bool wrote_once = false;
        while (tsr.read_record(&ttx, &ap)) {
            ttx.scale(in_ext_phys_scale);
            ap.scale(in_ext_phys_scale);
            TPush();
            TApply(ttx.tx, ttx.ty, ttx.ax, ttx.ay, ttx.magn, ttx.refly);

            if (in_out->size_test(in_chd_state.symref(), this)) {
                TPop();
                continue;
            }
            in_transform++;

            if (ap.nx > 1 || ap.ny > 1) {
                int tx, ty;
                TGetTrans(&tx, &ty);
                xyg_t xyg(0, ap.nx-1, 0, ap.ny-1);
                do {
                    TTransMult(xyg.x*ap.dx, xyg.y*ap.dy);
                    if (wrote_once)
                        BB = BBbak;
                    ret = a_box_cvt_prv(BB);
                    wrote_once = true;
                    if (!ret)
                        break;
                    TSetTrans(tx, ty);
                } while (xyg.advance());
            }
            else {
                if (wrote_once)
                    BB = BBbak;
                ret = a_box_cvt_prv(BB);
                wrote_once = true;
            }

            in_transform--;
            TPop();
            if (!ret)
                break;
        }
    }
    else
        ret = a_box_cvt_prv(BB);

    in_out->clear_property_queue();
    return (ret);
}


bool
cif_in::a_box_cvt_prv(BBox &BB)
{
    bool ret = true;
    if (in_transform) {
        Point *pp;
        TBB(&BB, &pp);
        if (pp) {
            Poly po(5, pp);
            if (!in_areafilt || po.intersect(&in_cBB, false)) {
                TPush();
                bool need_out = true;
                if (in_areafilt && in_clip) {
                    need_out = false;
                    PolyList *pl = po.clip(&in_cBB, &need_out);
                    for (PolyList *p = pl; p; p = p->next) {
                        ret = in_out->write_poly(&p->po);
                        if (!ret)
                            break;
                    }
                    PolyList::destroy(pl);
                }
                if (need_out)
                    ret = in_out->write_poly(&po);
                TPop();
            }
            delete [] pp;
            return (ret);
        }
    }
    if (!in_areafilt || BB.intersect(&in_cBB, false)) {
        if (in_areafilt && in_clip) {
            if (BB.left < in_cBB.left)
                BB.left = in_cBB.left;
            if (BB.bottom < in_cBB.bottom)
                BB.bottom = in_cBB.bottom;
            if (BB.right > in_cBB.right)
                BB.right = in_cBB.right;
            if (BB.top > in_cBB.top)
                BB.top = in_cBB.top;
        }
        TPush();
        ret = in_out->write_box(&BB);
        TPop();
    }
    return (ret);
}


bool
cif_in::a_round_flash()
{
    int width;
    if (!read_integer(&width))
        return (false);
    int x, y;
    if (!read_point(&x, &y))
        return (false);
    int c;
    if (!look_for_semi(&c))
        return (false);
    if (c != ';')
        return (error(PERRNOSEMI, 0));

    // Not supported, convert to box
    BBox BB;
    BB.set_cif(x, y, width, width);
    if (in_needs_mult) {
        BB.left = scale(BB.left);
        BB.bottom = scale(BB.bottom);
        BB.right = scale(BB.right);
        BB.top = scale(BB.top);
    }

    bool ret;
    if (in_action == cvOpenModeTrans)
        ret = a_box_cvt(BB);
    else
        ret = a_box_db(BB);
    return (ret);
}


// Read the polygon lines and dispatch appropriately.
//
bool
cif_in::a_polygon()
{
    int *ary, numpts;
    if (!read_array(&ary, &numpts)) {
        clear_properties();
        return (false);
    }
    if (numpts & 1) {
        delete [] ary;
        clear_properties();
        return (error(PERRETC, "Bad X,Y path element."));
    }
    numpts /= 2;
    Point *points = new Point[numpts];
    for (int i = 0, j = 0; i < numpts; i++) {
        points[i].x = ary[j++];
        points[i].y = ary[j++];
    }
    delete [] ary;

    // make sure that the path is closed
    if (points[0].x != points[numpts-1].x ||
            points[0].y != points[numpts-1].y) {
        Point *p = new Point[numpts+1];
        int i;
        for (i = 0; i < numpts; i++)
            p[i] = points[i];
        p[i] = p[0];
        numpts++;
        delete [] points;
        points = p;
    }

    if (in_needs_mult) {
        for (int i = 0; i < numpts; i++)
            points[i].set(scale(points[i].x), scale(points[i].y));
    }

    Poly po(numpts, points);
    bool ret;
    if (in_action == cvOpenModeTrans)
        ret = a_polygon_cvt(po);
    else
        ret = a_polygon_db(po);
    delete [] po.points;
    return (ret);
}


// Process a polygon, for cvOpenModeDb.  The po.points might be
// returned zeroed!
//
bool
cif_in::a_polygon_db(Poly &po)
{
    if (in_action == cvOpenModeDb && in_in_root) {
        clear_properties();
        return (true);
    }
    if (in_check_intersect) {
        if (!po.intersect(&in_cBB, false))
            in_skip = true;
    }
    else if (in_savebb || (in_sdesc && !in_skip)) {
        if (po.is_rect()) {
            BBox BB(po.points);

            if (in_savebb)
                in_cBB.add(&BB);
            else if (!in_areafilt || BB.intersect(&in_cBB, false)) {
                CDo *newo;
                CDerrType err = in_sdesc->makeBox(in_curlayer, &BB, &newo);
                if (err != CDok) {
                    clear_properties();
                    if (err == CDbadBox) {
                        warning("bad polygon-box (ignored)", false);
                        return (true);
                    }
                    return (error(PERRCD, 0));
                }
                if (newo) {
                    add_properties_db(in_sdesc, newo);
                    if (FIO()->IsMergeInput() && in_mode == Physical) {
                        if (!in_sdesc->mergeBox(newo, false)) {
                            clear_properties();
                            return (error(PERRCD, 0));
                        }
                    }
                }
            }
        }
        else if (in_savebb) {
            BBox BB;
            po.computeBB(&BB);
            in_cBB.add(&BB);
        }
        else if (!in_areafilt || po.intersect(&in_cBB, false)) {
            CDpo *newo;
            int pchk_flags;
            // This will zero po.points!
            CDerrType err =
                in_sdesc->makePolygon(in_curlayer, &po, &newo, &pchk_flags);
            if (err != CDok) {
                clear_properties();
                if (err == CDbadPolygon) {
                    warning("bad polygon (ignored)", false);
                    return (true);
                }
                return (error(PERRCD, 0));
            }
            if (newo) {
                add_properties_db(in_sdesc, newo);

                if (pchk_flags & PCHK_REENT)
                    warning("reentrant or badly formed polygon", false);
                if (pchk_flags & PCHK_ZERANG) {
                    if (pchk_flags & PCHK_FIXED)
                        warning("repaired polygon with zero degree angle",
                            false);
                    else
                        warning("polygon with zero degree angle", false);
                }
            }
        }
    }
    clear_properties();
    return (true);
}


// Process a polygon, for cvOpenModeTrans.
//
bool
cif_in::a_polygon_cvt(Poly &po)
{
    if (in_skip) {
        in_out->clear_property_queue();
        return (true);
    }

    bool ret = true;
    if (in_tf_list) {
        ts_reader tsr(in_tf_list);
        CDtx ttx;
        CDap ap;
        Point *scratch = new Point[po.numpts];
        memcpy(scratch, po.points, po.numpts*sizeof(Point));

        bool wrote_once = false;
        while (tsr.read_record(&ttx, &ap)) {
            ttx.scale(in_ext_phys_scale);
            ap.scale(in_ext_phys_scale);
            TPush();
            TApply(ttx.tx, ttx.ty, ttx.ax, ttx.ay, ttx.magn, ttx.refly);

            if (in_out->size_test(in_chd_state.symref(), this)) {
                TPop();
                continue;
            }
            in_transform++;

            if (ap.nx > 1 || ap.ny > 1) {
                int tx, ty;
                TGetTrans(&tx, &ty);
                xyg_t xyg(0, ap.nx-1, 0, ap.ny-1);
                do {
                    TTransMult(xyg.x*ap.dx, xyg.y*ap.dy);
                    if (wrote_once)
                        memcpy(po.points, scratch, po.numpts*sizeof(Point));
                    ret = a_polygon_cvt_prv(po);
                    wrote_once = true;
                    if (!ret)
                        break;
                    TSetTrans(tx, ty);
                } while (xyg.advance());
            }
            else {
                if (wrote_once)
                    memcpy(po.points, scratch, po.numpts*sizeof(Point));
                ret = a_polygon_cvt_prv(po);
                wrote_once = true;
            }

            in_transform--;
            TPop();
            if (!ret)
                break;
        }
        delete [] scratch;
    }
    else
        ret = a_polygon_cvt_prv(po);

    in_out->clear_property_queue();
    return (ret);
}


bool
cif_in::a_polygon_cvt_prv(Poly &po)
{
    bool ret = true;
    if (in_transform)
        TPath(po.numpts, po.points);
    if (po.is_rect()) {
        BBox BB(po.points);

        if (!in_areafilt || BB.intersect(&in_cBB, false)) {
            if (in_areafilt && in_clip) {
                if (BB.left < in_cBB.left)
                    BB.left = in_cBB.left;
                if (BB.bottom < in_cBB.bottom)
                    BB.bottom = in_cBB.bottom;
                if (BB.right > in_cBB.right)
                    BB.right = in_cBB.right;
                if (BB.top > in_cBB.top)
                    BB.top = in_cBB.top;
            }
            TPush();
            ret = in_out->write_box(&BB);
            TPop();
        }
    }
    else {
        if (!in_areafilt || po.intersect(&in_cBB, false)) {
            TPush();
            bool need_out = true;
            if (in_areafilt && in_clip) {
                need_out = false;
                PolyList *pl = po.clip(&in_cBB, &need_out);
                for (PolyList *p = pl; p; p = p->next) {
                    ret = in_out->write_poly(&p->po);
                    if (!ret)
                        break;
                }
                PolyList::destroy(pl);
            }
            if (need_out)
                ret = in_out->write_poly(&po);
            TPop();
        }
    }
    return (ret);
}


bool
cif_in::a_wire()
{
    // Have to deal with an extension here.  In our variation, the W
    // is immediately followed by the end style (0, 1, or 2), which is
    // followed by the path width and data points.  In standard CIF,
    // there is no style integer.  We read all of the integers up to
    // the ';' and can tell by the count whether a style was given.

    int *ary, numpts;
    if (!read_array(&ary, &numpts)) {
        clear_properties();
        return (false);
    }
    int j, width, style = -1;
    if (numpts & 1) {
        // style not given
        width = ary[0];
        numpts = (numpts-1)/2;
        j = 1;
    }
    else {
        if (ary[0] >= 0 && ary[0] <= 2)
            style = ary[0];
        else {
            delete [] ary;
            clear_properties();
            return (error(PERRETC, "Bad X,Y path element."));
        }
        width = ary[1];
        numpts = (numpts - 2)/2;
        j = 2;
    }
    Point *points = new Point[numpts];
    for (int i = 0; i < numpts; i++) {
        points[i].x = ary[j++];
        points[i].y = ary[j++];
    }
    delete [] ary;

    if (in_needs_mult) {
        width = scale(width);
        for (int i = 0; i < numpts; i++)
            points[i].set(scale(points[i].x), scale(points[i].y));
    }

    Wire w(width, style >= 0 ? (WireStyle)style : CDWIRE_EXTEND, numpts,
        points);

    bool ret;
    if (in_action == cvOpenModeTrans)
        ret = a_wire_cvt(w, (style >= 0));
    else
        ret = a_wire_db(w, (style >= 0));
    delete [] w.points;
    return (ret);
}


namespace {
    // This function looks for the pathtype properties and removes them.
    // These are no longer used, so this is for backward compatibility.
    // The return value is the last pathtype found (there should be only
    // one).  This does not reset the wire style.
    //
    int
    prptySetStyle(CDp **plst)
    {
        int ptype = -1;
        CDp *pp = 0, *pn;
        for (CDp *pd = *plst; pd; pd = pn) {
            pn = pd->next_prp();
            if (pd->value() == OLD_PATHTYPE_PROP &&
                    lstring::ciprefix("PATHTYPE", pd->string())) {
                if (!pp)
                    *plst = pn;
                else
                    pp->set_next_prp(pn);
                int x;
                if (sscanf(pd->string() + 9, "%d", &x) == 1 &&
                        x >= (int)CDWIRE_FLUSH && x <= (int)CDWIRE_EXTEND)
                    ptype = x;
                delete pd;
                continue;
            }
            pp = pd;
        }
        return (ptype);
    }
}


// Process a wire, for cvOpenModeDb.
//
bool
cif_in::a_wire_db(Wire &w, bool style_given)
{
    if (in_action == cvOpenModeDb && in_in_root) {
        clear_properties();
        return (true);
    }
    if (in_check_intersect) {
        if (!w.intersect(&in_cBB, false))
            in_skip = true;
    }
    else if (in_savebb || (in_sdesc && !in_skip)) {
        if (in_savebb) {
            BBox BB;
            // warning: if dup verts, computeBB may fail
            Point::removeDups(w.points, &w.numpts);
            w.computeBB(&BB);
            in_cBB.add(&BB);
        }
        else if (!in_areafilt || w.intersect(&in_cBB, false)) {
            // Look for a pathtype property.  This is for backward
            // compatibility.
            int pt = prptySetStyle(&in_prpty_list);
            if (!style_given && pt >= 0)
                w.set_wire_style((WireStyle)pt);

            Point *pres = 0;
            int nres = 0;
            for (;;) {
                if (in_mode == Physical)
                    w.checkWireVerts(&pres, &nres);

                CDw *newo;
                // This will zero w.points!
                int wchk_flags;
                CDerrType err = in_sdesc->makeWire(in_curlayer, &w, &newo,
                    &wchk_flags);
                if (err != CDok) {
                    if (err == CDbadWire)
                        warning("bad wire (ignored)", false);
                    else {
                        delete [] pres;
                        clear_properties();
                        return (error(PERRCD, 0));
                    }
                }
                else if (newo) {
                    add_properties_db(in_sdesc, newo);

                    if (wchk_flags) {
                        char buf[256];
                        Wire::flagWarnings(buf, wchk_flags,
                            "questionable wire found, warnings: ");
                        warning(buf, false);
                    }
                }
                if (!pres)
                    break;
                warning("breaking colinear reentrant wire", false);
                w.points = pres;
                w.numpts = nres;
            }
        }
    }
    clear_properties();
    return (true);
}


// Process a wire, for cvOpenModeTrans.
//
bool
cif_in::a_wire_cvt(Wire &w, bool style_given)
{
    if (in_skip) {
        in_out->clear_property_queue();
        return (true);
    }

    // Look for a pathtype property.  This is for backward
    // compatibility.
    CDp *list = in_out->set_property_queue(0);
    int pt = prptySetStyle(&list);
    in_out->set_property_queue(list);
    if (!style_given && pt >= 0)
        w.set_wire_style((WireStyle)pt);

    bool ret = true;
    int pwidth = w.wire_width();
    if (in_tf_list) {
        ts_reader tsr(in_tf_list);
        CDtx ttx;
        CDap ap;
        Point *scratch = new Point[w.numpts];
        memcpy(scratch, w.points, w.numpts*sizeof(Point));

        bool wrote_once = false;
        while (tsr.read_record(&ttx, &ap)) {
            ttx.scale(in_ext_phys_scale);
            ap.scale(in_ext_phys_scale);
            TPush();
            TApply(ttx.tx, ttx.ty, ttx.ax, ttx.ay, ttx.magn, ttx.refly);

            if (in_out->size_test(in_chd_state.symref(), this)) {
                TPop();
                continue;
            }
            in_transform++;

            if (ap.nx > 1 || ap.ny > 1) {
                int tx, ty;
                TGetTrans(&tx, &ty);
                xyg_t xyg(0, ap.nx-1, 0, ap.ny-1);
                do {
                    TTransMult(xyg.x*ap.dx, xyg.y*ap.dy);
                    if (wrote_once) {
                        memcpy(w.points, scratch, w.numpts*sizeof(Point));
                        w.set_wire_width(pwidth);
                    }
                    ret = a_wire_cvt_prv(w);
                    wrote_once = true;
                    if (!ret)
                        break;
                    TSetTrans(tx, ty);
                } while (xyg.advance());
            }
            else {
                if (wrote_once) {
                    memcpy(w.points, scratch, w.numpts*sizeof(Point));
                    w.set_wire_width(pwidth);
                }
                ret = a_wire_cvt_prv(w);
                wrote_once = true;
            }

            in_transform--;
            TPop();
            if (!ret)
                break;
        }
        delete [] scratch;
    }
    else
        ret = a_wire_cvt_prv(w);

    in_out->clear_property_queue();
    return (ret);
}


bool
cif_in::a_wire_cvt_prv(Wire &w)
{
    if (in_transform) {
        TPath(w.numpts, w.points);
        w.set_wire_width(mmRnd(w.wire_width() * TGetMagn()));
    }

    bool ret = true;
    Poly wp;
    if (!in_areafilt || (w.toPoly(&wp.points, &wp.numpts) &&
            wp.intersect(&in_cBB, false))) {
        TPush();
        bool need_out = true;
        if (in_areafilt && in_clip) {
            need_out = false;
            PolyList *pl = wp.clip(&in_cBB, &need_out);
            for (PolyList *p = pl; p; p = p->next) {
                ret = in_out->write_poly(&p->po);
                if (!ret)
                    break;
            }
            PolyList::destroy(pl);
        }
        if (need_out)
            ret = in_out->write_wire(&w);
        TPop();
    }
    delete [] wp.points;
    return (ret);
}


// Define this to accept the traditional 4-character layer names only.
//
//#define TRADITIONAL_LNAMES

bool
cif_in::a_layer()
{
#ifdef TRADITIONAL_LNAMES
    char lname[8];
    int c;
    bool abrt;
    if (!read_character(&c, &abrt, PSTRIP1, PFAILONEOF))
        return (false);
    if (abrt)
        return (false);
    if (c == ';')
        return (error(PERRETC,
            "Character expected after L in layer command."));
    lname[0] = c;
    lname[1] = 0;
    lname[2] = 0;
    lname[3] = 0;
    lname[4] = 0;
    lname[5] = 0;
    lname[6] = 0;
    lname[7] = 0;
    bool found_semi = false;
    for (int i = 1; i < 4; i++) {
        if ((c = getc(in_fp)) == EOF)
            return (error(PERREOF, 0));
        if (c == ';') {
            found_semi = true;
            break;
        }
        if (isspace(c))
            break;
        // check for valid CIF layer name character
        if (!isalnum(c))
            return (error(PERRETC,
                "Non-alphanumeric character in layer name encountered."));
        lname[i] = c;
    }
    if (!found_semi) {
        // clear the semicolon
        if (!read_character(&c, &abrt, PSTRIP1, PFAILONEOF))
            return (false);
        if (abrt)
            return (false);
    }
    if (c != ';')
        return (error(PERRETC,
            "Illegal CIF layer name with > 4 characters encountered."));
#else
    // Traditional CIF allowed only up to four characters in layer names,
    // in the format "L[ ]xxxx;".  Here we allow an extension in the form
    // L[ ]xxx...;
    // Leading white space is stripped and ignored.  The name can be
    // arbitrarily long.  We also allow any characters, except ';',
    // and leading white space.  A leading comma will be treated as
    // white space.  Xic will change the name if necessary.

    sLstr lstr;
    int c;
    bool abrt;
    if (!read_character(&c, &abrt, PSTRIP1, PFAILONEOF))
        return (false);
    if (abrt)
        return (false);
    if (c == ';')
        return (error(PERRETC,
            "Character expected after L in layer command."));
    lstr.add_c(c);
    for (;;) {
        c = getc(in_fp);
        if (c == EOF)
            return (error(PERREOF, 0));
        if (c == ';')
            break;
        lstr.add_c(c);
    }
    const char *lname = lstr.string();
#endif

    in_skip = 0;
    if (in_mode == Physical) {
        if (!in_listonly && in_lcheck &&
                !in_lcheck->layer_ok(lname, -1, -1)) {
            in_skip = true;
            return (true);
        }
        if (in_layer_alias) {
            const char *new_lname = in_layer_alias->alias(lname);
#ifdef TRADITIONAL_LNAMES
            if (new_lname && strlen(new_lname) <= 4) {
                strncpy(lname, new_lname, 4);
                lname[4] = 0;
            }
#else
            if (new_lname)
                lname = new_lname;
#endif
        }
    }
    if (in_listonly) {
        if (info())
            info()->add_layer(lname);
        return (true);
    }
    if ((in_action != cvOpenModeTrans && !in_sdesc) ||
            (in_action == cvOpenModeDb && in_in_root))
        return (true);

    if (in_action == cvOpenModeTrans) {
        // If using layer index translation, output the Xic layer names
        if (FIO()->CifStyle().layer_type() == EXTlayerNCA) {
            // The layer name is a *1-based* index into the existing layer
            // table, without a leading '0'.
            int i;
            for (i = 0; lname[i]; i++) {
                if (!isdigit(lname[i]))
                    break;
            }
            if (!lname[i] && lname[0] != '0') {
                int lnum = atoi(lname);
                CDl *ld = CDldb()->layer(lnum, in_mode);
                if (ld) {
                    Layer layer(ld->name(), -1, -1);
                    if (!in_out->queue_layer(&layer))
                        return (false);
                    strcpy(in_curlayer_name, ld->name());
                    return (true);
                }
            }
            if (FIO()->CifStyle().lread_type() == EXTlreadIndex)
                return (error(PERRETC, "Bad index after L in layer command."));
        }
        Layer layer(lname, -1, -1);
        if (!in_out->queue_layer(&layer))
            return (false);
        strcpy(in_curlayer_name, lname);
        return (true);
    }

    if (FIO()->CifStyle().lread_type() == EXTlreadDef ||
            FIO()->CifStyle().lread_type() == EXTlreadName) {
        CDl *ld = CDldb()->findLayer(lname, in_mode);
        if (ld) {
            in_curlayer = ld;
            in_layertype_set = true;
            return (true);
        }
    }

    if (FIO()->CifStyle().lread_type() == EXTlreadDef ||
            FIO()->CifStyle().lread_type() == EXTlreadIndex) {
        // The layer name is a *1-based* index into the existing layer
        // table, without a leading '0'.
        int i;
        for (i = 0; lname[i]; i++) {
            if (!isdigit(lname[i]))
                break;
        }
        if (!lname[i] && lname[0] != '0') {
            int lnum = atoi(lname);
            CDl *ld = CDldb()->layer(lnum, in_mode);
            if (ld) {
                in_curlayer = ld;
                if (in_update_style && !in_layertype_set)
                    FIO()->CifStyle().set_layer_type(EXTlayerNCA);
                in_layertype_set = true;
                return (true);
            }
        }
        if (FIO()->CifStyle().lread_type() == EXTlreadIndex)
            return (error(PERRETC, "Bad index after L in layer command."));
    }

    if (!in_no_create_layer) {
        CDl *ld = 0;
        if (FIO()->IsNoCreateLayer()) {
            ld = CDldb()->findLayer(lname, in_mode);
            if (!ld) {
                Errs()->add_error(
                    "unresolved layer, not allowed to create new layer %s.",
                    lname);
                return (false);
            }
        }
        else {
            ld = CDldb()->newLayer(lname, in_mode);
            if (!ld) {
                Errs()->add_error("could not create new layer %s.", lname);
                return (false);
            }
        }
        in_curlayer = ld;
        in_layertype_set = true;
    }
    else {
        in_curlayer = 0;
        in_layertype_set = false;
    }
    return (true);
}


bool
cif_in::a_extension(int digit)
{
    if (!in_cell_offset)
        in_cell_offset = in_offset;

    // New 10/29/12, keep the semicolon verbatim if preceded by a
    // backslash character.  The backslash is stripped.
    sLstr lstr;
    int lastc = 0;
    for (;;) {
        int c = 0;
        bool abrt;
        if (!read_character(&c, &abrt, PLEAVESPACE, PFAILONEOF))
            return (false);
        if (abrt)
            return (false);
        if (c == ';') {
            if (lastc == '\\')
                lstr.rem_c();
            else
                break;
        }
        lstr.add_c(c);
        lastc = c;
    }

    char *text = lstr.string_clear();
    GCarray<char*> gc_text(text);
    char tbuf[256];

    if (digit == '9') {
        if (text[0] == '4' || text[0] == '2') {
            // label extension
            if (in_skip || in_ignore_text ||
                    (in_action != cvOpenModeTrans && !in_sdesc && !in_savebb) ||
                    (in_action == cvOpenModeDb && in_in_root)) {
                clear_properties();
                return (true);
            }
            return (a_label(text[0], text+1));
        }
        if (isspace(*text) && in_cif_type != CFnone) {
            // symbol name
            char *t = text;
            while (isspace(*t)) t++;
            strcpy(in_cellname, t);
            return (true);
        }
    }
    else if (digit == '1' && !isdigit(*text)) {
        // Array, Bound and Magnify extensions
        if (in_ignore_inst || (in_action == cvOpenModeDb && in_in_root))
            return (true);
        const char *tt = text;
        char *tok = lstring::gettok(&tt);
        GCarray<char*> gc_tok(tok);
        if (!strcmp(tok, "Array")) {
            int nx, dx, ny, dy;
            if (sscanf(text, "%s%d%d%d%d", tbuf, &nx, &dx, &ny, &dy) < 5) {
                Errs()->add_error("Bad Array extension syntax.");
                return (error(PERRCD, 0));
            }
            in_calldesc.nx = nx;
            in_calldesc.dx = scale(dx);
            in_calldesc.ny = ny;
            in_calldesc.dy = scale(dy);
        }
        else if (!strcmp(tok, "Bound")) {
            int l, b, r, t;
            if (sscanf(text, "%s%d%d%d%d", tbuf, &l, &b, &r, &t) < 5) {
                Errs()->add_error("Bad Bound extension syntax.");
                return (error(PERRCD, 0));
            }
            // nothing to do yet
        }
        else if (!strcmp(tok, "Magnify")) {
            double d;
            if (sscanf(text, "%s%lf", tbuf, &d) < 2 || d <= 0) {
                Errs()->add_error("Bad Magnify extension syntax.");
                return (error(PERRCD, 0));
            }
            in_calldesc.magn = d;
        }
    }
    else if (digit == '5' && !isdigit(*text)) {
        // Reserved for CD Property List extensions
        char *t = text;
        // skip white space before property integer
        while (isspace(*t))
            t++;
        int value;
        if (sscanf(t, "%d", &value) < 1) {
            warning("bad property format (ignored)", false);
            return (true);
        }
        // skip property integer
        while (*t && !isspace(*t)) t++;
        // skip white space and control chars after property integer
        while (isspace(*t))
            t++;
        const char *string = *t ? t : " ";

        if (in_action == cvOpenModeTrans) {
            CDp *px = new CDp(string, value);
            px->scale(in_scale, in_phys_scale, in_mode);
            bool ret = in_out->queue_property(px->value(), px->string());
            delete px;
            if (!ret)
                return (false);
        }
        else {
            CDp *pdesc = new CDp(string, value);
            pdesc->set_next_prp(in_prpty_list);
            in_prpty_list = pdesc;
        }
    }
    return (true);
}


namespace { const char *comma = ","; }

bool
cif_in::a_label(char type, char *text)
{
    if (!text)
        return (true);

#ifdef WIN32
    // Get rid of '\r' characters, these can cause trouble.
    char *s1 = text;
    char *s2 = text;
    for (;;) {
        if (!*s1) {
            if (s2 != s1)
                *s2 = 0;
            break;
        }
        if (*s1 != '\r') {
            if (s2 != s1)
                *s2++ = *s1;
        }
        s1++;
    }
#endif

    sLstr lstr;
    Text otext;
    const char *texttmp = text;
    CD()->GetLabel(&texttmp, &lstr);
    text += (texttmp - text);

    // Fix hypertext references
    if (in_mode == Electrical && in_needs_mult) {
        char *t = lstr.string_clear();
        t = hyList::hy_scale(t, in_scale);
        lstr.add(t);
        delete [] t;
    }

    int ncnt = 0;
    char *tokens[5];
    char *tok;
    while ((tok = lstring::gettok(&text, comma)) != 0) {
        tokens[ncnt++] = tok;
        if (ncnt == 5)
            break;
    }

    Label label;
    int x, y;
    if (ncnt < 2 || sscanf(tokens[0], "%d", &x) != 1 ||
            sscanf(tokens[1], "%d", &y) != 1) {
        while (ncnt--)
            delete [] tokens[ncnt];
        CD()->Error(CDbadLabel, "");
        return (error(PERRCD, 0));
    }
    if (in_needs_mult) {
        x = scale(x);
        y = scale(y);
    }
    if (in_action == cvOpenModeTrans) {
        otext.text = lstr.string();
        otext.x = x;
        otext.y = y;
    }
    else {
        label.label = new hyList(in_sdesc, lstr.string(), HYcvAscii);
        label.x = x;
        label.y = y;
    }

    bool retval = true;
    if (ncnt == 2) {
        // simple label
        if (in_action == cvOpenModeTrans) {
            if (!write_label(&otext))
                return (false);
        }
        else {
            double tw, th;
            char *t = hyList::string(label.label, HYcvPlain, false);
            CD()->DefaultLabelSize(t, in_mode, &tw, &th);
            delete [] t;
            if (in_mode == Physical) {
                label.width = INTERNAL_UNITS(tw*in_ext_phys_scale);
                label.height = INTERNAL_UNITS(th*in_ext_phys_scale);
            }
            else {
                label.width = ELEC_INTERNAL_UNITS(tw);
                label.height = ELEC_INTERNAL_UNITS(th);
            }
            if (in_savebb) {
                BBox BB;
                label.computeBB(&BB);
                in_cBB.add(&BB);
                hyList::destroy(label.label);
            }
            else
                retval = create_label(label);
            if (in_update_style && !in_labeltype_set)
                FIO()->CifStyle().set_label_type(EXTlabelKIC);
        }
        while (ncnt--)
            delete [] tokens[ncnt];
        in_labeltype_set = true;
        return (retval);
    }
    if (ncnt == 3) {
        if (type == '2') {
            // NCA label, arg is layer index.
            CDl *ldp = 0;
            if (!in_listonly) {
                int ix = atoi(tokens[2]);
                if (ix <= 0) {
                    // index is 1-based
                    while (ncnt--)
                        delete [] tokens[ncnt];
                    CD()->Error(CDbadLabel, "");
                    return (error(PERRCD, 0));
                }
                ldp = CDldb()->layer(ix, in_mode);
                if (!ldp) {
                    while (ncnt--)
                        delete [] tokens[ncnt];
                    CD()->Error(CDbadLabel, "");
                    return (error(PERRCD, 0));
                }
            }
            if (in_action == cvOpenModeTrans) {
                if (!ldp)
                    // can't happen
                    return (false);
                if (strcmp(ldp->name(), in_curlayer_name)) {
                    Layer layer(ldp->name(), -1, -1);
                    if (!in_out->queue_layer(&layer))
                        return (false);
                    strcpy(in_curlayer_name, ldp->name());
                }
                if (!write_label(&otext))
                    return (false);
            }
            else {
                double tw, th;
                char *t = hyList::string(label.label, HYcvPlain, false);
                CD()->DefaultLabelSize(t, in_mode, &tw, &th);
                delete [] t;
                if (in_mode == Physical) {
                    label.width = INTERNAL_UNITS(tw*in_ext_phys_scale);
                    label.height = INTERNAL_UNITS(th*in_ext_phys_scale);
                }
                else {
                    label.width = ELEC_INTERNAL_UNITS(tw);
                    label.height = ELEC_INTERNAL_UNITS(th);
                }
                if (in_savebb) {
                    BBox BB;
                    label.computeBB(&BB);
                    in_cBB.add(&BB);
                    hyList::destroy(label.label);
                }
                else {
                    if (!ldp)
                        // can't happen
                        return (false);
                    retval = create_label(label, ldp);
                }
                if (in_update_style && !in_labeltype_set)
                    FIO()->CifStyle().set_label_type(EXTlabelNCA);
            }
        }
        else {
            // mextra label, arg is layer name
            if (in_action == cvOpenModeTrans) {
                if (strcmp(tokens[2], in_curlayer_name)) {
                    Layer layer(tokens[2], -1, -1);
                    if (!in_out->queue_layer(&layer))
                        return (false);
                    strcpy(in_curlayer_name, tokens[2]);
                }
                if (!write_label(&otext))
                    return (false);
            }
            else {
                double tw, th;
                char *t = hyList::string(label.label, HYcvPlain, false);
                CD()->DefaultLabelSize(t, in_mode, &tw, &th);
                delete [] t;
                if (in_mode == Physical) {
                    label.width = INTERNAL_UNITS(tw*in_ext_phys_scale);
                    label.height = INTERNAL_UNITS(th*in_ext_phys_scale);
                }
                else {
                    label.width = ELEC_INTERNAL_UNITS(tw);
                    label.height = ELEC_INTERNAL_UNITS(th);
                }
                if (in_savebb) {
                    BBox BB;
                    label.computeBB(&BB);
                    in_cBB.add(&BB);
                    hyList::destroy(label.label);
                }
                else {
                    CDl *ld = 0;
                    if (FIO()->IsNoCreateLayer())
                        ld = CDldb()->findLayer(tokens[2], in_mode);
                    else
                        ld = CDldb()->newLayer(tokens[2], in_mode);
                    if (!ld) {
                        while (ncnt--)
                            delete [] tokens[ncnt];
                        CD()->Error(CDbadLabel, "");
                        return (error(PERRCD, 0));
                    }
                    retval = create_label(label, ld);
                }
                if (in_update_style && !in_labeltype_set)
                    FIO()->CifStyle().set_label_type(EXTlabelMEXTRA);
            }
        }
        while (ncnt--)
            delete [] tokens[ncnt];
        in_labeltype_set = true;
        return (retval);
    }
    int xform;
    if (sscanf(tokens[2], "%d", &xform) != 1) {
        while (ncnt--)
            delete [] tokens[ncnt];
        CD()->Error(CDbadLabel, "");
        return (error(PERRCD, 0));
    }
    int width;
    if (sscanf(tokens[3], "%d", &width) != 1) {
        while (ncnt--)
            delete [] tokens[ncnt];
        CD()->Error(CDbadLabel, "");
        return (error(PERRCD, 0));
    }
    width = scale(width);

    int height;
    if (ncnt == 5) {
        if (sscanf(tokens[4], "%d", &height) != 1) {
            while (ncnt--)
                delete [] tokens[ncnt];
            CD()->Error(CDbadLabel, "");
            return (error(PERRCD, 0));
        }
        height = scale(height);
    }
    else {
        // given width but not height (odd!)
        double tw, th;
        char *t = hyList::string(label.label, HYcvPlain, false);
        CD()->DefaultLabelSize(t, in_mode, &tw, &th);
        delete [] t;
        height = mmRnd(width*th/tw);
    }
    while (ncnt--)
        delete [] tokens[ncnt];

    if (in_action == cvOpenModeTrans) {
        otext.width = width;
        otext.height = height;
        otext.xform = xform;
        if (!write_label(&otext))
            return (false);
    }
    else {
        label.xform = xform;
        label.width = width;
        label.height = height;
        if (in_savebb) {
            BBox BB;
            label.computeBB(&BB);
            in_cBB.add(&BB);
            hyList::destroy(label.label);
        }
        else
            retval = create_label(label);
    }
    in_labeltype_set = true;
    return (retval);
}


bool
cif_in::a_comment()
{
    sLstr lstr;
    bool abrt;
    int c = 0, dep = 1;
    for (;;) {
        if (!read_character(&c, &abrt, PLEAVESPACE, PFAILONEOF))
            return (false);
        if (abrt)
            return (false);
        if (c == '(')
            dep++;
        else if (c == ')') {
            dep--;
            if (!dep)
                break;
        }
        lstr.add_c(c);
    }
    char *text = lstr.string_clear();
    GCarray<char*> gc_text(text);

    if (!in_found_data) {
        if (!in_found_phys && !in_found_elec) {
            if (!strcmp(text,"PHYSICAL"))
                in_found_phys = true;
            else if (!strcmp(text, "ELECTRICAL"))
                in_found_elec = true;
        }
        if (!in_found_resol && !strncmp(text, "RESOLUTION", 10)) {
            int res;
            if (sscanf(text+10, "%d", &res) == 1 &&
                    (res >= 100 && res <= 10000)) {
                if (in_mode == Physical) {
                    in_resol_scale =
                        dfix((in_ext_phys_scale*CDphysResolution)/res);
                    in_phys_res_found = res;
                }
                else {
                    in_resol_scale = dfix(CDelecResolution/res);
                    in_elec_res_found = res;
                }
                in_scale = in_resol_scale;
                in_needs_mult = (in_scale != 0.0);
                if (!in_native) {
                    // Don't repeatedly print this when reading
                    // native cells.
                    FIO()->ifPrintCvLog(IFLOG_INFO,
                        "Using supplied resolution %d.", res);
                }
                in_found_resol = true;
            }
            else
                warning("bad resolution", false);
        }
    }
    if (!in_cell_offset)
        in_cell_offset = in_offset;

    return (true);
}


// Find the symbol name, called when beginning to parse a symbol.  The
// symbol is returned in in_cellname, after any aliasing.  If no name
// is found, a name will be generated.
//
bool
cif_in::symbol_name(int sym_num)
{
    if (in_cif_type == CFnative)
        // in_cellname should already have the symbol name from the user
        // extension line that appears before the DS line in Xic symbols.
        return (true);

    *in_cellname = 0;
    if (in_cif_type != CFnone) {
        long posn = large_ftell(in_fp);

        int c;
        bool abrt;
        for (;;) {
            if (!read_character(&c, &abrt, PLEAVESPACE, PFAILONEOF))
                return (false);
            if (abrt)
                return (false);
            if (!isspace(c))
                break;
        }

        const char *msg = "symbol name extension syntax error";
        char buf[256];
        buf[0] = 0;
        if (c == '9') {
            if (!read_character(&c, &abrt, PLEAVESPACE, PFAILONEOF))
                return (false);
            if (abrt)
                return (false);
            if (!isspace(c)) {
                large_fseek(in_fp, posn, SEEK_SET);
                in_cif_type = CFnone;
            }
            else {
                int bcnt = 0;
                for (;;) {
                    if (!read_character(&c, &abrt, PLEAVESPACE, PFAILONEOF))
                        return (false);
                    if (abrt)
                        return (false);
                    if (c == ';') {
                        buf[bcnt] = 0;
                        break;
                    }
                    if (bcnt == 255)
                        return (error(PERRETC, msg));
                    buf[bcnt++] = c;
                }
            }
        }
        else if (c == '(') {
            int bcnt = 0;
            int end = -1;
            for (;;) {
                if (!read_character(&c, &abrt, PLEAVESPACE, PFAILONEOF))
                    return (false);
                if (abrt)
                    return (false);
                if (c == ')')
                    end = bcnt;
                else if (c == ';') {
                    if (end >= 0) {
                        buf[end] = 0;
                        break;
                    }
                }
                else if (!isspace(c))
                    end = -1;
                if (bcnt == 255)
                    return (error(PERRETC, msg));
                buf[bcnt++] = c;
            }
        }
        else {
            large_fseek(in_fp, posn, SEEK_SET);
            in_cif_type = CFnone;
        }

        if (buf[0]) {
            char *s = buf + strlen(buf) - 1;
            while (s >= buf && isspace(*s))
                *s-- = 0;
            s = buf;
            while (isspace(*s))
                s++;
            if (*s == '9' && isspace(*(s+1))) {
                s++;
                while (isspace(*s))
                    s++;
                if (in_update_style && !in_cnametype_set)
                    FIO()->CifStyle().set_cname_type(EXTcnameICARUS);
            }
            else if (lstring::ciprefix("name:", s)) {
                s += 5;
                while (isspace(*s))
                    s++;
                if (in_update_style && !in_cnametype_set)
                    FIO()->CifStyle().set_cname_type(EXTcnameSIF);
            }
            else if (in_update_style && !in_cnametype_set)
                FIO()->CifStyle().set_cname_type(EXTcnameNCA);
            in_cnametype_set = true;
            strcpy(in_cellname, lstring::strip_path(s));
        }
        else {
            if (in_update_style && !in_cnametype_set)
                FIO()->CifStyle().set_cname_type(EXTcnameNone);
            in_cnametype_set = true;
        }
    }

    if (!*in_cellname)
        sprintf(in_cellname, "%s%d", UNDEF_PREFIX, sym_num);

    return (true);
}


bool
cif_in::write_label(const Text *text)
{
    bool ret = true;
    if (in_tf_list) {
        ts_reader tsr(in_tf_list);
        CDtx ttx;
        CDap ap;
        while (tsr.read_record(&ttx, &ap)) {
            ttx.scale(in_ext_phys_scale);
            ap.scale(in_ext_phys_scale);
            TPush();
            TApply(ttx.tx, ttx.ty, ttx.ax, ttx.ay, ttx.magn, ttx.refly);

            if (in_out->size_test(in_chd_state.symref(), this)) {
                TPop();
                continue;
            }
            in_transform++;

            if (ap.nx > 1 || ap.ny > 1) {
                int tx, ty;
                TGetTrans(&tx, &ty);
                xyg_t xyg(0, ap.nx-1, 0, ap.ny-1);
                do {
                    TTransMult(xyg.x*ap.dx, xyg.y*ap.dy);
                    ret = write_label_prv(text);
                    if (!ret)
                        break;
                    TSetTrans(tx, ty);
                } while (xyg.advance());
            }
            else
                ret = write_label_prv(text);

            in_transform--;
            TPop();
            if (!ret)
                break;
        }
    }
    else
        ret = write_label_prv(text);

    in_out->clear_property_queue();
    return (ret);
}


bool
cif_in::write_label_prv(const Text *ctext)
{
    Text text(*ctext);
    if (in_transform)
        text.transform(this);

    if (in_areafilt) {
        BBox lBB(0, 0, text.width, text.height);
        TSetTransformFromXform(text.xform, text.width, text.height);
        TTranslate(text.x, text.y);
        Poly po(5, 0);
        TBB(&lBB, &po.points);
        TPop();

        bool ret = bound_isect(&in_cBB, &lBB, &po);
        delete [] po.points;
        if (!ret)
            return (true);
        if (in_clip && !in_keep_clip_text && !(in_cBB >= lBB))
            return (true);
    }
    return (in_out->write_text(&text));
}


bool
cif_in::create_label(Label &label, CDl *ld)
{
    if (!ld)
        ld = in_curlayer;
    CDla *newo;
    // This zeroes label.label!
    CDerrType err = in_sdesc->makeLabel(ld, &label, &newo);
    if (err != CDok) {
        clear_properties();
        if (err == CDbadLabel) {
            warning("bad label (ignored)", false);
            return (true);
        }
        return (error(PERRCD, 0));
    }
    if (newo)
        add_properties_db(in_sdesc, newo);
    clear_properties();
    return (true);
}


// Add property list information.
//
void
cif_in::add_properties_db(CDs *sdesc, CDo *odesc)
{
    if (sdesc) {
        stringlist *s0 = sdesc->prptyApplyList(odesc, &in_prpty_list);
        for (stringlist *s = s0; s; s = s->next)
            warning(s->string, true);
        stringlist::destroy(s0);

        if (!odesc) {
            sdesc->setPCellFlags();
            if (sdesc->isPCellSubMaster())
                sdesc->setPCellReadFromFile(true);
        }
    }
}


void
cif_in::clear_properties()
{
    CDp::destroy(in_prpty_list);
    in_prpty_list = 0;
}


const char *
cif_in::cur_layer_name()
{
    if (in_action == cvOpenModeTrans)
        return (in_curlayer_name);
    if (in_action == cvOpenModeDb && in_curlayer)
        return (in_curlayer->name());
    return ("0000");
}


bool
cif_in::error(int id, const char *msg)
{
    switch (id) {
    case PERREOF:
        Errs()->add_error("Premature end of file.");
        return (false);
    case PERRNOSEMI:
        Errs()->add_error("; expected and not found.");
        break;
    case PERRLAYER:
        Errs()->add_error("Undefined Layer %s.", msg);
        break;
    case PERRCD:
        break;
    default:
    case PERRETC:
        Errs()->add_error(msg);
    }
    char buf[256];
    add_bad(buf);
    if (islower(*buf))
        *buf = toupper(*buf);
    Errs()->add_error("%s", buf);
    return (false);
}


void
cif_in::warning(const char *str, bool prop)
{
    char buf[256];
    if (str)
        strcpy(buf, str);
    else
        *buf = 0;

    if (prop && in_prpty_list) {
        sprintf(buf + strlen(buf),
            "\n**  \"%s\"\n**  at or before line %d (ignored).",
            in_prpty_list->string(), in_line_cnt);
    }
    else if (in_fp) {
        char *s = buf + strlen(buf);
        if (*(s-1) != '\n') {
            *s++ = '\n';
            *s = 0;
        }
        add_bad(s);
    }
    FIO()->ifPrintCvLog(IFLOG_WARN, "%s", buf);
}


#define BACK_CHARS 30

void
cif_in::add_bad(char *str)
{
    char buf[BACK_CHARS+2];
    if (in_fp) {
        long pos = large_ftell(in_fp);
        long j = pos - BACK_CHARS;
        if (j < 0)
            j = 0;
        large_fseek(in_fp, j, SEEK_SET);
        int i;
        for (i = 0; j < pos; i++, j++) {
            int c = getc(in_fp);
            if (c == EOF)
                break;
            buf[i] = c;
        }
        buf[i] = '\0';
        char *s = buf;
        for (i -= 2; i > 0; i--) {
            if (buf[i] == '\n') {
                s = buf + i + 1;
                break;
            }
        }
        sprintf(str, "on line %d near \" %s \"", in_line_cnt+1, s);
    }
}


bool
cif_in::read_array(int **array, int *numpts)
{
    int *a = new int[32];
    int asize = 32;

    int i;
    for (i = 0; ; i++) {
        int c;
        if (!look_for_semi(&c)) {
            delete [] a;
            return (false);
        }
        if (c == ';')
            break;
        int x;
        if (!read_integer(&x)) {
            delete [] a;
            return (false);
        }
        if (i == asize) {
            int *t = new int[2*asize];
            memcpy(t, a, asize*sizeof(int));
            delete [] a;
            a = t;
            asize *= 2;
        }
        a[i] = x;
    }
    *array = a;
    *numpts = i;
    return (true);
}


bool
cif_in::read_point(int *x, int *y)
{
    // it is assumed that a LookAhead is done prior to calling point
    if (!read_integer(x))
        return (false);
    int c;
    if (!look_for_semi(&c))
        return (false);
    if (c == ';')
        return (error(PERRETC, "Bad X,Y path element."));
    if (!read_integer(y))
        return (false);
    return (true);
}


#define issep(c) (c == '-' || c == ')' || c == '(' || c == ';')

namespace {
    int Pp1(int c)
        { return (isspace(c) || c == ','); }

    int Pp2(int c)
        { return (!isalpha(c) && !isdigit(c) && !issep(c)); }

    int Pp3(int c)
        { return (!isdigit(c) && !issep(c)); }
}


bool
cif_in::read_character(int *cret, bool *abort, int WSControl,
    int EOFControl)
{
    *abort = false;
    if (in_bytes_read > in_fb_incr) {
        show_feedback();
        if (in_interrupted) {
            error(PERRETC, "user interrupt");
            *abort = true;
            *cret = 0;
            return (true);
        }
    }
    int c;
    if (!white_space(&c, WSControl, EOFControl))
        return (false);
    *cret = c;
    return (true);
}


bool
cif_in::read_integer(int *iret)
{
    for (;;) {
        int c = 0;
        if (!get_next(&c, Pp3, PFAILONEOF))
            return (false);
        if (isdigit(c) || c == '-' || c == '+') {
            // read integer
            ungetc((char)c, in_fp);
            int r = fscanf(in_fp, "%d", iret);
            if (r == EOF)
                return (error(PERREOF, 0));
            else if (r == 1)
                break;
        }
    }
    return (true);
}


bool
cif_in::white_space(int *cret, int WSControl, int EOFControl)
{
    *cret = 0;
    int c = 0;
    switch (WSControl) {
    case PSTRIP1:
        if (!get_next(&c, Pp1, EOFControl))
            return (false);
        break;
    case PSTRIP2:
        if (!get_next(&c, Pp2, EOFControl))
            return (false);
        break;
    case PSTRIP3:
        if (!get_next(&c, Pp3, EOFControl))
            return (false);
        break;
    default:
        c = getc(in_fp);
        in_bytes_read++;
        if (c == '\n')
            in_line_cnt++;
        else if (c == EOF) {
            if (EOFControl != PDONTFAILONEOF)
                return (error(PERREOF, 0));
        }
    }
    *cret = c;
    return (true);
}


bool
cif_in::get_next(int *cret, int (*func)(int), int EOFControl)
{
    int c;
    for (;;) {
        c = getc(in_fp);
        in_bytes_read++;
        if (!func(c))
            break;
        if (c == '\n')
            in_line_cnt++;
        else if (c == EOF) {
            if (EOFControl != PDONTFAILONEOF)
                return (error(PERREOF, 0));
            break;
        }
    }
    *cret = c;
    return (true);
}


bool
cif_in::look_ahead(int *cret, int WSControl)
{
    int c;
    if (!white_space(&c, WSControl, PFAILONEOF))
        return (false);
    ungetc((char)c, in_fp);
    *cret = c;
    return (true);
}


bool
cif_in::look_for_semi(int *cret)
{
    int c = 0;
    if (!get_next(&c, Pp3, PFAILONEOF))
        return (false);
    if (c != ';')
        ungetc((char)c, in_fp);
    *cret = c;
    return (true);
}


namespace {
    bool
    test_oldstyle(FILE *fp, bool *issced)
    {
        long told = large_ftell(fp);
        rewind(fp);
        CFtype type;
        if (!FIO()->IsCIF(fp, &type, issced))
            return (false);
        if (type != CFnative)
            return (false);
        large_fseek(fp, told, SEEK_SET);
        return (true);
    }


    // Transform non-Manhattan box to polygon.
    //
    Point *
    box_to_poly(BBox *BB, int xdir, int ydir)
    {
        int x = (BB->left + BB->right);
        int y = (BB->bottom + BB->top);
        int left = 2*BB->left - x;
        int right = 2*BB->right - x;
        int bottom = 2*BB->bottom - y;
        int top = 2*BB->top - y;
        double d = sqrt(xdir*(double)xdir + ydir*(double)ydir);
        double C = xdir/d;
        double S = ydir/d;

        Point *points = new Point[5];
        points[0].set(mmRnd(0.5*(left*C - bottom*S + x)),
            mmRnd(0.5*(left*S + bottom*C + y)));
        points[1].set(mmRnd(0.5*(left*C - top*S + x)),
            mmRnd(0.5*(left*S + top*C + y)));
        points[2].set(mmRnd(0.5*(right*C - top*S + x)),
            mmRnd(0.5*(right*S + top*C + y)));
        points[3].set(mmRnd(0.5*(right*C - bottom*S + x)),
            mmRnd(0.5*(right*S + bottom*C + y)));
        points[4] = points[0];
        return (points);
    }
}


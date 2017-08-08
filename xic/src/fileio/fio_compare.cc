
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
#include "fio_chd.h"
#include "fio_chd_iter.h"
#include "fio_chd_diff.h"
#include "fio_compare.h"
#include "fio_library.h"
#include "cd_celldb.h"
#include "cd_digest.h"
#include "miscutil/filestat.h"
#include "miscutil/timer.h"
#include "miscutil/timedbg.h"


cCompare::cCompare()
{
    c_fname1 = 0;
    c_fname2 = 0;
    c_cell_list1 = 0;
    c_cell_list2 = 0;
    c_layer_list = 0;
    c_obj_types = 0;
    c_max_diffs = 0;
    c_fine_grid = 0;
    c_coarse_mult = 0;
    c_properties = 0;
    c_dmode = Physical;
    c_skip_layers = false;
    c_geometric = false;
    c_diff_only = false;
    c_exp_arrays = false;
    c_flat_geometric = false;
    c_aoi_given = false;
    c_recurse = false;
    c_sloppy = false;
    c_ignore_dups = false;

    c_free_chd1 = false;
    c_free_chd2 = false;
    c_chd1 = 0;
    c_chd2 = 0;
    c_cell_names1 = 0;
    c_cell_names2 = 0;
    c_fp = 0;
}


cCompare::~cCompare()
{
    delete [] c_fname1;
    delete [] c_fname2;
    delete [] c_cell_list1;
    delete [] c_cell_list2;
    delete [] c_layer_list;
    delete [] c_obj_types;

    if (c_free_chd1)
        delete c_chd1;
    if (c_free_chd2)
        delete c_chd2;
    stringlist::destroy(c_cell_names1);
    stringlist::destroy(c_cell_names2);
    if (c_fp)
        fclose(c_fp);
}


bool
cCompare::parse(const char *string)
{
    TimeDbg tdbg("compare_parse");

    char *tok;
    while ((tok = lstring::getqtok(&string)) != 0) {
        if (tok[0] == '-') {
            if (tok[1] == 'a') {
                char *s = lstring::getqtok(&string);
                if (!s || *s == '-') {
                    Errs()->add_error("Missing -a argument.");
                    delete [] tok;
                    return (false);
                }
                double l, b, r, t;
                if (sscanf(s, "%lf,%lf,%lf,%lf", &l, &b, &r, &t) != 4) {
                    Errs()->add_error("Bad -a argument.");
                    delete [] tok;
                    return (false);
                }
                c_AOI.left = INTERNAL_UNITS(l);
                c_AOI.bottom = INTERNAL_UNITS(b);
                c_AOI.right = INTERNAL_UNITS(r);
                c_AOI.top = INTERNAL_UNITS(t);
                c_AOI.fix();
                c_aoi_given = true;
            }
            else if (tok[1] == 'b')
                c_sloppy = true;
            else if (tok[1] == 'c') {
                if (!c_cell_list1 || tok[2] == '1') {
                    if (c_cell_list1) {
                        Errs()->add_error("Duplicate -c argument.");
                        delete [] tok;
                        return (false);
                    }
                    c_cell_list1 = lstring::getqtok(&string);
                    if (!c_cell_list1 || *c_cell_list1 == '-') {
                        Errs()->add_error("Missing -c argument.");
                        delete [] tok;
                        return (false);
                    }
                }
                else if (!c_cell_list2 || tok[2] == '2') {
                    if (c_cell_list2) {
                        Errs()->add_error("Duplicate -c argument.");
                        delete [] tok;
                        return (false);
                    }
                    c_cell_list2 = lstring::getqtok(&string);
                    if (!c_cell_list2 || *c_cell_list2 == '-') {
                        Errs()->add_error("Missing -c argument.");
                        delete [] tok;
                        return (false);
                    }
                }
                else {
                    Errs()->add_error("Error in -c arguments.");
                    delete [] tok;
                    return (false);
                }
            }
            else if (tok[1] == 'd')
                c_diff_only = true;
            else if (tok[1] == 'e')
                c_dmode = Electrical;
            else if (tok[1] == 'f') {
                if (tok[2] == '1') {
                    if (c_fname1) {
                        Errs()->add_error("Duplicate -f1 argument.");
                        delete [] tok;
                        return (false);
                    }
                    c_fname1 = lstring::getqtok(&string);
                    if (!c_fname1 || *c_fname1 == '-') {
                        Errs()->add_error("Missing -f1 argument.");
                        delete [] tok;
                        return (false);
                    }
                }
                else if (tok[2] == '2') {
                    if (c_fname2) {
                        Errs()->add_error("Duplicate -f2 argument.");
                        delete [] tok;
                        return (false);
                    }
                    c_fname2 = lstring::getqtok(&string);
                    if (!c_fname2 || *c_fname2 == '-') {
                        Errs()->add_error("Missing -f2 argument.");
                        delete [] tok;
                        return (false);
                    }
                }
                else
                    c_flat_geometric = true;
            }
            else if (tok[1] == 'g')
                c_geometric = true;
            else if (tok[1] == 'h')
                c_recurse = true;
            else if (tok[1] == 'n')
                c_ignore_dups = true;
            else if (tok[1] == 'p') {
                c_properties = 0;
                char *s = lstring::gettok(&string);
                if (!s || *s == '-') {
                    Errs()->add_error("Missing -p argument.");
                    delete [] s;
                    delete [] tok;
                    return (false);
                }
                lstring::strtolower(s);
                if (strchr(s, 'b'))
                    c_properties |= DiffPrpBox;
                if (strchr(s, 'p'))
                    c_properties |= DiffPrpPoly;
                if (strchr(s, 'w'))
                    c_properties |= DiffPrpWire;
                if (strchr(s, 'l'))
                    c_properties |= DiffPrpLabl;
                if (strchr(s, 'c'))
                    c_properties |= DiffPrpInst;
                if (strchr(s, 's'))
                    c_properties |= DiffPrpCell;
                if (strchr(s, 'n'))
                    ;
                else if (strchr(s, 'u'))
                    c_properties |= DiffPrpCstm;
                else
                    c_properties |= DiffPrpDflt;
                delete [] s;
            }
            else if (tok[1] == 'i') {
                char *s = lstring::gettok(&string);
                if (!s || *s == '-') {
                    Errs()->add_error("Missing -i argument.");
                    delete [] s;
                    delete [] tok;
                    return (false);
                }
                double d = atof(s);
                delete [] s;
                if (d < 1.0 || d > 100.0) {
                    Errs()->add_error("Bad -i argument (1.0 - 100.0).");
                    delete [] tok;
                    return (false);
                }
                c_fine_grid = INTERNAL_UNITS(d);
            }
            else if (tok[1] == 'l') {
                if (c_layer_list) {
                    Errs()->add_error("Duplicate -l argument.");
                    delete [] tok;
                    return (false);
                }
                c_layer_list = lstring::getqtok(&string);
                if (!c_layer_list || *c_layer_list == '-') {
                    Errs()->add_error("Missing -l argument.");
                    delete [] tok;
                    return (false);
                }
            }
            else if (tok[1] == 'm') {
                char *s = lstring::gettok(&string);
                if (!s || *s == '-') {
                    Errs()->add_error("Missing -m argument.");
                    delete [] s;
                    delete [] tok;
                    return (false);
                }
                int i = atoi(s);
                delete [] s;
                if (i < 1 || i > 100) {
                    Errs()->add_error("Bad -m argument (1 - 100).");
                    delete [] tok;
                    return (false);
                }
                c_coarse_mult = i;
            }
            else if (tok[1] == 'r') {
                char *s = lstring::gettok(&string);
                if (!s || *s == '-') {
                    Errs()->add_error("Missing -r argument.");
                    delete [] s;
                    delete [] tok;
                    return (false);
                }
                int i = atoi(s);
                delete [] s;
                if (i < 0) {
                    Errs()->add_error("Bad -r argument (>= 0).");
                    delete [] tok;
                    return (false);
                }
                c_max_diffs = i;
            }
            else if (tok[1] == 's')
                c_skip_layers = true;
            else if (tok[1] == 't') {
                if (c_obj_types) {
                    Errs()->add_error("Duplicate -t argument.");
                    delete [] tok;
                    return (false);
                }
                c_obj_types = lstring::gettok(&string);
                if (!c_obj_types || *c_obj_types == '-') {
                    Errs()->add_error("Missing -t argument.");
                    delete [] tok;
                    return (false);
                }
            }
            else if (tok[1] == 'x')
                c_exp_arrays = true;
            else {
                // unknown option
                Errs()->add_error("Unknown option %s.", tok);
                delete [] tok;
                return (false);
            }
            delete [] tok;
        }
        else if (!c_fname1)
            c_fname1 = tok;
        else if (!c_fname2)
            c_fname2 = tok;
        else {
            Errs()->add_error("Unrecognized argument %s.", tok);
            delete [] tok;
            return (false);
        }
    }
    if (c_dmode == Electrical && (c_geometric || c_flat_geometric)) {
        Errs()->add_error(
            "Can't do geometric comparison of electrical cells.");
        return (false);
    }
    if (c_flat_geometric) {
        if (c_fine_grid == 0)
            c_fine_grid = INTERNAL_UNITS(20.0);
        if (c_coarse_mult == 0)
            c_coarse_mult = 20;
    }
    return (true);
}


namespace {
    stringlist *to_stringlist(const char *str)
    {
        stringlist *s0 = 0, *se = 0;
        char *tok;
        const char *s = str;
        while ((tok = lstring::gettok(&s)) != 0) {
            if (!s0)
                s0 = se = new stringlist(tok, 0);
            else {
                se->next = new stringlist(tok, 0);
                se = se->next;
            }
        }
        return (s0);
    }
}


bool
cCompare::setup()
{
    TimeDbg tdbg("compare_setup");

    if (c_flat_geometric) {
        if (!c_fname1 || !c_fname2) {
            Errs()->add_error("Flat geometric comparison requires two files.");
            return (false);
        }
        if (!c_cell_list2) {
            // This mode requires that the top cell be given
            // explicitly if the file contains multiple top-level
            // cells, so accepting a blank here could cause trouble.

            c_cell_list2 = lstring::copy(c_cell_list1);
        }
    }
    c_cell_names1 = to_stringlist(c_cell_list1);
    c_cell_names2 = to_stringlist(c_cell_list2);

    // If both cell name lists have entries, the list lengths must
    // always be equal.
    //
    if (c_cell_names1 && c_cell_names2 &&
            stringlist::length(c_cell_names1) !=
            stringlist::length(c_cell_names2)) {
        Errs()->add_error("Cell name lists have different lengths.");
        return (false);
    }
    if (c_cell_names2 && !c_cell_names1) {
        Errs()->add_error("Cell name lists have different lengths.");
        return (false);
    }
    if (!c_fname1 && !c_fname2 && !c_cell_names1) {
        Errs()->add_error("No files or cells listed.");
        return (false);
    }
    if (c_recurse && !c_fname1) {
        Errs()->add_error(
            "Recursive comparison not available when left source is memory.");
        return (false);
    }

    if (c_fname1) {
        c_chd1 = CDchd()->chdRecall(c_fname1, false);
        if (!c_chd1) {
            char *realname;
            FILE *fp = FIO()->POpen(c_fname1, "rb", &realname);
            if (!fp) {
                Errs()->add_error("File %s could not be opened.", c_fname1);
                delete [] realname;
                return (false);
            }
            FileType ft1 = FIO()->GetFileType(fp);
            fclose(fp);
            if (!FIO()->IsSupportedArchiveFormat(ft1)) {
                sCHDin chd_in;
                if (!chd_in.check(realname)) {
                    delete [] realname;
                    Errs()->add_error("File %s type not supported.", c_fname1);
                    return (false);
                }
                c_chd1 = chd_in.read(realname, sCHDin::get_default_cgd_type());
                delete [] realname;
                if (!c_chd1) {
                    Errs()->add_error("Error reading CHD file %s.", c_fname1);
                    return (false);
                }
            }
            if (!c_chd1)
                c_chd1 = FIO()->NewCHD(c_fname1, ft1, c_dmode, 0);
            if (!c_chd1)
                return (false);
            c_free_chd1 = true;
        }
    }

    if (c_fname2) {
        c_chd2 = CDchd()->chdRecall(c_fname2, false);
        if (!c_chd2) {
            char *realname;
            FILE *fp = FIO()->POpen(c_fname2, "rb", &realname);
            if (!fp) {
                Errs()->add_error("File %s could not be opened.", c_fname2);
                return (false);
            }
            FileType ft2 = FIO()->GetFileType(fp);
            fclose(fp);
            if (!FIO()->IsSupportedArchiveFormat(ft2)) {
                sCHDin chd_in;
                if (!chd_in.check(realname)) {
                    delete [] realname;
                    Errs()->add_error("File %s type not supported.", c_fname2);
                    return (false);
                }
                c_chd2 = chd_in.read(realname, sCHDin::get_default_cgd_type());
                delete [] realname;
                if (!c_chd2) {
                    Errs()->add_error("Error reading CHD file %s.", c_fname2);
                    return (false);
                }
            }
            if (!c_chd2)
                c_chd2 = FIO()->NewCHD(c_fname2, ft2, c_dmode, 0);
            if (!c_chd2)
                return (false);
            c_free_chd2 = true;
        }
    }

    if (!c_cell_names1) {
        // We know that c_cell_names2 is null here.

        if (c_flat_geometric || c_recurse) {
            // In flat_geometric mode, or if recursive, an empty names
            // list is given the reference source (chd1) default name.

            const char *cn1 = c_chd1->defaultCell(c_dmode);
            if (!cn1) {
                Errs()->add_error("No cells found in first file or CHD.");
                return (false);
            }
            c_cell_names1 = new stringlist(lstring::copy(cn1), 0);
        }
        else {
            // Otherwise, an empty list implies all cells in the reference
            // source (chd1).

            SymTab *st = new SymTab(false, false);
            if (c_chd1->nameTab(c_dmode)) {
                namegen_t gen(c_chd1->nameTab(c_dmode));
                symref_t *p;
                while ((p = gen.next()) != 0) {
                    if (c_dmode == Electrical && FIO()->LookupLibCell(0,
                            Tstring(p->get_name()), LIBdevice, 0))
                        continue;
                    st->add(Tstring(p->get_name()), 0, true);
                }
            }
            c_cell_names1 = SymTab::names(st);
            stringlist::sort(c_cell_names1);
            delete st;

            if (!c_cell_names1) {
                Errs()->add_error("No cells found to compare.");
                return (false);
            }
        }
    }
    if (c_recurse && !c_flat_geometric) {
        // In recursive mode, add the hierarchy cells for each listed cell,
        // using the reference source (chd1).  Filter duplicates.

        SymTab *st = new SymTab(true, false);
        for (stringlist *sl = c_cell_names1; sl; sl = sl->next) {
            stringlist *sx = c_chd1->listCellnames(sl->string, c_dmode);
            while (sx) {
                if (SymTab::get(st, sx->string) == ST_NIL) {
                    if (c_dmode == Electrical && FIO()->LookupLibCell(0,
                            sx->string, LIBdevice, 0))
                        delete [] sx->string;
                    else
                        st->add(sx->string, 0, false);
                }
                else
                    delete [] sx->string;
                sx->string = 0;
                stringlist *stmp = sx;
                sx = sx->next;
                delete stmp;
            }
        }
        stringlist::destroy(c_cell_names1);
        c_cell_names1 = SymTab::names(st);
        stringlist::sort(c_cell_names1);
        delete st;
        stringlist::destroy(c_cell_names2);
        c_cell_names2 = 0;
    }

    if (!filestat::create_bak(DIFF_LOG_FILE)) {
        Errs()->add_error(filestat::error_msg());
        return (false);
    }
    c_fp = fopen(DIFF_LOG_FILE, "w");
    if (!c_fp) {
        Errs()->add_error("Can't open \"%s\" file for output.",
            DIFF_LOG_FILE);
        return (false);
    }

    fprintf(c_fp, "%s from %s\n", DIFF_LOG_HEADER, CD()->ifIdString());
    fprintf(c_fp, "%s\n%s %s\n%s %s\n\n", DIFF_ARCHIVES,
        DIFF_LTOK, c_fname1 ? c_fname1 : "in memory",
        DIFF_RTOK, c_fname2 ? c_fname2 : "in memory");
    fprintf(c_fp, "%s %s\n", DIFF_MODE, DisplayModeName(c_dmode));
    return (true);
}


namespace {
    // This handles the user progress feedback.
    //
    struct cfb_t
    {
        cfb_t()
            {
                cfb_numcells = 0;
                cfb_cellcnt = 0;
                cfb_tm = 0;
                cfb_nx = 0;
                cfb_str = "|/-\\";
                cfb_fmt = "Cells checked: %9u/%u  (%c)";
            }

        void pr1(unsigned int cellcnt, unsigned int numcells)
            {
                if (!(cellcnt & 0xff) || cfb_tm != Timer()->elapsed_msec()) {
                    FIO()->ifInfoMessage(IFMSG_INFO , cfb_fmt, 
                        cellcnt, numcells, cfb_str[(cfb_nx++ & 0x3)]);
                    cfb_tm = Timer()->elapsed_msec();
                }
                cfb_numcells = numcells;
                cfb_cellcnt = cellcnt;
            }

        void pr2()
            {
                if (cfb_tm != Timer()->elapsed_msec()) {
                    FIO()->ifInfoMessage(IFMSG_INFO , cfb_fmt, 
                        cfb_cellcnt, cfb_numcells, cfb_str[(cfb_nx++ & 0x3)]);
                    cfb_tm = Timer()->elapsed_msec();
                }
            }

    private:
        unsigned int cfb_numcells;
        unsigned int cfb_cellcnt;
        unsigned int cfb_tm;
        unsigned int cfb_nx;
        const char *cfb_str;
        const char *cfb_fmt;
    };

    cfb_t cfb;

    void pr2_wrapper() { cfb.pr2(); }
}


DFtype
cCompare::compare()
{
    TimeDbg tdbg("compare_run");
    Sdiff::ufb_setup(&pr2_wrapper);

    if (!c_fp) {
        Errs()->add_error("Output file not open.");
        return (DFerror);
    }

    DFtype df = DFsame;
    if (c_flat_geometric) {
        XIrt ret = XIok;
        unsigned int errcnt = 0;
        for (stringlist *s = c_cell_names1, *ss = c_cell_names2; s;
                s = s->next, ss = ss ? ss->next : 0) {
            fprintf(c_fp, "%s  %s %s  %s %s\n", DIFF_CELLS, DIFF_LTOK,
                s->string, DIFF_RTOK, ss ? ss->string : s->string);

            unsigned int ec;
            ret = cCHD::compareCHDs_fp(c_chd1, s->string, c_chd2,
                ss ? ss->string : 0, c_aoi_given ? &c_AOI : 0,
                c_layer_list, c_skip_layers, c_fp,
                c_max_diffs, &ec, c_coarse_mult, c_fine_grid);
            if (ret != XIok)
                break;
            errcnt += ec;
        }
        if (ret == XIintr) {
            fprintf(c_fp, "*** Run ended prematurely on user abort.\n");
            df = DFabort;
        }
        else if (ret == XIbad) {
            fprintf(c_fp, "*** Run ended prematurely on error.\n");
            df = DFerror;
        }
        else {
            fprintf(c_fp, "*** Run complete.\n");
            df = errcnt ? DFdiffer : DFsame;
        }
    }
    else {
        // This also handles case of both CHDs null, i.e., comparison
        // between cells in memory.

        CHDdiff chd_diff(c_chd1, c_chd2);
        chd_diff.set_layers(c_layer_list, c_skip_layers);
        chd_diff.set_types(c_obj_types);
        chd_diff.set_max_diffs(c_max_diffs);
        chd_diff.set_geometric(c_geometric);
        chd_diff.set_exp_arrays(c_exp_arrays);
        chd_diff.set_properties(c_properties);
        chd_diff.set_sloppy_boxes(c_sloppy);
        chd_diff.set_ignore_dups(c_ignore_dups);

        unsigned int numcells = stringlist::length(c_cell_names1);
        unsigned int cnt = 0;
        bool differ = false;

        for (stringlist *s = c_cell_names1, *ss = c_cell_names2; s;
                s = s->next, ss = ss ? ss->next : 0) {
            cnt++;

            cfb.pr1(cnt, numcells);

            Sdiff *sdiff = 0;
            df = chd_diff.diff(s->string, ss ? ss->string : 0, c_dmode,
                c_diff_only ? 0 : &sdiff);
            if (df == DFabort) {
                Errs()->get_error();
                fprintf(c_fp, "*** Run ended prematurely on user abort.\n");
                break;
            }
            if (df == DFerror) {
                Errs()->add_error("Comparison failed due to error.");
                fprintf(c_fp, "*** Run ended prematurely on error.\n");
                break;
            }

            const char *cn1 = s->string;
            const char *cn2 = ss ? ss->string : 0;
            if (!cn2)
                cn2 = cn1;

            if (df == DFnoLR)
                fprintf(c_fp, "\n%s  %s %s (not found)  %s %s (not found)\n",
                    DIFF_CELLS, DIFF_LTOK, cn1, DIFF_RTOK, cn2);
            else if (df == DFnoR)
                fprintf(c_fp, "\n%s  %s %s  %s %s (not found)\n",
                    DIFF_CELLS, DIFF_LTOK, cn1, DIFF_RTOK, cn2);
            else if (df == DFnoL)
                fprintf(c_fp, "\n%s %s  %s (not found)  %s %s\n",
                    DIFF_CELLS, DIFF_LTOK, cn1, DIFF_RTOK, cn2);
            else if (df == DFdiffer) {
                fprintf(c_fp, "\n%s  %s %s  %s %s\n",
                    DIFF_CELLS, DIFF_LTOK, cn1, DIFF_RTOK, cn2);
                if (c_diff_only)
                    fprintf(c_fp, "    differ\n");
                else {
                    if (sdiff)
                        sdiff->print(c_fp);
                    delete sdiff;
                }
                differ = true;
            }
            if (c_max_diffs && chd_diff.diff_count() >= c_max_diffs) {
                fprintf(c_fp, "*** Max difference count %d reached.\n",
                    c_max_diffs);
                break;
            }
        }
        if (df != DFabort && df != DFerror)
            df = differ ? DFdiffer : DFsame;
    }

    fclose(c_fp);
    c_fp = 0;
    return (df);
}
// End of cCompare functions.


// The diff_parser struct implements a parser for diff.log files.

#define DIFF_CSFXL "_df12"
#define DIFF_CSFXR "_df21"


diff_parser::diff_parser()
{
    dp_fp = 0;
    dp_fname = 0;
    dp_cn1 = 0;
    dp_cn2 = 0;
    dp_next_cn1 = 0;
    dp_next_cn2 = 0;
    dp_o12 = 0;
    dp_o21 = 0;
    dp_linecnt = 0;
    dp_mode = Physical;
}


diff_parser::~diff_parser()
{
    if (dp_fp)
        fclose(dp_fp);
    delete [] dp_fname;
    delete [] dp_cn1;
    delete [] dp_cn2;
    delete [] dp_next_cn1;
    delete [] dp_next_cn2;
    while (dp_o12) {
        const CDo *o = dp_o12;
        dp_o12 = dp_o12->const_next_odesc();
        delete o;
    }
    while (dp_o21) {
        const CDo *o = dp_o21;
        dp_o21 = dp_o21->const_next_odesc();
        delete o;
    }
}


// The public function to read a diff.log file and create new cells to
// hold the difference geometry.  The new cells have the base name of
// the original cell, with suffix DIFF_CSFXL/R added.
//
bool
diff_parser::diff2cells(const char *fname)
{
    if (!open(fname))
        return (false);
    while (dp_next_cn2) {
        if (!read_cell())
            return (false);
        if (!process())
            return (false);
    }
    return (true);
}


bool
diff_parser::open(const char *fname)
{
    if (!fname || !*fname)
        fname = DIFF_LOG_FILE;

    dp_fp = FIO()->POpen(fname, "rb");
    if (!dp_fp) {
        Errs()->add_error("Open: file %s could not be opened.", fname);
        return (false);
    }

    const char *s;
    char buf[256];
    for (;;) {
        s = fgets(buf, 256, dp_fp);
        if (!s) {
            Errs()->add_error("Open: file %s, premature EOF.", fname);
            return (false);
        }
        dp_linecnt++;
        while (isspace(*s))
            s++;
        if (!*s)
            continue;
        if (lstring::prefix(DIFF_LOG_HEADER, s))
            break;
        Errs()->add_error("Open: file %s, bad header.", fname);
        return (false);
    }

    bool found_cell = false;
    while ((s = fgets(buf, 256, dp_fp)) != 0) {
        dp_linecnt++;
        while (isspace(*s))
            s++;
        if (!*s || *s =='#')
            continue;
        char *tok = lstring::gettok(&s);
        if (!strcmp(tok, DIFF_MODE)) {
            delete [] tok;
            tok = lstring::gettok(&s);
            if (tok) {
                if (*tok == 'p' || *tok == 'P')
                    dp_mode = Physical;
                else if (*tok == 'e' || *tok == 'E')
                    dp_mode = Electrical;
            }
        }
        else if (!strcmp(tok, DIFF_CELLS)) {
            found_cell = true;
            delete [] tok;
            break;
        }
        delete [] tok;
    }
    if (!found_cell) {
        Errs()->add_error("Open: file %s, no cell data found.", fname);
        return (false);
    }

    delete [] lstring::gettok(&s);  // DIFF_LTOK
    dp_next_cn1 = lstring::getqtok(&s);
    delete [] lstring::gettok(&s);  // DIFF_RTOK
    dp_next_cn2 = lstring::getqtok(&s);

    dp_fname = lstring::copy(fname);
    return (true);
}


bool
diff_parser::read_cell()
{
    dp_cn1 = dp_next_cn1;
    dp_next_cn1 = 0;
    dp_cn2 = dp_next_cn2;
    dp_next_cn2 = 0;
    dp_o12 = 0;  // these should already be clear
    dp_o21 = 0;
    if (!dp_cn2) {
        Errs()->add_error(
            "read_cell: file %s, line %d, cell name syntax error.",
            dp_fname, dp_linecnt);
        return (false);
    }

    DirecType dr = DirNone;
    CDl *ld = 0;
    const char *s;
    char buf[256];
    while ((s = fgets(buf, 256, dp_fp)) != 0) {
        dp_linecnt++;
        while (isspace(*s))
            s++;
        if (!*s || *s =='#')
            continue;
        char *tok = lstring::gettok(&s);

        if (!strcmp(DIFF_LTOK, tok) || !strcmp(DIFF_RTOK, tok)) {
            if (!strcmp(DIFF_LTOK, tok))
                dr = DirL;
            else
                dr = DirR;
            delete [] tok;
            tok = lstring::gettok(&s);
            if (!tok) {
                Errs()->add_error(
                    "read_cell: file %s, line %d, missing token.",
                    dp_fname, dp_linecnt);
                return (false);
            }
            if (!strcmp(tok, DIFF_LAYER)) {
                delete [] tok;
                tok = lstring::gettok(&s);
                if (!tok) {
                    Errs()->add_error(
                        "read_cell: file %s, line %d, missing layer.",
                        dp_fname, dp_linecnt);
                    return (false);
                }
                if (FIO()->IsNoCreateLayer()) {
                    ld = CDldb()->findLayer(tok, dp_mode);
                    if (!ld) {
                        Errs()->add_error(
                            "read_cell: file %s, line %d, not allowed to "
                            "create new layer %s.",
                            dp_fname, dp_linecnt, tok);
                        delete [] tok;
                        return (false);
                    }
                }
                else {
                    ld = CDldb()->newLayer(tok, dp_mode);
                    if (!ld) {
                        Errs()->add_error(
                            "read_cell: file %s, line %d, failed to "
                            "create new layer %s.",
                            dp_fname, dp_linecnt, tok);
                        delete [] tok;
                        return (false);
                    }
                }
            }
            else if (!strcmp(tok, DIFF_INSTANCES)) {
            }
            else {
                Errs()->add_error(
                    "read_cell: file %s, line %d, unknown token \"%s\".",
                    dp_fname, dp_linecnt, tok);
                delete [] tok;
                return (false);
            }
        }
        else if (*tok == 'B' || *tok == 'P' || *tok == 'W') {
            if (!ld) {
                Errs()->add_error(
                    "read_cell: file %s, line %d, geometry spec without layer.",
                    dp_fname, dp_linecnt);
                delete [] tok;
                return (false);
            }
            if (!dr) {
                Errs()->add_error(
                    "read_cell: file %s, line %d, geometry spec without R/L.",
                    dp_fname, dp_linecnt);
                delete [] tok;
                return (false);
            }

            // May have to grab multiple lines, look for ';' termination.
            sLstr lstr;
            for (;;) {
                lstr.add(buf);
                char *t = buf + strlen(buf) - 1;
                while (t >= buf && isspace(*t))
                    t--;
                if (*t == ';')
                    break;
                s = fgets(buf, 256, dp_fp);
                if (!s) {
                    Errs()->add_error(
                        "read_cell: file %s, premature EOF.", dp_fname);
                    delete [] tok;
                    return (false);
                }
                dp_linecnt++;
            }

            CDo *odesc = CDo::fromCifString(ld, lstr.string());
            if (!odesc) {
                Errs()->add_error(
                    "read_cell: file %s, line %d, geometry parse failed.",
                    dp_fname, dp_linecnt);
                delete [] tok;
                return (false);
            }
            if (dr == DirL) {
                odesc->set_next_odesc(dp_o12);
                dp_o12 = odesc;
            }
            else {
                odesc->set_next_odesc(dp_o21);
                dp_o21 = odesc;
            }
        }
        else if (!strcmp(tok, DIFF_CELLS)) {
            delete [] lstring::gettok(&s);  // DIFF_LTOK
            dp_next_cn1 = lstring::getqtok(&s);
            delete [] lstring::gettok(&s);  // DIFF_RTOK
            dp_next_cn2 = lstring::getqtok(&s);
            delete [] tok;
            break;
        }
        delete [] tok;
    }
    return (true);
}


bool
diff_parser::process()
{
    char *stmp = new char[strlen(dp_cn1) + strlen(DIFF_CSFXL) + 1];
    strcpy(lstring::stpcpy(stmp, dp_cn1), DIFF_CSFXL);
    CDs *sd1 = CDcdb()->findCell(stmp, dp_mode);
    if (sd1)
        sd1->clear(dp_o12 != 0);
    else if (dp_o12)
        sd1 = CDcdb()->insertCell(stmp, dp_mode);
    delete [] stmp;
    while (dp_o12) {
        const CDo *o = dp_o12;
        dp_o12 = dp_o12->const_next_odesc();
        bool ret = add_object(sd1, o);
        delete o;
        if (!ret)
            return (false);
    }

    stmp = new char[strlen(dp_cn2) + strlen(DIFF_CSFXR) + 1];
    strcpy(lstring::stpcpy(stmp, dp_cn2), DIFF_CSFXR);
    CDs *sd2 = CDcdb()->findCell(stmp, dp_mode);
    if (sd2)
        sd2->clear(dp_o21 != 0);
    else if (dp_o21)
        sd2 = CDcdb()->insertCell(stmp, dp_mode);
    delete [] stmp;
    while (dp_o21) {
        const CDo *o = dp_o21;
        dp_o21 = dp_o21->const_next_odesc();
        bool ret = add_object(sd2, o);
        delete o;
        if (!ret)
            return (false);
    }

    return (true);
}


bool
diff_parser::add_object(CDs *cursd, const CDo *ocpy)
{
    CDl *ld = ocpy->ldesc();
    if (ocpy->type() == CDBOX) {
        BBox BB = ocpy->oBB();
        CDo *newo;
        CDerrType eret = cursd->makeBox(ld, &BB, &newo);
        if (eret != CDok && eret != CDbadBox) {
            Errs()->add_error("makeBox failed.");
            return (false);
        }
        if (eret == CDok && !cursd->mergeBoxOrPoly(newo, false)) {
            Errs()->add_error("mergeBoxOrPoly failed.");
            return (false);
        }
    }
    else if (ocpy->type() == CDPOLYGON) {
        int num = ((const CDpo*)ocpy)->numpts();
        Poly poly(num, Point::dup(((const CDpo*)ocpy)->points(), num));
        CDpo *newo;
        CDerrType eret = cursd->makePolygon(ld, &poly, &newo);
        if (eret != CDok) {
            if (eret != CDbadPolygon) {
                Errs()->add_error("makePolygon failed.");
                return (false);
            }
        }
        if (eret == CDok && !cursd->mergeBoxOrPoly(newo, false)) {
            Errs()->add_error("mergeBoxOrPoly failed.");
            return (false);
        }
    }
    else if (ocpy->type() == CDWIRE) {
        int num = ((const CDw*)ocpy)->numpts();
        Wire wire(num, Point::dup(((const CDw*)ocpy)->points(), num),
            ((const CDw*)ocpy)->attributes());
        CDw *newo;
        CDerrType eret = cursd->makeWire(ld, &wire, &newo);
        if (eret != CDok) {
            if (eret != CDbadWire) {
                Errs()->add_error("makeWire failed.");
                return (false);
            }
        }
        if (eret == CDok && !cursd->mergeWire(newo, false)) {
            Errs()->add_error("mergeWire failed.");
            return (false);
        }
    }
    return (true);
}


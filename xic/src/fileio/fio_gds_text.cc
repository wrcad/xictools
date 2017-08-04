
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
#include "fio_gdsii.h"


//
//===========================================================================
// Actions for printing records in ascii text format
//===========================================================================
//

// Set to text output mode.
//
// Printing starts at the first record with offset greater than or
// equal to start_ofs, and continues until numrecs have been printed,
// or numsyms mave been printed, or the record containing end_ofs has
// been printed, or EOF, whichever comes first.  If numsyms or numrecs
// is -1, or end_ofs is less than or equal to start_ofs, the
// respective test is skipped.  If start_ofs is given and numsyms is
// zero, records to the end of cell will be printed.
//
bool
gds_in::setup_ascii_out(const char *txtfile, uint64_t start_ofs,
    uint64_t end_ofs, int numrecs, int numsyms)
{
    in_print_fp = 0;
    if (txtfile) {
        if (!*txtfile || !strcmp(txtfile, "stdout"))
            in_print_fp = stdout;
        else if (!strcmp(txtfile, "stderr"))
            in_print_fp = stderr;
        else {
            in_print_fp = fopen(txtfile, "w");
            if (!in_print_fp) {
                Errs()->add_error("can't open %s, permission denied.",
                    txtfile);
                return (false);
            }
        }
        fprintf(in_print_fp, ">> %s\n", CD()->ifIdString());
        if (in_filename) {
            if (numsyms > 0 || numrecs > 0 || start_ofs > 0 ||
                    end_ofs > start_ofs)
                fprintf(in_print_fp,
                    ">> PARTIAL text rendition of GDSII file %s\n",
                    in_filename);
            else
                fprintf(in_print_fp, ">> Text rendition of GDSII file %s.\n",
                    in_filename);
        }
    }
    in_action = cvOpenModePrint;

    in_print_start = start_ofs;
    in_print_end = end_ofs;
    in_print_reccnt = numrecs;
    in_print_symcnt = numsyms;

    ftab[II_HEADER]         = &gds_in::ap_header;
    ftab[II_BGNLIB]         = &gds_in::ap_bgnlib;
    ftab[II_LIBNAME]        = &gds_in::ap_libname;
    ftab[II_UNITS]          = &gds_in::ap_units;
    ftab[II_BGNSTR]         = &gds_in::ap_bgnstr;
    ftab[II_STRNAME]        = &gds_in::ap_strname;
    ftab[II_BOUNDARY]       = &gds_in::ap_boundary;
    ftab[II_PATH]           = &gds_in::ap_path;
    ftab[II_SREF]           = &gds_in::ap_sref;
    ftab[II_AREF]           = &gds_in::ap_aref;
    ftab[II_TEXT]           = &gds_in::ap_text;
    ftab[II_LAYER]          = &gds_in::ap_layer;
    ftab[II_DATATYPE]       = &gds_in::ap_datatype;
    ftab[II_WIDTH]          = &gds_in::ap_width;
    ftab[II_XY]             = &gds_in::ap_xy;
    ftab[II_SNAME]          = &gds_in::ap_sname;
    ftab[II_COLROW]         = &gds_in::ap_colrow;
    ftab[II_SNAPNODE]       = &gds_in::ap_snapnode;
    ftab[II_TEXTTYPE]       = &gds_in::ap_texttype;
    ftab[II_PRESENTATION]   = &gds_in::ap_presentation;
    ftab[II_STRING]         = &gds_in::ap_string;
    ftab[II_STRANS]         = &gds_in::ap_strans;
    ftab[II_MAG]            = &gds_in::ap_mag;
    ftab[II_ANGLE]          = &gds_in::ap_angle;
    ftab[II_REFLIBS]        = &gds_in::ap_reflibs;
    ftab[II_FONTS]          = &gds_in::ap_fonts;
    ftab[II_PATHTYPE]       = &gds_in::ap_pathtype;
    ftab[II_GENERATIONS]    = &gds_in::ap_generations;
    ftab[II_ATTRTABLE]      = &gds_in::ap_attrtable;
    ftab[II_NODETYPE]       = &gds_in::ap_nodetype;
    ftab[II_PROPATTR]       = &gds_in::ap_propattr;
    ftab[II_PROPVALUE]      = &gds_in::ap_propvalue;
    ftab[II_BOX]            = &gds_in::ap_box;
    ftab[II_BOXTYPE]        = &gds_in::ap_boxtype;
    ftab[II_PLEX]           = &gds_in::ap_plex;
    ftab[II_BGNEXTN]        = &gds_in::ap_bgnextn;
    ftab[II_ENDEXTN]        = &gds_in::ap_endextn;

    return (true);
}


void
gds_in::print_offset()
{
    if (in_printing)
#ifdef WIN32
        fprintf(in_print_fp, "%09I64d ", (long long)in_offset);
#else
        fprintf(in_print_fp, "%09lld ", (long long)in_offset);
#endif
}


void
gds_in::print_space(int num)
{
    if (in_printing)
        fprintf(in_print_fp, "%*c", num, ' ');
}


void
gds_in::print_int(const char *text, int num, bool term)
{
    const char *fmt = term ? "%-16s%d;\n" : "%-16s%d\n";
    if (in_printing)
        fprintf(in_print_fp, fmt, text, num);
}


void
gds_in::print_int2(const char *text, int num1, int num2, bool term)
{
    const char *fmt = term ? "%-16s%d, %d;\n" : "%-16s%d, %d\n";
    if (in_printing)
        fprintf(in_print_fp, fmt, text, num1, num2);
}


void
gds_in::print_float(const char *text, double d, bool term)
{
    const char *fmt = term ? "%-16s%.6e;\n" : "%-16s%.6e\n";
    if (in_printing)
        fprintf(in_print_fp, fmt, text, d);
}


void
gds_in::print_word(const char *text, bool term)
{
    const char *fmt = term ? "%-16s;\n" : "%s\n";
    if (in_printing)
        fprintf(in_print_fp, fmt, text);
}


void
gds_in::print_word2(const char *text1, const char *text2, bool term)
{
    const char *fmt = term ? "%-16s%s;\n" : "%-16s%s\n";
    if (in_printing)
        fprintf(in_print_fp, fmt, text1, text2);
}


void
gds_in::print_word_end(const char *text)
{
    if (in_printing)
        fprintf(in_print_fp, "END %s;\n", text);
}


inline void
gds_in::print_date(const char *text, tm *tm, bool term)
{
    const char *fmt =
        term ? "%-16s%d/%d/%02d  %d:%d:%d;\n" : "%-16s%d/%d/%02d  %d:%d:%d\n";
    if (in_printing)
        fprintf(in_print_fp, fmt, text, tm->tm_mon, tm->tm_mday,
            tm->tm_year % 100, tm->tm_hour, tm->tm_min, tm->tm_sec);
}


bool
gds_in::ap_header()
{
    in_version = shortval(in_cbuf);
    if (in_printing)
        fprintf(in_print_fp, ">> Opening version %d library\n", in_version);
    return (true);
}


bool
gds_in::ap_bgnlib()
{
    char *p = in_cbuf;
    date_value(&in_cdate, &p);
    date_value(&in_mdate, &p);
    return (true);
}


bool
gds_in::ap_libname()
{
    print_offset();
    print_space(4);
    print_word2("LIBRARY", in_cbuf, false);
    print_space(14);
    print_date("MODIFICATION", &in_cdate, false);
    print_space(14);
    print_date("ACCESS", &in_mdate, true);
    return (true);
}


bool
gds_in::ap_units()
{
    double uunit = doubleval(in_cbuf);
    double munit = doubleval(in_cbuf+8);
    if (in_mode == Physical)
        in_scale = dfix(in_ext_phys_scale*1e6*CDphysResolution*munit);
    else
        in_scale = dfix(1e6*CDelecResolution*munit);
    in_needs_mult = (in_scale != 1.0);
    if (in_mode == Physical)
        in_phys_scale = in_scale;

    print_offset();
    print_space(4);
    print_float("U-UNIT", uunit, false);
    print_space(14);
    print_float("M-UNIT", munit, true);
    return (true);
}


bool
gds_in::ap_bgnstr()
{
    char *p = in_cbuf;
    date_value(&in_cdate, &p);
    date_value(&in_mdate, &p);

    if (!get_record())
        return (false);
    if (in_rectype != II_STRNAME) {
#ifdef WIN32
        fprintf(in_print_fp, ">> Unexpected record type %d at offset %I64u",
#else
        fprintf(in_print_fp, ">> Unexpected record type %d at offset %llu",
#endif
            in_rectype, (unsigned long long)in_offset);
        return (false);
    }
    if (!ap_strname())
        return (false);

    // read symbol
    in_obj_offset = 0;
    in_layer = in_dtype = in_attrib = -1;

    bool nbad;
    while ((nbad = get_record())) {
        if (in_rectype == II_ENDSTR) {
            print_offset();
            print_space(4);
            print_word_end("STRUCTURE");
            if (in_print_symcnt > 0)
                in_print_symcnt--;
            if (in_print_symcnt == 0)
                break;
            break;
        }

        in_obj_offset = in_offset;
        if (in_rectype == II_EOF)
            break;
        if (in_rectype >= GDS_NUM_REC_TYPES) {
            unsup();
            continue;
        }
        if (!(this->*ftab[in_rectype])())
            return (false);
    }
    return (nbad);
}


bool
gds_in::ap_strname()
{
    print_offset();
    print_space(4);
    print_word2("STRUCTURE", in_cbuf, false);
    if (strlen(in_cbuf) > GDS_MAX_STRNAM_LEN && in_version < 7)
        fprintf(in_print_fp,
            ">> Warning: name longer than %d characters, not allowed in "
            "GDSII\n>> release %d file.\n", GDS_MAX_STRNAM_LEN, in_version);
    print_space(14);
    print_date("CREATION", &in_cdate, false);
    print_space(14);
    print_date("ACCESS", &in_mdate, true);
    return (true);
}


bool
gds_in::ap_boundary()
{
    print_offset();
    print_space(8);
    print_word("BOUNDARY", false);
    if (!read_element())
        return (false);
    return (true);
}


bool
gds_in::ap_path()
{
    print_offset();
    print_space(8);
    print_word("PATH", false);
    if (!read_element())
        return (false);
    return (true);
}


bool
gds_in::ap_sref()
{
    print_offset();
    print_space(8);
    if (in_printing)
        fprintf(in_print_fp, "%-8s", "SREF");
    if (!read_element())
        return (false);
    return (true);
}


bool
gds_in::ap_aref()
{
    print_offset();
    print_space(8);
    if (in_printing)
        fprintf(in_print_fp, "%-8s", "AREF");
    if (!read_element())
        return (false);
    return (true);
}


bool
gds_in::ap_text()
{
    print_offset();
    print_space(8);
    print_word("TEXT", false);
    if (!read_element())
        return (false);
    return (true);
}


bool
gds_in::ap_layer()
{
    print_offset();
    print_space(12);
    print_int("LAYER", shortval(in_cbuf), true);
    return (true);
}


bool
gds_in::ap_datatype()
{
    print_offset();
    print_space(12);
    print_int("DATATYPE", shortval(in_cbuf), true);
    return (true);
}


bool
gds_in::ap_width()
{
    print_offset();
    print_space(12);
    print_int("WIDTH", longval(in_cbuf), true);
    return (true);
}


bool
gds_in::ap_xy()
{
    if (!in_printing)
        return (true);
    if (in_elemrec == II_AREF) {
        print_offset();
        print_space(12);
        print_int2("PREF", in_points[0].x, in_points[0].y, false);
        print_space(22);
        print_int2("PCOL", in_points[1].x, in_points[1].y, false);
        print_space(22);
        print_int2("PROW", in_points[2].x, in_points[2].y, true);
    }
    else {
        if (in_numpts > 1) {
            print_offset();
            print_space(12);
            fprintf(in_print_fp, "%-16s%-8d%d, %d\n", "COORDINATES",
                in_numpts, in_points[0].x, in_points[0].y);
            for (int i = 1; i < in_numpts-1; i++) {
                print_space(46);
                fprintf(in_print_fp, "%d, %d\n",
                    in_points[i].x, in_points[i].y);
            }
            print_space(46);
            fprintf(in_print_fp, "%d, %d;\n",
                in_points[in_numpts-1].x, in_points[in_numpts-1].y);
        }
        else {
            print_offset();
            print_space(12);
            print_int2("COORDINATE", in_points[0].x, in_points[0].y, true);
        }
    }
    return (true);
}


bool
gds_in::ap_sname()
{
    print_word(in_cbuf, false);
    if (strlen(in_cbuf) > GDS_MAX_STRNAM_LEN && in_version < 7)
        fprintf(in_print_fp,
            ">> Warning: name longer than %d characters, not allowed in "
            "GDSII\n>> release %d file.\n", GDS_MAX_STRNAM_LEN, in_version);
    return (true);
}


bool
gds_in::ap_colrow()
{
    print_offset();
    print_space(12);
    char *p = in_cbuf;
    print_int("COLUMNS", short_value(&p), false);
    print_space(22);
    print_int("ROWS", short_value(&p), true);
    return (true);
}


bool
gds_in::ap_snapnode()
{
    print_offset();
    print_space(8);
    print_word("SNAPNODE", false);
    if (!read_element())
        return (false);
    return (true);
}


bool
gds_in::ap_texttype()
{
    print_offset();
    print_space(12);
    print_int("TEXTTYPE", shortval(in_cbuf), true);
    return (true);
}


bool
gds_in::ap_presentation()
{
    print_offset();
    print_space(12);
    print_int("HJUSTIFICATION", in_cbuf[1] & 3, false);
    print_space(22);
    print_int("VJUSTIFICATION", (in_cbuf[1] & 12) >> 2, false);
    print_space(22);
    print_int("FONT", (in_cbuf[1] & 48) >> 4, true);
    return (true);
}


bool
gds_in::ap_string()
{
    print_offset();
    print_space(12);
    if (in_printing) {
        fprintf(in_print_fp, "%-16s", "STRING");
        fputs(in_cbuf, in_print_fp);
        fputs(";\n", in_print_fp);
    }
    if (strlen(in_cbuf) > GDS_MAX_TEXT_LEN && in_version < 7)
        fprintf(in_print_fp,
            ">> Warning: string longer than %d characters, not allowed in "
            "GDSII\n>> release %d file.\n", GDS_MAX_TEXT_LEN, in_version);
    return (true);
}


bool
gds_in::ap_strans()
{
    if (in_cbuf[0] & 128) {
        print_offset();
        print_space(12);
        print_word("REFLECTION", !((in_cbuf[1] & 4) || (in_cbuf[1] & 2)));
    }
    if (in_cbuf[1] & 4) {
        if (!(in_cbuf[0] & 128)) {
            print_offset();
            print_space(12);
        }
        else
            print_space(22);
        print_word("ABSOLUTE MAGNIFICATION", !(in_cbuf[1] & 2));
    }
    if (in_cbuf[1] & 2) {
        if ((in_cbuf[0] & 128) || (in_cbuf[1] & 4)) {
            print_offset();
            print_space(12);
        }
        else
            print_space(22);
        print_word("ABSOLUTE ANGLE", true);
    }
    return (true);
}


bool
gds_in::ap_mag()
{
    print_offset();
    print_space(12);
    print_float("MAGNIFICATION", doubleval(in_cbuf), true);
    return (true);
}


bool
gds_in::ap_angle()
{
    print_offset();
    print_space(12);
    print_float("ANGLE", doubleval(in_cbuf), true);
    return (true);
}


bool
gds_in::ap_reflibs()
{
    char tbuf[GDS_MAX_LIBNAM_LEN+1];
    tbuf[GDS_MAX_LIBNAM_LEN] = '\0';
    print_offset();
    print_space(4);
    strncpy(tbuf, in_cbuf, GDS_MAX_LIBNAM_LEN);
    print_word2("REFLIB", tbuf, false);
    strncpy(tbuf, in_cbuf + GDS_MAX_LIBNAM_LEN, GDS_MAX_LIBNAM_LEN);
    print_space(14);
    print_word2("REFLIB", tbuf, true);
    return (true);
}


bool
gds_in::ap_fonts()
{
    char tbuf[GDS_MAX_LIBNAM_LEN+1];
    tbuf[GDS_MAX_LIBNAM_LEN] = '\0';
    print_offset();
    print_space(4);
    strncpy(tbuf, in_cbuf, GDS_MAX_LIBNAM_LEN);
    print_word2("FONT0", tbuf, false);
    strncpy(tbuf, in_cbuf + GDS_MAX_LIBNAM_LEN, GDS_MAX_LIBNAM_LEN);
    print_space(14);
    print_word2("FONT1", tbuf, false);
    strncpy(tbuf, in_cbuf + 2*GDS_MAX_LIBNAM_LEN, GDS_MAX_LIBNAM_LEN);
    print_space(14);
    print_word2("FONT2", tbuf, false);
    strncpy(tbuf, in_cbuf + 3*GDS_MAX_LIBNAM_LEN, GDS_MAX_LIBNAM_LEN);
    print_space(14);
    print_word2("FONT3", tbuf, true);
    return (true);
}


bool
gds_in::ap_pathtype()
{
    print_offset();
    print_space(12);
    print_int("PATHTYPE", shortval(in_cbuf), true);
    return (true);
}


bool
gds_in::ap_generations()
{
    print_offset();
    print_space(4);
    print_int("GENERATIONS", shortval(in_cbuf), true);
    return (true);
}


bool
gds_in::ap_attrtable()
{
    print_offset();
    print_space(4);
    print_word2("ATTRIBUTE-FILE", in_cbuf, true);
    return (true);
}


bool
gds_in::ap_nodetype()
{
    print_offset();
    print_space(12);
    print_int("NODETYPE", shortval(in_cbuf), true);
    return (true);
}


bool
gds_in::ap_propattr()
{
    print_offset();
    if (in_elemrec)
        print_space(12);
    else
        print_space(4);
    if (in_printing)
        fprintf(in_print_fp, "%-16s%d=", "ATTRIBUTES", shortval(in_cbuf));
    return (true);
}


bool
gds_in::ap_propvalue()
{
    if (!in_elemrec) {
        if (in_printing) {
            fprintf(in_print_fp, "%s;\n", in_cbuf);
            fprintf(in_print_fp, ">> Non-standard extension.\n");
        }
    }
    else {
        if (in_printing) {
            fputs(in_cbuf, in_print_fp);
            fputs(";\n", in_print_fp);
        }
        if (strlen(in_cbuf) > GDS_MAX_PRPSTR_LEN && in_version < 7)
            fprintf(in_print_fp,
                ">> Warning: string longer than %d characters, not allowed in"
                " GDSII\n>> release %d file.\n", GDS_MAX_PRPSTR_LEN,
                in_version);
    }
    return (true);
}


bool
gds_in::ap_box()
{
    print_offset();
    print_space(8);
    print_word("BOX", false);
    if (!read_element())
        return (false);
    return (true);
}


bool
gds_in::ap_boxtype()
{
    print_offset();
    print_space(12);
    print_int("BOXTYPE", shortval(in_cbuf), true);
    return (true);
}


bool
gds_in::ap_plex()
{
    print_offset();
    print_space(12);
    print_int("PLEX", longval(in_cbuf), true);
    return (true);
}


bool
gds_in::ap_bgnextn()
{
    print_offset();
    print_space(12);
    print_int("BGNEXTN", longval(in_cbuf), true);
    return (true);
}


bool
gds_in::ap_endextn()
{
    print_offset();
    print_space(12);
    print_int("ENDEXTN", longval(in_cbuf), true);
    return (true);
}


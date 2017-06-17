
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
 $Id: fio_oas_text.cc,v 1.22 2011/07/13 18:42:31 stevew Exp $
 *========================================================================*/

#include "fio.h"
#include "fio_oasis.h"
#include <ctype.h>


//
//===========================================================================
// Functions for printing records in ascii text format
//===========================================================================
//

// Set to text output mode.
//
bool
oas_in::setup_ascii_out(const char *txtfile, uint64_t start_ofs,
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
    }
    in_action = cvOpenModePrint;

    // not used yet
    in_print_start = start_ofs;
    in_print_end = end_ofs;
    in_print_reccnt = numrecs;
    in_print_symcnt = numsyms;

    // reset dispatch table
    ftab[0] = &oas_in::print_nop;
    ftab[1] = &oas_in::print_start;
    ftab[2] = &oas_in::print_end;
    ftab[3] = &oas_in::print_name;
    ftab[4] = &oas_in::print_name_r;
    ftab[5] = &oas_in::print_name;
    ftab[6] = &oas_in::print_name_r;
    ftab[7] = &oas_in::print_name;
    ftab[8] = &oas_in::print_name_r;
    ftab[9] = &oas_in::print_name;
    ftab[10] = &oas_in::print_name_r;
    ftab[11] = &oas_in::print_layername;
    ftab[12] = &oas_in::print_layername;
    ftab[13] = &oas_in::print_cell;
    ftab[14] = &oas_in::print_cell;
    ftab[15] = &oas_in::print_nop;
    ftab[16] = &oas_in::print_nop;
    ftab[17] = &oas_in::print_placement;
    ftab[18] = &oas_in::print_placement;
    ftab[19] = &oas_in::print_text;
    ftab[20] = &oas_in::print_rectangle;
    ftab[21] = &oas_in::print_polygon;
    ftab[22] = &oas_in::print_path;
    ftab[23] = &oas_in::print_trapezoid;
    ftab[24] = &oas_in::print_trapezoid;
    ftab[25] = &oas_in::print_trapezoid;
    ftab[26] = &oas_in::print_ctrapezoid;
    ftab[27] = &oas_in::print_circle;
    ftab[28] = &oas_in::print_property;
    ftab[29] = &oas_in::print_nop;
    ftab[30] = &oas_in::print_xname;
    ftab[31] = &oas_in::print_xname;
    ftab[32] = &oas_in::print_xelement;
    ftab[33] = &oas_in::print_xgeometry;
    ftab[34] = &oas_in::print_cblock;

    if (FIO()->IsOasPrintNoWrap())
        in_print_break_lines = false;
    if (FIO()->IsOasPrintOffset())
        in_print_offset = true;

    return (true);
}


namespace {
    const unsigned int RIGHT_MARGIN = 75;
    const unsigned int INDENTATION = 8;

    // This is borrowed from anuvad-0.2
    const char *const oasisRecordNames[] =
    {
        "PAD",                  //  0
        "START",                //  1
        "END",                  //  2
        "CELLNAME",             //  3
        "CELLNAME",             //  4
        "TEXTSTRING",           //  5

        "TEXTSTRING",           //  6
        "PROPNAME",             //  7
        "PROPNAME",             //  8
        "PROPSTRING",           //  9
        "PROPSTRING",           // 10

        "LAYERNAME_GEOMETRY",   // 11
        "LAYERNAME_TEXT",       // 12
        "CELL",                 // 13
        "CELL",                 // 14
        "XYABSOLUTE",           // 15

        "XYRELATIVE",           // 16
        "PLACEMENT",            // 17
        "PLACEMENT_X",          // 18
        "TEXT",                 // 19
        "RECTANGLE",            // 20

        "POLYGON",              // 21
        "PATH",                 // 22
        "TRAPEZOID",            // 23
        "TRAPEZOID_A",          // 24
        "TRAPEZOID_B",          // 25

        "CTRAPEZOID",           // 26
        "CIRCLE",               // 27
        "PROPERTY",             // 28
        "PROPERTY_REPEAT",      // 29
        "XNAME",                // 30

        "XNAME",                // 31
        "XELEMENT",             // 32
        "XGEOMETRY",            // 33
        "CBLOCK",               // 34

        // begin "pseudo-records"
        "END_CBLOCK",           // 35
        "CELLNAME_TABLE",       // 36
        "TEXTSTRING_TABLE",     // 37
        "PROPNAME_TABLE",       // 38
        "PROPSTRING_TABLE",     // 39
        "LAYERNAME_TABLE",      // 40
        "XNAME_TABLE"           // 41
    };
}

enum AsciiKeyword
{
    KeyAngle,
    KeyAttribute,
    KeyChecksum32,
    KeyCRC32,
    KeyDatatype,
    KeyDelta_A,
    KeyDelta_B,
    KeyEndExtn,
    KeyFlip,
    KeyHalfwidth,
    KeyHeight,
    KeyHorizontal,
    KeyLayer,
    KeyMag,
    KeyName,
    KeyNone,
    KeyPointList,
    KeyRadius,
    KeyRefnum,
    KeyRepetition,
    KeySquare,
    KeyStandard,
    KeyStartExtn,
    KeyString,
    KeyTextlayer,
    KeyTexttype,
    KeyTrapType,
    KeyValues,
    KeyVertical,
    KeyWidth,
    KeyX,
    KeyY
};

namespace {
    const char *const asciiKeywords[] =
    {
        "angle",            // KeyAngle
        "attribute",        // KeyAttribute
        "checksum32",       // KeyChecksum32
        "crc32",            // KeyCRC32
        "datatype",         // KeyDatatype
        "delta_a",          // KeyDelta_A
        "delta_b",          // KeyDelta_B
        "end_extn",         // KeyEndExtn
        "flip",             // KeyFlip
        "halfwidth",        // KeyHalfwidth
        "height",           // KeyHeight
        "horizontal",       // KeyHorizontal
        "layer",            // KeyLayer
        "mag",              // KeyMag
        "name",             // KeyName
        "none",             // KeyNone
        "ptlist",           // KeyPointList
        "radius",           // KeyRadius
        "refnum",           // KeyRefnum
        "rep",              // KeyRepetition
        "square",           // KeySquare
        "standard",         // KeyStandard
        "start_extn",       // KeyStartExtn
        "string",           // KeyString
        "textlayer",        // KeyTextlayer
        "texttype",         // KeyTexttype
        "ctraptype",        // KeyTrapType
        "values",           // KeyValues
        "vertical",         // KeyVertical
        "width",            // KeyWidth
        "x",                // KeyX
        "y"                 // KeyY
    };
}


bool
oas_in::print_nop(unsigned int ix)
{
    // '0'
    print_recname(ix);
    put_char('\n');
    return (true);
}


bool
oas_in::print_start(unsigned int ix)
{
    // '1' version-string unit offset-flag [offset-table]
    print_recname(ix);

    if (!print_string(0, 0))
        return (false);

    if (!print_real(&in_unit))
        return (false);

    unsigned int uu;
    if (!print_unsigned(&uu))
        return (false);
    in_offset_flag = (uu != 0);

    if (in_offset_flag == 0) {
        if (!print_unsigned(0))
            return (false);
        if (!print_unsigned64(&in_cellname_off))
            return (false);
        if (!print_unsigned(0))
            return (false);
        if (!print_unsigned64(&in_textstring_off))
            return (false);
        if (!print_unsigned(0))
            return (false);
        if (!print_unsigned64(&in_propname_off))
            return (false);
        if (!print_unsigned(0))
            return (false);
        if (!print_unsigned64(&in_propstring_off))
            return (false);
        if (!print_unsigned(0))
            return (false);
        if (!print_unsigned64(&in_layername_off))
            return (false);
        if (!print_unsigned(0))
            return (false);
        if (!print_unsigned64(&in_xname_off))
            return (false);
    }
    else {
        // Have to (silently) read the table offsets in the END record.
        int64_t posn = in_fp->z_tell();
        if (in_fp->z_seek(-256, SEEK_END) < 0) {
            Errs()->add_error("print_start: seek end failed.");
            in_nogo = true;
            return (false);
        }
        read_unsigned();
        if (in_nogo)
            return (false);
        in_cellname_off = read_unsigned64();
        if (in_nogo)
            return (false);
        read_unsigned();
        if (in_nogo)
            return (false);
        in_textstring_off = read_unsigned64();
        if (in_nogo)
            return (false);
        read_unsigned();
        if (in_nogo)
            return (false);
        in_propname_off = read_unsigned64();
        if (in_nogo)
            return (false);
        read_unsigned();
        if (in_nogo)
            return (false);
        in_propstring_off = read_unsigned64();
        if (in_nogo)
            return (false);
        read_unsigned();
        if (in_nogo)
            return (false);
        in_layername_off = read_unsigned64();
        if (in_nogo)
            return (false);
        read_unsigned();
        if (in_nogo)
            return (false);
        in_xname_off = read_unsigned64();
        if (in_nogo)
            return (false);
        if (in_fp->z_seek(posn, SEEK_SET) < 0) {
            Errs()->add_error("print_start: seek end failed.");
            in_nogo = true;
            return (false);
        }
    }
    put_char('\n');
    return (true);
}


bool
oas_in::print_end(unsigned int ix)
{
    // '2'  [table-offsets] padding-string validation-scheme
    //      [validation-signature]
    print_recname(ix);

    if (in_offset_flag) {
        for (int i = 0; i < 6; i++) {
            if (!print_unsigned(0))
                return (false);
            if (!print_unsigned64(0))
                return (false);
        }
    }

    read_b_string_nr();
    if (in_nogo)
        return (false);

    int ct = read_record_index(2);
    if (in_nogo)
        return (false);
    if (in_printing) {
        if (ct == 0)
            fprintf(in_print_fp, "  none");
        else if (ct == 1)
            fprintf(in_print_fp, "  crc32");
        else if (ct == 2)
            fprintf(in_print_fp, "  checksum32");
        else
            fprintf(in_print_fp, "  %u", ct);
    }
    if (ct) {
        unsigned int csum = read_byte();
        csum |= (read_byte() << 8);
        csum |= (read_byte() << 16);
        csum |= (read_byte() << 24);
        if (in_nogo)
            return (false);
        if (in_printing)
            fprintf(in_print_fp, " %u", csum);
    }
    put_char('\n');
    return (true);
}


bool
oas_in::print_name(unsigned int ix)
{
    check_offsets();
    print_recname(ix);

    if (!print_string(0, 0))
        return (false);

    put_char('\n');
    return (true);
}


bool
oas_in::print_name_r(unsigned int ix)
{
    check_offsets();
    print_recname(ix);

    if (!print_string(0, 0))
        return (false);

    if (!print_unsigned(0))
        return (false);

    put_char('\n');
    return (true);
}


bool
oas_in::print_layername(unsigned int ix)
{
    check_offsets();
    // `11' layername-string layer-interval datatype-interval
    // `12' layername-string textlayer-interval texttype-interval
    print_recname(ix);

    if (!print_string(0, 0))
        return (false);

    unsigned int l;
    if (!print_unsigned(&l))
        return (false);
    if (l > 0) {
        if (!print_unsigned(0))
            return (false);
    }
    if (l > 3) {
        if (!print_unsigned(0))
            return (false);
    }

    unsigned int d;
    if (!print_unsigned(&d))
        return (false);
    if (d > 0) {
        if (!print_unsigned(0))
            return (false);
    }
    if (d > 3) {
        if (!print_unsigned(0))
            return (false);
    }

    put_char('\n');
    return (true);
}


bool
oas_in::print_cell(unsigned int ix)
{
    // `13' reference-number
    // `14' cellname-string
    print_recname(ix);

    if (ix == 13) {
        if (!print_unsigned(0))
            return (false);
    }
    else {
        if (!print_string(0, 0))
            return (false);
    }
    put_char('\n');
    if (in_print_symcnt > 0)
        in_print_symcnt--;

    return (true);
}


bool
oas_in::print_placement(unsigned int ix)
{
    // `17' placement-info-byte [reference-number | cellname-string]
    //      [x] [y] [repetition]
    // `18' placement-info-byte [reference-number | cellname-string]
    //      [magnification] [angle] [x] [y] [repetition]
    print_recname(ix);

    unsigned int info_byte;  // CNXYRAAF or CNXYRMAF
    if (!print_info_byte(&info_byte))
        return (false);

    if (info_byte & 0x80) {
        if (info_byte & 0x40) {
            print_keyword(KeyRefnum);
            if (!print_unsigned(0))
                return (false);
        }
        else {
            print_keyword(KeyName);
            if (!print_string(0, 0))
                return (false);
        }
    }
    if (ix == 17) {
        int aa = (info_byte & 0x6) >> 1;
        if (aa != 0) {
            print_keyword(KeyAngle);
            put_space(1);
            print("%d", aa*90);
        }
    }
    else {
        if (info_byte & 0x4) {
            print_keyword(KeyMag);
            if (!print_real(0))
                return (false);
        }
        if (info_byte & 0x2) {
            print_keyword(KeyAngle);
            if (!print_real(0))
                return (false);
        }
    }
    if (info_byte & 0x1)
        print_keyword(KeyFlip);

    if (info_byte & 0x20) {
        print_keyword(KeyX);
        if (!print_signed(0))
            return (false);
    }
    if (info_byte & 0x10) {
        print_keyword(KeyY);
        if (!print_signed(0))
            return (false);
    }

    if (info_byte & 0x8) {
        if (!print_repetition())
            return (false);
    }

    put_char('\n');
    return (true);
}


bool
oas_in::print_text(unsigned int ix)
{
    // '19' text-info-byte [reference-number | text-string]
    //      [textlayer-number] [texttype-number] [x] [y] [repetition]
    print_recname(ix);

    unsigned int info_byte;  // 0CNXYRTL
    if (!print_info_byte(&info_byte))
        return (false);

    if (info_byte & 0x40) {
        if (info_byte & 0x20) {
            print_keyword(KeyRefnum);
            if (!print_unsigned(0))
                return (false);
        }
        else {
            print_keyword(KeyString);
            if (!print_string(0, 0))
                return (false);
        }
    }

    if (info_byte & 0x1) {
        print_keyword(KeyTextlayer);
        if (!print_unsigned(0))
            return (false);
    }
    if (info_byte & 0x2) {
        print_keyword(KeyTexttype);
        if (!print_unsigned(0))
            return (false);
    }

    if (info_byte & 0x10) {
        print_keyword(KeyX);
        if (!print_signed(0))
            return (false);
    }
    if (info_byte & 0x8) {
        print_keyword(KeyY);
        if (!print_signed(0))
            return (false);
    }

    if (info_byte & 0x4) {
        if (!print_repetition())
            return (false);
    }

    put_char('\n');
    return (true);
}


bool
oas_in::print_rectangle(unsigned int ix)
{
    // '20' rectangle-info-byte [layer-number] [datatype-number]
    //      [width] [height] [x] [y] [repetition]
    print_recname(ix);

    unsigned int info_byte;  // SWHXYRDL
    if (!print_info_byte(&info_byte))
        return (false);

    if (info_byte & 0x1) {
        print_keyword(KeyLayer);
        if (!print_unsigned(0))
            return (false);
    }
    if (info_byte & 0x2) {
        print_keyword(KeyDatatype);
        if (!print_unsigned(0))
            return (false);
    }

    if (info_byte & 0x40) {
        print_keyword(KeyWidth);
        if (!print_unsigned(0))
            return (false);
    }
    if (info_byte & 0x20) {
        print_keyword(KeyHeight);
        if (!print_unsigned(0))
            return (false);
    }

    if (info_byte & 0x10) {
        print_keyword(KeyX);
        if (!print_signed(0))
            return (false);
    }
    if (info_byte & 0x8) {
        print_keyword(KeyY);
        if (!print_signed(0))
            return (false);
    }

    if (info_byte & 0x4) {
        if (!print_repetition())
            return (false);
    }

    put_char('\n');
    return (true);
}


bool
oas_in::print_polygon(unsigned int ix)
{
    // '21' polygon-info-byte [layer-number] [datatype-number] [point-list]
    //      [x] [y] [repetition]
    print_recname(ix);

    unsigned int info_byte;  // 00PXYRDL
    if (!print_info_byte(&info_byte))
        return (false);

    if (info_byte & 0x1) {
        print_keyword(KeyLayer);
        if (!print_unsigned(0))
            return (false);
    }
    if (info_byte & 0x2) {
        print_keyword(KeyDatatype);
        if (!print_unsigned(0))
            return (false);
    }

    // Anuvad writes the point list after x,y records, so we will too.
    ptlist_t *pl = 0;
    if (info_byte & 0x20) {
        pl = read_pt_list();
        if (!pl)
            return (false);
    }

    if (info_byte & 0x10) {
        print_keyword(KeyX);
        if (!print_signed(0))
            return (false);
    }
    if (info_byte & 0x8) {
        print_keyword(KeyY);
        if (!print_signed(0))
            return (false);
    }

    if (info_byte & 0x20) {
        print_pt_list(pl);
        delete pl;
        if (in_nogo)
            return (false);
    }

    if (info_byte & 0x4) {
        if (!print_repetition())
            return (false);
    }

    put_char('\n');
    return (true);
}


bool
oas_in::print_path(unsigned int ix)
{
    // '22' path-info-byte [layer-number] [datatype-number] [half-width]
    //      [extension-scheme [start-exntension] [end-extension]]
    //      [point-list] [x] [y] [repetition]
    print_recname(ix);

    unsigned int info_byte;  // EWPXYRDL
    if (!print_info_byte(&info_byte))
        return (false);

    if (info_byte & 0x1) {
        print_keyword(KeyLayer);
        if (!print_unsigned(0))
            return (false);
    }
    if (info_byte & 0x2) {
        print_keyword(KeyDatatype);
        if (!print_unsigned(0))
            return (false);
    }

    if (info_byte & 0x40) {
        print_keyword(KeyHalfwidth);
        if (!print_unsigned(0))
            return (false);
    }

    if (info_byte & 0x80) {
        unsigned int extn = read_record_index(15);  // 0000SSEE
        if (in_nogo)
            return (false);
        unsigned int s_extn = (extn >> 2) & 0x3;
        if (s_extn) {
            print_keyword(KeyStartExtn);
            if (s_extn == 1) {
                put_space(1);
                print("%d", 0);
            }
            else if (s_extn == 2)
                print_keyword(KeyHalfwidth);
            else {
                if (!print_signed(0))
                    return (false);
            }
        }
        unsigned int e_extn = extn & 0x3;
        if (e_extn) {
            print_keyword(KeyEndExtn);
            if (e_extn == 1) {
                put_space(1);
                print("%d", 0);
            }
            else if (e_extn == 2)
                print_keyword(KeyHalfwidth);
            else {
                if (!print_signed(0))
                    return (false);
            }
        }
    }

    // Anuvad writes the point list after x,y records, so we will too.
    ptlist_t *pl = 0;
    if (info_byte & 0x20) {
        pl = read_pt_list();
        if (!pl)
            return (false);
    }

    if (info_byte & 0x10) {
        print_keyword(KeyX);
        if (!print_signed(0))
            return (false);
    }
    if (info_byte & 0x8) {
        print_keyword(KeyY);
        if (!print_signed(0))
            return (false);
    }

    if (info_byte & 0x20) {
        print_pt_list(pl);
        delete pl;
        if (in_nogo)
            return (false);
    }

    if (info_byte & 0x4) {
        if (!print_repetition())
            return (false);
    }

    put_char('\n');
    return (true);
}


bool
oas_in::print_trapezoid(unsigned int ix)
{
    // `23' trap-info-byte [layer-number] [datatype-number]
    //      [width] [height] delta-a delta-b [x] [y] [repetition]
    //
    // `24' trap-info-byte [layer-number] [datatype-number]
    //      [width] [height] delta-a [x] [y] [repetition]
    //
    // `25' trap-info-byte [layer-number] [datatype-number]
    //      [width] [height] delta-b [x] [y] [repetition]
    print_recname(ix);

    unsigned int info_byte;  // OWHXYRDL
    if (!print_info_byte(&info_byte))
        return (false);

    if (info_byte & 0x1) {
        print_keyword(KeyLayer);
        if (!print_unsigned(0))
            return (false);
    }
    if (info_byte & 0x2) {
        print_keyword(KeyDatatype);
        if (!print_unsigned(0))
            return (false);
    }
    if (info_byte & 0x80)
        print_keyword(KeyVertical);
    else
        print_keyword(KeyHorizontal);

    if (info_byte & 0x40) {
        print_keyword(KeyWidth);
        if (!print_unsigned(0))
            return (false);
    }
    if (info_byte & 0x20) {
        print_keyword(KeyHeight);
        if (!print_unsigned(0))
            return (false);
    }

    if (ix == 23 || ix == 24) {
        print_keyword(KeyDelta_A);
        if (!print_signed(0))
            return (false);
    }
    if (ix == 23 || ix == 25) {
        print_keyword(KeyDelta_B);
        if (!print_signed(0))
            return (false);
    }

    if (info_byte & 0x10) {
        print_keyword(KeyX);
        if (!print_signed(0))
            return (false);
    }
    if (info_byte & 0x8) {
        print_keyword(KeyY);
        if (!print_signed(0))
            return (false);
    }

    if (info_byte & 0x4) {
        if (!print_repetition())
            return (false);
    }

    put_char('\n');
    return (true);
}


bool
oas_in::print_ctrapezoid(unsigned int ix)
{
    // '26' ctrapezoid-info-byte [layer-number] [datatype-number]
    //      [ctrapezoid-type] [width] [height] [x] [y] [repetition]
    print_recname(ix);

    unsigned int info_byte;  // TWHXYRDL
    if (!print_info_byte(&info_byte))
        return (false);

    if (info_byte & 0x1) {
        print_keyword(KeyLayer);
        if (!print_unsigned(0))
            return (false);
    }
    if (info_byte & 0x2) {
        print_keyword(KeyDatatype);
        if (!print_unsigned(0))
            return (false);
    }

    if (info_byte & 0x80) {
        print_keyword(KeyTrapType);
        if (!print_unsigned(0))
            return (false);
    }

    if (info_byte & 0x40) {
        print_keyword(KeyWidth);
        if (!print_unsigned(0))
            return (false);
    }
    if (info_byte & 0x20) {
        print_keyword(KeyHeight);
        if (!print_unsigned(0))
            return (false);
    }

    if (info_byte & 0x10) {
        print_keyword(KeyX);
        if (!print_signed(0))
            return (false);
    }
    if (info_byte & 0x8) {
        print_keyword(KeyY);
        if (!print_signed(0))
            return (false);
    }

    if (info_byte & 0x4) {
        if (!print_repetition())
            return (false);
    }

    put_char('\n');
    return (true);
}


bool
oas_in::print_circle(unsigned int ix)
{
    // '27' circle-info-byte [layer-number] [datatype-number]
    //      [radius] [x] [y] [repetition]
    print_recname(ix);

    unsigned int info_byte;  // 00rXYRDL
    if (!print_info_byte(&info_byte))
        return (false);

    if (info_byte & 0x1) {
        print_keyword(KeyLayer);
        if (!print_unsigned(0))
            return (false);
    }
    if (info_byte & 0x2) {
        print_keyword(KeyDatatype);
        if (!print_unsigned(0))
            return (false);
    }

    if (info_byte & 0x20) {
        print_keyword(KeyRadius);
        if (!print_unsigned(0))
            return (false);
    }

    if (info_byte & 0x10) {
        print_keyword(KeyX);
        if (!print_signed(0))
            return (false);
    }
    if (info_byte & 0x8) {
        print_keyword(KeyY);
        if (!print_signed(0))
            return (false);
    }

    if (info_byte & 0x4) {
        if (!print_repetition())
            return (false);
    }

    put_char('\n');
    return (true);
}


bool
oas_in::print_property(unsigned int ix)
{
    // `28' prop-info-byte [reference-number | propname-string]
    //      [prop-value-count]  [ <property-value>* ]
    // `29'
    print_recname(ix);
    // This function is called with 28 only.

    unsigned int info_byte;  // UUUUVCNS
    if (!print_info_byte(&info_byte))
        return (false);

    if (info_byte & 0x4) {
        if (info_byte & 0x2) {
            print_keyword(KeyRefnum);
            if (!print_unsigned(0))
                return (false);
        }
        else {
            print_keyword(KeyName);
            if (!print_string(0, 0))
                return (false);
        }
    }

    if (info_byte & 1)
        print_keyword(KeyStandard);

    if (!(info_byte & 0x8)) {
        unsigned int n = info_byte >> 4;
        if (n == 15) {
            n = read_unsigned();
            if (in_nogo)
                return (false);
        }
        print_keyword(KeyValues);
        put_space(1);
        print("%d", n);
        for (unsigned int i = 0; i < n; i++) {
            if (!print_property_value())
                return (false);
        }
    }

    put_char('\n');
    return (true);
}


bool
oas_in::print_xname(unsigned int ix)
{
    check_offsets();
    // `30' xname-attribute xname-string
    // `31' xname-attribute xname-string reference-number
    print_recname(ix);

    if (!print_unsigned(0))
        return (false);

    if (!print_string(0, 0))
        return (false);

    if (ix == 31) {
        if (!print_unsigned(0))
            return (false);
    }

    put_char('\n');
    return (true);
}


bool
oas_in::print_xelement(unsigned int ix)
{
    // `32' xelement-attribute xelement-string
    print_recname(ix);

    if (!print_unsigned(0))
        return (false);

    if (!print_string(0, 0))
        return (false);

    put_char('\n');
    return (true);
}


bool
oas_in::print_xgeometry(unsigned int ix)
{
    // `33' xgeometry-info-byte xgeometry-attribute [layer-number]
    //      [datatype-number] xgeometry-string [x] [y] [repetition]
    print_recname(ix);

    unsigned int info_byte;  // 000XYRDL
    if (!print_info_byte(&info_byte))
        return (false);

    print_keyword(KeyAttribute);
    if (!print_unsigned(0))
        return (false);

    if (info_byte & 0x1) {
        print_keyword(KeyLayer);
        if (!print_unsigned(0))
            return (false);
    }
    if (info_byte & 0x2) {
        print_keyword(KeyDatatype);
        if (!print_unsigned(0))
            return (false);
    }

    print_keyword(KeyString);
    if (!print_string(0, 0))
        return (false);

    if (info_byte & 0x10) {
        print_keyword(KeyX);
        if (!print_signed(0))
            return (false);
    }
    if (info_byte & 0x8) {
        print_keyword(KeyY);
        if (!print_signed(0))
            return (false);
    }

    if (info_byte & 0x4) {
        if (!print_repetition())
            return (false);
    }

    put_char('\n');
    return (true);
}


bool
oas_in::print_cblock(unsigned int ix)
{
    if (in_zfile) {
        Errs()->add_error(
            "print_cblock: already decompressing, nested CBLOCK?");
        in_nogo = true;
        return (false);
    }
    check_offsets();

    // `34' comp-type uncomp-byte-count comp-byte-count comp-bytes
    print_recname(ix);

    unsigned int ctype;
    if (!print_unsigned(&ctype))
        return (false);
    if (ctype != 0) {
        Errs()->add_error("print_cblock: unknown compression-type %d.",
            ctype);
        in_nogo = true;
        return (false);
    }

    uint64_t uncomp;
    if (!print_unsigned64(&uncomp))
        return (false);

    uint64_t comp;
    if (!print_unsigned64(&comp))
        return (false);

    in_byte_offset = in_fp->z_tell();  // should already be equal
    in_offset = in_byte_offset;
    in_zfile = zio_stream::zio_open(in_fp, "r", comp);
    if (!in_zfile) {
        Errs()->add_error("print_cblock: open failed.");
        in_nogo = true;
        return (false);
    }
    in_compression_end = in_byte_offset + uncomp;
    in_next_offset = in_byte_offset + comp;

    put_char('\n');
    return (true);
}


void
oas_in::check_offsets()
{
    if (in_cellname_off || in_textstring_off || in_propname_off ||
            in_propstring_off || in_layername_off || in_xname_off) {
        uint64_t offset = in_fp->z_tell() - 1;

        if (in_cellname_off == offset) {
            print_recname(36);  // "CELLNAME_TABLE"
            put_char('\n');
            in_cellname_off = 0;
        }
        else if (in_textstring_off == offset) {
            print_recname(37);  // "TEXTSTRING_TABLE"
            put_char('\n');
            in_textstring_off = 0;
        }
        else if (in_propname_off == offset) {
            print_recname(38);  // "PROPNAME_TABLE"
            put_char('\n');
            in_propname_off = 0;
        }
        else if (in_propstring_off == offset) {
            print_recname(39);  // "PROPSTRING_TABLE"
            put_char('\n');
            in_propstring_off = 0;
        }
        else if (in_layername_off == offset) {
            print_recname(40);  // "LAYERNAME_TABLE"
            put_char('\n');
            in_layername_off = 0;
        }
        else if (in_xname_off == offset) {
            print_recname(41);  // "XNAME_TABLE"
            put_char('\n');
            in_xname_off = 0;
        }
    }
}


bool
oas_in::print_repetition()
{
    print_keyword(KeyRepetition);

    unsigned int type;
    if (!print_unsigned(&type))
        return (false);

    if (type == 0) {
        // reuse previous
        ;
    }
    else if (type == 1) {
        // x-dimension y-dimension x-space y-space
        if (!print_unsigned(0))
            return (false);
        if (!print_unsigned(0))
            return (false);
        if (!print_unsigned(0))
            return (false);
        if (!print_unsigned(0))
            return (false);
    }
    else if (type == 2 || type == 3) {
        // x-dimension x-space
        // y-dimension y-space
        if (!print_unsigned(0))
            return (false);
        if (!print_unsigned(0))
            return (false);
    }
    else if (type == 4 || type == 6) {
        // x-dimension      x-space_1 ... x-space_(n-1)
        // y-dimension      y-space_1 ... y-space_(n-1)
        unsigned int d;
        if (!print_unsigned(&d))
            return (false);
        for (unsigned int i = 0; i <= d; i++) {
            if (!print_unsigned(0))
                return (false);
        }
    }
    else if (type == 5 || type == 7) {
        // x-dimension grid x-space_1 ... x-space_(n-1)
        // y-dimension grid y-space_1 ... y-space_(n-1)
        unsigned int d;
        if (!print_unsigned(&d))
            return (false);
        if (!print_unsigned(0))
            return (false);
        for (unsigned int i = 0; i <= d; i++) {
            if (!print_unsigned(0))
                return (false);
        }
    }
    else if (type == 8) {
        // n-dimension m-dimension n-displacement m-displacement
        if (!print_unsigned(0))
            return (false);
        if (!print_unsigned(0))
            return (false);
        if (!print_g_delta(0, 0))
            return (false);
        if (!print_g_delta(0, 0))
            return (false);
    }
    else if (type == 9) {
        // dimension displacement
        if (!print_unsigned(0))
            return (false);
        if (!print_g_delta(0, 0))
            return (false);
    }
    else if (type == 10) {
        // dimension      displacement_1 ... displacement_(n-1)
        unsigned int d;
        if (!print_unsigned(&d))
            return (false);
        for (unsigned int i = 0; i <= d; i++) {
            if (!print_g_delta(0, 0))
                return (false);
        }
    }
    else if (type == 11) {
        // Type 11:  dimension grid displacement_1 ... displacement_(n-1)
        unsigned int d;
        if (!print_unsigned(&d))
            return (false);
        if (!print_unsigned(0))
            return (false);
        for (unsigned int i = 0; i <= d; i++) {
            if (!print_g_delta(0, 0))
                return (false);
        }
    }
    else
        return (false);
    return (true);
}


ptlist_t *
oas_in::read_pt_list()
{
    ptlist_t *pl = new ptlist_t;

    pl->type = read_unsigned();
    if (in_nogo) {
        delete pl;
        return (0);
    }
    pl->size = read_unsigned();
    if (in_nogo) {
        delete pl;
        return (0);
    }
    if (pl->type == 0 || pl->type == 1) {
        pl->values = new int[pl->size];
        for (unsigned int i = 0; i < pl->size; i++) {
            pl->values[i] = read_signed();
            if (in_nogo) {
                delete pl;
                return (0);
            }
        }
    }
    else if (pl->type == 2) {
        pl->values = new int[2*pl->size];
        int j = 0;
        for (unsigned int i = 0; i < pl->size; i++) {
            pl->values[j+1] = read_delta(2, &pl->values[j]);
            if (in_nogo) {
                delete pl;
                return (0);
            }
            j += 2;
        }
    }
    else if (pl->type == 3) {
        pl->values = new int[2*pl->size];
        int j = 0;
        for (unsigned int i = 0; i < pl->size; i++) {
            pl->values[j+1] = read_delta(3, &pl->values[j]);
            if (in_nogo) {
                delete pl;
                return (0);
            }
            j += 2;
        }
    }
    else if (pl->type == 4 || pl->type == 5) {
        pl->values = new int[2*pl->size];
        int j = 0;
        for (unsigned int i = 0; i < pl->size; i++) {
            if (!read_g_delta(&pl->values[j], &pl->values[j+1])) {
                delete pl;
                return (0);
            }
            j += 2;
        }
    }
    else {
        delete pl;
        return (0);
    }
    return (pl);
}


void
oas_in::print_pt_list(ptlist_t *pl)
{
    static const char *const compass[] =
    {
        "e", "n", "w", "s", "ne", "nw", "sw", "se"
    };
    print_keyword(KeyPointList);
    put_space(1);
    print("%u", pl->type);
    put_space(1);
    print("%u", pl->size);

    if (pl->type == 0 || pl->type == 1) {
        for (unsigned int i = 0; i < pl->size; i++) {
            put_space(1);
            print("%d", pl->values[i]);
        }
    }
    else if (pl->type == 2) {
        int j = 0;
        for (unsigned int i = 0; i < pl->size; i++) {
            put_space(1);
            print("%s:%lu", compass[pl->values[j]], pl->values[j+1]);
            j += 2;
        }
    }
    else if (pl->type == 3) {
        int j = 0;
        for (unsigned int i = 0; i < pl->size; i++) {
            put_space(1);
            print("%s:%lu", compass[pl->values[j]], pl->values[j+1]);
            j += 2;
        }
    }
    else if (pl->type == 4 || pl->type == 5) {
        int j = 0;
        for (unsigned int i = 0; i < pl->size; i++) {
            put_space(1);
            print("(%d,%d)", pl->values[j], pl->values[j+1]);
            j += 2;
        }
    }
}


bool
oas_in::print_property_value()
{
    unsigned int type;
    if (!print_unsigned(&type))
        return (false);

    if (type <= 7) {
        if (!print_real(0))
            return (false);
    }
    else if (type == 8) {
        if (!print_unsigned(0))
            return (false);
    }
    else if (type == 9) {
        if (!print_signed(0))
            return (false);
    }
    else if (type == 10 || type == 11 || type == 12) {
        if (!print_string(0, 0))
            return (false);
    }
    else if (type == 13 || type == 14 || type == 15) {
        if (!print_unsigned(0))
            return (false);
    }
    else
        return (false);
    return (true);
}


void
oas_in::print_recname(unsigned int ix)
{
    // Skip a line ahead of CELL records and name tables.
    if (ix == 13 || ix == 14 || (ix >= 36 && ix <= 41))
        put_char('\n');

    if (in_print_offset) {
        if (in_zfile)
            print("[%llu:%06llu]  ",  (unsigned long long)in_offset,
                (unsigned long long)(in_byte_offset - 1 - in_offset));
        else {
            int64_t offset = in_fp->z_tell() - 1;
            print("[%07llu]  ",  (unsigned long long)offset);
        }
    }

    in_print_start_col = in_print_cur_col;
    if (ix <= 41)
        print("%s", oasisRecordNames[ix]);
    else
        print("%s", "");
}


void
oas_in::print_keyword(int keyword)
{
    put_space(2);
    print("%s", asciiKeywords[keyword]);
}


bool
oas_in::print_info_byte(unsigned int *pv)
{
    put_space(1);
    unsigned int info_byte = read_byte();
    if (in_nogo)
        return (false);

    if (info_byte == 0)
        put_string("0x00", 4);  // glibc printf bug
    else
        print("%#.2x", info_byte);
    if (pv)
        *pv = info_byte;
    return (true);
}


bool
oas_in::print_unsigned(unsigned int *pret)
{
    put_space(1);
    unsigned int val = read_unsigned();
    if (in_nogo)
        return (false);
    print("%u", val);
    if (pret)
        *pret = val;
    return (true);
}


bool
oas_in::print_unsigned64(uint64_t *pret)
{
    put_space(1);
    uint64_t val = read_unsigned64();
    if (in_nogo)
        return (false);
    print("%llu", val);
    if (pret)
        *pret = val;
    return (true);
}


bool
oas_in::print_signed(int *pret)
{
    put_space(1);
    int val = read_signed();
    if (in_nogo)
        return (false);
    print("%d", val);
    if (pret)
        *pret = val;
    return (true);
}


bool
oas_in::print_real(double *pval)
{
    put_space(1);
    unsigned int type = read_unsigned();
    if (in_nogo)
        return (false);
    if (type == 0) {
        unsigned int val = read_unsigned();
        if (in_nogo)
            return (false);
        print("%d", val);
        if (pval)
            *pval = val;
    }
    else if (type == 1) {
        unsigned int val = read_unsigned();
        if (in_nogo)
            return (false);
        print("-%d", val);
        if (pval)
            *pval = -val;
    }
    else if (type == 2) {
        unsigned int val = read_unsigned();
        if (in_nogo)
            return (false);
        print("1/%d", val);
        if (pval)
            *pval = 1.0/val;
    }
    else if (type == 3) {
        unsigned int val = read_unsigned();
        if (in_nogo)
            return (false);
        print("-1/%d", val);
        if (pval)
            *pval = -1.0/val;
    }
    else if (type == 4) {
        unsigned int val1 = read_unsigned();
        if (in_nogo)
            return (false);
        unsigned int val2 = read_unsigned();
        if (in_nogo)
            return (false);
        print("%d/%d", val1, val2);
        if (pval)
            *pval = ((double)val1)/val2;
    }
    else if (type == 5) {
        unsigned int val1 = read_unsigned();
        if (in_nogo)
            return (false);
        unsigned int val2 = read_unsigned();
        if (in_nogo)
            return (false);
        print("-%d/%d", val1, val2);
        if (pval)
            *pval = -((double)val1)/val2;
    }
    else if (type == 6 || type == 7) {
        double val;
        int prec;
        if (type == 6) {
            // ieee float
            union { float n; unsigned char b[4]; } uu;
            uu.b[0] = read_byte();
            if (in_nogo)
                return (false);
            uu.b[1] = read_byte();
            if (in_nogo)
                return (false);
            uu.b[2] = read_byte();
            if (in_nogo)
                return (false);
            uu.b[3] = read_byte();
            if (in_nogo)
                return (false);
            val = uu.n;
            prec = 7;
        }
        else {
            // ieee double
            union { double n; unsigned char b[8]; } uu;
            uu.b[0] = read_byte();
            if (in_nogo)
                return (false);
            uu.b[1] = read_byte();
            if (in_nogo)
                return (false);
            uu.b[2] = read_byte();
            if (in_nogo)
                return (false);
            uu.b[3] = read_byte();
            if (in_nogo)
                return (false);
            uu.b[4] = read_byte();
            if (in_nogo)
                return (false);
            uu.b[5] = read_byte();
            if (in_nogo)
                return (false);
            uu.b[6] = read_byte();
            if (in_nogo)
                return (false);
            uu.b[7] = read_byte();
            if (in_nogo)
                return (false);
            val = uu.n;
            prec = 15;
        }
        char buf[50];
        unsigned int n = snprintf(buf, sizeof(buf)-4, "%.*g", prec, val);
        if (!strpbrk(buf, ".e")) {
            buf[n++] = '.';
            buf[n++] = '0';
        }
        if (prec == 7)
            buf[n++] = 'f';
        buf[n] = 0;
        put_string(buf, n);
        if (pval)
            *pval = val;
    }
    else
        return (false);
    return (true);
}


bool
oas_in::print_string(char **pstr, unsigned int *plen)
{
    unsigned int len = 0;
    char *tstr = read_b_string(&len);
    if (in_nogo)
        return (false);
    GCarray<char*> gc_tstr(tstr);

    const char *segStart = tstr;    // start of current segment
    const char *end = tstr + len;   // one past the end of str
    const char *cp;

    put_space(1);

    // Most strings will contain only printable characters.  So instead
    // of printing char by char, we treat the string as segments of
    // printable chars separated by chars that must be escaped.

    put_char('"');
    for (cp = tstr; cp != end;  ++cp) {
        unsigned char uch = static_cast<unsigned char>(*cp);

        // Ordinary character
        if (isprint(uch)  &&  uch != '"'  &&  uch != '\\')
            continue;

        // Current char must be escaped.  Print the previous segment of
        // ordinary chars and start the next segment after this char.

        if (cp != segStart)
            put_string(segStart, cp - segStart);
        segStart = cp+1;

        // Escape " and \, and print a non-printing (control) char as \ooo
        // where ooo is the octal code for the char.

        const char*  fmt = (isprint(uch) ? "\\%c" : "\\%03o");
        print(fmt, uch);
    }
    if (cp != segStart)
        put_string(segStart, cp - segStart);
    put_char('"');
    if (in_nogo)
        return (false);

    if (pstr) {
        *pstr = tstr;
        gc_tstr.clear();
    }
    if (plen)
        *plen = len;

    return (true);
}


bool
oas_in::print_g_delta(int *pmx, int *pmy)
{
    put_space(1);
    int mx, my;
    if (!read_g_delta(&mx, &my))
        return (false);
    print("(%d,%d)", mx, my);
    if (pmx)
        *pmx = mx;
    if (pmy)
        *pmy = my;
    return (true);
}


void
oas_in::print(const char* fmt, ...)
{
    if (!in_printing)
        return;
    va_list  ap;
    va_start(ap, fmt);
    int  n = vfprintf(in_print_fp, fmt, ap);
    va_end(ap);

    if (n < 0)
        in_nogo = true;
    in_print_cur_col += n;
}


void
oas_in::put_string(const char* str, size_t len)
{
    if (!in_printing)
        return;
    if (fwrite(str, 1, len, in_print_fp) != len)
        in_nogo = true;
    in_print_cur_col += len;
}


void
oas_in::put_char(char ch)
{
    if (!in_printing)
        return;
    if (putc(ch, in_print_fp) == EOF)
        in_nogo = true;
    if (ch == '\n')
        in_print_cur_col = 1;
    else
        in_print_cur_col++;
}


void
oas_in::put_space(unsigned int nspaces)
{
    if (!in_printing)
        return;
    if (in_print_break_lines &&
            in_print_cur_col + nspaces > RIGHT_MARGIN) {
        put_char('\n');
        nspaces = in_print_start_col + INDENTATION - 1;
    }
    while (nspaces--)
        put_char(' ');
}


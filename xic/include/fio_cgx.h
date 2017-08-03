
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

#ifndef FIO_CGX_H
#define FIO_CGX_H

#include "fio_cvt_base.h"


//  Compact Graphical Exchange (CGX) File Format
//  --------------------------------------------
//
// This is a simple binary data format somewhat similar to GDSII, but
// designed to be more compact.  The first four bytes of a cgx file
// are 'c', 'g', 'x', '\0'.  There are 11 defined record types.  Each
// record begins with a 4-byte header:
//
//    record header: size[2] | rectype | flags
//
// The size field is the total record size, including the header.  The
// rectype is one of types below.  The flags field is used in some of
// the record types, otherwise it is ignored.
//
// The double, long (4-byte), and short (2-byte) formats are the same
// as GDSII, as is the string format.  The date/time record is similar,
// but more compact:
//   short year
//   char  mon + 1
//   char  mday
//   char  hour
//   char  min
//   char  sec
//   char  0
//
// Strings are written without the null terminator, unless the length
// is odd in which case a null byte is added to make the record length
// even (all records consist of an even number of bytes).
//
// There are no size limits other than the block size.
//
//    "cgx\0"       file id string, first 4 bytes not in any record
//
//  LIBRARY (flags: format version number)
//    munit         double machine units
//    uunit         double user units
//    cdate         library creation date, 8 bytes
//    mdate         library modification date, 8 bytes
//    string        library name
//
//  The LIBRARY record is the fiirst record of the file.  It should appear
//  only once.
//
//  STRUCT
//    cdate         creation date, 8 bytes
//    mdate         modification date, 8 bytes
//    name          structure name
//
// The STRUCT record opens a cell.  Records that follow will be
// assigned to that cell, until another STRUCT record is seen.
//
//  CPRPTY
//    number        long property number
//    string        property string
//
//  Zero or more CPRPTY records can appear following STRUCT.  These
//  are properties that are applied to the cell.
//
//  PROPERTY
//    number        long property number
//    string        property string
//
//  Zero or more PROPERTY records can appear ahead of BOX, POLY,
//  WIRE, TEXT, and SREF records.  If assigns a property to the object
//  that follows.
//
//  LAYER
//    number        short layer number
//    data_type     short data type
//    [string]      optional layer name
//
// A layer record can appear after a STRUCT, and must appear before
// any of BOX, POLY, WIRE, TEXT in the STRUCT.  The layer context will
// persist until the next LAYER or STRUCT record.
//
//  BOX
//    left          long left value
//    bottom        long bottom value
//    right         long right value
//    top           long top value
//    [repeat]      repeat for multiple boxes
//
// A BOX record can appear after a LAYER record.  The BOX record defines
// one or more rectangular data objects.
//
//  POLY
//    xy ...        long coordinate pairs, path must be closed
//
// A POLY record specifies a single polygon data object.
//
//  WIRE (flags: end style)
//     width        long path width
//     xy ...       long coordinate pairs (1 pair or more)
//
// A WIRE record specifies a single wire data object.  The GDSII end style
// code is stored in the flags field.
//
//  TEXT (flags: xform)
//     x            long x position
//     y            long y position
//     width        long field width
//     string       label text
//
// A TEXT record describes a logical text object.  The width gives the
// physical equivalent width of the text.  The height is undefined,
// but is determined by the text used for rendering.  The flags field
// contains the presentation code.
//
//  SREF (flags: see description)
//     x            long x coordinate
//     y            long y coordinate
//     angle        double, if RF_ANGLE flag only
//     magnif       double, if RF_MAGN flag only
//     cols         long array columns, if RF_ARRAY flag only
//     rows         long array rows, if RF_ARRAY flag only
//     xy[4]        long aref points (like GDSII), if RF_ARRAY flag only
//     string
//
// The SREF record describes an instance, or an array of instances.
// The data appear if needed, as indicated by a flag.  THere are also
// flags that indicate reflection, absolute magnification, and
// absolute angle.
//
//  ENDLIB
//
// The ENDLIB record is the last record of the file.  It contains no
// data.

// Record types
//
enum RecType
{
    R_LIBRARY,
    R_STRUCT,
    R_CPRPTY,
    R_PROPERTY,
    R_LAYER,
    R_BOX,
    R_POLY,
    R_WIRE,
    R_TEXT,
    R_SREF,
    R_ENDLIB,
    R_SKINFO
};

// Number of records in this table
#define CGX_NUM_REC_TYPES 12

// Flags for R_SREF
//
#define RF_ANGLE    0x1
#define RF_MAGN     0x2
#define RF_REFLECT  0x4
#define RF_ARRAY    0x8
#define RF_ABSMAG   0x10
#define RF_ABSANG   0x20

// In a hack to save the show/hide status of property labels in Xic,
// we hide a flags byte following the text label string. 
//
#define CGX_LA_SHOW 0x1
#define CGX_LA_HIDE 0x2
#define CGX_LA_TLEV 0x4
#define CGX_LA_LIML 0x8


// Information storage
//
struct cgx_info : public cv_info
{
    cgx_info(bool per_lyr, bool per_cell) : cv_info(per_lyr, per_cell)
        { memset(rec_counts, 0, CGX_NUM_REC_TYPES*sizeof(int)); }

    // virtual overrides
    void initialize()
        {
            memset(rec_counts, 0, CGX_NUM_REC_TYPES*sizeof(int));
            cv_info::initialize();
        }
    char *pr_records(FILE*);
    void add_record(int);

private:
    unsigned int    rec_counts[CGX_NUM_REC_TYPES];
};

// Header information, used with archive context.
//
struct cgx_header_info : public cv_header_info
{
    cgx_header_info(double u) : cv_header_info(Fcgx, u) { }
};


// Class for reading CGX files
//
struct cgx_in : public cv_in
{
    cgx_in(bool);  // arg allows layer mapping if true
    virtual ~cgx_in();

    // setup functions
    bool setup_source(const char*, const cCHD* = 0);
    bool setup_to_database();
    bool setup_destination(const char*, FileType, bool);
    bool setup_ascii_out(const char*, uint64_t=0, uint64_t=0, int=-1, int=-1);
    bool setup_backend(cv_out*);

    // main entry for reading
    bool parse(DisplayMode, bool, double, bool = false, cvINFO = cvINFOtotals);

    // entries for reading through cell hierarchy digest
    bool chd_read_header(double);
    bool chd_read_cell(symref_t*, bool, CDs** = 0);
    cv_header_info *chd_get_header_info();
    void chd_set_header_info(cv_header_info*);

    // misc. entries
    bool has_electrical();
    FileType file_type() { return (Fcgx); }
    void add_header_props(CDs*) { }
    bool has_header_props() { return (false); }
    OItype has_geom(symref_t*, const BBox* = 0);
    // end of virtual overrides

private:
    bool read_header(bool);
    bool end_struct();

    bool a_struct(int, int);
    bool a_cprpty(int, int);
    bool a_property(int, int);
    bool a_layer(int, int);
    bool a_box(int, int);
    bool a_poly(int, int);
    bool a_wire(int, int);
    bool a_text(int, int);
    bool a_sref(int, int);
    bool a_endlib(int, int);

    bool ac_struct(int, int);
    bool ac_cprpty(int, int);
    bool ac_property(int, int);
    bool ac_layer(int, int);
    bool ac_box(int, int);
    bool ac_box_prv(BBox&);
    bool ac_poly(int, int);
    bool ac_poly_prv(Poly&);
    bool ac_wire(int, int);
    bool ac_wire_prv(Wire&);
    bool ac_text(int, int);
    bool ac_text_prv(const Text&);
    bool ac_sref(int, int);
    bool ac_sref_backend(Instance*, symref_t*, bool);
    bool ac_endlib(int, int);

    bool ap_struct(int, int);
    bool ap_cprpty(int, int);
    bool ap_property(int, int);
    bool ap_layer(int, int);
    bool ap_box(int, int);
    bool ap_poly(int, int);
    bool ap_wire(int, int);
    bool ap_text(int, int);
    bool ap_sref(int, int);
    bool ap_endlib(int, int);

    void add_properties_db(CDo*);
    void clear_properties();
    void warning(const char*);
    void warning(const char*, int, int);
    void warning(const char*, const char*, int, int);
    bool get_record(int*, int*, int*);
    int short_value(char**);
    int long_value(char**);
    double double_value(char**);
    void date_value(tm*, char**);
    void pr_record(const char*, int, int);

    double      in_munit;           // scale factor
    bool        in_defsym;          // the "begin symbol" code was sent
    bool        in_skip;            // skip object conversion
    bool        in_no_create_layer; // don't create new layers
    bool        in_has_cprops;      // cell properties are queued
    FilePtr     in_fp;              // source file pointer
    CDl         *in_curlayer;       // current layer
    char        *in_cellname;       // current cell name
    char        *in_layername;      // current layer name
    bool(cgx_in::*ftab[R_ENDLIB+1])(int, int); // dispatch table
    char        data_buf[65536];    // block buffer
};

// Class for generating CGX output
//
struct cgx_out : public cv_out
{
    cgx_out();
    ~cgx_out();

    bool check_for_interrupt();
    bool flush_cache();
    bool write_header(const CDs*);
    bool write_object(const CDo*, cvLchk*);
    bool set_destination(const char*);
    bool set_destination(FILE *fp, void**, void**)
        {
            if (!out_fp) {
                out_fp = new sFilePtr(0, fp);
                return (true);
            }
            if (!out_fp->file) {
                out_fp->fp = fp;
                return (true);
            }
            return (false);
        }
    bool open_library(DisplayMode, double);
    bool queue_property(int, const char*);
    bool write_library(int, double, double, tm*, tm*, const char*);
    bool write_struct(const char*, tm*, tm*);
    bool write_end_struct(bool = false);
    bool queue_layer(const Layer*, bool* = 0);
    bool write_box(const BBox*);
    bool write_poly(const Poly*);
    bool write_wire(const Wire*);
    bool write_text(const Text*);
    bool write_sref(const Instance*);
    bool write_endlib(const char*);
    bool write_info(Attribute*, const char*);
    // end of virtual overrides

private:

    void begin_record(int count, int type, int flags)
        {
            out_buffer[out_bufcnt++] = count >> 8;
            out_buffer[out_bufcnt++] = count;
            out_buffer[out_bufcnt++] = type;
            out_buffer[out_bufcnt++] = flags;
            out_rec_count++;
            out_byte_count += count;
        }

    void short_copy(int i)
        {
            out_buffer[out_bufcnt++] = i >> 8;
            out_buffer[out_bufcnt++] = i;
        }

    void long_copy(int i)
        {
            out_buffer[out_bufcnt++] = i >> 24;
            out_buffer[out_bufcnt++] = i >> 16;
            out_buffer[out_bufcnt++] = i >> 8;
            out_buffer[out_bufcnt++] = i;
        }

    void date_copy(struct tm*);
    void dbl_copy(double);
    bool flush_buf();

    bool write_layer_rec();
    bool write_cprpty_rec(int, const char*);
    bool write_property_rec(int, const char*);
    bool write_ascii_rec(const char*);

    bool        out_layer_written;          // layer record was written
    char        *out_lname_temp;            // layer name copy

#define CGX_BUFSIZ 1024
    unsigned char out_buffer[CGX_BUFSIZ];   // output buffer
    int         out_bufcnt;                 // output buffer bytes used

// Max number of boxes to aggregate
#define CGX_BOX_MAX 4000

    FilePtr     out_fp;                     // output file pointer
    int         out_boxcnt;                 // number of boxes in cache
    BBox        out_boxbuf[CGX_BOX_MAX];    // box cache
};

#endif


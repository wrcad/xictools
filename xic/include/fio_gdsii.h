
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

#ifndef FIO_GDSII_H
#define FIO_GDSII_H

#include "fio_cvt_base.h"
#include "fio_gds_layer.h"


// Library name used in created gds files.
#define GDS_CD_LIBNAME "XicTools.db"

// Limits from Cadence Design Data Translators Reference, version 4.4.6

// There is "no limit" except that the size is given by an unsigned
// 2-byte integer.
#define GDS_MAX_REC_SIZE    65536

// If the stream file is on magnetic tape, the records of the library
// are usually divided in 2048 byte physical blocks.  The file is null
// padded to a physical block boundary.
#define GDS_PHYS_REC_SIZE   2048

// Maximum string length is 126 (null pad odd length).
// GDSII release < 7 only.
#define GDS_MAX_PRPSTR_LEN  126

// Library names are 44 chars max, null padded to width 44.
#define GDS_MAX_LIBNAM_LEN  44

// Structure names are 32 characters maximum, a-z, A-Z, 0-9, _?$ only.
// Length limit applies to GDSII release < 7 only.
#define GDS_MAX_STRNAM_LEN  32

// Text length up to 512 characters.
// Length limit applies to GDSII release < 7 only.
#define GDS_MAX_TEXT_LEN    512

#ifdef M_PI
#define RADTODEG (180.0/M_PI)
#else
#define RADTODEG 57.2957795130823228646
#endif

// Text for the dummy label used to save cell properties in physical
// mode.  THIS IS OBSOLETE (but kept for backwards compatibility).
#define GDS_CELL_PROPERTY_MAGIC "CELL PROPERTIES"

// We save physical cell properties on a snapnode, on layer/nodetype 0/0,
// with this plex number.
#define XIC_NODE_PLEXNUM 0x01ffffff
#define GDS_USE_NODE_PRPS

// The following are definitions of CALMA Stream elements
// that would otherwise be noted by number.

#define II_HEADER        0
#define II_BGNLIB        1
#define II_LIBNAME       2
#define II_UNITS         3
#define II_ENDLIB        4
#define II_BGNSTR        5
#define II_STRNAME       6
#define II_ENDSTR        7
#define II_BOUNDARY      8
#define II_PATH          9
#define II_SREF         10
#define II_AREF         11
#define II_TEXT         12
#define II_LAYER        13
#define II_DATATYPE     14
#define II_WIDTH        15
#define II_XY           16
#define II_ENDEL        17
#define II_SNAME        18
#define II_COLROW       19
#define II_TEXTNODE     20
#define II_SNAPNODE     21
#define II_TEXTTYPE     22
#define II_PRESENTATION 23
#define II_SPACING      24
#define II_STRING       25
#define II_STRANS       26
#define II_MAG          27
#define II_ANGLE        28
#define II_UINTEGER     29
#define II_USTRING      30
#define II_REFLIBS      31
#define II_FONTS        32
#define II_PATHTYPE     33
#define II_GENERATIONS  34
#define II_ATTRTABLE    35
#define II_STYPTABLE    36
#define II_STRTYPE      37
#define II_ELFLAGS      38
#define II_ELKEY        39
#define II_LINKTYPE     40
#define II_LINKKEYS     41
#define II_NODETYPE     42
#define II_PROPATTR     43
#define II_PROPVALUE    44

// R5 options
#define II_BOX          45
#define II_BOXTYPE      46
#define II_PLEX         47
#define II_BGNEXTN      48
#define II_ENDEXTN      49
#define II_TAPENUM      50
#define II_TAPECODE     51
#define II_STRCLASS     52
// reserved             53
#define II_FORMAT       54
#define II_MASK         55
#define II_ENDMASKS     56
#define II_LIBDIRSIZE   57
#define II_SRFNAME      58
#define II_LIBSECUR     59
#define II_BORDER       60
#define II_SOFTFENCE    61
#define II_HARDFENCE    62
#define II_SOFTWIRE     63
#define II_HARDWIRE     64
#define II_PATHPORT     65
#define II_NODEPORT     66
#define II_USERCONSTRAINT 67
#define II_SPACER_ERROR 68
#define II_CONTACT      69

// Phony record return.
#define II_EOF          -1

// Size of this table (doesn't include phony record).
#define GDS_NUM_REC_TYPES    70


// Information storage.
//
struct gds_info : public cv_info
{
    gds_info(bool per_lyr, bool per_cell) : cv_info(per_lyr, per_cell)
        { memset(rec_counts, 0, GDS_NUM_REC_TYPES*sizeof(int)); }

    // virtual overrides
    void initialize()
        {
            memset(rec_counts, 0, GDS_NUM_REC_TYPES*sizeof(int));
            cv_info::initialize();
        }
    void add_record(int);
    char *pr_records(FILE*);

    unsigned int    rec_counts[GDS_NUM_REC_TYPES];
};


#if defined(__i386__) || defined(__x86_64__)

inline unsigned int
byte_rev_32(unsigned int x)
{
    __asm("bswap   %0": "=r" (x) : "0" (x));
    return x;
}

#endif

// Header information, used with archive context.
//
struct gds_header_info : public cv_header_info
{
    gds_header_info(double u) : cv_header_info(Fgds, u) { }
};


// Return type for check_layer().
enum cl_type { cl_error, cl_ok, cl_skip };

// Class for reading GDSII input.
//
struct gds_in : public cv_in
{
public:
    gds_in(bool);  // arg allows layer mapping if true
    virtual ~gds_in();

    static bool check_file(FilePtr, int*, bool*);
    static bool is_gds(char*, int*);

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
    FileType file_type() { return (Fgds); }
    void add_header_props(CDs*);
    bool has_header_props() { return (in_sprops != 0); }
    OItype has_geom(symref_t*, const BBox* = 0);
    // end of virtual overrides

    void log_version();
    bool write_gds_header_props();
    void check_ptype4();

private:
    inline bool gds_read(void*, unsigned int);
    inline bool gds_seek(int64_t);
    inline int64_t gds_tell();

    // Stream format is big-endian.
    int shortval(char *p)
        {
            int i = (signed char)p[0];
            return ((i << 8) | (unsigned char)p[1]);
        }

    int short_value(char **p)
        {
            char *b = *p;
            (*p) += 2;
            return (shortval(b));
        }

    int longval(char *p)
        {
#if defined(__i386__) || defined(__x86_64__)
            return ((int)byte_rev_32(*(unsigned int*)p));
#else
            int i = (signed char)p[0];
            unsigned char *b = (unsigned char*)p;
            return ((i << 24) | (b[1] << 16) | (b[2] << 8) | b[3]);
#endif
        }

    int long_value(char **p)
        {
            char *b = *p;
            (*p) += 4;
            return (longval(b));
        }

    void reset_layer_check()
        {
            in_layer = -1;
            in_dtype = -1;
            in_curlayer = -1;
            in_curdtype = -1;
        }

    bool read_header(bool);
    bool read_data();
    bool read_element();
    bool read_text_property();
    bool get_record();
    void fatal_error();
    void warning(const char*);
    void warning(const char*, int, int);
    void warning(const char*, const char*, int, int);
    void date_value(tm*, char**);
    double doubleval(char*);

    cl_type check_layer(bool* = 0, bool = true);
    cl_type check_layer_cvt();
    void add_properties_db(CDs*, CDo*);

    bool unsup();
    bool nop();

    bool a_header();
    bool a_bgnlib();
    bool a_libname();
    bool a_units();
    bool a_bgnstr();
    bool a_strname();
    bool a_boundary();
    bool a_path();
    bool a_sref();
    bool a_aref();
    bool a_instance(bool);
    bool a_text();
    bool a_layer();
    bool a_datatype();
    bool a_width();
    bool a_xy();
    bool a_sname();
    bool a_colrow();
    bool a_snapnode();
    bool a_presentation();
    bool a_string();
    bool a_strans();
    bool a_mag();
    bool a_angle();
    bool a_reflibs();
    bool a_fonts();
    bool a_pathtype();
    bool a_generations();
    bool a_attrtable();
    bool a_propattr();
    bool a_propvalue();
    bool a_box();
    bool a_plex();
    bool a_bgnextn();
    bool a_endextn();

    bool ac_bgnstr();
    bool ac_boundary();
    bool ac_boundary_prv();
    bool ac_path();
    bool ac_path_prv();
    bool ac_sref();
    bool ac_aref();
    bool ac_instance(bool);
    bool ac_instance_backend(Instance*, symref_t*, bool);
    bool ac_text();
    bool ac_text_core();
    bool ac_text_prv(const Text*);
    bool ac_snapnode();
    bool ac_propvalue();
    bool ac_box();

    void clear_properties();
    void print_offset();
    void print_space(int);
    void print_int(const char*, int, bool);
    void print_int2(const char*, int, int, bool);
    void print_float(const char*, double, bool);
    void print_word(const char*, bool);
    void print_word2(const char*, const char*, bool);
    void print_word_end(const char*);
    void print_date(const char*, tm*, bool);

    bool ap_header();
    bool ap_bgnlib();
    bool ap_libname();
    bool ap_units();
    bool ap_bgnstr();
    bool ap_strname();
    bool ap_boundary();
    bool ap_path();
    bool ap_sref();
    bool ap_aref();
    bool ap_text();
    bool ap_layer();
    bool ap_datatype();
    bool ap_width();
    bool ap_xy();
    bool ap_sname();
    bool ap_colrow();
    bool ap_snapnode();
    bool ap_texttype();
    bool ap_presentation();
    bool ap_string();
    bool ap_strans();
    bool ap_mag();
    bool ap_angle();
    bool ap_reflibs();
    bool ap_fonts();
    bool ap_pathtype();
    bool ap_generations();
    bool ap_attrtable();
    bool ap_nodetype();
    bool ap_propattr();
    bool ap_propvalue();
    bool ap_box();
    bool ap_boxtype();
    bool ap_plex();
    bool ap_bgnextn();
    bool ap_endextn();

    double      in_magn;                // magnification
    double      in_angle;               // rotation angle
    double      in_munit;               // file m-units value
    uint64_t    in_obj_offset;          // offset of current object
    uint64_t    in_attr_offset;         // attribute file offset
    uint64_t    in_offset_next;         // start of next record
    FilePtr     in_fp;                  // source file pointer
    unsigned    in_recsize;             // current record size in bytes
    int         in_rectype;             // type return from get_record()
    int         in_elemrec;             // element id when in element
    int         in_curlayer;            // current layer being output
    int         in_curdtype;            // current datatype being output
    int         in_layer;               // layer read from input
    int         in_dtype;               // datatype read from input
    unsigned int in_nx, in_ny;          // aref array colums, rows
    int         in_attrib;              // attribute (property) number
    int         in_presentation;        // text presentation
    int         in_numpts;              // size of g_points[] read
    int         in_pwidth;              // path width attribute
    int         in_ptype;               // path type attribute
    int         in_text_width;          // text width from property
    int         in_text_height;         // text height from property
    int         in_plexnum;             // plex number
    int         in_bextn;               // extension value
    int         in_eextn;               // extension value
    int         in_version;             // gdsii release number
    int         in_text_flags;          // show/hide property label flags
    unsigned int in_ptype4_cnt;         // pathtype 4 -> 0 conversion count
    Attribute   *in_sprops;             // list of symbol proprties
    Attribute   *in_eprops;             // extension for electrical props

    unsigned char in_headrec[4];        // record header buffer
    char        in_layer_name[12];      // name of new layer created
    CDll        *in_layers;             // list of layers to output
    SymTab      *in_undef_layers;       // keep track of undefined layers
    SymTab      *in_phys_layer_tab;     // mapped layer cache
    SymTab      *in_elec_layer_tab;     // mapped layer cache
    bool        in_headrec_ok;          // saved header record valid
    bool        in_bswap;               // if true, swap input bytes
    bool        in_reflection;          // reflection transform
    char        *in_string;             // text string
    char        in_cellname[256];       // current symbol name, unaliased
    char        in_cbuf[GDS_MAX_REC_SIZE];      // input buffer
    Point       in_points[GDS_MAX_REC_SIZE/8];  // buffer for x-y data

    // dispatch table
    bool(gds_in::*ftab[GDS_NUM_REC_TYPES])();
};


struct gds_rec;

// Class for generating GDSII output.
//
struct gds_out : public cv_out
{
    gds_out();
    ~gds_out();

    bool check_for_interrupt();
    bool write_header(const CDs*);
    bool write_object(const CDo*, cvLchk*);
    bool set_destination(const char*);
    bool set_destination(FILE *fp, void**, void**)
        {
            if (!out_fp) {
                out_fp = new sFilePtr(0, fp);
                out_byte_count = out_fp->z_tell();
                return (true);
            }
            if (!out_fp->file) {
                out_fp->fp = fp;
                out_byte_count = out_fp->z_tell();
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
    // end virtual overrides

protected:

    void begin_record(int count, int type, int datatype)
        {
            out_buffer[out_bufcnt++] = count >> 8;
            out_buffer[out_bufcnt++] = count;
            out_buffer[out_bufcnt++] = type;
            out_buffer[out_bufcnt++] = datatype;
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

#define GDS_LIBNAMLEN 44

    void name_copy(const char *c)
        {
            int i;
            for (i = 0; i < GDS_LIBNAMLEN && *c; i++)
                out_buffer[out_bufcnt++] = *c;
            for ( ; i < GDS_LIBNAMLEN; i++)
                out_buffer[out_bufcnt++] = 0;
        }

    void date_copy(struct tm*);
    void dbl_copy(double);
    bool flush_buf();

    bool write_property_rec(int, const char*);
    bool write_ascii_rec(const char*, int);
    bool write_path(int, int, int);
    bool write_object_prv(const CDo*);


public:
    bool read_lib(DisplayMode);

protected:
    double      out_m_unit;                  // scale data for munits
    FilePtr     out_fp;                      // output file pointer
    int         out_max_poly;                // polygon vertex limit
    int         out_max_path;                // wire vertex limit
    int         out_version;                 // stream format version
    int         out_undef_count;             // unmapped layer indices
    tm          out_date;                    // datecode (now)
    Point       *out_xy;                     // data buffer
    SymTab      *out_layer_oor_tab;          // out of range layers
    table_t<tmp_odata> *out_layer_cache;     // layer name hash table

#define GDS_BUFSIZ 1024
    int         out_generation;              // generations number
    int         out_bufcnt;                  // output buffer bytes used
    unsigned char out_buffer[GDS_BUFSIZ+8];  // output buffer + slop

    // These are used in the file header, if set from saved properties
    char        out_libname[GDS_LIBNAMLEN+2];// library name
    char        out_lib1[GDS_LIBNAMLEN+2];   // reference library 1
    char        out_lib2[GDS_LIBNAMLEN+2];   // reference library 2
    char        out_font0[GDS_LIBNAMLEN+2];  // font 0
    char        out_font1[GDS_LIBNAMLEN+2];  // font 1
    char        out_font2[GDS_LIBNAMLEN+2];  // font 2
    char        out_font3[GDS_LIBNAMLEN+2];  // font 3
    char        out_attr[GDS_LIBNAMLEN+2];   // attribute table name
};

#endif


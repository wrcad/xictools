
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
 $Id: fio_oasis.h,v 5.64 2017/04/18 03:13:59 stevew Exp $
 *========================================================================*/

#ifndef FIO_OASIS_H
#define FIO_OASIS_H

#include "fio_cvt_base.h"
#include "fio_gds_layer.h"

#define OAS_MAGIC  "%SEMI-OASIS\r\n"

// OASIS record types
#define OAS_NUM_REC_TYPES 35

enum XYmode { XYabsolute, XYrelative };

struct oas_elt
{
    friend struct oas_table;

    oas_elt(unsigned int i, oas_elt *n)
        {
            e_next = n;
            e_string = 0;
            e_prpty_list = 0;
            e_data = 0;
            e_index = i;
            e_length = 0;
        }

    ~oas_elt()
        {
            delete [] e_string;
            e_prpty_list->free_list();
        }

    void set_string(char *s, unsigned int l = 0)
        {
            e_string = s;
            e_length = l;
        }
    void set_prpty_list(CDp *p)     { e_prpty_list = p; }
    void set_data(void *d)          { e_data = d; }

    const char *string()      const { return (e_string); }
    unsigned int length()     const { return (e_length); }
    CDp *prpty_list()               { return (e_prpty_list); }
    void *data()                    { return (e_data); }

private:
    oas_elt *e_next;
    char *e_string;
    CDp *e_prpty_list;
    void *e_data;
    unsigned int e_index;
    unsigned int e_length;
};

struct oas_table
{
    oas_table() { count = 0; hashmask = 0; tab[0] = 0; }
    ~oas_table();
    oas_elt *get(unsigned int);
    oas_elt *add(unsigned int);
    oas_table *check_rehash();

    unsigned int hash(unsigned int x) { return (x & hashmask); }

    unsigned int count;
    unsigned int hashmask;
    oas_elt *tab[1];
};

struct oas_repetition
{
    oas_repetition()
    {
        type = -1;
        xdim = ydim = 0;
        xspace = yspace = 0;
        array = 0;
    }

    ~oas_repetition()
    {
        delete [] array;
    }

    void reset(int t)
    {
        delete [] array;
        array = 0;
        xdim = ydim = 0;
        xspace = yspace = 0;
        type = t;
    }

    bool periodic()
        { return ((type >= 1 && type <= 3) || type == 8 || type == 9); }

    bool grid_params(int*, int*, unsigned int*, int*, int*, unsigned int*);

    int type;
    unsigned int xdim;
    unsigned int ydim;
    int xspace;
    int yspace;
    int *array;
};

// Generator for repetitions
//
struct oas_rgen
{
    oas_rgen(oas_repetition *rep = 0)  { init(rep); }
    void init(oas_repetition *rep)
        { r = rep; xnext = 0; ynext = 0; xacc = 0; yacc = 0; done = false; }

    bool next(int*, int*);

    // placement array
    void set_array(unsigned int nx, unsigned int ny, int dx, int dy)
        { r = 0; xnext = nx; ynext = ny; xacc = dx; yacc = dy; done = false; }
    bool get_array(unsigned int *nx, unsigned int *ny, int *dx, int *dy)
        {
            if (r || done)
                return (false);
            *nx = xnext > 0 ? xnext : 1;
            *ny = ynext > 0 ? ynext : 1;
            *dx = xacc;
            *dy = yacc;
            done = true;
            return (true);
        }

    oas_repetition *r;
    unsigned int xnext;
    unsigned int ynext;
    int xacc;
    int yacc;
    bool done;
};

struct oas_interval
{
    oas_interval()
    {
        minval = 0;
        maxval = 0;
    }

    unsigned int minval;
    unsigned int maxval;
};

struct oas_layer_map_elem
{
    oas_layer_map_elem()
    {
        name = 0;
        next = 0;
        is_text = false;
    }

    void free()
    {
        oas_layer_map_elem *el = this;
        while (el) {
            oas_layer_map_elem *e = el;
            el = el->next;
            delete e;
        }
    }

    oas_layer_map_elem *next;
    char *name;
    oas_interval number_range;
    oas_interval type_range;
    bool is_text;
};

struct oas_property_value
{
    oas_property_value() { type = 0; b_length = 0; u.rval = 0.0; }
    ~oas_property_value() { if (type >= 10 && type <= 12) delete [] u.sval; }

    unsigned int type;
    unsigned int b_length;
    union {
        double rval;
        int64_t ival;
        uint64_t uval;
        char *sval;
    } u;
};

struct oas_placement
{
    char *name;
    double magnification;
    double angle;
    int x;
    int y;
    oas_repetition *repetition;
    bool flip_y;
};

struct oas_text
{
    char *string;
    unsigned int textlayer;
    unsigned int texttype;
    int x;
    int y;
    oas_repetition *repetition;
    unsigned int width;
    unsigned int xform;
};

struct oas_rectangle
{
    unsigned int layer;
    unsigned int datatype;
    unsigned int width;
    unsigned int height;
    int x;
    int y;
    oas_repetition *repetition;
};

struct oas_polygon
{
    unsigned int layer;
    unsigned int datatype;
    Point *point_list;
    unsigned int point_list_size;
    int x;
    int y;
    oas_repetition *repetition;
};

struct oas_path
{
    unsigned int layer;
    unsigned int datatype;
    unsigned int half_width;
    int start_extension;
    int end_extension;
    Point *point_list;
    unsigned int point_list_size;
    int x;
    int y;
    oas_repetition *repetition;
    bool rounded_end;
};

struct oas_trapezoid
{
    unsigned int layer;
    unsigned int datatype;
    unsigned int width;
    unsigned int height;
    int delta_a;
    int delta_b;
    int x;
    int y;
    oas_repetition *repetition;
    bool vertical;
};

struct oas_ctrapezoid
{
    unsigned int layer;
    unsigned int datatype;
    unsigned int type;
    unsigned int width;
    unsigned int height;
    int x;
    int y;
    oas_repetition *repetition;
};

struct oas_circle
{
    unsigned int layer;
    unsigned int datatype;
    unsigned int radius;
    int x;
    int y;
    oas_repetition *repetition;
};

struct oas_xgeometry
{
    unsigned int attribute;
    unsigned int layer;
    unsigned int datatype;
    int x;
    int y;
    oas_repetition *repetition;
};

union oas_object
{
    oas_placement placement;
    oas_text text;
    oas_rectangle rectangle;
    oas_polygon polygon;
    oas_path path;
    oas_trapezoid trapezoid;
    oas_ctrapezoid ctrapezoid;
    oas_circle circle;
    oas_xgeometry xgeometry;
};

struct oas_modal
{
    oas_modal() { reset(false); }
    ~oas_modal() { reset(); }

    void reset(bool = true);

    // PLACEMENT, TEXT, POLYGON, PATH, RECTANGLE, TRAPEZOID, CTRAPEZOID
    // CIRCLE, XGEOMETRY
    oas_repetition repetition;

    // PLACEMENT
    unsigned int placement_x;
    unsigned int placement_y;
    char *placement_cell;

    // POLYGON, PATH, RECTANGLE, TRAPEZOID, CTRAPEZOID, CIRCLE, XGEOMETRY
    unsigned int layer;
    unsigned int datatype;

    // TEXT
    unsigned int textlayer;
    unsigned int texttype;
    unsigned int text_x;
    unsigned int text_y;
    char *text_string;

    // POLYGON, PATH, RECTANGLE, TRAPEZOID, CTRAPEZOID, CIRCLE, XGEOMETRY
    int geometry_x;
    int geometry_y;

    // PLACEMENT, TEXT, POLYGON, PATH, RECTANGLE, TRAPEZOID, CTRAPEZOID
    // CIRCLE, XGEOMETRY, XYABSOLUTE, XYRELATIVE
    XYmode xy_mode;

    // RECTANGLE, TRAPEZOID, CTRAPEZOID
    unsigned int geometry_w;
    unsigned int geometry_h;

    // POLYGON
    Point *polygon_point_list;
    unsigned int polygon_point_list_size;

    // PATH
    unsigned int path_half_width;
    Point *path_point_list;
    unsigned int path_point_list_size;
    int path_start_extension;
    int path_end_extension;

    // CTRAPEZOID
    unsigned int ctrapezoid_type;

    // CIRCLE
    unsigned int circle_radius;

    // PROPERTY
    char *last_property_name;
    oas_property_value *last_value_list;
    unsigned int last_value_list_size;

    bool repetition_set;

    bool placement_x_set;
    bool placement_y_set;
    bool placement_cell_set;

    bool layer_set;
    bool datatype_set;

    bool textlayer_set;
    bool texttype_set;
    bool text_x_set;
    bool text_y_set;
    bool text_string_set;

    bool geometry_x_set;
    bool geometry_y_set;

    bool xy_mode_set;

    bool geometry_w_set;
    bool geometry_h_set;

    bool polygon_point_list_set;

    bool path_half_width_set;
    bool path_point_list_set;
    bool path_start_extension_set;
    bool path_end_extension_set;

    bool ctrapezoid_type_set;

    bool circle_radius_set;

    bool last_property_name_set;
    bool last_value_list_set;
    bool last_value_standard;
};

// Parser state storage for recursion.
//
struct oas_state
{
    char *modal_store;      // save modal
    zio_stream *zfile;      // save in_zfile
    uint64_t last_pos;      // save in_byte_offset
    uint64_t last_offset;   // save in_offset
    uint64_t next_offset;   // save in_next_offset
    uint64_t comp_end;      // save in_compression_end
};

// Information storage
//
struct oas_info : public cv_info
{
    oas_info(bool per_lyr, bool per_cell) : cv_info(per_lyr, per_cell)
        { memset(rec_counts, 0, OAS_NUM_REC_TYPES*sizeof(int)); }
    void initialize()
        {
            memset(rec_counts, 0, OAS_NUM_REC_TYPES*sizeof(int));
            cv_info::initialize();
        }
    void add_record(int);
    char *pr_records(FILE*);

    unsigned int    rec_counts[OAS_NUM_REC_TYPES];
};

// Pointlist storage for ascii-output mode.
struct ptlist_t
{
    ptlist_t() { type = 0; size = 0; values = 0; }
    ~ptlist_t() { delete [] values; }

    unsigned int type;
    unsigned int size;
    int *values;
};

// Header information, used with archive context.
//
struct oas_header_info : public cv_header_info
{
    oas_header_info(double u) : cv_header_info(Foas, u)
        {
            propstring_tab = 0;
            propname_tab = 0;
            cellname_tab = 0;
            textstring_tab = 0;
            layername_tab = 0;
            xname_tab = 0;
        }

    ~oas_header_info()
        {
            delete propstring_tab;
            delete propname_tab;
            delete cellname_tab;
            delete textstring_tab;
            delete layername_tab;
            delete xname_tab;
        }

    oas_table *propstring_tab;
    oas_table *propname_tab;
    oas_table *cellname_tab;
    oas_table *textstring_tab;
    oas_table *layername_tab;
    oas_table *xname_tab;
};

// Base class for alternate byte source for reading.
struct oas_byte_stream
{
    virtual ~oas_byte_stream() { }

    virtual int get_byte() = 0;
    virtual bool done() const = 0;
    virtual bool error() const = 0;
};

struct cv_cgd_if;

// State of parse, used for incremental reading.
enum oasState
{
    oasError = -1,
    oasDone = 0,
    oasNeedInit,
    oasNeedRecord,
    oasHasPlacement,
    oasHasText,
    oasHasRectangle,
    oasHasPolygon,
    oasHasPath,
    oasHasTrapezoid,
    oasHasCtrapezoid,
    oasHasCircle,
    oasHasXgeometry
};


// Class for reading OASIS files
//
struct oas_in : public cv_in
{
    oas_in(bool);  // arg allows layer mapping if true
    virtual ~oas_in();

    // setup functions
    bool setup_source(const char*, const cCHD* = 0);
    bool setup_to_database();
    bool setup_destination(const char*, FileType, bool);
    bool setup_ascii_out(const char*, uint64_t=0, uint64_t=0, int=-1, int=-1);
    bool setup_backend(cv_out*);

    // Set a database interface for input, instead of a file.
    bool setup_cgd_if(cCGD*, bool, const char*, FileType, const cCHD* = 0);

    // main entry for reading
    bool parse(DisplayMode, bool, double, bool = false, cvINFO = cvINFOtotals);

    // incremental parse, requires byte stream
    bool parse_incremental(double);
    oasState get_state() { return (in_state); }

    // Incremental object processing
    bool run_placement();
    bool run_text();
    bool run_rectangle();
    bool run_polygon();
    bool run_path();
    bool run_trapezoid();
    bool run_ctrapezoid();
    bool run_circle();
    bool run_xgeometry();

    // entries for reading through cell hierarchy digest
    bool chd_read_header(double);
    bool chd_read_cell(symref_t*, bool, CDs** = 0);
    cv_header_info *chd_get_header_info();
    void chd_set_header_info(cv_header_info*);

    // misc. entries
    bool has_electrical();
    FileType file_type() { return (Foas); }
    void add_header_props(CDs*) { }
    bool has_header_props() { return (false); }
    OItype has_geom(symref_t*, const BBox* = 0);
    // end of virtual overrides

    void set_save_zoids(bool b) { in_save_zoids = b; }
    Zlist *get_zoids() { Zlist *z = in_zoidlist; in_zoidlist = 0; return (z); }

    // Set a byte stream to read, instead of a file.
    void set_byte_stream(oas_byte_stream *src)
        { in_byte_stream = src; modal.reset(); }

private:
    bool dispatch(int ix)
        {
            if (in_zfile || in_byte_stream)
                in_offset = in_byte_offset - 1;
            else if (in_fp)
                in_offset = in_fp->z_tell() - 1;
            else
                in_offset = 0;
            return ((this->*ftab[ix])(ix));
        }

    OItype has_geom_core(const BBox*);
    bool read_header(bool);
    bool end_cell();

    bool a_cell(const char*);
    bool a_placement(int, int, unsigned int, unsigned int);
    bool a_text();
    bool a_rectangle();
    bool a_polygon();
    bool a_path();
    bool a_trapezoid();
    bool a_ctrapezoid();
    bool a_circle();
    bool a_property();
    bool a_standard_property();
    bool a_endlib();
    void a_clear_properties();

    bool a_layer(unsigned int, unsigned int, bool* = 0, bool = false);
    void a_add_properties(CDs*, CDo*);
    bool a_polygon_save(Poly*);

    bool ac_cell(const char*);
    bool ac_placement(int, int, unsigned int, unsigned int);
    bool ac_placement_backend(Instance*, symref_t*, bool);
    bool ac_text();
    bool ac_text_prv(const Text*);
    bool ac_rectangle();
    bool ac_rectangle_prv(BBox&);
    bool ac_polygon();
    bool ac_path();
    bool ac_path_prv(Wire&);
    bool ac_trapezoid();
    bool ac_ctrapezoid();
    bool ac_circle();
    bool ac_property();
    bool ac_standard_property();
    bool ac_endlib();
    void ac_clear_properties();

    bool ac_layer(unsigned int, unsigned int);
    bool ac_polygon_save(Poly*);
    bool ac_polygon_save_prv(Poly*);

    bool trap2poly(Poly*);

    // ctrapezoid actions
    bool ctrap0(Poly*, int, int, unsigned int, unsigned int);
    bool ctrap1(Poly*, int, int, unsigned int, unsigned int);
    bool ctrap2(Poly*, int, int, unsigned int, unsigned int);
    bool ctrap3(Poly*, int, int, unsigned int, unsigned int);
    bool ctrap4(Poly*, int, int, unsigned int, unsigned int);
    bool ctrap5(Poly*, int, int, unsigned int, unsigned int);
    bool ctrap6(Poly*, int, int, unsigned int, unsigned int);
    bool ctrap7(Poly*, int, int, unsigned int, unsigned int);
    bool ctrap8(Poly*, int, int, unsigned int, unsigned int);
    bool ctrap9(Poly*, int, int, unsigned int, unsigned int);
    bool ctrap10(Poly*, int, int, unsigned int, unsigned int);
    bool ctrap11(Poly*, int, int, unsigned int, unsigned int);
    bool ctrap12(Poly*, int, int, unsigned int, unsigned int);
    bool ctrap13(Poly*, int, int, unsigned int, unsigned int);
    bool ctrap14(Poly*, int, int, unsigned int, unsigned int);
    bool ctrap15(Poly*, int, int, unsigned int, unsigned int);
    bool ctrap16(Poly*, int, int, unsigned int, unsigned int);
    bool ctrap17(Poly*, int, int, unsigned int, unsigned int);
    bool ctrap18(Poly*, int, int, unsigned int, unsigned int);
    bool ctrap19(Poly*, int, int, unsigned int, unsigned int);
    bool ctrap20(Poly*, int, int, unsigned int, unsigned int);
    bool ctrap21(Poly*, int, int, unsigned int, unsigned int);
    bool ctrap22(Poly*, int, int, unsigned int, unsigned int);
    bool ctrap23(Poly*, int, int, unsigned int, unsigned int);
    bool ctrap24(Poly*, int, int, unsigned int, unsigned int);
    bool ctrap25(Poly*, int, int, unsigned int, unsigned int);

    void warning(const char*);
    void warning(const char*, int, int, unsigned int, unsigned int);
    void warning(const char*, const char*, int, int);

    // Initialization and control functions
    void push_state(oas_state*);
    void pop_state(oas_state*);

    // Functions to read basic data types
    unsigned char read_byte();
    unsigned int read_unsigned();
    uint64_t read_unsigned64();
    int read_signed();
    int64_t read_signed64();
    double read_real();
    double read_real_data(unsigned int);
    unsigned int read_delta(int, int*);
    unsigned int read_delta_bytes(unsigned char, int, int*);
    bool read_g_delta(int*, int*);
    bool read_repetition();
    Point *read_point_list(unsigned int*, bool);
    char *read_a_string();
    void read_a_string_nr();
    char *read_b_string(unsigned int*);
    void read_b_string_nr();
    char *read_n_string();
    void read_n_string_nr();
    bool read_property_value(oas_property_value*);
    bool read_interval(oas_interval*);
    bool read_offset_table();
    bool read_tables();
    bool read_table(uint64_t, unsigned int, unsigned int);
    unsigned int read_record_index(unsigned int);
    bool scan_for_names();

    // Functions to read OASIS records (dispatch table)
    bool read_start(unsigned int);
    bool read_end(unsigned int);
    bool read_cellname(unsigned int);
    bool read_textstring(unsigned int);
    bool read_propname(unsigned int);
    bool read_propstring(unsigned int);
    bool read_layername(unsigned int);
    bool read_cell(unsigned int);
    bool xy_absolute(unsigned int);
    bool xy_relative(unsigned int);
    bool read_placement(unsigned int);
    bool read_text(unsigned int);
    bool read_rectangle(unsigned int);
    bool read_polygon(unsigned int);
    bool read_path(unsigned int);
    bool read_trapezoid(unsigned int);
    bool read_ctrapezoid(unsigned int);
    bool read_circle(unsigned int);
    bool read_property(unsigned int);
    bool read_xname(unsigned int);
    bool read_xelement(unsigned int);
    bool read_xgeometry(unsigned int);
    bool read_cblock(unsigned int);

    bool peek_property(unsigned int);
    bool placement_array_params(int*, int*, unsigned int*, unsigned int*);

    // Dispatch table for ascii output
    bool print_nop(unsigned int);
    bool print_start(unsigned int);
    bool print_end(unsigned int);
    bool print_name(unsigned int);
    bool print_name_r(unsigned int);
    bool print_layername(unsigned int);
    bool print_cell(unsigned int);
    bool print_placement(unsigned int);
    bool print_text(unsigned int);
    bool print_rectangle(unsigned int);
    bool print_polygon(unsigned int);
    bool print_path(unsigned int);
    bool print_trapezoid(unsigned int);
    bool print_ctrapezoid(unsigned int);
    bool print_circle(unsigned int);
    bool print_property(unsigned int);
    bool print_xname(unsigned int);
    bool print_xelement(unsigned int);
    bool print_xgeometry(unsigned int);
    bool print_cblock(unsigned int);

    void check_offsets();
    bool print_repetition();
    ptlist_t *read_pt_list();
    void print_pt_list(ptlist_t*);
    bool print_property_value();
    void print_recname(unsigned int);
    void print_keyword(int);
    bool print_info_byte(unsigned int*);
    bool print_unsigned(unsigned int*);
    bool print_unsigned64(uint64_t*);
    bool print_signed(int*);
    bool print_real(double*);
    bool print_string(char**, unsigned int*);
    bool print_g_delta(int*, int*);
    void print(const char*, ...);
    void put_string(const char*, size_t);
    void put_char(char);
    void put_space(unsigned int);

    FilePtr         in_fp;              // source file pointer (handles
                                        //  globally gzipped source files)
    char            *in_cellname;       // current cell name
    int             in_curlayer;        // current layer number
    int             in_curdtype;        // current datatype number
    CDll            *in_layers;         // mapped-to Xic layers
    SymTab          *in_undef_layers;   // keep track of undefined layers
    char            in_layer_name[12];  // created layer name
    double          in_unit;            // START unit value
    zio_stream      *in_zfile;          // zlib file pointer
    uint64_t        in_byte_offset;     // current offset
    uint64_t        in_compression_end; // last uncompresed byte
    uint64_t        in_next_offset;     // offset after compressed block
    uint64_t        in_start_offset;    // offset of START record
    uint64_t        in_scan_start_offset; // offset following START record
    SymTab          *in_phys_layer_tab; // mapped layer cache
    SymTab          *in_elec_layer_tab; // mapped layer cache

    cCGD            *in_cgd;            // interface to CGD for geometry
    oas_byte_stream *in_byte_stream;    // low-level cCGD input stream
    cv_in           *in_bakif;          // backup interface for in_cgd

    oas_rgen        in_rgen;            // repetition generator
    oasState        in_state;           // incremental parser state

    bool            in_xic_source;      // file written by friendly source
    bool            in_nogo;            // fail flag
    bool            in_incremental;     // true in incremental parse
    bool            in_skip_all;        // skip all processing for records
                                        //  other than <names>
    bool            in_skip_action;     // skip all actions
    bool            in_skip_elem;       // skip object action, for layer filter
    bool            in_skip_cell;       // skip cell action, but save name
    unsigned char   in_peek_record;     // current record number
    unsigned char   in_offset_flag;     // offset table location code
    unsigned char   in_peek_byte;       // read-ahead byte
    bool            in_peeked;          // have read ahead
    bool            in_tables_read;     // strict mode tables read
    bool            in_print_break_lines;  // wrap lines when printing
    bool            in_print_offset;    // print record offsets
    bool            in_looked_ahead;    // did search for name records
    bool            in_scanning;        // doing search for name records
    bool            in_reading_table;   // reading a name table

    unsigned int    in_print_cur_col;   // current print column
    unsigned int    in_print_start_col; // line start column for printing

    // file properties
    unsigned int    in_max_signed_integer_width;
    unsigned int    in_max_unsigned_integer_width;
    unsigned int    in_max_string_length;
    unsigned int    in_polygon_max_vertices;
    unsigned int    in_path_max_vertices;
    char            *in_top_cell;
    unsigned int    in_bounding_boxes_available;

    // dispatch table
    bool(oas_in::*ftab[35])(unsigned int);

    // ctrapezoid interpreting
    bool(oas_in::*ctrtab[26])(Poly*, int, int, unsigned int, unsigned int);

    // actions
    bool (oas_in::*cell_action)(const char*);
    bool (oas_in::*placement_action)(int, int, unsigned int, unsigned int);
    bool (oas_in::*text_action)();
    bool (oas_in::*rectangle_action)();
    bool (oas_in::*polygon_action)();
    bool (oas_in::*path_action)();
    bool (oas_in::*trapezoid_action)();
    bool (oas_in::*ctrapezoid_action)();
    bool (oas_in::*circle_action)();
    bool (oas_in::*property_action)();
    bool (oas_in::*standard_property_action)();
    bool (oas_in::*end_action)();
    void (oas_in::*clear_properties_action)();

    // <name> tables
    oas_table       *in_cellname_table;
    oas_table       *in_textstring_table;
    oas_table       *in_propname_table;
    oas_table       *in_propstring_table;
    oas_table       *in_layername_table;
    oas_layer_map_elem *in_layermap_list;
    oas_table       *in_xname_table;

    // <name> auto-generated indices
    unsigned int    in_cellname_index;
    unsigned int    in_textstring_index;
    unsigned int    in_propname_index;
    unsigned int    in_propstring_index;
    unsigned int    in_layername_index;
    unsigned int    in_xname_index;

    // <name> record types
    unsigned char   in_cellname_type;
    unsigned char   in_textstring_type;
    unsigned char   in_propname_type;
    unsigned char   in_propstring_type;
    unsigned char   in_layername_type;
    unsigned char   in_xname_type;

    // table offsets
    uint64_t        in_cellname_off;
    uint64_t        in_textstring_off;
    uint64_t        in_propname_off;
    uint64_t        in_propstring_off;
    uint64_t        in_layername_off;
    uint64_t        in_xname_off;

    // table strict-mode flags
    unsigned char   in_cellname_flag;
    unsigned char   in_textstring_flag;
    unsigned char   in_propname_flag;
    unsigned char   in_propstring_flag;
    unsigned char   in_layername_flag;
    unsigned char   in_xname_flag;

    // save trapezoids
    bool            in_save_zoids;
    Zlist           *in_zoidlist;

    // <geometry> parameter storage
    oas_object      uobj;

    // modal variables
    oas_modal       modal;
};


class cCGD;
struct cgd_layer_mux;
struct oas_cache;

// Class for generating OASIS output
//
struct oas_out : public cv_out
{
    friend struct oas_cache;

    oas_out(cCGD* = 0);
    ~oas_out();

    bool check_for_interrupt();
    bool flush_cache();
    bool write_repetition();
    bool write_header(const CDs*);
    bool write_object(const CDo*, cvLchk*);

    bool set_destination(const char*);
    bool set_destination(FILE*, void**, void**);

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

    bool write_trapezoid(Zoid*, bool);

    void init_tables(DisplayMode);
    void enable_cache(bool);

    void set_cgd(cCGD *o) { out_cgd = o; }
    cCGD *get_cgd() { cCGD *o = out_cgd; out_cgd = 0; return (o); }

private:
    inline bool write_char(int);
    bool write_unsigned(unsigned int);
    bool write_unsigned64(uint64_t);
    bool write_signed(int);
    bool write_delta(int, int, unsigned int);
    bool write_g_delta(int, int);
    bool write_point_list(Point*, int, bool);
    bool write_real(double);
    bool write_n_string(const char*);
    bool write_a_string(const char*);
    bool write_b_string(const char*, unsigned int);

    bool write_object_prv(const CDo*);
    bool write_cell_properties();
    bool write_elem_properties();
    bool write_offset_std_prpty(uint64_t);
    bool write_label_property(unsigned int, unsigned int);
    bool write_rounded_end_property();
    bool write_offset_table();
    bool write_tables();

    bool begin_compression(const char*);
    bool end_compression();

    bool setup_properties(CDp*);
    void set_layer_dt(int, int, int* = 0, int* = 0);

    FILE        *out_fp;                    // output file pointer
    FILE        *out_tmp_fp;                // temp fp for compression
    char        *out_tmp_fname;             // temp file name for compr.
    table_t<tmp_odata> *out_layer_cache;    // layer name hash table

    SymTab      *out_cellname_tab;          // cellname table
    SymTab      *out_textstring_tab;        // textstring table
    SymTab      *out_propname_tab;          // propname table
    SymTab      *out_propstring_tab;        // propstring table
    SymTab      *out_layername_tab;         // layername table
    SymTab      *out_xname_tab;             // xname table
    stbuf_t     *out_string_pool;           // string pool for tables

    unsigned int out_cellname_ix;           // cellname table index
    unsigned int out_textstring_ix;         // textstring table index
    unsigned int out_propname_ix;           // propname table index
    unsigned int out_propstring_ix;         // propstring table index
    unsigned int out_layername_ix;          // layername table index
    unsigned int out_xname_ix;              // xname table index

    uint64_t    out_cellname_off;           // cellname table offset
    uint64_t    out_textstring_off;         // textstring table offset
    uint64_t    out_propname_off;           // propname table offset
    uint64_t    out_propstring_off;         // propstring table offset
    uint64_t    out_layername_off;          // layername table offset
    uint64_t    out_xname_off;              // xname table offset

    uint64_t    out_comp_start;             // compression start offset
    uint64_t    out_offset_table;           // table location in START
    uint64_t    out_start_offset;           // START record

    cCGD        *out_cgd;                   // optional output database
    cgd_layer_mux *out_lmux;                // layer mux for cCGD
    oas_modal   *out_modal;                 // modal variables
    zio_stream  *out_zfile;                 // zlib file pointer
    oas_cache   *out_cache;                 // repetition cache
    unsigned char *out_compr_buf;           // buffer for compression
    const char  *out_rep_args;              // repetition cache args
    int         out_undef_count;            // unmapped layer indices

    unsigned char out_prp_mask;             // don't write certain properties
    bool        out_offset_flag;            // offset table flag
    bool        out_use_repetition;         // write using repetition
    bool        out_use_trapezoids;         // write zoids for trapezoid polys
    bool        out_boxes_for_wires;        // write boxes for rect wires
    bool        out_rnd_wire_to_poly;       // write polys for round-end wires
    bool        out_faster_sort;            // use quicksort exclusively
    bool        out_no_gcd_check;           // skip gcd check in repetitions
    unsigned char out_use_compression;      // compression strategy
    bool        out_compressing;            // true when compressing
    bool        out_use_tables;             // use string tables
    unsigned char out_validation_type;      // validation scheme

    oas_repetition out_repetition;          // object repetition spec
    oas_modal   out_default_modal;          // modal variables
};


// Class for incremental reading/decompressing from a byte stream.
//
struct cv_incr_reader : public cv_out
{
    cv_incr_reader(oas_byte_stream*, const FIOcvtPrms*);
    ~cv_incr_reader();

    CDo *next_object();

    cv_in *reader() { return (&ir_oas); }

private:
    bool set_destination(const char*) { return (true); }
    bool set_destination(FILE*, void**, void**) { return (true); }
    bool open_library(DisplayMode, double) { return (true); }
    bool queue_property(int, const char*) { return (true); }
    bool write_library(int, double, double, tm*, tm*, const char*)
        { return (true); }
    bool write_struct(const char*, tm*, tm*) { return (true); }
    bool write_end_struct(bool = false) { return (true); }

    bool queue_layer(const Layer *layer, bool*)
        {
            ir_ldesc = CDldb()->findLayer(layer->name, out_mode);
            if (!ir_ldesc)
                ir_ldesc = CDldb()->newLayer(layer->name, out_mode);
            return (true);
        }

    bool write_box(const BBox*);
    bool write_poly(const Poly*);
    bool write_wire(const Wire*);
    bool write_text(const Text*) { return (true); }
    bool write_sref(const Instance*) { return (true); }
    bool write_endlib(const char*) { return (true); }
    bool write_info(Attribute*, const char*) { return (true); }

    bool check_for_interrupt() { return (true); }
    bool flush_cache() { return (true); }
    bool write_header(const CDs*) { return (false); }
    bool write_object(const CDo*, cvLchk*) { return (false); }

    oas_in ir_oas;
    double ir_scale;
    CDo *ir_object;
    CDl *ir_ldesc;
    oas_byte_stream *ir_bstream;

    BBox ir_AOI;
    bool ir_useAOI;
    bool ir_clip;
    bool ir_error;
};


// Flags for out_cache_repetition
#define OAS_CR_BOX      0x1
#define OAS_CR_POLY     0x2
#define OAS_CR_WIRE     0x4
#define OAS_CR_LAB      0x8
#define OAS_CR_CELL     0x10
#define OAS_CR_GEOM     0xf

// Return the number of bytes needed to represent i.
//
inline unsigned int
usizeof(unsigned int i)
{
    unsigned int cnt = 0;
    do {
        i >>= 7;
        cnt++;
    } while (i);
    return (cnt);
}

#endif


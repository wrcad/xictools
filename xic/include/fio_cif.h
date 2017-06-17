
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
 $Id: fio_cif.h,v 5.42 2017/04/18 03:13:59 stevew Exp $
 *========================================================================*/

#ifndef FIO_CIF_H
#define FIO_CIF_H

#include "fio_cvt_base.h"


// Arguments to handle an EOF in PCharacter and PInteger
#define PFAILONEOF         1
#define PDONTFAILONEOF     2

// Error conditions
#define PERREOF            1
#define PERRNOSEMI         2
#define PERRLAYER          3
#define PERRCD             4
#define PERRETC            5

// Arguments to specify characters to be ignored by PWhiteSpace

// strip blanks, tabs, commas, or new lines
#define PSTRIP1  1

// strip everything but alpha, digits, hyphens, parens, and ;'s
#define PSTRIP2  2

// strip everything but digits hyphens, parens, and ;'s
#define PSTRIP3  3

// don't strip
#define PLEAVESPACE   4

struct cifCallDesc
{
    cifCallDesc() { nx = ny = 1; dx = dy = 0; name = 0; sdesc = 0; magn = 1.0; }
    cifCallDesc(const char *n, CDs *s, unsigned int nnx, unsigned int nny,
        int ndx, int ndy, double m)
        { name = n; sdesc = s; nx = nnx; ny = nny; dx = ndx; dy = ndy;
        magn = m; }

    unsigned int nx, ny;
    int dx, dy;
    const char *name;
    CDs *sdesc;
    double magn;
};

// Information storage
//
struct cif_info : public cv_info
{
    cif_info(bool per_lyr, bool per_cell) : cv_info(per_lyr, per_cell) { }

    char *pr_records(FILE*);
};

// Header information, used with archive context.
//
struct cif_header_info : public cv_header_info
{
    cif_header_info(double u) : cv_header_info(Fcif, u)
        {
            phys_res_found = 0;
            elec_res_found = 0;
        }

    int phys_res_found;
    int elec_res_found;
};

// Struct to hold native file offset and line count for later electrical
// mode parse.
struct cif_eos_t
{
    unsigned int e_offset;
    unsigned int e_lines;
};

struct numtab_t;

// Class for reading CIF
//
struct cif_in : public cv_in
{
public:
    cif_in(bool, CFtype = CFigs);  // first arg allows layer mapping if true
    virtual ~cif_in();

    // native mode
    void setup_native(const char*, FILE*, CDs*);
    OItype translate_native_cell(const char*, DisplayMode, double);

    // setup functions
    bool setup_source(const char*, const cCHD* = 0);
    bool setup_to_database();
    bool setup_destination(const char*, FileType, bool);
    bool setup_ascii_out(const char*, uint64_t=0, uint64_t=0, int=-1, int=-1)
        { return (false); }
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
    FileType file_type() { return (Fcif); }
    void add_header_props(CDs*) { }
    bool has_header_props() { return (false); }
    OItype has_geom(symref_t*, const BBox* = 0);
    // end of virtual overrides

    void set_update_style(bool tf) { in_update_style = tf; }

    // The numtab is a symref table accessed by symbol number, required
    // for CIF dialects that do no provide cell names.
    //
    numtab_t *numtab()
        { return (in_mode == Physical ? in_phys_map : in_elec_map); }
    void set_numtab(numtab_t *nt)
        { if (in_mode == Physical) in_phys_map = nt; else in_elec_map = nt; }

private:
    void reset_call()
        {
            *in_cellname = '\0';
            in_calldesc.nx = in_calldesc.ny = 1;
            in_calldesc.dx = in_calldesc.dy = 0;
            in_calldesc.magn = 1.0;
        }

    bool read_header(bool*);
    bool dispatch(int, bool*);
    bool a_symbol();
    bool a_symbol_db(const char*, int);
    bool a_symbol_cvt(const char*, int);
    bool a_end_symbol();
    bool a_delete_symbol();
    bool a_call();
    bool a_call_db(int);
    bool a_call_cvt(int);
    bool a_call_backend(Instance*, symref_t*, bool);
    bool a_box();
    bool a_box_db(BBox&);
    bool a_box_cvt(BBox&);
    bool a_box_cvt_prv(BBox&);
    bool a_round_flash();
    bool a_polygon();
    bool a_polygon_db(Poly&);
    bool a_polygon_cvt(Poly&);
    bool a_polygon_cvt_prv(Poly&);
    bool a_wire();
    bool a_wire_db(Wire&, bool);
    bool a_wire_cvt(Wire&, bool);
    bool a_wire_cvt_prv(Wire&);
    bool a_layer();
    bool a_extension(int);
    bool a_label(char, char*);
    bool a_comment();

    bool symbol_name(int);
    bool write_label(const Text*);
    bool write_label_prv(const Text*);
    bool create_label(Label&, CDl* = 0);
    void add_properties_db(CDs*, CDo*);
    void clear_properties();
    const char *cur_layer_name();

    bool error(int, const char*);
    void warning(const char*, bool);
    void add_bad(char*);

    bool read_array(int**, int*);
    bool read_point(int*, int*);
    bool read_character(int*, bool*, int, int);
    bool read_integer(int*);
    bool white_space(int*, int, int);
    bool get_next(int*, int(*)(int), int);
    bool look_ahead(int*, int);
    bool look_for_semi(int*);

    // Scaling:
    //  Native cells:
    //   Physical: (resolution scale)
    //   Electrical: (resolution scale)
    //  CIF imports:
    //   Physical: (resolution scale)*(A/B)*(scale button)
    //   Electrical: (resolution scale)
    //  The scale button and A/B are used in physical imports only

    double      in_resol_scale;         // resolution, from file
    cifCallDesc in_calldesc;            // current instance parameters
    FILE        *in_fp;                 // input file pointer

    CDtx        in_tx;                  // transform
    CDl         *in_curlayer;           // pointer to current layer desc
    int         in_line_cnt;            // number of lines read
    CFtype      in_cif_type;            // dialect
    numtab_t    *in_phys_map;           // number to symref map
    numtab_t    *in_elec_map;           // number to symref map
    SymTab      *in_eos_tab;            // elec offsets for native

    int         in_phys_res_found;      // phys (RESOLUTIOB ddd);
    int         in_elec_res_found;      // elec (RESOLUTIOB ddd);

    bool        in_native;              // reading native cell files
    bool        in_found_data;          // set true on first data item
    bool        in_found_resol;         // found "(RESOLUTION ddd);"
    bool        in_found_elec;          // found "(ELECTRICAL);"
    bool        in_found_phys;          // found "(PHYSICAL);"
    bool        in_in_root;             // true when not parsing symbol
    bool        in_skip;                // skip object
    bool        in_no_create_layer;     // don't create new layers
    bool        in_update_style;        // update style if set
    bool        in_cnametype_set;       // record setting style parameters
    bool        in_layertype_set;
    bool        in_labeltype_set;
    bool        in_check_intersect;     // has_geom() test
    char        in_cellname[256];       // current symbol name
    char        in_curlayer_name[256];  // current layer name for OpenModeFiles

    eltab_t<cif_eos_t> in_eos_fct;      // factory for eos elements
};

// Class for generating CIF output
//
struct cif_out : public cv_out
{
    cif_out(const char*);
    ~cif_out();

    bool check_for_interrupt();
    bool write_header(const CDs*);
    bool write_object(const CDo*, cvLchk*);
    bool set_destination(const char*);
    bool set_destination(FILE *fp, void**, void**)
        { out_fp = fp; return (true); }
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

    bool begin_native(const char*);
    bool end_native();

    bool allow_scale_extension()        { return (out_scale_extension); }

private:
    bool write_layer_rec();
    bool convert_2v(Point*, int);

    FILE        *out_fp;                // output file pointer
    const char  *out_cellname;          // current symbol name
    const char  *out_srcfile;           // name of source file
    SymTab      *out_layer_name_tab;    // seen layer names
    SymTab      *out_layer_map_tab;     // new layer names
    int         out_layer_count;        // new layer count
    bool        out_layer_written;      // layer record was written

    // Flags to enable misc. extensions
    bool out_scale_extension;
    bool out_cell_properties;
    bool out_inst_name_comment;
    bool out_inst_name_extension;
    bool out_inst_magn_extension;
    bool out_inst_array_extension;
    bool out_inst_bound_extension;
    bool out_inst_properties;
    bool out_obj_properties;
    bool out_wire_extension;
    bool out_wire_extension_new;
    bool out_text_extension;
    bool out_add_obj_bb;
};

#endif



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

#ifndef FIO_CVT_BASE_H
#define FIO_CVT_BASE_H

#include <time.h>
#include <math.h>
#include "fio_alias.h"
#include "fio_info.h"
#include "fio_tstream.h"
#include "fio_zio.h"


struct cv_out;
struct cv_in;
struct cv_bbif;
struct cv_fgif;
struct frf_out;
struct symref_t;
struct nametab_t;
struct cref_t;
struct crgen_t;
struct FIOlayerAliasTab;
struct CDcellTab;

// Log file
// Prehistoric filenames.
// #define READXIC_FN  "convert_rdxic.log"
// #define READGDS_FN  "convert_rdgds.log"
// #define READCGX_FN  "convert_rdcgx.log"
// #define READOAS_FN  "convert_rdoas.log"
// #define READCIF_FN  "convert_rdcif.log"
// #define TOXIC_FN    "convert_toxic.log"
// #define TOGDS_FN    "convert_togds.log"
// #define TOCGX_FN    "convert_tocgx.log"
// #define TOOAS_FN    "convert_tooas.log"
// #define TOCIF_FN    "convert_tocif.log"
// #define FRXIC_FN    "convert_frxic.log"
// #define FRGDS_FN    "convert_frgds.log"
// #define FRCGX_FN    "convert_frcgx.log"
// #define FROAS_FN    "convert_froas.log"
// #define FRCIF_FN    "convert_frcif.log"

#define READXIC_FN  "read_native.log"
#define READGDS_FN  "read_gdsii.log"
#define READCGX_FN  "read_cgx.log"
#define READOAS_FN  "read_oasis.log"
#define READCIF_FN  "read_cif.log"
// READ_OA_FN is in oa_if.h

#define TOXIC_FN    "write_native.log"
#define TOGDS_FN    "write_gds.log"
#define TOCGX_FN    "write_cgx.log"
#define TOOAS_FN    "write_oas.log"
#define TOCIF_FN    "write_cif.log"

#define FRXIC_FN    "convert_native.log"
#define FRGDS_FN    "convert_gds.log"
#define FRCGX_FN    "convert_cgx.log"
#define FROAS_FN    "convert_oas.log"
#define FRCIF_FN    "convert_cif.log"

// Magic cellname, writers will emit all cells in symbol table.
#define FIO_CUR_SYMTAB  "@%"


// If a scale factor is close to an integer multiplier, set it to be
// precise.
//
inline double
dfix(double xx)
{
    int i = mmRnd(xx);
    if (fabs(xx - i) < 1e-13)
        return ((double)i);
    return (xx);
}


// Return true if po or BBp overlaps BBaoi.
//
inline bool
bound_isect(const BBox *BBaoi, const BBox *BBp, const Poly *po)
{
    if (po->points)
        return (po->intersect(BBaoi, false));
    else
        return (BBp->intersect(BBaoi, false));
}


// Struct used to pass instance description.
//
struct Instance
{
    Instance()
        {
            magn = 1.0;
            angle = 0.0;
            name = 0;
            nx = ny = 1;
            dx = dy = 0;
            ax = 1;
            ay = 0;
            cdesc = 0;
            reflection = false;
            abs_mag = false;
            abs_ang = false;
            gds_text = false;
        }

    void applyTransform(cTfmStack *tstk) const
        { tstk->TApply(origin.x, origin.y, ax, ay, magn, reflection); }

    bool set(const CDc*);
    void set_angle(int, int);
    bool get_array_pts(Point*) const;
    void scale(double);
    bool check_overlap(cTfmStack*, const BBox*, const BBox*) const;

    double magn;
    double angle;
    const char *name;
    int nx, ny;
    int dx, dy;
    int ax, ay;
    Point_c origin;
    const CDc *cdesc;
    bool reflection;

    // For gds-text only.
    bool abs_mag;
    bool abs_ang;
    bool gds_text;  // set when apts valid
    Point_c apts[2];
};


// Struct used to pass text label description.
//
struct Text
{
    Text()
        {
            text = 0;
            x = y = 0;
            width = 0;
            height = 0;
            xform = 0;

            magn = 1.0;
            angle = 0.0;
            font = 0;
            hj = 0;  // left justify
            vj = 2;  // bottom justify
            pwidth = 0;
            ptype = 1;
            reflection = false;
            abs_mag = false;
            abs_ang = false;
            gds_valid = false;
        }

    bool set(const Label*, DisplayMode, FileType);
    void setup_gds(DisplayMode);
    void transform(cTfmStack*);
    void scale(double);

    const char *text;
    int x, y;
    int width;
    int height;
    int xform;

    // gdsii only
    double magn;
    double angle;
    int font;
    int hj, vj;
    int pwidth;
    int ptype;
    bool reflection;
    bool abs_mag;
    bool abs_ang;
    bool gds_valid;  // true if this block filled in
};


// Struct used to pass file header attributes.
//
struct Attribute : public CDp
{
    Attribute(int val, char *txt, char *cmt) : CDp(0, val)
        {
            p_string = lstring::copy(txt);
            pComment = lstring::copy(cmt);
        }
    ~Attribute() { delete [] pComment; }
    Attribute *next() { return (Attribute*)p_next; }

    char *pComment;
};


// Description of a hierarchy root cell.
//
struct Topcell
{
    Topcell()
        {
            cellname = 0;
            numinst = 0;
            instances = 0;
        }

    const char *cellname;
    int numinst;
    Instance *instances;
};


// Struct used to pass layer information.
//
struct Layer
{
    Layer() { set_null(); }

    Layer(const char *n, int l, int d, int ix = 0)
        {
            name = n;
            layer = l;
            datatype = d;
            index = ix;
        }

    void set_null()
        {
            name = 0;
            layer = -1;
            datatype = -1;
            index = -1;
        }

    const char *name;
    int layer;
    int datatype;
    int index;
};


// Struct to check and see if a layer is ok to use for input.
//
struct Lcheck
{
    // Struct to keep track of a layer for input filtering.
    //
    struct lnam_t
    {
        lnam_t()
            {
                lname = 0;
                layer = -1;
                dtype = -1;
            }
        ~lnam_t() { delete [] lname; }

        bool compare(const char *name, int l, int dt)
            {
                if (name && lname)
                    return (!strcmp(name, lname));
                if (layer < 0 && dtype < 0)
                    return (false);
                if (l < 0 && dt < 0)
                    return (false);
                return ((l < 0 || layer < 0 || l == layer) &&
                    (dt < 0 || dtype < 0 || dt == dtype));
            }

        void set(char*);

    private:
        char *lname;
        int layer;
        int dtype;
    };

    Lcheck(bool, const char*);
    ~Lcheck() { delete [] layers; }

    bool match(const char *name, int layer, int datatype)
        {
            // The first pass will match the name, or the layer/datatype
            // to a pseudo-name in hex format.
            for (int i = 0; i < nlayers; i++) {
                if (layers[i].compare(name, layer, datatype))
                    return (true);
            }

            // The second pass will map the layer/datatype to a known
            // layer name, then try to match the name.
            if (!name) {
                name = FIO()->GetGdsInputLayer(layer, datatype, Physical);
                if (name) {
                    for (int i = 0; i < nlayers; i++) {
                        if (layers[i].compare(name, -1, -1))
                            return (true);
                    }
                }
            }
            return (false);
        }

    bool layer_ok(const char *name, int layer, int datatype)
        { return (useonly == match(name, layer, datatype)); }

private:
    bool useonly;       // If true, use only layers listed here, and if false,
                        // don't use any of the layers listed here.
    int nlayers;        // Number of layers listed.
    lnam_t *layers;     // The list.
};


// List element for cells, passed to merge control functions.
//
struct mitem_t
{
    mitem_t(const char *na)
        {
            next = 0;
            name = na;
            overwrite_phys = false;
            overwrite_elec = false;
        }

    static void destroy(mitem_t *m)
        {
            while (m) {
                mitem_t *mn = m->next;
                delete m;
                m = mn;
            }
        }

    mitem_t *next;
    const char *name;                   // cell name
    bool overwrite_phys;                // physical data
    bool overwrite_elec;                // electrical data
};


// Base class for header info, used with cCellHierDigest.
//
struct cv_header_info
{
    cv_header_info(FileType t, double u) : h_filetype(t), h_unit(u) { }
    virtual ~cv_header_info() { }

    FileType file_type()    const { return (h_filetype); }
    double unit()           const { return (h_unit); }

protected:
    FileType h_filetype;
    double h_unit;
};


// Interface definition to query filterred tables during conversion.

struct cv_fgif
{
    virtual ~cv_fgif() { }

    // This returns whether of not to include an instance in output.
    virtual tristate_t get_flag(int) const = 0;
};

struct cv_bbif
{
    virtual ~cv_bbif() { }

    // This returns an effective bounding box for a cell.
    virtual const BBox *resolve_BB(symref_t**, unsigned int) = 0;

    // This returns an object containing instance use tables.
    virtual const cv_fgif *resolve_fgif(symref_t**, unsigned int) = 0;
};

// A collection of things used when reading via a CHD, makes use of the
// interface above.
//
struct cv_chd_state
{
    cv_chd_state()
        {
            s_ctab = 0;             // cell table, used when windowing
            s_chd = 0;              // source CHD
            s_symref = 0;           // current symref for reading
            s_gen = 0;              // instance generator for symref
            s_ct_item = 0;          // symref BB and instance use table
            s_phys_stab_bak = 0;    // backup for phys_sym_tab
            s_elec_stab_bak = 0;    // backup for elec_sym_tab
            s_ct_index = 0;         // ctab vector index
        }

    void setup(cCHD *tchd, cv_bbif *tctab, int ix, nametab_t *ptab,
            nametab_t *etab)
        {
            s_chd = tchd;
            s_symref = 0;
            s_gen = 0;
            s_ctab = tctab;
            s_ct_item = 0;
            s_ct_index = ix;
            s_phys_stab_bak = ptab;
            s_elec_stab_bak = etab;
        }

    cCHD *chd()                     { return (s_chd); }
    symref_t *symref()              { return (s_symref); }
    crgen_t *gen()                  { return (s_gen); }
    cv_bbif *ctab()                 { return (s_ctab); }
    const cv_fgif *ct_item()        { return (s_ct_item); }
    nametab_t *phys_stab_bak()      { return (s_phys_stab_bak); }
    nametab_t *elec_stab_bak()      { return (s_elec_stab_bak); }
    int ct_index()                  { return (s_ct_index); }

    void push_state(symref_t*, nametab_t*,  cv_chd_state*);
    void pop_state(cv_chd_state*);
    tristate_t get_inst_use_flag();

private:
    cCHD            *s_chd;
    symref_t        *s_symref;
    crgen_t         *s_gen;
    cv_bbif         *s_ctab;
    const cv_fgif   *s_ct_item;
    nametab_t       *s_phys_stab_bak;
    nametab_t       *s_elec_stab_bak;
    int             s_ct_index;
};

// Action enum for file readers,
enum cvOpenMode
{
    cvOpenModeNone,
    cvOpenModeDb,
    cvOpenModeTrans,
    cvOpenModePrint
};
//
//  cvOpenModeNone          Action unspecified.
//  cvOpenModeDb            We are reading input and adding new cells
//                           to the database.
//  cvOpenModeTrans         Direct translation mode.
//  cvOpenModePrint         Print diagnostic output.

// Print message after this many bytes.
#define UFB_INCR 100000

// Function type for missing-symref callback.
typedef bool(*cv_resolve_cb)(cv_in*, symref_t*, void*, CDs**);

struct cv_in : public cTfmStack
{
    cv_in(bool);  // arg allows layer mapping if true
    virtual ~cv_in();

    // setup functions
    virtual bool setup_source(const char*, const cCHD* = 0) = 0;
    virtual bool setup_destination(const char*, FileType, bool) = 0;
    virtual bool setup_ascii_out(const char*, uint64_t=0, uint64_t=0,
        int=-1, int=-1) = 0;
    virtual bool setup_backend(cv_out*) = 0;

    void set_to_database() { in_action = cvOpenModeDb; }

    // Swap in the state from another cv_in, used for CGD stream
    // switching.
    void init_to(cv_in *in)
        {
            char *fn = in_filename;
            FileType ft = in_filetype;
            *this = *in;
            in_filename = fn;
            in_filetype = ft;
        }

    void init_bak()
        {
            // Zero the pointers, since these are copies.
            in_symref = 0;
            in_prpty_list = 0;
            in_phys_sym_tab = 0;
            in_elec_sym_tab = 0;
            in_tf_list = 0;
            in_sdesc = 0;
            in_out = 0;
            in_lcheck = 0;
            in_alias = 0;
            in_layer_alias = 0;

            in_chd_state.setup(0, 0, 0, 0, 0);

            in_print_fp = 0;
            in_phys_info = 0;
            in_elec_info = 0;
            in_over_tab = 0;
        }

    // main entry for reading
    virtual bool parse(DisplayMode, bool, double, bool = false,
        cvINFO = cvINFOtotals) = 0;

    // CHD functions
    virtual bool chd_read_header(double) = 0;
    virtual bool chd_read_cell(symref_t*, bool, CDs** = 0) = 0;
    virtual cv_header_info *chd_get_header_info() = 0;
    virtual void chd_set_header_info(cv_header_info*) = 0;

    cCHD *new_chd();
    bool chd_setup(cCHD*, cv_bbif*, int, DisplayMode, double);
    void chd_finalize();
    OItype chd_process_override_cell(symref_t*);
    OItype chd_output_cell(CDs*, CDcellTab* = 0);

    cCHD *cur_chd() { return (in_chd_state.chd()); }
    void set_tf_list(unsigned char *s) { in_tf_list = s; }

    // misc. entries
    virtual bool has_electrical() = 0;
    virtual FileType file_type() = 0;
    virtual void add_header_props(CDs*) = 0;
    virtual bool has_header_props() = 0;
    virtual OItype has_geom(symref_t*, const BBox* = 0) = 0;

    // alias table manipulations
    void assign_alias(FIOaliasTab *t) { in_alias = t; }
    FIOaliasTab *extract_alias()
        { FIOaliasTab *t = in_alias; in_alias = 0; return (t); }
    const char *alias(const char *name)
        { return (in_alias ? in_alias->alias(name) : name); }
    void read_alias(const char *dst)
        { if (in_alias) in_alias->read_alias(dst); }
    void dump_alias(const char *dst)
        { if (in_alias) in_alias->dump_alias(dst); }

    // translation accessories
    void begin_log(DisplayMode);
    void end_log();
    void show_feedback();
    void set_show_progress(bool p) { in_show_progress = p; }
    cv_out *backend()
        {
            // caller takes ownership!
            in_own_in_out = false;
            return (in_out);
        }
    cv_out *peek_backend()
        {
            // ownership not changed, caller beware!
            return (in_out);
        }
    void set_no_open_lib(bool b) { in_cv_no_openlib = b; }
    bool no_open_lib() { return (in_cv_no_openlib); }
    void set_no_end_lib(bool b) { in_cv_no_endlib = b; }
    bool no_end_lib() { return (in_cv_no_endlib); }
    void set_transform_level(int l) { in_transform = l; }
    void set_ignore_instances(bool b) { in_ignore_inst = b; }

    // hierarchy checking
    const char *handle_lib_clash(const char*);
    bool mark_references(stringlist**);
    void mark_top(stringlist**, stringlist**);
    void set_no_test_empties(bool b) { in_no_test_empties = b; }

    // scaling
    double get_ext_phys_scale() { return (in_ext_phys_scale); }
    int scale(int xx)
        { return (in_needs_mult ? mmRnd(in_scale*xx) : xx); }

    // check for interrupt
    bool was_interrupted()
        { bool t = in_interrupted; in_interrupted = false; return (t); }

    // windowing and flattening setup
    void set_area_filt(bool on, const BBox *BB)
        { in_areafilt = on; if (BB) in_cBB = *BB; }
    inline void set_flatten(int, bool);  // Deferred definition below.
    void set_clip(bool on) { in_clip = on; }
    void set_keep_clipped_text(bool b) { in_keep_clip_text = b; }

    // via/pcell sub-master checking
    CDcellName check_sub_master(CDcellName);
    SymTab *get_submaster_tab()
        {
            SymTab *tab = in_submaster_tab;
            in_submaster_tab = 0;
            return (tab);
        }
    void set_submaster_tab(SymTab *tab)     { in_submaster_tab = tab; }

    // name list accessories
    symref_t *get_symref(const char*, DisplayMode);
    void add_symref(symref_t*, DisplayMode);
    nametab_t *get_sym_tab(DisplayMode m)
        { return (m == Physical ? in_phys_sym_tab : in_elec_sym_tab); }

    bool header_read() { return (in_header_read); }

    cv_info *info()
        { return (in_mode == Physical ? in_phys_info : in_elec_info); }

    DisplayMode cur_mode() { return (in_mode); }
    bool gzipped() { return (in_gzipped); }

protected:
    double      in_ext_phys_scale;  // applied overall scaling value
    double      in_scale;           // local scaling value
    double      in_phys_scale;      // for fixing elec mode properties
    uint64_t    in_bytes_read;      // bytes read so far
    uint64_t    in_fb_incr;         // user feedback monitor
    uint64_t    in_offset;          // offset of current record
    uint64_t    in_cell_offset;     // offset of current cell record

    cv_chd_state in_chd_state;      // CHD and related

    cvOpenMode  in_action;          // what to do with the cell
    DisplayMode in_mode;            // Physcial or Electrical
    FileType    in_filetype;        // type of input file
    char        *in_filename;       // source file name
    symref_t    *in_symref;         // for listing with subcells
    ticket_t    in_cref_end;        // for listing with subcells
    CDp         *in_prpty_list;     // object properties
    nametab_t   *in_phys_sym_tab;   // cell table
    nametab_t   *in_elec_sym_tab;   // cell table
    SymTab      *in_submaster_tab;  // pcell/via submasters created
    unsigned char *in_tf_list;      // transformation list
    bool        in_flatten;         // flattening mode
    bool        in_gzipped;         // file was gzipped
    bool        in_needs_mult;      // flag to indicate in_scale != 1.0
    bool        in_listonly;        // list the cell offsets only
    bool        in_uselist;         // use file offsets list
    bool        in_areafilt;        // area test for objects
    bool        in_clip;            // clip to area
    bool        in_no_test_empties; // skip warning about empty cells
    bool        in_cv_no_openlib;   // skip open_lib output
    bool        in_cv_no_endlib;    // skip end_lib output
    bool        in_savebb;          // don't save objects, but record BB
    bool        in_header_read;     // file header data is valid
    bool        in_ignore_text;     // don't read text records
    bool        in_ignore_inst;     // don't read instance records
    bool        in_ignore_prop;     // don't read properties
    bool        in_interrupted;     // user interrupt;
    bool        in_show_progress;   // print user feedback
    bool        in_own_in_out;      // destroy in_out in destructor
    bool        in_keep_clip_text;  // don't omit clipped labels
    int         in_flatmax;         // max depth to flatten
    int         in_transform;       // flattening transform level
    CDs         *in_sdesc;          // current cell
    tm          in_cdate;           // save create date
    tm          in_mdate;           // save mod date
    cv_out      *in_out;            // translation output
    BBox        in_cBB;             // area filter
    Lcheck      *in_lcheck;         // layer filter
    FIOaliasTab *in_alias;          // cell name translations
    FIOlayerAliasTab *in_layer_alias; // layer name aliasing
    FILE        *in_print_fp;       // output for OpenModePrint
    uint64_t    in_print_start;     // offset for printing start
    uint64_t    in_print_end;       // offset for printing end
    int         in_print_reccnt;    // count of records to print
    int         in_print_symcnt;    // count of symbols to print
    bool        in_printing;        // true when printing
    bool        in_printing_done;   // printing done, should quit

    cv_info     *in_phys_info;      // statistics
    cv_info     *in_elec_info;      // statistics
    SymTab      *in_over_tab;       // overwrite info
};


// Table to keep track of the cv_in associated with each CHD.
//
struct chd_intab : public SymTab
{
    chd_intab() : SymTab(false, false)
        {
            nofree_in = 0;
            CD()->RegisterCreate("chd_intab");
        }

    ~chd_intab()
        {
            SymTabGen tgen(this, true);
            SymTabEnt *h;
            while ((h = tgen.next()) != 0) {
                cv_in *in = (cv_in*)h->stData;
                if (in != nofree_in)
                    delete in;
                delete h;
            }
            CD()->RegisterDestroy("chd_intab");
        }

    void insert(const cCHD *chd, cv_in *in)
        {
            add((unsigned long)chd, in, false);
        }

    cv_in *find(const cCHD *chd)
        {
            void *p = get_prv((unsigned long)chd);
            if (p == ST_NIL)
                return (0);
            return ((cv_in*)p);
        }

    // At most one data element can be saved in destructor.
    void set_no_free(const cv_in *in)
        {
            nofree_in = in;
        }

private:
    const cv_in *nofree_in;
};


// Visited cell table, with local string buffer.
//
struct vtab_t
{
    // List element for strings, locally allocated.
    struct vl_t
    {

        const char *tab_name()      { return (name); }
        vl_t *tab_next()            { return (next); }
        void set_tab_next(vl_t *n)  { next = n; }
        vl_t *tgen_next(bool)       { return (next); }

        const char *name;
        vl_t *next;
        int num;
    };

    vtab_t(bool loc) { vt_tab = 0; local = loc; }
    ~vtab_t() { delete vt_tab; }

    // Return the symbol index for string (0 or larger), or -1 if
    // not found.
    //
    int find(const char *string)
        {
            if (!string || !vt_tab)
                return (-1);
            vl_t *e = vt_tab->find(string);
            return (e ? e->num : -1);
        }

    // Add string to string table if not already there, and return a
    // pointer to the string table entry.
    //
    const char *add(const char *string, int k)
        {
            if (!string || k < 0)
                return (0);
            if (!vt_tab)
                vt_tab = new table_t<vl_t>;
            vl_t *e = vt_tab->find(string);
            if (!e) {
                e = vt_elts.new_element();
                e->set_tab_next(0);
                e->name = local ? vt_buf.new_string(string) : string;
                e->num = k;
                vt_tab->link(e, false);
                vt_tab = vt_tab->check_rehash();
            }
            return (e->name);
        }

    void clear()
        {
            delete vt_tab;
            vt_tab = 0;
            vt_elts.clear();
            vt_buf.clear();
        }

private:
    table_t<vl_t> *vt_tab;      // offset hash table
    eltab_t<vl_t> vt_elts;      // table element factory
    stbuf_t       vt_buf;       // string pool
    bool          local;        // local string storage
};


// Special argument to cCHD::write.
#define CHD_USE_VISITED (SymTab*)1

// Layer mapping check for cv_out::write_object.
enum cvLchk { cvLneedCheck, cvLok, cvLnogo };

// Interface passed to cCHD::readFlat, redirects output.
//
struct cv_backend
{
    cv_backend()
        {
            be_w2p = false;
            be_abort = false;
            CD()->RegisterCreate("cv_backend");
        }

    virtual ~cv_backend()
        {
            CD()->RegisterDestroy("cv_backend");
        }

    bool wire_to_poly() const       { return (be_w2p); }
    void set_wire_to_poly(bool b)   { be_w2p = b; }
    bool aborted() const            { return (be_abort); }

    // Handle geometry.
    virtual bool queue_layer(const Layer*, bool* = 0) = 0;
    virtual bool write_box(const BBox*) = 0;
    virtual bool write_poly(const Poly*) = 0;
    virtual bool write_wire(const Wire*) = 0;
    virtual bool write_text(const Text*) = 0;

    // Called when done.
    virtual void print_report() { }

protected:
    bool be_w2p;            // True if we would rather receive polys than
                            // wires, setting this can save computation if,
                            // e.g., creating zoids.
    bool be_abort;          // Set this on user abort.
};


// Class for output generation, used as base for the file writers.
//
struct cv_out : public cv_backend
{
    cv_out();
    virtual ~cv_out();

    void assign_alias(FIOaliasTab *t) { out_alias = t; }
    FIOaliasTab *extract_alias()
        { FIOaliasTab *t = out_alias; out_alias = 0; return (t); }
    const char *alias(const char *name)
        { return (out_alias ? out_alias->alias(name) : name); }
    void read_alias(const char *dst)
        { if (out_alias) out_alias->read_alias(dst); }
    void dump_alias(const char *dst)
        { if (out_alias) out_alias->dump_alias(dst); }

    int visited(const char *name)
        { return (out_visited ? out_visited->find(name) : -1); }
    void add_visited(const char *name)
        {
            if (out_visited)
                out_visited->add(name, out_symnum);
            out_symnum++;
        }

    bool was_interrupted()
        { bool t = out_interrupted; out_interrupted = false; return (t); }

    void set_no_struct(bool b)  { out_no_struct = b; }

    FileType filetype()         { return (out_filetype); }
    const char *filename()      { return (out_filename); }

    const BBox *cellBB()        { return (&out_cellBB); }

    // Functions for database conversion
    bool write_all(DisplayMode, double);
    bool write(const stringlist*, DisplayMode, double);
    bool write_flat(const char*, double, const BBox*, bool);
    bool write_multi_final(Topcell*);
    bool write_begin(double);
    bool write_begin_struct(const char*);
    bool write_symbol(const CDs*, bool = false);
    bool write_chd_refs();
    bool write_symbol_flat(const CDs*, const BBox *AOI, bool clip);
    bool write_instances(const CDs*);
    bool write_geometry(const CDs*);
    bool write_object_clipped(const CDo*, const BBox*, cvLchk*);
    bool check_set_layer(const CDl*, bool*);

    // These are also used for translation
    bool queue_properties(const CDs*);
    bool queue_properties(const CDo*);
    void clear_property_queue();
    CDp *set_property_queue(CDp *p)
        { CDp *pret = out_prpty; out_prpty = p; return (pret); }
    virtual bool set_destination(const char*) = 0;
    virtual bool set_destination(FILE*, void**, void**) = 0;
    virtual bool open_library(DisplayMode, double) = 0;
    virtual bool queue_property(int, const char*) = 0;
    virtual bool write_library(int, double, double, tm*, tm*, const char*) = 0;
    virtual bool write_struct(const char*, tm*, tm*) = 0;
    virtual bool write_end_struct(bool = false) = 0;

    // cv_backend interface
    virtual bool queue_layer(const Layer*, bool* = 0) = 0;
    virtual bool write_box(const BBox*) = 0;
    virtual bool write_poly(const Poly*) = 0;
    virtual bool write_wire(const Wire*) = 0;
    virtual bool write_text(const Text*) = 0;

    virtual bool write_sref(const Instance*) = 0;
    virtual bool write_endlib(const char*) = 0;
    virtual bool write_info(Attribute*, const char*) = 0;

    void set_transform(cTfmStack *stk)  { out_stk = stk; }

    // Instance size filtering, hack for image creation.
    bool size_test(const symref_t *p, const cTfmStack *stk)
        { return (out_size_thr > 0 && size_test_prv(p, stk)); }

    void set_size_thr(int sz) { out_size_thr = sz; }

protected:
    // Support functions for database conversion
    int scale(int xx)
        { return (out_needs_mult ? mmRnd(out_scale*xx) : xx); }
    virtual bool check_for_interrupt() { return (false); }
    virtual bool flush_cache() { return (true); }
    virtual bool write_header(const CDs*) = 0;
    virtual bool write_object(const CDo*, cvLchk*) = 0;

    bool size_test_prv(const symref_t*, const cTfmStack*);

    double          out_scale;              // scale factor
    double          out_phys_scale;         // for scaling elec properties
    uint64_t        out_byte_count;         // bytes written
    uint64_t        out_fb_thresh;          // event check monitor
    char            *out_filename;          // name of output file
    vtab_t          *out_visited;           // record written cells
    CDp             *out_prpty;             // property cache
    FIOaliasTab     *out_alias;             // alias table
    stringlist      *out_chd_refs;          // list of chd ref cells found
    cTfmStack       *out_stk;               // apply to odesc output
    Layer           out_layer;              // current layer desc
    BBox            out_cellBB;             // current cell BB
    int             out_size_thr;           // size threshold
    unsigned int    out_symnum;             // symbol definition counter
    unsigned int    out_rec_count;          // records written
    unsigned int    out_struct_count;       // structures written
    DisplayMode     out_mode;               // Physical or Electrical
    FileType        out_filetype;           // file type of output
    bool            out_needs_mult;         // out_scale != 1.0
    bool            out_in_struct;          // in structure context
    bool            out_interrupted;        // user interrupt
    bool            out_no_struct;          // skip struct beg/end recs
};


// Definition deferred until cv_out defined.
//
inline void
cv_in::set_flatten(int i, bool fltn)
{
    in_flatten = fltn;
    if (!fltn)
        in_flatmax = 0;
    else {
        in_flatmax = i < 0 ? 0 : i;
        if (in_out)
            in_out->set_transform(this);
    }
}


// Class for generating native symbol files
//
struct xic_out : public cv_out
{
    xic_out(const char*);
    ~xic_out();

    void set_file_ptr(FILE*);
    bool write_this_cell(CDs*, const char*, double);

    bool check_for_interrupt();
    bool write_header(const CDs*);
    bool write_object(const CDo*, cvLchk*);
    bool set_destination(const char*);
    bool set_destination(FILE*, void**, void**) { return (false); }
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

    bool set_destination_flat(const char*);

    void set_lib_device(bool t) { out_lib_device = t; }

    void write_dummy(const char*, DisplayMode);
    void make_native_lib(const char*, const char*);

private:
    bool fopen_tof(const char*, const char*);
    bool open_root();
    bool write_layer_rec();

    FILE        *out_symfp;         // symbol file pointer
    FILE        *out_rootfp;        // "root" pointer
    const char  *out_infilename;    // input file name
    const char  *out_destdir;       // directory for symbol files
    const char  *out_destpath;      // file path used when flattening
    vtab_t      *out_written;       // keep track of written files
    uint64_t    out_last_offset;    // per-file byte count
    char        *out_lname_temp;    // layer name copy
    bool        out_fp_set;         // skip opening file
    bool        out_lib_device;     // writing a library device
    bool        out_layer_written;  // layer record written
};

#endif


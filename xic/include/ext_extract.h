
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
 $Id: ext_extract.h,v 5.33 2017/03/17 04:34:58 stevew Exp $
 *========================================================================*/

#ifndef EXT_EXTRACT_H
#define EXT_EXTRACT_H


//
//  Device/subcircuit extraction structs.
//

#include "ext_permute.h"
#include "cd_terminal.h"  // for sTlist

struct sDevComp;
struct sDevDesc;
struct sDevInst;
struct sParamTab;
struct sDumpOpts;
struct sDevInstList;
struct sEinstList;
struct sSubcInst;
struct sVContact;
struct sElecNetList;
struct sLVSstat;
struct sPermGrpList;
struct RLsolver;

namespace ext_group {
    struct sGrpGen;
    struct sSubcLink;
    struct sSubcGen;
    struct Ufb;
}
namespace ext_duality {
    struct sSymBrk;
}


// The sDevContactDesc::c_level values.  The value is nonzero only for
// bulk contacts.
//
// BC_immed
//   Handle like regular contact.
// BC_skip
//   Don't attempt to extract contact, use dummy internally.
// BC_defer
//   Try to extract, but if fails use dummy.
//
enum BC_level { BC_immed = 0, BC_skip = 1, BC_defer = 2 };

// Describe a device-related electrical contact.
//
struct sDevContactDesc
{
    sDevContactDesc()
        {
            c_next = 0;
            c_name = 0;
            c_netname = 0;
            c_ld = 0;
            c_name_gvn = 0;
            c_netname_gvn = 0;
            c_lname = 0;
            c_multiple = false;
            c_bulk = false;
            c_level = BC_immed;
            c_el_index = -1;
            c_bulk_bloat = 0;
        }

    ~sDevContactDesc()
        {
            delete [] c_name_gvn;
            delete [] c_netname_gvn;
            delete [] c_lname;
        }

    static void destroy(sDevContactDesc *c)
        {
            while (c) {
                sDevContactDesc *cx = c;
                c = c->c_next;
                delete cx;
            }
        }

    char *checkEquiv(const sDevContactDesc*);
    bool init();
    bool setup_contact_indices(const sDevDesc*);

    static char *parse_contact(const char**, sDevDesc*);
    static char *parse_bulk_contact(const char**, sDevDesc*);

    sDevContactDesc *next()         const { return (c_next); }
    CDnetName name()                const { return (c_name); }
    CDnetName netname()             const { return (c_netname); }
    const char *name_gvn()          const { return (c_name_gvn); }
    const char *netname_gvn()       const { return (c_netname_gvn); }
    const char *lname()             const { return (c_lname); }
    CDl *ldesc()                    const { return (c_ld); }
    sLspec *lspec()                       { return (&c_lspec); }
    bool multiple()                 const { return (c_multiple); }
    bool is_bulk()                  const { return (c_bulk); }
    BC_level level()                const { return ((BC_level)c_level); }
    int elec_index()                const { return (c_el_index); }
    void set_elec_index(int i)            { c_el_index = i; }
    float bulk_bloat()              const { return (c_bulk_bloat); }

private:
    sDevContactDesc *c_next;
    CDnetName       c_name;         // contact name in string table
    CDnetName       c_netname;      // net name in string table
    CDl             *c_ld;          // conductor layer for contact
    char            *c_name_gvn;    // contact name string, as given
    char            *c_netname_gvn; // global net name for bulk defer, as given
    char            *c_lname;       // conductor layer name
    sLspec          c_lspec;        // contact specification
    bool            c_multiple;     // this contact can be replicated
    bool            c_bulk;         // this is a bulk contact
    char            c_level;        // bulk contact handling level
    signed char     c_el_index;     // index number of node prop.
    float           c_bulk_bloat;   // bloat for bulk contact
};


// Specify an instance of a device-related electrical contact.
//
struct sDevContactInst
{
    sDevContactInst()
        {
            ci_next = 0;
            ci_desc = 0;
            ci_fillfct = 0.0;
            ci_group = -1;
            ci_pnode = 0;
            ci_dev = 0;
            ci_name = 0;
        }

    sDevContactInst(sDevInst *d, sDevContactDesc *c)
        {
            ci_next = 0;
            ci_desc = c;
            ci_fillfct = 0.0;
            ci_group = -1;
            ci_pnode = 0;
            ci_dev = d;
            ci_name = 0;
        }

    sDevContactInst(const sDevContactInst &c)
        {
            ci_next = 0;
            ci_desc = c.ci_desc;
            ci_BB = c.ci_BB;
            ci_fillfct = c.ci_fillfct;
            ci_group = c.ci_group;
            ci_pnode = 0;
            ci_dev = 0;
            ci_name = 0;
        }

    static void destroy(sDevContactInst *ci)
        {
            while (ci) {
                sDevContactInst *cx = ci;
                ci = ci->ci_next;
                delete cx;
            }
        }

    // ext_device.cc
    int node(const sEinstList* = 0) const;
    CDp_cnode *node_prpty(const sEinstList* = 0) const;
    static sDevContactInst *dup_list(const sDevContactInst*, sDevInst*);
    void show(WindowDesc*, BBox* = 0) const;
    void set_term_loc(CDs*, CDl*) const;
    CDo *is_set(CDs*, CDl*) const;

    CDnetName cont_name() const
        {
            return (ci_name ? ci_name : (ci_desc ? ci_desc->name() : 0));
        }

    // This returns false for a "dummy" used for bulk-skip and deferred.
    bool cont_ok() const
        {
            return (ci_BB.right > ci_BB.left && ci_BB.top > ci_BB.bottom);
        }

    sDevContactInst *next()         const { return (ci_next); }
    void set_next(sDevContactInst *ci)    { ci_next = ci; }
    sDevContactDesc *desc()         const { return (ci_desc); }
    void set_desc(sDevContactDesc *c)     { ci_desc = c; }
    const BBox *cBB()               const { return (&ci_BB); }
    BBox *BB()                            { return (&ci_BB); }
    void set_BB(const BBox *bb)           { ci_BB = *bb; }
    double fillfct()                const { return (ci_fillfct); }
    void set_fillfct(double f)            { ci_fillfct = f; }
    int group()                     const { return (ci_group); }
    void set_group(int g)                 { ci_group = g; }
    CDp_cnode *pnode()              const { return (ci_pnode); }
    void set_pnode(CDp_cnode *p)          { ci_pnode = p; }
    sDevInst *dev()                 const { return (ci_dev); }
    void set_dev(sDevInst *di)            { ci_dev = di; }
    CDnetName name()                const { return (ci_name); }
    void set_name(CDnetName n)            { ci_name = n; }

private:
    sDevContactInst *ci_next;
    sDevContactDesc *ci_desc;       // reference to contact description
    BBox            ci_BB;          // bounding box of contact
    float           ci_fillfct;     // fraction of cBB actually covered
    int             ci_group;       // associated conductor group
    CDp_cnode       *ci_pnode;      // associated node property
    sDevInst        *ci_dev;        // back pointer to device instance
    CDnetName       ci_name;        // special name, if null use ci_desc
};


// Default for m_precision below.
#define DEF_PRECISION 2

// Structure describing a measurement to be performed on extracted
// device.
//
struct sMeasure
{
    sMeasure(char *n)
        {
            m_next = 0;
            m_name = n;
            m_tree = 0;
            m_result = 0;
            m_lvsword = 0;
            m_precision = 0;
        }

    ~sMeasure()
        {
            delete [] m_name;
            delete m_tree;
            delete [] m_lvsword;
        }

    static void destroy(sMeasure *m)
        {
            while (m) {
                sMeasure *mx = m;
                m = m->m_next;
                delete mx;
            }
        }

    double measure();
    void print_result(FILE*);
    int print_compare(sLstr*, const double*, sParamTab*, int*, int*);
    void print(FILE*);

    sMeasure *next()            const { return (m_next); }
    const char *name()          const { return (m_name); }
    ParseNode *tree()           const { return (m_tree); }
    siVariable *result()        const { return (m_result); }
    const char *lvsword()       const { return (m_lvsword); }
    int precision()             const { return (m_precision); }

    void set_tree(ParseNode *t)       { m_tree = t; }
    void set_result(siVariable *v)    { m_result = v; }
    void set_precision(int p)         { m_precision = p; }
    void set_next(sMeasure *m)        { m_next = m; }
    void set_lvsword(char *w)         { m_lvsword = w; }

private:
    sMeasure    *m_next;
    char        *m_name;        // name key for this measurement
    ParseNode   *m_tree;        // the specification
    siVariable  *m_result;      // storage for result
    char        *m_lvsword;     // token in SPICE line for parameter
    int         m_precision;    // LVS error if diff > 1/10^precision
};


// Struct that contains measurement primitive evaluation information.
//
struct sMprim
{
    sMprim(siVariable*, siVariable*, sMprim*);

    static void destroy(sMprim *m)
        {
            while (m) {
                sMprim *mx = m;
                m = m->mp_next;
                delete mx;
            }
        }

    char *split(const char*, char**, char**) const;
    void setnums(const char*, int*, int*) const;

    sMprim *next()              const { return (mp_next); }
    siVariable *variable()      const { return (mp_variable); }
    siVariable *extravar()      const { return (mp_extravar); }

    bool (sMprim::*evfunc)(sDevInst*);  // evaluation function

private:
    bool sections(sDevInst*);
    bool bodyAP(sDevInst*);
    bool bodyMinDim(sDevInst*);
    bool cAP(sDevInst*);
    bool cWidth(sDevInst*);
    bool cnWidth(sDevInst*);
    bool cbWidth(sDevInst*);
    bool cbnWidth(sDevInst*);
    bool resistance(sDevInst*);
    bool inductance(sDevInst*);
    bool mutual(sDevInst*);
    bool capacitance(sDevInst*);
    bool nop(sDevInst*) { return (false); };

    sMprim      *mp_next;
    siVariable  *mp_variable;   // main variable
    siVariable  *mp_extravar;   // secondary variable (if computing two)
};


// Device desctiption flags.
#define EXT_DEV_MERGE_PARALLEL  0x1
#define EXT_DEV_MERGE_SERIES    0x2
    // Flags applied to mergeable devices.

#define EXT_DEV_CONTS_OVERLAP   0x4
    // The device is vertical so contacts overlap in the x-y plane, as
    // a capacitor.

#define EXT_DEV_CONTS_VARIABLE  0x8
    // The device can have a variable number of contacts, mostly
    // applies to resistors with multiple contacts to single piece of
    // resistive film.

#define EXT_DEV_CONTS_MINDIM    0x10
    // The "MINDIM" measure is defined as the distance between the two
    // permutable contacts that the device must have.  This is mostly
    // a hack for MOS devices, so that the MINDIM will always be the
    // gate length.  Set automatically if EXT_DEV_MOS is set.

#define EXT_DEV_CONTS_MINDGVN   0x20
    // Indicates that the ContactMinDimen was given.  For MOS, if the
    // ContactMinDimen is explicitly set false, this will override the
    // automatic setting.

#define EXT_DEV_SIMPLE_MINDIM   0x40
    // Use simple-minded algorithm to find the (non-MOS) MINDIM,
    // otherwise use a statistical method.

#define EXT_DEV_MOS             0x80
    // The device is a MOS device.  This is set automatically for
    // devices with a prefix starting with 'm'.

#define EXT_DEV_PTYPE           0x100
    // The device is p-type (semiconductor).  This is set
    // automatically of the device name starts with 'p'.

#define EXT_DEV_NOTMOS_GIVEN    0x200
    // Tne NotMOS keyword was given in the device block.

#define EXT_DEV_NTYPE_GIVEN     0x400
    // The Ntype keyword was given in the device block.


// Describe a device.
//
struct sDevDesc
{
    friend char *sDevContactDesc::parse_contact(const char**, sDevDesc*);
    friend char *sDevContactDesc::parse_bulk_contact(const char**, sDevDesc*);

    sDevDesc()
        {
            d_next = 0;
            d_name = 0;
            d_prefix = 0;
            d_contacts = 0;
            d_prm1 = 0;
            d_prm2 = 0;
            d_finds = 0;
            d_prmconts = 0;
            d_measures = 0;
            d_netline = 0;
            d_netline1 = 0;
            d_model = 0;
            d_value = 0;
            d_param = 0;
            d_variables = 0;
            d_mprims = 0;
            d_bloat = 0.0;
            d_num_contacts = 0;
            d_depth = 0;
            d_flags = 0;
        }

    ~sDevDesc()
        {
            sDevContactDesc::destroy(d_contacts);
            stringlist::destroy(d_finds);
            stringlist::destroy(d_prmconts);
            sMeasure::destroy(d_measures);
            delete [] d_netline;
            delete [] d_netline1;
            delete [] d_model;
            delete [] d_value;
            delete [] d_param;
            siVariable::destroy(d_variables);
            sMprim::destroy(d_mprims);
        }

    // ext_extract.cc
    char *checkEquiv(const sDevDesc*);
    bool init();
    bool parse_measure(const char*);
    sMeasure *find_measure(const char*);
    sMprim *find_prim(const char*);
    siVariable *find_variable(const char*);
    siVariable *find_variable_last(const char*);
    sDevContactDesc *find_contact(const char*);
    XIrt find(CDs*, sDevInst**, const BBox* = 0, bool = false, bool = false);
    bool identify_contacts(CDs*, sDevInst*, const Zlist*, XIrt*);
    sDevContactInst *identify_contact(CDs*, sDevInst*, Zlist**, const Zlist*,
        sDevContactDesc*, int&, XIrt*);
    sDevContactInst *identify_bulk_contact(CDs*, sDevInst*, sDevContactDesc*,
        XIrt*);
    bool find_finds(CDs*, sDevInst*, XIrt*);
    bool set_lvs_word(const char*, char*);

    // ext_tech.cc
    bool parse_device(FILE*, bool);
    char *parse_device_line();
    bool finalize_device(bool);
    void print(FILE*);

    bool is_permute(CDnetName nm)
        {
            return (nm && (nm == d_prm1 || nm == d_prm2));
        }

    sDevContactDesc *find_contact(CDnetName nm)
        {
            for (sDevContactDesc *c = d_contacts; c; c = c->next())
                if (c->name() == nm)
                    return (c);
            return (0);
        }

    sDevDesc *next()            const { return (d_next); }
    void set_next(sDevDesc *d)        { d_next = d; }
    CDcellName name()           const { return (d_name); }
    const char *prefix()        const { return (d_prefix); }
    sDevContactDesc *contacts() const { return (d_contacts); }
    CDnetName permute_cont1()   const { return (d_prm1); }
    CDnetName permute_cont2()   const { return (d_prm2); }
    sLspec *body()                    { return (&d_body); }
    stringlist *finds()         const { return (d_finds); }
    stringlist *prmconts()      const { return (d_prmconts); }
    double bloat()              const { return (d_bloat); }
    sMeasure *measures()        const { return (d_measures); }
    const char *netline()       const { return (d_netline); }
    const char *netline1()      const { return (d_netline1); }
    const char *model()         const { return (d_model); }
    const char *value()         const { return (d_value); }
    const char *param()         const { return (d_param); }
    siVariable *variables()     const { return (d_variables); }
    sMprim *mprims()            const { return (d_mprims); }
    int num_contacts()          const { return (d_num_contacts); }
    int depth()                 const { return (d_depth); }
    bool merge_parallel()       const
        { return (d_flags & EXT_DEV_MERGE_PARALLEL); }
    bool merge_series()         const
        { return (d_flags & EXT_DEV_MERGE_SERIES); }
    bool overlap_conts()        const
        { return (d_flags & EXT_DEV_CONTS_OVERLAP); }
    bool variable_conts()       const
        { return (d_flags & EXT_DEV_CONTS_VARIABLE); }
    bool contacts_min_dim()     const
        { return (d_flags & EXT_DEV_CONTS_MINDIM); }
    bool contacts_min_dim_given()     const
        { return (d_flags & EXT_DEV_CONTS_MINDGVN); }
    bool simple_min_dim()       const
        { return (d_flags & EXT_DEV_SIMPLE_MINDIM); }
    bool is_mos()               const
        { return (d_flags & EXT_DEV_MOS); }
    bool is_ptype()             const
        { return (d_flags & EXT_DEV_PTYPE); }
    bool is_nmos()              const
        { return ((d_flags & EXT_DEV_MOS) && !(d_flags & EXT_DEV_PTYPE)); }
    bool is_pmos()              const
        { return ((d_flags & EXT_DEV_MOS) && (d_flags & EXT_DEV_PTYPE)); }

private:
    bool check_vars();
    sMprim *find_prim(siVariable*);
    void add_prims();

    void set_contacts(sDevContactDesc *c) { d_contacts = c; }
    void set_variable_conts(bool b)
        {
            if (b)
                d_flags |= EXT_DEV_CONTS_VARIABLE;
            else
                d_flags &= ~EXT_DEV_CONTS_VARIABLE;
        }

    sDevDesc        *d_next;
    CDcellName      d_name;             // device name, cellname string table
    char            *d_prefix;          // SPICE prefix for device
    sDevContactDesc *d_contacts;        // list of contacts
    sLspec          d_body;             // body specification
    CDnetName       d_prm1;             // first permute name
    CDnetName       d_prm2;             // second permute name
    stringlist      *d_finds;           // sub-devices list
    stringlist      *d_prmconts;        // two contact names that can permute
    sMeasure        *d_measures;        // list of things to measure
    char            *d_netline;         // format for alternative netlist line
    char            *d_netline1;        // another alternative format
    char            *d_model;           // model property format
    char            *d_value;           // value property format
    char            *d_param;           // param property format
    siVariable      *d_variables;       // measure variables
    sMprim          *d_mprims;          // measurement primitives list
    float           d_bloat;            // enlarge body bounding box
    unsigned char   d_num_contacts;     // number of contacts, 0 if variable
    unsigned char   d_depth;            // search depth for structure
    unsigned short  d_flags;            // misc. settings
};


// The mstatus field in sDevInst.
enum {
    MS_NONE,
    MS_PARALLEL,
    MS_SERIES,
    MS_SPLIT,
    MS_SPLIT_NOFREE
};
//
// MS_NONE           not a compound device
// MS_PARALLEL       compound device, components in parallel
// MS_SERIES         compound device, components in series
// MS_SPLIT          device created in split_devices()
// MS_SPLIT_NOFREE   above, but mylti_devices should not be freed


// Array of precomputed primitives
//
struct sPreCmp
{
    sPreCmp() { which = 0; val = 0.0; }

    const char *which;
    float val;
};


// Specify an instance of a device.
//
struct sDevInst
{
    // bodyLayer argument
    enum bl_type { bl_resis, bl_cap, bl_induct };

    sDevInst(int i, sDevDesc *d, CDs *s)
        {
            di_next = 0;
            di_desc = d;
            di_sdesc = s;
            di_contacts = 0;
            di_fdevs = 0;
            di_index = i;
            di_spindex = 0;
            di_dual = 0;
            di_barea = 0.0;
            di_bperim = 0;
            di_bmindim = 0;
            di_bdepth = 0;
            di_fnum = 0;
            di_mvalues = 0;
            di_mvalues_permuted = 0;
            di_precmp = 0;
            di_multi_devs = 0;
            di_mstatus = MS_NONE;
            di_displayed = false;
            di_nomerge = false;
            di_nm_checked = false;
            di_merged = false;
        }

    ~sDevInst()
        {
            sDevContactInst::destroy(di_contacts);
            delete [] di_fdevs;
            delete [] di_mvalues;
            delete [] di_mvalues_permuted;
            delete [] di_precmp;
            if (di_mstatus != MS_SPLIT_NOFREE)
                sDevInst::destroy(di_multi_devs);
        }

    static void destroy(sDevInst *d)
        {
            while (d) {
                sDevInst *dx = d;
                d = d->di_next;
                delete dx;
            }
        }

    // ext_device.cc
    void show(WindowDesc*, BBox* = 0) const;
    void print(FILE*, bool);
    bool print_net(FILE*, const sDumpOpts*);
    bool net_line(char**, const char*, PhysSpicePrintMode);
    int print_compare(sLstr*, int*, int*);
    bool set_properties();
    CDl *body_layer(bl_type) const;
    sDevContactInst *find_contact(const char*) const;
    void clip_contacts() const;
    void permute();
    bool permute_params();
    bool is_parallel(sDevInst*) const;
    bool no_merge();
    sDevInst *copy(const cTfmStack*, int* = 0) const;
    bool measure();
    bool save_measures_in_prpty();
    bool get_measures_from_property();
    void destroy_measures_property() const;
    bool setup_dev_layer(CDl*) const;
    sDevInstList *getdev(BBox*, bool*);
    void insert_parallel(sDevInst*);
    void insert_series(sDevInst*);
    RLsolver *setup_squares(bool) const;

    sDevContactInst *find_contact(CDnetName name) const
        {
            if (name) {
                for (sDevContactInst *ci = di_contacts; ci; ci = ci->next()) {
                    if (ci->cont_name() == name)
                        return (ci);
                }
            }
            return (0);
        }

    // Find the n'th contact in the device descriptor, return the
    // corresponding contact instance.  This accounts for permutations.
    //
    sDevContactInst *find_contact(int n) const
        {
            sDevContactDesc *c = di_desc->contacts();
            while (n-- && c)
                c = c->next();
            if (!c)
                return (0);
            for (sDevContactInst *ci = di_contacts; ci; ci = ci->next()) {
                if (ci->desc() == c)
                    return (ci);
            }
            return (0);
        }

    // Return true if all device terminals are tied to the same group.
    //
    bool is_shorted() const
        {
            if (di_contacts->group() < 0)
                return (false);
            for (sDevContactInst *c = di_contacts->next(); c;
                    c = c->next()) {
                if (c->group() != di_contacts->group())
                    return (false);
            }
            return (true);
        }

    // Return the indicated precomputed value, if found.  The mkw is
    // one of the static strings from Mkw.
    //
    bool find_precmp(const char *mkw, double *val) const
        {
            if (!di_precmp)
                return (false);
            for (sPreCmp *p = di_precmp; p->which; p++) {
                if (p->which == mkw) {
                    *val = p->val;
                    return (true);
                }
            }
            return (false);
        }

    // Return the number of contacts to the device.
    //
    int count_contacts() const
        {
            int cnt = 0;
            for (sDevContactInst *c = di_contacts; c; c = c->next())
                cnt++;
            return (cnt);
        }


    // Return a total count of the number of component devices.
    //
    int count_sections() const
        {
            if (di_mstatus != MS_PARALLEL && di_mstatus != MS_SERIES)
                return (1);
            int cnt = 0;
            for (sDevInst *d = di_multi_devs; d; d = d->next())
                cnt += d->count_sections();
            return (cnt);
        }

    bool is_nmos()                      const { return (di_desc->is_nmos()); }
    bool is_pmos()                      const { return (di_desc->is_pmos()); }

    sDevInst *next()                    const { return (di_next); };
    void set_next(sDevInst *n)                { di_next = n; }
    sDevDesc *desc()                    const { return (di_desc); }
    CDs *celldesc()                     const { return (di_sdesc); }
    void set_celldesc(CDs *sd)                { di_sdesc = sd; }
    sDevContactInst *contacts()         const { return (di_contacts); }
    void set_contacts(sDevContactInst *c)     { di_contacts = c; }
    sDevInst **fdevs()                  const { return (di_fdevs); }
    void set_fdevs(sDevInst **f)              { di_fdevs = f; }
    int index()                         const { return (di_index); }
    void set_index(int i)                     { di_index = i; }
    int spindex()                       const { return (di_spindex); }
    void set_spindex(int i)                   { di_spindex = i; }
    sEinstList *dual()                  const { return (di_dual); }
    void set_dual(sEinstList *d)              { di_dual = d; }
    const BBox *BB()                    const { return (&di_BB); }
    const BBox *bBB()                   const { return (&di_bBB); }
    void set_BB(const BBox *tBB)              { if (tBB) di_BB = *tBB; }
    void add_BB(const BBox *tBB)              { if (tBB) di_BB.add(tBB); }
    void set_bBB(const BBox *tBB)             { if (tBB) di_bBB = *tBB; }
    void add_bBB(const BBox *tBB)             { if (tBB) di_bBB.add(tBB); }
    void bloat_BB(int b)                      { di_BB.bloat(b); }
    double barea()                      const { return (di_barea); }
    void set_barea(double a)                  { di_barea = a; }
    int bperim()                        const { return (di_bperim); }
    void set_bperim(int p)                    { di_bperim = p; }
    int bmindim()                       const { return (di_bmindim); }
    void set_bmindim(int i)                   { di_bmindim = i; }
    int bdepth()                        const { return (di_bdepth); }
    void set_bdepth(int d)                    { di_bdepth = d; }
    int fnum()                          const { return (di_fnum); }
    void set_fnum(int n)                      { di_fnum = n; }
    bool displayed()                    const { return (di_displayed); }
    void set_displayed(bool b)                { di_displayed = b; }
    sDevInst *multi_devs()              const { return (di_multi_devs); }
    void set_multi_devs(sDevInst *d)          { di_multi_devs = d; }
    sPreCmp *precmp()                   const { return (di_precmp); }
    void set_precmp(sPreCmp *p)               { di_precmp = p; }
    unsigned int mstatus()              const { return (di_mstatus); }
    void set_mstatus(unsigned int m)          { di_mstatus = m; }
    void set_no_merge(bool b)                 { di_nomerge = b; }
    bool merged()                       const { return (di_merged); }

private:
    void outline(WindowDesc*, BBox*) const;
    void cache_measures();
    CDo *get_my_data_box() const;

    sDevInst        *di_next;
    sDevDesc        *di_desc;           // reference to device description
    CDs             *di_sdesc;          // cell desc containing this
    sDevContactInst *di_contacts;       // list of contacts
    sDevInst        **di_fdevs;         // 'find' component device list
    int             di_index;           // reference count for instance
    int             di_spindex;         // name index assigned per prefix
    sEinstList      *di_dual;           // pointer to electrical device
    BBox            di_BB;              // instance bounding box
    BBox            di_bBB;             // body bounding box
    float           di_barea;           // body area (sq. microns)
    int             di_bperim;          // body perimeter (internal units)
    int             di_bmindim;         // smallest body zoid width or height
    short           di_bdepth;          // body depth from flatten
    short           di_fnum;            // size of fdevs
    float           *di_mvalues;        // cache for measured values
    float           *di_mvalues_permuted; // cache for measured values,
                                        //  after permute
    sPreCmp         *di_precmp;         // precomputed primitives
    sDevInst        *di_multi_devs;     // pointer to components of compound
                                        //  device
    unsigned char   di_mstatus;         // multiple component status
    bool            di_displayed;       // device is highlighted in display
    bool            di_nomerge;         // do not merge this device
    bool            di_nm_checked;      // we've searched for NOMERGE prpty
    bool            di_merged;          // device has been merged away
};


// List physical device instantiations, passed to/returned from
// functions.
//
struct sDevInstList
{
    sDevInstList(sDevInst *p, sDevInstList *n)
        {
            next = n;
            dev = p;
        }

    static void destroy(sDevInstList *l)
        {
            while (l) {
                sDevInstList *x = l;
                l = l->next;
                delete x;
            }
        }

    sDevInstList    *next;
    sDevInst        *dev;
};


// Physical contact list.
//
struct sDevContactList
{
    sDevContactList(sDevContactInst *c, sDevContactList *n)
        {
            dc_next = n;
            dc_contact = c;
        }

    static void destroy(sDevContactList *d)
        {
            while (d) {
                sDevContactList *x = d;
                d = d->dc_next;
                delete x;
            }
        }

    sDevContactList *next()             const { return (dc_next); }
    sDevContactInst *contact()          const { return (dc_contact); }

    void set_next(sDevContactList *n)         { dc_next = n; }
    void set_contact(sDevContactInst *c)      { dc_contact = c; }

private:
    sDevContactList *dc_next;
    sDevContactInst *dc_contact;
};


// List of electrical instance descs and duals.
//
struct sEinstList
{
    sEinstList(CDc *c, int i, sEinstList *n)
        {
            el_next = n;
            el_cdesc = c;
            el_parallel = 0;
            el_dual.subc = 0;
            el_vec_ix = i;
        }

    ~sEinstList()
        {
            destroy(el_parallel);
        }

    static void destroy(sEinstList *e)
        {
            while (e) {
                sEinstList *x = e;
                e = e->el_next;
                delete x;
            }
        }

    // ext_device.cc
    char *instance_name() const;
    bool is_parallel(const sEinstList*) const;
    void setup_eval(sParamTab**, double**) const;
    static sEinstList *sort(sEinstList*);
    const CDp_cnode* const *nodes(unsigned int*);
    const int *permutes(const sDevDesc*, int*);

    sEinstList *next()          const { return (el_next); }
    CDc *cdesc()                const { return (el_cdesc); }
    sEinstList *sections()      const { return (el_parallel); }
    int cdesc_index()           const { return (el_vec_ix); }
    sDevInst *dual_dev()        const { return (el_dual.dev); }
    sSubcInst *dual_subc()      const { return (el_dual.subc); }

    void set_next(sEinstList *n)      { el_next = n; }
    void set_sections(sEinstList *s)  { el_parallel = s; }
    void set_dual(sDevInst *d)        { el_dual.dev = d; }
    void set_dual(sSubcInst *s)       { el_dual.subc = s; }

private:
    sEinstList *el_next;
    CDc *el_cdesc;
    sEinstList *el_parallel;    // Parallel electrical devices.
    union {
        sDevInst *dev;
        sSubcInst *subc;
    } el_dual;
    int el_vec_ix;              // Index, if instance is vectorized.
};


// List of device instances with common prefix.
//
struct sDevPrefixList
{
    sDevPrefixList(sDevInst *p, sDevPrefixList *n)
        {
            p_next = n;
            p_devs = p;
        }

    ~sDevPrefixList()
        {
            sDevInst::destroy(p_devs);
        }

    static void destroy(sDevPrefixList *p)
        {
            while (p) {
                sDevPrefixList *x = p;
                p = p->p_next;
                delete x;
            }
        }

    sDevPrefixList *next()          const { return (p_next); }
    sDevInst *devs()                const { return (p_devs); }

    void set_next(sDevPrefixList *n)      { p_next = n; }
    void set_devs(sDevInst *d)            { p_devs = d; }

private:
    sDevPrefixList  *p_next;
    sDevInst        *p_devs;
};


// List physical/electrical device instantiations, all devices have
// the same name.
//
struct sDevList
{
    sDevList(sDevInst *p, sDevList *n)
        {
            dl_next = n;
            dl_prefixes = 0;
            dl_edevs = 0;
            add(p);
        }

    ~sDevList()
        {
            sDevPrefixList::destroy(dl_prefixes);
            sEinstList::destroy(dl_edevs);
        }

    static void destroy(sDevList *d)
        {
            while (d) {
                sDevList *x = d;
                d = d->dl_next;
                delete x;
            }
        }

    CDcellName devname() const
        {
            if (dl_prefixes && dl_prefixes->devs())
                return (dl_prefixes->devs()->desc()->name());
            return (0);
        }

    // ext_device.cc
    void add(sDevInst*);
    sDevInst *find_in_list(sDevInst*);

    sDevList *next()            const { return (dl_next); }
    sDevPrefixList *prefixes()  const { return (dl_prefixes); }
    sEinstList *edevs()         const { return (dl_edevs); }

    void set_next(sDevList *d)              { dl_next = d; }
    void set_prefixes(sDevPrefixList *p)    { dl_prefixes = p; }
    void set_edevs(sEinstList *e)           { dl_edevs = e; }

private:
    sDevList        *dl_next;
    sDevPrefixList  *dl_prefixes;   // device instances per prefix
    sEinstList      *dl_edevs;      // electrical devices with same name
};


struct sSubcContactInst
{
    sSubcContactInst(int p, int c, sSubcInst *s, sSubcContactInst *n)
        {
            sci_next = n;
            sci_subc = s;
            sci_parent_group = p;
            sci_subc_group = c;
        }

    static void destroy(sSubcContactInst *s)
        {
            while (s) {
                sSubcContactInst *x = s;
                s = s->sci_next;
                delete x;
            }
        }

    // ext_duality.cc 
    int node(const sEinstList* = 0) const;

    // ext_extract.cc
    bool is_wire_only() const;
    bool is_global() const;

    sSubcContactInst *next()        const { return (sci_next); }
    sSubcInst *subc()               const { return (sci_subc); }
    int parent_group()              const { return (sci_parent_group); }
    int subc_group()                const { return (sci_subc_group); }
    int *subc_group_addr()                { return (&sci_subc_group); }

    void set_next(sSubcContactInst *n)    { sci_next = n; }
    void set_parent_group(int g)          { sci_parent_group = g; }
    void set_subc_group(int g)            { sci_subc_group = g; }

private:
    sSubcContactInst    *sci_next;
    sSubcInst           *sci_subc;
    int                 sci_parent_group;
    int                 sci_subc_group;
};


// List the subcircuit connections, these are extracted from the
// physical database.
//
struct sSubcContactList
{
    sSubcContactList(sSubcContactInst *c, sSubcContactList *n)
        {
            sc_next = n;
            sc_contact = c;
        }

    static void destroy(sSubcContactList *s)
        {
            while (s) {
                sSubcContactList *x = s;
                s = s->sc_next;
                delete x;
            }
        }

    sSubcContactList *next()            const { return (sc_next); }
    sSubcContactInst *contact()         const { return (sc_contact); }

    void set_next(sSubcContactList *n)        { sc_next = n; }
    void set_contact(sSubcContactInst *c)     { sc_contact = c; }

private:
    sSubcContactList    *sc_next;
    sSubcContactInst    *sc_contact;
};


// Struct to set a reference terminal ordering and permutations for
// subcircuit connections.
//
struct sSubcDesc
{
    sSubcDesc(const sSubcInst*);

    ~sSubcDesc()
        {
            delete [] sd_array;
        }

    // ext_extract.cc
    void sort_and_update(sSubcInst*);

    CDs *master()               const { return ((CDs*)(sd_sdescF & ~0x1)); }
    unsigned int num_contacts() const { return (sd_array[0]); }

    int contact(unsigned int ix) const
        {
            if (ix >= (unsigned int)num_contacts())
                return (-1);
            return (sd_array[ix + 1]);
        }

    unsigned int num_groups()   const { return (sd_array[num_contacts()+1]); }

    int group_size(unsigned int i)
        {
            if (i >= num_groups())
                return (0);
            unsigned int off = num_contacts() + 2;
            for (unsigned int k = 0; k < i; k++) {
                int n = sd_array[off];
                off += n+2;
            }
            return (sd_array[off]);
        }

    // return pointer to one past the end of array data.
    unsigned int group_end()
        {
            unsigned int off = num_contacts() + 2;
            for (unsigned int k = 0; k < num_groups(); k++) {
                int n = sd_array[off];
                off += n+2;
            }
            return (off);
        }

    int group_type(unsigned int i)
        {
            if (i >= num_groups())
                return (0);
            unsigned int off = num_contacts() + 2;
            for (unsigned int k = 0; k < i; k++) {
                int n = sd_array[off];
                off += n+2;
            }
            return (sd_array[off + 1]);
        }

    int *group_ary(unsigned int i)
        {
            if (i >= num_groups())
                return (0);
            unsigned int off = num_contacts() + 2;
            for (unsigned int k = 0; k < i; k++) {
                int n = sd_array[off];
                off += n+2;
            }
            return (sd_array + off + 2);
        }

    bool setup_permutes()
        {
            if (!(sd_sdescF & 1)) {
                find_and_set_permutes();
                sd_sdescF |= 1;
            }
            return (num_groups() > 0);
        }

private:
    // ext_duality.cc
    void find_and_set_permutes();

    unsigned long   sd_sdescF;  // master and "hidden" flag bit
    int             *sd_array;  // address of array of group numbers

    // The array contains tables of integers, providing:
    // 1) The number of contacts.
    // 2) A list of the contact group numbers, in order of use.
    // 3) The number of permutation groups (can be 0, below are optional).
    // 4) The groups: size, type, elements.
};


struct sSubcInst
{
    sSubcInst(CDc *c, int x, int y, int i, sSubcInst *n)
        {
            sc_next = n;
            sc_cdesc = c;
            sc_contacts = 0;
            sc_glob_conts = 0;
            sc_dual = 0;
            sc_permutes = 0;
            sc_ix = x;
            sc_iy = y;
            sc_uid = i;
            sc_index = 0;
        }

    // ext_extract.cc
    sSubcInst(sVContact*, int, sSubcInst*);

    ~sSubcInst()
        {
            sSubcContactInst::destroy(sc_contacts);
            sSubcContactInst::destroy(sc_glob_conts);
            sExtPermGrp<int>::destroy(sc_permutes);
        }

    // ext_duality.cc
    void clear_duality();
    void place_phys_terminals();
    void pre_associate();
    void post_associate();

    // ext_extract.cc
    sSubcContactInst *add(int, int);
    sSubcInst *copy(CDc*, int*);
    void update_template();
    char *instance_name();

    static void destroy(sSubcInst *s)
        {
            while (s) {
                sSubcInst *x = s;
                s = s->sc_next;
                delete x;
            }
        }

    sSubcInst *next()               const { return (sc_next); }
    CDc *cdesc()                    const { return (sc_cdesc); }
    sSubcContactInst *contacts()    const { return (sc_contacts); }
    sSubcContactInst *global_contacts() const { return (sc_glob_conts); }
    sEinstList *dual()              const { return (sc_dual); }
    sExtPermGrp<int> *permutes()    const { return (sc_permutes); }
    int ix()                        const { return (sc_ix); }
    int iy()                        const { return (sc_iy); }
    int uid()                       const { return (sc_uid); }
    int index()                     const { return (sc_index); }

    bool iscopy() const
        {
            return (sc_cdesc && sc_cdesc->is_copy());
        }

    void set_next(sSubcInst *n)           { sc_next = n; }
    void set_contacts(sSubcContactInst *c) { sc_contacts = c; }
    void set_dual(sEinstList *el)         { sc_dual = el; }
    void set_uid(int i)                   { sc_uid = i; }
    void set_index(int i)                 { sc_index = i; }

private:
    sSubcInst           *sc_next;
    CDc                 *sc_cdesc;      // corresponding physical subcell
    sSubcContactInst    *sc_contacts;   // contact list
    sSubcContactInst    *sc_glob_conts; // global contact list
    sEinstList          *sc_dual;       // electrical dual
    sExtPermGrp<int>    *sc_permutes;   // permutable subc contacts
    int                 sc_ix;          // indices into array
    int                 sc_iy;
    int                 sc_uid;         // absolute unique index of instance
    int                 sc_index;       // unique index for this master
};


struct sSubcInstList
{
    sSubcInstList(sSubcInst *i, const sSubcInstList *n)
        {
            sl_next = n;
            sl_inst = i;
        }

    static void destroy(const sSubcInstList *l)
        {
            while (l) {
                const sSubcInstList *x = l;
                l = l->sl_next;
                delete x;
            }
        }

    // When the list is used as a stack, this returns the top-level
    // group, or -1 if there is no connection.
    //
    int top_group(int g) const
        {
            sSubcContactInst *ci = sl_inst->contacts();
            for ( ; ci; ci = ci->next()) {
                if (ci->subc_group() == g) {
                    if (sl_next)
                        return (sl_next->top_group(ci->parent_group()));
                    return (ci->parent_group());
                }
            }
            return (-1);
        }

    int parent_group(int g) const
        {
            sSubcContactInst *ci = sl_inst->contacts();
            for ( ; ci; ci = ci->next()) {
                if (ci->subc_group() == g)
                    return (ci->parent_group());
            }
            return (-1);
        }

    const sSubcInstList *next()     const { return (sl_next); }
    void set_next(const sSubcInstList *n) { sl_next = n; }
    sSubcInst *inst()               const { return (sl_inst); }

private:
    const sSubcInstList *sl_next;
    sSubcInst           *sl_inst;
};


struct sSubcList
{
    sSubcList(sSubcInst *s, sEinstList *e, sSubcList *n)
        {
            sl_next = n;
            sl_desc = 0;
            sl_subs = s;
            sl_esubs = e;
        }

    ~sSubcList()
        {
            delete sl_desc;
            sEinstList::destroy(sl_esubs);
            sSubcInst::destroy(sl_subs);
        }

    static void destroy(sSubcList *s)
        {
            while (s) {
                sSubcList *x = s;
                s = s->sl_next;
                delete x;
            }
        }

    sSubcList *next()           const { return (sl_next); }
    sSubcInst *subs()           const { return (sl_subs); }
    sEinstList *esubs()         const { return (sl_esubs); }

    void set_next(sSubcList *n)       { sl_next = n; }
    void set_subs(sSubcInst *s)       { sl_subs = s; }
    void set_esubs(sEinstList *e)     { sl_esubs = e; }

    void setup_template()
        {
            if (!sl_desc && sl_subs)
                sl_desc = new sSubcDesc(sl_subs);
        }

private:
    sSubcList   *sl_next;
    sSubcDesc   *sl_desc;
    sSubcInst   *sl_subs;
    sEinstList  *sl_esubs;
};


// Structure to hold objects in a group.
//
struct sGroupObjs
{
    sGroupObjs(CDol *ol, const BBox *bb)
        {
            go_list = ol;
            go_vias = 0;
            if (bb)
                go_BB = *bb;
            else
                computeBB();
            go_length = 0;
            go_sorted = false;
        }

    ~sGroupObjs()
        {
            CDol::destroy(go_list);
            CDol::destroy(go_vias);
        }

    CDol *objlist()             const { return (go_list); }
    void set_objlist(CDol *ol)
        {
            go_list = ol;
            go_length = 0;
            go_sorted = false;
        }

    CDol *vialist()             const { return (go_vias); }
    void set_vialist(CDol *vl)          { go_vias = vl; }

    BBox &BB()                        { return (go_BB); }
    const BBox &cBB()           const { return (go_BB); }

    void computeBB()                  { CDol::computeBB(go_list, &go_BB); }

    bool is_sorted()            const { return (go_sorted); }
    void sort()
        {
            if (!go_sorted) {
                sort_list(go_list);
                go_sorted = true;
            }
        }

    unsigned int length()       const { return (go_length); }
    void set_length()
        {
            unsigned int len = 0;
            for (CDol *o = go_list; o; o = o->next, len++) ;
            go_length = len;
        }

    // ext_connect.cc
    double area() const;
    CDol *find_object(const CDl*, const BBox*);
    void accumulate(CDol*, SymTab*);
    CDol *accumulate(const CDl*, const BBox*);
    bool intersect(const CDo*, bool) const;
    bool intersect(const BBox*, bool) const;
    bool intersect(const sGroupObjs&);
    CDs *mk_cell(const CDtf*, const CDl*) const;

    static void sort_list(CDol*);

private:
    CDol            *go_list;
    CDol            *go_vias;
    BBox            go_BB;
    unsigned int    go_length;
    bool            go_sorted;
};


// This wraps either a sGroupObjs list, or the same objects in a
// dummy cell.  The cell is more efficient for long lists, but most
// lists are short, and we keep these as simple linked lists.  The
// stack is assumed loaded with the transform from objects to the
// top-level cell.  This struct contains functions that look for
// connections between nets.
//
struct sGroupXf : public cTfmStack
{
    // ext_connect.cc
    sGroupXf(CDs*, sGroupObjs*, const CDl* = 0);

    ~sGroupXf()
        {
            delete sdesc();
        }

    const BBox *BB()
        {
            if (sdesc())
                return (sdesc()->BB());
            if (objs())
                return (&objs()->cBB());
            return (0);
        }

    // Only one of these will return non-null.
    CDs *sdesc()        const { return (gx_sdesc); }
    sGroupObjs *objs()  const { return (gx_objs); }

#define EXT_GRP_DEF_THRESH  6
    static void set_threshold(int i)    { gx_threshold = i; }

    // ext_connect.cc
    XIrt connected(CDol*, const cTfmStack*, bool*, int) const;
    XIrt connected_direct(CDol*, const cTfmStack*, bool*) const;
    XIrt connected_by_contact(CDol*, const cTfmStack*, bool*) const;
    XIrt connected_by_via(CDol*, const cTfmStack*, bool*) const;

    // ext_device.cc
    CDol *find_objects(const sDevContactInst*, const CDl*);

private:
    // ext_connect.cc
    XIrt connected_by_contact(const CDl*, CDol*, const CDl*,
        const cTfmStack*, const sVia*, bool*) const;
    XIrt connected_by_via(const CDl*, CDol*, const CDl*, const cTfmStack*,
        const sVia*, const CDl*, bool*) const;

    // ext_device.cc
    CDol *find_objects_rc(const sDevContactInst*, const CDl*, SymTab*);

    CDs             *gx_topcell;    // top cell
    CDs             *gx_sdesc;      // temporary cell for objects
    sGroupObjs      *gx_objs;       // group object list

    static int      gx_threshold;   // use cell for more objs than this
};


// Structure to keep a list of objects and terminals in a group.
//
struct sGroup
{
    sGroup()
        {
            clear();
        }

    ~sGroup()
        {
            delete g_net;
            CDpin::destroy(g_termlist);
            sDevContactList::destroy(g_device_contacts);
            sSubcContactList::destroy(g_subc_contacts);
        }

    // Used to clear unused array components.
    void clear()
        {
            g_net = 0;
            g_termlist = 0;
            g_device_contacts = 0;
            g_subc_contacts = 0;
            g_netname = 0;
            g_capac = 0.0;
            g_node = -1;
            g_split_group = -1;
            g_hint_node = -1;
            g_flags = 0;
            g_nn_origin = 0;
        }

    bool has_net_or_terms()
        {
            return (g_net || g_termlist || g_device_contacts ||
                g_subc_contacts);
        }

    bool has_terms()
        {
            return (g_termlist || g_device_contacts || g_subc_contacts);
        }

    // ext_group.cc
    void add_object(CDo*);
    bool is_wire_only();
    void newnum(int);
    int numentries();
    void purge_e_phonies();
    void show(WindowDesc*, BBox* = 0);
    void dump(FILE*);

    sGroupObjs *net()           const { return (g_net); }
    void set_net(sGroupObjs *o)       { g_net = o; }
    CDpin *termlist()           const { return (g_termlist); }
    void set_termlist(CDpin *p)       { g_termlist = p; }
    sDevContactList *device_contacts()
                                const { return (g_device_contacts); }
    void set_device_contacts(sDevContactList *c)
                                      { g_device_contacts = c; }
    sSubcContactList *subc_contacts()
                                const { return (g_subc_contacts); }
    void set_subc_contacts(sSubcContactList *c)
                                      { g_subc_contacts = c; }
    CDnetName netname()         const { return (g_netname); }
    float capac()               const { return (g_capac); }
    void set_capac(float c)           { g_capac = c; }
    int node()                  const { return (g_node); }
    void set_node(int n)              { g_node = n; }
    int split_group()           const { return (g_split_group); }
    void set_split_group(int g)       { g_split_group = g; }

#define EXT_GRP_DISPLAYED               0x1
    // Indicates that the group is highlighted in display.
#define EXT_GRP_GLOBAL                  0x2
    // The group corresponds to a global node (power/gnd, etc.).
#define EXT_GRP_GLOBAL_TESTED           0x4
    // Test for global-ness was performed.
#define EXT_GRP_UNAS_WIRE_ONLY          0x8
    // Group contains unassociated metal only.
#define EXT_GRP_UNAS_WIRE_ONLY_TESTED   0x10
    // Test for metal-only was performed.
#define EXT_GRP_CONNECT                 0x20
    // This group was connected to in an instantiation.

    void clear_flags()                { g_flags = 0; }

    bool displayed() const
        {
            return (g_flags & EXT_GRP_DISPLAYED);
        }
    void set_displayed(bool b)
        {
            if (b)
                g_flags |= EXT_GRP_DISPLAYED;
            else
                g_flags &= ~EXT_GRP_DISPLAYED;
        }

    bool global() const
        {
            return (g_flags & EXT_GRP_GLOBAL);
        }
    void set_global(bool b)
        {
            if (b)
                g_flags |= EXT_GRP_GLOBAL;
            else
                g_flags &= ~EXT_GRP_GLOBAL;
        }

    bool global_tested() const
        {
            return (g_flags & EXT_GRP_GLOBAL_TESTED);
        }
    void set_global_tested(bool b)
        {
            if (b)
                g_flags |= EXT_GRP_GLOBAL_TESTED;
            else
                g_flags &= ~EXT_GRP_GLOBAL_TESTED;
        }

    bool unas_wire_only() const
        {
            return (g_flags & EXT_GRP_UNAS_WIRE_ONLY);
        }
    void set_unas_wire_only(bool b)
        {
            if (b)
                g_flags |= EXT_GRP_UNAS_WIRE_ONLY;
            else
                g_flags &= ~EXT_GRP_UNAS_WIRE_ONLY;
        }

    bool unas_wire_only_tested() const
        {
            return (g_flags & EXT_GRP_UNAS_WIRE_ONLY_TESTED);
        }
    void set_unas_wire_only_tested(bool b)
        {
            if (b)
                g_flags |= EXT_GRP_UNAS_WIRE_ONLY_TESTED;
            else
                g_flags &= ~EXT_GRP_UNAS_WIRE_ONLY_TESTED;
        }

    bool cell_connection() const
        {
            return (g_flags & EXT_GRP_CONNECT);
        }
    void set_cell_connection(bool b)
        {
            if (b)
                g_flags |= EXT_GRP_CONNECT;
            else
                g_flags &= ~EXT_GRP_CONNECT;
        }

    // Values for g_nn_origin.  Note that these are in priority
    // order.
    enum NameFromType { NameFromNode, NameFromTerm, NameFromLabel };

    void set_netname(CDnetName n, NameFromType t)
        {
            if (!g_netname || t >= g_nn_origin) {
                g_netname = n;
                g_nn_origin = n ? t : NameFromNode;
            }
        }
    NameFromType netname_origin()   const
        { return ((NameFromType)g_nn_origin); }

    int hint_node()             const { return (g_hint_node); }
    void set_hint_node(int n)         { g_hint_node = n; }

private:
    sGroupObjs      *g_net;                 // object list
    CDpin           *g_termlist;            // connection points
    sDevContactList *g_device_contacts;     // device contacts
    sSubcContactList *g_subc_contacts;      // subcircuit contacts
    CDnetName       g_netname;              // name of net
    float           g_capac;                // capacitance of net
    int             g_node;                 // corresponding electrical node
    int             g_split_group;          // should connect to this
    short int       g_hint_node;            // preferred node, break ties
    unsigned char   g_flags;                // misc. flags
    unsigned char   g_nn_origin;            // netname origin code
};


// List element for a virtual contact due to subcells that overlap or
// abut.
//
struct sVContact
{
    sVContact(CDc *c1, int x1, int y1, int g1,
            CDc *c2, int x2, int y2, int g2, int vg, sVContact *n)
        {
            next = n;
            cdesc1 = c1;
            ix1 = x1;
            iy1 = y1;
            subg1 = g1;
            cdesc2 = c2;
            ix2 = x2;
            iy2 = y2;
            subg2 = g2;
            vgroup = vg;
        }

    static void destroy(sVContact *v)
        {
            while (v) {
                sVContact *x = v;
                v = v->next;
                delete x;
            }
        }

    sVContact *next;
    CDc *cdesc1;
    int ix1, iy1;
    int subg1;
    CDc *cdesc2;
    int ix2, iy2;
    int subg2;
    int vgroup;
};


// Linked list element returned by cGroupDesc::check_stamping, which
// indicates an unresolved substrate connection for a BC_defer
// contact.
//
struct sBcErr
{
    sBcErr(const CDs *sd, const sDevDesc *dd, const BBox *bb, sBcErr *n) :
            sbc_locBB(*bb)
        {
            sbc_ddesc = dd;
            sbc_owner = sd;
            sbc_next = n;
        }

    static void destroy(sBcErr *s)
        {
            while (s) {
                sBcErr *x = s;
                s = s->sbc_next;
                delete x;
            }
        }

    const BBox *locBB()         const { return (&sbc_locBB); }
    const sDevDesc *devDesc()   const { return (sbc_ddesc); }
    const CDs *owner()          const { return (sbc_owner); }
    sBcErr *next()                    { return (sbc_next); }
    void set_next(sBcErr *n)          { sbc_next = n; }

private:
    BBox            sbc_locBB;      // Device BB at top level.
    const sDevDesc  *sbc_ddesc;     // Desc. for device containing term.
    const CDs       *sbc_owner;     // Cell containing device.
    sBcErr          *sbc_next;
};


// These limits apply while associating.
#define EXT_DEF_LVS_LOOP_MAX    10000
#define EXT_DEF_LVS_ITER_MAX    200

// Maintain entire group structure.  The CDs sGroups field of physical
// structures points to one of these.
//
class cGroupDesc
{
public:
    cGroupDesc(CDs *s)
        {
            gd_groups = 0;
            gd_celldesc = s;
            gd_g_phonycell = 0;
            gd_e_phonycell = 0;
            gd_etlist = 0;
            gd_devices = 0;
            gd_subckts = 0;
            gd_vcontacts = 0;
            gd_extra_devs = 0;
            gd_extra_subs = 0;
            gd_master_list = 0;
            gd_global_nametab = 0;
            gd_flatten_tab = 0;
            gd_ignore_tab = 0;
            gd_sym_list = 0;
            gd_lvs_msgs = 0;
            gd_asize = 0;
            gd_discreps = 0;
            gd_flags = 0;
        }

    ~cGroupDesc()
        {
            clear_groups();
        }

    int nextindex() const
        {
            int i = gd_asize - 1;
            if (i < 0)
                return (1);
            while (i >= 1 && !gd_groups[i].has_net_or_terms()) {
                gd_groups[i].clear();
                i--;
            }
            return (i+1);
        }

    int num_groups()        const { return (gd_asize); }

    int vnextnum() const
        {
            return (gd_vcontacts ? gd_vcontacts->vgroup + 1 : nextindex());
        }
    bool nets_only()        const { return (!gd_devices && !gd_subckts); }
    bool isempty()          const { return (!gd_groups); }

    bool has_net_or_terms(int g) const
        {
            if (gd_groups && g >= 0 && g < gd_asize)
                return (gd_groups[g].has_net_or_terms());
            return (false);
        }

    bool has_terms(int g) const
        {
            if (gd_groups && g >= 0 && g < gd_asize)
                return (gd_groups[g].has_terms());
            return (false);
        }

    int node_of_group(int g) const
        {
            if (gd_groups && g >= 0 && g < gd_asize)
                return (gd_groups[g].node());
            return (-1);
        }

    sGroupObjs *net_of_group(int g) const
        {
            if (gd_groups && g >= 0 && g < gd_asize)
                return (gd_groups[g].net());
            return (0);
        }

    sGroup *group_for(int g) const
        {
            if (gd_groups && g >= 0 && g < gd_asize)
                return (&gd_groups[g]);
            return (0);
        }

    sSubcList *subckts()        const { return (gd_subckts); }
    sDevList *devices()         const { return (gd_devices); }
    bool is_device()            const { return (gd_devices != 0); }
    bool is_subckt()            const { return (gd_subckts != 0); }
    sElecNetList *elec_nets()   const { return (gd_etlist); }

    CDs *cell_desc()            const { return (gd_celldesc); }
    const CDs *g_phony()        const { return (gd_g_phonycell); }
    const CDs *e_phony()        const { return (gd_e_phonycell); }

    // Bits for gd_flags.
// Set in top-level cell when grouping.
#define EXT_GD_TOP_LEVEL        0x1

// Non-associable device or subcircuit present, don't bother with
// symmetry trials.
#define EXT_GD_SKIP_PERMUTES    0x2

// Set when all devices and subckts are associated, allows group
// symmetry breaking.
#define EXT_GD_GRP_BREAK        0x4

// Allow connection errors in comparisons, used in final pass to
// associate as much as possible, when overall association fails.
#define EXT_GD_ALLOW_ERRS       0x8

// Waive the requirement that device or subcircuit must have one
// associated contact, if no similar device or subcircuit has an
// associated connection.  Allows symmetry breaking to choose an
// association in this case, which would otherwise fail.
#define EXT_GD_NO_CONT_BRKSYM   0x10

// When using two-pass association, this flag is set during the first
// pass, where association is computed as much as possible without
// symmetry trials and without using hierarchy.
#define EXT_GD_FIRST_PASS       0x20

    bool top_level()        const { return (gd_flags & EXT_GD_TOP_LEVEL); }
    void set_top_level(bool b)
        {
            if (b)
                gd_flags |= EXT_GD_TOP_LEVEL;
            else
                gd_flags &= ~EXT_GD_TOP_LEVEL;
        }


    bool in_flatten_list(const CDc *cdesc)
        {
            if (gd_flatten_tab)
                return (gd_flatten_tab->get((unsigned long)cdesc) != ST_NIL);
            return (false);
        }


    bool in_ignore_list(const CDc *cdesc)
        {
            if (gd_ignore_tab)
                return (gd_ignore_tab->get((unsigned long)cdesc) != ST_NIL);
            return (false);
        }

    bool skip_permutes()  const { return (gd_flags & EXT_GD_SKIP_PERMUTES); }
    bool grp_break()      const { return (gd_flags & EXT_GD_GRP_BREAK); }
    bool allow_errs()     const { return (gd_flags & EXT_GD_ALLOW_ERRS); }
    bool no_cont_brksym() const { return (gd_flags & EXT_GD_NO_CONT_BRKSYM); }
    bool first_pass()     const { return (gd_flags & EXT_GD_FIRST_PASS); }

    static int assoc_loop_max()             { return (gd_loop_max); }
    static void set_assoc_loop_max(int i)   { gd_loop_max = i; }
    static int assoc_iter_max()             { return (gd_iter_max); }
    static void set_assoc_iter_max(int i)   { gd_iter_max = i; }

    // ext_nets.h
    inline bool node_active(int);
    inline int group_of_node(int);
    inline CDpin *pins_of_node(int);
    inline CDcont *conts_of_node(int);

    // ext_device.cc
    sDevInstList *find_dev(const char*, const char*, const char*, const BBox*);
    int find_dev_set(const char*, const char*, const char*, const BBox*, bool);
    void parse_find_dev(const char*, bool);
    stringlist *list_devs();
    void set_devs_display(bool);
    int show_devs(WindowDesc*, bool);
    bool link_contact(sDevContactInst*);
    int find_contact_group(sDevContactInst*, bool);
    bool has_bulk_contact(const sDevDesc*, const BBox*, sDevContactDesc*,
        XIrt*);
    bool check_bulk_contact_group(const BBox*, const sDevContactDesc*);

    void split_devices();
    void combine_devices();
    bool setup_dev_layer();
    bool update_measure_prpty();

    // ext_duality.cc
    XIrt setup_duality_first_pass(SymTab*, int = 0);
    XIrt setup_duality(int = 0);
    void clear_duality();
    void fixup_duality(const sSubcList*);
    sPermGrpList *check_permutes()const;
    void subcircuit_permutation_fix(const sSubcList*);
    sDevInst *find_dual_dev(const CDc*, int);
    sSubcInst *find_dual_subc(const CDc*, int);
    bool bind_term_to_group(CDsterm*, int);
    void clear_formal_terms();
    void set_association(int, int);
    void select_unassoc_groups();
    void select_unassoc_nodes();
    void select_unassoc_pdevs();
    void select_unassoc_edevs();
    void select_unassoc_psubs();
    void select_unassoc_esubs();

    // ext_ep_comp.cc
    int ep_grp_comp(int, int, const sSubcInst* = 0);
    void check_bulk_contact(const sDevInst*, sDevContactInst*);

    // ext_extract.cc
    XIrt setup_extract(int = 0);
    void clear_extract();
    sSubcInst *find_subc(CDc*, int, int);
    void compute_cap();
    bool test_nets_only();
    void list_callers(SymTab*, SymTab*);
    bool bind_term_group_at_location(CDsterm*);
    void fix_connections();

    // ext_group.cc
    XIrt setup_groups();
    void clear_groups(bool = false);
    CDo *intersect_phony(BBox*);
    void dump(FILE*);

    // ext_mosgate.cc
    bool is_mos_tpeq(const sDevInst*, const sDevInst*) const;

    // ext_netname.cc
    const char *group_name(int, bool*, bool*) const;
    const char *group_name(int, sLstr* = 0) const;
    void init_net_names();
    bool update_net_labels();
    bool update_net_label(int);
    bool create_net_label(int, const char*, bool, CDla**);
    CDol *find_net_labels(int, const char*);
    CDol *find_net_labels(CDol*, const CDo*, const CDl*, const char*);
    int find_set_net_names();
    int find_group_at_location(const CDl*, int, int, bool);

    // ext_out_lvs.cc
    LVSresult print_lvs(FILE*);
    void add_lvs_message(const char*, ...);

    // ext_out_phys.cc
    bool print_phys(FILE*, CDs*, int, sDumpOpts*);

    // ext_view.cc
    int show_objects(WindowDesc*, const CDl*);
    void show_groups(WindowDesc*, bool);
    void set_group_display(bool);

private:
    // ext_connect.cc
    XIrt connect_to_subs(const ext_group::sSubcGen*);
    XIrt connect_between_subs(const ext_group::sSubcGen*);
    XIrt connect_subs(const ext_group::sSubcGen*, const ext_group::sSubcGen*,
        const BBox&, ext_group::Ufb &ufb, bool);

    // ext_device.cc
    XIrt add_devs();
    bool combine_parallel(sDevPrefixList*);
    void unlink_contacts(sDevInst*);
    bool combine_series(sDevPrefixList*);

    // ext_duality.cc
    XIrt set_duality_first_pass();
    XIrt set_duality();
    bool check_series_merge();
    void link_elec_subs(sEinstList*);
    void link_elec_devs(sEinstList*);
    void combine_elec_parallel();
    void reposition_terminals(bool = false);
    bool reposition_vterm(CDterm*);
    bool bind_term_to_vgroup(CDsterm*, int);
    void position_labels(cTfmStack*);
    void subcircuit_permutation_fix_rc(int);
    bool ident_node(int);
    bool ident_dev(sDevList*, int, bool = false) throw (XIrt);
    bool find_match(sDevList*, sDevComp&, bool) throw (XIrt);
    bool ident_subckt(sSubcList*, int, bool = false, bool = false);
    bool find_match(sSubcList*, sSubcInst*, bool, bool);
    void ident_term_nodes(sDevInst*);
    int solve_duals() throw (XIrt);
    void check_split();
    bool check_global(int);
    bool check_unas_wire_only(int);
    bool check_associations(int);
    bool break_symmetry() throw (XIrt);

    // ext_ep_comp.cc
    int ep_hier_comp(int, int);
    int ep_hier_comp_rc(int, int);
    int ep_param_comp(sDevInst*, const sEinstList*) throw (XIrt);
    int ep_param_comp_core(sDevInst*, const double*, sParamTab*) throw (XIrt);
    int ep_subc_comp(sSubcInst*, const sEinstList*, bool);

    // ext_extract.cc
    CDpin *group_terms(CDpin*, bool);
    void set_subc_group(bool);
    XIrt add_subckts();
    int add(int, const ext_group::sSubcGen*, int);
    sSubcInst *add_sc(CDc*, int, int);
    int add_vcontact(CDc*, int, int, int, CDc*, int, int, int);
    int fixup_subc_contacts(sSubcInst*);
    void remap_subc_contacts(sSubcInst*, const int*);
    bool bind_term_group_at_location_rc(const sSubcInstList*, CDsterm*,
        cTfmStack&);
    void bind_term(CDsterm*, int);
    void unbind_term(CDsterm*);
    CDpin *list_cell_terms(bool = false) const;
    void warn_conductor(int, const char*);
    void fix_connections_rc(SymTab*);
    bool flatten(bool* = 0, bool = false);
    bool flatten_core(sSubcList*, bool, int*, int*, int*);
    void sort_subs(bool);
    void sort_and_renumber_subs();
    void sort_devs(bool);
    void sort_and_renumber_devs();
    bool check_merge();
    void remove_contact(sSubcContactInst*);
    sDevInst *add_dev_copy(cTfmStack*, sDevInst*, int*);
    void add_subc_copy(cTfmStack*, sSubcInst*, int*);
    CDc *copy_cdesc(cTfmStack*, CDc*);

    // ext_group.cc
    void find_ignored();
    bool process_exclude();
    void alloc_groups(int);
    XIrt group_objects();
    XIrt combine();
    void reduce(int, int);
    void renumber_groups(int** = 0);

    // ext_grpgen.cc
    ext_group::sSubcLink *build_links(cTfmStack*, const BBox*);

    // ext_mosgate.cc
    bool mos_np_input(const CDsterm*, sDevInst**, sDevInst**, int*) const;
    bool nand_match(int, int, const sDevInst*, const sDevInst*, int) const;
    bool nor_match(int, int, const sDevInst*, const sDevInst*, int) const;
    int find_nmos_totem_top(int, int, int*) const;
    int find_pmos_totem_bot(int, int, int*) const;
    int find_nmos_totem_next(int, const sDevInst*, sDevInst**) const;
    int find_pmos_totem_next(int, const sDevInst*, sDevInst**) const;
    bool find_nmos_sd(int, int*) const;
    bool find_pmos_sd(int, int*) const;

    // ext_netname.cc
    CDol *find_net_labels_rc(const CDs*, cTfmStack&, int, const char*, CDol*);
    int find_group_at_location_rc(const sSubcInstList*, const CDl*,
        int, int, cTfmStack&, bool);

    // ext_out_lvs.cc
    void print_subc_contact_lvs(FILE*, const CDcbin&, const sSubcContactInst*);
    sBcErr *check_stamping();
    sBcErr *check_stamping_rc(cGroupDesc*, SymTab*, cTfmStack&);
    void check_grp_node(int, struct sLVSstat&, FILE*);
    LVSresult print_summary_lvs(FILE*, sLVSstat&);
    bool check_wire_cap(CDc*, const char*, FILE*);

    // ext_out_phys.cc
    void print_groups(FILE*, sDumpOpts*);
    int print_devs_subs(FILE*, sDumpOpts*);
    bool print_spice(FILE*, CDs*, sDumpOpts*);
    void print_formatted(FILE*, const char*, sDumpOpts*);

    void set_skip_permutes(bool b)
        {
            if (b)
                gd_flags |= EXT_GD_SKIP_PERMUTES;
            else
                gd_flags &= ~EXT_GD_SKIP_PERMUTES;
        }

    void set_grp_break(bool b)
        {
            if (b)
                gd_flags |= EXT_GD_GRP_BREAK;
            else
                gd_flags &= ~EXT_GD_GRP_BREAK;
        }

    void set_allow_errs(bool b)
        {
            if (b)
                gd_flags |= EXT_GD_ALLOW_ERRS;
            else
                gd_flags &= ~EXT_GD_ALLOW_ERRS;
        }

    void set_no_cont_brksym(bool b)
        {
            if (b)
                gd_flags |= EXT_GD_NO_CONT_BRKSYM;
            else
                gd_flags &= ~EXT_GD_NO_CONT_BRKSYM;
        }

    void set_first_pass(bool b)
        {
            if (b)
                gd_flags |= EXT_GD_FIRST_PASS;
            else
                gd_flags &= ~EXT_GD_FIRST_PASS;
        }

    sGroup      *gd_groups;         // array of groups
    CDs         *gd_celldesc;       // back pointer to cell
    CDs         *gd_g_phonycell;    // cell for odescs created while grouping
    CDs         *gd_e_phonycell;    // cell for odescs created while extracting
    sElecNetList *gd_etlist;        // electrical terminal list
    sDevList    *gd_devices;        // list of devices for extraction
    sSubcList   *gd_subckts;        // list of subcircuits for extraction
    sVContact   *gd_vcontacts;      // virtual contact, when subcells abut
    sEinstList  *gd_extra_devs;     // non-associable electrical devices
    sEinstList  *gd_extra_subs;     // non-associable electrical subckts
    CDm         *gd_master_list;    // support for temporary cell descs, for
                                    //  flatten
    SymTab      *gd_global_nametab; // table of global net names in hierarchy
    SymTab      *gd_flatten_tab;    // table of flattened insts
    SymTab      *gd_ignore_tab;     // table of ignored insts
    ext_duality::sSymBrk *gd_sym_list; // context history for symmetry breaking
    stringlist  *gd_lvs_msgs;       // strings for LVS output
    int         gd_asize;           // size of array
    unsigned short gd_discreps;     // residual associaton discrepancy count
    unsigned short gd_flags;

    static int  gd_loop_max;        // limits in association algorithm
    static int  gd_iter_max;
};


// Linked-list element for cGroupDesc.
//
struct sGdList
{
    sGdList(cGroupDesc *g, sGdList *n)
        {
            next = n;
            gd = g;
        }

    static void destroy(sGdList *l)
        {
            while (l) {
                const sGdList *x = l;
                l = l->next;
                delete x;
            }
        }

    sGdList *next;
    cGroupDesc *gd;
};

#endif


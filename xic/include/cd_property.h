
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
 $Id: cd_property.h,v 5.95 2017/04/16 20:28:09 stevew Exp $
 *========================================================================*/

#ifndef CD_PRPTYPE_H
#define CD_PRPTYPE_H


//-----------------------------------------------------------------------------
// Derived properties for physical mode.
//-----------------------------------------------------------------------------

struct CDcterm;
struct CDsterm;

// This is added to physical objects that are the target of a terminal
// struct's oset field.  This property is used internally only, i.e.,
// never exported or imported.
//
struct CDp_oset : public CDp
{
    CDp_oset(CDterm*);
    ~CDp_oset();

    // virtual overrides
    CDp *dup()                const { return (0); }  // never duplicated!

    CDterm *term()                  { return (po_term); }
    void set_term(CDterm *t)        { po_term = t; }

private:
    CDterm *po_term;
};


//-----------------------------------------------------------------------------
// Derived global properties.
//-----------------------------------------------------------------------------

// Global properties should only be assigned to top-level cells.

// This is for the plot properties, so that the actual plot hypertext
// can be stored while the cell is in memory.  That allows the plot
// string to reflect updates if it points at a subcell that is edited.
// Applied to electrical top-level cells.
//
struct CDp_glob : public CDp
{
    CDp_glob(int pnum) : CDp(pnum) { pg_data = 0; }
    CDp_glob(const CDp_glob&);
    ~CDp_glob();

    // virtual overrides
    CDp *dup() const                { return (new CDp_glob(*this)); }

    bool print(sLstr*, int, int) const;
    hyList *hpstring(CDs*) const;

    CDp_glob *next()                { return ((CDp_glob*)next_n()); }

    hyList *data()                  { return (pg_data); }
    void set_data(hyList *d)        { pg_data = d; }

private:
    struct hyList *pg_data;
};


//------------------------------------------------------------------------------
// Electrical schematic properties.
//------------------------------------------------------------------------------

// These properties are used in electrical mode (schematics) to
// maintain state.

// Num name user-set 1max ckt-cell dev-cell ckt-inst dev-inst object
// 1   model   x      x              x                 x
// 2   value   x      x              x                 x
// 3   param   x      x     x        x        x        x
// 4   other   x            x        x        x        x
// 5   nophys  x(1)   x     x        x        x        x
// 6   virtual x      x     x
// 7   flatten x      x     x                 x
// 8   range   x      x                       x        x
// 9   bnode                x        x        x        x       wire
// 10  node                 x        x        x        x       wire
// 11  name    x(2)   x     x        x        x        x
// 12  labloc         x     x        x
// 13  mut
// 14  newmut               x
// 15  branch         x              x                 x
// 16  labrf                                                   label
// 17  mutlrf                                          x(3)
// 18  symblc  x(4)   x     x                 x
// 19  nodmap         x     x
// 20  macro   x      x              x
// 21  devref  x      x                                x
//
// 1. nophys property can be changed by the user in instances only.
// 2. name property can be changed by the user in instances only.
// 3. mutlrf property found on inductor devices only.
// 4. symblc property applied by user to instances only as "nosymb".


// Property of devices indicating SPICE model name.
//
// Property type: CDp_user
//
#define P_MODEL     1
#define KW_MODEL    CDp::elec_prp_name(P_MODEL)

// Property of devices indicating electrical value.
//
// Property type: CDp_user
//
#define P_VALUE     2
#define KW_VALUE    CDp::elec_prp_name(P_VALUE)

// Property of devices and circuits, indicates parameter
// values and initial conditions.
//
// Property type: CDp_user
//
#define P_PARAM     3
#define KW_PARAM    CDp::elec_prp_name(P_PARAM)
// Obsolete alias for KW_PARAM.
#define KW_INITC    "initc"

// Property of devices, and circuits, user-defined purpose.
//
// Property type: CDp_user
//
#define P_OTHER     4
#define KW_OTHER    CDp::elec_prp_name(P_OTHER)

// Property applied to devices and circuits that have no physical
// implementation.
//
// Property type: CDp
//
#define P_NOPHYS    5
#define KW_NOPHYS   CDp::elec_prp_name(P_NOPHYS)

// Property applied to circuit cells that will never be included in
// SPICE or other netlists.  Calls will be output, so these must be
// resolved through an include or by other means.
//
// Property type: CDp
//
#define P_VIRTUAL   6
#define KW_VIRTUAL  CDp::elec_prp_name(P_VIRTUAL)

// This can be applied to circuits.  The state is active if the
// instance has the property and the master does not, or the instance
// does not have the property and the master does.  If active, the
// schematic will be logically flattened into its parent before
// association in LVS.
//
// Property type: CDp
//
#define P_FLATTEN   7
#define KW_FLATTEN  CDp::elec_prp_name(P_FLATTEN)

// Range property, applies to instances only.  This vectorizes the
// instance.
//
// Property type: CDp_range
//
#define P_RANGE     8
#define KW_RANGE    CDp::elec_prp_name(P_RANGE)

// Bus node, a multi-connection contact location.  Applied to circuit
// cells/instances, bus terminal device, and wires.
//
// Property types: CDp_bnode (wire), CDp_bcnode (instance), CDp_bsnode (cell)
//               
#define P_BNODE     9
#define KW_BNODE    CDp::elec_prp_name(P_BNODE)

// Node, a scalar contact location.  Applied to circuit
// cells/instances, terminal devices, and wires.
//
// Property types: CDp_node (wire), CDp_cnode (instance), CDp_snode (cell)
//
#define P_NODE      10
#define KW_NODE     CDp::elec_prp_name(P_NODE)

// Name property of devices and circuits, stores name prefix in cells
// and placement name in instances.
//
// Property type: CDp_name
//
#define P_NAME      11
#define KW_NAME     CDp::elec_prp_name(P_NAME)

// Cell property to set locations of bound property labels.  Location
// codes:
//     The '.' position implies the horizontal justification. 
//     All are default vertical justification except 17,18
//     which are VJC.
// 
//           .0     1.
//           .2 1.6 3.
//        4. --------- .8
//        5. |.20 21.| .9
//       17. |       | .18
//        6. |.22 23.| .10
//        7. --------- .11
//          .12 1.9 13.
//          .14     15.
//
// Property type: CDp_labloc
//
#define P_LABLOC    12
#define KW_LABLOC   CDp::elec_prp_name(P_LABLOC)

// Old style mutual inductor property of circuit cells.  Keeps
// track of location of mutual inductor pair (not used).
//
// Property type: CDp_mut
//
#define P_MUT       13
#define KW_MUT      CDp::elec_prp_name(P_MUT)

// New style mutual inductor property of circuit cells.  Keeps
// track of pointers to mutual inductor pair.
//
// Property type: CDp_nmut
//
#define P_NEWMUT    14
#define KW_NEWMUT   CDp::elec_prp_name(P_NEWMUT)

// Branch property of devices.  Keeps track of a location to access
// the branch current in the matrix (applies to inductors and voltage
// sources only), through a string which specifies how to determine
// current.  The branch string may contain tokens
//       <v>               device voltage
//       <name>            device name
//       <value>           device value
// everything else is literal.  This is expanded and passed to SPICE
// as a vector expression.
//
// Property type: CDp_branch
//
#define P_BRANCH    15
#define KW_BRANCH   CDp::elec_prp_name(P_BRANCH)

// Property of labels, indicates that the label text is bound to a
// device, wire, or cell property.
//
// Property type: CDp_lref
//
#define P_LABRF     16
#define KW_LABRF    CDp::elec_prp_name(P_LABRF)

// Property of inductor device instances indicating use in a mutual
// inductor pair (string property).
//
// Property type: CDp_mutlrf
//
#define P_MUTLRF    17
#define KW_MUTLRF   CDp::elec_prp_name(P_MUTLRF)

// Property of circuit cell to provide symbolic representation.  Also
// found in instances to switch symbolic display per instance.
//
// Property type: CDp_sym
//
#define P_SYMBLC    18
#define KW_SYMBLC   CDp::elec_prp_name(P_SYMBLC)
#define KW_NOSYMB   "nosymb"

// Cell property of circuits to indicate use of node name mapping.
//
// Property type: CDp_nodmp
//
#define P_NODMAP    19
#define KW_NODMAP   CDp::elec_prp_name(P_NODMAP)

// Cell property to force a macro call in SPICE output.  Given to
// devices that are actually treated as a subcircuit in the model
// library (i.e., the "model" name matches a .subckt token in the
// model file.  This property will force an 'X' ahead of the device
// name in SPICE output.
//
#define P_MACRO     20
#define KW_MACRO    CDp::elec_prp_name(P_MACRO)

// Instance property to specify device names referenced in ccvs, cccs,
// wswitch devices,
//
#define P_DEVREF     21
#define KW_DEVREF    CDp::elec_prp_name(P_DEVREF)

#define P_MAX_PRP_NUM 21

// Mutual inductor SPICE key, used in mutual inductor properties.
#define MUT_CODE    "K"

// This is the maximum allowed index value for P_NODE and P_BNODE
// properties, which limits the maximum number of connections to a
// subcircuit.  We reserve the right to save the index value as an
// unsigned short, otherwise there is no "real" limit implied here.
//
#define P_NODE_MAX_INDEX 1023

// Terminal flags, used in node/bus node properties.
//
#define TE_BYNAME       0x1
// Unused, but reserve the flag, which is ignored (for backward
// compatibility).
//#define TE_VIRTUAL      0x2
#define TE_FIXED        0x4
#define TE_SCINVIS      0x8
#define TE_SYINVIS      0x10
#define TE_UNINIT       0x100
#define TE_LOCSET       0x200
#define TE_PTS          0x400
#define TE_NOPHYS       0x800
#define TE_OWNTERM      0x1000
#define TE_IOFLAGS      0x1d

// Terminal flags:
//
// TE_BYNAME        (EXTERNAL) (CDp_cnode flag)
//   Electrical terminal node is resolved by name only, physical
//   placement not considered.  Overrides VIRTUAL.
// TE_VIRTUAL
//   This is no longer used.
// TE_FIXED         (EXTERNAL) (CDsterm boolean)
//   User has set the physical location, Xic will not move it (the
//   location had better be correct!).
// TE_SCINVIS       (EXTERNAL) (CDp_cnode flag)
//   Electrical terminal is never shown in the schematic, physical
//   terminal if any will be shown.  Terminal can't make connections
//   by position.
// TE_SYINVIS       (EXTERNAL) (CDp_cnode flag)
//   Electrical terminal is never shown in the symbol, physical
//   terminal if any will be shown.  Terminal can't make connections
//   by position.
// TE_UNINIT        (INTERNAL) (CDterm boolean)
//   The formal terminal has not been "implemented".  A formal terminal
//   is implemented when it has an associated physical object desc or
//   virtual group, i.e., the terminal location is over a suitable
//   object (t_oset) or subcell contact.  This flag is turned off in
//   instance terminals when the terminal is moved into place by Xic.
// TE_LOCSET        (INTERNAL) (CDterm boolean)
//   The terminal physical placement location has been set.
// TE_PTS           (INTERNAL) (CDp_cnode flag)
//   Electrical terminal contains a multi-cooidinate list.
// TE_NOPHYS        (INTERNAL) (CDp_snode flag)
//   Device terminal has no physical implementation, such as
//   a temperature node.
// TE_OWNTERM       (INTERNAL) (CDp_snode flag)
//   The property owns the physical terminal, and should delete it in
//   the property destructor.  This will be the case in devices that
//   have no physical part (phys terms are otherwise owned by the
//   physical cell).
//
// TE_IOFLAGS
//   These flags are saved/restored with cell data file.

// Terminal types, must match FlagDef TermTypes[].
enum CDtermType
{
    TE_tINPUT,
    TE_tOUTPUT,
    TE_tINOUT,
    TE_tTRISTATE,
    TE_tCLOCK,
    TE_tOUTCLOCK,
    TE_tSUPPLY,
    TE_tOUTSUPPLY,
    TE_tGROUND
};


//-----------------------------------------------------------------------------
// The User property, implements model, value, param, and other
// properties.

struct CDp_user : public CDp
{
    CDp_user(int pnum) : CDp(pnum)
        {
            pu_label = 0;
            pu_data = 0;
        }

    CDp_user(CDs*, const char*, int);

    CDp_user(const CDp_user&);
    CDp_user &operator=(const CDp_user&);

    ~CDp_user();

    // virtual overrides
    CDp *dup()                const { return (new CDp_user(*this)); }

    bool print(sLstr*, int, int) const;
    hyList *hpstring(CDs*) const;

    bool cond_bind(CDla* odesc)
        {
            if (!pu_label)
                pu_label = odesc;
            return (pu_label == odesc);
        }

    void bind(CDla* odesc)          { pu_label = odesc; }
    CDla *bound()             const { return (pu_label); }

    void purge(CDo *odesc)
        {
            if (odesc == (CDo*)pu_label)
                pu_label = 0;
        }

    hyList *label_text(bool*, CDc* = 0) const;

    bool is_elec()            const { return (true); }

    CDp_user *next()                { return ((CDp_user*)next_n()); }

    hyList *data()            const { return (pu_data); }
    void set_data(hyList *h)        { pu_data = h; }

private:
    CDla *pu_label;                 // associated label
    hyList *pu_data;
};


//----------------------------------------------------------------------------
// Range property.
//
// Applied to devices and subciruits.

struct CDp_cnode;

struct CDp_range : public CDp
{
    CDp_range() : CDp(0, P_RANGE)
        {
            pr_beg_range = 0;
            pr_end_range = 0;
            pr_nodes = 0;
            pr_names = 0;
            pr_numnodes = 0;
            pr_asize = 0;
        }

    CDp_range(const CDp_range&);
    CDp_range &operator=(const CDp_range&);

    ~CDp_range();

    // virtual overrides
    CDp *dup()                  const { return (new CDp_range(*this)); }

    bool print(sLstr*, int, int) const;

    bool is_elec()              const { return (true); }

    CDp_range *next()                 { return ((CDp_range*)next_n()); }

    unsigned int beg_range()    const { return (pr_beg_range); }
    unsigned int end_range()    const { return (pr_end_range); }
    void set_range(unsigned int beg, unsigned int end)
        {
            pr_beg_range = beg;
            pr_end_range = end;
        }

    unsigned int width() const
        {
            return ((pr_end_range >= pr_beg_range ?
                pr_end_range - pr_beg_range :
                pr_beg_range - pr_end_range) + 1);
        }

    // The node property array stores the node properties associated
    // with the bit instances.  The instance node properties
    // correspond to the 'start' range value.  The array contains
    // groups of nodes sorted by index, in order, through the 'end'
    // range value.

    unsigned int num_nodes()      const { return (pr_numnodes); }

    bool parse_range(const char*);
    void setup(const CDc*);
    CDp_cnode *node(const CDc*, unsigned int, unsigned int) const;
    void print_nodes(const CDc*, FILE*, sLstr*) const;
    CDp_name *name_prp(const CDc*, unsigned int) const;

protected:
    unsigned short pr_beg_range;    // range start
    unsigned short pr_end_range;    // range end
    CDp_cnode *pr_nodes;            // array of node properties for 'bits'
    CDp_name *pr_names;             // array of name properties for 'bits'
    unsigned int pr_numnodes;       // number of node properties
    unsigned int pr_asize;          // size of allocated nodes array
};


//----------------------------------------------------------------------------
// Bus node base property, for wires.
//
// This supports an associated label.  In a wire, the label
// (pbn_label) always exists and provides a "net expression" that
// defines the connections.
//
// The propeerty can be a "bus" terminal or a "bundle" terminal.  Bus
// terminals have a name in pbn_name, and range in pbn_beg_range and
// pbn_end_range.  Bundles have a CDnetex in pbn_bundle.  The pbn_name
// can provide a name for the bundle.  The range is set to the overall
// range of the bundle.
//
// The user must keep the label string and the values of these fields
// consistent.  Update_bundle and set_range are useful for this.

struct CDnetex;
struct CDnetNameStr;
typedef CDnetNameStr* CDnetName;
 
struct CDp_bnode : public CDp
{
    CDp_bnode() : CDp(0, P_BNODE)
        {
            pbn_beg_range = 0;
            pbn_end_range = 0;
            pbn_name = 0;
            pbn_bundle = 0;
            pbn_label = 0;
        }

    CDp_bnode(const CDp_bnode&);
    CDp_bnode &operator=(const CDp_bnode&);

    virtual ~CDp_bnode();

    // virtual overrides
    virtual CDp *dup()        const { return (new CDp_bnode(*this)); }

    virtual bool print(sLstr*, int, int) const;

    bool is_elec()            const { return (true); }

    virtual CDp_bnode *next()       { return ((CDp_bnode*)next_n()); }

    CDnetName get_term_name()     const { return (pbn_name); }
    void set_term_name(CDnetName n)     { pbn_name = n; }

    unsigned int beg_range()      const { return (pbn_beg_range); }
    unsigned int end_range()      const { return (pbn_end_range); }

    unsigned int width() const
        {
            return ((pbn_end_range >= pbn_beg_range ?
                pbn_end_range - pbn_beg_range :
                pbn_beg_range - pbn_end_range) + 1);
        }

    const CDnetex *bundle_spec()  const { return (pbn_bundle); }
    void set_bundle_spec(CDnetex *n)    { pbn_bundle = n; }

    void set_range(unsigned int beg, unsigned int end)
        {
            update_bundle((CDnetex*)0);
            pbn_beg_range = beg;
            pbn_end_range = end;
        }

    bool cond_bind(CDla* odesc)
        {
            if (!pbn_label)
                pbn_label = odesc;
            return (pbn_label == odesc);
        }

    void bind(CDla* odesc)          { pbn_label = odesc; }
    CDla *bound()             const { return (pbn_label); }

    bool parse_bnode(const char*);
    void update_bundle(CDnetex*);
    void add_label_text(sLstr*) const;
    CDnetex *get_netex() const;
    bool has_name() const;

protected:
    unsigned short pbn_beg_range;   // range start
    unsigned short pbn_end_range;   // range end
    CDnetName pbn_name;             // assigned terminal name, null for
                                    // default name
    CDnetex *pbn_bundle;            // bundle spec
    CDla *pbn_label;                // associated label
};

//----------------------------------------------------------------------------
// Accessory CDnetex wrapper.


// If a CDp_bnode is "named" (i.e., has_name() returns true), it can
// have a fully-defined CDnetex.  If the name is a bundle spec, the
// CDnetex already exists and can be used directly.  Otherwise, the
// name is of a simple vector, and the CDnetex has to be created. 
// This struct will hide this, making use of the existing CDnetex when
// possible.  This is a bit more efficient then calling get_netex(),
// which always copies.

struct NetexWrap
{
    NetexWrap(const CDp_bnode *pb)
        {
            nx = pb->bundle_spec();
            if (nx)
                nxtmp = 0;
            else
                nx = nxtmp = pb->get_netex();
        }

    ~NetexWrap();

    const CDnetex *netex()      const { return (nx); }

private:
    const CDnetex *nx;
    CDnetex *nxtmp;
};

//----------------------------------------------------------------------------
// Accessory range generator.

// A bus wire or terminal may have a base name and will have a range,
// and can be specified in various ways with the base name followed by
// two integers, for example foo<3:0> or foo.3.0.  The first integer
// is the start and the second integer is the end.  The 'bits' are
// sequential from start through end, e.g., 3,2,1,0 for the 3:0 range. 
// A single bit can be specified as the base name followed by an
// integer.  The integer must be within the range.  For example, foo.2
// represents the second bit from the left, i.e., 0-based index 1, of
// the 3:0 range.
//
// A bus subcircuit connector (BSC) associates a bus with scalar
// subcell connections.  The connector property has an index value,
// which represents the scalar node index connected to the bus start
// bit.  Suppose that the connector is named foo<3:0> and has index
// 5.  The bus bits connect to the node terminals with the index
// values as foo<3>=5, foo<2>=6, foo<1>=7, foo<0>=8.
//
// A bus terminal or bus wire sets up its bits to connect by name. 
// For example, foo[3:0] sets up scalar nets foo[3], foo[2], foo[1],
// foo[0] which separately associate by name.  The name association
// with indexing will match any of the specification styles, so they
// can be mixed.
//
// Bus connections are made start-to-start and progressing to the
// smaller of the two widths, if different.  If widths differ, there
// will be unconnected bits.
//
// An unnamed wire that connects bus terminals acts as a vehicle to
// associate the connected terminals.  The wire itself does not impose
// a bus width.  All terminals will be mutually connected as described
// above.
//
// If a scalar net is connected to a bus net by position, it will
// connect to all bits of the bus.  It acts like a bus connector with
// arbitrarily many bits, all tied together.
//
// A vector instance has a similar range specification.  The scalar
// instance nodes act like bus connectors (bus connectors are not
// allowed in vector instances).  When connecting by position to a bus
// net, the start device connects to the start net bit, etc.  If
// connected by position to a scalar net, all instances are connected
// to the scalar.

// A generator for the range values used with bnode and range properties.
//
struct CDgenRange
{
    CDgenRange(const CDp_bnode *pb)
    {
        rg_beg = pb ? pb->beg_range() : 0;
        rg_end = pb ? pb->end_range() : 0;
        rg_dec = (rg_beg > rg_end);
    }

    CDgenRange(const CDp_range *pr)
    {
        rg_beg = pr ? pr->beg_range() : 0;
        rg_end = pr ? pr->end_range() : 0;
        rg_dec = (rg_beg > rg_end);
    }

    // Note that it the constructor was passed a null pointer, next()
    // will allow one pass through a loop.

    bool next(unsigned int *val)
    {
        if (val)
            *val = rg_beg;
        if (rg_dec) {
            if (rg_beg < rg_end)
                return (false);
            rg_beg--;
        }
        else {
            if (rg_beg > rg_end)
                return (false);
            rg_beg++;
        }
        return (true);
    }

private:
    int rg_beg;
    int rg_end;
    bool rg_dec;
};


//----------------------------------------------------------------------------
// Bus Node property for cell instances.

struct CDp_bsnode;

struct CDp_bcnode : public CDp_bnode
{
    CDp_bcnode()
        {
            pbcn_u.pos[0] = 0;
            pbcn_u.pos[1] = 0;
            pbcn_map = 0;
            pbcn_index = 0;
            pbcn_flags = 0;
        }

    CDp_bcnode(const CDp_bcnode&);
    CDp_bcnode &operator=(const CDp_bcnode&);

    virtual ~CDp_bcnode();

    // virtual overrides
    virtual CDp *dup()        const { return (new CDp_bcnode(*this)); }

    virtual bool print(sLstr*, int, int) const;

    virtual void transform(const cTfmStack *tstk)
        {
            for (unsigned int ix = 0; ; ix++) {
                int x, y;
                if (!get_pos(ix, &x, &y))
                    break;
                tstk->TPoint(&x, &y);
                set_pos(ix, x, y);
            }
        }

    virtual CDp_bcnode *next()      { return ((CDp_bcnode*)next_n()); }

    // The coordinate list is accessed and managed by the following
    // functions.

    bool get_pos(unsigned int ix, int *px, int *py) const
        {
            if (pts_flag()) {
                if (ix < (unsigned int)pbcn_u.pts[0]) {
                    int *p = pbcn_u.pts + ix + ix + 1;
                    *px = *p++;
                    *py = *p;
                    return (true);
                }
                return (false);
            }
            if (ix == 0) {
                *px = pbcn_u.pos[0];
                *py = pbcn_u.pos[1];
                return (true);
            }
            return (false);
        }

    bool set_pos(unsigned int ix, int x, int y)
        {
            if (pts_flag()) {
                if (ix < (unsigned int)pbcn_u.pts[0]) {
                    int *p = pbcn_u.pts + ix + ix + 1;
                    *p++ = x;
                    *p = y;
                    return (true);
                }
                return (false);
            }
            if (ix == 0) {
                pbcn_u.pos[0] = x;
                pbcn_u.pos[1] = y;
                return (true);
            }
            return (false);
        }

    void alloc_pos(unsigned int ix)
        {
            if (pts_flag()) {
                delete [] pbcn_u.pts;
                set_pts_flag(false);
            }
            pbcn_u.pos[0] = 0;
            pbcn_u.pos[1] = 0;
            if (ix > 1) {
                unsigned int sz = 2*ix + 1;
                pbcn_u.pts = new int[sz];
                memset(pbcn_u.pts, 0, sz*sizeof(int));
                pbcn_u.pts[0] = ix;
                set_pts_flag(true);
            }
        }

    unsigned int size_pos()
        {
            return (pts_flag() ? pbcn_u.pts[0] : 1);
        }

    // We try to have the bits indexed sequentially across the
    // connector width, which simplifies the interface.  Then, the
    // first (leftmost) bit has index equal to pbcn_index, the next
    // will be pbcn_index+1, etc.  This is not always possible, so we
    // also have provision for a map.  With a map present, the first
    // index is map[0], etc.  When the map is in use, the index takes
    // the index of the first bit (as it would normally).

    // The map size is always equal to the width() return.

    unsigned int index()      const { return (pbcn_index); }
    void set_index(unsigned int e)  { pbcn_index = e; }

    const unsigned short *index_map() const { return (pbcn_map); }
    void set_index_map(unsigned short *m)
        {
            delete [] pbcn_map;
            pbcn_map = m;
        }

    // TE_SCINVIS, TE_SYINVIS only
    unsigned int flags()      const { return (pbcn_flags); }
    bool has_flag(unsigned int f)
                              const { return (pbcn_flags & f); }
    void set_flag(unsigned int f)   { pbcn_flags |= f; }
    void unset_flag(unsigned int f) { pbcn_flags &= ~f; }

    bool parse_bcnode(const char*);
    void set_term_name(const char*);
    CDnetName term_name() const;
    char *full_name() const;
    char *id_text() const;
    void update_range(const CDp_bsnode*);

protected:
    bool pts_flag()           const { return (pbcn_flags & TE_PTS); }
    void set_pts_flag(bool b)
        {
            if (b)
                pbcn_flags |= TE_PTS;
            else
                pbcn_flags &= ~TE_PTS;
        }

    // If there is more than one coordinate, the pts field is used. 
    // The first value is the size parameter, followed by this number
    // of x,y coordinates.  The total array size is 2*size+1 integers. 
    // The flag in pcbn_xtra tracks whether the pts field is in use.

    union {
        int *pts;       // [numxy][x][y]...
        int pos[2];     // [x][y]
    } pbcn_u;

    unsigned short *pbcn_map;       // index map
    unsigned short pbcn_index;      // node index
    unsigned short pbcn_flags;      // flags, mostly unused
};


//----------------------------------------------------------------------------
// Bus Node property for cells.

struct CDp_bsnode : public CDp_bcnode
{
    CDp_bsnode()
        {
            pbsn_x = 0;
            pbsn_y = 0;
        }

    CDp_bsnode(const CDp_bsnode&);
    CDp_bsnode &operator=(const CDp_bsnode&);

    // virtual overrides
    CDp *dup()                const { return (new CDp_bsnode(*this)); }

    bool print(sLstr*, int, int) const;

    void transform(const cTfmStack*) { }

    CDp_bsnode *next()              { return ((CDp_bsnode*)next_n()); }

    void get_schem_pos(int *px, int *py) const
        {
            *px = pbsn_x;
            *py = pbsn_y;
        }

    void set_schem_pos(int x, int y)
        {
            pbsn_x = x;
            pbsn_y = y;
        }

     // The inherited get_pos, set_pos, alloc_pos apply only to
     // symbolic coordinates!

    bool parse_bsnode(const char*);
    CDp_bcnode *dupc(CDs*, const CDc*);

private:
    int pbsn_x, pbsn_y;             // Node location in schematic.
};


//----------------------------------------------------------------------------
// Node property for wires (base node property).
//
// This supports an "external" node, and associated label.

struct CDp_node : public CDp
{
    CDp_node() : CDp(0, P_NODE)
        {
            pno_enode = 0;
            pno_name = 0;
            pno_label = 0;
        }

    CDp_node(const CDp_node&);
    CDp_node &operator=(const CDp_node&);

    virtual ~CDp_node() { }  // Needed!  This is a base;

    // virtual overrides
    virtual CDp *dup()        const { return (new CDp_node(*this)); }

    virtual bool print(sLstr*, int, int) const;

    bool is_elec()            const { return (true); }

    virtual CDp_node *next()        { return ((CDp_node*)next_n()); }

    CDnetName get_term_name()     const { return (pno_name); }
    void set_term_name(CDnetName n)     { pno_name = n; }

    int enode()                   const { return (pno_enode); }
    void set_enode(int e)               { pno_enode = e; }

    bool cond_bind(CDla* odesc)
        {
            if (!pno_label)
                pno_label = odesc;
            return (pno_label == odesc);
        }

    void bind(CDla* odesc)          { pno_label = odesc; }
    CDla *bound()             const { return (pno_label); }

    bool parse_node(const char*);

protected:
    int pno_enode;                  // scalar node external node number
    CDnetName pno_name;             // assigned terminal name, null for
                                    // default name
    CDla *pno_label;                // associated label
};


//-----------------------------------------------------------------------------
// Extended node property base
//
// This is a base class to be used by cell and instance node property
// types, providing some common support.  In particular, this
// implements a list of coordinate locations for connectivity
// hot-spots.  For an instance of a non-symbolic cell, there is one
// coordinate only.  Symbolic cells may have more than one coordinate,
// to facilitate, for example, connection points on either side of a
// symbol for tiling.  For more than one connection point, a secondary
// allocation of an integer array is required.

struct CDp_nodeEx : public CDp_node
{
    CDp_nodeEx()
        {
            pxno_u.pos[0] = 0;
            pxno_u.pos[1] = 0;
            pxno_flags = 0;
            pxno_termtype = TE_tINPUT;
            pxno_index = 0;
        }

    CDp_nodeEx(const CDp_nodeEx&);
    CDp_nodeEx &operator=(const CDp_nodeEx&);

    virtual ~CDp_nodeEx();

    // virtual overrides
    CDp *dup()                const { return (new CDp_nodeEx(*this)); }

    void transform(const cTfmStack *tstk)
        {
            for (unsigned int ix = 0; ; ix++) {
                int x, y;
                if (!get_pos(ix, &x, &y))
                    break;
                tstk->TPoint(&x, &y);
                set_pos(ix, x, y);
            }
        }

    virtual unsigned int term_flags()   const { return (0); }

    virtual void set_term_name(const char*);
    CDnetName term_name() const;

    CDp_nodeEx *next()              { return ((CDp_nodeEx*)next_n()); }

    // The coordinate list is accessed and managed by the following
    // functions.

    bool get_pos(unsigned int ix, int *px, int *py) const
        {
            if (pts_flag()) {
                if (ix < (unsigned int)pxno_u.pts[0]) {
                    int *p = pxno_u.pts + ix + ix + 1;
                    *px = *p++;
                    *py = *p;
                    return (true);
                }
                return (false);
            }
            if (ix == 0) {
                *px = pxno_u.pos[0];
                *py = pxno_u.pos[1];
                return (true);
            }
            return (false);
        }

    bool set_pos(unsigned int ix, int x, int y)
        {
            if (pts_flag()) {
                if (ix < (unsigned int)pxno_u.pts[0]) {
                    int *p = pxno_u.pts + ix + ix + 1;
                    *p++ = x;
                    *p = y;
                    return (true);
                }
                return (false);
            }
            if (ix == 0) {
                pxno_u.pos[0] = x;
                pxno_u.pos[1] = y;
                return (true);
            }
            return (false);
        }

    void alloc_pos(unsigned int ix)
        {
            if (pts_flag()) {
                delete [] pxno_u.pts;
                set_pts_flag(false);
            }
            pxno_u.pos[0] = 0;
            pxno_u.pos[1] = 0;
            if (ix > 1) {
                unsigned int sz = 2*ix + 1;
                pxno_u.pts = new int[sz];
                memset(pxno_u.pts, 0, sz*sizeof(int));
                pxno_u.pts[0] = ix;
                set_pts_flag(true);
            }
        }

    unsigned int size_pos()
        {
            return (pts_flag() ? pxno_u.pts[0] : 1);
        }

    unsigned int index()      const { return (pxno_index); }
    void set_index(unsigned int i)  { pxno_index = i; }

    unsigned int flags()      const { return (pxno_flags); }
    bool has_flag(unsigned int f)
                              const { return (pxno_flags & f); }
    void set_flag(unsigned int f)   { pxno_flags |= f; }
    void unset_flag(unsigned int f) { pxno_flags &= ~f; }

    CDtermType termtype()     const { return ((CDtermType)pxno_termtype); }
    void set_termtype(CDtermType t) { pxno_termtype = t; }

    char *id_text() const;

protected:
    bool pts_flag()           const { return (pxno_flags & TE_PTS); }
    void set_pts_flag(bool b)
        {
            if (b)
                pxno_flags |= TE_PTS;
            else
                pxno_flags &= ~TE_PTS;
        }

    // If there is more than one coordinate, the pts field is used. 
    // The first value is the size parameter, followed by this number
    // of x,y coordinates.  The total array size is 2*size+1 integers. 
    // The flag in pxno_ival tracks whether the pts field is in use.

    union {
        int *pts;       // [numxy][x][y]...
        int pos[2];     // [x][y]
    } pxno_u;

    unsigned short pxno_index;      // terminal ordering index
    unsigned short pxno_termtype;   // CDtermType
    unsigned int pxno_flags;        // flags
};


//-----------------------------------------------------------------------------
// Node property for cell instances.

struct CDp_cnode : public CDp_nodeEx
{
    CDp_cnode()
        {
            pcno_term = 0;
        }

    CDp_cnode(const CDp_cnode&);
    CDp_cnode &operator=(const CDp_cnode&);

    virtual ~CDp_cnode();

    // virtual overrides
    CDp *dup()                const { return (new CDp_cnode(*this)); }

    bool print(sLstr*, int, int) const;
    unsigned int term_flags() const;

    CDp_cnode *next()               { return ((CDp_cnode*)next_n()); }

    CDcterm *inst_terminal()  const { return (pcno_term); }
    void set_terminal(CDcterm *t)   { pcno_term = t; }

    bool parse_cnode(const char*);

private:
    CDcterm *pcno_term;             // Physical terminal descriptor.
};


//-----------------------------------------------------------------------------
// Node property for cells.
//
// This provides an additional coordinate pair used for the hot-spot
// in the schematic representation.  The inherited point list is used
// (only) for symbolic reference points.
//
// This property also records the initial physical location and layer
// of the corresponding terminal in the layout.  This is used for
// initialization.

struct CDp_snode : public CDp_nodeEx
{
    CDp_snode()
        {
            psno_term = 0;
            psno_x = 0;
            psno_y = 0;
        }

    CDp_snode(const CDp_snode&);
    CDp_snode &operator=(const CDp_snode&);

    virtual ~CDp_snode();

    // virtual overrides
    CDp *dup()                const { return (new CDp_snode(*this)); }

    bool print(sLstr*, int, int) const;
    unsigned int term_flags() const;
    void set_term_name(const char*);

    void transform(const cTfmStack*) { }

    CDp_snode *next()               { return ((CDp_snode*)next_n()); }

    void get_schem_pos(int *px, int *py) const
        {
            *px = psno_x;
            *py = psno_y;
        }

    void set_schem_pos(int x, int y)
        {
            psno_x = x;
            psno_y = y;
        }

    CDsterm *cell_terminal()  const { return (psno_term); }
    void set_terminal(CDsterm *t)   { psno_term = t; }

    bool parse_snode(CDs*, const char*);
    CDp_cnode *dupc(CDs*, const CDc*);

private:
    CDsterm *psno_term;             // Physical terminal descriptor.
    int psno_x, psno_y;             // Node location in schematic.
};


//-----------------------------------------------------------------------------
// Name property.

// The name property name string can start with one of these special
// characters, to indicate a terminal type of device.  Additionally,
// the ground device has no name property, but contains exactly one
// node property.  Spice devices will have an alpha character key.
//
#define P_NAME_NULL '%'
#define P_NAME_TERM '@'
#define P_NAME_BTERM_DEPREC '#'

struct CDp_name : public CDp
{
    CDp_name() : CDp(P_NAME)
        {
            pna_num = 0;
            pna_label = 0;
            pna_name = 0;
            pna_setname = 0;
            pna_labtext = 0;
            pna_scindex = 0;
            pna_subckt = false;
            pna_located = false;
            pna_x = pna_y = 0;
        }

    CDp_name(const CDp_name&);
    CDp_name &operator=(const CDp_name&);

    virtual ~CDp_name()
        {
            delete [] pna_setname;
            delete [] pna_labtext;
        }


    // virtual overrides
    CDp *dup()                  const { return (new CDp_name(*this)); }

    bool print(sLstr*, int, int) const;

    bool cond_bind(CDla* odesc)
        {
            if (!pna_label)
                pna_label = odesc;
            return (pna_label == odesc);
        }

    void bind(CDla* odesc)            { pna_label = odesc; }
    CDla *bound()               const { return (pna_label); }

    void purge(CDo *odesc)
        {
            if (odesc == (CDo*)pna_label)
                pna_label = 0;
        }

    hyList *label_text(bool*, CDc* = 0) const;

    bool is_elec()              const { return (true); }

    CDp_name *next()                  { return ((CDp_name*)next_n()); }

    bool is_subckt()            const { return (pna_subckt); }
    void set_subckt(bool b)           { pna_subckt = b; }

    unsigned int number()       const { return (pna_num); }
    void set_number(unsigned int n)   { pna_num = n; }

    unsigned int scindex()      const { return (pna_scindex); }
    void set_scindex(unsigned int n)  { pna_scindex = n; }

    CDpfxName name_string()     const { return (pna_name); }
    void set_name_string(CDpfxName n) { pna_name = n; }
    void set_name_string(const char *n)
        {
            if (n && *n)
                pna_name = CD()->PfxTableAdd(n);
        }

    int key()                   const
        {
            int c = (pna_name ? *pna_name->string() : 0);
            return (isupper(c) ? tolower(c) : c);
        }

    const char *assigned_name() const { return (pna_setname); }
    void set_assigned_name(const char *n)
        {
            char *s = lstring::copy(n);
            delete [] pna_setname;
            pna_setname = s;
        }

    const char *label_text()    const { return (pna_labtext); }
    void set_label_text(const char *n)
        {
            char *s = lstring::copy(n);
            delete [] pna_labtext;
            pna_labtext = s;
        }

    bool located()              const { return (pna_located); }
    void set_located(bool b)          { pna_located = b; }

    int pos_x()                 const { return (pna_x); }
    void set_pos_x(int x)             { pna_x = x; }
    int pos_y()                 const { return (pna_y); }
    void set_pos_y(int y)             { pna_y = y; }

    bool parse_name(const char*);

private:
    unsigned int pna_num;       // flag, name index
    CDla *pna_label;            // associated label
    CDpfxName pna_name;         // name prefix
    char *pna_setname;          // overriding name
    char *pna_labtext;          // associated label text
    int pna_scindex;            // alternative index for subckts
    bool pna_subckt;            // true if subckt
    bool pna_located;           // physical location valid.
    int pna_x, pna_y;           // physical location of subckt ref. label
};


//-----------------------------------------------------------------------------
// Label Location property.

struct CDp_labloc : public CDp
{
    CDp_labloc() : CDp(P_LABLOC)
        {
            ploc_name = -1;
            ploc_model = -1;
            ploc_value = -1;
            ploc_param = -1;

            ploc_devref = -1;
            ploc_xtra1 = -1;
            ploc_xtra2 = -1;
            ploc_xtra3 = -1;
        }

    // virtual overrides
    CDp *dup()                const { return (new CDp_labloc(*this)); }

    bool print(sLstr*, int, int) const;

    bool is_elec()            const { return (true); }

    int name_code()           const { return (ploc_name); }
    void set_name_code(int c)       { ploc_name = c; }
    int model_code()          const { return (ploc_model); }
    void set_model_code(int c)      { ploc_model = c; }
    int value_code()          const { return (ploc_value); }
    void set_value_code(int c)      { ploc_value = c; }
    int param_code()          const { return (ploc_param); }
    void set_param_code(int c)      { ploc_param = c; }
    int devref_code()         const { return (ploc_devref); }
    void set_devref_code(int c)     { ploc_devref = c; }

    bool parse_labloc(const char*);

private:
    signed char ploc_name;
    signed char ploc_model;
    signed char ploc_value;
    signed char ploc_param;

    signed char ploc_devref;
    signed char ploc_xtra1;
    signed char ploc_xtra2;
    signed char ploc_xtra3;
};


//-----------------------------------------------------------------------------
// Old Mutual inductor property (obsolete)

struct CDp_mut : public CDp
{
    CDp_mut() : CDp(P_MUT)
        {
            pmu_x1 = pmu_y1 = 0;
            pmu_x2 = pmu_y2 = 0;
            pmu_coeff = 0.0;
        }

    // virtual overrides
    CDp *dup()                const { return (new CDp_mut(*this)); }

    bool print(sLstr*, int, int) const;

    void transform(const cTfmStack *tstk)
        {
            tstk->TPoint(&pmu_x1, &pmu_y1);
            tstk->TPoint(&pmu_x2, &pmu_y2);
        }

    bool is_elec()            const { return (true); }

    CDp_mut *next()                 { return ((CDp_mut*)next_n()); }

    double coeff()            const { return (pmu_coeff); }
    void set_coeff(double c)        { pmu_coeff = c; }
    int pos1_x()              const { return (pmu_x1); }
    void set_pos1_x(int x)          { pmu_x1 = x; }
    int pos1_y()              const { return (pmu_y1); }
    void set_pos1_y(int y)          { pmu_y1 = y; }
    int pos2_x()              const { return (pmu_x2); }
    void set_pos2_x(int x)          { pmu_x2 = x; }
    int pos2_y()              const { return (pmu_y2); }
    void set_pos2_y(int y)          { pmu_y2 = y; }

    bool parse_mut(const char*);
    void get_coords(int*, int*, int*, int*) const;

    static CDc *find(int, int, CDs*);

private:
    double pmu_coeff;
    int pmu_x1, pmu_y1;
    int pmu_x2, pmu_y2;
};


//-----------------------------------------------------------------------------
// Mutual inductor property.

struct CDp_nmut : public CDp
{
    CDp_nmut() : CDp(P_NEWMUT)
        {
            pnmu_indx = 0;
            pnmu_label = 0;
            pnmu_coefstr = 0;
            pnmu_num1 = 0;
            pnmu_num2 = 0;
            pnmu_odesc1 = 0;
            pnmu_odesc2 = 0;
            pnmu_name1 = 0;
            pnmu_name2 = 0;
            pnmu_setname = 0;
        }

    CDp_nmut(int, char*, int, char*, int, char*, char*);

    CDp_nmut(const CDp_nmut&);
    CDp_nmut &operator=(const CDp_nmut&);

    ~CDp_nmut()
        {
            delete [] pnmu_name1;
            delete [] pnmu_name2;
            delete [] pnmu_setname;
            delete [] pnmu_coefstr;
        }

    // virtual overrides
    CDp *dup()                const { return (new CDp_nmut(*this)); }

    bool print(sLstr*, int, int) const;

    bool cond_bind(CDla* odesc)
        {
            if (!pnmu_label)
                pnmu_label = odesc;
            return (pnmu_label == odesc);
        }

    void bind(CDla* odesc)          { pnmu_label = odesc; }
    CDla *bound()             const { return (pnmu_label); }

    void purge(CDo *odesc)
        {
            if (odesc == (CDo*)pnmu_label)
                pnmu_label = 0;
            if (odesc == (CDo*)pnmu_odesc1)
                pnmu_odesc1 = 0;
            if (odesc == (CDo*)pnmu_odesc2)
                pnmu_odesc2 = 0;
        }

    hyList *label_text(bool*, CDc* = 0) const;

    bool is_elec()            const { return (true); }

    CDp_nmut *next()                { return ((CDp_nmut*)next_n()); }

    int index()               const { return (pnmu_indx); }
    void set_index(int i)           { pnmu_indx = i; }
    const char *coeff_str()   const { return (pnmu_coefstr); }

    void set_coeff_str(const char *n)
        {
            char *s = lstring::copy(n);
            delete [] pnmu_coefstr;
            pnmu_coefstr = s;
        }

    int l1_index()            const { return (pnmu_num1); }
    void set_l1_index(int i)        { pnmu_num1 = i; }
    int l2_index()            const { return (pnmu_num2); }
    void set_l2_index(int i)        { pnmu_num2 = i; }
    CDc *l1_dev()             const { return (pnmu_odesc1); }
    void set_l1_dev(CDc *cd)        { pnmu_odesc1 = cd; }
    CDc *l2_dev()             const { return (pnmu_odesc2); }
    void set_l2_dev(CDc *cd)        { pnmu_odesc2 = cd; }
    const char *l1_name()     const { return (pnmu_name1); }
    const char *l2_name()     const { return (pnmu_name2); }
    const char *assigned_name() const { return (pnmu_setname); }

    void set_assigned_name(const char *n)
        {
            char *s = lstring::copy(n);
            delete [] pnmu_setname;
            pnmu_setname = s;
        }

    bool parse_nmut(const char*);
    bool rename(CDs*, const char*);
    void updat_ref(const CDc*, CDc*);
    bool get_descs(CDc**, CDc**) const;
    bool match(CDc*, CDc**) const;

private:
    int pnmu_indx;
    CDla *pnmu_label;       // associated label
    char *pnmu_coefstr;
    int pnmu_num1;
    int pnmu_num2;
    CDc *pnmu_odesc1;
    CDc *pnmu_odesc2;
    char *pnmu_name1;
    char *pnmu_name2;
    char *pnmu_setname;
};


//-----------------------------------------------------------------------------
// Branch property.

struct CDp_branch : public CDp
{
    CDp_branch() : CDp(P_BRANCH)
        {
            pb_x = pb_y = 0;
            pb_r1 = pb_r2 = 0;
            pb_bstring = 0;
        }

    CDp_branch(const CDp_branch&);
    CDp_branch &operator=(const CDp_branch&);

    ~CDp_branch() { delete [] pb_bstring; }

    // virtual overrides
    CDp *dup()                const { return (new CDp_branch(*this)); }

    bool print(sLstr*, int, int) const;

    void transform(const cTfmStack *tstk)
        {
            tstk->TPoint(&pb_x, &pb_y);
            int rx = pb_r1, ry = pb_r2;
            tstk->TPoint(&rx, &ry);
            int xx = 0, yy = 0;
            tstk->TPoint(&xx, &yy);
            pb_r1 = rx - xx;
            pb_r2 = ry - yy;
        }

    bool is_elec()            const { return (true); }

    CDp_branch *next()              { return ((CDp_branch*)next_n()); }

    int rot_x()               const { return (pb_r1); }
    void set_rot_x(int r)           { pb_r1 = r; }
    int rot_y()               const { return (pb_r2); }
    void set_rot_y(int r)           { pb_r2 = r; }
    int pos_x()               const { return (pb_x); }
    void set_pos_x(int x)           { pb_x = x; }
    int pos_y()               const { return (pb_y); }
    void set_pos_y(int y)           { pb_y = y; }
    const char *br_string()   const { return (pb_bstring); }

    void set_br_string(const char *n)
        {
            char *s = lstring::copy(n);
            delete [] pb_bstring;
            pb_bstring = s;
        }

    bool parse_branch(const char*);

private:
    short int pb_r1;
    short int pb_r2;
    int pb_x;
    int pb_y;
    char *pb_bstring;
};


//-----------------------------------------------------------------------------
// Label Reference property.

struct CDp_lref : public CDp
{
    CDp_lref() : CDp(P_LABRF)
        {
            plr_num = 0;
            plr_num2 = 0;
            plr_prpnum = 0;
            plr_name = 0;
            plr_prpref = 0;
            plr.devref = 0;
        }

    CDp_lref(const CDp_lref&);
    CDp_lref &operator=(const CDp_lref&);

    // virtual overrides
    CDp *dup()                  const { return (new CDp_lref(*this)); }

    bool print(sLstr*, int, int) const;

    bool is_elec()              const { return (true); }

    CDp_lref *next()                  { return ((CDp_lref*)next_n()); }

    CDo *find_my_object(CDs*, CDla* = 0);

    int number()                const { return (plr_num); }
    void set_number(int n)            { plr_num = n; }

    void set_xy(int x, int y)         { plr_num = x; plr_num2 = y; }
    int pos_x()                 const { return (plr_num); }
    int pos_y()                 const { return (plr_num2); }

    int propnum()               const { return (plr_prpnum); }
    void set_propnum(int n)           { plr_prpnum = n; }

    CDpfxName name()            const { return (plr_name); }
    void set_name(CDpfxName n)        { plr_name = n; }

    void set_name(const char *n)
        {
            if (n && *n)
                plr_name = CD()->PfxTableAdd(n);
        }

    CDp *propref()              const { return (plr_prpref); }
    void set_propref(CDp *p)          { plr_prpref = p; }
    CDc *devref()               const { return (plr.devref); }
    void set_devref(CDc *c)           { plr.devref = c; }
    CDs *cellref()              const { return (plr.cellref); }
    void set_cellref(CDs *s)          { plr.cellref = s; }
    CDw *wireref()              const { return (plr.wireref); }
    void set_wireref(CDw *w)          { plr.wireref = w; }

    // When used as a wire node name label, plr_name and plr_prpref
    // are always 0.

    bool parse_lref(const char*);

private:
    int plr_num;            // instance index or wire x position
    int plr_num2;           // wire y position
    int plr_prpnum;         // property number
    CDpfxName plr_name;     // instance name prefix
    CDp *plr_prpref;        // bound-to property
    union {
        CDc *devref;        // bound-to instance
        CDs *cellref;       // bound-to cell
        CDw *wireref;       // bound-to wire
    } plr;
};


//-----------------------------------------------------------------------------
// Mutual Reference property.

struct CDp_mutlrf : public CDp
{
    CDp_mutlrf() : CDp(P_MUTLRF) { }

    // virtual overrides
    CDp *dup()                const { return (new CDp_mutlrf(*this)); }

    bool print(sLstr*, int, int) const;

    bool is_elec()            const { return (true); }

    CDp_mutlrf *next()              { return ((CDp_mutlrf*)next_n()); }
};


//-----------------------------------------------------------------------------
// Symbolic property.

struct CDp_sym : public CDp
{
    CDp_sym() : CDp(P_SYMBLC)
        {
            ps_active = true;
            ps_rep = 0;
        }

    CDp_sym(const CDp_sym&);
    CDp_sym &operator=(const CDp_sym&);

    ~CDp_sym() { delete ps_rep; }

    // This is a bit of a hack.  When copying and there is a rep the
    // virtual override dup() returns 0.  One can check for this, and
    // call dup(CDs*) which will set the rep owner to the new owning
    // cell.  The rep will always be 0 in CDc properties.
    //
    CDp *dup() const
        {
            if (ps_rep)
                return (0);
            return (new CDp_sym(*this));
        }

    CDp *dup(CDs *sd)
        {
            CDp_sym *ps = new CDp_sym(*this);
            if (ps->ps_rep)
                ps->ps_rep->setOwner(sd);
            return (ps);
        }

    bool print(sLstr*, int, int) const;

    bool is_elec()            const { return (true); }

    CDp_sym *next()                 { return ((CDp_sym*)next_n()); }

    bool active()             const { return (ps_active); }
    void set_active(bool b)         { ps_active = b; }
    CDs *symrep()             const { return (ps_rep); }
    void set_symrep(CDs *sd)        { ps_rep = sd; }

    bool parse_sym(CDs*, const char*);

private:
    bool ps_active;
    CDs *ps_rep;
};


//-----------------------------------------------------------------------------
// Nodemap property.

// List element for CDp_nodmp below.
struct CDnmapRef
{
    CDnmapRef()
        {
            name = 0;
            x = y = 0;
        }

    CDnetName name;
    int x, y;
};

struct CDp_nodmp : public CDp
{
    CDp_nodmp() : CDp(P_NODMAP)
        {
            pnm_size = 0;
            pnm_list = 0;
        }

    CDp_nodmp(const CDp_nodmp&);
    CDp_nodmp &operator=(const CDp_nodmp&);

    ~CDp_nodmp() { delete [] pnm_list; }

    // virtual overrides
    CDp *dup()                const { return (new CDp_nodmp(*this)); }

    bool print(sLstr*, int, int) const;

    bool is_elec()            const { return (true); }

    CDp_nodmp *next()               { return ((CDp_nodmp*)next_n()); }

    int mapsize()             const { return (pnm_size); }
    void set_mapsize(int s)         { pnm_size = s; }
    CDnmapRef *maplist()      const { return (pnm_list); }
    void set_maplist(CDnmapRef *c)  { pnm_list = c; }

    bool parse_nodmp(const char*);

private:
    int pnm_size;
    CDnmapRef *pnm_list;
};


//-----------------------------------------------------------------------------
// List type for properties
//-----------------------------------------------------------------------------

struct CDpl
{
    CDpl()
        {
            pdesc = 0;
            next = 0;
        }

    CDpl(CDp *p, CDpl *n)
        {
            pdesc = p;
            next = n;
        }

    static void destroy(const CDpl *p)
        {
            while (p) {
                const CDpl *px = p;
                p = p->next;
                delete px;
            }
        }

    static void sort(CDpl*);

    CDp *pdesc;
    CDpl *next;
};

#endif


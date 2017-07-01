
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2013 Whiteley Research Inc, all rights reserved.        *
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
 $Id: sced_connect.h,v 5.11 2016/06/02 03:53:57 stevew Exp $
 *========================================================================*/

#ifndef SCED_CONNECT_H
#define SCED_CONNECT_H


class cScedConnect
{
public:
    // Connect-by-name reference element for cn_u.tname_tab. 
    // Only one node is actually saved in the tab, which
    // supplies the "real" node number after init returns. 
    // The node field is used in init to assign to other
    // terminals of the same name.  The table accesses these
    // elements by name and index, a negative index implies a
    // scalar net, else the element refers to part of a bus.
    //
    struct name_elt
    {
        // sntable_t<> stuff
        const char *tab_name()          { return (ne_name->string()); }
        int tab_indx()                  { return (ne_indx); }
        name_elt *tab_next()            { return (ne_next); }
        void set_tab_next(name_elt *n)  { ne_next = n; }
        name_elt *tgen_next(bool)       { return (ne_next); }

        // This is block-allocated, no constructor or destructor.
        // The name is a pointer into a string table.

        void set(int node, CDnetName nm, int indx, CDp_node *n)
            {
                ne_next = 0;
                ne_name = nm;
                ne_pn = n;
                ne_indx = indx;
                ne_node = node;
            }

        CDnetName name()            const { return (ne_name); }
        CDp_node *nodeprp()         const { return (ne_pn); }
        int index()                 const { return (ne_indx); }
        int nodenum()               const { return (ne_node); }

    private:
        name_elt *ne_next;
        CDnetName ne_name;      // Base name.
        CDp_node *ne_pn;        // Node property.
        int ne_indx;            // Index value, scalar if < 0.
        int ne_node;            // Node number, used in init only to
                                // assign same node to same name.
    };

    // Node property list element, used in cn_ntab.
    //
    struct node_list
    {
        node_list(CDp_node *p, node_list *n)
            {
                nl_next = n;
                nl_np = p;
            }

        void free()
            {
                node_list *n = this;
                while (n) {
                    node_list *nx = n;
                    n = n->nl_next;
                    delete nx;
                }
            }

        node_list *next()               { return (nl_next); }
        void set_next(node_list *n)     { nl_next = n; }
        CDp_node *node()                { return (nl_np); }

    private:
        node_list *nl_next;
        CDp_node *nl_np;   // node property of terminal, wire, or
                           // device/subc connection
    };

    // Stack element for the wire stack used for wire net traversal.
    //
    struct cstk_elt
    {
        cstk_elt(CDnetex *n, const CDw *wd, cstk_elt *nx)
            {
                stk_next = nx;
                stk_netex = n;
                stk_wdesc = wd;
            }

        ~cstk_elt()
            {
                CDnetex::destroy(stk_netex);
            }

        cstk_elt *next()            const { return (stk_next); }
        const CDnetex *netex()      const { return (stk_netex); }
        const CDw *wdesc()          const { return (stk_wdesc); }

    private:
        cstk_elt *stk_next;
        CDnetex *stk_netex;
        const CDw *stk_wdesc;
    };

    cScedConnect()
        {
            cn_sdesc = 0;
            cn_ntab = 0;
            cn_u.tname_tab = 0;
            cn_tmp_wires = 0;
            cn_ndprps = 0;
            cn_btprps = 0;
            cn_count = 0;
            cn_ntsize = 0;
            cn_wire_tab = 0;
            cn_wire_stack = 0;
            cn_un_bnode = 0;
            cn_un_node = 0;
            cn_un_cdesc = 0;
            cn_pass = 0;
            cn_case_insens = false;
        }
    ~cScedConnect();

    void run(CDs*, bool);

private:
    void init(CDs*, bool);
    bool init_terminal(CDc*);
    void init_nophys_shorts();

    void connect();
    bool push(const CDw*);
    bool push_unnamed(const CDw*);
    void pop();
    bool infer_name(const CDw*, CDnetex**);
    bool find_a_terminal(const CDw*);
    bool is_connected(const CDw*);
    bool is_connected_higher(const CDw*);
    CDnetex *get_netex(const CDw*);
    bool is_compatible(const CDw*, CDnetex**);
    void iterate_wire();
    void iterate_unnamed_wire();
    void connect_wires();
    void connect_to_terminals();
    void wire_to_term_dev(const CDc*, CDp_cnode*, const CDp_bcnode*);
    void wire_to_inst(const CDc*, CDp_cnode*, const CDp_bcnode*,
        const CDp_range*);
    void wire_to_cell(CDp_snode*, const CDp_bsnode*);
    void connect_inst_to_inst(const CDc*, CDp_cnode*,
        const CDp_bcnode*, const CDp_range*, int, int);
    void connect_cell_to_inst(CDp_snode*, const CDp_bsnode*, int, int);

    void bit_to_bit(CDp_nodeEx*, const CDp_range*, CDp_nodeEx*,
        const CDp_range*);
    void bit_to_inst(CDp_nodeEx*, const CDp_range*, const CDc*,
        const CDp_bcnode*, const CDp_range*);
    void bit_to_named(CDp_nodeEx*, const CDp_range*, const CDnetex*);
    void bit_to_cell(CDp_nodeEx*, const CDp_range*, const CDp_bsnode*);
    void wbit_to_bit(CDp_node*, CDp_nodeEx*, const CDp_range*);
    void wbit_to_inst(CDp_node*, const CDc*, const CDp_bcnode*,
        const CDp_range*);
    void wbit_to_named(CDp_node*, const CDnetex*);
    void wbit_to_cell(CDp_node*, const CDp_bsnode*);
    void inst_to_inst(const CDc*, const CDp_bcnode*, const CDp_range*,
        const CDc*, const CDp_bcnode*, const CDp_range*);
    void inst_to_named(const CDc*, const CDp_bcnode*, const CDp_range*,
        const CDnetex*);
    void inst_to_cell(const CDc*, const CDp_bcnode*, const CDp_range*,
        const CDp_bsnode*);
    void named_to_named(const CDnetex*, const CDnetex*);
    void named_to_cell(const CDnetex*, const CDp_bsnode*);
    void cell_to_cell(const CDp_bsnode*, const CDp_bsnode*);
    void connect_nodes(CDp_node*, CDp_node*);

    void setup_map(CDp_bsnode*);
    void new_node();
    void add_to_ntab(int, CDp_node*);
    void add_to_tname_tab(name_elt*);
    name_elt *tname_tab_find(const char*, int);
    void merge(int, int);
    void reduce(int);
    bool find_node(const char*, int*);
    int find_node(int, int);

    CDs *cn_sdesc;              // current cell
    node_list **cn_ntab;        // node list array
    union {
        sntable_t<name_elt> *tname_tab;     // terminal names
        csntable_t<name_elt> *tname_tab_ci; // case-insens terminal names
    } cn_u;
    CDol *cn_tmp_wires;         // temp wires, for P_NOPHYS
    CDp_cnode *cn_ndprps;       // dummy node properties for node mapping
    CDp_cnode *cn_btprps;       // more dummy node properties
    int cn_count;               // node number for assignment
    int cn_ntsize;              // size of node list array
    SymTab *cn_wire_tab;        // wires we've seen
    cstk_elt *cn_wire_stack;    // wire tree
    CDp_bcnode *cn_un_bnode;    // reference unnamed terminal
    CDp_cnode *cn_un_node;      // reference unnamed vector instance terminal
    const CDc *cn_un_cdesc;     // reference unnamed instance
    unsigned short cn_pass;     // internal state 
    bool cn_case_insens;        // case insens terminal name matching

    eltab_t<name_elt> cn_name_elt_alloc;        // block allocator
};

#endif


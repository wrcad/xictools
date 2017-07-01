
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
 $Id: cd_terminal.h,v 5.37 2016/06/02 03:53:57 stevew Exp $
 *========================================================================*/

#ifndef CD_TERMINAL_H
#define CD_TERMINAL_H


// Base class for terminals.
//
// Once the terminal is implemented:
//   These may have t_oset set to an object under the terminal.
//   If no t_oset, then t_vgroup will be set.
//
struct CDterm
{
    CDterm()
        {
            t_ldesc = 0;
            t_x = 0;
            t_y = 0;
            t_oset = 0;
            t_vgroup = -1;
            t_flags = TE_UNINIT;
        }

    CDterm(const CDterm &t)
        {
            t_ldesc = t.t_ldesc;
            t_x = t.t_x;
            t_y = t.t_y;
            t_oset = 0;
            t_vgroup = -1;
            t_flags = TE_UNINIT;
        }

    CDterm& operator=(const CDterm &t)
        {
            t_ldesc = t.t_ldesc;
            t_x = t.t_x;
            t_y = t.t_y;
            t_oset = 0;
            t_vgroup = -1;
            t_flags = TE_UNINIT;
            return (*this);
        }

    virtual ~CDterm()
        {
            // Unlink from t_oset.
            set_ref(0);
            CD()->ifInvalidateTerminal(this);
        }

    CDl* layer()       const { return (t_oset ? t_oset->ldesc() : t_ldesc); }
    void set_layer(CDl *ld)  { t_ldesc = ld; }

    int lx()                const { return (t_x); }
    int ly()                const { return (t_y); }

    bool is_loc_same(CDterm *t) const
        { return (t_x == t->t_x && t_y == t->t_y); }

    virtual CDnetName name() const = 0;
    virtual CDc *instance() const = 0;
    virtual int inst_index() const = 0;

    void set_loc(int x, int y)
        {
            t_x = x;
            t_y = y;
            t_flags |= TE_LOCSET;
        }

    int group() const
        {
            if (t_flags & TE_UNINIT)
                return (-1);
            const CDo *od = get_ref();
            if (od)
                return (od->group());
            return (get_v_group());
        }

    int get_v_group()       const { return (t_vgroup); }

    CDo *get_ref()          const { return (t_oset); }
    void clear_ref()
        {
            t_oset = 0;
            t_vgroup = -1;
            t_flags |= TE_UNINIT;
        }

    bool is_uninit()        const { return (t_flags & TE_UNINIT); }
    void set_uninit(bool b)
        {
            if (b)
                t_flags |= TE_UNINIT;
            else
                t_flags &= ~TE_UNINIT;
        }

    bool is_fixed()         const { return (t_flags & TE_FIXED); }
    void set_fixed(bool b)
        {
            if (b)
                t_flags |= TE_FIXED;
            else
                t_flags &= ~TE_FIXED;
        }

    bool is_loc_set()       const { return (t_flags & TE_LOCSET); }
    void set_loc_set(bool b)
        {
            if (b)
                t_flags |= TE_LOCSET;
            else
                t_flags &= ~TE_LOCSET;
        }

    void set_ref(CDo *o);
    void set_v_group(int);

protected:
    CDl *t_ldesc;           // Layer of physical connection.
    int t_x, t_y;           // Physical placement of terminal.
    CDo *t_oset;            // Physical obj terminal is assigned to, if any.
    int t_vgroup;           // Virtual group if no physical assignment.
    unsigned int t_flags;   // UNINIT, FIXED, LOCSET flags
};


// A "formal" cell physical terminal.
// These can be placed by the user.
//
// These are owned by the physical cell descriptor, but are linked
// into corresponding electrical node properties.
//
//
struct CDsterm : public CDterm
{
    CDsterm(CDs *sd, CDnetName n)
        {
            t_name = n;
            t_owner = sd;
            t_node = 0;
        }

    CDsterm(const CDsterm &t) : CDterm(t)
        {
            t_name = t.t_name;
            t_owner = 0;
            t_node = 0;
        }

    CDsterm& operator=(const CDsterm &t)
        {
            (CDterm&)*this = (const CDterm&)t;
            t_name = t.t_name;
            t_owner = 0;
            t_node = 0;
            return (*this);
        }

    ~CDsterm()
        {
            if (t_owner) {
                // Unlink from property.
                if (t_node) {
                    t_node->set_terminal(0);
                    t_node = 0;
                }

                // Unlink from cell.
                t_owner->removePinTerm(this);
            }
        }

    // Virtual overrides.
    CDnetName name()    const { return (t_name); }
    CDc *instance()     const { return (0); }
    int inst_index()    const { return (0); }

    CDp_snode *node_prpty()     const { return (t_node); }
    void set_node_prpty(CDp_snode *p) { t_node = p; }

    void set_name(CDnetName n) { t_name = n; }

private:
    // The t_name tracks the name from the node, but will provide the
    // terminal name when the mode property is unavailable (no
    // electrical cell).

    CDnetName t_name;       // Terminal name.
    CDs *t_owner;           // Owning cell.
    CDp_snode *t_node;      // Pointer to associated node property.
};


// The "instance" terminals that are linked into a CDp_cnode.
//   These always have t_cdesc set.
//   These are placed by Xic using the corresponding formal terminal and
//   the instance transformation.
//   The t_group is set to the connected group (if any).
//   The t_ldesc is set to the same value as the corresponding formal
//   terminal.
//
// These are linked into and owned by electrical instance node
// properties.
//
struct CDcterm : public CDterm
{
    CDcterm(CDp_cnode *n)
        {
            t_node = n;
            t_cdesc = 0;
            t_vecix = 0;
        }

    CDcterm(const CDcterm &t) : CDterm(t)
        {
            t_node = 0;
            t_cdesc = 0;
            t_vecix = 0;
        }

    CDcterm& operator=(const CDcterm &t)
        {
            (CDterm&)*this = (const CDterm&)t;
            t_node = 0;
            t_cdesc = 0;
            t_vecix = 0;
            return (*this);
        }

    ~CDcterm()
        {
            // Unlink from t_node.
            if (t_node) {
                t_node->set_terminal(0);
                t_node = 0;
            }
        }

    // Virtual overrides.
    CDnetName name()            const { return (t_node ?
                                      t_node->term_name() : undef_name()); }
    CDc *instance()             const { return (t_cdesc); }
    int inst_index()            const { return (t_vecix); }

    CDp_cnode *node_prpty()     const { return (t_node); }
    void set_node_prpty(CDp_cnode *p) { t_node = p; }

    int enode()        const { return (t_node ? t_node->enode() : -1); }
    int index()        const { return (t_node ? t_node->index() : -1); }

    void set_instance(CDc *cd, int vix)
        {
            t_cdesc = cd;
            t_vecix = vix;
        }

    // cd_terminal.cc
    CDnetName master_name() const;
    CDsterm *master_term() const;
    int master_group() const;

private:
    static CDnetName undef_name();

    CDp_cnode *t_node;      // Pointer to associated node property.
    CDc *t_cdesc;           // Electrical instance if instance terminal.
    int t_vecix;            // Vector index if t_cdesc is vectorized.
};


// List element for terminals.
//
template <class T>
struct CDtlist
{
    CDtlist()
        {
            tl_next = 0;
            tl_term = 0;
        }

    CDtlist(T *t, CDtlist<T> *n)
        {
            tl_next = n;
            tl_term = t;
        }

    static void destroy(const CDtlist *t)
        {
            while (t) {
                const CDtlist*x = t;
                t = t->tl_next;;
                delete x;
            }
        }

    CDtlist<T> *next()              { return (tl_next); }
    void set_next(CDtlist<T> *n)    { tl_next = n; }

    T *term()                       { return (tl_term); }
    void set_term(T *t)             { tl_term = t; }

private:
    CDtlist<T>  *tl_next;
    T           *tl_term;
};

typedef CDtlist<CDsterm> CDpin;
typedef CDtlist<CDcterm> CDcont;

#endif


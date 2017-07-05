
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
 $Id: ext_nets.h,v 5.18 2017/04/18 03:13:59 stevew Exp $
 *========================================================================*/

#ifndef EXT_NETS_H
#define EXT_NETS_H


#include "cd_terminal.h"

// Electrical terminal list, returned from cExt::GetElecTermlist()
// function.
//
struct sElecNetList
    {
    // Array element for an electrical net.
    struct sEnode
    {
        sEnode()
            {
                pins = 0;
                conts = 0;
                name = 0;
                group = -1;
            }

        CDpin *pins;        // cell connections
        CDcont *conts;      // device/subckt connections
        CDnetName name;     // name of this net
        int group;          // corresponding physical group
    };

    // List element for master copies of flattened subcircuits.  This
    // is a variable-sized srtruct to accommodate vectored instances
    // of arbitrary size.
    //
    struct sdlink
    {
        // DON'T use 'new' for this!

        static sdlink *alloc(const CDc *cd, sdlink *n, unsigned int sz)
            {
                if (sz != 0)
                    sz--;
                int size = sizeof(sdlink) + sz*sizeof(CDs*);
                void *mem = calloc(1, size);
                sdlink *sl = (sdlink*)mem;
                sl->cdesc = cd;
                sl->next = n;
                sl->width = sz + 1;
                return (sl);
            }

        ~sdlink()
            {
                for (unsigned int i = 0; i < width; i++)
                    delete sdcopy[i];
            }

        static void destroy(sdlink *s)
            {
                while (s) {
                    sdlink *x = s;
                    s = s->next;
                    ::free(x);
                }
            }

        sdlink          *next;
        const CDc       *cdesc;     // Original subcircuit instance.
        unsigned int    width;      // Vector width.
        CDs             *sdcopy[1]; // Copies of master.
    };

    sElecNetList(CDs*);
    ~sElecNetList();

    CDnetName net_name(int node)
        {
            if (node >= 0 && node <= (int)et_maxix)
                return (et_list[node].name);
            return (0);
        }

    int find_node(CDnetName n)
        {
            if (n) {
                for (unsigned int i = 0; i <= et_maxix; i++) {
                    if (et_list[i].name == n)
                        return (i);
                }
            }
            return (-1);
        }

    void set_group(int n, int g)
        {
            if (et_list && n >= 0 && n < (int)et_size)
                et_list[n].group = g;
        }

    int group_of_node(int n) const
        {
            if (et_list && n >= 0 && n <= (int)et_maxix)
                return (et_list[n].group);
            return (-1);
        }

    CDpin *pins_of_node(int n) const
        {
            if (et_list && n >= 0 && n <= (int)et_maxix)
                return (et_list[n].pins);
            return (0);
        }

    CDcont *conts_of_node(int n) const
        {
            if (et_list && n >= 0 && n <= (int)et_maxix)
                return (et_list[n].conts);
            return (0);
        }

    bool node_active(int n) const
        {
            if (et_list && n >= 0 && n <= (int)et_maxix)
                return (et_list[n].conts || et_list[n].pins);
            return (false);
        }

    int count_active() const
        {
            int n = 0;
            if (et_list) {
                for (unsigned int i = 1; i <= et_maxix; i++) {
                    if (et_list[i].conts || et_list[i].pins)
                        n++;
                }
            }
            return (n);
        }

    bool remove(const CDterm *term, int n)
        {
            if (term) {
                if (term->instance()) {
                    CDcont *tl = conts_of_node(n);
                    CDcont *tp = 0;
                    for ( ; tl; tl = tl->next()) {
                        if (tl->term() == term) {
                            if (tp)
                                tp->set_next(tl->next());
                            else
                                et_list[n].conts = tl->next();
                            delete tl;
                            return (true);
                        }
                        tp = tl;
                    }
                }
                else {
                    CDpin *tl = pins_of_node(n);
                    CDpin *tp = 0;
                    for ( ; tl; tl = tl->next()) {
                        if (tl->term() == term) {
                            if (tp)
                                tp->set_next(tl->next());
                            else
                                et_list[n].pins = tl->next();
                            delete tl;
                            return (true);
                        }
                        tp = tl;
                    }
                }
            }
            return (false);
        }

    int size()              const { return (et_maxix + 1); }

    static bool should_flatten(const CDc*, CDs*);
    static int count_sections(const CDc*, CDs*);

    void update_pins();
    bool remove(const CDc*, unsigned int);
    bool is_permutable(unsigned int, unsigned int);
    void check_flatten(CDc*);
    bool flatten(const CDc*, unsigned int, sEinstList** = 0, sEinstList** = 0);
    bool flattened(const CDc*, unsigned int);
    void list_devs_and_subs(sEinstList**, sEinstList**);
    void purge_terminals(SymTab*, SymTab*);
    void dump(FILE*);

private:
    CDs             *et_sdesc;      // Top level database elec. cell.
    sEnode          *et_list;       // Terminal list array indexed by node.
    sdlink          *et_fcells;     // List of cell copies of masters.
    SymTab          *et_ftab;       // Flattened instance tab.
    unsigned int    et_maxix;       // Largest array element used.
    unsigned int    et_size;        // Array size allocated.
};


//
// cGroupDesc deferred inline definitions.
//
#ifdef EXT_EXTRACT_H

inline bool
cGroupDesc::node_active(int n)
{
    return (gd_etlist ? gd_etlist->node_active(n) : false);
}


inline int
cGroupDesc::group_of_node(int n)
{
    return (gd_etlist ? gd_etlist->group_of_node(n) : -1);
}


inline CDpin *
cGroupDesc::pins_of_node(int n)
{
    return (gd_etlist ? gd_etlist->pins_of_node(n) : 0);
}


inline CDcont *
cGroupDesc::conts_of_node(int n)
{
    return (gd_etlist ? gd_etlist->conts_of_node(n) : 0);
}

#endif

#endif


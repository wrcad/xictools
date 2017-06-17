
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
 $Id: fio_nametab.h,v 5.21 2013/09/10 06:53:04 stevew Exp $
 *========================================================================*/

#ifndef FIO_NAMETAB_H
#define FIO_NAMETAB_H

#include "fio_cxfact.h"


//-----------------------------------------------------------------------------
// nametab_t
// This is a hash table used in cCHD and in the archive read
// functions.  The elements are symref_t that are allocated and
// deallocated through cx_fact_t.
//-----------------------------------------------------------------------------

// Table element for numtab_t.
//
struct nt_t
{
    friend struct numtab_t;

    unsigned long tab_key()     { return (num); }
    void set_tab_next(nt_t *n)  { next = n; }
    nt_t *tab_next()            { return (next); }

private:
    unsigned long num;
    nt_t *next;
    symref_t *symref;
};

struct nametab_t;

// CIF symbol number hash table.
//
struct numtab_t
{
    numtab_t() { nt_table = new itable_t<nt_t>; }
    inline numtab_t(nametab_t*);

    ~numtab_t() { delete nt_table; }

    symref_t *get(int k)
        {
            nt_t *n = nt_table->find(k);
            return (n ? n->symref : 0);
        }

    void add(symref_t *p)
        {
            int num = p->get_num();
            nt_t *e = nt_table->find(num);
            if (!e) {
                e = nt_elts.new_element();
                e->num = num;
                e->next = 0;
                e->symref = p;
                nt_table->link(e, false);
                nt_table = nt_table->check_rehash();
            }
        }

private:
    itable_t<nt_t> *nt_table;
    eltab_t<nt_t>   nt_elts;
};


// The names in the nametab (symref_t names) are all hashed in the
// string table, so address comparisons are used for equality testing.

struct nametab_t : public cxfact_t
{
    friend struct namegen_t;

    nametab_t(DisplayMode m)
        {
            nm_table = new itable_t<symref_t>;
            nm_elec_mode = (m == Electrical);
        }
    ~nametab_t() { delete nm_table; }

    void add(symref_t *srf)
        {
            if (!srf)
                return;
            nm_table->link(srf, false);
            nm_table = nm_table->check_rehash();
        }

    symref_t *get(CDcellName name)
        {
            if (name)
                return (nm_table->find((unsigned long)name));
            return (0);
        }

    symref_t *unlink(symref_t *srf)
        {
            return (nm_table->unlink(srf));
        }

    size_t memuse()
        {
            size_t base = sizeof(this) + sizeof(itable_t<symref_t>) +
                (nm_table->hashwidth() - 1)*sizeof(symref_t*);
            base += data_memuse();
            return (base);
        }

private:
    itable_t<symref_t> *nm_table;

    bool nm_elec_mode;
        // Set when this table references Electrical cells.
};


struct namegen_t : tgen_t<symref_t>
{
    namegen_t(nametab_t *st) : tgen_t<symref_t>(st->nm_table) { }
};


inline
numtab_t::numtab_t(nametab_t *nametab)
{
    nt_table = new itable_t<nt_t>;

    namegen_t ngen(nametab);
    symref_t *sref;
    while ((sref = ngen.next()) != 0) {
        nt_t *n = nt_elts.new_element();
        n->num = sref->get_num();
        n->symref = sref;
        n->set_tab_next(0);
        nt_table->link(n, false);
        nt_table = nt_table->check_rehash();
    }
}

#endif


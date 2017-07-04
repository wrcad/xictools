
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
 $Id: fio_chd_srctab.h,v 5.4 2013/01/13 04:44:04 stevew Exp $
 *========================================================================*/

#ifndef FIO_CHD_SRCTAB_H
#define FIO_CHD_SRCTAB_H

#include "cd_propnum.h"

//
// Special-purpose has table keyed by sChdPrp structs.  This allows
// hashing and listing of cell hierarchies for CHD reference cells.
// After adding the cells, the cells are listed under each unique CHD
// that contains the cells (no CHD is actually created/used).
//

// This is the table element for a unique CHD.
//
struct chd_src_t
{
    // Note that prp ownership changes.
    chd_src_t(const char *cname, sChdPrp *prp)
        {
            cs_seen_tab = 0;
            cs_cellnames = new stringlist(lstring::copy(cname), 0);
            cs_chd_prp = prp;
            cs_next = 0;
        }

    ~chd_src_t()
        {
            delete cs_seen_tab;
            stringlist::destroy(cs_cellnames);
            delete cs_chd_prp;
        }

    static void destroy(chd_src_t *c)
        {
            while (c) {
                chd_src_t *cx = c;
                c = c->cs_next;
                delete cx;
            }
        }

    SymTab *cs_seen_tab;        // caller uses this
    stringlist *cs_cellnames;   // list of cell names from this CHD
    sChdPrp *cs_chd_prp;        // unique reference property string
    chd_src_t *cs_next;
};


// The table, hashes the chd_src_t elements.
//
struct chd_src_tab
{
    friend struct chd_src_tab_gen;

    chd_src_tab()
        {
            array = new chd_src_t*[1];
            array[0] = 0;
            count = 0;
            hashmask = 0;
        }

    ~chd_src_tab()
        {
            for (unsigned int i = 0; i <= hashmask; i++)
                chd_src_t::destroy(array[i]);
            delete [] array;
        }

    // Add a cell name and corresponding sChdPrp, note that prp is
    // either taken in or deleted.
    //
    void add(const char *name, sChdPrp *prp)
        {
            unsigned int i = prp->hash(hashmask);
            for (chd_src_t *e = array[i]; e; e = e->cs_next) {
                if ((*prp) == (*e->cs_chd_prp)) {
                    // Matches an existing element, link cell name.
                    e->cs_cellnames =
                        new stringlist(lstring::copy(name), e->cs_cellnames);
                    delete prp;
                    return;
                }
            }

            // Need to create a new element.
            chd_src_t *enew = new chd_src_t(name, prp);
            enew->cs_next = array[i];
            array[i] = enew;
            count++;
            check_rehash();
        }

private:
    void check_rehash()
        {
            if (count/(hashmask+1) <= ST_MAX_DENS)
                return;

            unsigned int newmask = (hashmask << 1) | 1;
            chd_src_t **st = new chd_src_t*[newmask+1];
            for (unsigned int i = 0; i <= newmask; i++)
                st[i] = 0;
            for (unsigned int i = 0; i <= hashmask; i++) {
                chd_src_t *en;
                for (chd_src_t *e = array[i]; e; e = en) {
                    en = e->cs_next;
                    unsigned int j = e->cs_chd_prp->hash(newmask);
                    e->cs_next = st[j];
                    st[j] = e;
                }
                array[i] = 0;
            }
            delete [] array;
            array = st;
            hashmask = newmask;
        }

    chd_src_t **array;
    unsigned int count;
    unsigned int hashmask;
};

// Generator for traversing the table.
//
struct chd_src_tab_gen
{
    chd_src_tab_gen(chd_src_tab *tab)
        {
            array = tab ? tab->array : 0;
            elt = tab ? array[0] : 0;
            hashmask = tab ? tab->hashmask : 0;
            indx = 0;
        }

    // Return each element, 0 when done.
    //
    chd_src_t *next()
        {
            for (;;) {
                if (!elt) {
                    if (!array)
                        return (0);
                    indx++;
                    if (indx > hashmask)
                        return (0);
                    elt = array[indx];
                    continue;
                }
                chd_src_t *e = elt;
                elt = elt->cs_next;
                return (e);
            }
        }

private:
    chd_src_t **array;
    chd_src_t *elt;
    unsigned int hashmask;
    unsigned int indx;
};

#endif


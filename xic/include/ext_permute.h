
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
 $Id: ext_permute.h,v 5.6 2015/06/11 05:54:06 stevew Exp $
 *========================================================================*/

#ifndef EXT_PERMUTE_H
#define EXT_PERMUTE_H

//
// Connection permutation handling.
//

// The base class provides static tables of permutations for 2-4 signals,
// which is the limit of support presently.
//
struct sExtPermGrpB
{
protected:
    static char p2[2][2];
    static char p3[6][3];
    static char p4[24][4];
};

// The template class allows permuting of arbitrary object types. 
// This is a linked list element, as a subcell may have several sets
// of permutable connections.
//
template <class T>
struct sExtPermGrp : private sExtPermGrpB
{
    sExtPermGrp()
        {
            pg_objects = 0;
            pg_order = 0;
            pg_next = 0;
            pg_num = 0;
            pg_state = 0;
        }

    ~sExtPermGrp()
        {
            delete [] pg_order;
        }

    static void destroy(sExtPermGrp *pg)
        {
            while (pg) {
                sExtPermGrp *px = pg;
                pg = pg->next();
                delete px;
            }
        }

    // Set the reference object table.  This contains the objects in
    // the reference (state = 0) order.  We only store an address here
    // NOT a copy, so the array MUST NOT change during the lifetime
    // of this object.
    //
    bool set(T *vals, unsigned int sz)
        {
            if (sz < 2 || sz > 4)
                return (false);
            pg_objects = vals;
            pg_num = sz;
            pg_state = 0;
            return (true);
        }

    // Return the address of a previously set object.
    //
    T *get(unsigned int i)
        {
            if (i >= pg_num)
                return (0);
            if (!pg_objects)
                return (0);
            return (&pg_objects[i]);
        }

    // Set the target address table elememt.  These are addresses that
    // receive pointers to the permuted objects.  For state = 0,
    // *pg_order[i] = pg_object[i] for all i.  This initialization is
    // NOT done here.
    //
    bool set_target(T *val, unsigned int i)
        {
            if (i >= pg_num)
                return (false);
            if (!pg_order) {
                pg_order = new T*[pg_num];
                memset(pg_order, 0, pg_num*sizeof(T*));
            }
            pg_order[i] = val;
            return (true);
        }

    unsigned int num_states() const
        {
            unsigned ns = 1;
            switch (pg_num) {
                case 2: ns = 2;
                case 3: ns = 6;
                case 4: ns = 24;
                default: break;
            }
            if (pg_next)
                ns *= pg_next->num_states();
            return (ns);
        }

    unsigned int state() const
        {
            unsigned int s = pg_state;
            if (pg_next) {
                unsigned int xs = pg_next->num_states();
                if (xs > 1)
                    s = s*xs + pg_next->state();
            }
            return (s);
        }

    // Set the target to the given permutation state.
    //
    bool set_state_of_this(unsigned int s)
        {
            if (pg_num == 2) {
                if (s >= 2)
                    return (false);
                pg_state = s;
                for (int i = 0; i < 2; i++) {
                    if (pg_order[i])
                        *pg_order[i] = pg_objects[(int)p2[pg_state][i]];
                }
                return (true);
            }
            if (pg_num == 3) {
                if (s >= 6)
                    return (false);
                pg_state = s;
                for (int i = 0; i < 3; i++) {
                    if (pg_order[i])
                        *pg_order[i] = pg_objects[(int)p3[pg_state][i]];
                }
                return (true);
            }
            if (pg_num == 4) {
                if (s >= 24)
                    return (false);
                pg_state = s;
                for (int i = 0; i < 4; i++) {
                    if (pg_order[i])
                        *pg_order[i] = pg_objects[(int)p4[pg_state][i]];
                }
                return (true);
            }
            return (false);
        }

    bool set_state(unsigned int s)
        {
            if (s >= num_states())
                return (false);
            if (pg_next) {
                unsigned int xs = pg_next ? pg_next->num_states() : 1;
                unsigned int r = s/xs;
                if (!pg_next->set_state(s - r*xs))
                    return (false);
                return (set_state_of_this(r));
            }
            return (set_state_of_this(s));
        }

    unsigned int num_permutes()     const { return (pg_num); }
    sExtPermGrp<T> *next()          const { return (pg_next); }
    void set_next(sExtPermGrp *p)         { pg_next = p; }

    // This does permutation and updates the target.
    //
    void permute()
        {
            pg_state++;
            if (pg_num == 2) {
                if (pg_state == 2)
                    pg_state = 0;
                if (pg_order) {
                    for (int i = 0; i < 2; i++) {
                        if (pg_order[i])
                            *pg_order[i] = pg_objects[(int)p2[pg_state][i]];
                    }
                }
            }
            else if (pg_num == 3) {
                if (pg_state == 6)
                    pg_state = 0;
                if (pg_order) {
                    for (int i = 0; i < 3; i++) {
                        if (pg_order[i])
                            *pg_order[i] = pg_objects[(int)p3[pg_state][i]];
                    }
                }
            }
            else if (pg_num == 4) {
                if (pg_state == 24)
                    pg_state = 0;
                if (pg_order) {
                    for (int i = 0; i < 4; i++) {
                        if (pg_order[i])
                            *pg_order[i] = pg_objects[(int)p4[pg_state][i]];
                    }
                }
            }
        }

    // This puts back the orginal ordering (same as set_state(0) on
    // each component).
    //
    void reset()
        {
            if (pg_order) {
                for (unsigned int i = 0; i < pg_num; i++) {
                    if (pg_order[i])
                        *pg_order[i] = pg_objects[i];
                }
            }
            if (pg_next)
                pg_next->reset();
        }

    // Return true if the argument references a permutable object.
    //
    bool is_permute(const T &o) const
        {
            if (pg_objects) {
                for (unsigned int i = 0; i < pg_num; i++) {
                    if (pg_objects[i] == o)
                        return (true);
                }
            }
            if (pg_next)
                return (pg_next->is_permute(o));
            return (false);
        }

    // Return true if the two arguments are equal, or from the same
    // permutation group.
    //
    bool is_equiv(const T &o1, const T &o2) const
        {
            if (o1 == o2)
                return (true);
            if (pg_objects) {
                int f = 0;
                for (unsigned int i = 0; i < pg_num; i++) {
                    if (pg_objects[i] == o1 || pg_objects[i] == o2) {
                        if (++f == 2)
                            return (true);
                    }
                }
            }
            if (pg_next)
                return (pg_next->is_equiv(o1, o2));
            return (false);
        }

private:
    T               *pg_objects;    // Reference objects and order.
    T               **pg_order;     // Permutation target addresses.
    sExtPermGrp     *pg_next;
    unsigned int    pg_num;         // Size of arrays (2,3, or 4).
    unsigned int    pg_state;       // Permutation step: 0 to num!-1.
};


// Generator, iterates through all of the permutations.  Note that it
// loops once if there are no permutations.
//
template <class T>
struct sExtPermGen
{
    sExtPermGen(sExtPermGrp<T> *pgrp)
        {
            g_next = pgrp && pgrp->next() ? new sExtPermGen(pgrp->next()) : 0;
            g_perms = pgrp;
            g_end = pgrp ? pgrp->state() : 0;
            g_first = true;
        }

    void reset()
        {
            if (g_next)
                g_next->reset();
            g_first = true;
        }

    bool next()
        {
            if (g_next) {
                if (g_next->next())
                    return (true);
            }
            if (g_first) {
                g_first = false;
                if (g_next)
                    g_next->reset();
                return (true);
            }
            if (!g_perms)
                return (false);
            g_perms->permute();
            if (g_perms->state() == g_end)
                return (false);
            if (g_next)
                g_next->reset();
            return (true);
        }

private:
    sExtPermGen     *g_next;
    sExtPermGrp<T>  *g_perms;
    unsigned int    g_end;
    bool            g_first;
};


// Types of permutation groups.
enum PGtype { PGtopo, PGnand, PGnor };

// Linked list element for a permutation group of group numbers.  We
// support group sizes of 2-4.  Returned from cGroupDesc::check_permutes.
//
struct sPermGrpList
{
    sPermGrpList(int *v, unsigned int s, PGtype t, sPermGrpList *n)
        {
            if (s >= 4)
                s = 4;
            pg_next = n;
            memcpy(pg_group, v, s*sizeof(int));
            pg_size = s;
            pg_type = t;
        }

    static void destroy(sPermGrpList *p)
        {
            while (p) {
                sPermGrpList *px = p;
                p = p->pg_next;
                delete px;
            }
        }

    int group(unsigned int i) const
        {
            if (i >= pg_size)
                return (-1);
            return (pg_group[i]);
        }

    sPermGrpList *next()    const { return (pg_next); }
    unsigned int size()     const { return (pg_size); }
    PGtype type()           const { return (pg_type); }

private:
    sPermGrpList    *pg_next;
    int             pg_group[4];
    unsigned int    pg_size;
    PGtype          pg_type;
};

#endif


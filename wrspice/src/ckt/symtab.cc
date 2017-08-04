
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
 * WRspice Circuit Simulation and Analysis Tool                           *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3e2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1987 Thomas L. Quarles
         1992 Stephen R. Whiteley
****************************************************************************/

#include "device.h"
#include "errors.h"
#include "misc.h"
#include "hash.h"
#include "frontend.h"


//
// This contains code for the symbol tables, of which there are two: 
// one for terminal names (in the CKTnodeTab struct), and one for
// other names.  The second table resides in the sFtCirc so will be
// common to all sCKTs.
//

int
sCKT::insert(char **name) const
{
    if (CKTbackPtr && CKTbackPtr->symtab())
        return (CKTbackPtr->symtab()->insert(name));
    return (E_NOTFOUND);
}


// Allocate or return a UID.  The symbol table used, and the return
// arguments, depend on the type.  UIDs can not be destroyed, except
// by destroying the tables (when the circuit struct is deleted).
//
int
sCKT::newUid(IFuid *newuid, IFuid olduid, const char *suffix, UID_TYPE)
{
    // Note that the UID_TYPE is unused.
    if (!suffix)
        return (E_BADPARM);
    char *newname;
    if (olduid) {
        newname = new char[strlen(suffix) + strlen((char*)olduid) + 2];
        sprintf(newname, "%s#%s", (char*)olduid, suffix);
    }
    else {
        newname = new char[strlen(suffix) + 1];
        sprintf(newname, "%s", suffix);
    }

    int error = insert(&newname);
    if (error && error != E_EXISTS)
        return (error);
    *newuid = (IFuid) newname;
    return (OK);
}


int
sCKT::newTerm(IFuid *newuid, IFuid olduid, const char *suffix,
    sCKTnode **nodedata)
{
    if (!suffix)
        return (E_BADPARM);
    char *newname;
    if (olduid) {
        newname = new char[strlen(suffix) + strlen((char*)olduid) + 2];
        sprintf(newname, "%s#%s", (char*)olduid, suffix);
    }
    else {
        newname = new char[strlen(suffix) + 1];
        sprintf(newname, "%s", suffix);
    }

    int error = mkTerm(&newname, nodedata);
    if (error && error != E_EXISTS)
        return (error);
    *newuid = (IFuid) newname;
    return (error);
}


// Make the given name a 'node' of type voltage in the specified
// circuit.
//
int
sCKT::mkVolt(sCKTnode **node, IFuid basename, const char *suffix)
{
    sCKTnode *mynode = CKTnodeTab.newNode(SP_VOLTAGE);
    sCKTnode *checknode = mynode;
    IFuid uid;
    int error = newTerm(&uid, basename, suffix, &checknode);
    if (error) {
        CKTnodeTab.deallocateLast();
        if (node)
            *node = checknode;
        return (error);
    }
    mynode->set_name(uid);
    if (node)
        *node = mynode;
    return (OK);
}


// Make the given name a 'node' of type current in the specified
// circuit.
//
int
sCKT::mkCur(sCKTnode **node, IFuid basename, const char *suffix)
{
    sCKTnode *mynode = CKTnodeTab.newNode(SP_CURRENT);
    sCKTnode *checknode = mynode;
    IFuid uid;
    int error = newTerm(&uid, basename, suffix, &checknode);
    if (error) {
        CKTnodeTab.deallocateLast();
        if (node)
            *node = checknode;
        return (error);
    }
    mynode->set_name(uid);
    if (node)
        *node = mynode;
    return (OK);
}
// End of sCKT functions.


// A string hashing function (Bernstein, comp.lang.c).
//
template <class T>
unsigned int
sTab<T>::hash(const char *str) const
{
    if (!t_hashmask || !str)
        return (0);
    unsigned int hval = 5381;
    if (t_ci) {
        for ( ; *str; str++) {
            unsigned char c = isupper(*str) ? tolower(*str) : *str;
            hval = ((hval << 5) + hval) ^ c;
        }
    }
    else {
        for ( ; *str; str++)
            hval = ((hval << 5) + hval) ^ *(unsigned char*)str;
    }
    return (hval & t_hashmask);
}


// String comparison function.
//
template <class T>
int
sTab<T>::comp(const char *s, const char *t) const
{
    if (t_ci) {
        char c1 = isupper(*s) ? tolower(*s) : *s;
        char c2 = isupper(*t) ? tolower(*t) : *t;
        while (c1 && c1 == c2) {
            s++;
            t++;
            c1 = isupper(*s) ? tolower(*s) : *s;
            c2 = isupper(*t) ? tolower(*t) : *t;
        }
        return (c1 - c2);
    }
    else {
        while (*s && *s == *t) {
            s++;
            t++;
        }
        return (*s - *t);
    }
}


template <class T>
T *
sTab<T>::find(const char *name) const
{
    if (name == 0)
        return (0);
    unsigned int n = hash(name);
    for (T *t = t_tab[n]; t; t = t->t_next) {
        int i = comp(name, t->t_ent);
        if (i < 0)
            continue;
        if (i == 0)
            return (t);
        break;
    }
    return (0);
}


template <class T>
void
sTab<T>::add(T *ent)
{
    unsigned int n = hash(ent->t_ent);
    if (t_tab[n] == 0) {
        t_tab[n] = ent;
        ent->t_next = 0;
        t_allocated++;
        check_rehash();
        return;
    }

    T *prv = 0;
    for (T *t = t_tab[n]; t; prv = t, t = t->t_next) {
        int i = comp(ent->t_ent, t->t_ent);
        if (i < 0)
            continue;
        ent->t_next = t;
        if (prv)
            prv->t_next = ent;
        else
            t_tab[n] = ent;
        t_allocated++;
        check_rehash();
        return;
    }
    prv->t_next = ent;
    t_allocated++;
    check_rehash();
}


template <class T>
void
sTab<T>::remove(const char *name)
{
    if (name == 0)
        return;
    unsigned int n = hash(name);
    T *t = t_tab[n];
    for (T *tprv = 0; t; tprv = t, t = t->t_next) {
        int i = comp(name, t->t_ent);
        if (i < 0)
            continue;
        if (i == 0) {
            if (tprv)
                tprv->t_next = t->t_next;
            else
                t_tab[n] = t->t_next;
            delete t;
            t_allocated--;
            return;
        }
        break;
    }
}


#define STAB_MAX_DENS   5

template <class T>
void
sTab<T>::check_rehash()
{
    if (t_allocated/(t_hashmask + 1) > STAB_MAX_DENS) {
        unsigned int oldmask = t_hashmask;
        t_hashmask = (oldmask << 1) | 1;
        T **oldent = t_tab;
        t_tab = new T*[t_hashmask + 1];
        for (unsigned int i = 0; i <= t_hashmask; i++)
            t_tab[i] = 0;
        for (unsigned int i = 0; i <= oldmask; i++) {
            T *tn;
            for (T *t = oldent[i]; t; t = tn) {
                tn = t->t_next;
                t->t_next = 0;
                unsigned int n = hash(t->t_ent);

                T *tx = t_tab[n];
                if (tx == 0) {
                    t_tab[n] = t;
                    continue;
                }

                T *tprv = 0;
                for ( ; tx; tprv = tx, tx = tx->t_next) {
                    if (comp(t->t_ent, tx->t_ent) <= 0)
                        continue;
                    if (tprv) {
                        tprv->t_next = t;
                        tprv = tprv->t_next;
                    }
                    else {
                        tprv = t;
                        t_tab[n] = tprv;
                    }
                    tprv->t_next = tx;
                    tprv = 0;
                    break;
                }
                if (tprv)
                    tprv->t_next = t;
            }
        }
        delete [] oldent;
    }
}
// End of sTab<> functions.


int
sSymTab::insert(char **token)
{
    if (!token || !*token)
        return (E_BADPARM);

    sEnt *t = sym_tab.find(*token);
    if (t) {
        delete [] *token;
        *token = t->t_ent;
        return (E_EXISTS);
    }

    t = new sEnt;
    t->t_ent = lstring::copy(*token);
    delete [] *token;
    *token = t->t_ent;

    sym_tab.add(t);
    return (OK);
}
// End of sSymTab functions.


sCKTnodeTab::sCKTnodeTab()
{
    nt_ary = 0;
    nt_count = 0;
    nt_size = 0;
    nt_term_tab = 0;
}


sCKTnodeTab::~sCKTnodeTab()
{
    for (unsigned int i = 0; i < nt_size; i++)
        delete [] nt_ary[i];
    delete [] nt_ary;
    delete nt_term_tab;
}


// Call this to (re)initialize the factory.
//
void
sCKTnodeTab::reset()
{
    for (unsigned int i = 0; i < nt_size; i++)
        delete [] nt_ary[i];
    delete [] nt_ary;

    delete nt_term_tab;
    nt_term_tab = 0;

    nt_size = CKT_NT_INITSZ;
    nt_ary = new sCKTnode*[nt_size];
    memset(nt_ary, 0, nt_size*sizeof(sCKTnode*));

    nt_ary[0] = new sCKTnode[CKT_NT_MASK + 1];
    nt_count = 1;  // The ground node is pre-allocated.
    memset(nt_ary[0], 0, sizeof(sCKTnode));  // Clear ground node.
}


// Return a fresh sCKT node.  All fields are zeroed except the number
// field is preset.
//
sCKTnode *
sCKTnodeTab::newNode(unsigned int tp)
{
    unsigned int bank = nt_count >> CKT_NT_SHIFT;
    if (bank == nt_size) {
        sCKTnode **tmp = new sCKTnode*[nt_size + nt_size];
        memcpy(tmp, nt_ary, nt_size*sizeof(sCKTnode*));
        memset(tmp + nt_size, 0, nt_size*sizeof(sCKTnode*));
        delete [] nt_ary;
        nt_ary = tmp;
        nt_size += nt_size;
    }
    if (!nt_ary[bank])
        nt_ary[bank] = new sCKTnode[CKT_NT_MASK + 1];
    sCKTnode *n = nt_ary[bank] + (nt_count & CKT_NT_MASK);
    memset(n, 0, sizeof(sCKTnode));
    n->nd_type = tp;
    n->nd_number = nt_count++;
    return (n);
}


// Search for a node by number.  This is easy.
//
sCKTnode *
sCKTnodeTab::find(unsigned int num) const
{
    if (num >= nt_count)
        return (0);
    unsigned int offs = num & CKT_NT_MASK;
    unsigned int bank = num >> CKT_NT_SHIFT;
    return (nt_ary[bank] + offs);
}


int
sCKTnodeTab::term_insert(char **token, sCKTnode **node)
{
    if (!token || !*token)
        return (E_BADPARM);

    if (!nt_term_tab)
        nt_term_tab = new sTab<sNEnt>(sHtab::get_ciflag(CSE_NODE));

    sNEnt *t = nt_term_tab->find(*token);
    if (t) {
        delete [] *token;
        *token = t->t_ent;
        if (node)
            *node = t->t_node;
        return (E_EXISTS);
    }

    t = new sNEnt;
    t->t_ent = lstring::copy(*token);
    delete [] *token;
    *token = t->t_ent;

    t->t_node = newNode(SP_VOLTAGE);
    t->t_node->nd_name = t->t_ent;
    if (node)
        *node = t->t_node;

    nt_term_tab->add(t);
    return (OK);
}


int
sCKTnodeTab::mk_term(char **token, sCKTnode **node)
{
    if (!token || !*token)
        return (E_BADPARM);

    if (!nt_term_tab)
        nt_term_tab = new sTab<sNEnt>(sHtab::get_ciflag(CSE_NODE));

    sNEnt *t = nt_term_tab->find(*token);
    if (t) {
        delete [] *token;
        *token = t->t_ent;
        if (node)
            *node = t->t_node;
        return (E_EXISTS);
    }

    t = new sNEnt;
    t->t_ent = lstring::copy(*token);
    delete [] *token;
    *token = t->t_ent;

    t->t_node = *node;

    nt_term_tab->add(t);
    return (OK);
}


int
sCKTnodeTab::find_term(char **token, sCKTnode **node)
{
    if (node)
        *node = 0;
    if (token && *token && nt_term_tab) {
        sNEnt *t = nt_term_tab->find(*token);
        if (t) {
            delete [] *token;
            *token = t->t_ent;
            if (node)
                *node = t->t_node;
            return (OK);
        }
    }
    return (E_NOTFOUND);
}


int
sCKTnodeTab::gnd_insert(char **token, sCKTnode **node)
{
    if (!token || !*token)
        return (E_BADPARM);

    if (!nt_term_tab)
        nt_term_tab = new sTab<sNEnt>(sHtab::get_ciflag(CSE_NODE));

    sNEnt *t = nt_term_tab->find(*token);
    if (t) {
        delete [] *token;
        *token = t->t_ent;
        if (node)
            *node = t->t_node;
        return (E_EXISTS);
    }

    t = new sNEnt;
    t->t_ent = lstring::copy(*token);
    delete [] *token;
    *token = t->t_ent;

    t->t_node = find(0);
    if (t->t_node->nd_name)
        return (E_EXISTS);
    t->t_node->nd_name = t->t_ent;
    t->t_node->nd_type = SP_VOLTAGE;
    if (node)
        *node = t->t_node;

    nt_term_tab->add(t);
    return (OK);
}


// Deallocate all nodes with numbers larger than num, and remove and
// destroy the corresponding terminals.
//
void
sCKTnodeTab::dealloc(unsigned int num)
{
    sCKTnode *n = find(num + 1);
    unsigned int cnt = 0;
    for ( ; n; n = nextNode(n)) {
        if (nt_term_tab)
            nt_term_tab->remove((const char*)n->name());
        cnt++;
    }
    nt_count -= cnt;
}
// End of sCKTnodeTab functions.


// Ask about a parameter on a node.
//
int
sCKTnode::ask(int parm, IFvalue *value) const
{
    switch (parm) {
    case PARM_NS:
        value->rValue = nd_nodeset;
        break;
    case PARM_IC:
        value->rValue = nd_ic;
        break;
    case PARM_NODETYPE:
        value->iValue = nd_type;
        break;
    default:
        return (E_BADPARM);
    }
    return (OK);
}


// Set a parameter on a node.
//
int
sCKTnode::set(int parm, IFdata *data)
{
    switch (parm) {
    case PARM_NS:
        if ((data->type & IF_VARTYPES) == IF_REAL)
            nd_nodeset = data->v.rValue;
        else if ((data->type & IF_VARTYPES) == IF_INTEGER)
            nd_nodeset = (double)data->v.iValue;
        else
            return (E_BADPARM);
        nd_nsGiven = 1;
        break;
    case PARM_IC:
        if ((data->type & IF_VARTYPES) == IF_REAL)
            nd_ic = data->v.rValue;
        else if ((data->type & IF_VARTYPES) == IF_INTEGER)
            nd_ic = (double)data->v.iValue;
        else
            return (E_BADPARM);
        nd_icGiven = 1;
        break;
    case PARM_NODETYPE:
        if ((data->type & IF_VARTYPES) == IF_INTEGER)
            nd_type = data->v.iValue;
        else
            return (E_BADPARM);
        break;
    default:
        return (E_BADPARM);
    }
    return (OK);
}


// Bind a node of the specified device of the given type to its place
// in the specified circuit.
//
int
sCKTnode::bind(sGENinstance *fast, int term)
{
    if (!fast)
        return (E_NOTERM);
    int tp = fast->GENmodPtr->GENmodType;
    char key = *(const char*)fast->GENname;

    IFkeys *keys = DEV.device(tp)->keyMatch(key);
    if (!keys)
        return (E_NOTERM);

    if (keys->maxTerms >= term && term > 0) {
        int *ptr = fast->nodeptr(term);
        if (ptr)
            *ptr = nd_number;
        return (OK);
    }
    return (E_NOTERM);
}


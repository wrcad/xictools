
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
#include "stab.h"
#include "simulator.h"
#include "spnumber/hash.h"


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


// List the global ground names separated by space.  These are all
// equivalent and map to the internal ground node.  Each is taken as
// global.
//
const char *sCKTnodeTab::nt_ground_names = "0 gnd!";

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
    nt_term_tab->add(t);

    if (t->t_node->nd_name)
        return (E_EXISTS);
    t->t_node->nd_name = t->t_ent;
    t->t_node->nd_type = SP_VOLTAGE;
    if (node)
        *node = t->t_node;

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


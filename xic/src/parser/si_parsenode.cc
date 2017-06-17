
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2015 Whiteley Research Inc, all rights reserved.        *
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
 $Id: si_parsenode.cc,v 5.5 2017/05/01 17:12:05 stevew Exp $
 *========================================================================*/

#include "cd.h"
#include "cd_types.h"
#include "cd_sdb.h"
#include "si_parsenode.h"
#include "si_lexpr.h"
#include "si_parser.h"
#include "si_interp.h"
#include "timedbg.h"


//
// ParseNode functions.
//

ParseNode **ParseNode::pn_pnodes;
int ParseNode::pn_nodes;
int ParseNode::pn_size;


void
ParseNode::print(FILE *fp)
{
    sLstr lstr;
    string(lstr);
    fprintf(fp, "%s", lstr.string());
}


// Print the expression in lstr.  For layer expressions only:  if
// unique is true, reorder the symmetric binary op left and right
// strings alphabetically, so that the overall expression is uniquely
// presented.  We should be able to tell if two expressions are
// equivalent by comparing these strings.  Since TOK_POWER (XOR) is
// taken as symmetric, this is NOT good for general expressions.
//
void
ParseNode::string(sLstr &lstr, bool unique)
{
    if (!(void*)this)
        return;

    if (type == PT_VAR) {
        lstr.add(data.v->name);
        if (left) {
            lstr.add_c('[');
            left->string(lstr, unique);
            lstr.add_c(']');
        }
    }
    else if (type == PT_CONSTANT) {
        lstr.add_g(data.constant.value);
    }
    else if (type == PT_BINOP) {
        if (unique && (optype == TOK_PLUS || optype == TOK_TIMES ||
                optype == TOK_AND || optype == TOK_OR ||
                optype == TOK_POWER)) {

            sLstr ll;
            if (left->type == PT_CONSTANT || left->type == PT_VAR)
                left->string(ll, unique);
            else {
                ll.add_c('(');
                left->string(ll, unique);
                ll.add_c(')');
            }

            sLstr lr;
            if (right->type == PT_CONSTANT || right->type == PT_VAR)
                right->string(lr, unique);
            else {
                lr.add_c('(');
                right->string(lr, unique);
                lr.add_c(')');
            }
            if (strcmp(ll.string(), lr.string()) > 0) {
                lstr.add(lr.string());
                lstr.add(data.f.funcname);
                lstr.add(ll.string());
            }
            else {
                lstr.add(ll.string());
                lstr.add(data.f.funcname);
                lstr.add(lr.string());
            }
        }
        else {
            if (left->type == PT_CONSTANT || left->type == PT_VAR)
                left->string(lstr, unique);
            else {
                lstr.add_c('(');
                left->string(lstr, unique);
                lstr.add_c(')');
            }
            lstr.add(data.f.funcname);
            if (right->type == PT_CONSTANT || right->type == PT_VAR)
                right->string(lstr, unique);
            else {
                lstr.add_c('(');
                right->string(lstr, unique);
                lstr.add_c(')');
            }
        }
    }
    else if (type == PT_FUNCTION) {
        lstr.add(data.f.funcname);
        lstr.add_c('(');
        for (ParseNode *p = left; p; p = p->next) {
            p->string(lstr, unique);
            if (p->next)
                lstr.add_c(',');
        }
        lstr.add_c(')');
    }
    else if (type == PT_UNOP) {
        if (left->type == PT_CONSTANT || left->type == PT_VAR) {
            lstr.add(data.f.funcname);
            left->string(lstr, unique);
        }
        else {
            lstr.add(data.f.funcname);
            lstr.add_c('(');
            left->string(lstr, unique);
            lstr.add_c(')');
        }
    }
}


// Return the variable for a PT_VAR, takes care of resolving global
// variable references.
//
siVariable *
ParseNode::getVar()
{
    siVariable *v = 0;
    if (type == PT_VAR) {
        v = data.v;
        if (v && (v->flags & VF_GLOBAL)) {
            v = SIparse()->findGlobal(v->name);
            if (!v)
                SIparse()->pushError("-eglobal variable not found");
        }
    }
    return (v);
}


// Logical test function for conditionals.
//
bool
ParseNode::istrue(void *datap)
{
    if (!(void*)this)
        return (false);
    if (type == PT_VAR) {
        Variable *v = getVar();
        if (v)
            return (v->istrue());
        return (false);
    }
    if (type == PT_CONSTANT)
        return (to_boolean(data.constant.value));

    siVariable res;
    if ((*this->evfunc)(this, &res, datap) != OK) {
        if (!SI()->IsHalted())
            SI()->LineError("conditional execution error");
        return (false);
    }
    bool ret = res.istrue();
    res.gc_result();
    return (ret);
}


// Check for problems in the parse tree.
//
bool
ParseNode::check()
{
    if (!(void*)this)
        return (OK);

    if (type == PT_VAR || type == PT_CONSTANT)
        return (OK);
    if (type == PT_BINOP) {
        if (left->check() == BAD || right->check() == BAD)
            return (BAD);
        return (OK);
    }
    if (type == PT_FUNCTION) {
        for (ParseNode *arg = left; arg; arg = arg->next) {
            if (arg->check() == BAD)
                return (BAD);
        }
        return (OK);
    }
    if (type == PT_UNOP)
        return (left->check());

    SIparse()->pushError("-pbad node type %d", type);
    return (BAD);
}


void
ParseNode::free()
{
    if ((void*)this) {
        if (SIparse()->isSubFunc(this)) {
            SIfunc *sf = data.f.userfunc;
            if (sf)
                sf->sf_refcnt--;
        }
        left->free();
        right->free();
        next->free();
        delete this;
    }
}


// Static function.
// ParseNode allocation management.  This simply keeps track of the
// nodes allocated while parsing a function, so that nodes can be
// easily freed on error, before the nodes are linked.  After a
// successful parse, the nodes will be linked into a tree which can be
// cleared with ParseNode::free.
//
// Global clear/reset is taken care of in SIparser::getTree.
//
// P_ALLOC
// Allocate and return a new ParseNode, adding it to a list.  The list is
// created if necessary.
//
// P_CLEAR
// Free all nodes and delete the list.  This is called on parse error.
//
// P_RESET
// Free all PT_BOGUS nodes and delete the list.  This is called after
// a successful parse.  The PT_BOGUS nodes are allocated but unused in
// the tree.
//
#define INCR 20
//
ParseNode *
ParseNode::allocate_pnode(PallocMode mode)
{
    if (mode == P_ALLOC) {
        pn_nodes++;
        if (pn_nodes > pn_size) {
            pn_size += INCR;
            ParseNode **pn = new ParseNode*[pn_size];
            int i;
            for (i = 0; i < pn_size - INCR; i++)
                pn[i] = pn_pnodes[i];
            for ( ; i < pn_size; i++)
                pn[i] = 0;
            delete [] pn_pnodes;
            pn_pnodes = pn;
        }
        pn_pnodes[pn_nodes-1] = new ParseNode;
        return (pn_pnodes[pn_nodes-1]);
    }
    if (mode == P_CLEAR) {
        for (int i = 0; i < pn_nodes; i++) {
            if (pn_pnodes[i] && pn_pnodes[i]->type != PT_BOGUS) {
                delete pn_pnodes[i];
                pn_pnodes[i] = 0;
            }
        }
    }
    if (mode == P_CLEAR || mode == P_RESET) {
        for (int i = 0; i < pn_nodes; i++) {
            if (pn_pnodes[i] && pn_pnodes[i]->type == PT_BOGUS) {
                delete pn_pnodes[i];
                pn_pnodes[i] = 0;
            }
        }
    }
    delete [] pn_pnodes;
    pn_pnodes = 0;
    pn_size = 0;
    pn_nodes = 0;
    return (0);
}


//
// The remaioning functions are used in geometric trees (layer
// expressions).
//

namespace {
    // Tree evaluation call for a variable, which is interpreted as
    // a layer name.  Returns OK if the evaluation was performed.
    //
    bool
    layer_eval(ParseNode *p, siVariable *res, void *datap)
    {
        Variable *v = p->data.v;
        if (!v)
            return (BAD);
        if (p->left)
            return (BAD);

        if (v->type == TYP_STRING || v->type == TYP_NOTYPE) {
            // First time, transform into a TYP_LDESC.  In the new
            // CDldb the findLayer call is rather slow, since we have
            // to split the token, find layer and purpose numbers,
            // etc.  So, caching a pointer to the CDl is a good thing.

            LDorig *ldorig = new LDorig(v->name);
            if (!ldorig->ldesc()) {
                delete ldorig;
                return (BAD);
            }

            if (v->content.string && (v->flags & VF_ORIGINAL)) {
                delete [] v->content.string;
                v->flags &= ~VF_ORIGINAL;
            }
            v->type = TYP_LDORIG;
            v->content.ldorig = ldorig;
        }
        if (v->type == TYP_LDORIG) {
            res->type = TYP_ZLIST;
            SIlexprCx *cx = (SIlexprCx*)datap;
            if (!cx)
                return (BAD);
            XIrt ret = cx->getZlist(v->content.ldorig, &res->content.zlist);
            if (ret == XIintr) {
                cx->handleInterrupt();
                return (OK);
            }
            if (ret == XIbad)
                return (BAD);
            return (OK);
        }
        return (BAD);
    }


    // Alternative to above, passes through the string.
    //
    bool
    string_eval(ParseNode *p, siVariable *res, void*)
    {
        Variable *v = p->data.v;
        if (!v)
            return (BAD);
        if (p->left)
            return (BAD);
        if (v->type != TYP_STRING && v->type != TYP_NOTYPE)
            return (BAD);
        res->type = TYP_STRING;
        res->content.string = v->content.string;
        return (OK);
    }


    // Binary operator evaluation function for TYP_ZLISTs.
    //
    bool
    lpt_bop(ParseNode *p, siVariable *res, void *datap)
    {
        siVariable v[2];
        bool err = (*p->left->evfunc)(p->left, &v[0], datap);
        if (err != OK)
            return (err);
        if (v[0].type != TYP_ZLIST && v[0].type != TYP_SCALAR)
            return (BAD);
        if (v[0].type == TYP_ZLIST) {
            // look-ahead for logical ops
            if (p->optype == TOK_AND || p->optype == TOK_TIMES) {
                if (v[0].content.zlist == 0) {
                    res->type = TYP_ZLIST;
                    res->content.zlist = 0;
                    return (OK);
                }
            }
        }
        err = (*p->right->evfunc)(p->right, &v[1], datap);
        if (err != OK) {
            if (v[0].type == TYP_ZLIST)
                v[0].content.zlist->free();
            return (err);
        }
        if (v[1].type != TYP_ZLIST && v[1].type != TYP_SCALAR) {
            if (v[0].type == TYP_ZLIST)
                v[0].content.zlist->free();
            return (BAD);
        }
        err = (*p->data.f.function)(res, v, datap);
        if (v[0].type == TYP_ZLIST)
            v[0].content.zlist->free();
        if (v[1].type == TYP_ZLIST)
            v[1].content.zlist->free();
        return (err);
    }


    // Unary operator evaluation function for TYP_ZLISTs.
    //
    bool
    lpt_uop(ParseNode *p, siVariable *res, void *datap)
    {
        siVariable r1;
        bool err = (*p->left->evfunc)(p->left, &r1, datap);
        if (err != OK)
            return (err);
        if (r1.type != TYP_ZLIST && r1.type != TYP_SCALAR)
            return (BAD);
        err = (*p->data.f.function)(res, &r1, datap);
        if (r1.type == TYP_ZLIST)
            r1.content.zlist->free();
        return (err);
    }
}


XIrt
ParseNode::evalTree(SIlexprCx *cx, Zlist **zret, PolarityType retwhich)
{
    TimeDbgAccum ac("evalTree");

    *zret = 0;
    if (!(void*)this || !cx)
        return (XIok);

    siVariable v;
    try {
        cx->enableExceptions(true);
        bool ok = (*evfunc)(this, &v, cx);
        cx->enableExceptions(false);
        if (ok != OK)
            return (XIbad);
        if (v.type == TYP_SCALAR) {
            if (retwhich == PolarityClear) {
                if (!to_boolean(v.content.value))
                    *zret = cx->getZref()->copy();
            }
            else {
                if (to_boolean(v.content.value))
                    *zret = cx->getZref()->copy();
            }
        }
        else if (v.type == TYP_ZLIST) {
            if (retwhich == PolarityClear) {
                *zret = cx->getZref()->copy();
                XIrt ret = Zlist::zl_andnot(zret, v.content.zlist);
                if (ret != XIok)
                    return (ret);
            }
            else
                *zret = v.content.zlist;
        }
        else
            return (XIbad);
        return (XIok);
    }
    catch (XIrt ret) {
        return (XIintr);
    }
}


// Return true if the tree is valid.  Note that the layer tokens are
// not tested for existence.  Substitute the local evaluation
// functions.
//
bool
ParseNode::checkTree()
{
    if (!(void*)this)
        return (OK);

    if (type == PT_VAR) {
        if (data.v->type == TYP_STRING || data.v->type == TYP_NOTYPE) {
            evfunc = lexpr_string ? &string_eval : &layer_eval;
            return (OK);
        }
        if (data.v->type == TYP_LDORIG) {
            evfunc = &layer_eval;
            return (OK);
        }

        SIparse()->pushError("-pbad token %s", data.v->name);
        return (BAD);
    }
    if (type == PT_BINOP) {
        switch (optype) {
        case TOK_PLUS:
        case TOK_MINUS:
        case TOK_TIMES:
        case TOK_POWER:
        case TOK_AND:
        case TOK_OR:
            break;
        default:
            SIparse()->pushError("-punknown binop %s", data.f.funcname);
            return (BAD);
        }
        evfunc = &lpt_bop;
        if (left->checkTree() == BAD || right->checkTree() == BAD)
            return (BAD);
        return (OK);
    }
    if (type == PT_UNOP) {
        switch (optype) {
        case TOK_UMINUS:
        case TOK_NOT:
            break;
        default:
            SIparse()->pushError("-punknown unary %s", data.f.funcname);
            return (BAD);
        }
        evfunc = &lpt_uop;
        return (left->checkTree());
    }
    if (type == PT_FUNCTION) {
        // Here we set the ParseNode::lexpt_string flag if the
        // function requires an actual string rather than a Zlist.

        SIptfunc *ptf = SIparse()->altFunction(data.f.funcname);
        unsigned int saflgs = ptf ? ptf->string_arg_flags() : 0;
        for (ParseNode *arg = left; arg; arg = arg->next) {
            arg->lexpr_string = (saflgs & 1);
            if (arg->checkTree() == BAD)
                return (BAD);
            saflgs >>= 1;
        }
        return (OK);
    }
    if (type == PT_CONSTANT)
        return (OK);

    SIparse()->pushError("-pbad node type %d", type);
    return (BAD);
}


// Similar to checjTree, however recursively patch in subtrees from
// derived layer expressions.  The resulting tree should contain only
// normal layers.
//
bool
ParseNode::checkExpandTree(ParseNode **pn)
{
    *pn = 0;
    if (!(void*)this)
        return (OK);

    if (type == PT_VAR) {
        if (data.v->type == TYP_STRING || data.v->type == TYP_NOTYPE) {
            if (lexpr_string) {
                evfunc = &string_eval;
                return (OK);
            }
            evfunc = &layer_eval;
            CDl *ld = CDldb()->findDerivedLayer(data.v->name);
            if (ld) {
                const char *expr = ld->drvExpr();
                *pn = SIparse()->getLexprTree(&expr);
            }
            // Return OK if layer is not found, might not be defined
            // yet.  If so, it must be a normal layer.
            return (OK);
        }
        if (data.v->type == TYP_LDORIG) {
            evfunc = &layer_eval;
            return (OK);
        }

        SIparse()->pushError("-pbad token %s", data.v->name);
        return (BAD);
    }
    if (type == PT_BINOP) {
        switch (optype) {
        case TOK_PLUS:
        case TOK_MINUS:
        case TOK_TIMES:
        case TOK_POWER:
        case TOK_AND:
        case TOK_OR:
            break;
        default:
            SIparse()->pushError("-punknown binop %s", data.f.funcname);
            return (BAD);
        }
        evfunc = &lpt_bop;
        ParseNode *px;
        if (left->checkExpandTree(&px) == BAD)
            return (BAD);
        if (px) {
            delete left;
            left = px;
        }
        if (right->checkExpandTree(&px) == BAD)
            return (BAD);
        if (px) {
            delete right;
            right = px;
        }
        return (OK);
    }
    if (type == PT_UNOP) {
        switch (optype) {
        case TOK_UMINUS:
        case TOK_NOT:
            break;
        default:
            SIparse()->pushError("-punknown unary %s", data.f.funcname);
            return (BAD);
        }
        evfunc = &lpt_uop;
        ParseNode *px;
        if (left->checkExpandTree(&px) == BAD)
            return (BAD);
        if (px) {
            delete left;
            left = px;
        }
        return (OK);
    }
    if (type == PT_FUNCTION) {
        // Here we set the ParseNode::lexpr_string flag if the
        // function requires an actual string rather than a Zlist.

        SIptfunc *ptf = SIparse()->altFunction(data.f.funcname);
        unsigned int saflgs = ptf ? ptf->string_arg_flags() : 0;
        ParseNode *prv = 0;
        for (ParseNode *arg = left; arg; prv = arg, arg = arg->next) {
            arg->lexpr_string = (saflgs & 1);
            ParseNode *px;
            if (arg->checkExpandTree(&px) == BAD)
                return (BAD);
            if (px) {
                px->next = arg->next;
                if (prv)
                    prv->next = px;
                else
                    left = px;
                delete arg;
                arg = px;
            }
            saflgs >>= 1;
        }
        return (OK);
    }
    if (type == PT_CONSTANT)
        return (OK);

    SIparse()->pushError("-pbad node type %d", type);
    return (BAD);
}


// Return a list of layers encountered.  This supports the
// layer_name[.stname][.cellname] form.
//
CDll *
ParseNode::findLayersInTree()
{
    if (!(void*)this)
        return (0);

    if (type == PT_VAR) {
        if (data.v->type == TYP_STRING || data.v->type == TYP_NOTYPE) {
            char *lname = SIparse()->parseLayer(data.v->name, 0, 0);
            CDl *ld = CDldb()->findLayer(lname, Physical);
            if (!ld)
                ld = CDldb()->findDerivedLayer(lname);
            delete [] lname;
            if (ld)
                return (new CDll(ld, 0));
        }
        else if (data.v->type == TYP_LDORIG) {
            CDl *ld = data.v->content.ldorig->ldesc();
            if (ld)
                return (new CDll(ld, 0));
        }
        return (0);
    }
    if (type == PT_BINOP) {
        CDll *l0 = left->findLayersInTree();
        CDll *l1 = right->findLayersInTree();
        if (!l0)
            return (l1);
        if (!l1)
            return (l0);
        CDll *l = l0;
        while (l->next)
            l = l->next;
        l->next = l1;
        return (l0);
    }
    if (type == PT_UNOP)
        return (left->findLayersInTree());
    if (type == PT_FUNCTION) {
        CDll *l0 = 0;
        for (ParseNode *arg = left; arg; arg = arg->next) {
            CDll *l = arg->findLayersInTree();
            if (l) {
                CDll *l1 = l;
                while (l1->next)
                    l1 = l1->next;
                l1->next = l0;
                l0 = l;
            }
        }
        return (l0);
    }
    return (0);
}


// Return true if ld is used in p.
//
bool
ParseNode::isLayerInTree(const CDl *ld)
{
    if (!(void*)this || !ld)
        return (false);

    if (type == PT_VAR) {
        char *lname = SIparse()->parseLayer(data.v->name, 0, 0);
        if (CDldb()->findLayer(lname, Physical) == ld) {
            delete [] lname;
            return (true);
        }
        if (CDldb()->findDerivedLayer(lname) == ld) {
            delete [] lname;
            return (true);
        }
        delete [] lname;
        return (false);
    }
    if (type == PT_BINOP) {
        if (left->isLayerInTree(ld))
            return (true);
        if (right->isLayerInTree(ld))
            return (true);
        return (false);
    }
    if (type == PT_UNOP) {
        if (left->isLayerInTree(ld))
            return (true);
        return (false);
    }
    if (type == PT_FUNCTION) {
        for (ParseNode *arg = left; arg; arg = arg->next) {
            if (arg->isLayerInTree(ld))
                return (true);
        }
        return (false);
    }
    return (false);
}


// Check the layer references in the tree.  If an undefined layer is found,
// return a malloc'ed copy of the bad name.  Return 0 on success.
// This supports the layer_name[.stname][.cellname] form.
//
char *
ParseNode::checkLayersInTree()
{
    if (!(void*)this)
        return (0);

    if (type == PT_VAR) {
        if (lexpr_string)
            // Not a layer name.
            return (0);

        char *lname = SIparse()->parseLayer(data.v->name, 0, 0);
        if (!CDldb()->findLayer(lname, Physical) &&
                !CDldb()->findDerivedLayer(lname))
            return (lname);
        delete [] lname;
        return (0);
    }
    if (type == PT_BINOP) {
        char *s = left->checkLayersInTree();
        if (s)
            return (s);
        s = right->checkLayersInTree();
        if (s)
            return (s);
        return (0);
    }
    if (type == PT_UNOP)
        return (left->checkLayersInTree());
    if (type == PT_FUNCTION) {
        for (ParseNode *arg = left; arg; arg = arg->next) {
            char *s = arg->checkLayersInTree();
            if (s)
                return (s);
        }
        return (0);
    }
    return (0);
}


// Look through the tree for cells specified as part of a layer name, and
// make sure that they are in memory.  If a cell can't be opened, return
// a copy of the name.  Add the bounding box to BB of each cell found.
//
char *
ParseNode::checkCellsInTree(BBox *BB)
{
    if (!(void*)this)
        return (0);

    if (type == PT_VAR) {
        if (lexpr_string)
            // Not a layer name.
            return (0);

        char *cellname, *stname;
        char *lname = SIparse()->parseLayer(data.v->name, &cellname, &stname);
        delete [] lname;
        if (cellname || stname) {
            if (cellname && *cellname == SI_DBCHAR && (!stname || !*stname)) {
                // This triggers use of a named database.
                cSDB *sdb = CDsdb()->findDB(cellname);
                if (!sdb) {
                    char *t = new char[strlen(cellname) + 16];
                    sprintf(t, "(database) %s", cellname+1);
                    delete [] cellname;
                    delete [] stname;
                    return (t);
                }
            }
            else {
                CDs *targ = SIparse()->openReference(cellname, stname);
                if (!targ) {
                    if (!stname || !*stname) {
                        delete [] stname;
                        return (cellname);
                    }
                    if (cellname) {
                        char *t =
                            new char[strlen(stname) + strlen(cellname) + 2];
                        sprintf(t, "%s.%s", stname, cellname);
                        delete [] cellname;
                        delete [] stname;
                        return (t);
                    }
                    else {
                        char *t = new char[strlen(stname) + 2];
                        sprintf(t, "%s.NULL", stname);
                        delete [] stname;
                        return (t);
                    }
                }
                BB->add(targ->BB());
            }
            delete [] cellname;
            delete [] stname;
        }
        return (0);
    }
    if (type == PT_BINOP) {
        char *s = left->checkCellsInTree(BB);
        if (s)
            return (s);
        s = right->checkCellsInTree(BB);
        if (s)
            return (s);
        return (0);
    }
    if (type == PT_UNOP)
        return (left->checkCellsInTree(BB));
    if (type == PT_FUNCTION) {
        for (ParseNode *arg = left; arg; arg = arg->next) {
            char *s = arg->checkCellsInTree(BB);
            if (s)
                return (s);
        }
        return (0);
    }
    return (0);
}

using namespace zlist_funcs;

// When doing a bloat or similar using a grid, we expand the grid cell
// before evaluating, then clip off the excess when done.  This should
// avoid problems combining the results.  Here we look through the
// tree for bloat calls and similar, and return the maximum absolute
// value found.
//
// WARNING
// The bloat value applied to the functions MUST be a constant
// expression.  This is guaranteed in layer expressions, but not in
// regular scripts.  Presently, this function is called only when
// evaluating layer expressions.
//
void
ParseNode::getBloat(int *bp)
{
    if (!(void*)this)
        return;

    if (type == PT_UNOP)
        left->getBloat(bp);
    else if (type == PT_BINOP) {
        left->getBloat(bp);
        right->getBloat(bp);
    }
    else if (type == PT_FUNCTION) {
        if (data.f.function == PTbloatZ || data.f.function == PTedgesZ) {
            // The first argument evaluates to a real number.
            if (left) {
                // The argument had better be a constant expression. 
                // This is guaranteed in a layer expression, not so in
                // a regular script.

                siVariable v;
                (*left->evfunc)(left, &v, 0);
                if (v.type == TYP_SCALAR) {
                    int i = abs(INTERNAL_UNITS(v.content.value));
                    if (i > *bp)
                        *bp = i;
                }
            }
        }
    }
}



/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 1996 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *                                                                        *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,      *
 *   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES      *
 *   OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-        *
 *   INFRINGEMENT.  IN NO EVENT SHALL STEPHEN R. WHITELEY BE LIABLE       *
 *   FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION      *
 *   OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN           *
 *   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN         *
 *   THE SOFTWARE.                                                        *
 *                                                                        *
 *========================================================================*
 *                                                                        *
 * Whiteley Research Circuit Simulation and Analysis Tool                 *
 *                                                                        *
 *========================================================================*
 $Id: evaluate.cc,v 2.86 2016/05/12 03:25:54 stevew Exp $
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3e2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 Wayne A. Christopher
         1992 Stephen R. Whiteley
****************************************************************************/

//
// Convert a parse tree to a list of data vectors. 
//

#include "config.h"
#include "frontend.h"
#include "fteparse.h"
#include "errors.h"
#include "ttyio.h"
#include "circuit.h"
#include "graphics.h"


namespace {
    sDataVec *op_range(pnode*, pnode*);
    sDataVec *op_ind(pnode*, pnode*);
    sDataVec *evfunc(sDataVec**, sFunc*);
    sDataVec *do_fft(sDataVec*, sFunc*);
    void fft_scale(sDataVec*, sDataVec*, bool);
}


// Note that Evaluate will return 0 on invalid expressions.
//
sDataVec *
IFsimulator::Evaluate(pnode *node)
{
    if (!node)
        return (0);
    sDataVec *d = 0;
    if (node->value()) {
        if (node->is_localval()) {
            d = node->value()->copy();
            d->newtemp();
        }
        else
            d = node->value();
    }
    else if (node->token_string()) {
        if (node->type() == PN_TRAN) {
            if (!ft_curckt || !ft_curckt->runckt()) {
                GRpkgIf()->ErrPrintf(ET_ERROR,
                    "no circuit, evaluation failed for %s.\n",
                    node->token_string());
                return (0);
            }
            sDataVec *scale = 0;
            if (ft_plot_cur && (scale = ft_plot_cur->scale()) != 0 &&
                    scale->length() > 1 && scale->isreal()) {
                double *vec;
                if (ft_curckt->runckt()->evalTranFunc(&vec,
                        node->token_string(), scale->realvec(),
                        scale->length())) {
                    d = new sDataVec(lstring::copy(node->token_string()), 0,
                        scale->length(), 0, vec);
                    d->set_scale(scale);
                    d->newtemp();
                }
                else {
                    GRpkgIf()->ErrPrintf(ET_ERROR,
                        "evaluation failed for %s.\n", node->token_string());
                    return (0);
                }
            }
            else {
                GRpkgIf()->ErrPrintf(ET_ERROR, "bad scale for %s.\n",
                    node->token_string());
                return (0);
            }
        }
        if (d == 0) {
            d = VecGet(node->token_string(),
                ft_curckt ? ft_curckt->runckt() : 0);
            // note that "vs" can be a real vector, x-y plots only if
            // undefined.
            //
            if (d == 0) {
                if (!node->name() || !lstring::cieq(node->name(), "vs"))
                    Error(E_NOVEC, 0, node->token_string());
                return (0);
            }
            if (!d->link()) {
                d = d->copy();
                d->newtemp();
            }
            else {
                for (sDvList *dl = d->link(); dl; dl = dl->dl_next) {
                    if (dl->dl_dvec && (dl->dl_dvec->flags() & VF_PERMANENT)) {
                        dl->dl_dvec = dl->dl_dvec->copy();
                        dl->dl_dvec->newtemp();
                    }
                }
            }
        }
    }
    else if (node->func())
        d = node->apply_func();
    else if (node->oper())
        d = node->apply_bop();
    else {
        GRpkgIf()->ErrPrintf(ET_INTERR, "Evaluate: bad node.\n");
        return (0);
    }

    if (!d)
        return (0);

    if (node->name() && !ft_flags[FT_EVDB])
        d->set_name(node->name());
    
    if (!d->length() && !d->link()) {
        Error(E_NOVEC, 0, d->name());
        return (0);
    }
    return (d);
}


// Create a dveclist, free the pnlist.
//
sDvList *
IFsimulator::DvList(pnlist *pl0)
{
    sDvList *dl0 = 0, *dl = 0;
    for (pnlist *pl = pl0; pl; pl = pl->next()) {
        sDataVec *v = Evaluate(pl->node());
        if (v == 0) {
            // Just ignore bad vectors, Evaluate provides
            // error message.
            //
            if (!pl->node()->name() || !lstring::cieq(pl->node()->name(), "vs"))
                // special case
                continue;
        }
        if (!dl0)
            dl0 = dl = new sDvList;
        else {
            dl->dl_next = new sDvList;
            dl = dl->dl_next;
        }
        if (v && v->link()) {
            for (sDvList *dll = v->link(); dll; dll = dll->dl_next) {
                dl->dl_dvec = dll->dl_dvec;
                if (dll->dl_next) {
                    dl->dl_next = new sDvList;
                    dl = dl->dl_next;
                }
            }
        }
        else
            dl->dl_dvec = v;
    }
    pnlist::destroy(pl0);
    return (dl0);
}


namespace {
    // Return the actual count and up to n args in the array, or -1 if
    // error.
    //
    int get_n_args(pnode *ptop, int n, sDataVec **args, bool *more)
    {
        *more = false;
        if (!ptop)
            return (0);
        *more = true;
        int cnt = 0;
        while (cnt < n) {
            if (ptop->oper() && ptop->oper()->optype() == TT_COMMA) {
                if (!ptop->left())
                    return (-1);
                args[cnt] = Sp.Evaluate(ptop->left());
                if (!args[cnt])
                    return (-1);
                if (!ptop->right())
                    return (-1);
                ptop = ptop->right();
                cnt++;
            }
            else {
                args[cnt] = Sp.Evaluate(ptop);
                cnt++;
                *more = false;
                break;
            }
        }
        return (cnt);
    }
}


// Apply a function to an argument. Complex functions are called as follows:
//  cx_something(data, type, length, &newlength, &newtype),
//  and returns a char * that is cast to complex or double.
//
sDataVec *
pnode::apply_func() const
{
    if (!pn_func)
        return (0);

    // Special case.  Resolve vector reference
    if (!pn_func->func()) {
        if (!pn_left->pn_string) {
            GRpkgIf()->ErrPrintf(ET_ERROR, "bad %s() syntax.\n",
                pn_func->name());
            return (0);
        }
        char buf[256];
        if (*pn_func->name() == 'v')
            sprintf(buf, "v(%s)", pn_left->pn_string);
        else
            sprintf(buf, "%s", pn_left->pn_string);

        // If the vector is "special" we must use VecGet, otherwise
        // it should be in the current plot.
        sDataVec *t;
        if (*buf == Sp.SpecCatchar()) {
            sCKT *ckt = Sp.CurCircuit() ? Sp.CurCircuit()->runckt() : 0;
            t = Sp.VecGet(buf, ckt);
        }
        else
            t = Sp.CurPlot()->find_vec(buf);
        if (!t) {
            Sp.Error(E_NOVEC, 0, buf);
            return (0);
        }
        t = t->copy();
        t->newtemp();
        return (t);
    }

    if (pn_func->func() == &sDataVec::v_undefined) {
        // This is a dummy node that was created to resolve an
        // undefined function reference.  We will try and resolve it
        // now as a user-defined function.

        pnode *p = Sp.GetUserFuncTree(pn_func->name(), pn_left);
        if (p) {
            sDataVec *dv = Sp.Evaluate(p);
            if (!dv) {
                GRpkgIf()->ErrPrintf(ET_ERROR,
                    "evaluation of function %s failed.\n",
                    pn_func->name());
            }
            delete p;
            return (dv);
        }
        else {
            GRpkgIf()->ErrPrintf(ET_ERROR, "call to unknown function %s.\n",
                pn_func->name());
            return (0);
        }
    }

    sDataVec *v[10];
    memset(v, 0, sizeof(v));
    int nargs = 1;
    if (pn_func->argc() > 1) {
        bool more;
        nargs = get_n_args(pn_left, pn_func->argc(), v, &more);
        if (nargs < 0) {
            GRpkgIf()->ErrPrintf(ET_ERROR, "bad argument list to %s.\n",
                pn_func->name());
            return (0);
        }
        if (nargs < pn_func->argc()) {
            if (nargs == 0 || pn_func->func() !=
                    (fuFuncType)&sDataVec::v_hs_gauss) {
                GRpkgIf()->ErrPrintf(ET_ERROR, "too few arguments to %s.\n",
                    pn_func->name());
                return (0);
            }
        }
        if (more) {
            GRpkgIf()->ErrPrintf(ET_ERROR, "too many arguments to %s.\n",
                pn_func->name());
            return (0);
        }
    }
    else {
        v[0] = Sp.Evaluate(pn_left);
        if (v[0] == 0) {
            GRpkgIf()->ErrPrintf(ET_ERROR, "bad argument to %s.\n",
                pn_func->name());
            return (0);
        }
        if (v[0]->link()) {
            sDvList *tl0 = 0, *tl = 0;
            for (sDvList *dl = v[0]->link(); dl; dl = dl->dl_next) {
                v[0] = dl->dl_dvec;
                sDataVec *dv = evfunc(v, pn_func);
                if (dv) {
                    if (!tl0)
                        tl0 = tl = new sDvList;
                    else {
                        tl->dl_next = new sDvList;
                        tl = tl->dl_next;
                    }
                    tl->dl_dvec = dv;
                }
            }
            if (tl0) {
                sDataVec *newv = new sDataVec(lstring::copy("list"), 0, 0);
                newv->set_link(tl0);
                newv->newtemp();
                return (newv);
            }
            else
                return (0);
        }
    }
    sDataVec *res = evfunc(v, pn_func);
    if (res && nargs == 1 && !v[0]->link() && !res->scale() &&
            res->length() > 1) {
        sDataVec *sc = v[0]->scale();
        if (!sc && v[0]->plot() && v[0]->plot() != Sp.CurPlot())
            sc = v[0]->plot()->scale();
        if (sc && sc->length() == res->length() && sc->isreal())
            res->set_scale(sc);
    }
    return (res);
}


// Operate on two vectors, and return a third with the data, length,
// and flags fields filled in.  Add it to the current plot and get rid
// of the two args.
//
sDataVec *
pnode::apply_bop() const
{
    if (pn_op->optype() == TT_RANGE)
        return (op_range(pn_left, pn_right));
    if (pn_op->optype() == TT_INDX)
        return (op_ind(pn_left, pn_right));

    sDataVec *v1 = Sp.Evaluate(pn_left);
    if (!v1) {
        const char *s = pn_left->get_string();
        GRpkgIf()->ErrPrintf(ET_ERROR, "\"%s\"  evaluation failed.\n", s);
        delete [] s;
        return (0);
    }

    if (pn_op->optype() == TT_COLON) {
        // The colon operator is a side-effect of the conditional.  If
        // the parser doesn't catch a colon without the '?', just
        // return the "true" part.

        return (v1);
    }
    if (pn_op->optype() == TT_COND) {
        if (!pn_right || !pn_right->pn_op ||
                pn_right->pn_op->optype() != TT_COLON) {
            GRpkgIf()->ErrPrintf(ET_ERROR,
                "internal, parse error in conditional.\n");
            return (0);
        }
        sDataVec *v2;
        if (v1->realval(0) != 0.0) {
            if (!pn_right->pn_left) {
                const char *s = pn_right->get_string();
                GRpkgIf()->ErrPrintf(ET_ERROR,
                    "\"%s\", left evaluation failed.\n", s);
                delete [] s;
                return (0);
            }
            v2 = Sp.Evaluate(pn_right->pn_left);
            if (!v2) {
                const char *s = pn_right->pn_left->get_string();
                GRpkgIf()->ErrPrintf(ET_ERROR,
                    "\"%s\", evaluation failed.\n", s);
                delete [] s;
            }
        }
        else {
            if (!pn_right->pn_right) {
                const char *s = pn_right->get_string();
                GRpkgIf()->ErrPrintf(ET_ERROR,
                    "\"%s\", right evaluation failed.\n", s);
                delete [] s;
                return (0);
            }
            v2 = Sp.Evaluate(pn_right->pn_right);
            if (!v2) {
                char *s = pn_right->pn_right->get_string();
                GRpkgIf()->ErrPrintf(ET_ERROR,
                    "\"%s\", evaluation failed.\n", s);
                delete [] s;
            }
        }
        return (v2);
    }

    sDataVec *v2 = Sp.Evaluate(pn_right);
    if (!v2) {
        char *s = pn_right->get_string();
        GRpkgIf()->ErrPrintf(ET_ERROR, "\"%s\", evaluation failed.\n", s);
        delete [] s;
        return (0);
    }

    // Now the question is, what do we do when one or both of these
    // has more than one vector?  This is definitely not a good
    // thing.  For the time being don't do anything.
    //
    if (v1->link() || v2->link()) {
        GRpkgIf()->ErrPrintf(ET_WARN, "no operations on wildcards yet.\n");
        return (0);
    }
    
    // Make sure we have data of the same length
    bool free1, free2;
    int length = ((v1->length() > v2->length()) ? v1->length() : v2->length());
    sDataVec *x1 = v1->pad(length, &free1);;
    sDataVec *x2 = v2->pad(length, &free2);;

    // Pass the vectors to the appropriate function.
    sDataVec *res = (x1->*pn_op->func())(x2);

    if (res) {
        // Note: this string can get rather long, e.g., for max(min(), min())
        char *t0 = new char [strlen(v1->name()) + strlen(v2->name()) +
            strlen(pn_op->name()) + 5];
        char *t = t0;
        *t++ = '(';
        strcpy(t, v1->name());
        while(*t) t++;
        *t++ = ')';
        strcpy(t, pn_op->name());
        while(*t) t++;
        *t++ = '(';
        strcpy(t, v2->name());
        while(*t) t++;
        *t++ = ')';
        *t = 0;
        res->set_name(t0);
        delete [] t0;

        // Give the result the maximum dimensionality of the two vectors.
        // This also arbitrates the scale of the resultant.
        if (v1->numdims() >= v2->numdims()) {
            for (int i = 0; i < v1->numdims(); i++)
                res->set_dims(i, v1->dims(i));
            res->set_numdims(v1->numdims());
            if (res->length() > 1) {
                sDataVec *sc = v1->scale();
                if (!sc && v1->plot() && v1->plot() != Sp.CurPlot())
                    sc = v1->plot()->scale();
                if (sc && sc->length() == res->length() && sc->isreal())
                    res->set_scale(sc);
            }
        }
        else {
            for (int i = 0; i < v2->numdims(); i++)
                res->set_dims(i, v2->dims(i));
            res->set_numdims(v2->numdims());
            if (res->length() > 1) {
                sDataVec *sc = v1->scale();
                if (!sc && v2->plot() && v2->plot() != Sp.CurPlot())
                    sc = v2->plot()->scale();
                if (sc && sc->length() == res->length() && sc->isreal())
                    res->set_scale(sc);
            }
        }

        // Copy a few useful things
        res->set_defcolor(v1->defcolor());
        res->set_gridtype(v1->gridtype());
        res->set_plottype(v1->plottype());

        res->newtemp();
    }

    // Free the temporary data areas we used, if we allocated any
    if (free1)
        delete x1;
    if (free2)
        delete x2;

    return (res);
}


namespace {
    // This is an odd operation.  The first argument is the name of a
    // vector, and the second is a range in the scale, so that v(1)[[10,
    // 20]] gives all the values of v(1) for which the TIME value is
    // between 10 and 20.  If there is one argument it picks out the
    // values which have that scale value.  NOTE that we totally ignore
    // multi-dimensionality here -- the result is a 1-dim vector.
    //
    sDataVec *op_range(pnode *arg1, pnode *arg2)
    {
        sDataVec *v = Sp.Evaluate(arg1);
        sDataVec *ind = Sp.Evaluate(arg2);
        if (!v || !ind)
            return (0);
        sDataVec *scale = v->scale();
        if (!scale)
            scale = v->plot()->scale();
        if (!scale) {
            GRpkgIf()->ErrPrintf(ET_ERROR, "no scale for vector %s.\n",
                v->name());
            return (0);
        }

        if (ind->length() != 1) {
            GRpkgIf()->ErrPrintf(ET_ERROR, "strange range specification.\n");
            return (0);
        }
        double up, low;
        if (ind->isreal())
            up = low = ind->realval(0);
        else {
            up = ind->realval(0);
            low = ind->imagval(0);
        }
        bool rev = false;
        if (up < low) {
            double td = up;
            up = low;
            low = td;
            rev = true;
        }
        int i, len;
        for (i = len = 0; i < scale->length(); i++) {
            double td = scale->realval(i);
            if ((td <= up) && (td >= low))
                len++;
        }

        char *bf = new char[strlen(v->name()) + strlen(ind->name()) + 5];
        sprintf(bf, "%s[[%s]]", v->name(), ind->name());
        sDataVec *res = new sDataVec(bf, v->flags() & VF_COPYMASK,
            len, v->units());
        res->set_gridtype(v->gridtype());
        res->set_plottype(v->plottype());
        res->set_defcolor(v->defcolor());
        res->set_scale(scale);
        res->set_numdims(v->numdims());
        for (i = 0; i < v->numdims(); i++)
            res->set_dims(i, v->dims(i));
        
        // Toss in the data
        int j = 0;
        for (i = (rev ? v->length() - 1 : 0); i != (rev ? -1 : v->length());
                rev ? i-- : i++) {
            double td = scale->realval(i);
            if ((td <= up) && (td >= low)) {
                if (res->isreal())
                    res->set_realval(j, v->realval(i));
                else
                    res->set_compval(j, v->compval(i));
                j++;
            }
        }
        if (j != len)
            GRpkgIf()->ErrPrintf(ET_ERROR, "something funny..\n");

        res->newtemp();
        return (res);
    }


    // This is another operation we do specially -- if the argument is
    // a vector of dimension n, n > 0, the result will be either a
    // vector of dimension n - 1, or a vector of dimension n with only
    // a certain range of vectors present.
    //
    sDataVec *op_ind(pnode *arg1, pnode *arg2)
    {
        sDataVec *v = Sp.Evaluate(arg1);
        sDataVec *ind = Sp.Evaluate(arg2);
        if (!v || !ind)
            return (0);

        // First let's check to make sure that the vector is consistent
        if (v->numdims() > 1) {
            int i, j;
            for (i = 0, j = 1; i < v->numdims(); i++)
                j *= v->dims(i);
            if (v->length() != j) {
                GRpkgIf()->ErrPrintf(ET_INTERR,
                    "op_ind: length %d should be %d.\n", v->length(), j);
                return (0);
            }
        }
        else {
            // Just in case we were sloppy
            v->set_numdims(1);
            v->set_dims(0, v->length());
        }

        if (ind->length() != 1) {
            GRpkgIf()->ErrPrintf(ET_ERROR, "index %s is not of length 1.\n",
                ind->name());
            return (0);
        }

        int majsize = v->dims(0);
        if (majsize <= 1 && v->length() >= 1)
            majsize = v->length();
        int blocksize = v->length()/majsize;

        // Now figure out if we should put the dim down by one. 
        // Because of the way we parse the index, we figure that if
        // the value is complex (e.g, "[1,2]"), the guy meant a range. 
        // This is sort of bad though.
        //
        bool rev = false;
        bool newdim;
        int up, down, length;
        if (ind->isreal()) {
            newdim = true;
            down = up = (int)ind->realval(0);
            length = blocksize;
            if (down < 0) {
                GRpkgIf()->ErrPrintf(ET_WARN, "index %d should be 0.\n", down);
                down = up = 0;
            }
            else if (down >= v->length())
                GRpkgIf()->ErrPrintf(ET_WARN, "index %d out of range.\n", down);
        }
        else {
            newdim = false;
            down = (int)ind->realval(0);
            up = (int)ind->imagval(0);

            if (up < down) {
                int i = up;
                up = down;
                down = i;
                rev = true;
            }
            if (up < 0) {
                GRpkgIf()->ErrPrintf(ET_WARN, "upper limit %d should be 0.\n",
                    up);
                up = 0;
            }
            if (up >= majsize) {
                GRpkgIf()->ErrPrintf(ET_WARN, "upper limit %d should be %d.\n",
                    up, majsize - 1);
                up = majsize - 1;
            }
            if (down < 0) {
                GRpkgIf()->ErrPrintf(ET_WARN, "lower limit %d should be 0.\n",
                    down);
                down = 0;
            }
            if (down >= majsize) {
                GRpkgIf()->ErrPrintf(ET_WARN, "lower limit %d should be %d.\n",
                    down, majsize - 1);
                down = majsize - 1;
            }
            length = blocksize * (up - down + 1);
        }

        // Make up the new vector
        char *bf = new char[strlen(v->name()) + strlen(ind->name()) + 3];
        sprintf(bf, "%s[%s]", v->name(), ind->name());
        sDataVec *res = new sDataVec(bf, v->flags() & VF_COPYMASK,
            length, v->units());
        res->set_defcolor(v->defcolor());
        res->set_gridtype(v->gridtype());
        res->set_plottype(v->plottype());
        if (newdim) {
            res->set_numdims(v->numdims() - 1);
            for (int i = 0; i < res->numdims(); i++)
                res->set_dims(i, v->dims(i + 1));
            if (res->numdims() <= 0) {
                res->set_numdims(1);
                res->set_dims(0, length);
            }
        }
        else {
            res->set_numdims(v->numdims());
            res->set_dims(0, up - down + 1);
            for (int i = 1; i < res->numdims(); i++)
                res->set_dims(i, v->dims(i));
        }

        // And toss in the new data
        if (res->isreal()) {
            double *src = v->realvec() + up*blocksize;
            double *dst = res->realvec() + (rev ? 0 : up - down)*blocksize;
            for (int j = up - down; j >= 0; j--) {
                if (src + blocksize <= v->realvec() + v->length())
                    DCOPY(src, dst, blocksize);
                src -= blocksize;
                dst += (rev ? blocksize : -blocksize);
            }
        }
        else {
            complex *src = v->compvec() + up*blocksize;
            complex *dst = res->compvec() + (rev ? 0 : up - down)*blocksize;
            for (int j = up - down; j >= 0; j--) {
                if (src + blocksize <= v->compvec() + v->length())
                    CCOPY(src,dst,blocksize);
                src -= blocksize;
                dst += (rev ? blocksize : -blocksize);
            }
        }
        
        // This is a problem -- the old scale will be no good.  I guess we
        // should make an altered copy of the old scale also.
        //
        res->set_scale(0);

        res->newtemp();
        return (res);
    }


    sDataVec *evfunc(sDataVec **v, sFunc *func)
    {
        sDataVec *res;
        if (func->func() == &sDataVec::v_fft ||
                func->func() == &sDataVec::v_ifft)
            res = do_fft(v[0], func);
        else if (func->argc() == 1)
            res = (v[0]->*func->func())();
        else
            res = (v[0]->*(sDataVec*(sDataVec::*)(sDataVec**))
                func->func())(v+1);

        if (res) {
            res->newtemp();
#ifdef FTEDEBUG
            if (ft_evdb)
                GRpkgIf()->ErrPrintf(ET_MSGS,
                    "apply_func: func %s to %s len %d, type %d\n",
                        func->fu_name, v->v_name, res->length(), res->flags());
#endif
            char *bf =
                new char[strlen(func->name()) + strlen(v[0]->name()) + 3];
            sprintf(bf, "%s(%s)", func->name(), v[0]->name());
            res->set_name(bf);
            delete [] bf;

            res->set_numdims(v[0]->numdims());
            for (int i = 0; i < res->numdims(); i++)
                res->set_dims(i, v[0]->dims(i));

            if (func->func() == &sDataVec::v_fft ||
                    func->func() == &sDataVec::v_ifft) {
                sDataVec *scale = v[0]->scale();
                if (!scale && v[0]->plot())
                    scale = v[0]->plot()->scale();
                fft_scale(res, scale, (func->func() == &sDataVec::v_ifft));
            }
            else
                res->set_scale(v[0]->scale());

            // Copy a few useful things
            res->set_defcolor(v[0]->defcolor());
            res->set_gridtype(v[0]->gridtype());
            res->set_plottype(v[0]->plottype());
        }
        return (res);
    }


    // Perform the fft functions, taking care of dimensions.
    //
    sDataVec *do_fft(sDataVec *v, sFunc *func)
    {
        void *odata = 0;
        if (v->numdims() <= 1) {
            sDataVec *res = (v->*func->func())();
            return (res);
        }

        int blsize = v->dims(v->numdims() - 1);
        int nblks = 1;
        int i;
        for (i = 0; i < v->numdims() - 1; i++)
            nblks *= v->dims(i);
        int vlen = blsize*nblks;

        // make sure we have a complete data set
        bool free1 = false;
        sDataVec *xv = new sDataVec;
        *xv = *v;
        if (vlen < v->length()) {
            xv->set_length(vlen);
            free1 = true;
            if (v->isreal()) {
                xv->set_realvec(new double[vlen]);
                for (i = 0; i < v->length(); i++)
                    xv->set_realval(i, v->realval(i));
                for ( ; i < vlen; i++)
                    xv->set_realval(i, 0.0);
                odata = xv->realvec();
            }
            else {
                xv->set_compvec(new complex[vlen]);
                for (i = 0; i < v->length(); i++)
                    xv->set_compval(i, v->compval(i));
                complex ctmp;
                for ( ; i < vlen; i++)
                    xv->set_compval(i, ctmp);
                odata = xv->compvec();
            }
        }
        xv->set_length(blsize);

        // do the first block to get new block size
        sDataVec *res = (((sDataVec*)xv)->*func->func())();

        int len = nblks*(res->length());
        // we've now set blen, len, and type

        if (res->isreal()) {
            double *dd = res->realvec();
            res->set_realvec(new double[len]);
            for (i = 0; i < res->length(); i++)
                res->set_realval(i, dd[i]);
            delete [] dd;
        }
        else {
            complex *cc = res->compvec();
            res->set_compvec(new complex[len]);
            for (i = 0; i < res->length(); i++)
                res->set_compval(i, cc[i]);
            delete [] cc;
        }
        int rind = res->length();
        if (xv->isreal())
            xv->set_realvec(xv->realvec() + blsize);
        else
            xv->set_compvec(xv->compvec() + blsize);
            
        // now do the rest of the blocks
        for (i = 1; i < nblks; i++) {
            sDataVec *rtmp = (((sDataVec*)xv)->*func->func())();

            if (rtmp->isreal()) {
                double *dd = rtmp->realvec();
                for (i = 0; i < res->length(); i++)
                    res->set_realval(i + rind, dd[i]);
            }
            else {
                complex *cc = rtmp->compvec();
                for (i = 0; i < res->length(); i++)
                    res->set_compval(i + rind, cc[i]);
            }
            rind += res->length();
            delete rtmp;

            if (xv->isreal())
                xv->set_realvec(xv->realvec() + blsize);
            else
                xv->set_compvec(xv->compvec() + blsize);
        }
        res->set_dims(res->numdims() - 1, res->length());
        res->set_length(vlen);
        res->set_allocated(vlen);
        if (xv->isreal())
            xv->set_realvec(free1 ? (double*)odata : 0);
        else
            xv->set_compvec(free1 ? (complex*)odata : 0);
        delete xv;
        return (res);
    }


    // Create an appropriate scale for fft result.
    //
    void fft_scale(sDataVec *v, sDataVec *scale, bool inv)
    {
        double fc = inv ? 1.0 : 0.5;
        if (scale && scale->length() > 1) {
            // data samples assumed equally spaced
            double delta = scale->realval(1) - scale->realval(0);
            if (delta != 0)
                fc = inv ? 1.0/delta : 0.5/delta;
        }

        sDataVec *t = new sDataVec;
        t->newtemp();
        t->set_flags(0);
        if (scale && *scale->units() == UU_TIME)
            t->units()->set(UU_FREQUENCY);
        else if (scale && *scale->units() == UU_FREQUENCY)
            t->units()->set(UU_TIME);

        int nblks = 1;
        int blsize;
        if (v->numdims() > 1) {
            blsize = v->dims(v->numdims() - 1);
            for (int i = 0; i < v->numdims() - 1; i++)
                nblks *= v->dims(i);
        }
        else
            blsize = v->length();

        t->set_length(nblks*blsize);
        double *d = new double[t->length()];
        t->set_realvec(d);
        while (nblks--) {
            if (inv) {
                for (int i = 0; i < blsize; i++)
                    d[i] = fc*((double)i)/blsize;
            }
            else {
                for (int i = 0; i < blsize; i++)
                    d[i] = fc*((double)i)/(blsize-1);
            }
            d += blsize;
        }
        if (v->name()) {
            char *bf = new char[strlen(v->name()) + 10];
            sprintf(bf, "%s_scale", v->name());
            t->set_name(bf);
            delete [] bf;
        }
        t->set_numdims(v->numdims());
        for (int i = 0; i < t->numdims(); i++)
            t->set_dims(i, v->dims(i));
        if (t->numdims() > 0)
            t->set_dims(t->numdims() - 1, blsize);
        v->set_scale(t);
    }
}


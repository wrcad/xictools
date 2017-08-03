
/*========================================================================*
 *                                                                        *
 *  Whiteley Research Inc, Sunnyvale, CA USA, http://wrcad.com            *
 *  Copyright (C) 2017 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *  Licensed under the Apache License, Version 2.0 (the "License");       *
 *  you may not use this file except in compliance with the License.      *
 *  You may obtain a copy of the License at                               *
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
 * WRspice Circuit Simulation and Analysis Tool:  Device Library          *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#include "gencurrent.h"

//XXX Gak!  fix this stuff

// List-free function for ElemList.
//
void
dvaMatrix::ElemList::free()
{
    ElemList *e = this;
    while (e) {
        ElemList *en = e->next;
        delete e;
        e = en;
    }
}


// List-free function for HeadList.
//
void
dvaMatrix::HeadList::free()
{
    HeadList *r = this;
    while (r) {
        HeadList *rn = r->next;
        delete r;
        r = rn;
    }
}


// Zero all of the data.
//
void
dvaMatrix::clear()
{
    for (HeadList *r = dva_rows; r; r = r->next) {
        for (ElemList *e = r->head; e; e = e->next) {
            e->real = 0.0;
            e->imag = 0.0;
        }
    }
    for (ElemList *e = dva_rvec; e; e = e->next) {
        e->real = 0.0;
        e->imag = 0.0;
    }
}


namespace {
    // Something to return for ground node
    double dummy[2];
}

// Return a pointer the the row,col matrix element data.  The matrix
// element is created if it does not exist.  The pointer points to the
// real data, the imaginary data is at the next address.  This function
// should never fail
//
double *
dvaMatrix::get_elem(int row, int col)
{
    if (row <= 0 || col <= 0)
        return (dummy);
    HeadList *thisrow = 0;
    HeadList *rp = 0;
    for (HeadList *r = dva_rows; r; r = r->next) {
        if (r->row < row) {
            rp = r;
            continue;
        }
        if (r->row == row)
            thisrow = r;
        else
            thisrow = new HeadList(row, r);
        break;
    }
    if (!thisrow)
        thisrow = new HeadList(row, 0);
    if (rp)
        rp->next = thisrow;
    else
        dva_rows = thisrow;

    ElemList *thiscol = 0;
    ElemList *ep = 0;
    for (ElemList *e = thisrow->head; e; e = e->next) {
        if (e->indx < col) {
            ep = e;
            continue;
        }
        if (e->indx == col)
            return (&e->real);
        thiscol = new ElemList(col, e);
        break;
    }
    if (!thiscol)
        thiscol = new ElemList(col, 0);
    if (ep)
        ep->next = thiscol;
    else
        thisrow->head = thiscol;
    return (&thiscol->real);
}


// Return a pointer to the row'th element in the RHS data.  The
// element is created if it does not exist
//
double *
dvaMatrix::get_elem(int row)
{
    if (row <= 0)
        return (dummy);
    ElemList *thisval = 0;
    ElemList *ep = 0;
    for (ElemList *e = dva_rvec; e; e = e->next) {
        if (e->indx < row) {
            ep = e;
            continue;
        }
        if (e->indx == row)
            return (&e->real);
        thisval = new ElemList(row, e);
        break;
    }
    if (!thisval)
        thisval = new ElemList(row, 0);
    if (ep)
        ep->next = thisval;
    else
        dva_rvec = thisval;
    return (&thisval->real);
}


// Compute and return the device current for the node passed as row. 
// The vals are the last node voltages.  This is used for dc and
// transient analysis
//
double
dvaMatrix::compute_real(int row, double *vals)
{
    if (vals) {
        for (HeadList *r = dva_rows; r; r = r->next) {
            if (r->row < row)
                continue;
            if (r->row > row)
                return (0.0);
            double sum = 0.0;
            for (ElemList *e = r->head; e; e = e->next)
                sum += e->real * vals[e->indx];
            for (ElemList *e = dva_rvec; e; e = e->next) {
                if (e->indx == row) {
                    sum -= e->real;
                    break;
                }
            }
            return (sum);
        }
    }
    return (0.0);
}


// Compute and return the device current for the node passed as row. 
// The rvals,ivals are the last node voltages.  This is used for ac
// small-signal analysis.  The results are returned in rret,iret
//
void
dvaMatrix::compute_cplx(int row, double *rvals, double *ivals, double *rret,
    double *iret)
{
    if (rvals && ivals) {
        for (HeadList *r = dva_rows; r; r = r->next) {
            if (r->row < row)
                continue;
            if (r->row > row)
                break;
            double sumr = 0.0;
            double sumi = 0;
            for (ElemList *e = r->head; e; e = e->next) {
                sumr += e->real*rvals[e->indx] - e->imag*ivals[e->indx];
                sumi += e->real*ivals[e->indx] + e->imag*rvals[e->indx];
            }
            *rret = sumr;
            *iret = sumi;
            return;
        }
    }
    *rret = 0.0;
    *iret = 0.0;
}


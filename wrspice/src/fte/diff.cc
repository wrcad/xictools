
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
Authors: 1985 Wayne A. Christopher
         1992 Stephen R. Whiteley
****************************************************************************/

#include "simulator.h"
#include "commands.h"
#include "datavec.h"
#include "output.h"
#include "kwords_fte.h"
#include "ttyio.h"
#include "wlist.h"
#include "spnumber/hash.h"
#include "spnumber/spnumber.h"
#include "ginterf/graphics.h"


//
// Do a 'diff' of two plots.
//

namespace {
    bool nameeq(const char*, const char*);
    bool inwlist(const char*, wordlist*);
}

struct dvlist2
{
    const char *d2_name;
    sDataVec *d2_dv1;
    sDataVec *d2_dv2;
    dvlist2 *d2_next;
};


void
CommandTab::com_diff(wordlist *wl)
{
    const char *msg1 = "no such plot %s.\n";
    const char *msg2 = "plots %s and %s seem to be %s.\n";

    VTvalue vv;
    double vntol = DEF_diff_vntol;
    if (Sp.GetVar(kw_diff_vntol, VTYP_REAL, &vv))
        vntol = vv.get_real();
    double abstol = DEF_diff_abstol;
    if (Sp.GetVar(kw_diff_abstol, VTYP_REAL, &vv))
        abstol = vv.get_real();
    double reltol = DEF_diff_reltol;
    if (Sp.GetVar(kw_diff_reltol, VTYP_REAL, &vv))
        reltol = vv.get_real();

    // Let's try to be clever about defaults.
    sPlot *p1 = 0;
    if (!wl || !wl->wl_next) {
        p1 = OP.curPlot();
        if (p1 == OP.constants()) {
            GRpkg::self()->ErrPrintf(ET_ERROR,
                "invalid current plot (constants).\n");
            return;
        }
    }

    sPlot *p2 = 0;
    if (!wl) {
        // Try the current and previous plot of same name.
        for (p2 = p1->next_plot(); p2 && !lstring::eq(p1->name(), p2->name());
            p2 = p2->next_plot()) ;
        if (!p2) {
            GRpkg::self()->ErrPrintf(ET_ERROR, "no previous similar plot.\n");
            return;
        }
        TTY.printf("Plots are \"%s\" and \"%s\"\n",
            p1->type_name(), p2->type_name());
    }

    else if (!wl->wl_next) {
        // Try current and named plot.
        for (p2 = OP.plotList(); p2 &&
                !lstring::eq(p2->type_name(), wl->wl_word);
                p2 = p2->next_plot()) ;

        if (!p2 || p2 == OP.constants()) {
            GRpkg::self()->ErrPrintf(ET_ERROR,
                "invalid plot \"%s\".\n", wl->wl_word);
            return;
        }
        TTY.printf("Plots are \"%s\" and \"%s\"\n",
            p1->type_name(), p2->type_name());
        wl = 0;
    }
    else {
        for (p1 = OP.plotList(); p1; p1 = p1->next_plot()) {
            if (lstring::eq(wl->wl_word, p1->type_name()))
                break;
        }
        if (!p1) {
            GRpkg::self()->ErrPrintf(ET_ERROR, msg1, wl->wl_word);
            return;
        }
        wl = wl->wl_next;

        for (p2 = OP.plotList(); p2; p2 = p2->next_plot()) {
            if (lstring::eq(wl->wl_word, p2->type_name()))
                break;
        }
        if (!p2) {
            GRpkg::self()->ErrPrintf(ET_ERROR, msg1, wl->wl_word);
            return;
        }
        wl = wl->wl_next;
    }

    if (p1 == p2)
        return;

    // Now do some tests to make sure these plots are really the
    // same type, etc.
    //
    if (!lstring::eq(p1->name(), p2->name())) {
        GRpkg::self()->ErrPrintf(ET_WARN, msg2,
            p1->type_name(), p2->type_name(), "of different types");
    }
    if (!lstring::eq(p1->title(), p2->title())) {
        GRpkg::self()->ErrPrintf(ET_WARN, msg2,
            p1->type_name(), p2->type_name(), "from different circuits");
    }

    wordlist *wl10 = p1->list_perm_vecs(), *wl1;
    wordlist *wl20 = p2->list_perm_vecs(), *wl2;

    // Throw out the ones that aren't in the arg list
    wordlist *wn;
    if (wl && !lstring::cieq(wl->wl_word, "all")) {    // Just in case
        for (wl1 = wl10; wl1; wl1 = wn) {
            wn = wl1->wl_next;
            wordlist *tw;
            for (tw = wl; tw; tw = tw->wl_next) {
                if (nameeq(wl1->wl_word, tw->wl_word))
                    break;
            }
            if (!tw) {
                if (!wl1->wl_prev) {
                    wl10 = wn;
                    wl10->wl_prev = 0;
                }
                else {
                    wl1->wl_prev->wl_next = wn;
                    if (wn)
                        wn->wl_prev = wl1->wl_prev;
                }
                delete [] wl1->wl_word;
                delete wl1;
            }
        }
        for (wl2 = wl20; wl2; wl2 = wn) {
            wn = wl2->wl_next;
            wordlist *tw;
            for (tw = wl; tw; tw = tw->wl_next) {
                if (nameeq(wl2->wl_word, tw->wl_word))
                    break;
            }
            if (!tw) {
                if (!wl2->wl_prev) {
                    wl20 = wn;
                    wl20->wl_prev = 0;
                }
                else {
                    wl2->wl_prev->wl_next = wn;
                    if (wn)
                        wn->wl_prev = wl2->wl_prev;
                }
                delete [] wl2->wl_word;
                delete wl2;
            }
        }
    }

    // rid anything in 1 not in 2
    for (wl1 = wl10; wl1; wl1 = wn) {
        wn = wl1->wl_next;
        if (!inwlist(wl1->wl_word,wl20)) {
            TTY.printf(">>> vector %s in %s not in %s\n",
                wl1->wl_word, p1->type_name(), p2->type_name());
            if (!wl1->wl_prev) {
                wl10 = wn;
                wl10->wl_prev = 0;
            }
            else {
                wl1->wl_prev->wl_next = wn;
                if (wn)
                    wn->wl_prev = wl1->wl_prev;
            }
            delete wl1->wl_word;
            delete wl1;
        }
    }

    // rid 2, warn first if not in 1
    for (wl2 = wl20; wl2; wl2 = wl2->wl_next) {
        if (!inwlist(wl2->wl_word,wl10)) {
            TTY.printf(">>> vector %s in %s not in %s\n",
                wl2->wl_word, p2->type_name(), p1->type_name());
        }
    }
    wordlist::destroy(wl20);

    dvlist2 *dl0 = 0;
    for (wl1 = wl10; wl1; wl1 = wl1->wl_next) {
        sDataVec *v1 = p1->get_perm_vec(wl1->wl_word);
        sDataVec *v2 = p2->get_perm_vec(wl1->wl_word);
        dvlist2 *dl = 0;
        if (v1->isreal() == v2->isreal() && *v1->units() == *v2->units()) {
            if (!dl0)
                dl0 = dl = new dvlist2;
            else {
                dl->d2_next = new dvlist2;
                dl = dl->d2_next;
            }
            dl->d2_dv1 = v1;
            dl->d2_dv2 = v2;
        }
        else {
            GRpkg::self()->ErrPrintf(ET_ERROR, "vector %s type mismatch.\n",
                wl1->wl_word);
        }
    }
    wordlist::destroy(wl10);

    
    // Now we have all the vectors linked to their twins.  Travel
    // down each one and print values that differ enough.
    //
    dvlist2 *dl;
    for (dl = dl0; dl; dl = dl->d2_next) {
        sDataVec *v1 = dl->d2_dv1;
        sDataVec *v2 = dl->d2_dv2;

        double tol;
        if (*v1->units() == UU_VOLTAGE)
            tol = vntol;
        else
            tol = abstol;
        int j = SPMAX(v1->length(), v2->length());
        for (int i = 0; i < j; i++) {
            if (v1->length() <= i) {
                TTY.printf( 
                    ">>> %s is %d long in %s and %d long in %s\n",
                    v1->name(), v1->length(),
                    p1->type_name(), v2->length(), p2->type_name());
                break;
            }
            else if (v2->length() <= i) {
                TTY.printf( 
                    ">>> %s is %d long in %s and %d long in %s\n",
                    v2->name(), v2->length(),
                    p2->type_name(), v1->length(), p1->type_name());
                break;
            }
            else {
                if (v1->isreal()) {
                    double d1 = v1->realval(i);
                    double d2 = v2->realval(i);
                    if (SPMAX(fabs(d1), fabs(d2)) * reltol +
                            tol < fabs(d1 - d2)) {
                        TTY.printf(
                            "%s.%s[%d] = %-15s ",
                            p1->type_name(), v1->name(), i, SPnum.printnum(d1));
                        TTY.printf(
                            "%s.%s[%d] = %s\n",
                            p2->type_name(), v2->name(), i, SPnum.printnum(d2));
                    }
                }
                else {
                    complex c1 = v1->compval(i);
                    complex c2 = v2->compval(i);
                    complex c3;
                    c3.real = c1.real - c2.real;
                    c3.imag = c1.imag - c2.imag;
                    double cm1 = c1.mag();
                    double cm2 = c2.mag();
                    double cmax = SPMAX(cm1, cm2);
                    if (cmax * reltol + tol < c3.mag()) {
                        TTY.printf(
                            "%s.%s[%d] = %-10s, ", p1->type_name(), v1->name(),
                            i, SPnum.printnum(c1.real));
                        TTY.printf("%-10s ", SPnum.printnum(c1.imag));
                        TTY.printf(
                            "%s.%s[%d] = %-10s, ", p2->type_name(), v2->name(),
                            i, SPnum.printnum(c2.real));
                        TTY.printf("%-10s\n", SPnum.printnum(c2.imag));
                    }
                }
            }
        }
    }
    for (; dl0; dl0 = dl) {
        dl = dl0->d2_next;
        delete dl0;
    }
}
// End of CommandTab functions.


namespace {
    // Determine if two vectors have the 'same' name.
    //
    bool nameeq(const char *n1, const char *n2)
    {
        if (lstring::eq(n1, n2))
            return (true);
        char buf1[BSIZE_SP];
        if (isdigit(*n1))
            sprintf(buf1, "v(%s)", n1);
        else
            strcpy(buf1, n1);
        char buf2[BSIZE_SP];
        if (isdigit(*n2))
            sprintf(buf2, "v(%s)", n2);
        else
            strcpy(buf2, n2);
        int i;
        for (i = 0; buf1[i]; i++)
            if (isupper(buf1[i]))
                buf1[i] = tolower(buf1[i]);
        for (i = 0; buf2[i]; i++)
            if (isupper(buf2[i]))
                buf2[i] = tolower(buf2[i]);
        return (lstring::eq(buf1, buf2));
    }


    bool inwlist(const char *word, wordlist *wl)
    {
        while (wl) {
            if (lstring::eq(word,wl->wl_word))
                return (true);
            wl = wl->wl_next;
        }
        return (false);
    }
}


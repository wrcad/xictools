
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

#include "simulator.h"
#include "graph.h"
#include "output.h"
#include "trnames.h"


// Soft-wired names for trial specification.  These can be overridden
// by a variable of the same name.
//
const char *check_value1 = "value1";
const char *check_value2 = "value2";
const char *check_value = "value";
const char *checkN1 = "checkN1";
const char *checkN2 = "checkN2";


// Static Function.
// Allocate a name struct, set the names of the input variables.
//
sNames *
sNames::set_names()
{
    sNames *n = new sNames;
    variable *vv = Sp.GetRawVar(check_value1);
    if (vv && vv->type() == VTYP_STRING)
        n->set_value1(vv->string());
    else
        n->set_value1(check_value1);

    vv = Sp.GetRawVar(check_value2);
    if (vv && vv->type() == VTYP_STRING)
        n->set_value2(vv->string());
    else
        n->set_value2(check_value2);

    vv = Sp.GetRawVar(check_value);
    if (vv && vv->type() == VTYP_STRING)
        n->set_value(vv->string());
    else
        n->set_value(check_value);

    char buf[32];
    vv = Sp.GetRawVar(checkN1);
    if (vv && vv->type() == VTYP_STRING)
        n->set_n1(vv->string());
    else if (vv && vv->type() == VTYP_NUM) {
        snprintf(buf, sizeof(buf), "%d", vv->integer());
        n->set_n1(buf);
    }
    else if (vv && vv->type() == VTYP_REAL) {
        snprintf(buf, sizeof(buf), "%d", rnd(vv->real()));
        n->set_n1(buf);
    }
    else
        n->set_n1(checkN1);

    vv = Sp.GetRawVar(checkN2);
    if (vv && vv->type() == VTYP_STRING)
        n->set_n2(vv->string());
    else if (vv && vv->type() == VTYP_NUM) {
        snprintf(buf, sizeof(buf), "%d", vv->integer());
        n->set_n2(buf);
    }
    else if (vv && vv->type() == VTYP_REAL) {
        snprintf(buf, sizeof(buf), "%d", rnd(vv->real()));
        n->set_n2(buf);
    }
    else
        n->set_n2(checkN2);

    return (n);
}


namespace {
    bool is_int(const char *str)
    {
        if (str && isdigit(*str)) {
            str++;
            while (*str) {
                if (!isdigit(*str))
                    return (false);
                str++;
            }
            return (true);
        }
        return (false);
    }
}


// Set the input variables to the trial values.
//
void
sNames::set_input(sFtCirc *out_cir, sPlot *out_plot, double v1, double v2)
{
    sFtCirc *cir = Sp.CurCircuit();
    sPlot *plt = OP.curPlot();
    Sp.SetCurCircuit(out_cir);
    OP.setCurPlot(out_plot);

    out_cir->set_use_trial_deferred(true);
    Sp.SetVar(n_value1, v1);
    Sp.SetVar(n_value2, v2);
    out_cir->set_use_trial_deferred(false);

    sDataVec *d = out_plot->find_vec(n_value);
    if (d && d->isreal()) {
        int ix1 = -1, ix2 = -1;
        if (is_int(n_n1))
            ix1 = atoi(n_n1);
        else {
            sDataVec *dn1 = out_plot->find_vec(n_n1);
            if (dn1 && dn1->isreal())
                ix1 = (int)dn1->realval(0);
        }
        if (is_int(n_n2))
            ix2 = atoi(n_n2);
        else {
            sDataVec *dn2 = out_plot->find_vec(n_n2);
            if (dn2 && dn2->isreal())
                ix2 = (int)dn2->realval(0);
        }

        // resize if necessary
        int nx = SPMAX(ix1, ix2);
        if (nx > d->length())
            d->resize(nx+1);

        // N1 has precedence if N1 = N2
        if (ix2 >= 0)
            d->set_realval(ix2, v2);
        if (ix1 >= 0)
            d->set_realval(ix1, v1);
    }
    Sp.SetCurCircuit(cir);
    OP.setCurPlot(plt);
}


// Copy the "value" vectors from the previous plot to the current plot.
// Skip if the previous plot is the constants plot, the vectors are
// available from there by name anyway.
//
void
sNames::setup_newplot(sFtCirc *out_cir, sPlot *out_plot)
{
    if (!out_plot)
        out_plot = OP.constants();
    if (out_plot == OP.constants())
        return;
    sFtCirc *cir = Sp.CurCircuit();
    sPlot *plt = OP.curPlot();
    Sp.SetCurCircuit(out_cir);
    OP.setCurPlot(out_plot);
    sDataVec *dvalue = OP.vecGet(n_value, 0);
    sDataVec *dn1 = 0, *dn2 = 0;
    if (dvalue && dvalue->isreal()) {
        dn1 = OP.vecGet(n_n1, 0);
        dn2 = OP.vecGet(n_n2, 0);
    }
    Sp.SetCurCircuit(cir);
    OP.setCurPlot(plt);
    if (dvalue && dvalue->isreal() && dvalue->plot()) {
        char buf[BSIZE_SP];
        snprintf(buf, sizeof(buf), "%s%c%s", dvalue->plot()->type_name(),
            Sp.PlotCatchar(), n_value);
        OP.vecSet(n_value, buf);
        if (dn1 && dn1->isreal() && dn1->plot()) {
            snprintf(buf, sizeof(buf), "%s%c%s", dn1->plot()->type_name(),
                Sp.PlotCatchar(), n_n1);
            OP.vecSet(n_n1, buf);
        }
        if (dn2 && dn2->isreal() && dn2->plot()) {
            snprintf(buf, sizeof(buf), "%s%c%s", dn2->plot()->type_name(),
                Sp.PlotCatchar(), n_n2);
            OP.vecSet(n_n2, buf);
        }
    }
}


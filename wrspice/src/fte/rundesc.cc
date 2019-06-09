
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
Authors: 1988 Wayne A. Christopher
         1992 Stephen R. Whiteley
****************************************************************************/

#include "config.h"
#include "graph.h"
#include "cshell.h"
#include "simulator.h"
#include "rawfile.h"
#include "csdffile.h"
#include "psffile.h"
#include "runop.h"
#include "output.h"
#include "device.h"
#include "rundesc.h"
#include "toolbar.h"
#include "noisdefs.h"
#include "tfdefs.h"
#include "sensdefs.h"
#include <limits.h>

//
// Functions for low-level output control.
//

// Amount to increase size of vectors as they grow dynamically
#define SIZE_INCR 10


// The plot maintenance functions.
//
void
sRunDesc::plotInit(double tstart, double tstop, double tstep, sPlot *plot)
{
    ToolBar()->UpdatePlots(1);
    if (plot == 0) {
        rd_runPlot = new sPlot(rd_type);
        rd_runPlot->new_plot();
        OP.setCurPlot(rd_runPlot->type_name());
        // add some interface hooks
        rd_runPlot->set_circuit(rd_circ->name());
        rd_runPlot->set_options(rd_ckt->CKTcurTask->TSKshellOpts->copy());
    }
    else
        rd_runPlot = plot;
    rd_runPlot->set_title(rd_name);    // Circuit name.
    rd_runPlot->set_name(rd_title);    // Descriptive name.
    rd_runPlot->set_num_dimensions(0);
    rd_runPlot->set_range(tstart, tstop, tstep);

    // Below is bad.  Somehow we need to figure out the units for the
    // vectors.  It would be great to have the caller specify units
    // for each vector, however this would be an invasive change that
    // ripples back into the device models (for noise).

    if (lstring::cieq((const char*)rd_job->JOBname, "noise")) {
        // Noise analysis.  Vector name forms are
        //  xxx:dens,  xxx:dens@yyy
        //  xxx:tot,   xxx.tot@yyy
        // xxx is node or branch name.
        // yyy is voltage or current source name.
        //  onoise.zzz
        //  inoise.zzz
        // zzz is model/circuit dependent
        // The onoise/inoise forms are assigned in the device models. 
        // There is no guarantee that an imported model will follow
        // this convention.

        bool is_spectrum = false;
        if (lstring::cisubstring("Spectrum", rd_title))
            is_spectrum = true;
        bool in_is_cur = false;
        bool out_is_cur = false;
        sNOISEAN *an = (sNOISEAN*)rd_job;
        if (an->Ninput) {
            // Name of input source, must exist.
            if (*an->Ninput == 'i' || *an->Ninput == 'I')
                in_is_cur = true;
        }
        if (!an->NoutputRef && an->Noutput) {
            if (lstring::substring("#branch", an->Noutput))
                out_is_cur = true;
        }
        for (int i = 0; i < rd_numData; i++) {
            dataDesc *dd = rd_data + i;
            sDataVec *v = new sDataVec;
            if (lstring::cieq(dd->name, "frequency"))
                v->units()->set(UU_FREQUENCY);
            else if (dd->regular) {
                if (lstring::ciprefix("inoise", dd->name)) {
                    if (in_is_cur) {
                        if (is_spectrum)
                            v->units()->set("AAS");  // A^2/Hz
                        else
                            v->units()->set("AA");
                    }
                    else {
                        if (is_spectrum)
                            v->units()->set("VVS");  // V^2/Hz
                        else
                            v->units()->set("VV");
                    }
                }
                else if (lstring::ciprefix("onoise", dd->name)) {
                    if (out_is_cur) {
                        if (is_spectrum)
                            v->units()->set("AAS");  // A^2/Hz
                        else
                            v->units()->set("AA");
                    }
                    else {
                        if (is_spectrum)
                            v->units()->set("VVS");  // V^2/Hz
                        else
                            v->units()->set("VV");
                    }
                }
                else if (strchr(dd->name, Sp.SpecCatchar())) {
                    // Found the '@' in, e.g., dens@YYY
                    if (in_is_cur) {
                        if (is_spectrum)
                            v->units()->set("AAS");  // A^2/Hz
                        else
                            v->units()->set("AA");
                    }
                    else {
                        if (is_spectrum)
                            v->units()->set("VVS");  // V^2/Hz
                        else
                            v->units()->set("VV");
                    }
                }
                else if (lstring::cisubstring("dens", dd->name) ||
                        lstring::cisubstring("tot", dd->name)) {
                    if (out_is_cur) {
                        if (is_spectrum)
                            v->units()->set("AAS");  // A^2/Hz
                        else
                            v->units()->set("AA");
                    }
                    else {
                        if (is_spectrum)
                            v->units()->set("VVS");  // V^2/Hz
                        else
                            v->units()->set("VV");
                    }
                }
            }
            v->set_name(dd->name);
            v->set_scale(0);
            v->set_length(0);
            if (rd_isComplex)
                v->set_flags(VF_COMPLEX);
            else
                v->set_flags(0);
            v->alloc(!rd_isComplex, rd_numPoints*rd_cycles);
            v->newperm();
            dd->vec = v;
        }
    }
    else if (lstring::cieq((char*)rd_job->JOBname, "tf")) {
        sTFAN *an = (sTFAN*)rd_job;
        for (int i = 0; i < rd_numData; i++) {
            dataDesc *dd = rd_data + i;
            sDataVec *v = new sDataVec;
            if (lstring::cieq(dd->name, "frequency"))
                v->units()->set(UU_FREQUENCY);
            else if (dd->regular) {
                if (lstring::cieq(dd->name, "tranfunc")) {
                    if (an->TFinIsI && an->TFoutIsV)
                        v->units()->set(UU_RES);
                    else if (an->TFinIsV && an->TFoutIsI)
                        v->units()->set(UU_COND);
                }
                else if (lstring::cisubstring("Zi", dd->name) ||
                        lstring::cisubstring("Zo", dd->name))
                    v->units()->set(UU_RES);
                else if (lstring::cisubstring("isweep", dd->name))
                    v->units()->set(UU_CURRENT);
                else if (lstring::cisubstring("vsweep", dd->name))
                    v->units()->set(UU_VOLTAGE);
            }
            v->set_name(dd->name);
            v->set_scale(0);
            v->set_length(0);
            if (rd_isComplex)
                v->set_flags(VF_COMPLEX);
            else
                v->set_flags(0);
            v->alloc(!rd_isComplex, rd_numPoints*rd_cycles);
            v->newperm();
            dd->vec = v;
        }
    }
    else if (lstring::cieq((char*)rd_job->JOBname, "sens")) {
        sSENSAN *an = (sSENSAN*)rd_job;
        for (int i = 0; i < rd_numData; i++) {
            dataDesc *dd = rd_data + i;
            sDataVec *v = new sDataVec;
            if (lstring::cieq(dd->name, "frequency"))
                v->units()->set(UU_FREQUENCY);
            else if (dd->regular) {
                if (lstring::cisubstring("sweep", dd->name)) {
                    if (an->SENSoutSrc)
                        v->units()->set(UU_CURRENT);
                    else
                        v->units()->set(UU_VOLTAGE);
                }
            }
            v->set_name(dd->name);
            v->set_scale(0);
            v->set_length(0);
            if (rd_isComplex)
                v->set_flags(VF_COMPLEX);
            else
                v->set_flags(0);
            v->alloc(!rd_isComplex, rd_numPoints*rd_cycles);
            v->newperm();
            dd->vec = v;
        }
    }
    else {
        for (int i = 0; i < rd_numData; i++) {
            char buf[100];
            dataDesc *dd = rd_data + i;
            sDataVec *v = new sDataVec;

            const char *s;
            if (lstring::substring("#branch", dd->name) ||
                    lstring::cieq(dd->name, "isweep") ||
                    ((s = strchr(dd->name, '#')) != 0 && *(s+1) == 'i'))
                v->units()->set(UU_CURRENT);
            else if (lstring::cieq(dd->name, "time"))
                v->units()->set(UU_TIME);
            else if (lstring::cieq(dd->name, "frequency"))
                v->units()->set(UU_FREQUENCY);
            else if (dd->regular) {
                if (!strrchr(dd->name, '(')) {
                    v->units()->set(UU_VOLTAGE);
                    sprintf(buf, "v(%s)", dd->name);
                    v->set_name(buf);
                }
            }
            if (!v->name())
                v->set_name(dd->name);
            v->set_scale(0);
            v->set_length(0);
            if (rd_isComplex)
                v->set_flags(VF_COMPLEX);
            else
                v->set_flags(0);
            if (lstring::ciprefix("pole(", v->name()))
                v->set_flags(v->flags() | VF_POLE);
            else if (lstring::ciprefix("zero(", v->name()))
                v->set_flags(v->flags() | VF_ZERO);
            v->alloc(!rd_isComplex, rd_numPoints*rd_cycles);
            v->newperm();
            dd->vec = v;
        }
    }
    if (rd_refIndex >= 0)
        rd_runPlot->set_scale(rd_data[rd_refIndex].vec);
    ToolBar()->UpdatePlots(1);
}


// Add point, increment dvec length if inc is true.
//
void
sRunDesc::addPointToPlot(IFvalue *refValue, IFvalue *valuePtr, bool inc)
{
    for (int i = 0; i < rd_numData; i++) {
        if (rd_data[i].regular) {
            if (rd_data[i].outIndex == -1) {
                if (rd_data[i].type == IF_REAL)
                    rd_data[i].addRealValue(refValue->rValue, inc);
                else if (rd_data[i].type == IF_COMPLEX)
                    rd_data[i].addComplexValue(refValue->cValue, inc);
            }
            else {
                if (rd_data[i].type == IF_REAL)
                    rd_data[i].addRealValue(
                        valuePtr->v.vec.rVec[rd_data[i].outIndex], inc);
                else if (rd_data[i].type == IF_COMPLEX)
                    rd_data[i].addComplexValue(
                        valuePtr->v.vec.cVec[rd_data[i].outIndex], inc);
            }
        }
        else {
            // should pre-check instance
            IFvalue val;
            if (!getSpecial(rd_ckt, i, &val))
                continue;
            if (rd_data[i].type == IF_REAL)
                rd_data[i].addRealValue(val.rValue, inc);
            else if (rd_data[i].type == IF_COMPLEX)
                rd_data[i].addComplexValue(val.cValue, inc);
            else 
                GRpkgIf()->ErrPrintf(ET_INTERR,
                    "unsupported data type in plot.\n");
        }
    }
    if (rd_segfilebase && inc && refValue->rValue > rd_seglimit)
        dumpSegment();
}


void
sRunDesc::pushPointToPlot(sCKT *ckt, IFvalue *refValue, IFvalue *valuePtr,
    unsigned int indx)
{
    for (int i = 0; i < rd_numData; i++) {
        if (rd_data[i].regular) {
            if (rd_data[i].outIndex == -1) {
                if (rd_data[i].type == IF_REAL)
                    rd_data[i].pushRealValue(refValue->rValue, indx);
                else if (rd_data[i].type == IF_COMPLEX)
                    rd_data[i].pushComplexValue(refValue->cValue, indx);
            }
            else {
                if (rd_data[i].type == IF_REAL)
                    rd_data[i].pushRealValue(
                        valuePtr->v.vec.rVec[rd_data[i].outIndex], indx);
                else if (rd_data[i].type == IF_COMPLEX)
                    rd_data[i].pushComplexValue(
                        valuePtr->v.vec.cVec[rd_data[i].outIndex], indx);
            }
        }
        else {
            // should pre-check instance
            IFvalue val;
            if (!getSpecial(ckt, i, &val))
                continue;
            if (rd_data[i].type == IF_REAL)
                rd_data[i].pushRealValue(val.rValue, indx);
            else if (rd_data[i].type == IF_COMPLEX)
                rd_data[i].pushComplexValue(val.cValue, indx);
            else 
                GRpkgIf()->ErrPrintf(ET_INTERR,
                    "unsupported data type in plot.\n");
        }
    }
}


void
sRunDesc::setupSegments(const char *basename, double delta, sOUTdata *outd)
{
    // in a chained analysis, it is legitimate to set delta = 0, in
    // which case delta is one cycle
    if (delta <= 0.0) {
        if (rd_cycles > 1 || (rd_check && rd_check->out_mode == OutcCheckSeg))
            rd_segdelta = outd->finalValue - outd->step/2;
        else
            return;
    }
    else
        rd_segdelta = delta;
    rd_segfilebase = lstring::copy(basename);
    rd_seglimit = rd_segdelta;
    rd_segindex = 0;
}


void
sRunDesc::dumpSegment()
{
    if (rd_pointCount) {
        if (rd_segfilebase && !rd_rd) {
            char buf[128];
            sprintf(buf, "%s.s%02d", rd_segfilebase, rd_segindex);
            bool use_csdf = false;
            if (use_csdf) {
                cCSDFout csdf(rd_runPlot);
                csdf.file_write(buf, false);
            }
            else {
                cRawOut raw(rd_runPlot);
                raw.file_write(buf, false);
            }
            if (rd_cycles == 1)
                rd_seglimit += rd_segdelta;
            rd_segindex++;
        }
        rd_pointCount = 0;
        resetVecs();
    }
}


// Reset the length of the data vectors to 0.
//
void
sRunDesc::resetVecs()
{
    for (int i = 0; i < rd_numData; i++) {
        rd_data[i].vec->set_length(0);
        rd_data[i].sp.reset();
    }
}


void
sRunDesc::plotEnd()
{
    ToolBar()->UpdatePlots(0);
    if (Sp.RunCircuit() && Sp.RunCircuit()->runplot()) {
        Sp.RunCircuit()->runplot()->set_active(false);
        Sp.RunCircuit()->set_runplot(0);
    }
    if (rd_runPlot)
        rd_runPlot->set_active(false);
}


bool
sRunDesc::datasize()
{
    // if any of them are complex, make them all complex
    rd_isComplex = 0;
    for (int i = 0; i < rd_numData; i++) {
        if (rd_data[i].type == IF_COMPLEX) {
            rd_isComplex = 1;
            break;
        }
    }
    double mysize = (sizeof(double) * rd_numPoints * rd_numData *
        rd_cycles * (rd_isComplex + 1))/1000.0;
    double maxdata = DEF_maxData;

    VTvalue vv;
    if (Sp.GetVar("maxdata", VTYP_REAL, &vv, rd_circ))
        maxdata = vv.get_real();

    if (mysize > maxdata) {
        GRpkgIf()->ErrPrintf(ET_ERROR,
            "analysis would use %.1fKB which exceeds the maximum %gKB.\n"
            "Set \"maxdata\" to alter limit.\n", mysize, maxdata);
        return (true);
    }
    rd_maxPts = 1 + 
        (int)(maxdata*1000/(sizeof(double) * rd_numData * (rd_isComplex+1)));
    if (rd_maxPts < 0)
        rd_maxPts = INT_MAX;
    return (false);
}


void
sRunDesc::unrollVecs()
{
    for (int k = 0; k < rd_numData; k++) {
        dataDesc *d = rd_data + k;
        sDataVec *v = d->vec;
        if (v) {
            if (v->flags() & VF_ROLLOVER) {
                if (v->isreal()) {
                    double *r = new double[v->allocated()];
                    int i, j = 0;;
                    for (i = v->length(); i < v->allocated(); i++)
                        r[j++] = v->realval(i);
                    for (i = 0; i < v->length(); i++)
                        r[j++] = v->realval(i);
                    v->set_length(v->allocated());
                    v->set_realvec(r, true);
                }
                else {
                    complex *c = new complex[v->allocated()];
                    int i, j = 0;;
                    for (i = v->length(); i < v->allocated(); i++)
                        c[j++] = v->compval(i);
                    for (i = 0; i < v->length(); i++)
                        c[j++] = v->compval(i);
                    v->set_length(v->allocated());
                    v->set_compvec(c, true);
                }
                v->set_flags(v->flags() & ~VF_ROLLOVER);
            }
        }
    }
}


// Temporarily convert all the vectors to unit length, pointing
// to the latest value.  Then we can use the vector expression
// parser efficiently in the runops.
//
void
sRunDesc::scalarizeVecs()
{
    for (int k = 0; k < rd_numData; k++) {
        sDataVec *v = rd_data[k].vec;
        if (v)
            v->scalarize();
    }
}


void
sRunDesc::unscalarizeVecs()
{
    for (int k = 0; k < rd_numData; k++) {
        sDataVec *v = rd_data[k].vec;
        if (v)
            v->unscalarize();
    }
}


// Temporarily convert multi-dimensional vectors to single dimensional
// vectors of the last segment.
//
void
sRunDesc::segmentizeVecs()
{
    for (int k = 0; k < rd_numData; k++) {
        sDataVec *v = rd_data[k].vec;
        if (v)
            v->segmentize();
    }
}


void
sRunDesc::unsegmentizeVecs()
{
    for (int k = 0; k < rd_numData; k++) {
        sDataVec *v = rd_data[k].vec;
        if (v)
            v->unsegmentize();
    }
}


// dataDesc growth block size.
#define DATA_GROW 1024

int
sRunDesc::addDataDesc(const char *nm, int typ, int ind)
{
    if (rd_numData == rd_dataSize) {
        dataDesc *d = new dataDesc[rd_dataSize + DATA_GROW];
        if (rd_data) {
            memcpy(d, rd_data, rd_dataSize * sizeof(dataDesc));
            delete [] rd_data;
        }
        rd_data = d;
        rd_dataSize += DATA_GROW;
    }

    dataDesc *d = rd_data + rd_numData;
    *d = dataDesc();

    d->name = lstring::copy(nm);
    d->type = typ & IF_VARTYPES;
    d->gtype = GRID_LIN;
    d->regular = true;
    d->outIndex = ind;
    if (rd_scrolling && rd_numPoints > 1)
        d->rollover_ok = true;

    if (ind == -1) {
        // It's the reference vector
        rd_refIndex = rd_numData;
    }
    rd_numData++;
    return (OK);
}


int
sRunDesc::addSpecialDesc(const char *nm, int depind, bool usevec)
{
    if (rd_numData == rd_dataSize) {
        dataDesc *d = new dataDesc[rd_dataSize + DATA_GROW];
        if (rd_data) {
            memcpy(d, rd_data, rd_dataSize * sizeof(dataDesc));
            delete [] rd_data;
        }
        rd_data = d;
        rd_dataSize += DATA_GROW;
    }

    dataDesc *d = rd_data + rd_numData;
    *d = dataDesc();

    d->name = lstring::copy(nm);
    d->useVecIndx = usevec;
    d->outIndex = depind;
    d->regular = false;
    if (rd_scrolling && rd_numPoints > 1)
        d->rollover_ok = true;
    rd_numData++;
    return (OK);
}


bool
sRunDesc::getSpecial(sCKT *ckt, int indx, IFvalue *val)
{
    dataDesc *desc = rd_data + indx;
    if (desc->sp.sp_error == OK) {
        int i;
        // I don't know if this has any usefulness, i.e., using another
        // vector to index the (list type) special parameter.
        if (desc->useVecIndx) {
            if (desc->outIndex == -1) {
                dataDesc *dd = rd_data + rd_refIndex;
                i = (int)dd->vec->realval(dd->vec->length() - 1);
            }
            else {
                dataDesc *dd = rd_data + desc->outIndex;
                i = (int)dd->vec->realval(dd->vec->length() - 1);
            }
        }
        else
            i = desc->outIndex;
        IFdata d;
        bool set_vtype = desc->sp.sp_isset ? false : true;
        desc->sp.evaluate(desc->name, ckt, &d, i);
        if (desc->sp.sp_error == OK) {
            if ((d.type & IF_VARTYPES) == IF_INTEGER) {
                desc->type = IF_REAL;
                val->rValue = (double)d.v.iValue;
            }
            else if ((d.type & IF_VARTYPES) == IF_REAL ||
                    (d.type & IF_VARTYPES) ==  IF_COMPLEX) {
                desc->type = d.type & IF_VARTYPES;
                *val = d.v;
            }
            else
                return (false);
            // The data vecs were created before we had the units,
            // so set the units on the first pass thru here
            if (set_vtype)
                desc->vec->units()->set(d.toUU());
            return (true);
        }
        else {
            OP.error(ERR_WARNING, "can not evaluate %s.", desc->name);
            rd_runPlot->remove_vec(desc->name);
            desc->vec = 0;
        }
    }
    return (false);
}


// Not inlined to avoid use of sCKT in include file.
void
sRunDesc::setCkt(sCKT *c)
{
    rd_ckt = c;
    rd_circ = c->CKTbackPtr;
}
// End of sRunDesc functions.


void
dataDesc::addRealValue(double value, bool inc)
{
    if (!inc)
        vec->set_length(0);
    pushRealValue(value, vec->length());
}


void
dataDesc::pushRealValue(double value, unsigned int indx)
{
    unsigned int vl = vec->length();
    if (indx >= vl)
        vl = indx + 1;
    if (vl > (unsigned)vec->allocated())
        vec->resize(vl + SIZE_INCR);
    vec->set_length(vl);
    if (vec->isreal())
        vec->set_realval(indx, value);
    else {
        vec->set_realval(indx, value);
        vec->set_imagval(indx, 0.0);
    }
}


void
dataDesc::addComplexValue(IFcomplex value, bool inc)
{
    if (!inc)
        vec->set_length(0);
    else if (vec->length() == 0 && !(vec->flags() & VF_COMPLEX)) {
        delete [] vec->realvec();
        vec->alloc(false, vec->allocated());
        vec->set_flags(vec->flags() | VF_COMPLEX);
    }
    pushComplexValue(value, vec->length());
}


void
dataDesc::pushComplexValue(IFcomplex value, unsigned int indx)
{
    if (indx >= (unsigned int)vec->length())
        vec->set_length(indx+1);
    if (vec->length() > vec->allocated())
        vec->resize(vec->length() + SIZE_INCR);
    if (vec->isreal())
        vec->set_realval(indx, value.real);
    else {
        vec->set_realval(indx, value.real);
        vec->set_imagval(indx, value.imag);
    }
}
// End of dataDesc methods.


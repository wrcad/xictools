
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
#include "simulator.h"
#include "rawfile.h"
#include "csdffile.h"
#include "psffile.h"
#include "runop.h"
#include "output.h"
#include "device.h"
#include "rundesc.h"
#include "aspice.h"
#include "toolbar.h"
#include "spnumber/hash.h"
#include "miscutil/pathlist.h"
#include <limits.h>
#ifdef HAVE_SECURE
#include <signal.h>
#include <unistd.h>
extern int StateInitialized;  // security
#ifdef WIN32
#include "miscutil/msw.h"
#endif
#endif
#ifdef WIN32
#include <libiberty.h>  // provides vasprintf
#endif
#include <stdarg.h>


//
// Main functions for dumping simulation output.
//

namespace {
    // Where 'constants' go when defined on initialization.
    //
    struct sConstPlot : public sPlot
    {
        sConstPlot() : sPlot(0) {
            set_title("Constant values");
            set_date(datestring());
            set_name("constants");
            set_type_name("constants");
            set_written(true);
        }
    };
    sConstPlot constplot;
}


IFoutput::IFoutput()
{
    o_runops        = new sRunopDb;
    o_endit         = false;
    o_shouldstop    = false;

    o_plot_cur      = &constplot;
    o_plot_list     = &constplot;
    o_plot_cx       = 0;
    o_cxplots       = 0;
    o_constants     = &constplot;

    o_jobc          = new sJobc;

    o_outfile.set_outFile("rawspice.raw");
}

namespace {
    // ParseSpecial takes something of the form "@name[param, index]" or
    // equivalently "@name[param][index] and rips out index.
    //
    bool parseSpecial(const char *name, char *ind)
    {
        *ind = '\0';
        if (*name != Sp.SpecCatchar())
            return (false);
        name++;
        
        while (*name && *name != '[')
            name++;
        if (!*name)
            return (true);
        name++;

        while (*name && *name != ',' && *name != ']')
            name++;
        if (*name == ']') {
            name++;
            if (!*name)
                return (true);
            if (*name != '[')
                return (false);
            name++;
        }
        else if (*name == ',')
            name++;
        else
            return (false);

        char *s = ind;
        while (*name && *name != ']') {
            if (!isspace(*name))
                *s++ = *name;
            name++;
        }
        *s = '\0';
        if (*name && !name[1])
            return (true);
        else
            return (false);
    }


    char *name_tok(const char *str)
    {
        while (isspace(*str))
            str++;
        if (*str == 'v' || *str == 'V') {
            const char *s = str+1;
            while (isspace(*s))
                s++;
            if (*s == '(') {
                s++;
                while (isspace(*s))
                    s++;
                str = s;
            }
        }
        char *nm = lstring::copy(str);
        for (char *t = nm; *t; t++) {
            if (isspace(*t) || *t == ')') {
                *t = 0;
                break;
            }
        }
        return (nm);
    }
}


// Start pointwise output plot.
//
sRunDesc *
IFoutput::beginPlot(sOUTdata *outd, int multip,
    const char *segfilebase, double segwidth)
{
    // multip is the number of points in a chained dc analysis

    if (!outd || !outd->circuitPtr)
        return (0);
    sCKT *ckt = outd->circuitPtr;
    sFtCirc *circ = ckt->CKTbackPtr;

    sCHECKprms *chk = circ->check();
    sSWEEPprms *swp = 0;

    o_shouldstop = false;
    sRunDesc *run = 0;
    sPlot *plot = 0;
    if (chk) {
        chk->set_index(0);
        chk->set_failed(false);
        run = chk->out_rundesc;
        if (run)
            run->setCkt(ckt);  // this may have changed
        plot = chk->out_plot;
        if (chk->out_mode != OutcNormal && run) {
            // True if inside chained analysis
            if (chk->out_mode == OutcCheckSeg)
                run->dumpSegment();
            else if (chk->out_mode == OutcCheckMulti &&
                    run->numPoints() > 1) {
                int dims[2];
                // the numPoints is an estimate of the number of points
                // output, this should fix the value on the first pass
                if (run->pointCount() < run->numPoints())
                    run->set_numPoints(run->pointCount());
                run->set_dims(dims);
                setDims(run, dims, 2);
            }
            for (int i = 0; i < run->numData(); i++) {
                // clear these cached values, may be bogus
                run->data(i)->sp.sp_isset = false;
                run->data(i)->sp.sp_inst = 0;
                run->data(i)->sp.sp_mod = 0;
            }
            initRunops(run);
            return (run);
        }
        if (chk->out_mode == OutcCheckMulti)
            segfilebase = 0;
        else if (!segfilebase)
            segfilebase = chk->segbase();
        if (segfilebase && chk->out_mode == OutcCheck)
            chk->out_mode = OutcCheckSeg;
        if (chk->out_mode == OutcCheckSeg)
            segwidth = 0.0;
        else if (chk->out_mode == OutcCheckMulti)
            multip = chk->cycles();
    }
    else {
        swp = circ->sweep();
        if (swp) {
            run = swp->out_rundesc;
            if (run)
                run->setCkt(ckt);  // this may have changed
            plot = swp->out_plot;
            if (run) {
                for (int i = 0; i < run->numData(); i++) {
                    // clear these cached values, may be bogus
                    run->data(i)->sp.sp_isset = false;
                    run->data(i)->sp.sp_inst = 0;
                    run->data(i)->sp.sp_mod = 0;
                }
                initRunops(run);
                return (run);
            }
        }
    }

    if (!run) {
        run = new sRunDesc;
        run->set_check(chk);
        run->set_sweep(swp);
    }
    run->set_job(outd->analysisPtr);
    run->setCkt(ckt);
    run->set_name(run->circuit()->name());
    run->set_type((char*)outd->analName);
    run->set_cycles(multip);
    if (run->cycles() < 1)
        run->set_cycles(1);
    run->set_anType(ckt->CKTcurJob->JOBtype);
    run->set_scrolling(ckt->CKTmode & MODESCROLL);

    // If there is only one plot produced, the analName will be a simple
    // token like "TRAN".  Otherwise, the analType will have a form like
    // "DISTO: IM, (f1 - f2)".  If so, include this in the title.
    {
        const char *t = run->type();
        while (*t && !isspace(*t))
            t++;
        while (isspace(*t))
            t++;
        if (*t) {
            char buf[256];
            sprintf(buf, "%s: %s",
                IFanalysis::analysis(outd->analysisPtr->JOBtype)->description,
                t);
            run->set_title(buf);
        }
        else
            run->set_title(
                IFanalysis::analysis(outd->analysisPtr->JOBtype)->description);
    }

    if (outd->numPts <= 0 || (chk && chk->out_mode == OutcCheck))
        run->set_numPoints(1);
    else
        run->set_numPoints(outd->numPts);
    if (chk && chk->out_mode != OutcNormal)
        chk->out_rundesc = run;
    else if (swp)
        swp->out_rundesc = run;

    sSaveList saves;
    if (outd->analysisPtr && outd->analysisPtr->JOBname &&
            !lstring::cieq((char*)outd->analysisPtr->JOBname, "op")) {

        // When doing op analysis, ignore saves and output everything.
        run->circuit()->getSaves(&saves, ckt); // from .save's
        getSaves(run->circuit(), &saves);  // from front end
    }
    int numregular = 0;
    bool saveall = true;
    if (saves.numsaves()) {
        saveall = false;
        sHgen gen(saves.table());
        sHent *h;
        while ((h = gen.next()) != 0) {
            if (lstring::cieq(h->name(), "all")) {
                saveall = true;
                break;
            }
            if (*h->name() != Sp.SpecCatchar())
                numregular++;
        }
        if (!numregular)
            saveall = true;
    }

    // To avoid time-consuming searches, we use a couple of hash tables.
    // dataNameTab: all of the known data names.
    // outTab:      names of vectors to keep.
    // In both tables, the key string has any v(...) removed.  The
    // names must match whether or not this construct is present.

    sHtab dataNameTab(false);
    for (int j = 0; j < outd->numNames; j++) {
        char *nm = name_tok((const char*)outd->dataNames[j]);
        if (nm) {
            dataNameTab.add(nm, (void*)(unsigned long)(j+1));
            delete [] nm;
        }
    }

    sHtab outTab(false);

    // Add the scale vector and set its used flag.
    if (outd->refName) {
        char *nm = name_tok((const char*)outd->refName);
        if (nm) {
            run->addDataDesc((const char*)outd->refName, outd->refType, -1);
            outTab.add(nm, (void*)(long)(-1));
            saves.set_used((const char*)outd->refName, true);
            delete [] nm;
        }
        else
            run->set_refIndex(-1);
    }
    else
        run->set_refIndex(-1);

    // Pass 1
    if (saves.numsaves() && !saveall) {
        sHgen gen(saves.table());
        sHent *h;
        while ((h = gen.next()) != 0) {
            if (h->data())
                continue;
            char *nm = name_tok(h->name());
            if (!nm)
                continue;
            int j = (unsigned long)sHtab::get(&dataNameTab, nm);
            if (j && !sHtab::get(&outTab, nm))  {
                j--;
                run->addDataDesc((char*)outd->dataNames[j], outd->dataType, j);
                outTab.add(nm, (void*)(long)(j+1));
                h->set_data((void*)(long)true);
            }
            delete [] nm;
        }
    }
    else {
        char *nmref = 0;
        if (outd->refName)
            nmref = name_tok((const char*)outd->refName);
        for (int j = 0; j < outd->numNames; j++) {
            char *nm = name_tok((char*)outd->dataNames[j]);
            if (!nm)
                continue;
            if (nmref && lstring::eq(nm, nmref)) {
                delete [] nm;
                continue;
            }
            if (!sHtab::get(&outTab, nm)) {
                run->addDataDesc((char*)outd->dataNames[j], outd->dataType, j);
                outTab.add(nm, (void*)(long)(j+1));
            }
            delete [] nm;
        }
        delete [] nmref;
    }

    // Pass 2
    {
        sHgen gen(saves.table());
        sHent *h;
        while ((h = gen.next()) != 0) {
            if (h->data())
                continue;
            char depbuf[BSIZE_SP];
            if (*h->name() != Sp.SpecCatchar())
                continue;
            if (!parseSpecial(h->name(), depbuf)) {
                GRpkgIf()->ErrPrintf(ET_WARN, "can't parse '%s': ignored.\n",
                    h->name());
                continue;
            }
            // Now, if there's a dep variable, do we already have it?
            int depind = 0;
            bool usevec = false;
            if (*depbuf) {
                for (char *s = depbuf; *s; s++) {
                    if (!isdigit(*s)) {
                        usevec = true;
                        break;
                    }
                }
                if (usevec) {
                    char *nm = name_tok(depbuf);
                    int j = (long)sHtab::get(&outTab, nm);
                    if (j == 0) {
                        // Better add it.
                        j = (unsigned long)sHtab::get(&dataNameTab, nm);
                        if (j == 0) {
                            sDataVec *d = vecGet(depbuf, ckt);
                            if (d) {
                                j = (int)d->realval(0);
                                run->addSpecialDesc(h->name(), j, false);
                            }
                            else {
                                GRpkgIf()->ErrPrintf(ET_WARN,
                                    "can't find '%s': value '%s' ignored.\n", 
                                    depbuf, h->name());
                            }
                            h->set_data((void*)(long)true);
                            continue;
                        }
                        else {
                            j--;
                            run->addDataDesc((char*)outd->dataNames[j],
                                outd->dataType, j);
                            outTab.add(nm, (void*)(long)(j+1));
                            depind = j;
                        }
                    }
                    else if (j > 0)
                        depind = j-1;
                    else
                        depind = -1;
                }
                else
                    depind = atoi(depbuf);
            }
            run->addSpecialDesc(h->name(), depind, usevec);
            h->set_data((void*)(long)true);
        }
    }

    if (!saveall) {
        sHgen gen(saves.table());
        sHent *h;
        bool found = false;
        while ((h = gen.next()) != 0) {
            if (*h->name() != Sp.SpecCatchar() && h->data()) {
                found = true;
                break;
            }
        }
        if (!found) {
            // The vector(s) could not be found, save everything.
            char *nmref = 0;
            if (outd->refName)
                nmref = name_tok((const char*)outd->refName);
            for (int j = 0; j < outd->numNames; j++) {
                char *nm = name_tok((char*)outd->dataNames[j]);
                if (!nm)
                    continue;
                if (nmref && lstring::eq(nm, nmref)) {
                    delete [] nm;
                    continue;
                }
                if (!sHtab::get(&outTab, nm)) {
                    run->addDataDesc((char*)outd->dataNames[j],
                        outd->dataType, j);
                    outTab.add(nm, (void*)(long)(j+1));
                }
                delete [] nm;
            }
            delete [] nmref;
        }
    }

#ifdef HAVE_SECURE
    // Below is a booby trap in case the call to Validate() is patched
    // over.  This is part of the security system.
    //
    if (!StateInitialized) {
        char namebuf[256];
        char *uname = pathlist::get_user_name(false);
#ifdef WIN32
        sprintf(namebuf, "wrspice: run %s\n", uname);
        delete [] uname;
        msw::MapiSend(Global.BugAddr(), 0, namebuf, 0, 0);
        raise(SIGTERM);
#else
        sprintf(namebuf, "mail %s", Global.BugAddr());
        FILE *fp = popen(namebuf, "w");
        if (fp) {
            fprintf(fp, "wrspice: run %s\n", uname);
            pclose(fp);
        }
        delete [] uname;
        kill(0, SIGKILL);
#endif
    }
#endif

    // If we are dumping to a file or PSF directory, set rawout.
    bool rawout = OP.getOutDesc()->outFp() ||
        OP.getOutDesc()->outFtype() == OutFpsf;


    // If writing to file, create a plot anyway, but keep only the
    // latest value (length = 1) for the runops, etc.
    //
    if (rawout)
        run->set_numPoints(1);

    // memory use test, this also sets isComplex and maxPts
    if (run->datasize()) {
        if (run->check())
            run->check()->out_rundesc = 0;
        delete run;
        return (0);
    }

    run->plotInit(outd->initValue, outd->finalValue, outd->step, plot);
    if (rawout) {
        if (OP.getOutDesc()->outFtype() == OutFpsf) {
            run->set_rd(new cPSFout(run->runPlot()));
            const char *dn = cPSFout::is_psf(OP.getOutDesc()->outFile());
            run->rd()->file_open(dn, "w", false);
        }
        else if (OP.getOutDesc()->outFtype() == OutFcsdf) {
            run->set_rd(new cCSDFout(run->runPlot()));
            run->rd()->file_open(0, "w", false);
            run->rd()->file_set_fp(OP.getOutDesc()->outFp());
        }
        else {
            run->set_rd(new cRawOut(run->runPlot()));
            bool binary = OP.getOutDesc()->outBinary();
            run->rd()->file_open(0, binary ? "wb" : "w", binary);
            run->rd()->file_set_fp(OP.getOutDesc()->outFp());
        }
        run->rd()->file_head();
    }
    else if (segfilebase)
        run->setupSegments(segfilebase, segwidth, outd);

    if (run->rd() && isIplot()) {
        GRpkgIf()->ErrPrintf(ET_WARN,
            "no iplots will be produced in this mode.\n");
    }

    if (outd->refName && run->runPlot())
        run->runPlot()->set_num_dimensions(1);

    run->circuit()->set_runplot(OP.curPlot());
    OP.curPlot()->set_active(true);
    initRunops(run);
    return (run);
}


// Append data for one point to the saved vectors.
//
int
IFoutput::appendData(sRunDesc *run, IFvalue *refValue, IFvalue *valuePtr)
{
    if (!run)
        return (OK);
    sCHECKprms *chk = run->check();
    if (chk && chk->out_mode == OutcCheck)
        run->addPointToPlot(refValue, valuePtr, false);
    else {
        run->inc_pointCount();
        run->addPointToPlot(refValue, valuePtr, run->rd() ? false : true);
        if (run->rd()) {
            if (run->pointCount() == 1)
                run->rd()->file_vars();
            run->rd()->file_points(run->pointCount()-1);
        }
        else if (run->maxPts() &&
                run->data(0)->vec->length() >= run->maxPts()) {
            // memory limit reached
            o_endit = true;
            double maxdata = DEF_maxData;

            VTvalue vv;
            if (Sp.GetVar("maxdata", VTYP_REAL, &vv, run->circuit()))
                maxdata = vv.get_real();
            GRpkgIf()->ErrPrintf(ET_ERROR,
                "stored plot data size exceeds the maximum %gKB.\n"
                "Set \"maxdata\" to alter limit.\n", maxdata);
        }
    }
    return (OK);
}


// Write data for one point to the saved vectors at the given index.
// The vectors will be expanded as necessary to include the index.
//
int
IFoutput::insertData(sCKT *ckt, sRunDesc *run, IFvalue *refValue,
    IFvalue *valuePtr, unsigned int indx)
{
    if (!run)
        return (OK);
    if (run->rd())
        return (E_PANIC);

    // Presently this is called only when using multi-threads.
    // XXX may want to enable commented blocks at some point

    /*
    sOUTcontrol *out = run->check();
    if (chk && chk->out_mode == OutcCheck) {
        run->addPointToPlot(refValue, valuePtr, false);
        if (!chk->points() ||
                refValue->rValue < chk->points()[chk->index()]) {
            Sp.VecGc();
            return (OK);
        }

        chk->evaluate();

        chk->out_index++;
        if (chk->failed() || chk->out_index == chk->out_max)
            o_endit = true;
        Sp.VecGc();
        return (OK);
    }
    */

    // The run circuit was set in beginPlot and is the primary thread
    // ciruit.  We need that actual thread circuit here.
    run->pushPointToPlot(ckt, refValue, valuePtr, indx);

    if (run->maxPts() && run->data(0)->vec->length() >= run->maxPts()) {
        // memory limit reached
        o_endit = true;
        double maxdata = DEF_maxData;

        VTvalue vv;
        if (Sp.GetVar("maxdata", VTYP_REAL, &vv, run->circuit()))
            maxdata = vv.get_real();
        GRpkgIf()->ErrPrintf(ET_ERROR,
            "stored plot data size exceeds the maximum %gKB.\n"
            "Set \"maxdata\" to alter limit.\n", maxdata);
    }

    /* what to do here?
    if (!checkRunops(run))
        o_shouldstop = true;
    */

    /* ?
    if (chk && (chk->out_mode == OutcCheckSeg ||
            chk->out_mode == OutcCheckMulti)) {
        if (!chk->points() ||
                refValue->rValue < chk->points()[chk->index()] ||
                chk->index() >= chk->out_max) {
            Sp.VecGc();
            return (OK);
        }

        if (!chk->failed()) {
            run->scalarizeVecs();
            chk->evaluate();
            run->unscalarizeVecs();
        }

        chk->out_index++;
        if ((chk->failed() || chk->out_index == chk->out_max) &&
                chk->out_mode != OutcCheckMulti)
            o_endit = true;
    }
    */

    vecGc();
    return (OK);
}


// Modify the plot dimensionality.
//
int
IFoutput::setDims(sRunDesc *run, int *dims, int numDims, bool looping)
{
    if (!run)
        return (OK);
    sCHECKprms *chk = run->check();
    if (chk && chk->out_mode == OutcCheck) {
        chk->set_index(0);
        if (chk->failed())
            o_endit = true;
        return (OK);
    }
    if (run->rd())
        return (OK);

    for (int i = 0; i < run->numData(); i++) {
        sDataVec *v = run->data(i)->vec;
        if (v) {
            if (looping) {
                if (Sp.GetFlag(FT_GRDB) && ((run->refIndex() >= 0 &&
                        i == run->refIndex()) || i == 0)) {
                    // debugging
                    sLstr lstr;
                    lstr.add("setDims: loop ");
                    lstr.add_i(numDims);
                    lstr.add(" { ");
                    for (int j = 0; j < numDims; j++) {
                        lstr.add_i(dims[j]);
                        lstr.add_c(' ');
                    }
                    lstr.add("}  scale ");
                    lstr.add_i(v->numdims());
                    lstr.add(" { ");
                    for (int j = 0; j < v->numdims(); j++) {
                        lstr.add_i(v->dims(j));
                        lstr.add_c(' ');
                    }
                    lstr.add("}\n");
                    GRpkgIf()->ErrPrintf(ET_MSGS, lstr.string());
                }

                if (run->data(i)->numbasedims == 0) {
                    if (v->numdims() == 0) {
                        run->data(i)->numbasedims = 1;
                        run->data(i)->basedims[0] = v->length();
                    }
                    else {
                        for (int j = 0; j < v->numdims(); j++)
                            run->data(i)->basedims[j] = v->dims(j);
                        run->data(i)->numbasedims = v->numdims();
                    }
                }
                for (int j = 0; j < numDims - 1; j++)
                    v->set_dims(j, dims[j]);
                for (int k = 0; k < run->data(i)->numbasedims; k++)
                    v->set_dims(k + numDims - 1, run->data(i)->basedims[k]);
                v->set_numdims(numDims - 1 + run->data(i)->numbasedims);
            }
            else {

                for (int j = 0; j < numDims; j++)
                    v->set_dims(j, dims[j]);
                v->set_numdims(numDims);
            }
        }
    }
    return (OK);
}


// Initialize plot for chained dc.
//
int
IFoutput::setDC(sRunDesc *run, sDCTprms *dc)
{
    if (run->runPlot()) {
        sDimen *dm = new sDimen((char*)dc->elt(0)->GENname,
            dc->nestLevel() == 1 ? (char*)dc->elt(1)->GENname : 0);

        dm->set_start1(dc->vstart(0));
        dm->set_stop1(dc->vstop(0));
        dm->set_step1(dc->vstep(0));
        if (dc->nestLevel() == 1) {
            dm->set_start2(dc->vstart(1));
            dm->set_stop2(dc->vstop(1));
            dm->set_step2(dc->vstep(1));
        }
        run->runPlot()->set_dimensions(dm);
    }
    return (OK);
}


// Modify the trace attributes.
//
int
IFoutput::setAttrs(sRunDesc *run, IFuid *varName, OUTscaleType param, IFvalue*)
{
    if (!run)
        return (OK);
    GridType type;
    if (param == OUT_SCALE_LIN)
        type = GRID_LIN;
    else if (param == OUT_SCALE_LOG)
        type = GRID_XLOG;
    else
        return (E_UNSUPP);

    if (run->rd()) { 
        if (varName) {
            for (int i = 0; i < run->numData(); i++)
                if (lstring::eq((char*)varName, run->data(i)->name))
                    run->data(i)->gtype = type;
        }
        else
            run->data(run->refIndex())->gtype = type;
    }
    else {
        if (varName) {
            for (sDataVec *d = run->runPlot()->tempvecs(); d; d = d->next())
                if (lstring::eq((char*)varName, d->name()))
                    d->set_gridtype(type);
        }
        else
            run->runPlot()->scale()->set_gridtype(type);
    }
    return (OK);
}


// Make scrolled plot monotonic.
//
void
IFoutput::unrollPlot(sRunDesc *run)
{
    if (run)
        run->unrollVecs();
}


void
IFoutput::addPlotNote(sRunDesc *run, const char *note)
{
    if (!run || !run->runPlot())
        return;
    if (!note || !*note)
        return;
    run->runPlot()->add_note(note);
}


// Finish building plot struct, clean up.
//
// End the run.  If force is true, delete the desc, otherwise it
// may be kept for future use.
//
void
IFoutput::endPlot(sRunDesc *run, bool force)
{
    if (!run)
        return;
    sCHECKprms *chk = run->check();
    if (chk && !force) {
        // if checkPNTS not given, evaluate here
        if (chk->out_mode == OutcCheck) {
            if (!chk->points())
                chk->evaluate();
            o_endit = false;
            return;
        }
        if (chk->out_mode == OutcCheckSeg ||
                chk->out_mode == OutcCheckMulti) {
            if (!chk->points() && !chk->failed()) {
                run->scalarizeVecs();
                chk->evaluate();
                run->unscalarizeVecs();
            }
            o_endit = false;
            return;
        }
        if (chk->out_mode != OutcNormal)
            return;
    }
    if (run->sweep())
        return;
    if (run->segfilebase() && !run->rd())
        run->dumpSegment();

    run->plotEnd();
    if (run->rd())
        run->rd()->file_update_pcnt(run->pointCount());
    endIplot(run);

    if (run->rd()) {
        run->runPlot()->destroy();
        run->set_runPlot(0);
    }
    run->job()->JOBrun = 0;
    delete run;
}


// Return system time for accounting.
//
double
IFoutput::seconds()
{
    return (::seconds());
}


// Print out error messages.

sMsg IFoutput::o_msgs[] = {
    sMsg( "Note: ", ERR_INFO ), 
    sMsg( "Warning: ", ERR_WARNING ), 
    sMsg( "Fatal: ", ERR_FATAL ), 
    sMsg( "Panic: ", ERR_PANIC ), 
    sMsg( 0, ERR_INFO )
};


namespace {
    bool check_prefix(const char *pfx, const char *string)
    {
        if (!pfx)
            return (true);
        if (!string)
            return (false);
        while (isalpha(*pfx)) {
            if ((isupper(*pfx) ? tolower(*pfx) : *pfx) !=
                    (isupper(*string) ? tolower(*string) : *string))
                return (false);
            pfx++;
            string++;
        }
        return (true);
    }
}


// Output an error or warning message.
//
int
IFoutput::error(ERRtype flag, const char *fmt, ...)
{
    va_list args;
    if (flag == ERR_INFO && !Sp.GetFlag(FT_SIMDB))
        return (OK);
    const char *pfx = 0;
    for (sMsg *m = o_msgs; m->string; m++) {
        if (flag == m->flag) {
            pfx = m->string;
            break;
        }
    }
    sLstr lstr;
    if (!check_prefix(pfx, fmt))
        lstr.add(pfx);

    va_start(args, fmt);
#ifdef HAVE_VASPRINTF
    char *str = 0;
    if (vasprintf(&str, fmt, args) >= 0) {
        lstr.add(str);
        delete [] str;
    }
#else
    char buf[1024];
    vsnprintf(buf, 1024, fmt, args);
    lstr.add(buf);
#endif
    va_end(args);

    if (lstr.string() && lstr.string()[strlen(lstr.string()) - 1] != '\n')
        lstr.add_c('\n');
    GRpkgIf()->ErrPrintf(ET_MSG, lstr.string());
    return (OK);
}
// End of IFoutput functions.


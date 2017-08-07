
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
#include "outplot.h"
#include "spglobal.h"
#include "cshell.h"
#include "frontend.h"
#include "rawfile.h"
#include "csdffile.h"
#include "psffile.h"
#include "ftedebug.h"
#include "ftemeas.h"
#include "outdata.h"
#include "ttyio.h"
#include "misc.h"
#include "errors.h"
#include "device.h"
#include "toolbar.h"
#include "noisdefs.h"
#include "tfdefs.h"
#include "sensdefs.h"
#include "spnumber/hash.h"
#include "miscutil/pathlist.h"
#ifdef SECURITY_TEST
#include <signal.h>
#include <unistd.h>
#include <limits.h>
extern int StateInitialized;  // security
#ifdef WIN32
#include "msw.h"
#include <libiberty.h>  // provides vasprintf
#endif
#endif
#include <stdarg.h>


#define DOUBLE_PRECISION 15

// Amount to increase size of vectors as they grow dynamically
#define SIZE_INCR 10

// Temp storage of vector data for scalarizing.
//
struct scData
{
    // No construcor, part of dataDesc.

    double real;
    double imag;
    int length;
    int rlength;
    int numdims;
    int dims[MAXDIMS];
};

// Temp storage of vector data for segmentizing.
//
struct segData
{
    // No construcor, part of dataDesc.

    union { double *real; complex *comp; } tdata;
    int length;
    int rlength;
    int numdims;
    int dims[MAXDIMS];
};

// Struct used to store information on a data object.
//
struct dataDesc
{
    // No constructor, allocated by Realloc which provides zeroed
    // memory.

    void addRealValue(double, bool);
    void pushRealValue(double, unsigned int);
    void addComplexValue(IFcomplex, bool);
    void pushComplexValue(IFcomplex, unsigned int);

    const char *name;   // The name of the vector
    sDataVec *vec;      // The data
    int type;           // The type
    int gtype;          // Default plot grid type
    int outIndex;       // Index of regular vector or dependent
    int numbasedims;    // Vector numdims for looping
    int basedims[MAXDIMS]; // Vecor dimensions for looping
    scData sc;          // Back store for scalarization
    segData seg;        // Back store for segmentization
    IFspecial sp;       // Descriptor for special parameter
    bool useVecIndx;    // Use vector for special array index
    bool regular;       // True if regular vector, false if special
    bool rollover_ok;   // If true, roll over
    bool scalarized;    // True if scalarized
    bool segmentized;   // True if segmentized
};

// Struct used to store run status and context.  This is returned from
// beginPlot as an opaque data type to be passed to the other methods.
//
struct sRunDesc
{
    sRunDesc()
        {
            rd_name = 0;
            rd_type = 0;
            rd_title = 0;
            rd_job = 0;
            rd_ckt = 0;
            rd_circ = 0;
            rd_check = 0;
            rd_sweep = 0;
            rd_runPlot = 0;
            rd_rd = 0;
            rd_data = 0;
            rd_numData = 0;
            rd_dataSize = 0;
            rd_refIndex = 0;
            rd_pointCount = 0;
            rd_numPoints = 0;
            rd_isComplex = 0;
            rd_maxPts = 0;
            rd_cycles = 0;
            rd_anType = 0;

            rd_segfilebase = 0;
            rd_segdelta = 0.0;
            rd_seglimit = 0.0;
            rd_segindex = 0;
            rd_scrolling = false;
        }

    ~sRunDesc()
        {
            delete [] rd_name;
            delete [] rd_type;
            delete [] rd_title;

            for (int i = 0; i < rd_numData; i++)
                delete [] rd_data[i].name;
            delete [] rd_data;
            delete rd_rd;
            delete rd_segfilebase;
        }

    bool datasize();
    void unrollVecs();
    void scalarizeVecs();
    void unscalarizeVecs();
    void segmentizeVecs();
    void unsegmentizeVecs();
    int addDataDesc(const char*, int, int);
    int addSpecialDesc(const char*, int, bool);
    bool getSpecial(sCKT*, int, IFvalue*);
    void plotInit(double, double, double, sPlot*);
    void addPointToPlot(IFvalue*, IFvalue*, bool);
    void pushPointToPlot(sCKT*, IFvalue*, IFvalue*, unsigned int);
    void setupSegments(const char*, double, sOUTdata*);
    void dumpSegment();
    void resetVecs();
    void init_debugs();
    void init_measures();
    bool measure();
    bool breakPtCheck();
    void iplot(sDbComm*);
    void updateDims(sGraph*);
    void endIplot();
    void plotEnd();

    const char *name()              { return (rd_name); }
    void set_name(const char *n)
        {
            char *s = lstring::copy(n);
            delete [] rd_name;
            rd_name = s;
        }

    const char *type()              { return (rd_type); }
    void set_type(const char *t)
        {
            char *s = lstring::copy(t);
            delete [] rd_type;
            rd_type = s;
        }

    const char *title()             { return (rd_title); }
    void set_title(const char *t)
        {
            char *s = lstring::copy(t);
            delete [] rd_title;
            rd_title = s;
        }

    sJOB *job()                     { return (rd_job); }
    void set_job(sJOB *a)           { rd_job = a; }

    void set_ckt(sCKT *c)           { rd_ckt = c; rd_circ = c->CKTbackPtr; }
    sCKT *get_ckt()                 { return (rd_ckt); }
    sFtCirc *circuit()              { return (rd_circ); }

    sPlot *runPlot()                { return (rd_runPlot); }
    void set_runPlot(sPlot *p)      { rd_runPlot = p; }

    sCHECKprms *check()             { return (rd_check); }
    void set_check(sCHECKprms *c)   { rd_check = c; }

    sSWEEPprms *sweep()             { return (rd_sweep); }
    void set_sweep(sSWEEPprms *c)   { rd_sweep = c; }

    cFileOut *rd()                  { return (rd_rd); }
    void set_rd(cFileOut *f)        { rd_rd = f; }

    dataDesc *data(int i)           { return (&rd_data[i]); }

    int numData()                   { return (rd_numData); }

    int refIndex()                  { return (rd_refIndex); }
    void set_refIndex(int i)        { rd_refIndex = i; }

    int pointCount()                { return (rd_pointCount); }
    void inc_pointCount()           { rd_pointCount++; }

    int numPoints()                 { return (rd_numPoints); }
    void set_numPoints(int n)       { rd_numPoints = n; }

    int maxPts()                    { return (rd_maxPts); }

    int cycles()                    { return (rd_cycles); }
    void set_cycles(int c)          { rd_cycles = c; }

    int anType()                    { return (rd_anType); }
    void set_anType(int t)          { rd_anType = t; }

    void set_dims(int *d)
        {
            d[1] = rd_numPoints;
            d[0] = rd_segindex + 2;
            rd_segindex++;
        }
    const char *segfilebase()       { return (rd_segfilebase); }

    void set_scrolling(bool b)      { rd_scrolling = b; }

private:
    char *rd_name;      // circuit name: e.g., CKT1
    char *rd_type;      // analysis key: e.g., TRAN
    char *rd_title;     // analysis descr: e.g., Transient Analysis

    sJOB *rd_job;           // pointer to analysis job struct
    sCKT *rd_ckt;           // pointer to running circuit
    sCHECKprms *rd_check;   // pointer to range analysis control struct
    sSWEEPprms *rd_sweep;   // pointer to sweep analysis control struct
    sFtCirc *rd_circ;       // pointer to circuit
    sPlot *rd_runPlot;      // associated plot
    cFileOut *rd_rd;        // associated plot file interface
    dataDesc *rd_data;      // the data descriptors

    int rd_numData;         // size of data array
    int rd_dataSize;        // allocated size of data array
    int rd_refIndex;        // index of reference datum in data array
    int rd_pointCount;      // running count of output points
    int rd_numPoints;       // num points expected, -1 if not known
    int rd_isComplex;       // set if any vector is complex
    int rd_maxPts;          // point limit
    int rd_cycles;          // number of data cycles in multi-dim analysis
    int rd_anType;          // analysis type

    // These parameters are for multi-file (segmented) output.
    const char *rd_segfilebase; // base file name, files are "basename.sNN"
    double rd_segdelta;     // range if refValue for segment
    double rd_seglimit;     // end of segment
    int rd_segindex;        // count of segments outputs
    bool rd_scrolling;      // true when scrolling
};

namespace {
    bool parseSpecial(const char*, char*);
    char *name_tok(const char*);
    void update_dvecs(sGraph*, sDbComm*, sDataVec*, bool*, sRunDesc*);
}


// beginPlot():         Start pointwise output plot.
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
            run->set_ckt(ckt);  // this may have changed
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
            run->init_debugs();
            run->init_measures();
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
                run->set_ckt(ckt);  // this may have changed
            plot = swp->out_plot;
            if (run) {
                for (int i = 0; i < run->numData(); i++) {
                    // clear these cached values, may be bogus
                    run->data(i)->sp.sp_isset = false;
                    run->data(i)->sp.sp_inst = 0;
                    run->data(i)->sp.sp_mod = 0;
                }
                run->init_debugs();
                run->init_measures();
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
    run->set_ckt(ckt);
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
        Sp.GetSaves(run->circuit(), &saves);  // from front end
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
                            sDataVec *d = Sp.VecGet(depbuf, ckt);
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

#ifdef SECURITY_TEST
    // Below is a booby trap in case the call to Validate() is patched
    // over.  This is part of the security system.
    //
    if (!StateInitialized) {
        char namebuf[256];
        char *uname = pathlist::get_user_name(false);
#ifdef WIN32
        sprintf(namebuf, "wrspice: run %s\n", uname);
        delete [] uname;
        msw::MapiSend(MAIL_ADDR, 0, namebuf, 0, 0);
        raise(SIGTERM);
#else
        sprintf(namebuf, "mail %s", MAIL_ADDR);
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
    bool rawout = Sp.GetOutDesc()->outFp() ||
        Sp.GetOutDesc()->outFtype() == OutFpsf;


    // If writing to file, create a plot anyway, but keep only the
    // latest value (length = 1) for the debugs, etc.
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
        if (Sp.GetOutDesc()->outFtype() == OutFpsf) {
            run->set_rd(new cPSFout(run->runPlot()));
            const char *dn = cPSFout::is_psf(Sp.GetOutDesc()->outFile());
            run->rd()->file_open(dn, "w", false);
        }
        else if (Sp.GetOutDesc()->outFtype() == OutFcsdf) {
            run->set_rd(new cCSDFout(run->runPlot()));
            run->rd()->file_open(0, "w", false);
            run->rd()->file_set_fp(Sp.GetOutDesc()->outFp());
        }
        else {
            run->set_rd(new cRawOut(run->runPlot()));
            bool binary = Sp.GetOutDesc()->outBinary();
            run->rd()->file_open(0, binary ? "wb" : "w", binary);
            run->rd()->file_set_fp(Sp.GetOutDesc()->outFp());
        }
        run->rd()->file_head();
    }
    else if (segfilebase)
        run->setupSegments(segfilebase, segwidth, outd);

    if (run->rd() && Sp.IsIplot())
        GRpkgIf()->ErrPrintf(ET_WARN,
            "no iplots will be produced in this mode.\n");

    if (outd->refName && run->runPlot())
        run->runPlot()->set_num_dimensions(1);

    run->circuit()->set_runplot(Sp.CurPlot());
    Sp.CurPlot()->set_active(true);
    run->init_debugs();
    run->init_measures();
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
    if (chk && chk->out_mode == OutcCheck) {
        run->addPointToPlot(refValue, valuePtr, false);
        if (!chk->points() ||
                refValue->rValue < chk->points()[chk->index()]) {
            Sp.VecGc();
            return (OK);
        }

        chk->evaluate();

        chk->set_index(chk->index() + 1);
        if (chk->failed() || chk->index() == chk->max_index())
            o_endit = true;
        Sp.VecGc();
        return (OK);
    }

    run->inc_pointCount();
    run->addPointToPlot(refValue, valuePtr, run->rd() ? false : true);
    if (run->rd()) {
        if (run->pointCount() == 1)
            run->rd()->file_vars();
        run->rd()->file_points(run->pointCount()-1);
    }
    else if (run->maxPts() && run->data(0)->vec->length() >= run->maxPts()) {
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

    if (run->measure())
        o_shouldstop = true;
    if (!run->breakPtCheck())
        o_shouldstop = true;

    if (chk && (chk->out_mode == OutcCheckSeg ||
            chk->out_mode == OutcCheckMulti)) {
        if (!chk->points() ||
                refValue->rValue < chk->points()[chk->index()] ||
                chk->index() >= chk->max_index()) {
            Sp.VecGc();
            return (OK);
        }

        if (!chk->failed()) {
            run->scalarizeVecs();
            chk->evaluate();
            run->unscalarizeVecs();
        }

        chk->set_index(chk->index() + 1);
        if ((chk->failed() || chk->index() == chk->max_index()) &&
                chk->out_mode != OutcCheckMulti)
            o_endit = true;
    }

    Sp.VecGc();
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
    if (run->measure())
        o_shouldstop = true;
    if (!run->breakPtCheck())
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

    Sp.VecGc();
    return (OK);
}


// setDims():           Modify the plot dimensionality.
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


// setDC():             Initialize plot for chained dc.
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


// setAttrs():          Modify the trace attributes.
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


// initMeasure():       Initialize .measures.
//
int
IFoutput::initMeasure(sRunDesc *run)
{
    if (run)
        run->init_measures();
    return (OK);
}


// pauseText():         Check for requested pause.
//
int
IFoutput::pauseTest(sRunDesc *run)
{
    if (!Sp.GetFlag(FT_BATCHMODE))
        GP.Checkup();
    if (Sp.GetFlag(FT_INTERRUPT)) {
        o_shouldstop = false;
        Sp.SetFlag(FT_INTERRUPT, false);
        ToolBar()->UpdatePlots(0);
        if (run)
            run->endIplot();
        return (E_INTRPT);
    }
    else if (o_shouldstop) {
        o_shouldstop = false;
        ToolBar()->UpdatePlots(0);
        if (run)
            run->endIplot();
        return (E_PAUSE);
    }
    else
        return (OK);
}


// unrollPlot():        Make scrolled plot monotonic.
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


// endPlot():           Finish building plot struct, clean up.
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
    run->endIplot();

    if (run->rd()) {
        run->runPlot()->destroy();
        run->set_runPlot(0);
    }
    run->job()->JOBrun = 0;
    delete run;
}


// endIplot():          Clean up after iplot.
//
// Clean up after an iplot.
//
void
IFoutput::endIplot(sRunDesc *run)
{
    if (run)
        run->endIplot();
}


// seconds():           Return system time for accounting.
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


// error():             Output an error or warning message.
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
// parser efficiently in the debugs.
//
void
sRunDesc::scalarizeVecs()
{
    for (int k = 0; k < rd_numData; k++) {
        if (rd_data[k].scalarized)
            continue;
        scData *bk = &rd_data[k].sc;
        sDataVec *v = rd_data[k].vec;
        if (v && v->length() > 1) {
            bk->length = v->length();
            bk->rlength = v->allocated();
            bk->numdims = v->numdims();
            for (int i = 0; i < MAXDIMS; i++)
                bk->dims[i] = v->dims(i);

            v->set_length(1);
            v->set_allocated(1);
            v->set_numdims(1);
            for (int i = 0; i < MAXDIMS; i++)
                v->set_dims(i, 0);
            if (v->isreal()) {
                bk->real = v->realval(0);
                v->set_realval(0, v->realval(bk->length - 1));
            }
            else {
                bk->real = v->realval(0);
                bk->imag = v->imagval(0);
                v->set_compval(0, v->compval(bk->length - 1));
            }
            rd_data[k].scalarized = true;
        }
    }
}


void
sRunDesc::unscalarizeVecs()
{
    for (int k = 0; k < rd_numData; k++) {
        if (!rd_data[k].scalarized)
            continue;
        scData *bk = &rd_data[k].sc;
        sDataVec *v = rd_data[k].vec;
        if (v) {
            v->set_length(bk->length);
            v->set_allocated(bk->rlength);
            v->set_numdims(bk->numdims);
            for (int i = 0; i < MAXDIMS; i++)
                v->set_dims(i, bk->dims[i]);
            if (v->isreal())
                v->set_realval(0, bk->real);
            else {
                v->set_realval(0, bk->real);
                v->set_imagval(0, bk->imag);
            }
            rd_data[k].scalarized = false;
        }
    }
}


// Temporarily convert multi-dimensional vectors to single dimensional
// vectors of the last segment.
//
void
sRunDesc::segmentizeVecs()
{
    for (int k = 0; k < rd_numData; k++) {
        if (rd_data[k].segmentized)
            continue;
        segData *bk = &rd_data[k].seg;
        sDataVec *v = rd_data[k].vec;
        if (v && v->numdims() > 1) {
            int per = v->dims(v->numdims() - 1);
            int l = v->length();
            if (l < per)
                continue;
            int lx = (l/per)*per;
            if (lx == l)
                lx = l - per;
            int newlen = l - lx;
            bk->length = v->length();
            bk->rlength = v->allocated();
            bk->numdims = v->numdims();
            for (int i = 0; i < MAXDIMS; i++)
                bk->dims[i] = v->dims(i);
            v->set_length(newlen);
            v->set_allocated(newlen);
            v->set_numdims(1);
            for (int i = 0; i < MAXDIMS; i++)
                v->set_dims(i, 0);
            if (v->isreal()) {
                bk->tdata.real = v->realvec();
                v->set_realvec(v->realvec() + lx);
            }
            else {
                bk->tdata.comp = v->compvec();
                v->set_compvec(v->compvec() + lx);
            }
            rd_data[k].segmentized = true;
        }
    }
}


void
sRunDesc::unsegmentizeVecs()
{
    for (int k = 0; k < rd_numData; k++) {
        if (!rd_data[k].segmentized)
            continue;
        segData *bk = &rd_data[k].seg;
        sDataVec *v = rd_data[k].vec;
        if (v) {
            v->set_length(bk->length);
            v->set_allocated(bk->rlength);
            v->set_numdims(bk->numdims);
            for (int i = 0; i < MAXDIMS; i++)
                v->set_dims(i, bk->dims[i]);
            if (v->isreal())
                v->set_realvec(bk->tdata.real);
            else
                v->set_compvec(bk->tdata.comp);
            rd_data[k].segmentized = false;
        }
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
    memset(d, 0, sizeof(dataDesc));

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
    memset(d, 0, sizeof(dataDesc));

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


// The plot maintenance functions.
//
void
sRunDesc::plotInit(double tstart, double tstop, double tstep, sPlot *plot)
{
    ToolBar()->UpdatePlots(1);
    if (plot == 0) {
        rd_runPlot = new sPlot(rd_type);
        rd_runPlot->new_plot();
        Sp.SetCurPlot(rd_runPlot->type_name());
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


// bool inc: increment dvec length if true.
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
sRunDesc::init_debugs()
{
    sDebug *db = rd_circ->debugs();
    sDbComm *d, *dt;
    // called at beginning of run
    if (DB.step_count() != DB.num_steps()) {
        // left over from last run
        DB.set_step_count(0);
        DB.set_num_steps(0);
    }
    for (d = DB.stops(); d; d = d->next()) {
        for (dt = d; dt; dt = dt->also()) {
            if (dt->type() == DB_STOPWHEN)
                dt->set_point(0);
        }
    }
    for (dt = DB.traces(); dt; dt = dt->next())
        dt->set_point(0);
    for (dt = DB.iplots(); dt; dt = dt->next()) {
        if (dt->type() == DB_DEADIPLOT) {
            // user killed the window
            if (dt->graphid())
                GP.DestroyGraph(dt->graphid());
            dt->set_type(DB_IPLOT);
        }
        if (rd_check && rd_check->out_mode == OutcCheckSeg)
            dt->set_reuseid(dt->graphid());
        if (!(rd_check && (rd_check->out_mode == OutcCheckMulti ||
                rd_check->out_mode == OutcLoop))) {
            dt->set_point(0);
            dt->set_graphid(0);
        }
    }
    if (db) {
        for (d = db->stops(); d; d = d->next()) {
            for (dt = d; dt; dt = dt->also()) {
                if (dt->type() == DB_STOPWHEN)
                    dt->set_point(0);
            }
        }
        for (dt = db->traces(); dt; dt = dt->next())
            dt->set_point(0);
        for (dt = db->iplots(); dt; dt = dt->next()) {
            if (dt->type() == DB_DEADIPLOT) {
                // user killed the window
                if (dt->graphid())
                    GP.DestroyGraph(dt->graphid());
                dt->set_type(DB_IPLOT);
            }
            if (rd_check && rd_check->out_mode == OutcCheckSeg)
                dt->set_reuseid(dt->graphid());
            if (!(rd_check && (rd_check->out_mode == OutcCheckMulti ||
                    rd_check->out_mode == OutcLoop))) {
                dt->set_point(0);
                dt->set_graphid(0);
            }
        }
    }
}


// Initialize the measurements.
//
void
sRunDesc::init_measures()
{
    for (sMeas *m = rd_circ->measures(); m; m = m->next) {
        if (rd_job->JOBtype != m->analysis)
            continue;
        m->reset(rd_runPlot);
    }
}


// Call the check function for each measurement.  If all measurements
// are complete and the stop flag was given, return true.
//
bool
sRunDesc::measure()
{
    if (rd_circ->measures()) {
        segmentizeVecs();
        bool done = true;
        bool stop = false;
        sMeas *m;
        for (m = rd_circ->measures(); m; m = m->next) {
            if (rd_anType != m->analysis)
                continue;
            if (!m->check(rd_circ)) {
                done = false;
                continue;
            }
            if (m->shouldstop())
                stop = true;
        }
        done &= stop;
        if (done) {
            // reset stop flags so analysis can be continued
            for (m = rd_circ->measures(); m; m = m->next)
                m->nostop();
        }
        unsegmentizeVecs();
        return (done);
    }
    return (false);
}


// The output routines call this routine to update the debugs, and to
// see if the run should continue. If it returns true, then the run should
// continue.  This should be called with point = -1 at the end of analysis.
//
bool
sRunDesc::breakPtCheck()
{
    if (rd_pointCount <= 0)
        return (true);
    sDebug *db = 0;
    if (rd_circ && rd_circ->debugs())
        db = rd_circ->debugs();
    bool nohalt = true;
    if (DB.traces() || DB.stops() || (db && (db->traces() || db->stops()))) {
        bool tflag = true;
        scalarizeVecs();
        sDbComm *d;
        for (d = DB.traces(); d; d = d->next())
            d->print_trace(rd_runPlot, &tflag, rd_pointCount);
        if (db) {
            for (d = db->traces(); d; d = d->next())
                d->print_trace(rd_runPlot, &tflag, rd_pointCount);
        }
        for (d = DB.stops(); d; d = d->next()) {
            if (d->should_stop(rd_pointCount)) {
                bool need_pr = TTY.is_tty() && CP.GetFlag(CP_WAITING);
                TTY.printf("%-2d: condition met: stop ", d->number());
                d->printcond(0);
                nohalt = false;
                if (need_pr)
                    CP.Prompt();
            }
        }
        if (db) {
            for (d = db->stops(); d; d = d->next()) {
                if (d->should_stop(rd_pointCount)) {
                    bool need_pr = TTY.is_tty() && CP.GetFlag(CP_WAITING);
                    TTY.printf("%-2d: condition met: stop ", d->number());
                    d->printcond(0);
                    nohalt = false;
                    if (need_pr)
                        CP.Prompt();
                }
            }
        }
        unscalarizeVecs();
    }
    if (DB.iplots() || (db && db->iplots())) {
        int len = rd_runPlot->scale()->length();
        if (len >= IPOINTMIN || (rd_runPlot->scale()->flags() & VF_ROLLOVER)) {
            bool doneone = false;
            for (sDbComm *d = DB.iplots(); d; d = d->next()) {
                if (d->type() == DB_IPLOT) {
                    iplot(d);
                    if (GRpkgIf()->CurDev() &&
                            GRpkgIf()->CurDev()->devtype == GRfullScreen) {
                        doneone = true;
                        break;
                    }
                }
            }
            if (rd_circ->debugs() && !doneone) {
                db = rd_circ->debugs();
                for (sDbComm *d = db->iplots(); d; d = d->next()) {
                    if (d->type() == DB_IPLOT) {
                        iplot(d);
                        if (GRpkgIf()->CurDev() &&
                                GRpkgIf()->CurDev()->devtype == GRfullScreen)
                            break;
                    }
                }
            }
        }
    }

    if (DB.step_count() > 0 && DB.dec_step_count() == 0) {
        if (DB.num_steps() > 1) {
            bool need_pr = TTY.is_tty() && CP.GetFlag(CP_WAITING);
            TTY.printf("Stopped after %d steps.\n", DB.num_steps());
            if (need_pr)
                CP.Prompt();
        }
        return (false);
    }
    return (nohalt);
}


// Do some incremental plotting. 3 cases -- first, if length < IPOINTMIN, 
// don't do anything. Second, if length = IPOINTMIN, plot what we have
// so far. Third, if length > IPOINTMIN, plot the last points and resize
// if needed.
// Note we don't check for pole / zero because they are of length 1.
//
#define FACTOR .25     // How much to expand the scale during iplot.
#define IPLTOL 2e-3    // Allow this fraction out of range before redraw.

void
sRunDesc::iplot(sDbComm *db)
{
    if (db->point() == -1)
        return;
    if (Sp.GetFlag(FT_GRDB))
        GRpkgIf()->ErrPrintf(ET_MSGS, "Entering iplot, len = %d\n\r",
            rd_pointCount);

    sGraph *graph;
    sDvList *dl0, *dl;
    if (!db->graphid()) {
        // Draw the grid for the first time, and plot everything

        // Error handling:  If an error occurs during plot initialization, 
        // point() is set to -1 (graphic() remains 0).
        // If an error occurs later, point() is set to -1, and the plot
        // is "frozen".

        wordlist *wl = CP.LexStringSub(db->string());
        if (!wl) {
            db->set_point(-1);
            return;
        }
        dl0 = 0;

        sDataVec *xs = rd_runPlot->scale();
        if (!xs) {
            db->set_point(-1);
            return;
        }
        if (xs->length() < IPOINTMIN)
            return;
        int rlen = xs->allocated();

        sPlot *plot = Sp.CurPlot();
        sFtCirc *circ = Sp.CurCircuit();
        Sp.SetCurPlot(rd_runPlot);
        Sp.SetCurCircuit(rd_circ);
        pnlist *pl = Sp.GetPtree(wl, false);
        if (pl)
            dl0 = Sp.DvList(pl);
        Sp.SetCurPlot(plot);
        Sp.SetCurCircuit(circ);
        if (!dl0) {
            db->set_point(-1);
            return;
        }
        // check for "vs"
        bool foundvs = false;
        sDvList *dp, *dn;
        for (dp = 0, dl = dl0; dl; dl = dn) {
            dn = dl->dl_next;
            if (!dl->dl_dvec) {
                // only from "vs"
                if (!dp || !dn) {
                    GRpkgIf()->ErrPrintf(ET_ERROR, "misplaced vs arg.\n");
                    sDvList::destroy(dl0);
                    dl0 = 0;
                    db->set_point(-1);
                    return;
                }
                foundvs = true;
                xs = dn->dl_dvec;
                dp->dl_next = dn->dl_next;
                delete dl;
                delete dn;
                break;
            }
            dp = dl;
        }
        if (!foundvs) {
            xs = xs->copy();
            xs->newtemp();
        }

        dl = dl0;
        sGrInit gr;
        if (!GP.Setup(&gr, &dl0, 0, xs, 0)) {
            db->set_point(-1);
            return;
        }
        if (dl != dl0)
            // copied
            sDvList::destroy(dl);
        gr.command = wl;

        // the dvec list is copied to graph->plotdata

        bool reuse = false;
        graph = 0;
        if (db->reuseid() > 0) {
            // if this is set, reuse the iplot window
            graph = GP.FindGraph(db->reuseid());
            if (graph) {
                graph->dev()->Clear();
                reuse = true;
                graph->gr_reset();
            }
        }
        graph = GP.Init(dl0, &gr, graph);
        sDvList::destroy(dl0);
        if (graph)
            db->set_graphid(graph->id());
        if (!db->graphid()) {
            db->set_point(-1);
            return;
        }

        // push graph for possible use by err/interrupt handlers
        if (GRpkgIf()->CurDev() &&
                GRpkgIf()->CurDev()->devtype == GRfullScreen)
            GP.PushGraphContext(graph);

        // extend the vectors to the length of the scale
        dl0 = (sDvList*)graph->plotdata();
        if (dl0->dl_dvec->scale()->allocated() != rlen) {
            xs = dl0->dl_dvec->scale();
            if (xs->isreal()) {
                double *d = new double[rlen];
                if (!d)
                    return;
                for (int i = 0; i < xs->length(); i++)
                    d[i] = xs->realval(i);
                xs->set_realvec(d, true);
            }
            else {
                complex *c = new complex[rlen];
                if (!c)
                    return;
                for (int i = 0; i < xs->length(); i++)
                    c[i] = xs->compval(i);
                xs->set_compvec(c, true);
            }
            xs->set_allocated(rlen);
        }
        for (dl = dl0; dl; dl = dl->dl_next) {
            if (dl->dl_dvec->allocated() != rlen) {
                if (dl->dl_dvec->isreal()) {
                    double *d = new double[rlen];
                    if (!d)
                        return;
                    for (int i = 0; i < dl->dl_dvec->length(); i++)
                        d[i] = dl->dl_dvec->realval(i);
                    dl->dl_dvec->set_realvec(d, true);
                }
                else {
                    complex *c = new complex[rlen];
                    if (!c)
                        return;
                    for (int i = 0; i < dl->dl_dvec->length(); i++)
                        c[i] = dl->dl_dvec->compval(i);
                    dl->dl_dvec->set_compvec(c, true);
                }
                dl->dl_dvec->set_allocated(rlen);
            }
        }
        if (reuse)
            graph->gr_redraw();
        return;
    }

    graph = GP.FindGraph(db->graphid());
    if (!graph) {
        // shouldn't happen
        db->set_point(-1);
        return;
    }
    graph->set_noevents(true);  // suppress event checking

    dl0 = (sDvList*)graph->plotdata();
    bool vs_flag;
    update_dvecs(graph, db, rd_runPlot->scale(), &vs_flag, this);
    sDataVec *xs = dl0->dl_dvec->scale();
    int len = xs->length() - 1;

    // First see if we have to make the screen bigger
    double val = xs->realval(len - 1);
    if (Sp.GetFlag(FT_GRDB))
        GRpkgIf()->ErrPrintf(ET_MSGS, "x = %G\n\r", val);

    // hack to keep from overshooting range
    double start, stop;
    rd_runPlot->range(&start, &stop, 0);
    if (stop < start) {
        double tmp = start;
        start = stop;
        stop = tmp;
    }

    bool changed = false;
    if (graph->rawdata().xmax < graph->rawdata().xmin)
        graph->rawdata().xmax = graph->rawdata().xmin;

    if (graph->datawin().xmin == graph->datawin().xmax)
        changed = true;
    else if (val < graph->datawin().xmin) {
        changed = true;
        if (rd_runPlot->scale()->flags() & VF_ROLLOVER) {
            double dxs = stop - start;
            if (dxs > 0 && graph->datawin().xmax - graph->datawin().xmin <
                    .99*dxs) {
                graph->rawdata().xmin = graph->datawin().xmin;
                do {
                    graph->rawdata().xmin -=
                        (graph->datawin().xmax - graph->rawdata().xmin)*FACTOR;

                    if (!vs_flag && graph->rawdata().xmin <
                            graph->rawdata().xmax - dxs) {
                        graph->rawdata().xmin = graph->rawdata().xmax - dxs;
                        break;
                    }
                } while (val < graph->rawdata().xmin);
            }
            else {
                int nsp = graph->xaxis().lin.numspace;
                double del =
                    (graph->datawin().xmax - graph->datawin().xmin)/nsp;
                graph->datawin().xmax -= del;
                graph->datawin().xmin -= del;
                graph->rawdata().xmin -= del;
                graph->rawdata().xmax -= del;
            }
        }
        else {
            graph->rawdata().xmin = graph->datawin().xmin;
            do {
                if (Sp.GetFlag(FT_GRDB)) {
                    GRpkgIf()->ErrPrintf(ET_MSGS, "resize: xlo %G -> %G\n\r", 
                        graph->datawin().xmin, graph->datawin().xmin -
                        (graph->datawin().xmax -
                        graph->datawin().xmin)*FACTOR);
                }

                graph->rawdata().xmin -=
                    (graph->datawin().xmax - graph->rawdata().xmin)*FACTOR;

                if (!vs_flag && start != stop &&
                        graph->rawdata().xmin < start) {
                    graph->rawdata().xmin = start;
                    break;
                }
            } while (val < graph->rawdata().xmin);
        }
    }
    else if (val > graph->datawin().xmax) {
        changed = true;
        if (rd_runPlot->scale()->flags() & VF_ROLLOVER) {
            double dxs = stop - start;
            if (dxs > 0 && graph->datawin().xmax - graph->datawin().xmin <
                    .99*dxs) {
                graph->rawdata().xmax = graph->datawin().xmax;
                do {
                    graph->rawdata().xmax +=
                        (graph->rawdata().xmax - graph->datawin().xmin)*FACTOR;

                    if (!vs_flag && graph->rawdata().xmax >
                            graph->rawdata().xmin + dxs) {
                        graph->rawdata().xmax = graph->rawdata().xmin + dxs;
                        break;
                    }
                } while (val > graph->rawdata().xmax);
            }
            else {
                int nsp = graph->xaxis().lin.numspace;
                double del =
                    (graph->datawin().xmax - graph->datawin().xmin)/nsp;
                graph->datawin().xmax += del;
                graph->datawin().xmin += del;
                graph->rawdata().xmin += del;
                graph->rawdata().xmax += del;
            }
        }
        else {
            graph->rawdata().xmax = graph->datawin().xmax;
            do {
                if (Sp.GetFlag(FT_GRDB)) {
                    GRpkgIf()->ErrPrintf(ET_MSGS, "resize: xhi %G -> %G\n\r", 
                        graph->datawin().xmax, graph->datawin().xmax +
                        (graph->datawin().xmax -
                        graph->datawin().xmin)*FACTOR);
                }

                graph->rawdata().xmax +=
                    (graph->rawdata().xmax - graph->datawin().xmin)*FACTOR;

                if (!vs_flag && start != stop &&
                        graph->rawdata().xmax > stop) {
                    graph->rawdata().xmax = stop;
                    break;
                }
            } while (val > graph->rawdata().xmax);
        }
    }
    else if (val > graph->rawdata().xmax)
        graph->rawdata().xmax = val;
    else if (val < graph->rawdata().xmin)
        graph->rawdata().xmin = val;

    sDataVec *v;
    for (dl = dl0; dl; dl = dl->dl_next) {
        v = dl->dl_dvec;
        val = v->realval(len - 1);
        if (Sp.GetFlag(FT_GRDB))
            GRpkgIf()->ErrPrintf(ET_MSGS, "y = %G\n\r", val);
        if (val < graph->rawdata().ymin)
            graph->rawdata().ymin = val;
        else if (val > graph->rawdata().ymax)
            graph->rawdata().ymax = val;
        double delta;
        if (graph->format() == FT_GROUP) {
            if (*v->units() == UU_VOLTAGE) {
                delta = IPLTOL*(graph->grpmax(0) - graph->grpmin(0));
                if (val < graph->grpmin(0) - delta ||
                        val > graph->grpmax(0) + delta) {
                    changed = true;
                    break;
                }
            }
            else if (*v->units() == UU_CURRENT) {
                delta = IPLTOL*(graph->grpmax(1) - graph->grpmin(1));
                if (val < graph->grpmin(1) - delta ||
                        val > graph->grpmax(1) + delta) {
                    changed = true;
                    break;
                }
            }
            else {
                delta = IPLTOL*(graph->grpmax(2) - graph->grpmin(2));
                if (val < graph->grpmin(2) - delta ||
                        val > graph->grpmax(2) + delta) {
                    changed = true;
                    break;
                }
            }
        }
        else if (graph->format() == FT_MULTI) {
            delta = IPLTOL*(v->maxsignal() - v->minsignal());
            if (val < v->minsignal() - delta || val > v->maxsignal() + delta) {
                changed = true;
                break;
            }
        }
        else {
            delta = IPLTOL*(graph->datawin().ymax - graph->datawin().ymin);
            if (val < graph->datawin().ymin - delta ||
                    val > graph->datawin().ymax + delta) {
                changed = true;
                break;
            }
        }
    }

    updateDims(graph);

    if (changed) {
        // Redraw everything
        graph->clear_selections();
        graph->dev()->Clear();
        graph->clear_saved_text();
        graph->gr_redraw();
    }
    else {
        // blank the "backtrace" in multi-dimensional plot
        bool blank = false;
        if (xs && xs->numdims() > 1 &&
                (len - 1) % xs->dims(xs->numdims() - 1) == 0)
            blank = true;
        if (!blank) {
            graph->gr_draw_last(len);
            graph->dev()->Update();
        }
    }
    graph->set_noevents(false);
}


// Update the dimensions of the graph's data vectors.
//
void
sRunDesc::updateDims(sGraph *graph)
{
    sDataVec *xs = rd_runPlot->scale();
    if (xs && xs->numdims() > 1) {
        sDvList *dl0 = (sDvList*)graph->plotdata();
        sDataVec *ys = dl0->dl_dvec->scale();
        ys->set_numdims(xs->numdims());
        if (ys) {
            for (int i = 0; i < xs->numdims(); i++)
                ys->set_dims(i, xs->dims(i));
            for (sDvList *dl = dl0; dl; dl = dl->dl_next) {
                sDataVec *v = dl->dl_dvec;
                v->set_numdims(xs->numdims());
                for (int i = 0; i < xs->numdims(); i++)
                    v->set_dims(i, xs->dims(i));
            }
        }
    }
}


void
sRunDesc::endIplot()
{
    sDebug *db = 0;
    if (rd_circ)
        db = rd_circ->debugs();
    if (DB.iplots() || (db && db->iplots())) {
        if (GRpkgIf()->CurDev() &&
                GRpkgIf()->CurDev()->devtype == GRfullScreen)
            // redraw
            GP.PopGraphContext();
        for (sDbComm *d = DB.iplots(); d; d = d->next()) {
            d->set_reuseid(0);
            if (d->type() == DB_IPLOT && d->point() == 0) {
                if (d->graphid()) {
                    sGraph *graph = GP.FindGraph(d->graphid());
                    graph->dev()->Clear();
                    graph->clear_selections();
                    updateDims(graph);
                    graph->gr_redraw();
                    graph->gr_end();
                }
            }
            if (d->type() == DB_DEADIPLOT) {
                // user killed the window while it was running
                if (d->graphid())
                    GP.DestroyGraph(d->graphid());
                d->set_type(DB_IPLOT);
                d->set_graphid(0);
            }
        }
        if (db) {
            for (sDbComm *d = db->iplots(); d; d = d->next()) {
                d->set_reuseid(0);
                if (d->type() == DB_IPLOT && d->point() == 0) {
                    if (d->graphid()) {
                        sGraph *graph = GP.FindGraph(d->graphid());
                        graph->dev()->Clear();
                        graph->clear_selections();
                        updateDims(graph);
                        graph->gr_redraw();
                        graph->gr_end();
                    }
                }
                if (d->type() == DB_DEADIPLOT) {
                    // user killed the window while it was running
                    if (d->graphid())
                        GP.DestroyGraph(d->graphid());
                    d->set_type(DB_IPLOT);
                    d->set_graphid(0);
                }
            }
        }
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
// End of sRunDesc methods.


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


    // Compute the last entry, and grow the iplot's dvecs.
    //
    void update_dvecs(sGraph *graph, sDbComm *db, sDataVec *xs, bool *vs_flag,
        sRunDesc *run)
    {
        const char *msg = "iplot #%d.\n";
        *vs_flag = false;
        sDvList *dvl = (sDvList*)graph->plotdata();
        wordlist *wl = CP.LexStringSub(db->string());
        if (!wl) {
            db->set_point(-1);
            GRpkgIf()->ErrPrintf(ET_INTERR, msg, 1);
            return;
        }
        run->scalarizeVecs();
        sPlot *plot = Sp.CurPlot();
        sFtCirc *circ = Sp.CurCircuit();
        Sp.SetCurPlot(run->runPlot());
        Sp.SetCurCircuit(run->circuit());
        pnlist *pl = Sp.GetPtree(wl, false);
        wordlist::destroy(wl);
        sDvList *dl0 = 0;
        if (pl) {
            Sp.SetFlag(FT_SILENT, true);  // silence "vec not found" msgs
            dl0 = Sp.DvList(pl);
            Sp.SetFlag(FT_SILENT, false);
        }
        Sp.SetCurPlot(plot);
        Sp.SetCurCircuit(circ);
        if (!dl0) {
            db->set_point(-1);
            GRpkgIf()->ErrPrintf(ET_INTERR, msg, 2);
            run->unscalarizeVecs();
            return;
        }
        sDvList *dl, *dd, *dp, *dn;
        for (dp = 0, dl = dl0; dl; dl = dn) {
            dn = dl->dl_next;
            if (!dl->dl_dvec) {
                // only from "vs"
                *vs_flag = true;
                if (!dp || !dn) {
                    GRpkgIf()->ErrPrintf(ET_INTERR, msg, 3);
                    sDvList::destroy(dl0);
                    db->set_point(-1);
                    return;
                }
                xs = dn->dl_dvec;
                dp->dl_next = dn->dl_next;
                delete dl;
                delete dn;
                break;
            }
            dp = dl;
        }
        for (dl = dvl, dd = dl0; dl && dd;
                dl = dl->dl_next, dd = dd->dl_next) {
            sDataVec *vt = dl->dl_dvec;
            sDataVec *vf = dd->dl_dvec;

            if (vt->isreal()) {
                if (!vf->isreal()) {
                    db->set_point(-1);
                    GRpkgIf()->ErrPrintf(ET_INTERR, msg, 3);
                    break;
                }
                if (vt->length() >= vt->allocated()) {
                    double *tv = new double[vt->length() + 1];
                    for (int i = 0; i < vt->length(); i++)
                        tv[i] = vt->realval(i);
                    vt->set_realvec(tv, true);
                    vt->set_allocated(vt->length() + 1);
                }
                if (graph->gridtype() == GRID_SMITH) {
                    double re = vf->realval(0);
                    vt->set_realval(vt->length(), (re - 1) / (re + 1));
                }
                else
                    vt->set_realval(vt->length(), vf->realval(0));
            }
            else {
                if (vf->isreal()) {
                    db->set_point(-1);
                    GRpkgIf()->ErrPrintf(ET_INTERR, msg, 4);
                    break;
                }
                if (vt->length() >= vt->allocated()) {
                    complex *cv = new complex[vt->length() + 1];
                    for (int i = 0; i < vt->length(); i++)
                        cv[i] = vt->compval(i);
                    vt->set_compvec(cv, true);
                    vt->set_allocated(vt->length() + 1);
                }
                if (graph->gridtype() == GRID_SMITH) {
                    double re = vf->realval(0);
                    double im = vf->imagval(0);
                    double dnom = (re+1.0)*(re+1.0) + im*im;
                    vt->set_realval(vt->length(), (re*re + im*im + 1.0)/dnom);
                    vt->set_imagval(vt->length(), 2.0*im/dnom);
                }
                else
                    vt->set_compval(vt->length(), vf->compval(0));
            }
            vt->set_length(vt->length() + 1);;
        }
        if (dl || dd) {
            db->set_point(-1);
            GRpkgIf()->ErrPrintf(ET_INTERR, msg, 5);
        }
        sDataVec *scale = dvl->dl_dvec->scale();
        // is it already updated?
        for (dl = dvl; dl; dl = dl->dl_next)
            if (scale == dl->dl_dvec)
                break;
        if (!dl) {
            // no need to Smith transform the scale
            if (xs->isreal()) {
                if (!scale->isreal()) {
                    db->set_point(-1);
                    GRpkgIf()->ErrPrintf(ET_INTERR, msg, 6);
                }
                if (scale->length() >= scale->allocated()) {
                    double *tv = new double[scale->length() + 1];
                    for (int i = 0; i < scale->length(); i++)
                        tv[i] = scale->realval(i);
                    scale->set_realvec(tv, true);
                    scale->set_allocated(scale->length() + 1);
                }
                scale->set_realval(scale->length(), xs->realval(0));
            }
            else {
                if (scale->isreal()) {
                    db->set_point(-1);
                    GRpkgIf()->ErrPrintf(ET_INTERR, msg, 7);
                }
                if (scale->length() >= scale->allocated()) {
                    complex *cv = new complex[scale->length() + 1];
                    for (int i = 0; i < scale->length(); i++)
                        cv[i] = scale->compval(i);
                    scale->set_compvec(cv, true);
                    scale->set_allocated(scale->length() + 1);
                }
                scale->set_compval(scale->length(), xs->compval(0));
            }
            scale->set_length(scale->length() + 1);
        }
        sDvList::destroy(dl0);

        // The following resizes the storage in scroll mode.  The scale of
        // the analysis must be monotonically increasing.
        //
        if (xs->flags() & VF_ROLLOVER) {
            int i;
            for (i = 0; i < scale->length(); i++) {
                // This is true when we bump up the lower limit
                if (scale->realval(i) > graph->datawin().xmin)
                    break;
            }
            if (i) {
                for (dl = dvl; dl; dl = dl->dl_next) {
                    sDataVec *v = dl->dl_dvec;
                    v->set_length(v->length() - i);
                    v->set_allocated(v->allocated() - i);
                    if (v->isreal()) {
                        double *d = new double[v->allocated()];
                        for (int j = 0; j < v->allocated(); j++)
                            d[j] = v->realval(j+i);
                        v->set_realvec(d, true);
                    }
                    else {
                        complex *c = new complex[v->allocated()];
                        for (int j = 0; j < v->allocated(); j++)
                            c[j] = v->compval(j+i);
                        v->set_compvec(c, true);
                    }
                }
                for (dl = dvl; dl; dl = dl->dl_next) {
                    if (scale == dl->dl_dvec)
                        break;
                }
                if (!dl) {
                    scale->set_length(scale->length() - i);
                    scale->set_allocated(scale->allocated() - i);
                    if (scale->isreal()) {
                        double *d = new double[scale->allocated()];
                        for (int j = 0; j < scale->allocated(); j++)
                            d[j] = scale->realval(j+i);
                        scale->set_realvec(d, true);
                    }
                    else {
                        complex *c = new complex[scale->allocated()];
                        for (int j = 0; j < scale->allocated(); j++)
                            c[j] = scale->compval(j+i);
                        scale->set_compvec(c, true);
                    }
                }
            }
        }
        run->unscalarizeVecs();
    }
}


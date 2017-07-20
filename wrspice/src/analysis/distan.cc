
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
 $Id: distan.cc,v 2.35 2016/09/26 01:48:46 stevew Exp $
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3e2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1988 Jaijeet S. Roychowhury
         1992 Stephen R. Whiteley
****************************************************************************/

#include "distdefs.h"
#include "dcodefs.h"
#include "device.h"
#include "spmatrix.h"
#include "input.h"
#include "outdata.h"
#include "misc.h"


extern int CKTdisto(sCKT*, int);
extern int DkerProc(int, double*, double*, int, sDISTOAN*);

#define DISswap(a, b) temp = (a); (a) = (b); (b) = temp
#define DmemAlloc(a, size) a = new double[size]
#define DstorAlloc(a, size) a = new double*[size]


sDISTOAN::~sDISTOAN()
{
    for (int i = 0; i < displacement; i++) {
        if (r1H1stor && r1H1stor[i]) delete [] r1H1stor[i];
        if (i1H1stor && i1H1stor[i]) delete [] i1H1stor[i];
        if (r2H11stor && r2H11stor[i]) delete [] r2H11stor[i];
        if (i2H11stor && i2H11stor[i]) delete [] i2H11stor[i];

        if (r3H11stor && r3H11stor[i]) delete [] r3H11stor[i];
        if (i3H11stor && i3H11stor[i]) delete [] i3H11stor[i];

        if (r1H2stor && r1H2stor[i]) delete [] r1H2stor[i];
        if (i1H2stor && i1H2stor[i]) delete [] i1H2stor[i];
        if (r2H12stor && r2H12stor[i]) delete [] r2H12stor[i];
        if (i2H12stor && i2H12stor[i]) delete [] i2H12stor[i];
        if (r2H1m2stor && r2H1m2stor[i]) delete [] r2H1m2stor[i];
        if (i2H1m2stor && i2H1m2stor[i]) delete [] i2H1m2stor[i];
        if (r3H1m2stor && r3H1m2stor[i]) delete [] r3H1m2stor[i];
        if (i3H1m2stor && i3H1m2stor[i]) delete [] i3H1m2stor[i];
    }
    delete [] r1H1stor; r1H1stor = 0;
    delete [] i1H1stor; i1H1stor = 0;
    delete [] r2H11stor; r2H11stor = 0;
    delete [] i2H11stor; i2H11stor = 0;

    delete [] r3H11stor; r3H11stor = 0;
    delete [] i3H11stor; i3H11stor = 0;

    delete [] r1H2stor; r1H2stor = 0;
    delete [] i1H2stor; i1H2stor = 0;
    delete [] r2H12stor; r2H12stor = 0;
    delete [] i2H12stor; i2H12stor = 0;
    delete [] r2H1m2stor; r2H1m2stor = 0;
    delete [] i2H1m2stor; i2H1m2stor = 0;
    delete [] r3H1m2stor; r3H1m2stor = 0;
    delete [] i3H1m2stor; i3H1m2stor = 0;

    delete [] r1H1ptr; r1H1ptr = 0;
    delete [] r2H11ptr; r2H11ptr = 0;
    delete [] i1H1ptr; i1H1ptr = 0;
    delete [] i2H11ptr; i2H11ptr = 0;

    delete [] r3H11ptr; r3H11ptr = 0;
    delete [] i3H11ptr; i3H11ptr = 0;

    delete [] r1H2ptr; r1H2ptr = 0;
    delete [] i1H2ptr; i1H2ptr = 0;
    delete [] r2H12ptr; r2H12ptr = 0;
    delete [] i2H12ptr; i2H12ptr = 0;
    delete [] r2H1m2ptr; r2H1m2ptr = 0;
    delete [] i2H1m2ptr; i2H1m2ptr = 0;
    delete [] r3H1m2ptr; r3H1m2ptr = 0;
    delete [] i3H1m2ptr; i3H1m2ptr = 0;
}


int
DISTOanalysis::anFunc(sCKT *ckt, int restart)
{
    sDISTOAN* job = static_cast<sDISTOAN*>(ckt->CKTcurJob);
    sCKTmodGen mgen(ckt->CKTmodels);
    for (sGENmodel *m = mgen.next(); m; m = mgen.next()) {
        if (DEV.device(m->GENmodType)->flags() & DV_NOAC) {
            OP.error(ERR_FATAL,
                "Distortion analysis not possible with device %s.",
                DEV.device(m->GENmodType)->name());
            return (OK);
        }
    }

    // start at beginning
    ckt->CKTcurrentAnalysis |= DOING_AC | DOING_DISTO;

    int i;
    int numNames;
    double *temp;
    IFuid *nameList;
    IFuid freqUid;
    sRunDesc *Drun;
    int error;
    if (restart) {
        if (!job->JOBoutdata)
            job->JOBoutdata = new sOUTdata;

        switch(job->DstepType) {

        case DECADE:
            job->DfreqDelta =  exp(log(10.0)/job->DnumSteps);
            job->FreqTol = job->DfreqDelta * 
                job->DstopF1 * ckt->CKTcurTask->TSKreltol;
            job->NumPoints = 1 + (int)floor((job->DnumSteps) / log(10.0) *
                log((job->DstopF1+job->FreqTol)/(job->DstartF1)));
            break;

        case OCTAVE:
            job->DfreqDelta = exp(log(2.0)/job->DnumSteps);
            job->FreqTol = job->DfreqDelta * 
                job->DstopF1 * ckt->CKTcurTask->TSKreltol;
            job->NumPoints = 1 + (int)floor((job->DnumSteps) / log(2.0) *
                log((job->DstopF1+job->FreqTol)/(job->DstartF1)));
            break;

        case LINEAR:
            job->DfreqDelta = (job->DstopF1 - job->DstartF1)/
                    (job->DnumSteps+1);
            job->FreqTol = job->DfreqDelta * ckt->CKTcurTask->TSKreltol;
            job->NumPoints = job->DnumSteps+ 1 +
                (int)floor(job->FreqTol/(job->DfreqDelta));
            break;

        default:
            ckt->CKTcurrentAnalysis &= ~(DOING_AC | DOING_DISTO);
            return (E_BADPARM);
        }

        // freqUid is used as temporary storage
        freqUid = job->JOBname;
        job->JOBname = (char*)"DISTORTION - operating pt.";
        error = DCOinfo.anFunc(ckt, 0); 
        if (error) {
            ckt->CKTcurrentAnalysis &= ~(DOING_AC | DOING_DISTO);
            return(error);
        }

        job->JOBname = freqUid;

        error = CKTdisto(ckt, D_SETUP);
        if (error) {
            ckt->CKTcurrentAnalysis &= ~(DOING_AC | DOING_DISTO);
            return(error);
        }

        job->displacement = 0;

        job->Freq = job->DstartF1;
        if (job->Df2wanted) {
            /*
            omegadelta = 2.0 * M_PI * job->Freq *(1. - job->Df2ovrF1);
            */
            // keeping f2 const to be compatible with spectre
            job->Domega2 = 2.0 * M_PI * job->Freq * job->Df2ovrF1;
        }
        DstorAlloc(job->r1H1stor, job->NumPoints+2);
        DstorAlloc(job->r2H11stor, job->NumPoints+2);
        DstorAlloc(job->i1H1stor, job->NumPoints+2);
        DstorAlloc(job->i2H11stor, job->NumPoints+2);
        job->size = ckt->CKTmatrix->spGetSize(1);

        DmemAlloc(job->r1H1ptr, job->size+2);
        DmemAlloc(job->r2H11ptr, job->size+2);
        DmemAlloc(job->i1H1ptr, job->size+2);
        DmemAlloc(job->i2H11ptr, job->size+2);

        if (!job->Df2wanted) {
            DstorAlloc(job->r3H11stor, job->NumPoints+2);
            DstorAlloc(job->i3H11stor, job->NumPoints+2);

            DmemAlloc(job->r3H11ptr, job->size+2);
            DmemAlloc(job->i3H11ptr, job->size+2);
        }
        else {
            DstorAlloc(job->r1H2stor, job->NumPoints+2);
            DstorAlloc(job->i1H2stor, job->NumPoints+2);
            DstorAlloc(job->r2H12stor, job->NumPoints+2);
            DstorAlloc(job->i2H12stor, job->NumPoints+2);
            DstorAlloc(job->r2H1m2stor, job->NumPoints+2);
            DstorAlloc(job->i2H1m2stor, job->NumPoints+2);
            DstorAlloc(job->r3H1m2stor, job->NumPoints+2);
            DstorAlloc(job->i3H1m2stor, job->NumPoints+2);

            DmemAlloc(job->r1H2ptr, job->size+2);
            DmemAlloc(job->r2H12ptr, job->size+2);
            DmemAlloc(job->r2H1m2ptr, job->size+2);
            DmemAlloc(job->r3H1m2ptr, job->size+2);
            DmemAlloc(job->i1H2ptr, job->size+2);
            DmemAlloc(job->i2H12ptr, job->size+2);
            DmemAlloc(job->i2H1m2ptr, job->size+2);
            DmemAlloc(job->i3H1m2ptr, job->size+2);
        }
    }

    sOUTdata *outd = job->JOBoutdata;

    while (job->Freq <= job->DstopF1+job->FreqTol) {

        if ((error = OP.pauseTest(0)) < 0) {
            // pause
            ckt->CKTcurrentAnalysis &= ~(DOING_AC | DOING_DISTO);
            return (error);
        }

        ckt->CKTomega = 2.0 * M_PI * job->Freq;
        job->Domega1 = ckt->CKTomega;
        ckt->CKTmode = MODEAC;

        error = ckt->acLoad();
        if (error) goto done;

        error = CKTdisto(ckt, D_RHSF1);
        // sets up the RHS vector for all inputs corresponding to F1
        if (error) goto done;

        error = ckt->NIdIter();
        if (error) goto done;
        DISswap(ckt->CKTrhsOld, job->r1H1ptr);
        DISswap(ckt->CKTirhsOld, job->i1H1ptr);

        ckt->CKTomega *= 2;
        error = ckt->acLoad();
        if (error) goto done;

        error = CKTdisto(ckt, D_TWOF1);
        if (error) goto done;
        error = ckt->NIdIter();
        if (error) goto done;
        DISswap(ckt->CKTrhsOld, job->r2H11ptr);
        DISswap(ckt->CKTirhsOld, job->i2H11ptr);

        if (!job->Df2wanted) {

            ckt->CKTomega = 3 * job->Domega1;
            error = ckt->acLoad();
            if (error) goto done;

            error = CKTdisto(ckt, D_THRF1);
            if (error) goto done;
            error = ckt->NIdIter();
            if (error) goto done;
            DISswap(ckt->CKTrhsOld, job->r3H11ptr);
            DISswap(ckt->CKTirhsOld, job->i3H11ptr);
        }
        else if (job->Df2given) {

            /*
            ckt->CKTomega = job->Domega1 - omegadelta;
            job->Domega2 = ckt->CKTomega;
            */
            ckt->CKTomega = job->Domega2;
            error = ckt->acLoad();
            if (error) goto done;

            error = CKTdisto(ckt, D_RHSF2);
            if (error) goto done;
            error = ckt->NIdIter();
            if (error) goto done;
            DISswap(ckt->CKTrhsOld, job->r1H2ptr);
            DISswap(ckt->CKTirhsOld, job->i1H2ptr);

            ckt->CKTomega = job->Domega1 + job->Domega2;
            error = ckt->acLoad();
            if (error) goto done;

            error = CKTdisto(ckt, D_F1PF2);
            if (error) goto done;
            error = ckt->NIdIter();
            if (error) goto done;
            DISswap(ckt->CKTrhsOld, job->r2H12ptr);
            DISswap(ckt->CKTirhsOld, job->i2H12ptr);

            ckt->CKTomega = job->Domega1 - job->Domega2;
            error = ckt->acLoad();
            if (error) goto done;

            error = CKTdisto(ckt, D_F1MF2);
            if (error) goto done;
            error = ckt->NIdIter();
            if (error) goto done;
            DISswap(ckt->CKTrhsOld, job->r2H1m2ptr);
            DISswap(ckt->CKTirhsOld, job->i2H1m2ptr);

            ckt->CKTomega = 2*job->Domega1 - job->Domega2;
            error = ckt->acLoad();
            if (error) goto done;

            error = CKTdisto(ckt, D_2F1MF2);
            if (error) goto done;
            error = ckt->NIdIter();
            if (error) goto done;
            DISswap(ckt->CKTrhsOld, job->r3H1m2ptr);
            DISswap(ckt->CKTirhsOld, job->i3H1m2ptr);
        }
        else {
            IP.logError(0, "No source with f2 distortion input");
            error = E_NOF2SRC;
            goto done;
        }

        DmemAlloc(job->r1H1stor[job->displacement], job->size+2);
        DISswap(job->r1H1stor[job->displacement], job->r1H1ptr);
        job->r1H1stor[job->displacement][0]=job->Freq;
        DmemAlloc(job->r2H11stor[job->displacement], job->size+2);
        DISswap(job->r2H11stor[job->displacement], job->r2H11ptr);
        job->r2H11stor[job->displacement][0]=job->Freq;
        DmemAlloc(job->i1H1stor[job->displacement], job->size+2);
        DISswap(job->i1H1stor[job->displacement], job->i1H1ptr);
        job->i1H1stor[job->displacement][0]=0.0;
        DmemAlloc(job->i2H11stor[job->displacement], job->size+2);
        DISswap(job->i2H11stor[job->displacement], job->i2H11ptr);
        job->i2H11stor[job->displacement][0]=0.0;

        if (!job->Df2wanted) {
            DmemAlloc(job->r3H11stor[job->displacement], job->size+2); 
            DISswap(job->r3H11stor[job->displacement], job->r3H11ptr);
            job->r3H11stor[job->displacement][0]=job->Freq;
            DmemAlloc(job->i3H11stor[job->displacement], job->size+2); 
            DISswap(job->i3H11stor[job->displacement], job->i3H11ptr);
            job->i3H11stor[job->displacement][0]=0.0;
        }
        else {
            DmemAlloc(job->r1H2stor[job->displacement], job->size+2);
            DISswap(job->r1H2stor[job->displacement], job->r1H2ptr);
            job->r1H2stor[job->displacement][0]=job->Freq;
            DmemAlloc(job->r2H12stor[job->displacement], job->size+2);
            DISswap(job->r2H12stor[job->displacement], job->r2H12ptr);
            job->r2H12stor[job->displacement][0]=job->Freq;
            DmemAlloc(job->r2H1m2stor[job->displacement], job->size+2);
            DISswap(job->r2H1m2stor[job->displacement], job->r2H1m2ptr);
            job->r2H1m2stor[job->displacement][0]=job->Freq;
            DmemAlloc(job->r3H1m2stor[job->displacement], job->size+2);
            DISswap(job->r3H1m2stor[job->displacement], job->r3H1m2ptr);
            job->r3H1m2stor[job->displacement][0]=job->Freq;

            DmemAlloc(job->i1H2stor[job->displacement], job->size+2);
            DISswap(job->i1H2stor[job->displacement], job->i1H2ptr);
            job->i1H2stor[job->displacement][0]=0.0;
            DmemAlloc(job->i2H12stor[job->displacement], job->size+2);
            DISswap(job->i2H12stor[job->displacement], job->i2H12ptr);
            job->i2H12stor[job->displacement][0]=0.0;
            DmemAlloc(job->i2H1m2stor[job->displacement], job->size+2);
            DISswap(job->i2H1m2stor[job->displacement], job->i2H1m2ptr);
            job->i2H1m2stor[job->displacement][0]=0.0;
            DmemAlloc(job->i3H1m2stor[job->displacement], job->size+2);
            DISswap(job->i3H1m2stor[job->displacement], job->i3H1m2ptr);
            job->i3H1m2stor[job->displacement][0]=0.0;
        }
        job->displacement++;

        switch (job->DstepType) {
        case DECADE:
        case OCTAVE:
            job->Freq *= job->DfreqDelta;
            if (job->DfreqDelta == 1) break;
            continue;

        case LINEAR:
            job->Freq += job->DfreqDelta;
            if (job->DfreqDelta == 0) break;
            continue;

        default:
            error = E_INTERN;
            goto done;
        }

        break;
    }

    // output routines to process the H's and output actual ckt variable
    // values

    error = OK;
    if (!job->Df2wanted) {
        error = ckt->names(&numNames, &nameList);
        if (error)
            goto done;

        ckt->newUid(&freqUid, 0, "frequency", UID_OTHER);
        Drun = 0;
        outd->circuitPtr = ckt;
        outd->analysisPtr = ckt->CKTcurJob;
        outd->analName = (char*)"DISTORTION: 2nd harmonic";
        outd->refName = freqUid;
        outd->refType = IF_REAL;
        outd->numNames = numNames;
        outd->dataNames = nameList;
        outd->dataType = IF_COMPLEX;
        outd->numPts = 1;
        outd->count = 0;
        outd->initValue = 0;
        outd->finalValue = 0;
        outd->step = 0;
        Drun = OP.beginPlot(outd);
        if (!Drun) {
            error = E_TOOMUCH;
            goto done;
        }

        for (i = 0; i < job->displacement ; i++) {
            DkerProc(D_TWOF1, *(job->r2H11stor + i), 
                 *(job->i2H11stor + i), job->size, job);
            double *t1 = ckt->CKTrhsOld;
            double *t2 = ckt->CKTirhsOld;
            ckt->CKTrhsOld = *((job->r2H11stor) + i);
            ckt->CKTirhsOld = *((job->i2H11stor) + i);
            ckt->acDump(ckt->CKTrhsOld[0], Drun);
            ckt->CKTrhsOld = t1;
            ckt->CKTirhsOld = t2;
        }
        OP.endPlot(Drun, false);
        Drun = 0;
        if (error) goto done;

        error = ckt->names(&numNames, &nameList);
        if (error)
            goto done;

        ckt->newUid(&freqUid, 0, "frequency", UID_OTHER);
        outd->circuitPtr = ckt;
        outd->analysisPtr = ckt->CKTcurJob;
        outd->analName = (char*)"DISTORTION: 3rd harmonic";
        outd->refName = freqUid;
        outd->refType = IF_REAL;
        outd->numNames = numNames;
        outd->dataNames = nameList;
        outd->dataType = IF_COMPLEX;
        outd->numPts = 1;
        outd->count = 0;
        outd->initValue = 0;
        outd->finalValue = 0;
        outd->step = 0;
        Drun = OP.beginPlot(outd);
        if (!Drun) {
            error = E_TOOMUCH;
            goto done;
        }

        for (i=0; i< job->displacement ; i++) {
            DkerProc(D_THRF1, *(job->r3H11stor + i), 
                 *(job->i3H11stor + i), 
                 job->size, job);
            double *t1 = ckt->CKTrhsOld;
            double *t2 = ckt->CKTirhsOld;
            ckt->CKTrhsOld = *((job->r3H11stor) + i);
            ckt->CKTirhsOld = *((job->i3H11stor) + i);
            ckt->acDump(ckt->CKTrhsOld[0], Drun);
            ckt->CKTrhsOld = t1;
            ckt->CKTirhsOld = t2;
        }
        OP.endPlot(Drun, false);
        Drun = 0;
    }
    else {

        error = ckt->names(&numNames, &nameList);
        if (error)
            goto done;

        ckt->newUid(&freqUid, 0, "frequency", UID_OTHER);
        Drun = 0;
        outd->circuitPtr = ckt;
        outd->analysisPtr = ckt->CKTcurJob;
        outd->analName = (char*)"DISTORTION: IM, f1+f2";
        outd->refName = freqUid;
        outd->refType = IF_REAL;
        outd->numNames = numNames;
        outd->dataNames = nameList;
        outd->dataType = IF_COMPLEX;
        outd->numPts = 1;
        outd->count = 0;
        outd->initValue = 0;
        outd->finalValue = 0;
        outd->step = 0;
        Drun = OP.beginPlot(outd);
        if (!Drun) {
            error = E_TOOMUCH;
            goto done;
        }

        for (i=0; i< job->displacement ; i++) {
            DkerProc(D_F1PF2, *(job->r2H12stor + i), 
                 *(job->i2H12stor + i), 
                 job->size, job);
            double *t1 = ckt->CKTrhsOld;
            double *t2 = ckt->CKTirhsOld;
            ckt->CKTrhsOld = *((job->r2H12stor) + i);
            ckt->CKTirhsOld = *((job->i2H12stor) + i);
            ckt->acDump(ckt->CKTrhsOld[0], Drun);
            ckt->CKTrhsOld = t1;
            ckt->CKTirhsOld = t2;
        }
        OP.endPlot(Drun, false);
        Drun = 0;
        if (error) goto done;

        error = ckt->names(&numNames, &nameList);
        if (error)
            goto done;

        ckt->newUid(&freqUid, 0, "frequency", UID_OTHER);
        outd->circuitPtr = ckt;
        outd->analysisPtr = ckt->CKTcurJob;
        outd->analName = (char*)"DISTORTION: IM, f1-f2";
        outd->refName = freqUid;
        outd->refType = IF_REAL;
        outd->numNames = numNames;
        outd->dataNames = nameList;
        outd->dataType = IF_COMPLEX;
        outd->numPts = 1;
        outd->count = 0;
        outd->initValue = 0;
        outd->finalValue = 0;
        outd->step = 0;
        Drun = OP.beginPlot(outd);
        if (!Drun) {
            error = E_TOOMUCH;
            goto done;
        }

        for (i=0; i< job->displacement ; i++) {
            DkerProc(D_F1MF2, 
                *(job->r2H1m2stor + i), 
                *(job->i2H1m2stor + i), 
                 job->size, job);
            double *t1 = ckt->CKTrhsOld;
            double *t2 = ckt->CKTirhsOld;
            ckt->CKTrhsOld = *((job->r2H1m2stor) + i);
            ckt->CKTirhsOld = *((job->i2H1m2stor) + i);
            ckt->acDump(ckt->CKTrhsOld[0], Drun);
            ckt->CKTrhsOld = t1;
            ckt->CKTirhsOld = t2;
        }
        OP.endPlot(Drun, false);
        Drun = 0;
        if (error) goto done;

        error = ckt->names(&numNames, &nameList);
        if (error) {
            ckt->CKTcurrentAnalysis &= ~(DOING_AC | DOING_DISTO);
            return (error);
        }

        ckt->newUid(&freqUid, 0, "frequency", UID_OTHER);
        outd->circuitPtr = ckt;
        outd->analysisPtr = ckt->CKTcurJob;
        outd->analName = (char*)"DISTORTION: IM, 2f1-f2";
        outd->refName = freqUid;
        outd->refType = IF_REAL;
        outd->numNames = numNames;
        outd->dataNames = nameList;
        outd->dataType = IF_COMPLEX;
        outd->numPts = 1;
        outd->count = 0;
        outd->initValue = 0;
        outd->finalValue = 0;
        outd->step = 0;
        Drun = OP.beginPlot(outd);
        if (!Drun) {
            error = E_TOOMUCH;
            goto done;
        }

        for (i=0; i< job->displacement ; i++) {
            DkerProc(D_2F1MF2, 
                *(job->r3H1m2stor + i), 
                *(job->i3H1m2stor + i), 
                 job->size, job);
            double *t1 = ckt->CKTrhsOld;
            double *t2 = ckt->CKTirhsOld;
            ckt->CKTrhsOld = *((job->r3H1m2stor) + i);
            ckt->CKTirhsOld = *((job->i3H1m2stor) + i);
            ckt->acDump(ckt->CKTrhsOld[0], Drun);
            ckt->CKTrhsOld = t1;
            ckt->CKTirhsOld = t2;
        }
        OP.endPlot(Drun, false);
        Drun = 0;
    }

done:
    delete job;
    ckt->CKTcurrentAnalysis &= ~(DOING_AC | DOING_DISTO);
    return (error);
}


int
CKTdisto(sCKT *ckt, int mode)
{
    sDISTOAN* cv = static_cast<sDISTOAN*>(ckt->CKTcurJob);
    int error = OK;

    switch (mode) {

    case D_TWOF1:
    case D_THRF1:
    case D_F1PF2:
    case D_F1MF2:
    case D_2F1MF2:
        {
            int size = ckt->CKTmatrix->spGetSize(1);
            for (int i = 1; i <= size; i++) {
                *(ckt->CKTrhs + i) = 0;
                *(ckt->CKTirhs + i) = 0;
            }
        }
        // fallthrough

    case D_SETUP:
        {
            sCKTmodGen mgen(ckt->CKTmodels);
            sGENmodel *m;
            while ((m = mgen.next()) != 0) {
                error = DEV.device(m->GENmodType)->disto(mode, m, ckt);
                if (error)
                    return (error);
            }
        }
        break;

    case D_RHSF1:
         cv->Df2given = 0; // will change if any F2 source is found
        // fallthrough

    case D_RHSF2:
        {
            int size = ckt->CKTmatrix->spGetSize(1);
            for (int i = 0; i <= size; i++) {
                *(ckt->CKTrhs+i) = 0;
                *(ckt->CKTirhs+i) = 0;
            }
        }
        DEV.distoSet(ckt, mode, cv);
        break;

    default: 
        error = E_BADPARM;
        break;
    }

    return (error);
}


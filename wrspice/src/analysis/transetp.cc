
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
 $Id: transetp.cc,v 2.63 2016/03/15 00:07:01 stevew Exp $
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3f2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 Thomas L. Quarles
         1993 Stephen R. Whiteley
****************************************************************************/

#include "trandefs.h"
#include "errors.h"
#include "outdata.h"
#include "kwords_analysis.h"


TRANanalysis TRANinfo;

// Tran keywords
const char *trkw_part     = "part";
const char *trkw_tstart   = "start";
const char *trkw_tmax     = "tmax";
const char *trkw_uic      = "uic";
const char *trkw_scroll   = "scroll";
const char *trkw_segment  = "segment";
const char *trkw_segwidth = "segwidth";

namespace {
    IFparm TRANparms[] = {
        IFparm(trkw_part,       TRAN_PARTS,     IF_IO|IF_REALVEC,
            "time ranges"),
        IFparm(trkw_tstart,     TRAN_TSTART,    IF_IO|IF_REAL,
            "starting time"),
        IFparm(trkw_tmax,       TRAN_TMAX,      IF_IO|IF_REAL,
            "maximum time step"),
        IFparm(trkw_uic,        TRAN_UIC,       IF_IO|IF_FLAG,
            "use initial conditions"),
        IFparm(trkw_scroll,     TRAN_SCROLL,    IF_IO|IF_FLAG,
            "simulate continuously"),
        IFparm(trkw_segment,    TRAN_SEGMENT,   IF_IO|IF_STRING,
            "dump data files"),
        IFparm(trkw_segwidth,   TRAN_SEGWIDTH,  IF_IO|IF_REAL,
            "dump interval"),
        IFparm(dckw_name1,      DC_NAME1,       IF_IO|IF_INSTANCE,
            "name of source to step"),
        IFparm(dckw_start1,     DC_START1,      IF_IO|IF_REAL,
            "starting voltage/current"),
        IFparm(dckw_stop1,      DC_STOP1,       IF_IO|IF_REAL,
            "ending voltage/current"),
        IFparm(dckw_step1,      DC_STEP1,       IF_IO|IF_REAL,
            "voltage/current step"),
        IFparm(dckw_name2,      DC_NAME2,       IF_IO|IF_INSTANCE,
            "name of source to step"),
        IFparm(dckw_start2,     DC_START2,      IF_IO|IF_REAL,
            "starting voltage/current"),
        IFparm(dckw_stop2,      DC_STOP2,       IF_IO|IF_REAL,
            "ending voltage/current"),
        IFparm(dckw_step2,      DC_STEP2,       IF_IO|IF_REAL,
            "voltage/current step")
    };
}


TRANanalysis::TRANanalysis()
{
    name = "TRAN";
    description = "Transient analysis";
    numParms = sizeof(TRANparms)/sizeof(IFparm);
    analysisParms = TRANparms;
    domain = TIMEDOMAIN;
};


int 
TRANanalysis::setParm(sJOB *anal, int which, IFdata *data)
{
    sTRANAN *job = static_cast<sTRANAN*>(anal);
    if (!job)
        return (E_PANIC);
    IFvalue *value = &data->v;

    switch (which) {
    case TRAN_PARTS:
        {
            int n = value->v.numValue;
            const double *v = value->v.vec.rVec;
            // Don't call data->cleanup() until finished with v.

            if (n < 2 || (n & 1)) {
                data->cleanup();
                return (E_PARMVAL);
            }
            n >>= 1;
            delete job->TRANspec;
            job->TRANspec = transpec_t::new_transpec(0.0, n, v);
            data->cleanup();
            if (!job->TRANspec)
                return (E_PARMVAL);
        }
        break;

    case TRAN_TSTART:
        if (!job->TRANspec || value->rValue >= job->TRANspec->end(0) ||
                value->rValue < 0.0)
            return (E_PARMVAL);
        job->TRANspec->tstart = value->rValue;
        break;

    case TRAN_TMAX:
        if (value->rValue <= 0.0)
            return (E_PARMVAL);
        job->TRANmaxStep = value->rValue;
        break;

    case TRAN_UIC:
        if (value->iValue)
            job->TRANmode |= MODEUIC;
        break;

    case TRAN_SCROLL:
        if (value->iValue)
            job->TRANmode |= MODESCROLL;
        break;

    case TRAN_SEGMENT:
        if (value->sValue)
            job->TRANsegBaseName = value->sValue;
        break;

    case TRAN_SEGWIDTH:
        if (value->rValue)
            job->TRANsegDelta = value->rValue;
        break;

    default:
        if (job->JOBdc.setp(which, data) == OK)
            return (OK);
        return (E_BADPARM);
    }
    return (OK);
}
// End of TRANanalysis functions.


//
// The following implements a specification for output points allowing the
// point density to be different in different regions.
//

// Static function.
// Create a new specification.  The v is an array of size 2*num containing
// tsep and tend for each of the num regions.  the ts is the start
// point.
//
transpec_t *
transpec_t::new_transpec(double ts, int num, const double *v)
{
    if (ts < 0.0)
        ts = 0.0;
    transpec_t *t = (transpec_t*)new char[sizeof(transpec_t) +
        2*(num-1)*sizeof(double)];
    t->tstart = ts;
    t->nparts = num;
    num *= 2;
    for (int i = 0; i < num; i++) {
        if (v[i] <= 0.0) {
            delete t;
            return (0);
        }
        if (i == 1) {
            if (v[i] <= ts) {
                delete t;
                return (0);
            }
        }
        else if (i > 2 && (i & 1) && v[i] <= v[i-2]) {
            delete t;
            return (0);
        }
        t->vals[i] = v[i];
    }
    return (t);
}


// Return the total number of output points for the scale.
//
int
transpec_t::points(const sCKT *ckt)
{
    int n = 0;
    double tch = tstart;
    double tnd = end(nparts-1);
    tnd += ckt->CKTcurTask->TSKminBreak;
    for (;;) {
        n++;
        if (n  == 0x7fffffff) {
            // Uh-oh, someone gave bad values and we've overrun the
            // integer point count.  Return a max int and hope for the
            // best.
            break;
        }
        int prt = part(tch + ckt->CKTcurTask->TSKminBreak);
        tch += step(prt);
        if (tch >= tnd)
            break;
    }
    return (n);
}
       

transpec_t *
transpec_t::dup()
{
    int sz = sizeof(transpec_t) + 2*(nparts-1)*sizeof(double);
    transpec_t *t = (transpec_t*)new char[sz];
    memcpy(t, this, sz);
    return (t);
}


// Return the region index holding d.
//
int
transpec_t::part(double d)
{
    for (int i = 0; i < nparts; i++) {
        if (d < end(i))
            return (i);
    }
    return (nparts-1);
}


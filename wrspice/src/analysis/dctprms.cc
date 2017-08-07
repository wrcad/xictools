
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

#include "device.h"
#include "outdata.h"
#include "frontend.h"
#include "miscutil/threadpool.h"


//#define TH_DEBUG

//
// DC sweep driver object.  This is used in analyses that allow
// chaining to a DC sweep.
//

// Obtain a parameter value.
//
int 
sDCTprms::query(int which, IFdata *data) const
{
    IFvalue *value = &data->v;

    switch (which) {
    case DC_NAME1:
#ifdef ALLPRMS
        value->sValue = dct_eltRef[0];
        data->type = IF_STRING;
#else
        value->uValue = dct_eltName[0];
        data->type = IF_INSTANCE;
#endif
        break;

    case DC_START1:
        value->rValue = dct_vstart[0];
        data->type = IF_REAL;
        break;

    case DC_STOP1:
        value->rValue = dct_vstop[0];
        data->type = IF_REAL;
        break;

    case DC_STEP1:
        value->rValue = dct_vstep[0];
        data->type = IF_REAL;
        break;

    case DC_NAME2:
#ifdef ALLPRMS
        value->sValue = dct_eltRef[1];
        data->type = IF_STRING;
#else
        value->uValue = dct_eltName[1];
        data->type = IF_INSTANCE;
#endif
        break;

    case DC_START2:
        value->rValue = dct_vstart[1];
        data->type = IF_REAL;
        break;

    case DC_STOP2:
        value->rValue = dct_vstop[1];
        data->type = IF_REAL;
        break;

    case DC_STEP2:
        value->rValue = dct_vstep[1];
        data->type = IF_REAL;
        break;

    default:
        return (E_BADPARM);
    }
    return (OK);
}


// Set a parameter value.
//
int 
sDCTprms::setp(int which, IFdata *data)
{
    IFvalue *value = &data->v;

    switch (which) {
    case DC_NAME1:
        dct_nestLevel = 0;
#ifdef ALLPRMS
        {
            char *s = lstring::copy(value->sValue);
            delete [] dct_eltRef[0];
            dct_eltRef[0] = s;
        }
#else
        dct_eltName[0] = value->uValue;
#endif
        break;

    case DC_START1:
        dct_vstart[0] = value->rValue;
        break;

    case DC_STOP1:
        dct_vstop[0] = value->rValue;
        break;

    case DC_STEP1:
        dct_vstep[0] = value->rValue;
        break;

    case DC_NAME2:
#ifdef ALLPRMS
        {
            char *s = lstring::copy(value->sValue);
            delete [] dct_eltRef[1];
            dct_eltRef[1] = s;
        }
#else
        dct_eltName[1] = value->uValue;
#endif
        break;

    case DC_START2:
        dct_vstart[1] = value->rValue;
        break;

    case DC_STOP2:
        dct_vstop[1] = value->rValue;
        break;

    case DC_STEP2:
        dct_vstep[1] = value->rValue;
        dct_nestLevel = 1;
        break;

    default:
        return (E_BADPARM);
    }
    return (OK);
}


namespace {
    // Parse device[param].
    //
    inline void parse_ref(const sCKT *ckt, const char *str, IFuid *dev,
        char **param)
    {
        if (!str) {
            *dev = 0;
            *param = 0;
            return;
        }
        char *s = lstring::copy(str);
        char *t = strrchr(s, '[');
        if (t) {
            char *e = t + strlen(t) - 1;
            if (*e == ']') {
                *t++ = 0;
                *e = 0;
                if (*t)
                    *param = lstring::copy(t);
                else
                    *param = 0;
                ckt->insert(&s);
                *dev = s;
                return;
            }
        }
        ckt->insert(&s);
        *dev = s;
        *param = 0;
    }
}


// Minimum voltage step.
#define VSTEPMIN 1e-8
#ifdef ALLPRMS
#define STEPFCT 1e-7
#endif

// Resolve the sources and check/fix parameter values.
//
int
sDCTprms::init(const sCKT *ckt)
{
#ifdef ALLPRMS
    for (int i = 0; i <= dct_nestLevel; i++) {
        IFuid nm;
        char *pm;
        parse_ref(ckt, dct_eltRef[i], &nm, &pm);
        if (!nm)
            break;
        GCarray<char*> gc_pm(pm);

        sGENinstance *here = 0;
        int type = -1;
        int error = ckt->findInst(&type, &here, nm, 0, 0);
        if (error) {
            OP.error(ERR_FATAL, "DCtrCurv: device %s not found in circuit",
                (const char*)nm);
            return (E_NOTFOUND);
        }
        dct_elt[i] = here;

        if (!pm) {
            const char *cpm = sDevLib::def_param((const char*)nm);
            if (!cpm) {
                OP.error(ERR_FATAL,
                    "DCtrCurv: no parameter given for device %s",
                    (const char*)nm);
                return (E_NOTFOUND);
            }
            pm = lstring::copy(cpm);
        }
        IFparm *p = DEV.device(type)->findInstanceParm(pm, IF_ASK);
        if (!p) {
            OP.error(ERR_FATAL,
                "DCtrCurv: parameter %s not found in device %s",
                pm, (const char*)nm);
            return (E_NOTFOUND);
        }
        dct_param[i] = p->id;

        IFdata data;
        error = here->askParam(ckt, p->id, &data);
        if (error) {
            OP.error(ERR_FATAL,
                "DCtrCurv: parameter %s not found in device %s",
                pm, (const char*)nm);
            return (E_NOTFOUND);  // can't happen...
        }
        if ((data.type & IF_VARTYPES) == IF_REAL)
            dct_vsave[i] = data.v.rValue;
        else {
            OP.error(ERR_FATAL,
                "DCtrCurv: parameter %s not real valued", pm);
            return (E_BADPARM);
        }
    }

    // The parameters can be anything, so VSTEPMIN is too large.  Allow
    // 1e7 steps, which should be (hopefully) plenty.

    for (int i = 0; i <= dct_nestLevel; i++) {
        double stepmin = STEPFCT*fabs(dct_vstart[i] - dct_vstop[i]);
        if (stepmin == 0.0)
            stepmin = VSTEPMIN;
        if (dct_vstep[i] == 0) {
            // pass through loop once
            dct_vstep[i] = dct_vstop[i] - dct_vstart[i];
            if (dct_vstep[i] >= 0)
                dct_vstep[i] += 2*stepmin;
            else
                dct_vstep[i] -= 2*stepmin;
        }
        else if ((dct_vstop[i] - dct_vstart[i] > 0 && dct_vstep[i] < 0) ||
                (dct_vstop[i] - dct_vstart[i] < 0 && dct_vstep[i] > 0)) {
            OP.error(ERR_WARNING, 
                "Changing sign of STEP%d", i+1);
            dct_vstep[i] = -dct_vstep[i];
        }
        if (dct_vstep[i] > 0 && dct_vstep[i] < stepmin) {
            OP.error(ERR_WARNING, 
                "STEP%d reset to STEPMIN=%g", i+1, stepmin);
            dct_vstep[i] = stepmin;
        }
        if (dct_vstep[i] < 0 && dct_vstep[i] > -stepmin) {
            OP.error(ERR_WARNING, 
                "STEP%d reset to STEPMIN=-%g", i+1, stepmin);
            dct_vstep[i] = -stepmin;
        }
    }
#else
    if (dct_eltName[0] != 0) {
        // DC source was given
        int code = ckt->typelook("Source");

        for (int i = 0; i <= dct_nestLevel; i++) {
            sGENinstance *here = 0;
            int error = ckt->findInst(&code, &here, dct_eltName[i], 0, 0);
            if (error) {
                OP.error(ERR_FATAL, "DCtrCurv: source %s not in circuit",
                    dct_eltName[i]);
                return (E_NOTFOUND);
            }
            sGENSRCinstance *sinst = (sGENSRCinstance*)here;
            dct_elt[i] = sinst;
            dct_vsave[i] = sinst->SRCdcValue;
        }
    }

    // fix any bogusness
    for (int i = 0; i <= dct_nestLevel; i++) {
        if (dct_vstep[i] == 0) {
            // pass through loop once
            dct_vstep[i] = dct_vstop[i] - dct_vstart[i];
            if (dct_vstep[i] >= 0)
                dct_vstep[i] += 2*VSTEPMIN;
            else
                dct_vstep[i] -= 2*VSTEPMIN;
        }
        else if ((dct_vstop[i] - dct_vstart[i] > 0 && dct_vstep[i] < 0) ||
                (dct_vstop[i] - dct_vstart[i] < 0 && dct_vstep[i] > 0)) {
            OP.error(ERR_WARNING, 
                "Changing sign of VSTEP%d", i+1);
            dct_vstep[i] = -dct_vstep[i];
        }
        if (dct_vstep[i] > 0 && dct_vstep[i] < VSTEPMIN) {
            OP.error(ERR_WARNING, 
                "VSTEP%d reset to VSTEPMIN=%g", i+1, VSTEPMIN);
            dct_vstep[i] = VSTEPMIN;
        }
        if (dct_vstep[i] < 0 && dct_vstep[i] > -VSTEPMIN) {
            OP.error(ERR_WARNING, 
                "VSTEP%d reset to -VSTEPMIN=-%g", i+1, VSTEPMIN);
            dct_vstep[i] = -VSTEPMIN;
        }
    }
#endif
    return (OK);
}


// Count the number of points to evaluate.  We actually run the loop
// so as to be sure to get this right.
//
int
sDCTprms::points(const sCKT *ckt)
{
    double values[DCTNESTLEVEL];
    for (int i = 0; i <= dct_nestLevel; i++)
        values[i] = dct_vstart[i];

    int nseq = 0;
    int i = 0;
    bool do_last = ckt->CKTcurTask->TSKdcOddStep;
    for (;;) {
        bool do_me = true;
        double tt = values[i] - dct_vstop[i];
#ifdef ALLPRMS
        double stepmin = STEPFCT*fabs(dct_vstart[i] - dct_vstop[i]);
        if (stepmin == 0.0)
            stepmin = VSTEPMIN;
        if ((dct_vstep[i] > 0 && tt > stepmin) ||
                (dct_vstep[i] < 0 && tt < -stepmin)) {
            if (do_last && fabs(fabs(tt) - fabs(dct_vstep[i])) > stepmin)
#else
        if ((dct_vstep[i] > 0 && tt > VSTEPMIN) ||
                (dct_vstep[i] < 0 && tt < -VSTEPMIN)) {
            if (do_last && fabs(fabs(tt) - fabs(dct_vstep[i])) > VSTEPMIN)
#endif
                values[i] = dct_vstop[i];
            else {
                i++; 
                if (i > dct_nestLevel)
                    break;
                do_me = false;
            }
        }
        if (do_me) {
            while (i > 0) { 
                i--; 
                values[i] = dct_vstart[i];
            }
            // do operation
            nseq++;
        }
        values[i] += dct_vstep[i];
    }
    return (nseq);
}


int
sDCTprms::loop(LoopWorkFunc func, sCKT *ckt, int restart)
{
    if (dct_elt[0] == 0) {
        // No swept elements specified!
        return ((*func)(ckt, restart));
    }

    if (restart) {
        dct_dims[0] = dct_dims[1] = dct_dims[2] = 0;
        dct_nestSave = 0;
        dct_skip = 0;
    }
    ckt->CKTinitV1 = dct_vstart[0];
    ckt->CKTfinalV1 = dct_vstop[0];
    ckt->CKTinitV2 = dct_nestLevel ? dct_vstart[0] : 0.0;
    ckt->CKTfinalV2 = dct_nestLevel ? dct_vstop[0] : 0.0;

#ifdef ALLPRMS
    int error;
    IFdata data;
    data.type = IF_REAL;
    for (int i = 0; i <= dct_nestLevel; i++) {
        data.v.rValue = dct_vstart[i];
        error = dct_elt[i]->setParam(dct_param[i], &data);
        if (error)
            return (error);
    }
#else
    for (int i = 0; i <= dct_nestLevel; i++)
        dct_elt[i]->SRCdcValue = dct_vstart[i];
#endif

    sJOB *job = ckt->CKTcurJob;
#ifdef WITH_THREADS
    if (job->threadable() && ckt->CKTcurTask->TSKloopThreads > 0 &&
            points(ckt) > 1)
        return (loop_mt(func, ckt, restart));
#endif

    // Setup progress reporting.
    ckt->CKTnumDC = points(ckt);

    sOUTdata *outd = job->JOBoutdata;
    int i = dct_nestSave;
    bool first = restart;
    bool do_last = ckt->CKTcurTask->TSKdcOddStep;
    for (;;) {
        bool do_me = true;

#ifdef ALLPRMS
        error = dct_elt[i]->askParam(ckt, dct_param[i], &data);
        if (error)
            return (error);
        double tt = data.v.rValue - dct_vstop[i];
#else
        int error;
        double tt = dct_elt[i]->SRCdcValue - dct_vstop[i];
#endif
#ifdef ALLPRMS
        double stepmin = STEPFCT*fabs(dct_vstart[i] - dct_vstop[i]);
        if (stepmin == 0.0)
            stepmin = VSTEPMIN;
        if (!dct_skip && ((dct_vstep[i] > 0 && tt > stepmin) ||
                (dct_vstep[i] < 0 && tt < -stepmin))) {
            if (do_last && fabs(fabs(tt) - fabs(dct_vstep[i])) > stepmin) {
#else
        if (!dct_skip && ((dct_vstep[i] > 0 && tt > VSTEPMIN) ||
                (dct_vstep[i] < 0 && tt < -VSTEPMIN))) {
            if (do_last && fabs(fabs(tt) - fabs(dct_vstep[i])) > VSTEPMIN) {
#endif
#ifdef ALLPRMS
                data.v.rValue = dct_vstop[i];
                error = dct_elt[i]->setParam(dct_param[i], &data);
                if (error)
                    return (error);
#else
                dct_elt[i]->SRCdcValue = dct_vstop[i];
#endif
            }
            else {
                i++; 
                ckt->CKTmode =
                    (ckt->CKTmode & MODEUIC) | MODEDCTRANCURVE | MODEINITJCT;
                if (i > dct_nestLevel)
                    break;
                dct_dims[0]++;
                if (dct_dims[2] <= 1)
                    OP.setDims(job->JOBrun, dct_dims, 2);
                else
                    OP.setDims(job->JOBrun, dct_dims, 3);
                do_me = false;
            }
        }
        if (do_me) {
            if (!dct_skip) {
                if (!dct_dims[0])
                    dct_dims[1]++;
            
                while (i > 0) { 
                    i--; 
#ifdef ALLPRMS
                    data.v.rValue = dct_vstart[i];
                    error = dct_elt[i]->setParam(dct_param[i], &data);
                    if (error)
                        return (error);
#else
                    dct_elt[i]->SRCdcValue = dct_vstart[i];
#endif
                }

                outd->count = 0;
            }
            // do operation
            dct_skip = 0;
            OP.initMeasure(job->JOBrun);
#ifdef ALLPRMS
            ckt->doTaskSetup();
#endif
            error = (*func)(ckt, restart);
            ckt->CKTcntDC++;  // For progress reporting.
            if (first) {
                OP.setDC(job->JOBrun, this);
                first = false;
            }
            if (error) {
                // save state, put everything back to normal
                dct_nestSave = i;
                dct_skip = 1;
#ifdef ALLPRMS
                int ret = error;
                for (i = 0; i <= dct_nestLevel; i++) {
                    error = dct_elt[i]->askParam(ckt, dct_param[i], &data);
                    if (error)
                        return (error);
                    dct_vstate[i] = data.v.rValue;

                    data.v.rValue = dct_vsave[i];
                    error = dct_elt[i]->setParam(dct_param[i], &data);
                    if (error)
                        return (error);
                }
                return (ret);
#else
                for (i = 0; i <= dct_nestLevel; i++) {
                    dct_vstate[i] = dct_elt[i]->SRCdcValue;
                    dct_elt[i]->SRCdcValue = dct_vsave[i];
                }
                return (error);
#endif
            }
            // so next sub-analysis starts properly
            restart = true;

            // store block size in dims[]
            dct_dims[2] = outd->count;

            if (dct_dims[2] > 1) {

                if (dct_dims[0])
                    OP.setDims(job->JOBrun, dct_dims, 3);
                else
                    OP.setDims(job->JOBrun, dct_dims+1, 2);
            }
            if (OP.endit()) {
                OP.set_endit(false);
                for (i = 0; i <= dct_nestLevel; i++) {
#ifdef ALLPRMS
                    error = dct_elt[i]->askParam(ckt, dct_param[i], &data);
                    if (error)
                        return (error);
                    dct_vstate[i] = data.v.rValue;

                    data.v.rValue = dct_vsave[i];
                    error = dct_elt[i]->setParam(dct_param[i], &data);
                    if (error)
                        return (error);
#else
                    dct_vstate[i] = dct_elt[i]->SRCdcValue;
                    dct_elt[i]->SRCdcValue = dct_vsave[i];
#endif
                }
                return (OK);
            }
        }

#ifdef ALLPRMS
        error = dct_elt[i]->askParam(ckt, dct_param[i], &data);
        if (error)
            return (error);
        data.v.rValue += dct_vstep[i];
        error = dct_elt[i]->setParam(dct_param[i], &data);
        if (error)
            return (error);
#else
        dct_elt[i]->SRCdcValue += dct_vstep[i];
#endif

        if ((error = OP.pauseTest(job->JOBrun)) < 0) {
            // pause requested, save state
            dct_nestSave = i;
            dct_skip = 0;
            // save state, put everything back to normal
#ifdef ALLPRMS
            int ret = error;
            for (i = 0; i <= dct_nestLevel; i++) {
                error = dct_elt[i]->askParam(ckt, dct_param[i], &data);
                if (error)
                    return (error);
                dct_vstate[i] = data.v.rValue;

                data.v.rValue = dct_vsave[i];
                error = dct_elt[i]->setParam(dct_param[i], &data);
                if (error)
                    return (error);
            }
            return (ret);
#else
            for (i = 0; i <= dct_nestLevel; i++) {
                dct_vstate[i] = dct_elt[i]->SRCdcValue;
                dct_elt[i]->SRCdcValue = dct_vsave[i];
            }
            return (error);
#endif
        }
    }

    // Done, put everything back to normal.
    for (i = 0; i <= dct_nestLevel; i++) {
#ifdef ALLPRMS
        data.v.rValue = dct_vsave[i];
        error = dct_elt[i]->setParam(dct_param[i], &data);
        if (error)
            return (error);
#else
        dct_elt[i]->SRCdcValue = dct_vsave[i];
#endif
    }
    return (OK);
}


#ifdef WITH_THREADS

// Per-thread data.  Each thread will have its own circuit object.
//
struct sDCTthCx : public sTPthreadData
{
    sDCTthCx(sCKT *c, bool keepckt = false)
        {
#ifdef TH_DEBUG
            printf("create %lx %d\n", (unsigned long)this, keepckt);
#endif
            cx_ckt = c;
            cx_keepckt = keepckt;
            for (int i = 0; i < DCTNESTLEVEL; i++) {
                cx_elt[i] = 0;
#ifdef ALLPRMS
                cx_param[i] = 0;
#endif
            }
        }

    ~sDCTthCx()
        {
#ifdef TH_DEBUG
            printf("delete %lx %d\n", (unsigned long)this, cx_keepckt);
#endif
            if (!cx_keepckt)
                delete cx_ckt;
        }

    sCKT *ckt()
        {
            return (cx_ckt);
        }

#ifdef ALLPRMS
    sGENinstance *elt(unsigned int i)
        {
            if (i >= DCTNESTLEVEL)
                return (0);
            return (cx_elt[i]);
        }

    void set_elt(sGENinstance *a, int i)
        {
            if (i >= DCTNESTLEVEL)
                return;
            cx_elt[i] = a;
        }

    int param(unsigned int i)
        {
            if (i >= DCTNESTLEVEL)
                return (-1);
            return (cx_param[i]);
        }

    void set_param(int n, int i)
        {
            if (i >= DCTNESTLEVEL)
                return;
            cx_param[i] = n;
        }
#else
    sGENSRCinstance *elt(unsigned int i)
        {
            if (i >= DCTNESTLEVEL)
                return (0);
            return (cx_elt[i]);
        }

    void set_elt(sGENSRCinstance *a, int i)
        {
            if (i >= DCTNESTLEVEL)
                return;
            cx_elt[i] = a;
        }
#endif

private:
    sCKT *cx_ckt;
#ifdef ALLPRMS
    sGENinstance *cx_elt[DCTNESTLEVEL];
    int cx_param[DCTNESTLEVEL];
#else
    sGENSRCinstance *cx_elt[DCTNESTLEVEL];
#endif
    bool cx_keepckt;
};


struct sDCTrun
{
    sDCTrun(LoopWorkFunc f, double *v, int n, int *d)
        {
            next = 0;
            func = f;
            values[0] = v[0];
            values[1] = v[1];
            seqnum = n;
            dims[0] = d[0];
            dims[1] = d[1];
            dims[2] = d[2];
        }

    sDCTrun *next;
    LoopWorkFunc func;
    double values[DCTNESTLEVEL];
    int seqnum;
    int dims[3];
};


namespace {
    // The thread work procedure.
    //
    int dct_thread_proc(sTPthreadData *data, void *arg)
    {
        sDCTthCx *cx = (sDCTthCx*)data;
        sDCTrun *run = (sDCTrun*)arg;

        IFdata tdata;
        tdata.type = IF_REAL;
        for (int i = 0; i < DCTNESTLEVEL; i++) {
            if (!cx->elt(i))
                break;
#ifdef ALLPRMS
            tdata.v.rValue = run->values[i];
            int error = cx->elt(i)->setParam(cx->param(i), &tdata);
            if (error)
                return (error);
#else
            cx->elt(i)->SRCdcValue = run->values[i];
#endif
        }
        sCKT *ckt = cx->ckt();
        sJOB *job = ckt->CKTcurJob;
        sOUTdata *outd = job->JOBoutdata;

        // The cycle field being nonzero signals that multi-threading
        // is being used.  This is used to compute the offset for
        // output data from the thread in the plot dvecs.
        outd->cycle = 1 + run->seqnum;
        outd->count = 0;

#ifdef TH_DEBUG
        printf("running %d %lx", run->seqnum, (long)data);
        for (int i = 0; i < DCTNESTLEVEL; i++) {
            if (!cx->elt(i))
                break;
            printf(" %g", run->values[i]);
        }
        printf("\n");
#endif

#ifdef ALLPRMS
        cx->ckt()->doTaskSetup();
#endif
        OP.initMeasure(job->JOBrun);
        int error = (*run->func)(ckt, true);
        return (error);
    }
}


// Multi-threaded loop function.
//
int
sDCTprms::loop_mt(LoopWorkFunc func, sCKT *ckt, int restart)
{
    (void)restart;
    sJOB *job = ckt->CKTcurJob;

    // Run the loop, creating a linked list of analysis conditions.

    sDCTrun *v0 = 0, *ve = 0;
    int nseq = 0;

    double values[DCTNESTLEVEL];
    for (int i = 0; i <= dct_nestLevel; i++)
        values[i] = dct_vstart[i];

    // store block size in dims[]
    dct_dims[2] = job->points(ckt);

    int i = 0;
    bool do_last = ckt->CKTcurTask->TSKdcOddStep;
    for (;;) {
        bool do_me = true;
        double tt = values[i] - dct_vstop[i];
#ifdef ALLPRMS
        double stepmin = STEPFCT*fabs(dct_vstart[i] - dct_vstop[i]);
        if (stepmin == 0.0)
            stepmin = VSTEPMIN;
        if ((dct_vstep[i] > 0 && tt > stepmin) ||
                (dct_vstep[i] < 0 && tt < -stepmin)) {
            if (do_last && fabs(fabs(tt) - fabs(dct_vstep[i])) > stepmin)
#else
        if ((dct_vstep[i] > 0 && tt > VSTEPMIN) ||
                (dct_vstep[i] < 0 && tt < -VSTEPMIN)) {
            if (do_last && fabs(fabs(tt) - fabs(dct_vstep[i])) > VSTEPMIN)
#endif
                values[i] = dct_vstop[i];
            else {
                i++; 
                if (i > dct_nestLevel)
                    break;
                dct_dims[0]++;
                do_me = false;
            }
        }
        if (do_me) {
            if (!dct_dims[0])
                dct_dims[1]++;
        
            while (i > 0) { 
                i--; 
                values[i] = dct_vstart[i];
            }
            // do operation
            if (!v0)
                v0 = ve = new sDCTrun(func, values, nseq, dct_dims);
            else {
                ve->next = new sDCTrun(func, values, nseq, dct_dims);
                ve = ve->next;
            }
            nseq++;
        }
        values[i] += dct_vstep[i];
    }

    if (dct_dims[2] <= 1) {
        if (dct_dims[0])
            OP.setDims(job->JOBrun, dct_dims, 2);
        else
            OP.setDims(job->JOBrun, dct_dims+1, 1);
    }
    else if (dct_dims[0])
        OP.setDims(job->JOBrun, dct_dims, 3);
    else if (dct_dims[1])
        OP.setDims(job->JOBrun, dct_dims+1, 2);
    else
        OP.setDims(job->JOBrun, dct_dims+2, 1);

    int nth = ckt->CKTcurTask->TSKloopThreads;  // number of threads
    if (nth > nseq-1)
        nth = nseq-1;
    ckt->CKTstat->STATloopThreads = nth;

    cThreadPool tp(nth);

    // Create the per-thread data, which consists of separate circuit
    // objects.

#ifdef ALLPRMS
#else
    int code = ckt->typelook("Source");
#endif
    for (int j = 0; j < nth; j++) {
        sCKT *tckt;
        int err = ckt->CKTbackPtr->newCKT(&tckt, 0);
        if (err != OK)
            return (err);
        tckt->CKTthreadId = j+1;

        sTASK *ttsk = ckt->CKTcurTask->dup();
        sJOB *tjob = job->dup();
        if (!tjob) {
            // Can't thread this analysis.
            return (E_PANIC);
        }

        ttsk->TSKjobs = tjob;
        tckt->CKTcurTask = ttsk;
        tckt->CKTcurJob = tjob;
        sDCTthCx *cx = new sDCTthCx(tckt);
        tp.setThreadData(cx, j);
        for (i = 0; i <= dct_nestLevel; i++) {
#ifdef ALLPRMS
            IFuid nm;
            char *pm;
            parse_ref(tckt, dct_eltRef[i], &nm, &pm);
            if (!nm)
                break;
            GCarray<char*> gc_pm(pm);

            sGENinstance *here = 0;
            int type = -1;
            int error = tckt->findInst(&type, &here, nm, 0, 0);
            if (error) {
                OP.error(ERR_FATAL, "DCtrCurv: device %s not found in circuit",
                    (const char*)nm);
                return (E_NOTFOUND);
            }
            cx->set_elt(here, i);

            if (!pm) {
                const char *cpm = sDevLib::def_param((const char*)nm);
                if (!cpm) {
                    OP.error(ERR_FATAL,
                        "DCtrCurv: no parameter given for device %s",
                        (const char*)nm);
                    return (E_NOTFOUND);
                }
                pm = lstring::copy(cpm);
            }
            IFparm *p = DEV.device(type)->findInstanceParm(pm, IF_ASK);
            if (!p) {
                OP.error(ERR_FATAL,
                    "DCtrCurv: parameter %s not found in device %s",
                    pm, (const char*)nm);
                return (E_NOTFOUND);
            }
            cx->set_param(p->id, i);
#else
           sGENinstance *here = 0;
            int error = tckt->findInst(&code, &here, dct_eltName[i], 0, 0);
            if (error) {
                OP.error(ERR_FATAL, "DCtrCurv: source %s not in circuit",
                    dct_eltName[i]);
                return (E_NOTFOUND);
            }
            cx->set_elt((sGENSRCinstance*)here, i);
#endif
        }
#ifdef ALLPRMS
#else
        err = tckt->doTaskSetup();
        if (err != OK)
            return (err);
#endif
        err = tjob->init(tckt);
        if (err != OK)
            return (err);
    }

    // Set up the thread jobs.
    for (sDCTrun *v = v0; v; v = v->next)
        tp.submit(dct_thread_proc, v);

    // Create a dummy context for the main thread, and run the pool.
    sDCTthCx tcx(ckt, true);
    for (i = 0; i <= dct_nestLevel; i++) {
#ifdef ALLPRMS
        IFuid nm;
        char *pm;
        parse_ref(ckt, dct_eltRef[i], &nm, &pm);
        if (!nm)
            break;
        GCarray<char*> gc_pm(pm);

        sGENinstance *here = 0;
        int type = -1;
        int error = ckt->findInst(&type, &here, nm, 0, 0);
        if (error) {
            OP.error(ERR_FATAL, "DCtrCurv: device %s not found in circuit",
                (const char*)nm);
            return (E_NOTFOUND);
        }
        tcx.set_elt(here, i);

        if (!pm) {
            const char *cpm = sDevLib::def_param((const char*)nm);
            if (!cpm) {
                OP.error(ERR_FATAL,
                    "DCtrCurv: no parameter given for device %s",
                    (const char*)nm);
                return (E_NOTFOUND);
            }
            pm = lstring::copy(cpm);
        }
        IFparm *p = DEV.device(type)->findInstanceParm(pm, IF_ASK);
        if (!p) {
            OP.error(ERR_FATAL,
                "DCtrCurv: parameter %s not found in device %s",
                pm, (const char*)nm);
            return (E_NOTFOUND);
        }
        tcx.set_param(p->id, i);
#else
        sGENinstance *here = 0;
        int error = ckt->findInst(&code, &here, dct_eltName[i], 0, 0);
        if (error) {
            OP.error(ERR_FATAL, "DCtrCurv: source %s not in circuit",
                dct_eltName[i]);
            return (E_NOTFOUND);
        }
        tcx.set_elt((sGENSRCinstance*)here, i);
#endif
    }
    int err = tp.run(&tcx);
#ifdef TH_DEBUG
    printf("run done %d\n", err);
#endif

    // Done, put everything back to normal in the main thread.
    for (i = 0; i <= dct_nestLevel; i++) {
#ifdef ALLPRMS
        IFdata data;
        data.type = IF_REAL;
        data.v.rValue = dct_vsave[i];
        int error = dct_elt[i]->setParam(dct_param[i], &data);
        if (error)
            return (error);
#else
        dct_elt[i]->SRCdcValue = dct_vsave[i];
#endif
    }
    return (err);
}

#endif


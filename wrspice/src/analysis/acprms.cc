
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2015 Whiteley Research Inc, all rights reserved.        *
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
 $Id: acprms.cc,v 2.5 2016/10/16 01:33:22 stevew Exp $
 *========================================================================*/

#include "device.h"
#include "spmatrix.h"
#include "outdata.h"


//
// AC analysis driver.  Used in analyses that use an AC frequency
// sweep.
//

int 
sACprms::query(int which, IFdata *data) const
{
    IFvalue *value = &data->v;

    switch (which) {
    case AC_START:
        value->rValue = ac_fstart;
        data->type = IF_REAL;
        break;

    case AC_STOP:
        value->rValue = ac_fstop;
        data->type = IF_REAL;
        break;

    case AC_STEPS:
        value->iValue = ac_numSteps;
        data->type = IF_INTEGER;
        break;

    case AC_DEC:
        if (ac_stepType == DECADE)
            value->iValue = 1;
        else
            value->iValue = 0;
        data->type = IF_FLAG;
        break;

    case AC_OCT:
        if (ac_stepType == OCTAVE)
            value->iValue = 1;
        else
            value->iValue = 0;
        data->type = IF_FLAG;
        break;

    case AC_LIN:
        if (ac_stepType == LINEAR)
            value->iValue = 1;
        else
            value->iValue = 0;
        data->type = IF_FLAG;
        break;

    default:
        return (E_BADPARM);
    }
    return (OK);
}


int 
sACprms::setp(int which, IFdata *data)
{
    IFvalue *value = &data->v;

    switch (which) {
    case AC_START:
        ac_fstart = value->rValue;
        break;

    case AC_STOP:
        ac_fstop = value->rValue;
        break;

    case AC_STEPS:
        ac_numSteps = value->iValue;
        break;

    case AC_DEC:
        if (value->iValue)
            ac_stepType = DECADE;
        else if (ac_stepType == DECADE)
            ac_stepType = DCSTEP;
        break;

    case AC_OCT:
        if (value->iValue)
            ac_stepType = OCTAVE;
        else if (ac_stepType == OCTAVE)
            ac_stepType = DCSTEP;
        break;

    case AC_LIN:
        if (value->iValue)
            ac_stepType = LINEAR;
        else if (ac_stepType == LINEAR)
            ac_stepType = DCSTEP;
        break;

    default:
        return (E_BADPARM);
    }
    return (OK);
}


// Return the number of points to output.
//
int
sACprms::points(const sCKT *ckt)
{
    int npts = 0;
    double freqTol, freqDel;
    switch (ac_stepType) {
    case DECADE:
        if (ac_fstart <= 0.0)
            ac_fstart = 1e-3;
        if (ac_fstop < ac_fstart)
            ac_fstop = ac_fstart;
        if (ac_numSteps <= 0)
            ac_numSteps = 1;
        freqDel = exp(log(10.0)/ac_numSteps);
        if (ac_numSteps > 1) {
            // See if we can tweak freqDel so we land exactly on fstop.
            // This is useful when fstop is 2,4 or 8 * fstart, and
            // numSteps is 10, where this can be done.

            double t1 = ac_fstop/ac_fstart;
            if (t1 < 10.0) {
                double b = log10(t1);
                double del = 1.0/ac_numSteps;
                double t2 = b/del;
                int nn = (int)t2;
                double r = t2 - nn;
                if (r > 0.5) {
                    r = 1.0 - r;
                    nn++;
                }
                if (r < 0.05) {
                    del = b/nn;
                    freqDel = pow(10.0, del);
                }
            }
        }
        freqTol = freqDel * ac_fstop * ckt->CKTcurTask->TSKreltol;
        break;
    case OCTAVE:
        if (ac_fstart <= 0.0)
            ac_fstart = 1e-3;
        if (ac_fstop < ac_fstart)
            ac_fstop = ac_fstart;
        if (ac_numSteps <= 0)
            ac_numSteps = 1;
        freqDel = exp(log(2.0)/ac_numSteps);
        freqTol = freqDel * ac_fstop * ckt->CKTcurTask->TSKreltol;
        break;
    case LINEAR:
        if (ac_numSteps <= 0)
            ac_numSteps = 1;
        freqDel = (ac_fstop - ac_fstart)/(ac_numSteps+1);
        freqTol = freqDel * ckt->CKTcurTask->TSKreltol;
        break;
    default:
        return (1);
    }
    double freq = ac_fstart;

    while (freq <= ac_fstop + freqTol) {
        npts++;

        switch (ac_stepType) {
        case DECADE:
        case OCTAVE:
            freq *= freqDel;
            if (freqDel == 1)
                return (1);
            break;
        case LINEAR:
            freq += freqDel;
            if (freqDel == 0)
                return (1);
        default:
            break;
        }
    }
    return (npts);
}


int
sACprms::loop(LoopWorkFunc func, sCKT *ckt, int restart)
{
    double freqTol, freqDel;
    switch (ac_stepType) {
    case DECADE:
        if (ac_fstart <= 0.0)
            ac_fstart = 1e-3;
        if (ac_fstop < ac_fstart)
            ac_fstop = ac_fstart;
        if (ac_numSteps <= 0)
            ac_numSteps = 1;
        freqDel = exp(log(10.0)/ac_numSteps);
        if (ac_numSteps > 1) {
            // See if we can tweak freqDel so we land exactly on fstop.
            // This is useful when fstop is 2,4 or 8 * fstart, and
            // numSteps is 10, where this can be done.

            double t1 = ac_fstop/ac_fstart;
            if (t1 < 10.0) {
                double b = log10(t1);
                double del = 1.0/ac_numSteps;
                double t2 = b/del;
                int nn = (int)t2;
                double r = t2 - nn;
                if (r > 0.5) {
                    r = 1.0 - r;
                    nn++;
                }
                if (r < 0.05) {
                    del = b/nn;
                    freqDel = pow(10.0, del);
                }
            }
        }
        freqTol = freqDel * ac_fstop * ckt->CKTcurTask->TSKreltol;
        break;
    case OCTAVE:
        if (ac_fstart <= 0.0)
            ac_fstart = 1e-3;
        if (ac_fstop < ac_fstart)
            ac_fstop = ac_fstart;
        if (ac_numSteps <= 0)
            ac_numSteps = 1;
        freqDel = exp(log(2.0)/ac_numSteps);
        freqTol = freqDel * ac_fstop * ckt->CKTcurTask->TSKreltol;
        break;
    case LINEAR:
        if (ac_numSteps <= 0)
            ac_numSteps = 1;
        freqDel = (ac_fstop - ac_fstart)/(ac_numSteps+1);
        freqTol = freqDel * ckt->CKTcurTask->TSKreltol;
        break;
    case DCSTEP:
        return ((*func)(ckt, restart) );
    default:
        return (E_BADPARM);
    }
    if (ac_stepType != LINEAR)
        // set default to log scale 
        OP.setAttrs(ckt->CKTcurJob->JOBrun, 0, OUT_SCALE_LOG, 0);

    if (restart) {
        // reset static variables
        ac_fsave = ac_fstart;
    }
    double freq = ac_fsave;

    ckt->CKTinitFreq = ac_fstart * 2.0 * M_PI;
    ckt->CKTfinalFreq = ac_fstop * 2.0 * M_PI;

    int error;
    while (freq <= ac_fstop + freqTol) {

        if ((error = OP.pauseTest(ckt->CKTcurJob->JOBrun)) < 0) { 
            // pause request
            ac_fsave = freq;
            return (error);
        }
        ckt->CKTomega = 2.0 * M_PI * freq;
        ckt->CKTmode = MODEAC;

        error = (*func)(ckt, restart);
        if (error) {
            ac_fsave = freq;
            return (error);
        }
        if (OP.endit()) {
            OP.set_endit(false);
            return (OK);
        }

        //  increment frequency
        switch (ac_stepType) {
        case DECADE:
        case OCTAVE:
            freq *= freqDel;
            if (freqDel == 1)
                return (OK);
            break;
        case LINEAR:
            freq += freqDel;
            if (freqDel == 0)
                return (OK);
            break;
        default:
            return (E_INTERN);
        }
    }
    return (OK);
}


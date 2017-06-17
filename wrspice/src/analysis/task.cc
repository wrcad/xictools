
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
 $Id: task.cc,v 2.5 2015/08/06 00:57:02 stevew Exp $
 *========================================================================*/

#include "circuit.h"
#include "outdata.h"
#include "lstring.h"


sTASK::~sTASK()
{
    delete TSKshellOpts;
    while (TSKjobs) {
        sJOB *jx = TSKjobs;
        TSKjobs = TSKjobs->JOBnextJob;
        delete jx;
    }
}

 
// Create a new job, linked into task.
//
int
sTASK::newAnal(int type, sJOB **analPtr)
{
    IFanalysis *an = IFanalysis::analysis(type);
    if (an) {
        sJOB *job = an->newAnal();
        if (job) {
            job->JOBname = an->name;
            job->JOBtype = type;
            if ((void*)this) {
                job->JOBnextJob = TSKjobs;
                TSKjobs = job;
            }
            if (analPtr)
                *analPtr = job;
            return (OK);
        }
    }
    return (E_NOANAL);
}


// Find the given Analysis given its name and return the Analysis
// pointer.  If name is null, count the number of jobs.
//
int
sTASK::findAnal(int *numjobs, sJOB **anal, IFuid name)
{
    for (sJOB *job = TSKjobs; job; job = job->JOBnextJob) {
        if (!job->JOBname)
            continue;  // "can't happen"

        if (!name) {
            if (numjobs) {
                int n = 0;
                for ( ; job; job = job->JOBnextJob)
                    n++;
                *numjobs = n;
            }
            return (OK);
        }
        if (strcmp((const char*)job->JOBname, (const char*)name) == 0) {
            if (anal)
                *anal = job;
            return (OK);
        }
    }
    return (E_NOTFOUND);
}


// Set the task struct to represent the options set in optp, and if
// domerge is set, merge in the options set in sOPTIONS::shellOpts(). 
// Save a pointer to a copy of the sOPTIONS::shellOpts().
//
void
sTASK::setOptions(sOPTIONS *optp, bool domerge)
{
    if (!optp)
        return;

    TSKopts.setup(optp, OMRG_GLOBAL);
    OMRG_TYPE mt = OMRG_NOSHELL;
    if (domerge) {
        if (optp->OPToptmerge_given)
            mt = (OMRG_TYPE)optp->OPToptmerge;
        else
            mt = OMRG_GLOBAL;
    }

    if (mt == OMRG_GLOBAL || mt == OMRG_LOCAL) {
        TSKopts.setup(sOPTIONS::shellOpts(), mt);
        delete TSKshellOpts;
        TSKshellOpts = sOPTIONS::shellOpts()->copy();
    }
}
// End of sTASK functions.


sJOB::~sJOB()
{
    OP.endPlot(JOBrun, true);   
    delete JOBoutdata;
}


// Set an analysis parameter given the parameter id.
//
int
sJOB::setParam(int parm, IFdata *data)
{
    IFanalysis *an = IFanalysis::analysis(JOBtype);
    if (!an)
        return (E_NOANAL);
    return (an->setParm(this, parm, data));
}


// Set an analysis parameter given the parameter name.
//
int
sJOB::setParam(const char *pname, IFdata *data)
{
    if (pname) {
        IFanalysis *an = IFanalysis::analysis(JOBtype);
        if (an) {
            for (int i = 0; i < an->numParms; i++) {
                if (lstring::cieq(pname, an->analysisParms[i].keyword)) {
                    return (an->setParm(this,
                        an->analysisParms[i].id, data));
                }
            }
            return (E_BADPARM);
        }
    }
    return (E_NOANAL);
}


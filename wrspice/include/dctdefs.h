
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
Authors: 1985 Thomas L. Quarles
         1992 Stephen R. Whiteley
****************************************************************************/

#ifndef DCTDEFS_H
#define DCTDEFS_H

#include "circuit.h"

//
// Structures used to describe D.C. transfer curve analyses to
// be performed.
//

struct sDCTprms
{
    sDCTprms()
        {
            for (int i = 0; i < DCTNESTLEVEL; i++) {
                dct_vstart[i] = 0.0;
                dct_vstop[i] = 0.0;
                dct_vstep[i] = 0.0;
                dct_vsave[i] = 0.0;
                dct_vstate[i] = 0.0;
                dct_elt[i] = 0;
#ifdef ALLPRMS
                dct_eltRef[i] = 0;
                dct_param[i] = -1;
#else
                dct_eltName[i] = 0;
#endif
            }

            dct_nestLevel = 0;
            dct_nestSave = 0;

            for (int i = 0; i <= DCTNESTLEVEL; i++)
                dct_dims[i] = 0;

            dct_skip = 0;
        }

#ifdef ALLPRMS
    ~sDCTprms()
        {
            for (int i = 0; i < DCTNESTLEVEL; i++)
                delete [] dct_eltRef[i];
        }
#endif

    int query(int, IFdata*) const;
        int setp(int, IFdata*);
    int init(const sCKT*);
    int points(const sCKT*);
    int loop(LoopWorkFunc, sCKT*, int, int = 0);

    double vstart(unsigned int i)
        { return (i < DCTNESTLEVEL ? dct_vstart[i] : 0.0); }
    double vstop(unsigned int i)
        { return (i < DCTNESTLEVEL ? dct_vstop[i] : 0.0); }
    double vstep(unsigned int i)
        { return (i < DCTNESTLEVEL ? dct_vstep[i] : 0.0); }
#ifdef ALLPRMS
    sGENinstance *elt(unsigned int i)
        { return (i < DCTNESTLEVEL ? dct_elt[i] : 0); }
    int param(unsigned int i)
        { return (i < DCTNESTLEVEL ? dct_param[i] : -1); }
#else
    sGENSRCinstance *elt(unsigned int i)
        { return (i < DCTNESTLEVEL ? dct_elt[i] : 0); }
#endif
    void get_dims(int *d)
        { *d++ = dct_dims[0]; *d++ = dct_dims[1]; *d = dct_dims[2]; }
    int nestLevel()
        { return (dct_nestLevel); }
    void uninit()
        { 
            for (int i = 0; i < DCTNESTLEVEL; i++)
                dct_elt[i] = 0;
        }

private:
#ifdef WITH_THREADS
    int loop_mt(LoopWorkFunc, sCKT*, int);
#endif

    double dct_vstart[DCTNESTLEVEL];     // starting value
    double dct_vstop[DCTNESTLEVEL];      // ending value
    double dct_vstep[DCTNESTLEVEL];      // value step
    double dct_vsave[DCTNESTLEVEL];      // value of this parameter before
                                         //   analysis - to restore when done
    double dct_vstate[DCTNESTLEVEL];     // internal values saved during pause
#ifdef ALLPRMS
    sGENinstance *dct_elt[DCTNESTLEVEL]; // pointer to device
    char *dct_eltRef[DCTNESTLEVEL];      // device reference: devname[param]
    int dct_param[DCTNESTLEVEL];         // parameter id being varied
#else
    sGENSRCinstance *dct_elt[DCTNESTLEVEL]; // pointer to source
    IFuid dct_eltName[DCTNESTLEVEL];     // source being varied
#endif
    int dct_nestLevel;                   // number of levels of nesting
    int dct_nestSave;                    // iteration state during pause
    int dct_dims[DCTNESTLEVEL+1];        // dimensions of output vector
    int dct_skip;                        // restart subanalysis
};

struct sDCTAN : public sJOB
{
    virtual ~sDCTAN() { }

    virtual sJOB *dup()
        {
            sDCTAN *dct = new sDCTAN;
            dct->b_name = b_name;
            dct->b_type = b_type;
            dct->JOBoutdata = new sOUTdata(*JOBoutdata);
            dct->JOBrun = JOBrun;
            dct->JOBdc = JOBdc;
            dct->JOBdc.uninit();
            return (dct);
        }

    sDCTprms JOBdc;             // DC parameter storage
};

struct DCTanalysis : public IFanalysis
{
    // dctsetp.cc
    DCTanalysis();
    int setParm(sJOB*, int, IFdata*);

    // dctaskq.cc
    int askQuest(const sCKT*, const sJOB*, int, IFdata*) const;

    // dctprse.cc
    int parse(sLine*, sCKT*, int, const char**, sTASK*);

    // dctan.cc
    int anFunc(sCKT*, int);

    sJOB *newAnal() { return (new sDCTAN); }

private:
    static int dct_init(sCKT*);
    static int dct_operation(sCKT*, int);
};

extern DCTanalysis DCTinfo;

#endif // DCTDEFS_H


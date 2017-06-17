
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
 $Id: dctdefs.h,v 2.48 2015/08/07 04:03:51 stevew Exp $
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


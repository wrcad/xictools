
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
 $Id: dcodefs.h,v 2.45 2015/07/29 04:50:21 stevew Exp $
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3e2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 Thomas L. Quarles
         1992 Stephen R. Whiteley
****************************************************************************/

#ifndef DCODEFS_H
#define DCODEFS_H

#include "circuit.h"

//
// Structures used to describe D.C. operationg point analyses to
// be performed.
//

struct sDCOAN : public sJOB
{
    ~sDCOAN();
};

struct DCOanalysis : public IFanalysis
{
    // dcosetp.cc
    DCOanalysis();
    int setParm(sJOB*, int, IFdata*);

    // dcoaskq.cc
    int askQuest(const sCKT*, const sJOB*, int, IFdata*) const;

    // dcoprse.cc
    int parse(sLine*, sCKT*, int, const char**, sTASK*);

    // dcoan.cc
    int anFunc(sCKT*, int);

    sJOB *newAnal() { return (new sDCOAN); }
};

extern DCOanalysis DCOinfo;

#endif // DCODEFS_H


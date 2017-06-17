
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
 $Id: optdefs.h,v 2.61 2016/07/27 04:41:10 stevew Exp $
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3e2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 Thomas L. Quarles
         1992 Stephen R. Whiteley
****************************************************************************/

#ifndef OPTDEFS_H
#define OPTDEFS_H

#include "circuit.h"


enum OPTtype
{
    OPT_NOTUSED,

    // real values parameters
    OPT_ABSTOL,
    OPT_CHGTOL,
    OPT_DCMU,
    OPT_DEFAD,
    OPT_DEFAS,
    OPT_DEFL,
    OPT_DEFW,
    OPT_DELMIN,
    OPT_DPHIMAX,
    OPT_GMAX,
    OPT_GMIN,
    OPT_MAXDATA,
    OPT_MINBREAK,
    OPT_PIVREL,
    OPT_PIVTOL,
    OPT_RAMPUP,
    OPT_RELTOL,
    OPT_TEMP,
    OPT_TNOM,
    OPT_TRAPRATIO,
    OPT_TRTOL,
    OPT_VNTOL,
    OPT_XMU,

    // integer parameters
    OPT_BYPASS,
    OPT_FPEMODE,
    OPT_GMINSTEPS,
    OPT_INTERPLEV,
    OPT_ITL1,
    OPT_ITL2,
    OPT_ITL2GMIN,
    OPT_ITL2SRC,
    OPT_ITL4,
#ifdef WITH_THREADS
    OPT_LOADTHRDS,
    OPT_LOOPTHRDS,
#endif
    OPT_MAXORD,
    OPT_SRCSTEPS,

    // flags
    OPT_DCODDSTEP,
    OPT_EXTPREC,
    OPT_FORCEGMIN,
    OPT_GMINFIRST,
    OPT_HSPICE,
    OPT_JJACCEL,
    OPT_NOITER,
    OPT_NOJJTP,
    OPT_NOKLU,
    OPT_NOMATSORT,
    OPT_NOOPITER,
    OPT_NOSHELLOPTS,
    OPT_OLDLIMIT,
    OPT_OLDSTEPLIM,
    OPT_RENUMBER,
    OPT_SAVECURRENT,
    OPT_SPICE3,
    OPT_TRAPCHECK,
    OPT_TRYTOCOMPACT,
    OPT_USEADJOINT,

    // strings
    OPT_METHOD,
    OPT_OPTMERGE,
    OPT_PARHIER,
    OPT_STEPTYPE,

    // special parameters
    OPT_DELTA,
    OPT_EXTERNAL
};

struct OPTanalysis : public IFanalysis
{
    OPTanalysis();
    int parse(sLine*, sCKT*, int, const char**, sTASK*);
    int setParm(sJOB*, int, IFdata*);
    int askQuest(const sCKT*, const sJOB*, int, IFdata*) const;
    int anFunc(sCKT*, int) { return (0); };

    sJOB *newAnal() { return (new sOPTIONS); }
};

extern OPTanalysis OPTinfo;

#endif // OPTDEFS_H


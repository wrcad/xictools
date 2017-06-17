
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
 $Id: statsetp.cc,v 2.47 2015/08/07 05:32:04 stevew Exp $
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3f2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 Thomas L. Quarles
         1992 Stephen R. Whiteley
****************************************************************************/

#include "statdefs.h"
#include "errors.h"
#include "misc.h"
#include "kwords_analysis.h"


// Resource keywords
//
const char *stkw_accept         = "accept";
const char *stkw_cvchktime      = "cvchktime";
const char *stkw_equations      = "equations";
const char *stkw_fillin         = "fillin";
const char *stkw_involcxswitch  = "involcxswitch";
const char *stkw_loadtime       = "loadtime";
#ifdef WITH_THREADS
const char *stkw_loadthrds      = "loadthrds";
const char *stkw_loopthrds      = "loopthrds";
#endif
const char *stkw_lutime         = "lutime";
const char *stkw_matsize        = "matsize";
const char *stkw_nonzero        = "nonzero";
const char *stkw_pagefaults     = "pagefaults";
const char *stkw_rejected       = "rejected";
const char *stkw_reordertime    = "reordertime";
const char *stkw_solvetime      = "solvetime";
const char *stkw_time           = "time";
const char *stkw_totiter        = "totiter";
const char *stkw_trancuriters   = "trancuriters";
const char *stkw_traniter       = "traniter";
const char *stkw_tranitercut    = "tranitercut";
const char *stkw_tranlutime     = "tranlutime";
const char *stkw_tranouttime    = "tranouttime";
const char *stkw_tranpoints     = "tranpoints";
const char *stkw_transolvetime  = "transolvetime";
const char *stkw_trantime       = "trantime";
const char *stkw_trantrapcut    = "trantrapcut";
const char *stkw_trantstime     = "trantstime";
const char *stkw_volcxswitch    = "volcxswitch";

// Exported array to specify the keywords in a nice table order for
// printing.
//
const char *stat_print_array[] =
{
    stkw_matsize,
    stkw_equations,
    stkw_nonzero,
    stkw_fillin,
    "",
    stkw_pagefaults,
    stkw_volcxswitch,
    stkw_involcxswitch,
#ifdef WITH_THREADS
    stkw_loadthrds,
    stkw_loopthrds,
#endif
    "",
    stkw_totiter,
    stkw_traniter,
    stkw_trancuriters,
    stkw_tranpoints,
    stkw_accept,
    stkw_rejected,
    stkw_tranitercut,
    stkw_trantrapcut,
    "",
    stkw_time,
    stkw_trantime,
    stkw_loadtime,
    stkw_lutime,
    stkw_tranlutime,
    stkw_reordertime,
    stkw_solvetime,
    stkw_transolvetime,
    stkw_cvchktime,
    stkw_tranouttime,
    stkw_trantstime,
    0
};

namespace {
    IFparm STATtbl[] = {
        IFparm(stkw_accept,         ST_ACCEPT,          IF_ASK|IF_INTEGER,
            "Accepted timepoints"),
        IFparm(stkw_cvchktime,      ST_CVCHKTIME,       IF_ASK|IF_REAL,
            "Time spent convergence testing"),
        IFparm(stkw_equations,      ST_EQUATIONS,       IF_ASK|IF_INTEGER,
            "Circuit Equations"),
        IFparm(stkw_fillin,         ST_FILLIN,          IF_ASK|IF_INTEGER,
            "Fill-in terms from decomposition"),
        IFparm(stkw_involcxswitch,  ST_INVOLCXSWITCH,   IF_ASK|IF_INTEGER,
            "Involuntary context switches during analysis"),
        IFparm(stkw_loadtime,       ST_LOADTIME,        IF_ASK|IF_REAL,
            "Load time"),
#ifdef WITH_THREADS
        IFparm(stkw_loadthrds,      ST_LOADTHRDS,       IF_ASK|IF_INTEGER,
            "Number of device loading helper threads"),
        IFparm(stkw_loopthrds,      ST_LOOPTHRDS,       IF_ASK|IF_INTEGER,
            "Number of repetitive analysis helper threads"),
#endif
        IFparm(stkw_lutime,         ST_LUTIME,          IF_ASK|IF_REAL,
            "L-U decomposition time"),
        IFparm(stkw_matsize,        ST_MATSIZE,         IF_ASK|IF_INTEGER,
            "Matrix size"),
        IFparm(stkw_nonzero,        ST_NONZERO,         IF_ASK|IF_INTEGER,
            "Number of nonzero matrix entries"),
        IFparm(stkw_pagefaults,     ST_PGFAULTS,        IF_ASK|IF_INTEGER,
            "Number of page faults during analysis"),
        IFparm(stkw_rejected,       ST_REJECTED,        IF_ASK|IF_INTEGER,
            "Rejected timepoints"),
        IFparm(stkw_reordertime,    ST_REORDERTIME,     IF_ASK|IF_REAL,
            "Matrix reordering time"),
        IFparm(stkw_solvetime,      ST_SOLVETIME,       IF_ASK|IF_REAL,
            "Matrix solve time"),
        IFparm(stkw_time,           ST_TIME,            IF_ASK|IF_REAL,
            "Total Analysis Time"),
        IFparm(stkw_totiter,        ST_TOTITER,         IF_ASK|IF_INTEGER,
            "Total iterations"),
        IFparm(stkw_trancuriters,   ST_TRANCURITERS,    IF_ASK|IF_INTEGER,
            "Transient iters at last time point"),
        IFparm(stkw_traniter,       ST_TRANITER,        IF_ASK|IF_INTEGER,
            "Transient iterations"),
        IFparm(stkw_tranitercut,    ST_TRANITERCUT,     IF_ASK|IF_INTEGER,
            "Transient timepoints where iter limit exceeded"),
        IFparm(stkw_tranlutime,     ST_TRANLUTIME,      IF_ASK|IF_REAL,
            "Transient L-U decomp time"),
        IFparm(stkw_tranouttime,    ST_TRANOUTTIME,     IF_ASK|IF_REAL,
            "Transient data recording time"),
        IFparm(stkw_tranpoints,     ST_TRANPOINTS,      IF_ASK|IF_INTEGER,
            "Transient timepoints"),
        IFparm(stkw_transolvetime,  ST_TRANSOLVETIME,   IF_ASK|IF_REAL,
            "Transient solve time"),
        IFparm(stkw_trantime,       ST_TRANTIME,        IF_ASK|IF_REAL,
            "Transient time"),
        IFparm(stkw_trantrapcut,    ST_TRANTRAPCUT,     IF_ASK|IF_INTEGER,
            "Transient timepoints where trapcheck failed"),
        IFparm(stkw_trantstime,     ST_TRANTSTIME,      IF_ASK|IF_REAL,
            "Transient timestep computation time"),
        IFparm(stkw_volcxswitch,    ST_VOLCXSWITCH,     IF_ASK|IF_INTEGER,
            "Voluntary context switches during analysis"),
    };
}


STATanalysis::STATanalysis()
{
    name = "resources";
    description = "Statistics";
    numParms = sizeof(STATtbl)/sizeof(IFparm);
    analysisParms = STATtbl;
    domain = NODOMAIN;
};

STATanalysis STATinfo;


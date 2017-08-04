
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
         1993 Stephen R. Whiteley
****************************************************************************/

#include "optdefs.h"
#include "errors.h"
#include "cshell.h"
#include "ttyio.h"
#include "kwords_analysis.h"
#include "variable.h"


void
sOPTIONS::dump()
{
    IFvalue value;
    int notset;
    int nd = CP.NumDigits() > 0 ? CP.NumDigits() : 5;
    const char *dfmt = "%-16s %.*e";
    const char *ifmt = "%-16s %d";
    const char *sfmt = "%-16s %s";

    askOpt(OPT_ABSTOL, &value, &notset);
    if (!notset)
        TTY.printf(dfmt, spkw_abstol, nd, value.rValue);
    askOpt(OPT_CHGTOL, &value, &notset);
    if (!notset)
        TTY.printf(dfmt, spkw_chgtol, nd, value.rValue);
    askOpt(OPT_DCMU, &value, &notset);
    if (!notset)
        TTY.printf(dfmt, spkw_dcmu, nd, value.rValue);
    askOpt(OPT_DEFAD, &value, &notset);
    if (!notset)
        TTY.printf(dfmt, spkw_defad, nd, value.rValue);
    askOpt(OPT_DEFAS, &value, &notset);
    if (!notset)
        TTY.printf(dfmt, spkw_defas, nd, value.rValue);
    askOpt(OPT_DEFL, &value, &notset);
    if (!notset)
        TTY.printf(dfmt, spkw_defl, nd, value.rValue);
    askOpt(OPT_DEFW, &value, &notset);
    if (!notset)
        TTY.printf(dfmt, spkw_defw, nd, value.rValue);
    askOpt(OPT_DELMIN, &value, &notset);
    if (!notset)
        TTY.printf(dfmt, spkw_delmin, nd, value.rValue);
    askOpt(OPT_DPHIMAX, &value, &notset);
    if (!notset)
        TTY.printf(dfmt, spkw_dphimax, nd, value.rValue);
    askOpt(OPT_GMAX, &value, &notset);
    if (!notset)
        TTY.printf(dfmt, spkw_gmax, nd, value.rValue);
    askOpt(OPT_GMIN, &value, &notset);
    if (!notset)
        TTY.printf(dfmt, spkw_gmin, nd, value.rValue);
    askOpt(OPT_MAXDATA, &value, &notset);
    if (!notset)
        TTY.printf(dfmt, spkw_maxdata, value.rValue);
    askOpt(OPT_MINBREAK, &value, &notset);
    if (!notset)
        TTY.printf(dfmt, spkw_minbreak, nd, value.rValue);
    askOpt(OPT_PIVREL, &value, &notset);
    if (!notset)
        TTY.printf(dfmt, spkw_pivrel, nd, value.rValue);
    askOpt(OPT_PIVTOL, &value, &notset);
    if (!notset)
        TTY.printf(dfmt, spkw_pivtol, nd, value.rValue);
    askOpt(OPT_TEMP, &value, &notset);
    if (!notset)
        TTY.printf(dfmt, spkw_temp, nd, value.rValue);
    askOpt(OPT_TNOM, &value, &notset);
    if (!notset)
        TTY.printf(dfmt, spkw_tnom, nd, value.rValue);
    askOpt(OPT_TRAPRATIO, &value, &notset);
    if (!notset)
        TTY.printf(dfmt, spkw_trapratio, nd, value.rValue);
    askOpt(OPT_TRTOL, &value, &notset);
    if (!notset)
        TTY.printf(dfmt, spkw_trtol, nd, value.rValue);
    askOpt(OPT_VNTOL, &value, &notset);
    if (!notset)
        TTY.printf(dfmt, spkw_vntol, nd, value.rValue);
    askOpt(OPT_XMU, &value, &notset);
    if (!notset)
        TTY.printf(dfmt, spkw_xmu, value.rValue);

    askOpt(OPT_BYPASS, &value, &notset);
    if (!notset)
        TTY.printf(ifmt, spkw_bypass, value.iValue);
    askOpt(OPT_FPEMODE, &value, &notset);
    if (!notset)
        TTY.printf(ifmt, spkw_fpemode, value.iValue);
    askOpt(OPT_FPEMODE, &value, &notset);
    if (!notset)
        TTY.printf(ifmt, spkw_fpemode, value.iValue);
    askOpt(OPT_GMINSTEPS, &value, &notset);
    if (!notset)
        TTY.printf(ifmt, spkw_gminsteps, value.iValue);
    askOpt(OPT_INTERPLEV, &value, &notset);
    if (!notset)
        TTY.printf(ifmt, spkw_interplev, value.iValue);
    askOpt(OPT_ITL1, &value, &notset);
    if (!notset)
        TTY.printf(ifmt, spkw_itl1, value.iValue);
    askOpt(OPT_ITL2, &value, &notset);
    if (!notset)
        TTY.printf(ifmt, spkw_itl2, value.iValue);
    askOpt(OPT_ITL2GMIN, &value, &notset);
    if (!notset)
        TTY.printf(ifmt, spkw_itl2gmin, value.iValue);
    askOpt(OPT_ITL2SRC, &value, &notset);
    if (!notset)
        TTY.printf(ifmt, spkw_itl2src, value.iValue);
    askOpt(OPT_ITL4, &value, &notset);
    if (!notset)
        TTY.printf(ifmt, spkw_itl4, value.iValue);
#ifdef WITH_THREADS
    askOpt(OPT_LOADTHRDS, &value, &notset);
    if (!notset)
        TTY.printf(ifmt, spkw_loadthrds, value.iValue);
    askOpt(OPT_LOOPTHRDS, &value, &notset);
    if (!notset)
        TTY.printf(ifmt, spkw_loopthrds, value.iValue);
#endif
    askOpt(OPT_MAXORD, &value, &notset);
    if (!notset)
        TTY.printf(ifmt, spkw_maxord, value.iValue);
    askOpt(OPT_SRCSTEPS, &value, &notset);
    if (!notset)
        TTY.printf(ifmt, spkw_srcsteps, value.iValue);

    askOpt(OPT_DCODDSTEP, &value, &notset);
    if (!notset)
        TTY.printf(ifmt, spkw_dcoddstep, value.iValue);
    askOpt(OPT_EXTPREC, &value, &notset);
    if (!notset)
        TTY.printf(ifmt, spkw_extprec, value.iValue);
    askOpt(OPT_FORCEGMIN, &value, &notset);
    if (!notset)
        TTY.printf(ifmt, spkw_forcegmin, value.iValue);
    askOpt(OPT_GMINFIRST, &value, &notset);
    if (!notset)
        TTY.printf(ifmt, spkw_gminfirst, value.iValue);
    askOpt(OPT_HSPICE, &value, &notset);
    if (!notset)
        TTY.printf(ifmt, spkw_hspice, value.iValue);
    askOpt(OPT_JJACCEL, &value, &notset);
    if (!notset)
        TTY.printf(ifmt, spkw_jjaccel, value.iValue);
    askOpt(OPT_NOITER, &value, &notset);
    if (!notset)
        TTY.printf(ifmt, spkw_noiter, value.iValue);
    askOpt(OPT_NOJJTP, &value, &notset);
    if (!notset)
        TTY.printf(ifmt, spkw_nojjtp, value.iValue);
    askOpt(OPT_NOKLU, &value, &notset);
    if (!notset)
        TTY.printf(ifmt, spkw_noklu, value.iValue);
    askOpt(OPT_NOMATSORT, &value, &notset);
    if (!notset)
        TTY.printf(ifmt, spkw_nomatsort, value.iValue);
    askOpt(OPT_NOOPITER, &value, &notset);
    if (!notset)
        TTY.printf(ifmt, spkw_noopiter, value.iValue);
    askOpt(OPT_NOSHELLOPTS, &value, &notset);
    if (!notset)
        TTY.printf(ifmt, spkw_noshellopts, value.iValue);
    askOpt(OPT_OLDLIMIT, &value, &notset);
    if (!notset)
        TTY.printf(ifmt, spkw_oldlimit, value.iValue);
    askOpt(OPT_OLDSTEPLIM, &value, &notset);
    if (!notset)
        TTY.printf(ifmt, spkw_oldsteplim, value.iValue);
    askOpt(OPT_RENUMBER, &value, &notset);
    if (!notset)
        TTY.printf(ifmt, spkw_renumber, value.iValue);
    askOpt(OPT_SAVECURRENT, &value, &notset);
    if (!notset)
        TTY.printf(ifmt, spkw_savecurrent, value.iValue);
    askOpt(OPT_SPICE3, &value, &notset);
    if (!notset)
        TTY.printf(ifmt, spkw_spice3, value.iValue);
    askOpt(OPT_TRAPCHECK, &value, &notset);
    if (!notset)
        TTY.printf(ifmt, spkw_trapcheck, value.iValue);
    askOpt(OPT_TRYTOCOMPACT, &value, &notset);
    if (!notset)
        TTY.printf(ifmt, spkw_trytocompact, value.iValue);
    askOpt(OPT_USEADJOINT, &value, &notset);
    if (!notset)
        TTY.printf(ifmt, spkw_useadjoint, value.iValue);

    askOpt(OPT_METHOD, &value, &notset);
    if (!notset)
        TTY.printf(sfmt, spkw_method, value.sValue);
    askOpt(OPT_OPTMERGE, &value, &notset);
    if (!notset)
        TTY.printf(sfmt, spkw_optmerge, value.sValue);
    askOpt(OPT_PARHIER, &value, &notset);
    if (!notset)
        TTY.printf(sfmt, spkw_parhier, value.sValue);
    askOpt(OPT_STEPTYPE, &value, &notset);
    if (!notset)
        TTY.printf(sfmt, spkw_steptype, value.sValue);
}


int
sOPTIONS::askOpt(int which, IFvalue *value, int *notset)
{
    *notset = 0;
    const sOPTIONS *opt = this;

    switch (which) {
    case OPT_ABSTOL:
        if (opt && OPTabstol_given)
            value->rValue = OPTabstol;
        else
            *notset = 1;
        break;
    case OPT_CHGTOL:
        if (opt && OPTchgtol_given)
            value->rValue = OPTchgtol;
        else
            *notset = 1;
        break;
    case OPT_DCMU:
        if (opt && OPTdcmu_given)
            value->rValue = OPTdcmu;
        else
            *notset = 1;
        break;
    case OPT_DEFAD:
        if (opt && OPTdefad_given)
            value->rValue = OPTdefad;
        else
            *notset = 1;
        break;
    case OPT_DEFAS:
        if (opt && OPTdefas_given)
            value->rValue = OPTdefas;
        else
            *notset = 1;
        break;
    case OPT_DEFL:
        if (opt && OPTdefl_given)
            value->rValue = OPTdefl;
        else
            *notset = 1;
        break;
    case OPT_DEFW:
        if (opt && OPTdefw_given)
            value->rValue = OPTdefw;
        else
            *notset = 1;
        break;
    case OPT_DELMIN:
        if (opt && OPTdelmin_given)
            value->rValue = OPTdelmin;
        else
            *notset = 1;
        break;
    case OPT_DPHIMAX:
        if (opt && OPTdphimax_given)
            value->rValue = OPTdphimax;
        else
            *notset = 1;
        break;
    case OPT_GMAX:
        if (opt && OPTgmax_given)
            value->rValue = OPTgmax;
        else
            *notset = 1;
        break;
    case OPT_GMIN:
        if (opt && OPTgmin_given)
            value->rValue = OPTgmin;
        else
            *notset = 1;
        break;
    case OPT_MAXDATA:
        if (opt && OPTmaxdata_given)
            value->rValue = OPTmaxdata;
        else
            *notset = 1;
        break;
    case OPT_MINBREAK:
        if (opt && OPTminbreak_given)
            value->rValue = OPTminbreak;
        else
            *notset = 1;
        break;
    case OPT_PIVREL:
        if (opt && OPTpivrel_given)
            value->rValue = OPTpivrel;
        else
            *notset = 1;
        break;
    case OPT_PIVTOL:
        if (opt && OPTpivtol_given)
            value->rValue = OPTpivtol;
        else
            *notset = 1;
        break;
    case OPT_RAMPUP:
        if (opt && OPTrampup_given)
            value->rValue = OPTrampup;
        else
            *notset = 1;
        break;
    case OPT_RELTOL:
        if (opt && OPTreltol_given)
            value->rValue = OPTreltol;
        else
            *notset = 1;
        break;
    case OPT_TEMP:
        if (opt && OPTtemp_given)
            value->rValue = OPTtemp - wrsCONSTCtoK;
        else
            *notset = 1;
        break;
    case OPT_TNOM:
        if (opt && OPTtnom_given)
            value->rValue = OPTtnom - wrsCONSTCtoK;
        else
            *notset = 1;
        break;
    case OPT_TRAPRATIO:
        if (opt && OPTtrapratio_given)
            value->rValue = OPTtrapratio;
        else
            *notset = 1;
        break;
    case OPT_TRTOL:
        if (opt && OPTtrtol_given)
            value->rValue = OPTtrtol;
        else
            *notset = 1;
        break;
    case OPT_VNTOL:
        if (opt && OPTvntol_given)
            value->rValue = OPTvntol;
        else
            *notset = 1;
        break;
    case OPT_XMU:
        if (opt && OPTxmu_given)
            value->rValue = OPTxmu;
        else
            *notset = 1;
        break;

    case OPT_BYPASS:
        if (opt && OPTbypass_given)
            value->iValue = OPTbypass;
        else
            *notset = 1;
        break;
    case OPT_FPEMODE:
        if (opt && OPTfpemode_given)
            value->iValue = OPTfpemode;
        else
            *notset = 1;
        break;
    case OPT_GMINSTEPS:
        if (opt && OPTgminsteps_given)
            value->iValue = OPTgminsteps;
        else
            *notset = 1;
        break;
    case OPT_INTERPLEV:
        if (opt && OPTinterplev_given)
            value->iValue = OPTinterplev;
        else
            *notset = 1;
        break;
    case OPT_ITL1:
        if (opt && OPTitl1_given)
            value->iValue = OPTitl1;
        else
            *notset = 1;
        break;
    case OPT_ITL2:
        if (opt && OPTitl2_given)
            value->iValue = OPTitl2;
        else
            *notset = 1;
        break;
    case OPT_ITL2GMIN:
        if (opt && OPTitl2gmin_given)
            value->iValue = OPTitl2gmin;
        else
            *notset = 1;
        break;
    case OPT_ITL2SRC:
        if (opt && OPTitl2src_given)
            value->iValue = OPTitl2src;
        else
            *notset = 1;
        break;
    case OPT_ITL4:
        if (opt && OPTitl4_given)
            value->iValue = OPTitl4;
        else
            *notset = 1;
        break;
#ifdef WITH_THREADS
    case OPT_LOADTHRDS:
        if (opt && OPTloadthrds_given)
            value->iValue = OPTloadthrds;
        else
            *notset = 1;
        break;
    case OPT_LOOPTHRDS:
        if (opt && OPTloopthrds_given)
            value->iValue = OPTloopthrds;
        else
            *notset = 1;
        break;
#endif
    case OPT_MAXORD:
        if (opt && OPTmaxord_given)
            value->iValue = OPTmaxord;
        else
            *notset = 1;
        break;
    case OPT_SRCSTEPS:
        if (opt && OPTsrcsteps_given)
            value->iValue = OPTsrcsteps;
        else
            *notset = 1;
        break;

    case OPT_DCODDSTEP:
        if (opt && OPTdcoddstep_given)
            value->iValue = OPTdcoddstep;
        else
            *notset = 1;
        break;
    case OPT_EXTPREC:
        if (opt && OPTextprec_given)
            value->iValue = OPTextprec;
        else
            *notset = 1;
        break;
    case OPT_FORCEGMIN:
        if (opt && OPTforcegmin_given)
            value->iValue = OPTforcegmin;
        else
            *notset = 1;
        break;
    case OPT_GMINFIRST:
        if (opt && OPTgminfirst_given)
            value->iValue = OPTgminfirst;
        else
            *notset = 1;
        break;
    case OPT_HSPICE:
        if (opt && OPThspice_given)
            value->iValue = OPThspice;
        else
            *notset = 1;
        break;
    case OPT_JJACCEL:
        if (opt && OPTjjaccel_given)
            value->iValue = OPTjjaccel;
        else
            *notset = 1;
        break;
    case OPT_NOITER:
        if (opt && OPTnoiter_given)
            value->iValue = OPTnoiter;
        else
            *notset = 1;
        break;
    case OPT_NOJJTP:
        if (opt && OPTnojjtp_given)
            value->iValue = OPTnojjtp;
        else
            *notset = 1;
        break;
    case OPT_NOKLU:
        if (opt && OPTnoklu_given)
            value->iValue = OPTnoklu;
        else
            *notset = 1;
        break;
    case OPT_NOMATSORT:
        if (opt && OPTnomatsort_given)
            value->iValue = OPTnomatsort;
        else
            *notset = 1;
        break;
    case OPT_NOOPITER:
        if (opt && OPTnoopiter_given)
            value->iValue = OPTnoopiter;
        else
            *notset = 1;
        break;
    case OPT_NOSHELLOPTS:
        if (opt && OPTnoshellopts_given)
            value->iValue = OPTnoshellopts;
        else
            *notset = 1;
        break;
    case OPT_OLDLIMIT:
        if (opt && OPToldlimit_given)
            value->iValue = OPToldlimit;
        else
            *notset = 1;
        break;
    case OPT_OLDSTEPLIM:
        if (opt && OPToldsteplim_given)
            value->iValue = OPToldsteplim;
        else
            *notset = 1;
        break;
    case OPT_RENUMBER:
        if (opt && OPTrenumber_given)
            value->iValue = OPTrenumber;
        else
            *notset = 1;
        break;
    case OPT_SAVECURRENT:
        if (opt && OPTsavecurrent_given)
            value->iValue = OPTsavecurrent;
        else
            *notset = 1;
        break;
    case OPT_SPICE3:
        if (opt && OPTspice3_given)
            value->iValue = OPTspice3;
        else
            *notset = 1;
        break;
    case OPT_TRAPCHECK:
        if (opt && OPTtrapcheck_given)
            value->iValue = OPTtrapcheck;
        else
            *notset = 1;
        break;
    case OPT_TRYTOCOMPACT:
        if (opt && OPTtrytocompact_given)
            value->iValue = OPTtrytocompact;
        else
            *notset = 1;
        break;
    case OPT_USEADJOINT:
        if (opt && OPTuseadjoint_given)
            value->iValue = OPTuseadjoint;
        else
            *notset = 1;
        break;

    case OPT_METHOD:
        if (opt && OPTmaxord_given) {
            if (OPTmethod == TRAPEZOIDAL)
                value->sValue = spkw_trap;
            else if (OPTmethod == GEAR)
                value->sValue = spkw_gear;
            else
                value->sValue = "unspecified";
        }
        else
            *notset = 1;
        break;
    case OPT_OPTMERGE:
        if (opt && OPToptmerge_given) {
            if (OPToptmerge == OMRG_GLOBAL)
                value->sValue = spkw_global;
            else if (OPToptmerge == OMRG_LOCAL)
                value->sValue = spkw_local;
            else if (OPToptmerge == OMRG_NOSHELL)
                value->sValue = spkw_noshell;
            else
                value->sValue = "unspecified";
        }
        else
            *notset = 1;
        break;
    case OPT_PARHIER:
        if (opt && OPTparhier_given) {
            if (OPTparhier == 0)
                value->sValue = spkw_global;
            else if (OPToptmerge == 1)
                value->sValue = spkw_local;
            else
                value->sValue = "unspecified";
        }
        else
            *notset = 1;
        break;
    case OPT_STEPTYPE:
        if (opt && OPTsteptype_given) {
            if (OPTsteptype == STEP_NORMAL)
                value->sValue = spkw_interpolate;
            else if (OPTsteptype == STEP_HITUSERTP)
                value->sValue = spkw_hitusertp;
            else if (OPTsteptype == STEP_NOUSERTP)
                value->sValue = spkw_nousertp;
            else if (OPTsteptype == STEP_FIXEDSTEP)
                value->sValue = spkw_fixedstep;
            else
                value->sValue = "unspecified";
        }
        else
            *notset = 1;
        break;

    default:
        return (E_BADPARM);
    }

    return (OK);
}


// Create a variable list from the sOPTIONS struct.
//
variable *
sOPTIONS::tovar()
{
    variable *v0=0, *v=0;
    IFanalysis *an = &OPTinfo;
    for (int i = 0; i < an->numParms; i++) {
        if (!(an->analysisParms[i].dataType & IF_SET))
            continue;
        IFvalue parm;
        int notset;
        if (askOpt(an->analysisParms[i].id, &parm, &notset) != OK)
            continue;
        if (notset) {
            if (v) {
                v->set_next(new variable(an->analysisParms[i].keyword));
                v = v->next();
            }
            else
                v = v0 = new variable(an->analysisParms[i].keyword);
            v->set_boolean(false);
        }
        else {
            if (v) {
                v->set_next(an->analysisParms[i].tovar(&parm));
                v = v->next();
            }
            else
                v = v0 = an->analysisParms[i].tovar(&parm);
        }
    }
    return (v0);
}
// End of sOPTIONS functions.


int
OPTanalysis::askQuest(const sCKT *ckt, const sJOB*, int which,
    IFdata *data) const
{
    if (!ckt)
        return (E_NOCKT);
    IFvalue *value = &data->v;
    const sTASK *task = ckt->CKTcurTask;

    switch (which) {
    case OPT_ABSTOL:
        value->rValue = task->TSKabstol;
        data->type = IF_REAL;
        break;
    case OPT_CHGTOL:
        value->rValue = task->TSKchgtol;
        data->type = IF_REAL;
        break;
    case OPT_DCMU:
        value->rValue = task->TSKdcMu;
        data->type = IF_REAL;
        break;
    case OPT_DEFAD:
        value->rValue = task->TSKdefaultMosAD;
        data->type = IF_REAL;
        break;
    case OPT_DEFAS:
        value->rValue = task->TSKdefaultMosAD;
        data->type = IF_REAL;
        break;
    case OPT_DEFL:
        value->rValue = task->TSKdefaultMosL;
        data->type = IF_REAL;
        break;
    case OPT_DEFW:
        value->rValue = task->TSKdefaultMosW;
        data->type = IF_REAL;
        break;
    case OPT_DPHIMAX:
        value->rValue = task->TSKdphiMax;
        data->type = IF_REAL;
        break;
    case OPT_GMAX:
        value->rValue = task->TSKgmax;
        data->type = IF_REAL;
        break;
    case OPT_GMIN:
        value->rValue = task->TSKgmin;
        data->type = IF_REAL;
        break;
    case OPT_MAXDATA:
        value->rValue = task->TSKmaxData;
        data->type = IF_REAL;
        break;
    case OPT_MINBREAK:
        value->rValue = task->TSKminBreak;
        data->type = IF_REAL;
        break;
    case OPT_PIVREL:
        value->rValue = task->TSKpivotRelTol;
        data->type = IF_REAL;
        break;
    case OPT_PIVTOL:
        value->rValue = task->TSKpivotAbsTol;
        data->type = IF_REAL;
        break;
    case OPT_RAMPUP:
        value->rValue = task->TSKrampUpTime;
        data->type = IF_REAL;
        break;
    case OPT_RELTOL:
        value->rValue = task->TSKreltol;
        data->type = IF_REAL;
        break;
    case OPT_TEMP:
        value->rValue = task->TSKtemp - wrsCONSTCtoK;
        data->type = IF_REAL;
        break;
    case OPT_TNOM:
        value->rValue = task->TSKnomTemp - wrsCONSTCtoK;
        data->type = IF_REAL;
        break;
    case OPT_TRAPRATIO:
        value->rValue = task->TSKtrapRatio;
        data->type = IF_REAL;
        break;
    case OPT_TRTOL:
        value->rValue = task->TSKtrtol;
        data->type = IF_REAL;
        break;
    case OPT_VNTOL:
        value->rValue = task->TSKvoltTol;
        data->type = IF_REAL;
        break;
    case OPT_XMU:
        value->rValue = task->TSKxmu;
        data->type = IF_REAL;
        break;

    case OPT_BYPASS:
        value->iValue = task->TSKbypass;
        data->type = IF_INTEGER;
        break;
    case OPT_FPEMODE:
        value->iValue = task->TSKfpeMode;
        data->type = IF_INTEGER;
        break;
    case OPT_GMINSTEPS:
        value->iValue = task->TSKnumGminSteps;
        data->type = IF_INTEGER;
        break;
    case OPT_INTERPLEV:
        value->iValue = task->TSKinterpLev;
        data->type = IF_INTEGER;
        break;
    case OPT_ITL1:
        value->iValue = task->TSKdcMaxIter;
        data->type = IF_INTEGER;
        break;
    case OPT_ITL2:
        value->iValue = task->TSKdcTrcvMaxIter;
        data->type = IF_INTEGER;
        break;
    case OPT_ITL4:
        value->iValue = task->TSKtranMaxIter;
        data->type = IF_INTEGER;
        break;
#ifdef WITH_THREADS
    case OPT_LOADTHRDS:
        value->iValue = task->TSKloadThreads;
        data->type = IF_INTEGER;
        break;
    case OPT_LOOPTHRDS:
        value->iValue = task->TSKloopThreads;
        data->type = IF_INTEGER;
        break;
#endif
    case OPT_MAXORD:
        value->iValue = task->TSKmaxOrder;
        data->type = IF_INTEGER;
        break;
    case OPT_SRCSTEPS:
        value->iValue = task->TSKnumSrcSteps;
        data->type = IF_INTEGER;
        break;

    case OPT_DCODDSTEP:
        value->iValue = task->TSKdcOddStep;
        data->type = IF_FLAG;
        break;
    case OPT_EXTPREC:
        value->iValue = task->TSKextPrec;
        data->type = IF_FLAG;
        break;
    case OPT_FORCEGMIN:
        value->iValue = task->TSKforceGmin;
        data->type = IF_FLAG;
        break;
    case OPT_GMINFIRST:
        value->iValue = task->TSKgminFirst;
        data->type = IF_FLAG;
        break;
    case OPT_HSPICE:
        value->iValue = task->TSKhspice;
        data->type = IF_FLAG;
        break;
    case OPT_JJACCEL:
        value->iValue = task->TSKjjaccel;
        data->type = IF_FLAG;
        break;
    case OPT_NOITER:
        value->iValue = task->TSKnoiter;
        data->type = IF_FLAG;
        break;
    case OPT_NOJJTP:
        value->iValue = task->TSKnojjtp;
        data->type = IF_FLAG;
        break;
    case OPT_NOKLU:
        value->iValue = task->TSKnoKLU;
        data->type = IF_FLAG;
        break;
    case OPT_NOMATSORT:
        value->iValue = task->TSKnoMatrixSort;
        data->type = IF_FLAG;
        break;
    case OPT_NOOPITER:
        value->iValue = task->TSKnoOpIter;
        data->type = IF_FLAG;
        break;
    case OPT_NOSHELLOPTS:
        value->iValue = task->TSKnoShellOpts;
        data->type = IF_FLAG;
        break;
    case OPT_OLDLIMIT:
        value->iValue = task->TSKfixLimit;
        data->type = IF_FLAG;
        break;
    case OPT_OLDSTEPLIM:
        value->iValue = task->TSKoldStepLim;
        data->type = IF_FLAG;
        break;
    case OPT_RENUMBER:
        value->iValue = task->TSKrenumber;
        data->type = IF_FLAG;
        break;
    case OPT_SAVECURRENT:
        value->iValue = task->TSKsaveCurrent;
        data->type = IF_FLAG;
        break;
    case OPT_SPICE3:
        value->iValue = task->TSKspice3;
        data->type = IF_FLAG;
        break;
    case OPT_TRAPCHECK:
        value->iValue = task->TSKtrapCheck;
        data->type = IF_FLAG;
        break;
    case OPT_TRYTOCOMPACT:
        value->iValue = task->TSKtryToCompact;
        data->type = IF_FLAG;
        break;
    case OPT_USEADJOINT:
        value->iValue = task->TSKuseAdjoint;
        data->type = IF_FLAG;
        break;

    case OPT_METHOD:
        if (task->TSKintegrateMethod == TRAPEZOIDAL)
            value->sValue = spkw_trap;
        else if (task->TSKintegrateMethod == GEAR)
            value->sValue = spkw_gear;
        else
            value->sValue = "unspecified";
        data->type = IF_STRING;
        break;
    case OPT_OPTMERGE:
        if (task->TSKoptMerge == OMRG_GLOBAL)
            value->sValue = spkw_global;
        else if (task->TSKoptMerge == OMRG_LOCAL)
            value->sValue = spkw_local;
        else if (task->TSKoptMerge == OMRG_NOSHELL)
            value->sValue = spkw_noshell;
        else
            value->sValue = "unspecified";
        data->type = IF_STRING;
        break;
    case OPT_PARHIER:
        if (task->TSKparHier == 0)
            value->sValue = spkw_global;
        else if (task->TSKparHier == 1)
            value->sValue = spkw_local;
        else
            value->sValue = "unspecified";
        data->type = IF_STRING;
        break;
    case OPT_STEPTYPE:
        if (task->TSKtranStepType == STEP_NORMAL)
            value->sValue = spkw_interpolate;
        else if (task->TSKtranStepType == STEP_HITUSERTP)
            value->sValue = spkw_hitusertp;
        else if (task->TSKtranStepType == STEP_NOUSERTP)
            value->sValue = spkw_nousertp;
        else if (task->TSKtranStepType == STEP_FIXEDSTEP)
            value->sValue = spkw_fixedstep;
        else
            value->sValue = "unspecified";
        data->type = IF_STRING;
        break;

    case OPT_DELTA:
        value->rValue = ckt->CKTdeltaOld[0];
        data->type = IF_REAL;
        break;

    case OPT_EXTERNAL:
        break;
    case OPT_NOTUSED:
        break;

    default:
        return (E_BADPARM);
    }
    return (OK);
}


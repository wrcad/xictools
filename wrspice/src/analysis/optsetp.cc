
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
JSPICE3 adaptation of Spice3f2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 Thomas L. Quarles
         1992 Stephen R. Whiteley
****************************************************************************/

#include "optdefs.h"
#include "errors.h"
#include "output.h"
#include "misc.h"
#include "kwords_analysis.h"


// Spice option keywords
// reals
const char *spkw_abstol         = "abstol";
const char *spkw_chgtol         = "chgtol";
const char *spkw_dcmu           = "dcmu";
const char *spkw_defad          = "defad";
const char *spkw_defas          = "defas";
const char *spkw_defl           = "defl";
const char *spkw_defw           = "defw";
const char *spkw_delmin         = "delmin";
const char *spkw_dphimax        = "dphimax";
const char *spkw_jjdphimax      = "jjdphimax";  // alias
const char *spkw_gmax           = "gmax";
const char *spkw_gmin           = "gmin";
const char *spkw_maxdata        = "maxdata";
const char *spkw_minbreak       = "minbreak";
const char *spkw_pivrel         = "pivrel";
const char *spkw_pivtol         = "pivtol";
const char *spkw_rampup         = "rampup";
const char *spkw_reltol         = "reltol";
const char *spkw_temp           = "temp";
const char *spkw_tnom           = "tnom";
const char *spkw_trapratio      = "trapratio";
const char *spkw_trtol          = "trtol";
const char *spkw_vntol          = "vntol";
const char *spkw_xmu            = "xmu";

// integers
const char *spkw_bypass         = "bypass";
const char *spkw_fpemode        = "fpemode";
const char *spkw_gminsteps      = "gminsteps";
const char *spkw_interplev      = "interplev";
const char *spkw_itl1           = "itl1";
const char *spkw_itl2           = "itl2";
const char *spkw_itl2gmin       = "itl2gmin";
const char *spkw_itl2src        = "itl2src";
const char *spkw_itl4           = "itl4";
#ifdef WITH_THREADS
const char *spkw_loadthrds      = "loadthrds";
const char *spkw_loopthrds      = "loopthrds";
#endif
const char *spkw_maxord         = "maxord";
const char *spkw_srcsteps       = "srcsteps";
const char *spkw_itl6           = "itl6";  // alias
const char *spkw_vastep         = "vastep";

// bools
const char *spkw_dcoddstep      = "dcoddstep";
const char *spkw_extprec        = "extprec";
const char *spkw_forcegmin      = "forcegmin";
const char *spkw_gminfirst      = "gminfirst";
const char *spkw_hspice         = "hspice";
const char *spkw_jjaccel        = "jjaccel";
const char *spkw_noiter         = "noiter";
const char *spkw_nojjtp         = "nojjtp";
const char *spkw_noklu          = "noklu";
const char *spkw_nomatsort      = "nomatsort";
const char *spkw_noopiter       = "noopiter";
const char *spkw_nopmdc         = "nopmdc";
const char *spkw_noshellopts    = "noshellopts";
const char *spkw_oldlimit       = "oldlimit";
const char *spkw_oldsteplim     = "oldsteplim";
const char *spkw_renumber       = "renumber";
const char *spkw_savecurrent    = "savecurrent";
const char *spkw_spice3         = "spice3";
const char *spkw_trapcheck      = "trapcheck";
const char *spkw_trytocompact   = "trytocompact";
const char *spkw_useadjoint     = "useadjoint";
const char *spkw_translate      = "translate";

// strings
const char *spkw_method         = "method";
const char *spkw_optmerge       = "optmerge";
const char *spkw_parhier        = "parhier";
const char *spkw_steptype       = "steptype";

// "external" options
const char *spkw_acct           = "acct";
const char *spkw_dev            = "dev";
const char *spkw_list           = "list";
const char *spkw_mod            = "mod";
const char *spkw_node           = "node";
const char *spkw_nopage         = "nopage";
const char *spkw_numdgt         = "numdgt";
const char *spkw_opts           = "opts";
const char *spkw_post           = "post";

// obsolete and/or unsupported
const char *spkw_cptime         = "cptime";
const char *spkw_itl3           = "itl3";
const char *spkw_itl5           = "itl5";
const char *spkw_limpts         = "limpts";
const char *spkw_limtim         = "limtim";
const char *spkw_lvlcod         = "lvlcod";
const char *spkw_lvltim         = "lvltim";
const char *spkw_nomod          = "nomod";

// string option values
const char *spkw_trap           = "trap";
const char *spkw_gear           = "gear";
const char *spkw_global         = "global";
const char *spkw_local          = "local";
const char *spkw_noshell        = "noshell";
const char *spkw_interpolate    = "interpolate";
const char *spkw_hitusertp      = "hitusertp";
const char *spkw_nousertp       = "nousertp";
const char *spkw_fixedstep      = "fixedstep";
const char *spkw_delta          = "delta";
const char *spkw_maxdelta       = "maxdelta";
const char *spkw_tstep          = "tstep";
const char *spkw_tstop          = "tstop";
const char *spkw_tstart         = "tstart";
const char *spkw_fstop          = "fstop";
const char *spkw_fstart         = "fstart";

// This tracks options set by the front end.
sOPTIONS sOPTIONS::OPTsh_opts;


sOPTIONS *
sOPTIONS::copy()
{
    const sOPTIONS *opt = this;
    if (!opt)
        return (0);
    sOPTIONS *n = new sOPTIONS(*this);
    return (n);
}


void
sOPTIONS::setup(const sOPTIONS *opts, OMRG_TYPE mt)
{
    if (!opts)
        return;

    // reals
    if (opts->OPTabstol_given && (mt == OMRG_GLOBAL || !OPTabstol_given)) {
        OPTabstol = opts->OPTabstol;
        OPTabstol_given = 1;
    }
    if (opts->OPTchgtol_given && (mt == OMRG_GLOBAL || !OPTchgtol_given)) {
        OPTchgtol = opts->OPTchgtol;
        OPTchgtol_given = 1;
    }
    if (opts->OPTdcmu_given && (mt == OMRG_GLOBAL || !OPTdcmu_given)) {
        OPTdcmu = opts->OPTdcmu;
        OPTdcmu_given = 1;
    }
    if (opts->OPTdefad_given && (mt == OMRG_GLOBAL || !OPTdefad_given)) {
        OPTdefad = opts->OPTdefad;
        OPTdefad_given = 1;
    }
    if (opts->OPTdefas_given && (mt == OMRG_GLOBAL || !OPTdefas_given)) {
        OPTdefas = opts->OPTdefas;
        OPTdefas_given = 1;
    }
    if (opts->OPTdefl_given && (mt == OMRG_GLOBAL || !OPTdefl_given)) {
        OPTdefl = opts->OPTdefl;
        OPTdefl_given = 1;
    }
    if (opts->OPTdefw_given && (mt == OMRG_GLOBAL || !OPTdefw_given)) {
        OPTdefw = opts->OPTdefw;
        OPTdefw_given = 1;
    }
    if (opts->OPTdelmin_given && (mt == OMRG_GLOBAL || !OPTdelmin_given)) {
        OPTdelmin = opts->OPTdelmin;
        OPTdelmin_given = 1;
    }
    if (opts->OPTdphimax_given && (mt == OMRG_GLOBAL || !OPTdphimax_given)) {
        OPTdphimax = opts->OPTdphimax;
        OPTdphimax_given = 1;
    }
    if (opts->OPTgmax_given && (mt == OMRG_GLOBAL || !OPTgmax_given)) {
        OPTgmax = opts->OPTgmax;
        OPTgmax_given = 1;
    }
    if (opts->OPTgmin_given && (mt == OMRG_GLOBAL || !OPTgmin_given)) {
        OPTgmin = opts->OPTgmin;
        OPTgmin_given = 1;
    }
    if (opts->OPTmaxdata_given && (mt == OMRG_GLOBAL || !OPTmaxdata_given)) {
        OPTmaxdata = opts->OPTmaxdata;
        OPTmaxdata_given = 1;
    }
    if (opts->OPTminbreak_given && (mt == OMRG_GLOBAL || !OPTminbreak_given)) {
        OPTminbreak = opts->OPTminbreak;
        OPTminbreak_given = 1;
    }
    if (opts->OPTpivrel_given && (mt == OMRG_GLOBAL || !OPTpivrel_given)) {
        OPTpivrel = opts->OPTpivrel;
        OPTpivrel_given = 1;
    }
    if (opts->OPTpivtol_given && (mt == OMRG_GLOBAL || !OPTpivtol_given)) {
        OPTpivtol = opts->OPTpivtol;
        OPTpivtol_given = 1;
    }
    if (opts->OPTrampup_given && (mt == OMRG_GLOBAL || !OPTrampup_given)) {
        OPTrampup = opts->OPTrampup;
        OPTrampup_given = 1;
    }
    if (opts->OPTreltol_given && (mt == OMRG_GLOBAL || !OPTreltol_given)) {
        OPTreltol = opts->OPTreltol;
        OPTreltol_given = 1;
    }
    if (opts->OPTtemp_given && (mt == OMRG_GLOBAL || !OPTtemp_given)) {
        OPTtemp = opts->OPTtemp;
        OPTtemp_given = 1;
    }
    if (opts->OPTtnom_given && (mt == OMRG_GLOBAL || !OPTtnom_given)) {
        OPTtnom = opts->OPTtnom;
        OPTtnom_given = 1;
    }
    if (opts->OPTtrapratio_given && (mt == OMRG_GLOBAL ||
            !OPTtrapratio_given)) {
        OPTtrapratio = opts->OPTtrapratio;
        OPTtrapratio_given = 1;
    }
    if (opts->OPTtrtol_given && (mt == OMRG_GLOBAL || !OPTtrtol_given)) {
        OPTtrtol = opts->OPTtrtol;
        OPTtrtol_given = 1;
    }
    if (opts->OPTvntol_given && (mt == OMRG_GLOBAL || !OPTvntol_given)) {
        OPTvntol = opts->OPTvntol;
        OPTvntol_given = 1;
    }
    if (opts->OPTxmu_given && (mt == OMRG_GLOBAL || !OPTxmu_given)) {
        OPTxmu = opts->OPTxmu;
        OPTxmu_given = 1;
    }

    // ints
    if (opts->OPTbypass_given && (mt == OMRG_GLOBAL || !OPTbypass_given)) {
        OPTbypass = opts->OPTbypass;
        OPTbypass_given = 1;
    }
    if (opts->OPTfpemode_given && (mt == OMRG_GLOBAL || !OPTfpemode_given)) {
        OPTfpemode = opts->OPTfpemode;
        OPTfpemode_given = 1;
    }
    if (opts->OPTgminsteps_given && (mt == OMRG_GLOBAL ||
            !OPTgminsteps_given)) {
        OPTgminsteps = opts->OPTgminsteps;
        OPTgminsteps_given = 1;
    }
    if (opts->OPTinterplev_given && (mt == OMRG_GLOBAL ||
            !OPTinterplev_given)) {
        OPTinterplev = opts->OPTinterplev;
        OPTinterplev_given = 1;
    }
    if (opts->OPTitl1_given && (mt == OMRG_GLOBAL || !OPTitl1_given)) {
        OPTitl1 = opts->OPTitl1;
        OPTitl1_given = 1;
    }
    if (opts->OPTitl2_given && (mt == OMRG_GLOBAL || !OPTitl2_given)) {
        OPTitl2 = opts->OPTitl2;
        OPTitl2_given = 1;
    }
    if (opts->OPTitl2gmin_given && (mt == OMRG_GLOBAL || !OPTitl2gmin_given)) {
        OPTitl2gmin = opts->OPTitl2gmin;
        OPTitl2gmin_given = 1;
    }
    if (opts->OPTitl2src_given && (mt == OMRG_GLOBAL || !OPTitl2src_given)) {
        OPTitl2src = opts->OPTitl2src;
        OPTitl2src_given = 1;
    }
    if (opts->OPTitl4_given && (mt == OMRG_GLOBAL || !OPTitl4_given)) {
        OPTitl4 = opts->OPTitl4;
        OPTitl4_given = 1;
    }
#ifdef WITH_THREADS
    if (opts->OPTloadthrds_given && (mt == OMRG_GLOBAL || !OPTloadthrds_given)) {
        OPTloadthrds = opts->OPTloadthrds;
        OPTloadthrds_given = 1;
    }
    if (opts->OPTloopthrds_given && (mt == OMRG_GLOBAL || !OPTloopthrds_given)) {
        OPTloopthrds = opts->OPTloopthrds;
        OPTloopthrds_given = 1;
    }
#endif
    if (opts->OPTmaxord_given && (mt == OMRG_GLOBAL || !OPTmaxord_given)) {
        OPTmaxord = opts->OPTmaxord;
        OPTmaxord_given = 1;
    }
    if (opts->OPTsrcsteps_given && (mt == OMRG_GLOBAL || !OPTsrcsteps_given)) {
        OPTsrcsteps = opts->OPTsrcsteps;
        OPTsrcsteps_given = 1;
    }
    if (opts->OPTvastep_given && (mt == OMRG_GLOBAL || !OPTvastep_given)) {
        OPTvastep = opts->OPTvastep;
        OPTvastep_given = 1;
    }

    // bools
    if (opts->OPTdcoddstep_given && (mt == OMRG_GLOBAL ||
            !OPTdcoddstep_given)) {
        OPTdcoddstep = opts->OPTdcoddstep;
        OPTdcoddstep_given = 1;
    }
    if (opts->OPTextprec_given && (mt == OMRG_GLOBAL || !OPTextprec_given)) {
        OPTextprec = opts->OPTextprec;
        OPTextprec_given = 1;
    }
    if (opts->OPTforcegmin_given && (mt == OMRG_GLOBAL ||
            !OPTforcegmin_given)) {
        OPTforcegmin = opts->OPTforcegmin;
        OPTforcegmin_given = 1;
    }
    if (opts->OPTgminfirst_given && (mt == OMRG_GLOBAL ||
            !OPTgminfirst_given)) {
        OPTgminfirst = opts->OPTgminfirst;
        OPTgminfirst_given = 1;
    }
    if (opts->OPThspice_given && (mt == OMRG_GLOBAL || !OPThspice_given)) {
        OPThspice = opts->OPThspice;
        OPThspice_given = 1;
    }
    if (opts->OPTjjaccel_given && (mt == OMRG_GLOBAL || !OPTjjaccel_given)) {
        OPTjjaccel = opts->OPTjjaccel;
        OPTjjaccel_given = 1;
    }
    if (opts->OPTnoiter_given && (mt == OMRG_GLOBAL || !OPTnoiter_given)) {
        OPTnoiter = opts->OPTnoiter;
        OPTnoiter_given = 1;
    }
    if (opts->OPTnojjtp_given && (mt == OMRG_GLOBAL || !OPTnojjtp_given)) {
        OPTnojjtp = opts->OPTnojjtp;
        OPTnojjtp_given = 1;
    }
    if (opts->OPTnoklu_given && (mt == OMRG_GLOBAL || !OPTnoklu_given)) {
        OPTnoklu = opts->OPTnoklu;
        OPTnoklu_given = 1;
    }
    if (opts->OPTnomatsort_given && (mt == OMRG_GLOBAL ||
            !OPTnomatsort_given)) {
        OPTnomatsort = opts->OPTnomatsort;
        OPTnomatsort_given = 1;
    }
    if (opts->OPTnoopiter_given && (mt == OMRG_GLOBAL || !OPTnoopiter_given)) {
        OPTnoopiter = opts->OPTnoopiter;
        OPTnoopiter_given = 1;
    }
    if (opts->OPTnopmdc_given && (mt == OMRG_GLOBAL || !OPTnopmdc_given)) {
        OPTnopmdc = opts->OPTnopmdc;
        OPTnopmdc_given = 1;
    }
    if (opts->OPTnoshellopts_given && (mt == OMRG_GLOBAL ||
            !OPTnoshellopts_given)) {
        OPTnoshellopts = opts->OPTnoshellopts;
        OPTnoshellopts_given = 1;
    }
    if (opts->OPToldlimit_given && (mt == OMRG_GLOBAL || !OPToldlimit_given)) {
        OPToldlimit = opts->OPToldlimit;
        OPToldlimit_given = 1;
    }
    if (opts->OPToldsteplim_given &&
            (mt == OMRG_GLOBAL || !OPToldsteplim_given)) {
        OPToldsteplim = opts->OPToldsteplim;
        OPToldsteplim_given = 1;
    }
    if (opts->OPTrenumber_given && (mt == OMRG_GLOBAL || !OPTrenumber_given)) {
        OPTrenumber = opts->OPTrenumber;
        OPTrenumber_given = 1;
    }
    if (opts->OPTsavecurrent_given && (mt == OMRG_GLOBAL ||
            !OPTsavecurrent_given)) {
        OPTsavecurrent = opts->OPTsavecurrent;
        OPTsavecurrent_given = 1;
    }
    if (opts->OPTspice3_given && (mt == OMRG_GLOBAL || !OPTspice3_given)) {
        OPTspice3 = opts->OPTspice3;
        OPTspice3_given = 1;
    }
    if (opts->OPTtrapcheck_given && (mt == OMRG_GLOBAL ||
            !OPTtrapcheck_given)) {
        OPTtrapcheck = opts->OPTtrapcheck;
        OPTtrapcheck_given = 1;
    }
    if (opts->OPTtrytocompact_given &&
            (mt == OMRG_GLOBAL || !OPTtrytocompact_given)) {
        OPTtrytocompact = opts->OPTtrytocompact;
        OPTtrytocompact_given = 1;
    }
    if (opts->OPTuseadjoint_given && (mt == OMRG_GLOBAL ||
            !OPTuseadjoint_given)) {
        OPTuseadjoint = opts->OPTuseadjoint;
        OPTuseadjoint_given = 1;
    }
    if (opts->OPTtranslate_given && (mt == OMRG_GLOBAL ||
            !OPTtranslate_given)) {
        OPTtranslate = opts->OPTtranslate;
        OPTtranslate_given = 1;
    }

    // strings
    if (opts->OPTmethod_given && (mt == OMRG_GLOBAL || !OPTmethod_given)) {
        OPTmethod = opts->OPTmethod;
        OPTmethod_given = 1;
    }
    if (opts->OPToptmerge_given && (mt == OMRG_GLOBAL || !OPToptmerge_given)) {
        OPToptmerge = opts->OPToptmerge;
        OPToptmerge_given = 1;
    }
    if (opts->OPTparhier_given && (mt == OMRG_GLOBAL || !OPTparhier_given)) {
        OPTparhier = opts->OPTparhier;
        OPTparhier_given = 1;
    }
    if (opts->OPTsteptype_given && (mt == OMRG_GLOBAL || !OPTsteptype_given)) {
        OPTsteptype = opts->OPTsteptype;
        OPTsteptype_given = 1;
    }
}
// End of sOPTIONS functions.


#define CHECKSET(a,b,c,d,e) \
{ \
    b = c; \
    if (b < d) { \
        OP.error(ERR_WARNING, "refusing to set %s less than minimum %g.\n", \
        a, (double)d); \
        b = d; \
    } \
    else if (b > e) { \
        OP.error(ERR_WARNING, "refusing to set %s larger than maximum %g.\n", \
        a, (double)e); \
        b = e; \
    } \
}


// Pass data=null to clear the value.
//
int
OPTanalysis::setParm(sJOB *anal, int which, IFdata *data)
{
    sOPTIONS *opt;
    if (anal)
        opt = (sOPTIONS *)anal;
    else
        opt = sOPTIONS::shellOpts();
    IFvalue *value = data ? &data->v : 0;

    switch (which) {
    case OPT_ABSTOL:
        if (value) {
            CHECKSET(spkw_abstol, opt->OPTabstol, value->rValue,
                DEF_abstol_MIN, DEF_abstol_MAX)
            opt->OPTabstol_given = 1;
        }
        else
            opt->OPTabstol_given = 0;
        break;
    case OPT_CHGTOL:
        if (value) {
            CHECKSET(spkw_chgtol, opt->OPTchgtol, value->rValue,
                DEF_chgtol_MIN, DEF_chgtol_MAX)
            opt->OPTchgtol_given = 1;
        }
        else
            opt->OPTchgtol_given = 0;
        break;
    case OPT_DCMU:
        if (value) {
            CHECKSET(spkw_dcmu, opt->OPTdcmu, value->rValue,
                DEF_dcMu_MIN, DEF_dcMu_MAX)
            opt->OPTdcmu_given = 1;
        }
        else
            opt->OPTdcmu_given = 0;
        break;
    case OPT_DEFAD:
        if (value) {
            CHECKSET(spkw_defad, opt->OPTdefad, value->rValue,
                DEF_defaultMosAD_MIN, DEF_defaultMosAD_MAX)
            opt->OPTdefad_given = 1;
        }
        else
            opt->OPTdefad_given = 0;
        break;
    case OPT_DEFAS:
        if (value) {
            CHECKSET(spkw_defas, opt->OPTdefas, value->rValue,
                DEF_defaultMosAS_MIN, DEF_defaultMosAS_MAX)
            opt->OPTdefas_given = 1;
        }
        else
            opt->OPTdefas_given = 0;
        break;
    case OPT_DEFL:
        if (value) {
            CHECKSET(spkw_defl, opt->OPTdefl, value->rValue,
                DEF_defaultMosL_MIN, DEF_defaultMosL_MAX)
            opt->OPTdefl_given = 1;
        }
        else
            opt->OPTdefl_given = 0;
        break;
    case OPT_DEFW:
        if (value) {
            CHECKSET(spkw_defw, opt->OPTdefw, value->rValue,
                DEF_defaultMosW_MIN, DEF_defaultMosW_MAX)
            opt->OPTdefw_given = 1;
        }
        else
            opt->OPTdefw_given = 0;
        break;
    case OPT_DELMIN:
        if (value) {
            CHECKSET(spkw_delmin, opt->OPTdelmin, value->rValue,
                DEF_delMin_MIN, DEF_delMin_MAX)
            opt->OPTdelmin_given = 1;
        }
        else
            opt->OPTdelmin_given = 0;
        break;
    case OPT_DPHIMAX:
        if (value) {
            CHECKSET(spkw_dphimax, opt->OPTdphimax, value->rValue,
                DEF_dphiMax_MIN, DEF_dphiMax_MAX)
            opt->OPTdphimax_given = 1;
        }
        else
            opt->OPTdphimax_given = 0;
        break;
    case OPT_GMAX:
        if (value) {
            CHECKSET(spkw_gmax, opt->OPTgmax, value->rValue,
                DEF_gmax_MIN, DEF_gmax_MAX)
            opt->OPTgmax_given = 1;
        }
        else
            opt->OPTgmax_given = 0;
        break;
    case OPT_GMIN:
        if (value) {
            CHECKSET(spkw_gmin, opt->OPTgmin, value->rValue,
                DEF_gmin_MIN, DEF_gmin_MAX)
            opt->OPTgmin_given = 1;
        }
        else
            opt->OPTgmin_given = 0;
        break;
    case OPT_MAXDATA:
        if (value) {
            CHECKSET(spkw_maxdata, opt->OPTmaxdata, value->rValue,
                DEF_maxData_MIN, DEF_maxData_MAX)
            opt->OPTmaxdata_given = 1;
        }
        else
            opt->OPTmaxdata_given = 0;
        break;
    case OPT_MINBREAK:
        if (value) {
            CHECKSET(spkw_minbreak, opt->OPTminbreak, value->rValue,
                DEF_minBreak_MIN, DEF_minBreak_MAX)
            opt->OPTminbreak_given = 1;
        }
        else
            opt->OPTminbreak_given = 0;
        break;
    case OPT_PIVREL:
        if (value) {
            CHECKSET(spkw_pivrel, opt->OPTpivrel, value->rValue,
                DEF_pivotRelTol_MIN, DEF_pivotRelTol_MAX)
            opt->OPTpivrel_given = 1;
        }
        else
            opt->OPTpivrel_given = 0;
        break;
    case OPT_PIVTOL:
        if (value) {
            CHECKSET(spkw_pivtol, opt->OPTpivtol, value->rValue,
                DEF_pivotAbsTol_MIN, DEF_pivotAbsTol_MAX)
            opt->OPTpivtol_given = 1;
        }
        else
            opt->OPTpivtol_given = 0;
        break;
    case OPT_RAMPUP:
        if (value) {
            CHECKSET(spkw_rampup, opt->OPTrampup, value->rValue,
                DEF_rampup_MIN, DEF_rampup_MAX)
            opt->OPTrampup_given = 1;
        }
        else
            opt->OPTrampup_given = 0;
        break;
    case OPT_RELTOL:
        if (value) {
            CHECKSET(spkw_reltol, opt->OPTreltol, value->rValue,
                DEF_reltol_MIN, DEF_reltol_MAX)
            opt->OPTreltol_given = 1;
        }
        else
            opt->OPTreltol_given = 0;
        break;
    case OPT_TEMP:
        if (value) {
            CHECKSET(spkw_temp, opt->OPTtemp, value->rValue + wrsCONSTCtoK,
                DEF_temp_MIN, DEF_temp_MAX)
            // Centegrade to Kelvin
            opt->OPTtemp_given = 1;
        }
        else
            opt->OPTtemp_given = 0;
        break;
    case OPT_TNOM:
        if (value) {
            CHECKSET(spkw_tnom, opt->OPTtnom, value->rValue + wrsCONSTCtoK,
                DEF_nomTemp_MIN, DEF_nomTemp_MAX)
            // Centegrade to Kelvin
            opt->OPTtnom_given = 1;
        }
        else
            opt->OPTtnom_given = 0;
        break;
    case OPT_TRAPRATIO:
        if (value) {
            CHECKSET(spkw_trapratio, opt->OPTtrapratio, value->rValue,
                DEF_trapRatio_MIN, DEF_trapRatio_MAX)
            opt->OPTtrapratio_given = 1;
        }
        else
            opt->OPTtrapratio_given = 0;
        break;
    case OPT_TRTOL:
        if (value) {
            CHECKSET(spkw_trtol, opt->OPTtrtol, value->rValue,
                DEF_trtol_MIN, DEF_trtol_MAX)
            opt->OPTtrtol_given = 1;
        }
        else
            opt->OPTtrtol_given = 0;
        break;
    case OPT_VNTOL:
        if (value) {
            CHECKSET(spkw_vntol, opt->OPTvntol, value->rValue,
                DEF_voltTol_MIN, DEF_voltTol_MAX)
            opt->OPTvntol_given = 1;
        }
        else
            opt->OPTvntol_given = 0;
        break;
    case OPT_XMU:
        if (value) {
            CHECKSET(spkw_xmu, opt->OPTxmu, value->rValue,
                DEF_xmu_MIN, DEF_xmu_MAX)
            opt->OPTxmu_given = 1;
        }
        else
            opt->OPTxmu_given = 0;
        break;

    case OPT_BYPASS:
        if (value) {
            CHECKSET(spkw_bypass, opt->OPTbypass, value->iValue,
                DEF_bypass_MIN, DEF_bypass_MAX)
            opt->OPTbypass_given = 1;
        }
        else
            opt->OPTbypass_given = 0;
        break;
    case OPT_FPEMODE:
        if (value) {
            CHECKSET(spkw_fpemode, opt->OPTfpemode, value->iValue,
                DEF_FPEmode_MIN, DEF_FPEmode_MAX)
            opt->OPTfpemode_given = 1;
        }
        else
            opt->OPTfpemode_given = 0;
        break;
    case OPT_GMINSTEPS:
        if (value) {
            CHECKSET(spkw_gminsteps, opt->OPTgminsteps, value->iValue,
                DEF_numGminSteps_MIN, DEF_numGminSteps_MAX)
            opt->OPTgminsteps_given = 1;
        }
        else
            opt->OPTgminsteps_given = 0;
        break;
    case OPT_INTERPLEV:
        if (value) {
            CHECKSET(spkw_interplev, opt->OPTinterplev, value->iValue,
                DEF_interplev_MIN, DEF_interplev_MAX)
            opt->OPTinterplev_given = 1;
        }
        else
            opt->OPTinterplev_given = 0;
        break;
    case OPT_ITL1:
        if (value) {
            CHECKSET(spkw_itl1, opt->OPTitl1, value->iValue,
                DEF_dcMaxIter_MIN, DEF_dcMaxIter_MAX)
            opt->OPTitl1_given = 1;
        }
        else
            opt->OPTitl1_given = 0;
        break;
    case OPT_ITL2:
        if (value) {
            CHECKSET(spkw_itl2, opt->OPTitl2, value->iValue,
                DEF_dcTrcvMaxIter_MIN, DEF_dcTrcvMaxIter_MAX)
            opt->OPTitl2_given = 1;
        }
        else
            opt->OPTitl2_given = 0;
        break;
    case OPT_ITL2GMIN:
        if (value) {
            CHECKSET(spkw_itl2gmin, opt->OPTitl2gmin, value->iValue,
                DEF_dcOpGminMaxIter_MIN, DEF_dcOpGminMaxIter_MAX)
            opt->OPTitl2gmin_given = 1;
        }
        else
            opt->OPTitl2gmin_given = 0;
        break;
    case OPT_ITL2SRC:
        if (value) {
            CHECKSET(spkw_itl2src, opt->OPTitl2src, value->iValue,
                DEF_dcOpSrcMaxIter_MIN, DEF_dcOpSrcMaxIter_MAX)
            opt->OPTitl2src_given = 1;
        }
        else
            opt->OPTitl2src_given = 0;
        break;
    case OPT_ITL4:
        if (value) {
            CHECKSET(spkw_itl4, opt->OPTitl4, value->iValue,
                DEF_tranMaxIter_MIN, DEF_tranMaxIter_MAX)
            opt->OPTitl4_given = 1;
        }
        else
            opt->OPTitl4_given = 0;
        break;
#ifdef WITH_THREADS
    case OPT_LOADTHRDS:
        if (value) {
            CHECKSET(spkw_loadthrds, opt->OPTloadthrds, value->iValue,
                DEF_loadThreads_MIN, DEF_loadThreads_MAX)
            opt->OPTloadthrds_given = 1;
        }
        else
            opt->OPTloadthrds_given = 0;
        break;
    case OPT_LOOPTHRDS:
        if (value) {
            CHECKSET(spkw_loopthrds, opt->OPTloopthrds, value->iValue,
                DEF_loopThreads_MIN, DEF_loopThreads_MAX)
            opt->OPTloopthrds_given = 1;
        }
        else
            opt->OPTloopthrds_given = 0;
        break;
#endif
    case OPT_MAXORD:
        if (value) {
            CHECKSET(spkw_maxord, opt->OPTmaxord, value->iValue,
                DEF_maxOrder_MIN, DEF_maxOrder_MAX)
            opt->OPTmaxord_given = 1;
        }
        else
            opt->OPTmaxord_given = 0;
        break;
    case OPT_SRCSTEPS:
        if (value) {
            CHECKSET(spkw_srcsteps, opt->OPTsrcsteps, value->iValue,
                DEF_numSrcSteps_MIN, DEF_numSrcSteps_MAX)
            opt->OPTsrcsteps_given = 1;
        }
        else
            opt->OPTsrcsteps_given = 0;
        break;
    case OPT_VASTEP:
        if (value) {
            CHECKSET(spkw_vastep, opt->OPTvastep, value->iValue,
                DEF_vastep_MIN, DEF_vastep_MAX)
            opt->OPTvastep_given = 1;
        }
        else
            opt->OPTvastep_given = 0;
        break;

    case OPT_DCODDSTEP:
        if (value) {
            opt->OPTdcoddstep = value->iValue;
            opt->OPTdcoddstep_given = 1;
        }
        else
            opt->OPTdcoddstep_given = 0;
        break;
    case OPT_EXTPREC:
        if (value) {
            opt->OPTextprec = value->iValue;
            opt->OPTextprec_given = 1;
        }
        else
            opt->OPTextprec_given = 0;
        break;
    case OPT_FORCEGMIN:
        if (value) {
            opt->OPTforcegmin = value->iValue;
            opt->OPTforcegmin_given = 1;
        }
        else
            opt->OPTforcegmin_given = 0;
        break;
    case OPT_GMINFIRST:
        if (value) {
            opt->OPTgminfirst = value->iValue;
            opt->OPTgminfirst_given = 1;
        }
        else
            opt->OPTgminfirst_given = 0;
        break;
    case OPT_HSPICE:
        if (value) {
            opt->OPThspice = value->iValue;
            opt->OPThspice_given = 1;
        }
        else
            opt->OPThspice_given = 0;
        break;
    case OPT_JJACCEL:
        if (value) {
            opt->OPTjjaccel = value->iValue;
            opt->OPTjjaccel_given = 1;
        }
        else
            opt->OPTjjaccel_given = 0;
        break;
    case OPT_NOITER:
        if (value) {
            opt->OPTnoiter = value->iValue;
            opt->OPTnoiter_given = 1;
        }
        else
            opt->OPTnoiter_given = 0;
        break;
    case OPT_NOJJTP:
        if (value) {
            opt->OPTnojjtp = value->iValue;
            opt->OPTnojjtp_given = 1;
        }
        else
            opt->OPTnojjtp_given = 0;
        break;
    case OPT_NOKLU:
        if (value) {
            opt->OPTnoklu = value->iValue;
            opt->OPTnoklu_given = 1;
        }
        else
            opt->OPTnoklu_given = 0;
        break;
    case OPT_NOMATSORT:
        if (value) {
            opt->OPTnomatsort = value->iValue;
            opt->OPTnomatsort_given = 1;
        }
        else
            opt->OPTnomatsort_given = 0;
        break;
    case OPT_NOOPITER:
        if (value) {
            opt->OPTnoopiter = value->iValue;
            opt->OPTnoopiter_given = 1;
        }
        else
            opt->OPTnoopiter_given = 0;
        break;
    case OPT_NOPMDC:
        if (value) {
            opt->OPTnopmdc = value->iValue;
            opt->OPTnopmdc_given = 1;
        }
        else
            opt->OPTnopmdc_given = 0;
        break;
    case OPT_NOSHELLOPTS:
        if (value) {
            opt->OPTnoshellopts = value->iValue;
            opt->OPTnoshellopts_given = 1;
        }
        else
            opt->OPTnoshellopts_given = 0;
        break;
    case OPT_OLDLIMIT:
        if (value) {
            opt->OPToldlimit = value->iValue;
            opt->OPToldlimit_given = 1;
        }
        else
            opt->OPToldlimit_given = 0;
        break;
    case OPT_OLDSTEPLIM:
        if (value) {
            opt->OPToldsteplim = value->iValue;
            opt->OPToldsteplim_given = 1;
        }
        else
            opt->OPToldsteplim_given = 0;
        break;
    case OPT_RENUMBER:
        if (value) {
            opt->OPTrenumber = value->iValue;
            opt->OPTrenumber_given = 1;
        }
        else
            opt->OPTrenumber_given = 0;
        break;
    case OPT_SAVECURRENT:
        if (value) {
            opt->OPTsavecurrent = value->iValue;
            opt->OPTsavecurrent_given = 1;
        }
        else
            opt->OPTsavecurrent_given = 0;
        break;
    case OPT_SPICE3:
        if (value) {
            opt->OPTspice3 = value->iValue;
            opt->OPTspice3_given = 1;
        }
        else
            opt->OPTspice3_given = 0;
        break;
    case OPT_TRAPCHECK:
        if (value) {
            opt->OPTtrapcheck = value->iValue;
            opt->OPTtrapcheck_given = 1;
        }
        else
            opt->OPTtrapcheck_given = 0;
        break;
    case OPT_TRYTOCOMPACT:
        if (value) {
            opt->OPTtrytocompact = value->iValue;
            opt->OPTtrytocompact_given = 1;
        }
        else
            opt->OPTtrytocompact_given = 0;
        break;
    case OPT_USEADJOINT:
        if (value) {
            opt->OPTuseadjoint = value->iValue;
            opt->OPTuseadjoint_given = 1;
        }
        else
            opt->OPTuseadjoint_given = 0;
        break;
    case OPT_TRANSLATE:
        if (value) {
            opt->OPTtranslate = value->iValue;
            opt->OPTtranslate_given = 1;
        }
        else
            opt->OPTtranslate_given = 0;
        break;

    case OPT_METHOD:
        if (value) {
            if (!strncasecmp(value->sValue, spkw_trap, 4) ||
                    !strcmp(value->sValue, "0")) {
                opt->OPTmethod = TRAPEZOIDAL;
                opt->OPTmethod_given = 1;
            }
            else if (!strcasecmp(value->sValue, spkw_gear) ||
                    !strcmp(value->sValue, "1")) {
                opt->OPTmethod = GEAR;
                opt->OPTmethod_given = 1;
            }
            else
                return (E_METHOD);
        }
        else
            opt->OPTmethod_given = 0;
        break;
    case OPT_OPTMERGE:
        if (value) {
            if (!strncasecmp(value->sValue, spkw_global, 3) ||
                    !strcmp(value->sValue, "0")) {
                opt->OPToptmerge = OMRG_GLOBAL;
                opt->OPToptmerge_given = 1;
            }
            else if (!strncasecmp(value->sValue, spkw_local, 3) ||
                    !strcmp(value->sValue, "1")) {
                opt->OPToptmerge = OMRG_LOCAL;
                opt->OPToptmerge_given = 1;
            }
            else if (!strcmp(value->sValue, spkw_noshell) ||
                    !strcmp(value->sValue, "2")) {
                opt->OPToptmerge = OMRG_NOSHELL;
                opt->OPToptmerge_given = 1;
            }
            else
                return (E_BADPARM);
        }
        else
            opt->OPToptmerge_given = 0;
        break;
    case OPT_PARHIER:
        if (value) {
            if (!strncasecmp(value->sValue, spkw_global, 3) ||
                    !strcmp(value->sValue, "0")) {
                opt->OPTparhier = 0;
                opt->OPTparhier_given = 1;
            }
            else if (!strncasecmp(value->sValue, spkw_local, 3) ||
                    !strcmp(value->sValue, "1")) {
                opt->OPTparhier = 1;
                opt->OPTparhier_given = 1;
            }
            else
                return (E_BADPARM);
        }
        else
            opt->OPTparhier_given = 0;
        break;
    case OPT_STEPTYPE:
        if (value) {
            if (!strncasecmp(value->sValue, spkw_interpolate, 6)  ||
                    !strcmp(value->sValue, "0")) {
                opt->OPTsteptype = STEP_NORMAL;
                opt->OPTsteptype_given = 1;
            }
            else if (!strncasecmp(value->sValue, spkw_hitusertp, 3) ||
                    !strcmp(value->sValue, "1")) {
                opt->OPTsteptype = STEP_HITUSERTP;
                opt->OPTsteptype_given = 1;
            }
            else if (!strncasecmp(value->sValue, spkw_nousertp, 2) ||
                    !strcmp(value->sValue, "2")) {
                opt->OPTsteptype = STEP_NOUSERTP;
                opt->OPTsteptype_given = 1;
            }
            else if (!strncasecmp(value->sValue, spkw_fixedstep, 2) ||
                    !strcmp(value->sValue, "3")) {
                opt->OPTsteptype = STEP_FIXEDSTEP;
                opt->OPTsteptype_given = 1;
            }
            else
                return (E_BADPARM);
        }
        else
            opt->OPTsteptype_given = 0;
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

namespace {
    IFparm OPTtbl[] = {
        IFparm(spkw_abstol,         OPT_ABSTOL,         IF_IO|IF_REAL,
            "Absolute error tolerence"),
        IFparm(spkw_chgtol,         OPT_CHGTOL,         IF_IO|IF_REAL,
            "Charge error tolerence"),
        IFparm(spkw_dcmu,           OPT_DCMU,           IF_IO|IF_REAL,
            "DC last iteration mixing parameter"),
        IFparm(spkw_defad,          OPT_DEFAD,          IF_IO|IF_REAL,
            "Default MOSfet area of drain"),
        IFparm(spkw_defas,          OPT_DEFAS,          IF_IO|IF_REAL,
            "Default MOSfet area of source"),
        IFparm(spkw_defl,           OPT_DEFL,           IF_IO|IF_REAL,
            "Default MOSfet length"),
        IFparm(spkw_defw,           OPT_DEFW,           IF_IO|IF_REAL,
            "Default MOSfet width"),
        IFparm(spkw_delmin,         OPT_DELMIN,         IF_IO|IF_REAL,
            "Minimum transient time step"),
        IFparm(spkw_dphimax,        OPT_DPHIMAX,        IF_IO|IF_REAL,
            "Max phase change per step"),
        IFparm(spkw_jjdphimax,      OPT_DPHIMAX,        IF_IO|IF_REAL,
            "Max phase change per step"),
        IFparm(spkw_gmax,           OPT_GMAX,           IF_IO|IF_REAL,
            "Maximum conductance"),
        IFparm(spkw_gmin,           OPT_GMIN,           IF_IO|IF_REAL,
            "Minimum conductance"),
        IFparm(spkw_maxdata,        OPT_MAXDATA,        IF_IO|IF_REAL,
            "Maximum output data size kilobytes"),
        IFparm(spkw_minbreak,       OPT_MINBREAK,       IF_IO|IF_REAL,
            "Minimum time between breakpoints"),
        IFparm(spkw_pivrel,         OPT_PIVREL,         IF_IO|IF_REAL,
            "Minimum acceptable ratio of pivot"),
        IFparm(spkw_pivtol,         OPT_PIVTOL,         IF_IO|IF_REAL,
            "Minimum acceptable pivot"),
        IFparm(spkw_rampup,         OPT_RAMPUP,         IF_IO|IF_REAL,
            "Transient source ramp-up time"),
        IFparm(spkw_reltol,         OPT_RELTOL,         IF_IO|IF_REAL,
            "Relative error tolerence"),
        IFparm(spkw_temp,           OPT_TEMP,           IF_IO|IF_REAL,
            "Operating temperature"),
        IFparm(spkw_tnom,           OPT_TNOM,           IF_IO|IF_REAL,
            "Nominal temperature"),
        IFparm(spkw_trapratio,      OPT_TRAPRATIO,      IF_IO|IF_REAL,
            "Trapezoid convergence test ratio"),
        IFparm(spkw_trtol,          OPT_TRTOL,          IF_IO|IF_REAL,
            "Truncation error overestimation factor"),
        IFparm(spkw_vntol,          OPT_VNTOL,          IF_IO|IF_REAL,
            "Voltage error tolerence"),
        IFparm(spkw_xmu,            OPT_XMU,            IF_IO|IF_REAL,
            "Trap/Euler mixing"),

        IFparm(spkw_bypass,         OPT_BYPASS,         IF_IO|IF_INTEGER,
            "Allow bypass of unchanging elements"),
        IFparm(spkw_fpemode,        OPT_FPEMODE,        IF_IO|IF_INTEGER,
            "Set floating-point error handling"),
        IFparm(spkw_gminsteps,      OPT_GMINSTEPS,      IF_IO|IF_INTEGER,
            "Number of Gmin steps"),
        IFparm(spkw_interplev,      OPT_INTERPLEV,      IF_IO|IF_INTEGER,
            "Degree of output interpolation poly"),
        IFparm(spkw_itl1,           OPT_ITL1,           IF_IO|IF_INTEGER,
            "DC iteration limit"),
        IFparm(spkw_itl2,           OPT_ITL2,           IF_IO|IF_INTEGER,
            "DC transfer curve iteration limit"),
        IFparm(spkw_itl2gmin,       OPT_ITL2GMIN,       IF_IO|IF_INTEGER,
            "DCOP dynamic gmin stepping iteration limit"),
        IFparm(spkw_itl2src,        OPT_ITL2SRC,        IF_IO|IF_INTEGER,
            "DCOP dynamic source stepping iteration limit"),
        IFparm(spkw_itl4,           OPT_ITL4,           IF_IO|IF_INTEGER,
            "Upper transient iteration limit"),
#ifdef WITH_THREADS
        IFparm(spkw_loadthrds,      OPT_LOADTHRDS,      IF_IO|IF_INTEGER,
            "Number of loading threads"),
        IFparm(spkw_loopthrds,      OPT_LOOPTHRDS,      IF_IO|IF_INTEGER,
            "Number of looping threads"),
#endif
        IFparm(spkw_maxord,         OPT_MAXORD,         IF_IO|IF_INTEGER,
            "Maximum integration order"),
        IFparm(spkw_srcsteps,       OPT_SRCSTEPS,       IF_IO|IF_INTEGER,
            "Number of source steps"),
        IFparm(spkw_itl6,           OPT_SRCSTEPS,       IF_IO|IF_INTEGER,
            "Number of source steps"),
        IFparm(spkw_vastep,         OPT_VASTEP,         IF_IO|IF_INTEGER,
            "Verilog time step mapping"),

        IFparm(spkw_dcoddstep,      OPT_DCODDSTEP,      IF_IO|IF_FLAG,
            "DC sweep will include end of range point if off-step"),
        IFparm(spkw_extprec,        OPT_EXTPREC,        IF_IO|IF_FLAG,
            "Use extra precision when solving real matrix"),
        IFparm(spkw_forcegmin,      OPT_FORCEGMIN,      IF_IO|IF_FLAG,
            "Enforce min gmin conductivity to ground on all nodes, always"),
        IFparm(spkw_gminfirst,      OPT_GMINFIRST,      IF_IO|IF_FLAG,
            "Try gmin stepping before source stepping"),
        IFparm(spkw_hspice,         OPT_HSPICE,         IF_IO|IF_FLAG,
            "suppress warning, promote Hspice compatibility"),
        IFparm(spkw_jjaccel,        OPT_JJACCEL,        IF_IO|IF_FLAG,
            "Accelerate Josephson-only simulation"),
        IFparm(spkw_noiter,         OPT_NOITER,         IF_IO|IF_FLAG,
            "Supress transient iterations past predictor"),
        IFparm(spkw_nojjtp,         OPT_NOJJTP,         IF_IO|IF_FLAG,
            "Supress Josephson timestep control"),
        IFparm(spkw_noklu,          OPT_NOKLU,          IF_IO|IF_FLAG,
            "Don't use KLU for matrix solving"),
        IFparm(spkw_nomatsort,      OPT_NOMATSORT,      IF_IO|IF_FLAG,
            "Don't sort sparse matrix before solving"),
        IFparm(spkw_noopiter,       OPT_NOOPITER,       IF_IO|IF_FLAG,
            "Go directly to gmin stepping"),
        IFparm(spkw_nopmdc,         OPT_NOPMDC,         IF_IO|IF_FLAG,
            "Do not allow phase-mode DC analysis"),
        IFparm(spkw_noshellopts,    OPT_NOSHELLOPTS,    IF_IO|IF_FLAG,
            "Ignore shell variables as options"),
        IFparm(spkw_oldlimit,       OPT_OLDLIMIT,       IF_IO|IF_FLAG,
            "Use SPICE2 MOSfet limiting"),
        IFparm(spkw_oldsteplim,     OPT_OLDSTEPLIM,     IF_IO|IF_FLAG,
            "Use SPICE3/WRspice-3 timestep limiting"),
        IFparm(spkw_renumber,       OPT_RENUMBER,       IF_IO|IF_FLAG,
            "Renumber input lines after subcircuit expansion"),
        IFparm(spkw_savecurrent,    OPT_SAVECURRENT,    IF_IO|IF_FLAG,
            "save all device current special vectors"),
        IFparm(spkw_spice3,         OPT_SPICE3,         IF_IO|IF_FLAG,
            "Use Spice3 timestep control"),
        IFparm(spkw_trapcheck,      OPT_TRAPCHECK,      IF_IO|IF_FLAG,
            "Do trapezoidal integration convergence test"),
        IFparm(spkw_trytocompact,   OPT_TRYTOCOMPACT,   IF_IO|IF_FLAG,
            "Try compaction for LTRA lines"),
        IFparm(spkw_useadjoint,     OPT_USEADJOINT,     IF_IO|IF_FLAG,
            "Compute adjoint matrix to measure current in some devices"),
        IFparm(spkw_translate,      OPT_TRANSLATE,      IF_IO|IF_FLAG,
            "Map node numbers into matrix assuming nodes are not compact"),

        IFparm(spkw_method,         OPT_METHOD,         IF_IO|IF_STRING,
            "Integration method"),
        IFparm(spkw_optmerge,       OPT_OPTMERGE,       IF_IO|IF_STRING,
            "Options merging method"),
        IFparm(spkw_parhier,        OPT_PARHIER,        IF_IO|IF_STRING,
            "Parameter expansion mode"),
        IFparm(spkw_steptype,       OPT_STEPTYPE,       IF_IO|IF_STRING,
            "Transient step output control"),

        IFparm(spkw_acct,           OPT_EXTERNAL,       IF_FLAG,
            "Print resource use (batch mode only)"),
        IFparm(spkw_dev,            OPT_EXTERNAL,       IF_FLAG,
            "Print device list (batch mode only)"),
        IFparm(spkw_list,           OPT_EXTERNAL,       IF_FLAG,
            "Print a listing (batch mode only)"),
        IFparm(spkw_mod,            OPT_EXTERNAL,       IF_FLAG,
            "Print model list (batch mode only)"),
        IFparm(spkw_node,           OPT_EXTERNAL,       IF_FLAG,
            "Print node voltages (batch mode only)"),
        IFparm(spkw_nopage,         OPT_EXTERNAL,       IF_FLAG,
            "Don't insert page breaks"),
        IFparm(spkw_numdgt,         OPT_EXTERNAL,       IF_INTEGER,
            "Set number of digits printed"),
        IFparm(spkw_opts,           OPT_EXTERNAL,       IF_FLAG,
            "Print options list (batch mode only)"),
        IFparm(spkw_post,           OPT_EXTERNAL,       IF_STRING,
            "Specify result file (batch mode only)"),

        IFparm(spkw_cptime,         OPT_NOTUSED,        IF_REAL,
            "Total cpu time in seconds"),
        IFparm(spkw_itl3,           OPT_NOTUSED,        IF_INTEGER,
            "Lower transient iteration limit"),
        IFparm(spkw_itl5,           OPT_NOTUSED,        IF_INTEGER,
            "Total transient iteration limit"),
        IFparm(spkw_limpts,         OPT_NOTUSED,        IF_INTEGER,
            "Maximum points per analysis"),
        IFparm(spkw_limtim,         OPT_NOTUSED,        IF_INTEGER,
            "Time to reserve for output"),
        IFparm(spkw_lvlcod,         OPT_NOTUSED,        IF_INTEGER,
            "Generate machine code"),
        IFparm(spkw_lvltim,         OPT_NOTUSED,        IF_INTEGER,
            "Type of timestep control"),
        IFparm(spkw_nomod,          OPT_NOTUSED,        IF_FLAG,
            "Don't print a model summary"),

        IFparm(spkw_delta,          OPT_DELTA,          IF_ASK|IF_REAL,
            "Transient analysis internal time step"),
        IFparm(spkw_maxdelta,       OPT_MAXDELTA,       IF_ASK|IF_REAL,
            "Transient analysis maximum internal time step"),
        IFparm(spkw_tstep,          OPT_TSTEP,          IF_ASK|IF_REAL,
            "Transient analysis print increment"),
        IFparm(spkw_tstop,          OPT_TSTOP,          IF_ASK|IF_REAL,
            "Transient analysis final time"),
        IFparm(spkw_tstart,         OPT_TSTART,         IF_ASK|IF_REAL,
            "Transient analysis start output time"),
        IFparm(spkw_fstop,          OPT_FSTOP,          IF_ASK|IF_REAL,
            "AC analysis end frequency"),
        IFparm(spkw_fstart,         OPT_FSTART,         IF_ASK|IF_REAL,
            "AC analysis start frequency")
    };
}


OPTanalysis::OPTanalysis()
{
    name = "options";
    description = "Task option selection";
    numParms = sizeof(OPTtbl)/sizeof(IFparm);
    analysisParms = OPTtbl;
    domain = NODOMAIN;
};

OPTanalysis OPTinfo;


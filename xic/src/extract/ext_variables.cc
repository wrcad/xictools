
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
 * Xic Integrated Circuit Layout and Schematic Editor                     *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#include "main.h"
#include "ext.h"
#include "ext_extract.h"
#include "ext_fc.h"
#include "ext_fh.h"
#include "ext_fxunits.h"
#include "ext_fxjob.h"
#include "ext_rlsolver.h"
#include "cd_lgen.h"
#include "errorlog.h"
#include "tech.h"
#include "tech_extract.h"
#include "tech_layer.h"


//-----------------------------------------------------------------------------
// Setup variables for extraction commands

namespace {
    // A smarter atoi()
    //
    inline bool
    str_to_int(int *iret, const char *s)
    {
        if (!s)
            return (false);
        char *e;
        *iret = strtol(s, &e, 0);
        return (e != s);
    }

    // A smarter atof()
    //
    inline bool
    str_to_dbl(double *dret, const char *s)
    {
        if (!s)
            return (false);
        return (sscanf(s, "%lf", dret) == 1);
    }

    void
    postset(const char*)
    {
        EX()->PopUpSelections(0, MODE_UPD);
    }

    void
    postset_cfg(const char*)
    {
        EX()->PopUpExtSetup(0, MODE_UPD);
    }

    bool
    evExtractOpaque(const char*, bool set)
    {
        if (EX()->isExtractOpaque() != set)
            EX()->invalidateGroups();
        EX()->setExtractOpaque(set);
        CDvdb()->registerPostFunc(postset_cfg);
        return (true);
    }

    bool
    evFlattenPrefix(const char *vstring, bool set)
    {
        delete [] EX()->flattenPrefix();
        EX()->setFlattenPrefix(set ? lstring::copy(vstring) : 0);
        EX()->invalidateGroups();
        CDvdb()->registerPostFunc(postset_cfg);
        return (true);
    }

    bool
    evGlobalExclude(const char *vstring, bool set)
    {
        if (set) {
            if (!vstring || !*vstring) {
                Log()->ErrorLog(mh::Variables,
                    "Incorrect GlobalExclude: layer expression required.");
                return (false);
            }
            // We can't init the sLspec from here, the layers might
            // not be defined yet.  The init is done in cExt::group.
        }
        EX()->invalidateGroups();
        CDvdb()->registerPostFunc(postset_cfg);
        return (true);
    }

    bool
    evGroundPlaneGlobal(const char*, bool set)
    {
        if (Tech()->IsGroundPlaneGlobal() != set)
            EX()->invalidateGroups(true);
        Tech()->SetGroundPlaneGlobal(set);
        CDvdb()->registerPostFunc(postset_cfg);
        return (true);
    }

    bool
    evGroundPlaneMulti(const char*, bool set)
    {
        if (Tech()->IsInvertGroundPlane() != set)
            EX()->invalidateGroups(true);
        Tech()->SetInvertGroundPlane(set);
        CDvdb()->registerPostFunc(postset_cfg);
        return (true);
    }

    bool
    evGroundPlaneMethod(const char *vstring, bool set)
    {
        if (set) {
            int i;
            if (str_to_int(&i, vstring) && i >= 0 && i <= 2) {
                if ((int)Tech()->GroundPlaneMode() != i) {
                    EX()->invalidateGroups(true);
                    Tech()->SetGroundPlaneMode((GPItype)i);
                }
            }
            else {
                Log()->ErrorLog(mh::Variables,
                    "Incorrect GroundPlaneMethod: range 0-2.");
                return (false);
            }
        }
        else if (Tech()->GroundPlaneMode() != GPI_PLACE) {
            EX()->invalidateGroups(true);
            Tech()->SetGroundPlaneMode(GPI_PLACE);
        }
        CDvdb()->registerPostFunc(postset_cfg);
        return (true);
    }

    bool
    evKeepShortedDevs(const char*, bool set)
    {
        if (EX()->isKeepShortedDevs() != set)
            EX()->invalidateGroups();
        EX()->setKeepShortedDevs(set);
        CDvdb()->registerPostFunc(postset_cfg);
        return (true);
    }

    bool
    evMaxAssocLoops(const char *vstring, bool set)
    {
        if (set) {
            int i;
            if (str_to_int(&i, vstring) && i >= 0 && i <= 1000000)
                cGroupDesc::set_assoc_loop_max(i);
            else {
                Log()->ErrorLog(mh::Variables,
                    "Incorrect MaxAssocLoops: range 0-1000000.");
                return (false);
            }
        }
        else
            cGroupDesc::set_assoc_loop_max(EXT_DEF_LVS_LOOP_MAX);
        CDvdb()->registerPostFunc(postset_cfg);
        return (true);
    }

    bool
    evMaxAssocIters(const char *vstring, bool set)
    {
        if (set) {
            int i;
            if (str_to_int(&i, vstring) && i >= 10 && i <= 1000000)
                cGroupDesc::set_assoc_iter_max(i);
            else {
                Log()->ErrorLog(mh::Variables,
                    "Incorrect MaxAssocIters: range 10-1000000.");
                return (false);
            }
        }
        else
            cGroupDesc::set_assoc_iter_max(EXT_DEF_LVS_ITER_MAX);
        CDvdb()->registerPostFunc(postset_cfg);
        return (true);
    }

    bool
    evNoMeasure(const char*, bool set)
    {
        if (EX()->isNoMeasure() != set)
            EX()->invalidateGroups();
        EX()->setNoMeasure(set);
        CDvdb()->registerPostFunc(postset_cfg);
        return (true);
    }

    bool
    evUseMeasurePrpty(const char*, bool set)
    {
        if (EX()->isUseMeasurePrpty() != set)
            EX()->invalidateGroups();
        EX()->setUseMeasurePrpty(set);
        CDvdb()->registerPostFunc(postset_cfg);
        return (true);
    }

    bool
    evNoReadMeasurePrpty(const char*, bool set)
    {
        if (EX()->isNoReadMeasurePrpty() != set)
            EX()->invalidateGroups();
        EX()->setNoReadMeasurePrpty(set);
        CDvdb()->registerPostFunc(postset_cfg);
        return (true);
    }

    bool
    evNoMergeParallel(const char*, bool set)
    {
        if (EX()->isNoMergeParallel() != set)
            EX()->invalidateGroups();
        EX()->setNoMergeParallel(set);
        CDvdb()->registerPostFunc(postset_cfg);
        return (true);
    }

    bool
    evNoMergeSeries(const char*, bool set)
    {
        if (EX()->isNoMergeSeries() != set)
            EX()->invalidateGroups();
        EX()->setNoMergeSeries(set);
        CDvdb()->registerPostFunc(postset_cfg);
        return (true);
    }

    bool
    evNoMergeShorted(const char*, bool set)
    {
        if (EX()->isNoMergeShorted() != set)
            EX()->invalidateGroups();
        EX()->setNoMergeShorted(set);
        CDvdb()->registerPostFunc(postset_cfg);
        return (true);
    }

    bool
    evIgnoreNetLabels(const char*, bool set)
    {
        if (EX()->isIgnoreNetLabels() != set)
            EX()->invalidateGroups();
        EX()->setIgnoreNetLabels(set);
        CDvdb()->registerPostFunc(postset_cfg);
        return (true);
    }

    bool
    evFindOldTermLabels(const char*, bool set)
    {
        if (EX()->isFindOldTermLabels() != set)
            EX()->invalidateGroups();
        EX()->setFindOldTermLabels(set);
        CDvdb()->registerPostFunc(postset_cfg);
        return (true);
    }

    bool
    evUpdateNetLabels(const char*, bool set)
    {
        if (EX()->isUpdateNetLabels() != set)
            EX()->invalidateGroups();
        EX()->setUpdateNetLabels(set);
        CDvdb()->registerPostFunc(postset_cfg);
        return (true);
    }

    bool
    evMergeMatchingNamed(const char*, bool set)
    {
        if (EX()->isMergeMatchingNamed() != set)
            EX()->invalidateGroups();
        EX()->setMergeMatchingNamed(set);
        CDvdb()->registerPostFunc(postset_cfg);
        return (true);
    }

    bool
    evMergePhysContacts(const char*, bool set)
    {
        if (EX()->isMergePhysContacts() != set)
            EX()->invalidateGroups();
        EX()->setMergePhysContacts(set);
        CDvdb()->registerPostFunc(postset_cfg);
        return (true);
    }

    bool
    evNoPermute(const char*, bool set)
    {
        if (EX()->isNoPermute() != set)
            EX()->invalidateGroups();
        EX()->setNoPermute(set);
        CDvdb()->registerPostFunc(postset_cfg);
        return (true);
    }

    bool
    evPinLayer(const char*, bool)
    {
        EX()->invalidateGroups();
        CDvdb()->registerPostFunc(postset_cfg);

        // Zero everything here, the correct values will be reset
        // elsewhere during extraction.
        EX()->setPinLayer(0);
        CDextLgen gen(CDL_CONDUCTOR);
        CDl *ld;
        while ((ld = gen.next()) != 0)
            tech_prm(ld)->set_pin_layer(0);
        return (true);
    }

    bool
    evPinPurpose(const char*, bool)
    {
        EX()->invalidateGroups();
        CDvdb()->registerPostFunc(postset_cfg);
        return (true);
    }

    bool
    evRLSolverDelta(const char *vstring, bool set)
    {
        if (set) {
            double d;
            if (str_to_dbl(&d, vstring) &&
                    INTERNAL_UNITS(d*100) >= CDphysResolution) {
                int i = INTERNAL_UNITS(d);
                if (RLsolver::rl_given_delta != i)
                    EX()->invalidateGroups();
                RLsolver::rl_given_delta = i;
            }
            else {
                Log()->ErrorLog(mh::Variables,
                    "Incorrect RLSolverDelta: requires value >= 0.01.");
                return (false);
            }
        }
        else {
            if (RLsolver::rl_given_delta != 0)
                EX()->invalidateGroups();
            RLsolver::rl_given_delta = 0;
        }
        CDvdb()->registerPostFunc(postset_cfg);
        return (true);
    }

    bool
    evRLSolverTryTile(const char*, bool set)
    {
        if (RLsolver::rl_try_tile != set)
            EX()->invalidateGroups();
        RLsolver::rl_try_tile = set;
        CDvdb()->registerPostFunc(postset_cfg);
        return (true);
    }

    bool
    evRLSolverGridPoints(const char *vstring, bool set)
    {
        if (set) {
            int i;
            if (str_to_int(&i, vstring) && i >= 10 && i <= 100000) {
                if (RLsolver::rl_numgrid != i)
                    EX()->invalidateGroups();
                RLsolver::rl_numgrid = i;
            }
            else {
                Log()->ErrorLog(mh::Variables,
                    "Incorrect RLSolverGridPoints: range 10-100000.");
                return (false);
            }
        }
        else {
            if (RLsolver::rl_numgrid != RLS_DEF_NUM_GRID)
                EX()->invalidateGroups();
            RLsolver::rl_numgrid = RLS_DEF_NUM_GRID;
        }
        CDvdb()->registerPostFunc(postset_cfg);
        return (true);
    }

    bool
    evRLSolverMaxPoints(const char *vstring, bool set)
    {
        if (set) {
            int i;
            if (str_to_int(&i, vstring) && i >= 1000 && i <= 100000) {
                if (RLsolver::rl_maxgrid != i)
                    EX()->invalidateGroups();
                RLsolver::rl_maxgrid = i;
            }
            else {
                Log()->ErrorLog(mh::Variables,
                    "Incorrect RLSolverMaxPoints: range 1000-100000.");
                return (false);
            }
        }
        else {
            if (RLsolver::rl_maxgrid != RLS_DEF_MAX_GRID)
                EX()->invalidateGroups();
            RLsolver::rl_maxgrid = RLS_DEF_MAX_GRID;
        }
        CDvdb()->registerPostFunc(postset_cfg);
        return (true);
    }

    bool
    evSubcPermutationFix(const char*, bool set)
    {
        if (EX()->isSubcPermutationFix() != set)
            EX()->invalidateGroups();
        EX()->setSubcPermutationFix(set);
        CDvdb()->registerPostFunc(postset_cfg);
        return (true);
    }

    bool
    evVerbosePromptline(const char*, bool set)
    {
        EX()->setVerbosePromptline(set);
        CDvdb()->registerPostFunc(postset_cfg);
        return (true);
    }

    bool
    evViaCheckBtwnSubs(const char*, bool set)
    {
        if (EX()->isViaCheckBtwnSubs() != set)
            EX()->invalidateGroups();
        EX()->setViaCheckBtwnSubs(set);
        CDvdb()->registerPostFunc(postset_cfg);
        return (true);
    }

    bool
    evViaSearchDepth(const char *vstring, bool set)
    {
        int d = EX()->viaSearchDepth();
        if (set) {
            int i;
            if (str_to_int(&i, vstring) && i >= 0) {
                if (i > CDMAXCALLDEPTH)
                    i = CDMAXCALLDEPTH;
                EX()->setViaSearchDepth(i);
            }
            else {
                Log()->ErrorLog(mh::Variables,
                    "Incorrect ViaSearchDepth, integer >= 0.");
                return (false);
            }
        }
        else
            EX()->setViaSearchDepth(EXT_DEF_VIA_SEARCH_DEPTH);
        if (EX()->viaSearchDepth() != d)
            EX()->invalidateGroups();
        CDvdb()->registerPostFunc(postset_cfg);
        return (true);
    }

    bool
    evViaConvex(const char*, bool set)
    {
        if (EX()->isViaConvex() != set)
            EX()->invalidateGroups();
        EX()->setViaConvex(set);
        CDvdb()->registerPostFunc(postset_cfg);
        return (true);
    }

    bool
    evNetCellThreshold(const char *vstring, bool set)
    {
        if (set) {
            int i;
            if (str_to_int(&i, vstring) && i <= 20)
                sGroupXf::set_threshold(i);
            else {
                Log()->ErrorLog(mh::Variables,
                    "Incorrect NetCellThresold, integer <= 20.");
                return (false);
            }
        }
        else
            sGroupXf::set_threshold(EXT_GRP_DEF_THRESH);
        return (true);
    }

    bool
    evQpathGroundPlane(const char *vstring, bool set)
    {
        if (set) {
            int i;
            if (str_to_int(&i, vstring) && i >= 0 && i <= 2)
                EX()->setQuickPathMode((QPtype)i);
            else {
                Log()->ErrorLog(mh::Variables,
                    "Incorrect QpathGroundPlane: range 0-2.");
                return (false);
            }
        }
        else
            EX()->setQuickPathMode(QPifavail);
        CDvdb()->registerPostFunc(postset);
        return (true);
    }

    bool
    evQpathUseConductor(const char*, bool set)
    {
        EX()->setQuickPathUseConductor(set);
        CDvdb()->registerPostFunc(postset);
        return (true);
    }

    void
    postExtCmd(const char*)
    {
        EX()->PopUpExtCmd(0, MODE_UPD, 0, 0, 0);
    }

    bool
    evExtCmd(const char*, bool)
    {
        CDvdb()->registerPostFunc(postExtCmd);
        return (true);
    }

    bool
    evPathFileVias(const char*, bool)
    {
        CDvdb()->registerPostFunc(postset);
        return (true);
    }
}

//
// FastCap Interface
//

namespace {
    void
    post_fc(const char*)
    {
        FC()->PopUpExtIf(0, MODE_UPD);
    }

    bool
    evSubstrateEps(const char *vstring, bool set)
    {
        if (set) {
            double d;
            if (str_to_dbl(&d, vstring) && d >= SUBSTRATE_EPS_MIN &&
                    d <= SUBSTRATE_EPS_MAX )
                Tech()->SetSubstrateEps(d);
            else {
                Log()->ErrorLogV(mh::Variables,
                    "Incorrect SubstrateEps, real %.3f - %.3f.",
                    SUBSTRATE_EPS_MIN, SUBSTRATE_EPS_MAX);
                return (false);
            }
        }
        else
            Tech()->SetSubstrateEps(SUBSTRATE_EPS);
        CDvdb()->registerPostFunc(post_fc);
        return (true);
    }

    bool
    evSubstrateThickness(const char *vstring, bool set)
    {
        if (set) {
            double d;
            if (str_to_dbl(&d, vstring) && d >= SUBSTRATE_THICKNESS_MIN &&
                    d <= SUBSTRATE_THICKNESS_MAX )
                Tech()->SetSubstrateThickness(d);
            else {
                Log()->ErrorLogV(mh::Variables,
                    "Incorrect SubstrateThickness, real %.3f - %.3f.",
                    SUBSTRATE_THICKNESS_MIN, SUBSTRATE_THICKNESS_MAX);
                return (false);
            }
        }
        else
            Tech()->SetSubstrateThickness(SUBSTRATE_THICKNESS);
        CDvdb()->registerPostFunc(post_fc);
        return (true);
    }

    bool
    evFC(const char*, bool)
    {
        CDvdb()->registerPostFunc(post_fc);
        return (true);
    }

    bool
    evFcPanelTarget(const char *vstring, bool set)
    {
        if (set) {
            double pmin = FC_MIN_TARG_PANELS;
            double pmax = FC_MAX_TARG_PANELS;
            double d;
            if (str_to_dbl(&d, vstring) && d >= pmin && d <= pmax)
                ;
            else {
                Log()->ErrorLogV(mh::Variables,
                    "Incorrect FcPanelTarget: range %.1f - %.1f.",
                    pmin, pmax);
                return (false);
            }
        }
        CDvdb()->registerPostFunc(post_fc);
        return (true);
    }

    bool
    evFcPlaneBloat(const char *vstring, bool set)
    {
        if (set) {
            double pmin = FC_PLANE_BLOAT_MIN;
            double pmax = FC_PLANE_BLOAT_MAX;
            double d;
            if (str_to_dbl(&d, vstring) && d >= pmin && d <= pmax)
                ;
            else {
                Log()->ErrorLogV(mh::Variables,
                    "Incorrect FcPlaneBloat: range %.1f - %.1f.",
                    pmin, pmax);
                return (false);
            }
        }
        CDvdb()->registerPostFunc(post_fc);
        return (true);
    }
}


//
// Fasthenry Interface
//

namespace {
    void
    post_fh(const char*)
    {
        FH()->PopUpExtIf(0, MODE_UPD);
    }

    bool
    evFH(const char*, bool)
    {
        CDvdb()->registerPostFunc(post_fh);
        return (true);
    }

    bool
    evFhMinRectSize(const char *vstring, bool set)
    {
        if (set) {
            double pmin = FH_MIN_RECT_SIZE_MIN;
            double pmax = FH_MIN_RECT_SIZE_MAX;
            double d;
            if (str_to_dbl(&d, vstring) && d >= pmin && d <= pmax)
                ;
            else {
                Log()->ErrorLogV(mh::Variables,
                    "Incorrect FhMinRectSize: range %.2f - %.1f.",
                    pmin, pmax);
                return (false);
            }
        }
        CDvdb()->registerPostFunc(post_fh);
        return (true);
    }

    bool
    evFhMinManhPartSize(const char *vstring, bool set)
    {
        if (set) {
            double pmin = FH_MIN_MANH_PART_SIZE_MIN;
            double pmax = FH_MIN_MANH_PART_SIZE_MAX;
            double d;
            if (str_to_dbl(&d, vstring) && d >= pmin && d <= pmax)
                ;
            else {
                Log()->ErrorLogV(mh::Variables,
                    "Incorrect FhMinManhPartSize: range %.2f - %.1f.",
                    pmin, pmax);
                return (false);
            }
        }
        CDvdb()->registerPostFunc(post_fh);
        return (true);
    }

    bool
    evFhVolElTarget(const char *vstring, bool set)
    {
        if (set) {
            double pmin = FH_MIN_TARG_VOLEL;
            double pmax = FH_MAX_TARG_VOLEL;
            double d;
            if (str_to_dbl(&d, vstring) && d >= pmin && d <= pmax)
                ;
            else {
                Log()->ErrorLogV(mh::Variables,
                    "Incorrect FhVolElTarget: range %.1f - %.1f.",
                    pmin, pmax);
                return (false);
            }
        }
        CDvdb()->registerPostFunc(post_fh);
        return (true);
    }
}


#define B 'b'
#define S 's'

namespace {
    void vsetup(const char *vname, char c, bool(*fn)(const char*, bool))
    {
        CDvdb()->registerInternal(vname,  fn);
        if (c == B)
            Tech()->RegisterBooleanAttribute(vname);
        else if (c == S)
            Tech()->RegisterStringAttribute(vname);
    }
}


// Called on program startup.
//
void
cExt::setupVariables()
{
    // Extract General
    vsetup(VA_ExtractOpaque,        B,  evExtractOpaque);
    vsetup(VA_FlattenPrefix,        S,  evFlattenPrefix);
    vsetup(VA_GlobalExclude,        S,  evGlobalExclude);
    vsetup(VA_GroundPlaneGlobal,    B,  evGroundPlaneGlobal);
    vsetup(VA_GroundPlaneMulti,     B,  evGroundPlaneMulti);
    vsetup(VA_GroundPlaneMethod,    S,  evGroundPlaneMethod);
    vsetup(VA_KeepShortedDevs,      B,  evKeepShortedDevs);

    vsetup(VA_MaxAssocLoops,        S,  evMaxAssocLoops);
    vsetup(VA_MaxAssocIters,        S,  evMaxAssocIters);
    vsetup(VA_NoMeasure,            B,  evNoMeasure);
    vsetup(VA_UseMeasurePrpty,      B,  evUseMeasurePrpty);
    vsetup(VA_NoReadMeasurePrpty,   B,  evNoReadMeasurePrpty);
    vsetup(VA_NoMergeParallel,      B,  evNoMergeParallel);
    vsetup(VA_NoMergeSeries,        B,  evNoMergeSeries);
    vsetup(VA_NoMergeShorted,       B,  evNoMergeShorted);
    vsetup(VA_IgnoreNetLabels,      B,  evIgnoreNetLabels);
    vsetup(VA_FindOldTermLabels,    B,  evFindOldTermLabels);
    vsetup(VA_UpdateNetLabels,      B,  evUpdateNetLabels);
    vsetup(VA_MergeMatchingNamed,   B,  evMergeMatchingNamed);
    vsetup(VA_MergePhysContacts,    B,  evMergePhysContacts);
    vsetup(VA_NoPermute,            B,  evNoPermute);
    vsetup(VA_PinLayer,             S,  evPinLayer);
    vsetup(VA_PinPurpose,           S,  evPinPurpose);
    vsetup(VA_RLSolverDelta,        S,  evRLSolverDelta);
    vsetup(VA_RLSolverTryTile,      B,  evRLSolverTryTile);
    vsetup(VA_RLSolverGridPoints,   S,  evRLSolverGridPoints);
    vsetup(VA_RLSolverMaxPoints,    S,  evRLSolverMaxPoints);
    vsetup(VA_SubcPermutationFix,   B,  evSubcPermutationFix);
    vsetup(VA_VerbosePromptline,    B,  evVerbosePromptline);
    vsetup(VA_ViaCheckBtwnSubs,     B,  evViaCheckBtwnSubs);
    vsetup(VA_ViaSearchDepth,       S,  evViaSearchDepth);
    vsetup(VA_ViaConvex,            B,  evViaConvex);

    vsetup(VA_NetCellThreshold,     S,  evNetCellThreshold);

    // Extract Commands
    vsetup(VA_QpathGroundPlane,     S,  evQpathGroundPlane);
    vsetup(VA_QpathUseConductor,    B,  evQpathUseConductor);
    vsetup(VA_EnetNet,              B,  evExtCmd);
    vsetup(VA_EnetSpice,            B,  evExtCmd);
    vsetup(VA_EnetBottomUp,         B,  evExtCmd);
    vsetup(VA_PnetNet,              B,  evExtCmd);
    vsetup(VA_PnetDevs,             B,  evExtCmd);
    vsetup(VA_PnetSpice,            B,  evExtCmd);
    vsetup(VA_PnetBottomUp,         B,  evExtCmd);
    vsetup(VA_PnetShowGeometry,     B,  evExtCmd);
    vsetup(VA_PnetIncludeWireCap,   B,  evExtCmd);
    vsetup(VA_PnetListAll,          B,  evExtCmd);
    vsetup(VA_PnetNoLabels,         B,  evExtCmd);
    vsetup(VA_PnetVerbose,          B,  evExtCmd);
    vsetup(VA_SourceAllDevs,        B,  evExtCmd);
    vsetup(VA_SourceCreate,         B,  evExtCmd);
    vsetup(VA_SourceClear,          B,  evExtCmd);
    vsetup(VA_NoExsetAllDevs,       B,  evExtCmd);
    vsetup(VA_NoExsetCreate,        B,  evExtCmd);
    vsetup(VA_ExsetClear,           B,  evExtCmd);
    vsetup(VA_ExsetIncludeWireCap,  B,  evExtCmd);
    vsetup(VA_ExsetNoLabels,        B,  evExtCmd);
    vsetup(VA_LvsFailNoConnect,     B,  evExtCmd);
    vsetup(VA_PathFileVias,         S,  evPathFileVias);

    // FastCap Interface
    vsetup(VA_SubstrateEps,         S,  evSubstrateEps);
    vsetup(VA_SubstrateThickness,   S,  evSubstrateThickness);
    vsetup(VA_FcArgs,               S,  evFC);
    vsetup(VA_FcForeg,              B,  evFC);
    vsetup(VA_FcLayerName,          S,  evFC);
    vsetup(VA_FcMonitor,            B,  evFC);
    vsetup(VA_FcPanelTarget,        S,  evFcPanelTarget);
    vsetup(VA_FcPath,               S,  evFC);
    vsetup(VA_FcPlaneBloat,         S,  evFcPlaneBloat);
    vsetup(VA_FcUnits,              S,  evFC);

    // FastHenry Interface
    vsetup(VA_FhArgs,               S,  evFH);
    vsetup(VA_FhForeg,              B,  evFH);
    vsetup(VA_FhFreq,               S,  evFH);
    vsetup(VA_FhMinRectSize,        S,  evFhMinRectSize);
    vsetup(VA_FhMinManhPartSize,    S,  evFhMinManhPartSize);
    vsetup(VA_FhMonitor,            B,  evFH);
    vsetup(VA_FhPath,               S,  evFH);
    vsetup(VA_FhUnits,              S,  evFH);
    vsetup(VA_FhVolElTarget,        S,  evFhVolElTarget);
}


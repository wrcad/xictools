
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

#include "config.h"
#include "main.h"
#include "drc.h"
#include "dsp_layer.h"
#include "events.h"
#include "promptline.h"
#include "tech_layer.h"


cDRC::cDRC()
{
    drc_layer_list      = 0;
    drc_rule_list       = 0;
    drc_err_fp          = 0;
    drc_err_list        = 0;
    drc_drv_tab         = 0;
    drc_job_list        = 0;
    drc_check_time      = 0;

    drc_obj_points      = 0;
    drc_obj_numpts      = 0;
    drc_obj_cur_vertex  = 0;
    drc_obj_type        = 0;

    drc_use_layer_list  = DrcListNone;
    drc_use_rule_list   = DrcListNone;

    drc_show_zoids      = false;
    drc_doing_inter     = false;
    drc_doing_grid      = false;
    drc_with_chd        = false;
    drc_abort           = false;

    drc_grid_size       = 0;

    drc_obj_count       = 0;
    drc_err_count       = 0;
    drc_num_checked     = 0;

    drc_start_time      = 0;
    drc_stop_time       = 0;

    drc_max_errors      = DRC_MAX_ERRS_DEF;
    drc_intr_max_objs   = DRC_INTR_MAX_OBJS_DEF;
    drc_intr_max_time   = DRC_INTR_MAX_TIME_DEF;
    drc_intr_max_errors = DRC_INTR_MAX_ERRS_DEF;
    drc_err_level       = DRC_ERR_LEVEL_DEF;
    drc_fudge           = DRC_FUDGE_DEF;
    drc_interactive     = DRC_INTERACTIVE_DEF;
    drc_intr_no_errmsg  = DRC_INTR_NO_ERRMSG_DEF;
    drc_intr_skip_inst  = DRC_INTR_SKIP_INST_DEF;

    memset(drc_rule_disable, 0, sizeof(drc_rule_disable));

    drc_variables       = 0;
    drc_user_tests      = 0;
    drc_test_state      = 0;

    setupInterface();
    setupBangCmds();
    setupVariables();
    setupTech();
    loadScriptFuncs();
};


// Stubs for functions defined in graphical toolkit, include when
// not building with toolkit.
#if (!defined(WITH_QT5) && !defined(WITH_QT6) && !defined(WITH_GTK2) &&\
    !defined(GTK3))
void cDRC::PopUpRules(      GRobject, ShowMode) { }
void cDRC::PopUpDrcLimits(  GRobject, ShowMode) { }
void cDRC::PopUpDrcRun(     GRobject, ShowMode) { }
void cDRC::PopUpRuleEdit(   GRobject, ShowMode, DRCtype, const char*,
    bool (*)(const char*, void*), void*, const DRCtestDesc*) { }
#endif


//-----------------------------------------------------------------------------
// Keyboard action handler

bool
cDRC::actionHandler(WindowDesc *wdesc, int a)
{
    switch (a) {  // a is eKeyAction enum type
    case DRCb_action:
        if (DRC()->errCb(2, PL()->KeyBuf(wdesc), PL()->KeyPos(wdesc)) != 0) {
            PL()->SetKeys(wdesc, 0);
            PL()->ShowKeys(wdesc);
            return (true);
        }
        return (false);
    case DRCf_action:
        if (DRC()->errCb(6, PL()->KeyBuf(wdesc), PL()->KeyPos(wdesc)) != 0) {
            PL()->SetKeys(wdesc, 0);
            PL()->ShowKeys(wdesc);
            return (true);
        }
        return (false);
    case DRCp_action:
        if (DRC()->errCb(16, PL()->KeyBuf(wdesc), PL()->KeyPos(wdesc)) != 0) {
            PL()->SetKeys(wdesc, 0);
            PL()->ShowKeys(wdesc);
            return (true);
        }
        return (false);
    default:
        break;
    }
    return (false);
}


// Export, return the MinWidth value, if any.
//
int
cDRC::minDimension(CDl *ld)
{
    if (!ld)
        return (0);
    for (DRCtestDesc *td = *tech_prm(ld)->rules_addr(); td; td = td->next()) {
        if (td->type() == drMinWidth && !td->hasRegion())
            return (td->dimen());
    }
    return (0);
}


// This is called after the current layer changes.
//
void
cDRC::layerChangeCallback()
{
    PopUpRules(0, MODE_UPD);
}


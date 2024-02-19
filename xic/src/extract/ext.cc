
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
#include "ext.h"
#include "ext_extract.h"
#include "ext_fc.h"
#include "ext_fh.h"
#include "ext_rlsolver.h"
#include "ext_errlog.h"
#include "dsp_tkif.h"
#include "dsp_layer.h"
#include "dsp_inlines.h"
#include "layertab.h"


// Instantiate error logger.
cExtErrLog ExtErrLog;

// Permutations for 2,3,4 signals.
char sExtPermGrpB::p2[2][2] = {{0, 1}, {1, 0}};
char sExtPermGrpB::p3[6][3] = {
    {0, 1, 2}, {0, 2, 1}, {1, 0, 2},
    {1, 2, 0}, {2, 0, 1}, {2, 1, 0}};
char sExtPermGrpB::p4[24][4] = {
    {0, 1, 2, 3}, {0, 1, 3, 2}, {0, 2, 3, 1}, {0, 2, 1, 3},
    {0, 3, 1, 2}, {0, 3, 2, 1},
    {1, 2, 3, 0}, {1, 2, 0, 3}, {1, 3, 0, 2}, {1, 3, 2, 0},
    {1, 0, 2, 3}, {1, 0, 3, 2},
    {2, 3, 0, 1}, {2, 3, 1, 0}, {2, 0, 1, 3}, {2, 0, 3, 1},
    {2, 1, 3, 0}, {2, 1, 0, 3},
    {3, 0, 1, 2}, {3, 0, 2, 1}, {3, 1, 2, 0}, {3, 1, 0, 2},
    {3, 2, 0, 1}, {3, 2, 1, 0}};

bool cExt::ext_via_convex = false;

cExt::cExt()
{
    ext_selected_devices        = 0;
    ext_devtmpl_tab             = 0;
    ext_device_tab              = 0;
    ext_subckt_tab              = 0;
    ext_reference_tab           = 0;
    ext_dead_devices            = 0;
    ext_flatten_prefix          = 0;
    ext_gp_layer                = 0;
    ext_gp_layer_inv            = 0;
    ext_param_cx                = 0;
    ext_pin_layer               = 0;
    ext_tech_devtmpls           = 0;

    ext_gp_inv_set              = false;
    ext_blink_sels              = false;
    ext_subpath_enabled         = false;
    ext_gn_show_path            = false;
    ext_extraction_view         = false;
    ext_extraction_select       = false;
    ext_showing_groups          = false;
    ext_showing_nodes           = false;
    ext_showing_devs            = false;
    ext_no_merge_parallel       = false;
    ext_no_merge_series         = false;
    ext_no_merge_shorted        = false;
    ext_extract_opaque          = false;
    ext_keep_shorted_devs       = false;
    ext_devsel_compute          = false;
    ext_devsel_compare          = false;
    ext_verbose_prompt          = false;
    ext_qp_use_conductor        = false;

    ext_ign_net_labels          = false;
    ext_upd_net_labels          = false;
    ext_find_old_term_labels    = false;
    ext_merge_named             = false;
    ext_no_permute              = false;
    ext_no_measure              = false;
    ext_use_meas_prop           = false;
    ext_no_read_meas_prop       = false;
    ext_ignore_group_names      = false;
    ext_merge_phys_conts        = false;
    ext_subc_permute_fix        = false;
    ext_via_check_btwn_subs     = false;

    ext_qp_mode                 = QPifavail;
    ext_path_depth              = CDMAXCALLDEPTH;
    ext_via_search_depth        = EXT_DEF_VIA_SEARCH_DEPTH;
    ext_ghost                   = new cExtGhost;
    ext_pathfinder              = 0;
    ext_terminals               = 0;

    setupBangCmds();
    setupVariables();
    setupTech();
    loadScriptFuncs();
}

// Stubs for functions defined in graphical toolkit, include when
// not building with toolkit.
#if (!defined(WITH_QT5) && !defined(WITH_QT6) && !defined(WITH_GTK2) &&\
    !defined(GTK3))
void cExt::PopUpExtCmd(     GRobject, ShowMode, sExtCmd*,
    bool (*)(const char*, void*, bool, const char*, int, int), void*, int) { }
void cExt::PopUpDevices(    GRobject, ShowMode) { }
void cExt::PopUpSelections( GRobject, ShowMode) { }
void cExt::PopUpExtSetup(   GRobject, ShowMode) { }
void cExt::PopUpPhysTermEdit(GRobject, ShowMode, TermEditInfo*,
    void(*)(TermEditInfo*, CDsterm*), CDsterm*, int, int) { }
#endif


//-----------------------------------------------------------------------------
// Misc. for export


// Export for use in script interpreter.
//
CDs *
cExt::cellDesc(cGroupDesc *gd)
{
    return (gd ? gd->cell_desc() : 0);
}


// Consolidated export for drawing static highlighting.
//
void
cExt::windowShowHighlighting(WindowDesc *wd)
{
    // Show conductor group/node numbers.
    showExtract(wd, DISPLAY);

    // Show the current path.
    showCurrentPath(wd, DISPLAY);

    EX()->showTerminals(wd, DISPLAY);
    EX()->showMeasureBox(wd, DISPLAY);
}


// Consolidated export for drawing blinking highlighting.
//
void
cExt::windowShowBlinking(WindowDesc *wd)
{
    if (isBlinkSelections())
        showCurrentPath(wd, DISPLAY);
    showSelectedDevices(wd);
}


// This is called after the current layer changes.
//
void
cExt::layerChangeCallback()
{
    // Nothing to do here at present.
}


// This is called before the current cell changes.
//
void
cExt::preCurCellChangeCallback()
{
    // Un-highlight devices.
    CDs *cursdp = CurCell(Physical);
    if (cursdp) {
        cGroupDesc *gd = cursdp->groups();
        if (gd)
            gd->parse_find_dev(0, ERASE);
    }
    FCif()->clearMarks();
}


// This is called after the current cell changes.
//
void
cExt::postCurCellChangeCallback()
{
    // Fix the group display.
    if (ext_showing_groups) {
        CDs *cursdp = CurCell(Physical);
        if (cursdp) {
            group(cursdp, 0);
            cGroupDesc *gd = cursdp->groups();
            if (gd)
                gd->set_group_display(true);
        }
    }

    PopUpDevices(0, MODE_UPD);
}


// This is called after global deselection.
//
void
cExt::deselect()
{
    queueDevices(0);
}


// This is called after mode change.
//
void
cExt::postModeSwitchCallback()
{
    PopUpDevices(0, MODE_UPD);
}


// Create the preset variables that are available from the formatting
// script functions.
//
siVariable *
cExt::createFormatVars()
{
    siVariable *variables = 0;
    siVariable *v = new siVariable;
    v->name = lstring::copy("_cellname");
    v->type = TYP_NOTYPE;
    v->next = variables;
    variables = v;

    v = new siVariable;
    v->name = lstring::copy("_viewname");
    v->type = TYP_NOTYPE;
    v->next = variables;
    variables = v;

    v = new siVariable;
    v->name = lstring::copy("_techname");
    v->type = TYP_NOTYPE;
    v->next = variables;
    variables = v;

    v = new siVariable;
    v->name = lstring::copy("_num_nets");
    v->type = TYP_SCALAR;
    v->next = variables;
    variables = v;

    v = new siVariable;
    v->name = lstring::copy("_mode");
    v->type = TYP_SCALAR;
    v->next = variables;
    variables = v;

    v = new siVariable;
    v->name = lstring::copy("_list_all");
    v->type = TYP_SCALAR;
    v->next = variables;
    variables = v;

    v = new siVariable;
    v->name = lstring::copy("_bottom_up");
    v->type = TYP_SCALAR;
    v->next = variables;
    variables = v;

    v = new siVariable;
    v->name = lstring::copy("_show_geom");
    v->type = TYP_SCALAR;
    v->next = variables;
    variables = v;

    v = new siVariable;
    v->name = lstring::copy("_show_wire_cap");
    v->type = TYP_SCALAR;
    v->next = variables;
    variables = v;

    v = new siVariable;
    v->name = lstring::copy("_ignore_labels");
    v->type = TYP_SCALAR;
    v->next = variables;
    variables = v;

    return (variables);
}


// Logger interface.
//
bool
cExt::rlsolverMsgs()
{
    return (ExtErrLog.rlsolver_msgs());
}


// Logger interface.
//
void
cExt::setRLsolverMsgs(bool b)
{
    ExtErrLog.set_rlsolver_msgs(b);
}


// Logger interface.
//
bool
cExt::logRLsolver()
{
    return (ExtErrLog.log_rlsolver());
}


// Logger interface.
//
void
cExt::setLogRLsolver(bool b)
{
    ExtErrLog.set_log_rlsolver(b);
}


// Logger interface.
//
bool
cExt::logGrouping()
{
    return (ExtErrLog.log_grouping());
}


// Logger interface.
//
void
cExt::setLogGrouping(bool b)
{
    ExtErrLog.set_log_grouping(b);
}


// Logger interface.
//
bool
cExt::logExtracting()
{
    return (ExtErrLog.log_extracting());
}


// Logger interface.
//
void
cExt::setLogExtracting(bool b)
{
    ExtErrLog.set_log_extracting(b);
}


// Logger interface.
//
bool
cExt::logAssociating()
{
    return (ExtErrLog.log_associating());
}


// Logger interface.
//
void
cExt::setLogAssociating(bool b)
{
    ExtErrLog.set_log_associating(b);
}


// Logger interface.
//
bool
cExt::logVerbose()
{
    return (ExtErrLog.verbose());
}


// Logger interface.
//
void
cExt::setLogVerbose(bool b)
{
    ExtErrLog.set_verbose(b);
}



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

#ifndef EXT_FC2_H
#define EXT_FC2_H

#include "geo_3d.h"
#include "tech_ldb3d.h"
#include "ext_fxunits.h"
#include "ext_fxjob.h"

//
// Definitions for the "new" Fast[er]Cap extraction system.
//

// Variables
#define VA_FcArgs               "FcArgs"
#define VA_FcForeg              "FcForeg"
#define VA_FcLayerName          "FcLayerName"
#define VA_FcMonitor            "FcMonitor"
#define VA_FcPanelTarget        "FcPanelTarget"
#define VA_FcPath               "FcPath"
#define VA_FcPlaneBloat         "FcPlaneBloat"
#define VA_FcUnits              "FcUnits"

#define FC_LAYER_NAME           "FCAP"

#define FC_MAX_TARG_PANELS      1e6
#define FC_MIN_TARG_PANELS      1e3
#define FC_DEF_TARG_PANELS      1e4

// Default value and range of substrate bloat parameter.
#define FC_PLANE_BLOAT_DEF      0.0
#define FC_PLANE_BLOAT_MIN      0.0
#define FC_PLANE_BLOAT_MAX      1000.0

// File extensions
#define FC_LST_SFX              "lst"

// Class integer for cross marks, should be unique among cross mark
// users.
#define CROSS_MARK_FC           1

// These are probably temporary, for debugging, not documented.
#define MRG_C_ZBOT              0x1
#define MRG_C_ZTOP              0x2
#define MRG_C_YL                0x4
#define MRG_C_YU                0x8
#define MRG_C_LEFT              0x10
#define MRG_C_RIGHT             0x20
#define MRG_D_ZBOT              0x100
#define MRG_D_ZTOP              0x200
#define MRG_D_YL                0x400
#define MRG_D_YU                0x800
#define MRG_D_LEFT              0x1000
#define MRG_D_RIGHT             0x2000
#define MRG_ALL                 0x3f3f
#define VA_FcZoids              "FcZoids"
#define VA_FcVerboseOut         "FcVerboseOut"
#define VA_FcNoMerge            "FcNoMerge"
#define VA_FcMergeFlags         "FcMergeFlags"
#define VA_FcDebug              "FcDebug"

enum fcPanelType
{
    fcP_NONE,
    fcP_ZBOT,
    fcP_ZTOP,
    fcP_YL,
    fcP_YU,
    fcP_LEFT,
    fcP_RIGHT
};

// Base panel type.
//
struct fcPanel
{
    fcPanel(fcPanelType t, int x1, int y1, int z1, int x2, int y2, int z2,
            int x3, int y3, int z3, int x4, int y4, int z4,
            double prm) :
        Q(x1, y1, z1, x2, y2, z2, x3, y3, z3, x4, y4, z4)
        {
            outperm = prm;
            type = t;
        }


    qface3d Q;
    double outperm;
    fcPanelType type;
};

// Conductor panel list element.
//
struct fcCpanel : public fcPanel
{
    fcCpanel(fcPanelType t, int x1, int y1, int z1, int x2, int y2, int z2,
            int x3, int y3, int z3, int x4, int y4, int z4,
            int g, double prm) :
        fcPanel(t, x1, y1, z1, x2, y2, z2, x3, y3, z3, x4, y4, z4, prm)
        {
            next = 0;
            group = g;
        }

    static void destroy(fcCpanel *p)
        {
            while (p) {
                fcCpanel *px = p;
                p = p->next;
                delete px;
            }
        }

    void print_c(FILE*, const char*, e_unit) const;
    static void print_c_term(FILE*, bool);
    static void print_panel_begin(FILE*, const char*);
    void print_panel(FILE*, int, int, e_unit, unsigned int,
        unsigned int*) const;
    static void print_panel_end(FILE*);

    fcCpanel *next;
    int group;
};

// Dielectric panel list element.
//
struct fcDpanel : public fcPanel
{
    fcDpanel(fcPanelType t, int x1, int y1, int z1, int x2, int y2, int z2,
            int x3, int y3, int z3, int x4, int y4, int z4,
            int ix, double ip, double op) :
        fcPanel(t, x1, y1, z1, x2, y2, z2, x3, y3, z3, x4, y4, z4, op)
        {
            next = 0;
            inperm = ip;
            index = ix;
        }

    static void destroy(fcDpanel *p)
        {
            while (p) {
                fcDpanel *px = p;
                p = p->next;
                delete px;
            }
        }

    void print_d(FILE*, const char*, e_unit, const xyz3d* = 0) const;
    static void print_panel_begin(FILE*, const char*);
    void print_panel(FILE*, int, int, e_unit, unsigned int,
        unsigned int*) const;
    static void print_panel_end(FILE*);

    fcDpanel *next;
    double inperm;
    int index;
};

// Coordinate and layer to mark a group, this will be arrayed and the
// index is the group number.  Returned from fcLayout::group_points().
//
struct fcGrpPtr : public Point
{
    const CDl *layer_desc;
};

// Description of the structure.
//
struct fcLayout : public Ldb3d
{
    fcLayout();

    bool setup_refine(double);
    bool write_panels(FILE*, int, int, e_unit);
    fcGrpPtr *group_points() const;

    static void clear_dbg_zlist();

private:
    void write_subs_panels(FILE*, FILE*, int, int, e_unit);
    void write_d_panels(FILE*, FILE*, fcDpanel*, char*, int, int, int, e_unit);
    fcCpanel *panelize_group_zbot(const glZlistRef3d*) const;
    fcCpanel *panelize_group_ztop(const glZlistRef3d*) const;
    fcCpanel *panelize_group_yl(const glZlistRef3d*) const;
    fcCpanel *panelize_group_yu(const glZlistRef3d*) const;
    fcCpanel *panelize_group_left(const glZlistRef3d*) const;
    fcCpanel *panelize_group_right(const glZlistRef3d*) const;
    fcDpanel *panelize_dielectric_zbot(const Layer3d*) const;
    fcDpanel *panelize_dielectric_ztop(const Layer3d*) const;
    fcDpanel *panelize_dielectric_yl(const Layer3d*) const;
    fcDpanel *panelize_dielectric_yu(const Layer3d*) const;
    fcDpanel *panelize_dielectric_left(const Layer3d*) const;
    fcDpanel *panelize_dielectric_right(const Layer3d*) const;
    double area_group_zbot(const glZlistRef3d*) const;
    double area_group_ztop(const glZlistRef3d*) const;
    double area_group_yl(const glZlistRef3d*) const;
    double area_group_yu(const glZlistRef3d*) const;
    double area_group_left(const glZlistRef3d*) const;
    double area_group_right(const glZlistRef3d*) const;
    double area_dielectric_zbot(const Layer3d*) const;
    double area_dielectric_ztop(const Layer3d*) const;
    double area_dielectric_yl(const Layer3d*) const;
    double area_dielectric_yu(const Layer3d*) const;
    double area_dielectric_left(const Layer3d*) const;
    double area_dielectric_right(const Layer3d*) const;

    double fcl_cond_area;           // Total C panel area.
    double fcl_diel_area;           // Total D panel area.
    double fcl_numpanels_target;    // Partitioning target panel count.
    unsigned int fcl_maxdim;                 // Max partition grid length.
    unsigned int fcl_num_c_panels_raw;       // Number of unpart. C panels.
    unsigned int fcl_num_c_panels_written;   // Number of C panels in file.
    unsigned int fcl_num_d_panels_raw;       // Number of unpart. D panels.
    unsigned int fcl_num_d_panels_written;   // Number of D panels in file.

    // debugging
    bool fcl_zoids;                          // Create top/btm zoids in db.
    bool fcl_verbose_out;                    // Lots of info in output.
    bool fcl_domerge;           // Use the panel merging code.
    unsigned int fcl_mrgflgs;   // Flags to select individual merge ops.
};


inline class cFC *FCif();

// New interface to the FasterCap capacitance extraction program from
// FastFieldSolvers.com.
//
class cFC : public fxGif
{
    static cFC *ptr()
        {
            if (!instancePtr)
                on_null_ptr();
            return (instancePtr);
        }

    static void on_null_ptr();

public:
    friend inline cFC *FCif() { return (cFC::ptr()); }

    // ext_fc.cc
    cFC();
    void doCmd(const char*, const char*);
    bool fcDump(const char*);
    void fcRun(const char*, const char*, const char*, bool = false);
    char *getFileName(const char*, int i = -1);
    const char *getUnitsString(const char*);
    int getUnitsIndex(const char*);
    char *statusString();
    void showMarks(bool);
    char *jobList();
    void killProcess(int);

    // graphics
    void PopUpExtIf(GRobject, ShowMode);
    void updateString();
    void updateMarks();
    void clearMarks();

    // Used by the control panel to record visibility.
    //
    void setPopUpVisible(bool vis)  { fc_popup_visible = vis; }

private:
    fcGrpPtr *fc_groups;        // Saved group info for display.
    int fc_ngroups;
    bool fc_popup_visible;      // True when controlling pop-up is displayed.

    static cFC *instancePtr;
};

// As in geo_zoid.h, but don't return true if touching only.
struct fc_ovlchk_t
{
    fc_ovlchk_t(int l, int r, int y) { minleft = l; maxright = r; yu = y; }
 
    // Return true if Z can't overlap saved reference zoid, but the next
    // Z in order might overlap.
    //
    bool check_continue(const Zoid &Z)
        {
            return (Z.maxright() < minleft || Z.yl > yu);
        }

    // Return true if Z can't overlap saved reference zoid, and no other
    // ordered zoids can overlap.
    //
    bool check_break(const Zoid &Z)
        {  
            return (Z.minleft() > maxright);
        }
  
private:   
    int maxright;
    int minleft;
    int yu;
};

#endif


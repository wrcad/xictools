
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

#ifndef TECH_LAYER_H
#define TECH_LAYER_H

#include "miscutil/hashfunc.h"


//
// Application-level things to attach to a layer descriptor.
//

struct DRCtestDesc;
struct ParseNode;
struct sLspec;
struct sVia;

enum tRouteDir { tDirNone, tDirHoriz, tDirVert };

// FastHenry defaults
#define DEF_FH_NHINC    1
#define DEF_FH_RH       2.0

struct TechLayerParams
{
    // layers.cc
    TechLayerParams(CDl*);
    ~TechLayerParams();

    // Never use this to add a rule, unless the correct list order
    // is maintained.
    DRCtestDesc **rules_addr()          { return (&lp_rules); }

    CDl *pin_layer()                    { return (lp_pinld); }
    void set_pin_layer(CDl *ld)         { lp_pinld = ld; }
    sLspec *exclude()                   { return (lp_exclude); }
    void set_exclude(sLspec *e)         { lp_exclude = e; }
    sVia *via_list()                    { return (lp_vialist); }
    void set_via_list(sVia *v)          { lp_vialist = v; }

    double rho()                        { return (lp_rho); }
    void set_rho(double d)              { lp_rho = d; }
    double ohms_per_sq()                { return (lp_ohms_per_sq); }
    void set_ohms_per_sq(double d)      { lp_ohms_per_sq = d; }

    double epsrel()                     { return (lp_epsrel); }
    void set_epsrel(double d)           { lp_epsrel = d; }
    double cap_per_area()               { return (lp_cap_per_area); }
    void set_cap_per_area(double d)     { lp_cap_per_area = d; }
    double cap_per_perim()              { return (lp_cap_per_perim); }
    void set_cap_per_perim(double d)    { lp_cap_per_perim = d; }

    double lambda()                     { return (lp_lambda); }
    void set_lambda(double d)           { lp_lambda = d; }

    const char *gp_lname()              { return (lp_gp_lname); }
    void set_gp_lname(char *s)          { lp_gp_lname = s; }
    double diel_thick()                 { return (lp_diel_thick); }
    void set_diel_thick(double d)       { lp_diel_thick = d; }
    double diel_const()                 { return (lp_diel_const); }
    void set_diel_const(double d)       { lp_diel_const = d; }

    int fh_nhinc()                      { return (lp_fh_nhinc); }
    void set_fh_nhinc(int n)            { lp_fh_nhinc = n; }
    double fh_rh()                      { return (lp_fh_rh); }
    void set_fh_rh(double d)            { lp_fh_rh = d; }

    double ant_ratio()                  { return (lp_ant_ratio); }
    void set_ant_ratio(double d)        { lp_ant_ratio = d; }

    tRouteDir route_dir()               { return (lp_route_dir); }
    void set_route_dir(tRouteDir d)     { lp_route_dir = d; }
    int route_h_pitch()                 { return (lp_route_h_pitch); }
    void set_route_h_pitch(int p)       { lp_route_h_pitch = p; }
    int route_v_pitch()                 { return (lp_route_v_pitch); }
    void set_route_v_pitch(int p)       { lp_route_v_pitch = p; }
    int route_h_offset()                { return (lp_route_h_offset); }
    void set_route_h_offset(int o)      { lp_route_h_offset = o; }
    int route_v_offset()                { return (lp_route_v_offset); }
    void set_route_v_offset(int o)      { lp_route_v_offset = o; }
    int route_width()                   { return (lp_route_width); }
    void set_route_width(int w)         { lp_route_width = w; }
    int route_max_dist()                { return (lp_route_max_dist); }
    void set_route_max_dist(int d)      { lp_route_max_dist = d; }
    int spacing()                       { return (lp_spacing); }
    void set_spacing(int s)             { lp_spacing = s; }

private:
    // Used in DRC.
    DRCtestDesc *lp_rules;              // design rule list

    // Used in Extract.
    CDl *lp_pinld;                      // PIN purpose layer for this
    sLspec *lp_exclude;                 // exclude for conductor grouping
    sVia *lp_vialist;                   // via list

    // Resistance parameters (Extract).
    float lp_rho;                       // resistivity, ohm-meter
    float lp_ohms_per_sq;

    // Capacitance parameters (Extract).
    float lp_epsrel;                    // relative permititivity
    float lp_cap_per_area;              // pF per sq micron
    float lp_cap_per_perim;             // pF per micron

    // Superconducting penetration depth (Extract).
    float lp_lambda;                    // London penetration depth, microns

    // Transmission line (Extract).
    char *lp_gp_lname;                  // ground plane layer name
    float lp_diel_thick;                // assumed dielectric thickness
    float lp_diel_const;                // assumed rel. dielectric const.

    // FastHenry interface (Extract).
    int lp_fh_nhinc;                    // FastHenry filaments
    float lp_fh_rh;                     // FastHenry filament height ratio

    // Misc. (Extract).
    float lp_ant_ratio;                 // antenna ratio

    // Routing info.
    tRouteDir lp_route_dir;             // route direction
    int lp_route_h_pitch;               // period of horiz routing grid
    int lp_route_v_pitch;               // period of vert routing grid
    int lp_route_h_offset;              // offset of horiz routing grid
    int lp_route_v_offset;              // offset of vert routing grid
    int lp_route_width;                 // routing path width
    int lp_route_max_dist;              // routing max distance
    int lp_spacing;                     // route or cut spacing
};


// Return the params struct.
//
inline TechLayerParams *
tech_prm(const CDl *ld)
{
    return (static_cast<TechLayerParams*>(ld->appData()));
}

#endif


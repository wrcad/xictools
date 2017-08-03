
/*========================================================================*
 *                                                                        *
 *  Whiteley Research Inc, Sunnyvale, CA USA, http://wrcad.com            *
 *  Copyright (C) 2017 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *  Licensed under the Apache License, Version 2.0 (the "License");       *
 *  you may not use this file except in compliance with the License.      *
 *  You may obtain a copy of the License at                               *
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

#ifndef DSP_SNAP_H
#define DSP_SNAP_H

//
// An object for taking care of grid and edge snapping.
//

struct DSPsnapper
{
    DSPsnapper()
        {
            sn_mode             = Physical;
            sn_x_init           = 0;
            sn_y_init           = 0;
            sn_x_final          = 0;
            sn_y_final          = 0;
            sn_coarse_resol     = 0;
            sn_fine_resol       = 0;
            sn_x_snapped        = false;
            sn_y_snapped        = false;
            sn_vertex           = false;

            sn_indicate         = false;
            sn_use_edges        = false;
            sn_allow_off_grid   = false;
            sn_do_non_manh      = false;
            sn_do_wire_edges    = false;
            sn_do_wire_path     = false;
        }

    void set_indicate(bool b)           { sn_indicate = b; }
    void set_use_edges(bool b)          { sn_use_edges = b; }
    void set_allow_off_grid(bool b)     { sn_allow_off_grid = b; }
    void set_do_non_manh(bool b)        { sn_do_non_manh = b; }
    void set_do_wire_edges(bool b)      { sn_do_wire_edges = b; }
    void set_do_wire_path(bool b)       { sn_do_wire_path = b; }

    void snap(WindowDesc*, int*, int*);

private:
    bool check_snap(int*);
    bool check_x_edge(const BBox&, int*, int);
    bool check_y_edge(const BBox&, int*, int);
    bool check_region(int, int, int);
    void check_box(const BBox&, const BBox&);
    void check_poly(const Poly&, const BBox&, bool);
    void check_wire(const Wire&, const BBox&, bool);
    void check_snap_edges(WindowDesc*, int);

    DisplayMode sn_mode;
    int sn_x_init;
    int sn_y_init;
    int sn_x_final;
    int sn_y_final;
    int sn_coarse_resol;
    int sn_fine_resol;
    bool sn_x_snapped;
    bool sn_y_snapped;
    bool sn_vertex;

    bool sn_indicate;
    bool sn_use_edges;
    bool sn_allow_off_grid;
    bool sn_do_non_manh;
    bool sn_do_wire_edges;
    bool sn_do_wire_path;
};

#endif


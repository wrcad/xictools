
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

#ifndef DSP_WINDOW_H
#define DSP_WINDOW_H

#include "dsp_grid.h"


//
// Definitions having to do with management of the Xic drawing windows.
//

class cCHD;
struct bnd_draw_t;
struct symref_t;
struct DSPwbag;
struct hyParent;
namespace ginterf
{
    struct RGBzimg;
    struct GRdraw;
    struct GRvecFont;
    struct GRimage;
}

// Several functions use these inputs.
#define ERASE false
#define DISPLAY true

// Enumeration for three levels of edge snapping support.
enum EdgeSnapMode { EdgeSnapNone, EdgeSnapSome, EdgeSnapAll };

// Per-window display attributes
//
struct DSPattrib
{
    DSPattrib();

    GridDesc *grid(DisplayMode m)
        {
            return (m == Physical ? &a_phys_grid : &a_elec_grid);
        }
    int expand_level(DisplayMode m) const
        {
            return (m == Physical ?
                a_phys_expand_level : a_elec_expand_level);
        }
    SLtype display_labels(DisplayMode m) const
        {
            return (m == Physical ?
                a_phys_display_labels : a_elec_display_labels);
        }
    SLtype label_instances(DisplayMode m) const
        {
            return (m == Physical ?
                a_phys_label_instances : a_elec_label_instances);
        }
    bool show_context(DisplayMode m) const
        {
            return (m == Physical ?
                a_phys_show_context : a_elec_show_context);
        }
    bool show_tiny_bb(DisplayMode m) const
        {
            return (m == Physical ?
                a_phys_show_tiny_bb : a_elec_show_tiny_bb);
        }
    bool no_show_unexpand(DisplayMode m) const
        {
            return (m == Physical ?
                a_phys_no_show_unexpand : a_elec_no_show_unexpand);
        }

    bool show_phys_props()              const { return (a_phys_props); }
    bool show_no_highlighting()         const { return (a_no_highlighting); }
    bool no_elec_symbolic()             const { return (a_no_elec_symbolic); }
    bool showing_boxes()                const { return (a_show_boxes); }
    bool showing_polys()                const { return (a_show_polys); }
    bool showing_wires()                const { return (a_show_wires); }
    bool showing_labels()               const { return (a_show_labels); }

    EdgeSnapMode edge_snapping()        const { return ((EdgeSnapMode)
                                                  a_edge_snapping); }
    bool edge_off_grid()                const { return (a_edge_off_grid); }
    bool edge_non_manh()                const { return (a_edge_non_manh); }
    bool edge_wire_edge()               const { return (a_edge_wire_edge); }
    bool edge_wire_path()               const { return (a_edge_wire_path); }

    void set_expand_level(DisplayMode m, int l)
        {
            if (l < -1 || l >= CDMAXCALLDEPTH)
                l = -1;
            if (m == Physical)
                a_phys_expand_level = l;
            else
                a_elec_expand_level = l;
        }
    void set_display_labels(DisplayMode m, SLtype d)
        {
            if (m == Physical)
                a_phys_display_labels = d;
            else
                a_elec_display_labels = d;
        }
    void set_label_instances(DisplayMode m, SLtype d)
        {
            if (m == Physical)
                a_phys_label_instances = d;
            else
                a_elec_label_instances = d;
        }
    void set_show_context(DisplayMode m, bool b)
        {
            if (m == Physical)
                a_phys_show_context = b;
            else
                a_elec_show_context = b;
        }
    void set_show_tiny_bb(DisplayMode m, bool b)
        {
            if (m == Physical)
                a_phys_show_tiny_bb = b;
            else
                a_elec_show_tiny_bb = b;
        }
    void set_no_show_unexpand(DisplayMode m, bool b)
        {
            if (m == Physical)
                a_phys_no_show_unexpand = b;
            else
                a_elec_no_show_unexpand = b;
        }

    void set_show_phys_props(bool b)            { a_phys_props = b; }
    void set_show_no_highlighting(bool b)       { a_no_highlighting = b; }
    void set_no_elec_symbolic(bool b)           { a_no_elec_symbolic = b; }
    void set_showing_boxes(bool b)              { a_show_boxes = b; }
    void set_showing_polys(bool b)              { a_show_polys = b; }
    void set_showing_wires(bool b)              { a_show_wires = b; }
    void set_showing_labels(bool b)             { a_show_labels = b; }

    void set_edge_snapping(EdgeSnapMode m)      { a_edge_snapping = m; }
    void set_edge_off_grid(bool b)              { a_edge_off_grid = b; }
    void set_edge_non_manh(bool b)              { a_edge_non_manh = b; }
    void set_edge_wire_edge(bool b)             { a_edge_wire_edge = b; }
    void set_edge_wire_path(bool b)             { a_edge_wire_path = b; }

private:
    // The grid to render in this window.
    GridDesc a_phys_grid;
    GridDesc a_elec_grid;

    // Level to expand instances: 0 no expansion, -1 all.
    int a_phys_expand_level;
    int a_elec_expand_level;

    // Label display control.
    SLtype a_phys_display_labels;
    SLtype a_elec_display_labels;

    // Code to label instances in the viewport.
    SLtype a_phys_label_instances;
    SLtype a_elec_label_instances;

    // If true, show context in subedit.
    bool a_phys_show_context;
    bool a_elec_show_context;

    // If true, show the BB of tiny cells rather than detail.
    bool a_phys_show_tiny_bb;
    bool a_elec_show_tiny_bb;

    // If true, do not show unexpanded subcells.
    bool a_phys_no_show_unexpand;
    bool a_elec_no_show_unexpand;

    // If true, show physical properties.
    bool a_phys_props;

    // If true, don't show any highlighting.
    bool a_no_highlighting;

    // If true, in electrical windows, the top cell is never shown
    // symbolically.
    bool a_no_elec_symbolic;

    bool a_show_boxes;
    bool a_show_polys;
    bool a_show_wires;
    bool a_show_labels;

    // Edge snapping modes.
    char a_edge_snapping;
    bool a_edge_off_grid;
    bool a_edge_non_manh;
    bool a_edge_wire_edge;
    bool a_edge_wire_path;
};


// Structures used to save windows in window stack.
//
struct wStackElt
{
    wStackElt() { next = 0; memset(name, 0, sizeof(name)); }
    wStackElt(BBox*, const char*, wStackElt*);

    static void destroy(wStackElt *w)
        {
            while (w) {
                wStackElt *wx = w;
                w = w->next;
                delete wx;
            }
        }

    BBox BB;
    wStackElt *next;
    char name[8];
};

// save this many views in history list
#define WSTACK_HIST 5

// save this many named views
#define WSTACK_VIEWS 5

struct wStack
{
    wStack() { w_views = w_viewhist = 0; }
    ~wStack() { clear(); }
    void clear();
    void add_hist(BBox*);
    int add_view(BBox*);
    void rot_hist_l();
    void rot_hist_r();
    BBox *view_hist();
    BBox *view_view(const char*);
    void numsaved(int*, int*);
    void zero();

    wStackElt *view() { return (w_views); }

private:
    wStackElt *w_views;      // list of named views
    wStackElt *w_viewhist;   // history list of views
};


// Return from WindowDesc::set_current_aoi(), indicates:
//  AOInone    no visible update area
//  AOIpart    partial viewport update
//  AOIfull    full viewport update
//
enum AOItype { AOInone, AOIpart, AOIfull };

// Direction enumeration, for Pan.
//
enum DirectionType
{
    DirNone,
    DirWest,
    DirNorthWest,
    DirNorth,
    DirNorthEast,
    DirEast,
    DirSouthEast,
    DirSouth,
    DirSouthWest
};

#define DSP_CACHE_SIZE 500

// Cache objects for accelerated rendering.
struct sDSPcache
{
    sDSPcache()
        {
            c_boxes = new GRmultiPt(2*DSP_CACHE_SIZE);
            c_sboxes = new GRmultiPt(2*DSP_CACHE_SIZE);
            c_lines = new GRmultiPt(2*DSP_CACHE_SIZE);
            c_pixels = new GRmultiPt(DSP_CACHE_SIZE);
            c_fill = 0;
            c_numsboxes = 0;
            c_numboxes = 0;
            c_numlines = 0;
            c_numpixels = 0;
        }

    ~sDSPcache()
        {
            delete c_boxes;
            delete c_sboxes;
            delete c_lines;
            delete c_fill;
        }

    void add_box(int, int, int, int);
    void add_sbox(int, int, int, int);
    void add_line(int, int, int, int);
    void add_pixel(int, int);

    GRmultiPt *c_boxes;     // box list base
    GRmultiPt *c_sboxes;    // solid box list base (thick edges)
    GRmultiPt *c_lines;     // outline list base
    GRmultiPt *c_pixels;    // pixel list base
    GRfillType *c_fill;     // cached fill pattern
    int c_numboxes;         // size of box cache
    int c_numsboxes;        // size of sbox cache
    int c_numlines;         // size of line cache
    int c_numpixels;        // size of pixel cache
};

// Flags set by FindContact.
#define FC_CX 1
#define FC_CY 2

// Flags used by IsSimilar.
#define WDsimXmode 0x1
#define WDsimXsymb 0x2
#define WDsimXcell 0x4

// Deferred redisplay states.
enum WDaccumMode { WDaccumDone, WDaccumStart, WDaccumAccum };

// Window descriptor.
//
struct WindowDesc
{
    // Mode-specific window dimension backup.
    struct w_win_str
    {
        w_win_str()
            {
                ex = ey = 0;
                ewid = 0;
                px = py = 0;
                pwid = 0;
                eset = pset = false;
            }

        int ex, ey;
        int ewid;
        int px, py;
        int pwid;
        bool eset, pset;
    };

    // State passed to redisplay_layer.
    struct w_rdl_state
    {
        w_rdl_state(const BBox *bb, int exp)
            {
                AOI = bb;
                layer = 0;
                expand = exp;
                map_color = 0;
                is_context = false;
                check_geom = false;
                has_geom = false;
                did_twires = false;
            }

        const BBox *AOI;    // Pointer to display area, must not be
                            // null.

        CDl *layer;         // Layer to display.  If null, paint only
                            // bounding boxes, those too small to expand,
                            // and all on first level if not expanding.

        int expand;         // Expand level.  If -1, always display the
                            // contents of subcells.  If positive
                            // expand up to hierarchy level (if 0, no
                            // expansion).

        int map_color;      // If nonzero, remapping of colors is in force.
                            // This is used to paint NOPHYS devices a
                            // different color.

        bool is_context;    // When true, don't render current cell or
                            // its descendents.

        bool check_geom;    // When set, set has_geom if intersecting
                            // object on layer found.

        bool has_geom;      // Set in redisplay_geom if check_geom set
                            // and intersecting object found on layer.

        bool did_twires;    // We've shown the phony wires when rendering
                            // a tiny schematic in the symbol in Peek mode.
    };

    // dsp_window.cc
    WindowDesc();
    ~WindowDesc();
    void ShowTitleDirect();
    void ShowTitle();
    void SetSubwinMode(DisplayMode);
    void SetHierDisplayMode(const char*, const char*, const BBox*);
    void SetSpecial(WDdbType, const char*, bool, const char* = 0);
    void ClearSpecial();
    void SetSymbol(const CDcbin*);
    bool IsSimilar(const WindowDesc*, unsigned int = 0);
    bool IsSimilar(DisplayMode, const WindowDesc*, unsigned int = 0);
    bool IsSimilarNonSymbolic(const WindowDesc*);
    bool IsShowing(const CDs*);
    CDs *CurCellDesc(DisplayMode, bool = false) const;
    CDs *TopCellDesc(DisplayMode, bool = false) const;
    bool Expand(const char*);
    void ClearExpand();
    void SetID();
    int WinNumber() const;
    bool FindContact(int, int, int*, int*, int*, int, CDo*, bool);
    bool FindBterm(int, int, int*, int*, int*, int, CDo*, bool);
    bool SelectNode(int*, int*, const CDp_snode* = 0);
    void Snap(int*, int*, bool = false);
    double YScale();
    void Display(const CDo*);
    void DisplayIncmplt(const CDo*);
    void DisplaySelected(const CDo*);
    void DisplayUnselected(const CDo*);

    // dsp_box.cc
    void ShowBox(const BBox*, int, const GRfillType*);
    void ShowLineBox(int, int, int, int);
    void ShowLineBox(const BBox*);
    void InitCache();
    void FlushCache();

    // dsp_control.cc
    void Update(const BBox*, bool = false);
    void GhostUpdate(const BBox*);
    void Refresh(const BBox*);
    void RefreshList(const Blist*);
    void Redisplay(const BBox*);
    void RedisplayList(const Blist*);
    void RedisplayDirect(const BBox* = 0, bool = false, int = 0);
    Blist *AddEdges(Blist*, const BBox*);
    void RedisplayHighlighting(const BBox*, bool);
    void RunPending();
    void ClearPending();
    void SwitchToPixmap();
    void SwitchFromPixmap(const BBox*);
    GRobject DrawableReset();
    void CopyPixmap(const BBox*);
    void DestroyPixmap();
    bool DumpWindow(const char*, const BBox*);
    bool PixmapOk();

    // dsp_grid.cc
    int  CheckGrid(const CDo*, bool);
    void ShowGrid();
    void ShowAxes(bool);

    // dsp_image.cc
    GRimage *CreateImage(const BBox*, int* = 0);
    GRimage *CreateChdImage(cCHD*, const char*, const BBox*, int);

    // dsp_label.cc
    void ShowLabel(const Label*);
    void ShowLabel(const char*, int, int, int, int, int, GRvecFont* = 0);
    void ShowLabel(const hyList*, int, int, int, int, int, GRvecFont* = 0);
    void ViewportText(const char*, int, int, double, bool);
    void ShowLabelOutline(int, int, int, int, int);
    void ShowUnexpInstanceLabel(double*, const BBox*, const char*, const char*,
        int);
    static bool LabelHideTest(CDla*);
    static void LabelHideBB(CDla*, BBox*);
    static bool LabelHideHandler(CDol*);

    // dsp_line.cc
    void ShowLine(int, int, int, int);
    void ShowLineW(int, int, int, int);
    void ShowLineV(int, int, int, int);
    void ShowCross(int, int, int, bool);

    // dsp_mark.cc
    int LogScale(int);
    int LogScaleToPix(int);
    void ShowHighlighting();
    void ShowWindowMarks();
    void AddWindowMarksBB(BBox *BB);
    void ShowInstanceOriginMark(bool, const CDc*);
    void ShowObjectCentroidMark(bool, const CDo*);
    bool InstanceOriginMarkBB(const CDc*, BBox*);
    void ShowRulers();
    bool SetProxy(const hyEnt*);
    bool HasProxy() const;
    hyParent *ProxyList() const;
    void UpdateProxy();
    void ClearProxy();

    // dsp_poly.cc
    void ShowPolygon(const Poly*, int, const GRfillType*, const BBox*);
    void fat_seg(const Point*, const Point*, Otype);
    void ShowPath(const Point*, int, bool);

    // dsp_prpty.cc
    void ShowPhysProperties(const BBox*, int);

    // dsp_view.cc
    bool SetView(const char*);
    void SaveViewOnStack();
    void DefaultWindow();
    void CenterFullView(const BBox* = 0);
    const BBox *ContentBB();
    void InitViewport(int, int);
    void InitWindow(int, int, double);
    void InitWindow(const BBox*);
    void Center(int, int);
    void Pan(DirectionType, double);
    void Zoom(double);
    void ClearViews();

    // dsp_wire.cc
    void ShowWire(const Wire*, int, const GRfillType*);
    void ShowLinePath(const Point*, int);

    void EnableCache()          { if (!DSP()->NoDisplayCache() && w_cache)
                                  w_usecache = true; }
    void DisableCache()         { w_usecache = false; }

    WDdbType DbType() const     { return (w_dbtype); }
    const char *DbName() const  { return (w_dbname); }
    const char *DbCellName() const { return (w_dbcellname); }

    // Lambda (window) to pixel (screen) for x
    void LToPx(int x, int &xp) const
        { xp = mmRnd((x - w_window.left)*w_ratio); }
    void LToPx(int x, short &xp) const
        { xp = mmRnd((x - w_window.left)*w_ratio); }

    // Lambda (window) to pixel (screen) for y
    void LToPy(int y, int &yp) const
        { yp = mmRnd((w_window.top - y)*w_ratio); }
    void LToPy(int y, short &yp) const
        { yp = mmRnd((w_window.top - y)*w_ratio); }

    // Lambda (window) to pixel (screen) for x,y
    void LToP(int x, int y, int &xp, int &yp) const
        { LToPx(x, xp); LToPy(y, yp); }
    void LToP(int x, int y, short &xp, short &yp) const
        { LToPx(x, xp); LToPy(y, yp); }

    // Lambda (window) to pixel (screen) for BB
    void LToPbb(const BBox &BB, BBox &BBp) const
        { LToP(BB.left, BB.bottom, BBp.left, BBp.bottom);
            LToP(BB.right, BB.top, BBp.right, BBp.top); }

    // Pixel (screen) to lambda (window) for x
    void PToLx(int xp, int &x) const
        { x = mmRnd(xp/w_ratio) + w_window.left; }

    // Pixel (screen) to lambda (window) for y
    void PToLy(int yp, int &y) const
        { y = mmRnd(-yp/w_ratio) + w_window.top; }

    // Pixel (screen) to lambda (window) for x,y
    void PToL(int xp, int yp, int &x, int &y) const
        { PToLx(xp, x); PToLy(yp, y); }

    // Pixel (screen) to lambda (window) for BB
    void PToLbb(const BBox &BBp, BBox &BB) const
        { PToL(BBp.left, BBp.bottom, BB.left, BB.bottom);
            PToL(BBp.right, BBp.top, BB.right, BB.top); }

    // Ghost drawing finished, update the highlighting.
    void GhostFinalUpdate()
        {
            if (w_accum_mode == WDaccumAccum) {
                w_accum_mode = WDaccumDone;
                Update(&w_accum_rect);
            }
        }

    BBox *Window()                          { return (&w_window); }
    BBox *ClipRect()                        { return (&w_clip_rect); }
    void SetClipRect(const BBox &bb)        { w_clip_rect = bb; }
    const BBox &Viewport() const            { return (w_viewport); }
    int ViewportWidth() const               { return (w_width); }
    int ViewportHeight() const              { return (w_height); }
    double MinWidth()                       { return (w_width/20.0); }

    double Ratio() const                    { return (w_ratio); }
    void SetRatio(double r)                 { w_ratio = r; }
    double Aspect() const           { return (((double)w_width)/w_height); }

    uintptr_t WindowId() const              { return (w_windowid); }

    unsigned int DisplFlags() const         { return (w_displflag); }
    void SetDisplFlags(unsigned int f)      { w_displflag = f; }

    DisplayMode Mode() const                { return (w_mode); }
    void SetMode(DisplayMode m)             { w_mode = m; }

    DSPwbag *Wbag() const                   { return (w_wbag); }
    void SetWbag(DSPwbag *w)                { w_wbag = w; }
    GRdraw *Wdraw() const                   { return (w_draw); }
    void SetWdraw(GRdraw *w)                { w_draw = w; }

    DSPattrib *Attrib()                     { return (&w_attributes); }
    const DSPattrib *AttribC() const        { return (&w_attributes); }
    wStack *Views()                         { return (&w_views); }

    bool IsFrozen() const                   { return (w_frozen); }
    void SetFrozen(bool b)                  { w_frozen = b; }
    bool IsShowLoc() const                  { return (w_show_loc); }
    void SetShowLoc(bool b)                 { w_show_loc = b; }
    bool IsMainLoc() const                  { return (w_main_loc); }
    void SetMainLoc(bool b)                 { w_main_loc = b; }

    // The name pointers must point to string table entries.
    CDcellName CurCellName() const          { return (w_cur_cellname); }
    void SetCurCellName(CDcellName n)       { w_cur_cellname = n; }
    CDcellName TopCellName() const          { return (w_top_cellname); }
    void SetTopCellName(CDcellName n)       { w_top_cellname = n; }

    double XSectYScale()                    { return (w_xsect_yscale); }
    void SetXSectYScale(double d)           { w_xsect_yscale = d; }
    bool IsXSect()                          { return (w_is_xsect); }
    void SetXSect(bool b)                   { w_is_xsect = b; }
    bool IsXSectAutoY()                     { return (w_xsect_auto_y); }
    void SetXSectAutoY(bool b)              { w_xsect_auto_y = b; }

    w_win_str *WinStr()                     { return (&w_win); }

    void
    SetContents(const WindowDesc *wd)
    {
        w_cur_cellname = wd->w_cur_cellname;
        w_top_cellname = wd->w_top_cellname;
        w_mode = wd->w_mode;
        w_displflag = wd->w_displflag;
        w_dbname = lstring::copy(wd->w_dbname);
        w_dbcellname = lstring::copy(wd->w_dbcellname);
        w_dbtype = wd->w_dbtype;
        w_dbfree = false;
    }

    void PushRedisplay()            { w_redisplay++; }
    void PopRedisplay()             { if (w_redisplay) w_redisplay--; }
    int InRedisplay()         const { return (w_redisplay); }
    bool NeedRedisplay()      const { return (w_need_redisplay); }
    void SetNeedRedisplay(bool b)   { w_need_redisplay = b; }

    void SetAccumMode(WDaccumMode m) { w_accum_mode = m; }

private:
    // dsp_box.cc
    void cache_box(int, int, int, int);
    void cache_solid_box(int, int, int, int);
    void cache_line(int, int, int, int);

    // dsp_control.cc
    void add_redisp_region(const BBox*);
    void add_update_region(const BBox*);
    bool show_cellbb(bool, const BBox*, bool, Blist** = 0);
    AOItype set_current_aoi(BBox*);

    // dsp_image.cc
    int compose_chd_image(cCHD*, const char*, const BBox*, int);
    bool show_boundaries(symref_t*, bnd_draw_t*, int);
    int redisplay_cddb_zimg(const BBox*);
    int redisplay_cddb_zimg_rc(CDs*, int, w_rdl_state*, bool);

    // dsp_label.cc
    void show_label(const void*, int, int, int, int, int, bool,
        const GRvecFont*);
    void show_inst_text(int, const char*, const char*, const BBox*, int,
        int, int, bool, bool);

    // dsp_render.cc
    int redisplay_geom(const BBox*, bool = false);
    int redisplay_geom_direct(const BBox*);
    int redisplay_cddb(const BBox*);
    int redisplay_chd(const BBox*);
    int redisplay_blist(const BBox*);
    int redisplay_sdb(const BBox*);
    int redisplay_layer(CDs*, w_rdl_state*);
    int redisplay_layer_rc(CDs*, int, w_rdl_state*, bool);
    void show_unexpanded_instance(const CDc*);
    static bool syscale(const CDs*);

    BBox w_window;              // 0,0 in lower left
    BBox w_clip_rect;           // viewport coords
    int w_width;                // viewport width;
    int w_height;               // viewport height;
    double w_ratio;             // viewport width / window width
    DSPwbag *w_wbag;            // widget bag
    GRdraw *w_draw;             // drawing context
    CDcellName w_cur_cellname;  // name of current symbol, string tab pointer
    CDcellName w_top_cellname;  // name of current symbol, string tab pointer
    wStack w_views;             // view manager

    hyEnt *w_proxy;             // proxy link for hypertext
    Blist *w_pending_R;         // redisplay areas pending
    Blist *w_pending_U;         // update areas pending
    BBox w_last_cellbb;         // current cell bounding box

    sDSPcache *w_cache;         // the cache
    const GRfillType *w_fill;   // cached fill pattern

    double w_xsect_yscale;      // cross-section display Y scale
    bool w_is_xsect;            // true if window showing cross section
    bool w_xsect_auto_y;        // true if auto-Y scale in cross section

    bool w_outline;             // for cache, textured outline if no fill

    bool w_dbfree;              // true to free database in destructor
    WDdbType w_dbtype;          // source database type
    char *w_dbname;             // name of database for display
    char *w_dbcellname;         // cellname for database display

    RGBzimg *w_rgbimg;          // local image

    DisplayMode w_mode;         // Physical or Electrical
    unsigned int w_displflag;   // cell expansion flag

    int w_redisplay;            // redisplay call count

    bool w_usecache;            // use cache if set
    bool w_frozen;              // don't draw geometry when set
    bool w_show_loc;            // show location in main window
    bool w_main_loc;            // show main location is this

    bool w_need_redisplay;      // flag, full redisplay needed
    bool w_using_pixmap;        // using backing store
    bool w_using_image;         // using in-core image
    bool w_old_image;           // using old image composition logic

    w_win_str w_win;            // window view alternate mode store
    uintptr_t w_windowid;       // window identifier

    WDaccumMode w_accum_mode;   // deferred refresh state
    BBox w_accum_rect;          // deferred refresh area
    BBox w_viewport;            // viewport BBox for convenience
    DSPattrib w_attributes;     // window attributes
};

// Generator for cycling through the windows.
//
struct WDgen
{
    enum WDst { MAIN=0, SUBW=1 };
    // MAIN   cycle through all windows
    // SUBW   cycle through subwindows only

    enum WDmd { CDDB, CHD, ALL };
    // CDDB   return WDcddb windows only
    // CHD    return WDchd and WDcddb windows only
    // ALL    return all windows

    WDgen(WDst start, WDmd mode) { g_cnt = (int)start; g_mode = mode; }

    WindowDesc *next()
        {
            for ( ; g_cnt < DSP_NUMWINS; g_cnt++) {
                if (!DSP()->Window(g_cnt))
                    continue;
                if (DSP()->Window(g_cnt)->DbType() == WDcddb ||
                        g_mode == ALL ||
                        (DSP()->Window(g_cnt)->DbType() == WDchd &&
                            g_mode == CHD)) {
                    g_cnt++;
                    return (DSP()->Window(g_cnt - 1));
                }
            }
            return (0);
        }

private:
    int g_cnt;
    WDmd g_mode;
};

#endif


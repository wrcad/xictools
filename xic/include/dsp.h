
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

#ifndef DSP_H
#define DSP_H

#include <stdio.h>
#include <ctype.h>
#include "ginterf/graphics.h"
#include "cd.h"
#include "cd_types.h"
#include "dsp_if.h"


//
// Main include file for display module.
//

// Attributes
#define VA_BoxLineStyle         "BoxLinestyle"
#define VA_GridNoCoarseOnly     "GridNoCoarseOnly"
#define VA_GridThreshold        "GridTrheshold"
#define VA_NoInstnameLabels     "NoInstnameLabels"

// In a sub-edit (Push command), the context colors are darkened
// to the percentage below.
#define DSP_DEF_CX_DARK_PCNT    65
#define DSP_MIN_CX_DARK_PCNT    30

// This is really a program-wide parameter, the number of helper
// threads to use for various purposes (VA_Threads in main_variables).
#define DSP_DEF_THREADS         0
#define DSP_MIN_THREADS         0
#define DSP_MAX_THREADS         31

// If the smallest side of an instance bounding box shown in a window
// is less than this value in pixels, don't show fence (grip) marks.
#define DSP_MIN_FENCE_INST_PIXELS   100

// The grip marks that are not shown as fence lines are diamond shapes
// extending this many pixels from the center.
#define DSP_GRIP_MARK_PIXELS    4

// Default dashed line style for unfilled boxes.
#define DEF_BoxLineStyle        0xe38

// Default screen pixel slop value for selecting.
#define DEF_PixelDelta          4.0

// Default mouse wheel pan and zoom factors.
#define DEF_MW_PAN_FCT          0.1
#define DEF_MW_ZOOM_FCT         0.1

// Default maximum string length of labels to display as text.  Labels
// longer than this are displayed as a small box instead.  This is set
// high to effectively disable this feature by default, since it tends
// to confuse users.  Users can alternatively use long text labels.
#define DSP_DEF_MAX_LABEL_LEN 256
#define DSP_MAX_MAX_LABEL_LEN 1000
#define DSP_MIN_MAX_LABEL_LEN 6

// If labels contain multiple lines, the number of lines shown may be
// limited.  This is the default limit.
#define DSP_DEF_MAX_LABEL_LINES 5
#define DSP_MAX_MAX_LABEL_LINES 20
#define DSP_MIN_MAX_LABEL_LINES 0

// Pixel threshold for grid display.
#define DSP_DEF_GRID_THRESHOLD 8
#define DSP_MAX_GRID_THRESHOLD 40
#define DSP_MIN_GRID_THRESHOLD 4

// Default min pixel size for displayed subcell.
#define DSP_DEF_CELL_THRESHOLD 4
#define DSP_MAX_CELL_THRESHOLD 100
#define DSP_MIN_CELL_THRESHOLD 0

// Default sleep time for slow-mode drawing (peek command).
#define DSP_SLEEP_TIME_MS 400

// Default pixel height of text labels.
#define DSP_DEF_PTRM_TXTHT 14
#define DSP_MAX_PTRM_TXTHT 6
#define DSP_MIN_PTRM_TXTHT 48

// Default pixel size of physical terminal cross, from center.
#define DSP_DEF_PTRM_DELTA 10
#define DSP_MAX_PTRM_DELTA 6
#define DSP_MIN_PTRM_DELTA 48

// various marks
// basic mark types
#define MARK_CROSS 0
#define MARK_BOX   1
#define MARK_ARROW 2
#define MARK_ETERM 3
#define MARK_STERM 4
#define MARK_PTERM 5
#define MARK_ILAB  6
#define MARK_BSC   7
#define MARK_SYBSC 8
#define MARK_FENCE 9
#define MARK_OBASE 10 // must be last!

// Types of user marks, passed to cDisplay::AddUserMark().
enum hlType { hlNone, hlLine, hlBox, hlVtriang, hlHtriang, hlCircle,
    hlEllipse, hlPoly, hlText };

// Erase behind type.
enum ErbhType { ErbhNone, ErbhSome, ErbhAll };

// Hidden label mode.  Allow hidden labels { everywhere, only
// electrical, only electrical bound labels, nowhere }.
enum HLmode { HLall, HLel, HLelprp, HLnone };

// Label orientation control.
enum { SLnone, SLupright, SLtrueOrient };
typedef unsigned char SLtype;

struct DSPattrib;
struct DSPwbag;
struct sColorTab;
struct hyParent;

struct sRuler
{
    sRuler(int wn, int x1, int y1, int x2, int y2, bool m, double l,
        sRuler *nx) : p1(x1, y1), p2(x2, y2)
        { win_num = wn; mirror = m; loff = 0.0; loff = l; next = nx; }

    static void destroy(sRuler *r)
        {
            while (r) {
                sRuler *rx = r;
                r = r->next;
                delete rx;
            }
        }

    void show(bool, bool = false);
    bool bbox(BBox&, WindowDesc*);

    Point_c p1, p2;
    int win_num;     // window number of ruler (indep. of graphics context)
    bool mirror;     // orient grads reverse of normal
    double loff;     // offset for chained ruler
    sRuler *next;
};

// WindowDesc source database type
//  WDcddb     Cells from CD (normal mode)
//  WDchd      Context struct display
//  WDblist    Box list display
//  WDsdb      cSDB database display
//
enum WDdbType { WDcddb, WDchd, WDblist, WDsdb };

// Interrupt types.
enum DSPinterType { DSPinterNone, DSPinterUser, DSPinterSys };

// Number of drawing windows, the 0 index is the main drawing window.
#define DSP_NUMWINS 5

inline class cDisplay *DSP();

// NOTE:  The cDisplay class has its own transform stack.  ALL
// functions called in files in the display module should use this
// stack, and should NOT use the CD stack.  This should facilitate use
// of a separate display thread.
//
// An exception is the dsp_image code, which uses FIO.

class cDisplay : public cDisplayIf, public cTfmStack
{
    static cDisplay *ptr()
        {
            if (!instancePtr)
                on_null_ptr();
            return (instancePtr);
        }

    static void on_null_ptr();

public:
    friend inline cDisplay *DSP() { return (cDisplay::ptr()); }

    // dsp.cc
    cDisplay();
    void Initialize(int, int, int, bool = false);
    int OpenSubwin(const BBox*, WDdbType = WDcddb, const char* = 0,
        bool = false);
    int OpenSubwin(const CDs*, const hyEnt* = 0, bool = false);
    WindowDesc *Windesc(unsigned long);
    void RedisplayArea(const BBox*, int = -1);
    void RedisplayAll(int = -1);
    void RedisplayAfterInterrupt();
    void QueueRedisplay();
    static int RedisplayIdleProc(void*);

    // dsp_image.cc
    GRimage *CreateImage(cCHD*, const char*, BBox*, unsigned int,
        unsigned int, int = -1);

    // dsp_label.cc
    int DefaultLabelSize(const char*, DisplayMode, int*, int*);
    int DefaultLabelSize(hyList*, DisplayMode, int*, int*);
    int LabelExtent(const char*, int*, int*);
    int LabelExtent(hyList*, int*, int*);
    int LabelSize(const char*, DisplayMode, int*, int*);
    int LabelSize(hyList*, DisplayMode, int*, int*);
    void LabelResize(const char*, const char*, int*, int*);
    void LabelResize(hyList*, hyList*, int*, int*);
    void LabelSetTransform(int, SLtype, int*, int*, int*, int*);
    void LabelHyUpdate();

    // dsp_mark.cc
    void ShowCurrentObject(bool, CDo*, int = -1);
    void ShowCells(const char*);
    void ShowNode(bool, int);
    void ShowTerminals(bool);
    void ShowCellTerminalMarks(bool);
    void ShowInstTerminalMarks(bool, CDc*, int);
    void ShowPhysTermList(bool, CDpin*);
    void ShowPhysTermList(bool, CDcont*);
    void HlitePhysTermList(bool, CDpin*);
    void HlitePhysTermList(bool, CDcont*);
    void HlitePhysTerm(bool, const CDterm*);
    void HliteElecTerm(bool, const CDp_node*, const CDc*, int);
    void HliteElecBsc(bool, const CDp_bsnode*);
    void ClearTerminal(CDterm*);
    void ShowInstanceLabel(bool, CDc*, int);
    void ShowPlotMark(bool, hyEnt*, int, bool);
    void SetPlotMarkColor(int, int);
    void ShowCrossMark(bool, int, int, int, int, DisplayMode, int = 0,
        const char* = 0, const char* = 0);
    void EraseCrossMarks(DisplayMode, int);
    void ShowBoxMark(bool, int, int, int, int, DisplayMode);
    void ShowEtermMark(bool, int, int, int, int, const CDp_nodeEx*, int);
    void ShowStermMark(bool, int, int, int, int, const CDp_nodeEx*, int);
    void ShowBscMark(bool, int, int, int, int, const CDp_bcnode*, int);
    void ShowSyBscMark(bool, int, int, int, int, const CDp_bcnode*, int);
    void ShowFenceMark(bool, const CDc*, int, int, int, int, int, int);
    void ShowObaseMark(bool, const CDs*, const hyParent*, int, int, int,
        int, int);
    void EraseMarks(int);
    void ClearWindowMarks();
    int AddUserMark(int, ...);
    bool RemoveUserMark(int);
    bool RemoveUserMarkAt(const BBox*);
    void ClearUserMarks(const CDs*);
    int DumpUserMarks(const char*, const CDs*);
    int ReadUserMarks(const char*);

    // dsp_prpty.cc
    void ShowOdescPhysProperties(CDo*, int);

    // dsp_ruler.cc
    void RulerSetSnapDefaults(const DSPattrib*, const bool*);
    void RulerGetSnapDefaults(DSPattrib*, bool*, bool);
    void StartRulerCmd();
    void EndRulerCmd();

    // dsp_terminal.cc
    bool FindTerminal(const char*, CDc**, int*, CDp_node**);
    void ShowTerminal(CDc*, int, CDp_node*);

    // dsp_view.cc
    void ClearViews();

    // Current display mode
    inline DisplayMode CurMode();
    inline void SetCurMode(DisplayMode);

    // Current symbol
    inline CDcellName CurCellName();
    inline CDcellName TopCellName();
    inline void SetCurCellName(CDcellName);
    inline void SetTopCellName(CDcellName);

    // Main window
    inline DSPwbag *MainWbag();
    inline GRdraw *MainDraw();
    WindowDesc *MainWdesc() { return (d_windows[0]); }

    // Color pixel
    inline int Color(int, DisplayMode);
    inline int Color(int);
    sColorTab *ColorTab() { return (d_color_table); }

    // If a physical master is saved in the invisible_master_tab,
    // it won't appear in the display.

    void SetInvisible(CDm *mdesc)
        {
            if (!mdesc)
                return;
            if (!d_invisible_master_tab)
                d_invisible_master_tab = new SymTab(false, false);
            d_invisible_master_tab->add((unsigned long)mdesc, 0, true);
        }

    void ClearInvisible(CDm *mdesc)
        {
            if (!d_invisible_master_tab)
                return;
            if (!mdesc) {
                delete d_invisible_master_tab;
                d_invisible_master_tab = 0;
            }
            else
                d_invisible_master_tab->remove((unsigned long)mdesc);
        }

    bool IsInvisible(CDm *mdesc)
        {
            return (d_invisible_master_tab && mdesc &&
                SymTab::get(d_invisible_master_tab, (unsigned long)mdesc) !=
                ST_NIL);
        }

    WindowDesc *Window(int i)
        {
            if (i >= 0 && i < DSP_NUMWINS)
                return (d_windows[i]);
            return (0);
        }
    void SetWindow(int i, WindowDesc *w)
        {
            if (i >= 0 && i < DSP_NUMWINS)
                d_windows[i] = w;
        }

    HLmode HiddenLabelMode()                { return (d_hidden_label_mode); }
    void SetHiddenLabelMode(HLmode m)       { d_hidden_label_mode = m; }

    // Extra space around "center full view."
    int CFVbloat()                          { return (d_cfv_bloat); }
    void SetCFVbloat(int b)                 { d_cfv_bloat = b; }

    CDo *IncompleteObject()                 { return (d_incomplete_object); }
    void SetIncompleteObject(CDo *o)        { d_incomplete_object = o; }
    sRuler *Rulers()                        { return (d_rulers); }
    void SetRulers(sRuler *r)               { d_rulers = r; }
    GRlineType *BoxLinestyle()              { return (&d_box_linestyle); }

    int NumThreads()                        { return (d_threads); }
    void SetNumThreads(int t)               { d_threads = t; }
    int CellThreshold()                     { return (d_cell_threshold); }
    void SetCellThreshold(int t)            { d_cell_threshold = t; }
    int GridThreshold()                     { return (d_grid_threshold); }
    void SetGridThreshold(int t)            { d_grid_threshold = t; }
    int MaxLabelLen()                       { return (d_max_label_len); }
    void SetMaxLabelLen(int l)              { d_max_label_len = l; }
    int MaxLabelLines()                     { return (d_max_label_lines); }
    void SetMaxLabelLines(int l)            { d_max_label_lines = l; }
    int SleepTimeMs()                       { return (d_sleep_time_ms); }
    void SetSleepTimeMs(int t)              { d_sleep_time_ms = t; }
    int SelectPixel()                       { return (d_select_pixel); }
    void SetSelectPixel(int p)              { d_select_pixel = p; }
    int PhysPropSize()                      { return (d_phys_prop_size); }
    void SetPhysPropSize(int s)             { d_phys_prop_size = s; }
    int TermTextSize()                      { return (d_term_text_size); }
    void SetTermTextSize(int s)             { d_term_text_size = s; }
    int TermMarkSize()                      { return (d_term_mark_size); }
    void SetTermMarkSize(int s)             { d_term_mark_size = s; }

    double PhysCharWidth()                  { return (d_phys_char_width); }
    void SetPhysCharWidth(double w)         { d_phys_char_width = w; }
    double PhysCharHeight()                 { return (d_phys_char_height); }
    void SetPhysCharHeight(double h)        { d_phys_char_height = h; }
    double ElecCharWidth()                  { return (d_elec_char_width); }
    void SetElecCharWidth(double w)         { d_elec_char_width = w; }
    double ElecCharHeight()                 { return (d_elec_char_height); }
    void SetElecCharHeight(double h)        { d_elec_char_height = h; }

    double MouseWheelPanFactor()            { return (d_mw_pan_factor); }
    void SetMouseWheelPanFactor(double d)   { d_mw_pan_factor = d; }
    double MouseWheelZoomFactor()           { return (d_mw_zoom_factor); }
    void SetMouseWheelZoomFactor(double d)  { d_mw_zoom_factor = d; }

    double PixelDelta()                     { return (d_pixel_delta); }
    void SetPixelDelta(double d)            { d_pixel_delta = d; }

    DSPinterType Interrupt()                { return (d_interrupt); }
    void SetInterrupt(DSPinterType t)       { d_interrupt = t; }

    ErbhType EraseBehindTerms()             { return (d_erase_behind_terms); }
    void SetEraseBehindTerms(ErbhType e)    { d_erase_behind_terms = e; }
    Point_c *PhysVisGridOrigin()          { return (&d_phys_vis_grid_origin); }

    bool InEdgeSnappingCmd()                { return (d_in_edge_snap_cmd); }
    void SetInEdgeSnappingCmd(bool b)       { d_in_edge_snap_cmd = b; }
    bool NoGridSnapping()                   { return (d_no_grid_snap); }
    void SetNoGridSnapping(bool b)          { d_no_grid_snap = b; }
    bool GridNoCoarseOnly()                 { return (d_no_coarse_only); }
    void SetGridNoCoarseOnly(bool b)        { d_no_coarse_only = b; }
    bool ShowCndrNumbers()                  { return (d_show_cnums); }
    void SetShowCndrNumbers(bool b)         { d_show_cnums = b; }
    bool ShowInvisMarks()                   { return (d_show_invis_marks); }
    void SetShowInvisMarks(bool b)          { d_show_invis_marks = b; }
    bool NoGraphics()                       { return (d_no_graphics); }
    void SetNoGraphics(bool b)              { d_no_graphics = b; }
    bool NoRedisplay()                      { return (d_no_redisplay); }
    void SetNoRedisplay(bool b)             { d_no_redisplay = b; }
    bool SlowMode()                         { return (d_slow_mode); }
    void SetSlowMode(bool b)                { d_slow_mode = b; }

    bool DoingHcopy()                       { return (d_doing_hcopy); }
    void SetDoingHcopy(bool b)              { d_doing_hcopy = b; }
    bool NoPixmapStore()                    { return (d_no_pixmap_store); }
    void SetNoPixmapStore(bool b)           { d_no_pixmap_store = b; }
    bool NoLocalImage()                     { return (d_no_local_image); }
    void SetNoLocalImage(bool b)            { d_no_local_image = b; }
    bool NoDisplayCache()                   { return (d_no_display_cache); }
    void SetNoDisplayCache(bool b)          { d_no_display_cache = b; }
    bool UseDriverLabels()                  { return (d_use_driver_labels); }
    void SetUseDriverLabels(bool b)         { d_use_driver_labels = b; }
    bool NumberVertices()                   { return (d_number_vertices); }
    void SetNumberVertices(bool b)          { d_number_vertices = b; }
    bool ShowTerminals()                    { return (d_show_terminals); }
    void SetShowTerminals(bool b)           { d_show_terminals = b; }
    bool TerminalsVisible()                 { return (d_terminals_visible); }
    void SetTerminalsVisible(bool b)        { d_terminals_visible = b; }

    bool ContactsVisible()                  { return (d_contacts_visible); }
    void SetContactsVisible(bool b)         { d_contacts_visible = b; }
    bool EraseBehindProps()                 { return (d_erase_behind_props); }
    void SetEraseBehindProps(bool b)        { d_erase_behind_props = b; }
    bool ShowInstanceOriginMark()           { return (d_show_instance_mark); }
    void SetShowInstanceOriginMark(bool b)  { d_show_instance_mark = b; }
    bool ShowObjectCentroidMark()           { return (d_show_centroid_mark); }
    void SetShowObjectCentroidMark(bool b)  { d_show_centroid_mark = b; }
    bool NoInstnameLabels()                 { return (d_no_instname_labels); }
    void SetNoInstnameLabels(bool b)        { d_no_instname_labels = b; }

    int ShowingNode()                       { return (d_showing_node); }

    int InternalPixel()                     { return (d_internal_pixel); }
    void SetInternalPixel(int p)            { d_internal_pixel = p; }
    int AllocCount()                        { return (d_alloc_count); }
    void SetAllocCount(int c)               { d_alloc_count = c; }
    int ContextDarkPcnt()                   { return (d_context_dark_pcnt); }
    void SetContextDarkPcnt(int p)
        {
            if (p >= 0 && p <= 100)
                d_context_dark_pcnt = p;
            else
                d_context_dark_pcnt = DSP_DEF_CX_DARK_PCNT;
        }

    int FenceInstPixSize()                  { return (d_fence_inst_pixsz); }
    void SetFenceInstPixSize(int s)         { d_fence_inst_pixsz = s; }

    sColorTab *ColorTable()                 { return (d_color_table); }
    SymTab *InvisibleMasterTab()            { return (d_invisible_master_tab); }
    int MinCellWidth()                      { return (d_min_cell_width); }
    void SetMinCellWidth(int w)             { d_min_cell_width = w; }
    int EmptyCellWidth()                    { return (d_empty_cell_width); }
    void SetEmptyCellWidth(int w)           { d_empty_cell_width = w; }
    bool TransformOverflow()                { return (d_transform_overflow); }
    void SetTransformOverflow(bool b)       { d_transform_overflow = b; }

private:
    // dsp_color.cc
    void createColorTable();

    // dsp_ifsetup.cc
    void setupInterface();

    WindowDesc *d_windows[DSP_NUMWINS];  // Drawing windows.

    CDo *d_incomplete_object;   // Object currently under construction.
    sRuler *d_rulers;           // List of rulers to show.
    GRlineType d_box_linestyle; // Line style for boxes in electrical mode.

    int d_threads;              // Number of helper threads in use.
    int d_cell_threshold;       // Min pixel size for displayed subcell.
    int d_grid_threshold;       // Min pixel spacing for displayed grid.
    int d_max_label_len;        // Max length of displayed property label.
    int d_max_label_lines;      // Max lines shown in property labels.
    int d_sleep_time_ms;        // Delay time msec for slow mode.
    int d_select_pixel;         // Color index for select highlighting.
    int d_phys_prop_size;       // Size of text used for physical properties.
    int d_term_text_size;       // Size of text used for terminals.
    int d_term_mark_size;       // Size of mark used for terminals.

    double d_phys_char_width;   // Default size of char cell for labels,
    double d_phys_char_height;  // in microns.
    double d_elec_char_width;   // Default size of char cell for labels,
    double d_elec_char_height;  // in microns.

    double d_mw_pan_factor;     // Mouse wheel pan factor.
    double d_mw_zoom_factor;    // Mouse wheel zoom factor.

    double d_pixel_delta;       // Screen pixel delta for selection.

    DSPinterType d_interrupt;    // Interrupt received, halt drawing.

    ErbhType d_erase_behind_terms;  // Erase box around terminals.
    Point_c d_phys_vis_grid_origin; // Origin for *displayed* physical grid.

    bool d_in_edge_snap_cmd;    // Set in commands that use edge snapping.
    bool d_no_grid_snap;        // Inhibit snapping to grid.
    bool d_no_coarse_only;      // Don't show coarse grid without fine
    bool d_show_cnums;          // Show conductor numbers.
    bool d_show_invis_marks;    // Show invisible marks.
    bool d_no_graphics;         // No graphics support.
    bool d_no_redisplay;        // Suppress redraw.
    bool d_slow_mode;           // Slow redraw enable.

    bool d_doing_hcopy;         // In hard copy mode.
    bool d_no_pixmap_store;     // Don't use backing pixmap.
    bool d_no_local_image;      // Don't use local image.
    bool d_no_display_cache;    // Don't use display cache.
    bool d_use_driver_labels;   // Use hardcopy driver text for labels.
    bool d_number_vertices;     // Display polygon vertex numbers
    bool d_show_terminals;      // Show terminal points.
    bool d_terminals_visible;   // Terminals are visible.

    bool d_contacts_visible;    // Contacts are visible.
    bool d_erase_behind_props;  // Erase behind prysical property text.
    bool d_show_instance_mark;  // Show instance origin marks when selected.
    bool d_show_centroid_mark;  // Show object centroid marks when selected.
    bool d_no_instname_labels;  // Use master name not instance name on screen.
    bool d_redisplay_queued;    // Internal flag.
    bool d_transform_overflow;  // Transform stack overflow.
    bool d_initialized;         // Initialize() called.

    HLmode d_hidden_label_mode; // Scope allowed for hidden labels.
    int d_cfv_bloat;            // Space around "center full view".
    int d_showing_node;         // Node showing from ShowNode.
    int d_internal_pixel;       // Common default internal layer pixel.
    int d_alloc_count;          // Default color assignment counter.
    int d_context_dark_pcnt;    // Intensity percentage of subedit context.
    int d_fence_inst_pixsz;     // Pixel size threshold for inst. fence marks.

    sColorTab *d_color_table;   // Rendering colors
    SymTab *d_invisible_master_tab;
    int d_min_cell_width;       // Size threshold for subcell display.
    int d_empty_cell_width;     // Effective size for empty cell.

    static cDisplay *instancePtr;
};

#endif


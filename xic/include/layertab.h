
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

#ifndef LAYERTAB_H
#define LAYERTAB_H


class cLtab;
struct CDldbState;

// For drag/drop of fillpatterns and layers, passed to FillLoadCallback.
//
struct LayerFillData
{
    LayerFillData(const CDl* = 0);

    short d_nx, d_ny;           // pattern dimensions
    int d_layernum;             // layer table index or fp edit store index
    bool d_from_layer;          // true if from layer table/palette
    bool d_from_sample;         // true if from fill editor sample

#define LFD_OUTLINE     0x1
#define LFD_FAT         0x2
#define LFD_CUT         0x4
    unsigned char d_flags;      // attribute flags
    unsigned char d_foo;        // unused, for alignment

    unsigned char d_data[128];  // pattern data, up to 32x32
};

// A subclass is returned from PopUpLayerEditor, provides access to
// widget control functions.
//
struct sLcb
{
    virtual ~sLcb() { }

    void quit_cb();
    void add_cb(bool);
    void rem_cb(bool);
    static void check_update(CDll*, DisplayMode);

    // These are implemented in the widget code.
    virtual void update(CDll*) = 0;
    virtual char *layername() = 0;
    virtual void desel_rem() = 0;
    virtual void popdown() = 0;
};


// Argument to SetLayerVisibility, etc.
enum LTstate { LToff, LTon, LTtoggle };

// The controller for the layer table display.
//
class cLtab : virtual public GRdraw
{
public:
    cLtab();

    void init();
    void show_direct(const CDl* = 0);
    void set_layer_visibility(LTstate, int, bool);
    void set_layer_selectability(LTstate, int);
    void b1_handler(int, int, int, bool);
    void b2_handler(int, int, int, bool);
    void b3_handler(int, int, int, bool);
    void scroll_handler(bool);
    bool drag_check(int, int);
    void box(int, const CDl*);
    void indicators(int, const CDl*);
    void text(int, const CDl*);
    void outline(int);
    void more();
    int entry_of_xy(int, int);
    void entry_to_xy(int, int*, int*);
    void entry_size(int*, int*);
    void define_color(int*, int, int, int);
    int last_entry();

    void set_no_graphics()          { lt_disabled = true; }
    int vis_entries()               { return (lt_vis_entries); }
    int first_visible()             { return (lt_first_visible); }
    void set_first_visible(int v)   { lt_first_visible = v; }
    void set_no_phys_redraw(bool b) { lt_no_phys_redraw = b; }
    bool no_phys_redraw()           { return (lt_no_phys_redraw); }

    // toolkit-specific functions

    virtual void setup_drawable() = 0;
        // Initialize for drawing.

    virtual void blink(CDl*) = 0;
        // Handle blinking layers.

    virtual void show(const CDl* = 0) = 0;
        // Handle redraw, using backing store.

    virtual void refresh(int, int, int, int) = 0;
        // Refresh the area passed from backing.

    virtual void win_size(int*, int*) = 0;
        // Return the drawing area size.

    virtual void update() = 0;
        // Update the display.

    virtual void update_scrollbar() = 0;
        // Update the scroll bar.

    virtual void hide_layer_table(bool) = 0;
        // Hide or show the layer table.

    virtual void set_layer() = 0;
        // Current layer has changed.

protected:
    int lt_win_width;             // drawing window width
    int lt_win_height;            // drawing window height
    int lt_first_visible;         // index of first visible entry
    int lt_vis_entries;           // number of entries shown
    int lt_x_margin;              // left of layer box
    int lt_spa;                   // space round layer box
    int lt_y_text_fudge;          // fine y text positioning
    int lt_box_dimension;         // size of sample box
    int lt_backg;                 // drawing win background color
    int lt_drag_x;                // drag press location
    int lt_drag_y;                // drag press location
    bool lt_dragging;             // true initiates drag/drop
    bool lt_disabled;             // true disables all actions
    bool lt_no_phys_redraw;       // true disables redraw in phys mode
};

inline class cLayerTab *LT();

// Main container for layer info and misc. functions.
//
class cLayerTab
{
    static cLayerTab *ptr()
        {
            if (!instancePtr)
                on_null_ptr();
            return (instancePtr);
        }

    static void on_null_ptr();

public:
    friend inline cLayerTab *LT() { return (cLayerTab::ptr()); }

    // layertab.cc
    cLayerTab();
    void SetLayerVisibility(LTstate, const CDl*, bool);
    void SetLayerSelectability(LTstate, const CDl*);
    void ProvisionallySetCurLayer(CDl*);
    void SetCurLayer(CDl*);
    void SetCurLayerFromClick(CmdDesc*);
    void HandleLayerSelect(CDl*);
    void InitLayerTable();
    void SetLayerColor(CDl*, int, int, int);
    void GetLayerColor(CDl*, int*, int*, int*);
    void ShowLayerTable(CDl* = 0);
    void FreezeLayerTable(bool);
    bool AddLayer(const char*, int);
    const char *RemoveLayer(const char*, DisplayMode);
    bool RenameLayer(const char*, const char*);
    void InitElecLayers();
    CDl *LayerAt(int, int);

    CDl *CurLayer() { return (ltc_curlayer); }

    void SetNoGraphics()
        {
            if (ltc_ltab)
                ltc_ltab->set_no_graphics();
        }

    void SetNoPhysRedraw(bool b)
        {
            if (ltc_ltab)
                ltc_ltab->set_no_phys_redraw(b);
        }

    bool NoPhysRedraw()
        {
            return (ltc_ltab ? ltc_ltab->no_phys_redraw() : false);
        }

    void HideLayerTable(bool b)
        {
            if (ltc_ltab)
                ltc_ltab->hide_layer_table(b);
        }

    void SetLtab(cLtab *t)    { ltc_ltab = t; }

private:
    // layertab_setif.cc
    void setupInterface();

    // Current layer
    CDl *ltc_curlayer;

    cLtab *ltc_ltab;
    bool ltc_ltab_frozen;

    static cLayerTab *instancePtr;
};

#endif


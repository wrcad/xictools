
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

//
// Main Xic application-specific header
//

#ifndef GTKMAIN_H
#define GTKMAIN_H

#include "gtkinterf/gtkinterf.h"
#include "dsp_tkif.h"

struct WindowDesc;
struct CDsym;
struct sLcb;
struct Ptxt;
struct sEvent;

namespace gtkexpand {
    struct sExp;
}
namespace gtkgrid {
    struct sGrd;
}
namespace gtkzoom {
    struct sZm;
}

// Graphics contexgt classes for application windows.
enum XIC_WINDOW_CLASS
{
    XW_DEFAULT, // Misc.
    XW_TEXT,    // Prompt line and similar.
    XW_DRAWING, // Main drawing area and viewports.
    XW_LPAL,    // The layer palette.
    XW_LTAB     // The layer table.
};

// widget name, for selections
#define PROPERTY_TEXT_WIDGET "property_text"

// gtkkeyb.cc
extern void LogEvent(GdkEvent*);

// Length of keypress buffer
#define CBUFMAX 16


class GTKpkg : public DSPpkg
{
public:
    GTKpkg()
    {
        busy_popup = 0;
        in_main_loop = false;
        not_mapped = false;
    }

    static GTKpkg *self() { return (dynamic_cast<GTKpkg*>(DSPpkg::self())); }

    // DSPpkg virtual overrides
    GRwbag *NewGX();
    int Initialize(GRwbag*);
    void ReinitNoGraphics();
    void Halt();
    void AppLoop();
    bool CheckForInterrupt();
    int Iconify(int);
    bool SubwinInit(int);
    void SubwinDestroy(int);

    bool SetWorking(bool);
    void SetOverrideBusy(bool);
    bool GetMainWinIdentifier(char*);

    bool UsingX11();
    void CloseGraphicsConnection();
    const char *GetDisplayString();
    bool CheckScreenAccess(hostent*, const char*, const char*);
    int RegisterIdleProc(int(*)(void*), void*);
    bool RemoveIdleProc(int);
    int RegisterTimeoutProc(int, int(*)(void*), void*);
    bool RemoveTimeoutProc(int);
    int StartTimer(int, bool*);
    void SetFont(const char*, int, FNT_FMT = FNT_FMT_ANY);
    const char *GetFont(int);
    FNT_FMT GetFontFmt();
    // end of overrides

    void RegisterEventHandler(void(*)(GdkEvent*, void*), void*);
    bool NotMapped() { return (not_mapped); }

    GRpopup *busy_popup;        // busy message
private:
    bool in_main_loop;          // gtk_main called
    bool not_mapped;            // true when iconic
};

// Base class for drawing window, used for subwindows.  Pop-ups here
// are associated with each drawing window, independently.
//
class GTKsubwin : virtual public DSPwbag, public GTKbag, public GTKdraw
{
public:
    GTKsubwin();
    ~GTKsubwin();

    void subw_initialize(int);
    void pre_destroy(int);

    // cAppWinFuncs interface (gtkmain.cc)
    //
    // pixmap manipulations
    void SwitchToPixmap();
    void SwitchFromPixmap(const BBox*);
    GRobject DrawableReset();
    void CopyPixmap(const BBox*);
    void DestroyPixmap();
    bool DumpWindow(const char*, const BBox*);
    bool PixmapOk();

    // key handling/display interface
    void GetTextBuf(char*);
    void SetTextBuf(const char*);
    void ShowKeys();
    void SetKeys(const char*);
    void BspKeys();
    bool AddKey(int);
    bool CheckBsp();
    void CheckExec(bool);
    char *KeyBuf();
    int KeyPos();

    // label
    void SetLabelText(const char*);

    // misc pop-ups
    // gtkgrid.cc
    void PopUpGrid(GRobject, ShowMode);
    // gtkmain.cc
    void PopUpExpand(GRobject, ShowMode,
        bool(*)(const char*, void*), void*, const char*, bool);
    void PopUpZoom(GRobject, ShowMode);

    //
    // End of cAppWinFuncs interface

    WindowDesc *windesc()   { return (wib_windesc); }

    // When dragging without pixmap backing, it takes too long to
    // redraw the drawing area behind the drag image.  This
    // short-circuits the redraw handler.  If is pretty awful to have
    // the drag image eat away the display, but NoPixmapStore should
    // not be used for anything but debugging.
    //
    // Exported to gtkfillp.cc and gtkltab.cc
    //
    static bool HaveDrag;

protected:
    // gtkmain.cc
    bool keypress_handler(unsigned, unsigned, char*, bool, bool);

    static int map_hdlr(GtkWidget*, GdkEvent*, void*);
    static int resize_hdlr(GtkWidget*, GdkEvent*, void*);
#if GTK_CHECK_VERSION(3,0,0)
    static int redraw_hdlr(GtkWidget*, cairo_t*, void*);
#else
    static int redraw_hdlr(GtkWidget*, GdkEvent*, void*);
#endif
    static int key_dn_hdlr(GtkWidget*, GdkEvent*, void*);
    static int key_up_hdlr(GtkWidget*, GdkEvent*, void*);
    static int button_dn_hdlr(GtkWidget*, GdkEvent*, void*);
    static int button_up_hdlr(GtkWidget*, GdkEvent*, void*);
    static int scroll_hdlr(GtkWidget*, GdkEvent*, void*);
    static int motion_hdlr(GtkWidget*, GdkEvent*, void*);
    static int motion_idle(void*);
    static int enter_hdlr(GtkWidget*, GdkEvent*, void*);
    static int leave_hdlr(GtkWidget*, GdkEvent*, void*);
    static int focus_hdlr(GtkWidget*, GdkEvent*, void*);
    static void drag_data_received(GtkWidget*, GdkDragContext*, gint, gint,
        GtkSelectionData*, guint, guint);
    static void target_drag_leave(GtkWidget*, GdkDragContext*, guint);
    static gboolean target_drag_motion(GtkWidget*, GdkDragContext*, gint, gint,
        guint);
    static void subwin_cancel_proc(GtkWidget*, void*);
    static int keys_hdlr(GtkWidget*, GdkEvent*, void*);

    WindowDesc *wib_windesc;    // back pointer
    gtkgrid::sGrd *wib_gridpop; // grid pop-up
    gtkexpand::sExp *wib_expandpop; // expand pop-up
    gtkzoom::sZm *wib_zoompop;  // zoom pop-up
    GtkWidget *wib_menulabel;   // subwindow menubar label

    GtkWidget *wib_keyspressed; // key press history
    int wib_keypos;             // index into keys
    char wib_keys[CBUFMAX + 3]; // keys pressed

#if GTK_CHECK_VERSION(3,0,0)
#else
    GdkWindow *wib_window_bak;  // screen buffer
    GdkPixmap *wib_draw_pixmap; // backing pixmap
    int wib_px_width;           // pixmap width
    int wib_px_height;          // pixmap height
#endif

    int wib_id;                 // motion idle id
    int wib_state;              // motion state
    int wib_x, wib_y;           // motion coords
    int wib_x0, wib_y0;         // reference coords
};

struct sKeyEvent;

// Main window class.
//
class GTKmainwin : public GTKsubwin
{
public:
    GTKmainwin()
    {
        mb_readout = 0;
        mb_auxw = 0;
    }

    static GTKmainwin *self()
    {
        if (DSP()->MainWdesc())
            return (dynamic_cast<GTKmainwin*>(DSP()->MainWdesc()->Wbag()));
        return (0);
    }

    static bool exists()
    {
        return (DSP()->MainWdesc() &&
            dynamic_cast<GTKmainwin*>(DSP()->MainWdesc()->Wbag()) != 0);
    }

    static bool is_shift_down()
    {
        if (self()) {
            unsigned state;
            self()->QueryPointer(0, 0, &state);
            if (state & GR_SHIFT_MASK)
                return (true);
        }
        return (false);
    }

    // gtkmain.cc
    void initialize();
    void send_key_event(sKeyEvent*);

    // gtkcells.cc
    static char *get_cell_selection();
    static void cells_panic();

    // gtkfiles.cc
    static char *get_file_selection();
    static void files_panic();

    // gtklibs.cc
    static char *get_lib_selection();
    static void libs_panic();

    // gtktree.cc
    static char *get_tree_selection();
    static void tree_panic();

private:
    static int main_destroy_proc(GtkWidget*, GdkEvent*, void*);
    static void xrm_load_colors();

    GtkWidget *mb_readout;
    GtkWidget *mb_auxw;
};

#endif



/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2007 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *  WHITELEY RESEARCH INCORPORATED PROPRIETARY SOFTWARE                   *
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
 *                                                                        *
 * XIC Integrated Circuit Layout and Schematic Editor                     *
 *                                                                        *
 *========================================================================*
 $Id: gtkmain.h,v 5.29 2016/03/02 00:39:41 stevew Exp $
 *========================================================================*/

//
// Main Xic application-specific header
//

#ifndef GTKMAIN_H
#define GTKMAIN_H

#include "gtkinterf.h"
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

// The XIC apptype for window group creation
#define XIC_APPTYPE 3

// widget name, for selections
#define PROPERTY_TEXT_WIDGET "property_text"

// gtkkeyb.cc
extern void LogEvent(GdkEvent*);

// Length of keypress buffer
#define CBUFMAX 16

inline class GTKpkg *gtkPkgIf();

class GTKpkg : public cGrPkg
{
public:
    GTKpkg()
        {
            busy_popup = 0;
            in_main_loop = false;
            not_mapped = false;
        }

    friend inline GTKpkg *gtkPkgIf()
        { return (dynamic_cast<GTKpkg*>(GRpkgIf())); }

    // cGrPkg virtual overrides
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

    bool IsDualPlane();
    bool IsTrueColor();
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
struct win_bag : virtual public DSPwbag, public gtk_bag, public gtk_draw
{
    win_bag();
    ~win_bag();

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
    static int redraw_hdlr(GtkWidget*, GdkEvent*, void*);
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
    bool wib_resized;           // window was just resized

    GdkWindow *wib_window_bak;  // screen buffer
    GdkPixmap *wib_draw_pixmap; // backing pixmap
    int wib_px_width;           // pixmap width
    int wib_px_height;          // pixmap height

    int wib_id;                 // motion idle id
    int wib_state;              // motion state
    int wib_x, wib_y;           // motion coords
};

struct sKeyEvent;

// Main window class.
//
struct main_bag : public win_bag
{
    main_bag()
        {
            mb_readout = 0;
            mb_auxw = 0;
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


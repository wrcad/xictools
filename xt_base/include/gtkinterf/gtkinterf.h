
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
 * GtkInterf Graphical Interface Library                                  *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#ifndef GTKINTERF_H
#define GTKINTERF_H

#include <time.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <gtk/gtk.h>
#include "ginterf/graphics.h"
#include "gtkinterf/gtkdraw.h"

//
//  Main header for the graphical interface using the GTK toolkit.
//


namespace gtkinterf { }
using namespace gtkinterf;

// Default background for HTML windows.
#define HTML_BG_COLOR "#e8e8f0"

#define NUMITEMS(x)  sizeof(x)/sizeof(x[0])


namespace gtkinterf {
    struct GTKdraw;     // The drawing package.
    struct GTKbag;      // A collection of support dialogs and widgets.

    // GTK driver class
    //
    class GTKdev : public GRscreenDev
    {
    public:
        struct sColorAlloc
        {
            unsigned long plane_mask[1];
            unsigned long drawing_pixels[64];
            int num_allocated;       // number private single plane cells
                                     //  allocated
            int num_mask_allocated;  // number private dual plane cells
                                     //  allocated
            bool no_alloc;           // set if no private colors anywhere
        };
        static sColorAlloc ColorAlloc;

        GTKdev();
        ~GTKdev();

        // virtual overrides
        // gtkinterf.cc
        virtual bool Init(int*, char**);
        bool InitColormap(int, int, bool);
        void RGBofPixel(int, int*, int*, int*);
        int AllocateColor(int*, int, int, int);
        int NameColor(const char*);
        bool NameToRGB(const char*, int*);
        GRdraw *NewDraw(int);
        GRwbag *NewWbag(const char*, GRwbag*);
        int AddTimer(int, int(*)(void*), void*);
        void RemoveTimer(int);
        GRsigintHdlr RegisterSigintHdlr(GRsigintHdlr);
        bool CheckForEvents();
        int Input(int, int, int*);
        void MainLoop(bool=false);
        int LoopLevel()             { return (dv_loop_level); }
        void BreakLoop()            { gtk_main_quit(); }
        int UseSHM();

        // virtual override, gtkhcopy.cc
        void HCmessage(const char*);

        // Remaining functions are unique to class.

        // This can be set by the application to the main frame widget
        // bag, in which case certain pop-ups called from other pop-ups
        // will be rooted in the main window, rather than the pop-up's
        // window
        //
        void RegisterMainFrame(GTKbag *w) { dv_main_wbag = w; }
        GTKbag *MainFrame()         { return (dv_main_wbag); }

        // Which image transfer code block to use.
        int ImageCode()             { return (dv_image_code); }

        // gtkinterf.cc
        int  ConnectFd();
        void Deselect(GRobject);
        void Select(GRobject);
        bool GetStatus(GRobject);
        void SetStatus(GRobject, bool);
        void CallCallback(GRobject);
        void Location(GRobject, int*, int*);
        void PointerRootLoc(int*, int*);
        const char *GetLabel(GRobject);
        void SetLabel(GRobject, const char*);
        void SetSensitive(GRobject, bool);
        bool IsSensitive(GRobject);
        void SetVisible(GRobject, bool);
        bool IsVisible(GRobject);
        void DestroyButton(GRobject);

        // gtkutil.cc
        void SetPopupLocation(GRloc, GtkWidget*, GtkWidget*);
        void ComputePopupLocation(GRloc, GtkWidget*, GtkWidget*, int*, int*);
        void WidgetLocation(GtkWidget*, int*, int*, int*, int*);
        void SetFocus(GtkWidget*);
        void SetDoubleClickExit(GtkWidget*, GtkWidget*);
        void RegisterBigWindow(GtkWidget *window);
        void RegisterBigForeignWindow(unsigned int);

#if GTK_CHECK_VERSION(3,0,0)
#else
        GdkColormap *Colormap()     { return (dv_cmap); }
#endif
        GdkVisual *Visual()         { return (dv_visual); }
        bool IsTrueColor()          { return (dv_true_color); }
        int  LowerWinOffset()       { return (dv_lower_win_offset); }
        void SetLowerWinOffset(int o) { dv_lower_win_offset = o; }
        bool IsDualPlane()          { return (dv_dual_plane); }
        void SetDefaultFocusWin(GdkWindow *w) { dv_default_focus_win = w; }
        GdkWindow *DefaultFocusWin() { return (dv_default_focus_win); }
#ifdef WITH_X11
        void SetSilenceErrs(bool b) { dv_silence_xerrors = b; }
        bool IsSilenceErrs()        { return (dv_silence_xerrors); }
        void SetNoToTop(bool b)     { dv_no_to_top = b; }
        bool IsNoToTop()            { return (dv_no_to_top); }
        unsigned int ConsoleXid()   { return (dv_console_xid); }
        unsigned int BigWindowXid() { return (dv_big_window_xid); }
#else
        void SetSilenceErrs(bool) { }
        bool IsSilenceErrs() { return (false); }
#endif

#ifdef WIN32
        // Flag for DOS/UNIX line termination in output.
        bool GetCRLFtermination()       { return (dv_crlf_terminate); }
        void SetCRLFtermination(bool b) { dv_crlf_terminate = b; }
#endif

    private:
        int dv_minx;
        int dv_miny;
        int dv_image_code;
        int dv_loop_level;
        GTKbag *dv_main_wbag;
        GdkWindow *dv_default_focus_win;
#if GTK_CHECK_VERSION(3,0,0)
#else
        GdkColormap *dv_cmap;
#endif
        GdkVisual *dv_visual;
        int dv_lower_win_offset;
        bool dv_dual_plane;  // color map has separate highlighting plane
        bool dv_true_color;  // not pseudo-color visual
#ifdef WITH_X11
        bool dv_silence_xerrors;
        bool dv_no_to_top;
        int dv_screen;
        unsigned int dv_console_xid;
        unsigned int dv_big_window_xid;
#endif
#ifdef WIN32
        bool dv_crlf_terminate;
#endif
    };

    struct GTKfontPopup;
    struct GTKledPopup;
    struct GTKmsgPopup;
    struct GTKtextPopup;
    struct GTKprintPopup;

    // Context implementation
    //
    struct GTKbag : virtual public GRwbag
    {
        friend GRwbag *GTKdev::NewWbag(const char*, GRwbag*);
        friend GtkWidget *gtk_NewPopup(GTKbag*, const char*,
            void(*)(GtkWidget*, void*), void*);

        GTKbag();
        virtual ~GTKbag();

        // This return will be used for positioning pop-ups rather than
        // the shell.  It is generally up to the user to set this.
        //
        GtkWidget *PositionReferenceWidget()
            {
                // This can be a derived class that includes a GTKdraw.
                GTKdraw *drw = dynamic_cast<GTKdraw*>(this);
                if (drw && drw->Viewport())
                    return (drw->Viewport());
                if (wb_textarea)
                    return (wb_textarea);
                return (wb_shell);
            }

        // misc. access to protected members

        void MonitorAdd(GRpopup *w)     { wb_monitor.add(w); }
        void MonitorRemove(GRpopup *w)  { wb_monitor.remove(w); }
        bool MonitorActive(GRpopup *w)  { return (wb_monitor.is_active(w)); }

        GtkWidget *Shell()              { return (wb_shell); }
        GTKprintPopup *HC()             { return (wb_hc); }
        void SetHC(GTKprintPopup *p)    { wb_hc = p; }
        GtkWidget *TextArea()           { return (wb_textarea); }

        // Pass a title for the window and icon.
        //
        void Title(const char *title, const char *icontitle)
            {
                if (wb_shell) {
                    gtk_window_set_title(GTK_WINDOW(wb_shell), title);
                    GdkWindow *w = gtk_widget_get_window(wb_shell);
                    if (w)
                        gdk_window_set_icon_name(w, icontitle);
                }
            }

        // gtkedit.cc
        GReditPopup *PopUpTextEditor(const char*,
            bool(*)(const char*, void*, XEtype), void*, bool);
        GReditPopup *PopUpFileBrowser(const char*);
        GReditPopup *PopUpStringEditor(const char*,
            bool(*)(const char*, void*, XEtype), void*);
        GReditPopup *PopUpMail(const char*, const char*,
            void(*)(GReditPopup*), GRloc = GRloc());

        // gtkfile.cc
        GRfilePopup *PopUpFileSelector(FsMode, GRloc,
            void(*)(const char*, void*),
            void(*)(GRfilePopup*, void*), void*, const char*);

        // gtkfont.cc
        void PopUpFontSel(GRobject, GRloc, ShowMode,
            void(*)(const char*, const char*, void*),
            void*, int, const char** = 0, const char* = 0);

        // gtkhcopy.cc
        void PopUpPrint(GRobject, HCcb*, HCmode, GRdraw* = 0);
        void HCupdate(HCcb*, GRobject);
        void HCsetFormat(int);
        void HcopyDisableMsgs();

        // Override these when printing support for a widget collection
        // is needed.
        virtual char *GetPostscriptText(int, const char*, const char*,
            bool, bool)
            { return (0); }  // see htm/htm_text.cc
        virtual char *GetPlainText()
            { return (0); }
        virtual char *GetHtmlText()
            { return (0); }

        // gtkhelp.cc
        bool PopUpHelp(const char*);

        // gtklist.cc
        GRlistPopup *PopUpList(stringlist*, const char*, const char*,
            void(*)(const char*, void*), void*, bool, bool);

        // gtkmcol.cc
        GRmcolPopup *PopUpMultiCol(stringlist*, const char*,
            void(*)(const char*, void*), void*, const char**,
            int = 0, bool = false);

        // gtkutil.cc
        void ClearPopups();
        void ClearPopup(GRpopup*);

        GRaffirmPopup *PopUpAffirm(GRobject, GRloc, const char*,
            void(*)(bool, void*), void*);
        GRnumPopup *PopUpNumeric(GRobject, GRloc, const char*, double,
            double, double, double, int, void(*)(double, bool, void*), void*);
        GRledPopup *PopUpEditString(GRobject, GRloc, const char*,
            const char*, ESret(*)(const char*, void*), void*, int,
            void(*)(bool), bool = false, const char* = 0);
        void PopUpInput(const char*, const char*, const char*,
            void(*)(const char*, void*), void*, int = 0);
        GRmsgPopup *PopUpMessage(const char*, bool, bool = false,
            bool = false, GRloc = GRloc());
        int PopUpWarn(ShowMode, const char*, STYtype = STY_NORM,
            GRloc = GRloc());
        int PopUpErr(ShowMode, const char*, STYtype = STY_NORM,
            GRloc = GRloc());
        GRtextPopup *PopUpErrText(const char*, STYtype = STY_NORM,
            GRloc = GRloc(LW_UL));
        int PopUpInfo(ShowMode, const char*, STYtype = STY_NORM,
            GRloc = GRloc(LW_LL));
        int PopUpInfo2(ShowMode, const char*, bool(*)(bool, void*),
            void*, STYtype = STY_NORM, GRloc = GRloc(LW_LL));
        int PopUpHTMLinfo(ShowMode, const char*, GRloc = GRloc(LW_LL));

        GRledPopup *ActiveInput();
        GRmsgPopup *ActiveMessage();
        GRtextPopup *ActiveInfo();
        GRtextPopup *ActiveInfo2();
        GRtextPopup *ActiveHtinfo();
        GRtextPopup *ActiveWarn();
        GRtextPopup *ActiveError();
        GRfontPopup *ActiveFontsel();

        void SetErrorLogName(const char*);

    protected:
        GtkWidget *wb_shell;        // top level widget
        GtkWidget *wb_textarea;     // text widget
        GTKledPopup *wb_input;      // dialog input popup
        GTKmsgPopup *wb_message;    // message popup
        GTKtextPopup *wb_info;      // info popup
        GTKtextPopup *wb_info2;     // two button info popup
        GTKtextPopup *wb_htinfo;    // html info popup
        GTKtextPopup *wb_warning;   // warning popup
        GTKtextPopup *wb_error;     // error popup
        GTKfontPopup *wb_fontsel;   // font select popup
        GTKprintPopup *wb_hc;       // hard copy parameters
        GRmonList wb_monitor;       // certain popups use this
        void *wb_call_data;         // internal data
        void (*wb_sens_set)(GTKbag*, bool, int);
                                    // sensitivity change callback
        int wb_warn_cnt;            // counters for window id
        int wb_err_cnt;
        int wb_info_cnt;
        int wb_info2_cnt;
        int wb_htinfo_cnt;

        // These are used in a lot of places.
        static const char *wb_closed_folder_xpm[];
        static const char *wb_open_folder_xpm[];
    };


    //
    // Misc global functions exported
    //

    // Drain the event queue, return false if max_events (> 0) given
    // and reached.
    //
    inline bool gtk_DoEvents(int max_events)
    {
        int i = 0;
        while (gtk_events_pending()) {
            gtk_main_iteration();
            if (++i == max_events)
                return (false);
        }
        return (true);
    }

    // gtkfile.cc
    void gtk_DoFileAction(GtkWidget*, const char*, const char*,
        GdkDragContext*, bool);
    void gtk_FileAction(GtkWidget*, const char*, const char*,
        GdkDragContext*);
    GtkWidget *gtk_Progress(GtkWidget*, const char*);
    void gtk_Message(GtkWidget*, bool, const char*);

    // gtkinterf.cc
    GtkWidget *gtk_NewPopup(GTKbag*, const char*,
        void(*)(GtkWidget*, void*), void*);
    void gtk_QueryColor(GdkColor*);
    bool gtk_ColorSet(GdkColor*, const char*);
    GdkColor *gtk_PopupColor(GRattrColor);

    // gtkutil.cc
    GtkWidget *DblClickSpinBtnContainer(GtkWidget*);
    void BlackHoleFix(GtkWidget*);
    bool ShellGeometry(GtkWidget*, GdkRectangle*, GdkRectangle*);
    int Btn1MoveHdlr(GtkWidget*, GdkEvent*, void*);
    int ToTop(GtkWidget*, GdkEvent*, void*);
    bool IsIconic(GtkWidget*);
    int MonitorGeom(GtkWidget*, int*, int*, int* = 0, int* = 0);

    GtkWidget *new_pixmap_button(const char**, const char*, bool);

    void text_scrollable_new(GtkWidget**, GtkWidget**, int);
    void text_set_chars(GtkWidget*, const char*);
    void text_insert_chars_at_point(GtkWidget*, GdkColor*, const char*,
        int, int);
    int text_get_insertion_point(GtkWidget*);
    void text_set_insertion_point(GtkWidget*, int);
    void text_delete_chars(GtkWidget*, int, int);
    void text_replace_chars(GtkWidget*, GdkColor*, const char*, int,
        int);
    char *text_get_chars(GtkWidget*, int, int);
    void text_select_range(GtkWidget*, int, int);
    bool text_has_selection(GtkWidget*);
    void text_get_selection_pos(GtkWidget*, int*, int*);
    char *text_get_selection(GtkWidget*);
    double text_get_scroll_value(GtkWidget*);
    double text_get_scroll_to_line_value(GtkWidget*, int, double);
    void text_set_scroll_value(GtkWidget*, double);
    void text_scroll_to_pos(GtkWidget*, int);
    void text_set_editable(GtkWidget*, bool);
    int text_get_length(GtkWidget*);
    void text_cut_clipboard(GtkWidget*);
    void text_copy_clipboard(GtkWidget*);
    void text_paste_clipboard(GtkWidget*);
    void text_set_change_hdlr(GtkWidget*, void(*)(GtkWidget*, void*),
        void*, bool);
    void text_realize_proc(GtkWidget*, void*);
}

// Global access, set in constructor
extern gtkinterf::GTKdev *GRX;

#endif // GTKINTERF_H


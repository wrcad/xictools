
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
 * WRspice Circuit Simulation and Analysis Tool                           *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#ifndef GTKTOOLB_H
#define GTKTOOLB_H

#include "gtkinterf/gtkinterf.h"
#include "keywords.h"
#include "toolbar.h"

// max size of the menu
#define NumMenuEnt 20

// save a tool state
struct tbent
{
    tbent() { name = 0; active = false; x = y = 0; }

    const char *name;
    bool active;
    int x, y;
};

struct tbpoint_t
{
    int x, y;
};

struct tb_bag : public gtk_bag, public gtk_draw
{
    tb_bag(int type = 0) : gtk_draw(type)
        {
            b_wid = 0;
            b_hei = 0;
            b_pixmap = 0;
            b_winbak = 0;
        }

    void switch_to_pixmap();
    void switch_from_pixmap();

    int b_wid;
    int b_hei;
    GdkPixmap *b_pixmap;
    GdkWindow *b_winbak;
};

extern inline class GTKtoolbar *TB();

// Keep track of the active popup widgets
//
class GTKtoolbar : public sToolbar
{
    static GTKtoolbar *ptr()
        {
            if (!instancePtr)
                on_null_ptr();
            return (instancePtr);
        }

    static void on_null_ptr();

public:
    friend inline GTKtoolbar *TB() { return (GTKtoolbar::ptr()); }

    // gtktoolb.cc
    GTKtoolbar();
    void Toolbar();
    void PopUpBugRpt(int, int);
    void PopDownBugRpt();
    void PopUpFont(int, int);
    void PopDownFont();
    void PopUpTBhelp(GRobject, GRobject, TBH_type);
    void PopDownTBhelp(TBH_type);
    void PopUpSpiceErr(bool, const char*);
    void PopUpSpiceMessage(const char*, int, int);
    void UpdateMain(ResUpdType);
    void CloseGraphicsConnection();
    int RegisterIdleProc(int(*)(void*), void*);
    bool RemoveIdleProc(int);
    int RegisterTimeoutProc(int, int(*)(void*), void*);
    bool RemoveTimeoutProc(int);
    void RegisterBigForeignWindow(unsigned int);

    void RevertFocus(GtkWidget*);
    tbent *FindEnt(const char*);
    GtkWidget *GetShell(const char*);
    void SetActive(const char*, bool);
    void SetLoc(const char*, GtkWidget*);
    void FixLoc(int*, int*);
    char *ConfigString();

    // gtkcmds.cc
    void PopUpCmdConfig(int, int);
    void PopDownCmdConfig();

    // gtkcolor.cc
    void PopUpColors(int, int);
    void PopDownColors();
    void UpdateColors(const char*);
    void LoadResourceColors();
    const char *XRMgetFromDb(const char*);

    // gtkdebug.cc
    void PopUpDebugDefs(int, int);
    void PopDownDebugDefs();

    // gtkpldef.cc
    void PopUpPlotDefs(int, int);
    void PopDownPlotDefs();

    // gtkshell.cc
    void PopUpShellDefs(int, int);
    void PopDownShellDefs();

    // gtksim.cc
    void PopUpSimDefs(int, int);
    void PopDownSimDefs();

    // gtkfte.cc
    void SuppressUpdate(bool);
    void PopUpPlots(int, int);
    void PopDownPlots();
    void UpdatePlots(int);
    void PopUpVectors(int, int);
    void PopDownVectors();
    void UpdateVectors(int);
    void PopUpCircuits(int, int);
    void PopDownCircuits();
    void UpdateCircuits();
    void PopUpFiles(int, int);
    void PopDownFiles();
    void UpdateFiles();
    void PopUpTrace(int, int);
    void PopDownTrace();
    void UpdateTrace();
    void PopUpVariables(int, int);
    void PopDownVariables();
    void UpdateVariables();

    void PopUpInfo(const char *msg)
        {
            if (context)
                context->PopUpInfo(MODE_ON, msg);
        }

    void PopUpNotes()       { notes_proc(0, 0); }

    GtkWidget *TextPop(int, int, const char*, const char*,
        const char*, void(*)(GtkWidget*, int, int), const char**, int,
        void(*)(GtkWidget*, void*));
    void RUsure(GtkWidget*, void(*)());

    bool Saved()            { return (tb_saved); }
    void SetSaved(bool b)   { tb_saved = b; }

    tb_bag *context;
    GtkWidget *toolbar;
    GtkWidget *tb_bug, *bg_shell, *bg_text;
    GtkWidget *tb_circuits, *ci_shell, *ci_text;
    GtkWidget *tb_colors, *co_shell;
    GtkWidget *tb_commands, *cm_shell;
    GtkWidget *tb_debug, *db_shell;
    GtkWidget *tb_files, *fi_shell, *fi_text, *fi_source, *fi_edit;
    GtkWidget *tb_font, *ft_shell;
    GtkWidget *tb_plotdefs, *pd_shell;
    GtkWidget *tb_plots, *pl_shell, *pl_text;
    GtkWidget *tb_shell, *sh_shell;
    GtkWidget *tb_simdefs, *sd_shell;
    GtkWidget *tb_trace, *tr_shell, *tr_text;
    GtkWidget *tb_variables, *va_shell, *va_text;
    GtkWidget *tb_vectors, *ve_shell, *ve_text;

    GtkWidget *tb_kw_help[TBH_end];
    tbpoint_t  tb_kw_help_pos[TBH_end];

    const char *ntb_bug;
    const char *ntb_circuits;
    const char *ntb_colors;
    const char *ntb_commands;
    const char *ntb_debug;
    const char *ntb_files;
    const char *ntb_font;
    const char *ntb_plotdefs;
    const char *ntb_plots;
    const char *ntb_shell;
    const char *ntb_simdefs;
    const char *ntb_toolbar;
    const char *ntb_trace;
    const char *ntb_variables;
    const char *ntb_vectors;

    tbent entries[NumMenuEnt];

private:
    // gtktoolb.cc
    void tbpop(bool);
    static void drag_data_received(GtkWidget*, GdkDragContext*, gint, gint,
        GtkSelectionData*, guint, guint);
    static gboolean target_drag_motion(GtkWidget*, GdkDragContext*, gint, gint,
        guint);
    static void target_drag_leave(GtkWidget*, GdkDragContext*, guint);
    static void wr_btn_hdlr(GtkWidget*, void*);
    static void open_proc(GtkWidget*, void*);
    static void source_proc(GtkWidget*, void*);
    static void load_proc(GtkWidget*, void*);
    static void update_proc(GtkWidget*, void*);
    static void wrupdate_proc(GtkWidget*, void*);
    static int quit_proc(GtkWidget*, void*);
    static void edit_proc(GtkWidget*, void*);
    static void xic_proc(GtkWidget*, void*);
    static void menu_proc(GtkWidget*, void*);
    static void help_proc(GtkWidget*, void*);
    static void about_proc(GtkWidget*, void*);
    static void notes_proc(GtkWidget*, void*);

    double tb_elapsed_start;
    char *tb_dropfile;

    GtkWidget *tb_file_menu;
    GtkWidget *tb_source_btn;
    GtkWidget *tb_load_btn;
    GtkWidget *tb_edit_menu;
    GtkWidget *tb_tools_menu;

    GReditPopup *tb_mailer;
    int tb_clr_1, tb_clr_2, tb_clr_3, tb_clr_4;
    bool tb_suppress_update;
    bool tb_saved;

    static GTKtoolbar *instancePtr;
};

// Differernt modes for the entry
//
enum EntryMode { KW_NORMAL, KW_INT_2, KW_REAL_2, KW_FLOAT, KW_NO_SPIN,
    KW_NO_CB, KW_STR_PICK };

// KW_NORMAL       Normal mode (regular spin button or entry)
// KW_INT_2        Use two integer fields (min/max)
// KW_REAL_2       Use two real fields (min/max)
// KW_FLOAT        Use exponential notation
// KW_NO_SPIN      Numeric entry but no adjustment
// KW_NO_CB        Special callbacks for debug popup
// KW_STR_PICK     String selection - callback arg passed to
//                  create_widgets()

// Struct used in the keyword entry popups
//
struct xEnt : public userEnt
{
    xEnt(void(*cb)(bool, variable*, xEnt*))
        { update = (void(*)(bool, variable*, void*))cb; val = 0.0; del = 0.0;
        pgsize = 0.0; rate = 0.0; numd = 0; defstr = 0; active = 0; deflt = 0;
        entry = 0; entry2 = 0; frame = 0; thandle = 0; down = false;
        mode = KW_NORMAL; }
    virtual ~xEnt() { delete [] defstr; }
    void setup(float v, float d, float p, float r, int n)
        { val = v; del = d; pgsize = p; rate = r; numd = n; }
    void callback(bool isset, variable *v)
        { if (update) (*update)(isset, v, this); }
    void create_widgets(sKWent<xEnt>*, const char*,
        int(*)(GtkWidget*, GdkEvent*, void*) = 0);
    void set_state(bool);
    void handler(void*);

    void (*update)(bool, variable*, void*);

    float val;                               // default numeric value
    float del, pgsize, rate;                 // spin button parameters
    int numd;                                // fraction digits shown
    const char *defstr;                      // default string
    GtkWidget *active;                       // "set" button
    GtkWidget *deflt;                        // "def" button
    GtkWidget *entry;                        // entry area
    GtkWidget *entry2;                       // entry area
    GtkWidget *frame;                        // top level of composite
    int thandle;                             // timer handle
    bool down;                               // decrement flag
    EntryMode mode;                          // operating mode
};
typedef sKWent<xEnt> xKWent;

//
// Some global callback functions for use as an argument to
// xEnt::xEnt(), for generic data
//
extern void kw_bool_func(bool, variable*, xEnt*);
extern void kw_int_func(bool, variable*, xEnt*);
extern void kw_real_func(bool, variable*, xEnt*);
extern void kw_string_func(bool, variable*, xEnt*);
extern int kw_float_hdlr(GtkWidget*, GdkEvent*, void*);

#endif


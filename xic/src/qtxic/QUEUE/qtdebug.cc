
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

#include "main.h"
#include "editif.h"
#include "fio.h"
#include "dsp_color.h"
#include "dsp_inlines.h"
#include "si_parsenode.h"
#include "si_parser.h"
#include "si_interp.h"
#include "menu.h"
#include "promptline.h"
#include "events.h"
#include "gtkmain.h"
#include "gtkinterf/gtkfont.h"
#include "gtkinterf/gtkutil.h"
#include "gtkinterf/gtksearch.h"
#include "miscutil/pathlist.h"
#include "miscutil/filestat.h"
#include <gdk/gdkkeysyms.h>


//-----------------------------------------------------------------------------
// Pop-up panel and supporting functions for script debugger.
//
// Help system keywords used:
//  xic:debug

// Default window size, assumes 6X13 chars, 80 cols, 20 rows
// with a 2-pixel margin
#define DEF_WIDTH 484
#define DEF_HEIGHT 264

// line buffer size
#define LINESIZE 1024

// max number of active breakpoints
#define NUMBKPTS 5

#define DEF_FILE "unnamed"

namespace {
    // Drag/drop stuff.
    GtkTargetEntry target_table[] = {
        { (char*)"TWOSTRING",   0, 0 },
        { (char*)"STRING",      0, 1 },
        { (char*)"text/plain",  0, 2 }
    };
    guint n_targets = sizeof(target_table) / sizeof(target_table[0]);

    // breakpoints
    struct sBp
    {
        sBp() { line = 0; active = false; }

        int line;
        bool active;
    };

    namespace gtkdebug {
        // function codes
        enum {NoCode, CancelCode, NewCode, LoadCode, PrintCode, SaveAsCode,
            CRLFcode, RunCode, StepCode, StartCode, MonitorCode, HelpCode };

        // current status
        enum Estatus { Equiescent, Eexecuting };

        // configuration mode
        enum DBmode { DBedit, DBrun }; 

        // Code passed to refresh()
        // locStart          scroll to top
        // locPresent        keep present position
        // locFollowCurrent  keep the current exec. line visible
        //
        enum locType { locStart, locPresent, locFollowCurrent };

        // Wariables listing pop-up.
        struct sDbV
        {
            sDbV(void*);
            ~sDbV();

            GtkWidget *Shell() { return (dv_popup); }
            void popdown() { delete this; }

            void update(stringlist*);

        private:
            static void dv_cancel_proc(GtkWidget*, void*);
            static int dv_select_proc(GtkTreeSelection*, GtkTreeModel*,
                GtkTreePath*, int, void*);
            static bool dv_focus_proc(GtkWidget*, GdkEvent*, void*);

            GtkWidget *dv_popup;
            GtkWidget *dv_list;
            void *dv_pointer;
            bool dv_no_select;      // treeview focus hack
        };

        // Main window.
        struct sDbg : public GTKbag
        {
            friend struct sDbV;

            // Undo list element.
            struct histlist
            {
                histlist(char *t, int p, bool d, histlist *n)
                    {
                        h_next = n;
                        h_text = t;
                        h_cpos = p;
                        h_deletion = d;
                    }

                ~histlist()
                    {
                        delete [] h_text;
                    }

                static void destroy(const histlist *l)
                    {
                        while (l) {
                            const histlist *x = l;
                            l = l->h_next;
                            delete x;
                        }
                    }

                histlist *h_next;
                char *h_text;
                int h_cpos;
                bool h_deletion;
            };

            sDbg(GRobject);
            ~sDbg();

            bool load_from_menu(MenuEnt*);

        private:
            void update_variables()
                {
                    if (db_vars_pop)
                        db_vars_pop->update(db_vlist);
                }

            void check_sens();
            void set_mode(DBmode);
            void set_line();
            bool is_last_line();
            void step();
            void run();
            void set_sens(bool);
            void start();
            void breakpoint(int);
            bool write_file(const char*);
            bool check_save(int);
            void refresh(bool, locType, bool = false);
            char *listing(bool);
            void monitor();
            const char *var_prompt(const char*, const char*, bool*);

            static void db_undo_proc(GtkWidget*, void*);
            static void db_redo_proc(GtkWidget*, void*);
            static void db_cut_proc(GtkWidget*, void*);
            static void db_copy_proc(GtkWidget*, void*);
            static void db_paste_proc(GtkWidget*, void*);
            static void db_paste_prim_proc(GtkWidget*, void*);
            static void db_search_proc(GtkWidget*, void*);
            static void db_font_proc(GtkWidget*, void*);
            static int db_step_idle(void*);
            static void db_font_changed();
            static void db_cancel_proc(GtkWidget*, void*);
            static void db_mode_proc(GtkWidget*, void*);
            static void db_change_proc(GtkWidget*, void*);
            static int db_key_dn_hdlr(GtkWidget*, GdkEvent*, void*);
            static int db_text_btn_hdlr(GtkWidget*, GdkEvent*, void*);
            static void db_action_proc(GtkWidget*, void*);
            static ESret db_open_cb(const char*, void*);
            static int db_open_idle(void*);
            static void db_do_saveas_proc(const char*, void*);
            static void db_drag_data_received(GtkWidget*, GdkDragContext*,
                gint, gint, GtkSelectionData*, guint, guint, void*);
            static void db_insert_text_proc(GtkTextBuffer*, GtkTextIter*,
                char*, int, void*);
            static void db_delete_range_proc(GtkTextBuffer*, GtkTextIter*,
                GtkTextIter*, void*);
            static stringlist *db_mklist(const char*);

            GRobject db_caller;
            GtkWidget *db_modelabel;
            GtkWidget *db_title;
            GtkWidget *db_modebtn;
            GtkWidget *db_saveas;
            GtkWidget *db_undo;
            GtkWidget *db_redo;
            GtkWidget *db_filemenu;
            GtkWidget *db_editmenu;
            GtkWidget *db_execmenu;
            GtkWidget *db_load_btn;
            GRledPopup *db_load_pop;
            sDbV *db_vars_pop;

            char *db_file_path;
            const char *db_line_ptr;
            char *db_line_save;
            char *db_dropfile;
            FILE *db_file_ptr;
            stringlist *db_vlist;
            GTKsearchPopup *db_search_pop;

            int db_line;
            int db_last_code;
            Estatus db_status;
            DBmode db_mode;
            bool db_text_changed;
            bool db_row_cb_flag;
            bool db_in_edit;
            bool db_in_undo;
            histlist *db_undo_list;
            histlist *db_redo_list;
            struct sBp db_breaks[NUMBKPTS];
        };

        sDbg *Dbg;
    }

    // for hardcopies
    HCcb dbgHCcb =
    {
        0,            // hcsetup
        0,            // hcgo
        0,            // hcframe
        0,            // format
        0,            // drvrmask
        HClegNone,    // legend
        HCportrait,   // orient
        0,            // resolution
        0,            // command
        false,        // tofile
        "",           // tofilename
        0.25,         // left
        0.25,         // top
        8.0,          // width
        10.5          // height
    };
}

using namespace gtkdebug;


// Menu command to bring up a panel which facilitates debugging of
// scripts.
//
void
cMain::PopUpDebug(GRobject caller, ShowMode mode)
{
    if (!GTKdev::exists() || !GTKmainwin::exists())
        return;
    if (mode == MODE_OFF) {
        delete Dbg;
        return;
    }

    if (Dbg)
        return;

    new sDbg(caller);
    if (!Dbg->Shell()) {
        delete Dbg;
        return;
    }
    gtk_window_set_transient_for(GTK_WINDOW(Dbg->Shell()),
        GTK_WINDOW(GTKmainwin::self()->Shell()));

    GTKdev::self()->SetPopupLocation(GRloc(), Dbg->Shell(),
        GTKmainwin::self()->Viewport());
    gtk_widget_show(Dbg->Shell());
}


// This is a callback from the main menu that sets the file name when
// one of the scripts in the debug menu is selected.
//
bool
cMain::DbgLoad(MenuEnt *ent)
{
    if (Dbg)
        return (Dbg->load_from_menu(ent));
    return (false);
}


// Assumptions about EditPrompt()
//  1.  returns 0 immediately if call is reentrant
//  2.  ErasePrompt() safe to call, does nothing while
//      editor is active

namespace {
    const char *MIDX = "midx";
}

sDbg::sDbg(GRobject c)
{
    Dbg = this;
    db_caller = c;
    db_modelabel = 0;
    db_title = 0;
    db_modebtn = 0;
    db_saveas = 0;
    db_undo = 0;
    db_redo = 0;
    db_filemenu = 0;
    db_editmenu = 0;
    db_execmenu = 0;
    db_load_btn = 0;
    db_load_pop = 0;
    db_vars_pop = 0;

    db_file_path = 0;
    db_line_ptr = 0;
    db_line_save = 0;
    db_dropfile = 0;
    db_file_ptr = 0;
    db_vlist = 0;
    db_search_pop = 0;

    db_line = 0;
    db_last_code = 0;
    db_status = Equiescent;
    db_mode = DBedit;
    db_text_changed = false;
    db_row_cb_flag = false;
    db_in_edit = false;
    db_in_undo = true;
    db_undo_list = 0;
    db_redo_list = 0;
    memset(db_breaks, 0, NUMBKPTS*sizeof(sBp));

    wb_shell = gtk_NewPopup(0, "Script Debugger", db_cancel_proc, 0);
    if (!wb_shell)
        return;
    // don't propogate unhandled key events to main window
    g_object_set_data(G_OBJECT(wb_shell), "no_prop_key", (void*)1);

    GtkWidget *form = gtk_table_new(1, 4, false);
    gtk_widget_show(form);
    gtk_container_add(GTK_CONTAINER(wb_shell), form);

    //
    // menu bar
    //
    GtkAccelGroup *accel_group = gtk_accel_group_new();
    gtk_window_add_accel_group(GTK_WINDOW(wb_shell), accel_group);
    GtkWidget *menubar = gtk_menu_bar_new();
    g_object_set_data(G_OBJECT(wb_shell), "menubar", menubar);
    gtk_widget_show(menubar);
    GtkWidget *item;

    // File menu.
    item = gtk_menu_item_new_with_mnemonic("_File");
    gtk_widget_set_name(item, "File");
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), item);
    GtkWidget *submenu = gtk_menu_new();
    gtk_widget_show(submenu);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), submenu);
    db_filemenu = item;

    // _New, 0, db_action_proc, NewCode, 0
    item = gtk_menu_item_new_with_mnemonic("_New");
    gtk_widget_set_name(item, "New");
    g_object_set_data(G_OBJECT(item), MIDX, (gpointer)(long)NewCode);
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(db_action_proc), this);

    // _Load", <control>", db_action_proc, LoadCode, 0
    item = gtk_menu_item_new_with_mnemonic("_Load");
    gtk_widget_set_name(item, "Load");
    g_object_set_data(G_OBJECT(item), MIDX, (gpointer)(long)LoadCode);
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(db_action_proc), this);
    gtk_widget_add_accelerator(item, "activate", accel_group, GDK_KEY_l,
        GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    db_load_btn = item;

    // _Print, <control>P, db_action_proc, PrintCode, 0
    item = gtk_menu_item_new_with_mnemonic("_Print");
    gtk_widget_set_name(item, "Print");
    g_object_set_data(G_OBJECT(item), MIDX, (gpointer)(long)PrintCode);
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(db_action_proc), this);
    gtk_widget_add_accelerator(item, "activate", accel_group, GDK_KEY_p,
        GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    // _Save As, <alt>A, db_action_proc, SaveAsCode, 0
    item = gtk_menu_item_new_with_mnemonic("_Save As");
    gtk_widget_set_name(item, "Save As");
    g_object_set_data(G_OBJECT(item), MIDX, (gpointer)(long)SaveAsCode);
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(db_action_proc), this);
//    gtk_widget_add_accelerator(item, "activate", accel_group, GDK_KEY_a,
//        GDK_ALT_MASK, GTK_ACCEL_VISIBLE);
    db_saveas = item;

#ifdef WIN32
    // _Write CRLF, 0, db_action_proc, CRLFcode, <CheckItem>"
    item = gtk_check_menu_item_new_with_mnemonic("_Write CRLF");
    gtk_widget_set_name(item, "Write CRLF");
    g_object_set_data(G_OBJECT(item), MIDX, (gpointer)(long)CRLFcode);
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(db_action_proc), this);
    GTKdev::SetStatus(item, GTKdev::self()->GetCRLFtermination());
#endif

    item = gtk_separator_menu_item_new();
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);

    // _Quit, <control>Q, db_action_proc, CancelCode, 0
    item = gtk_menu_item_new_with_mnemonic("_Quit");
    gtk_widget_set_name(item, "Quit");
    g_object_set_data(G_OBJECT(item), MIDX, (gpointer)(long)CancelCode);
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(db_action_proc), this);
    gtk_widget_add_accelerator(item, "activate", accel_group, GDK_KEY_q,
        GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    // Edit menu.
    item = gtk_menu_item_new_with_mnemonic("_Edit");
    gtk_widget_set_name(item, "Edit");
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), item);
    submenu = gtk_menu_new();
    gtk_widget_show(submenu);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), submenu);
    db_editmenu = item;

    // Undo, <Alt>U, db_undo_proc, 0, 0
    item = gtk_menu_item_new_with_mnemonic("Undo");
    gtk_widget_set_name(item, "Undo");
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(db_undo_proc), this);
//    gtk_widget_add_accelerator(item, "activate", accel_group, GDK_KEY_u,
//        GDK_ALT_MASK, GTK_ACCEL_VISIBLE);
    db_undo = item;

    // Redo, <Alt>R, db_redo_proc, 0, 0
    item = gtk_menu_item_new_with_mnemonic("Redo");
    gtk_widget_set_name(item, "Redo");
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(db_redo_proc), this);
//    gtk_widget_add_accelerator(item, "activate", accel_group, GDK_KEY_r,
//        GDK_ALT_MASK, GTK_ACCEL_VISIBLE);
    db_redo = item;

    item = gtk_separator_menu_item_new();
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);

    // Cut to Clipboard, <control>X, db_cut_proc, 0, 0
    item = gtk_menu_item_new_with_mnemonic("Cut to Clipboard");
    gtk_widget_set_name(item, "Cut to Clipboard");
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(db_cut_proc), this);
    gtk_widget_add_accelerator(item, "activate", accel_group, GDK_KEY_x,
        GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    // Copy to Clipboard, <control>C, db_copy_proc,  0, 0
    item = gtk_menu_item_new_with_mnemonic("Copy to Clipboard");
    gtk_widget_set_name(item, "Copy to Clipboard");
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(db_copy_proc), this);
    gtk_widget_add_accelerator(item, "activate", accel_group, GDK_KEY_c,
        GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    // Paste from Clipboard, <control>V, db_paste_proc, 0, 0
    item = gtk_menu_item_new_with_mnemonic("Paste from Clipboard");
    gtk_widget_set_name(item, "Paste from Clipboard");
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(db_paste_proc), this);
    gtk_widget_add_accelerator(item, "activate", accel_group, GDK_KEY_v,
        GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    // Paste Primary, <alt>P, db_paste_prim_proc, 0, 0
    item = gtk_menu_item_new_with_mnemonic("Paste Primary");
    gtk_widget_set_name(item, "Paste Primary");
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(db_paste_prim_proc), this);
//    gtk_widget_add_accelerator(item, "activate", accel_group, GDK_KEY_p,
//        GDK_ALT_MASK, GTK_ACCEL_VISIBLE);

    // Execute menu.
    item = gtk_menu_item_new_with_mnemonic("E_xecute");
    gtk_widget_set_name(item, "Execute");
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), item);
    submenu = gtk_menu_new();
    gtk_widget_show(submenu);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), submenu);
    db_execmenu = item;

    // _Run, <control>R, db_action_proc, RunCode, 0
    item = gtk_menu_item_new_with_mnemonic("_Run");
    gtk_widget_set_name(item, "Run");
    g_object_set_data(G_OBJECT(item), MIDX, (gpointer)(long)RunCode);
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(db_action_proc), this);
    gtk_widget_add_accelerator(item, "activate", accel_group, GDK_KEY_r,
        GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    // S_tep, <control>T, db_action_proc, StepCode, 0
    item = gtk_menu_item_new_with_mnemonic("S_tep");
    gtk_widget_set_name(item, "Step");
    g_object_set_data(G_OBJECT(item), MIDX, (gpointer)(long)StepCode);
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(db_action_proc), this);
    gtk_widget_add_accelerator(item, "activate", accel_group, GDK_KEY_t,
        GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    // R_eset, <control>E, db_action_proc, StartCode, 0
    item = gtk_menu_item_new_with_mnemonic("R_eset");
    gtk_widget_set_name(item, "Reset");
    g_object_set_data(G_OBJECT(item), MIDX, (gpointer)(long)StartCode);
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(db_action_proc), this);
    gtk_widget_add_accelerator(item, "activate", accel_group, GDK_KEY_e,
        GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    // _Monitor, <control>M, db_action_proc, MonitorCode,0
    item = gtk_menu_item_new_with_mnemonic("_Monitor");
    gtk_widget_set_name(item, "Monitor");
    g_object_set_data(G_OBJECT(item), MIDX, (gpointer)(long)MonitorCode);
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(db_action_proc), this);
    gtk_widget_add_accelerator(item, "activate", accel_group, GDK_KEY_m,
        GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    // Options menu.
    item = gtk_menu_item_new_with_mnemonic("_Options");
    gtk_widget_set_name(item, "Options");
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), item);
    submenu = gtk_menu_new();
    gtk_widget_show(submenu);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), submenu);

    // _Search, 0, db_search_proc, 0, <CheckItem>);
    item = gtk_check_menu_item_new_with_mnemonic("_Search");
    gtk_widget_set_name(item, "Search");
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(db_search_proc), this);

    // _Font, 0, db_font_proc, 0, <CheckItem>
    item = gtk_check_menu_item_new_with_mnemonic("_Font");
    gtk_widget_set_name(item, "Font");
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(db_font_proc), this);

    // Help menu.
    item = gtk_menu_item_new_with_mnemonic("_Help");
    gtk_menu_item_set_right_justified(GTK_MENU_ITEM(item), true);
    gtk_widget_set_name(item, "Help");
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), item);
    submenu = gtk_menu_new();
    gtk_widget_show(submenu);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), submenu);

    // _Help, <control>H, db_action_proc, HelpCode, 0
    item = gtk_menu_item_new_with_mnemonic("_Help");
    gtk_widget_set_name(item, "Help");
    g_object_set_data(G_OBJECT(item), MIDX, (gpointer)(long)HelpCode);
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(db_action_proc), this);
    gtk_widget_add_accelerator(item, "activate", accel_group, GDK_KEY_h,
        GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    gtk_table_attach(GTK_TABLE(form), menubar, 0, 1, 0, 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    //
    // label area
    //
    GtkWidget *hbox = gtk_hbox_new(false, 0);
    gtk_widget_show(hbox);

    db_modelabel = gtk_label_new("Edit Mode");
    gtk_widget_show(db_modelabel);
    gtk_label_set_justify(GTK_LABEL(db_modelabel), GTK_JUSTIFY_LEFT);
    gtk_box_pack_start(GTK_BOX(hbox), db_modelabel, false, false, 2);

    db_title = gtk_label_new("");
    gtk_widget_show(db_title);
    gtk_box_pack_end(GTK_BOX(hbox), db_title, true, true, 2);

    GtkWidget *frame = gtk_frame_new(0);
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), hbox);

    hbox = gtk_hbox_new(false, 0);
    gtk_widget_show(hbox);

    db_modebtn = gtk_button_new_with_label("Run");
    gtk_widget_set_name(db_modebtn, "Mode");
    gtk_widget_set_sensitive(db_modebtn, false);
    gtk_widget_show(db_modebtn);
    g_signal_connect(G_OBJECT(db_modebtn), "clicked",
        G_CALLBACK(db_mode_proc), 0);
    gtk_box_pack_start(GTK_BOX(hbox), db_modebtn, false, false, 0);
    gtk_box_pack_start(GTK_BOX(hbox), frame, true, true, 0);

    gtk_table_attach(GTK_TABLE(form), hbox, 0, 1, 1, 2,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    //
    // text window with scroll bar
    //
    GtkWidget *contr;
    text_scrollable_new(&contr, &wb_textarea, FNT_FIXED);

    g_signal_connect(G_OBJECT(wb_textarea), "button-press-event",
        G_CALLBACK(db_text_btn_hdlr), 0);

    gtk_widget_add_events(wb_shell, GDK_KEY_PRESS_MASK);
    g_signal_connect(G_OBJECT(wb_shell), "key-press-event",
        G_CALLBACK(db_key_dn_hdlr), 0);

    gtk_widget_set_size_request(wb_textarea, DEF_WIDTH, DEF_HEIGHT);

    // The font change pop-up uses this to redraw the widget
    g_object_set_data(G_OBJECT(wb_textarea), "font_changed",
        (void*)db_font_changed);

    gtk_table_attach(GTK_TABLE(form), contr, 0, 1, 2, 3,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 0);

    // drop site
    gtk_drag_dest_set(wb_textarea, GTK_DEST_DEFAULT_ALL, target_table,
        n_targets, GDK_ACTION_COPY);
    g_signal_connect_after(G_OBJECT(wb_textarea), "drag-data-received",
        G_CALLBACK(db_drag_data_received), 0);

    GtkTextBuffer *tbf =
        gtk_text_view_get_buffer(GTK_TEXT_VIEW(wb_textarea));
    g_signal_connect(G_OBJECT(tbf), "insert-text",
        G_CALLBACK(db_insert_text_proc), this);
    g_signal_connect(G_OBJECT(tbf), "delete-range",
        G_CALLBACK(db_delete_range_proc), this);
    db_in_undo = false;
    check_sens();

    if (db_caller) {
        g_signal_connect(G_OBJECT(db_caller), "toggled",
            G_CALLBACK(db_cancel_proc), wb_shell);
    }

    text_set_editable(wb_textarea, true);
    text_set_change_hdlr(wb_textarea, db_change_proc, 0, true);
    gtk_widget_set_sensitive(db_execmenu, false);
    gtk_window_set_focus(GTK_WINDOW(wb_shell), wb_textarea);
}


sDbg::~sDbg()
{
    Dbg = 0;
    stringlist::destroy(db_vlist);
    delete [] db_line_save;
    delete [] db_dropfile;
    delete [] db_file_path;

    if (db_in_edit || db_row_cb_flag)
        PL()->AbortEdit();
    if (db_load_pop)
        db_load_pop->popdown();
    if (db_vars_pop)
        db_vars_pop->popdown();
    if (db_caller) {
        g_signal_handlers_disconnect_by_func(G_OBJECT(db_caller),
            (gpointer)db_cancel_proc, wb_shell);
        GTKdev::Deselect(db_caller);
    }
    histlist::destroy(db_undo_list);
    histlist::destroy(db_redo_list);

    SI()->Clear();
    if (wb_shell) {
        g_signal_handlers_disconnect_by_func(G_OBJECT(wb_shell),
            (gpointer)db_cancel_proc, wb_shell);
    }
}


// Check/set menu item sensitivity for items that change during editing.
//
void
sDbg::check_sens()
{
    if (db_undo)
        gtk_widget_set_sensitive(db_undo, (db_undo_list != 0));
    if (db_redo)
        gtk_widget_set_sensitive(db_redo, (db_redo_list != 0));
}


// Set configuration mode.
//
void
sDbg::set_mode(DBmode mode)
{
    if (mode == DBedit) {
        if (db_mode != DBedit) {
            db_mode = DBedit;
            text_set_editable(wb_textarea, true);
            refresh(true, locPresent);
            text_set_change_hdlr(wb_textarea, db_change_proc, 0, true);
            gtk_label_set_text(GTK_LABEL(db_modelabel), "Edit Mode");
            gtk_widget_set_sensitive(db_execmenu, false);
            gtk_widget_set_sensitive(db_editmenu, true);
            g_object_set(G_OBJECT(db_modebtn), "label", "Run", (char*)0);

            GdkCursor *c = gdk_cursor_new(GDK_XTERM);
            gdk_window_set_cursor(
                gtk_text_view_get_window(GTK_TEXT_VIEW(wb_textarea),
                GTK_TEXT_WINDOW_TEXT), c);
            g_object_unref(G_OBJECT(c));
        }
    }
    else if (mode == DBrun) {
        if (db_mode != DBrun) {
            db_mode = DBrun;
            text_set_change_hdlr(wb_textarea, db_change_proc, 0, false);
            text_set_editable(wb_textarea, false);
            refresh(true, locPresent);
            start();
            gtk_label_set_text(GTK_LABEL(db_modelabel), "Exec Mode");
            gtk_widget_set_sensitive(db_execmenu, true);
            gtk_widget_set_sensitive(db_editmenu, false);
            g_object_set(G_OBJECT(db_modebtn), "label", "Edit", (char*)0);

            GdkCursor *c = gdk_cursor_new(GDK_TOP_LEFT_ARROW);
            gdk_window_set_cursor(
                gtk_text_view_get_window(GTK_TEXT_VIEW(wb_textarea),
                GTK_TEXT_WINDOW_TEXT),  c);
            g_object_unref(G_OBJECT(c));
        }
    }
}


bool
sDbg::load_from_menu(MenuEnt *ent)
{
    if (db_load_pop) {
        const char *entry = ent->menutext + strlen("/User/");
        // entry is the same as ent->entry, but contains the menu path
        // for submenu items
        char buf[256];
        if (lstring::strdirsep(entry)) {
            // from submenu, add a distinguishing prefix to avoid
            // confusion with file path
            snprintf(buf, sizeof(buf), "%s%s", SCR_LIBCODE, entry);
            entry = buf;
        }
        char *scrfile = XM()->FindScript(entry);
        if (scrfile) {
            db_load_pop->update(0, scrfile);
            delete [] scrfile;
            return (true);
        }
    }
    return (false);
}


// Set db_line_ptr pointing at the line for the "Line" field.
//
void
sDbg::set_line()
{
    delete [] db_line_save;
    db_line_save = 0;

    if (!wb_textarea) {
        db_line_ptr = 0;
        return;
    }
    char *string = text_get_chars(wb_textarea, 0, -1);
    int count = 0;
    char *t;
    for (t = string; *t && count < db_line; t++) {
        if (*t == '\n')
            count++;
    }
    if (!*t || !*(t+1) || !*(t+2))
        db_line_ptr = 0;
    else
        db_line_ptr = db_line_save = lstring::copy(t);
    delete [] string;
}


// See if db_line now points beyond the end, in which case we're done.
//
bool
sDbg::is_last_line()
{
    char *string = text_get_chars(wb_textarea, 0, -1);
    int count = 0;
    char *t;
    for (t = string; *t && count < db_line; t++) {
        if (*t == '\n')
            count++;
    }
    bool ret = (!*t || !*(t+1) || !*(t+2));
    delete [] string;
    return (ret);
}


// Execute a single line of code.
//
void
sDbg::step()
{
    if (!Dbg)
        return;
    if (db_row_cb_flag) {
        // We're waiting for input in the prompt line, back out.
        PL()->AbortEdit();
        return;
    }
    set_line();
    if (!SI()->IsInBlock() && !db_line_ptr) {
        PL()->SavePrompt();
        start();
        PL()->RestorePrompt();
        GTKdev::SetFocus(Dbg->wb_shell);
        gtk_window_set_focus(GTK_WINDOW(Dbg->wb_shell), Dbg->wb_textarea);
        return;
    }
    db_status = Eexecuting;
    set_sens(false);
    db_line = SI()->Interpret(0, 0, &db_line_ptr, 0, true);
    if (!Dbg) {
        // debugger window deleted
        SI()->Clear();
        EditIf()->ulCommitChanges();
        return;
    }
    if (SI()->IsHalted() || (!SI()->IsInBlock() && is_last_line())) {
        EditIf()->ulCommitChanges();
        PL()->SavePrompt();
        start();
        PL()->RestorePrompt();
    }
    else
        refresh(false, locFollowCurrent);
    db_status = Equiescent;
    set_sens(true);
    update_variables();
    GTKdev::SetFocus(Dbg->wb_shell);
    gtk_window_set_focus(GTK_WINDOW(Dbg->wb_shell), Dbg->wb_textarea);
}


// Execute until the next breakpoint.
//
void
sDbg::run()
{
    if (db_row_cb_flag) {
        // We're waiting for input in the prompt line, back out.
        PL()->AbortEdit();
        return;
    }
    db_status = Eexecuting;
    set_sens(false);
    int tline = db_line;
    db_line = -1;
    refresh(false, locPresent);  // erase caret
    db_line = tline;
    for (;;) {
        set_line();
        if (!SI()->IsInBlock() && !db_line_ptr) {
            // shouldn't happen
            SI()->Clear();
            PL()->SavePrompt();
            start();
            PL()->RestorePrompt();
            break;
        }

        db_line = SI()->Interpret(0, 0, &db_line_ptr, 0, true);
        if (!Dbg) {
            // debugger window deleted
            SI()->Clear();
            EditIf()->ulCommitChanges();
            return;
        }
        if (SI()->IsHalted() || (!SI()->IsInBlock() &&
                (!db_line_ptr || !*db_line_ptr || !*(db_line_ptr+1) ||
                !*(db_line_ptr+2)))) {
            // finished
            SI()->Clear();
            PL()->SavePrompt();
            start();
            PL()->RestorePrompt();
            break;
        }
        if (GTKpkg::self()->CheckForInterrupt()) {
            // ^C typed
            refresh(false, locFollowCurrent);
            break;
        }

        int i;
        for (i = 0; i < NUMBKPTS; i++)
            if (db_breaks[i].line == db_line && db_breaks[i].active)
                break;
        if (i < NUMBKPTS) {
            // hit a breakpoint
            refresh(false, locFollowCurrent);
            break;
        }
    }
    EditIf()->ulCommitChanges();
    db_status = Equiescent;
    set_sens(true);
    update_variables();
}


// Desensitize buttons while executing.
//
void
sDbg::set_sens(bool sens)
{
    gtk_widget_set_sensitive(db_filemenu, sens);
    gtk_widget_set_sensitive(db_execmenu, sens);
}


// Reset a paused executing script.
//
void
sDbg::start()
{
    if (db_row_cb_flag) {
        // We're waiting for input in the prompt line, back out.
        PL()->AbortEdit();
        return;
    }
    SI()->Init();
    db_line = 0;
    set_line();
    db_line = SI()->Interpret(0, 0, &db_line_ptr, 0, true);
    refresh(false, locStart);
    EV()->InitCallback();
    EditIf()->ulListCheck("run", CurCell(), false);
}


// Set/reset breakpoint at line.
//
void
sDbg::breakpoint(int line)
{
    int i;
    if (line < 0) {
        for (i = 0; i < NUMBKPTS; i++)
            db_breaks[i].active = false;
        return;
    }
    if (line == db_line) {
        // Clicking on the active line steps it
        GTKpkg::self()->RegisterIdleProc(db_step_idle, 0);
        return;
    }
    for (i = 0; i < NUMBKPTS; i++) {
        if (db_breaks[i].line == line) {
            db_breaks[i].active = !db_breaks[i].active;
            refresh(false, locPresent);
            return;
        }
    }
    for (i = NUMBKPTS-1; i > 0; i--)
        db_breaks[i] = db_breaks[i-1];
    db_breaks[0].line = line;
    db_breaks[0].active = true;
    refresh(false, locPresent);
}


// Dump the buffer to fname.  Return false if error.
//
bool
sDbg::write_file(const char *fname)
{
    if (!filestat::create_bak(fname)) {
        GTKpkg::self()->ErrPrintf(ET_ERROR, "%s", filestat::error_msg());
        return (false);
    }
    FILE *fp = fopen(fname, "w");
    if (!fp) {
        // shouldn't happen
        PopUpMessage("Can't open file, text not saved", true);
        return (false);
    }
#ifdef WIN32
    char lastc = 0;
#endif
    char *string = text_get_chars(wb_textarea, 0, -1);
    if (db_mode == DBedit) {
#ifdef WIN32
        char *s = string;
        while (*s) {
            if (!GTKdev::self()->GetCRLFtermination()) {
                if (s[0] == '\r' && s[1] == '\n') {
                    lastc = *s++;
                    continue;
                }
            }
            else if (s[0] == '\n' && lastc != '\r')
                putc('\r', fp);
            putc(s[0], fp);
            lastc = *s++;
        }
#else
        fputs(string, fp);
#endif
    }
    else {
        char *s = string;
        if (*s && *(s+1)) {
            s += 2;
#ifdef WIN32
            while (*s) {
                if (!GTKdev::self()->GetCRLFtermination()) {
                    if (s[0] == '\r' && s[1] == '\n') {
                        lastc = *s++;
                        continue;
                    }
                }
                else if (s[0] == '\n' && lastc != '\r')
                    putc('\r', fp);
                putc(s[0], fp);
                lastc = *s++;
            }
#else
            while (*s) {
                if (*s == '\n') {
                    putc('\n', fp);
                    s++;
                    if (!*s || !*(s+1))
                        break;
                    s += 2;
                }
                else {
                    putc(*s, fp);
                    s++;
                }
            }
#endif
        }
    }
    fclose(fp);
    delete [] string;
    return (true);
}


// If unsaved text, warn.  Return true to abort operation.
//
bool
sDbg::check_save(int code)
{
    if (db_text_changed) {
        const char *str;
        char buf[128];
        switch (code) {
        case CancelCode:
            str = "Press Quit again to quit\nwithout saving changes.";
            break;
        case LoadCode:
            str = "Press Load again to load.";
            break;
        case NewCode:
            str = "Press New again to clear.";
            break;
        default:
            return (false);
        }
        if (db_last_code != code) {
            db_last_code = code;
            snprintf(buf, sizeof(buf), "Text has been modified.  %s", str);
            PopUpMessage(buf, false);
            return (true);
        }
        if (wb_message)
            wb_message->popdown();
    }
    return (false);
}


// Update the text listing.
// If mode_switch, strip margin for edit mode.
//
void
sDbg::refresh(bool mode_switch, locType loc, bool clear)
{
    if (!Dbg)
        return;
    char *s = listing(clear ? false : mode_switch);
    double val = text_get_scroll_value(wb_textarea);
    if (db_mode == DBedit) {
        db_in_undo = true;
        text_set_chars(wb_textarea, s);
        db_in_undo = false;
        if (loc == locStart)
            text_set_scroll_value(wb_textarea, 0.0);
        else
            text_set_scroll_value(wb_textarea, val);
    }
    else {
        if (loc == locStart)
            val = 0.0;
        else if (loc == locFollowCurrent)
            val = text_get_scroll_to_line_value(wb_textarea, db_line, val);
        db_in_undo = true;
        if (mode_switch || clear) {
            text_set_chars(wb_textarea, "");
            GdkColor *c = gtk_PopupColor(GRattrColorHl1);
            char *t = s;
            while (*t) {
                char *end = t;
                while (*end && *end != '\n')
                    end++;
                if (*end == '\n')
                    end++;
                text_insert_chars_at_point(wb_textarea, c, t, 1, -1);
                text_insert_chars_at_point(wb_textarea, 0, t+1, end-t-1, -1);
                if (!*end)
                    break;
                t = end;
            }
        }
        else {
            char *str = text_get_chars(wb_textarea, 0, -1);
            GdkColor *c = gtk_PopupColor(GRattrColorHl1);
            char *t = s;
            while (*t) {
                char *end = t;
                while (*end && *end != '\n')
                    end++;
                if (*end == '\n')
                    end++;
                if (*t != str[t-s])
                    text_replace_chars(wb_textarea, c, t, t-s, t-s + 1);
                if (!*end)
                    break;
                t = end;
            }
            delete [] str;
        }
        db_in_undo = false;
        double vt = text_get_scroll_value(wb_textarea);
        if (fabs(vt - val) > 1.0)
            text_set_scroll_value(wb_textarea, val);
    }
    delete [] s;
}


// If the db_file_ptr is not 0, read in the file.  Otherwise return the
// buffer contents.  If mode_switch is true, an edit mode switch is in
// progress.
//
char *
sDbg::listing(bool mode_switch)
{
    if (!Dbg)
        return (0);
    char buf[LINESIZE];
    if (!db_file_ptr) {
        char *str = text_get_chars(wb_textarea, 0, -1);
        char *t = str;
        if (mode_switch) {
            if (db_mode == DBedit) {
                // switching to edit mode, strip margin
                char *ostr = str;
                t = new char[strlen(t)+1];
                char *s = t;
                if (*str && *(str+1)) {
                    str += 2;
                    while (*str) {
                        if (*str == '\n') {
                            *s++ = '\n';
                            str++;
                            if (*str && *(str+1))
                                str += 2;
                            else
                                break;
                        }
                        else
                            *s++ = *str++;
                    }
                }
                *s = '\0';
                delete [] ostr;
                str = t;
            }
            else {
                // switching from edit mode, add margin
                int i;
                for (i = 0; *t; i++, t++) {
                    if (*t == '\n')
                        i += 2;
                }
                t = new char[i+5];
                char *s = t;
                *s++ = ' ';
                *s++ = ' ';
                char *ostr = str;
                while (*str) {
                    if (*str == '\n') {
                        *s++ = '\n';
                        *s++ = ' ';
                        *s++ = ' ';
                        str++;
                    }
                    else
                        *s++ = *str++;
                }
                *s = '\0';
                delete [] ostr;
                str = t;
            }
        }
        if (db_mode == DBrun) {
            int line = 0;
            for (;;) {
                if (!*t || !*(t+1) || !*(t+2))
                    break;
                int i;
                for (i = 0; i < NUMBKPTS; i++)
                    if (db_breaks[i].active && line == db_breaks[i].line)
                        break;
                if (i == NUMBKPTS)
                    t[0] = ' ';
                else
                    t[0] = 'B';
                if (line == db_line)
                    t[0] = '>';
                t[1] = ' ';
                while (*t && *t != '\n')
                    t++;
                if (!*t)
                    break;
                line++;
                t++;
            }
        }
        return (str);
    }
    char *str = lstring::copy("");
    int line = 0;
    long ftold = ftell(db_file_ptr);
    rewind(db_file_ptr);
    while (fgets(buf+2, 1021, db_file_ptr) != 0) {
        NTstrip(buf+2);
        if (db_mode == DBedit)
            str = lstring::build_str(str, buf+2);
        else {
            int i;
            for (i = 0; i < NUMBKPTS; i++)
                if (db_breaks[i].active && line == db_breaks[i].line)
                    break;
            if (i == NUMBKPTS)
                buf[0] = ' ';
            else
                buf[0] = 'B';
            if (line == db_line)
                buf[0] = '>';
            buf[1] = ' ';
            str = lstring::build_str(str, buf);
        }
        line++;
    }
    fseek(db_file_ptr, ftold, SEEK_SET);
    return (str);
}


// Solicit a list of variables, and pop up the monitor window.
//
void
sDbg::monitor()
{
    if (!Dbg)
        return;
    char buf[256];
    char *s = buf;
    *s = 0;
    for (stringlist *wl = db_vlist; wl; wl = wl->next) {
        if (s - buf + strlen(wl->string) + 2 >= 256)
            break;
        if (s > buf)
           *s++ = ' ';
        strcpy(s, wl->string);
        while (*s)
            s++;
    }
    db_in_edit = true;
    char *in = PL()->EditPrompt("Variables? ", buf);
    db_in_edit = false;
    PL()->ErasePrompt();
    if (!in)
        return;
    stringlist::destroy(db_vlist);
    db_vlist = db_mklist(in);

    if (db_vars_pop) {
        db_vars_pop->update(db_vlist);
        return;
    }
    if (!db_vlist)
        return;
    if (!GTKdev::exists() || !GTKmainwin::exists())
        return;

    db_vars_pop = new sDbV(&db_vars_pop);
    if (!db_vars_pop->Shell()) {
        delete db_vars_pop;
        return;
    }

    GTKdev::self()->SetPopupLocation(GRloc(LW_LR), db_vars_pop->Shell(),
        wb_shell);
    gtk_window_set_transient_for(GTK_WINDOW(db_vars_pop->Shell()),
        GTK_WINDOW(GTKmainwin::self()->Shell()));
    gtk_widget_show(db_vars_pop->Shell());

    // Calling this from here avoids spontaneously selecting the first
    // entry, not sure that I know why.  The selection is very
    // undesirable since it prompts for a value.
    db_vars_pop->update(db_vlist);
}


// Export for the variables window selection handler.
//
const char *
sDbg::var_prompt(const char *text, const char *buf, bool *busy)
{
    *busy = false;
    if (Dbg->db_row_cb_flag) {
        PL()->EditPrompt(buf, text, PLedUpdate);
        *busy = true;
        return (0);
    }
    Dbg->db_row_cb_flag = true;
    char *in = PL()->EditPrompt(buf, text);
    if (!Dbg) {
        *busy = true;
        return (0);
    }
    Dbg->db_row_cb_flag = false;
    return (in);
}


// Private static GTK signal handler.
// Undo last insert/delete.
//
void
sDbg::db_undo_proc(GtkWidget*, void*)
{
    if (!Dbg || !Dbg->db_undo_list)
        return;
    histlist *h = Dbg->db_undo_list;
    Dbg->db_undo_list = Dbg->db_undo_list->h_next;
    Dbg->db_in_undo = true;
    GtkTextBuffer *tbf =
        gtk_text_view_get_buffer(GTK_TEXT_VIEW(Dbg->wb_textarea));
    GtkTextIter istart;
    gtk_text_buffer_get_iter_at_offset(tbf, &istart, h->h_cpos);
    gtk_text_buffer_place_cursor(tbf, &istart);
    if (h->h_deletion)
        gtk_text_buffer_insert(tbf, &istart, h->h_text, -1);
    else {
        GtkTextIter iend;
        gtk_text_buffer_get_iter_at_offset(tbf, &iend,
            h->h_cpos + strlen(h->h_text));
        gtk_text_buffer_delete(tbf, &istart, &iend);
    }
    Dbg->db_in_undo = false;
    h->h_next = Dbg->db_redo_list;
    Dbg->db_redo_list = h;
    Dbg->check_sens();
}


// Private static GTK signal handler.
// Redo last undone operation.
//
void
sDbg::db_redo_proc(GtkWidget*, void*)
{
    if (!Dbg || !Dbg->db_redo_list)
        return;
    histlist *h = Dbg->db_redo_list;
    Dbg->db_redo_list = Dbg->db_redo_list->h_next;
    Dbg->db_in_undo = true;
    GtkTextBuffer *tbf =
        gtk_text_view_get_buffer(GTK_TEXT_VIEW(Dbg->wb_textarea));
    GtkTextIter istart;
    gtk_text_buffer_get_iter_at_offset(tbf, &istart, h->h_cpos);
    gtk_text_buffer_place_cursor(tbf, &istart);
    if (!h->h_deletion)
        gtk_text_buffer_insert(tbf, &istart, h->h_text, -1);
    else {
        GtkTextIter iend;
        gtk_text_buffer_get_iter_at_offset(tbf, &iend,
            h->h_cpos + strlen(h->h_text));
        gtk_text_buffer_delete(tbf, &istart, &iend);
    }
    Dbg->db_in_undo = false;
    h->h_next = Dbg->db_undo_list;
    Dbg->db_undo_list = h;
    Dbg->check_sens();
}


// Private static GTK signal handler.
// Kill selected text, copy to clipboard.
//
void
sDbg::db_cut_proc(GtkWidget*, void*)
{
    if (Dbg)
        text_cut_clipboard(Dbg->wb_textarea);
}


// Private static GTK signal handler.
// Copy selected text to the clipboard.
//
void
sDbg::db_copy_proc(GtkWidget*, void*)
{
    if (Dbg)
        text_copy_clipboard(Dbg->wb_textarea);
}


// Private static GTK signal handler.
// Insert clipboard text.
//
void
sDbg::db_paste_proc(GtkWidget*, void*)
{
    if (Dbg)
        text_paste_clipboard(Dbg->wb_textarea);
}


// Private static GTK signal handler.
// Insert primary selection text.
//
void
sDbg::db_paste_prim_proc(GtkWidget*, void*)
{
    if (Dbg) {
        GtkTextBuffer *tbf =
            gtk_text_view_get_buffer(GTK_TEXT_VIEW(Dbg->wb_textarea));
        GtkClipboard *cb = gtk_clipboard_get_for_display(
            gdk_display_get_default(), GDK_SELECTION_PRIMARY);
        gtk_text_buffer_paste_clipboard(tbf, cb, 0, true);
    }
}


// Private static GTK signal handler.
// Pop up the regular expression search dialog.
//
void
sDbg::db_search_proc(GtkWidget *caller, void*)
{
    if (Dbg) {
        if (!Dbg->db_search_pop)
            Dbg->db_search_pop = new GTKsearchPopup(caller, Dbg->wb_textarea,
                0, 0);
        if (GTKdev::GetStatus(caller))
            Dbg->db_search_pop->pop_up_search(MODE_ON);
        else
            Dbg->db_search_pop->pop_up_search(MODE_OFF);
    }
}


// Private static GTK signal handler.
// Font selector pop-up.
//
void
sDbg::db_font_proc(GtkWidget *caller, void*)
{
    if (Dbg) {
        if (GTKdev::GetStatus(caller))
            Dbg->PopUpFontSel(caller, GRloc(), MODE_ON, 0, 0, FNT_FIXED);
        else
            Dbg->PopUpFontSel(caller, GRloc(), MODE_OFF, 0, 0, FNT_FIXED);
    }
}


// Static function.
// Don't want to wait in the interrupt handler, take care of single
// step when user clicks on active line.
//
int
sDbg::db_step_idle(void*)
{
    if (Dbg)
        Dbg->step();
    return (false);
}


// Static function.
// This is called when the font is changed.
//
void
sDbg::db_font_changed()
{
    if (Dbg)
        Dbg->refresh(false, locPresent, true);
}


// Static function.
void
sDbg::db_cancel_proc(GtkWidget*, void*)
{
    if (Dbg) {
        if (Dbg->db_status == Eexecuting)
            EV()->InitCallback();  // force a return if pushed
        if (Dbg->check_save(CancelCode)) {
            if (Dbg->db_caller)
                GTKdev::Select(Dbg->db_caller);
            return;
        }
        XM()->PopUpDebug(0, MODE_OFF);
    }
}


// Static function.
// Tiggle configuration mode.
//
void
sDbg::db_mode_proc(GtkWidget*, void*)
{
    if (Dbg) {
        if (Dbg->db_mode == DBedit)
            Dbg->set_mode(DBrun);
        else
            Dbg->set_mode(DBedit);
    }
}


// Static function.
// This callback is called whenever the text is modified.
//
void
sDbg::db_change_proc(GtkWidget*, void*)
{
    if (Dbg->db_text_changed)
        return;
    Dbg->db_text_changed = true;
    gtk_widget_set_sensitive(Dbg->db_saveas, true);
    char buf[256];
    const char *fname = gtk_label_get_text(GTK_LABEL(Dbg->db_title));
    strcpy(buf, DEF_FILE);
    if (fname) {
        while (isspace(*fname))
            fname++;
    }
    if (fname && *fname) {
        strcpy(buf, fname);
        char *f = buf;
        while (*f && !isspace(*f))
            f++;
        *f++ = ' ';
        *f++ = ' ';
        strcpy(f, "(modified)");
    }
    gtk_label_set_text(GTK_LABEL(Dbg->db_title), buf);
    gtk_widget_set_sensitive(Dbg->db_modebtn, true);
}


// Static function.
// Handle key presses in the debugger window.  This provides additional
// accelerators for start/run/reset.
//
int
sDbg::db_key_dn_hdlr(GtkWidget*, GdkEvent *event, void*)
{
    if (!Dbg)
        return (false);
    if (Dbg->db_mode == DBedit) {
        // Eat the spacebar press, so that it doesn't "press" the
        // mode button.
        if (event->key.string) {
            if (*event->key.string == ' ')
                return (true);
        }
        return (false);
    }
    else if (Dbg->db_mode == DBrun) {
        if (event->key.string) {
            switch (*event->key.string) {
            case ' ':
            case 't':
                Dbg->step();
                return (true);
            case 'r':
                Dbg->run();
                return (true);
            case '\b':
            case 'e':
                Dbg->start();
                return (true);
            }
        }
    }
    return (false);
}


namespace {
    // Return true if s matches word followed by null or space.
    //
    bool
    isword(const char *s, const char *word)
    {
        while (*word) {
            if (*s != *word)
                return (false);
            s++;
            word++;
        }
        return (!*s || isspace(*s));
    }


    // Return true if line is in a function definition.
    //
    bool
    infunc(char *line)
    {
        char *t = strchr(line, '\n');
        while (t) {
            while (isspace(*t))
                t++;
            if (isword(t, "function"))
                break;
            if (isword(t, "endfunc"))
                return (true);
            t = strchr(t, '\n');
        }
        return (false);
    }
}


// Static function.
// Handle button presses in the text area.
//
int
sDbg::db_text_btn_hdlr(GtkWidget *caller, GdkEvent *event, void*)
{
    if (Dbg->db_mode == DBedit)
        return (false);
    if (event->type != GDK_BUTTON_PRESS)
        return (false);

    char *string = text_get_chars(caller, 0, -1);
    int x = (int)event->button.x;
    int y = (int)event->button.y;
    gtk_text_view_window_to_buffer_coords(GTK_TEXT_VIEW(caller),
        GTK_TEXT_WINDOW_WIDGET, x, y, &x, &y);
    GtkTextIter iline;
    gtk_text_view_get_line_at_y(GTK_TEXT_VIEW(caller), &iline, y, 0);
    y = gtk_text_iter_get_line(&iline);
    char *line_start = string + gtk_text_iter_get_offset(&iline);
    // line_start points to start of selected line, or 0
    if (!line_start) {
        text_select_range(caller, 0, 0);
        delete [] string;
        return (true);
    }

    char *s = line_start;
    while (isspace(*s) && *s != '\n')
        s++;

    text_select_range(caller, 0, 0);

    if (line_start > string) {
        if (*(line_start - 2) == '\\') {
            // continuation line, invalid target
            delete [] string;
            return (true);
        }
    }

    // don't allow line in a function block
    if (infunc(line_start)) {
        delete [] string;
        return (true);
    }

    char buf[16];
    strncpy(buf, s, 16);
    s = buf;
    delete [] string;

    if (!*s || *s == '\n' || *s == '#' || (*s == '/' && *(s+1) == '/'))
        // pointing beyond text, or blank/comment line
        return (true);
    // these lines are never visited
    if (isword(s, "end"))
        return (true);
    if (isword(s, "else"))
        return (true);
    if (isword(s, "function"))
        return (true);
    if (isword(s, "endfunc"))
        return (true);
    Dbg->breakpoint(y);
    return (true);
}


// Static function.
// General menu processing.
//
void
sDbg::db_action_proc(GtkWidget *caller, void *client_data)
{
    long code = (long)g_object_get_data(G_OBJECT(caller), MIDX);
    if (!Dbg)
        return;
    switch (code) {
    case CancelCode:
        db_cancel_proc(caller, client_data);
        break;
    case NewCode:
        if (Dbg->check_save(NewCode))
            return;
        histlist::destroy(Dbg->db_undo_list);
        Dbg->db_undo_list = 0;
        histlist::destroy(Dbg->db_redo_list);
        Dbg->db_redo_list = 0;
        Dbg->check_sens();
        text_set_chars(Dbg->wb_textarea, "");
        delete [] Dbg->db_file_path;
        Dbg->db_file_path = 0;
        SI()->Clear();
        SI()->Init();
        Dbg->breakpoint(-1);
        Dbg->db_line = 0;
        gtk_widget_set_sensitive(Dbg->db_saveas, false);
        gtk_label_set_text(GTK_LABEL(Dbg->db_title), DEF_FILE);
        Dbg->set_mode(DBedit);
        Dbg->db_text_changed = false;
        break;
    case LoadCode:
        if (Dbg->db_load_pop) {
            Dbg->db_load_pop->popdown();
            return;
        }
        if (Dbg->check_save(LoadCode))
            return;
        Dbg->db_load_pop = Dbg->PopUpEditString(0, GRloc(),
            "Enter script file name: ", Dbg->db_dropfile, db_open_cb,
                0, 214, 0);
        if (Dbg->db_load_pop)
            Dbg->db_load_pop->register_usrptr((void**)&Dbg->db_load_pop);
        break;
    case PrintCode:
        if (dbgHCcb.command)
            delete [] dbgHCcb.command;
        dbgHCcb.command = lstring::copy(GRappIf()->GetPrintCmd());
        Dbg->PopUpPrint(caller, &dbgHCcb, HCtext);
        break;
    case SaveAsCode:
        {
            if (Dbg->wb_input) {
                Dbg->wb_input->popdown();
                return;
            }
            Dbg->PopUpInput(0, Dbg->db_file_path, "Save File",
                db_do_saveas_proc, 0);
        }
        break;
    case CRLFcode:
#ifdef WIN32
        GTKdev::self()->SetCRLFtermination(GTKdev::GetStatus(caller));
#endif
        break;
    case RunCode:
        Dbg->set_mode(DBrun);
        Dbg->run();
        break;
    case StepCode:
        Dbg->set_mode(DBrun);
        Dbg->step();
        break;
    case StartCode:
        Dbg->set_mode(DBrun);
        Dbg->start();
        break;
    case MonitorCode:
        Dbg->monitor();
        break;
    case HelpCode:
        DSPmainWbag(PopUpHelp("xic:debug"))
        break;
    default:
        break;
    }
}


// Static function.
// Callback for the load command popup.
//
ESret
sDbg::db_open_cb(const char *namein, void*)
{
    delete [] Dbg->db_dropfile;
    Dbg->db_dropfile = 0;
    if (namein && *namein) {
        char *name = pathlist::expand_path(namein, false, true);
        char *t = strrchr(name, '.');
        if (!t || !lstring::cieq(t, SCR_SUFFIX)) {
            char *ct = new char[strlen(name) + strlen(SCR_SUFFIX) + 1];
            strcpy(ct, name);
            strcat(ct, SCR_SUFFIX);
            delete [] name;
            name = ct;
        }
        FILE *fp;
        if (lstring::strdirsep(name))
            fp = fopen(name, "r");
        else {
            // Search will check CWD first, then path.
            char *fpath;
            fp = pathlist::open_path_file(name,
                CDvdb()->getVariable(VA_ScriptPath), "r", &fpath, true);
            if (fpath) {
                delete [] name;
                name = fpath;
            }
        }
        if (fp) {
            fclose(fp);
            delete [] Dbg->db_file_path;
            Dbg->db_file_path = name;
            EV()->InitCallback();
            GTKpkg::self()->RegisterIdleProc(db_open_idle, 0);
            return (ESTR_DN);
        }
        else
            delete [] name;
    }
    Dbg->db_load_pop->update("No file found, try again: ", 0);
    return (ESTR_IGN);
}


// This need to be in an idle proc, otherwise problems if a script is
// already running.
//
int
sDbg::db_open_idle(void*)
{
    if (Dbg) {
        Dbg->db_file_ptr = fopen(Dbg->db_file_path, "r");
        if (Dbg->db_file_ptr) {
            histlist::destroy(Dbg->db_undo_list);
            Dbg->db_undo_list = 0;
            histlist::destroy(Dbg->db_redo_list);
            Dbg->db_redo_list = 0;
            Dbg->check_sens();
            Dbg->db_in_undo = true;
            Dbg->set_mode(DBedit);
            SI()->Clear();
            Dbg->breakpoint(-1);
            Dbg->db_line = 0;
            Dbg->start();
            Dbg->db_text_changed = false;
            Dbg->db_in_undo = false;
            gtk_widget_set_sensitive(Dbg->db_saveas, false);
            gtk_label_set_text(GTK_LABEL(Dbg->db_title), Dbg->db_file_path);
            fclose(Dbg->db_file_ptr);
            Dbg->db_file_ptr = 0;
            gtk_widget_set_sensitive(Dbg->db_modebtn, true);
        }
    }
    return (0);
}


// Static function.
// Callback passed to PopUpInput() to actually save the file.
//
void
sDbg::db_do_saveas_proc(const char *fnamein, void*)
{
    char *fname = pathlist::expand_path(fnamein, false, true);
    if (!Dbg->write_file(fname)) {
        delete [] fname;
        return;
    }
    Dbg->db_text_changed = false;
    gtk_widget_set_sensitive(Dbg->db_saveas, false);
    if (Dbg->wb_input)
        Dbg->wb_input->popdown();
    gtk_label_set_text(GTK_LABEL(Dbg->db_title), fname);
    delete [] fname;
}


// Static function.
// Receive drop data (a path name).
//
void
sDbg::db_drag_data_received(GtkWidget *caller, GdkDragContext *context, gint,
    gint, GtkSelectionData *data, guint, guint time, void*)
{
    if (gtk_selection_data_get_length(data) >= 0 &&
            gtk_selection_data_get_format(data) == 8 &&
            gtk_selection_data_get_data(data)) {
        if (Dbg) {
            char *src = (char*)gtk_selection_data_get_data(data);
            if (Dbg->db_mode == DBedit) {
                GdkModifierType mask;
                gdk_window_get_pointer(0, 0, 0, &mask);
                if (mask & GDK_CONTROL_MASK) {
                    // If we're pressing Ctrl, insert text at cursor.
                    int n = text_get_insertion_point(caller);
                    text_insert_chars_at_point(caller, 0, src, strlen(src), n);
                    gtk_drag_finish(context, true, false, time);
                    return;
                }
            }
            if (gtk_selection_data_get_target(data) ==
                    gdk_atom_intern("TWOSTRING", true)) {
                // Drops from content lists may be in the form
                // "fname_or_chd\ncellname".  Keep the filename.
                char *t = strchr(src, '\n');
                if (t)
                    *t = 0;
            }

            delete [] Dbg->db_dropfile;
            Dbg->db_dropfile = 0;
            Dbg->set_mode(DBedit);
            if (Dbg->db_load_pop)
                Dbg->db_load_pop->update(0, src);
            else {
                Dbg->db_dropfile = lstring::copy(src);
                db_action_proc(Dbg->db_load_btn, 0);
            }
            gtk_drag_finish(context, true, false, time);
            return;
        }
    }
    gtk_drag_finish(context, false, false, time);
}


void
sDbg::db_insert_text_proc(GtkTextBuffer*, GtkTextIter *istart, char *text,
    int len, void*)
{
    if (!Dbg || Dbg->db_in_undo || !len)
        return;
    int start = gtk_text_iter_get_offset(istart);
    char *ntext = new char[len+1];
    strncpy(ntext, text, len);
    ntext[len] = 0;
    Dbg->db_undo_list = new histlist(ntext, start, false, Dbg->db_undo_list);
    histlist::destroy(Dbg->db_redo_list);
    Dbg->db_redo_list = 0;
    Dbg->check_sens();
}


void
sDbg::db_delete_range_proc(GtkTextBuffer*, GtkTextIter *istart,
    GtkTextIter *iend, void*)
{
    if (!Dbg || Dbg->db_in_undo)
        return;
    int start = gtk_text_iter_get_offset(istart);
    char *s = lstring::tocpp(gtk_text_iter_get_text(istart, iend));
    Dbg->db_undo_list = new histlist(s, start, true, Dbg->db_undo_list);
    histlist::destroy(Dbg->db_redo_list);
    Dbg->db_redo_list = 0;
    Dbg->check_sens();
}


namespace {
    // Return a variable token.  The tokens can be separated by white
    // space and/or commas.  A token can have a following range
    // specification in square brackets that can contain commas and/or
    // white space.
    //
    char *
    getvar(const char **s)
    {
        char buf[512];
        int i = 0;
        if (s == 0 || *s == 0)
            return (0);
        while (isspace(**s) || **s == ',')
            (*s)++;
        if (!**s)
            return (0);
        while (**s && !isspace(**s) && **s != '[')
            buf[i++] = *(*s)++;
        if (**s == '[') {
            buf[i++] = *(*s)++;
            while (**s && **s != ']')
                buf[i++] = *(*s)++;
            if (**s == ']')
                buf[i++] = *(*s)++;
        }
        buf[i] = '\0';
        while (isspace(**s) || **s == ',')
            (*s)++;
        return (lstring::copy(buf));
    }
}


// Static function.
// Return a list of the tokens in str.
//
stringlist *
sDbg::db_mklist(const char *str)
{
    if (!str)
        return (0);
    while (isspace(*str))
        str++;
    stringlist *wl = 0, *wl0 = 0;
    if (!strcmp(str, "all") || ((*str == '*' || *str == '.') && !*(str+1))) {
        for (Variable *v = SIparse()->getVariables(); v; v = v->next) {
            if (!wl0)
                wl = wl0 = new stringlist;
            else {
                wl->next = new stringlist;
                wl = wl->next;
            }
            wl->string = lstring::copy(v->name);
        }
    }
    else {
        char *tok;
        while ((tok = getvar(&str)) != 0) {
            if (!wl0)
                wl = wl0 = new stringlist(tok, 0);
            else {
                wl->next = new stringlist(tok, 0);
                wl = wl->next;
            }
        }
    }
    return (wl0);
}
// End of sDbg functions.


//
// The variables monitor.
//

sDbV::sDbV(void *p)
{
    dv_no_select = false;
    dv_pointer = p;
    dv_popup = gtk_NewPopup(0, "Variables", dv_cancel_proc, this);

    GtkWidget *form = gtk_table_new(1, 2, false);
    gtk_widget_show(form);
    gtk_container_add(GTK_CONTAINER(dv_popup), form);

    //
    // variable listing text
    //
    GtkWidget *swin = gtk_scrolled_window_new(0, 0);
    gtk_widget_show(swin);
    gtk_widget_set_size_request(swin, 220, 200);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin),
        GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_set_border_width(GTK_CONTAINER(swin), 2);

    const char *tbuf[2];
    tbuf[0] = "Variable";
    tbuf[1] = "Value";
    GtkListStore *store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
    dv_list = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
    gtk_widget_show(dv_list);
    gtk_tree_view_set_enable_search(GTK_TREE_VIEW(dv_list), false);
    GtkCellRenderer *rnd = gtk_cell_renderer_text_new();
    GtkTreeViewColumn *tvcol = gtk_tree_view_column_new_with_attributes(
        tbuf[0], rnd, "text", 0, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(dv_list), tvcol);
    tvcol = gtk_tree_view_column_new_with_attributes(
        tbuf[1], rnd, "text", 1, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(dv_list), tvcol);

    GtkTreeSelection *sel =
        gtk_tree_view_get_selection(GTK_TREE_VIEW(dv_list));
    gtk_tree_selection_set_select_function(sel, dv_select_proc, 0, 0);
    // TreeView bug hack, see note with handlers.   
    g_signal_connect(G_OBJECT(dv_list), "focus",
        G_CALLBACK(dv_focus_proc), this);

    gtk_container_add(GTK_CONTAINER(swin), dv_list);

    // Set up font and tracking.
    GTKfont::setupFont(dv_list, FNT_PROP, true);

    gtk_table_attach(GTK_TABLE(form), swin, 0, 1, 0, 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);

    //
    // Dismiss button
    //
    GtkWidget *button = gtk_button_new_with_label("Dismiss");
    gtk_widget_set_name(button, "Dismiss");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(dv_cancel_proc), this);

    gtk_table_attach(GTK_TABLE(form), button, 0, 1, 1, 2,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    gtk_window_set_focus(GTK_WINDOW(dv_popup), button);
}


sDbV::~sDbV()
{
    if (dv_pointer)
        *(void**)dv_pointer = 0;
    if (dv_popup) {
        g_signal_handlers_disconnect_by_func(G_OBJECT(dv_popup),
            (gpointer)dv_cancel_proc, this);
        gtk_widget_destroy(dv_popup);
    }
}


// Update the variables listing.
//
void
sDbV::update(stringlist *vars)
{
    if (!vars) {
        popdown();
        return;
    }
    GtkListStore *store =
        GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(dv_list)));
    gtk_list_store_clear(store);

    GtkTreeIter iter;
    char buf[256];
    for (stringlist *wl = vars; wl; wl = wl->next) {
        SIparse()->printVar(wl->string, buf);
        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter, 0, wl->string, 1, buf, -1);
    }
}


// Static function.
// Cancel procedure for the variables monitor popup.
//
void
sDbV::dv_cancel_proc(GtkWidget*, void *client_data)
{
    sDbV *p = static_cast<sDbV*>(client_data);
    if (p)
        p->popdown();
}


// Static function.
// Selection callback for the list.  Note that we return false which
// prevents actually accepting the selection, so this is just a fancy
// button-press handler.
//
int
sDbV::dv_select_proc(GtkTreeSelection*, GtkTreeModel *store,
    GtkTreePath *path, int issel, void*)
{
    if (Dbg && Dbg->db_vars_pop) {
        if (issel)
            return (true);
        if (!Dbg->db_vars_pop->dv_no_select) {
            Dbg->db_vars_pop->dv_no_select = false;
            return (false);
        }
        char *var = 0, *val = 0;
        GtkTreeIter iter;
        if (gtk_tree_model_get_iter(store, &iter, path))
            gtk_tree_model_get(store, &iter, 0, &var, 1, &val, -1);
        if (var && val) {
            char buf[128];
            snprintf(buf, sizeof(buf), "assign %s = ", var);

            bool busy;
            const char *in = Dbg->var_prompt(val, buf, &busy);
            if (busy) {
                free(var);
                free(val);
                return (false);
            }

            if (!in) {
                PL()->ErasePrompt();
                free(var);
                free(val);
                return (false);
            }
            char *s = SIparse()->setVar(var, in);
            if (s) {
                PL()->ShowPrompt(s);
                delete [] s;
                free(var);
                free(val);
                return (false);
            }
            Dbg->update_variables();
            PL()->ErasePrompt();
        }
        free(var);
        free(val);
    }
    return (false);
}


// Static function.
// This handler is a hack to avoid a GtkTreeWidget defect:  when focus
// is taken and there are no selections, the 0'th row will be
// selected.  There seems to be no way to avoid this other than a hack
// like this one.  We set a flag to lock out selection changes in this
// case.
//
bool
sDbV::dv_focus_proc(GtkWidget*, GdkEvent*, void*)
{
    if (Dbg && Dbg->db_vars_pop) {
        GtkTreeSelection *sel = gtk_tree_view_get_selection(
            GTK_TREE_VIEW(Dbg->db_vars_pop->dv_list));
        // If nothing selected set the flag.
        if (!gtk_tree_selection_get_selected(sel, 0, 0))
            Dbg->db_vars_pop->dv_no_select = true;
    }
    return (false);
}
 


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

#include "config.h"
#include "gtkinterf/gtkinterf.h"
#include "gtkhelp.h"
#include "gtkinterf/gtkutil.h"
#include "gtkinterf/gtkfont.h"
#include "gtkinterf/gtkfile.h"
#include "gtkviewer.h"
#include "help/help_startup.h"
#include "help/help_cache.h"
#include "help/help_topic.h"
#include "httpget/transact.h"
#include "htm/htm_image.h"
#include "htm/htm_callback.h"
#include "htm/htm_frame.h"
#include "htm/htm_form.h"
#include "miscutil/pathlist.h"
#include "miscutil/filestat.h"
#include "miscutil/timer.h"
#include "miscutil/proxy.h"

#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/stat.h>
#include <errno.h>
#include <signal.h>
#ifdef HAVE_PWD_H
#include <pwd.h>
#include <sys/wait.h>
#endif
#include <fcntl.h>
#ifdef WIN32
#include <windows.h>
#endif

#include <gdk/gdkkeysyms.h>
#include <gdk/gdkprivate.h>


/*=======================================================================
 =
 =  HTML Viewer for WWW and Help System
 =
 =======================================================================*/

// Help keywords used in this file
//  helpview

// Architecture note
// Each window has a corresponding "parent" topic struct linked from
// HLP()->context()->TopList.  In the parents, the context field
// points to a GTKbag which contains the window information.  Each
// new page displayed in the window has a corresponding topic struct
// linked from lastborn in the parent, and has the parent field set to
// point to the parent (in the TopList).  Topics linked to lastborn
// do not have the context field set.

#define HLP_DEF_WIDTH 730
#define HLP_DEF_HEIGHT 400

namespace {
    // for hardcopies
    HCcb hlpHCcb =
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

    // Default color selection pop-up.
    struct sClr : public GTKbag
    {
        sClr(GRobject caller);
        ~sClr();

        void update();

    private:
        static void clr_cancel_proc(GtkWidget*, void*);
        static void clr_action_proc(GtkWidget*, void*);
        static void clr_list_callback(const char*, void*);

        GRobject clr_caller;
        GtkWidget *clr_bgclr;
        GtkWidget *clr_bgimg;
        GtkWidget *clr_text;
        GtkWidget *clr_link;
        GtkWidget *clr_vislink;
        GtkWidget *clr_actfglink;
        GtkWidget *clr_imgmap;
        GtkWidget *clr_sel;
        GtkWidget *clr_listbtn;
        GRlistPopup *clr_listpop;

        char *clr_selection;  // GTK-1 only
    };
    sClr *Clr;

    // XPM
    const char * forward_xpm[] = {
    "32 25 4 1",
    " 	c none",
    ".	c black",
    "x	c blue",
    "+  c white",
    "                                ",
    "               +                ",
    "               +.               ",
    "               +x.              ",
    "               +xx.             ",
    "               +xxx.            ",
    "               +xxxx.           ",
    "               +xxxxx.          ",
    "      +.........xxxxxx.         ",
    "      +xxxxxxxxxxxxxxxx.        ",
    "      +xxxxxxxxxxxxxxxxx.       ",
    "      +xxxxxxxxxxxxxxxxxx.      ",
    "      +xxxxxxxxxxxxxxxxxxx.     ",
    "      +xxxxxxxxxxxxxxxxxx.      ",
    "      +xxxxxxxxxxxxxxxxx.       ",
    "      +xxxxxxxxxxxxxxxx.        ",
    "      +.........xxxxxx.         ",
    "               +xxxxx.          ",
    "               +xxxx.           ",
    "               +xxx.            ",
    "               +xx.             ",
    "               +x.              ",
    "               +.               ",
    "               +                ",
    "                                "};

    // XPM
    const char * backward_xpm[] = {
    "32 25 4 1",
    " 	c none",
    ".	c black",
    "x	c blue",
    "+  c white",
    "                                ",
    "                +               ",
    "               .+               ",
    "              .x+               ",
    "             .xx+               ",
    "            .xxx+               ",
    "           .xxxx+               ",
    "          .xxxxx+               ",
    "         .xxxxxx.........+      ",
    "        .xxxxxxxxxxxxxxxx+      ",
    "       .xxxxxxxxxxxxxxxxx+      ",
    "      .xxxxxxxxxxxxxxxxxx+      ",
    "     .xxxxxxxxxxxxxxxxxxx+      ",
    "      .xxxxxxxxxxxxxxxxxx+      ",
    "       .xxxxxxxxxxxxxxxxx+      ",
    "        .xxxxxxxxxxxxxxxx+      ",
    "         .xxxxxx.........+      ",
    "          .xxxxx+               ",
    "           .xxxx+               ",
    "            .xxx+               ",
    "             .xx+               ",
    "              .x+               ",
    "               .+               ",
    "                +               ",
    "                                "};

    // XPM
    const char * stop_xpm[] = {
    "32 26 4 1",
    " 	c none",
    ".	c red",
    "x  c white",
    "+  c black",
    "                                ",
    "         xxxxxxxxxx             ",
    "        x..........x            ",
    "       x............x           ",
    "      x..............x          ",
    "     x................x         ",
    "    x..................x        ",
    "   x....................x       ",
    "  x......................+      ",
    "  x......................+      ",
    "  x....x..xxx..xx..xxx...+      ",
    "  x...x.x..x..x..x.x.x...+      ",
    "  x...x....x..x..x.xxx...+      ",
    "  x....x...x..x..x.x.....+      ",
    "  x.....x..x..x..x.x.....+      ",
    "  x...x.x..x..x..x.x.....+      ",
    "  x....x...x...xx..x.....+      ",
    "  x......................+      ",
    "   +....................+       ",
    "    +..................+        ",
    "     +................+         ",
    "      +..............+          ",
    "       +............+           ",
    "        +..........+            ",
    "         ++++++++++             ",
    "                                "};

    // Drag/drop info
    GtkTargetEntry target_table[] = {
        { (char*)"STRING",      0, 0 },
        { (char*)"text/plain",  0, 1 },
        { (char*)"text/url",    0, 2 }
    };
    guint n_targets = sizeof(target_table) / sizeof(target_table[0]);

    const char *MIDX = "midx";
}


// In order to have the keyword entry popup pop down without leaving
// a "hole" in the parent during the search time, the search is placed
// in a timeout procedure.
//
struct ntop
{
    ntop(const char *k, HLPtopic *p) { kw = lstring::copy(k); parent = p; }
    ~ntop() { delete [] kw; }

    char *kw;
    HLPtopic *parent;
};


//-----------------------------------------------------------------------------
// Main entry - GTKbag method

// Top level help popup call, takes care of accessing the database.
// Return false if the topic is not found.
//
bool
GTKbag::PopUpHelp(const char *wordin)
{
    if (!HLP()->get_path(0)) {
        PopUpErr(MODE_ON, "Error: no path to database.");
        HLP()->context()->quitHelp();
        return (false);
    }
    char buf[256];
    buf[0] = 0;
    if (wordin && *wordin) {
        strcpy(buf, wordin);
        char *s = buf + strlen(buf) - 1;
        while (s >= buf && isspace(*s))
            *s-- = '\0';
    }

    HLPtopic *top = 0;
    if (HLP()->context()->resolveKeyword(buf, &top, 0, 0, 0, false, true))
        return (false);
    if (!top) {
        if (*buf) {
            sLstr lstr;
            lstr.add("Error: No such topic: ");
            lstr.add(buf);
            lstr.add_c('\n');
            PopUpErr(MODE_ON, lstr.string());
        }
        else
            PopUpErr(MODE_ON, "Error: no top level topic\n");
        return (false);
    }
    top->show_in_window();
    return (true);
}


//-----------------------------------------------------------------------------
// Idle function controller for progressive image loading.

namespace {
    int queue_idle(void*);
}

namespace gtkinterf {
    struct gtkQueueLoop : public QueueLoop
    {
        gtkQueueLoop() { id = 0; }

        void start();
        void suspend();
        void resume();

    private:
        unsigned int id;
    };


    void
    gtkQueueLoop::start()
    {
        if (!id)
            id = g_idle_add((GSourceFunc)queue_idle, this);
    }


    void
    gtkQueueLoop::suspend()
    {
        if (id) {
            g_source_remove(id);
            id = 0;
        }
    }


    void
    gtkQueueLoop::resume()
    {
        if (!id)
            id = g_idle_add((GSourceFunc)queue_idle, this);
    }
}


namespace {
    int
    queue_idle(void *qp)
    {
       bool b = HLP()->context()->processList();
       if (!b)
           // clear id
           ((gtkQueueLoop*)qp)->suspend();
       return (b);
    }


    // queue_timer
    gtkQueueLoop _queue_timer_;
}

QueueLoop &HLPcontext::hcxImageQueueLoop = _queue_timer_;

// Menu dispatch codes
enum { HA_NIL, HA_CANCEL, HA_QUIT, HA_OPEN, HA_FILE, HA_BACK,
    HA_FORWARD, HA_DUMPCFG, HA_PROXY, HA_SEARCH, HA_FIND, HA_SAVE,
    HA_PRINT, HA_RELOAD, HA_ISO8859, HA_MKFIFO, HA_COLORS, HA_FONT,
    HA_NOCACHE, HA_CLRCACHE, HA_LDCACHE, HA_SHCACHE, HA_NOCKS,
    HA_NOIMG, HA_SYIMG, HA_DLIMG, HA_PGIMG, HA_APLN, HA_ABUT, HA_AUND,
    HA_HLITE, HA_WARN, HA_FREEZ, HA_COMM, HA_BMADD, HA_BMDEL, HA_HELP };

GtkWindow *GTKhelpPopup::h_transient_for = 0;

//-----------------------------------------------------------------------------
// Constructor/Destructor for main widget class

GTKhelpPopup::GTKhelpPopup(bool has_menu, int xpos, int ypos,
    GtkWidget *parent_shell)
{
    // If has_menu is false, the widget will not have the menu or the
    // status bar visible (intended for use as frame child).
    h_parent = parent_shell;
    h_menubar = 0;
    h_file_menu = 0;
    h_options_menu = 0;
    h_bookmarks_menu = 0;
    h_viewer = 0;
    h_params = 0;
    h_root_topic = 0;
    h_cur_topic = 0;
    h_stop_btn_pressed = false;
    h_is_frame = !has_menu;
    h_cache_list = 0;
    h_fsels[0] = h_fsels[1] = h_fsels[2] = h_fsels[3] = 0;
    h_find_text = 0;
    h_frame_array = 0;
    h_frame_array_size = 0;
    h_frame_parent = 0;
    h_frame_name = 0;
    h_fifo_name = 0;
    h_fifo_fd = -1;
    h_fifo_tid = 0;
#ifdef WIN32
    h_fifo_pipe = 0;
    h_fifo_tfiles = 0;
#endif

    GtkWidget *form = 0;
    if (!h_is_frame) {
        h_params = new HLPparams(HLP()->no_file_fonts());
        wb_shell = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_window_set_wmclass(GTK_WINDOW(wb_shell), "Mozy", "mozy");

        char buf[128];
        sprintf(buf, "%s -- Whiteley Research Inc.",  HLP()->get_name());
        gtk_window_set_title(GTK_WINDOW(wb_shell), buf);

        GtkWidget *topw = 0;
        if (parent_shell)
            topw = parent_shell;
        else if (h_transient_for)
            topw = GTK_WIDGET(h_transient_for);
        if (topw) {
            gtk_window_set_transient_for(GTK_WINDOW(wb_shell),
                GTK_WINDOW(topw));
        }

        // set position
        if (xpos > 0 && ypos > 0) {
            int x=0, y=0;
            if (topw)
                MonitorGeom(topw, &x, &y);
            gtk_window_move(GTK_WINDOW(wb_shell), xpos + x, ypos + y);
        }

        gtk_window_set_default_size(GTK_WINDOW(wb_shell), HLP_DEF_WIDTH,
            HLP_DEF_HEIGHT);

        wb_sens_set = h_sens_set;

        gtk_widget_add_events(wb_shell, GDK_VISIBILITY_NOTIFY_MASK);
        g_signal_connect(G_OBJECT(wb_shell), "visibility-notify-event",
            G_CALLBACK(ToTop), 0);
        gtk_widget_add_events(wb_shell, GDK_BUTTON_PRESS_MASK);
        g_signal_connect_after(G_OBJECT(wb_shell), "button-press-event",
            G_CALLBACK(Btn1MoveHdlr), 0);
        g_object_set_data(G_OBJECT(wb_shell), MIDX, (gpointer)HA_CANCEL);
        g_signal_connect(G_OBJECT(wb_shell), "destroy",
            G_CALLBACK(h_menu_hdlr), this);

        // Redirect WM_DESTROY
        g_signal_connect(G_OBJECT(wb_shell), "delete-event",
            G_CALLBACK(h_destroy_hdlr), this);

        form = gtk_table_new(1, 4, false);
        gtk_widget_show(form);
        gtk_container_add(GTK_CONTAINER(wb_shell), form);

        GtkWidget *hbox = gtk_hbox_new(false, 2);
        gtk_widget_show(hbox);

        GtkWidget *button = new_pixmap_button(backward_xpm, 0, false);
        gtk_widget_set_name(button, "Back");
        gtk_widget_show(button);
        g_object_set_data(G_OBJECT(button), MIDX, (gpointer)HA_BACK);
        g_signal_connect(G_OBJECT(button), "clicked",
            G_CALLBACK(h_menu_hdlr), this);
        gtk_widget_set_sensitive(button, false);
        g_object_set_data(G_OBJECT(wb_shell), "back", button);
        gtk_box_pack_start(GTK_BOX(hbox), button, false, false, 0);

        button = new_pixmap_button(forward_xpm, 0, false);
        gtk_widget_set_name(button, "Forward");
        gtk_widget_show(button);
        g_object_set_data(G_OBJECT(button), MIDX, (gpointer)HA_FORWARD);
        g_signal_connect(G_OBJECT(button), "clicked",
            G_CALLBACK(h_menu_hdlr), this);
        gtk_widget_set_sensitive(button, false);
        g_object_set_data(G_OBJECT(wb_shell), "forward", button);
        gtk_box_pack_start(GTK_BOX(hbox), button, false, false, 0);

        //
        // Menus.
        //
        GtkAccelGroup *accel_group = gtk_accel_group_new();
        gtk_window_add_accel_group(GTK_WINDOW(wb_shell), accel_group);
        h_menubar = gtk_menu_bar_new();
        g_object_set_data(G_OBJECT(wb_shell), "menubar", h_menubar);
        gtk_widget_show(h_menubar);

        gtk_box_pack_start(GTK_BOX(hbox), h_menubar, true, true, 0);

        button = new_pixmap_button(stop_xpm, 0, false);
        gtk_widget_set_name(button, "Stop");
        gtk_widget_show(button);
        g_signal_connect(G_OBJECT(button), "clicked",
            G_CALLBACK(h_stop_proc), this);
        gtk_widget_set_sensitive(button, false);
        g_object_set_data(G_OBJECT(wb_shell), "stopbtn", button);
        gtk_box_pack_end(GTK_BOX(hbox), button, false, false, 0);

        gtk_table_attach(GTK_TABLE(form), hbox, 0, 1, 0, 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);

        GtkWidget *item;

        // File menu.
        item = gtk_menu_item_new_with_mnemonic("_File");
        gtk_widget_set_name(item, "File");
        gtk_widget_show(item);
        gtk_menu_shell_append(GTK_MENU_SHELL(h_menubar), item);
        h_file_menu = gtk_menu_new();
        gtk_widget_show(h_file_menu);
        gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), h_file_menu);

        // "/File/_Open", "<control>O", h_menu_hdlr, HA_OPEN, 0
        item = gtk_menu_item_new_with_mnemonic("_Open");
        gtk_widget_show(item);
        g_object_set_data(G_OBJECT(item), MIDX, (gpointer)HA_OPEN);
        gtk_menu_shell_append(GTK_MENU_SHELL(h_file_menu), item);
        g_object_set_data(G_OBJECT(wb_shell), "open", item);
        g_signal_connect(G_OBJECT(item), "activate",
            G_CALLBACK(h_menu_hdlr), this);
        gtk_widget_add_accelerator(item, "activate", accel_group, GDK_KEY_o,
            GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

        // "/File/Open _File", "<control>F", h_menu_hdlr, HA_FILE, 0
        item = gtk_menu_item_new_with_mnemonic("Open _File");
        gtk_widget_show(item);
        g_object_set_data(G_OBJECT(item), MIDX, (gpointer)HA_FILE);
        gtk_menu_shell_append(GTK_MENU_SHELL(h_file_menu), item);
        g_signal_connect(G_OBJECT(item), "activate",
            G_CALLBACK(h_menu_hdlr), this);
        gtk_widget_add_accelerator(item, "activate", accel_group, GDK_KEY_f,
            GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

        // "/File/_Save", "<control>S", h_menu_hdlr, HA_SAVE, 0
        item = gtk_menu_item_new_with_mnemonic("_Save");
        gtk_widget_show(item);
        g_object_set_data(G_OBJECT(item), MIDX, (gpointer)HA_SAVE);
        gtk_menu_shell_append(GTK_MENU_SHELL(h_file_menu), item);
        g_object_set_data(G_OBJECT(wb_shell), "save", item);
        g_signal_connect(G_OBJECT(item), "activate",
            G_CALLBACK(h_menu_hdlr), this);
        gtk_widget_add_accelerator(item, "activate", accel_group, GDK_KEY_s,
            GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

        // "/File/_Print", "<control>P", h_menu_hdlr, HA_PRINT, 0
        item = gtk_menu_item_new_with_mnemonic("_Print");
        gtk_widget_show(item);
        g_object_set_data(G_OBJECT(item), MIDX, (gpointer)HA_PRINT);
        gtk_menu_shell_append(GTK_MENU_SHELL(h_file_menu), item);
        g_signal_connect(G_OBJECT(item), "activate",
            G_CALLBACK(h_menu_hdlr), this);
        gtk_widget_add_accelerator(item, "activate", accel_group, GDK_KEY_p,
            GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

        // "/File/_Reload", "<control>R", h_menu_hdlr, HA_RELOAD, 0
        item = gtk_menu_item_new_with_mnemonic("_Reload");
        gtk_widget_show(item);
        g_object_set_data(G_OBJECT(item), MIDX, (gpointer)HA_RELOAD);
        gtk_menu_shell_append(GTK_MENU_SHELL(h_file_menu), item);
        g_signal_connect(G_OBJECT(item), "activate",
            G_CALLBACK(h_menu_hdlr), this);
        gtk_widget_add_accelerator(item, "activate", accel_group, GDK_KEY_r,
            GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

        // "/File/Old _Charset", 0, h_menu_hdlr, HA_ISO8859, "<CheckItem>"
        item = gtk_check_menu_item_new_with_mnemonic("Old _Charset");
        gtk_widget_show(item);
        g_object_set_data(G_OBJECT(item), MIDX, (gpointer)HA_ISO8859);
        gtk_menu_shell_append(GTK_MENU_SHELL(h_file_menu), item);
        g_signal_connect(G_OBJECT(item), "activate",
            G_CALLBACK(h_menu_hdlr), this);

        // "/File/_Make FIFO", "<control>M", h_menu_hdlr, HA_MKFIFO,
        //  "<CheckItem>"
        item = gtk_check_menu_item_new_with_mnemonic("_Make FIFO");
        gtk_widget_show(item);
        g_object_set_data(G_OBJECT(item), MIDX, (gpointer)HA_MKFIFO);
        gtk_menu_shell_append(GTK_MENU_SHELL(h_file_menu), item);
        g_signal_connect(G_OBJECT(item), "activate",
            G_CALLBACK(h_menu_hdlr), this);
        gtk_widget_add_accelerator(item, "activate", accel_group, GDK_KEY_m,
            GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
        if (HLP()->fifo_start()) {
            GRX->SetStatus(item, true);
            register_fifo(0);
        }

        item = gtk_separator_menu_item_new();
        gtk_widget_show(item);
        gtk_menu_shell_append(GTK_MENU_SHELL(h_file_menu), item);

        // "/File/_Quit", "<control>Q", h_menu_hdlr, HA_QUIT, 0
        item = gtk_menu_item_new_with_mnemonic("_Quit");
        gtk_widget_show(item);
        g_object_set_data(G_OBJECT(item), MIDX, (gpointer)HA_QUIT);
        gtk_menu_shell_append(GTK_MENU_SHELL(h_file_menu), item);
        g_signal_connect(G_OBJECT(item), "activate",
            G_CALLBACK(h_menu_hdlr), this);
        gtk_widget_add_accelerator(item, "activate", accel_group, GDK_KEY_q,
            GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

        // Options menu.
        item = gtk_menu_item_new_with_mnemonic("_Options");
        gtk_widget_set_name(item, "Options");
        gtk_widget_show(item);
        gtk_menu_shell_append(GTK_MENU_SHELL(h_menubar), item);
        h_options_menu = gtk_menu_new();
        gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), h_options_menu);
        gtk_widget_show(h_options_menu);

        // "/Options/Save Config", 0, h_menu_hdlr, HA_DUMPCFG, 0
        item = gtk_menu_item_new_with_mnemonic("Save Config");
        gtk_widget_show(item);
        g_object_set_data(G_OBJECT(item), MIDX, (gpointer)HA_DUMPCFG);
        gtk_menu_shell_append(GTK_MENU_SHELL(h_options_menu), item);
        g_signal_connect(G_OBJECT(item), "activate",
            G_CALLBACK(h_menu_hdlr), this);

        // "/Options/Set Proxy", 0, h_menu_hdlr, HA_PROXY, 0
        item = gtk_menu_item_new_with_mnemonic("Set Proxy");
        gtk_widget_show(item);
        g_object_set_data(G_OBJECT(item), MIDX, (gpointer)HA_PROXY);
        gtk_menu_shell_append(GTK_MENU_SHELL(h_options_menu), item);
        g_signal_connect(G_OBJECT(item), "activate",
            G_CALLBACK(h_menu_hdlr), this);

#if defined(HAVE_REGEX_H) || defined(HAVE_REGEXP_H) || defined(HAVE_RE_COMP)

        // "/Options/_Search Database", "<Alt>S", h_menu_hdlr) HA_SEARCH, 0
        item = gtk_menu_item_new_with_mnemonic("_Search Database");
        gtk_widget_show(item);
        g_object_set_data(G_OBJECT(item), MIDX, (gpointer)HA_SEARCH);
        gtk_menu_shell_append(GTK_MENU_SHELL(h_options_menu), item);
        g_object_set_data(G_OBJECT(wb_shell), "search", item);
        g_signal_connect(G_OBJECT(item), "activate",
            G_CALLBACK(h_menu_hdlr), this);
        // gtk_widget_add_accelerator(item, "activate", accel_group, GDK_KEY_s,
        //     GDK_ALT_MASK, GTK_ACCEL_VISIBLE);

        // "/Options/Find _Text", "<Alt>T", h_menu_hdlr, HA_FIND, "<CheckItem>"
        item = gtk_check_menu_item_new_with_mnemonic("Find _Text");
        gtk_widget_show(item);
        g_object_set_data(G_OBJECT(item), MIDX, (gpointer)HA_FIND);
        gtk_menu_shell_append(GTK_MENU_SHELL(h_options_menu), item);
        g_signal_connect(G_OBJECT(item), "activate",
            G_CALLBACK(h_menu_hdlr), this);
        // gtk_widget_add_accelerator(item, "activate", accel_group, GDK_KEY_t,
        //     GDK_ALT_MASK, GTK_ACCEL_VISIBLE);

        // "/Options/Default Colors", 0, h_menu_hdlr, HA_COLORS, "<CheckItem>"
        item = gtk_check_menu_item_new_with_mnemonic("Defult Colors");
        gtk_widget_show(item);
        g_object_set_data(G_OBJECT(item), MIDX, (gpointer)HA_COLORS);
        gtk_menu_shell_append(GTK_MENU_SHELL(h_options_menu), item);
        g_signal_connect(G_OBJECT(item), "activate",
            G_CALLBACK(h_menu_hdlr), this);

        // "/Options/Set _Font", 0, h_menu_hdlr, HA_FONT, "<CheckItem>"
        item = gtk_check_menu_item_new_with_mnemonic("Set _Font");
        gtk_widget_show(item);
        g_object_set_data(G_OBJECT(item), MIDX, (gpointer)HA_FONT);
        gtk_menu_shell_append(GTK_MENU_SHELL(h_options_menu), item);
        g_signal_connect(G_OBJECT(item), "activate",
            G_CALLBACK(h_menu_hdlr), this);

        // "/Options/_Don't Cache", 0, h_menu_hdlr, HA_NOCACHE, "<CheckItem>"),
        item = gtk_check_menu_item_new_with_mnemonic("_Don't Cache");
        gtk_widget_show(item);
        g_object_set_data(G_OBJECT(item), MIDX, (gpointer)HA_NOCACHE);
        gtk_menu_shell_append(GTK_MENU_SHELL(h_options_menu), item);
        g_signal_connect(G_OBJECT(item), "activate",
            G_CALLBACK(h_menu_hdlr), this);

        // "/Options/_Clear Cache", 0, h_menu_hdlr, HA_CLRCACHE, 0
        item = gtk_menu_item_new_with_mnemonic("_Clear Cache");
        gtk_widget_show(item);
        g_object_set_data(G_OBJECT(item), MIDX, (gpointer)HA_CLRCACHE);
        gtk_menu_shell_append(GTK_MENU_SHELL(h_options_menu), item);
        g_signal_connect(G_OBJECT(item), "activate",
            G_CALLBACK(h_menu_hdlr), this);

        // "/Options/_Reload Cache", 0, h_menu_hdlr, HA_LDCACHE, 0
        item = gtk_menu_item_new_with_mnemonic("_Reload Cache");
        gtk_widget_show(item);
        g_object_set_data(G_OBJECT(item), MIDX, (gpointer)HA_LDCACHE);
        gtk_menu_shell_append(GTK_MENU_SHELL(h_options_menu), item);
        g_signal_connect(G_OBJECT(item), "activate",
            G_CALLBACK(h_menu_hdlr), this);

        // "/Options/Show Cache", 0, h_menu_hdlr, HA_SHCACHE, 0
        item = gtk_menu_item_new_with_mnemonic("Show Cache");
        gtk_widget_show(item);
        g_object_set_data(G_OBJECT(item), MIDX, (gpointer)HA_SHCACHE);
        gtk_menu_shell_append(GTK_MENU_SHELL(h_options_menu), item);
        g_signal_connect(G_OBJECT(item), "activate",
            G_CALLBACK(h_menu_hdlr), this);

        item = gtk_separator_menu_item_new();
        gtk_widget_show(item);
        gtk_menu_shell_append(GTK_MENU_SHELL(h_options_menu), item);

        // "/Options/No Cookies", 0, h_menu_hdlr, HA_NOCKS, "<CheckItem>"),
        item = gtk_check_menu_item_new_with_mnemonic("No Cookies");
        gtk_widget_show(item);
        g_object_set_data(G_OBJECT(item), MIDX, (gpointer)HA_NOCKS);
        gtk_menu_shell_append(GTK_MENU_SHELL(h_options_menu), item);
        g_signal_connect(G_OBJECT(item), "activate",
            G_CALLBACK(h_menu_hdlr), this);

        // "/Options/No Images", 0, h_menu_hdlr, HA_NOIMG, "<RadioItem>"
        GSList *group = 0;
        item = gtk_radio_menu_item_new_with_mnemonic(group, "No Images");
        group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(item));
        gtk_widget_show(item);
        g_object_set_data(G_OBJECT(item), MIDX, (gpointer)HA_NOIMG);
        gtk_menu_shell_append(GTK_MENU_SHELL(h_options_menu), item);
        g_signal_connect(G_OBJECT(item), "activate",
            G_CALLBACK(h_menu_hdlr), this);

        // "/Options/Sync Images", 0, h_menu_hdlr, HA_SYIMG,
        //  "/Options/No Images"
        item = gtk_radio_menu_item_new_with_mnemonic(group, "Sync Images");
        group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(item));
        gtk_widget_show(item);
        g_object_set_data(G_OBJECT(item), MIDX, (gpointer)HA_SYIMG);
        gtk_menu_shell_append(GTK_MENU_SHELL(h_options_menu), item);
        g_signal_connect(G_OBJECT(item), "activate",
            G_CALLBACK(h_menu_hdlr), this);

        // "/Options/Delayed Images", 0, h_menu_hdlr, HA_DLIMG,
        //  "/Options/Sync Images"
        item = gtk_radio_menu_item_new_with_mnemonic(group, "Delayed Images");
        group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(item));
        gtk_widget_show(item);
        g_object_set_data(G_OBJECT(item), MIDX, (gpointer)HA_DLIMG);
        gtk_menu_shell_append(GTK_MENU_SHELL(h_options_menu), item);
        g_signal_connect(G_OBJECT(item), "activate",
            G_CALLBACK(h_menu_hdlr), this);

        // "/Options/Progressive Images", 0, h_menu_hdlr, HA_PGIMG,
        //  "/Options/Delayed Images"
        item = gtk_radio_menu_item_new_with_mnemonic(group,"Progressive Images");
        gtk_widget_show(item);
        g_object_set_data(G_OBJECT(item), MIDX, (gpointer)HA_PGIMG);
        gtk_menu_shell_append(GTK_MENU_SHELL(h_options_menu), item);
        g_signal_connect(G_OBJECT(item), "activate",
            G_CALLBACK(h_menu_hdlr), this);

        item = gtk_separator_menu_item_new();
        gtk_widget_show(item);
        gtk_menu_shell_append(GTK_MENU_SHELL(h_options_menu), item);

        // "/Options/Anchor Plain", 0, h_menu_hdlr, HA_APLN, "<RadioItem>"
        group = 0;
        item = gtk_radio_menu_item_new_with_mnemonic(group, "Anchor Plain");
        group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(item));
        gtk_widget_show(item);
        g_object_set_data(G_OBJECT(item), MIDX, (gpointer)HA_APLN);
        gtk_menu_shell_append(GTK_MENU_SHELL(h_options_menu), item);
        g_signal_connect(G_OBJECT(item), "activate",
            G_CALLBACK(h_menu_hdlr), this);

        // "/Options/Anchor Buttons", 0, h_menu_hdlr, HA_ABUT,
        //  "/Options/Anchor Plain"
        item = gtk_radio_menu_item_new_with_mnemonic(group, "Anchor Buttons");
        group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(item));
        gtk_widget_show(item);
        g_object_set_data(G_OBJECT(item), MIDX, (gpointer)HA_ABUT);
        gtk_menu_shell_append(GTK_MENU_SHELL(h_options_menu), item);
        g_signal_connect(G_OBJECT(item), "activate",
            G_CALLBACK(h_menu_hdlr), this);

        // "/Options/Anchor Underline", 0, h_menu_hdlr, HA_AUND,
        //  "/Options/Anchor Buttons"
        item = gtk_radio_menu_item_new_with_mnemonic(group, "Anchor Underline");
        gtk_widget_show(item);
        g_object_set_data(G_OBJECT(item), MIDX, (gpointer)HA_AUND);
        gtk_menu_shell_append(GTK_MENU_SHELL(h_options_menu), item);
        g_signal_connect(G_OBJECT(item), "activate",
            G_CALLBACK(h_menu_hdlr), this);

        // "/Options/Anchor Highlight", 0, GIFC(h_menu_hdlr), HA_HLITE,
        //  "<CheckItem>"
        item = gtk_check_menu_item_new_with_mnemonic("Anchor Highlight");
        gtk_widget_show(item);
        g_object_set_data(G_OBJECT(item), MIDX, (gpointer)HA_HLITE);
        gtk_menu_shell_append(GTK_MENU_SHELL(h_options_menu), item);
        g_signal_connect(G_OBJECT(item), "activate",
            G_CALLBACK(h_menu_hdlr), this);

        // "/Options/Bad HTML Warnings", 0, h_menu_hdlr, HA_WARN, "<CheckItem>"
        item = gtk_check_menu_item_new_with_mnemonic("Bad HTML Warnings");
        gtk_widget_show(item);
        g_object_set_data(G_OBJECT(item), MIDX, (gpointer)HA_WARN);
        gtk_menu_shell_append(GTK_MENU_SHELL(h_options_menu), item);
        g_signal_connect(G_OBJECT(item), "activate",
            G_CALLBACK(h_menu_hdlr), this);

        // "/Options/Freeze Animations", 0, h_menu_hdlr, HA_FREEZ, "<CheckItem>"
        item = gtk_check_menu_item_new_with_mnemonic("Freeze Animations");
        gtk_widget_show(item);
        g_object_set_data(G_OBJECT(item), MIDX, (gpointer)HA_FREEZ);
        gtk_menu_shell_append(GTK_MENU_SHELL(h_options_menu), item);
        g_signal_connect(G_OBJECT(item), "activate",
            G_CALLBACK(h_menu_hdlr), this);

        // "/Options/Log Transactions", 0, h_menu_hdlr, HA_COMM, "<CheckItem>"
        item = gtk_check_menu_item_new_with_mnemonic("Log Transactions");
        gtk_widget_show(item);
        g_object_set_data(G_OBJECT(item), MIDX, (gpointer)HA_COMM);
        gtk_menu_shell_append(GTK_MENU_SHELL(h_options_menu), item);
        g_signal_connect(G_OBJECT(item), "activate",
            G_CALLBACK(h_menu_hdlr), this);

        // Bookmarks menu.
        item = gtk_menu_item_new_with_mnemonic("_Bookmarks");
        gtk_widget_set_name(item, "Bookmarks");
        gtk_widget_show(item);
        gtk_menu_shell_append(GTK_MENU_SHELL(h_menubar), item);
        h_bookmarks_menu = gtk_menu_new();
        gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), h_bookmarks_menu);
        gtk_widget_show(h_bookmarks_menu);

        // "/Bookmarks/Add", 0, h_menu_hdlr, HA_BMADD, 0
        item = gtk_menu_item_new_with_mnemonic("Add");
        gtk_widget_show(item);
        g_object_set_data(G_OBJECT(item), MIDX, (gpointer)HA_BMADD);
        gtk_menu_shell_append(GTK_MENU_SHELL(h_bookmarks_menu), item);
        g_signal_connect(G_OBJECT(item), "activate",
            G_CALLBACK(h_menu_hdlr), this);

        // "/Bookmarks/Delete", 0, GIFC(h_menu_hdlr), HA_BMDEL, "<CheckItem>"
        item = gtk_check_menu_item_new_with_mnemonic("Delete");
        gtk_widget_show(item);
        g_object_set_data(G_OBJECT(item), MIDX, (gpointer)HA_BMDEL);
        gtk_menu_shell_append(GTK_MENU_SHELL(h_bookmarks_menu), item);
        g_signal_connect(G_OBJECT(item), "activate",
            G_CALLBACK(h_menu_hdlr), this);

        item = gtk_separator_menu_item_new();
        gtk_widget_show(item);
        gtk_menu_shell_append(GTK_MENU_SHELL(h_bookmarks_menu), item);

        // Read bookmarks
        HLP()->context()->readBookmarks();
        for (HLPbookMark *b = HLP()->context()->bookmarks(); b; b = b->next) {
            char title[36];
            strncpy(title, b->title, 32);
            item = gtk_menu_item_new_with_label(title);
            gtk_widget_show(item);
            gtk_menu_shell_append(GTK_MENU_SHELL(h_bookmarks_menu), item);
            g_object_set_data_full(G_OBJECT(item), "data",
                lstring::copy(b->url), h_bm_dest);
            g_signal_connect(G_OBJECT(item), "activate",
                G_CALLBACK(h_bm_handler), this);
        }

        // Help menu.
        item = gtk_menu_item_new_with_mnemonic("_Help");
        gtk_widget_set_name(item, "Help");
        gtk_widget_show(item);
        gtk_menu_item_set_right_justified(GTK_MENU_ITEM(item), true);
        gtk_menu_shell_append(GTK_MENU_SHELL(h_menubar), item);
        GtkWidget *submenu = gtk_menu_new();
        gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), submenu);
        gtk_widget_show(submenu);

        // "/Help/_Help", "<control>H", h_menu_hdlr, HA_HELP, 0
        item = gtk_menu_item_new_with_mnemonic("_Help");
        gtk_widget_show(item);
        g_object_set_data(G_OBJECT(item), MIDX, (gpointer)HA_HELP);
        gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
        g_signal_connect(G_OBJECT(item), "activate",
            G_CALLBACK(h_menu_hdlr), this);
        gtk_widget_add_accelerator(item, "activate", accel_group, GDK_KEY_h,
            GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
#endif
    }

    GtkWidget *frame = gtk_frame_new(0);
    gtk_widget_show(frame);
    if (h_is_frame)
        wb_shell = frame;

    h_viewer = new gtk_viewer(1, 1, this);
    gtk_container_add(GTK_CONTAINER(frame), h_viewer->top_widget());

    if (!h_is_frame) {
        set_defaults();

        gtk_table_attach(GTK_TABLE(form), frame, 0, 1, 1, 2,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);

        GtkWidget *sep = gtk_hseparator_new();
        gtk_widget_show(sep);
        gtk_table_attach(GTK_TABLE(form), sep, 0, 1, 2, 3,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);

        GtkWidget *hbox = gtk_hbox_new(false, 2);
        gtk_widget_show(hbox);

        frame = gtk_frame_new(0);
        gtk_widget_show(frame);

        GtkWidget *label = gtk_label_new(0);
        gtk_widget_show(label);
        gtk_container_add(GTK_CONTAINER(frame), label);
        gtk_box_pack_start(GTK_BOX(hbox), frame, true, true, 0);
        g_object_set_data(G_OBJECT(wb_shell), "label", label);

        gtk_table_attach(GTK_TABLE(form), hbox, 0, 1, 3, 4,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    // drop site
    gtk_drag_dest_set(h_viewer->top_widget(),
        GTK_DEST_DEFAULT_ALL, target_table, n_targets, GDK_ACTION_COPY);
    g_signal_connect_after(G_OBJECT(h_viewer->top_widget()),
        "drag-data-received", G_CALLBACK(h_drag_data_received), this);

    gtk_widget_show(wb_shell);
}


GTKhelpPopup::~GTKhelpPopup()
{
    unregister_fifo();
    for (int i = 0; i < 4; i++) {
        if (h_fsels[i])
            h_fsels[i]->popdown();
    }
    halt_images();
    HLP()->context()->removeTopic(h_root_topic);
    HLPtopic::destroy(h_root_topic);
    if (!h_is_frame && h_params) {
        h_params->update();
        delete h_params;
    }
    delete h_find_text;
    delete h_viewer;

    if (h_frame_array) {
        for (int i = 0; i < h_frame_array_size; i++)
            delete h_frame_array[i];
        delete [] h_frame_array;
    }
    delete [] h_frame_name;

    gtk_widget_hide(wb_shell);
    // shell destroyed in GTKbag destructor

    if (!HLP()->context()->topList())
        delete Clr;
}


//-----------------------------------------------------------------------------
// ViewWidget and HelpWidget interface

// Static function.
HelpWidget *
HelpWidget::get_widget(HLPtopic *top)
{
    if (!top)
        return (0);
    GTKhelpPopup *w =
        dynamic_cast<GTKhelpPopup*>(HLPtopic::get_parent(top)->context());
    return (w);
}


// Static function.
HelpWidget *
HelpWidget::new_widget(GRwbag **ptr, int xpos, int ypos)
{
    GtkWidget *parent = 0;
    if (GRX->MainFrame())
        parent = GRX->MainFrame()->Shell();

    GTKhelpPopup *w = new GTKhelpPopup(true, xpos, ypos, parent);
    if (ptr)
        *ptr = w;
    return (w);
}


void
GTKhelpPopup::freeze()
{
    h_viewer->freeze();
}


void
GTKhelpPopup::thaw()
{
    h_viewer->thaw();
}


void
GTKhelpPopup::set_transaction(Transaction *t, const char *cookiedir)
{
    h_viewer->set_transaction(t, cookiedir);
    if (t) {
        t->set_timeout(h_params->Timeout);
        t->set_retries( h_params->Retries);
        t->set_http_port(h_params->HTTP_Port);
        t->set_ftp_port(h_params->FTP_Port);
        t->set_http_debug(h_params->DebugMode);
        if (h_params->PrintTransact)
            t->set_logfile("stderr");
        if (cookiedir && !h_params->NoCookies) {
            char *cf = new char [strlen(cookiedir) + 20];
            sprintf(cf, "%s/%s", cookiedir, "cookies");
            t->set_cookiefile(cf);
            delete [] cf;
        }
    }
}


Transaction *
GTKhelpPopup::get_transaction()
{
    return (h_viewer->get_transaction());
}


bool
GTKhelpPopup::check_halt_processing(bool run_events)
{
    if (run_events) {
        gtk_DoEvents(100);
        GtkWidget *stopbtn =
            (GtkWidget*)g_object_get_data(G_OBJECT(wb_shell), "stopbtn");
        if (stopbtn && !gtk_widget_get_sensitive(stopbtn) &&
                g_object_get_data(G_OBJECT(stopbtn), "pressed"))
            // abort
            return (true);
    }
    else {
        GtkWidget *stopbtn =
            (GtkWidget*)g_object_get_data(G_OBJECT(wb_shell), "stopbtn");
        if (stopbtn && g_object_get_data(G_OBJECT(stopbtn), "pressed"))
            // stop button pressed already
            return (true);
    }
    return (false);
}


void
GTKhelpPopup::set_halt_proc_sens(bool sens)
{
    GtkWidget *stopbtn =
        (GtkWidget*)g_object_get_data(G_OBJECT(wb_shell), "stopbtn");
    if (stopbtn)
        gtk_widget_set_sensitive(stopbtn, sens);
}


void
GTKhelpPopup::set_status_line(const char *msg)
{
    if (h_frame_parent)
        h_frame_parent->set_status_line(msg);
    else {
        GtkWidget *label = (GtkWidget*)g_object_get_data(
            G_OBJECT(wb_shell), "label");
        if (label)
            gtk_label_set_text(GTK_LABEL(label), msg);
    }
}


htmImageInfo *
GTKhelpPopup::new_image_info(const char *url, bool progressive)
{
    htmImageInfo *image = new htmImageInfo();
    if (progressive)
        image->options = IMAGE_PROGRESSIVE | IMAGE_ALLOW_SCALE;
    else
        image->options = IMAGE_DELAYED;
    image->url = strdup(url);
    return (image);
}


bool
GTKhelpPopup::call_plc(const char *url)
{
    return (h_viewer->call_plc(url));
}


htmImageInfo *
GTKhelpPopup::image_procedure(const char *fname)
{
    return (h_viewer->image_procedure(fname));
}


void
GTKhelpPopup::image_replace(htmImageInfo *oldim, htmImageInfo *newim)
{
    h_viewer->image_replace(oldim, newim);
}


bool
GTKhelpPopup::is_body_image(const char *url)
{
    return (h_viewer->is_body_image(url));
}


const char *
GTKhelpPopup::get_url()
{
    return (h_cur_topic ? h_cur_topic->keyword() : 0);
}


bool
GTKhelpPopup::no_url_cache()
{
    return (h_params->NoCache);
}


int
GTKhelpPopup::image_load_mode()
{
    return (h_params->LoadMode);
}


int
GTKhelpPopup::image_debug_mode()
{
    return (h_params->LocalImageTest);
}


// Return a graphics context pointer.
//
GRwbag *
GTKhelpPopup::get_widget_bag()
{
    return (this);
}


// Link a new topic into the lists, and display it.
//
void
GTKhelpPopup::link_new(HLPtopic *top)
{
    HLP()->context()->linkNewTopic(top);
    h_root_topic = top;
    h_cur_topic = top;
    reuse(0, false);
}


namespace {
    // Strip HTML tokens out of the window title
    //
    void
    strip_html(char *buf)
    {
        char tbuf[256];
        strcpy(tbuf, buf);
        char *d = buf;
        for (char *s = tbuf; *s; s++) {
            if (*s == '<' && (*(s+1) == '/' || isalpha(*(s+1)))) {
                while (*s && *s != '>')
                    s++;
                continue;
            }
            *d++ = *s;
        }
        *d = 0;
    }

    // Timeout proc. to start display and image loop.
    //
    int ils_to(void *arg)
    {
        GTKhelpPopup *hp = (GTKhelpPopup*)arg;
        hp->reuse_display();
        return (0);
    }
}


// Display newtop, link it in the list of topics viewed if newlink is
// true.
//
void
GTKhelpPopup::reuse(HLPtopic *newtop, bool newlink)
{
    if (h_root_topic->lastborn()) {
        // in the forward/back operations, the new topic is stitched in
        // as lastborn, and the scroll update is handled elsewhere
        if (h_root_topic->lastborn() != newtop)
            h_root_topic->lastborn()->set_topline(scroll_position(false));
    }
    else if (newtop)
        h_root_topic->set_topline(scroll_position(false));

    if (!newtop)
        newtop = h_root_topic;

    if (!newtop->is_html())
        newtop->set_show_plain(HLP()->context()->isPlain(newtop->keyword()));
    if (newtop != h_root_topic) {
        h_root_topic->set_text(newtop->get_text());
        newtop->clear_text();
    }
    else
        h_root_topic->get_text();
    h_cur_topic = newtop;

    if (newlink && newtop != h_root_topic) {
        newtop->set_sibling(h_root_topic->lastborn());
        h_root_topic->set_lastborn(newtop);
        newtop->set_parent(h_root_topic);
        GtkWidget *back = (GtkWidget*)g_object_get_data(G_OBJECT(wb_shell),
            "back");
        if (back)
            gtk_widget_set_sensitive(back, true);
    }

    // If the widget is just popping up, have to wait for the viewer's
    // expose event.  This enables the viewer to be initialized, and
    // the image processing loop started properly.  If we don't wait,
    // non-local images for this page won't display in delayed or
    // progressive loading mode.

    if (!h_viewer->initialized())
        g_timeout_add(200, (GSourceFunc)ils_to, this);
    else
        reuse_display();
}


void
GTKhelpPopup::reuse_display()
{
    char *anchor = 0;
    const char *t = HLP()->context()->findAnchorRef(h_cur_topic->keyword());
    if (t)
        anchor = lstring::copy(t);

    h_viewer->freeze();
    redisplay();

    set_scroll_position(0, true);
    if (h_cur_topic->get_topline())
        set_scroll_position(h_cur_topic->get_topline(), false);
    else if (anchor) {
        // Need this or won't scroll to anchor on initiial pop-up.
        int cnt = 0;
        while (!h_viewer->is_ready() && cnt < 20) {
            cTimer::milli_sleep(50);
            GRX->CheckForEvents();
            cnt++;
        }
        set_scroll_position(h_viewer->anchor_position(anchor), false);
    }
    else
        set_scroll_position(0, false);
    h_viewer->thaw();

    delete [] anchor;
    GtkWidget *label =
        (GtkWidget*)g_object_get_data(G_OBJECT(wb_shell), "label");
    if (label)
        gtk_label_set_text(GTK_LABEL(label), h_cur_topic->keyword());

    if (GTK_IS_WINDOW(wb_shell)) {
        // Can be a frame, no title in that case.
        char *t0 = 0;
        t = h_cur_topic->title();
        if (!t || !*t)
            t = t0 = h_viewer->get_title();
        if (!t || !*t)
            t = "Whiteley Research Inc.";
        char buf[80];
        snprintf(buf, 80, "%s -- %s", HLP()->get_name(), t);
        delete [] t0;
        strip_html(buf);
        gtk_window_set_title(GTK_WINDOW(wb_shell), buf);
    }

    HLP()->context()->imageLoopStart();
}


void
GTKhelpPopup::redisplay()
{
    Transaction *t = h_viewer->get_transaction();
    if (t) {
        // still downloading previous page, abort
        t->set_abort();
        h_viewer->set_transaction(0, 0);
    }
    halt_images();

    HLPtopic *top = h_root_topic->lastborn();
    if (!top)
        top = h_root_topic;

    if (top->show_plain() || !top->is_html())
        h_viewer->set_mime_type("text/plain");
    else
        h_viewer->set_mime_type("text/html");
    h_viewer->set_source(h_root_topic->get_cur_text());

    // Process events to avoid scroll position error.
    GRX->CheckForEvents();
}


HLPtopic *
GTKhelpPopup::get_topic()
{
    return (h_cur_topic);
}


void
GTKhelpPopup::unset_halt_flag()
{
    h_stop_btn_pressed = false;
}


// Halt all image transfers in progress for the widget, and clear the
// queue of jobs for this window
//
void
GTKhelpPopup::halt_images()
{
    stop_image_download();
    HLP()->context()->flushImages(this);
}


// Pop-up to display a list of cache entries
//
void
GTKhelpPopup::show_cache(int mode)
{
    if (mode == MODE_OFF) {
        if (h_cache_list)
            h_cache_list->popdown();
        return;
    }
    if (mode == MODE_UPD) {
        if (h_cache_list) {
            stringlist *s0 = HLP()->context()->listCache();
            h_cache_list->update(s0, "Cache Entries", 0);
            stringlist::destroy(s0);
        }
        return;
    }
    if (h_cache_list)
        return;
    stringlist *s0 = HLP()->context()->listCache();
    h_cache_list =
        PopUpList(s0, "Cache Entries", 0, h_list_cb, this, false, false);
    h_cache_list->register_usrptr((void**)&h_cache_list);
    stringlist::destroy(s0);
}


//-----------------------------------------------------------------------------
// htmDataInterface functions

// Dispatch function for widget "signals".
//
void
GTKhelpPopup::emit_signal(SignalID id, void *payload)
{
    switch (id) {
    case S_ARM:
        break;
    case S_ACTIVATE:
        activate_signal_handler(
            static_cast<htmAnchorCallbackStruct*>(payload));
        break;
    case S_ANCHOR_TRACK:
        anchor_track_signal_handler(
            static_cast<htmAnchorCallbackStruct*>(payload));
        break;
    case S_ANCHOR_VISITED:
        {
            htmVisitedCallbackStruct *cbs =
                static_cast<htmVisitedCallbackStruct*>(payload);
            if (cbs)
                cbs->visited = HLP()->context()->isVisited(cbs->url);
        }
        break;
    case S_DOCUMENT:
        break;
    case S_LINK:
        break;
    case S_FRAME:
        frame_signal_handler(
            static_cast<htmFrameCallbackStruct*>(payload));
        break;
    case S_FORM:
        form_signal_handler(
            static_cast<htmFormCallbackStruct*>(payload));
        break;
    case S_IMAGEMAP:
        break;
    case S_HTML_EVENT:
        break;
    default:
        break;
    }
}


void *
GTKhelpPopup::event_proc(const char*)
{
    return (0);
}


// Called on unrecoverable error.
//
void
GTKhelpPopup::panic_callback(const char*)
{
}


// Called by the widget to resolve image references.
//
htmImageInfo *
GTKhelpPopup::image_resolve(const char *fname)
{
    if (!fname)
        return (0);
    return (HLP()->context()->imageResolve(fname, this));
}


// This is the "get_data" callback for progressive image loading.
//
int
GTKhelpPopup::get_image_data(htmPLCStream *stream, void *buffer)
{
    HLPimageList *im = (HLPimageList*)stream->user_data;
    return (HLP()->context()->getImageData(im, stream, buffer));
}


// This is the "end_data" callback for progressive image loading.
//
void
GTKhelpPopup::end_image_data(htmPLCStream *stream, void*,
    int type, bool)
{
    if (type == PLC_IMAGE) {
        HLPimageList *im = (HLPimageList*)stream->user_data;
        HLP()->context()->inactivateImages(im);
    }
}


// This returns the client area that can be used for display frames.
// We use the entire widget width, and the height between the menu and
// status bar.
//
void
GTKhelpPopup::frame_rendering_area(htmRect *rect)
{
    rect->x = 0;
    GtkAllocation a;
    gtk_widget_get_allocation(h_viewer->top_widget(), &a);
    rect->y = a.x;
    rect->width = a.width;
    rect->height = a.height;
}


// If this is a frame, return the frame name.
//
const char *
GTKhelpPopup::get_frame_name()
{
    return (h_frame_name);
}


// Return the keyword and title from the current topic, if possible.
//
void
GTKhelpPopup::get_topic_keys(char **pkw, char **ptitle)
{
    if (pkw)
        *pkw = 0;
    if (ptitle)
        *ptitle = 0;
    HLPtopic *t = h_cur_topic;
    if (t) {
        if (pkw)
            *pkw = strdup(t->keyword());
        if (ptitle) {
            const char *title = t->title();
            if (title)
                *ptitle = strdup(title);
        }
    }
}


// Ensure that the bounding box passed is visible.
//
void
GTKhelpPopup::scroll_visible(int l, int t, int r, int b)
{
    h_viewer->scroll_visible(l, t, r, b);
}


//-----------------------------------------------------------------------------
// GTKbag functions

// Entry point for postscript output.
//
// The font is encoded as:
//        0: times (default)
//        1: helvetica
//        2: new century schoolbook
//        3: lucida
//
// The title string appears in the upper header, the url string appears
// in the footer (if use_header is true).  Paper size is US Letter unless
// a4 is set.
//
char *
GTKhelpPopup::GetPostscriptText(int fontfamily, const char *url,
    const char *title, bool use_header, bool a4)
{
    return (h_viewer->get_postscript_text(fontfamily, url, title, use_header,
        a4));
}


// Return an ascii string representation of the widget contents.  All
// text will be shown, images and other graphics are ignored.
//
char *
GTKhelpPopup::GetPlainText()
{
    return (h_viewer->get_plain_text());
}


// Return a copy of the HTML currently loaded.
//
char *
GTKhelpPopup::GetHtmlText()
{
    return (h_viewer->get_html_text());
}


//-----------------------------------------------------------------------------
// Misc. public functions

// Function to display a new topic, or respond to a link.
//
GTKhelpPopup::NTtype
GTKhelpPopup::newtopic(const char *href, bool spawn, bool force_download,
    bool nonrelative)
{
    HLPtopic *newtop;
    char hanchor[128];
    if (HLP()->context()->resolveKeyword(href, &newtop, hanchor, this,
            h_cur_topic, force_download, nonrelative))
        return (GTKhelpPopup::NThandled);
    if (!newtop) {
        char buf[256];
        sprintf(buf, "Unresolved link: %s.", href);
        PopUpErr(MODE_ON, buf);
        return (GTKhelpPopup::NTnone);
    }
    if (spawn)
        newtop->set_context(0);
    else
        newtop->set_context(this);

    newtop->link_new_and_show(spawn, h_cur_topic);
    return (GTKhelpPopup::NTnew);
}


GTKhelpPopup::NTtype
GTKhelpPopup::newtopic(const char *fname, FILE *fp, bool spawn)
{
//    topic *top = checkImage(url, this);
//    if (top)
//        return (top);
    HLPtopic *top = new HLPtopic(fname, "");
    top->get_file(fp, fname);

    if (spawn)
        top->set_context(0);
    else
        top->set_context(this);

    top->link_new_and_show(spawn, h_cur_topic);
    return (GTKhelpPopup::NTnew);
}


namespace {
    GtkWidget *find_item(GList *list, unsigned int ix)
    {
        for (GList *l = list; l; l = l->next) {
            unsigned long x = (unsigned long)g_object_get_data(
                G_OBJECT(l->data), MIDX);
            if (x == ix)
                return (GTK_WIDGET(l->data));
        }
        return (0);
    }
}


// Initialize the menu buttons and the widget according to the values
// in the params struct.
//
void
GTKhelpPopup::set_defaults()
{
    if (!h_options_menu)
        return;
    GList *list = gtk_container_get_children(GTK_CONTAINER(h_options_menu));

    GtkWidget *btn;
    if (h_params->LoadMode == HLPparams::LoadProgressive) {
        btn = find_item(list, HA_PGIMG);
        if (btn && !GRX->GetStatus(btn))
            gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(btn), true);
    }
    else if (h_params->LoadMode == HLPparams::LoadDelayed) {
        btn = find_item(list, HA_DLIMG);
        if (btn && !GRX->GetStatus(btn))
            gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(btn), true);
    }
    else if (h_params->LoadMode == HLPparams::LoadSync) {
        btn = find_item(list, HA_SYIMG);
        if (btn && !GRX->GetStatus(btn))
            gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(btn), true);
    }
    else {
        h_params->LoadMode = HLPparams::LoadNone;
        btn = find_item(list, HA_NOIMG);
        if (btn && !GRX->GetStatus(btn))
            gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(btn), true);
    }

    if (h_params->AnchorUnderlineType)
        h_params->AnchorUnderlined = true;
    else {
        h_params->AnchorUnderlined = false;
        h_params->AnchorUnderlineType = 1;
    }

    if (h_params->AnchorButtons) {
        btn = find_item(list, HA_ABUT);
        if (btn && !GRX->GetStatus(btn))
            gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(btn), true);
    }
    else if (h_params->AnchorUnderlined) {
        btn = find_item(list, HA_AUND);
        if (btn && !GRX->GetStatus(btn))
            gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(btn), true);
    }
    else {
        btn = find_item(list, HA_APLN);
        if (btn && !GRX->GetStatus(btn))
            gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(btn), true);
    }

    btn = find_item(list, HA_HLITE);
    if (btn)
        GRX->SetStatus(btn, h_params->AnchorHighlight);

    btn = find_item(list, HA_COMM);
    if (btn)
        GRX->SetStatus(btn, h_params->PrintTransact);

    btn = find_item(list, HA_WARN);
    if (btn)
        GRX->SetStatus(btn, h_params->BadHTMLwarnings);

    btn = find_item(list, HA_FREEZ);
    if (btn)
        GRX->SetStatus(btn, h_params->FreezeAnimations);

    btn = find_item(list, HA_NOCACHE);
    if (btn)
        GRX->SetStatus(btn, h_params->NoCache);
    btn = find_item(list, HA_NOCKS);
    if (btn)
        GRX->SetStatus(btn, h_params->NoCookies);

    g_list_free(list);

    if (!h_viewer) {
        fprintf(stderr, "internal error: null viewer\n");
        return;
    }
    if (h_params->AnchorButtons)
        h_viewer->set_anchor_style(ANC_BUTTON);
    else if (h_params->AnchorUnderlineType)
        h_viewer->set_anchor_style(ANC_SINGLE_LINE);
    else
        h_viewer->set_anchor_style(ANC_PLAIN);

    h_viewer->set_anchor_highlighting(h_params->AnchorHighlight);
    h_viewer->set_html_warnings(h_params->BadHTMLwarnings);
    h_viewer->set_freeze_animations(h_params->FreezeAnimations);
}


void
GTKhelpPopup::stop_image_download()
{
    if (h_params->LoadMode == HLPparams::LoadProgressive)
        h_viewer->progressive_kill();
    HLP()->context()->abortImageDownload(this);
}


int
GTKhelpPopup::scroll_position(bool horiz)
{
    return (h_viewer->scroll_position(horiz));
}


void
GTKhelpPopup::set_scroll_position(int value, bool horiz)
{
    return (h_viewer->set_scroll_position(value, horiz));
}


// New topic callback, handle clicking on an anchor.
//
void
GTKhelpPopup::activate_signal_handler(htmAnchorCallbackStruct *cbs)
{
    if (cbs == 0 || cbs->href == 0)
        return;
    HLPtopic *parent = h_cur_topic;
    cbs->visited = true;

    // add link to visited table
    HLP()->context()->addVisited(cbs->href);

    // download if shift pressed
    bool force_download = false;

    GdkEvent *event = (GdkEvent*)cbs->event;
    if (event && (event->type == GDK_BUTTON_PRESS ||
            event->type == GDK_2BUTTON_PRESS ||
            event->type == GDK_3BUTTON_PRESS ||
            event->type == GDK_BUTTON_RELEASE) &&
            (event->button.state & GDK_SHIFT_MASK))
        force_download = true;

    bool spawn = false;
    if (!force_download) {
        if (cbs->target) {
            if (!parent->target() ||
                    strcmp(parent->target(), cbs->target)) {
                for (HLPtopic *t = HLP()->context()->topList(); t;
                        t = t->sibling()) {
                    if (t->target() && !strcmp(t->target(), cbs->target)) {
                        newtopic(cbs->href, false, false, false);
                        return;
                    }
                }
                // Special targets:
                //  _top    reuse same window, no frames
                //  _self   put in originating frame
                //  _blank  put in new window
                //  _parent put in parent frame (nested framesets)

                if (!strcmp(cbs->target, "_top")) {
                    newtopic(cbs->href, false, false, false);
                    return;
                }
                // note: _parent not handled, use new window
                if (strcmp(cbs->target, "_self"))
                    spawn = true;
            }
        }
    }

    if (!spawn) {
        // spawn a new window if button 2 pressed
        if (event && (event->type == GDK_BUTTON_PRESS ||
                event->type == GDK_2BUTTON_PRESS ||
                event->type == GDK_3BUTTON_PRESS ||
                event->type == GDK_BUTTON_RELEASE) &&
                event->button.button == 2)
            spawn = true;
    }

    newtopic(cbs->href, spawn, force_download, false);

    if (cbs->target && spawn) {
        for (HLPtopic *t = HLP()->context()->topList(); t; t = t->sibling()) {
            if (!strcmp(t->keyword(), cbs->href)) {
                t->set_target(cbs->target);
                break;
            }
        }
    }
}


// Callback to print the current anchor href in the label.
//
void
GTKhelpPopup::anchor_track_signal_handler(htmAnchorCallbackStruct *cbs)
{
    if (!h_cur_topic)
        return;
    GtkWidget *label = (GtkWidget*)g_object_get_data(G_OBJECT(wb_shell),
        "label");
    if (!label)
        return;
    if (cbs->href)
        gtk_label_set_text(GTK_LABEL(label), cbs->href);
    else
        gtk_label_set_text(GTK_LABEL(label), h_cur_topic->keyword());
}


// Callback from the widget for frame management.
//
void
GTKhelpPopup::frame_signal_handler(htmFrameCallbackStruct *cbs)
{
    GtkWidget *fixed = h_viewer->fixed_widget();
    if (cbs->reason == htm::HTM_FRAMECREATE) {
        h_viewer->hide_drawing_area(true);
        h_frame_array_size = cbs->nframes;
        h_frame_array = new GTKhelpPopup*[h_frame_array_size];
        for (int i = 0; i < h_frame_array_size; i++) {
            h_frame_array[i] = new GTKhelpPopup(false, 0, 0, 0);
            // use parent's defaults
            h_frame_array[i]->h_params = h_params;
            h_frame_array[i]->set_frame_parent(this);
            h_frame_array[i]->set_frame_name(cbs->frames[i].name);

            gtk_widget_set_size_request(h_frame_array[i]->Shell(),
                cbs->frames[i].width, cbs->frames[i].height);
            gtk_fixed_put(GTK_FIXED(fixed), h_frame_array[i]->Shell(),
                cbs->frames[i].x, cbs->frames[i].y);

            if (cbs->frames[i].scroll_type == FRAME_SCROLL_NONE)
                h_frame_array[i]->h_viewer->set_scroll_policy(
                    GTK_POLICY_NEVER, GTK_POLICY_NEVER);
            else if (cbs->frames[i].scroll_type == FRAME_SCROLL_AUTO)
                h_frame_array[i]->h_viewer->set_scroll_policy(
                    GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
            else if (cbs->frames[i].scroll_type == FRAME_SCROLL_YES)
                h_frame_array[i]->h_viewer->set_scroll_policy(
                    GTK_POLICY_ALWAYS, GTK_POLICY_ALWAYS);
        }
        for (int i = 0; i < h_frame_array_size; i++) {
            HLPtopic *newtop;
            char hanchor[128];
            HLP()->context()->resolveKeyword(cbs->frames[i].src, &newtop,
                hanchor, this, h_cur_topic, false, false);
            if (!newtop) {
                char buf[256];
                sprintf(buf, "Unresolved link: %s.", cbs->frames[i].src);
                PopUpErr(MODE_ON, buf);
            }
            else {
                newtop->set_target(cbs->frames[i].name);
                newtop->set_context(h_frame_array[i]);
                h_frame_array[i]->h_root_topic = newtop;
                h_frame_array[i]->h_cur_topic = newtop;

                if (!newtop->is_html() &&
                        HLP()->context()->isPlain(newtop->keyword())) {
                    newtop->set_show_plain(true);
                    h_frame_array[i]->h_viewer->set_mime_type("text/plain");
                }
                else {
                    newtop->set_show_plain(false);
                    h_frame_array[i]->h_viewer->set_mime_type("text/html");
                }
                h_frame_array[i]->h_viewer->set_source(newtop->get_text());
            }
        }
    }
    else if (cbs->reason == htm::HTM_FRAMERESIZE) {
        for (int i = 0; i < h_frame_array_size; i++) {

            gtk_widget_set_size_request(h_frame_array[i]->Shell(),
                cbs->frames[i].width, cbs->frames[i].height);
            gtk_fixed_move(GTK_FIXED(fixed), h_frame_array[i]->Shell(),
                cbs->frames[i].x, cbs->frames[i].y);
        }
    }
    else if (cbs->reason == htm::HTM_FRAMEDESTROY) {
        for (int i = 0; i < h_frame_array_size; i++)
            delete h_frame_array[i];
        delete [] h_frame_array;
        h_frame_array = 0;
        h_frame_array_size = 0;
        h_viewer->hide_drawing_area(false);
    }
}


// Handle the "submit" request for an html form.  The form return is
// always downloaded and never taken from the cache, since this prevents
// multiple submissions of the same form
//
void
GTKhelpPopup::form_signal_handler(htmFormCallbackStruct *cbs)
{
    HLP()->context()->formProcess(cbs, this);
}


//-----------------------------------------------------------------------------
// Callback functions for Glib signals.

// Static function.
// Receive drop data (a path name, keyword, or url)
//
void
GTKhelpPopup::h_drag_data_received(GtkWidget*, GdkDragContext *context,
    gint, gint, GtkSelectionData *data, guint, guint time, void *hlpptr)
{
    if (gtk_selection_data_get_length(data) >= 0 &&
            gtk_selection_data_get_format(data) == 8 &&
            gtk_selection_data_get_data(data)) {
        char *src = (char*)gtk_selection_data_get_data(data);
        GTKhelpPopup *w = static_cast<GTKhelpPopup*>(hlpptr);
        if (w) {
            if (w->wb_input) {
                if (!w->wb_input->no_drops()) {
                    w->wb_input->update(0, src);
                    gtk_drag_finish(context, true, false, time);
                    return;
                }
                if (w->wb_input)
                    w->wb_input->popdown();
            }
            w->PopUpInput("Enter Keyword", src, "Open", h_open_cb, w);
            gtk_drag_finish(context, true, false, time);
            return;
        }
    }
    gtk_drag_finish(context, false, false, time);
}


// Static function.
// Handler for the stop function, stop transfer.
//
void
GTKhelpPopup::h_stop_proc(GtkWidget *btn, void *hlpptr)
{
    GTKhelpPopup *w = static_cast<GTKhelpPopup*>(hlpptr);
    if (w)
        w->stop_image_download();
    gtk_widget_set_sensitive(btn, false);
    g_object_set_data(G_OBJECT(btn), "pressed", (void*)1);
}


// Static function.
// Pop up the font selection widget.
//
void
GTKhelpPopup::h_fontsel(GTKbag *w, GtkWidget *caller)
{
    if (GRX->GetStatus(caller))
        w->PopUpFontSel(0, GRloc(), MODE_ON, h_font_cb, caller, FNT_MOZY);
    else
        w->PopUpFontSel(0, GRloc(), MODE_OFF, 0, 0, 0);
}


// Static function.
// Handler for the menubar menus.
//
void
GTKhelpPopup::h_menu_hdlr(GtkWidget *caller, void *hlpptr)
{
    unsigned long activate =
        (unsigned long)g_object_get_data(G_OBJECT(caller), MIDX);
    if (activate == HA_NIL)
        return;
    GTKhelpPopup *w = static_cast<GTKhelpPopup*>(hlpptr);
    if (!w)
        return;
    if (activate == HA_CANCEL || activate == HA_QUIT) {
        HLP()->context()->removeTopic(w->h_root_topic);
        g_signal_handlers_disconnect_by_func(G_OBJECT(w->Shell()),
            (gpointer)h_menu_hdlr, (gpointer)w);
        delete w;
        HLP()->context()->quitHelp();
    }
    else if (activate == HA_OPEN)
        w->PopUpInput("Enter keyword, file name, or URL", "", "Open",
            h_open_cb, w);
    else if (activate == HA_FILE) {
        int ix;
        for (ix = 0; ix < 4; ix++)
            if (w->h_fsels[ix] == 0)
                break;
        if (ix == 4) {
            w->PopUpMessage("Too many file selectors open.", true);
            return;
        }
        GRloc loc(LW_XYR, 100 + ix*20, 100 + ix*20);
        w->h_fsels[ix] = w->PopUpFileSelector(fsSEL, loc, h_open_cb, 0, w, 0);
        if (w->h_fsels[ix])
            w->h_fsels[ix]->register_usrptr((void**)&w->h_fsels[ix]);
    }
    else if (activate == HA_BACK) {
        HLPtopic *top = w->h_root_topic;
        if (top->lastborn()) {
            HLPtopic *last = top->lastborn();
            top->set_lastborn(last->sibling());
            last->set_sibling(top->next());
            top->set_next(last);
            last->set_topline(w->scroll_position(false));
            top->reuse(top->lastborn(), false);
            GtkWidget *back = (GtkWidget*)
                g_object_get_data(G_OBJECT(w->Shell()), "back");
            if (back)
                gtk_widget_set_sensitive(back, top->lastborn() != 0);
            GtkWidget *forw = (GtkWidget*)
                g_object_get_data(G_OBJECT(w->Shell()), "forward");
            if (forw)
                gtk_widget_set_sensitive(forw, true);
       }
    }
    else if (activate == HA_FORWARD) {
        HLPtopic *top = w->h_root_topic;
        if (top->next()) {
            HLPtopic *next = top->next();
            top->set_next(next->sibling());
            next->set_sibling(top->lastborn());
            top->set_lastborn(next);
            if (next->sibling())
                next->sibling()->set_topline(w->scroll_position(false));
            else
                top->set_topline(w->scroll_position(false));
            top->reuse(top->lastborn(), false);
            GtkWidget *forw = (GtkWidget*)
                g_object_get_data(G_OBJECT(w->Shell()), "forward");
            if (forw)
                gtk_widget_set_sensitive(forw, top->next() != 0);
            GtkWidget *back = (GtkWidget*)
                g_object_get_data(G_OBJECT(w->Shell()), "back");
            if (back)
                gtk_widget_set_sensitive(back, true);
        }
    }
    else if (activate == HA_DUMPCFG) {
        if (!w->h_params->dump())
            w->PopUpErr(MODE_ON,
                "Failed to write .mozyrc file, permission problem?");
        else
            w->PopUpMessage("Saved .mozyrc file.", false);
    }
    else if (activate == HA_PROXY) {
        char *pxy = proxy::get_proxy();
        w->PopUpInput("Enter proxy url:", pxy, "Proxy", h_proxy_proc, w);
        delete [] pxy;
    }
    else if (activate == HA_SEARCH) {
        w->PopUpInput("Enter text for database search:", "",
            "Search", h_do_search_proc, w);
        if (w->wb_input)
            w->wb_input->set_no_drops(true);
    }
    else if (activate == HA_FIND) {
        if (!w->h_find_text)
            w->h_find_text = new GTKsearchPopup(caller, w->wb_shell,
                h_find_text_proc, w);
        if (GRX->GetStatus(caller))
            w->h_find_text->pop_up_search(MODE_ON);
        else
            w->h_find_text->pop_up_search(MODE_OFF);
    }
    else if (activate == HA_SAVE) {
        w->PopUpInput(0, "", "Save", h_do_save_proc, w);
        if (w->wb_input)
            w->wb_input->set_no_drops(true);
    }
    else if (activate == HA_PRINT) {
        if (!hlpHCcb.command)
            hlpHCcb.command = lstring::copy(GRappIf()->GetPrintCmd());
        w->PopUpPrint(caller, &hlpHCcb, HCtextPS);
    }
    else if (activate == HA_RELOAD) {
        if (w->h_cur_topic) {
            w->h_cur_topic->set_topline(w->scroll_position(false));
            w->newtopic(w->h_cur_topic->keyword(), false, false, true);
        }
    }
    else if (activate == HA_ISO8859) {
        w->h_viewer->set_iso8859_source(GRX->GetStatus(caller));
    }
    else if (activate == HA_MKFIFO) {
        if (GRX->GetStatus(caller)) {
            if (w->register_fifo(0)) {
                sLstr tstr;
                tstr.add("Listening for input on pipe named\n");
                tstr.add(w->h_fifo_name);
                w->PopUpMessage(tstr.string(), false);
            }
        }
        else
            w->unregister_fifo();
    }
    else if (activate == HA_COLORS) {
        if (GRX->GetStatus(caller)) {
            if (!Clr) {
                new sClr(caller);
                int x, y;
                GRX->ComputePopupLocation(GRloc(LW_UL), w->Shell(),
                    w->h_viewer->top_widget(), &x, &y);
                x += 200;
                y += 200;
                int mwid;
                MonitorGeom(0, 0, 0, &mwid, 0);
                GtkRequisition req;
                gtk_widget_get_requisition(Clr->Shell(), &req);
                if (x + req.width > mwid)
                    x = mwid - req.width;
                gtk_window_move(GTK_WINDOW(Clr->Shell()), x, y);
                if (w->h_parent) {
                    gtk_window_set_transient_for(GTK_WINDOW(Clr->Shell()),
                        GTK_WINDOW(w->h_parent));
                }
                gtk_widget_show(Clr->Shell());
            }
        }
        else {
            if (Clr)
                delete Clr;
        }
    }
    else if (activate == HA_FONT)
        h_fontsel(w, caller);
    else if (activate == HA_NOCACHE)
        w->h_params->NoCache = GRX->GetStatus(caller);
    else if (activate == HA_CLRCACHE)
        HLP()->context()->clearCache();
    else if (activate == HA_LDCACHE)
        HLP()->context()->reloadCache();
    else if (activate == HA_SHCACHE)
        w->show_cache(MODE_ON);
    else if (activate == HA_HELP) {
        if (GRX->MainFrame())
            GRX->MainFrame()->PopUpHelp("helpview");
        else
            w->PopUpHelp("helpview");
    }
    else if (activate == HA_NOCKS)
        w->h_params->NoCookies = GRX->GetStatus(caller);
    else if (activate == HA_NOIMG) {
        if (GRX->GetStatus(caller)) {
            w->stop_image_download();
            w->h_params->LoadMode = HLPparams::LoadNone;
        }
    }
    else if (activate == HA_SYIMG) {
        if (GRX->GetStatus(caller)) {
            w->stop_image_download();
            w->h_params->LoadMode = HLPparams::LoadSync;
        }
    }
    else if (activate == HA_DLIMG) {
        if (GRX->GetStatus(caller)) {
            w->stop_image_download();
            w->h_params->LoadMode = HLPparams::LoadDelayed;
        }
    }
    else if (activate == HA_PGIMG) {
        if (GRX->GetStatus(caller)) {
            w->stop_image_download();
            w->h_params->LoadMode = HLPparams::LoadProgressive;
        }
    }
    else if (activate == HA_APLN) {
        if (GRX->GetStatus(caller)) {
            int position = w->scroll_position(false);
            w->h_params->AnchorButtons = false;
            w->h_params->AnchorUnderlined = false;
            w->h_viewer->set_anchor_style(ANC_PLAIN);
            w->set_scroll_position(position, false);

        }
    }
    else if (activate == HA_ABUT) {
        if (GRX->GetStatus(caller)) {
            int position = w->scroll_position(false);
            w->h_params->AnchorButtons = true;
            w->h_params->AnchorUnderlined = false;
            w->h_viewer->set_anchor_style(ANC_BUTTON);
            w->set_scroll_position(position, false);
        }
    }
    else if (activate == HA_AUND) {
        if (GRX->GetStatus(caller)) {
            int position = w->scroll_position(false);
            w->h_params->AnchorButtons = false;
            w->h_params->AnchorUnderlined = true;
            w->h_viewer->set_anchor_style(ANC_SINGLE_LINE);
            w->set_scroll_position(position, false);
        }
    }
    else if (activate == HA_HLITE) {
        w->h_params->AnchorHighlight = GRX->GetStatus(caller);
        w->h_viewer->set_anchor_highlighting(w->h_params->AnchorHighlight);
    }
    else if (activate == HA_WARN) {
        w->h_params->BadHTMLwarnings = GRX->GetStatus(caller);
        w->h_viewer->set_html_warnings(w->h_params->BadHTMLwarnings);
    }
    else if (activate == HA_FREEZ) {
        w->h_params->FreezeAnimations = GRX->GetStatus(caller);
        w->h_viewer->set_freeze_animations(w->h_params->FreezeAnimations);
    }
    else if (activate == HA_COMM)
        w->h_params->PrintTransact = GRX->GetStatus(caller);
    else if (activate == HA_BMADD) {
        HLPtopic *tp = w->h_cur_topic;
        const char *ptitle = tp->title();
        char *title;
        if (ptitle && *ptitle)
            title = lstring::copy(ptitle);
        else
            title = w->h_viewer->get_title();
        char *url = lstring::copy(tp->keyword());
        if (!url || !*url) {
            delete [] url;
            delete [] title;
            return;
        }
        if (!title || !*title) {
            delete [] title;
            title = lstring::copy(url);
        }
        HLP()->context()->bookmarkUpdate(title, url);

        if (strlen(title) > 32)
            title[32] = 0;
        GtkWidget *item = gtk_menu_item_new_with_label(title);
        gtk_widget_set_name(item, title);
        gtk_widget_show(item);
        gtk_menu_shell_append(GTK_MENU_SHELL(w->h_bookmarks_menu), item);
        g_object_set_data_full(G_OBJECT(item), "data", url, h_bm_dest);
        g_signal_connect(G_OBJECT(item), "activate",
            G_CALLBACK(h_bm_handler), w);
    }
    else if (activate == HA_BMDEL) {
        g_object_set_data(G_OBJECT(w->Shell()), "bm_delete",
            (void*)GRX->GetStatus(caller) ? caller : 0);
    }
}


// Static function.
// Handler for bookmarks selected in the menu.  The user data "data"
// in the caller contains the url string.
//
void
GTKhelpPopup::h_bm_handler(GtkWidget *caller, void *hlpptr)
{
    char *url = (char*)g_object_get_data(G_OBJECT(caller), "data");
    if (!url)
        return;
    GTKhelpPopup *w = static_cast<GTKhelpPopup*>(hlpptr);
    if (!w)
        return;
    GtkWidget *delbtn = (GtkWidget*)g_object_get_data(G_OBJECT(w->Shell()),
        "bm_delete");
    if (delbtn) {
        // delete button is active, delete entry
        HLP()->context()->bookmarkUpdate(0, url);
        gtk_widget_destroy(caller);
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(delbtn), false);
        return;
    }
    w->newtopic(url, false, false, true);
}


// Static function.
// This catches a WM_DESTROY from the window manager, before the window
// delete is sent.   We ignore it, and call our own exit process.
//
int
GTKhelpPopup::h_destroy_hdlr(GtkWidget *w, GdkEvent*, void *hlpptr)
{
    h_menu_hdlr(w, hlpptr);
    return (1);
}


// Static function.
void
GTKhelpPopup::h_list_cb(const char *string, void *arg)
{
    GTKhelpPopup *w = (GTKhelpPopup*)arg;
    if (string) {
        lstring::advtok(&string);
        w->newtopic(string, false, false, true);
    }
}


// Static function.
// Callback for the "Open" and "Open File" menu commands, opens a new
// keyword or file.
//
void
GTKhelpPopup::h_open_cb(const char *name, void *hlpptr)
{
    if (name) {
        GTKhelpPopup *w = static_cast<GTKhelpPopup*>(hlpptr);
        if (!w)
            return;
        while (isspace(*name))
            name++;
        if (*name) {
            char *url = 0;
            const char *t = strrchr(name, '.');
            if (t) {
                t++;
                if (lstring::cieq(t, "html") || lstring::cieq(t, "htm") ||
                        lstring::cieq(t, "jpg") || lstring::cieq(t, "gif") ||
                        lstring::cieq(t, "png")) {
                    if (!lstring::is_rooted(name)) {
                        char *cwd = getcwd(0, 256);
                        if (cwd) {
                            url = new char[strlen(cwd) + strlen(name) + 2];
                            sprintf(url, "%s/%s", cwd, name);
                            free(cwd);
                            if (access(url, R_OK)) {
                                // no such file
                                delete [] url;
                                url = 0;
                            }
                        }
                    }
                }
            }
            if (!url)
                url = (char*)name;
            if (w->newtopic(url, false, false, true) != GTKhelpPopup::NTnone) {
                if (w->wb_input)
                    w->wb_input->popdown();
            }
        }
    }
}


// Static function.
// Callback passed to PopUpInput to actually perform a database keyword
// search.
//
void
GTKhelpPopup::h_do_search_proc(const char *target, void *hlpptr)
{
    GTKhelpPopup *w = static_cast<GTKhelpPopup*>(hlpptr);
    if (w) {
        if (target && *target) {
            ntop *n = new ntop(target, w->h_cur_topic);
            g_timeout_add(100, (GSourceFunc)h_ntop_timeout, n);
        }
        if (w->wb_input)
            w->wb_input->popdown();
    }
}


// Static function.
// Callback passed to PopUpInput to set a proxy url.
//
// The string is in the form "url [port]", where the port can be part
// of the url, separated by a colon.  In this case, the second token
// should not be given.  An explicit port number must be provided by
// either means.
//
void
GTKhelpPopup::h_proxy_proc(const char *str, void *hlpptr)
{
    GTKhelpPopup *w = static_cast<GTKhelpPopup*>(hlpptr);
    if (!w || !str)
        return;
    char buf[256];
    char *addr = lstring::getqtok(&str);

    // If not address given, convert to "-", which indicates to move
    // .wrproxy -> .wrproxy.bak
    if (!addr)
        addr = lstring::copy("-");
    else if (!*addr) {
        delete [] addr;
        addr = lstring::copy("-");
    }
    if (*addr == '-' || *addr == '+') {
        const char *err = proxy::move_proxy(addr);
        if (err) {
            snprintf(buf, 256, "Operation failed: %s.", err);
            w->PopUpErr(MODE_ON, buf);
        }
        else {
            const char *t = addr+1;
            if (!*t)
                t = "bak";
            if (*addr == '-') {
                snprintf(buf, 256,
                    "Move .wrproxy file to .wrproxy.%s succeeded.\n", t);
                w->PopUpMessage(buf, false);
            }
            else {
                snprintf(buf, 256,
                    "Move .wrproxy.%s to .wrproxy file succeeded.\n", t);
                w->PopUpMessage(buf, false);
            }
        }
        delete [] addr;
        if (w->wb_input)
            w->wb_input->popdown();
        return;
    }
    if (!lstring::prefix("http:", addr)) {
        w->PopUpMessage("Error: \"http:\" prefix required in address.", true);
        delete [] addr;
        if (w->wb_input)
            w->wb_input->popdown();
        return;
    }

    bool a_has_port = false;
    const char *e = strrchr(addr, ':');
    if (e) {
        e++;
        if (isdigit(*e)) {
            e++;
            while (isdigit(*e))
                e++;
        }
        if (!*e)
            a_has_port = true;
    }

    char *port = lstring::gettok(&str, ":");
    if (!a_has_port && !port) {
        // Default to port 80.
        port = lstring::copy("80");
    }
    if (port) {
        for (const char *c = port; *c; c++) {
            if (!isdigit(*c)) {
                w->PopUpMessage("Error: port is not numeric.", true);
                delete [] addr;
                delete [] port;
                if (w->wb_input)
                    w->wb_input->popdown();
                return;
            }
        }
    }

    const char *err = proxy::set_proxy(addr, port);
    if (err) {
        snprintf(buf, 256, "Operation failed: %s.", err);
        w->PopUpErr(MODE_ON, buf);
    }
    else if (port) {
        snprintf(buf, 256, "Created .wrproxy file for %s:%s.\n", addr, port);
        w->PopUpMessage(buf, false);
    }
    else {
        snprintf(buf, 256, "Created .wrproxy file for %s.\n", addr);
        w->PopUpMessage(buf, false);
    }
    delete [] addr;
    delete [] port;
    if (w->wb_input)
        w->wb_input->popdown();
}


// Static function.
// Search for the target text.  This will be highlighted and scrolled
// into view.
//
bool
GTKhelpPopup::h_find_text_proc(const char *target, bool up, bool case_insens,
    void *hlpptr)
{
    GTKhelpPopup *w = static_cast<GTKhelpPopup*>(hlpptr);
    if (w)
        return (w->h_viewer->find_words(target, up, case_insens));
    return (false);
}


// Static function.
// Callback passed to PopUpInput to actually save the text in a file.
//
void
GTKhelpPopup::h_do_save_proc(const char *fnamein, void *hlpptr)
{
    GTKhelpPopup *w = static_cast<GTKhelpPopup*>(hlpptr);
    if (w) {
        char *fname = pathlist::expand_path(fnamein, false, true);
        if (!fname)
            return;
        if (filestat::check_file(fname, W_OK) == NOGO) {
            w->PopUpMessage(filestat::error_msg(), true);
            delete [] fname;
            return;
        }

        FILE *fp = fopen(fname, "w");
        if (!fp) {
            char tbuf[256];
            if (strlen(fname) > 64)
                strcpy(fname + 60, "...");
            sprintf(tbuf, "Error: can't open file %s", fname);
            w->PopUpMessage(tbuf, true);
            delete [] fname;
            return;
        }
        char *tptr = w->h_viewer->get_plain_text();
        const char *mesg;
        if (tptr) {
            if (fputs(tptr, fp) == EOF) {
                w->PopUpMessage("Error: block write error", true);
                delete [] tptr;
                fclose(fp);
                delete [] fname;
                return;
            }
            delete [] tptr;
            mesg = "Text saved";
        }
        else
            mesg = "Text file is empty";

        fclose(fp);
        if (w->wb_input)
            w->wb_input->popdown();
        w->PopUpMessage(mesg, false);
        delete [] fname;
    }
}


// Static function.
// Callback for PopUpInput() sensitivity set.
//
void
GTKhelpPopup::h_sens_set(GTKbag *w, bool set, int)
{
    GtkWidget *search =
        (GtkWidget*)g_object_get_data(G_OBJECT(w->Shell()), "search");
    GtkWidget *save =
        (GtkWidget*)g_object_get_data(G_OBJECT(w->Shell()), "save");
    GtkWidget *open =
        (GtkWidget*)g_object_get_data(G_OBJECT(w->Shell()), "open");
    if (set) {
        if (search)
            gtk_widget_set_sensitive(search, true);
        if (save)
            gtk_widget_set_sensitive(save, true);
        if (open)
            gtk_widget_set_sensitive(open, true);
    }
    else {
        if (search)
            gtk_widget_set_sensitive(search, false);
        if (save)
            gtk_widget_set_sensitive(save, false);
        if (open)
            gtk_widget_set_sensitive(open, false);
    }
}


// Static function.
void
GTKhelpPopup::h_font_cb(const char *btn, const char *fname, void *arg)
{
    if (!btn && !fname) {
        GtkWidget *caller = GTK_WIDGET(arg);
        GRX->Deselect(caller);
    }
}


// Static function.
// Callback to destroy the url string user data when menu entries are
// destroyed.
//
void
GTKhelpPopup::h_bm_dest(void *arg)
{
    char *s = (char*)arg;
    delete [] s;
}


// Static function.
// Actually do the search/update.  Conceivably, the parent could be
// deleted during the interval, so there may be a little danger here.
//
int
GTKhelpPopup::h_ntop_timeout(void *data)
{
    ntop *n = (ntop*)data;
    HLPtopic *newtop = HLP()->search(n->kw);
    if (!newtop) {
        if (GRX->MainFrame())
            GRX->MainFrame()->PopUpErr(MODE_ON, "Unresolved link.");
    }
    else
        newtop->link_new_and_show(false, n->parent);
    delete n;
    return (false);
}


#define MOZY_FIFO "mozyfifo"

// Experimental new feature:  create a named pipe and set up a
// listener.  When anything is written to the pipe, grab it and show
// it.  This is intended for displaying HTML messages from an email
// client.
//
bool 
GTKhelpPopup::register_fifo(const char *fname)
{
    if (!fname)
        fname = getenv("MOZY_FIFO");
    if (!fname)
        fname = MOZY_FIFO;
        
    bool ret = false;
#ifdef WIN32
    if (fname)
        fname = lstring::strip_path(fname);
    sLstr lstr;
    lstr.add("\\\\.\\pipe\\");
    lstr.add(fname);
    int len = lstr.length();
    int cnt = 1;
    for (;;) {
        if (access(lstr.string(), F_OK) < 0)
            break;
        lstr.truncate(len, 0);
        lstr.add_i(cnt);
        cnt++;
    }
    SECURITY_DESCRIPTOR sd;
    InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = &sd;
    sa.bInheritHandle = false;

    HANDLE hpipe = CreateNamedPipe(lstr.string(),
        PIPE_ACCESS_INBOUND,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
        PIPE_UNLIMITED_INSTANCES,
        2048,
        2048,
        NMPWAIT_USE_DEFAULT_WAIT,
        &sa);

    if (hpipe != INVALID_HANDLE_VALUE) {
        ret = true;
        delete [] h_fifo_name;
        h_fifo_name = lstr.string_trim();
        h_fifo_pipe = hpipe;
        _beginthread(pipe_thread_proc, 0, this);
    }
#else
    sLstr lstr;
    passwd *pw = getpwuid(getuid());
    if (pw == 0) {
        GRpkgIf()->Perror("getpwuid");
        char *cwd = getcwd(0, 0);
        lstr.add(cwd);
        if (strcmp(cwd, "/"))
            lstr.add_c('/');
        free(cwd);
    }
    else {
        lstr.add(pw->pw_dir);
        lstr.add_c('/');
    }
    lstr.add(fname);
    int len = lstr.length();
    int cnt = 1;
    for (;;) {
        if (access(lstr.string(), F_OK) < 0)
            break;
        lstr.truncate(len, 0);
        lstr.add_i(cnt);
        cnt++;
    }
    if (!mkfifo(lstr.string(), 0666)) {
        ret = true;
        delete [] h_fifo_name;
        h_fifo_name = lstr.string_trim();
        filestat::queue_deletion(h_fifo_name);
    }
#endif

    if (h_fifo_tid)
        g_source_remove(h_fifo_tid);
    h_fifo_tid = g_timeout_add(100, fifo_check_proc, this);

    return (ret);
}


void 
GTKhelpPopup::unregister_fifo()
{
    if (h_fifo_tid) {
        g_source_remove(h_fifo_tid);
        h_fifo_tid = 0;
    }
    if (h_fifo_name) {
#ifdef WIN32
        unlink(h_fifo_name);
        delete [] h_fifo_name;
        h_fifo_name = 0;
        if (h_fifo_pipe) {
            CloseHandle(h_fifo_pipe);
            h_fifo_pipe = 0;
        }
#else
        if (h_fifo_fd > 0)
            close(h_fifo_fd);
        h_fifo_fd = -1;
        unlink(h_fifo_name);
        delete [] h_fifo_name;
        h_fifo_name = 0;
#endif
    }
#ifdef WIN32
    stringlist *sl = h_fifo_tfiles;
    h_fifo_tfiles = 0;
    while (sl) {
        stringlist *sx = sl;
        sl = sl->next;
        unlink(sx->string);
        delete [] sx->string;
        delete sx;
    }
#endif
}


#ifdef WIN32

// Static function.
// Thread procedure to listen on the named pipe.
//
void
GTKhelpPopup::pipe_thread_proc(void *arg)
{
    GTKhelpPopup *hw = (GTKhelpPopup*)arg;
    if (!hw)
        return;

    for (;;) {
        HANDLE hpipe = hw->h_fifo_pipe;
        if (!hpipe)
            return;

        if (ConnectNamedPipe(hpipe, 0)) {

            if (hw->h_fifo_name) {
                char *tempfile = filestat::make_temp("mz");
                FILE *fp = fopen(tempfile, "w");
                if (fp) {
                    unsigned int total = 0;
                    char buf[2048];
                    for (;;) {
                        DWORD bytes_read;
                        bool ok = ReadFile(hpipe, buf, 2048, &bytes_read, 0);
                        if (!ok)
                            break;
                        if (bytes_read > 0) {
                            fwrite(buf, 1, bytes_read, fp);
                            total += bytes_read;
                        }
                    }
                    fclose(fp);
                    if (total > 0) {
                        hw->h_fifo_tfiles =
                            new stringlist(tempfile, hw->h_fifo_tfiles);
                        tempfile = 0;
                    }
                }
                else
                    perror(tempfile);
                delete [] tempfile;
            }
            DisconnectNamedPipe(hpipe);
        }
    }
}

#endif


// Static timer callback function.
//
int 
GTKhelpPopup::fifo_check_proc(void *arg)
{
    GTKhelpPopup *hw = (GTKhelpPopup*)arg;
    if (!hw)
        return (0);

#ifdef WIN32
    if (hw->h_fifo_tfiles) {
        stringlist *sl = hw->h_fifo_tfiles;
        hw->h_fifo_tfiles = sl->next;
        char *tempfile = sl->string;
        delete sl;
        FILE *fp = fopen(tempfile, "r");
        if (fp) {
            hw->newtopic("fifo", fp, false);
            fclose(fp);
        }
        unlink(tempfile);
        delete [] tempfile;
    }
    return (1);

#else
    // Unfortunately, the stat and open calls appear to fail with
    // Mingw, so can't use this.

    struct stat st;
    if (stat(hw->h_fifo_name, &st) < 0 || !(st.st_mode & S_IFIFO))
        return (1);

    if (hw->h_fifo_fd < 0) {
        if (hw->h_fifo_name)
            hw->h_fifo_fd = open(hw->h_fifo_name, O_RDONLY | O_NONBLOCK);
        if (hw->h_fifo_fd < 0) {
            hw->h_fifo_tid = 0;
            return (0);
        }
    }
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(hw->h_fifo_fd, &readfds);
    timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 500;
    int i = select(hw->h_fifo_fd + 1, &readfds, 0, 0, &timeout);
    if (i < 0) {
        // interrupted
        return (1);
    }
    if (i == 0) {
        // nothing to read
        // return (1);
    }
    if (FD_ISSET(hw->h_fifo_fd, &readfds)) {
        FILE *fp = fdopen(hw->h_fifo_fd, "r");
        if (!fp) {
            // Something wrong, close the fd and try again.
            close (hw->h_fifo_fd);
            hw->h_fifo_fd = -1;
            return (1);
        }
        hw->newtopic("fifo", fp, false);
        fclose(fp);  // closes fd, too
        hw->h_fifo_fd = -1;
    }
    return (1);
#endif
}
// End of GTKhelpPopup functions


//
// The Default Colors entry widget.  This allows changing the colors
// used when there is no <body> tag, and some attribute colors.
//

sClr::sClr(GRobject caller)
{
    Clr = this;
    clr_caller = caller;
    clr_bgclr = 0;
    clr_bgimg = 0;
    clr_text = 0;
    clr_link = 0;
    clr_vislink = 0;
    clr_actfglink = 0;
    clr_imgmap = 0;
    clr_sel = 0;
    clr_listbtn = 0;
    clr_listpop = 0;
    clr_selection = 0;

    wb_shell = gtk_NewPopup(0, "Default Colors", clr_cancel_proc, 0);
    if (!wb_shell)
        return;

    GtkWidget *form = gtk_table_new(2, 1, false);
    gtk_widget_show(form);
    gtk_container_set_border_width(GTK_CONTAINER(form), 2);
    gtk_container_add(GTK_CONTAINER(wb_shell), form);
    int rowcnt = 0;

    GtkWidget *label = gtk_label_new("Background color");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);

    gtk_table_attach(GTK_TABLE(form), label, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    GtkWidget *entry = gtk_entry_new();
    gtk_widget_set_name(entry, "defbg");
    gtk_widget_show(entry);
    clr_bgclr = entry;

    gtk_table_attach(GTK_TABLE(form), entry, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    label = gtk_label_new("Background image");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);

    gtk_table_attach(GTK_TABLE(form), label, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    entry = gtk_entry_new();
    gtk_widget_set_name(entry, "defimg");
    gtk_widget_show(entry);
    clr_bgimg = entry;

    gtk_table_attach(GTK_TABLE(form), entry, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    label = gtk_label_new("Text color");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);

    gtk_table_attach(GTK_TABLE(form), label, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    entry = gtk_entry_new();
    gtk_widget_set_name(entry, "deffg");
    gtk_widget_show(entry);
    clr_text = entry;

    gtk_table_attach(GTK_TABLE(form), entry, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    label = gtk_label_new("Link color");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);

    gtk_table_attach(GTK_TABLE(form), label, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    entry = gtk_entry_new();
    gtk_widget_set_name(entry, "deflink");
    gtk_widget_show(entry);
    clr_link = entry;

    gtk_table_attach(GTK_TABLE(form), entry, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    label = gtk_label_new("Visited link color");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);

    gtk_table_attach(GTK_TABLE(form), label, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    entry = gtk_entry_new();
    gtk_widget_set_name(entry, "defvislink");
    gtk_widget_show(entry);
    clr_vislink = entry;

    gtk_table_attach(GTK_TABLE(form), entry, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    label = gtk_label_new("Activated link color");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);

    gtk_table_attach(GTK_TABLE(form), label, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    entry = gtk_entry_new();
    gtk_widget_set_name(entry, "deftrglink");
    gtk_widget_show(entry);
    clr_actfglink = entry;

    gtk_table_attach(GTK_TABLE(form), entry, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    label = gtk_label_new("Select color");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);

    gtk_table_attach(GTK_TABLE(form), label, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    entry = gtk_entry_new();
    gtk_widget_set_name(entry, "defsel");
    gtk_widget_show(entry);
    clr_sel = entry;

    gtk_table_attach(GTK_TABLE(form), entry, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    label = gtk_label_new("Imagemap border color");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);

    gtk_table_attach(GTK_TABLE(form), label, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    entry = gtk_entry_new();
    gtk_widget_set_name(entry, "deftrglink");
    gtk_widget_show(entry);
    clr_imgmap = entry;

    gtk_table_attach(GTK_TABLE(form), entry, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    GtkWidget *hbox = gtk_hbox_new(false, 2);
    gtk_widget_show(hbox);

    GtkWidget *button = gtk_toggle_button_new_with_label("Colors");
    gtk_widget_set_name(button, "Colors");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(clr_action_proc), 0);
    gtk_box_pack_start(GTK_BOX(hbox), button, false, false, 0);
    clr_listbtn = button;

    button = gtk_button_new_with_label("Apply");
    gtk_widget_set_name(button, "Apply");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(clr_action_proc), 0);
    gtk_box_pack_start(GTK_BOX(hbox), button, true, true, 0);

    button = gtk_button_new_with_label("Dismiss");
    gtk_widget_set_name(button, "Dismiss");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(clr_cancel_proc), 0);
    gtk_box_pack_start(GTK_BOX(hbox), button, true, true, 0);

    gtk_table_attach(GTK_TABLE(form), hbox, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    update();
}


sClr::~sClr()
{
    Clr = 0;
    if (clr_caller)
        GRX->Deselect(clr_caller);
    delete [] clr_selection;
}


#define fxp(c) (c ? c : "")

void
sClr::update()
{
    HLPcontext *cx = HLP()->context();
    const char *cc = cx->getDefaultColor(HLP_DefaultBgColor);
    const char *ent = gtk_entry_get_text(GTK_ENTRY(clr_bgclr));
    if (strcmp(fxp(cc), fxp(ent)))
        gtk_entry_set_text(GTK_ENTRY(clr_bgclr), cc);

    cc = cx->getDefaultColor(HLP_DefaultBgImage);
    ent = gtk_entry_get_text(GTK_ENTRY(clr_bgimg));
    if (strcmp(fxp(cc), fxp(ent)))
        gtk_entry_set_text(GTK_ENTRY(clr_bgimg), cc);

    cc = cx->getDefaultColor(HLP_DefaultFgText);
    ent = gtk_entry_get_text(GTK_ENTRY(clr_text));
    if (strcmp(fxp(cc), fxp(ent)))
        gtk_entry_set_text(GTK_ENTRY(clr_text), cc);

    cc = cx->getDefaultColor(HLP_DefaultFgLink);
    ent = gtk_entry_get_text(GTK_ENTRY(clr_link));
    if (strcmp(fxp(cc), fxp(ent)))
        gtk_entry_set_text(GTK_ENTRY(clr_link), cc);

    cc = cx->getDefaultColor(HLP_DefaultFgVisitedLink);
    ent = gtk_entry_get_text(GTK_ENTRY(clr_vislink));
    if (strcmp(fxp(cc), fxp(ent)))
        gtk_entry_set_text(GTK_ENTRY(clr_vislink), cc);

    cc = cx->getDefaultColor(HLP_DefaultFgActiveLink);
    ent = gtk_entry_get_text(GTK_ENTRY(clr_actfglink));
    if (strcmp(fxp(cc), fxp(ent)))
        gtk_entry_set_text(GTK_ENTRY(clr_actfglink), cc);

    cc = cx->getDefaultColor(HLP_DefaultBgSelect);
    ent = gtk_entry_get_text(GTK_ENTRY(clr_sel));
    if (strcmp(fxp(cc), fxp(ent)))
        gtk_entry_set_text(GTK_ENTRY(clr_sel), cc);

    cc = cx->getDefaultColor(HLP_DefaultFgImagemap);
    ent = gtk_entry_get_text(GTK_ENTRY(clr_imgmap));
    if (strcmp(fxp(cc), fxp(ent)))
        gtk_entry_set_text(GTK_ENTRY(clr_imgmap), cc);
}


// Static function.
void
sClr::clr_cancel_proc(GtkWidget*, void*)
{
    delete Clr;
}


// Static function.
void
sClr::clr_action_proc(GtkWidget *caller, void*)
{
    const char *name = gtk_widget_get_name(caller);
    if (!name)
        return;
    if (!Clr)
        return;
    if (!strcmp(name, "Apply")) {
        bool body_chg = false;
        HLPcontext *cx = HLP()->context();

        const char *cc = cx->getDefaultColor(HLP_DefaultBgImage);
        const char *ent = gtk_entry_get_text(GTK_ENTRY(Clr->clr_bgimg));
        if (strcmp(fxp(cc), fxp(ent))) {
            cx->setDefaultColor(HLP_DefaultBgImage, ent);
            body_chg = true;
        }

        cc = cx->getDefaultColor(HLP_DefaultBgColor);
        ent = gtk_entry_get_text(GTK_ENTRY(Clr->clr_bgclr));
        if (strcmp(fxp(cc), fxp(ent))) {
            cx->setDefaultColor(HLP_DefaultBgColor, ent);
            body_chg = true;
        }

        cc = cx->getDefaultColor(HLP_DefaultFgText);
        ent = gtk_entry_get_text(GTK_ENTRY(Clr->clr_text));
        if (strcmp(fxp(cc), fxp(ent))) {
            cx->setDefaultColor(HLP_DefaultFgText, ent);
            body_chg = true;
        }

        cc = cx->getDefaultColor(HLP_DefaultFgLink);
        ent = gtk_entry_get_text(GTK_ENTRY(Clr->clr_link));
        if (strcmp(fxp(cc), fxp(ent))) {
            cx->setDefaultColor(HLP_DefaultFgLink, ent);
            body_chg = true;
        }

        cc = cx->getDefaultColor(HLP_DefaultFgVisitedLink);
        ent = gtk_entry_get_text(GTK_ENTRY(Clr->clr_vislink));
        if (strcmp(fxp(cc), fxp(ent))) {
            cx->setDefaultColor(HLP_DefaultFgVisitedLink, ent);
            body_chg = true;
        }

        cc = cx->getDefaultColor(HLP_DefaultFgActiveLink);
        ent = gtk_entry_get_text(GTK_ENTRY(Clr->clr_actfglink));
        if (strcmp(fxp(cc), fxp(ent))) {
            cx->setDefaultColor(HLP_DefaultFgActiveLink, ent);
            body_chg = true;
        }

        // The keys above operate through inclusion of a <body> tag
        // that sets background color or image, which is added to HTML
        // text that has no body tag.  This probably includes the
        // user's help text.  The keys below operate by actually
        // changing the color definitions in the widget.

        cc = cx->getDefaultColor(HLP_DefaultBgSelect);
        ent = gtk_entry_get_text(GTK_ENTRY(Clr->clr_sel));
        if (strcmp(fxp(cc), fxp(ent))) {
            cx->setDefaultColor(HLP_DefaultBgSelect, ent);
            for (HLPtopic *t = HLP()->context()->topList(); t;
                    t = t->sibling()) {
                GTKhelpPopup *w = (GTKhelpPopup*)HelpWidget::get_widget(t);
                w->viewer()->set_select_color(ent);
            }
        }

        cc = cx->getDefaultColor(HLP_DefaultFgImagemap);
        ent = gtk_entry_get_text(GTK_ENTRY(Clr->clr_imgmap));
        if (strcmp(fxp(cc), fxp(ent))) {
            cx->setDefaultColor(HLP_DefaultFgImagemap, ent);
            for (HLPtopic *t = HLP()->context()->topList(); t;
                    t = t->sibling()) {
                GTKhelpPopup *w = (GTKhelpPopup*)HelpWidget::get_widget(t);
                w->viewer()->set_imagemap_boundary_color(ent);
            }
        }

        // Uodate all the help windows present.  The topic actually
        // shown is the topic::lastborn() if it is not null.

        for (HLPtopic *t = HLP()->context()->topList(); t; t = t->sibling()) {
            GTKhelpPopup *w = (GTKhelpPopup*)HelpWidget::get_widget(t);
            if (body_chg) {
                // The changed body tag will cause a re-parse and redisplay.
                HLPtopic *ct = t->lastborn();
                if (!ct)
                    ct = t;
                ct->load_text();
                w->viewer()->set_source(ct->get_cur_text());
            }
            else
                // This reformats and redisplays.
                w->viewer()->redisplay_view();
        }
    }
    else if (!strcmp(name, "Colors")) {
        bool state = GRX->GetStatus(caller);
        if (!Clr->clr_listpop && state) {
            stringlist *list = GRcolorList::listColors();
            if (!list) {
                GRX->SetStatus(caller, false);
                return;
            }
            Clr->clr_listpop = Clr->PopUpList(list, "Colors",
                "click to select, color name to clipboard",
                clr_list_callback, 0, false, false);
            stringlist::destroy(list);
            if (Clr->clr_listpop)
                Clr->clr_listpop->register_usrptr((void**)&Clr->clr_listpop);
        }
        else if (Clr->clr_listpop && !state)
            Clr->clr_listpop->popdown();
    }
}


// Insert the selected rgb.txt entry into the color selector.
//
void
sClr::clr_list_callback(const char *string, void*)
{
    if (string) {
        // Put the color name in the "primary" clipboard, so we can
        // paste it into the entry areas.

        lstring::advtok(&string);
        lstring::advtok(&string);
        lstring::advtok(&string);
        gtk_clipboard_set_text(gtk_clipboard_get(GDK_SELECTION_PRIMARY),
            string, strlen(string));
        gtk_clipboard_store(gtk_clipboard_get(GDK_SELECTION_PRIMARY));
    }
    else if (Clr) {
        GRX->SetStatus(Clr->clr_listbtn, false);
        Clr->clr_listpop = 0;
    }
}


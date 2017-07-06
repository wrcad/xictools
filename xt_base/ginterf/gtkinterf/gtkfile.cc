
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2005 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *                                                                        *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,      *
 *   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES      *
 *   OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-        *
 *   INFRINGEMENT.  IN NO EVENT SHALL STEPHEN R. WHITELEY BE LIABLE       *
 *   FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION      *
 *   OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN           *
 *   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN         *
 *   THE SOFTWARE.                                                        *
 *                                                                        *
 * This library is available for non-commercial use under the terms of    *
 * the GNU Library General Public License as published by the Free        *
 * Software Foundation; either version 2 of the License, or (at your      *
 * option) any later version.                                             *
 *                                                                        *
 * A commercial license is available to those wishing to use this         *
 * library or a derivative work for commercial purposes.  Contact         *
 * Whiteley Research Inc., 456 Flora Vista Ave., Sunnyvale, CA 94086.     *
 * This provision will be enforced to the fullest extent possible under   *
 * law.                                                                   *
 *                                                                        *
 * This library is distributed in the hope that it will be useful,        *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU      *
 * Library General Public License for more details.                       *
 *                                                                        *
 * You should have received a copy of the GNU Library General Public      *
 * License along with this library; if not, write to the Free             *
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.     *
 *========================================================================*
 *                                                                        *
 * GtkInterf Graphical Interface Library                                  *
 *                                                                        *
 *========================================================================*
 $Id: gtkfile.cc,v 2.128 2017/04/16 20:27:51 stevew Exp $
 *========================================================================*/

#include "config.h"
#include "gtkinterf.h"
#include "gtkutil.h"
#include "gtkfile.h"
#include "gtkfont.h"
#include "lstring.h"
#include "pathlist.h"
#include "filestat.h"
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>
#include <ctype.h>
#ifdef HAVE_FNMATCH_H
#include <fnmatch.h>
#else
#ifdef WIN32
// This is in the mingw library, but there is no prototype.
extern "C" { extern int fnmatch(const char*, const char*, int); }
#endif
#endif

// Define this to omit the open/closed icons.
// #define NO_ICONS

//
// File selection pop-up.  The panel consists of a CTree which maintains
// a visual representation of the directory hierarchy, and a text window
// for listing files.  Selecting a directory in the tree will display
// the files in the list window, where they can be selected for operations.
// Both windows are drag sources and drop sites.
//

// Help keywords used in this file:
// filesel

namespace {
    GtkTargetEntry target_table[] = {
      { (char*)"STRING",     0, 0 },
      { (char*)"text/plain", 0, 1 }
    };
    guint n_targets = sizeof(target_table) / sizeof(target_table[0]);

    // XPM
    const char *up_xpm[] = {
    "32 16 3 1",
    "     c none",
    ".    c blue",
    "x    c sienna",
    "                                ",
    "                                ",
    "                                ",
    "                                ",
    "               .                ",
    "              ...x              ",
    "             .....x             ",
    "            .......x            ",
    "           .........x           ",
    "          ...........x          ",
    "         .............x         ",
    "        ...............x        ",
    "        x.............xx        ",
    "         xxxxxxxxxxxxxx         ",
    "                                ",
    "                                "};

    // XPM
    const char * go_xpm[] = {
    "32 18 4 1",
    "   c none",
    ".  c lightgreen",
    "x  c white",
    "+  c black",
    "                                ",
    "             xxxxxx             ",
    "            x......x            ",
    "           x........x           ",
    "          x..........x          ",
    "         x............x         ",
    "        x..............x        ",
    "        x..............x        ",
    "        x..............x        ",
    "        x..............x        ",
    "        x..............x        ",
    "        x..............x        ",
    "         +............+         ",
    "          +..........+          ",
    "           +........+           ",
    "            +......+            ",
    "             ++++++             ",
    "                                "};
}

namespace gtkinterf {
    struct GTKfsMon : public GRmonList
    {
        char *any_selection();
    };
}

namespace {
    // Keep a list of all active file selection pop-ups so we can find
    // selected text.
    //
    GTKfsMon FSmonitor;
}


// Return the selection from any file selection pop-up.  The window
// manager probably allows only one selection.
//
char *
GTKfsMon::any_selection()
{
    for (elt *e = list; e; e = e->next) {
        GTKfilePopup *fs = static_cast<GTKfilePopup*>(e->obj);
        if (fs) {
            char *s = fs->get_selection();
            if (s)
                return (s);
        }
    }
    return (0);
}
// End of GTKfsMon functions.


namespace gtkinterf {
    enum PIXBstate { PIXBnone, PIXBgray, PIXBactive};

    // Take care of bitmap presentation.
    //
    struct sFsBmap
    {
        sFsBmap(GTKfilePopup*);
        ~sFsBmap();
        void enable(GTKfilePopup*, int);
        void disable(GTKfilePopup*, int);

        GtkWidget *up;
        GtkWidget *go;
        GdkPixbuf *up_pb;
        GdkPixbuf *up_gray_pb;
        GdkPixbuf *go_pb;
        GdkPixbuf *go_gray_pb;
        GdkPixbuf *open_pb;
        GdkPixbuf *close_pb;
        PIXBstate up_state;
        PIXBstate go_state;
        bool no_disable_go;
    };
}


// Careful! this assumes some things about the above xpm structure.
// Create the pixmap widgets and the grayed versions.
//
sFsBmap::sFsBmap(GTKfilePopup *fs)
{
    no_disable_go = false;

    up_pb = gdk_pixbuf_new_from_xpm_data(up_xpm);
    const char *tmp = up_xpm[2];
    up_xpm[2] = ".  c gray";
    up_gray_pb = gdk_pixbuf_new_from_xpm_data(up_xpm);
    up_xpm[2] = tmp;
    up = gtk_image_new_from_pixbuf(up_pb);

    go_pb = gdk_pixbuf_new_from_xpm_data(go_xpm);
    tmp = go_xpm[2];
    go_xpm[2] = ".  c gray";
    go_gray_pb = gdk_pixbuf_new_from_xpm_data(go_xpm);
    go_xpm[2] = tmp;
    go = gtk_image_new_from_pixbuf(go_pb);

    up_state = PIXBnone;
    go_state = PIXBnone;

    // pixbufs for directory tree
    open_pb = gdk_pixbuf_new_from_xpm_data(fs->wb_open_folder_xpm);
    close_pb = gdk_pixbuf_new_from_xpm_data(fs->wb_closed_folder_xpm);

};


sFsBmap::~sFsBmap()
{
    gtk_widget_destroy(up);
    gtk_widget_destroy(go);
    if (up_pb)
        g_object_unref(up_pb);
    if (up_gray_pb)
        g_object_unref(up_gray_pb);
    if (go_pb)
        g_object_unref(go_pb);
    if (go_gray_pb)
        g_object_unref(go_gray_pb);
    if (open_pb)
        g_object_unref(open_pb);
    if (close_pb)
        g_object_unref(close_pb);
}


// Place the button in the enabled state.
//
void
sFsBmap::enable(GTKfilePopup *fs, int id)
{
    if (!GTK_BIN(fs->fs_up_btn)->child) {
        gtk_container_add(GTK_CONTAINER(fs->fs_up_btn), up);
        gtk_widget_show(up);
    }
    if (!GTK_BIN(fs->fs_go_btn)->child) {
        gtk_container_add(GTK_CONTAINER(fs->fs_go_btn), go);
        gtk_widget_show(go);
    }
    if (id == 0) {
        if (fs->fs_up_btn && up_state != PIXBactive) {
            GtkImage *im = GTK_IMAGE(GTK_BIN(fs->fs_up_btn)->child);
            gtk_image_set_from_pixbuf(im, up_pb);
            gtk_widget_set_sensitive(fs->fs_up_btn, true);
            up_state = PIXBactive;
        }
    }
    else {
        if (fs->fs_go_btn && !no_disable_go && go_state != PIXBactive) {
            GtkImage *im = GTK_IMAGE(GTK_BIN(fs->fs_go_btn)->child);
            gtk_image_set_from_pixbuf(im, go_pb);
            gtk_widget_set_sensitive(fs->fs_go_btn, true);
            go_state = PIXBactive;
        }
    }
}


// Place the button in the disabled state.
//
void
sFsBmap::disable(GTKfilePopup *fs, int id)
{
    if (!GTK_BIN(fs->fs_up_btn)->child) {
        gtk_container_add(GTK_CONTAINER(fs->fs_up_btn), up);
        gtk_widget_show(up);
    }
    if (!GTK_BIN(fs->fs_go_btn)->child) {
        gtk_container_add(GTK_CONTAINER(fs->fs_go_btn), go);
        gtk_widget_show(go);
    }
    if (id == 0) {
        if (fs->fs_up_btn && up_state != PIXBgray) {
            GtkImage *im = GTK_IMAGE(GTK_BIN(fs->fs_up_btn)->child);
            gtk_image_set_from_pixbuf(im, up_gray_pb);
            gtk_widget_set_sensitive(fs->fs_up_btn, false);
            up_state = PIXBgray;
        }
    }
    else {
        if (fs->fs_go_btn && !no_disable_go && go_state != PIXBgray) {
            GtkImage *im = GTK_IMAGE(GTK_BIN(fs->fs_go_btn)->child);
            gtk_image_set_from_pixbuf(im, go_gray_pb);
            gtk_widget_set_sensitive(fs->fs_go_btn, false);
            go_state = PIXBgray;
        }
    }
}
// End of sFsBmap functions


// Method to access file browser via a gtk_bag.
//
GRfilePopup *
gtk_bag::PopUpFileSelector(FsMode mode, GRloc loc,
    void(*cb)(const char*, void*), void(*quit)(GRfilePopup*, void*),
    void *arg, const char *name)
{
    GTKfilePopup *fs = new GTKfilePopup(this, mode, arg, name);
    fs->register_callback(cb);
    fs->register_quit_callback(quit);

    gtk_window_set_transient_for(GTK_WINDOW(fs->wb_shell),
        GTK_WINDOW(wb_shell));
    GRX->SetPopupLocation(loc, fs->wb_shell, wb_shell);
    gtk_widget_show(fs->wb_shell);

    return (fs);
}


namespace {
    // Return the name of the current root directory.
    const char *cur_root()
    {
#ifdef WIN32
        static char *msw_curdir;

        // In Windows, this can be a drive letter followed by a colon
        // and a trailing separator, or //servername/sharename/.

        char *tbuf = lstring::tocpp(getcwd(0, 0));
        delete [] msw_curdir;
        msw_curdir = tbuf;

        if (isalpha(tbuf[0]) && tbuf[1] == ':' &&
                lstring::is_dirsep(tbuf[2])) {
            tbuf[3] = 0;
            return (tbuf);
        }
        if (lstring::is_dirsep(tbuf[0]) && lstring::is_dirsep(tbuf[1])) {
            char *t = tbuf + 2;
            t = lstring::strdirsep(t);
            if (t)
                t = lstring::strdirsep(t+1);
            if (t) {
                *++t = 0;
                return (tbuf);
            }
        }
        // Something strange, shouldn't happen.
        delete [] msw_curdir;
        msw_curdir = 0;
#endif
        return ("/");
    }

    // Return true if dir names a root directory.
    //
    bool is_root(const char *dir)
    {
#ifdef WIN32
        if (isalpha(dir[0]) && dir[1] == ':') {
            if (!dir[2])
                return (true);
            if (lstring::is_dirsep(dir[2]) && !dir[3])
                return (true);
            return (false);
        }
        if (lstring::is_dirsep(dir[0]) && lstring::is_dirsep(dir[1])) {
            const char *t = dir + 2;
            t = lstring::strdirsep(t);
            if (t && t[1])
                t = lstring::strdirsep(t+1);
            if (!t)
                return (true);
            if (!t[1])
                return (true);
            return (false);
        }
#endif
        return (lstring::is_dirsep(dir[0]) && !dir[1]);
    }


    // Add a trailing dirsep to a root directory token if needed.
    //
    void fix_root(char **p)
    {
#ifdef WIN32
        char *dir = *p;
        if (isalpha(dir[0]) && dir[1] == ':') {
            if (!dir[2]) {
                char *nd = new char[4];
                nd[0] = dir[0];
                nd[1] = ':';
                nd[2] = '\\';
                nd[3] = 0;
                delete [] dir;
                *p = nd;
                return;
            }
        }
        else if (lstring::is_dirsep(dir[0]) && lstring::is_dirsep(dir[1])) {
            const char *t = dir + 2;
            t = lstring::strdirsep(t);
            if (t && t[1])
                t = lstring::strdirsep(t+1);
            if (!t) {
                char *nd = new char[strlen(dir) + 2];
                char *e = lstring::stpcpy(nd, dir);
                *e++ = '\\';
                *e = 0;
                delete [] dir;
                *p = nd;
                return;
            }
        }
#else
        (void)p;
#endif
    }
}


//-----------------------------------------------------------------------------
// GTKfilePopup functions
//

// Initial "show label" states
bool GTKfilePopup::fs_sel_show_label = true;   // show label in fsSEL
bool GTKfilePopup::fs_open_show_label = false; // don't show label in fsOPEN


// The default strings for the filter combo.  The first two are not
// editable, the rest can be set arbitrarily by the user.
//
const char *GTKfilePopup::fs_filter_options[] =
{
    "all:",
    "archive: *.cif *.cgx *.gds *.oas *.strm *.stream",
    0,
    0,
    0,
    0,
    0
};

namespace gtkinterf {
    // Menu callback codes.
    enum
    {
        fsOpen,
        fsNew,
        fsDelete,
        fsRename,
        fsRoot,
        fsCwd,
        fsFilt,
        fsRelist,
        fsMtime,
        fsLabel,
        fsHelp
    };
}


// Multi-purpose object used to pass data to callbacks.
//
struct fs_data
{
    fs_data(GTKfilePopup *f, char *s) { fs = f; string = s; }
    ~fs_data() { delete [] string; }

    GTKfilePopup *fs;
    char *string;
};


// Static function - obtain a selection from the open file
// selectors.
//
char *
GTKfilePopup::any_selection()
{
    return (FSmonitor.any_selection());
}


#define IFINIT(i, a, b, c, d, e) { \
    menu_items[i].path = (char*)a; \
    menu_items[i].accelerator = (char*)b; \
    menu_items[i].callback = (GtkItemFactoryCallback)c; \
    menu_items[i].callback_action = d; \
    menu_items[i].item_type = (char*)e; \
    i++; }

#define DEF_TEXT_USWIDTH 300


GTKfilePopup::GTKfilePopup(gtk_bag *owner, FsMode mode, void *arg,
    const char *root_or_fname)
{
    p_parent = owner;
    p_cb_arg = arg;
    fs_tree = 0;
    fs_label = 0;
    fs_label_frame = 0;
    fs_entry = 0;
    fs_up_btn = 0;
    fs_go_btn = 0;
    fs_open_btn = 0;
    fs_new_btn = 0;
    fs_delete_btn = 0;
    fs_rename_btn = 0;
    fs_anc_btn = 0;
    fs_filter = 0;
    fs_scrwin = 0;
    fs_item_factory = 0;

    fs_bmap = 0;
    fs_curnode = 0;
    fs_drag_node = 0;
    fs_cset_node = 0;
    fs_rootdir = 0;
    fs_curfile = 0;
    fs_cwd_bak = lstring::tocpp(getcwd(0, 0));
    fs_colwid = 0;

    fs_type = mode;
    fs_mtime = 0;
    fs_timer_tag = 0;
    fs_filter_index = 0;
    fs_alloc_width = 0;
    fs_drag_btn = 0;
    fs_drag_x = fs_drag_y = 0;
    fs_start = 0;
    fs_end = 0;
    fs_vtimer = 0;
    fs_tid = 0;
    fs_dragging = false;
    fs_mtime_sort = false;

    if (owner)
        owner->MonitorAdd(this);
    FSmonitor.add(this);

    // initialize editable filter lines
    if (!fs_filter_options[2])
        fs_filter_options[2] = lstring::copy("user1:");
    if (!fs_filter_options[3])
        fs_filter_options[3] = lstring::copy("user2:");
    if (!fs_filter_options[4])
        fs_filter_options[4] = lstring::copy("user3:");
    if (!fs_filter_options[5])
        fs_filter_options[5] = lstring::copy("user4:");

    bool nofiles = false;
    if (fs_type == fsSEL) {
        fs_rootdir = lstring::copy(root_or_fname);
        if (!fs_rootdir)
            fs_rootdir = lstring::copy(fs_cwd_bak);
        if (!fs_rootdir)
            fs_rootdir = lstring::copy(cur_root());
        wb_shell = gtk_NewPopup(owner, "File Selection", fs_quit_proc, this);
    }
    else if (fs_type == fsDOWNLOAD) {
        if (!root_or_fname)
            root_or_fname = "unnamed";
        // root_or_fname assumed pathless
        fs_curfile = lstring::copy(root_or_fname);
        fs_rootdir = lstring::copy(fs_cwd_bak);
        if (!fs_rootdir)
            fs_rootdir = lstring::copy(cur_root());
        wb_shell = gtk_NewPopup(owner, "Target Selection", fs_quit_proc, this);
        nofiles = true;
        gtk_widget_set_usize(wb_shell, 400, 200);
    }
    else if (fs_type == fsSAVE || fs_type == fsOPEN) {
        // root_or_fname should be tilde and dot expanded
        char *fn;
        if (root_or_fname && *root_or_fname) {
            if (!lstring::is_rooted(root_or_fname)) {
                const char *cwd = fs_cwd_bak;
                if (!cwd)
                    cwd = "";
                fn = new char[strlen(cwd) + strlen(root_or_fname) + 2];
                sprintf(fn, "%s/%s", cwd, root_or_fname);
            }
            else
                fn = lstring::copy(root_or_fname);
            char *s = lstring::strip_path(fn);
            if (s) {
                *s = 0;
                if (s-1 > fn)
                    *(s-1) = 0;
            }
        }
        else {
            fn = lstring::copy(fs_cwd_bak);
            if (!fn)
                fn = lstring::copy(cur_root());
        }
        fs_rootdir = fn;
        if (fs_type == fsSAVE) {
            wb_shell = gtk_NewPopup(owner, "Path Selection", fs_quit_proc,
                this);
            gtk_widget_set_usize(wb_shell, 250, 200);
            nofiles = true;
        }
        else
            wb_shell = gtk_NewPopup(owner, "File Selection", fs_quit_proc,
                this);
    }

    // Revert focus to application window when file selector pops up.
    gtk_signal_connect(GTK_OBJECT(wb_shell), "focus-in-event",
        (GtkSignalFunc)fs_focus_hdlr, 0);

    GtkWidget *form = gtk_table_new(1, 1, false);
    gtk_widget_show(form);
    gtk_container_add(GTK_CONTAINER(wb_shell), form);

    //
    // buttons
    //
    GtkWidget *hbox = gtk_hbox_new(false, 2);
    gtk_widget_show(hbox);
    fs_up_btn = gtk_button_new();
    gtk_widget_set_name(fs_up_btn, "Up");
    gtk_widget_show(fs_up_btn);
    gtk_signal_connect(GTK_OBJECT(fs_up_btn), "clicked",
        GTK_SIGNAL_FUNC(fs_up_btn_proc), this);
    gtk_box_pack_start(GTK_BOX(hbox), fs_up_btn, false, false, 0);

    fs_go_btn = gtk_button_new();
    if (fs_type == fsDOWNLOAD)
        gtk_widget_set_name(fs_go_btn, "Download");
    else
        gtk_widget_set_name(fs_go_btn, "Go");
    gtk_widget_show(fs_go_btn);
    gtk_signal_connect(GTK_OBJECT(fs_go_btn), "clicked",
        GTK_SIGNAL_FUNC(fs_open_proc), this);
    gtk_box_pack_start(GTK_BOX(hbox), fs_go_btn, false, false, 0);

    if (fs_type == fsDOWNLOAD) {
        fs_entry = gtk_entry_new();
        gtk_widget_show(fs_entry);
        gtk_entry_set_text(GTK_ENTRY(fs_entry), root_or_fname);
        gtk_box_pack_start(GTK_BOX(hbox), fs_entry, true, true, 0);
    }

    //
    // menu bar
    //
    if (fs_type == fsSEL || fs_type == fsSAVE || fs_type == fsOPEN) {
        GtkItemFactoryEntry menu_items[50];
        int nitems = 0;

        IFINIT(nitems, "/_File", 0, 0, 0, "<Branch>");
        if (fs_type == fsSEL)
            IFINIT(nitems, "/File/_Open", "<control>O", fs_menu_proc, fsOpen,
                0);
        IFINIT(nitems, "/File/_New Folder", "<control>F", fs_menu_proc, fsNew,
            0);
        IFINIT(nitems, "/File/_Delete", "<control>D", fs_menu_proc, fsDelete,
            0);
        IFINIT(nitems, "/File/_Rename", "<control>R", fs_menu_proc, fsRename,
            0);
        IFINIT(nitems, "/File/N_ew Root", "<control>E", fs_menu_proc, fsRoot,
            0);
        IFINIT(nitems, "/File/N_ew CWD", "<control>C", fs_menu_proc, fsCwd, 0);
        IFINIT(nitems, "/File/sep1", 0, 0, 0, "<Separator>");
        IFINIT(nitems, "/File/_Quit", "<control>Q", fs_quit_proc, 0, 0);
        IFINIT(nitems, "/_Up", 0, 0, 0, "<Branch>");
        if (!nofiles) {
            IFINIT(nitems, "/_Listing", 0, 0, 0, "<Branch>");
            IFINIT(nitems, "/Listing/_Show Filter", "<control>S", fs_menu_proc,
                fsFilt, "<CheckItem>");
            IFINIT(nitems, "/Listing/_Relist", "<control>L", fs_menu_proc,
                fsRelist, 0);
            IFINIT(nitems, "/Listing/_List by Date", "<control>T",
                fs_menu_proc, fsMtime, "<CheckItem>");
            IFINIT(nitems, "/Listing/Show La_bel", "<control>B", fs_menu_proc,
                fsLabel, "<CheckItem>");
        }
        IFINIT(nitems, "/_Help", 0, 0, 0, "<LastBranch>");
        IFINIT(nitems, "/Help/_Help", "<control>H", fs_menu_proc, fsHelp, 0);

        GtkAccelGroup *accel_group = gtk_accel_group_new();
        fs_item_factory = gtk_item_factory_new(GTK_TYPE_MENU_BAR,
            "<files>", accel_group);
        for (int i = 0; i < nitems; i++) {
            gtk_item_factory_create_item(fs_item_factory, menu_items + i,
                this, 2);
        }
        gtk_window_add_accel_group(GTK_WINDOW(wb_shell), accel_group);

        fs_anc_btn = gtk_item_factory_get_item(fs_item_factory, "/Up");
        if (fs_type == fsSEL)
            fs_open_btn = gtk_item_factory_get_widget(fs_item_factory,
                "/File/Open");
        fs_new_btn = gtk_item_factory_get_widget(fs_item_factory,
            "/File/New Folder");
        fs_delete_btn = gtk_item_factory_get_widget(fs_item_factory,
            "/File/Delete");
        fs_rename_btn = gtk_item_factory_get_widget(fs_item_factory,
            "/File/Rename");

        GtkWidget *menubar = gtk_item_factory_get_widget(fs_item_factory,
            "<files>");
        gtk_widget_show(menubar);
        gtk_box_pack_start(GTK_BOX(hbox), menubar, true, true, 0);

        if (!nofiles) {
            // Initialize Show Label button.
            GtkWidget *btn = gtk_item_factory_get_widget(fs_item_factory,
                "/Listing/Show Label");
            if (btn) {
                if (fs_type == fsSEL)
                    GRX->SetStatus(btn, fs_sel_show_label);
                else if (fs_type == fsOPEN)
                    GRX->SetStatus(btn, fs_open_show_label);
            }
        }
    }

    int rowcnt = 0;
    gtk_table_attach(GTK_TABLE(form), hbox, 0, nofiles ? 1 : 2,
        rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(0), 2, 2);
    rowcnt++;

    //
    // directory tree
    //
    GtkWidget *swin = gtk_scrolled_window_new(0, 0);
    gtk_widget_show(swin);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin),
        GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_set_border_width(GTK_CONTAINER(swin), 2);

#ifdef NO_ICONS
#define COL_TEXT 0
#define COL_MT   1
#define COL_BG   2
    GtkTreeStore *store = gtk_tree_store_new(3, G_TYPE_STRING, G_TYPE_ULONG,
        G_TYPE_STRING);
#else
#define COL_PBUF 0
#define COL_TEXT 1
#define COL_MT   2
#define COL_BG   3
    GtkTreeStore *store = gtk_tree_store_new(4, GDK_TYPE_PIXBUF,
        G_TYPE_STRING, G_TYPE_ULONG, G_TYPE_STRING);
#endif
    fs_tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
    gtk_widget_show(fs_tree);
    // Important, we don't want to take key events.
    gtk_tree_view_set_enable_search(GTK_TREE_VIEW(fs_tree), false);

#ifdef NO_ICONS
    GtkCellRenderer *rnd = gtk_cell_renderer_text_new();
    GtkTreeViewColumn *tvcol = gtk_tree_view_column_new_with_attributes(0, rnd,
        "text", COL_TEXT, "background", COL_BG, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(fs_tree), tvcol);
#else
    GtkCellRenderer *rnd = gtk_cell_renderer_pixbuf_new();
    GtkTreeViewColumn *tvcol = gtk_tree_view_column_new_with_attributes(0, rnd,
        "pixbuf", COL_PBUF, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(fs_tree), tvcol);
    rnd = gtk_cell_renderer_text_new();
    tvcol = gtk_tree_view_column_new_with_attributes(0, rnd,
        "text", COL_TEXT, "background", COL_BG, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(fs_tree), tvcol);
#endif

#if GTK_CHECK_VERSION(2,12,0)
    gtk_tree_view_set_show_expanders(GTK_TREE_VIEW(fs_tree), true);
#endif
    gtk_tree_view_set_enable_tree_lines(GTK_TREE_VIEW(fs_tree), true);
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(fs_tree), false);

    GtkTreeSelection *sel =
        gtk_tree_view_get_selection(GTK_TREE_VIEW(fs_tree));
    gtk_tree_selection_set_select_function(sel,
        (GtkTreeSelectionFunc)fs_tree_select_proc, this, 0);

    gtk_container_add(GTK_CONTAINER(swin), fs_tree);
    if (nofiles) {
        gtk_table_attach(GTK_TABLE(form), swin, 0, 1, rowcnt, rowcnt+1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);
        rowcnt++;
    }

    gtk_widget_add_events(fs_tree, GDK_BUTTON_PRESS_MASK);

    gtk_signal_connect (GTK_OBJECT(fs_tree), "test-collapse-row",
        (GtkSignalFunc)fs_tree_collapse_proc, this);
    gtk_signal_connect (GTK_OBJECT(fs_tree), "test-expand-row",
        (GtkSignalFunc)fs_tree_expand_proc, this);

    // directory list drag source (explicit drag start)
    gtk_signal_connect(GTK_OBJECT(fs_tree), "drag-data-get",
        GTK_SIGNAL_FUNC(fs_source_drag_data_get), this);
    gtk_signal_connect(GTK_OBJECT(fs_tree), "button-press-event",
        GTK_SIGNAL_FUNC(fs_button_press_proc), this);
    gtk_signal_connect(GTK_OBJECT(fs_tree), "button-release-event",
        GTK_SIGNAL_FUNC(fs_button_release_proc), this);
    gtk_signal_connect(GTK_OBJECT(fs_tree), "motion-notify-event",
        GTK_SIGNAL_FUNC(fs_motion_proc), this);

    // directory list drop site
    gtk_drag_dest_set(fs_tree, GTK_DEST_DEFAULT_ALL, target_table, n_targets,
        (GdkDragAction)(GDK_ACTION_COPY | GDK_ACTION_MOVE | GDK_ACTION_LINK |
        GDK_ACTION_ASK));
    gtk_signal_connect_after(GTK_OBJECT(fs_tree), "drag-data-received",
        GTK_SIGNAL_FUNC(fs_drag_data_received), this);
    gtk_signal_connect(GTK_OBJECT(fs_tree), "drag-motion",
        GTK_SIGNAL_FUNC(fs_dir_drag_motion), this);
    gtk_signal_connect(GTK_OBJECT(fs_tree), "drag-leave",
        GTK_SIGNAL_FUNC(fs_dir_drag_leave), this);

    gtk_selection_add_targets(fs_tree, GDK_SELECTION_PRIMARY, target_table,
        n_targets);
    gtk_signal_connect(GTK_OBJECT(fs_tree), "selection-get",
        GTK_SIGNAL_FUNC(fs_selection_get), 0);

    //
    // files list
    //
    if (!nofiles) {
    
        GtkWidget *contr;
        text_scrollable_new(&contr, &wb_textarea, FNT_FIXED);
        gtk_widget_set_usize(wb_textarea, DEF_TEXT_USWIDTH, 200);
        fs_scrwin = contr;

        GtkWidget *vbox = gtk_vbox_new(false, 2);
        gtk_widget_show(vbox);
        gtk_box_pack_start(GTK_BOX(vbox), contr, true, true, 0);

        fs_filter = gtk_combo_new();
        gtk_widget_hide(fs_filter);
        gtk_entry_set_editable(GTK_ENTRY(GTK_COMBO(fs_filter)->entry), false);
        gtk_combo_disable_activate(GTK_COMBO(fs_filter));

        GList *items = 0;
        for (const char **s = fs_filter_options; *s; s++)
            items = g_list_append(items, (char*)*s);
        gtk_combo_set_popdown_strings(GTK_COMBO(fs_filter), items);
        gtk_signal_connect(GTK_OBJECT(GTK_COMBO(fs_filter)->list),
            "select-child", GTK_SIGNAL_FUNC(fs_filter_sel_proc), this);
        gtk_signal_connect(GTK_OBJECT(GTK_COMBO(fs_filter)->list),
            "unselect-child", GTK_SIGNAL_FUNC(fs_filter_unsel_proc), this);
        gtk_signal_connect(GTK_OBJECT(GTK_COMBO(fs_filter)->entry),
            "activate", GTK_SIGNAL_FUNC(fs_filter_activate_proc), this);
        gtk_box_pack_start(GTK_BOX(vbox), fs_filter, false, false, 0);

        GtkWidget *paned = gtk_hpaned_new();
        gtk_paned_add1(GTK_PANED(paned), swin);
        gtk_paned_add2(GTK_PANED(paned), vbox);
        gtk_widget_show(paned);
        gtk_widget_set_size_request(swin, 150, -1);
        gtk_table_attach(GTK_TABLE(form), paned, 0, 2, rowcnt, rowcnt+1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);
        rowcnt++;

        gtk_widget_add_events(wb_textarea, GDK_BUTTON_PRESS_MASK);

        // file list drag source (explicit drag start)
        gtk_signal_connect(GTK_OBJECT(wb_textarea), "drag-begin",
            GTK_SIGNAL_FUNC(fs_drag_begin), this);
        gtk_signal_connect(GTK_OBJECT(wb_textarea), "drag-data-get",
            GTK_SIGNAL_FUNC(fs_source_drag_data_get), this);
        gtk_signal_connect(GTK_OBJECT(wb_textarea), "button-press-event",
            GTK_SIGNAL_FUNC(fs_button_press_proc), this);
        gtk_signal_connect(GTK_OBJECT(wb_textarea), "button-release-event",
            GTK_SIGNAL_FUNC(fs_button_release_proc), this);
        // This is needed to enable motion events when mouse buttons
        // are not pressed.
        gtk_widget_add_events(wb_textarea, GDK_POINTER_MOTION_MASK);
        gtk_signal_connect(GTK_OBJECT(wb_textarea), "motion-notify-event",
            GTK_SIGNAL_FUNC(fs_motion_proc), this);
        gtk_signal_connect(GTK_OBJECT(wb_textarea), "size-allocate",
            GTK_SIGNAL_FUNC(fs_resize_proc), this);
        gtk_widget_add_events(wb_textarea, GDK_LEAVE_NOTIFY_MASK);
        gtk_signal_connect(GTK_OBJECT(wb_textarea), "leave-notify-event",
            GTK_SIGNAL_FUNC(fs_leave_proc), this);
        gtk_signal_connect_after(GTK_OBJECT(wb_textarea), "realize",
            GTK_SIGNAL_FUNC(fs_realize_proc), this);
        gtk_signal_connect(GTK_OBJECT(wb_textarea), "unrealize",
            GTK_SIGNAL_FUNC(fs_unrealize_proc), this);

        // file list drop site
        gtk_drag_dest_set(wb_textarea, GTK_DEST_DEFAULT_ALL, target_table,
            n_targets,
            (GdkDragAction)(GDK_ACTION_COPY | GDK_ACTION_MOVE |
            GDK_ACTION_LINK | GDK_ACTION_ASK));
        gtk_signal_connect_after(GTK_OBJECT(wb_textarea), "drag-data-received",
            GTK_SIGNAL_FUNC(fs_drag_data_received), this);

        // Gtk-2 is tricky to overcome internal selection handling.
        // Must remove clipboard (in fs_realize_proc), and explicitly
        // call gtk_selection_owner_set after selecting text.  Note
        // also that explicit clear-event handling is needed.

        gtk_selection_add_targets(wb_textarea, GDK_SELECTION_PRIMARY,
            target_table, n_targets);
        gtk_signal_connect(GTK_OBJECT(wb_textarea), "selection-clear-event",
            GTK_SIGNAL_FUNC(fs_selection_clear), 0);
        gtk_signal_connect(GTK_OBJECT(wb_textarea), "selection-get",
            GTK_SIGNAL_FUNC(fs_selection_get), 0);

        GtkTextBuffer *textbuf =
            gtk_text_view_get_buffer(GTK_TEXT_VIEW(wb_textarea));
        const char *bclr = GRpkgIf()->GetAttrColor(GRattrColorLocSel);
        gtk_text_buffer_create_tag(textbuf, "primary", "background", bclr,
            NULL);
    }

    //
    // root directory label or entry
    //
    if (fs_type == fsSEL || fs_type == fsOPEN || fs_type == fsDOWNLOAD) {
        if (fs_type == fsDOWNLOAD)
            fs_label = gtk_label_new("\n");
        else
            fs_label = gtk_label_new("\n\n");
        gtk_widget_show(fs_label);

        // Use fixed-pitch font
        const char *fname = FC.getName(FNT_SCREEN);
        PangoFontDescription *pfd = pango_font_description_from_string(fname);
        gtk_widget_modify_font(fs_label, pfd);

        gtk_label_set_justify(GTK_LABEL(fs_label), GTK_JUSTIFY_LEFT);
        gtk_misc_set_alignment(GTK_MISC(fs_label), 0, 0.5);
        gtk_misc_set_padding(GTK_MISC(fs_label), 4, 2);

        fs_label_frame = gtk_frame_new(0);
        gtk_widget_show(fs_label_frame);
        gtk_container_add(GTK_CONTAINER(fs_label_frame), fs_label);

        gtk_table_attach(GTK_TABLE(form), fs_label_frame, 0, 2,
            rowcnt, rowcnt+1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
        rowcnt++;

        if (fs_type == fsSEL || fs_type == fsDOWNLOAD) {
            if (!fs_sel_show_label)
                gtk_widget_hide(fs_label_frame);
        }
        else if (fs_type == fsOPEN) {
            if (!fs_open_show_label)
                gtk_widget_hide(fs_label_frame);
        }
    }
    if (fs_type == fsDOWNLOAD) {
        GtkWidget *button = gtk_button_new_with_label("Dismiss");
        gtk_widget_set_name(button, "Dismiss");
        gtk_widget_show(button);
        gtk_signal_connect(GTK_OBJECT(button), "clicked",
            GTK_SIGNAL_FUNC(fs_quit_proc), this);

        gtk_table_attach(GTK_TABLE(form), button, 0, 1, rowcnt, rowcnt+1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
        gtk_window_set_focus(GTK_WINDOW(wb_shell), button);
        rowcnt++;
    }

    gtk_object_set_data(GTK_OBJECT(wb_shell), "fsbag", this);
    gtk_object_set_data(GTK_OBJECT(fs_tree), "fsbag", this);
    if (wb_textarea)
        gtk_object_set_data(GTK_OBJECT(wb_textarea), "fsbag", this);

    fs_bmap = new sFsBmap(this);
    if (fs_type == fsDOWNLOAD || fs_type == fsSAVE || fs_type == fsOPEN) {
        fs_bmap->enable(this, 1);
        fs_bmap->no_disable_go = true;
    }
    init();
    if (fs_type == fsSEL || fs_type == fsOPEN || fs_type == fsDOWNLOAD) {
        GtkTreePath *p = gtk_tree_path_new_first();
        gtk_tree_selection_select_path(sel, p);
        gtk_tree_path_free(p);
    }
    monitor_setup();
    if (fs_type == fsSAVE || fs_type == fsOPEN)
        gtk_window_set_focus(GTK_WINDOW(wb_shell), fs_go_btn);
    else if (fs_type != fsDOWNLOAD)
        gtk_window_set_focus(GTK_WINDOW(wb_shell), fs_tree);
}


GTKfilePopup::~GTKfilePopup()
{
    if (p_parent) {
        gtk_bag *owner = dynamic_cast<gtk_bag*>(p_parent);
        if (owner)
            owner->MonitorRemove(this);
    }

    FSmonitor.remove(this);
    if (fs_timer_tag)
        gtk_timeout_remove(fs_timer_tag);
    if (p_cancel)
        (*p_cancel)(this, p_cb_arg);
    if (p_usrptr)
        *p_usrptr = 0;
    if (p_caller)
        GRX->Deselect(p_caller);
    if (fs_curnode)
        gtk_tree_path_free(fs_curnode);
    if (fs_drag_node)
        gtk_tree_path_free(fs_drag_node);
    if (fs_cset_node)
        gtk_tree_path_free(fs_cset_node);

    delete [] fs_rootdir;
    delete [] fs_curfile;
    delete [] fs_cwd_bak;
    delete [] fs_colwid;
    delete fs_bmap;

    gtk_signal_disconnect_by_func(GTK_OBJECT(wb_shell),
        GTK_SIGNAL_FUNC(fs_quit_proc), this);

    // It seems that this is needed to avoid a memory leak.
    if (fs_item_factory)
        g_object_unref(fs_item_factory);

    gtk_widget_hide(wb_shell);
    // shell destroyed in gtk_bag destructor
}


// GRpopup override
//
void
GTKfilePopup::popdown()
{
    if (p_parent) {
        gtk_bag *owner = dynamic_cast<gtk_bag*>(p_parent);
        if (!owner || !owner->MonitorActive(this))
            return;
    }
    delete this;
}


// GRfilePopup override
// Return the full path to the currently selected file.
//
char *
GTKfilePopup::get_selection()
{
    if (p_parent) {
        gtk_bag *owner = dynamic_cast<gtk_bag*>(p_parent);
        if (!owner || !owner->MonitorActive(this))
            return (0);
    }
    if (!fs_curnode || !fs_curfile)
        return (0);
    char *dir = get_path(fs_curnode, false);
    char *path = fs_make_dir(dir, fs_curfile);
    delete [] dir;
    return (path);
}


// Initialize the pop-up for a new root path.
//
void
GTKfilePopup::init()
{
    if (wb_textarea)
        text_set_chars(wb_textarea, "");
    GtkTreeStore *store =
        GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(fs_tree)));
    GtkTreeSelection *sel =
        gtk_tree_view_get_selection(GTK_TREE_VIEW(fs_tree));
    gtk_tree_selection_unselect_all(sel);
    gtk_tree_store_clear(store);
    select_dir(0);
    select_file(0);
    set_bmap();
    GtkTreeIter iter;
    if (insert_node(fs_rootdir, &iter, 0))
        add_dir(&iter, fs_rootdir);
    set_label();
}


// Set/unset the sensitivity status of the file operations.
//
void
GTKfilePopup::select_file(const char *fname)
{
    if (!wb_textarea)
        return;
    delete [] fs_curfile;
    if (fname) {
        fs_curfile = lstring::copy(fname);
        fs_bmap->enable(this, 1);
        if (fs_open_btn)
            gtk_widget_set_sensitive(fs_open_btn, true);
    }
    else {
        fs_curfile = 0;
        fs_bmap->disable(this, 1);
        if (fs_open_btn)
            gtk_widget_set_sensitive(fs_open_btn, false);
    }
}


// Set/unset the sensitivity status of the operations which work on
// directories.
//
void
GTKfilePopup::select_dir(GtkTreeIter *iter)
{
    if (fs_curnode) {
        gtk_tree_path_free(fs_curnode);
        fs_curnode = 0;
    }
    if (iter) {
        GtkTreeStore *store =
            GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(fs_tree)));
        GtkTreePath *p = gtk_tree_model_get_path(GTK_TREE_MODEL(store), iter);
        if (p) {
            fs_curnode = p;
            if (wb_textarea) {
                gtk_widget_set_sensitive(fs_new_btn, true);
                gtk_widget_set_sensitive(fs_delete_btn, true);
                gtk_widget_set_sensitive(fs_rename_btn, true);
            }
            return;
        }
    }
    if (wb_textarea) {
        gtk_widget_set_sensitive(fs_new_btn, false);
        gtk_widget_set_sensitive(fs_delete_btn, false);
        gtk_widget_set_sensitive(fs_rename_btn, false);
    }
}


// Set up a timer to monitor the currently displayed hierarchy.  If the
// modification time of any directory changes, update the directory and
// file listings.
//
void
GTKfilePopup::monitor_setup()
{
    if (!fs_timer_tag)
        fs_timer_tag = GRX->AddTimer(500, fs_files_timer, this);
}


// This is called periodically to update the displayed directory and
// file listings.
//
void
GTKfilePopup::check_update()
{
    char *cwd = getcwd(0, 0);
    if (cwd) {
        if (!fs_cwd_bak || strcmp(cwd, fs_cwd_bak)) {
            delete [] fs_cwd_bak;
            fs_cwd_bak = lstring::tocpp(cwd);
            set_label();
        }
        else
            free(cwd);
    }

    GtkTreeStore *store =
        GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(fs_tree)));
    GtkTreeIter iter;
    if (!gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter))
        return;

    for (;;) {
        unsigned long mt;
        gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, COL_MT, &mt, -1);

        char *dir = get_path(&iter, false);
        unsigned long ptime = fs_dirtime(dir);
        if (mt == ptime) {
            delete [] dir;
            if (!gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter))
                break;
            continue;
        }

	    // We know that the content of the iter directory has changed.
        gtk_tree_store_set(GTK_TREE_STORE(store), &iter, COL_MT, ptime, -1);

        GtkTreePath *p = gtk_tree_model_get_path(GTK_TREE_MODEL(store), &iter);
        if (gtk_tree_path_compare(p, fs_curnode) == 0)
			list_files();  // this is selected node, update files list
        gtk_tree_path_free(p);

        stringlist *fa = fs_list_subdirs(dir);
        stringlist *fd = fs_list_node_children(&iter);
	    fs_fdiff(&fa, &fd);

        if (fa || fd) {
            for (stringlist *f = fd; f; f = f->next) {
                GtkTreeIter chiter;
                if (fs_find_child(&iter, f->string, &chiter))
                    gtk_tree_store_remove(GTK_TREE_STORE(store), &chiter);
            }
            for (stringlist *f = fa; f; f = f->next) {
                char *ndir = fs_make_dir(dir, f->string);
                GtkTreeIter chiter;
                insert_node(ndir, &chiter, &iter);
                delete [] ndir;
            }

            stringlist::destroy(fa);
            stringlist::destroy(fd);
        }
        delete [] dir;
        if (!gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter))
            break;
    }
}

// Return the complete directory path to the node.  If noexpand is true,
// return 0 if the node has already been expanded.
//
char *
GTKfilePopup::get_path(GtkTreePath *path, bool noexpand)
{
    GtkTreeStore *store =
        GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(fs_tree)));
    GtkTreeIter iter;
    if (!gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &iter, path))
        return (0);
    if (noexpand && fs_has_child(&iter))
        return (0);

    char *s;
    gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, COL_TEXT, &s, -1);
    s = lstring::tocpp(s);
    for (;;) {
        GtkTreeIter prnt;
        if (!gtk_tree_model_iter_parent(GTK_TREE_MODEL(store), &prnt, &iter))
            break;
        char *c;
        gtk_tree_model_get(GTK_TREE_MODEL(store), &prnt, COL_TEXT, &c, -1);
        char *t = new char[strlen(s) + strlen(c) + 2];
        strcpy(t, c);
        char *tt = t + strlen(t);
        *tt++ = '/';
        strcpy(tt, s);
        delete [] s;
        free(c);
        s = t;
        iter = prnt;
    }
    if (is_root(fs_rootdir)) {
        if (lstring::is_rooted(s))
            return (s);
        else {
            char *t = new char[strlen(s) + 2];
            *t = '/';
            strcpy(t+1, s);
            delete [] s;
            return (t);
        }
    }
    if (!lstring::strrdirsep(s)) {
        delete [] s;
        return (lstring::copy(fs_rootdir));
    }
    char *t = fs_make_dir(fs_rootdir, lstring::strdirsep(s) + 1);
    delete [] s;
    return (t);
}


char *
GTKfilePopup::get_path(GtkTreeIter *iter, bool noexpand)
{
    GtkTreeStore *store =
        GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(fs_tree)));
    GtkTreePath *p = gtk_tree_model_get_path(GTK_TREE_MODEL(store), iter);
    if (!p)
        return (0);
    char *path = get_path(p, noexpand);
    gtk_tree_path_free(p);
    return (path);
}


// Insert a new node into the tree.
//
bool
GTKfilePopup::insert_node(char *dir, GtkTreeIter *iter,
    GtkTreeIter *parent_iter)
{
    char *name = lstring::strrdirsep(dir);
    if (!name)
        return (0);
    if (*(name+1))
        name++;

    GtkTreeModel *tmod = gtk_tree_view_get_model(GTK_TREE_VIEW(fs_tree));
    GtkTreeStore *store = GTK_TREE_STORE(tmod);

    // Sort alphabetically.  There does not seem to be an "auto-sort"
    // mode in GtkTreeView, at least I don't see it.
    //
    if (!gtk_tree_model_iter_children(tmod, iter, parent_iter))
        gtk_tree_store_append(store, iter, parent_iter);
    else {
        for (;;) {
            char *cname;
            gtk_tree_model_get(tmod, iter, COL_TEXT, &cname, -1);
            int cmp = strcmp(name, cname);
            free(cname);
            if (cmp < 0) {
                gtk_tree_store_insert_before(store, iter, parent_iter, iter);
                break;
            }
            GtkTreeIter titer = *iter;
            if (!gtk_tree_model_iter_next(tmod, &titer)) {
                gtk_tree_store_insert_after(store, iter, parent_iter, iter);
                break;
            }
            *iter = titer;
        }
    }

#ifdef NO_ICONS
    gtk_tree_store_set(store, iter, COL_TEXT, name,
        COL_MT, fs_dirtime(dir), -1);
#else
    gtk_tree_store_set(store, iter, COL_PBUF, fs_bmap->close_pb,
        COL_TEXT, name, COL_MT, fs_dirtime(dir), -1);
#endif

    return (true);
}


// Add a directory's subdirectories to the tree.
//
void
GTKfilePopup::add_dir(GtkTreeIter *parent_node, char *dir)
{
    if (!dir || !*dir)
        return;
    DIR *wdir = opendir(dir);
    if (wdir) {

        int sz = strlen(dir) + 64;
        char *p = new char[sz];
        strcpy(p, dir);
        char *dt = p + strlen(p) - 1;
        if (!lstring::is_dirsep(*dt)) {
            *++dt = '/';
            *++dt = 0;
        }
        else
            dt++;
        int reflen = (dt - p);

        struct dirent *de;
        while ((de = readdir(wdir)) != 0) {
            if (!strcmp(de->d_name, "."))
                continue;
            if (!strcmp(de->d_name, ".."))
                continue;
            int flen = strlen(de->d_name);
            while (reflen + flen >= sz) {
                sz += sz;
                char *t = new char[sz];
                *dt = 0;
                dt = lstring::stpcpy(t, p);
                delete [] p;
                p = t;
            }
            strcpy(dt, de->d_name);
#ifdef DT_DIR
            if (de->d_type != DT_UNKNOWN) {
                // Linux glibc returns DT_UNKNOWN for everything
                if (de->d_type == DT_DIR) {
                    GtkTreeIter iter;
                    insert_node(p, &iter, parent_node);
                }
                if (de->d_type != DT_LNK)
                    continue;
            }
#endif
            if (filestat::is_directory(p)) {
                GtkTreeIter iter;
                insert_node(p, &iter, parent_node);
            }
        }
        closedir(wdir);
        delete [] p;
    }
}


namespace {

    // Remove trailing directory separator, if any, unless root token.
    //
    void rm_trail_sep(char **p)
    {
        char *root = *p;
        if (lstring::is_dirsep(root[0]) && !root[1])
            return;
#ifdef WIN32
        if (isalpha(root[0]) && root[1] == ':' &&
                lstring::is_dirsep(root[2]) && !root[3])
            return;
#endif
        char *t = root + strlen(root) - 1;
        while (t >= root && lstring::is_dirsep(*t))
            *t-- = 0;
    }
}


// Preprocess directory string input.
//
char *
GTKfilePopup::get_newdir(const char *rootin)
{
    char *root = pathlist::expand_path(rootin, true, true);
    if (!root || !*root) {
        PopUpMessage("Input syntax error", true);
        return (0);
    }
    fix_root(&root);

    if (!lstring::is_rooted(root)) {
        char *t = root;
        root = pathlist::mk_path(fs_cwd_bak, root);
        delete [] t;
    }
    rm_trail_sep(&root);
    if (!filestat::is_directory(root)) {
        PopUpMessage("Input is not a readable directory", true);
        delete [] root;
        return (0);
    }
    return (root);
}


// Return a stringlist of the tokens in the present filename filter,
// or null if no filter.
//
stringlist *
GTKfilePopup::tokenize_filter()
{
    const char *str = fs_filter_options[fs_filter_index];
    const char *s = strchr(str, ':');
    if (!s)
        s = str;
    else
        s++;
    while (isspace(*s))
        s++;
    if (!*s)
        return (0);
    stringlist *s0 = 0, *se = 0;
    char *tok;
    while ((tok = lstring::gettok(&s)) != 0) {
        if (s0) {
            se->next = new stringlist(tok, 0);
            se = se->next;
        }
        else
            s0 = se = new stringlist(tok, 0);
    }
    return (s0);
}


// File matching, return true if fname should be listed.
//
inline bool
is_match(stringlist *s0, const char *fname)
{
    if (!s0)
        return (true);
#if defined(HAVE_FNMATCH_H) || defined(WIN32)
    for (stringlist *s = s0; s; s = s->next) {
        if (!fnmatch(s->string, fname, 0))
            return (true);
    }
#endif
    return (false);
}


// List the files in the currently selected subdir, in the list window.
//
void
GTKfilePopup::list_files()
{
    if (!wb_textarea)
        return;
    char *dir = get_path(fs_curnode, false);
    if (!dir)
        return;

    DIR *wdir;
    if (!(wdir = opendir(dir))) {
        delete [] dir;
        return;
    }
    int sz = strlen(dir) + 64;
    char *p = new char[sz];
    strcpy(p, dir);
    char *dt = p + strlen(p) - 1;
    if (!lstring::is_dirsep(*dt)) {
        *++dt = '/';
        *++dt = 0;
    }
    else
        dt++;
    int reflen = (dt - p);
    stringlist *filt = tokenize_filter();
    sFileList fl(dir);
    delete [] dir;
    struct dirent *de;
    while ((de = readdir(wdir)) != 0) {
        if (!strcmp(de->d_name, "."))
            continue;
        if (!strcmp(de->d_name, ".."))
            continue;
        int flen = strlen(de->d_name);
        while (reflen + flen >= sz) {
            sz += sz;
            char *t = new char[sz];
            *dt = 0;
            dt = lstring::stpcpy(t, p);
            delete [] p;
            p = t;
        }
        strcpy(dt, de->d_name);
#ifdef DT_DIR
        if (de->d_type != DT_UNKNOWN) {
            // Linux glibc returns DT_UNKNOWN for everything
            if (de->d_type == DT_DIR)
                continue;
            if (de->d_type == DT_LNK) {
                if (filestat::is_directory(p))
                    continue;
            }
            if (!is_match(filt, de->d_name))
                continue;
            fl.add_file(de->d_name);
            continue;
        }
#endif
        if (!is_match(filt, de->d_name))
            continue;

        if (filestat::is_directory(p))
            continue;
        fl.add_file(de->d_name);
    }
    delete [] p;
    closedir(wdir);
    stringlist::destroy(filt);
    int w = wb_textarea->allocation.width;
    if (w <= 1)
        w = DEF_TEXT_USWIDTH;
    int ncols = w/GTKfont::stringWidth(wb_textarea, 0);
    int *colwid;
    char *s = fl.get_formatted_list(ncols, fs_mtime_sort, 0, &colwid);

    // maintain scroll position
    double val = text_get_scroll_value(wb_textarea);
    text_set_chars(wb_textarea, s);
    text_set_scroll_value(wb_textarea, val);
    delete [] fs_colwid;
    fs_colwid = colwid;

    select_file(0);
    delete [] s;
}


// Return the file name under the given pixel coordinates in the file
// listing.  If the pointers are not null, return the character
// offsets of the start and end of the returned text.
//
char *
GTKfilePopup::file_name_at(int x, int y, int *pstart, int *pend)
{
    if (pstart)
        *pstart = 0;
    if (pend)
        *pend = 0;
    if (!wb_textarea)
        return (0);
    char *string = text_get_chars(wb_textarea, 0, -1);

    gtk_text_view_window_to_buffer_coords(GTK_TEXT_VIEW(wb_textarea),
        GTK_TEXT_WINDOW_WIDGET, x, y, &x, &y);
    GtkTextIter ihere, iline;
    gtk_text_view_get_iter_at_location(GTK_TEXT_VIEW(wb_textarea), &ihere,
        x, y);
    gtk_text_view_get_line_at_y(GTK_TEXT_VIEW(wb_textarea), &iline, y, 0);
    x = gtk_text_iter_get_offset(&ihere) - gtk_text_iter_get_offset(&iline);
    char *line_start = string + gtk_text_iter_get_offset(&iline);

    int cstart = 0;
    int cend = 0;
    if (fs_colwid) {
        for (int i = 0; fs_colwid[i]; i++) {
            cstart = cend;
            cend += fs_colwid[i];
            if (x >= cstart && x < cend)
                break;
        }
    }
    if (cstart == cend || x >= cend) {
            delete [] string;
            return (0);
    }
    for (int st = 0 ; st <= x; st++) {
        if (line_start[st] == '\n' || line_start[st] == 0) {
            // pointing to right of line end
            delete [] string;
            return (0);
        }
    }

    // We know that the file name starts at cstart, find the actual
    // end.  Note that we deal with file names that contain spaces.
    for (int st = 0 ; st < cend; st++) {
        if (line_start[st] == '\n') {
            cend = st;
            break;
        }
    }

    // Omit trailing space.
    while (isspace(line_start[cend-1]))
        cend--;
    if (x >= cend) {
        // Clicked on trailing white space.
        delete [] string;
        return (0);
    }

    cstart += (line_start - string);
    cend += (line_start - string);
    string[cend] = 0;
    char *name = lstring::copy(string + cstart);
    delete [] string;
    if (pstart)
        *pstart = cstart;
    if (pend)
        *pend = cend;
    return (name);
}


// Update the text in the "Root" label and rebuild the Up menu.
//
void
GTKfilePopup::set_label()
{
    if (!fs_rootdir)
        return;
    if (fs_label) {
        if (fs_type == fsDOWNLOAD) {
            char *dir = get_path(fs_curnode, false);
            if (dir) {
                gtk_label_set_text(GTK_LABEL(fs_label), dir);
                delete [] dir;
            }
            return;
        }
        const char *cwd = fs_cwd_bak;
        if (!cwd)
            cwd = "";
        char *tmpc = new char[strlen(cwd) + strlen(fs_rootdir) + 20];
        sprintf(tmpc, "Root: %s\nCwd: %s", fs_rootdir, cwd);
        gtk_label_set_text(GTK_LABEL(fs_label), tmpc);
        delete [] tmpc;
    }

    if (fs_anc_btn) {
        GtkWidget *menu = GTK_MENU_ITEM(fs_anc_btn)->submenu;
        if (menu)
            gtk_widget_destroy(menu);
        menu = gtk_menu_new();

        char buf[256];
        char *s = fs_rootdir;
        char *e = lstring::strdirsep(fs_rootdir);
        if (e)
            e++;
        else
            e = fs_rootdir + strlen(fs_rootdir);
        for (;;) {
            strncpy(buf, s, e-s);
            buf[e-s] = 0;
            s = e;
            GtkWidget *menu_item = gtk_menu_item_new_with_label(buf);
            gtk_widget_set_name(menu_item, buf);
            gtk_menu_append(GTK_MENU(menu), menu_item);
            gtk_object_set_data(GTK_OBJECT(menu_item), "offset",
                (void*)(e - fs_rootdir - 1));
            gtk_signal_connect(GTK_OBJECT(menu_item), "activate",
                GTK_SIGNAL_FUNC(fs_upmenu_proc), this);
            gtk_widget_show(menu_item);
            if (!*s)
                break;
            e = lstring::strdirsep(s);
            if (!e)
                break;
            e++;
        }
        gtk_menu_item_set_submenu(GTK_MENU_ITEM(fs_anc_btn), menu);
    }
}


// Update the "up" bitmap state.
//
void
GTKfilePopup::set_bmap()
{
    if (fs_rootdir && !is_root(fs_rootdir)) {
        fs_bmap->enable(this, 0);
        if (fs_anc_btn)
            gtk_widget_set_sensitive(fs_anc_btn, true);
    }
    else {
        fs_bmap->disable(this, 0);
        if (fs_anc_btn)
            gtk_widget_set_sensitive(fs_anc_btn, false);
    }
}


// Tree selection/deselection handler.
//
void
GTKfilePopup::select_row_handler(GtkTreeIter *iter, bool desel)
{
    if (desel) {
        select_dir(0);
        if (wb_textarea)
            text_set_chars(wb_textarea, "");
        return;
    }
    select_dir(iter);
    char *dir = get_path(fs_curnode, true);
    add_dir(iter, dir);
    delete [] dir;
    list_files();
    if (fs_type == fsDOWNLOAD)
        set_label();
    else if (p_path_get && p_path_set) {
        dir = get_path(fs_curnode, false);
        char *path = (*p_path_get)();
        if (path && *path) {
            char *fname = lstring::strip_path(path);
            fname = fs_make_dir(dir, fname);
            (*p_path_set)(fname);  // frees fname
            delete [] path;
            delete [] dir;
        }
        else {
            (*p_path_set)(dir);  // frees dir
            delete [] path;
        }
    }
}


void
GTKfilePopup::menu_handler(GtkWidget *widget, int code)
{
    if (code == fsOpen) {
        fs_data *data = 0;
        if (p_callback) {
            if (fs_entry) {
                const char *sel = gtk_entry_get_text(GTK_ENTRY(fs_entry));
                if (sel && *sel)  {
                    char *dir = get_path(fs_curnode, false);
                    char *path = fs_make_dir(dir, sel);
                    delete [] dir;
                    data = new fs_data(this, path);
                }
            }
            else {
                char *sel = get_selection();
                if (sel || fs_bmap->no_disable_go)
                    data = new fs_data(this, sel);
            }
        }
        if (data)
            gtk_idle_add((GtkFunction)fs_open_idle, data);
    }
    else if (code == fsNew) {
        char *path = get_path(fs_curnode, false);
        if (!path) {
            PopUpMessage("No parent directory selected.", true);
            return;
        }
        fs_data *data = new fs_data(this, path);
        PopUpInput("Enter new folder name:", 0, "Create", fs_new_cb, data);
    }
    else if (code == fsDelete) {
        char *path = get_selection();
        if (!path)
            path = get_path(fs_curnode, false);
        if (!path) {
            PopUpMessage("Nothing selected to delete.", true);
            return;
        }
        char buf[256];
        if (strlen(path) < 80)
            sprintf(buf, "Delete %s?", path);
        else
            sprintf(buf, "Delete .../%s?", lstring::strip_path(path));
        fs_data *data = new fs_data(this, path);
        PopUpAffirm(0, GRloc(LW_XYR, 50, 100), buf, fs_delete_cb, data);
    }
    else if (code == fsRename) {
        char *path = get_selection();
        if (!path)
            path = get_path(fs_curnode, false);
        if (!path) {
            PopUpMessage("Nothing selected to renme.", true);
            return;
        }
        char buf[256];
        sprintf(buf, "Enter new name for %s?", lstring::strip_path(path));
        fs_data *data = new fs_data(this, path);
        PopUpInput(buf, 0, "Rename", fs_rename_cb, data);
    }
    else if (code == fsRoot) {
        PopUpInput("Enter full path to new directory", fs_rootdir, "Apply",
            fs_root_cb, this, 300);
    }
    else if (code == fsCwd) {
        PopUpInput("Enter new current directory", fs_cwd_bak, "Apply",
            fs_cwd_cb, this, 300);
    }
    else if (code == fsFilt) {
        if (fs_filter) {
            if (GRX->GetStatus(widget))
                gtk_widget_show(fs_filter);
            else
                gtk_widget_hide(fs_filter);
        }
    }
    else if (code == fsRelist)
        list_files();
    else if (code == fsMtime) {
        fs_mtime_sort = GRX->GetStatus(widget);
        list_files();
    }
    else if (code == fsLabel) {
        if (fs_label_frame) {
            if (GRX->GetStatus(widget)) {
                gtk_widget_show(fs_label_frame);
                if (fs_type == fsSEL)
                    fs_sel_show_label = true;
                else if (fs_type == fsOPEN)
                    fs_open_show_label = true;
            }
            else {
                gtk_widget_hide(fs_label_frame);
                if (fs_type == fsSEL)
                    fs_sel_show_label = false;
                else if (fs_type == fsOPEN)
                    fs_open_show_label = false;
            }
        }
    }
    else if (code == fsHelp) {
        if (GRX->MainFrame())
            GRX->MainFrame()->PopUpHelp("filesel");
        else
            PopUpHelp("filesel");
    }
}


bool
GTKfilePopup::button_handler(GtkWidget *widget, GdkEvent *event, bool up)
{
    if (up) {
        fs_dragging = false;
        return (false);
    }

    // double-clicking on a file entry calls the open proc
    bool dblclk = false;
    if (event->type == GDK_2BUTTON_PRESS) {
        if (GTK_IS_TREE_VIEW(widget))
            return (true);
        if (event->button.button != 1)
            return (true);
        dblclk = true;
        fs_dragging = false;
    }

    if (!dblclk) {
        if (event->type != GDK_BUTTON_PRESS)
            return (true);

        if (GTK_IS_TREE_VIEW(widget)) {
            GtkTreePath *path;
            if (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(widget), 
                    (int)event->button.x, (int)event->button.y, &path,
                    0, 0, 0)) {

                fs_dragging = true;
                fs_drag_node = path;
                fs_drag_btn = event->button.button;
                fs_drag_x = (int)event->button.x;
                fs_drag_y = (int)event->button.y;
            }
            return (false);
        }
    }

    int start, end;
    text_get_selection_pos(widget, &start, &end);
    if (dblclk && start != end) {
        menu_handler(0, fsOpen);
        return (true);
    }

    select_range(0, 0);
    select_file(0);

    int x = (int)event->button.x;
    int y = (int)event->button.y;
    char *text = file_name_at(x, y, &start, &end);
    if (!text)
        return (true);

    select_range(start, end);
    select_file(text);
    delete [] text;
    if (dblclk) {
        menu_handler(0, fsOpen);
        return (true);
    }
    if (p_path_set)
        (*p_path_set)(get_selection());
    fs_dragging = true;
    fs_drag_btn = event->button.button;
    fs_drag_x = x;
    fs_drag_y = y;
    return (true);
}


// Select the chars in the range, start=end deselects existing.  In
// GTK-1, selecting gives blue inverse, which turns gray if
// unselected, retaining an indication for the Go button.  GTK-2
// doesn't do this automatically so we provide something similar here.
//
void
GTKfilePopup::select_range(int start, int end)
{
    GtkTextBuffer *textbuf =
        gtk_text_view_get_buffer(GTK_TEXT_VIEW(wb_textarea));
    GtkTextIter istart, iend;
    if (fs_end != fs_start) {
        gtk_text_buffer_get_iter_at_offset(textbuf, &istart, fs_start);
        gtk_text_buffer_get_iter_at_offset(textbuf, &iend, fs_end);
        gtk_text_buffer_remove_tag_by_name(textbuf, "primary", &istart, &iend);
    }
    text_select_range(wb_textarea, start, end);
    if (end != start) {
        gtk_text_buffer_get_iter_at_offset(textbuf, &istart, start);
        gtk_text_buffer_get_iter_at_offset(textbuf, &iend, end);
        gtk_text_buffer_apply_tag_by_name(textbuf, "primary", &istart, &iend);
        gtk_selection_owner_set(wb_textarea, GDK_SELECTION_PRIMARY,
            GDK_CURRENT_TIME);
    }
    fs_start = start;
    fs_end = end;
}


// Return a list of the names associated with the children.
//
stringlist *
GTKfilePopup::fs_list_node_children(GtkTreeIter *parent_iter)
{
    stringlist *s0 = 0;
    GtkTreeStore *store =
        GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(fs_tree)));
    int n = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(store), parent_iter);
    if (n <= 0)
        return (0);
    GtkTreeIter iter;
    if (!gtk_tree_model_iter_children(GTK_TREE_MODEL(store), &iter,
            parent_iter))
        return (0);
    while (n--) {
        char *cname;
        gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, COL_TEXT, &cname, -1);
        s0 = new stringlist(lstring::tocpp(cname), s0);
        gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter);
    }
    return (s0);
}


// Return true is node has a child.  The "children" field seems to point
// to the internal list just below node, so the "children" are not
// necessarily parented by node.
//
bool
GTKfilePopup::fs_has_child(GtkTreeIter *parent_iter)
{
    GtkTreeStore *store =
        GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(fs_tree)));
    return (gtk_tree_model_iter_has_child(GTK_TREE_MODEL(store), parent_iter));
}


// Return with iter pointing to the child of parent_iter with the
// given name.
//
bool
GTKfilePopup::fs_find_child(GtkTreeIter *parent_iter, const char *name,
    GtkTreeIter *iter)
{
    if (!name)
        return (false);
    GtkTreeStore *store =
        GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(fs_tree)));
    int n = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(store), parent_iter);
    if (n <= 0)
        return (false);
    if (!gtk_tree_model_iter_children(GTK_TREE_MODEL(store), iter, parent_iter))
        return (false);
    while (n--) {
        char *cname;
        gtk_tree_model_get(GTK_TREE_MODEL(store), iter, COL_TEXT, &cname, -1);
        if (!strcmp(cname, name)) {
            free(cname);
            return (true);
        }
        free(cname);
        gtk_tree_model_iter_next(GTK_TREE_MODEL(store), iter);
    }
    return (false);
}


// Private static GTK signal handler.
// Cancel action.
//
void
GTKfilePopup::fs_quit_proc(GtkWidget*, void *client_data)
{
    GTKfilePopup *fs = static_cast<GTKfilePopup*>(client_data);
    if (fs)
        fs->popdown();
}


// Private static GTK signal handler.
// Revert keyboard focus to main window when file selector first gains
// focus on pop-up.
//
int
GTKfilePopup::fs_focus_hdlr(GtkWidget *widget, GdkEvent*, void*)
{
    if (GRX->MainFrame() && GRX->MainFrame()->PositionReferenceWidget())
        GRX->SetFocus(GRX->MainFrame()->PositionReferenceWidget());
    gtk_signal_disconnect_by_func(GTK_OBJECT(widget),
        GTK_SIGNAL_FUNC(fs_focus_hdlr), 0);
    return (0);
}


// Private static GTK signal handler.
// We use an idle function for the open user function, since this
// may block.
//
int
GTKfilePopup::fs_open_idle(void *arg)
{
    fs_data *data = (fs_data*)arg;
    GTKfilePopup *f = data->fs;
    if (f && f->p_callback)
        (*f->p_callback)(data->string, f->p_cb_arg);
    delete data;
    return (false);
}


// Private static GTK signal handler.
// Tree selection callback.
//
// We use an idle function here to avoid a GtkTreeView bug.  When
// using arrow keys to move the selected row, this handler is called
// with issel true when attempting to move before the first or after
// the last populated row.  Thus, we get an indication here that the
// first or last entry is being unselected, which is not actually
// true.  The idle function looks at the actual widget state and
// responds accordingly.
//
bool
GTKfilePopup::fs_tree_select_proc(GtkTreeSelection*, GtkTreeModel*,
    GtkTreePath*, bool, void *data)
{
    GTKfilePopup *fs = static_cast<GTKfilePopup*>(data);
    if (!fs)
        return (false);
    if (!fs->fs_tid)
        fs->fs_tid = gtk_idle_add(fs_sel_test_idle, fs);
    return (true);
}


// Private static idle procedure.
// Handle the selection change.
// 
int
GTKfilePopup::fs_sel_test_idle(void *arg)
{
    GTKfilePopup *fs = static_cast<GTKfilePopup*>(arg);
    if (!fs)
        return (0);
    fs->fs_tid = 0;
    GtkTreeSelection *sel =
        gtk_tree_view_get_selection(GTK_TREE_VIEW(fs->fs_tree));
    GtkTreeIter iter;
    bool issel = gtk_tree_selection_get_selected(sel, 0, &iter);
    fs->select_row_handler(&iter, !issel);
    return (0);
}


// Private static GTK signal handler.
// If the selected row is collapsed, deselect.  This does not happen
// automatically.
//
int
GTKfilePopup::fs_tree_collapse_proc(GtkTreeView *tv, GtkTreeIter *iter,
    GtkTreePath *path, void *data)
{
    GTKfilePopup *fs = static_cast<GTKfilePopup*>(data);
    if (!fs)
        return (true);
#ifdef NO_ICONS
    (void)iter;
#else
    GtkTreeStore *store = GTK_TREE_STORE(gtk_tree_view_get_model(tv));
    gtk_tree_store_set(store, iter, COL_PBUF, fs->fs_bmap->close_pb, -1);
#endif

    if (fs->fs_curnode && gtk_tree_path_is_ancestor(path, fs->fs_curnode)) {
        GtkTreeSelection *sel = gtk_tree_view_get_selection(tv);
        gtk_tree_selection_unselect_path(sel, fs->fs_curnode);
    }
    return (false);
}


int
GTKfilePopup::fs_tree_expand_proc(GtkTreeView *tv, GtkTreeIter *iter,
    GtkTreePath*, void *data)
{
#ifdef NO_ICONS
    (void)tv;
    (void)iter;
    (void)data;
#else
    GTKfilePopup *fs = static_cast<GTKfilePopup*>(data);
    if (!fs)
        return (true);
    GtkTreeStore *store = GTK_TREE_STORE(gtk_tree_view_get_model(tv));
    gtk_tree_store_set(store, iter, COL_PBUF, fs->fs_bmap->open_pb, -1);
#endif
    return (false);
}


// Private static GTK signal handler.
// Handler for the Up menu.
//
void
GTKfilePopup::fs_upmenu_proc(GtkWidget *widget, void *client_data)
{
    GTKfilePopup *fs = static_cast<GTKfilePopup*>(client_data);
    if (fs) {
        long offset = (long)gtk_object_get_data(GTK_OBJECT(widget), "offset");
        if (offset <= 0)
            offset = 1;
        if (offset >= 256)
            return;
        char *tbuf = lstring::copy(fs->fs_rootdir);
        tbuf[offset] = 0;
        fs_root_cb(tbuf, fs);
        delete [] tbuf;
    }
}


// Private static GTK signal handler.
//
void
GTKfilePopup::fs_menu_proc(GtkWidget *widget, void *client_data,
    unsigned int code)
{
    GTKfilePopup *fs = static_cast<GTKfilePopup*>(client_data);
    if (fs)
        fs->menu_handler(widget, code);
}


// Private static GTK signal handler.
//
void
GTKfilePopup::fs_open_proc(GtkWidget *widget, void *client_data)
{
    GTKfilePopup *fs = static_cast<GTKfilePopup*>(client_data);
    if (fs)
        fs->menu_handler(widget, fsOpen);
}


// Private static GTK signal handler.
// A new filter entry has been selected, update the index and toggle
// the editable flag.
//
void
GTKfilePopup::fs_filter_sel_proc(GtkList *list, GtkWidget *widget, void *fsp)
{
    GTKfilePopup *fs = static_cast<GTKfilePopup*>(fsp);
    if (fs) {
        int ox = fs->fs_filter_index;
        fs->fs_filter_index = gtk_list_child_position(list, widget);
        gtk_entry_set_editable(GTK_ENTRY(GTK_COMBO(fs->fs_filter)->entry),
            (fs->fs_filter_index > 1));
        if (fs->fs_filter_index != ox)
            fs->list_files();
    }
}


// Private static GTK signal handler.
// A filter entry has been selected, update the text for this entry.
//
void
GTKfilePopup::fs_filter_unsel_proc(GtkList *list, GtkWidget *widget, void *fsp)
{
    GTKfilePopup *fs = static_cast<GTKfilePopup*>(fsp);
    if (fs) {
        int i = gtk_list_child_position(list, widget);
        if (i > 1) {
            // update entry
            const char *text =
                gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(fs->fs_filter)->entry));
            gtk_label_set_text(GTK_LABEL(GTK_BIN(widget)->child), text);
            delete [] fs_filter_options[i];
            fs_filter_options[i] = lstring::copy(text);
        }
    }
}


// Private static GTK signal handler.
// Called when Enter pressed while editing combo box string.
//
void
GTKfilePopup::fs_filter_activate_proc(GtkWidget*, void *fsp)
{
    GTKfilePopup *fs = static_cast<GTKfilePopup*>(fsp);
    if (fs)
        fs->list_files();
}


// Private static GTK signal handler.
// Handler for the "up" button, changes the root directory to the parent
// directory.  This clears and resets both windows.
//
void
GTKfilePopup::fs_up_btn_proc(GtkWidget*, void *fsp)
{
    GTKfilePopup *fs = static_cast<GTKfilePopup*>(fsp);
    if (fs) {
        if (!fs->fs_rootdir || is_root(fs->fs_rootdir))
            return;

        char *s = lstring::strrdirsep(fs->fs_rootdir);
        if (s) {
            if (s == lstring::strdirsep(fs->fs_rootdir))
                s++;
            *s = 0;
            s = lstring::copy(fs->fs_rootdir);
            delete [] fs->fs_rootdir;
            fs->fs_rootdir = s;
        }
        fs->init();
    }
}


// Private static GTK signal handler.
void
GTKfilePopup::fs_realize_proc(GtkWidget *widget, void *fsp)
{
    // Remove the primary selection clipboard before selection. 
    // In order to return the full path, we have to handle
    // selections ourselves.
    //
    if (GTK_IS_TEXT_VIEW(widget))
        gtk_text_buffer_remove_selection_clipboard(
            gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget)),
            gtk_widget_get_clipboard(widget, GDK_SELECTION_PRIMARY));
    text_realize_proc(widget, fsp);
}


// Private static GTK signal handler.
void
GTKfilePopup::fs_unrealize_proc(GtkWidget *widget, void*)
{
    // Stupid alert:  put the clipboard back into the buffer to
    // avoid a whine when remove_selection_clipboard is called in
    // unrealize.

    if (GTK_IS_TEXT_VIEW(widget))
        gtk_text_buffer_add_selection_clipboard(
            gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget)),
            gtk_widget_get_clipboard(widget, GDK_SELECTION_PRIMARY));
}


// Compute new column layout on resize.
//
void
GTKfilePopup::fs_resize_proc(GtkWidget *widget, GtkAllocation *a, void *fsp)
{
    if (GTK_WIDGET_REALIZED(widget) && fsp) {
        GTKfilePopup *fs = static_cast<GTKfilePopup*>(fsp);
        if (fs) {
            // This handler is called when a scrollbar is added or
            // removed.  We actually only care about the containing
            // window size, which includes the scrollbars.  Probably
            // should attach this handler to the container, but maybe
            // we will want to deal with scrollbar changes in future.
            a = &fs->fs_scrwin->allocation;

            if (a->width == fs->fs_alloc_width)
                return;

            fs->fs_alloc_width = a->width;
            fs->list_files();
        }
    }
}


// Private static GTK signal handler.
// Handler for button presses in the directory and file listing windows.
//
int
GTKfilePopup::fs_button_press_proc(GtkWidget *widget, GdkEvent *event,
    void *fsp)
{
    GTKfilePopup *fs = static_cast<GTKfilePopup*>(fsp);
    if (fs)
        return (fs->button_handler(widget, event, false));
    return (false);
}


// Private static GTK signal handler.
// Handler for button releases in the directory and file listing windows.
//
int
GTKfilePopup::fs_button_release_proc(GtkWidget *widget, GdkEvent *event,
    void *fsp)
{
    GTKfilePopup *fs = static_cast<GTKfilePopup*>(fsp);
    if (fs)
        return (fs->button_handler(widget, event, true));
    return (false);
}



// Private static GTK signal handler.
// Start the drag if we are over text and the pointer moved, or show the
// "ls -l" string in the label.
//
int
GTKfilePopup::fs_motion_proc(GtkWidget *widget, GdkEvent *event, void *fsp)
{
    GTKfilePopup *fs = static_cast<GTKfilePopup*>(fsp);
    if (fs) {
        if (fs->fs_dragging) {
#if GTK_CHECK_VERSION(2,12,0)
            if (event->motion.is_hint)
                gdk_event_request_motions((GdkEventMotion*)event);
#else
            // Strange voodoo to "turn on" motion events, that are
            // otherwise suppressed since GDK_POINTER_MOTION_HINT_MASK
            // is set.  See GdkEventMask doc.
            gdk_window_get_pointer(widget->window, 0, 0, 0);
#endif
            if ((abs((int)event->motion.x - fs->fs_drag_x) > 4 ||
                    abs((int)event->motion.y - fs->fs_drag_y) > 4)) {
                fs->fs_dragging = false;
                if (widget == fs->fs_tree) {
                    gtk_selection_owner_set(widget, GDK_SELECTION_PRIMARY,
                        GDK_CURRENT_TIME);
                }
                GtkTargetList *targets =
                    gtk_target_list_new(target_table, n_targets);
                gtk_drag_begin(widget, targets,
                    (GdkDragAction)(GDK_ACTION_COPY | GDK_ACTION_MOVE |
                    GDK_ACTION_LINK | GDK_ACTION_ASK), fs->fs_drag_btn, event);
                // Hmmmm, somebody set a keyboard grab.  This causes the
                // clist to lose focus, which erases the box marking the
                // press location.
                if (widget == fs->fs_tree)
                    gdk_keyboard_ungrab(GDK_CURRENT_TIME);
                return (true);
            }
        }
        if (fs->fs_label && widget == fs->wb_textarea)  {
            char *fn = fs->file_name_at((int)event->motion.x,
                (int)event->motion.y, 0, 0);
            if (fn) {
                char *dir = fs->get_path(fs->fs_curnode, false);
                char *path = pathlist::mk_path(dir, fn);
                delete [] fn;
                delete [] dir;

                char *text = pathlist::ls_longform(path, true);
                delete [] path;
                gtk_label_set_text(GTK_LABEL(fs->fs_label), text);
                delete [] text;
            }
        }
    }
    return (false);
}


// Private static GTK signal handler.
// Restore the label.
int
GTKfilePopup::fs_leave_proc(GtkWidget*, GdkEvent*, void *fsp)
{
    GTKfilePopup *fs = static_cast<GTKfilePopup*>(fsp);
    if (fs)
        fs->set_label();
    return (false);
}


// Private static GTK signal handler.
// If dragging from the text widget, set the drag icon, so that it looks
// the same as when starting from the ctree.
//
void
GTKfilePopup::fs_drag_begin(GtkWidget*, GdkDragContext *context)
{
    gtk_drag_set_icon_default(context);
}


// Private static GTK signal handler.
// Process the path name received in the drop.
//
void
GTKfilePopup::fs_drag_data_received(GtkWidget *widget, GdkDragContext *context,
    gint x, gint y, GtkSelectionData *data, guint, guint time)
{
    if (data->length >= 0 && data->format == 8 && data->data) {
        GTKfilePopup *fs =
            (GTKfilePopup*)gtk_object_get_data(GTK_OBJECT(widget), "fsbag");
        if (fs) {
            char *dst = 0;
            if (widget == fs->wb_textarea)
                dst = fs->get_path(fs->fs_curnode, false);
            else if (widget == fs->fs_tree) {
                GtkTreePath *p;
                if (gtk_tree_view_get_path_at_pos(
                        GTK_TREE_VIEW(fs->fs_tree), x, y, &p, 0, 0, 0)) {
                    GtkTreeModel *store = 
                        gtk_tree_view_get_model(GTK_TREE_VIEW(fs->fs_tree));

                    GtkTreeIter iter;
                    if (gtk_tree_model_get_iter(store, &iter, p))
                        dst = fs->get_path(&iter, false);
                    gtk_tree_path_free(p);
                }
            }
            if (dst) {
                gtk_DoFileAction(fs->wb_shell, (char*)data->data, dst,
                    context, true);
                delete [] dst;
            }
            gtk_drag_finish(context, true, false, time);
            return;
        }
    }
    gtk_drag_finish(context, false, false, time);
}


// Private static GTK signal handler.
// Set the drag source data.
//
void
GTKfilePopup::fs_source_drag_data_get(GtkWidget *widget, GdkDragContext*,
    GtkSelectionData *selection_data, guint, guint, gpointer)
{
    // stop native handler
    gtk_signal_emit_stop_by_name(GTK_OBJECT(widget), "drag-data-get");

    GTKfilePopup *fs =
        (GTKfilePopup*)gtk_object_get_data(GTK_OBJECT(widget), "fsbag");
    if (fs) {
        char *path;
        if (GTK_IS_TREE_VIEW(widget))
            path = fs->get_path(fs->fs_drag_node, false);
        else
            path = fs->get_selection();
        if (path) {
            gtk_selection_data_set(selection_data, selection_data->target,
                8, (unsigned char*)path, strlen(path));
            delete [] path;
        }
    }
}


// Private static GTK signal handler.
// If the pointer is in the directory list and a drag is active,
// highlight the underlying directory (will be the destination).
//
int
GTKfilePopup::fs_dir_drag_motion(GtkWidget *widget, GdkDragContext*, int x,
    int y, guint)
{
    GTKfilePopup *fs =
        (GTKfilePopup*)gtk_object_get_data(GTK_OBJECT(widget), "fsbag");
    if (!fs)
        return (true);
    fs_scroll_hdlr(widget);
    GtkTreeModel *store = gtk_tree_view_get_model(GTK_TREE_VIEW(widget));
    GtkTreePath *p;
    if (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(widget), 
            x, y, &p, 0, 0, 0)) {
        if (fs->fs_cset_node && !gtk_tree_path_compare(fs->fs_cset_node, p)) {
            gtk_tree_path_free(p);
            return (true);
        }
        GtkTreeIter iter;
        if (fs->fs_cset_node &&
                gtk_tree_model_get_iter(store, &iter, fs->fs_cset_node))
            gtk_tree_store_set(GTK_TREE_STORE(store), &iter, COL_BG, 0, -1);
        gtk_tree_path_free(fs->fs_cset_node);
        fs->fs_cset_node = p;
        gtk_tree_model_get_iter(store, &iter, p);
        const char *c = GRpkgIf()->GetAttrColor(GRattrColorLocSel);
        gtk_tree_store_set(GTK_TREE_STORE(store), &iter, COL_BG, c, -1);
    }
    return (true);
}


// Private static GTK signal handler.
// In GTK-2, this overrides the built-in selection-get function.
//
void
GTKfilePopup::fs_selection_get(GtkWidget *widget,
    GtkSelectionData *selection_data, guint, guint, void*)
{
    if (selection_data->selection != GDK_SELECTION_PRIMARY)
        return;  

    // stop native handler
    gtk_signal_emit_stop_by_name(GTK_OBJECT(widget), "selection-get");

    GTKfilePopup *fs =
        (GTKfilePopup*)gtk_object_get_data(GTK_OBJECT(widget), "fsbag");
    if (fs) {
        char *path;
        if (GTK_IS_TREE_VIEW(widget))
            path = fs->get_path(fs->fs_drag_node, false);
        else
            path = fs->get_selection();
        if (path) {
            gtk_selection_data_set(selection_data, selection_data->target,
                8, (unsigned char*)path, strlen(path));
            delete [] path;
        }
    }
}


// Private static GTK signal handler.
// Selection clear handler, need to do this ourselves in GTK-2.
//
int
GTKfilePopup::fs_selection_clear(GtkWidget *widget, GdkEventSelection*, void*)  
{
    text_select_range(widget, 0, 0);
    return (true);
}


// Private static GTK signal handler.
// Unhighlight the directory list when leaving.
//
void
GTKfilePopup::fs_dir_drag_leave(GtkWidget *widget, GdkDragContext*, guint)
{
    GTKfilePopup *fs =
        (GTKfilePopup*)gtk_object_get_data(GTK_OBJECT(widget), "fsbag");
    if (!fs)
        return;
    if (fs->fs_cset_node) {
        GtkTreeModel *store = gtk_tree_view_get_model(GTK_TREE_VIEW(widget));
        GtkTreeIter iter;
        if (gtk_tree_model_get_iter(store, &iter, fs->fs_cset_node))
            gtk_tree_store_set(GTK_TREE_STORE(store), &iter, COL_BG, 0, -1);
    }
    fs_scroll_hdlr(widget);
}


// Approx. line height.
#define VINCR 25

namespace {
    // Vertical scroll adjustment function.
    //
    void
    move_vertical(GtkTreeView *tree, bool up)
    {
        GtkAdjustment *vadj = gtk_tree_view_get_vadjustment(tree);
        if (!vadj)
            return;
        if (up) {
#if GTK_CHECK_VERSION(2,12,0)
            GdkWindow *window = gtk_widget_get_window(GTK_WIDGET(tree));
#else
            GdkWindow *window = GTK_WIDGET(tree)->window;
#endif
            int hei;
            gdk_window_get_size(window, 0, &hei);
            float value = vadj->value;
            float upper = vadj->upper - hei;
            value += VINCR;
            if (value > upper)
                value = upper;
            gtk_adjustment_set_value(vadj, value);
        }
        else {
            float value = vadj->value;
            value -= VINCR;
            if (value < 0)
                value = 0;
            gtk_adjustment_set_value(vadj, value);
        }
    }
}


// Timer function for vertical autoscroll.
//
int
GTKfilePopup::fs_vertical_timeout(void *data)
{
    GTKfilePopup *fs = static_cast<GTKfilePopup*>(data);
    if (!fs)
        return (false);
    fs->fs_vtimer = 0;
    GTKfilePopup::fs_scroll_hdlr(fs->fs_tree);
    return (false);
}


// Scroll the ctree drop site if the pointer is just above or below the
// window.
//
void
GTKfilePopup::fs_scroll_hdlr(GtkWidget *tree)
{
    if (!GTK_IS_TREE_VIEW(tree))
        return;
#if GTK_CHECK_VERSION(2,12,0)
    GdkWindow *window = gtk_widget_get_window(tree);
#else
    GdkWindow *window = tree->window;
#endif
    if (!window)
        return;
    int x, y;
    GdkModifierType mask;
    gdk_window_get_pointer(window, &x, &y, &mask);
    if (!(mask & (GDK_BUTTON1_MASK | GDK_BUTTON2_MASK | GDK_BUTTON3_MASK)))
        return;
    GTKfilePopup *fs =
        (GTKfilePopup*)gtk_object_get_data(GTK_OBJECT(tree), "fsbag");
    if (!fs)
        return;
    int hei;
    gdk_window_get_size(window, 0, &hei);
#define SENS_PIXELS 4
#define VT_REP 200
    if (y < SENS_PIXELS) {
        if (fs->fs_vtimer)
            return;
        move_vertical(GTK_TREE_VIEW(tree), false);
        fs->fs_vtimer = gtk_timeout_add(VT_REP,
            (GtkFunction)fs_vertical_timeout, fs);
    }
    else if (hei - y < SENS_PIXELS) {
        if (fs->fs_vtimer)
            return;
        move_vertical(GTK_TREE_VIEW(tree), true);
        fs->fs_vtimer = gtk_timeout_add(VT_REP,
            (GtkFunction)fs_vertical_timeout, fs);
    }
    else if (fs->fs_vtimer) {
        gtk_timeout_remove(fs->fs_vtimer);
        fs->fs_vtimer = 0;
    }
}


//-----------------------------------------------------------------------------
// Callbacks for menu-launched pop-ups.

// Private static callback.
// Timer callback to check directory modification time.
//
int
GTKfilePopup::fs_files_timer(void *arg)
{
    GTKfilePopup *fs = static_cast<GTKfilePopup*>(arg);
    if (fs)
        fs->check_update();
    return (1);
}


// Private static callback.
// Action for "New Folder".
//
void
GTKfilePopup::fs_new_cb(const char *string, void *arg)
{
    fs_data *data = (fs_data*)arg;
    if (lstring::strdirsep(string))
        data->fs->PopUpMessage("Invalid name.", true);
    else {
        char *dir = fs_make_dir(data->string, string);
#ifdef WIN32
        if (mkdir(dir) != 0)
#else
        if (mkdir(dir, 0755) != 0)
#endif
            data->fs->PopUpMessage(strerror(errno), true);
        delete [] dir;
    }
    // pop down
    if (data->fs->wb_input)
        data->fs->wb_input->popdown();
    delete data;
}


// Private static callback.
// Action for "Delete".
//
void
GTKfilePopup::fs_delete_cb(bool yesno, void *arg)
{
    fs_data *data = (fs_data*)arg;
    if (yesno) {
        if (filestat::is_directory(data->string)) {
            if (rmdir(data->string) != 0)
                data->fs->PopUpMessage(strerror(errno), true);
        }
        else {
            if (unlink(data->string) != 0)
                data->fs->PopUpMessage(strerror(errno), true);
        }
    }
    delete data;
}


// Private static callback.
// Action for "Rename".
//
void
GTKfilePopup::fs_rename_cb(const char *string, void *arg)
{
    fs_data *data = (fs_data*)arg;
    if (lstring::strdirsep(string))
        data->fs->PopUpMessage("Invalid name.", true);
    else {
        char *path = lstring::copy(data->string);
        char *t = lstring::strrdirsep(path);
        if (t) {
            *t++ = 0;
            t = fs_make_dir(path, string);
            if (rename(data->string, t) != 0)
                data->fs->PopUpMessage(strerror(errno), true);
            delete [] t;
        }
        else
            data->fs->PopUpMessage("Internal error - bad path.", true);
        delete [] path;
    }
    // pop down
    if (data->fs->wb_input)
        data->fs->wb_input->popdown();
    delete data;
}


// Private static callback.
// Action for "New Root".
//
void
GTKfilePopup::fs_root_cb(const char *rootin, void *fsp)
{
    GTKfilePopup *fs = static_cast<GTKfilePopup*>(fsp);
    if (fs) {
        char *root = fs->get_newdir(rootin);
        if (root) {
            delete [] fs->fs_rootdir;
            fs->fs_rootdir = root;
            fs->init();
            if (fs->wb_input)
                fs->wb_input->popdown();
        }
    }
}


// Private static callback.
// Action for "New CWD".
//
void
GTKfilePopup::fs_cwd_cb(const char *wd, void *fsp)
{
    GTKfilePopup *fs = static_cast<GTKfilePopup*>(fsp);
    if (fs) {
        if (!wd || !*wd)
            wd = "~";
        char *nwd = fs->get_newdir(wd);
        if (nwd) {
            if (!chdir(nwd)) {
                if (fs->wb_input)
                    fs->wb_input->popdown();
                delete [] fs->fs_rootdir;
                fs->fs_rootdir = nwd;
                delete [] fs->fs_cwd_bak;
                fs->fs_cwd_bak = lstring::tocpp(getcwd(0, 0));
                fs->init();
            }
            else {
                char buf[256];
                sprintf(buf, "Directory change failed:\n%s", strerror(errno));
                fs->PopUpMessage(buf, true);
                delete [] nwd;
            }
        }
    }
}


//-----------------------------------------------------------------------------
// Misc. support functions

// Appropriately cat the two args into a valid directory path.  We assume
// that neither arg has a trailing directory separator.  The returned
// path will have a trailing separator only for the root path.
//
char *
GTKfilePopup::fs_make_dir(const char *parent_dir, const char *cur_dir)
{
    char *dir;
    if (parent_dir && *parent_dir) {
        if (!cur_dir || !*cur_dir)
            return (lstring::copy(parent_dir));
        if (is_root(parent_dir)) {
            if (lstring::is_rooted(cur_dir))
                return (lstring::copy(cur_dir));
            dir = new char[strlen(cur_dir) + 2];
            *dir = '/';
            strcpy(dir+1, cur_dir);
            return (dir);
        }
        dir = new char[strlen(parent_dir) + strlen(cur_dir) + 2];
        sprintf(dir, "%s/%s", parent_dir, cur_dir);
    }
    else {
        dir = new char[cur_dir ? strlen(cur_dir) + 2 : 2];
        sprintf(dir, "/%s", cur_dir ? cur_dir : "");
    }
    return (dir);
}


// Remove the entries that appear in both lists from both lists.
//
void
GTKfilePopup::fs_fdiff(stringlist **f1p, stringlist **f2p)
{
	stringlist *f1 = *f1p;
	stringlist *f2 = *f2p;
	for (stringlist *f = f1; f; f = f->next) {
		for (stringlist *ff = f2; ff; ff = ff->next) {
			if (!ff->string)
			    continue;
		    if (!strcmp(f->string, ff->string)) {
				delete [] ff->string;
				ff->string = 0;
				delete [] f->string;
				f->string = 0;
				break;
			}
		}
	}
	stringlist *f = f1;
	f1 = 0;
	while (f) {
		stringlist *fn = f->next;
		if (f->string) {
			f->next = f1;
			f1 = f;
		}
		else
		    delete f;
		f = fn;
	}
	f = f2;
	f2 = 0;
	while (f) {
		stringlist *fn = f->next;
		if (f->string) {
			f->next = f2;
			f2 = f;
		}
		else
		    delete f;
		f = fn;
	}
	*f1p = f1;
	*f2p = f2;
}


// Return a list of subdirs under dir.
//
stringlist *
GTKfilePopup::fs_list_subdirs(const char *dir)
{
    stringlist *s0 = 0;
    DIR *wdir = opendir(dir);
    if (wdir) {
        int sz = strlen(dir) + 64;
        char *p = new char[sz];
        strcpy(p, dir);
        char *dt = p + strlen(p) - 1;
        if (!lstring::is_dirsep(*dt)) {
            *++dt = '/';
            *++dt = 0;
        }
        else
            dt++;
        int reflen = (dt - p);
        struct dirent *de;
        while ((de = readdir(wdir)) != 0) {
            if (!strcmp(de->d_name, "."))
                continue;
            if (!strcmp(de->d_name, ".."))
                continue;
#ifdef DT_DIR
            if (de->d_type != DT_UNKNOWN) {
                // Linux glibc returns DT_UNKNOWN for everything
                if (de->d_type == DT_DIR)
                    s0 = new stringlist(lstring::copy(de->d_name), s0);
                if (de->d_type != DT_LNK)
                    continue;
            }
#endif
            int flen = strlen(de->d_name);
            while (reflen + flen >= sz) {
                sz += sz;
                char *t = new char[sz];
                *dt = 0;
                dt = lstring::stpcpy(t, p);
                delete [] p;
                p = t;
            }
            strcpy(dt, de->d_name);
            if (filestat::is_directory(p))
                s0 = new stringlist(lstring::copy(de->d_name), s0);
        }
        delete [] p;
        closedir(wdir);
    }
    return (s0);
}


// Static function.
// Return the modification time for dir.
//
unsigned long
GTKfilePopup::fs_dirtime(const char *dir)
{
    struct stat st;
    if (!stat(dir, &st))
        return (st.st_mtime);
    return (0);
}
// End of GTKfilePopup functions


//=========================================================================
// File Action Dialog
//=========================================================================

namespace {
    int progress_destroy_timeout(void*);

    // Invoke the shell function in buf, and return any output.
    //
    void doit(sLstr &lstr)
    {
        FILE *fp = popen(lstr.string(), "r");
        lstr.free();
        if (!fp) {
            lstr.add("process open failed, unknown error.");
            return;
        }
        char buf[256];
        while (fgets(buf, 256, fp) != 0)
            lstr.add(buf);
        pclose(fp);
    }


    // Add the path to lstr, quoting if necessary, and fixing dirseps
    // in Windows.
    //
    void add_path(sLstr &lstr, const char *path)
    {
        const char *p = path;
        char *tok = lstring::getqtok(&p);
        delete [] tok;
        tok = lstring::getqtok(&p);
        bool nt = (tok != 0);
        delete [] tok;
        if (nt)
            lstr.add_c('"');
#ifdef WIN32
        for (p = path; *p; p++) {
            if (*p == '/')
                lstr.add_c('\\');
            else
                lstr.add_c(*p);
        }
#else
        lstr.add(path);
#endif
        if (nt)
            lstr.add_c('"');
    }
}


// Actually perform the move/copy/link.
//
void
gtkinterf::gtk_DoFileAction(GtkWidget *shell, const char *src, const char *dst,
    GdkDragContext *context, bool ask)
{
    if (!src || !*src || !dst || !*dst || !strcmp(src, dst))
        return;

    // prohibit src = path/file, dst = path
    if (lstring::prefix(dst, src)) {
        const char *s = src + strlen(dst);
        if (*s == '/') {
            s++;
            if (!strchr(s, '/'))
                return;
        }
    }

    // if src is a directory, prohibit a recursive hierarchy
    bool isdir = filestat::is_directory(src);
    if (isdir && lstring::prefix(src, dst))
        return;

    if (ask && !GRwbag::IsNoAskFileAction()) {
        gtk_FileAction(shell, src, dst, context);
        return;
    }

    sLstr lstr;
    GtkWidget *prog = 0;
    switch (context->suggested_action) {
    default:
    case GDK_ACTION_COPY:
#ifdef WIN32
        lstr.add("xcopy ");
#else
        lstr.add("cp ");
        if (isdir)
            lstr.add("-R ");
#endif
        add_path(lstr, src);
        lstr.add_c(' ');
        add_path(lstr, dst);
#ifdef WIN32
        lstr.add(" /y /q");
        if (isdir)
            lstr.add(" /s /e");
#endif
        lstr.add(" 2>&1");
        prog = gtk_Progress(shell, "Copying...");
        doit(lstr);
        break;
    case GDK_ACTION_MOVE:
#ifdef WIN32
        lstr.add("move /y ");
#else
        lstr.add("mv ");
#endif
        add_path(lstr, src);
        lstr.add_c(' ');
        add_path(lstr, dst);
        lstr.add(" 2>&1");
        prog = gtk_Progress(shell, "Moving...");
        doit(lstr);
        break;
    case GDK_ACTION_LINK:
#ifdef WIN32
        lstr.add("mklink ");
        if (isdir)
            lstr.add("/D ");
#else
        lstr.add("ln -s ");
#endif
        add_path(lstr, src);
        lstr.add_c(' ');
        add_path(lstr, dst);
        lstr.add(" 2>&1");
        prog = gtk_Progress(shell, "Linking...");
        doit(lstr);
        break;
    case GDK_ACTION_ASK:
        gtk_FileAction(shell, src, dst, context);
        return;
    }
    if (prog)
        gtk_timeout_add(1000, (GtkFunction)progress_destroy_timeout,
            prog);

    if (lstr.string())
        gtk_Message(shell, false, lstr.string());
}


namespace {
    // Dismiss the file action popup.
    //
    void
    action_cancel(GtkWidget *caller, void*)
    {
        GtkWidget *popup = (GtkWidget*)gtk_object_get_data(GTK_OBJECT(caller),
            "popup");
        if (!popup)
            popup  = caller;
        if (popup) {
            gtk_signal_disconnect_by_func(GTK_OBJECT(popup),
                GTK_SIGNAL_FUNC(action_cancel), popup);
            gtk_widget_destroy(popup);
        }
    }


    // Perform the action requested by the user.
    //
    void
    action_proc(GtkWidget *caller, void *client_data)
    {
        if (client_data == 0) {
            action_cancel(caller, 0);
            return;
        }
        GtkWidget *popup = (GtkWidget*)gtk_object_get_data(GTK_OBJECT(caller),
            "popup");
        if (popup) {
            const char *src = 0, *dst = 0;
            GtkWidget *entry_src =
                (GtkWidget*)gtk_object_get_data(GTK_OBJECT(popup), "source");
            if (entry_src)
                src = gtk_entry_get_text(GTK_ENTRY(entry_src));
            GtkWidget *entry_dst =
                (GtkWidget*)gtk_object_get_data(GTK_OBJECT(popup), "dest");
            if (entry_dst)
                dst = gtk_entry_get_text(GTK_ENTRY(entry_dst));

            GdkDragAction action =
                (GdkDragAction)(long)gtk_object_get_data(GTK_OBJECT(caller),
                    "action");
            if (action == GDK_ACTION_MOVE || action == GDK_ACTION_COPY ||
                    action == GDK_ACTION_LINK) {
                GdkDragContext *context = (GdkDragContext*)client_data;
                context->suggested_action = action;
                gtk_DoFileAction(popup, src, dst, context, false);
            }
        }
    }
}


// Function to pop up a dialog for file move/copy/linking.  The shell
// is used for positioning.
//
// data set:
// popup            "source"            entry_src
// popup            "dest"              entry_dst
// button           "popup"             popup
// button           "action"            GDK_ACTION_MOVE
// button           "popup"             popup
// button           "action"            GDK_ACTION_COPY
// button           "popup"             popup
// button           "action"            GDK_ACTION_LINK
// button           "popup"             popup
//
void
gtkinterf::gtk_FileAction(GtkWidget *shell, const char *src, const char *dst,
    GdkDragContext *context)
{
    GtkWidget *popup = gtk_NewPopup(0, "File Transfer", action_cancel, 0);

    GtkWidget *form = gtk_table_new(1, 3, false);
    gtk_widget_show(form);
    gtk_container_add(GTK_CONTAINER(popup), form);

    GtkWidget *entry_src = gtk_entry_new();
    gtk_widget_show(entry_src);
    gtk_entry_set_text(GTK_ENTRY(entry_src), src);
    GtkWidget *frame = gtk_frame_new("Source");
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), entry_src);
    gtk_object_set_data(GTK_OBJECT(popup), "source", entry_src);
    gtk_table_attach(GTK_TABLE(form), frame, 0, 1, 0, 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    GtkWidget *entry_dst = gtk_entry_new();
    gtk_widget_show(entry_dst);
    gtk_entry_set_text(GTK_ENTRY(entry_dst), dst);
    frame = gtk_frame_new("Destination");
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), entry_dst);
    gtk_object_set_data(GTK_OBJECT(popup), "dest", entry_dst);
    gtk_table_attach(GTK_TABLE(form), frame, 0, 1, 1, 2,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    GtkWidget *hbox = gtk_hbox_new(false, 2);
    gtk_widget_show(hbox);

    GtkWidget *button = gtk_button_new_with_label("Move");
    gtk_widget_set_name(button, "Move");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(action_proc), context);
    gtk_object_set_data(GTK_OBJECT(button), "popup", popup);
    gtk_object_set_data(GTK_OBJECT(button), "action",
        (void*)GDK_ACTION_MOVE);
    gtk_box_pack_start(GTK_BOX(hbox), button, true, true, 0);

    button = gtk_button_new_with_label("Copy");
    gtk_widget_set_name(button, "Copy");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(action_proc), context);
    gtk_object_set_data(GTK_OBJECT(button), "popup", popup);
    gtk_object_set_data(GTK_OBJECT(button), "action",
        (void*)GDK_ACTION_COPY);
    gtk_box_pack_start(GTK_BOX(hbox), button, true, true, 0);

    button = gtk_button_new_with_label("Link");
    gtk_widget_set_name(button, "Link");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(action_proc), context);
    gtk_object_set_data(GTK_OBJECT(button), "popup", popup);
    gtk_object_set_data(GTK_OBJECT(button), "action",
        (void*)GDK_ACTION_LINK);
    gtk_box_pack_start(GTK_BOX(hbox), button, true, true, 0);

    button = gtk_button_new_with_label("Cancel");
    gtk_widget_set_name(button, "Cancel");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(action_proc), 0);
    gtk_object_set_data(GTK_OBJECT(button), "popup", popup);
    gtk_box_pack_start(GTK_BOX(hbox), button, true, true, 0);

    gtk_table_attach(GTK_TABLE(form), hbox, 0, 1, 2, 3,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    gtk_window_set_focus(GTK_WINDOW(popup), button);

    gtk_widget_set_usize(popup, 400, -1);
    if (shell) {
        gtk_window_set_transient_for(GTK_WINDOW(popup), GTK_WINDOW(shell));
        GRX->SetPopupLocation(GRloc(), popup, shell);
    }
    gtk_widget_show(popup);
}


//=========================================================================
// Progress Bar
//=========================================================================

namespace {
    // Cancel callback for the progress bar.
    //
    void
    progress_cancel(GtkWidget*, void *client_data)
    {
        GtkWidget *popup = (GtkWidget*)client_data;
        if (popup) {
            unsigned long timer =
                (unsigned long)gtk_object_get_data(GTK_OBJECT(popup), "timer");
            if (timer)
                gtk_timeout_remove(timer);
            gtk_signal_disconnect_by_func(GTK_OBJECT(popup),
                GTK_SIGNAL_FUNC(progress_cancel), popup);
            gtk_widget_destroy(popup);
        }
    }


    // Move the progress bar along.
    //
    int
    progress_timeout(void *data)
    {

        float new_val = gtk_progress_get_value(GTK_PROGRESS(data)) + 2;
        GtkAdjustment *adj = GTK_PROGRESS(data)->adjustment;
        if (new_val > adj->upper)
            new_val = adj->lower;
        gtk_progress_set_value(GTK_PROGRESS(data), new_val);
        return (true);
    }


    int
    progress_destroy_timeout(void *data)
    {
        if (data)
            gtk_widget_destroy(GTK_WIDGET(data));
        return (false);
    }
}


// Pop up a progress bar widget, showing the message.  Destroy the
// returned widget to remove.
//
// data set:
// popup            "timer"             timer
//
GtkWidget *
gtkinterf::gtk_Progress(GtkWidget *shell, const char *msg)
{
    GtkWidget *popup = gtk_NewPopup(0, "Working", progress_cancel, 0);

    GtkWidget *form = gtk_table_new(1, 2, false);
    gtk_widget_show(form);
    gtk_container_add(GTK_CONTAINER(popup), form);

    GtkWidget *label = gtk_label_new(msg);
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);

    GtkWidget *frame = gtk_frame_new(0);
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), label);

    gtk_table_attach(GTK_TABLE(form), frame, 0, 1, 0, 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    GtkAdjustment *adj =
        (GtkAdjustment*)gtk_adjustment_new(0, 1, 100, 0, 0, 0);
    GtkWidget *pbar = gtk_progress_bar_new_with_adjustment(adj);
    gtk_widget_show(pbar);
    gtk_progress_bar_set_bar_style(GTK_PROGRESS_BAR(pbar),
        GTK_PROGRESS_CONTINUOUS);
    gtk_progress_set_activity_mode(GTK_PROGRESS(pbar), true);
    unsigned timer = gtk_timeout_add(100, (GtkFunction)progress_timeout,
        pbar);
    gtk_object_set_data(GTK_OBJECT(popup), "timer", (void*)(long)timer);

    gtk_table_attach(GTK_TABLE(form), pbar, 0, 1, 1, 2,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    if (shell) {
        gtk_window_set_transient_for(GTK_WINDOW(popup), GTK_WINDOW(shell));
        GRX->SetPopupLocation(GRloc(), popup, shell);
    }
    gtk_widget_show(popup);
    gdk_flush();
    gtk_DoEvents(100);
    return (popup);
}


//=========================================================================
// Error Message Pop-Up
//=========================================================================

namespace {
    // Cancel callback for the error message popup.
    //
    void
    fail_cancel(GtkWidget *caller, void*)
    {
        GtkWidget *popup = (GtkWidget*)gtk_object_get_data(GTK_OBJECT(caller),
            "popup");
        if (!popup)
            popup = caller;
        if (popup) {
            gtk_signal_disconnect_by_func(GTK_OBJECT(popup),
                GTK_SIGNAL_FUNC(fail_cancel), popup);
            gtk_widget_destroy(popup);
        }
    }
}


// Pop up an error message.  The shell is used for positioning.
//
// data set:
// button           "popup"             popup
//
void
gtkinterf::gtk_Message(GtkWidget *shell, bool failed, const char *msg)
{
    GtkWidget *popup = gtk_NewPopup(0,
        failed ? "Action Failed" : "Action Complete", fail_cancel, 0);
    gtk_widget_set_usize(popup, 240, -1);

    GtkWidget *form = gtk_table_new(1, 2, false);
    gtk_widget_show(form);
    gtk_container_add(GTK_CONTAINER(popup), form);

    GtkWidget *label = gtk_label_new(msg);
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);

    GtkWidget *frame = gtk_frame_new(0);
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), label);

    gtk_table_attach(GTK_TABLE(form), frame, 0, 1, 0, 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    GtkWidget *button = gtk_button_new_with_label("Dismiss");
    gtk_widget_set_name(button, "Dismiss");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(fail_cancel), 0);
    gtk_object_set_data(GTK_OBJECT(button), "popup", popup);

    gtk_table_attach(GTK_TABLE(form), button, 0, 1, 1, 2,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    gtk_window_set_focus(GTK_WINDOW(popup), button);

    if (shell) {
        gtk_window_set_transient_for(GTK_WINDOW(popup), GTK_WINDOW(shell));
        GRX->SetPopupLocation(GRloc(), popup, shell);
    }
    gtk_widget_show(popup);
}



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
 $Id: gtklibs.cc,v 5.62 2017/04/13 17:06:22 stevew Exp $
 *========================================================================*/

#include "config.h"
#include "main.h"
#include "cvrt.h"
#include "editif.h"
#include "dsp_inlines.h"
#include "fio.h"
#include "fio_library.h"
#include "gtkmain.h"
#include "gtklist.h"
#include "gtkmcol.h"
#include "gtkfont.h"
#include "gtkinlines.h"
#include "events.h"
#include "filestat.h"
#include "pathlist.h"
#include <algorithm>

#ifdef HAVE_DIRENT_H
#include <dirent.h>
#ifndef direct
#define direct dirent
#endif
#else
#ifdef HAVE_SYS_DIR_H
#include <sys/dir.h>
#endif
#endif


//----------------------------------------------------------------------
//  Libraries Popup
//
// Help system keywords used:
//  libspanel

namespace {
    namespace gtklibs {
        struct sLB : public gtk_bag
        {
            sLB(GRobject);
            ~sLB();

            char *get_selection();
            void update();

            enum { LBnil, LBopen, LBcont, LBhelp, LBovr };

        private:
            void pop_up_contents();

            static void lb_cancel(GtkWidget*, void*);
            static bool lb_selection_proc(GtkTreeSelection*, GtkTreeModel*,
                GtkTreePath*, bool, void*);
            static int lb_focus_proc(GtkWidget*, GdkEvent*, void*);
            static int lb_button_press_proc(GtkWidget*, GdkEvent*, void*);
            static void lb_action_proc(GtkWidget*, void*);
            static void lb_content_cb(const char*, void*);
            static stringlist *lb_pathlibs();
            static stringlist *lb_add_dir(char*, stringlist*);

            GRobject lb_caller;
            GtkWidget *lb_openbtn;
            GtkWidget *lb_contbtn;
            GtkWidget *lb_list;
            GtkWidget *lb_noovr;
            GRmcolPopup *lb_content_pop;
            char *lb_selection;
            char *lb_contlib;

            GdkPixbuf *lb_open_pb;
            GdkPixbuf *lb_close_pb;
            bool lb_no_select;      // treeview focus hack

            static const char *nolibmsg;
        };

        sLB *LB;
    }
}

using namespace gtklibs;

const char *sLB::nolibmsg = "There are no libraries found.";

// Contests pop-up buttons.
#define OPEN_BTN "Open"
#define PLACE_BTN "Place"


// Static function.
//
char *
main_bag::get_lib_selection()
{
    if (LB)
        return (LB->get_selection());
    return (0);
}


// Static function.
// Called on crash to prevent updates.
//
void
main_bag::libs_panic()
{
    LB = 0;
}


// Pop up a listing of the libraries found in the search path.
// The popup contains the following buttons:
//  Open/Close:   Open or close the selected library.
//  Contents:     Pop up a listing of the selected library contents.
//
void
cConvert::PopUpLibraries(GRobject caller, ShowMode mode)
{
    if (!GRX || !mainBag())
        return;
    if (mode == MODE_OFF) {
        delete LB;
        return;
    }
    if (mode == MODE_UPD) {
        if (LB)
            LB->update();
        return;
    }
    if (LB)
        return;

    new sLB(caller);
    if (!LB->Shell()) {
        delete LB;
        return;
    }
    gtk_window_set_transient_for(GTK_WINDOW(LB->Shell()),
        GTK_WINDOW(mainBag()->Shell()));

    GRX->SetPopupLocation(GRloc(LW_UL), LB->Shell(), mainBag()->Viewport());
    gtk_widget_show(LB->Shell());
}


sLB::sLB(GRobject c)
{
    LB = this;
    lb_caller = c;
    lb_openbtn = 0;
    lb_contbtn = 0;
    lb_list = 0;
    lb_noovr = 0;
    lb_content_pop = 0;
    lb_selection = 0;
    lb_contlib = 0;
    lb_open_pb = 0;
    lb_close_pb = 0;
    lb_no_select = false;

    wb_shell = gtk_NewPopup(0, "Libraries", lb_cancel, 0);
    if (!wb_shell)
        return;
    gtk_window_set_default_size(GTK_WINDOW(wb_shell), 450, 200);

    GtkWidget *form = gtk_table_new(1, 5, false);
    gtk_widget_show(form);
    gtk_container_add(GTK_CONTAINER(wb_shell), form);

    GtkWidget *hbox = gtk_hbox_new(false, 2);
    gtk_widget_show(hbox);

    lb_openbtn = gtk_button_new_with_label("Open/Close");
    gtk_widget_set_name(lb_openbtn, "Open");
    gtk_widget_show(lb_openbtn);
    gtk_signal_connect(GTK_OBJECT(lb_openbtn), "clicked",
        GTK_SIGNAL_FUNC(lb_action_proc), (void*)LBopen);
    gtk_box_pack_start(GTK_BOX(hbox), lb_openbtn, true, true, 0);

    lb_contbtn = gtk_button_new_with_label("Contents");
    gtk_widget_set_name(lb_contbtn, "Contents");
    gtk_widget_show(lb_contbtn);
    gtk_signal_connect(GTK_OBJECT(lb_contbtn), "clicked",
        GTK_SIGNAL_FUNC(lb_action_proc), (void*)LBcont);
    gtk_box_pack_start(GTK_BOX(hbox), lb_contbtn, true, true, 0);

    GtkWidget *button = gtk_button_new_with_label("Help");
    gtk_widget_set_name(button, "Help");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(lb_action_proc), (void*)LBhelp);
    gtk_box_pack_start(GTK_BOX(hbox), button, true, true, 0);

    gtk_table_attach(GTK_TABLE(form), hbox, 0, 1, 0, 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    //
    // scrolled list
    //
    GtkWidget *swin = gtk_scrolled_window_new(0, 0);
    gtk_widget_show(swin);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin),
        GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_set_border_width(GTK_CONTAINER(swin), 2);

    const char *title[2];
    title[0] = "Open?";
    title[1] = "Libraries in search path, click to select";
    GtkListStore *store = gtk_list_store_new(2, GDK_TYPE_PIXBUF,
        G_TYPE_STRING);
    lb_list = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
    gtk_widget_show(lb_list);
    gtk_tree_view_set_enable_search(GTK_TREE_VIEW(lb_list), false);
    GtkTreeViewColumn *tvcol = gtk_tree_view_column_new();
    gtk_tree_view_column_set_title(tvcol, title[0]);
    gtk_tree_view_append_column(GTK_TREE_VIEW(lb_list), tvcol);
    GtkCellRenderer *rnd = gtk_cell_renderer_pixbuf_new();
    gtk_tree_view_column_pack_start(tvcol, rnd, true);
    gtk_tree_view_column_add_attribute(tvcol, rnd, "pixbuf", 0);

    tvcol = gtk_tree_view_column_new();
    gtk_tree_view_column_set_title(tvcol, title[1]);
    gtk_tree_view_append_column(GTK_TREE_VIEW(lb_list), tvcol);
    rnd = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(tvcol, rnd, true);
    gtk_tree_view_column_add_attribute(tvcol, rnd, "text", 1);

    GtkTreeSelection *sel =
        gtk_tree_view_get_selection(GTK_TREE_VIEW(lb_list));
    gtk_tree_selection_set_select_function(sel,
        (GtkTreeSelectionFunc)lb_selection_proc, 0, 0);
    // TreeView bug hack, see note with handlers.   
    gtk_signal_connect(GTK_OBJECT(lb_list), "focus",
        GTK_SIGNAL_FUNC(lb_focus_proc), this);

    gtk_container_add(GTK_CONTAINER(swin), lb_list);
    gtk_widget_set_usize(lb_list, -1, 100);

    // Set up font and tracking.
    GTKfont::setupFont(lb_list, FNT_PROP, true);

    gtk_signal_connect(GTK_OBJECT(lb_list), "button-press-event",
        GTK_SIGNAL_FUNC(lb_button_press_proc), this);

    gtk_table_attach(GTK_TABLE(form), swin, 0, 1, 1, 2,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);

    GtkWidget *sep = gtk_hseparator_new();
    gtk_widget_show(sep);
    gtk_table_attach(GTK_TABLE(form), sep, 0, 1, 2, 3,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    //
    // dismiss button line
    //
    hbox = gtk_hbox_new(false, 2);
    gtk_widget_show(hbox);

    lb_noovr = gtk_toggle_button_new_with_label("No Overwrite Lib Cells");
    gtk_widget_set_name(lb_noovr, "NoOverwrite");
    gtk_widget_show(lb_noovr);
    gtk_signal_connect(GTK_OBJECT(lb_noovr), "clicked",
        GTK_SIGNAL_FUNC(lb_action_proc), (void*)LBovr);
    gtk_box_pack_start(GTK_BOX(hbox), lb_noovr, false, false, 0);

    button = gtk_button_new_with_label("Dismiss");
    gtk_widget_set_name(button, "Dismiss");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(lb_cancel), 0);
    gtk_box_pack_start(GTK_BOX(hbox), button, true, true, 0);

    gtk_table_attach(GTK_TABLE(form), hbox, 0, 1, 3, 4,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    gtk_window_set_focus(GTK_WINDOW(wb_shell), button);

    // Create pixmaps.
    lb_open_pb = gdk_pixbuf_new_from_xpm_data(wb_open_folder_xpm);
    lb_close_pb = gdk_pixbuf_new_from_xpm_data(wb_closed_folder_xpm);

    update();
}


sLB::~sLB()
{
    LB = 0;
    delete [] lb_selection;
    delete [] lb_contlib;

    if (lb_caller)
        GRX->Deselect(lb_caller);
    if (lb_content_pop)
        lb_content_pop->popdown();

    if (wb_shell)
        gtk_signal_disconnect_by_func(GTK_OBJECT(wb_shell),
            GTK_SIGNAL_FUNC(lb_cancel), wb_shell);

    if (lb_open_pb)
        g_object_unref(lb_open_pb);
    if (lb_close_pb)
        g_object_unref(lb_close_pb);
}


char *
sLB::get_selection()
{
    sLB *lbt = this;
    if (lbt && lb_contlib && lb_content_pop) {
        char *sel = lb_content_pop->get_selection();
        if (sel) {
            int len = strlen(lb_contlib) + strlen(sel) + 2;
            char *tbuf = new char[len];
            char *t = lstring::stpcpy(tbuf, lb_contlib);
            *t++ = ' ';
            strcpy(t, sel);
            delete [] sel;
            return (tbuf);
        }
    }
    return (0);
}


// Update the listing of open directories in the main pop-up.
//
void
sLB::update()
{
    {
        sLB *lbt = this;
        if (!lbt)
            return;
    }

    GRX->SetStatus(lb_noovr, CDvdb()->getVariable(VA_NoOverwriteLibCells));

    stringlist *liblist = lb_pathlibs();
    if (!liblist)
        liblist = new stringlist(lstring::copy(nolibmsg), 0);
    GtkListStore *store =
        GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(LB->lb_list)));
    gtk_list_store_clear(store);
    GtkTreeIter iter;
    for (stringlist *l = liblist; l; l = l->next) {
        gtk_list_store_append(store, &iter);
        if (FIO()->FindLibrary(l->string))
            gtk_list_store_set(store, &iter, 0, lb_open_pb, 1, l->string, -1);
        else
            gtk_list_store_set(store, &iter, 0, lb_close_pb, 1, l->string, -1);
    }
    liblist->free();
    char *oldsel = lb_selection;
    lb_selection = 0;
    gtk_widget_set_sensitive(lb_openbtn, false);
    gtk_widget_set_sensitive(lb_contbtn, false);

    if (lb_content_pop) {
        if (lb_contlib && !FIO()->FindLibrary(lb_contlib))
            lb_content_pop->update(0, "No library selected");
        if (DSP()->MainWdesc()->DbType() == WDchd)
            lb_content_pop->set_button_sens(0);
        else
            lb_content_pop->set_button_sens(-1);
    }
    if (oldsel) {
        // This re-selects the previously selected library.
        if (!gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter)) {
            delete [] oldsel;
            return;
        }
        for (;;) {
            char *text;
            gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, 1, &text, -1);
            int sc = strcmp(text, oldsel);
            free(text);
            if (!sc) {
                GtkTreeSelection *sel =
                    gtk_tree_view_get_selection(GTK_TREE_VIEW(lb_list));
                gtk_tree_selection_select_iter(sel, &iter);
                break;
            }
            if (!gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter))
                break;
        }
    }
    delete [] oldsel;
}


// Pop up a listing of the contents of the selected library.
//
void
sLB::pop_up_contents()
{
    {
        sLB *lbt = this;
        if (!lbt)
            return;
    }
    if (!lb_selection)
        return;

    delete [] lb_contlib;
    lb_contlib = lstring::copy(lb_selection);
    stringlist *list = FIO()->GetLibNamelist(lb_contlib, LIBuser);
    if (list) {
        sLstr lstr;
        lstr.add("References found in library - click to select\n");
        lstr.add(lb_contlib);

        if (lb_content_pop)
            lb_content_pop->update(list, lstr.string());
        else {
            const char *buttons[3];
            buttons[0] = OPEN_BTN;
            buttons[1] = EditIf()->hasEdit() ? PLACE_BTN : 0;
            buttons[2] = 0;

            int pagesz = 0;
            const char *s = CDvdb()->getVariable(VA_ListPageEntries);
            if (s) {
                pagesz = atoi(s);
                if (pagesz < 100 || pagesz > 50000)
                    pagesz = 0;
            }
            lb_content_pop = DSPmainWbagRet(PopUpMultiCol(list, lstr.string(),
                lb_content_cb, 0, buttons, pagesz));
            if (lb_content_pop) {
                lb_content_pop->register_usrptr((void**)&lb_content_pop);
                if (DSP()->MainWdesc()->DbType() == WDchd)
                    lb_content_pop->set_button_sens(0);
                else
                    lb_content_pop->set_button_sens(-1);
            }
        }
        list->free();
    }
}


// Static function.
// Dismissal callback
//
void
sLB::lb_cancel(GtkWidget*, void*)
{
    Cvt()->PopUpLibraries(0, MODE_OFF);
}


// Static function.
// Selection callback for the list.  This is called when a new selection
// is made, but not when the selection disappears, which happens when the
// list is updated.
//
bool
sLB::lb_selection_proc(GtkTreeSelection*, GtkTreeModel *store,
    GtkTreePath *path, bool issel, void *)
{
    if (LB) {
        if (LB->lb_no_select && !issel)
            return (false);
        char *text = 0;
        GtkTreeIter iter;
        if (gtk_tree_model_get_iter(store, &iter, path))
            gtk_tree_model_get(store, &iter, 1, &text, -1);
        if (!text || !strcmp(nolibmsg, text)) {
            gtk_widget_set_sensitive(LB->lb_openbtn, false);
            gtk_widget_set_sensitive(LB->lb_contbtn, false);
            free(text);
            return (false);
        }
        // Behavior is a bit strange.  When clicking on a new selection,
        // we get called three times:
        // 0  old  (initial click, integer is issel value)
        // 0  new  (the second click an a new library gives these three)
        // 1  old
        // 0  new
        // printf("%d %s\n", issel, text);

        if (issel) {
            gtk_widget_set_sensitive(LB->lb_openbtn, false);
            gtk_widget_set_sensitive(LB->lb_contbtn, false);
            free(text);
            return (true);
        }
        if (!LB->lb_selection || strcmp(text, LB->lb_selection)) {
            delete [] LB->lb_selection;
            LB->lb_selection = lstring::copy(text);
        }
        gtk_widget_set_sensitive(LB->lb_openbtn, true);
        gtk_widget_set_sensitive(LB->lb_contbtn, 
            (FIO()->FindLibrary(text) != 0));
        free(text);
    }
    return (true);
}


// Static function.
// This handler is a hack to avoid a GtkTreeWidget defect:  when focus
// is taken and there are no selections, the 0'th row will be
// selected.  There seems to be no way to avoid this other than a hack
// like this one.  We set a flag to lock out selection changes in this
// case.
//
int
sLB::lb_focus_proc(GtkWidget*, GdkEvent*, void*)
{
    if (LB) {
        GtkTreeSelection *sel =
            gtk_tree_view_get_selection(GTK_TREE_VIEW(LB->lb_list));
        // If nothing selected set the flag.
        if (!gtk_tree_selection_get_selected(sel, 0, 0))
            LB->lb_no_select = true;
    }
    return (false);
}


// Toggle closed/open by clicking on the icon in the selected row.
//
int
sLB::lb_button_press_proc(GtkWidget*, GdkEvent *event, void*)
{
    if (LB) {
        GtkTreePath *p;
        GtkTreeViewColumn *col;
        if (!gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(LB->lb_list),
                (int)event->button.x, (int)event->button.y, &p,
                &col, 0, 0))
            return (false);
        if (col != gtk_tree_view_get_column(GTK_TREE_VIEW(LB->lb_list), 0)) {
            gtk_tree_path_free(p);
            return (false);
        }
        GtkTreeModel *mod =
            gtk_tree_view_get_model(GTK_TREE_VIEW(LB->lb_list));
        GtkTreeIter iter;
        gtk_tree_model_get_iter(mod, &iter, p);
        gtk_tree_path_free(p);

        GtkTreeSelection *sel =
            gtk_tree_view_get_selection(GTK_TREE_VIEW(LB->lb_list));
        if (!gtk_tree_selection_iter_is_selected(sel, &iter))
            return (false);

        if (LB->lb_selection) {
            char *tmp = lstring::copy(LB->lb_selection);
            if (FIO()->FindLibrary(tmp))
                FIO()->CloseLibrary(tmp, LIBuser);
            else
                FIO()->OpenLibrary(0, tmp);
            // These call update.
            delete [] tmp;
        }
    }
    return (false);
}


// Static function.
// Consolidated handler for the buttons.
//
void
sLB::lb_action_proc(GtkWidget *caller, void *client_data)
{
    if (!LB)
        return;
    if (client_data == (void*)LBhelp) {
        DSPmainWbag(PopUpHelp("libspanel"))
        return;
    }
    if (client_data == (void*)LBcont) {
        LB->pop_up_contents();
        return;
    }
    if (client_data == (void*)LBopen) {
        if (LB->lb_selection) {
            char *tmp = lstring::copy(LB->lb_selection);
            if (FIO()->FindLibrary(tmp))
                FIO()->CloseLibrary(tmp, LIBuser);
            else
                FIO()->OpenLibrary(0, tmp);
            // These call update.
            delete [] tmp;
        }
        return;
    }

    bool state = GRX->GetStatus(caller);

    if (client_data == (void*)LBovr) {
        if (state)
            CDvdb()->setVariable(VA_NoOverwriteLibCells, "");
        else
            CDvdb()->clearVariable(VA_NoOverwriteLibCells);
        return;
    }
}


// Static function.
// Callback for a content window.
//
void
sLB::lb_content_cb(const char *cellname, void*)
{
    if (!LB)
        return;
    if (!LB->lb_contlib || !LB->lb_content_pop)
        return;
    if (!cellname)
        return;
    if (*cellname != '/')
        return;

    // Callback from button press.
    cellname++;
    if (!strcmp(cellname, OPEN_BTN)) {
        char *sel = LB->lb_content_pop->get_selection();
        if (sel) {
            EV()->InitCallback();
            XM()->EditCell(LB->lb_contlib, false, FIO()->DefReadPrms(), sel);
            delete [] sel;
        }
    }
    else if (!strcmp(cellname, PLACE_BTN)) {
        char *sel = LB->lb_content_pop->get_selection();
        if (sel) {
            EV()->InitCallback();
            EditIf()->addMaster(LB->lb_contlib, sel);
            delete [] sel;
        }
    }
}


// Static function.
// Return a sorted stringlist of the library files found along the
// search path, for the Open pop-up.
//
stringlist *
sLB::lb_pathlibs()
{
    stringlist *sl = 0;
    const char *path = FIO()->PGetPath();
    if (pathlist::is_empty_path(path))
        path = ".";
    pathgen pg(path);
    char *p;
    while ((p = pg.nextpath(false)) != 0) {
        sl = lb_add_dir(p, sl);
        delete [] p;
    }
    if (sl) {
        // sl is in reverse directory search order.
        stringlist *s0 = 0;
        while (sl) {
            stringlist *sx = sl;
            sl = sl->next;
            sx->next = s0;
            s0 = sx;
        }
        sl = s0;
    }
    return (sl);
}


// Static function.
// Add the library files found in dir to sl.
// Note: to minimize grinding, library files are assumed to have a ".lib"
// suffix.
//
stringlist *
sLB::lb_add_dir(char *dir, stringlist *sl)
{
    DIR *wdir;
    if (!(wdir = opendir(dir)))
        return (sl);
    struct direct *de;
    while ((de = readdir(wdir)) != 0) {
        char *s = strrchr(de->d_name, '.');
        if (!s || strcmp(s, ".lib"))
            continue;
        if (!strcmp(de->d_name, XM()->DeviceLibName()))
            continue;
        char *fn = pathlist::mk_path(dir, de->d_name);
        char *fname = pathlist::expand_path(fn, true, true);
        delete [] fn;
        if (filestat::is_readable(fname)) {
            FILE *fp = fopen(fname, "r");
            if (!fp) {
                delete [] fname;
                continue;
            }
            bool islib = FIO()->IsLibrary(fp);
            fclose(fp);
            if (!islib) {
                delete [] fname;
                continue;
            }
            sl = new stringlist(lstring::copy(fname), sl);
        }
        delete [] fname;
    }
    closedir(wdir);
    return (sl);
}


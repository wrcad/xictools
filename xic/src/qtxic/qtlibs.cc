
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

#include "config.h"
#include "qtlibs.h"
#include "cvrt.h"
#include "editif.h"
#include "dsp_inlines.h"
#include "fio.h"
#include "fio_library.h"
#include "events.h"
#include "errorlog.h"
#include "qtinterf/qtlist.h"
#include "qtinterf/qtmcol.h"
#include "qtinterf/qtfont.h"
#include "miscutil/filestat.h"
#include "miscutil/pathlist.h"
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

#include <QLayout>
#include <QPushButton>
#include <QTreeWidget>
#include <QPixmap>
#include <QHeaderView>


//----------------------------------------------------------------------
//  Libraries Popup
//
// Help system keywords used:
//  libspanel

// Static function.
//
char *
QTmainwin::get_lib_selection()
{
    if (cLibs::self())
        return (cLibs::self()->get_selection());
    return (0);
}


// Static function.
// Called on crash to prevent updates.
//
void
QTmainwin::libs_panic()
{
    cLibs::set_panic();
}
// End of QTmainwin functions.


// Pop up a listing of the libraries found in the search path.
// The popup contains the following buttons:
//  Open/Close:   Open or close the selected library.
//  Contents:     Pop up a listing of the selected library contents.
//
void
cConvert::PopUpLibraries(GRobject caller, ShowMode mode)
{
    if (!QTdev::exists() || !QTmainwin::exists())
        return;
    if (mode == MODE_OFF) {
        if (cLibs::self())
            cLibs::self()->deleteLater();
        return;
    }
    if (mode == MODE_UPD) {
        if (cLibs::self())
            cLibs::self()->update();
        return;
    }
    if (cLibs::self())
        return;

    new cLibs(caller);

    QTdev::self()->SetPopupLocation(GRloc(LW_UL), cLibs::self(),
        QTmainwin::self()->Viewport());
    cLibs::self()->show();
}
// End of cConvert functions.


// Contests pop-up buttons.
#define OPEN_BTN "Open"
#define PLACE_BTN "Place"

const char *cLibs::nolibmsg = "There are no libraries found.";
cLibs *cLibs::instPtr;

cLibs::cLibs(GRobject c)
{
    instPtr = this;
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

    setWindowTitle(tr("Libraries"));
    setAttribute(Qt::WA_DeleteOnClose);
    setAttribute(Qt::WA_ShowWithoutActivating);

    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setMargin(2);
    vbox->setSpacing(2);

    QHBoxLayout *hbox = new QHBoxLayout(0);
    hbox->setMargin(0);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    lb_openbtn = new QPushButton(tr("Open/Close"));
    hbox->addWidget(lb_openbtn);
    connect(lb_openbtn, SIGNAL(clicked()), this, SLOT(open_btn_slot()));

    lb_contbtn = new QPushButton(tr("Contents"));
    hbox->addWidget(lb_contbtn);
    connect(lb_openbtn, SIGNAL(clicked()), this, SLOT(cont_btn_slot()));

    QPushButton *btn = new QPushButton(tr("Help"));
    hbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(help_btn_slot()));

    // scrolled list
    //
    lb_list = new QTreeWidget();
    vbox->addWidget(lb_list);
    lb_list->setHeaderLabels(QStringList(QList<QString>() << tr("Open?") <<
        tr("Libraries in search path, click to select")));
    lb_list->header()->setMinimumSectionSize(25);
    lb_list->header()->resizeSection(0, 50);

    connect(lb_list,
        SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)),
        this,
        SLOT(current_item_changed_slot(QTreeWidgetItem*, QTreeWidgetItem*)));
    connect(lb_list, SIGNAL(itemActivated(QTreeWidgetItem*, int)),
        this, SLOT(item_activated_slot(QTreeWidgetItem*, int)));
    connect(lb_list, SIGNAL(itemClicked(QTreeWidgetItem*, int)),
        this, SLOT(item_clicked_slot(QTreeWidgetItem*, int)));
    connect(lb_list, SIGNAL(itemSelectionChanged()),
        this, SLOT(item_selection_changed()));
//    g_signal_connect(G_OBJECT(lb_list), "button-press-event",
//        G_CALLBACK(lb_button_press_proc), this);


    // Set up font and tracking.
//    GTKfont::setupFont(lb_list, FNT_PROP, true);

    // dismiss button line
    //
    hbox = new QHBoxLayout(0);
    hbox->setMargin(0);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    lb_noovr = new QPushButton(tr("No Overwrite Lib Cells"));
    lb_noovr->setCheckable(true);
    hbox->addWidget(lb_noovr);
    connect(lb_noovr, SIGNAL(toggled(bool)),
        this, SLOT(noovr_btn_slot(bool)));

    btn = new QPushButton(tr("Dismiss"));
    hbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(dismiss_btn_slot()));

    // Create pixmaps.
    lb_open_pb = new QPixmap(wb_open_folder_xpm);
    lb_close_pb = new QPixmap(wb_closed_folder_xpm);

    update();
}


cLibs::~cLibs()
{
    instPtr = 0;
    delete [] lb_selection;
    delete [] lb_contlib;

    if (lb_caller)
        QTdev::Deselect(lb_caller);
    if (lb_content_pop)
        lb_content_pop->popdown();
    if (lb_open_pb)
        delete lb_open_pb;
    if (lb_close_pb)
        delete lb_close_pb;
}


QSize
cLibs::sizeHint() const
{
    return (QSize(450, 200));
}


char *
cLibs::get_selection()
{
    if (lb_contlib && lb_content_pop) {
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
cLibs::update()
{
    QTdev::SetStatus(lb_noovr,
        CDvdb()->getVariable(VA_NoOverwriteLibCells));

    lb_list->clear();
    stringlist *liblist = lb_pathlibs();
    if (liblist) {
        for (stringlist *l = liblist; l; l = l->next) {
            QTreeWidgetItem *item = new QTreeWidgetItem(lb_list);
            item->setText(1, l->string);
            if (FIO()->FindLibrary(l->string))
                item->setIcon(0, *lb_open_pb);
            else
                item->setIcon(0, *lb_close_pb);
        }
        stringlist::destroy(liblist);
    }
    else {
        QTreeWidgetItem *item = new QTreeWidgetItem(lb_list);
        item->setText(1, nolibmsg);
    }

    char *oldsel = lb_selection;
    lb_selection = 0;
    lb_openbtn->setEnabled(false);
    lb_contbtn->setEnabled(false);

    if (lb_content_pop) {
        if (lb_contlib && !FIO()->FindLibrary(lb_contlib))
            lb_content_pop->update(0, "No library selected");
        if (DSP()->MainWdesc()->DbType() == WDchd)
            lb_content_pop->set_button_sens(0);
        else
            lb_content_pop->set_button_sens(-1);
    }
    if (oldsel) {
        /*
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
        */
    }
    delete [] oldsel;
}


// Pop up a listing of the contents of the selected library.
//
void
cLibs::pop_up_contents()
{
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
        stringlist::destroy(list);
    }
}


// Static function.
// Callback for a content window.
//
void
cLibs::lb_content_cb(const char *cellname, void*)
{
    if (!instPtr)
        return;
    if (!instPtr->lb_contlib || !instPtr->lb_content_pop)
        return;
    if (!cellname)
        return;
    if (*cellname != '/')
        return;

    // Callback from button press.
    cellname++;
    if (!strcmp(cellname, OPEN_BTN)) {
        char *sel = instPtr->lb_content_pop->get_selection();
        if (sel) {
            EV()->InitCallback();
            XM()->EditCell(instPtr->lb_contlib, false, FIO()->DefReadPrms(),
                sel);
            delete [] sel;
        }
    }
    else if (!strcmp(cellname, PLACE_BTN)) {
        char *sel = instPtr->lb_content_pop->get_selection();
        if (sel) {
            EV()->InitCallback();
            EditIf()->addMaster(instPtr->lb_contlib, sel);
            delete [] sel;
        }
    }
}


// Static function.
// Return a sorted stringlist of the library files found along the
// search path, for the Open pop-up.
//
stringlist *
cLibs::lb_pathlibs()
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
cLibs::lb_add_dir(char *dir, stringlist *sl)
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


void
cLibs::open_btn_slot()
{
    if (lb_selection) {
        char *tmp = lstring::copy(lb_selection);
        if (FIO()->FindLibrary(tmp)) {
            FIO()->CloseLibrary(tmp, LIBuser);
        }
        else {
            if (!FIO()->OpenLibrary(0, tmp))
                Log()->PopUpErr(Errs()->get_error());
        }
        // These call update.
        delete [] tmp;
    }
}


void
cLibs::cont_btn_slot()
{
    pop_up_contents();
}


void
cLibs::help_btn_slot()
{
    DSPmainWbag(PopUpHelp("libspanel"))
}


void
cLibs::current_item_changed_slot(QTreeWidgetItem*, QTreeWidgetItem*)
{
}


void
cLibs::item_activated_slot(QTreeWidgetItem*, int)
{
}


void
cLibs::item_clicked_slot(QTreeWidgetItem *item, int col)
{
    // Toggle closed/open by clicking on the icon in the selected row.
    //
    if (col == 0) {
        if (lb_selection) {
            char *tmp = lstring::copy(lb_selection);
            if (FIO()->FindLibrary(tmp))
                FIO()->CloseLibrary(tmp, LIBuser);
            else
                FIO()->OpenLibrary(0, tmp);
            // These call update.
            delete [] tmp;
        }
    }
    /*
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

    */
}


void
cLibs::item_selection_changed()
{
    /*
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
    */
}


void
cLibs::noovr_btn_slot(bool state)
{
    if (state)
        CDvdb()->setVariable(VA_NoOverwriteLibCells, "");
    else
        CDvdb()->clearVariable(VA_NoOverwriteLibCells);
}


void
cLibs::dismiss_btn_slot()
{
    Cvt()->PopUpLibraries(0, MODE_OFF);
}


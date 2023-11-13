
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

#include "qtoalibs.h"
#include "oa_if.h"
#include "pcell.h"
#include "pcell_params.h"
#include "editif.h"
#include "events.h"
#include "errorlog.h"
#include "dsp_inlines.h"
#include "qtinterf/qtmcol.h"
#include "qtinterf/qtfont.h"
#include "qtinterf/qtinput.h"

#include <QLayout>
#include <QPushButton>
#include <QRadioButton>
#include <QTreeWidget>
#include <QHeaderView>
#include <QLabel>


//----------------------------------------------------------------------
//  OpenAccess Libraries Popup
//
// Help system keywords used:
//  xic:oalib

// Pop up a listing of the libraries which are currently visible
// through the OpenAccess interface.
//
// The popup contains the following buttons:
//  Open/Close:   Toggle the open/closed status of selected library.
//  Contents:     Pop up a listing of the selected library contents.
//  Help:         Pop Up help.
//
void
cOAif::PopUpOAlibraries(GRobject caller, ShowMode mode)
{
    if (!QTdev::exists() || !QTmainwin::exists())
        return;
    if (mode == MODE_OFF) {
        if (QToaLibsDlg::self())
            QToaLibsDlg::self()->deleteLater();
        return;
    }
    if (mode == MODE_UPD) {
        if (QToaLibsDlg::self())
            QToaLibsDlg::self()->update();
        return;
    }
    if (QToaLibsDlg::self())
        return;

    new QToaLibsDlg(caller);

    QToaLibsDlg::self()->set_transient_for(QTmainwin::self());
    QTdev::self()->SetPopupLocation(GRloc(LW_UL), QToaLibsDlg::self(),
        QTmainwin::self()->Viewport());
    QToaLibsDlg::self()->show();
}


void
cOAif::GetSelection(const char **plibname, const char **pcellname)
{
    if (plibname)
        *plibname = 0;
    if (pcellname)
        *pcellname = 0;
    if (QToaLibsDlg::self())
        QToaLibsDlg::self()->get_selection(plibname, pcellname);
}
// End of cOAif functions.


const char *QToaLibsDlg::nolibmsg = "There are no open libraries.";
QToaLibsDlg *QToaLibsDlg::instPtr;

// Contents pop-up buttons.
#define OPEN_BTN "Open"
#define PLACE_BTN "Place"


QToaLibsDlg::QToaLibsDlg(GRobject c) : QTbag(this)
{
    instPtr = this;
    lb_caller = c;
    lb_openbtn = 0;
    lb_writbtn = 0;
    lb_contbtn = 0;
    lb_techbtn = 0;
    lb_destbtn = 0;
    lb_both = 0;
    lb_phys = 0;
    lb_elec = 0;
    lb_list = 0;
    lb_content_pop = 0;
    lb_selection = 0;
    lb_contlib = 0;
    lb_open_pm = 0;
    lb_close_pm = 0;
    lb_tempstr = 0;

    setWindowTitle(tr("OpenAccess Libraries"));
    setAttribute(Qt::WA_DeleteOnClose);

    QMargins qmtop(2, 2, 2, 2);
    QMargins qm;
    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(qmtop);
    vbox->setSpacing(2);

    QHBoxLayout *hbox = new QHBoxLayout(0);
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    lb_openbtn = new QPushButton(tr("Open/Close"));
    hbox->addWidget(lb_openbtn);
    lb_openbtn->setAutoDefault(false);
    connect(lb_openbtn, SIGNAL(clicked()), this, SLOT(open_btn_slot()));

    lb_writbtn = new QPushButton(tr("Writable Y/N"));
    hbox->addWidget(lb_writbtn);
    lb_writbtn->setAutoDefault(false);
    connect(lb_writbtn, SIGNAL(clicked()), this, SLOT(write_btn_slot()));

    lb_contbtn = new QPushButton(tr("Contents"));
    hbox->addWidget(lb_contbtn);
    lb_contbtn->setAutoDefault(false);
    connect(lb_contbtn, SIGNAL(clicked()), this, SLOT(cont_btn_slot()));

    QPushButton *btn = new QPushButton(tr("Create"));
    hbox->addWidget(btn);
    btn->setAutoDefault(false);
    connect(btn, SIGNAL(clicked()), this, SLOT(create_btn_slot()));

    lb_defsbtn = new QPushButton(tr("Defaults"));
    hbox->addWidget(lb_defsbtn);
    lb_defsbtn->setCheckable(true);
    lb_defsbtn->setAutoDefault(false);
    connect(lb_defsbtn, SIGNAL(toggled(bool)),
        this, SLOT(defs_btn_slot(bool)));

    btn = new QPushButton(tr("Help"));
    hbox->addWidget(btn);
    btn->setAutoDefault(false);
    connect(btn, SIGNAL(clicked()), this, SLOT(help_btn_slot()));

    hbox = new QHBoxLayout(0);
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    lb_techbtn = new QPushButton(tr("Tech"));
    hbox->addWidget(lb_techbtn);
    lb_techbtn->setCheckable(true);
    lb_techbtn->setAutoDefault(false);
    connect(lb_techbtn, SIGNAL(toggled(bool)),
        this, SLOT(tech_btn_slot(bool)));

    lb_destbtn = new QPushButton(tr("Destroy"));
    hbox->addWidget(lb_destbtn);
    lb_destbtn->setAutoDefault(false);
    connect(lb_destbtn, SIGNAL(clicked()), this, SLOT(dest_btn_slot()));

    sLstr lstr;
    lstr.add("Using OpenAccess ");
    lstr.add(OAif()->version());
    QLabel *label = new QLabel(lstr.string());
    label->setAlignment(Qt::AlignCenter);
    hbox->addWidget(label);

    // read mode radio group
    //
    hbox = new QHBoxLayout(0);
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    label = new QLabel(tr("Data to use from OA: "));
    hbox->addWidget(label);

    lb_both = new QRadioButton(tr("All"));
    hbox->addWidget(lb_both);
    connect(lb_both, SIGNAL(toggled(bool)),
        this, SLOT(both_btn_slot(bool)));

    lb_phys = new QRadioButton(tr("Physical"));
    hbox->addWidget(lb_phys);
    connect(lb_phys, SIGNAL(toggled(bool)),
        this, SLOT(phys_btn_slot(bool)));

    lb_elec = new QRadioButton(tr("Electrical"));
    hbox->addWidget(lb_elec);
    connect(lb_elec, SIGNAL(toggled(bool)),
        this, SLOT(elec_btn_slot(bool)));

    // scrolled list
    //
    lb_list = new QTreeWidget();
    vbox->addWidget(lb_list);
    lb_list->setHeaderLabels(QStringList(QList<QString>() << tr("Open?") <<
        tr("Write?") << tr("OpenAccess Libraries, click to select")));
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
        this, SLOT(item_selection_changed_slot()));

    QFont *fnt;
    if (FC.getFont(&fnt, FNT_PROP))
        lb_list->setFont(*fnt);
    connect(QTfont::self(), SIGNAL(fontChanged(int)),
        this, SLOT(font_changed_slot(int)), Qt::QueuedConnection);

    // dismiss button line
    //
    btn = new QPushButton(tr("Dismiss"));
    vbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(dismiss_btn_slot()));

    // Create pixmaps.
    lb_open_pm = new QPixmap(wb_open_folder_xpm);
    lb_close_pm = new QPixmap(wb_closed_folder_xpm);

    update();
}


QToaLibsDlg::~QToaLibsDlg()
{
    instPtr = 0;
    OAif()->PopUpOAdefs(0, MODE_OFF, 0, 0);
    OAif()->PopUpOAtech(0, MODE_OFF, 0, 0);
    delete [] lb_selection;
    delete [] lb_contlib;
    delete [] lb_tempstr;

    if (lb_caller)
        QTdev::Deselect(lb_caller);
    if (lb_content_pop)
        lb_content_pop->popdown();
    delete lb_open_pm;
    delete lb_close_pm;
}


QSize
QToaLibsDlg::sizeHint() const
{
    return (QSize(450, 200));
}


// For export, return the current selections, if any.  If we pass null
// for the second arg, we alws get the currently selected library,
// otherwise it will be the library that contains the file selection,
// if any.
//
void
QToaLibsDlg::get_selection(const char **plibname, const char **pcellname)
{
    if (pcellname && lb_content_pop && lb_contlib) {
        char *sel = lb_content_pop->get_selection();
        if (sel) {
            if (plibname)
                *plibname = lb_contlib;
            delete [] lb_tempstr;
            lb_tempstr = sel;
            *pcellname = sel;
            return;
        }
    }
    else {
        if (plibname)
            *plibname = lb_selection;
    }
}


// Update the listing of open directories in the main pop-up.
//
void
QToaLibsDlg::update()
{
    const char *s = CDvdb()->getVariable(VA_OaUseOnly);
    if (s && ((s[0] == '1' && s[1] == 0) || s[0] == 'p' || s[0] == 'P')) {
        if (!QTdev::GetStatus(lb_phys)) {
            QTdev::SetStatus(lb_phys, true);
            QTdev::SetStatus(lb_both, false);
            QTdev::SetStatus(lb_elec, false);
        }
    }
    else if (s && ((s[0] == '2' && s[1] == 0) || s[0] == 'e' || s[0] == 'E')) {
        if (!QTdev::GetStatus(lb_elec)) {
            QTdev::SetStatus(lb_elec, true);
            QTdev::SetStatus(lb_both, false);
            QTdev::SetStatus(lb_phys, false);
        }
    }
    else {
        if (!QTdev::GetStatus(lb_both)) {
            QTdev::SetStatus(lb_both, true);
            QTdev::SetStatus(lb_phys, false);
            QTdev::SetStatus(lb_elec, false);
        }
    }

    stringlist *liblist;
    if (!OAif()->list_libraries(&liblist)) {
        Log()->ErrorLog(mh::Processing, Errs()->get_error());
        return;
    }
    if (!liblist)
        liblist = new stringlist(lstring::copy(nolibmsg), 0);
    else
        stringlist::sort(liblist);
    lb_list->clear();
    for (stringlist *l = liblist; l; l = l->next) {
        bool isopen;
        if (!OAif()->is_lib_open(l->string, &isopen)) {
            Log()->ErrorLog(mh::Processing, Errs()->get_error());
            continue;
        }
        bool branded;
        if (!OAif()->is_lib_branded(l->string, &branded)) {
            Log()->ErrorLog(mh::Processing, Errs()->get_error());
            continue;
        }
        QTreeWidgetItem *item = new QTreeWidgetItem(lb_list);
        item->setText(2, l->string);
        item->setText(1, branded ? "Y" : "N");
        if (isopen)
            item->setIcon(0, *lb_open_pm);
        else
            item->setIcon(0, *lb_close_pm);
    }
    stringlist::destroy(liblist);
    char *oldsel = lb_selection;
    lb_selection = 0;
    set_sensitive(false);

    if (lb_content_pop) {
        if (lb_contlib) {
            bool islib;
            if (!OAif()->is_library(lb_contlib, &islib))
                Log()->ErrorLog(mh::Processing, Errs()->get_error());
            if (!islib)
                lb_content_pop->update(0, "No library selected");
        }
        if (DSP()->MainWdesc()->DbType() == WDchd)
            lb_content_pop->set_button_sens(0);
        else
            lb_content_pop->set_button_sens(-1);
    }
    if (oldsel) {
        // This re-selects the previously selected library.
/*
        for (int i = 0; ; i++) {
            GtkTreePath *p = gtk_tree_path_new_from_indices(i, -1);
            if (!gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &iter, p)) {
                gtk_tree_path_free(p);
                break;
            }
            char *text;
            gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, 2, &text, -1);
            int sc = strcmp(text, oldsel);
            free(text);
            if (!sc) {
                GtkTreeSelection *sel =
                    gtk_tree_view_get_selection(GTK_TREE_VIEW(lb_list));
                gtk_tree_selection_select_path(sel, p);
                gtk_tree_path_free(p);
                break;
            }
            gtk_tree_path_free(p);
        }
*/
    }
    delete [] oldsel;

}


// Pop up a listing of the contents of the selected library.
//
void
QToaLibsDlg::pop_up_contents()
{
    if (!lb_selection)
        return;

    delete [] lb_contlib;
    lb_contlib = lstring::copy(lb_selection);
    stringlist *list;
    if (!OAif()->list_lib_cells(lb_contlib, &list)) {
        Log()->ErrorLog(mh::Processing, Errs()->get_error());
        return;
    }
    sLstr lstr;
    lstr.add("Cells found in library - click to select\n");
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


void
QToaLibsDlg::update_contents(bool upd_dir)
{
    if (!lb_content_pop)
        return;
    if (!upd_dir) {
        if (!lb_contlib)
            return;
    }
    else {
        if (!lb_selection)
            return;
        delete [] lb_contlib;
        lb_contlib = lstring::copy(lb_selection);
    }

    stringlist *list;
    if (!OAif()->list_lib_cells(lb_contlib, &list)) {
        Log()->ErrorLog(mh::Processing, Errs()->get_error());
        return;
    }
    sLstr lstr;
    lstr.add("Cells found in library - click to select\n");
    lstr.add(lb_contlib);
    lb_content_pop->update(list, lstr.string());
    stringlist::destroy(list);
}


void
QToaLibsDlg::set_sensitive(bool sens)
{
    lb_openbtn->setEnabled(sens);
    lb_writbtn->setEnabled(sens);
    lb_contbtn->setEnabled(sens);
    if (!sens) {
        lb_techbtn->setEnabled(false);
        lb_destbtn->setEnabled(false);
    }
    else {
        bool branded;
        if (!OAif()->is_lib_branded(lb_selection, &branded)) {
            Log()->ErrorLog(mh::Processing, Errs()->get_error());
            lb_techbtn->setEnabled(false);
            lb_destbtn->setEnabled(false);
        }
        else {
            lb_techbtn->setEnabled(branded);
            lb_destbtn->setEnabled(branded);
        }
    }
}


void
QToaLibsDlg::lb_lib_cb(const char *lname, void*)
{
    if (!QToaLibsDlg::self())
        return;
    char *nametok = lstring::clip_space(lname);
    if (!nametok)
        return;
    bool ret = OAif()->create_lib(nametok, 0);
    delete [] nametok;
    if (!ret) {
        Log()->ErrorLog(mh::Processing, Errs()->get_error());
        return;
    }
    if (QToaLibsDlg::self()->wb_input)
        QToaLibsDlg::self()->wb_input->popdown();
    QToaLibsDlg::self()->update();
}


void
QToaLibsDlg::lb_dest_cb(bool yes, void *arg)
{
    char *str = (char*)arg;
    if (QToaLibsDlg::self() && yes) {
        char *t = strchr(str, '\n');
        if (t)
            *t++ = 0;
        if (!OAif()->destroy(str, t, 0))
            Log()->ErrorLog(mh::Processing, Errs()->get_error());
        if (t)
            QToaLibsDlg::self()->update_contents(false);
        else
            QToaLibsDlg::self()->update();
    }
    delete [] str;
}


// Static function.
// Callback for a content window.
//
void
QToaLibsDlg::lb_content_cb(const char *cellname, void*)
{
    if (!QToaLibsDlg::self())
        return;
    if (!QToaLibsDlg::self()->lb_contlib || !QToaLibsDlg::self()->lb_content_pop)
        return;
    if (!cellname)
        return;
    if (*cellname != '/')
        return;

    // Callback from button press.
    cellname++;
    if (!strcmp(cellname, OPEN_BTN)) {
        char *sel = QToaLibsDlg::self()->lb_content_pop->get_selection();
        if (!sel)
            return;
        PCellParam *p0 = 0;
        EV()->InitCallback();

        // Clear persistent OA cells-loaded table.
        OAif()->clear_name_table();

        if (!OAif()->load_cell(QToaLibsDlg::self()->lb_contlib, sel, 0, CDMAXCALLDEPTH, true,
                0, &p0)) {
            Log()->ErrorLog(mh::Processing, Errs()->get_error());
            delete [] sel;
            return;
        }
        if (p0) {
            char *dbname = PC()->addSuperMaster(QToaLibsDlg::self()->lb_contlib, sel,
                DSP()->CurMode() == Physical ? "layout" : "schematic", p0);
            PCellParam::destroy(p0);
            if (EditIf()->hasEdit()) {
                if (!EditIf()->openPlacement(0, dbname)) {
                    Log()->ErrorLogV(mh::PCells,
                        "Failed to open sub-master:\n%s",
                        Errs()->get_error());
                }
            }
            else {
                Log()->ErrorLogV(mh::PCells,
                    "Support for PCell sub-master creation is not "
                    "available in\nthis feature set.");
            }
        }
        delete [] sel;
    }
    else if (!strcmp(cellname, PLACE_BTN)) {
        char *sel = QToaLibsDlg::self()->lb_content_pop->get_selection();
        if (!sel)
            return;
        EV()->InitCallback();
        EditIf()->addMaster(QToaLibsDlg::self()->lb_contlib, sel);
        delete [] sel;
    }
}


void
QToaLibsDlg::open_btn_slot()
{
    if (lb_selection) {
        bool isopen;
        if (!OAif()->is_lib_open(lb_selection, &isopen)) {
            Log()->ErrorLog(mh::Processing, Errs()->get_error());
            OAif()->set_lib_open(lb_selection, false);
        }
        else if (isopen)
            OAif()->set_lib_open(lb_selection, false);
        else
            OAif()->set_lib_open(lb_selection, true);
        update();
    }
}


void
QToaLibsDlg::write_btn_slot()
{
    if (lb_selection) {
        bool branded;
        if (!OAif()->is_lib_branded(lb_selection, &branded)) {
            Log()->ErrorLog(mh::Processing, Errs()->get_error());
            OAif()->brand_lib(lb_selection, false);
        }
        else if (!OAif()->brand_lib(lb_selection, !branded))
            Log()->ErrorLog(mh::Processing, Errs()->get_error());
        update();
    }
}


void
QToaLibsDlg::cont_btn_slot()
{
    pop_up_contents();
}


void
QToaLibsDlg::create_btn_slot()
{
    PopUpInput("New library name? ", "", "Create", lb_lib_cb, 0);
}


void
QToaLibsDlg::defs_btn_slot(bool state)
{
    if (state) {
        int xx, yy;
        QTdev::self()->Location(lb_defsbtn, &xx, &yy);
        OAif()->PopUpOAdefs(lb_defsbtn, MODE_ON, xx - 100, yy + 50);
    }
    else
        OAif()->PopUpOAdefs(0, MODE_OFF, 0, 0);
}


void
QToaLibsDlg::help_btn_slot()
{
    DSPmainWbag(PopUpHelp("xic:oalib"))
}


void
QToaLibsDlg::tech_btn_slot(bool state)
{
    if (state) {
        int xx, yy;
        QTdev::self()->Location(lb_techbtn, &xx, &yy);
        OAif()->PopUpOAtech(lb_techbtn, MODE_ON, xx + 100, yy + 50);
    }
    else
        OAif()->PopUpOAtech(0, MODE_OFF, 0, 0);
}


void
QToaLibsDlg::dest_btn_slot()
{
    if (!lb_selection)
        return;
    sLstr lstr;
    char *sel = 0;
    if (lb_content_pop) {
        sel = lb_content_pop->get_selection();
        if (sel && lb_contlib) {
            lstr.add(lb_contlib);
            lstr.add_c('\n');
            lstr.add(sel);
        }
        else
            return;
    }
    else
        lstr.add(lb_selection);

    sLstr tstr;
    tstr.add("Do you really want to irretrievalby\n");
    if (sel) {
        tstr.add("destroy cell ");
        tstr.add(sel);
        tstr.add(" in library ");
        tstr.add(lb_contlib);
        tstr.add_c('?');
    }
    else {
        tstr.add("destroy library ");
        tstr.add(lb_selection);
        tstr.add(" and its contents?");
    }

    PopUpAffirm(0, LW_CENTER, tstr.string(), lb_dest_cb, lstr.string_trim());
}


void
QToaLibsDlg::both_btn_slot(bool state)
{
    if (state)
        CDvdb()->clearVariable(VA_OaUseOnly);
}


void
QToaLibsDlg::phys_btn_slot(bool state)
{
    if (state)
        CDvdb()->setVariable(VA_OaUseOnly, "Physical");
}


void
QToaLibsDlg::elec_btn_slot(bool state)
{
    if (state)
        CDvdb()->setVariable(VA_OaUseOnly, "Electrical");
}


void
QToaLibsDlg::current_item_changed_slot(QTreeWidgetItem*, QTreeWidgetItem*)
{
}


void
QToaLibsDlg::item_activated_slot(QTreeWidgetItem*, int)
{
}


void
QToaLibsDlg::item_clicked_slot(QTreeWidgetItem *itm, int col)
{
    // Toggle closed/open pr writable Y/N by clicking on the icon in
    // the selected row.

    if (!lb_selection)
        return;
    QTreeWidget *w = dynamic_cast<QTreeWidget*>(sender());
    if (!w)
        return;
    QList<QTreeWidgetItem*> lst = w->selectedItems();
    if (lst.isEmpty() || lst[0] != itm)
        return;

    if (col == 0) {
        bool isopen;
        if (!OAif()->is_lib_open(lb_selection, &isopen)) {
            Log()->ErrorLog(mh::Processing, Errs()->get_error());
            OAif()->set_lib_open(lb_selection, false);
        }
        else if (isopen)
            OAif()->set_lib_open(lb_selection, false);
        else
            OAif()->set_lib_open(lb_selection, true);
        update();
    }
    else if (col == 1) {
        bool branded;
        if (!OAif()->is_lib_branded(lb_selection, &branded)) {
            Log()->ErrorLog(mh::Processing, Errs()->get_error());
            OAif()->brand_lib(lb_selection, false);
        }
        else if (!OAif()->brand_lib(lb_selection, !branded))
            Log()->ErrorLog(mh::Processing, Errs()->get_error());
        update();
    }
}


void
QToaLibsDlg::item_selection_changed_slot()
{
//XXX check this
// Selection callback for the list.  This is called when a new selection
// is made, but not when the selection disappears, which happens when the
// list is updated.

    QTreeWidget *w = dynamic_cast<QTreeWidget*>(sender());
    if (!w)
        return;
    QList<QTreeWidgetItem*> lst = w->selectedItems();
    char *text = 0;
    if (!lst.isEmpty())
        text = lstring::copy(lst[0]->text(2).toLatin1().constData());

    if (!text || !strcmp(nolibmsg, text)) {
        set_sensitive(false);
        delete [] text;
        return;
    }
    if (!lb_selection || strcmp(text, lb_selection)) {
        delete [] lb_selection;
        lb_selection = text;
        OAif()->PopUpOAtech(0, MODE_UPD, 0, 0);
        update_contents(true);
    }
    set_sensitive(true);
}


void
QToaLibsDlg::dismiss_btn_slot()
{
    OAif()->PopUpOAlibraries(0, MODE_OFF);
}


void
QToaLibsDlg::font_changed_slot(int fnum)
{
    if (fnum == FNT_PROP) {
        QFont *fnt;
        if (FC.getFont(&fnt, fnum))
            lb_list->setFont(*fnt);
        update();
    }
}


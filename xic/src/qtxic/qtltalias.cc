
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

#include "qtltalias.h"
#include "cvrt_variables.h"
#include "fio_layermap.h"
#include "qtinterf/qtfont.h"
#include <errno.h>

#include <QLayout>
#include <QPushButton>
#include <QTreeWidget>
#include <QCheckBox>


//--------------------------------------------------------------------
// Layer Alias Table Editor pop-up
//
// Help system keywords used:
//  layerchange


void
cMain::PopUpLayerAliases(GRobject caller, ShowMode mode)
{
    if (!QTdev::exists() || !QTmainwin::exists())
        return;
    if (mode == MODE_OFF) {
        if (QTlayerAliasDlg::self())
            QTlayerAliasDlg::self()->deleteLater();
        return;
    }
    if (mode == MODE_UPD) {
        if (QTlayerAliasDlg::self())
            QTlayerAliasDlg::self()->update();
        return;
    }
    if (QTlayerAliasDlg::self()) {
        if (caller && caller != QTlayerAliasDlg::self()->call_btn())
            QTdev::Deselect(caller);
        return;
    }

    new QTlayerAliasDlg(caller);

    QTdev::self()->SetPopupLocation(GRloc(), QTlayerAliasDlg::self(),
        QTmainwin::self()->Viewport());
    QTlayerAliasDlg::self()->show();
}


namespace {
    // function codes
    enum {NoCode, CancelCode, OpenCode, SaveCode, NewCode, DeleteCode,
        EditCode, DecCode, HelpCode };
}

QTlayerAliasDlg *QTlayerAliasDlg::instPtr;

QTlayerAliasDlg::QTlayerAliasDlg(GRobject c)
{
    instPtr = this;
    la_open = 0;
    la_save = 0;
    la_del = 0;
    la_new = 0;
    la_edit = 0;
    la_list = 0;
    la_decimal = 0;
    la_affirm = 0;
    la_line_edit = 0;
    la_calling_btn = c;
    la_row = -1;
    la_show_dec = false;

    setWindowTitle(tr("Layer Aliases"));
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

// XXX Translation to buttons from original menu bar.
    // _Open, <control>O, la_action_proc, OpenCode, 0
    la_open = new QPushButton(tr("Open"));
    hbox->addWidget(la_open);
    connect(la_open, SIGNAL(clicked()), this, SLOT(open_btn_slot()));

    // _Save, <control>S, la_action_proc, SaveCode, 0
    la_save = new QPushButton(tr("Save"));
    hbox->addWidget(la_save);
    connect(la_save, SIGNAL(clicked()), this, SLOT(save_btn_slot()));

    // _New, <control>N, la_action_proc, NewCode, 0
    la_new = new QPushButton(tr("New"));
    hbox->addWidget(la_new);
    connect(la_new, SIGNAL(clicked()), this, SLOT(new_btn_slot()));

    // _Delete, <control>D, la_action_proc, DeleteCode, 0
    la_del = new QPushButton(tr("Edit"));
    la_del->setEnabled(false);
    hbox->addWidget(la_del);
    connect(la_del, SIGNAL(clicked()), this, SLOT(del_btn_slot()));

    // _Edit, <control>E, la_action_proc, EditCode, 0
    la_edit = new QPushButton(tr("Edit"));
    la_edit->setEnabled(false);
    hbox->addWidget(la_edit);
    connect(la_edit, SIGNAL(clicked()), this, SLOT(edit_btn_slot()));

    // Help menu.
    QPushButton *btn = new QPushButton(tr("Help"));
    hbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(help_btn_slot()));

    // Scrolled display window.
    la_list = new QTreeWidget();
    vbox->addWidget(la_list);
    la_list->setHeaderLabels(QStringList(QList<QString>() <<
        tr("Layer Name") << tr("Alias")));

    connect(la_list,
        SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)),
        this,
        SLOT(current_item_changed_slot(QTreeWidgetItem*, QTreeWidgetItem*)));
/*
    connect(la_list, SIGNAL(itemActivated(QTreeWidgetItem*, int)),
        this, SLOT(item_activated_slot(QTreeWidgetItem*, int)));
    connect(la_list, SIGNAL(itemClicked(QTreeWidgetItem*, int)),
        this, SLOT(item_clicked_slot(QTreeWidgetItem*, int)));
    connect(la_list, SIGNAL(itemSelectionChanged()),
        this, SLOT(item_selection_changed()));
*/
//    g_signal_connect(G_OBJECT(chl_list), "button-press-event",
//        G_CALLBACK(lb_button_press_proc), this);

    QFont *fnt;
    if (FC.getFont(&fnt, FNT_PROP))
        la_list->setFont(*fnt);
    connect(QTfont::self(), SIGNAL(fontChanged(int)),
        this, SLOT(font_changed_slot(int)), Qt::QueuedConnection);

    hbox = new QHBoxLayout(0);
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    // Decimal _Form, <control>F, la_action_proc, DecCode, <CheckItem>
    la_decimal = new QCheckBox(tr("Decimal Form"));
    hbox->addWidget(la_decimal);
    connect(la_decimal, SIGNAL(stateChanged(int)),
        this, SLOT(decimal_btn_slot(int)));

    btn = new QPushButton(tr("Dismiss"));
    hbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(dismiss_btn_slot()));

    update();
}


QTlayerAliasDlg::~QTlayerAliasDlg()
{
    instPtr = 0;
    if (la_calling_btn)
        QTdev::Deselect(la_calling_btn);
}


void
QTlayerAliasDlg::update()
{
    FIOlayerAliasTab latab;
    latab.parse(CDvdb()->getVariable(VA_LayerAlias));
    char *s0 = latab.toString(la_show_dec);
    const char *str = s0;

/*XXX fixme
    // We need to deselect before clearing, so that the deselection
    // signal is generated.
    GtkTreeSelection *sel =
        gtk_tree_view_get_selection(GTK_TREE_VIEW(la_list));
    gtk_tree_selection_unselect_all(sel);
    GtkListStore *store =
        GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(la_list)));
    gtk_list_store_clear(store);

    GtkTreeIter iter;
    char *stok;
    while ((stok = lstring::gettok(&str)) != 0) {
        char *t = strchr(stok, '=');
        if (t) {
            *t++ = 0;
            gtk_list_store_append(store, &iter);
            gtk_list_store_set(store, &iter, 0, stok, 1, t, -1);
        }
        delete [] stok;
    }
    delete [] s0;
*/
}


// Static function.
ESret
QTlayerAliasDlg::str_cb(const char *str, void *arg)
{
    if (arg == (void*)OpenCode) {
        if (str && *str) {
            char buf[256];
            FILE *fp = fopen(str, "r");
            if (!fp) {
                snprintf(buf, sizeof(buf), "Failed to open file\n%s",
                    strerror(errno));
                QTlayerAliasDlg::self()->PopUpMessage(buf, true, false, true);
            }
            else {
                FIOlayerAliasTab latab;
                latab.readFile(fp);
                char *t = latab.toString(false);
                CDvdb()->setVariable(VA_LayerAlias, t);
                delete [] t;
                fclose(fp);
                QTlayerAliasDlg::self()->update();
            }
        }
        return (ESTR_DN);
    }
    if (arg == (void*)SaveCode) {
        if (str && *str) {
            char buf[256];
            FILE *fp = fopen(str, "w");
            if (!fp) {
                snprintf(buf, sizeof(buf), "Failed to open file\n%s",
                    strerror(errno));
                QTlayerAliasDlg::self()->PopUpMessage(buf, true, false, true);
            }
            else {
                FIOlayerAliasTab latab;
                latab.parse(CDvdb()->getVariable(VA_LayerAlias));
                latab.dumpFile(fp);
                fclose(fp);
                QTlayerAliasDlg::self()->PopUpMessage("Layer alias data saved.", true, false, true);
            }
        }
        return (ESTR_DN);
    }
    if (arg == (void*)NewCode) {
        if (str && *str) {
            FIOlayerAliasTab latab;
            latab.parse(CDvdb()->getVariable(VA_LayerAlias));
            latab.parse(str);
            char *t = latab.toString(false);
            CDvdb()->setVariable(VA_LayerAlias, t);
            delete [] t;
            QTlayerAliasDlg::self()->update();
        }
        return (ESTR_IGN);
    }
    if (arg == (void*)EditCode) {
        if (str && *str) {
            const char *s0 = str;
            char *s = CDl::get_layer_tok(&str);
            FIOlayerAliasTab latab;
            latab.parse(CDvdb()->getVariable(VA_LayerAlias));
            latab.remove(s);
            latab.parse(s0);
            char *t = latab.toString(false);
            CDvdb()->setVariable(VA_LayerAlias, t);
            delete [] t;
            QTlayerAliasDlg::self()->update();
        }
        return (ESTR_IGN);
    }
    return (ESTR_IGN);
}


// Static function.
void
QTlayerAliasDlg::yn_cb(bool yn, void*)
{
    if (yn && QTlayerAliasDlg::self()->la_row >= 0) {
/*
        GtkTreePath *p = gtk_tree_path_new_from_indices(QTlayerAliasDlg::self()->la_row, -1);
        GtkTreeModel *store = 
            gtk_tree_view_get_model(GTK_TREE_VIEW(QTlayerAliasDlg::self()->la_list));
        GtkTreeIter iter;
        gtk_tree_model_get_iter(store, &iter, p);
        char *text;
        gtk_tree_model_get(store, &iter, 0, &text, -1);
        gtk_tree_path_free(p);
*/
char *text = 0;

        if (text) {
            FIOlayerAliasTab latab;
            latab.parse(CDvdb()->getVariable(VA_LayerAlias));
            latab.remove(text);
            free(text);
            char *t = latab.toString(false);
            if (t && *t)
                CDvdb()->setVariable(VA_LayerAlias, t);
            else {
                CDvdb()->clearVariable(VA_LayerAlias);
                CDvdb()->clearVariable(VA_UseLayerAlias);
            }
            delete [] t;
            QTlayerAliasDlg::self()->update();
        }
    }
}


void
QTlayerAliasDlg::open_btn_slot()
{
    if (la_line_edit)
        la_line_edit->popdown();
    GRloc loc(LW_XYR, width() + 4, 0);
    la_line_edit = PopUpEditString(0, loc,
        "Enter file name to read aliases", 0,
        str_cb, (void*)OpenCode, 200, 0, false, 0);
    if (la_line_edit)
        la_line_edit->register_usrptr((void**)&la_line_edit);
}


void
QTlayerAliasDlg::save_btn_slot()
{
    if (la_line_edit)
        la_line_edit->popdown();
    GRloc loc(LW_XYR, width() + 4, 0);
    la_line_edit = PopUpEditString(0, loc,
        "Enter file name to save aliases", 0,
        str_cb, (void*)SaveCode, 200, 0, false, 0);
    if (la_line_edit)
        la_line_edit->register_usrptr((void**)&la_line_edit);
}


void
QTlayerAliasDlg::new_btn_slot()
{
    if (la_line_edit)
        la_line_edit->popdown();
    GRloc loc(LW_XYR, width() + 4, 0);
    la_line_edit = PopUpEditString(0, loc,
        "Enter layer name and alias", 0,
        str_cb, (void*)NewCode, 200, 0, false, 0);
    if (la_line_edit)
        la_line_edit->register_usrptr((void**)&la_line_edit);
}


void
QTlayerAliasDlg::del_btn_slot()
{
    if (la_affirm)
        la_affirm->popdown();
    GRloc loc(LW_XYR, width() + 4, 0);
    la_affirm = PopUpAffirm(0, loc, "Delete selected entry?",
        yn_cb, 0);
    if (la_affirm)
        la_affirm->register_usrptr((void**)&la_affirm);
}


void
QTlayerAliasDlg::edit_btn_slot()
{
    /*
    if (la_row >= 0) {
        GtkTreePath *p = gtk_tree_path_new_from_indices(la_row, -1);
        GtkTreeModel *store = 
            gtk_tree_view_get_model(GTK_TREE_VIEW(la_list));
        GtkTreeIter iter;
        gtk_tree_model_get_iter(store, &iter, p);
        char *n, *a;
        gtk_tree_model_get(store, &iter, 0, &n, 1, &a, -1);
        gtk_tree_path_free(p);

        if (n && a) {
            char buf[128];
            snprintf(buf, sizeof(buf), "%s %s", n, a);
            if (la_line_edit)
                la_line_edit->popdown();
    GRloc loc(LW_XYR, width() + 4, 0);
            la_line_edit = PopUpEditString(0, loc,
                "Enter layer name and alias", buf,
                str_cb, (void*)EditCode, 200, 0, false, 0);
            if (la_line_edit) {
                la_line_edit->register_usrptr(
                    (void**)&la_line_edit);
            }
        }
        free(n);
        free(a);
    }
    */
}


void
QTlayerAliasDlg::help_btn_slot()
{
    PopUpHelp("layerchange");
}


void
QTlayerAliasDlg::current_item_changed(QTreeWidgetItem *cur, QTreeWidgetItem*)
{
    if (!cur) {
        la_row = -1;
        if (la_del)
            la_del->setEnabled(false);
        if (la_edit)
            la_edit->setEnabled(false);
        return;
    }
        /*
    if (indices)
        la_row = *indices;
        */
    if (la_del)
        la_del->setEnabled(true);
    if (la_edit)
        la_edit->setEnabled(true);
}


void
QTlayerAliasDlg::decimal_btn_slot(int state)
{
    la_show_dec = state;
    update();
}


void
QTlayerAliasDlg::dismiss_btn_slot()
{
    XM()->PopUpLayerAliases(0, MODE_OFF);
}


void
QTlayerAliasDlg::font_changed_slot(int fnum)
{
    if (fnum == FNT_PROP) {
        QFont *fnt;
        if (FC.getFont(&fnt, fnum))
            la_list->setFont(*fnt);
        update();
    }
}


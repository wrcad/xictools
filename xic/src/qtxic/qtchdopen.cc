
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

#include "qtchdopen.h"
#include "cvrt.h"
#include "dsp_inlines.h"
#include "fio.h"
#include "fio_chd.h"
#include "fio_alias.h"
#include "qtcnmap.h"
#include "qtinterf/qtfont.h"

#include <QLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QTabWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QRadioButton>


//-----------------------------------------------------------------------------
// Pop up to obtain CHD creation info.
//
// Help system keywords used:
//  xic:chdadd

void
cConvert::PopUpChdOpen(GRobject caller, ShowMode mode,
    const char *init_idname, const char *init_str, int x, int y,
    bool(*callback)(const char*, const char*, int, void*), void *arg)
{
    if (!QTdev::exists() || !QTmainwin::exists())
        return;
    if (mode == MODE_OFF) {
        if (QTchdOpenDlg::self())
            QTchdOpenDlg::self()->deleteLater();
        return;
    }
    if (mode == MODE_UPD) {
        if (QTchdOpenDlg::self())
            QTchdOpenDlg::self()->update(init_idname, init_str);
        return;
    }
    if (QTchdOpenDlg::self())
        return;

    new QTchdOpenDlg(caller, callback, arg, init_idname, init_str);

    QTchdOpenDlg::self()->set_transient_for(QTmainwin::self());
    QTdev::self()->SetPopupLocation(GRloc(LW_XYA, x, y),
        QTchdOpenDlg::self(), QTmainwin::self()->Viewport());
    QTchdOpenDlg::self()->show();
}
// End of cConvert functions.


QTchdOpenDlg *QTchdOpenDlg::instPtr;

QTchdOpenDlg::QTchdOpenDlg(GRobject caller,
    bool(*callback)(const char*, const char*, int, void*),
    void *arg, const char *init_idname, const char *init_str)
{
    instPtr = this;
    co_caller = caller;
    co_nbook = 0;
    co_p1_text = 0;
    co_p1_info = 0;
    co_p2_text = 0;
    co_p2_mem = 0;
    co_p2_file = 0;
    co_p2_none = 0;
    co_idname = 0;
    co_apply = 0;
    co_p1_cnmap = 0;
    co_callback = callback;
    co_arg = arg;

    setWindowTitle("Open Cell Hierarchy Digest");
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

    // Label in frame plus help button.
    //
    QGroupBox *gb = new QGroupBox();
    hbox->addWidget(gb);
    QHBoxLayout *hb = new QHBoxLayout(gb);
    hb->setContentsMargins(qm);
    hb->setSpacing(2);
    QLabel *label = new QLabel( tr(
        "Enter path to layout or saved digest file"));
    hb->addWidget(label);

    QPushButton *btn = new QPushButton(tr("Help"));
    hbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(help_btn_slot()));

    // This allows user to change label text.
//    g_object_set_data(G_OBJECT(wb_shell), "label", label);

    co_nbook = new QTabWidget();
    vbox->addWidget(co_nbook);

    // Layout file page.
    //
    QWidget *page = new QWidget();
    QVBoxLayout *p_vbox = new QVBoxLayout(page);
    p_vbox->setContentsMargins(qmtop);
    p_vbox->setSpacing(2);

    co_p1_text = new QLineEdit();
    p_vbox->addWidget(co_p1_text);
    co_p1_text->setReadOnly(false);

    // drop site
/*
    GtkDestDefaults DD = (GtkDestDefaults)
        (GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_HIGHLIGHT);
    gtk_drag_dest_set(co_p1_text, DD, target_table, n_targets,
        GDK_ACTION_COPY);
    g_signal_connect_after(G_OBJECT(co_p1_text), "drag-data-received",
        G_CALLBACK(co_drag_data_received), 0);
*/

    co_p1_cnmap = new QTcnameMap(false);
    p_vbox->addWidget(co_p1_cnmap);

    QHBoxLayout *p_hbox = new QHBoxLayout();
    p_vbox->addLayout(p_hbox);
    p_hbox->setContentsMargins(qm);
    p_hbox->setSpacing(2);

    // Info options
    label = new QLabel(tr("Geometry Counts"));
    p_hbox->addWidget(label);

    co_p1_info = new QComboBox();
    p_hbox->addWidget(co_p1_info);
    co_p1_info->addItem(tr("no geometry info saved"));
    co_p1_info->addItem(tr("totals only"));
    co_p1_info->addItem(tr("per-layer counts"));
    co_p1_info->addItem(tr("per-cell counts"));
    co_p1_info->addItem(tr("per-cell and per-layer counts"));
    co_p1_info->setCurrentIndex(FIO()->CvtInfo());
    connect(co_p1_info, SIGNAL(currentIndexChanged(int)),
        this, SLOT(p1_info_slot(int)));

    co_nbook->addTab(page, tr("layout file"));

    // CHD file page.
    //
    page = new QWidget();
    p_vbox = new QVBoxLayout(page);
    p_vbox->setContentsMargins(qmtop);
    p_vbox->setSpacing(2);

    co_p2_text = new QLineEdit();
    p_vbox->addWidget(co_p2_text);
    co_p2_text->setReadOnly(false);

// Add vspace

    label = new QLabel(tr("Handle geometry records in saved CHD file:"));
    p_vbox->addWidget(label);

    co_p2_mem = new QRadioButton(tr(
        "Read geometry records into new MEMORY CGD"));
    p_vbox->addWidget(co_p2_mem);

    co_p2_file = new QRadioButton(tr(
        "Read geometry records into new FILE CGD"));
    p_vbox->addWidget(co_p2_file);

    co_p2_none = new QRadioButton(tr("Ignore geometry records"));
    p_vbox->addWidget(co_p2_none);

    switch (sCHDin::get_default_cgd_type()) {
    case CHD_CGDmemory:
        QTdev::SetStatus(co_p2_mem, true);
        QTdev::SetStatus(co_p2_file, false);
        QTdev::SetStatus(co_p2_none, false);
        break;
    case CHD_CGDfile:
        QTdev::SetStatus(co_p2_mem, false);
        QTdev::SetStatus(co_p2_file, true);
        QTdev::SetStatus(co_p2_none, false);
        break;
    case CHD_CGDnone:
        QTdev::SetStatus(co_p2_mem, false);
        QTdev::SetStatus(co_p2_file, false);
        QTdev::SetStatus(co_p2_none, true);
        break;
    }

    co_nbook->addTab(page, tr("CHD file"));

    // below pages
    //
    hbox = new QHBoxLayout();
    vbox->addLayout(hbox);
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);

    label = new QLabel(tr("Access name:"));
    hbox->addWidget(label);

    co_idname = new QLineEdit();
    co_idname->setReadOnly(false);
    hbox->addWidget(co_idname);

/*
    g_signal_connect(G_OBJECT(wb_shell), "key-press-event",
        G_CALLBACK(co_key_hdlr), 0);
    g_signal_connect(G_OBJECT(co_nbook), "switch-page",
        G_CALLBACK(co_page_proc), 0);
*/

    // Apply/Dismiss buttons
    //
    hbox = new QHBoxLayout();
    vbox->addLayout(hbox);
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);

    co_apply = new QPushButton(tr("Apply"));
    hbox->addWidget(co_apply);
    connect(co_apply, SIGNAL(clicked()), this, SLOT(apply_btn_slot()));

    btn = new QPushButton(tr("Dismiss"));
    hbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(dismiss_btn_slot()));

    update(init_idname, init_str);
}


QTchdOpenDlg::~QTchdOpenDlg()
{
    instPtr = 0;
    if (co_caller)
        QTdev::Deselect(co_caller);
}


void
QTchdOpenDlg::update(const char *init_idname, const char *init_str)
{
    if (init_str) {
        int pg = co_nbook->currentIndex();
        if (pg == 0)
            co_p1_text->setText(init_str);
        else
            co_p2_text->setText(init_str);
    }
    if (init_idname)
        co_idname->setText(init_idname);

    // Since the enum is defined elsewhere, don't assume that the
    // values are the same as the menu button order.
    switch (FIO()->CvtInfo()) {
    case cvINFOnone:
        co_p1_info->setCurrentIndex(0);
        break;
    case cvINFOtotals:
        co_p1_info->setCurrentIndex(1);
        break;
    case cvINFOpl:
        co_p1_info->setCurrentIndex(2);
        break;
    case cvINFOpc:
        co_p1_info->setCurrentIndex(3);
        break;
    case cvINFOplpc:
        co_p1_info->setCurrentIndex(4);
        break;
    }
    co_p1_cnmap->update();
}

void
QTchdOpenDlg::help_btn_slot()
{
    DSPmainWbag(PopUpHelp("xic:chdadd"))
}


void
QTchdOpenDlg::p1_info_slot(int tp)
{
    cvINFO cv;
    switch (tp) {
    case cvINFOnone:
        cv = cvINFOnone;
        break;
    case cvINFOtotals:
        cv = cvINFOtotals;
        break;
    case cvINFOpl:
        cv = cvINFOpl;
        break;
    case cvINFOpc:
        cv = cvINFOpc;
        break;
    case cvINFOplpc:
        cv = cvINFOplpc;
        break;
    default:
        return;
    }
    if (cv != FIO()->CvtInfo()) {
        FIO()->SetCvtInfo(cv);
        Cvt()->PopUpConvert(0, MODE_UPD, 0, 0, 0);
    }
}


void
QTchdOpenDlg::apply_btn_slot()
{
    // Pop down and destroy, call callback.  Note that if the callback
    // returns nonzero but not one, the caller is not deselected.
    int ret = true;
    if (co_callback) {
        QTdev::Deselect(co_apply);
        QString string;
        int m = 0;
        int pg = co_nbook->currentIndex();
        if (pg == 0)
            string = co_p1_text->text();
        else {
            string = co_p2_text->text();
            if (QTdev::GetStatus(co_p2_file))
                m = 1;
            else if (QTdev::GetStatus(co_p2_none))
                m = 2;
        }
        if (string.isNull() || string.isEmpty()) {
            PopUpMessage("No input source entered.", true);
            return;
        }
        QString idname = co_idname->text();
        const char *str = lstring::copy(string.toLatin1().constData());
        const char *idn = lstring::copy(idname.toLatin1().constData());
        ret = (*co_callback)(idn, str, m, co_arg);
        delete [] str;
        delete [] idn;
    }
    if (ret)
        deleteLater();
}


void
QTchdOpenDlg::dismiss_btn_slot()
{
    QTchdOpenDlg::self()->deleteLater();
}


#ifdef notdef


// Private static GTK signal handler.
// In single-line mode, Return is taken as an "OK" termination.
//
int
QTchdOpenDlg::co_key_hdlr(GtkWidget*, GdkEvent *ev, void*)
{
    if (Co && ev->key.keyval == GDK_KEY_Return) {
        const char *string;
        int pg = gtk_notebook_get_current_page(GTK_NOTEBOOK(Co->co_nbook));
        if (pg == 0)
            string = gtk_entry_get_text(GTK_ENTRY(Co->co_p1_text));
        else
            string = gtk_entry_get_text(GTK_ENTRY(Co->co_p2_text));
        if (string && *string) {
            Co->apply_proc(Co->co_apply);
            return (true);
        }
    }
    return (false);
}

/*
namespace {
    // Drag/drop stuff, also used in PopUpInput(), PopUpEditString()
    //
    GtkTargetEntry target_table[] = {
        { (char*)"TWOSTRING",   0, 0 },
        { (char*)"STRING",      0, 1 },
        { (char*)"text/plain",  0, 2 }
    };
    guint n_targets = sizeof(target_table) / sizeof(target_table[0]);
}
*/


// Private static GTK signal handler.
// Drag data received in editing window, grab it
//
void
QTchdOpenDlg::co_drag_data_received(GtkWidget *entry,
    GdkDragContext *context, gint, gint, GtkSelectionData *data,
    guint, guint time)
{
    if (gtk_selection_data_get_length(data) >= 0 &&
            gtk_selection_data_get_format(data) == 8 &&
            gtk_selection_data_get_data(data)) {
        char *src = (char*)gtk_selection_data_get_data(data);
        if (gtk_selection_data_get_target(data) ==
                gdk_atom_intern("TWOSTRING", true)) {
            // Drops from content lists may be in the form
            // "fname_or_chd\ncellname".  Keep the filename.
            char *t = strchr(src, '\n');
            if (t)
                *t = 0;
        }
        gtk_entry_set_text(GTK_ENTRY(entry), src);
        gtk_drag_finish(context, true, false, time);
        return;
    }
    gtk_drag_finish(context, false, false, time);
}


// Private static GTK signal handler.
// Handle page change, set focus to text entry.
//
void
QTchdOpenDlg::co_page_proc(GtkNotebook*, void*, int num, void*)
{
    if (!Co)
        return;
    if (num == 0)
        gtk_window_set_focus(GTK_WINDOW(Co->wb_shell), Co->co_p1_text);
    else
        gtk_window_set_focus(GTK_WINDOW(Co->wb_shell), Co->co_p2_text);
}

#endif

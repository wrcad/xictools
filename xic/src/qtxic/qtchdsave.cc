
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

#include "qtchdsave.h"
#include "cvrt.h"
#include "dsp_inlines.h"
#include "qtllist.h"

#include <QLayout>
#include <QGroupBox>
#include <QLabel>
#include <QCheckBox>
#include <QPushButton>
#include <QLineEdit>


//-----------------------------------------------------------------------------
// Pop up to save CHD file.
//
// Help system keywords used:
//  xic:chdsav

void
cConvert::PopUpChdSave(GRobject caller, ShowMode mode,
    const char *chdname, int x, int y,
    bool(*callback)(const char*, bool, void*), void *arg)
{
    if (!QTdev::exists() || !QTmainwin::exists())
        return;
    if (mode == MODE_OFF) {
        if (QTchdSaveDlg::self())
            QTchdSaveDlg::self()->deleteLater();
        return;
    }
    if (mode == MODE_UPD) {
        if (QTchdSaveDlg::self())
            QTchdSaveDlg::self()->update(chdname);
        return;
    }
    if (QTchdSaveDlg::self())
        return;

    new QTchdSaveDlg(caller, callback, arg, chdname);

    QTdev::self()->SetPopupLocation(GRloc(LW_XYA, x, y),
        QTchdSaveDlg::self(), QTmainwin::self()->Viewport());
    QTchdSaveDlg::self()->show();
}
// End of cConvert functions.


QTchdSaveDlg *QTchdSaveDlg::instPtr;

QTchdSaveDlg::QTchdSaveDlg(GRobject caller,
    bool(*callback)(const char*, bool, void*), void *arg, const char *chdname)
{
    instPtr = this;
    cs_caller = caller;
    cs_label = 0;
    cs_text = 0;
    cs_geom = 0;
    cs_apply = 0;
    cs_llist = 0;
    cs_callback = callback;
    cs_arg = arg;

    setWindowTitle(tr("Save Hierarchy Digest File"));
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

    // label in frame plus help btn
    //
    char buf[256];
    snprintf(buf, sizeof(buf), "Saving %s, enter pathname for file:",
        chdname ? chdname : "");
    QGroupBox *gb = new QGroupBox();
    hbox->addWidget(gb);
    QHBoxLayout *hb = new QHBoxLayout(gb);
    hb->setContentsMargins(qm);
    hb->setSpacing(2);
    cs_label = new QLabel(tr(buf));
    hb->addWidget(cs_label);

    QPushButton *btn = new QPushButton(tr("Help"));
    hbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(help_btn_slot()));

    // This allows user to change label text.
//    g_object_set_data(G_OBJECT(cs_popup), "label", cs_label);

    cs_text = new QLineEdit();
    vbox->addWidget(cs_text);
    cs_text->setReadOnly(false);
    connect(cs_text, SIGNAL(textChanged(const QString&)),
        this, SLOT(text_changed_slot(const QString&)));

    // drop site
/*
    GtkDestDefaults DD = (GtkDestDefaults)
        (GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_HIGHLIGHT);
    gtk_drag_dest_set(cs_text, DD, target_table, n_targets,
        GDK_ACTION_COPY);
    g_signal_connect_after(G_OBJECT(cs_text), "drag-data-received",
        G_CALLBACK(cs_drag_data_received), 0);
*/

    cs_geom = new QCheckBox(tr("Include geometry records in file"));
    vbox->addWidget(cs_geom);
    connect(cs_geom, SIGNAL(stateChanged(int)),
        this, SLOT(geom_btn_slot(int)));

    // Layer list
    //
    cs_llist = new QTlayerList;
    cs_llist->setEnabled(false);
    vbox->addWidget(cs_llist);

    // Apply/Dismiss buttons
    //
    hbox = new QHBoxLayout(0);
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    cs_apply = new QPushButton(tr("Apply"));
    hbox->addWidget(cs_apply);
    connect(cs_apply, SIGNAL(clicked()), this, SLOT(apply_btn_slot()));

    btn = new QPushButton(tr("Dismiss"));
    hbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(dismiss_btn_slot()));
}


QTchdSaveDlg::~QTchdSaveDlg()
{
    instPtr = 0;
    if (cs_caller)
        QTdev::Deselect(cs_caller);
}


void
QTchdSaveDlg::update(const char *chdname)
{
    if (!chdname)
        return;
    char buf[256];
    snprintf(buf, sizeof(buf), "Saving %s, enter pathname for file:",
        chdname);
    cs_label->setText(buf);
    cs_llist->update();
}


void
QTchdSaveDlg::help_btn_slot()
{
    DSPmainWbag(PopUpHelp("xic:chdsav"))
}


void
QTchdSaveDlg::text_changed_slot(const QString&)
{
}


void
QTchdSaveDlg::geom_btn_slot(int state)
{
    cs_llist->setEnabled(state);
}


void
QTchdSaveDlg::apply_btn_slot()
{
    int ret = true;
    if (cs_callback) {
        const char *string =
            lstring::copy(cs_text->text().toLatin1().constData());
        ret = (*cs_callback)(string, QTdev::GetStatus(cs_geom),
            cs_arg);
        delete [] string;
    }
    if (ret)
        QTchdSaveDlg::self()->deleteLater();
}


void
QTchdSaveDlg::dismiss_btn_slot()
{
    Cvt()->PopUpChdSave(0, MODE_OFF, 0, 0, 0, 0, 0);
}

#ifdef notdef

// Private static GTK signal handler.
// The entry widget has the focus initially, and the Enter key is ignored.
// After the user types, the Enter key will call the Apply method.
//
void
QTchdSaveDlg::cs_change_proc(GtkWidget*, void*)
{
    if (Cs) {
        g_signal_connect(G_OBJECT(Cs->cs_popup), "key-press-event",
            G_CALLBACK(cs_key_hdlr), 0);
        g_signal_handlers_disconnect_by_func(G_OBJECT(Cs->cs_text),
            (gpointer)cs_change_proc, 0);
    }
}


// Private static GTK signal handler.
// In single-line mode, Return is taken as an "OK" termination.
//
int
QTchdSaveDlg::cs_key_hdlr(GtkWidget*, GdkEvent *ev, void*)
{
    if (Cs && ev->key.keyval == GDK_KEY_Return) {
        Cs->button_hdlr(Cs->cs_apply);
        return (true);
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
QTchdSaveDlg::cs_drag_data_received(GtkWidget *entry,
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
#endif


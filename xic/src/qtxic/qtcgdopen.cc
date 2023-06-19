
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

#include "qtcgdopen.h"
#include "cvrt.h"
#include "dsp_inlines.h"
#include "fio.h"
#include "qtllist.h"
#include "qtcnmap.h"

#include <QLayout>
#include <QLabel>
#include <QGroupBox>
#include <QLineEdit>
#include <QTabWidget>
#include <QPushButton>


//-----------------------------------------------------------------------------
// Pop up to obtain CGD creation info.
//
// Help system keywords used:
//  xic:cgdopen

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


void
cConvert::PopUpCgdOpen(GRobject caller, ShowMode mode,
    const char *init_idname, const char *init_str, int x, int y,
    bool(*callback)(const char*, const char*, int, void*), void *arg)
{
    if (!QTdev::exists() || !QTmainwin::exists())
        return;
    if (mode == MODE_OFF) {
        if (cCGDopen::self())
            cCGDopen::self()->deleteLater();
        return;
    }
    if (mode == MODE_UPD) {
        if (cCGDopen::self())
            cCGDopen::self()->update(init_idname, init_str);
        return;
    }
    if (cCGDopen::self())
        return;

    new cCGDopen(caller, callback, arg, init_idname, init_str);

    /*
    int mwid;
    gtk_MonitorGeom(QTmainwin::self()->Shell(), 0, 0, &mwid, 0);
    GtkRequisition req;
    gtk_widget_get_requisition(Cgo->Shell(), &req);
    if (x + req.width > mwid)
        x = mwid - req.width;
    gtk_window_move(GTK_WINDOW(Cgo->Shell()), x, y);
    gtk_widget_show(Cgo->Shell());

    // OpenSuse-13.1 gtk-2.24.23 bug
    gtk_window_move(GTK_WINDOW(Cgo->Shell()), x, y);
    */
    cCGDopen::self()->show();
}
// End of cConvert functions.


// When are the layer list and cell name mapping entries applicable?
// 1) Neither is applied in file or remote CGD creation.
// 2) Layer mapping is applied for memory CGDs from layout files or
//    from CHD names, and from CHD files without geometry.  If a CHD
//    file has geometry, its CGD will be read verbatim.  A CGD file
//    will be read verbatim.
// 3) Cell name mapping is applied only for memory CGDs and layout
//    files.  A CHD source will use the CHD's aliasing.  A CGD file
//    will be read verbatim.

cCGDopen *cCGDopen::instPtr;

cCGDopen::cCGDopen(GRobject caller,
    bool(*callback)(const char*, const char*, int, void*), void *arg,
    const char *init_idname, const char *init_str)
{
    instPtr = this;
    cgo_caller = caller;
    cgo_nbook = 0;
    cgo_p1_entry = 0;
    cgo_p2_entry = 0;
    cgo_p3_host = 0;
    cgo_p3_port = 0;
    cgo_p3_idname = 0;
    cgo_idname = 0;
    cgo_apply = 0;
    cgo_p1_llist = 0;
    cgo_p1_cnmap = 0;
    cgo_callback = callback;
    cgo_arg = arg;

    setWindowTitle(tr("Open Cell Geometry Digest"));
    setAttribute(Qt::WA_DeleteOnClose);

    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setMargin(2);
    vbox->setSpacing(2);

    QHBoxLayout *hbox = new QHBoxLayout(0);
    hbox->setMargin(0);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    // label in frame plus help btn
    //
    QGroupBox *gb = new QGroupBox();
    hbox->addWidget(gb);
    QHBoxLayout *hb = new QHBoxLayout(gb);
    QLabel *label = new QLabel(tr(
        "Enter parameters to create new Cell Geometry Digest"));
    hb->addWidget(label);

    QPushButton *btn = new QPushButton(tr("Help"));
    hbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(help_btn_slot()));

    cgo_nbook = new QTabWidget();
    vbox->addWidget(cgo_nbook);

    // Memory page.
    //
    QWidget *page = new QWidget();
    cgo_nbook->addTab(page, tr("in memory"));

    QVBoxLayout *pvbox = new QVBoxLayout(page);
    pvbox->setMargin(2);
    pvbox->setSpacing(2);

    label = new QLabel(tr("All geometry data will be kept in memory."));
    pvbox->addWidget(label);

    label = new QLabel(tr(
        "Enter path to layout, CHD, or CGD file, or CHD name:"));
    pvbox->addWidget(label);

    cgo_p1_entry = new QLineEdit();
    cgo_p1_entry->setReadOnly(false);
    pvbox->addWidget(cgo_p1_entry);

    // Drop site.
/*
    GtkDestDefaults DD = (GtkDestDefaults)
        (GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_HIGHLIGHT);
    gtk_drag_dest_set(cgo_p1_entry, DD, target_table, n_targets,
        GDK_ACTION_COPY);
    g_signal_connect_after(G_OBJECT(cgo_p1_entry), "drag-data-received",
        G_CALLBACK(cgo_drag_data_received), 0);
*/

    label = new QLabel(tr(
        "For layout file, CHD name, or CHD file without geometry only:"));
    pvbox->addWidget(label);

    // Layer list.
    //
    cgo_p1_llist = new cLayerList();;
    pvbox->addWidget(cgo_p1_llist);

    label = new QLabel(tr("For layout file input only:"));

    // Cell name mapping.
    //
    cgo_p1_cnmap = new cCnmap(false);
    pvbox->addWidget(cgo_p1_cnmap);

    // File reference page.
    //
    page = new QWidget();
    cgo_nbook->addTab(page, tr("file reference"));

    pvbox = new QVBoxLayout(page);
    pvbox->setMargin(2);
    pvbox->setSpacing(2);

    label = new QLabel(tr("Geometry will be read from given file."));
    pvbox->addWidget(label);
    label->setMaximumHeight(80);

    label = new QLabel(tr(
        "Enter path to CGD file or CHD file containing geometry:"));
    pvbox->addWidget(label);

    cgo_p2_entry = new QLineEdit();
    pvbox->addWidget(cgo_p2_entry);
    pvbox->addStretch(0.8);


    // Drop site.
/*
    gtk_drag_dest_set(cgo_p2_entry, DD, target_table, n_targets,
        GDK_ACTION_COPY);
    g_signal_connect_after(G_OBJECT(cgo_p2_entry), "drag-data-received",
        G_CALLBACK(cgo_drag_data_received), 0);

    gtk_notebook_append_page(GTK_NOTEBOOK(cgo_nbook), tab_form, tab_label);
*/

    // Remote reference page.
    //
    page = new QWidget();
    cgo_nbook->addTab(page, tr("remote server reference"));

    pvbox = new QVBoxLayout(page);
    pvbox->setMargin(2);
    pvbox->setSpacing(2);

    label = new QLabel(tr("Geometry will be read from server on remote host."));
    pvbox->addWidget(label);

    QHBoxLayout *phbox = new QHBoxLayout();
    pvbox->addLayout(phbox);
    phbox->setMargin(0);
    phbox->setSpacing(2);

    QVBoxLayout *col1 = new QVBoxLayout();
    phbox->addLayout(col1);
    col1->setMargin(0);
    col1->setSpacing(2);

    QVBoxLayout *col2 = new QVBoxLayout();
    phbox->addLayout(col2);
    col2->setMargin(0);
    col2->setSpacing(2);

    label = new QLabel(tr("Host name:"));
    col1->addWidget(label);

    cgo_p3_host = new QLineEdit();
    cgo_p3_host->setReadOnly(false);
    col2->addWidget(cgo_p3_host);

    label = new QLabel(tr("Port number (optional):"));
    col1->addWidget(label);

    cgo_p3_port = new QLineEdit();
    cgo_p3_port->setReadOnly(false);
    col2->addWidget(cgo_p3_port);

    label = new QLabel(tr("Server CGD access name:"));
    col1->addWidget(label);

    cgo_p3_idname = new QLineEdit();
    cgo_p3_idname->setReadOnly(false);
    col2->addWidget(cgo_p3_idname);
    pvbox->addStretch(0.8);
    //
    // End of pages

    hbox = new QHBoxLayout();
    vbox->addLayout(hbox);
    hbox->setMargin(0);
    hbox->setSpacing(2);

    label = new QLabel(tr("Access name:"));
    hbox->addWidget(label);

    cgo_idname = new QLineEdit();
    cgo_idname->setReadOnly(false);
    hbox->addWidget(cgo_idname);

/*
    g_signal_connect(G_OBJECT(wb_shell), "key-press-event",
        G_CALLBACK(cgo_key_hdlr), 0);
    g_signal_connect(G_OBJECT(cgo_nbook), "switch-page",
        G_CALLBACK(cgo_page_proc), 0);
*/


    // Apply/Dismiss buttons.
    //
    hbox = new QHBoxLayout();
    vbox->addLayout(hbox);
    hbox->setMargin(0);
    hbox->setSpacing(2);

    cgo_apply = new QPushButton(tr("Apply"));
    hbox->addWidget(cgo_apply);
    connect(cgo_apply, SIGNAL(clicked()), this, SLOT(apply_btn_slot()));

    btn = new QPushButton(tr("Dismiss"));
    hbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(dismiss_btn_slot()));

    update(init_idname, init_str);
}


cCGDopen::~cCGDopen()
{
    instPtr = 0;
    delete cgo_p1_llist;
    delete cgo_p1_cnmap;
    if (cgo_caller)
        QTdev::Deselect(cgo_caller);
}


void
cCGDopen::update(const char *init_idname, const char *init_str)
{
    if (init_idname)
        cgo_idname->setText(init_idname);
    if (init_str) {
        int pg = cgo_nbook->currentIndex();
        if (pg == 0)
            cgo_p1_entry->setText(init_str);
        else if (pg == 1)
            cgo_p2_entry->setText(init_str);
        else
            load_remote_spec(init_str);
    }
    cgo_p1_llist->update();
    cgo_p1_cnmap->update();
}


// Take the remote access entries and formuate a string in the format
//   hostname[:port]/idname
// If a bad entry is encountered, return the widget pointer in the arg,
// and a null string.
//
char *
cCGDopen::encode_remote_spec(QLineEdit **bad)
{
    if (bad)
        *bad = 0;

    const char *host =
        lstring::copy(cgo_p3_host->text().toLatin1().constData());
    const char *t = host;
    char *tok = lstring::gettok(&t);
    if (!tok) {
        if (bad)
            *bad = cgo_p3_host;
        delete [] host;
        return (0);
    }
    sLstr lstr;
    lstr.add(tok);
    delete [] tok;
    delete [] host;

    const char *port =
        lstring::copy(cgo_p3_port->text().toLatin1().constData());
    t = port;
    tok = lstring::gettok(&t);
    if (tok) {
        int p;
        if (sscanf(tok, "%d", &p) == 1) {
            if (p > 0) {
                lstr.add_c(':');
                lstr.add_i(p);
            }
        }
        else {
            if (bad)
                *bad = cgo_p3_port;
            delete [] tok;
            delete [] port;
            return (0);
        }
        delete [] tok;
    }
    delete [] port;

    const char *idname =
        lstring::copy(cgo_p3_idname->text().toLatin1().constData());
    t = idname;
    tok = lstring::gettok(&t);
    if (!tok) {
        if (bad)
            *bad = cgo_p3_idname;
        delete [] idname;
        return (0);
    }
    lstr.add_c('/');
    lstr.add(tok);
    delete [] tok;
    delete [] idname;
    return (lstr.string_trim());
}


// Parse the string in the form hostname[:port]/idname as much as
// possible, filling in the entries.
//
void
cCGDopen::load_remote_spec(const char *str)
{
    if (!str)
        return;
    while (isspace(*str))
        str++;
    char *host = lstring::copy(str);
    char *t = host;
    while (*t && *t != ':' && *t != '/')
        t++;
    if (!*t) {
        cgo_p3_host->setText(host);
        cgo_p3_port->setText("");
        cgo_p3_idname->setText("");
        return;
    }
    char c = *t;
    *t++ = 0;
    cgo_p3_host->setText(host);
    char *e = t;
    while (*e && *e != '/')
        e++;
    if (*e == '/')
        e++;
    cgo_p3_idname->setText(e);
    *e = 0;
    if (c == ':')
        cgo_p3_port->setText(t);
    else
        cgo_p3_port->setText("");
}


void
cCGDopen::help_btn_slot()
{
}


void
cCGDopen::apply_btn_slot()
{
    int ret = true;
    if (cgo_callback) {
        char *string;
        int pg = cgo_nbook->currentIndex();
        if (pg == 0) {
            string = lstring::copy(cgo_p1_entry->text().toLatin1().constData());
            if (!string || !*string) {
                PopUpMessage("No input source entered.", true);
                delete [] string;
                return;
            }
        }
        else if (pg == 1) {
            string = lstring::copy(cgo_p2_entry->text().toLatin1().constData());
            if (!string || !*string) {
                PopUpMessage("No input source entered.", true);
                delete [] string;
                return;
            }
        }
        else {
            QLineEdit *bad;
            string = encode_remote_spec(&bad);
            if (bad) {
                const char *msg;
                if (bad == cgo_p3_host)
                    msg = "Host name text error.";
                else if (bad == cgo_p3_port)
                    msg = "Port nunmber text error.";
                else
                    msg = "Access name text error.";
                PopUpMessage(msg, true);
                return;
            }
        }

        char *idname = lstring::copy(cgo_idname->text().toLatin1().constData());
        ret = (*cgo_callback)(idname, string, pg, cgo_arg);
        delete [] idname;
        delete [] string;
    }
    if (ret)
        deleteLater();
}


void
cCGDopen::dismiss_btn_slot()
{
    Cvt()->PopUpCgdOpen(0, MODE_OFF, 0, 0, 0, 0, 0, 0);
}


#ifdef notdef


// Private static GTK signal handler.
// Return is taken as an Apply press, if chars have been entered.
//
int
cCGDopen::cgo_key_hdlr(GtkWidget*, GdkEvent *ev, void*)
{
    if (Cgo && ev->key.keyval == GDK_KEY_Return) {
        const char *string;
        int pg = gtk_notebook_get_current_page(GTK_NOTEBOOK(Cgo->cgo_nbook));
        if (pg == 0) {
            string = gtk_entry_get_text(GTK_ENTRY(Cgo->cgo_p1_entry));
            if (string && *string) {
                Cgo->apply_proc(Cgo->cgo_apply);
                return (true);
            }
        }
        else if (pg == 1) {
            string = gtk_entry_get_text(GTK_ENTRY(Cgo->cgo_p2_entry));
            if (string && *string) {
                Cgo->apply_proc(Cgo->cgo_apply);
                return (true);
            }
        }
        else {
            string = Cgo->encode_remote_spec(0);
            if (string && *string) {
                Cgo->apply_proc(Cgo->cgo_apply);
                delete [] string;
                return (true);
            }
        }
    }
    return (false);
}


// Private static GTK signal handler.
// Drag data received in editing window, grab it.
//
void
cCGDopen::cgo_drag_data_received(GtkWidget *entry,
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
cCGDopen::cgo_page_proc(GtkNotebook*, void*, int num, void*)
{
    if (!Cgo)
        return;
    if (num == 0)
        gtk_window_set_focus(GTK_WINDOW(Cgo->wb_shell), Cgo->cgo_p1_entry);
    else if (num == 1)
        gtk_window_set_focus(GTK_WINDOW(Cgo->wb_shell), Cgo->cgo_p2_entry);
    else
        gtk_window_set_focus(GTK_WINDOW(Cgo->wb_shell), Cgo->cgo_p3_host);
}

#endif

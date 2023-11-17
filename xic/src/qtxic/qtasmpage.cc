
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

#include "qtasm.h"
#include "cvrt.h"
#include "fio_alias.h"
#include "menu.h"
#include "qtinterf/qtdblsb.h"

#include <QLayout>
#include <QLineEdit>
#include <QCheckBox>
#include <QLabel>
#include <QGroupBox>
#include <QTreeWidget>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>


class QTasmPagePathEdit : public QLineEdit
{
public:
    QTasmPagePathEdit(QWidget *prnt = 0) : QLineEdit(prnt) { }

    void dragEnterEvent(QDragEnterEvent*);
    void dropEvent(QDropEvent*);
};


void
QTasmPagePathEdit::dragEnterEvent(QDragEnterEvent *ev)
{
    if (ev->mimeData()->hasUrls()) {
        ev->accept();
        return;
    }
    if (ev->mimeData()->hasFormat("text/plain")) {
        ev->accept();
        return;
    }
    ev->ignore();
}


void
QTasmPagePathEdit::dropEvent(QDropEvent *ev)
{
    if (ev->mimeData()->hasUrls()) {
        QByteArray ba = ev->mimeData()->data("text/plain");
        const char *str = ba.constData() + strlen("File://");
        setText(str);
        ev->accept();
        return;
    }
    if (ev->mimeData()->hasFormat("text/twostring")) {
        // Drops from content lists may be in the form
        // "fname_or_chd\ncellname".  Keep the cellname.
        char *str = lstring::copy(ev->mimeData()->data("text/plain").constData());
        char *t = strchr(str, '\n');
        if (t)
            *t = 0;
        setText(str);
        delete [] str;
        ev->accept();
        return;
    }
    if (ev->mimeData()->hasFormat("text/plain")) {
        // The default action will insert the text at the click location,
        // instead here we replace any existing text.
        QByteArray ba = ev->mimeData()->data("text/plain");
        setText(ba.constData());
        ev->accept();
        return;
    }
    ev->ignore();
}


//-----------------------------------------------------------------------------
// The QTasmPage (notebook page) class.

QTasmPage::QTasmPage(QTasmDlg *mt)
{
    pg_owner = mt;
    pg_path = 0;
    pg_layers_only = 0;
    pg_skip_layers = 0;
    pg_layer_list = 0;
    pg_layer_aliases = 0;
    pg_sb_scale = 0;
    pg_prefix = 0;
    pg_suffix = 0;
    pg_prefix_lab = 0;
    pg_suffix_lab = 0;
    pg_to_lower = 0;
    pg_to_upper = 0;
    pg_toplevels = 0;
    pg_tx = 0;
    pg_cellinfo = new tlinfo*[pg_infosize];
    for (unsigned int i = 0; i < pg_infosize; i++)
        pg_cellinfo[i] = 0;
    pg_infosize = 20;
    pg_numtlcells = 0;
    pg_curtlcell = -1;

    QMargins qmtop(2, 2, 2, 2);
    QMargins qm;
    QGridLayout *grid = new QGridLayout(this);
    grid->setContentsMargins(qmtop);
    grid->setSpacing(2);

    QGroupBox *gb = new QGroupBox();
    grid->addWidget(gb, 0, 0, 1, 3);
    QVBoxLayout *vbox = new QVBoxLayout(gb);
    vbox->setContentsMargins(qmtop);
    vbox->setSpacing(2);

    // path to source
    //
    QLabel *label = new QLabel(tr(QTasmDlg::path_to_source_string));
    label->setAlignment(Qt::AlignCenter);
    vbox->addWidget(label);

    pg_path = new QTasmPagePathEdit();
    vbox->addWidget(pg_path);
    pg_path->setReadOnly(false);
    pg_path->setAcceptDrops(true);

    // scale factor and cell name aliasing
    //
    QHBoxLayout *hbox = new QHBoxLayout();
    vbox->addLayout(hbox);
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);

    label = new QLabel(tr("Conversion Scale Factor"));
    hbox->addWidget(label);

    label = new QLabel(tr("Cell Name Change"));
    label->setAlignment(Qt::AlignCenter);
    hbox->addWidget(label);

    hbox = new QHBoxLayout();
    vbox->addLayout(hbox);
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);

    pg_sb_scale = new QTdoubleSpinBox();
    pg_sb_scale->setRange(CDSCALEMIN, CDSCALEMAX);
    pg_sb_scale->setDecimals(ASM_NUMD);
    pg_sb_scale->setValue(1.0);
    hbox->addWidget(pg_sb_scale);

    hbox->addStretch(1);

    pg_prefix_lab = new QLabel(tr("Prefix"));
    hbox->addWidget(pg_prefix_lab);

    pg_prefix = new QLineEdit();
    hbox->addWidget(pg_prefix);

    pg_suffix_lab = new QLabel(tr("Suffix"));
    hbox->addWidget(pg_suffix_lab);

    pg_suffix = new QLineEdit();
    hbox->addWidget(pg_suffix);

    pg_to_lower = new QCheckBox(tr("To Lower"));
    hbox->addWidget(pg_to_lower);

    pg_to_upper = new QCheckBox(tr("To Upper"));
    hbox->addWidget(pg_to_upper);

    // layer list and associated
    //
    label = new QLabel(tr("Layer List"));
    grid->addWidget(label, 1, 0);

    pg_layers_only = new QCheckBox(tr("Layers Only"));
    grid->addWidget(pg_layers_only, 1, 1);

    pg_skip_layers = new QCheckBox(tr("Skip Layers"));
    grid->addWidget(pg_skip_layers, 1, 2);

    pg_layer_list = new QLineEdit();
    grid->addWidget(pg_layer_list, 2, 0, 1, 3);

    label = new QLabel(tr("Layer Aliases"));
    grid->addWidget(label, 3, 0);

    pg_layer_aliases = new QLineEdit();
    grid->addWidget(pg_layer_aliases, 4, 0, 1, 3);

    // top-level cells list and transform entry
    //
    label = new QLabel(tr("Top-Level Cells"));
    grid->addWidget(label, 5, 0);

    pg_toplevels = new QTreeWidget();;
    grid->addWidget(pg_toplevels, 6, 0);
    connect(pg_toplevels, SIGNAL(itemSelectionChanged()),
        this, SLOT(toplev_selection_changed_slot()));

    QFont *fnt;
    if (FC.getFont(&fnt, FNT_PROP))
        pg_toplevels->setFont(*fnt);
    connect(QTfont::self(), SIGNAL(fontChanged(int)),
        this, SLOT(font_changed_slot(int)), Qt::QueuedConnection);

/*
    GtkWidget *swin = gtk_scrolled_window_new(0, 0);
    gtk_widget_show(swin);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin),
        GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    GtkListStore *store = gtk_list_store_new(1, G_TYPE_STRING);
    pg_toplevels = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
    gtk_widget_show(pg_toplevels);
    gtk_tree_view_set_enable_search(GTK_TREE_VIEW(pg_toplevels), false);
    GtkTreeViewColumn *tvcol = gtk_tree_view_column_new();
    gtk_tree_view_append_column(GTK_TREE_VIEW(pg_toplevels), tvcol);
    GtkCellRenderer *rnd = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(tvcol, rnd, true);
    gtk_tree_view_column_add_attribute(tvcol, rnd, "text", 0);

    GtkTreeSelection *sel =
        gtk_tree_view_get_selection(GTK_TREE_VIEW(pg_toplevels));
    gtk_tree_selection_set_select_function(sel, pg_selection_proc, this, 0);
    // TreeView bug hack, see note with handlers.   
    g_signal_connect(G_OBJECT(pg_toplevels), "focus",
        G_CALLBACK(pg_focus_proc), this);
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(swin),
        pg_toplevels);
    gtk_box_pack_start(GTK_BOX(vbox), swin, true, true, 2);
    gtk_table_attach(GTK_TABLE(table), vbox, 0, 1, 0, 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL), 2, 2);

    // drop site
    gtk_drag_dest_set(pg_toplevels, GTK_DEST_DEFAULT_ALL,
        QTasmDlg::target_table, QTasmDlg::n_targets, GDK_ACTION_COPY);
    g_signal_connect_after(G_OBJECT(pg_toplevels), "drag-data-received",
        G_CALLBACK(pg_drag_data_received), this);
*/

    pg_tx = new QTasmTf(this);
    grid->addWidget(pg_tx, 5, 1, 2, 2);

    upd_sens();
}


QTasmPage::~QTasmPage()
{
    for (unsigned int i = 0; i < pg_numtlcells; i++)
        delete pg_cellinfo[i];
    delete [] pg_cellinfo;
}


void
QTasmPage::upd_sens()
{
    const char *topc = pg_owner->top_level_cell();
    pg_tx->set_sens((pg_curtlcell >= 0), (topc && *topc));
}


void
QTasmPage::reset()
{
    pg_path->clear();
    QTdev::Deselect(pg_layers_only);
    QTdev::Deselect(pg_skip_layers);
    pg_layer_list->clear();
    pg_layer_aliases->clear();
    pg_sb_scale->setValue(1.0);
    pg_prefix->clear();
    pg_suffix->clear();
    pg_toplevels->clear();

    for (unsigned int j = 0; j < pg_numtlcells; j++) {
        delete pg_cellinfo[j];
        pg_cellinfo[j] = 0;
    }
    pg_numtlcells = 0;
    pg_curtlcell = -1;
    pg_tx->reset();
    upd_sens();
}


// Append a new instance to the "top level" cell list.
//
tlinfo *
QTasmPage::add_instance(const char *cname)
{
    pg_owner->store_tx_params();
    if (pg_numtlcells >= pg_infosize) {
        tlinfo **tmp = new tlinfo*[pg_infosize + pg_infosize];
        unsigned int i;
        for (i = 0; i < pg_infosize; i++)
            tmp[i] = pg_cellinfo[i];
        pg_infosize += pg_infosize;
        for ( ; i < pg_infosize; i++)
            tmp[i] = 0;
        delete [] pg_cellinfo;
        pg_cellinfo = tmp;
    }
    tlinfo *tl = new tlinfo(cname);
    pg_cellinfo[pg_numtlcells] = tl;
/*
    GtkListStore *store = GTK_LIST_STORE(gtk_tree_view_get_model(
        GTK_TREE_VIEW(pg_toplevels)));
    GtkTreeIter iter;
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, cname ? cname : ASM_TOPC, -1);
    pg_numtlcells++;
    pg_curtlcell = -1;
    GtkTreeSelection *sel = gtk_tree_view_get_selection(
        GTK_TREE_VIEW(pg_toplevels));
    gtk_tree_selection_select_iter(sel, &iter);
*/
    upd_sens();
    return (tl);
}


void
QTasmPage::toplev_selection_changed_slot()
{
    /*
    QTasmPage *src = static_cast<QTasmPage*>(srcp);
    if (src->pg_no_select && !issel)
        return (false);
    if (!issel) {
        src->pg_owner->store_tx_params();
        int n = gtk_tree_path_get_indices(tp)[0];
        src->pg_owner->show_tx_params(n);
    }
    else {
        src->pg_curtlcell = -1;
        src->pg_tx->reset();
        src->upd_sens();
    }
    */
}


void
QTasmPage::font_changed_slot(int fnum)
{
    if (fnum == FNT_PROP) {
        QFont *fnt;
        if (FC.getFont(&fnt, FNT_PROP))
            pg_toplevels->setFont(*fnt);
//XXX need to redraw
    }
}


#ifdef notdef

//
// Handler functions
//

// Static function.
// Handle selection change in the "top-level" cell list.
//
int
QTasmPage::pg_selection_proc(GtkTreeSelection*, GtkTreeModel*,
    GtkTreePath *tp, int issel, void *srcp)
{
    QTasmPage *src = static_cast<QTasmPage*>(srcp);
    if (src->pg_no_select && !issel)
        return (false);
    if (!issel) {
        src->pg_owner->store_tx_params();
        int n = gtk_tree_path_get_indices(tp)[0];
        src->pg_owner->show_tx_params(n);
    }
    else {
        src->pg_curtlcell = -1;
        src->pg_tx->reset();
        src->upd_sens();
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
QTasmPage::pg_focus_proc(GtkWidget*, GdkEvent*, void *srcp)
{
    QTasmPage *src = static_cast<QTasmPage*>(srcp);
    GtkTreeSelection *sel =
        gtk_tree_view_get_selection(GTK_TREE_VIEW(src->pg_toplevels));
    // If nothing selected set the flag.
    if (!gtk_tree_selection_get_selected(sel, 0, 0))
        src->pg_no_select = true;
    return (false);
}


// Static function.
// Drag data received in entry window, grab it.
//
void
QTasmPage::pg_drag_data_received(GtkWidget*, GdkDragContext *context,
    gint, gint, GtkSelectionData *data, guint, guint time, void *srcp)
{
    QTasmPage *src = static_cast<QTasmPage*>(srcp);
    if (gtk_selection_data_get_length(data) >= 0 &&
            gtk_selection_data_get_format(data) == 8 &&
            gtk_selection_data_get_data(data)) {
        char *s = (char*)gtk_selection_data_get_data(data);
        if (gtk_selection_data_get_target(data) ==
                gdk_atom_intern("TWOSTRING", true)) {
            // Drops from content lists may be in the form
            // "fname_or_chd\ncellname".  Keep the cellname.
            char *t = strchr(s, '\n');
            if (t)
                s = t+1;
        }
        src->add_instance(s);
        gtk_drag_finish(context, true, false, time);
        return;
    }
    gtk_drag_finish(context, false, false, time);
}

#endif


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
#include "fio_assemble.h"
#include "cd_strmdata.h"
#include "cvrt.h"
#include "qtcvofmt.h"
#include "qtinterf/qtlist.h"
#include "qtinterf/qtfont.h"
#include "qtinterf/qtinput.h"
#include "miscutil/filestat.h"

#include <QLayout>
#include <QMenuBar>
#include <QToolBar>
#include <QMenu>
#include <QAction>
#include <QTabWidget>
#include <QPushButton>
#include <QToolButton>
#include <QLabel>
#include <QLineEdit>
#include <QGroupBox>
#include <QDoubleSpinBox>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>


#ifdef __APPLE__
#define USE_QTOOLBAR
#endif

//-----------------------------------------------------------------------------
// Pop-up to merge layout sources into a single file.
//
// Help system keywords used:
//  xic:assem

// Exported function to pop up/down the tool.
//
void
cConvert::PopUpAssemble(GRobject caller, ShowMode mode)
{
    if (!QTdev::exists() || !QTmainwin::exists())
        return;
    if (mode == MODE_OFF) {
        if (QTasmDlg::self())
            QTasmDlg::self()->deleteLater();
        return;
    }
    if (mode == MODE_UPD) {
        if (QTasmDlg::self())
            QTasmDlg::self()->update();
        return;
    }
    if (QTasmDlg::self())
        return;

    new QTasmDlg(caller);

    QTasmDlg::self()->set_transient_for(QTmainwin::self());
    QTdev::self()->SetPopupLocation(GRloc(), QTasmDlg::self(),
        QTmainwin::self()->Viewport());
    QTasmDlg::self()->show();
}
// End of cConvert functions.


class QTasmPathEdit : public QLineEdit
{
public:
    QTasmPathEdit(QWidget *prnt = 0) : QLineEdit(prnt) { }

    void dragEnterEvent(QDragEnterEvent*);
    void dropEvent(QDropEvent*);
};


void
QTasmPathEdit::dragEnterEvent(QDragEnterEvent *ev)
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
QTasmPathEdit::dropEvent(QDropEvent *ev)
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
// The QTasmDlg (main) class.


const char *QTasmDlg::path_to_source_string =
    "Path to Source:  Layout File, CHD File, or CHD Name";
const char *QTasmDlg::path_to_new_string = "Path to New Layout File";
int QTasmDlg::asm_fmt_type = cConvert::cvGds;

QTasmDlg *QTasmDlg::instPtr;

QTasmDlg::QTasmDlg(GRobject c)
{
    instPtr = this;
    asm_caller = c;
    asm_filesel_btn = 0;
    asm_notebook = 0;
    asm_outfile = 0;
    asm_topcell = 0;
    asm_status = 0;
    asm_fsel = 0;
    asm_fmt = 0;

#define SRC_SIZE 20
    asm_sources = new QTasmPage*[SRC_SIZE];
    asm_srcsize = SRC_SIZE;
    asm_pages = 0;
    for (unsigned int i = 0; i < asm_srcsize; i++)
        asm_sources[i] = 0;
    asm_listobj = 0;
    asm_timer_id = 0;
    asm_refptr = 0;
    asm_doing_scan = false;
    asm_abort = false;

    setWindowTitle(tr("Layout File Merge Tool"));
    setAttribute(Qt::WA_DeleteOnClose);
//    gtk_window_set_resizable(GTK_WINDOW(wb_shell), false);
    QTpkg::self()->RegisterMainWbag(this);

    QMargins qmtop(2, 2, 2, 2);
    QMargins qm;
    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(qmtop);
    vbox->setSpacing(2);

    // menu bar
    //
#ifdef USE_QTOOLBAR
    QToolBar *menubar = new QToolBar();
#else
    QMenuBar *menubar = new QMenuBar();
#endif
    vbox->addWidget(menubar);

    // File menu.
    QAction *a;
#ifdef USE_QTOOLBAR
    a = menubar->addAction(tr("&File"));
    QMenu *menu = new QMenu();
    a->setMenu(menu);
    QToolButton *tb = dynamic_cast<QToolButton*>(menubar->widgetForAction(a));
    if (tb)
        tb->setPopupMode(QToolButton::InstantPopup);
#else
    QMenu *menu = menubar->addMenu(tr("&File"));
#endif
    // _File Select, <control>O, asm_action_proc, OpenCode, CheckItem>
    a = menu->addAction(tr("_File Select"));
    a->setCheckable(true);
    a->setData(OpenCode);
    a->setShortcut(QKeySequence("Ctrl+O"));
    asm_filesel_btn = a;
    // _Save, <control>S, asm_action_proc, SaveCode, 0
    a = menu->addAction(tr("&Save"));
    a->setData(SaveCode);
    a->setShortcut(QKeySequence("Ctrl+S"));
    // File/_Recall, <control>R, asm_action_proc, RecallCode, 0
    a = menu->addAction(tr("&Recall"));
    a->setData(RecallCode);
    a->setShortcut(QKeySequence("Ctrl+R"));

    menu->addSeparator();
    // _Quit, <control>Q, asm_action_proc, CancelCode, 0
    a = menu->addAction(tr("&Quit"));
    a->setData(CancelCode);
    a->setShortcut(QKeySequence("Ctrl+Q"));
    connect(menu, SIGNAL(triggered(QAction*)),
        this, SLOT(main_menu_slot(QAction*)));

    // Options menu.
#ifdef USE_QTOOLBAR
    a = menubar->addAction(tr("&Options"));
    menu = new QMenu();
    a->setMenu(menu);
    tb = dynamic_cast<QToolButton*>(menubar->widgetForAction(a));
    if (tb)
        tb->setPopupMode(QToolButton::InstantPopup);
#else
    menu = menubar->addMenu(tr("&Options"));
#endif
    // R_eset, <control>E, asm_action_proc, ResetCode, 0
    a = menu->addAction(tr("R&eset"));
    a->setData(ResetCode);
    a->setShortcut(QKeySequence("Ctrl+E"));
    // _New Source, <control>N, asm_action_proc, NewCode, 0
    a = menu->addAction(tr("&New Source"));
    a->setData(NewCode);
    a->setShortcut(QKeySequence("Ctrl+N"));
    // Remove Source, 0, asm_action_proc, DelCode, 0
    a = menu->addAction(tr("Remove Source"));
    a->setData(DelCode);
    // New _Toplevel, <control>T, asm_action_proc, NewTlCode, 0
    a = menu->addAction(tr("New &Toplevel"));
    a->setData(NewTlCode);
    a->setShortcut(QKeySequence("Ctrl+T"));
    // Remove Toplevel, 0, asm_action_proc, DelTlCode, 0
    a = menu->addAction(tr("Remove Toplevel"));
    a->setData(DelTlCode);
    connect(menu, SIGNAL(triggered(QAction*)),
        this, SLOT(main_menu_slot(QAction*)));

    // Help menu.
#ifdef USE_QTOOLBAR
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    menubar->addAction(tr("&Help"), Qt::CTRL|Qt::Key_H, this,
        SLOT(help_slot()));
#else
    a = menubar->addAction(tr("&Help"), this, SLOT(help_slot()));
    a->setShortcut(QKeySequence("Ctrl+H"));
#endif
#else
    menu = menubar->addMenu(tr("&Help"));
    // _Help, <control>H, asm_action_proc, HelpCode, 0
    a = menu->addAction(tr("&Help"), this, SLOT(help_slot()));
    a->setData(HelpCode);
    a->setShortcut(QKeySequence("Ctrl+H"));
#endif

    // notebook setup
    //
    asm_notebook = new QTabWidget();
    vbox->addWidget(asm_notebook);
    connect(asm_notebook, SIGNAL(currentChanged(int)),
        this, SLOT(tab_changed_slot(int)));


    output_page_setup();
    notebook_append();

    // button row
    //
    QHBoxLayout *hbox = new QHBoxLayout();
    vbox->addLayout(hbox);
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);


    QPushButton *btn = new QPushButton(tr("Create Layout File"));
    hbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(crlayout_btn_slot()));

    btn = new QPushButton(tr("Dismiss"));
    hbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(dismiss_btn_slot()));

    // status message line
    //
    asm_status = new QLabel();
    vbox->addWidget(asm_status);
}


QTasmDlg::~QTasmDlg()
{
    instPtr = 0;
    if (asm_caller)
        QTdev::Deselect(asm_caller);
    if (asm_sources) {
        for (unsigned int i = 0; i < asm_pages; i++)
            delete asm_sources[i];
    }
    delete [] asm_sources;
    delete asm_fmt;
}


void
QTasmDlg::update()
{
    if (asm_fmt)
        asm_fmt->update();
}


// The real work is done here.
//
bool
QTasmDlg::run()
{
    char *fname = filestat::make_temp("asb");
    FILE *fp = fopen(fname, "w");
    if (!fp) {
        PopUpErr(MODE_ON, "Error opening temporary file.", STY_NORM);
        delete [] fname;
        return (false);;
    }
    Errs()->init_error();
    if (!dump_file(fp, true)) {
        Errs()->add_error("run: error writing temporary file.");
        PopUpErr(MODE_ON, Errs()->get_error(), STY_NORM);
        fclose(fp);
        unlink(fname);
        delete [] fname;
        return (false);
    }
    fclose(fp);
    pop_up_monitor(MODE_ON, 0, ASM_INIT);
    bool ret = asm_do_run(fname);
    delete [] fname;
    return (ret);
}


// Clear all entries.
//
void
QTasmDlg::reset()
{
    asm_outfile->clear();
    asm_topcell->clear();
    for (unsigned int i = 0; i < asm_pages; i++) {
        if (i) {
            notebook_remove(1);
            continue;
        }
        asm_sources[0]->reset();
    }
    asm_pages = 1;
}


namespace {
    // Error checking for QTasmDlg::dump_file.
    //
    bool
    strchk(const char *string, const char *what, const char*)
    {
        if (!string || !*string) {
            Errs()->add_error("dump file: %s not given, operation aborted.",
                what);
            return (false);
        }
        return (true);
    }

  /*
    // Strip leading and trailing white space.
    //
    char *
    strip_sp(const char *str)
    {
        if (!str)
            return (0);
        while (isspace(*str))
            str++;
        if (!*str)
            return (0);
        char *sstr = lstring::copy(str);
        char *t = sstr + strlen(sstr) - 1;
        while (t >= sstr && isspace(*t))
            *t-- = 0;
        return (sstr);
    }
    */
}


// Dump a representation of the state of all entries to the stream.
//
bool
QTasmDlg::dump_file(FILE *fp, bool check)
{
    // save present transform entries
    store_tx_params();

    QByteArray outf_ba = asm_outfile->text().trimmed().toLatin1();
    const char *outf = outf_ba.constData();
    if (check && !strchk(outf, path_to_new_string, 0))
        return (false);
    fprintf(fp, "OutFile %s\n", outf ? outf : "");

    QByteArray tc_ba = asm_topcell->text().trimmed().toLatin1();
    const char *topc = tc_ba.constData();
    if (topc)
        fprintf(fp, "TopCell %s\n", topc);

    for (unsigned int i = 0; i < asm_pages; i++) {
        char page[32];
        snprintf(page, sizeof(page), "Source %d", i+1);
        QTasmPage *src = asm_sources[i];
        QByteArray path_ba = src->pg_path->text().trimmed().toLatin1();
        const char *str = path_ba.constData();
        if (check && !strchk(str, path_to_source_string, page))
            return (false);
        fprintf(fp, "Source %s\n", str ? str : "");

        QByteArray ll_ba = src->pg_layer_list->text().trimmed().toLatin1();
        str = ll_ba.constData();
        if (str) {
            fprintf(fp, "LayerList %s\n", str);
            delete [] str;
            if (QTdev::GetStatus(src->pg_layers_only))
                fprintf(fp, "OnlyLayers\n");
            else if (QTdev::GetStatus(src->pg_skip_layers))
                fprintf(fp, "SkipLayers\n");
        }

        QByteArray la_ba = src->pg_layer_aliases->text().trimmed().toLatin1();
        str = la_ba.constData();
        if (str)
            fprintf(fp, "LayerAliases %s\n", str);

        QByteArray scale_ba = src->pg_sb_scale->cleanText().toLatin1();
        str = scale_ba.constData();
        if (str) {
            if (fabs(atof(str) - 1.0) > 1e-12)
                fprintf(fp, "ConvertScale %s\n", str);
        }

        QByteArray pref_ba = src->pg_prefix->text().trimmed().toLatin1();
        str = pref_ba.constData();
        if (str)
            fprintf(fp, "CellNamePrefix %s\n", str);

        QByteArray sufx_ba = src->pg_suffix->text().trimmed().toLatin1();
        str = sufx_ba.constData();
        if (str)
            fprintf(fp, "CellNameSuffix %s\n", str);

        if (QTdev::GetStatus(src->pg_to_lower))
            fprintf(fp, "ToLower\n");
        if (QTdev::GetStatus(src->pg_to_upper))
            fprintf(fp, "ToUpper\n");

        for (unsigned int j = 0; j < src->pg_numtlcells; j++) {
            tlinfo *tl = src->pg_cellinfo[j];
            const char *pname = tl->placename;
            const char *cname = tl->cellname;
            if (pname) {
                if (cname)
                    fprintf(fp, "Place %s %s\n", cname, pname);
                else
                    fprintf(fp, "PlaceTop %s\n", pname);
            }
            else {
                if (cname)
                    fprintf(fp, "Place %s\n", cname);
                else
                    fprintf(fp, "PlaceTop\n");
            }
            fprintf(fp, "Translate %.4f %.4f\n",
                MICRONS(tl->x), MICRONS(tl->y));
            if (tl->angle != 0)
                fprintf(fp, "Rotate %d\n", tl->angle);
            if (fabs(tl->magn - 1.0) > 1e-12)
                fprintf(fp, "Magnify %.6f\n", tl->magn);
            if (fabs(tl->scale - 1.0) > 1e-12)
                fprintf(fp, "Scale %.6f\n", tl->scale);
            if (tl->mirror_y)
                fprintf(fp, "Reflect\n");
            if (tl->flatten)
                fprintf(fp, "Flatten\n");
            if (tl->ecf_level == ECFall)
                fprintf(fp, "NoEmpties\n");
            else if (tl->ecf_level == ECFpre)
                fprintf(fp, "NoEmpties 2\n");
            else if (tl->ecf_level == ECFpost)
                fprintf(fp, "NoEmpties 3\n");
            if (tl->use_win) {
                fprintf(fp, "Window %.4f %.4f %.4f %.4f\n",
                    MICRONS(tl->winBB.left), MICRONS(tl->winBB.bottom),
                    MICRONS(tl->winBB.right), MICRONS(tl->winBB.top));
                if (tl->clip)
                    fprintf(fp, "Clip\n");
            }
            if (tl->no_hier)
                fprintf(fp, "NoHier\n");
        }
    }
    return (true);
}


/*
inline const char *
cknull(const char *s)
{
    return (s ? s : "");
}
*/


bool
QTasmDlg::read_file(FILE *fp)
{
    reset();
    ajob_t job(0);
    if (!job.parse(fp))
        return (false);
    asm_outfile->setText(job.outfile());
    asm_topcell->setText(job.topcell());

    bool first_page = true;
    for (const asource_t *src = job.sources(); src; src = src->next_source()) {
        if (!first_page)
            notebook_append();
        first_page = false;
        asm_sources[asm_pages-1]->pg_path->setText(src->path());
        asm_sources[asm_pages-1]->pg_layer_list->setText(src->layer_list());
        QTdev::SetStatus(asm_sources[asm_pages-1]->pg_layers_only,
            src->only_layers());
        QTdev::SetStatus(asm_sources[asm_pages-1]->pg_skip_layers,
            src->skip_layers());
        asm_sources[asm_pages-1]->pg_layer_aliases->setText(
            src->layer_aliases());
        asm_sources[asm_pages-1]->pg_sb_scale->setValue(src->scale());
        asm_sources[asm_pages-1]->pg_prefix->setText(src->prefix());
        asm_sources[asm_pages-1]->pg_suffix->setText(src->suffix());
        QTdev::SetStatus(asm_sources[asm_pages-1]->pg_to_lower,
            src->to_lower());
        QTdev::SetStatus(asm_sources[asm_pages-1]->pg_to_upper,
            src->to_upper());

        for (ainst_t *inst = src->instances(); inst;
                inst = inst->next_instance()) {
            tlinfo *tl = asm_sources[asm_pages-1]->add_instance(
                inst->cellname());
            tl->placename = lstring::copy(inst->placename());
            tl->x = inst->pos_x();
            tl->y = inst->pos_y();
            tl->angle = mmRnd(inst->angle());
            tl->magn = inst->magn();
            tl->scale = inst->scale();
            tl->mirror_y = inst->reflc();
            tl->no_hier = inst->no_hier();
            tl->ecf_level = inst->ecf_level();
            tl->flatten = inst->flatten();
            tl->use_win = inst->use_win();
            tl->winBB = *inst->winBB();
            tl->clip = inst->clip();
        }
    }
    asm_sources[asm_pages-1]->upd_sens();
    return (true);
}


void
QTasmDlg::notebook_append()
{
    char buf[32];
    if (asm_pages+1 >= asm_srcsize) {
        QTasmPage **tmp = new QTasmPage*[asm_srcsize + asm_srcsize];
        unsigned int i;
        for (i = 0; i < asm_srcsize; i++)
            tmp[i] = asm_sources[i];
        asm_srcsize += asm_srcsize;
        for ( ; i < asm_srcsize; i++)
            tmp[i] = 0;
        delete [] asm_sources;
        asm_sources = tmp;
    }

    int index = asm_pages;
    asm_pages++;

    QTasmPage *src = new QTasmPage(this);
    asm_sources[index] = src;
    snprintf(buf, sizeof(buf), "Source %d", index+1);
    asm_notebook->insertTab(index+1, src, buf);
/*
    src->pg_tablabel = gtk_label_new(buf);
    gtk_widget_show(src->pg_tablabel);
    gtk_notebook_insert_page(GTK_NOTEBOOK(asm_notebook), src->pg_form,
        src->pg_tablabel, index + 1);
*/
}


void
QTasmDlg::notebook_remove(int index)
{
    char buf[32];
    if (index == 0 && asm_pages <= 1)
        // never delete first entry
        return;
    QTasmPage *src = asm_sources[index];
    for (unsigned int i = index; i < asm_pages-1; i++) {
        asm_sources[i] = asm_sources[i+1];
        snprintf(buf, sizeof(buf), "source %d", i+1);
//        gtk_label_set_text(GTK_LABEL(asm_sources[i]->pg_tablabel), buf);
    }
    asm_pages--;
    asm_sources[asm_pages] = 0;
/*
    delete src;
    // Have to do this after deleting src or tne signal handler removal
    // in the destructor will emit CRITICAL warnings.
    gtk_notebook_remove_page(GTK_NOTEBOOK(asm_notebook), index + 1);
*/
    asm_notebook->removeTab(index+1);
}


int
QTasmDlg::current_page_index()
{
    // Compensate for the "Output" tab.
    int ix = asm_notebook->currentIndex();
    return (ix - 1);
}


void
QTasmDlg::output_page_setup()
{
    QWidget *page = new QWidget();
    asm_notebook->insertTab(0, page, tr("Output"));
    QMargins qmtop(2, 2, 2, 2);
    QMargins qm;
    QVBoxLayout *vbox = new QVBoxLayout(page);
    vbox->setContentsMargins(qmtop);
    vbox->setSpacing(2);

    asm_fmt = new QTconvOutFmt(0, asm_fmt_type, QTconvOutFmt::cvofmt_asm);
    vbox->addWidget(asm_fmt);

    // cell name and path to archive
    //
    QHBoxLayout *hbox = new QHBoxLayout();
    vbox->addLayout(hbox);
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);

    QLabel *label = new QLabel(tr("Top-Level Cell Name"));
    hbox->addWidget(label);

    asm_topcell = new QLineEdit();
    hbox->addWidget(asm_topcell);

    QGroupBox *gb = new QGroupBox(path_to_new_string);
    vbox->addWidget(gb);
    QVBoxLayout *vb = new QVBoxLayout(gb);
    vb->setContentsMargins(qmtop);
    vb->setSpacing(2);

    asm_outfile = new QTasmPathEdit();
    vb->addWidget(asm_outfile);
    asm_outfile->setReadOnly(false);
    asm_outfile->setAcceptDrops(true);
}


// Save the visible transform parameters into the respective tlinfo
// storage.
//
void
QTasmDlg::store_tx_params()
{
    int ix = current_page_index();
    if (ix < 0)
        return;
    QTasmPage *src = asm_sources[ix];
    int n = src->pg_curtlcell;
    if (n >= 0) {
        tlinfo *tl = src->pg_cellinfo[n];
        src->pg_tx->get_tx_params(tl);
    }
}


// Display the saved transform parameters in the widgets, for cellname
// index n.  This becomes "current".
//
void
QTasmDlg::show_tx_params(unsigned int n)
{
    int ix = current_page_index();
    if (ix < 0)
        return;
    QTasmPage *src = asm_sources[ix];
    if (n < src->pg_numtlcells) {
        tlinfo *tl = src->pg_cellinfo[n];
        src->pg_tx->set_tx_params(tl);
        src->pg_curtlcell = n;
        src->upd_sens();
    }
    else {
        src->pg_tx->reset();
        src->upd_sens();
    }
}


// Export top cell name, for use in source page sensitivity test.
//
const char *
QTasmDlg::top_level_cell()
{
    return (lstring::copy(asm_topcell->text().toLatin1().constData()));
//    return (gtk_entry_get_text(GTK_ENTRY(asm_topcell)));
// copy?
}


// Static function.
void
QTasmDlg::set_status_message(const char *msg)
{
    /*
    if (Asm) {
        if (Asm->asm_timer_id)
            g_source_remove(Asm->asm_timer_id);
        Asm->asm_timer_id = g_timeout_add(10000, asm_timer_callback, 0);
        gtk_label_set_text(GTK_LABEL(Asm->asm_status), msg);
    }
    */
}


// Static function.
// Exported interface function for progress monitor.
//
void
QTasmDlg::pop_up_monitor(int mode, const char *msg, ASMcode code)
{
    if (mode == MODE_OFF) {
        if (QTasmPrgDlg::self())
            QTasmPrgDlg::self()->deleteLater();
        return;
    }
    if (mode == MODE_UPD) {
        if (instPtr && instPtr->scanning()) {
            if (code == ASM_READ) {
                char *str = lstring::copy(msg);
                char *s = str + strlen(str) - 1;
                while (s >= str && isspace(*s))
                    *s-- = 0;
                set_status_message(str);
                delete [] str;
            }
        }
        else if (QTasmPrgDlg::self())
            QTasmPrgDlg::self()->update(msg, code);
        return;
    }
    if (QTasmPrgDlg::self())
        return;

    new QTasmPrgDlg();
//    instPtr->set_refptr((void**)&instPtr);
    if (instPtr) {
        QTdev::self()->SetPopupLocation(GRloc(), QTasmPrgDlg::self(),
            instPtr);
    }
    QTasmPrgDlg::self()->show();
}


// Static function.
// Handler for the Save file name input.  Save the state to file.
//
void
QTasmDlg::asm_save_cb(const char *fname, void*)
{
    if (!instPtr)
        return;
    if (fname && *fname) {
        if (!filestat::create_bak(fname, 0)) {
            char buf[512];
            snprintf(buf, sizeof(buf),
                "Error: %s/ncould not back up existing file, save aborted.",
                Errs()->get_error());
            instPtr->PopUpErr(MODE_ON, buf, STY_NORM);
            return;
        }
        FILE *fp = fopen(fname, "w");
        if (!fp) {
    /*
            const char *msg = "Can't open file, try again";
            GtkWidget *label = (GtkWidget*)g_object_get_data(
                G_OBJECT(Asm->wb_input), "label");
            if (label)
                gtk_label_set_text(GTK_LABEL(label), msg);
            else
                set_status_message(msg);
     */
            return;
        }
        Errs()->init_error();
        if (instPtr->dump_file(fp, false)) {
            int len = strlen(fname) + 100;
            char *s = new char[len];
            snprintf(s, len, "State saved in file %s", fname);
            set_status_message(s);
            delete [] s;
            if (instPtr->wb_input)
                instPtr->wb_input->popdown();
        }
        else {
            Errs()->add_error("save_cb: operation failed.");
            instPtr->PopUpErr(MODE_ON, Errs()->get_error(), STY_NORM);
        }
        fclose(fp);
    }
}


// Static function.
// Handler for the Recall file name input.  Recall the state from the
// file.
//
void
QTasmDlg::asm_recall_cb(const char *fname, void*)
{
    if (!instPtr)
        return;
    if (fname && *fname) {
        FILE *fp = fopen(fname, "r");
        if (!fp) {
         /*
            const char *msg = "Can't open file, try again";
            GtkWidget *label = (GtkWidget*)g_object_get_data(
                G_OBJECT(Asm->wb_input), "label");
            if (label)
                gtk_label_set_text(GTK_LABEL(label), msg);
            else
                set_status_message(msg);
         */
            return;
        }
        Errs()->init_error();
        if (instPtr->read_file(fp)) {
            int len = strlen(fname) + 100;
            char *s = new char[len];
            snprintf(s, len, "State restored from file %s", fname);
            set_status_message(s);
            delete [] s;
            if (instPtr->wb_input)
                instPtr->wb_input->popdown();
        }
        else {
            Errs()->add_error("recall_cb: operation failed.");
            instPtr->PopUpErr(MODE_ON, Errs()->get_error(), STY_NORM);
        }
        fclose(fp);
    }
}


// Static function.
// Handler for the File Selection pop-up, fill the archive path entry.
//
void
QTasmDlg::asm_fsel_open(const char *path, void*)
{
    if (!instPtr)
        return;
    if (path && *path) {
        int ix = instPtr->current_page_index();
        if (ix < 0)
            return;
        QTasmPage *src = instPtr->asm_sources[ix];
        src->pg_path->setText(path);
    }
}


// Static function.
// Handle File Selection deletion.
//
void
QTasmDlg::asm_fsel_cancel(GRfilePopup*, void*)
{
    if (!instPtr)
        return;
    instPtr->asm_fsel = 0;
    QTdev::Select(instPtr->asm_filesel_btn);
}


// Static function.
// Handler for "top-level" cell name input.  Add the cell name to the
// list for the current page.
//
void
QTasmDlg::asm_tladd_cb(const char *cname, void*)
{
    if (!instPtr)
        return;
    if (cname) {
        while (isspace(*cname))
            cname++;
        if (!*cname || !strcmp(cname, ASM_TOPC))
            cname = 0;
        int ix = instPtr->current_page_index();
        if (ix < 0)
            return;
        instPtr->asm_sources[ix]->add_instance(cname);
        if (instPtr->wb_input)
            instPtr->wb_input->popdown();
    }
}


// Static function.
// Erase the status message.
//
int
QTasmDlg::asm_timer_callback(void*)
{
    if (instPtr) {
        instPtr->asm_status->clear();
        instPtr->asm_timer_id = 0;
    }
    return (0);
}


namespace {
    void
    ifInfoMessage(INFOmsgType code, const char *string, va_list args)
    {
        char buf[512];
        if (!string)
            string = "";
        vsnprintf(buf, 512, string, args);
        if (code == IFMSG_INFO)
            QTasmDlg::pop_up_monitor(MODE_UPD, buf, ASM_INFO);
        else if (code == IFMSG_RD_PGRS)
            QTasmDlg::pop_up_monitor(MODE_UPD, buf, ASM_READ);
        else if (code == IFMSG_WR_PGRS)
            QTasmDlg::pop_up_monitor(MODE_UPD, buf, ASM_WRITE);
        else if (code == IFMSG_CNAME)
            QTasmDlg::pop_up_monitor(MODE_UPD, buf, ASM_CNAME);
        else if (code == IFMSG_POP_ERR) {
            if (QTmainwin::self())
                QTmainwin::self()->PopUpErr(MODE_ON, buf);
        }
        else if (code == IFMSG_POP_WARN) {
            if (QTmainwin::self())
                QTmainwin::self()->PopUpWarn(MODE_ON, buf);
        }
        else if (code == IFMSG_POP_INFO) {
            if (QTmainwin::self())
                QTmainwin::self()->PopUpInfo(MODE_ON, buf);
        }
        else if (code == IFMSG_LOG_ERR) {
            if (QTmainwin::self())
                QTmainwin::self()->PopUpErr(MODE_ON, buf);
        }
        else if (code == IFMSG_LOG_WARN) {
            if (QTmainwin::self())
                QTmainwin::self()->PopUpWarn(MODE_ON, buf);
        }
    }
}


// Static function.
void
QTasmDlg::asm_setup_monitor(bool active)
{
    static void (*info_msg)(INFOmsgType, const char*, va_list);

    if (active)
        info_msg = FIO()->RegisterIfInfoMessage(ifInfoMessage);
    else
        info_msg = FIO()->RegisterIfInfoMessage(info_msg);
}


// Static function.
// The real work is done here.
//
bool
QTasmDlg::asm_do_run(const char *fname)
{
    QTpkg::self()->SetWorking(true);
    set_status_message("Working...");
    asm_setup_monitor(true);

    FILE *fp = fopen(fname, "r");
    if (!fp) {
        (instPtr ? (QTmainwin*)instPtr : QTmainwin::self())->PopUpErr(
            MODE_ON, "Unable to reopen temporary file.", STY_NORM);
        set_status_message("Terminated with error");
        return (false);
    }
    ajob_t *job = new ajob_t(0);
    if (!job->parse(fp)) {
        Errs()->add_error("asm_do_run: processing failed.");
        Errs()->add_error("asm_do_run: keeping temp file %s.", fname);
        (instPtr ? (QTmainwin*)instPtr : QTmainwin::self())->PopUpErr(
            MODE_ON, Errs()->get_error(), STY_NORM);
        fclose(fp);
        delete job;
        set_status_message("Terminated with error");
        QTpkg::self()->SetWorking(false);
        asm_setup_monitor(false);
        return (false);
    }
    fclose(fp);

    if (!job->run(0)) {
        if (QTasmPrgDlg::self() && QTasmPrgDlg::self()->aborted()) {
            set_status_message("Task ABORTED on user request.");
            unlink(fname);
        }
        else {
            Errs()->add_error("asm_do_run: processing failed.");
            Errs()->add_error("asm_do_run: keeping temp file %s.", fname);
            (instPtr ? (QTmainwin*)instPtr : QTmainwin::self())->PopUpErr(
                MODE_ON, Errs()->get_error(), STY_NORM);
            set_status_message("Terminated with error");
        }
        delete job;
        QTpkg::self()->SetWorking(false);
        asm_setup_monitor(false);
        return (false);
    }
    delete job;
    unlink(fname);
    QTpkg::self()->SetWorking(false);
    set_status_message("Operation completed successfully");
    asm_setup_monitor(false);
    return (true);
}


void
QTasmDlg::main_menu_slot(QAction *a)
{
    long code = a->data().toInt();
    if (code == NoCode) {
        // shouldn't receive this
    }
    else if (code == CancelCode) {
        // cancel the pop-up
        Cvt()->PopUpAssemble(0, MODE_OFF);
    }
    else if (code == OpenCode) {
        // pop up/down file selection panel
        if (a->isChecked()) {
            if (!asm_fsel) {
                asm_fsel = PopUpFileSelector(fsSEL, GRloc(LW_LR),
                    asm_fsel_open, asm_fsel_cancel, 0, 0);
                asm_fsel->register_usrptr((void**)&asm_fsel);
            }
        }
        else if (asm_fsel)
            asm_fsel->popdown();
    }
    else if (code == SaveCode) {
        // solicit for a new file to save state
        PopUpInput("Enter file name", "", "Save State", asm_save_cb, 0);
    }
    else if (code == RecallCode) {
        // solicit for a new file to read state
        PopUpInput("Enter file name", "", "Recall State", asm_recall_cb, 0);
    }
    else if (code == ResetCode) {
        // clear all entries
        reset();
    }
    else if (code == NewCode) {
        // add a source notebook page
        notebook_append();
    }
    else if (code == DelCode) {
        // delete the current notebook page
        notebook_remove(current_page_index());
    }
    else if (code == NewTlCode) {
        // solicit for a new "top-level" cell to convert
        PopUpInput("Enter cell name", "", "Add Cell", asm_tladd_cb, 0);
    }
    else if (code == DelTlCode) {
        // delete the currently selected "top-level" cell
        int ix = current_page_index();
        if (ix < 0)
            return;

        QTasmPage *src = asm_sources[ix];
        if (src->pg_numtlcells) {
            int n = src->pg_curtlcell;
            if (n >= 0) {

                // Remove the nth row.
             /*
                GtkTreeModel *store = gtk_tree_view_get_model(
                    GTK_TREE_VIEW(src->pg_toplevels));
                GtkTreePath *path = gtk_tree_path_new_from_indices(n, -1);
                GtkTreeIter iter;
                gtk_tree_model_get_iter(store, &iter, path);
                gtk_list_store_remove(GTK_LIST_STORE(store), &iter);
                gtk_tree_path_free(path);
             */

                delete src->pg_cellinfo[n];
                src->pg_numtlcells--;
                for (unsigned int i = n; i < src->pg_numtlcells; i++)
                    src->pg_cellinfo[i] = src->pg_cellinfo[i+1];
                src->pg_cellinfo[src->pg_numtlcells] = 0;
                src->pg_curtlcell = -1;
                src->upd_sens();

                // Select the top row.
              /*
                path = gtk_tree_path_new_from_indices(
                    src->pg_numtlcells-1, -1);
                GtkTreeSelection *sel = gtk_tree_view_get_selection(
                    GTK_TREE_VIEW(src->pg_toplevels));
                gtk_tree_selection_select_path(sel, path);
                gtk_tree_path_free(path);
              */
            }
        }
    }
    else if (code == HelpCode)
        PopUpHelp("xic:assem");
}


void
QTasmDlg::tab_changed_slot(int page)
{
    if (current_page_index() >= 0)
        // save present transform entries
        store_tx_params();

    if (page > 0) {
        QTasmPage *src = asm_sources[page-1];
        int n = src->pg_curtlcell;
/*
        GtkTreeSelection *sel =
            gtk_tree_view_get_selection(GTK_TREE_VIEW(src->pg_toplevels));
        if (n >= 0) {
            // Select the nth row.
            GtkTreePath *path = gtk_tree_path_new_from_indices(n, -1);
            gtk_tree_selection_select_path(sel, path);
            gtk_tree_path_free(path);
        }
        else {
            // Clear selections.
            gtk_tree_selection_unselect_all(sel);
        }
*/
        src->upd_sens();
    }
}


namespace {
    int
    run_idle(void*)
    {
        if (QTasmDlg::self())
            QTasmDlg::self()->run();
        return (0);
    }
}


void
QTasmDlg::crlayout_btn_slot()
{
    // Handle the Create Layout File button.
    QTpkg::self()->RegisterIdleProc(run_idle, 0);
}


void
QTasmDlg::dismiss_btn_slot()
{
    Cvt()->PopUpAssemble(0, MODE_OFF);
}


void
QTasmDlg::help_slot()
{
    PopUpHelp("xic:assem");
}


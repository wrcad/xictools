
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

#include "qtdrcrun.h"
#include "drc.h"
#include "fio.h"
#include "fio_chd.h"
#include "cd_digest.h"
#include "dsp_inlines.h"
#include "dsp_color.h"
#include "events.h"
#include "ghost.h"
#include "errorlog.h"
#include "promptline.h"
#include "qtinterf/qttextw.h"
#ifdef WIN32
#include "windows.h"
#endif
#include <signal.h>

#include <QLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QTabWidget>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QMouseEvent>


//------------------------------------------------------------------------
// DRC Run dialog.
//
// Allowe initiation and control of batch DRC runs.
//
// Help system keywords used:
//  xic:check

void
cDRC::PopUpDrcRun(GRobject caller, ShowMode mode)
{
    if (!QTdev::exists() || !QTmainwin::exists())
        return;
    if (mode == MODE_OFF) {
        if (QTdrcRunDlg::self())
            QTdrcRunDlg::self()->deleteLater();
        return;
    }
    if (mode == MODE_UPD) {
        if (QTdrcRunDlg::self())
            QTdrcRunDlg::self()->update();
        return;
    }
    if (QTdrcRunDlg::self())
        return;

    new QTdrcRunDlg(caller);

    QTdev::self()->SetPopupLocation(GRloc(), QTdrcRunDlg::self(),
        QTmainwin::self()->Viewport());
    QTdrcRunDlg::self()->show();
}
// End of cDRC functions.


namespace {
    struct WinState : public CmdState
    {
        WinState(const char*, const char*);
        virtual ~WinState();

        void b1down();
        void b1up();
        void esc();

        void message() { PL()->ShowPrompt(msg1p); }

    private:
        BBox AOI;
        int state;
        bool ghost_on;

        static const char *msg1p;
    };

    WinState *WinCmd;
    const char *WinState::msg1p = "Click twice or drag to set area.";
}


double QTdrcRunDlg::dc_last_part_size = DRC_PART_DEF;
double QTdrcRunDlg::dc_l_val = 0.0;
double QTdrcRunDlg::dc_b_val = 0.0;
double QTdrcRunDlg::dc_r_val = 0.0;
double QTdrcRunDlg::dc_t_val = 0.0;
bool QTdrcRunDlg::dc_use_win = false;
bool QTdrcRunDlg::dc_flatten = false;
bool QTdrcRunDlg::dc_use_chd = false;

QTdrcRunDlg *QTdrcRunDlg::instPtr;

QTdrcRunDlg::QTdrcRunDlg(GRobject c)
{
    instPtr = this;
    dc_caller = c;
    dc_use = 0;
    dc_chdname = 0;
    dc_cname = 0;
    dc_none = 0;
    dc_sb_part = 0;
    dc_wind = 0;
    dc_flat = 0;
    dc_set = 0;
    dc_l_label = 0;
    dc_b_label = 0;
    dc_r_label = 0;
    dc_t_label = 0;
    dc_sb_left = 0;
    dc_sb_bottom = 0;
    dc_sb_right = 0;
    dc_sb_top = 0;
    dc_check = 0;
    dc_checkbg = 0;
    dc_jobs = 0;
    dc_kill = 0;
    dc_start = 0;
    dc_end = 0;
    dc_line_selected = -1;

    setWindowTitle(tr("DRC Run Control"));
    setAttribute(Qt::WA_DeleteOnClose);
//    gtk_window_set_resizable(GTK_WINDOW(dc_popup), false);

    QMargins qmtop(2, 2, 2, 2);
    QMargins qm;
    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(qmtop);
    vbox->setSpacing(2);

    QHBoxLayout *hbox = new QHBoxLayout();
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    // label in frame plus help btn
    //
    QGroupBox *gb = new QGroupBox();
    hbox->addWidget(gb);
    QHBoxLayout *hb = new QHBoxLayout(gb);
    hb->setContentsMargins(qm);
    hb->setSpacing(2);
    QLabel *label = new QLabel(tr("Initiate batch DRC run"));
    hb->addWidget(label);

    QPushButton *btn = new QPushButton(tr("Help"));
    hbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(help_btn_slot()));

    QTabWidget *nbook = new QTabWidget();
    vbox->addWidget(nbook);

    // Run page
    //
    QWidget *page = new QWidget();
    nbook->addTab(page, tr("Run"));

    QVBoxLayout *vb = new QVBoxLayout(page);
    vb->setContentsMargins(qm);
    vb->setSpacing(2);
    QGridLayout *grid = new QGridLayout();
    grid->setContentsMargins(qm);
    grid->setSpacing(2);
    vb->addLayout(grid);

    // CHD name
    //
    label = new QLabel(tr("CHD reference name"));
    grid->addWidget(label, 0, 0);

    hb = new QHBoxLayout();
    grid->addLayout(hb, 0, 1);
    hb->setContentsMargins(qm);
    hb->setSpacing(2);

    dc_use = new QPushButton(tr("Use"));
    dc_use->setCheckable(true);
    hb->addWidget(dc_use);
    connect(dc_use, SIGNAL(toggled(bool)), this, SLOT(use_btn_slot(bool)));

    dc_chdname = new QLineEdit();
    hb->addWidget(dc_chdname);
    connect(dc_chdname, SIGNAL(textChanged(const QString&)),
        this, SLOT(chd_name_slot(const QString&)));

    label = new QLabel(tr("CHD top cell"));
    grid->addWidget(label, 1, 0);

    dc_cname = new QLineEdit();
    grid->addWidget(dc_cname, 1, 1);
    connect(dc_cname, SIGNAL(textChanged(const QString&)),
        this, SLOT(cname_slot(const QString&)));

    // Gridding.
    //
    label = new QLabel(tr("Partition grid size"));
    grid->addWidget(label, 2, 0);

    hb = new QHBoxLayout();
    grid->addLayout(hb, 2, 1);
    hb->setContentsMargins(qm);
    hb->setSpacing(2);

    dc_none = new QPushButton(tr("None"));
    dc_none->setCheckable(true);
    hb->addWidget(dc_none);
    connect(dc_none, SIGNAL(toggled(bool)), this, SLOT(none_btn_slot(bool)));

    dc_sb_part = new QDoubleSpinBox();
    dc_sb_part->setRange(DRC_PART_MIN, DRC_PART_MAX);
    dc_sb_part->setDecimals(2);
    dc_sb_part->setValue(DRC_PART_DEF);
    hb->addWidget(dc_sb_part);
    connect(dc_sb_part, SIGNAL(valueChanged(double)),
        this, SLOT(part_changed_slot(double)));

    // Use Window
    //
    grid = new QGridLayout();
    grid->setContentsMargins(qmtop);
    grid->setSpacing(2);
    vb->addLayout(grid);

    dc_wind = new QCheckBox(tr("Use window"));
    grid->addWidget(dc_wind, 0, 0, 1, 2);
    connect(dc_wind, SIGNAL(stateChanged(int)),
        this, SLOT(win_btn_slot(int)));

    dc_flat = new QCheckBox(tr("Flatten"));
    grid->addWidget(dc_flat, 0, 2);
    connect(dc_flat, SIGNAL(stateChanged(int)),
        this, SLOT(flat_btn_slot(int)));

    dc_set = new QPushButton(tr("Set"));
    dc_set->setCheckable(true);
    grid->addWidget(dc_set, 0, 3);
    connect(dc_set, SIGNAL(toggled(bool)), this, SLOT(set_btn_slot(bool)));

    // Window LBRT
    //
    dc_l_label = new QLabel(tr("Left"));
    grid->addWidget(dc_l_label, 1, 0);

    int ndgt = CD()->numDigits();

    dc_sb_left = new QDoubleSpinBox();
    dc_sb_left->setRange(-1e6, 1e6);
    dc_sb_left->setDecimals(ndgt);
    dc_sb_left->setValue(0.0);
    grid->addWidget(dc_sb_left, 1, 1);
    connect(dc_sb_left, SIGNAL(valueChanged(double)),
        this, SLOT(left_changed_slot(double)));

    dc_b_label = new QLabel(tr("Bottom"));
    grid->addWidget(dc_b_label, 1, 2);

    dc_sb_bottom = new QDoubleSpinBox();
    dc_sb_bottom->setRange(-1e6, 1e6);
    dc_sb_bottom->setDecimals(ndgt);
    dc_sb_bottom->setValue(0.0);
    grid->addWidget(dc_sb_bottom, 1, 3);
    connect(dc_sb_bottom, SIGNAL(valueChanged(double)),
        this, SLOT(botm_changed_slot(double)));

    dc_r_label = new QLabel(tr("Right"));
    grid->addWidget(dc_r_label, 2, 0);

    dc_sb_right = new QDoubleSpinBox();
    dc_sb_right->setRange(-1e6, 1e6);
    dc_sb_right->setDecimals(ndgt);
    dc_sb_right->setValue(0.0);
    grid->addWidget(dc_sb_right, 2, 1);
    connect(dc_sb_right, SIGNAL(valueChanged(double)),
        this, SLOT(right_changed_slot(double)));

    dc_t_label = new QLabel(tr("Top"));
    grid->addWidget(dc_t_label, 2, 2);

    dc_sb_top = new QDoubleSpinBox();
    dc_sb_top->setRange(-1e6, 1e6);
    dc_sb_top->setDecimals(ndgt);
    dc_sb_top->setValue(0.0);
    grid->addWidget(dc_sb_top, 2, 3);
    connect(dc_sb_top, SIGNAL(valueChanged(double)),
        this, SLOT(top_changed_slot(double)));

    // Check, Check Bg buttons
    //
    dc_check = new QPushButton(tr("Check\n"));
    dc_check->setCheckable(true);
    grid->addWidget(dc_check, 3, 0, 1, 2);
    connect(dc_check, SIGNAL(toggled(bool)), this, SLOT(check_btn_slot(bool)));

    // This is black magic to allow button pressess/releases to be
    // dispatched when the busy flag is set.  Un-setting the Check
    // button will pause the DRC run.
//XXX    g_object_set_data(G_OBJECT(button), "abort", (void*)1);

    dc_checkbg = new QPushButton(tr("Check in\nBackground"));
    dc_checkbg->setCheckable(true);
    grid->addWidget(dc_checkbg, 3, 2, 1, 2);
    connect(dc_checkbg, SIGNAL(toggled(bool)),
        this, SLOT(checkbg_btn_slot(bool)));

    // Jobs page
    //
    page = new QWidget();
    nbook->addTab(page, tr("Jobs"));
    vb = new QVBoxLayout(page);
    vb->setContentsMargins(qm);
    vb->setSpacing(2);

    dc_jobs = new QTtextEdit();
    dc_jobs->setReadOnly(true);
    dc_jobs->setMouseTracking(true);
    vb->addWidget(dc_jobs);
    connect(dc_jobs, SIGNAL(press_event(QMouseEvent*)),
        this, SLOT(mouse_press_slot(QMouseEvent*)));

    QFont *fnt;
    if (FC.getFont(&fnt, FNT_FIXED))
        dc_jobs->setFont(*fnt);
    connect(QTfont::self(), SIGNAL(fontChanged(int)),
        this, SLOT(font_changed_slot(int)), Qt::QueuedConnection);

    dc_kill = new QPushButton(tr("Abort job"));
    vb->addWidget(dc_kill);
    connect(dc_kill, SIGNAL(clicked()), this, SLOT(abort_btn_slot()));

    // Dismiss button
    //
    btn = new QPushButton(tr("Dismiss"));
    vbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(dismiss_btn_slot()));

    update();
}


QTdrcRunDlg::~QTdrcRunDlg()
{
    instPtr = 0;
    dc_region_quit();
    if (dc_caller)
        QTdev::Deselect(dc_caller);
}


void
QTdrcRunDlg::update()
{
    const char *s = CDvdb()->getVariable(VA_DrcChdName);
    QByteArray chd_ba = dc_chdname->text().toLatin1();
    const char *ss = chd_ba.constData();
    if (!s)
        s = "";
    if (!ss)
        ss = "";
    if (strcmp(s, ss))
        dc_chdname->setText(s);

    s = CDvdb()->getVariable(VA_DrcChdCell);
    QByteArray cname_ba = dc_cname->text().toLatin1();
    ss = cname_ba.constData();
    if (!s)
        s = "";
    if (!ss)
        ss = "";
    if (strcmp(s, ss))
        dc_cname->setText(s);

    s = CDvdb()->getVariable(VA_DrcPartitionSize);
    if (s) {
        double d = atof(s);
        if (d >= DRC_PART_MIN && d <= DRC_PART_MAX) {
            QTdev::SetStatus(dc_none, false);
            dc_sb_part->setEnabled(true);
            dc_sb_part->setValue(d);
        }
        else {
            // Huh?  Bad value, clear it.
            CDvdb()->clearVariable(VA_DrcPartitionSize);
        }
    }
    else {
        QTdev::SetStatus(dc_none, true);
        dc_sb_part->setEnabled(false);
    }

    QTdev::SetStatus(dc_use, dc_use_chd);

    dc_sb_left->setValue(dc_l_val);
    dc_sb_bottom->setValue(dc_b_val);
    dc_sb_right->setValue(dc_r_val);
    dc_sb_top->setValue(dc_t_val);

    QTdev::SetStatus(dc_wind, dc_use_win);
    dc_l_label->setEnabled(dc_use_win);
    dc_sb_left->setEnabled(dc_use_win);
    dc_b_label->setEnabled(dc_use_win);
    dc_sb_bottom->setEnabled(dc_use_win);
    dc_r_label->setEnabled(dc_use_win);
    dc_sb_right->setEnabled(dc_use_win);
    dc_t_label->setEnabled(dc_use_win);
    dc_sb_top->setEnabled(dc_use_win);

    QTdev::SetStatus(dc_flat, dc_flatten);
    dc_flat->setEnabled(dc_use_chd);

    update_jobs_list();
}


void
QTdrcRunDlg::update_jobs_list()
{
    if (!dc_jobs)
        return;
    QColor c1 = QTbag::PopupColor(GRattrColorHl4);

    int pid = get_pid();
    if (pid <= 0)
        dc_kill->setEnabled(false);

    double val = dc_jobs->get_scroll_value();
    dc_jobs->clear();

    char *list = DRC()->jobs();
    if (!list) {
        dc_jobs->setTextColor(c1);
        dc_jobs->set_chars("No background jobs running.");
    }
    else {
        dc_jobs->setTextColor(QColor("black"));
        dc_jobs->set_chars(list);
    }
    dc_jobs->set_scroll_value(val);
    if (pid > 0)
        select_pid(pid);
    dc_kill->setEnabled(get_pid() > 0);
}


void
QTdrcRunDlg::set_window(const BBox *BB)
{
    dc_sb_left->setValue(MICRONS(BB->left));
    dc_sb_bottom->setValue(MICRONS(BB->bottom));
    dc_sb_right->setValue(MICRONS(BB->right));
    dc_sb_top->setValue(MICRONS(BB->top));
}


void
QTdrcRunDlg::unset_set()
{
    QTdev::Deselect(dc_set);
}


// Select the chars in the range, start=end deselects existing.
//
void
QTdrcRunDlg::select_range(int start, int end)
{
    dc_jobs->select_range(start, end);
    dc_start = start;
    dc_end = end;
}


// Return the PID value of the selected line, or -1.
//
int
QTdrcRunDlg::get_pid()
{
    if (dc_line_selected < 0)
        return (-1);
    char *string = dc_jobs->get_chars();
    int line = 0;
    for (const char *s = string; *s; s++) {
        if (line == dc_line_selected) {
            while (isspace(*s))
                s++;
            int pid;
            int r = sscanf(s, "%d", &pid);
            delete [] string;
            return (r == 1 ? pid : -1);
        }
        if (*s == '\n')
            line++;
    }
    delete [] string;
    return (-1);
}


void
QTdrcRunDlg::select_pid(int p)
{
    char *string = dc_jobs->get_chars();
    bool nl = true;
    int line = 0;
    const char *cs = 0;
    for (const char *s = string; *s; s++) {
        if (nl) {
            cs = s;
            while (isspace(*s))
                s++;
            nl = false;
            int pid;
            int r = sscanf(s, "%d", &pid);
            if (r == 1 && p == pid) {
                const char *ce = cs;
                while (*ce && *ce != 'n')
                    ce++;
                select_range(cs - string, ce - string);
                delete [] string;
                dc_line_selected = line;
                dc_kill->setEnabled(true);
                return;
            }
        }
        if (*s == '\n') {
            nl = true;
            line++;
        }
    }
    delete [] string;
}


namespace {
    cCHD *find_chd(const char *name)
    {
        if (!name || !*name)
            return (0);
        cCHD *chd = CDchd()->chdRecall(name, false);
        if (chd)
            return (chd);

        sCHDin chd_in;
        if (!chd_in.check(name))
            return (0);
        chd = chd_in.read(name, sCHDin::get_default_cgd_type());
        return (chd);
    }
}


void
QTdrcRunDlg::run_drc(bool state, bool bg)
{
    if (!state) {
        if (!bg) {
            DSP()->SetInterrupt(DSPinterUser);
            if (!XM()->ConfirmAbort())
                QTdev::SetStatus(bg ? dc_checkbg : dc_check, true);
            else
                DRC()->setAbort(true);
            DSP()->SetInterrupt(DSPinterNone);
        }
        return;
    }
    if (dc_use_chd) {
        QByteArray chdname_ba = dc_chdname->text().toLatin1();
        const char *chdname = chdname_ba.constData();
        cCHD *chd = find_chd(chdname);
        if (!chd) {
            Log()->ErrorLog("DRC", "DRC aborted, CHD not found.");
            QTdev::Deselect(bg ? dc_checkbg : dc_check);
            return;
        }
        const char *cellname = 0;
        QByteArray cname_ba = dc_cname->text().toLatin1();
        const char *cn = cname_ba.constData();
        if (cn)
            cellname = lstring::gettok(&cn);
        if (dc_use_win) {
            BBox BB(INTERNAL_UNITS(dc_sb_left->value()),
                INTERNAL_UNITS(dc_sb_bottom->value()),
                INTERNAL_UNITS(dc_sb_right->value()),
                INTERNAL_UNITS(dc_sb_top->value()));
            DRC()->runDRC(&BB, bg, chd, cellname, dc_flatten);
        }
        else
            DRC()->runDRC(0, bg, chd, cellname, dc_flatten);
        delete [] cellname;
    }
    else {
        CDs *cursdp = CurCell(Physical);
        if (!cursdp) {
            QTdev::Deselect(bg ? dc_checkbg : dc_check);
            return;
        }
        if (dc_use_win) {
            BBox BB(INTERNAL_UNITS(dc_sb_left->value()),
                INTERNAL_UNITS(dc_sb_bottom->value()),
                INTERNAL_UNITS(dc_sb_right->value()),
                INTERNAL_UNITS(dc_sb_top->value()));
            DRC()->runDRC(&BB, bg);
        }
        else
            DRC()->runDRC(cursdp->BB(), bg);
    }

    if (instPtr)
        QTdev::Deselect(bg ? dc_checkbg : dc_check);
    DRC()->setAbort(false);
}


// Menu command for performing design rule checking on the current
// cell and its hierarchy.  The errors are printed on-screen, no
// file is generated.  The command will abort after DRC_PTMAX errors.
// The Enter key is ignored.  The command remains in effect until
// escaped.
//
void
QTdrcRunDlg::dc_region()
{
    if (WinCmd) {
        WinCmd->esc();
        return;
    }

    WinCmd = new WinState("window", "xic:check#set");
    if (!EV()->PushCallback(WinCmd)) {
        delete WinCmd;
        return;
    }
    WinCmd->message();
}


void
QTdrcRunDlg::dc_region_quit()
{
    if (WinCmd)
        WinCmd->esc();
}


void
QTdrcRunDlg::help_btn_slot()
{
    DSPmainWbag(PopUpHelp("xic:check"))
}


void
QTdrcRunDlg::use_btn_slot(bool state)
{
    dc_use_chd = state;
    dc_flat->setEnabled(dc_use_chd);
}


void
QTdrcRunDlg::chd_name_slot(const QString &qs)
{
    QByteArray qs_ba = qs.toLatin1();
    const char *s = qs_ba.constData();
    const char *ss = CDvdb()->getVariable(VA_DrcChdName);
    if (s && *s) {
        if (!ss || strcmp(ss, s))
            CDvdb()->setVariable(VA_DrcChdName, s);
    }
    else
        CDvdb()->clearVariable(VA_DrcChdName);
}


void
QTdrcRunDlg::cname_slot(const QString &qs)
{
    QByteArray qs_ba = qs.toLatin1();
    const char *s = qs_ba.constData();
    const char *ss = CDvdb()->getVariable(VA_DrcChdCell);
    if (s && *s) {
        if (!ss || strcmp(ss, s))
            CDvdb()->setVariable(VA_DrcChdCell, s);
    }
    else
        CDvdb()->clearVariable(VA_DrcChdCell);
}


void
QTdrcRunDlg::none_btn_slot(bool state)
{
    if (state) {
        dc_last_part_size = dc_sb_part->value();
        CDvdb()->clearVariable(VA_DrcPartitionSize);
    }
    else {
        char buf[32];
        snprintf(buf, sizeof(buf), "%.2f", dc_last_part_size);
        CDvdb()->setVariable(VA_DrcPartitionSize, buf);
    }
}


void
QTdrcRunDlg::part_changed_slot(double)
{
    QByteArray val_ba = dc_sb_part->cleanText().toLatin1();
    const char *s = val_ba.constData();
    CDvdb()->setVariable(VA_DrcPartitionSize, s);
}


void
QTdrcRunDlg::win_btn_slot(int state)
{
    dc_use_win = state;
    dc_l_label->setEnabled(dc_use_win);
    dc_sb_left->setEnabled(dc_use_win);
    dc_b_label->setEnabled(dc_use_win);
    dc_sb_bottom->setEnabled(dc_use_win);
    dc_r_label->setEnabled(dc_use_win);
    dc_sb_right->setEnabled(dc_use_win);
    dc_t_label->setEnabled(dc_use_win);
    dc_sb_top->setEnabled(dc_use_win);
}


void
QTdrcRunDlg::flat_btn_slot(int state)
{
    dc_flatten = state;
}


void
QTdrcRunDlg::set_btn_slot(bool)
{
    dc_region();
}


void
QTdrcRunDlg::left_changed_slot(double d)
{
    dc_l_val = d;
}


void
QTdrcRunDlg::botm_changed_slot(double d)
{
    dc_l_val = d;
}


void
QTdrcRunDlg::right_changed_slot(double d)
{
    dc_r_val = d;
}


void
QTdrcRunDlg::top_changed_slot(double d)
{
    dc_t_val = d;
}


void
QTdrcRunDlg::check_btn_slot(bool state)
{
    run_drc(state, false);
}


void
QTdrcRunDlg::checkbg_btn_slot(bool state)
{
    run_drc(state, true);
}


void
QTdrcRunDlg::mouse_press_slot(QMouseEvent *ev)
{
    if (ev->type() != QEvent::MouseButtonPress) {
        ev->ignore();
        return;
    }
    ev->accept();

    const char *str = lstring::copy(
        (const char*)dc_jobs->toPlainText().toLatin1());
    int x = ev->x();
    int y = ev->y();
    QTextCursor cur = dc_jobs->cursorForPosition(QPoint(x, y));
    int pos = cur.position();
    
    if (isspace(str[pos])) {
        // Clicked on white space.
        delete [] str;
        return;
    }

    const char *lineptr = str;
    int linenum = 0;
    for (int i = 0; i <= pos; i++) {
        if (str[i] == '\n') {
            if (i == pos) {
                // Clicked to right of line.
                delete [] str;
                return;
            }
            lineptr = str + i+1;
            linenum++;
        }
    }

    if (lineptr && *lineptr != '\n') {

        int start = lineptr - str;
        int end = start;
        while (str[end] && str[end] != '\n')
            end++;

        dc_line_selected = linenum;
        select_range(start, end);
        delete [] str;
        dc_kill->setEnabled(get_pid() > 0);
        return;
    }
    dc_line_selected = -1;
    delete [] str;
    select_range(0, 0);
    dc_kill->setEnabled(false);
}


void
QTdrcRunDlg::font_changed_slot(int fnum)
{
    if (fnum == FNT_FIXED) {
        QFont *fnt;
        if (FC.getFont(&fnt, FNT_FIXED))
            dc_jobs->setFont(*fnt);
        update_jobs_list();
    }
}


void
QTdrcRunDlg::abort_btn_slot()
{
    int pid = get_pid();
    if (pid > 0) {
#ifdef WIN32
        HANDLE h = OpenProcess(PROCESS_TERMINATE, 0, pid);
        TerminateProcess(h, 1);
#else
        kill(pid, SIGTERM);
#endif
        // Handler will clean up and update.
    }
}


void
QTdrcRunDlg::dismiss_btn_slot()
{
    DRC()->PopUpDrcRun(0, MODE_OFF);
}
// End of QTdrcRunDlg functions and slots.


WinState::WinState(const char *nm, const char *hk) : CmdState(nm, hk)
{
    WinCmd = this;
    state = 0;
    ghost_on = false;
}


WinState::~WinState()
{
    WinCmd = 0;
}


void
WinState::b1down()
{
    int x, y;
    EV()->Cursor().get_xy(&x, &y);
    if (state <= 0) {
        AOI.left = x;
        AOI.bottom = y;
        AOI.right = x;
        AOI.top = y;

        XM()->SetCoordMode(CO_RELATIVE, AOI.left, AOI.bottom);
        Gst()->SetGhost(GFbox);
        ghost_on = true;
        state = 1;
    }
    else {
        Gst()->SetGhost(GFnone);
        ghost_on = false;
        XM()->SetCoordMode(CO_ABSOLUTE);
        AOI.right = x;
        AOI.top = y;
        AOI.fix();
        QTdrcRunDlg *dc = QTdrcRunDlg::self();
        if (dc)
            dc->set_window(&AOI);
        state = 0;
    }
}


// Drc the box, if the pointer has moved, and the box has area.
// Otherwise set the state for next button 1 press.
//
void
WinState::b1up()
{
    if (state < 0) {
        state = 0;
        return;
    }
    if (state == 1) {
        if (!EV()->Cursor().is_release_ok()) {
            state = 2;
            return;
        }
        // dragged...
        int x, y;
        WindowDesc *wdesc = EV()->ButtonWin();
        if (!wdesc)
            wdesc = DSP()->MainWdesc();
        EV()->Cursor().get_release(&x, &y);
        wdesc->Snap(&x, &y);
        AOI.right = x;
        AOI.top = y;
        AOI.fix();
        int delta = (int)(2.0/wdesc->Ratio());
        if (delta >= AOI.width() && delta >= AOI.height()) {
            // ...but pointer didn't move enough
            state = 2;
            return;
        }
    }

    Gst()->SetGhost(GFnone);
    ghost_on = false;
    XM()->SetCoordMode(CO_ABSOLUTE);
    state = 0;

    QTdrcRunDlg *dc = QTdrcRunDlg::self();
    if (dc)
        dc->set_window(&AOI);
}


// Esc entered, clean up and abort.
//
void
WinState::esc()
{
    if (ghost_on)
        Gst()->SetGhost(GFnone);
    PL()->ErasePrompt();
    EV()->PopCallback(this);
    XM()->SetCoordMode(CO_ABSOLUTE);
    QTdrcRunDlg *dc = QTdrcRunDlg::self();
    if (dc)
        dc->unset_set();
    delete this;
}


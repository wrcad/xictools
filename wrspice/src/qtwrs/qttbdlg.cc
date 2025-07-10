
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
 * WRspice Circuit Simulation and Analysis Tool                           *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#include "config.h"
#include "qttbdlg.h"
#include "spglobal.h"
#include "cshell.h"
#include "commands.h"
#include "simulator.h"
#include "inpline.h"
#include "graph.h"
#include "kwords_analysis.h"
#include "circuit.h"
#include "qtinterf/qtcanvas.h"
#include "qtinterf/qtfont.h"
#include "miscutil/filestat.h"
#include "qttbdlg.bmp"

#include <signal.h>

#include <QLayout>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QPushButton>
#include <QGroupBox>
#include <QLineEdit>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QToolButton>
#include <QTimer>
//#include <QSocketNotifier>

#ifdef WIN32
#include <windows.h>
#endif
#include "../../icons/wrspice_16x16.xpm"
#include "../../icons/wrspice_32x32.xpm"
#include "../../icons/wrspice_48x48.xpm"


void
QTtoolbar::PopUpToolbar(ShowMode mode, int x, int y)
{
    if (mode == MODE_OFF) {
        delete QTtbDlg::self();
        return;
    }
    if (mode == MODE_UPD) {
        if (QTtbDlg::self())
            QTtbDlg::self()->update();
        return;
    }
    if (QTtbDlg::self())
        return;

    new QTtbDlg(x, y);
    QTtbDlg::self()->show();
}
// End of QTtoolbar functions.


QTtbDlg *QTtbDlg::instPtr;

QTtbDlg::QTtbDlg(int xx, int yy) : QTdraw(0)
{
    instPtr = this;
    tb_file_menu = 0;
    tb_source_btn = 0;
    tb_load_btn = 0;
    tb_edit_menu = 0;
    tb_tools_menu = 0;
    tb_help_menu = 0;
    tb_dropfile = 0;
    tb_elapsed_start = 0.0;
    tb_clr_1 = 0;
    tb_clr_2 = 0;
    tb_clr_3 = 0;
    tb_clr_4 = 0;

    QIcon icon;
    icon.addPixmap(QPixmap(wrs_16x16_xpm));
    icon.addPixmap(QPixmap(wrs_32x32_xpm));
    icon.addPixmap(QPixmap(wrs_48x48_xpm));
    setWindowIcon(icon);

    char tbuf[28];
    snprintf(tbuf, sizeof(tbuf), "WRspice-%s", Global.Version());
    setWindowTitle(tbuf);
    setAttribute(Qt::WA_DeleteOnClose);

    QMenuBar *menubar = new QMenuBar(this);
    QAction *a;

    // File menu.
    tb_file_menu = menubar->addMenu("&File");
    // "/File/_File Select", "<control>O"
    a = tb_file_menu->addAction(tr("&File Select"));
    a->setData(MA_file_sel);
    a->setShortcut(QKeySequence("Ctrl+O"));
    a->setToolTip(tr("Show File Selection panel"));

    // "/File/_Source", "<control>S"
    a = tb_file_menu->addAction(tr("&Source"));
    a->setData(MA_source);
    a->setShortcut(QKeySequence("Ctrl+S"));
    a->setToolTip(tr("Source input file"));
    tb_source_btn = a;;

    // "/File/_Load", "<control>L"
    a = tb_file_menu->addAction(tr("&Load"));
    a->setData(MA_load);
    a->setShortcut(QKeySequence("Ctrl+L"));
    a->setToolTip(tr("Load plot data file"));
    tb_load_btn = a;

    // "/File/Save _Tools", 0
    a = tb_file_menu->addAction(tr("Save &Tools"));
    a->setData(MA_save_tools);
    a->setToolTip(tr("Save the current tool window locations and visibility"));

    // "/File/Save Fonts", 0
    a = tb_file_menu->addAction(tr("Save Fonts"));
    a->setData(MA_save_fonts);
    a->setToolTip(tr("Save the current fonts persistently"));

    /*  Package update is not currently supported.
    // "/File/Update _WRspice", 0
    a = tb_file_menu->addAction(tr("Update &WRspice"));
    a->setData(MA_upd_wrs);
    a->setToolTip(tr("Update WRspice"));
    */

    tb_file_menu->addSeparator();

    if (CP.GetFlag(CP_NOTTYIO)) {
        // "/File/_Close", "<control>Q"
        a = tb_file_menu->addAction(tr("&Close"));
        a->setShortcut(QKeySequence("Ctrl+Q"));
        a->setToolTip(tr("Close WRspice"));
    }
    else {
        // "/File/_Quit", "<control>Q"
        a = tb_file_menu->addAction(tr("&Quit"));
        a->setShortcut(QKeySequence("Ctrl+Q"));
        a->setToolTip(tr("Quit WRspice"));
    }
    a->setData(MA_dismiss);
    connect(tb_file_menu, &QMenu::triggered,
        this, &QTtbDlg::file_menu_slot);

    // Edit menu.
    tb_edit_menu = menubar->addMenu(tr("&Edit"));
    // "/Edit/_Text Editor", "<control>T"
    a = tb_edit_menu->addAction(tr("&Text Editor"));
    a->setData(MA_txt_edit);
    a->setShortcut(QKeySequence("Ctrl+T"));
    a->setToolTip(tr("Pop up text editor"));

    if (!CP.GetFlag(CP_NOTTYIO)) {
        // "/Edit/_Xic", "<control>X"
        a = tb_edit_menu->addAction(tr("&Xic"));
        a->setData(MA_xic);
        a->setShortcut(QKeySequence("Ctrl+X"));
        a->setToolTip(tr("Start Xic"));
    }
    connect(tb_edit_menu, &QMenu::triggered,
        this, &QTtbDlg::edit_menu_slot);

    // Tools menu.
    tb_tools_menu = menubar->addMenu(tr("&Tools"));

    // "/Tools/Fo_nts", "<alt>N", "<CheckItem>");
    a = tb_tools_menu->addAction(tr("Fo&nts"));
    a->setCheckable(true);
    a->setData(tid_font);
    a->setShortcut(QKeySequence("Alt+N"));
    a->setToolTip(tr("Set window fonts"));
    QTdev::SetStatus(a, (QTtoolbar::entries(tid_font)->dialog() != 0));
    QTtoolbar::entries(tid_font)->set_action(a);

    // "/Tools/_Files", "<alt>Z", "<CheckItem>"
    a = tb_tools_menu->addAction(tr("&Files"));
    a->setCheckable(true);
    a->setData(tid_files);
    a->setShortcut(QKeySequence("Alt+Z"));
    a->setToolTip(tr("List search path files"));
    QTdev::SetStatus(a, (QTtoolbar::entries(tid_files)->dialog() != 0));
    QTtoolbar::entries(tid_files)->set_action(a);

    // "/Tools/_Circuits", "<alt>C", "<CheckItem>"
    a = tb_tools_menu->addAction(tr("&Circuits"));
    a->setCheckable(true);
    a->setData(tid_circuits);
    a->setShortcut(QKeySequence("Alt+C"));
    a->setToolTip(tr("List circuits"));
    QTdev::SetStatus(a, (QTtoolbar::entries(tid_circuits)->dialog() != 0));
    QTtoolbar::entries(tid_circuits)->set_action(a);

    // "/Tools/_Plots", "<alt>P", "<CheckItem>");
    a = tb_tools_menu->addAction(tr("&Plots"));
    a->setCheckable(true);
    a->setData(tid_plots);
    a->setShortcut(QKeySequence("Alt+P"));
    a->setToolTip(tr("List result plot data"));
    QTdev::SetStatus(a, (QTtoolbar::entries(tid_plots)->dialog() != 0));
    QTtoolbar::entries(tid_plots)->set_action(a);

    // "/Tools/P_lot Opts", "<alt>L", "<CheckItem>"
    a = tb_tools_menu->addAction(tr("P&lot Opts"));
    a->setCheckable(true);
    a->setData(tid_plotdefs);
    a->setShortcut(QKeySequence("Alt+L"));
    a->setToolTip(tr("Set plot options"));
    QTdev::SetStatus(a, (QTtoolbar::entries(tid_plotdefs)->dialog() != 0));
    QTtoolbar::entries(tid_plotdefs)->set_action(a);

    // "/Tools/C_olors", "<alt>O", "<CheckItem>");
    a = tb_tools_menu->addAction(tr("C&olors"));
    a->setCheckable(true);
    a->setData(tid_colors);
    a->setShortcut(QKeySequence("Alt+O"));
    a->setToolTip(tr("Set plot colors"));
    QTdev::SetStatus(a, (QTtoolbar::entries(tid_colors)->dialog() != 0));
    QTtoolbar::entries(tid_colors)->set_action(a);

    // "/Tools/_Vectors", "<alt>V", "<CheckItem>"
    a = tb_tools_menu->addAction(tr("&Vectors"));
    a->setCheckable(true);
    a->setData(tid_vectors);
    a->setShortcut(QKeySequence("Alt+V"));
    a->setToolTip(tr("List vectors in current plot"));
    QTdev::SetStatus(a, (QTtoolbar::entries(tid_vectors)->dialog() != 0));
    QTtoolbar::entries(tid_vectors)->set_action(a);

    // "/Tools/Va_riables", "<alt>R", "<CheckItem>");
    a = tb_tools_menu->addAction(tr("Va&riables"));
    a->setCheckable(true);
    a->setData(tid_variables);
    a->setShortcut(QKeySequence("Alt+R"));
    a->setToolTip(tr("List set shell variables"));
    QTdev::SetStatus(a, (QTtoolbar::entries(tid_variables)->dialog() != 0));
    QTtoolbar::entries(tid_variables)->set_action(a);

    // "/Tools/_Shell", "<alt>S", "<CheckItem>");
    a = tb_tools_menu->addAction(tr("&Shell"));
    a->setCheckable(true);
    a->setData(tid_shell);
    a->setShortcut(QKeySequence("Alt+S"));
    a->setToolTip(tr("Set shell options"));
    QTdev::SetStatus(a, (QTtoolbar::entries(tid_shell)->dialog() != 0));
    QTtoolbar::entries(tid_shell)->set_action(a);

    // "/Tools/S_im Opts", "<alt>I", "<CheckItem>"
    a = tb_tools_menu->addAction(tr("S&im Opts"));
    a->setCheckable(true);
    a->setData(tid_simdefs);
    a->setShortcut(QKeySequence("Alt+I"));
    a->setToolTip(tr("Set simulation options"));
    QTdev::SetStatus(a, (QTtoolbar::entries(tid_simdefs)->dialog() != 0));
    QTtoolbar::entries(tid_simdefs)->set_action(a);

    // "/Tools/Co_mmands", "<alt>M", "<CheckItem>"
    a = tb_tools_menu->addAction(tr("Co&mmands"));
    a->setCheckable(true);
    a->setData(tid_commands);
    a->setShortcut(QKeySequence("Alt+M"));
    a->setToolTip(tr("Set command options"));
    QTdev::SetStatus(a, (QTtoolbar::entries(tid_commands)->dialog() != 0));
    QTtoolbar::entries(tid_commands)->set_action(a);

    // "/Tools/_Runops", "<alt>U", "<CheckItem>");
    a = tb_tools_menu->addAction(tr("&Runops"));
    a->setCheckable(true);
    a->setData(tid_runops);
    a->setShortcut(QKeySequence("Alt+U"));
    a->setToolTip(tr("List runops (traces, etc.) in effect"));
    QTdev::SetStatus(a, (QTtoolbar::entries(tid_runops)->dialog() != 0));
    QTtoolbar::entries(tid_runops)->set_action(a);

    // "/Tools/_Debug", "<alt>D", "<CheckItem>");
    a = tb_tools_menu->addAction(tr("&Debug"));
    a->setCheckable(true);
    a->setData(tid_debug);
    a->setShortcut(QKeySequence("Alt+D"));
    a->setToolTip(tr("Set debugging options"));
    QTdev::SetStatus(a, (QTtoolbar::entries(tid_debug)->dialog() != 0));
    QTtoolbar::entries(tid_debug)->set_action(a);
    connect(tb_tools_menu, &QMenu::triggered,
        this, &QTtbDlg::tools_menu_slot);

    // Help menu.
    tb_help_menu = menubar->addMenu(tr("&Help"));

    // "/Help/_Help", "<control>H"
    a = tb_help_menu->addAction(tr("&Help"));
    a->setData(MA_help);
    a->setShortcut(QKeySequence("Ctrl+H"));
    a->setToolTip(tr("Pop up Help window"));

    // "/Help/_About", "<control>A"
    a = tb_help_menu->addAction(tr("&About"));
    a->setData(MA_about);
    a->setShortcut(QKeySequence("Ctrl+A"));
    a->setToolTip(tr("Pop up About window"));

    // "/Help/_Notes", "<control>N"
    a = tb_help_menu->addAction(tr("&Notes"));
    a->setData(MA_notes);
    a->setShortcut(QKeySequence("Ctrl+N"));
    a->setToolTip(tr("Show release notes"));
    connect(tb_help_menu, &QMenu::triggered,
        this, &QTtbDlg::help_menu_slot);

    QMargins qmtop(2, 2, 2, 2);
    QMargins qm;
    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(qmtop);
    vbox->setSpacing(2);

    QHBoxLayout *hbox = new QHBoxLayout();
    vbox->addLayout(hbox);
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);

    // the WR logo button
    QToolButton *btn =  new QToolButton();
    btn->setIcon(QPixmap(tm30));
    hbox->addWidget(btn);
    btn->setToolTip(tr("Pop up email client"));
    connect(btn, &QAbstractButton::clicked, this, &QTtbDlg::wr_btn_slot);

    // the Run button
    btn = new QToolButton();
    btn->setIcon(QPixmap(run_xpm));
    hbox->addWidget(btn);
    btn->setToolTip(tr("Run current circuit"));
    connect(btn, &QAbstractButton::clicked, this, &QTtbDlg::run_btn_slot);

    // the Stop button
    btn = new QToolButton();
    btn->setIcon(QPixmap(stop_xpm));
    hbox->addWidget(btn);
    btn->setToolTip(tr("Pause current analysis"));
    connect(btn, &QAbstractButton::clicked, this, &QTtbDlg::stop_btn_slot);

#ifdef Q_OS_MACOS
    hbox->addWidget(new QWidget());
    hbox->setStretch(3, 1);
#else
    hbox->addSpacing(20);
    hbox->addWidget(menubar);
#endif

    gd_viewport = new QTcanvas();
    vbox->addWidget(gd_viewport);
    vbox->setStretch(1, 1);

    QFont *fnt;
    if (Fnt()->getFont(&fnt, FNT_SCREEN))
        gd_viewport->set_font(fnt);
    connect(QTfont::self(), &QTfont::fontChanged,
        this, &QTtbDlg::font_changed_slot, Qt::QueuedConnection);
    gd_viewport->setFocusPolicy(Qt::StrongFocus);
    gd_viewport->setAcceptDrops(true);

    connect(gd_viewport, &QTcanvas::drag_enter_event,
        this, &QTtbDlg::drag_enter_slot);
    connect(gd_viewport, &QTcanvas::drop_event,
        this, &QTtbDlg::drop_slot);

    int wid, hei;
    TextExtent(0, &wid, &hei);
    wid = 40*wid + 6;
    hei = 6*hei + 6;
    Viewport()->setMinimumSize(QSize(wid, hei));

    // drawing colors
    const char *c1 = 0;
    const char *c2 = 0;
    const char *c3 = 0;
    const char *c4 = 0;
#ifdef WITH_X11
    c1 = XRMgetFromDb("fgcolor1");
    c2 = XRMgetFromDb("fgcolor2");
    c3 = XRMgetFromDb("fgcolor3");
    c4 = XRMgetFromDb("fgcolor4");
#endif
    if (!c1)
        c1 = "sienna";
    if (!c2)
        c2 = "black";
    if (!c3)
        c3 = "red";
    if (!c4)
        c4 = "blue";
    tb_clr_1 = QTdev::self()->NameColor(c1);
    tb_clr_2 = QTdev::self()->NameColor(c2);
    tb_clr_3 = QTdev::self()->NameColor(c3);
    tb_clr_4 = QTdev::self()->NameColor(c4);

    SetWindowBackground(SpGrPkg::DefColors[0].pixel);
    SetBackground(SpGrPkg::DefColors[0].pixel);
    Clear();

    // Can't call update from here, it won't show.  We need to wait
    // until the window is realized.  The timout will be called after
    // the event queue is processed so a small delay should be ok. 
    // Here we draw once quickly and periodically refresh more slowly.

    TB()->RegisterTimeoutProc(50, tb_res_timeout, (void*)1L);
    TB()->RegisterTimeoutProc(2000, tb_res_timeout, 0);
    TB()->FixLoc(&xx, &yy);
    TB()->SetActiveDlg(tid_toolbar, this);
    move(xx, yy);

    if (isatty(fileno(stdin))) {
        setWindowFlag(Qt::WindowStaysOnTopHint);
#ifdef Q_OS_MACOS
        QTimer::singleShot(100, this, &QTtbDlg::revert_focus);
#else
        setAttribute(Qt::WA_ShowWithoutActivating);
        QTimer::singleShot(0, this, &QTtbDlg::revert_focus);
#endif
    }

    // Here's a bad thing...
    // We need to do something to increase the frequency of interrupt
    // handling of the text interface, otherwise it is very slow, when
    // typing characters, several seconds are required for each character
    // to appear.  This solves the text response problem, and also allows
    // simulation to run quickly, but will hog cpu time.
    //
    startTimer(0);

    // Here's something that is interesting but doesn't solve the problem.
    // This will wake the event loop when there is something to read on
    // stdin, so character processing appears instantaneous, and no cpu
    // hogging.  However, the simulations run incredibly slowly.
    //
    //new QSocketNotifier(STDIN_FILENO, QSocketNotifier::Read);
}


QTtbDlg::~QTtbDlg()
{
    instPtr = 0;
    TB()->SetLoc(tid_toolbar, this);
    TB()->SetActiveDlg(tid_toolbar, 0);
}


void
QTtbDlg::closeEvent(QCloseEvent *ev)
{
    ev->ignore();
    // Quit the program, confirm if work is unsaved.
    if (CP.GetFlag(CP_NOTTYIO)) {
        // In server mode, just hide ourself.
        hide();
    }
    else {
        CommandTab::com_quit(0);
        ::raise(SIGINT);  // for new prompt, else it hangs
    }
}


namespace {
    // Convert elapsed seconds to hours:minutes:seconds.
    //
    void
    hms(double elapsed, int *hours, int *minutes, double *secs)
    {
        *hours = (int)(elapsed/3600);
        elapsed -= *hours*3600;
        *minutes = (int)(elapsed/60);
        elapsed -= *minutes*60;
        *secs = elapsed;
    }
}


// Redraw the resource information.
//
void
QTtbDlg::update(ResUpdType updt)
{
    if (!GP.isMainThread())
        return;
    if (updt == RES_UPD_TIME) {
        double elapsed, user, cpu;
        ResPrint::get_elapsed(&elapsed, &user, &cpu);
        tb_elapsed_start = elapsed;
    }
    else {
        int wid = width();
        int hei = height();

        int fwid, dy;
        TextExtent(0, &fwid, &dy);
        SetWindowBackground(SpGrPkg::DefColors[0].pixel);
        SetBackground(SpGrPkg::DefColors[0].pixel);
        int yy = dy;
        int xx = 4;
        int ux = 18*fwid;
        int vx = ux + 14*fwid;

        SetColor(SpGrPkg::DefColors[0].pixel);
        Box(0, 0, wid, hei);

        double elapsed, user, cpu;
        ResPrint::get_elapsed(&elapsed, &user, &cpu);
        elapsed -= tb_elapsed_start;
        int hours, minutes;
        double secs;
        char buf[128];
        if (elapsed < 0.0)
            elapsed = 0.0;
        hms(elapsed, &hours, &minutes, &secs);
        SetColor(tb_clr_1);
        Text("elapsed", xx, yy, 0);
        SetColor(tb_clr_2);
        *buf = 0;
        if (hours)
            snprintf(buf, sizeof(buf), "%d:", hours);
        if (minutes || hours) {
            int len = strlen(buf);
            snprintf(buf + len, sizeof(buf) - len, "%d:", minutes);
        }
        int len = strlen(buf);
        snprintf(buf + len, sizeof(buf) - len, "%.2f", secs);
        Text(buf, ux, yy, 0);
        if (Sp.GetFlag(FT_SIMFLAG)) {
            strcpy(buf, "running");
            SetColor(tb_clr_3);
        }
        else if (Sp.CurCircuit() && Sp.CurCircuit()->inprogress()) {
            strcpy(buf, "stopped");
            SetColor(tb_clr_4);
        }
        else
            strcpy(buf, "idle");
        Text(buf, vx, yy, 0);
        yy += dy;
        if (Sp.GetFlag(FT_SIMFLAG) && Sp.CurCircuit()) {
            double pct = Sp.CurCircuit()->getPctDone();
            if (pct > 0.0) {
                snprintf(buf, sizeof(buf), "%.1f%%", pct);
                Text(buf, vx, yy, 0);
            }
        }
        if (user >= 0.0) {
            hms(user, &hours, &minutes, &secs);
            SetColor(tb_clr_1);
            Text("total user", xx, yy, 0);
            SetColor(tb_clr_2);
            *buf = 0;
            if (hours)
                snprintf(buf, sizeof(buf), "%d:", hours);
            if (minutes || hours) {
                len = strlen(buf);
                snprintf(buf + len, sizeof(buf) - len, "%d:", minutes);
            }
            len = strlen(buf);
            snprintf(buf + len, sizeof(buf) - len, "%.2f", secs);
            Text(buf, ux, yy, 0);
            yy += dy;
        }
        if (cpu >= 0.0) {
            hms(cpu, &hours, &minutes, &secs);
            SetColor(tb_clr_1);
            Text("total system", xx, yy, 0);
            SetColor(tb_clr_2);
            *buf = 0;
            if (hours)
                snprintf(buf, sizeof(buf), "%d:", hours);
            if (minutes || hours) {
                len = strlen(buf);
                snprintf(buf + len, sizeof(buf) - len, "%d:", minutes);
            }
            len = strlen(buf);
            snprintf(buf + len, sizeof(buf) - len, "%.2f", secs);
            Text(buf, ux, yy, 0);
            yy += dy;
        }
        unsigned int dta, hlimit, slimit;
        ResPrint::get_space(&dta, &hlimit, &slimit);
        if (dta) {
            SetColor(tb_clr_1);
            Text("data size", xx, yy, 0);
            SetColor(tb_clr_2);
            snprintf(buf, sizeof(buf), "%d", dta/1024);
            Text(buf, ux, yy, 0);
            yy += dy;

            double val = DEF_maxData;
            VTvalue vv;
            if (Sp.GetVar(spkw_maxdata, VTYP_REAL, &vv, Sp.CurCircuit()))
                val = vv.get_real();
            SetColor(tb_clr_1);
            Text("program limit", xx, yy, 0);
            SetColor(tb_clr_2);
            snprintf(buf, sizeof(buf), "%d", (int)val);
            Text(buf, ux, yy, 0);
            yy += dy;

            if (hlimit || slimit) {
                if (hlimit == 0 || (slimit > 0 && slimit < hlimit))
                    hlimit = slimit;
                SetColor(tb_clr_1);
                Text("system limit", xx, yy, 0);
                SetColor(tb_clr_2);
                snprintf(buf, sizeof(buf), "%d", hlimit/1024);
                Text(buf, ux, yy, 0);
                yy += dy;
            }
        }
        Update();
    }
}


// Revert focus to the starting console.
//
void
QTtbDlg::revert_focus()
{
#ifdef Q_OS_MACOS
    if (Sp.GetVar("tbrevert", VTYP_BOOL, 0)) {
        system(
            "osascript -e \"tell application \\\"Terminal\\\" to activate\"");
    }
    if (!Sp.GetVar("tbontop", VTYP_BOOL, 0))
        setWindowFlag(Qt::WindowStaysOnTopHint, false);
#else
    if (!Sp.GetVar("tbontop", VTYP_BOOL, 0))
        setWindowFlag(Qt::WindowStaysOnTopHint, false);
    setAttribute(Qt::WA_ShowWithoutActivating, false);
#endif
    show();
}


// Static function.
// Periodically update the resource listing in the toolbar widget.
//
int
QTtbDlg::tb_res_timeout(void *v)
{
    if (instPtr) {
        instPtr->update(RES_UPD);
        if (!v)
            return (true);
    }
    return (false);
}


namespace {
    // OK button callback for the file selection.
    //
    void
    file_sel_cb(const char *fname, void*)
    {
        wordlist wl;
        wl.wl_word = (char*)fname;

        // if the file is a rawfile, load it
        FILE *fp = fopen(fname, "r");
        if (fp) {
            char buf[128];
            const char *s = fgets(buf, 128, fp);
            if (lstring::prefix("Title:", s)) {
                s = fgets(buf, 128, fp);
                if (lstring::prefix("Date:", s)) {
                    fclose(fp);
                    TTY.monitor();
                    CommandTab::com_load(&wl);
                    if (TTY.wasoutput())
                        CP.Prompt();
                    return;
                }
            }
            fclose(fp);
        }
        TTY.monitor();
        CommandTab::com_source(&wl);
        if (TTY.wasoutput())
            CP.Prompt();
    }


    // Source circuit, passed to pop-up.
    //
    void source_cb(const char *fname, void*)
    {
        if (fname && *fname) {
            wordlist wl;
            wl.wl_word = (char*)fname;
            TTY.monitor();
            CommandTab::com_source(&wl);
            if (TTY.wasoutput())
                CP.Prompt();
            if (TB()->ActiveInput())
                TB()->ActiveInput()->popdown();
        }
    }


    // Load rawfile, passed to pop-up.
    //
    void load_cb(const char *fname, void*)
    {
        if (fname && *fname) {
            wordlist wl;
            wl.wl_word = (char*)fname;
            TTY.monitor();
            CommandTab::com_load(&wl);
            if (TTY.wasoutput())
                CP.Prompt();
            if (TB()->ActiveInput())
                TB()->ActiveInput()->popdown();
        }
    }
}


void
QTtbDlg::file_menu_slot(QAction *a)
{
    int tp = a->data().toInt();
    if (tp == MA_file_sel) {
        // Pop up the file selection panel.
        static int cnt;
        int xx = 100, yy = 100;
        xx += cnt*200;
        yy += cnt*100;
        TB()->FixLoc(&xx, &yy);
        cnt++;
        if (cnt == 3)
            cnt = 0;
        TB()->PopUpFileSelector(fsSEL, GRloc(LW_XYA, xx, yy),
            file_sel_cb, 0, 0, 0);
    }
    else if (tp == MA_source) {
        // Prompt for circuit to source.
        char buf[512];
        *buf = '\0';
        if (tb_dropfile) {
            strcpy(buf, tb_dropfile);
            delete [] tb_dropfile;
            tb_dropfile = 0;
        }
        TB()->PopUpInput("Name of circuit file to source?", buf,
            "Source", source_cb, 0);
    }
    else if (tp == MA_load) {
        // Prompt for rawfile to load.
        char buf[512];
        *buf = '\0';
        if (tb_dropfile) {
            strcpy(buf, tb_dropfile);
            delete [] tb_dropfile;
            tb_dropfile = 0;
        }
        TB()->PopUpInput("Name of rawfile to load?", buf, "Load",
            load_cb, 0);
    }
    else if (tp == MA_save_tools) {
        // Save the tool window locations and visibilities.
        CommandTab::com_tbupdate(0);
    }
    else if (tp == MA_save_fonts) {
        // Save the current fonts.
        CommandTab::com_savefonts(0);
    }
/* Not currently supported.
    else if (tp == MA_upd_wrs) {
        // Check for updates, download/install update.
        CommandTab::com_wrupdate(0);
        ::raise(SIGINT);  // for new prompt, else it hangs
    }
*/
    else if (tp == MA_dismiss) {
        // Quit the program, confirm if work is unsaved.
        if (CP.GetFlag(CP_NOTTYIO)) {
            // In server mode, just hide ourself.
            hide();
        }
        else {
            CommandTab::com_quit(0);
            ::raise(SIGINT);  // for new prompt, else it hangs
        }
    }
}


void
QTtbDlg::edit_menu_slot(QAction *a)
{
    int tp = a->data().toInt();
    if (tp == MA_txt_edit) {
        // Pop up a text editor.
        CommandTab::com_edit(0);
    }
    else if (tp == MA_xic) {
        // Start the Xic program.
        bool usearg = false;
        wordlist wl;
        if (Sp.CurCircuit() && Sp.CurCircuit()->filename()) {
            FILE *fp = fopen(Sp.CurCircuit()->filename(), "r");
            if (fp) {
                char *s, buf[132];
                while ((s = fgets(buf, 128, fp)) != 0) {
                    while (isspace(*s))
                        s++;
                    if (!*s)
                        continue;
                    if (lstring::prefix(GFX_PREFIX, s)) {
                        wl.wl_next = wl.wl_prev = 0;
                        wl.wl_word = (char*)Sp.CurCircuit()->filename();
                        usearg = true;
                    }
                    break;
                }
                fclose(fp);
            }
        }
        CommandTab::com_sced(usearg ? &wl : 0);
    }
}


void
QTtbDlg::tools_menu_slot(QAction *a)
{
    int id = a->data().toInt();
    QTtoolbar::tbent_t *tb = QTtoolbar::entries(id);

    if (id == tid_bug) {
        if (a->isChecked() && !tb->dialog())
            TB()->PopUpBugRpt(MODE_ON, tb->x(), tb->y());
        else if (!a->isChecked() && tb->dialog())
            TB()->PopUpBugRpt(MODE_OFF, tb->x(), tb->y());
    }
    else if (id == tid_font) {
        if (a->isChecked() && !tb->dialog())
            TB()->PopUpFont(MODE_ON, tb->x(), tb->y());
        else if (!a->isChecked() && tb->dialog())
            TB()->PopUpFont(MODE_OFF, tb->x(), tb->y());
    }
    else if (id == tid_files) {
        if (a->isChecked() && !tb->dialog())
            TB()->PopUpFiles(MODE_ON, tb->x(), tb->y());
        else if (!a->isChecked() && tb->dialog())
            TB()->PopUpFiles(MODE_OFF, tb->x(), tb->y());
    }
    else if (id == tid_circuits) {
        if (a->isChecked() && !tb->dialog())
            TB()->PopUpCircuits(MODE_ON, tb->x(), tb->y());
        else if (!a->isChecked() && tb->dialog())
            TB()->PopUpCircuits(MODE_OFF, tb->x(), tb->y());
    }
    else if (id == tid_plots) {
        if (a->isChecked() && !tb->dialog())
            TB()->PopUpPlots(MODE_ON, tb->x(), tb->y());
        else if (!a->isChecked() && tb->dialog())
            TB()->PopUpPlots(MODE_OFF, tb->x(), tb->y());
    }
    else if (id == tid_plotdefs) {
        if (a->isChecked() && !tb->dialog())
            TB()->PopUpPlotDefs(MODE_ON, tb->x(), tb->y());
        else if (!a->isChecked() && tb->dialog())
            TB()->PopUpPlotDefs(MODE_OFF, tb->x(), tb->y());
    }
    else if (id == tid_colors) {
        if (a->isChecked() && !tb->dialog())
            TB()->PopUpColors(MODE_ON, tb->x(), tb->y());
        else if (!a->isChecked() && tb->dialog())
            TB()->PopUpColors(MODE_OFF, tb->x(), tb->y());
    }
    else if (id == tid_vectors) {
        if (a->isChecked() && !tb->dialog())
            TB()->PopUpVectors(MODE_ON, tb->x(), tb->y());
        else if (!a->isChecked() && tb->dialog())
            TB()->PopUpVectors(MODE_OFF, tb->x(), tb->y());
    }
    else if (id == tid_variables) {
        if (a->isChecked() && !tb->dialog())
            TB()->PopUpVariables(MODE_ON, tb->x(), tb->y());
        else if (!a->isChecked() && tb->dialog())
            TB()->PopUpVariables(MODE_OFF, tb->x(), tb->y());
    }
    else if (id == tid_shell) {
        if (a->isChecked() && !tb->dialog())
            TB()->PopUpShellDefs(MODE_ON, tb->x(), tb->y());
        else if (!a->isChecked() && tb->dialog())
            TB()->PopUpShellDefs(MODE_OFF, tb->x(), tb->y());
    }
    else if (id == tid_simdefs) {
        if (a->isChecked() && !tb->dialog())
            TB()->PopUpSimDefs(MODE_ON, tb->x(), tb->y());
        else if (!a->isChecked() && tb->dialog())
            TB()->PopUpSimDefs(MODE_OFF, tb->x(), tb->y());
    }
    else if (id == tid_commands) {
        if (a->isChecked() && !tb->dialog())
            TB()->PopUpCmdConfig(MODE_ON, tb->x(), tb->y());
        else if (!a->isChecked() && tb->dialog())
            TB()->PopUpCmdConfig(MODE_OFF, tb->x(), tb->y());
    }
    else if (id == tid_runops) {
        if (a->isChecked() && !tb->dialog())
            TB()->PopUpRunops(MODE_ON, tb->x(), tb->y());
        else if (!a->isChecked() && tb->dialog())
            TB()->PopUpRunops(MODE_OFF, tb->x(), tb->y());
    }
    else if (id == tid_debug) {
        if (a->isChecked() && !tb->dialog())
            TB()->PopUpDebugDefs(MODE_ON, tb->x(), tb->y());
        else if (!a->isChecked() && tb->dialog())
            TB()->PopUpDebugDefs(MODE_OFF, tb->x(), tb->y());
    }
}


void
QTtbDlg::help_menu_slot(QAction *a)
{
    int tp = a->data().toInt();
    if (tp == MA_help) {
        // Pop up the help browser.
        CommandTab::com_help(0);
    }
    else if (tp == MA_about) {
        // Pop up a file containing the "about" information.
        char buf[256];
        if (Global.StartupDir() && *Global.StartupDir()) {
            snprintf(buf, sizeof(buf), "%s/%s", Global.StartupDir(),
                "wrspice_mesg");
            FILE *fp = fopen(buf, "r");
            if (fp) {
                bool didsub = false;
                sLstr lstr;
                while (fgets(buf, 256, fp) != 0) {
                    if (!didsub) {
                        char *s = strchr(buf, '$');
                        if (s) { 
                            didsub = true;
                            *s = '\0';
                            lstr.add(buf);
                            lstr.add(Global.Version());
                            lstr.add(s+1);
                            continue;
                        }
                    }
                    lstr.add(buf);
                }
                fclose(fp);
                TB()->PopUpHTMLinfo(MODE_ON, lstr.string(), GRloc(LW_CENTER));
                return;
            }
        }
        fprintf(stderr, "Warning: can't open wrspice_mesg file.\n");
    }
    else if (tp == MA_notes) {
        // Pop up a file browser loaded with the release notes.
        const char *docspath = 0;
        VTvalue vv;
        if (Sp.GetVar("docsdir", VTYP_STRING, &vv))
            docspath = vv.get_string();
        else {
            docspath = getenv("SPICE_DOCS_DIR");
            if (!docspath)
                docspath = Global.DocsDir();
        }
        if (!docspath || !*docspath) {
            TB()->PopUpMessage(
                "No path to release notes (set docsdir).", true);
            return;
        }

        char buf[256];
        snprintf(buf, sizeof(buf), "%s/wrs%s", docspath, Global.Version());

        // Remove last component of version, file name is like "wrs3.0".
        char *t = strrchr(buf, '.');
        if (t)
            *t = 0;

        check_t ret = filestat::check_file(buf, R_OK);
        if (ret == NOGO) {
            TB()->PopUpMessage(filestat::error_msg(), true);
            return;
        }
        if (ret == NO_EXIST) {
            int len = strlen(buf) + 64;
            char *tt = new char[len];
            snprintf(tt, len, "Can't find file %s.", buf);
            TB()->PopUpMessage(tt, true);
            delete [] tt;
            return;
        }
        TB()->PopUpFileBrowser(buf);
    }
}


void
QTtbDlg::wr_btn_slot()
{
    // The WR button slot, linked to the bug report popup.
    QTtoolbar::tbent_t *tb = QTtoolbar::entries(tid_bug);
    TB()->PopUpBugRpt(tb->dialog() ? MODE_OFF : MODE_ON, tb->x(), tb->y());
}


void
QTtbDlg::run_btn_slot()
{
    // Resume a stopped run.
    CommandTab::com_resume(0);
}


void
QTtbDlg::stop_btn_slot()
{
    // Stop a run in progress.
    ::raise(SIGINT);
}


void
QTtbDlg::font_changed_slot(int fnum)
{
    if (fnum == FNT_SCREEN) {
        QFont *fnt;
        if (Fnt()->getFont(&fnt, fnum)) {
            gd_viewport->set_font(fnt);
            int fw, fh;
            TextExtent(0, &fw, &fh);
            int wid = 40*fw + 6;
            int hei = 6*fh + 6;
            Viewport()->setMinimumSize(QSize(wid, hei));
            Viewport()->resize(wid, hei);
            update();
        }

    }
}


void
QTtbDlg::drag_enter_slot(QDragEnterEvent *ev)
{
    if (ev->mimeData()->hasFormat("text:plain"))
        ev->acceptProposedAction();
}


void
QTtbDlg::drop_slot(QDropEvent *ev)
{
    // Receive a file name in the main window.  If the Source or Load
    // popup is active, put the file name in the popup.  Otherwise,
    // pop up the Source popup loaded with the name.

    if (ev->mimeData()->hasFormat("text:plain")) {
        QByteArray ba = ev->mimeData()->data("text:plain");
        const char *src = ba.constData();
        delete [] tb_dropfile;
        tb_dropfile = 0;
        if (TB()->ActiveInput()) {
            TB()->ActiveInput()->update(0, src);
            return;
        }
        QAction *item = tb_source_btn;
        if (item) {
            tb_dropfile = lstring::copy(src);
            item->activate(QAction::Trigger);
        }
    }
}



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

/***************************************************************************
JSPICE3 adaptation of Spice3e2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1988 Jeffrey M. Hsu
         1995 Stephen R. Whiteley
****************************************************************************/

//
// QT drivers for plotting.
//

#include "qtplot.h"
#include "graph.h"
#include "cshell.h"
#include "kwords_fte.h"
#include "commands.h"
#include "simulator.h"
#include "qttoolb.h"
#include "qtinterf/qtcanvas.h"
#include "qtinterf/qtfont.h"
#include "qtinterf/qtinput.h"
#include "miscutil/pathlist.h"
#include "miscutil/filestat.h"
#include "spnumber/spnumber.h"

#ifndef WIN32
#include "../../icons/wrspice_16x16.xpm"
#include "../../icons/wrspice_32x32.xpm"
#include "../../icons/wrspice_48x48.xpm"
#endif

#include <QApplication>
#include <QLayout>
#include <QPushButton>
#include <QGroupBox>
#include <QLabel>
#include <QResizeEvent>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QEnterEvent>
#include <QFocusEvent>
#include <QDragEnterEvent>
#include <QDropEvent>

// help keywords used in this file
// plotpanel
// mplotpanel


QSize
QTplotDlg::sizeHint() const
{
    int wid, hei;
    if (pb_graph->apptype() == GR_PLOT) {
        wid = 500;
        hei = 300;
    
        variable *v = Sp.GetRawVar(kw_plotgeom);
        if (v) {
            if (v->type() == VTYP_STRING) {
                const char *s = v->string();
                double *d = SPnum.parse(&s, false);
                if (d && *d >= 100.0 && *d <= 2000.0) {
                    wid = (int)*d;
                    while (*s && !isdigit(*s)) s++;
                    d = SPnum.parse(&s, false);
                    if (d && *d >= 100.0 && *d <= 2000.0)
                        hei = (int)*d;
                }
            }
            else if (v->type() == VTYP_LIST) {
                v = v->list();
                bool ok = false;
                if (v->type() == VTYP_NUM && v->integer() >= 100 &&
                        v->integer() <= 2000) {
                    wid = v->integer();
                    ok = true;
                }
                else if (v->type() == VTYP_REAL && v->real() >= 100.0 &&
                        v->real() <= 2000) {
                    wid = (int)v->real();
                    ok = true;
                }
                v = v->next();
                if (ok && v) {
                    if (v->type() == VTYP_NUM && v->integer() >= 100 &&
                            v->integer() <= 2000)
                        hei = v->integer();
                    else if (v->type() == VTYP_REAL && v->real() >= 100.0 &&
                            v->real() <= 2000.0)
                        hei = (int)v->real();
                }
            }
        }
    }
    else {
        wid = 400;
        hei = 300;
    }
    return (QSize(wid, hei));
}


bool
QTplotDlg::init(sGraph *gr)
{
    wb_sens_set = sens_set;
    pb_graph = gr;

    QIcon icon;
    icon.addPixmap(QPixmap(wrs_16x16_xpm));
    icon.addPixmap(QPixmap(wrs_32x32_xpm));
    icon.addPixmap(QPixmap(wrs_48x48_xpm));
    setWindowIcon(icon);

    char buf[128];
    const char *anam = "";
    if (gr->apptype() == GR_PLOT)
        anam = GR_PLOTstr;
    else if (gr->apptype() == GR_MPLT)
        anam = GR_MPLTstr;
    snprintf(buf, sizeof(buf), "%s %s %d", CP.Program(), anam, gr->id());
    setWindowTitle(buf);

    QHBoxLayout *hbox = new QHBoxLayout(this);
    hbox->setContentsMargins(2, 2, 2, 2);
    hbox->setSpacing(2);

    // set up viewport
    gd_viewport = new QTcanvas();
    hbox->addWidget(Viewport());
    Viewport()->setFocusPolicy(Qt::StrongFocus);

    QFont *fnt;
    if (FC.getFont(&fnt, FNT_SCREEN))
        gd_viewport->set_font(fnt);
    connect(QTfont::self(), SIGNAL(fontChanged(int)),
        this, SLOT(font_changed_slot(int)), Qt::QueuedConnection);

    connect(Viewport(), SIGNAL(resize_event(QResizeEvent*)),
        this, SLOT(resize_slot(QResizeEvent*)));
    connect(Viewport(), SIGNAL(press_event(QMouseEvent*)),
        this, SLOT(button_down_slot(QMouseEvent*)));
    connect(Viewport(), SIGNAL(release_event(QMouseEvent*)),
        this, SLOT(button_up_slot(QMouseEvent*)));
    connect(Viewport(), SIGNAL(motion_event(QMouseEvent*)),
        this, SLOT(motion_slot(QMouseEvent*)));
    connect(Viewport(), SIGNAL(key_press_event(QKeyEvent*)),
        this, SLOT(key_down_slot(QKeyEvent*)));
    connect(Viewport(), SIGNAL(enter_event(QEnterEvent*)),
        this, SLOT(enter_slot(QEnterEvent*)));
    connect(Viewport(), SIGNAL(leave_event(QEvent*)),
        this, SLOT(leave_slot(QEvent*)));

    pb_gbox = new QGroupBox();
    hbox->addWidget(pb_gbox);
    pb_gbox->setMaximumWidth(100);

    init_gbuttons();

    hbox->setStretch(0, 1);

    // Position the plot on screen and enable display.
    int xx, yy;
    GP.PlotPosition(&xx, &yy);
    TB()->FixLoc(&xx, &yy);
    move(xx, yy);
    show();

    gr->gr_pkg_init_colors();
    gr->set_fontsize();
    gr->area().set_width(Viewport()->width());
    gr->area().set_height(Viewport()->height());

    Clear();
    pb_graph->gr_redraw();

    return (false);
}


//
// Set up the button array, return true if the button count changes.
//
bool
QTplotDlg::init_gbuttons()
{
    QVBoxLayout *vbox = new QVBoxLayout(pb_gbox);
    vbox->setContentsMargins(0, 0, 0, 0);
    vbox->setSpacing(2);

    // Just rebuild the whole button array, easier that way.
    for (int i = 0; i < pbtn_NUMBTNS; i++)
        pb_checkwins[i] = 0;

    if (!pb_checkwins[pbtn_dismiss]) {
        QPushButton *btn = new QPushButton(tr("Dismiss"));
        vbox->addWidget(btn);
        btn->setToolTip(tr("Delete this window"));
        pb_checkwins[pbtn_dismiss] = btn;
        connect(btn, SIGNAL(clicked()), this, SLOT(dismiss_btn_slot()));
    }

    if (!pb_checkwins[pbtn_help]) {
        QPushButton *btn = new QPushButton(tr("Help"));
        vbox->addWidget(btn);
        btn->setAutoDefault(false);
        btn->setToolTip(tr("Press for help"));
        pb_checkwins[pbtn_help] = btn;
        connect(btn, SIGNAL(clicked()), this, SLOT(help_btn_slot()));
    }

    if (!pb_checkwins[pbtn_redraw]) {
        QPushButton *btn = new QPushButton(tr("Redraw"));
        vbox->addWidget(btn);
        btn->setAutoDefault(false);
        btn->setToolTip(tr("Redraw the plot"));
        pb_checkwins[pbtn_redraw] = btn;
        connect(btn, SIGNAL(clicked()),
            this, SLOT(redraw_btn_slot()));
    }

    if (!pb_checkwins[pbtn_print]) {
        QPushButton *btn = new QPushButton(tr("Print"));
        vbox->addWidget(btn);
        btn->setCheckable(true);
        btn->setAutoDefault(false);
        btn->setToolTip(tr("Print control panel"));
        pb_checkwins[pbtn_print] = btn;
        connect(btn, SIGNAL(toggled(bool)),
            this, SLOT(print_btn_slot(bool)));
    }

    if (pb_graph->apptype() == GR_PLOT) {
        if (!pb_checkwins[pbtn_saveplot]) {
            QPushButton *btn = new QPushButton(tr("Save Plot"));
            vbox->addWidget(btn);
            btn->setAutoDefault(false);
            btn->setToolTip(tr("Save the plot in a plot file"));
            pb_checkwins[pbtn_saveplot] = btn;
            connect(btn, SIGNAL(clicked()),
                this, SLOT(saveplt_btn_slot()));
        }

        if (!pb_checkwins[pbtn_saveprint]) {
            QPushButton *btn = new QPushButton(tr("Save Print "));
            vbox->addWidget(btn);
            btn->setAutoDefault(false);
            btn->setToolTip(tr("Save the plot in a print file"));
            pb_checkwins[pbtn_saveprint] = btn;
            connect(btn, SIGNAL(clicked()),
                this, SLOT(savepr_btn_slot()));
        }

        if (!pb_checkwins[pbtn_points]) {
            QPushButton *btn = new QPushButton(tr("Points"));
            vbox->addWidget(btn);
            btn->setCheckable(true);
            btn->setChecked(pb_graph->plottype() == PLOT_POINT); 
            btn->setAutoDefault(false);
            btn->setToolTip(tr("Plot data as points"));
            pb_checkwins[pbtn_points] = btn;
            connect(btn, SIGNAL(toggled(bool)),
                this, SLOT(ptype_btn_slot(bool)));
        }

        if (!pb_checkwins[pbtn_comb]) {
            QPushButton *btn = new QPushButton(tr("Comb"));
            vbox->addWidget(btn);
            btn->setCheckable(true);
            btn->setChecked(pb_graph->plottype() == PLOT_COMB); 
            btn->setAutoDefault(false);
            btn->setToolTip(tr("Plot data as histogram"));
            pb_checkwins[pbtn_comb] = btn;
            connect(btn, SIGNAL(toggled(bool)),
                this, SLOT(ptype_btn_slot(bool)));
        }

        if (pb_graph->rawdata().xmin > 0) {
            if (!pb_checkwins[pbtn_logx]) {
                QPushButton *btn = new QPushButton(tr("Log X"));
                vbox->addWidget(btn);
                btn->setCheckable(true);
                btn->setChecked(pb_graph->gridtype() == GRID_XLOG ||
                    pb_graph->gridtype() == GRID_LOGLOG);
                btn->setAutoDefault(false);
                btn->setToolTip(tr(
                    "Use logarithmic horizontal scale"));
                pb_checkwins[pbtn_logx] = btn;
                connect(btn, SIGNAL(toggled(bool)),
                    this, SLOT(logx_btn_slot(bool)));
            }
        }
        else {
            if (pb_graph->gridtype() == GRID_LOGLOG)
                pb_graph->set_gridtype(GRID_YLOG);
            else if (pb_graph->gridtype() == GRID_XLOG)
                pb_graph->set_gridtype(GRID_LIN);
        }

        if (pb_graph->rawdata().ymin > 0) {
            if (!pb_checkwins[pbtn_logy]) {
                QPushButton *btn = new QPushButton(tr("Log Y"));
                vbox->addWidget(btn);
                btn->setCheckable(true);
                btn->setChecked(pb_graph->gridtype() == GRID_YLOG ||
                    pb_graph->gridtype() == GRID_LOGLOG);
                btn->setAutoDefault(false);
                btn->setToolTip(tr(
                    "Use logarithmic vertical scale"));
                pb_checkwins[pbtn_logy] = btn;
                connect(btn, SIGNAL(toggled(bool)),
                    this, SLOT(logy_btn_slot(bool)));
            }
        }
        else {
            if (pb_graph->gridtype() == GRID_LOGLOG)
                pb_graph->set_gridtype(GRID_XLOG);
            else if (pb_graph->gridtype() == GRID_YLOG)
                pb_graph->set_gridtype(GRID_LIN);
        }

        if (!pb_checkwins[pbtn_marker]) {
            QPushButton *btn = new QPushButton(tr("Marker"));
            vbox->addWidget(btn);
            btn->setCheckable(true);
            btn->setChecked(pb_graph->reference().mark);
            btn->setAutoDefault(false);
            btn->setToolTip(tr("Show marker"));
            pb_checkwins[pbtn_marker] = btn;
            connect(btn, SIGNAL(toggled(bool)),
                this, SLOT(marker_btn_slot(bool)));
        }

        if (pb_graph->numtraces() > 1 && pb_graph->numtraces() <= MAXNUMTR) {
            if (!pb_checkwins[pbtn_separate]) {
                QPushButton *btn = new QPushButton(tr("Separate"));
                vbox->addWidget(btn);
                btn->setCheckable(true);
                btn->setChecked(pb_graph->yseparate());
                btn->setAutoDefault(false);
                btn->setToolTip(tr("Show traces separately"));
                pb_checkwins[pbtn_separate] = btn;
                connect(btn, SIGNAL(toggled(bool)),
                    this, SLOT(separate_btn_slot(bool)));
            }

            int i = 0;
            for (sDvList *link = (sDvList*)pb_graph->plotdata(); link;
                    link = link->dl_next, i++) {
                if (!(*((sDvList *)pb_graph->plotdata())->dl_dvec->units() ==
                        *link->dl_dvec->units())) {
                    i = 0;
                    break;
                }
            }
            if (i > 1) {
                // more than one trace, same type
                if (!pb_checkwins[pbtn_single]) {
                    QPushButton *btn = new QPushButton(tr("Single"));
                    vbox->addWidget(btn);
                    btn->setCheckable(true);
                    btn->setChecked(pb_graph->format() == FT_SINGLE);
                    btn->setAutoDefault(false);
                    btn->setToolTip(tr(
                        "Use single vertical scale"));
                    pb_checkwins[pbtn_single] = btn;
                    connect(btn, SIGNAL(toggled(bool)),
                        this, SLOT(single_btn_slot(bool)));
                }
            }
            else if (!i) {
                // more than one trace type
                if (!pb_checkwins[pbtn_single]) {
                    QPushButton *btn = new QPushButton(tr("Single"));
                    vbox->addWidget(btn);
                    btn->setCheckable(true);
                    btn->setChecked(pb_graph->format() == FT_SINGLE);
                    btn->setAutoDefault(false);
                    btn->setToolTip(tr( 
                        "Use single vertical scale"));
                    pb_checkwins[pbtn_single] = btn;
                    connect(btn, SIGNAL(toggled(bool)),
                        this, SLOT(single_btn_slot(bool)));
                }

                if (!pb_checkwins[pbtn_group]) {
                    QPushButton *btn = new QPushButton(tr("Group"));
                    vbox->addWidget(btn);
                    btn->setCheckable(true);
                    btn->setChecked(pb_graph->format() == FT_GROUP);
                    btn->setAutoDefault(false);
                    btn->setToolTip(tr(
                        "Same scale for groups: V, I, other"));
                    pb_checkwins[pbtn_group] = btn;
                    connect(btn, SIGNAL(toggled(bool)),
                        this, SLOT(group_btn_slot(bool)));
                }
            }
        }
    }
    return (false);
}


bool
QTplotDlg::event(QEvent *ev)
{
    if (pb_event_test && check_event(ev)) {
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
        QApplication::postEvent(this, ev->clone());
#else
        // XXX fixme, no clone in QT5
#endif
        pb_event_deferred = true;
        return (true);
    }
    return (QWidget::event(ev));
}


// Core event test.
//
bool
QTplotDlg::check_event(QEvent *ev)
{
    if (ev->type() == QEvent::Resize) {
        return (true);
    }
    else if (ev->type() == QEvent::MouseButtonPress) {
/*XXX
        // This is the magic to get a widget from a window.
        GtkWidget *widget = 0;
        gdk_window_get_user_data(ev->button.window, (void**)&widget);
        if (widget) {
            if (widget == w->pb_checkwins[pbtn_dismiss])
                return (true);
            if (widget == w->pb_checkwins[pbtn_points])
                return (true);
            if (widget == w->pb_checkwins[pbtn_comb])
                return (true);
            if (widget == w->pb_checkwins[pbtn_logx])
                return (true);
            if (widget == w->pb_checkwins[pbtn_logy])
                return (true);
            if (widget == w->pb_checkwins[pbtn_separate])
                return (true);
            if (widget == w->pb_checkwins[pbtn_single])
                return (true);
            if (widget == w->pb_checkwins[pbtn_group])
                return (true);
        }
*/
    }
    return (false);
}


// Static function.
// Callback for PopUpInput() sensitivity set.
//
void
QTplotDlg::sens_set(QTbag *w, bool set, int)
{
    if (w->ActiveInput()) {
        QTledDlg *qw = static_cast<QTledDlg*>(w->ActiveInput());
        if (qw)
            qw->setEnabled(set);
    }
}


// Static function.
// The redraw is in a timeout so that plots with a lot of data and
// take time to render can be resized more easily.
int
QTplotDlg::redraw_timeout(void *arg)
{
    QTplotDlg *w = static_cast<QTplotDlg*>(arg);

    w->pb_rdid = 0;
    w->pb_graph->gr_redraw();
    return (0);
}


// Static function.
// Idle function to handle motion events.  The events are filtered to
// avoid cursor lag.
//
int
QTplotDlg::motion_idle(void *arg)
{
    QTplotDlg *w = static_cast<QTplotDlg*>(arg);
    if (w && w->ShowingGhost()) {
        w->pb_id = 0;
        GP.PushGraphContext(w->pb_graph);
        w->UndrawGhost();
        w->DrawGhost(w->pb_x, w->pb_y);
        GP.PopGraphContext();
    }
    return (0);
}


// Static function.
void
QTplotDlg::do_save_plot(const char *fnamein, void *arg)
{
    QTplotDlg *w = static_cast<QTplotDlg*>(arg);
    char *fname = pathlist::expand_path(fnamein, false, true);
    if (!fname)
        return;
    if (filestat::check_file(fname, W_OK) == NOGO) {
        w->PopUpMessage(filestat::error_msg(), true);
        delete [] fname;
        return;
    }
    wordlist wl;
    wl.wl_word = fname;
    wl.wl_next = w->pb_graph->command();
    w->pb_graph->command()->wl_prev = &wl;
    CommandTab::com_write(&wl);
    w->pb_graph->command()->wl_prev = 0;

    if (w->wb_input)
        w->wb_input->popdown();
    w->PopUpMessage("Operation completed.", false);
    delete [] fname;
}


// Static function.
void
QTplotDlg::do_save_print(const char *fnamein, void *arg)
{
    QTplotDlg *w = static_cast<QTplotDlg*>(arg);
    char *fname = pathlist::expand_path(fnamein, false, true);
    if (!fname)
        return;
    if (filestat::check_file(fname, W_OK) == NOGO) {
        w->PopUpMessage(filestat::error_msg(), true);
        delete [] fname;
        return;
    }
    FILE *fp = fopen(fname, "w");
    if (fp) {
        TTY.ioRedirect(0, fp, 0);
        CommandTab::com_print(w->pb_graph->command());
        TTY.ioReset();
    }
    else
        w->PopUpErr(MODE_ON, "Can't open file.");
    if (w->wb_input)
        w->wb_input->popdown();
    if (fp)
        w->PopUpMessage("Plot saved to file as print data.", false);
    delete [] fname;
}


// Static function.
// Procedure to initialize part of the structure passed to the
// hardcopy widget.
//
void
QTplotDlg::set_hccb(HCcb *cb)
{
    if (cb->command)
        delete [] cb->command;
    cb->command = GRappIf()->GetPrintCmd();

    HCdesc *hcdesc = 0;
    variable *v = Sp.GetRawVar(kw_hcopydriver);
    if (v && v->type() == VTYP_STRING) {
        int i = GRpkg::self()->FindHCindex(v->string());
        if (i >= 0) {
            hcdesc = GRpkg::self()->HCof(i);
            cb->format = i;
        }
    }
    if (!hcdesc)
        hcdesc = GRpkg::self()->HCof(cb->format);
    if (!hcdesc)
        return;

    VTvalue vv;
    if (Sp.GetVar(kw_hcopyresol, VTYP_STRING, &vv)) {
        int r;
        if (sscanf(vv.get_string(), "%d", &r) == 1) {
            for (int j = 0; hcdesc->limits.resols[j]; j++) {
                if (r == atoi(hcdesc->limits.resols[j])) {
                    cb->resolution = r;
                    break;
                }
            }
        }
    }
    double d;
    if (Sp.GetVar(kw_hcopywidth, VTYP_STRING, &vv)) {
        if (get_dim(vv.get_string(), &d) &&
                d >= hcdesc->limits.minwidth &&
                d <= hcdesc->limits.maxwidth)
            cb->width = d;
    }
    if (Sp.GetVar(kw_hcopyheight, VTYP_STRING, &vv)) {
        if (get_dim(vv.get_string(), &d) &&
                d >= hcdesc->limits.minheight &&
                d <= hcdesc->limits.maxheight)
            cb->height = d;
    }
    if (Sp.GetVar(kw_hcopyxoff, VTYP_STRING, &vv)) {
        if (get_dim(vv.get_string(), &d) &&
                d >= hcdesc->limits.minxoff &&
                d <= hcdesc->limits.maxxoff)
            cb->left = d;
    }
    if (Sp.GetVar(kw_hcopyyoff, VTYP_STRING, &vv)) {
        if (get_dim(vv.get_string(), &d) &&
                d >= hcdesc->limits.minyoff &&
                d <= hcdesc->limits.maxyoff)
            cb->top = d;
    }
    if (Sp.GetVar(kw_hcopylandscape, VTYP_BOOL, 0))
        cb->orient = HClandscape;
}


// Static function.
// Return val in inches and true if ok, return false otherwise.  str
// is a floating point number possibly followed by "cm".
//
bool
QTplotDlg::get_dim(const char *str, double *val)
{
    char buf[128];
    int i = sscanf(str, "%lf%s", val, buf);
    if (i == 2 && buf[0] == 'c' && buf[1] == 'm')
        *val *= 2.54;
    if (i > 0)
        return (true);
    return (false);
}


void
QTplotDlg::font_changed_slot(int fnum)
{
    if (fnum == FNT_SCREEN) {
        QFont *fnt;
        if (FC.getFont(&fnt, fnum)) {
            gd_viewport->set_font(fnt);
            pb_graph->set_dirty(true);
        }
    }
}


void
QTplotDlg::resize_slot(QResizeEvent*)
{
    pb_graph->area().set_width(Viewport()->width());
    pb_graph->area().set_height(Viewport()->height());
    pb_graph->gr_abort();
    pb_graph->gr_pkg_init_colors();
    Clear();
    if (pb_rdid)
        QTdev::self()->RemoveTimer(pb_rdid);
    pb_rdid = QTdev::self()->AddTimer(250, redraw_timeout, this);
}


void
QTplotDlg::button_down_slot(QMouseEvent *ev)
{
    if (GRpkg::self()->CurDev()->devtype == GRhardcopy) {
        ev->ignore();
        return;
    }
    if (ev->type() != QEvent::MouseButtonPress) {
        ev->ignore();
        return;
    }
    ev->accept();

    int button = 0;
    if (ev->button() == Qt::LeftButton)
        button = 1;
    else if (ev->button() == Qt::MiddleButton)
        button = 2;
    else if (ev->button() == Qt::RightButton)
        button = 3;

    int state = ev->modifiers();

    if (state & Qt::ShiftModifier)
        pb_graph->set_cmdmode(pb_graph->cmdmode() | grShiftMode);
    else
        pb_graph->set_cmdmode(pb_graph->cmdmode() & ~grShiftMode);
    if (state & Qt::ControlModifier)
        pb_graph->set_cmdmode(pb_graph->cmdmode() | grControlMode);
    else
        pb_graph->set_cmdmode(pb_graph->cmdmode() & ~grControlMode);

#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    pb_graph->gr_bdown_hdlr(button, ev->position().x(),
        pb_graph->yinv(ev->position().y()));
#else
    pb_graph->gr_bdown_hdlr(button, ev->x(), pb_graph->yinv(ev->y()));
#endif
}


void
QTplotDlg::button_up_slot(QMouseEvent *ev)
{
    if (GRpkg::self()->CurDev()->devtype == GRhardcopy) {
        ev->ignore();
        return;
    }
    if (ev->type() != QEvent::MouseButtonRelease) {
        ev->ignore();
        return;
    }
    ev->accept();

    int button = 0;
    if (ev->button() == Qt::LeftButton)
        button = 1;
    else if (ev->button() == Qt::MiddleButton)
        button = 2;
    else if (ev->button() == Qt::RightButton)
        button = 3;

    int state = ev->modifiers();

    if (state & Qt::ShiftModifier)
        pb_graph->set_cmdmode(pb_graph->cmdmode() | grShiftMode);
    else
        pb_graph->set_cmdmode(pb_graph->cmdmode() & ~grShiftMode);
    if (state & Qt::ControlModifier)
        pb_graph->set_cmdmode(pb_graph->cmdmode() | grControlMode);
    else
        pb_graph->set_cmdmode(pb_graph->cmdmode() & ~grControlMode);

#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    pb_graph->gr_bup_hdlr(button, ev->position().x(),
        pb_graph->yinv(ev->position().y()));
#else
    pb_graph->gr_bup_hdlr(button, ev->x(), pb_graph->yinv(ev->y()));
#endif
}


void
QTplotDlg::motion_slot(QMouseEvent *ev)
{
    if (ShowingGhost()) {
        if (pb_id) {
            QTdev::self()->RemoveIdleProc(pb_id);
            pb_id = 0;
        }
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
        pb_x = ev->position().x();
        pb_y = ev->position().y();
#else
        pb_x = ev->x();
        pb_y = ev->y();
#endif
        pb_id = QTdev::self()->AddIdleProc(motion_idle, this);
    }
}


void
QTplotDlg::key_down_slot(QKeyEvent *ev)
{
    if (ev->type() != QEvent::KeyPress) {
        ev->ignore();
        return;
    }
    if (!underMouse()) {
        // Throw out when cursor is not in viewport.
        ev->ignore();
        return;
    }
    ev->accept();

    int code = 0;
    switch (ev->key()) {
    case Qt::Key_Escape:
    case Qt::Key_Tab:
        // Tab will switch focus.
        return;

    case Qt::Key_Up:
        code = UP_KEY;
        break;
    case Qt::Key_Down:
        code = DOWN_KEY;
        break;
    case Qt::Key_Left:
        code = LEFT_KEY;
        break;
    case Qt::Key_Right:
        code = RIGHT_KEY;
        break;
    case Qt::Key_Return:
        code = ENTER_KEY;
        break;
    case Qt::Key_Backspace:
        code = BSP_KEY;
        break;
    case Qt::Key_Delete:
        code = DELETE_KEY;
        break;
    }

    QByteArray qba = ev->text().toLatin1();
    const char *string = qba.constData();

    QPoint pt = mapFromGlobal(QCursor::pos());

    gd_viewport->set_overlay_mode(true);
    pb_graph->gr_key_hdlr(string, code, pt.x(), pb_graph->yinv(pt.y()));
    gd_viewport->set_overlay_mode(false);
}


void
QTplotDlg::enter_slot(QEnterEvent*)
{
    // Pointer entered a drawing window.
    if (GP.SourceGraph() && pb_graph != GP.SourceGraph()) {
        // hack a ghost for move/copy
//        DrawIf()->set_ghost_func(dynamic_cast<QTplotDlg*>(
//            GP.SourceGraph()->dev())->DrawIf()->get_ghost_func());
    }
//    ShowGhost(true);
}


void
QTplotDlg::leave_slot(QEvent*)
{
    // Pointer left the drawing window.
    if (Viewport()->has_ghost()) {
        GP.PushGraphContext(pb_graph);
//XXX        ShowGhost(false);
        GP.PopGraphContext();
    }
    if (GP.SourceGraph() && pb_graph != GP.SourceGraph()) {
        // remove hacked ghost
//XXX        Viewport()->set_ghost_func(0);
    }
}


void
QTplotDlg::dismiss_btn_slot()
{
    pb_graph->gr_popdown();
}


void
QTplotDlg::help_btn_slot()
{
    PopUpHelp(pb_graph->apptype() == GR_PLOT ? "plotpanel" : "mplotpanel");
}


void
QTplotDlg::redraw_btn_slot()
{
    pb_graph->gr_pkg_init_colors();
    Clear();
    pb_graph->gr_redraw();
}


void
QTplotDlg::print_btn_slot(bool)
{
    set_hccb(&wrsHCcb);
    PopUpPrint(sender(), &wrsHCcb, HCgraphical, this);
}


void
QTplotDlg::saveplt_btn_slot()
{
    PopUpInput("Enter name for data file", 0, "Save Plot File",
        do_save_plot, this);
}


void
QTplotDlg::savepr_btn_slot()
{
    PopUpInput("Enter name for print file", 0, "Save Print File",
        do_save_print, this);
}


void
QTplotDlg::ptype_btn_slot(bool)
{
    // handle points and comb modes
    QPushButton *pts = pb_checkwins[pbtn_points];
    QPushButton *cmb = pb_checkwins[pbtn_comb];
    if (pts && cmb) {
        bool ptson = QTdev::GetStatus(pts);
        bool cmbon = QTdev::GetStatus(cmb);

        if (!ptson && !cmbon)
            pb_graph->set_plottype(PLOT_LIN);
        else if (sender() == pts) {
            if (cmbon)
                QTdev::SetStatus(cmb, false);
            pb_graph->set_plottype(PLOT_POINT);
        }
        else {
            if (ptson)
                QTdev::SetStatus(pts, false);
            pb_graph->set_plottype(PLOT_COMB);
        }
        Clear();
        pb_graph->gr_redraw();
    }
}


void
QTplotDlg::logx_btn_slot(bool)
{
    if (pb_graph->rawdata().xmin <= 0) {
        PopUpErr(MODE_ON,
            "The X-axis scale must be greater than zero for log plot.");
        QTdev::SetStatus(sender(), false);
        return;
    }
    bool state = QTdev::GetStatus(sender());
    if (state) {
        if (pb_graph->gridtype() == GRID_YLOG)
            pb_graph->set_gridtype(GRID_LOGLOG);
        else
            pb_graph->set_gridtype(GRID_XLOG);
    }
    else {
        if (pb_graph->gridtype() == GRID_LOGLOG)
            pb_graph->set_gridtype(GRID_YLOG);
        else
            pb_graph->set_gridtype(GRID_LIN);
    }

    pb_graph->clear_units_text(LAxunits);
    Clear();
    pb_graph->gr_redraw();
}


void
QTplotDlg::logy_btn_slot(bool)
{
    if (pb_graph->rawdata().ymin <= 0) {
        PopUpErr(MODE_ON,
            "The Y-axis scale must be greater than zero for log plot.");
        QTdev::SetStatus(sender(), false);
        return;
    }
    bool state = QTdev::GetStatus(sender());
    if (state) {
        if (pb_graph->gridtype() == GRID_XLOG)
            pb_graph->set_gridtype(GRID_LOGLOG);
        else
            pb_graph->set_gridtype(GRID_YLOG);
    }
    else {
        if (pb_graph->gridtype() == GRID_LOGLOG)
            pb_graph->set_gridtype(GRID_XLOG);
        else
            pb_graph->set_gridtype(GRID_LIN);
    }

    pb_graph->clear_units_text(LAyunits);
    Clear();
    pb_graph->gr_redraw();
}


void
QTplotDlg::marker_btn_slot(bool state)
{
    if (!pb_graph->xmonotonic() && pb_graph->yseparate()) {
        PopUpErr(MODE_ON,
"Marker is not available in separate trace mode with nonmonotonic scale.");
        QTdev::SetStatus(sender(), false);
        return;
    }
    if (state) {
        if (!pb_graph->reference().mark) {
            pb_graph->reference().mark = true;
            pb_graph->gr_mark();
        }
    }
    else if (pb_graph->reference().mark) {
        pb_graph->reference().mark = false;
        pb_graph->gr_mark();
        pb_graph->reference().set = false;
        pb_graph->dev()->Update();
    }
}


void
QTplotDlg::separate_btn_slot(bool)
{
    if (!pb_graph->xmonotonic() && pb_graph->reference().mark) {
        PopUpErr(MODE_ON,
"Can't enter separate trace mode with nonmonotonic scale when\n\
marker is active.");
        QTdev::SetStatus(sender(), false);
        return;
    }
    if (pb_graph->yseparate())
        pb_graph->set_yseparate(false);
    else
        pb_graph->set_yseparate(true);
    Clear();
    pb_graph->gr_redraw();
}


void
QTplotDlg::single_btn_slot(bool)
{
    if (pb_graph->format() == FT_MULTI)
        pb_graph->set_format(FT_SINGLE);
    else
        pb_graph->set_format(FT_MULTI);
    Clear();
    pb_graph->gr_redraw();
}


void
QTplotDlg::group_btn_slot(bool)
{
    // Handle one scale and grp scale modes.
    QPushButton *one = pb_checkwins[pbtn_single];
    QPushButton *grp = pb_checkwins[pbtn_group];
    if (one && grp) {
        bool oneon = QTdev::GetStatus(one);
        bool grpon = QTdev::GetStatus(grp);

        if (!oneon && !grpon)
            pb_graph->set_format(FT_MULTI);
        else if (sender() == one) {
            if (grpon)
                QTdev::SetStatus(grp, false);
            pb_graph->set_format(FT_SINGLE);
        }
        else {
            if (oneon)
                QTdev::SetStatus(one, false);
            pb_graph->set_format(FT_GROUP);
        }
        pb_graph->dev()->Clear();
        pb_graph->gr_redraw();
    }
}


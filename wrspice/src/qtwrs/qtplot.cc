
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

#include "../../icons/wrspice_16x16.xpm"
#include "../../icons/wrspice_32x32.xpm"
#include "../../icons/wrspice_48x48.xpm"

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
#include <QDrag>
#include <QMimeData>
#include <QTimer>

// help keywords used in this file
// plotpanel
// mplotpanel


QTplotDlg::QTplotDlg(int type, cGraph *gr) : QTbag(this), QTdraw(type)
{
    pb_graph = gr;
    pb_gbox = 0;
    for (int i = 0; i < pbtn_NUMBTNS; i++)
        pb_checkwins[i] = 0;
    pb_id = 0;
    pb_x = pb_y = 0;
    pb_rdid = 0;
    pb_event_test = false;
    pb_event_deferred = false;
    setAttribute(Qt::WA_DeleteOnClose);

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
    hbox->addWidget(gd_viewport);
    gd_viewport->setFocusPolicy(Qt::StrongFocus);
    gd_viewport->setAcceptDrops(true);

    QFont *fnt;
    if (Fnt()->getFont(&fnt, FNT_SCREEN))
        gd_viewport->set_font(fnt);
    connect(QTfont::self(), &QTfont::fontChanged,
        this, &QTplotDlg::font_changed_slot, Qt::QueuedConnection);

    connect(gd_viewport, &QTcanvas::resize_event,
        this, &QTplotDlg::resize_slot);
    connect(gd_viewport, &QTcanvas::press_event,
        this, &QTplotDlg::button_down_slot);
    connect(gd_viewport, &QTcanvas::release_event,
        this, &QTplotDlg::button_up_slot);
    connect(gd_viewport, &QTcanvas::motion_event,
        this, &QTplotDlg::motion_slot);
    connect(gd_viewport, &QTcanvas::key_press_event,
        this, &QTplotDlg::key_down_slot);
    connect(gd_viewport, &QTcanvas::drag_enter_event,
        this, &QTplotDlg::drag_enter_slot);
    connect(gd_viewport, &QTcanvas::drop_event,
        this, &QTplotDlg::drop_slot);

    pb_gbox = new QGroupBox();
    hbox->addWidget(pb_gbox);
    pb_gbox->setMaximumWidth(100);
    init_gbuttons();
    hbox->setStretch(0, 1);
}


QTplotDlg::~QTplotDlg()
{
    if (pb_id)
        QTdev::self()->RemoveIdleProc(pb_id);
}



QSize
QTplotDlg::sizeHint() const
{
    int wid, hei;
    if (pb_graph->apptype() == GR_PLOT) {
        wid = 500;
        hei = pb_graph->gr_win_ht(300);

    
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
QTplotDlg::init(cGraph*)
{
    // Position the plot on screen and enable display.
    int xx, yy;
    GP.PlotPosition(&xx, &yy);
    TB()->FixLoc(&xx, &yy);
    move(xx, yy);
    show();

    pb_graph->gr_pkg_init_colors();
    pb_graph->set_fontsize();
    pb_graph->area().set_width(Viewport()->width());
    pb_graph->area().set_height(Viewport()->height());

    Clear();
    pb_graph->gr_redraw();

    if (isatty(fileno(stdin))) {
        // Start out with the pop-up on top, but revert focus to console
        // if possible.
        setWindowFlag(Qt::WindowStaysOnTopHint);
#ifdef __APPLE__
        QTimer::singleShot(100, this, &QTplotDlg::revert_focus);
#else
        setAttribute(Qt::WA_ShowWithoutActivating);
        QTimer::singleShot(0, this, &QTplotDlg::revert_focus);
#endif
    }

    return (false);
}


// Set up the button array, return true if the button count changes.
//
bool
QTplotDlg::init_gbuttons()
{
    // Just rebuild the whole button array, easier that way.

    QObjectList ol = pb_gbox->children();
    int sz = ol.size();
    for (int i = 0; i < sz; i++)
        delete ol[i];
    delete pb_gbox->layout();
    QVBoxLayout *vbox = new QVBoxLayout(pb_gbox);
    vbox->setContentsMargins(0, 0, 0, 0);
    vbox->setSpacing(2);

    for (int i = 0; i < pbtn_NUMBTNS; i++)
        pb_checkwins[i] = 0;

    if (!pb_checkwins[pbtn_dismiss]) {
        QPushButton *btn = new QPushButton(tr("Dismiss"));
        btn->installEventFilter(this);
        vbox->addWidget(btn);
        btn->setToolTip(tr("Delete this window"));
        pb_checkwins[pbtn_dismiss] = btn;
        connect(btn, &QAbstractButton::clicked,
            this, &QTplotDlg::dismiss_btn_slot);
    }

    if (!pb_checkwins[pbtn_help]) {
        QPushButton *btn = new QPushButton(tr("Help"));
        vbox->addWidget(btn);
        btn->setAutoDefault(false);
        btn->setToolTip(tr("Press for help"));
        pb_checkwins[pbtn_help] = btn;
        connect(btn, &QAbstractButton::clicked,
            this, &QTplotDlg::help_btn_slot);
    }

    if (!pb_checkwins[pbtn_redraw]) {
        QPushButton *btn = new QPushButton(tr("Redraw"));
        btn->installEventFilter(this);
        vbox->addWidget(btn);
        btn->setAutoDefault(false);
        btn->setToolTip(tr("Redraw the plot"));
        pb_checkwins[pbtn_redraw] = btn;
        connect(btn, &QAbstractButton::clicked,
            this, &QTplotDlg::redraw_btn_slot);
    }

    if (!pb_checkwins[pbtn_print]) {
        QPushButton *btn = new QPushButton(tr("Print"));
        vbox->addWidget(btn);
        btn->setCheckable(true);
        btn->setAutoDefault(false);
        btn->setToolTip(tr("Print control panel"));
        pb_checkwins[pbtn_print] = btn;
        connect(btn, &QAbstractButton::toggled,
            this, &QTplotDlg::print_btn_slot);
    }

    if (pb_graph->apptype() == GR_PLOT) {
        if (!pb_checkwins[pbtn_saveplot]) {
            QPushButton *btn = new QPushButton(tr("Save Plot"));
            vbox->addWidget(btn);
            btn->setAutoDefault(false);
            btn->setToolTip(tr("Save the plot in a plot file"));
            pb_checkwins[pbtn_saveplot] = btn;
            connect(btn, &QAbstractButton::clicked,
                this, &QTplotDlg::saveplt_btn_slot);
        }

        if (!pb_checkwins[pbtn_saveprint]) {
            QPushButton *btn = new QPushButton(tr("Save Print "));
            vbox->addWidget(btn);
            btn->setAutoDefault(false);
            btn->setToolTip(tr("Save the plot in a print file"));
            pb_checkwins[pbtn_saveprint] = btn;
            connect(btn, &QAbstractButton::clicked,
                this, &QTplotDlg::savepr_btn_slot);
        }

        if (!pb_checkwins[pbtn_points]) {
            QPushButton *btn = new QPushButton(tr("Points"));
            btn->installEventFilter(this);
            vbox->addWidget(btn);
            btn->setCheckable(true);
            btn->setChecked(pb_graph->plottype() == PLOT_POINT); 
            btn->setAutoDefault(false);
            btn->setToolTip(tr("Plot data as points"));
            pb_checkwins[pbtn_points] = btn;
            connect(btn, &QAbstractButton::toggled,
                this, &QTplotDlg::ptype_btn_slot);
        }

        if (!pb_checkwins[pbtn_comb]) {
            QPushButton *btn = new QPushButton(tr("Comb"));
            btn->installEventFilter(this);
            vbox->addWidget(btn);
            btn->setCheckable(true);
            btn->setChecked(pb_graph->plottype() == PLOT_COMB); 
            btn->setAutoDefault(false);
            btn->setToolTip(tr("Plot data as histogram"));
            pb_checkwins[pbtn_comb] = btn;
            connect(btn, &QAbstractButton::toggled,
                this, &QTplotDlg::ptype_btn_slot);
        }

        if (pb_graph->rawdata().xmin > 0) {
            if (!pb_checkwins[pbtn_logx]) {
                QPushButton *btn = new QPushButton(tr("Log X"));
                btn->installEventFilter(this);
                vbox->addWidget(btn);
                btn->setCheckable(true);
                btn->setChecked(pb_graph->gridtype() == GRID_XLOG ||
                    pb_graph->gridtype() == GRID_LOGLOG);
                btn->setAutoDefault(false);
                btn->setToolTip(tr(
                    "Use logarithmic horizontal scale"));
                pb_checkwins[pbtn_logx] = btn;
                connect(btn, &QAbstractButton::toggled,
                    this, &QTplotDlg::logx_btn_slot);
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
                btn->installEventFilter(this);
                vbox->addWidget(btn);
                btn->setCheckable(true);
                btn->setChecked(pb_graph->gridtype() == GRID_YLOG ||
                    pb_graph->gridtype() == GRID_LOGLOG);
                btn->setAutoDefault(false);
                btn->setToolTip(tr(
                    "Use logarithmic vertical scale"));
                pb_checkwins[pbtn_logy] = btn;
                connect(btn, &QAbstractButton::toggled,
                    this, &QTplotDlg::logy_btn_slot);
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
            connect(btn, &QAbstractButton::toggled,
                this, &QTplotDlg::marker_btn_slot);
        }

        if (pb_graph->numtraces() > 1 && pb_graph->numtraces() <= MAXNUMTR) {
            if (!pb_checkwins[pbtn_separate]) {
                QPushButton *btn = new QPushButton(tr("Separate"));
                btn->installEventFilter(this);
                vbox->addWidget(btn);
                btn->setCheckable(true);
                btn->setChecked(pb_graph->yseparate());
                btn->setAutoDefault(false);
                btn->setToolTip(tr("Show traces separately"));
                pb_checkwins[pbtn_separate] = btn;
                connect(btn, &QAbstractButton::toggled,
                    this, &QTplotDlg::separate_btn_slot);
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
                    btn->installEventFilter(this);
                    vbox->addWidget(btn);
                    btn->setCheckable(true);
                    btn->setChecked(pb_graph->format() == FT_SINGLE);
                    btn->setAutoDefault(false);
                    btn->setToolTip(tr(
                        "Use single vertical scale"));
                    pb_checkwins[pbtn_single] = btn;
                    connect(btn, &QAbstractButton::toggled,
                        this, &QTplotDlg::single_btn_slot);
                }
            }
            else if (!i) {
                // more than one trace type
                if (!pb_checkwins[pbtn_single]) {
                    QPushButton *btn = new QPushButton(tr("Single"));
                    btn->installEventFilter(this);
                    vbox->addWidget(btn);
                    btn->setCheckable(true);
                    btn->setChecked(pb_graph->format() == FT_SINGLE);
                    btn->setAutoDefault(false);
                    btn->setToolTip(tr( 
                        "Use single vertical scale"));
                    pb_checkwins[pbtn_single] = btn;
                    connect(btn, &QAbstractButton::toggled,
                        this, &QTplotDlg::single_btn_slot);
                }

                if (!pb_checkwins[pbtn_group]) {
                    QPushButton *btn = new QPushButton(tr("Group"));
                    btn->installEventFilter(this);
                    vbox->addWidget(btn);
                    btn->setCheckable(true);
                    btn->setChecked(pb_graph->format() == FT_GROUP);
                    btn->setAutoDefault(false);
                    btn->setToolTip(tr(
                        "Same scale for groups: V, I, other"));
                    pb_checkwins[pbtn_group] = btn;
                    connect(btn, &QAbstractButton::toggled,
                        this, &QTplotDlg::group_btn_slot);
                }
            }
        }
    }
    return (false);
}


bool
QTplotDlg::event(QEvent *ev)
{
    // When event testing, defer resize events.  The widget will stop
    // drawing and then handle the event.

    if (ev->type() == QEvent::Resize) {
        if (pb_event_test) {
            // Might need ev->clone() here, but this exists only in QT6.
            QApplication::postEvent(this, ev);
            pb_event_deferred = true;
            return (true);
        }
    }
    return (QWidget::event(ev));
}


bool
QTplotDlg::eventFilter(QObject *obj, QEvent *ev)
{
    // When event testing, defer certain button presses.  The widget
    // will stop drawing and then handle the event.

    if (ev->type() == QEvent::MouseButtonPress ||
            ev->type() == QEvent::MouseButtonRelease) {
        if (pb_event_test) {
            // Might need ev->clone() here, but this exists only in QT6.
            QApplication::postEvent(obj, ev);
            pb_event_deferred = true;
            return (true);
        }
    }
    return (false);
}


// Revert focus to the starting console.
// //
void
QTplotDlg::revert_focus()
{
#ifdef __APPLE__
    system(
        "osascript -e \"tell application \\\"Terminal\\\" to activate\"");
    setWindowFlags(windowFlags() & ~Qt::WindowStaysOnTopHint);
#else
    setWindowFlags(windowFlags() & ~Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_ShowWithoutActivating, false);
#endif
    show();
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
        if (Fnt()->getFont(&fnt, fnum)) {
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
    PopUpInput("Enter name for data file", 0, "Save Plot File", 0, 0);
    connect(wb_input, &QTledDlg::action_call,
        this, &QTplotDlg::do_save_plot_slot);
}


void
QTplotDlg::savepr_btn_slot()
{
    PopUpInput("Enter name for print file", 0, "Save Print File", 0, 0);
    connect(wb_input, &QTledDlg::action_call,
        this, &QTplotDlg::do_save_print_slot);
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


void
QTplotDlg::drag_enter_slot(QDragEnterEvent *ev)
{
    if (GRpkg::self()->CurDev()->devtype == GRhardcopy) {
        ev->ignore();
        return;
    }
    if (ev->mimeData()->hasFormat(MIME_TYPE_TRACE) ||
            ev->mimeData()->hasFormat("text/plain")) {
        if (sender() == this)
            ev->setDropAction(Qt::MoveAction);
        else
            ev->setDropAction(Qt::CopyAction);
        ev->accept();
    }
    else {
        ev->ignore();
        return;
    }
}


void
QTplotDlg::drop_slot(QDropEvent *ev)
{
    if (GRpkg::self()->CurDev()->devtype == GRhardcopy) {
        ev->ignore();
        return;
    }
    if (ev->mimeData()->hasFormat(MIME_TYPE_TRACE) ||
            ev->mimeData()->hasFormat("text/plain")) {
        ev->setDropAction(Qt::CopyAction);
        ev->accept();

        int button = 1;
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
        int state = ev->modifiers();
#else
        int state = QApplication::queryKeyboardModifiers();
#endif

        if (state & Qt::ShiftModifier)
            pb_graph->set_cmdmode(pb_graph->cmdmode() | grShiftMode);
        else
            pb_graph->set_cmdmode(pb_graph->cmdmode() & ~grShiftMode);
        if (state & Qt::ControlModifier)
            pb_graph->set_cmdmode(pb_graph->cmdmode() | grControlMode);
        else
            pb_graph->set_cmdmode(pb_graph->cmdmode() & ~grControlMode);

        const char *new_keyed = 0;
        QByteArray ba = ev->mimeData()->text().toLatin1();
        if (!dynamic_cast<QTplotDlg*>(ev->source())) {
            // Received plain text not from a plot window.  This will
            // become a new keyed text element at the drop location. 
            // Need to set the flag below and pass the string to the
            // handler through a "hidden" argument.

            pb_graph->set_cmdmode(pb_graph->cmdmode() | grMoving);
            new_keyed = ba.constData();
        }

#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
        pb_graph->gr_bup_hdlr(button, ev->position().x(),
            pb_graph->yinv(ev->position().y()), new_keyed);
#else
        pb_graph->gr_bup_hdlr(button, ev->pos().x(),
            pb_graph->yinv(ev->pos().y()), new_keyed);
#endif
        return;
    }
    ev->ignore();
}


void
QTplotDlg::do_save_plot_slot(const char *fnamein, void*)
{
    char *fname = pathlist::expand_path(fnamein, false, true);
    if (!fname)
        return;
    if (filestat::check_file(fname, W_OK) == NOGO) {
        PopUpMessage(filestat::error_msg(), true);
        delete [] fname;
        return;
    }
    wordlist wl;
    wl.wl_word = fname;
    wl.wl_next = pb_graph->command();
    pb_graph->command()->wl_prev = &wl;
    CommandTab::com_write(&wl);
    pb_graph->command()->wl_prev = 0;

    PopUpMessage("Operation completed.", false);
    delete [] fname;

    wb_input->deleteLater();
    wb_input = 0;
}


void
QTplotDlg::do_save_print_slot(const char *fnamein, void*)
{
    char *fname = pathlist::expand_path(fnamein, false, true);
    if (!fname)
        return;
    if (filestat::check_file(fname, W_OK) == NOGO) {
        PopUpMessage(filestat::error_msg(), true);
        delete [] fname;
        return;
    }
    FILE *fp = fopen(fname, "w");
    if (fp) {
        TTY.ioRedirect(0, fp, 0);
        CommandTab::com_print(pb_graph->command());
        TTY.ioReset();
    }
    else
        PopUpErr(MODE_ON, "Can't open file.");
    if (fp)
        PopUpMessage("Plot saved to file as print data.", false);
    delete [] fname;

    wb_input->deleteLater();
    wb_input = 0;
}


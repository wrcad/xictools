
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
#include "miscutil/pathlist.h"
#include "miscutil/filestat.h"
#include "spnumber/spnumber.h"

#ifndef WIN32
#include "../../icons/wrspice_16x16.xpm"
#include "../../icons/wrspice_32x32.xpm"
#include "../../icons/wrspice_48x48.xpm"
#endif

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

// X dependent default parameters
#define DEF_FONT "6x10"
#define NUMLINESTYLES 8
#define NXPLANES 5

/* do this in constructor
    // set up initial xor color
    GdkColor clr;
    clr.pixel = SpGrPkg::DefColors[0].pixel ^ SpGrPkg::DefColors[1].pixel;
#if GTK_CHECK_VERSION(3,0,0)
    w->XorGC()->set_foreground(&clr);
#else
    gdk_gc_set_foreground(w->XorGC(), &clr);
#endif
*/


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

/*  icons in QT?
#ifdef WIN32
    // Icons are obtained from resources.
#else
    GList *g1 = new GList;
    g1->data = gdk_pixbuf_new_from_xpm_data(wrs_48x48_xpm);
    GList *g2 = new GList;
    g2->data = gdk_pixbuf_new_from_xpm_data(wrs_32x32_xpm);
    g1->next = g2; 
    GList *g3 = new GList;
    g3->data = gdk_pixbuf_new_from_xpm_data(wrs_16x16_xpm);
    g3->next = 0;  
    g2->next = g3;
    gtk_window_set_icon_list(GTK_WINDOW(Shell()), g1);
#endif
*/

    char buf[128];
    const char *anam = "";
    if (gr->apptype() == GR_PLOT)
        anam = GR_PLOTstr;
    else if (gr->apptype() == GR_MPLT)
        anam = GR_MPLTstr;
    snprintf(buf, sizeof(buf), "%s %s %d", CP.Program(), anam, gr->id());
    setWindowTitle(buf);

/*
    g_signal_connect(G_OBJECT(Shell()), "destroy",
        G_CALLBACK(b_quit), gr);
*/

    QHBoxLayout *hbox = new QHBoxLayout(this);
    hbox->setMargin(2);
    hbox->setSpacing(2);

    // set up viewport
    gd_viewport = new QTcanvas();
    hbox->addWidget(Viewport(), 0, 0);
    Gbag()->set_draw_if(gd_viewport);
    Viewport()->setFocusPolicy(Qt::StrongFocus);

    QFont *fnt;
    if (FC.getFont(&fnt, FNT_SCREEN))
        gd_viewport->set_font(fnt);
    connect(QTfont::self(), SIGNAL(fontChanged(int)),
        this, SLOT(font_changed_slot(int)), Qt::QueuedConnection);

    connect(Viewport(), SIGNAL(resize_event(QResizeEvent*)),
        this, SLOT(resize_slot(QResizeEvent*)));
    connect(Viewport(), SIGNAL(paint_event(QPaintEvent*)),
        this, SLOT(paint_slot(QPaintEvent*)));
    connect(Viewport(), SIGNAL(press_event(QMouseEvent*)),
        this, SLOT(button_down_slot(QMouseEvent*)));
    connect(Viewport(), SIGNAL(release_event(QMouseEvent*)),
        this, SLOT(button_up_slot(QMouseEvent*)));
    connect(Viewport(), SIGNAL(motion_event(QMouseEvent*)),
        this, SLOT(motion_slot(QMouseEvent*)));
    connect(Viewport(), SIGNAL(key_press_event(QKeyEvent*)),
        this, SLOT(key_down_slot(QKeyEvent*)));
    connect(Viewport(), SIGNAL(key_release_event(QKeyEvent*)),
        this, SLOT(key_up_slot(QKeyEvent*)));
    connect(Viewport(), SIGNAL(enter_event(QEnterEvent*)),
        this, SLOT(enter_slot(QEnterEvent*)));
    connect(Viewport(), SIGNAL(leave_event(QEvent*)),
        this, SLOT(leave_slot(QEvent*)));
    connect(Viewport(), SIGNAL(focus_in_event(QFocusEvent*)),
        this, SLOT(focus_in_slot(QFocusEvent*)));
    connect(Viewport(), SIGNAL(focus_out_event(QFocusEvent*)),
        this, SLOT(focus_out_slot(QFocusEvent*)));
    connect(Viewport(), SIGNAL(drag_enter_event(QDragEnterEvent*)),
        this, SLOT(drag_enter_slot(QDragEnterEvent*)));
    connect(Viewport(), SIGNAL(drop_event(QDropEvent*)),
        this, SLOT(drop_slot(QDropEvent*)));

//    Viewport()->resize(dawid, dahei);

    pb_gbox = new QGroupBox();
    hbox->addWidget(pb_gbox);
    pb_gbox->setMaximumWidth(100);

    init_gbuttons();

    hbox->setStretch(0, 1);

/*
    // The first plot is issued a sGbag and GCs.  Subsequent plots are
    // issued the same sGbag for the class (plots and mplots are
    // separate classes).  This is a problem for plots, which have
    // ghost drawn markers in some plots and not others, so a copy of
    // the Gbag is made in this case.  We use the same GC's though
    // (same GC's for each class).
    //
    if (GC() && gr->apptype() == GR_PLOT) {
        // already filled in, make a copy
        sGbag *gbag = new sGbag;
        gbag->set_gc(GC());
        gbag->set_xorgc(XorGC());
        SetGbag(gbag);
    }
*/

    // position the plot on screen
    int x, y;
    GP.PlotPosition(&x, &y);
    TB()->FixLoc(&x, &y);
    move(x, y);
/*
    gtk_window_move(GTK_WINDOW(Shell()), x, y);
    gtk_widget_add_events(Shell(), GDK_VISIBILITY_NOTIFY_MASK);
    g_signal_connect(G_OBJECT(Shell()), "visibility_notify_event",
        G_CALLBACK(gtk_ToTop), 0);

    // MSW seems to need this before gtk_window_show.
    TB()->RevertFocus(Shell());

    gtk_widget_show(Shell());
*/
    show();


    gr->gr_pkg_init_colors();
    gr->set_fontsize();
    gr->area().set_width(Viewport()->width());
    gr->area().set_height(Viewport()->height());

    return (false);
}


//
// Set up button array, return true if the button count changes
//
bool
QTplotDlg::init_gbuttons()
{
    QVBoxLayout *vbox = new QVBoxLayout(pb_gbox);
    vbox->setMargin(0);
    vbox->setSpacing(2);

    // Just rebuild the whole button array, easier that way.
    for (int i = 0; i < pbtn_NUMBTNS; i++)
        pb_checkwins[i] = 0;

    if (!pb_checkwins[pbtn_dismiss]) {
        QPushButton *btn = new QPushButton(tr("Dismiss"));
        vbox->addWidget(btn);
//        g_signal_connect(G_OBJECT(button), "clicked",
//            G_CALLBACK(b_quit), graph);
        btn->setToolTip(tr("Delete this window"));
        pb_checkwins[pbtn_dismiss] = btn;
        connect(btn, SIGNAL(clicked()), this, SLOT(dismiss_btn_slot()));
    }

    if (!pb_checkwins[pbtn_help]) {
        QPushButton *btn = new QPushButton(tr("Help"));
        vbox->addWidget(btn);
//        g_signal_connect(G_OBJECT(button), "clicked",
//            G_CALLBACK(b_help), graph);
        btn->setToolTip(tr("Press for help"));
        pb_checkwins[pbtn_help] = btn;
        connect(btn, SIGNAL(clicked()), this, SLOT(help_btn_slot()));
    }

    if (!pb_checkwins[pbtn_redraw]) {
        QPushButton *btn = new QPushButton(tr("Redraw"));
        vbox->addWidget(btn);
//        g_signal_connect(G_OBJECT(button), "clicked",
//            G_CALLBACK(b_recolor), graph);
        btn->setToolTip(tr("Redraw the plot"));
        pb_checkwins[pbtn_redraw] = btn;
        connect(btn, SIGNAL(clicked()),
            this, SLOT(redraw_btn_slot()));
    }

    if (!pb_checkwins[pbtn_print]) {
        QPushButton *btn = new QPushButton(tr("Print"));
        vbox->addWidget(btn);
        btn->setCheckable(true);
//        g_signal_connect(G_OBJECT(button), "clicked",
//            G_CALLBACK(b_hardcopy), graph);
        btn->setToolTip(tr("Print control panel"));
        pb_checkwins[pbtn_print] = btn;
        connect(btn, SIGNAL(toggled(bool)),
            this, SLOT(print_btn_slot(bool)));
    }

    if (pb_graph->apptype() == GR_PLOT) {
        if (!pb_checkwins[pbtn_saveplot]) {
            QPushButton *btn = new QPushButton(tr("Save Plot"));
            vbox->addWidget(btn);
//            g_object_set_data(G_OBJECT(wb_shell), "save_plot", button);
//            g_signal_connect(G_OBJECT(button), "clicked",
//                G_CALLBACK(b_save_plot), graph);
            btn->setToolTip(tr("Save the plot in a plot file"));
            pb_checkwins[pbtn_saveplot] = btn;
            connect(btn, SIGNAL(clicked()),
                this, SLOT(saveplt_btn_slot()));
        }

        if (!pb_checkwins[pbtn_saveprint]) {
            QPushButton *btn = new QPushButton(tr("Save Print "));
            vbox->addWidget(btn);
//            g_object_set_data(G_OBJECT(wb_shell), "save_print", button);
//            g_signal_connect(G_OBJECT(button), "clicked",
//                G_CALLBACK(b_save_print), graph);
            btn->setToolTip(tr("Save the plot in a print file"));
            pb_checkwins[pbtn_saveprint] = btn;
            connect(btn, SIGNAL(clicked()),
                this, SLOT(savepr_btn_slot()));
        }

        if (!pb_checkwins[pbtn_points]) {
            QPushButton *btn = new QPushButton(tr("Points"));
            vbox->addWidget(btn);
            btn->setCheckable(true);
//            g_object_set_data(G_OBJECT(wb_shell), "points", button);
            btn->setChecked(pb_graph->plottype() == PLOT_POINT); 
//            g_signal_connect(G_OBJECT(button), "clicked",
//                G_CALLBACK(b_points), graph);
            btn->setToolTip(tr("Plot data as points"));
            pb_checkwins[pbtn_points] = btn;
            connect(btn, SIGNAL(toggled(bool)),
                this, SLOT(points_btn_slot(bool)));
        }

        if (!pb_checkwins[pbtn_comb]) {
            QPushButton *btn = new QPushButton(tr("Comb"));
            vbox->addWidget(btn);
            btn->setCheckable(true);
//            g_object_set_data(G_OBJECT(wb_shell), "combplot", button);
            btn->setChecked(pb_graph->plottype() == PLOT_COMB); 
//            g_signal_connect(G_OBJECT(button), "clicked",
//                G_CALLBACK(b_points), graph);
            btn->setToolTip(tr("Plot data as histogram"));
            pb_checkwins[pbtn_comb] = btn;
            connect(btn, SIGNAL(toggled(bool)),
                this, SLOT(comb_btn_slot(bool)));
        }

        if (pb_graph->rawdata().xmin > 0) {
            if (!pb_checkwins[pbtn_logx]) {
                QPushButton *btn = new QPushButton(tr("Log X"));
                vbox->addWidget(btn);
                btn->setCheckable(true);
                btn->setChecked(pb_graph->gridtype() == GRID_XLOG ||
                        pb_graph->gridtype() == GRID_LOGLOG);
//                g_signal_connect(G_OBJECT(button), "clicked",
//                    G_CALLBACK(b_logx), graph);
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
//                g_signal_connect(G_OBJECT(button), "clicked",
//                    G_CALLBACK(b_logy), graph);
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
//            g_signal_connect(G_OBJECT(button), "clicked",
//                G_CALLBACK(b_marker), graph);
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
//                g_signal_connect(G_OBJECT(button), "clicked",
//                    G_CALLBACK(b_separate), graph);
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
//                    g_object_set_data(G_OBJECT(wb_shell), "onescale",
//                        button);
                    btn->setChecked(pb_graph->format() == FT_SINGLE);
//                    g_signal_connect(G_OBJECT(button), "clicked",
//                        G_CALLBACK(b_onescale), graph);
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
//                    g_object_set_data(G_OBJECT(wb_shell), "onescale",
//                        button);
                    btn->setChecked(pb_graph->format() == FT_SINGLE);
//                    g_signal_connect(G_OBJECT(button), "clicked",
//                        G_CALLBACK(b_multiscale), graph);
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
//                    g_object_set_data(G_OBJECT(wb_shell), "grpscale",
//                        button);
                    btn->setChecked(pb_graph->format() == FT_GROUP);
//                    g_signal_connect(G_OBJECT(button), "clicked",
//                        G_CALLBACK(b_multiscale), graph);
                    btn->setToolTip(tr(
                        "Same scale for groups: V, I, other"));
                    pb_checkwins[pbtn_group] = btn;
                    connect(btn, SIGNAL(toggled(bool)),
                        this, SLOT(group_btn_slot(bool)));
                }
            }
        }
    }

    bool ret = true;
    /*
    if (pb_bbox) {
        int nlen = g_list_length(gtk_container_get_children(
            GTK_CONTAINER(vbox)));
        int olen = g_list_length(gtk_container_get_children(
            GTK_CONTAINER(gtk_bin_get_child(GTK_BIN(pb_bbox)))));
        if (nlen == olen)
            ret = false;
        gtk_container_remove(GTK_CONTAINER(pb_bbox),
            gtk_bin_get_child(GTK_BIN(pb_bbox)));
    }
    else {
        pb_bbox = gtk_frame_new(0);
        gtk_widget_show(pb_bbox);
    }
    gtk_container_add(GTK_CONTAINER(pb_bbox), vbox);
    */
    return (ret);
return (false);
}


// Static function.
// Callback for PopUpInput() sensitivity set.
//
void
QTplotDlg::sens_set(QTbag *w, bool set, int)
{
    /*
    GtkWidget *save_plot =
        (GtkWidget*)g_object_get_data(G_OBJECT(w->Shell()), "save_plot");
    GtkWidget *save_print =
        (GtkWidget*)g_object_get_data(G_OBJECT(w->Shell()), "save_print");
    if (set) {
        if (save_plot)
            gtk_widget_set_sensitive(save_plot, true);
        if (save_print)
            gtk_widget_set_sensitive(save_print, true);
    }
    else {
        if (save_plot)
            gtk_widget_set_sensitive(save_plot, false);
        if (save_print)
            gtk_widget_set_sensitive(save_print, false);
    }
    */
}


/*
// Static function.
// Core test for gr_check_plot_events.
//
bool
QTplotDlg::check_event(GdkEvent *ev, sGraph *graph)
{
    if (!graph)
        return (false);
    QTplotDlg *w = dynamic_cast<QTplotDlg*>(graph->dev());
    if (!w)
        return (false);
    if (ev->type == GDK_EXPOSE) {
        if (ev->expose.window == gtk_widget_get_window(w->Shell()))
            return (true);
    }
    else if (ev->type == GDK_BUTTON_PRESS) {
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
    }
    return (false);
}
*/


// Static function.
// The redraw is in a timeout so that plots with a lot of data and
// take time to render can be resized more easily.
int
QTplotDlg::redraw_timeout(void *arg)
{
    /*
    sGraph *graph = static_cast<sGraph*>(arg);
    QTplotDlg *wb = dynamic_cast<QTplotDlg*>(graph->dev());

    wb->pb_rdid = 0;
#if GTK_CHECK_VERSION(3,0,0)
    ndkDrawable *dw = wb->GetDrawable();
    dw->set_draw_to_pixmap();
    wb->SetColor(graph->color(0).pixel);
    wb->Box(0, 0, graph->area().width(), graph->area().height());
    graph->gr_redraw_direct();
    dw->set_draw_to_window();
    dw->copy_pixmap_to_window(wb->GC(), 0, 0, -1, -1);

#else
    GdkWindow *wtmp = wb->Window();
    wb->SetWindow(wb->pb_pixmap);
    wb->SetColor(graph->color(0).pixel);
    wb->Box(0, 0, graph->area().width(), graph->area().height());
    graph->gr_redraw_direct();
    wb->SetWindow(wtmp);
    gdk_window_copy_area(wb->Window(), wb->GC(), 0, 0, wb->pb_pixmap, 0, 0,
        graph->area().width(), graph->area().height());
#endif

    graph->gr_redraw_keyed();
    graph->set_dirty(false);
    return (0);
    */
}


/*
// Static function.
int
QTplotDlg::redraw(GtkWidget*, GdkEvent *event, void *client_data)
{
    sGraph *graph = static_cast<sGraph*>(client_data);
    QTplotDlg *wb = dynamic_cast<QTplotDlg*>(graph->dev());

    GdkEventExpose *pev = &event->expose;
    // hack for Motif
    if (pev->window != wb->Window())
        return (false);

    int w = gdk_window_get_width(wb->Window());
    int h = gdk_window_get_height(wb->Window());
    if (!wb->pb_pixmap || wb->pb_pmwid != w || wb->pb_pmhei != h) {
        graph->area().set_width(w);
        graph->area().set_height(h);

        if (wb->pb_pixmap)
            gdk_pixmap_unref(wb->pb_pixmap);
        wb->pb_pixmap = gdk_pixmap_new(wb->Window(), graph->area().width(),
            graph->area().height(),
            gdk_visual_get_depth(GTKdev::self()->Visual()));
        wb->pb_pmwid = graph->area().width();
        wb->pb_pmhei = graph->area().height();
        graph->set_dirty(true);
    }
    if (graph->dirty()) {
        if (wb->pb_rdid)
            g_source_remove(wb->pb_rdid);
        wb->pb_rdid = g_timeout_add(250, redraw_timeout, client_data);
    }
    else {
        gdk_window_copy_area(wb->Window(), wb->GC(), pev->area.x,
            pev->area.y, wb->pb_pixmap, pev->area.x, pev->area.y,
            pev->area.width, pev->area.height);

        gdk_gc_set_clip_rectangle(wb->GC(), &pev->area);
        gdk_gc_set_clip_rectangle(wb->XorGC(), &pev->area);
        graph->gr_redraw_keyed();
        gdk_gc_set_clip_rectangle(wb->GC(), 0);
        gdk_gc_set_clip_rectangle(wb->XorGC(), 0);
    }
    return (true);
}
*/


// Static function.
// Idle function to handle motion events.  The events are filtered to
// avoid cursor lag seen in GTK-2 with Pango and certain fonts.
//
int
QTplotDlg::motion_idle(void *arg)
{
    /*
    sGraph *graph = static_cast<sGraph*>(arg);
    QTplotDlg *w = dynamic_cast<QTplotDlg*>(graph->dev());
    if (w && w->Gbag()->showing_ghost()) {
        w->pb_id = 0;
        GP.PushGraphContext(graph);
        w->UndrawGhost();
        w->DrawGhost(w->pb_x, w->pb_y);
        GP.PopGraphContext();
    }
    */
    return (0);
}


// Static function.
void
QTplotDlg::do_save_plot(const char *fnamein, void *client_data)
{
    /*
    sGraph *graph = static_cast<sGraph*>(client_data);
    QTplotDlg *w = dynamic_cast<QTplotDlg*>(graph->dev());
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
    wl.wl_next = graph->command();
    graph->command()->wl_prev = &wl;
    CommandTab::com_write(&wl);
    graph->command()->wl_prev = 0;

    if (w->wb_input)
        w->wb_input->popdown();
    w->PopUpMessage("Operation completed.", false);
    delete [] fname;
    */
}


// Static function.
void
QTplotDlg::do_save_print(const char *fnamein, void *client_data)
{
    /*
    sGraph *graph = static_cast<sGraph*>(client_data);
    QTplotDlg *w = dynamic_cast<QTplotDlg*>(graph->dev());
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
        CommandTab::com_print(graph->command());
        TTY.ioReset();
    }
    else
        w->PopUpErr(MODE_ON, "Can't open file.");
    if (w->wb_input)
        w->wb_input->popdown();
    if (fp)
        w->PopUpMessage("Plot saved to file as print data.", false);
    delete [] fname;
    */
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
    /*
    sGraph *graph = static_cast<sGraph*>(client_data);
    QTplotDlg *wb = dynamic_cast<QTplotDlg*>(graph->dev());

#if GTK_CHECK_VERSION(3,0,0)
    if (!wb->GetDrawable()->get_window()) {
        GtkWidget *vp = wb->Viewport();
        if (gtk_widget_get_window(vp))
            wb->GetDrawable()->set_window(gtk_widget_get_window(vp));
    }

#else
    if (!wb->Window())
        wb->SetWindow(gtk_widget_get_window(wb->Viewport()));
#endif
    int w = gdk_window_get_width(gtk_widget_get_window(caller));
    int h = gdk_window_get_height(gtk_widget_get_window(caller));
    graph->area().set_width(w);
    graph->area().set_height(h);
    graph->gr_abort();
    */
}


void
QTplotDlg::paint_slot(QPaintEvent*)
{
}


void
QTplotDlg::button_down_slot(QMouseEvent*)
{
    /*
    if (GRpkg::self()->CurDev()->devtype == GRhardcopy)
        return (false);
    GdkEventButton *buttonev = &event->button;
    sGraph *graph = static_cast<sGraph*>(client_data);

    gtk_widget_grab_focus(widget);

    int button = 0;
    switch (buttonev->button) {
    case 1:
        button = 1;
        break;
    case 2:
        button = 2;
        break;
    case 3:
        button = 3;
        break;
    }
    if (buttonev->state & GDK_SHIFT_MASK)
        graph->set_cmdmode(graph->cmdmode() | ShiftMode);
    else
        graph->set_cmdmode(graph->cmdmode() & ~ShiftMode);
    if (buttonev->state & GDK_CONTROL_MASK)
        graph->set_cmdmode(graph->cmdmode() | ControlMode);
    else
        graph->set_cmdmode(graph->cmdmode() & ~ControlMode);
    graph->gr_bdown_hdlr(button, (int)buttonev->x,
        graph->yinv((int)buttonev->y));
    */
}


void
QTplotDlg::button_up_slot(QMouseEvent*)
{
    /*
    if (GRpkg::self()->CurDev()->devtype == GRhardcopy)
        return (false);
    GdkEventButton *buttonev = &event->button;
    sGraph *graph = static_cast<sGraph*>(client_data);
    int button = 0;
    switch (buttonev->button) {
    case 1:
        button = 1;
        break;
    case 2:
        button = 2;
        break;
    case 3:
        button = 3;
        break;
    }
    if (buttonev->state & GDK_SHIFT_MASK)
        graph->set_cmdmode(graph->cmdmode() | ShiftMode);
    else
        graph->set_cmdmode(graph->cmdmode() & ~ShiftMode);
    if (buttonev->state & GDK_CONTROL_MASK)
        graph->set_cmdmode(graph->cmdmode() | ControlMode);
    else
        graph->set_cmdmode(graph->cmdmode() & ~ControlMode);
    graph->gr_bup_hdlr(button, (int)buttonev->x,
        graph->yinv((int)buttonev->y));
    */
}


void
QTplotDlg::motion_slot(QMouseEvent*)
{
    /*
    sGraph *graph = static_cast<sGraph*>(client_data);
    QTplotDlg *w = dynamic_cast<QTplotDlg*>(graph->dev());
    if (w && w->Gbag()->showing_ghost()) {
        if (w->pb_id) {
            g_source_remove(w->pb_id);
            w->pb_id = 0;
        }
        w->pb_x = (int)event->motion.x;
        w->pb_y = (int)event->motion.y;
        w->pb_id = g_idle_add(motion_idle, client_data);
        // GTK-2 sets a grab, prevents dragging between windows.
        gdk_pointer_ungrab(GDK_CURRENT_TIME);
    }
    */
}


void
QTplotDlg::key_down_slot(QKeyEvent*)
{
    /*
    sGraph *graph = static_cast<sGraph*>(client_data);
    GdkEventKey *kev = &event->key;
    int keyval = kev->keyval;

    // Make sure we catch backspace.
    if (keyval == GDK_KEY_h && kev->string && *kev->string == '\b')
        keyval = GDK_KEY_BackSpace;

    int code = 0;
    switch (keyval) {
    case GDK_KEY_Escape:
    case GDK_KEY_Tab:
        // Tab will switch focus.
        return (false);

    case GDK_KEY_Up:
        code = UP_KEY;
        break;
    case GDK_KEY_Down:
        code = DOWN_KEY;
        break;
    case GDK_KEY_Left:
        code = LEFT_KEY;
        break;
    case GDK_KEY_Right:
        code = RIGHT_KEY;
        break;
    case GDK_KEY_Return:
        code = ENTER_KEY;
        break;
    case GDK_KEY_BackSpace:
        code = BSP_KEY;
        break;
    case GDK_KEY_Delete:
        code = DELETE_KEY;
        break;
    }
    char text[8];
    *text = 0;
    if (kev->string) {
        strncpy(text, kev->string, 8);
        text[7] = 0;
    }

    // Throw out when cursor is not in viewport.
    int x, y;
    gdk_window_get_pointer(kev->window, &x, &y, 0);
    if (x < 0 || y < 0 ||
            x >= graph->area().width() || y >= graph->area().height())
        return (false);

    graph->gr_key_hdlr(text, code, x, graph->yinv(y));
    */
}


void
QTplotDlg::key_up_slot(QKeyEvent*)
{
}


void
QTplotDlg::enter_slot(QEnterEvent*)
{
    /*
    // pointer entered a drawing window
    sGraph *graph = static_cast<sGraph*>(client_data);
    QTplotDlg *w = dynamic_cast<QTplotDlg*>(graph->dev());
    gtk_widget_set_can_focus(caller, true);
    gtk_window_set_focus(GTK_WINDOW(w->Shell()), caller);
    if (GP.SourceGraph() && graph != GP.SourceGraph()) {
        // hack a ghost for move/copy
        w->Gbag()->set_ghost_func(dynamic_cast<QTplotDlg*>(
            GP.SourceGraph()->dev())->Gbag()->get_ghost_func());
    }
    graph->dev()->ShowGhost(true);
    */
}


void
QTplotDlg::leave_slot(QEvent*)
{
    /*
    // pointer left the drawing window
    sGraph *graph = static_cast<sGraph*>(client_data);
    QTplotDlg *w = dynamic_cast<QTplotDlg*>(graph->dev());
    if (w->Gbag()->has_ghost()) {
        GP.PushGraphContext(graph);
        w->ShowGhost(false);
        GP.PopGraphContext();
    }
    if (GP.SourceGraph() && graph != GP.SourceGraph()) {
        // remove hacked ghost
        w->Gbag()->set_ghost_func(0);
    }
    */
}


void
QTplotDlg::focus_in_slot(QFocusEvent*)
{
}


void
QTplotDlg::focus_out_slot(QFocusEvent*)
{
}


void
QTplotDlg::drag_enter_slot(QDragEnterEvent*)
{
}


void
QTplotDlg::drop_slot(QDropEvent*)
{
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
        do_save_plot, pb_graph);
}


void
QTplotDlg::savepr_btn_slot()
{
    PopUpInput("Enter name for print file", 0, "Save Print File",
        do_save_print, pb_graph);
}


void
QTplotDlg::points_btn_slot(bool)
{
    /*
    // handle points and combplot modes
    sGraph *graph = static_cast<sGraph*>(client_data);
    QTplotDlg *w = dynamic_cast<QTplotDlg*>(graph->dev());

    GtkWidget *pts = (GtkWidget*)g_object_get_data(G_OBJECT(w->Shell()),
        "points");
    GtkWidget *cmb = (GtkWidget*)g_object_get_data(G_OBJECT(w->Shell()),
        "combplot");
    if (pts && cmb) {
        bool ptson = GTKdev::GetStatus(pts);
        bool cmbon = GTKdev::GetStatus(cmb);

        if (!ptson && !cmbon)
            graph->set_plottype(PLOT_LIN);
        else if (caller == pts) {
            if (cmbon)
                GTKdev::SetStatus(cmb, false);
            graph->set_plottype(PLOT_POINT);
        }
        else {
            if (ptson)
                GTKdev::SetStatus(pts, false);
            graph->set_plottype(PLOT_COMB);
        }
        graph->dev()->Clear();
        graph->gr_redraw();
    }
    */
}


void
QTplotDlg::comb_btn_slot(bool)
{
}


void
QTplotDlg::logx_btn_slot(bool)
{
    /*
    sGraph *graph = static_cast<sGraph*>(client_data);
    QTplotDlg *w = dynamic_cast<QTplotDlg*>(graph->dev());
    if (graph->rawdata().xmin <= 0) {
        w->PopUpErr(MODE_ON,
            "The X-axis scale must be greater than zero for log plot.");
        GTKdev::SetStatus(caller, false);
        return;
    }
    bool state = GTKdev::GetStatus(caller);
    if (state) {
        if (graph->gridtype() == GRID_YLOG)
            graph->set_gridtype(GRID_LOGLOG);
        else
            graph->set_gridtype(GRID_XLOG);
    }
    else {
        if (graph->gridtype() == GRID_LOGLOG)
            graph->set_gridtype(GRID_YLOG);
        else
            graph->set_gridtype(GRID_LIN);
    }

    graph->clear_units_text(LAxunits);
    graph->dev()->Clear();
    graph->gr_redraw();
    */
}


void
QTplotDlg::logy_btn_slot(bool)
{
    /*
    sGraph *graph = static_cast<sGraph*>(client_data);
    QTplotDlg *w = dynamic_cast<QTplotDlg*>(graph->dev());
    if (graph->rawdata().ymin <= 0) {
        w->PopUpErr(MODE_ON,
            "The Y-axis scale must be greater than zero for log plot.");
        GTKdev::SetStatus(caller, false);
        return;
    }
    bool state = GTKdev::GetStatus(caller);
    if (state) {
        if (graph->gridtype() == GRID_XLOG)
            graph->set_gridtype(GRID_LOGLOG);
        else
            graph->set_gridtype(GRID_YLOG);
    }
    else {
        if (graph->gridtype() == GRID_LOGLOG)
            graph->set_gridtype(GRID_XLOG);
        else
            graph->set_gridtype(GRID_LIN);
    }

    // clear the yunits text
    graph->clear_units_text(LAyunits);
    graph->dev()->Clear();
    graph->gr_redraw();
    */
}


void
QTplotDlg::marker_btn_slot(bool)
{
    /*
    sGraph *graph = static_cast<sGraph*>(client_data);
    QTplotDlg *w = dynamic_cast<QTplotDlg*>(graph->dev());
    if (!graph->xmonotonic() && graph->yseparate()) {
        w->PopUpErr(MODE_ON,
"Marker is not available in separate trace mode with nonmonotonic scale.");
        GTKdev::SetStatus(caller, false);
        return;
    }
    if (GTKdev::GetStatus(caller)) {
        if (!graph->reference().mark) {
            graph->reference().mark = true;
            graph->gr_mark();
        }
    }
    else if (graph->reference().mark) {
        graph->reference().mark = false;
        graph->gr_mark();
        graph->reference().set = false;
    }
    */
}


void
QTplotDlg::separate_btn_slot(bool)
{
    /*
    sGraph *graph = static_cast<sGraph*>(client_data);
    QTplotDlg *w = dynamic_cast<QTplotDlg*>(graph->dev());
    if (!graph->xmonotonic() && graph->reference().mark) {
        w->PopUpErr(MODE_ON,
"Can't enter separate trace mode with nonmonotonic scale when\n\
marker is active.");
        GTKdev::SetStatus(caller, false);
        return;
    }
    if (graph->yseparate())
        graph->set_yseparate(false);
    else
        graph->set_yseparate(true);
    graph->dev()->Clear();
    graph->gr_redraw();
    */
}


void
QTplotDlg::single_btn_slot(bool)
{
    /*
    sGraph *graph = static_cast<sGraph*>(client_data);
    if (graph->format() == FT_MULTI)
        graph->set_format(FT_SINGLE);
    else
        graph->set_format(FT_MULTI);
    graph->dev()->Clear();
    graph->gr_redraw();
    */
}


void
QTplotDlg::group_btn_slot(bool)
{
    /*
    // Handle one scale and grp scale modes.
    sGraph *graph = static_cast<sGraph*>(client_data);
    QTplotDlg *w = dynamic_cast<QTplotDlg*>(graph->dev());

    GtkWidget *one = (GtkWidget*)g_object_get_data(G_OBJECT(w->Shell()),
        "onescale");
    GtkWidget *grp =  (GtkWidget*)g_object_get_data(G_OBJECT(w->Shell()),
        "grpscale");
    if (one && grp) {
        bool oneon = GTKdev::GetStatus(one);
        bool grpon = GTKdev::GetStatus(grp);

        if (!oneon && !grpon)
            graph->set_format(FT_MULTI);
        else if (caller == one) {
            if (grpon)
                GTKdev::SetStatus(grp, false);
            graph->set_format(FT_SINGLE);
        }
        else {
            if (oneon)
                GTKdev::SetStatus(one, false);
            graph->set_format(FT_GROUP);
        }
        graph->dev()->Clear();
        graph->gr_redraw();
    }
    */
}


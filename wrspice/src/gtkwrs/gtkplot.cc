
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
// GTK drivers for plotting.
//

#include "graph.h"
#include "cshell.h"
#include "kwords_fte.h"
#include "commands.h"
#include "simulator.h"
#include "gtktoolb.h"
#include "gtkinterf/gtkfont.h"
#include "gtkinterf/gtkutil.h"
#include "miscutil/pathlist.h"
#include "miscutil/filestat.h"
#include "spnumber/spnumber.h"

#include <gdk/gdkkeysyms.h>
#ifndef WIN32
#include "../../icons/wrspice_16x16.xpm"
#include "../../icons/wrspice_32x32.xpm"
#include "../../icons/wrspice_48x48.xpm"
#endif


// help keywords used in this file
// plotpanel
// mplotpanel

// X dependent default parameters
#define DEF_FONT "6x10"
#define NUMLINESTYLES 8
#define NXPLANES 5

// Indices into the buttons array for the button witget pointers.
enum pbtn_type
{
    pbtn_dismiss,
    pbtn_help,
    pbtn_redraw,
    pbtn_print,
    pbtn_saveplot,
    pbtn_saveprint,
    pbtn_points,
    pbtn_comb,
    pbtn_logx,
    pbtn_logy,
    pbtn_marker,
    pbtn_separate,
    pbtn_single,
    pbtn_group,
    pbtn_NUMBTNS
};

// special 'widget bag' for plot and mplot windows.
//
struct plot_bag : public GTKbag,  public GTKdraw
{
    friend struct sGraph;

    plot_bag(int type) : GTKdraw(type)
        {
            pb_bbox = 0;
            for (int i = 0; i < pbtn_NUMBTNS; i++)
                pb_checkwins[i] = 0;
#if GTK_CHECK_VERSION(3,0,0)
#else
            pb_pixmap = 0;
            pb_pmwid = pb_pmhei = 0;
#endif
            pb_id = 0;
            pb_x = pb_y = 0;
            pb_rdid = 0;
        }

    ~plot_bag()
        {
#if GTK_CHECK_VERSION(3,0,0)
#else
            if (pb_pixmap)
                gdk_pixmap_unref(pb_pixmap);
#endif
            if (pb_id)
                g_source_remove(pb_id);
        }

    bool init_gbuttons(sGraph*);

    bool init(sGraph*);

private:
    static bool check_event(GdkEvent*, sGraph*);
    static void sens_set(GTKbag*, bool, int);
    static int resize(GtkWidget*, GdkEvent*, void*);
    static int redraw_timeout(void*);
#if GTK_CHECK_VERSION(3,0,0)
    static int redraw(GtkWidget*, cairo_t*, void*);
#else
    static int redraw(GtkWidget*, GdkEvent*, void*);
#endif
    static int motion(GtkWidget*, GdkEvent*, void*);
    static int motion_idle(void*);
    static int focus(GtkWidget*, GdkEvent*, void*);
    static int keypress(GtkWidget*, GdkEvent*, void*);
    static int buttonpress(GtkWidget*, GdkEvent*, void*);
    static int buttonup(GtkWidget*, GdkEvent*, void*);
    static int enter_hdlr(GtkWidget*, GdkEvent*, void*);
    static int leave_hdlr(GtkWidget*, GdkEvent*, void*);
    static void font_change_hdlr(GtkWidget*, void*, void*);
    static void b_quit(GtkWidget*, void*);
    static void b_help(GtkWidget*, void*);
    static void b_recolor(GtkWidget*, void*);
    static void b_hardcopy(GtkWidget*, void*);
    static void b_save_plot(GtkWidget*, void*);
    static void do_save_plot(const char*, void*);
    static void b_save_print(GtkWidget*, void*);
    static void do_save_print(const char*, void*);
    static void b_points(GtkWidget*, void*);
    static void b_logx(GtkWidget*, void*);
    static void b_logy(GtkWidget*, void*);
    static void b_marker(GtkWidget*, void*);
    static void b_separate(GtkWidget*, void*);
    static void b_onescale(GtkWidget*, void*);
    static void b_multiscale(GtkWidget*, void*);
    static void set_hccb(HCcb*);
    static bool get_dim(const char*, double*);

    GtkWidget *pb_bbox;             // frame containing button box
    GtkWidget *pb_checkwins[pbtn_NUMBTNS];  // button widget pointers
#if GTK_CHECK_VERSION(3,0,0)
#else
    GdkPixmap *pb_pixmap;           // backing store
    int pb_pmwid, pb_pmhei;         // pixmap size
#endif
    int pb_id;                      // motion idle id
    int pb_x, pb_y;                 // motion coords
    int pb_rdid;                    // redisplay timeout id
};


bool
plot_bag::init(sGraph *gr)
{
    wb_sens_set = sens_set;

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

    g_signal_connect(G_OBJECT(Shell()), "destroy",
        G_CALLBACK(b_quit), gr);

    GtkWidget *form = gtk_table_new(2, 1, false);
    gtk_widget_show(form);
    gtk_container_add(GTK_CONTAINER(Shell()), form);
    gtk_container_set_border_width(GTK_CONTAINER(Shell()), 5);

    // set up viewport
    SetViewport(gtk_drawing_area_new());

    int dawid, dahei;
    if (gr->apptype() == GR_PLOT) {
        dawid = 400;
        dahei = 300;
    
        variable *v = Sp.GetRawVar(kw_plotgeom);
        if (v) {
            if (v->type() == VTYP_STRING) {
                const char *s = v->string();
                double *d = SPnum.parse(&s, false);
                if (d && *d >= 100.0 && *d <= 2000.0) {
                    dawid = (int)*d;
                    while (*s && !isdigit(*s)) s++;
                    d = SPnum.parse(&s, false);
                    if (d && *d >= 100.0 && *d <= 2000.0)
                        dahei = (int)*d;
                }
            }
            else if (v->type() == VTYP_LIST) {
                v = v->list();
                bool ok = false;
                if (v->type() == VTYP_NUM && v->integer() >= 100 &&
                        v->integer() <= 2000) {
                    dawid = v->integer();
                    ok = true;
                }
                else if (v->type() == VTYP_REAL && v->real() >= 100.0 &&
                        v->real() <= 2000) {
                    dawid = (int)v->real();
                    ok = true;
                }
                v = v->next();
                if (ok && v) {
                    if (v->type() == VTYP_NUM && v->integer() >= 100 &&
                            v->integer() <= 2000)
                        dahei = v->integer();
                    else if (v->type() == VTYP_REAL && v->real() >= 100.0 &&
                            v->real() <= 2000.0)
                        dahei = (int)v->real();
                }
            }
        }
    }
    else {
        dawid = 300;
        dahei = 300;
    }
    gtk_widget_set_size_request(Viewport(), dawid, dahei);
    gtk_widget_show(Viewport());
    GTKfont::setupFont(Viewport(), FNT_SCREEN, true);

    gtk_widget_add_events(Viewport(), GDK_STRUCTURE_MASK);
    g_signal_connect(G_OBJECT(Viewport()), "configure_event",
        G_CALLBACK(resize), gr);
#if GTK_CHECK_VERSION(3,0,0)
    g_signal_connect(G_OBJECT(Viewport()), "draw",
        G_CALLBACK(redraw), gr);
#else
    gtk_widget_add_events(Viewport(), GDK_EXPOSURE_MASK);
    g_signal_connect(G_OBJECT(Viewport()), "expose_event",
        G_CALLBACK(redraw), gr);
#endif
    gtk_widget_add_events(Viewport(), GDK_KEY_PRESS_MASK);
    g_signal_connect_after(G_OBJECT(Viewport()), "key_press_event",
        G_CALLBACK(keypress), gr);
    gtk_widget_add_events(Viewport(), GDK_BUTTON_PRESS_MASK);
    g_signal_connect_after(G_OBJECT(Viewport()), "button_press_event",
        G_CALLBACK(buttonpress), gr);
    gtk_widget_add_events(Viewport(), GDK_BUTTON_RELEASE_MASK);
    g_signal_connect_after(G_OBJECT(Viewport()), "button_release_event",
        G_CALLBACK(buttonup), gr);
    gtk_widget_add_events(Viewport(), GDK_POINTER_MOTION_MASK);
    g_signal_connect(G_OBJECT(Viewport()), "motion_notify_event",
        G_CALLBACK(motion), gr);
    gtk_widget_add_events(Viewport(), GDK_ENTER_NOTIFY_MASK);
    g_signal_connect(G_OBJECT(Viewport()), "enter_notify_event",
        G_CALLBACK(enter_hdlr), gr);
    gtk_widget_add_events(Viewport(), GDK_LEAVE_NOTIFY_MASK);
    g_signal_connect(G_OBJECT(Viewport()), "leave_notify_event",
        G_CALLBACK(leave_hdlr), gr);
    g_signal_connect(G_OBJECT(Viewport()), "style_set",
        G_CALLBACK(font_change_hdlr), gr);

    // This avoids issue of an expose event on focus change.
    gtk_widget_add_events(Viewport(), GDK_FOCUS_CHANGE_MASK);
    g_signal_connect(G_OBJECT(Viewport()), "focus_in_event",
        G_CALLBACK(focus), this);
    g_signal_connect(G_OBJECT(Viewport()), "focus_out_event",
        G_CALLBACK(focus), this);

    GtkWidget *frame = gtk_frame_new(0);
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), Viewport());

    gtk_table_attach(GTK_TABLE(form), frame, 0, 1, 0, 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 0, 0);

    init_gbuttons(gr);
    gtk_table_attach(GTK_TABLE(form), pb_bbox, 1, 2, 0, 1,
        (GtkAttachOptions)0,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 0, 0);

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

    // position the plot on screen
    int x, y;
    GP.PlotPosition(&x, &y);
    TB()->FixLoc(&x, &y);
    gtk_window_move(GTK_WINDOW(Shell()), x, y);
    gtk_widget_add_events(Shell(), GDK_VISIBILITY_NOTIFY_MASK);
    g_signal_connect(G_OBJECT(Shell()), "visibility_notify_event",
        G_CALLBACK(gtk_ToTop), 0);

    // MSW seems to need this before gtk_window_show.
    TB()->RevertFocus(Shell());

    gtk_widget_show(Shell());
#if GTK_CHECK_VERSION(3,0,0)
    GetDrawable()->set_window(gtk_widget_get_window(Viewport()));
#else
    SetWindow(gtk_widget_get_window(Viewport()));
#endif
    {
        char buf1[128], buf2[64];
        const char *anam = "";
        if (gr->apptype() == GR_PLOT)
            anam = GR_PLOTstr;
        else if (gr->apptype() == GR_MPLT)
            anam = GR_MPLTstr;
        snprintf(buf1, sizeof(buf1), "%s %s %d", CP.Program(), anam, gr->id());
        snprintf(buf2, sizeof(buf2), "%s%d", anam, gr->id());
        Title(buf1, buf2);
    }

    gr->gr_pkg_init_colors();
    gr->set_fontsize();

    // set up cursor
    GdkCursor *cursor = gdk_cursor_new(GDK_LEFT_PTR);
#if GTK_CHECK_VERSION(3,0,0)
    GdkWindow *window = GetDrawable()->get_window();
    if (window) {
        gdk_window_set_cursor(window, cursor);
        gr->area().set_width(gdk_window_get_width(window));
        gr->area().set_height(gdk_window_get_height(window));
    }
#else
    gdk_window_set_cursor(Window(), cursor);

    int w = gdk_window_get_width(Window());
    int h = gdk_window_get_height(Window());
    gr->area().set_width(w);
    gr->area().set_height(h);
#endif

#ifndef __APPLE__
    // With OSX 13.0.1 abd XQuartz 2.8.2, these lines cause the
    // toolbar to become hidden when a plot window pops up.  Not sure
    // why these lines are needed in any case.
#if GTK_CHECK_VERSION(3,0,0)
    if (TB()->context && TB()->context->GetDrawable()->get_window()) {
#else
    if (TB()->context && TB()->context->Window()) {
#endif
        gtk_window_set_transient_for(GTK_WINDOW(Shell()),
            GTK_WINDOW(TB()->context->Shell()));
    }
#endif
    return (false);
}


//
// Set up button array, return true if the button count changes
//
bool
plot_bag::init_gbuttons(sGraph *graph)
{

    GtkWidget *vbox = gtk_vbox_new(false, 0);
    gtk_widget_show(vbox);

    // Just rebuild the whole button array, easier that way.
    for (int i = 0; i < pbtn_NUMBTNS; i++)
        pb_checkwins[i] = 0;

    if (!pb_checkwins[pbtn_dismiss]) {
        GtkWidget *button = gtk_button_new_with_label("Dismiss");
        gtk_widget_show(button);
        g_signal_connect(G_OBJECT(button), "clicked",
            G_CALLBACK(b_quit), graph);
        gtk_box_pack_start(GTK_BOX(vbox), button, true, true, 2);
        gtk_widget_set_tooltip_text(button, "Delete this window");
        pb_checkwins[pbtn_dismiss] = button;
    }

    if (!pb_checkwins[pbtn_help]) {
        GtkWidget *button = gtk_button_new_with_label("Help");
        gtk_widget_show(button);
        g_signal_connect(G_OBJECT(button), "clicked",
            G_CALLBACK(b_help), graph);
        gtk_box_pack_start(GTK_BOX(vbox), button, true, true, 2);
        gtk_widget_set_tooltip_text(button, "Press for help");
        pb_checkwins[pbtn_help] = button;
    }

    if (!pb_checkwins[pbtn_redraw]) {
        GtkWidget *button = gtk_button_new_with_label("Redraw");
        gtk_widget_show(button);
        g_signal_connect(G_OBJECT(button), "clicked",
            G_CALLBACK(b_recolor), graph);
        gtk_box_pack_start(GTK_BOX(vbox), button, true, true, 2);
        gtk_widget_set_tooltip_text(button, "Redraw the plot");
        pb_checkwins[pbtn_redraw] = button;
    }

    if (!pb_checkwins[pbtn_print]) {
        GtkWidget *button = gtk_toggle_button_new_with_label("Print");
        gtk_widget_show(button);
        g_signal_connect(G_OBJECT(button), "clicked",
            G_CALLBACK(b_hardcopy), graph);
        gtk_box_pack_start(GTK_BOX(vbox), button, true, true, 2);
        gtk_widget_set_tooltip_text(button, "Print control panel");
        pb_checkwins[pbtn_print] = button;
    }

    if (graph->apptype() == GR_PLOT) {
        if (!pb_checkwins[pbtn_saveplot]) {
            GtkWidget *button = gtk_button_new_with_label("Save Plot");
            gtk_widget_show(button);
            g_object_set_data(G_OBJECT(wb_shell), "save_plot", button);
            g_signal_connect(G_OBJECT(button), "clicked",
                G_CALLBACK(b_save_plot), graph);
            gtk_box_pack_start(GTK_BOX(vbox), button, true, true, 2);
            gtk_widget_set_tooltip_text(button,
                "Save the plot in a plot file");
            pb_checkwins[pbtn_saveplot] = button;
        }

        if (!pb_checkwins[pbtn_saveprint]) {
            GtkWidget *button = gtk_button_new_with_label("Save Print ");
            gtk_widget_show(button);
            g_object_set_data(G_OBJECT(wb_shell), "save_print", button);
            g_signal_connect(G_OBJECT(button), "clicked",
                G_CALLBACK(b_save_print), graph);
            gtk_box_pack_start(GTK_BOX(vbox), button, true, true, 2);
            gtk_widget_set_tooltip_text(button,
                "Save the plot in a print file");
            pb_checkwins[pbtn_saveprint] = button;
        }

        if (!pb_checkwins[pbtn_points]) {
            GtkWidget *button = gtk_toggle_button_new_with_label("Points");
            gtk_widget_show(button);
            g_object_set_data(G_OBJECT(wb_shell), "points", button);
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),
                (graph->plottype() == PLOT_POINT)); 
            g_signal_connect(G_OBJECT(button), "clicked",
                G_CALLBACK(b_points), graph);
            gtk_box_pack_start(GTK_BOX(vbox), button, true, true, 2);
            gtk_widget_set_tooltip_text(button, "Plot data as points");
            pb_checkwins[pbtn_points] = button;
        }

        if (!pb_checkwins[pbtn_comb]) {
            GtkWidget *button = gtk_toggle_button_new_with_label("Comb");
            gtk_widget_show(button);
            g_object_set_data(G_OBJECT(wb_shell), "combplot", button);
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),
                (graph->plottype() == PLOT_COMB)); 
            g_signal_connect(G_OBJECT(button), "clicked",
                G_CALLBACK(b_points), graph);
            gtk_box_pack_start(GTK_BOX(vbox), button, true, true, 2);
            gtk_widget_set_tooltip_text(button, "Plot data as histogram");
            pb_checkwins[pbtn_comb] = button;
        }

        if (graph->rawdata().xmin > 0) {
            if (!pb_checkwins[pbtn_logx]) {
                GtkWidget *button = gtk_toggle_button_new_with_label("Log X");
                gtk_widget_show(button);
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),
                    (graph->gridtype() == GRID_XLOG ||
                        graph->gridtype() == GRID_LOGLOG));
                g_signal_connect(G_OBJECT(button), "clicked",
                    G_CALLBACK(b_logx), graph);
                gtk_box_pack_start(GTK_BOX(vbox), button, true, true, 2);
                gtk_widget_set_tooltip_text(button,
                    "Use logarithmic horizontal scale");
                pb_checkwins[pbtn_logx] = button;
            }
        }
        else {
            if (graph->gridtype() == GRID_LOGLOG)
                graph->set_gridtype(GRID_YLOG);
            else if (graph->gridtype() == GRID_XLOG)
                graph->set_gridtype(GRID_LIN);
        }

        if (graph->rawdata().ymin > 0) {
            if (!pb_checkwins[pbtn_logy]) {
                GtkWidget *button = gtk_toggle_button_new_with_label("Log Y");
                gtk_widget_show(button);
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),
                    (graph->gridtype() == GRID_YLOG ||
                        graph->gridtype() == GRID_LOGLOG));
                g_signal_connect(G_OBJECT(button), "clicked",
                    G_CALLBACK(b_logy), graph);
                gtk_box_pack_start(GTK_BOX(vbox), button, true, true, 2);
                gtk_widget_set_tooltip_text(button,
                    "Use logarithmic vertical scale");
                pb_checkwins[pbtn_logy] = button;
            }
        }
        else {
            if (graph->gridtype() == GRID_LOGLOG)
                graph->set_gridtype(GRID_XLOG);
            else if (graph->gridtype() == GRID_YLOG)
                graph->set_gridtype(GRID_LIN);
        }

        if (!pb_checkwins[pbtn_marker]) {
            GtkWidget *button = gtk_toggle_button_new_with_label("Marker");
            gtk_widget_show(button);
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),
                graph->reference().mark);
            g_signal_connect(G_OBJECT(button), "clicked",
                G_CALLBACK(b_marker), graph);
            gtk_box_pack_start(GTK_BOX(vbox), button, true, true, 2);
            gtk_widget_set_tooltip_text(button, "Show marker");
            pb_checkwins[pbtn_marker] = button;
        }

        if (graph->numtraces() > 1 && graph->numtraces() <= MAXNUMTR) {
            if (!pb_checkwins[pbtn_separate]) {
                GtkWidget *button =
                    gtk_toggle_button_new_with_label("Separate");
                gtk_widget_show(button);
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),
                    (graph->yseparate()));
                g_signal_connect(G_OBJECT(button), "clicked",
                    G_CALLBACK(b_separate), graph);
                gtk_box_pack_start(GTK_BOX(vbox), button, true, true, 2);
                gtk_widget_set_tooltip_text(button, "Show traces separately");
                pb_checkwins[pbtn_separate] = button;
            }

            int i = 0;
            for (sDvList *link = (sDvList*)graph->plotdata(); link;
                    link = link->dl_next, i++) {
                if (!(*((sDvList *)graph->plotdata())->dl_dvec->units() ==
                        *link->dl_dvec->units())) {
                    i = 0;
                    break;
                }
            }
            if (i > 1) {
                // more than one trace, same type
                if (!pb_checkwins[pbtn_single]) {
                    GtkWidget *button =
                        gtk_toggle_button_new_with_label("Single");
                    gtk_widget_show(button);
                    g_object_set_data(G_OBJECT(wb_shell), "onescale",
                        button);
                    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),
                        (graph->format() == FT_SINGLE));
                    g_signal_connect(G_OBJECT(button), "clicked",
                        G_CALLBACK(b_onescale), graph);
                    gtk_box_pack_start(GTK_BOX(vbox), button, true, true, 2);
                    gtk_widget_set_tooltip_text(button, 
                        "Use single vertical scale");
                    pb_checkwins[pbtn_single] = button;
                }
            }
            else if (!i) {
                // more than one trace type
                if (!pb_checkwins[pbtn_single]) {
                    GtkWidget *button =
                        gtk_toggle_button_new_with_label("Single");
                    gtk_widget_show(button);
                    g_object_set_data(G_OBJECT(wb_shell), "onescale",
                        button);
                    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),
                        (graph->format() == FT_SINGLE));
                    g_signal_connect(G_OBJECT(button), "clicked",
                        G_CALLBACK(b_multiscale), graph);
                    gtk_box_pack_start(GTK_BOX(vbox), button, true, true, 2);
                    gtk_widget_set_tooltip_text(button, 
                        "Use single vertical scale");
                    pb_checkwins[pbtn_single] = button;
                }

                if (!pb_checkwins[pbtn_group]) {
                    GtkWidget *button =
                        gtk_toggle_button_new_with_label("Group");
                    gtk_widget_show(button);
                    g_object_set_data(G_OBJECT(wb_shell), "grpscale",
                        button);
                    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),
                        (graph->format() == FT_GROUP));
                    g_signal_connect(G_OBJECT(button), "clicked",
                        G_CALLBACK(b_multiscale), graph);
                    gtk_box_pack_start(GTK_BOX(vbox), button, true, true, 2);
                    gtk_widget_set_tooltip_text(button, 
                        "Same scale for groups: V, I, other");
                    pb_checkwins[pbtn_group] = button;
                }
            }
        }
    }

    bool ret = true;
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
    return (ret);
}
// End of plot_bag functions


// Return a new graphics context struct.
//
GRwbag *
sGraph::gr_new_gx(int type)
{
    return (new plot_bag(type));
}


// Initialization of X11 graphics, return false on success.
//
int
sGraph::gr_pkg_init()
{
    if (!gr_dev)
        return (true);

    plot_bag *w = dynamic_cast<plot_bag*>(gr_dev);
    return (w->init(this));
}


// Fill in the current colors in the graph from the DefColors array,
// which is updated.
//
void
sGraph::gr_pkg_init_colors()
{
    SpGrPkg::SetDefaultColors();
    const sGraph *tgraph = this;
    if (!tgraph)
        return;

    plot_bag *w = dynamic_cast<plot_bag*>(gr_dev);
    if (w && w->Viewport() && gtk_widget_get_window(w->Viewport())) {
        w->SetWindowBackground(SpGrPkg::DefColors[0].pixel);
        w->SetBackground(SpGrPkg::DefColors[0].pixel);
        w->Clear();
    }
    for (int i = 0; i < GRpkg::self()->MainDev()->numcolors; i++)
        gr_colors[i] = SpGrPkg::DefColors[i];
}


// This rebuilds the button array after adding/removing a trace.  The
// return is true if the button count changes
//
bool
sGraph::gr_init_btns()
{
    plot_bag *w = dynamic_cast<plot_bag*>(gr_dev);
    if (w)
        return (w->init_gbuttons(this));
    return (false);
}


// This is called periodically while a plot is being drawn.  If a button
// press or exposure event is detected for the plot window, return true.
//
bool
sGraph::gr_check_plot_events()
{
    GdkEvent *ev;
    while ((ev = gdk_event_peek()) != 0) {
        if (plot_bag::check_event(ev, this)) {
            gdk_event_free(ev);
            return (true);
        }
        gdk_event_free(ev);
        if ((ev = gdk_event_get()) != 0)
            gtk_main_do_event(ev);
    }
    return (false);
}


void
sGraph::gr_redraw()
{
    if (GRpkg::self()->CurDev()->devtype == GRhardcopy) {
        gr_redraw_direct();
        gr_redraw_keyed();
        return;
    }
    plot_bag *wb = dynamic_cast<plot_bag*>(gr_dev);
#if GTK_CHECK_VERSION(3,0,0)
    ndkDrawable *dw = wb->GetDrawable();
    dw->set_draw_to_pixmap();
    int width = dw->get_width();
    int height = dw->get_height();
    area().set_width(width);
    area().set_height(height);
    wb->SetColor(gr_colors[0].pixel);
    wb->Box(0, 0, width, height);
    gr_redraw_direct();  // This might change area().width/area().height.
    dw->set_draw_to_window();
    dw->copy_pixmap_to_window(wb->GC(), 0, 0, -1, -1);

#else
    if (!wb->Window())
        return;
    int width = gdk_window_get_width(wb->Window());
    int height = gdk_window_get_height(wb->Window());
    area().set_width(width);
    area().set_height(height);
    if (!wb->pb_pixmap || wb->pb_pmwid != width || wb->pb_pmhei != height) {
        if (wb->pb_pixmap)
            gdk_pixmap_unref(wb->pb_pixmap);
        wb->pb_pixmap = gdk_pixmap_new(wb->Window(), width, height,
            gdk_visual_get_depth(GTKdev::self()->Visual()));
        wb->pb_pmwid = width;
        wb->pb_pmhei = height;
    }
    GdkWindow *wtmp = wb->Window();
    wb->SetWindow(wb->pb_pixmap);
    wb->SetColor(gr_colors[0].pixel);
    wb->Box(0, 0, width, height);
    gr_redraw_direct();  // This might change area().width/area().height.
    wb->SetWindow(wtmp);
    gdk_window_copy_area(wb->Window(), wb->GC(), 0, 0, wb->pb_pixmap, 0, 0,
        width, height);
#endif

    gr_redraw_keyed();
    gr_dirty = false;
}


void
sGraph::gr_refresh(int left, int bottom, int right, int top, bool notxt)
{
    plot_bag *wb = dynamic_cast<plot_bag*>(gr_dev);
#if GTK_CHECK_VERSION(3,0,0)
    ndkDrawable *dw = wb->GetDrawable();
    int w = dw->get_width();
    int h = dw->get_height();
    area().set_width(w);
    area().set_height(h);
    bool dirty = dw->set_draw_to_pixmap();
    dw->set_draw_to_window();
    if (dirty) {
        gr_redraw();
        return;
    }
    dw->copy_pixmap_to_window(wb->GC(), 0, 0, -1, -1);

    GdkRectangle r;
    r.x = left;
    r.y = top;
    r.width = right - left;
    r.height = bottom - top;
    wb->GC()->set_clip_rectangle(&r);
    wb->XorGC()->set_clip_rectangle(&r);
    if (!notxt)
        gr_redraw_keyed();
    wb->GC()->set_clip_rectangle(0);
    wb->XorGC()->set_clip_rectangle(0);

#else
    int w = gdk_window_get_width(wb->Window());
    int h = gdk_window_get_height(wb->Window());
    area().set_width(w);
    area().set_height(h);
    if (gr_dirty || !wb->pb_pixmap || wb->pb_pmwid != area().width() ||
            wb->pb_pmhei != area().height()) {
        gr_redraw();
        return;
    }
    gdk_window_copy_area(wb->Window(), wb->GC(), left,
        top, wb->pb_pixmap, left, top, right - left, bottom - top);
    GdkRectangle r;
    r.x = left;
    r.y = top;
    r.width = right - left;
    r.height = bottom - top;
    gdk_gc_set_clip_rectangle(wb->GC(), &r);
    gdk_gc_set_clip_rectangle(wb->XorGC(), &r);
    if (!notxt)
        gr_redraw_keyed();
    gdk_gc_set_clip_rectangle(wb->GC(), 0);
    gdk_gc_set_clip_rectangle(wb->XorGC(), 0);
#endif
}


// Pop down and destroy.
//
void
sGraph::gr_popdown()
{
    plot_bag *w = dynamic_cast<plot_bag*>(dev());
    g_signal_handlers_disconnect_by_func(G_OBJECT(w->Shell()),
        (gpointer)plot_bag::b_quit, this);
    delete w;
    set_dev(0);
    GP.DestroyGraph(id());
}
// End of sGraph functions.


// Static function.
// Core test for gr_check_plot_events.
//
bool
plot_bag::check_event(GdkEvent *ev, sGraph *graph)
{
    if (!graph)
        return (false);
    plot_bag *w = dynamic_cast<plot_bag*>(graph->dev());
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


// Static function.
// Callback for PopUpInput() sensitivity set
//
void
plot_bag::sens_set(GTKbag *w, bool set, int)
{
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
}


//
// Event Handlers
//

// Static function.
int
plot_bag::resize(GtkWidget *caller, GdkEvent*, void *client_data)
{
    sGraph *graph = static_cast<sGraph*>(client_data);
    plot_bag *wb = dynamic_cast<plot_bag*>(graph->dev());

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
    return (true);
}


// Static function.
// The redraw is in a timeout so that plots with a lot of data and
// take time to render can be resized more easily.
int
plot_bag::redraw_timeout(void *arg)
{
    sGraph *graph = static_cast<sGraph*>(arg);
    plot_bag *wb = dynamic_cast<plot_bag*>(graph->dev());

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
}


// Static function.
int
#if GTK_CHECK_VERSION(3,0,0)
plot_bag::redraw(GtkWidget*, cairo_t *cr, void *client_data)
#else
plot_bag::redraw(GtkWidget*, GdkEvent *event, void *client_data)
#endif
{
    sGraph *graph = static_cast<sGraph*>(client_data);
    plot_bag *wb = dynamic_cast<plot_bag*>(graph->dev());

#if GTK_CHECK_VERSION(3,0,0)
    ndkDrawable *dw = wb->GetDrawable();

    bool dirty = dw->set_draw_to_pixmap();
    graph->area().set_width(dw->get_width());
    graph->area().set_height(dw->get_height());
    if (dirty)
        graph->set_dirty(true);

    if (graph->dirty()) {
        if (wb->pb_rdid)
            g_source_remove(wb->pb_rdid);
        wb->pb_rdid = g_timeout_add(250, redraw_timeout, client_data);
    }
    else {
        cairo_rectangle_int_t rect;
        ndkDrawable::redraw_area(cr, &rect);

        dw->copy_pixmap_to_window(wb->GC(), rect.x, rect.y,
            rect.width, rect.height);
        wb->GC()->set_clip_rectangle(&rect);
        wb->XorGC()->set_clip_rectangle(&rect);
        graph->gr_redraw_keyed();
        wb->GC()->set_clip_rectangle(0);
        wb->XorGC()->set_clip_rectangle(0);
    }

#else
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
#endif
    return (true);
}


// Static function.
int
plot_bag::motion(GtkWidget*, GdkEvent *event, void *client_data)
{
    sGraph *graph = static_cast<sGraph*>(client_data);
    plot_bag *w = dynamic_cast<plot_bag*>(graph->dev());
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
    return (true);
}


// Static function.
// Idle function to handle motion events.  The events are filtered to
// avoid cursor lag seen in GTK-2 with Pango and certain fonts.
//
int
plot_bag::motion_idle(void *arg)
{
    sGraph *graph = static_cast<sGraph*>(arg);
    plot_bag *w = dynamic_cast<plot_bag*>(graph->dev());
    if (w && w->Gbag()->showing_ghost()) {
        w->pb_id = 0;
        GP.PushGraphContext(graph);
        w->UndrawGhost();
        w->DrawGhost(w->pb_x, w->pb_y);
        GP.PopGraphContext();
    }
    return (0);
}


// Static function.
int
plot_bag::focus(GtkWidget*, GdkEvent*, void*)
{
    // Nothing to do but handle this to avoid default action of
    // issuing expose event 
    return (true); 
} 
 
 
// Static function.
int
plot_bag::keypress(GtkWidget*, GdkEvent *event, void *client_data)
{
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
    return (true);
}


// Static function.
int
plot_bag::buttonpress(GtkWidget *widget, GdkEvent *event, void *client_data)
{
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
        graph->set_cmdmode(graph->cmdmode() | grShiftMode);
    else
        graph->set_cmdmode(graph->cmdmode() & ~grShiftMode);
    if (buttonev->state & GDK_CONTROL_MASK)
        graph->set_cmdmode(graph->cmdmode() | grControlMode);
    else
        graph->set_cmdmode(graph->cmdmode() & ~grControlMode);
    graph->gr_bdown_hdlr(button, (int)buttonev->x,
        graph->yinv((int)buttonev->y));
    return (true);
}


// Static function.
int
plot_bag::buttonup(GtkWidget*, GdkEvent *event, void *client_data)
{
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
        graph->set_cmdmode(graph->cmdmode() | grShiftMode);
    else
        graph->set_cmdmode(graph->cmdmode() & ~grShiftMode);
    if (buttonev->state & GDK_CONTROL_MASK)
        graph->set_cmdmode(graph->cmdmode() | grControlMode);
    else
        graph->set_cmdmode(graph->cmdmode() & ~grControlMode);
    graph->gr_bup_hdlr(button, (int)buttonev->x,
        graph->yinv((int)buttonev->y));
    return (true);
}


// Static function.
int
plot_bag::enter_hdlr(GtkWidget *caller, GdkEvent*, void *client_data)
{
    // pointer entered a drawing window
    sGraph *graph = static_cast<sGraph*>(client_data);
    plot_bag *w = dynamic_cast<plot_bag*>(graph->dev());
    gtk_widget_set_can_focus(caller, true);
    gtk_window_set_focus(GTK_WINDOW(w->Shell()), caller);
    if (GP.SourceGraph() && graph != GP.SourceGraph()) {
        // hack a ghost for move/copy
        w->Gbag()->set_ghost_func(dynamic_cast<plot_bag*>(
            GP.SourceGraph()->dev())->Gbag()->get_ghost_func());
    }
    graph->dev()->ShowGhost(true);
    return (true);
}


// Static function.
// Gracefully terminate ghost drawing when the pointer leaves a
// window.
//
int
plot_bag::leave_hdlr(GtkWidget*, GdkEvent*, void *client_data)
{
    // pointer left the drawing window
    sGraph *graph = static_cast<sGraph*>(client_data);
    plot_bag *w = dynamic_cast<plot_bag*>(graph->dev());
    if (w->Gbag()->has_ghost()) {
        GP.PushGraphContext(graph);
        w->ShowGhost(false);
        GP.PopGraphContext();
    }
    if (GP.SourceGraph() && graph != GP.SourceGraph()) {
        // remove hacked ghost
        w->Gbag()->set_ghost_func(0);
    }
    return (true);
}


// Static function.
void
plot_bag::font_change_hdlr(GtkWidget*, void*, void *arg)
{
    sGraph *graph = (sGraph*)arg;
    graph->set_dirty(true);
}


//
// Signal Handlers
//

// Static function.
void
plot_bag::b_quit(GtkWidget*, void *client_data)
{
    sGraph *graph = static_cast<sGraph*>(client_data);
    graph->gr_popdown();
}


// Static function.
void
plot_bag::b_help(GtkWidget *caller, void *client_data)
{
    sGraph *graph = static_cast<sGraph*>(client_data);
    plot_bag *w = dynamic_cast<plot_bag*>(graph->dev());
    bool state = GTKdev::GetStatus(caller);
    if (state)
        w->PopUpHelp(graph->apptype() == GR_PLOT ? "plotpanel" : "mplotpanel");
}


// Static function.
void
plot_bag::b_recolor(GtkWidget*, void *client_data)
{
    sGraph *graph = static_cast<sGraph*>(client_data);
    graph->gr_pkg_init_colors();
    graph->dev()->Clear();
    graph->gr_redraw();
}


// Static function.
void
plot_bag::b_hardcopy(GtkWidget *caller, void *client_data)
{
    sGraph *graph = static_cast<sGraph*>(client_data);
    plot_bag *w = dynamic_cast<plot_bag*>(graph->dev());
    set_hccb(&wrsHCcb);
    w->PopUpPrint(caller, &wrsHCcb, HCgraphical, w);
}


// Static function.
void
plot_bag::b_save_plot(GtkWidget*, void *client_data)
{
    sGraph *graph = static_cast<sGraph*>(client_data);
    plot_bag *w = dynamic_cast<plot_bag*>(graph->dev());
    w->PopUpInput("Enter name for data file", 0, "Save Plot File",
        do_save_plot, client_data);
}


// Static function.
void
plot_bag::do_save_plot(const char *fnamein, void *client_data)
{
    sGraph *graph = static_cast<sGraph*>(client_data);
    plot_bag *w = dynamic_cast<plot_bag*>(graph->dev());
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
}


// Static function.
void
plot_bag::b_save_print(GtkWidget*, void *client_data)
{
    sGraph *graph = static_cast<sGraph*>(client_data);
    plot_bag *w = dynamic_cast<plot_bag*>(graph->dev());
    w->PopUpInput("Enter name for print file", 0, "Save Print File",
        do_save_print, client_data);
}


// Static function.
void
plot_bag::do_save_print(const char *fnamein, void *client_data)
{
    sGraph *graph = static_cast<sGraph*>(client_data);
    plot_bag *w = dynamic_cast<plot_bag*>(graph->dev());
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
}


// Static function.
void
plot_bag::b_points(GtkWidget *caller, void *client_data)
{
    // handle points and combplot modes
    sGraph *graph = static_cast<sGraph*>(client_data);
    plot_bag *w = dynamic_cast<plot_bag*>(graph->dev());

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
}


// Static function.
void
plot_bag::b_logx(GtkWidget *caller, void *client_data)
{
    sGraph *graph = static_cast<sGraph*>(client_data);
    plot_bag *w = dynamic_cast<plot_bag*>(graph->dev());
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
}


// Static function.
void
plot_bag::b_logy(GtkWidget *caller, void *client_data)
{
    sGraph *graph = static_cast<sGraph*>(client_data);
    plot_bag *w = dynamic_cast<plot_bag*>(graph->dev());
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
}


// Static function.
void
plot_bag::b_marker(GtkWidget *caller, void *client_data)
{
    sGraph *graph = static_cast<sGraph*>(client_data);
    plot_bag *w = dynamic_cast<plot_bag*>(graph->dev());
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
}


// Static function.
void
plot_bag::b_separate(GtkWidget *caller, void *client_data)
{
    sGraph *graph = static_cast<sGraph*>(client_data);
    plot_bag *w = dynamic_cast<plot_bag*>(graph->dev());
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
}


// Static function.
void
plot_bag::b_onescale(GtkWidget*, void *client_data)
{
    sGraph *graph = static_cast<sGraph*>(client_data);
    if (graph->format() == FT_MULTI)
        graph->set_format(FT_SINGLE);
    else
        graph->set_format(FT_MULTI);
    graph->dev()->Clear();
    graph->gr_redraw();
}


// Static function.
void
plot_bag::b_multiscale(GtkWidget *caller, void *client_data)
{
    // Handle one scale and grp scale modes.
    sGraph *graph = static_cast<sGraph*>(client_data);
    plot_bag *w = dynamic_cast<plot_bag*>(graph->dev());

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
}


// Static function.
// Procedure to initialize part of the structure passed to the
// hardcopy widget.
//
void
plot_bag::set_hccb(HCcb *cb)
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
plot_bag::get_dim(const char *str, double *val)
{
    char buf[128];
    int i = sscanf(str, "%lf%s", val, buf);
    if (i == 2 && buf[0] == 'c' && buf[1] == 'm')
        *val *= 2.54;
    if (i > 0)
        return (true);
    return (false);
}


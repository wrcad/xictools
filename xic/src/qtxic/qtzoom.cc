
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

#include "main.h"
#include "dsp_inlines.h"
#include "qtmain.h"
//#include "gtkinterf/gtkspinbtn.h"
//#include <gdk/gdkkeysyms.h>
#include <math.h>


//-----------------------------------------------------------------------------
//  Pop-up for the Zoom command
//
// Help system keywords used:
//  xic:zoom

//namespace qtzoom {
    struct cZoom : public GRpopup
    {
        cZoom(QTbag*, WindowDesc*);
        ~cZoom();

//        GtkWidget *shell() { return (zm_popup); }

        // GRpopup overrides
        /*
        void set_visible(bool visib)
            {
                if (visib)
                    gtk_widget_show(zm_popup);
                else
                    gtk_widget_hide(zm_popup);
            }
            */

        void popdown();
        void initialize();
        void update();

        /*
    private:
        void action_proc(GtkWidget*);

        static void zm_cancel_proc(GtkWidget*, void*);
        static void zm_action(GtkWidget*, void*);

        GtkWidget *zm_popup;
        GtkWidget *zm_autoy;
        GtkWidget *zm_yapply;
        GtkWidget *zm_zapply;
        GtkWidget *zm_wapply;

        WindowDesc *zm_window;

        GTKspinBtn sb_yscale;
        GTKspinBtn sb_zoom;
        GTKspinBtn sb_x;
        GTKspinBtn sb_y;
        GTKspinBtn sb_wid;
        */
    };
//}

//using namespace qtzoom;


cZoom::cZoom(QTbag *owner, WindowDesc *w)
{
#ifdef notdef
    p_parent = owner;
    zm_autoy = 0;
    zm_yapply = 0;
    zm_zapply = 0;
    zm_wapply = 0;
    zm_window = w;

    if (owner)
        owner->MonitorAdd(this);

    zm_popup = gtk_NewPopup(0, "Set Display Window", zm_cancel_proc, this);
    if (!zm_popup)
        return;
    gtk_window_set_resizable(GTK_WINDOW(zm_popup), false);

    GtkWidget *form = gtk_table_new(3, 1, false);
    gtk_widget_show(form);
    gtk_container_set_border_width(GTK_CONTAINER(form), 2);
    gtk_container_add(GTK_CONTAINER(zm_popup), form);
    int rowcnt = 0;

    //
    // label in frame plus help btn
    //
    GtkWidget *row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);
    GtkWidget *label = gtk_label_new("Set zoom factor or display window");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    GtkWidget *frame = gtk_frame_new(0);
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), label);
    gtk_box_pack_start(GTK_BOX(row), frame, true, true, 0);
    GtkWidget *button = gtk_button_new_with_label("Help");
    gtk_widget_set_name(button, "Help");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(zm_action), this);
    gtk_box_pack_end(GTK_BOX(row), button, false, false, 0);

    gtk_table_attach(GTK_TABLE(form), row, 0, 3, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);
    rowcnt++;

    if (w->IsXSect()) {
        // Showing cross-section, add a control set for the Y-scale.

        button = gtk_check_button_new_with_label("Auto Y-Scale");
        gtk_widget_set_name(button, "autoy");
        gtk_widget_show(button);
        g_signal_connect(G_OBJECT(button), "clicked",
            G_CALLBACK(zm_action), this);
        zm_autoy = button;

        gtk_table_attach(GTK_TABLE(form), button, 0, 3, rowcnt, rowcnt+1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
        rowcnt++;

        label = gtk_label_new("Y-Scale");
        gtk_widget_show(label);
        gtk_misc_set_padding(GTK_MISC(label), 2, 2);

        gtk_table_attach(GTK_TABLE(form), label, 0, 1, rowcnt, rowcnt+1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);

        GtkWidget *sb = sb_yscale.init(1.0, CDSCALEMIN, CDSCALEMAX, 5);
        gtk_widget_set_size_request(sb, 120, -1);

        gtk_table_attach(GTK_TABLE(form), sb, 1, 2, rowcnt, rowcnt+1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);

        zm_yapply = gtk_button_new_with_label("Apply");
        gtk_widget_set_name(zm_yapply, "yscale");
        gtk_widget_show(zm_yapply);
        g_signal_connect(G_OBJECT(zm_yapply), "clicked",
            G_CALLBACK(zm_action), this);

        gtk_table_attach(GTK_TABLE(form), zm_yapply, 2, 3, rowcnt, rowcnt+1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
        rowcnt++;

        GtkWidget *hsep = gtk_hseparator_new();
        gtk_widget_show(hsep);

        gtk_table_attach(GTK_TABLE(form), hsep, 0, 3, rowcnt, rowcnt+1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
        rowcnt++;
    }

    label = gtk_label_new("Zoom Factor");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);

    gtk_table_attach(GTK_TABLE(form), label, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    GtkWidget *sb = sb_zoom.init(1.0, CDSCALEMIN, CDSCALEMAX, 5);
    gtk_widget_set_size_request(sb, 120, -1);

    gtk_table_attach(GTK_TABLE(form), sb, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    zm_zapply = gtk_button_new_with_label("Apply");
    gtk_widget_set_name(zm_zapply, "zoom");
    gtk_widget_show(zm_zapply);
    g_signal_connect(G_OBJECT(zm_zapply), "clicked",
        G_CALLBACK(zm_action), this);

    gtk_table_attach(GTK_TABLE(form), zm_zapply, 2, 3, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    GtkWidget *hsep = gtk_hseparator_new();
    gtk_widget_show(hsep);

    gtk_table_attach(GTK_TABLE(form), hsep, 0, 3, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    label = gtk_label_new("Center X,Y");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);

    gtk_table_attach(GTK_TABLE(form), label, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    int ndgt = w->Mode() == Physical ? CD()->numDigits() : 3;

    sb = sb_x.init(0.0, -1e6, 1e6, ndgt);
    gtk_widget_set_size_request(sb, 120, -1);

    gtk_table_attach(GTK_TABLE(form), sb, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    sb = sb_y.init(0.0, -1e6, 1e6, ndgt);
    gtk_widget_set_size_request(sb, 120, -1);

    gtk_table_attach(GTK_TABLE(form), sb, 2, 3, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    label = gtk_label_new("Window Width");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);

    gtk_table_attach(GTK_TABLE(form), label, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    sb = sb_wid.init(100.0, 0.1, 1e6, ndgt);
    gtk_widget_set_size_request(sb, 120, -1);

    gtk_table_attach(GTK_TABLE(form), sb, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    zm_wapply = gtk_button_new_with_label("Apply");
    gtk_widget_set_name(zm_wapply, "window");
    gtk_widget_show(zm_wapply);
    g_signal_connect(G_OBJECT(zm_wapply), "clicked",
        G_CALLBACK(zm_action), this);

    gtk_table_attach(GTK_TABLE(form), zm_wapply, 2, 3, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    hsep = gtk_hseparator_new();
    gtk_widget_show(hsep);

    gtk_table_attach(GTK_TABLE(form), hsep, 0, 3, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    button = gtk_button_new_with_label("Dismiss");
    gtk_widget_set_name(button, "Dismiss");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(zm_cancel_proc), this);

    gtk_table_attach(GTK_TABLE(form), button, 0, 3, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    gtk_window_set_focus(GTK_WINDOW(zm_popup), button);

    update();
#endif
}


cZoom::~cZoom()
{
    /*
    if (p_parent) {
        GTKsubwin *owner = dynamic_cast<GTKsubwin*>(p_parent);
        if (owner)
            owner->MonitorRemove(this);
    }
    if (p_usrptr)
        *p_usrptr = 0;
    if (p_caller && !p_no_desel)
        GTKdev::Deselect(p_caller);
    if (zm_popup) {
        g_signal_handlers_disconnect_by_func(G_OBJECT(zm_popup),
            (gpointer)zm_cancel_proc, this);
        gtk_widget_destroy(zm_popup);
    }
    */
}


// GRpopup override
//
void
cZoom::popdown()
{
    /*
    if (!p_parent)
        return;
    GTKbag *owner = dynamic_cast<GTKbag*>(p_parent);
    if (owner)
        GTKdev::SetFocus(owner->Shell());
    if (!owner || !owner->MonitorActive(this))
        return;

    delete this;
    */
}


// Set positioning and transient-for property, call before setting
// visible after creation.
//
void
cZoom::initialize()
{
    /*
    GTKsubwin *w = dynamic_cast<GTKsubwin*>(p_parent);
    if (w && w->Shell()) {
        gtk_window_set_transient_for(GTK_WINDOW(zm_popup),
            GTK_WINDOW(w->Shell()));
        GTKdev::self()->SetPopupLocation(GRloc(), zm_popup, w->Viewport());
    }
    */
}


void
cZoom::update()
{
    /*
    int xc = (zm_window->Window()->left + zm_window->Window()->right)/2;
    int yc = (zm_window->Window()->bottom + zm_window->Window()->top)/2;
    int w = zm_window->Window()->width();

    if (zm_window->Mode() == Physical) {
        char *endp;
        const char *s = sb_x.get_string();
        double d = strtod(s, &endp);
        if (INTERNAL_UNITS(d) != xc)
            sb_x.set_value(MICRONS(xc));

        s = sb_y.get_string();
        d = strtod(s, &endp);
        if (INTERNAL_UNITS(d) != yc)
            sb_y.set_value(MICRONS(yc));

        s = sb_wid.get_string();
        d = strtod(s, &endp);
        if (INTERNAL_UNITS(d) != w)
            sb_wid.set_value(MICRONS(w));
    }
    else {
        char *endp;
        const char *s = sb_x.get_string();
        double d = strtod(s, &endp);
        if (ELEC_INTERNAL_UNITS(d) != xc)
            sb_x.set_value(ELEC_MICRONS(xc));

        s = sb_y.get_string();
        d = strtod(s, &endp);
        if (ELEC_INTERNAL_UNITS(d) != yc)
            sb_y.set_value(ELEC_MICRONS(yc));

        s = sb_wid.get_string();
        d = strtod(s, &endp);
        if (ELEC_INTERNAL_UNITS(d) != w)
            sb_wid.set_value(ELEC_MICRONS(w));
    }
    if (zm_window->IsXSect()) {
        GTKdev::SetStatus(zm_autoy, zm_window->IsXSectAutoY());
        sb_yscale.set_value(zm_window->XSectYScale());
    }
    */
}

#ifdef notdef

void
cZoom::action_proc(GtkWidget *caller)
{
    const char *name = gtk_widget_get_name(caller);
    if (!name)
        return;
    if (!strcmp(name, "Help")) {
        DSPmainWbag(PopUpHelp("xic:zoom"))
    }
    else if (!strcmp(name, "yscale")) {
        double ysc = sb_yscale.get_value();
        zm_window->SetXSectYScale(ysc);
        if (fabs(ysc - 1.0) < 1e-9)
            CDvdb()->clearVariable(VA_XSectYScale);
        else
            CDvdb()->setVariable(VA_XSectYScale, sb_yscale.get_string());

        bool autoy = GTKdev::GetStatus(zm_autoy);
        zm_window->SetXSectAutoY(autoy);
        if (autoy)
            CDvdb()->clearVariable(VA_XSectNoAutoY);
        else
            CDvdb()->setVariable(VA_XSectNoAutoY, "");
        zm_window->Redisplay(0);
    }
    else if (!strcmp(name, "zoom")) {
        const char *zstr = sb_zoom.get_string();
        char *endp;
        double d = strtod(zstr, &endp);
        if (endp == zstr)
            return;
        WindowDesc *wdesc = zm_window;
        d *= wdesc->Window()->width();
        wdesc->InitWindow(
            (wdesc->Window()->left + wdesc->Window()->right)/2,
            (wdesc->Window()->top + wdesc->Window()->bottom)/2, d);
        wdesc->Redisplay(0);
    }
    else if (!strcmp(name, "window")) {
        const char *zx =  sb_x.get_string();
        const char *zy =  sb_y.get_string();
        const char *zw =  sb_wid.get_string();
        char *endp;
        double dx = strtod(zx, &endp);
        if (endp == zx)
            return;
        double dy = strtod(zy, &endp);
        if (endp == zy)
            return;
        double dw = strtod(zw, &endp);
        if (endp == zw)
            return;
        WindowDesc *wdesc = zm_window;
        if (wdesc->Mode() == Physical) {
            wdesc->InitWindow(INTERNAL_UNITS(dx), INTERNAL_UNITS(dy),
                dw*CDphysResolution);
        }
        else {
            wdesc->InitWindow(ELEC_INTERNAL_UNITS(dx), ELEC_INTERNAL_UNITS(dy),
                dw*CDelecResolution);
        }
        wdesc->Redisplay(0);
    }
}


// Private static GTK signal handler.
void
cZoom::zm_cancel_proc(GtkWidget*, void *client_data)
{
    cZoom *zm = static_cast<cZoom*>(client_data);
    if (zm)
        zm->popdown();
}


// Private static GTK signal handler.
//
void
cZoom::zm_action(GtkWidget *widget, void *client_data)
{
    cZoom *zm = static_cast<cZoom*>(client_data);
    if (zm)
        zm->action_proc(widget);
}

#endif


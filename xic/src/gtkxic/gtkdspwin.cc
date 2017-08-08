
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
#include "cvrt.h"
#include "gtkmain.h"
#include "gtkinlines.h"
#include "gtkinterf/gtkspinbtn.h"


//-----------------------------------------------------------------------------
//  Pop-up for the CHD Display command
//

namespace {
    namespace gtkdspwin {
        struct sDw
        {
            sDw(GRobject, bool(*)(bool, const BBox*, void*), void*);
            ~sDw();

            GtkWidget *shell() { return (dw_popup); }

            void update(const BBox*);

        private:
            void action_proc(GtkWidget*);

            static void dw_cancel_proc(GtkWidget*, void*);
            static void dw_action(GtkWidget*, void*);

            GRobject dw_caller;
            GtkWidget *dw_popup;
            GtkWidget *dw_apply;
            GtkWidget *dw_center;

            WindowDesc *dw_window;
            bool (*dw_callback)(bool, const BBox*, void*);
            void *dw_arg;
            GTKspinBtn sb_x;
            GTKspinBtn sb_y;
            GTKspinBtn sb_wid;
        };

        sDw *Dw;
    }
}

using namespace gtkdspwin;


void
cConvert::PopUpDisplayWindow(GRobject caller, ShowMode mode, const BBox *BB,
    bool(*cb)(bool, const BBox*, void*), void *arg)
{
    if (!GRX || !mainBag())
        return;
    if (mode == MODE_OFF) {
        delete Dw;
        return;
    }
    if (mode == MODE_UPD) {
        if (Dw)
            Dw->update(BB);
        return;
    }
    if (Dw)
        return;

    new sDw(caller, cb, arg);
    if (!Dw->shell()) {
        delete Dw;
        Dw = 0;
        return;
    }
    Dw->update(BB);

    gtk_window_set_transient_for(GTK_WINDOW(Dw->shell()),
        GTK_WINDOW(mainBag()->Shell()));
    GRX->SetPopupLocation(GRloc(), Dw->shell(), mainBag()->Viewport());
    gtk_widget_show(Dw->shell());
}


sDw::sDw(GRobject caller, bool(*cb)(bool, const BBox*, void*), void *arg)
{
    Dw = this;
    dw_caller = caller;
    dw_apply = 0;
    dw_center = 0;
    dw_window = DSP()->MainWdesc();
    dw_callback = cb;
    dw_arg = arg;

    dw_popup = gtk_NewPopup(0, "Set Display Window", dw_cancel_proc, 0);
    if (!dw_popup)
        return;
    gtk_window_set_resizable(GTK_WINDOW(dw_popup), false);

    GtkWidget *form = gtk_table_new(3, 1, false);
    gtk_widget_show(form);
    gtk_container_set_border_width(GTK_CONTAINER(form), 2);
    gtk_container_add(GTK_CONTAINER(dw_popup), form);
    int rowcnt = 0;

    //
    // label in frame plus help btn
    //
    GtkWidget *label = gtk_label_new("Set area to display");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    GtkWidget *frame = gtk_frame_new(0);
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), label);
    gtk_table_attach(GTK_TABLE(form), frame, 0, 3, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);
    rowcnt++;

    label = gtk_label_new("Center X,Y");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);

    gtk_table_attach(GTK_TABLE(form), label, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    int ndgt = CD()->numDigits();

    GtkWidget *sb = sb_x.init(0.0, -1e6, 1e6, ndgt);
    gtk_widget_set_usize(sb, 120, -1);

    gtk_table_attach(GTK_TABLE(form), sb, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    sb = sb_y.init(0.0, -1e6, 1e6, ndgt);
    gtk_widget_set_usize(sb, 120, -1);

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

    sb = sb_wid.init(100.0, 0.1, 1e6, 2);
    gtk_widget_set_usize(sb, 120, -1);

    gtk_table_attach(GTK_TABLE(form), sb, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    dw_apply = gtk_button_new_with_label("Apply");
    gtk_widget_set_name(dw_apply, "window");
    gtk_widget_show(dw_apply);
    gtk_signal_connect(GTK_OBJECT(dw_apply), "clicked",
        GTK_SIGNAL_FUNC(dw_action), 0);

    gtk_table_attach(GTK_TABLE(form), dw_apply, 2, 3, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    GtkWidget *hsep = gtk_hseparator_new();
    gtk_widget_show(hsep);

    gtk_table_attach(GTK_TABLE(form), hsep, 0, 3, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    GtkWidget *hbox = gtk_hbox_new(false, 2);
    gtk_widget_show(hbox);

    dw_center = gtk_button_new_with_label("Center Full View");
    gtk_widget_set_name(dw_center, "center");
    gtk_widget_show(dw_center);
    gtk_signal_connect(GTK_OBJECT(dw_center), "clicked",
        GTK_SIGNAL_FUNC(dw_action), 0);
    gtk_box_pack_start(GTK_BOX(hbox), dw_center, true, true, 0);

    GtkWidget *button = gtk_button_new_with_label("Dismiss");
    gtk_widget_set_name(button, "Dismiss");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(dw_cancel_proc), 0);
    gtk_box_pack_start(GTK_BOX(hbox), button, true, true, 0);

    gtk_table_attach(GTK_TABLE(form), hbox, 0, 3, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    gtk_window_set_focus(GTK_WINDOW(dw_popup), button);
}


sDw::~sDw()
{
    Dw = 0;
    if (dw_caller)
        GRX->Deselect(dw_caller);
    if (dw_callback)
        (*dw_callback)(false, 0, dw_arg);
    if (dw_popup)
        gtk_widget_destroy(dw_popup);
}


void
sDw::update(const BBox *BB)
{
    if (!BB)
        return;
    int x = (BB->left + BB->right)/2;
    int y = (BB->bottom + BB->top)/2;
    int w = BB->width();
    sb_x.set_value(MICRONS(x));
    sb_y.set_value(MICRONS(y));
    sb_wid.set_value(MICRONS(w));
}


void
sDw::action_proc(GtkWidget *caller)
{
    const char *name = gtk_widget_get_name(caller);
    if (!name)
        return;
    if (!strcmp(name, "window")) {
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

        int wid2 = abs(INTERNAL_UNITS(dw)/2);
        int x = INTERNAL_UNITS(dx);
        int y = INTERNAL_UNITS(dy);

        BBox BB(x - wid2, y, x + wid2, y);
        if (dw_callback && !(*dw_callback)(true, &BB, dw_arg))
            return;
        Cvt()->PopUpDisplayWindow(0, MODE_OFF, 0, 0, 0);
    }
    if (!strcmp(name, "center")) {
        if (dw_callback && !(*dw_callback)(true, 0, dw_arg))
            return;
        Cvt()->PopUpDisplayWindow(0, MODE_OFF, 0, 0, 0);
    }
}


// Private static GTK signal handler.
void
sDw::dw_cancel_proc(GtkWidget*, void*)
{
    Cvt()->PopUpDisplayWindow(0, MODE_OFF, 0, 0, 0);
}


// Private static GTK signal handler.
//
void
sDw::dw_action(GtkWidget *widget, void*)
{
    if (Dw)
        Dw->action_proc(widget);
}


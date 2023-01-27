
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
#include "sced.h"
#include "sced_spiceipc.h"
#include "gtkmain.h"
#include "gtkinlines.h"
#include <signal.h>


//-----------------------------------------------------------------------------
// Popup for monitoring asynchronous simulation runs
//

namespace {
    namespace gtksim {
        struct sSim
        {
            sSim();

            void control(SpType);

            GtkWidget *shell()  { return (sp_shell); }

        private:
            static int sp_down_timer(void*);
            static void sp_cancel_proc(GtkWidget*, void*);
            static void sp_pause_proc(GtkWidget*, void*);
            static void sp_destroy_proc(GtkWidget*, void*);

            SpType sp_status;
            GtkWidget *sp_shell;
        };

        sSim Sim;
    }
}

using namespace gtksim;

namespace {
    int label_set_idle(void *arg)
    {
        const char *msg = (const char*)arg;
        GtkWidget *shell = Sim.shell();
        if (shell) {
            GtkWidget *label = (GtkWidget*)gtk_object_get_data(
                GTK_OBJECT(shell), "label");
            if (label) {
                gtk_label_set_text(GTK_LABEL(label), msg);
                if (!GTK_WIDGET_MAPPED(shell)) {
                    GRX->SetPopupLocation(GRloc(LW_LL), shell,
                        mainBag()->Viewport());
                    gtk_widget_show(shell);
                }
            }
        }
        return (0);
    }
}


// data set:
// popup            "label"             label
//
void
cSced::PopUpSim(SpType status)
{
    if (!GRX || !mainBag())
        return;
    Sim.control(status);
}
// End of cSced functions.


sSim::sSim()
{
    sp_status = SpNil;
    sp_shell = 0;
}


void
sSim::control(SpType status)
{
    const char *msg;
    switch (status) {
    case SpNil:
    default:
        if (sp_shell)
            gtk_widget_hide(sp_shell);
        sp_status = SpNil;
        return;
    case SpBusy:
        msg = "Running...";
        sp_status = SpBusy;
        break;
    case SpPause:
        msg = "Paused";
        sp_status = SpPause;
        GRX->AddTimer(2000, sp_down_timer, 0);
        break;
    case SpDone:
        msg = "Analysis Complete";
        sp_status = SpDone;
        GRX->AddTimer(2000, sp_down_timer, 0);
        break;
    case SpError:
        msg = "Error: connection broken";
        sp_status = SpError;
        break;
    }
    if (sp_shell) {
        gtk_idle_add(label_set_idle, (void*)msg);
        return;
    }
    main_bag *w = mainBag();
    GtkWidget *popup = gtk_NewPopup(w, "SPICE Run", sp_destroy_proc, 0);
    gtk_window_set_resizable(GTK_WINDOW(popup), false);
    gtk_widget_set_size_request(popup, 200, -1);

    // Prevent the pop-up from taking focus.  Otherwise, when it pops
    // down it will give focus back to the main window, causing iplots
    // to disappear below.
#ifndef __APPLE__
    // With these set as below in XQuartz 2.8.2 and OSX 13.0.1, the
    // buttons don't respond to presses, and when the widget pops down
    // it hides the main window.
    gtk_window_set_accept_focus(GTK_WINDOW(popup), false);
    gtk_window_set_focus_on_map(GTK_WINDOW(popup), false);
#endif

    GtkWidget *form = gtk_table_new(1, 3, false);
    gtk_widget_show(form);
    gtk_container_add(GTK_CONTAINER(popup), form);

    GtkWidget *label = gtk_label_new(msg);
    gtk_widget_show(label);
    GtkWidget *frame = gtk_frame_new(0);
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), label);
    gtk_object_set_data(GTK_OBJECT(popup), "label", label);

    gtk_table_attach(GTK_TABLE(form), frame, 0, 1, 0, 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);

    GtkWidget *sep = gtk_hseparator_new();
    gtk_widget_show(sep);
    gtk_table_attach(GTK_TABLE(form), sep, 0, 1, 1, 2,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    GtkWidget *hbox = gtk_hbox_new(false, 2);
    gtk_widget_show(hbox);

    GtkWidget *button = gtk_button_new_with_label("Pause");
    gtk_widget_set_name(button, "Pause");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(sp_pause_proc), w);
    gtk_box_pack_start(GTK_BOX(hbox), button, true, true, 0);

    button = gtk_button_new_with_label("Dismiss");
    gtk_widget_set_name(button, "Dismiss");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(sp_cancel_proc), w);
    gtk_box_pack_start(GTK_BOX(hbox), button, true, true, 0);

    gtk_table_attach(GTK_TABLE(form), hbox, 0, 1, 2, 3,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    gtk_window_set_focus(GTK_WINDOW(popup), button);

    sp_shell = popup;
    gtk_window_set_transient_for(GTK_WINDOW(popup), GTK_WINDOW(w->Shell()));
    GRX->SetPopupLocation(GRloc(LW_LL), popup, w->Viewport());
    gtk_widget_show(popup);
}


// Static function.
// Make the popup go away after an interval.
//
int
sSim::sp_down_timer(void*)
{
    if (Sim.sp_shell)
        gtk_widget_hide(Sim.sp_shell);
    return (false);
}


// Static function.
// Pop down the Spice popup.
//
void
sSim::sp_cancel_proc(GtkWidget*, void*)
{
    if (Sim.sp_shell)
        gtk_widget_hide(Sim.sp_shell);
}


// Static function.
// Tell the simulator to pause, if an analysis is in progress.
//
void
sSim::sp_pause_proc(GtkWidget*, void*)
{
    SCD()->spif()->InterruptSpice();
}


// Static function.
// Destroy the Spice popup
//
void
sSim::sp_destroy_proc(GtkWidget*, void*)
{
    if (Sim.sp_shell) {
        gtk_widget_destroy(Sim.sp_shell);
        Sim.sp_shell = 0;
        Sim.sp_status = SpNil;
    }
}


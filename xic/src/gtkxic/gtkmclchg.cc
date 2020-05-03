
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
#include "edit.h"
#include "edit_variables.h"
#include "dsp_inlines.h"
#include "menu.h"
#include "modf_menu.h"
#include "gtkmain.h"
#include "gtkinlines.h"


//-------------------------------------------------------------------------
// Pop-up to control the layer change option in move/copy.
//
// Help system keywords used:
//  xic:mclcg

namespace {
    namespace gtkmclchg {
        struct sLcg
        {
            sLcg();
            ~sLcg();

            void update();

            GtkWidget *shell() { return (lcg_popup); }

        private:
            static void lcg_cancel_proc(GtkWidget*, void*);
            static void lcg_action(GtkWidget*, void*);
            static void lcg_help(GtkWidget*, void*);

            GtkWidget *lcg_popup;
            GtkWidget *lcg_none;
            GtkWidget *lcg_cur;
            GtkWidget *lcg_all;
        };

        sLcg *Lcg;
    }
}

using namespace gtkmclchg;


void
cEdit::PopUpLayerChangeMode(ShowMode mode)
{
    if (!GRX || !mainBag())
        return;
    if (mode == MODE_OFF) {
        Menu()->MenuButtonSet("main", MenuMCLCG, false);
        delete Lcg;
        return;
    }
    if (mode == MODE_UPD) {
        if (Lcg)
            Lcg->update();
        return;
    }
    if (Lcg)
        return;

    new sLcg;
    if (!Lcg->shell()) {
        delete Lcg;
        return;
    }
    gtk_window_set_transient_for(GTK_WINDOW(Lcg->shell()),
        GTK_WINDOW(mainBag()->Shell()));

    GRX->SetPopupLocation(GRloc(), Lcg->shell(), mainBag()->Viewport());
    gtk_widget_show(Lcg->shell());
}


sLcg::sLcg()
{
    Lcg = this;
    lcg_none = 0;
    lcg_cur = 0;
    lcg_all = 0;

    lcg_popup = gtk_NewPopup(0, "Layer Change Mode", lcg_cancel_proc, 0);
    if (!lcg_popup)
        return;
    gtk_window_set_resizable(GTK_WINDOW(lcg_popup), false);

    GtkWidget *form = gtk_table_new(1, 1, false);
    gtk_widget_show(form);
    gtk_container_set_border_width(GTK_CONTAINER(form), 2);
    gtk_container_add(GTK_CONTAINER(lcg_popup), form);
    int rowcnt = 0;

    // label in frame plus help btn
    //
    GtkWidget *row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);
    GtkWidget *label = gtk_label_new(
        "Set layer change option for Move/Copy.");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    GtkWidget *frame = gtk_frame_new(0);
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), label);
    gtk_box_pack_start(GTK_BOX(row), frame, true, true, 0);
    GtkWidget *button = gtk_button_new_with_label("Help");
    gtk_widget_set_name(button, "Help");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(lcg_help), 0);
    gtk_box_pack_end(GTK_BOX(row), button, false, false, 0);

    gtk_table_attach(GTK_TABLE(form), row, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    lcg_none = gtk_radio_button_new_with_label(0, "Don't allow layer change.");
    gtk_widget_set_name(lcg_none, "none");
    gtk_widget_show(lcg_none);
    GSList *group = gtk_radio_button_group(GTK_RADIO_BUTTON(lcg_none));
    gtk_signal_connect(GTK_OBJECT(lcg_none), "clicked",
        GTK_SIGNAL_FUNC(lcg_action), 0);

    gtk_table_attach(GTK_TABLE(form), lcg_none, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    lcg_cur = gtk_radio_button_new_with_label(group,
        "Allow layer change for objects on current layer.  ");
    gtk_widget_set_name(lcg_cur, "norm");
    gtk_widget_show(lcg_cur);
    group = gtk_radio_button_group(GTK_RADIO_BUTTON(lcg_cur));
    gtk_signal_connect(GTK_OBJECT(lcg_cur), "clicked",
        GTK_SIGNAL_FUNC(lcg_action), 0);

    gtk_table_attach(GTK_TABLE(form), lcg_cur, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    lcg_all = gtk_radio_button_new_with_label(group,
        "Allow layer change for all objects.");
    gtk_widget_set_name(lcg_all, "all");
    gtk_widget_show(lcg_all);
    gtk_signal_connect(GTK_OBJECT(lcg_all), "clicked",
        GTK_SIGNAL_FUNC(lcg_action), 0);

    gtk_table_attach(GTK_TABLE(form), lcg_all, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    GtkWidget *sep = gtk_hseparator_new();
    gtk_widget_show(sep);
    gtk_table_attach(GTK_TABLE(form), sep, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    button = gtk_button_new_with_label("Dismiss");
    gtk_widget_set_name(button, "Dismiss");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(lcg_cancel_proc), 0);
    gtk_widget_set_size_request(button, 250, -1);

    gtk_table_attach(GTK_TABLE(form), button, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);
    gtk_window_set_focus(GTK_WINDOW(lcg_popup), button);

    update();
}


sLcg::~sLcg()
{
    Lcg = 0;
    if (lcg_popup)
        gtk_widget_destroy(lcg_popup);
}


void
sLcg::update()
{
    const char *v = CDvdb()->getVariable(VA_LayerChangeMode);
    if (!v) {
        GRX->SetStatus(lcg_none, true);
        GRX->SetStatus(lcg_cur, false);
        GRX->SetStatus(lcg_all, false);
    }
    else if (*v == 'a' || *v == 'A') {
        GRX->SetStatus(lcg_none, false);
        GRX->SetStatus(lcg_cur, false);
        GRX->SetStatus(lcg_all, true);
    }
    else {
        GRX->SetStatus(lcg_none, false);
        GRX->SetStatus(lcg_cur, true);
        GRX->SetStatus(lcg_all, false);
    }
}


// Static Function
void
sLcg::lcg_cancel_proc(GtkWidget*, void*)
{
    ED()->PopUpLayerChangeMode(MODE_OFF);
}


// Static function.
void
sLcg::lcg_help(GtkWidget*, void*)
{
    DSPmainWbag(PopUpHelp("xic:mclcg"))
}


// Static function.
void
sLcg::lcg_action(GtkWidget *caller, void*)
{
    if (!Lcg)
        return;
    if (!GRX->GetStatus(caller))
        return;
    if (caller == Lcg->lcg_none)
        CDvdb()->clearVariable(VA_LayerChangeMode);
    else if (caller == Lcg->lcg_cur)
        CDvdb()->setVariable(VA_LayerChangeMode, "");
    else if (caller == Lcg->lcg_all)
        CDvdb()->setVariable(VA_LayerChangeMode, "all");
}



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
#include "cvrt_variables.h"
#include "gtkmain.h"
#include "dsp_inlines.h"
#include "gtkinlines.h"
#include "gtkspinbtn.h"


//--------------------------------------------------------------------
// Pop-up to control pcell abutment and similar.
//
// Help system keywords used:
//  xic:pcctl

namespace {
    namespace gtkpcc {
        struct sPCc
        {
            sPCc(GRobject);
            ~sPCc();

            void update();

            GtkWidget *shell() { return (pcc_popup); }

        private:
            static void pcc_cancel_proc(GtkWidget*, void*);
            static void pcc_action(GtkWidget*, void*);
            static void pcc_abut_menu_proc(GtkWidget*, void*);
            static void pcc_val_changed(GtkWidget*, void*);

            GRobject pcc_caller;
            GtkWidget *pcc_popup;
            GtkWidget *pcc_abut;
            GtkWidget *pcc_hidestr;
            GtkWidget *pcc_listsm;
            GtkWidget *pcc_allwarn;

            GTKspinBtn sb_psz;
        };

        sPCc *PCc;
    }

    const char *abutvals[] =
    {
        "Mode 0 (no auto-abutment)",
        "Mode 1 (no contact)",
        "Mode 2 (with contact)",
        0
    };
}

using namespace gtkpcc;


void
cEdit::PopUpPCellCtrl(GRobject caller, ShowMode mode)
{
    if (!GRX || !mainBag())
        return;
    if (mode == MODE_OFF) {
        delete PCc;
        return;
    }
    if (mode == MODE_UPD) {
        if (PCc)
            PCc->update();
        return;
    }
    if (PCc)
        return;

    new sPCc(caller);
    if (!PCc->shell()) {
        delete PCc;
        return;
    }
    gtk_window_set_transient_for(GTK_WINDOW(PCc->shell()),
        GTK_WINDOW(mainBag()->Shell()));

    GRX->SetPopupLocation(GRloc(), PCc->shell(), mainBag()->Viewport());
    gtk_widget_show(PCc->shell());
}


sPCc::sPCc(GRobject c)
{
    PCc = this;
    pcc_caller = c;
    pcc_popup = 0;
    pcc_abut = 0;
    pcc_hidestr = 0;
    pcc_listsm = 0;
    pcc_allwarn = 0;

    pcc_popup = gtk_NewPopup(0, "PCell Control", pcc_cancel_proc, 0);
    if (!pcc_popup)
        return;
    gtk_window_set_resizable(GTK_WINDOW(pcc_popup), false);

    GtkWidget *form = gtk_table_new(2, 1, false);
    gtk_widget_show(form);
    gtk_container_set_border_width(GTK_CONTAINER(form), 2);
    gtk_container_add(GTK_CONTAINER(pcc_popup), form);
    int rowcnt = 0;

    //
    // label in frame plus help btn
    //
    GtkWidget *row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);
    GtkWidget *label = gtk_label_new("Control Parameterized Cell Options");
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
        GTK_SIGNAL_FUNC(pcc_action), 0);
    gtk_box_pack_end(GTK_BOX(row), button, false, false, 0);
    gtk_table_attach(GTK_TABLE(form), row, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);

    label = gtk_label_new("Auto-abutment mode");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_box_pack_start(GTK_BOX(row), label, true, true, 0);

    GtkWidget *entry = gtk_option_menu_new();
    gtk_widget_set_name(entry, "abutmenu");
    gtk_widget_show(entry);
    GtkWidget *menu = gtk_menu_new();
    gtk_widget_set_name(menu, "overmenu");
    for (int i = 0; abutvals[i]; i++) {
        GtkWidget *mi = gtk_menu_item_new_with_label(abutvals[i]);
        gtk_widget_set_name(mi, abutvals[i]);
        gtk_widget_show(mi);
        gtk_menu_append(GTK_MENU(menu), mi);
        gtk_signal_connect(GTK_OBJECT(mi), "activate",
            GTK_SIGNAL_FUNC(pcc_abut_menu_proc), (void*)abutvals[i]);
    }
    gtk_option_menu_set_menu(GTK_OPTION_MENU(entry), menu);
    gtk_box_pack_start(GTK_BOX(row), entry, true, true, 0);
    pcc_abut = entry;

    gtk_table_attach(GTK_TABLE(form), row, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    button = gtk_check_button_new_with_label(
        "Hide and disable stretch handles");
    gtk_widget_set_name(button, "hidestr");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(pcc_action), 0);
    gtk_table_attach(GTK_TABLE(form), button, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;
    pcc_hidestr = button;

    row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);

    label = gtk_label_new("Instance min. pixel size for stretch handles");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_box_pack_start(GTK_BOX(row), label, true, true, 0);

    GtkWidget *sb = sb_psz.init(DSP_MIN_FENCE_INST_PIXELS, 0, 1000, 0);
    sb_psz.connect_changed(GTK_SIGNAL_FUNC(pcc_val_changed), 0, 0);
    gtk_widget_set_usize(sb, 60, -1);
    gtk_box_pack_end(GTK_BOX(row), sb, false, false, 0);

    gtk_table_attach(GTK_TABLE(form), row, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    button = gtk_check_button_new_with_label(
        "List sub-masters as modified cells");
    gtk_widget_set_name(button, "listsm");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(pcc_action), 0);
    gtk_table_attach(GTK_TABLE(form), button, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;
    pcc_listsm = button;

    button = gtk_check_button_new_with_label(
        "Show all evaluation warnings");
    gtk_widget_set_name(button, "allwarn");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(pcc_action), 0);
    gtk_table_attach(GTK_TABLE(form), button, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;
    pcc_allwarn = button;

    //
    // Dismiss button
    //
    button = gtk_button_new_with_label("Dismiss");
    gtk_widget_set_name(button, "Dismiss");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(pcc_cancel_proc), 0);

    gtk_table_attach(GTK_TABLE(form), button, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);
    gtk_window_set_focus(GTK_WINDOW(pcc_popup), button);

    update();
}


sPCc::~sPCc()
{
    PCc = 0;
    if (pcc_caller)
        GRX->Deselect(pcc_caller);
    if (pcc_popup)
        gtk_widget_destroy(pcc_popup);
}


void
sPCc::update()
{
    const char *s = CDvdb()->getVariable(VA_PCellAbutMode);
    if (s && atoi(s) == 2)
        gtk_option_menu_set_history(GTK_OPTION_MENU(pcc_abut), 2);
    else if (s && atoi(s) == 0)
        gtk_option_menu_set_history(GTK_OPTION_MENU(pcc_abut), 0);
    else
        gtk_option_menu_set_history(GTK_OPTION_MENU(pcc_abut), 1);
    GRX->SetStatus(pcc_hidestr, CDvdb()->getVariable(VA_PCellHideGrips));
    GRX->SetStatus(pcc_listsm, CDvdb()->getVariable(VA_PCellListSubMasters));
    GRX->SetStatus(pcc_allwarn, CDvdb()->getVariable(VA_PCellShowAllWarnings));

    int d;
    const char *str = CDvdb()->getVariable(VA_PCellGripInstSize);
    if (str && sscanf(str, "%d", &d) == 1 && d >= 0 && d <= 1000)
        ;
    else
        d = DSP_MIN_FENCE_INST_PIXELS;
    sb_psz.set_value(d);
}


// Static function.
void
sPCc::pcc_cancel_proc(GtkWidget*, void*)
{
    ED()->PopUpPCellCtrl(0, MODE_OFF);
}


// Static function.
void
sPCc::pcc_action(GtkWidget *caller, void*)
{
    if (!PCc)
        return;
    const char *name = gtk_widget_get_name(caller);
    if (!strcmp(name, "Help")) {
        DSPmainWbag(PopUpHelp("xic:pcctl"))
        return;
    }

    bool state = GRX->GetStatus(caller);
    if (!strcmp(name, "hidestr")) {
        if (state)
            CDvdb()->setVariable(VA_PCellHideGrips, "");
        else
            CDvdb()->clearVariable(VA_PCellHideGrips);
    }
    else if (!strcmp(name, "listsm")) {
        if (state)
            CDvdb()->setVariable(VA_PCellListSubMasters, "");
        else
            CDvdb()->clearVariable(VA_PCellListSubMasters);
    }
    else if (!strcmp(name, "allwarn")) {
        if (state)
            CDvdb()->setVariable(VA_PCellShowAllWarnings, "");
        else
            CDvdb()->clearVariable(VA_PCellShowAllWarnings);
    }
}


// Static function.
void
sPCc::pcc_abut_menu_proc(GtkWidget*, void *client_data)
{
    const char *s = (const char*)client_data;
    if (s == abutvals[0])
        CDvdb()->setVariable(VA_PCellAbutMode, "0");
    else if (s == abutvals[1])
        CDvdb()->clearVariable(VA_PCellAbutMode);
    else if (s == abutvals[2])
        CDvdb()->setVariable(VA_PCellAbutMode, "2");
}


// Static function.
void
sPCc::pcc_val_changed(GtkWidget*, void*)
{
    if (!PCc)
        return;
    const char *s = PCc->sb_psz.get_string();
    char *endp;
    int d = (int)strtod(s, &endp);
    if (endp > s && d >= 0 && d <= 1000) {
        if (d == DSP_MIN_FENCE_INST_PIXELS)
            CDvdb()->clearVariable(VA_PCellGripInstSize);
        else {
            char buf[32];
            sprintf(buf, "%d", d);
            CDvdb()->setVariable(VA_PCellGripInstSize, buf);
        }
    }
}


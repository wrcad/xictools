
/*========================================================================*
 *                                                                        *
 *  Whiteley Research Inc, Sunnyvale, CA USA, http://wrcad.com            *
 *  Copyright (C) 2017 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *  Licensed under the Apache License, Version 2.0 (the "License");       *
 *  you may not use this file except in compliance with the License.      *
 *  You may obtain a copy of the License at                               *
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
#include "cd_compare.h"
#include "dsp_inlines.h"
#include "gtkmain.h"
#include "gtkinlines.h"


//-----------------------------------------------------------------------------
// The Custom Property Filter Setup pop-up, called from the Compare
// Layouts panel.
//
// Help system keywords used:
//  xic:prpfilt

namespace {
    namespace gtkpfilt {
        struct sPflt
        {
            sPflt(GRobject);
            ~sPflt();

            void update();

            GtkWidget *shell() { return (pf_popup); }

        private:
            static void pf_cancel_proc(GtkWidget*, void*);
            static void pf_action(GtkWidget*, void*);

            GRobject pf_caller;
            GtkWidget *pf_popup;
            GtkWidget *pf_phys_cell;
            GtkWidget *pf_phys_inst;
            GtkWidget *pf_phys_obj;
            GtkWidget *pf_elec_cell;
            GtkWidget *pf_elec_inst;
            GtkWidget *pf_elec_obj;
        };

        sPflt *Pflt;
    }
}

using namespace gtkpfilt;


void
cConvert::PopUpPropertyFilter(GRobject caller, ShowMode mode)
{
    if (!GRX || !mainBag())
        return;
    if (mode == MODE_OFF) {
        delete Pflt;
        return;
    }
    if (mode == MODE_UPD) {
        if (Pflt)
            Pflt->update();
        return;
    }
    if (Pflt)
        return;

    new sPflt(caller);
    if (!Pflt->shell()) {
        delete Pflt;
        return;
    }
    gtk_window_set_transient_for(GTK_WINDOW(Pflt->shell()),
        GTK_WINDOW(mainBag()->Shell()));

    GRX->SetPopupLocation(GRloc(LW_UR), Pflt->shell(), mainBag()->Viewport());
    gtk_widget_show(Pflt->shell());
}


sPflt::sPflt(GRobject c)
{
    Pflt = this;
    pf_caller = c;
    pf_phys_cell = 0;
    pf_phys_inst = 0;
    pf_phys_obj = 0;
    pf_elec_cell = 0;
    pf_elec_inst = 0;
    pf_elec_obj = 0;

    pf_popup = gtk_NewPopup(0, "Custom Property Filter Setup",
        pf_cancel_proc, 0);
    if (!pf_popup)
        return;

    GtkWidget *form = gtk_table_new(2, 1, false);
    gtk_widget_show(form);
    gtk_container_set_border_width(GTK_CONTAINER(form), 2);
    gtk_container_add(GTK_CONTAINER(pf_popup), form);
    int rowcnt = 0;

    //
    // physical strings
    //
    GtkWidget *hbox = gtk_hbox_new(false, 2);
    gtk_widget_show(hbox);
    GtkWidget *label = gtk_label_new("Physical property filter strings");
    gtk_widget_show(label);
    gtk_box_pack_start(GTK_BOX(hbox), label, false, false, 0);

    GtkWidget *button = gtk_button_new_with_label("Help");
    gtk_widget_set_name(button, "Help");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(pf_action), (void*)10L);
    gtk_box_pack_end(GTK_BOX(hbox), button, false, false, 0);

    gtk_table_attach(GTK_TABLE(form), hbox, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    label = gtk_label_new("Cell");
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(form), label, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)GTK_SHRINK,
        (GtkAttachOptions)0, 2, 2);

    pf_phys_cell = gtk_entry_new();
    gtk_widget_show(pf_phys_cell);
    gtk_signal_connect(GTK_OBJECT(pf_phys_cell), "changed",
        GTK_SIGNAL_FUNC(pf_action), (void*)0L);
    gtk_table_attach(GTK_TABLE(form), pf_phys_cell, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    label = gtk_label_new("Inst");
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(form), label, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)GTK_SHRINK,
        (GtkAttachOptions)0, 2, 2);

    pf_phys_inst = gtk_entry_new();
    gtk_widget_show(pf_phys_inst);
    gtk_signal_connect(GTK_OBJECT(pf_phys_inst), "changed",
        GTK_SIGNAL_FUNC(pf_action), (void*)1L);
    gtk_table_attach(GTK_TABLE(form), pf_phys_inst, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    label = gtk_label_new("Obj");
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(form), label, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)GTK_SHRINK,
        (GtkAttachOptions)0, 2, 2);

    pf_phys_obj = gtk_entry_new();
    gtk_widget_show(pf_phys_obj);
    gtk_signal_connect(GTK_OBJECT(pf_phys_obj), "changed",
        GTK_SIGNAL_FUNC(pf_action), (void*)2L);
    gtk_table_attach(GTK_TABLE(form), pf_phys_obj, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // electrical strings
    //
    hbox = gtk_hbox_new(false, 2);
    gtk_widget_show(hbox);
    label = gtk_label_new("Electrical property filter strings");
    gtk_widget_show(label);
    gtk_box_pack_start(GTK_BOX(hbox), label, false, false, 0);
    gtk_table_attach(GTK_TABLE(form), hbox, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    label = gtk_label_new("Cell");
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(form), label, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)GTK_SHRINK,
        (GtkAttachOptions)0, 2, 2);

    pf_elec_cell = gtk_entry_new();
    gtk_widget_show(pf_elec_cell);
    gtk_signal_connect(GTK_OBJECT(pf_elec_cell), "changed",
        GTK_SIGNAL_FUNC(pf_action), (void*)3L);
    gtk_table_attach(GTK_TABLE(form), pf_elec_cell, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    label = gtk_label_new("Inst");
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(form), label, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)GTK_SHRINK,
        (GtkAttachOptions)0, 2, 2);

    pf_elec_inst = gtk_entry_new();
    gtk_widget_show(pf_elec_inst);
    gtk_signal_connect(GTK_OBJECT(pf_elec_inst), "changed",
        GTK_SIGNAL_FUNC(pf_action), (void*)4L);
    gtk_table_attach(GTK_TABLE(form), pf_elec_inst, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    label = gtk_label_new("Obj");
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(form), label, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)GTK_SHRINK,
        (GtkAttachOptions)0, 2, 2);

    pf_elec_obj = gtk_entry_new();
    gtk_widget_show(pf_elec_obj);
    gtk_signal_connect(GTK_OBJECT(pf_elec_obj), "changed",
        GTK_SIGNAL_FUNC(pf_action), (void*)5L);
    gtk_table_attach(GTK_TABLE(form), pf_elec_obj, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // dismiss button
    //
    button = gtk_button_new_with_label("Dismiss");
    gtk_widget_set_name(button, "Dismiss");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(pf_cancel_proc), 0);

    gtk_table_attach(GTK_TABLE(form), button, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);
    gtk_window_set_focus(GTK_WINDOW(pf_popup), button);

    gtk_widget_set_usize(pf_popup, 350, -1);
    update();
}


sPflt::~sPflt()
{
    Pflt = 0;
    if (pf_caller)
        GRX->Deselect(pf_caller);
    if (pf_popup)
        gtk_widget_destroy(pf_popup);
}


void
sPflt::update()
{
    const char *s1 = CDvdb()->getVariable(VA_PhysPrpFltCell);
    const char *s2 = gtk_entry_get_text(GTK_ENTRY(pf_phys_cell));
    if (s1 && strcmp(s1, s2))
        gtk_entry_set_text(GTK_ENTRY(pf_phys_cell), s1);
    else if (!s1)
        gtk_entry_set_text(GTK_ENTRY(pf_phys_cell), "");

    s1 = CDvdb()->getVariable(VA_PhysPrpFltInst);
    s2 = gtk_entry_get_text(GTK_ENTRY(pf_phys_inst));
    if (s1 && strcmp(s1, s2))
        gtk_entry_set_text(GTK_ENTRY(pf_phys_inst), s1);
    else if (!s1)
        gtk_entry_set_text(GTK_ENTRY(pf_phys_inst), "");

    s1 = CDvdb()->getVariable(VA_PhysPrpFltObj);
    s2 = gtk_entry_get_text(GTK_ENTRY(pf_phys_obj));
    if (s1 && strcmp(s1, s2))
        gtk_entry_set_text(GTK_ENTRY(pf_phys_obj), s1);
    else if (!s1)
        gtk_entry_set_text(GTK_ENTRY(pf_phys_obj), "");

    s1 = CDvdb()->getVariable(VA_ElecPrpFltCell);
    s2 = gtk_entry_get_text(GTK_ENTRY(pf_elec_cell));
    if (s1 && strcmp(s1, s2))
        gtk_entry_set_text(GTK_ENTRY(pf_elec_cell), s1);
    else if (!s1)
        gtk_entry_set_text(GTK_ENTRY(pf_elec_cell), "");

    s1 = CDvdb()->getVariable(VA_ElecPrpFltInst);
    s2 = gtk_entry_get_text(GTK_ENTRY(pf_elec_inst));
    if (s1 && strcmp(s1, s2))
        gtk_entry_set_text(GTK_ENTRY(pf_elec_inst), s1);
    else if (!s1)
        gtk_entry_set_text(GTK_ENTRY(pf_elec_inst), "");

    s1 = CDvdb()->getVariable(VA_ElecPrpFltObj);
    s2 = gtk_entry_get_text(GTK_ENTRY(pf_elec_obj));
    if (s1 && strcmp(s1, s2))
        gtk_entry_set_text(GTK_ENTRY(pf_elec_obj), s1);
    else if (!s1)
        gtk_entry_set_text(GTK_ENTRY(pf_elec_obj), "");
}


// Static function.
void
sPflt::pf_cancel_proc(GtkWidget*, void*)
{
    Cvt()->PopUpPropertyFilter(0, MODE_OFF);
}


// Static function.
void
sPflt::pf_action(GtkWidget *caller, void *arg)
{
    if (!Pflt)
        return;
    if (arg == (void*)10L) {
        DSPmainWbag(PopUpHelp("xic:prpfilt"))
        return;
    }
    const char *s = gtk_entry_get_text(GTK_ENTRY(caller));
    if (arg == (void*)0L) {
        if (!*s)
            CDvdb()->clearVariable(VA_PhysPrpFltCell);
        else
            CDvdb()->setVariable(VA_PhysPrpFltCell, s);
    }
    else if (arg == (void*)1L) {
        if (!*s)
            CDvdb()->clearVariable(VA_PhysPrpFltInst);
        else
            CDvdb()->setVariable(VA_PhysPrpFltInst, s);
    }
    else if (arg == (void*)2L) {
        if (!*s)
            CDvdb()->clearVariable(VA_PhysPrpFltObj);
        else
            CDvdb()->setVariable(VA_PhysPrpFltObj, s);
    }
    else if (arg == (void*)3L) {
        if (!*s)
            CDvdb()->clearVariable(VA_ElecPrpFltCell);
        else
            CDvdb()->setVariable(VA_ElecPrpFltCell, s);
    }
    else if (arg == (void*)4L) {
        if (!*s)
            CDvdb()->clearVariable(VA_ElecPrpFltInst);
        else
            CDvdb()->setVariable(VA_ElecPrpFltInst, s);
    }
    else if (arg == (void*)5L) {
        if (!*s)
            CDvdb()->clearVariable(VA_ElecPrpFltObj);
        else
            CDvdb()->setVariable(VA_ElecPrpFltObj, s);
    }
}



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
#include "fio.h"
#include "fio_cvt_base.h"
#include "dsp_tkif.h"
#include "dsp_inlines.h"
#include "cvrt.h"
#include "qtmain.h"
#include "qtmenu.h"


//--------------------------------------------------------------------------
// This variation handles the cells one at a time.  The return value
// is 1<<Physical | 1<<Electrical where a set flag indicates overwrite.
// Call with mode = MODE_OFF after all processing to finish up.
//

/*
namespace {
    void
    start_modal(QWidget *w)
    {
        QTmenu::self()->SetSensGlobal(false);
        QTmenu::self()->SetModal(w);
        QTpkg::self()->SetOverrideBusy(true);
        DSPmainDraw(ShowGhost(ERASE))
    }


    void
    end_modal()
    {
        QTmenu::self()->SetModal(0);
        QTmenu::self()->SetSensGlobal(true);
        QTpkg::self()->SetOverrideBusy(false);
        DSPmainDraw(ShowGhost(DISPLAY))
    }
}
*/


namespace {
    namespace qtmerge {
        struct sMC
        {
            sMC(mitem_t*);
            ~sMC();

            //GtkWidget *shell() { return (mc_popup); }
            bool is_hidden() { return (mc_allflag); }

            void query(mitem_t*);
            bool set_apply_to_all();
            bool refresh(mitem_t*);

        private:
            //static void mc_cancel_proc(GtkWidget*, void*);
            //static void mc_btn_proc(GtkWidget*, void*);

            //GtkWidget *mc_popup;
            //GtkWidget *mc_label;
            SymTab *mc_names;
            bool mc_allflag;        // user pressed "apply to all"
            bool mc_do_phys;
            bool mc_do_elec;
        };

        sMC *MC;

        enum { MC_nil, MC_apply, MC_abort, MC_readP, MC_readE, MC_skipP,
            MC_skipE };
    }
}

using namespace qtmerge;


// MODE_ON:   Pop up the dialog, and desensitize application.
// MODE_UPD:  Delete the dialog, and sensitize application, but
//            keep MC.
// MODE_OFF:  Destroy MC, and do the MODE_UPD stuff if not done.
//
bool
cConvert::PopUpMergeControl(ShowMode mode, mitem_t *mi)
{
    if (!QTdev::exists() || !QTmainwin::exists())
        return (true);
    if (mode == MODE_OFF) {
        if (MC && !MC->is_hidden()) {
            if (QTdev::self()->LoopLevel() > 1)
                QTdev::self()->BreakLoop();
//            end_modal();
        }
        delete MC;
        return (true);
    }
    if (mode == MODE_UPD) {
        // We get here when the user presses "apply to all".  Hide the
        // widget and break out of modality.

        if (MC && MC->set_apply_to_all()) {
            if (QTdev::self()->LoopLevel() > 1)
                QTdev::self()->BreakLoop();
//            end_modal();
        }
        return (true);
    }
    if (MC) {
        if (MC->refresh(mi))
            return (true);
    }
    else {
        if (!FIO()->IsMergeControlEnabled())
            // If the popup is not enabled, return the default.
            return (true);

        new sMC(mi);
        if (MC->is_hidden())
            return (true);

//        QTdev::self()->SetPopupLocation(GRloc(LW_LL), MC->shell(),
//            QTmainwin::self()->Viewport());
//        start_modal(MC->shell());
    }

    QTdev::self()->MainLoop();  // wait for user's response

    if (MC)
        MC->query(mi);
    return (true);
}


sMC::sMC(mitem_t *mi)
{
    MC = this;
//    mc_popup = 0;
//    mc_label = 0;
    mc_names = new SymTab(true, false);
    mc_allflag = false;
    mc_do_phys = false;
    mc_do_elec = false;

    mc_names->add(lstring::copy(mi->name), (void*)1, false);

    mc_do_phys = !FIO()->IsNoOverwritePhys();
    mc_do_elec = !FIO()->IsNoOverwriteElec();
    if (CDvdb()->getVariable(VA_NoAskOverwrite) ||
            XM()->RunMode() != ModeNormal) {
        mc_allflag = true;
        query(mi);
        return;
    }

/*
    mc_popup = gtk_NewPopup(0, "Symbol Merge", mc_cancel_proc, 0);
    if (!mc_popup) {
        mc_allflag = true;
        query(mi);
        return;
    }
    gtk_window_set_resizable(GTK_WINDOW(mc_popup), false);
    gtk_BlackHoleFix(mc_popup);

    GtkWidget *form = gtk_table_new(3, 3, false);
    gtk_container_add(GTK_CONTAINER(mc_popup), form);
    gtk_widget_show(form);

    GtkWidget *frame = gtk_frame_new(0);
    gtk_widget_show(frame);
    char buf[256];
    snprintf(buf, sizeof(buf), "Cell: %s", mi->name);
    mc_label = gtk_label_new(buf);
    gtk_widget_show(mc_label);
    gtk_container_add(GTK_CONTAINER(frame), mc_label);

    gtk_table_attach(GTK_TABLE(form), frame, 0, 2, 0, 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    GtkWidget *vbox = gtk_vbox_new(false, 2);
    gtk_widget_show(vbox);

    GtkWidget *button =
        gtk_check_button_new_with_label("Overwrite Physical");
    gtk_widget_set_name(button, "OverwritePhysical");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(mc_btn_proc), (void*)MC_readP);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), mc_do_phys);
    gtk_box_pack_start(GTK_BOX(vbox), button, false, false, 0);

    button = gtk_check_button_new_with_label("Overwrite Electrical");
    gtk_widget_set_name(button, "ReadAllElectrical");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(mc_btn_proc), (void*)MC_readE);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),
        mc_do_elec);
    gtk_box_pack_start(GTK_BOX(vbox), button, false, false, 0);

    gtk_table_attach(GTK_TABLE(form), vbox, 0, 1, 1, 2,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);

    GtkWidget *hbox = gtk_hbox_new(false, 2);
    gtk_widget_show(hbox);

    button = gtk_button_new_with_label("Apply");
    gtk_widget_set_name(button, "Apply");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(mc_btn_proc), (void*)MC_apply);
    gtk_box_pack_start(GTK_BOX(hbox), button, true, true, 0);

    button = gtk_button_new_with_label("Apply To Rest");
    gtk_widget_set_name(button, "ApplyToRest");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(mc_btn_proc), (void*)MC_abort);
    gtk_box_pack_start(GTK_BOX(hbox), button, true, true, 0);

    gtk_table_attach(GTK_TABLE(form), hbox, 0, 2, 2, 3,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    // Constrain overall widget width so title text isn't truncated.
    gtk_widget_set_size_request(mc_popup, 220, -1);
*/
}


sMC::~sMC()
{
    MC = 0;
    delete mc_names;
//    delete mc_popup;
}


void
sMC::query(mitem_t *mi)
{
    mi->overwrite_phys = mc_do_phys;
    mi->overwrite_elec = mc_do_elec;
}


bool
sMC::set_apply_to_all()
{
    if (!mc_allflag) {
        mc_allflag = true;
//        gtk_widget_hide(mc_popup);
        return (true);
    }
    return (false);
}


bool
sMC::refresh(mitem_t *mi)
{
    if (SymTab::get(mc_names, mi->name) != ST_NIL) {
        // we've seen this cell before
        mi->overwrite_phys = false;
        mi->overwrite_elec = false;
        return (true);
    }
    mc_names->add(lstring::copy(mi->name), (void*)1, false);
    if (mc_allflag) {
        query(mi);
        return (true);
    }
    char buf[256];
    snprintf(buf, sizeof(buf), "Cell: %s", mi->name);
//    gtk_label_set_text(GTK_LABEL(mc_label), buf);
    return (false);
}


/*
// Static function.
void
sMC::mc_cancel_proc(GtkWidget*, void*)
{
    Cvt()->PopUpMergeControl(MODE_UPD, 0);
}


// Static function.
void
sMC::mc_btn_proc(GtkWidget *caller, void *client_data)
{
    if (!MC)
        return;
    int mode = (intptr_t)client_data;
    if (mode == MC_apply) {
        if (GTKdev::self()->LoopLevel() > 1)
            GTKdev::self()->BreakLoop();
    }
    else if (mode == MC_abort)
        Cvt()->PopUpMergeControl(MODE_UPD, 0);
    else if (mode == MC_readP)
        MC->mc_do_phys = GTKdev::GetStatus(caller);
    else if (mode == MC_readE)
        MC->mc_do_elec = GTKdev::GetStatus(caller);
}
*/


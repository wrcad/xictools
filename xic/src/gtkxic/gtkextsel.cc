
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
#include "ext.h"
#include "sced.h"
#include "gtkmain.h"
#include "gtkspinbtn.h"
#include "gtkinlines.h"
#include "dsp_inlines.h"
#include "menu.h"
#include "ext_menu.h"
#include "ext_pathfinder.h"
#include "promptline.h"


//-------------------------------------------------------------------------
// Pop-up to control group/node/path selections
//
// Help system keywords used:
//  xic:exsel


namespace {
    namespace gtkextsel {
        struct sES : public gtk_bag
        {
            sES(GRobject);
            ~sES();

            void update();

        private:
            void set_sens();

            static void es_cancel_proc(GtkWidget*, void*);
            static void es_action_proc(GtkWidget*, void*);
            static void es_val_changed(GtkWidget*, void*);
            static int es_redraw_idle(void*);
            static void es_menu_proc(GtkWidget*, void*);

            GRobject es_caller;
            GtkWidget *es_gnsel;
            GtkWidget *es_paths;
            GtkWidget *es_qpath;
            GtkWidget *es_gpmnu;
            GtkWidget *es_qpconn;
            GtkWidget *es_blink;
            GtkWidget *es_subpath;
            GtkWidget *es_antenna;
            GtkWidget *es_zoid;
            GtkWidget *es_tofile;
            GtkWidget *es_vias;
            GtkWidget *es_vtree;
            GtkWidget *es_rlab;
            GtkWidget *es_terms;
            GtkWidget *es_meas;

            GTKspinBtn sb_depth;
        };

        sES *ES;
    }
}

using namespace gtkextsel;


void
cExt::PopUpSelections(GRobject caller, ShowMode mode)
{
    if (!GRX || !mainBag())
        return;
    if (mode == MODE_OFF) {
        delete ES;
        return;
    }
    if (mode == MODE_UPD) {
        if (ES)
            ES->update();
        return;
    }
    if (ES)
        return;

    new sES(caller);
    if (!ES->Shell()) {
        delete ES;
        return;
    }
    gtk_window_set_transient_for(GTK_WINDOW(ES->Shell()),
        GTK_WINDOW(mainBag()->Shell()));

    GRX->SetPopupLocation(GRloc(LW_UL), ES->Shell(), mainBag()->Viewport());
    gtk_widget_show(ES->Shell());
}


sES::sES(GRobject caller)
{
    ES = this;
    es_caller = caller;
    es_gnsel = 0;
    es_paths = 0;
    es_qpath = 0;
    es_gpmnu = 0;
    es_qpconn = 0;
    es_blink = 0;
    es_subpath = 0;
    es_antenna = 0;
    es_zoid = 0;
    es_tofile = 0;
    es_vias = 0;
    es_vtree = 0;
    es_rlab = 0;
    es_terms = 0;
    es_meas = 0;

    wb_shell = gtk_NewPopup(0, "Path Selection Control", es_cancel_proc, 0);
    if (!wb_shell)
        return;
    gtk_window_set_resizable(GTK_WINDOW(wb_shell), false);

    GtkWidget *form = gtk_table_new(1, 1, false);
    gtk_widget_show(form);
    gtk_container_set_border_width(GTK_CONTAINER(form), 2);
    gtk_container_add(GTK_CONTAINER(wb_shell), form);
    int rowcnt = 0;

    //
    // top button row
    //
    GtkWidget *hbox = gtk_hbox_new(false, 2);
    gtk_widget_show(hbox);

    GtkWidget *button = gtk_toggle_button_new_with_label("Select Group/Node");
    gtk_widget_set_name(button, "gnsel");
    gtk_widget_show(button);
    es_gnsel = button;
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(es_action_proc), 0);
    gtk_box_pack_start(GTK_BOX(hbox), button, true, true, 0);

    button = gtk_toggle_button_new_with_label("Select Path");
    gtk_widget_set_name(button, "paths");
    gtk_widget_show(button);
    es_paths = button;
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(es_action_proc), 0);
    gtk_box_pack_start(GTK_BOX(hbox), button, true, true, 0);

    button = gtk_toggle_button_new_with_label("\"Quick\" Path");
    gtk_widget_set_name(button, "qpath");
    gtk_widget_show(button);
    es_qpath = button;
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(es_action_proc), 0);
    gtk_box_pack_start(GTK_BOX(hbox), button, true, true, 0);

    button = gtk_button_new_with_label("Help");
    gtk_widget_set_name(button, "help");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(es_action_proc), 0);
    gtk_box_pack_start(GTK_BOX(hbox), button, true, true, 0);

    gtk_table_attach(GTK_TABLE(form), hbox, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // quick paths ground plane method
    //
    hbox = gtk_hbox_new(false, 2);
    gtk_widget_show(hbox);

    GtkWidget *label = gtk_label_new("\"Quick\" Path gound plane handling");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_box_pack_start(GTK_BOX(hbox), label, true, true, 0);

    es_gpmnu = gtk_option_menu_new();
    gtk_widget_set_name(es_gpmnu, "qpgp");
    gtk_widget_show(es_gpmnu);
    GtkWidget *menu = gtk_menu_new();
    gtk_widget_set_name(menu, "qpgp");

    GtkWidget *mi = gtk_menu_item_new_with_label(
        "Use ground plane if available");
    gtk_widget_set_name(mi, "menu0");
    gtk_widget_show(mi);
    gtk_menu_append(GTK_MENU(menu), mi);
    gtk_signal_connect(GTK_OBJECT(mi), "activate",
        GTK_SIGNAL_FUNC(es_menu_proc), (void*)0);

    mi = gtk_menu_item_new_with_label("Create and use ground plane");
    gtk_widget_set_name(mi, "menu1");
    gtk_widget_show(mi);
    gtk_menu_append(GTK_MENU(menu), mi);
    gtk_signal_connect(GTK_OBJECT(mi), "activate",
        GTK_SIGNAL_FUNC(es_menu_proc), (void*)1);

    mi = gtk_menu_item_new_with_label("Never use ground plane");
    gtk_widget_set_name(mi, "menu2");
    gtk_widget_show(mi);
    gtk_menu_append(GTK_MENU(menu), mi);
    gtk_signal_connect(GTK_OBJECT(mi), "activate",
        GTK_SIGNAL_FUNC(es_menu_proc), (void*)2);

    gtk_option_menu_set_menu(GTK_OPTION_MENU(es_gpmnu), menu);
    gtk_box_pack_start(GTK_BOX(hbox), es_gpmnu, false, false, 0);

    gtk_table_attach(GTK_TABLE(form), hbox, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // depth control line
    //
    hbox = gtk_hbox_new(false, 2);
    gtk_widget_show(hbox);

    label = gtk_label_new("Path search depth");
    gtk_widget_show(label);
    gtk_box_pack_start(GTK_BOX(hbox), label, false, false, 0);

    GtkWidget *sb = sb_depth.init(CDMAXCALLDEPTH, 0.0, CDMAXCALLDEPTH, 0);
    sb_depth.connect_changed(GTK_SIGNAL_FUNC(es_val_changed), 0);
    gtk_widget_set_usize(sb, 50, -1);
    gtk_box_pack_start(GTK_BOX(hbox), sb, false, false, 0);

    button = gtk_button_new_with_label("0");
    gtk_widget_set_name(button, "d0");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(es_action_proc), 0);
    gtk_box_pack_start(GTK_BOX(hbox), button, false, false, 0);

    button = gtk_button_new_with_label("all");
    gtk_widget_set_name(button, "all");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(es_action_proc), 0);
    gtk_box_pack_start(GTK_BOX(hbox), button, false, false, 0);

    button = gtk_check_button_new_with_label("\"Quick\" Path use Conductor");
    gtk_widget_show(button);
    gtk_widget_set_name(button, "QPCndr");
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(es_action_proc), 0);
    gtk_box_pack_end(GTK_BOX(hbox), button, false, false, 0);
    es_qpconn = button;

    gtk_table_attach(GTK_TABLE(form), hbox, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // check boxes
    //
    hbox = gtk_hbox_new(false, 2);
    gtk_widget_show(hbox);

    button = gtk_check_button_new_with_label("Blink highlighting");
    gtk_widget_set_name(button, "blink");
    gtk_widget_show(button);
    es_blink = button;
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(es_action_proc), 0);
    gtk_box_pack_start(GTK_BOX(hbox), button, true, true, 0);

    button = gtk_check_button_new_with_label("Enable sub-path selection");
    gtk_widget_set_name(button, "subpath");
    gtk_widget_show(button);
    es_subpath = button;
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(es_action_proc), 0);
    gtk_box_pack_end(GTK_BOX(hbox), button, true, true, 0);

    gtk_table_attach(GTK_TABLE(form), hbox, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // botton button row
    //
    hbox = gtk_hbox_new(false, 2);
    gtk_widget_show(hbox);

    button = gtk_button_new_with_label("Load Antenna file");
    gtk_widget_set_name(button, "antenna");
    gtk_widget_show(button);
    es_antenna = button;
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(es_action_proc), 0);
    gtk_box_pack_start(GTK_BOX(hbox), button, true, true, 0);

    button = gtk_button_new_with_label("To trapezoids");
    gtk_widget_set_name(button, "zoid");
    gtk_widget_show(button);
    es_zoid = button;
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(es_action_proc), 0);
    gtk_box_pack_start(GTK_BOX(hbox), button, true, true, 0);

    button = gtk_button_new_with_label("Save path to file");
    gtk_widget_set_name(button, "tofile");
    gtk_widget_show(button);
    es_tofile = button;
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(es_action_proc), 0);
    gtk_box_pack_start(GTK_BOX(hbox), button, true, true, 0);

    gtk_table_attach(GTK_TABLE(form), hbox, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // Path file vias check boxes.
    //
    hbox = gtk_hbox_new(false, 2);
    gtk_widget_show(hbox);

    button = gtk_check_button_new_with_label("Path file contains vias");
    gtk_widget_set_name(button, "vias");
    gtk_widget_show(button);
    es_vias = button;
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(es_action_proc), 0);
    gtk_box_pack_start(GTK_BOX(hbox), button, true, true, 0);

    button = gtk_check_button_new_with_label(
        "Path file contains via check layers");
    gtk_widget_set_name(button, "vtree");
    gtk_widget_show(button);
    es_vtree = button;
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(es_action_proc), 0);
    gtk_box_pack_end(GTK_BOX(hbox), button, true, true, 0);

    gtk_table_attach(GTK_TABLE(form), hbox, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // resistance measurememt
    //
    hbox = gtk_hbox_new(false, 2);
    gtk_widget_show(hbox);

    label = gtk_label_new("Resistance measurement");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_box_pack_start(GTK_BOX(hbox), label, true, true, 0);
    es_rlab = label;

    button = gtk_toggle_button_new_with_label("Define terminals");
    gtk_widget_set_name(button, "terms");
    gtk_widget_show(button);
    es_terms = button;
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(es_action_proc), 0);
    gtk_box_pack_start(GTK_BOX(hbox), button, true, true, 0);

    button = gtk_button_new_with_label("Measure");
    gtk_widget_set_name(button, "meas");
    gtk_widget_show(button);
    es_meas = button;
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(es_action_proc), 0);
    gtk_box_pack_start(GTK_BOX(hbox), button, true, true, 0);

    gtk_table_attach(GTK_TABLE(form), hbox, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // dismiss button
    //
    GtkWidget *sep = gtk_hseparator_new();
    gtk_widget_show(sep);
    gtk_table_attach(GTK_TABLE(form), sep, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    button = gtk_button_new_with_label("Dismiss");
    gtk_widget_set_name(button, "Dismiss");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(es_cancel_proc), 0);

    gtk_table_attach(GTK_TABLE(form), button, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);
    gtk_window_set_focus(GTK_WINDOW(wb_shell), button);

    update();
}


sES::~sES()
{
    if (GRX->GetStatus(es_gnsel))
        GRX->CallCallback(es_gnsel);
    else if (GRX->GetStatus(es_paths))
        GRX->CallCallback(es_paths);
    else if (GRX->GetStatus(es_qpath))
        GRX->CallCallback(es_qpath);

    ES = 0;
    if (es_caller)
        GRX->Deselect(es_caller);
    // Needed to unset the Click-Select Mode button.
    SCD()->PopUpNodeMap(0, MODE_UPD);

    if (wb_shell)
        gtk_signal_disconnect_by_func(GTK_OBJECT(wb_shell),
            GTK_SIGNAL_FUNC(es_cancel_proc), wb_shell);
}


void
sES::update()
{
    gtk_option_menu_set_history(GTK_OPTION_MENU(es_gpmnu),
        EX()->quickPathMode());
    GRX->SetStatus(es_qpconn, EX()->isQuickPathUseConductor());
    GRX->SetStatus(es_blink, EX()->isBlinkSelections());
    if (GRX->GetStatus(es_gnsel))
        GRX->SetStatus(es_paths, EX()->isGNShowPath());
    else
        GRX->SetStatus(es_subpath, EX()->isSubpathEnabled());
    sb_depth.set_value(EX()->pathDepth());
    set_sens();

    const char *vstr = CDvdb()->getVariable(VA_PathFileVias);
    if (!vstr) {
        GRX->SetStatus(es_vias, false);
        GRX->SetStatus(es_vtree, false);
        gtk_widget_set_sensitive(es_vtree, false);
    }
    else if (!*vstr) {
        GRX->SetStatus(es_vias, true);
        GRX->SetStatus(es_vtree, false);
        gtk_widget_set_sensitive(es_vtree, true);
    }
    else {
        GRX->SetStatus(es_vias, true);
        GRX->SetStatus(es_vtree, true);
        gtk_widget_set_sensitive(es_vtree, true);
    }
}


void
sES::set_sens()
{
    pathfinder *pf = EX()->pathFinder(cExt::PFget);
    bool has_path = (pf && !pf->is_empty());

    if (has_path && (GRX->GetStatus(es_gnsel) || GRX->GetStatus(es_paths) ||
            GRX->GetStatus(es_qpath))) {
        gtk_widget_set_sensitive(es_zoid, true);
        gtk_widget_set_sensitive(es_tofile, true);
        gtk_widget_set_sensitive(es_rlab, true);
        gtk_widget_set_sensitive(es_terms, true);

        Blist *t = EX()->terminals();
        if (t && t->next)
            gtk_widget_set_sensitive(es_meas, true);
        else
            gtk_widget_set_sensitive(es_meas, false);
    }
    else {
        gtk_widget_set_sensitive(es_zoid, false);
        gtk_widget_set_sensitive(es_tofile, false);
        gtk_widget_set_sensitive(es_rlab, false);
        gtk_widget_set_sensitive(es_terms, false);
        gtk_widget_set_sensitive(es_meas, false);
    }
    if (!GRX->GetStatus(es_gnsel) && GRX->GetStatus(es_paths))
        gtk_widget_set_sensitive(es_antenna, true);
    else
        gtk_widget_set_sensitive(es_antenna, false);

    if (DSP()->CurMode() == Electrical) {
        gtk_widget_set_sensitive(es_qpath, false);
        if (!GRX->GetStatus(es_gnsel))
            gtk_widget_set_sensitive(es_paths, false);

    }
    else {
        gtk_widget_set_sensitive(es_qpath, true);
        gtk_widget_set_sensitive(es_paths, true);
    }
}


// Static function.
void
sES::es_cancel_proc(GtkWidget*, void*)
{
    EX()->PopUpSelections(0, MODE_OFF);
}


namespace {
    void
    tof_cb(const char *name, void*)
    {
        if (name && *name) {
            // Maybe write out the vias, too.
            const char *vstr = CDvdb()->getVariable(VA_PathFileVias);
            if (EX()->saveCurrentPathToFile(name, (vstr != 0))) {
                const char *msg = "Path saved to native cell file.";
                if (ES)
                    ES->PopUpMessage(msg, false);
                else
                    PL()->ShowPrompt(msg);
            }
            else {
                const char *msg = "Operation failed.";
                if (ES)
                    ES->PopUpMessage(msg, false);
                else
                    PL()->ShowPrompt(msg);
            }
            if (ES && ES->ActiveInput())
                ES->ActiveInput()->popdown();
        }
    }
}


// Static function.
void
sES::es_action_proc(GtkWidget *caller, void*)
{
    if (!ES)
        return;
    const char *name = gtk_widget_get_name(caller);
    if (!name)
        return;
    if (!strcmp(name, "gnsel")) {
        EX()->selectGroupNode(caller);
        if (GRX->GetStatus(caller)) {
            GRX->Deselect(ES->es_subpath);
            gtk_widget_set_sensitive(ES->es_subpath, false);
            gtk_widget_set_sensitive(ES->es_paths, true);
        }
        else {
            GRX->SetStatus(ES->es_subpath, EX()->isSubpathEnabled());
            gtk_widget_set_sensitive(ES->es_subpath, true);
            GRX->Deselect(ES->es_paths);
            if (DSP()->CurMode() == Electrical)
                gtk_widget_set_sensitive(ES->es_paths, false);
        }
        ES->set_sens();
    }
    else if (!strcmp(name, "paths")) {
        if (GRX->GetStatus(ES->es_gnsel))
            EX()->selectShowPath(GRX->GetStatus(caller));
        else
            EX()->selectPath(caller);
        ES->set_sens();
    }
    else if (!strcmp(name, "qpath")) {
        EX()->selectPathQuick(caller);
        ES->set_sens();
    }
    else if (!strcmp(name, "help"))
        DSPmainWbag(PopUpHelp("xic:exsel"))
    else if (!strcmp(name, "d0"))
        ES->sb_depth.set_value(0);
    else if (!strcmp(name, "all"))
        ES->sb_depth.set_value(CDMAXCALLDEPTH);
    else if (!strcmp(name, "QPCndr")) {
        if (GRX->GetStatus(caller))
            CDvdb()->setVariable(VA_QpathUseConductor, "");
        else
            CDvdb()->clearVariable(VA_QpathUseConductor);
    }
    else if (!strcmp(name, "blink")) {
        EX()->setBlinkSelections(GRX->GetStatus(caller));
        pathfinder *pf = EX()->pathFinder(cExt::PFget);
        if (pf) {
            pf->show_path(0, ERASE);
            pf->show_path(0, DISPLAY);
        }
    }
    else if (!strcmp(name, "subpath"))
        EX()->setSubpathEnabled(GRX->GetStatus(caller));
    else if (!strcmp(name, "antenna"))
        EX()->getAntennaPath();
    else if (!strcmp(name, "zoid")) {
        pathfinder *pf = EX()->pathFinder(cExt::PFget);
        if (pf) {
            pf->show_path(0, ERASE);
            pf->atomize_path();
            pf->show_path(0, DISPLAY);
        }
        gtk_widget_set_sensitive(ES->es_zoid, false);
    }
    else if (!strcmp(name, "tofile")) {
        pathfinder *pf = EX()->pathFinder(cExt::PFget);
        if (pf) {
            char buf[256];
            sprintf(buf, "%s_grp_%s", Tstring(DSP()->CurCellName()),
                pf->pathname());
            ES->PopUpInput("Enter native cell name", buf, "Save Cell",
                tof_cb, 0);
        }
        else
            PL()->ShowPrompt("No current path!");
    }
    else if (!strcmp(name, "vias")) {
        if (GRX->GetStatus(caller)) {
            CDvdb()->setVariable(VA_PathFileVias, "");
            gtk_widget_set_sensitive(ES->es_vtree, true);
        }
        else {
            CDvdb()->clearVariable(VA_PathFileVias);
            GRX->SetStatus(ES->es_vtree, false);
            gtk_widget_set_sensitive(ES->es_vtree, false);
        }
    }
    else if (!strcmp(name, "vtree")) {
        if (GRX->GetStatus(caller))
            CDvdb()->setVariable(VA_PathFileVias, "check");
        else
            CDvdb()->setVariable(VA_PathFileVias, "");
    }
    else if (!strcmp(name, "terms"))
        EX()->editTerminals(caller);
    else if (!strcmp(name, "meas")) {
        double *vals;
        int sz;
        if (EX()->extractNetResistance(&vals, &sz, 0))
            PL()->ShowPromptV("Measured resistance: %g", vals[0]);
        else
            PL()->ShowPromptV("Failed: %s", Errs()->get_error());
    }
}


// Static function.
int
sES::es_redraw_idle(void *)
{
    if (GRX->GetStatus(ES->es_gnsel))
        EX()->selectRedrawPath();
    else if (GRX->GetStatus(ES->es_paths) || GRX->GetStatus(ES->es_qpath))
        EX()->redrawPath();
    return (0);
}


// Static function.
void
sES::es_val_changed(GtkWidget*, void*)
{
    if (!ES)
        return;
    int d = ES->sb_depth.get_value_as_int();
    EX()->setPathDepth(d);

    // Redraw the path in an idle proc, otherwise spin button
    // behaves strangely.
    dspPkgIf()->RegisterIdleProc(es_redraw_idle, 0);
}


// Static function.
void
sES::es_menu_proc(GtkWidget*, void *arg)
{
    if (!ES)
        return;
    long code = (long)arg;
    if (code == 0)
        CDvdb()->clearVariable(VA_QpathGroundPlane);
    else if (code == 1)
        CDvdb()->setVariable(VA_QpathGroundPlane, "1");
    else if (code == 2)
        CDvdb()->setVariable(VA_QpathGroundPlane, "2");
}



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
#include "menu.h"
#include "tech.h"
#include "promptline.h"
#include "errorlog.h"
#include "gtkmain.h"
#include "miscutil/filestat.h"


//-------------------------------------------------------------------------
// Pop-up to control writing of a technology file
//
// Help system keywords used:
//  xic:updat

namespace {
    namespace gtktech {
        struct sTc
        {
            sTc(GRobject);
            ~sTc();

            void update();

            GtkWidget *shell() { return (tc_popup); }

        private:
            static void tc_cancel_proc(GtkWidget*, void*);
            static void tc_action(GtkWidget*, void*);

            GRobject tc_caller;
            GtkWidget *tc_popup;
            GtkWidget *tc_none;
            GtkWidget *tc_cmt;
            GtkWidget *tc_use;
            GtkWidget *tc_entry;
            GtkWidget *tc_write;
        };

        sTc *Tc;
    }
}

using namespace gtktech;


void
cMain::PopUpTechWrite(GRobject caller, ShowMode mode)
{
    if (!GTKdev::exists() || !GTKmainwin::exists())
        return;
    if (mode == MODE_OFF) {
        delete Tc;
        return;
    }
    if (mode == MODE_UPD) {
        if (Tc)
            Tc->update();
        return;
    }
    if (Tc)
        return;

    new sTc(caller);
    if (!Tc->shell()) {
        delete Tc;
        return;
    }
    gtk_window_set_transient_for(GTK_WINDOW(Tc->shell()),
        GTK_WINDOW(GTKmainwin::self()->Shell()));

    GTKdev::self()->SetPopupLocation(GRloc(), Tc->shell(),
        GTKmainwin::self()->Viewport());
    gtk_widget_show(Tc->shell());
}


sTc::sTc(GRobject caller)
{
    Tc = this;
    tc_caller = caller;
    tc_none = 0;
    tc_cmt = 0;
    tc_use = 0;
    tc_entry = 0;
    tc_write = 0;

    tc_popup = gtk_NewPopup(0, "Write Tech File", tc_cancel_proc, 0);
    if (!tc_popup)
        return;
    gtk_window_set_resizable(GTK_WINDOW(tc_popup), false);

    GtkWidget *form = gtk_table_new(1, 1, false);
    gtk_widget_show(form);
    gtk_container_set_border_width(GTK_CONTAINER(form), 2);
    gtk_container_add(GTK_CONTAINER(tc_popup), form);
    int rowcnt = 0;

    tc_none = gtk_radio_button_new_with_label(0, "Omit default definitions");
    gtk_widget_set_name(tc_none, "none");
    gtk_widget_show(tc_none);
    GSList *group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(tc_none));
    g_signal_connect(G_OBJECT(tc_none), "clicked",
        G_CALLBACK(tc_action), 0);
    GtkWidget *row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);
    gtk_box_pack_start(GTK_BOX(row), tc_none, true, true, 0);

    GtkWidget *button = gtk_button_new_with_label("Help");
    gtk_widget_set_name(button, "Help");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(tc_action), 0);
    gtk_box_pack_end(GTK_BOX(row), button, false, false, 0);

    gtk_table_attach(GTK_TABLE(form), row, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    tc_cmt = gtk_radio_button_new_with_label(group,
        "Comment default definitions");
    gtk_widget_set_name(tc_cmt, "cmt");
    gtk_widget_show(tc_cmt);
    group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(tc_cmt));
    g_signal_connect(G_OBJECT(tc_cmt), "clicked",
        G_CALLBACK(tc_action), 0);

    gtk_table_attach(GTK_TABLE(form), tc_cmt, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    tc_use = gtk_radio_button_new_with_label(group,
        "Include default definitions");
    gtk_widget_set_name(tc_use, "all");
    gtk_widget_show(tc_use);
    g_signal_connect(G_OBJECT(tc_use), "clicked",
        G_CALLBACK(tc_action), 0);

    gtk_table_attach(GTK_TABLE(form), tc_use, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    GtkWidget *label = gtk_label_new("Tech File");
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(form), label, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    char string[256];
    if (Tech()->TechExtension() && *Tech()->TechExtension())
        snprintf(string, sizeof(string), "./%s.%s",
            XM()->TechFileBase(), Tech()->TechExtension());
    else
        snprintf(string, sizeof(string), "./%s", XM()->TechFileBase());

    tc_entry = gtk_entry_new();
    gtk_widget_show(tc_entry);
    gtk_entry_set_text(GTK_ENTRY(tc_entry), string);
    gtk_table_attach(GTK_TABLE(form), tc_entry, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    GtkWidget *sep = gtk_hseparator_new();
    gtk_widget_show(sep);
    gtk_table_attach(GTK_TABLE(form), sep, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    button = gtk_button_new_with_label("Write File");
    gtk_widget_set_name(button, "write");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(tc_action), 0);
    gtk_table_attach(GTK_TABLE(form), button, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);
    tc_write = button;

    button = gtk_button_new_with_label("Dismiss");
    gtk_widget_set_name(button, "Dismiss");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(tc_cancel_proc), 0);
    gtk_table_attach(GTK_TABLE(form), button, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);

    gtk_widget_set_size_request(tc_popup, 250, -1);
    gtk_window_set_focus(GTK_WINDOW(tc_popup), tc_entry);

    update();
}


sTc::~sTc()
{
    Tc = 0;
    if (tc_caller)
        GTKdev::Deselect(tc_caller);
    if (tc_popup)
        gtk_widget_destroy(tc_popup);
}


void
sTc::update()
{
    const char *v = CDvdb()->getVariable(VA_TechPrintDefaults);
    if (!v) {
        GTKdev::SetStatus(tc_none, true);
        GTKdev::SetStatus(tc_cmt, false);
        GTKdev::SetStatus(tc_use, false);
    }
    else if (!*v) {
        GTKdev::SetStatus(tc_none, false);
        GTKdev::SetStatus(tc_cmt, true);
        GTKdev::SetStatus(tc_use, false);
    }
    else {
        GTKdev::SetStatus(tc_none, false);
        GTKdev::SetStatus(tc_cmt, false);
        GTKdev::SetStatus(tc_use, true);
    }
}


// Static Function
void
sTc::tc_cancel_proc(GtkWidget*, void*)
{
    XM()->PopUpTechWrite(0, MODE_OFF);
}


// Static function.
void
sTc::tc_action(GtkWidget *caller, void*)
{
    if (!Tc)
        return;
    const char *nm = gtk_widget_get_name(caller);
    if (nm && !strcmp(nm, "Help")) {
        DSPmainWbag(PopUpHelp("xic:updat"))
        return;
    }
    if (!GTKdev::GetStatus(caller))
        return;
    if (caller == Tc->tc_none)
        CDvdb()->clearVariable(VA_TechPrintDefaults);
    else if (caller == Tc->tc_cmt)
        CDvdb()->setVariable(VA_TechPrintDefaults, "");
    else if (caller == Tc->tc_use)
        CDvdb()->setVariable(VA_TechPrintDefaults, "all");
    else if (caller == Tc->tc_write) {
        const char *s = CDvdb()->getVariable(VA_TechPrintDefaults);
        if (s) {
            if (*s)
                Tech()->SetPrintMode(TCPRall);
            else
                Tech()->SetPrintMode(TCPRcmt);
        }
        else
            Tech()->SetPrintMode(TCPRnondef);

        const char *string = gtk_entry_get_text(GTK_ENTRY(Tc->tc_entry));

        // make a backup of the present file
        if (!filestat::create_bak(string)) {
            GTKpkg::self()->ErrPrintf(ET_ERROR, "%s", filestat::error_msg());
            PL()->ShowPromptV("Update of %s failed.", string);
            return;
        }

        FILE *techfp;
        if ((techfp = filestat::open_file(string, "w")) == 0) {
            Log()->ErrorLog(mh::Initialization, filestat::error_msg());
            return;
        }
        HCcb &cb = Tech()->HC();
        DSPmainWbag(HCupdate(&cb, 0))
        // If in hardcopy mode, switch back temporarily.
        int save_drvr = Tech()->HcopyDriver();
        if (DSP()->DoingHcopy())
            XM()->HCswitchMode(false, true, Tech()->HcopyDriver());

        fprintf(techfp, "# update from %s\n", XM()->IdString());

        Tech()->Print(techfp);

        fclose(techfp);
        if (DSP()->DoingHcopy())
            XM()->HCswitchMode(true, true, save_drvr);

        PL()->ShowPromptV("Current attributes written to new %s file.",
            string);
        XM()->PopUpTechWrite(0, MODE_OFF);
    }
}


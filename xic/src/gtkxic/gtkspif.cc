
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
#include "edit_variables.h"
#include "cvrt_variables.h"
#include "gtkmain.h"
#include "dsp_inlines.h"
#include "gtkinlines.h"


//--------------------------------------------------------------------
// Pop-up to control SPICE interface.
//
// Help system keywords used:
//  xic:spif

namespace {
    namespace gtkscd {
        struct sSC
        {
            sSC(GRobject);
            ~sSC();

            void update();

            GtkWidget *shell() { return (sc_popup); }

        private:
            static void sc_cancel_proc(GtkWidget*, void*);
            static void sc_action(GtkWidget*, void*);

            GRobject sc_caller;
            GtkWidget *sc_popup;
            GtkWidget *sc_listall;
            GtkWidget *sc_checksol;
            GtkWidget *sc_notools;
            GtkWidget *sc_alias;
            GtkWidget *sc_alias_b;
            GtkWidget *sc_hostname;
            GtkWidget *sc_hostname_b;
            GtkWidget *sc_dispname;
            GtkWidget *sc_dispname_b;
            GtkWidget *sc_progname;
            GtkWidget *sc_progname_b;
            GtkWidget *sc_execdir;
            GtkWidget *sc_execdir_b;
            GtkWidget *sc_execname;
            GtkWidget *sc_execname_b;
            GtkWidget *sc_catchar;
            GtkWidget *sc_catchar_b;
            GtkWidget *sc_catmode;
            GtkWidget *sc_catmode_b;
        };

        sSC *SC;
    }
}

using namespace gtkscd;


void
cSced::PopUpSpiceIf(GRobject caller, ShowMode mode)
{
    if (!GRX || !mainBag())
        return;
    if (mode == MODE_OFF) {
        delete SC;
        return;
    }
    if (mode == MODE_UPD) {
        if (SC)
            SC->update();
        return;
    }
    if (SC)
        return;

    new sSC(caller);
    if (!SC->shell()) {
        delete SC;
        return;
    }
    gtk_window_set_transient_for(GTK_WINDOW(SC->shell()),
        GTK_WINDOW(mainBag()->Shell()));

    GRX->SetPopupLocation(GRloc(LW_UL), SC->shell(), mainBag()->Viewport());
    gtk_widget_show(SC->shell());
}


sSC::sSC(GRobject c)
{
    SC = this;
    sc_caller = c;
    sc_popup = 0;
    sc_listall = 0;
    sc_checksol = 0;
    sc_notools = 0;
    sc_alias = 0;
    sc_alias = 0;
    sc_hostname = 0;
    sc_hostname_b = 0;
    sc_dispname = 0;
    sc_dispname_b = 0;
    sc_progname = 0;
    sc_progname_b = 0;
    sc_execdir = 0;
    sc_execdir_b = 0;
    sc_execname = 0;
    sc_execname_b = 0;
    sc_catchar = 0;
    sc_catchar_b = 0;
    sc_catmode = 0;
    sc_catmode_b = 0;

    sc_popup = gtk_NewPopup(0, "WRspice Interface Control Panel",
        sc_cancel_proc, 0);
    if (!sc_popup)
        return;
    gtk_window_set_resizable(GTK_WINDOW(sc_popup), false);

    GtkWidget *form = gtk_table_new(2, 1, false);
    gtk_widget_show(form);
    gtk_container_set_border_width(GTK_CONTAINER(form), 2);
    gtk_container_add(GTK_CONTAINER(sc_popup), form);
    int rowcnt = 0;

    //
    // label in frame plus help btn
    //
    GtkWidget *row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);
    GtkWidget *label = gtk_label_new("WRspice Interface Options");
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
        GTK_SIGNAL_FUNC(sc_action), 0);
    gtk_box_pack_end(GTK_BOX(row), button, false, false, 0);
    gtk_table_attach(GTK_TABLE(form), row, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // WRspice interface controls
    //
    button = gtk_check_button_new_with_label(
        "List all devices and subcircuits.");
    gtk_widget_set_name(button, "listall");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(sc_action), 0);
    gtk_table_attach(GTK_TABLE(form), button, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;
    sc_listall = button;

    button = gtk_check_button_new_with_label(
        "Check and report solitary connections.");
    gtk_widget_set_name(button, "checksol");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(sc_action), 0);
    gtk_table_attach(GTK_TABLE(form), button, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;
    sc_checksol = button;

    button = gtk_check_button_new_with_label(
        "Don't show WRspice Tool Control panel.");
    gtk_widget_set_name(button, "notools");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(sc_action), 0);
    gtk_table_attach(GTK_TABLE(form), button, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;
    sc_notools = button;

    button = gtk_check_button_new_with_label(
        "Spice device prefix aliases");
    gtk_widget_set_name(button, "alias_b");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(sc_action), 0);
    gtk_table_attach(GTK_TABLE(form), button, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    GtkWidget *entry = gtk_entry_new();
    gtk_widget_show(entry);
    gtk_table_attach(GTK_TABLE(form), entry, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;
    sc_alias = entry;
    sc_alias_b = button;

    button = gtk_check_button_new_with_label(
        "Remote WRspice server host\nname");
    gtk_widget_set_name(button, "hostname_b");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(sc_action), 0);
    gtk_table_attach(GTK_TABLE(form), button, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    entry = gtk_entry_new();
    gtk_widget_show(entry);
    gtk_table_attach(GTK_TABLE(form), entry, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;
    sc_hostname = entry;
    sc_hostname_b = button;

    button = gtk_check_button_new_with_label(
        "Remote WRspice server host\ndisplay name");
    gtk_widget_set_name(button, "dispname_b");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(sc_action), 0);
    gtk_table_attach(GTK_TABLE(form), button, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    entry = gtk_entry_new();
    gtk_widget_show(entry);
    gtk_table_attach(GTK_TABLE(form), entry, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;
    sc_dispname = entry;
    sc_dispname_b = button;

    button = gtk_check_button_new_with_label(
        "Path to local WRspice\nexecutable");
    gtk_widget_set_name(button, "progname_b");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(sc_action), 0);
    gtk_table_attach(GTK_TABLE(form), button, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    entry = gtk_entry_new();
    gtk_widget_show(entry);
    gtk_table_attach(GTK_TABLE(form), entry, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;
    sc_progname = entry;
    sc_progname_b = button;

    button = gtk_check_button_new_with_label(
        "Path to local directory\ncontaining WRspice executable");
    gtk_widget_set_name(button, "execdir_b");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(sc_action), 0);
    gtk_table_attach(GTK_TABLE(form), button, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    entry = gtk_entry_new();
    gtk_widget_show(entry);
    gtk_table_attach(GTK_TABLE(form), entry, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;
    sc_execdir = entry;
    sc_execdir_b = button;

    button = gtk_check_button_new_with_label(
        "Assumed WRspice program\nexecutable name");
    gtk_widget_set_name(button, "execname_b");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(sc_action), 0);
    gtk_table_attach(GTK_TABLE(form), button, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    entry = gtk_entry_new();
    gtk_widget_show(entry);
    gtk_table_attach(GTK_TABLE(form), entry, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;
    sc_execname = entry;
    sc_execname_b = button;

    button = gtk_check_button_new_with_label(
        "Assumed WRspice subcircuit\nconcatenation character");
    gtk_widget_set_name(button, "catchar_b");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(sc_action), 0);
    gtk_table_attach(GTK_TABLE(form), button, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    entry = gtk_entry_new();
    gtk_widget_show(entry);
    gtk_table_attach(GTK_TABLE(form), entry, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;
    sc_catchar = entry;
    sc_catchar_b = button;

    button = gtk_check_button_new_with_label(
        "Assumed WRspice subcircuit\nexpansion mode");
    gtk_widget_set_name(button, "catmode_b");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(sc_action), 0);
    gtk_table_attach(GTK_TABLE(form), button, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    entry = gtk_option_menu_new();
    gtk_widget_set_name(entry, "expmode");
    gtk_widget_show(entry);
    GtkWidget *menu = gtk_menu_new();
    gtk_widget_set_name(menu, "expmenu");
    GtkWidget *mi = gtk_menu_item_new_with_label("WRspice");
    gtk_widget_set_name(mi, "WRspice");
    gtk_widget_show(mi);
    gtk_menu_append(GTK_MENU(menu), mi);
    mi = gtk_menu_item_new_with_label("Spice3");
    gtk_widget_set_name(mi, "Spice3");
    gtk_widget_show(mi);
    gtk_menu_append(GTK_MENU(menu), mi);
    gtk_option_menu_set_menu(GTK_OPTION_MENU(entry), menu);
    gtk_table_attach(GTK_TABLE(form), entry, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;
    sc_catmode = entry;
    sc_catmode_b = button;


    //
    // Dismiss button
    //
    button = gtk_button_new_with_label("Dismiss");
    gtk_widget_set_name(button, "Dismiss");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(sc_cancel_proc), 0);

    gtk_table_attach(GTK_TABLE(form), button, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);
    gtk_window_set_focus(GTK_WINDOW(sc_popup), button);

    update();
}


sSC::~sSC()
{
    SC = 0;
    if (sc_caller)
        GRX->Deselect(sc_caller);
    if (sc_popup)
        gtk_widget_destroy(sc_popup);
}


void
sSC::update()
{
    GRX->SetStatus(sc_listall, CDvdb()->getVariable(VA_SpiceListAll));
    GRX->SetStatus(sc_checksol, CDvdb()->getVariable(VA_CheckSolitary));
    GRX->SetStatus(sc_notools, CDvdb()->getVariable(VA_NoSpiceTools));

    const char *s = CDvdb()->getVariable(VA_SpiceAlias);
    const char *t = gtk_entry_get_text(GTK_ENTRY(sc_alias));
    if (s && t && strcmp(s, t))
        gtk_entry_set_text(GTK_ENTRY(sc_alias), s);
    bool set = (s != 0);
    gtk_widget_set_sensitive(sc_alias, !set);
    GRX->SetStatus(sc_alias_b, set);

    s = CDvdb()->getVariable(VA_SpiceHost);
    t = gtk_entry_get_text(GTK_ENTRY(sc_hostname));
    if (s && t && strcmp(s, t))
        gtk_entry_set_text(GTK_ENTRY(sc_hostname), s);
    set = (s != 0);
    gtk_widget_set_sensitive(sc_hostname, !set);
    GRX->SetStatus(sc_hostname_b, set);

    s = CDvdb()->getVariable(VA_SpiceHostDisplay);
    t = gtk_entry_get_text(GTK_ENTRY(sc_dispname));
    if (s && t && strcmp(s, t))
        gtk_entry_set_text(GTK_ENTRY(sc_dispname), s);
    set = (s != 0);
    gtk_widget_set_sensitive(sc_dispname, !set);
    GRX->SetStatus(sc_dispname_b, set);

    s = CDvdb()->getVariable(VA_SpiceProg);
    t = gtk_entry_get_text(GTK_ENTRY(sc_progname));
    if (s && t && strcmp(s, t))
        gtk_entry_set_text(GTK_ENTRY(sc_progname), s);
    set = (s != 0);
    gtk_widget_set_sensitive(sc_progname, !set);
    GRX->SetStatus(sc_progname_b, set);

    s = CDvdb()->getVariable(VA_SpiceExecDir);
    t = gtk_entry_get_text(GTK_ENTRY(sc_execdir));
    if (s) {
        if (t && strcmp(s, t))
            gtk_entry_set_text(GTK_ENTRY(sc_execdir), s);
    }
    else
        gtk_entry_set_text(GTK_ENTRY(sc_execdir), XM()->ExecDirectory());
    set = (s != 0);
    gtk_widget_set_sensitive(sc_execdir, !set);
    GRX->SetStatus(sc_execdir_b, set);

    s = CDvdb()->getVariable(VA_SpiceExecName);
    t = gtk_entry_get_text(GTK_ENTRY(sc_execname));
    if (s) {
        if (t && strcmp(s, t))
            gtk_entry_set_text(GTK_ENTRY(sc_execname), s);
    }
    else
        gtk_entry_set_text(GTK_ENTRY(sc_execname), XM()->SpiceExecName());
    set = (s != 0);
    gtk_widget_set_sensitive(sc_execname, !set);
    GRX->SetStatus(sc_execname_b, set);

    s = CDvdb()->getVariable(VA_SpiceSubcCatchar);
    t = gtk_entry_get_text(GTK_ENTRY(sc_catchar));
    if (s) {
        if (t && strcmp(s, t))
            gtk_entry_set_text(GTK_ENTRY(sc_catchar), s);
    }
    else {
        char tmp[2];
        tmp[0] = CD()->GetSubcCatchar();
        tmp[1] = 0;
        gtk_entry_set_text(GTK_ENTRY(sc_catchar), tmp);
    }
    set = (s != 0);
    gtk_widget_set_sensitive(sc_catchar, !set);
    GRX->SetStatus(sc_catchar_b, set);

    s = CDvdb()->getVariable(VA_SpiceSubcCatmode);
    int h = gtk_option_menu_get_history(GTK_OPTION_MENU(sc_catmode));
    set = (s != 0);
    if (s) {
        if (*s == 'w' || *s == 'W') {
            if (h != 0)
                gtk_option_menu_set_history(GTK_OPTION_MENU(sc_catmode), 0);
        }
        else if (*s == 's' || *s == 'S') {
            if (h != 1)
                gtk_option_menu_set_history(GTK_OPTION_MENU(sc_catmode), 1);
        }
        else {
            CDvdb()->clearVariable(VA_SpiceSubcCatmode);
            set = false;
        }
    }
    else {
        if (CD()->GetSubcCatmode() == cCD::SUBC_CATMODE_WR && h != 0)
            gtk_option_menu_set_history(GTK_OPTION_MENU(sc_catmode), 0);
        else if (CD()->GetSubcCatmode() == cCD::SUBC_CATMODE_SPICE3 && h != 1)
            gtk_option_menu_set_history(GTK_OPTION_MENU(sc_catmode), 1);
    }
    gtk_widget_set_sensitive(sc_catmode, !set);
    GRX->SetStatus(sc_catmode_b, set);
}


// Static function.
void
sSC::sc_cancel_proc(GtkWidget*, void*)
{
    SCD()->PopUpSpiceIf(0, MODE_OFF);
}


// Static function.
void
sSC::sc_action(GtkWidget *caller, void*)
{
    if (!SC)
        return;
    const char *name = gtk_widget_get_name(caller);
    if (!strcmp(name, "Help")) {
        DSPmainWbag(PopUpHelp("xic:spif"))
        return;
    }

    bool state = GRX->GetStatus(caller);
    if (!strcmp(name, "listall")) {
        if (state)
            CDvdb()->setVariable(VA_SpiceListAll, "");
        else
            CDvdb()->clearVariable(VA_SpiceListAll);
    }
    else if (!strcmp(name, "checksol")) {
        if (state)
            CDvdb()->setVariable(VA_CheckSolitary, "");
        else
            CDvdb()->clearVariable(VA_CheckSolitary);
    }
    else if (!strcmp(name, "notools")) {
        if (state)
            CDvdb()->setVariable(VA_NoSpiceTools, "");
        else
            CDvdb()->clearVariable(VA_NoSpiceTools);
    }
    else if (!strcmp(name, "alias_b")) {
        if (state) {
            const char *text = gtk_entry_get_text(GTK_ENTRY(SC->sc_alias));
            CDvdb()->setVariable(VA_SpiceAlias, text);
        }
        else
            CDvdb()->clearVariable(VA_SpiceAlias);
    }
    else if (!strcmp(name, "hostname_b")) {
        if (state) {
            const char *text = gtk_entry_get_text(GTK_ENTRY(SC->sc_hostname));
            CDvdb()->setVariable(VA_SpiceHost, text);
        }
        else
            CDvdb()->clearVariable(VA_SpiceHost);
    }
    else if (!strcmp(name, "dispname_b")) {
        if (state) {
            const char *text = gtk_entry_get_text(GTK_ENTRY(SC->sc_dispname));
            CDvdb()->setVariable(VA_SpiceHostDisplay, text);
        }
        else
            CDvdb()->clearVariable(VA_SpiceHostDisplay);
    }
    else if (!strcmp(name, "progname_b")) {
        if (state) {
            const char *text = gtk_entry_get_text(GTK_ENTRY(SC->sc_progname));
            CDvdb()->setVariable(VA_SpiceProg, text);
        }
        else
            CDvdb()->clearVariable(VA_SpiceProg);
    }
    else if (!strcmp(name, "execdir_b")) {
        if (state) {
            const char *text = gtk_entry_get_text(GTK_ENTRY(SC->sc_execdir));
            CDvdb()->setVariable(VA_SpiceExecDir, text);
        }
        else
            CDvdb()->clearVariable(VA_SpiceExecDir);
    }
    else if (!strcmp(name, "execname_b")) {
        if (state) {
            const char *text = gtk_entry_get_text(GTK_ENTRY(SC->sc_execname));
            CDvdb()->setVariable(VA_SpiceExecName, text);
        }
        else
            CDvdb()->clearVariable(VA_SpiceExecName);
    }
    else if (!strcmp(name, "catchar_b")) {
        if (state) {
            const char *text = gtk_entry_get_text(GTK_ENTRY(SC->sc_catchar));
            CDvdb()->setVariable(VA_SpiceSubcCatchar, text);
        }
        else
            CDvdb()->clearVariable(VA_SpiceSubcCatchar);
    }
    else if (!strcmp(name, "catmode_b")) {
        if (state) {
            int h =
                gtk_option_menu_get_history(GTK_OPTION_MENU(SC->sc_catmode));
            CDvdb()->setVariable(VA_SpiceSubcCatmode,
                h ? "Spice3" : "WRspice");
        }
        else
            CDvdb()->clearVariable(VA_SpiceSubcCatmode);
    }
}


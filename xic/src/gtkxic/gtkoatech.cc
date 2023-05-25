
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
#include "oa_if.h"
#include "dsp_inlines.h"
#include "errorlog.h"
#include "gtkmain.h"


//
// Panel for setting tech attachment for OA library.
//
// Help keyword:
// xic:oatech

namespace {
    namespace gtkoatech {
        struct sOAtc
        {
            sOAtc(GRobject);
            ~sOAtc();

            void update();

            GtkWidget *shell() { return (ot_popup); }

        private:
            static void ot_cancel_proc(GtkWidget*, void*);
            static void ot_action(GtkWidget*, void*);

            GRobject ot_caller;
            GtkWidget *ot_popup;
            GtkWidget *ot_label;
            GtkWidget *ot_unat;
            GtkWidget *ot_at;
            GtkWidget *ot_tech;
            GtkWidget *ot_def;
            GtkWidget *ot_dest;
            GtkWidget *ot_crt;
            GtkWidget *ot_status;

            char *ot_attachment;
        };

        sOAtc *OAtc;
    }
}

using namespace gtkoatech;


void
cOAif::PopUpOAtech(GRobject caller, ShowMode mode, int x, int y)
{
    if (!GTKdev::exists() || !GTKmainwin::exists())
        return;
    if (mode == MODE_OFF) {
        delete OAtc;
        return;
    }
    if (mode == MODE_UPD) {
        if (OAtc)
            OAtc->update();
        return;
    }
    if (OAtc) {
        OAtc->update();
        return;
    }

    new sOAtc(caller);
    if (!OAtc->shell()) {
        delete OAtc;
        return;
    }
    gtk_window_set_transient_for(GTK_WINDOW(OAtc->shell()),
        GTK_WINDOW(GTKmainwin::self()->Shell()));

    int mwid;
    gtk_MonitorGeom(GTKmainwin::self()->Shell(), 0, 0, &mwid, 0);
    GtkRequisition req;
    gtk_widget_get_requisition(OAtc->shell(), &req);
    if (x + req.width > mwid)
        x = mwid - req.width;
    gtk_window_move(GTK_WINDOW(OAtc->shell()), x, y);
    gtk_widget_show(OAtc->shell());
}


sOAtc::sOAtc(GRobject c)
{
    OAtc = this;
    ot_caller = c;
    ot_label = 0;
    ot_unat = 0;
    ot_at = 0;
    ot_tech = 0;
    ot_def = 0;
    ot_dest = 0;
    ot_crt = 0;
    ot_status = 0;
    ot_attachment = 0;

    ot_popup = gtk_NewPopup(0, "OpenAccess Tech", ot_cancel_proc, 0);
    if (!ot_popup)
        return;

    GtkWidget *form = gtk_table_new(1, 1, false);
    gtk_widget_show(form);
    gtk_container_set_border_width(GTK_CONTAINER(form), 2);
    gtk_container_add(GTK_CONTAINER(ot_popup), form);
    int rowcnt = 0;

    //
    // label in frame plus help btn
    //
    GtkWidget *row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);

    GtkWidget *label = gtk_label_new("Set technology for library");
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
        G_CALLBACK(ot_action), 0);
    gtk_box_pack_end(GTK_BOX(row), button, false, false, 0);
    ot_label = label;

    gtk_table_attach(GTK_TABLE(form), row, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);

    button = gtk_button_new_with_label("Unattach");
    gtk_widget_set_name(button, "Unattach");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(ot_action), 0);
    gtk_box_pack_start(GTK_BOX(row), button, false, false, 0);
    ot_unat = button;

    button = gtk_button_new_with_label("Attach");
    gtk_widget_set_name(button, "Attach");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(ot_action), 0);
    gtk_box_pack_start(GTK_BOX(row), button, false, false, 0);
    ot_at = button;

    GtkWidget *entry = gtk_entry_new();
    gtk_widget_show(entry);
    gtk_box_pack_start(GTK_BOX(row), entry, true, true, 0);
    ot_tech = entry;

    button = gtk_button_new_with_label("Default");
    gtk_widget_set_name(button, "Default");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(ot_action), 0);
    gtk_box_pack_start(GTK_BOX(row), button, false, false, 0);
    ot_def = button;

    gtk_table_attach(GTK_TABLE(form), row, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);
    rowcnt++;

    row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);

    button = gtk_button_new_with_label("Destroy Tech");
    gtk_widget_set_name(button, "Destroy");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(ot_action), 0);
    gtk_box_pack_start(GTK_BOX(row), button, true, true, 0);
    ot_dest = button;

    button = gtk_button_new_with_label("Create New Tech");
    gtk_widget_set_name(button, "Create");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(ot_action), 0);
    gtk_box_pack_start(GTK_BOX(row), button, true, true, 0);
    ot_crt = button;

    gtk_table_attach(GTK_TABLE(form), row, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);
    rowcnt++;

    label = gtk_label_new("Tech status:");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    frame = gtk_frame_new(0);
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), label);
    ot_status = label;

    gtk_table_attach(GTK_TABLE(form), frame, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);
    rowcnt++;

    //
    // Dismiss button
    //
    button = gtk_button_new_with_label("Dismiss");
    gtk_widget_set_name(button, "Dismiss");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(ot_cancel_proc), 0);

    gtk_table_attach(GTK_TABLE(form), button, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);
    gtk_window_set_focus(GTK_WINDOW(ot_popup), button);

    update();
}


sOAtc::~sOAtc()
{
    OAtc = 0;
    if (ot_caller)
        GTKdev::Deselect(ot_caller);
    if (ot_popup)
        gtk_widget_destroy(ot_popup);
    delete [] ot_attachment;
}


void
sOAtc::update()
{
    char buf[256];
    const char *libname;
    OAif()->GetSelection(&libname, 0);
    if (!libname)
        return;
    bool branded;
    if (!OAif()->is_lib_branded(libname, &branded)) {
        Log()->ErrorLog(mh::Processing, Errs()->get_error());
        return;
    }
    if (branded)
        snprintf(buf, sizeof(buf), "Set technology for library %s", libname);
    else
        snprintf(buf, sizeof(buf), "Technology for library %s", libname);
    gtk_label_set_text(GTK_LABEL(ot_label), buf);
    char *attachlib;
    if (!OAif()->has_attached_tech(libname, &attachlib)) {
        Log()->ErrorLog(mh::Processing, Errs()->get_error());
        return;
    }
    if (attachlib) {
        delete [] ot_attachment;
        ot_attachment = lstring::copy(attachlib);
        snprintf(buf, sizeof(buf), "Tech status: attached library %s",
            attachlib);
        gtk_label_set_text(GTK_LABEL(ot_status), buf);
        delete [] attachlib;
        gtk_widget_set_sensitive(ot_unat, branded);
        gtk_widget_set_sensitive(ot_at, false);
        gtk_widget_set_sensitive(ot_tech, false);
        gtk_widget_set_sensitive(ot_def, false);
        gtk_widget_set_sensitive(ot_dest, false);
        gtk_widget_set_sensitive(ot_crt, false);
    }
    else {
        bool hastech;
        if (!OAif()->has_local_tech(libname, &hastech)) {
            Log()->ErrorLog(mh::Processing, Errs()->get_error());
            return;
        }
        if (hastech) {
            gtk_label_set_text(GTK_LABEL(ot_status),
                "Tech status: local database");
            gtk_widget_set_sensitive(ot_unat, false);
            gtk_widget_set_sensitive(ot_at, false);
            gtk_widget_set_sensitive(ot_tech, false);
            gtk_widget_set_sensitive(ot_def, false);
            gtk_widget_set_sensitive(ot_dest, branded);
            gtk_widget_set_sensitive(ot_crt, false);
        }
        else {
            gtk_label_set_text(GTK_LABEL(ot_status),
                "Tech status: no database (not good!)");
            gtk_widget_set_sensitive(ot_unat, false);
            gtk_widget_set_sensitive(ot_at, branded);
            gtk_widget_set_sensitive(ot_tech, branded);
            gtk_widget_set_sensitive(ot_def, branded);
            gtk_widget_set_sensitive(ot_dest, false);
            gtk_widget_set_sensitive(ot_crt, branded);
        }
    }
}


// Static function.
void
sOAtc::ot_cancel_proc(GtkWidget*, void*)
{
    OAif()->PopUpOAtech(0, MODE_OFF, 0, 0);
}


// Static function.
void
sOAtc::ot_action(GtkWidget *caller, void*)
{
    if (!OAtc)
        return;
    const char *name = gtk_widget_get_name(caller);
    if (!strcmp(name, "Help")) {
        DSPmainWbag(PopUpHelp("xic:oatech"))
        return;
    }

    const char *libname;
    OAif()->GetSelection(&libname, 0);
    if (!libname)
        return;

    if (!strcmp(name, "Unattach")) {
        if (!OAif()->destroy_tech(libname, true))
            Log()->ErrorLog(mh::Processing, Errs()->get_error());
        OAtc->update();
    }
    else if (!strcmp(name, "Attach")) {
        const char *alib = gtk_entry_get_text(GTK_ENTRY(OAtc->ot_tech));
        char *tok = lstring::gettok(&alib);
        if (!tok) {
            Log()->PopUpErr("No tech library name given!");
            return;
        }
        bool ret = OAif()->attach_tech(libname, tok);
        delete [] tok;
        if (!ret)
            Log()->ErrorLog(mh::Processing, Errs()->get_error());
        OAtc->update();
    }
    else if (!strcmp(name, "Default")) {
        const char *def = CDvdb()->getVariable(VA_OaDefTechLibrary);
        if (!def)
            def = OAtc->ot_attachment;
        if (!def)
            def = "";
        gtk_entry_set_text(GTK_ENTRY(OAtc->ot_tech), def);
    }
    else if (!strcmp(name, "Destroy")) {
        if (!OAif()->destroy_tech(libname, false))
            Log()->ErrorLog(mh::Processing, Errs()->get_error());
        OAtc->update();
    }
    else if (!strcmp(name, "Create")) {
        if (!OAif()->create_local_tech(libname))
            Log()->ErrorLog(mh::Processing, Errs()->get_error());
        OAtc->update();
    }
}


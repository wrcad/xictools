
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
#include "gtkinlines.h"


//
// Panel for setting misc OA interface parameters.
//
// Help keyword:
// xic:oadefs

namespace {
    namespace gtkoadefs {
        struct sOAdf
        {
            sOAdf(GRobject);
            ~sOAdf();

            void update();

            GtkWidget *shell() { return (od_popup); }

        private:
            static void od_cancel_proc(GtkWidget*, void*);
            static void od_action(GtkWidget*, void*);
            static void od_change(GtkWidget*, void*);

            GRobject od_caller;
            GtkWidget *od_popup;
            GtkWidget *od_path;
            GtkWidget *od_lib;
            GtkWidget *od_techlib;
            GtkWidget *od_layout;
            GtkWidget *od_schem;
            GtkWidget *od_symb;
            GtkWidget *od_prop;
            GtkWidget *od_cdf;
        };

        sOAdf *OAdf;
    }
}

using namespace gtkoadefs;


void
cOAif::PopUpOAdefs(GRobject caller, ShowMode mode, int x, int y)
{
    if (!GRX || !mainBag())
        return;
    if (mode == MODE_OFF) {
        delete OAdf;
        return;
    }
    if (mode == MODE_UPD) {
        if (OAdf)
            OAdf->update();
        return;
    }
    if (OAdf) {
        OAdf->update();
        return;
    }

    new sOAdf(caller);
    if (!OAdf->shell()) {
        delete OAdf;
        return;
    }
    gtk_window_set_transient_for(GTK_WINDOW(OAdf->shell()),
        GTK_WINDOW(mainBag()->Shell()));

    int mwid;
    gtk_MonitorGeom(mainBag()->Shell(), 0, 0, &mwid, 0);
    GtkRequisition req;
    gtk_widget_get_requisition(OAdf->shell(), &req);
    if (x + req.width > mwid)
        x = mwid - req.width;
    gtk_window_move(GTK_WINDOW(OAdf->shell()), x, y);
    gtk_widget_show(OAdf->shell());
}


sOAdf::sOAdf(GRobject c)
{
    OAdf = this;
    od_caller = c;
    od_path = 0;
    od_lib = 0;
    od_techlib = 0;
    od_layout = 0;
    od_schem = 0;
    od_symb = 0;
    od_prop = 0;
    od_cdf = 0;

    od_popup = gtk_NewPopup(0, "OpenAccess Defaults", od_cancel_proc, 0);
    if (!od_popup)
        return;

    GtkWidget *form = gtk_table_new(2, 1, false);
    gtk_widget_show(form);
    gtk_container_set_border_width(GTK_CONTAINER(form), 2);
    gtk_container_add(GTK_CONTAINER(od_popup), form);
    int rowcnt = 0;

    //
    // label in frame plus help btn
    //
    GtkWidget *row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);

    GtkWidget *label = gtk_label_new("Set interface defaults and options");
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
        G_CALLBACK(od_action), 0);
    gtk_box_pack_end(GTK_BOX(row), button, false, false, 0);

    gtk_table_attach(GTK_TABLE(form), row, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    GtkWidget *entry = gtk_entry_new();
    gtk_widget_show(entry);
    gtk_widget_set_name(entry, "path");
    frame = gtk_frame_new("Library Path");
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), entry);
    g_signal_connect(G_OBJECT(entry), "changed",
        G_CALLBACK(od_change), 0);
    od_path = entry;

    gtk_table_attach(GTK_TABLE(form), frame, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    label = gtk_label_new("Default Library");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);

    gtk_table_attach(GTK_TABLE(form), label, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    entry = gtk_entry_new();
    gtk_widget_show(entry);
    gtk_widget_set_name(entry, "lib");
    g_signal_connect(G_OBJECT(entry), "changed",
        G_CALLBACK(od_change), 0);
    od_lib = entry;

    gtk_table_attach(GTK_TABLE(form), entry, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    label = gtk_label_new("Default Tech Library");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);

    gtk_table_attach(GTK_TABLE(form), label, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    entry = gtk_entry_new();
    gtk_widget_show(entry);
    gtk_widget_set_name(entry, "tech");
    g_signal_connect(G_OBJECT(entry), "changed",
        G_CALLBACK(od_change), 0);
    od_techlib = entry;

    gtk_table_attach(GTK_TABLE(form), entry, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    label = gtk_label_new("Default Layout View");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);

    gtk_table_attach(GTK_TABLE(form), label, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    entry = gtk_entry_new();
    gtk_widget_show(entry);
    gtk_widget_set_name(entry, "layout");
    g_signal_connect(G_OBJECT(entry), "changed",
        G_CALLBACK(od_change), 0);
    od_layout = entry;

    gtk_table_attach(GTK_TABLE(form), entry, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    label = gtk_label_new("Default Schematic View");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);

    gtk_table_attach(GTK_TABLE(form), label, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    entry = gtk_entry_new();
    gtk_widget_show(entry);
    gtk_widget_set_name(entry, "schem");
    g_signal_connect(G_OBJECT(entry), "changed",
        G_CALLBACK(od_change), 0);
    od_schem = entry;

    gtk_table_attach(GTK_TABLE(form), entry, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    label = gtk_label_new("Default Symbol View");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);

    gtk_table_attach(GTK_TABLE(form), label, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    entry = gtk_entry_new();
    gtk_widget_show(entry);
    gtk_widget_set_name(entry, "symb");
    g_signal_connect(G_OBJECT(entry), "changed",
        G_CALLBACK(od_change), 0);
    od_symb = entry;

    gtk_table_attach(GTK_TABLE(form), entry, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    label = gtk_label_new("Default Properties View");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);

    gtk_table_attach(GTK_TABLE(form), label, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    entry = gtk_entry_new();
    gtk_widget_show(entry);
    gtk_widget_set_name(entry, "prop");
    g_signal_connect(G_OBJECT(entry), "changed",
        G_CALLBACK(od_change), 0);
    od_prop = entry;

    gtk_table_attach(GTK_TABLE(form), entry, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    button = gtk_check_button_new_with_label("Dump CDF files when reading");
    gtk_widget_set_name(button, "cdf");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(od_action), 0);
    od_cdf = button;

    gtk_table_attach(GTK_TABLE(form), button, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // Dismiss button
    //
    button = gtk_button_new_with_label("Dismiss");
    gtk_widget_set_name(button, "Dismiss");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(od_cancel_proc), 0);

    gtk_table_attach(GTK_TABLE(form), button, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);
    gtk_window_set_focus(GTK_WINDOW(od_popup), button);

    update();
}


sOAdf::~sOAdf()
{
    OAdf = 0;
    if (od_caller)
        GRX->Deselect(od_caller);
    if (od_popup)
        gtk_widget_destroy(od_popup);
}


void
sOAdf::update()
{
    const char *s = CDvdb()->getVariable(VA_OaLibraryPath);
    if (!s)
        s = "";
    const char *t = gtk_entry_get_text(GTK_ENTRY(od_path));
    if (!t)
        s = "";
    if (strcmp(s, t))
        gtk_entry_set_text(GTK_ENTRY(od_path), s);

    s = CDvdb()->getVariable(VA_OaDefLibrary);
    if (!s)
        s = "";
    t = gtk_entry_get_text(GTK_ENTRY(od_lib));
    if (!t)
        s = "";
    if (strcmp(s, t))
        gtk_entry_set_text(GTK_ENTRY(od_lib), s);

    s = CDvdb()->getVariable(VA_OaDefTechLibrary);
    if (!s)
        s = "";
    t = gtk_entry_get_text(GTK_ENTRY(od_techlib));
    if (!t)
        s = "";
    if (strcmp(s, t))
        gtk_entry_set_text(GTK_ENTRY(od_techlib), s);

    s = CDvdb()->getVariable(VA_OaDefLayoutView);
    if (!s)
        s = "";
    t = gtk_entry_get_text(GTK_ENTRY(od_layout));
    if (!t)
        s = "";
    if (strcmp(s, t))
        gtk_entry_set_text(GTK_ENTRY(od_layout), s);

    s = CDvdb()->getVariable(VA_OaDefSchematicView);
    if (!s)
        s = "";
    t = gtk_entry_get_text(GTK_ENTRY(od_schem));
    if (!t)
        s = "";
    if (strcmp(s, t))
        gtk_entry_set_text(GTK_ENTRY(od_schem), s);

    s = CDvdb()->getVariable(VA_OaDefSymbolView);
    if (!s)
        s = "";
    t = gtk_entry_get_text(GTK_ENTRY(od_symb));
    if (!t)
        s = "";
    if (strcmp(s, t))
        gtk_entry_set_text(GTK_ENTRY(od_symb), s);

    s = CDvdb()->getVariable(VA_OaDefDevPropView);
    if (!s)
        s = "";
    t = gtk_entry_get_text(GTK_ENTRY(od_prop));
    if (!t)
        s = "";
    if (strcmp(s, t))
        gtk_entry_set_text(GTK_ENTRY(od_prop), s);

    GRX->SetStatus(od_cdf, CDvdb()->getVariable(VA_OaDumpCdfFiles));
}


// Static function.
void
sOAdf::od_cancel_proc(GtkWidget*, void*)
{
    OAif()->PopUpOAdefs(0, MODE_OFF, 0, 0);
}


// Static function.
void
sOAdf::od_action(GtkWidget *caller, void*)
{
    if (!OAdf)
        return;
    const char *name = gtk_widget_get_name(caller);
    if (!name)
        return;
    if (!strcmp(name, "Help")) {
        DSPmainWbag(PopUpHelp("xic:oadefs"))
        return;
    }
    if (!strcmp(name, "cdf")) {
        if (GRX->GetStatus(caller))
            CDvdb()->setVariable(VA_OaDumpCdfFiles, "");
        else
            CDvdb()->clearVariable(VA_OaDumpCdfFiles);
    }
}


// Static function.
void
sOAdf::od_change(GtkWidget *caller, void*)
{
    if (!OAdf)
        return;
    const char *name = gtk_widget_get_name(caller);
    if (!name)
        return;
    if (!strcmp(name, "path")) {
        const char *t = gtk_entry_get_text(GTK_ENTRY(OAdf->od_path));
        if (!t || !*t)
            CDvdb()->clearVariable(VA_OaLibraryPath);
        else
            CDvdb()->setVariable(VA_OaLibraryPath, t);
    }
    else if (!strcmp(name, "lib")) {
        const char *t = gtk_entry_get_text(GTK_ENTRY(OAdf->od_lib));
        if (!t || !*t)
            CDvdb()->clearVariable(VA_OaDefLibrary);
        else
            CDvdb()->setVariable(VA_OaDefLibrary, t);
    }
    else if (!strcmp(name, "tech")) {
        const char *t = gtk_entry_get_text(GTK_ENTRY(OAdf->od_techlib));
        if (!t || !*t)
            CDvdb()->clearVariable(VA_OaDefTechLibrary);
        else
            CDvdb()->setVariable(VA_OaDefTechLibrary, t);
    }
    else if (!strcmp(name, "layout")) {
        const char *t = gtk_entry_get_text(GTK_ENTRY(OAdf->od_layout));
        if (!t || !*t)
            CDvdb()->clearVariable(VA_OaDefLayoutView);
        else
            CDvdb()->setVariable(VA_OaDefLayoutView, t);
    }
    else if (!strcmp(name, "schem")) {
        const char *t = gtk_entry_get_text(GTK_ENTRY(OAdf->od_schem));
        if (!t || !*t)
            CDvdb()->clearVariable(VA_OaDefSchematicView);
        else
            CDvdb()->setVariable(VA_OaDefSchematicView, t);
    }
    else if (!strcmp(name, "symb")) {
        const char *t = gtk_entry_get_text(GTK_ENTRY(OAdf->od_symb));
        if (!t || !*t)
            CDvdb()->clearVariable(VA_OaDefSymbolView);
        else
            CDvdb()->setVariable(VA_OaDefSymbolView, t);
    }
    else if (!strcmp(name, "prop")) {
        const char *t = gtk_entry_get_text(GTK_ENTRY(OAdf->od_prop));
        if (!t || !*t)
            CDvdb()->clearVariable(VA_OaDefDevPropView);
        else
            CDvdb()->setVariable(VA_OaDefDevPropView, t);
    }
}


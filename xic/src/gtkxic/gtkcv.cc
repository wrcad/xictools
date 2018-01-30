
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
#include "cvrt.h"
#include "dsp_inlines.h"
#include "gtkmain.h"
#include "gtkcv.h"
#include "gtkinlines.h"


//-------------------------------------------------------------------------
// Pop-up to control stand-alone format conversions
//
// Help system keywords used:
//  xic:convt

namespace {
    namespace gtkcv {
        struct sCv
        {
            sCv(GRobject, int, bool(*)(int, void*), void*);
            ~sCv();

            void update(int);

            GtkWidget *shell() { return (cv_popup); }

        private:
            static void cv_cancel_proc(GtkWidget*, void*);
            static void cv_page_chg_proc(GtkWidget*, void*, int, void*);
            static void cv_action(GtkWidget*, void*);
            static void cv_input_proc(GtkWidget*, void*);
            static void cv_format_proc(int);
            static void cv_sens_test();
            static void cv_val_changed(GtkWidget*, void*);
            static WndSensMode wnd_sens_test();

            GRobject cv_caller;
            GtkWidget *cv_popup;
            GtkWidget *cv_label;
            GtkWidget *cv_nbook;
            GtkWidget *cv_strip;
            GtkWidget *cv_noflvias;
            GtkWidget *cv_noflpcs;
            GtkWidget *cv_input;
            GtkWidget *cv_tx_label;
            bool (*cv_callback)(int, void*);
            void *cv_arg;
            cvofmt_t *cv_fmt;
            cnmap_t *cv_cnmap;
            llist_t *cv_llist;
            wnd_t *cv_wnd;
            GTKspinBtn sb_scale;

            static int cv_fmt_type;
            static int cv_inp_type;
        };

        sCv *Cv;
    }
}

using namespace gtkcv;

int sCv::cv_fmt_type = cConvert::cvGds;
int sCv::cv_inp_type = cConvert::cvLayoutFile;


// The inp_type has two fields:
// Low short:  set input source
//    enum { cvDefault, cvLayoutFile, cvChdName, cvChdFile, cvNativeDir };
// High short: set page, if 0 no change, else subtract 1 for
//    enum { cvGds, cvOas, cvCif, cvCgx, cvXic, cvTxt, cvChd, cvCgd };

void
cConvert::PopUpConvert(GRobject caller, ShowMode mode, int inp_type,
    bool(*callback)(int, void*), void *arg)
{
    if (!GRX || !mainBag())
        return;
    if (mode == MODE_OFF) {
        delete Cv;
        return;
    }
    if (mode == MODE_UPD) {
        if (Cv)
            Cv->update(inp_type);
        return;
    }
    if (Cv)
        return;

    new sCv(caller, inp_type, callback, arg);
    if (!Cv->shell()) {
        delete Cv;
        return;
    }
    gtk_window_set_transient_for(GTK_WINDOW(Cv->shell()),
        GTK_WINDOW(mainBag()->Shell()));

    GRX->SetPopupLocation(GRloc(), Cv->shell(), mainBag()->Viewport());
    gtk_widget_show(Cv->shell());
}


sCv::sCv(GRobject c, int inp_type, bool(*callback)(int, void*), void *arg)
{
    Cv = this;
    cv_caller = c;
    cv_popup = 0;
    cv_label = 0;
    cv_nbook = 0;
    cv_strip = 0;
    cv_noflvias = 0;
    cv_noflpcs = 0;
    cv_input = 0;
    cv_tx_label = 0;
    cv_callback = callback;
    cv_arg = arg;
    cv_fmt = 0;
    cv_cnmap = 0;
    cv_llist = 0;
    cv_wnd = 0;

    cv_popup = gtk_NewPopup(0, "Format Conversion", cv_cancel_proc, 0);
    if (!cv_popup)
        return;
    gtk_window_set_resizable(GTK_WINDOW(cv_popup), false);

    // Without this, spin entries sometimes freeze up for some reason.
    gtk_object_set_data(GTK_OBJECT(cv_popup), "no_prop_key", (void*)1);

    GtkWidget *topform = gtk_table_new(2, 1, false);
    gtk_widget_show(topform);
    gtk_container_set_border_width(GTK_CONTAINER(topform), 2);
    gtk_container_add(GTK_CONTAINER(cv_popup), topform);
    int toprcnt = 0;

    //
    // Label in frame plus help btn
    //
    GtkWidget *row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);
    GtkWidget *label = gtk_label_new("");
    gtk_widget_show(label);
    cv_label = label;
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    GtkWidget *frame = gtk_frame_new(0);
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), label);
    gtk_box_pack_start(GTK_BOX(row), frame, true, true, 0);
    GtkWidget *button = gtk_button_new_with_label("Help");
    gtk_widget_set_name(button, "Help");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(cv_action), 0);
    gtk_box_pack_end(GTK_BOX(row), button, false, false, 0);
    gtk_table_attach(GTK_TABLE(topform), row, 0, 2, toprcnt, toprcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    toprcnt++;

    //
    // Input selection menu
    //
    row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);

    label = gtk_label_new("Input Source");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_box_pack_start(GTK_BOX(row), label, false, false, 0);

    cv_input = gtk_option_menu_new();
    gtk_widget_set_name(cv_input, "Input");
    gtk_widget_show(cv_input);
    GtkWidget *menu = gtk_menu_new();
    gtk_widget_set_name(menu, "inpmenu");

    GtkWidget *mi = gtk_menu_item_new_with_label("Layout File");
    gtk_widget_set_name(mi, "Lfile");
    gtk_widget_show(mi);
    gtk_menu_append(GTK_MENU(menu), mi);
    gtk_signal_connect(GTK_OBJECT(mi), "activate",
        GTK_SIGNAL_FUNC(cv_input_proc), (void*)(long)cConvert::cvLayoutFile);
    mi = gtk_menu_item_new_with_label("Cell Hierarchy Digest Name");
    gtk_widget_set_name(mi, "CHname");
    gtk_widget_show(mi);
    gtk_menu_append(GTK_MENU(menu), mi);
    gtk_signal_connect(GTK_OBJECT(mi), "activate", 
        GTK_SIGNAL_FUNC(cv_input_proc), (void*)(long)cConvert::cvChdName);
    mi = gtk_menu_item_new_with_label("Cell Hierarchy Digest File");
    gtk_widget_set_name(mi, "CHfile");
    gtk_widget_show(mi);
    gtk_menu_append(GTK_MENU(menu), mi);
    gtk_signal_connect(GTK_OBJECT(mi), "activate",
        GTK_SIGNAL_FUNC(cv_input_proc), (void*)(long)cConvert::cvChdFile);
    mi = gtk_menu_item_new_with_label("Native Cell Directory");
    gtk_widget_set_name(mi, "Cdir");
    gtk_widget_show(mi);
    gtk_menu_append(GTK_MENU(menu), mi);
    gtk_signal_connect(GTK_OBJECT(mi), "activate",
        GTK_SIGNAL_FUNC(cv_input_proc), (void*)(long)cConvert::cvNativeDir);

    gtk_option_menu_set_menu(GTK_OPTION_MENU(cv_input), menu);
    gtk_box_pack_start(GTK_BOX(row), cv_input, false, false, 0);

    gtk_table_attach(GTK_TABLE(topform), row, 0, 2, toprcnt, toprcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    toprcnt++;

    //
    // Output format selection notebook
    //
    cv_fmt = new cvofmt_t(cv_format_proc, cv_fmt_type, cvofmt_file);
    gtk_table_attach(GTK_TABLE(topform), cv_fmt->frame(), 0, 2, toprcnt,
        toprcnt+1, (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    toprcnt++;

    cv_nbook = gtk_notebook_new();
    gtk_widget_show(cv_nbook);
    gtk_signal_connect(GTK_OBJECT(cv_nbook), "switch-page",
        GTK_SIGNAL_FUNC(cv_page_chg_proc), 0);
    gtk_table_attach(GTK_TABLE(topform), cv_nbook, 0, 2, toprcnt, toprcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    toprcnt++;

    //
    // The Setup page
    //
    GtkWidget *form = gtk_table_new(2, 1, false);
    gtk_widget_show(form);
    gtk_container_set_border_width(GTK_CONTAINER(form), 2);
    int rowcnt = 0;

    //
    // Strip for Export button
    //
    button = gtk_check_button_new_with_label(
        "Strip For Export - (convert physical data only)");
    gtk_widget_set_name(button, "strip");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(cv_action), 0);
    GRX->SetStatus(button, CDvdb()->getVariable(VA_StripForExport));
    gtk_table_attach(GTK_TABLE(form), button, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;
    cv_strip = button;

    button = gtk_check_button_new_with_label(
        "Don't flatten standard vias, keep as instance at top level");
    gtk_widget_set_name(button, "noflvias");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(cv_action), 0);
    gtk_table_attach(GTK_TABLE(form), button, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;
    cv_noflvias = button;

    button = gtk_check_button_new_with_label(
        "Don't flatten pcells, keep as instance at top level");
    gtk_widget_set_name(button, "noflpcs");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(cv_action), 0);
    gtk_table_attach(GTK_TABLE(form), button, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;
    cv_noflpcs = button;


    GtkWidget *tab_label = gtk_label_new("Setup");
    gtk_widget_show(tab_label);
    gtk_notebook_append_page(GTK_NOTEBOOK(cv_nbook), form, tab_label);

    //
    // The Convert File page
    //
    form = gtk_table_new(2, 1, false);
    gtk_widget_show(form);
    gtk_container_set_border_width(GTK_CONTAINER(form), 2);
    rowcnt = 0;

    //
    // Layer list
    //
    cv_llist = new llist_t;
    gtk_table_attach(GTK_TABLE(form), cv_llist->frame(), 0, 2, rowcnt,
        rowcnt+1, (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // Cell name mapping
    //
    cv_cnmap = new cnmap_t(false);
    gtk_table_attach(GTK_TABLE(form), cv_cnmap->frame(), 0, 2, rowcnt,
        rowcnt+1, (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // Window
    //
    cv_wnd = new wnd_t(wnd_sens_test, WndFuncCvt);
    gtk_table_attach(GTK_TABLE(form), cv_wnd->frame(), 0, 2, rowcnt,
        rowcnt+1, (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // Conversion scale
    //
    cv_tx_label = gtk_label_new("Conversion Scale Factor");
    gtk_widget_show(cv_tx_label);
    gtk_misc_set_padding(GTK_MISC(cv_tx_label), 2, 2);
    gtk_table_attach(GTK_TABLE(form), cv_tx_label, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    GtkWidget *sb = sb_scale.init(FIO()->TransScale(), CDSCALEMIN, CDSCALEMAX,
        5);
    sb_scale.connect_changed(GTK_SIGNAL_FUNC(cv_val_changed), 0);
    gtk_widget_set_usize(sb, 100, -1);

    gtk_table_attach(GTK_TABLE(form), sb, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    GtkWidget *sep = gtk_hseparator_new();
    gtk_widget_show(sep);
    gtk_table_attach(GTK_TABLE(form), sep, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // Go button
    //
    button = gtk_button_new_with_label("Convert");
    gtk_widget_set_name(button, "Convert");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(cv_action), 0);
    gtk_table_attach(GTK_TABLE(form), button, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)0,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);

    tab_label = gtk_label_new("Convert File");
    gtk_widget_show(tab_label);
    gtk_notebook_append_page(GTK_NOTEBOOK(cv_nbook), form, tab_label);

    //
    // Dismiss button
    //
    button = gtk_button_new_with_label("Dismiss");
    gtk_widget_set_name(button, "Dismiss");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(cv_cancel_proc), 0);

    gtk_table_attach(GTK_TABLE(topform), button, 1, 2, toprcnt, toprcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);
    gtk_window_set_focus(GTK_WINDOW(cv_popup), button);

    if (inp_type == cConvert::cvDefault)
        inp_type = cv_inp_type;
    update(inp_type);
}


sCv::~sCv()
{
    Cv = 0;
    delete cv_fmt;
    delete cv_llist;
    delete cv_cnmap;
    delete cv_wnd;
    if (cv_caller)
        GRX->Deselect(cv_caller);
    if (cv_callback)
        (*cv_callback)(-1, cv_arg);
    if (cv_popup)
        gtk_widget_destroy(cv_popup);
}


void
sCv::update(int inp_type)
{
    int op_type = inp_type >> 16;
    inp_type &= 0xffff;

    GRX->SetStatus(cv_strip, CDvdb()->getVariable(VA_StripForExport));
    GRX->SetStatus(cv_noflvias, CDvdb()->getVariable(VA_NoFlattenStdVias));
    GRX->SetStatus(cv_noflpcs, CDvdb()->getVariable(VA_NoFlattenPCells));
    sb_scale.set_value(FIO()->TransScale());

    cv_fmt->update();
    cv_wnd->update();
    cv_llist->update();
    cv_cnmap->update();
    cv_sens_test();

    if (inp_type >= cConvert::cvLayoutFile &&
            inp_type <= cConvert::cvNativeDir)
        gtk_option_menu_set_history(GTK_OPTION_MENU(cv_input), inp_type - 1);
    if (inp_type == cConvert::cvChdName)
        cv_fmt->configure(cvofmt_chd);
    else if (inp_type == cConvert::cvChdFile)
        cv_fmt->configure(cvofmt_chdfile);
    else if (inp_type == cConvert::cvNativeDir)
        cv_fmt->configure(cvofmt_native);

    if (op_type > 0) {
        op_type -= 1;
        cv_fmt->set_page(op_type);
    }
    cv_format_proc(cv_fmt_type);
}


// Static function.
void
sCv::cv_cancel_proc(GtkWidget*, void*)
{
    Cvt()->PopUpConvert(0, MODE_OFF, 0, 0, 0);
}


// Static function.
void
sCv::cv_page_chg_proc(GtkWidget*, void*, int pg, void*)
{
    if (!Cv)
        return;
    const char *lb;
    if (pg == 0)
        lb = "Set parameters for converting cell data";
    else
        lb = "Convert cell data file";
    gtk_label_set_text(GTK_LABEL(Cv->cv_label), lb);
}


// Static function.
void
sCv::cv_action(GtkWidget *caller, void*)
{
    if (!Cv)
        return;
    const char *name = gtk_widget_get_name(caller);
    if (!name)
        return;
    if (!strcmp(name, "strip")) {
        if (GRX->GetStatus(caller))
            CDvdb()->setVariable(VA_StripForExport, 0);
        else
            CDvdb()->clearVariable(VA_StripForExport);
    }
    if (!strcmp(name, "noflvias")) {
        if (GRX->GetStatus(caller))
            CDvdb()->setVariable(VA_NoFlattenStdVias, 0);
        else
            CDvdb()->clearVariable(VA_NoFlattenStdVias);
        return;
    }
    if (!strcmp(name, "noflpcs")) {
        if (GRX->GetStatus(caller))
            CDvdb()->setVariable(VA_NoFlattenPCells, 0);
        else
            CDvdb()->clearVariable(VA_NoFlattenPCells);
        return;
    }
    else if (!strcmp(name, "Help")) {
        DSPmainWbag(PopUpHelp("xic:convt"))
    }
    else if (!strcmp(name, "Convert")) {
        int code = cv_fmt_type;
        if (Cv->cv_inp_type == cConvert::cvChdName)
            // Input is a CHD name from the database.
            code |= (cConvert::CVchdName << 16);
        else if (Cv->cv_inp_type == cConvert::cvChdFile)
            // Input is a saved CHD file from disk.
            code |= (cConvert::CVchdFile << 16);
        else if (Cv->cv_inp_type == cConvert::cvNativeDir)
            // Input is a directory containing native files.
            code |= (cConvert::CVnativeDir << 16);
        else if (code == cConvert::cvGds && Cv->cv_fmt->gds_text_input())
            // Input is a gds-text file.
            code |= (cConvert::CVgdsText << 16);
        else
            // Input is normal archive file.
            code |= (cConvert::CVlayoutFile << 16);
        if (!Cv->cv_callback || !(*Cv->cv_callback)(code, Cv->cv_arg))
            Cvt()->PopUpConvert(0, MODE_OFF, 0, 0, 0);
    }
}


// Static function.
void
sCv::cv_input_proc(GtkWidget*, void *arg)
{
    if (!Cv)
        return;
    if ((long)arg == cConvert::cvLayoutFile) {
        gtk_widget_set_sensitive(Cv->cv_cnmap->frame(), true);
        Cv->cv_inp_type = cConvert::cvLayoutFile;
        Cv->cv_fmt->configure(cvofmt_file);
    }
    else if ((long)arg == cConvert::cvChdName) {
        gtk_widget_set_sensitive(Cv->cv_cnmap->frame(), false);
        Cv->cv_inp_type = cConvert::cvChdName;
        Cv->cv_fmt->configure(cvofmt_chd);
    }
    else if ((long)arg == cConvert::cvChdFile) {
        gtk_widget_set_sensitive(Cv->cv_cnmap->frame(), false);
        Cv->cv_inp_type = cConvert::cvChdFile;
        Cv->cv_fmt->configure(cvofmt_chdfile);
    }
    else if ((long)arg == cConvert::cvNativeDir) {
        gtk_widget_set_sensitive(Cv->cv_cnmap->frame(), true);
        Cv->cv_inp_type = cConvert::cvNativeDir;
        Cv->cv_fmt->configure(cvofmt_native);
    }
    cv_sens_test();
}



// Static function.
void
sCv::cv_format_proc(int type)
{
    if (!Cv)
        return;
    cv_fmt_type = type;
    if (type == cConvert::cvXic) {
        gtk_widget_set_sensitive(Cv->cv_llist->frame(), true);
        gtk_widget_set_sensitive(Cv->cv_cnmap->frame(), true);
        gtk_widget_set_sensitive(Cv->cv_wnd->frame(), true);
        gtk_widget_set_sensitive(Cv->cv_strip, false);
    }
    else if (type == cConvert::cvTxt) {
        gtk_widget_set_sensitive(Cv->cv_llist->frame(), false);
        gtk_widget_set_sensitive(Cv->cv_cnmap->frame(), false);
        gtk_widget_set_sensitive(Cv->cv_wnd->frame(), false);
        gtk_widget_set_sensitive(Cv->cv_strip, false);
    }
    else if (type == cConvert::cvChd) {
        bool cn = (cv_inp_type == cConvert::cvLayoutFile ||
            cv_inp_type == cConvert::cvNativeDir);
        gtk_widget_set_sensitive(Cv->cv_llist->frame(), false);
        gtk_widget_set_sensitive(Cv->cv_cnmap->frame(), cn);
        gtk_widget_set_sensitive(Cv->cv_wnd->frame(), false);
        gtk_widget_set_sensitive(Cv->cv_strip, false);
    }
    else if (type == cConvert::cvCgd) {
        bool cn = (cv_inp_type == cConvert::cvLayoutFile ||
            cv_inp_type == cConvert::cvNativeDir);
        gtk_widget_set_sensitive(Cv->cv_llist->frame(), true);
        gtk_widget_set_sensitive(Cv->cv_cnmap->frame(), cn);
        gtk_widget_set_sensitive(Cv->cv_wnd->frame(), false);
        gtk_widget_set_sensitive(Cv->cv_strip, false);
    }
    else {
        bool cn = (cv_inp_type == cConvert::cvLayoutFile ||
            cv_inp_type == cConvert::cvNativeDir);
        gtk_widget_set_sensitive(Cv->cv_llist->frame(), true);
        gtk_widget_set_sensitive(Cv->cv_cnmap->frame(), cn);
        gtk_widget_set_sensitive(Cv->cv_wnd->frame(), true);
        gtk_widget_set_sensitive(Cv->cv_strip, true);
    }
    cv_sens_test();
}


// Static function.
void
sCv::cv_sens_test()
{
    if (!Cv)
        return;
    bool ns = cv_fmt_type == cConvert::cvGds && Cv->cv_fmt->gds_text_input();
    if (!ns && cv_fmt_type == cConvert::cvTxt)
        ns = true;
    if (!ns && (cv_fmt_type == cConvert::cvChd ||
            cv_fmt_type == cConvert::cvCgd))
        ns = true;

    Cv->sb_scale.set_sensitive(!ns);
    gtk_widget_set_sensitive(Cv->cv_tx_label, !ns);
    Cv->cv_wnd->set_sens();
}


// Static function.
void
sCv::cv_val_changed(GtkWidget*, void*)
{
    if (!Cv)
        return;
    const char *s = Cv->sb_scale.get_string();
    char *endp;
    double d = strtod(s, &endp);
    if (endp > s && d >= CDSCALEMIN && d <= CDSCALEMAX)
        FIO()->SetTransScale(d);
}


// Static function.
WndSensMode
sCv::wnd_sens_test()
{
    if (cv_inp_type == cConvert::cvNativeDir)
        return (WndSensNone);
    if (cv_fmt_type == cConvert::cvGds && Cv->cv_fmt->gds_text_input())
        return (WndSensNone);
    if (cv_fmt_type == cConvert::cvTxt)
        return (WndSensNone);
    if (cv_fmt_type == cConvert::cvXic)
        return (WndSensFlatten);
    return (WndSensAllModes);
}


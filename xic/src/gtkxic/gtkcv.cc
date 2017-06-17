
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2007 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *  WHITELEY RESEARCH INCORPORATED PROPRIETARY SOFTWARE                   *
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
 *                                                                        *
 * XIC Integrated Circuit Layout and Schematic Editor                     *
 *                                                                        *
 *========================================================================*
 $Id: gtkcv.cc,v 5.55 2016/03/02 00:39:40 stevew Exp $
 *========================================================================*/

#include "main.h"
#include "fio.h"
#include "cvrt.h"
#include "dsp_inlines.h"
#include "gtkmain.h"
#include "gtkinlines.h"
#include "gtkcv.h"


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
            static void cv_action(GtkWidget*, void*);
            static void cv_input_proc(GtkWidget*, void*);
            static void cv_format_proc(int);
            static void cv_sens_test();
            static void cv_val_changed(GtkWidget*, void*);
            static WndSensMode wnd_sens_test();

            GRobject cv_caller;
            GtkWidget *cv_popup;
            GtkWidget *cv_input;
            GtkWidget *cv_tx_label;
            GtkWidget *cv_strip;
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
    cv_input = 0;
    cv_tx_label = 0;
    cv_strip = 0;
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

    GtkWidget *form = gtk_table_new(2, 1, false);
    gtk_widget_show(form);
    gtk_container_set_border_width(GTK_CONTAINER(form), 2);
    gtk_container_add(GTK_CONTAINER(cv_popup), form);
    int rowcnt = 0;

    //
    // Label in frame plus help btn
    //
    GtkWidget *row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);
    GtkWidget *label = gtk_label_new(
        "Direct File-to-File Conversions - (do not alter database)");
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
        GTK_SIGNAL_FUNC(cv_action), 0);
    gtk_box_pack_end(GTK_BOX(row), button, false, false, 0);
    gtk_table_attach(GTK_TABLE(form), row, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

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

    gtk_table_attach(GTK_TABLE(form), row, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // Output format selection notebook
    //
    cv_fmt = new cvofmt_t(cv_format_proc, cv_fmt_type, cvofmt_file);
    gtk_table_attach(GTK_TABLE(form), cv_fmt->frame(), 0, 2, rowcnt,
        rowcnt+1, (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

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
    cv_wnd = new wnd_t(wnd_sens_test, false);
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

    //
    // Go and Dismiss buttons
    //
    button = gtk_button_new_with_label("Convert");
    gtk_widget_set_name(button, "Convert");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(cv_action), 0);
    gtk_table_attach(GTK_TABLE(form), button, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);

    button = gtk_button_new_with_label("Dismiss");
    gtk_widget_set_name(button, "Dismiss");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(cv_cancel_proc), 0);

    gtk_table_attach(GTK_TABLE(form), button, 1, 2, rowcnt, rowcnt+1,
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


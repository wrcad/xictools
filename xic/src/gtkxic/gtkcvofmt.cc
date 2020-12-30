
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
#include "cvrt.h"
#include "fio.h"
#include "gtkmain.h"
#include "gtkcv.h"


namespace {
    const char *gds_limits[] =
    {
        "7  8000/8000",
        "3  600/200",
        "3  200/200",
        0
    };

    fmt_menu gds_input[] =
    {
        { "archive", Fnone },
        { "gds-text",Fnone },
        { 0,         Fnone }
    };

    const char *which_flags[] =
    {
        "WITHOUT Strip For Export",
        "WITH Strip For Export"
    };

    fmt_menu cif_extensions[] =
    {
        { "scale extension",        CIF_SCALE_EXTENSION },
        { "cell properties",        CIF_CELL_PROPERTIES },
        { "inst name comment",      CIF_INST_NAME_COMMENT },
        { "inst name extension",    CIF_INST_NAME_EXTENSION },
        { "inst magn extension ",   CIF_INST_MAGN_EXTENSION },
        { "inst array extension",   CIF_INST_ARRAY_EXTENSION },
        { "inst bound extension",   CIF_INST_BOUND_EXTENSION },
        { "inst properties",        CIF_INST_PROPERTIES },
        { "obj properties",         CIF_OBJ_PROPERTIES },
        { "wire extension",         CIF_WIRE_EXTENSION },
        { "wire extension new",     CIF_WIRE_EXTENSION_NEW },
        { "text extension",         CIF_TEXT_EXTENSION },
        { 0,                        0 }
    };


    fmt_menu cname_formats[] =
    {
        { "9 cellname;",        EXTcnameDef },
        { "(cellname);",        EXTcnameNCA },
        { "(9 cellname);",      EXTcnameICARUS },
        { "(Name: cellname);",  EXTcnameSIF },
        { "No Cell Names",      EXTcnameNone },
        { 0,                    0 }
    };

    fmt_menu layer_formats[] =
    {
        { "By Name",            EXTlayerDef },
        { "By Index",           EXTlayerNCA },
        { 0,                    0 }
    };

    fmt_menu label_formats[] = {
        { "94 text x y xform w h;",     EXTlabelDef },
        { "94 text x y;",               EXTlabelKIC },
        { "92 text x y layer_index;",   EXTlabelNCA },
        { "94 text x y layer_name;",    EXTlabelMEXTRA },
        { "No Labels",                  EXTlabelNone },
        { 0,                            0 }
    };
}

int cvofmt_t::fmt_gds_inp = 0;
char *cvofmt_t::fmt_oas_rep_string = 0;


cvofmt_t::cvofmt_t(void(*cb)(int), int init_format, cvofmt_mode fmtmode)
{
    fmt_cb = cb;
    fmt_form = gtk_notebook_new();
    gtk_widget_show(fmt_form);

    fmt_level = 0;
    fmt_gdsmap = 0;
    fmt_gdscut = 0;
    fmt_cgxcut = 0;
    fmt_oasmap = 0;
    fmt_oascmp = 0;
    fmt_oasrep = 0;
    fmt_oastab = 0;
    fmt_oassum = 0;
    fmt_oasoff = 0;
    fmt_oasnwp = 0;
    fmt_oasadv = 0;
    fmt_gdsftlab = 0;
    fmt_gdsftopt = 0;
    fmt_cifflags = 0;
    fmt_cifcname = 0;
    fmt_ciflayer = 0;
    fmt_ciflabel = 0;
#ifdef FMT_WITH_DIGESTS
    fmt_chdlabel = 0;
    fmt_chdinfo = 0;
#endif

    //
    // GDSII page
    //
    int rcnt = 0;
    GtkWidget *table = gtk_table_new(2, 1, false);
    gtk_widget_show(table);

    GtkWidget *row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);

    GtkWidget *entry = gtk_option_menu_new();
    gtk_widget_set_name(entry, "input");
    gtk_widget_show(entry);
    GtkWidget *menu = gtk_menu_new();
    gtk_widget_set_name(menu, "input");
    for (int i = 0; gds_input[i].name; i++) {
        GtkWidget *mi = gtk_menu_item_new_with_label(gds_input[i].name);
        gtk_widget_set_name(mi, gds_input[i].name);
        gtk_widget_show(mi);
        gtk_menu_append(GTK_MENU(menu), mi);
        gtk_signal_connect(GTK_OBJECT(mi), "activate",
            GTK_SIGNAL_FUNC(fmt_input_proc), this);
    }
    gtk_option_menu_set_menu(GTK_OPTION_MENU(entry), menu);
    gtk_option_menu_set_history(GTK_OPTION_MENU(entry), cvofmt_t::fmt_gds_inp);
    gtk_box_pack_start(GTK_BOX(row), entry, false, false, 0);
    fmt_gdsftopt = entry;

    GtkWidget *label = gtk_label_new("Input File Type");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    fmt_gdsftlab = label;
    gtk_box_pack_start(GTK_BOX(row), label, false, false, 0);

    label = gtk_label_new("Output Format:  GDSII archive");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_box_pack_start(GTK_BOX(row), label, true, true, 0);

    gtk_table_attach(GTK_TABLE(table), row, 0, 2, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rcnt++;

    entry = gtk_option_menu_new();
    gtk_widget_set_name(entry, "levmenu");
    gtk_widget_show(entry);
    menu = gtk_menu_new();
    gtk_widget_set_name(menu, "levmenu");
    for (int i = 0; gds_limits[i]; i++) {
        GtkWidget *mi = gtk_menu_item_new_with_label(gds_limits[i]);
        gtk_widget_set_name(mi, gds_limits[i]);
        gtk_widget_show(mi);
        gtk_menu_append(GTK_MENU(menu), mi);
        gtk_signal_connect(GTK_OBJECT(mi), "activate",
            GTK_SIGNAL_FUNC(fmt_level_proc), (void*)gds_limits[i]);
    }
    gtk_option_menu_set_menu(GTK_OPTION_MENU(entry), menu);
    gtk_option_menu_set_history(GTK_OPTION_MENU(entry),
        FIO()->GdsOutLevel());
    fmt_level = entry;
    gtk_table_attach(GTK_TABLE(table), entry, 0, 1, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    label = gtk_label_new("GDSII version number, polyon/wire vertex limit");
    gtk_widget_show(label);

    gtk_table_attach(GTK_TABLE(table), label, 1, 2, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rcnt++;

    GtkWidget *button = gtk_check_button_new_with_label(
        "Skip layers without Xic to GDSII layer mapping");
    gtk_widget_set_name(button, "gdsmap");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(fmt_action), 0);
    GRX->SetStatus(button, CDvdb()->getVariable(VA_NoGdsMapOk));
    fmt_gdsmap = button;

    gtk_table_attach(GTK_TABLE(table), button, 0, 2, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rcnt++;

    row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);

    button = gtk_check_button_new_with_label(
        "Accept but truncate too-long strings");
    gtk_widget_set_name(button, "gdscut");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(fmt_action), 0);
    GRX->SetStatus(button, CDvdb()->getVariable(VA_GdsTruncateLongStrings));
    gtk_box_pack_start(GTK_BOX(row), button, false, false, 0);
    fmt_gdscut = button;

    label = gtk_label_new("Unit Scale");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_box_pack_end(GTK_BOX(row), label, false, false, 0);

    double initd = 1.0;
    const char *s = CDvdb()->getVariable(VA_GdsMunit);
    if (s) {
        double v;
        if (sscanf(s, "%lf", &v) == 1 && v >= 0.01 && v <= 100.0)
            initd = v;
        else
            // bogus value, clear it
            CDvdb()->clearVariable(VA_GdsMunit);
    }

    GtkWidget *sb = sb_gdsunit.init(initd, 0.01, 100.0, 5);
    sb_gdsunit.connect_changed(GTK_SIGNAL_FUNC(fmt_val_changed), this);
    gtk_widget_set_size_request(sb, 100, -1);
    gtk_box_pack_end(GTK_BOX(row), sb, false, false, 0);

    gtk_table_attach(GTK_TABLE(table), row, 0, 2, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rcnt++;

    GtkWidget *tab_label = gtk_label_new("GDSII");
    gtk_widget_show(tab_label);
    gtk_notebook_append_page(GTK_NOTEBOOK(fmt_form), table, tab_label);

    //
    // OASIS page
    //
    table = gtk_table_new(2, 1, false);
    gtk_widget_show(table);
    rcnt = 0;

    GtkWidget *hbox = gtk_hbox_new(false, 2);
    gtk_widget_show(hbox);

    label = gtk_label_new("Output Format:  OASIS archive");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_box_pack_start(GTK_BOX(hbox), label, true, true, 0);

    button = gtk_toggle_button_new_with_label("Advanced");
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(fmt_action), 0);
    gtk_widget_show(button);
    gtk_widget_set_name(button, "Advanced");
    gtk_box_pack_start(GTK_BOX(hbox), button, false, false, 0);
    fmt_oasadv = button;

    gtk_table_attach(GTK_TABLE(table), hbox, 0, 2, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rcnt++;

    button = gtk_check_button_new_with_label(
        "Skip layers without Xic to GDSII layer mapping");
    gtk_widget_set_name(button, "oasmap");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(fmt_action), 0);
    GRX->SetStatus(button, CDvdb()->getVariable(VA_NoGdsMapOk));
    fmt_oasmap = button;

    gtk_table_attach(GTK_TABLE(table), button, 0, 2, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rcnt++;

    button = gtk_check_button_new_with_label("Use compression");
    gtk_widget_set_name(button, "oascmp");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(fmt_action), 0);
    GRX->SetStatus(button, CDvdb()->getVariable(VA_OasWriteCompressed));
    fmt_oascmp = button;

    gtk_table_attach(GTK_TABLE(table), button, 0, 1, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    button = gtk_check_button_new_with_label("Find repetitions");
    gtk_widget_set_name(button, "oasrep");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(fmt_action), 0);
    GRX->SetStatus(button, CDvdb()->getVariable(VA_OasWriteRep));
    fmt_oasrep = button;

    gtk_table_attach(GTK_TABLE(table), button, 1, 2, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rcnt++;

    button = gtk_check_button_new_with_label("Use string tables");
    gtk_widget_set_name(button, "oastab");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(fmt_action), 0);
    GRX->SetStatus(button, CDvdb()->getVariable(VA_OasWriteNameTab));
    fmt_oastab = button;

    gtk_table_attach(GTK_TABLE(table), button, 0, 1, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    button = gtk_check_button_new_with_label("Write CRC checksum");
    gtk_widget_set_name(button, "oassum");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(fmt_action), 0);
    GRX->SetStatus(button, CDvdb()->getVariable(VA_OasWriteChecksum));
    fmt_oassum = button;

    gtk_table_attach(GTK_TABLE(table), button, 1, 2, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rcnt++;

    tab_label = gtk_label_new("OASIS");
    gtk_widget_show(tab_label);
    gtk_notebook_append_page(GTK_NOTEBOOK(fmt_form), table, tab_label);

    //
    // CIF page
    //
    table = gtk_table_new(3, 3, false);
    gtk_widget_show(table);
    rcnt = 0;

    label = gtk_label_new("Output Format:  CIF archive");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);

    hbox = gtk_hbox_new(false, 2);
    gtk_widget_show(hbox);
    gtk_box_pack_start(GTK_BOX(hbox), label, true, true, 0);

    GtkWidget *menubar = gtk_menu_bar_new();
    gtk_widget_show(menubar);
    GtkWidget *pc_item = gtk_menu_item_new_with_label("Extension Flags");
    gtk_widget_show(pc_item);
    gtk_menu_bar_append(GTK_MENU_BAR(menubar), pc_item);

    fmt_strip = FIO()->IsStripForExport();
    menu = gtk_menu_new();
    fmt_cifflags = menu;
    unsigned int flgs = fmt_strip ?
        FIO()->CifStyle().flags_export() : FIO()->CifStyle().flags();
    GtkWidget *mi =
        gtk_menu_item_new_with_label(which_flags[(int)fmt_strip]);
    gtk_widget_show(mi);
    gtk_menu_append(GTK_MENU(menu), mi);
    gtk_object_set_data(GTK_OBJECT(mi), "cvofmt", this);
    gtk_signal_connect(GTK_OBJECT(mi), "activate",
        GTK_SIGNAL_FUNC(fmt_flags_proc), (void*)"Strip");
    GtkWidget *msep = gtk_menu_item_new();  // separator
    gtk_menu_append(GTK_MENU(menu), msep);
    gtk_widget_show(msep);
    for (fmt_menu *m = cif_extensions; m->name; m++) {
        mi = gtk_check_menu_item_new_with_label(m->name);
        gtk_menu_append(GTK_MENU(menu), mi);
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(mi),
            m->code & flgs);
        gtk_object_set_data(GTK_OBJECT(mi), "cvofmt", this);
        gtk_signal_connect(GTK_OBJECT(mi), "activate",
            GTK_SIGNAL_FUNC(fmt_flags_proc), (void*)m->name);
        gtk_widget_show(mi);
    }
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(pc_item), menu);
    gtk_box_pack_start(GTK_BOX(hbox), menubar, false, false, 0);

    GtkWidget *btn = gtk_button_new_with_label("Last Seen");
    gtk_widget_show(btn);
    gtk_widget_set_name(btn, "last_seen");
    gtk_box_pack_start(GTK_BOX(hbox), btn, false, false, 0);
    gtk_signal_connect(GTK_OBJECT(btn), "clicked",
        GTK_SIGNAL_FUNC(fmt_action), 0);

    gtk_table_attach(GTK_TABLE(table), hbox, 0, 3, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rcnt++;

    label = gtk_label_new("Cell Name Extension");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_table_attach(GTK_TABLE(table), label, 0, 1, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    entry = gtk_option_menu_new();
    gtk_widget_set_name(entry, "cnamemenu");
    gtk_widget_show(entry);
    menu = gtk_menu_new();
    gtk_widget_set_name(menu, "cnamemenu");
    for (fmt_menu *m = cname_formats; m->name; m++) {
        mi = gtk_menu_item_new_with_label(m->name);
        gtk_widget_set_name(mi, m->name);
        gtk_widget_show(mi);
        gtk_menu_append(GTK_MENU(menu), mi);
        gtk_signal_connect(GTK_OBJECT(mi), "activate",
            GTK_SIGNAL_FUNC(fmt_style_proc), (void*)m->name);
    }
    gtk_option_menu_set_menu(GTK_OPTION_MENU(entry), menu);
    for (int i = 0; cname_formats[i].name; i++) {
        if (cname_formats[i].code == FIO()->CifStyle().cname_type()) {
            gtk_option_menu_set_history(GTK_OPTION_MENU(entry), i);
            break;
        }
    }
    fmt_cifcname = entry;

    gtk_table_attach(GTK_TABLE(table), entry, 0, 1, rcnt+1, rcnt+2,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    label = gtk_label_new("Layer Specification");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_table_attach(GTK_TABLE(table), label, 1, 2, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    entry = gtk_option_menu_new();
    gtk_widget_set_name(entry, "layermenu");
    gtk_widget_show(entry);
    menu = gtk_menu_new();
    gtk_widget_set_name(menu, "layermenu");
    for (fmt_menu *m = layer_formats; m->name; m++) {
        mi = gtk_menu_item_new_with_label(m->name);
        gtk_widget_set_name(mi, m->name);
        gtk_widget_show(mi);
        gtk_menu_append(GTK_MENU(menu), mi);
        gtk_signal_connect(GTK_OBJECT(mi), "activate",
            GTK_SIGNAL_FUNC(fmt_style_proc), (void*)m->name);
    }
    gtk_option_menu_set_menu(GTK_OPTION_MENU(entry), menu);
    for (int i = 0; layer_formats[i].name; i++) {
        if (layer_formats[i].code == FIO()->CifStyle().layer_type()) {
            gtk_option_menu_set_history(GTK_OPTION_MENU(entry), i);
            break;
        }
    }
    fmt_ciflayer = entry;

    gtk_table_attach(GTK_TABLE(table), entry, 1, 2, rcnt+1, rcnt+2,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    label = gtk_label_new("Label Extension");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_table_attach(GTK_TABLE(table), label, 2, 3, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    entry = gtk_option_menu_new();
    gtk_widget_set_name(entry, "labelmenu");
    gtk_widget_show(entry);
    menu = gtk_menu_new();
    gtk_widget_set_name(menu, "labelmenu");
    for (fmt_menu *m = label_formats; m->name; m++) {
        mi = gtk_menu_item_new_with_label(m->name);
        gtk_widget_set_name(mi, m->name);
        gtk_widget_show(mi);
        gtk_menu_append(GTK_MENU(menu), mi);
        gtk_signal_connect(GTK_OBJECT(mi), "activate",
            GTK_SIGNAL_FUNC(fmt_style_proc), (void*)m->name);
    }
    gtk_option_menu_set_menu(GTK_OPTION_MENU(entry), menu);
    for (int i = 0; label_formats[i].name; i++) {
        if (label_formats[i].code == FIO()->CifStyle().label_type()) {
            gtk_option_menu_set_history(GTK_OPTION_MENU(entry), i);
            break;
        }
    }
    fmt_ciflabel = entry;

    gtk_table_attach(GTK_TABLE(table), entry, 2, 3, rcnt+1, rcnt+2,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rcnt += 2;

    tab_label = gtk_label_new("CIF");
    gtk_widget_show(tab_label);
    gtk_notebook_append_page(GTK_NOTEBOOK(fmt_form), table, tab_label);

    //
    // CGX page
    //
    table = gtk_table_new(1, 1, false);
    gtk_widget_show(table);

    label = gtk_label_new("Output Format: CGX archive");
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_widget_show(label);

    gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    label = gtk_label_new("");
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);

    button = gtk_check_button_new_with_label(
        "Accept but truncate too-long strings");
    gtk_widget_set_name(button, "cgxcut");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(fmt_action), 0);
    GRX->SetStatus(button, CDvdb()->getVariable(VA_GdsTruncateLongStrings));
    fmt_cgxcut = button;

    gtk_table_attach(GTK_TABLE(table), button, 0, 1, 2, 3,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    tab_label = gtk_label_new("CGX");
    gtk_widget_show(tab_label);
    gtk_notebook_append_page(GTK_NOTEBOOK(fmt_form), table, tab_label);

    //
    // XIC page
    //
    tab_label = gtk_label_new("XIC cell files");
    gtk_widget_show(tab_label);
    table = gtk_label_new("Output Format: XIC native cell files");
    gtk_misc_set_padding(GTK_MISC(table), 2, 2);
    gtk_widget_show(table);
    gtk_notebook_append_page(GTK_NOTEBOOK(fmt_form), table, tab_label);

    //
    // TEXT page
    //
    table = gtk_table_new(2, 1, false);
    gtk_widget_show(table);
    rcnt = 0;

    label = gtk_label_new("Output Format:  ASCII text file");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_table_attach(GTK_TABLE(table), label, 0, 2, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rcnt++;

    button = gtk_check_button_new_with_label("OASIS text: print offsets");
    gtk_widget_set_name(button, "oasoff");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(fmt_action), 0);
    GRX->SetStatus(button, CDvdb()->getVariable(VA_OasPrintOffset));
    fmt_oasoff = button;

    gtk_table_attach(GTK_TABLE(table), button, 0, 1, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    button = gtk_check_button_new_with_label("OASIS text: no line wrap");
    gtk_widget_set_name(button, "oasnwp");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(fmt_action), 0);
    GRX->SetStatus(button, CDvdb()->getVariable(VA_OasPrintNoWrap));
    fmt_oasnwp = button;

    gtk_table_attach(GTK_TABLE(table), button, 1, 2, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    tab_label = gtk_label_new("ASCII text");
    gtk_widget_show(tab_label);
    gtk_notebook_append_page(GTK_NOTEBOOK(fmt_form), table, tab_label);

#ifdef FMT_WITH_DIGESTS
#define FMT_MAX_ID 7
    //
    // New CHD page
    //
    tab_label = gtk_label_new("New CHD");
    gtk_widget_show(tab_label);

    table = gtk_table_new(1, 2, false);
    gtk_widget_show(table);
    rcnt = 0;

    label = gtk_label_new("Output Format:  in-memory Cell Hierarchy Digest");
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_widget_show(label);

    gtk_table_attach(GTK_TABLE(table), label, 0, 1, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);
    rcnt++;

    // Info options
    fmt_chdlabel = gtk_label_new("Geometry Counts");
    gtk_widget_show(fmt_chdlabel);

    hbox = gtk_hbox_new(false, 2);
    gtk_widget_show(hbox);
    gtk_box_pack_start(GTK_BOX(hbox), fmt_chdlabel, false, false, 0);

    fmt_chdinfo = gtk_option_menu_new();
    gtk_widget_show(fmt_chdinfo);

    menu = gtk_menu_new();
    gtk_widget_show(menu);
    mi = gtk_menu_item_new_with_label("no geometry info saved");
    gtk_widget_show(mi);
    gtk_menu_append(GTK_MENU(menu), mi);
    gtk_signal_connect(GTK_OBJECT(mi), "activate",
        GTK_SIGNAL_FUNC(fmt_info_proc), (void*)cvINFOnone);
    mi = gtk_menu_item_new_with_label("totals only");
    gtk_widget_show(mi);
    gtk_menu_append(GTK_MENU(menu), mi);
    gtk_signal_connect(GTK_OBJECT(mi), "activate",
        GTK_SIGNAL_FUNC(fmt_info_proc), (void*)cvINFOtotals);
    mi = gtk_menu_item_new_with_label("per-layer counts");
    gtk_widget_show(mi);
    gtk_menu_append(GTK_MENU(menu), mi);
    gtk_signal_connect(GTK_OBJECT(mi), "activate",
        GTK_SIGNAL_FUNC(fmt_info_proc), (void*)cvINFOpl);
    mi = gtk_menu_item_new_with_label("per-cell counts");
    gtk_widget_show(mi);
    gtk_menu_append(GTK_MENU(menu), mi);
    gtk_signal_connect(GTK_OBJECT(mi), "activate",
        GTK_SIGNAL_FUNC(fmt_info_proc), (void*)cvINFOpc);
    mi = gtk_menu_item_new_with_label("per-cell and per-layer counts");
    gtk_widget_show(mi);
    gtk_menu_append(GTK_MENU(menu), mi);
    gtk_signal_connect(GTK_OBJECT(mi), "activate",
        GTK_SIGNAL_FUNC(fmt_info_proc), (void*)cvINFOplpc);
    gtk_option_menu_set_menu(GTK_OPTION_MENU(fmt_chdinfo), menu);
    gtk_option_menu_set_history(GTK_OPTION_MENU(fmt_chdinfo),
        FIO()->CvtInfo());
    gtk_box_pack_start(GTK_BOX(hbox), fmt_chdinfo, false, false, 0);

    gtk_table_attach(GTK_TABLE(table), hbox, 0, 1, rcnt, rcnt+1,
        (GtkAttachOptions)0, (GtkAttachOptions)0, 2, 2);
    gtk_notebook_append_page(GTK_NOTEBOOK(fmt_form), table, tab_label);

    //
    // New CGD page
    //
    tab_label = gtk_label_new("New CGD");
    gtk_widget_show(tab_label);
    table = gtk_label_new("Output Format:  in-memory Cell Geometry Digest");
    gtk_misc_set_padding(GTK_MISC(table), 2, 2);
    gtk_widget_show(table);
    gtk_notebook_append_page(GTK_NOTEBOOK(fmt_form), table, tab_label);
#else
#define FMT_MAX_ID 5
#endif

    // End of pages

    if (init_format >= 0 && init_format <= FMT_MAX_ID)
        gtk_notebook_set_page(GTK_NOTEBOOK(fmt_form), init_format);

    configure(fmtmode);

    gtk_signal_connect(GTK_OBJECT(fmt_form), "switch-page",
        GTK_SIGNAL_FUNC(fmt_page_proc), this);
}


cvofmt_t::~cvofmt_t()
{
    if (fmt_oasadv && GRX->GetStatus(fmt_oasadv))
        GRX->CallCallback(fmt_oasadv);

    // This prevents the handler from being called after this is
    // deleted, which can happen when the widgets are destroyed.

    gtk_signal_disconnect_by_func(GTK_OBJECT(fmt_form),
        GTK_SIGNAL_FUNC(fmt_page_proc), this);
}


void
cvofmt_t::update()
{
    GRX->SetStatus(fmt_gdsmap,
        CDvdb()->getVariable(VA_NoGdsMapOk));
    GRX->SetStatus(fmt_gdscut,
        CDvdb()->getVariable(VA_GdsTruncateLongStrings));
    GRX->SetStatus(fmt_cgxcut,
        CDvdb()->getVariable(VA_GdsTruncateLongStrings));
    GRX->SetStatus(fmt_oasmap,
        CDvdb()->getVariable(VA_NoGdsMapOk));
    GRX->SetStatus(fmt_oascmp,
        CDvdb()->getVariable(VA_OasWriteCompressed));
    GRX->SetStatus(fmt_oastab,
        CDvdb()->getVariable(VA_OasWriteNameTab));
    GRX->SetStatus(fmt_oasrep,
        CDvdb()->getVariable(VA_OasWriteRep));
    GRX->SetStatus(fmt_oassum,
        CDvdb()->getVariable(VA_OasWriteChecksum));
    gtk_option_menu_set_history(GTK_OPTION_MENU(fmt_level),
        FIO()->GdsOutLevel());
#ifdef FMT_WITH_DIGESTS
    gtk_option_menu_set_history(GTK_OPTION_MENU(fmt_chdinfo),
        FIO()->CvtInfo());
#endif

    const char *s = CDvdb()->getVariable(VA_GdsMunit);
    if (sb_gdsunit.is_valid(s))
        sb_gdsunit.set_value(atof(s));
    else {
        if (s)
            // bogus value, clear it
            CDvdb()->clearVariable(VA_GdsMunit);
        sb_gdsunit.set_value(1.0);
    }

    unsigned int flgs = fmt_strip ?
        FIO()->CifStyle().flags_export() : FIO()->CifStyle().flags();
    GList *g0 = gtk_container_children(GTK_CONTAINER(fmt_cifflags));
    GtkWidget *lab = GTK_BIN(g0->data)->child;
    gtk_label_set_text(GTK_LABEL(lab), which_flags[(int)fmt_strip]);
    int cnt = 0;
    // list: strip, separator, flags ...
    for (GList *l = g0->next->next; l; cnt++, l = l->next) {
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(l->data),
           cif_extensions[cnt].code & flgs);
    }
    g_list_free(g0);

    for (int i = 0; cname_formats[i].name; i++) {
        if (cname_formats[i].code == FIO()->CifStyle().cname_type()) {
            gtk_option_menu_set_history(
                GTK_OPTION_MENU(fmt_cifcname), i);
            break;
        }
    }
    for (int i = 0; layer_formats[i].name; i++) {
        if (layer_formats[i].code == FIO()->CifStyle().layer_type()) {
            gtk_option_menu_set_history(
                GTK_OPTION_MENU(fmt_ciflayer), i);
            break;
        }
    }
    for (int i = 0; label_formats[i].name; i++) {
        if (label_formats[i].code == FIO()->CifStyle().label_type()) {
            gtk_option_menu_set_history(
                GTK_OPTION_MENU(fmt_ciflabel), i);
            break;
        }
    }
    GRX->SetStatus(fmt_oasoff,
        CDvdb()->getVariable(VA_OasPrintOffset));
    GRX->SetStatus(fmt_oasnwp,
        CDvdb()->getVariable(VA_OasPrintNoWrap));
}


bool
cvofmt_t::gds_text_input()
{
    return (fmt_gds_inp);
}


void
cvofmt_t::configure(cvofmt_mode mode)
{
    if (mode == cvofmt_file) {
        // Converting layout file.
        gtk_widget_hide(fmt_gdsmap);
        gtk_widget_show(fmt_gdsftlab);
        gtk_widget_show(fmt_gdsftopt);
        gtk_widget_hide(fmt_oasmap);
        for (int i = 0; ; i++) {
            GtkWidget *w =
                gtk_notebook_get_nth_page(GTK_NOTEBOOK(fmt_form), i);
            if (!w)
                break;
            gtk_widget_show(w);
        }
#ifdef FMT_WITH_DIGESTS
        gtk_widget_set_sensitive(fmt_chdlabel, true);
        gtk_widget_set_sensitive(fmt_chdinfo, true);
#endif
    }
    else if (mode == cvofmt_native) {
        // Converting native cell files.
        gtk_widget_hide(fmt_gdsmap);
        gtk_widget_hide(fmt_gdsftlab);
        gtk_widget_hide(fmt_gdsftopt);
        gtk_widget_hide(fmt_oasmap);
        // hide 4 (XIC) and larger
        for (int i = 0; ; i++) {
            GtkWidget *w =
                gtk_notebook_get_nth_page(GTK_NOTEBOOK(fmt_form), i);
            if (!w)
                break;
            if (i >= 4)
                gtk_widget_hide(w);
            else
                gtk_widget_show(w);
        }
    }
    else if (mode == cvofmt_chd) {
        // Converting using CHD.
        gtk_widget_hide(fmt_gdsmap);
        gtk_widget_hide(fmt_gdsftlab);
        gtk_widget_hide(fmt_gdsftopt);
        gtk_widget_hide(fmt_oasmap);
        // hide 4 (XIC) and larger but keep 7 (CGD)
        for (int i = 0; ; i++) {
            GtkWidget *w =
                gtk_notebook_get_nth_page(GTK_NOTEBOOK(fmt_form), i);
            if (!w)
                break;
#ifdef FMT_WITH_DIGESTS
            if (i >= 4 && i != 7)
                gtk_widget_hide(w);
#else
            if (i >= 4)
                gtk_widget_hide(w);
#endif
            else
                gtk_widget_show(w);
        }
    }
    else if (mode == cvofmt_chdfile) {
        // Converting using CHD file.
        gtk_widget_hide(fmt_gdsmap);
        gtk_widget_hide(fmt_gdsftlab);
        gtk_widget_hide(fmt_gdsftopt);
        gtk_widget_hide(fmt_oasmap);
        // hide 4 (XIC) and larger but keep 6 and 7 (CHD and CGD)
        for (int i = 0; ; i++) {
            GtkWidget *w =
                gtk_notebook_get_nth_page(GTK_NOTEBOOK(fmt_form), i);
            if (!w)
                break;
#ifdef FMT_WITH_DIGESTS
            if (i >= 4 && i != 6 && i != 7)
                gtk_widget_hide(w);
#else
            if (i >= 4)
                gtk_widget_hide(w);
#endif
            else
                gtk_widget_show(w);
        }
#ifdef FMT_WITH_DIGESTS
        gtk_widget_set_sensitive(fmt_chdlabel, false);
        gtk_widget_set_sensitive(fmt_chdinfo, false);
#endif
    }
    else if (mode == cvofmt_asm) {
        // Assembling.
        gtk_widget_show(fmt_gdsmap);
        gtk_widget_hide(fmt_gdsftlab);
        gtk_widget_hide(fmt_gdsftopt);
        gtk_widget_show(fmt_oasmap);
        // hide 5 (Text) and larger
        for (int i = 0; ; i++) {
            GtkWidget *w =
                gtk_notebook_get_nth_page(GTK_NOTEBOOK(fmt_form), i);
            if (!w)
                break;
            if (i >= 5)
                gtk_widget_hide(w);
            else
                gtk_widget_show(w);
        }
    }
    else if (mode == cvofmt_db) {
        // Setting options (export control).
        gtk_widget_show(fmt_gdsmap);
        gtk_widget_hide(fmt_gdsftlab);
        gtk_widget_hide(fmt_gdsftopt);
        gtk_widget_show(fmt_oasmap);
        // hide 4 (XIC) and larger
        for (int i = 0; ; i++) {
            GtkWidget *w =
                gtk_notebook_get_nth_page(GTK_NOTEBOOK(fmt_form), i);
            if (!w)
                break;
            if (i >= 4)
                gtk_widget_hide(w);
            else
                gtk_widget_show(w);
        }
    }
}


void
cvofmt_t::set_page(int ix)
{
    if (ix >= 0 && ix <= 7)
        gtk_notebook_set_page(GTK_NOTEBOOK(fmt_form), ix);
}


// Static function.
void
cvofmt_t::fmt_action(GtkWidget *caller, void*)
{
    const char *name = gtk_widget_get_name(caller);
    if (!strcmp(name, "Advanced")) {
        if (GRX->GetStatus(caller)) {
            int x, y;
            GRX->Location(caller, &x, &y);
            Cvt()->PopUpOasAdv(caller, MODE_ON, x, y);
        }
        else
            Cvt()->PopUpOasAdv(0, MODE_OFF, 0, 0);
        return;
    }
    if (!strcmp(name, "gdsmap")) {
        if (GRX->GetStatus(caller))
            CDvdb()->setVariable(VA_NoGdsMapOk, 0);
        else
            CDvdb()->clearVariable(VA_NoGdsMapOk);
        return;
    }
    if (!strcmp(name, "gdscut")) {
        if (GRX->GetStatus(caller))
            CDvdb()->setVariable(VA_GdsTruncateLongStrings, 0);
        else
            CDvdb()->clearVariable(VA_GdsTruncateLongStrings);
        return;
    }
    if (!strcmp(name, "cgxcut")) {
        if (GRX->GetStatus(caller))
            CDvdb()->setVariable(VA_GdsTruncateLongStrings, 0);
        else
            CDvdb()->clearVariable(VA_GdsTruncateLongStrings);
        return;
    }
    if (!strcmp(name, "oasmap")) {
        if (GRX->GetStatus(caller))
            CDvdb()->setVariable(VA_NoGdsMapOk, 0);
        else
            CDvdb()->clearVariable(VA_NoGdsMapOk);
        return;
    }
    if (!strcmp(name, "oascmp")) {
        if (GRX->GetStatus(caller))
            CDvdb()->setVariable(VA_OasWriteCompressed, 0);
        else
            CDvdb()->clearVariable(VA_OasWriteCompressed);
        return;
    }
    if (!strcmp(name, "oastab")) {
        if (GRX->GetStatus(caller))
            CDvdb()->setVariable(VA_OasWriteNameTab, 0);
        else
            CDvdb()->clearVariable(VA_OasWriteNameTab);
        return;
    }
    if (!strcmp(name, "oasrep")) {
        if (GRX->GetStatus(caller)) {
            if (!rep_string())
                set_rep_string(lstring::copy(""));
            CDvdb()->setVariable(VA_OasWriteRep, rep_string());
        }
        else
            CDvdb()->clearVariable(VA_OasWriteRep);
        return;
    }
    if (!strcmp(name, "oassum")) {
        if (GRX->GetStatus(caller))
            CDvdb()->setVariable(VA_OasWriteChecksum, 0);
        else
            CDvdb()->clearVariable(VA_OasWriteChecksum);
        return;
    }
    if (!strcmp(name, "oasoff")) {
        if (GRX->GetStatus(caller))
            CDvdb()->setVariable(VA_OasPrintOffset, 0);
        else
            CDvdb()->clearVariable(VA_OasPrintOffset);
        return;
    }
    if (!strcmp(name, "oasnwp")) {
        if (GRX->GetStatus(caller))
            CDvdb()->setVariable(VA_OasPrintNoWrap, 0);
        else
            CDvdb()->clearVariable(VA_OasPrintNoWrap);
        return;
    }

    if (!strcmp(name, "last_seen")) {
        char *string = FIO()->LastCifStyle();
        if (!strcmp(string, "x"))
            CDvdb()->clearVariable(VA_CifOutStyle);
        else
            CDvdb()->setVariable(VA_CifOutStyle, string);
        delete [] string;
        return;
    }
}


// Static function.
void
cvofmt_t::fmt_flags_proc(GtkWidget *caller, void *client_data)
{
    cvofmt_t *fmt = (cvofmt_t*)gtk_object_get_data(GTK_OBJECT(caller),
        "cvofmt");
    if (!fmt)
        return;
    char *s = (char*)client_data;
    if (!strcmp(s, "Strip")) {
        fmt->fmt_strip = !fmt->fmt_strip;
        unsigned int flgs = fmt->fmt_strip ?
            FIO()->CifStyle().flags_export() : FIO()->CifStyle().flags();
        GList *g0 =
            gtk_container_children(GTK_CONTAINER(fmt->fmt_cifflags));
        GtkWidget *lab = GTK_BIN(g0->data)->child;
        gtk_label_set_text(GTK_LABEL(lab), which_flags[(int)fmt->fmt_strip]);
        int cnt = 0;
        // list: strip, separator, flags ...
        for (GList *l = g0->next->next; l; cnt++, l = l->next) {
            gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(l->data),
                cif_extensions[cnt].code & flgs);
        }
        g_list_free(g0);
        return;
    }
    for (int i = 0; cif_extensions[i].name; i++) {
        if (strcmp(s, cif_extensions[i].name))
            continue;
        if (GRX->GetStatus(caller)) {
            if (fmt->fmt_strip)
                FIO()->CifStyle().set_flag_export(cif_extensions[i].code);
            else
                FIO()->CifStyle().set_flag(cif_extensions[i].code);
        }
        else {
            if (fmt->fmt_strip)
                FIO()->CifStyle().unset_flag_export(cif_extensions[i].code);
            else
                FIO()->CifStyle().unset_flag(cif_extensions[i].code);
        }
        if (FIO()->CifStyle().flags() != 0xfff ||
                FIO()->CifStyle().flags_export() != 0) {
            char buf[64];
            sprintf(buf, "%d %d", FIO()->CifStyle().flags(),
                FIO()->CifStyle().flags_export());
            CDvdb()->setVariable(VA_CifOutExtensions, buf);
        }
        else
            CDvdb()->clearVariable(VA_CifOutExtensions);
        return;
    }
}


// Static function.
void
cvofmt_t::fmt_level_proc(GtkWidget*, void *client_data)
{
    char *s = (char*)client_data;
    for (int i = 0; gds_limits[i]; i++) {
        if (!strcmp(s, gds_limits[i])) {
            if (i)
                CDvdb()->setVariable(VA_GdsOutLevel, i == 1 ? "1" : "2");
            else
                CDvdb()->clearVariable(VA_GdsOutLevel);
            return;
        }
    }
}


namespace {
    // Update the CifOutStyle variable.
    //
    void
    resetvar()
    {
        char *string = FIO()->CifStyle().string();
        if (!strcmp(string, "x"))
            CDvdb()->clearVariable(VA_CifOutStyle);
        else
            CDvdb()->setVariable(VA_CifOutStyle, string);
        delete [] string;
    }
}


// Static function.
void
cvofmt_t::fmt_style_proc(GtkWidget*, void *client_data)
{
    char *s = (char*)client_data;
    for (int i = 0; cname_formats[i].name; i++) {
        if (!strcmp(s, cname_formats[i].name)) {
            FIO()->CifStyle().set_cname_type(
                (EXTcnameType)cname_formats[i].code);
            resetvar();
            return;
        }
    }
    for (int i = 0; layer_formats[i].name; i++) {
        if (!strcmp(s, layer_formats[i].name)) {
            FIO()->CifStyle().set_layer_type(
                (EXTlayerType)layer_formats[i].code);
            resetvar();
            return;
        }
    }
    for (int i = 0; label_formats[i].name; i++) {
        if (!strcmp(s, label_formats[i].name)) {
            FIO()->CifStyle().set_label_type(
                (EXTlabelType)label_formats[i].code);
            resetvar();
            return;
        }
    }
}


// Static function.
void
cvofmt_t::fmt_input_proc(GtkWidget *w, void *arg)
{
    cvofmt_t *fmt = (cvofmt_t*)arg;
    for (int i = 0; gds_input[i].name; i++) {
        const char *n = gtk_widget_get_name(w);
        if (n && !strcmp(n, gds_input[i].name)) {
            fmt_gds_inp = i;
            if (fmt && fmt->fmt_cb)
                (*fmt->fmt_cb)(cConvert::cvGds);
            break;
        }
    }
}


// Static function.
//
void
cvofmt_t::fmt_val_changed(GtkWidget*, void *arg)
{
    cvofmt_t *fmt = (cvofmt_t*)arg;
    const char *s = fmt->sb_gdsunit.get_string();
    double val;
    if (sscanf(s, "%lf", &val) == 1 && val >= 0.01 && val <= 100.0) {
        if (val == 1.0)
            CDvdb()->clearVariable(VA_GdsMunit);
        else
            CDvdb()->setVariable(VA_GdsMunit, s);
    }
}


// Static function.
//
void
cvofmt_t::fmt_page_proc(GtkWidget*, GtkWidget*, int pagenum, void *arg)
{
    cvofmt_t *fmt = (cvofmt_t*)arg;
    if (fmt->fmt_cb)
        (*fmt->fmt_cb)(pagenum);
}


// Static function.
//
void
cvofmt_t::fmt_info_proc(GtkWidget*, void *arg)
{
    cvINFO cv;
    switch ((intptr_t)arg) {
    case cvINFOnone:
        cv = cvINFOnone;
        break;
    case cvINFOtotals:
        cv = cvINFOtotals;
        break;
    case cvINFOpl:
        cv = cvINFOpl;
        break;
    case cvINFOpc:
        cv = cvINFOpc;
        break;
    case cvINFOplpc:
        cv = cvINFOplpc;
        break;
    default:
        return;
    }
    if (cv != FIO()->CvtInfo()) {
        FIO()->SetCvtInfo(cv);
        Cvt()->PopUpChdOpen(0, MODE_UPD, 0, 0, 0, 0, 0, 0);
    }
}


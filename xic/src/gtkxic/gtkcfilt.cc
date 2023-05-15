
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
#include "errorlog.h"
#include "cfilter.h"
#include "gtkmain.h"


//
// Panel for setting cell name filtering fro the Cells Listing panel.
//
// Help keyword:
// xic:cfilt

namespace {
    namespace gtkcfilt {
        struct sCf
        {
            sCf(GRobject, DisplayMode, void(*)(cfilter_t*, void*), void*);
            ~sCf();

            void update(DisplayMode);

            GtkWidget *shell() { return (cf_popup); }

        private:
            void setup(const cfilter_t*);
            cfilter_t *new_filter();
            static void cf_cancel_proc(GtkWidget*, void*);
            static void cf_action(GtkWidget*, void*);
            static void cf_radio(GtkWidget*, void*);
            static void cf_vlayer_menu_proc(GtkWidget*, void*);
            static void cf_name_menu_proc(GtkWidget*, void*);
            static void cf_sto_menu_proc(GtkWidget*, void*);
            static void cf_rcl_menu_proc(GtkWidget*, void*);

            GRobject cf_caller;
            GtkWidget *cf_popup;
            GtkWidget *cf_nimm;
            GtkWidget *cf_imm;
            GtkWidget *cf_nvsm;
            GtkWidget *cf_vsm;
            GtkWidget *cf_nlib;
            GtkWidget *cf_lib;
            GtkWidget *cf_npsm;
            GtkWidget *cf_psm;
            GtkWidget *cf_ndev;
            GtkWidget *cf_dev;
            GtkWidget *cf_nspr;
            GtkWidget *cf_spr;
            GtkWidget *cf_ntop;
            GtkWidget *cf_top;
            GtkWidget *cf_nmod;
            GtkWidget *cf_mod;
            GtkWidget *cf_nalt;
            GtkWidget *cf_alt;
            GtkWidget *cf_nref;
            GtkWidget *cf_ref;
            GtkWidget *cf_npcl;
            GtkWidget *cf_pcl;
            GtkWidget *cf_pclent;
            GtkWidget *cf_nscl;
            GtkWidget *cf_scl;
            GtkWidget *cf_sclent;
            GtkWidget *cf_nlyr;
            GtkWidget *cf_lyr;
            GtkWidget *cf_lyrent;
            GtkWidget *cf_nflg;
            GtkWidget *cf_flg;
            GtkWidget *cf_flgent;
            GtkWidget *cf_nftp;
            GtkWidget *cf_ftp;
            GtkWidget *cf_ftpent;
            GtkWidget *cf_apply;

            void(*cf_cb)(cfilter_t*, void*);
            void *cf_arg;
            DisplayMode cf_mode;

#define NUMREGS 6
            static char *cf_phys_regs[];
            static char *cf_elec_regs[];
        };

        sCf *Cf;

        char *sCf::cf_phys_regs[NUMREGS];
        char *sCf::cf_elec_regs[NUMREGS];
    }
}

using namespace gtkcfilt;


void
cMain::PopUpCellFilt(GRobject caller, ShowMode mode, DisplayMode dm,
    void(*cb)(cfilter_t*, void*), void *arg)
{
    if (!GTKdev::exists() || !GTKmainwin::exists())
        return;
    if (mode == MODE_OFF) {
        delete Cf;
        return;
    }
    if (mode == MODE_UPD) {
        if (Cf)
            Cf->update(dm);
        return;
    }
    if (Cf)
        return;

    new sCf(caller, dm, cb, arg);
    if (!Cf->shell()) {
        delete Cf;
        return;
    }
    gtk_window_set_transient_for(GTK_WINDOW(Cf->shell()),
        GTK_WINDOW(GTKmainwin::self()->Shell()));

    GTKdev::self()->SetPopupLocation(GRloc(LW_LL), Cf->shell(),
        GTKmainwin::self()->Viewport());
    gtk_widget_show(Cf->shell());
}


sCf::sCf(GRobject c, DisplayMode dm, void(*cb)(cfilter_t*, void*), void *arg)
{
    Cf = this;
    cf_caller = c;
    cf_mode = dm;
    cf_cb = cb;
    cf_arg = arg;

    cf_nimm = 0;
    cf_imm = 0;
    cf_nvsm = 0;
    cf_vsm = 0;
    cf_nlib = 0;
    cf_lib = 0;
    cf_npsm = 0;
    cf_psm = 0;
    cf_ndev = 0;
    cf_dev = 0;
    cf_nspr = 0;
    cf_spr = 0;
    cf_ntop = 0;
    cf_top = 0;
    cf_nmod = 0;
    cf_mod = 0;
    cf_nalt = 0;
    cf_alt = 0;
    cf_nref = 0;
    cf_ref = 0;
    cf_npcl = 0;
    cf_pcl = 0;
    cf_pclent = 0;
    cf_nscl = 0;
    cf_scl = 0;
    cf_sclent = 0;
    cf_nlyr = 0;
    cf_lyr = 0;
    cf_lyrent = 0;
    cf_nflg = 0;
    cf_flg = 0;
    cf_flgent = 0;
    cf_nftp = 0;
    cf_ftp = 0;
    cf_ftpent = 0;
    cf_apply = 0;

    cf_popup = gtk_NewPopup(0, "Cell List Filter", cf_cancel_proc, 0);
    if (!cf_popup)
        return;

    GtkWidget *form = gtk_table_new(2, 1, false);
    gtk_widget_show(form);
    gtk_container_set_border_width(GTK_CONTAINER(form), 2);
    gtk_container_add(GTK_CONTAINER(cf_popup), form);
    int rowcnt = 0;

    //
    // store/recall menus, label in frame plus help btn
    //

    GtkWidget *row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);

    GtkWidget *menubar = gtk_menu_bar_new();
    gtk_widget_set_name(menubar, "StoreB");
    gtk_widget_show(menubar);

    GtkWidget *item = gtk_menu_item_new_with_label("Store");
    gtk_widget_set_name(item, "Store");
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), item);
    {
        char buf[16];
        GtkWidget *menu = gtk_menu_new();
        gtk_widget_set_name(menu, "StMenu");
        for (int i = 1; i < NUMREGS; i++) {
            snprintf(buf, sizeof(buf), "reg%d", i);
            GtkWidget *menu_item = gtk_menu_item_new_with_label(buf);
            gtk_widget_set_name(menu_item, buf);
            gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
            g_signal_connect(G_OBJECT(menu_item), "activate",
                G_CALLBACK(cf_sto_menu_proc), (void*)(long)i);
            gtk_widget_show(menu_item);
        }
        gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), menu);
    }
    gtk_box_pack_start(GTK_BOX(row), menubar, false, false, 0);

    menubar = gtk_menu_bar_new();
    gtk_widget_set_name(menubar, "RecallB");
    gtk_widget_show(menubar);

    item = gtk_menu_item_new_with_label("Recall");
    gtk_widget_set_name(item, "Recall");
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), item);
    {
        char buf[16];
        GtkWidget *menu = gtk_menu_new();
        gtk_widget_set_name(menu, "StMenu");
        for (int i = 1; i < NUMREGS; i++) {
            snprintf(buf, sizeof(buf), "reg%d", i);
            GtkWidget *menu_item = gtk_menu_item_new_with_label(buf);
            gtk_widget_set_name(menu_item, buf);
            gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
            g_signal_connect(G_OBJECT(menu_item), "activate",
                G_CALLBACK(cf_rcl_menu_proc), (void*)(long)i);
            gtk_widget_show(menu_item);
        }
        gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), menu);
    }
    gtk_box_pack_start(GTK_BOX(row), menubar, false, false, 0);

    GtkWidget *label = gtk_label_new("Set filtering for cells list");
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
        G_CALLBACK(cf_action), 0);
    gtk_box_pack_end(GTK_BOX(row), button, false, false, 0);
    gtk_table_attach(GTK_TABLE(form), row, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    // Immutable
    GtkWidget *hbox = gtk_hbox_new(false, 2);
    gtk_widget_show(hbox);
    cf_nimm = gtk_check_button_new_with_label("not");
    gtk_widget_show(cf_nimm);
    g_signal_connect(G_OBJECT(cf_nimm), "clicked",
        G_CALLBACK(cf_radio), 0);
    gtk_box_pack_start(GTK_BOX(hbox), cf_nimm, false, false, 0);
    cf_imm = gtk_check_button_new_with_label("Immutable");
    gtk_widget_show(cf_imm);
    g_signal_connect(G_OBJECT(cf_imm), "clicked",
        G_CALLBACK(cf_radio), 0);
    gtk_box_pack_start(GTK_BOX(hbox), cf_imm, false, false, 0);

    gtk_table_attach(GTK_TABLE(form), hbox, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    // Via sub-master
    hbox = gtk_hbox_new(false, 2);
    gtk_widget_show(hbox);
    cf_nvsm = gtk_check_button_new_with_label("not");
    gtk_widget_show(cf_nvsm);
    g_signal_connect(G_OBJECT(cf_nvsm), "clicked",
        G_CALLBACK(cf_radio), 0);
    gtk_box_pack_start(GTK_BOX(hbox), cf_nvsm, false, false, 0);
    cf_vsm = gtk_check_button_new_with_label("Via sub-master");
    gtk_widget_show(cf_vsm);
    g_signal_connect(G_OBJECT(cf_vsm), "clicked",
        G_CALLBACK(cf_radio), 0);
    gtk_box_pack_start(GTK_BOX(hbox), cf_vsm, false, false, 0);

    gtk_table_attach(GTK_TABLE(form), hbox, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    // Library
    hbox = gtk_hbox_new(false, 2);
    gtk_widget_show(hbox);
    cf_nlib = gtk_check_button_new_with_label("not");
    gtk_widget_show(cf_nlib);
    g_signal_connect(G_OBJECT(cf_nlib), "clicked",
        G_CALLBACK(cf_radio), 0);
    gtk_box_pack_start(GTK_BOX(hbox), cf_nlib, false, false, 0);
    cf_lib = gtk_check_button_new_with_label("Library");
    gtk_widget_show(cf_lib);
    g_signal_connect(G_OBJECT(cf_lib), "clicked",
        G_CALLBACK(cf_radio), 0);
    gtk_box_pack_start(GTK_BOX(hbox), cf_lib, false, false, 0);

    gtk_table_attach(GTK_TABLE(form), hbox, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    // PCell sub-master
    hbox = gtk_hbox_new(false, 2);
    gtk_widget_show(hbox);
    cf_npsm = gtk_check_button_new_with_label("not");
    gtk_widget_show(cf_npsm);
    g_signal_connect(G_OBJECT(cf_npsm), "clicked",
        G_CALLBACK(cf_radio), 0);
    gtk_box_pack_start(GTK_BOX(hbox), cf_npsm, false, false, 0);
    cf_psm = gtk_check_button_new_with_label("PCell sub-master");
    gtk_widget_show(cf_psm);
    g_signal_connect(G_OBJECT(cf_psm), "clicked",
        G_CALLBACK(cf_radio), 0);
    gtk_box_pack_start(GTK_BOX(hbox), cf_psm, false, false, 0);

    gtk_table_attach(GTK_TABLE(form), hbox, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    // Device
    hbox = gtk_hbox_new(false, 2);
    gtk_widget_show(hbox);
    cf_ndev = gtk_check_button_new_with_label("not");
    gtk_widget_show(cf_ndev);
    g_signal_connect(G_OBJECT(cf_ndev), "clicked",
        G_CALLBACK(cf_radio), 0);
    gtk_box_pack_start(GTK_BOX(hbox), cf_ndev, false, false, 0);
    cf_dev = gtk_check_button_new_with_label("Device");
    gtk_widget_show(cf_dev);
    g_signal_connect(G_OBJECT(cf_dev), "clicked",
        G_CALLBACK(cf_radio), 0);
    gtk_box_pack_start(GTK_BOX(hbox), cf_dev, false, false, 0);

    gtk_table_attach(GTK_TABLE(form), hbox, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    // PCell super-master
    hbox = gtk_hbox_new(false, 2);
    gtk_widget_show(hbox);
    cf_nspr = gtk_check_button_new_with_label("not");
    gtk_widget_show(cf_nspr);
    g_signal_connect(G_OBJECT(cf_nspr), "clicked",
        G_CALLBACK(cf_radio), 0);
    gtk_box_pack_start(GTK_BOX(hbox), cf_nspr, false, false, 0);
    cf_spr = gtk_check_button_new_with_label("PCell super");
    gtk_widget_show(cf_spr);
    g_signal_connect(G_OBJECT(cf_spr), "clicked",
        G_CALLBACK(cf_radio), 0);
    gtk_box_pack_start(GTK_BOX(hbox), cf_spr, false, false, 0);

    gtk_table_attach(GTK_TABLE(form), hbox, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    // Top level
    hbox = gtk_hbox_new(false, 2);
    gtk_widget_show(hbox);
    cf_ntop = gtk_check_button_new_with_label("not");
    gtk_widget_show(cf_ntop);
    g_signal_connect(G_OBJECT(cf_ntop), "clicked",
        G_CALLBACK(cf_radio), 0);
    gtk_box_pack_start(GTK_BOX(hbox), cf_ntop, false, false, 0);
    cf_top = gtk_check_button_new_with_label("Top level");
    gtk_widget_show(cf_top);
    g_signal_connect(G_OBJECT(cf_top), "clicked",
        G_CALLBACK(cf_radio), 0);
    gtk_box_pack_start(GTK_BOX(hbox), cf_top, false, false, 0);

    gtk_table_attach(GTK_TABLE(form), hbox, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    // Modified
    hbox = gtk_hbox_new(false, 2);
    gtk_widget_show(hbox);
    cf_nmod = gtk_check_button_new_with_label("not");
    gtk_widget_show(cf_nmod);
    g_signal_connect(G_OBJECT(cf_nmod), "clicked",
        G_CALLBACK(cf_radio), 0);
    gtk_box_pack_start(GTK_BOX(hbox), cf_nmod, false, false, 0);
    cf_mod = gtk_check_button_new_with_label("Modified");
    gtk_widget_show(cf_mod);
    g_signal_connect(G_OBJECT(cf_mod), "clicked",
        G_CALLBACK(cf_radio), 0);
    gtk_box_pack_start(GTK_BOX(hbox), cf_mod, false, false, 0);

    gtk_table_attach(GTK_TABLE(form), hbox, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    // With alt
    hbox = gtk_hbox_new(false, 2);
    gtk_widget_show(hbox);
    cf_nalt = gtk_check_button_new_with_label("not");
    gtk_widget_show(cf_nalt);
    g_signal_connect(G_OBJECT(cf_nalt), "clicked",
        G_CALLBACK(cf_radio), 0);
    gtk_box_pack_start(GTK_BOX(hbox), cf_nalt, false, false, 0);
    cf_alt = gtk_check_button_new_with_label("With alt");
    gtk_widget_show(cf_alt);
    g_signal_connect(G_OBJECT(cf_alt), "clicked",
        G_CALLBACK(cf_radio), 0);
    gtk_box_pack_start(GTK_BOX(hbox), cf_alt, false, false, 0);

    gtk_table_attach(GTK_TABLE(form), hbox, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    // Reference
    hbox = gtk_hbox_new(false, 2);
    gtk_widget_show(hbox);
    cf_nref = gtk_check_button_new_with_label("not");
    gtk_widget_show(cf_nref);
    g_signal_connect(G_OBJECT(cf_nref), "clicked",
        G_CALLBACK(cf_radio), 0);
    gtk_box_pack_start(GTK_BOX(hbox), cf_nref, false, false, 0);
    cf_ref = gtk_check_button_new_with_label("Reference");
    gtk_widget_show(cf_ref);
    g_signal_connect(G_OBJECT(cf_ref), "clicked",
        G_CALLBACK(cf_radio), 0);
    gtk_box_pack_start(GTK_BOX(hbox), cf_ref, false, false, 0);

    gtk_table_attach(GTK_TABLE(form), hbox, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    // Parent cells
    hbox = gtk_hbox_new(false, 2);
    gtk_widget_show(hbox);
    cf_npcl = gtk_check_button_new_with_label("not");
    gtk_widget_show(cf_npcl);
    g_signal_connect(G_OBJECT(cf_npcl), "clicked",
        G_CALLBACK(cf_radio), 0);
    gtk_box_pack_start(GTK_BOX(hbox), cf_npcl, false, false, 0);
    cf_pcl = gtk_check_button_new_with_label("Parent cells");
    gtk_widget_show(cf_pcl);
    g_signal_connect(G_OBJECT(cf_pcl), "clicked",
        G_CALLBACK(cf_radio), 0);
    gtk_box_pack_start(GTK_BOX(hbox), cf_pcl, false, false, 0);

    gtk_table_attach(GTK_TABLE(form), hbox, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    cf_pclent = gtk_entry_new();
    gtk_widget_show(cf_pclent);

    gtk_table_attach(GTK_TABLE(form), cf_pclent, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    // Subcell cells
    hbox = gtk_hbox_new(false, 2);
    gtk_widget_show(hbox);
    cf_nscl = gtk_check_button_new_with_label("not");
    gtk_widget_show(cf_nscl);
    g_signal_connect(G_OBJECT(cf_nscl), "clicked",
        G_CALLBACK(cf_radio), 0);
    gtk_box_pack_start(GTK_BOX(hbox), cf_nscl, false, false, 0);
    cf_scl = gtk_check_button_new_with_label("Subcell cells");
    gtk_widget_show(cf_scl);
    g_signal_connect(G_OBJECT(cf_scl), "clicked",
        G_CALLBACK(cf_radio), 0);
    gtk_box_pack_start(GTK_BOX(hbox), cf_scl, false, false, 0);

    gtk_table_attach(GTK_TABLE(form), hbox, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    cf_sclent = gtk_entry_new();
    gtk_widget_show(cf_sclent);

    gtk_table_attach(GTK_TABLE(form), cf_sclent, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    // With layers
    hbox = gtk_hbox_new(false, 2);
    gtk_widget_show(hbox);
    cf_nlyr = gtk_check_button_new_with_label("not");
    gtk_widget_show(cf_nlyr);
    g_signal_connect(G_OBJECT(cf_nlyr), "clicked",
        G_CALLBACK(cf_radio), 0);
    gtk_box_pack_start(GTK_BOX(hbox), cf_nlyr, false, false, 0);
    cf_lyr = gtk_check_button_new_with_label("With layers");
    gtk_widget_show(cf_lyr);
    g_signal_connect(G_OBJECT(cf_lyr), "clicked",
        G_CALLBACK(cf_radio), 0);
    gtk_box_pack_start(GTK_BOX(hbox), cf_lyr, false, false, 0);

    gtk_table_attach(GTK_TABLE(form), hbox, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    cf_lyrent = gtk_entry_new();
    gtk_widget_show(cf_lyrent);

    gtk_table_attach(GTK_TABLE(form), cf_lyrent, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    // With flags
    hbox = gtk_hbox_new(false, 2);
    gtk_widget_show(hbox);
    cf_nflg = gtk_check_button_new_with_label("not");
    gtk_widget_show(cf_nflg);
    g_signal_connect(G_OBJECT(cf_nflg), "clicked",
        G_CALLBACK(cf_radio), 0);
    gtk_box_pack_start(GTK_BOX(hbox), cf_nflg, false, false, 0);
    cf_flg = gtk_check_button_new_with_label("With flags");
    gtk_widget_show(cf_flg);
    g_signal_connect(G_OBJECT(cf_flg), "clicked",
        G_CALLBACK(cf_radio), 0);
    gtk_box_pack_start(GTK_BOX(hbox), cf_flg, false, false, 0);

    gtk_table_attach(GTK_TABLE(form), hbox, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    cf_flgent = gtk_entry_new();
    gtk_widget_show(cf_flgent);

    gtk_table_attach(GTK_TABLE(form), cf_flgent, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    // From filetypes
    hbox = gtk_hbox_new(false, 2);
    gtk_widget_show(hbox);
    cf_nftp = gtk_check_button_new_with_label("not");
    gtk_widget_show(cf_nftp);
    g_signal_connect(G_OBJECT(cf_nftp), "clicked",
        G_CALLBACK(cf_radio), 0);
    gtk_box_pack_start(GTK_BOX(hbox), cf_nftp, false, false, 0);
    cf_ftp = gtk_check_button_new_with_label("From filetypes");
    gtk_widget_show(cf_ftp);
    g_signal_connect(G_OBJECT(cf_ftp), "clicked",
        G_CALLBACK(cf_radio), 0);
    gtk_box_pack_start(GTK_BOX(hbox), cf_ftp, false, false, 0);

    gtk_table_attach(GTK_TABLE(form), hbox, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    cf_ftpent = gtk_entry_new();
    gtk_widget_show(cf_ftpent);

    gtk_table_attach(GTK_TABLE(form), cf_ftpent, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // Apply and Dismiss buttons
    //
    button = gtk_button_new_with_label("Apply");
    gtk_widget_set_name(button, "Apply");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(cf_action), 0);
    cf_apply = button;
    gtk_table_attach(GTK_TABLE(form), button, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);

    button = gtk_button_new_with_label("Dismiss");
    gtk_widget_set_name(button, "Dismiss");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(cf_cancel_proc), 0);
    gtk_table_attach(GTK_TABLE(form), button, 1, 3, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);
    gtk_window_set_focus(GTK_WINDOW(cf_popup), button);

    update(cf_mode);
}


sCf::~sCf()
{
    Cf = 0;
    if (cf_caller)
        GTKdev::Deselect(cf_caller);
    if (cf_popup)
        gtk_widget_destroy(cf_popup);
}


// This is called when the listing mode changes in the Cells Listing
// panel, before the actual list is generated.
//
void
sCf::update(DisplayMode mode)
{
    cf_mode = mode;
    cfilter_t *cf;
    if (mode == Physical)
        cf = cfilter_t::parse(cf_phys_regs[0], Physical, 0);
    else
        cf = cfilter_t::parse(cf_elec_regs[0], Electrical, 0);
    setup(cf);
    if (cf_cb)
        (*cf_cb)(cf, Cf->cf_arg);  // Receiver takes ownership of cf.
}


void
sCf::setup(const cfilter_t *cf)
{
    if (!cf)
        return;
    // We know that for the flags, at most one of NOTX, X is set. 
    // This is not the case for the list forms, however this simple
    // panel can handle only one of each polarity.  Check these and
    // throw out any where both bits are set.

    unsigned int f = cf->flags();
    if ((f & CF_NOTPARENT) && (f & CF_PARENT))
        f &= ~(CF_NOTPARENT | CF_PARENT);
    if ((f & CF_NOTSUBCELL) && (f & CF_SUBCELL))
        f &= ~(CF_NOTSUBCELL | CF_SUBCELL);
    if ((f & CF_NOTLAYER) && (f & CF_LAYER))
        f &= ~(CF_NOTLAYER | CF_LAYER);
    if ((f & CF_NOTFLAG) && (f & CF_FLAG))
        f &= ~(CF_NOTFLAG | CF_FLAG);
    if ((f & CF_NOTFTYPE) && (f & CF_FTYPE))
        f &= ~(CF_NOTFTYPE | CF_FTYPE);

    GTKdev::SetStatus(cf_nimm, f & CF_NOTIMMUTABLE);
    GTKdev::SetStatus(cf_imm,  f & CF_IMMUTABLE);
    GTKdev::SetStatus(cf_nvsm, f & CF_NOTVIASUBM);
    GTKdev::SetStatus(cf_vsm,  f & CF_VIASUBM);
    GTKdev::SetStatus(cf_nlib, f & CF_NOTLIBRARY);
    GTKdev::SetStatus(cf_lib,  f & CF_LIBRARY);
    GTKdev::SetStatus(cf_npsm, f & CF_NOTPCELLSUBM);
    GTKdev::SetStatus(cf_psm,  f & CF_PCELLSUBM);
    GTKdev::SetStatus(cf_ndev, f & CF_NOTDEVICE);
    GTKdev::SetStatus(cf_dev,  f & CF_DEVICE);
    GTKdev::SetStatus(cf_nspr, f & CF_NOTPCELLSUPR);
    GTKdev::SetStatus(cf_spr,  f & CF_PCELLSUPR);
    GTKdev::SetStatus(cf_ntop, f & CF_NOTTOPLEV);
    GTKdev::SetStatus(cf_top,  f & CF_TOPLEV);
    GTKdev::SetStatus(cf_nmod, f & CF_NOTMODIFIED);
    GTKdev::SetStatus(cf_mod,  f & CF_MODIFIED);
    GTKdev::SetStatus(cf_nalt, f & CF_NOTWITHALT);
    GTKdev::SetStatus(cf_alt,  f & CF_WITHALT);
    GTKdev::SetStatus(cf_nref, f & CF_NOTREFERENCE);
    GTKdev::SetStatus(cf_ref,  f & CF_REFERENCE);
    GTKdev::SetStatus(cf_npcl, f & CF_NOTPARENT);
    GTKdev::SetStatus(cf_pcl,  f & CF_PARENT);
    GTKdev::SetStatus(cf_nscl, f & CF_NOTSUBCELL);
    GTKdev::SetStatus(cf_scl,  f & CF_SUBCELL);
    GTKdev::SetStatus(cf_nlyr, f & CF_NOTLAYER);
    GTKdev::SetStatus(cf_lyr,  f & CF_LAYER);
    GTKdev::SetStatus(cf_nflg, f & CF_NOTFLAG);
    GTKdev::SetStatus(cf_flg,  f & CF_FLAG);
    GTKdev::SetStatus(cf_nftp, f & CF_NOTFTYPE);
    GTKdev::SetStatus(cf_ftp,  f & CF_FTYPE);

    char *s = 0;
    if (f & CF_NOTPARENT)
        s = cf->not_prnt_list();
    else if (f & CF_PARENT)
        s = cf->prnt_list();
    if (s) {
        gtk_entry_set_text(GTK_ENTRY(cf_pclent), s);
        delete [] s;
    }
    else
        gtk_entry_set_text(GTK_ENTRY(cf_pclent), "");

    s = 0;
    if (f & CF_NOTSUBCELL)
        s = cf->not_subc_list();
    else if (f & CF_SUBCELL)
        s = cf->subc_list();
    if (s) {
        gtk_entry_set_text(GTK_ENTRY(cf_sclent), s);
        delete [] s;
    }
    else
        gtk_entry_set_text(GTK_ENTRY(cf_sclent), "");

    s = 0;
    if (f & CF_NOTLAYER)
        s = cf->not_layer_list();
    else if (f & CF_LAYER)
        s = cf->layer_list();
    if (s) {
        gtk_entry_set_text(GTK_ENTRY(cf_lyrent), s);
        delete [] s;
    }
    else
        gtk_entry_set_text(GTK_ENTRY(cf_lyrent), "");

    s = 0;
    if (f & CF_NOTFLAG)
        s = cf->not_flag_list();
    else if (f & CF_FLAG)
        s = cf->flag_list();
    if (s) {
        gtk_entry_set_text(GTK_ENTRY(cf_flgent), s);
        delete [] s;
    }
    else
        gtk_entry_set_text(GTK_ENTRY(cf_flgent), "");

    s = 0;
    if (f & CF_NOTFTYPE)
        s = cf->not_ftype_list();
    else if (f & CF_FTYPE)
        s = cf->ftype_list();
    if (s) {
        gtk_entry_set_text(GTK_ENTRY(cf_ftpent), s);
        delete [] s;
    }
    else
        gtk_entry_set_text(GTK_ENTRY(cf_ftpent), "");

    if (cf_mode == Physical) {
        gtk_widget_set_sensitive(cf_ndev, false);
        gtk_widget_set_sensitive(cf_dev, false);
        gtk_widget_set_sensitive(cf_nvsm, true);
        gtk_widget_set_sensitive(cf_vsm, true);
        gtk_widget_set_sensitive(cf_npsm, true);
        gtk_widget_set_sensitive(cf_psm, true);
        gtk_widget_set_sensitive(cf_nspr, true);
        gtk_widget_set_sensitive(cf_spr, true);
        gtk_widget_set_sensitive(cf_nref, true);
        gtk_widget_set_sensitive(cf_ref, true);
    }
    else {
        gtk_widget_set_sensitive(cf_nvsm, false);
        gtk_widget_set_sensitive(cf_vsm, false);
        gtk_widget_set_sensitive(cf_npsm, false);
        gtk_widget_set_sensitive(cf_psm, false);
        gtk_widget_set_sensitive(cf_nspr, false);
        gtk_widget_set_sensitive(cf_spr, false);
        gtk_widget_set_sensitive(cf_nref, false);
        gtk_widget_set_sensitive(cf_ref, false);
        gtk_widget_set_sensitive(cf_ndev, true);
        gtk_widget_set_sensitive(cf_dev, true);
    }
}


// Create a new filter based on the present panel content.
//
cfilter_t *
sCf::new_filter()
{
    cfilter_t *cf = new cfilter_t(cf_mode);
    unsigned int f = 0;
    if (GTKdev::GetStatus(cf_imm))
        f |= CF_IMMUTABLE;
    else if (GTKdev::GetStatus(cf_nimm))
        f |= CF_NOTIMMUTABLE;
    if (GTKdev::GetStatus(cf_vsm))
        f |= CF_VIASUBM;
    else if (GTKdev::GetStatus(cf_nvsm))
        f |= CF_NOTVIASUBM;
    if (GTKdev::GetStatus(cf_lib))
        f |= CF_LIBRARY;
    else if (GTKdev::GetStatus(cf_nlib))
        f |= CF_NOTLIBRARY;
    if (GTKdev::GetStatus(cf_psm))
        f |= CF_PCELLSUBM;
    else if (GTKdev::GetStatus(cf_npsm))
        f |= CF_NOTPCELLSUBM;
    if (GTKdev::GetStatus(cf_dev))
        f |= CF_DEVICE;
    else if (GTKdev::GetStatus(cf_ndev))
        f |= CF_NOTDEVICE;
    if (GTKdev::GetStatus(cf_spr))
        f |= CF_PCELLSUPR;
    else if (GTKdev::GetStatus(cf_nspr))
        f |= CF_NOTPCELLSUPR;
    if (GTKdev::GetStatus(cf_top))
        f |= CF_TOPLEV;
    else if (GTKdev::GetStatus(cf_ntop))
        f |= CF_NOTTOPLEV;
    if (GTKdev::GetStatus(cf_mod))
        f |= CF_MODIFIED;
    else if (GTKdev::GetStatus(cf_nmod))
        f |= CF_NOTMODIFIED;
    if (GTKdev::GetStatus(cf_alt))
        f |= CF_WITHALT;
    else if (GTKdev::GetStatus(cf_nalt))
        f |= CF_NOTWITHALT;
    if (GTKdev::GetStatus(cf_ref))
        f |= CF_REFERENCE;
    else if (GTKdev::GetStatus(cf_nref))
        f |= CF_NOTREFERENCE;

    if (GTKdev::GetStatus(cf_pcl)) {
        const char *s = gtk_entry_get_text(GTK_ENTRY(cf_pclent));
        if (GTKdev::GetStatus(cf_npcl)) {
            f |= CF_NOTPARENT;
            cf->parse_parent(true, s, 0);
        }
        else {
            f |= CF_PARENT;
            cf->parse_parent(false, s, 0);
        }
    }
    if (GTKdev::GetStatus(cf_scl)) {
        const char *s = gtk_entry_get_text(GTK_ENTRY(cf_sclent));
        if (GTKdev::GetStatus(cf_nscl)) {
            f |= CF_NOTSUBCELL;
            cf->parse_subcell(true, s, 0);
        }
        else {
            f |= CF_SUBCELL;
            cf->parse_subcell(false, s, 0);
        }
    }
    if (GTKdev::GetStatus(cf_lyr)) {
        const char *s = gtk_entry_get_text(GTK_ENTRY(cf_lyrent));
        if (GTKdev::GetStatus(cf_nlyr)) {
            f |= CF_NOTLAYER;
            cf->parse_layers(true, s, 0);
        }
        else {
            f |= CF_LAYER;
            cf->parse_layers(false, s, 0);
        }
    }
    if (GTKdev::GetStatus(cf_flg)) {
        const char *s = gtk_entry_get_text(GTK_ENTRY(cf_flgent));
        if (GTKdev::GetStatus(cf_nflg)) {
            f |= CF_NOTFLAG;
            cf->parse_flags(true, s, 0);
        }
        else {
            f |= CF_FLAG;
            cf->parse_flags(false, s, 0);
        }
    }
    if (GTKdev::GetStatus(cf_ftp)) {
        const char *s = gtk_entry_get_text(GTK_ENTRY(cf_ftpent));
        if (GTKdev::GetStatus(cf_nftp)) {
            f |= CF_NOTFTYPE;
            cf->parse_ftypes(true, s, 0);
        }
        else {
            f |= CF_FTYPE;
            cf->parse_ftypes(false, s, 0);
        }
    }
    cf->set_flags(f);
    return (cf);
}


// Static function.
void
sCf::cf_cancel_proc(GtkWidget*, void*)
{
    XM()->PopUpCellFilt(0, MODE_OFF, Physical, 0, 0);
}


// Static function.
void
sCf::cf_action(GtkWidget *caller, void*)
{
    if (!Cf)
        return;
    const char *name = gtk_widget_get_name(caller);
    if (!strcmp(name, "Help")) {
        DSPmainWbag(PopUpHelp("xic:cfilt"))
        return;
    }
    if (!strcmp(name, "Apply")) {
        if (!Cf->cf_cb)
            return;
        cfilter_t *cf = Cf->new_filter();
        if (!cf)
            return;
        (*Cf->cf_cb)(cf, Cf->cf_arg);  // Receiver takes ownership of cf.
    }
}


// Static function.
void
sCf::cf_radio(GtkWidget *caller, void*)
{
    if (!Cf)
        return;
    if (!GTKdev::GetStatus(caller))
        return;
    if (caller == Cf->cf_nimm) {
        GTKdev::SetStatus(Cf->cf_imm, false);
        return;
    }
    if (caller == Cf->cf_imm) {
        GTKdev::SetStatus(Cf->cf_nimm, false);
        return;
    }
    if (caller == Cf->cf_nvsm) {
        GTKdev::SetStatus(Cf->cf_vsm, false);
        return;
    }
    if (caller == Cf->cf_vsm) {
        GTKdev::SetStatus(Cf->cf_nvsm, false);
        return;
    }
    if (caller == Cf->cf_nlib) {
        GTKdev::SetStatus(Cf->cf_lib, false);
        return;
    }
    if (caller == Cf->cf_lib) {
        GTKdev::SetStatus(Cf->cf_nlib, false);
        return;
    }
    if (caller == Cf->cf_npsm) {
        GTKdev::SetStatus(Cf->cf_psm, false);
        return;
    }
    if (caller == Cf->cf_psm) {
        GTKdev::SetStatus(Cf->cf_npsm, false);
        return;
    }
    if (caller == Cf->cf_ndev) {
        GTKdev::SetStatus(Cf->cf_dev, false);
        return;
    }
    if (caller == Cf->cf_dev) {
        GTKdev::SetStatus(Cf->cf_ndev, false);
        return;
    }
    if (caller == Cf->cf_nspr) {
        GTKdev::SetStatus(Cf->cf_spr, false);
        return;
    }
    if (caller == Cf->cf_spr) {
        GTKdev::SetStatus(Cf->cf_nspr, false);
        return;
    }
    if (caller == Cf->cf_ntop) {
        GTKdev::SetStatus(Cf->cf_top, false);
        return;
    }
    if (caller == Cf->cf_top) {
        GTKdev::SetStatus(Cf->cf_ntop, false);
        return;
    }
    if (caller == Cf->cf_nmod) {
        GTKdev::SetStatus(Cf->cf_mod, false);
        return;
    }
    if (caller == Cf->cf_mod) {
        GTKdev::SetStatus(Cf->cf_nmod, false);
        return;
    }
    if (caller == Cf->cf_nalt) {
        GTKdev::SetStatus(Cf->cf_alt, false);
        return;
    }
    if (caller == Cf->cf_alt) {
        GTKdev::SetStatus(Cf->cf_nalt, false);
        return;
    }
    if (caller == Cf->cf_nref) {
        GTKdev::SetStatus(Cf->cf_ref, false);
        return;
    }
    if (caller == Cf->cf_ref) {
        GTKdev::SetStatus(Cf->cf_nref, false);
        return;
    }
    if (caller == Cf->cf_npcl) {
        GTKdev::SetStatus(Cf->cf_pcl, false);
        return;
    }
    if (caller == Cf->cf_pcl) {
        GTKdev::SetStatus(Cf->cf_npcl, false);
        return;
    }
    if (caller == Cf->cf_nscl) {
        GTKdev::SetStatus(Cf->cf_scl, false);
        return;
    }
    if (caller == Cf->cf_scl) {
        GTKdev::SetStatus(Cf->cf_nscl, false);
        return;
    }
    if (caller == Cf->cf_nlyr) {
        GTKdev::SetStatus(Cf->cf_lyr, false);
        return;
    }
    if (caller == Cf->cf_lyr) {
        GTKdev::SetStatus(Cf->cf_nlyr, false);
        return;
    }
    if (caller == Cf->cf_nflg) {
        GTKdev::SetStatus(Cf->cf_flg, false);
        return;
    }
    if (caller == Cf->cf_flg) {
        GTKdev::SetStatus(Cf->cf_nflg, false);
        return;
    }
    if (caller == Cf->cf_nftp) {
        GTKdev::SetStatus(Cf->cf_ftp, false);
        return;
    }
    if (caller == Cf->cf_ftp) {
        GTKdev::SetStatus(Cf->cf_nftp, false);
        return;
    }
}


// Static function.
void
sCf::cf_sto_menu_proc(GtkWidget*, void *arg)
{
    if (!Cf)
        return;
    int ix = (intptr_t)arg;
    cfilter_t *cf = Cf->new_filter();
    if (!cf) {
        Log()->PopUpErrV("Failed: %s", Errs()->get_error());
        return;
    }
    if (Cf->cf_mode == Physical) {
        delete [] cf_phys_regs[ix];
        cf_phys_regs[ix] = cf->string();
    }
    else {
        delete [] cf_elec_regs[ix];
        cf_elec_regs[ix] = cf->string();
    }
    delete cf;
}


// Static function.
void
sCf::cf_rcl_menu_proc(GtkWidget*, void *arg)
{
    if (!Cf)
        return;
    int ix = (intptr_t)arg;
    cfilter_t *cf;
    if (Cf->cf_mode == Physical)
        cf = cfilter_t::parse(cf_phys_regs[ix], Physical, 0);
    else
        cf = cfilter_t::parse(cf_elec_regs[ix], Electrical, 0);
    Cf->setup(cf);
    delete cf;
}


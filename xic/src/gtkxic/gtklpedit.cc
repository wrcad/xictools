
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
#include "kwstr_lyr.h"
#include "kwstr_ext.h"
#include "kwstr_phy.h"
#include "kwstr_cvt.h"
#include "dsp_color.h"
#include "dsp_inlines.h"
#include "tech.h"
#include "tech_extract.h"
#include "tech_layer.h"
#include "tech_ldb3d.h"
#include "layertab.h"
#include "promptline.h"
#include "events.h"
#include "gtkmain.h"
#include "gtkinlines.h"
#include "gtkinterf/gtkfont.h"
#include <gdk/gdkkeysyms.h>


//--------------------------------------------------------------------
// Tech parameter editor pop-up
//
// Help system keywords used:
//  xic:lpedt

// Default window size, assumes 6X13 chars, 80 cols, 12 rows
// with a 2-pixel margin
#define DEF_WIDTH 364
#define DEF_HEIGHT 80

namespace {
    namespace gtklpedit {
        enum { LayerPage, ExtractPage, PhysicalPage, ConvertPage };
        enum {lpBoxLineStyle, lpLayerReorderMode, lpNoPlanarize,
            lpAntennaTotal, lpSubstrateEps, lpSubstrateThickness };

        struct sLpe
        {

            sLpe(GRobject, const char*, const char*);
            ~sLpe();

            void update(const char*, const char*);

            GtkWidget *shell() { return (lp_popup); }

        private:
            void insert(const char*, const char*, const char*);
            void undoop();
            void load(const char*);
            void clear_undo();
            void update();
            void call_callback(const char*);
            void select_range(int, int);
            void check_sens();

            static void lp_font_changed();
            static void lp_cancel_proc(GtkWidget*, void*);
            static void lp_help_proc(GtkWidget*, void*);
            static void lp_edit_proc(GtkWidget*, void*);
            static void lp_delete_proc(GtkWidget*, void*);
            static void lp_undo_proc(GtkWidget*, void*);
            static void lp_kw_proc(GtkWidget*, void*);
            static void lp_attr_proc(GtkWidget*, void*);
            static int lp_text_btn_hdlr(GtkWidget*, GdkEvent*, void*);
            static void lp_page_change_proc(GtkWidget*, void*, int, void*);

            GRobject lp_caller;             // initiating button
            GtkWidget *lp_popup;            // the popup shell
            GtkWidget *lp_text;             // text window

            GtkWidget *lp_lyr_menu;         // sub-menu
            GtkWidget *lp_ext_menu;         // sub-menu
            GtkWidget *lp_phy_menu;         // sub-menu
            GtkWidget *lp_cvt_menu;         // sub-menu

            GtkWidget *lp_label;            // info label
            GtkWidget *lp_cancel;           // quit menu entry
            GtkWidget *lp_edit;             // edit menu entry
            GtkWidget *lp_del;              // delete menu entry
            GtkWidget *lp_undo;             // undo menu entry
            GtkWidget *lp_ivis;             // Invisible menu entry
            GtkWidget *lp_nmrg;             // NoMerge menu entry
            GtkWidget *lp_xthk;             // CrossThick menu entry
            GtkWidget *lp_wira;             // WireActive menu entry
            GtkWidget *lp_noiv;             // NoInstView menu entry
            GtkWidget *lp_darkfield;        // DarkField menu entry
            GtkWidget *lp_rho;              // Rho menu entry
            GtkWidget *lp_sigma;            // Sigma menu entry
            GtkWidget *lp_rsh;              // Rsh menu entry
            GtkWidget *lp_tau;              // Tau menu entry
            GtkWidget *lp_epsrel;           // EpsRel menu entry
            GtkWidget *lp_cap;              // Capacitance menu entry
            GtkWidget *lp_lambda;           // Lambda menu entry
            GtkWidget *lp_tline;            // Tline menu entry
            GtkWidget *lp_antenna;          // Antenna menu entry
            GtkWidget *lp_nddt;             // NoDrcDataType menu entry

            CDl *lp_ldesc;                  // current layer
            int lp_line_selected;           // number of line selected or -1
            bool lp_in_callback;            // true when callback called
            int lp_start;
            int lp_end;
            int lp_page;
            lyrKWstruct lp_lyr_kw;
            extKWstruct lp_ext_kw;
            phyKWstruct lp_phy_kw;
            cvtKWstruct lp_cvt_kw;
        };

        sLpe *Lpe;
    }
}

using namespace gtklpedit;


void
cMain::PopUpLayerParamEditor(GRobject caller, ShowMode mode, const char *msg,
    const char *string)
{
    if (!GRX || !mainBag())
        return;
    if (mode == MODE_OFF) {
        delete Lpe;
        return;
    }
    if (mode == MODE_UPD) {
        // update the text
        if (Lpe)
            Lpe->update(msg, string);
        return;
    }
    if (Lpe)
        return;

    new sLpe(caller, msg, string);
    if (!Lpe->shell()) {
        delete Lpe;
        return;
    }
    gtk_window_set_transient_for(GTK_WINDOW(Lpe->shell()),
        GTK_WINDOW(mainBag()->Shell()));

    GRX->SetPopupLocation(GRloc(), Lpe->shell(), mainBag()->Viewport());
    gtk_widget_show(Lpe->shell());
}


namespace {
    const char *MIDX = "midx";
}

sLpe::sLpe(GRobject c, const char *msg, const char *string)
{
    Lpe = this;
    lp_caller = c;
    lp_popup = 0;
    lp_text = 0;
    lp_label = 0;
    lp_cancel = 0;
    lp_edit = 0;
    lp_del = 0;
    lp_undo = 0;
    lp_ivis = 0;
    lp_nmrg = 0;
    lp_xthk = 0;
    lp_wira = 0;
    lp_noiv = 0;
    lp_darkfield = 0;
    lp_rho = 0;
    lp_sigma = 0;
    lp_rsh = 0;
    lp_tau = 0;
    lp_epsrel = 0;
    lp_cap = 0;
    lp_lambda = 0;
    lp_tline = 0;
    lp_antenna = 0;
    lp_nddt = 0;
    lp_page = 0;

    lp_ldesc = 0;
    lp_line_selected = -1;
    lp_in_callback = false;
    lp_start = 0;
    lp_end = 0;

    lp_popup = gtk_NewPopup(0, "Tech Parameter Editor", lp_cancel_proc, 0);
    if (!lp_popup)
        return;

    GtkWidget *form = gtk_table_new(1, 3, false);
    gtk_widget_show(form);
    gtk_container_add(GTK_CONTAINER(lp_popup), form);

    //
    // menu bar
    //
    GtkAccelGroup *accel_group = gtk_accel_group_new();
    gtk_window_add_accel_group(GTK_WINDOW(lp_popup), accel_group);
    GtkWidget *menubar = gtk_menu_bar_new();
    g_object_set_data(G_OBJECT(lp_popup), "menubar", menubar);
    gtk_widget_show(menubar);
    GtkWidget *item;

    // Edit menu.
    item = gtk_menu_item_new_with_mnemonic("_Edit");
    gtk_widget_set_name(item, "Edit");
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), item);
    GtkWidget *submenu = gtk_menu_new();
    gtk_widget_show(submenu);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), submenu);

    // _Edit, <control>E, lp_edit_proc, 0, 0
    item = gtk_menu_item_new_with_mnemonic("_Edit");
    gtk_widget_set_name(item, "Edit");
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(lp_edit_proc), 0);
    gtk_widget_add_accelerator(item, "activate", accel_group, GDK_KEY_e,
        GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    lp_edit = item;

    // _Delete, <control>D, lp_delete_proc, 0, 0
    item = gtk_menu_item_new_with_mnemonic("_Delete");
    gtk_widget_set_name(item, "Delete");
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(lp_delete_proc), 0);
    gtk_widget_add_accelerator(item, "activate", accel_group, GDK_KEY_d,
        GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    lp_del = item;

    // _Undo, <control>U, lp_undo_proc, 0, 0
    item = gtk_menu_item_new_with_mnemonic("_Undo");
    gtk_widget_set_name(item, "Undo");
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(lp_undo_proc), 0);
    gtk_widget_add_accelerator(item, "activate", accel_group, GDK_KEY_u,
        GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    lp_undo = item;

    item = gtk_separator_menu_item_new();
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);

    // _Quit, <control>Q, lp_cancel_proc, 0, 0
    item = gtk_menu_item_new_with_mnemonic("_Quit");
    gtk_widget_set_name(item, "Quit");
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(lp_cancel_proc), 0);
    gtk_widget_add_accelerator(item, "activate", accel_group, GDK_KEY_q,
        GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    // Layer Keywords menu.
    item = gtk_menu_item_new_with_mnemonic("Layer _Keywords");
    gtk_widget_set_name(item, "Layer Keywords");
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), item);
    submenu = gtk_menu_new();
    gtk_widget_show(submenu);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), submenu);
    lp_lyr_menu = item;

    // LppName, 0, lp_kw_proc, lpLppName, 0
    item = gtk_menu_item_new_with_mnemonic("LppName");
    gtk_widget_set_name(item, "LppName");
    g_object_set_data(G_OBJECT(item), MIDX, (gpointer)(long)lpLppName);
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(lp_kw_proc), 0);

    // Description, 0, lp_kw_proc, lpDescription, 0);
    item = gtk_menu_item_new_with_mnemonic("Description");
    gtk_widget_set_name(item, "Description");
    g_object_set_data(G_OBJECT(item), MIDX, (gpointer)(long)lpDescription);
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(lp_kw_proc), 0);

    // NoSelect, 0, lp_kw_proc, lpNoSelect, 0);
    item = gtk_menu_item_new_with_mnemonic("NoSelect");
    gtk_widget_set_name(item, "NoSelect");
    g_object_set_data(G_OBJECT(item), MIDX, (gpointer)(long)lpNoSelect);
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(lp_kw_proc), 0);

    // NoMerge, 0, lp_kw_proc, lpNoMerge, 0);
    item = gtk_menu_item_new_with_mnemonic("NoMerge");
    gtk_widget_set_name(item, "NoMerge");
    g_object_set_data(G_OBJECT(item), MIDX, (gpointer)(long)lpNoMerge);
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(lp_kw_proc), 0);
    lp_nmrg = item;

    // WireActive, 0, lp_kw_proc, lpWireActive, 0);
    item = gtk_menu_item_new_with_mnemonic("WireActive");
    gtk_widget_set_name(item, "WireActive");
    g_object_set_data(G_OBJECT(item), MIDX, (gpointer)(long)lpWireActive);
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(lp_kw_proc), 0);
    lp_wira = item;

    // Symbolic, 0, lp_kw_proc, lpSymbolic, 0
    item = gtk_menu_item_new_with_mnemonic("Symbolic");
    gtk_widget_set_name(item, "Symbolic");
    g_object_set_data(G_OBJECT(item), MIDX, (gpointer)(long)lpSymbolic);
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(lp_kw_proc), 0);

    // The Invalid keyword is not really supported.  At best
    // it can be provided in the tech file only.

    // Invisible, 0, lp_kw_proc, lpInvisible, 0
    item = gtk_menu_item_new_with_mnemonic("Invisible");
    gtk_widget_set_name(item, "Invisible");
    g_object_set_data(G_OBJECT(item), MIDX, (gpointer)(long)lpInvisible);
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(lp_kw_proc), 0);
    lp_ivis = item;

    // NoInstView, 0, lp_kw_proc, lpNoInstView, 0
    item = gtk_menu_item_new_with_mnemonic("NoInstView");
    gtk_widget_set_name(item, "NoInstView");
    g_object_set_data(G_OBJECT(item), MIDX, (gpointer)(long)lpNoInstView);
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(lp_kw_proc), 0);
    lp_noiv = item;

    // WireWidth, 0, lp_kw_proc, lpWireWidth, 0
    item = gtk_menu_item_new_with_mnemonic("WireWidth");
    gtk_widget_set_name(item, "WireWidth");
    g_object_set_data(G_OBJECT(item), MIDX, (gpointer)(long)lpWireWidth);
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(lp_kw_proc), 0);

    // CrossThick, 0, lp_kw_proc, lpCrossThick, 0
    item = gtk_menu_item_new_with_mnemonic("CrossThick");
    gtk_widget_set_name(item, "CrossThick");
    g_object_set_data(G_OBJECT(item), MIDX, (gpointer)(long)lpCrossThick);
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(lp_kw_proc), 0);
    lp_xthk = item;

    // Extract Keywords menu.
    item = gtk_menu_item_new_with_mnemonic("Extract _Keywords");
    gtk_widget_set_name(item, "Extract Keywords");
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), item);
    submenu = gtk_menu_new();
    gtk_widget_show(submenu);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), submenu);
    lp_ext_menu = item;
    gtk_widget_hide(lp_ext_menu);

    // Conductor, 0, lp_kw_proc, exConductor, 0
    item = gtk_menu_item_new_with_mnemonic("Conductor");
    gtk_widget_set_name(item, "Conductor");
    g_object_set_data(G_OBJECT(item), MIDX, (gpointer)(long)exConductor);
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(lp_kw_proc), 0);

    // Routing, 0, lp_kw_proc, exRouting, 0);
    item = gtk_menu_item_new_with_mnemonic("Routing");
    gtk_widget_set_name(item, "Routing");
    g_object_set_data(G_OBJECT(item), MIDX, (gpointer)(long)exRouting);
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(lp_kw_proc), 0);

    // GroundPlane, 0, lp_kw_proc, exGroundPlane, 0);
    item = gtk_menu_item_new_with_mnemonic("GroundPlane");
    gtk_widget_set_name(item, "GroundPlane");
    g_object_set_data(G_OBJECT(item), MIDX, (gpointer)(long)exGroundPlane);
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(lp_kw_proc), 0);

    // GroundPlaneClear, 0, lp_kw_proc, exGroundPlaneClear, 0
    item = gtk_menu_item_new_with_mnemonic("GroundPlaneClear");
    gtk_widget_set_name(item, "GroundPlaneClear");
    g_object_set_data(G_OBJECT(item), MIDX, (gpointer)(long)exGroundPlaneClear);
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(lp_kw_proc), 0);

    // Contact, 0, lp_kw_proc, exContact, 0);
    item = gtk_menu_item_new_with_mnemonic("Contact");
    gtk_widget_set_name(item, "Contact");
    g_object_set_data(G_OBJECT(item), MIDX, (gpointer)(long)exContact);
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(lp_kw_proc), 0);

    // Via, 0, lp_kw_proc, exVia, 0);
    item = gtk_menu_item_new_with_mnemonic("Via");
    gtk_widget_set_name(item, "Via");
    g_object_set_data(G_OBJECT(item), MIDX, (gpointer)(long)exVia);
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(lp_kw_proc), 0);

    // ViaCut, 0, lp_kw_proc, exViaCut, 0
    item = gtk_menu_item_new_with_mnemonic("ViaCut");
    gtk_widget_set_name(item, "ViaCut");
    g_object_set_data(G_OBJECT(item), MIDX, (gpointer)(long)exViaCut);
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(lp_kw_proc), 0);

    // Dielectric, 0, lp_kw_proc, exDielectric, 0
    item = gtk_menu_item_new_with_mnemonic("Dielectric");
    gtk_widget_set_name(item, "Dielectric");
    g_object_set_data(G_OBJECT(item), MIDX, (gpointer)(long)exDielectric);
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(lp_kw_proc), 0);

    // DarkField, 0, lp_kw_proc, exDarkField, 0
    item = gtk_menu_item_new_with_mnemonic("DarkField");
    gtk_widget_set_name(item, "DarkField");
    g_object_set_data(G_OBJECT(item), MIDX, (gpointer)(long)exDarkField);
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(lp_kw_proc), 0);
    lp_darkfield = item;

    // Physical Keywords menu.
    item = gtk_menu_item_new_with_mnemonic("Physical _Keywords");
    gtk_widget_set_name(item, "Physical Keywords");
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), item);
    submenu = gtk_menu_new();
    gtk_widget_show(submenu);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), submenu);
    lp_phy_menu = item;
    gtk_widget_hide(lp_phy_menu);

    // Planarize, 0, lp_kw_proc, phPlanarize, 0
    item = gtk_menu_item_new_with_mnemonic("Planarize");
    gtk_widget_set_name(item, "Planarize");
    g_object_set_data(G_OBJECT(item), MIDX, (gpointer)(long)phPlanarize);
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(lp_kw_proc), 0);

    // Thickness, 0, lp_kw_proc, phThickness, 0
    item = gtk_menu_item_new_with_mnemonic("Thickness");
    gtk_widget_set_name(item, "Thickness");
    g_object_set_data(G_OBJECT(item), MIDX, (gpointer)(long)phThickness);
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(lp_kw_proc), 0);

    // FH_nhinc, 0, lp_kw_proc, phFH_nhinc, 0
    item = gtk_menu_item_new_with_mnemonic("FH_nhinc");
    gtk_widget_set_name(item, "FH_nhinc");
    g_object_set_data(G_OBJECT(item), MIDX, (gpointer)(long)phFH_nhinc);
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(lp_kw_proc), 0);

    // FH_rh, 0, lp_kw_proc, phFH_rh, 0
    item = gtk_menu_item_new_with_mnemonic("FH_rh");
    gtk_widget_set_name(item, "FH_rh");
    g_object_set_data(G_OBJECT(item), MIDX, (gpointer)(long)phFH_rh);
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(lp_kw_proc), 0);

    // Rho, 0, lp_kw_proc, phRho, 0
    item = gtk_menu_item_new_with_mnemonic("Rho");
    gtk_widget_set_name(item, "Rho");
    g_object_set_data(G_OBJECT(item), MIDX, (gpointer)(long)phRho);
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(lp_kw_proc), 0);
    lp_rho = item;

    // Sigma, 0, lp_kw_proc, phSigma, 0
    item = gtk_menu_item_new_with_mnemonic("Sigma");
    gtk_widget_set_name(item, "Sigma");
    g_object_set_data(G_OBJECT(item), MIDX, (gpointer)(long)phSigma);
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(lp_kw_proc), 0);
    lp_sigma = item;

    // Rsh, 0, lp_kw_proc, phRsh, 0
    item = gtk_menu_item_new_with_mnemonic("Rsh");
    gtk_widget_set_name(item, "Rsh");
    g_object_set_data(G_OBJECT(item), MIDX, (gpointer)(long)phRsh);
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(lp_kw_proc), 0);
    lp_rsh = item;

    // Tau, 0, lp_kw_proc, phTau, 0
    item = gtk_menu_item_new_with_mnemonic("Tau");
    gtk_widget_set_name(item, "Tau");
    g_object_set_data(G_OBJECT(item), MIDX, (gpointer)(long)phTau);
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(lp_kw_proc), 0);
    lp_tau = item;

    // EpsRel, 0, lp_kw_proc, phEpsRel, 0
    item = gtk_menu_item_new_with_mnemonic("EpsRel");
    gtk_widget_set_name(item, "EpsRel");
    g_object_set_data(G_OBJECT(item), MIDX, (gpointer)(long)phEpsRel);
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(lp_kw_proc), 0);
    lp_epsrel = item;

    // Capacitance, 0, lp_kw_proc, phCapacitance, 0
    item = gtk_menu_item_new_with_mnemonic("Capacitance");
    gtk_widget_set_name(item, "Capacitance");
    g_object_set_data(G_OBJECT(item), MIDX, (gpointer)(long)phCapacitance);
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(lp_kw_proc), 0);
    lp_cap = item;

    // Lambda, 0, lp_kw_proc, phLambda, 0);
    item = gtk_menu_item_new_with_mnemonic("Lambda");
    gtk_widget_set_name(item, "Lambda");
    g_object_set_data(G_OBJECT(item), MIDX, (gpointer)(long)phLambda);
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(lp_kw_proc), 0);
    lp_lambda = item;

    // Tline, 0, lp_kw_proc, phTline, 0
    item = gtk_menu_item_new_with_mnemonic("Tline");
    gtk_widget_set_name(item, "Tline");
    g_object_set_data(G_OBJECT(item), MIDX, (gpointer)(long)phTline);
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(lp_kw_proc), 0);
    lp_tline = item;

    // Antenna, 0, lp_kw_proc, phAntenna, 0
    item = gtk_menu_item_new_with_mnemonic("Antenna");
    gtk_widget_set_name(item, "Antenna");
    g_object_set_data(G_OBJECT(item), MIDX, (gpointer)(long)phAntenna);
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(lp_kw_proc), 0);
    lp_antenna = item;

    // Convert Keywords menu.
    item = gtk_menu_item_new_with_mnemonic("Convert _Keywords");
    gtk_widget_set_name(item, "Convert Keywords");
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), item);
    submenu = gtk_menu_new();
    gtk_widget_show(submenu);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), submenu);
    lp_cvt_menu = item;
    gtk_widget_hide(lp_cvt_menu);

    // StreamIn, 0, lp_kw_proc, cvStreamIn, 0
    item = gtk_menu_item_new_with_mnemonic("StreamIn");
    gtk_widget_set_name(item, "StreamIn");
    g_object_set_data(G_OBJECT(item), MIDX, (gpointer)(long)cvStreamIn);
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(lp_kw_proc), 0);

    // StreamOut, 0, lp_kw_proc, cvStreamOut, 0
    item = gtk_menu_item_new_with_mnemonic("StreamOut");
    gtk_widget_set_name(item, "StreamOut");
    g_object_set_data(G_OBJECT(item), MIDX, (gpointer)(long)cvStreamOut);
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(lp_kw_proc), 0);

    // NoDrcDataType, 0, lp_kw_proc, cvNoDrcDataType, 0
    item = gtk_menu_item_new_with_mnemonic("NoDrcDataType");
    gtk_widget_set_name(item, "NoDrcDataType");
    g_object_set_data(G_OBJECT(item), MIDX, (gpointer)(long)cvNoDrcDataType);
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(lp_kw_proc), 0);
    lp_nddt = item;

    // Global Attributes menu.
    item = gtk_menu_item_new_with_mnemonic("Global _Attributes");
    gtk_widget_set_name(item, "Global Attributes");
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), item);
    submenu = gtk_menu_new();
    gtk_widget_show(submenu);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), submenu);

    // BoxLineStyle, 0, lp_attr_proc, lpBoxLineStyle, 0
    item = gtk_menu_item_new_with_mnemonic("BoxLineStyle");
    gtk_widget_set_name(item, "BoxLineStyle");
    g_object_set_data(G_OBJECT(item), MIDX, (gpointer)(long)lpBoxLineStyle);
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(lp_kw_proc), 0);

    // LayerReorderMode, 0, lp_attr_proc, lpLayerReorderMode, 0
    item = gtk_menu_item_new_with_mnemonic("LayerReorderMode");
    gtk_widget_set_name(item, "LayerReorderMode");
    g_object_set_data(G_OBJECT(item), MIDX, (gpointer)(long)lpLayerReorderMode);
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(lp_kw_proc), 0);

    // NoPlanarize, 0, lp_attr_proc, lpNoPlanarize, 0
    item = gtk_menu_item_new_with_mnemonic("NoPlanarize");
    gtk_widget_set_name(item, "NoPlanarize");
    g_object_set_data(G_OBJECT(item), MIDX, (gpointer)(long)lpNoPlanarize);
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(lp_kw_proc), 0);

    // AntennaTotal, 0, lp_attr_proc, lpAntennaTotal, 0
    item = gtk_menu_item_new_with_mnemonic("AntennaTotal");
    gtk_widget_set_name(item, "AntennaTotal");
    g_object_set_data(G_OBJECT(item), MIDX, (gpointer)(long)lpAntennaTotal);
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(lp_kw_proc), 0);

    // SubstrateEps, 0, lp_attr_proc, lpSubstrateEps, 0
    item = gtk_menu_item_new_with_mnemonic("SubstrateEps");
    gtk_widget_set_name(item, "SubstrateEps");
    g_object_set_data(G_OBJECT(item), MIDX, (gpointer)(long)lpSubstrateEps);
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(lp_kw_proc), 0);

    // SubstrateThickness, 0, lp_attr_proc, lpSubstrateThickness, 0
    item = gtk_menu_item_new_with_mnemonic("SubstrateThickness");
    gtk_widget_set_name(item, "SubstrateThickness");
    g_object_set_data(G_OBJECT(item), MIDX, (gpointer)(long)lpSubstrateThickness);
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(lp_kw_proc), 0);

    // Help Menu.
    item = gtk_menu_item_new_with_mnemonic("_Help");
    gtk_menu_item_set_right_justified(GTK_MENU_ITEM(item), true);
    gtk_widget_set_name(item, "Help");
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), item);
    submenu = gtk_menu_new();
    gtk_widget_show(submenu);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), submenu);

    // _Help, <control>H, lp_help_proc, 0, 0
    item = gtk_menu_item_new_with_mnemonic("_Help");
    gtk_widget_set_name(item, "Help");
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(lp_help_proc), 0);
    gtk_widget_add_accelerator(item, "activate", accel_group, GDK_KEY_h,
        GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    int rowcnt = 0;
    gtk_table_attach(GTK_TABLE(form), menubar, 0, 1, rowcnt, rowcnt + 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    GtkWidget *nbook = gtk_notebook_new();
    gtk_widget_show(nbook);
    g_signal_connect(G_OBJECT(nbook), "switch-page",
        G_CALLBACK(lp_page_change_proc), 0);

    gtk_table_attach(GTK_TABLE(form), nbook, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    GtkWidget *label = gtk_label_new("Layer");
    gtk_widget_show(label);
    GtkWidget *label1 = gtk_label_new("Edit basic parameters for layer");
    gtk_widget_show(label1);
    gtk_notebook_append_page(GTK_NOTEBOOK(nbook), label1, label);

    label = gtk_label_new("Extract");
    gtk_widget_show(label);
    label1 = gtk_label_new("Edit extraction parameters for layer");
    gtk_widget_show(label1);
    gtk_notebook_append_page(GTK_NOTEBOOK(nbook), label1, label);

    label = gtk_label_new("Physical");
    gtk_widget_show(label);
    label1 = gtk_label_new("Edit physical attributes of layer");
    gtk_widget_show(label1);
    gtk_notebook_append_page(GTK_NOTEBOOK(nbook), label1, label);

    label = gtk_label_new("Convert");
    gtk_widget_show(label);
    label1 = gtk_label_new("Edit conversion parameters of layer");
    gtk_widget_show(label1);
    gtk_notebook_append_page(GTK_NOTEBOOK(nbook), label1, label);

    GtkWidget *contr;
    text_scrollable_new(&contr, &lp_text, FNT_FIXED);

    gtk_widget_add_events(lp_text, GDK_BUTTON_PRESS_MASK);
    g_signal_connect(G_OBJECT(lp_text), "button-press-event",
        G_CALLBACK(lp_text_btn_hdlr), 0);
    g_signal_connect_after(G_OBJECT(lp_text), "realize",
        G_CALLBACK(text_realize_proc), 0);

    GtkTextBuffer *textbuf =
        gtk_text_view_get_buffer(GTK_TEXT_VIEW(lp_text));
    const char *bclr = GRpkgIf()->GetAttrColor(GRattrColorLocSel);
    gtk_text_buffer_create_tag(textbuf, "primary", "background", bclr, NULL);

    gtk_widget_set_size_request(lp_text, DEF_WIDTH, DEF_HEIGHT);

    // The font change pop-up uses this to redraw the widget
    g_object_set_data(G_OBJECT(lp_text), "font_changed",
        (void*)lp_font_changed);

    gtk_table_attach(GTK_TABLE(form), contr, 0, 1, rowcnt, rowcnt + 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);
    rowcnt++;

    lp_label = gtk_label_new("");
    gtk_widget_show(lp_label);
    GtkWidget *frame = gtk_frame_new(0);
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), lp_label);

    gtk_table_attach(GTK_TABLE(form), frame, 0, 1, rowcnt, rowcnt + 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    if (lp_undo)
        gtk_widget_set_sensitive(lp_undo, false);
    update(msg, string);
}


sLpe::~sLpe()
{
    Lpe = 0;

    if (lp_page == LayerPage) {
        if (lp_lyr_kw.is_editing())
            PL()->AbortEdit();
    }
    else if (lp_page == ExtractPage) {
        if (lp_ext_kw.is_editing())
            PL()->AbortEdit();
    }
    else if (lp_page == PhysicalPage) {
        if (lp_phy_kw.is_editing())
            PL()->AbortEdit();
    }
    else if (lp_page == ConvertPage) {
        if (lp_cvt_kw.is_editing())
            PL()->AbortEdit();
    }
    if (lp_caller)
        GRX->Deselect(lp_caller);
    if (lp_popup)
        gtk_widget_destroy(lp_popup);
}


void
sLpe::update(const char *msg, const char *string)
{
    if (lp_ldesc != LT()->CurLayer() || !lp_in_callback) {
        load(string);
        lp_ldesc = LT()->CurLayer();
    }

    if (lp_page == LayerPage) {
        if (DSP()->CurMode() == Physical) {
            gtk_widget_show(lp_nmrg);
            gtk_widget_show(lp_xthk);
            gtk_widget_hide(lp_wira);
            gtk_widget_hide(lp_noiv);
            gtk_widget_set_sensitive(lp_ivis, true);
        }
        else {
            gtk_widget_hide(lp_nmrg);
            gtk_widget_hide(lp_xthk);
            gtk_widget_show(lp_wira);
            gtk_widget_show(lp_noiv);
            gtk_widget_set_sensitive(lp_ivis,
                !LT()->CurLayer()->isWireActive());
        }
    }
    else if (lp_page == ExtractPage) {
        if (DSP()->CurMode() == Physical) {
            gtk_widget_set_sensitive(lp_ext_menu, true);
            if (lp_ldesc) {
                char *s = Tech()->ExtCheckLayerKeywords(lp_ldesc);
                if (s) {
                    DSPmainWbag(PopUpInfo(MODE_ON, s))
                    delete [] s;
                }
            } 
            gtk_widget_set_sensitive(lp_darkfield, true);

            if (lp_ldesc) {
                if (lp_ldesc->isConductor()) {
                    if (lp_ldesc->isGroundPlane()) {
                        if (lp_ldesc->isDarkField())
                            gtk_widget_set_sensitive(lp_darkfield, false);
                    }
                }
            }
        }
        else {
            gtk_widget_set_sensitive(lp_ext_menu, false);
        }
    }
    else if (lp_page == PhysicalPage) {
        if (DSP()->CurMode() == Physical) {
            gtk_widget_set_sensitive(lp_phy_menu, true);
            gtk_widget_set_sensitive(lp_rho, true);
            gtk_widget_set_sensitive(lp_sigma, true);
            gtk_widget_set_sensitive(lp_rsh, true);
            gtk_widget_set_sensitive(lp_tau, true);
            gtk_widget_set_sensitive(lp_epsrel, true);
            gtk_widget_set_sensitive(lp_cap, true);
            gtk_widget_set_sensitive(lp_lambda, true);
            gtk_widget_set_sensitive(lp_tline, true);
            gtk_widget_set_sensitive(lp_antenna, true);

            if (lp_ldesc) {
                if (lp_ldesc->isConductor()) {
                    gtk_widget_set_sensitive(lp_epsrel, false);
                    if (lp_ldesc->isGroundPlane()) {
                        gtk_widget_set_sensitive(lp_cap, false);
                        gtk_widget_set_sensitive(lp_tline, false);
                    }
                    if (!lp_ldesc->isRouting())
                        gtk_widget_set_sensitive(lp_antenna, false);
                }
                else if (lp_ldesc->isVia() || lp_ldesc->isViaCut()) {
                    gtk_widget_set_sensitive(lp_rho, false);
                    gtk_widget_set_sensitive(lp_sigma, false);
                    gtk_widget_set_sensitive(lp_rsh, false);
                    gtk_widget_set_sensitive(lp_tau, false);
                    gtk_widget_set_sensitive(lp_cap, false);
                    gtk_widget_set_sensitive(lp_lambda, false);
                    gtk_widget_set_sensitive(lp_tline, false);
                    gtk_widget_set_sensitive(lp_antenna, false);
                }
                else {
                    TechLayerParams *lp = tech_prm(lp_ldesc);
                    if (lp->epsrel() > 0.0) {
                        gtk_widget_set_sensitive(lp_rho, false);
                        gtk_widget_set_sensitive(lp_sigma, false);
                        gtk_widget_set_sensitive(lp_rsh, false);
                        gtk_widget_set_sensitive(lp_tau, false);
                        gtk_widget_set_sensitive(lp_cap, false);
                        gtk_widget_set_sensitive(lp_lambda, false);
                        gtk_widget_set_sensitive(lp_tline, false);
                        gtk_widget_set_sensitive(lp_antenna, false);
                    }
                    else if (lp->rho() > 0.0 || lp->ohms_per_sq() > 0.0 ||
                            lp->tau() > 0.0 || lp->cap_per_area() > 0.0 ||
                            lp->cap_per_perim() > 0.0 ||
                            lp->lambda() > 0.0 || lp->gp_lname() ||
                            lp->ant_ratio() > 0.0)
                        gtk_widget_set_sensitive(lp_epsrel, false);
                }
            }
        }
        else {
            gtk_widget_set_sensitive(lp_phy_menu, false);
        }
    }
    else if (lp_page == ConvertPage) {
        gtk_widget_set_sensitive(lp_nddt, (DSP()->CurMode() == Physical));
    }
    gtk_label_set_text(GTK_LABEL(lp_label), msg);
}


void
sLpe::insert(const char *str, const char *l1, const char *l2)
{
    if (lp_undo)
        gtk_widget_set_sensitive(lp_undo, false);

    char *status = 0;
    if (lp_page == LayerPage)
        status = lp_lyr_kw.insert_keyword_text(str, 0, 0);
    else if (lp_page == ExtractPage)
        status = lp_ext_kw.insert_keyword_text(str, l1, l2);
    else if (lp_page == PhysicalPage)
        status = lp_phy_kw.insert_keyword_text(str, 0, 0);
    else if (lp_page == ConvertPage)
        status = lp_cvt_kw.insert_keyword_text(str, l1, 0);

    update();
    if (status) {
        PL()->ShowPrompt(status);
        delete [] status;
    }
    else
        PL()->ErasePrompt();
}


// Undo the last operation.
//
void
sLpe::undoop()
{
    if (lp_page == LayerPage)
        lp_lyr_kw.undo_keyword_change();
    else if (lp_page == ExtractPage)
        lp_ext_kw.undo_keyword_change();
    else if (lp_page == PhysicalPage)
        lp_phy_kw.undo_keyword_change();
    else if (lp_page == ConvertPage)
        lp_cvt_kw.undo_keyword_change();

    if (lp_undo)
        gtk_widget_set_sensitive(lp_undo, false);
    update();
}


void
sLpe::load(const char *string)
{
    if (lp_undo)
        gtk_widget_set_sensitive(lp_undo, false);

    if (lp_page == LayerPage)
        lp_lyr_kw.load_keywords(LT()->CurLayer(), string);
    else if (lp_page == ExtractPage)
        lp_ext_kw.load_keywords(LT()->CurLayer(), string);
    else if (lp_page == PhysicalPage)
        lp_phy_kw.load_keywords(LT()->CurLayer(), string);
    else if (lp_page == ConvertPage)
        lp_cvt_kw.load_keywords(LT()->CurLayer(), string);

    update();
}


void
sLpe::clear_undo()
{
    if (lp_page == LayerPage)
        lp_lyr_kw.clear_undo_list();
    else if (lp_page == ExtractPage)
        lp_ext_kw.clear_undo_list();
    else if (lp_page == PhysicalPage)
        lp_phy_kw.clear_undo_list();
    else if (lp_page == ConvertPage)
        lp_cvt_kw.clear_undo_list();

    if (lp_undo)
        gtk_widget_set_sensitive(lp_undo, false);
}


void
sLpe::update()
{
    select_range(0, 0);
    stringlist *list = 0;

    if (lp_page == LayerPage) {
        list = lp_lyr_kw.list();
        lp_lyr_kw.sort();
    }
    else if (lp_page == ExtractPage) {
        list = lp_ext_kw.list();
        lp_ext_kw.sort();
    }
    else if (lp_page == PhysicalPage) {
        list = lp_phy_kw.list();
        lp_phy_kw.sort();
    }
    else if (lp_page == ConvertPage) {
        list = lp_cvt_kw.list();
        lp_cvt_kw.sort();
    }

    GdkColor *c = gtk_PopupColor(GRattrColorHl4);
    double val = text_get_scroll_value(lp_text);
    text_set_chars(lp_text, "");
    for (stringlist *l = list; l; l = l->next) {
        char *t = l->string;
        while (*t && !isspace(*t))
            t++;
        text_insert_chars_at_point(lp_text, c, l->string, t - l->string, -1);
        if (*t)
            text_insert_chars_at_point(lp_text, 0, t, -1, -1);
        text_insert_chars_at_point(lp_text, 0, "\n", 1, -1);
    }
    text_set_scroll_value(lp_text, val);

    if (lp_page == LayerPage) {
        if (lp_undo && (lp_lyr_kw.undo_list() || lp_lyr_kw.new_string()))
            gtk_widget_set_sensitive(lp_undo, true);
    }
    else if (lp_page == ExtractPage) {
        if (lp_undo && (lp_ext_kw.undo_list() || lp_ext_kw.new_string()))
            gtk_widget_set_sensitive(lp_undo, true);
    }
    else if (lp_page == PhysicalPage) {
        if (lp_undo && (lp_phy_kw.undo_list() || lp_phy_kw.new_string()))
            gtk_widget_set_sensitive(lp_undo, true);
    }
    else if (lp_page == ConvertPage) {
        if (lp_undo && (lp_cvt_kw.undo_list() || lp_cvt_kw.new_string()))
            gtk_widget_set_sensitive(lp_undo, true);
    }
    check_sens();
}


void
sLpe::call_callback(const char *before)
{
    char *after = text_get_chars(lp_text, 0, -1);
    if (strcmp(before, after)) {
        lp_in_callback = true;
        char *err = 0;
        if (lp_page == LayerPage)
            err = lyrKWstruct::set_settings(LT()->CurLayer(), after);
        else if (lp_page == ExtractPage)
            err = extKWstruct::set_settings(LT()->CurLayer(), after);
        else if (lp_page == PhysicalPage)
            err = phyKWstruct::set_settings(LT()->CurLayer(), after);
        else if (lp_page == ConvertPage)
            err = cvtKWstruct::set_settings(LT()->CurLayer(), after);
        if (!Lpe)
            return;
        if (!err)
            err = lstring::copy(
                "Succeeded - keywords attached to current layer");
        update(err, 0);
        delete [] err;
        lp_in_callback = false;
    }
    delete [] after;
}


// Static function.
// This is called when the font is changed.
//
void
sLpe::lp_font_changed()
{
    if (Lpe)
        Lpe->update();
}


// Select the chars in the range, start=end deselects existing.  In
// GTK-1, selecting gives blue inverse, which turns gray if
// unselected, retaining an indication for the buttons.  GTK-2
// doesn't do this automatically so we provide something similar here.
//
void
sLpe::select_range(int start, int end)
{
    GtkTextBuffer *textbuf =
        gtk_text_view_get_buffer(GTK_TEXT_VIEW(lp_text));
    GtkTextIter istart, iend;
    if (lp_end != lp_start) {
        gtk_text_buffer_get_iter_at_offset(textbuf, &istart, lp_start);
        gtk_text_buffer_get_iter_at_offset(textbuf, &iend, lp_end);
        gtk_text_buffer_remove_tag_by_name(textbuf, "primary", &istart, &iend);
    }
    text_select_range(lp_text, start, end);
    if (end != start) {
        gtk_text_buffer_get_iter_at_offset(textbuf, &istart, start);
        gtk_text_buffer_get_iter_at_offset(textbuf, &iend, end);
        gtk_text_buffer_apply_tag_by_name(textbuf, "primary", &istart, &iend);
    }
    lp_start = start;
    lp_end = end;
    check_sens();
}


void
sLpe::check_sens()
{
    char *s = text_get_selection(Lpe->lp_text);
    if (Lpe->lp_edit) {
        if (Lpe->lp_page == LayerPage) {
            if (s) {
                // Don't turn on editing for booleans.
                if (lstring::prefix(Tkw.Symbolic(), s) ||
                        lstring::prefix(Tkw.NoMerge(), s) ||
                        lstring::prefix(Tkw.Invisible(), s) ||
                        lstring::prefix(Tkw.NoSelect(), s) ||
                        lstring::prefix(Tkw.WireActive(), s) ||
                        lstring::prefix(Tkw.NoInstView(), s))
                    gtk_widget_set_sensitive(Lpe->lp_edit, false);
                else
                    gtk_widget_set_sensitive(Lpe->lp_edit, true);
            }
            else
                gtk_widget_set_sensitive(Lpe->lp_edit, false);
        }
        else if (Lpe->lp_page == ExtractPage) {
            if (s) {
                if (lstring::prefix(Ekw.GroundPlane(), s) ||
                        lstring::prefix(Ekw.DarkField(), s))
                    gtk_widget_set_sensitive(Lpe->lp_edit, false);
                else
                    gtk_widget_set_sensitive(Lpe->lp_edit, true);
            }
            else
                gtk_widget_set_sensitive(Lpe->lp_edit, false);
        }       
        else if (Lpe->lp_page == PhysicalPage) {
            if (s)
                gtk_widget_set_sensitive(Lpe->lp_edit, true);
            else
                gtk_widget_set_sensitive(Lpe->lp_edit, false);
        }       
        else if (Lpe->lp_page == ConvertPage) {
            if (s)
                gtk_widget_set_sensitive(Lpe->lp_edit, false);
            else
                gtk_widget_set_sensitive(Lpe->lp_edit, true);
        }
    }

    if (Lpe->lp_del) {
        if (s)
            gtk_widget_set_sensitive(Lpe->lp_del, true);
        else
            gtk_widget_set_sensitive(Lpe->lp_del, false);
    }
    if (!s)
        Lpe->lp_line_selected = -1;
    delete [] s;

}


// Static function.
// Pop down the extraction editor.
//
void
sLpe::lp_cancel_proc(GtkWidget*, void*)
{
    XM()->PopUpLayerParamEditor(0, MODE_OFF, 0, 0);
}


// Static function.
// Enter help mode.
//
void
sLpe::lp_help_proc(GtkWidget*, void*)
{
    DSPmainWbag(PopUpHelp("xic:lpedt"))
}


// Static function.
// Edit the selected entry.
//
void
sLpe::lp_edit_proc(GtkWidget*, void*)
{
    if (!Lpe)
        return;
    int start, end;
    start = Lpe->lp_start;
    end = Lpe->lp_end;
    if (start == end)
        return;
    char *string = text_get_chars(Lpe->lp_text, start, end);
    char *nl = string + strlen(string) - 1;
    if (*nl == '\n')
        *nl = 0;

    int type = -1;
    char *nstring = 0;
    if (Lpe->lp_page == LayerPage) {
        type = lyrKWstruct::kwtype(string);
        nstring = Lpe->lp_lyr_kw.get_string_for(type, string);
        if (nstring) {
            char *before = text_get_chars(Lpe->lp_text, 0, -1);
            Lpe->insert(nstring, 0, 0);
            delete [] nstring;
            Lpe->call_callback(before);
            delete [] before;
        }
    }
    else if (Lpe->lp_page == ExtractPage) {
        type = extKWstruct::kwtype(string);
        nstring = Lpe->lp_ext_kw.get_string_for(type, string);
        if (nstring) {
            char *before = text_get_chars(Lpe->lp_text, 0, -1);
 
            // The Via keyword is the only type which can appear more
            // than once.  The layer reference names are passed to
            // distinguish the entry.

            if (type == exVia) {
                char *s = string;
                lstring::advtok(&s);
                char *ln1 = lstring::gettok(&s);
                char *ln2 = lstring::gettok(&s);
                Lpe->insert(nstring, ln1, ln2);
                delete [] ln1;
                delete [] ln2;
            }
            else
                Lpe->insert(nstring, 0, 0);
            delete [] nstring;
            Lpe->call_callback(before);
            delete [] before;
        }
    }
    else if (Lpe->lp_page == PhysicalPage) {
        type = phyKWstruct::kwtype(string);
        nstring = Lpe->lp_phy_kw.get_string_for(type, string);
        if (nstring) {
            char *before = text_get_chars(Lpe->lp_text, 0, -1);
            Lpe->insert(nstring, 0, 0);
            delete [] nstring;
            Lpe->call_callback(before);
            delete [] before;
        }
    }
    else if (Lpe->lp_page == ConvertPage) {
        type = cvtKWstruct::kwtype(string);
        nstring = Lpe->lp_cvt_kw.get_string_for(type, string);
        if (nstring) {
            char *before = text_get_chars(Lpe->lp_text, 0, -1);
            Lpe->insert(nstring, string, 0);
            delete [] nstring;
            Lpe->call_callback(before);
            delete [] before;
        }
    }
    delete [] string;
}


// Static function.
// Delete the selected entry.
//
void
sLpe::lp_delete_proc(GtkWidget*, void*)
{
    if (Lpe) {
        int start, end;
        start = Lpe->lp_start;
        end = Lpe->lp_end;
        if (start == end)
            return;
        char *string = text_get_chars(Lpe->lp_text, start, end);
        char *nl = string + strlen(string) - 1;
        if (*nl == '\n')
            *nl = 0;

        if (Lpe->lp_page == LayerPage) {
            int type = lyrKWstruct::kwtype(string);
            if (type != lpNil) {
                char *before = text_get_chars(Lpe->lp_text, 0, -1);
                Lpe->clear_undo();
                Lpe->lp_lyr_kw.remove_keyword_text(type);
                Lpe->update();
                Lpe->call_callback(before);
                delete [] before;
            }
        }
        else if (Lpe->lp_page == ExtractPage) {
            int type = extKWstruct::kwtype(string);
            if (type != exNil) {
                char *before = text_get_chars(Lpe->lp_text, 0, -1);
                Lpe->clear_undo();

                // The Via keyword is the only type which can appear
                // more than once.  The layer reference names are
                // passed to distinguish the entry.

                if (type == exVia) {
                    char *s = string;
                    lstring::advtok(&s);
                    char *ln1 = lstring::gettok(&s);
                    char *ln2 = lstring::gettok(&s);
                    Lpe->lp_ext_kw.remove_keyword_text(type, false, ln1, ln2);
                    delete [] ln1;
                    delete [] ln2;
                }
                else
                    Lpe->lp_ext_kw.remove_keyword_text(type);
                Lpe->update();
                Lpe->call_callback(before);
                delete [] before;
            }
        }
        else if (Lpe->lp_page == PhysicalPage) {
            int type = phyKWstruct::kwtype(string);
            if (type != phNil) {
                char *before = text_get_chars(Lpe->lp_text, 0, -1);
                Lpe->clear_undo();
                Lpe->lp_phy_kw.remove_keyword_text(type);
                Lpe->update();
                Lpe->call_callback(before);
                delete [] before;
            }
        }
        else if (Lpe->lp_page == ConvertPage) {
            int type = cvtKWstruct::kwtype(string);
            if (type != cvNil) {
                char *before = text_get_chars(Lpe->lp_text, 0, -1);
                Lpe->clear_undo();
                Lpe->lp_cvt_kw.remove_keyword_text(type, false, string);
                Lpe->update();
                Lpe->call_callback(before);
                delete [] before;
            }
        }
        delete [] string;
    }
}


// Static function.
// Undo the last operation.
//
void
sLpe::lp_undo_proc(GtkWidget*, void*)
{
    if (!Lpe)
        return;
    char *before = text_get_chars(Lpe->lp_text, 0, -1);
    Lpe->undoop();
    Lpe->call_callback(before);
    delete [] before;
}


// Static function.
// Handle addition of a new keyword entry.
//
void
sLpe::lp_kw_proc(GtkWidget *caller, void*)
{
    long type = (long)g_object_get_data(G_OBJECT(caller), MIDX);
    if (!Lpe)
        return;
    char *string = 0;
    if (Lpe->lp_page == LayerPage)
        string = Lpe->lp_lyr_kw.get_string_for(type, 0);
    else if (Lpe->lp_page == ExtractPage)
        string = Lpe->lp_ext_kw.get_string_for(type, 0);
    else if (Lpe->lp_page == PhysicalPage)
        string = Lpe->lp_phy_kw.get_string_for(type, 0);
    else if (Lpe->lp_page == ConvertPage)
        string = Lpe->lp_cvt_kw.get_string_for(type, 0);

    if (string) {
        char *before = text_get_chars(Lpe->lp_text, 0, -1);
        if (Lpe->lp_page == ExtractPage && type == exVia) {
            // Pass two dummy via strings so that existing Via
            // keywords are not deleted.

            Lpe->insert(string, "", "");
        }
        else
            Lpe->insert(string, 0, 0);
        Lpe->call_callback(before);
        delete [] before;
    }
}


// Static function.
// Handle attribute.
//
void
sLpe::lp_attr_proc(GtkWidget *caller, void*)
{
    long type = (long)g_object_get_data(G_OBJECT(caller), MIDX);
    if (!Lpe)
        return;
    char tbuf[64];
    if (type == lpBoxLineStyle) {
        sprintf(tbuf, "0x%x", DSP()->BoxLinestyle()->mask);
        char *in = Lpe->lp_ext_kw.prompt(
            "Enter highlighting box linestyle mask: ", tbuf);
        if (in) {
            if (*in)
                CDvdb()->setVariable(VA_BoxLineStyle, in);
            else
                CDvdb()->clearVariable(VA_BoxLineStyle);
            PL()->ErasePrompt();
        }
    }
    else if (type == lpLayerReorderMode) {
        sprintf(tbuf, "%d", Tech()->ReorderMode());
        char *in = Lpe->lp_ext_kw.prompt(
            "Enter Via layer reordering mode (integer 0-2): ", tbuf);
        unsigned int d;
        if (in) {
            if (sscanf(in, "%u", &d) == 1 &&  d <= 2) {
                if (d > 0)
                    CDvdb()->setVariable(VA_LayerReorderMode, in);
                else
                    CDvdb()->clearVariable(VA_LayerReorderMode);
                PL()->ErasePrompt();
            }
            else
                PL()->ShowPrompt("Bad value, must be integer 0-2 ");
        }
    }
    else if (type == lpNoPlanarize) {
        char *in;
        if (Tech()->IsNoPlanarize()) {
            in = Lpe->lp_ext_kw.prompt(
                "NoPlanarize is currently set, press Enter to unset, "
                "Esc to abort. ", "");
            if (in)
                CDvdb()->clearVariable(VA_NoPlanarize);
        }
        else {
            in = Lpe->lp_ext_kw.prompt(
                "NoPlanarize is currently unset, press Enter to set. "
                "Esc to abort. ", "");
            if (in)
                CDvdb()->setVariable(VA_NoPlanarize, "");
        }
    }
    else if (type == lpAntennaTotal) {
        sprintf(tbuf, "%g", Tech()->AntennaTotal());
        char *in = Lpe->lp_ext_kw.prompt(
            "Enter total net antenna ratio limit: ", tbuf);
        double d;
        if (in) {
            if (sscanf(in, "%lf", &d) == 1 &&  d >= 0.0) {
                if (d > 0.0)
                    CDvdb()->setVariable(VA_AntennaTotal, in);
                else
                    CDvdb()->clearVariable(VA_AntennaTotal);
                PL()->ErasePrompt();
            }
            else
                PL()->ShowPrompt("Bad value, must be >= 0.0");
        }
    }
    else if (type == lpSubstrateEps) {
        sprintf(tbuf, "%g", Tech()->SubstrateEps());
        char *in = Lpe->lp_ext_kw.prompt(
            "Enter substrate relative dielectric constant: ", tbuf);
        double d;
        if (in) {
            if (sscanf(in, "%lf", &d) == 1 && d >= SUBSTRATE_EPS_MIN &&
                    d <= SUBSTRATE_EPS_MAX) {
                if (d != SUBSTRATE_EPS)
                    CDvdb()->setVariable(VA_SubstrateEps, in);
                else
                    CDvdb()->clearVariable(VA_SubstrateEps);
                PL()->ErasePrompt();
            }
            else
                PL()->ShowPromptV("Bad value, must be %.3f - %.3f",
                    SUBSTRATE_EPS_MIN, SUBSTRATE_EPS_MAX);
        }
    }
    else if (type == lpSubstrateThickness) {
        sprintf(tbuf, "%g", Tech()->SubstrateThickness());
        char *in = Lpe->lp_ext_kw.prompt(
            "Enter substrate thickness in microns: ", tbuf);
        double d;
        if (in) {
            if (sscanf(in, "%lf", &d) == 1 && d >= SUBSTRATE_THICKNESS_MIN &&
                    d <= SUBSTRATE_THICKNESS_MAX) {
                if (d != SUBSTRATE_THICKNESS)
                    CDvdb()->setVariable(VA_SubstrateThickness, in);
                else
                    CDvdb()->clearVariable(VA_SubstrateThickness);
                PL()->ErasePrompt();
            }
            else
                PL()->ShowPromptV("Bad value, must be %.3f - %.3f",
                    SUBSTRATE_THICKNESS_MIN, SUBSTRATE_THICKNESS_MAX);
        }
    }
}


// Static function.
// Handle button presses in the text area.  If neither edit mode or
// delete mode is active, highlight the line pointed to.  Otherwise,
// perform the operation on the pointed-to line.
//
int
sLpe::lp_text_btn_hdlr(GtkWidget *caller, GdkEvent *event, void*)
{
    if (!Lpe)
        return (false);
    if (event->type != GDK_BUTTON_PRESS)
        return (true);

    Lpe->select_range(0, 0);
    char *string = text_get_chars(caller, 0, -1);
    int x = (int)event->button.x;
    int y = (int)event->button.y;
    gtk_text_view_window_to_buffer_coords(GTK_TEXT_VIEW(caller),
        GTK_TEXT_WINDOW_WIDGET, x, y, &x, &y);
    GtkTextIter ihere, iline;
    gtk_text_view_get_iter_at_location(GTK_TEXT_VIEW(caller), &ihere, x, y);
    gtk_text_view_get_line_at_y(GTK_TEXT_VIEW(caller), &iline, y, 0);
    char *line_start = string + gtk_text_iter_get_offset(&iline);
    y = gtk_text_iter_get_line(&iline);
    if (Lpe->lp_line_selected >= 0 && Lpe->lp_line_selected == y) {
        delete [] string;
        Lpe->select_range(0, 0);
        return (true);
    }
    Lpe->lp_line_selected = y;
    int start = (line_start - string);

    // find the end of line
    char *s = line_start;
    for ( ; *s && *s != '\n'; s++) ;

    Lpe->select_range(start, start + (s - line_start));
    delete [] string;
    return (true);
}


void
sLpe::lp_page_change_proc(GtkWidget*, void*, int page, void*)
{
    if (!Lpe)
        return;
    switch (Lpe->lp_page) {
        case LayerPage:
            gtk_widget_hide(Lpe->lp_lyr_menu);
            break;
        case ExtractPage:
            gtk_widget_hide(Lpe->lp_ext_menu);
            break;
        case PhysicalPage:
            gtk_widget_hide(Lpe->lp_phy_menu);
            break;
        case ConvertPage:
            gtk_widget_hide(Lpe->lp_cvt_menu);
            break;
    }
    Lpe->lp_page = page;
    switch (Lpe->lp_page) {
        case LayerPage:
            gtk_widget_show(Lpe->lp_lyr_menu);
            break;
        case ExtractPage:
            gtk_widget_show(Lpe->lp_ext_menu);
            break;
        case PhysicalPage:
            gtk_widget_show(Lpe->lp_phy_menu);
            break;
        case ConvertPage:
            gtk_widget_show(Lpe->lp_cvt_menu);
            break;
    }
    if (Lpe->lp_text)
        Lpe->update(0, 0);
}


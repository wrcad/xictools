
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
#include "scedif.h"
#include "cd_lgen.h"
#include "dsp_layer.h"
#include "dsp_color.h"
#include "dsp_inlines.h"
#include "menu.h"
#include "layertab.h"
#include "select.h"
#include "gtkmain.h"
#include "gtkparam.h"
#include "gtkcoord.h"
#include "gtkhtext.h"
#include "gtkinterf/gtkfont.h"
#include "miscutil/pathlist.h"


// Color selection panel
//
// Help system keywords used:
//  xic:color

#define CLR_CURLYR  -1

namespace {
    // The menus for attribute colors.
    //
    struct clritem
    {
        const char *descr;
        int tab_indx;
    };

    // Attributes, electrical and physical mode.
    //
    clritem Menu1[] = {
        { "Current Layer", CLR_CURLYR },
        { "Background", BackgroundColor },
        { "Coarse Grid", CoarseGridColor },
        { "Fine Grid", FineGridColor },
        { "Ghosting", GhostColor },
        { "Highlighting", HighlightingColor },
        { "Selection Color 1", SelectColor1 },
        { "Selection Color 2", SelectColor2 },
        { "Terminals", MarkerColor },
        { "Instance Boundary", InstanceBBColor },
        { "Instance Name Text", InstanceNameColor },
        { "Instance Size Text", InstanceSizeColor },
        { 0, 0}
    };

    // Prompt line, electrical and physical mode.
    //
    clritem Menu2[] = {
        { "Text", PromptEditTextColor },
        { "Prompt Text", PromptTextColor },
        { "Highlight Text", PromptHighlightColor },
        { "Cursor", PromptCursorColor },
        { "Background", PromptBackgroundColor },
        { "Edit Background", PromptEditBackgColor },
        { "Focussed Background", PromptEditFocusBackgColor },
        { 0, 0 }
    };

    // Plot mark colors, electrical only.
    //
    clritem Menu3[] = {
        { "Plot Mark 1", Color2 },
        { "Plot Mark 2", Color3 },
        { "Plot Mark 3", Color4 },
        { "Plot Mark 4", Color5 },
        { "Plot Mark 5", Color6 },
        { "Plot Mark 6", Color7 },
        { "Plot Mark 7", Color8 },
        { "Plot Mark 8", Color9 },
        { "Plot Mark 9", Color10 },
        { "Plot Mark 10", Color11 },
        { "Plot Mark 11", Color12 },
        { "Plot Mark 12", Color13 },
        { "Plot Mark 13", Color14 },
        { "Plot Mark 14", Color15 },
        { "Plot Mark 15", Color16 },
        { "Plot Mark 17", Color17 },
        { "Plot Mark 17", Color18 },
        { "Plot Mark 18", Color19 },
        { 0, 0 }
    };

    namespace gtkcolor {
        struct sClr
        {
            enum { CATEG_ATTR, CATEG_PROMPT, CATEG_PLOT };

            sClr(GRobject);
            ~sClr();

            GtkWidget *shell() { return (c_shell); }

            void update();

        private:
            void fill_categ_menu();
            void fill_attr_menu(int);
            void update_color();
            void set_sample_bg();

            static void c_change_proc(GtkWidget*, void*);
            static void c_btn_proc(GtkWidget*, void*);
            static void c_mode_menu_proc(GtkWidget*, void*);
            static void c_categ_menu_proc(GtkWidget*, void*);
            static void c_attr_menu_proc(GtkWidget*, void*);
            static void c_cancel_proc(GtkWidget*, void*);
            static void c_apply_proc(GtkWidget*, void*);
            static void c_set_rgb(int, int, int);
            static void c_get_rgb(int, int*, int*, int*);
            static void c_list_callback(const char*, void*);
            static void c_list_btn_proc(GtkWidget*, void*);

            GRobject c_caller;
            GtkWidget *c_shell;
            GtkWidget *c_modemenu;
            GtkWidget *c_categmenu;
            GtkWidget *c_attrmenu;
            GtkWidget *c_sel;
            GtkWidget *c_listbtn;
            GtkWidget *c_sample;
            GRlistPopup *c_listpop;
            DisplayMode c_display_mode;
            DisplayMode c_ref_mode;

            int c_mode;
            int c_red;
            int c_green;
            int c_blue;
        };

        sClr *Clr;
    }
}

using namespace gtkcolor;


// Menu function to display, update, or destroy the color popup.
//
void
cMain::PopUpColor(GRobject caller, ShowMode mode)
{
    if (!GTKdev::exists() || !GTKmainwin::exists())
        return;
    if (mode == MODE_OFF) {
        delete Clr;
        return;
    }
    if (mode == MODE_UPD) {
        if (Clr)
            Clr->update();
        return;
    }
    if (Clr)
        return;

    new sClr(caller);
    if (!Clr->shell()) {
        delete Clr;
        return;
    }
    gtk_window_set_transient_for(GTK_WINDOW(Clr->shell()),
        GTK_WINDOW(GTKmainwin::self()->Shell()));

    GTKdev::self()->SetPopupLocation(GRloc(LW_LL), Clr->shell(),
        GTKmainwin::self()->Viewport());
    gtk_widget_show(Clr->shell());
}


// The following functions periodically alter the colormap to implement
// blinking layers and selection boundary.

namespace { int colortimer(void*); }

// Allocate a private color for the selection borders, and set the
// initial timer.
//
void
cMain::ColorTimerInit()
{
    if (!GTKdev::exists())
        return;
    int pixel = DSP()->Color(SelectColor1, Physical);
    int red, green, blue;
    GTKdev::self()->RGBofPixel(pixel, &red, &green, &blue);
    int sp = DSP()->SelectPixelPhys();
    GTKdev::self()->AllocateColor(&sp, red, green, blue);
    DSP()->SetSelectPixelPhys(sp);

    pixel = DSP()->Color(SelectColor1, Electrical);
    GTKdev::self()->RGBofPixel(pixel, &red, &green, &blue);
    sp = DSP()->SelectPixelElec();
    GTKdev::self()->AllocateColor(&sp, red, green, blue);
    DSP()->SetSelectPixelElec(sp);

    GTKdev::self()->AddTimer(500, colortimer, 0);
}


sClr::sClr(GRobject c)
{
    Clr = this;
    c_mode = CLR_CURLYR;
    c_caller = c;
    c_shell = 0;
    c_modemenu = 0;
    c_categmenu = 0;
    c_attrmenu = 0;
    c_sel = 0;
    c_listbtn = 0;
    c_sample = 0;
    c_listpop = 0;
    c_display_mode = DSP()->CurMode();
    c_ref_mode = DSP()->CurMode();
    c_red = 0;
    c_green = 0;
    c_blue = 0;

    if (!GTKmainwin::self())
        return;

    // In 256-color mode, the color selector's color display areas
    // are gibberish unless it is read-only.  We go through some
    // hoops here to replace the color wheel and sample areas with
    // our own sample area.
    bool fix256 = !GTKpkg::self()->IsTrueColor();

    c_shell = gtk_NewPopup(0, "Color Selection", c_cancel_proc, 0);
    if (!c_shell)
        return;
    gtk_window_set_resizable(GTK_WINDOW(c_shell), false);

    GtkWidget *form = gtk_table_new(1, 5, false);
    gtk_widget_show(form);
    gtk_container_add(GTK_CONTAINER(c_shell), form);

    GtkWidget *hbox = gtk_hbox_new(false, 2);
    gtk_widget_show(hbox);

    //
    // Physical/Electrical mode selector
    //
    GtkWidget *entry = gtk_combo_box_text_new();
    c_modemenu = entry;
    gtk_widget_set_name(entry, "ModeMenu");
    if (ScedIf()->hasSced())
        gtk_widget_show(entry);
    else
        gtk_widget_hide(entry);
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(entry), "Physical");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(entry), "Electrical");
    gtk_combo_box_set_active(GTK_COMBO_BOX(entry), c_display_mode);
    g_signal_connect(G_OBJECT(entry), "changed",
        G_CALLBACK(c_mode_menu_proc), this);
    gtk_box_pack_start(GTK_BOX(hbox), entry, false, false, 0);

    //
    // Categories menu
    //
    entry = gtk_combo_box_text_new();
    c_categmenu = entry;
    gtk_widget_set_name(entry, "CategMenu");
    gtk_widget_show(entry);
    gtk_box_pack_start(GTK_BOX(hbox), entry, false, false, 0);

    fill_categ_menu();

    g_signal_connect(G_OBJECT(entry), "changed",
        G_CALLBACK(c_categ_menu_proc), 0);

    //
    // Attribute selection menu
    //
    // For this one, we need to pass an index to the handler function. 
    // To do this, it doesn't seem possible to use GtkComboBoxText,
    // instead we implement the GtkComboBox interfaces and place the
    // callback data in a "hidden" column.  Perhaps there is an easier
    // way?

    // Thse GtkListStore thingy is actually a GtkTreeModel!
    GtkListStore *store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_INT);
    entry = gtk_combo_box_new_with_model(GTK_TREE_MODEL(store));

    // Pass off ownership to the congtainer.
    g_object_unref(G_OBJECT(store));

    // Now set up rendering for a single text column.  Without this
    // the menu will be blank.
    GtkCellRenderer *rnd = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(entry), rnd, true);
    gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(entry), rnd,
        "text", 0, (void*)0);

    // See the menu setup code for how to pass the integer, and the
    // handler code for how to get retrieve it.
    g_signal_connect(G_OBJECT(entry), "changed",
        G_CALLBACK(c_attr_menu_proc), 0);

    c_attrmenu = entry;
    gtk_widget_set_name(entry, "AttrMenu");
    gtk_widget_show(entry);
    gtk_box_pack_start(GTK_BOX(hbox), entry, false, false, 0);
    gtk_widget_set_size_request(entry, 180, -1);
    fill_attr_menu(CATEG_ATTR);

    //
    // Colors button
    //
    GtkWidget *button = gtk_toggle_button_new_with_label("Colors");
    c_listbtn = button;
    gtk_widget_set_name(button, "Colors");
    gtk_widget_show(button);
    gtk_box_pack_start(GTK_BOX(hbox), button, true, true, 0);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(c_list_btn_proc), 0);

    int rowcnt = 0;
    gtk_table_attach(GTK_TABLE(form), hbox, 0, 1, rowcnt, rowcnt + 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 0);
    rowcnt++;

    GtkWidget *sep = gtk_hseparator_new();
    gtk_widget_show(sep);
    gtk_table_attach(GTK_TABLE(form), sep, 0, 1, rowcnt, rowcnt + 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // Color Selection widget
    //
    c_sel = gtk_color_selection_new();
    gtk_widget_show(c_sel);

    gtk_color_selection_set_has_opacity_control(
        GTK_COLOR_SELECTION(c_sel), false);
    gtk_color_selection_set_has_palette(GTK_COLOR_SELECTION(c_sel),
        true);

    g_signal_connect(G_OBJECT(c_sel), "color-changed",
        G_CALLBACK(c_change_proc), 0);
    update_color();

    if (fix256) {
        GtkWidget *frame = gtk_frame_new(0);
        gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
        gtk_widget_show(frame);
        GtkWidget *da = gtk_drawing_area_new();
        c_sample = da;
        gtk_widget_show(da);
        gtk_container_add(GTK_CONTAINER(frame), da);
        gtk_widget_set_size_request(da, 150, -1);
        hbox = gtk_hbox_new(false, 2);
        gtk_widget_show(hbox);
        gtk_box_pack_start(GTK_BOX(hbox), frame, false, true, 2);
        gtk_box_pack_end(GTK_BOX(hbox), c_sel, false, true, 0);
        gtk_table_attach(GTK_TABLE(form), hbox, 0, 1, rowcnt, rowcnt + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 0);
    }
    else {
        gtk_table_attach(GTK_TABLE(form), c_sel, 0, 1, rowcnt, rowcnt + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 0);
    }
    rowcnt++;

    sep = gtk_hseparator_new();
    gtk_widget_show(sep);
    gtk_table_attach(GTK_TABLE(form), sep, 0, 1, rowcnt, rowcnt + 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // Apply and Dismiss buttons
    //
    hbox = gtk_hbox_new(false, 2);
    gtk_widget_show(hbox);

    button = gtk_button_new_with_label("Apply");
    gtk_widget_set_name(button, "Apply");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(c_apply_proc), c_shell);
    gtk_box_pack_start(GTK_BOX(hbox), button, false, false, 0);

    button = gtk_button_new_with_label("Help");
    gtk_widget_set_name(button, "Help");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(c_btn_proc), c_shell);
    gtk_box_pack_start(GTK_BOX(hbox), button, false, false, 0);

    button = gtk_button_new_with_label("Dismiss");
    gtk_widget_set_name(button, "Dismiss");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(c_cancel_proc), c_shell);
    gtk_box_pack_start(GTK_BOX(hbox), button, true, true, 0);

    gtk_table_attach(GTK_TABLE(form), hbox, 0, 1, rowcnt, rowcnt + 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    gtk_window_set_focus(GTK_WINDOW(c_shell), button);

    Clr->set_sample_bg();
}


sClr::~sClr()
{
    Clr = 0;
    if (c_caller)
        GTKdev::Deselect(c_caller);
    if (c_listpop)
        c_listpop->popdown();
    if (c_shell)
        gtk_widget_destroy(c_shell);
}


void
sClr::update()
{
    if (c_ref_mode != DSP()->CurMode()) {
        // User changed between Physical and Electrical modes. 
        // Rebuild the ATTR menu if current, since the visibility of
        // the Current Layer item will have changed.
        //
        if (gtk_combo_box_get_active(
                GTK_COMBO_BOX(Clr->c_categmenu)) == CATEG_ATTR) {
            fill_attr_menu(CATEG_ATTR);
        }
        c_ref_mode = DSP()->CurMode();
    }
    else {
        // The current layer changed.  The second term in the
        // conditional is redundant, for sanity.
        //
        if (c_mode == CLR_CURLYR && c_display_mode == DSP()->CurMode())
            update_color();
    }
}


// Update the color shown.
//
void
sClr::update_color()
{
    sClr *ct = this;
    if (ct && c_sel) {
        int r, g, b;
        c_get_rgb(c_mode, &r, &g, &b);
        GdkColor rgb;
        rgb.red = r << 8;
        rgb.green = g << 8;
        rgb.blue = b << 8;
        gtk_color_selection_set_current_color(GTK_COLOR_SELECTION(c_sel), &rgb);
        set_sample_bg();
    }
}


// Set the sample background to the current pixel, 256-color mode only.
//
void
sClr::set_sample_bg()
{
    GdkColor clr;
    gtk_color_selection_get_current_color(GTK_COLOR_SELECTION(c_sel), &clr);
    gtk_color_selection_set_previous_color(GTK_COLOR_SELECTION(c_sel), &clr);
    if (c_sample) {
        clr.pixel = 0;
        if (c_mode == CLR_CURLYR) {
            if (LT()->CurLayer())
                clr.pixel = dsp_prm(LT()->CurLayer())->pixel();
        }
        else
            clr.pixel = DSP()->Color(c_mode);
        gtk_widget_hide(c_sample);
        gdk_window_set_background(gtk_widget_get_window(c_sample), &clr);
        gtk_widget_show(c_sample);
    }
}


void
sClr::fill_categ_menu()
{
    if (Clr) {
        GtkListStore *store = GTK_LIST_STORE(
            gtk_combo_box_get_model(GTK_COMBO_BOX(c_categmenu)));
        gtk_list_store_clear(store);
        gtk_combo_box_text_append_text(
            GTK_COMBO_BOX_TEXT(c_categmenu), "Attributes");
        gtk_combo_box_text_append_text(
            GTK_COMBO_BOX_TEXT(c_categmenu), "Prompt Line");
        if (c_display_mode == Electrical) {
            gtk_combo_box_text_append_text(
                GTK_COMBO_BOX_TEXT(c_categmenu), "Plot Marks");
        }
        gtk_combo_box_set_active(GTK_COMBO_BOX(Clr->c_categmenu), 0);
    }
}


void
sClr::fill_attr_menu(int categ)
{
    if (!c_attrmenu)
        return;
    if (categ == CATEG_ATTR) {
        GtkListStore *store = GTK_LIST_STORE(
            gtk_combo_box_get_model(GTK_COMBO_BOX(c_attrmenu)));
        gtk_list_store_clear(store);

        GtkTreeIter iter;
        for (clritem *c = Menu1; c->descr; c++) {
            // We want the Current Layer entry only when the mode
            // matches the display mode, since the current layer
            // of the "opposite" mode is not known here.
            //
            if (c == Menu1 && c_display_mode != DSP()->CurMode())
                continue;

            gtk_list_store_append(store, &iter);
            gtk_list_store_set(store, &iter, 0, c->descr,
                1, c->tab_indx, -1);
        }
        if (c_display_mode == DSP()->CurMode())
            c_mode = Menu1[0].tab_indx;
        else
            c_mode = Menu1[1].tab_indx;
    }
    else if (categ == CATEG_PROMPT) {
        GtkListStore *store = GTK_LIST_STORE(
            gtk_combo_box_get_model(GTK_COMBO_BOX(c_attrmenu)));
        gtk_list_store_clear(store);

        GtkTreeIter iter;
        for (clritem *c = Menu2; c->descr; c++) {
            gtk_list_store_append(store, &iter);
            gtk_list_store_set(store, &iter, 0, c->descr,
                1, c->tab_indx, -1);
        }
        c_mode = Menu2[0].tab_indx;
    }
    else if (categ == CATEG_PLOT) {
        GtkListStore *store = GTK_LIST_STORE(
            gtk_combo_box_get_model(GTK_COMBO_BOX(c_attrmenu)));
        gtk_list_store_clear(store);

        GtkTreeIter iter;
        for (clritem *c = Menu3; c->descr; c++) {
            gtk_list_store_append(store, &iter);
            gtk_list_store_set(store, &iter, 0, c->descr,
                1, c->tab_indx, -1);
        }
        c_mode = Menu3[0].tab_indx;
    }
    else
        return;
    gtk_combo_box_set_active(GTK_COMBO_BOX(c_attrmenu), 0);
    update_color();
}


// Static function.
void
sClr::c_change_proc(GtkWidget*, void*)
{
    if (Clr) {
        GdkColor rgb;
        gtk_color_selection_get_current_color(
            GTK_COLOR_SELECTION(Clr->c_sel), &rgb);
        int r = (unsigned int)rgb.red >> 8;
        int g = (unsigned int)rgb.green >> 8;
        int b = (unsigned int)rgb.blue >> 8;
        c_set_rgb(r, g, b);
    }
}


// Static function.
void
sClr::c_btn_proc(GtkWidget *widget, void*)
{
    if (!Clr)
        return;
    if (widget) {
        const char *name = gtk_widget_get_name(widget);
        if (name && !strcmp(name, "Help")) {
            DSPmainWbag(PopUpHelp("xic:color"))
            return;
        }
    }
}


// Static function.
void
sClr::c_mode_menu_proc(GtkWidget*, void*)
{
    if (!Clr)
        return;
    Clr->c_display_mode = gtk_combo_box_get_active(
        GTK_COMBO_BOX(Clr->c_modemenu)) ? Electrical : Physical;
    Clr->fill_categ_menu();
    Clr->fill_attr_menu(CATEG_ATTR);
}


// Static function.
void
sClr::c_categ_menu_proc(GtkWidget*, void*)
{
    if (!Clr)
        return;

    Clr->fill_attr_menu(gtk_combo_box_get_active(
        GTK_COMBO_BOX(Clr->c_categmenu)));
}


// Static function.
void
sClr::c_attr_menu_proc(GtkWidget*, void *)
{
    if (Clr) {
        GtkTreeIter iter;
        if (!gtk_combo_box_get_active_iter(GTK_COMBO_BOX(Clr->c_attrmenu), &iter))
            return;
        GtkListStore *store = GTK_LIST_STORE(
            gtk_combo_box_get_model(GTK_COMBO_BOX(Clr->c_attrmenu)));
        GValue gv = G_VALUE_INIT;
        gtk_tree_model_get_value(GTK_TREE_MODEL(store), &iter, 1, &gv);
        if (g_value_get_int(&gv) >= 0) {
            Clr->c_mode = g_value_get_int(&gv);;
            Clr->update_color();
        }
        g_value_unset(&gv);
    }
}


// Static function.
void
sClr::c_cancel_proc(GtkWidget*, void*)
{
    XM()->PopUpColor(0, MODE_OFF);
}


// Static function
// Actually perform the color change.
//
void
sClr::c_apply_proc(GtkWidget*, void*)
{
    if (!Clr)
        return;
    int mode = Clr->c_mode;
    int red = Clr->c_red;
    int green = Clr->c_green;
    int blue = Clr->c_blue;
    DisplayMode dm = Clr->c_display_mode;

    if (mode == CLR_CURLYR) {
        if (LT()->CurLayer() && dm == DSP()->CurMode()) {
            LT()->SetLayerColor(LT()->CurLayer(), red, green, blue);
            LT()->ShowLayerTable(LT()->CurLayer());
            XM()->PopUpFillEditor(0, MODE_UPD);
            XM()->PopUpLayerPalette(0, MODE_UPD, false, 0);

            if (DSP()->CurMode() == Electrical || !LT()->NoPhysRedraw()) {
                DSP()->RedisplayAll();
            }
        }
        return;
    }
    if (mode == BackgroundColor) {
        DSP()->ColorTab()->set_rgb(mode, dm, red, green, blue);
        if (dm == DSP()->CurMode()) {
            LT()->ShowLayerTable(LT()->CurLayer());
            XM()->PopUpFillEditor(0, MODE_UPD);
            XM()->PopUpLayerPalette(0, MODE_UPD, false, 0);
        }
        XM()->UpdateCursor(0, XM()->GetCursor());
        if (dm == Electrical || !LT()->NoPhysRedraw()) {
            DSP()->RedisplayAll(dm);
        }
        return;
    }

    DSP()->ColorTab()->set_rgb(mode, dm, red, green, blue);
    switch (mode) {
    case PromptEditTextColor:
    case PromptTextColor:
    case PromptHighlightColor:
    case PromptCursorColor:
    case PromptBackgroundColor:
    case PromptEditBackgColor:
        Param()->redraw();
        Coord()->redraw();
        DSP()->MainWbag()->ShowKeys();
        gtkEdit()->redraw();
        break;

    case CoarseGridColor:
    case FineGridColor:
        if (dm == Electrical || !LT()->NoPhysRedraw()) {
            DSP()->RedisplayAll(dm);
        }
        break;

    case GhostColor:
        XM()->UpdateCursor(0, XM()->GetCursor());
        break;

    case HighlightingColor:
        if (dm == Electrical || !LT()->NoPhysRedraw()) {
            DSP()->RedisplayAll(dm);
        }
        break;

    case SelectColor1:
    case SelectColor2:
        break;

    case MarkerColor:
    case InstanceBBColor:
    case InstanceNameColor:
    case InstanceSizeColor:
        if (dm == Electrical || !LT()->NoPhysRedraw()) {
            DSP()->RedisplayAll(dm);
        }
        break;

    default:
        break;
    }
}


// Static function.
// Return the RGB for the current mode.
//
void
sClr::c_get_rgb(int mode, int *red, int *green, int *blue)
{
    if (mode == CLR_CURLYR) {
        if (LT()->CurLayer())
            LT()->GetLayerColor(LT()->CurLayer(), red, green, blue);
        else {
            *red = 0;
            *green = 0;
            *blue = 0;
        }
    }
    else {
        if (Clr) {
            DSP()->ColorTab()->get_rgb(mode, Clr->c_display_mode,
                red, green, blue);
        }
        else {
            *red = 0;
            *green = 0;
            *blue = 0;
        }
    }
}


// Static function.
// This used to actually effect color change, now we defer to the
// Apply button.
//
void
sClr::c_set_rgb(int red, int green, int blue)
{
    if (!Clr)
        return;
    Clr->c_red = red;
    Clr->c_green = green;
    Clr->c_blue = blue;
}


//
// Pop-up selectable list of rgb.txt color entries.
//


// Insert the selected rgb.txt entry into the color selector.
//
void
sClr::c_list_callback(const char *string, void*)
{
    if (string) {
        while (isspace(*string))
            string++;
        int r, g, b;
        if (sscanf(string, "%d %d %d", &r, &g, &b) != 3)
            return;
        if (Clr) {
            GdkColor rgb;
            rgb.red = r << 8;
            rgb.green = g << 8;
            rgb.blue = b << 8;
            gtk_color_selection_set_current_color(
                GTK_COLOR_SELECTION(Clr->c_sel), &rgb);
            sClr::c_set_rgb(r, g, b);
        }
    }
    else if (Clr) {
        GTKdev::SetStatus(Clr->c_listbtn, false);
        Clr->c_listpop = 0;
    }
}


// Static function.
// Pop up a list of colors from the rgb.txt file.
//
void
sClr::c_list_btn_proc(GtkWidget *btn, void*)
{
    if (Clr) {
        bool state = GTKdev::GetStatus(btn);
        if (!Clr->c_listpop && state) {
            stringlist *list = GRcolorList::listColors();
            if (!list) {
                GTKdev::SetStatus(btn, false);
                return;
            }
            Clr->c_listpop = DSPmainWbagRet(PopUpList(list, "Colors",
                "click to select", c_list_callback, 0, false, false));
            stringlist::destroy(list);
            if (Clr->c_listpop)
                Clr->c_listpop->register_usrptr((void**)&Clr->c_listpop);
        }
        else if (Clr->c_listpop && !state)
            Clr->c_listpop->popdown();
    }
}
// End of color selection popup functions.


namespace {
    int idle_id;

    // Idle function to redraw highlighting.
    //
    int
    idlefunc(void*)
    {
        static int on;
        if (!GTKpkg::self()->IsBusy()) {
            if (GTKpkg::self()->IsTrueColor()) {
                WindowDesc *wd;
                WDgen wgen(WDgen::MAIN, WDgen::ALL);
                while ((wd = wgen.next()) != 0) {
                    if (!on) {
                        DSP()->SetSelectPixelPhys(
                            DSP()->Color(SelectColor1, Physical));
                        DSP()->SetSelectPixelElec(
                            DSP()->Color(SelectColor1, Electrical));
                    }
                    else {
                        DSP()->SetSelectPixelPhys(
                            DSP()->Color(SelectColor2, Physical));
                        DSP()->SetSelectPixelElec(
                            DSP()->Color(SelectColor2, Electrical));
                    }

                    if (wd->DbType() == WDcddb) {
                        if (Selections.blinking())
                            Selections.show(wd);
                    }
                    wd->ShowHighlighting();
                }
            }
            else {
                GdkColor colorcell;
                if (on)
                    colorcell.pixel = DSP()->Color(SelectColor1);
                else
                    colorcell.pixel = DSP()->Color(SelectColor2);

                int red, green, blue;
                GTKdev::self()->RGBofPixel(colorcell.pixel,
                    &red, &green, &blue);
                colorcell.red = red << 8;
                colorcell.green = green << 8;
                colorcell.blue = blue << 8;

                colorcell.pixel = DSP()->SelectPixel();

                CDl *ld;
                CDlgen lgen(DSP()->CurMode());
                while ((ld = lgen.next()) != 0) {
                    if (ld->isBlink()) {
                        DspLayerParams *lp = dsp_prm(ld);
                        colorcell.pixel = lp->pixel();
                        if (on) {
                            colorcell.red = 256 * lp->red();
                            colorcell.green = 256 * lp->green();
                            colorcell.blue = 256 * lp->blue();
                        }
                        else {
                            colorcell.red = 192 * lp->red();
                            colorcell.green = 192 * lp->green();
                            colorcell.blue = 192 * lp->blue();
                        }
                    }
                }
            }
        }
        on ^= true;
        idle_id = 0;
        return (false);
    }


    // Timer callback.  This self-regenerates the timing interval, and
    // switches the colors of all flashing layers and the selection
    // boundary.
    //
    int
    colortimer(void*)
    {
        if (!idle_id)
            idle_id = g_idle_add(idlefunc, 0);
        return (true);
    }
}



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
 $Id: gtkcolor.cc,v 5.134 2016/02/05 03:50:35 stevew Exp $
 *========================================================================*/

#include "main.h"
#include "cd_lgen.h"
#include "dsp_layer.h"
#include "dsp_color.h"
#include "dsp_inlines.h"
#include "gtkmain.h"
#include "gtkfont.h"
#include "gtkparam.h"
#include "gtkcoord.h"
#include "gtkhtext.h"
#include "gtkinlines.h"
#include "menu.h"
#include "layertab.h"
#include "select.h"
#include "pathlist.h"


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
        { "Plot Mark 2", Color2 },
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
            GtkWidget *c_pmmi;
            GtkWidget *c_entry;
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
    if (!GRX || !mainBag())
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
        GTK_WINDOW(mainBag()->Shell()));

    GRX->SetPopupLocation(GRloc(LW_LL), Clr->shell(),
        mainBag()->Viewport());
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
    if (!GRX)
        return;
    int pixel = DSP()->Color(SelectColor1);
    int red, green, blue;
    GRX->RGBofPixel(pixel, &red, &green, &blue);
    int sp = DSP()->SelectPixel();
    GRX->AllocateColor(&sp, red, green, blue);
    DSP()->SetSelectPixel(sp);
    GRX->AddTimer(500, colortimer, 0);
}


sClr::sClr(GRobject c)
{
    Clr = this;
    c_mode = CLR_CURLYR;
    c_caller = c;
    c_shell = 0;
    c_modemenu = 0;
    c_categmenu = 0;
    c_pmmi = 0;
    c_entry = 0;
    c_sel = 0;
    c_listbtn = 0;
    c_sample = 0;
    c_listpop = 0;
    c_display_mode = DSP()->CurMode();
    c_ref_mode = DSP()->CurMode();
    c_red = 0;
    c_green = 0;
    c_blue = 0;

    if (!mainBag())
        return;

    // In 256-color mode, the color selector's color display areas
    // are gibberish unless it is read-only.  We go through some
    // hoops here to replace the color wheel and sample areas with
    // our own sample area.
    bool fix256 = !dspPkgIf()->IsTrueColor();

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
    GtkWidget *entry = gtk_option_menu_new();
    c_modemenu = entry;
    gtk_widget_set_name(entry, "ModeMenu");
    gtk_widget_show(entry);
    gtk_box_pack_start(GTK_BOX(hbox), entry, false, false, 0);
    {
        GtkWidget *menu = gtk_menu_new();
        gtk_widget_set_name(menu, "modemenu");

        GtkWidget *mi = gtk_menu_item_new_with_label("Physical");
        gtk_widget_set_name(mi, "ph");
        gtk_widget_show(mi);
        gtk_menu_append(GTK_MENU(menu), mi);
        gtk_signal_connect(GTK_OBJECT(mi), "activate",
            GTK_SIGNAL_FUNC(c_mode_menu_proc), (void*)(long)Physical);

        mi = gtk_menu_item_new_with_label("Electrical");
        gtk_widget_set_name(mi, "ph");
        gtk_widget_show(mi);
        gtk_menu_append(GTK_MENU(menu), mi);
        gtk_signal_connect(GTK_OBJECT(mi), "activate",
            GTK_SIGNAL_FUNC(c_mode_menu_proc), (void*)(long)Electrical);

        gtk_option_menu_set_menu(GTK_OPTION_MENU(c_modemenu), menu);
        gtk_option_menu_set_history(GTK_OPTION_MENU(c_modemenu),
            c_display_mode);
    }

    //
    // Categories menu
    //
    entry = gtk_option_menu_new();
    c_categmenu = entry;
    gtk_widget_set_name(entry, "CategMenu");
    gtk_widget_show(entry);
    gtk_box_pack_start(GTK_BOX(hbox), entry, false, false, 0);
    {
        GtkWidget *menu = gtk_menu_new();
        gtk_widget_set_name(menu, "CMenu");

        GtkWidget *mi = gtk_menu_item_new_with_label("Attributes");
        gtk_widget_set_name(mi, "at");
        gtk_widget_show(mi);
        gtk_menu_append(GTK_MENU(menu), mi);
        gtk_signal_connect(GTK_OBJECT(mi), "activate",
            GTK_SIGNAL_FUNC(c_categ_menu_proc),
            (void*)(long)CATEG_ATTR);

        mi = gtk_menu_item_new_with_label("Prompt Line");
        gtk_widget_set_name(mi, "pl");
        gtk_widget_show(mi);
        gtk_menu_append(GTK_MENU(menu), mi);
        gtk_signal_connect(GTK_OBJECT(mi), "activate",
            GTK_SIGNAL_FUNC(c_categ_menu_proc),
            (void*)(long)CATEG_PROMPT);

        mi = gtk_menu_item_new_with_label("Plot Marks");
        c_pmmi = mi;
        gtk_widget_set_name(mi, "pm");
        if (c_display_mode == Electrical)
            gtk_widget_show(mi);
        else
            gtk_widget_hide(mi);
        gtk_menu_append(GTK_MENU(menu), mi);
        gtk_signal_connect(GTK_OBJECT(mi), "activate",
            GTK_SIGNAL_FUNC(c_categ_menu_proc),
            (void*)(long)CATEG_PLOT);

        gtk_option_menu_set_menu(GTK_OPTION_MENU(c_categmenu), menu);
        gtk_option_menu_set_history(GTK_OPTION_MENU(c_categmenu), 0);
    }

    //
    // Attribute selection menu
    //
    entry = gtk_option_menu_new();
    c_entry = entry;
    gtk_widget_set_name(entry, "AttrMenu");
    gtk_widget_show(entry);
    gtk_box_pack_start(GTK_BOX(hbox), entry, false, false, 0);
    gtk_widget_set_usize(entry, 180, -1);
    c_categ_menu_proc(0, (void*)(long)CATEG_ATTR);

    //
    // Colors button
    //
    GtkWidget *button = gtk_toggle_button_new_with_label("Colors");
    c_listbtn = button;
    gtk_widget_set_name(button, "Colors");
    gtk_widget_show(button);
    gtk_box_pack_start(GTK_BOX(hbox), button, true, true, 0);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(c_list_btn_proc), 0);

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

    gtk_signal_connect(GTK_OBJECT(c_sel), "color-changed",
        GTK_SIGNAL_FUNC(c_change_proc), 0);
    update_color();

    if (fix256) {
        GtkWidget *frame = gtk_frame_new(0);
        gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
        gtk_widget_show(frame);
        GtkWidget *da = gtk_drawing_area_new();
        c_sample = da;
        gtk_widget_show(da);
        gtk_container_add(GTK_CONTAINER(frame), da);
        gtk_drawing_area_size(GTK_DRAWING_AREA(da), 150, -1);
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
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(c_apply_proc), c_shell);
    gtk_box_pack_start(GTK_BOX(hbox), button, false, false, 0);

    button = gtk_button_new_with_label("Help");
    gtk_widget_set_name(button, "Help");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(c_btn_proc), c_shell);
    gtk_box_pack_start(GTK_BOX(hbox), button, false, false, 0);

    button = gtk_button_new_with_label("Dismiss");
    gtk_widget_set_name(button, "Dismiss");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(c_cancel_proc), c_shell);
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
        GRX->Deselect(c_caller);
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
        if (gtk_option_menu_get_history(
                GTK_OPTION_MENU(Clr->c_categmenu)) == CATEG_ATTR) {
            sClr::c_categ_menu_proc(0, (void*)CATEG_ATTR);
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
        double rgb[4];
        rgb[0] = r/255.0;
        rgb[1] = g/255.0;
        rgb[2] = b/255.0;
        rgb[3] = 0.0;
        gtk_color_selection_set_color(GTK_COLOR_SELECTION(c_sel), rgb);
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
        gdk_window_set_background(c_sample->window, &clr);
        gtk_widget_show(c_sample);
    }
}


// Static function.
void
sClr::c_change_proc(GtkWidget*, void*)
{
    if (Clr) {
        double rgb[4];
        gtk_color_selection_get_color(GTK_COLOR_SELECTION(Clr->c_sel), rgb);
        int r = 0xff & (int)(rgb[0] * 255);
        int g = 0xff & (int)(rgb[1] * 255);
        int b = 0xff & (int)(rgb[2] * 255);
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
sClr::c_mode_menu_proc(GtkWidget*, void *arg)
{
    if (!Clr)
        return;
    bool atupd = false;
    Clr->c_display_mode = arg ? Electrical : Physical;
    if (Clr->c_display_mode == Electrical)
        gtk_widget_show(Clr->c_pmmi);
    else {
        // The plot marks are available when showing Electrical only.
        if (gtk_option_menu_get_history(
                GTK_OPTION_MENU(Clr->c_categmenu)) == CATEG_PLOT) {
            sClr::c_categ_menu_proc(0, (void*)CATEG_ATTR);
            atupd = true;
        }
        gtk_widget_hide(Clr->c_pmmi);
    }

    // Rebuild the ATTR menu if current, since the visibility of the
    // Current Layer item will have changed.  Don't need to call this
    // if already done above.
    //
    if (!atupd && gtk_option_menu_get_history(
            GTK_OPTION_MENU(Clr->c_categmenu)) == CATEG_ATTR) {
        sClr::c_categ_menu_proc(0, (void*)CATEG_ATTR);
    }
    Clr->update_color();
}


// Static function.
void
sClr::c_categ_menu_proc(GtkWidget*, void *arg)
{
    if (!Clr)
        return;
    switch ((long)arg) {
    case CATEG_ATTR:
        {
            GtkWidget *menu = gtk_menu_new();
            gtk_widget_set_name(menu, "Menu");
            for (clritem *c = Menu1; c->descr; c++) {
                // We want the Current Layer entry only when the mode
                // matches the display mode, since the current layer
                // of the "opposite" mode is not known here.
                //
                if (c == Menu1 && Clr->c_display_mode != DSP()->CurMode())
                    continue;
                GtkWidget *mi = gtk_menu_item_new_with_label(c->descr);
                gtk_widget_set_name(mi, c->descr);
                gtk_widget_show(mi);
                gtk_menu_append(GTK_MENU(menu), mi);
                gtk_signal_connect(GTK_OBJECT(mi), "activate",
                    GTK_SIGNAL_FUNC(sClr::c_attr_menu_proc),
                    (void*)(long)c->tab_indx);
            }
            if (Clr->c_display_mode == DSP()->CurMode())
                Clr->c_mode = Menu1[0].tab_indx;
            else
                Clr->c_mode = Menu1[1].tab_indx;
            gtk_option_menu_set_menu(GTK_OPTION_MENU(Clr->c_entry), menu);
            gtk_option_menu_set_history(GTK_OPTION_MENU(Clr->c_entry), 0);
            Clr->update_color();
        }
        break;
    case CATEG_PROMPT:
        {
            GtkWidget *menu = gtk_menu_new();
            gtk_widget_set_name(menu, "Menu");
            for (clritem *c = Menu2; c->descr; c++) {
                GtkWidget *mi = gtk_menu_item_new_with_label(c->descr);
                gtk_widget_set_name(mi, c->descr);
                gtk_widget_show(mi);
                gtk_menu_append(GTK_MENU(menu), mi);
                gtk_signal_connect(GTK_OBJECT(mi), "activate",
                    GTK_SIGNAL_FUNC(sClr::c_attr_menu_proc),
                    (void*)(long)c->tab_indx);
            }
            Clr->c_mode = Menu2[0].tab_indx;
            gtk_option_menu_set_menu(GTK_OPTION_MENU(Clr->c_entry), menu);
            gtk_option_menu_set_history(GTK_OPTION_MENU(Clr->c_entry), 0);
            Clr->update_color();
        }
        break;
    case CATEG_PLOT:
        {
            GtkWidget *menu = gtk_menu_new();
            gtk_widget_set_name(menu, "Menu");
            for (clritem *c = Menu3; c->descr; c++) {
                GtkWidget *mi = gtk_menu_item_new_with_label(c->descr);
                gtk_widget_set_name(mi, c->descr);
                gtk_widget_show(mi);
                gtk_menu_append(GTK_MENU(menu), mi);
                gtk_signal_connect(GTK_OBJECT(mi), "activate",
                    GTK_SIGNAL_FUNC(sClr::c_attr_menu_proc),
                    (void*)(long)c->tab_indx);
            }
            Clr->c_mode = Menu3[0].tab_indx;
            gtk_option_menu_set_menu(GTK_OPTION_MENU(Clr->c_entry), menu);
            gtk_option_menu_set_history(GTK_OPTION_MENU(Clr->c_entry), 0);
            Clr->update_color();
        }
        break;
    }
}


// Static function.
void
sClr::c_attr_menu_proc(GtkWidget*, void *arg)
{
    if (Clr) {
        Clr->c_mode = (long)arg;
        Clr->update_color();
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
            double rgb[4];
            rgb[0] = (double)r/255.0;
            rgb[1] = (double)g/255.0;
            rgb[2] = (double)b/255.0;
            rgb[3] = 0.0;
            gtk_color_selection_set_color(
                GTK_COLOR_SELECTION(Clr->c_sel), rgb);
            sClr::c_set_rgb(r, g, b);
        }
    }
    else if (Clr) {
        GRX->SetStatus(Clr->c_listbtn, false);
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
        bool state = GRX->GetStatus(btn);
        if (!Clr->c_listpop && state) {
            stringlist *list = GRcolorList::listColors();
            if (!list) {
                GRX->SetStatus(btn, false);
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
        if (!dspPkgIf()->IsBusy()) {
            if (dspPkgIf()->IsTrueColor()) {
                WindowDesc *wd;
                WDgen wgen(WDgen::MAIN, WDgen::ALL);
                while ((wd = wgen.next()) != 0) {
                    if (!on)
                        DSP()->SetSelectPixel(
                            DSP()->Color(SelectColor1, wd->Mode()));
                    else
                        DSP()->SetSelectPixel(
                            DSP()->Color(SelectColor2, wd->Mode()));

                    if (wd->DbType() == WDcddb) {
                        if (Selections.blinking())
                            Selections.show(wd);
                    }
                    wd->ShowHighlighting();
                }
                DSP()->SetSelectPixel(DSP()->Color(SelectColor1));
            }
            else {
                GdkColor colorcell;
                if (on)
                    colorcell.pixel = DSP()->Color(SelectColor1);
                else
                    colorcell.pixel = DSP()->Color(SelectColor2);

                int red, green, blue;
                GRX->RGBofPixel(colorcell.pixel, &red, &green, &blue);
                colorcell.red = red << 8;
                colorcell.green = green << 8;
                colorcell.blue = blue << 8;

                colorcell.pixel = DSP()->SelectPixel();
                gdk_color_change(GRX->Colormap(), &colorcell);

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
                        gdk_color_change(GRX->Colormap(), &colorcell);
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
            idle_id = gtk_idle_add(idlefunc, 0);
        return (true);
    }
}


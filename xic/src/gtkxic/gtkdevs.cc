
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
#include "edit.h"
#include "cd_lgen.h"
#include "dsp_layer.h"
#include "dsp_color.h"
#include "dsp_inlines.h"
#include "fio.h"
#include "fio_library.h"
#include "events.h"
#include "menu.h"
#include "ebtn_menu.h"
#include "errorlog.h"
#include "gtkmain.h"
#include "gtkinlines.h"
#include "gtkinterf/gtkfont.h"


// This implements a menu of devices from the device library, in two
// styles.  The drop-down menu sytle takes less screen space and is
// the default.  The graphical palette might be more useful to
// beginners.
//
// Help system keywords used:
//  dev:xxxxx   (device name)

// This is the name of a dummy cell that if found in the device library
// will enable mutual inductors.
#define MUT_DUMMY "mut"

namespace {
    // XPM
    const char * more_xpm[] = {
    "32 25 3 1",
    " 	c none",
    ".	c black",
    "x	c blue",
    "                                ",
    "               .                ",
    "               ..               ",
    "               .x.              ",
    "               .xx.             ",
    "               .xxx.            ",
    "               .xxxx.           ",
    "               .xxxxx.          ",
    "      ..........xxxxxx.         ",
    "      .xxxxxxxxxxxxxxxx.        ",
    "      .xxxxxxxxxxxxxxxxx.       ",
    "      .xxxxxxxxxxxxxxxxxx.      ",
    "      .xxxxxxxxxxxxxxxxxxx.     ",
    "      .xxxxxxxxxxxxxxxxxx.      ",
    "      .xxxxxxxxxxxxxxxxx.       ",
    "      .xxxxxxxxxxxxxxxx.        ",
    "      ..........xxxxxx.         ",
    "               .xxxxx.          ",
    "               .xxxx.           ",
    "               .xxx.            ",
    "               .xx.             ",
    "               .x.              ",
    "               ..               ",
    "               .                ",
    "                                "};

    // XPM
    const char * dd_xpm[] = {
    "32 25 3 1",
    " 	c none",
    ".	c black",
    "x	c sienna",
    "                                ",
    "   ..........................   ",
    "   .          .         .   .   ",
    "   .          . ....... .   .   ",
    "   ............         .....   ",
    "              .         .       ",
    "              .  xx  xx .       ",
    "              . x xx  x .       ",
    "              .         .       ",
    "              .         .       ",
    "              . x xx xx .       ",
    "              .  xx xx  .       ",
    "              .         .       ",
    "              .         .       ",
    "              .  xx x x .       ",
    "              . xx x x  .       ",
    "              .         .       ",
    "              .         .       ",
    "              . x xx x  .       ",
    "              .  xx x x .       ",
    "              .         .       ",
    "              ...........       ",
    "                                ",
    "                                ",
    "                                "};

    // XPM
    const char * dda_xpm[] = {
    "32 25 3 1",
    " 	c none",
    ".	c black",
    "x	c sienna",
    "                                ",
    "   ..........................   ",
    "   .          .         .   .   ",
    "   .          . ....... .   .   ",
    "   ............         .....   ",
    "              .         .       ",
    "              .  xx  xx .       ",
    "              . x xx  x .       ",
    "              .         .       ",
    "              .         .       ",
    "              . x xx xx .       ",
    "      .       .  xx xx  .       ",
    "     ...      .         .       ",
    "     . .      .         .       ",
    "    .. ..     .  xx x x .       ",
    "    .....     . xx x x  .       ",
    "   ..   ..    .         .       ",
    "   ..   ..    .         .       ",
    "  ...   ...   . x xx x  .       ",
    "              .  xx x x .       ",
    "              .         .       ",
    "              ...........       ",
    "                                ",
    "                                ",
    "                                "};

    // XPM
    const char * pict_xpm[] = {
    "32 24 3 1",
    " 	c none",
    ".	c black",
    "x	c sienna",
    "                                ",
    "                                ",
    "   ..........................   ",
    "   xxxxxxxxxxxxxxxxxxxxxxxxxx   ",
    "       xx             xx        ",
    "       xx          .  xx        ",
    "       xx          .  xx        ",
    "       xx          .  xx        ",
    "       xx      .....  xx        ",
    "       xx    . ..     xx        ",
    "       xx    . ..     xx        ",
    "       xx .... ..     xx        ",
    "       xx    . ..     xx        ",
    "       xx    . ..     xx        ",
    "       xx      .....  xx        ",
    "       xx          .  xx        ",
    "       xx          .  xx        ",
    "       xx          .  xx        ",
    "       xx             xx        ",
    "   xxxxxxxxxxxxxxxxxxxxxxxxxx   ",
    "   ..........................   ",
    "                                ",
    "                                ",
    "                                "};
}

// Characteristic device size (window space)
#define DEVSIZE (11*CDelecResolution)

// Positional parameters (viewport space)
#define CELL_SIZE 60
#define SPA 6

namespace {
    // Device list element
    //
    struct sEnt
    {
        sEnt() { name = 0; x = 0; width = 0; }
        ~sEnt() { delete [] name; }

        char *name;
        int x;
        int width;
    };

    namespace gtkdevs {
        enum dvType { dvMenuCateg, dvMenuAlpha, dvMenuPict };

        struct sDv : public GTKbag, public GTKdraw
        {
            sDv(GRobject, stringlist*);
            ~sDv();

            void activate(bool);
            void esc();

            bool is_active()        { return (dv_active); }

            GRobject get_caller()
                {
                    GRobject tc = dv_caller;
                    dv_caller = 0;
                    return (tc);
                }

        private:
            int init_sizes();
            void render_cell(int, bool);
            void cyclemore();
            int  whichent(int);
            void show_selected(int);
            void show_unselected(int);

            static void dv_cancel_proc(GtkWidget*, void*);
            static void dv_more_proc(GtkWidget*, void*);
            static void dv_switch_proc(GtkWidget*, void*);
            static void dv_menu_proc(GtkWidget*, void*);
            static bool dv_comp_func(const char*, const char*);

            static int dv_redraw_idle(void*);
            static int dv_redraw_hdlr(GtkWidget*, GdkEvent*, void*);
            static int dv_btn_hdlr(GtkWidget*, GdkEvent*, void*);
            static int dv_motion_hdlr(GtkWidget*, GdkEvent*, void*);
            static int dv_enter_hdlr(GtkWidget*, GdkEvent*, void*);
            static int dv_leave_hdlr(GtkWidget*, GdkEvent*, void*);
            static void dv_font_change_hdlr(GtkWidget*, void*, void*);

            GRobject dv_caller;
            GtkWidget *dv_morebtn;
            sEnt *dv_entries;
            int dv_numdevs;
            int dv_leftindx;
            int dv_leftofst;
            int dv_rightindx;
            int dv_curdev;
            int dv_pressed;
            int dv_px, dv_py;
            unsigned int dv_foreg, dv_backg, dv_hlite, dv_selec;
            bool dv_active;
            dvType dv_type;
        };

        sDv *Dv;
    }
}

using namespace gtkdevs;


// The Device Menu popup control function
//
void
cSced::PopUpDevs(GRobject caller, ShowMode mode)
{
    if (!GRX || !mainBag())
        return;
    if (mode == MODE_ON) {
        if (Dv) {
            Dv->activate(true);
            return;
        }
    }
    else if (mode == MODE_OFF) {
        if (Dv && Dv->Shell() && !gtk_widget_get_window(Dv->Shell())) {
            // "delete window" received
            delete Dv;
        }
        else if (Dv)
            Dv->activate(false);
        SCD()->setShowDevs(false);
        return;
    }
    else if (mode == MODE_UPD) {
        if (!Dv)
            return;
        bool wasactive = Dv->is_active();
        caller = Dv->get_caller();
        delete Dv;
        if (!wasactive)
            return;
    }

    stringlist *wl = FIO()->GetLibNamelist(XM()->DeviceLibName(), LIBdevice);
    if (!wl) {
        Log()->ErrorLog(mh::Initialization,
            "No device library found.\n"
            "You must exit and properly install the device.lib file,\n"
            "or set XIC_LIB_PATH in the environment.");
        return;
    }

    Dv = new sDv(caller, wl);
    if (!Dv->Shell()) {
        delete Dv;
        return;
    }
    gtk_window_set_transient_for(GTK_WINDOW(Dv->Shell()),
        GTK_WINDOW(mainBag()->Shell()));

    GRX->SetPopupLocation(GRloc(LW_UL), Dv->Shell(), mainBag()->Viewport());
    gtk_widget_show(Dv->Shell());

    // GTK-2.20.1 needs this, the first call is ignored for some reason,
    // for the pictorial menu only.
    GRX->SetPopupLocation(GRloc(LW_UL), Dv->Shell(), mainBag()->Viewport());

    if (Dv->Viewport())
#ifdef NEW_NDK
        Dv->GetDrawable()->set_window(gtk_widget_get_window(Dv->Viewport()));
#else
        Dv->SetWindow(gtk_widget_get_window(Dv->Viewport()));
#endif
}


// Reset the currently selected device colors in the toolbar.
// Called on exit from XM()->LPlace().
//
void
cSced::DevsEscCallback()
{
    if (Dv)
        Dv->esc();
}


// wl is consumed
sDv::sDv(GRobject caller, stringlist *wl) : GTKdraw(XW_DEFAULT)
{
    Dv = this;
    dv_caller = caller;
    dv_morebtn = 0;
    dv_entries = 0;
    dv_numdevs = 0;
    dv_leftindx = dv_leftofst = dv_rightindx = 0;
    dv_curdev = dv_pressed = 0;
    dv_px = dv_py = 0;
    dv_foreg = 0;
    dv_backg = 0;
    dv_hlite = 0;
    dv_selec = 0;
    dv_active = false;

    stringlist::sort(wl, dv_comp_func);

    wb_shell = gtk_NewPopup(mainBag(), "Device Palette", dv_cancel_proc, 0);
    if (!wb_shell)
        return;

    GtkWidget *form = gtk_table_new(2, 1, false);
    gtk_widget_show(form);
    gtk_container_add(GTK_CONTAINER(wb_shell), form);

    const char *type = CDvdb()->getVariable(VA_DevMenuStyle);

    if (type && *type == '2') {
        dv_type = dvMenuPict;
        gd_viewport = gtk_drawing_area_new();
        gtk_widget_show(gd_viewport);
        gtk_widget_set_size_request(gd_viewport, 40, CELL_SIZE);

        GTKfont::setupFont(gd_viewport, FNT_SCREEN, true);

        int i = stringlist::length(wl);
        dv_entries = new sEnt[i];
        dv_numdevs = i;
        dv_leftindx = 0;
        i = 0;
        for (stringlist *wx = wl; wx; i++, wx = wl) {
            wl = wx->next;
            if (OIfailed(CD()->OpenExisting(wx->string, 0)) &&
                    OIfailed(FIO()->OpenLibCell(XM()->DeviceLibName(),
                    wx->string, LIBdevice, 0))) {
                delete [] wx->string;
                delete wx;
                continue;
            }
            dv_entries[i].name = wx->string;
            wx->string = 0;
            delete wx;
        }

        int width = init_sizes();
        gtk_window_set_default_size(GTK_WINDOW(wb_shell), width, -1);

        gtk_widget_add_events(gd_viewport, GDK_EXPOSURE_MASK);
        g_signal_connect(G_OBJECT(gd_viewport), "expose-event",
            G_CALLBACK(dv_redraw_hdlr), 0);
        gtk_widget_add_events(gd_viewport, GDK_BUTTON_PRESS_MASK);
        g_signal_connect_after(G_OBJECT(gd_viewport), "button-press-event",
            G_CALLBACK(dv_btn_hdlr), 0);
        gtk_widget_add_events(gd_viewport, GDK_POINTER_MOTION_MASK);
        g_signal_connect(G_OBJECT(gd_viewport), "motion-notify-event",
            G_CALLBACK(dv_motion_hdlr), 0);
        gtk_widget_add_events(gd_viewport, GDK_ENTER_NOTIFY_MASK);
        g_signal_connect(G_OBJECT(gd_viewport), "enter-notify-event",
            G_CALLBACK(dv_enter_hdlr), 0);
        gtk_widget_add_events(gd_viewport, GDK_LEAVE_NOTIFY_MASK);
        g_signal_connect(G_OBJECT(gd_viewport), "leave-notify-event",
            G_CALLBACK(dv_leave_hdlr), 0);
        g_signal_connect(G_OBJECT(gd_viewport), "style-set",
            G_CALLBACK(dv_font_change_hdlr), 0);

        gtk_table_attach(GTK_TABLE(form), gd_viewport, 0, 1, 0, 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);

        GtkWidget *vbox = gtk_vbox_new(false, 2);
        gtk_widget_show(vbox);

        GtkWidget *button = new_pixmap_button(more_xpm, 0, false);
        gtk_widget_set_name(button, "More");
        dv_morebtn = button;
        // don't show unless needed
        g_signal_connect(G_OBJECT(button), "clicked",
            G_CALLBACK(dv_more_proc), 0);
        gtk_box_pack_start(GTK_BOX(vbox), button, true, true, 0);

        button = new_pixmap_button(dd_xpm, 0, false);
        gtk_widget_set_name(button, "Drop");
        gtk_widget_show(button);
        g_signal_connect(G_OBJECT(button), "clicked",
            G_CALLBACK(dv_switch_proc), 0);
        gtk_box_pack_start(GTK_BOX(vbox), button, true, true, 0);

        gtk_table_attach(GTK_TABLE(form), vbox, 1, 2, 0, 1,
            (GtkAttachOptions)0,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);

        dv_backg = gtk_PopupColor(GRattrColorDvBg)->pixel;
        dv_foreg = gtk_PopupColor(GRattrColorDvFg)->pixel;
        dv_hlite = gtk_PopupColor(GRattrColorDvHl)->pixel;
        dv_selec = gtk_PopupColor(GRattrColorDvSl)->pixel;
        dv_pressed = -1;
        dv_curdev = -1;

        dv_active = true;
        return;
    }

    gtk_window_set_resizable(GTK_WINDOW(wb_shell), false);

    if (type && *type == '1') {
        dv_type = dvMenuAlpha;
        GtkWidget *frame = gtk_frame_new("Devices");
        gtk_widget_show(frame);

        GtkWidget *hbox = gtk_hbox_new(false, 2);
        gtk_widget_show(hbox);

        GtkWidget *menubar = gtk_menu_bar_new();
        gtk_widget_show(menubar);
        gtk_container_add(GTK_CONTAINER(frame), hbox);
        gtk_box_pack_start(GTK_BOX(hbox), menubar, true, true, 0);

        char lastc = 0;
        GtkWidget *menu = 0;
        for (stringlist *ww = wl; ww; ww = ww->next) {
            if (OIfailed(CD()->OpenExisting(ww->string, 0)) &&
                    OIfailed(FIO()->OpenLibCell(XM()->DeviceLibName(),
                    ww->string, LIBdevice, 0)))
                continue;
            char c = *ww->string;
            if (islower(c))
                c = toupper(c);
            if (c != lastc) {
                char bf[4];
                bf[0] = c;
                bf[1] = '\0';

                GtkWidget *head = gtk_menu_item_new_with_label(bf);
                gtk_widget_set_name(head, bf);
                gtk_widget_show(head);
                gtk_menu_shell_append(GTK_MENU_SHELL(menubar), head);

                menu = gtk_menu_new();
                gtk_menu_item_set_submenu(GTK_MENU_ITEM(head), menu);
            }
            GtkWidget *ent = gtk_menu_item_new_with_label(ww->string);
            gtk_widget_set_name(ent, ww->string);
            gtk_widget_show(ent);
            g_signal_connect(G_OBJECT(ent), "activate",
                G_CALLBACK(dv_menu_proc), 0);
            g_object_set_data(G_OBJECT(ent), "user", ww->string);
            ww->string = 0;
            gtk_menu_shell_append(GTK_MENU_SHELL(menu), ent);
            lastc = c;
        }
        stringlist::destroy(wl);

        GtkWidget *button = new_pixmap_button(pict_xpm, 0, false);
        gtk_widget_set_name(button, "Style");
        gtk_widget_show(button);
        g_signal_connect(G_OBJECT(button), "clicked",
            G_CALLBACK(dv_switch_proc), 0);
        gtk_box_pack_start(GTK_BOX(hbox), button, true, true, 0);

        gtk_table_attach(GTK_TABLE(form), frame, 0, 1, 0, 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }
    else {
        dv_type = dvMenuCateg;
        GtkWidget *hbox = gtk_hbox_new(false, 2);
        gtk_widget_show(hbox);

        GtkWidget *menubar = gtk_menu_bar_new();
        gtk_widget_show(menubar);
        gtk_box_pack_start(GTK_BOX(hbox), menubar, true, true, 0);

        GtkWidget *head = gtk_menu_item_new_with_label("Devices");
        gtk_widget_set_name(head, "Devices");
        gtk_widget_show(head);
        gtk_menu_shell_append(GTK_MENU_SHELL(menubar), head);
        GtkWidget *menu_d = gtk_menu_new();
        gtk_menu_item_set_submenu(GTK_MENU_ITEM(head), menu_d);

        head = gtk_menu_item_new_with_label("Sources");
        gtk_widget_set_name(head, "Sources");
        gtk_widget_show(head);
        gtk_menu_shell_append(GTK_MENU_SHELL(menubar), head);
        GtkWidget *menu_s = gtk_menu_new();
        gtk_menu_item_set_submenu(GTK_MENU_ITEM(head), menu_s);

        head = gtk_menu_item_new_with_label("Macros");
        gtk_widget_set_name(head, "Macros");
        gtk_widget_show(head);
        gtk_menu_shell_append(GTK_MENU_SHELL(menubar), head);
        GtkWidget *menu_m = gtk_menu_new();
        gtk_menu_item_set_submenu(GTK_MENU_ITEM(head), menu_m);

        head = gtk_menu_item_new_with_label("Terminals");
        gtk_widget_set_name(head, "Terminals");
        gtk_widget_show(head);
        gtk_menu_shell_append(GTK_MENU_SHELL(menubar), head);
        GtkWidget *menu_t = gtk_menu_new();
        gtk_menu_item_set_submenu(GTK_MENU_ITEM(head), menu_t);

        for (stringlist *ww = wl; ww; ww = ww->next) {
            CDcbin cbin;
            if (OIfailed(CD()->OpenExisting(ww->string, &cbin)) &&
                    OIfailed(FIO()->OpenLibCell(XM()->DeviceLibName(),
                    ww->string, LIBdevice, &cbin)))
                continue;
            if (!cbin.elec())
                continue;

            GtkWidget *menu = 0;
            CDpfxName namestr;
            CDelecCellType et = cbin.elec()->elecCellType(&namestr);
            switch (et) {
            case CDelecBad:
                break;
            case CDelecNull:
                // This should include the "mut" pseudo-device.
                menu = menu_d;
                break;
            case CDelecGnd:
            case CDelecTerm:
                menu = menu_t;
                break;
            case CDelecDev:
                {
                    char c = namestr ? *Tstring(namestr) : 0;
                    if (isupper(c))
                        c = tolower(c);
                    switch (c) {
                    case 'e':
                    case 'f':
                    case 'g':
                    case 'h':
                    case 'i':
                    case 'v':
                        menu = menu_s;
                        break;
                    case 'x':
                        // shouldn't get here
                        menu = menu_m;
                        break;
                    default:
                        menu = menu_d;
                        break;
                    }
                }
                break;
            case CDelecMacro:
            case CDelecSubc:
                menu = menu_m;
                break;
            }
            if (menu) {
                GtkWidget *ent = gtk_menu_item_new_with_label(ww->string);
                gtk_widget_set_name(ent, ww->string);
                gtk_widget_show(ent);
                g_signal_connect(G_OBJECT(ent), "activate",
                    G_CALLBACK(dv_menu_proc), 0);
                g_object_set_data(G_OBJECT(ent), "user", ww->string);
                gtk_menu_shell_append(GTK_MENU_SHELL(menu), ent);
                ww->string = 0;
            }
        }
        stringlist::destroy(wl);

        GtkWidget *button = new_pixmap_button(dda_xpm, 0, false);
        gtk_widget_set_name(button, "Style");
        gtk_widget_show(button);
        g_signal_connect(G_OBJECT(button), "clicked",
            G_CALLBACK(dv_switch_proc), 0);
        gtk_box_pack_start(GTK_BOX(hbox), button, true, true, 0);

        gtk_table_attach(GTK_TABLE(form), hbox, 0, 1, 0, 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }
    dv_active = true;
}


sDv::~sDv()
{
    Dv = 0;
    if (dv_caller)
        GRX->SetStatus(dv_caller, false);
    delete [] dv_entries;
    if (wb_shell) {
        g_signal_handlers_disconnect_by_func(G_OBJECT(wb_shell),
            (gpointer)dv_cancel_proc, wb_shell);
    }
}


void
sDv::activate(bool active)
{
    // Keep the saved position relative to the main window, absolute
    // coordinates can cause trouble with multi-monitors.

    if (active) {
        if (!dv_active) {
            int x, y;
            gdk_window_get_root_origin(
                gtk_widget_get_window(mainBag()->Shell()), &x, &y);
            gtk_window_move(GTK_WINDOW(wb_shell), dv_px + x, dv_py + y);
            gtk_widget_show(wb_shell);
            dv_active = true;
        }
    }
    else {
        if (dv_active) {
            gdk_window_get_root_origin(gtk_widget_get_window(wb_shell),
                &dv_px, &dv_py);
            int x, y;
            gdk_window_get_root_origin(
                gtk_widget_get_window(mainBag()->Shell()), &x, &y);
            dv_px -= x;
            dv_py -= y;
            gtk_widget_hide(wb_shell);
            dv_active = false;
            if (dv_caller)
                GRX->SetStatus(dv_caller, false);
        }
    }
}


void
sDv::esc()
{
    if (dv_type == dvMenuPict) {
        if (dv_pressed >= 0)
            render_cell(dv_pressed, false);
        dv_pressed = -1;
    }
}


// These functions are used only in the pictorial mode.
//

// Initialize the widths of the display areas for each device, return the
// total width.
//
int
sDv::init_sizes()
{
    double ratio = (double)(CELL_SIZE - 2*SPA)/DEVSIZE;
    int left = 2*SPA;

    for (int i = 0; i < dv_numdevs; i++) {
        CDcbin cbin;
        if (OIfailed(CD()->OpenExisting(dv_entries[i].name, &cbin)))
            continue;
        CDs *sdesc = cbin.elec();
        if (!sdesc)
            continue;
        CDs *syr = sdesc->symbolicRep(0);
        if (syr)
            sdesc = syr;
        sdesc->computeBB();
        BBox BB = *sdesc->BB();
        dv_entries[i].x = left - SPA;
        dv_entries[i].width = (int)(BB.width()*ratio) + 2*SPA;
        int sw =
            GTKfont::stringWidth(gd_viewport, dv_entries[i].name);
        if (dv_entries[i].width < sw)
            dv_entries[i].width = sw;
        left += dv_entries[i].width + 2*SPA;
    }
    if (!mainBag())
        return (0);

    int width = gdk_window_get_width(
        gtk_widget_get_window(mainBag()->Viewport()));
    left += 40 + SPA;  // Button width is approx 40.
    if (left < width)
        width = left;
    if (width <= 0)
        // all lib cells are empty, shouldn't happen
        width = 100;
    return (width);
}


// Render the library device in the toolbar.
//
void
sDv::render_cell(int which, bool selected)
{
    SetColor(selected ? dv_selec : dv_backg);
    Box(dv_entries[which].x - dv_leftofst, SPA, dv_entries[which].width,
        CELL_SIZE - 2*SPA);

    CDcbin cbin;
    if (OIfailed(CD()->OpenExisting(dv_entries[which].name, &cbin)))
        return;
    CDs *sdesc = cbin.elec();
    if (!sdesc)
        return;
    CDs *syr = sdesc->symbolicRep(0);
    if (syr)
        sdesc = syr;
    sdesc->computeBB();
    BBox BB = *sdesc->BB();
    int x = (BB.left + BB.right)/2;
    int y = (BB.bottom + BB.top)/2;

    int vp_height = CELL_SIZE - 2*SPA;
    int vp_width = dv_entries[which].width - 2;

    WindowDesc wd;
    wd.Attrib()->set_display_labels(Electrical, SLupright);
    wd.SetWbag(DSP()->MainWdesc()->Wbag());
    wd.SetWdraw(this);

    wd.SetRatio(((double)vp_height)/DEVSIZE);
    int height = (int)(vp_height/wd.Ratio());
    wd.Window()->top = y + height/2;
    wd.Window()->bottom = y - height/2;
    // Shift a little for text placement compensation.
    wd.Window()->left = BB.left - (int)(SPA/wd.Ratio());
    wd.Window()->right = BB.right - (int)(SPA/wd.Ratio());

    wd.InitViewport(vp_width, vp_height);
    *wd.ClipRect() = wd.Viewport();

#ifdef NEW_NDK
    GetDrawable()->set_draw_to_pixmap();
#else
    GdkPixmap *pm = gdk_pixmap_new(gd_window,  vp_width, vp_height,
        gdk_visual_get_depth(GRX->Visual()));
    // swap in the pixmap
    GRobject window_bak = gd_window;
    gd_window = pm;
#endif

    SetColor(selected ? dv_selec : dv_backg);
    SetFillpattern(0);
    Box(0, 0, vp_width - 1, vp_height - 1);

    DSP()->TPush();
    SetColor(dv_foreg);
    CDl *ld;
    CDlgen lgen(DSP()->CurMode());
    while ((ld = lgen.next()) != 0) {
        if (ld->isInvisible() || ld->isNoInstView())
            continue;
        CDg gdesc;
        gdesc.init_gen(sdesc, ld);
        CDo *pointer;
        while ((pointer = gdesc.next()) != 0) {
            if (pointer->type() == CDBOX)
                wd.ShowBox(&pointer->oBB(), ld->getAttrFlags(),
                    dsp_prm(ld)->fill());
            else if (pointer->type() == CDPOLYGON) {
                const Poly po(((const CDpo*)pointer)->po_poly());
                wd.ShowPolygon(&po, ld->getAttrFlags(), dsp_prm(ld)->fill(),
                    &pointer->oBB());
            }
            else if (pointer->type() == CDWIRE) {
                const Wire w(((const CDw*)pointer)->w_wire());
                wd.ShowWire(&w, 0, 0);
            }
            else if (pointer->type() == CDLABEL) {
                const Label la(((const CDla*)pointer)->la_label());
                wd.ShowLabel(&la);
            }
        }
    }
    int fwid, fhei;
    TextExtent(dv_entries[which].name, &fwid, &fhei);
    x = 0;
    y = vp_height - fhei;
    SetColor(selected ? dv_selec : dv_backg);
    BBox tBB(x, y+fhei, x+fwid, y);
    Box(tBB.left, tBB.bottom, tBB.right,
        tBB.bottom - (abs(tBB.height())*8)/10);
    SetColor(dv_hlite);
    Text(dv_entries[which].name, tBB.left, tBB.bottom, 0,
        tBB.width(), abs(tBB.height()));
    DSP()->TPop();

    int xoff = dv_entries[which].x - dv_leftofst - 2;
    int yoff = SPA;

#ifdef NEW_NDK
    GetDrawable()->set_draw_to_window();
#else
    gdk_window_copy_area((GdkWindow*)window_bak, GC(),
        xoff, yoff, (GdkWindow*)gd_window, 0, 0, vp_width, vp_height);

    gd_window = (GdkWindow*)window_bak;
    gdk_pixmap_unref(pm);
#endif
}


// Function to show another box full of devices, called in 'more'
// mode when there are too many devices to fit at once.
//
void
sDv::cyclemore()
{
    Clear();
    if (dv_rightindx == dv_numdevs)
        dv_leftindx = 0;
    else
        dv_leftindx = dv_rightindx;
    dv_redraw_hdlr(0, 0, 0);
}


// Return the device index of the device under the pointer, or
// -1 if the pointer is not over a device.
//
int
sDv::whichent(int x)
{
    x += dv_leftofst;
    for (int i = dv_leftindx; i < dv_rightindx; i++) {
        if (dv_entries[i].x <= x && x <= dv_entries[i].x + dv_entries[i].width)
            return (i);
    }
    return (-1);
}


// Outline the device as selected.  This happens when the pointer is
// over the device.
//
void
sDv::show_selected(int which)
{
    if (which < 0)
        return;
    int w = dv_entries[which].width + SPA;
    int h = CELL_SIZE - SPA;

    SetLinestyle(0);
    SetFillpattern(0);

    GRmultiPt xp(3);
    xp.assign(0, dv_entries[which].x - SPA/2 - dv_leftofst, SPA/2 + h);
    xp.assign(1, xp.at(0).x, xp.at(0).y - h);
    xp.assign(2, xp.at(1).x + w, xp.at(1).y);
    GtkStyle *style = gtk_widget_get_style(gd_viewport);
    int state = gtk_widget_get_state(gd_viewport);
    SetColor(style->light[state].pixel);
    PolyLine(&xp, 3);
    xp.assign(0, dv_entries[which].x - SPA/2 - dv_leftofst, SPA/2 + h);
    xp.assign(1, xp.at(0).x + w, xp.at(0).y);
    xp.assign(2, xp.at(1).x, xp.at(1).y - h);
    SetColor(style->dark[state].pixel);
    PolyLine(&xp, 3);
}


// Outline the device as unselected, the normal state.
//
void
sDv::show_unselected(int which)
{
    if (which < 0)
        return;
    int w = dv_entries[which].width + SPA;
    int h = CELL_SIZE - SPA;

    SetLinestyle(0);
    SetFillpattern(0);

    GRmultiPt xp(3);
    xp.assign(0, dv_entries[which].x - SPA/2 - dv_leftofst, SPA/2 + h);
    xp.assign(1, xp.at(0).x, xp.at(0).y - h);
    xp.assign(2, xp.at(1).x + w, xp.at(1).y);
    GtkStyle *style = gtk_widget_get_style(gd_viewport);
    int state = gtk_widget_get_state(gd_viewport);
    SetColor(style->dark[state].pixel);

    PolyLine(&xp, 3);
    xp.assign(0, dv_entries[which].x - SPA/2 - dv_leftofst, SPA/2 + h);
    xp.assign(1, xp.at(0).x + w, xp.at(0).y);
    xp.assign(2, xp.at(1).x, xp.at(1).y - h);
    SetColor(style->light[state].pixel);
    PolyLine(&xp, 3);
}


// Static function.
// Pop down if we get a message from the window manager.
//
void
sDv::dv_cancel_proc(GtkWidget*, void*)
{
    SCD()->PopUpDevs(0, MODE_OFF);
}


// Static function.
// Show the next page of entries (Pictorial only).
//
void
sDv::dv_more_proc(GtkWidget*, void*)
{
    Dv->cyclemore();
}


// Static function.
// Switch to the alternate presentation style.
//
void
sDv::dv_switch_proc(GtkWidget*, void*)
{
    switch (Dv->dv_type) {
    case dvMenuCateg:
        CDvdb()->setVariable(VA_DevMenuStyle, "1");
        break;
    case dvMenuAlpha:
        CDvdb()->setVariable(VA_DevMenuStyle, "2");
        break;
    case dvMenuPict:
        CDvdb()->clearVariable(VA_DevMenuStyle);
        break;
    }
}


// Static function.
// Menu handler (MenuCateg and MenuAlpha only).
//
void
sDv::dv_menu_proc(GtkWidget *caller, void*)
{
    char *string = (char*)g_object_get_data(G_OBJECT(caller), "user");
    if (XM()->IsDoingHelp()) {
        char tbuf[128];
        sprintf(tbuf, "dev:%s", string);
        DSPmainWbag(PopUpHelp(tbuf))
        return;
    }
    GRX->SetFocus(mainBag()->Shell());  // give focus to main window
    EV()->InitCallback();
    if (!strcmp(string, MUT_DUMMY)) {
        CmdDesc cmd;
        cmd.caller = caller;
        cmd.wdesc = DSP()->MainWdesc();
        SCD()->showMutualExec(&cmd);
    }
    else
        ED()->placeDev(0, string, false);
}


// Static function.
// Comparison function for device names.
//
bool
sDv::dv_comp_func(const char *p, const char *q)
{
    while (*p) {
        char c = islower(*p) ? toupper(*p) : *p;
        char d = islower(*q) ? toupper(*q) : *q;
        if (c != d)
            return (c < d);
        p++;
        q++;
    }
    return (q != 0);
}


//
// Handler functions, used in Pictorial mode only.
//

// Static function.
// In gtk2, this must be an idle proc to update correctly.
//
int
sDv::dv_redraw_idle(void*)
{
    Dv->dv_leftofst = Dv->dv_entries[Dv->dv_leftindx].x - SPA;
#ifdef NEW_NDK
    int width = Dv->GetDrawable()->get_width();
#else
    int width = gdk_window_get_width(gtk_widget_get_window(Dv->gd_viewport));
#endif
    int i;
    for (i = Dv->dv_leftindx; i < Dv->dv_numdevs; i++) {
        if (Dv->dv_entries[i].x - Dv->dv_leftofst +
                Dv->dv_entries[i].width + SPA > width)
            break;
    }
    if (i == Dv->dv_numdevs && Dv->dv_leftindx == 0) {
        if (gtk_widget_get_visible(Dv->dv_morebtn))
            gtk_widget_hide(Dv->dv_morebtn);
    }
    else {
        if (!gtk_widget_get_visible(Dv->dv_morebtn))
            gtk_widget_show(Dv->dv_morebtn);
    }
    Dv->dv_rightindx = i;

    for (i = Dv->dv_leftindx; i < Dv->dv_rightindx; i++) {
        Dv->render_cell(i, i == Dv->dv_pressed);
        Dv->show_unselected(i);
    }
    return (false);
}


// Static function.
// Draw/redraw the toolbar.
//
int
sDv::dv_redraw_hdlr(GtkWidget*, GdkEvent*, void*)
{
    dspPkgIf()->RegisterIdleProc(dv_redraw_idle, 0);
    return (true);
}


// Static function.
// Draw/redraw the toolbar.
// Select the device clicked on, and call the function to allow
// device placement.
//
int
sDv::dv_btn_hdlr(GtkWidget*, GdkEvent *event, void*)
{
    int n = Dv->whichent((int)event->button.x);
    if (n < 0)
        return (false);
    if (event->type == GDK_BUTTON_PRESS && event->button.button <= 3) {
        Dv->dv_curdev = n;
        if (XM()->IsDoingHelp()) {
            char tbuf[128];
            sprintf(tbuf, "dev:%s", Dv->dv_entries[n].name);
            DSPmainWbag(PopUpHelp(tbuf))
            return (true);
        }
        if (Dv->dv_pressed >= Dv->dv_leftindx &&
                Dv->dv_pressed < Dv->dv_rightindx)
            Dv->render_cell(Dv->dv_pressed, false);
        int lp = Dv->dv_pressed;
        EV()->InitCallback();  // sets pressed to -1
        if (lp != n) {
            Dv->render_cell(n, true);
            Dv->dv_pressed = n;
            if (!strcmp(Dv->dv_entries[n].name, MUT_DUMMY)) {
                CmdDesc cmd;
                cmd.caller = 0;
                cmd.wdesc = DSP()->MainWdesc();
                SCD()->showMutualExec(&cmd);
            }
            else
                ED()->placeDev(0, Dv->dv_entries[n].name, false);
        }
    }
    return (true);
}


// Static function.
// Change the border surrounding the device under the pointer.
//
int
sDv::dv_motion_hdlr(GtkWidget*, GdkEvent *event, void*)
{
    int n = Dv->whichent((int)event->motion.x);
    if (n != Dv->dv_curdev)
        Dv->show_unselected(Dv->dv_curdev);
    Dv->dv_curdev = n;
    Dv->show_selected(Dv->dv_curdev);
    return (true);
}


// Static function.
// On entering the toolbar, change the border of the device
// under the pointer.
//
int
sDv::dv_enter_hdlr(GtkWidget*, GdkEvent *event, void*)
{
    Dv->dv_curdev = Dv->whichent((int)event->crossing.x);
    Dv->show_selected(Dv->dv_curdev);
    return (true);
}


// Static function.
// On leaving the toolbar, return the border to normal.
//
int
sDv::dv_leave_hdlr(GtkWidget*, GdkEvent *event, void*)
{
    Dv->dv_curdev = Dv->whichent((int)event->crossing.x);
    Dv->show_unselected(Dv->dv_curdev);
    return (true);
}


// Static function.
// Compute new field widths on font change.
//
void
sDv::dv_font_change_hdlr(GtkWidget*, void*, void*)
{
    Dv->init_sizes();
}


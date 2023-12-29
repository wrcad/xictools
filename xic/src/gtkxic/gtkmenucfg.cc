
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
#include "editif.h"
#include "scedif.h"
#include "dsp_inlines.h"
#include "si_parsenode.h"
#include "si_parser.h"
#include "si_interp.h"
#include "select.h"
#include "events.h"
#include "errorlog.h"
#include "oa_if.h"

#include "file_menu.h"
#include "cell_menu.h"
#include "edit_menu.h"
#include "modf_menu.h"
#include "view_menu.h"
#include "attr_menu.h"
#include "cvrt_menu.h"
#include "drc_menu.h"
#include "ext_menu.h"
#include "user_menu.h"
#include "help_menu.h"
#include "pbtn_menu.h"
#include "ebtn_menu.h"
#include "misc_menu.h"

#include "gtkmain.h"
#include "gtkmenu.h"
#include "gtkmenucfg.h"
#include "bitmaps/style_e.xpm"
#include "bitmaps/style_f.xpm"
#include "bitmaps/style_r.xpm"

#include <gdk/gdkkeysyms.h>


//-----------------------------------------------------------------------------
// Menu Configuration
//
// Help system keywords used:
//  xic:xxx  (xxx is command code)
//  xic:open
//  xic:view
//  xic:width
//  xic:end_flush
//  xic:end_round
//  xic:end_extend
//  shapes:xxx (xxx is shape code)
//  user:xxx (xxx is user code)

// Offset code for menu entries not defined in the data structs.
//
#define NOTMAPPED 255

// Pointer to menu handler.
#define voidptr (gpointer)(long)

namespace {
    const char *MIDX = "midx";

    // Instantiate
    GTKmenuConfig _cfg_;
}


GTKmenuConfig *GTKmenuConfig::instancePtr = 0;

GTKmenuConfig::GTKmenuConfig()
{
    if (instancePtr) {
        fprintf(stderr,
            "Singleton class GTKmenuConfig already instantiated.\n");
        exit(1);
    }
    instancePtr = this;
}


GTKmenuConfig::~GTKmenuConfig()
{
    instancePtr = 0;
}


// Private static error exit.
//
void
GTKmenuConfig::on_null_ptr()
{
    fprintf(stderr,
        "Singleton class GTKmenuConfig used before instantiated.\n");
    exit(1);
}


namespace {

    // The MenuEnt structure is defined in the main application code,
    // otherwise these could be methods.  We prefer to keep this
    // low-level toolkit-specific code out the the application
    // structures.

    void set(MenuEnt *ent, const char *text, const char *ac, const char *it = 0)
    {
        ent->menutext = text;
        ent->accel = ac;
        if (it)
            ent->item = it;
        else if (ent->is_menu())
            ent->item = "<Branch>";
        else if (ent->is_toggle())
            ent->item = "<CheckItem>";
    }


    GtkWidget *mnset(MenuEnt *ent, const char *text)
    {
        ent->menutext = text;
        GtkWidget *item = gtk_menu_item_new_with_mnemonic(ent->menutext);
        if (ent->is_right()) {
            ent->item = "<LastBranch>";
            gtk_menu_item_set_right_justified(GTK_MENU_ITEM(item), true);
        }
        else
            ent->item = "<Branch>";
        gtk_widget_show(item);
        ent->cmd.caller = item;
        return (item);
    }


    GtkWidget *new_item(MenuEnt *ent)
    {
        GtkWidget *item;
        if (ent->is_toggle()) {
            item = gtk_check_menu_item_new_with_mnemonic(ent->menutext);
            if (ent->is_set())
                MainMenu()->Select(item);
        }
        else
            item = gtk_menu_item_new_with_mnemonic(ent->menutext);
        gtk_widget_show(item);
        ent->cmd.caller = item;
        if (ent->description)
            gtk_widget_set_tooltip_text(item, ent->description);

        if (ent->menutext) {
            // Name the button by stripping the underscore accelerator
            // from trhe lavel text.
            char bf[32];
            int i = 0;
            for (const char *s = ent->menutext; *s && i < 31; s++) {
                if (*s == '_')
                    continue;
                bf[i++] = *s;
            }
            bf[i] = 0;
            if (bf[0])
                gtk_widget_set_name(item, bf);
        }
        return (item);
    }


    GtkWidget *miset(MenuEnt *ent, const char *text, const char *ac,
        const char *it = 0)
    {
        ent->menutext = text;
        ent->accel = ac;
        if (it)
            ent->item = it;
        else if (ent->is_menu())
            ent->item = "<Branch>";
        else if (ent->is_toggle())
            ent->item = "<CheckItem>";
        GtkWidget *item;
        if (ent->is_toggle()) {
            item = gtk_check_menu_item_new_with_mnemonic(ent->menutext);
            if (ent->is_set())
                MainMenu()->Select(item);
        }
        else
            item = gtk_menu_item_new_with_mnemonic(ent->menutext);
        ent->cmd.caller = item;
        gtk_widget_show(item);
        if (ent->description)
            gtk_widget_set_tooltip_text(item, ent->description);
        if (ent->menutext) {
            // Name the button by stripping the underscore accelerator
            // from trhe lavel text.
            char bf[32];
            int i = 0;
            for (const char *s = ent->menutext; *s && i < 31; s++) {
                if (*s == '_')
                    continue;
                bf[i++] = *s;
            }
            bf[i] = 0;
            if (bf[0])
                gtk_widget_set_name(item, bf);
        }
        return (item);
    }


    void set_btn(MenuEnt *ent, unsigned long ix, gpointer cb)
    {
        GtkWidget *button = gtk_NewPixmapButton(ent->xpm, ent->menutext,
            ent->is_toggle());
        g_object_set_data(G_OBJECT(button), MIDX, voidptr ix);
        if (ent->is_set())
            MainMenu()->Select(button);
        g_signal_connect(G_OBJECT(button), "clicked",
            G_CALLBACK(cb), (gpointer)(ent - ix));
        if (ent->description)
            gtk_widget_set_tooltip_text(button, ent->description);
        gtk_widget_show(button);
        gtk_widget_set_name(button, ent->menutext);
        ent->cmd.caller = button;
    }


    void check_separator(MenuEnt *ent, GtkWidget *menu)
    {
        if (ent->is_separator()) {
            GtkWidget *item = gtk_separator_menu_item_new();
            gtk_widget_show(item);
            gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
        }
    }
}


void
GTKmenuConfig::instantiateMainMenus()
{
    instantiateFileMenu();
    instantiateCellMenu();
    instantiateEditMenu();
    instantiateModifyMenu();
    instantiateViewMenu();
    instantiateAttributesMenu();
    instantiateConvertMenu();
    instantiateDRCMenu();
    instantiateExtractMenu();
    instantiateUserMenu();
    instantiateHelpMenu();

    switch_menu_mode(DSP()->CurMode(), 0);
}


void
GTKmenuConfig::instantiateTopButtonMenu()
{
    // Horizontal button line at top of main window.
    MenuBox *mbox = MainMenu()->GetMiscMenu();
    if (!mbox || !mbox->menu)
        return;

    MenuEnt *ent = &mbox->menu[miscMenu];
    set(ent, 0, 0);

    ent = &mbox->menu[miscMenuMail];
    set(ent, "Mail", 0);
    set_btn(ent, miscMenuMail, (gpointer)menu_handler);

    ent = &mbox->menu[miscMenuLtvis];
    set(ent, "LTvisib", 0);
    set_btn(ent, miscMenuLtvis, (gpointer)menu_handler);

    ent = &mbox->menu[miscMenuLpal];
    set(ent, "Palette", 0);
    set_btn(ent, miscMenuLpal, (gpointer)menu_handler);

    ent = &mbox->menu[miscMenuSetcl];
    set(ent, "SetCL", 0);
    set_btn(ent, miscMenuSetcl, (gpointer)menu_handler);

    ent = &mbox->menu[miscMenuSelcp];
    set(ent, "SelCP", 0);
    set_btn(ent, miscMenuSelcp, (gpointer)menu_handler);

    ent = &mbox->menu[miscMenuDesel];
    set(ent, "Desel", 0);
    set_btn(ent, miscMenuDesel, (gpointer)menu_handler);

    ent = &mbox->menu[miscMenuRdraw];
    set(ent, "Rdraw", 0);
    set_btn(ent, miscMenuRdraw, (gpointer)menu_handler);
}


void
GTKmenuConfig::instantiateSideButtonMenus()
{
    instantiatePhysSideButtonMenu();
    instantiateElecSideButtonMenu();
}


void
GTKmenuConfig::instantiateSubwMenus(int wnum, GtkWidget *menubar,
    GtkAccelGroup *accel_group)
{
    instantiateSubwViewMenu(wnum, menubar, accel_group);
    instantiateSubwAttrMenu(wnum, menubar, accel_group);
    instantiateSubwHelpMenu(wnum, menubar, accel_group);

    switch_menu_mode(DSP()->CurMode(), wnum);
}


void
GTKmenuConfig::updateDynamicMenus()
{
    struct submenu_list
    {
        submenu_list(GtkWidget *w, submenu_list *n)
        {
            submenu = w;
            next = n;
        }

        GtkWidget *submenu;
        submenu_list *next;
    };

    MenuBox *mbox = MainMenu()->FindMainMenu(MMuser);
    if (!mbox || !mbox->menu || !mbox->isDynamic())
        return;
    MenuEnt *ent = mbox->menu;
    GtkWidget *item = 0;
    if (ent && ent->cmd.caller) {
        item = GTK_WIDGET(ent->cmd.caller);
        if (item) {
            GtkWidget *submenu = gtk_menu_item_get_submenu(GTK_MENU_ITEM(item));
            if (submenu) {
                GList *gl = gtk_container_get_children(GTK_CONTAINER(submenu));
                for (GList *l = gl; l; l = l->next)
                    gtk_widget_destroy(GTK_WIDGET(l->data));
                g_list_free(gl);
#ifdef USERDBG
                fprintf(stderr, "DBG:  I just killed the user menu widgets\n");
#endif
            }
#ifdef USERDBG
            else
                fprintf(stderr, "DBG:  User menu top ent has no submenu\n");
#endif
        }
#ifdef USERDBG
        else
            fprintf(stderr, "DBG:  User menu top ent has no cmd.caller set\n");
#endif
    }

    MainMenu()->RebuildDynamicMenus();

    mbox = MainMenu()->FindMainMenu(MMuser);
    if (!mbox || !mbox->menu || !mbox->isDynamic())
        return;
    ent = mbox->menu;
    if (!ent->cmd.caller)
        ent->cmd.caller = item;
    item = GTK_WIDGET(ent->cmd.caller);
    if (!item) {
#ifdef USERDBG
        fprintf(stderr, "Top User menu item not found, bye.\n");
#endif
        return;
    }
    GtkWidget *submenu = gtk_menu_item_get_submenu(GTK_MENU_ITEM(item));
    if (!submenu) {
        // Hmmm, someone killed it.
#ifdef USERDBG
        fprintf(stderr, "In top User menu, recreating submenu.\n");
#endif
        submenu = gtk_menu_new();
        gtk_widget_show(submenu);
        gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), submenu);
    }

    ent = &mbox->menu[userMenuDebug];
    item = miset(ent, "_Debugger", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr userMenuDebug);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

    ent = &mbox->menu[userMenuHash];
    item = miset(ent, "_Rehash", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr userMenuHash);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

    // Create new GtkMenuStrings.
    int cnt = 0;
    for (ent = mbox->menu; ent->entry; ent++) {
        if (cnt >= userMenu_END) {
            ent->menutext = ent->description;
            if (ent->is_menu())
                ent->item = "<Branch>";
        }
        cnt++;
    }

    submenu_list *submenu_stack = 0;
    int lastnsep = 0;

    // Add the dynamic entries.
    for (ent = &mbox->menu[userMenu_END]; ent->entry; ent++) {
        char buf[256];

        // The path saved in the menutext field is in the form
        // User/top/next/...  The top component is resolved
        // through the ScriptPath, so *must* be the basename of a
        // .scr or .scm file.  Components that follow are label
        // text.  The entry field is label text.
        //
        // Below we implement a stack, so that the display labels
        // can be substituted into the item factory path. 
        // Further, we need to strip out the '_' characters from
        // the item path since these would be interpreted as
        // accelerator characters.

// Strip '/User/' from the front of the menutext.
#define STRIPNBITS 6

        strcpy(buf, ent->menutext);
        char *t = strrchr(buf, '/');
        if (t)
            *t = 0;
        t = buf + strlen(buf);
        *t++ = '/';
        strcpy(t, ent->entry);
        // Strip out underscores (change them to hyphens).
        for ( ; *t; t++) {
            if (*t == '_')
                *t = '-';
        }

#ifdef USERDBG
        fprintf(stderr, "%s\n", buf);
#endif
        // Count the number of slashes, this is the "stack depth".
        int nsep = 0;
        for (t = buf + STRIPNBITS; *t; t++) {
            if (*t == '/')
                nsep++;
        }
        while (nsep < lastnsep) {
            // If the stack depth has decresed, pop until equality.
            submenu_list *el = submenu_stack;
            submenu_stack = el->next;
            el->next = 0;
            submenu = el->submenu;
            delete el;
            lastnsep--;
        }
        const char *stpth = buf + STRIPNBITS;
        const char *s = strrchr(stpth, '/');
        if (s)
            stpth = s+1;

        item = miset(ent, lstring::copy(stpth), ent->accel); 
        if (s) {
            // Bit of a hack here.  Strip the menu tokens from the
            // displayed name, but put them back in ent->menutext as the
            // evaluation logic requires it.

            delete [] ent->menutext;
            ent->menutext = lstring::copy(buf + STRIPNBITS);
        }
        g_object_set_data(G_OBJECT(item), MIDX, voidptr (ent - mbox->menu));
        gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
        g_signal_connect(G_OBJECT(item), "activate",
            G_CALLBACK(menu_handler), mbox->menu);
        check_separator(ent, submenu);
        if (ent->is_menu()) {
            submenu_stack = new submenu_list(submenu, submenu_stack);
            submenu = gtk_menu_new();
            gtk_widget_show(submenu);
            gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), submenu);
        }
        lastnsep = nsep;
    }
}


void
GTKmenuConfig::switch_menu_mode(DisplayMode mode, int wnum)
{
    if (wnum == 0) {
        GtkWidget *bp = GTKmenu::self()->FindMainMenuWidget(MMview, MenuPHYS);
        GtkWidget *be = GTKmenu::self()->FindMainMenuWidget(MMview, MenuSCED);
        if (bp && be) {
            if (mode == Physical) {
                gtk_widget_show(be);
                gtk_widget_hide(bp);
            }
            else {
                gtk_widget_show(bp);
                gtk_widget_hide(be);
            }
        }
        GtkWidget *ns = GTKmenu::self()->FindMainMenuWidget(MMattr, MenuNOSYM);
        if (ns) {
            if (mode == Physical)
                gtk_widget_hide(ns);
            else
                gtk_widget_show(ns);
        }

        // Desensitize the DRC menu in electrical mode.
        GtkWidget *mitem = GTKmenu::self()->FindMainMenuWidget(MMdrc, 0);
        if (mitem)
            gtk_widget_set_sensitive(mitem, (DSP()->CurMode() == Physical));

        // swap button menu
        if (mode == Physical) {
            if (GTKmenu::self()->btnElecMenuWidget)
                gtk_widget_hide(GTKmenu::self()->btnElecMenuWidget);
            if (GTKmenu::self()->btnPhysMenuWidget)
                gtk_widget_show(GTKmenu::self()->btnPhysMenuWidget);
        }
        else {
            if (GTKmenu::self()->btnPhysMenuWidget)
                gtk_widget_hide(GTKmenu::self()->btnPhysMenuWidget);
            if (GTKmenu::self()->btnElecMenuWidget)
                gtk_widget_show(GTKmenu::self()->btnElecMenuWidget);
        }
    }
    else {
        if (wnum >= DSP_NUMWINS)
            return;
        if (!DSP()->Window(wnum))
            return;

        const char *mmtok = 0;
        if (wnum == 1)
            mmtok = MMsub1;
        else if (wnum == 2)
            mmtok = MMsub2;
        else if (wnum == 3)
            mmtok = MMsub3;
        else if (wnum == 4)
            mmtok = MMsub4;
        if (!mmtok)
            return;

        GtkWidget *be = GTKmenu::self()->FindMainMenuWidget(mmtok, MenuSCED);
        GtkWidget *bp = GTKmenu::self()->FindMainMenuWidget(mmtok, MenuPHYS);
        if (be && bp) {
            if (mode == Physical) {
                gtk_widget_hide(bp);
                gtk_widget_show(be);
            }
            else {
                gtk_widget_hide(be);
                gtk_widget_show(bp);
            }
        }
        GtkWidget *ns = GTKmenu::self()->FindMainMenuWidget(mmtok, MenuNOSYM);
        if (ns) {
            if (mode == Physical)
                gtk_widget_hide(ns);
            else
                gtk_widget_show(ns);
        }
    }
}


// Turn on/off sensitivity of all menus in the main window.
//
void
GTKmenuConfig::set_main_global_sens(bool sens)
{
    if (GTKmenu::self()->ButtonMenu(Physical))
        gtk_widget_set_sensitive(GTKmenu::self()->ButtonMenu(Physical), sens);
    if (GTKmenu::self()->ButtonMenu(Electrical))
        gtk_widget_set_sensitive(GTKmenu::self()->ButtonMenu(Electrical), sens);

    for (const MenuList *ml = GTKmenu::self()->GetMainMenus(); ml;
            ml = ml->next) {
        if (!ml->menubox || !ml->menubox->menu)
            continue;
        MenuEnt *ent = ml->menubox->menu;

        GtkWidget *widget = GTK_WIDGET(ent->cmd.caller);
        if (!widget)
            continue;

        if (lstring::ciprefix(MMview, ml->menubox->name)) {
            // for View Menu, keep Allocation button sensitive
            GtkWidget *menu = gtk_menu_item_get_submenu(GTK_MENU_ITEM(widget));
            if (menu) {
                GList *gl = gtk_container_get_children(GTK_CONTAINER(menu));
                for (GList *l = gl; l; l = l->next)
                    gtk_widget_set_sensitive(GTK_WIDGET(l->data), sens);
                g_list_free(gl);
            }
            MenuEnt *nent = GTKmenu::self()->FindEntry(MMview, MenuALLOC, 0);
            if (nent && nent->cmd.caller)
                gtk_widget_set_sensitive(GTK_WIDGET(nent->cmd.caller), true);
        }
        else if (lstring::ciprefix(MMattr, ml->menubox->name)) {
            // for Attributes Menu, keep Main Window/Freeze button sensitive
            GtkWidget *menu = gtk_menu_item_get_submenu(GTK_MENU_ITEM(widget));
            if (menu) {
                GList *gl = gtk_container_get_children(GTK_CONTAINER(menu));
                for (GList *l = gl; l; l = l->next)
                    gtk_widget_set_sensitive(GTK_WIDGET(l->data), sens);
                g_list_free(gl);
            }
            MenuEnt *nent = GTKmenu::self()->FindEntry(MMattr, MenuMAINW, 0);
            if (nent && nent->cmd.caller) {
                GtkWidget *btn = GTK_WIDGET(nent->cmd.caller);
                gtk_widget_set_sensitive(btn, true);
                menu = gtk_menu_item_get_submenu(GTK_MENU_ITEM(btn));
                if (menu) {
                    GList *gl = gtk_container_get_children(GTK_CONTAINER(menu));
                    for (GList *l = gl; l; l = l->next)
                        gtk_widget_set_sensitive(GTK_WIDGET(l->data), sens);
                    g_list_free(gl);
                    nent = GTKmenu::self()->FindEntry(MMattr, MenuFREEZ, 0);
                    if (nent && nent->cmd.caller) {
                        gtk_widget_set_sensitive(GTK_WIDGET(nent->cmd.caller),
                            true);
                    }
                }
            }
        }
        else
            gtk_widget_set_sensitive(widget, sens);
    }
}


void
GTKmenuConfig::instantiateFileMenu()
{
    // File menu
    MenuBox *mbox = MainMenu()->FindMainMenu(MMfile);
    if (!mbox || !mbox->menu)
        return;
    GtkAccelGroup *accel_group = GTKmenu::self()->AccelGroup();
    if (!accel_group)
        return;
    GtkWidget *menubar = GTKmenu::self()->MainMenu();
    if (!menubar)
        return;

    MenuEnt *ent = &mbox->menu[fileMenu];
    GtkWidget *item = mnset(ent, "_File");
    gtk_widget_set_name(item, "File");
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), item);
    GtkWidget *submenu = gtk_menu_new();
    gtk_widget_show(submenu);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), submenu);

    ent = &mbox->menu[fileMenuFsel];
    item = miset(ent, "F_ile Select", "<control>O");
    g_object_set_data(G_OBJECT(item), MIDX, voidptr fileMenuFsel);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    gtk_widget_add_accelerator(item, "activate", accel_group, GDK_KEY_o,
        GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    check_separator(ent, submenu);

    ent = &mbox->menu[fileMenuOpen];
    item = miset(ent, "_Open", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr fileMenuOpen);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    // g_signal_connect(G_OBJECT(item), "activate",
    //     G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

    // Initialize the popup menus
    // set data "menu" -> menu in the buttons
    // set data "callb" -> callback function
    // set data "data" -> callback data
    GtkWidget *popup = GTKmenu::self()->new_popup_menu(item,
        XM()->OpenCellMenuList(), G_CALLBACK(edmenu_proc), 0);
    if (popup) {
        gtk_widget_set_name(popup, "EditSubmenu");
        g_object_set_data(G_OBJECT(item), "menu", popup);
        g_object_set_data(G_OBJECT(item), "callb", (void*)edmenu_proc);

        // Set up the mapping so that the accelerator for the
        // "open" button will activate the "new" operation from
        // the sub-menu.

        GList *contents = gtk_container_get_children(GTK_CONTAINER(popup));
        if (contents) {
            // the first item is the "new" button
            GtkWidget *newbut = GTK_WIDGET(contents->data);
            mbox->menu[fileMenuOpen].alt_caller = newbut;
            g_list_free(contents);
        }
        if (mbox->menu[fileMenuOpen].description) {
            gtk_widget_set_tooltip_text(item, 
                mbox->menu[fileMenuOpen].description);
        }
    }

    ent = &mbox->menu[fileMenuSave];
    set(ent, "_Save", "<Alt>S");
    if (EditIf()->hasEdit()) {
        item = new_item(ent);
        g_object_set_data(G_OBJECT(item), MIDX, voidptr fileMenuSave);
        gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
        g_signal_connect(G_OBJECT(item), "activate",
            G_CALLBACK(menu_handler), mbox->menu);
        gtk_widget_add_accelerator(item, "activate", accel_group, GDK_KEY_s,
            (GdkModifierType) (GDK_CONTROL_MASK|GDK_SHIFT_MASK),
            GTK_ACCEL_VISIBLE);
    }
    check_separator(ent, submenu);

    ent = &mbox->menu[fileMenuSaveAs];
    item = miset(ent, "Save _As", "<control>S");
    g_object_set_data(G_OBJECT(item), MIDX, voidptr fileMenuSaveAs);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    gtk_widget_add_accelerator(item, "activate", accel_group, GDK_KEY_s,
        GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    check_separator(ent, submenu);

    ent = &mbox->menu[fileMenuSaveAsDev];
    item = miset(ent, "Save As _Device", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr fileMenuSaveAsDev);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

    ent = &mbox->menu[fileMenuHcopy];
    item = miset(ent, "_Print", "<Alt>N");
    g_object_set_data(G_OBJECT(item), MIDX, voidptr fileMenuHcopy);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    gtk_widget_add_accelerator(item, "activate", accel_group, GDK_KEY_n,
        (GdkModifierType) (GDK_CONTROL_MASK|GDK_SHIFT_MASK),
        GTK_ACCEL_VISIBLE);
    check_separator(ent, submenu);

    ent = &mbox->menu[fileMenuFiles];
    item = miset(ent, "_Files List", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr fileMenuFiles);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

    ent = &mbox->menu[fileMenuHier];
    item = miset(ent, "_Hierarchy Digests", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr fileMenuHier);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

    ent = &mbox->menu[fileMenuGeom];
    item = miset(ent, "_Geometry Digests", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr fileMenuGeom);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

    ent = &mbox->menu[fileMenuLibs];
    item = miset(ent, "_Libraries List", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr fileMenuLibs);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

    ent = &mbox->menu[fileMenuOAlib];
    set(ent, "Open_Access Libs", 0);
    if (OAif()->hasOA()) {
        item = new_item(ent);
        g_object_set_data(G_OBJECT(item), MIDX, voidptr fileMenuOAlib);
        gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
        g_signal_connect(G_OBJECT(item), "activate",
            G_CALLBACK(menu_handler), mbox->menu);
    }
    check_separator(ent, submenu);

    ent = &mbox->menu[fileMenuExit];
    item = miset(ent, "_Quit", "<control>Q");
    g_object_set_data(G_OBJECT(item), MIDX, voidptr fileMenuExit);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    gtk_widget_add_accelerator(item, "activate", accel_group, GDK_KEY_q,
        GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    check_separator(ent, submenu);
}


void
GTKmenuConfig::instantiateCellMenu()
{
    // Cell menu
    MenuBox *mbox = MainMenu()->FindMainMenu(MMcell);
    if (!mbox || !mbox->menu)
        return;
    GtkAccelGroup *accel_group = GTKmenu::self()->AccelGroup();
    if (!accel_group)
        return;
    GtkWidget *menubar = GTKmenu::self()->MainMenu();
    if (!menubar)
        return;

    MenuEnt *ent = &mbox->menu[cellMenu];
    GtkWidget *item = mnset(ent, "_Cell");
    gtk_widget_set_name(item, "Cell");
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), item);
    GtkWidget *submenu = gtk_menu_new();
    gtk_widget_show(submenu);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), submenu);

    ent = &mbox->menu[cellMenuPush];
    item = miset(ent, "_Push", "<Alt>G");
    g_object_set_data(G_OBJECT(item), MIDX, voidptr cellMenuPush);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    gtk_widget_add_accelerator(item, "activate", accel_group, GDK_KEY_g,
        (GdkModifierType) (GDK_CONTROL_MASK|GDK_SHIFT_MASK),
        GTK_ACCEL_VISIBLE);
    check_separator(ent, submenu);

    ent = &mbox->menu[cellMenuPop];
    item = miset(ent, "P_op", "<Alt>B");
    g_object_set_data(G_OBJECT(item), MIDX, voidptr cellMenuPop);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    gtk_widget_add_accelerator(item, "activate", accel_group, GDK_KEY_b,
        (GdkModifierType) (GDK_CONTROL_MASK|GDK_SHIFT_MASK),
        GTK_ACCEL_VISIBLE);
    check_separator(ent, submenu);

    ent = &mbox->menu[cellMenuStabs];
    item = miset(ent, "_Symbol Tables", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr cellMenuStabs);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

    ent = &mbox->menu[cellMenuCells];
    item = miset(ent, "_Cells List", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr cellMenuCells);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

    ent = &mbox->menu[cellMenuTree];
    item = miset(ent, "Show _Tree", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr cellMenuTree);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);
}


void
GTKmenuConfig::instantiateEditMenu()
{
    // Edit menu
    MenuBox *mbox = MainMenu()->FindMainMenu(MMedit);
    if (!mbox || !mbox->menu)
        return;
    GtkAccelGroup *accel_group = GTKmenu::self()->AccelGroup();
    if (!accel_group)
        return;
    GtkWidget *menubar = GTKmenu::self()->MainMenu();
    if (!menubar)
        return;

    MenuEnt *ent = &mbox->menu[editMenu];
    GtkWidget *item = mnset(ent, "_Edit");
    gtk_widget_set_name(item, "Edit");
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), item);
    GtkWidget *submenu = gtk_menu_new();
    gtk_widget_show(submenu);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), submenu);

    ent = &mbox->menu[editMenuCedit];
    item = miset(ent, "_Enable Editing", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr editMenuCedit);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

    ent = &mbox->menu[editMenuEdSet];
    item = miset(ent, "Editing _Setup", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr editMenuEdSet);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

    ent = &mbox->menu[editMenuPcctl];
    item = miset(ent, "PCell C_ontrol", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr editMenuPcctl);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

    ent = &mbox->menu[editMenuCrcel];
    item = miset(ent, "Cre_ate Cell", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr editMenuCrcel);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

    ent = &mbox->menu[editMenuCrvia];
    item = miset(ent, "Create _Via", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr editMenuCrvia);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

    ent = &mbox->menu[editMenuFlatn];
    item = miset(ent, "_Flatten", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr editMenuFlatn);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

    ent = &mbox->menu[editMenuJoin];
    item = miset(ent, "_Join and Split", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr editMenuJoin);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

    ent = &mbox->menu[editMenuLexpr];
    item = miset(ent, "_Layer Expression", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr editMenuLexpr);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

    ent = &mbox->menu[editMenuPrpty];
    item = miset(ent, "Propert_ies", "<Alt>P");
    g_object_set_data(G_OBJECT(item), MIDX, voidptr editMenuPrpty);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

    ent = &mbox->menu[editMenuCprop];
    item = miset(ent, "_Cell Properties", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr editMenuCprop);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);
}


void
GTKmenuConfig::instantiateModifyMenu()
{
    // Modify menu
    MenuBox *mbox = MainMenu()->FindMainMenu(MMmod);
    if (!mbox || !mbox->menu)
        return;
    GtkAccelGroup *accel_group = GTKmenu::self()->AccelGroup();
    if (!accel_group)
        return;
    GtkWidget *menubar = GTKmenu::self()->MainMenu();
    if (!menubar)
        return;

    MenuEnt *ent = &mbox->menu[modfMenu];
    GtkWidget *item = mnset(ent, "_Modify");
    gtk_widget_set_name(item, "Modify");
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), item);
    GtkWidget *submenu = gtk_menu_new();
    gtk_widget_show(submenu);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), submenu);

    ent = &mbox->menu[modfMenuUndo];
    item = miset(ent, "_Undo", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr modfMenuUndo);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

    ent = &mbox->menu[modfMenuRedo];
    item = miset(ent, "_Redo", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr modfMenuRedo);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

    ent = &mbox->menu[modfMenuDelet];
    item = miset(ent, "_Delete", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr modfMenuDelet);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

    ent = &mbox->menu[modfMenuEundr];
    item = miset(ent, "Erase U_nder", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr modfMenuEundr);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

    ent = &mbox->menu[modfMenuMove];
    item = miset(ent, "_Move", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr modfMenuMove);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

    ent = &mbox->menu[modfMenuCopy];
    item = miset(ent, "Cop_y", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr modfMenuCopy);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

    ent = &mbox->menu[modfMenuStrch];
    item = miset(ent, "_Stretch", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr modfMenuStrch);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

    ent = &mbox->menu[modfMenuChlyr];
    item = miset(ent, "Chan_ge Layer", "<control>L");
    g_object_set_data(G_OBJECT(item), MIDX, voidptr modfMenuChlyr);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    gtk_widget_add_accelerator(item, "activate", accel_group, GDK_KEY_l,
        GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    check_separator(ent, submenu);

    ent = &mbox->menu[modfMenuMClcg];
    item = miset(ent, "Set _Layer Chg Mode", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr modfMenuMClcg);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);
}


void
GTKmenuConfig::instantiateViewMenu()
{
    // View menu
    MenuBox *mbox = MainMenu()->FindMainMenu(MMview);
    if (!mbox || !mbox->menu)
        return;
    GtkAccelGroup *accel_group = GTKmenu::self()->AccelGroup();
    if (!accel_group)
        return;
    GtkWidget *menubar = GTKmenu::self()->MainMenu();
    if (!menubar)
        return;

    MenuEnt *ent = &mbox->menu[viewMenu];
    GtkWidget *item = mnset(ent, "_View");
    gtk_widget_set_name(item, "View");
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), item);
    GtkWidget *submenu = gtk_menu_new();
    gtk_widget_show(submenu);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), submenu);

    ent = &mbox->menu[viewMenuView];
    item = miset(ent, "Vie_w", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr viewMenuView);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    check_separator(ent, submenu);

    // Initialize the popup menu.
    // set data "menu" -> menu in the buttons
    // set data "callb" -> callback function
    // set data "data" -> callback data
    GtkWidget *popup = GTKmenu::self()->new_popup_menu(item,
        XM()->ViewList(), G_CALLBACK(vimenu_proc), 0);
    if (popup) {
        gtk_widget_set_name(popup, "ViewSubmenu");
        g_object_set_data(G_OBJECT(item), "menu", popup);
        g_object_set_data(G_OBJECT(item), "callb", (void*)vimenu_proc);

        // Set up the mapping so that the accelerator for the
        // "view" button will activate the "full" operation from
        // the sub-menu.

        GList *contents = gtk_container_get_children(GTK_CONTAINER(popup));
        if (contents) {
            // the first item is the "full" button
            GtkWidget *fullbut = GTK_WIDGET(contents->data);
            mbox->menu[viewMenuView].alt_caller = fullbut;
            g_list_free(contents);
        }
        if (mbox->menu[viewMenuView].description) {
            gtk_widget_set_tooltip_text(item, 
                mbox->menu[viewMenuView].description);
        }
    }

    ent = &mbox->menu[viewMenuSced];
    set(ent, "E_lectrical", 0);
    if (ScedIf()->hasSced()) {
        item = new_item(ent);
        g_object_set_data(G_OBJECT(item), MIDX, voidptr viewMenuSced);
        gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
        g_signal_connect(G_OBJECT(item), "activate",
            G_CALLBACK(menu_handler), mbox->menu);
        check_separator(ent, submenu);
    }

    ent = &mbox->menu[viewMenuPhys];
    set(ent, "P_hysical", 0);
    if (ScedIf()->hasSced()) {
        item = new_item(ent);
        g_object_set_data(G_OBJECT(item), MIDX, voidptr viewMenuPhys);
        gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
        g_signal_connect(G_OBJECT(item), "activate",
            G_CALLBACK(menu_handler), mbox->menu);
        check_separator(ent, submenu);
    }

    ent = &mbox->menu[viewMenuExpnd];
    item = miset(ent, "_Expand", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr viewMenuExpnd);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

    ent = &mbox->menu[viewMenuZoom];
    item = miset(ent, "_Zoom", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr viewMenuZoom);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

    ent = &mbox->menu[viewMenuVport];
    item = miset(ent, "_Viewport", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr viewMenuVport);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

    ent = &mbox->menu[viewMenuPeek];
    item = miset(ent, "_Peek", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr viewMenuPeek);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

    ent = &mbox->menu[viewMenuCsect];
    item = miset(ent, "_Cross Section", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr viewMenuCsect);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

    ent = &mbox->menu[viewMenuRuler];
    item = miset(ent, "_Rulers", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr viewMenuRuler);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

    ent = &mbox->menu[viewMenuInfo];
    item = miset(ent, "_Info", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr viewMenuInfo);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

    ent = &mbox->menu[viewMenuAlloc];
    item = miset(ent, "_Allocation", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr viewMenuAlloc);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);
}


void
GTKmenuConfig::instantiateAttributesMenu()
{
    // Attributes menu
    MenuBox *mbox = MainMenu()->FindMainMenu(MMattr);
    if (!mbox || !mbox->menu)
        return;
    GtkAccelGroup *accel_group = GTKmenu::self()->AccelGroup();
    if (!accel_group)
        return;
    GtkWidget *menubar = GTKmenu::self()->MainMenu();
    if (!menubar)
        return;

    MenuEnt *ent = &mbox->menu[attrMenu];
    GtkWidget *item = mnset(ent, "_Attributes");
    gtk_widget_set_name(item, "Attributes");
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), item);
    GtkWidget *submenu = gtk_menu_new();
    gtk_widget_show(submenu);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), submenu);

    ent = &mbox->menu[attrMenuUpdat];
    item = miset(ent, "Save _Tech", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr attrMenuUpdat);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

    ent = &mbox->menu[attrMenuKeymp];
    item = miset(ent, "_Key Map", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr attrMenuKeymp);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

    ent = &mbox->menu[attrMenuMacro];
    item = miset(ent, "Define _Macro", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr attrMenuMacro);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

    ent = &mbox->menu[attrMenuMainWin];
    item = miset(ent, "Main _Window", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr attrMenuMainWin);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    check_separator(ent, submenu);
    instantiateAttrSubMenu(item);

    ent = &mbox->menu[attrMenuAttr];
    item = miset(ent, "Set _Attributes", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr attrMenuAttr);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

    ent = &mbox->menu[attrMenuDots];
    set(ent, "Connection _Dots", 0);
    if (ScedIf()->hasSced()) {
        item = new_item(ent);
        g_object_set_data(G_OBJECT(item), MIDX, voidptr attrMenuDots);
        gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
        g_signal_connect(G_OBJECT(item), "activate",
            G_CALLBACK(menu_handler), mbox->menu);
        check_separator(ent, submenu);
    }

    ent = &mbox->menu[attrMenuFont];
    item = miset(ent, "Set F_ont", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr attrMenuFont);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

    ent = &mbox->menu[attrMenuColor];
    item = miset(ent, "Set _Color", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr attrMenuColor);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

    ent = &mbox->menu[attrMenuFill];
    item = miset(ent, "Set _Fill", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr attrMenuFill);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

    ent = &mbox->menu[attrMenuEdlyr];
    item = miset(ent, "_Edit Layers", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr attrMenuEdlyr);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

    ent = &mbox->menu[attrMenuLpedt];
    item = miset(ent, "Edit Tech _Params", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr attrMenuLpedt);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);
}


void
GTKmenuConfig::instantiateAttrSubMenu(GtkWidget *root)
{
    MenuBox *mbox = MainMenu()->GetAttrSubMenu();
    if (!mbox || !mbox->menu)
        return;

    GtkWidget *submenu = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(root), submenu);

    MenuEnt *ent = &mbox->menu[subwAttrMenuFreez];
    GtkWidget *item = miset(ent, "Freeze _Display", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr subwAttrMenuFreez);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

    ent = &mbox->menu[subwAttrMenuCntxt];
    item = miset(ent, "Show Conte_xt in Push", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr subwAttrMenuCntxt);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

    ent = &mbox->menu[subwAttrMenuProps];
    item = miset(ent, "Show _Phys Properties", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr subwAttrMenuProps);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

    ent = &mbox->menu[subwAttrMenuLabls];
    item = miset(ent, "Show _Labels", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr subwAttrMenuLabls);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

    ent = &mbox->menu[subwAttrMenuLarot];
    item = miset(ent, "L_abel True Orient", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr subwAttrMenuLarot);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

    ent = &mbox->menu[subwAttrMenuCnams];
    item = miset(ent, "Show Cell _Names", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr subwAttrMenuCnams);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

    ent = &mbox->menu[subwAttrMenuCnrot];
    item = miset(ent, "Cell Na_me True Orient", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr subwAttrMenuCnrot);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

    ent = &mbox->menu[subwAttrMenuNouxp];
    item = miset(ent, "Don't Show _Unexpanded", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr subwAttrMenuNouxp);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

    ent = &mbox->menu[subwAttrMenuObjs];
    item = miset(ent, "Objects Shown", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr subwAttrMenuObjs);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    check_separator(ent, submenu);
    instantiateObjSubMenu(item);

    ent = &mbox->menu[subwAttrMenuTinyb];
    item = miset(ent, "Subthreshold _Boxes", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr subwAttrMenuNouxp);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

    ent = &mbox->menu[subwAttrMenuNosym];
    set(ent, "No Top _Symbolic", 0);
    if (ScedIf()->hasSced()) {
        item = new_item(ent);
        g_object_set_data(G_OBJECT(item), MIDX, voidptr subwAttrMenuNosym);
        gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
        g_signal_connect(G_OBJECT(item), "activate",
            G_CALLBACK(menu_handler), mbox->menu);
        check_separator(ent, submenu);
    }

    ent = &mbox->menu[subwAttrMenuGrid];
    item = miset(ent, "Set _Grid", "<control>G");
    g_object_set_data(G_OBJECT(item), MIDX, voidptr subwAttrMenuGrid);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);
}


void
GTKmenuConfig::instantiateObjSubMenu(GtkWidget *root)
{
    MenuBox *mbox = MainMenu()->GetObjSubMenu();
    if (!mbox || !mbox->menu)
        return;

    GtkWidget *submenu = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(root), submenu);
//    g_signal_connect_object(G_OBJECT(root), "event",
//        G_CALLBACK(button_press), G_OBJECT(submenu), (GConnectFlags)0);

    MenuEnt *ent = &mbox->menu[0];
    GtkWidget *item = miset(ent, "Boxes", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr 0);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

    ent = &mbox->menu[1];
    item = miset(ent, "Polys", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr 1);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

    ent = &mbox->menu[2];
    item = miset(ent, "Wires", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr 2);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

    ent = &mbox->menu[3];
    item = miset(ent, "Labels", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr 3);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);
}


void
GTKmenuConfig::instantiateConvertMenu()
{
    // Convert menu
    MenuBox *mbox = MainMenu()->FindMainMenu(MMconv);
    if (!mbox || !mbox->menu)
        return;
    GtkAccelGroup *accel_group = GTKmenu::self()->AccelGroup();
    if (!accel_group)
        return;
    GtkWidget *menubar = GTKmenu::self()->MainMenu();
    if (!menubar)
        return;

    MenuEnt *ent = &mbox->menu[cvrtMenu];
    GtkWidget *item = mnset(ent, "C_onvert");
    gtk_widget_set_name(item, "Convert");
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), item);
    GtkWidget *submenu = gtk_menu_new();
    gtk_widget_show(submenu);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), submenu);

    ent = &mbox->menu[cvrtMenuExprt];
    item = miset(ent, "_Export Cell Data", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr cvrtMenuExprt);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

    ent = &mbox->menu[cvrtMenuImprt];
    item = miset(ent, "_Import Cell Data", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr cvrtMenuImprt);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

    ent = &mbox->menu[cvrtMenuConvt];
    item = miset(ent, "_Format Conversion", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr cvrtMenuConvt);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

    ent = &mbox->menu[cvrtMenuAssem];
    item = miset(ent, "_Assemble Layout", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr cvrtMenuAssem);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

    ent = &mbox->menu[cvrtMenuDiff];
    item = miset(ent, "_Compare Layouts", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr cvrtMenuDiff);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

    ent = &mbox->menu[cvrtMenuCut];
    item = miset(ent, "C_ut and Export", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr cvrtMenuCut);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

    ent = &mbox->menu[cvrtMenuTxted];
    item = miset(ent, "_Text Editor", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr cvrtMenuTxted);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);
}


void
GTKmenuConfig::instantiateDRCMenu()
{
    // DRC menu
    MenuBox *mbox = MainMenu()->FindMainMenu(MMdrc);
    if (!mbox || !mbox->menu)
        return;
    GtkAccelGroup *accel_group = GTKmenu::self()->AccelGroup();
    if (!accel_group)
        return;
    GtkWidget *menubar = GTKmenu::self()->MainMenu();
    if (!menubar)
        return;

    MenuEnt *ent = &mbox->menu[drcMenu];
    GtkWidget *item = mnset(ent, "_DRC");
    gtk_widget_set_name(item, "DRC");
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), item);
    GtkWidget *submenu = gtk_menu_new();
    gtk_widget_show(submenu);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), submenu);

    ent = &mbox->menu[drcMenuLimit];
    item = miset(ent, "DRC _Setup", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr drcMenuLimit);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

    ent = &mbox->menu[drcMenuSflag];
    item = miset(ent, "Set Skip _Flags", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr drcMenuSflag);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

    ent = &mbox->menu[drcMenuIntr];
    item = miset(ent, "Enable _Interactive", "<Alt>I");
    g_object_set_data(G_OBJECT(item), MIDX, voidptr drcMenuIntr);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    gtk_widget_add_accelerator(item, "activate", accel_group, GDK_KEY_i,
        (GdkModifierType) (GDK_CONTROL_MASK|GDK_SHIFT_MASK),
        GTK_ACCEL_VISIBLE);
    check_separator(ent, submenu);

    ent = &mbox->menu[drcMenuNopop];
    item = miset(ent, "_No Pop Up Errors", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr drcMenuNopop);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

    ent = &mbox->menu[drcMenuCheck];
    item = miset(ent, "_Batch Check", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr drcMenuCheck);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

    ent = &mbox->menu[drcMenuPoint];
    item = miset(ent, "Check In Re_gion", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr drcMenuPoint);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

    ent = &mbox->menu[drcMenuClear];
    item = miset(ent, "_Clear Errors", "<Alt>R");
    g_object_set_data(G_OBJECT(item), MIDX, voidptr drcMenuClear);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    gtk_widget_add_accelerator(item, "activate", accel_group, GDK_KEY_r,
        (GdkModifierType) (GDK_CONTROL_MASK|GDK_SHIFT_MASK),
        GTK_ACCEL_VISIBLE);
    check_separator(ent, submenu);

    ent = &mbox->menu[drcMenuQuery];
    item = miset(ent, "_Query Errors", "<Alt>Q");
    g_object_set_data(G_OBJECT(item), MIDX, voidptr drcMenuQuery);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    gtk_widget_add_accelerator(item, "activate", accel_group, GDK_KEY_q,
        (GdkModifierType) (GDK_CONTROL_MASK|GDK_SHIFT_MASK),
        GTK_ACCEL_VISIBLE);
    check_separator(ent, submenu);

    ent = &mbox->menu[drcMenuErdmp];
    item = miset(ent, "_Dump Error File", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr drcMenuErdmp);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

    ent = &mbox->menu[drcMenuErupd];
    item = miset(ent, "_Update Highlighting", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr drcMenuErupd);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

    ent = &mbox->menu[drcMenuNext];
    item = miset(ent, "Show _Errors", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr drcMenuNext);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

//XXX Erlyr to Crlyr?
    ent = &mbox->menu[drcMenuErlyr];
    item = miset(ent, "Create _Layer", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr drcMenuErlyr);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

    ent = &mbox->menu[drcMenuDredt];
    item = miset(ent, "Edit R_ules", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr drcMenuDredt);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);
}


void
GTKmenuConfig::instantiateExtractMenu()
{
    // Extract menu
    MenuBox *mbox = MainMenu()->FindMainMenu("extr");
    if (!mbox || !mbox->menu)
        return;
    GtkAccelGroup *accel_group = GTKmenu::self()->AccelGroup();
    if (!accel_group)
        return;
    GtkWidget *menubar = GTKmenu::self()->MainMenu();
    if (!menubar)
        return;

    MenuEnt *ent = &mbox->menu[extMenu];
    GtkWidget *item = mnset(ent, "E_xtract");
    gtk_widget_set_name(item, "Extract");
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), item);
    GtkWidget *submenu = gtk_menu_new();
    gtk_widget_show(submenu);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), submenu);

    ent = &mbox->menu[extMenuExcfg];
    item = miset(ent, "Extraction Set_up", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr extMenuExcfg);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

    ent = &mbox->menu[extMenuSel];
    item = miset(ent, "_Net Selections", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr extMenuSel);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

    ent = &mbox->menu[extMenuDvsel];
    item = miset(ent, "_Device Selections", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr extMenuDvsel);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

    ent = &mbox->menu[extMenuSourc];
    item = miset(ent, "_Source SPICE", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr extMenuSourc);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

    ent = &mbox->menu[extMenuExset];
    item = miset(ent, "Source P_hysical", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr extMenuExset);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

    ent = &mbox->menu[extMenuPnet];
    item = miset(ent, "Dump Ph_ys Netlist", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr extMenuPnet);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

    ent = &mbox->menu[extMenuEnet];
    item = miset(ent, "Dump E_lec Netlist", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr extMenuEnet);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

    ent = &mbox->menu[extMenuLvs];
    item = miset(ent, "Dump L_VS", 0);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr extMenuLvs);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

    ent = &mbox->menu[extMenuExC];
    item = miset(ent, "Extract _C", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr extMenuExC);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

    ent = &mbox->menu[extMenuExLR];
    item = miset(ent, "Extract L_R", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr extMenuExLR);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);
}


void
GTKmenuConfig::instantiateUserMenu()
{
    // User menu
    MenuBox *mbox = MainMenu()->FindMainMenu(MMuser);
    if (!mbox || !mbox->menu)
        return;
    GtkAccelGroup *accel_group = GTKmenu::self()->AccelGroup();
    if (!accel_group)
        return;
    GtkWidget *menubar = GTKmenu::self()->MainMenu();
    if (!menubar)
        return;

    MenuEnt *ent = &mbox->menu[userMenu];
    GtkWidget *item = mnset(ent, "_User");
    gtk_widget_set_name(item, "User");
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), item);
    GtkWidget *submenu = gtk_menu_new();
    gtk_widget_show(submenu);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), submenu);

    ent = &mbox->menu[userMenuDebug];
    item = miset(ent, "_Debugger", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr userMenuDebug);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

    ent = &mbox->menu[userMenuHash];
    item = miset(ent, "_Rehash", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr userMenuHash);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);
}


void
GTKmenuConfig::instantiateHelpMenu()
{
    // Extract menu
    MenuBox *mbox = MainMenu()->FindMainMenu(MMhelp);
    if (!mbox || !mbox->menu)
        return;
    GtkAccelGroup *accel_group = GTKmenu::self()->AccelGroup();
    if (!accel_group)
        return;
    GtkWidget *menubar = GTKmenu::self()->MainMenu();
    if (!menubar)
        return;

    MenuEnt *ent = &mbox->menu[helpMenu];
    GtkWidget *item = mnset(ent, "_Help");
    gtk_widget_set_name(item, "Help");
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), item);
    GtkWidget *submenu = gtk_menu_new();
    gtk_widget_show(submenu);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), submenu);

    ent = &mbox->menu[helpMenuHelp];
    item = miset(ent, "_Help", "<control>H");
    g_object_set_data(G_OBJECT(item), MIDX, voidptr helpMenuHelp);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

    ent = &mbox->menu[helpMenuMultw];
    item = miset(ent, "_Multi-Window", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr helpMenuMultw);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

    ent = &mbox->menu[helpMenuAbout];
    item = miset(ent, "_About", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr helpMenuAbout);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

    ent = &mbox->menu[helpMenuNotes];
    item = miset(ent, "_Release Notes", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr helpMenuNotes);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

    ent = &mbox->menu[helpMenuLogs];
    item = miset(ent, "Log _Files", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr helpMenuLogs);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

    ent = &mbox->menu[helpMenuDebug];
    item = miset(ent, "_Logging", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr helpMenuDebug);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);
}


void
GTKmenuConfig::instantiatePhysSideButtonMenu()
{
    // Physical button menu.
    MenuBox *mbox = MainMenu()->GetPhysButtonMenu();
    if (!mbox || !mbox->menu)
        return;

    MenuEnt *ent = &mbox->menu[btnPhysMenu];
    set(ent, 0, 0);

    ent = &mbox->menu[btnPhysMenuXform];
    set(ent, "Xform", 0);
    set_btn(ent, btnPhysMenuXform, (gpointer)menu_handler);

    ent = &mbox->menu[btnPhysMenuPlace];
    set(ent, "Place", 0);
    set_btn(ent, btnPhysMenuPlace, (gpointer)menu_handler);

    ent = &mbox->menu[btnPhysMenuLabel];
    set(ent, "Labels", 0);
    set_btn(ent, btnPhysMenuLabel, (gpointer)menu_handler);

    ent = &mbox->menu[btnPhysMenuLogo];
    set(ent, "Logo", 0);
    set_btn(ent, btnPhysMenuLogo, (gpointer)menu_handler);

    ent = &mbox->menu[btnPhysMenuBox];
    set(ent, "Boxes", 0);
    set_btn(ent, btnPhysMenuBox, (gpointer)menu_handler);

    ent = &mbox->menu[btnPhysMenuPolyg];
    set(ent, "Polygons", 0);
    set_btn(ent, btnPhysMenuPolyg, (gpointer)menu_handler);

    ent = &mbox->menu[btnPhysMenuWire];
    set(ent, "Wires", 0);
    set_btn(ent, btnPhysMenuWire, (gpointer)menu_handler);

    ent = &mbox->menu[btnPhysMenuStyle];
    set(ent, "Style", 0);

    const char **spm = get_style_pixmap();
    GtkWidget *button = gtk_NewPixmapButton(spm, ent->menutext, false);
    GtkWidget *menu = GTKmenu::self()->new_popup_menu(0,
        EditIf()->styleList(), G_CALLBACK(stmenu_proc), 0);
    if (menu) {
        gtk_widget_set_name(menu, "StyleMenu");
        g_signal_connect(G_OBJECT(button), "button-press-event",
            G_CALLBACK(popup_btn_proc), menu);
    }
    if (ent->description)
        gtk_widget_set_tooltip_text(button, ent->description); 
    gtk_widget_show(button);
    gtk_widget_set_name(button, ent->menutext);
    ent->cmd.caller = button;

    ent = &mbox->menu[btnPhysMenuRound];
    set(ent, "Round", 0);
    set_btn(ent, btnPhysMenuRound, (gpointer)menu_handler);

    ent = &mbox->menu[btnPhysMenuDonut];
    set(ent, "Donut", 0);
    set_btn(ent, btnPhysMenuDonut, (gpointer)menu_handler);

    ent = &mbox->menu[btnPhysMenuArc];
    set(ent, "Arc", 0);
    set_btn(ent, btnPhysMenuArc, (gpointer)menu_handler);

    ent = &mbox->menu[btnPhysMenuSides];
    set(ent, "Sides", 0);
    set_btn(ent, btnPhysMenuSides, (gpointer)menu_handler);

    ent = &mbox->menu[btnPhysMenuXor];
    set(ent, "Xor", 0);
    set_btn(ent, btnPhysMenuXor, (gpointer)menu_handler);

    ent = &mbox->menu[btnPhysMenuBreak];
    set(ent, "Break", 0);
    set_btn(ent, btnPhysMenuBreak, (gpointer)menu_handler);

    ent = &mbox->menu[btnPhysMenuErase];
    set(ent, "Erase", 0);
    set_btn(ent, btnPhysMenuErase, (gpointer)menu_handler);

    ent = &mbox->menu[btnPhysMenuPut];
    set(ent, "Put", 0);
    set_btn(ent, btnPhysMenuPut, (gpointer)menu_handler);

    ent = &mbox->menu[btnPhysMenuSpin];
    set(ent, "Spin", 0);
    set_btn(ent, btnPhysMenuSpin, (gpointer)menu_handler);
}

void
GTKmenuConfig::instantiateElecSideButtonMenu()
{
    // Electrical button menu.
    MenuBox *mbox = MainMenu()->GetElecButtonMenu();
    if (!mbox || !mbox->menu)
        return;

    MenuEnt *ent = &mbox->menu[btnElecMenu];
    set(ent, 0, 0);

    ent = &mbox->menu[btnElecMenuXform];
    set(ent, "Xform", 0);
    set_btn(ent, btnElecMenuXform, (gpointer)menu_handler);

    ent = &mbox->menu[btnElecMenuPlace];
    set(ent, "Place", 0);
    set_btn(ent, btnElecMenuPlace, (gpointer)menu_handler);

    ent = &mbox->menu[btnElecMenuDevs];
    set(ent, "Devices", 0);
    set_btn(ent, btnElecMenuDevs, (gpointer)menu_handler);

    ent = &mbox->menu[btnElecMenuShape];
    set(ent, "Shape", 0);

    GtkWidget *button = gtk_NewPixmapButton(ent->xpm, ent->menutext, false);
    GtkWidget *menu = GTKmenu::self()->new_popup_menu(0,
        ScedIf()->shapesList(), G_CALLBACK(shmenu_proc), 0);
    if (menu) {
        gtk_widget_set_name(menu, "ShapeMenu");
        g_signal_connect(G_OBJECT(button), "button-press-event",
            G_CALLBACK(popup_btn_proc), menu);
    }
    if (ent->description)
        gtk_widget_set_tooltip_text(button, ent->description); 
    gtk_widget_show(button);
    gtk_widget_set_name(button, ent->menutext);
    ent->cmd.caller = button;

    ent = &mbox->menu[btnElecMenuWire];
    set(ent, "Wire", 0);
    set_btn(ent, btnElecMenuWire, (gpointer)menu_handler);

    ent = &mbox->menu[btnElecMenuLabel];
    set(ent, "Label", 0);
    set_btn(ent, btnElecMenuLabel, (gpointer)menu_handler);

    ent = &mbox->menu[btnElecMenuErase];
    set(ent, "Erase", 0);
    set_btn(ent, btnElecMenuErase, (gpointer)menu_handler);

    ent = &mbox->menu[btnElecMenuBreak];
    set(ent, "Break", 0);
    set_btn(ent, btnElecMenuBreak, (gpointer)menu_handler);

    ent = &mbox->menu[btnElecMenuSymbl];
    set(ent, "Symbolic", 0);
    set_btn(ent, btnElecMenuSymbl, (gpointer)menu_handler);

    ent = &mbox->menu[btnElecMenuNodmp];
    set(ent, "Node Map", 0);
    set_btn(ent, btnElecMenuNodmp, (gpointer)menu_handler);

    ent = &mbox->menu[btnElecMenuSubct];
    set(ent, "Subcircuit", 0);
    set_btn(ent, btnElecMenuSubct, (gpointer)menu_handler);

    ent = &mbox->menu[btnElecMenuTerms];
    set(ent, "Show Terms", 0);
    set_btn(ent, btnElecMenuTerms, (gpointer)menu_handler);

    ent = &mbox->menu[btnElecMenuSpCmd];
    set(ent, "Command", 0);
    set_btn(ent, btnElecMenuSpCmd, (gpointer)menu_handler);

    ent = &mbox->menu[btnElecMenuRun];
    set(ent, "Run", 0);
    set_btn(ent, btnElecMenuRun, (gpointer)menu_handler);

    ent = &mbox->menu[btnElecMenuDeck];
    set(ent, "Dump Deck", 0);
    set_btn(ent, btnElecMenuDeck, (gpointer)menu_handler);

    ent = &mbox->menu[btnElecMenuPlot];
    set(ent, "Plot", 0);
    set_btn(ent, btnElecMenuPlot, (gpointer)menu_handler);

    ent = &mbox->menu[btnElecMenuIplot];
    set(ent, "Intr Plot", 0);
    set_btn(ent, btnElecMenuIplot, (gpointer)menu_handler);
}


void
GTKmenuConfig::instantiateSubwViewMenu(int wnum, GtkWidget *menubar,
    GtkAccelGroup *accel_group)
{
    // Subwin View Menu
    MenuBox *mbox = MainMenu()->FindSubwMenu(MMview, wnum);
    if (!mbox || !mbox->menu)
        return;
    if (!menubar || !accel_group)
        return;

    MenuEnt *ent = &mbox->menu[subwViewMenu];
    GtkWidget *item = mnset(ent, "_View");
    gtk_widget_set_name(item, "View");
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), item);
    GtkWidget *submenu = gtk_menu_new();
    gtk_widget_show(submenu);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), submenu);

    ent = &mbox->menu[subwViewMenuView];
    item = miset(ent, "_View", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr subwViewMenuView);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    check_separator(ent, submenu);

    GtkWidget *popup = GTKmenu::self()->new_popup_menu(item,
        XM()->ViewList(), G_CALLBACK(vimenu_proc), (void*)(intptr_t)wnum);
    if (popup) {
        g_object_set_data(G_OBJECT(item), "menu", popup);
        g_object_set_data(G_OBJECT(item), "callb", (void*)vimenu_proc);
        g_object_set_data(G_OBJECT(item), "data",(void*)(intptr_t)wnum);

        // Set up the mapping so that the accelerator for the
        // "view" button will activate the "full" operation from
        // the sub-menu.

        GList *contents = gtk_container_get_children(GTK_CONTAINER(popup));
        if (contents) {
            // the first item is the "full" button
            GtkWidget *fullbut = GTK_WIDGET(contents->data);
            mbox->menu[subwViewMenuView].alt_caller = fullbut;
            g_list_free(contents);
        }
    }

    ent = &mbox->menu[subwViewMenuSced];
    set(ent, "E_lectrical", 0);
    if (ScedIf()->hasSced()) {
        item = new_item(ent);
        g_object_set_data(G_OBJECT(item), MIDX, voidptr subwViewMenuSced);
        gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
        g_signal_connect(G_OBJECT(item), "activate",
            G_CALLBACK(menu_handler), mbox->menu);
        check_separator(ent, submenu);
    }

    ent = &mbox->menu[subwViewMenuPhys];
    set(ent, "P_hysical", 0);
    if (ScedIf()->hasSced()) {
        item = new_item(ent);
        g_object_set_data(G_OBJECT(item), MIDX, voidptr subwViewMenuPhys);
        gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
        g_signal_connect(G_OBJECT(item), "activate",
            G_CALLBACK(menu_handler), mbox->menu);
        check_separator(ent, submenu);
    }

    ent = &mbox->menu[subwViewMenuExpnd];
    item = miset(ent, "_Expand", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr subwViewMenuExpnd);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

    ent = &mbox->menu[subwViewMenuZoom];
    item = miset(ent, "_Zoom", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr subwViewMenuZoom);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

    ent = &mbox->menu[subwViewMenuWdump];
    item = miset(ent, "_Dump To File", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr subwViewMenuWdump);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

    ent = &mbox->menu[subwViewMenuLshow];
    item = miset(ent, "_Show Location", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr subwViewMenuLshow);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

    ent = &mbox->menu[subwViewMenuSwap];
    item = miset(ent, "Swap With _Main", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr subwViewMenuSwap);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

    ent = &mbox->menu[subwViewMenuLoad];
    item = miset(ent, "Load _New", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr subwViewMenuLoad);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

    ent = &mbox->menu[subwViewMenuCancl];
    item = miset(ent, "_Quit", "<control>Q");
    g_object_set_data(G_OBJECT(item), MIDX, voidptr subwViewMenuCancl);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    gtk_widget_add_accelerator(item, "activate", accel_group, GDK_KEY_q,
        GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    check_separator(ent, submenu);
}


void
GTKmenuConfig::instantiateSubwAttrMenu(int wnum, GtkWidget *menubar,
    GtkAccelGroup *accel_group)
{
    // Subwin Attr Menu
    MenuBox *mbox = MainMenu()->FindSubwMenu(MMattr, wnum);
    if (!mbox || !mbox->menu)
        return;
    if (!menubar || !accel_group)
        return;

    MenuEnt *ent = &mbox->menu[subwAttrMenu];
    GtkWidget *item = mnset(ent, "_Attributes");
    gtk_widget_set_name(item, "Attributes");
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), item);
    GtkWidget *submenu = gtk_menu_new();
    gtk_widget_show(submenu);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), submenu);

    ent = &mbox->menu[subwAttrMenuFreez];
    item = miset(ent, "Freeze _Display", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr subwAttrMenuFreez);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

    ent = &mbox->menu[subwAttrMenuCntxt];
    item = miset(ent, "Show Conte_xt in Push", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr subwAttrMenuCntxt);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

    ent = &mbox->menu[subwAttrMenuProps];
    item = miset(ent, "Show _Phys Properties", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr subwAttrMenuProps);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

    ent = &mbox->menu[subwAttrMenuLabls];
    item = miset(ent, "Show _Labels", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr subwAttrMenuLabls);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

    ent = &mbox->menu[subwAttrMenuLarot];
    item = miset(ent, "L_abel True Orient", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr subwAttrMenuLarot);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

    ent = &mbox->menu[subwAttrMenuCnams];
    item = miset(ent, "Show Cell _Names", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr subwAttrMenuCnams);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

    ent = &mbox->menu[subwAttrMenuCnrot];
    item = miset(ent, "Cell Na_me True Orient", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr subwAttrMenuCnrot);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

    ent = &mbox->menu[subwAttrMenuNouxp];
    item = miset(ent, "Don't Show _Unexpanded", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr subwAttrMenuNouxp);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

    ent = &mbox->menu[subwAttrMenuObjs];
    item = miset(ent, "Objects Shown", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr subwAttrMenuObjs);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    check_separator(ent, submenu);
    instantiateSubwObjSubMenu(wnum,item);

    ent = &mbox->menu[subwAttrMenuTinyb];
    item = miset(ent, "Subthreshold _Boxes", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr subwAttrMenuTinyb);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

    ent = &mbox->menu[subwAttrMenuNosym];
    set(ent, "No Top _Symbolic", 0);
    if (ScedIf()->hasSced()) {
        item = new_item(ent);
        g_object_set_data(G_OBJECT(item), MIDX, voidptr subwAttrMenuNosym);
        gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
        g_signal_connect(G_OBJECT(item), "activate",
            G_CALLBACK(menu_handler), mbox->menu);
        check_separator(ent, submenu);
    }

    ent = &mbox->menu[subwAttrMenuGrid];
    item = miset(ent, "Set _Grid", "<control>G");
    g_object_set_data(G_OBJECT(item), MIDX, voidptr subwAttrMenuGrid);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    gtk_widget_add_accelerator(item, "activate", accel_group, GDK_KEY_g,
        GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    check_separator(ent, submenu);
}


void
GTKmenuConfig::instantiateSubwObjSubMenu(int wnum, GtkWidget *root)
{
    MenuBox *mbox = MainMenu()->GetSubwObjMenu(wnum);
    if (!mbox || !mbox->menu)
        return;

    GtkWidget *submenu = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(root), submenu);
//    g_signal_connect_object(G_OBJECT(root), "event",
//        G_CALLBACK(button_press), G_OBJECT(submenu), (GConnectFlags)0);

    MenuEnt *ent = &mbox->menu[0];
    GtkWidget *item = miset(ent, "Boxes", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr 0);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

    ent = &mbox->menu[1];
    item = miset(ent, "Polys", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr 1);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

    ent = &mbox->menu[2];
    item = miset(ent, "Wires", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr 2);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);

    ent = &mbox->menu[3];
    item = miset(ent, "Labels", 0);
    g_object_set_data(G_OBJECT(item), MIDX, voidptr 3);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    check_separator(ent, submenu);
}


void
GTKmenuConfig::instantiateSubwHelpMenu(int wnum, GtkWidget *menubar,
    GtkAccelGroup *accel_group)
{
    // Subwin Help Menu
    MenuBox *mbox = MainMenu()->FindSubwMenu(MMhelp, wnum);
    if (!mbox || !mbox->menu)
        return;
    if (!menubar || !accel_group)
        return;

    MenuEnt *ent = &mbox->menu[subwHelpMenu];
    GtkWidget *item = mnset(ent, "_Help");
    gtk_widget_set_name(item, "Help");
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), item);
    GtkWidget *submenu = gtk_menu_new();
    gtk_widget_show(submenu);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), submenu);

    ent = &mbox->menu[subwHelpMenuHelp];
    item = miset(ent, "_Help", "<control>H");
    g_object_set_data(G_OBJECT(item), MIDX, voidptr subwHelpMenuHelp);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), mbox->menu);
    gtk_widget_add_accelerator(item, "activate", accel_group, GDK_KEY_h,
        GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    check_separator(ent, submenu);
}


// Static function.
// This callback controls the dispatching of application
// commands from the menus.
//
void
GTKmenuConfig::menu_handler(GtkWidget *caller, void *client_data)
{
    unsigned int action =
        (uintptr_t)g_object_get_data(G_OBJECT(caller), MIDX);
    if (action == 0) {
        fprintf(stderr, "Null action in menu_handler\n");
        return;
    }
    // sanity check
    if (!client_data)
        return;
    MenuEnt *ent = static_cast<MenuEnt*>(client_data);
    if (MainMenu()->IsMainMenu(ent) || MainMenu()->IsButtonMenu(ent) ||
            MainMenu()->IsMiscMenu(ent) || MainMenu()->IsSubwMenu(ent)) {
        unsigned i;
        for (i = 0; ent[i].entry; i++) ;
        if (action >= i)
            return;
    }
    else
        return;

    ent += action;
    if (ent->alt_caller) {
        // Spurious call from menu pop-up.  This handler should only
        // be called through the alt_caller.
        return;
    }

    ent->cmd.caller = caller;  // should already be set
    if (!ent->cmd.wdesc)
        ent->cmd.wdesc = DSP()->MainWdesc();

    // If a modifier key is down, the key will be grabbed, and if text
    // editing mode is entered somehow the up event is lost.  E.g,
    // press Shift and click logo.  Windows can't be moved until Shift
    // is pressed and released.  Call gtk_keyboard_ungrab to avoid
    // this.
    gdk_keyboard_ungrab(0);

    char buf[128];
    if (XM()->IsDoingHelp()) {
        int x, y;
        GdkModifierType mstate;
        gdk_window_get_pointer(GTKdev::self()->DefaultFocusWin(), &x, &y,
            &mstate);
        bool state = MainMenu()->GetStatus(caller);

        const char *s;
        if (ent->is_dynamic()) {
            snprintf(buf, sizeof(buf), "user:%s", ent->entry);
            s = buf;
        }
        else {
            s = ent->entry;
            if (!strcmp(s, MenuHELP)) {
                // quit help
                XM()->QuitHelp();
                return;
            }
            snprintf(buf, sizeof(buf), "xic:%s", s);
            s = buf;
        }
        if (!(mstate & GDK_SHIFT_MASK)) {
            DSPmainWbag(PopUpHelp(s))
            if (ent->flags & ME_TOGGLE) {
                // put the button back the way it was
                state = !state;
                MainMenu()->SetStatus(caller, state);
            }
            return;
        }
    }
    if (ent->is_dynamic()) {
        // script from user menu
        if (!ent->is_menu() && !XM()->DbgLoad(ent)) {

            EV()->InitCallback();
            // Putting the call in a timeout proc allows the current
            // command to return and finish before the new one starts
            // (see below).
            g_timeout_add(50, user_cmd_proc, ent);
        }
        return;
    }

    bool call_on_up;
    if (!MainMenu()->SetupCommand(ent, &call_on_up))
        return;

    if (ent->type == CMD_NOTSAFE) {

        // Terminate present command state.
        // We enable the coordinate export here.  This is a
        // convenience feature that allows the same ghost box to be
        // used in the next command.  Suppose you set up a box to
        // erase, perhaps with panning/zooming, but then click to
        // finish the operation.  Oops, we were in the boxes command,
        // so undo, then press the erase button.  The erase command
        // will have the same rectangle anchor already set.

        CmdState::SetEnableExport(true);
        EV()->InitCallback();
        CmdState::SetEnableExport(false);
        Selections.check(); // consistency check

        // Putting the call in a timeout proc allows the current
        // command to return and finish before the new one starts (see
        // below).

        if (MainMenu()->GetStatus((GtkWidget*)ent->cmd.caller))
            g_timeout_add(50, cmd_proc, ent);
        else if (call_on_up)
            g_timeout_add(50, cmd_proc, ent);
        return;
    }
    if (GTKmainwin::exists()) {
        if (ent->action) {
            GTKmainwin::self()->ShowGhost(ERASE);
            (*ent->action)(&ent->cmd);
            GTKmainwin::self()->ShowGhost(DISPLAY);
        }
    }
}


// The two functions below initiate commands.  They are called from a
// timeout rather than an idle loop to avoid problems like the
// following:  Assume that the previous command's esc procedure calls
// redisplay, so there is a redisplay idle pending.  The new command
// would add a second idle proc.  During the redisplay, the
// CheckForInterrupt calls will start the second idle proc, before
// drawing is complete.  If the second idle proc starts a command like
// Edit, which blocks, then we are stuck in "busy" mode with no way
// out.
//
// Below, we wait until the drawing is complete ("not busy") before
// allowing the command to be launched.

// Static function.
int
GTKmenuConfig::user_cmd_proc(void *arg)
{
    if (GTKpkg::self()->IsBusy())
        return (true);
    DSP()->SetInterrupt(DSPinterNone);
    MenuEnt *ent = (MenuEnt*)arg;
    const char *entry = ent->menutext;
    // entry is the same as m->entry, but contains the menu path
    // for submenu items
    char buf[128];
    if (lstring::strdirsep(entry)) {
        // from submenu, add a distinguishing prefix to avoid confusion with
        // file path
        snprintf(buf, sizeof(buf), "%s%s", SCR_LIBCODE, entry);
        entry = buf;
    }
    SIfile *sfp;
    stringlist *wl;
    XM()->OpenScript(entry, &sfp, &wl);
    if (sfp || wl) {
        // Make sure that there is a current cell before calling the
        // script.  If we create a cell and it remains empty and
        // unconnected, we will delete it.

        bool cell_created = false;
        CDs *cursd = CurCell();
        if (!cursd) {
            if (!CD()->ReopenCell(Tstring(DSP()->CurCellName()),
                    DSP()->CurMode())) {
                Log()->ErrorLog(mh::Processing, Errs()->get_error());
                return (false);
            }
            cell_created = true;
            cursd = CurCell();
        }
        EditIf()->ulListCheck(ent->entry, cursd, false);
        SI()->Interpret(sfp, wl, 0, 0);
        if (sfp)
            delete sfp;
        EditIf()->ulCommitChanges(true);
        if (cell_created && cursd->isEmpty() && !cursd->isSubcell())
            delete cursd;
    }
    return (false);
}


// Static function.
int
GTKmenuConfig::cmd_proc(void *arg)
{
    if (GTKpkg::self()->IsBusy())
        return (true);
    DSP()->SetInterrupt(DSPinterNone);
    MenuEnt *ent = (MenuEnt*)arg;
    if (GTKmainwin::self()) {
        if (ent->action) {
            GTKmainwin::self()->ShowGhost(ERASE);
            (*ent->action)(&ent->cmd);
            GTKmainwin::self()->ShowGhost(DISPLAY);
        }
    }
    CmdState::SetExported(false);
    return (false);
}


// Static function.
// Edit menu handler.
//
void
GTKmenuConfig::edmenu_proc(GtkWidget *caller, void*)
{
    if (XM()->IsDoingHelp()) {
        int x, y;
        GdkModifierType mstate;
        gdk_window_get_pointer(GTKdev::self()->DefaultFocusWin(), &x, &y,
            &mstate);
        if (!(mstate & GDK_SHIFT_MASK)) {
            DSPmainWbag(PopUpHelp("xic:open"))
            return;
        }
    }
    const char *string = MainMenu()->GetLabel(caller);
    if (!string || !*string)
        return;

    int x, y;
    GdkModifierType mstate;
    gdk_window_get_pointer(GTKdev::self()->DefaultFocusWin(), &x, &y, &mstate);
    XM()->HandleOpenCellMenu(string, (mstate & GDK_SHIFT_MASK));
}


// Static function.
// View menu handler.
//
void
GTKmenuConfig::vimenu_proc(GtkWidget *caller, void *client_data)
{
    if (XM()->IsDoingHelp()) {
        int x, y;
        GdkModifierType mstate;
        gdk_window_get_pointer(GTKdev::self()->DefaultFocusWin(), &x, &y,
            &mstate);
        if (!(mstate & GDK_SHIFT_MASK)) {
            DSPmainWbag(PopUpHelp("xic:view"))
            return;
        }
    }
    const char *string = MainMenu()->GetLabel(caller);
    if (!string || !*string)
        return;
    int wnum = (intptr_t)client_data;
    if (wnum < 0 || wnum >= DSP_NUMWINS)
        return;
    WindowDesc *wdesc = DSP()->Window(wnum);
    if (wdesc)
        wdesc->SetView(string);
}


// Static function.
// Style menu handler
//
void
GTKmenuConfig::stmenu_proc(GtkWidget *caller, void*)
{
    const char *string = MainMenu()->GetLabel(caller);
    if (!string || !*string)
        return;
    if (!EditIf()->styleList()) 
        return;
    int i = 0;
    for (const char *const *s = EditIf()->styleList(); *s; s++, i++) {
        if (!strcmp(*s, string))
            break;
    }

    if (XM()->IsDoingHelp()) {
        int x, y;
        GdkModifierType mstate;
        gdk_window_get_pointer(GTKdev::self()->DefaultFocusWin(), &x, &y,
            &mstate);
        if (!(mstate & GDK_SHIFT_MASK)) {
            if (i == 0)
                DSPmainWbag(PopUpHelp("xic:width"))
            else if (i == 1)
                DSPmainWbag(PopUpHelp("xic:end_flush"))
            else if (i == 2)
                DSPmainWbag(PopUpHelp("xic:end_round"))
            else if (i == 3)
                DSPmainWbag(PopUpHelp("xic:end_extend"))
            return;
        }
    }

    EditIf()->setWireAttribute((WsType)i);

    MenuBox *mbox = MainMenu()->GetPhysButtonMenu();
    if (mbox && mbox->menu) {
        GtkWidget *item = GTK_WIDGET(mbox->menu[btnPhysMenuStyle].cmd.caller);
        if (item) {
            const char **spm = get_style_pixmap();
            GdkPixbuf *pb = gdk_pixbuf_new_from_xpm_data(spm);
            GtkWidget *img = gtk_image_new_from_pixbuf(pb);
            g_object_unref(pb);
            gtk_widget_show(img);
            GtkWidget *oldw = gtk_bin_get_child(GTK_BIN(item));
            if (oldw)
                gtk_widget_destroy(oldw);
            gtk_container_add(GTK_CONTAINER(item), img);
        }
    }
}


// Static function.
// Shape menu handler.
//
void
GTKmenuConfig::shmenu_proc(GtkWidget *caller, void*)
{
    if (!ScedIf()->shapesList())
        return;
    const char *string = MainMenu()->GetLabel(caller);
    if (!string || !*string)
        return;
    if (XM()->IsDoingHelp()) {
        int x, y;
        GdkModifierType mstate;
        gdk_window_get_pointer(GTKdev::self()->DefaultFocusWin(), &x, &y,
            &mstate);
        if (!(mstate & GDK_SHIFT_MASK)) {
            char buf[64];
            snprintf(buf, sizeof(buf), "shapes:%s", string);
            DSPmainWbag(PopUpHelp(buf))
            return;
        }
    }
    EV()->InitCallback();
    if (!strcmp(string, "sides")) {
        CmdDesc cmd;
        cmd.caller = caller;
        cmd.wdesc = DSP()->MainWdesc();
        EditIf()->sidesExec(&cmd);
        return;
    }
    int i = 0;
    for (const char *const *s = ScedIf()->shapesList(); *s; s++, i++)
        if (!strcmp(*s, string))
            break;
    ScedIf()->addShape(i);
}


namespace {
    // Positioning function for pop-up menus.
    //
    void
    pos_func(GtkMenu*, int *x, int *y, gboolean *pushin, void *data)
    {
        *pushin = true;
        GtkWidget *btn = GTK_WIDGET(data);
        GTKdev::self()->Location(btn, x, y);
    }
}


// Static function.
// Button-press handler to produce pop-up menu.
//
int
GTKmenuConfig::popup_btn_proc(GtkWidget *widget, GdkEvent *event, void *arg)
{
    gtk_menu_popup(GTK_MENU(arg), 0, 0, pos_func, widget, event->button.button,
        event->button.time);
    return (true);
}


// Static function.
const char **
GTKmenuConfig::get_style_pixmap()
{
    if (EditIf()->getWireStyle() == CDWIRE_FLUSH)
        return (style_f_xpm);
    else if (EditIf()->getWireStyle() == CDWIRE_ROUND)
        return (style_r_xpm);
    // CDWIRE_EXTEND
    return (style_e_xpm);
}


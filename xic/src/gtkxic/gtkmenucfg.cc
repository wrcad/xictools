
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
#include "gtkinlines.h"
#include "bitmaps/style_e.xpm"
#include "bitmaps/style_f.xpm"
#include "bitmaps/style_r.xpm"


bool
cMain::MenuItemLocation(int, int*, int*)
{
    return (0);
}


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
#ifdef UseItemFactsory
#define HANDLER (GtkItemFactoryCallback)menu_handler
#else
#define HANDLER G_CALLBACK(menu_handler)
#endif


gtkMenuConfig *gtkMenuConfig::instancePtr = 0;

gtkMenuConfig::gtkMenuConfig()
{
    if (instancePtr) {
        fprintf(stderr,
            "Singleton class gtkMenuConfig already instantiated.\n");
        exit(1);
    }
    instancePtr = this;
}

// Instantiate
gtkMenuConfig _cfg_;


// Private static error exit.
//
void
gtkMenuConfig::on_null_ptr()
{
    fprintf(stderr,
        "Singleton class GtlMenuConfig used before instantiated.\n");
    exit(1);
}


// This is the GtkItemFactoryEntry structure used to generate new menus.
// Item 1: The menu path. The letter after the underscore indicates an
//         accelerator key once the menu is open.
// Item 2: The accelerator key for the entry
// Item 3: The callback function.
// Item 4: The callback action.  This changes the parameters with
//         which the function is called.  The default is 0.
// Item 5: The item type, used to define what kind of an item it is.
//         Here are the possible values:

//         NULL               -> "<Item>"
//         ""                 -> "<Item>"
//         "<Title>"          -> create a title item
//         "<Item>"           -> create a simple item
//         "<CheckItem>"      -> create a check item
//         "<ToggleItem>"     -> create a toggle item
//         "<RadioItem>"      -> create a radio item
//         <path>             -> path of a radio item to link against
//         "<Separator>"      -> create a separator
//         "<Branch>"         -> create an item to hold sub items
//         "<LastBranch>"     -> create a right justified branch

namespace {
#ifdef UseItemFactory
    // Subclass GtkItemFactoryEntry so can use constructor
    //
    struct if_entry : public GtkItemFactoryEntry
    {
        if_entry()
            {
                path = 0;
                accelerator = 0;
                callback = 0;
                callback_action = 0;
                item_type = 0;
            }
        if_entry(const char *p, const char *a, GtkItemFactoryCallback cb,
            int act, const char *it)
            {
                path = (char*)p;
                accelerator = (char*)a;
                callback = cb;
                callback_action = act;
                item_type = (char*)it;
            }
    };

#else

    // This is toolkit-specific so not a method.
    inline GtkWidget *
    miset(MenuEnt *ent, const char *text, const char *ac, const char *it = 0)
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
        if (ent->is_toggle())
            item = gtk_check_menu_item_new_with_mnemonic(ent->menutext);
        else
            item = gtk_menu_item_new_with_mnemonic(ent->menutext);
        gtk_widget_show(item);
        return (item);
    }

    inline GtkWidget *new_item(MenuEnt *ent)
    {
        GtkWidget *item;
        if (ent->is_toggle())
            item = gtk_check_menu_item_new_with_mnemonic(ent->menutext);
        else
            item = gtk_menu_item_new_with_mnemonic(ent->menutext);
        gtk_widget_show(item);
        return (item);
    }
#endif

    // This is toolkit-specific so not a method.
    inline void
    set(MenuEnt &ent, const char *text, const char *ac, const char *it = 0)
    {
        ent.menutext = text;
        ent.accel = ac;
        if (it)
            ent.item = it;
        else if (ent.is_menu())
            ent.item = "<Branch>";
        else if (ent.is_toggle())
            ent.item = "<CheckItem>";
    }

}


void
gtkMenuConfig::instantiateMainMenus()
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
gtkMenuConfig::instantiateTopButtonMenu()
{
    // Horizontal button line at top of main window.
    MenuBox *mbox = Menu()->GetMiscMenu();
    if (mbox && mbox->menu) {
        set(mbox->menu[miscMenu], 0, 0);
        set(mbox->menu[miscMenuMail], "Mail", 0);
        set(mbox->menu[miscMenuLtvis], "LTvisib", 0);
        set(mbox->menu[miscMenuLpal], "Palette", 0);
        set(mbox->menu[miscMenuSetcl], "SetCL", 0);
        // This is really weird, in Ubuntu-18 a widget named
        // "SelPanel" gets a black background from somewhere.  Maybe
        // from the theme?
        // set(mbox->menu[miscMenuSelcp], "SelPanel", 0);
        set(mbox->menu[miscMenuSelcp], "SelCP", 0);
        set(mbox->menu[miscMenuDesel], "Desel", 0);
        set(mbox->menu[miscMenuRdraw], "Rdraw", 0);

        for (int i = 1; i < miscMenu_END; i++) {
            MenuEnt *ent = &mbox->menu[i];
            GtkWidget *button = new_pixmap_button(ent->xpm, ent->menutext,
                ent->is_toggle());
            if (ent->is_set())
                Menu()->Select(button);
            g_signal_connect(G_OBJECT(button), "clicked",
                G_CALLBACK(top_btnmenu_callback), (void*)(intptr_t)i);
            if (ent->description)
                gtk_widget_set_tooltip_text(button, ent->description);
            gtk_widget_show(button);
            gtk_widget_set_name(button, ent->menutext);
            ent->cmd.caller = button;
        }
    }
}


void
gtkMenuConfig::instantiateSideButtonMenus()
{
    // Physical button menu.
    MenuBox *mbox = Menu()->GetPhysButtonMenu();
    if (mbox && mbox->menu) {

        set(mbox->menu[btnPhysMenu], 0, 0);
        set(mbox->menu[btnPhysMenuLabel], "Labels", 0);
        set(mbox->menu[btnPhysMenuLogo], "Logo", 0);
        set(mbox->menu[btnPhysMenuBox], "Boxes", 0);
        set(mbox->menu[btnPhysMenuPolyg], "Polygons", 0);
        set(mbox->menu[btnPhysMenuWire], "Wires", 0);
        set(mbox->menu[btnPhysMenuStyle], "Style", 0);
        set(mbox->menu[btnPhysMenuRound], "Round", 0);
        set(mbox->menu[btnPhysMenuDonut], "Donut", 0);
        set(mbox->menu[btnPhysMenuArc], "Arc", 0);
        set(mbox->menu[btnPhysMenuSides], "Sides", 0);
        set(mbox->menu[btnPhysMenuXor], "Xor", 0);
        set(mbox->menu[btnPhysMenuBreak], "Break", 0);
        set(mbox->menu[btnPhysMenuErase], "Erase", 0);
        set(mbox->menu[btnPhysMenuPut], "Put", 0);
        set(mbox->menu[btnPhysMenuSpin], "Spin", 0);

        for (int i = 1; i < btnPhysMenu_END; i++) {
            MenuEnt *ent = &mbox->menu[i];
            GtkWidget *button;
            if (i == btnPhysMenuStyle) {

                const char **spm = get_style_pixmap();
                button = new_pixmap_button(spm, ent->menutext, false);
                GtkWidget *menu = gtkMenu()->new_popup_menu(0,
                    EditIf()->styleList(), G_CALLBACK(stmenu_proc), 0);
                if (menu) {
                    gtk_widget_set_name(menu, "StyleMenu");
                    g_signal_connect(G_OBJECT(button), "button-press-event",
                        G_CALLBACK(popup_btn_proc), menu);
                }
            }
            else {
                button = new_pixmap_button(ent->xpm, ent->menutext,
                    ent->is_toggle());
                if (ent->is_set())
                    Menu()->Select(button);
                g_signal_connect(G_OBJECT(button), "clicked",
                    G_CALLBACK(btnmenu_callback), (void*)(intptr_t)i);
            }
            if (ent->description)
                gtk_widget_set_tooltip_text(button, ent->description); 
            gtk_widget_show(button);
            gtk_widget_set_name(button, ent->menutext);
            ent->cmd.caller = button;
        }
    }

    // Electrical button menu.
    mbox = Menu()->GetElecButtonMenu();
    if (mbox && mbox->menu) {

        set(mbox->menu[btnElecMenu], 0, 0);
        set(mbox->menu[btnElecMenuDevs], "Devices", 0);
        set(mbox->menu[btnElecMenuShape], "Shape", 0);
        set(mbox->menu[btnElecMenuWire], "Wire", 0);
        set(mbox->menu[btnElecMenuLabel], "Label", 0);
        set(mbox->menu[btnElecMenuErase], "Erase", 0);
        set(mbox->menu[btnElecMenuBreak], "Break", 0);
        set(mbox->menu[btnElecMenuSymbl], "Symbolic", 0);
        set(mbox->menu[btnElecMenuNodmp], "Node Map", 0);
        set(mbox->menu[btnElecMenuSubct], "Subcircuit", 0);
        set(mbox->menu[btnElecMenuTerms], "Show Terms", 0);
        set(mbox->menu[btnElecMenuSpCmd], "Command", 0);
        set(mbox->menu[btnElecMenuRun], "Run", 0);
        set(mbox->menu[btnElecMenuDeck], "Dump Deck", 0);
        set(mbox->menu[btnElecMenuPlot], "Plot", 0);
        set(mbox->menu[btnElecMenuIplot], "Intr Plot", 0);

        for (int i = 1; i < btnElecMenu_END; i++) {
            MenuEnt *ent = &mbox->menu[i];
            GtkWidget *button;
            if (i == btnElecMenuShape) {
                button = new_pixmap_button(ent->xpm, ent->menutext, false);
                GtkWidget *menu = gtkMenu()->new_popup_menu(0,
                    ScedIf()->shapesList(), G_CALLBACK(shmenu_proc), 0);
                if (menu) {
                    gtk_widget_set_name(menu, "ShapeMenu");
                    g_signal_connect(G_OBJECT(button), "button-press-event",
                        G_CALLBACK(popup_btn_proc), menu);
                }
            }
            else {
                button = new_pixmap_button(ent->xpm, ent->menutext,
                    ent->is_toggle());
                if (ent->is_set())
                    Menu()->Select(button);
                g_signal_connect(G_OBJECT(button), "clicked",
                    G_CALLBACK(btnmenu_callback), (void*)(intptr_t)i);
            }
            if (ent->description)
                gtk_widget_set_tooltip_text(button, ent->description); 
            gtk_widget_show(button);
            gtk_widget_set_name(button, ent->menutext);
            ent->cmd.caller = button;
        }
    }
}


void
gtkMenuConfig::instantiateSubwMenus(int wnum, GtkItemFactory *item_factory)
{
#ifdef UseItemFactory
    if (!item_factory)
        return;
    if_entry mi[50];
#endif

    // Subwin View Menu
    MenuBox *mbox = Menu()->FindSubwMenu("view", wnum);
    if (mbox && mbox->menu) {

        set(mbox->menu[subwViewMenu], "/_View", 0);
        set(mbox->menu[subwViewMenuView], "/View/_View", 0);
        set(mbox->menu[subwViewMenuSced], "/View/E_lectrical", 0);
        set(mbox->menu[subwViewMenuPhys], "/View/P_hysical", 0);
        set(mbox->menu[subwViewMenuExpnd], "/View/_Expand", 0);
        set(mbox->menu[subwViewMenuZoom], "/View/_Zoom", 0);
        set(mbox->menu[subwViewMenuWdump], "/View/_Dump To File", 0);
        set(mbox->menu[subwViewMenuLshow], "/View/_Show Location", 0);
        set(mbox->menu[subwViewMenuSwap], "/View/Swap With _Main", 0);
        set(mbox->menu[subwViewMenuLoad], "/View/Load _New", 0);
        set(mbox->menu[subwViewMenuCancl], "/View/_Quit", "<control>Q");

#ifdef UseItemFactory
        int j = 0, i = 0;
        for (MenuEnt *ent = mbox->menu; ent->entry; ent++) {
            if (i == subwViewMenuSced || i == subwViewMenuPhys) {
                if (!ScedIf()->hasSced()) {
                    i++;
                    continue;
                }
            }
            mi[j++] = if_entry(ent->menutext, ent->accel,
                i && i != subwViewMenuView ? HANDLER : 0, i, ent->item);
            if (ent->is_separator())
                mi[j++] = if_entry("/View/sep", 0, 0, NOTMAPPED,
                    "<Separator>");
            i++;
        }
        GtkItemFactoryEntry *menu = new GtkItemFactoryEntry[j];
        memcpy(menu, mi, j*sizeof(GtkItemFactoryEntry));
        make_entries(item_factory, menu, j, mbox->menu, i);
        delete [] menu;

        GtkWidget *btn = gtk_item_factory_get_widget(item_factory,
            "/View/View");
        GtkWidget *popup = gtkMenu()->new_popup_menu(btn,
            XM()->ViewList(), G_CALLBACK(vimenu_proc),
            (void*)(intptr_t)wnum);
        if (popup) {
            gtk_object_set_data(GTK_OBJECT(btn), "menu", popup);
            gtk_object_set_data(GTK_OBJECT(btn), "callb", (void*)vimenu_proc);
            gtk_object_set_data(GTK_OBJECT(btn), "data",(void*)(intptr_t)wnum);

            // Set up the mapping so that the accelerator for the
            // "view" button will activate the "full" operation from
            // the sub-menu.

            GList *contents = gtk_container_children(GTK_CONTAINER(popup));
            if (contents) {
                // the first item is the "full" button
                GtkWidget *fullbut = GTK_WIDGET(contents->data);
                mbox->menu[subwViewMenuView].alt_caller = fullbut;
                g_list_free(contents);
            }
        }

        // name the menubar object
        GtkWidget *widget = gtk_item_factory_get_item(item_factory, "/View");
        if (widget)
            gtk_widget_set_name(widget, "View");
#else
#endif
    }

    // Subwin Attr Menu
    mbox = Menu()->FindSubwMenu("attr", wnum);
    if (mbox && mbox->menu) {

        set(mbox->menu[subwAttrMenu], "/_Attributes", 0);
        set(mbox->menu[subwAttrMenuFreez], "/Attributes/Freeze _Display", 0);
        set(mbox->menu[subwAttrMenuCntxt], "/Attributes/Show Conte_xt in Push",
            0);
        set(mbox->menu[subwAttrMenuProps], "/Attributes/Show _Phys Properties",
            0);
        set(mbox->menu[subwAttrMenuLabls], "/Attributes/Show _Labels", 0);
        set(mbox->menu[subwAttrMenuLarot], "/Attributes/L_abel True Orient", 0);
        set(mbox->menu[subwAttrMenuCnams], "/Attributes/Show Cell _Names", 0);
        set(mbox->menu[subwAttrMenuCnrot], "/Attributes/Cell Na_me True Orient",
            0);
        set(mbox->menu[subwAttrMenuNouxp],"/Attributes/Don't Show _Unexpanded",
            0);
        set(mbox->menu[subwAttrMenuObjs],"/Attributes/Objects Shown", 0);
        set(mbox->menu[subwAttrMenuTinyb], "/Attributes/Subthreshold _Boxes",
            0);
        set(mbox->menu[subwAttrMenuNosym], "/Attributes/No Top _Symbolic", 0);
        set(mbox->menu[subwAttrMenuGrid], "/Attributes/Set _Grid", "<control>G");

#ifdef UseItemFactory
        int j = 0, i = 0;
        for (MenuEnt *ent = mbox->menu; ent->entry; ent++) {
            if (i == subwAttrMenuNosym) {
                if (!ScedIf()->hasSced()) {
                    i++;
                    continue;
                }
            }
            mi[j++] = if_entry(ent->menutext, ent->accel,
                i && i != subwAttrMenuObjs ? HANDLER : 0, i, ent->item);
            if (ent->is_separator())
                mi[j++] = if_entry("/Attributes/sep", 0, 0, NOTMAPPED,
                    "<Separator>");
            i++;
        }
        GtkItemFactoryEntry *menu = new GtkItemFactoryEntry[j];
        memcpy(menu, mi, j*sizeof(GtkItemFactoryEntry));
        make_entries(item_factory, menu, j, mbox->menu, i);
        delete [] menu;

        mbox = Menu()->GetSubwObjMenu(wnum);
        if (mbox && mbox->menu) {
            set(mbox->menu[0],
                "/Attributes/Objects Shown/Boxes", 0);
            set(mbox->menu[1],
                "/Attributes/Objects Shown/Polys", 0);
            set(mbox->menu[2],
                "/Attributes/Objects Shown/Wires", 0);
            set(mbox->menu[3],
                "/Attributes/Objects Shown/Labels", 0);

            j = 0, i = 0;
            for (MenuEnt *ent = mbox->menu; ent->entry && ent->menutext;
                    ent++) {
                mi[j++] = if_entry(ent->menutext, ent->accel,
                    HANDLER, i, ent->item);
                i++;
            }
            menu = new GtkItemFactoryEntry[j];
            memcpy(menu, mi, j*sizeof(GtkItemFactoryEntry));
            make_entries(item_factory, menu, j, mbox->menu, i);
            delete [] menu;
        }

        // name the menubar object
        GtkWidget *widget = gtk_item_factory_get_item(item_factory,
            "/Attributes");
        if (widget)
            gtk_widget_set_name(widget, "Attributes");
#else
#endif
    }

    // Subwin Help Menu
    mbox = Menu()->FindSubwMenu("help", wnum);
    if (mbox && mbox->menu) {

        set(mbox->menu[subwHelpMenu], "/_Help", 0, "<LastBranch>");
        set(mbox->menu[subwHelpMenuHelp], "/Help/_Help", "<control>H");

#ifdef UseItemFactory
        int j = 0, i = 0;
        for (MenuEnt *ent = mbox->menu; ent->entry; ent++) {
            mi[j++] = if_entry(ent->menutext, ent->accel, i ? HANDLER : 0,
                i, ent->item);
            if (ent->is_separator())
                mi[j++] = if_entry("/Help/sep", 0, 0, NOTMAPPED,
                    "<Separator>");
            i++;
        }
        GtkItemFactoryEntry *menu = new GtkItemFactoryEntry[j];
        memcpy(menu, mi, j*sizeof(GtkItemFactoryEntry));
        make_entries(item_factory, menu, j, mbox->menu, i);
        delete [] menu;

        // name the menubar object
        GtkWidget *widget = gtk_item_factory_get_item(item_factory, "/Help");
        if (widget)
            gtk_widget_set_name(widget, "Help");
    }
    switch_menu_mode(DSP()->CurMode(), wnum);
#else
#endif
}


void
gtkMenuConfig::updateDynamicMenus()
{
#ifdef UseItemFactory
    GtkItemFactory *item_factory = gtkMenu()->itemFactory;
    if (!item_factory)
        return;
    // Destroy the present user menu widget tree.
    {
        GtkWidget *mitem = gtk_item_factory_get_item(item_factory, "/User");
        if (!mitem)
            return;
        GtkWidget *menu = GTK_MENU_ITEM(mitem)->submenu;
        if (!menu)
            return;
        GList *gl = gtk_container_children(GTK_CONTAINER(menu));
        for (GList *l = gl; l; l = l->next)
            gtk_widget_destroy(GTK_WIDGET(l->data));
        g_list_free(gl);
    }
#else
#endif

    Menu()->RebuildDynamicMenus();

    if_entry mi[50];
    MenuBox *mbox = Menu()->FindMainMenu("user");
    if (mbox && mbox->menu && mbox->isDynamic()) {

        set(mbox->menu[userMenu], "/_User", 0);
        set(mbox->menu[userMenuDebug], "/User/_Debugger", 0);
        set(mbox->menu[userMenuHash], "/User/_Rehash", 0);

#ifdef UseItemFactory
        int i = 0, j = 0;
        for (MenuEnt *ent = mbox->menu; ent->entry && i < userMenu_END;
                ent++) {
            mi[j++] = if_entry(ent->menutext, ent->accel, i ? HANDLER : 0,
                i, ent->item);
            if (ent->is_separator())
                mi[j++] = if_entry("/User/sep", 0, 0, NOTMAPPED,
                    "<Separator>");
            i++;
        }

        GtkItemFactoryEntry *menu = new GtkItemFactoryEntry[j];
        memcpy(menu, mi, j*sizeof(GtkItemFactoryEntry));
        make_entries(item_factory, menu, j, mbox->menu, i);
        delete [] menu;

        // name the menubar object
        GtkWidget *widget = gtk_item_factory_get_item(item_factory,
            "/User");
        if (widget)
            gtk_widget_set_name(widget, "User");

        // Create new GtkMenuStrings.
        int cnt = 0;
        for (MenuEnt *ent = mbox->menu; ent->entry; ent++) {
            if (cnt >= userMenu_END) {
                ent->menutext = ent->description;
                if (ent->is_menu())
                    ent->item = "<Branch>";
            }
            cnt++;
        }

        stringlist *snew = 0;
        stringlist *sold = 0;

        // Add the dynamic entries.
        for (MenuEnt *ent = &mbox->menu[userMenu_END]; ent->entry; ent++) {
            GtkItemFactoryEntry entry;
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

            // Pop the stack until the old path is a prefix of the
            // present path.
            while (sold && !lstring::prefix(sold->string, ent->menutext)) {
                stringlist *sx = sold;
                sold = sold->next;
                sx->next = 0;
                stringlist::destroy(sx);
                sx = snew;
                snew = snew->next;
                sx->next = 0;
                stringlist::destroy(sx);
            }

            // Build the item factory path in buf.
            if (!snew) {
                strcpy(buf, ent->menutext);
                char *t = strrchr(buf, '/');
                *t = 0;
            }
            else
                strcpy(buf, snew->string);
            char *t = buf + strlen(buf);
            *t++ = '/';
            strcpy(t, ent->entry);
            // Strip out underscores.
            for ( ; *t; t++) {
                if (*t == '_')
                    *t = '-';
            }
            if (ent->is_menu()) {
                // Push the stack.
                sold = new stringlist(lstring::copy(ent->menutext), sold);
                snew = new stringlist(lstring::copy(buf), snew);
            }

            entry.path = buf;
            entry.accelerator = (gchar*)ent->accel;
            entry.item_type = (gchar*)ent->item;
            if (!entry.item_type) {
                entry.callback = HANDLER;
                entry.callback_action = (ent - mbox->menu);
            }
            else {
                entry.callback = 0;
                entry.callback_action = 0;
            }
            gtk_item_factory_create_item(item_factory, &entry,
                mbox->menu, 2);
            GtkWidget *btn = gtk_item_factory_get_widget(item_factory,
                entry.path);
            if (btn)
                ent->cmd.caller = btn;
        }
        stringlist::destroy(snew);
        stringlist::destroy(sold);
#else
#endif
    }
}


void
gtkMenuConfig::switch_menu_mode(DisplayMode mode, int wnum)
{
#ifdef UseItemFactory
    if (wnum == 0) {
        GtkItemFactory *ifact = gtkMenu()->itemFactory;
        if (!ifact)
            return;
        GtkWidget *bp = gtk_item_factory_get_item(ifact, "/View/Physical");
        GtkWidget *be = gtk_item_factory_get_item(ifact, "/View/Electrical");
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
        GtkWidget *ns = gtk_item_factory_get_widget(ifact,
            "/Attributes/Main Window/No Top Symbolic");
        if (ns) {
            if (mode == Physical)
                gtk_widget_hide(ns);
            else
                gtk_widget_show(ns);
        }

        // Desensitize the DRC menu in electrical mode.
        GtkWidget *mitem = gtk_item_factory_get_item(ifact, "/DRC");
        if (mitem)
            gtk_widget_set_sensitive(mitem, (DSP()->CurMode() == Physical));

        // swap button menu
        if (mode == Physical) {
            if (gtkMenu()->btnElecMenuWidget)
                gtk_widget_hide(gtkMenu()->btnElecMenuWidget);
            if (gtkMenu()->btnPhysMenuWidget)
                gtk_widget_show(gtkMenu()->btnPhysMenuWidget);
        }
        else {
            if (gtkMenu()->btnPhysMenuWidget)
                gtk_widget_hide(gtkMenu()->btnPhysMenuWidget);
            if (gtkMenu()->btnElecMenuWidget)
                gtk_widget_show(gtkMenu()->btnElecMenuWidget);
        }
    }
    else {
        if (wnum >= DSP_NUMWINS)
            return;
        if (!DSP()->Window(wnum))
            return;

        MenuEnt *ent = gtkMenu()->mm_subw_menus[wnum]->menu;
        GtkItemFactory *ifact =
            gtk_item_factory_from_widget(GTK_WIDGET((ent+3)->cmd.caller));
            // ent+3 is the first element that reliably returns the ifact
        if (ifact) {
            GtkWidget *be = gtk_item_factory_get_widget(ifact,
                "/View/Electrical");
            GtkWidget *bp = gtk_item_factory_get_widget(ifact,
                "/View/Physical");
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
            GtkWidget *ns = gtk_item_factory_get_widget(ifact,
                "/Attributes/No Top Symbolic");
            if (ns) {
                if (mode == Physical)
                    gtk_widget_hide(ns);
                else
                    gtk_widget_show(ns);
            }
        }
    }
#else
#endif
}


// Turn on/off sensitivity of all menus in the main window.
//
void
gtkMenuConfig::set_main_global_sens(bool sens)
{
    if (gtkMenu()->ButtonMenu(Physical))
        gtk_widget_set_sensitive(gtkMenu()->ButtonMenu(Physical), sens);
    if (gtkMenu()->ButtonMenu(Electrical))
        gtk_widget_set_sensitive(gtkMenu()->ButtonMenu(Electrical), sens);

#ifdef UseItemFacgtory
    GtkItemFactory *item_factory = gtkMenu()->itemFactory;
    if (!item_factory)
        return;
    for (const MenuList *ml = gtkMenu()->GetMainMenus(); ml; ml = ml->next) {
        if (!ml->menubox || !ml->menubox->menu)
            continue;
        MenuEnt *ent = ml->menubox->menu;

        char *t = gtkMenu()->strip_accel(ent->menutext);
        GtkWidget *widget = gtk_item_factory_get_item(item_factory, t);
        delete [] t;
        if (!widget)
            continue;

        if (lstring::ciprefix("view", ml->menubox->name)) {
            // for View Menu, keep Allocation button sensitive
            GtkWidget *menu = GTK_MENU_ITEM(widget)->submenu;
            if (menu) {
                GList *gl = gtk_container_children(GTK_CONTAINER(menu));
                for (GList *l = gl; l; l = l->next)
                    gtk_widget_set_sensitive(GTK_WIDGET(l->data), sens);
                g_list_free(gl);
            }
            widget = gtk_item_factory_get_item(item_factory,
                "/View/Allocation");
            if (widget)
                gtk_widget_set_sensitive(GTK_WIDGET(widget), true);
        }
        else if (lstring::ciprefix("attr", ml->menubox->name)) {
            // for Attributes Menu, keep Freeze button sensitive
            GtkWidget *menu = GTK_MENU_ITEM(widget)->submenu;
            if (menu) {
                GList *gl = gtk_container_children(GTK_CONTAINER(menu));
                for (GList *l = gl; l; l = l->next)
                    gtk_widget_set_sensitive(GTK_WIDGET(l->data), sens);
                g_list_free(gl);
            }
            widget = gtk_item_factory_get_item(item_factory,
                "/Attributes/Main Window");
            if (widget) {
                gtk_widget_set_sensitive(GTK_WIDGET(widget), true);
                menu = GTK_MENU_ITEM(widget)->submenu;
                if (menu) {
                    GList *gl = gtk_container_children(GTK_CONTAINER(menu));
                    for (GList *l = gl; l; l = l->next)
                        gtk_widget_set_sensitive(GTK_WIDGET(l->data), sens);
                    g_list_free(gl);

                    widget = gtk_item_factory_get_item(item_factory,
                        "/Attributes/Main Window/Freeze Display");
                    if (widget)
                        gtk_widget_set_sensitive(GTK_WIDGET(widget), true);
                }
            }
        }
        else
            gtk_widget_set_sensitive(widget, sens);
    }
#endif
}

#ifdef UseItemFactory
#else
namespace {
    void check_separator(MenuEnt *ent, GtkWidget *menu)
    {
        if (ent->issep()) {
            GtkWidget *item = gtk_separator_menu_item_new();
            gtk_widget_show(item);
            gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
        }
    }
}
#endif


void
gtkMenuConfig::instantiateFileMenu()
{
#ifdef UseItemFactory
    GtkItemFactory *item_factory = gtkMenu()->itemFactory;
    if (!item_factory)
        return;
    if_entry mi[50];

    // File memu
    MenuBox *mbox = Menu()->FindMainMenu("file");
    if (mbox && mbox->menu) {

        set(mbox->menu[fileMenu], "/_File", 0);
        set(mbox->menu[fileMenuFsel], "/File/F_ile Select", "<control>O");
        set(mbox->menu[fileMenuOpen], "/File/_Open", 0);
        set(mbox->menu[fileMenuSave], "/File/_Save", "<Alt>S");
        set(mbox->menu[fileMenuSaveAs], "/File/Save _As", "<control>S");
        set(mbox->menu[fileMenuSaveAsDev], "/File/Save As _Device", 0);
        set(mbox->menu[fileMenuHcopy], "/File/_Print", "<Alt>N");
        set(mbox->menu[fileMenuFiles], "/File/_Files List", 0);
        set(mbox->menu[fileMenuHier], "/File/_Hierarchy Digests", 0);
        set(mbox->menu[fileMenuGeom], "/File/_Geometry Digests", 0);
        set(mbox->menu[fileMenuLibs], "/File/_Libraries List", 0);
        set(mbox->menu[fileMenuOAlib], "/File/Open_Access Libs", 0);
        set(mbox->menu[fileMenuExit], "/File/_Quit", "<control>Q");

        int j = 0, i = 0;
        for (MenuEnt *ent = mbox->menu; ent->entry; ent++) {
            if (i == fileMenuSave) {
                if (!EditIf()->hasEdit()) {
                    i++;
                    continue;
                }
            }
            if (i == fileMenuOAlib) {
                if (!OAif()->hasOA()) {
                    i++;
                    continue;
                }
            }
            mi[j++] = if_entry(ent->menutext, ent->accel,
                i && i != fileMenuOpen ? HANDLER : 0, i, ent->item);
            if (ent->is_separator())
                mi[j++] = if_entry("/File/sep", 0, 0, NOTMAPPED, "<Separator>");
            i++;
        }
        GtkItemFactoryEntry *menu = new GtkItemFactoryEntry[j];
        memcpy(menu, mi, j*sizeof(GtkItemFactoryEntry));
        make_entries(item_factory, menu, j, mbox->menu, i);
        delete [] menu;

        // Initialize the popup menus
        // set data "menu" -> menu in the buttons
        // set data "callb" -> callback function
        // set data "data" -> callback data
        GtkWidget *btn = gtk_item_factory_get_widget(item_factory,
            "/File/Open");
        GtkWidget *popup = gtkMenu()->new_popup_menu(btn,
            XM()->OpenCellMenuList(), G_CALLBACK(edmenu_proc), 0);
        if (popup) {
            gtk_widget_set_name(popup, "EditSubmenu");
            gtk_object_set_data(GTK_OBJECT(btn), "menu", popup);
            gtk_object_set_data(GTK_OBJECT(btn), "callb", (void*)edmenu_proc);

            // Set up the mapping so that the accelerator for the
            // "open" button will activate the "new" operation from
            // the sub-menu.

            GList *contents = gtk_container_children(GTK_CONTAINER(popup));
            if (contents) {
                // the first item is the "new" button
                GtkWidget *newbut = GTK_WIDGET(contents->data);
                mbox->menu[fileMenuOpen].alt_caller = newbut;
                g_list_free(contents);
            }
            if (mbox->menu[fileMenuOpen].description) {
                gtk_widget_set_tooltip_text(btn, 
                    mbox->menu[fileMenuOpen].description);
            }
        }

        // name the menubar object
        GtkWidget *widget = gtk_item_factory_get_item(item_factory, "/File");
        if (widget)
            gtk_widget_set_name(widget, "File");
    }
#else
    // File menu
    MenuBox *mbox = Menu()->FindMainMenu("file");
    if (!mbox || !mbox->menu)
        return
    GtkAccelGroup *accel_group = gtkMenu()->AccelGroup();
    if (!accel_group)
        return;
    GtkWidget *menubar = gtkMenu()->MainMenu();
    if (!menubar)
        return;

    MenuEnt *ent = &mbox->menu[fileMenu];
    set(*ent, "_File", 0);

    GtkWidget *item = gtk_menu_item_new_with_mnemonic(ent->menutext);
    gtk_widget_set_name(item, "File");
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), item);
    GtkWidget *fileMenu = gtk_menu_new();
    gtk_widget_show(fileMenu);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), fileMenu);

    ent = &mbox->menu[fileMenuFsel];
    item = miset(ent, "F_ile Select", "<control>O");
    gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), (gpointer)(long)fileMenuFsel);
    gtk_widget_add_accelerator(item, "activate", accel_group, GDK_o,
        GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    check_separator(ent, fileMenu);

    ent = &mbox->menu[fileMenuOpen];
    item = miset(ent, "_Open", 0);
    gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), item);
    // g_signal_connect(G_OBJECT(item), "activate",
    //     G_CALLBACK(menu_handler), (gpointer)(long)fileMenuOpen);
    check_separator(ent, fileMenu);

    // Initialize the popup menus
    // set data "menu" -> menu in the buttons
    // set data "callb" -> callback function
    // set data "data" -> callback data
    GtkWidget *btn = item;
    GtkWidget *popup = gtkMenu()->new_popup_menu(btn,
        XM()->OpenCellMenuList(), G_CALLBACK(edmenu_proc), 0);
    if (popup) {
        gtk_widget_set_name(popup, "EditSubmenu");
        gtk_object_set_data(GTK_OBJECT(btn), "menu", popup);
        gtk_object_set_data(GTK_OBJECT(btn), "callb", (void*)edmenu_proc);

        // Set up the mapping so that the accelerator for the
        // "open" button will activate the "new" operation from
        // the sub-menu.

        GList *contents = gtk_container_children(GTK_CONTAINER(popup));
        if (contents) {
            // the first item is the "new" button
            GtkWidget *newbut = GTK_WIDGET(contents->data);
            mbox->menu[fileMenuOpen].alt_caller = newbut;
            g_list_free(contents);
        }
        if (mbox->menu[fileMenuOpen].description) {
            gtk_widget_set_tooltip_text(btn, 
                mbox->menu[fileMenuOpen].description);
        }
    }

    ent = &mbox->menu[fileMenuSave];
    set(*ent, "_Save", "<Alt>S");
    if (EditIf()->hasEdit()) {
        item = new_item(ent);
        gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), item);
        g_signal_connect(G_OBJECT(item), "activate",
            G_CALLBACK(menu_handler), (gpointer)(long)fileMenuSave);
    //    gtk_widget_add_accelerator(item, "activate", accel_group, GDK_s,
    //        GDK_ALT_MASK, GTK_ACCEL_VISIBLE);
    }
    check_separator(ent, fileMenu);

    ent = &mbox->menu[fileMenuSaveAs];
    item = miset(ent, "Save _As", "<control>S");
    gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(ed_open_proc), this);
    gtk_widget_add_accelerator(item, "activate", accel_group, GDK_s,
        G_CALLBACK(menu_handler), (gpointer)(long)fileMenuSaveAs);
    check_separator(ent, fileMenu);

    ent = &mbox->menu[fileMenuSaveAsDev];
    item = miset(ent, "Save As _Device", 0);
    gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), (gpointer)(long)fileMenuSaveAsDev);
    check_separator(ent, fileMenu);

    ent = &mbox->menu[fileMenuHcopy];
    item = miset(ent, "_Print", "<Alt>N");
    gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), (gpointer)(long)fileMenuHcopy);
//    gtk_widget_add_accelerator(item, "activate", accel_group, GDK_n,
//        GDK_ALTL_MASK, GTK_ACCEL_VISIBLE);
    check_separator(ent, fileMenu);

    ent = &mbox->menu[fileMenuFiles];
    item = miset(ent, "_Files List", 0);
    gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), (gpointer)(long)fileMenuFiles);
    check_separator(ent, fileMenu);

    ent = &mbox->menu[fileMenuHier];
    item = miset(ent, "_Hierarchy Digests", 0);
    gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), (gpointer)(long)fileMenuHier);
    check_separator(ent, fileMenu);

    ent = &mbox->menu[fileMenuGeom];
    item = miset(ent, "_Geometry Digests", 0);
    gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), (gpointer)(long)fileMenuGeom);
    check_separator(ent, fileMenu);

    ent = &mbox->menu[fileMenuLibs];
    item = miset(ent, "_Libraries List", 0);
    gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), (gpointer)(long)fileMenuLibs);
    check_separator(ent, fileMenu);

    ent = &mbox->menu[fileMenuOAlib];
    set(*ent, "Open_Access Libs", 0);
    if (OAif()->hasOA()) {
        item = new_item(ent);
        gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), item);
        g_signal_connect(G_OBJECT(item), "activate",
            G_CALLBACK(menu_handler), (gpointer)(long)fileMenuOAlib);
    }
    check_separator(ent, fileMenu);

    ent = &mbox->menu[fileMenuExit];
    item = miset(ent, "_Quit", "<control>Q");
    gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), (gpointer)(long)fileMenuExit);
    gtk_widget_add_accelerator(item, "activate", accel_group, GDK_q,
        GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    check_separator(ent, fileMenu);
#endif
}


void
gtkMenuConfig::instantiateCellMenu()
{
#ifdef UseItemFactory
    GtkItemFactory *item_factory = gtkMenu()->itemFactory;
    if (!item_factory)
        return;
    if_entry mi[50];

    // Cell menu
    MenuBox *mbox = Menu()->FindMainMenu("cell");
    if (mbox && mbox->menu) {
        set(mbox->menu[cellMenu], "/_Cell", 0);
        set(mbox->menu[cellMenuPush], "/Cell/_Push", "<Alt>G");
        set(mbox->menu[cellMenuPop], "/Cell/P_op", "<Alt>B");
        set(mbox->menu[cellMenuStabs], "/Cell/_Symbol Tables", 0);
        set(mbox->menu[cellMenuCells], "/Cell/_Cells List", 0);
        set(mbox->menu[cellMenuTree], "/Cell/Show _Tree", 0);

        int j = 0, i = 0;
        for (MenuEnt *ent = mbox->menu; ent->entry; ent++) {
            mi[j++] = if_entry(ent->menutext, ent->accel, i ? HANDLER : 0,
                i, ent->item);
            if (ent->is_separator())
                mi[j++] = if_entry("/Cell/sep", 0, 0, NOTMAPPED, "<Separator>");
            i++;
        }
        GtkItemFactoryEntry *menu = new GtkItemFactoryEntry[j];
        memcpy(menu, mi, j*sizeof(GtkItemFactoryEntry));
        make_entries(item_factory, menu, j, mbox->menu, i);
        delete [] menu;

        // name the menubar object
        GtkWidget *widget = gtk_item_factory_get_item(item_factory, "/Cell");
        if (widget)
            gtk_widget_set_name(widget, "Cell");
    }
#else
    // Cell menu
    MenuBox *mbox = Menu()->FindMainMenu("cell");
    if (!mbox || !mbox->menu)
        return
    GtkAccelGroup *accel_group = gtkMenu()->AccelGroup();
    if (!accel_group)
        return;
    GtkWidget *menubar = gtkMenu()->MainMenu();
    if (!menubar)
        return;

    MenuBox *ent = &mbox->menu[cellMenu];
    set(*ent, "_Cell", 0);

    GtkWidget *item = gtk_menu_item_new_with_mnemonic(ent->menutext);
    gtk_widget_set_name(item, "Cell");
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), item);
    GtkWidget *cellMenu = gtk_menu_new();
    gtk_widget_show(cellMenu);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), cellMenu);

    ent = &mbox->menu[cellMenuPush];
    item = miset(ent, "_Push", "<Alt>G");
    gtk_menu_shell_append(GTK_MENU_SHELL(cellMenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), (gpointer)(long)cellMenuPush);
//    gtk_widget_add_accelerator(item, "activate", accel_group, GDK_g,
//        GDK_ALT_MASK, GTK_ACCEL_VISIBLE);
    check_separator(ent, cellMenu);

    ent = &mbox->menu[cellMenuPop];
    item = miset(ent, "P_op", "<Alt>B");
    gtk_menu_shell_append(GTK_MENU_SHELL(cellMenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), (gpointer)(long)cellMenuPop);
//    gtk_widget_add_accelerator(item, "activate", accel_group, GDK_b,
//        GDK_ALT_MASK, GTK_ACCEL_VISIBLE);
    check_separator(ent, cellMenu);

    ent = &mbox->menu[cellMenuStabs];
    item = miset(ent, "_Symbol Tables", 0);
    gtk_menu_shell_append(GTK_MENU_SHELL(cellMenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), (gpointer)(long)cellMenuStabs);
    check_separator(ent, cellMenu);

    ent = &mbox->menu[cellMenuCells];
    item = miset(ent, "_Cells List", 0);
    gtk_menu_shell_append(GTK_MENU_SHELL(cellMenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), (gpointer)(long)cellMenuCells);
    check_separator(ent, cellMenu);

    ent = &mbox->menu[cellMenuTree];
    item = miset(ent, "Show _Tree", 0);
    gtk_menu_shell_append(GTK_MENU_SHELL(cellMenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), (gpointer)(long)cellMenuTree);
    check_separator(ent, cellMenu);
#endif
}


void
gtkMenuConfig::instantiateEditMenu()
{
#ifdef UseItemFactory
    GtkItemFactory *item_factory = gtkMenu()->itemFactory;
    if (!item_factory)
        return;
    if_entry mi[50];

    // Edit menu
    MenuBox *mbox = Menu()->FindMainMenu("edit");
    if (mbox && mbox->menu) {

        set(mbox->menu[editMenu], "/_Edit", 0);
        set(mbox->menu[editMenuCedit], "/Edit/_Enable Editing", 0);
        set(mbox->menu[editMenuEdSet], "/Edit/_Setup", 0);
        set(mbox->menu[editMenuPcctl], "/Edit/PCell C_ontrol", 0);
        set(mbox->menu[editMenuCrcel], "/Edit/Cre_ate Cell", 0);
        set(mbox->menu[editMenuCrvia], "/Edit/Create _Via", 0);
        set(mbox->menu[editMenuFlatn], "/Edit/_Flatten", 0);
        set(mbox->menu[editMenuJoin], "/Edit/_Join\\/Split", 0);
        set(mbox->menu[editMenuLexpr], "/Edit/_Layer Expression", 0);
        set(mbox->menu[editMenuPrpty], "/Edit/Propert_ies", "<Alt>P");
        set(mbox->menu[editMenuCprop], "/Edit/_Cell Properties", 0);

        int j = 0, i = 0;
        for (MenuEnt *ent = mbox->menu; ent->entry; ent++) {
            mi[j++] = if_entry(ent->menutext, ent->accel, i ? HANDLER : 0,
                i, ent->item);
            if (ent->is_separator())
                mi[j++] = if_entry("/Edit/sep", 0, 0, NOTMAPPED, "<Separator>");
            i++;
        }
        GtkItemFactoryEntry *menu = new GtkItemFactoryEntry[j];
        memcpy(menu, mi, j*sizeof(GtkItemFactoryEntry));
        make_entries(item_factory, menu, j, mbox->menu, i);
        delete [] menu;

        // name the menubar object
        GtkWidget *widget = gtk_item_factory_get_item(item_factory, "/Edit");
        if (widget)
            gtk_widget_set_name(widget, "Edit");
    }
#else
    // Edit menu
    MenuBox *mbox = Menu()->FindMainMenu("edit");
    if (!mbox || !mbox->menu)
        return
    GtkAccelGroup *accel_group = gtkMenu()->AccelGroup();
    if (!accel_group)
        return;
    GtkWidget *menubar = gtkMenu()->MainMenu();
    if (!menubar)
        return;

    MenoEnt *ent = &mbox->menu[editMenu];
    set(*ent, "_Edit", 0);

    GtkWidget *item = gtk_menu_item_new_with_mnemonic(ent->menutext);
    gtk_widget_set_name(item, "Edit");
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), item);
    GtkWidget *editMenu = gtk_menu_new();
    gtk_widget_show(editMenu);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), editMenu);

    ent = &mbox->menu[editMenuCedit];
    item = miset(ent, "_Enable Editing", 0);
    gtk_menu_shell_append(GTK_MENU_SHELL(editMenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), (gpointer)(long)editMenuCedit);
    check_separator(ent, editMenu);

    ent = &mbox->menu[editMenuEdSet];
    item = miset(ent, "_Setup", 0);
    gtk_menu_shell_append(GTK_MENU_SHELL(editMenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), (gpointer)(long)editMenuEdSet);
    check_separator(ent, editMenu);

    ent = &mbox->menu[editMenuPcctl];
    item = miset(ent, "PCell C_ontrol", 0);
    gtk_menu_shell_append(GTK_MENU_SHELL(editMenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), (gpointer)(long)editMenuPcctl);
    check_separator(ent, editMenu);

    ent = &mbox->menu[editMenuCrcel];
    item = miset(ent, "Cre_ate Cell", 0);
    gtk_menu_shell_append(GTK_MENU_SHELL(editMenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), (gpointer)(long)editMenuCrcel);
    check_separator(ent, editMenu);

    ent = &mbox->menu[editMenuCrvia];
    item = miset(ent, "Create _Via", 0);
    gtk_menu_shell_append(GTK_MENU_SHELL(editMenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), (gpointer)(long)editMenuCrvia);
    check_separator(ent, editMenu);

    ent = &mbox->menu[editMenuFlatn];
    item = miset(ent, "_Flatten", 0);
    gtk_menu_shell_append(GTK_MENU_SHELL(editMenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), (gpointer)(long)editMenuFlatn);
    check_separator(ent, editMenu);

    ent = &mbox->menu[editMenuJoin];
    item = miset(ent, "_Join\\/Split", 0);
    gtk_menu_shell_append(GTK_MENU_SHELL(editMenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), (gpointer)(long)editMenuJoin);
    check_separator(ent, editMenu);

    ent = &mbox->menu[editMenuLexpr];
    item = miset(ent, "_Layer Expression", 0);
    gtk_menu_shell_append(GTK_MENU_SHELL(editMenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), (gpointer)(long)editMenuLexpr);
    check_separator(ent, editMenu);

    ent = &mbox->menu[editMenuPrpty];
    item = miset(ent, "Propert_ies", "<Alt>P");
    gtk_menu_shell_append(GTK_MENU_SHELL(editMenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), (gpointer)(long)editMenuPrpty);
    check_separator(ent, editMenu);

    ent = &mbox->menu[editMenuCprop];
    item = miset(ent, "_Cell Properties", 0);
    gtk_menu_shell_append(GTK_MENU_SHELL(editMenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), (gpointer)(long)editMenuCprop);
    check_separator(ent, editMenu);
#endif
}


void
gtkMenuConfig::instantiateModifyMenu()
{
#ifdef UseItemFactory
    GtkItemFactory *item_factory = gtkMenu()->itemFactory;
    if (!item_factory)
        return;
    if_entry mi[50];

    // Modify menu
    MenuBox *mbox = Menu()->FindMainMenu("mod");
    if (mbox && mbox->menu) {

        set(mbox->menu[modfMenu], "/_Modify", 0);
        set(mbox->menu[modfMenuUndo], "/Modify/_Undo", 0);
        set(mbox->menu[modfMenuRedo], "/Modify/_Redo", 0);
        set(mbox->menu[modfMenuDelet], "/Modify/_Delete", 0);
        set(mbox->menu[modfMenuEundr], "/Modify/Erase U_nder", 0);
        set(mbox->menu[modfMenuMove], "/Modify/_Move", 0);
        set(mbox->menu[modfMenuCopy], "/Modify/Cop_y", 0);
        set(mbox->menu[modfMenuStrch], "/Modify/_Stretch", 0);
        set(mbox->menu[modfMenuChlyr], "/Modify/Chan_ge Layer", "<control>L");
        set(mbox->menu[modfMenuMClcg], "/Modify/Set _Layer Chg Mode", 0);

        int j = 0, i = 0;
        for (MenuEnt *ent = mbox->menu; ent->entry; ent++) {
            mi[j++] = if_entry(ent->menutext, ent->accel, i ? HANDLER : 0,
                i, ent->item);
            if (ent->is_separator())
                mi[j++] = if_entry("/Modify/sep", 0, 0, NOTMAPPED,
                    "<Separator>");
            i++;
        }
        GtkItemFactoryEntry *menu = new GtkItemFactoryEntry[j];
        memcpy(menu, mi, j*sizeof(GtkItemFactoryEntry));
        make_entries(item_factory, menu, j, mbox->menu, i);
        delete [] menu;

        // name the menubar object
        GtkWidget *widget = gtk_item_factory_get_item(item_factory, "/Modify");
        if (widget)
            gtk_widget_set_name(widget, "Modify");
    }
#else
    // Modify menu
    MenuBox *mbox = Menu()->FindMainMenu("mod");
    if (!mbox || !mbox->menu)
        return
    GtkAccelGroup *accel_group = gtkMenu()->AccelGroup();
    if (!accel_group)
        return;
    GtkWidget *menubar = gtkMenu()->MainMenu();
    if (!menubar)
        return;

    MenuEnt *ent = &mbox->menu[modfMenu];
    set(*ent, "_Modify", 0);

    GtkWidget *item = gtk_menu_item_new_with_mnemonic(ent->menutext);
    gtk_widget_set_name(item, "Modify");
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), item);
    GtkWidget *modMenu = gtk_menu_new();
    gtk_widget_show(modMenu);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), modMenu);

    ent = &mbox->menu[modfMenuUndo];
    item = miset(ent, "_Undo", 0);
    gtk_menu_shell_append(GTK_MENU_SHELL(modMenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), (gpointer)(long)modMenuUndo);
    check_separator(ent, modMenu);

    ent = &mbox->menu[modfMenuRedo];
    item = miset(ent, "_Redo", 0);
    gtk_menu_shell_append(GTK_MENU_SHELL(modMenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), (gpointer)(long)modMenuRedo);
    check_separator(ent, modMenu);

    ent = &mbox->menu[modfMenuDelet];
    item = miset(ent, "_Delete", 0);
    gtk_menu_shell_append(GTK_MENU_SHELL(modMenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), (gpointer)(long)modMenuDelet);
    check_separator(ent, modMenu);

    ent = &mbox->menu[modfMenuEundr];
    item = miset(ent, "Erase U_nder", 0);
    gtk_menu_shell_append(GTK_MENU_SHELL(modMenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), (gpointer)(long)modMenuEundr);
    check_separator(ent, modMenu);

    ent = &mbox->menu[modfMenuMove];
    item = miset(ent, "_Move", 0);
    gtk_menu_shell_append(GTK_MENU_SHELL(modMenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), (gpointer)(long)modMenuMove);
    check_separator(ent, modMenu);

    ent = &mbox->menu[modfMenuCopy];
    item = miset(ent, "Cop_y", 0);
    gtk_menu_shell_append(GTK_MENU_SHELL(modMenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), (gpointer)(long)modMenuCopy);
    check_separator(ent, modMenu);

    ent = &mbox->menu[modfMenuStrch];
    item = miset(ent, "_Stretch", 0);
    gtk_menu_shell_append(GTK_MENU_SHELL(modMenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), (gpointer)(long)modMenuStrch);
    check_separator(ent, modMenu);

    ent = &mbox->menu[modfMenuChlyr];
    item = miset(ent, "Chan_ge Layer", "<control>L");
    gtk_menu_shell_append(GTK_MENU_SHELL(modMenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), (gpointer)(long)modMenuChlyr);
    gtk_widget_add_accelerator(item, "activate", accel_group, GDK_l,
        GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    check_separator(ent, modMenu);

    ent = &mbox->menu[modfMenuMClcg];
    item = miset(ent, "Set _Layer Chg Mode", 0);
    gtk_menu_shell_append(GTK_MENU_SHELL(modMenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), (gpointer)(long)modMenuMClcg);
    check_separator(ent, modMenu);
#endif
}


void
gtkMenuConfig::instantiateViewMenu()
{
#ifdef UseItemFactory
    GtkItemFactory *item_factory = gtkMenu()->itemFactory;
    if (!item_factory)
        return;
    if_entry mi[50];

    // View menu
    MenuBox *mbox = Menu()->FindMainMenu("view");
    if (mbox && mbox->menu) {

        set(mbox->menu[viewMenu], "/_View", 0);
        set(mbox->menu[viewMenuView], "/View/Vie_w", 0);
        set(mbox->menu[viewMenuSced], "/View/E_lectrical", 0);
        set(mbox->menu[viewMenuPhys], "/View/P_hysical", 0);
        set(mbox->menu[viewMenuExpnd], "/View/_Expand", 0);
        set(mbox->menu[viewMenuZoom], "/View/_Zoom", 0);
        set(mbox->menu[viewMenuVport], "/View/_Viewport", 0);
        set(mbox->menu[viewMenuPeek], "/View/_Peek", 0);
        set(mbox->menu[viewMenuCsect], "/View/_Cross Section", 0);
        set(mbox->menu[viewMenuRuler], "/View/_Rulers", 0);
        set(mbox->menu[viewMenuInfo], "/View/_Info", 0);
        set(mbox->menu[viewMenuAlloc], "/View/_Allocation", 0);

        int j = 0, i = 0;
        for (MenuEnt *ent = mbox->menu; ent->entry; ent++) {

            if (i == viewMenuSced || i == viewMenuPhys) {
                if (!ScedIf()->hasSced()) {
                    i++;
                    continue;
                }
            }
            mi[j++] = if_entry(ent->menutext, ent->accel,
                i && i != viewMenuView ? HANDLER : 0, i, ent->item);
            if (ent->is_separator())
                mi[j++] = if_entry("/View/sep", 0, 0, NOTMAPPED, "<Separator>");
            i++;
        }
        GtkItemFactoryEntry *menu = new GtkItemFactoryEntry[j];
        memcpy(menu, mi, j*sizeof(GtkItemFactoryEntry));
        make_entries(item_factory, menu, j, mbox->menu, i);
        delete [] menu;

        // Initialize the popup menus
        // set data "menu" -> menu in the buttons
        // set data "callb" -> callback function
        // set data "data" -> callback data
        GtkWidget *btn = gtk_item_factory_get_widget(item_factory,
            "/View/View");
        GtkWidget *popup = gtkMenu()->new_popup_menu(btn,
            XM()->ViewList(), G_CALLBACK(vimenu_proc), 0);
        if (popup) {
            gtk_widget_set_name(popup, "ViewSubmenu");
            gtk_object_set_data(GTK_OBJECT(btn), "menu", popup);
            gtk_object_set_data(GTK_OBJECT(btn), "callb", (void*)vimenu_proc);

            // Set up the mapping so that the accelerator for the
            // "view" button will activate the "full" operation from
            // the sub-menu.

            GList *contents = gtk_container_children(GTK_CONTAINER(popup));
            if (contents) {
                // the first item is the "full" button
                GtkWidget *fullbut = GTK_WIDGET(contents->data);
                mbox->menu[viewMenuView].alt_caller = fullbut;
                g_list_free(contents);
            }
            if (mbox->menu[viewMenuView].description) {
                gtk_widget_set_tooltip_text(btn, 
                    mbox->menu[viewMenuView].description);
            }
        }

        // name the menubar object
        GtkWidget *widget = gtk_item_factory_get_item(item_factory, "/View");
        if (widget)
            gtk_widget_set_name(widget, "View");
    }
#else
    // View menu
    MenuBox *mbox = Menu()->FindMainMenu("view");
    if (!mbox || !mbox->menu)
        return
    GtkAccelGroup *accel_group = gtkMenu()->AccelGroup();
    if (!accel_group)
        return;
    GtkWidget *menubar = gtkMenu()->MainMenu();
    if (!menubar)
        return;

    MenuBox *ent = &mbox->menu[viewMenu];
    set(*ent, "_View", 0);

    GtkWidget *item = gtk_menu_item_new_with_mnemonic(ent->menutext);
    gtk_widget_set_name(item, "View");
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), item);
    GtkWidget *viewMenu = gtk_menu_new();
    gtk_widget_show(viewMenu);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), viewMenu);

    ent = &mbox->menu[viewMenuView];
    item = miset(ent, "Vie_w", 0);
    gtk_menu_shell_append(GTK_MENU_SHELL(viewMenu), item);
    check_separator(ent, viewMenu);

    // Initialize the popup menu.
    // set data "menu" -> menu in the buttons
    // set data "callb" -> callback function
    // set data "data" -> callback data
    GtkWidget *btn = item;
    GtkWidget *popup = gtkMenu()->new_popup_menu(btn,
        XM()->ViewList(), G_CALLBACK(vimenu_proc), 0);
    if (popup) {
        gtk_widget_set_name(popup, "ViewSubmenu");
        gtk_object_set_data(GTK_OBJECT(btn), "menu", popup);
        gtk_object_set_data(GTK_OBJECT(btn), "callb", (void*)vimenu_proc);

        // Set up the mapping so that the accelerator for the
        // "view" button will activate the "full" operation from
        // the sub-menu.

        GList *contents = gtk_container_children(GTK_CONTAINER(popup));
        if (contents) {
            // the first item is the "full" button
            GtkWidget *fullbut = GTK_WIDGET(contents->data);
            mbox->menu[viewMenuView].alt_caller = fullbut;
            g_list_free(contents);
        }
        if (mbox->menu[viewMenuView].description) {
            gtk_widget_set_tooltip_text(btn, 
                mbox->menu[viewMenuView].description);
        }
    }

    ent = &mbox->menu[viewMenuSced];
    set(*ent, "E_lectrical", 0);
    if (ScedIf()->hasSced()) {
        item = new_item(ent);
        gtk_menu_shell_append(GTK_MENU_SHELL(viewMenu), item);
        g_signal_connect(G_OBJECT(item), "activate",
            G_CALLBACK(menu_handler), (gpointer)(long)viewMenuSced);
        check_separator(ent, viewMenu);
    }

    ent = &mbox->menu[viewMenuPhys];
    set(*ent, "P_hysical", 0);
    if (ScedIf()->hasSced()) {
        item = new_item(ent);
        gtk_menu_shell_append(GTK_MENU_SHELL(viewMenu), item);
        g_signal_connect(G_OBJECT(item), "activate",
            G_CALLBACK(menu_handler), (gpointer)(long)viewMenuPhys);
        check_separator(ent, viewMenu);
    }

    ent = &mbox->menu[viewMenuExpnd];
    item = miset(ent, "_Expand", 0);
    gtk_menu_shell_append(GTK_MENU_SHELL(viewMenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), (gpointer)(long)viewMenuExpnd);
    check_separator(ent, viewMenu);

    ent = &mbox->menu[viewMenuZoom];
    item = miset(ent, "_Zoom", 0);
    gtk_menu_shell_append(GTK_MENU_SHELL(viewMenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), (gpointer)(long)viewMenuZoom);
    check_separator(ent, viewMenu);

    ent = &mbox->menu[viewMenuVport];
    item = miset(ent, "_Viewport", 0);
    gtk_menu_shell_append(GTK_MENU_SHELL(viewMenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), (gpointer)(long)viewMenuVport);
    check_separator(ent, viewMenu);

    ent = &mbox->menu[viewMenuPeek];
    item = miset(ent, "_Peek", 0);
    gtk_menu_shell_append(GTK_MENU_SHELL(viewMenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), (gpointer)(long)viewMenuPeek);
    check_separator(ent, viewMenu);

    ent = &mbox->menu[viewMenuCsect];
    item = miset(ent, "_Cross Section", 0);
    gtk_menu_shell_append(GTK_MENU_SHELL(viewMenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), (gpointer)(long)viewMenuCsect);
    check_separator(ent, viewMenu);

    ent = &mbox->menu[viewMenuRuler];
    item = miset(ent, "_Rulers", 0);
    gtk_menu_shell_append(GTK_MENU_SHELL(viewMenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), (gpointer)(long)viewMenuRuler);
    check_separator(ent, viewMenu);

    ent = &mbox->menu[viewMenuInfo];
    item = miset(ent, "_Info", 0);
    gtk_menu_shell_append(GTK_MENU_SHELL(viewMenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), (gpointer)(long)viewMenuInfo);
    check_separator(ent, viewMenu);

    ent = &mbox->menu[viewMenuAlloc];
    item = miset(ent, "_Allocation", 0);
    gtk_menu_shell_append(GTK_MENU_SHELL(viewMenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), (gpointer)(long)viewMenuAlloc);
    check_separator(ent, viewMenu);

#endif
}


void
gtkMenuConfig::instantiateAttributesMenu()
{
#ifdef UseItemFactory
    GtkItemFactory *item_factory = gtkMenu()->itemFactory;
    if (!item_factory)
        return;
    if_entry mi[50];

    // Attributes menu
    MenuBox *mbox = Menu()->FindMainMenu("attr");
    if (mbox && mbox->menu) {

        set(mbox->menu[attrMenu], "/_Attributes", 0);
        set(mbox->menu[attrMenuUpdat], "/Attributes/Save _Tech", 0);
        set(mbox->menu[attrMenuKeymp], "/Attributes/_Key Map", 0);
        set(mbox->menu[attrMenuMacro], "/Attributes/Define _Macro", 0);
        set(mbox->menu[attrMenuMainWin], "/Attributes/Main _Window", 0);
        set(mbox->menu[attrMenuAttr], "/Attributes/Set _Attributes", 0);
        set(mbox->menu[attrMenuDots], "/Attributes/Connection _Dots", 0);
        set(mbox->menu[attrMenuFont], "/Attributes/Set F_ont", 0);
        set(mbox->menu[attrMenuColor], "/Attributes/Set _Color", 0);
        set(mbox->menu[attrMenuFill], "/Attributes/Set _Fill", 0);
        set(mbox->menu[attrMenuEdlyr], "/Attributes/_Edit Layers", 0);
        set(mbox->menu[attrMenuLpedt], "/Attributes/Edit Tech _Params", 0);

        int j = 0, i = 0;
        for (MenuEnt *ent = mbox->menu; ent->entry; ent++) {
            if (i == attrMenuDots) {
                if (!ScedIf()->hasSced()) {
                    i++;
                    continue;
                }
            }
            mi[j++] = if_entry(ent->menutext, ent->accel,
                i && i != attrMenuMainWin ? HANDLER : 0, i, ent->item);
            if (ent->is_separator())
                mi[j++] = if_entry("/Attributes/sep", 0, 0, NOTMAPPED,
                    "<Separator>");
            i++;
        }
        GtkItemFactoryEntry *menu = new GtkItemFactoryEntry[j];
        memcpy(menu, mi, j*sizeof(GtkItemFactoryEntry));
        make_entries(item_factory, menu, j, mbox->menu, i);
        delete [] menu;

        mbox = Menu()->GetAttrSubMenu();
        if (mbox && mbox->menu) {
            set(mbox->menu[subwAttrMenuFreez],
                "/Attributes/Main Window/Freeze _Display", 0);
            set(mbox->menu[subwAttrMenuCntxt],
                "/Attributes/Main Window/Show Conte_xt in Push", 0);
            set(mbox->menu[subwAttrMenuProps],
                "/Attributes/Main Window/Show _Phys Properties", 0);
            set(mbox->menu[subwAttrMenuLabls],
                "/Attributes/Main Window/Show _Labels", 0);
            set(mbox->menu[subwAttrMenuLarot],
                "/Attributes/Main Window/L_abel True Orient", 0);
            set(mbox->menu[subwAttrMenuCnams],
                "/Attributes/Main Window/Show Cell _Names", 0);
            set(mbox->menu[subwAttrMenuCnrot],
                "/Attributes/Main Window/Cell Na_me True Orient", 0);
            set(mbox->menu[subwAttrMenuNouxp],
                "/Attributes/Main Window/Don't Show _Unexpanded", 0);
            set(mbox->menu[subwAttrMenuObjs],
                "/Attributes/Main Window/Objects Shown", 0);
            set(mbox->menu[subwAttrMenuTinyb],
                "/Attributes/Main Window/Subthreshold _Boxes", 0);
            set(mbox->menu[subwAttrMenuNosym],
                "/Attributes/Main Window/No Top _Symbolic", 0);
            set(mbox->menu[subwAttrMenuGrid],
                "/Attributes/Main Window/Set _Grid", "<control>G");

            j = 0, i = 1;
            for (MenuEnt *ent = mbox->menu + 1; ent->entry && ent->menutext;
                    ent++) {
                if (i == subwAttrMenuNosym) {
                    if (!ScedIf()->hasSced()) {
                        i++;
                        continue;
                    }
                }
                mi[j++] = if_entry(ent->menutext, ent->accel,
                    i != subwAttrMenuObjs ? HANDLER : 0, i, ent->item);
                if (ent->is_separator())
                    mi[j++] = if_entry("/Attributes/Main Window/sep", 0, 0,
                        NOTMAPPED, "<Separator>");
                i++;
            }
            menu = new GtkItemFactoryEntry[j];
            memcpy(menu, mi, j*sizeof(GtkItemFactoryEntry));
            make_entries(item_factory, menu, j, mbox->menu, i);
            delete [] menu;
        }

        mbox = Menu()->GetObjSubMenu();
        if (mbox && mbox->menu) {
            set(mbox->menu[0],
                "/Attributes/Main Window/Objects Shown/Boxes", 0);
            set(mbox->menu[1],
                "/Attributes/Main Window/Objects Shown/Polys", 0);
            set(mbox->menu[2],
                "/Attributes/Main Window/Objects Shown/Wires", 0);
            set(mbox->menu[3],
                "/Attributes/Main Window/Objects Shown/Labels", 0);

            j = 0, i = 0;
            for (MenuEnt *ent = mbox->menu; ent->entry && ent->menutext;
                    ent++) {
                mi[j++] = if_entry(ent->menutext, ent->accel,
                    HANDLER, i, ent->item);
                i++;
            }
            menu = new GtkItemFactoryEntry[j];
            memcpy(menu, mi, j*sizeof(GtkItemFactoryEntry));
            make_entries(item_factory, menu, j, mbox->menu, i);
            delete [] menu;
        }

        // name the menubar object
        GtkWidget *widget = gtk_item_factory_get_item(item_factory,
            "/Attributes");
        if (widget)
            gtk_widget_set_name(widget, "Attributes");
    }
#else
    // Attributes menu
    MenuBox *mbox = Menu()->FindMainMenu("attr");
    if (!mbox || !mbox->menu)
        return
    GtkAccelGroup *accel_group = gtkMenu()->AccelGroup();
    if (!accel_group)
        return;
    GtkWidget *menubar = gtkMenu()->MainMenu();
    if (!menubar)
        return;

    MenuEnt *ent = &mbox->menu[attrMenu];
    set(*ent, "_Attributes", 0);

    GtkWidget *item = gtk_menu_item_new_with_mnemonic(ent->menutext);
    gtk_widget_set_name(item, "Attributes");
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), item);
    GtkWidget *attrMenu = gtk_menu_new();
    gtk_widget_show(attrMenu);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), attrMenu);

    ent = &mbox->menu[attrMenuUpdat];
    item = miset(ent, "Save _Tech", 0);
    gtk_menu_shell_append(GTK_MENU_SHELL(attrMenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), (gpointer)(long)attrMenuUpdat);
    check_separator(ent, attrMenu);

    ent = &mbox->menu[attrMenuKeymp];
    item = miset(ent, "_Key Map", 0);
    gtk_menu_shell_append(GTK_MENU_SHELL(attrMenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), (gpointer)(long)attrMenuKeymp);
    check_separator(ent, attrMenu);

    ent = &mbox->menu[attrMenuMacro];
    item = miset(ent, "Define _Macro", 0);
    gtk_menu_shell_append(GTK_MENU_SHELL(attrMenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), (gpointer)(long)attrMenuMacro);
    check_separator(ent, attrMenu);

    ent = &mbox->menu[attrMenuMainWin];
    item = miset(ent, "Main _Window", 0);
    gtk_menu_shell_append(GTK_MENU_SHELL(attrMenu), item);
    check_separator(ent, attrMenu);

    ent = &mbox->menu[attrMenuAttr];
    item = miset(ent, "Set _Attributes", 0);
    gtk_menu_shell_append(GTK_MENU_SHELL(attrMenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), (gpointer)(long)attrMenuAttr);
    check_separator(ent, attrMenu);

    ent = &mbox->menu[attrMenuDots];
    set(*ent, "Connection _Dots", 0);
    if (ScedIf()->hasSced()) {
        item = new_item(ent);
        gtk_menu_shell_append(GTK_MENU_SHELL(attrMenu), item);
        g_signal_connect(G_OBJECT(item), "activate",
            G_CALLBACK(menu_handler), (gpointer)(long)attrMenuDots);
        check_separator(ent, attrMenu);
    }

    ent = &mbox->menu[attrMenuFont];
    item = miset(ent, "Set F_ont", 0);
    gtk_menu_shell_append(GTK_MENU_SHELL(attrMenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), (gpointer)(long)attrMenuFont);
    check_separator(ent, attrMenu);

    ent = &mbox->menu[attrMenuColor];
    item = miset(ent, "Set _Color", 0);
    gtk_menu_shell_append(GTK_MENU_SHELL(attrMenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), (gpointer)(long)attrMenuColor);
    check_separator(ent, attrMenu);

    ent = &mbox->menu[attrMenuFill];
    item = miset(ent, "Set _Fill", 0);
    gtk_menu_shell_append(GTK_MENU_SHELL(attrMenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), (gpointer)(long)attrMenuFill);
    check_separator(ent, attrMenu);

    ent = &mbox->menu[attrMenuEdlyr];
    item = miset(ent, "_Edit Layers", 0);
    gtk_menu_shell_append(GTK_MENU_SHELL(attrMenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), (gpointer)(long)attrMenuEdlyr);
    check_separator(ent, attrMenu);

    ent = &mbox->menu[attrMenuLpedt];
    item = miset(ent, "Edit Tech _Params", 0);
    gtk_menu_shell_append(GTK_MENU_SHELL(attrMenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), (gpointer)(long)attrMenuLpedt);
    check_separator(ent, attrMenu);

    mbox = Menu()->GetAttrSubMenu();
    if (mbox && mbox->menu) {
        set(mbox->menu[subwAttrMenuFreez],
            "/Attributes/Main Window/Freeze _Display", 0);
        set(mbox->menu[subwAttrMenuCntxt],
            "/Attributes/Main Window/Show Conte_xt in Push", 0);
        set(mbox->menu[subwAttrMenuProps],
            "/Attributes/Main Window/Show _Phys Properties", 0);
        set(mbox->menu[subwAttrMenuLabls],
            "/Attributes/Main Window/Show _Labels", 0);
        set(mbox->menu[subwAttrMenuLarot],
            "/Attributes/Main Window/L_abel True Orient", 0);
        set(mbox->menu[subwAttrMenuCnams],
            "/Attributes/Main Window/Show Cell _Names", 0);
        set(mbox->menu[subwAttrMenuCnrot],
            "/Attributes/Main Window/Cell Na_me True Orient", 0);
        set(mbox->menu[subwAttrMenuNouxp],
            "/Attributes/Main Window/Don't Show _Unexpanded", 0);
        set(mbox->menu[subwAttrMenuObjs],
            "/Attributes/Main Window/Objects Shown", 0);
        set(mbox->menu[subwAttrMenuTinyb],
            "/Attributes/Main Window/Subthreshold _Boxes", 0);
        set(mbox->menu[subwAttrMenuNosym],
            "/Attributes/Main Window/No Top _Symbolic", 0);
        set(mbox->menu[subwAttrMenuGrid],
            "/Attributes/Main Window/Set _Grid", "<control>G");

        j = 0, i = 1;
        for (MenuEnt *ent = mbox->menu + 1; ent->entry && ent->menutext;
                ent++) {
            if (i == subwAttrMenuNosym) {
                if (!ScedIf()->hasSced()) {
                    i++;
                    continue;
                }
            }
            mi[j++] = if_entry(ent->menutext, ent->accel,
                i != subwAttrMenuObjs ? HANDLER : 0, i, ent->item);
            if (ent->is_separator())
                mi[j++] = if_entry("/Attributes/Main Window/sep", 0, 0,
                    NOTMAPPED, "<Separator>");
            i++;
        }
        menu = new GtkItemFactoryEntry[j];
        memcpy(menu, mi, j*sizeof(GtkItemFactoryEntry));
        make_entries(item_factory, menu, j, mbox->menu, i);
        delete [] menu;
    }

    mbox = Menu()->GetObjSubMenu();
    if (mbox && mbox->menu) {
        set(mbox->menu[0],
            "/Attributes/Main Window/Objects Shown/Boxes", 0);
        set(mbox->menu[1],
            "/Attributes/Main Window/Objects Shown/Polys", 0);
        set(mbox->menu[2],
            "/Attributes/Main Window/Objects Shown/Wires", 0);
        set(mbox->menu[3],
            "/Attributes/Main Window/Objects Shown/Labels", 0);

        j = 0, i = 0;
        for (MenuEnt *ent = mbox->menu; ent->entry && ent->menutext;
                ent++) {
            mi[j++] = if_entry(ent->menutext, ent->accel,
                HANDLER, i, ent->item);
            i++;
        }
        menu = new GtkItemFactoryEntry[j];
        memcpy(menu, mi, j*sizeof(GtkItemFactoryEntry));
        make_entries(item_factory, menu, j, mbox->menu, i);
        delete [] menu;
    }
#endif
}


void
gtkMenuConfig::instantiateConvertMenu()
{
#ifdef UseItemFactory
    GtkItemFactory *item_factory = gtkMenu()->itemFactory;
    if (!item_factory)
        return;
    if_entry mi[50];

    // Convert menu
    MenuBox *mbox = Menu()->FindMainMenu("conv");
    if (mbox && mbox->menu) {

        set(mbox->menu[cvrtMenu], "/C_onvert", 0);
        set(mbox->menu[cvrtMenuExprt], "/Convert/_Export Cell Data", 0);
        set(mbox->menu[cvrtMenuImprt], "/Convert/_Import Cell Data", 0);
        set(mbox->menu[cvrtMenuConvt], "/Convert/_Format Conversion", 0);
        set(mbox->menu[cvrtMenuAssem], "/Convert/_Assemble Layout", 0);
        set(mbox->menu[cvrtMenuDiff],  "/Convert/_Compare _Layouts", 0);
        set(mbox->menu[cvrtMenuCut],   "/Convert/C_ut and Export", 0);
        set(mbox->menu[cvrtMenuTxted], "/Convert/_Text Editor", 0);

        int j = 0, i = 0;
        for (MenuEnt *ent = mbox->menu; ent->entry; ent++) {
            mi[j++] = if_entry(ent->menutext, ent->accel, i ? HANDLER : 0,
                i, ent->item);
            if (ent->is_separator())
                mi[j++] = if_entry("/Convert/sep", 0, 0, NOTMAPPED,
                    "<Separator>");
            i++;
        }
        GtkItemFactoryEntry *menu = new GtkItemFactoryEntry[j];
        memcpy(menu, mi, j*sizeof(GtkItemFactoryEntry));
        make_entries(item_factory, menu, j, mbox->menu, i);
        delete [] menu;

        // name the menubar object
        GtkWidget *widget = gtk_item_factory_get_item(item_factory,
            "/Convert");
        if (widget)
            gtk_widget_set_name(widget, "Convert");
    }
#else
    // Convert menu
    MenuBox *mbox = Menu()->FindMainMenu("conv");
    if (!mbox || !mbox->menu)
        return
    GtkAccelGroup *accel_group = gtkMenu()->AccelGroup();
    if (!accel_group)
        return;
    GtkWidget *menubar = gtkMenu()->MainMenu();
    if (!menubar)
        return;

    MenuEnt *ent = &mbox->menu[cvrtMenu];
    set(*ent, "C_onvert", 0);

    GtkWidget *item = gtk_menu_item_new_with_mnemonic(ent->menutext);
    gtk_widget_set_name(item, "Convert");
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), item);
    GtkWidget *convMenu = gtk_menu_new();
    gtk_widget_show(convMenu);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), convMenu);

    ent = &mbox->menu[cvrtMenuExprt];
    item = miset(ent, "_Export Cell Data", 0);
    gtk_menu_shell_append(GTK_MENU_SHELL(convMenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), (gpointer)(long)cvrtMenuExprt);
    check_separator(ent, convMenu);

    ent = &mbox->menu[cvrtMenuImprt];
    item = miset(ent, "_Import Cell Data", 0);
    gtk_menu_shell_append(GTK_MENU_SHELL(convMenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), (gpointer)(long)cvrtMenuImprt);
    check_separator(ent, convMenu);

    ent = &mbox->menu[cvrtMenuConvt];
    item = miset(ent, "_Format Conversion", 0);
    gtk_menu_shell_append(GTK_MENU_SHELL(convMenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), (gpointer)(long)cvrtMenuConvt);
    check_separator(ent, convMenu);

    ent = &mbox->menu[cvrtMenuAssem];
    item = miset(ent, "_Assemble Layout", 0);
    gtk_menu_shell_append(GTK_MENU_SHELL(convMenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), (gpointer)(long)cvrtMenuAssem);
    check_separator(ent, convMenu);

    ent = &mbox->menu[cvrtMenuDiff];
    item = miset(ent, "_Compare _Layouts", 0);
    gtk_menu_shell_append(GTK_MENU_SHELL(convMenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), (gpointer)(long)cvrtMenuDiff);
    check_separator(ent, convMenu);

    ent = &mbox->menu[cvrtMenuCut];
    item = miset(ent, "C_ut and Export", 0);
    gtk_menu_shell_append(GTK_MENU_SHELL(convMenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), (gpointer)(long)cvrtMenuCut);
    check_separator(ent, convMenu);

    ent = &mbox->menu[cvrtMenuTxted];
    item = miset(ent, "_Text Editor", 0);
    gtk_menu_shell_append(GTK_MENU_SHELL(convMenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), (gpointer)(long)cvrtMenuCut);
    check_separator(ent, convMenu);
#endif
}


void
gtkMenuConfig::instantiateDRCMenu()
{
#ifdef UseItemFactory
    GtkItemFactory *item_factory = gtkMenu()->itemFactory;
    if (!item_factory)
        return;
    if_entry mi[50];

    // Drc menu
    MenuBox *mbox = Menu()->FindMainMenu("drc");
    if (mbox && mbox->menu) {

        set(mbox->menu[drcMenu], "/_DRC", 0);
        set(mbox->menu[drcMenuLimit], "/DRC/_Setup", 0);
        set(mbox->menu[drcMenuSflag], "/DRC/Set Skip _Flags", 0);
        set(mbox->menu[drcMenuIntr], "/DRC/Enable _Interactive", "<Alt>I");
        set(mbox->menu[drcMenuNopop], "/DRC/_No Pop Up Errors", 0);
        set(mbox->menu[drcMenuCheck], "/DRC/_Batch Check", 0);
        set(mbox->menu[drcMenuPoint], "/DRC/Check In Re_gion", 0);
        set(mbox->menu[drcMenuClear], "/DRC/_Clear Errors", "<Alt>R");
        set(mbox->menu[drcMenuQuery], "/DRC/_Query Errors", "<Alt>Q");
        set(mbox->menu[drcMenuErdmp], "/DRC/_Dump Error File", 0);
        set(mbox->menu[drcMenuErupd], "/DRC/_Update Highlighting", 0);
        set(mbox->menu[drcMenuNext], "/DRC/Show _Errors", 0);
        set(mbox->menu[drcMenuErlyr], "/DRC/Create _Layer", 0);
        set(mbox->menu[drcMenuDredt], "/DRC/Edit R_ules", 0);

        int j = 0, i = 0;
        for (MenuEnt *ent = mbox->menu; ent->entry; ent++) {
            mi[j++] = if_entry(ent->menutext, ent->accel, i ? HANDLER : 0,
                i, ent->item);
            if (ent->is_separator())
                mi[j++] = if_entry("/DRC/sep", 0, 0, NOTMAPPED,
                    "<Separator>");
            i++;
        }
        GtkItemFactoryEntry *menu = new GtkItemFactoryEntry[j];
        memcpy(menu, mi, j*sizeof(GtkItemFactoryEntry));
        make_entries(item_factory, menu, j, mbox->menu, i);
        delete [] menu;

        // name the menubar object
        GtkWidget *widget = gtk_item_factory_get_item(item_factory, "/DRC");
        if (widget)
            gtk_widget_set_name(widget, "DRC");
    }
#else
    // DRC menu
    MenuBox *mbox = Menu()->FindMainMenu("drc");
    if (!mbox || !mbox->menu)
        return
    GtkAccelGroup *accel_group = gtkMenu()->AccelGroup();
    if (!accel_group)
        return;
    GtkWidget *menubar = gtkMenu()->MainMenu();
    if (!menubar)
        return;

    MenuEnt *ent = &mbox->menu[drcMenu];
    set(*ent, "_DRC", 0);

    GtkWidget *item = gtk_menu_item_new_with_mnemonic(ent->menutext);
    gtk_widget_set_name(item, "DRC");
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), item);
    GtkWidget *drcMenu = gtk_menu_new();
    gtk_widget_show(drcMenu);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), drcMenu);

    ent = &mbox->menu[drcMenuLimit];
    item = miset(ent, "_Setup", 0);
    gtk_menu_shell_append(GTK_MENU_SHELL(drcMenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), (gpointer)(long)drcMenuLimin);
    check_separator(ent, drcMenu);

    ent = &mbox->menu[drcMenuSflag];
    item = miset(ent, "Set Skip _Flags", 0);
    gtk_menu_shell_append(GTK_MENU_SHELL(drcMenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), (gpointer)(long)drcMenuSflag);
    check_separator(ent, drcMenu);

    ent = &mbox->menu[drcMenuIntr];
    item = miset(ent, "Enable _Interactive", "<Alt>I");
    gtk_menu_shell_append(GTK_MENU_SHELL(drcMenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), (gpointer)(long)drcMenuIntr);
//    gtk_widget_add_accelerator(item, "activate", accel_group, GDK_i,
//        GDK_ALT_MASK, GTK_ACCEL_VISIBLE);
    check_separator(ent, drcMenu);

    ent = &mbox->menu[drcMenuNopop];
    item = miset(ent, "_No Pop Up Errors", 0);
    gtk_menu_shell_append(GTK_MENU_SHELL(drcMenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), (gpointer)(long)drcMenuNopop);
    check_separator(ent, drcMenu);

    ent = &mbox->menu[drcMenuCheck];
    item = miset(ent, "_Batch Check", 0);
    gtk_menu_shell_append(GTK_MENU_SHELL(drcMenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), (gpointer)(long)drcMenuCheck);
    check_separator(ent, drcMenu);

    ent = &mbox->menu[drcMenuPoint];
    item = miset(ent, "Check In Re_gion", 0);
    gtk_menu_shell_append(GTK_MENU_SHELL(drcMenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), (gpointer)(long)drcMenuPoint);
    check_separator(ent, drcMenu);

    ent = &mbox->menu[drcMenuClear];
    item = miset(ent, "_Clear Errors", "<Alt>R");
    gtk_menu_shell_append(GTK_MENU_SHELL(drcMenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), (gpointer)(long)drcMenuClear);
//    gtk_widget_add_accelerator(item, "activate", accel_group, GDK_r,
//        GDK_ALT_MASK, GTK_ACCEL_VISIBLE);
    check_separator(ent, drcMenu);

    ent = &mbox->menu[drcMenuQuery];
    item = miset(ent, "_Query Errors", "<Alt>Q");
    gtk_menu_shell_append(GTK_MENU_SHELL(drcMenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), (gpointer)(long)drcMenuQuery);
//    gtk_widget_add_accelerator(item, "activate", accel_group, GDK_q,
//        GDK_ALT_MASK, GTK_ACCEL_VISIBLE);
    check_separator(ent, drcMenu);

    ent = &mbox->menu[drcMenuErdmp];
    item = miset(ent, "_Dump Error File", 0);
    gtk_menu_shell_append(GTK_MENU_SHELL(drcMenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), (gpointer)(long)drcMenuErdmp);
    check_separator(ent, drcMenu);

    ent = &mbox->menu[drcMenuErupd];
    item = miset(ent, "_Update Highlighting", 0);
    gtk_menu_shell_append(GTK_MENU_SHELL(drcMenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), (gpointer)(long)drcMenuErupd);
    check_separator(ent, drcMenu);

    ent = &mbox->menu[drcMenuNext];
    item = miset(ent, "Show _Errors", 0);
    gtk_menu_shell_append(GTK_MENU_SHELL(drcMenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), (gpointer)(long)drcMenuNext);
    check_separator(ent, drcMenu);

    ent = &mbox->menu[drcMenuErlyr];
    item = miset(ent, "Create _Layer", 0);
    gtk_menu_shell_append(GTK_MENU_SHELL(drcMenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), (gpointer)(long)drcMenuErlyr);
    check_separator(ent, drcMenu);

    ent = &mbox->menu[drcMenuDredt];
    item = miset(ent, "Edit R_ules", 0);
    gtk_menu_shell_append(GTK_MENU_SHELL(drcMenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), (gpointer)(long)drcMenuDredt);
    check_separator(ent, drcMenu);

#endif
}


void
gtkMenuConfig::instantiateExtractMenu()
{
#ifdef UseItemFactory
    GtkItemFactory *item_factory = gtkMenu()->itemFactory;
    if (!item_factory)
        return;
    if_entry mi[50];

    // Extract menu
    MenuBox *mbox = Menu()->FindMainMenu("extr");
    if (mbox && mbox->menu) {

        set(mbox->menu[extMenu], "/E_xtract", 0);
        set(mbox->menu[extMenuExcfg], "/Extract/Set_up", 0);
        set(mbox->menu[extMenuSel], "/Extract/_Net Selections", 0);
        set(mbox->menu[extMenuDvsel], "/Extract/_Device Selections", 0);
        set(mbox->menu[extMenuSourc], "/Extract/_Source SPICE", 0);
        set(mbox->menu[extMenuExset], "/Extract/Source P_hysical", 0);
        set(mbox->menu[extMenuPnet], "/Extract/Dump Ph_ys Netlist", 0);
        set(mbox->menu[extMenuEnet], "/Extract/Dump E_lec Netlist", 0);
        set(mbox->menu[extMenuLvs], "/Extract/Dump L_VS", 0);
        set(mbox->menu[extMenuExC], "/Extract/Extract _C", 0);
        set(mbox->menu[extMenuExLR], "/Extract/Extract L_R", 0);

        int j = 0, i = 0;
        for (MenuEnt *ent = mbox->menu; ent->entry; ent++) {
            mi[j++] = if_entry(ent->menutext, ent->accel, i ? HANDLER : 0,
                i, ent->item);
            if (ent->is_separator())
                mi[j++] = if_entry("/Extract/sep", 0, 0, NOTMAPPED,
                    "<Separator>");
            i++;
        }
        GtkItemFactoryEntry *menu = new GtkItemFactoryEntry[j];
        memcpy(menu, mi, j*sizeof(GtkItemFactoryEntry));
        make_entries(item_factory, menu, j, mbox->menu, i);
        delete [] menu;

        // name the menubar object
        GtkWidget *widget = gtk_item_factory_get_item(item_factory,
            "/Extract");
        if (widget)
            gtk_widget_set_name(widget, "Extract");
    }
#else
    // Extract menu
    MenuBox *mbox = Menu()->FindMainMenu("extr");
    if (!mbox || !mbox->menu)
        return
    GtkAccelGroup *accel_group = gtkMenu()->AccelGroup();
    if (!accel_group)
        return;
    GtkWidget *menubar = gtkMenu()->MainMenu();
    if (!menubar)
        return;

    MenuEnt *ent = &mbox->menu[extMenu];
    set(*ent, "E_xtract", 0);

    GtkWidget *item = gtk_menu_item_new_with_mnemonic(ent->menutext);
    gtk_widget_set_name(item, "Extract");
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), item);
    GtkWidget *extMenu = gtk_menu_new();
    gtk_widget_show(extMenu);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), extMenu);

    ent = &mbox->menu[extMenuExcfg];
    item = miset(ent, "Set_up", 0);
    gtk_menu_shell_append(GTK_MENU_SHELL(extMenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), (gpointer)(long)extMenuExcfg);
    check_separator(ent, extMenu);

    ent = &mbox->menu[extMenuSel];
    item = miset(ent, "_Net Selections", 0);
    gtk_menu_shell_append(GTK_MENU_SHELL(extMenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), (gpointer)(long)extMenuSel);
    check_separator(ent, extMenu);

    ent = &mbox->menu[extMenuDvsel];
    item = miset(ent, "_Device Selections", 0);
    gtk_menu_shell_append(GTK_MENU_SHELL(extMenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), (gpointer)(long)extMenuDvsel);
    check_separator(ent, extMenu);

    ent = &mbox->menu[extMenuSourc];
    item = miset(ent, "_Source SPICE", 0);
    gtk_menu_shell_append(GTK_MENU_SHELL(extMenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), (gpointer)(long)extMenuSourc);
    check_separator(ent, extMenu);

    ent = &mbox->menu[extMenuExset];
    item = miset(ent, "Source P_hysical", 0);
    gtk_menu_shell_append(GTK_MENU_SHELL(extMenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), (gpointer)(long)extMenuExset);
    check_separator(ent, extMenu);

    ent = &mbox->menu[extMenuPnet];
    item = miset(ent, "Dump Ph_ys Netlist", 0);
    gtk_menu_shell_append(GTK_MENU_SHELL(extMenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), (gpointer)(long)extMenuPnet);
    check_separator(ent, extMenu);

    ent = &mbox->menu[extMenuEnet];
    item = miset(ent, "Dump E_lec Netlist", 0);
    gtk_menu_shell_append(GTK_MENU_SHELL(extMenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), (gpointer)(long)extMenuEnet);
    check_separator(ent, extMenu);

    ent = &mbox->menu[extMenuLvs];
    item = miset(ent, "Dump L_VS", 0);
    gtk_menu_shell_append(GTK_MENU_SHELL(extMenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), (gpointer)(long)extMenuLvs);
    check_separator(ent, extMenu);

    ent = &mbox->menu[extMenuExC];
    item = miset(ent, "Extract _C", 0);
    gtk_menu_shell_append(GTK_MENU_SHELL(extMenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), (gpointer)(long)extMenuExC);
    check_separator(ent, extMenu);

    ent = &mbox->menu[extMenuExLR];
    item = miset(ent, "Extract L_R", 0);
    gtk_menu_shell_append(GTK_MENU_SHELL(extMenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), (gpointer)(long)extMenuExLR);
    check_separator(ent, extMenu);
#endif
}


void
gtkMenuConfig::instantiateUserMenu()
{
#ifdef UseItemFactory
    GtkItemFactory *item_factory = gtkMenu()->itemFactory;
    if (!item_factory)
        return;
    if_entry mi[50];

    // User menu
    MenuBox *mbox = Menu()->FindMainMenu("user");
    if (mbox && mbox->menu) {

        set(mbox->menu[userMenu], "/_User", 0);
        set(mbox->menu[userMenuDebug], "/User/_Debugger", 0);
        set(mbox->menu[userMenuHash], "/User/_Rehash", 0);

        int j = 0, i = 0;
        for (MenuEnt *ent = mbox->menu; ent->entry; ent++) {
            mi[j++] = if_entry(ent->menutext, ent->accel, i ? HANDLER : 0,
                i, ent->item);
            if (ent->is_separator())
                mi[j++] = if_entry("/User/sep", 0, 0, NOTMAPPED,
                    "<Separator>");
            i++;
        }
        GtkItemFactoryEntry *menu = new GtkItemFactoryEntry[j];
        memcpy(menu, mi, j*sizeof(GtkItemFactoryEntry));
        make_entries(item_factory, menu, j, mbox->menu, i);
        delete [] menu;

        // name the menubar object
        GtkWidget *widget = gtk_item_factory_get_item(item_factory, "/User");
        if (widget)
            gtk_widget_set_name(widget, "User");
    }
#else
    // User menu
    MenuBox *mbox = Menu()->FindMainMenu("user");
    if (!mbox || !mbox->menu)
        return
    GtkAccelGroup *accel_group = gtkMenu()->AccelGroup();
    if (!accel_group)
        return;
    GtkWidget *menubar = gtkMenu()->MainMenu();
    if (!menubar)
        return;

    MenuEnt *ent = &mbox->menu[userMenu];
    item = miset(ent, "/_User", 0);

    GtkWidget *item = gtk_menu_item_new_with_mnemonic(ent->menutext);
    gtk_widget_set_name(item, "User");
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), item);
    GtkWidget *userMenu = gtk_menu_new();
    gtk_widget_show(userMenu);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), userMenu);

    ent = &mbox->menu[userMenuDebug];
    item = miset(ent, "/User/_Debugger", 0);
    gtk_menu_shell_append(GTK_MENU_SHELL(extMenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), (gpointer)(long)userMenuDebug);
    check_separator(ent, userMenu);

    ent = &mbox->menu[userMenuHash];
    item = miset(ent, "/User/_Rehash", 0);
    gtk_menu_shell_append(GTK_MENU_SHELL(extMenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), (gpointer)(long)userMenuHash);
    check_separator(ent, userMenu);
#endif
}


void
gtkMenuConfig::instantiateHelpMenu()
{
#ifdef UseItemFactory
    GtkItemFactory *item_factory = gtkMenu()->itemFactory;
    if (!item_factory)
        return;
    if_entry mi[50];

    // Help menu
    MenuBox *mbox = Menu()->FindMainMenu("help");
    if (mbox && mbox->menu) {

        set(mbox->menu[helpMenu], "/_Help", 0, "<LastBranch>");
        set(mbox->menu[helpMenuHelp], "/Help/_Help", "<control>H");
        set(mbox->menu[helpMenuMultw], "/Help/_Multi-Window", 0);
        set(mbox->menu[helpMenuAbout], "/Help/_About", 0);
        set(mbox->menu[helpMenuNotes], "/Help/_Release Notes", 0);
        set(mbox->menu[helpMenuLogs], "/Help/Log _Files", 0);
        set(mbox->menu[helpMenuDebug], "/Help/_Logging", 0);

        int j = 0, i = 0;
        for (MenuEnt *ent = mbox->menu; ent->entry; ent++) {
            mi[j++] = if_entry(ent->menutext, ent->accel, i ? HANDLER : 0,
                i, ent->item);
            if (ent->is_separator())
                mi[j++] = if_entry("/Help/sep", 0, 0, NOTMAPPED,
                    "<Separator>");
            i++;
        }
        GtkItemFactoryEntry *menu = new GtkItemFactoryEntry[j];
        memcpy(menu, mi, j*sizeof(GtkItemFactoryEntry));
        make_entries(item_factory, menu, j, mbox->menu, i);
        delete [] menu;

        // name the menubar object
        GtkWidget *widget = gtk_item_factory_get_item(item_factory, "/Help");
        if (widget)
            gtk_widget_set_name(widget, "Help");
    }
#else
    // Extract menu
    MenuBox *mbox = Menu()->FindMainMenu("help");
    if (!mbox || !mbox->menu)
        return
    GtkAccelGroup *accel_group = gtkMenu()->AccelGroup();
    if (!accel_group)
        return;
    GtkWidget *menubar = gtkMenu()->MainMenu();
    if (!menubar)
        return;

    MenuEnt *ent = &mbox->menu[helpMenu];
    set(, "_Help", 0, "<LastBranch>");

    GtkWidget *item = gtk_menu_item_new_with_mnemonic(ent->menutext);
    gtk_widget_set_name(item, "Help");
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), item);
    GtkWidget *helpMenu = gtk_menu_new();
    gtk_widget_show(helpMenu);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), helpMenu);

    ent = &mbox->menu[helpMenuHelp];
    item = miset(ent, "_Help", "<control>H");
    gtk_menu_shell_append(GTK_MENU_SHELL(helpMenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), (gpointer)(long)helpMenuHelp);
    check_separator(ent, helpMenu);

    ent = &mbox->menu[helpMenuMultw];
    item = miset(ent, "_Multi-Window", 0);
    gtk_menu_shell_append(GTK_MENU_SHELL(helpMenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), (gpointer)(long)helpMenuMultw);
    check_separator(ent, helpMenu);

    ent = &mbox->menu[helpMenuAbout];
    item = miset(ent, "_About", 0);
    gtk_menu_shell_append(GTK_MENU_SHELL(helpMenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), (gpointer)(long)helpMenuAbout);
    check_separator(ent, helpMenu);

    ent = &mbox->menu[helpMenuNotes];
    item = miset(ent, "_Release Notes", 0);
    gtk_menu_shell_append(GTK_MENU_SHELL(helpMenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), (gpointer)(long)helpMenuNotes);
    check_separator(ent, helpMenu);

    ent = &mbox->menu[helpMenuLogs];
    item = miset(ent, "Log _Files", 0);
    gtk_menu_shell_append(GTK_MENU_SHELL(helpMenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), (gpointer)(long)helpMenuLogs);
    check_separator(ent, helpMenu);

    ent = &mbox->menu[helpMenuDebug];
    item = miset(ent, "_Logging", 0);
    gtk_menu_shell_append(GTK_MENU_SHELL(helpMenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(menu_handler), (gpointer)(long)helpMenuDebug);
    check_separator(ent, helpMenu);

#endif
}


// Static function.
// This callback controls the dispatching of application
// commands from the menus.
//
void
gtkMenuConfig::menu_handler(GtkWidget *caller, void *client_data,
    unsigned int action)
{
    // sanity check
    if (!client_data)
        return;
    MenuEnt *ent = static_cast<MenuEnt*>(client_data);
    if (Menu()->IsMainMenu(ent) || Menu()->IsButtonMenu(ent) ||
            Menu()->IsMiscMenu(ent) || Menu()->IsSubwMenu(ent)) {
        unsigned i;
        for (i = 0; ent[i].entry; i++) ;
        if (action >= i)
            return;
    }
    else
        return;

    ent += action;
    if (ent->alt_caller)
        // Spurious call from menu pop-up.  This handler should only
        // be called through the alt_caller.
        return;

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
        gdk_window_get_pointer(GRX->DefaultFocusWin(), &x, &y, &mstate);
        bool state = Menu()->GetStatus(caller);

        const char *s;
        if (ent->is_dynamic()) {
            sprintf(buf, "user:%s", ent->entry);
            s = buf;
        }
        else {
            s = ent->entry;
            if (!strcmp(s, MenuHELP)) {
                // quit help
                XM()->QuitHelp();
                return;
            }
            sprintf(buf, "xic:%s", s);
            s = buf;
        }
        if (!(mstate & GDK_SHIFT_MASK)) {
            DSPmainWbag(PopUpHelp(s))
            if (ent->flags & ME_TOGGLE) {
                // put the button back the way it was
                state = !state;
                Menu()->SetStatus(caller, state);
            }
            return;
        }
    }
    if (ent->is_dynamic()) {
        // script from user menu
        if (!XM()->DbgLoad(ent)) {
            EV()->InitCallback();
            // Putting the call in a timeout proc allows the current
            // command to return and finish before the new one starts
            // (see below).
            gtk_timeout_add(50, user_cmd_proc, ent);
        }
        return;
    }

    bool call_on_up;
    if (!Menu()->SetupCommand(ent, &call_on_up))
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

        if (Menu()->GetStatus((GtkWidget*)ent->cmd.caller))
            gtk_timeout_add(50, cmd_proc, ent);
        else if (call_on_up)
            gtk_timeout_add(50, cmd_proc, ent);
        return;
    }
    if (mainBag()) {
        if (ent->action) {
            mainBag()->ShowGhost(ERASE);
            (*ent->action)(&ent->cmd);
            mainBag()->ShowGhost(DISPLAY);
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
gtkMenuConfig::user_cmd_proc(void *arg)
{
    if (dspPkgIf()->IsBusy())
        return (true);
    DSP()->SetInterrupt(DSPinterNone);
    MenuEnt *ent = (MenuEnt*)arg;
    const char *entry = ent->menutext + strlen("/User/");
    // entry is the same as m->entry, but contains the menu path
    // for submenu items
    char buf[128];
    if (lstring::strdirsep(entry)) {
        // from submenu, add a distinguishing prefix to avoid confusion with
        // file path
        sprintf(buf, "%s%s", SCR_LIBCODE, entry);
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
gtkMenuConfig::cmd_proc(void *arg)
{
    if (dspPkgIf()->IsBusy())
        return (true);
    DSP()->SetInterrupt(DSPinterNone);
    MenuEnt *ent = (MenuEnt*)arg;
    if (mainBag()) {
        if (ent->action) {
            mainBag()->ShowGhost(ERASE);
            (*ent->action)(&ent->cmd);
            mainBag()->ShowGhost(DISPLAY);
        }
    }
    CmdState::SetExported(false);
    return (false);
}


// Static function.
// Add a set of entries to the item factory, extracting the just-added
// widget into the CmdDesc of the related menu.
//
void
gtkMenuConfig::make_entries(GtkItemFactory *item_factory,
    GtkItemFactoryEntry *entries, int nitems, MenuEnt *menu, int menuitems)
{
#ifdef UseItemFactyory
    for (int i = 0; i < nitems; i++) {
        if (entries[i].callback_action == NOTMAPPED) {
            // separator
            gtk_item_factory_create_item(item_factory, entries + i, menu, 2);
            continue;
        }

        char *path = gtkMenu()->strip_accel(entries[i].path);
        GtkWidget *w = gtk_item_factory_get_item(item_factory, path);
        if (!w) {
            gtk_item_factory_create_item(item_factory, entries + i, menu, 2);
            w = gtk_item_factory_get_item(item_factory, path);
        }
        int indx = entries[i].callback_action;
        if (w && indx < menuitems) {
            menu[indx].cmd.caller = w;
            if (entries[i].callback == (GtkSignalFunc)menu_handler &&
                    menu[indx].is_set() && entries[i].item_type &&
                    (!strcmp(entries[i].item_type, "<CheckItem>") ||
                    !strcmp(entries[i].item_type, "<ToggleItem>")))
                Menu()->Select(w);
            if (menu[indx].description)
                gtk_widget_set_tooltip_text(w, menu[indx].description);
        }
        delete [] path;
    }
#else
#endif
}


// Static function.
// Edit menu handler.
//
void
gtkMenuConfig::edmenu_proc(GtkWidget *caller, void*)
{
    if (XM()->IsDoingHelp()) {
        int x, y;
        GdkModifierType mstate;
        gdk_window_get_pointer(GRX->DefaultFocusWin(), &x, &y, &mstate);
        if (!(mstate & GDK_SHIFT_MASK)) {
            DSPmainWbag(PopUpHelp("xic:open"))
            return;
        }
    }
    const char *string = Menu()->GetLabel(caller);
    if (!string || !*string)
        return;

    int x, y;
    GdkModifierType mstate;
    gdk_window_get_pointer(GRX->DefaultFocusWin(), &x, &y, &mstate);
    XM()->HandleOpenCellMenu(string, (mstate & GDK_SHIFT_MASK));
}


// Static function.
// View menu handler.
//
void
gtkMenuConfig::vimenu_proc(GtkWidget *caller, void *client_data)
{
    if (XM()->IsDoingHelp()) {
        int x, y;
        GdkModifierType mstate;
        gdk_window_get_pointer(GRX->DefaultFocusWin(), &x, &y, &mstate);
        if (!(mstate & GDK_SHIFT_MASK)) {
            DSPmainWbag(PopUpHelp("xic:view"))
            return;
        }
    }
    const char *string = Menu()->GetLabel(caller);
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
gtkMenuConfig::stmenu_proc(GtkWidget *caller, void*)
{
    const char *string = Menu()->GetLabel(caller);
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
        gdk_window_get_pointer(GRX->DefaultFocusWin(), &x, &y, &mstate);
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

    MenuBox *mbox = Menu()->GetPhysButtonMenu();
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
gtkMenuConfig::shmenu_proc(GtkWidget *caller, void*)
{
    if (!ScedIf()->shapesList())
        return;
    const char *string = Menu()->GetLabel(caller);
    if (!string || !*string)
        return;
    if (XM()->IsDoingHelp()) {
        int x, y;
        GdkModifierType mstate;
        gdk_window_get_pointer(GRX->DefaultFocusWin(), &x, &y, &mstate);
        if (!(mstate & GDK_SHIFT_MASK)) {
            char buf[64];
            sprintf(buf, "shapes:%s", string);
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


// Static function.
void
gtkMenuConfig::top_btnmenu_callback(GtkWidget *widget, void *data)
{
    menu_handler(widget,  Menu()->GetMiscMenu()->menu, (uintptr_t)data);
}


// Static function.
void
gtkMenuConfig::btnmenu_callback(GtkWidget *widget, void *data)
{
    menu_handler(widget,  Menu()->GetButtonMenu()->menu, (uintptr_t)data);
}


namespace {
    // Positioning function for pop-up menus.
    //
    void
    pos_func(GtkMenu*, int *x, int *y, gboolean *pushin, void *data)
    {
        *pushin = true;
        GtkWidget *btn = GTK_WIDGET(data);
        GRX->Location(btn, x, y);
    }
}


// Static function.
// Button-press handler to produce pop-up menu.
//
int
gtkMenuConfig::popup_btn_proc(GtkWidget *widget, GdkEvent *event, void *arg)
{
    gtk_menu_popup(GTK_MENU(arg), 0, 0, pos_func, widget, event->button.button,
        event->button.time);
    return (true);
}


// Static function.
const char **
gtkMenuConfig::get_style_pixmap()
{
    if (EditIf()->getWireStyle() == CDWIRE_FLUSH)
        return (style_f_xpm);
    else if (EditIf()->getWireStyle() == CDWIRE_ROUND)
        return (style_r_xpm);
    // CDWIRE_EXTEND
    return (style_e_xpm);
}



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
#define HANDLER (GtkItemFactoryCallback)menu_handler


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
    // Subclass GtkItemFactoryEntry so can use constructor
    //
    struct if_entry : public GtkItemFactoryEntry
    {
        if_entry()
            {
                path = 0;
                accelerator = 0;
                callback =
                0; callback_action = 0;
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
            XM()->OpenCellMenuList(), GTK_SIGNAL_FUNC(edmenu_proc), 0);
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
                GtkTooltips *tt = gtk_NewTooltip();
                gtk_tooltips_set_tip(tt, btn,
                    mbox->menu[fileMenuOpen].description, "");
            }
        }

        // name the menubar object
        GtkWidget *widget = gtk_item_factory_get_item(item_factory, "/File");
        if (widget)
            gtk_widget_set_name(widget, "File");
    }

    // Cell menu
    mbox = Menu()->FindMainMenu("cell");
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

    // Edit menu
    mbox = Menu()->FindMainMenu("edit");
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

    // Modify menu
    mbox = Menu()->FindMainMenu("mod");
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

    // View menu
    mbox = Menu()->FindMainMenu("view");
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
            XM()->ViewList(), GTK_SIGNAL_FUNC(vimenu_proc), 0);
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
                GtkTooltips *tt = gtk_NewTooltip();
                gtk_tooltips_set_tip(tt, btn,
                    mbox->menu[viewMenuView].description, "");
            }
        }

        // name the menubar object
        GtkWidget *widget = gtk_item_factory_get_item(item_factory, "/View");
        if (widget)
            gtk_widget_set_name(widget, "View");
    }

    // Attributes menu
    mbox = Menu()->FindMainMenu("attr");
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

    // Convert menu
    mbox = Menu()->FindMainMenu("conv");
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

    // Drc menu
    mbox = Menu()->FindMainMenu("drc");
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

    // Extract menu
    mbox = Menu()->FindMainMenu("extr");
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

    // User menu
    mbox = Menu()->FindMainMenu("user");
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

    // Help menu
    mbox = Menu()->FindMainMenu("help");
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
            gtk_signal_connect(GTK_OBJECT(button), "clicked",
                GTK_SIGNAL_FUNC(top_btnmenu_callback), (void*)(intptr_t)i);
            if (ent->description) {
                GtkTooltips *tt = gtk_NewTooltip();
                gtk_tooltips_set_tip(tt, button, ent->description, "");
            }
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
                    EditIf()->styleList(), GTK_SIGNAL_FUNC(stmenu_proc), 0);
                if (menu) {
                    gtk_widget_set_name(menu, "StyleMenu");
                    gtk_signal_connect(GTK_OBJECT(button), "button-press-event",
                        GTK_SIGNAL_FUNC(popup_btn_proc), menu);
                }
            }
            else {
                button = new_pixmap_button(ent->xpm, ent->menutext,
                    ent->is_toggle());
                if (ent->is_set())
                    Menu()->Select(button);
                gtk_signal_connect(GTK_OBJECT(button), "clicked",
                    GTK_SIGNAL_FUNC(btnmenu_callback), (void*)(intptr_t)i);
            }
            if (ent->description) {
                GtkTooltips *tt = gtk_NewTooltip();
                gtk_tooltips_set_tip(tt, button, ent->description, "");
            }
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
                button = new_pixmap_button(ent->xpm, ent->menutext,
                    false);
                GtkWidget *menu = gtkMenu()->new_popup_menu(0,
                    ScedIf()->shapesList(), GTK_SIGNAL_FUNC(shmenu_proc), 0);
                if (menu) {
                    gtk_widget_set_name(menu, "ShapeMenu");
                    gtk_signal_connect(GTK_OBJECT(button), "button-press-event",
                        GTK_SIGNAL_FUNC(popup_btn_proc), menu);
                }
            }
            else {
                button = new_pixmap_button(ent->xpm, ent->menutext,
                    ent->is_toggle());
                if (ent->is_set())
                    Menu()->Select(button);
                gtk_signal_connect(GTK_OBJECT(button), "clicked",
                    GTK_SIGNAL_FUNC(btnmenu_callback), (void*)(intptr_t)i);
            }
            if (ent->description) {
                GtkTooltips *tt = gtk_NewTooltip();
                gtk_tooltips_set_tip(tt, button, ent->description, "");
            }
            gtk_widget_show(button);
            gtk_widget_set_name(button, ent->menutext);
            ent->cmd.caller = button;
        }
    }
}


void
gtkMenuConfig::instantiateSubwMenus(int wnum, GtkItemFactory *item_factory)
{
    if (!item_factory)
        return;
    if_entry mi[50];

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
            XM()->ViewList(), GTK_SIGNAL_FUNC(vimenu_proc),
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
    }

    // Subwin Help Menu
    mbox = Menu()->FindSubwMenu("help", wnum);
    if (mbox && mbox->menu) {

        set(mbox->menu[subwHelpMenu], "/_Help", 0, "<LastBranch>");
        set(mbox->menu[subwHelpMenuHelp], "/Help/_Help", "<control>H");

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
}


void
gtkMenuConfig::updateDynamicMenus()
{
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

    Menu()->RebuildDynamicMenus();

    if_entry mi[50];
    MenuBox *mbox = Menu()->FindMainMenu("user");
    if (mbox && mbox->menu && mbox->isDynamic()) {

        set(mbox->menu[userMenu], "/_User", 0);
        set(mbox->menu[userMenuDebug], "/User/_Debugger", 0);
        set(mbox->menu[userMenuHash], "/User/_Rehash", 0);

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
    }
}


void
gtkMenuConfig::switch_menu_mode(DisplayMode mode, int wnum)
{
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
            if (menu[indx].description) {
                GtkTooltips *tt = gtk_NewTooltip();
                gtk_tooltips_set_tip(tt, w, menu[indx].description, "");
            }
        }
        delete [] path;
    }
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



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

#include "qtmain.h"
#include "qtmenucfg.h"
#include "qtmenu.h"
#include "qtinlines.h"
#include "dsp_inlines.h"
#include "editif.h"
#include "scedif.h"
#include "si_parsenode.h"
#include "si_parser.h"
#include "si_interp.h"
#include "select.h"
#include "events.h"

#include "file_menu.h"
#include "edit_menu.h"
#include "modf_menu.h"
#include "view_menu.h"
#include "attr_menu.h"
#include "cvrt_menu.h"
#include "drc_menu.h"
#include "ext_menu.h"
#include "user_menu.h"
#include "help_menu.h"
#include "psd_menu.h"
#include "sced_menu.h"

#include <QApplication>
#include <QMenuBar>
#include <QMenu>
#include <QLayout>

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
//  xic:XXX  (XXX is command code)
//  xic:edit
//  xic:open
//  xic:width
//  xic:end_flush
//  xic:end_round
//  xic:end_extend
//  shapes:XXX (XXX is shape code)
//  user:XXX (XXX is user code)


qtMenuConfig *qtMenuConfig::instancePtr = 0;

qtMenuConfig::qtMenuConfig()
{
    if (instancePtr) {
        fprintf(stderr,
            "Singleton class qtMenuConfig already instantiated.\n");
        exit(1);
    }
    instancePtr = this;

    style_menu = 0;
    shape_menu = 0;

    connect(this, SIGNAL(exec_idle(MenuEnt*)),
        this, SLOT(idle_exec_slot(MenuEnt*)), Qt::QueuedConnection);
}

// Instantiate
qtMenuConfig _cfg_;


// Private static error exit.
//
void
qtMenuConfig::nullptr()
{
    fprintf(stderr,
        "Singleton class qtMenuCfg used before instantiated.\n");
    exit(1);
}


namespace {
    inline void
    set(MenuEnt &ent, const char *text, const char *ac)
    {
        ent.menutext = text;
        ent.accel = ac;
    }
}


void
qtMenuConfig::instantiateMainMenus()
{
    mainwin *main_win = mainBag();
    if (!main_win)
        return;
    QMenuBar *menubar = main_win->MenuBar();
    if (!menubar)
        return;

    // File memu
    MenuBox *mbox = Menu()->FindMainMenu("file");
    if (mbox && mbox->menu) {
        QMenu *file_menu = new QMenu(main_win);
        file_menu->setTitle(tr(mbox->name));
        menubar->addMenu(file_menu);

        set(mbox->menu[fileMenu], "&File", 0);
        set(mbox->menu[fileMenuOpen], "&Open", 0);
        set(mbox->menu[fileMenuFsel], "F&ile Select", "Alt+O");
        set(mbox->menu[fileMenuSave], "&Save", "Alt+S");
        set(mbox->menu[fileMenuSaveAs], "Save &As", "Ctrl+S");
        set(mbox->menu[fileMenuHcopy], "&Print", "Alt+N");
        set(mbox->menu[fileMenuFiles], "&Files List", 0);
        set(mbox->menu[fileMenuHier], "&Hierarchy Digests", 0);
        set(mbox->menu[fileMenuGeom], "&Geometry Digests", 0);
        set(mbox->menu[fileMenuLibs], "&Libraries List", 0);
        set(mbox->menu[fileMenuStabs], "S&ymbol Tables", 0);
        set(mbox->menu[fileMenuCells], "&Cells List", 0);
        set(mbox->menu[fileMenuTree], "Show &Tree", 0);
        set(mbox->menu[fileMenuExit], "&Quit", "Ctrl+Q");

        // First elt is a dummy containing the menubar item.
        mbox->menu[0].cmd.caller = file_menu;
        for (MenuEnt *ent = mbox->menu + 1; ent->entry; ent++) {
            ent->user_action = file_menu->addAction(
                QString(tr(ent->menutext)));
            if (ent->is_toggle()) {
                action(ent)->setCheckable(true);
                action(ent)->setChecked(ent->is_set());
            }
            action(ent)->setShortcut(QKeySequence(ent->accel));
            action(ent)->setToolTip(QString(ent->description));
            action(ent)->setData(ent - mbox->menu);
            if (ent->is_alt())
                action(ent)->setVisible(false);
            if (ent->is_separator())
                file_menu->addSeparator();
            ent->cmd.caller = ent->user_action;
            ent->cmd.wdesc = DSP()->MainWdesc();
 
            // Edit sub-menu.
            if (ent - mbox->menu == fileMenuOpen) {
                Menu()->NewDDmenu(ent->user_action, XM()->OpenCellMenuList());
                QMenu *submenu = action(ent)->menu();
                connect(submenu, SIGNAL(triggered(QAction*)),
                    this, SLOT(file_open_menu_slot(QAction*)));
            }
        }
        connect(file_menu, SIGNAL(triggered(QAction*)),
            this, SLOT(file_menu_slot(QAction*)));
    }

    // Edit menu
    mbox = Menu()->FindMainMenu("edit");
    if (mbox && mbox->menu) {
        QMenu *edit_menu = new QMenu(main_win);
        edit_menu->setTitle(tr(mbox->name));
        menubar->addMenu(edit_menu);

        set(mbox->menu[editMenu], "&Edit", 0);
        set(mbox->menu[editMenu45s], "Constrain &45", 0);
        set(mbox->menu[editMenuMerge], "MergeBoxes", 0);
        set(mbox->menu[editMenuXform], "Current &Transform", "Alt+T");
        set(mbox->menu[editMenuPlace], "P&lace", "Alt+L");
        set(mbox->menu[editMenuPush], "&Push", 0);
        set(mbox->menu[editMenuPop], "P&op", 0);
        set(mbox->menu[editMenuFlatn], "&Flatten", 0);
        set(mbox->menu[editMenuCrcel], "Cre&ate Cell", 0);
        set(mbox->menu[editMenuPrpty], "Propert&ies", "Alt+P");
        set(mbox->menu[editMenuCprop], "Cell Properties", 0);

        // First elt is a dummy containing the menubar item.
        mbox->menu[0].cmd.caller = edit_menu;
        for (MenuEnt *ent = mbox->menu + 1; ent->entry; ent++) {
            ent->user_action = edit_menu->addAction(
                QString(tr(ent->menutext)));
            if (ent->is_toggle()) {
                action(ent)->setCheckable(true);
                action(ent)->setChecked(ent->is_set());
            }
            action(ent)->setShortcut(QKeySequence(ent->accel));
            action(ent)->setToolTip(QString(ent->description));
            action(ent)->setData(ent - mbox->menu);
            if (ent->is_alt())
                action(ent)->setVisible(false);
            if (ent->is_separator())
                edit_menu->addSeparator();
            ent->cmd.caller = ent->user_action;
            ent->cmd.wdesc = DSP()->MainWdesc();
        }
        connect(edit_menu, SIGNAL(triggered(QAction*)),
            this, SLOT(edit_menu_slot(QAction*)));
    }

    // Modify menu
    mbox = Menu()->FindMainMenu("mod");
    if (mbox && mbox->menu) {
        QMenu *modf_menu = new QMenu(main_win);
        modf_menu->setTitle(tr(mbox->name));
        menubar->addMenu(modf_menu);

        set(mbox->menu[modfMenu], "&Modify", 0);
        set(mbox->menu[modfMenuUndo], "&Undo", 0);
        set(mbox->menu[modfMenuRedo], "&Redo", 0);
        set(mbox->menu[modfMenuDelet], "&Delete", 0);
        set(mbox->menu[modfMenuEundr], "Erase U&nder", 0);
        set(mbox->menu[modfMenuMove], "&Move", 0);
        set(mbox->menu[modfMenuCopy], "&Copy", 0);
        set(mbox->menu[modfMenuStrch], "&Stretch", 0);
        set(mbox->menu[modfMenuChlyr], "Change La&yer", "Ctrl+L");

        // First elt is a dummy containing the menubar item.
        mbox->menu[0].cmd.caller = modf_menu;
        for (MenuEnt *ent = mbox->menu + 1; ent->entry; ent++) {
            ent->user_action = modf_menu->addAction(
                QString(tr(ent->menutext)));
            if (ent->is_toggle()) {
                action(ent)->setCheckable(true);
                action(ent)->setChecked(ent->is_set());
            }
            action(ent)->setShortcut(QKeySequence(ent->accel));
            action(ent)->setToolTip(QString(ent->description));
            action(ent)->setData(ent - mbox->menu);
            if (ent->is_alt())
                action(ent)->setVisible(false);
            if (ent->is_separator())
                modf_menu->addSeparator();
            ent->cmd.caller = ent->user_action;
            ent->cmd.wdesc = DSP()->MainWdesc();
        }
        connect(modf_menu, SIGNAL(triggered(QAction*)),
            this, SLOT(modf_menu_slot(QAction*)));
    }

    // View menu
    mbox = Menu()->FindMainMenu("view");
    if (mbox && mbox->menu) {
        QMenu *view_menu = new QMenu(main_win);
        view_menu->setTitle(tr(mbox->name));
        menubar->addMenu(view_menu);

        set(mbox->menu[viewMenu], "&View", 0);
        set(mbox->menu[viewMenuView], "Vie&w", 0);
        set(mbox->menu[viewMenuSced], "E&lectrical", 0);
        set(mbox->menu[viewMenuPhys], "P&hysical", 0);
        set(mbox->menu[viewMenuExpnd], "&Expand", 0);
        set(mbox->menu[viewMenuZoom], "&Zoom", 0);
        set(mbox->menu[viewMenuVport], "&Viewport", 0);
        set(mbox->menu[viewMenuPeek], "&Peek", 0);
        set(mbox->menu[viewMenuCsect], "&Cross Section", 0);
        set(mbox->menu[viewMenuRuler], "&Rulers", 0);
        set(mbox->menu[viewMenuInfo], "&Info", 0);
        set(mbox->menu[viewMenuAlloc], "&Allocation", 0);

        // First elt is a dummy containing the menubar item.
        mbox->menu[0].cmd.caller = view_menu;
        for (MenuEnt *ent = mbox->menu + 1; ent->entry; ent++) {
            ent->user_action = view_menu->addAction(
                QString(tr(ent->menutext)));
            if (ent->is_toggle()) {
                action(ent)->setCheckable(true);
                action(ent)->setChecked(ent->is_set());
            }
            action(ent)->setShortcut(QKeySequence(ent->accel));
            action(ent)->setToolTip(QString(ent->description));
            action(ent)->setData(ent - mbox->menu);
            if (ent->is_alt())
                action(ent)->setVisible(false);
            if (ent->is_separator())
                view_menu->addSeparator();
            ent->cmd.caller = ent->user_action;
            ent->cmd.wdesc = DSP()->MainWdesc();

            // View sub-menu.
            if (ent - mbox->menu == viewMenuView) {
                Menu()->NewDDmenu(ent->user_action, XM()->ViewList());
                QMenu *submenu = action(ent)->menu();
                connect(submenu, SIGNAL(triggered(QAction*)),
                    this, SLOT(view_view_menu_slot(QAction*)));
            }

            // Show/hide mode buttons.
            if (ent - mbox->menu == viewMenuSced)
                action(ent)->setVisible(DSP()->CurMode() == Electrical);
            if (ent - mbox->menu == viewMenuPhys)
                action(ent)->setVisible(DSP()->CurMode() == Physical);
        }
        connect(view_menu, SIGNAL(triggered(QAction*)),
            this, SLOT(view_menu_slot(QAction*)));
    }

    // Attributes menu
    mbox = Menu()->FindMainMenu("attr");
    if (mbox && mbox->menu) {
        QMenu *attr_menu = new QMenu(main_win);
        attr_menu->setTitle(tr(mbox->name));
        menubar->addMenu(attr_menu);

        set(mbox->menu[attrMenu], "&Attributes", 0);
        set(mbox->menu[attrMenuUpdat], "Save &Tech", 0);
        set(mbox->menu[attrMenuKeymp], "&Key Map", 0);
        set(mbox->menu[attrMenuFreez], "Freeze &Display", 0);
        set(mbox->menu[attrMenuAttr], "Set &Attributes", 0);
        set(mbox->menu[attrMenuCursr], "&Set Cursor", 0);
        set(mbox->menu[attrMenuFont], "Set F&ont", 0);
        set(mbox->menu[attrMenuGrid], "Set &Grid", 0);
        set(mbox->menu[attrMenuColor], "Set &Color", 0);
        set(mbox->menu[attrMenuFill], "Set &Fill", 0);
        set(mbox->menu[attrMenuEdlyr], "&Edit Layers", 0);

        // First elt is a dummy containing the menubar item.
        mbox->menu[0].cmd.caller = attr_menu;
        for (MenuEnt *ent = mbox->menu + 1; ent->entry; ent++) {
            ent->user_action = attr_menu->addAction(
                QString(tr(ent->menutext)));
            if (ent->is_toggle()) {
                action(ent)->setCheckable(true);
                action(ent)->setChecked(ent->is_set());
            }
            action(ent)->setShortcut(QKeySequence(ent->accel));
            action(ent)->setToolTip(QString(ent->description));
            action(ent)->setData(ent - mbox->menu);
            if (ent->is_alt())
                action(ent)->setVisible(false);
            if (ent->is_separator())
                attr_menu->addSeparator();
            ent->cmd.caller = ent->user_action;
            ent->cmd.wdesc = DSP()->MainWdesc();
        }
        connect(attr_menu, SIGNAL(triggered(QAction*)),
            this, SLOT(attr_menu_slot(QAction*)));
    }

    // Convert menu
    mbox = Menu()->FindMainMenu("conv");
    if (mbox && mbox->menu) {
        QMenu *cvrt_menu = new QMenu(main_win);
        cvrt_menu->setTitle(tr(mbox->name));
        menubar->addMenu(cvrt_menu);

        set(mbox->menu[cvrtMenu], "&Convert", 0);
        set(mbox->menu[cvrtMenuWrprm], "Set &Export Params", 0);
        set(mbox->menu[cvrtMenuExprt], "&Write Layout File", 0);
        set(mbox->menu[cvrtMenuRdprm], "Set &Import Params", 0);
        set(mbox->menu[cvrtMenuImprt], "&Read Layout File", 0);
        set(mbox->menu[cvrtMenuConvt], "&Conversion", 0);
        set(mbox->menu[cvrtMenuAssem], "&Assemble", 0);
        set(mbox->menu[cvrtMenuCut], "C&ut and Export", 0);
        set(mbox->menu[cvrtMenuTxted], "&Text Edito&r", 0);
        set(mbox->menu[cvrtMenuAdgds], "Edit &Parameters", 0);

        // First elt is a dummy containing the menubar item.
        mbox->menu[0].cmd.caller = cvrt_menu;
        for (MenuEnt *ent = mbox->menu + 1; ent->entry; ent++) {
            ent->user_action = cvrt_menu->addAction(
                QString(tr(ent->menutext)));
            if (ent->is_toggle()) {
                action(ent)->setCheckable(true);
                action(ent)->setChecked(ent->is_set());
            }
            action(ent)->setShortcut(QKeySequence(ent->accel));
            action(ent)->setToolTip(QString(ent->description));
            action(ent)->setData(ent - mbox->menu);
            if (ent->is_alt())
                action(ent)->setVisible(false);
            if (ent->is_separator())
                cvrt_menu->addSeparator();
            ent->cmd.caller = ent->user_action;
            ent->cmd.wdesc = DSP()->MainWdesc();
        }
        connect(cvrt_menu, SIGNAL(triggered(QAction*)),
            this, SLOT(cvrt_menu_slot(QAction*)));
    }

    // Drc menu
    mbox = Menu()->FindMainMenu("drc");
    if (mbox && mbox->menu) {
        QMenu *drc_menu = new QMenu(main_win);
        drc_menu->setTitle(tr(mbox->name));
        menubar->addMenu(drc_menu);

        set(mbox->menu[drcMenu], "&DRC", 0);
        set(mbox->menu[drcMenuClear], "&Clear Errors", "Alt+R");
        set(mbox->menu[drcMenuLimit], "&Set Defaults", 0);
        set(mbox->menu[drcMenuSflag], "Set Skip &Flags", 0);
        set(mbox->menu[drcMenuIntr], "Enable &Interactive", "Alt+I");
        set(mbox->menu[drcMenuNopop], "&No Pop-up Errors", 0);
        set(mbox->menu[drcMenuCheck], "Check In Fo&reground", 0);
        set(mbox->menu[drcMenuBgchk], "Check In Bac&kground", 0);
        set(mbox->menu[drcMenuPoint], "Check In Re&gion", 0);
        set(mbox->menu[drcMenuErdmp], "&Dump Error File", 0);
        set(mbox->menu[drcMenuNext], "Show &Errors", 0);
        set(mbox->menu[drcMenuQuery], "&Query Errors", "Alt+Q");
        set(mbox->menu[drcMenuDimen], "Edit R&ules", 0);

        // First elt is a dummy containing the menubar item.
        mbox->menu[0].cmd.caller = drc_menu;
        for (MenuEnt *ent = mbox->menu + 1; ent->entry; ent++) {
            ent->user_action = drc_menu->addAction(
                QString(tr(ent->menutext)));
            if (ent->is_toggle()) {
                action(ent)->setCheckable(true);
                action(ent)->setChecked(ent->is_set());
            }
            action(ent)->setShortcut(QKeySequence(ent->accel));
            action(ent)->setToolTip(QString(ent->description));
            action(ent)->setData(ent - mbox->menu);
            if (ent->is_alt())
                action(ent)->setVisible(false);
            if (ent->is_separator())
                drc_menu->addSeparator();
            ent->cmd.caller = ent->user_action;
            ent->cmd.wdesc = DSP()->MainWdesc();
        }
        connect(drc_menu, SIGNAL(triggered(QAction*)),
            this, SLOT(drc_menu_slot(QAction*)));
    }

    // Extract menu
    mbox = Menu()->FindMainMenu("extr");
    if (mbox && mbox->menu) {
        QMenu *ext_menu = new QMenu(main_win);
        ext_menu->setTitle(tr(mbox->name));
        menubar->addMenu(ext_menu);

        set(mbox->menu[extMenu], "E&xtract", 0);
        set(mbox->menu[extMenuViext], "&View Extraction", 0);
        set(mbox->menu[extMenuQpath], "&Quick Paths", 0);
        set(mbox->menu[extMenuPaths], "Show &Paths", 0);
        set(mbox->menu[extMenuGroup], "Show &Groups", 0);
        set(mbox->menu[extMenuNodes], "Show &Nodes", 0);
        set(mbox->menu[extMenuTshow], "Show &Terminals", 0);
        set(mbox->menu[extMenuTedit], "&Edit Terminals", 0);
        set(mbox->menu[extMenuTfind], "&Find Terminals", 0);
        set(mbox->menu[extMenuSourc], "&Source SPICE", 0);
        set(mbox->menu[extMenuExset], "Source P&hysical", 0);
        set(mbox->menu[extMenuPnet], "Dump Ph&ys Netlist", 0);
        set(mbox->menu[extMenuEnet], "Dump E&lec Netlist", 0);
        set(mbox->menu[extMenuLvs], "Dump L&VS", 0);
        set(mbox->menu[extMenuCmpar], "Compare &Devices", 0);
        set(mbox->menu[extMenuCmput], "Compute Para&ms", 0);
        set(mbox->menu[extMenuExrlc], "Extract &RLC", 0);
        set(mbox->menu[extMenuAdelp], "Edit E&xtraction", 0);

        // First elt is a dummy containing the menubar item.
        mbox->menu[0].cmd.caller = ext_menu;
        for (MenuEnt *ent = mbox->menu + 1; ent->entry; ent++) {
            ent->user_action = ext_menu->addAction(
                QString(tr(ent->menutext)));
            if (ent->is_toggle()) {
                action(ent)->setCheckable(true);
                action(ent)->setChecked(ent->is_set());
            }
            action(ent)->setShortcut(QKeySequence(ent->accel));
            action(ent)->setToolTip(QString(ent->description));
            action(ent)->setData(ent - mbox->menu);
            if (ent->is_alt())
                action(ent)->setVisible(false);
            if (ent->is_separator())
                ext_menu->addSeparator();
            ent->cmd.caller = ent->user_action;
            ent->cmd.wdesc = DSP()->MainWdesc();
        }
        connect(ext_menu, SIGNAL(triggered(QAction*)),
            this, SLOT(ext_menu_slot(QAction*)));
    }

    // User menu
    mbox = Menu()->FindMainMenu("user");
    if (mbox && mbox->menu) {
        QMenu *user_menu = new QMenu(main_win);
        user_menu->setTitle(tr(mbox->name));
        menubar->addMenu(user_menu);

        set(mbox->menu[userMenu], "&User", 0);
        set(mbox->menu[userMenuDebug], "&Debugger", 0);
        set(mbox->menu[userMenuHash], "&Rehash", 0);

        // First elt is a dummy containing the menubar item.
        mbox->menu[0].cmd.caller = user_menu;
        for (MenuEnt *ent = mbox->menu + 1; ent->entry; ent++) {
            ent->user_action = user_menu->addAction(
                QString(tr(ent->menutext)));
            if (ent->is_toggle()) {
                action(ent)->setCheckable(true);
                action(ent)->setChecked(ent->is_set());
            }
            action(ent)->setShortcut(QKeySequence(ent->accel));
            action(ent)->setToolTip(QString(ent->description));
            action(ent)->setData(ent - mbox->menu);
            if (ent->is_alt())
                action(ent)->setVisible(false);
            if (ent->is_separator())
                user_menu->addSeparator();
            ent->cmd.caller = ent->user_action;
            ent->cmd.wdesc = DSP()->MainWdesc();
        }
        connect(user_menu, SIGNAL(triggered(QAction*)),
            this, SLOT(user_menu_slot(QAction*)));
    }

    // Help menu
    mbox = Menu()->FindMainMenu("help");
    if (mbox && mbox->menu) {
        menubar->addSeparator();
        QMenu *help_menu = new QMenu(main_win);
        help_menu->setTitle(tr(mbox->name));
        menubar->addMenu(help_menu);

        set(mbox->menu[helpMenu], "&Help", 0);
        set(mbox->menu[helpMenuHelp], "&Help", "Ctrl+H");
        set(mbox->menu[helpMenuMultw], "&Multi-Window Mode", 0);
        set(mbox->menu[helpMenuAbout], "&About", 0);
        set(mbox->menu[helpMenuNotes], "&Release Notes", 0);
        set(mbox->menu[helpMenuLogs], "&Log Files", 0);

        // First elt is a dummy containing the menubar item.
        mbox->menu[0].cmd.caller = help_menu;
        for (MenuEnt *ent = mbox->menu + 1; ent->entry; ent++) {
            ent->user_action = help_menu->addAction(
                QString(tr(ent->menutext)));
            if (ent->is_toggle()) {
                action(ent)->setCheckable(true);
                action(ent)->setChecked(ent->is_set());
            }
            action(ent)->setShortcut(QKeySequence(ent->accel));
            action(ent)->setToolTip(QString(ent->description));
            action(ent)->setData(ent - mbox->menu);
            if (ent->is_alt())
                action(ent)->setVisible(false);
            if (ent->is_separator())
                help_menu->addSeparator();
            ent->cmd.caller = ent->user_action;
            ent->cmd.wdesc = DSP()->MainWdesc();
        }
        connect(help_menu, SIGNAL(triggered(QAction*)),
            this, SLOT(help_menu_slot(QAction*)));
    }
}


void
qtMenuConfig::instantiateSideMenus()
{
    mainwin *main_win = mainBag();
    if (!main_win)
        return;

    QWidget *phys_button_box = main_win->PhysButtonBox();
    MenuBox *mbox = Menu()->GetPhysSideMenu();
    if (phys_button_box && mbox && mbox->menu) {

        if (DSP()->CurMode() != Physical)
            phys_button_box->hide();

        // Physical side menu
        QVBoxLayout *vbox = new QVBoxLayout(phys_button_box);
        vbox->setMargin(2);
        vbox->setSpacing(2);
        phys_button_box->setMinimumWidth(30);
        phys_button_box->setMaximumWidth(30);

        set(mbox->menu[sidePhysMenu], 0, 0);
        set(mbox->menu[sidePhysMenuLabel], "Labels", 0);
        set(mbox->menu[sidePhysMenuLogo], "Logo", 0);
        set(mbox->menu[sidePhysMenuBox], "Boxes", 0);
        set(mbox->menu[sidePhysMenuPolyg], "Polygons", 0);
        set(mbox->menu[sidePhysMenuWire], "Wires", 0);
        set(mbox->menu[sidePhysMenuStyle], "Style", 0);
        set(mbox->menu[sidePhysMenuRound], "Round", 0);
        set(mbox->menu[sidePhysMenuDonut], "Donut", 0);
        set(mbox->menu[sidePhysMenuArc], "Arc", 0);
        set(mbox->menu[sidePhysMenuSides], "Sides", 0);
        set(mbox->menu[sidePhysMenuXor], "Xor", 0);
        set(mbox->menu[sidePhysMenuBreak], "Break", 0);
        set(mbox->menu[sidePhysMenuErase], "Erase", 0);
        set(mbox->menu[sidePhysMenuPut], "Put", 0);
        set(mbox->menu[sidePhysMenuSpin], "Spin", 0);

        for (MenuEnt *ent = mbox->menu + 1; ent->entry; ent++) {
            menu_button *b = new menu_button(ent, phys_button_box);
            b->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
            ent->cmd.caller = b;
            if (ent - mbox->menu == sidePhysMenuStyle) {
                const char **spm = get_style_pixmap();
                b->setIcon(QPixmap(spm));
            }
            else {
                if (ent->xpm)
                    b->setIcon(QPixmap(ent->xpm));
            }
            vbox->addWidget(b);
            ent->cmd.caller = b;
            ent->cmd.wdesc = DSP()->MainWdesc();

            if (ent - mbox->menu == sidePhysMenuStyle) {
                connect(b, SIGNAL(button_pressed(MenuEnt*)),
                    this, SLOT(style_slot(MenuEnt*)));
            }
            else {
                connect(b, SIGNAL(button_pressed(MenuEnt*)),
                    this, SLOT(exec_slot(MenuEnt*)));
            }
        }
        if (EditIf()->styleList()) {
            style_menu = new QMenu(mainBag());
            for (const char *const *s = EditIf()->styleList(); *s; s++)
                style_menu->addAction(tr(*s));

            connect(style_menu, SIGNAL(triggered(QAction*)),
                this, SLOT(style_menu_slot(QAction*)));
        }
    }

    QWidget *elec_button_box = main_win->ElecButtonBox();
    mbox = Menu()->GetElecSideMenu();
    if (elec_button_box && mbox && mbox->menu) {

        if (DSP()->CurMode() != Electrical)
            elec_button_box->hide();

        // Electrical side menu
        QVBoxLayout *vbox = new QVBoxLayout(elec_button_box);
        vbox->setMargin(2);
        vbox->setSpacing(2);
        elec_button_box->setMinimumWidth(30);
        elec_button_box->setMaximumWidth(30);

        set(mbox->menu[sideElecMenu], 0, 0);
        set(mbox->menu[sideElecMenuDevs], "Devices", 0);
        set(mbox->menu[sideElecMenuShape], "Shape", 0);
        set(mbox->menu[sideElecMenuWire], "Wire", 0);
        set(mbox->menu[sideElecMenuLabel], "Label", 0);
        set(mbox->menu[sideElecMenuErase], "Erase", 0);
        set(mbox->menu[sideElecMenuBreak], "Break", 0);
        set(mbox->menu[sideElecMenuSymbl], "Symbolic", 0);
        set(mbox->menu[sideElecMenuNodmp], "Node Map", 0);
        set(mbox->menu[sideElecMenuSubct], "Subcircuit", 0);
        set(mbox->menu[sideElecMenuTerms], "Show Terms", 0);
        set(mbox->menu[sideElecMenuDots], "Show Dots", 0);
        set(mbox->menu[sideElecMenuRun], "Run", 0);
        set(mbox->menu[sideElecMenuDeck], "Dump Deck", 0);
        set(mbox->menu[sideElecMenuPlot], "Plot", 0);
        set(mbox->menu[sideElecMenuIplot], "Intr Plot", 0);

        for (MenuEnt *ent = mbox->menu + 1; ent->entry; ent++) {
            menu_button *b = new menu_button(ent, elec_button_box);
            b->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
            ent->cmd.caller = b;
            if (ent->xpm)
                b->setIcon(QPixmap(ent->xpm));
            vbox->addWidget(b);
            ent->cmd.caller = b;
            ent->cmd.wdesc = DSP()->MainWdesc();

            if (ent - mbox->menu == sideElecMenuShape) {
                connect(b, SIGNAL(button_pressed(MenuEnt*)),
                    this, SLOT(shape_slot(MenuEnt*)));
            }
            else {
                connect(b, SIGNAL(button_pressed(MenuEnt*)),
                    this, SLOT(exec_slot(MenuEnt*)));
            }
        }
        if (ScedIf()->shapesList()) {
            shape_menu = new QMenu(mainBag());
            for (const char *const *s = ScedIf()->shapesList(); *s; s++)
                shape_menu->addAction(tr(*s));

            connect(shape_menu, SIGNAL(triggered(QAction*)),
                this, SLOT(shape_menu_slot(QAction*)));
        }
    }
}


void
qtMenuConfig::instantiateSubwMenus(int wnum)
{
    WindowDesc *wdesc = DSP()->Window(wnum);
    if (!wdesc)
        return;
    subwin_d *sub_win = dynamic_cast<subwin_d*>(wdesc->Wbag());
    if (!sub_win)
        return;
    QMenuBar *menubar = sub_win->MenuBar();
    if (!menubar)
        return;

    // Encode window number in QAccess data.
    int wincode = wnum << 16;

    // Subwin View Menu
    MenuBox *mbox = Menu()->FindSubwMenu("view", wnum);
    if (mbox && mbox->menu) {

        set(mbox->menu[subwViewMenu], "&View", 0);
        set(mbox->menu[subwViewMenuView], "&View", 0);
        set(mbox->menu[subwViewMenuSced], "E&lectrical", 0);
        set(mbox->menu[subwViewMenuPhys], "P&hysical", 0);
        set(mbox->menu[subwViewMenuExpnd], "&Expand", 0);
        set(mbox->menu[subwViewMenuZoom], "&Zoom", 0);
        set(mbox->menu[subwViewMenuWdump], "&Dump To File", 0);
        set(mbox->menu[subwViewMenuLshow], "&Show Location", 0);
        set(mbox->menu[subwViewMenuSwap], "Swap With &Main", 0);
        set(mbox->menu[subwViewMenuLoad], "Load &New", 0);
        set(mbox->menu[subwViewMenuCancl], "&Quit", "Ctrl+Q");

        QMenu *subwin_view_menu = new QMenu(sub_win);
        subwin_view_menu->setTitle(tr(mbox->name));
        menubar->addMenu(subwin_view_menu);

        // First elt is a dummy containing the menubar item.
        mbox->menu[0].cmd.caller = subwin_view_menu;
        for (MenuEnt *ent = mbox->menu + 1; ent->entry; ent++) {
            ent->user_action = subwin_view_menu->addAction(
                QString(tr(ent->menutext)));
            if (ent->is_toggle()) {
                action(ent)->setCheckable(true);
                action(ent)->setChecked(ent->is_set());
            }
            action(ent)->setShortcut(QKeySequence(ent->accel));
            action(ent)->setToolTip(QString(ent->description));
            action(ent)->setData((ent - mbox->menu) | wincode);
            if (ent->is_alt())
                action(ent)->setVisible(false);
            if (ent->is_separator())
                subwin_view_menu->addSeparator();
            ent->cmd.caller = ent->user_action;
            ent->cmd.wdesc = wdesc;

            // View sub-menu.
            if (ent - mbox->menu == subwViewMenuView) {
                Menu()->NewDDmenu(ent->user_action, XM()->ViewList());
                QMenu *submenu = action(ent)->menu();

                // Encode the window number.
                QList<QAction*> lst = submenu->actions();
                for (int i = 0; i < lst.size(); i++)
                    lst.value(i)->setData(wincode);

                connect(submenu, SIGNAL(triggered(QAction*)),
                    this, SLOT(subwin_view_view_menu_slot(QAction*)));
            }

            // Show/hide mode buttons.
            if (ent - mbox->menu == subwViewMenuSced)
                action(ent)->setVisible(DSP()->CurMode() == Electrical);
            if (ent - mbox->menu == subwViewMenuPhys)
                action(ent)->setVisible(DSP()->CurMode() == Physical);
        }

        connect(subwin_view_menu, SIGNAL(triggered(QAction*)),
            this, SLOT(subwin_view_menu_slot(QAction*)));
    }

    // Subwin Attr Menu
    mbox = Menu()->FindSubwMenu("attr", wnum);
    if (mbox && mbox->menu) {

        set(mbox->menu[subwAttrMenu], "&Attributes", 0);
        set(mbox->menu[subwAttrMenuFreez], "Freeze &Display", 0);
        set(mbox->menu[subwAttrMenuCntxt], "Show Conte&xt", 0);
        set(mbox->menu[subwAttrMenuProps], "Show &Phys Properties", 0);
        set(mbox->menu[subwAttrMenuLabls], "Show &Labels", 0);
        set(mbox->menu[subwAttrMenuLarot], "L&abel True Orient", 0);
        set(mbox->menu[subwAttrMenuCnams], "Show Cell &Names", 0);
        set(mbox->menu[subwAttrMenuCnrot], "Cell Na&me True Orient", 0);
        set(mbox->menu[subwAttrMenuNouxp], "Don't Show &Unexpanded", 0);
        set(mbox->menu[subwAttrMenuTinyb], "Subthreshold &Boxes", 0);
        set(mbox->menu[subwAttrMenuGrid], "Set &Grid", 0);

        QMenu *subwin_attr_menu = new QMenu(sub_win);
        subwin_attr_menu->setTitle(tr(mbox->name));
        menubar->addMenu(subwin_attr_menu);

        // First elt is a dummy containing the menubar item.
        mbox->menu[0].cmd.caller = subwin_attr_menu;
        for (MenuEnt *ent = mbox->menu + 1; ent->entry; ent++) {
            ent->user_action = subwin_attr_menu->addAction(
                QString(tr(ent->menutext)));
            if (ent->is_toggle()) {
                action(ent)->setCheckable(true);
                action(ent)->setChecked(ent->is_set());
            }
            action(ent)->setShortcut(QKeySequence(ent->accel));
            action(ent)->setToolTip(QString(ent->description));
            action(ent)->setData((ent - mbox->menu) | wincode);
            if (ent->is_alt())
                action(ent)->setVisible(false);
            if (ent->is_separator())
                subwin_attr_menu->addSeparator();
            ent->cmd.caller = ent->user_action;
            ent->cmd.wdesc = wdesc;
        }

        connect(subwin_attr_menu, SIGNAL(triggered(QAction*)),
            this, SLOT(subwin_attr_menu_slot(QAction*)));
    }

    // Subwin Help Menu
    mbox = Menu()->FindSubwMenu("help", wnum);
    if (mbox && mbox->menu) {

        set(mbox->menu[subwHelpMenu], "&Help", 0);
        set(mbox->menu[subwHelpMenuHelp], "&Help", "Ctrl+H");

        QMenu *subwin_help_menu = new QMenu(sub_win);
        subwin_help_menu->setTitle(tr(mbox->name));
        menubar->addSeparator();
        menubar->addMenu(subwin_help_menu);

        // First elt is a dummy containing the menubar item.
        mbox->menu[0].cmd.caller = subwin_help_menu;
        for (MenuEnt *ent = mbox->menu + 1; ent->entry; ent++) {
            ent->user_action = subwin_help_menu->addAction(
                QString(tr(ent->menutext)));
            if (ent->is_toggle()) {
                action(ent)->setCheckable(true);
                action(ent)->setChecked(ent->is_set());
            }
            action(ent)->setShortcut(QKeySequence(ent->accel));
            action(ent)->setToolTip(QString(ent->description));
            action(ent)->setData((ent - mbox->menu) | wincode);
            if (ent->is_alt())
                action(ent)->setVisible(false);
            if (ent->is_separator())
                subwin_help_menu->addSeparator();
            ent->cmd.caller = ent->user_action;
            ent->cmd.wdesc = wdesc;
        }

        connect(subwin_help_menu, SIGNAL(triggered(QAction*)),
            this, SLOT(subwin_help_menu_slot(QAction*)));
    }
}


void
qtMenuConfig::updateDynamicMenus()
{
    /*
    GtkItemFactory *item_factory = gtkMenu()->itemFactory;
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
            mi[j++] = if_entry(ent->menutext, ent->accel,
                i ? HANDLED : 0, i, ent->item);
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

        // Add the dynamic entries.
        cnt = 0;
        for (MenuEnt *ent = &mbox->menu[userMenu_END]; ent->entry; ent++) {
            GtkItemFactoryEntry entry;
            char buf[256];
            // The name may have underscores, which we don't want
            // to be taken as accelerator keys.  Have to hack
            // around this.
            bool name_hack = false;
            entry.path = (gchar*)ent->menutext;
            if (strchr(entry.path, '_')) {
                strcpy(buf, entry.path);
                // change underscores to hyphens
                for (char *s = buf; *s; s++) {
                    if (*s == '_')
                        *s = '-';
                }
                entry.path = buf;
                name_hack = true;
            }
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
            char *tmp = gtkMenu()->strip_accel(entry.path);
            GtkWidget *btn = gtk_item_factory_get_widget(item_factory, tmp);
            delete [] tmp;
            if (btn) {
                if (name_hack) {
                    // btn just added, set "real" label
                    char *t = strrchr(ent->menutext, '/');
                    if (t)
                        Menu()->SetLabel(btn, t+1);
                }
                ent->cmd.caller = btn;
            }
            cnt++;
        }
    }
    */
}


void
qtMenuConfig::switch_menu_mode(DisplayMode mode, int wnum)
{
    if (wnum == 0) {
        mainwin *main_win = mainBag();
        if (!main_win)
            return;
        QWidget *phys_button_box = main_win->PhysButtonBox();
        QWidget *elec_button_box = main_win->ElecButtonBox();

        if (mode == Physical) {
            MenuBox *mbox = Menu()->FindMainMenu("view");
            if (mbox && mbox->menu) {
                action(&mbox->menu[viewMenuPhys])->setVisible(false);
                action(&mbox->menu[viewMenuSced])->setVisible(true);
            }

            // Desensitize the DRC menu in electrical mode.
            mbox = Menu()->FindMainMenu("drc");
            if (mbox && mbox->menu) {
                QWidget *w = (QWidget*)mbox->menu[0].cmd.caller;
                if (w)
                    w->setEnabled(true);
            }

            if (elec_button_box)
                elec_button_box->hide();
            if (phys_button_box)
                phys_button_box->show();
        }
        else {
            MenuBox *mbox = Menu()->FindMainMenu("view");
            if (mbox && mbox->menu) {
                action(&mbox->menu[viewMenuSced])->setVisible(false);
                action(&mbox->menu[viewMenuPhys])->setVisible(true);
            }

            // Desensitize the DRC menu in electrical mode.
            mbox = Menu()->FindMainMenu("drc");
            if (mbox && mbox->menu) {
                QWidget *w = (QWidget*)mbox->menu[0].cmd.caller;
                if (w)
                    w->setEnabled(false);
            }

            if (phys_button_box)
                phys_button_box->hide();
            if (elec_button_box)
                elec_button_box->show();
        }
    }
    else {
        if (wnum >= DSP_NUMWINS)
            return;
        if (!DSP()->Window(wnum))
            return;

        MenuBox *mbox = Menu()->FindSubwMenu("view", wnum);
        if (mbox && mbox->menu) {
            if (mode == Physical) {
                action(&mbox->menu[subwViewMenuSced])->setVisible(true);
                action(&mbox->menu[subwViewMenuPhys])->setVisible(false);
            }
            else {
                action(&mbox->menu[subwViewMenuSced])->setVisible(false);
                action(&mbox->menu[subwViewMenuPhys])->setVisible(true);
            }
        }
    }
}


// Turn on/off sensitivity of all menus in the main window.
//
void
qtMenuConfig::set_main_global_sens(const MenuList *list, bool sens)
{
    /*
    GtkItemFactory *item_factory = gtkMenu()->itemFactory;
    if (!item_factory)
        return;
    for (const MenuList *ml = list; ml; ml = ml->next) {
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
                "/Attributes/Freeze Display");
            if (widget)
                gtk_widget_set_sensitive(GTK_WIDGET(widget), true);
        }
        else
            gtk_widget_set_sensitive(widget, sens);
    }
    */
}


//-----------------------------------------------------------------------
// Private Slots

// File Menu slot.
//
void
qtMenuConfig::file_menu_slot(QAction *a)
{
    int i = a->data().toInt();
    MenuBox *mbox = Menu()->FindMainMenu("file");
    if (!mbox)
        return;
    MenuEnt *ent = &mbox->menu[i];
    ent->cmd.caller = ent->user_action;
    emit exec_slot(ent);
}


// File/Edit submenu slot.
//
void
qtMenuConfig::file_open_menu_slot(QAction *a)
{
    if (XM()->IsDoingHelp()) {
        int mstate = QApplication::keyboardModifiers();
        if (!(mstate & Qt::ShiftModifier)) {
            DSPmainWbag(PopUpHelp("xic:open"))
            return;
        }
    }

    const char *string = a->text().toAscii().constData();
    if (!string || !*string)
        return;

    int mstate = QApplication::keyboardModifiers();
    XM()->HandleOpenCellMenu(string, (mstate & Qt::ShiftModifier));
}


// Edit Menu slot.
//
void
qtMenuConfig::edit_menu_slot(QAction *a)
{
    int i = a->data().toInt();
    MenuBox *mbox = Menu()->FindMainMenu("edit");
    if (!mbox)
        return;
    MenuEnt *ent = &mbox->menu[i];
    ent->cmd.caller = ent->user_action;
    emit exec_slot(ent);
}


// Modify Menu slot.
//
void
qtMenuConfig::modf_menu_slot(QAction *a)
{
    int i = a->data().toInt();
    MenuBox *mbox = Menu()->FindMainMenu("mod");
    if (!mbox)
        return;
    MenuEnt *ent = &mbox->menu[i];
    ent->cmd.caller = ent->user_action;
    emit exec_slot(ent);
}


// View Menu slot.
//
void
qtMenuConfig::view_menu_slot(QAction *a)
{
    int i = a->data().toInt();
    MenuBox *mbox = Menu()->FindMainMenu("view");
    if (!mbox)
        return;
    MenuEnt *ent = &mbox->menu[i];
    ent->cmd.caller = ent->user_action;
    emit exec_slot(ent);
}


// View/View submenu slot.
//
void
qtMenuConfig::view_view_menu_slot(QAction *a)
{
    if (XM()->IsDoingHelp()) {
        int mstate = QApplication::keyboardModifiers();
        if (!(mstate & Qt::ShiftModifier)) {
            DSPmainWbag(PopUpHelp("xic:view"))
            return;
        }
    }
    const char *string = Menu()->GetLabel(a);
    if (!string || !*string)
        return;
    DSP()->MainWdesc()->SetView(string);
}


// Attributes Menu slot.
//
void
qtMenuConfig::attr_menu_slot(QAction *a)
{
    int i = a->data().toInt();
    MenuBox *mbox = Menu()->FindMainMenu("attr");
    if (!mbox)
        return;
    MenuEnt *ent = &mbox->menu[i];
    ent->cmd.caller = ent->user_action;
    emit exec_slot(ent);
}


// Convert Menu slot.
//
void
qtMenuConfig::cvrt_menu_slot(QAction *a)
{
    int i = a->data().toInt();
    MenuBox *mbox = Menu()->FindMainMenu("conv");
    if (!mbox)
        return;
    MenuEnt *ent = &mbox->menu[i];
    ent->cmd.caller = ent->user_action;
    emit exec_slot(ent);
}


// DRC Menu slot.
//
void
qtMenuConfig::drc_menu_slot(QAction *a)
{
    int i = a->data().toInt();
    MenuBox *mbox = Menu()->FindMainMenu("drc");
    if (!mbox)
        return;
    MenuEnt *ent = &mbox->menu[i];
    ent->cmd.caller = ent->user_action;
    emit exec_slot(ent);
}


// Extract Menu slot.
//
void
qtMenuConfig::ext_menu_slot(QAction *a)
{
    int i = a->data().toInt();
    MenuBox *mbox = Menu()->FindMainMenu("ext");
    if (!mbox)
        return;
    MenuEnt *ent = &mbox->menu[i];
    ent->cmd.caller = ent->user_action;
    emit exec_slot(ent);
}


// Debug Menu slot.
//
void
qtMenuConfig::user_menu_slot(QAction *a)
{
    int i = a->data().toInt();
    MenuBox *mbox = Menu()->FindMainMenu("user");
    if (!mbox)
        return;
    MenuEnt *ent = &mbox->menu[i];
    ent->cmd.caller = ent->user_action;
    emit exec_slot(ent);
}


// Help Menu slot.
//
void
qtMenuConfig::help_menu_slot(QAction *a)
{
    int i = a->data().toInt();
    MenuBox *mbox = Menu()->FindMainMenu("help");
    if (!mbox)
        return;
    MenuEnt *ent = &mbox->menu[i];
    ent->cmd.caller = ent->user_action;
    emit exec_slot(ent);
}


// Subwindow View Menu slot.
//
void
qtMenuConfig::subwin_view_menu_slot(QAction *a)
{
    int i = a->data().toInt();
    int wnum = i >> 16;  // Decode window number.
    i &= 0xffff;
    MenuBox *mbox = Menu()->FindSubwMenu("view", wnum);
    if (!mbox)
        return;
    MenuEnt *ent = &mbox->menu[i];
    ent->cmd.caller = ent->user_action;
    if (i == subwViewMenuCancl)
        // can't destroy here, defer
        emit exec_idle(ent);
    else
        emit exec_slot(ent);
}


// Subwindow View/View submenu slot.
//
void
qtMenuConfig::subwin_view_view_menu_slot(QAction *a)
{
    if (XM()->IsDoingHelp()) {
        int mstate = QApplication::keyboardModifiers();
        if (!(mstate & Qt::ShiftModifier)) {
            DSPmainWbag(PopUpHelp("xic:view"))
            return;
        }
    }
    const char *string = Menu()->GetLabel(a);
    if (!string || !*string)
        return;
    DSP()->MainWdesc()->SetView(string);

    int i = a->data().toInt();
    int wnum = i >> 16;  // Decode window number.
    if (wnum <= 0 || wnum >= DSP_NUMWINS)
        return;
    WindowDesc *wdesc = DSP()->Window(wnum);
    if (wdesc)
        wdesc->SetView(string);
}


// Subwindow Attributes Menu slot.
//
void
qtMenuConfig::subwin_attr_menu_slot(QAction *a)
{
    int i = a->data().toInt();
    int wnum = i >> 16;  // Decode window number.
    i &= 0xffff;
    MenuBox *mbox = Menu()->FindSubwMenu("attr", wnum);
    if (!mbox)
        return;
    MenuEnt *ent = &mbox->menu[i];
    ent->cmd.caller = ent->user_action;
    emit exec_slot(ent);
}


// Subwindow Help Menu slot.
//
void
qtMenuConfig::subwin_help_menu_slot(QAction *a)
{
    int i = a->data().toInt();
    int wnum = i >> 16;  // Decode window number.
    i &= 0xffff;
    MenuBox *mbox = Menu()->FindSubwMenu("help", wnum);
    if (!mbox)
        return;
    MenuEnt *ent = &mbox->menu[i];
    ent->cmd.caller = ent->user_action;
    emit exec_slot(ent);
}


void
qtMenuConfig::idle_exec_slot(MenuEnt *ent)
{
    if (ent->action) {
        mainBag()->ShowGhost(ERASE);
        (*ent->action)(&ent->cmd);
        mainBag()->ShowGhost(DISPLAY);
    }
}


void
qtMenuConfig::exec_slot(MenuEnt *ent)
{
    if (ent->type == CMD_NOTSAFE) {
        // hack for the "run" button
        if (!strcmp(ent->entry, MenuRUN) && ScedIf()->simulationActive()) {
            (*ent->action)(&ent->cmd);
            return;
        }
        bool state = Menu()->GetStatus(ent->cmd.caller);
        // If the state is false, then we have reentered the command,
        // terminate it and exit.  Otherwise, a new command was
        // selected.  Terminate the present command, and call the
        // new one.
        EV()->InitCallback();
        // consistency check
        Selections.check();
        if (state)
            // putting the call in an idle proc allows the current command
            // function to return before the new one starts
            emit exec_idle(ent);
        return;
    }
    idle_exec_slot(ent);
}


void
qtMenuConfig::style_slot(MenuEnt *ent)
{
    if (style_menu) {
        QWidget *btn = (QWidget*)ent->cmd.caller;
        if (!btn)
            return;
        QPoint pt = btn->pos();
        pt.setX(pt.x() + btn->width());
        style_menu->exec(btn->parentWidget()->mapToGlobal(pt));
    }
}


void
qtMenuConfig::shape_slot(MenuEnt *ent)
{
    if (style_menu) {
        QWidget *btn = (QWidget*)ent->cmd.caller;
        if (!btn)
            return;
        QPoint pt = btn->pos();
        pt.setX(pt.x() + btn->width());
        shape_menu->exec(btn->parentWidget()->mapToGlobal(pt));
    }
}


void
qtMenuConfig::style_menu_slot(QAction *a)
{
    const char *string = a->text().toAscii().constData();
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
        int mstate = QApplication::keyboardModifiers();
        if (!(mstate & Qt::ShiftModifier)) {
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

    MenuBox *mbox = Menu()->GetPhysSideMenu();
    if (mbox && mbox->menu) {
        menu_button *b =
            (menu_button*)mbox->menu[sidePhysMenuStyle].cmd.caller;
        if (b) {
            const char **spm = get_style_pixmap();
            b->setIcon(QPixmap(spm));
        }
    }
}


void
qtMenuConfig::shape_menu_slot(QAction *a)
{
    const char *string = a->text().toAscii().constData();
    if (!string || !*string)
        return;
    if (!ScedIf()->shapesList())
        return;

    if (XM()->IsDoingHelp()) {
        int mstate = QApplication::keyboardModifiers();
        if (!(mstate & Qt::ShiftModifier)) {
            char buf[64];
            sprintf(buf, "shapes:%s", string);
            DSPmainWbag(PopUpHelp(buf))
            return;
        }
    }
    EV()->InitCallback();
    if (!strcmp(string, "sides")) {
        CmdDesc cmd;
        cmd.caller = a;
        cmd.wdesc = DSP()->MainWdesc();
        EditIf()->sidesExec(&cmd);
        return;
    }
    int i = 0;
    for (const char *const *s = ScedIf()->shapesList(); *s; s++, i++) {
        if (!strcmp(*s, string))
            break;
    }
    ScedIf()->addShape(i);
}


// Static function.
const char **
qtMenuConfig::get_style_pixmap()
{
    if (EditIf()->getWireStyle() == CDWIRE_FLUSH)
        return (style_f_xpm);
    else if (EditIf()->getWireStyle() == CDWIRE_ROUND)
        return (style_r_xpm);
    // CDWIRE_EXTEND
    return (style_e_xpm);
}


#ifdef notdef

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
    if (Menu()->IsMainMenu(ent) || Menu()->IsSideMenu(ent) ||
            Menu()->IsSubwMenu(ent)) {
        unsigned i;
        for (i = 0; ent[i].entry; i++);
        if (action >= i)
            return;
    }
    else
        return;

    ent += action;
    ent->cmd.caller = caller;
    if (!ent->cmd.wdesc)
        ent->cmd.wdesc = DSP()->MainWdesc();

    char buf[128];
    if (XM()->IsDoingHelp()) {
        int x, y;
        GdkModifierType mstate;
        gdk_window_get_pointer(GTKdev::DefaultFocusWin(), &x, &y, &mstate);
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
            // put the button back the way it was
            state = !state;
            Menu()->SetStatus(caller, state);
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

    if (ent->type == CMD_NOTSAFE) {
        // hack for the "run" button
        if (!strcmp(ent->entry, MenuRUN) && ScedIf()->simulationActive()) {
            if (ent->action)
                (*ent->action)(&ent->cmd);
            return;
        }

        // Terminate present command state.
        EV()->InitCallback();
        Selections.check(); // consistency check

        // Putting the call in a timeout proc allows the current
        // command to return and finish before the new one starts (see
        // below).

        if (Menu()->GetStatus((GtkWidget*)ent->cmd.caller))
            gtk_timeout_add(50, cmd_proc, ent);
        else if (!strcmp(ent->entry, MenuIPLOT))
            // Hack for the "iplot" button
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
    DSP()->SetInterrupt(false);
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
        CDs *cursd = CurCell();
        if (cursd) {
            EditIf()->ulListCheck(ent->entry, cursd, false);
            SI()->Interpret(sfp, wl, 0, 0);
            if (sfp)
                delete sfp;
            if (EditIf()->ulHasChanged()) {
                BBox BB;
                if (EditIf()->ulCommitChanges(false, &BB, false))
                    DSP()->RedisplayArea(&BB);
            }
        }
    }
    return (false);
}


// Static function.
int
gtkMenuConfig::cmd_proc(void *arg)
{
    if (dspPkgIf()->IsBusy())
        return (true);
    DSP()->SetInterrupt(false);
    MenuEnt *ent = (MenuEnt*)arg;
    if (mainBag()) {
        if (ent->action) {
            mainBag()->ShowGhost(ERASE);
            (*ent->action)(&ent->cmd);
            mainBag()->ShowGhost(DISPLAY);
        }
    }
    return (false);
}


// Static function.
// Add a set of entries the the item factory, extracting the just-added
// widget into the CmdDesc of the related menu.
//
void
gtkMenuConfig::make_entries(GtkItemFactory *item_factory,
    GtkItemFactoryEntry *entries, int nitems, MenuEnt *menu, int menuitems)
{
    for (int i = 0; i < nitems; i++) {
        if (entries[i].callback == HANDLED)
            entries[i].callback = HANDLER;
        else if (entries[i].callback_action != 0) {
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

#endif


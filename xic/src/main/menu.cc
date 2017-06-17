
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
 $Id: menu.cc,v 5.123 2017/04/16 20:28:12 stevew Exp $
 *========================================================================*/

#include "main.h"
#include "dsp_inlines.h"
#include "menu.h"
#include "scedif.h"
#include "editif.h"
#include "extif.h"
#include "errorlog.h"

// Help keywords used in this file
//  xic:vport
//  xicinfo
//  mainwindow
//  subwindow


// Copy a MenuBox and menu.
//
MenuBox *
MenuBox::copy_to(int wnum, MenuBox *nbox)
{
    if (!nbox)
        nbox = new MenuBox(*this);
    else
        *nbox = *this;
    nbox->menu = 0;
    int msz = 0;
    for (MenuEnt *ent = menu; ent && ent->entry; ent++)
        msz++;
    if (msz) {
        msz++;
        nbox->menu = new MenuEnt[msz];
        msz = 0;
        for (MenuEnt *ent = menu; ent && ent->entry; ent++) {
            nbox->menu[msz] = *ent;
            nbox->menu[msz].cmd.wdesc = DSP()->Window(wnum);
            msz++;
        }
    }
    return (nbox);
}
// End of MenuBox functions.


MenuMain *MenuMain::instancePtr = 0;

MenuMain::MenuMain()
{
    if (instancePtr) {
        fprintf(stderr, "Singleton class MenuMain already instantiated.\n");
        exit(1);
    }
    instancePtr = this;

    mm_menus = 0;
    mm_subw_template = 0;
    for (int i = 0; i < DSP_NUMWINS; i++) {
        mm_subw_menus[i] = 0;
        mm_subw_obj_menus[i] = 0;
    }
    mm_phys_button_menu = 0;
    mm_elec_button_menu = 0;
    mm_misc_menu = 0;
    mm_attr_submenu = 0;
    mm_obj_submenu = 0;
    mm_insensitive = false;
}


// Private static error exit.
//
void
MenuMain::on_null_ptr()
{
    fprintf(stderr, "Singleton class MenuMain used before instantiated.\n");
    exit(1);
}


// This is called before every command is dispatched.  Special oddball
// things that have to be done for particular commands are done here. 
// If false is returned, the command processing is aborted.  If
// call_on_up ia returned true, the command callback for NOTSAFE
// commands will also be called on button-up.
//
bool
MenuMain::SetupCommand(MenuEnt *ent, bool *call_on_up)
{
    *call_on_up = false;
    if (!ent)
        return (false);

    bool rv, cu;
    if (ScedIf()->setupCommand(ent, &rv, &cu)) {
        *call_on_up = cu;
        return (rv);
    }
    if (ExtIf()->setupCommand(ent, &rv, &cu)) {
        *call_on_up = cu;
        return (rv);
    }
    if (XM()->setupCommand(ent, &rv, &cu)) {
        *call_on_up = cu;
        return (rv);
    }

    return (true);
}


void
MenuMain::RegisterMenu(MenuBox *mbox)
{
    if (!mbox)
        return;
    for (MenuList *ml = mm_menus; ml; ml = ml->next) {
        if (ml->menubox == mbox)
            return;
    }
    MenuList *ml = new MenuList(mbox);
    if (!mm_menus)
        mm_menus = ml;
    else {
        MenuList *end = mm_menus;
        while (end->next)
            end = end->next;
        end->next = ml;
        ml->prev = end;
    }
    if (lstring::ciprefix("attr", mbox->name)) {
        // This is the Attributes menu, copy the sub-menu which is the
        // same as the viewport Attributes menu.

        mm_attr_submenu = XM()->createSubwAttrMenu()->copy_to(0);
        mm_obj_submenu = XM()->createObjSubMenu()->copy_to(0);
    }
}


void
MenuMain::RegisterButtonMenu(MenuBox *mbox, DisplayMode mode)
{
    if (mode == Physical)
        mm_phys_button_menu = mbox;
    else
        mm_elec_button_menu = mbox;

}


void
MenuMain::RegisterSubwMenu(MenuBox *mbox)
{
    if (!mbox)
        return;
    for (MenuList *ml = mm_subw_template; ml; ml = ml->next) {
        if (ml->menubox == mbox)
            return;
    }
    MenuList *ml = new MenuList(mbox);
    if (!mm_subw_template)
        mm_subw_template = ml;
    else {
        MenuList *end = mm_subw_template;
        while (end->next)
            end = end->next;
        end->next = ml;
        ml->prev = end;
    }
}


void
MenuMain::RegisterMiscMenu(MenuBox *mbox)
{
    mm_misc_menu = mbox;
}


// Reset the entries in the main menu, and partially reinitialize internal
// variables.  This is called upon switching between schematic and physical
// layout modes.
//
void
MenuMain::CleanupBeforeModeSwitch()
{
    for (MenuList *l = mm_menus; l; l = l->next)
        l->menubox->cleanupBeforeModeSwitch(0);
    if (GetButtonMenu())
        GetButtonMenu()->cleanupBeforeModeSwitch(0);
}


// Reset the state variables in the menus to correspond to internal
// application variables.  This should be called on creation and after
// a mode switch.
//
void
MenuMain::InitAfterModeSwitch(int wnum)
{
    if (wnum < 0 || wnum >= DSP_NUMWINS)
        return;
    if (wnum == 0) {
        for (MenuList *l = mm_menus; l; l = l->next)
            l->menubox->initAfterModeSwitch(0);
        if (GetAttrSubMenu())
            GetAttrSubMenu()->initAfterModeSwitch(0);
        if (GetObjSubMenu())
            GetObjSubMenu()->initAfterModeSwitch(0);
        if (GetButtonMenu())
            GetButtonMenu()->initAfterModeSwitch(0);
    }
    else {
        for (MenuBox *m = mm_subw_menus[wnum]; m->name; m++)
            m->initAfterModeSwitch(wnum);
        mm_subw_obj_menus[wnum]->initAfterModeSwitch(wnum);
    }
}


// Set the sensitivity of the Undo/Redo and all of the CMD_PROMPT menu
// entries.  Since the prompt line editor is in use, the CMD_PROMPT
// commands will otherwise return, doing nothing.
//
void
MenuMain::SetUndoSens(bool sens)
{
    for (MenuList *l = mm_menus; l; l = l->next) {
        if (!l->menubox || !l->menubox->menu)
            continue;
        for (MenuEnt *ent = l->menubox->menu; ent->entry; ent++) {
            if (ent->cmd.caller && (ent->type == CMD_UNDO ||
                    ent->type == CMD_REDO || ent->type == CMD_PROMPT))
                SetSensitive(ent->cmd.caller, sens);
        }
    }
    for (int i = 1; i < DSP_NUMWINS; i++) {
        for (MenuBox *m = mm_subw_menus[i]; m && m->name; m++) {
            for (MenuEnt *ent = m->menu; ent && ent->entry; ent++) {
                if (ent->cmd.caller && ent->type == CMD_PROMPT)
                    SetSensitive(ent->cmd.caller, sens);
            }
        }
    }
}


void
MenuMain::RebuildDynamicMenus()
{
    for (MenuList *l = mm_menus; l; l = l->next) {
        if (l->menubox)
            l->menubox->rebuildMenu();
    }
}


// Make a copy of the static subwindow menu to apply to a new
// subwindow, and create the new subwindow menu.  The return is the
// graphics menu object.
//
GRobject
MenuMain::CreateSubwinMenu(int wnum)
{
    if (wnum < 1 || wnum >= DSP_NUMWINS)
        return (0);
    if (!DSP()->Window(wnum))
        return (0);

    // Now copy into a new menu.
    //
    int n = 0;
    for (MenuList *l = mm_subw_template; l; l = l->next)
        n++;
    n++;
    MenuBox *nbox = new MenuBox[n];
    n = 0;
    for (MenuList *l = mm_subw_template; l; l = l->next) {
        l->menubox->copy_to(wnum, nbox + n);
        if (nbox[n].menu)
            n++;
    }
    mm_subw_menus[wnum] = nbox;

    MenuBox *mb = XM()->createObjSubMenu();
    nbox = new MenuBox[1];
    mb->copy_to(wnum, nbox);
    mm_subw_obj_menus[wnum] = nbox;

    // Call the graphics to create a new menu object.  This must be done
    // before initializing the states.
    //
    GRobject menu = NewSubwMenu(wnum);

    InitAfterModeSwitch(wnum);
    return (menu);
}


// Destroy the subwindow menu.
//
void
MenuMain::DestroySubwinMenu(int wnum)
{
    if (wnum < 1 || wnum >= DSP_NUMWINS)
        return;
    if (!mm_subw_menus[wnum])
        return;
    for (MenuBox *m = mm_subw_menus[wnum]; m->name; m++)
        delete [] m->menu;
    delete [] mm_subw_menus[wnum];
    mm_subw_menus[wnum] = 0;

    delete [] mm_subw_obj_menus[wnum]->menu;
    delete [] mm_subw_obj_menus[wnum];
    mm_subw_obj_menus[wnum] = 0;
}


// Return the status (1 or 0) of the named button in the named menu,
// or -1 if the button is not found.  The button need not be mapped.
//
int
MenuMain::MenuButtonStatus(const char *menuname, const char *button)
{
    MenuEnt *ent = FindEntry(menuname, button);
    if (!ent || !ent->cmd.caller)
        return (-1);
    return (GetStatus(ent->cmd.caller));
}


// Change the state and call the callbacks of the named button.  Return
// true if operation was performed.  The button need not be mapped.
//
bool
MenuMain::MenuButtonPress(const char *menuname, const char *button)
{
    MenuEnt *ent = FindEntry(menuname, button);
    if (!ent || !ent->cmd.caller)
        return (false);
    CallCallback(ent->cmd.caller);
    return (true);
}


// Set the state of the named button.  The button need not be mapped.
// This does not call the callbacks.
//
bool
MenuMain::MenuButtonSet(const char *menuname, const char *button, bool state)
{
    MenuEnt *ent = FindEntry(menuname, button);
    if (!ent || !ent->cmd.caller)
        return (false);
    SetStatus(ent->cmd.caller, state);
    return (true);
}


int
MenuMain::FindMainMenuPos(const char *pref)
{
    if (!pref)
        return (-1);
    int cnt = 0;
    for (MenuList *l = mm_menus; l; l = l->next) {
        if (l->menubox && l->menubox->name &&
                lstring::ciprefix(pref, l->menubox->name))
            return (cnt);
        cnt++;
    }
    return (-1);
}


MenuBox *
MenuMain::FindMainMenu(const char *pref)
{
    if (!pref)
        return (0);
    for (MenuList *l = mm_menus; l; l = l->next) {
        if (l->menubox && l->menubox->name &&
                lstring::ciprefix(pref, l->menubox->name))
            return (l->menubox);
    }
    return (0);
}


bool
MenuMain::IsMainMenu(MenuEnt *ent)
{
    if (!ent)
        return (false);
    for (MenuList *l = mm_menus; l; l = l->next) {
        if (l->menubox && l->menubox->menu == ent)
            return (true);
    }
    if (mm_attr_submenu && mm_attr_submenu->menu == ent)
        return (true);
    if (mm_obj_submenu && mm_obj_submenu->menu == ent)
        return (true);
    return (false);
}


MenuBox *
MenuMain::FindSubwMenu(const char *pref, int wnum)
{
    if (wnum < 1 || wnum >= DSP_NUMWINS)
        return (0);
    MenuBox *mbox = mm_subw_menus[wnum];
    if (!mbox)
        return (0);
    if (!pref)
        return (mbox);
    for ( ; mbox->name; mbox++) {
        if (lstring::ciprefix(pref, mbox->name))
            return (mbox);
    }
    return (0);
}


MenuBox *
MenuMain::FindSubwMenu(const char *pref)
{
    if (!pref)
        return (0);
    for (MenuList *l = mm_subw_template; l; l = l->next) {
        if (l->menubox && l->menubox->name &&
                lstring::ciprefix(pref, l->menubox->name))
            return (l->menubox);
    }
    return (0);
}


MenuBox *
MenuMain::GetSubwObjMenu(int wnum)
{
    if (wnum < 1 || wnum >= DSP_NUMWINS)
        return (0);
    return (mm_subw_obj_menus[wnum]);
}


bool
MenuMain::IsSubwMenu(MenuEnt *ent)
{
    if (!ent)
        return (0);
    for (int i = 1; i < DSP_NUMWINS; i++) {
        MenuBox *m = mm_subw_menus[i];
        if (m) {
            for ( ; m->name; m++) {
                if (m->menu == ent)
                    return (true);
            }
        }
        m = mm_subw_obj_menus[i];
        if (m && m->menu == ent)
            return (true);
    }
    return (false);
}


// This function processes keyboard input as accelerators for command
// buttons.  If the nchars of item uniquely prefix a command button
// name, the menu structure corresponding to the button is returned.
// The button must be enabled.
//
MenuEnt *
MenuMain::MatchEntry(const char *item, int nchars, int wnum, bool exact)
{
    if (!item)
        return (0);
    if (nchars < 2)
        return (0);
    if (nchars > 15)
        nchars = 15;

    char ibuf[16];
    memcpy(ibuf, item, nchars);
    ibuf[nchars] = 0;
    item = ibuf;

    int cnt = 0;
    MenuEnt *last = 0;
    if (wnum > 0 && wnum < DSP_NUMWINS) {
        for (MenuBox *m = mm_subw_menus[wnum]; m && m->name; m++) {
            for (MenuEnt *ent = m->menu; ent && ent->entry; ent++) {
                if (exact && !strcmp(item, ent->entry) &&
                        IsSensitive(ent->cmd.caller))
                    return (ent);
                if (lstring::ciprefix(item, ent->entry) &&
                        IsSensitive(ent->cmd.caller)) {
                    if (cnt++ > 1)
                        return (0);
                    last = ent;
                }
            }
        }
        MenuBox *m = mm_subw_obj_menus[wnum];
        if (m && m->menu) {
            for (MenuEnt *ent = m->menu; ent && ent->entry; ent++) {
                if (exact && !strcmp(item, ent->entry) &&
                        IsSensitive(ent->cmd.caller))
                    return (ent);
                if (lstring::ciprefix(item, ent->entry) &&
                        IsSensitive(ent->cmd.caller)) {
                    if (cnt++ > 1)
                        return (0);
                    last = ent;
                }
            }
        }
        if (cnt == 1)
            return (last);
    }
    if (GetButtonMenu() && GetButtonMenu()->menu) {
        for (MenuEnt *ent = GetButtonMenu()->menu; ent && ent->entry; ent++) {
            if (exact && !strcmp(item, ent->entry) &&
                    IsSensitive(ent->cmd.caller))
                return (ent);
            if (lstring::ciprefix(item, ent->entry) &&
                    IsSensitive(ent->cmd.caller)) {
                if (cnt && !strcmp(ent->entry, last->entry))
                    continue;
                if (cnt++ > 1)
                    return (0);
                last = ent;
            }
        }
    }
    for (MenuList *l = mm_menus; l; l = l->next) {
        if (!l->menubox || !l->menubox->menu)
            continue;
        for (MenuEnt *ent = l->menubox->menu; ent && ent->entry; ent++) {
            if (ent == l->menubox->menu) {
                // First elt is a dummy containing the menubar item.
                if (!ent->cmd.caller)
                    Log()->ErrorLog(mh::Internal,
                        "MatchEntry: foo! no caller!\n");
                else if (!IsSensitive(ent->cmd.caller))
                    break;
                continue;
            }
            if (exact && !strcmp(item, ent->entry) &&
                    IsSensitive(ent->cmd.caller))
                return (ent);
            if (lstring::ciprefix(item, ent->entry) &&
                    IsSensitive(ent->cmd.caller)) {
                if (cnt && !strcmp(ent->entry, last->entry))
                    continue;
                if (cnt++ > 1)
                    return (0);
                last = ent;
            }
        }
    }
    if (mm_misc_menu && mm_misc_menu->menu) {
        for (MenuEnt *ent = mm_misc_menu->menu; ent->entry; ent++) {
            if (exact && !strcmp(item, ent->entry) &&
                    IsSensitive(ent->cmd.caller))
                return (ent);
            if (lstring::ciprefix(item, ent->entry) &&
                    IsSensitive(ent->cmd.caller)) {
                if (cnt && !strcmp(ent->entry, last->entry))
                    continue;
                if (cnt++ > 1)
                    return (0);
                last = ent;
            }
        }
    }
    if (mm_attr_submenu && mm_attr_submenu->menu) {
        for (MenuEnt *ent = mm_attr_submenu->menu; ent->entry; ent++) {
            if (exact && !strcmp(item, ent->entry) &&
                    IsSensitive(ent->cmd.caller))
                return (ent);
            if (lstring::ciprefix(item, ent->entry) &&
                    IsSensitive(ent->cmd.caller)) {
                if (cnt && !strcmp(ent->entry, last->entry))
                    continue;
                if (cnt++ > 1)
                    return (0);
                last = ent;
            }
        }
    }
    if (mm_obj_submenu && mm_obj_submenu->menu) {
        MenuEnt *ent = mm_obj_submenu->menu;
        if (exact && !strcmp(item, ent->entry) &&
                IsSensitive(ent->cmd.caller))
            return (ent);
        if (lstring::ciprefix(item, ent->entry) &&
                IsSensitive(ent->cmd.caller)) {
            if (cnt && !strcmp(ent->entry, last->entry))
                ;
            else {
                if (cnt++ > 1)
                    return (0);
                last = ent;
            }
        }
    }
    if (cnt == 1)
        return (last);
    return (0);
}


// Menu names.
//
#define MNmain      "main"
#define MNside      "side"
#define MNtop       "top"

#define MNfile      "file"
#define MNcell      "cell"
#define MNedit      "edit"
#define MNmod       "mod"
#define MNview      "view"
#define MNattri     "attri"
#define MNcnvrt     "cnvrt"
#define MNdrc       "drc"
#define MNextrc     "extrc"
#define MNuser      "user"
#define MNhelp      "help"
 
#define MNsub1      "sub1"
#define MNsub2      "sub2"
#define MNsub3      "sub3"
#define MNsub4      "sub4"

// Look through the indicated menu for button, and return the MenuEnt
// object if found, also return its MenuBox if mbox is not null.  If
// menuname is 0 or empty, search all the main window menus.
//
// menuname can take one of the following:
// (null)     same as MNmain
// MNmain     search all top, button, and misc menus
// MNside     search the side menu only
// MNtop      search the top entries only
//
// main window top menu:
// MNfile     search the file menu only
// MNcell     search the cell menu only
// MNedit     search the edit menu only
// MNmod      search the modify menu only
// MNview     search the view menu only
// MNattri    search the attributes menu (and sub-menu) only
// MNcnvrt    search the convert menu only
// MNdrc      search the drc menu only
// MNextrc    search the extract menu only
// MNuser     search the user menu only
// MNhelp     search the help menu only
//
// subwindows:
// MNsub1     search all menus in subwin 1
// MNsub2     search all menus in subwin 2
// MNsub3     search all menus in subwin 3
// MNsub4     sea3ch all menus in subwin 4
//
MenuEnt*
MenuMain::FindEntry(const char *menuname, const char *button, MenuBox **mbox)
{
    if (!button)
        return (0);
    if (mbox)
        *mbox = 0;

    char buf1[16];
    if (menuname) {
        char *s = buf1;
        while (*menuname && !isspace(*menuname) && (s - buf1) < 15)
            *s++ = *menuname++;
        *s = '\0';
        menuname = buf1;
    }

    char buf2[16];
    char *s = buf2;
    while (*button && !isspace(*button) && (s - buf2) < 15)
        *s++ = *button++;
    *s = '\0';
    button = buf2;

    if (menuname && *menuname) {
        if (lstring::cieq(menuname, MNmain))
            return (FindEntOfWin(DSP()->MainWdesc(), button, mbox));
        if (lstring::cieq(menuname, MNside)) {
            if (GetButtonMenu() && GetButtonMenu()->menu) {
                for (MenuEnt *ent = GetButtonMenu()->menu; ent->entry; ent++) {
                    if (lstring::ciprefix(button, ent->entry)) {
                        if (mbox)
                            *mbox = GetButtonMenu();
                        return (ent);
                    }
                }
            }
        }
        else if (lstring::cieq(menuname, MNtop) ||
                lstring::cieq(menuname, "misc")) {
            if (mm_misc_menu && mm_misc_menu->menu) {
                for (MenuEnt *ent = mm_misc_menu->menu; ent->entry; ent++) {
                    if (lstring::ciprefix(button, ent->entry))
                        return (ent);
                }
            }
        }
        else if (lstring::cieq(menuname, MNsub1))
            return (FindEntOfWin(DSP()->Window(1), button, mbox));
        else if (lstring::cieq(menuname, MNsub2))
            return (FindEntOfWin(DSP()->Window(2), button, mbox));
        else if (lstring::cieq(menuname, MNsub3))
            return (FindEntOfWin(DSP()->Window(3), button, mbox));
        else if (lstring::cieq(menuname, MNsub4))
            return (FindEntOfWin(DSP()->Window(4), button, mbox));
        else {
            char mbuf[16];
            memset(mbuf, 0, 16);

            MenuBox *m = 0;
            for (int k = 0; k < 5; k++) {
                mbuf[k] = menuname[k];
                if (!mbuf[k])
                    break;
                int cnt = 0;
                MenuBox *mlast = 0;
                for (MenuList *l = mm_menus; l; l = l->next) {
                    if (!l->menubox)
                        continue;
                    if (lstring::ciprefix(mbuf, l->menubox->name)) {
                        mlast = l->menubox;
                        cnt++;
                    }
                }
                if (cnt == 1) {
                    m = mlast;
                    break;
                }
            }
            if (m && m->menu) {
                for (MenuEnt *ent = m->menu; ent->entry; ent++) {
                    if (lstring::ciprefix(button, ent->entry)) {
                        if (mbox)
                            *mbox = m;
                        return (ent);
                    }
                }
                if (lstring::ciprefix("attr", m->name) && mm_attr_submenu &&
                        mm_attr_submenu->menu) {
                    for (MenuEnt *ent = mm_attr_submenu->menu; ent->entry;
                            ent++) {
                        if (lstring::ciprefix(button, ent->entry)) {
                            if (mbox)
                                *mbox = mm_attr_submenu;
                            return (ent);
                        }
                    }
                    for (MenuEnt *ent = mm_obj_submenu->menu; ent->entry;
                            ent++) {
                        if (lstring::ciprefix(button, ent->entry)) {
                            if (mbox)
                                *mbox = mm_obj_submenu;
                            return (ent);
                        }
                    }
                }
            }
        }
        return (0);
    }
    return (FindEntOfWin(DSP()->MainWdesc(), button, mbox));
}


// If wdesc is the main window, search the main, button, and misc
// entries for button.  If wdesc is a subwindow, search the subwindow
// menus.  If found, return the entry and set mbox to the containing
// MenuBox if mbox is not null.
//
MenuEnt *
MenuMain::FindEntOfWin(WindowDesc *wdesc, const char *button, MenuBox **mbox)
{
    if (!button || !wdesc)
        return (0);
    if (mbox)
        *mbox = 0;
    if (wdesc == DSP()->MainWdesc()) {
        if (GetButtonMenu() && GetButtonMenu()->menu) {
            for (MenuEnt *ent = GetButtonMenu()->menu; ent->entry; ent++) {
                if (!strcmp(ent->entry, button)) {
                    if (mbox)
                        *mbox = GetButtonMenu();
                    return (ent);
                }
            }
        }
        for (MenuList *l = mm_menus; l; l = l->next) {
            if (!l->menubox || !l->menubox->menu)
                continue;
            for (MenuEnt *ent = l->menubox->menu; ent->entry; ent++) {
                if (!strcmp(ent->entry, button)) {
                    if (mbox)
                        *mbox = l->menubox;
                    return (ent);
                }
            }
        }
        if (mm_misc_menu && mm_misc_menu->menu) {
            for (MenuEnt *ent = mm_misc_menu->menu; ent->entry; ent++) {
                if (lstring::ciprefix(button, ent->entry))
                    return (ent);
            }
        }
        if (mm_attr_submenu && mm_attr_submenu->menu) {
            for (MenuEnt *ent = mm_attr_submenu->menu; ent->entry; ent++) {
                if (lstring::ciprefix(button, ent->entry))
                    return (ent);
            }
        }
        if (mm_obj_submenu && mm_obj_submenu->menu) {
            for (MenuEnt *ent = mm_obj_submenu->menu; ent->entry; ent++) {
                if (lstring::ciprefix(button, ent->entry))
                    return (ent);
            }
        }
    }
    else {
        int i = wdesc->WinNumber();
        if (i > 0) {
            MenuBox *m = mm_subw_menus[i];
            if (m) {
                for ( ; m && m->name; m++) {
                    if (!m->menu)
                        continue;
                    for (MenuEnt *ent = m->menu; ent->entry; ent++) {
                        if (!strcmp(ent->entry, button)) {
                            if (mbox)
                                *mbox = m;
                            return (ent);
                        }
                    }
                }
            }
            m = mm_subw_obj_menus[i];
            if (m && m->menu) {
                for (MenuEnt *ent = m->menu; ent->entry; ent++) {
                    if (!strcmp(ent->entry, button)) {
                        if (mbox)
                            *mbox = m;
                        return (ent);
                    }
                }
            }
        }
    }
    return (0);
}


// Find the menu entry corresponding to obj.  If menuname is given, confine
// search to that menu, otherwise search all menus.
//
MenuEnt *
MenuMain::FindEntByObj(const char *menuname, GRobject obj)
{
    if (!obj)
        return (0);

    char buf1[16];
    if (menuname) {
        char *s = buf1;
        while (*menuname && !isspace(*menuname) && (s - buf1) < 15)
            *s++ = *menuname++;
        *s = '\0';
        menuname = buf1;
    }

    if (!menuname || !*menuname || lstring::cieq(menuname, MNmain)) {
        if (GetButtonMenu() && GetButtonMenu()->menu) {
            for (MenuEnt *ent = GetButtonMenu()->menu; ent->entry; ent++) {
                if (ent->cmd.caller == obj)
                    return (ent);
            }
        }
        for (MenuList *l = mm_menus; l; l = l->next) {
            if (!l->menubox || !l->menubox->menu)
                continue;
            for (MenuEnt *ent = l->menubox->menu; ent->entry; ent++) {
                if (ent->cmd.caller == obj)
                    return (ent);
            }
        }
        if (mm_misc_menu && mm_misc_menu->menu) {
            for (MenuEnt *ent = mm_misc_menu->menu; ent->entry; ent++) {
                if (ent->cmd.caller == obj)
                    return (ent);
            }
        }
        if (mm_attr_submenu && mm_attr_submenu->menu) {
            for (MenuEnt *ent = mm_attr_submenu->menu; ent->entry; ent++) {
                if (ent->cmd.caller == obj)
                    return (ent);
            }
        }
        if (mm_obj_submenu && mm_obj_submenu->menu) {
            for (MenuEnt *ent = mm_obj_submenu->menu; ent->entry; ent++) {
                if (ent->cmd.caller == obj)
                    return (ent);
            }
        }
        return (0);
    }

    int subw_num = 0;
    if (lstring::cieq(menuname, MNside)) {
        if (GetButtonMenu() && GetButtonMenu()->menu) {
            for (MenuEnt *ent = GetButtonMenu()->menu; ent->entry; ent++) {
                if (ent->cmd.caller == obj)
                    return (ent);
            }
        }
    }
    else if (lstring::cieq(menuname, MNtop) ||
            lstring::cieq(menuname, "misc")) {
        if (mm_misc_menu && mm_misc_menu->menu) {
            for (MenuEnt *ent = mm_misc_menu->menu; ent->entry; ent++) {
                if (ent->cmd.caller == obj)
                    return (ent);
            }
        }
    }
    else if (lstring::cieq(menuname, MNsub1))
        subw_num = 1;
    else if (lstring::cieq(menuname, MNsub2))
        subw_num = 2;
    else if (lstring::cieq(menuname, MNsub3))
        subw_num = 3;
    else if (lstring::cieq(menuname, MNsub4))
        subw_num = 4;
    else {
        char mbuf[16];
        memset(mbuf, 0, 16);

        MenuBox *m = 0;
        for (int k = 0; k < 5; k++) {
            mbuf[k] = menuname[k];
            if (!mbuf[k])
                break;
            int cnt = 0;
            MenuBox *mlast = 0;
            for (MenuList *l = mm_menus; l; l = l->next) {
                if (!l->menubox || !l->menubox->menu)
                    continue;
                if (lstring::ciprefix(mbuf, l->menubox->name)) {
                    mlast = l->menubox;
                    cnt++;
                }
            }
            if (cnt == 1) {
                m = mlast;
                break;
            }
        }
        if (m && m->menu) {
            for (MenuEnt *ent = m->menu; ent->entry; ent++) {
                if (ent->cmd.caller == obj)
                    return (ent);
            }
            if (lstring::ciprefix("attr", m->name) && mm_attr_submenu &&
                    mm_attr_submenu->menu) {
                for (MenuEnt *ent = mm_attr_submenu->menu; ent->entry; ent++) {
                    if (ent->cmd.caller == obj)
                        return (ent);
                }
                for (MenuEnt *ent = mm_obj_submenu->menu; ent->entry; ent++) {
                    if (ent->cmd.caller == obj)
                        return (ent);
                }
            }
        }
    }

    if (subw_num > 0 && DSP()->Window(subw_num)) {
        if (mm_subw_menus[subw_num]) {
            for (MenuBox *m = mm_subw_menus[subw_num]; m->name; m++) {
                if (!m->menu)
                    continue;
                for (MenuEnt *ent = m->menu; ent->entry; ent++) {
                    if (ent->cmd.caller == obj)
                        return (ent);
                }
            }
        }
        if (mm_subw_obj_menus[subw_num]) {
            MenuBox *m = mm_subw_obj_menus[subw_num];
            if (m->menu) {
                for (MenuEnt *ent = m->menu; ent->entry; ent++) {
                    if (ent->cmd.caller == obj)
                        return (ent);
                }
            }
        }
    }
    return (0);
}


MenuBox *
MenuMain::GetButtonMenu()
{
    return (DSP()->CurMode() == Physical ?
        mm_phys_button_menu : mm_elec_button_menu);
}


void
M_NoOp(CmdDesc*) { }



/*========================================================================*
 *                                                                        *
 *  Whiteley Research Inc, Sunnyvale, CA USA, http://wrcad.com            *
 *  Copyright (C) 2017 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *  Licensed under the Apache License, Version 2.0 (the "License");       *
 *  you may not use this file except in compliance with the License.      *
 *  You may obtain a copy of the License at                               *
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

#ifndef MENU_H
#define MENU_H

// Command class
//
// CMD_NOTFOUND          Error indication
// CMD_UNDO              Undo command
// CMD_REDO              Redo command
// CMD_SAFE              Commands that can execute at any time, within
//                        any other command.  These are simple commands
//                        that don't use the prompt line for text input.
// CMD_PROMPT            Commands that use the prompt line for text
//                        input, but are otherwise "safe".
// CMD_NOTSAFE           Commands that will terminate other commands
//                        in progress before executing.  These are
//                        generally complex commands that can modify
//                        the database.
enum CmdType
{
    CMD_NOTFND,
    CMD_UNDO,
    CMD_REDO,
    CMD_SAFE,
    CMD_PROMPT,
    CMD_NOTSAFE
};

// Flags for MenuEnt
//
enum
{
    ME_VANILLA  = 0x0,
    ME_TOGGLE   = 0x1,
    ME_SET      = 0x2,
    ME_MENU     = 0x4,
    ME_SEP      = 0x8,
    ME_DYNAMIC  = 0x10,
    ME_ALT      = 0x20
};
//  ME_VANILLA
//    Nothing special.
//  ME_TOGGLE
//    The menu button should be bi-state.
//  ME_SET
//    If ME_TOGGLE, initial state is "on".
//  ME_MENU
//    This item brings forth a sub-menu.
//  ME_SEP
//    A separator should follow this item in the menu.
//  ME_DYNAMIC
//    This item is a dynamic element.
//  ME_ALT
//    This item is an alternate for the previous item.

struct WindowDesc;

// Structure passed to menu commands.
//
struct CmdDesc
{
    CmdDesc() { caller = 0; wdesc = 0; }

    void *caller;           // calling button (Widget)
    WindowDesc *wdesc;      // calling window
};

typedef void(MenuFunc)(CmdDesc*);

// Structure for a menu entry.
//
struct MenuEnt
{
    MenuEnt()
        {
            entry = 0;
            description = 0;
            action = 0;
            type = CMD_SAFE;
            flags = ME_VANILLA;
            xpm = 0;
            menutext = 0;
            accel = 0;
            item = 0;
            alt_caller = 0;
            user_action = 0;
            id = 0;
        }

    MenuEnt(MenuFunc *cb, const char *ent, unsigned int flg, CmdType cmdtype,
        const char *desc, const char **x = 0)
        {
            entry = ent;
            description = desc;
            action = cb;
            type = cmdtype;
            flags = flg;
            xpm = x;
            menutext = 0;
            accel = 0;
            item = 0;
            alt_caller = 0;
            user_action = 0;
            id = 0;
        }

    bool is_toggle()    { return (flags & ME_TOGGLE); }
    bool is_set()       { return (flags & ME_SET); }
    bool is_menu()      { return (flags & ME_MENU); }
    bool is_dynamic()   { return (flags & ME_DYNAMIC); }
    bool is_separator() { return (flags & ME_SEP); }
    bool is_alt()       { return (flags & ME_ALT); }

    void set_state(bool b)
        {
            if (b)
                flags |= ME_SET;
            else
                flags &= ~ME_SET;
        }

    void set_menu(bool b)
        {
            if (b)
                flags |= ME_MENU;
            else
                flags &= ~ME_MENU;
        }

    void set_dynamic(bool b)
        {
            if (b)
                flags |= ME_DYNAMIC;
            else
                flags &= ~ME_DYNAMIC;
        }

    const char *entry;              // the keyword (*not* UI text)
    const char *description;        // brief description of command, or
                                    //  entry path for user menu items
    CmdDesc cmd;                    // callback info
    MenuFunc *action;               // the callback
    CmdType type;                   // type of command
    unsigned int flags;             // ME_XXX flags

    const char **xpm;               // associated XPM pixmap data

    // The remaining fields are for use by the toolkit-specific code.

    const char *menutext;           // actual menu text
    const char *accel;              // accelerator
    const char *item;               // menu item type
    void *alt_caller;               // alternate cmd.caller for remapping
    void *user_action;              // user defined
    int id;                         // resource id
};

// Container for a menu
struct MenuBox
{
    MenuBox()
        {
            name = 0;
            menu = 0;
            pre_mode_switch_proc = 0;
            post_mode_switch_proc = 0;
            rebuild_menu = 0;
        }

    MenuBox *copy_to(int, MenuBox* = 0);

    // These handle setting states, etc. in mode switch.
    void cleanupBeforeModeSwitch(int wnum)
        { if (pre_mode_switch_proc) (*pre_mode_switch_proc)(wnum, menu); }
    void initAfterModeSwitch(int wnum)
        { if (post_mode_switch_proc) (*post_mode_switch_proc)(wnum, menu); }

    // This handles rebuilding dynamically, only applies to main window
    // menus.
    void rebuildMenu()
        { if (rebuild_menu) (*rebuild_menu)(this); }
    bool isDynamic() { return (rebuild_menu != 0); }

    void registerPreSwitchProc(void(*cb)(int, MenuEnt*))
        { pre_mode_switch_proc = cb; }
    void registerPostSwitchProc(void(*cb)(int, MenuEnt*))
        { post_mode_switch_proc = cb; }
    void registerRebuildMenuProc(void(*cb)(MenuBox*))
        { rebuild_menu = cb; }

    const char *name;
    MenuEnt *menu;

private:
    void(*pre_mode_switch_proc)(int, MenuEnt*);
    void(*post_mode_switch_proc)(int, MenuEnt*);
    void(*rebuild_menu)(MenuBox*);
};

// List element for menus
struct MenuList
{
    MenuList(MenuBox *mb) { next = prev = 0; menubox = mb; }

    MenuList *next;
    MenuList *prev;
    MenuBox *menubox;
};

// If an entry string passed to NewDDmenu has this text, a separator is
// inserted.
#define MENU_SEP_STRING "<separator>"

inline class MenuMain *Menu();

// The main menu class.  This contains information about the
// application's menus.  This or a derived class is intended to be
// exported globally.
//
class MenuMain
{
    static MenuMain *ptr()
        {
            if (!instancePtr)
                on_null_ptr();
            return (instancePtr);
        }

    static void on_null_ptr();

public:
    friend inline MenuMain *Menu() { return (MenuMain::ptr()); }

    MenuMain();
    virtual ~MenuMain() { }

    bool SetupCommand(MenuEnt*, bool*);
    void RegisterMenu(MenuBox*);
    void RegisterSubwMenu(MenuBox*);
    void RegisterButtonMenu(MenuBox*, DisplayMode);
    void RegisterMiscMenu(MenuBox*);
    void CleanupBeforeModeSwitch();
    void InitAfterModeSwitch(int);
    void SetUndoSens(bool);
    void RebuildDynamicMenus();
    GRobject CreateSubwinMenu(int);
    void DestroySubwinMenu(int);

    // Misc utilities

    int MenuButtonStatus(const char*, const char*);
    bool MenuButtonPress(const char*, const char*);
    bool MenuButtonSet(const char*, const char*, bool);

    int FindMainMenuPos(const char*);
    MenuBox *FindMainMenu(const char*);
    bool IsMainMenu(MenuEnt*);
    MenuBox *FindSubwMenu(const char*, int);
    MenuBox *FindSubwMenu(const char*);
    MenuBox *GetSubwObjMenu(int);
    bool IsSubwMenu(MenuEnt*);
    MenuEnt *MatchEntry(const char*, int, int, bool);
    MenuEnt *FindEntry(const char*, const char*, MenuBox** = 0);
    MenuEnt *FindEntOfWin(WindowDesc*, const char*, MenuBox** = 0);
    MenuEnt *FindEntByObj(const char*, GRobject);
    MenuBox *GetButtonMenu();

    bool IsMiscMenu(MenuEnt *ent)
        { return (GetMiscMenu() && ent && GetMiscMenu()->menu == ent); }
    bool IsButtonMenu(MenuEnt *ent)
        { return (GetButtonMenu() && ent && GetButtonMenu()->menu == ent); }
    MenuBox *GetPhysButtonMenu()  { return (mm_phys_button_menu); }
    MenuBox *GetElecButtonMenu()  { return (mm_elec_button_menu); }
    MenuBox *GetMiscMenu()      { return (mm_misc_menu); }
    MenuBox *GetAttrSubMenu()   { return (mm_attr_submenu); }
    MenuBox *GetObjSubMenu()    { return (mm_obj_submenu); }
    const MenuList *GetMainMenus() { return (mm_menus); }

    bool IsGlobalInsensitive()  { return (mm_insensitive); }

    // virtual utilities
    virtual void SetSensGlobal(bool) = 0;
    virtual void Deselect(GRobject) = 0;
    virtual void Select(GRobject) = 0;
    virtual bool GetStatus(GRobject) = 0;
    virtual void SetStatus(GRobject, bool) = 0;
    virtual void CallCallback(GRobject) = 0;
    virtual void Location(GRobject, int*, int*) = 0;
    virtual void PointerRootLoc(int*, int*) = 0;
    virtual const char *GetLabel(GRobject) = 0;
    virtual void SetLabel(GRobject, const char*) = 0;
    virtual void SetSensitive(GRobject, bool) = 0;
    virtual bool IsSensitive(GRobject) = 0;
    virtual void SetVisible(GRobject, bool) = 0;
    virtual bool IsVisible(GRobject) = 0;
    virtual void DestroyButton(GRobject) = 0;
    virtual void SwitchMenu() = 0;
    virtual void SwitchSubwMenu(int, DisplayMode) = 0;
    virtual GRobject NewSubwMenu(int) = 0;
    virtual void SetDDentry(GRobject, int, const char*) = 0;
    virtual void NewDDentry(GRobject, const char*) = 0;
    virtual void NewDDmenu(GRobject, const char*const*) = 0;
    virtual void UpdateUserMenu() = 0;
    virtual void HideButtonMenu(bool) = 0;
    virtual void DisableMainMenuItem(const char*, const char*, bool) = 0;

protected:
    MenuList *mm_menus;
    MenuList *mm_subw_template;
    MenuBox *mm_subw_menus[DSP_NUMWINS];
    MenuBox *mm_subw_obj_menus[DSP_NUMWINS];
    MenuBox *mm_phys_button_menu;
    MenuBox *mm_elec_button_menu;
    MenuBox *mm_misc_menu;
    MenuBox *mm_attr_submenu;
    MenuBox *mm_obj_submenu;
    bool mm_insensitive;  // set when menus are insensitive

private:
    static MenuMain *instancePtr;
};

extern MenuFunc M_NoOp;

#endif


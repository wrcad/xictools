
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
 $Id: gtkmenu.h,v 5.22 2015/03/29 03:31:52 stevew Exp $
 *========================================================================*/

#ifndef GTKMENU_H
#define GTKMENU_H

#include "menu.h"


inline class GTKmenu *gtkMenu();

// MenuMain subclass.
//
class GTKmenu : public MenuMain
{
public:
    friend inline GTKmenu *gtkMenu()
        { return (static_cast<GTKmenu*>(Menu())); }
    friend class gtkMenuConfig;

    GTKmenu();

    void InitMainMenu(GtkWidget*);
    void InitTopButtonMenu();
    void InitSideButtonMenus(bool);

    // virtual functions from MenuClass
    void SetSensGlobal(bool);
    void Deselect(GRobject);
    void Select(GRobject);
    bool GetStatus(GRobject);
    void SetStatus(GRobject, bool);
    void CallCallback(GRobject);
    void Location(GRobject, int*, int*);
    void PointerRootLoc(int*, int*);
    const char *GetLabel(GRobject);
    void SetLabel(GRobject, const char*);
    void SetSensitive(GRobject, bool);
    bool IsSensitive(GRobject);
    void SetVisible(GRobject, bool);
    bool IsVisible(GRobject);
    void DestroyButton(GRobject);
    void SwitchMenu();
    void SwitchSubwMenu(int, DisplayMode);
    GRobject NewSubwMenu(int);
    void SetDDentry(GRobject, int, const char*);
    void NewDDentry(GRobject, const char*);
    void NewDDmenu(GRobject, const char*const*);
    void UpdateUserMenu();
    void HideButtonMenu(bool);
    void DisableMainMenuItem(const char*, const char*, bool);

    void SetModal(GtkWidget *w) { modalShell = w; }
    GtkWidget *GetModal()       { return (modalShell); }

    GtkWidget *MainMenu()       { return (mainMenu); }
    GtkWidget *TopButtonMenu()  { return (topButtonWidget); }
    GtkWidget *ButtonMenu(DisplayMode m)
        {
            return (m == Physical ? btnPhysMenuWidget : btnElecMenuWidget);
        }

private:
    GtkWidget *name_to_widget(const char*);

    static char *strip_accel(const char*);
    static GtkWidget *new_popup_menu(GtkWidget*, const char* const*,
        GtkSignalFunc, void*);
    static int button_press(GtkWidget*, GdkEvent*);
    static void gtk_vbox_size_allocate(GtkWidget*, GtkAllocation*);

    GtkWidget *mainMenu;
    GtkWidget *topButtonWidget;
    GtkWidget *btnPhysMenuWidget;
    GtkWidget *btnElecMenuWidget;

    GtkItemFactory *itemFactory;
    GtkWidget *modalShell;
};

#endif


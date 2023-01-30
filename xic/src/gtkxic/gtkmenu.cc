
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
#include "dsp_inlines.h"
#include "gtkmain.h"
#include "gtkmenu.h"
#include "gtkmenucfg.h"


// Instantiate the menus.
namespace { GTKmenu _gtk_menu_; }


GTKmenu::GTKmenu()
{
    mainMenu = 0;
    topButtonWidget = 0;
    btnPhysMenuWidget = 0;
    btnElecMenuWidget = 0;
#ifdef UseItemFactory
    itemFactory = 0;
#else
    accelGroup = 0;
#endif
    modalShell = 0;
}


// Create the menu bar and entries.
//
// data set:
// main item factory name:  "<main>"
// btn              "menu"              menu
// btn              "callb"             edmenu_proc
// btn              "menu"              menu
// btn              "callb"             vimenu_proc
//
//
void
GTKmenu::InitMainMenu(GtkWidget *window)
{
#ifdef UseItemFactory
    GtkAccelGroup *accel_group = gtk_accel_group_new();

    // This function initializes the item factory.
    //  Param 1: The type of menu - can be GTK_TYPE_MENU_BAR, GTK_TYPE_MENU,
    //           or GTK_TYPE_OPTION_MENU.
    //  Param 2: The path of the menu.
    //  Param 3: A pointer to a gtk_accel_group.  The item factory sets up
    //           the accelerator table while generating menus.

    GtkItemFactory *item_factory = gtk_item_factory_new(GTK_TYPE_MENU_BAR,
        "<main>", accel_group);
    itemFactory = item_factory;

    gtkCfg()->instantiateMainMenus();

    // Attach the new accelerator group to the top level window.
    gtk_window_add_accel_group(GTK_WINDOW(window), accel_group);

    mainMenu = gtk_item_factory_get_widget(item_factory, "<main>");
    gtk_widget_show(mainMenu);
#else
    accelGroup = gtk_accel_group_new();
    gtk_window_add_accel_group(GTK_WINDOW(window), accelGroup);
    mainMenu = gtk_menu_bar_new();
    gtk_widget_show(mainMenu);
    gtkCfg()->instantiateMainMenus();
#endif
}


// Create the top button menu.
//
void
GTKmenu::InitTopButtonMenu()
{
    gtkCfg()->instantiateTopButtonMenu();

    // Top horizontal buttons.
    MenuBox *btn_menu = GetMiscMenu();
    if (btn_menu && btn_menu->menu) {
        GtkWidget *hbox = gtk_hbox_new(false, 0);
        gtk_widget_show(hbox);
        // The first entry is the WR button, which is not packed here.
        for (int i = 2; btn_menu->menu[i].entry; i++) {
            MenuEnt *ent = &btn_menu->menu[i];
            GtkWidget *button = static_cast<GtkWidget*>(ent->cmd.caller);
            if (!button)
                continue;
            gtk_box_pack_start(GTK_BOX(hbox), button, false, false, 0);
        }
        topButtonWidget = hbox;
    }
}


// Create the Physical and Electrical side button menus.  If the
// argument is true, this will be arrayed horizontally across the top,
// not to be confused with the "top button menu".
//
// data set:
// pvbox            "stitem"            stitem
//
void
GTKmenu::InitSideButtonMenus(bool horiz_buttons)
{
    gtkCfg()->instantiateSideButtonMenus();

    // Physical vertical buttons
    MenuBox *pbtn_menu = GetPhysButtonMenu();
    if (pbtn_menu && pbtn_menu->menu) {
        GtkWidget *pbox;
        if (horiz_buttons)
            pbox = gtk_hbox_new(true, 0);
        else {
            pbox = gtk_vbox_new(true, 0);

            // Override the class size allocation method with a local version
            // that does a better job keeping buttons the same size.
            GtkWidgetClass *ks = GTK_WIDGET_GET_CLASS(pbox);
            if (ks)
                ks->size_allocate = gtk_vbox_size_allocate;
            gtk_widget_set_size_request(pbox, 28, -1);
        }

        for (int i = 1; pbtn_menu->menu[i].entry; i++) {
            MenuEnt *ent = &pbtn_menu->menu[i];
            GtkWidget *button = static_cast<GtkWidget*>(ent->cmd.caller);
            if (!button)
                continue;
            gtk_box_pack_start(GTK_BOX(pbox), button, true, true, 0);
        }
        btnPhysMenuWidget = pbox;
    }

    // Electrical vertical buttons
    MenuBox *ebtn_menu = GetElecButtonMenu();
    if (ebtn_menu && ebtn_menu->menu) {
        GtkWidget *ebox;
        if (horiz_buttons)
            ebox = gtk_hbox_new(true, 0);
        else {
            ebox = gtk_vbox_new(true, 0);
            gtk_widget_set_size_request(ebox, 28, -1);
        }

        for (int i = 1; ebtn_menu->menu[i].entry; i++) {
            MenuEnt *ent = &ebtn_menu->menu[i];
            GtkWidget *button = static_cast<GtkWidget*>(ent->cmd.caller);
            if (!button)
                continue;
            gtk_box_pack_start(GTK_BOX(ebox), button, true, true, 0);
        }
        btnElecMenuWidget = ebox;
    }
}


//
// Virtual function overrides
//

// Set the sensitivity of all menus.
//
void
GTKmenu::SetSensGlobal(bool sens)
{
    mm_insensitive = !sens;
    gtkCfg()->set_main_global_sens(sens);
    if (sens)
        dspPkgIf()->CheckForInterrupt();
}


// Set the state of obj to unselected.
//
void
GTKmenu::Deselect(GRobject obj)
{
    if (GRX)
        GRX->Deselect(obj);
}


// Set the state of obj to selected.
//
void
GTKmenu::Select(GRobject obj)
{
    if (GRX)
        GRX->Select(obj);
}


// Return the status of obj.
//
bool
GTKmenu::GetStatus(GRobject obj)
{
    if (!GRX)
        return (false);
    return (GRX->GetStatus(obj));
}


// Set the status of obj.
//
void
GTKmenu::SetStatus(GRobject obj, bool state)
{
    if (GRX)
        GRX->SetStatus(obj, state);
}


// Call the button action callback.
//
void
GTKmenu::CallCallback(GRobject obj)
{
    if (GRX)
        GRX->CallCallback(obj);
}


// Return the root window location of the upper right corner of the
// object.
//
void
GTKmenu::Location(GRobject obj, int *x, int *y)
{
    if (GRX)
        GRX->Location(obj, x, y);
    else {
        *x = 0;
        *y = 0;
    }
}


// Return screen coordinates of pointer.
//
void
GTKmenu::PointerRootLoc(int *x, int *y)
{
    if (GRX)
        GRX->PointerRootLoc(x, y);
    else {
        *x = 0;
        *y = 0;
    }
}


// Return the label string of the button.
//
const char *
GTKmenu::GetLabel(GRobject obj)
{
    return (GRX ? GRX->GetLabel(obj) : "");
}


// Set the label of the button.
//
void
GTKmenu::SetLabel(GRobject obj, const char *text)
{
    if (GRX)
        GRX->SetLabel(obj, text);
}


// Set button sensitivity.
//
void
GTKmenu::SetSensitive(GRobject obj, bool sens_state)
{
    if (GRX)
        GRX->SetSensitive(obj, sens_state);
}


// Return button sensitivity.
//
bool
GTKmenu::IsSensitive(GRobject obj)
{
    if (!GRX)
        return (false);
    return (GRX->IsSensitive(obj));
}


// Set button visibility.
//
void
GTKmenu::SetVisible(GRobject obj, bool vis_state)
{
    if (GRX)
        GRX->SetVisible(obj, vis_state);
}


// Return button visibility.
//
bool
GTKmenu::IsVisible(GRobject obj)
{
    if (!GRX)
        return (false);
    return (GRX->IsVisible(obj));
}


void
GTKmenu::DestroyButton(GRobject obj)
{
    if (GRX)
        GRX->DestroyButton(obj);
}


// Switch between the Physical and Electrical menus.
//
void
GTKmenu::SwitchMenu()
{
    gtkCfg()->switch_menu_mode(DSP()->CurMode(), 0);
}


// Set the Electrical/Physical menu entry and window mode in a subwindow.
//
void
GTKmenu::SwitchSubwMenu(int wnum, DisplayMode mode)
{
    if (wnum > 0)
        gtkCfg()->switch_menu_mode(mode, wnum);
}


// Create the menu for a subwindow that is about to be displayed.
//
// data set:
// btn              "menu"              menu
// btn              "callb"             v1menu_proc
// btn              "data"              wnum
//
GRobject
GTKmenu::NewSubwMenu(int wnum)
{
    if (wnum < 1 || wnum >= DSP_NUMWINS)
        return (0);
    win_bag *w = dynamic_cast<win_bag*>(DSP()->Window(wnum)->Wbag());
    if (!w)
        return (0);

#ifdef UseItemFactory
    char mname[8];
    mname[0] = '<';
    mname[1] = 's';
    mname[2] = 'u';
    mname[3] = 'b';
    mname[4] = wnum + '0';
    mname[5] = '>';
    mname[6] = 0;

    GtkAccelGroup *accel_group = gtk_accel_group_new();
    GtkItemFactory *item_factory = gtk_item_factory_new(GTK_TYPE_MENU_BAR,
        mname, accel_group);

    gtkCfg()->instantiateSubwMenus(wnum, item_factory);

    gtk_window_add_accel_group(GTK_WINDOW(w->Shell()), accel_group);
    GtkWidget *menubar = gtk_item_factory_get_widget(item_factory, mname);
    gtk_widget_show(menubar);

    return (menubar);
#else
    GtkAccelGroup *accel_group = gtk_accel_group_new();
    gtk_window_add_accel_group(GTK_WINDOW(w->Shell()), accel_group);
    GtkWidget *menubar = gtk_menu_bar_new();
    gtk_widget_show(menubar);
    gtkCfg()->instantiateSubwMenus(wnum, menubar, accel_group);

    return (menubar);
#endif
}


// Replace the label for the indx'th menu item with newlabel.
//
void
GTKmenu::SetDDentry(GRobject button, int indx, const char *newlabel)
{
    if (!button)
        return;
    GtkWidget *menu = (GtkWidget*)gtk_object_get_data(GTK_OBJECT(button),
        "menu");
    // The button can be either a button in the menu, or the button that
    // pops up the menu.
    if (!menu)
        menu = GTK_WIDGET(button)->parent;
    if (menu && GTK_IS_MENU(menu)) {
        int count = 0;
        GList *btns = gtk_container_children(GTK_CONTAINER(menu));
        for (GList *a = btns; a; a = a->next) {
            GtkWidget *menu_item = (GtkWidget*)a->data;
            if (GTK_IS_MENU_ITEM(menu_item)) {
                GList *contents =
                    gtk_container_children(GTK_CONTAINER(menu_item));
                if (contents) {
                    GtkWidget *label = (GtkWidget*)contents->data;
                    if (GTK_IS_LABEL(label)) {
                        if (count == indx) {
                            gtk_label_set_text(GTK_LABEL(label), newlabel);
                            gtk_widget_set_name(menu_item, newlabel);
                            g_list_free(contents);
                            g_list_free(btns);
                            return;
                        }
                    }
                }
                g_list_free(contents);
            }
            count++;
        }
        g_list_free(btns);
    }
}


// Add a menu entry for label to the end of the list.
//
void
GTKmenu::NewDDentry(GRobject button, const char *label)
{
    if (!button || !label)
        return;
    GtkWidget *menu = (GtkWidget*)gtk_object_get_data(GTK_OBJECT(button),
        "menu");
    // The button can be either a button in the menu, or the button that
    // pops up the menu.
    if (!menu) {
        menu = GTK_WIDGET(button)->parent;
        if (menu && GTK_IS_MENU(menu)) {
            button = gtk_menu_get_attach_widget(GTK_MENU(menu));
            if (!button)
                return;
        }
        else
            return;
    }
    GtkSignalFunc func = (GtkSignalFunc)gtk_object_get_data(GTK_OBJECT(button),
        "callb");
    void *data = gtk_object_get_data(GTK_OBJECT(button), "data");

    GtkWidget *menu_item = gtk_menu_item_new_with_label(label);
    gtk_widget_set_name(menu_item, label);
    gtk_menu_append(GTK_MENU(menu), menu_item);
    g_signal_connect(G_OBJECT(menu_item), "activate",
        G_CALLBACK(func), data);
    gtk_widget_show(menu_item);
}


// Rebuild the menu for button.
//
// data set:
// button           "menu"              menu
//
void
GTKmenu::NewDDmenu(GRobject button, const char *const *list)
{
    if (!button || !list)
        return;
    GtkWidget *menu = (GtkWidget*)gtk_object_get_data(GTK_OBJECT(button),
        "menu");

    // The button can be either a button in the menu, or the button that
    // pops up the menu.
    if (!menu)
        menu = GTK_WIDGET(button)->parent;
    if (menu && GTK_IS_MENU(menu)) {
        int count = 0;
        GList *btns = gtk_container_children(GTK_CONTAINER(menu));
        for (GList *a = btns; a; a = a->next) {
            GtkWidget *menu_item = (GtkWidget*)a->data;
            if (GTK_IS_MENU_ITEM(menu_item)) {
                GList *contents =
                    gtk_container_children(GTK_CONTAINER(menu_item));
                if (contents) {
                    GtkWidget *label = (GtkWidget*)contents->data;
                    if (GTK_IS_LABEL(label)) {
                        if (list[count]) {
                            gtk_label_set_text(GTK_LABEL(label), list[count]);
                            gtk_widget_set_name(menu_item, list[count]);
                        }
                        else {
                            gtk_container_remove(GTK_CONTAINER(menu),
                                GTK_WIDGET(a->data));
                            count--;
                        }
                    }
                }
                g_list_free(contents);
            }
            count++;
        }
        g_list_free(btns);
    }
}


// Update or create the user menu.  This is the only menu that is
// dynamic.
//
void
GTKmenu::UpdateUserMenu()
{
    gtkCfg()->updateDynamicMenus();
}


// Hide or show the button menu.  The menu is desensitized when hidden
// so that the accelerators are invisible.
//
void
GTKmenu::HideButtonMenu(bool hide)
{
    if (hide) {
        if (btnPhysMenuWidget) {
            gtk_widget_set_sensitive(btnPhysMenuWidget->parent, false);
            gtk_widget_hide(btnPhysMenuWidget->parent);
        }
        else if (btnElecMenuWidget) {
            gtk_widget_set_sensitive(btnElecMenuWidget->parent, false);
            gtk_widget_hide(btnElecMenuWidget->parent);
        }
    }
    else {
        if (btnPhysMenuWidget) {
            gtk_widget_set_sensitive(btnPhysMenuWidget->parent, true);
            gtk_widget_show(btnPhysMenuWidget->parent);
        }
        else if (btnElecMenuWidget) {
            gtk_widget_set_sensitive(btnElecMenuWidget->parent, true);
            gtk_widget_show(btnElecMenuWidget->parent);
        }
    }
}


GtkWidget *
GTKmenu::FindMenuWidget(const char *path)
{
    // replacement for gtk_item_factory_get_item
    // XXX write me!
    if (!path || !*path)
        return (0);
    fprintf(stderr, "GTKmenu::FindMenuWidget was called for\n%s\n", path);
    return (0);
}


// Disable/enable menus (item is null) or menu entries.
//
void
GTKmenu::DisableMainMenuItem(const char *mname, const char *item, bool desens)
{
#ifdef UseItemFactory
    if (!itemFactory)
        return;
    MenuBox *mbox = FindMainMenu(mname);
    if (mbox && mbox->menu) {
        if (!item) {
            char *tmp = strip_accel(mbox->menu[0].menutext);
            GtkWidget *widget = gtk_item_factory_get_item(itemFactory, tmp);
            delete [] tmp;
            if (widget)
                gtk_widget_set_sensitive(widget, !desens);
            return;
        }
        MenuEnt *ent = FindEntry(mname, item, 0);
        if (ent) {
            char *tmp = strip_accel(ent->menutext);
            GtkWidget *widget = gtk_item_factory_get_item(itemFactory, tmp);
            delete [] tmp;
            if (widget)
                gtk_widget_set_sensitive(widget, !desens);
        }
    }
#else
    MenuBox *mbox = FindMainMenu(mname);
    if (mbox && mbox->menu) {
        if (!item) {
            char *tmp = strip_accel(mbox->menu[0].menutext);
            GtkWidget *widget = FindMenuWidget(tmp);
            delete [] tmp;
            if (widget)
                gtk_widget_set_sensitive(widget, !desens);
            return;
        }
        MenuEnt *ent = FindEntry(mname, item, 0);
        if (ent) {
            char *tmp = strip_accel(ent->menutext);
            GtkWidget *widget = FindMenuWidget(tmp);
            delete [] tmp;
            if (widget)
                gtk_widget_set_sensitive(widget, !desens);
        }
    }
#endif
}


// Return the activating widget indicated by name, which is either an
// iconfactory path for the main menu, or the button text for the button
// menu, or the button text following "subN" for one of the subwindow
// menus.
//
GtkWidget *
GTKmenu::name_to_widget(const char *name)
{
#ifdef UseItemFactory
    if (!name || !*name)
        return (0);
    if (*name == '/')
        // widget is an iconfactory item whose path is in name
        return (gtk_item_factory_get_item(itemFactory, name));
    if (lstring::prefix("sub", name) && isdigit(name[3])) {
        // widget is in subwindow menu
        int wnum = name[3] - '0';
        if (wnum > 0 && wnum < DSP_NUMWINS &&
                DSP()->Window(wnum)) {
            for (MenuBox *m = mm_subw_menus[wnum]; m && m->name; m++) {
                for (MenuEnt *ent = m->menu; ent && ent->entry; ent++) {
                    if (!strcmp(name + 4, ent->menutext))
                        return ((GtkWidget*)ent->cmd.caller);
                }
            }
        }
    }

    // try the button menu
    if (GetButtonMenu() && GetButtonMenu()->menu) {
        for (MenuEnt *ent = GetButtonMenu()->menu; ent->entry; ent++) {
            if (!strcmp(name, ent->menutext))
                return ((GtkWidget*)ent->cmd.caller);
        }
    }
#else
    if (!name || !*name)
        return (0);
    if (*name == '/')
        // Name is an "icon factory" path.
        return (FindMenuWidget(name));
    if (lstring::prefix("sub", name) && isdigit(name[3])) {
        // widget is in subwindow menu
        int wnum = name[3] - '0';
        if (wnum > 0 && wnum < DSP_NUMWINS &&
                DSP()->Window(wnum)) {
            for (MenuBox *m = mm_subw_menus[wnum]; m && m->name; m++) {
                for (MenuEnt *ent = m->menu; ent && ent->entry; ent++) {
                    if (!strcmp(name + 4, ent->menutext))
                        return ((GtkWidget*)ent->cmd.caller);
                }
            }
        }
    }

    // try the button menu
    if (GetButtonMenu() && GetButtonMenu()->menu) {
        for (MenuEnt *ent = GetButtonMenu()->menu; ent->entry; ent++) {
            if (!strcmp(name, ent->menutext))
                return ((GtkWidget*)ent->cmd.caller);
        }
    }
#endif

    return (0);
}


// Static function.
// Strip underscores (accelerator indicators) out of the path string.
//
char *
GTKmenu::strip_accel(const char *string)
{
    if (!string)
        return (0);
    char *s = new char[strlen(string) + 1];
    char *t = s;
    while (*string) {
        if (*string != '_')
             *t++ = *string;
        string++;
    }
    *t = 0;
    return (s);
}


// Static function.
GtkWidget *
GTKmenu::new_popup_menu(GtkWidget *root, const char *const *list,
    GtkSignalFunc handler, void *arg)
{
    GtkWidget *menu = 0;
    if (list && *list) {
        menu = gtk_menu_new();
        for (const char *const *s = list; *s; s++) {

            if (!strcmp(*s, MENU_SEP_STRING)) {
                // insert a separator
                GtkWidget *msep = gtk_menu_item_new();  // separator
                gtk_menu_append(GTK_MENU(menu), msep);
                gtk_widget_show(msep);
                continue;
            }

            GtkWidget *menu_item = gtk_menu_item_new_with_label(*s);
            gtk_widget_set_name(menu_item, *s);
            gtk_menu_append(GTK_MENU(menu), menu_item);
            g_signal_connect(G_OBJECT(menu_item), "activate",
                handler, arg);
            gtk_widget_show(menu_item);
        }
        if (root) {
            gtk_menu_item_set_submenu(GTK_MENU_ITEM(root), menu);
            g_signal_connect_object(G_OBJECT(root), "event",
                G_CALLBACK(button_press), G_OBJECT(menu), (GConnectFlags)0);
        }
    }
    return (menu);
}


// Static function.
// Respond to a button-press by posting a menu passed in as widget.
//
// Note that the "widget" argument is the menu being posted, NOT
// the button that was pressed.
//
int
GTKmenu::button_press(GtkWidget *widget, GdkEvent *event)
{
    if (event->type == GDK_BUTTON_PRESS) {
        GdkEventButton *bevent = (GdkEventButton*)event;
        gtk_menu_popup(GTK_MENU(widget), 0, 0, 0, 0, bevent->button,
            bevent->time);
        // Tell calling code that we have handled this event; the buck
        // stops here.
        return (true);
    }

    // Tell calling code that we have not handled this event; pass it on.
    return (false);
}


// Static function.
// This was pulled from gtkvbox.c in the gtk 1.2.10 library, and
// modified so that in homogeneous mode, the quantization error is
// distributed as evenly as possible rather than being dumped on the
// last child.  This makes the button menu buttons look lots better.
//
void
GTKmenu::gtk_vbox_size_allocate(GtkWidget *widget, GtkAllocation *allocation)
{
    g_return_if_fail (widget != NULL);
    g_return_if_fail (GTK_IS_VBOX (widget));
    g_return_if_fail (allocation != NULL);

    GtkBox *box = GTK_BOX(widget);
    widget->allocation = *allocation;

    gint nvis_children = 0;
    gint nexpand_children = 0;
    GList *children = box->children;

    GtkBoxChild *child;
    while (children) {
        child = (GtkBoxChild*)children->data;
        children = children->next;

        if (GTK_WIDGET_VISIBLE (child->widget)) {
            nvis_children += 1;
            if (child->expand)
                nexpand_children += 1;
        }
    }

    if (nvis_children > 0) {
        gint height, extra, mod = 0;
        if (box->homogeneous) {
            height = (allocation->height -
                GTK_CONTAINER(box)->border_width * 2 -
                (nvis_children - 1) * box->spacing);
            extra = height / nvis_children;
            mod = height % nvis_children;
        }
        else if (nexpand_children > 0) {
            height = (gint)allocation->height -
                (gint)widget->requisition.height;
            extra = height / nexpand_children;
        }
        else {
            height = 0;
            extra = 0;
        }

        GtkAllocation child_allocation;
        gint y = allocation->y + GTK_CONTAINER(box)->border_width;
        child_allocation.x = allocation->x +
            GTK_CONTAINER(box)->border_width;
        child_allocation.width = MAX(1, (gint)allocation->width -
            (gint)GTK_CONTAINER(box)->border_width * 2);

        gint child_height;

        gint count = 0;
        children = box->children;
        while (children) {
            child = (GtkBoxChild*)children->data;
            children = children->next;

            if ((child->pack == GTK_PACK_START) &&
                    GTK_WIDGET_VISIBLE(child->widget)) {
                if (box->homogeneous) {
                    /*
                    if (nvis_children == 1)
                        child_height = height;
                    else
                    */
                        child_height = extra + (count < mod);

                    nvis_children -= 1;
                    height -= (extra + (count < mod));
                    count++;
                }
                else {
                    GtkRequisition child_requisition;

                    gtk_widget_get_child_requisition(child->widget,
                        &child_requisition);
                    child_height = child_requisition.height +
                        child->padding * 2;

                    if (child->expand) {
                        if (nexpand_children == 1)
                            child_height += height;
                        else
                            child_height += extra;

                        nexpand_children -= 1;
                        height -= extra;
                    }
                }

                if (child->fill) {
                    child_allocation.height = MAX(1, child_height -
                        (gint)child->padding * 2);
                    child_allocation.y = y + child->padding;
                }
                else {
                    GtkRequisition child_requisition;

                    gtk_widget_get_child_requisition(child->widget,
                        &child_requisition);
                    child_allocation.height = child_requisition.height;
                    child_allocation.y = y + (child_height -
                        child_allocation.height) / 2;
                }

                gtk_widget_size_allocate(child->widget, &child_allocation);

                y += child_height + box->spacing;
            }
        }

        y = allocation->y + allocation->height -
            GTK_CONTAINER(box)->border_width;

        children = box->children;
        while (children) {
            child = (GtkBoxChild*)children->data;
            children = children->next;

            if ((child->pack == GTK_PACK_END) &&
                    GTK_WIDGET_VISIBLE(child->widget)) {
                GtkRequisition child_requisition;
                gtk_widget_get_child_requisition(child->widget,
                    &child_requisition);

                if (box->homogeneous) {
                    /*
                    if (nvis_children == 1)
                        child_height = height;
                    else
                    */
                        child_height = extra + (count < mod);

                    nvis_children -= 1;
                    height -= (extra + (count < mod));
                }
                else {
                    child_height = child_requisition.height +
                        child->padding * 2;

                    if (child->expand) {
                        if (nexpand_children == 1)
                            child_height += height;
                        else
                            child_height += extra;

                        nexpand_children -= 1;
                        height -= extra;
                    }
                }

                if (child->fill) {
                    child_allocation.height = MAX(1, child_height -
                        (gint)child->padding * 2);
                    child_allocation.y = y + child->padding - child_height;
                }
                else {
                    child_allocation.height = child_requisition.height;
                    child_allocation.y = y + (child_height -
                        child_allocation.height) / 2 - child_height;
                }

                gtk_widget_size_allocate(child->widget, &child_allocation);

                y -= (child_height + box->spacing);
            }
        }
    }
}
// End of GTKmenu functions.


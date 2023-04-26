
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
    accelGroup = 0;
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
    accelGroup = gtk_accel_group_new();
    gtk_window_add_accel_group(GTK_WINDOW(window), accelGroup);
    mainMenu = gtk_menu_bar_new();
    gtk_widget_show(mainMenu);
    gtkCfg()->instantiateMainMenus();
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

#if GTK_CHECK_VERSION(3,0,0)
            // This appears unneeded on gtk3, plus it generates error
            // messages indicating that a dimension is less than 0.
#else
            // Override the class size allocation method with a local
            // version that does a better job keeping buttons the same
            // size.
            GtkWidgetClass *ks = GTK_WIDGET_GET_CLASS(pbox);
            if (ks)
                ks->size_allocate = gtk_vbox_size_allocate;
            gtk_widget_set_size_request(pbox, 28, -1);
#endif
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
    GTKsubwin *w = dynamic_cast<GTKsubwin*>(DSP()->Window(wnum)->Wbag());
    if (!w)
        return (0);

    GtkAccelGroup *accel_group = gtk_accel_group_new();
    gtk_window_add_accel_group(GTK_WINDOW(w->Shell()), accel_group);
    GtkWidget *menubar = gtk_menu_bar_new();
    gtk_widget_show(menubar);
    gtkCfg()->instantiateSubwMenus(wnum, menubar, accel_group);

    return (menubar);
}


// Replace the label for the indx'th menu item with newlabel.
//
void
GTKmenu::SetDDentry(GRobject button, int indx, const char *newlabel)
{
    if (!button)
        return;
    GtkWidget *menu = (GtkWidget*)g_object_get_data(G_OBJECT(button),
        "menu");
    // The button can be either a button in the menu, or the button that
    // pops up the menu.
    if (!menu)
        menu = gtk_widget_get_parent(GTK_WIDGET(button));
    if (menu && GTK_IS_MENU(menu)) {
        int count = 0;
        GList *btns = gtk_container_get_children(GTK_CONTAINER(menu));
        for (GList *a = btns; a; a = a->next) {
            GtkWidget *menu_item = (GtkWidget*)a->data;
            if (GTK_IS_MENU_ITEM(menu_item)) {
                GList *contents =
                    gtk_container_get_children(GTK_CONTAINER(menu_item));
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
    GtkWidget *menu = (GtkWidget*)g_object_get_data(G_OBJECT(button),
        "menu");
    // The button can be either a button in the menu, or the button that
    // pops up the menu.
    if (!menu) {
        menu = gtk_widget_get_parent(GTK_WIDGET(button));
        if (menu && GTK_IS_MENU(menu)) {
            button = gtk_menu_get_attach_widget(GTK_MENU(menu));
            if (!button)
                return;
        }
        else
            return;
    }
    GCallback func = (GCallback)g_object_get_data(G_OBJECT(button),
        "callb");
    void *data = g_object_get_data(G_OBJECT(button), "data");

    GtkWidget *menu_item = gtk_menu_item_new_with_label(label);
    gtk_widget_set_name(menu_item, label);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
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
    GtkWidget *menu = (GtkWidget*)g_object_get_data(G_OBJECT(button),
        "menu");

    // The button can be either a button in the menu, or the button that
    // pops up the menu.
    if (!menu)
        menu = gtk_widget_get_parent(GTK_WIDGET(button));
    if (menu && GTK_IS_MENU(menu)) {
        int count = 0;
        GList *btns = gtk_container_get_children(GTK_CONTAINER(menu));
        for (GList *a = btns; a; a = a->next) {
            GtkWidget *menu_item = (GtkWidget*)a->data;
            if (GTK_IS_MENU_ITEM(menu_item)) {
                GList *contents =
                    gtk_container_get_children(GTK_CONTAINER(menu_item));
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
            gtk_widget_set_sensitive(gtk_widget_get_parent(btnPhysMenuWidget),
                false);
            gtk_widget_hide(gtk_widget_get_parent(btnPhysMenuWidget));
        }
        else if (btnElecMenuWidget) {
            gtk_widget_set_sensitive(gtk_widget_get_parent(btnElecMenuWidget),
                false);
            gtk_widget_hide(gtk_widget_get_parent(btnElecMenuWidget));
        }
    }
    else {
        if (btnPhysMenuWidget) {
            gtk_widget_set_sensitive(gtk_widget_get_parent(btnPhysMenuWidget),
                true);
            gtk_widget_show(gtk_widget_get_parent(btnPhysMenuWidget));
        }
        else if (btnElecMenuWidget) {
            gtk_widget_set_sensitive(gtk_widget_get_parent(btnElecMenuWidget),
                true);
            gtk_widget_show(gtk_widget_get_parent(btnElecMenuWidget));
        }
    }
}


GtkWidget *
GTKmenu::FindMainMenuWidget(const char *mname, const char *item)
{
    MenuEnt *ent = 0;
    if (!item) {
        MenuBox *mbox = FindMainMenu(mname);
        if (mbox)
            ent = mbox->menu;
    }
    else
        ent = FindEntry(mname, item, 0);
    if (ent && ent->cmd.caller)
        return (GTK_WIDGET(ent->cmd.caller));
    return (0);
}


#define MENU_DEBUG

// Disable/enable menus (item is null) or menu entries.
//
void
GTKmenu::DisableMainMenuItem(const char *mname, const char *item, bool desens)
{
    MenuEnt *ent = 0;
    if (!item) {
        MenuBox *mbox = FindMainMenu(mname);
        if (mbox)
            ent = mbox->menu;
    }
    else
        ent = FindEntry(mname, item, 0);
    if (ent && ent->cmd.caller)
        gtk_widget_set_sensitive(GTK_WIDGET(ent->cmd.caller), !desens);
#ifdef MENU_DEBUG
    else if (ent) {
        printf("DisableMainMenuItem assertion failed:  caller %p %s %s\n",
            ent->cmd.caller, mname, item);
    }
    else {
        printf("DisableMainMenuItem assertion failed:  caller failed %s %s\n",
            mname, item);
    }
#endif
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
// Create a pop-up menu.  The root arg can only be a GtkMenuItem, not
// a vanilla button.  Note that the menu item takes care of handling the
// pop-up signal, a vanilla button must provide a handler to actually
// display the menu (pass null root in this case).
//
GtkWidget *
GTKmenu::new_popup_menu(GtkWidget *root, const char *const *list,
    GCallback handler, void *arg)
{
    GtkWidget *menu = 0;
    if (list && *list) {
        menu = gtk_menu_new();
        for (const char *const *s = list; *s; s++) {

            if (!strcmp(*s, MENU_SEP_STRING)) {
                // insert a separator
                GtkWidget *msep = gtk_menu_item_new();  // separator
                gtk_menu_shell_append(GTK_MENU_SHELL(menu), msep);
                gtk_widget_show(msep);
                continue;
            }

            GtkWidget *menu_item = gtk_menu_item_new_with_label(*s);
            gtk_widget_set_name(menu_item, *s);
            gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
            g_signal_connect(G_OBJECT(menu_item), "activate",
                handler, arg);
            gtk_widget_show(menu_item);
        }
        if (root)
            gtk_menu_item_set_submenu(GTK_MENU_ITEM(root), menu);
    }
    return (menu);
}


#if GTK_CHECK_VERSION(3,0,0)
// Not needed and broken in gtk3.
#else

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
    gtk_widget_set_allocation(widget, allocation);

    gint nvis_children = 0;
    gint nexpand_children = 0;
    GList *children_list = gtk_container_get_children(GTK_CONTAINER(box));

    GList *children = children_list;
    while (children) {
        GtkWidget *child = GTK_WIDGET(children->data);
        children = children->next;

        if (gtk_widget_get_visible(child)) {
            nvis_children += 1;
            gboolean expand;
            gtk_box_query_child_packing(box, child, &expand, 0, 0, 0);
            if (expand)
                nexpand_children += 1;
        }
    }

    gint border_width = gtk_container_get_border_width(GTK_CONTAINER(box));
    if (nvis_children > 0) {
        gint height, extra, mod = 0;
        if (gtk_box_get_homogeneous(box)) {
            height = (allocation->height - border_width * 2 -
                (nvis_children - 1) * gtk_box_get_spacing(box));
            extra = height / nvis_children;
            mod = height % nvis_children;
        }
        else if (nexpand_children > 0) {
            GtkRequisition req;
            gtk_widget_get_requisition(widget, &req);
            height = (gint)allocation->height - (gint)req.height;
            extra = height / nexpand_children;
        }
        else {
            height = 0;
            extra = 0;
        }

        GtkAllocation child_allocation;
        gint y = allocation->y + border_width;
        child_allocation.x = allocation->x + border_width;
        child_allocation.width = MAX(1, (gint)allocation->width -
            border_width * 2);

        gint child_height;
        gint count = 0;
        children = children_list;
        while (children) {
            GtkWidget *child = GTK_WIDGET(children->data);
            children = children->next;

            GtkPackType pack;
            guint padding;
            gboolean expand;
            gboolean fill;
            gtk_box_query_child_packing(box, child, &expand, &fill,
                &padding, &pack);
            if ((pack == GTK_PACK_START) && gtk_widget_get_visible(child)) {
                if (gtk_box_get_homogeneous(box)) {
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

                    gtk_widget_get_child_requisition(child, &child_requisition);
                    child_height = child_requisition.height + padding * 2;

                    if (expand) {
                        if (nexpand_children == 1)
                            child_height += height;
                        else
                            child_height += extra;

                        nexpand_children -= 1;
                        height -= extra;
                    }
                }

                if (fill) {
                    child_allocation.height = MAX(1, child_height -
                        (gint)padding * 2);
                    child_allocation.y = y + padding;
                }
                else {
                    GtkRequisition child_requisition;

                    gtk_widget_get_child_requisition(child, &child_requisition);
                    child_allocation.height = child_requisition.height;
                    child_allocation.y = y + (child_height -
                        child_allocation.height) / 2;
                }

                gtk_widget_size_allocate(child, &child_allocation);

                y += child_height + gtk_box_get_spacing(GTK_BOX(box));
            }
        }

        y = allocation->y + allocation->height - border_width;

        children = children_list;
        while (children) {
            GtkWidget *child = GTK_WIDGET(children->data);
            children = children->next;

            GtkPackType pack;
            guint padding;
            gboolean expand;
            gboolean fill;
            gtk_box_query_child_packing(box, child, &expand, &fill,
                &padding, &pack);

            if ((pack == GTK_PACK_END) && gtk_widget_get_visible(child)) {
                GtkRequisition child_requisition;
                gtk_widget_get_child_requisition(child, &child_requisition);

                if (gtk_box_get_homogeneous(box)) {
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
                    child_height = child_requisition.height + padding * 2;

                    if (expand) {
                        if (nexpand_children == 1)
                            child_height += height;
                        else
                            child_height += extra;

                        nexpand_children -= 1;
                        height -= extra;
                    }
                }

                if (fill) {
                    child_allocation.height = MAX(1, child_height -
                        (gint)padding * 2);
                    child_allocation.y = y + padding - child_height;
                }
                else {
                    child_allocation.height = child_requisition.height;
                    child_allocation.y = y + (child_height -
                        child_allocation.height) / 2 - child_height;
                }

                gtk_widget_size_allocate(child, &child_allocation);

                y -= (child_height + gtk_box_get_spacing(box));
            }
        }
    }
    g_list_free(children_list);
}

#endif
// End of GTKmenu functions.


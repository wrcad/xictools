
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

#ifdef notdef

//--------------------------------------------------------------------
//
// Layer Alias Table Editor pop-up
//
//--------------------------------------------------------------------

struct sLA : public qt_bag
{
    sLA(GRobject c)
        {
            yn_cancel = 0;
            str_cancel = 0;
            calling_btn = c;
            row = -1;
            show_dec = false;
        }
    void update();

    GRobject yn_cancel;
    GRobject str_cancel;
    GRobject calling_btn;
    int row;
    bool show_dec;
};
static sLA *LA;


void
sLA::update()
{
    gtk_clist_clear(GTK_CLIST(viewport));
    char *str = CD()->PrintLayerAliases(show_dec);
    char *s0 = str;
    char *stok;
    while ((stok = gettok(&str)) != 0) {
        char *t = strchr(stok, '=');
        if (t) {
            *t++ = 0;
            char *strs[2];
            strs[0] = stok;
            strs[1] = t;
            gtk_clist_append(GTK_CLIST(viewport), strs);
        }
        delete [] stok;
    }
    delete [] s0;
}
 
static void la_cancel_proc(GtkWidget*, void*);
static void la_action_proc(GtkWidget*, void*, unsigned);
static void la_select_proc(GtkCList*, int, int, GdkEventButton*, void*);
static void la_unselect_proc(GtkCList*, int, int, GdkEventButton*, void*);

// function codes
enum {NoCode, CancelCode, OpenCode, SaveCode, NewCode, DeleteCode, EditCode,
    DecCode, HelpCode };

#define IFINIT(i, a, b, c, d, e) { \
    menu_items[i].path = a; \
    menu_items[i].accelerator = b; \
    menu_items[i].callback = (GtkItemFactoryCallback)c; \
    menu_items[i].callback_action = d; \
    menu_items[i].item_type = e; \
    i++; }

void
GTKapp::PopUpLayerAliases(int mode, GRobject caller)
{
    if (mode == MODE_OFF) {
        if (LA) {
            if (LA->calling_btn)
                QTdev::self()->Deselect(LA->calling_btn);
            GtkWidget *widg = LA->shell;
            LA->shell = 0;
            delete LA;
            LA = 0;
            gtk_widget_destroy(widg);
        }
        return;
    }
    if (mode == MODE_UPD) {
        if (LA)
            LA->update();
        return;
    }
    if (LA) {
        if (caller)
            QTdev::self()->Deselect(caller);
        return;
    }
    LA = new sLA(caller);
    LA->shell = gtk_NewPopup(0, "Layer Aliases", la_cancel_proc, 0);
    GtkWidget *form = gtk_table_new(1, 1, false);
    gtk_widget_show(form);
    gtk_container_add(GTK_CONTAINER(LA->shell), form);
    gtk_widget_set_usize(LA->shell, 200, 200);

    GtkItemFactoryEntry menu_items[20];
    int nitems = 0;

    IFINIT(nitems, "/_File", 0, 0, 0, "<Branch>")
    IFINIT(nitems, "/File/_Open", "<control>O", la_action_proc, OpenCode, 0);
    IFINIT(nitems, "/File/_Save", "<control>S", la_action_proc, SaveCode, 0);
    IFINIT(nitems, "/File/sep1", 0, 0, 0, "<Separator>");
    IFINIT(nitems, "/File/_Quit", "<control>Q", la_action_proc, CancelCode, 0);

    IFINIT(nitems, "/_Edit", 0, 0, 0, "<Branch>")
    IFINIT(nitems, "/Edit/_New", "<control>N", la_action_proc, NewCode, 0);
    IFINIT(nitems, "/Edit/_Delete", "<control>D", la_action_proc, DeleteCode, 0);
    IFINIT(nitems, "/Edit/_Edit", "<control>E", la_action_proc, EditCode, 0);
    IFINIT(nitems, "/Edit/Decimal _Form", "<control>F", la_action_proc,
        DecCode, "<CheckItem>");

    IFINIT(nitems, "/_Help", 0, 0, 0, "<LastBranch>");
    IFINIT(nitems, "/Help/_Help", "<control>H", la_action_proc, HelpCode, 0);

    GtkAccelGroup *accel_group = gtk_accel_group_new();
    GtkItemFactory *item_factory = gtk_item_factory_new(GTK_TYPE_MENU_BAR,
        "<aliases>", accel_group);
    for (int i = 0; i < nitems; i++)
        gtk_item_factory_create_item(item_factory, menu_items + i, 0, 2);
#if GTK_CHECK_VERSION(1,3,15)
    gtk_window_add_accel_group(GTK_WINDOW(LA->shell), accel_group);
#else
    gtk_accel_group_attach(accel_group, GTK_OBJECT(LA->shell));
#endif

    GtkWidget *menubar = gtk_item_factory_get_widget(item_factory, "<aliases>");
    gtk_widget_show(menubar);

    // name the menubar objects
    GtkWidget *widget = gtk_item_factory_get_item(item_factory, "/File");
    if (widget)
        gtk_widget_set_name(widget, "File");
    widget = gtk_item_factory_get_item(item_factory, "/Edit");
    if (widget)
        gtk_widget_set_name(widget, "Edit");
    widget = gtk_item_factory_get_item(item_factory, "/Help");
    if (widget)
        gtk_widget_set_name(widget, "Help");

    gtk_table_attach(GTK_TABLE(form), menubar, 0, 1, 0, 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    GtkWidget *swin = gtk_scrolled_window_new(0, 0);
    gtk_widget_show(swin);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin),
        GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_set_border_width(GTK_CONTAINER(swin), 2);

    char *titles[2];
    titles[0] = "Layer Name   ";
    titles[1] = "Alias";
    LA->viewport = gtk_clist_new_with_titles(2, titles);
    gtk_widget_show(LA->viewport);
    gtk_clist_set_shadow_type(GTK_CLIST(LA->viewport), GTK_SHADOW_IN);
    gtk_clist_column_titles_passive(GTK_CLIST(LA->viewport));
    gtk_container_add(GTK_CONTAINER(swin), LA->viewport);
    gtk_signal_connect(GTK_OBJECT(LA->viewport), "select-row",
        GTK_SIGNAL_FUNC(la_select_proc), 0);
    gtk_signal_connect(GTK_OBJECT(LA->viewport), "unselect-row",
        GTK_SIGNAL_FUNC(la_unselect_proc), 0);

    gtk_table_attach(GTK_TABLE(form), swin, 0, 1, 1, 2,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);

    GtkWidget *sep = gtk_hseparator_new();
    gtk_widget_show(sep);
    gtk_table_attach(GTK_TABLE(form), sep, 0, 1, 2, 3,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    GtkWidget *button = gtk_button_new_with_label("Dismiss");
    gtk_widget_set_name(button, "Dismiss");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(la_cancel_proc), 0);
    gtk_window_set_focus(GTK_WINDOW(LA->shell), button);
    QTdev::self()->SetDoubleClickExit(LA->shell, button);

    gtk_table_attach(GTK_TABLE(form), button, 0, 1, 3, 4,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    gtk_window_set_transient_for(GTK_WINDOW(LA->shell),
        GTK_WINDOW(GTKmainWin->shell));
    QTdev::self()->SetPopupLocation(GRloc(), LA->shell, GTKmainWin->shell);
    LA->update();
    gtk_widget_show(LA->shell);
}


static void
la_cancel_proc(GtkWidget*, void*)
{
    App->PopUpLayerAliases(MODE_OFF, 0);
}


static int
str_cb(const char *str, void *arg)
{
    if (arg == (void*)OpenCode) {
        if (str && *str) {
            char buf[256];
            FILE *fp = fopen(str, "r");
            if (!fp) {
                snprintf(buf, sizeof(buf), "Failed to open file\n%s",
                    strerror(errno));
                LA->PopUpMessage(buf, true, false, true);
            }
            else {
                CD()->ClearLayerAliases();
                CD()->ReadLayerAliases(fp);
                fclose(fp);
                LA->update();
            }
        }
        return (ESTR_DN);
    }
    if (arg == (void*)SaveCode) {
        if (str && *str) {
            char buf[256];
            FILE *fp = fopen(str, "w");
            if (!fp) {
                snprintf(buf, sizeof(buf), "Failed to open file\n%s",
                    strerror(errno));
                LA->PopUpMessage(buf, true, false, true);
            }
            else {
                CD()->DumpLayerAliases(fp);
                fclose(fp);
                LA->PopUpMessage("Layer alias data saved.", true, false, true);
            }
        }
        return (ESTR_DN);
    }
    if (arg == (void*)NewCode) {
        if (str && *str) {
            CD()->ParseLayerAliases(str);
            LA->update();
        }
        return (ESTR_IGN);
    }
    if (arg == (void*)EditCode) {
        if (str && *str) {
            const char *s0 = str;
            char *t = get_layer_tok(&str);
            CD()->RemoveLayerAlias(t);
            delete [] t;
            CD()->ParseLayerAliases(s0);
            LA->update();
        }
        return (ESTR_IGN);
    }
    return (ESTR_IGN);
}


static void
yn_cb(bool yn, void*)
{
    if (yn && LA->row >= 0) {
        char *text;
        gtk_clist_get_text(GTK_CLIST(LA->viewport), LA->row, 0, &text);
        CD()->RemoveLayerAlias(text);
        LA->update();
    }
}


static void
la_action_proc(GtkWidget *caller, void *client_data, unsigned code)
{
    int w, h;
    gdk_window_get_size(LA->shell->window, &w, &h);
    GRloc loc(LW_XYR, w + 4, 0);
    if (code == CancelCode)
        App->PopUpLayerAliases(MODE_OFF, 0);
    else if (code == OpenCode)
        LA->PopUpEditString(0, loc, "Enter file name to read aliases", 0,
            str_cb, (void*)OpenCode, 200, &LA->str_cancel, 0, false, 0);
    else if (code == SaveCode)
        LA->PopUpEditString(0, loc, "Enter file name to save aliases", 0,
            str_cb, (void*)SaveCode, 200, &LA->str_cancel, 0, false, 0);
    else if (code == NewCode)
        LA->PopUpEditString(0, loc, "Enter layer name and alias", 0,
            str_cb, (void*)NewCode, 200, &LA->str_cancel, 0, false, 0);
    else if (code == DeleteCode)
        LA->PopUpAffirm(0, loc, "Delete selected entry?", yn_cb, 0,
            &LA->yn_cancel);
    else if (code == EditCode) {
        if (LA->row >= 0) {
            char *n, *a;
            gtk_clist_get_text(GTK_CLIST(LA->viewport), LA->row, 0, &n);
            gtk_clist_get_text(GTK_CLIST(LA->viewport), LA->row, 1, &a);
            char buf[128];
            snprintf(buf, sizeof(buf), "%s %s", n, a);
            LA->PopUpEditString(0, loc, "Enter layer name and alias", buf,
                str_cb, (void*)EditCode, 200, &LA->str_cancel, 0, false, 0);
        }
    }
    else if (code == DecCode) {
        LA->show_dec = QTdev::self()->GetStatus(caller);
        LA->update();
    }
    else if (code == HelpCode)
        LA->PopUpHelp("layerchange");
}


static void
la_select_proc(GtkCList*, int row, int col, GdkEventButton*, void*)
{
    LA->row = row;
}


static void
la_unselect_proc(GtkCList*, int row, int col, GdkEventButton*, void*)
{
    LA->row = -1;
}

#endif


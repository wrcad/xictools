
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
#include "cd_strmdata.h"
#include "qtmain.h"
#include "layertab.h"

//--------------------------------------------------------------------
// Layer Editor pop-up
//

struct qtLcb : public sLcb
{
    qtLcb(GRobject);
    virtual ~qtLcb();

//    GtkWidget *Shell() { return (le_shell); }

    // virtual overrides
    void update(CDll*);
    char *layername();
    void desel_rem();
    void popdown();

/*
private:
    static void le_popdown(GtkWidget*, void*);
    static void le_add_proc(GtkWidget*, void*);
    static void le_rem_proc(GtkWidget*, void*);
    static char *le_get_lname(GtkWidget*, GtkWidget*);

    GRobject le_caller;     // initiating button

    GtkWidget *le_shell;    // pop-up shell
    GtkWidget *le_add;      // add layer button
    GtkWidget *le_rem;      // remove layer button
    GtkWidget *le_opmenu;   // removed layers menu;

    static const char *initmsg;
*/
};


// Pop up the Layer editor.  The editor has buttons to add a layer, remove
// layers, and a combo box for layer name entry.  The combo contains a
// list of previously removed layers.
//
sLcb *
cMain::PopUpLayerEditor(GRobject c)
{
    /*
    if (!GRX || !QTmainwin::self())
        return (0);
    gtkLcb *cbs = new gtkLcb(c);
    if (!cbs->Shell()) {
        delete cbs;
        return (0);
    }

    gtk_window_set_transient_for(GTK_WINDOW(cbs->Shell()),
        GTK_WINDOW(QTmainwin::self()->Shell()));
    GRX->SetPopupLocation(GRloc(), cbs->Shell(), QTmainwin::self()->viewport);
    gtk_widget_show(cbs->Shell());
    return (cbs);
    */
return (0);
}
// End of cMain functions.


// Update the list of removed layers
//
void
qtLcb::update(CDll *list)
{
/*
    if (!this)
        return;
    gtk_list_clear_items(GTK_LIST(opmenu), 0, -1);
    if (!list) {
        GtkWidget *text =
            (GtkWidget*)gtk_object_get_data(GTK_OBJECT(shell), "text");
        if (text)
            gtk_entry_set_text(GTK_ENTRY(text), "");
    }
    for (CDll *l = list; l; l = l->next) {
        GtkWidget *item = gtk_list_item_new_with_label(l->ldesc->name);
        gtk_widget_show(item);
        gtk_container_add(GTK_CONTAINER(opmenu), item);
    }
*/
(void)list;
}


// Return the current name in the text box.  The return value should be
// freed.  If the value isn't good, 0 is returned.  This is called when
// adding layers
//
char *
qtLcb::layername()
{
/*
    GtkWidget *text =
        (GtkWidget*)gtk_object_get_data(GTK_OBJECT(shell), "text");
    if (!text)
        return (0);
    GtkWidget *label =
        (GtkWidget*)gtk_object_get_data(GTK_OBJECT(shell), "label");
    char *string = get_lname(text, label);
    if (!string) {
        GRX->Deselect(add);
        add_cb(false);
        return (0);
    }
    return (string);
*/
return (0);
}


// Pop down the widget (from the caller)
//
void
qtLcb::popdown()
{
//    le_popdown(0, this);
}


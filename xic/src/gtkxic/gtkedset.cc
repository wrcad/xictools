
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
#include "edit.h"
#include "undolist.h"
#include "dsp_inlines.h"
#include "tech.h"
#include "gtkmain.h"
#include "gtkinlines.h"
#include "gtkspinbtn.h"


//--------------------------------------------------------------------
// Pop-up to set edit defaults.
//
// Help system keywords used:
//  xic:edset

namespace {
    namespace gtkedset {
        struct sEd
        {
            sEd(GRobject);
            ~sEd();

            void update();

            GtkWidget *shell() { return (ed_popup); }

        private:
            static void ed_cancel_proc(GtkWidget*, void*);
            static void ed_action(GtkWidget*, void*);
            static void ed_val_changed(GtkWidget*, void*);
            static void ed_depth_menu_proc(GtkWidget*, void*);

            GRobject ed_caller;
            GtkWidget *ed_popup;
            GtkWidget *ed_cons45;
            GtkWidget *ed_merge;
            GtkWidget *ed_noply;
            GtkWidget *ed_prompt;
            GtkWidget *ed_noww;
            GtkWidget *ed_crcovr;
            GtkWidget *ed_depth;

            GTKspinBtn sb_ulen;
            GTKspinBtn sb_maxgobjs;
        };

        sEd *Ed;
    }

    const char *depthvals[] =
    {
        "as expanded",
        "0",
        "1",
        "2",
        "3",
        "4",
        "5",
        "6",
        "7",
        "8",
        0
    };
}

using namespace gtkedset;


void
cEdit::PopUpEditSetup(GRobject caller, ShowMode mode)
{
    if (!GRX || !mainBag())
        return;
    if (mode == MODE_OFF) {
        delete Ed;
        return;
    }
    if (mode == MODE_UPD) {
        if (Ed)
            Ed->update();
        return;
    }
    if (Ed)
        return;

    new sEd(caller);
    if (!Ed->shell()) {
        delete Ed;
        return;
    }
    gtk_window_set_transient_for(GTK_WINDOW(Ed->shell()),
        GTK_WINDOW(mainBag()->Shell()));

    GRX->SetPopupLocation(GRloc(), Ed->shell(), mainBag()->Viewport());
    gtk_widget_show(Ed->shell());
}

sEd::sEd(GRobject c)
{
    Ed = this;
    ed_caller = c;
    ed_cons45 = 0;
    ed_merge = 0;
    ed_noply = 0;
    ed_prompt = 0;
    ed_noww = 0;
    ed_crcovr = 0;
    ed_depth = 0;

    ed_popup = gtk_NewPopup(0, "Editing Setup", ed_cancel_proc, 0);
    if (!ed_popup)
        return;
    gtk_window_set_resizable(GTK_WINDOW(ed_popup), false);

    GtkWidget *form = gtk_table_new(2, 1, false);
    gtk_widget_show(form);
    gtk_container_set_border_width(GTK_CONTAINER(form), 2);
    gtk_container_add(GTK_CONTAINER(ed_popup), form);
    int rowcnt = 0;

    //
    // label in frame plus help btn
    //
    GtkWidget *row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);
    GtkWidget *label = gtk_label_new("Set editing flags and parameters");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    GtkWidget *frame = gtk_frame_new(0);
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), label);
    gtk_box_pack_start(GTK_BOX(row), frame, true, true, 0);
    GtkWidget *button = gtk_button_new_with_label("Help");
    gtk_widget_set_name(button, "Help");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(ed_action), 0);
    gtk_box_pack_end(GTK_BOX(row), button, false, false, 0);
    gtk_table_attach(GTK_TABLE(form), row, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // check boxes
    //
    button = gtk_check_button_new_with_label(
        "Constrain angles to 45 degree multiples");
    gtk_widget_set_name(button, "cons45");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(ed_action), 0);
    gtk_table_attach(GTK_TABLE(form), button, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;
    ed_cons45 = button;

    button = gtk_check_button_new_with_label(
        "Merge new boxes and polys with existing boxes/polys");
    gtk_widget_set_name(button, "merge");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(ed_action), 0);
    gtk_table_attach(GTK_TABLE(form), button, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;
    ed_merge = button;

    button = gtk_check_button_new_with_label(
        "Clip and merge new boxes only, not polys");
    gtk_widget_set_name(button, "noply");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(ed_action), 0);
    gtk_table_attach(GTK_TABLE(form), button, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;
    ed_noply = button;

    button = gtk_check_button_new_with_label(
        "Prompt to save modified native cells");
    gtk_widget_set_name(button, "prompt");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(ed_action), 0);
    gtk_table_attach(GTK_TABLE(form), button, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;
    ed_prompt = button;

    button = gtk_check_button_new_with_label(
        "No wire width change in magnification");
    gtk_widget_set_name(button, "noww");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(ed_action), 0);
    gtk_table_attach(GTK_TABLE(form), button, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;
    ed_noww = button;

    button = gtk_check_button_new_with_label(
        "Allow Create Cell to overwrite existing cell");
    gtk_widget_set_name(button, "crcovr");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(ed_action), 0);
    gtk_table_attach(GTK_TABLE(form), button, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;
    ed_crcovr = button;

    //
    // integer parameters
    //
    row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);

    label = gtk_label_new("Maximum undo list length");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_box_pack_start(GTK_BOX(row), label, true, true, 0);

    GtkWidget *sb = sb_ulen.init(DEF_MAX_UNDO_LEN, 0.0, 1000.0, 0);
    sb_ulen.connect_changed(GTK_SIGNAL_FUNC(ed_val_changed), 0, "ulen");
    gtk_widget_set_usize(sb, 80, -1);
    gtk_box_pack_end(GTK_BOX(row), sb, false, false, 0);

    gtk_table_attach(GTK_TABLE(form), row, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);

    label = gtk_label_new("Maximum number of ghost-drawn objects");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_box_pack_start(GTK_BOX(row), label, true, true, 0);

    sb = sb_maxgobjs.init(DEF_MAX_GHOST_OBJECTS, 50.0, 50000.0, 0);
    sb_maxgobjs.connect_changed(GTK_SIGNAL_FUNC(ed_val_changed), 0, "maxg");
    gtk_widget_set_usize(sb, 80, -1);
    gtk_box_pack_end(GTK_BOX(row), sb, false, false, 0);

    gtk_table_attach(GTK_TABLE(form), row, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);

    label = gtk_label_new("Maximum subcell depth in ghosting");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_box_pack_start(GTK_BOX(row), label, true, true, 0);

    GtkWidget *entry = gtk_option_menu_new();
    gtk_widget_set_name(entry, "depthmenu");
    gtk_widget_show(entry);
    GtkWidget *menu = gtk_menu_new();
    gtk_widget_set_name(menu, "depthmenu");
    for (int i = 0; depthvals[i]; i++) {
        GtkWidget *mi = gtk_menu_item_new_with_label(depthvals[i]);
        gtk_widget_set_name(mi, depthvals[i]);
        gtk_widget_show(mi);
        gtk_menu_append(GTK_MENU(menu), mi);
        gtk_signal_connect(GTK_OBJECT(mi), "activate",
            GTK_SIGNAL_FUNC(ed_depth_menu_proc), (void*)depthvals[i]);
    }
    gtk_option_menu_set_menu(GTK_OPTION_MENU(entry), menu);
    gtk_box_pack_end(GTK_BOX(row), entry, false, false, 0);
    ed_depth = entry;

    gtk_table_attach(GTK_TABLE(form), row, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // Dismiss button
    //
    button = gtk_button_new_with_label("Dismiss");
    gtk_widget_set_name(button, "Dismiss");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(ed_cancel_proc), 0);

    gtk_table_attach(GTK_TABLE(form), button, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);
    gtk_window_set_focus(GTK_WINDOW(ed_popup), button);

    update();
}


sEd::~sEd()
{
    Ed = 0;
    if (ed_caller)
        GRX->Deselect(ed_caller);
    if (ed_popup)
        gtk_widget_destroy(ed_popup);
}


void
sEd::update()
{
    GRX->SetStatus(ed_cons45, CDvdb()->getVariable(VA_Constrain45));
    GRX->SetStatus(ed_merge, !CDvdb()->getVariable(VA_NoMergeObjects));
    GRX->SetStatus(ed_noply, CDvdb()->getVariable(VA_NoMergePolys));
    GRX->SetStatus(ed_prompt, CDvdb()->getVariable(VA_AskSaveNative));

    int n;
    const char *s = sb_ulen.get_string();
    if (sscanf(s, "%d", &n) != 1 || n != Ulist()->UndoLength())
        sb_ulen.set_value(Ulist()->UndoLength());

    s = sb_maxgobjs.get_string();
    if (sscanf(s, "%d", &n) != 1 ||
            (unsigned int)n != EGst()->maxGhostObjects())
        sb_ulen.set_value(EGst()->maxGhostObjects());

    GRX->SetStatus(ed_noww, CDvdb()->getVariable(VA_NoWireWidthMag));
    GRX->SetStatus(ed_crcovr, CDvdb()->getVariable(VA_CrCellOverwrite));

    gtk_widget_set_sensitive(ed_noply, GRX->GetStatus(ed_merge));

    int hst = 0;
    const char *str = CDvdb()->getVariable(VA_MaxGhostDepth);
    if (str)
        hst = atoi(str) + 1;
    gtk_option_menu_set_history(GTK_OPTION_MENU(ed_depth), hst);
}


// Static function.
void
sEd::ed_cancel_proc(GtkWidget*, void*)
{
    ED()->PopUpEditSetup(0, MODE_OFF);
}


// Static function.
void
sEd::ed_action(GtkWidget *caller, void*)
{
    if (!Ed)
        return;
    const char *name = gtk_widget_get_name(caller);
    if (!name)
        return;
    if (!strcmp(name, "Help")) {
        DSPmainWbag(PopUpHelp("xic:edset"))
    }
    else if (!strcmp(name, "cons45")) {
        if (GRX->GetStatus(caller))
            CDvdb()->setVariable(VA_Constrain45, "");
        else
            CDvdb()->clearVariable(VA_Constrain45);
    }
    else if (!strcmp(name, "merge")) {
        if (GRX->GetStatus(caller))
            CDvdb()->clearVariable(VA_NoMergeObjects);
        else
            CDvdb()->setVariable(VA_NoMergeObjects, "");
    }
    else if (!strcmp(name, "noply")) {
        if (GRX->GetStatus(caller))
            CDvdb()->setVariable(VA_NoMergePolys, "");
        else
            CDvdb()->clearVariable(VA_NoMergePolys);
    }
    else if (!strcmp(name, "prompt")) {
        if (GRX->GetStatus(caller))
            CDvdb()->setVariable(VA_AskSaveNative, "");
        else
            CDvdb()->clearVariable(VA_AskSaveNative);
    }
    else if (!strcmp(name, "noww")) {
        if (GRX->GetStatus(caller))
            CDvdb()->setVariable(VA_NoWireWidthMag, "");
        else
            CDvdb()->clearVariable(VA_NoWireWidthMag);
    }
    else if (!strcmp(name, "crcovr")) {
        if (GRX->GetStatus(caller))
            CDvdb()->setVariable(VA_CrCellOverwrite, "");
        else
            CDvdb()->clearVariable(VA_CrCellOverwrite);
    }
}


// Static function.
void
sEd::ed_val_changed(GtkWidget *caller, void*)
{
    if (!Ed)
        return;
    const char *name = gtk_widget_get_name(caller);
    if (!name)
        return;
    if (!strcmp(name, "ulen")) {
        const char *s = Ed->sb_ulen.get_string();
        int n;
        if (sscanf(s, "%d", &n) == 1 && n >= 0 && n <= 1000) {
            if (n == DEF_MAX_UNDO_LEN)
                CDvdb()->clearVariable(VA_UndoListLength);
            else
                CDvdb()->setVariable(VA_UndoListLength, s);
        }
    }
    else if (!strcmp(name, "maxg")) {
        const char *s = Ed->sb_maxgobjs.get_string();
        int n;
        if (sscanf(s, "%d", &n) == 1 && n >= 50 && n <= 50000) {
            if (n == DEF_MAX_GHOST_OBJECTS)
                CDvdb()->clearVariable(VA_MaxGhostObjects);
            else
                CDvdb()->setVariable(VA_MaxGhostObjects, s);
        }
    }
}


// Static function.
void
sEd::ed_depth_menu_proc(GtkWidget*, void *client_data)
{
    char *s = (char*)client_data;
    if (isdigit(*s))
        CDvdb()->setVariable(VA_MaxGhostDepth, s);
    else
        CDvdb()->clearVariable(VA_MaxGhostDepth);
}


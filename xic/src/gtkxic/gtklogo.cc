
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
#include "dsp_inlines.h"
#include "gtkmain.h"
#include "gtkinlines.h"
#include "gtkinterf/gtkspinbtn.h"
#include "ginterf/grfont.h"
#include "miscutil/filestat.h"


namespace {
    namespace gtklogo {
        struct sLgo : public gtk_bag
        {
            sLgo(GRobject);
            ~sLgo();

            void update();

        private:
            static void lgo_cancel_proc(GtkWidget*, void*);
            static void lgo_action(GtkWidget*, void*);
            static void lgo_es_menu_proc(GtkWidget*, void*);
            static void lgo_pw_menu_proc(GtkWidget*, void*);
            static void lgo_val_changed(GtkWidget*, void*);
            static ESret lgo_sav_cb(const char*, void*);

            GRobject lgo_caller;
            GtkWidget *lgo_vector;
            GtkWidget *lgo_manh;
            GtkWidget *lgo_pretty;
            GtkWidget *lgo_setpix;
            GtkWidget *lgo_endstyle;
            GtkWidget *lgo_pwidth;
            GtkWidget *lgo_create;
            GtkWidget *lgo_dump;
            GtkWidget *lgo_sel;
            GRledPopup *lgo_sav_pop;

            GTKspinBtn lgo_sb_pix;

            static double lgo_defpixsz;
        };

        sLgo *Lgo;
    }

    const char *endstyles[] =
    {
        "flush",
        "rounded",
        "extended",
        0
    };

    const char *pathwidth[] =
    {
        "thinner",
        "thin",
        "medium",
        "thick",
        "thicker",
        0
    };


    inline bool str_to_int(int *iret, const char *s)
    {
        if (!s)
            return (false);
        return (sscanf(s, "%d", iret) == 1);
    }
}

using namespace gtklogo;

double sLgo::lgo_defpixsz = 1.0;


void
cEdit::PopUpLogo(GRobject caller, ShowMode mode)
{
    if (!GRX || !mainBag())
        return;
    if (mode == MODE_OFF) {
        delete Lgo;
        return;
    }
    if (mode == MODE_UPD) {
        if (Lgo)
            Lgo->update();
        return;
    }
    if (Lgo)
        return;

    new sLgo(caller);
    if (!Lgo->Shell()) {
        delete Lgo;
        return;
    }
    gtk_window_set_transient_for(GTK_WINDOW(Lgo->Shell()),
        GTK_WINDOW(mainBag()->Shell()));

    GRX->SetPopupLocation(GRloc(LW_LL), Lgo->Shell(), mainBag()->Viewport());
    gtk_widget_show(Lgo->Shell());
}

sLgo::sLgo(GRobject c)
{
    Lgo = this;
    lgo_caller = c;
    lgo_vector = 0;
    lgo_manh = 0;
    lgo_pretty = 0;
    lgo_setpix = 0;
    lgo_endstyle = 0;
    lgo_pwidth = 0;
    lgo_create = 0;
    lgo_dump = 0;
    lgo_sel = 0;
    lgo_sav_pop = 0;

    wb_shell = gtk_NewPopup(0, "Logo Font Setup", lgo_cancel_proc, 0);
    if (!wb_shell)
        return;
    gtk_window_set_resizable(GTK_WINDOW(wb_shell), false);

    GtkWidget *form = gtk_table_new(2, 1, false);
    gtk_widget_show(form);
    gtk_container_set_border_width(GTK_CONTAINER(form), 2);
    gtk_container_add(GTK_CONTAINER(wb_shell), form);
    int rowcnt = 0;

    // Font selection radio buttons
    //
    GtkWidget *row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);

    GtkWidget *label = gtk_label_new("Font:  ");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_box_pack_start(GTK_BOX(row), label, true, true, 0);

    GtkWidget *button = gtk_radio_button_new_with_label(0, "Vector");
    gtk_widget_set_name(button, "Vector");
    gtk_widget_show(button);
    GSList *group = gtk_radio_button_group(GTK_RADIO_BUTTON(button));
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(lgo_action), 0);
    gtk_box_pack_start(GTK_BOX(row), button, true, true, 0);
    lgo_vector = button;

    button = gtk_radio_button_new_with_label(group, "Manhattan");
    gtk_widget_set_name(button, "Manhattan");
    gtk_widget_show(button);
    group = gtk_radio_button_group(GTK_RADIO_BUTTON(button));
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(lgo_action), 0);
    gtk_box_pack_start(GTK_BOX(row), button, true, true, 0);
    lgo_manh = button;

    button = gtk_radio_button_new_with_label(group, "Pretty");
    gtk_widget_set_name(button, "Pretty");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(lgo_action), 0);
    gtk_box_pack_start(GTK_BOX(row), button, true, true, 0);
    lgo_pretty = button;

    gtk_table_attach(GTK_TABLE(form), row, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    button = gtk_check_button_new_with_label("Define \"pixel\" size");
    gtk_widget_set_name(button, "pixsz");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(lgo_action), 0);
    lgo_setpix = button;

    gtk_table_attach(GTK_TABLE(form), button, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    int ndgt = CD()->numDigits();
    GtkWidget *sb = lgo_sb_pix.init(lgo_defpixsz, MICRONS(1), 100.0, ndgt);
    lgo_sb_pix.connect_changed(GTK_SIGNAL_FUNC(lgo_val_changed), 0, 0);

    gtk_table_attach(GTK_TABLE(form), sb, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    label = gtk_label_new("Vector end style");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);

    gtk_table_attach(GTK_TABLE(form), label, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    GtkWidget *entry = gtk_option_menu_new();
    gtk_widget_set_name(entry, "endstyle");
    gtk_widget_show(entry);
    GtkWidget *menu = gtk_menu_new();
    gtk_widget_set_name(menu, "esmenu");

    for (int i = 0; endstyles[i]; i++) {
        GtkWidget *mi = gtk_menu_item_new_with_label(endstyles[i]);
        gtk_widget_set_name(mi, endstyles[i]);
        gtk_widget_show(mi);
        gtk_menu_append(GTK_MENU(menu), mi);
        gtk_signal_connect(GTK_OBJECT(mi), "activate",
            GTK_SIGNAL_FUNC(lgo_es_menu_proc), (void*)(long)i);
    }
    gtk_option_menu_set_menu(GTK_OPTION_MENU(entry), menu);
    lgo_endstyle = entry;

    gtk_table_attach(GTK_TABLE(form), entry, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    label = gtk_label_new("Vector path width");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);

    gtk_table_attach(GTK_TABLE(form), label, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    entry = gtk_option_menu_new();
    gtk_widget_set_name(entry, "pathwidth");
    gtk_widget_show(entry);
    menu = gtk_menu_new();
    gtk_widget_set_name(menu, "pwmenu");

    for (int i = 0; pathwidth[i]; i++) {
        GtkWidget *mi = gtk_menu_item_new_with_label(pathwidth[i]);
        gtk_widget_set_name(mi, pathwidth[i]);
        gtk_widget_show(mi);
        gtk_menu_append(GTK_MENU(menu), mi);
        gtk_signal_connect(GTK_OBJECT(mi), "activate",
            GTK_SIGNAL_FUNC(lgo_pw_menu_proc), (void*)(long)(i+1));
    }
    gtk_option_menu_set_menu(GTK_OPTION_MENU(entry), menu);
    lgo_pwidth = entry;

    gtk_table_attach(GTK_TABLE(form), entry, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    button = gtk_check_button_new_with_label("Create cell for text");
    gtk_widget_set_name(button, "crcell");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(lgo_action), 0);
    lgo_create = button;

    gtk_table_attach(GTK_TABLE(form), button, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    button = gtk_toggle_button_new_with_label("Dump Vector Font ");
    gtk_widget_set_name(button, "Dump");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(lgo_action), 0);
    lgo_dump = button;

    gtk_table_attach(GTK_TABLE(form), button, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);
    rowcnt++;

    button = gtk_toggle_button_new_with_label("Select Pretty Font");
    gtk_widget_set_name(button, "Select");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(lgo_action), 0);
    lgo_sel = button;

    gtk_table_attach(GTK_TABLE(form), button, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);

    button = gtk_button_new_with_label("Dismiss");
    gtk_widget_set_name(button, "Dismiss");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(lgo_cancel_proc), 0);

    gtk_table_attach(GTK_TABLE(form), button, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);
    gtk_window_set_focus(GTK_WINDOW(wb_shell), button);

    update();
}


sLgo::~sLgo()
{
    Lgo = 0;
    ED()->PopUpPolytextFont(0, MODE_OFF);
    if (lgo_caller)
        GRX->Deselect(lgo_caller);
    if (lgo_sav_pop)
        lgo_sav_pop->popdown();
    if (wb_shell)
        gtk_signal_disconnect_by_func(GTK_OBJECT(wb_shell),
            GTK_SIGNAL_FUNC(lgo_cancel_proc), wb_shell);
}


void
sLgo::update()
{
    int dd;
    if (str_to_int(&dd, CDvdb()->getVariable(VA_LogoAltFont)) &&
            dd >= 0 && dd <= 1) {
        if (dd == 0)
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lgo_manh), true);
        else
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lgo_pretty), true);
    }
    else
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lgo_vector), true);

    const char *pix = CDvdb()->getVariable(VA_LogoPixelSize);
    if (pix) {
        char *nstr;
        double d = strtod(pix, &nstr);
        if (nstr != pix)
            lgo_sb_pix.set_value(d);
        GRX->SetStatus(lgo_setpix, true);
        lgo_sb_pix.set_sensitive(true);
    }
    else {
        GRX->SetStatus(lgo_setpix, false);
        lgo_sb_pix.set_sensitive(false);
    }
    ED()->assert_logo_pixel_size();

    if (str_to_int(&dd, CDvdb()->getVariable(VA_LogoPathWidth)) &&
            dd >= 1 && dd <= 5)
        gtk_option_menu_set_history(GTK_OPTION_MENU(lgo_pwidth), dd - 1);
    else
        gtk_option_menu_set_history(GTK_OPTION_MENU(lgo_pwidth),
            DEF_LOGO_PATH_WIDTH - 1);

    if (str_to_int(&dd, CDvdb()->getVariable(VA_LogoEndStyle)) &&
            dd >= 0 && dd <= 2)
        gtk_option_menu_set_history(GTK_OPTION_MENU(lgo_endstyle), dd);
    else
        gtk_option_menu_set_history(GTK_OPTION_MENU(lgo_endstyle),
            DEF_LOGO_END_STYLE);

    if (CDvdb()->getVariable(VA_LogoToFile))
        GRX->SetStatus(lgo_create, true);
    else
        GRX->SetStatus(lgo_create, false);
}


// Static function.
void
sLgo::lgo_cancel_proc(GtkWidget*, void*)
{
    ED()->PopUpLogo(0, MODE_OFF);
}


// Static function.
void
sLgo::lgo_action(GtkWidget *caller, void*)
{
   if (!Lgo)
        return;
    const char *name = gtk_widget_get_name(caller);
    if (!strcmp(name, "pixsz")) {
        bool state = GRX->GetStatus(caller);
        if (state) {
            const char *s = Lgo->lgo_sb_pix.get_string();
            CDvdb()->setVariable(VA_LogoPixelSize, s);
        }
        else
            CDvdb()->clearVariable(VA_LogoPixelSize);
        return;
    }
    if (!strcmp(name, "crcell")) {
        bool state = GRX->GetStatus(caller);
        if (state)
            CDvdb()->setVariable(VA_LogoToFile, "");
        else
            CDvdb()->clearVariable(VA_LogoToFile);
        return;
    }
    if (!strcmp(name, "Select")) {
        if (GRX->GetStatus(caller))
            ED()->PopUpPolytextFont(caller, MODE_ON);
        else
            ED()->PopUpPolytextFont(0, MODE_OFF);
        return;
    }
    if (!strcmp(name, "Dump")) {
        if (Lgo->lgo_sav_pop)
            Lgo->lgo_sav_pop->popdown();
        if (GRX->GetStatus(caller)) {
            Lgo->lgo_sav_pop = Lgo->PopUpEditString((GRobject)Lgo->lgo_dump,
                GRloc(), "Enter pathname for font file: ",
                XM()->LogoFontFileName(), Lgo->lgo_sav_cb,
                0, 250, 0, false, 0);
            if (Lgo->lgo_sav_pop)
                Lgo->lgo_sav_pop->register_usrptr((void**)&Lgo->lgo_sav_pop);
        }
        return;
    }

    if (!GRX->GetStatus(caller))
        return;
    if (!strcmp(name, "Vector"))
        CDvdb()->clearVariable(VA_LogoAltFont);
    else if (!strcmp(name, "Manhattan"))
        CDvdb()->setVariable(VA_LogoAltFont, "0");
    else if (!strcmp(name, "Pretty"))
        CDvdb()->setVariable(VA_LogoAltFont, "1");
}


// Static function.
void
sLgo::lgo_es_menu_proc(GtkWidget*, void *client_data)
{
    if (!Lgo)
        return;
    char buf[32];
    int es = (intptr_t)client_data;
    if (es >= 0 && es <= 2 && es != DEF_LOGO_END_STYLE) {
        sprintf(buf, "%d", es);
        CDvdb()->setVariable(VA_LogoEndStyle, buf);
    }
    else
        CDvdb()->clearVariable(VA_LogoEndStyle);
}


// Static function.
void
sLgo::lgo_pw_menu_proc(GtkWidget*, void *client_data)
{
    if (!Lgo)
        return;
    char buf[32];
    int pw = (intptr_t)client_data;
    if (pw >= 1 && pw <= 5 && pw != DEF_LOGO_PATH_WIDTH) {
        sprintf(buf, "%d", pw);
        CDvdb()->setVariable(VA_LogoPathWidth, buf);
    }
    else
        CDvdb()->clearVariable(VA_LogoPathWidth);
}


// Static function.
void
sLgo::lgo_val_changed(GtkWidget*, void*)
{
    if (!Lgo)
        return;
    const char *s = Lgo->lgo_sb_pix.get_string();
    char *endp;
    double d = strtod(s, &endp);
    if (endp > s) {
        lgo_defpixsz = d;
        if (GRX->GetStatus(Lgo->lgo_setpix))
            CDvdb()->setVariable(VA_LogoPixelSize, s);
    }
}


// Static function.
// Callback for the Save dialog.
//
ESret
sLgo::lgo_sav_cb(const char *fname, void*)
{
    if (!fname)
        return (ESTR_IGN);
    char *tok = lstring::getqtok(&fname);
    if (!tok)
        return (ESTR_IGN);
    if (filestat::create_bak(tok)) {
        FILE *fp = fopen(tok, "w");
        delete [] tok;
        if (fp) {
            ED()->logoFont()->dumpFont(fp);
            fclose(fp);
            Lgo->PopUpMessage("Logo vector font saved in file.", false);
        }
        else
            GRpkgIf()->Perror(lstring::strip_path(tok));
    }
    else
        GRpkgIf()->ErrPrintf(ET_ERROR, "%s", filestat::error_msg());
    delete [] tok;
    return (ESTR_DN);
}


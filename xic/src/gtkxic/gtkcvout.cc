
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
#include "cvrt.h"
#include "fio.h"
#include "dsp_inlines.h"
#include "gtkmain.h"
#include "gtkcv.h"
#include "gtkinlines.h"


//--------------------------------------------------------------------
// Pop-up to write cell data files.
//
// Help system keywords used:
//  xic:exprt

namespace {
    typedef bool(*CvoCallback)(FileType, bool, void*);

    namespace gtkcvout {
        struct sCvo
        {
            sCvo(GRobject, CvoCallback, void*);
            ~sCvo();

            void update();

            GtkWidget *shell() { return (cvo_popup); }

        private:
            static void cvo_cancel_proc(GtkWidget*, void*);
            static void cvo_action(GtkWidget*, void*);
            static void cvo_format_proc(int);
            static void cvo_val_changed(GtkWidget*, void*);
            static WndSensMode wnd_sens_test();

            GRobject cvo_caller;
            GtkWidget *cvo_popup;
            cvofmt_t *cvo_fmt;
            GtkWidget *cvo_strip;
            GtkWidget *cvo_wrall;
            GtkWidget *cvo_pcsub;
            GtkWidget *cvo_viasub;
            GtkWidget *cvo_allcells;
            GtkWidget *cvo_invis_p;
            GtkWidget *cvo_invis_e;
            CvoCallback cvo_callback;
            void *cvo_arg;
            cnmap_t *cvo_cnmap;
            wnd_t *cvo_wnd;
            GTKspinBtn sb_scale;
            bool cvo_useallcells;

            static int cvo_fmt_type;
        };

        sCvo *Cvo;
    }

    struct fmtval_t
    {
        fmtval_t(const char *n, FileType t) { name = n; filetype = t; }

        const char *name;
        FileType filetype;
    };

    fmtval_t fmtvals[] =
    {
        fmtval_t("GDSII", Fgds),
        fmtval_t("OASIS", Foas),
        fmtval_t("CIF", Fcif),
        fmtval_t("CGX", Fcgx),
        fmtval_t("Xic Cell Files", Fnative),
        fmtval_t(0, Fnone)
    };
}

using namespace gtkcvout;

int sCvo::cvo_fmt_type = cConvert::cvGds;


void
cConvert::PopUpExport(GRobject caller, ShowMode mode,
    bool (*callback)(FileType, bool, void*), void *arg)
{
    if (!GRX || !mainBag())
        return;
    if (mode == MODE_OFF) {
        delete Cvo;
        return;
    }
    if (mode == MODE_UPD) {
        if (Cvo)
            Cvo->update();
        return;
    }
    if (Cvo)
        return;

    new sCvo(caller, callback, arg);
    if (!Cvo->shell()) {
        delete Cvo;
        return;
    }
    gtk_window_set_transient_for(GTK_WINDOW(Cvo->shell()),
        GTK_WINDOW(mainBag()->Shell()));

    GRX->SetPopupLocation(GRloc(LW_UR), Cvo->shell(), mainBag()->Viewport());
    gtk_widget_show(Cvo->shell());
}


sCvo::sCvo(GRobject c, CvoCallback callback, void *arg)
{
    Cvo = this;
    cvo_caller = c;
    cvo_popup = 0;
    cvo_fmt = 0;
    cvo_strip = 0;
    cvo_wrall = 0;
    cvo_pcsub = 0;
    cvo_viasub = 0;
    cvo_allcells = 0;
    cvo_invis_p = 0;
    cvo_invis_e = 0;
    cvo_callback = callback;
    cvo_arg = arg;
    cvo_cnmap = 0;
    cvo_wnd = 0;
    cvo_useallcells = false;

    cvo_popup = gtk_NewPopup(0, "Export Control", cvo_cancel_proc, 0);
    if (!cvo_popup)
        return;
    gtk_window_set_resizable(GTK_WINDOW(cvo_popup), false);

    GtkWidget *form = gtk_table_new(2, 1, false);
    gtk_widget_show(form);
    gtk_container_set_border_width(GTK_CONTAINER(form), 2);
    gtk_container_add(GTK_CONTAINER(cvo_popup), form);
    int rowcnt = 0;

    //
    // label in frame plus help btn
    //
    GtkWidget *row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);
    GtkWidget *label = gtk_label_new("Write cell data file");
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
        GTK_SIGNAL_FUNC(cvo_action), 0);
    gtk_box_pack_end(GTK_BOX(row), button, false, false, 0);
    gtk_table_attach(GTK_TABLE(form), row, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // Format selection notebook
    //
    cvo_fmt = new cvofmt_t(cvo_format_proc, cvo_fmt_type, cvofmt_asm);
    gtk_table_attach(GTK_TABLE(form), cvo_fmt->frame(), 0, 2, rowcnt,
        rowcnt+1, (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // Cell name mapping
    //
    cvo_cnmap = new cnmap_t(true);
    gtk_table_attach(GTK_TABLE(form), cvo_cnmap->frame(), 0, 2, rowcnt,
        rowcnt+1, (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // Window
    //
    cvo_wnd = new wnd_t(wnd_sens_test, true);
    gtk_table_attach(GTK_TABLE(form), cvo_wnd->frame(), 0, 2, rowcnt,
        rowcnt+1, (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // Scale spin button and label
    //
    label = gtk_label_new("Conversion Scale Factor");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_table_attach(GTK_TABLE(form), label, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    GtkWidget *sb = sb_scale.init(FIO()->WriteScale(), CDSCALEMIN, CDSCALEMAX,
        5);
    sb_scale.connect_changed(GTK_SIGNAL_FUNC(cvo_val_changed), 0);
    gtk_widget_set_usize(sb, 100, -1);

    gtk_table_attach(GTK_TABLE(form), sb, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    GtkWidget *sep = gtk_hseparator_new();
    gtk_widget_show(sep);
    gtk_table_attach(GTK_TABLE(form), sep, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // Invisible layer conversion
    //
    row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);
    label = gtk_label_new("Don't convert invisible layers:");
    gtk_widget_show(label);
    gtk_box_pack_start(GTK_BOX(row), label, true, true, 0);
    button = gtk_check_button_new_with_label("Physical");
    gtk_widget_set_name(button, "invis_p");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(cvo_action), 0);
    gtk_box_pack_start(GTK_BOX(row), button, true, true, 0);
    cvo_invis_p = button;
    button = gtk_check_button_new_with_label("Electrical");
    gtk_widget_set_name(button, "invis_e");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(cvo_action), 0);
    gtk_box_pack_start(GTK_BOX(row), button, true, true, 0);
    cvo_invis_e = button;
    const char *s = CDvdb()->getVariable(VA_SkipInvisible);
    if (!s) {
        GRX->SetStatus(cvo_invis_p, false);
        GRX->SetStatus(cvo_invis_e, false);
    }
    else {
        if (*s != 'e' && *s != 'E')
            GRX->SetStatus(cvo_invis_p, true);
        else
            GRX->SetStatus(cvo_invis_p, false);
        if (*s != 'p' && *s != 'P')
            GRX->SetStatus(cvo_invis_e, true);
        else
            GRX->SetStatus(cvo_invis_e, false);
    }
    gtk_table_attach(GTK_TABLE(form), row, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // Check boxes
    //
    button = gtk_check_button_new_with_label(
        "Strip For Export - (convert physical data only)");
    gtk_widget_set_name(button, "strip");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(cvo_action), 0);
    GRX->SetStatus(button, CDvdb()->getVariable(VA_StripForExport));
    gtk_table_attach(GTK_TABLE(form), button, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;
    cvo_strip = button;

    button = gtk_check_button_new_with_label("Include Library Cells");
    gtk_widget_set_name(button, "libcells");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(cvo_action), 0);
    GRX->SetStatus(button, CDvdb()->getVariable(VA_WriteAllCells));
    gtk_table_attach(GTK_TABLE(form), button, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;
    cvo_wrall = button;

    button = gtk_check_button_new_with_label(
        "Include parameterized cell sub-masters");
    gtk_widget_set_name(button, "pcsub");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(cvo_action), 0);
    gtk_table_attach(GTK_TABLE(form), button, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;
    cvo_pcsub = button;

    button = gtk_check_button_new_with_label(
        "Include standard via cell sub-masters");
    gtk_widget_set_name(button, "viasub");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(cvo_action), 0);
    gtk_table_attach(GTK_TABLE(form), button, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;
    cvo_viasub = button;

    button = gtk_check_button_new_with_label(
        "Consider ALL cells in current symbol table for output");
    gtk_widget_set_name(button, "allcells");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(cvo_action), 0);
    gtk_table_attach(GTK_TABLE(form), button, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;
    cvo_allcells = button;

    //
    // Write File and Dismiss buttons
    //
    button = gtk_button_new_with_label("Write File");
    gtk_widget_set_name(button, "WriteFile");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(cvo_action), 0);
    gtk_table_attach(GTK_TABLE(form), button, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);

    button = gtk_button_new_with_label("Dismiss");
    gtk_widget_set_name(button, "Dismiss");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(cvo_cancel_proc), 0);

    gtk_table_attach(GTK_TABLE(form), button, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);
    gtk_window_set_focus(GTK_WINDOW(cvo_popup), button);

    update();
}


sCvo::~sCvo()
{
    Cvo = 0;
    delete cvo_fmt;
    delete cvo_cnmap;
    delete cvo_wnd;
    if (cvo_caller)
        GRX->Deselect(cvo_caller);
    if (cvo_callback)
        (*cvo_callback)(Fnone, false, cvo_arg);
    if (cvo_popup)
        gtk_widget_destroy(cvo_popup);
}


void
sCvo::update()
{
    cvo_fmt->update();
    GRX->SetStatus(cvo_strip, CDvdb()->getVariable(VA_StripForExport));
    GRX->SetStatus(cvo_wrall, CDvdb()->getVariable(VA_WriteAllCells));
    GRX->SetStatus(cvo_pcsub, CDvdb()->getVariable(VA_PCellKeepSubMasters));
    GRX->SetStatus(cvo_viasub, CDvdb()->getVariable(VA_ViaKeepSubMasters));

    const char *s = CDvdb()->getVariable(VA_SkipInvisible);
    if (!s) {
        GRX->SetStatus(cvo_invis_p, false);
        GRX->SetStatus(cvo_invis_e, false);
    }
    else {
        if (*s != 'e' && *s != 'E')
            GRX->SetStatus(cvo_invis_p, true);
        else
            GRX->SetStatus(cvo_invis_p, false);
        if (*s != 'p' && *s != 'P')
            GRX->SetStatus(cvo_invis_e, true);
        else
            GRX->SetStatus(cvo_invis_e, false);
    }

    sb_scale.set_value(FIO()->WriteScale());
    cvo_cnmap->update();
    cvo_wnd->update();
    cvo_wnd->set_sens();
}


// Static function.
void
sCvo::cvo_cancel_proc(GtkWidget*, void*)
{
    Cvt()->PopUpExport(0, MODE_OFF, 0, 0);
}


// Static function.
void
sCvo::cvo_action(GtkWidget *caller, void*)
{
    if (!Cvo)
        return;
    const char *name = gtk_widget_get_name(caller);
    if (!strcmp(name, "Help")) {
        DSPmainWbag(PopUpHelp("xic:exprt"))
        return;
    }
    if (!strcmp(name, "strip")) {
        if (GRX->GetStatus(caller))
            CDvdb()->setVariable(VA_StripForExport, 0);
        else
            CDvdb()->clearVariable(VA_StripForExport);
        return;
    }
    if (!strcmp(name, "libcells")) {
        if (GRX->GetStatus(caller))
            CDvdb()->setVariable(VA_WriteAllCells, 0);
        else
            CDvdb()->clearVariable(VA_WriteAllCells);
        return;
    }
    if (!strcmp(name, "pcsub")) {
        if (GRX->GetStatus(caller))
            CDvdb()->setVariable(VA_PCellKeepSubMasters, "");
        else
            CDvdb()->clearVariable(VA_PCellKeepSubMasters);
        return;
    }
    if (!strcmp(name, "viasub")) {
        if (GRX->GetStatus(caller))
            CDvdb()->setVariable(VA_ViaKeepSubMasters, "");
        else
            CDvdb()->clearVariable(VA_ViaKeepSubMasters);
        return;
    }
    if (!strcmp(name, "allcells")) {
        Cvo->cvo_useallcells = GRX->GetStatus(caller);
        return;
    }
    if (!strcmp(name, "invis_p")) {
        bool ske = GRX->GetStatus(Cvo->cvo_invis_e);
        if (GRX->GetStatus(caller)) {
            if (ske)
                CDvdb()->setVariable(VA_SkipInvisible, 0);
            else
                CDvdb()->setVariable(VA_SkipInvisible, "p");
        }
        else {
            if (ske)
                CDvdb()->setVariable(VA_SkipInvisible, "e");
            else
                CDvdb()->clearVariable(VA_SkipInvisible);
        }
        return;
    }
    if (!strcmp(name, "invis_e")) {
        bool skp = GRX->GetStatus(Cvo->cvo_invis_p);
        if (GRX->GetStatus(caller)) {
            if (skp)
                CDvdb()->setVariable(VA_SkipInvisible, 0);
            else
                CDvdb()->setVariable(VA_SkipInvisible, "e");
        }
        else {
            if (skp)
                CDvdb()->setVariable(VA_SkipInvisible, "p");
            else
                CDvdb()->clearVariable(VA_SkipInvisible);
        }
        return;
    }
    if (!strcmp(name, "WriteFile")) {
        if (!Cvo->cvo_callback ||
                !(*Cvo->cvo_callback)(fmtvals[cvo_fmt_type].filetype,
                    Cvo->cvo_useallcells, Cvo->cvo_arg))
            Cvt()->PopUpExport(0, MODE_OFF, 0, 0);
    }
}


// Static function.
void
sCvo::cvo_format_proc(int type)
{
    cvo_fmt_type = type;
    Cvo->cvo_wnd->set_sens();
}


// Static function.
void
sCvo::cvo_val_changed(GtkWidget*, void*)
{
    if (!Cvo)
        return;
    const char *s = Cvo->sb_scale.get_string();
    char *endp;
    double d = strtod(s, &endp);
    if (endp > s && d >= CDSCALEMIN && d <= CDSCALEMAX)
        FIO()->SetWriteScale(d);
}


// Static function.
WndSensMode
sCvo::wnd_sens_test()
{
    return (WndSensFlatten);
}



/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2015 Whiteley Research Inc, all rights reserved.        *
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
 $Id: gtkvia.cc,v 5.13 2017/03/14 01:26:41 stevew Exp $
 *========================================================================*/

#include "main.h"
#include "edit.h"
#include "tech.h"
#include "tech_via.h"
#include "fio.h"
#include "dsp_inlines.h"
#include "gtkmain.h"
#include "gtkinlines.h"
#include "gtkcv.h"
#include "cd_lgen.h"
#include "errorlog.h"

#define WID_DEF     0.1
#define WID_MIN     0.01
#define WID_MAX     10.0
#define HEI_DEF     0.1
#define HEI_MIN     0.01
#define HEI_MAX     10.0
#define ROWS_DEF    1
#define ROWS_MIN    1
#define ROWS_MAX    100
#define COLS_DEF    1
#define COLS_MIN    1
#define COLS_MAX    100
#define SPA_X_DEF   0.0
#define SPA_X_MIN   0.0
#define SPA_X_MAX   10.0
#define SPA_Y_DEF   0.0
#define SPA_Y_MIN   0.0
#define SPA_Y_MAX   10.0
#define ENC1_X_DEF  0.0
#define ENC1_X_MIN  0.0
#define ENC1_X_MAX  10.0
#define ENC1_Y_DEF  0.0
#define ENC1_Y_MIN  0.0
#define ENC1_Y_MAX  10.0
#define OFF1_X_DEF  0.0
#define OFF1_X_MIN  0.0
#define OFF1_X_MAX  10.0
#define OFF1_Y_DEF  0.0
#define OFF1_Y_MIN  0.0
#define OFF1_Y_MAX  10.0
#define ENC2_X_DEF  0.0
#define ENC2_X_MIN  0.0
#define ENC2_X_MAX  10.0
#define ENC2_Y_DEF  0.0
#define ENC2_Y_MIN  0.0
#define ENC2_Y_MAX  10.0
#define OFF2_X_DEF  0.0
#define OFF2_X_MIN  0.0
#define OFF2_X_MAX  10.0
#define OFF2_Y_DEF  0.0
#define OFF2_Y_MIN  0.0
#define OFF2_Y_MAX  10.0
#define ORG_X_DEF   0.0
#define ORG_X_MIN   0.0
#define ORG_X_MAX   10.0
#define ORG_Y_DEF   0.0
#define ORG_Y_MIN   0.0
#define ORG_Y_MAX   10.0
#define IMP1_X_DEF  0.0
#define IMP1_X_MIN  0.0
#define IMP1_X_MAX  10.0
#define IMP1_Y_DEF  0.0
#define IMP1_Y_MIN  0.0
#define IMP1_Y_MAX  10.0
#define IMP2_X_DEF  0.0
#define IMP2_X_MIN  0.0
#define IMP2_X_MAX  10.0
#define IMP2_Y_DEF  0.0
#define IMP2_Y_MIN  0.0
#define IMP2_Y_MAX  10.0

//--------------------------------------------------------------------
// Pop-up to create a standard via.
//
// Help system keywords used:
//  xic:stvia

namespace {
    namespace gtkstdvia {
        struct sStv
        {
            sStv(GRobject);
            ~sStv();

            void update();

            GtkWidget *shell() { return (stv_popup); }

        private:
            static void stv_cancel_proc(GtkWidget*, void*);
            static void stv_action(GtkWidget*, void*);
            static void stv_vlayer_menu_proc(GtkWidget*, void*);
            static void stv_name_menu_proc(GtkWidget*, void*);

            GRobject stv_caller;
            GtkWidget *stv_popup;
            GtkWidget *stv_name;
            GtkWidget *stv_layer1;
            GtkWidget *stv_layer2;
            GtkWidget *stv_layerv;
            GtkWidget *stv_imp1;
            GtkWidget *stv_imp2;
            GtkWidget *stv_apply;

            GTKspinBtn sb_wid;
            GTKspinBtn sb_hei;
            GTKspinBtn sb_rows;
            GTKspinBtn sb_cols;
            GTKspinBtn sb_spa_x;
            GTKspinBtn sb_spa_y;
            GTKspinBtn sb_enc1_x;
            GTKspinBtn sb_enc1_y;
            GTKspinBtn sb_off1_x;
            GTKspinBtn sb_off1_y;
            GTKspinBtn sb_enc2_x;
            GTKspinBtn sb_enc2_y;
            GTKspinBtn sb_off2_x;
            GTKspinBtn sb_off2_y;
            GTKspinBtn sb_org_x;
            GTKspinBtn sb_org_y;
            GTKspinBtn sb_imp1_x;
            GTKspinBtn sb_imp1_y;
            GTKspinBtn sb_imp2_x;
            GTKspinBtn sb_imp2_y;
        };

        sStv *Stv;
    }
}

using namespace gtkstdvia;


void
cTech::PopUpStdVia(GRobject caller, ShowMode mode)
{
    if (!GRX || !mainBag())
        return;
    if (mode == MODE_OFF) {
        delete Stv;
        return;
    }
    if (mode == MODE_UPD) {
        if (Stv)
            Stv->update();
        return;
    }
    if (Stv)
        return;

    new sStv(caller);
    if (!Stv->shell()) {
        delete Stv;
        return;
    }
    gtk_window_set_transient_for(GTK_WINDOW(Stv->shell()),
        GTK_WINDOW(mainBag()->Shell()));

    GRX->SetPopupLocation(GRloc(LW_LL), Stv->shell(), mainBag()->Viewport());
    gtk_widget_show(Stv->shell());
}

sStv::sStv(GRobject c)
{
    Stv = this;
    stv_caller = c;
    stv_name = 0;
    stv_layer1 = 0;
    stv_layer2 = 0;
    stv_layerv = 0;
    stv_imp1 = 0;
    stv_imp2 = 0;
    stv_apply = 0;

    stv_popup = gtk_NewPopup(0, "Via Creation", stv_cancel_proc, 0);
    if (!stv_popup)
        return;
    gtk_window_set_resizable(GTK_WINDOW(stv_popup), false);

    GtkWidget *form = gtk_table_new(3, 1, false);
    gtk_widget_show(form);
    gtk_container_set_border_width(GTK_CONTAINER(form), 2);
    gtk_container_add(GTK_CONTAINER(stv_popup), form);
    int rowcnt = 0;

    //
    // label in frame plus help btn
    //
    GtkWidget *row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);
    GtkWidget *label = gtk_label_new("Set parameters for via");
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
        GTK_SIGNAL_FUNC(stv_action), 0);
    gtk_box_pack_end(GTK_BOX(row), button, false, false, 0);
    gtk_table_attach(GTK_TABLE(form), row, 0, 3, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

#define NUMWID 100
    label = gtk_label_new("Via name, cut layer");
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(form), label, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
#if GTK_CHECK_VERSION(2,24,0)
    stv_name = gtk_combo_box_text_new();
#else
    stv_name = gtk_combo_box_new_text();
#endif
    gtk_widget_show(stv_name);
    gtk_signal_connect(GTK_OBJECT(stv_name), "changed",
        GTK_SIGNAL_FUNC(stv_name_menu_proc), 0);
    gtk_table_attach(GTK_TABLE(form), stv_name, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
#if GTK_CHECK_VERSION(2,24,0)
    stv_layerv = gtk_combo_box_text_new();
#else
    stv_layerv = gtk_combo_box_new_text();
#endif
    gtk_widget_show(stv_layerv);
    gtk_signal_connect(GTK_OBJECT(stv_layerv), "changed",
        GTK_SIGNAL_FUNC(stv_vlayer_menu_proc), 0);
    gtk_table_attach(GTK_TABLE(form), stv_layerv, 2, 3, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    // Fill in the cut layers, only those that are used by an existing
    // standard via.

    CDlgen lgen(Physical);
    CDl *ld;
    while ((ld = lgen.next()) != 0) {
        if (!ld->isVia())
            continue;

        tgen_t<sStdVia> gen(Tech()->StdViaTab());
        const sStdVia *sv;
        while ((sv = gen.next()) != 0) {
            if (sv->via() == ld) {
#if GTK_CHECK_VERSION(2,24,0)
                gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(stv_layerv),
                    ld->name());
#else
                gtk_combo_box_append_text(GTK_COMBO_BOX(stv_layerv),
                    ld->name());
#endif
                break;
            }
        }
    }

    label = gtk_label_new("Layer 1, layer 2");
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(form), label, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    stv_layer1 = gtk_entry_new();
    gtk_entry_set_editable(GTK_ENTRY(stv_layer1), false);
    gtk_widget_set_usize(stv_layer1, NUMWID, -1);
    gtk_widget_show(stv_layer1);
    gtk_table_attach(GTK_TABLE(form), stv_layer1, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    stv_layer2 = gtk_entry_new();
    gtk_entry_set_editable(GTK_ENTRY(stv_layer2), false);
    gtk_widget_set_usize(stv_layer2, NUMWID, -1);
    gtk_widget_show(stv_layer2);
    gtk_table_attach(GTK_TABLE(form), stv_layer2, 2, 3, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    int ndgt = CD()->numDigits();
    label = gtk_label_new("Cut width, height");
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(form), label, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    GtkWidget *sb = sb_wid.init(WID_DEF, WID_MIN, WID_MAX, ndgt);
    gtk_widget_set_usize(sb, NUMWID, -1);
    gtk_table_attach(GTK_TABLE(form), sb, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    sb = sb_hei.init(HEI_DEF, HEI_MIN, HEI_MAX, ndgt);
    gtk_widget_set_usize(sb, NUMWID, -1);
    gtk_table_attach(GTK_TABLE(form), sb, 2, 3, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    label = gtk_label_new("Cut rows, columns");
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(form), label, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    sb = sb_rows.init(ROWS_DEF, ROWS_MIN, ROWS_MAX, 0);
    gtk_widget_set_usize(sb, NUMWID, -1);
    gtk_table_attach(GTK_TABLE(form), sb, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    sb = sb_cols.init(COLS_DEF, COLS_MIN, COLS_MAX, 0);
    gtk_widget_set_usize(sb, NUMWID, -1);
    gtk_table_attach(GTK_TABLE(form), sb, 2, 3, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    label = gtk_label_new("Cut spacing X,Y");
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(form), label, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    sb = sb_spa_x.init(SPA_X_DEF, SPA_X_MIN, SPA_X_MAX, ndgt);
    gtk_widget_set_usize(sb, NUMWID, -1);
    gtk_table_attach(GTK_TABLE(form), sb, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    sb = sb_spa_y.init(SPA_Y_DEF, SPA_Y_MIN, SPA_Y_MAX, ndgt);
    gtk_widget_set_usize(sb, NUMWID, -1);
    gtk_table_attach(GTK_TABLE(form), sb, 2, 3, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    label = gtk_label_new("Enclosure 1 X,Y");
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(form), label, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    sb = sb_enc1_x.init(ENC1_X_DEF, ENC1_X_MIN, ENC1_X_MAX, ndgt);
    gtk_widget_set_usize(sb, NUMWID, -1);
    gtk_table_attach(GTK_TABLE(form), sb, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    sb = sb_enc1_y.init(ENC1_Y_DEF, ENC1_Y_MIN, ENC1_Y_MAX, ndgt);
    gtk_widget_set_usize(sb, NUMWID, -1);
    gtk_table_attach(GTK_TABLE(form), sb, 2, 3, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    label = gtk_label_new("Offset 1 X,Y");
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(form), label, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    sb = sb_off1_x.init(OFF1_X_DEF, OFF1_X_MIN, OFF1_X_MAX, ndgt);
    gtk_widget_set_usize(sb, NUMWID, -1);
    gtk_table_attach(GTK_TABLE(form), sb, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    sb = sb_off1_y.init(OFF1_Y_DEF, OFF1_Y_MIN, OFF1_Y_MAX, ndgt);
    gtk_widget_set_usize(sb, NUMWID, -1);
    gtk_table_attach(GTK_TABLE(form), sb, 2, 3, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    label = gtk_label_new("Enclosure 2 X,Y");
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(form), label, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    sb = sb_enc2_x.init(ENC2_X_DEF, ENC2_X_MIN, ENC2_X_MAX, ndgt);
    gtk_widget_set_usize(sb, NUMWID, -1);
    gtk_table_attach(GTK_TABLE(form), sb, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    sb = sb_enc2_y.init(ENC2_Y_DEF, ENC2_Y_MIN, ENC2_Y_MAX, ndgt);
    gtk_widget_set_usize(sb, NUMWID, -1);
    gtk_table_attach(GTK_TABLE(form), sb, 2, 3, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    label = gtk_label_new("Offset 2 X,Y");
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(form), label, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    sb = sb_off2_x.init(OFF2_X_DEF, OFF2_X_MIN, OFF2_X_MAX, ndgt);
    gtk_widget_set_usize(sb, NUMWID, -1);
    gtk_table_attach(GTK_TABLE(form), sb, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    sb = sb_off2_y.init(OFF2_Y_DEF, OFF2_Y_MIN, OFF2_Y_MAX, ndgt);
    gtk_widget_set_usize(sb, NUMWID, -1);
    gtk_table_attach(GTK_TABLE(form), sb, 2, 3, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    label = gtk_label_new("Origin offset X,Y");
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(form), label, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    sb = sb_org_x.init(ORG_X_DEF, ORG_X_MIN, ORG_X_MAX, ndgt);
    gtk_widget_set_usize(sb, NUMWID, -1);
    gtk_table_attach(GTK_TABLE(form), sb, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    sb = sb_org_y.init(ORG_Y_DEF, ORG_Y_MIN, ORG_Y_MAX, ndgt);
    gtk_widget_set_usize(sb, NUMWID, -1);
    gtk_table_attach(GTK_TABLE(form), sb, 2, 3, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    label = gtk_label_new("Implant 1, implant 2");
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(form), label, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    stv_imp1 = gtk_entry_new();
    gtk_entry_set_editable(GTK_ENTRY(stv_imp1), false);
    gtk_widget_set_usize(stv_imp1, NUMWID, -1);
    gtk_widget_show(stv_imp1);
    gtk_table_attach(GTK_TABLE(form), stv_imp1, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    stv_imp2 = gtk_entry_new();
    gtk_entry_set_editable(GTK_ENTRY(stv_imp2), false);
    gtk_widget_set_usize(stv_imp2, NUMWID, -1);
    gtk_widget_show(stv_imp2);
    gtk_table_attach(GTK_TABLE(form), stv_imp2, 2, 3, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    label = gtk_label_new("Implant 1 Enc X,Y");
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(form), label, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    sb = sb_imp1_x.init(IMP1_X_DEF, IMP1_X_MIN, IMP1_X_MAX, ndgt);
    gtk_widget_set_usize(sb, NUMWID, -1);
    gtk_table_attach(GTK_TABLE(form), sb, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    sb = sb_imp1_y.init(IMP1_Y_DEF, IMP1_Y_MIN, IMP1_Y_MAX, ndgt);
    gtk_widget_set_usize(sb, NUMWID, -1);
    gtk_table_attach(GTK_TABLE(form), sb, 2, 3, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    label = gtk_label_new("Implant 2 enc X,Y");
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(form), label, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    sb = sb_imp2_x.init(IMP2_X_DEF, IMP2_X_MIN, IMP2_X_MAX, ndgt);
    gtk_widget_set_usize(sb, NUMWID, -1);
    gtk_table_attach(GTK_TABLE(form), sb, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    sb = sb_imp2_y.init(IMP2_Y_DEF, IMP2_Y_MIN, IMP2_Y_MAX, ndgt);
    gtk_widget_set_usize(sb, NUMWID, -1);
    gtk_table_attach(GTK_TABLE(form), sb, 2, 3, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // Apply and Dismiss buttons
    //
    button = gtk_button_new_with_label("Apply");
    gtk_widget_set_name(button, "Apply");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(stv_action), 0);
    stv_apply = button;
    gtk_table_attach(GTK_TABLE(form), button, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);

    button = gtk_button_new_with_label("Dismiss");
    gtk_widget_set_name(button, "Dismiss");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(stv_cancel_proc), 0);
    gtk_table_attach(GTK_TABLE(form), button, 1, 3, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);
    gtk_window_set_focus(GTK_WINDOW(stv_popup), button);

    gtk_combo_box_set_active(GTK_COMBO_BOX(Stv->stv_layerv), 0);
}


sStv::~sStv()
{
    Stv = 0;
    if (stv_caller)
        GRX->Deselect(stv_caller);
    if (stv_popup)
        gtk_widget_destroy(stv_popup);
}


void
sStv::update()
{
#if GTK_CHECK_VERSION(2,24,0)
    char *vlyr =
        gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(stv_layerv));
#else
    char *vlyr = gtk_combo_box_get_active_text(GTK_COMBO_BOX(stv_layerv));
#endif
    if (!vlyr)
        return;
    CDl *ldv = CDldb()->findLayer(vlyr, Physical);
    g_free(vlyr);
    if (!ldv)
        return;

    gtk_combo_box_set_model(GTK_COMBO_BOX(stv_name), 0);

    tgen_t<sStdVia> gen(Tech()->StdViaTab());
    const sStdVia *sv;
    while ((sv = gen.next()) != 0) {
        if (sv->via() == ldv) {
#if GTK_CHECK_VERSION(2,24,0)
            gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(stv_name),
                sv->tab_name());
#else
            gtk_combo_box_append_text(GTK_COMBO_BOX(stv_name), sv->tab_name());
#endif
        }
    }
}


// Static function.
void
sStv::stv_cancel_proc(GtkWidget*, void*)
{
    Tech()->PopUpStdVia(0, MODE_OFF);
}


// Static function.
void
sStv::stv_action(GtkWidget *caller, void*)
{
    if (!Stv)
        return;
    const char *name = gtk_widget_get_name(caller);
    if (!strcmp(name, "Help")) {
        DSPmainWbag(PopUpHelp("xic:crvia"))
        return;
    }
    if (!strcmp(name, "Apply")) {
        if (!Tech()->StdViaTab())
            return;
#if GTK_CHECK_VERSION(2,24,0)
        char *nm = gtk_combo_box_text_get_active_text(
            GTK_COMBO_BOX_TEXT(Stv->stv_name));
#else
        char *nm = gtk_combo_box_get_active_text(GTK_COMBO_BOX(Stv->stv_name));
#endif
        if (!nm)
            return;
        sStdVia *sv = Tech()->FindStdVia(nm);
        g_free(nm);
        if (!sv)
            return;
        if (!sv->bottom() || !sv->top())
            return;
        sStdVia rv(0, sv->via(), sv->bottom(), sv->top());
        rv.set_via_wid(INTERNAL_UNITS(Stv->sb_wid.get_value()));
        rv.set_via_hei(INTERNAL_UNITS(Stv->sb_hei.get_value()));
        rv.set_via_rows(mmRnd(Stv->sb_rows.get_value()));
        rv.set_via_cols(mmRnd(Stv->sb_cols.get_value()));
        rv.set_via_spa_x(INTERNAL_UNITS(Stv->sb_spa_x.get_value()));
        rv.set_via_spa_y(INTERNAL_UNITS(Stv->sb_spa_y.get_value()));
        rv.set_bot_enc_x(INTERNAL_UNITS(Stv->sb_enc1_x.get_value()));
        rv.set_bot_enc_y(INTERNAL_UNITS(Stv->sb_enc1_y.get_value()));
        rv.set_bot_off_x(INTERNAL_UNITS(Stv->sb_off1_x.get_value()));
        rv.set_bot_off_y(INTERNAL_UNITS(Stv->sb_off1_y.get_value()));
        rv.set_top_enc_x(INTERNAL_UNITS(Stv->sb_enc2_x.get_value()));
        rv.set_top_enc_y(INTERNAL_UNITS(Stv->sb_enc2_y.get_value()));
        rv.set_top_off_x(INTERNAL_UNITS(Stv->sb_off2_x.get_value()));
        rv.set_top_off_y(INTERNAL_UNITS(Stv->sb_off2_y.get_value()));
        rv.set_org_off_x(INTERNAL_UNITS(Stv->sb_org_x.get_value()));
        rv.set_org_off_y(INTERNAL_UNITS(Stv->sb_org_y.get_value()));
        rv.set_implant1(sv->implant1());
        rv.set_imp1_enc_x(INTERNAL_UNITS(Stv->sb_imp1_x.get_value()));
        rv.set_imp1_enc_y(INTERNAL_UNITS(Stv->sb_imp1_y.get_value()));
        rv.set_implant2(sv->implant2());
        rv.set_imp2_enc_x(INTERNAL_UNITS(Stv->sb_imp2_x.get_value()));
        rv.set_imp2_enc_y(INTERNAL_UNITS(Stv->sb_imp2_y.get_value()));

        CDs *sd = 0;
        for (sStdVia *v = sv; v; v = v->variations()) {
            if (*v == rv) {
                sd = v->open();
                break;
            }
        }
        if (!sd) {
            sStdVia *nv = new sStdVia(rv);
            sv->add_variation(nv);
            sd = nv->open();
        }
        if (!sd) {
            // This really can't happen.
            Log()->PopUpErr("Failed to create standard via sub-master.");
            return;
        }

        // Place instance
        ED()->placeDev(Stv->stv_apply, Tstring(sd->cellname()), false);
    }
}


// Static function.
//
void
sStv::stv_vlayer_menu_proc(GtkWidget*, void*)
{
    if (!Stv)
        return;
#if GTK_CHECK_VERSION(2,24,0)
    char *vlyr =
        gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(Stv->stv_layerv));
#else
    char *vlyr = gtk_combo_box_get_active_text(GTK_COMBO_BOX(Stv->stv_layerv));
#endif
    if (!vlyr)
        return;
    CDl *ldv = CDldb()->findLayer(vlyr, Physical);
    g_free(vlyr);
    if (!ldv)
        return;

    GtkTreeModel *tm = gtk_combo_box_get_model(GTK_COMBO_BOX(Stv->stv_name));
    gtk_list_store_clear(GTK_LIST_STORE(tm));

    tgen_t<sStdVia> gen(Tech()->StdViaTab());
    const sStdVia *sv;
    while ((sv = gen.next()) != 0) {
        if (sv->via() == ldv) {
#if GTK_CHECK_VERSION(2,24,0)
            gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(Stv->stv_name),
                sv->tab_name());
#else
            gtk_combo_box_append_text(GTK_COMBO_BOX(Stv->stv_name),
                sv->tab_name());
#endif
        }
    }
    gtk_combo_box_set_active(GTK_COMBO_BOX(Stv->stv_name), 0);
}


// Static function.
//
void
sStv::stv_name_menu_proc(GtkWidget*, void*)
{
    if (!Stv)
        return;
    if (!Tech()->StdViaTab())
        return;
#if GTK_CHECK_VERSION(2,24,0)
    char *nm =
        gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(Stv->stv_name));
#else
    char *nm = gtk_combo_box_get_active_text(GTK_COMBO_BOX(Stv->stv_name));
#endif
    if (!nm)
        return;
    const sStdVia *sv = Tech()->FindStdVia(nm);
    g_free(nm);
    if (!sv)
        return;
    if (!sv->bottom() || !sv->top())
        return;
    gtk_entry_set_text(GTK_ENTRY(Stv->stv_layer1), sv->bottom()->name());
    gtk_entry_set_text(GTK_ENTRY(Stv->stv_layer2), sv->top()->name());
    Stv->sb_wid.set_value(MICRONS(sv->via_wid()));
    Stv->sb_hei.set_value(MICRONS(sv->via_hei()));
    Stv->sb_rows.set_value(sv->via_rows());
    Stv->sb_cols.set_value(sv->via_cols());
    Stv->sb_spa_x.set_value(MICRONS(sv->via_spa_x()));
    Stv->sb_spa_y.set_value(MICRONS(sv->via_spa_y()));
    Stv->sb_enc1_x.set_value(MICRONS(sv->bot_enc_x()));
    Stv->sb_enc1_y.set_value(MICRONS(sv->bot_enc_y()));
    Stv->sb_off1_x.set_value(MICRONS(sv->bot_off_x()));
    Stv->sb_off1_y.set_value(MICRONS(sv->bot_off_y()));
    Stv->sb_enc2_x.set_value(MICRONS(sv->top_enc_x()));
    Stv->sb_enc2_y.set_value(MICRONS(sv->top_enc_y()));
    Stv->sb_off2_x.set_value(MICRONS(sv->top_off_x()));
    Stv->sb_off2_y.set_value(MICRONS(sv->top_off_y()));
    Stv->sb_org_x.set_value(MICRONS(sv->org_off_x()));
    Stv->sb_org_y.set_value(MICRONS(sv->org_off_y()));
    CDl *ldim1 = sv->implant1();
    if (ldim1) {
        gtk_entry_set_text(GTK_ENTRY(Stv->stv_imp1), ldim1->name());
        Stv->sb_imp1_x.set_value(MICRONS(sv->imp1_enc_x()));
        Stv->sb_imp1_y.set_value(MICRONS(sv->imp1_enc_y()));
        gtk_widget_set_sensitive(Stv->stv_imp1, true);
        Stv->sb_imp1_x.set_sensitive(true);
        Stv->sb_imp1_y.set_sensitive(true);
    }
    else {
        gtk_entry_set_text(GTK_ENTRY(Stv->stv_imp1), "");
        Stv->sb_imp1_x.set_value(0);
        Stv->sb_imp1_y.set_value(0);
        gtk_widget_set_sensitive(Stv->stv_imp1, false);
        Stv->sb_imp1_x.set_sensitive(false);
        Stv->sb_imp1_y.set_sensitive(false);
    }
    CDl *ldim2 = sv->implant2();
    if (ldim1 && ldim2) {
        gtk_entry_set_text(GTK_ENTRY(Stv->stv_imp2), ldim2->name());
        Stv->sb_imp2_x.set_value(MICRONS(sv->imp2_enc_x()));
        Stv->sb_imp2_y.set_value(MICRONS(sv->imp2_enc_y()));
        gtk_widget_set_sensitive(Stv->stv_imp2, true);
        Stv->sb_imp2_x.set_sensitive(true);
        Stv->sb_imp2_y.set_sensitive(true);
    }
    else {
        gtk_entry_set_text(GTK_ENTRY(Stv->stv_imp2), "");
        Stv->sb_imp2_x.set_value(0);
        Stv->sb_imp2_y.set_value(0);
        gtk_widget_set_sensitive(Stv->stv_imp2, false);
        Stv->sb_imp2_x.set_sensitive(false);
        Stv->sb_imp2_y.set_sensitive(false);
    }
}


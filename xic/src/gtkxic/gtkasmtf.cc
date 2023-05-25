
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
#include "gtkmain.h"
#include "gtkasm.h"


//-----------------------------------------------------------------------------
// The sAsmTf (instance transform entry) class

sAsmTf::sAsmTf(sAsmPage *src)
{
    tf_owner = src;
    GtkWidget *notebook = gtk_notebook_new();
    gtk_widget_show(notebook);
    tf_top = notebook;

    //
    // the "Basic" page
    //
    GtkWidget *label = gtk_label_new("Basic");
    gtk_widget_show(label);
    GtkWidget *form = gtk_table_new(3, 3, false);
    gtk_widget_show(form);
    gtk_notebook_insert_page(GTK_NOTEBOOK(notebook), form, label, 0);

    //
    // placement x, y
    //
    int ndgt = CD()->numDigits();

    tf_pxy_label = gtk_label_new("Placement X,Y");
    gtk_widget_show(tf_pxy_label);
    gtk_label_set_justify(GTK_LABEL(tf_pxy_label), GTK_JUSTIFY_LEFT);

    int row = 0;
    gtk_table_attach(GTK_TABLE(form), tf_pxy_label, 0, 1, row, row+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    GtkWidget *sb = sb_placement_x.init(0.0, -1e6, 1e6, ndgt);
    gtk_widget_set_size_request(sb, ASM_NFWP, -1);

    gtk_table_attach(GTK_TABLE(form), sb, 1, 2, row, row+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    sb = sb_placement_y.init(0.0, -1e6, 1e6, ndgt);
    gtk_widget_set_size_request(sb, ASM_NFWP, -1);

    gtk_table_attach(GTK_TABLE(form), sb, 2, 3, row, row+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    row++;

    //
    // rotation and mirror
    //
    tf_ang_label = gtk_label_new("Rotation Angle");
    gtk_widget_show(tf_ang_label);
    gtk_label_set_justify(GTK_LABEL(tf_ang_label), GTK_JUSTIFY_LEFT);

    gtk_table_attach(GTK_TABLE(form), tf_ang_label, 0, 1, row, row+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    tf_angle = gtk_combo_box_text_new();
    gtk_widget_set_name(tf_angle, "rotation");
    gtk_widget_show(tf_angle);

    tf_angle_ix = 0;
    for (int i = 0; i < 8; i++) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%d", i*45);
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(tf_angle), buf);
    }
    gtk_combo_box_set_active(GTK_COMBO_BOX(tf_angle), tf_angle_ix);
    g_signal_connect(G_OBJECT(tf_angle), "changed",
        G_CALLBACK(tf_angle_proc), this);

    gtk_table_attach(GTK_TABLE(form), tf_angle, 1, 2, row, row+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
        (GtkAttachOptions)(GTK_FILL), 2, 2);

    tf_mirror = gtk_check_button_new_with_label("Mirror-Y");
    gtk_widget_show(tf_mirror);

    gtk_table_attach(GTK_TABLE(form), tf_mirror, 2, 3, row, row+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    row++;

    //
    // magnification
    //
    tf_mag_label = gtk_label_new("Magnification");
    gtk_widget_show(tf_mag_label);
    gtk_label_set_justify(GTK_LABEL(tf_mag_label), GTK_JUSTIFY_LEFT);

    gtk_table_attach(GTK_TABLE(form), tf_mag_label, 0, 1, row, row+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    sb = sb_magnification.init(1.0, CDMAGMIN, CDMAGMAX, ASM_NUMD);
    gtk_widget_set_size_request(sb, ASM_NFWP, -1);

    gtk_table_attach(GTK_TABLE(form), sb, 1, 2, row, row+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    row++;

    //
    // placement name
    //
    tf_name_label = gtk_label_new("Placement Name");
    gtk_widget_show(tf_name_label);
    gtk_label_set_justify(GTK_LABEL(tf_name_label), GTK_JUSTIFY_LEFT);

    gtk_table_attach(GTK_TABLE(form), tf_name_label, 0, 1, row, row+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    tf_name = gtk_entry_new();
    gtk_widget_show(tf_name);

    gtk_table_attach(GTK_TABLE(form), tf_name, 1, 3, row, row+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    row++;

    //
    // the "Advanced" page
    //
    label = gtk_label_new("Advanced");
    gtk_widget_show(label);
    form = gtk_table_new(4, 3, false);
    gtk_widget_show(form);
    gtk_notebook_insert_page(GTK_NOTEBOOK(notebook), form, label, 1);
    row = 0;

    //
    // window, clip, flatten check boxes
    //
    GtkWidget *hbox = gtk_hbox_new(false, 2);
    gtk_widget_show(hbox);
    tf_use_win = gtk_check_button_new_with_label("Use Window");
    gtk_widget_set_name(tf_use_win, "Window");
    gtk_widget_show(tf_use_win);
    g_signal_connect(G_OBJECT(tf_use_win), "clicked",
        G_CALLBACK(tf_use_win_proc), this);
    gtk_box_pack_start(GTK_BOX(hbox), tf_use_win, true, true, 0);

    tf_do_clip = gtk_check_button_new_with_label("Clip");
    gtk_widget_set_name(tf_do_clip, "Clip");
    gtk_widget_show(tf_do_clip);
    gtk_box_pack_start(GTK_BOX(hbox), tf_do_clip, true, true, 0);

    tf_do_flatn = gtk_check_button_new_with_label("Flatten");
    gtk_widget_set_name(tf_do_flatn, "Flatten");
    gtk_widget_show(tf_do_flatn);
    gtk_box_pack_start(GTK_BOX(hbox), tf_do_flatn, true, true, 0);

    tf_ecf_label = gtk_label_new("Empty Cell Filter");
    gtk_widget_show(tf_ecf_label);
    gtk_misc_set_padding(GTK_MISC(tf_ecf_label), 2, 2);
    gtk_box_pack_end(GTK_BOX(hbox), tf_ecf_label, true, true, 0);

    gtk_table_attach(GTK_TABLE(form), hbox, 0, 4, row, row+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    row++;

    //
    // window coordinate entries
    //
    tf_lb_label = gtk_label_new("Left, Bottom");
    gtk_widget_show(tf_lb_label);
    gtk_misc_set_padding(GTK_MISC(tf_lb_label), 2, 2);

    gtk_table_attach(GTK_TABLE(form), tf_lb_label, 0, 1, row, row+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    sb = sb_win_l.init(0.0, -1e6, 1e6, ndgt);
    gtk_widget_set_size_request(sb, ASM_NFWP, -1);

    gtk_table_attach(GTK_TABLE(form), sb, 1, 2, row, row+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    sb = sb_win_b.init(0.0, -1e6, 1e6, ndgt);
    gtk_widget_set_size_request(sb, ASM_NFWP, -1);

    gtk_table_attach(GTK_TABLE(form), sb, 2, 3, row, row+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    tf_ecf_pre = gtk_check_button_new_with_label("pre-filt");
    gtk_widget_set_name(tf_ecf_pre, "pre");
    gtk_widget_show(tf_ecf_pre);

    gtk_table_attach(GTK_TABLE(form), tf_ecf_pre, 3, 4, row, row+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    row++;

    tf_rt_label = gtk_label_new("Right, Top");
    gtk_widget_show(tf_rt_label);
    gtk_misc_set_padding(GTK_MISC(tf_rt_label), 2, 2);

    gtk_table_attach(GTK_TABLE(form), tf_rt_label, 0, 1, row, row+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    sb = sb_win_r.init(0.0, -1e6, 1e6, ndgt);
    gtk_widget_set_size_request(sb, ASM_NFWP, -1);

    gtk_table_attach(GTK_TABLE(form), sb, 1, 2, row, row+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    sb = sb_win_t.init(0.0, -1e6, 1e6, ndgt);
    gtk_widget_set_size_request(sb, ASM_NFWP, -1);

    gtk_table_attach(GTK_TABLE(form), sb, 2, 3, row, row+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    tf_ecf_post = gtk_check_button_new_with_label("post-filt");
    gtk_widget_set_name(tf_ecf_post, "post");
    gtk_widget_show(tf_ecf_post);

    gtk_table_attach(GTK_TABLE(form), tf_ecf_post, 3, 4, row, row+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    row++;

    //
    // scale factor and no hierarchy entries
    //
    tf_sc_label = gtk_label_new("Scale Fct");
    gtk_widget_show(tf_sc_label);
    gtk_misc_set_padding(GTK_MISC(tf_sc_label), 2, 2);

    gtk_table_attach(GTK_TABLE(form), tf_sc_label, 0, 1, row, row+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    sb = sb_scale.init(1.0, CDSCALEMIN, CDSCALEMAX, ASM_NUMD);
    gtk_widget_set_size_request(sb, ASM_NFWP, -1);

    gtk_table_attach(GTK_TABLE(form), sb, 1, 2, row, row+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    tf_no_hier = gtk_check_button_new_with_label("No Hierarchy");
    gtk_widget_set_name(tf_no_hier, "NoHier");
    gtk_widget_show(tf_no_hier);

    gtk_table_attach(GTK_TABLE(form), tf_no_hier, 2, 4, row, row+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
}


sAsmTf::~sAsmTf()
{
}


// Enable or gray the transformation entries.
//
void
sAsmTf::set_sens(bool has_selection, bool has_toplev)
{
    bool sens = has_selection && has_toplev;
    sb_placement_x.set_sensitive(sens);
    sb_placement_y.set_sensitive(sens);
    gtk_widget_set_sensitive(tf_pxy_label, sens);
    gtk_widget_set_sensitive(tf_angle, sens);
    gtk_widget_set_sensitive(tf_ang_label, sens);
    gtk_widget_set_sensitive(tf_mirror, sens);
    sb_magnification.set_sensitive(sens);
    gtk_widget_set_sensitive(tf_mag_label, sens);

    sens = has_selection;
    gtk_widget_set_sensitive(tf_name_label, sens);
    gtk_widget_set_sensitive(tf_name, sens);
    gtk_widget_set_sensitive(tf_use_win, sens);
    gtk_widget_set_sensitive(tf_do_flatn, sens);
    gtk_widget_set_sensitive(tf_ecf_label, sens);
    gtk_widget_set_sensitive(tf_ecf_pre, sens);
    gtk_widget_set_sensitive(tf_ecf_post, sens);
    gtk_widget_set_sensitive(tf_no_hier, sens);
    gtk_widget_set_sensitive(tf_sc_label, sens);
    sb_scale.set_sensitive(sens);
    if (sens)
        sens = GTKdev::GetStatus(tf_use_win);
    gtk_widget_set_sensitive(tf_do_clip, sens);
    sb_win_l.set_sensitive(sens);
    sb_win_b.set_sensitive(sens);
    sb_win_r.set_sensitive(sens);
    sb_win_t.set_sensitive(sens);
    gtk_widget_set_sensitive(tf_lb_label, sens);
    gtk_widget_set_sensitive(tf_rt_label, sens);
}


// Reset the values of the transformation entries to defaults.
//
void
sAsmTf::reset()
{
    sb_placement_x.set_value(0.0);
    sb_placement_y.set_value(0.0);
    gtk_combo_box_set_active(GTK_COMBO_BOX(tf_angle), 0);
    GTKdev::Deselect(tf_mirror);
    sb_magnification.set_value(1.0);
    GTKdev::Deselect(tf_ecf_pre);
    GTKdev::Deselect(tf_ecf_post);
    GTKdev::Deselect(tf_use_win);
    GTKdev::Deselect(tf_do_clip);
    GTKdev::Deselect(tf_do_flatn);
    GTKdev::Deselect(tf_no_hier);
    sb_win_l.set_value(0.0);
    sb_win_b.set_value(0.0);
    sb_win_r.set_value(0.0);
    sb_win_t.set_value(0.0);
    sb_scale.set_value(1.0);
    gtk_entry_set_text(GTK_ENTRY(tf_name), "");
}


void
sAsmTf::get_tx_params(tlinfo *tl)
{
    if (!tl)
        return;
    tl->x = INTERNAL_UNITS(sb_placement_x.get_value());
    tl->y = INTERNAL_UNITS(sb_placement_y.get_value());
    tl->angle = 45*tf_angle_ix;
    tl->magn = sb_magnification.get_value();
    if (tl->magn < CDSCALEMIN || tl->magn > CDSCALEMAX)
        tl->magn = 1.0;
    tl->scale = sb_scale.get_value();
    if (tl->scale < CDSCALEMIN || tl->scale > CDSCALEMAX)
        tl->scale = 1.0;
    tl->mirror_y = GTKdev::GetStatus(tf_mirror);
    tl->ecf_level = ECFnone;
    if (GTKdev::GetStatus(tf_ecf_pre)) {
        if (GTKdev::GetStatus(tf_ecf_post))
            tl->ecf_level = ECFall;
        else
            tl->ecf_level = ECFpre;
    }
    else if (GTKdev::GetStatus(tf_ecf_post))
        tl->ecf_level = ECFpost;
    tl->use_win = GTKdev::GetStatus(tf_use_win);
    tl->clip = GTKdev::GetStatus(tf_do_clip);
    tl->flatten = GTKdev::GetStatus(tf_do_flatn);
    tl->no_hier = GTKdev::GetStatus(tf_no_hier);
    tl->winBB.left = INTERNAL_UNITS(sb_win_l.get_value());
    tl->winBB.bottom = INTERNAL_UNITS(sb_win_b.get_value());
    tl->winBB.right = INTERNAL_UNITS(sb_win_r.get_value());
    tl->winBB.top = INTERNAL_UNITS(sb_win_t.get_value());
    delete [] tl->placename;
    tl->placename = 0;
    const char *s = gtk_entry_get_text(GTK_ENTRY(tf_name));
    if (s) {
        // strip any leading or trailing white space
        while (isspace(*s))
            s++;
        if (*s) {
            tl->placename = lstring::copy(s);
            char *t = tl->placename + strlen(tl->placename) - 1;
            while (t >= tl->placename && isspace(*t))
                *t-- = 0;
        }
    }
}


void
sAsmTf::set_tx_params(tlinfo *tl)
{
    if (!tl)
        return;
    int ndgt = CD()->numDigits();
    sb_placement_x.set_digits(ndgt);
    sb_placement_x.set_value(MICRONS(tl->x));
    sb_placement_y.set_digits(ndgt);
    sb_placement_y.set_value(MICRONS(tl->y));
    tf_angle_ix = tl->angle/45;
    gtk_combo_box_set_active(GTK_COMBO_BOX(tf_angle), tf_angle_ix);
    sb_magnification.set_value(tl->magn);
    sb_scale.set_value(tl->scale);
    GTKdev::SetStatus(tf_mirror, tl->mirror_y);
    GTKdev::SetStatus(tf_ecf_pre, tl->ecf_level == ECFall ||
        tl->ecf_level == ECFpre);
    GTKdev::SetStatus(tf_ecf_post, tl->ecf_level == ECFall ||
        tl->ecf_level == ECFpost);
    GTKdev::SetStatus(tf_use_win, tl->use_win);
    GTKdev::SetStatus(tf_do_clip, tl->clip);
    GTKdev::SetStatus(tf_do_flatn, tl->flatten);
    GTKdev::SetStatus(tf_no_hier, tl->no_hier);
    sb_win_l.set_digits(ndgt);
    sb_win_l.set_value(MICRONS(tl->winBB.left));
    sb_win_b.set_digits(ndgt);
    sb_win_b.set_value(MICRONS(tl->winBB.bottom));
    sb_win_r.set_digits(ndgt);
    sb_win_r.set_value(MICRONS(tl->winBB.right));
    sb_win_t.set_digits(ndgt);
    sb_win_t.set_value(MICRONS(tl->winBB.top));
    gtk_entry_set_text(GTK_ENTRY(tf_name), tl->placename ? tl->placename : "");
}


// Static function.
// Handle angle selection in the instance transform area.
//
void
sAsmTf::tf_angle_proc(GtkWidget *widget, void *mtxp)
{
    sAsmTf *tx = static_cast<sAsmTf*>(mtxp);
    tx->tf_angle_ix = gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
}


// Static function.
// Handler for the Use Window check box.
//
void
sAsmTf::tf_use_win_proc(GtkWidget*, void *mtxp)
{
    sAsmTf *tx = static_cast<sAsmTf*>(mtxp);
    tx->tf_owner->upd_sens();
}



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
#include "errorlog.h"
#include "dsp_inlines.h"
#include "fio.h"
#include "fio_chd.h"
#include "fio_cgd.h"
#include "cd_digest.h"
#include "gtkmain.h"
#include "gtkinlines.h"


//-----------------------------------------------------------------------------
// Pop up to configure a CHD.  The CHD can be configured with
// 1.  a default top cell.
// 2.  an associated geometry database.
//
// When a configured CHD is used for access, it will use only cells in
// the hierarchy under the top cell that are needed to render the area.
// The original file is accessed, unless the in-core geometry database
// is loaded.
//
// Help system keywords used:
//  xic:chdconfig

namespace {
    // Drag/drop stuff.
    //
    GtkTargetEntry target_table[] = {
        { (char*)"TWOSTRING",   0, 0 },
        { (char*)"CELLNAME",    0, 1 },
        { (char*)"STRING",      0, 2 },
        { (char*)"text/plain",  0, 3 }
    };
    guint n_targets = sizeof(target_table) / sizeof(target_table[0]);

    namespace gtkchdcfg {
        struct sCfg : public gtk_bag
        {
            sCfg(GRobject, const char*);
            ~sCfg();

            void update(const char*);

        private:
            void button_hdlr(GtkWidget*);

            static bool cf_new_cgd_cb(const char*, const char*, int, void*);
            static void cf_cancel_proc(GtkWidget*, void*);
            static void cf_action(GtkWidget*, void*);
            static void cf_drag_data_received(GtkWidget*, GdkDragContext*,
                gint, gint, GtkSelectionData*, guint, guint);

            GRobject cf_caller;
            GtkWidget *cf_label;
            GtkWidget *cf_dtc_label;
            GtkWidget *cf_last;
            GtkWidget *cf_text;
            GtkWidget *cf_apply_tc;
            GtkWidget *cf_newcgd;
            GtkWidget *cf_cgdentry;
            GtkWidget *cf_cgdlabel;
            GtkWidget *cf_apply_cgd;

            char *cf_chdname;
            char *cf_lastname;
            char *cf_cgdname;
        };

        sCfg *Cfg;
    }
}

using namespace gtkchdcfg;


void
cConvert::PopUpChdConfig(GRobject caller, ShowMode mode,
    const char *chdname, int x, int y)
{
    if (!GRX || !mainBag())
        return;
    if (mode == MODE_OFF) {
        delete Cfg;
        return;
    }
    if (mode == MODE_UPD) {
        if (Cfg)
            Cfg->update(chdname);
        return;
    }
    if (Cfg)
        return;

    new sCfg(caller, chdname);
    if (!Cfg->Shell()) {
        delete Cfg;
        return;
    }
    gtk_window_set_transient_for(GTK_WINDOW(Cfg->Shell()),
        GTK_WINDOW(mainBag()->Shell()));

    int mwid;
    MonitorGeom(mainBag()->Shell(), 0, 0, &mwid, 0);
    GtkRequisition req;
    gtk_widget_get_requisition(Cfg->Shell(), &req);
    if (x + req.width > mwid)
        x = mwid - req.width;
    gtk_window_move(GTK_WINDOW(Cfg->Shell()), x, y);
    gtk_widget_show(Cfg->Shell());
}


sCfg::sCfg(GRobject caller, const char *chdname)
{
    Cfg = this;
    cf_caller = caller;
    cf_label = 0;
    cf_dtc_label = 0;
    cf_last = 0;
    cf_text = 0;
    cf_apply_tc = 0;
    cf_newcgd = 0;
    cf_cgdentry = 0;
    cf_cgdlabel = 0;
    cf_apply_cgd = 0;
    cf_chdname = 0;
    cf_lastname = 0;
    cf_cgdname = 0;

    wb_shell = gtk_NewPopup(0, "Configure Cell Hierarchy Digest",
        cf_cancel_proc, 0);
    if (!wb_shell)
        return;

    GtkWidget *form = gtk_table_new(1, 1, false);
    gtk_widget_show(form);
    gtk_container_set_border_width(GTK_CONTAINER(form), 2);
    gtk_container_add(GTK_CONTAINER(wb_shell), form);
    int rowcnt = 0;

    // Label in frame plus help button.
    //
    GtkWidget *hbox = gtk_hbox_new(false, 2);
    gtk_widget_show(hbox);
    cf_label = gtk_label_new("");
    gtk_widget_show(cf_label);
    gtk_misc_set_padding(GTK_MISC(cf_label), 2, 2);
    GtkWidget *frame = gtk_frame_new(0);
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), cf_label);
    gtk_box_pack_start(GTK_BOX(hbox), frame, true, true, 0);
    GtkWidget *button = gtk_button_new_with_label("Help");
    gtk_widget_set_name(button, "Help");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(cf_action), 0);
    gtk_box_pack_end(GTK_BOX(hbox), button, false, false, 0);
    gtk_table_attach(GTK_TABLE(form), hbox, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    // Frame and name group.
    //
    GtkWidget *tform = gtk_table_new(2, 1, false);
    gtk_widget_show(tform);
    frame = gtk_frame_new(0);
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), tform);
    gtk_table_attach(GTK_TABLE(form), frame, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;
    int ncnt = 0;

    hbox = gtk_hbox_new(false, 2);
    gtk_widget_show(hbox);
    cf_apply_tc = gtk_button_new_with_label("");
    gtk_widget_set_name(cf_apply_tc, "ApplyTc");
    gtk_widget_show(cf_apply_tc);
    gtk_box_pack_start(GTK_BOX(hbox), cf_apply_tc, false, false, 0);
    g_signal_connect(G_OBJECT(cf_apply_tc), "clicked",
        G_CALLBACK(cf_action), 0);

    GtkWidget *label = gtk_label_new("Set Default Cell");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_box_pack_start(GTK_BOX(hbox), label, true, true, 0);

    gtk_table_attach(GTK_TABLE(tform), hbox, 0, 2, ncnt, ncnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    ncnt++;

    GtkWidget *sep = gtk_hseparator_new();
    gtk_widget_show(sep);
    gtk_table_attach(GTK_TABLE(tform), sep, 0, 2, ncnt, ncnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    ncnt++;

    // Name group controls.
    //
    hbox = gtk_hbox_new(false, 2);
    gtk_widget_show(hbox);
    cf_dtc_label = gtk_label_new("Default top cell");
    gtk_widget_show(cf_dtc_label);
    gtk_box_pack_start(GTK_BOX(hbox), cf_dtc_label, true, true, 0);

    cf_last = gtk_button_new_with_label("Last");
    gtk_widget_set_name(cf_last, "Last");
    gtk_widget_show(cf_last);
    gtk_box_pack_start(GTK_BOX(hbox), cf_last, false, false, 0);
    g_signal_connect(G_OBJECT(cf_last), "clicked",
        G_CALLBACK(cf_action), 0);

    gtk_table_attach(GTK_TABLE(tform), hbox, 0, 1, ncnt, ncnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    cf_text = gtk_entry_new();
    gtk_widget_show(cf_text);
    gtk_editable_set_editable(GTK_EDITABLE(cf_text), true);
    gtk_entry_set_text(GTK_ENTRY(cf_text), "");

    // drop site
    GtkDestDefaults DD = (GtkDestDefaults)
        (GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_HIGHLIGHT);
    gtk_drag_dest_set(cf_text, DD, target_table, n_targets,
        GDK_ACTION_COPY);
    g_signal_connect_after(G_OBJECT(cf_text), "drag-data-received",
        G_CALLBACK(cf_drag_data_received), 0);

    gtk_table_attach(GTK_TABLE(tform), cf_text, 1, 2, ncnt, ncnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 0);
    //
    // End of name group.

    sep = gtk_hseparator_new();
    gtk_widget_show(sep);
    gtk_table_attach(GTK_TABLE(form), sep, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    // Frame and CGD group.
    //
    tform = gtk_table_new(2, 1, false);
    gtk_widget_show(tform);
    frame = gtk_frame_new(0);
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), tform);
    gtk_table_attach(GTK_TABLE(form), frame, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;
    int cgcnt = 0;

    hbox = gtk_hbox_new(false, 2);
    gtk_widget_show(hbox);
    cf_apply_cgd = gtk_button_new_with_label("");
    gtk_widget_set_name(cf_apply_cgd, "ApplyCgd");
    gtk_widget_show(cf_apply_cgd);
    gtk_box_pack_start(GTK_BOX(hbox), cf_apply_cgd, false, false, 0);
    g_signal_connect(G_OBJECT(cf_apply_cgd), "clicked",
        G_CALLBACK(cf_action), 0);

    label = gtk_label_new("Setup Linked Cell Geometry Digest");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_box_pack_start(GTK_BOX(hbox), label, true, true, 0);

    gtk_table_attach(GTK_TABLE(tform), hbox, 0, 2, cgcnt, cgcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    cgcnt++;

    sep = gtk_hseparator_new();
    gtk_widget_show(sep);
    gtk_table_attach(GTK_TABLE(tform), sep, 0, 2, cgcnt, cgcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    cgcnt++;

    cf_newcgd = gtk_check_button_new_with_label("Open new CGD");
    gtk_widget_set_name(cf_newcgd, "NewCGD");
    gtk_widget_show(cf_newcgd);
    g_signal_connect(G_OBJECT(cf_newcgd), "clicked",
        G_CALLBACK(cf_action), 0);
    gtk_table_attach(GTK_TABLE(tform), cf_newcgd, 0, 2, cgcnt, cgcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 0);
    cgcnt++;

    cf_cgdlabel = gtk_label_new("CGD name");
    gtk_widget_show(cf_cgdlabel);
    gtk_misc_set_padding(GTK_MISC(cf_cgdlabel), 2, 2);
    gtk_table_attach(GTK_TABLE(tform), cf_cgdlabel, 0, 1, cgcnt, cgcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    cf_cgdentry = gtk_entry_new();
    gtk_widget_show(cf_cgdentry);
    gtk_table_attach(GTK_TABLE(tform), cf_cgdentry, 1, 2, cgcnt, cgcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    //
    // End of CGD group.

    //
    // Dismiss button
    //
    button = gtk_button_new_with_label("Dismiss");
    gtk_widget_set_name(button, "Dismiss");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(cf_cancel_proc), 0);

    gtk_table_attach(GTK_TABLE(form), button, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);
    gtk_window_set_focus(GTK_WINDOW(wb_shell), button);

    // Constrain overall widget width so title text isn't truncated.
    gtk_widget_set_size_request(wb_shell, 360, -1);

    update(chdname);
}


sCfg::~sCfg()
{
    Cfg = 0;
    delete [] cf_lastname;
    delete [] cf_chdname;
    delete [] cf_cgdname;
    if (cf_caller)
        GRX->Deselect(cf_caller);
    if (wb_shell) {
        g_signal_handlers_disconnect_by_func(G_OBJECT(wb_shell),
            (gpointer)cf_cancel_proc, wb_shell);
    }
}


void
sCfg::update(const char *chdname)
{
    if (!chdname)
        return;
    if (chdname != cf_chdname) {
        delete [] cf_chdname;
        cf_chdname = lstring::copy(chdname);
    }
    cCHD *chd = CDchd()->chdRecall(chdname, false);
    if (chd) {
        gtk_entry_set_text(GTK_ENTRY(cf_text), chd->defaultCell(Physical));
        bool has_name = (chd->getConfigSymref() != 0);

        gtk_widget_set_sensitive(cf_dtc_label, !has_name);
        gtk_widget_set_sensitive(cf_last, !has_name);
        gtk_widget_set_sensitive(cf_text, !has_name);

        const char *cgdname = chd->getCgdName();
        if (cgdname) {
            gtk_entry_set_text(GTK_ENTRY(cf_cgdentry), cgdname);
            gtk_editable_set_editable(GTK_EDITABLE(cf_cgdentry), false);
            GRX->SetStatus(cf_newcgd, false);
        }
        else {
            gtk_entry_set_text(GTK_ENTRY(cf_cgdentry),
                cf_cgdname ? cf_cgdname : "");
            gtk_editable_set_editable(GTK_EDITABLE(cf_cgdentry), true);
        }

        gtk_widget_set_sensitive(cf_newcgd, !chd->hasCgd());
        gtk_widget_set_sensitive(cf_cgdentry, !chd->hasCgd());
        gtk_widget_set_sensitive(cf_cgdlabel, !chd->hasCgd());

        char buf[256];
        if (has_name || chd->hasCgd()) {
            sprintf(buf, "CHD %s is configured with ", chdname);
            int xx = 0;
            if (has_name) {
                strcat(buf, "Cell");
                xx++;
            }
            if (chd->hasCgd()) {
                if (xx)
                    strcat(buf, ", Geometry");
                else
                    strcat(buf, "Geometry");
            }
            strcat(buf, ".");
        }
        else
            sprintf(buf, "CHD %s is not configured.", chdname);
        gtk_label_set_text(GTK_LABEL(cf_label), buf);

        gtk_label_set_text(GTK_LABEL(gtk_bin_get_child(GTK_BIN(cf_apply_tc))),
            has_name ? "Clear" : "Apply");
        gtk_label_set_text(GTK_LABEL(gtk_bin_get_child(GTK_BIN(cf_apply_cgd))),
            chd->hasCgd() ? "Clear" : "Apply");
    }
    gtk_widget_set_sensitive(cf_apply_tc, chd != 0);
    gtk_widget_set_sensitive(cf_apply_cgd, chd != 0);
}


void
sCfg::button_hdlr(GtkWidget *widget)
{
    const char *name = gtk_widget_get_name(widget);
    if (!strcmp(name, "Help")) {
        DSPmainWbag(PopUpHelp("xic:chdconfig"))
        return;
    }
    if (!strcmp(name, "NewCGD"))
        return;

    if (!cf_chdname)
        return;
    cCHD *chd = CDchd()->chdRecall(cf_chdname, false);
    if (!chd) {
        PopUpMessage("Error: can't find named CHD.", true);
        return;
    }

    if (!strcmp(name, "Last")) {
        char *ent = lstring::copy(gtk_entry_get_text(GTK_ENTRY(cf_text)));
        gtk_entry_set_text(GTK_ENTRY(cf_text), cf_lastname ? cf_lastname :
            chd->defaultCell(Physical));
        delete [] cf_lastname;
        cf_lastname = ent;
        return;
    }

    if (widget == cf_apply_tc) {
        // NOTE:  cCHD::setDefaultCellname calls back to the update
        // function, so we don't call it here.  Have to be careful to
        // not revert cf_text before we get the new name.

        bool iscfg = chd->getConfigSymref() != 0;
        if (iscfg) {
            // Save the current configuration cellname, for the Last button.
            delete [] cf_lastname;
            cf_lastname = lstring::copy(gtk_entry_get_text(GTK_ENTRY(cf_text)));
            chd->setDefaultCellname(0, 0);
            update(cf_chdname);
            return;
        }
        const char *ent = gtk_entry_get_text(GTK_ENTRY(cf_text));
        if (ent && *ent) {
            if (!chd->findSymref(ent, Physical)) {
                PopUpMessage("Error: can't find named cell in CHD.", true);
                return;
            }
        }
        const char *dfl = chd->defaultCell(Physical);
        if (ent && dfl && strcmp(ent, dfl)) {
            if (!chd->setDefaultCellname(ent, 0)) {
                Errs()->add_error("Call to configure failed.");
                PopUpMessage(Errs()->get_error(), true);
                return;
            }
        }
    }
    if (widget == cf_apply_cgd) {
        bool iscfg = chd->hasCgd();
        chd->setCgd(0);
        if (iscfg) {
            // clearing only
            update(cf_chdname);
            return;
        }
        const char *str = gtk_entry_get_text(GTK_ENTRY(cf_cgdentry));
        delete [] cf_cgdname;
        cf_cgdname = lstring::copy(str);
        cCGD *cgd = CDcgd()->cgdRecall(cf_cgdname, false);
        if (!cgd) {
            if (GRX->GetStatus(cf_newcgd)) {
                int xo, yo;
                gdk_window_get_root_origin(gtk_widget_get_window(Shell()),
                    &xo, &yo);
                char *cn;
                if (cf_cgdname && *cf_cgdname)
                    cn = lstring::copy(cf_cgdname);
                else
                    cn = CDcgd()->newCgdName();
                // Pop down first, panel used elsewhere.
                Cvt()->PopUpCgdOpen(0, MODE_OFF, 0, 0, 0, 0, 0, 0);
                Cvt()->PopUpCgdOpen(0, MODE_ON, cn, chd->filename(),
                    xo, yo, cf_new_cgd_cb, chd);
                delete [] cn;
            }
            else {
                char buf[256];
                if (!cf_cgdname || !*cf_cgdname)
                    strcpy(buf, "No CGD access name given.");
                else
                    sprintf(buf, "No CGD with access name %s "
                        "currently exists.", cf_cgdname);
                PopUpMessage(buf, false);
            }
        }
        else
            chd->setCgd(cgd);
        update(cf_chdname);
    }
}


// Static function.
// Callback for the Open CGD panel.
//
bool
sCfg::cf_new_cgd_cb(const char *idname, const char *string, int mode,
    void *arg)
{
    if (!idname || !*idname)
        return (false);
    if (!string || !*string)
        return (false);
    CgdType tp = CGDremote;
    if (mode == 0)
        tp = CGDmemory;
    else if (mode == 1)
        tp = CGDfile;
    cCGD *cgd = FIO()->NewCGD(idname, string, tp);
    if (!cgd) {
        const char *fmt = "Failed to create new Geometry Digest:\n%s";
        if (Cfg) {
            const char *s = Errs()->get_error();
            int len = strlen(fmt) + (s ? strlen(s) : 0) + 10;
            char *t = new char[len];
            sprintf(t, fmt, s);
            Cfg->PopUpMessage(t, true);
            delete [] t;
        }
        else
            Log()->ErrorLogV(mh::Processing,
                "Failed to create new Geometry Digest:\n%s",
                Errs()->get_error());
        return (false);
    }
    // Link the new CHD, and set the flag to delete the CGD when
    // unlinked.
    cCHD *chd = (cCHD*)arg;
    if (chd) {
        cgd->set_free_on_unlink(true);
        chd->setCgd(cgd);
        if (Cfg)
            Cfg->update(Cfg->cf_chdname);
    }
    return (true);
}


// Static function.
void
sCfg::cf_cancel_proc(GtkWidget*, void*)
{
    delete Cfg;
}


// Static function.
void
sCfg::cf_action(GtkWidget *caller, void*)
{
    if (Cfg)
        Cfg->button_hdlr(caller);
}


// Private static GTK signal handler.
// Drag data received in editing window, grab it
//
void
sCfg::cf_drag_data_received(GtkWidget *entry, GdkDragContext *context,
    gint, gint, GtkSelectionData *data, guint, guint time)
{
    if (gtk_selection_data_get_length(data) >= 0 &&
            gtk_selection_data_get_format(data) == 8 &&
            gtk_selection_data_get_data(data)) {
        char *src = (char*)gtk_selection_data_get_data(data);
        if (gtk_selection_data_get_target(data) ==
                gdk_atom_intern("TWOSTRING", true)) {
            // Drops from content lists may be in the form
            // "fname_or_chd\ncellname".  Keep the cellname.
            char *t = strchr(src, '\n');
            if (t)
                src = t+1;
        }
        gtk_entry_set_text(GTK_ENTRY(entry), src);
        gtk_drag_finish(context, true, false, time);
        return;
    }
    gtk_drag_finish(context, false, false, time);
}


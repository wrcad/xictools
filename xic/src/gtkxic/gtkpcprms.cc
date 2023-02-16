
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
#include "pcell.h"
#include "pcell_params.h"
#include "dsp_inlines.h"
#include "errorlog.h"
#include "gtkmain.h"
#include "gtkmenu.h"
#include "gtkinlines.h"
#include "gtkinterf/gtkspinbtn.h"
#include "spnumber/spnumber.h"


//-------------------------------------------------------------------------
// Pop-up to edit a perhaps long list of parameters, probably for a
// pcell.
//
// Help system keywords used:
//  xic:pcparams

namespace {
    void start_modal(GtkWidget *w)
    {
        gtkMenu()->SetSensGlobal(false);
        gtkMenu()->SetModal(w);
        dspPkgIf()->SetOverrideBusy(true);
        DSPmainDraw(ShowGhost(ERASE))
    }


    void end_modal()
    {
        gtkMenu()->SetModal(0);
        gtkMenu()->SetSensGlobal(true);
        dspPkgIf()->SetOverrideBusy(false);
        DSPmainDraw(ShowGhost(DISPLAY))
    }

    bool return_flag;

    namespace gtkpcprms {
        struct sbsv
        {
            sbsv(GTKspinBtn *b, sbsv *n)
                {
                    sb = b;
                    next = n;
                }

            ~sbsv()
                {
                    delete sb;
                }

            static void destroy(const sbsv *s)
                {
                    while (s) {
                        const sbsv *sx = s;
                        s = s->next;
                        delete sx;
                    }
                }

        private:
            sbsv *next;
            GTKspinBtn *sb;
        };

        struct sPcp
        {
            sPcp(GRobject, PCellParam*, const char*, pcpMode);
            ~sPcp();

            void update(const char*, PCellParam*);

            GtkWidget *shell() { return (pcp_popup); }

        private:
            void sbsave(GTKspinBtn *b)
                {
                    pcp_sbsave = new sbsv(b, pcp_sbsave);
                }

            GtkWidget *setup_entry(PCellParam*, sLstr&, char**);

            static void pcp_cancel_proc(GtkWidget*, void*);
            static void pcp_bool_proc(GtkWidget*, void*);
            static void pcp_num_proc(GtkWidget*, void*);
            static void pcp_string_proc(GtkWidget*, void*);
            static void pcp_menu_proc(GtkWidget*, void*);
            static void pcp_action_proc(GtkWidget*, void*);

            GRobject pcp_caller;
            GtkWidget *pcp_popup;
            GtkWidget *pcp_label;
            GtkWidget *pcp_swin;
            GtkWidget *pcp_table;

            sbsv *pcp_sbsave;
            PCellParam *pcp_params;
            PCellParam *pcp_params_bak;
            char *pcp_dbname;
            pcpMode pcp_mode;
        };

        sPcp *Pcp;
    }
}

using namespace gtkpcprms;


// mode == pcpPlace:
// The panel is intended for instance placement.  The Apply button
// when pressed will create a new sub-master and set this as the
// placement master.  This is optional, as this is done anyway before
// a placement operation.  However, it may be useful to see the
// modified bounding box before placement.  Non-modal.
//
// mode == pcpPlaceScr
// For use with the Place script function, pretty much identical to
// pcpEdit.
//
// mode == pcpOpen:
// The panel is intended for opening a pcell as the top level cell. 
// Once the user sets the parameters, pressing the Open button will
// create a sub-master and make it the current cell.  Non-modal.
//
// mode == pcpEdit:
// The panel is modal, for use when updating properties.  It is used
// instead of the prompt line string editor.  Pressing Apply pops down
// with return value true, otherwise the return value is false.
//
// The PCellParam array passed is updated when the user alters
// parameters, it can not be freed while the pop-up is active.  A new
// set of parameters can be provided with MODE_UPD.
//
bool
cEdit::PopUpPCellParams(GRobject caller, ShowMode mode, PCellParam *p,
    const char *dbname, pcpMode pmode)
{
    if (!GRX || !mainBag())
        return (false);
    if (mode == MODE_OFF) {
        delete Pcp;
        return (false);
    }
    if (mode == MODE_UPD) {
        if (Pcp)
            Pcp->update(dbname, p);
        return (false);
    }
    if (Pcp || pmode == pcpNone)
        return (false);

    new sPcp(caller, p, dbname, pmode);
    if (!Pcp->shell()) {
        delete Pcp;
        return (false);
    }

    gtk_window_set_transient_for(GTK_WINDOW(Pcp->shell()),
        GTK_WINDOW(mainBag()->Shell()));
    GRX->SetPopupLocation(GRloc(LW_UR), Pcp->shell(), mainBag()->Viewport());
    gtk_widget_show(Pcp->shell());

    if (pmode == pcpEdit || pmode == pcpPlaceScr) {
        return_flag = false;
        start_modal(Pcp->shell());
        GRX->MainLoop();  // wait for user's response
        end_modal();
        if (return_flag) {
            return_flag = false;
            return (true);
        }
    }
    return (false);
}


sPcp::sPcp(GRobject c, PCellParam *prm, const char *dbname, pcpMode mode)
{
    Pcp = this;
    pcp_caller = c;
    pcp_label = 0;
    pcp_swin = 0;
    pcp_table = 0;
    pcp_sbsave = 0;
    pcp_params = 0;
    pcp_params_bak = 0;
    pcp_dbname = 0;
    pcp_mode = mode;

    pcp_popup = gtk_NewPopup(0, "Parameters", pcp_cancel_proc, 0);
    if (!pcp_popup)
        return;

    gtk_widget_set_size_request(pcp_popup, 300, 400);
    GtkWidget *form = gtk_table_new(2, 1, false);
    gtk_widget_show(form);
    gtk_container_set_border_width(GTK_CONTAINER(form), 2);
    gtk_container_add(GTK_CONTAINER(pcp_popup), form);

    //
    // Label in frame plus help btn
    //
    int rowcnt = 0;
    GtkWidget *row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);
    char buf[256];
    char *cname;
    if (PCellDesc::split_dbname(dbname, 0, &cname, 0)) {
        snprintf(buf, 256, "Parameters for %s", cname);
        delete [] cname;
    }
    else {
        snprintf(buf, 256, "Parameters for %s", dbname ? dbname : "cell");
    }
    GtkWidget *label = gtk_label_new(buf);
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    GtkWidget *frame = gtk_frame_new(0);
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), label);
    gtk_box_pack_start(GTK_BOX(row), frame, true, true, 0);
    GtkWidget *button = gtk_button_new_with_label("Help");
    gtk_widget_set_name(button, "Help");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(pcp_action_proc), 0);
    gtk_box_pack_end(GTK_BOX(row), button, false, false, 0);
    gtk_table_attach(GTK_TABLE(form), row, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;
    pcp_label = label;

    pcp_swin = gtk_scrolled_window_new(0, 0);
    gtk_widget_show(pcp_swin);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(pcp_swin),
        GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_set_border_width(GTK_CONTAINER(pcp_swin), 2);

    gtk_table_attach(GTK_TABLE(form), pcp_swin, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);
    rowcnt++;

    // Button row.
    //
    GtkWidget *hbox = gtk_hbox_new(false, 2);
    gtk_widget_show(hbox);
    if (pcp_mode == pcpOpen) {
        button = gtk_button_new_with_label("Open");
        gtk_widget_set_name(button, "Open");
    }
    else {
        button = gtk_button_new_with_label("Apply");
        gtk_widget_set_name(button, "Apply");
    }
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(pcp_action_proc), 0);
    gtk_box_pack_start(GTK_BOX(hbox), button, true, true, 0);

    button = gtk_button_new_with_label("Reset");
    gtk_widget_set_name(button, "Reset");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(pcp_action_proc), 0);
    gtk_box_pack_start(GTK_BOX(hbox), button, true, true, 0);

    button = gtk_button_new_with_label("Dismiss");
    gtk_widget_set_name(button, "Dismiss");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(pcp_cancel_proc), 0);
    gtk_box_pack_start(GTK_BOX(hbox), button, true, true, 0);

    gtk_table_attach(GTK_TABLE(form), hbox, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    update(dbname, prm);
}


sPcp::~sPcp()
{
    Pcp = 0;
    if (pcp_caller)
        GRX->Deselect(pcp_caller);
    if (pcp_popup)
        gtk_widget_destroy(pcp_popup);
    sbsv::destroy(pcp_sbsave);
    PCellParam::destroy(pcp_params_bak);
    if (pcp_mode == pcpEdit || pcp_mode == pcpPlaceScr) {
        if (GRX->LoopLevel() > 1)
            GRX->BreakLoop();
    }
}


void
sPcp::update(const char *dbname, PCellParam *p0)
{
    pcp_params = p0;
    if (pcp_mode == pcpEdit || Pcp->pcp_mode == pcpPlaceScr) {
        PCellParam::destroy(pcp_params_bak);
        pcp_params_bak = PCellParam::dup(p0);
    }
    if (dbname) {
        delete [] pcp_dbname;
        pcp_dbname = lstring::copy(dbname);
        char buf[256];
        char *cname;
        if (PCellDesc::split_dbname(dbname, 0, &cname, 0)) {
            snprintf(buf, 256, "Parameters for %s", cname);
            delete [] cname;
        }
        else {
            snprintf(buf, 256, "Parameters for %s", dbname);
        }
        gtk_label_set_text(GTK_LABEL(pcp_label), buf);
    }

    GtkWidget *table = gtk_table_new(2, 1, false);
    gtk_widget_show(table);
    gtk_container_set_border_width(GTK_CONTAINER(table), 2);

    if (pcp_table)
        gtk_widget_destroy(pcp_table);

    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(pcp_swin),
        table);
    pcp_table = table;

    int rcnt = 0;
    sLstr lstr;
    for (PCellParam *p = pcp_params; p; p = p->next()) {
        GtkWidget *label = gtk_label_new(p->name());
        gtk_widget_show(label);
        gtk_misc_set_padding(GTK_MISC(label), 2, 2);

        gtk_table_attach(GTK_TABLE(table), label, 0, 1, rcnt, rcnt+1,
            (GtkAttachOptions)0,
            (GtkAttachOptions)0, 2, 2);

        char *ltext;
        GtkWidget *entry = setup_entry(p, lstr, &ltext);
        gtk_widget_show(entry);
        gtk_widget_set_size_request(entry, 120, -1);

        gtk_table_attach(GTK_TABLE(table), entry, 1, 2, rcnt, rcnt+1,
        (GtkAttachOptions)0,
        (GtkAttachOptions)0, 2, 2);

        if (ltext) {
            label = gtk_label_new(ltext);
            gtk_widget_show(label);
            gtk_misc_set_padding(GTK_MISC(label), 2, 2);

            gtk_table_attach(GTK_TABLE(table), label, 2, 3, rcnt, rcnt+1,
            (GtkAttachOptions)0,
            (GtkAttachOptions)0, 2, 2);
            delete [] ltext;
        }
        rcnt++;
    }
    if (lstr.string())
        Log()->WarningLog(mh::PCells, lstr.string());
}


namespace {
    int numdgts(const PConstraint *pc)
    {
        int ndgt = 0;
        if (pc->resol_none() || pc->resol() < 1.0)
            ndgt = CD()->numDigits();
        else if (pc->resol() > 1e5)
            ndgt = 6;
        else if (pc->resol() > 1e4)
            ndgt = 5;
        else if (pc->resol() > 1e3)
            ndgt = 4;
        else if (pc->resol() > 1e2)
            ndgt = 3;
        else if (pc->resol() > 1e1)
            ndgt = 2;
        else if (pc->resol() > 1e0)
            ndgt = 1;
        return (ndgt);
    }

    double *numparse(const char *s)
    {
        while (isspace(*s) || *s == '\'' || *s == '"')
            s++;
        return (SPnum.parse(&s, false));
    }
}


GtkWidget *
sPcp::setup_entry(PCellParam *p, sLstr &errlstr, char **ltext)
{
    if (ltext)
        *ltext = 0;
    if (!p)
        return (0);
    if (p->type() == PCPappType)
        return (0);

    // Booleans are always a check box, any constraint string is
    // ignored.
    if (p->type() == PCPbool) {
        GtkWidget *w = gtk_check_button_new();
        gtk_widget_show(w);
        GRX->SetStatus(w, p->boolVal());
        g_signal_connect(G_OBJECT(w), "clicked",
            G_CALLBACK(pcp_bool_proc), p);
        return (w);
    }

    // The constraint will set the control style, so we will need to
    // coerce things to the proper type.
    const PConstraint *pc = p->constraint();
    if (pc) {
        if (pc->type() == PCchoice) {
            // An option menu.  The entries should make sense for the
            // parameter type.

            stringlist *slc = pc->choices();
            GtkWidget *w = gtk_combo_box_text_new();
            gtk_widget_show(w);
            int hstv = -1, i = 0;
            for (stringlist *sl = slc; sl; sl = sl->next) {
                gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(w),
                    sl->string);
                char buf[64];
                if (p->type() == PCPint) {
                    sprintf(buf, "%ld", p->intVal());
                    if (!strcmp(buf, sl->string))
                        hstv = i;
                }
                else if (p->type() == PCPtime) {
                    sprintf(buf, "%ld", (long)p->timeVal());
                    if (!strcmp(buf, sl->string))
                        hstv = i;
                }
                else if (p->type() == PCPfloat) {
                    double *d = numparse(sl->string);
                    if (d) {
                        float f1 = *d;
                        float f2 = p->floatVal();
                        if (f1 == f2)
                            hstv = i;
                        else if (fabs(f1 - f2) < 1e-9*(fabs(f1) + fabs(f2)))
                            hstv = i;
                    }
                }
                else if (p->type() == PCPdouble) {
                    double *d = numparse(sl->string);
                    if (d) {
                        double d1 = *d;
                        double d2 = p->doubleVal();
                        if (d1 == d2)
                            hstv = i;
                        else if (fabs(d1 - d2) < 1e-9*(fabs(d1) + fabs(d2)))
                            hstv = i;
                    }
                }
                else if (p->type() == PCPstring) {
                    if (p->stringVal() && !strcmp(p->stringVal(), sl->string))
                        hstv = i;
                }
                i++;
            }
            g_signal_connect(G_OBJECT(w), "changed",
                G_CALLBACK(pcp_menu_proc), p);
            if (hstv >= 0)
                gtk_combo_box_set_active(GTK_COMBO_BOX(w), hstv);
            else {
                gtk_widget_set_sensitive(w, false);
                errlstr.add("Parameter ");
                errlstr.add(p->name());
                errlstr.add(": default value not found in choice list.\n");
            }
            return (w);
        }
        if (pc->type() == PCrange) {
            // A spin button.  Will use .4f format for floating point
            // numbers, need a way to specify print format.

            GTKspinBtn *sb = new GTKspinBtn;
            sbsave(sb);

            int ndgt = 0;
            double minv = -1e30;
            double maxv = 1e30;
            if (!pc->low_none())
                minv = pc->low();
            if (!pc->high_none())
                maxv = pc->high();
            if (p->type() == PCPfloat || p->type() == PCPdouble ||
                    p->type() == PCPstring) {
                ndgt = numdgts(pc);
            }

            GtkWidget *w = 0;
            if (p->type() == PCPint)
                w = sb->init(p->intVal(), minv, maxv, 0);
            else if (p->type() == PCPtime)
                w = sb->init(p->timeVal(), minv, maxv, 0);
            else if (p->type() == PCPfloat)
                w = sb->init(p->floatVal(), minv, maxv, ndgt);
            else if (p->type() == PCPdouble)
                w = sb->init(p->doubleVal(), minv, maxv, ndgt);
            else if (p->type() == PCPstring) {
                double *d = numparse(p->stringVal());
                if (d)
                    w = sb->init(*d, minv, maxv, ndgt);
                else {
                    w = sb->init(0.0, minv, maxv, ndgt);
                    gtk_widget_set_sensitive(w, false);
                    errlstr.add("Parameter ");
                    errlstr.add(p->name());
                    errlstr.add(": default value is non-numeric, with range "
                        "constraint.\n");
                }
            }
            else
                return (0);
            gtk_widget_show(w);
            g_object_set_data(G_OBJECT(w), "sb", sb);
            sb->connect_changed(G_CALLBACK(pcp_num_proc), p, 0);

            if (!pc->checkConstraint(p)) {
                gtk_widget_set_sensitive(w, false);
                errlstr.add("Parameter ");
                errlstr.add(p->name());
                errlstr.add(": default value out of range.\n");
            }
            return (w);
        }
        if (pc->type() == PCstep) {
            // As for range, but with a step increment.

            GTKspinBtn *sb = new GTKspinBtn;
            sbsave(sb);

            int ndgt = 0;
            double minv = -1e30;
            double maxv = 1e30;
            double del = 0.0;
            if (!pc->start_none())
                minv = pc->start();
            else
                minv = 0.0;
            if (!pc->step_none())
                del = pc->step();
            if (!pc->limit_none())
                maxv = pc->limit();
            if (p->type() == PCPfloat || p->type() == PCPdouble ||
                    p->type() == PCPstring) {
                ndgt = numdgts(pc);
            }

            GtkWidget *w = 0;
            if (p->type() == PCPint)
                w = sb->init(p->intVal(), minv, maxv, 0);
            else if (p->type() == PCPtime)
                w = sb->init(p->timeVal(), minv, maxv, 0);
            else if (p->type() == PCPfloat)
                w = sb->init(p->floatVal(), minv, maxv, ndgt);
            else if (p->type() == PCPdouble)
                w = sb->init(p->doubleVal(), minv, maxv, ndgt);
            else if (p->type() == PCPstring) {
                double *d = numparse(p->stringVal());
                if (d)
                    w = sb->init(*d, minv, maxv, ndgt);
                else {
                    w = sb->init(0.0, minv, maxv, ndgt);
                    gtk_widget_set_sensitive(w, false);
                    errlstr.add("Parameter ");
                    errlstr.add(p->name());
                    errlstr.add(": default value is non-numeric, with step "
                        "constraint.\n");
                }
            }
            else
                return (0);
            if (del > 0.0) {
                sb->set_delta(del);
                sb->set_snap(true);
            }
            gtk_widget_show(w);
            g_object_set_data(G_OBJECT(w), "sb", sb);
            sb->connect_changed(G_CALLBACK(pcp_num_proc), p, 0);

            if (!pc->checkConstraint(p)) {
                gtk_widget_set_sensitive(w, false);
                errlstr.add("Parameter ");
                errlstr.add(p->name());
                errlstr.add(": default value not allowed by step constraint.\n");
            }
            return (w);
        }
        if (pc->type() == PCnumStep) {
            // As for step, but allow SPICE-type numbers with scale suffixes.
            // This is not really implemented, need to see what Ciranova
            // does with this.

            GTKspinBtn *sb = new GTKspinBtn;
            sbsave(sb);

            int ndgt = 0;
            double minv = -1e30;
            double maxv = 1e30;
            double del = 0.0;
            if (!pc->start_none())
                minv = pc->start();
            else
                minv = 0.0;
            if (!pc->step_none())
                del = pc->step();
            if (!pc->limit_none())
                maxv = pc->limit();
            if (p->type() == PCPfloat || p->type() == PCPdouble ||
                    p->type() == PCPstring) {
                ndgt = numdgts(pc);
            }
            double f = pc->scale_factor();
            if (f != 0.0) {
                minv /= f;
                maxv /= f;
                del /= f;
            }

            GtkWidget *w = 0;
            if (p->type() == PCPint)
                w = sb->init(p->intVal(), minv, maxv, 0);
            else if (p->type() == PCPtime)
                w = sb->init(p->timeVal(), minv, maxv, 0);
            else if (p->type() == PCPfloat)
                w = sb->init(p->floatVal(), minv, maxv, ndgt);
            else if (p->type() == PCPdouble)
                w = sb->init(p->doubleVal(), minv, maxv, ndgt);
            else if (p->type() == PCPstring) {
                double *d = numparse(p->stringVal());
                if (d) {
                    double dd = *d;
                    if (f != 0.0)
                        dd /= f;
                    w = sb->init(dd, minv, maxv, ndgt);
                }
                else {
                    w = sb->init(0.0, minv, maxv, ndgt);
                    gtk_widget_set_sensitive(w, false);
                    errlstr.add("Parameter ");
                    errlstr.add(p->name());
                    errlstr.add(": default value is non-numeric, with step "
                        "constraint.\n");
                }
            }
            else
                return (0);
            if (del > 0.0) {
                sb->set_delta(del);
                sb->set_snap(true);
            }
            gtk_widget_show(w);
            g_object_set_data(G_OBJECT(w), "sb", sb);
            sb->connect_changed(G_CALLBACK(pcp_num_proc), p, 0);

            if (!pc->checkConstraint(p)) {
                gtk_widget_set_sensitive(w, false);
                errlstr.add("Parameter ");
                errlstr.add(p->name());
                errlstr.add(": default value not allowed by numstep constraint.\n");
            }

            if (pc->scale_factor() != 0.0) {
                char buf[64];
                sprintf(buf, "scale: %s", pc->scale());
                *ltext = lstring::copy(buf);
            }
            return (w);
        }
    }

    // No constraint string, use spin button for numerical
    // entries, a plain entry area for strings.

    if (p->type() == PCPint) {
        GTKspinBtn *sb = new GTKspinBtn;
        sbsave(sb);
        double minv = -1e30;
        double maxv = 1e30;
        GtkWidget *w = sb->init(p->intVal(), minv, maxv, 0);
        gtk_widget_show(w);
        g_object_set_data(G_OBJECT(w), "sb", sb);
        sb->connect_changed(G_CALLBACK(pcp_num_proc), p, 0);
        return (w);
    }
    else if (p->type() == PCPtime) {
        GTKspinBtn *sb = new GTKspinBtn;
        sbsave(sb);
        double minv = -1e30;
        double maxv = 1e30;
        GtkWidget *w = sb->init(p->timeVal(), minv, maxv, 0);
        gtk_widget_show(w);
        g_object_set_data(G_OBJECT(w), "sb", sb);
        sb->connect_changed(G_CALLBACK(pcp_num_proc), p, 0);
        return (w);
    }
    else if (p->type() == PCPfloat) {
        GTKspinBtn *sb = new GTKspinBtn;
        sbsave(sb);
        double minv = -1e30;
        double maxv = 1e30;
        GtkWidget *w = sb->init(p->floatVal(), minv, maxv, 4);
        gtk_widget_show(w);
        g_object_set_data(G_OBJECT(w), "sb", sb);
        sb->connect_changed(G_CALLBACK(pcp_num_proc), p, 0);
        return (w);
    }
    else if (p->type() == PCPdouble) {
        GTKspinBtn *sb = new GTKspinBtn;
        sbsave(sb);
        double minv = -1e30;
        double maxv = 1e30;
        GtkWidget *w = sb->init(p->doubleVal(), minv, maxv, 4);
        gtk_widget_show(w);
        g_object_set_data(G_OBJECT(w), "sb", sb);
        sb->connect_changed(G_CALLBACK(pcp_num_proc), p, 0);
        return (w);
    }
    else if (p->type() == PCPstring) {
        GtkWidget *w = gtk_entry_new();
        gtk_widget_show(w);
        gtk_entry_set_text(GTK_ENTRY(w), p->stringVal());
        g_signal_connect(G_OBJECT(w), "changed",
            G_CALLBACK(pcp_string_proc), p);
        return (w);
    }
    return (0);
}


// Static function.
void
sPcp::pcp_cancel_proc(GtkWidget*, void*)
{
    ED()->PopUpPCellParams(0, MODE_OFF, 0, 0, pcpNone);
}


// Static function.
// Boolean value check box handler.
//
void
sPcp::pcp_bool_proc(GtkWidget *w, void *arg)
{
    PCellParam *p = (PCellParam*)arg;
    bool state = GRX->GetStatus(w);
    if (state != p->boolVal())
        p->setBoolVal(state);
}


// Static function.
// Spin button handler.
//
void
sPcp::pcp_num_proc(GtkWidget *w, void *arg)
{
    PCellParam *p = (PCellParam*)arg;
    GTKspinBtn *sb = (GTKspinBtn*)g_object_get_data(G_OBJECT(w), "sb");
    if (!sb)
        return;
    if (p->type() == PCPint) {
        int i = sb->get_value_as_int();
        if (i != p->intVal())
            p->setIntVal(i);
    }
    else if (p->type() == PCPtime) {
        time_t t = (time_t)sb->get_value();
        if (t != p->timeVal())
            p->setTimeVal(t);
    }
    else if (p->type() == PCPfloat) {
        float f = (float)sb->get_value();
        if (f != p->floatVal())
            p->setFloatVal(f);
    }
    else if (p->type() == PCPdouble) {
        double d = sb->get_value();
        if (d != p->doubleVal())
            p->setDoubleVal(d);
    }
    else if (p->type() == PCPstring) {

        // Two things here.  First, for the numberStep constraint.  we
        // have to put the scale factor back, following the number. 
        // Second, in any case, if the existing string is single or
        // double quoted, apply the same quoting to the update.  This
        // is necessary for Python and TCL.

        const char *sfx = 0;
        const PConstraint *pc = p->constraint();
        if (pc && pc->type() == PCnumStep)
            sfx = pc->scale_factor() != 0.0 ? pc->scale() : 0;
        sLstr lstr;
        const char *s2 = p->stringVal();
        if (s2 && (*s2 == '\'' || *s2 == '"'))
            lstr.add_c(*s2);
        lstr.add(sb->get_string());
        lstr.add(sfx);
        if (s2 && (*s2 == '\'' || *s2 == '"'))
            lstr.add_c(*s2);

        const char *s1 = lstr.string();
        if (s1 && (!s2 || strcmp(s2, s1)))
            p->setStringVal(s1);
    }
}


// Static function.
void
sPcp::pcp_string_proc(GtkWidget *w, void *arg)
{
    PCellParam *p = (PCellParam*)arg;
    const char *s = gtk_entry_get_text(GTK_ENTRY(w));
    while (isspace(*s))
        s++;
    char *t = lstring::copy(s);
    char *e = t + strlen(t) - 1;
    while (e >= t && isspace(*e))
        *e-- = 0;

    if (p->type() == PCPint) {
        int i;
        if (sscanf(t, "%d", &i) == 1 && i != p->intVal())
            p->setIntVal(i);
    }
    else if (p->type() == PCPtime) {
        long i;
        if (sscanf(t, "%ld", &i) == 1 && i != p->timeVal())
            p->setTimeVal(i);
    }
    else if (p->type() == PCPfloat) {
        double *d = numparse(t);
        if (d) {
            float f = *d;
            if (f != p->floatVal())
                p->setFloatVal(f);
        }
    }
    else if (p->type() == PCPdouble) {
        double *d = numparse(t);
        if (d) {
            if (*d != p->doubleVal())
                p->setDoubleVal(*d);
        }
    }
    else if (p->type() == PCPstring) {
        if (!p->stringVal() || strcmp(t, p->stringVal()))
            p->setStringVal(t);
    }
    delete [] t;
}


// Static function.
void
sPcp::pcp_menu_proc(GtkWidget *w, void *arg)
{
    PCellParam *p = (PCellParam*)arg;
    char *t = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(w));
    char *s = t;
    while (isspace(*s))
        s++;
    char *e = s + strlen(s) - 1;
    while (e >= s && isspace(*e))
        *e-- = 0;
    char *tt = lstring::copy(s);
    g_free(t);
    t = tt;

    if (p->type() == PCPint) {
        int i;
        if (sscanf(t, "%d", &i) == 1 && i != p->intVal())
            p->setIntVal(i);
    }
    else if (p->type() == PCPtime) {
        long i;
        if (sscanf(t, "%ld", &i) == 1 && i != p->timeVal())
            p->setTimeVal(i);
    }
    else if (p->type() == PCPfloat) {
        double *d = numparse(t);
        if (d) {
            float f = *d;
            if (f != p->floatVal())
                p->setFloatVal(f);
        }
    }
    else if (p->type() == PCPdouble) {
        double *d = numparse(t);
        if (d) {
            if (*d != p->doubleVal())
                p->setDoubleVal(*d);
        }
    }
    else if (p->type() == PCPstring) {
        if (!p->stringVal() || strcmp(t, p->stringVal()))
            p->setStringVal(t);
    }
    delete [] t;
}


// Static function.
void
sPcp::pcp_action_proc(GtkWidget *caller, void*)
{
    if (!Pcp)
        return;
    const char *name = gtk_widget_get_name(caller);
    if (!name)
        return;
    if (!strcmp(name, "Help")) {
        DSPmainWbag(PopUpHelp("xic:pcparams"))
    }
    else if (!strcmp(name, "Apply")) {
        if (Pcp->pcp_mode == pcpEdit || Pcp->pcp_mode == pcpPlaceScr) {
            return_flag = true;
            ED()->PopUpPCellParams(0, MODE_OFF, 0, 0, pcpNone);
        }
        else
            ED()->plUpdateSubMaster();
    }
    else if (!strcmp(name, "Open")) {
        // pcpOpen only
        CDcbin cbin;
        if (!ED()->resolvePCell(&cbin, Pcp->pcp_dbname, true)) {
            Log()->ErrorLogV(mh::PCells,
                "Open failed.\n%s", Errs()->get_error());
        }
        ED()->stopPlacement();

        // Of the open succeeds and the pcell is not native, open the
        // sub-master that we just created or referenced.
        if (DSP()->CurMode() == Physical && cbin.phys()) {
            if (cbin.phys()->pcType() == CDpcOA) {
                EditType et = XM()->EditCell(Tstring(cbin.cellname()),
                    true);
                if (et != EditOK) {
                    Log()->ErrorLogV(mh::PCells,
                        "Open failed.\n%s", Errs()->get_error());
                }
            }
        }
    }
    else if (!strcmp(name, "Reset")) {
        if (Pcp->pcp_mode == pcpEdit || Pcp->pcp_mode == pcpPlaceScr) {
            // revert using local parameters.
            Pcp->pcp_params->reset(Pcp->pcp_params_bak);
            Pcp->update(0, Pcp->pcp_params);
        }
        else {
            // Revert to the super-master defaults.
            if (!ED()->resetPlacement(Pcp->pcp_dbname)) {
                Log()->ErrorLogV(mh::PCells, "Parameter reset failed:\n%s",
                    Errs()->get_error());
            }
        }
    }
}


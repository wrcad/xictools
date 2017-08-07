
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
 * GtkInterf Graphical Interface Library                                  *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#include "config.h"
#include "gtkinterf.h"
#include "gtkhcopy.h"
#include "miscutil/lstring.h"
#include "miscutil/filestat.h"
#include "miscutil/childproc.h"
#ifdef WIN32
#include "miscutil/msw.h"
#include "mswdraw.h"
#include "mswpdev.h"
#include <algorithm>
using namespace mswinterf;
#endif

#include <signal.h>
#include <sys/types.h>
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

#include <gdk/gdkkeysyms.h>


/************************************************************************
 *
 * Popup form to control a printer for hard copy graphics and text.
 *
 ************************************************************************/

// Help keywords used in this file:
// hcopypanel

namespace gtkinterf {
    struct sMedia
    {
        const char *name;
        int width, height;
    };
}

// list of standard paper sizes, units are points (1/72 inch)
//
struct sMedia pagesizes[] = {
    { "Letter",       612,  792 },
    { "Legal",        612,  1008 },
    { "Tabloid",      792,  1224 },
    { "Ledger",       1224, 792 },
    { "10x14",        720,  1008 },
    { "11x17 \"B\"",  792,  1224 },
    { "12x18",        864,  1296 },
    { "17x22 \"C\"",  1224, 1584 },
    { "18x24",        1296, 1728 },
    { "22x34 \"D\"",  1584, 2448 },
    { "24x36",        1728, 2592 },
    { "30x42",        2160, 3024 },
    { "34x44 \"E\"",  2448, 3168 },
    { "36x48",        2592, 3456 },
    { "Statement",    396,  612 },
    { "Executive",    540,  720 },
    { "Folio",        612,  936 },
    { "Quarto",       610,  780 },
    { "A0",           2384, 3370 },
    { "A1",           1684, 2384 },
    { "A2",           1190, 1684 },
    { "A3",           842,  1190 },
    { "A4",           595,  842 },
    { "A5",           420,  595 },
    { "A6",           298,  420 },
    { "B0",           2835, 4008 },
    { "B1",           2004, 2835 },
    { "B2",           1417, 2004 },
    { "B3",           1001, 1417 },
    { "B4",           729,  1032 },
    { "B5",           516,  729 },
    { NULL,           0,    0 }
};

#define MMPI 25.4
#define MM(x) (hc->hc_metric ? (x)*MMPI : (x))

namespace gtkinterf {
    // For certain graphics drivers, the go button initiates a popup which
    // includes an abort button and status label.
    //
    struct sGP
    {
        sGP() { popup = label = 0; }

        GtkWidget *popup, *label;
    };
}

namespace {  sGP *GP; }


// Write a status/error message on the go button popup, called from
// the graphics drivers.
//
void
GTKdev::HCmessage(const char *str)
{
    if (GP) {
        if (GRpkgIf()->CheckForEvents()) {
            GRpkgIf()->HCabort("User aborted");
            str = "ABORTED";
        }
        gtk_label_set_text(GTK_LABEL(GP->label), str);
    }
}
// End of GTKdev functions.


namespace gtkinterf {
    // Class to maintain a list of active windows for asynchronous
    // messages from the printer.  The process forks to initiate a
    // print job, and prints a message when complete.  The list here
    // keeps track of the initiating window and job pid, so that the
    // message can be dispatched to the correct window.

    struct MsgList
    {
        struct Msg
        {
            friend struct MsgList;

            Msg(gtk_bag *wb, pid_t p, Msg *n)
                { next = n; w = wb; pid = p; msg_list = 0; err = false; }
            ~Msg() { stringlist::destroy(msg_list); }

            void show()
                {
                    if (w && msg_list) {
                        char *msg = stringlist::flatten(msg_list, "\n");
                        GTKprintPopup::hc_pop_up_text(w, msg, err);
                        delete [] msg;
                    }
                }

            void add_msg(const char *msg)
                {
                    stringlist *snew = new stringlist(lstring::copy(msg), 0);
                    if (!msg_list)
                        msg_list = snew;
                    else {
                        stringlist *se = msg_list;
                        while (se->next)
                            se = se->next;
                        se->next = snew;
                    }
                }

            void set_error(bool e)
                {
                    if (e)
                        err = true;
                }

        private:
            Msg *next;
            gtk_bag *w;
            pid_t pid;
            stringlist *msg_list;
            bool err;
        };

        static Msg *List;

        void add(gtk_bag*, pid_t);
        Msg *find(pid_t);
        void remove(gtk_bag*);
        Msg *remove(pid_t);
    };
}

MsgList::Msg *MsgList::List;

namespace { MsgList Mlist; }


// Add a job for this gtk_bag and pid.
//
void
MsgList::add(gtk_bag *w, pid_t pid)
{
    List = new Msg(w, pid, List);
}


// Return the Msg for pid.
//
MsgList::Msg *
MsgList::find(pid_t pid)
{
    for (Msg *s = List; s; s = s->next) {
        if (s->pid == pid)
            return (s);
    }
    return (0);
}


// Remove from the list those elements for w, which is being destroyed.
//
void
MsgList::remove(gtk_bag *w)
{
    Msg *sp = 0, *sn;
    for (Msg *s = List; s; s = sn) {
        sn = s->next;
        if (s->w == w) {
            if (!sp)
                List = sn;
            else
                sp->next = sn;
            delete s;
            continue;
        }
        sp = s;
    }
}


// Unlink and return the element for pid.
//
MsgList::Msg *
MsgList::remove(pid_t pid)
{
    Msg *sp = 0;
    for (Msg *s = List; s; s = s->next) {
        if (s->pid == pid) {
            if (!sp)
                List = s->next;
            else
                sp->next = s->next;
            return (s);
        }
        sp = s;
    }
    return (0);
}
// End of MsgList functions


// Entry for graphics hardcopy support.  Caller is the initiating button,
// if any.
//
void
gtk_bag::PopUpPrint(GRobject caller, HCcb *cb, HCmode mode, GRdraw *context)
{
    GTKprintPopup::hc_hcpopup(caller, this, cb, mode, context);
}


// Function to query values for command text, resolution, etc.
// The sHCcb struct is filled in with the present values.
// If caller is given, it replaces the internal value, allowing the
// popup to be attached to a new button.
//
void
gtk_bag::HCupdate(HCcb *cb, GRobject caller)
{
    if (!wb_hc)
        return;
    if (caller)
        wb_hc->hc_caller = caller;
    if (!cb)
        return;
    cb->hcsetup = wb_hc->hc_cb ? wb_hc->hc_cb->hcsetup : 0;
    cb->hcgo = wb_hc->hc_cb ? wb_hc->hc_cb->hcgo : 0;
    cb->hcframe = wb_hc->hc_cb ? wb_hc->hc_cb->hcframe : 0;

    if (wb_hc->hc_tofile) {
        if (wb_hc->hc_cmdtxtbox)
            cb->tofilename =
                gtk_entry_get_text(GTK_ENTRY(wb_hc->hc_cmdtxtbox));
        else
            cb->tofilename = wb_hc->hc_tofilename;
        cb->command = wb_hc->hc_cmdtext;
    }
    else {
#ifdef WIN32
        cb->command = wb_hc->hc_printers[wb_hc->curprinter];
#else
        if (wb_hc->hc_cmdtxtbox)
            cb->command = gtk_entry_get_text(GTK_ENTRY(wb_hc->hc_cmdtxtbox));
        else
            cb->command = wb_hc->hc_cmdtext;
#endif
        cb->tofilename = wb_hc->hc_tofilename;
    }

    cb->resolution = wb_hc->hc_resol;
    cb->format = wb_hc->hc_fmt;
    cb->drvrmask = wb_hc->hc_drvrmask;
    cb->legend = wb_hc->hc_legend;
    cb->orient = wb_hc->hc_orient;
    cb->tofile = wb_hc->hc_tofile;

    double d;
    if (wb_hc->hc_left)
        d = gtk_spin_button_get_value_as_float(GTK_SPIN_BUTTON(wb_hc->hc_left));
    else
        d = wb_hc->hc_lft_val;
    if (wb_hc->hc_metric)
        d /= MMPI;
    cb->left = d;

    if (wb_hc->hc_top)
        d = gtk_spin_button_get_value_as_float(GTK_SPIN_BUTTON(wb_hc->hc_top));
    else
        d = wb_hc->hc_top_val;
    if (wb_hc->hc_metric)
        d /= MMPI;
    cb->top = d;

    if (wb_hc->hc_wid)
        d = gtk_spin_button_get_value_as_float(GTK_SPIN_BUTTON(wb_hc->hc_wid));
    else
        d = wb_hc->hc_wid_val;
    if (wb_hc->hc_metric)
        d /= MMPI;
    cb->width = d;

    if (wb_hc->hc_hei)
        d = gtk_spin_button_get_value_as_float(GTK_SPIN_BUTTON(wb_hc->hc_hei));
    else
        d = wb_hc->hc_hei_val;
    if (wb_hc->hc_metric)
        d /= MMPI;
    cb->height = d;
}


// Change the format selection of the Print panel.
//
void
gtk_bag::HCsetFormat(int fmt)
{
    GTKprintPopup::hc_set_format(this, fmt, true);
}


// Remove from the message dispatch list.  Messages to this window
// will not be shown.
//
void
gtk_bag::HcopyDisableMsgs()
{
    Mlist.remove(this);
}
// End of gtk_bag functions.


#ifdef WIN32
int GTKprintPopup::curprinter;
#endif

GTKprintPopup::GTKprintPopup()
{
    hc_caller = 0;
    hc_popup = 0;
    destroy_widgets();

    hc_cb = 0;
    hc_context = 0;
    hc_cmdtext = 0;
    hc_tofilename = 0;
    hc_clobber_str = 0;
    hc_resol = 0;
    hc_fmt = 0;
    hc_textmode = HCgraphical;
    hc_textfmt = PStimes;
    hc_legend = HClegOff;
    hc_orient = HCportrait;
    hc_tofile = false;
    hc_tofbak = 0;
    hc_active = false;
    hc_metric = false;
    hc_drvrmask = 0;
    hc_pgsindex = 0;
    hc_wid_val = 0.0;
    hc_hei_val = 0.0;
    hc_lft_val = 0.0;
    hc_top_val = 0.0;

#ifdef WIN32
    hc_numprinters = msw::ListPrinters(&curprinter, &hc_printers);
    if (curprinter >= hc_numprinters)
        curprinter = 0;
#endif
}


GTKprintPopup::~GTKprintPopup()
{
    delete [] hc_cmdtext;
    delete [] hc_tofilename;
    delete [] hc_clobber_str;
#ifdef WIN32
    if (hc_printers) {
        for (int i = 0; i < hc_numprinters; i++)
            delete [] hc_printers[i];
        delete [] hc_printers;
    }
#endif
}


void
GTKprintPopup::destroy_widgets()
{
    if (hc_popup) {
        // avoid second hc_cancel disconnect
        GtkWidget *w = hc_popup;
        hc_popup = 0;
        gtk_widget_destroy(w);
    }
    hc_popup = 0;
    hc_cmdlab = 0;
    hc_cmdtxtbox = 0;
#ifdef WIN32
    hc_prntmenu = 0;
#endif
    hc_wlabel = 0;
    hc_wid = 0;
    hc_hlabel = 0;
    hc_hei = 0;
    hc_xlabel = 0;
    hc_left = 0;
    hc_ylabel = 0;
    hc_top = 0;
    hc_portbtn = 0;
    hc_landsbtn = 0;
    hc_fitbtn = 0;
    hc_legbtn = 0;
    hc_tofbtn = 0;
    hc_metbtn = 0;
    hc_fontmenu = 0;
    hc_fmtmenu = 0;
    hc_resmenu = 0;
    hc_pgsmenu = 0;
}


#ifdef WIN32
namespace {
    const char *no_printer_msg = "NO PRINTERS FOUND!";
}
#endif

// Static function
// Main procedure for the hardcopy popup.  On the first call, the
// popup is created.  Subsequent calls pop it down and up.  The actual
// work is done by the application-supplied functions in the cb
// struct.
//
void
GTKprintPopup::hc_hcpopup(GRobject caller, gtk_bag *wb, HCcb *cb,
    HCmode textmode, GRdraw *context)
{
    GTKprintPopup *hc = wb->HC();
    if (hc) {
        hc->hc_caller = caller;
        hc->hc_context = context;
        if (!hc->hc_active) {
            hc->hc_active = true;
            if (hc->hc_popup) {
                if (cb)
                    wb->HCsetFormat(cb->format);
                if (wb->PositionReferenceWidget()) {
                    GRX->SetPopupLocation(
                        GRloc(hc->hc_textmode == HCgraphical ?
                        LW_UL : LW_CENTER), hc->hc_popup,
                        wb->PositionReferenceWidget());
                }
                if (hc->hc_cb && hc->hc_cb->hcsetup)
                    (*hc->hc_cb->hcsetup)(true, hc->hc_fmt, false,
                        hc->hc_context);
                gtk_widget_show(hc->hc_popup);
                return;
            }
        }
        else {
            hc->hc_active = false;
            if (hc->hc_caller)
                GRX->Deselect(hc->hc_caller);
            if (hc->hc_popup) {
                gtk_widget_hide(hc->hc_popup);
                if (hc->hc_cb && hc->hc_cb->hcsetup)
                    (*hc->hc_cb->hcsetup)(false, hc->hc_fmt, false,
                        hc->hc_context);

                // in graphics mode, leave popup alive but hidden if we can
                if (hc->hc_textmode != HCgraphical || !hc->hc_popup->window ||
                        IsIconic(hc->hc_popup)) {
                    // window is 0 if "delete window" event
                    gtk_signal_disconnect_by_func(GTK_OBJECT(hc->hc_popup),
                        GTK_SIGNAL_FUNC(hc_cancel_proc), wb);
                    if (hc->hc_left)
                        hc->hc_lft_val = gtk_spin_button_get_value_as_float(
                            GTK_SPIN_BUTTON(hc->hc_left));
                    if (hc->hc_top)
                        hc->hc_top_val = gtk_spin_button_get_value_as_float(
                            GTK_SPIN_BUTTON(hc->hc_top));
                    if (hc->hc_wid) {
                        if (hc->hc_wlabel && GRX->GetStatus(hc->hc_wlabel))
                            hc->hc_wid_val = 0.0;
                        else
                            hc->hc_wid_val =
                                gtk_spin_button_get_value_as_float(
                                    GTK_SPIN_BUTTON(hc->hc_wid));
                    }
                    if (hc->hc_hei) {
                        if (hc->hc_hlabel && GRX->GetStatus(hc->hc_hlabel))
                            hc->hc_hei_val = 0.0;
                        else
                            hc->hc_hei_val =
                                gtk_spin_button_get_value_as_float(
                                    GTK_SPIN_BUTTON(hc->hc_hei));
                    }
                    if (hc->hc_cmdtxtbox) {
                        const char *s =
                            gtk_entry_get_text(GTK_ENTRY(hc->hc_cmdtxtbox));
                        if (GRX->GetStatus(hc->hc_tofbtn)) {
                            delete hc->hc_tofilename;
                            hc->hc_tofilename = lstring::copy(s);
                        }
                        else {
                            delete hc->hc_cmdtext;
                            hc->hc_cmdtext = lstring::copy(s);
                        }
                    }
                    hc->destroy_widgets();
                }
            }
            return;
        }
    }
    if (!hc) {
        hc = new GTKprintPopup;
        hc->hc_context = context;
        hc->hc_caller = caller;
        hc->hc_cb = cb;
        if (cb) {
            hc->hc_drvrmask = cb->drvrmask;
            hc->hc_fmt = cb->format;
            int i;
            for (i = 0; GRpkgIf()->HCof(i); i++) ;
            if (hc->hc_fmt >= i || (hc->hc_drvrmask & (1 << hc->hc_fmt))) {
                for (i = 0; GRpkgIf()->HCof(i); i++) {
                    if (!(hc->hc_drvrmask & (1 << i)))
                        break;
                }
                if (GRpkgIf()->HCof(i))
                    hc->hc_fmt = i;
                else if (textmode == HCgraphical) {
                    hc_pop_up_text(wb, "No hardcopy drivers available.", true);
                    if (hc->hc_caller)
                        GRX->Deselect(hc->hc_caller);
                    delete hc;
                    return;
                }
            }
            HCdesc *hcdesc = GRpkgIf()->HCof(hc->hc_fmt);
            if (hcdesc) {
                hc->hc_resol = hcdesc->defaults.defresol;
                if (hcdesc->limits.resols)
                    for (i = 0; hcdesc->limits.resols[i]; i++) ;
                else
                    i = 0;
                if (hc->hc_resol >= i)
                    hc->hc_resol = i-1;
                if (hc->hc_resol < 0)
                    hc->hc_resol = 0;
                if (hcdesc->defaults.command)
                    hc->hc_cmdtext = lstring::copy(hcdesc->defaults.command);
                else if (cb->command && *cb->command)
                    hc->hc_cmdtext = lstring::copy(cb->command);
                else if (HCdefaults::default_print_cmd)
                    hc->hc_cmdtext =
                        lstring::copy(HCdefaults::default_print_cmd);
                else
                    hc->hc_cmdtext = lstring::copy("");
                hc->hc_legend = hcdesc->defaults.legend;
                hc->hc_orient = hcdesc->defaults.orient;
            }
            else {
                hc->hc_resol = 0;
                hc->hc_cmdtext = lstring::copy("");
                hc->hc_legend = cb->legend;
                hc->hc_orient = cb->orient;
            }
            hc->hc_tofilename =
                lstring::copy(cb->tofilename ? cb->tofilename : "");
            hc->hc_tofile = cb->tofile;
        }
        else {
            hc->hc_cmdtext = lstring::copy("");
            hc->hc_tofilename = lstring::copy("");
            hc->hc_resol = 0;
            hc->hc_fmt = 0;
            hc->hc_orient = HCbest;
            hc->hc_legend = HClegOn;
            hc->hc_tofile = false;
            hc->hc_drvrmask = 0;
        }

        if (textmode == HCgraphical) {
            HCdesc *hcdesc = GRpkgIf()->HCof(hc->hc_fmt);
            if (hcdesc) {
                hc->hc_wid_val = MM(hcdesc->defaults.defwidth);
                hc->hc_hei_val = MM(hcdesc->defaults.defheight);
                hc->hc_lft_val = MM(hcdesc->defaults.defxoff);
                hc->hc_top_val = MM(hcdesc->defaults.defyoff);
            }
        }
        hc->hc_active = true;
        hc->hc_textmode = textmode;
        wb->SetHC(hc);
    }

    hc->hc_popup = gtk_NewPopup(wb, "Print Control Panel", hc_cancel_proc, wb);
    gtk_window_set_resizable(GTK_WINDOW(hc->hc_popup), false);

    GtkWidget *form = gtk_table_new(1, 8, false);
    gtk_widget_show(form);
    gtk_container_add(GTK_CONTAINER(hc->hc_popup), form);
    gtk_container_set_border_width(GTK_CONTAINER(hc->hc_popup), 2);

    //
    // top button rows
    //
    GtkWidget *button;
    GtkWidget *row1 = gtk_hbox_new(false, 2);
    gtk_widget_show(row1);
    int row1cnt = 0;
    if (textmode == HCgraphical) {
        if (cb && cb->hcframe) {
            button = gtk_toggle_button_new_with_label("Frame");
            gtk_widget_set_name(button, "Frame");
            gtk_widget_show(button);
            gtk_box_pack_start(GTK_BOX(row1), button, true, true, 0);
            gtk_signal_connect(GTK_OBJECT(button), "clicked",
                GTK_SIGNAL_FUNC(hc_frame_proc), wb);
            row1cnt++;
        }

        button = gtk_check_button_new_with_label("Best Fit");
        gtk_widget_set_name(button, "BestFit");
        gtk_widget_show(button);
        gtk_box_pack_start(GTK_BOX(row1), button, false, false, 0);
        gtk_signal_connect(GTK_OBJECT(button), "clicked",
            GTK_SIGNAL_FUNC(hc_fit_proc), wb);
        hc->hc_fitbtn = button;
        row1cnt++;

        if (!cb || cb->legend != HClegNone) {
            button = gtk_check_button_new_with_label("Legend");
            gtk_widget_set_name(button, "Legend");
            gtk_widget_show(button);
            gtk_box_pack_start(GTK_BOX(row1), button, false, false, 0);
            gtk_signal_connect(GTK_OBJECT(button), "clicked",
                GTK_SIGNAL_FUNC(hc_legend_proc), wb);
            hc->hc_legbtn = button;
            row1cnt++;
        }
    }
    else {
        // Handle Return, same as pressing Print.
        gtk_signal_connect(GTK_OBJECT(hc->hc_popup), "key-press-event",
            GTK_SIGNAL_FUNC(hc_key_hdlr), wb);
        if (!cb || cb->legend != HClegNone) {
            button = gtk_check_button_new_with_label("Legend");
            gtk_widget_set_name(button, "Legend");
            gtk_widget_show(button);
            gtk_box_pack_start(GTK_BOX(row1), button, true, true, 0);
            if (hc->hc_legend == HClegNone)
                gtk_widget_set_sensitive(button, false);
            else
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),
                    hc->hc_legend == HClegOff ? false : true);
            gtk_signal_connect(GTK_OBJECT(button), "clicked",
                GTK_SIGNAL_FUNC(hc_legend_proc), wb);
            hc->hc_legbtn = button;
            row1cnt++;
        }
        if (textmode == HCtextPS) {
            button = gtk_radio_button_new_with_label(0, "A4");
            gtk_widget_set_name(button, "A4");
            gtk_widget_show(button);
            gtk_box_pack_start(GTK_BOX(row1), button, false, false, 0);
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), true);
            row1cnt++;

            GSList *group = gtk_radio_button_group(GTK_RADIO_BUTTON(button));
            button = gtk_radio_button_new_with_label(group, "Letter");
            gtk_widget_set_name(button, "Letter");
            gtk_widget_show(button);
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), true);
            gtk_box_pack_start(GTK_BOX(row1), button, false, false, 0);
            gtk_signal_connect(GTK_OBJECT(button), "clicked",
                GTK_SIGNAL_FUNC(hc_pagesize_proc), wb);
            row1cnt++;

            hc->hc_textfmt = PStimes;  // default postscript times

            GtkWidget *entry = gtk_option_menu_new();
            gtk_widget_set_name(entry, "TextFmt");
            gtk_widget_show(entry);
            gtk_box_pack_start(GTK_BOX(row1), entry, true, true, 0);
            GtkWidget *menu = gtk_menu_new();
            gtk_widget_set_name(menu, "TextFmt");
            gtk_option_menu_set_menu(GTK_OPTION_MENU(entry), menu);
            hc->hc_fontmenu = entry;
            row1cnt++;

            hc_update_menu(hc);
            // for some reason, the option menu doesn't size itself properly
            gtk_window_set_default_size(GTK_WINDOW(hc->hc_popup), 320, -1);
        }
    }

    GtkWidget *helpbutton = gtk_button_new_with_label("Help");
    gtk_widget_set_name(helpbutton, "Help");
    gtk_widget_show(helpbutton);
    gtk_signal_connect(GTK_OBJECT(helpbutton), "clicked",
        GTK_SIGNAL_FUNC(hc_help_proc), wb);
    // don't leave a lonely help button in the top row, move it to the
    // second row
    if (row1cnt) {
        gtk_misc_set_padding(GTK_MISC(GTK_BIN(helpbutton)->child), 4, 0);
        if (textmode == HCtextPS)
            gtk_box_pack_start(GTK_BOX(row1), helpbutton, false, false, 2);
        else
            gtk_box_pack_start(GTK_BOX(row1), helpbutton, true, true, 0);
        gtk_table_attach(GTK_TABLE(form), row1, 0, 1, 0, 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }
    else
        gtk_widget_destroy(row1);

    //
    // framed printer command label and entry area
    //
    GtkWidget *row2 = gtk_hbox_new(false, 2);
    gtk_widget_show(row2);

#ifdef WIN32
    hc->hc_cmdlab = gtk_label_new(
        hc->hc_tofile ? "File Name" : "Printer Name");
#else
    hc->hc_cmdlab = gtk_label_new(
        hc->hc_tofile ? "File Name" : "Print Command");
#endif
    gtk_widget_show(hc->hc_cmdlab);
    gtk_misc_set_alignment(GTK_MISC(hc->hc_cmdlab), 0, 0.5);
    gtk_misc_set_padding(GTK_MISC(hc->hc_cmdlab), 2, 2);
    gtk_box_pack_start(GTK_BOX(row2), hc->hc_cmdlab, true, true, 0);

    button = gtk_check_button_new_with_label("To File");
    gtk_widget_set_name(button, "ToFile");
    gtk_widget_show(button);
    gtk_box_pack_end(GTK_BOX(row2), button, false, false, 0);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(hc_tofile_proc), wb);
    if (hc->hc_tofile)
        GRX->Select(button);
    hc->hc_tofbtn = button;

    GtkWidget *frame = gtk_frame_new(0);
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), row2);

    if (!row1cnt) {
        row2 = gtk_hbox_new(false, 2);
        gtk_widget_show(row2);
        gtk_box_pack_start(GTK_BOX(row2), frame, true, true, 0);
        gtk_box_pack_start(GTK_BOX(row2), helpbutton, true, true, 0);
        gtk_table_attach(GTK_TABLE(form), row2, 0, 1, 1, 2,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);
    }
    else
        gtk_table_attach(GTK_TABLE(form), frame, 0, 1, 1, 2,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);

    GtkWidget *hbox = gtk_hbox_new(false, 2);
    gtk_widget_show(hbox);

    hc->hc_cmdtxtbox = gtk_entry_new();
    gtk_widget_show(hc->hc_cmdtxtbox);
    gtk_entry_set_text(GTK_ENTRY(hc->hc_cmdtxtbox),
        hc->hc_tofile ? hc->hc_tofilename : hc->hc_cmdtext);
    gtk_box_pack_start(GTK_BOX(hbox), hc->hc_cmdtxtbox, true, true, 0);
#ifdef WIN32
    hc->hc_prntmenu = gtk_option_menu_new();
    gtk_widget_show(hc->hc_prntmenu);
    gtk_box_pack_start(GTK_BOX(hbox), hc->hc_prntmenu, true, true, 0);
#endif

    gtk_table_attach(GTK_TABLE(form), hbox, 0, 1, 2, 3,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    if (textmode == HCgraphical) {

        //
        // format and resolution labels and option menus
        //
        GtkWidget *row3 = gtk_table_new(2, 2, false);
        gtk_widget_show(row3);

        hbox = gtk_hbox_new(false, 2);
        gtk_widget_show(hbox);

        // format select button and display label
        GtkWidget *label = gtk_label_new("Format");
        gtk_widget_show(label);
        gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
        gtk_misc_set_padding(GTK_MISC(label), 2, 2);
        gtk_box_pack_start(GTK_BOX(hbox), label, true, true, 0);

        button = gtk_radio_button_new_with_label(0, "Portrait");
        gtk_widget_set_name(button, "Portrait");
        gtk_widget_show(button);
        gtk_box_pack_start(GTK_BOX(hbox), button, false, false, 0);
        gtk_signal_connect(GTK_OBJECT(button), "toggled",
            GTK_SIGNAL_FUNC(hc_port_proc), wb);
        hc->hc_portbtn = button;
        GSList *group = gtk_radio_button_group(GTK_RADIO_BUTTON(button));
        button = gtk_radio_button_new_with_label(group, "Landscape");
        gtk_widget_set_name(button, "Landscape");
        gtk_widget_show(button);
        gtk_box_pack_start(GTK_BOX(hbox), button, false, false, 0);
        hc->hc_landsbtn = button;

        frame = gtk_frame_new(0);
        gtk_widget_show(frame);
        gtk_container_add(GTK_CONTAINER(frame), hbox);
        gtk_table_attach(GTK_TABLE(row3), frame, 0, 1, 0, 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);

        GtkWidget *entry = gtk_option_menu_new();
        gtk_widget_set_name(entry, "Format");
        gtk_widget_show(entry);
        hc->hc_fmtmenu = entry;

        GtkWidget *menu = gtk_menu_new();
        gtk_widget_set_name(menu, "Format");
        for (int i = 0; GRpkgIf()->HCof(i); i++) {
            if (hc->hc_drvrmask & (1 << i))
                continue;
            HCdesc *hcdesc = GRpkgIf()->HCof(i);
            GtkWidget *mi = gtk_menu_item_new_with_label(hcdesc->descr);
            gtk_widget_set_name(mi, hcdesc->descr);
            gtk_widget_show(mi);
            gtk_menu_append(GTK_MENU(menu), mi);
            gtk_object_set_user_data(GTK_OBJECT(mi), (void*)(long)i);
            gtk_signal_connect(GTK_OBJECT(mi), "activate",
                GTK_SIGNAL_FUNC(hc_formenu_proc), wb);
        }
        gtk_option_menu_set_menu(GTK_OPTION_MENU(entry), menu);
        gtk_option_menu_set_history(GTK_OPTION_MENU(entry), hc->hc_fmt);

        gtk_table_attach(GTK_TABLE(row3), entry, 0, 1, 1, 2,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);

        // resolution select buttom and display label
        label = gtk_label_new("Resolution");
        gtk_widget_show(label);
        gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
        gtk_misc_set_padding(GTK_MISC(label), 2, 2);
        frame = gtk_frame_new(0);
        gtk_widget_show(frame);
        gtk_container_add(GTK_CONTAINER(frame), label);
        gtk_table_attach(GTK_TABLE(row3), frame, 1, 2, 0, 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);

        entry = gtk_option_menu_new();
        gtk_widget_set_name(entry, "Resolution");
        gtk_widget_show(entry);
        hc->hc_resmenu = entry;

        menu = gtk_menu_new();
        gtk_widget_set_name(menu, "Resolution");
        const char **s = GRpkgIf()->HCof(hc->hc_fmt) ?
            GRpkgIf()->HCof(hc->hc_fmt)->limits.resols : 0;
        if (s && *s) {
            for (int i = 0; s[i]; i++) {
                GtkWidget *mi = gtk_menu_item_new_with_label(s[i]);
                gtk_widget_set_name(mi, s[i]);
                gtk_widget_show(mi);
                gtk_menu_append(GTK_MENU(menu), mi);
                gtk_object_set_user_data(GTK_OBJECT(mi), (void*)(long)i);
                gtk_signal_connect(GTK_OBJECT(mi), "activate",
                    GTK_SIGNAL_FUNC(hc_resol_proc), wb);
            }
        }
        else {
            GtkWidget *mi = gtk_menu_item_new_with_label("fixed");
            gtk_widget_set_name(mi, "fixed");
            gtk_widget_show(mi);
            gtk_menu_append(GTK_MENU(menu), mi);
        }
        gtk_option_menu_set_menu(GTK_OPTION_MENU(entry), menu);
        gtk_option_menu_set_history(GTK_OPTION_MENU(entry), hc->hc_resol);

        gtk_table_attach(GTK_TABLE(row3), entry, 1, 2, 1, 2,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);

        gtk_table_attach(GTK_TABLE(form), row3, 0, 1, 3, 4,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);

        //
        // size labels and text widgets
        //
        HCdesc *hcdesc = GRpkgIf()->HCof(hc->hc_fmt);
        hc_checklims(hcdesc);
        GtkWidget *row4 = gtk_hbox_new(false, 2);
        gtk_widget_show(row4);

        GtkWidget *vbox = gtk_vbox_new(false, 2);
        gtk_widget_show(vbox);

        button = gtk_toggle_button_new_with_label("Width");
        gtk_widget_set_name(button, "AutoWidth");
        gtk_widget_show(button);
        gtk_signal_connect(GTK_OBJECT(button), "clicked",
            GTK_SIGNAL_FUNC(hc_auto_proc), wb);

        frame = gtk_frame_new(0);
        gtk_widget_show(frame);
        gtk_container_add(GTK_CONTAINER(frame), button);
        gtk_box_pack_start(GTK_BOX(vbox), frame, true, true, 0);
        hc->hc_wlabel = button;

        double amin = 1.0;
        double amax = 10.0;
        if (hcdesc) {
            amin = MM(hcdesc->limits.minwidth);
            amax = MM(hcdesc->limits.maxwidth);
        }
        GtkObject *adj = gtk_adjustment_new(hc->hc_wid_val, amin, amax, .1,
            1.0, 0.0);
        entry = gtk_spin_button_new(GTK_ADJUSTMENT(adj), 1.0, 2);
        gtk_widget_set_name(entry, "Width");
        gtk_widget_show(entry);
        gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(entry), true);
        hc->hc_wid = entry;
        gtk_box_pack_start(GTK_BOX(vbox), entry, true, true, 0);
        gtk_box_pack_start(GTK_BOX(row4), vbox, true, true, 0);

        vbox = gtk_vbox_new(false, 2);
        gtk_widget_show(vbox);

        button = gtk_toggle_button_new_with_label("Height");
        gtk_widget_set_name(button, "AutoHeight");
        gtk_widget_show(button);
        gtk_signal_connect(GTK_OBJECT(button), "clicked",
            GTK_SIGNAL_FUNC(hc_auto_proc), wb);

        frame = gtk_frame_new(0);
        gtk_widget_show(frame);
        gtk_container_add(GTK_CONTAINER(frame), button);
        gtk_box_pack_start(GTK_BOX(vbox), frame, true, true, 0);
        hc->hc_hlabel = button;

        if (hcdesc) {
            amin = MM(hcdesc->limits.minheight);
            amax = MM(hcdesc->limits.maxheight);
        }
        adj = gtk_adjustment_new(hc->hc_hei_val, amin, amax, .1, 1.0, 0.0);
        entry = gtk_spin_button_new(GTK_ADJUSTMENT(adj), 1.0, 2);
        gtk_widget_set_name(entry, "Height");
        gtk_widget_show(entry);
        gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(entry), true);
        hc->hc_hei = entry;
        gtk_box_pack_start(GTK_BOX(vbox), entry, true, true, 0);
        gtk_box_pack_start(GTK_BOX(row4), vbox, true, true, 0);

        vbox = gtk_vbox_new(false, 2);
        gtk_widget_show(vbox);

        label = gtk_label_new("Left");
        gtk_widget_show(label);
        gtk_misc_set_alignment(GTK_MISC(label), 0.5, 0.5);
        gtk_misc_set_padding(GTK_MISC(label), 2, 2);
        frame = gtk_frame_new(0);
        gtk_widget_show(frame);
        gtk_container_add(GTK_CONTAINER(frame), label);
        gtk_box_pack_start(GTK_BOX(vbox), frame, true, true, 0);
        hc->hc_xlabel = label;

        amax = 1.0;
        if (hcdesc) {
            amin = MM(hcdesc->limits.minxoff);
            amax = MM(hcdesc->limits.maxxoff);
        }
        adj = gtk_adjustment_new(hc->hc_lft_val, amin, amax, .1, 1.0, 0.0);
        entry = gtk_spin_button_new(GTK_ADJUSTMENT(adj), 1.0, 2);
        gtk_widget_set_name(entry, "Left");
        gtk_widget_show(entry);
        gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(entry), true);
        hc->hc_left = entry;
        gtk_box_pack_start(GTK_BOX(vbox), entry, true, true, 0);
        gtk_box_pack_start(GTK_BOX(row4), vbox, true, true, 0);

        vbox = gtk_vbox_new(false, 2);
        gtk_widget_show(vbox);

        label = gtk_label_new(
            (GRpkgIf()->HCof(hc->hc_fmt)->limits.flags & HCtopMargin) ?
                "Top" : "Bottom");
        gtk_widget_show(label);
        gtk_misc_set_alignment(GTK_MISC(label), 0.5, 0.5);
        gtk_misc_set_padding(GTK_MISC(label), 2, 2);
        frame = gtk_frame_new(0);
        gtk_widget_show(frame);
        gtk_container_add(GTK_CONTAINER(frame), label);
        gtk_box_pack_start(GTK_BOX(vbox), frame, true, true, 0);
        hc->hc_ylabel = label;

        if (hcdesc) {
            amin = MM(hcdesc->limits.minyoff);
            amax = MM(hcdesc->limits.maxyoff);
        }
        adj = gtk_adjustment_new(hc->hc_top_val, amin, amax, .1, 1.0, 0.0);
        entry = gtk_spin_button_new(GTK_ADJUSTMENT(adj), 1.0, 2);
        gtk_widget_set_name(entry, "TopBot");
        gtk_widget_show(entry);
        gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(entry), true);
        hc->hc_top = entry;

        gtk_box_pack_start(GTK_BOX(vbox), entry, true, true, 0);
        gtk_box_pack_start(GTK_BOX(row4), vbox, true, true, 0);

        gtk_table_attach(GTK_TABLE(form), row4, 0, 1, 4, 5,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);

        //
        // paper size option menu and metric button
        //
        GtkWidget *row5 = gtk_hbox_new(false, 2);
        gtk_widget_show(row5);

        entry = gtk_option_menu_new();
        gtk_widget_set_name(entry, "PageSize");
        gtk_widget_show(entry);
        menu = gtk_menu_new();
        gtk_widget_set_name(menu, "PageSize");
        int i = 0;
        for (sMedia *m = pagesizes; m->name; m++, i++) {
            GtkWidget *mi = gtk_menu_item_new_with_label(m->name);
            gtk_widget_set_name(mi, m->name);
            gtk_widget_show(mi);
            gtk_menu_append(GTK_MENU(menu), mi);
            gtk_object_set_user_data(GTK_OBJECT(mi), (void*)(long)i);
            gtk_signal_connect(GTK_OBJECT(mi), "activate",
                GTK_SIGNAL_FUNC(hc_pagesize_proc), wb);
        }
        gtk_option_menu_set_menu(GTK_OPTION_MENU(entry), menu);
        gtk_option_menu_set_history(GTK_OPTION_MENU(entry), 0);
        gtk_box_pack_start(GTK_BOX(row5), entry, true, true, 0);
        hc->hc_pgsmenu = entry;

        button = gtk_check_button_new_with_label("Metric (mm)");
        gtk_widget_set_name(button, "Metric");
        gtk_widget_show(button);
        gtk_signal_connect(GTK_OBJECT(button), "clicked",
            GTK_SIGNAL_FUNC(hc_metric_proc), wb);
        gtk_box_pack_end(GTK_BOX(row5), button, false, false, 0);
        hc->hc_metbtn = button;

        gtk_table_attach(GTK_TABLE(form), row5, 0, 1, 5, 6,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    GtkWidget *sep = gtk_hseparator_new();
    gtk_widget_show(sep);
    gtk_table_attach(GTK_TABLE(form), sep, 0, 1, 6, 7,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    //
    // print and dismiss buttons
    //
    GtkWidget *row5 = gtk_hbox_new(false, 2);
    gtk_widget_show(row5);

    button = gtk_button_new_with_label("Print");
    gtk_widget_set_name(button, "Print");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(hc_go_proc), wb);
    gtk_box_pack_start(GTK_BOX(row5), button, true, true, 0);

    button = gtk_button_new_with_label("Dismiss");
    gtk_widget_set_name(button, "Dismiss");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(hc_cancel_proc), wb);
    gtk_box_pack_start(GTK_BOX(row5), button, true, true, 0);

    gtk_table_attach(GTK_TABLE(form), row5, 0, 1, 7, 8,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    gtk_window_set_focus(GTK_WINDOW(hc->hc_popup), button);

    if (hc->hc_cb && hc->hc_cb->hcsetup)
        (*cb->hcsetup)(true, hc->hc_fmt, false, hc->hc_context);

    if (textmode == HCgraphical) {
        HCdesc *hcdesc = GRpkgIf()->HCof(hc->hc_fmt);
        if (hcdesc) {
            hc_set_sens(hc, hcdesc->limits.flags);
            if (!(hcdesc->limits.flags & HCdontCareWidth) &&
                    hc->hc_wid_val == 0.0 && hcdesc->limits.minwidth > 0.0) {
                if (hcdesc->limits.flags & HCnoAutoWid) {
                    hc->hc_wid_val = MM(hcdesc->limits.minwidth);
                    hcdesc->defaults.defwidth = hcdesc->limits.minwidth;
                }
                else {
                    GRX->Select(hc->hc_wlabel);
                    float tmp = hcdesc->last_w;
                    hc_auto_proc(hc->hc_wlabel, wb);
                    hcdesc->last_w = tmp;
                }
            }
            if (!(hcdesc->limits.flags & HCdontCareHeight) &&
                    hc->hc_hei_val == 0.0 && hcdesc->limits.minheight > 0.0) {
                if (hcdesc->limits.flags & HCnoAutoHei) {
                    hc->hc_hei_val = MM(hcdesc->limits.minheight);
                    hcdesc->defaults.defheight = hcdesc->limits.minheight;
                }
                else {
                    GRX->Select(hc->hc_hlabel);
                    float tmp = hcdesc->last_h;
                    hc_auto_proc(hc->hc_hlabel, wb);
                    hcdesc->last_h = tmp;
                }
            }
        }
    }

#ifdef WIN32
    if (hc->hc_tofile) {
        gtk_widget_show(hc->hc_cmdtxtbox);
        gtk_widget_hide(hc->hc_prntmenu);
    }
    else {
        gtk_widget_hide(hc->hc_cmdtxtbox);
        gtk_widget_show(hc->hc_prntmenu);
    }
    GtkWidget *menu = gtk_menu_new();
    gtk_widget_set_name(menu, "printers");
    gtk_widget_show(menu);
    gtk_option_menu_set_menu(GTK_OPTION_MENU(hc->hc_prntmenu), menu);
    if (!hc->hc_numprinters) {
        GtkWidget *mi =
            gtk_menu_item_new_with_label(no_printer_msg);
        gtk_widget_set_name(mi, no_printer_msg);
        gtk_widget_show(mi);
        gtk_menu_append(GTK_MENU(menu), mi);
        gtk_signal_connect(GTK_OBJECT(mi), "activate",
            GTK_SIGNAL_FUNC(hc_formenu_proc), wb);
    }
    else {
        for (int i = 0; i < hc->hc_numprinters; i++) {
            GtkWidget *mi =
                gtk_menu_item_new_with_label(hc->hc_printers[i]);
            gtk_widget_set_name(mi, hc->hc_printers[i]);
            gtk_widget_show(mi);
            gtk_menu_append(GTK_MENU(menu), mi);
            gtk_signal_connect(GTK_OBJECT(mi), "activate",
                GTK_SIGNAL_FUNC(hc_formenu_proc), wb);
        }
    }
    hc_set_printer(wb);
#endif

    gtk_window_set_transient_for(GTK_WINDOW(hc->hc_popup),
        GTK_WINDOW(wb->Shell()));
    if (wb->PositionReferenceWidget())
        GRX->SetPopupLocation(
            GRloc(textmode == HCgraphical ? LW_UL : LW_CENTER),
            hc->hc_popup, wb->PositionReferenceWidget());
    gtk_widget_show(hc->hc_popup);
}


// Static function.
// Pop up an advisory, hopefully next to the hardcopy panel.
//
void
GTKprintPopup::hc_pop_up_text(gtk_bag *w, const char *message, bool error)
{
    if (!w)
        return;
    w->PopUpMessage(message, error);
}


#ifdef WIN32
namespace {
    inline bool ncomp(const char *s1, const char *s2)
    {
        int i1 = atoi(s1);
        int i2 = atoi(s2);
        return (i1 < i2);
    }
}

// Static function.
//
void
GTKprintPopup::hc_set_printer(gtk_bag *wb)
{
    GTKprintPopup *hc = wb->HC();
    if (!hc)
        return;
    if (!hc->hc_printers) {
        curprinter = 0;
        if (hc->hc_prntmenu)
            gtk_option_menu_set_history(GTK_OPTION_MENU(hc->hc_prntmenu), 0);
        return;
    }
    if (curprinter < 0)
        curprinter = hc->hc_numprinters - 1;
    else if (curprinter >= hc->hc_numprinters)
        curprinter = 0;

    if (hc->hc_prntmenu)
        gtk_option_menu_set_history(GTK_OPTION_MENU(hc->hc_prntmenu),
            curprinter);
    char *name = hc->hc_printers[curprinter];

    if (MSPdesc.limits.resols) {
        for (const char **s = MSPdesc.limits.resols; *s; s++)
            delete [] *s;
        delete [] MSPdesc.limits.resols;
        MSPdesc.limits.resols = 0;
    }

    int ret = DeviceCapabilities(name, 0, DC_ENUMRESOLUTIONS, 0, 0);
    long *resols = 0;
    if (ret <= 1)
        // fixed resolution
        MSPdesc.limits.flags |= HCfixedResol;
    else {
        MSPdesc.limits.flags &= ~HCfixedResol;
        resols = new long[2*ret];
        DeviceCapabilities(name, 0, DC_ENUMRESOLUTIONS, (CHAR*)resols, 0);
        MSPdesc.limits.resols = new const char*[ret+1];
        char buf[64];
        for (int i = 0; i < 2*ret; i += 2) {
            sprintf(buf, "%ld", resols[i]);
            MSPdesc.limits.resols[i/2] = lstring::copy(buf);
        }
        MSPdesc.limits.resols[ret] = 0;
        delete [] resols;
        std::sort(MSPdesc.limits.resols, MSPdesc.limits.resols + ret, ncomp);
    }
    ret = DeviceCapabilities(name, 0, DC_ORIENTATION, 0, 0);
    if (ret > 0)
        // landscape supported
        MSPdesc.limits.flags &= ~HCnoCanRotate;
    else
        MSPdesc.limits.flags |= HCnoCanRotate;

    HCdesc *hcdesc = GRpkgIf()->HCof(hc->hc_fmt);
    if (!hcdesc)
        return;
    if (hc->hc_textmode == HCgraphical && hcdesc == &MSPdesc) {
        hc_set_sens(hc, hcdesc->limits.flags);
        if (hc->hc_wlabel) {
            if (!(hcdesc->limits.flags & HCdontCareWidth) &&
                    hcdesc->defaults.defwidth == 0.0 &&
                    hcdesc->limits.minwidth > 0.0) {
                GRX->Select(hc->hc_wlabel);
                hc_auto_proc(hc->hc_wlabel, wb);
            }
        }
        if (hc->hc_hlabel) {
            if (!(hcdesc->limits.flags & HCdontCareHeight) &&
                    hcdesc->defaults.defheight == 0.0 &&
                    hcdesc->limits.minheight > 0.0) {
                GRX->Select(hc->hc_hlabel);
                hc_auto_proc(hc->hc_hlabel, wb);
            }
        }
    }
}
#endif


// Static function.
// Change the print format.
//
void
GTKprintPopup::hc_set_format(gtk_bag *wb, int index, bool set_menu)
{
    GTKprintPopup *hc = wb->HC();
    if (!hc || !hc->hc_active)
        return;

    if (GRX->GetStatus(hc->hc_wlabel))
        GRX->SetStatus(hc->hc_wlabel, false);
    if (GRX->GetStatus(hc->hc_hlabel))
        GRX->SetStatus(hc->hc_hlabel, false);

    if (index < 0 || index > 100)
        // sanity check
        return;

    if (hc->hc_drvrmask & (1 << index))
        return;;
    int i = hc->hc_fmt;
    hc->hc_fmt = index;
    HCdesc *oldhcdesc = GRpkgIf()->HCof(i);
    HCdesc *newhcdesc = GRpkgIf()->HCof(index);
    if (!oldhcdesc || !newhcdesc)
        return;

    if (set_menu)
        gtk_option_menu_set_history(GTK_OPTION_MENU(hc->hc_fmtmenu), index);

    // Set the current defaults to the current values.
    if (oldhcdesc->defaults.command)
        delete [] oldhcdesc->defaults.command;
    if (!hc->hc_tofile) {
        const char *str = gtk_entry_get_text(GTK_ENTRY(hc->hc_cmdtxtbox));
        oldhcdesc->defaults.command = lstring::copy(str);
    }
    else
        oldhcdesc->defaults.command = lstring::copy(hc->hc_cmdtext);
    oldhcdesc->defaults.defresol = hc->hc_resol;

    oldhcdesc->defaults.legend = hc->hc_legend;
    hc->hc_legend = newhcdesc->defaults.legend;
    oldhcdesc->defaults.orient = hc->hc_orient;
    hc->hc_orient = newhcdesc->defaults.orient;

    if (hc->hc_wid) {
        double w = gtk_spin_button_get_value_as_float(
            GTK_SPIN_BUTTON(hc->hc_wid));
        if (hc->hc_metric)
            w /= MMPI;
        oldhcdesc->defaults.defwidth = w;
    }
    if (hc->hc_hei) {
        double h = gtk_spin_button_get_value_as_float(
            GTK_SPIN_BUTTON(hc->hc_hei));
        if (hc->hc_metric)
            h /= MMPI;
        oldhcdesc->defaults.defheight = h;
    }
    if (hc->hc_left) {
        double x = gtk_spin_button_get_value_as_float(
            GTK_SPIN_BUTTON(hc->hc_left));
        if (hc->hc_metric)
            x /= MMPI;
        oldhcdesc->defaults.defxoff = x;
    }
    if (hc->hc_top) {
        double y = gtk_spin_button_get_value_as_float(
            GTK_SPIN_BUTTON(hc->hc_top));
        if (hc->hc_metric)
            y /= MMPI;
        oldhcdesc->defaults.defyoff = y;
    }

    hc_checklims(newhcdesc);

    // set the new values
    delete [] hc->hc_cmdtext;
    if (newhcdesc->defaults.command)
        hc->hc_cmdtext = lstring::copy(newhcdesc->defaults.command);
    else if (HCdefaults::default_print_cmd)
        hc->hc_cmdtext = lstring::copy(HCdefaults::default_print_cmd);
    else
        hc->hc_cmdtext = lstring::copy("");
    if (!hc->hc_tofile)
        gtk_entry_set_text(GTK_ENTRY(hc->hc_cmdtxtbox), hc->hc_cmdtext);

    hc->hc_resol = newhcdesc->defaults.defresol;

    if (hc->hc_wid) {
        GtkAdjustment *adj = (GtkAdjustment*)gtk_adjustment_new(
            MM(newhcdesc->defaults.defwidth), MM(newhcdesc->limits.minwidth),
            MM(newhcdesc->limits.maxwidth), .1, 1.0, 0.0);
        gtk_spin_button_set_adjustment(GTK_SPIN_BUTTON(hc->hc_wid), adj);
        gtk_adjustment_value_changed(adj);
    }
    if (hc->hc_hei) {
        GtkAdjustment *adj = (GtkAdjustment*)gtk_adjustment_new(
            MM(newhcdesc->defaults.defheight), MM(newhcdesc->limits.minheight),
            MM(newhcdesc->limits.maxheight), .1, 1.0, 0.0);
        gtk_spin_button_set_adjustment(GTK_SPIN_BUTTON(hc->hc_hei), adj);
        gtk_adjustment_value_changed(adj);
    }
    if (hc->hc_left) {
        GtkAdjustment *adj = (GtkAdjustment*)gtk_adjustment_new(
            MM(newhcdesc->defaults.defxoff), MM(newhcdesc->limits.minxoff),
            MM(newhcdesc->limits.maxxoff), .1, 1.0, 0.0);
        gtk_spin_button_set_adjustment(GTK_SPIN_BUTTON(hc->hc_left), adj);
        gtk_adjustment_value_changed(adj);
    }
    if (hc->hc_top) {
        GtkAdjustment *adj = (GtkAdjustment*)gtk_adjustment_new(
            MM(newhcdesc->defaults.defyoff), MM(newhcdesc->limits.minyoff),
            MM(newhcdesc->limits.maxyoff), .1, 1.0, 0.0);
        gtk_spin_button_set_adjustment(GTK_SPIN_BUTTON(hc->hc_top), adj);
        gtk_adjustment_value_changed(adj);
    }

    if (hc->hc_ylabel) {
        if (newhcdesc->limits.flags & HCtopMargin)
            gtk_label_set_text(GTK_LABEL(hc->hc_ylabel), "Top");
        else
            gtk_label_set_text(GTK_LABEL(hc->hc_ylabel), "Bottom");
    }

    hc_set_sens(hc, newhcdesc->limits.flags);
    if (!(newhcdesc->limits.flags & HCdontCareWidth) &&
            newhcdesc->defaults.defwidth == 0.0 &&
            newhcdesc->limits.minwidth > 0.0) {
        if (newhcdesc->limits.flags & HCnoAutoWid)
            newhcdesc->defaults.defwidth = newhcdesc->limits.minwidth;
        else if (hc->hc_wlabel) {
            GRX->Select(hc->hc_wlabel);
            hc_auto_proc(hc->hc_wlabel, wb);
        }
    }
    if (!(newhcdesc->limits.flags & HCdontCareHeight) &&
            newhcdesc->defaults.defheight == 0.0 &&
            newhcdesc->limits.minheight > 0.0) {
        if (newhcdesc->limits.flags & HCnoAutoHei)
            newhcdesc->defaults.defheight = newhcdesc->limits.minheight;
        else if (hc->hc_ylabel) {
            GRX->SetStatus(hc->hc_hlabel, true);
            hc_auto_proc(hc->hc_hlabel, wb);
        }
    }

    if (hc->hc_resmenu) {
        GtkWidget *menu =
            gtk_option_menu_get_menu(GTK_OPTION_MENU(hc->hc_resmenu));
        const char *mname = gtk_widget_get_name(menu);
        mname = lstring::copy(mname);
        gtk_widget_destroy(menu);
        menu = gtk_menu_new();
        gtk_widget_set_name(menu, mname);
        delete [] mname;

        const char **s = newhcdesc->limits.resols;
        if (s && *s) {
            for (int j = 0; s[j]; j++) {
                GtkWidget *mi = gtk_menu_item_new_with_label(s[j]);
                gtk_widget_set_name(mi, s[j]);
                gtk_widget_show(mi);
                gtk_menu_append(GTK_MENU(menu), mi);
                gtk_object_set_user_data(GTK_OBJECT(mi), (void*)(long)j);
                gtk_signal_connect(GTK_OBJECT(mi), "activate",
                    GTK_SIGNAL_FUNC(hc_resol_proc), wb);
            }
        }
        else {
            GtkWidget *mi = gtk_menu_item_new_with_label("fixed");
            gtk_widget_set_name(mi, "fixed");
            gtk_widget_show(mi);
            gtk_menu_append(GTK_MENU(menu), mi);
        }
        gtk_option_menu_set_menu(GTK_OPTION_MENU(hc->hc_resmenu), menu);
        gtk_option_menu_set_history(GTK_OPTION_MENU(hc->hc_resmenu),
            hc->hc_resol);
    }
    if (hc->hc_cb && hc->hc_cb->hcsetup)
        (*hc->hc_cb->hcsetup)(true, hc->hc_fmt, false, hc->hc_context);
}


// Static function.
void
GTKprintPopup::hc_update_menu(GTKprintPopup *hc)
{
    GtkWidget *menu =
        gtk_option_menu_get_menu(GTK_OPTION_MENU(hc->hc_fontmenu));
    if (!menu)
        return;
    GList *gl = gtk_container_children(GTK_CONTAINER(menu));
    for (GList *l = gl; l; l = l->next)
        gtk_widget_destroy(GTK_WIDGET(l->data));
    g_list_free(gl);

    int i = 0, hist = -1;
    GtkWidget *mi = gtk_menu_item_new_with_label("PostScript Times");
    gtk_widget_set_name(mi, "PostScript Times");
    gtk_widget_show(mi);
    gtk_menu_append(GTK_MENU(menu), mi);
    gtk_object_set_user_data(GTK_OBJECT(mi), (void*)PStimes);
    gtk_signal_connect(GTK_OBJECT(mi), "activate",
        GTK_SIGNAL_FUNC(hc_menu_proc), hc);
    if (hc->hc_textfmt == PStimes)
        hist = i;
    i++;

    mi = gtk_menu_item_new_with_label("PostScript Helvetica");
    gtk_widget_set_name(mi, "PostScript Helvetica");
    gtk_widget_show(mi);
    gtk_menu_append(GTK_MENU(menu), mi);
    gtk_object_set_user_data(GTK_OBJECT(mi), (void*)PShelv);
    gtk_signal_connect(GTK_OBJECT(mi), "activate",
        GTK_SIGNAL_FUNC(hc_menu_proc), hc);
    if (hc->hc_textfmt == PShelv)
        hist = i;
    i++;

    mi = gtk_menu_item_new_with_label("PostScript Century");
    gtk_widget_set_name(mi, "PostScript Century");
    gtk_widget_show(mi);
    gtk_menu_append(GTK_MENU(menu), mi);
    gtk_object_set_user_data(GTK_OBJECT(mi), (void*)PScentury);
    gtk_signal_connect(GTK_OBJECT(mi), "activate",
        GTK_SIGNAL_FUNC(hc_menu_proc), hc);
    if (hc->hc_textfmt == PScentury)
        hist = i;
    i++;

    mi = gtk_menu_item_new_with_label("PostScript Lucida");
    gtk_widget_set_name(mi, "PostScript Lucida");
    gtk_widget_show(mi);
    gtk_menu_append(GTK_MENU(menu), mi);
    gtk_object_set_user_data(GTK_OBJECT(mi), (void*)PSlucida);
    gtk_signal_connect(GTK_OBJECT(mi), "activate",
        GTK_SIGNAL_FUNC(hc_menu_proc), hc);
    if (hc->hc_textfmt == PSlucida)
        hist = i;
    i++;

    mi = gtk_menu_item_new_with_label("Plain Text");
    gtk_widget_set_name(mi, "Plain Text");
    gtk_widget_show(mi);
    gtk_menu_append(GTK_MENU(menu), mi);
    gtk_object_set_user_data(GTK_OBJECT(mi), (void*)PlainText);
    gtk_signal_connect(GTK_OBJECT(mi), "activate",
        GTK_SIGNAL_FUNC(hc_menu_proc), hc);
    if (hc->hc_textfmt == PlainText)
        hist = i;
    i++;

    /*
    mi = gtk_menu_item_new_with_label("Pretty Text");
    gtk_widget_set_name(mi, "Pretty Text");
    gtk_widget_show(mi);
    gtk_menu_append(GTK_MENU(menu), mi);
    gtk_object_set_user_data(GTK_OBJECT(mi), (void*)PrettyText);
    gtk_signal_connect(GTK_OBJECT(mi), "activate",
        GTK_SIGNAL_FUNC(hc_menu_proc), hc);
    if (hc->hc_textfmt == PrettyText)
        hist = i;
    i++;
    */

    mi = gtk_menu_item_new_with_label("HTML Text");
    gtk_widget_set_name(mi, "HTML Text");
    gtk_widget_show(mi);
    gtk_menu_append(GTK_MENU(menu), mi);
    gtk_object_set_user_data(GTK_OBJECT(mi), (void*)HtmlText);
    gtk_signal_connect(GTK_OBJECT(mi), "activate",
        GTK_SIGNAL_FUNC(hc_menu_proc), hc);
    if (hc->hc_textfmt == HtmlText)
        hist = i;
    i++;

    if (hist < 0) {
        hist = 0;
        hc->hc_textfmt = PStimes;
    }
    gtk_option_menu_set_history(GTK_OPTION_MENU(hc->hc_fontmenu), hist);
}


// Static function.
// Handler for the "font" menu.
//
void
GTKprintPopup::hc_menu_proc(GtkWidget *caller, void *client_data)
{
    HCtextType indx =
        (HCtextType)(long)gtk_object_get_user_data(GTK_OBJECT(caller));
    GTKprintPopup *hc = (GTKprintPopup*)client_data;
    hc->hc_textfmt = indx;
}


// Static function.
// Handler for the "format" menu.
//
void
GTKprintPopup::hc_formenu_proc(GtkWidget *caller, void *client_data)
{
    gtk_bag *wb = static_cast<gtk_bag*>(client_data);
    GTKprintPopup *hc = wb->HC();
    if (hc) {
        long index = (long)gtk_object_get_user_data(GTK_OBJECT(caller));
        hc_set_format(wb, index, false);
    }
}


#ifdef WIN32
// Static function.
// Handler for the printers menu.
//
void
GTKprintPopup::hc_prntmenu_proc(GtkWidget *caller, void*)
{
    curprinter = gtk_option_menu_get_history(GTK_OPTION_MENU(caller));
}
#endif


// Static function.
// Handler for the page size menu.
//
void
GTKprintPopup::hc_pagesize_proc(GtkWidget *caller, void *client_data)
{
    GTKprintPopup *hc = static_cast<gtk_bag*>(client_data)->HC();
    if (!hc)
        return;
    if (GTK_IS_RADIO_BUTTON(caller)) {
        // This is text-only mode, set the metric field for A4
        if (GTK_TOGGLE_BUTTON(caller)->active)
            hc->hc_metric = false;
        else
            hc->hc_metric = true;
        return;
    }
    long index = (long)gtk_object_get_user_data(GTK_OBJECT(caller));
    double shrink = 0.375 * 72;
    double width = pagesizes[index].width - 2*shrink;
    double height = pagesizes[index].height - 2*shrink;
    hc->hc_pgsindex = index;
    if (hc->hc_metric) {
        width *= MMPI;
        height *= MMPI;
        shrink *= MMPI;
    }
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(hc->hc_wid), width/72);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(hc->hc_hei), height/72);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(hc->hc_left), shrink/72);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(hc->hc_top), shrink/72);
}


// Static function.
void
GTKprintPopup::hc_metric_proc(GtkWidget *caller, void *client_data)
{
    GTKprintPopup *hc = static_cast<gtk_bag*>(client_data)->HC();
    if (!hc)
        return;
    bool wasmetric = hc->hc_metric;
    hc->hc_metric = GTK_TOGGLE_BUTTON(caller)->active;

    if (wasmetric != hc->hc_metric) {
        double d;

        d = gtk_spin_button_get_value_as_float(GTK_SPIN_BUTTON(hc->hc_wid));
        GtkAdjustment *adj = GTK_SPIN_BUTTON(hc->hc_wid)->adjustment;
        if (wasmetric) {
            d /= MMPI;
            adj = (GtkAdjustment*)gtk_adjustment_new(d, adj->lower/MMPI,
                adj->upper/MMPI, .1, 1.0, 0.0);
        }
        else {
            d *= MMPI;
            adj = (GtkAdjustment*)gtk_adjustment_new(d, adj->lower*MMPI,
                adj->upper*MMPI, .1, 1.0, 0.0);
        }
        gtk_spin_button_set_adjustment(GTK_SPIN_BUTTON(hc->hc_wid), adj);
        gtk_adjustment_value_changed(adj);

        d = gtk_spin_button_get_value_as_float(GTK_SPIN_BUTTON(hc->hc_hei));
        adj = GTK_SPIN_BUTTON(hc->hc_hei)->adjustment;
        if (wasmetric) {
            d /= MMPI;
            adj = (GtkAdjustment*)gtk_adjustment_new(d, adj->lower/MMPI,
                adj->upper/MMPI, .1, 1.0, 0.0);
        }
        else {
            d *= MMPI;
            adj = (GtkAdjustment*)gtk_adjustment_new(d, adj->lower*MMPI,
                adj->upper*MMPI, .1, 1.0, 0.0);
        }
        gtk_spin_button_set_adjustment(GTK_SPIN_BUTTON(hc->hc_hei), adj);
        gtk_adjustment_value_changed(adj);

        d = gtk_spin_button_get_value_as_float(GTK_SPIN_BUTTON(hc->hc_left));
        adj = GTK_SPIN_BUTTON(hc->hc_left)->adjustment;
        if (wasmetric) {
            d /= MMPI;
            adj = (GtkAdjustment*)gtk_adjustment_new(d, adj->lower/MMPI,
                adj->upper/MMPI, .1, 1.0, 0.0);
        }
        else {
            d *= MMPI;
            adj = (GtkAdjustment*)gtk_adjustment_new(d, adj->lower*MMPI,
                adj->upper*MMPI, .1, 1.0, 0.0);
        }
        gtk_spin_button_set_adjustment(GTK_SPIN_BUTTON(hc->hc_left), adj);
        gtk_adjustment_value_changed(adj);

        d = gtk_spin_button_get_value_as_float(GTK_SPIN_BUTTON(hc->hc_top));
        adj = GTK_SPIN_BUTTON(hc->hc_top)->adjustment;
        if (wasmetric) {
            d /= MMPI;
            adj = (GtkAdjustment*)gtk_adjustment_new(d, adj->lower*MMPI,
                adj->upper*MMPI, .1, 1.0, 0.0);
        }
        else {
            d *= MMPI;
            adj = (GtkAdjustment*)gtk_adjustment_new(d, adj->lower*MMPI,
                adj->upper*MMPI, .1, 1.0, 0.0);
        }
        gtk_spin_button_set_adjustment(GTK_SPIN_BUTTON(hc->hc_top), adj);
        gtk_adjustment_value_changed(adj);
    }
}


// Static function.
// Cancel callback.  Pop the widget down.
//
void
GTKprintPopup::hc_cancel_proc(GtkWidget*, void *client_data)
{
    gtk_bag *wb = static_cast<gtk_bag*>(client_data);
    GTKprintPopup *hc = wb->HC();
    if (hc) {
        hc->hc_active = true;
        hc_hcpopup(hc->hc_caller, wb, 0, HCgraphical, 0);
    }
}


// Static function.
// Frame callback.  This allows the user to define the area  of
// graphics to be printed.
//
void
GTKprintPopup::hc_frame_proc(GtkWidget *caller, void *client_data)
{
    GTKprintPopup *hc = static_cast<gtk_bag*>(client_data)->HC();
    if (hc) {
        if (hc->hc_cb && hc->hc_cb->hcframe)
            (*hc->hc_cb->hcframe)(HCframeCmd, (GRobject)caller, 0, 0, 0, 0,
                hc->hc_context);
    }
}


// Static function.
// Switch between portrait and landscape orientation.
//
void
GTKprintPopup::hc_port_proc(GtkWidget *caller, void *client_data)
{
    GTKprintPopup *hc = static_cast<gtk_bag*>(client_data)->HC();
    if (hc) {
        // The landscape button merely sets a flag passed to the driver.
        // It is up to the driver to respond appropriately.
        bool state = GTK_TOGGLE_BUTTON(caller)->active;
        if (state)
            hc->hc_orient &= ~HClandscape;
        else
            hc->hc_orient |= HClandscape;

        // See if we should swap the margin label.
        HCdesc *hcdesc = GRpkgIf()->HCof(hc->hc_fmt);
        if (hcdesc && (hcdesc->limits.flags & HClandsSwpYmarg)) {
            char *str;
            gtk_label_get(GTK_LABEL(hc->hc_ylabel), &str);
            if (!strcmp(str, "Top"))
                gtk_label_set_text(GTK_LABEL(hc->hc_ylabel), "Bottom");
            else
                gtk_label_set_text(GTK_LABEL(hc->hc_ylabel), "Top");
        }
    }
}


// Static function.
// If the "best fit" button is active, allow rotation of the image.
//
void
GTKprintPopup::hc_fit_proc(GtkWidget *caller, void *client_data)
{
    GTKprintPopup *hc = static_cast<gtk_bag*>(client_data)->HC();
    if (hc) {
        if (GTK_TOGGLE_BUTTON(caller)->active)
            hc->hc_orient |= HCbest;
        else
            hc->hc_orient &= ~HCbest;
    }
}


// Static function.
// Send the output to a file, rather than a printer.
//
void
GTKprintPopup::hc_tofile_proc(GtkWidget *caller, void *client_data)
{
    GTKprintPopup *hc = static_cast<gtk_bag*>(client_data)->HC();
    if (hc) {
        bool state = GTK_TOGGLE_BUTTON(caller)->active;
        hc->hc_tofile = state;
        const char *s = gtk_entry_get_text(GTK_ENTRY(hc->hc_cmdtxtbox));
        if (state) {
            gtk_label_set_text(GTK_LABEL(hc->hc_cmdlab), "File Name");
            delete [] hc->hc_cmdtext;
            hc->hc_cmdtext = lstring::copy(s);
            gtk_entry_set_text(GTK_ENTRY(hc->hc_cmdtxtbox), hc->hc_tofilename);
#ifdef WIN32
            gtk_widget_show(hc->hc_cmdtxtbox);
            gtk_widget_hide(hc->hc_prntmenu);
#endif
        }
        else {
#ifdef WIN32
            gtk_label_set_text(GTK_LABEL(hc->hc_cmdlab), "Printer Name");
            gtk_widget_hide(hc->hc_cmdtxtbox);
            gtk_widget_show(hc->hc_prntmenu);
#else
            gtk_label_set_text(GTK_LABEL(hc->hc_cmdlab), "Print Command");
            delete [] hc->hc_tofilename;
            hc->hc_tofilename = lstring::copy(s);
            gtk_entry_set_text(GTK_ENTRY(hc->hc_cmdtxtbox), hc->hc_cmdtext);
#endif
        }
    }
}


// Static function.
// Toggle display of the legend associated with the plot.
//
void
GTKprintPopup::hc_legend_proc(GtkWidget *caller, void *client_data)
{
    GTKprintPopup *hc = static_cast<gtk_bag*>(client_data)->HC();
    if (hc) {
        bool state = GTK_TOGGLE_BUTTON(caller)->active;
        hc->hc_legend = (state ? HClegOn : HClegOff);
    }
}


// Static function.
// Handle the auto-height and auto-width buttons.
//
void
GTKprintPopup::hc_auto_proc(GtkWidget *btn, void *client_data)
{
    GTKprintPopup *hc = static_cast<gtk_bag*>(client_data)->HC();
    if (!hc)
        return;
    HCdesc *hcdesc = GRpkgIf()->HCof(hc->hc_fmt);
    if (!hcdesc)
        return;
    if (btn == hc->hc_wlabel) {
        if (GRX->GetStatus(btn)) {
            hcdesc->last_w = gtk_spin_button_get_value_as_float(
                GTK_SPIN_BUTTON(hc->hc_wid));
            if (hc->hc_metric)
                hcdesc->last_w /= MMPI;
            hcdesc->defaults.defwidth = 0;
            gtk_widget_set_sensitive(hc->hc_wid, false);
            gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(hc->hc_wid), false);
            gtk_entry_set_text(GTK_ENTRY(hc->hc_wid), "auto");
            if (GRX->GetStatus(hc->hc_hlabel)) {
                GRX->Deselect(hc->hc_hlabel);
                gtk_widget_set_sensitive(hc->hc_hei, true);
                gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(hc->hc_hei), true);
                double h = hcdesc->last_h;
                if (h == 0.0)
                    h = hcdesc->limits.minheight;
                gtk_spin_button_set_value(GTK_SPIN_BUTTON(hc->hc_hei), MM(h));
            }
        }
        else {
            gtk_widget_set_sensitive(hc->hc_wid, true);
            gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(hc->hc_wid), true);
            double w = hcdesc->last_w;
            if (w == 0.0)
                w = hcdesc->limits.minwidth;
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(hc->hc_wid), MM(w));
        }
    }
    else if (btn == hc->hc_hlabel) {
        if (GRX->GetStatus(btn)) {
            hcdesc->last_h = gtk_spin_button_get_value_as_float(
                GTK_SPIN_BUTTON(hc->hc_hei));
            if (hc->hc_metric)
                hcdesc->last_h /= MMPI;
            hcdesc->defaults.defheight = 0;
            gtk_widget_set_sensitive(hc->hc_hei, false);
            gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(hc->hc_hei), false);
            gtk_entry_set_text(GTK_ENTRY(hc->hc_hei), "auto");
            if (GRX->GetStatus(hc->hc_wlabel)) {
                GRX->Deselect(hc->hc_wlabel);
                gtk_widget_set_sensitive(hc->hc_wid, true);
                gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(hc->hc_wid), true);
                double w = hcdesc->last_w;
                if (w == 0.0)
                    w = hcdesc->limits.minwidth;
                gtk_spin_button_set_value(GTK_SPIN_BUTTON(hc->hc_wid), MM(w));
            }
        }
        else {
            gtk_widget_set_sensitive(hc->hc_hei, true);
            gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(hc->hc_hei), true);
            double h = hcdesc->last_h;
            if (h == 0.0)
                h = hcdesc->limits.minheight;
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(hc->hc_hei), MM(h));
        }
    }
}


// Static function.
//
int
GTKprintPopup::hc_key_hdlr(GtkWidget*, GdkEvent *ev, void *client_data)
{
    gtk_bag *wb = static_cast<gtk_bag*>(client_data);
    GTKprintPopup *hc = wb->HC();
    if (hc && ev->key.keyval == GDK_Return) {
        hc->hc_go_proc(0, client_data);
        return (true);
    }
    return (false);
}


// Static function.
// Callback to actually generate the hardcopy.
//
void
GTKprintPopup::hc_go_proc(GtkWidget*, void *client_data)
{
    gtk_bag *wb = static_cast<gtk_bag*>(client_data);
    GTKprintPopup *hc = wb->HC();
    GRpkgIf()->HCabort(0);
    if (GRpkgIf()->HCof(hc->hc_fmt) &&
            (GRpkgIf()->HCof(hc->hc_fmt)->limits.flags & HCconfirmGo))
        hc_pop_message(wb);
    else
        hc_do_go(wb);

    // hc might be dead already
    if (wb->HC() && hc->hc_textmode != HCgraphical)
        hc_hcpopup(hc->hc_caller, wb, 0, HCgraphical, 0);
}


#define MAX_ARGS 20

// Static function.
void
GTKprintPopup::hc_do_go(gtk_bag *wb)
{
    static bool in_do_go;
    GTKprintPopup *hc = wb->HC();

#ifdef WIN32
    if (!hc->hc_printers && !hc->hc_tofile) {
        hc_pop_up_text(wb, "No default printer!", true);
        return;
    }
    const char *tmpstr;
    if (hc->hc_tofile)
        tmpstr = gtk_entry_get_text(GTK_ENTRY(hc->hc_cmdtxtbox));
    else {
        int prn =
            gtk_option_menu_get_history(GTK_OPTION_MENU(hc->hc_prntmenu));
        tmpstr = hc->hc_printers[prn];
    }
#else
    const char *tmpstr = gtk_entry_get_text(GTK_ENTRY(hc->hc_cmdtxtbox));
#endif

    if (!tmpstr || !*tmpstr) {
        if (hc->hc_tofile)
            hc_pop_up_text(wb, "No filename given!", true);
        else
#ifdef WIN32
            hc_pop_up_text(wb, "No printer given!", true);
#else
            hc_pop_up_text(wb, "No command given!", true);
#endif
        return;
    }
    if (hc->hc_tofile) {
        if (!access(tmpstr, F_OK) &&
                (!hc->hc_clobber_str || strcmp(hc->hc_clobber_str, tmpstr))) {
            // file already exists
            hc_pop_up_text(wb,
                "Operation will overwrite an existing file,\n"
                "press Print again to overwrite.", false);
            delete [] hc->hc_clobber_str;
            hc->hc_clobber_str = lstring::copy(tmpstr);
            return;
        }
    }
    delete [] hc->hc_clobber_str;
    hc->hc_clobber_str = 0;

    char *filename, buf[512];
    if (hc->hc_textmode != HCgraphical) {
        bool is_tmpfile = false;
        if (!hc->hc_tofile) {
            filename = filestat::make_temp("hc");
            is_tmpfile = true;
        }
        else
            filename = lstring::copy(tmpstr);

        if (hc->hc_textmode == HCtext) {
            bool err = false;
            FILE *fp = fopen(filename, "w");
            if (fp) {
                int length = text_get_length(wb->TextArea());
                int start = 0;
                for (;;) {
                    int end = start + 1024;
                    if (end > length)
                        end = length;
                    if (end == start)
                        break;
                    char *s = text_get_chars(wb->TextArea(), start, end);
                    if (fwrite(s, 1, end - start, fp) <
                            (unsigned)(end - start)) {
                        delete [] s;
                        fclose(fp);
                        err = true;
                        break;
                    }
                    delete [] s;
                    if (end - start < 1024)
                        break;
                    start = end;
                }
                fclose(fp);
                if (!hc->hc_tofile) {
                    int pid = hc_printit(tmpstr, filename, wb);

                    // Schedule tmp file for unlink.
                    if (is_tmpfile) {
                        if (pid > 0) {
                            char *mstr = filestat::schedule_rm_file(filename);
                            if (mstr) {
                                MsgList::Msg *msg = Mlist.find(pid);
                                if (msg)
                                    msg->add_msg(mstr);
                                delete [] mstr;
                            }
                        }
                        else
                            unlink(filename);
                    }
                }
                else
                    hc_pop_up_text(wb, "Text saved", false);
            }
            if (!fp || err)
                hc_pop_up_text(wb, "write error: text not saved", true);
        }
        else if (hc->hc_textmode == HCtextPS) {
            char *text = 0;
            int fontcode = 0;
            switch (hc->hc_textfmt) {
            case PlainText:
            case PrettyText:
                text = wb->GetPlainText();
                break;
            case PSlucida:
                fontcode++;
                // fallthrough
            case PScentury:
                fontcode++;
                // fallthrough
            case PShelv:
                fontcode++;
                // fallthrough
            case PStimes:
                text = wb->GetPostscriptText(fontcode, 0, 0, true,
                    hc->hc_metric);
                break;
            case HtmlText:
                text = wb->GetHtmlText();
                // returns pointer to internal data
                break;
            }

            if (!text) {
                hc_pop_up_text(wb, "Internal error: no text found", true);
                delete [] filename;
                return;
            }
            FILE *fp = fopen(filename, "w");
            if (!fp) {
                sprintf(buf, "Error: can't open file %s", filename);
                hc_pop_up_text(wb, buf, true);
                delete [] filename;
                if (hc->hc_textfmt != HtmlText)
                    delete [] text;
                return;
            }
            if (fputs(text, fp) == EOF) {
                hc_pop_up_text(wb, "Internal error: block write error", true);
                delete [] filename;
                if (hc->hc_textfmt != HtmlText)
                    delete [] text;
                fclose(fp);
                return;
            }
            fclose(fp);
            if (hc->hc_textfmt != HtmlText)
                delete [] text;
            if (!hc->hc_tofile) {
                int pid = hc_printit(tmpstr, filename, wb);

                // Schedule tmp file for unlink.
                if (is_tmpfile) {
                    if (pid > 0) {
                        char *mstr = filestat::schedule_rm_file(filename);
                        if (mstr) {
                            MsgList::Msg *msg = Mlist.find(pid);
                            if (msg)
                                msg->add_msg(mstr);
                            delete [] mstr;
                        }
                    }
                    else
                        unlink(filename);
                }
            }
            else
                hc_pop_up_text(wb, "Text saved", false);
        }
        delete [] filename;
        return;
    }

    if (in_do_go) {
        hc_pop_up_text(wb, "I'm busy, please wait.", true);
        return;
    }
    in_do_go = true;

    HCdesc *hcdesc = GRpkgIf()->HCof(hc->hc_fmt);
    if (!hcdesc)
        return;
    double w = 0.0;
    if (!(hcdesc->limits.flags & HCdontCareWidth)) {
        if (!GRX->GetStatus(hc->hc_wlabel)) {
            w = gtk_spin_button_get_value_as_float(
                GTK_SPIN_BUTTON(hc->hc_wid));
            if (hc->hc_metric)
                w /= MMPI;
        }
    }
    double h = 0.0;
    if (!(hcdesc->limits.flags & HCdontCareHeight)) {
        if (!GRX->GetStatus(hc->hc_hlabel)) {
            h = gtk_spin_button_get_value_as_float(
                GTK_SPIN_BUTTON(hc->hc_hei));
            if (hc->hc_metric)
                h /= MMPI;
        }
    }
    double x = 0.0;
    if (!(hcdesc->limits.flags & HCdontCareXoff)) {
        x = gtk_spin_button_get_value_as_float(
            GTK_SPIN_BUTTON(hc->hc_left));
        if (hc->hc_metric)
            x /= MMPI;
    }
    double y = 0.0;
    if (!(hcdesc->limits.flags & HCdontCareYoff)) {
        y = gtk_spin_button_get_value_as_float(
            GTK_SPIN_BUTTON(hc->hc_top));
        if (hc->hc_metric)
            y /= MMPI;
    }
    bool is_tmpfile = false;
    if (!hc->hc_tofile) {
        filename = filestat::make_temp("hc");
        is_tmpfile = true;
    }
    else
        filename = lstring::copy(tmpstr);
    int resol = 0;
    if (hcdesc->limits.resols)
        sscanf(hcdesc->limits.resols[hc->hc_resol], "%d", &resol);
    sprintf(buf, hcdesc->fmtstring, filename, resol, w, h, x, y);
    if (hc->hc_orient & HClandscape)
        strcat(buf, " -l");
    char *cmdstr = lstring::copy(buf);
    char *argv[MAX_ARGS];
    int argc;
    hc_mkargv(&argc, argv, cmdstr);

#ifdef WIN32
    // Butt-ugly hack to pass printer name and media index to
    // Windows Native driver.
    int media = gtk_option_menu_get_history(GTK_OPTION_MENU(hc->hc_pgsmenu));
    int prnt = gtk_option_menu_get_history(GTK_OPTION_MENU(hc->hc_prntmenu));
    if (!strcmp(hcdesc->keyword, "windows_native")) {
        argv[argc++] = (char*)"-nat";
        argv[argc++] = (char*)hc->hc_printers[prnt];
        argv[argc++] = (char*)media;
    }
#endif

    HCswitchErr err =
        GRpkgIf()->SwitchDev(hcdesc->drname, &argc, argv);
    if (err == HCSinhc)
        hc_pop_up_text(wb, "Internal error - aborted", true);
    else if (err == HCSnotfnd) {
        sprintf(buf, "No hardcopy driver named %s available",
            hcdesc->drname);
        hc_pop_up_text(wb, buf, true);
    }
    else if (err == HCSinit)
        hc_pop_up_text(wb, "Init callback failed - aborted", true);
    else {
        bool ok = true;
        // hc might be freed during hcgo
        bool tofile = hc->hc_tofile;
        char *cmd =
#ifdef WIN32
            lstring::copy(tmpstr);
#else
            lstring::copy(gtk_entry_get_text(GTK_ENTRY(hc->hc_cmdtxtbox)));
#endif

        if (hc->hc_cb && hc->hc_cb->hcgo) {
            HCorientFlags ot = hc->hc_orient & HCbest;
            // pass the landscape flag only if the driver can't rotate
            if ((hc->hc_orient & HClandscape) &&
                    (hcdesc->limits.flags & HCnoCanRotate))
                ot |= HClandscape;
            if ((*hc->hc_cb->hcgo)(ot, hc->hc_legend, hc->hc_context)) {
                if (GRpkgIf()->HCaborted()) {
                    sprintf(buf, "Terminated: %s.", GRpkgIf()->HCabortMsg());
                    hc_pop_up_text(wb, buf, true);
                }
                else
                    hc_pop_up_text(wb,
                        "Error encountered in hardcopy generation - aborted",
                        true);
                ok = false;
                unlink(filename);
            }
        }
        GRpkgIf()->SwitchDev(0, 0, 0);
        if (ok) {
            if (!tofile) {
                int pid = hc_printit(cmd, filename, wb);

                // Schedule tmp file for unlink.
                if (is_tmpfile) {
                    if (pid > 0) {
                        char *mstr = filestat::schedule_rm_file(filename);
                        if (mstr) {
                            MsgList::Msg *msg = Mlist.find(pid);
                            if (msg)
                                msg->add_msg(mstr);
                            delete [] mstr;
                        }
                    }
#ifndef KEEP_TMPFILE
                    else
                        unlink(filename);
#endif
                }
            }
            else
                hc_pop_up_text(wb, "Command executed successfully", false);
        }
        delete [] cmd;
    }
    delete [] cmdstr;
    delete [] filename;
    in_do_go = false;
}


// Static function, returns child pid if successful.
//
int
GTKprintPopup::hc_printit(const char *str, const char *filename, gtk_bag *wb)
{
    if (!str || !*filename)
        return (-1);

#ifdef WIN32
    const char *err = msw::RawFileToPrinter(str, filename);
    char buf[256];
    if (err) {
        sprintf(buf, "Print spooler reported error:\n%s.", err);
        hc_pop_up_text(wb, buf, true);
    }
    else {
        sprintf(buf, "Print job submitted, no errors.");
        hc_pop_up_text(wb, buf, false);
    }
#ifndef KEEP_TMPFILE
    filestat::queue_deletion(filename);
#endif

#else
    // Check for '%s' and substitute filename, otherwise
    // cat the filename.
    bool submade = false;
    const char *s = str;
    char buf[256];
    char *t = buf;
    while (*s) {
        if (*s == '%' && *(s+1) == 's') {
            strcpy(t, filename);
            while (*t)
                t++;
            s += 2;
            submade = true;
        }
        else
            *t++ = *s++;
    }
    if (!submade) {
        *t++ = ' ';
        strcpy(t, filename);
    }
    else
        *t = '\0';
    int pid = fork();
    if (pid == -1) {
        hc_pop_up_text(wb, "Command failed, error on fork.", true);
        return (-1);
    }
    if (pid) {
        Mlist.add(wb, pid);
        Proc()->RegisterChildHandler(pid, hc_proc_hdlr, 0);
        return (pid);
    }
    if (gtk_main_level())
        gtk_main_quit();  // exit graphics in child
    close(GRX->ConnectFd());
    execl("/bin/sh", "sh", "-c", buf, (char*)0);
    _exit(127);
#endif
    return (0);
}


// Static function.
// Pop up the message in an idle proc.  This *might* fix reports of
// program hangs after hard copy generation using xpp.
//
int
GTKprintPopup::hc_msg_idle_proc(void *arg)
{
    pid_t pid = (pid_t)(long)arg;
    MsgList::Msg *msg = Mlist.remove(pid);
    if (msg) {
        msg->show();
        delete msg;
    }
    return (false);
}


// Static function.
void
GTKprintPopup::hc_proc_hdlr(int pid, int status, void*)
{
#ifdef HAVE_SYS_WAIT_H
    bool err = false;
    char buf[128];
    *buf = '\0';
    if (WIFEXITED(status)) {
        sprintf(buf, "Command exited ");
        if (WEXITSTATUS(status)) {
            sprintf(buf + strlen(buf), "with error status %d.",
                WEXITSTATUS(status));
            err = true;
        }
        else
            strcat(buf, "normally.");
    }
    else if (WIFSIGNALED(status)) {
        sprintf(buf, "Command exited on signal %d.",
            WIFSIGNALED(status));
        err = true;
    }
    if (*buf) {
        MsgList::Msg *msg = Mlist.find(pid);
        if (msg) {
            msg->add_msg(buf);
            msg->set_error(err);
            gtk_idle_add((GtkFunction)hc_msg_idle_proc, (void*)(long)pid);
        }
    }
#else
    (void)pid;
    (void)status;
#endif
}


// Static function.
// Set the printer resolution.  This cycles through a list of
// possibilities.
//
void
GTKprintPopup::hc_resol_proc(GtkWidget *caller, void *client_data)
{
    GTKprintPopup *hc = static_cast<gtk_bag*>(client_data)->HC();
    if (hc) {
        long i = (long)gtk_object_get_user_data(GTK_OBJECT(caller));
        if (i >= 0 && i < 100)
            // sanity check
            hc->hc_resol = i;
    }
}


// Static function.
// Handler for button presses within the widget not caught by subwidgets,
// for help mode.
//
void
GTKprintPopup::hc_help_proc(GtkWidget*, void *client_data)
{
    if (GRX->MainFrame())
        GRX->MainFrame()->PopUpHelp("hcopypanel");
    else {
        gtk_bag *w = static_cast<gtk_bag*>(client_data);
        w->PopUpHelp("hcopypanel");
    }
}


// Static function.
// Make an argv-type string array from string str.
//
void
GTKprintPopup::hc_mkargv(int *acp, char **av, char *str)
{
    char *s = str;
    int j = 0;
    for (;;) {
        while (isspace(*s)) s++;
        if (!*s) {
            *acp = j;
            return;
        }
        char *t = s;
        while (*t && !isspace(*t)) t++;
        if (*t)
            *t++ = '\0';
        av[j++] = s;
        s = t;
    }
}


// Static function.
// Sanity check limits/defaults.
//
void
GTKprintPopup::hc_checklims(HCdesc *hcdesc)
{
    if (!hcdesc)
        return;
    if (hcdesc->limits.minwidth > hcdesc->limits.maxwidth) {
        double tmp = hcdesc->limits.minwidth;
        hcdesc->limits.minwidth = hcdesc->limits.maxwidth;
        hcdesc->limits.maxwidth = tmp;
    }
    if (hcdesc->defaults.defwidth != 0.0 &&
            hcdesc->defaults.defwidth < hcdesc->limits.minwidth)
        hcdesc->defaults.defwidth = hcdesc->limits.minwidth;
    if (hcdesc->defaults.defwidth > hcdesc->limits.maxwidth)
        hcdesc->defaults.defwidth = hcdesc->limits.maxwidth;

    if (hcdesc->limits.minheight > hcdesc->limits.maxheight) {
        double tmp = hcdesc->limits.minheight;
        hcdesc->limits.minheight = hcdesc->limits.maxheight;
        hcdesc->limits.maxheight = tmp;
    }
    if (hcdesc->defaults.defheight != 0.0 &&
            hcdesc->defaults.defheight < hcdesc->limits.minheight)
        hcdesc->defaults.defheight = hcdesc->limits.minheight;
    if (hcdesc->defaults.defheight > hcdesc->limits.maxheight)
        hcdesc->defaults.defheight = hcdesc->limits.maxheight;

    if (hcdesc->defaults.defwidth == 0.0 && hcdesc->defaults.defheight == 0.0)
        hcdesc->defaults.defwidth = hcdesc->limits.minwidth;

    if (hcdesc->limits.minxoff > hcdesc->limits.maxxoff) {
        double tmp = hcdesc->limits.minxoff;
        hcdesc->limits.minxoff = hcdesc->limits.maxxoff;
        hcdesc->limits.maxxoff = tmp;
    }
    if (hcdesc->limits.minxoff > hcdesc->limits.maxwidth)
        hcdesc->limits.minxoff = 0.0;
    if (hcdesc->limits.maxxoff > hcdesc->limits.maxwidth)
        hcdesc->limits.minxoff = hcdesc->limits.maxwidth;
    if (hcdesc->defaults.defxoff < hcdesc->limits.minxoff)
        hcdesc->defaults.defxoff = hcdesc->limits.minxoff;
    if (hcdesc->defaults.defxoff > hcdesc->limits.maxxoff)
        hcdesc->defaults.defxoff = hcdesc->limits.maxxoff;

    if (hcdesc->limits.minyoff > hcdesc->limits.maxyoff) {
        double tmp = hcdesc->limits.minyoff;
        hcdesc->limits.minyoff = hcdesc->limits.maxyoff;
        hcdesc->limits.maxyoff = tmp;
    }
    if (hcdesc->limits.minyoff > hcdesc->limits.maxheight)
        hcdesc->limits.minyoff = 0.0;
    if (hcdesc->limits.maxyoff > hcdesc->limits.maxheight)
        hcdesc->limits.minyoff = hcdesc->limits.maxheight;
    if (hcdesc->defaults.defyoff < hcdesc->limits.minyoff)
        hcdesc->defaults.defyoff = hcdesc->limits.minyoff;
    if (hcdesc->defaults.defyoff > hcdesc->limits.maxyoff)
        hcdesc->defaults.defyoff = hcdesc->limits.maxyoff;
}


// Static function.
void
GTKprintPopup::hc_set_sens(GTKprintPopup *hc, unsigned int word)
{
    if (hc->hc_resmenu) {
        if (word & HCfixedResol)
            gtk_widget_set_sensitive(hc->hc_resmenu, false);
        else
            gtk_widget_set_sensitive(hc->hc_resmenu, true);
    }
    if (hc->hc_left) {
        if (word & HCdontCareXoff) {
            gtk_widget_set_sensitive(hc->hc_xlabel, false);
            gtk_widget_set_sensitive(hc->hc_left, false);
        }
        else {
            gtk_widget_set_sensitive(hc->hc_xlabel, true);
            gtk_widget_set_sensitive(hc->hc_left, true);
        }
    }
    if (hc->hc_top) {
        if (word & HCdontCareYoff) {
            gtk_widget_set_sensitive(hc->hc_ylabel, false);
            gtk_widget_set_sensitive(hc->hc_top, false);
        }
        else {
            gtk_widget_set_sensitive(hc->hc_ylabel, true);
            gtk_widget_set_sensitive(hc->hc_top, true);
        }
    }
    if (hc->hc_wid) {
        if (word & HCdontCareWidth) {
            gtk_widget_set_sensitive(hc->hc_wlabel, false);
            gtk_widget_set_sensitive(hc->hc_wid, false);
        }
        else {
            gtk_widget_set_sensitive(hc->hc_wlabel, true);
            gtk_widget_set_sensitive(hc->hc_wid, true);
        }
    }
    if (hc->hc_hei) {
        if (word & HCdontCareHeight) {
            gtk_widget_set_sensitive(hc->hc_hlabel, false);
            gtk_widget_set_sensitive(hc->hc_hei, false);
        }
        else {
            gtk_widget_set_sensitive(hc->hc_hlabel, true);
            gtk_widget_set_sensitive(hc->hc_hei, true);
        }
    }

    if (hc->hc_pgsmenu) {
        if ((word & HCdontCareXoff) && (word & HCdontCareYoff) &&
                (word & HCdontCareWidth) && (word & HCdontCareHeight)) {
            gtk_widget_set_sensitive(hc->hc_pgsmenu, false);
            gtk_widget_set_sensitive(hc->hc_metbtn, false);
        }
        else {
            gtk_widget_set_sensitive(hc->hc_pgsmenu, true);
            gtk_widget_set_sensitive(hc->hc_metbtn, true);
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(hc->hc_metbtn),
                hc->hc_metric);
            gtk_option_menu_set_history(GTK_OPTION_MENU(hc->hc_pgsmenu),
                hc->hc_pgsindex);
        }
    }

    if (hc->hc_landsbtn) {
        if (word & HCnoLandscape) {
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(hc->hc_portbtn),
                true);
            gtk_widget_set_sensitive(hc->hc_landsbtn, false);
        }
        else {
            gtk_widget_set_sensitive(hc->hc_landsbtn, true);
            if (hc->hc_orient & HClandscape) {
                gtk_toggle_button_set_active(
                    GTK_TOGGLE_BUTTON(hc->hc_portbtn), false);
                gtk_toggle_button_set_active(
                    GTK_TOGGLE_BUTTON(hc->hc_landsbtn), true);
            }
            else {
                gtk_toggle_button_set_active(
                    GTK_TOGGLE_BUTTON(hc->hc_portbtn), true);
                gtk_toggle_button_set_active(
                    GTK_TOGGLE_BUTTON(hc->hc_landsbtn), false);
            }
        }
    }

    if (hc->hc_fitbtn) {
        if (word & HCnoBestOrient) {
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(hc->hc_fitbtn),
                false);
            gtk_widget_set_sensitive(hc->hc_fitbtn, false);
        }
        else {
            gtk_widget_set_sensitive(hc->hc_fitbtn, true);
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(hc->hc_fitbtn),
                (hc->hc_orient & HCbest));
        }
    }
    if (hc->hc_legbtn) {
        if (hc->hc_legend == HClegNone) {
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(hc->hc_legbtn),
                false);
            gtk_widget_set_sensitive(hc->hc_legbtn, false);
            hc->hc_legend = HClegNone;
        }
        else {
            gtk_widget_set_sensitive(hc->hc_legbtn, true);
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(hc->hc_legbtn),
                    hc->hc_legend != HClegOff);
        }
    }
    if (hc->hc_tofbtn) {
        if (word & HCfileOnly) {
            if (!(hc->hc_tofbak & 1))
                hc->hc_tofbak = hc->hc_tofile ? 3 : 1;
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(hc->hc_tofbtn),
                true);
            // hc->hc_tofile is now true
            gtk_widget_set_sensitive(hc->hc_tofbtn, false);
#ifdef WIN32
            gtk_widget_show(hc->hc_cmdtxtbox);
            gtk_widget_hide(hc->hc_prntmenu);
#endif
        }
        else {
            if (hc->hc_tofbak & 1) {
                hc->hc_tofile = (hc->hc_tofbak & 2);
                hc->hc_tofbak = 0;
            }
            gtk_widget_set_sensitive(hc->hc_tofbtn, true);
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(hc->hc_tofbtn),
                hc->hc_tofile);
#ifdef WIN32
            if (hc->hc_tofile) {
                gtk_widget_show(hc->hc_cmdtxtbox);
                gtk_widget_hide(hc->hc_prntmenu);
            }
            else {
                gtk_widget_hide(hc->hc_cmdtxtbox);
                gtk_widget_show(hc->hc_prntmenu);
            }
#endif
        }
    }
}


// Popup for the go button.  Consists of an "abort" button to stop
// the hardcopy generation, and an area for messages from the driver.

// Static function.
void
GTKprintPopup::hc_pop_message(gtk_bag *wb)
{
    if (GP)
        return;
    else
        GP = new sGP;
    GP->popup = gtk_NewPopup(wb, "Message", hc_go_cancel_proc, wb);
    gtk_window_set_default_size(GTK_WINDOW(GP->popup), 300, 150);

    GtkWidget *form = gtk_table_new(1, 3, false);
    gtk_widget_show(form);
    gtk_container_add(GTK_CONTAINER(GP->popup), form);

    GtkWidget *label = gtk_label_new(0);
    gtk_widget_show(label);
    GP->label = label;

    GtkWidget *frame = gtk_frame_new("Status");
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), label);
    gtk_table_attach(GTK_TABLE(form), frame, 0, 1, 0, 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);

    GtkWidget *vsep = gtk_vseparator_new();
    gtk_widget_show(vsep);
    gtk_table_attach(GTK_TABLE(form), vsep, 0, 1, 1, 2,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    GtkWidget *button = gtk_button_new_with_label("Abort");
    gtk_widget_set_name(button, "Abort");
    gtk_object_set_data(GTK_OBJECT(button), "abort", (void*)1);
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(hc_go_abort_proc), wb);
    gtk_table_attach(GTK_TABLE(form), button, 0, 1, 2, 3,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    GdkRectangle rect;
    ShellGeometry(wb->HC()->hc_popup, 0, &rect);
    gtk_widget_set_uposition(GP->popup, rect.x + rect.width, rect.y);
    gtk_widget_show(GP->popup);

    gtk_idle_add((GtkFunction)hc_go_idle_proc, wb);
}


// Static function.
// Do the hardcopy.
//
int
GTKprintPopup::hc_go_idle_proc(void *client_data)
{
    hc_do_go(static_cast<gtk_bag*>(client_data));
    hc_go_cancel_proc(0, 0);
    return (false);
}


// Static function.
void
GTKprintPopup::hc_go_cancel_proc(GtkWidget*, void*)
{
    if (GP) {
        GtkWidget *pop = GP->popup;
        delete GP;
        GP = 0;
        gtk_widget_destroy(pop);
    }
}


// Static function.
// Abort the current hardcopy and/or pop down.
///
void
GTKprintPopup::hc_go_abort_proc(GtkWidget*, void*)
{
    if (GP) {
        GRX->HCmessage("ABORTED");
        GRpkgIf()->HCabort("User aborted");
    }
}


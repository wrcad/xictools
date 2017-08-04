
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
#include "tech_ldb3d.h"
#include "extif.h"
#include "scedif.h"
#include "oa_if.h"
#include "si_lisp.h"
#include "dsp_inlines.h"
#include "gtkmain.h"
#include "gtkinlines.h"
#include "errorlog.h"
#include "filestat.h"


//--------------------------------------------------------------------
// Pop-up to control program logging and debugging flags.
//
// Help system keywords used:
// xic:dblog

namespace {
    namespace gtkdbgflg {
        struct sDbgFlg
        {
            sDbgFlg(void*);
            ~sDbgFlg();

            void update();

            GtkWidget *shell() { return (df_popup); }

        private:
            static void df_cancel_proc(GtkWidget*, void*);
            static void df_action(GtkWidget*, void*);
            static int df_focus_out_proc(GtkWidget*, GdkEvent*, void*);

            GRobject df_caller;
            GtkWidget *df_popup;
            GtkWidget *df_sel;
            GtkWidget *df_undo;
            GtkWidget *df_ldb3d;
            GtkWidget *df_rlsolv;
            GtkWidget *df_fname;

            GtkWidget *df_lisp;
            GtkWidget *df_connect;
            GtkWidget *df_rlsolvlog;
            GtkWidget *df_group;
            GtkWidget *df_extract;
            GtkWidget *df_assoc;
            GtkWidget *df_verbose;
            GtkWidget *df_load;
            GtkWidget *df_net;
            GtkWidget *df_pcell;
        };

        sDbgFlg *DF;
    }
}

using namespace gtkdbgflg;


void
cMain::PopUpDebugFlags(GRobject caller, ShowMode mode)
{
    if (!GRX || !mainBag())
        return;
    if (mode == MODE_OFF) {
        delete DF;
        return;
    }
    if (mode == MODE_UPD) {
        if (DF)
            DF->update();
        return;
    }
    if (DF)
        return;

    new sDbgFlg(caller);
    if (!DF->shell()) {
        delete DF;
        return;
    }
    gtk_window_set_transient_for(GTK_WINDOW(DF->shell()),
        GTK_WINDOW(mainBag()->Shell()));

    GRX->SetPopupLocation(GRloc(), DF->shell(), mainBag()->Viewport());
    gtk_widget_show(DF->shell());
}


sDbgFlg::sDbgFlg(GRobject c)
{
    DF = this;
    df_caller = c;
    df_popup = 0;
    df_sel = 0;
    df_undo = 0;
    df_ldb3d = 0;
    df_rlsolv = 0;
    df_fname = 0;
    df_lisp = 0;
    df_connect = 0;
    df_rlsolvlog = 0;
    df_group = 0;
    df_extract = 0;
    df_assoc = 0;
    df_verbose = 0;
    df_load = 0;
    df_net = 0;
    df_pcell = 0;

    df_popup = gtk_NewPopup(0, "Logging Options", df_cancel_proc, 0);
    if (!df_popup)
        return;
    gtk_window_set_resizable(GTK_WINDOW(df_popup), false);

    GtkWidget *form = gtk_table_new(2, 1, false);
    gtk_widget_show(form);
    gtk_container_set_border_width(GTK_CONTAINER(form), 2);
    gtk_container_add(GTK_CONTAINER(df_popup), form);
    int rowcnt = 0;

    //
    // label in frame plus help btn
    //
    GtkWidget *row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);
    GtkWidget *label = gtk_label_new("Enable debugging messages");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_box_pack_start(GTK_BOX(row), label, true, true, 0);
    GtkWidget *button = gtk_button_new_with_label("Help");
    gtk_widget_set_name(button, "Help");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(df_action), 0);
    gtk_box_pack_end(GTK_BOX(row), button, false, false, 0);
    gtk_table_attach(GTK_TABLE(form), row, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // check boxes
    //
    button = gtk_check_button_new_with_label("Selection list consistency");
    gtk_widget_set_name(button, "sel");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(df_action), 0);
    gtk_table_attach(GTK_TABLE(form), button, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;
    df_sel = button;

    button = gtk_check_button_new_with_label("Undo/redo list processing");
    gtk_widget_set_name(button, "undo");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(df_action), 0);
    gtk_table_attach(GTK_TABLE(form), button, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;
    df_undo = button;

    if (ExtIf()->hasExtract()) {
        button = gtk_check_button_new_with_label(
            "3-D processing (cross sections, C and LR extract)");
    }
    else {
        button = gtk_check_button_new_with_label(
            "3-D processing for cross sections");
    }
    gtk_widget_set_name(button, "ldb3d");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(df_action), 0);
    gtk_table_attach(GTK_TABLE(form), button, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;
    df_ldb3d = button;

    if (ExtIf()->hasExtract()) {
        button = gtk_check_button_new_with_label(
            "Net resistance solver");
        gtk_widget_set_name(button, "rlsolv");
        gtk_widget_show(button);
        gtk_signal_connect(GTK_OBJECT(button), "clicked",
            GTK_SIGNAL_FUNC(df_action), 0);
        gtk_table_attach(GTK_TABLE(form), button, 0, 2, rowcnt, rowcnt+1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
        rowcnt++;
        df_rlsolv = button;
    }

    //
    // file name entry
    //
    label = gtk_label_new("Message file:");
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(form), label, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)0, (GtkAttachOptions)0, 2, 2);
    df_fname = gtk_entry_new();
    gtk_widget_show(df_fname);
    gtk_table_attach(GTK_TABLE(form), df_fname, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    gtk_signal_connect(GTK_OBJECT(df_fname), "focus-out-event",
        GTK_SIGNAL_FUNC(df_focus_out_proc), 0);

    GtkWidget *sep = gtk_hseparator_new();
    gtk_widget_show(sep);
    gtk_table_attach(GTK_TABLE(form), sep, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // Log file creation
    //
    label = gtk_label_new("Enable optional log files");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_table_attach(GTK_TABLE(form), label, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    button = gtk_check_button_new_with_label(
        "Lisp parser (lisp.log)");
    gtk_widget_set_name(button, "lisp");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(df_action), 0);
    gtk_table_attach(GTK_TABLE(form), button, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;
    df_lisp = button;

    if (ScedIf()->hasSced()) {
        button = gtk_check_button_new_with_label(
            "Schematic connectivity (connect.log)");
        gtk_widget_set_name(button, "connect");
        gtk_widget_show(button);
        gtk_signal_connect(GTK_OBJECT(button), "clicked",
            GTK_SIGNAL_FUNC(df_action), 0);
        gtk_table_attach(GTK_TABLE(form), button, 0, 2, rowcnt, rowcnt+1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
        rowcnt++;
        df_connect = button;
    }

    if (ExtIf()->hasExtract()) {
        button = gtk_check_button_new_with_label(
            "Resistance/inductance extraction (rlsolver.log)");
        gtk_widget_set_name(button, "rlsolvlog");
        gtk_widget_show(button);
        gtk_signal_connect(GTK_OBJECT(button), "clicked",
            GTK_SIGNAL_FUNC(df_action), 0);
        gtk_table_attach(GTK_TABLE(form), button, 0, 2, rowcnt, rowcnt+1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
        rowcnt++;
        df_rlsolvlog = button;

        GtkWidget *frame = gtk_frame_new("Grouping/Extraction/Association");
        gtk_widget_show(frame);
        row = gtk_hbox_new(false, 2);
        gtk_widget_show(row);
        gtk_container_add(GTK_CONTAINER(frame), row);

        button = gtk_check_button_new_with_label("Group");
        gtk_widget_set_name(button, "group");
        gtk_widget_show(button);
        gtk_signal_connect(GTK_OBJECT(button), "clicked",
            GTK_SIGNAL_FUNC(df_action), 0);
        gtk_box_pack_start(GTK_BOX(row), button, true, true, 0);
        df_group = button;

        button = gtk_check_button_new_with_label("Extract");
        gtk_widget_set_name(button, "extract");
        gtk_widget_show(button);
        gtk_signal_connect(GTK_OBJECT(button), "clicked",
            GTK_SIGNAL_FUNC(df_action), 0);
        gtk_box_pack_start(GTK_BOX(row), button, true, true, 0);
        df_extract = button;

        button = gtk_check_button_new_with_label("Assoc");
        gtk_widget_set_name(button, "assoc");
        gtk_widget_show(button);
        gtk_signal_connect(GTK_OBJECT(button), "clicked",
            GTK_SIGNAL_FUNC(df_action), 0);
        gtk_box_pack_start(GTK_BOX(row), button, true, true, 0);
        df_assoc = button;

        button = gtk_check_button_new_with_label("Verbose");
        gtk_widget_set_name(button, "verbose");
        gtk_widget_show(button);
        gtk_signal_connect(GTK_OBJECT(button), "clicked",
            GTK_SIGNAL_FUNC(df_action), 0);
        gtk_box_pack_start(GTK_BOX(row), button, true, true, 0);
        df_verbose = button;

        gtk_table_attach(GTK_TABLE(form), frame, 0, 2, rowcnt, rowcnt+1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
        rowcnt++;
    }

    if (OAif()->hasOA()) {
        GtkWidget *frame = gtk_frame_new("OpenAccess (oa_debug.log)");
        gtk_widget_show(frame);
        row = gtk_hbox_new(false, 2);
        gtk_widget_show(row);
        gtk_container_add(GTK_CONTAINER(frame), row);

        button = gtk_check_button_new_with_label("Load");
        gtk_widget_set_name(button, "load");
        gtk_widget_show(button);
        gtk_signal_connect(GTK_OBJECT(button), "clicked",
            GTK_SIGNAL_FUNC(df_action), 0);
        gtk_box_pack_start(GTK_BOX(row), button, true, true, 0);
        df_load = button;

        button = gtk_check_button_new_with_label("Net");
        gtk_widget_set_name(button, "net");
        gtk_widget_show(button);
        gtk_signal_connect(GTK_OBJECT(button), "clicked",
            GTK_SIGNAL_FUNC(df_action), 0);
        gtk_box_pack_start(GTK_BOX(row), button, true, true, 0);
        df_net = button;

        button = gtk_check_button_new_with_label("PCell");
        gtk_widget_set_name(button, "pcell");
        gtk_widget_show(button);
        gtk_signal_connect(GTK_OBJECT(button), "clicked",
            GTK_SIGNAL_FUNC(df_action), 0);
        gtk_box_pack_start(GTK_BOX(row), button, true, true, 0);
        df_pcell = button;

        gtk_table_attach(GTK_TABLE(form), frame, 0, 2, rowcnt, rowcnt+1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
        rowcnt++;
    }

    //
    // dismiss button
    //
    button = gtk_button_new_with_label("Dismiss");
    gtk_widget_set_name(button, "Dismiss");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(df_cancel_proc), 0);

    gtk_table_attach(GTK_TABLE(form), button, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);
    gtk_window_set_focus(GTK_WINDOW(df_popup), button);

    update();
}


sDbgFlg::~sDbgFlg()
{
    DF = 0;
    if (df_caller)
        GRX->Deselect(df_caller);
    if (df_popup)
        gtk_widget_destroy(df_popup);
}


void
sDbgFlg::update()
{
    unsigned int flags = XM()->DebugFlags();
    GRX->SetStatus(df_sel, flags & DBG_SELECT);
    GRX->SetStatus(df_undo, flags & DBG_UNDOLIST);
    GRX->SetStatus(df_ldb3d, Ldb3d::logging());
    if (ExtIf()->hasExtract()) {
        GRX->SetStatus(df_rlsolv, ExtIf()->rlsolverMsgs());
    }

    const char *fn = XM()->DebugFile();
    if (!fn)
        fn = "";
    gtk_entry_set_text(GTK_ENTRY(df_fname), fn);
    GRX->SetStatus(df_lisp, cLispEnv::is_logging());
    GRX->SetStatus(df_connect, ScedIf()->logConnect());
    if (ExtIf()->hasExtract()) {
        GRX->SetStatus(df_rlsolvlog, ExtIf()->logRLsolver());
        GRX->SetStatus(df_group, ExtIf()->logGrouping());
        GRX->SetStatus(df_extract, ExtIf()->logExtracting());
        GRX->SetStatus(df_assoc, ExtIf()->logAssociating());
        GRX->SetStatus(df_verbose, ExtIf()->logVerbose());
    }
    if (OAif()->hasOA()) {
        const char *str = OAif()->set_debug_flags(0, 0);
        const char *s = strstr(str, "load=");
        if (s) {
            s += 5;
            GRX->SetStatus(df_load, *s != '0');
        }
        s = strstr(str, "net=");
        if (s) {
            s += 4;
            GRX->SetStatus(df_net, *s != '0');
        }
        s = strstr(str, "pcell=");
        if (s) {
            s += 6;
            GRX->SetStatus(df_pcell, *s != '0');
        }
    }
}


// Static function.
void
sDbgFlg::df_cancel_proc(GtkWidget*, void*)
{
    XM()->PopUpDebugFlags(0, MODE_OFF);
}


// Static function.
void
sDbgFlg::df_action(GtkWidget *caller, void*)
{
    if (!DF)
        return;
    const char *name = gtk_widget_get_name(caller);
    if (!name)
        return;
    if (!strcmp(name, "Help"))
        DSPmainWbag(PopUpHelp("xic:dblog"))
    else if (!strcmp(name, "sel")) {
        unsigned int f = XM()->DebugFlags();
        if (GRX->GetStatus(DF->df_sel))
            f |= DBG_SELECT;
        else
            f &= ~DBG_SELECT;
        XM()->SetDebugFlags(f);
    }
    else if (!strcmp(name, "undo")) {
        unsigned int f = XM()->DebugFlags();
        if (GRX->GetStatus(DF->df_undo))
            f |= DBG_UNDOLIST;
        else
            f &= ~DBG_UNDOLIST;
        XM()->SetDebugFlags(f);
    }
    else if (!strcmp(name, "ldb3d")) {
        Ldb3d::set_logging(GRX->GetStatus(caller));
    }
    else if (!strcmp(name, "rlsolv")) {
        ExtIf()->setRLsolverMsgs(GRX->GetStatus(caller));
    }

    else if (!strcmp(name, "lisp")) {
        cLispEnv::set_logging(GRX->GetStatus(caller));
    }
    else if (!strcmp(name, "connect")) {
        ScedIf()->setLogConnect(GRX->GetStatus(caller));
    }
    else if (!strcmp(name, "rlsolvlog")) {
        ExtIf()->setLogRLsolver(GRX->GetStatus(caller));
    }
    else if (!strcmp(name, "group")) {
        ExtIf()->setLogGrouping(GRX->GetStatus(caller));
    }
    else if (!strcmp(name, "extract")) {
        ExtIf()->setLogExtracting(GRX->GetStatus(caller));
    }
    else if (!strcmp(name, "assoc")) {
        ExtIf()->setLogAssociating(GRX->GetStatus(caller));
    }
    else if (!strcmp(name, "verbose")) {
        ExtIf()->setLogVerbose(GRX->GetStatus(caller));
    }
    else if (!strcmp(name, "load")) {
        if (GRX->GetStatus(caller))
            OAif()->set_debug_flags("l", 0);
        else
            OAif()->set_debug_flags(0, "l");
    }
    else if (!strcmp(name, "net")) {
        if (GRX->GetStatus(caller))
            OAif()->set_debug_flags("n", 0);
        else
            OAif()->set_debug_flags(0, "n");
    }
    else if (!strcmp(name, "pcell")) {
        if (GRX->GetStatus(caller))
            OAif()->set_debug_flags("p", 0);
        else
            OAif()->set_debug_flags(0, "p");
    }
}


// Static function.
int
sDbgFlg::df_focus_out_proc(GtkWidget *entry, GdkEvent*, void*)
{
    const char *text = gtk_entry_get_text(GTK_ENTRY(entry));
    if (!text)
        text = "";
    char *fn = lstring::getqtok(&text);
    if (fn) {
        if (!strcmp(fn, "stdout")) {
            if (XM()->DebugFp()) {
                fclose(XM()->DebugFp());
                XM()->SetDebugFp(0);
            }
            delete [] XM()->DebugFile();
            XM()->SetDebugFile(fn);
        }
        else if (!strcmp(fn, "stderr")) {
            if (XM()->DebugFp()) {
                fclose(XM()->DebugFp());
                XM()->SetDebugFp(0);
            }
            delete [] XM()->DebugFile();
            XM()->SetDebugFile(0);
            delete [] fn;
        }
        else {
            if (XM()->DebugFile() && !strcmp(XM()->DebugFile(), fn))
                return (1);
            FILE *fp = filestat::open_file(fn, "w");
            if (!fp) {
                Log()->ErrorLog(mh::Initialization, filestat::error_msg());
                delete [] fn;
                return (1);
            }
            if (XM()->DebugFp()) {
                fclose(XM()->DebugFp());
                XM()->SetDebugFp(fp);
            }
            delete [] XM()->DebugFile();
            XM()->SetDebugFile(fn);
        }
    }
    else {
        if (XM()->DebugFp()) {
            fclose(XM()->DebugFp());
            XM()->SetDebugFp(0);
        }
        delete [] XM()->DebugFile();
        XM()->SetDebugFile(0);
    }
    return (1);
}


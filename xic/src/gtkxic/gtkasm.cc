
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
#include "fio.h"
#include "fio_assemble.h"
#include "cd_strmdata.h"
#include "cvrt.h"
#include "gtkmain.h"
#include "gtkasm.h"
#include "gtkinlines.h"
#include "gtkcv.h"
#include "gtkinterf/gtklist.h"
#include "gtkinterf/gtkfont.h"
#include "gtkinterf/gtkutil.h"
#include "miscutil/filestat.h"


//-----------------------------------------------------------------------------
// Pop-up to merge layout sources into a single file
//
// Help system keywords used:
//  xic:assem

namespace {
    sAsm *Asm;
    sAsmPrg *AsmPrg;
}


// Exported function to pop up/down the tool.
//
void
cConvert::PopUpAssemble(GRobject caller, ShowMode mode)
{
    if (!GRX || !mainBag())
        return;
    if (mode == MODE_OFF) {
        delete Asm;
        return;
    }
    if (mode == MODE_UPD) {
        if (Asm)
            Asm->update();
        return;
    }
    if (Asm)
        return;

    new sAsm(caller);
    if (!Asm->Shell()) {
        delete Asm;
        return;
    }
    gtk_window_set_transient_for(GTK_WINDOW(Asm->Shell()),
        GTK_WINDOW(mainBag()->Shell()));

    GRX->SetPopupLocation(GRloc(), Asm->Shell(), mainBag()->Viewport());
    gtk_widget_show(Asm->Shell());
}


//-----------------------------------------------------------------------------
// The sAsm (main) class

// function codes
enum
{
    NoCode,
    CancelCode,
    OpenCode,
    SaveCode,
    RecallCode,
    ResetCode,
    NewCode,
    DelCode,
    NewTlCode,
    DelTlCode,
    HelpCode
};

// Drag/drop stuff.
//
GtkTargetEntry sAsm::target_table[] =
{
    { (char*)"TWOSTRING",   0, 0 },
    { (char*)"CELLNAME",    0, 1 },
    { (char*)"STRING",      0, 2 },
    { (char*)"text/plain",  0, 3 }
};
guint sAsm::n_targets =
    sizeof(sAsm::target_table)/sizeof(sAsm::target_table[0]);

const char *sAsm::path_to_source_string =
    "Path to Source:  Layout File, CHD File, or CHD Name";
const char *sAsm::path_to_new_string = "Path to New Layout File";
int sAsm::asm_fmt_type = cConvert::cvGds;

#define IFINIT(i, a, b, c, d, e) { \
    menu_items[i].path = (char*)a; \
    menu_items[i].accelerator = (char*)b; \
    menu_items[i].callback = (GtkItemFactoryCallback)c; \
    menu_items[i].callback_action = d; \
    menu_items[i].item_type = (char*)e; \
    i++; }


sAsm::sAsm(GRobject c)
{
    Asm = this;
    asm_caller = c;
    asm_item_factory = 0;
    asm_notebook = 0;
    asm_outfile = 0;
    asm_topcell = 0;
    asm_status = 0;
    asm_fsel = 0;
    asm_fmt = 0;

#define SRC_SIZE 20
    asm_sources = new sAsmPage*[SRC_SIZE];
    asm_srcsize = SRC_SIZE;
    asm_pages = 0;
    for (unsigned int i = 0; i < asm_srcsize; i++)
        asm_sources[i] = 0;
    asm_listobj = 0;
    asm_timer_id = 0;
    asm_refptr = 0;
    asm_doing_scan = false;
    asm_abort = false;

    wb_shell = gtk_NewPopup(0, "Layout File Merge Tool", asm_cancel_proc, 0);
    if (!wb_shell)
        return;
    gtk_window_set_resizable(GTK_WINDOW(wb_shell), false);
    GRpkgIf()->RegisterMainWbag(this);

    // Without this, spin entries sometimes freeze up for some reason.
    gtk_object_set_data(GTK_OBJECT(wb_shell), "no_prop_key", (void*)1);

    GtkWidget *form = gtk_table_new(1, 4, false);
    gtk_widget_show(form);
    gtk_container_add(GTK_CONTAINER(wb_shell), form);

    // menu bar
    //
    GtkItemFactoryEntry menu_items[30];
    int nitems = 0;

    IFINIT(nitems, "/_File", 0, 0, 0, "<Branch>")
    IFINIT(nitems, "/File/_File Select", "<control>O", asm_action_proc,
        OpenCode, "<CheckItem>");
    IFINIT(nitems, "/File/_Save", "<control>S", asm_action_proc,
        SaveCode, 0);
    IFINIT(nitems, "/File/_Recall", "<control>R", asm_action_proc,
        RecallCode, 0);
    IFINIT(nitems, "/File/sep1", 0, 0, 0, "<Separator>");
    IFINIT(nitems, "/File/_Quit", "<control>Q", asm_action_proc,
        CancelCode, 0);

    IFINIT(nitems, "/_Options", 0, 0, 0, "<Branch>")
    IFINIT(nitems, "/Options/R_eset", "<control>E", asm_action_proc,
        ResetCode, 0);
    IFINIT(nitems, "/Options/_New Source", "<control>N", asm_action_proc,
        NewCode, 0);
    IFINIT(nitems, "/Options/Remove Source", 0, asm_action_proc,
        DelCode, 0);
    IFINIT(nitems, "/Options/New _Toplevel", "<control>T", asm_action_proc,
        NewTlCode, 0);
    IFINIT(nitems, "/Options/Remove Toplevel", 0, asm_action_proc,
        DelTlCode, 0);

    IFINIT(nitems, "/_Help", 0, 0, 0, "<LastBranch>");
    IFINIT(nitems, "/Help/_Help", "<control>H", asm_action_proc,
        HelpCode, 0);

    GtkAccelGroup *accel_group = gtk_accel_group_new();
    asm_item_factory = gtk_item_factory_new(GTK_TYPE_MENU_BAR, "<filetool>",
        accel_group);
    for (int i = 0; i < nitems; i++)
        gtk_item_factory_create_item(asm_item_factory, menu_items + i,
            this, 2);
    gtk_window_add_accel_group(GTK_WINDOW(wb_shell), accel_group);

    GtkWidget *menubar = gtk_item_factory_get_widget(asm_item_factory,
        "<filetool>");
    gtk_widget_show(menubar);

    int row = 0;
    gtk_table_attach(GTK_TABLE(form), menubar, 0, 1, row, row+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    row++;

    //
    // notebook setup
    //
    asm_notebook = gtk_notebook_new();
    gtk_widget_show(asm_notebook);
    gtk_signal_connect(GTK_OBJECT(asm_notebook), "switch-page",
        GTK_SIGNAL_FUNC(asm_page_change_proc), 0);

    gtk_table_attach(GTK_TABLE(form), asm_notebook, 0, 1, row, row+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL), 2, 2);
    row++;

    output_page_setup();
    notebook_append();

    //
    // button row
    //
    GtkWidget *hbox = gtk_hbox_new(false, 2);
    gtk_widget_show(hbox);

    GtkWidget *button = gtk_button_new_with_label("Create Layout File");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(asm_go_proc), this);
    gtk_box_pack_start(GTK_BOX(hbox), button, true, true, 0);
    button = gtk_button_new_with_label("Dismiss");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(asm_cancel_proc), 0);
    gtk_box_pack_start(GTK_BOX(hbox), button, true, true, 0);

    gtk_table_attach(GTK_TABLE(form), hbox, 0, 1, row, row+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    row++;

    // status message line
    //
    asm_status = gtk_label_new(0);
    gtk_widget_show(asm_status);
    gtk_label_set_justify(GTK_LABEL(asm_status), GTK_JUSTIFY_LEFT);

    gtk_table_attach(GTK_TABLE(form), asm_status, 0, 1, row, row+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
}


sAsm::~sAsm()
{
    Asm = 0;
    if (asm_caller)
        GRX->Deselect(asm_caller);
    if (asm_sources) {
        for (unsigned int i = 0; i < asm_pages; i++)
            delete asm_sources[i];
    }
    delete [] asm_sources;
    delete asm_fmt;
    if (wb_shell)
        gtk_signal_disconnect_by_func(GTK_OBJECT(wb_shell),
            GTK_SIGNAL_FUNC(asm_cancel_proc), wb_shell);

    if (asm_item_factory)
        g_object_unref(asm_item_factory);
}


void
sAsm::update()
{
    if (asm_fmt)
        asm_fmt->update();
}


// The real work is done here.
//
bool
sAsm::run()
{
    char *fname = filestat::make_temp("asb");
    FILE *fp = fopen(fname, "w");
    if (!fp) {
        PopUpErr(MODE_ON, "Error opening temporary file.", STY_NORM);
        delete [] fname;
        return (false);;
    }
    Errs()->init_error();
    if (!dump_file(fp, true)) {
        Errs()->add_error("run: error writing temporary file.");
        PopUpErr(MODE_ON, Errs()->get_error(), STY_NORM);
        fclose(fp);
        unlink(fname);
        delete [] fname;
        return (false);
    }
    fclose(fp);
    pop_up_monitor(MODE_ON, 0, ASM_INIT);
    bool ret = asm_do_run(fname);
    delete [] fname;
    return (ret);
}


// Clear all entries.
//
void
sAsm::reset()
{
    gtk_entry_set_text(GTK_ENTRY(asm_outfile), "");
    gtk_entry_set_text(GTK_ENTRY(asm_topcell), "");
    for (unsigned int i = 0; i < asm_pages; i++) {
        if (i) {
            notebook_remove(1);
            continue;
        }
        asm_sources[0]->reset();
    }
    asm_pages = 1;
}


namespace {
    // Error checking for sAsm::dump_file.
    //
    bool
    strchk(const char *string, const char *what, const char*)
    {
        if (!string || !*string) {
            Errs()->add_error("dump file: %s not given, operation aborted.",
                what);
            return (false);
        }
        return (true);
    }

    // Strip leading and trailing white space.
    //
    char *
    strip_sp(const char *str)
    {
        if (!str)
            return (0);
        while (isspace(*str))
            str++;
        if (!*str)
            return (0);
        char *sstr = lstring::copy(str);
        char *t = sstr + strlen(sstr) - 1;
        while (t >= sstr && isspace(*t))
            *t-- = 0;
        return (sstr);
    }
}


// Dump a representation of the state of all entries to the stream.
//
bool
sAsm::dump_file(FILE *fp, bool check)
{
    // save present transform entries
    store_tx_params();

    const char *outf = strip_sp(gtk_entry_get_text(GTK_ENTRY(asm_outfile)));
    if (check && !strchk(outf, path_to_new_string, 0)) {
        delete [] outf;
        return (false);
    }
    fprintf(fp, "OutFile %s\n", outf ? outf : "");
    delete [] outf;

    const char *topc = strip_sp(gtk_entry_get_text(GTK_ENTRY(asm_topcell)));
    if (topc) {
        fprintf(fp, "TopCell %s\n", topc);
        delete [] topc;
    }

    for (unsigned int i = 0; i < asm_pages; i++) {
        char page[32];
        sprintf(page, "Source %d", i+1);
        sAsmPage *src = asm_sources[i];
        char *str = strip_sp(gtk_entry_get_text(GTK_ENTRY(src->pg_path)));
        if (check && !strchk(str, path_to_source_string, page)) {
            delete [] str;
            return (false);
        }
        fprintf(fp, "Source %s\n", str ? str : "");
        delete [] str;

        str = strip_sp(gtk_entry_get_text(GTK_ENTRY(src->pg_layer_list)));
        if (str) {
            fprintf(fp, "LayerList %s\n", str);
            delete [] str;
            if (GRX->GetStatus(src->pg_layers_only))
                fprintf(fp, "OnlyLayers\n");
            else if (GRX->GetStatus(src->pg_skip_layers))
                fprintf(fp, "SkipLayers\n");
        }

        str = strip_sp(gtk_entry_get_text(GTK_ENTRY(src->pg_layer_aliases)));
        if (str) {
            fprintf(fp, "LayerAliases %s\n", str);
            delete [] str;
        }

        str = strip_sp(src->sb_scale.get_string());
        if (str) {
            if (fabs(atof(str) - 1.0) > 1e-12)
                fprintf(fp, "ConvertScale %s\n", str);
            delete [] str;
        }

        str = strip_sp(gtk_entry_get_text(GTK_ENTRY(src->pg_prefix)));
        if (str) {
            fprintf(fp, "CellNamePrefix %s\n", str);
            delete [] str;
        }

        str = strip_sp(gtk_entry_get_text(GTK_ENTRY(src->pg_suffix)));
        if (str) {
            fprintf(fp, "CellNameSuffix %s\n", str);
            delete [] str;
        }

        if (GRX->GetStatus(src->pg_to_lower))
            fprintf(fp, "ToLower\n");
        if (GRX->GetStatus(src->pg_to_upper))
            fprintf(fp, "ToUpper\n");

        for (unsigned int j = 0; j < src->pg_numtlcells; j++) {
            tlinfo *tl = src->pg_cellinfo[j];
            char *pname = strip_sp(tl->placename);
            char *cname = strip_sp(tl->cellname);
            if (pname) {
                if (cname)
                    fprintf(fp, "Place %s %s\n", cname, pname);
                else
                    fprintf(fp, "PlaceTop %s\n", pname);
            }
            else {
                if (cname)
                    fprintf(fp, "Place %s\n", cname);
                else
                    fprintf(fp, "PlaceTop\n");
            }
            delete [] pname;
            delete [] cname;
            fprintf(fp, "Translate %.4f %.4f\n",
                MICRONS(tl->x), MICRONS(tl->y));
            if (tl->angle != 0)
                fprintf(fp, "Rotate %d\n", tl->angle);
            if (fabs(tl->magn - 1.0) > 1e-12)
                fprintf(fp, "Magnify %.6f\n", tl->magn);
            if (fabs(tl->scale - 1.0) > 1e-12)
                fprintf(fp, "Scale %.6f\n", tl->scale);
            if (tl->mirror_y)
                fprintf(fp, "Reflect\n");
            if (tl->flatten)
                fprintf(fp, "Flatten\n");
            if (tl->ecf_level == ECFall)
                fprintf(fp, "NoEmpties\n");
            else if (tl->ecf_level == ECFpre)
                fprintf(fp, "NoEmpties 2\n");
            else if (tl->ecf_level == ECFpost)
                fprintf(fp, "NoEmpties 3\n");
            if (tl->use_win) {
                fprintf(fp, "Window %.4f %.4f %.4f %.4f\n",
                    MICRONS(tl->winBB.left), MICRONS(tl->winBB.bottom),
                    MICRONS(tl->winBB.right), MICRONS(tl->winBB.top));
                if (tl->clip)
                    fprintf(fp, "Clip\n");
            }
            if (tl->no_hier)
                fprintf(fp, "NoHier\n");
        }
    }
    return (true);
}


inline const char *
cknull(const char *s)
{
    return (s ? s : "");
}


bool
sAsm::read_file(FILE *fp)
{
    reset();
    ajob_t job(0);
    if (!job.parse(fp))
        return (false);
    gtk_entry_set_text(GTK_ENTRY(asm_outfile), cknull(job.outfile()));
    gtk_entry_set_text(GTK_ENTRY(asm_topcell), cknull(job.topcell()));

    bool first_page = true;
    for (const asource_t *src = job.sources(); src; src = src->next_source()) {
        if (!first_page)
            notebook_append();
        first_page = false;
        gtk_entry_set_text(GTK_ENTRY(asm_sources[asm_pages-1]->pg_path),
            cknull(src->path()));
        gtk_entry_set_text(GTK_ENTRY(asm_sources[asm_pages-1]->pg_layer_list),
            cknull(src->layer_list()));
        GRX->SetStatus(asm_sources[asm_pages-1]->pg_layers_only,
            src->only_layers());
        GRX->SetStatus(asm_sources[asm_pages-1]->pg_skip_layers,
            src->skip_layers());
        gtk_entry_set_text(
            GTK_ENTRY(asm_sources[asm_pages-1]->pg_layer_aliases),
            cknull(src->layer_aliases()));
        asm_sources[asm_pages-1]->sb_scale.set_value(src->scale());
        gtk_entry_set_text(GTK_ENTRY(asm_sources[asm_pages-1]->pg_prefix),
            cknull(src->prefix()));
        gtk_entry_set_text(GTK_ENTRY(asm_sources[asm_pages-1]->pg_suffix),
            cknull(src->suffix()));
        GRX->SetStatus(asm_sources[asm_pages-1]->pg_to_lower, src->to_lower());
        GRX->SetStatus(asm_sources[asm_pages-1]->pg_to_upper, src->to_upper());

        for (ainst_t *inst = src->instances(); inst;
                inst = inst->next_instance()) {
            tlinfo *tl = asm_sources[asm_pages-1]->add_instance(
                inst->cellname());
            tl->placename = lstring::copy(inst->placename());
            tl->x = inst->pos_x();
            tl->y = inst->pos_y();
            tl->angle = mmRnd(inst->angle());
            tl->magn = inst->magn();
            tl->scale = inst->scale();
            tl->mirror_y = inst->reflc();
            tl->no_hier = inst->no_hier();
            tl->ecf_level = inst->ecf_level();
            tl->flatten = inst->flatten();
            tl->use_win = inst->use_win();
            tl->winBB = *inst->winBB();
            tl->clip = inst->clip();
        }
    }
    asm_sources[asm_pages-1]->upd_sens();
    return (true);
}


void
sAsm::notebook_append()
{
    char buf[32];
    if (asm_pages+1 >= asm_srcsize) {
        sAsmPage **tmp = new sAsmPage*[asm_srcsize + asm_srcsize];
        unsigned int i;
        for (i = 0; i < asm_srcsize; i++)
            tmp[i] = asm_sources[i];
        asm_srcsize += asm_srcsize;
        for ( ; i < asm_srcsize; i++)
            tmp[i] = 0;
        delete [] asm_sources;
        asm_sources = tmp;
    }

    int index = asm_pages;
    asm_pages++;

    sAsmPage *src = new sAsmPage(this);
    asm_sources[index] = src;
    sprintf(buf, "Source %d", index+1);
    src->pg_tablabel = gtk_label_new(buf);
    gtk_widget_show(src->pg_tablabel);
    gtk_notebook_insert_page(GTK_NOTEBOOK(asm_notebook), src->pg_form,
        src->pg_tablabel, index + 1);
}


void
sAsm::notebook_remove(int index)
{
    char buf[32];
    if (index == 0 && asm_pages <= 1)
        // never delete first entry
        return;
    gtk_notebook_remove_page(GTK_NOTEBOOK(asm_notebook), index + 1);
    sAsmPage *src = asm_sources[index];
    for (unsigned int i = index; i < asm_pages-1; i++) {
        asm_sources[i] = asm_sources[i+1];
        sprintf(buf, "source %d", i+1);
        gtk_label_set_text(GTK_LABEL(asm_sources[i]->pg_tablabel), buf);
    }
    asm_pages--;
    asm_sources[asm_pages] = 0;
    delete src;
}


int
sAsm::current_page_index()
{
    // Compensate for the "Output" tab.
    int ix = gtk_notebook_current_page(GTK_NOTEBOOK(asm_notebook));
    return (ix - 1);
}


void
sAsm::output_page_setup()
{
    GtkWidget *label = gtk_label_new("Output");
    gtk_widget_show(label);

    GtkWidget *form = gtk_table_new(1, 1, false);
    gtk_widget_show(form);
    gtk_notebook_insert_page(GTK_NOTEBOOK(asm_notebook), form, label, 0);

    asm_fmt = new cvofmt_t(0, asm_fmt_type, cvofmt_asm);

    int row = 0;
    gtk_table_attach(GTK_TABLE(form), asm_fmt->frame(), 0, 1, row, row+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    row++;

    GtkWidget *hsep = gtk_hseparator_new();
    gtk_widget_show(hsep);
    gtk_table_attach(GTK_TABLE(form), hsep, 0, 1, row, row+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    row++;

    GtkWidget *hbox = gtk_hbox_new(false, 2);
    gtk_widget_show(hbox);

    //
    // cell name and path to archive
    //
    label = gtk_label_new("Top-Level Cell Name");
    gtk_widget_show(label);
    gtk_box_pack_start(GTK_BOX(hbox), label, true, true, 0);
    asm_topcell = gtk_entry_new();
    gtk_widget_show(asm_topcell);
    gtk_box_pack_start(GTK_BOX(hbox), asm_topcell, true, true, 0);

    gtk_table_attach(GTK_TABLE(form), hbox, 0, 1, row, row+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    row++;

    GtkWidget *vbox = gtk_vbox_new(false, 0);
    gtk_widget_show(vbox);

    label = gtk_label_new(path_to_new_string);
    gtk_widget_show(label);
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
    gtk_box_pack_start(GTK_BOX(vbox), label, false, false, 2);
    asm_outfile = gtk_entry_new();
    gtk_widget_show(asm_outfile);
    gtk_box_pack_start(GTK_BOX(vbox), asm_outfile, true, true, 0);

    // drop site
    GtkDestDefaults DD = (GtkDestDefaults)
        (GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_HIGHLIGHT);
    gtk_drag_dest_set(asm_outfile, DD, target_table, n_targets,
        GDK_ACTION_COPY);
    gtk_signal_connect_after(GTK_OBJECT(asm_outfile), "drag-data-received",
        GTK_SIGNAL_FUNC(asm_drag_data_received), 0);

    GtkWidget *frame = gtk_frame_new(0);
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), vbox);

    gtk_table_attach(GTK_TABLE(form), frame, 0, 1, row, row+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    row++;
}


// Save the visible transform parameters into the respective tlinfo
// storage.
//
void
sAsm::store_tx_params()
{
    int ix = current_page_index();
    if (ix < 0)
        return;
    sAsmPage *src = asm_sources[ix];
    int n = src->pg_curtlcell;
    if (n >= 0) {
        tlinfo *tl = src->pg_cellinfo[n];
        src->pg_tx->get_tx_params(tl);
    }
}


// Display the saved transform parameters in the widgets, for cellname
// index n.  This becomes "current".
//
void
sAsm::show_tx_params(unsigned int n)
{
    int ix = current_page_index();
    if (ix < 0)
        return;
    sAsmPage *src = asm_sources[ix];
    if (n < src->pg_numtlcells) {
        tlinfo *tl = src->pg_cellinfo[n];
        src->pg_tx->set_tx_params(tl);
        src->pg_curtlcell = n;
        src->upd_sens();
    }
    else {
        src->pg_tx->reset();
        src->upd_sens();
    }
}


// Export top cell name, for use in source page sensitivity test.
//
const char *
sAsm::top_level_cell()
{
    return (gtk_entry_get_text(GTK_ENTRY(asm_topcell)));
}


// Static function.
void
sAsm::set_status_message(const char *msg)
{
    if (Asm) {
        if (Asm->asm_timer_id)
            gtk_timeout_remove(Asm->asm_timer_id);
        Asm->asm_timer_id = gtk_timeout_add(10000, asm_timer_callback, 0);
        gtk_label_set_text(GTK_LABEL(Asm->asm_status), msg);
    }
}


// Static function.
// Exported interface function for progress monitor.
//
void
sAsm::pop_up_monitor(int mode, const char *msg, ASMcode code)
{
    if (mode == MODE_OFF) {
        delete AsmPrg;
        AsmPrg = 0;
        return;
    }
    if (mode == MODE_UPD) {
        if (Asm && Asm->scanning()) {
            if (code == ASM_READ) {
                char *str = lstring::copy(msg);
                char *s = str + strlen(str) - 1;
                while (s >= str && isspace(*s))
                    *s-- = 0;
                set_status_message(str);
                delete [] str;
            }
        }
        else if (AsmPrg)
            AsmPrg->update(msg, code);
        return;
    }
    if (AsmPrg)
        return;

    AsmPrg = new sAsmPrg();
    AsmPrg->set_refptr((void**)&AsmPrg);
    if (Asm) {
        gtk_window_set_transient_for(GTK_WINDOW(AsmPrg->shell()),
            GTK_WINDOW(Asm->wb_shell));
        GRX->SetPopupLocation(GRloc(), AsmPrg->shell(), Asm->wb_shell);
    }
    gtk_widget_show(AsmPrg->shell());
}


// Static function.
// Drag data received in entry window, grab it.
//
void
sAsm::asm_drag_data_received(GtkWidget *entry, GdkDragContext *context,
    gint, gint, GtkSelectionData *data, guint, guint time)
{
    if (data->length >= 0 && data->format == 8 && data->data) {
        char *src = (char*)data->data;
        if (data->target == gdk_atom_intern("TWOSTRING", true)) {
            // Drops from content lists may be in the form
            // "fname_or_chd\ncellname".  Keep the filename.
            char *t = strchr(src, '\n');
            if (t)
                *t = 0;
        }
        gtk_entry_set_text(GTK_ENTRY(entry), src);
        gtk_drag_finish(context, true, false, time);
        return;
    }
    gtk_drag_finish(context, false, false, time);
}


// Static function.
void
sAsm::asm_cancel_proc(GtkWidget*, void*)
{
    delete Asm;
}


namespace {
    int
    run_idle(void*)
    {
        if (Asm)
            Asm->run();
        return (0);
    }
}


// Static function.
// Handle the Create Layout File button.
//
void
sAsm::asm_go_proc(GtkWidget*, void*)
{
    dspPkgIf()->RegisterIdleProc(run_idle, 0);
}


// Static function.
void
sAsm::asm_page_change_proc(GtkWidget*, void*, int page, void*)
{
    if (!Asm)
        return;
    if (Asm->current_page_index() >= 0)
        // save present transform entries
        Asm->store_tx_params();

    if (page > 0) {
        sAsmPage *src = Asm->asm_sources[page-1];
        int n = src->pg_curtlcell;
        if (n >= 0)
            gtk_list_select_item(GTK_LIST(src->pg_toplevels), n);
        else {
            src->pg_tx->reset();
            gtk_list_unselect_all(GTK_LIST(src->pg_toplevels));
        }
        src->upd_sens();
    }
}


// Static function.
// Handle menu button presses.
//
void
sAsm::asm_action_proc(GtkWidget *caller, void*, unsigned int code)
{
    if (!Asm)
        return;
    if (code == NoCode) {
        // shouldn't receive this
    }
    else if (code == CancelCode) {
        // cancel the pop-up
        Cvt()->PopUpAssemble(0, MODE_OFF);
    }
    else if (code == OpenCode) {
        // pop up/down file selection panel
        if (GRX->GetStatus(caller)) {
            if (!Asm->asm_fsel) {
                Asm->asm_fsel = Asm->PopUpFileSelector(fsSEL, GRloc(LW_LR),
                    asm_fsel_open, asm_fsel_cancel, 0, 0);
                Asm->asm_fsel->register_usrptr((void**)&Asm->asm_fsel);
            }
        }
        else if (Asm->asm_fsel)
            Asm->asm_fsel->popdown();
    }
    else if (code == SaveCode) {
        // solicit for a new file to save state
        Asm->PopUpInput("Enter file name", "", "Save State", asm_save_cb, 0);
    }
    else if (code == RecallCode) {
        // solicit for a new file to read state
        Asm->PopUpInput("Enter file name", "", "Recall State", asm_recall_cb,
            0);
    }
    else if (code == ResetCode) {
        // clear all entries
        Asm->reset();
    }
    else if (code == NewCode) {
        // add a source notebook page
        Asm->notebook_append();
    }
    else if (code == DelCode) {
        // delete the current notebook page
        Asm->notebook_remove(Asm->current_page_index());
    }
    else if (code == NewTlCode) {
        // solicit for a new "top-level" cell to convert
        Asm->PopUpInput("Enter cell name", "", "Add Cell", asm_tladd_cb, 0);
    }
    else if (code == DelTlCode) {
        // delete the currently selected "top-level" cell
        int ix = Asm->current_page_index();
        if (ix < 0)
            return;

        sAsmPage *src = Asm->asm_sources[ix];
        if (src->pg_numtlcells) {
            int n = src->pg_curtlcell;
            if (n >= 0) {

                if (n > 0)
                    gtk_list_clear_items(GTK_LIST(src->pg_toplevels), n, n+1);
                else {
                    // Hideous gtk1 bug in gtk_list_clear_items:
                    // removing the first element if there is more
                    // than one causes a seg fault.
                    // gtklist.c (1.2.10) line 1302:
                    //   new_focus_child = list->children->prev->data;
                    // should be
                    //   new_focus_child = list->children->data;
                    // Here's a hack-around.
                    //
                    GtkList *l = GTK_LIST(src->pg_toplevels);
                    GList *g = g_list_append(0, l->children->data);
                    gtk_list_remove_items(GTK_LIST(src->pg_toplevels), g);
                }

                delete src->pg_cellinfo[n];
                src->pg_numtlcells--;
                for (unsigned int i = n; i < src->pg_numtlcells; i++)
                    src->pg_cellinfo[i] = src->pg_cellinfo[i+1];
                src->pg_cellinfo[src->pg_numtlcells] = 0;
                src->pg_curtlcell = -1;
                src->upd_sens();
                if (src->pg_numtlcells > 0)
                    gtk_list_select_item(GTK_LIST(src->pg_toplevels),
                        src->pg_numtlcells-1);
            }
        }
    }
    else if (code == HelpCode)
        Asm->PopUpHelp("xic:assem");
}


// Static function.
// Handler for the Save file name input.  Save the state to file.
//
void
sAsm::asm_save_cb(const char *fname, void*)
{
    if (!Asm)
        return;
    if (fname && *fname) {
        if (!filestat::create_bak(fname, 0)) {
            char buf[512];
            sprintf(buf,
                "Error: %s/ncould not back up existing file, save aborted.",
                Errs()->get_error());
            Asm->PopUpErr(MODE_ON, buf, STY_NORM);
            return;
        }
        FILE *fp = fopen(fname, "w");
        if (!fp) {
            const char *msg = "Can't open file, try again";
            GtkWidget *label = (GtkWidget*)gtk_object_get_data(
                GTK_OBJECT(Asm->wb_input), "label");
            if (label)
                gtk_label_set_text(GTK_LABEL(label), msg);
            else
                set_status_message(msg);
            return;
        }
        Errs()->init_error();
        if (Asm->dump_file(fp, false)) {
            char *s = new char[strlen(fname) + 100];
            sprintf(s, "State saved in file %s", fname);
            set_status_message(s);
            delete [] s;
            if (Asm->wb_input)
                Asm->wb_input->popdown();
        }
        else {
            Errs()->add_error("save_cb: operation failed.");
            Asm->PopUpErr(MODE_ON, Errs()->get_error(), STY_NORM);
        }
        fclose(fp);
    }
}


// Static function.
// Handler for the Recall file name input.  Recall the state from the
// file.
//
void
sAsm::asm_recall_cb(const char *fname, void*)
{
    if (!Asm)
        return;
    if (fname && *fname) {
        FILE *fp = fopen(fname, "r");
        if (!fp) {
            const char *msg = "Can't open file, try again";
            GtkWidget *label = (GtkWidget*)gtk_object_get_data(
                GTK_OBJECT(Asm->wb_input), "label");
            if (label)
                gtk_label_set_text(GTK_LABEL(label), msg);
            else
                set_status_message(msg);
            return;
        }
        Errs()->init_error();
        if (Asm->read_file(fp)) {
            char *s = new char[strlen(fname) + 100];
            sprintf(s, "State restored from file %s", fname);
            set_status_message(s);
            delete [] s;
            if (Asm->wb_input)
                Asm->wb_input->popdown();
        }
        else {
            Errs()->add_error("recall_cb: operation failed.");
            Asm->PopUpErr(MODE_ON, Errs()->get_error(), STY_NORM);
        }
        fclose(fp);
    }
}


// Static function.
// Handler for the File Selection pop-up, fill the archive path entry.
//
void
sAsm::asm_fsel_open(const char *path, void*)
{
    if (!Asm)
        return;
    if (path && *path) {
        int ix = Asm->current_page_index();
        if (ix < 0)
            return;
        sAsmPage *src = Asm->asm_sources[ix];
        gtk_entry_set_text(GTK_ENTRY(src->pg_path), path);
    }
}


// Static function.
// Handle File Selection deletion.
//
void
sAsm::asm_fsel_cancel(GRfilePopup*, void*)
{
    if (!Asm)
        return;
    Asm->asm_fsel = 0;
    GtkWidget *item = gtk_item_factory_get_widget(Asm->asm_item_factory,
        "/File/File Select");
    GRX->Deselect(item);
}


// Static function.
// Handler for "top-level" cell name input.  Add the cell name to the
// list for the current page.
//
void
sAsm::asm_tladd_cb(const char *cname, void*)
{
    if (!Asm)
        return;
    if (cname) {
        while (isspace(*cname))
            cname++;
        if (!*cname || !strcmp(cname, ASM_TOPC))
            cname = 0;
        int ix = Asm->current_page_index();
        if (ix < 0)
            return;
        Asm->asm_sources[ix]->add_instance(cname);
        if (Asm->wb_input)
            Asm->wb_input->popdown();
    }
}


// Static function.
// Erase the status message.
//
int
sAsm::asm_timer_callback(void*)
{
    if (Asm) {
        gtk_label_set_text(GTK_LABEL(Asm->asm_status), "");
        Asm->asm_timer_id = 0;
    }
    return (0);
}


namespace {
    void
    ifInfoMessage(INFOmsgType code, const char *string, va_list args)
    {
        char buf[512];
        if (!string)
            string = "";
        vsnprintf(buf, 512, string, args);
        if (code == IFMSG_INFO)
            sAsm::pop_up_monitor(MODE_UPD, buf, ASM_INFO);
        else if (code == IFMSG_RD_PGRS)
            sAsm::pop_up_monitor(MODE_UPD, buf, ASM_READ);
        else if (code == IFMSG_WR_PGRS)
            sAsm::pop_up_monitor(MODE_UPD, buf, ASM_WRITE);
        else if (code == IFMSG_CNAME)
            sAsm::pop_up_monitor(MODE_UPD, buf, ASM_CNAME);
        else if (code == IFMSG_POP_ERR) {
            if (mainBag())
                mainBag()->PopUpErr(MODE_ON, buf);
        }
        else if (code == IFMSG_POP_WARN) {
            if (mainBag())
                mainBag()->PopUpWarn(MODE_ON, buf);
        }
        else if (code == IFMSG_POP_INFO) {
            if (mainBag())
                mainBag()->PopUpInfo(MODE_ON, buf);
        }
        else if (code == IFMSG_LOG_ERR) {
            if (mainBag())
                mainBag()->PopUpErr(MODE_ON, buf);
        }
        else if (code == IFMSG_LOG_WARN) {
            if (mainBag())
                mainBag()->PopUpWarn(MODE_ON, buf);
        }
    }
}


// Static function.
void
sAsm::asm_setup_monitor(bool active)
{
    static void (*info_msg)(INFOmsgType, const char*, va_list);

    if (active)
        info_msg = FIO()->RegisterIfInfoMessage(ifInfoMessage);
    else
        info_msg = FIO()->RegisterIfInfoMessage(info_msg);
}


// Static function.
// The real work is done here.
//
bool
sAsm::asm_do_run(const char *fname)
{
    dspPkgIf()->SetWorking(true);
    set_status_message("Working...");
    asm_setup_monitor(true);

    FILE *fp = fopen(fname, "r");
    if (!fp) {
        (Asm ? (main_bag*)Asm : mainBag())->PopUpErr(
            MODE_ON, "Unable to reopen temporary file.", STY_NORM);
        set_status_message("Terminated with error");
        return (false);
    }
    ajob_t *job = new ajob_t(0);
    if (!job->parse(fp)) {
        Errs()->add_error("asm_do_run: processing failed.");
        Errs()->add_error("asm_do_run: keeping temp file %s.", fname);
        (Asm ? (main_bag*)Asm : mainBag())->PopUpErr(
            MODE_ON, Errs()->get_error(), STY_NORM);
        fclose(fp);
        delete job;
        set_status_message("Terminated with error");
        dspPkgIf()->SetWorking(false);
        asm_setup_monitor(false);
        return (false);
    }
    fclose(fp);

    if (!job->run(0)) {
        if (AsmPrg && AsmPrg->aborted()) {
            set_status_message("Task ABORTED on user request.");
            unlink(fname);
        }
        else {
            Errs()->add_error("asm_do_run: processing failed.");
            Errs()->add_error("asm_do_run: keeping temp file %s.", fname);
            (Asm ? (main_bag*)Asm : mainBag())->PopUpErr(
                MODE_ON, Errs()->get_error(), STY_NORM);
            set_status_message("Terminated with error");
        }
        delete job;
        dspPkgIf()->SetWorking(false);
        asm_setup_monitor(false);
        return (false);
    }
    delete job;
    unlink(fname);
    dspPkgIf()->SetWorking(false);
    set_status_message("Operation completed successfully");
    asm_setup_monitor(false);
    return (true);
}


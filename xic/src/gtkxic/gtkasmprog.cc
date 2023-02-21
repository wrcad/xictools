
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
// Progress Monitor Pop-Up

sAsmPrg::sAsmPrg()
{
    prg_refptr = 0;
    prg_shell = gtk_NewPopup(0, "Progress", prg_cancel_proc, this);

    GtkWidget *form = gtk_table_new(1, 1, false);
    gtk_widget_show(form);
    gtk_container_add(GTK_CONTAINER(prg_shell), form);

    GtkWidget *frame = gtk_frame_new("Input");
    gtk_widget_show(frame);

    prg_inp_label = gtk_label_new("");
    gtk_widget_show(prg_inp_label);
    gtk_container_add(GTK_CONTAINER(frame), prg_inp_label);
    gtk_widget_set_size_request(prg_inp_label, 240, 20);

    int row = 0;
    gtk_table_attach(GTK_TABLE(form), frame, 0, 1, row, row + 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    frame = gtk_frame_new("Output");
    gtk_widget_show(frame);

    prg_out_label = gtk_label_new("");
    gtk_widget_show(prg_out_label);
    gtk_container_add(GTK_CONTAINER(frame), prg_out_label);
    gtk_widget_set_size_request(prg_out_label, 240, 20);

    gtk_table_attach(GTK_TABLE(form), frame, 1, 2, row, row + 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    row++;

    frame = gtk_frame_new("Info");
    gtk_widget_show(frame);

    prg_info_label = gtk_label_new("");
    gtk_widget_show(prg_info_label);
    gtk_container_add(GTK_CONTAINER(frame), prg_info_label);
    gtk_widget_set_size_request(prg_info_label, -1, 40);

    gtk_table_attach(GTK_TABLE(form), frame, 0, 2, row, row + 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL), 2, 2);
    row++;

    frame = gtk_frame_new(0);
    gtk_widget_show(frame);

    prg_cname_label = gtk_label_new("");
    gtk_widget_show(prg_cname_label);
    gtk_container_add(GTK_CONTAINER(frame), prg_cname_label);
    gtk_widget_set_size_request(prg_cname_label, -1, 40);

    gtk_table_attach(GTK_TABLE(form), frame, 0, 2, row, row + 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL), 2, 2);
    row++;

    GtkWidget *hbox = gtk_hbox_new(false, 2);
    gtk_widget_show(hbox);

    GtkWidget *button = gtk_button_new_with_label("Abort");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(prg_abort_proc), this);
    gtk_box_pack_start(GTK_BOX(hbox), button, true, true, 0);

    button = gtk_button_new_with_label("Dismiss");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(prg_cancel_proc), this);
    gtk_box_pack_start(GTK_BOX(hbox), button, true, true, 0);

    gtk_table_attach(GTK_TABLE(form), hbox, 0, 1, row, row + 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    prg_pbar = gtk_progress_bar_new();
    gtk_widget_show(prg_pbar);
    gtk_progress_bar_set_pulse_step(GTK_PROGRESS_BAR(prg_pbar), 0.05);

    gtk_table_attach(GTK_TABLE(form), prg_pbar, 1, 2, row, row + 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    row++;

    gtk_widget_set_size_request(prg_shell, 500, -1);
}


sAsmPrg::~sAsmPrg()
{
    if (prg_refptr)
        *prg_refptr = 0;
    g_signal_handlers_disconnect_by_func(G_OBJECT(prg_shell),
        (gpointer)prg_cancel_proc, this);
    gtk_widget_destroy(prg_shell);
}


void
sAsmPrg::update(const char *msg, ASMcode code)
{
    gtk_progress_bar_pulse(GTK_PROGRESS_BAR(prg_pbar));
    char *str = lstring::copy(msg);
    char *s = str + strlen(str) - 1;
    while (s >= str && isspace(*s))
        *s-- = 0;
    if (code == ASM_INFO)
        gtk_label_set_text(GTK_LABEL(prg_info_label), str);
    else if (code == ASM_READ)
        gtk_label_set_text(GTK_LABEL(prg_inp_label), str);
    else if (code == ASM_WRITE)
        gtk_label_set_text(GTK_LABEL(prg_out_label), str);
    else if (code == ASM_CNAME)
        gtk_label_set_text(GTK_LABEL(prg_cname_label), str);
    delete [] str;
}


// Static function.
void
sAsmPrg::prg_cancel_proc(GtkWidget*, void *arg)
{
    sAsmPrg *ptr = static_cast<sAsmPrg*>(arg);
    delete ptr;
}


// Static function.
void
sAsmPrg::prg_abort_proc(GtkWidget*, void *arg)
{
    sAsmPrg *ptr = static_cast<sAsmPrg*>(arg);
    ptr->prg_abort = true;
}


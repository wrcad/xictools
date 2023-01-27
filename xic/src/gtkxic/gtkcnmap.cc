
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
#include "gtkmain.h"
#include "gtkcv.h"


//-------------------------------------------------------------------------
// Subwidget group for cell name mapping

cnmap_t::cnmap_t(bool outp)
{
    GtkWidget *tform = gtk_table_new(1, 1, false);
    gtk_widget_show(tform);

    cn_output = outp;
    int col = 0;
    GtkWidget *label = gtk_label_new("Prefix");
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(tform), label, col, col+1, 0, 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    col++;
    cn_prefix = gtk_entry_new();
    gtk_widget_show(cn_prefix);
    gtk_widget_set_size_request(cn_prefix, 60, -1);
    gtk_table_attach(GTK_TABLE(tform), cn_prefix, col, col+1, 0, 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    col++;
    cn_to_lower = gtk_check_button_new_with_label("To Lower");
    gtk_widget_show(cn_to_lower);
    gtk_widget_set_name(cn_to_lower, "tolower");
    g_signal_connect(G_OBJECT(cn_to_lower), "clicked",
        G_CALLBACK(cn_action), this);
    gtk_table_attach(GTK_TABLE(tform), cn_to_lower, col, col+1, 0, 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    col++;
    cn_rd_alias = gtk_check_button_new_with_label("Read Alias");
    gtk_widget_show(cn_rd_alias);
    gtk_widget_set_name(cn_rd_alias, "rdalias");
    g_signal_connect(G_OBJECT(cn_rd_alias), "clicked",
        G_CALLBACK(cn_action), this);
    gtk_table_attach(GTK_TABLE(tform), cn_rd_alias, col, col+1, 0, 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    col = 0;
    label = gtk_label_new("Suffix");
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(tform), label, col, col+1, 1, 2,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    col++;
    cn_suffix = gtk_entry_new();
    gtk_widget_show(cn_suffix);
    gtk_widget_set_size_request(cn_suffix, 60, -1);
    gtk_table_attach(GTK_TABLE(tform), cn_suffix, col, col+1, 1, 2,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    col++;
    cn_to_upper = gtk_check_button_new_with_label("To Upper");
    gtk_widget_show(cn_to_upper);
    gtk_widget_set_name(cn_to_upper, "toupper");
    g_signal_connect(G_OBJECT(cn_to_upper), "clicked",
        G_CALLBACK(cn_action), this);
    gtk_table_attach(GTK_TABLE(tform), cn_to_upper, col, col+1, 1, 2,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    col++;
    cn_wr_alias = gtk_check_button_new_with_label("Write Alias");
    gtk_widget_show(cn_wr_alias);
    gtk_widget_set_name(cn_wr_alias, "wralias");
    g_signal_connect(G_OBJECT(cn_wr_alias), "clicked",
        G_CALLBACK(cn_action), this);
    gtk_table_attach(GTK_TABLE(tform), cn_wr_alias, col, col+1, 1, 2,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    update();
    // must be done after entry text set
    g_signal_connect(G_OBJECT(cn_prefix), "changed",
        G_CALLBACK(cn_text_changed), this);
    g_signal_connect(G_OBJECT(cn_suffix), "changed",
        G_CALLBACK(cn_text_changed), this);

    cn_frame = gtk_frame_new("Cell Name Mapping");
    gtk_widget_show(cn_frame);
    gtk_container_add(GTK_CONTAINER(cn_frame), tform);
}


void
cnmap_t::update()
{
    if (cn_output) {
        const char *str = CDvdb()->getVariable(VA_OutCellNamePrefix);
        if (!str)
            str = "";
        const char *s = gtk_entry_get_text(GTK_ENTRY(cn_prefix));
        if (!s)
            s = "";
        if (strcmp(str, s))
            gtk_entry_set_text(GTK_ENTRY(cn_prefix), str);
        str = CDvdb()->getVariable(VA_OutCellNameSuffix);
        if (!str)
            str = "";
        s = gtk_entry_get_text(GTK_ENTRY(cn_suffix));
        if (!s)
            s = "";
        if (strcmp(str, s))
            gtk_entry_set_text(GTK_ENTRY(cn_suffix), str);
        GRX->SetStatus(cn_to_lower, CDvdb()->getVariable(VA_OutToLower));
        GRX->SetStatus(cn_to_upper, CDvdb()->getVariable(VA_OutToUpper));
        str = CDvdb()->getVariable(VA_OutUseAlias);
        if (!str) {
            GRX->SetStatus(cn_rd_alias, false);
            GRX->SetStatus(cn_wr_alias, false);
        }
        else if (*str == 'r' || *str == 'R') {
            GRX->SetStatus(cn_rd_alias, true);
            GRX->SetStatus(cn_wr_alias, false);
        }
        else if (*str == 'w' || *str == 'W' || *str == 's' || *str == 'S') {
            GRX->SetStatus(cn_rd_alias, false);
            GRX->SetStatus(cn_wr_alias, true);
        }
        else {
            GRX->SetStatus(cn_rd_alias, true);
            GRX->SetStatus(cn_wr_alias, true);
        }
    }
    else {
        const char *str = CDvdb()->getVariable(VA_InCellNamePrefix);
        if (!str)
            str = "";
        const char *s = gtk_entry_get_text(GTK_ENTRY(cn_prefix));
        if (!s)
            s = "";
        if (strcmp(str, s))
            gtk_entry_set_text(GTK_ENTRY(cn_prefix), str);
        str = CDvdb()->getVariable(VA_InCellNameSuffix);
        if (!str)
            str = "";
        s = gtk_entry_get_text(GTK_ENTRY(cn_suffix));
        if (!s)
            s = "";
        if (strcmp(str, s))
            gtk_entry_set_text(GTK_ENTRY(cn_suffix), str);
        GRX->SetStatus(cn_to_lower, CDvdb()->getVariable(VA_InToLower));
        GRX->SetStatus(cn_to_upper, CDvdb()->getVariable(VA_InToUpper));
        str = CDvdb()->getVariable(VA_InUseAlias);
        if (!str) {
            GRX->SetStatus(cn_rd_alias, false);
            GRX->SetStatus(cn_wr_alias, false);
        }
        else if (*str == 'r' || *str == 'R') {
            GRX->SetStatus(cn_rd_alias, true);
            GRX->SetStatus(cn_wr_alias, false);
        }
        else if (*str == 'w' || *str == 'W' || *str == 's' || *str == 'S') {
            GRX->SetStatus(cn_rd_alias, false);
            GRX->SetStatus(cn_wr_alias, true);
        }
        else {
            GRX->SetStatus(cn_rd_alias, true);
            GRX->SetStatus(cn_wr_alias, true);
        }
    }
}


void
cnmap_t::text_changed(GtkWidget *caller)
{
    if (caller == cn_prefix) {
        const char *s = gtk_entry_get_text(GTK_ENTRY(caller));
        const char *ss = cn_output ?
            CDvdb()->getVariable(VA_OutCellNamePrefix) :
            CDvdb()->getVariable(VA_InCellNamePrefix);
        if (s && *s) {
            if (!ss || strcmp(ss, s)) {
                if (cn_output)
                    CDvdb()->setVariable(VA_OutCellNamePrefix, s);
                else
                    CDvdb()->setVariable(VA_InCellNamePrefix, s);
            }
        }
        else {
            if (cn_output)
                CDvdb()->clearVariable(VA_OutCellNamePrefix);
            else
                CDvdb()->clearVariable(VA_InCellNamePrefix);
        }
    }
    else if (caller == cn_suffix) {
        const char *s = gtk_entry_get_text(GTK_ENTRY(caller));
        const char *ss = cn_output ?
            CDvdb()->getVariable(VA_OutCellNameSuffix) :
            CDvdb()->getVariable(VA_InCellNameSuffix);
        if (s && *s) {
            if (!ss || strcmp(ss, s)) {
                if (cn_output)
                    CDvdb()->setVariable(VA_OutCellNameSuffix, s);
                else
                    CDvdb()->setVariable(VA_InCellNameSuffix, s);
            }
        }
        else {
            if (cn_output)
                CDvdb()->clearVariable(VA_OutCellNameSuffix);
            else
                CDvdb()->clearVariable(VA_InCellNameSuffix);
        }
    }
}


void
cnmap_t::action(GtkWidget *caller)
{
    const char *name = gtk_widget_get_name(caller);
    if (cn_output) {
        if (!strcmp(name, "tolower")) {
            if (GRX->GetStatus(caller))
                CDvdb()->setVariable(VA_OutToLower, 0);
            else
                CDvdb()->clearVariable(VA_OutToLower);
        }
        else if (!strcmp(name, "toupper")) {
            if (GRX->GetStatus(caller))
                CDvdb()->setVariable(VA_OutToUpper, 0);
            else
                CDvdb()->clearVariable(VA_OutToUpper);
        }
        else if (!strcmp(name, "rdalias") || !strcmp(name, "wralias")) {
            bool rd = GRX->GetStatus(cn_rd_alias);
            bool wr = GRX->GetStatus(cn_wr_alias);
            if (rd) {
                if (wr)
                    CDvdb()->setVariable(VA_OutUseAlias, 0);
                else
                    CDvdb()->setVariable(VA_OutUseAlias, "r");
            }
            else {
                if (wr)
                    CDvdb()->setVariable(VA_OutUseAlias, "w");
                else
                    CDvdb()->clearVariable(VA_OutUseAlias);
            }
        }
    }
    else {
        if (!strcmp(name, "tolower")) {
            if (GRX->GetStatus(caller))
                CDvdb()->setVariable(VA_InToLower, 0);
            else
                CDvdb()->clearVariable(VA_InToLower);
            return;
        }
        else if (!strcmp(name, "toupper")) {
            if (GRX->GetStatus(caller))
                CDvdb()->setVariable(VA_InToUpper, 0);
            else
                CDvdb()->clearVariable(VA_InToUpper);
            return;
        }
        else if (!strcmp(name, "rdalias") || !strcmp(name, "wralias")) {
            bool rd = GRX->GetStatus(cn_rd_alias);
            bool wr = GRX->GetStatus(cn_wr_alias);
            if (rd) {
                if (wr)
                    CDvdb()->setVariable(VA_InUseAlias, 0);
                else
                    CDvdb()->setVariable(VA_InUseAlias, "r");
            }
            else {
                if (wr)
                    CDvdb()->setVariable(VA_InUseAlias, "w");
                else
                    CDvdb()->clearVariable(VA_InUseAlias);
            }
            return;
        }
    }
}


// Static function.
void
cnmap_t::cn_text_changed(GtkWidget *caller, void *arg)
{
    ((cnmap_t*)arg)->text_changed(caller);
}


// Static function.
void
cnmap_t::cn_action(GtkWidget *caller, void *arg)
{
    ((cnmap_t*)arg)->action(caller);
}


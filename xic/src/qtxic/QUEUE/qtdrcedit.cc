
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

#include "qtdrcedit.h"
#include "main.h"
#include "drc.h"
#include "drc_edit.h"
#include "cd_lgen.h"
#include "dsp_layer.h"
#include "dsp_color.h"
#include "dsp_inlines.h"
#include "layertab.h"
#include "tech_layer.h"
#include "promptline.h"
#include "errorlog.h"
#include "tech.h"
#include "qtinterf/qtfont.h"
#include "miscutil/filestat.h"


//-----------------------------------------------------------------------------
// Pop up to display a listing of design rules for the current layer.
//
// Help system keywords used:
//  xic:dredt

void
cDRC::PopUpRules(GRobject caller, ShowMode mode)
{
    if (!QTdev::exists() || !QTmainwin::exists())
        return;
    if (mode == MODE_OFF) {
        if (QTdrcRuleEdutDlg::self())
            QTdrcRuleEditDlg::self()->deleteLater();
        return;
    }
    if (mode == MODE_UPD) {
        if (!QTdrcRuleEditDlg::self())
            return;
        if (DSP()->CurMode() == Electrical) {
            PopUpRules(0, MODE_OFF);
            return;
        }
        QTdrcRuleEditDlg::self()->update();
        return;
    }

    if (QTdrcRuleEditDlg::self())
        return;

    new QRdrcRuleEditDlg(caller);

    QTdev::self()->SetPopupLocation(GRloc(), QTdrcRuleEditDlg::self(),
        QTmainwin::self()->Viewport());
    QTdrcRuleEditDlg::self()->show();
}
// End of cDRC functions.


// Default window size, assumes 6X13 chars, 80 cols, 12 rows
// with a 2-pixel margin.
#define DEF_WIDTH 484
#define DEF_HEIGHT 160

namespace {
    const char *MIDX = "midx";
}

QTdrcRuleEditDlg *QTdrcRuleEditDlg::instPtr;

QRdrcRuleEditDlg::QRdrcRuleEditDlg(GRobject c)
{
    instPtr = this;
    dim_caller = c;
    dim_text = 0;
    dim_inhibit = 0;
    dim_edit = 0;
    dim_del = 0;
    dim_undo = 0;
    dim_menu = 0;
    dim_umenu = 0;
    dim_delblk = 0;
    dim_undblk = 0;
    dim_editing_rule = 0;
    dim_start = 0;
    dim_end = 0;

    setWindowTitle(tr("Design Rule Editor");
    setAttribute(Qt::WA_DeleteOnClose);

    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setMargin(2);
    vbox->setSpacing(2);

    // menu bar
    //
    QMenuBar *menubar = new QMenuBar(this);

    // Edit menu.
    QMenu *menu = menubar->addMenu(tr("&Edit"));
    // _Edit, <control>E, dim_edit_proc, 0, 0
    dim_edit = menu->addAction(tr("&Edit"));
    // _Inhibit, <control>I, dim_inhibit_proc, 0, 0
    dim_inhibit = menu->addAction(tr("&Inhibit"));
    // _Delete, <control>D, dim_delete_proc, 0, 0
    dim_del = menu->addAction(tr("&Delete"));
    // _Undo, <control>U, dim_undo_proc, 0, 0
    dim_undo = menu->addAction(tr("&Undo"));
// SEPARATOR
    // _Quit, <control>Q, dim_cancel_proc, 0, 0
    QAction *a = menu->addAction(tr("_Quit"));

    // Rules menu.
    menu = menubar->addMenu(tr("&Rules"));
    // User Defined Rule, 0, 0, 0, "<Branch>"
    QMenu *sm = menu->addMenu(tr("User Defined Rule"));

    for (DRCtest *tst = DRC()->userTests(); tst; tst = tst->next()) {
        QAction *a = sm->addAction(tst->name());
    }
    // Connected, 0, dim_rule_proc, drConnected, 0
    menu->addAction("Connected");
    // NoHoles, 0, dim_rule_proc, drNoHoles, 0
    menu->addAction("NoHoles");
    // Exist, 0, dim_rule_proc, drExist, 0
    menu->addAction("Exist");
    // Overlap", 0, dim_rule_proc, drOverlap, 0
    menu->addAction("Overlap");
    // IfOverlap, 0, dim_rule_proc, drIfOverlap, 0
    menu->addAction("IfOverlap");
    // NoOverlap, 0, dim_rule_proc, drNoOverlap, 0
    menu->addAction("NoOverlap");
    // AnyOverlap, 0, dim_rule_proc, drAnyOverlap, 0
    menu->addAction(("AnyOverlap");
    // PartOverlap, 0, dim_rule_proc, drPartOverlap, 0
    menu->addAction("PartOverlap");
    // AnyNoOverlap, 0, dim_rule_proc, drAnyNoOverlap, 0);
    menu->addAction("AnyNoOverlap");
    // MinArea, 0, dim_rule_proc, drMinArea, 0)
    menu->addAction("MinArea");
    // MaxArea, 0, dim_rule_proc, drMaxArea, 0
    menu->addAction("MaxArea");
    // MinEdgeLength, 0, dim_rule_proc, drMinEdgeLength, 0
    menu->addAction("MinEdgeLength");
    // MaxWidth, 0, dim_rule_proc, drMaxWidth, 0);
    menu->addAction("MaxWidth");
    // MinWidth, 0, dim_rule_proc, drMinWidth, 0
    menu->addAction("MinWidth");
    // MinSpace, 0, dim_rule_proc, drMinSpace, 0
    menu->addAction("MinSpace");
    // MinSpaceTo, 0, dim_rule_proc, drMinSpaceTo, 0
    menu->addAction("MinSpaceTo");
    // MinSpaceFrom", 0, dim_rule_proc, drMinSpaceFrom, 0
    menu->addAction("MinSpaceFrom");
    // MinOverlap, 0, dim_rule_proc, drMinOverlap, 0
    menu->addAction("MinOverlap");
    // MinNoOverlap, 0, dim_rule_proc, drMinNoOverlap, 0
    menu->addAction("MinNoOverlap");

    // Rule Block menu.
    menu = menubar->addMenu(tr("Rule &Block"));
    // New, 0, dim_rule_menu_proc, 0, 0
    menu->addAction(tr("New"));
    // Delete, 0, dim_rule_menu_proc, 1, <CheckItem>
    dim_delblk = menu->addAction(tr("Delete"));
    // Undelete, 0, dim_rule_menu_proc, 2, 0
    dim_undblk = menu->addAction(tr("Undelete"));
    dim_undblk->setEnabled(false);
//Separator

    for (DRCtest *tst = DRC()->userTests(); tst; tst = tst->next()) {
        menu->addAction(tst->name());
    }

    // Help menu.
    menu = menubar->addMenu(tr("&Help"));
    // _Help, <control>H, dim_help_proc, 0, 0);
    menu->addAction(tr("&Help"));

    dim_text = new QTtextEdit();
    vbox->addQidget(dim_text);

/*
    gtk_widget_add_events(dim_text, GDK_BUTTON_PRESS_MASK);
    g_signal_connect(G_OBJECT(dim_text), "button-press-event",
        G_CALLBACK(dim_text_btn_hdlr), 0);
    g_signal_connect_after(G_OBJECT(dim_text), "realize",
        G_CALLBACK(text_realize_proc), 0);

    GtkTextBuffer *textbuf =
        gtk_text_view_get_buffer(GTK_TEXT_VIEW(dim_text));
    const char *bclr = GTKpkg::self()->GetAttrColor(GRattrColorLocSel);
    gtk_text_buffer_create_tag(textbuf, "primary", "background", bclr, NULL);

    gtk_widget_set_size_request(dim_text, DEF_WIDTH, DEF_HEIGHT);

    // The font change pop-up uses this to redraw the widget
    g_object_set_data(G_OBJECT(dim_text), "font_changed",
        (void*)dim_font_changed);
*/

    if (dim_undo)
        dim_undo->setEnablerd(false);
    update();
}


QRdrcRuleEditDlg::~QRdrcRuleEditDlg()
{
    instPtr = 0;
    if (ed_text_input)
        PL()->AbortEdit();
    if (dim_caller)
        QTdev::Deselect(dim_caller);

    DRC()->PopUpRuleEdit(0, MODE_OFF, drNoRule, 0, 0, 0, 0);
}


// Update text.
//
void
QRdrcRuleEditDlg::update()
{
    dim_editing_rule = 0;
    select_range(0, 0);
    stringlist *s0 = rule_list();
    QColor c1 = QTbag::PopupColor(GRattrColorHl1);
    QColor c2 = QTbag::PopupColor(GRattrColorHl4);
    double val = dim_text->get_scroll_value();
    dim_text->clear();
    for (stringlist *l = s0; l && *l->string; l = l->next) {
        char bf[4];
        char *t = l->string;
        bf[0] = *t++;
        bf[1] = *t++;
        bf[2] = 0;
        while (*t && !isspace(*t))
            t++;
        dim_text->setTextColot(c1);
        dim_text->set_chars(bf);
/*
        text_insert_chars_at_point(dim_text, c2, l->string + 2,
            t - l->string - 2, -1);
        if (*t)
            text_insert_chars_at_point(dim_text, 0, t, -1, -1);
        text_insert_chars_at_point(dim_text, 0, "\n", 1, -1);
*/
    }
    dim_text->set_scroll_value(val);
    stringlist::destroy(s0);

    check_sens();
    DRC()->PopUpRuleEdit(0, MODE_UPD, drNoRule, 0, 0, 0, 0);
}


// Update the Rule Block and User Defined Rules menus.
//
void
QRdrcRuleEditDlg::rule_menu_upd()
{
    /*
    GList *gl = gtk_container_get_children(GTK_CONTAINER(dim_menu));
    int cnt = 0;
    for (GList *l = gl; l; l = l->next, cnt++) {
        if (cnt > 3)  // ** skip first four entries **
            gtk_widget_destroy(GTK_WIDGET(l->data));
    }
    g_list_free(gl);

    for (DRCtest *tst = DRC()->userTests(); tst; tst = tst->next()) {
        GtkWidget *mi = gtk_menu_item_new_with_label(tst->name());
        gtk_widget_show(mi);
        g_signal_connect(G_OBJECT(mi), "activate",
            G_CALLBACK(QRdrcRuleEditDlg::dim_rule_menu_proc), tst);
        gtk_menu_shell_append(GTK_MENU_SHELL(dim_menu), mi);
    }

    gl = gtk_container_get_children(GTK_CONTAINER(dim_umenu));
    cnt = 0;
    for (GList *l = gl; l; l = l->next, cnt++)
        gtk_widget_destroy(GTK_WIDGET(l->data));
    g_list_free(gl);

    for (DRCtest *tst = DRC()->userTests(); tst; tst = tst->next()) {
        GtkWidget *mi = gtk_menu_item_new_with_label(tst->name());
        gtk_widget_show(mi);
        g_signal_connect(G_OBJECT(mi), "activate",
            G_CALLBACK(QRdrcRuleEditDlg::dim_rule_proc), (void*)tst->name());
        gtk_menu_shell_append(GTK_MENU_SHELL(dim_umenu), mi);
    }
    */
}


// Save the last operation for undo.
//
void
QRdrcRuleEditDlg::save_last_op(DRCtestDesc *tdold, DRCtestDesc *tdnew)
{
    if (ed_last_delete)
        delete ed_last_delete;
    ed_last_delete = tdold;
    ed_last_insert = tdnew;
    if (dim_undo)
        dim_undo->setEnabled(true);
}


// Select the chars in the range, start=end deselects existing.  In
// GTK-1, selecting gives blue inverse, which turns gray if
// unselected, retaining an indication for the buttons.  GTK-2
// doesn't do this automatically so we provide something similar here.
//
void
QRdrcRuleEditDlg::select_range(int start, int end)
{
    dim_text->select_range(start, end);
    dim_start = start;
    dim_end = end;
    check_sens();
}


void
QRdrcRuleEditDlg::check_sens()
{
    int start, end;
//XXX    text_get_selection_pos(dim_text, &start, &end);
    if (dim_edit) {
        if (start == end)
            dim_edit->setEnabled(false);
        else
            dim_edit->setEnabled(true);
    }
    if (dim_del) {
        if (start == end)
            dim_del->setEnabled(false);
        else
            dim_del->setEnabled(true);
    }
    if (dim_inhibit) {
        if (start == end)
            dim_inhibit->setEnablrd(false);
        else
            dim_inhibit->setEnablrd(true);
    }
    if (start == end)
        ed_rule_selected = -1;
}


#ifdef notdef

// Static function.
// Redraw text after font change.
//
void
QRdrcRuleEditDlg::dim_font_changed()
{
    if (Dim)
        Dim->update();
}


// Static function.
// Pop down the dimensions panel.
//
void
QRdrcRuleEditDlg::dim_cancel_proc(GtkWidget*, void*)
{
    DRC()->PopUpRules(0, MODE_OFF);
}


// Static function.
// Enter help mode.
//
void
QRdrcRuleEditDlg::dim_help_proc(GtkWidget*, void*)
{
    DSPmainWbag(PopUpHelp("xic:dredt"))
}


// Static function.
void
QRdrcRuleEditDlg::dim_inhibit_proc(GtkWidget*, void*)
{
    if (!Dim)
        return;
    if (Dim->ed_rule_selected >= 0) {
        DRCtestDesc *td = Dim->inhibit_selected();
        if (td)
            DRC()->PopUpRules(0, MODE_UPD);
        Dim->ed_rule_selected = -1;
    }
}


// Static function.
// Use the Rule Editor pop-up to edit the parameters associated with
// the selected rule.
//
void
QRdrcRuleEditDlg::dim_edit_proc(GtkWidget*, void*)
{
    if (!LT()->CurLayer() || !Dim)
        return;
    if (Dim->ed_rule_selected >= 0) {
        DRCtestDesc *td = *tech_prm(LT()->CurLayer())->rules_addr();
        for (int i = Dim->ed_rule_selected; i && td; i--, td = td->next()) ;
        if (!td) {
            Dim->ed_rule_selected = -1;
            return;
        }

        Dim->dim_editing_rule = td;
        const DRCtest *ur = td->userRule();
        DRC()->PopUpRuleEdit(0, MODE_ON, td->type(), ur ? ur->name() : 0,
            &Dim->dim_cb, 0, td);
    }
}


// Static function.
// Remove any selected rule from the list, and redraw.
//
void
QRdrcRuleEditDlg::dim_delete_proc(GtkWidget*, void*)
{
    if (!LT()->CurLayer() || !Dim)
        return;
    if (Dim->ed_rule_selected >= 0) {
        DRCtestDesc *td = Dim->remove_selected();
        if (td) {
            Dim->save_last_op(td, 0);
            DRC()->PopUpRules(0, MODE_UPD);
        }
        Dim->ed_rule_selected = -1;
    }
}


// Static function.
// Undo the last insertion or deletion.  A second call undoes the undo,
// etc.
//
void
QRdrcRuleEditDlg::dim_undo_proc(GtkWidget*, void*)
{
    if (!LT()->CurLayer() || !Dim)
        return;
    DRCtestDesc *td = 0;
    if (Dim->ed_last_insert)
        td = DRC()->unlinkRule(Dim->ed_last_insert);
    if (Dim->ed_last_delete)
        DRC()->linkRule(Dim->ed_last_delete);
    Dim->ed_last_insert = Dim->ed_last_delete;
    Dim->ed_last_delete = td;
    DRC()->PopUpRules(0, MODE_UPD);
}


// Static function.
// Handle the rule keyword menu entries, for both regular and
// user-defined rules.  For regular rules, user_name is null, and
// action is the rule id.  For user-defined rules, user_name is the
// rule name, action is undefined and must be set to
// drUserDefinedRule.
//
void
QRdrcRuleEditDlg::dim_rule_proc(GtkWidget *caller, void *user_name)
{
    long action = (long)g_object_get_data(G_OBJECT(caller), MIDX);
    if (!LT()->CurLayer() || !Dim)
        return;
    if (user_name)
        action = drUserDefinedRule;
    Dim->dim_editing_rule = 0;
    DRC()->PopUpRuleEdit(0, MODE_ON, (DRCtype)action, (const char*)user_name,
        Dim->dim_cb, 0, 0);
}


// Static function.
// Callback for the RuleEditor pop-up.  A true return will cause the
// RuleEdit form to pop down.  The string is a complete rule
// specification.
//
bool
QRdrcRuleEditDlg::dim_cb(const char *string, void*)
{
    if (!LT()->CurLayer() || !Dim)
        return (true);
    if (!string || !*string)
        return (false);
    char *tok = lstring::gettok(&string);
    if (!tok)
        return (false);
    DRCtype type = DRCtestDesc::ruleType(tok);
    DRCtest *tst = 0;
    if (type == drNoRule) {
        // Not a regular rule, check the user rules.
        for (tst = DRC()->userTests(); tst; tst = tst->next()) {
            if (lstring::cieq(tst->name(), tok)) {
                type = drUserDefinedRule;
                break;
            }
        }
        if (!tst) {
            delete [] tok;
            return (false);  // "can't happen"
        }
    }
    delete [] tok;

    DRCtestDesc *td;
    if (Dim->dim_editing_rule)
        td = new DRCtestDesc(Dim->dim_editing_rule, 0, 0, 0);
    else
        td = new DRCtestDesc(type, LT()->CurLayer());
    const char *emsg = td->parse(string, 0, tst);
    if (emsg) {
        Log()->ErrorLogV(mh::DRC, "Rule parse returned error: %s.", emsg);
        return (false);
    }

    // See if we can identify an existing rule that can/should be
    // replaced by the new rule.

    DRCtestDesc *oldrule = 0;
    if (td->type() != drUserDefinedRule && !Dim->dim_editing_rule) {
        char *rstr = td->regionString();
        if (!rstr) {
            if (DRCtestDesc::requiresTarget(td->type())) {
                CDl *tld = td->targetLayer();
                if (tld)
                    oldrule = DRCtestDesc::findRule(td->layer(), td->type(),
                        tld);
            }
            else
                oldrule = DRCtestDesc::findRule(td->layer(), td->type(), 0);
        }
        else
            delete [] rstr;
    }

    td->initialize();
    DRCtestDesc *erule = 0;
    if (Dim->dim_editing_rule)
        erule = DRC()->unlinkRule(Dim->dim_editing_rule);
    else if (oldrule)
        erule = DRC()->unlinkRule(oldrule);
    DRC()->linkRule(td);
    Dim->save_last_op(erule, td);
    Dim->dim_editing_rule = 0;
    DRC()->PopUpRules(0, MODE_UPD);

    return (true);
}


// Static function.
// If any instances of the named user-defined rule are found, pop up
// an informational message.
//
void
QRdrcRuleEditDlg::dim_show_msg(const char *name)
{
    char buf[512];
    snprintf(buf, sizeof(buf),
        "Existing instances of %s have been inhibited.  These\n"
        "implement the old rule so must be recreated for the new rule to\n"
        "have effect.  Old rules are found on:", name);

    bool found = false;
    CDl *ld;
    CDlgen lgen(Physical);
    while ((ld = lgen.next()) != 0) {
        for (DRCtestDesc *td = *tech_prm(ld)->rules_addr();
                td; td = td->next()) {
            if (td->matchUserRule(name)) {
                strcat(buf, " ");
                strcat(buf, ld->name());
                found = true;
                break;
            }
        }
    }
    if (found) {
        strcat(buf, ".");
        DSPmainWbag(PopUpMessage(buf, false))
    }
}


// Static function.
// Callback from the text editor popup.
//
bool
QRdrcRuleEditDlg::dim_editsave(const char *fname, void*, XEtype type)
{
    if (type == XE_QUIT)
        unlink(fname);
    else if (type == XE_SAVE) {
        FILE *fp = filestat::open_file(fname, "r");
        if (!fp) {
            Log()->ErrorLog(mh::Initialization, filestat::error_msg());
            return (true);
        }

        const char *name;
        bool ret = DRC()->techParseUserRule(fp, &name);
        fclose(fp);
        if (ret && Dim && DRC()->userTests()) {
            Dim->rule_menu_upd();
            dim_show_msg(name);
            DRC()->PopUpRules(0, MODE_UPD);
        }
    }
    return (true);
}


// Static function.
// Edit a user-defined rule block.
//
void
QRdrcRuleEditDlg::dim_rule_menu_proc(GtkWidget *caller, void *client_data)
{
    long type = (long)g_object_get_data(G_OBJECT(caller), MIDX);
    if (type == 2) {
        // Undelete button
        if (!DRC()->userTests())
            DRC()->setUserTests(Dim->ed_usertest);
        else {
            DRCtest *tst = DRC()->userTests();
            while (tst->next())
                tst = tst->next();
            tst->setNext(Dim->ed_usertest);
        }
        Dim->user_rule_mod(Uuninhibit);
        Dim->ed_usertest = 0;
        gtk_widget_set_sensitive(Dim->dim_undblk, false);
        Dim->rule_menu_upd();
        return;
    }
    if (type == 1) {
        // delete button;
        return;
    }
    DRCtest *tst = (DRCtest*)client_data;
    if (GTKdev::GetStatus(Dim->dim_delblk)) {
        GTKdev::Deselect(Dim->dim_delblk);
        if (tst) {
            DRCtest *tp = 0;
            for (DRCtest *tx = DRC()->userTests(); tx; tx = tx->next()) {
                if (tx == tst) {
                    if (!tp)
                        DRC()->setUserTests(tx->next());
                    else
                        tp->setNext(tx->next());
                    tst->setNext(0);
                    Dim->user_rule_mod(Udelete);
                    delete Dim->ed_usertest;
                    Dim->ed_usertest = tst;
                    Dim->user_rule_mod(Uinhibit);
                    gtk_widget_set_sensitive(Dim->dim_undblk, true);
                    Dim->rule_menu_upd();
                    break;
                }
                tp = tx;
            }
            return;
        }
    }
    sLstr lstr;
    if (tst)
        tst->print(&lstr);
    else
        lstr.add("");
    char *fname = filestat::make_temp("xi");
    FILE *fp = fopen(fname, "w");
    if (!fp) {
        Log()->PopUpErr("Error: couldn't open temporary file.");
        return;
    }
    fputs(lstr.string(), fp);
    fclose(fp);
    DSPmainWbag(PopUpTextEditor(fname, dim_editsave, tst, false))
}


// Static function.
// Handle button presses in the text area.  If neither edit mode or
// delete mode is active, highlight the rule pointed to.  Otherwise,
// perform the operation on the pointed-to rule.
//
int
QRdrcRuleEditDlg::dim_text_btn_hdlr(GtkWidget *caller, GdkEvent *event, void*)
{
    if (!Dim)
        return (false);
    if (event->type != GDK_BUTTON_PRESS)
        return (true);

    char *string = text_get_chars(caller, 0, -1);
    int x = (int)event->button.x;
    int y = (int)event->button.y;
    gtk_text_view_window_to_buffer_coords(GTK_TEXT_VIEW(caller),
        GTK_TEXT_WINDOW_WIDGET, x, y, &x, &y);
    GtkTextIter ihere, iline;
    gtk_text_view_get_iter_at_location(GTK_TEXT_VIEW(caller), &ihere, x, y);
    gtk_text_view_get_line_at_y(GTK_TEXT_VIEW(caller), &iline, y, 0);
    y = gtk_text_iter_get_line(&iline);

    int line = 0;
    int rule = 0;
    int start = 0;
    for (const char *s = string; *s; s++) {
        if (line == y)
            break;
        if (*s == '\n') {
            line++;
            if (s > string && *(s-1) == '\\')
                continue;
            rule++;
            start = (s + 1 - string);
        }
    }
    if (Dim->ed_rule_selected >= 0 && Dim->ed_rule_selected == rule) {
        delete [] string;
        Dim->select_range(0, 0);
        return (true);
    }
    Dim->ed_rule_selected = rule;

    // Find the end of the rule.
    int end = -1;
    for (const char *s = string + start; *s; s++) {
        if (*s == '\n') {
            if (s > string && *(s-1) == '\\')
                continue;
            end = (s - string);
                break;
        }
    }

    Dim->select_range(start + 2, end);
    delete [] string;
    return (true);
}

#endif

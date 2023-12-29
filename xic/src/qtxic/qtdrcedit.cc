
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
#include "qtinterf/qttextw.h"
#include "miscutil/filestat.h"

#include <QLayout>
#include <QMenuBar>
#include <QToolBar>
#include <QToolButton>
#include <QMenu>
#include <QAction>
#include <QMouseEvent>


//-----------------------------------------------------------------------------
// QTdrcRuleEditDlg:  Dialog to display a listing of design rules for
// the current layer for editing.
// Called from main menu: DRC/Edit Rules.
//
// Help system keywords used:
//  xic:dredt

#ifdef __APPLE__
#define USE_QTOOLBAR
#endif

void
cDRC::PopUpRules(GRobject caller, ShowMode mode)
{
    if (!QTdev::exists() || !QTmainwin::exists())
        return;
    if (mode == MODE_OFF) {
        delete QTdrcRuleEditDlg::self();
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

    new QTdrcRuleEditDlg(caller);

    QTdrcRuleEditDlg::self()->set_transient_for(QTmainwin::self());
    QTdev::self()->SetPopupLocation(GRloc(), QTdrcRuleEditDlg::self(),
        QTmainwin::self()->Viewport());
    QTdrcRuleEditDlg::self()->show();
}
// End of cDRC functions.


// Default window size, assumes 6X13 chars, 80 cols, 12 rows
// with a 2-pixel margin.
#define DEF_WIDTH 484
#define DEF_HEIGHT 160


QTdrcRuleEditDlg *QTdrcRuleEditDlg::instPtr;

QTdrcRuleEditDlg::QTdrcRuleEditDlg(GRobject c)
{
    instPtr = this;
    dim_caller = c;
    dim_text = 0;
    dim_edit = 0;
    dim_inhibit = 0;
    dim_del = 0;
    dim_undo = 0;
    dim_menu = 0;
    dim_umenu = 0;
    dim_rbmenu = 0;
    dim_delblk = 0;
    dim_undblk = 0;
    dim_editing_rule = 0;
    dim_start = 0;
    dim_end = 0;

    setWindowTitle(tr("Design Rule Editor"));
    setAttribute(Qt::WA_DeleteOnClose);

    QMargins qmtop(2, 2, 2, 2);
    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(qmtop);
    vbox->setSpacing(2);

    // menu bar
    //
#ifdef USE_QTOOLBAR
    QToolBar *menubar = new QToolBar(this);
#else
    QMenuBar *menubar = new QMenuBar(this);
#endif
    vbox->addWidget(menubar);

    // Edit menu.
    QAction *a;
#ifdef USE_QTOOLBAR
    a = menubar->addAction(tr("&Edit"));
    QMenu *menu = new QMenu();
    a->setMenu(menu);
    QToolButton *tb = dynamic_cast<QToolButton*>(menubar->widgetForAction(a));
    if (tb)
        tb->setPopupMode(QToolButton::InstantPopup);
#else
    QMenu *menu = menubar->addMenu(tr("&Edit"));
#endif
    // _Edit, <control>E, dim_edit_proc, 0, 0
    dim_edit = menu->addAction(tr("&Edit"));
    dim_edit->setShortcut(QKeySequence("Ctrl+E"));
    // _Inhibit, <control>I, dim_inhibit_proc, 0, 0
    dim_inhibit = menu->addAction(tr("&Inhibit"));
    dim_inhibit->setShortcut(QKeySequence("Ctrl+I"));
    // _Delete, <control>D, dim_delete_proc, 0, 0
    dim_del = menu->addAction(tr("&Delete"));
    dim_del->setShortcut(QKeySequence("Ctrl+D"));
    // _Undo, <control>U, dim_undo_proc, 0, 0
    dim_undo = menu->addAction(tr("&Undo"));
    dim_del->setShortcut(QKeySequence("Ctrl+U"));
    menu->addSeparator();
    // _Quit, <control>Q, dim_cancel_proc, 0, 0
    a = menu->addAction(tr("&Quit"));
    a->setShortcut(QKeySequence("Ctrl+Q"));
    connect(menu, SIGNAL(triggered(QAction*)),
        this, SLOT(edit_menu_slot(QAction*)));

    // Rules menu.
#ifdef USE_QTOOLBAR
    a = menubar->addAction(tr("&Rules"));
    dim_menu = new QMenu();
    a->setMenu(dim_menu);
    tb = dynamic_cast<QToolButton*>(menubar->widgetForAction(a));
    if (tb)
        tb->setPopupMode(QToolButton::InstantPopup);
#else
    dim_menu = menubar->addMenu(tr("&Rules"));
#endif
    // User Defined Rule, 0, 0, 0, "<Branch>"
    dim_umenu = dim_menu->addMenu(tr("User Defined Rule"));
    for (DRCtest *tst = DRC()->userTests(); tst; tst = tst->next()) {
        dim_umenu->addAction(tst->name());
    }
    connect(dim_umenu, SIGNAL(triggered(QAction*)),
        this, SLOT(user_menu_slot(QAction*)));

    // Connected, 0, dim_rule_proc, drConnected, 0
    a = dim_menu->addAction("Connected");
    a->setData(drConnected);
    // NoHoles, 0, dim_rule_proc, drNoHoles, 0
    a = dim_menu->addAction("NoHoles");
    a->setData(drNoHoles);
    // Exist, 0, dim_rule_proc, drExist, 0
    a = dim_menu->addAction("Exist");
    a->setData(drExist);
    // Overlap", 0, dim_rule_proc, drOverlap, 0
    a = dim_menu->addAction("Overlap");
    a->setData(drOverlap);
    // IfOverlap, 0, dim_rule_proc, drIfOverlap, 0
    a = dim_menu->addAction("IfOverlap");
    a->setData(drIfOverlap);
    // NoOverlap, 0, dim_rule_proc, drNoOverlap, 0
    a = dim_menu->addAction("NoOverlap");
    a->setData(drNoOverlap);
    // AnyOverlap, 0, dim_rule_proc, drAnyOverlap, 0
    a = dim_menu->addAction("AnyOverlap");
    a->setData(drAnyOverlap);
    // PartOverlap, 0, dim_rule_proc, drPartOverlap, 0
    a = dim_menu->addAction("PartOverlap");
    a->setData(drPartOverlap);
    // AnyNoOverlap, 0, dim_rule_proc, drAnyNoOverlap, 0);
    a = dim_menu->addAction("AnyNoOverlap");
    a->setData(drAnyNoOverlap);
    // MinArea, 0, dim_rule_proc, drMinArea, 0)
    a = dim_menu->addAction("MinArea");
    a->setData(drMinArea);
    // MaxArea, 0, dim_rule_proc, drMaxArea, 0
    a = dim_menu->addAction("MaxArea");
    a->setData(drMaxArea);
    // MinEdgeLength, 0, dim_rule_proc, drMinEdgeLength, 0
    a = dim_menu->addAction("MinEdgeLength");
    a->setData(drMinEdgeLength);
    // MaxWidth, 0, dim_rule_proc, drMaxWidth, 0);
    a = dim_menu->addAction("MaxWidth");
    a->setData(drMaxWidth);
    // MinWidth, 0, dim_rule_proc, drMinWidth, 0
    a = dim_menu->addAction("MinWidth");
    a->setData(drMinWidth);
    // MinSpace, 0, dim_rule_proc, drMinSpace, 0
    a = dim_menu->addAction("MinSpace");
    a->setData(drMinSpace);
    // MinSpaceTo, 0, dim_rule_proc, drMinSpaceTo, 0
    a = dim_menu->addAction("MinSpaceTo");
    a->setData(drMinSpaceTo);
    // MinSpaceFrom", 0, dim_rule_proc, drMinSpaceFrom, 0
    a = dim_menu->addAction("MinSpaceFrom");
    a->setData(drMinSpaceFrom);
    // MinOverlap, 0, dim_rule_proc, drMinOverlap, 0
    a = dim_menu->addAction("MinOverlap");
    a->setData(drMinOverlap);
    // MinNoOverlap, 0, dim_rule_proc, drMinNoOverlap, 0
    a = dim_menu->addAction("MinNoOverlap");
    a->setData(drMinNoOverlap);
    connect(dim_menu, SIGNAL(triggered(QAction*)),
        this, SLOT(rules_menu_slot(QAction*)));

    // Rule Block menu.
#ifdef USE_QTOOLBAR
    a = menubar->addAction(tr("Rule &Block"));
    dim_rbmenu = new QMenu();
    a->setMenu(dim_rbmenu);
    tb = dynamic_cast<QToolButton*>(menubar->widgetForAction(a));
    if (tb)
        tb->setPopupMode(QToolButton::InstantPopup);
#else
    dim_rbmenu = menubar->addMenu(tr("Rule &Block"));
#endif
    // New, 0, dim_rule_menu_proc, 0, 0
    dim_rbmenu->addAction(tr("New"));
    // Delete, 0, dim_rule_menu_proc, 1, <CheckItem>
    dim_delblk = dim_rbmenu->addAction(tr("Delete"));
    // Undelete, 0, dim_rule_menu_proc, 2, 0
    dim_undblk = dim_rbmenu->addAction(tr("Undelete"));
    dim_undblk->setEnabled(false);
    dim_rbmenu->addSeparator();
    for (DRCtest *tst = DRC()->userTests(); tst; tst = tst->next()) {
        dim_rbmenu->addAction(tst->name());
    }
    connect(dim_rbmenu, SIGNAL(triggered(QAction*)),
        this, SLOT(ruleblk_menu_slot(QAction*)));

    // Help menu.
#ifdef USE_QTOOLBAR
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    menubar->addAction(tr("&Help"), Qt::CTRL|Qt::Key_H, this,
        SLOT(help_slot()));
#else
    a = menubar->addAction(tr("&Help"), this, SLOT(help_slot()));
    a->setShortcut(QKeySequence("Ctrl+H"));
#endif
#else
    menu = menubar->addMenu(tr("&Help"));
    // _Help, <control>H, dim_help_proc, 0, 0);
    a = menu->addAction(tr("&Help"), this, SLOT(help_slot()));
    a->setShortcut(QKeySequence("Ctrl+H"));
#endif

    dim_text = new QTtextEdit();
    vbox->addWidget(dim_text);
    dim_text->setReadOnly(true);
    dim_text->setMouseTracking(true);
    connect(dim_text, SIGNAL(press_event(QMouseEvent*)),
        this, SLOT(mouse_press_slot(QMouseEvent*)));

    QFont *fnt;
    if (FC.getFont(&fnt, FNT_FIXED))
        dim_text->setFont(*fnt);
    connect(QTfont::self(), SIGNAL(fontChanged(int)),
        this, SLOT(font_changed_slot(int)), Qt::QueuedConnection);

    if (dim_undo)
        dim_undo->setEnabled(false);
    update();
}


QTdrcRuleEditDlg::~QTdrcRuleEditDlg()
{
    instPtr = 0;
    if (ed_text_input)
        PL()->AbortEdit();
    if (dim_caller)
        QTdev::Deselect(dim_caller);

    DRC()->PopUpRuleEdit(0, MODE_OFF, drNoRule, 0, 0, 0, 0);
}


QSize
QTdrcRuleEditDlg::sizeHint() const
{
    int fw, fh;
    QTfont::stringBounds(0, FNT_FIXED, &fw, &fh);
    return (QSize(80*fw + 4, 32*fh));
}


// Update text.
//
void
QTdrcRuleEditDlg::update()
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
        dim_text->setTextColor(c1);
        dim_text->insertPlainText(bf);
        dim_text->setTextColor(c2);
        char ctmp = *t;
        *t = 0;
        dim_text->insertPlainText(l->string + 2);
        *t = ctmp;
        dim_text->setTextColor(QColor("black"));
        if (*t)
            dim_text->insertPlainText(t);
        dim_text->insertPlainText("\n");
    }
    dim_text->set_scroll_value(val);
    stringlist::destroy(s0);

    check_sens();
    DRC()->PopUpRuleEdit(0, MODE_UPD, drNoRule, 0, 0, 0, 0);
}


// Update the Rule Block and User Defined Rules menus.
//
void
QTdrcRuleEditDlg::rule_menu_upd()
{
    QList<QAction*> alist = dim_rbmenu->actions();
    int sz = alist.size();
    for (int i = 0; i < sz; i++) {
        if (i > 3) {
            // ** skip first four entries **
            dim_rbmenu->removeAction(alist.at(i));
        }
    }
    for (DRCtest *tst = DRC()->userTests(); tst; tst = tst->next()) {
        dim_rbmenu->addAction(tst->name());
    }

    alist = dim_umenu->actions();
    sz = alist.size();
    for (int i = 0; i < sz; i++) {
        dim_umenu->removeAction(alist.at(i));
    }
    for (DRCtest *tst = DRC()->userTests(); tst; tst = tst->next()) {
        dim_umenu->addAction(tst->name());
    }
}


// Save the last operation for undo.
//
void
QTdrcRuleEditDlg::save_last_op(DRCtestDesc *tdold, DRCtestDesc *tdnew)
{
    if (ed_last_delete)
        delete ed_last_delete;
    ed_last_delete = tdold;
    ed_last_insert = tdnew;
    if (dim_undo)
        dim_undo->setEnabled(true);
}


// Select the chars in the range, start=end deselects existing.
//
void
QTdrcRuleEditDlg::select_range(int start, int end)
{
    dim_text->select_range(start, end);
    dim_start = start;
    dim_end = end;
    check_sens();
}


void
QTdrcRuleEditDlg::check_sens()
{
    bool has_sel = dim_text->has_selection();
    if (dim_edit)
        dim_edit->setEnabled(has_sel);
    if (dim_del)
        dim_del->setEnabled(has_sel);
    if (dim_inhibit)
        dim_inhibit->setEnabled(has_sel);
    if (!has_sel)
        ed_rule_selected = -1;
}


// Static function.
// Callback for the RuleEditor pop-up.  A true return will cause the
// RuleEdit form to pop down.  The string is a complete rule
// specification.
//
bool
QTdrcRuleEditDlg::dim_cb(const char *string, void*)
{
    if (!LT()->CurLayer() || !instPtr)
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
    if (instPtr->dim_editing_rule)
        td = new DRCtestDesc(instPtr->dim_editing_rule, 0, 0, 0);
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
    if (td->type() != drUserDefinedRule && !instPtr->dim_editing_rule) {
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
    if (instPtr->dim_editing_rule)
        erule = DRC()->unlinkRule(instPtr->dim_editing_rule);
    else if (oldrule)
        erule = DRC()->unlinkRule(oldrule);
    DRC()->linkRule(td);
    instPtr->save_last_op(erule, td);
    instPtr->dim_editing_rule = 0;
    DRC()->PopUpRules(0, MODE_UPD);

    return (true);
}


// Static function.
// If any instances of the named user-defined rule are found, pop up
// an informational message.
//
void
QTdrcRuleEditDlg::dim_show_msg(const char *name)
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
QTdrcRuleEditDlg::dim_editsave(const char *fname, void*, XEtype type)
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
        if (ret && instPtr && DRC()->userTests()) {
            instPtr->rule_menu_upd();
            dim_show_msg(name);
            DRC()->PopUpRules(0, MODE_UPD);
        }
    }
    return (true);
}


void
QTdrcRuleEditDlg::edit_menu_slot(QAction *a)
{
    if (a == dim_edit) {
        if (!LT()->CurLayer())
            return;
        if (ed_rule_selected >= 0) {
            DRCtestDesc *td = *tech_prm(LT()->CurLayer())->rules_addr();
            for (int i = ed_rule_selected; i && td; i--, td = td->next()) ;
            if (!td) {
                ed_rule_selected = -1;
                return;
            }

            dim_editing_rule = td;
            const DRCtest *ur = td->userRule();
            DRC()->PopUpRuleEdit(0, MODE_ON, td->type(), ur ? ur->name() : 0,
                &dim_cb, 0, td);
        }
    }
    else if (a == dim_inhibit) {
        if (ed_rule_selected >= 0) {
            DRCtestDesc *td = inhibit_selected();
            if (td)
                DRC()->PopUpRules(0, MODE_UPD);
            ed_rule_selected = -1;
        }
    }
    else if (a == dim_del) {
        if (!LT()->CurLayer())
            return;
        if (ed_rule_selected >= 0) {
            DRCtestDesc *td = remove_selected();
            if (td) {
                save_last_op(td, 0);
                DRC()->PopUpRules(0, MODE_UPD);
            }
            ed_rule_selected = -1;
        }
    }
    else if (a == dim_undo) {
        if (!LT()->CurLayer())
            return;
        DRCtestDesc *td = 0;
        if (ed_last_insert)
            td = DRC()->unlinkRule(ed_last_insert);
        if (ed_last_delete)
            DRC()->linkRule(ed_last_delete);
        ed_last_insert = ed_last_delete;
        ed_last_delete = td;
        DRC()->PopUpRules(0, MODE_UPD);
    }
    else 
        DRC()->PopUpRules(0, MODE_OFF);
}


void
QTdrcRuleEditDlg::user_menu_slot(QAction *a)
{
    // Handle the user rule keyword menu entries.
    if (!LT()->CurLayer())
        return;

    QByteArray ba = a->text().toLatin1();
    dim_editing_rule = 0;
    DRC()->PopUpRuleEdit(0, MODE_ON, drUserDefinedRule, ba.constData(),
        dim_cb, 0, 0);
}


void
QTdrcRuleEditDlg::rules_menu_slot(QAction *a)
{
    // Handle the rule keyword menu entries from the Rules menu.
    if (!LT()->CurLayer())
        return;

    int rule = a->data().toInt();
    dim_editing_rule = 0;
    DRC()->PopUpRuleEdit(0, MODE_ON, (DRCtype)rule, 0, dim_cb, 0, 0);
}


void
QTdrcRuleEditDlg::ruleblk_menu_slot(QAction *a)
{
    QString qs = a->text();
    if (qs == "Undelete") {
        // Undelete button
        if (!DRC()->userTests())
            DRC()->setUserTests(ed_usertest);
        else {
            DRCtest *tst = DRC()->userTests();
            while (tst->next())
                tst = tst->next();
            tst->setNext(ed_usertest);
        }
        user_rule_mod(Uuninhibit);
        ed_usertest = 0;
        dim_undblk->setEnabled(false);
        rule_menu_upd();
        return;
    }
    if (qs == "Delete") {
        // delete button;
        return;
    }

    DRCtest *tst = 0;
    for (DRCtest *tx = DRC()->userTests(); tx; tx = tx->next()) {
        if (qs == tx->name()) {
            tst = tx;
            break;
        }
    }

    if (QTdev::GetStatus(dim_delblk)) {
        QTdev::Deselect(dim_delblk);
        if (tst) {
            DRCtest *tp = 0;
            for (DRCtest *tx = DRC()->userTests(); tx; tx = tx->next()) {
                if (tx == tst) {
                    if (!tp)
                        DRC()->setUserTests(tx->next());
                    else
                        tp->setNext(tx->next());
                    tx->setNext(0);
                    user_rule_mod(Udelete);
                    delete ed_usertest;
                    ed_usertest = tx;
                    user_rule_mod(Uinhibit);
                    dim_undblk->setEnabled(true);
                    rule_menu_upd();
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


void
QTdrcRuleEditDlg::help_slot()
{
    DSPmainWbag(PopUpHelp("xic:dredt"))
}


void
QTdrcRuleEditDlg::mouse_press_slot(QMouseEvent *ev)
{
    // Handle button presses in the text area.  If neither edit mode or
    // delete mode is active, highlight the rule pointed to.  Otherwise,
    // perform the operation on the pointed-to rule.

    if (ev->type() != QEvent::MouseButtonPress) {
        ev->ignore();
        return;
    }
    ev->accept();

    char *str = dim_text->get_chars();
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    int xx = ev->position().x();
    int yy = ev->position().y();
#else
    int xx = ev->x();
    int yy = ev->y();
#endif
    QTextCursor cur = dim_text->cursorForPosition(QPoint(xx, yy));
    int posn = cur.position();

    int rule = 0;
    int start = 0;
    for (int i = 0; i < posn; i++) {
        if (str[i] == '\n') {
            if (i > 0 && str[i-1] == '\\')
                continue;
            rule++;
            start = (i + 1);
        }
    }

    // Find the end of the rule.
    int end = -1;
    int i = start;
    for ( ; str[i]; i++) {
        if (str[i] == '\n') {
            if (str[i-1] == '\\')
                continue;
            break;
        }
    }
    end = i;

    if (ed_rule_selected >= 0 && ed_rule_selected == rule) {
        delete [] str;
        select_range(0, 0);
        return;
    }
    ed_rule_selected = rule;

    select_range(start + 2, end);
    delete [] str;
}


void
QTdrcRuleEditDlg::font_changed_slot(int fnum)
{
    if (fnum == FNT_FIXED) {
        QFont *fnt;
        if (FC.getFont(&fnt, FNT_FIXED))
            dim_text->setFont(*fnt);
        update();
    }
}


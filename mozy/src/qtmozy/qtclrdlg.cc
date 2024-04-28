
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

#include "qthelp.h"
#include "qtviewer.h"
#include "qtclrdlg.h"
#include "help/help_startup.h"
#include "help/help_cache.h"
#include "help/help_topic.h"

#include <QApplication>
#include <QGuiApplication>
#include <QClipboard>
#include <QLayout>
#include <QLineEdit>
#include <QToolButton>
#include <QPushButton>
#include <QLabel>


// The Default Colors entry widget.  This allows changing the colors
// used when there is no <body> tag, and some attribute colors.

QTmozyClrDlg::QTmozyClrDlg(QWidget *prnt) : QDialog(prnt), QTbag(this)
{
    clr_bgclr = 0;
    clr_bgimg = 0;
    clr_text = 0;
    clr_link = 0;
    clr_vislink = 0;
    clr_actfglink = 0;
    clr_imgmap = 0;
    clr_sel = 0;
    clr_listbtn = 0;
    clr_listpop = 0;

    setWindowTitle(tr("Default Colors"));
    setAttribute(Qt::WA_DeleteOnClose);

    QMargins qmtop(2, 2, 2, 2);
    QGridLayout *grid = new QGridLayout(this);
    grid->setContentsMargins(qmtop);
    grid->setSpacing(2);

    QLabel *label = new QLabel(tr("Background color"));
    grid->addWidget(label, 0, 0);

    clr_bgclr = new QLineEdit();
    grid->addWidget(clr_bgclr, 0, 1);

    label = new QLabel(tr("Background image"));
    grid->addWidget(label, 1, 0);

    clr_bgimg = new QLineEdit();
    grid->addWidget(clr_bgimg, 1, 1);

    label = new QLabel(tr("Text color"));
    grid->addWidget(label, 2, 0);

    clr_text = new QLineEdit();
    grid->addWidget(clr_text, 2, 1);

    label = new QLabel(tr("Link color"));
    grid->addWidget(label, 3, 0);

    clr_link = new QLineEdit();
    grid->addWidget(clr_link, 3, 1);

    label = new QLabel(tr("Visited link color"));
    grid->addWidget(label, 4, 0);

    clr_vislink = new QLineEdit();
    grid->addWidget(clr_vislink, 4, 1);

    label = new QLabel(tr("Activated link color"));
    grid->addWidget(label, 5, 0);

    clr_actfglink = new QLineEdit();
    grid->addWidget(clr_actfglink, 5, 1);

    label = new QLabel(tr("Select color"));
    grid->addWidget(label, 6, 0);

    clr_sel = new QLineEdit();
    grid->addWidget(clr_sel, 6, 1);

    label = new QLabel(tr("Imagemap border color"));
    grid->addWidget(label, 7, 0);

    clr_imgmap = new QLineEdit();
    grid->addWidget(clr_imgmap, 7, 1);

    QHBoxLayout *hbox = new QHBoxLayout();
    grid->addLayout(hbox, 8, 0, 1, 2);

    clr_listbtn = new QToolButton();
    clr_listbtn->setText(tr("Colors"));
    hbox->addWidget(clr_listbtn);
    clr_listbtn->setCheckable(true);
    connect(clr_listbtn, SIGNAL(toggled(bool)),
        this, SLOT(colors_btn_slot(bool)));

    QToolButton *tbtn = new QToolButton();
    tbtn->setText(tr("Apply"));
    hbox->addWidget(tbtn);
    connect(tbtn, SIGNAL(clicked()), this, SLOT(apply_btn_slot()));

    QPushButton *btn = new QPushButton(tr("Dismiss"));
    btn->setObjectName("Default");
    hbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(quit_btn_slot()));

    update();
}


QTmozyClrDlg::~QTmozyClrDlg()
{
    if (p_caller)
        QTdev::Deselect(p_caller);
    if (p_usrptr)
        *p_usrptr = 0;
}


#ifdef Q_OS_MACOS
#define DLGTYPE QTmozyClrDlg
#include "qtinterf/qtmacos_event.h"
#endif


namespace {
    void chkclr_set(QLineEdit *led, const char *nm)
    {
        if (!nm)
            nm = "";
        QByteArray ba = led->text().toLatin1();
        if (strcmp(nm, ba.constData()))
            led->setText(nm);
    }

    bool chkclr_apply(QLineEdit *led, const char *nm, const char *cid)
    {
        if (!nm)
            nm = "";
        QByteArray ba = led->text().toLatin1();
        if (strcmp(nm, ba.constData())) {
            HLP()->context()->setDefaultColor(cid, ba.constData());
            return (true);
        }
        return (false);
    }
}


void
QTmozyClrDlg::update()
{
    HLPcontext *cx = HLP()->context();
    const char *cc = cx->getDefaultColor(HLP_DefaultBgColor);
    chkclr_set(clr_bgclr, cc);
    cc = cx->getDefaultColor(HLP_DefaultBgImage);
    chkclr_set(clr_bgimg, cc);
    cc = cx->getDefaultColor(HLP_DefaultFgText);
    chkclr_set(clr_text, cc);
    cc = cx->getDefaultColor(HLP_DefaultFgLink);
    chkclr_set(clr_link, cc);
    cc = cx->getDefaultColor(HLP_DefaultFgVisitedLink);
    chkclr_set(clr_vislink, cc);
    cc = cx->getDefaultColor(HLP_DefaultFgActiveLink);
    chkclr_set(clr_actfglink, cc);
    cc = cx->getDefaultColor(HLP_DefaultBgSelect);
    chkclr_set(clr_sel, cc);
    cc = cx->getDefaultColor(HLP_DefaultFgImagemap);
    chkclr_set(clr_imgmap, cc);
}


// Static function.
// Insert the selected rgb.txt entry into the color selector.
//
void
QTmozyClrDlg::clr_list_callback(const char *string, void*)
{
    if (string) {
        // Put the color name in the "primary" clipboard, so we can
        // paste it into the entry areas.

        lstring::advtok(&string);
        lstring::advtok(&string);
        lstring::advtok(&string);
        QClipboard *cb = QGuiApplication::clipboard();
        cb->setText(string);
    }
}


void
QTmozyClrDlg::colors_btn_slot(bool state)
{
    if (!clr_listpop && state) {
        stringlist *list = GRcolorList::listColors();
        if (!list) {
            QTdev::SetStatus(sender(), false);
            return;
        }
        clr_listpop = PopUpList(list, "Colors",
            "click to select, color name to clipboard",
            clr_list_callback, 0, false, false);
        stringlist::destroy(list);
        if (clr_listpop)
            clr_listpop->register_usrptr((void**)&clr_listpop);
        clr_listpop->set_visible(true);
    }
    else if (clr_listpop && !state)
        clr_listpop->popdown();
}


void
QTmozyClrDlg::apply_btn_slot()
{
    bool body_chg = false;
    HLPcontext *cx = HLP()->context();

    const char *cc = cx->getDefaultColor(HLP_DefaultBgImage);
    if (chkclr_apply(clr_bgimg, cc, HLP_DefaultBgImage))
        body_chg = true;

    cc = cx->getDefaultColor(HLP_DefaultBgColor);
    if (chkclr_apply(clr_bgclr, cc, HLP_DefaultBgColor))
        body_chg = true;

    cc = cx->getDefaultColor(HLP_DefaultFgText);
    if (chkclr_apply(clr_text, cc, HLP_DefaultFgText))
        body_chg = true;

    cc = cx->getDefaultColor(HLP_DefaultFgLink);
    if (chkclr_apply(clr_link, cc, HLP_DefaultFgLink))
        body_chg = true;

    cc = cx->getDefaultColor(HLP_DefaultFgVisitedLink);
    if (chkclr_apply(clr_vislink, cc, HLP_DefaultFgVisitedLink))
        body_chg = true;

    cc = cx->getDefaultColor(HLP_DefaultFgActiveLink);
    if (chkclr_apply(clr_actfglink, cc, HLP_DefaultFgActiveLink))
        body_chg = true;

    // The keys above operate through inclusion of a <body> tag
    // that sets background color or image, which is added to HTML
    // text that has no body tag.  This probably includes the
    // user's help text.  The keys below operate by actually
    // changing the color definitions in the widget.

    cc = cx->getDefaultColor(HLP_DefaultBgSelect);
    if (chkclr_apply(clr_sel, cc, HLP_DefaultBgSelect)) {
        cc = cx->getDefaultColor(HLP_DefaultBgSelect);
        for (HLPtopic *t = HLP()->context()->topList(); t;
                t = t->sibling()) {
            QThelpDlg *w = (QThelpDlg*)HelpWidget::get_widget(t);
            w->viewer()->set_select_color(cc);
        }
    }

    cc = cx->getDefaultColor(HLP_DefaultFgImagemap);
    if (chkclr_apply(clr_imgmap, cc, HLP_DefaultFgImagemap)) {
        cc = cx->getDefaultColor(HLP_DefaultFgImagemap);
        for (HLPtopic *t = HLP()->context()->topList(); t;
                t = t->sibling()) {
            QThelpDlg *w = (QThelpDlg*)HelpWidget::get_widget(t);
            w->viewer()->set_imagemap_boundary_color(cc);
        }
    }

    // Uodate all the help windows present.  The topic actually
    // shown is the topic::lastborn() if it is not null.

    for (HLPtopic *t = HLP()->context()->topList(); t; t = t->sibling()) {
        QThelpDlg *w = (QThelpDlg*)HelpWidget::get_widget(t);
        if (body_chg) {
            // The changed body tag will cause a re-parse and redisplay.
            HLPtopic *ct = t->lastborn();
            if (!ct)
                ct = t;
            ct->load_text();
            w->viewer()->set_source(ct->get_cur_text());
        }
        else
            // This reformats and redisplays.
            w->viewer()->redisplay_view();
    }
}


void
QTmozyClrDlg::quit_btn_slot()
{
    delete this;
}


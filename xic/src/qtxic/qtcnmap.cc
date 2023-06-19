
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

#include "qtcnmap.h"
#include "main.h"
#include "cvrt.h"
#include "qtmain.h"

#include <QLayout>
#include <QLabel>
#include <QCheckBox>
#include <QLineEdit>


//-------------------------------------------------------------------------
// Subwidget group for cell name mapping

cCnmap::cCnmap(bool outp)
{
    cn_output = outp;

    setTitle(tr("Cell Name Mapping"));

    QGridLayout *grid = new QGridLayout(this);

    QLabel *label = new QLabel(tr("Prefix"));
    grid->addWidget(label, 0, 0);

    cn_prefix = new QLineEdit();
    grid->addWidget(cn_prefix, 0, 1);

    cn_to_lower = new QCheckBox(tr("To Lower"));
    grid->addWidget(cn_to_lower, 0, 2);
    connect(cn_to_lower, SIGNAL(stateChanged(int)),
        this, SLOT(to_lower_slot(int)));

    cn_rd_alias = new QCheckBox(tr("Read Alias"));
    grid->addWidget(cn_rd_alias, 0, 3);
    connect(cn_rd_alias, SIGNAL(stateChanged(int)),
        this, SLOT(rd_alias_slot(int)));

    label = new QLabel(tr("Suffix"));
    grid->addWidget(label, 1, 0);

    cn_suffix = new QLineEdit();
    grid->addWidget(cn_suffix, 1, 1);

    cn_to_upper = new QCheckBox(tr("To Upper"));
    grid->addWidget(cn_to_upper, 1, 2);
    connect(cn_to_upper, SIGNAL(stateChanged(int)),
        this, SLOT(to_upper_slot(int)));

    cn_wr_alias = new QCheckBox(tr("Write Alias"));
    grid->addWidget(cn_wr_alias, 1, 3);
    connect(cn_wr_alias, SIGNAL(stateChanged(int)),
        this, SLOT(wr_alias_slot(int)));

    update();

    connect(cn_prefix, SIGNAL(textChanged(const QString&)),
        this, SLOT(prefix_changed_slot(const QString&)));
    connect(cn_suffix, SIGNAL(textChanged(const QString&)),
        this, SLOT(suffix_changed_slot(const QString&)));
}


void
cCnmap::update()
{
    if (cn_output) {
        QString str(CDvdb()->getVariable(VA_OutCellNamePrefix));
        if (str != cn_prefix->text())
            cn_prefix->setText(str);
        str = QString(CDvdb()->getVariable(VA_OutCellNameSuffix));
        if (str != cn_suffix->text())
            cn_suffix->setText(str);

        QTdev::SetStatus(cn_to_lower,
            CDvdb()->getVariable(VA_OutToLower));
        QTdev::SetStatus(cn_to_upper,
            CDvdb()->getVariable(VA_OutToUpper));

        str = QString(CDvdb()->getVariable(VA_OutUseAlias));
        if (str.isNull()) {
            QTdev::SetStatus(cn_rd_alias, false);
            QTdev::SetStatus(cn_wr_alias, false);
        }
        else {
            QByteArray qba = str.toLatin1();
            if (qba[0] == 'r' || qba[0] == 'R') {
                QTdev::SetStatus(cn_rd_alias, true);
                QTdev::SetStatus(cn_wr_alias, false);
            }
            else if (qba[0] == 'w' || qba[0] == 'W' ||
                    qba[0] == 's' || qba[0] == 'S') {
                QTdev::SetStatus(cn_rd_alias, false);
                QTdev::SetStatus(cn_wr_alias, true);
            }
            else {
                QTdev::SetStatus(cn_rd_alias, true);
                QTdev::SetStatus(cn_wr_alias, true);
            }
        }
    }
    else {
        QString str(CDvdb()->getVariable(VA_InCellNamePrefix));
        if (str != cn_prefix->text())
            cn_prefix->setText(str);
        str = QString(CDvdb()->getVariable(VA_InCellNameSuffix));
        if (str != cn_suffix->text())
            cn_suffix->setText(str);

        QTdev::SetStatus(cn_to_lower,
            CDvdb()->getVariable(VA_InToLower));
        QTdev::SetStatus(cn_to_upper,
            CDvdb()->getVariable(VA_InToUpper));

        str = QString(CDvdb()->getVariable(VA_InUseAlias));
        if (str.isNull()) {
            QTdev::SetStatus(cn_rd_alias, false);
            QTdev::SetStatus(cn_wr_alias, false);
        }
        else {
            QByteArray qba = str.toLatin1();
            if (qba[0] == 'r' || qba[0] == 'R') {
                QTdev::SetStatus(cn_rd_alias, true);
                QTdev::SetStatus(cn_wr_alias, false);
            }
            else if (qba[0] == 'w' || qba[0] == 'W' ||
                    qba[0] == 's' || qba[0] == 'S') {
                QTdev::SetStatus(cn_rd_alias, false);
                QTdev::SetStatus(cn_wr_alias, true);
            }
            else {
                QTdev::SetStatus(cn_rd_alias, true);
                QTdev::SetStatus(cn_wr_alias, true);
            }
        }
    }
}


void
cCnmap::prefix_changed_slot(const QString &str)
{
    if (str.isNull() || str.isEmpty()) {
        if (cn_output)
            CDvdb()->clearVariable(VA_OutCellNamePrefix);
        else
            CDvdb()->clearVariable(VA_InCellNamePrefix);
        return;
    }
    const char *ss = cn_output ?
        CDvdb()->getVariable(VA_OutCellNamePrefix) :
        CDvdb()->getVariable(VA_InCellNamePrefix);
    if (!ss || (str != QString(ss))) {
        const char *s = lstring::copy(str.toLatin1().constData());
        if (cn_output)
            CDvdb()->setVariable(VA_OutCellNamePrefix, s);
        else
            CDvdb()->setVariable(VA_InCellNamePrefix, s);
        delete [] s;
    }
}


void
cCnmap::suffix_changed_slot(const QString &str)
{
    if (str.isNull() || str.isEmpty()) {
        if (cn_output)
            CDvdb()->clearVariable(VA_OutCellNameSuffix);
        else
            CDvdb()->clearVariable(VA_InCellNameSuffix);
        return;
    }
    const char *ss = cn_output ?
        CDvdb()->getVariable(VA_OutCellNameSuffix) :
        CDvdb()->getVariable(VA_InCellNameSuffix);
    if (!ss || (str != QString(ss))) {
        char *s = lstring::copy(str.toLatin1().constData());
        if (cn_output)
            CDvdb()->setVariable(VA_OutCellNameSuffix, s);
        else
            CDvdb()->setVariable(VA_InCellNameSuffix, s);
        delete [] s;
    }
}


void
cCnmap::to_lower_slot(int state)
{
    if (cn_output) {
        if (state)
            CDvdb()->setVariable(VA_OutToLower, 0);
        else
            CDvdb()->clearVariable(VA_OutToLower);
    }
    else {
        if (state)
            CDvdb()->setVariable(VA_InToLower, 0);
        else
            CDvdb()->clearVariable(VA_InToLower);
    }
}


void
cCnmap::to_upper_slot(int state)
{
    if (cn_output) {
        if (state)
            CDvdb()->setVariable(VA_OutToUpper, 0);
        else
            CDvdb()->clearVariable(VA_OutToUpper);
    }
    else {
        if (state)
            CDvdb()->setVariable(VA_InToUpper, 0);
        else
            CDvdb()->clearVariable(VA_InToUpper);
    }
}


void
cCnmap::rd_alias_slot(int state)
{
    if (cn_output) {
        bool rd = state;
        bool wr = QTdev::GetStatus(cn_wr_alias);
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
    else {
        bool rd = state;
        bool wr = QTdev::GetStatus(cn_wr_alias);
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
    }
}


void
cCnmap::wr_alias_slot(int state)
{
    if (cn_output) {
        bool rd = QTdev::GetStatus(cn_rd_alias);
        bool wr = state;
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
    else {
        bool rd = QTdev::GetStatus(cn_rd_alias);
        bool wr = state;
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
    }
}


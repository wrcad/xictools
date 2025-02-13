
/*========================================================================
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
 * Qt MOZY help viewer.
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#include "qtform_file.h"
#include "qtinterf/qtfile.h"
#include "htm/htm_widget.h"
#include "htm/htm_form.h"

#include <QLineEdit>
#include <QPushButton>
#include <QLayout>
#include <QFont>
#include <QFontMetrics>

//
// The file entry widget for forms, consists of an entry field and
// browse button.  Pressing the browse button pops up the file
// selection panel.  Making a selection from this panel enters the
// path into the entry area.
//


using namespace qtinterf;

inline int
char_width(QWidget *w)
{
    QFont f = w->font();
    QFontMetrics fm(f);
#if QT_VERSION >= QT_VERSION_CHECK(5,11,0)
    return (fm.horizontalAdvance(QString("X")));
#else
    return (fm.width(QString("X")));
#endif
}

inline int
line_height(QWidget *w)
{
    QFont f = w->font();
    QFontMetrics fm(f);
    return (fm.height());
}

QTform_file::QTform_file(htmForm *entry, QWidget *prnt) : QWidget(prnt)
{
    ff_fsel = 0;
    ff_edit = new QLineEdit(this);
    int wd = entry->size * char_width(ff_edit) + 4;
    int ht = line_height(ff_edit);
    ff_edit->resize(wd, ht);
    ff_browse = new QPushButton(this);
    ff_browse->setText(QString("Browse..."));
    ff_browse->setMaximumHeight(ht);
    QHBoxLayout *hbox = new QHBoxLayout(this);
    hbox->setContentsMargins(0, 0, 0, 0);
    hbox->setSpacing(4);
    hbox->addWidget(ff_edit);
    hbox->addWidget(ff_browse);
    QSize qs = size();
    entry->width = qs.width();
    entry->height = ht + 4;
    connect(ff_browse, &QAbstractButton::clicked,
        this, &QTform_file::browse_btn_slot);
}


void
QTform_file::browse_btn_slot()
{
    if (!ff_fsel) {
        ff_fsel = new QTfileDlg(0, fsSEL, 0, 0);
        ff_fsel->register_usrptr((void**)&ff_fsel);
        connect(ff_fsel, &QTfileDlg::file_selected,
            this, &QTform_file::file_selected_slot);
    }
    ff_fsel->set_visible(true);
}


void
QTform_file::file_selected_slot(const char *fname, void*)
{
    if (fname && *fname)
        ff_edit->setText(QString(fname));
}


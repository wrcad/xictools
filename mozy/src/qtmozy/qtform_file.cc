
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
    return (fm.width(QString("X")));
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
    fsel = 0;
    edit = new QLineEdit(this);
    int wd = entry->size * char_width(edit) + 4;
    int ht = line_height(edit);
    edit->resize(wd, ht);
    browse = new QPushButton(this);
    browse->setText(QString("Browse..."));
    browse->setMaximumHeight(ht);
    QHBoxLayout *hbox = new QHBoxLayout(this);
    hbox->setMargin(0);
    hbox->setSpacing(4);
    hbox->addWidget(edit);
    hbox->addWidget(browse);
    QSize qs = size();
    entry->width = qs.width();
    entry->height = ht + 4;
    connect(browse, SIGNAL(clicked()), this, SLOT(browse_btn_slot()));
}


void
QTform_file::browse_btn_slot()
{
    if (!fsel) {
        fsel = new QTfilePopup(0, fsSEL, 0, 0);
        fsel->register_usrptr((void**)&fsel);
        connect(fsel, SIGNAL(file_selected(const char*, void*)),
            this, SLOT(file_selected_slot(const char*, void*)));
    }
    fsel->set_visible(true);
}


void
QTform_file::file_selected_slot(const char *fname, void*)
{
    if (fname && *fname)
        edit->setText(QString(fname));
}


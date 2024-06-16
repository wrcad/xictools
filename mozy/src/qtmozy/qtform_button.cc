
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

#include "qtform_button.h"
#include "qtviewer.h"
#include "help/help_startup.h"
#include "htm/htm_format.h"


//
// A push button for forms.
//

QTform_button::QTform_button(htmForm *entry, QWidget *prnt) :
    QToolButton(prnt)
{
    form_entry = entry;

    QFontMetrics fm(font());
    if (entry->type == FORM_RESET || entry->type == FORM_SUBMIT) {
        const char *str = entry->value ? entry->value : entry->name;
        if (!str || !*str)
            str = "X";
        else
            setText(str);
#if QT_VERSION >= QT_VERSION_CHECK(5,11,0)
        entry->width = fm.horizontalAdvance(str) + 4;
#else
        entry->width = fm.width(str) + 4;
#endif
        entry->height = fm.height() + 4;
    }
    else {
#if QT_VERSION >= QT_VERSION_CHECK(5,11,0)
        entry->width = fm.horizontalAdvance("X") + 4;
#else
        entry->width = fm.width("X") + 4;
#endif
        entry->height = entry->width;
        setCheckable(true);
        setChecked(entry->checked);
    }
    setFixedSize(QSize(entry->width, entry->height));

    connect(this, &QTform_button::pressed,
        this, &QTform_button::pressed_slot);
    connect(this, &QTform_button::released,
        this, &QTform_button::released_slot);
}


void
QTform_button::pressed_slot()
{
    if (form_entry->type == FORM_RADIO) {
        // get start of this radiobox
        htmForm *tmp;
        for (tmp = form_entry->parent->components; tmp;
                tmp = tmp->next)
            if (tmp->type == FORM_RADIO &&
                    !(strcasecmp(tmp->name, form_entry->name)))
                break;

        if (tmp == 0)
            return;

        // unset all other toggle buttons in this radiobox
        for ( ; tmp != 0; tmp = tmp->next) {
            if (tmp->type == FORM_RADIO && tmp != form_entry) {
                if (!strcasecmp(tmp->name, form_entry->name)) {
                    // same group, unset it
                    QTform_button *btn = (QTform_button*)tmp->widget;
                    btn->setChecked(false);
                }
                else
                    // Not a member of this group, we processed all
                    // elements in this radio box, break out.
                    break;
            }
        }
        form_entry->checked = true;
    }
    emit pressed(form_entry);
}


void
QTform_button::released_slot()
{
    if (form_entry->type == FORM_RADIO)
        setChecked(true);
}



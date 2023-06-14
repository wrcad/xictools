
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
 * QtInterf Graphical Interface Library                                   *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#include <QAction>
#include <QEvent>
#include <QLabel>
#include <QLayout>
#include <QPushButton>

#include "qtmcol.h"

#define COLUMN_SPACING 20


// Popup to display a list.  title is the title label text, callback
// is called with the word pointed to when the user points in the
// window, and 0 when the popup is destroyed.
//
// The list is column-formatted, and a text widget is used for the
// display.  If buttons is given, it is a 0-terminated list of
// auxiliary toggle button button names.
//
GRmcolPopup *
QTbag::PopUpMultiCol(stringlist *symlist, const char *title,
    void(*callback)(const char*, void*), void *arg,
    const char **buttons, int pgsize, bool no_dd)
{
    QTmcolPopup *mcol = new QTmcolPopup(this, symlist, title,
        buttons, pgsize, arg);
    mcol->register_callback(callback);
    mcol->set_no_dragdrop(no_dd);

    mcol->set_visible(true);
    return (mcol);
}

namespace qtinterf
{
    // Go through some wretched shit in order to get reasonable
    // spacing between columns.
    //
    class mcol_list_widget : public QListWidget
    {
    public:
        mcol_list_widget(QWidget*);

        QListWidgetItem *item_of(const QModelIndex &index)
            {
                return (itemFromIndex(index));
            }
    };

    class mcol_delegate : public QItemDelegate
    {
    public:
        mcol_delegate(mcol_list_widget *w) : QItemDelegate(w)
            {
                widget = w;
            }

        QSize sizeHint(const QStyleOptionViewItem&,
            const QModelIndex&)  const;

    private:
        mcol_list_widget *widget;
    };
}


mcol_list_widget::mcol_list_widget(QWidget *prnt) : QListWidget(prnt)
{
    setItemDelegate(new mcol_delegate(this));
}


QSize
mcol_delegate::sizeHint(const QStyleOptionViewItem&,
    const QModelIndex &index)  const
{
    QListWidgetItem *item = widget->item_of(index);
    QFontMetrics fm(item->font());
#if QT_VERSION >= QT_VERSION_CHECK(5,11,0)
    return (QSize(fm.horizontalAdvance(item->text()) + COLUMN_SPACING,
        fm.height()));
#else
    return (QSize(fm.width(item->text()) + COLUMN_SPACING, fm.height()));
#endif
}


QTmcolPopup::QTmcolPopup(QTbag *owner, stringlist *symlist,
    const char *title, const char **buttons, int pgsize, void *arg) :
    QDialog(owner ? owner->Shell() : 0), QTbag()
{
wb_shell = this;
    p_parent = owner;
    p_cb_arg = arg;

    if (owner)
        owner->MonitorAdd(this);

    setWindowTitle(tr("Listing"));
    setAttribute(Qt::WA_DeleteOnClose);
    label = new QLabel(QString(title), this);
    lbox = new mcol_list_widget(this);
    lbox->setMinimumWidth(300);
    lbox->setWrapping(true);
    lbox->setFlow(QListView::TopToBottom);

    for (stringlist *l = symlist; l; l = l->next)
        lbox->addItem(QString(l->string));
    lbox->sortItems();
    connect(lbox,
        SIGNAL(currentItemChanged(QListWidgetItem*, QListWidgetItem*)),
        this, SLOT(action_slot()));
    b_cancel = new QPushButton(tr("Dismiss"), this);
    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setMargin(4);
    vbox->setSpacing(2);
    vbox->addWidget(label);
    vbox->addWidget(lbox);
    vbox->addWidget(b_cancel);
    connect(b_cancel, SIGNAL(clicked()), this, SLOT(quit_slot()));
}


QTmcolPopup::~QTmcolPopup()
{
    if (p_usrptr)
        *p_usrptr = 0;
    if (p_caller) {
        QObject *o = (QObject*)p_caller;
        if (o->isWidgetType()) {
            QPushButton *btn = dynamic_cast<QPushButton*>(o);
            if (btn)
                btn->setChecked(false);
        }
        else {
            QAction *a = dynamic_cast<QAction*>(o);
            if (a)
                a->setChecked(false);
        }
    }
    if (p_callback)
        (*p_callback)(0, p_cb_arg);
    if (p_parent) {
        QTbag *owner = dynamic_cast<QTbag*>(p_parent);
        if (owner)
            owner->MonitorRemove(this);
    }
}


// GRpopup override
//
void
QTmcolPopup::popdown()
{
    if (p_parent) {
        QTbag *owner = dynamic_cast<QTbag*>(p_parent);
        if (!owner || !owner->MonitorActive(this))
            return;
    }
    delete this;
}


// Update the title, header, and contents of the mcol widget.
//
void
QTmcolPopup::update(stringlist *symlist, const char *title)
{
    if (p_parent) {
        QTbag *owner = dynamic_cast<QTbag*>(p_parent);
        if (!owner || !owner->MonitorActive(this))
            return;
    }

    if (title)
        label->setText(QString(title));
    lbox->clear();
    for (stringlist *l = symlist; l; l = l->next)
        lbox->addItem(QString(l->string));
    lbox->sortItems();
    raise();
    activateWindow();
}


// GRmcolPopup override
//
// Return the selected text, null if no selection.
//
char *
QTmcolPopup::get_selection()
{
//XXX    return (text_get_selection(wb_textarea));
return (0);
}


// GRmcolPopup override
//
// Set sensitivity of optional buttons.
// Bit == 0:  button always insensitive
// Bit == 1:  button sensitive when selection.
//
void
QTmcolPopup::set_button_sens(int bmask)
{
/*XXX
    int bm = 1;
    mc_btnmask = ~mask;
    bool has_sel = text_has_selection(wb_textarea);
    for (int i = 0; i < MC_MAXBTNS && mc_buttons[i]; i++) {
        gtk_widget_set_sensitive(mc_buttons[i], (bm & mask) && has_sel);
        bm <<= 1;
    }
*/
}


QList<QListWidgetItem*>
QTmcolPopup::get_items()
{
    return (lbox->findItems(QString("*"), Qt::MatchWildcard));
}


void
QTmcolPopup::action_slot()
{
/*XXX
    if (lbox) {
        QByteArray ba = lbox->currentItem()->text().toLatin1();
        if (p_callback)
            (*p_callback)(ba, p_cb_arg);
        emit action_call(ba, p_cb_arg);
    }
*/
}


void
QTmcolPopup::quit_slot()
{
    delete this;
}


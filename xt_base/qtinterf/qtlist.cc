
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

#include "qtlist.h"

#define COLUMN_SPACING 20


// Dialog to display a list.  title is the title label text, header is
// the first line displayed or 0, callback is called with the word
// pointed to when the user clicks in the window, and 0 when the popup
// is destroyed.
//
// If usepix is set, a two-column list is used, with the first column
// an open or closed icon.  This is controlled by the first two chars
// of each string (which are stripped):  if one is non-space, the open
// icon is used.
//
// If use_apply is set, the list will have an Apply button, which will
// call the callback with the selection and dismiss.
//
GRlistPopup *
QTbag::PopUpList(stringlist *symlist, const char *title,
    const char *header, void(*callback)(const char*, void*), void *arg,
    bool usepix, bool use_apply)
{
    //XXX implement me
    (void)use_apply;

    QTlistDlg *list = new QTlistDlg(this, symlist, title, header,
        usepix);
    if (wb_shell)
       list->set_transient_for(wb_shell);
    list->register_callback(callback);
    list->set_callback_arg(arg);

    list->set_visible(true);
    return (list);
}

namespace qtinterf
{
    // Go through some wretched shit in order to get reasonable
    // spacing between columns.
    //
    class list_list_widget : public QListWidget
    {
    public:
        list_list_widget(QWidget*);

        QListWidgetItem *item_of(const QModelIndex &index)
        {
            return (itemFromIndex(index));
        }
    };

    class list_delegate : public QItemDelegate
    {
    public:
        list_delegate(list_list_widget *w) : QItemDelegate(w)
        {
            widget = w;
        }

        QSize sizeHint(const QStyleOptionViewItem&,
            const QModelIndex&)  const;

    private:
        list_list_widget *widget;
    };
}


list_list_widget::list_list_widget(QWidget *prnt) : QListWidget(prnt)
{
    setItemDelegate(new list_delegate(this));
}


QSize
list_delegate::sizeHint(const QStyleOptionViewItem&,
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


QTlistDlg::QTlistDlg(QTbag *owner, stringlist *symlist, const char *title,
    const char *header, bool usepix) : QTbag(this)
{
    p_parent = owner;
    li_use_pix = usepix;
    li_open_pm = 0;
    li_close_pm = 0;

    if (owner)
        owner->MonitorAdd(this);

    setWindowTitle(tr("Listing"));
    setAttribute(Qt::WA_DeleteOnClose);

    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(2, 2, 2, 2);
    vbox->setSpacing(2);

    const char *t;
    char buf[256];
    if (title && header) {
        snprintf(buf, sizeof(buf), "%s\n%s", title, header);
        t = buf;
    }
    else
        t = header ? header : title;
    li_label = new QLabel(t);
    vbox->addWidget(li_label);

    li_lbox = new list_list_widget(this);
    li_lbox->setWrapping(false);
    for (stringlist *l = symlist; l; l = l->next)
        li_lbox->addItem(QString(l->string));
    li_lbox->sortItems();
    vbox->addWidget(li_lbox);

    connect(li_lbox,
        SIGNAL(currentItemChanged(QListWidgetItem*, QListWidgetItem*)),
        this, SLOT(action_slot()));

    li_cancel = new QPushButton(tr("Dismiss"), this);
    vbox->addWidget(li_cancel);
    connect(li_cancel, SIGNAL(clicked()), this, SLOT(dismiss_btn_slot()));
}


QTlistDlg::~QTlistDlg()
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
    delete li_open_pm;
    delete li_close_pm;
}


// GRpopup override
//
void
QTlistDlg::popdown()
{
    if (p_parent) {
        QTbag *owner = dynamic_cast<QTbag*>(p_parent);
        if (!owner || !owner->MonitorActive(this))
            return;
    }
    delete this;
}


// Update the title, header, and contents of the list widget.
//
void
QTlistDlg::update(stringlist *symlist, const char *title, const char *header)
{
    if (p_parent) {
        QTbag *owner = dynamic_cast<QTbag*>(p_parent);
        if (!owner || !owner->MonitorActive(this))
            return;
    }

    const char *t;
    char buf[256];
    if (title && header) {
        snprintf(buf, sizeof(buf), "%s\n%s", title, header);
        t = buf;
    }
    else
        t = header ? header : title;
    li_label->setText(t);

    li_lbox->clear();
    for (stringlist *l = symlist; l; l = l->next)
        li_lbox->addItem(QString(l->string));
    li_lbox->sortItems();
    raise();
    activateWindow();
}


// GRlistPopup override.
//
// In use_pix mode, reset the status of the pixmaps.  A true return
// from the callback is "open".
//
void
QTlistDlg::update(bool(*cb)(const char*))
{
    if (!cb || !li_use_pix)
        return;
    if (!li_open_pm)
        li_open_pm = new QPixmap(wb_open_folder_xpm);
    if (!li_close_pm)
        li_close_pm = new QPixmap(wb_closed_folder_xpm);

    for (int r = 0; ; r++) {
        QListWidgetItem *itm = li_lbox->item(r);
        QByteArray row_ba = itm->text().toLatin1();
        if ((*cb)(row_ba.constData()))
            itm->setIcon(*li_open_pm);
        else
            itm->setIcon(*li_close_pm);
    }
}


// GRlistPopup override.
//
void
QTlistDlg::unselect_all()
{
    li_lbox->clearSelection();
}


QList<QListWidgetItem*>
QTlistDlg::get_items()
{
    return (li_lbox->findItems("*", Qt::MatchWildcard));
}


void
QTlistDlg::action_slot()
{
    if (li_lbox) {
        QByteArray ba = li_lbox->currentItem()->text().toLatin1();
        if (p_callback)
            (*p_callback)(ba, p_cb_arg);
        emit action_call(ba, p_cb_arg);
    }
}


void
QTlistDlg::dismiss_btn_slot()
{
    delete this;
}


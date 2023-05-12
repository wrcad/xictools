
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


// Popup to display a list.  title is the title label text, header
// is the first line displayed or 0.  callback is called with
// the word pointed to when the user points in the window, and 0
// when the popup is destroyed.
//
// If usepix is set, a two-column list is used, with the first column
// an open or closed icon.  This is controlled by the first two chars
// of each string (which are stripped):  if one in non-space, the open
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
    //XXX
    (void)use_apply;

    static int list_count;

    QTlistPopup *list = new QTlistPopup(this, symlist, title, header,
        usepix, arg);
    list->register_callback(callback);

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


QTlistPopup::QTlistPopup(QTbag *owner, stringlist *symlist, const char *title,
    const char *header, bool usepix, void *arg) :
    QDialog(owner ? owner->Shell() : 0), QTbag()
{
wb_shell = this;
    p_parent = owner;
    p_cb_arg = arg;

    if (owner)
        owner->MonitorAdd(this);

    setWindowTitle(tr("Listing"));
    const char *t;
    char buf[256];
    if (title && header) {
        snprintf(buf, sizeof(buf), "%s\n%s", title, header);
        t = buf;
    }
    else
        t = header ? header : title;
    label = new QLabel(QString(t), this);
    lbox = new list_list_widget(this);
    lbox->setWrapping(false);
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


QTlistPopup::~QTlistPopup()
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
QTlistPopup::popdown()
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
QTlistPopup::update(stringlist *symlist, const char *title, const char *header)
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
    label->setText(QString(t));
    lbox->clear();
    for (stringlist *l = symlist; l; l = l->next)
        lbox->addItem(QString(l->string));
    lbox->sortItems();
    raise();
    activateWindow();
}


// GRlistPopup override.
//
// In use_pix mode, reset the status of the pixmaps.  A true return
// from the callback is "open".
//
void
QTlistPopup::update(bool(*cb)(const char*))
{
/* XXX
    if (ls_use_pix && cb) {
        for (int r = 0; ; r++) {
            char *text = 0;
            if (!gtk_clist_get_text(GTK_CLIST(ls_clist), r, 1, &text))
                break;
            if (!text || !*text)
                break;
            if ((*cb)(text))
                gtk_clist_set_pixmap(GTK_CLIST(ls_clist), r, 0,
                    ls_open_pm, ls_open_mask);
            else
                gtk_clist_set_pixmap(GTK_CLIST(ls_clist), r, 0,
                    ls_close_pm, ls_close_mask);
        }
    }
*/
}


// GRlistPopup override.
//
void
QTlistPopup::unselect_all()
{
 //XXX   gtk_clist_unselect_all(GTK_CLIST(ls_clist));
}


QList<QListWidgetItem*>
QTlistPopup::get_items()
{
    return (lbox->findItems(QString("*"), Qt::MatchWildcard));
}


void
QTlistPopup::action_slot()
{
    if (lbox) {
        QByteArray ba = lbox->currentItem()->text().toLatin1();
        if (p_callback)
            (*p_callback)(ba, p_cb_arg);
        emit action_call(ba, p_cb_arg);
    }
}


void
QTlistPopup::quit_slot()
{
    delete this;
}


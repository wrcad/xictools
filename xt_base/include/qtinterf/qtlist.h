
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

#ifndef QTLIST_H
#define QTLIST_H

#include "qtinterf.h"
#include "miscutil/lstring.h"

#include <QVariant>
#include <QDialog>
#include <QListWidget>
#include <QListWidgetItem>
#include <QItemDelegate>

class QCloseEvent;
class QLabel;
class QPushButton;

namespace qtinterf {
    class QTbag;
    class list_list_widget;
    class QTlistDlg;
}

class qtinterf::QTlistDlg : public QDialog, public GRlistPopup,
    public QTbag
{
    Q_OBJECT

public:
    QTlistDlg(QTbag*, stringlist*, const char*, const char*, bool);
    ~QTlistDlg();

    // GRpopup overrides
    void set_visible(bool visib)
    {
        if (visib) {
            show();
            raise();
            activateWindow();
        }
        else
            hide();
    }
    void popdown();

    // GRlistPopup override
    void update(stringlist*, const char*, const char*);
    void update(bool(*)(const char*));
    void unselect_all();

    QList<QListWidgetItem*> get_items();

signals:
    void action_call(const char*, void*);

private slots:
    void action_slot();
    void dismiss_btn_slot();

private:
    QLabel      *li_label;
    list_list_widget *li_lbox;
    QPushButton *li_cancel;
    QPixmap     *li_open_pm;
    QPixmap     *li_close_pm;
    bool        li_use_pix;
};

#endif


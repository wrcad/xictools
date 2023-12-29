
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

#ifndef QTLIBS_H
#define QTLIBS_H

#include "main.h"
#include "qtmain.h"

#include <QDialog>


//-----------------------------------------------------------------------------
// QTlibsDlg:  Libraries Listing dialog.

class QPushButton;
class QTreeWidget;
class QTreeWidgetItem;
class QPixmap;

class QTlibsDlg : public QDialog, public QTbag
{
    Q_OBJECT

public:
    QTlibsDlg(GRobject);
    ~QTlibsDlg();

    QSize sizeHint() const;

    char *get_selection();
    void update();

    void set_transient_for(QWidget *prnt)
        {
            Qt::WindowFlags f = windowFlags();
            setParent(prnt);
#ifdef __APPLE__
            f |= Qt::Tool;
#endif
            setWindowFlags(f);
        }

    static void set_panic()             { instPtr = 0; }
    static QTlibsDlg *self()            { return (instPtr); }

private slots:
    void open_btn_slot();
    void cont_btn_slot();
    void help_btn_slot();
    void current_item_changed_slot(QTreeWidgetItem*, QTreeWidgetItem*);
    void item_clicked_slot(QTreeWidgetItem*, int);
    void noovr_btn_slot(bool);
    void dismiss_btn_slot();
    void font_changed_slot(int);

private:
    void pop_up_contents();
    static void lb_content_cb(const char*, void*);
    static stringlist *lb_pathlibs();
    static stringlist *lb_add_dir(char*, stringlist*);

    GRobject    lb_caller;
    QPushButton *lb_openbtn;
    QPushButton *lb_contbtn;
    QTreeWidget *lb_list;
    QPushButton *lb_noovr;
    GRmcolPopup *lb_content_pop;
    char        *lb_selection;
    char        *lb_contlib;

    QPixmap     *lb_open_pb;
    QPixmap     *lb_close_pb;

    static const char *nolibmsg;
    static QTlibsDlg *instPtr;
};

#endif


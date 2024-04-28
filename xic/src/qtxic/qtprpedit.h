
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

#ifndef QTPRPEDIT_H
#define QTPRPEDIT_H

#include "qtprpbase.h"

#include <QDialog>


//-----------------------------------------------------------------------------
// QTprpEditorDlg:  Dialog to modify proerties of objects.

class QToolButton;
class QMenu;
class QAction;
class QMimeData;

class QTprpEditorDlg : public QDialog, public QTprpBase
{
    Q_OBJECT

public:
    struct sAddEnt
    {
        sAddEnt(const char *n, int v) : name(n), value(v) { }

        const char *name;
        int value;
    };

    QTprpEditorDlg(CDo*, PRPmode);
    ~QTprpEditorDlg();

#ifdef Q_OS_MACOS
    bool event(QEvent*);
#endif

    void update(CDo*, PRPmode);
    void purge(CDo*, CDo*);
    PrptyText *select(int);
    PrptyText *cycle(CDp*, bool(*)(const CDp*), bool);
    void set_btn_callback(int(*)(PrptyText*));

    void set_transient_for(QWidget *prnt)
        {
            Qt::WindowFlags f = windowFlags();
            setParent(prnt);
#ifdef Q_OS_MACOS
            f |= Qt::Tool;
#endif
            setWindowFlags(f);
        }

    // Don't pop down from Esc press.
    void keyPressEvent(QKeyEvent *ev)
        {
            if (ev->key() != Qt::Key_Escape)
                QDialog::keyPressEvent(ev);
        }

    static QTprpEditorDlg *self()           { return (instPtr); }

private slots:
    void edit_btn_slot(bool);
    void del_btn_slot(bool);
    void add_menu_slot(QAction*);
    void global_btn_slot(bool);
    void info_btn_slot(bool);
    void help_btn_slot();
    void activ_btn_slot(bool);
    void dismiss_btn_slot();
    void mouse_press_slot(QMouseEvent*);
    void mouse_release_slot(QMouseEvent*);
    void mouse_motion_slot(QMouseEvent*);
    void mime_data_handled_slot(const QMimeData*, int*) const;
    void mime_data_delivered_slot(const QMimeData*, int*);
    void font_changed_slot(int);

private:
    void activate(bool);
    void call_prpty_add(int);
    void call_prpty_edit(PrptyText*);
    void call_prpty_del(PrptyText*);

    QToolButton *po_activ;
    QToolButton *po_edit;
    QToolButton *po_del;
    QToolButton *po_add;
    QMenu       *po_addmenu;
    QToolButton *po_global;
    QToolButton *po_info;
    QAction     *po_name_btn;

    int po_dspmode;

    static QTprpEditorDlg *instPtr;
    static sAddEnt po_elec_addmenu[];
    static sAddEnt po_phys_addmenu[];
};

#endif


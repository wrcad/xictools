
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

#ifndef QTEMPTY_H
#define QTEMPTY_H

#include "main.h"
#include "qtmain.h"
#include "qtinterf/qttextw.h"
#include <QDialog>


//-----------------------------------------------------------------------------
// QTemptyDlg:  Dialog to allow the user to delete empty cells from a
// hierarchy.

class QLabel;
class QMouseEvent;

class QTemptyDlg : public QDialog, public QTbag
{
    Q_OBJECT

public:

    // List element for empty cells.
    //
    struct e_item
    {
        e_item() { name = 0; del = false; }

        const char *name;                   // cell name
        bool del;                           // deletion flag
    };

    QTemptyDlg(stringlist*);
    ~QTemptyDlg();

#ifdef Q_OS_MACOS
    bool event(QEvent*);
#endif

    QSize sizeHint() const;
    void update(stringlist*);

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

    static QTemptyDlg *self()           { return (instPtr); }

private slots:
    void delete_btn_slot();
    void skip_btn_slot();
    void apply_btn_slot();
    void dismiss_btn_slot();
    void font_changed_slot(int);
    void mouse_press_slot(QMouseEvent*);

private:
    void refresh();

    e_item  *ec_list;                   // list of cells
    SymTab  *ec_tab;                    // table of checked cells
    QLabel  *ec_label;                  // label widget
    QTtextEdit *ec_text;                // text area
    int     ec_field;                   // max cell name length
    bool    ec_changed;

    static QTemptyDlg *instPtr;
};

#endif


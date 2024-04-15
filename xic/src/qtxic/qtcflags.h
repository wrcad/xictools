
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

#ifndef QTCFLAGS_H
#define QTCFLAGS_H

#include "main.h"
#include "qtmain.h"

#include <QDialog>


//-----------------------------------------------------------------------------
// QTcflagsDlg:  Dialog to set cell flags (immutable, library).

class QLabel;

struct QTcflagsDlg : public QDialog, public QTbag
{
    Q_OBJECT

public:
    struct cf_elt
    {
        cf_elt(CDcellName n, bool i, bool l)
            {
                next = 0;
                name = n;
                immutable = i;
                library = l;
            }

        static void destroy(const cf_elt *e)
            {
                while (e) {
                    const cf_elt *ex = e;
                    e = e->next;
                    delete ex;
                }
            }

        cf_elt *next;
        CDcellName name;
        bool immutable;
        bool library;
    };

    QTcflagsDlg(GRobject, const stringlist*, int);
    ~QTcflagsDlg();

#ifdef Q_OS_MACOS
    bool event(QEvent*);
#endif

    void update(const stringlist*, int);

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

    static QTcflagsDlg *self()          { return (instPtr); }

private slots:
    void imm_none_btn_slot();
    void imm_all_btn_slot();
    void lib_none_btn_slot();
    void lib_all_btn_slot();
    void mouse_press_slot(QMouseEvent*);
    void apply_btn_slot();
    void dismiss_btn_slot();
    void font_changed_slot(int);

private:
    void init();
    void set(int, int);
    void refresh(bool = false);

    GRobject cf_caller;
    QLabel *cf_label;
    cf_elt *cf_list;                    // list of cells
    int cf_field;                       // name field width
    int cf_dmode;                       // display mode of cells
    
    static QTcflagsDlg *instPtr;
};

#endif


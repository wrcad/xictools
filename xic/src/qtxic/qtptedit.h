
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

#ifndef QTPTEDIT_H
#define QTPTEDIT_H

#include "main.h"
#include "sced.h"
#include "qtmain.h"

#include <QDialog>


//-----------------------------------------------------------------------------
// QTphysTermDlg:  Dialog interface for terminal/property editing.  This
// handles physical mode (TEDIT command).

class QLabel;
class QGroupBox;
class QComboBox;
class QCheckBox;
class QSpinBox;

class QTphysTermDlg : public QDialog
{
    Q_OBJECT

public:
    QTphysTermDlg(GRobject, TermEditInfo*, void(*)(TermEditInfo*, CDsterm*),
        CDsterm*);
    ~QTphysTermDlg();

#ifdef Q_OS_MACOS
    bool event(QEvent*);
#endif

    void update(TermEditInfo*, CDsterm*);

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

    static QTphysTermDlg *self()            { return (instPtr); }

private slots:
    void help_btn_slot();
    void prev_btn_slot();
    void next_btn_slot();
    void toindex_btn_slot();
    void apply_btn_slot();
    void dismiss_btn_slot();
    void layer_menu_slot(int);

private:
    void set_layername(const char *n)
        {
            const char *nn = lstring::copy(n);
            delete [] te_lname;
            te_lname = nn;
        }

    GRobject    te_caller;
    QLabel      *te_name;
    QGroupBox   *te_physgrp;
    QComboBox   *te_layer;
    QCheckBox   *te_fixed;
    QLabel      *te_flags;
    QSpinBox    *te_sb_toindex;

    void (*te_action)(TermEditInfo*, CDsterm*);
    CDsterm *te_term;
    const char *te_lname;

    static QTphysTermDlg *instPtr;
};

#endif


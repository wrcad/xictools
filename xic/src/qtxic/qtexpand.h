
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

#ifndef EXPAND_D_H
#define EXPAND_D_H

#include "main.h"
#include "qtmain.h"
#include <QDialog>

class QLabel;
class QLineEdit;
class QPushButton;
class QToolButton;


class QTexpandDlg : public QDialog, public GRpopup
{
    Q_OBJECT

public:
    typedef bool(*ExpandCallback)(const char*, void*);

    QTexpandDlg(QTbag*, const char *, bool, void*);
    ~QTexpandDlg();

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

    void register_callback(ExpandCallback cb) { p_callback = cb; }

    void set_transient_for(QWidget *prnt)
        {
            Qt::WindowFlags f = windowFlags();
            setParent(prnt);
#ifdef __APPLE__
            f |= Qt::Tool;
#endif
            setWindowFlags(f);
        }

    void update(const char *);

signals:
    void apply(const char*, void*);

private slots:
    void help_slot();
    void plus_slot();
    void minus_slot();
    void all_slot();
    void b0_slot();
    void b1_slot();
    void b2_slot();
    void b3_slot();
    void b4_slot();
    void b5_slot();
    void peek_slot();
    void apply_slot();
    void dismiss_slot();

private:
    QLabel      *b_label;
    QLineEdit   *b_edit;
    QPushButton *b_help;
    QToolButton *b_plus;
    QToolButton *b_minus;
    QToolButton *b_all;
    QToolButton *b_0;
    QToolButton *b_1;
    QToolButton *b_2;
    QToolButton *b_3;
    QToolButton *b_4;
    QToolButton *b_5;
    QPushButton *b_peek;
    QPushButton *b_apply;
    QPushButton *b_dismiss;

    void *p_cb_arg;
    ExpandCallback p_callback;
};

#endif

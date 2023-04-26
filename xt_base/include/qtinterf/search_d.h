
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

#ifndef SEARCH_D_H
#define SEARCH_D_H

#include "ginterf/graphics.h"
#include <QVariant>
#include <QDialog>
#include <QTimer>

class QCheckBox;
class QGroupBox;
class QLabel;
class QLineEdit;
class QPushButton;

namespace qtinterf
{
    class QTbag;

    class search_d : public QDialog, public GRpopup
    {
        Q_OBJECT

    public:
        search_d(QTbag*, const char*);
        ~search_d();

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

        void set_ign_case(bool);
        void set_message(const char*);
        void set_transient_message(const char*);
        QString get_target();

        // This widget will be deleted when closed with the title bar "X"
        // button.  Qt::WA_DeleteOnClose does not work - our destructor is
        // not called.  The default behavior is to hide the widget instead
        // of deleting it, which would likely be a core leak here.
        void closeEvent(QCloseEvent*) { quit_slot(); }

    signals:
        void search_down();
        void search_up();
        void ignore_case(bool);

    private slots:
        void quit_slot();
        void down_slot();
        void up_slot();
        void ign_case_slot(bool);
        void timeout_slot();

    private:
        QLabel *label;
        QLineEdit *edit;
        QPushButton *b_up;
        QPushButton *b_dn;
        QCheckBox *b_nc;
        QPushButton *b_cancel;
        const char *label_string;
        QTimer timer;
    };
}

#endif


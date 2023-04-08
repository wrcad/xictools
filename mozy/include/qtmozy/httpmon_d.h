
/*========================================================================
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
 * Qt MOZY help viewer.
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#ifndef HTTPMON_D_H
#define HTTPMON_D_H
 
#include <QVariant>
#include <QDialog>
#include <setjmp.h>
#include "httpget/transact.h"

//
// Definition of the QT download widget.
//
 
class QApplication;
class QGroupBox;
class QLabel;
class QPushButton;

namespace qtinterf
{
    class httpmon_d : public QDialog, public http_monitor
    {
        Q_OBJECT

    public:
        httpmon_d(QWidget*);
        ~httpmon_d();

        bool widget_print(const char*);     // print to monitor
        void abort();                       // abort transmission
        void run(Transaction*);             // start transfer

        void set_transaction(Transaction *t) { transaction = t; }

    private slots:
        void run_slot();
        void abort_slot();
        void quit_slot();

    private:
        bool event(QEvent*);

        QGroupBox *gb;
        QLabel *label;
        QPushButton *b_cancel;

        Transaction *transaction;

        char *g_textbuf;
        bool g_jbuf_set;
    public:
        jmp_buf g_jbuf;
    };
}

#endif


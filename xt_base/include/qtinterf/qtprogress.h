
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

#ifndef QTPROGRESS_H
#define QTPROGRESS_H

#include "ginterf/graphics.h"

#include <QVariant>
#include <QDialog>

class QGroupBox;
class QLabel;
class QPushButton;
class QTextEdit;

namespace qtinterf {
    class QTactivity;
    class QTbag;
    class QTprogressDlg;
}

class qtinterf::QTprogressDlg : public QDialog, public GRpopup
{
    Q_OBJECT

public:
    enum prgMode { prgPrint, prgFileop };

    QTprogressDlg(QTbag*, prgMode);
    ~QTprogressDlg();

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

    void set_input(const char*);
    void set_output(const char*);
    void set_info(const char*);
    void set_etc(const char*);
    void start();
    void finished();

    void set_info_limit(int n)  { pg_info_limit = n; pg_info_count = 0; }

signals:
    void abort();

private slots:
    void quit_slot();
    void abort_slot();

private:
    QGroupBox   *pg_gb_in;
    QLabel      *pg_label_in;
    QGroupBox   *pg_gb_out;
    QLabel      *pg_label_out;
    QGroupBox   *pg_gb_info;
    QTextEdit   *pg_te_info;
    QGroupBox   *pg_gb_etc;
    QLabel      *pg_label_etc;
    QPushButton *pg_abort;
    QPushButton *pg_cancel;
    QTactivity  *pg_pbar;
    int         pg_info_limit;
    int         pg_info_count;
};

#endif


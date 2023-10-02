
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

#ifndef QTTEXT_H
#define QTTEXT_H

#include "ginterf/graphics.h"
#include "miscutil/lstring.h"

#include <QVariant>
#include <QDialog>

class QGroupBox;
class QPushButton;
class QTextEdit;
namespace qtinterf {
    class QTbag;
    class QTtextDlg;
    class QTmsgDlg;
    class QTledDlg;
}


class qtinterf::QTtextDlg : public QDialog, public GRtextPopup
{
    Q_OBJECT

public:
    // Fancy message box types.
    enum PuType {PuWarn, PuErr, PuErrAlso, PuInfo, PuInfo2, PuHTML};

    QTtextDlg(QTbag*, const char*, PuType=PuInfo, STYtype=STY_NORM);
    ~QTtextDlg();

    QSize sizeHint() const;

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
    void set_desens()               { tx_desens = true; }
    bool is_desens()                { return (tx_desens); }

    // When set, error pop-ups have a "Show Error Log" button that
    // pops up a file browser on this file.
    //
    static void set_error_log(const char *s)
    {
        char *t = lstring::copy(s);
        delete [] tx_errlog;
        tx_errlog = t;
    }

    void popdown();
    void setTitle(const char*);
    void setText(const char*);
    bool get_btn2_state();
    void set_btn2_state(bool);
    bool update(const char*);

private slots:
    void save_btn_slot(bool);
    void showlog_btn_slot();
    void help_btn_slot();
    void activate_btn_slot(bool);
    void dismiss_btn_slot();

private:
    static ESret tx_save_cb(const char*, void*);
    static int tx_timeout(void*);

    QTextEdit   *tx_tbox;
    QPushButton *tx_save;
    QPushButton *tx_activate;
    QTledDlg    *tx_save_pop;
    QTmsgDlg    *tx_msg_pop;
    PuType      tx_which;
    STYtype     tx_style;
    bool        tx_desens;      // If true, parent->wb_inout is disabled.

    static char *tx_errlog;
};

#endif


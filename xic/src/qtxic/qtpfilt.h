
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

#ifndef QTPFILT_H
#define QTPFILT_H

#include "main.h"
#include "qtmain.h"

#include <QDialog>


//-----------------------------------------------------------------------------
// The Custom Property Filter Setup pop-up, called from the Compare
// Layouts panel.
//

class QLineEdit;

class QTcmpPrpFltDlg : public QDialog
{
    Q_OBJECT

public:
    QTcmpPrpFltDlg(GRobject);
    ~QTcmpPrpFltDlg();

    void update();

    void set_transient_for(QWidget *prnt)
    {
        Qt::WindowFlags f = windowFlags();
        setParent(prnt);
        setWindowFlags(f);
    }

    static QTcmpPrpFltDlg *self()           { return (instPtr); }

private slots:
    void help_btn_slot();
    void phys_cellstr_slot(const QString&);
    void phys_inststr_slot(const QString&);
    void phys_objstr_slot(const QString&);
    void elec_cellstr_slot(const QString&);
    void elec_inststr_slot(const QString&);
    void elec_objstr_slot(const QString&);
    void dismiss_btn_slot();

private:
    GRobject pf_caller;
    QLineEdit *pf_phys_cell;
    QLineEdit *pf_phys_inst;
    QLineEdit *pf_phys_obj;
    QLineEdit *pf_elec_cell;
    QLineEdit *pf_elec_inst;
    QLineEdit *pf_elec_obj;

    static QTcmpPrpFltDlg *instPtr;
};

#endif


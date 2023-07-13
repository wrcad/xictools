
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

#ifndef QTSPIF_H
#define QTSPIF_H

#include "main.h"
#include "qtmain.h"

#include <QDialog>


//--------------------------------------------------------------------
// Pop-up to control the WRspice interface.
//

class QCheckBox;
class QLineEdit;
class QComboBox;
class QPushButton;

class QTspiceIfDlg : public QDialog
{
    Q_OBJECT

public:
    QTspiceIfDlg(GRobject);
    ~QTspiceIfDlg();

    void update();

    static QTspiceIfDlg *self()         { return (instPtr); }

private slots:
    void help_btn_slot();
    void listall_btn_slot(int);
    void checksol_btn_slot(int);
    void notools_btn_slot(int);
    void alias_btn_slot(int);
    void hostname_btn_slot(int);
    void dispname_btn_slot(int);
    void progname_btn_slot(int);
    void execdir_btn_slot(int);
    void execname_btn_slot(int);
    void catchar_btn_slot(int);
    void catmode_btn_slot(int);
    void dismiss_btn_slot();

private:
    GRobject  sc_caller;
    QCheckBox *sc_listall;
    QCheckBox *sc_checksol;
    QCheckBox *sc_notools;
    QLineEdit *sc_alias;
    QCheckBox *sc_alias_b;
    QLineEdit *sc_hostname;
    QCheckBox *sc_hostname_b;
    QLineEdit *sc_dispname;
    QCheckBox *sc_dispname_b;
    QLineEdit *sc_progname;
    QCheckBox *sc_progname_b;
    QLineEdit *sc_execdir;
    QCheckBox *sc_execdir_b;
    QLineEdit *sc_execname;
    QCheckBox *sc_execname_b;
    QLineEdit *sc_catchar;
    QCheckBox *sc_catchar_b;
    QComboBox *sc_catmode;
    QCheckBox *sc_catmode_b;

    static QTspiceIfDlg *instPtr;
};

#endif


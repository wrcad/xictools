
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
 * WRspice Circuit Simulation and Analysis Tool                           *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#ifndef QTTBHELP_H
#define QTTBHELP_H

#include "ginterf/graphics.h"
#include "toolbar.h"

#include <QDialog>

namespace qtinterf { class QTtextEdit; }
class QMouseEvent;
using namespace qtinterf;

// Dialog to display keyword help lists.  Clicking on the list entries
// calls the main help system.  This is called from the dialogs which
// contain lists of 'set' variables to modify.
//
class QTtbHelpDlg : public QDialog
{
    Q_OBJECT

public:
    QTtbHelpDlg(GRobject, GRobject, TBH_type);
    ~QTtbHelpDlg();

    TBH_type type()         { return (th_type); }
    GRobject parent()       { return (th_parent); }
    GRobject caller()       { return (th_caller); }

private slots:
    void dismiss_btn_slot();
    void mouse_press_slot(QMouseEvent*);
    void font_changed_slot(int);

private:
    QTtextEdit *th_text;

    GRobject th_parent;
    GRobject th_caller;
    TBH_type th_type;
    int th_lx;
    int th_ly;
};

#endif


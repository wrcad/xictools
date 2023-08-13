
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

#ifndef QTERRMSG_H
#define QTERRMSG_H

#include "toolbar.h"

#include <QDialog>

namespace qtinterf { class QTtextEdit; }
using namespace qtinterf;

// The invisible part, handles text trimming, etc.
//
class QTmsgDb : public cMsgHdlr
{
public:
    void PopUpErr(const char*);
    void ToLog(const char *s)   { first_message(s); }

    // Pop-up state kept here for persistence.
    void set_x(int x)           { er_x = x; }
    int  get_x()                { return (er_x); }
    void set_y(int y)           { er_y = y; }
    int  get_y()                { return (er_x); }
    void set_wrap(bool w)       { er_wrap = w; }
    bool get_wrap()             { return (er_wrap); }

private:
    void stuff_msg(const char*);

    int er_x;
    int er_y;
    bool er_wrap;
};


// The dialog, shows the lines in the database.
//
class QTerrmsgDlg : public QDialog
{
    Q_OBJECT

public:
    QTerrmsgDlg();
    ~QTerrmsgDlg();

    void stuff_msg(const char*);

    static QTerrmsgDlg *self()          { return (instPtr); }

private slots:
    void wrap_btn_slot(bool);
    void dismiss_btn_slot();

private:
    QTtextEdit *er_text;

    static QTerrmsgDlg *instPtr;
};

#endif


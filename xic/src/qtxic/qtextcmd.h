
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

#ifndef QTEXTCMD_H
#define QTEXTCMD_H

#include "main.h"
#include "qtmain.h"

#include <QDialog>


//---------------------------------------------------------------------------
// Pop-up interface for the following extraction commands:
//  PNET, ENET, SOURC, EXSET
//

struct sExtCmd;
class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QCheckBox;

class QTextCmdDlg : public QDialog
{
    Q_OBJECT

public:
    QTextCmdDlg(GRobject, sExtCmd*,
        bool(*)(const char*, void*, bool, const char*, int, int),
        void*, int);
    ~QTextCmdDlg();

    sExtCmd *excmd()                    { return (cmd_excmd); }

    static QTextCmdDlg *self()          { return (instPtr); }

    void set_transient_for(QWidget *prnt)
    {
        Qt::WindowFlags f = windowFlags();
        setParent(prnt);
#ifdef __APPLE__
        f |= Qt::Tool;
#endif
        setWindowFlags(f);
    }

    void update();

private slots:
    void help_btn_slot();
    void go_btn_slot(bool);
    void check_state_changed_slot(int);
    void depth_changed_slot(int);
    void cancel_btn_slot();

private:
    GRobject    cmd_caller;
    QComboBox   *cmd_depth;
    QLabel      *cmd_label;
    QLineEdit   *cmd_text;
    QPushButton *cmd_go;
    QPushButton *cmd_cancel;
    QCheckBox   **cmd_bx;

    sExtCmd *cmd_excmd;
    bool (*cmd_action)(const char*, void*, bool, const char*, int, int);
    void *cmd_arg;
    const char *cmd_helpkw;

    static QTextCmdDlg *instPtr;
};

#endif

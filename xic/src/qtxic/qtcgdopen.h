
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

#ifndef QTCGDOPEN_H
#define QTCGDOPEN_H

#include "main.h"
#include "qtmain.h"

#include <QDialog>


class QLineEdit;
class QPushButton;
class QTabWidget;
class QTlayerList;
class QTcnameMap;;

class QTcgdOpenDlg : public QDialog, public QTbag
{
    Q_OBJECT

public:
    QTcgdOpenDlg(GRobject, bool(*)(const char*, const char*, int, void*),
        void*, const char*, const char*);
    ~QTcgdOpenDlg();

    void update(const char*, const char*);

    void set_transient_for(QWidget *prnt)
        {
            Qt::WindowFlags f = windowFlags();
            setParent(prnt);
#ifdef __APPLE__
            f |= Qt::Tool;
#endif
            setWindowFlags(f);
        }

    static QTcgdOpenDlg *self()         { return (instPtr); }

private slots:
    void help_btn_slot();
    void apply_btn_slot();
    void dismiss_btn_slot();

private:
    char *encode_remote_spec(QLineEdit**);
    void load_remote_spec(const char*);

    GRobject    cgo_caller;
    QTabWidget  *cgo_nbook;
    QLineEdit   *cgo_p1_entry;
    QLineEdit   *cgo_p2_entry;
    QLineEdit   *cgo_p3_host;
    QLineEdit   *cgo_p3_port;
    QLineEdit   *cgo_p3_idname;
    QLineEdit   *cgo_idname;
    QPushButton *cgo_apply;

    QTlayerList *cgo_p1_llist;
    QTcnameMap  *cgo_p1_cnmap;

    bool(*cgo_callback)(const char*, const char*, int, void*);
    void *cgo_arg;

    static QTcgdOpenDlg *instPtr;
};

#endif


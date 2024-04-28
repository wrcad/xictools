
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

#ifndef QTOADEFS_H
#define QTOADEFS_H

#include "main.h"
#include "qtmain.h"

#include <QDialog>


//-----------------------------------------------------------------------------
// QToaDefsDlg:  Panel for setting misc. OpenAccess interface parameters.

class QLineEdit;
class QCheckBox;

class QToaDefsDlg : public QDialog
{
    Q_OBJECT

public:
    QToaDefsDlg(GRobject);
    ~QToaDefsDlg();

#ifdef Q_OS_MACOS
    bool event(QEvent*);
#endif

    void update();

    void set_transient_for(QWidget *prnt)
        {
            Qt::WindowFlags f = windowFlags();
            setParent(prnt);
#ifdef Q_OS_MACOS
            f |= Qt::Tool;
#endif
            setWindowFlags(f);
        }

    // Don't pop down from Esc press.
    void keyPressEvent(QKeyEvent *ev)
        {
            if (ev->key() != Qt::Key_Escape)
                QDialog::keyPressEvent(ev);
        }

    static QToaDefsDlg *self()          { return (instPtr); }

private slots:
    void help_btn_slot();
    void path_text_changed(const QString&);
    void lib_text_changed(const QString&);
    void tech_text_changed(const QString&);
    void lview_text_changed(const QString&);
    void schview_text_changed(const QString&);
    void symview_text_changed(const QString&);
    void prop_text_changed(const QString&);
    void cdf_btn_slot(int);
    void dismiss_btn_slot();

private:
    GRobject    od_caller;
    QLineEdit   *od_path;
    QLineEdit   *od_lib;
    QLineEdit   *od_techlib;
    QLineEdit   *od_layout;
    QLineEdit   *od_schem;
    QLineEdit   *od_symb;
    QLineEdit   *od_prop;
    QCheckBox   *od_cdf;

    static QToaDefsDlg *instPtr;
};

#endif


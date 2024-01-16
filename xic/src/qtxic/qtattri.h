
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

#ifndef QTATTRI_H
#define QTATTRI_H

#include "main.h"
#include "qtmain.h"

#include <QDialog>


//-----------------------------------------------------------------------------
// QTattributesDlg:  Dialog to control cursor modes and other window
// attributes.

class QWidget;
class QSpinBox;
class QCheckBox;
class QComboBox;
namespace qtinterf {
    class QTdoubleSpinBox;
}

class QTattributesDlg : public QDialog
{
    Q_OBJECT

public:
    QTattributesDlg(GRobject);
    ~QTattributesDlg();

    void update();

    void set_transient_for(QWidget *prnt)
        {
            Qt::WindowFlags f = windowFlags();
            setParent(prnt);
#ifdef __APPLE__
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

    static QTattributesDlg *self()          { return (instPtr); }

private slots:
    void help_btn_slot();
    void cursor_menu_changed_slot(int);
    void usefwc_btn_slot(int);
    void visthr_sb_slot(int);
    void cxpct_sb_slot(int);
    void offset_sb_slot(int);
    void markinst_btn_slot(int);
    void markcntr_btn_slot(int);
    void abprop_btn_slot(int);
    void tsize_sb_slot(int);
    void ebt_menu_changed_slot(int);
    void ttsize_sb_slot(int);
    void tmsize_sb_slot(int);
    void hdn_menu_changed_slot(int);
    void lheight_sb_slot(double);
    void llen_sb_slot(int);
    void llines_sb_slot(int);
    void dismiss_btn_slot();

private:
    GRobject    at_caller;

    QComboBox   *at_cursor;
    QCheckBox   *at_fullscr;
    QSpinBox    *at_sb_visthr;
    QSpinBox    *at_sb_cxpct;
    QSpinBox    *at_sb_offset;

    QCheckBox   *at_minst;
    QCheckBox   *at_mcntr;

    QCheckBox   *at_abprop;
    QSpinBox    *at_sb_tsize;

    QComboBox   *at_ebterms;
    QSpinBox    *at_sb_ttsize;
    QSpinBox    *at_sb_tmsize;

    QComboBox   *at_hdn;
    QTdoubleSpinBox *at_sb_lheight;
    QSpinBox    *at_sb_llen;
    QSpinBox    *at_sb_llines;

    int         at_ebthst;

    static QTattributesDlg *instPtr;
};

#endif


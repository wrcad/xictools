
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

#ifndef QTCVIN_H
#define QTCVIN_H

#include "main.h"
#include "qtmain.h"

#include <QDialog>


//-----------------------------------------------------------------------------
// QTconvertInDlg: Dialog to set input parameters and read cell files.

class QLabel;
class QTabWidget;
class QCheckBox;
class QComboBox;
class QTcnameMap;
class QTlayerList;
class QTwindowCfg;
namespace qtinterf {
    class QTdoubleSpinBox;
}

class QTconvertInDlg : public QDialog
{
    Q_OBJECT

public:
    struct fmt_menu
    {
        const char *name;
        int code;
    };

    QTconvertInDlg(GRobject, bool(*)(int, void*), void*);
    ~QTconvertInDlg();

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

    static QTconvertInDlg *self()           { return (instPtr); }

private slots:
    void help_btn_slot();
    void nbook_page_slot(int);
    void nonpc_btn_slot(int);
    void yesoapc_btn_slot(int);
    void nolyr_btn_slot(int);
    void over_menu_slot(int);
    void replace_btn_slot(int);
    void merge_btn_slot(int);
    void polys_btn_slot(int);
    void dup_menu_slot(int);
    void empties_btn_slot(int);
    void dtypes_btn_slot(int);
    void force_menu_slot(int);
    void noflvias_btn_slot(int);
    void noflpcs_btn_slot(int);
    void nofllbs_btn_slot(int);
    void nolabels_btn_slot(int);
    void merg_menu_slot(int);
    void scale_changed_slot(double);
    void read_btn_slot();
    void dismiss_btn_slot();

private:
    GRobject    cvi_caller;
    QLabel      *cvi_label;
    QTabWidget  *cvi_nbook;
    QCheckBox   *cvi_nonpc;
    QCheckBox   *cvi_yesoapc;
    QCheckBox   *cvi_nolyr;
    QCheckBox   *cvi_replace;
    QComboBox   *cvi_over;
    QCheckBox   *cvi_merge;
    QCheckBox   *cvi_polys;
    QComboBox   *cvi_dup;
    QCheckBox   *cvi_empties;
    QCheckBox   *cvi_dtypes;
    QComboBox   *cvi_force;
    QCheckBox   *cvi_noflvias;
    QCheckBox   *cvi_noflpcs;
    QCheckBox   *cvi_nofllbs;
    QCheckBox   *cvi_nolabels;
    QComboBox   *cvi_merg;
    QTdoubleSpinBox *cvi_sb_scale;
    QTcnameMap  *cvi_cnmap;
    QTlayerList *cvi_llist;
    QTwindowCfg *cvi_wnd;
    bool        (*cvi_callback)(int, void*);
    void        *cvi_arg;

    static const char *cvi_mergvals[];
    static const char *cvi_overvals[];
    static const char *cvi_dupvals[];
    static fmt_menu cvi_forcevals[];
    static int cvi_merg_val;
    static QTconvertInDlg *instPtr;
};

#endif


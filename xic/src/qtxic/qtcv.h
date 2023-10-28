
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

#ifndef QTCV_H
#define QTCV_H

#include "main.h"
#include "qtmain.h"

#include <QDialog>


class QTlayerList;
class QTconvOutFmt;
class QTcnameMap;
class QTwindowCfg;

class QLabel;
class QTabWidget;
class QCheckBox;
class QPushButton;
class QDoubleSpinBox;

class QTconvertFmtDlg : public QDialog
{
    Q_OBJECT

public:
    // Sensitivity logic, MUST be the same as WndSensMode in qtwndc.h.
    enum CvSensMode {
        CvSensAllModes,
        CvSensFlatten,
        CvSensNone
    };

    QTconvertFmtDlg(GRobject, int, bool(*)(int, void*), void*);
    ~QTconvertFmtDlg();

    void update(int);

    void set_transient_for(QWidget *prnt)
    {
        Qt::WindowFlags f = windowFlags();
        setParent(prnt);
#ifdef __APPLE__
        f |= Qt::Tool;
#endif
        setWindowFlags(f);
    }

    static QTconvertFmtDlg *self()          { return (instPtr); }

private slots:
    void help_btn_slot();
    void input_menu_slot(int);
    void nbook_page_slot(int);
    void strip_btn_slot(int);
    void libsub_btn_slot(int);
    void pcsub_btn_slot(int);
    void viasub_btn_slot(int);
    void noflvias_btn_slot(int);
    void noflpcs_btn_slot(int);
    void nofllbs_btn_slot(int);
    void nolabels_btn_slot(int);
    void keepbad_btn_slot(int);
    void convert_btn_slot();
    void scale_changed_slot(double);
    void dismiss_btn_slot();

private:
    static void cv_format_proc(int);
    static void cv_sens_test();
    static CvSensMode wnd_sens_test();

    GRobject    cv_caller;
    QLabel      *cv_label;
    QComboBox   *cv_input;
    QTconvOutFmt *cv_fmt;
    QTabWidget  *cv_nbook;

    QCheckBox   *cv_strip;
    QCheckBox   *cv_libsub;
    QCheckBox   *cv_pcsub;
    QCheckBox   *cv_viasub;
    QCheckBox   *cv_noflvias;
    QCheckBox   *cv_noflpcs;
    QCheckBox   *cv_nofllbs;
    QCheckBox   *cv_nolabels;
    QCheckBox   *cv_keepbad;

    QTlayerList *cv_llist;
    QTcnameMap  *cv_cnmap;
    QTwindowCfg *cv_wnd;
    QLabel      *cv_tx_label;
    QDoubleSpinBox *cv_sb_scale;

    bool (*cv_callback)(int, void*);
    void *cv_arg;

    static int cv_fmt_type;
    static int cv_inp_type;
    static QTconvertFmtDlg *instPtr;
};

#endif


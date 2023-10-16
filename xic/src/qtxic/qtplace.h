
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

#ifndef QTPLACE_H
#define QTPLACE_H

#include "main.h"
#include "edit.h"
#include "dsp_inlines.h"
#include "qtmain.h"

#include <QDialog>

class QLabel;
class QPushButton;
class QComboBox;
class QSpinBox;
class QDoubleSpinBox;
class QDragEnterEvent;
class QDropEvent;

class QTplaceDlg : public QDialog, public cEdit::sPCpopup
{
    Q_OBJECT

public:
    QTplaceDlg(bool);
    ~QTplaceDlg();

    void update();

    // virtual overrides
    void rebuild_menu();
    void desel_placebtn();
    bool smash_mode();

    static QTplaceDlg *self()           { return (instPtr); }

    static void update_params()
    {
        if (DSP()->CurMode() == Physical)
            pl_iap = ED()->arrayParams();
    }

private slots:
    void array_btn_slot(bool);
    void replace_btn_slot(bool);
    void refmenu_slot(int);
    void help_btn_slot();
    void dismiss_btn_slot();
    void master_menu_slot(int);
    void place_btn_slot(bool);
    void menu_placebtn_slot(bool);
    void mmlen_change_slot(int);
    void nx_change_slot(int);
    void ny_change_slot(int);
    void dx_change_slot(double);
    void dy_change_slot(double);

private:
    void dragEnterEvent(QDragEnterEvent*);
    void dropEvent(QDropEvent*);
    void set_sens(bool);
    static ESret pl_new_cb(const char*, void*);
    static int pl_timeout(void*);

    QPushButton *pl_arraybtn;
    QPushButton *pl_replbtn;
    QPushButton *pl_smashbtn;
    QComboBox *pl_refmenu;

    QLabel *pl_label_nx;
    QLabel *pl_label_ny;
    QLabel *pl_label_dx;
    QLabel *pl_label_dy;

    QComboBox *pl_masterbtn;
    QPushButton *pl_placebtn;
    QPushButton *pl_menu_placebtn;

    GRledPopup *pl_str_editor;
    char *pl_dropfile;

    QSpinBox *pl_nx;
    QSpinBox *pl_ny;
    QDoubleSpinBox *pl_dx;
    QDoubleSpinBox *pl_dy;
    QSpinBox *pl_mmlen;

    static QTplaceDlg *instPtr;

    static iap_t pl_iap;
};

#endif


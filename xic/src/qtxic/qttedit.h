
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

#ifndef QTTEDIT_H
#define QTTEDIT_H

#include "main.h"
#include "qtmain.h"

#include <QDialog>


//---------------------------------------------------------------------------
// Pop-up interface for terminal/property editing.  This handles
// electrical mode (SUBCT command).
//

class QLabel;
class QSpinBox;
class QLineEdit;
class QComboBox;
class QCheckBox;
class QGroupBox;
class QPushButton;
struct TermEditInfo;

class QTelecTermEditDlg : public QDialog
{
    Q_OBJECT

public:
    QTelecTermEditDlg(GRobject, TermEditInfo*, void(*)(TermEditInfo*, CDp*),
        CDp*);
    ~QTelecTermEditDlg();

    void update(TermEditInfo*, CDp*);

    static QTelecTermEditDlg *self()            { return (instPtr); }

private slots:
    void help_btn_slot();
    void has_phys_term_slot(int);
    void destroy_btn_slot();
    void crbits_btn_slot();
    void ordbits_btn_slot();
    void scvis_btn_slot();
    void scinvis_btn_slot();
    void syvis_btn_slot();
    void syinvis_btn_slot();
    void prev_btn_slot();
    void next_btn_slot();
    void toindex_btn_slot();
    void apply_btn_slot();
    void layer_menu_slot(int);
    void dismiss_btn_slot();

private:
    void set_layername(const char *n)
        {
            const char *nn = lstring::copy(n);
            delete [] te_lname;
            te_lname = nn;
        }

    GRobject    te_caller;
    QLabel      *te_lab_top;
    QLabel      *te_lab_index;
    QSpinBox    *te_sb_index;
    QLineEdit   *te_name;
    QLabel      *te_lab_netex;
    QLineEdit   *te_netex;
    QComboBox   *te_layer;
    QCheckBox   *te_fixed;
    QCheckBox   *te_phys;
    QGroupBox   *te_physgrp;
    QCheckBox   *te_byname;
    QCheckBox   *te_scinvis;
    QCheckBox   *te_syinvis;
    QGroupBox   *te_bitsgrp;
    QPushButton *te_crtbits;
    QPushButton *te_ordbits;
    QSpinBox    *te_sb_toindex;

    void        (*te_action)(TermEditInfo*, CDp*);
    CDp         *te_prp;
    const char  *te_lname;
    bool        te_bterm;

    static QTelecTermEditDlg *instPtr;
};

#endif


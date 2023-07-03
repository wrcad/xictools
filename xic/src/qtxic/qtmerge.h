
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

#ifndef QTMERGE_H
#define QTMERGE_H

#include "main.h"
#include "fio.h"
#include "fio_cvt_base.h"
#include "dsp_tkif.h"
#include "dsp_inlines.h"
#include "cvrt.h"
#include "qtmain.h"
#include "qtmenu.h"

class QLabel;
class QCheckBox;

class QTmergeDlg : public QDialog
{
    Q_OBJECT

public:
    QTmergeDlg(mitem_t*);
    ~QTmergeDlg();

    bool is_hidden()                    { return (mc_allflag); }
    static QTmergeDlg *self()           { return (instPtr); }

    void query(mitem_t*);
    bool set_apply_to_all();
    bool refresh(mitem_t*);

private slots:
    void apply_btn_slot();
    void apply_to_all_btn_slot();
    void phys_check_box_slot(int);
    void elec_check_box_slot(int);

private:
    QLabel *mc_label;
    QCheckBox *mc_ophys;
    QCheckBox *mc_oelec;
    SymTab *mc_names;
    bool mc_allflag;        // user pressed "apply to all"
    bool mc_do_phys;
    bool mc_do_elec;

    static QTmergeDlg *instPtr;
};

#endif


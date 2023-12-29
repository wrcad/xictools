
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

#ifndef QTEXTSEL_H
#define QTEXTSEL_H

#include "main.h"
#include "qtmain.h"

#include <QDialog>


//-----------------------------------------------------------------------------
// QTextNetSelDlg:  Dialog to control group/node/path selections.

class QPushButton;
class QComboBox;
class QSpinBox;
class QCheckBox;
class QLabel;

class QTextNetSelDlg : public QDialog, public QTbag
{
    Q_OBJECT

public:
    QTextNetSelDlg(GRobject);
    ~QTextNetSelDlg();

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

    static QTextNetSelDlg *self()           { return (instPtr); }

private slots:
    void gnsel_btn_slot(bool);
    void pathsel_btn_slot(bool);
    void qpath_btn_slot(bool);
    void help_btn_slot();
    void gplane_menu_slot(int);
    void depth_changed_slot(int);
    void zbtn_slot();
    void allbtn_slot();
    void qp_usec_btn_slot(bool);
    void blink_btn_slot(bool);
    void subpath_btn_slot(bool);
    void ldant_btn_slot();
    void zoid_btn_slot();
    void tofile_btn_slot();
    void pathvias_btn_slot(int);
    void vcheck_btn_slot(int);
    void def_terms_slot(bool);
    void meas_btn_slot();
    void dismiss_btn_slot();

private:
    void set_sens();
    static int es_redraw_idle(void*);

    GRobject    es_caller;
    QPushButton *es_gnsel;
    QPushButton *es_paths;
    QPushButton *es_qpath;
    QComboBox   *es_gpmnu;
    QSpinBox    *es_sb_depth;
    QCheckBox   *es_qpconn;
    QCheckBox   *es_blink;
    QCheckBox   *es_subpath;
    QPushButton *es_antenna;
    QPushButton *es_zoid;
    QPushButton *es_tofile;
    QCheckBox   *es_vias;
    QCheckBox   *es_vtree;
    QLabel      *es_rlab;
    QPushButton *es_terms;
    QPushButton *es_meas;

    static QTextNetSelDlg *instPtr;
};

#endif


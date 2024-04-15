
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

#ifndef QTGRID_H
#define QTGRID_H

#include "main.h"
#include "qtmain.h"

#include <QDialog>


//-----------------------------------------------------------------------------
// QTgridDlg:  The Grid Setup panel, used to control the grid in each
// drawing window.

class QAction;
class QGroupBox;
class QLabel;
class QToolButton;
class QPushButton;
class QSpinBox;
class QCheckBox;
class QComboBox;
class QRadioButton;
class QMenu;
class QMouseEvent;
class QResizeEvent;
class QKeyPressEvent;
namespace qtinterf {
    class QTcanvas;
    class QTdoubleSpinBox;
}

class QTgridDlg : public QDialog, public QTbag, public QTdraw, public GRpopup
{
    Q_OBJECT

public:
    QTgridDlg(QTbag*, WindowDesc*);
    ~QTgridDlg();

#ifdef Q_OS_MACOS
    bool event(QEvent*);
#endif

    // GRpopup override
    void set_visible(bool visib)
        {
            if (visib)
                show();
            else
                hide();
        }

    void set_transient_for(QWidget *prnt)
        {
            Qt::WindowFlags f = windowFlags();
            setParent(prnt);
#ifdef Q_OS_MACOS
            f |= Qt::Tool;
#endif
            setWindowFlags(f);
        }

    void popdown();
    void update(bool = false);
    void initialize();

private slots:
    void help_btn_slot();
    void resol_changed_slot(double);
    void snap_changed_slot(int);
    void gridpersnap_btn_slot(int);
    void edge_menu_slot(int);
    void off_grid_btn_slot(int);
    void use_nm_btn_slot(int);
    void wire_edge_btn_slot(int);
    void wire_path_btn_slot(int);

    void show_btn_slot(bool);
    void top_btn_slot(bool);
    void store_menu_slot(QAction*);
    void rcl_menu_slot(QAction*);
    void noaxes_btn_slot(bool);
    void plaxes_btn_slot(bool);
    void oraxes_btn_slot(bool);
    void cmult_changed_slot(int);
    void solid_btn_slot(bool);
    void dots_btn_slot(bool);
    void stip_btn_slot(bool);
    void cross_size_changed(int);
    void nocoarse_btn_slot(int);
    void thresh_changed_slot(int);
    void apply_slot();
    void dismiss_slot();
    void smp_btn_press_slot(QMouseEvent*);
    void smp_btn_release_slot(QMouseEvent*);
    void smp_motion_slot(QMouseEvent*);
    void vp_btn_press_slot(QMouseEvent*);
    void vp_btn_release_slot(QMouseEvent*);
    void vp_resize_slot(QResizeEvent*);

private:
    void redraw();
    void keyPressEvent(QKeyEvent*);

    QGroupBox   *gd_snapbox;
    QTdoubleSpinBox *gd_resol;
    QLabel      *gd_mfglabel;
    QSpinBox    *gd_snap;
    QCheckBox   *gd_snapbtn;
    QGroupBox   *gd_edgegrp;
    QComboBox   *gd_edge;
    QCheckBox   *gd_off_grid;
    QCheckBox   *gd_use_nm_edge;
    QCheckBox   *gd_wire_edge;
    QCheckBox   *gd_wire_path;

    QToolButton *gd_showbtn;
    QToolButton *gd_topbtn;
    QRadioButton *gd_noaxesbtn;
    QRadioButton *gd_plaxesbtn;
    QRadioButton *gd_oraxesbtn;
    QSpinBox    *gd_cmult;
    QCheckBox   *gd_nocoarse;
    QTcanvas    *gd_sample;
    QRadioButton *gd_solidbtn;
    QRadioButton *gd_dotsbtn;
    QRadioButton *gd_stipbtn;
    QGroupBox   *gd_crs_frame;
    QSpinBox    *gd_crs;
    QSpinBox    *gd_thresh;

    QPushButton *gd_cancel;

    GridDesc gd_grid;
    unsigned int gd_mask_bak;
    int gd_win_num;
    int gd_last_n;
    int gd_drag_x;
    int gd_drag_y;
    bool gd_dragging;

    static QTgridDlg *grid_pops[DSP_NUMWINS];
};

#endif


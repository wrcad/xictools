
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

#ifndef QTCMP_H
#define QTCMP_H

#include "main.h"
#include "qtmain.h"
#include "cd_compare.h"

#include <QDialog>


class QTabWidget;
class QLineEdit;
class QCheckBox;
class QSpinBox;
class QDoubleSpinBox;
class QPushButton;
class QRadioButton;
class QMenu;

class QTcompareDlg : public QDialog
{
    Q_OBJECT

public:
    // Entry storage, all entries are persistent.
    //
    struct sCmp_store
    {
        sCmp_store();
        ~sCmp_store();

        char *cs_file1;
        char *cs_file2;
        char *cs_cells1;
        char *cs_cells2;
        char *cs_layers;
        int cs_mode;
        bool cs_layer_only;
        bool cs_layer_skip;
        bool cs_differ;
        int cs_max_errs;

        bool cs_p1_recurse;
        bool cs_p1_expand_arrays;
        bool cs_p1_slop;
        bool cs_p1_dups;
        bool cs_p1_boxes;
        bool cs_p1_polys;
        bool cs_p1_wires;
        bool cs_p1_labels;
        bool cs_p1_insts;
        bool cs_p1_boxes_prp;
        bool cs_p1_polys_prp;
        bool cs_p1_wires_prp;
        bool cs_p1_labels_prp;
        bool cs_p1_insts_prp;
        bool cs_p1_elec;
        bool cs_p1_cell_prp;
        unsigned char cs_p1_fltr;

        bool cs_p2_recurse;
        bool cs_p2_expand_arrays;
        bool cs_p2_insts;

        bool cs_p3_use_window;
        double cs_p3_left;
        double cs_p3_bottom;
        double cs_p3_right;
        double cs_p3_top;
        int cs_p3_fine_grid;
        int cs_p3_coarse_mult;
    };

    QTcompareDlg(GRobject);
    ~QTcompareDlg();

    void update();

    static QTcompareDlg *self()         { return (instPtr); }

private slots:
    void help_btn_slot();
    void luse_btn_slot(int);
    void lskip_btn_slot(int);
    void go_btn_slot();
    void dismiss_btn_slot();

    void p1_sens_set_slot(int);
    void p1_mode_btn_slot(bool);
    void p1_fltr_menu_slot(int);
    void p1_setup_btn_slot(bool);

    void p3_usewin_btn_slot(int);
    void p3_s_menu_slot(QAction*);
    void p3_r_menu_slot(QAction*);

private:
    void save();
    void recall();
    void recall_p1();
    void recall_p2();
    void recall_p3();

    void per_cell_obj_page();
    void per_cell_geom_page();
    void flat_geom_page();
    void p1_sens();
    char *compose_arglist();
    bool get_bb(BBox*);
    void set_bb(const BBox*);

    GRobject    cmp_caller;
    QTabWidget  *cmp_mode;
    QLineEdit   *cmp_fname1;
    QLineEdit   *cmp_fname2;
    QLineEdit   *cmp_cnames1;
    QLineEdit   *cmp_cnames2;
    QCheckBox   *cmp_diff_only;
    QLineEdit   *cmp_layer_list;
    QCheckBox   *cmp_layer_use;
    QCheckBox   *cmp_layer_skip;
    QSpinBox    *cmp_sb_max_errs;

    QCheckBox   *cmp_p1_recurse;
    QCheckBox   *cmp_p1_expand_arrays;
    QCheckBox   *cmp_p1_slop;
    QCheckBox   *cmp_p1_dups;
    QCheckBox   *cmp_p1_boxes;
    QCheckBox   *cmp_p1_polys;
    QCheckBox   *cmp_p1_wires;
    QCheckBox   *cmp_p1_labels;
    QCheckBox   *cmp_p1_insts;
    QCheckBox   *cmp_p1_boxes_prp;
    QCheckBox   *cmp_p1_polys_prp;
    QCheckBox   *cmp_p1_wires_prp;
    QCheckBox   *cmp_p1_labels_prp;
    QCheckBox   *cmp_p1_insts_prp;
    QRadioButton *cmp_p1_phys;
    QRadioButton *cmp_p1_elec;
    QCheckBox   *cmp_p1_cell_prp;
    QComboBox   *cmp_p1_fltr;
    QPushButton *cmp_p1_setup;
    PrpFltMode  cmp_p1_fltr_mode;

    QCheckBox   *cmp_p2_expand_arrays;
    QCheckBox   *cmp_p2_recurse;
    QCheckBox   *cmp_p2_insts;

    QCheckBox   *cmp_p3_aoi_use;
    QPushButton *cmp_p3_s_btn;
    QPushButton *cmp_p3_r_btn;
    QMenu       *cmp_p3_s_menu;
    QMenu       *cmp_p3_r_menu;
    QDoubleSpinBox *cmp_sb_p3_aoi_left;
    QDoubleSpinBox *cmp_sb_p3_aoi_bottom;
    QDoubleSpinBox *cmp_sb_p3_aoi_right;
    QDoubleSpinBox *cmp_sb_p3_aoi_top;
    QSpinBox    *cmp_sb_p3_fine_grid;
    QSpinBox    *cmp_sb_p3_coarse_mult;

    static sCmp_store cmp_store;
    static QTcompareDlg *instPtr;
};

#endif


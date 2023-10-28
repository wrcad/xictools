
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

#ifndef QTFILLP_H
#define QTFILLP_H

#include "main.h"
#include "qtmain.h"

#include <QDialog>

class QHBoxLayout;
class QGroupBox;
class QSpinBox;
class QDragEnterEvent;
class QDropEvent;


class QTfillPatDlg : public QDialog, public QTbag, public QTdraw
{
    Q_OBJECT

public:
    // Pixel operations
    enum FPSETtype { FPSEToff, FPSETon, FPSETflip };

    QTfillPatDlg(GRobject);
    ~QTfillPatDlg();

    void update();
    void drag_load(LayerFillData*, CDl*);

    void set_transient_for(QWidget *prnt)
    {
        Qt::WindowFlags f = windowFlags();
        setParent(prnt);
        setWindowFlags(f);
    }

    static QTfillPatDlg *self()             { return (instPtr); }

private slots:
    void nx_change_slot(int);
    void ny_change_slot(int);
    void rot90_btn_slot();
    void x_btn_slot();
    void y_btn_slot();
    void stores_btn_slot();
    void defpats_change_slot(int);
    void dump_btn_slot();
    void pixed_btn_slot();
    void load_btn_slot();
    void apply_btn_slot();
    void help_btn_slot();
    void outline_btn_slot(bool);
    void fat_btn_slot(bool);
    void cut_btn_slot(bool);
    void dismiss_btn_slot();
    void button_down_slot(QMouseEvent*);
    void button_up_slot(QMouseEvent*);
    void key_down_slot(QKeyEvent*);
    void motion_slot(QMouseEvent*);
    void enter_slot(QEnterEvent*);
    void drag_enter_slot(QDragEnterEvent*);
    void drop_event_slot(QDropEvent*);

private:
    void fp_mode_proc(bool);
    void redraw_edit();
    void redraw_sample();
    void redraw_store(int);
    void show_pixel(int, int);
    void set_fp(unsigned char*, int, int);
    bool getij(int*, int*);
    void set_pixel(int, int, FPSETtype);
    FPSETtype get_pixel(int, int);
    void line(int, int, int, int, FPSETtype);
    void box(int, int, int, int, FPSETtype);
    void def_to_sample(LayerFillData*);
    void sample_to_def(LayerFillData*, int);
    void layer_to_def_or_sample(LayerFillData*, int);
    void pattern_to_layer(LayerFillData*, CDl*);
    void connect_sigs(QTcanvas*, bool);

    static void fp_drawghost(int, int, int, int, bool = false);
    static int fp_update_idle_proc(void*);

    GRobject fp_caller;
    QPushButton *fp_outl;
    QPushButton *fp_fat;
    QPushButton *fp_cut;
    QTcanvas *fp_editor;
    QTcanvas *fp_sample;
    QGroupBox *fp_editframe;
    QWidget *fp_editctrl;
    QGroupBox *fp_stoframe;
    QWidget *fp_stoctrl;
    QTcanvas *fp_stores[18];
    QSpinBox *fp_spnx;
    QSpinBox *fp_spny;
    QSpinBox *fp_defpats;

    GRfillType *fp_fp;
    int fp_pattern_bank;
    int fp_width, fp_height;
    long fp_foreg, fp_pixbg;
    unsigned char fp_array[128];  // 32x32 max
    int fp_nx, fp_ny;
    int fp_margin;
    int fp_def_box_w, fp_def_box_h;
    int fp_pat_box_h;
    int fp_edt_box_dim;
    int fp_spa;
    int fp_epsz;
    int fp_ii, fp_jj;
    int fp_drag_btn, fp_drag_x, fp_drag_y;
    int fp_pm_w, fp_pm_h;
    int fp_downbtn;
    bool fp_dragging;
    bool fp_editing;

    static QTfillPatDlg *instPtr;
};

#endif


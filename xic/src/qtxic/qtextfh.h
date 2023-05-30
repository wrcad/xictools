
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

#ifndef QTEXTFH_H
#define QTEXTFH_H

#include "main.h"
#include "qtmain.h"

#include <QDialog>

class QWidget;
class QDoubleSpinBox;

class cFHdlg : public QDialog, public QTbag
{
//    Q_OBJECT

public:
    cFHdlg(GRobject);
    ~cFHdlg();

    void update();
    void update_jobs_list();
    void update_label(const char*);

    static cFHdlg *self()           { return (instPtr); }

private slots:

private:
    void set_sens(bool);
    void update_fh_freq_widgets();
    void update_fh_freq();
    void select_range(int, int);
    int get_pid();
    void select_pid(int);

    static const char *fh_def_string(int);
    /*
    static void fh_cancel_proc(GtkWidget*, void*);
    static void fh_help_proc(GtkWidget*, void*);
    static void fh_change_proc(GtkWidget*, void*);
    static void fh_units_proc(GtkWidget*, void*);
    static void fh_p_cb(bool, void*);
    static void fh_dump_cb(const char*, void*);
    static void fh_btn_proc(GtkWidget*, void*);
    static int fh_button_dn(GtkWidget*, GdkEvent*, void*);
    static void fh_font_changed();
    */

    GRobject fh_caller;
    QWidget *fh_label;

    QWidget *fh_units;
    QWidget *fh_nhinc_ovr;
    QWidget *fh_nhinc_fh;
    QWidget *fh_enab;

    QWidget *fh_foreg;
    QWidget *fh_out;
    QWidget *fh_file;
    QWidget *fh_args;
    QWidget *fh_defs;
    QWidget *fh_fmin;
    QWidget *fh_fmax;
    QWidget *fh_ndec;
    QWidget *fh_path;

    QWidget *fh_jobs;
    QWidget *fh_kill;

    bool fh_no_reset;
    int fh_start;
    int fh_end;
    int fh_line_selected;

    QDoubleSpinBox *fh_fh_manh_grid_cnt;
    QDoubleSpinBox *fh_fh_nhinc;
    QDoubleSpinBox *fh_fh_rh;
    QDoubleSpinBox *fh_fh_volel_min;
    QDoubleSpinBox *fh_fh_volel_target;

    static cFHdlg *instPtr;
};

#endif


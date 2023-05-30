
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

#ifndef QTEXTFC_H
#define QTEXTFC_H

#include "main.h"
#include "qtmain.h"

#include <QDialog>

class QWidget;
class QDoubleSpinBox;

class cFCdlg : public QDialog, public QTbag
{
//    Q_OBJECT

public:
    cFCdlg(GRobject);
    ~cFCdlg();

    void update();
    void update_jobs_list();
    void update_label(const char*);
    void update_numbers();
    void clear_numbers();

    static cFCdlg *self()           { return (instPtr); }

private:
    void select_range(int, int);
    int get_pid();
    void select_pid(int);

    /*
    static const char *fc_def_string(int);
    static void fc_cancel_proc(GtkWidget*, void*);
    static void fc_help_proc(GtkWidget*, void*);
    static void fc_page_change_proc(GtkWidget*, void*, int, void*);
    static void fc_change_proc(GtkWidget*, void*);
    static void fc_units_proc(GtkWidget*, void*);
    static void fc_p_cb(bool, void*);
    static void fc_dump_cb(const char*, void*);
    static void fc_btn_proc(GtkWidget*, void*);
    static int fc_button_dn(GtkWidget*, GdkEvent*, void*);
    static void fc_font_changed();
    static void fc_dbg_btn_proc(GtkWidget*, void*);
    */

    GRobject fc_caller;
    QWidget *fc_label;
    QWidget *fc_foreg;
    QWidget *fc_out;
    QWidget *fc_shownum;
    QWidget *fc_file;
    QWidget *fc_args;
    QWidget *fc_path;

    QWidget *fc_units;
    QWidget *fc_enab;

    QWidget *fc_jobs;
    QWidget *fc_kill;

    QWidget *fc_dbg_zoids;
    QWidget *fc_dbg_vrbo;
    QWidget *fc_dbg_nm;
    QWidget *fc_dbg_czbot;
    QWidget *fc_dbg_dzbot;
    QWidget *fc_dbg_cztop;
    QWidget *fc_dbg_dztop;
    QWidget *fc_dbg_cyl;
    QWidget *fc_dbg_dyl;
    QWidget *fc_dbg_cyu;
    QWidget *fc_dbg_dyu;
    QWidget *fc_dbg_cleft;
    QWidget *fc_dbg_dleft;
    QWidget *fc_dbg_cright;
    QWidget *fc_dbg_dright;

    bool fc_no_reset;
    bool fc_frozen;
    int fc_start;
    int fc_end;
    int fc_line_selected;

    QDoubleSpinBox *fc_fc_plane_bloat;
    QDoubleSpinBox *fc_substrate_thickness;
    QDoubleSpinBox *fc_substrate_eps;
    QDoubleSpinBox *fc_fc_panel_target;

    static cFCdlg *instPtr;
};

#endif


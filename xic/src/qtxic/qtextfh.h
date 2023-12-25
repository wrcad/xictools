
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


//-----------------------------------------------------------------------------
// Pop-up to control FastHenry interface.
//

class QLabel;
class QCheckBox;
class QLineEdit;
class QComboBox;
class QSpinBox;
class QPushButton;
namespace qtinterf {
    class QTdoubleSpinBox;
}

class QTfastHenryDlg : public QDialog, public QTbag
{
    Q_OBJECT

public:
    enum { fhRun, fhRunFile, fhDump, fhForeg, fhMonitor, fhEnable,
        fhOverrd, fhFlmt, fhKill,  fhManhGridCnt, fhNhinc, fhRh,
        fhVolElMin, fhVolElTarg, fhPath, fhArgs, fhDefaults, fhFreq };

    QTfastHenryDlg(GRobject);
    ~QTfastHenryDlg();

    void update();
    void update_jobs_list();
    void update_label(const char*);

    void set_transient_for(QWidget *prnt)
        {
            Qt::WindowFlags f = windowFlags();
            setParent(prnt);
#ifdef __APPLE__
            f |= Qt::Tool;
#endif
            setWindowFlags(f);
        }

    static QTfastHenryDlg *self()           { return (instPtr); }

private slots:
    void help_btn_slot();
    void foreg_btn_slot(int);
    void console_btn_slot(int);
    void runfile_btn_slot();
    void runext_btn_slot();
    void dumplist_btn_slot();
    void args_changed_slot(const QString&);
    void fmin_changed_slot(const QString&);
    void fmax_changed_slot(const QString&);
    void ndec_changed_slot(const QString&);
    void path_changed_slot(const QString&);
    void units_changed_slot(int);
    void manh_grid_changed_slot(int);
    void defaults_changed_slot(const QString&);
    void nhinc_changed_slot(int);
    void rh_changed_slot(double);
    void override_btn_slot(int);
    void internal_btn_slot(int);
    void enable_btn_slot(int);
    void volel_min_changed_slot(double);
    void volel_target_changed_slot(int);
    void mouse_press_slot(QMouseEvent*);
    void abort_btn_slot();
    void dismiss_btn_slot();
    void font_changed_slot(int);

private:
    void set_sens(bool);
    void update_fh_freq_widgets();
    void update_fh_freq();
    void select_range(int, int);
    int get_pid();
    void select_pid(int);
    static const char *fh_def_string(int);
    static void fh_p_cb(bool, void*);
    static void fh_dump_cb(const char*, void*);

    GRobject    fh_caller;
    QLabel      *fh_label;

    QCheckBox   *fh_foreg;
    QCheckBox   *fh_out;
    QLineEdit   *fh_file;
    QLineEdit   *fh_args;
    QLineEdit   *fh_defs;
    QLineEdit   *fh_fmin;
    QLineEdit   *fh_fmax;
    QLineEdit   *fh_ndec;
    QLineEdit   *fh_path;

    QComboBox   *fh_units;
    QCheckBox   *fh_nhinc_ovr;
    QCheckBox   *fh_nhinc_fh;
    QCheckBox   *fh_enab;
    QSpinBox    *fh_sb_manh_grid_cnt;
    QSpinBox    *fh_sb_nhinc;
    QTdoubleSpinBox *fh_sb_rh;
    QTdoubleSpinBox *fh_sb_volel_min;
    QSpinBox    *fh_sb_volel_target;

    QTtextEdit  *fh_jobs;
    QPushButton *fh_kill;

    bool    fh_no_reset;
    int     fh_start;
    int     fh_end;
    int     fh_line_selected;

    static QTfastHenryDlg *instPtr;
};

#endif


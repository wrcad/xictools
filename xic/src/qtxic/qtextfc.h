
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


//-----------------------------------------------------------------------------
// QTfastCapDlg:  Dialog to control FasterCap/FastCap-WR interface.

class QLabel;
class QToolButton;
class QLineEdit;
class QComboBox;
class QCheckBox;
class QMouseEvent;
namespace qtinterf {
    class QTdoubleSpinBox;
    class QTexpDoubleSpinBox;
}

class QTfastCapDlg : public QDialog, public QTbag
{
    Q_OBJECT

public:
    enum { FcRun, FcRunFile, FcDump, FcPath, FcArgs, ShowNums, 
        Foreg, ToCons, FcPlaneBloat, SubstrateThickness, 
        SubstrateEps, Enable, FcPanelTarget, Kill };

    QTfastCapDlg(GRobject);
    ~QTfastCapDlg();

#ifdef Q_OS_MACOS
    bool event(QEvent*);
#endif

    void update();
    void update_jobs_list();
    void update_label(const char*);
    void update_numbers();
    void clear_numbers();

    void set_transient_for(QWidget *prnt)
        {
            Qt::WindowFlags f = windowFlags();
            setParent(prnt);
#ifdef Q_OS_MACOS
            f |= Qt::Tool;
#endif
            setWindowFlags(f);
        }

    // Don't pop down from Esc press.
    void keyPressEvent(QKeyEvent *ev)
        {
            if (ev->key() != Qt::Key_Escape)
                QDialog::keyPressEvent(ev);
        }

    static QTfastCapDlg *self()           { return (instPtr); }

private slots:
    void help_btn_slot();
    void page_changed_slot(int);
    void foreg_btn_slot(int);
    void console_btn_slot(int);
    void shownum_btn_slot(int);
    void runfile_btn_slot();
    void runext_btn_slot();
    void dumplist_btn_slot();
    void args_changed_slot(const QString&);
    void path_changed_slot(const QString&);
    void plane_bloat_changed_slot(double);
    void subthick_changed_slot(double);
    void units_changed_slot(int);
    void subeps_changed_slot(double);
    void enable_btn_slot(int);
    void panels_changed_slot(double);
    void abort_btn_slot();
    void dismiss_btn_slot();

    void zoid_dbg_btn_slot(int);
    void vrbo_dbg_btn_slot(int);
    void nm_dbg_btn_slot(int);
    void czbot_dbg_btn_slot(int);
    void dzbot_dbg_btn_slot(int);
    void cztop_dbg_btn_slot(int);
    void dztop_dbg_btn_slot(int);
    void cyl_dbg_btn_slot(int);
    void dyl_dbg_btn_slot(int);
    void cyu_dbg_btn_slot(int);
    void dyu_dbg_btn_slot(int);
    void cleft_dbg_btn_slot(int);
    void dleft_dbg_btn_slot(int);
    void cright_dbg_btn_slot(int);
    void dright_dbg_btn_slot(int);

    void mouse_press_slot(QMouseEvent*);
    void font_changed_slot(int);

private:
    void select_range(int, int);
    int get_pid();
    void select_pid(int);
    void debug_btn_hdlr(int, int);
    static const char *fc_def_string(int);
    static void fc_p_cb(bool, void*);
    static void fc_dump_cb(const char*, void*);

    GRobject    fc_caller;
    QLabel      *fc_label;
    QCheckBox   *fc_foreg;
    QCheckBox   *fc_out;
    QCheckBox   *fc_shownum;
    QLineEdit   *fc_file;
    QLineEdit   *fc_args;
    QLineEdit   *fc_path;

    QComboBox   *fc_units;
    QCheckBox   *fc_enab;
    QTdoubleSpinBox *fc_sb_plane_bloat;
    QTdoubleSpinBox *fc_sb_substrate_thickness;
    QTdoubleSpinBox *fc_sb_substrate_eps;
    QTexpDoubleSpinBox *fc_sb_panel_target;

    QCheckBox   *fc_dbg_zoids;
    QCheckBox   *fc_dbg_vrbo;
    QCheckBox   *fc_dbg_nm;
    QCheckBox   *fc_dbg_czbot;
    QCheckBox   *fc_dbg_dzbot;
    QCheckBox   *fc_dbg_cztop;
    QCheckBox   *fc_dbg_dztop;
    QCheckBox   *fc_dbg_cyl;
    QCheckBox   *fc_dbg_dyl;
    QCheckBox   *fc_dbg_cyu;
    QCheckBox   *fc_dbg_dyu;
    QCheckBox   *fc_dbg_cleft;
    QCheckBox   *fc_dbg_dleft;
    QCheckBox   *fc_dbg_cright;
    QCheckBox   *fc_dbg_dright;

    QTtextEdit  *fc_jobs;
    QToolButton *fc_kill;

    bool    fc_no_reset;
    bool    fc_frozen;
    int     fc_start;
    int     fc_end;
    int     fc_line_selected;

    static QTfastCapDlg *instPtr;
};

#endif


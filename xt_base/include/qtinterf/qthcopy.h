
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
 * QtInterf Graphical Interface Library                                   *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#ifndef QTHCOPY_H
#define QTHCOPY_H

#include <QVariant>
#include <QDialog>
#include <QProcess>
#include <QKeyEvent>
#include "qtinterf.h"

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QToolButton;
class QProcess;

namespace qtinterf {
    class QTprogressDlg;
    class QTprintDlg;

    // values for textfmt
    enum HCtextType
    {
        PlainText,
        PrettyText,
        HtmlText,
        PStimes,
        PShelv,
        PScentury,
        PSlucida
    };
}

class qtinterf::QTprintDlg : public QDialog
{
    Q_OBJECT

public:
    QTprintDlg(GRobject, HCcb*, HCmode, QTbag* = 0);
    ~QTprintDlg();

#ifdef Q_OS_MACOS
    bool event(QEvent*);
#endif
    void update(HCcb*);
    void set_message(const char*);
    void disable_progress();

    void set_format(int);
    void set_active(bool);
    bool is_active()                { return (pd_active); }
    int format_index()              { return (pd_fmt); }
    HCcb *callbacks()               { return (pd_cb); }

    void set_transient_for(QWidget *prnt)
        {
            Qt::WindowFlags f = windowFlags();
            setParent(prnt);
#ifdef __APPLE__
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

    // Window title bar X button.
    void closeEvent(QCloseEvent*)   { quit_slot(); }

signals:
    void dismiss();

private slots:
    void format_slot(int);
    void resol_slot(int);
    void font_slot(int);
    void pagesize_slot(int);
    void a4_slot(bool);
    void letter_slot(bool);
    void metric_slot(bool);
    void frame_slot(bool);
    void portrait_slot(bool);
    void landscape_slot(bool);
    void best_fit_slot(bool);
    void tofile_slot(bool);
    void legend_slot(bool);
    void auto_width_slot(bool);
    void auto_height_slot(bool);
    void help_slot();
    void print_slot();
    void quit_slot();

    void process_error_slot(QProcess::ProcessError);
    void process_finished_slot(int);

private:
    void set_sens(unsigned int);
    void fork_and_submit(const char*, const char*);
    static void checklims(HCdesc*);
    static void mkargv(int*, char**, char*);

    GRobject        pd_caller;      // launching button
    QTbag           *pd_owner;      // back pointer to owning set
    HCcb            *pd_cb;         // parameter data struct
    const char      *pd_cmdtext;    // print command or file name
    const char      *pd_tofilename; // file name
    int             pd_resol;       // resolution index
    int             pd_fmt;         // format index
    HCmode          pd_textmode;    // display mode (text/graphical)
    HCtextType      pd_textfmt;     // text font for text mode
    HClegType       pd_legend;      // use legend
    HCorientFlags   pd_orient;      // portrait or landscape
    bool            pd_tofile;      // print to file
    unsigned char   pd_tofbak;      // backup value
    bool            pd_active;      // pop-up is visible
    bool            pd_metric;      // dimensions are metric
    int             pd_drvrmask;    // available drivers
    int             pd_pgsindex;    // page size
    float           pd_wid_val;     // rendering are width on page
    float           pd_hei_val;     // rendering area height on page
    float           pd_lft_val;     // rendering area left on page
    float           pd_top_val;     // rendering top or bottom on page

    QLineEdit       *pd_cmdtxtbox;
    QLabel          *pd_cmdlab;
    QCheckBox       *pd_wlabel;
    QCheckBox       *pd_hlabel;
    QLabel          *pd_xlabel;
    QLabel          *pd_ylabel;
    QDoubleSpinBox  *pd_wid;
    QDoubleSpinBox  *pd_hei;
    QDoubleSpinBox  *pd_left;
    QDoubleSpinBox  *pd_top;
    QCheckBox       *pd_portbtn;
    QCheckBox       *pd_landsbtn;
    QCheckBox       *pd_fitbtn;
    QCheckBox       *pd_legbtn;
    QCheckBox       *pd_tofbtn;
    QCheckBox       *pd_metbtn;
    QCheckBox       *pd_a4btn;
    QCheckBox       *pd_ltrbtn;
    QComboBox       *pd_fontmenu;
    QComboBox       *pd_fmtmenu;
    QComboBox       *pd_resmenu;
    QComboBox       *pd_pgsmenu;
    QToolButton     *pd_frmbtn;
    QToolButton     *pd_hlpbtn;
    QProcess        *pd_process;
    QTprogressDlg   *pd_progress;

    bool            pd_printer_busy;
};

#endif



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

#ifndef PRINT_D_H
#define PRINT_D_H

#include <QVariant>
#include <QDialog>
#include <QProcess>
#include "qtinterf.h"

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QProcess;

namespace qtinterf
{
    class progress_d;

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

    class QTprintPopup : public QDialog
    {
        Q_OBJECT

    public:
        QTprintPopup(HCcb*, HCmode, QTbag* = 0);
        ~QTprintPopup();

        void update(HCcb*);
        void set_message(const char*);
        void disable_progress();

        bool is_active() { return (pd_active); }
        void set_active(bool);
        int format_index() { return (pd_fmt); }
        HCcb *callbacks() { return (pd_cb); }

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

        // Window title bar X button
        void closeEvent(QCloseEvent*) { quit_slot(); }

    private:
        QTbag           *pd_owner;          // back pointer to owning set
        HCcb            *pd_cb;             // parameter data struct
        const char      *pd_cmdtext;        // print command or file name
        const char      *pd_tofilename;     // file name
        int             pd_resol;           // resolution index
        int             pd_fmt;             // format index
        HCmode          pd_textmode;        // display mode (text/graphical)
        HCtextType      pd_textfmt;         // text font for text mode
        HClegType       pd_legend;          // use legend
        HCorientFlags   pd_orient;          // portrait or landscape
        bool            pd_tofile;          // print to file
        unsigned char   pd_tofbak;          // backup value
        bool            pd_active;          // pop-up is visible
        bool            pd_metric;          // dimensions are metric
        int             pd_drvrmask;        // available drivers
        int             pd_pgsindex;        // page size
        float           pd_wid_val;         // rendering are width on page
        float           pd_hei_val;         // rendering area height on page
        float           pd_lft_val;         // rendering area left on page
        float           pd_top_val;         // rendering top or bottom on page

        void set_sens(unsigned int);
        void fork_and_submit(const char*, const char*);

        QLineEdit *cmdtxtbox;
        QLabel *cmdlab;
        QCheckBox *wlabel;
        QCheckBox *hlabel;
        QLabel *xlabel;
        QLabel *ylabel;
        QDoubleSpinBox *wid;
        QDoubleSpinBox *hei;
        QDoubleSpinBox *left;
        QDoubleSpinBox *top;
        QCheckBox *portbtn;
        QCheckBox *landsbtn;
        QCheckBox *fitbtn;
        QCheckBox *legbtn;
        QCheckBox *tofbtn;
        QCheckBox *metbtn;
        QCheckBox *a4btn;
        QCheckBox *ltrbtn;
        QComboBox *fontmenu;
        QComboBox *fmtmenu;
        QComboBox *resmenu;
        QComboBox *pgsmenu;
        QPushButton *frmbtn;
        QPushButton *hlpbtn;
        QPushButton *printbtn;
        QPushButton *dismissbtn;
        QProcess *process;
        progress_d *progress;

        bool printer_busy;
    };
}

#endif


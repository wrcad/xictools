
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

#ifndef QTCVOFMT_H
#define QTCVOFMT_H

#include "main.h"
#include "qtmain.h"

#include <QTabWidget>

class QComboBox;
class QCheckBox;
class QPushButton;
class QLabel;
class QComboBox;
class QMenu;
namespace qtinterf {
    class QTdoubleSpinBox;
}


//-------------------------------------------------------------------------
// Subwidget for output format selection and control.
//

class QTconvOutFmt : public QTabWidget
{
    Q_OBJECT

public:
    enum cvofmt_mode
    {
        cvofmt_file,
        cvofmt_native,
        cvofmt_chd,
        cvofmt_chdfile,
        cvofmt_asm,
        cvofmt_db
    };

    struct fmt_menu
    {
        const char *name;
        int code;
    };

    QTconvOutFmt(void(*)(int), int, cvofmt_mode);
    ~QTconvOutFmt();

    void update();
    bool gds_text_input();
    void configure(cvofmt_mode);
    void set_page(int);

    // This is set from the OASIS-Advanced pop-up.
    static void set_rep_string(char *s)
    {
        delete [] fmt_oas_rep_string;
        fmt_oas_rep_string = s;
    }
    static char *rep_string()   { return (fmt_oas_rep_string); }

private slots:
    void gdsftopt_menu_changed(int);
    void gdslevel_menu_changed(int);
    void gdsmap_btn_slot(int);
    void gdscut_btn_slot(int);
    void gdsunit_changed_slot(double);
    void oasadv_btn_slot(bool);
    void oasmap_btn_slot(int);
    void oascmp_btn_slot(int);
    void oasrep_btn_slot(int);
    void oastab_btn_slot(int);
    void oassum_btn_slot(int);
    void cifcname_menu_changed(int);
    void ciflayer_menu_changed(int);
    void ciflabel_menu_changed(int);
    void cif_flags_slot(QAction*);
    void ciflast_btn_slot();
    void cgxcut_btn_slot(int);
    void oasoff_btn_slot(int);
    void oasnwp_btn_slot(int);
    void format_changed_slot(int);

private:
    QComboBox   *fmt_level;
    QCheckBox   *fmt_gdsmap;
    QCheckBox   *fmt_gdscut;
    QCheckBox   *fmt_cgxcut;
    QCheckBox   *fmt_oasmap;
    QCheckBox   *fmt_oascmp;
    QCheckBox   *fmt_oasrep;
    QCheckBox   *fmt_oastab;
    QCheckBox   *fmt_oassum;
    QCheckBox   *fmt_oasoff;
    QCheckBox   *fmt_oasnwp;
    QPushButton *fmt_oasadv;
    QLabel      *fmt_gdsftlab;
    QComboBox   *fmt_gdsftopt;
    QPushButton *fmt_cifext;
    QMenu       *fmt_cifflags;
    QComboBox   *fmt_cifcname;
    QComboBox   *fmt_ciflayer;
    QComboBox   *fmt_ciflabel;
    QTdoubleSpinBox *fmt_sb_gdsunit;

    void (*fmt_cb)(int);
    bool fmt_strip;

    static int          fmt_gds_inp;
    static char         *fmt_oas_rep_string;
    static const char   *fmt_gds_limits[];
    static const char   *fmt_which_flags[];
    static fmt_menu     fmt_gds_input[];
    static fmt_menu     fmt_cif_extensions[];
    static fmt_menu     fmt_cname_formats[];
    static fmt_menu     fmt_layer_formats[];
    static fmt_menu     fmt_label_formats[];
};

#endif


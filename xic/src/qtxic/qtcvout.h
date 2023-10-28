
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

#ifndef QTCVOUT_H
#define QTCVOUT_H

#include "main.h"
#include "qtmain.h"

#include <QDialog>


class QLabel;
class QTabWidget;
class QCheckBox;
class QDoubleSpinBox;
class QTconvOutFmt;
class QTwindowCfg;
class QTcnameMap;

typedef bool(*CvoCallback)(FileType, bool, void*);

class QTconvertOutDlg : public QDialog
{
    Q_OBJECT

public:
    struct fmtval_t
    {
        fmtval_t(const char *n, FileType t) { name = n; filetype = t; }

        const char *name;
        FileType filetype;
    };

    QTconvertOutDlg(GRobject, CvoCallback, void*);
    ~QTconvertOutDlg();

    void update();

    void set_transient_for(QWidget *prnt)
    {
        Qt::WindowFlags f = windowFlags();
        setParent(prnt);
#ifdef __APPLE__
        f |= Qt::Tool;
#endif
        setWindowFlags(f);
    }

    static QTconvertOutDlg *self()          { return (instPtr); }

private slots:
    void help_btn_slot();
    void nbook_page_slot(int);
    void invis_p_btn_slot(int);
    void invis_e_btn_slot(int);
    void strip_btn_slot(int);
    void libsub_btn_slot(int);
    void pcsub_btn_slot(int);
    void viasub_btn_slot(int);
    void allcells_btn_slot(int);
    void noflvias_btn_slot(int);
    void noflpcs_btn_slot(int);
    void nofllbs_btn_slot(int);
    void keepbad_btn_slot(int);
    void scale_changed_slot(double);
    void write_btn_slot();
    void dismiss_btn_slot();

private:
    static void cvo_format_proc(int);

    GRobject    cvo_caller;
    QLabel      *cvo_label;
    QTabWidget  *cvo_nbook;
    QCheckBox   *cvo_strip;
    QCheckBox   *cvo_libsub;
    QCheckBox   *cvo_pcsub;
    QCheckBox   *cvo_viasub;
    QCheckBox   *cvo_allcells;
    QCheckBox   *cvo_noflvias;
    QCheckBox   *cvo_noflpcs;
    QCheckBox   *cvo_nofllbs;
    QCheckBox   *cvo_keepbad;
    QCheckBox   *cvo_invis_p;
    QCheckBox   *cvo_invis_e;
    QDoubleSpinBox *cvo_sb_scale;
    QTconvOutFmt *cvo_fmt;
    QTwindowCfg *cvo_wnd;
    QTcnameMap  *cvo_cnmap;

    CvoCallback cvo_callback;
    void        *cvo_arg;
    bool        cvo_useallcells;

    static fmtval_t cvo_fmtvals[];
    static int cvo_fmt_type;
    static QTconvertOutDlg *instPtr;
};

#endif


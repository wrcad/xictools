
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

#ifndef QTCFILT_H
#define QTCFILT_H

#include "main.h"
#include "qtmain.h"

#include <QDialog>


//-----------------------------------------------------------------------------
// QTcfiltDlg:  Panel for setting up cell name filtering for the Cells
// Listing panel (QTcellsDlg).

class QCheckBox;
class QLineEdit;

class QTcfiltDlg : public QDialog
{
    Q_OBJECT

public:
    QTcfiltDlg(GRobject, DisplayMode, void(*)(cfilter_t*, void*), void*);
    ~QTcfiltDlg();

    void update(DisplayMode);

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

    static QTcfiltDlg *self()           { return (instPtr); }

private slots:
    void store_menu_slot(QAction*);
    void recall_menu_slot(QAction*);
    void help_btn_slot();
    void nimm_btn_slot(int);
    void imm_btn_slot(int);
    void nvsm_btn_slot(int);
    void vsm_btn_slot(int);
    void nlib_btn_slot(int);
    void lib_btn_slot(int);
    void npsm_btn_slot(int);
    void psm_btn_slot(int);
    void ndev_btn_slot(int);
    void dev_btn_slot(int);
    void nspr_btn_slot(int);
    void spr_btn_slot(int);
    void ntop_btn_slot(int);
    void top_btn_slot(int);
    void nmod_btn_slot(int);
    void mod_btn_slot(int);
    void nalt_btn_slot(int);
    void alt_btn_slot(int);
    void nref_btn_slot(int);
    void ref_btn_slot(int);
    void npcl_btn_slot(int);
    void pcl_btn_slot(int);
    void nscl_btn_slot(int);
    void scl_btn_slot(int);
    void nlyr_btn_slot(int);
    void lyr_btn_slot(int);
    void nflg_btn_slot(int);
    void flg_btn_slot(int);
    void nftp_btn_slot(int);
    void ftp_btn_slot(int);
    void apply_btn_slot();
    void dismiss_btn_slot();

private:
    void setup(const cfilter_t*);
    cfilter_t *new_filter();

    GRobject cf_caller;
    QCheckBox *cf_nimm;
    QCheckBox *cf_imm;
    QCheckBox *cf_nvsm;
    QCheckBox *cf_vsm;
    QCheckBox *cf_nlib;
    QCheckBox *cf_lib;
    QCheckBox *cf_npsm;
    QCheckBox *cf_psm;
    QCheckBox *cf_ndev;
    QCheckBox *cf_dev;
    QCheckBox *cf_nspr;
    QCheckBox *cf_spr;
    QCheckBox *cf_ntop;
    QCheckBox *cf_top;
    QCheckBox *cf_nmod;
    QCheckBox *cf_mod;
    QCheckBox *cf_nalt;
    QCheckBox *cf_alt;
    QCheckBox *cf_nref;
    QCheckBox *cf_ref;
    QCheckBox *cf_npcl;
    QCheckBox *cf_pcl;
    QLineEdit *cf_pclent;
    QCheckBox *cf_nscl;
    QCheckBox *cf_scl;
    QLineEdit *cf_sclent;
    QCheckBox *cf_nlyr;
    QCheckBox *cf_lyr;
    QLineEdit *cf_lyrent;
    QCheckBox *cf_nflg;
    QCheckBox *cf_flg;
    QLineEdit *cf_flgent;
    QCheckBox *cf_nftp;
    QCheckBox *cf_ftp;
    QLineEdit *cf_ftpent;
    QPushButton *cf_apply;

    void(*cf_cb)(cfilter_t*, void*);
    void *cf_arg;
    DisplayMode cf_mode;

#define NUMREGS 6
    static char *cf_phys_regs[];
    static char *cf_elec_regs[];

    static QTcfiltDlg *instPtr;
};

#endif


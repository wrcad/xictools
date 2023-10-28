
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

#ifndef QTEXTSET_H
#define QTEXTSET_H

#include "main.h"
#include "qtmain.h"

#include <QDialog>


//--------------------------------------------------------------------
// Pop-up to control misc. extraction variables.
//

class QTabWidget;
class QPushButton;
class QCheckBox;
class QLineEdit;
class QSpinBox;
class QDoubleSpinBox;
class QLabel;
struct sDevDesc;


class QTextSetupDlg : public QDialog
{
    Q_OBJECT

public:
    QTextSetupDlg(void*);
    ~QTextSetupDlg();

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

    static QTextSetupDlg *self()            { return (instPtr); }

private slots:
    void help_btn_slot();
    void clrex_btn_slot();
    void doex_btn_slot();
    void dismiss_btn_slot();

    void p1_extview_btn_slot(int);
    void p1_groups_btn_slot(int);
    void p1_nodes_btn_slot(int);
    void p1_terms_btn_slot(int);
    void p1_cterms_btn_slot(int);
    void p1_rsterms_btn_slot();
    void p1_rssubs_btn_slot();
    void p1_recurs_btn_slot(int);
    void p1_tedit_btn_slot(bool);
    void p1_tfind_btn_slot(bool);
    void p1_uagn_btn_slot();
    void p1_uadev_btn_slot();
    void p1_uasub_btn_slot();

    void p2_papply_btn_slot(bool);
    void p2_lapply_btn_slot(bool);
    void p2_ignnm_btn_slot(int);
    void p2_oldlab_btn_slot(int);
    void p2_updlab_btn_slot(int);
    void p2_merge_btn_slot(int);
    void p2_vcvx_btn_slot(int);
    void p2_vdepth_slot(int);
    void p2_vsubs_btn_slot(int);
    void p2_gpglob_btn_slot(int);
    void p2_gpmulti_btn_slot(int);
    void p2_gpmthd_menu_slot(int);

    void p3_noseries_btn_slot(int);
    void p3_nopara_btn_slot(int);
    void p3_keepshrt_btn_slot(int);
    void p3_nomrgshrt_btn_slot(int);
    void p3_nomeas_btn_slot(int);
    void p3_usecache_btn_slot(int);
    void p3_nordcache_btn_slot(int);
    void p3_deltaset_btn_slot(int);
    void p3_gridsize_slot(double);
    void p3_trytile_btn_slot(int);
    void p3_maxpts_slot(int);
    void p3_gridpts_slot(int);

    void p4_flapply_btn_slot(bool);
    void p4_exopq_btn_slot(int);
    void p4_vrbos_btn_slot(int);
    void p4_glapply_btn_slot(bool);
    void p4_noperm_btn_slot(int);
    void p4_apmrg_btn_slot(int);
    void p4_apfix_btn_slot(int);
    void p4_loop_count_slot(int);
    void p4_iter_count_slot(int);

private:
    void views_and_ops_page();
    void net_and_cell_page();
    void devs_page();
    void misc_page();
    void set_sens();
    void show_grp_node(QCheckBox*);

    /*
    void dev_menu_upd();
    static bool es_editsave(const char*, void*, XEtype);
    static void es_dev_menu_proc(GtkWidget*, void*);
    */

    GRobject    es_caller;
    QTabWidget  *es_notebook;
    QPushButton *es_clrex;
    QPushButton *es_doex;

    QCheckBox   *es_p1_extview;
    QCheckBox   *es_p1_groups;
    QCheckBox   *es_p1_nodes;
    QCheckBox   *es_p1_terms;
    QCheckBox   *es_p1_cterms;
    QCheckBox   *es_p1_recurs;
    QPushButton *es_p1_tedit;
    QPushButton *es_p1_tfind;

    QPushButton *es_p2_nlprpset;
    QLineEdit   *es_p2_nlprp;
    QPushButton *es_p2_nllset;
    QLineEdit   *es_p2_nll;
    QCheckBox   *es_p2_ignlab;
    QCheckBox   *es_p2_oldlab;
    QCheckBox   *es_p2_updlab;
    QCheckBox   *es_p2_merge;
    QCheckBox   *es_p2_vcvx;
    QLabel      *es_p2_lmax;
    QSpinBox    *es_p2_sb_vdepth;
    QCheckBox   *es_p2_vsubs;
    QCheckBox   *es_p2_gpglob;
    QCheckBox   *es_p2_gpmulti;
    QComboBox   *es_p2_gpmthd;

    QCheckBox   *es_p3_noseries;
    QCheckBox   *es_p3_nopara;
    QCheckBox   *es_p3_keepshrt;
    QCheckBox   *es_p3_nomrgshrt;
    QCheckBox   *es_p3_nomeas;
    QCheckBox   *es_p3_usecache;
    QCheckBox   *es_p3_nordcache;
    QCheckBox   *es_p3_deltaset;
    QDoubleSpinBox *es_p3_sb_delta;
    QCheckBox   *es_p3_trytile;
    QLabel      *es_p3_lmax;
    QLabel      *es_p3_lgrid;
    QSpinBox    *es_p3_sb_maxpts;
    QSpinBox    *es_p3_sb_gridpts;

    QPushButton *es_p4_flkeyset;
    QLineEdit   *es_p4_flkeys;
    QCheckBox   *es_p4_exopq;
    QCheckBox   *es_p4_vrbos;
    QPushButton *es_p4_glbexset;
    QLineEdit   *es_p4_glbex;
    QCheckBox   *es_p4_noperm;
    QCheckBox   *es_p4_apmrg;
    QCheckBox   *es_p4_apfix;
    QSpinBox    *es_p4_sb_loop;
    QSpinBox    *es_p4_sb_iters;

    int         es_gpmhst;
    sDevDesc    *es_devdesc;

    static QTextSetupDlg *instPtr;
};

#endif


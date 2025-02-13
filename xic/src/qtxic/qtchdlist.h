
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

#ifndef QTCHDLIST_H
#define QTCHDLIST_H

#include "main.h"
#include "qtmain.h"

#include <QDialog>


//-----------------------------------------------------------------------------
// QTchdListDlg:   Cell Hierarchy Digests Listing dialog.

class QToolButton;
class QTreeWidget;
class QTreeWidgetItem;
class QCheckBox;
class QComboBox;

class QTchdListDlg : public QDialog, public QTbag
{
    Q_OBJECT

public:
    QTchdListDlg(GRobject);
    ~QTchdListDlg();

#ifdef Q_OS_MACOS
    bool event(QEvent*);
#endif

    void update();

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

    static QTchdListDlg *self()         { return (instPtr); }

private slots:
    void add_btn_slot(bool);
    void sav_btn_slot(bool);
    void del_btn_slot(bool);
    void cfg_btn_slot(bool);
    void dsp_btn_slot(bool);
    void cnt_btn_slot();
    void cel_btn_slot(bool);
    void inf_btn_slot();
    void qinf_btn_slot();
    void help_btn_slot();
    void current_item_changed_slot(QTreeWidgetItem*, QTreeWidgetItem*);
    void rename_btn_slot(int);
    void loadtop_btn_slot(int);
    void failres_btn_slot(int);
    void usetab_btn_slot(int);
    void showtab_btn_slot(bool);
    void geom_change_slot(int);
    void dismiss_btn_slot();
    void font_changed_slot(int);

private:
    void recolor();
    void err_message(const char*);
    static bool chl_add_cb(const char*, const char*, int, void*);
    static bool chl_sav_cb(const char*, bool, void*);
    static void chl_del_cb(bool, void*);
    static bool chl_display_cb(bool, const BBox*, void*);
    static void chl_cnt_cb(const char*, void*);
    static ESret chl_cel_cb(const char*, void*);

    GRobject    chl_caller;
    QToolButton *chl_addbtn;
    QToolButton *chl_savbtn;
    QToolButton *chl_delbtn;
    QToolButton *chl_cfgbtn;
    QToolButton *chl_dspbtn;
    QToolButton *chl_cntbtn;
    QToolButton *chl_celbtn;
    QToolButton *chl_infbtn;
    QToolButton *chl_qinfbtn;
    QTreeWidget *chl_list;
    QCheckBox   *chl_loadtop;
    QCheckBox   *chl_rename;
    QCheckBox   *chl_usetab;
    QToolButton *chl_showtab;
    QCheckBox   *chl_failres;
    QComboBox   *chl_geomenu;
    GRledPopup  *chl_cel_pop;
    GRmcolPopup *chl_cnt_pop;
    GRaffirmPopup *chl_del_pop;
    char        *chl_selection;
    char        *chl_contlib;

    static QTchdListDlg *instPtr;
};

#endif


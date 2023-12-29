
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

#ifndef QTOALIBS_h
#define QTOALIBS_h

#include "main.h"
#include "qtmain.h"

#include <QDialog>


//-----------------------------------------------------------------------------
// QToaLibsDlg:  OpenAccess Libraries List dialog.

class QPushButton;
class QRadioButton;
class QTreeWidget;
class QTreeWidgetItem;

class QToaLibsDlg : public QDialog, public QTbag
{
    Q_OBJECT

public:
    enum { LBhelp, LBopen, LBwrt, LBcont, LBcrt, LBdefs,
        LBtech, LBdest, LBboth, LBphys, LBelec };

    QToaLibsDlg(GRobject);
    ~QToaLibsDlg();

    QSize sizeHint() const;

    void get_selection(const char**, const char**);
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

    static QToaLibsDlg *self()          { return (instPtr); }

private slots:
    void open_btn_slot();
    void write_btn_slot();
    void cont_btn_slot();
    void create_btn_slot();
    void defs_btn_slot(bool);
    void help_btn_slot();
    void tech_btn_slot(bool);
    void dest_btn_slot();
    void both_btn_slot(bool);
    void phys_btn_slot(bool);
    void elec_btn_slot(bool);
    void current_item_changed_slot(QTreeWidgetItem*, QTreeWidgetItem*);
    void item_activated_slot(QTreeWidgetItem*, int);
    void item_clicked_slot(QTreeWidgetItem*, int);
    void item_selection_changed_slot();
    void dismiss_btn_slot();
    void font_changed_slot(int);

private:
    void pop_up_contents();
    void update_contents(bool);
    void set_sensitive(bool);
    static void lb_lib_cb(const char*, void*);
    static void lb_dest_cb(bool, void*);
    static void lb_content_cb(const char*, void*);

    GRobject    lb_caller;
    QPushButton *lb_openbtn;
    QPushButton *lb_writbtn;
    QPushButton *lb_contbtn;
    QPushButton *lb_defsbtn;
    QPushButton *lb_techbtn;
    QPushButton *lb_destbtn;
    QRadioButton *lb_both;
    QRadioButton *lb_phys;
    QRadioButton *lb_elec;
    QTreeWidget *lb_list;

    GRmcolPopup *lb_content_pop;
    QPixmap     *lb_open_pm;
    QPixmap     *lb_close_pm;
    char        *lb_selection;
    char        *lb_contlib;
    char        *lb_tempstr;

    static const char *nolibmsg;
    static QToaLibsDlg *instPtr;
};

#endif



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

class cCHDlist : public QDialog, public GTKbag
{
    Q_OBJECT

public:
    cCHDlist(GRobject);
    ~cCHDlist();

    void update();

    cCHDlist *self()            { return (instPtr); }

private slots:

private:
    void recolor();
    void action_hdlr(GtkWidget*, void*);
    void err_message(const char*);

    /*
    static void chl_cancel(GtkWidget*, void*);
    static void chl_action_proc(GtkWidget*, void*);
    static void chl_geom_proc(GtkWidget*, void*);
    static int chl_selection_proc(GtkTreeSelection*, GtkTreeModel*,
        GtkTreePath*, int, void*);
    static bool chl_focus_proc(GtkWidget*, GdkEvent*, void*);
    static bool chl_add_cb(const char*, const char*, int, void*);
    static bool chl_sav_cb(const char*, bool, void*);
    static void chl_del_cb(bool, void*);
    static bool chl_display_cb(bool, const BBox*, void*);
    static void chl_cnt_cb(const char*, void*);
    static ESret chl_cel_cb(const char*, void*);
    */

    GRobject chl_caller;
    GtkWidget *chl_addbtn;
    GtkWidget *chl_savbtn;
    GtkWidget *chl_delbtn;
    GtkWidget *chl_cfgbtn;
    GtkWidget *chl_dspbtn;
    GtkWidget *chl_cntbtn;
    GtkWidget *chl_celbtn;
    GtkWidget *chl_infbtn;
    GtkWidget *chl_qinfbtn;
    GtkWidget *chl_list;
    GtkWidget *chl_loadtop;
    GtkWidget *chl_rename;
    GtkWidget *chl_usetab;
    GtkWidget *chl_showtab;
    GtkWidget *chl_failres;
    GtkWidget *chl_geomenu;
    GRledPopup *chl_cel_pop;
    GRmcolPopup *chl_cnt_pop;
    GRaffirmPopup *chl_del_pop;
    char *chl_selection;
    char *chl_contlib;
    bool chl_no_select;     // treeview focus hack

    static cCHDlist *instPtr;
};

#endif

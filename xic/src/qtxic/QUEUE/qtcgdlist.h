
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

#ifndef QTCGDLIST_H
#define QTCGDLIST_H

#include "main.h"
#include "qtmain.h"

#include <QDialog>


class cCGDlist : public QDialog, public GTKbag
{
    cCGDlist(GRobject);
    ~cCGDlist();

    void update();

    static cCGDlist *self()         { return (instPtr); }

private slots:

private:
    void action_hdlr(GtkWidget*, void*);
    void err_message(const char*);

    /*
    static int cgl_selection_proc(GtkTreeSelection*, GtkTreeModel*,
        GtkTreePath*, int, void*);
    static bool cgl_focus_proc(GtkWidget*, GdkEvent*, void*);
    static bool cgl_add_cb(const char*, const char*, int, void*);
    static ESret cgl_sav_cb(const char*, void*);
    static void cgl_del_cb(bool, void*);
    static void cgl_cnt_cb(const char*, void*);
    static void cgl_action_proc(GtkWidget*, void*);
    static void cgl_cancel(GtkWidget*, void*);
    */

    GRobject cgl_caller;
    GtkWidget *cgl_addbtn;
    GtkWidget *cgl_savbtn;
    GtkWidget *cgl_delbtn;
    GtkWidget *cgl_cntbtn;
    GtkWidget *cgl_infbtn;
    GtkWidget *cgl_list;
    GRledPopup *cgl_sav_pop;
    GRaffirmPopup *cgl_del_pop;
    GRmcolPopup *cgl_cnt_pop;
    GRmcolPopup *cgl_inf_pop;
    char *cgl_selection;
    char *cgl_contlib;
    bool cgl_no_select;     // treeview focus hack

    static cCGDlist *instPtr;
};

#endif

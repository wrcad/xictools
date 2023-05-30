
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

#ifndef QTATTRI_H
#define QTATTRI_H

#include "main.h"
#include "qtmain.h"

#include <QDialog>

class QWidget;
class QSpinBox;


cAttr:: *cAttr::instPtr;

class cAttr : public QDialog
{
    Q_OBJECT

public:
    cAttr(GRobject);
    ~cAttr();

    void update();

    static *cAttr self()        { return (instPtr); }

private slots:

private:
    /*
    static void at_cancel_proc(GtkWidget*, void*);
    static void at_action(GtkWidget*, void*);
    static void at_curs_menu_proc(GtkWidget*, void*);
    static void at_menuproc(GtkWidget*, void*);
    static void at_val_changed(GtkWidget*, void*);
    static void at_ebt_proc(GtkWidget*, void*);
    */

    GRobject at_caller;
    QWidget *at_cursor;
    QWidget *at_fullscr;
    QWidget *at_minst;
    QWidget *at_mcntr;
    QWidget *at_ebprop;
    QWidget *at_ebterms;
    QWidget *at_hdn;
    int at_ebthst;

    QSpinBox *at_tsize;
    QSpinBox *at_ttsize;
    QSpinBox *at_tmsize;
    QSpinBox *at_cellthr;
    QSpinBox *at_cxpct;
    QSpinBox *at_offset;
    QSpinBox *at_lheight;
    QSpinBox *at_llen;
    QSpinBox *at_llines;

    static cAttr *instPtr;
};

#endif


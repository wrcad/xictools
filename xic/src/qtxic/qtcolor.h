
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

#ifndef QTCOLOR_H
#define QTCOLOR_H

#include "main.h"
#include "qtmain.h"

//#include <QColorDialog>
#include <QDialog>

class QColorDialog;
class QComboBox;

//class cColor : public QColorDialog
class cColor : public QDialog
{
    Q_OBJECT

public:
    struct clritem
    {
        const char *descr;
        int tab_indx;
    };
    enum { CATEG_ATTR, CATEG_PROMPT, CATEG_PLOT };

    cColor(GRobject);
    ~cColor();

    void update();

    static cColor *self()           { return (instPtr); }

private slots:
    void mode_menu_change_slot(int);
    void categ_menu_change_slot(int);
    void attr_menu_change_slot(int);
    void color_selected_slot(const QColor&);
    void help_btn_slot();
    void colors_btn_slot(bool);
    void apply_btn_slot();
    void dismiss_btn_slot();

private:
    void update_color();
    void fill_categ_menu();
    void fill_attr_menu(int);

    static void c_set_rgb(int, int, int);
    static void c_get_rgb(int, int*, int*, int*);
    static void c_list_callback(const char*, void*);

    GRobject c_caller;
    QComboBox   *c_modemenu;
    QComboBox   *c_categmenu;
    QComboBox   *c_attrmenu;
    QColorDialog *c_clrd;
    QPushButton *c_listbtn;
    GRlistPopup *c_listpop;
    DisplayMode c_display_mode;
    DisplayMode c_ref_mode;

    int c_mode;
    int c_red;
    int c_green;
    int c_blue;

    static cColor *instPtr;

    static clritem Menu1[];
    static clritem Menu2[];
    static clritem Menu3[];
};

#endif


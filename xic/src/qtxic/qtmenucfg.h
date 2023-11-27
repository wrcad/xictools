
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

#ifndef QTMENUCFG_H
#define QTMENUCFG_H

#include "dsp.h"
#include "dsp_window.h"

#include <QObject>
#include <QAction>


struct MenuEnt;
struct MenuList;

class QTmenuConfig : public QObject
{
    Q_OBJECT

public:
    QTmenuConfig();
    ~QTmenuConfig();

    void instantiateMainMenus();
    void instantiateTopButtonMenu();
    void instantiateSideButtonMenus();
    void instantiateSubwMenus(int);
    void updateDynamicMenus();
    void switch_menu_mode(DisplayMode, int);
    void set_main_global_sens(const MenuList*, bool);

    bool menu_disabled() const  { return (mc_menu_disabled); }

    static QTmenuConfig *self()
    {
        if (!instancePtr)
            on_null_ptr();
        return (instancePtr);
    }

signals:
    void exec_idle(MenuEnt*);

private slots:
    void file_menu_slot(QAction*);
    void file_open_menu_slot(QAction*);
    void cell_menu_slot(QAction*);
    void edit_menu_slot(QAction*);
    void modf_menu_slot(QAction*);
    void view_menu_slot(QAction*);
    void view_view_menu_slot(QAction*);
    void attr_menu_slot(QAction*);
    void attr_main_win_menu_slot(QAction*);
    void attr_main_win_obj_menu_slot(QAction*);
    void cvrt_menu_slot(QAction*);
    void drc_menu_slot(QAction*);
    void ext_menu_slot(QAction*);
    void user_menu_slot(QAction*);
    void help_menu_slot(QAction*);

    void subwin_view_menu_slot(QAction*);
    void subwin_view_view_menu_slot(QAction*);
    void subwin_attr_menu_slot(QAction*);
    void subwin_help_menu_slot(QAction*);

    void idle_exec_slot(MenuEnt*);
    void exec_slot(MenuEnt*);
    void style_slot(MenuEnt*);
    void shape_slot(MenuEnt*);

    void style_menu_slot(QAction*);
    void shape_menu_slot(QAction*);

private:
    const char **get_style_pixmap();
    static void on_null_ptr();

    QMenu *mc_style_menu;
    QMenu *mc_shape_menu;
    bool mc_menu_disabled;

    static QTmenuConfig *instancePtr;
};

#endif


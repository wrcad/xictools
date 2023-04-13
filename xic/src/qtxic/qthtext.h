
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

//
// Header for the "hypertext" editor and related.
//

#ifndef QTHTEXT_H
#define QTHTEXT_H

#include "cd_hypertext.h"
#include "promptline.h"
#include "qtinterf/qtinterf.h"

// String storage registers, 0 is "last", 1-5 are general.
#define PE_NUMSTORES 6

inline class QTedit *qtEdit();

class QTedit : public cPromptEdit, public qt_draw
{
    static QTedit *ptr() { return (instancePtr); }

public:
    friend inline QTedit *qtEdit() { return (QTedit::ptr()); }

    QTedit(bool, QWidget*);

    // the widgets are owned by the main window
//XXX    virtual ~QTedit() { viewport = 0; }

    // virtual overrides
    void flash_msg(const char*, ...);
    void flash_msg_here(int, int, const char*, ...);
    void save_line();
    int win_width(bool = false);
    int win_height();
    void set_focus();
    void set_indicate();
    void show_lt_button(bool);
    void get_selection(bool);
    void *setup_backing(bool);
    void restore_backing(void*);
    void init_window();
    bool check_pixmap();
    void init_selection(bool);
    void warp_pointer();

    QWidget *container()    { return (pe_container); }
    QWidget *keys()         { return (pe_keys); }
    int xpos()              { return (pe_colmin*pe_fntwid); }

    /*
    // virtual overrides
    int hyWidth(bool = false);
    void hySetFocus();
    void hySetIndicate();
    void hyShowLbutton(bool);
    void hyGetSelection();
    void *hySetupBacking(bool);
    void hyRestoreBacking(void*);
    void hyInitWindow();
    void hyCheckPixmap();
    */

private:
    QWidget *pe_container;
    QWidget *pe_keys;

    static hyList *pe_stores[PE_NUMSTORES]; // Editor text string registers.

    static QTedit *instancePtr;
};

#endif

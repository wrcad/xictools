
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
// Header for the layer table composite.
//

#ifndef QTLTAB_H
#define QTLTAB_H

#include "main.h"
#include "dsp.h"
#include "dsp_window.h"
#include "layertab.h"
#include "events.h"
#include "qtinterf/qtinterf.h"
#include "qtinterf/draw_if.h"

#include <QWidget>

class QPushButton;
class QScrollBar;
class QHBoxLayout;
class QMouseEvent;
class QResizeEvent;

inline class QTltab *qtLtab();

class QTltab : public cLtab, public qt_draw
{
    static QTltab *ptr() { return (instancePtr); }

public:
    friend inline QTltab *qtLtab() { return (QTltab::ptr()); }

    QTltab(bool, QWidget*);

    void setup_drawable();
    void blink(CDl*);
    void show(const CDl* = 0);
    void refresh(int, int, int, int);
    void win_size(int*, int*);
    void update();
    void update_scrollbar();
    void hide_layer_table(bool);
    void set_layer();

//    QWidget *container()        { return (ltab_container); }
//    QWidget *scrollbar()        { return (ltab_scrollbar); }
//    QWidget *searcher()         { return (ltab_search_container); }

    /*
    void setup_drawable();
    void blink(CDl*);
    void refresh(int, int, int, int);
    void lspec_callback();
    void win_size(int*, int*);
    void update();
    void update_scrollbar();
    */

private:
    static QTltab *instancePtr;
};

class layertab_w : public QWidget
{
    Q_OBJECT

public:
    layertab_w(int, QWidget*);

    draw_if *draw_area() { return (viewport); }
    QPushButton *ls_button() { return (lspec_btn); }

    void update_scrollbar();

private slots:
    void button_press_slot(QMouseEvent*);
    void button_release_slot(QMouseEvent*);
    void resize_slot(QResizeEvent*);
    void s_btn_slot(bool);
    void ltab_scroll_value_changed_slot(int);

private:
    draw_if *viewport;
    QPushButton *lspec_btn;
    QScrollBar *ltab_sb;
};

#endif

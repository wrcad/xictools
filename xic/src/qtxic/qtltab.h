
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

#include <QWidget>

class QToolButton;
class QScrollBar;
class QHBoxLayout;
class QMouseEvent;
class QResizeEvent;
class QDragEnterEvent;
class QDropEvent;
class QLineEdit;

Q_DECLARE_METATYPE(LayerFillData);

class QTltab : public QWidget, public cLtab, public QTdraw
{
    Q_OBJECT

public:
    QTltab(bool);
    ~QTltab();

    void setup_drawable();
    void blink(CDl*);
    void show(const CDl* = 0);
    void refresh(int, int, int, int);
    void win_size(int*, int*);
    void update();
    void update_scrollbar();
    void hide_layer_table(bool);
    void set_layer();

    // Instantiated in main window.
    void set_search_widgets(QToolButton*, QLineEdit*);

    QScrollBar *scrollbar()                 { return (ltab_scrollbar); }

    static const char *mime_type()          { return (ltab_mime_type); }
    static const char **fillpattern_xpm()   { return (ltab_fillpattern_xpm); }

    static QTltab *self()
    {
        if (!instancePtr)
            on_null_ptr();
        return (instancePtr);
    }

    QSize sizeHint()            const { return (QSize(160, -1)); }

signals:
    void valueChanged(int);

private slots:
    void button_press_slot(QMouseEvent*);
    void button_release_slot(QMouseEvent*);
    void motion_slot(QMouseEvent*);
    void drag_enter_slot(QDragEnterEvent*);
    void drop_event_slot(QDropEvent*);
    void resize_slot(QResizeEvent*);
    void ltab_scroll_value_changed_slot(int);
    void font_changed(int);
    void pressed_slot();

private:
    static void on_null_ptr();
    static int  ltab_ent_timer(void*);

    QScrollBar  *ltab_scrollbar;
    QLineEdit   *ltab_entry;
    QToolButton *ltab_sbtn;
    QPixmap     *ltab_lsearch;
    QPixmap     *ltab_lsearchn;

    char        *ltab_search_str;
    int         ltab_last_index;
    int         ltab_last_mode;
    int         ltab_timer_id;
    bool        ltab_hidden;

    static const char *ltab_mime_type;
    static const char *ltab_fillpattern_xpm[];
    static QTltab *instancePtr;
};

#endif

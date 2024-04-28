
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
 * QtInterf Graphical Interface Library                                   *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#ifndef QTTEXTW_H
#define QTTEXTW_H

#include <QTextBrowser>
#include <QMimeData>
#include <QDragEnterEvent>


class QResizeEvent;
class QMouseEvent;
class QDropEvent;
class QWidget;

namespace qtinterf {
    class QTtextEdit;
}

class qtinterf::QTtextEdit : public QTextBrowser
{
    Q_OBJECT

public:
    QTtextEdit(QWidget* = 0);

    // editing
    void delete_chars(int, int);
    void replace_chars(const QColor*, const char*, int, int);

    // insertion
    void set_editable(bool);
    void insert_chars_at_point(const QColor*, const char*, int, int);
    int get_insertion_point();
    void set_insertion_point(int);
    void set_chars(const char*);

    // get text
    char *get_chars(int = 0, int = -1);
    int get_length();

    // selection
    bool has_selection();
    char *get_selection();
    void get_selection_pos(int*, int*);
    void select_range(int, int);

    // scrolling
    int get_scroll_value();
    void set_scroll_value(int);

    Qt::DropAction drop_action()    const { return (tw_drop_action); }

signals:
    void resize_event(QResizeEvent*);
    void press_event(QMouseEvent*);
    void release_event(QMouseEvent*);
    void motion_event(QMouseEvent*);
    void mime_data_handled(const QMimeData*, int*) const;
    void mime_data_delivered(const QMimeData*, int*);
    void key_press_event(QKeyEvent*);

protected:
    void resizeEvent(QResizeEvent*);
    void mousePressEvent(QMouseEvent*);
    void mouseReleaseEvent(QMouseEvent*);
    void mouseMoveEvent(QMouseEvent*);
    void dragEnterEvent(QDragEnterEvent*);
    void dragMoveEvent(QDragMoveEvent*);
    void dropEvent(QDropEvent*);
    bool canInsertFromMimeData(const QMimeData*) const;
    void insertFromMimeData(const QMimeData*);
    void keyPressEvent(QKeyEvent*);
    bool event(QEvent*);

private:
    Qt::DropAction tw_drop_action;
};

#endif


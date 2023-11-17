
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

#include <QTextEdit>
#include <QMimeData>
#include <QDragEnterEvent>

class QResizeEvent;
class QMouseEvent;
class QDropEvent;
class QWidget;

namespace qtinterf {
    class QTtextEdit;
}

class qtinterf::QTtextEdit : public QTextEdit
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

    // clipboard
    void cut_clipboard();
    void copy_clipboard();
    void paste_clipboard();

    // selection
    bool has_selection();
    char *get_selection();
    void get_selection_pos(int*, int*);
    void select_range(int, int);

    // scrolling
    int get_scroll_value();
    void set_scroll_value(int);
    int get_scroll_to_line_value(int, int);
    void scroll_to_pos(int);

signals:
    void resize_event(QResizeEvent*);
    void press_event(QMouseEvent*);
    void motion_event(QMouseEvent*);
    void mime_data_received(const QMimeData*);
    void key_press_event(QKeyEvent*);

protected:
    void resizeEvent(QResizeEvent *ev)
    {
        QTextEdit::resizeEvent(ev);
        emit resize_event(ev);
    }

    void mousePressEvent(QMouseEvent *ev)
    {
        emit press_event(ev);
    }

    void mouseMoveEvent(QMouseEvent *ev)
    {
        emit motion_event(ev);
    }

    // Tricky stuff here to allow window to handle drag/drop while
    // in read-only mode.

    void dragEnterEvent(QDragEnterEvent *ev)
    {
        if (canInsertFromMimeData(ev->mimeData()))
            ev->acceptProposedAction();
    }

    void dragMoveEvent(QDragMoveEvent *ev)
    {
        if (canInsertFromMimeData(ev->mimeData()))
            ev->acceptProposedAction();
    }

    void dropEvent(QDropEvent *ev)
    {
        insertFromMimeData(ev->mimeData());
        ev->acceptProposedAction();
    }

    bool canInsertFromMimeData(const QMimeData *source) const
    {
        // Extend mime types as needed.
        if (source->hasFormat("text/property"))
            return (true);
        return (QTextEdit::canInsertFromMimeData(source));
    }

    void insertFromMimeData(const QMimeData *source)
    {
        emit mime_data_received(source);
    }

    void keyPressEvent(QKeyEvent *ev)
    {
        emit key_press_event(ev);
        if (ev->isAccepted())
            QTextEdit::keyPressEvent(ev);
    }
};

#endif


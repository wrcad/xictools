
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

// Derive a new class to expose the button press override.

namespace qtinterf
{
    class QTtextEdit : public QTextEdit
    {
        Q_OBJECT

    public:
        QTtextEdit();

        // qttextw.cc
        bool has_selection();
        char *get_selection();
        void select_range(int, int);
        char *get_chars(int, int);
        void set_chars(const char*);
        int get_scroll_value();
        void set_scroll_value(int);


    signals:
        void resize_event(QResizeEvent*);
        void press_event(QMouseEvent*);
        void motion_event(QMouseEvent*);
        void mime_data_received(const QMimeData*);

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

        void dragEnterEvent(QDragEnterEvent *ev) {
            if (canInsertFromMimeData(ev->mimeData()))
                ev->acceptProposedAction();
        }

        void dragMoveEvent(QDragMoveEvent *ev) {
            if (canInsertFromMimeData(ev->mimeData()))
                ev->acceptProposedAction();
        }

        void dropEvent(QDropEvent *ev) {
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

        void insertFromMimeData(const QMimeData *source) {
            emit mime_data_received(source);
        }
    };
}

#endif


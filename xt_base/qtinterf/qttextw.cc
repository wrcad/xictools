
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

#include "qttextw.h"
#include "miscutil/lstring.h"

#include <QScrollBar>
#include <QGuiApplication>
#include <QTextDocument>
#include <QTextBlock>
#include <QClipboard>


// A fixed text window class for general use, incorporates the
// "text_window" utilities of the GTK verssion.

using namespace qtinterf;


QTtextEdit::QTtextEdit(QWidget *prnt) : QTextBrowser(prnt)
{
    // Set an arrow cursor rather than te default I-beam.
    viewport()->setCursor(Qt::ArrowCursor);

    // Default to no wrapping, horizontal scroll bar should appear
    // insted.
    setLineWrapMode(QTextEdit::NoWrap);

    tw_drop_action = Qt::IgnoreAction;
}


// Editing.

// Delete the characters from start to end, -1 indicates end of text.
//
void
QTtextEdit::delete_chars(int start, int end)
{
    if (start == end)
        return;
    QTextCursor c = textCursor();
    c.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor);
    if (start) {
        c.movePosition(QTextCursor::NextCharacter,
            QTextCursor::MoveAnchor, start);
    }
    if (end < 0)
        c.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
    else if (end > start) {
        c.movePosition(QTextCursor::NextCharacter,
            QTextCursor::KeepAnchor, end-start);
    }
    else {
        c.movePosition(QTextCursor::PreviousCharacter,
            QTextCursor::KeepAnchor, start-end);
    }
    c.removeSelectedText();
    setTextCursor(c);
}


// Replace the chars from start to end with the same number of chars
// from string, -1 indicates end of text.  If -1 is given for end,
// the entirety of string will be inserted.
//
void
QTtextEdit::replace_chars(const QColor *color, const char *string,
    int start, int end)
{
    if (!string)
        return;
    if (start == end)
        return;
    delete_chars(start, end);
    QTextCursor c = textCursor();
    QColor clr = textColor();
    if (color)
        setTextColor(*color);
    if (end < 0)
        insertPlainText(string);
    else {
        int nc = abs(end - start);
        int len = strlen(string);
        if (len <= nc)
            insertPlainText(string);
        else {
            char *tstr = lstring::copy(string);
            tstr[nc] = 0;
            insertPlainText(tstr);
            delete [] tstr;
        }
    }
    setTextCursor(c);
    setTextColor(clr);
}


// Insertion.

void
QTtextEdit::set_editable(bool editable)
{
    setReadOnly(!editable);
}


// Insert nc chars from string into the text window at the given
// position.  If nc is -1, string must be null terminated and will be
// inserted in its entirety.  If pos is -1, insertion will start at
// the end of existing text.
//
void
QTtextEdit::insert_chars_at_point(const QColor *color, const char *string,
    int nc, int posn)
{
    if (!string || nc == 0)
        return;
    QTextCursor c = textCursor();
    if (posn < 0)
        c.movePosition(QTextCursor::End, QTextCursor::MoveAnchor);
    else {
        c.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor);
        if (posn) {
            c.movePosition(QTextCursor::NextCharacter,
                QTextCursor::MoveAnchor, posn);
        }
    }
    setTextCursor(c);
    if (color)
        setTextColor(*color);
    else
        setTextColor(QColor("black"));
    if (nc < 0) {
        insertPlainText(string);
        return;
    }
    int len = strlen(string);
    if (len <= nc) {
        insertPlainText(string);
        return;
    }
    char *tstr = lstring::copy(string);
    tstr[nc] = 0;
    insertPlainText(tstr);
    delete [] tstr;
}


// Return the cursor offset into text.
//
int
QTtextEdit::get_insertion_point()
{
    QTextCursor c = textCursor();
    return (c.position());
}


// Set the cursor offset into text, -1 indicates end.
//
void
QTtextEdit::set_insertion_point(int posn)
{
    QTextCursor c = textCursor();
    if (posn < 0)
        c.movePosition(QTextCursor::End, QTextCursor::MoveAnchor);
    else {
        c.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor);
        if (posn) {
            c.movePosition(QTextCursor::NextCharacter,
                QTextCursor::MoveAnchor, posn);
        }
    }
    setTextCursor(c);
}


// Set the text in the text window, discarding any previous content.
//
void
QTtextEdit::set_chars(const char *str)
{
    setPlainText(str);
}


// Get text.

// Return the chars from start to end, -1 indicates end of text.
//
char *
QTtextEdit::get_chars(int start, int end)
{
    QByteArray qba = toPlainText().toLatin1();
    if (end < 0 || end > qba.length())
        end = qba.length();
    if (start < 0)
        start = 0;
    if (end < start) {
        int t = end;
        end = start;
        start = t;
    }
    char *str = new char[end - start + 1];
    int i = 0;
    while (start < end)
        str[i++] = qba[start++];
    str[i] = 0;
    return (str);
}


// Return the number of characters in the text.
//
int
QTtextEdit::get_length()
{
    return (toPlainText().toLatin1().size());
}


// Selection.

// Return true if the text window has a selection.
//
bool
QTtextEdit::has_selection()
{
    QTextCursor c = textCursor();
    return (c.position() != c.anchor());
}


// Return the selected text.
//
char *
QTtextEdit::get_selection()
{
    QTextCursor c = textCursor();
    if (c.position() == c.anchor())
        return (0);
    int s = c.position();
    int e = c.anchor();
    if (e < s) {
        int t = e;
        e = s;
        s = t;
    }
    char *sel = new char[e-s + 1];
    QByteArray qba = toPlainText().toLatin1();
    int i = 0;
    while (s < e)
        sel[i++] = qba[s++];
    sel[i] = 0;
    return (sel);
}


// Return the offsets of selected text.
//
void
QTtextEdit::get_selection_pos(int *strtp, int *endp)
{
    QTextCursor c = textCursor();
    if (strtp)
        *strtp = c.anchor();
    if (endp)
        *endp = c.position();
}


// Select the chars in the range, start=end deselects existing.
//
void
QTtextEdit::select_range(int start, int end)
{
    if (start == end) {
        QTextCursor c = textCursor();
        c.clearSelection();
        setTextCursor(c);
    }
    else {
        QTextCursor c = textCursor();
        c.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor);
        if (start) {
            c.movePosition(QTextCursor::NextCharacter,
                QTextCursor::MoveAnchor, start);
        }
        if (end < 0)
            c.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
        else if (end > start) {
            c.movePosition(QTextCursor::NextCharacter,
                QTextCursor::KeepAnchor, end-start);
        }
        else {
            c.movePosition(QTextCursor::PreviousCharacter,
                QTextCursor::KeepAnchor, start-end);
        }
        setTextCursor(c);
    }
}


// Scrolling.

// Return the current scroll position.
//
int
QTtextEdit::get_scroll_value()
{
    QScrollBar *vsb = verticalScrollBar();
    int val = 0;
    if (vsb)
        val = (int)vsb->value();
    return (val);
}


// Set the scroll position.
//
void
QTtextEdit::set_scroll_value(int val)
{
    QScrollBar *vsb = verticalScrollBar();
    if (vsb)
        vsb->setValue(val);
}


void
QTtextEdit::resizeEvent(QResizeEvent *ev)
{
    QTextEdit::resizeEvent(ev);
    emit resize_event(ev);
}


void
QTtextEdit::mousePressEvent(QMouseEvent *ev)
{
    ev->ignore();
    emit press_event(ev);
    if (!ev->isAccepted())
        QTextEdit::mousePressEvent(ev);
}


void
QTtextEdit::mouseReleaseEvent(QMouseEvent *ev)
{
    ev->ignore();
    emit release_event(ev);
    if (!ev->isAccepted())
        QTextEdit::mouseReleaseEvent(ev);
}


void
QTtextEdit::mouseMoveEvent(QMouseEvent *ev)
{
    ev->ignore();
    emit motion_event(ev);
    if (!ev->isAccepted())
        QTextEdit::mouseMoveEvent(ev);
}


// Tricky stuff here to allow window to handle drag/drop while
// in read-only mode.

void
QTtextEdit::dragEnterEvent(QDragEnterEvent *ev)
{
    if (canInsertFromMimeData(ev->mimeData()))
        ev->acceptProposedAction();
}


void
QTtextEdit::dragMoveEvent(QDragMoveEvent *ev)
{
    if (canInsertFromMimeData(ev->mimeData()))
        ev->acceptProposedAction();
}


void
QTtextEdit::dropEvent(QDropEvent *ev)
{
    insertFromMimeData(ev->mimeData());
    ev->acceptProposedAction();
    tw_drop_action = ev->proposedAction();
}


bool
QTtextEdit::canInsertFromMimeData(const QMimeData *src) const
{
    // handled: if > 0 accept, if < 0 reject, if == 0 default action.
    int handled = 0;
    emit mime_data_handled(src, &handled);
    if (handled > 0)
        return (true);
    if (handled < 0)
        return (false);
    return (QTextEdit::canInsertFromMimeData(src));
}


void
QTtextEdit::insertFromMimeData(const QMimeData *src)
{
    int handled = 0;
    emit mime_data_delivered(src, &handled);
    if (!handled)
        QTextEdit::insertFromMimeData(src);
}


void
QTtextEdit::keyPressEvent(QKeyEvent *ev)
{
    emit key_press_event(ev);
    if (ev->isAccepted())
        QTextEdit::keyPressEvent(ev);
}


bool
QTtextEdit::event(QEvent *ev)
{
    // Override Ctrl-C when read-only so this can be used for "copy".
    // Ordinarily, this shortcut will be recognized but do nothing.
    if (ev->type() == QEvent::ShortcutOverride) {
        if (isReadOnly()) {
            QKeyEvent *kev = dynamic_cast<QKeyEvent*>(ev);
            if (kev && kev->key() == Qt::Key_C &&
                    (kev->modifiers() & Qt::ControlModifier))
            {
                emit key_press_event(kev);
                ev->accept();
            }
        }
    }
    return (QTextEdit::event(ev));
}


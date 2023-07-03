
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

#include <QScrollBar>


// A fixed text window class for general use, replaces the
// "text_window" utilities in the GTK verssion.

using namespace qtinterf;

QTtextEdit::QTtextEdit()
{
    // Doesn't work, fixme!
    viewport()->setCursor(Qt::ArrowCursor);
}


bool
QTtextEdit::has_selection()
{
    QTextCursor c = textCursor();
    return (c.position() != c.anchor());
}


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


// Select the chars in the range, start=end deselects existing.
//
void
QTtextEdit::select_range(int start, int end)
{
    if (start == end) {
        QTextCursor c = textCursor();
        int pos = c.position();
        int apos = c.anchor();
        if (apos != pos) {
            if (pos < apos) {
                int t = pos;
                pos = apos;
                apos = t;
            }
            int n = pos - apos;
            c.movePosition(QTextCursor::PreviousCharacter,
                QTextCursor::KeepAnchor, n);
            setTextCursor(c);
        }
    }
    else {
        if (start > end) {
            int t = start;
            start = end;
            end = t;
        }
        QTextCursor c = textCursor();
        c.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor);
        if (start) {
            c.movePosition(QTextCursor::NextCharacter, QTextCursor::MoveAnchor,
                start);
        }
        c.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor,
            end-start);
        setTextCursor(c);
    }
}


char *
QTtextEdit::get_chars(int start, int end)
{
    QByteArray qba = toPlainText().toLatin1();
    if (start < 0)
        start = 0;
    if (end < 0 || end > qba.length())
        end = qba.length();
    if (end <= start)
        return (0);
    char *str = new char[end - start + 1];
    int i = 0;
    while (start < end)
        str[i++] = qba[start++];
    str[i] = 0;
    return (str);
}


void
QTtextEdit::set_chars(const char *str)
{
    setPlainText(str);
}


int
QTtextEdit::get_scroll_value()
{
    QScrollBar *vsb = verticalScrollBar();
    int val = 0;
    if (vsb)
        val = (int)vsb->value();
    return (val);
}


void
QTtextEdit::set_scroll_value(int val)
{
    QScrollBar *vsb = verticalScrollBar();
    if (vsb)
        vsb->setValue(val);
}


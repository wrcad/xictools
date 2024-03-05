
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

#ifndef QTPARAM_H
#define QTPARAM_H

#include <QWidget>
#include "qtinterf/qtinterf.h"


//-----------------------------------------------------------------------------
// QTparam:  Parameter/status display area of the main window.

class QTmainwin;
class QTparam;
class QMouseEvent;
class QResizeEvent;

// Represent a character to display.
//
struct pchar_t
{
    char pc_char;               // ASCII code
    unsigned char pc_width;     // pixel width
    unsigned short pc_posn;     // pixel bounding box left
    unsigned int pc_color;      // color value
};

// Container for displayed characters.
//
struct ptext_t
{
    ptext_t()
        {
            pt_end = 0;
            pt_sel_start = 0;
            pt_sel_end = 0;
            pt_set = false;
        }

    void append_string(const char *str, unsigned int clr)
        {
            if (str) {
                while (*str) {
                    append_char(*str, clr);
                    str++;
                }
            }
        }

    void append_char(char c, unsigned int clr)
        {
            if (pt_end < 255) {
                pchar_t *pc = pt_chars + pt_end++;
                pc->pc_char = c;
                pc->pc_width = 0;
                pc->pc_posn = 0;
                pc->pc_color = clr;
            }
        }

    void reset()
        {
            pt_end = 0;
            pt_set = false;
        }

    char *get_sel()
        {
            if (pt_sel_end > pt_sel_start) {
                char *str = new char[pt_sel_end - pt_sel_start + 1];
                char *s = str;
                unsigned int i = pt_sel_start;
                while (i < pt_sel_end)
                    *s++ = pt_chars[i++].pc_char;
                *s = 0;
                return (str);
            }
            return (0);
        }

    bool has_sel(unsigned int *xmin, unsigned int *xmax)
        {
            if (pt_sel_end > pt_sel_start) {
                if (xmin)
                    *xmin = pt_sel_start;
                if (xmax)
                    *xmax = pt_sel_end;
                return (true);
            }
            return (false);
        }

    void set_sel(unsigned int s, unsigned int e)
        {
            if (s < 256 && e < 256 && e >= s) {
                pt_sel_start = s;
                pt_sel_end = e;
            }
        }

    void setup(QTparam*);
    void display(QTparam*, unsigned int, unsigned int);
    void display_c(QTparam*, int, int);
    bool select(int, int);
    bool select_word(int);

private:
    unsigned char pt_end;           // string end (last char + 1)
    unsigned char pt_sel_start;     // selecton start
    unsigned char pt_sel_end;       // selection end (+ 1)
    bool pt_set;                    // true if char widths set
    pchar_t pt_chars[256];
};


// The status readout line, singleton, instantiated in main window.
//
class QTparam : public QWidget, public QTdraw
{
    Q_OBJECT

public:
    QTparam(QTmainwin*);
    ~QTparam();

    static QTparam *self()      { return (instPtr); }

    QSize sizeHint() const
        {
            return (QSize(200, QFontMetrics(font()).height()));
        }

    void print();
    void display(int, int);

    void redraw()               { print(); }

    unsigned int xval()         { return (p_xval); }
    unsigned int yval()         { return (p_yval); }

private slots:
    void font_changed_slot(int);
    void resize_slot(QResizeEvent*);
    void button_down_slot(QMouseEvent*);
    void button_up_slot(QMouseEvent*);
    void motion_slot(QMouseEvent*);

private:
    void select(int, int);
    void select_word(int);
    bool deselect();

    int p_drag_x;
    int p_drag_y;
    bool p_has_drag;
    bool p_dragged;
    int p_xval;
    int p_yval;
    int p_width;
    int p_height;
    ptext_t p_text;

    static QTparam *instPtr;
};

#endif


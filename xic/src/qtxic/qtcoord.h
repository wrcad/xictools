
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

#ifndef QTCOORD_H
#define QTCOORD_H

#include <QWidget>
#include "qtinterf/qtinterf.h"

class QTmainwin;

inline class cCoord *Coord();

// The coordinate readout
class cCoord : public QWidget, public QTdraw
{
    Q_OBJECT

private slots:
    void redraw_slot();

public:
    // update mode for cCoord::print()
    enum { COOR_BEGIN, COOR_MOTION, COOR_REL };

    friend inline cCoord *Coord() { return (cCoord::instancePtr); }

    cCoord(QTmainwin*);
    ~cCoord();

    void print(int, int, int);

    void set_mode(int x, int y, bool relative, bool snap)
    {
        co_x = x;
        co_y = y;
        co_rel = relative;
        co_snap = snap;
        print(0, 0, COOR_REL);
    }

    void redraw()
    {
        redraw_slot();
    }

private:
    int co_width;
    int co_height;
    int co_lx;
    int co_ly;
    int co_x;
    int co_y;
    bool co_rel;
    bool co_snap;

    static cCoord *instancePtr;
};

#endif


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

#include "qtactivity.h"

#include <QPainter>
#include <QBrush>
#include <QPixmap>

//
// Activity Indicator Widget
//
// When active, translates a smiley back and forth.
//


using namespace qtinterf;

// XPM
static const char * const smile_xpm[] = {
// columns rows colors chars-per-pixel
"15 15 5 1",
"  c black",
". c gray75",
"x c yellow",
"o c black",
"O c None",
/* pixels */
"OOOOO     OOOOO",
"OOO  xxxxx  OOO",
"OO xxxxxxxxx OO",
"O xxxxxxxxxxx O",
"O xx  xxx  xx O",
" xxx  xxx  xxx ",
" xxxxxxxxxxxxx ",
" xxxxxxxxxxxxx ",
" xxxxxxxxxxxxx ",
" xx xxxxxxx xx ",
"O xx xxxxx xx O",
"O xxx     xxx O",
"OO xxxxxxxxx OO",
"OOO  xxxxx  OOO",
"OOOOO     OOOOO"
};


QTactivity::QTactivity(QWidget *prnt) : QWidget(prnt)
{
    a_pos_x = 0;
    a_vel_x = 4;
    a_rad = 8;  // half image size
    a_active = false;
    a_pixmap = new QPixmap(smile_xpm);
    connect(&a_timer, &QTimer::timeout, this, &QTactivity::increment_slot);
}


QTactivity::~QTactivity()
{
    delete a_pixmap;
}


void
QTactivity::start()
{
    a_pos_x = 0;
    a_vel_x = 4;
    a_timer.start(100);
    a_active = true;
}

void
QTactivity::stop()
{
    a_timer.stop();
    a_active = false;
    increment_slot();
}


void
QTactivity::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.eraseRect(0, 0, size().width(), size().height());
    if (a_active)
        p.drawPixmap(a_pos_x, size().height()/2 - a_rad, *a_pixmap);
    else
        p.drawText(a_rad, size().height()/2 + a_rad, QString("idle"));
    p.end();
}


void
QTactivity::increment_slot()
{
    int nx = a_pos_x + a_vel_x;
    if (nx + 2*a_rad > size().width()) {
        a_vel_x = -a_vel_x;
        nx += (size().width() - nx - 2*a_rad);
    }
    if (nx < 0) {
        a_vel_x = -a_vel_x;
        nx = - nx;
    }
    a_pos_x = nx;
    repaint(0, 0, size().width(), size().height());
}


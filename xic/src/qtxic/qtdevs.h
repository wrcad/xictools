
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

#ifndef QTDEVS_H
#define QTDEVS_H

#include "main.h"
#include "qtmain.h"

#include <QDialog>


//-----------------------------------------------------------------------------
// QTdevMenuDlg::This implements a menu of devices from the device
// library, in three styles.

class QPushButton;
class QToolButton;

class QTdevMenuDlg : public QDialog, public QTbag, public QTdraw
{
    Q_OBJECT

public:
    // Device list element
    //
    struct sEnt
    {
        sEnt() : name(0), x(0), width(0) { }
        ~sEnt() { delete [] name; }

        char *name;
        int x;
        int width;
    };

    enum dvType { dvMenuCateg, dvMenuAlpha, dvMenuPict };

    QTdevMenuDlg(GRobject, stringlist*);
    ~QTdevMenuDlg();

    QSize sizeHint() const;

    void activate(bool);
    void esc();

    GRobject get_caller()
        {
            GRobject tc = dv_caller;
            dv_caller = 0;
            return (tc);
        }

    void set_transient_for(QWidget *prnt)
        {
            Qt::WindowFlags f = windowFlags();
            setParent(prnt);
#ifdef __APPLE__
            f |= Qt::Tool;
#endif
            setWindowFlags(f);
        }

    // Don't pop down from Esc press.
    void keyPressEvent(QKeyEvent *ev)
        {
            if (ev->key() != Qt::Key_Escape)
                QDialog::keyPressEvent(ev);
        }

    bool is_active()                    { return (dv_active); }
    static QTdevMenuDlg *self()         { return (instPtr); }

private slots:
    void menu_slot(QAction*);
    void style_btn_slot();
    void more_btn_slot();
    void font_changed_slot(int);
    void button_down_slot(QMouseEvent*);
    void button_up_slot(QMouseEvent*);
    void motion_slot(QMouseEvent*);
    void enter_slot(QEnterEvent*);
    void leave_slot(QEvent*);
    void resize_slot(QResizeEvent*);

private:
    int init_sizes();
    void render_cell(int, bool);
    void cyclemore();
    int  whichent(int);
    void show_selected(int);
    void show_unselected(int);
    void redraw();

    GRobject    dv_caller;
    QToolButton *dv_morebtn;
    sEnt        *dv_entries;
    int         dv_numdevs;
    int         dv_leftindx;
    int         dv_leftofst;
    int         dv_rightindx;
    int         dv_curdev;
    int         dv_pressed;
    int         dv_px;
    int         dv_py;
    int         dv_width;
    unsigned int dv_foreg;
    unsigned int dv_backg;
    unsigned int dv_hlite;
    unsigned int dv_selec;
    bool        dv_active;
    dvType      dv_type;

    static QTdevMenuDlg *instPtr;
};

#endif



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
 * WRspice Circuit Simulation and Analysis Tool                           *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#ifndef QTPLOT_H
#define QTPLOT_H

#include "qtinterf/qtinterf.h"
#include "qtinterf/qtdraw.h"

#include <QDialog>
#include <QKeyEvent>


//-----------------------------------------------------------------------------
// QTplotDlg:  plot and mplot display dialogs.

#define MIME_TYPE_TRACE     "application/wrspice-trace"

struct sGraph;
class QGroupBox;
class QPushButton;
class QResizeEvent;
class QPaintEvent;
class QMouseEvent;
class QKeyEvent;
class QEnterEvent;
class QFocusEvent;
class QDragMoveEvent;
class QDragEnterEvent;
class QDropEvent;

class QTplotDlg : public QDialog, public QTbag,  public QTdraw
{
    Q_OBJECT

public:
    friend struct sGraph;

    // Indices into the buttons array for the button witget pointers.
    enum pbtn_type
    {
        pbtn_dismiss,
        pbtn_help,
        pbtn_redraw,
        pbtn_print,
        pbtn_saveplot,
        pbtn_saveprint,
        pbtn_points,
        pbtn_comb,
        pbtn_logx,
        pbtn_logy,
        pbtn_marker,
        pbtn_separate,
        pbtn_single,
        pbtn_group,
        pbtn_NUMBTNS
    };

    QTplotDlg(int type) : QTbag(this), QTdraw(type)
    {
        pb_graph = 0;
        pb_gbox = 0;
        for (int i = 0; i < pbtn_NUMBTNS; i++)
            pb_checkwins[i] = 0;
        pb_id = 0;
        pb_x = pb_y = 0;
        pb_rdid = 0;
        pb_event_test = false;
        pb_event_deferred = false;
        setAttribute(Qt::WA_DeleteOnClose);
    }

    ~QTplotDlg()
    {
        if (pb_id)
            QTdev::self()->RemoveIdleProc(pb_id);
    }

    // Don't pop down from Esc press.
    void keyPressEvent(QKeyEvent *ev)
        {
            if (ev->key() != Qt::Key_Escape)
                QDialog::keyPressEvent(ev);
        }

    QSize sizeHint() const;

    bool init(sGraph*);
    bool init_gbuttons();

    bool event(QEvent*);

    void enable_event_test(bool b)
    {
        pb_event_test = b;
        if (b)
            pb_event_deferred = false;
    }

    bool event_deferred()               { return (pb_event_deferred); }

private slots:
    void font_changed_slot(int);
    void resize_slot(QResizeEvent*);
    void button_down_slot(QMouseEvent*);
    void button_up_slot(QMouseEvent*);
    void motion_slot(QMouseEvent*);
    void key_down_slot(QKeyEvent*);
    void enter_slot(QEnterEvent*);
    void leave_slot(QEvent*);
    void dismiss_btn_slot();
    void help_btn_slot();
    void redraw_btn_slot();
    void print_btn_slot(bool);
    void saveplt_btn_slot();
    void savepr_btn_slot();
    void ptype_btn_slot(bool);
    void logx_btn_slot(bool);
    void logy_btn_slot(bool);
    void marker_btn_slot(bool);
    void separate_btn_slot(bool);
    void single_btn_slot(bool);
    void group_btn_slot(bool);
    void drag_enter_slot(QDragEnterEvent*);
    void drop_slot(QDropEvent*);

private:
    bool check_event(QEvent*);

    static void sens_set(QTbag*, bool, int);
    static int redraw_timeout(void*);
    static int motion_idle(void*);
    static void do_save_plot(const char*, void*);
    static void do_save_print(const char*, void*);
    static void set_hccb(HCcb*);
    static bool get_dim(const char*, double*);

    sGraph      *pb_graph;
    QGroupBox   *pb_gbox;                       // frame containing the buttons
    QPushButton *pb_checkwins[pbtn_NUMBTNS];    // button widget pointers
    int         pb_id;                          // motion idle id
    int         pb_x, pb_y;                     // motion coords
    int         pb_rdid;                        // redisplay timeout id
    bool        pb_event_test;
    bool        pb_event_deferred;
};

#endif


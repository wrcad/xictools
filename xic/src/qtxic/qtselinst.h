
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

#ifndef QTSELINST_H
#define QTSELINST_H

#include "main.h"
#include "qtmain.h"

#include <QDialog>


//-----------------------------------------------------------------------------
// QTselInstSelectDlg:  Dialog to allow the user to select/deselect cell
// instances.

class QLabel;

class QTcellInstSelectDlg : public QDialog, public QTbag
{
    Q_OBJECT

public:
    // List element for cell instances.
    //
    struct ci_item
    {
        ci_item()
            {
                cdesc = 0;
                name = 0;
                index = 0;
                sel = false;
            }

        CDc *cdesc;                         // instance desc
        const char *name;                   // master cell name
        int index;                          // instance index
        bool sel;                           // selection flag
    };

    QTcellInstSelectDlg(CDol*, bool = false);
    ~QTcellInstSelectDlg();

    QSize sizeHint() const;
    void update(CDol*);

    void set_transient_for(QWidget *prnt)
        {
            Qt::WindowFlags f = windowFlags();
            setParent(prnt);
#ifdef __APPLE__
            f |= Qt::Tool;
#endif
            setWindowFlags(f);
        }

    static CDol *instances()                    { return (ci_return); }
    static QTcellInstSelectDlg *self()          { return (instPtr); }

private slots:
    void sel_btn_slot();
    void desel_btn_slot();
    void mouse_press_slot(QMouseEvent*);
    void dismiss_btn_slot();
    void font_changed_slot(int);

private:
    void refresh();
    static void apply_selection(ci_item*);

    ci_item     *ci_list;           // list of cell instances
    QLabel      *ci_label;          // label widget
    int         ci_field;           // max cell name length
    bool        ci_filt;            // instance filtering mode

    static CDol *ci_return;
    static QTcellInstSelectDlg *instPtr;
};

#endif


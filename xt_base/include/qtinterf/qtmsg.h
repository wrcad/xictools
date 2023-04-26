
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

#ifndef MESSAGE_D_H
#define MESSAGE_D_H

#include "ginterf/graphics.h"

#include <QVariant>
#include <QDialog>

class QGroupBox;
class QPushButton;
class QTextEdit;

namespace qtinterf
{
    class QTbag;
    class text_box;

    class QTmsgPopup : public QDialog, public GRtextPopup
    {
        Q_OBJECT

    public:
        QTmsgPopup(QTbag*, const char*, STYtype style, int, int);
        ~QTmsgPopup();

        // GRpopup overrides
        void set_visible(bool visib)
            {
                if (visib) {
                    show();
                    raise();
                    activateWindow();
                }
                else
                    hide();
            }
        void popdown();

        // GRtextPopup overrides
        bool get_btn2_state() { return (false); }
        void set_btn2_state(bool) { }

        void setTitle(const char*);
        void setText(const char*);

        // This widget will be deleted when closed with the title bar "X"
        // button.  Qt::WA_DeleteOnClose does not work - our destructor is
        // not called.  The default behavior is to hide the widget instead
        // of deleting it, which would likely be a core leak here.
        void closeEvent(QCloseEvent*) { quit_slot(); }

    private slots:
        void quit_slot();

    private:
        QGroupBox *gbox;
        text_box *tx;
        QPushButton *b_cancel;
        STYtype display_style;
    };
}

#endif


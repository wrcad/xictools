
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

#ifndef QTMODIF_H
#define QTMODIF_H

#include "main.h"
#include "qtmain.h"
#include "editif.h"
#include <QDialog>


//-----------------------------------------------------------------------------
// QTmodifDlg:  Dialog to question the user about saving modified
// cells.

class QLabel;
class QMouseEvent;

class QTmodifDlg : public QDialog, public QTbag
{
    Q_OBJECT

public:

    // List element for modified cells.
    //
    struct s_item
    {
        s_item() { name = 0; path = 0; save = true; ft[0] = 0; }
        ~s_item() { delete [] name; delete [] path; }

        char *name;                         // cell name
        char *path;                         // full path name
        bool save;                          // save flag
        char ft[4];                         // file type code
    };

    QTmodifDlg(stringlist*, bool(*)(const char*));
    ~QTmodifDlg();

    QSize sizeHint() const;

    void set_transient_for(QWidget *prnt)
        {
            Qt::WindowFlags f = windowFlags();
            setParent(prnt);
#ifdef Q_OS_MACOS
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

    bool is_empty()                 { return (!m_field || !m_width); }
    static PMretType retval()       { return (m_retval); }
    static QTmodifDlg *self()       { return (instPtr); }

private slots:
    void save_all_slot();
    void skip_all_slot();
    void help_slot();
    void apply_slot();
    void abort_slot();
    void font_changed_slot(int);
    void mouse_press_slot(QMouseEvent*);

private:
    void refresh();

    s_item      *m_list;                // list of cells
    bool (*m_saveproc)(const char*);    // save callback
    int         m_field;                // max cell name length
    int         m_width;                // max total string length
    QLabel      *m_label;
    QTtextEdit  *m_text;

    static PMretType m_retval;          // return flag
    static QTmodifDlg *instPtr;
};

#endif


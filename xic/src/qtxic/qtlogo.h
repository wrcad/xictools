
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

#ifndef QTLOGO_H
#define QTLOGO_H

#include "main.h"
#include "qtmain.h"

#include <QDialog>


//-----------------------------------------------------------------------------
// QTlogoDlg:  Panel to control generation of physical text in layouts.

class QRadioButton;
class QCheckBox;
class QComboBox;
class QToolButton;
namespace qtinterf {
    class QTdoubleSpinBox;
}

class QTlogoDlg : public QDialog, public QTbag
{
    Q_OBJECT

public:
    QTlogoDlg(GRobject);
    ~QTlogoDlg();

#ifdef Q_OS_MACOS
    bool event(QEvent*);
#endif

    void update();
    void update_fontsel();
    static const char *font(bool init=false);

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

    QTfontDlg *fontsel()            { return (lgo_fontsel); }
    static QTlogoDlg *self()        { return (instPtr); }

private slots:
    void vector_btn_slot(bool);
    void manh_btn_slot(bool);
    void pretty_btn_slot(bool);
    void help_btn_slot();
    void pixel_btn_slot(int);
    void value_changed_slot(double);
    void endstyle_change_slot(int);
    void pwidth_change_slot(int);
    void create_btn_slot(int);
    void dump_btn_slot(bool);
    void sel_btn_slot(bool);
    void dismiss_btn_slot();

private:
    static int lgo_upd_label(void*);
    static ESret lgo_sav_cb(const char*, void*);
    static void lgo_cb(const char*, const char*, void*);

    GRobject    lgo_caller;
    QRadioButton *lgo_vector;
    QRadioButton *lgo_manh;
    QRadioButton *lgo_pretty;
    QCheckBox   *lgo_setpix;
    QComboBox   *lgo_endstyle;
    QComboBox   *lgo_pwidth;
    QCheckBox   *lgo_create;
    QToolButton *lgo_dump;
    QToolButton *lgo_sel;
    QTdoubleSpinBox *lgo_sb_pix;
    GRledPopup  *lgo_sav_pop;
    QTfontDlg   *lgo_fontsel;

    static const char *lgo_endstyles[];
    static const char *lgo_pathwidth[];
    static double lgo_defpixsz;
    static char *lgo_font;
    static QTlogoDlg *instPtr;
};

#endif


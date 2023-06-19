
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

#ifndef QTCHDSAVE_H
#define QTCHDSAVE_H

#include "main.h"
#include "qtmain.h"

#include <QDialog>

class QLabel;
class QLineEdit;
class QCheckBox;
class QPushButton;
class cLayerList;

class cCHDsave : public QDialog
{
    Q_OBJECT

public:
    cCHDsave(GRobject, bool(*)(const char*, bool, void*), void*, const char*);
    ~cCHDsave();

    void update(const char*);
    
    static cCHDsave *self()         { return (instPtr); }

private slots:
    void help_btn_slot();
    void text_changed_slot(const QString&);
    void geom_btn_slot(int);
    void apply_btn_slot();
    void dismiss_btn_slot();

private:
    /*
    void button_hdlr(GtkWidget*);

    static void cs_cancel_proc(GtkWidget*, void*);
    static void cs_action(GtkWidget*, void*);

    static void cs_change_proc(GtkWidget*, void*);
    static int cs_key_hdlr(GtkWidget*, GdkEvent*, void*);
    static void cs_drag_data_received(GtkWidget*, GdkDragContext*,
        gint, gint, GtkSelectionData*, guint, guint);
    */

    GRobject    cs_caller;
    QLabel      *cs_label;
    QLineEdit   *cs_text;
    QCheckBox   *cs_geom;
    QPushButton *cs_apply;
    cLayerList  *cs_llist;

    bool(*cs_callback)(const char*, bool, void*);
    void *cs_arg;

    static cCHDsave *instPtr;
};

#endif

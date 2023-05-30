
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


struct cCHDsave : public QDialog
{
    cCHDsave(GRobject, bool(*)(const char*, bool, void*), void*, const char*);
    ~cCHDsave();

    void update(const char*);
    
    cCHDsave *self()            return (instPtr); }

private:
    void button_hdlr(GtkWidget*);

    /*
    static void cs_cancel_proc(GtkWidget*, void*);
    static void cs_action(GtkWidget*, void*);

    static void cs_change_proc(GtkWidget*, void*);
    static int cs_key_hdlr(GtkWidget*, GdkEvent*, void*);
    static void cs_drag_data_received(GtkWidget*, GdkDragContext*,
        gint, gint, GtkSelectionData*, guint, guint);
    */

    GRobject cs_caller;
    GtkWidget *cs_popup;
    GtkWidget *cs_label;
    GtkWidget *cs_text;
    GtkWidget *cs_geom;
    GtkWidget *cs_apply;
    llist_t *cs_llist;

    bool(*cs_callback)(const char*, bool, void*);
    void *cs_arg;

    static cCHDsave *instPtr;
};

#endif


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

#ifndef QTCGDOPEN_H
#define QTCGDOPEN_H

#include "main.h"
#include "qtmain.h"

#include <QDialog>


class cCGDopen : public QDialog, public GTKbag
{
    Q_OBJECT

public:
    cCGDopen(GRobject, bool(*)(const char*, const char*, int, void*),
        void*, const char*, const char*);
    ~cCGDopen();

    void update(const char*, const char*);

    static cCGDopen *self()         { return (instPtr); }

private:
    void apply_proc(GtkWidget*);
    char *encode_remote_spec(GtkWidget**);
    void load_remote_spec(const char*);

    /*
    static void cgo_cancel_proc(GtkWidget*, void*);
    static void cgo_action(GtkWidget*, void*);

    static int cgo_key_hdlr(GtkWidget*, GdkEvent*, void*);
    static void cgo_info_proc(GtkWidget*, void*);
    static void cgo_drag_data_received(GtkWidget*, GdkDragContext*,
        gint, gint, GtkSelectionData*, guint, guint);
    static void cgo_page_proc(GtkNotebook*, void*, int, void*);
    */

    GRobject cgo_caller;
    GtkWidget *cgo_nbook;
    GtkWidget *cgo_p1_entry;
    GtkWidget *cgo_p2_entry;
    GtkWidget *cgo_p3_host;
    GtkWidget *cgo_p3_port;
    GtkWidget *cgo_p3_idname;
    GtkWidget *cgo_idname;
    GtkWidget *cgo_apply;

    llist_t *cgo_p1_llist;
    cnmap_t *cgo_p1_cnmap;

    bool(*cgo_callback)(const char*, const char*, int, void*);
    void *cgo_arg;

    static cCGDopen *instPtr;
};

#endif


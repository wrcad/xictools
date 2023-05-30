
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

#ifndef QTCHDOPEN_H
#define QTCHDOPEN_H

#include "main.h"
#include "qtmain.h"

#include <QDialog>

class cCHDopen : public GTKbag
{
    Q_OBJECT

public:
    cCHDopen(GRobject, bool(*)(const char*, const char*, int, void*), void*,
        const char*, const char*);
    ~cCHDopen();

    void update(const char*, const char*);

    cCHDopen *self()            { return (instPtr); }

private:
    void apply_proc(GtkWidget*);

    /*
    static void co_cancel_proc(GtkWidget*, void*);
    static void co_action(GtkWidget*, void*);

    static int co_key_hdlr(GtkWidget*, GdkEvent*, void*);
    static void co_info_proc(GtkWidget*, void*);
    static void co_drag_data_received(GtkWidget*, GdkDragContext*,
        gint, gint, GtkSelectionData*, guint, guint);
    static void co_page_proc(GtkNotebook*, void*, int, void*);
    */

    GRobject co_caller;
    GtkWidget *co_nbook;
    GtkWidget *co_p1_text;
    GtkWidget *co_p1_info;
    GtkWidget *co_p2_text;
    GtkWidget *co_p2_mem;
    GtkWidget *co_p2_file;
    GtkWidget *co_p2_none;
    GtkWidget *co_idname;
    GtkWidget *co_apply;

    cnmap_t *co_p1_cnmap;

    bool(*co_callback)(const char*, const char*, int, void*);
    void *co_arg;

    static cCHDopen *instPtr;
};

#endif


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

#ifndef QTCHDCFG_H
#define QTCHDCFG_H

#include "main.h"
#include "qtmain.h"

#include <QDialog>


class cCHDcfg : public QDialog, public GTKbag
{
    Q_OBJECT

public:
    cCHDcfg(GRobject, const char*);
    ~cCHDcfg();

    void update(const char*);

    static cCHDcfg *self()          { return (instPtr); }

private:
    void button_hdlr(GtkWidget*);

    /*
    static bool cf_new_cgd_cb(const char*, const char*, int, void*);
    static void cf_cancel_proc(GtkWidget*, void*);
    static void cf_action(GtkWidget*, void*);
    static void cf_drag_data_received(GtkWidget*, GdkDragContext*,
        gint, gint, GtkSelectionData*, guint, guint);
    */

    GRobject cf_caller;
    GtkWidget *cf_label;
    GtkWidget *cf_dtc_label;
    GtkWidget *cf_last;
    GtkWidget *cf_text;
    GtkWidget *cf_apply_tc;
    GtkWidget *cf_newcgd;
    GtkWidget *cf_cgdentry;
    GtkWidget *cf_cgdlabel;
    GtkWidget *cf_apply_cgd;

    char *cf_chdname;
    char *cf_lastname;
    char *cf_cgdname;

    static cCHDcfg *instPtr;
};

#endif


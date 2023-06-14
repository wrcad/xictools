
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

#ifndef QTFILES_H
#define QTFILES_H

#include "main.h"
#include "qtmain.h"
#include "qtinterf/qtpfiles.h"

#include <QDialog>

struct sPathList;

class cFilesList : public QDialog, public files_bag
{
//    Q_OBJECT

public:
    cFilesList(GRobject);
    ~cFilesList();

    void update();
    char *get_selection();

    static cFilesList *self()           { return (instPtr); }

private:
    /*
    void action_hdlr(GtkWidget*);
    bool button_hdlr(GtkWidget*, GdkEvent*);
    */
    bool show_content();
    void set_sensitive(const char*, bool);

    static sPathList *fl_listing(int);
    static char *fl_is_symfile(const char*);
    /*
    static void fl_action_proc(GtkWidget*, void*);
    static int fl_btn_proc(GtkWidget*, GdkEvent*, void*);
    static void fl_content_cb(const char*, void*);
    static void fl_down_cb(GtkWidget*, void*);
    static void fl_desel(void*);
    */

    GRobject fl_caller;
    char *fl_selection;
    char *fl_contlib;
    GRmcolPopup *fl_content_pop;
    cCHD *fl_chd;
    int fl_noupdate;

    static const char *nofiles_msg;
    static const char *files_msg;
    static cFilesList *instPtr;
};

#endif


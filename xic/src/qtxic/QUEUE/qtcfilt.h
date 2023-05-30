
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

#ifndef QTCFILT_H
#define QTCFILT_H

#include "main.h"
#include "gtkmain.h"

#include <QDialog>



class cCfilt : public QDialog
{
    Q_OBJECT

public
    cCfilt(GRobject, DisplayMode, void(*)(cfilter_t*, void*), void*);
    ~cCfilt();

    void update(DisplayMode);

    static cCfilt *self()           { return (instPtr); }

private:
    void setup(const cfilter_t*);
    cfilter_t *new_filter();
    /*
    static void cf_cancel_proc(GtkWidget*, void*);
    static void cf_action(GtkWidget*, void*);
    static void cf_radio(GtkWidget*, void*);
    static void cf_vlayer_menu_proc(GtkWidget*, void*);
    static void cf_name_menu_proc(GtkWidget*, void*);
    static void cf_sto_menu_proc(GtkWidget*, void*);
    static void cf_rcl_menu_proc(GtkWidget*, void*);
    */

    GRobject cf_caller;
    GtkWidget *cf_nimm;
    GtkWidget *cf_imm;
    GtkWidget *cf_nvsm;
    GtkWidget *cf_vsm;
    GtkWidget *cf_nlib;
    GtkWidget *cf_lib;
    GtkWidget *cf_npsm;
    GtkWidget *cf_psm;
    GtkWidget *cf_ndev;
    GtkWidget *cf_dev;
    GtkWidget *cf_nspr;
    GtkWidget *cf_spr;
    GtkWidget *cf_ntop;
    GtkWidget *cf_top;
    GtkWidget *cf_nmod;
    GtkWidget *cf_mod;
    GtkWidget *cf_nalt;
    GtkWidget *cf_alt;
    GtkWidget *cf_nref;
    GtkWidget *cf_ref;
    GtkWidget *cf_npcl;
    GtkWidget *cf_pcl;
    GtkWidget *cf_pclent;
    GtkWidget *cf_nscl;
    GtkWidget *cf_scl;
    GtkWidget *cf_sclent;
    GtkWidget *cf_nlyr;
    GtkWidget *cf_lyr;
    GtkWidget *cf_lyrent;
    GtkWidget *cf_nflg;
    GtkWidget *cf_flg;
    GtkWidget *cf_flgent;
    GtkWidget *cf_nftp;
    GtkWidget *cf_ftp;
    GtkWidget *cf_ftpent;
    GtkWidget *cf_apply;

    void(*cf_cb)(cfilter_t*, void*);
    void *cf_arg;
    DisplayMode cf_mode;

#define NUMREGS 6
    static char *cf_phys_regs[];
    static char *cf_elec_regs[];

    static cCfilt *instPtr;
};

#endif

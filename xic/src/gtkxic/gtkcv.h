
/*========================================================================*
 *                                                                        *
 *  Whiteley Research Inc, Sunnyvale, CA USA, http://wrcad.com            *
 *  Copyright (C) 2017 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *  Licensed under the Apache License, Version 2.0 (the "License");       *
 *  you may not use this file except in compliance with the License.      *
 *  You may obtain a copy of the License at                               *
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

#ifndef GTKCV_H
#define GTKCV_H

#include "gtkspinbtn.h"

struct fmt_menu
{
    const char *name;
    int code;
};

//-------------------------------------------------------------------------
// Subwidget group for cell name mapping
//
struct cnmap_t
{
    cnmap_t(bool);

    void update();
    void text_changed(GtkWidget*);
    void action(GtkWidget*);

    GtkWidget *frame() { return (cn_frame); }

private:
    static void cn_text_changed(GtkWidget*, void*);
    static void cn_action(GtkWidget*, void*);

    GtkWidget *cn_prefix;
    GtkWidget *cn_suffix;
    GtkWidget *cn_to_lower;
    GtkWidget *cn_to_upper;
    GtkWidget *cn_rd_alias;
    GtkWidget *cn_wr_alias;
    GtkWidget *cn_frame;
    bool cn_output;
};

//-------------------------------------------------------------------------
// Subwidget group for layer list
//
struct llist_t
{
    llist_t();
    ~llist_t();

    void update();
    void text_changed(GtkWidget*);
    void action(GtkWidget*);

    GtkWidget *frame() { return (ll_frame); }

private:
    static void ll_text_changed(GtkWidget*, void*);
    static void ll_action(GtkWidget*, void*);

    GtkWidget *ll_luse;
    GtkWidget *ll_lskip;
    GtkWidget *ll_laylist;
    GtkWidget *ll_aluse;
    GtkWidget *ll_aledit;
    GtkWidget *ll_frame;
};

//-------------------------------------------------------------------------
// Subwidget group for window control
//

// Sensitivity logic.
enum WndSensMode { WndSensAllModes, WndSensFlatten, WndSensNone };

struct wnd_t
{
    wnd_t(WndSensMode(*)(), bool);
    ~wnd_t();

    void update();
    void val_changed(GtkWidget*);
    void action(GtkWidget*);
    void set_sens();
    bool get_bb(BBox*);
    void set_bb(const BBox*);

    GtkWidget *frame() { return (wnd_frame); }

private:
    static void wnd_val_changed(GtkWidget*, void*);
    static void wnd_action(GtkWidget*, void*);
    static int wnd_popup_btn_proc(GtkWidget*, GdkEvent*, void*);
    static void wnd_sto_menu_proc(GtkWidget*, void*);
    static void wnd_rcl_menu_proc(GtkWidget*, void*);

    GtkWidget *wnd_use_win;
    GtkWidget *wnd_clip;
    GtkWidget *wnd_flatten;
    GtkWidget *wnd_ecf_label;
    GtkWidget *wnd_ecf_pre;
    GtkWidget *wnd_ecf_post;
    GtkWidget *wnd_l_label;
    GtkWidget *wnd_b_label;
    GtkWidget *wnd_r_label;
    GtkWidget *wnd_t_label;
    GtkWidget *wnd_frame;
    GtkWidget *wnd_sbutton;
    GtkWidget *wnd_rbutton;
    GtkWidget *wnd_s_menu;
    GtkWidget *wnd_r_menu;

    GTKspinBtn sb_left;
    GTKspinBtn sb_bottom;
    GTKspinBtn sb_right;
    GTKspinBtn sb_top;

    WndSensMode (*wnd_sens_test)();
    bool wnd_from_db;
};

//-------------------------------------------------------------------------
// Subwidget group for output format control
//

// Define to include support for CHD/CGD creation.  This support has
// been removed.
// #define FMT_WITH_DIGESTS

// configuration
enum cvofmt_mode
{
    cvofmt_file,
    cvofmt_native,
    cvofmt_chd,
    cvofmt_chdfile,
    cvofmt_asm,
    cvofmt_db
};

struct cvofmt_t
{
    cvofmt_t(void(*)(int), int, cvofmt_mode);
    ~cvofmt_t();

    void update();
    bool gds_text_input();
    void configure(cvofmt_mode);
    void set_page(int);

    GtkWidget *frame() { return (fmt_form); }

    // This is set from the OASIS-Advanced pop-up.
    static void set_rep_string(char *s)
        {
            delete [] fmt_oas_rep_string;
            fmt_oas_rep_string = s;
        }
    static char *rep_string()   { return (fmt_oas_rep_string); }

private:
    static void fmt_action(GtkWidget*, void*);
    static void fmt_flags_proc(GtkWidget*, void*);
    static void fmt_level_proc(GtkWidget*, void*);
    static void fmt_style_proc(GtkWidget*, void*);
    static void fmt_input_proc(GtkWidget*, void*);
    static void fmt_val_changed(GtkWidget*, void*);
    static void fmt_page_proc(GtkWidget*, GtkWidget*, int, void*);
    static void fmt_info_proc(GtkWidget*, void*);

    GtkWidget *fmt_form;
    GtkWidget *fmt_level;
    GtkWidget *fmt_gdsmap;
    GtkWidget *fmt_gdscut;
    GtkWidget *fmt_cgxcut;
    GtkWidget *fmt_oasmap;
    GtkWidget *fmt_oascmp;
    GtkWidget *fmt_oasrep;
    GtkWidget *fmt_oastab;
    GtkWidget *fmt_oassum;
    GtkWidget *fmt_oasoff;
    GtkWidget *fmt_oasnwp;
    GtkWidget *fmt_oasadv;
    GtkWidget *fmt_gdsftlab;
    GtkWidget *fmt_gdsftopt;
    GtkWidget *fmt_cifflags;
    GtkWidget *fmt_cifcname;
    GtkWidget *fmt_ciflayer;
    GtkWidget *fmt_ciflabel;
#ifdef FMT_WITH_DIGESTS
    GtkWidget *fmt_chdlabel;
    GtkWidget *fmt_chdinfo;
#endif
    void (*fmt_cb)(int);
    bool fmt_strip;

    GTKspinBtn sb_gdsunit;

    static int fmt_gds_inp;
    static char *fmt_oas_rep_string;
};

#endif


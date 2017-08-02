
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

#ifndef GTKASM_H
#define GTKASM_H

#include "gtkspinbtn.h"

enum ASMcode { ASM_INIT, ASM_INFO, ASM_READ, ASM_WRITE, ASM_CNAME };

// Scale spin button digits and size
#define ASM_NUMD 5
#define ASM_NFW 100

// Position spin button size
#define ASM_NFWP 100

// Token shown in Top-Level Cells listing for default cell.
#define ASM_TOPC "<default>"

struct sAsm;
struct sAsmPage;
struct cvofmt_t;

// Transformation parameters for instances.
//
struct tlinfo
{
    tlinfo(const char *cn)
        {
            magn = 1.0;
            scale = 1.0;
            cellname = lstring::copy(cn);
            placename = 0;
            x = y = 0;
            angle = 0;
            mirror_y = false;
            ecf_level = ECFnone;
            use_win = false;
            clip = false;
            flatten = false;
            no_hier = false;
        };

    ~tlinfo()
        {
            delete [] cellname;
            delete [] placename;
        }

    double magn;                    // instance magnification
    double scale;                   // cell definition scaling
    char *cellname;                 // name of cell to instantiate
    char *placename;                // alias for cellname
    int x, y;                       // translation x, y
    int angle;                      // rotation angle, multiple of 45
    BBox winBB;                     // window coords
    bool mirror_y;                  // reflection
    unsigned char ecf_level;        // empty cell filtering level
    bool use_win;                   // use window
    bool clip;                      // clip to window
    bool flatten;                   // flatten hierarchy
    bool no_hier;                   // don't read subcells
};

// Container for instance transform widgets, used in notebook pages.
//
struct sAsmTf
{
    sAsmTf(sAsmPage*);
    ~sAsmTf();

    GtkWidget *shell() { return (tf_top); }

    void set_sens(bool, bool);
    void reset();
    void get_tx_params(tlinfo*);
    void set_tx_params(tlinfo*);

private:
    static void tf_angle_proc(GtkWidget*, void*);
    static void tf_use_win_proc(GtkWidget*, void*);

    sAsmPage *tf_owner;             // container
    GtkWidget *tf_top;              // top-level container
    // "basic" page
    GtkWidget *tf_pxy_label;        // placement label
    GtkWidget *tf_angle;            // angle option menu 0-315 in 45 incr.
    GtkWidget *tf_ang_label;        // angle label
    GtkWidget *tf_mirror;           // reflection button
    GtkWidget *tf_mag_label;        // magnification label
    GtkWidget *tf_name_label;       // placement name label
    GtkWidget *tf_name;             // placement name
    // "advanced" page
    GtkWidget *tf_use_win;          // use window check box
    GtkWidget *tf_do_clip;          // clip check box
    GtkWidget *tf_do_flatn;         // flatten check box
    GtkWidget *tf_ecf_label;        // empty cell filtering label
    GtkWidget *tf_ecf_pre;          // empty cell pre-filtering
    GtkWidget *tf_ecf_post;         // empty cell post-filtering
    GtkWidget *tf_lb_label;         // l,b label
    GtkWidget *tf_rt_label;         // r,t label
    GtkWidget *tf_sc_label;         // scale factor label
    GtkWidget *tf_no_hier;          // no hier check box
    int tf_angle_ix;                // current angle selection index

    GTKspinBtn sb_placement_x;      // placement x spin button
    GTKspinBtn sb_placement_y;      // placement y spin button
    GTKspinBtn sb_magnification;    // magnification spin button
    GTKspinBtn sb_win_l;            // window left spin button
    GTKspinBtn sb_win_b;            // window bottom spin button
    GTKspinBtn sb_win_r;            // window right spin button
    GTKspinBtn sb_win_t;            // window top spin button
    GTKspinBtn sb_scale;            // scale spin button
};

// Container for notebook page widgets.
//
struct sAsmPage
{
    friend struct sAsm;

    sAsmPage(sAsm*);
    ~sAsmPage();

    void upd_sens();
    void reset();
    tlinfo *add_instance(const char*);

private:
    static void pg_selection_proc(GtkWidget*, GtkWidget*, void*);
    static void pg_unselection_proc(GtkWidget*, GtkWidget*, void*);

    static void pg_drag_data_received(GtkWidget*, GdkDragContext*,
        gint, gint, GtkSelectionData*, guint, guint, void*);

    sAsm *pg_owner;                 // container
    GtkWidget *pg_form;             // top-level container
    GtkWidget *pg_tablabel;         // tab label
    GtkWidget *pg_path;             // archive path entry
    GtkWidget *pg_layers_only;      // use layer list layers only button
    GtkWidget *pg_skip_layers;      // skip layer list layers button
    GtkWidget *pg_layer_list;       // layer list entry
    GtkWidget *pg_layer_aliases;    // layer aliases entry
    GtkWidget *pg_prefix;           // cell name modification prefix entry
    GtkWidget *pg_suffix;           // cell name modification suffix entry
    GtkWidget *pg_prefix_lab;       // prefix label
    GtkWidget *pg_suffix_lab;       // suffix label
    GtkWidget *pg_to_lower;         // cell name case change
    GtkWidget *pg_to_upper;         // cell name case change
    GtkWidget *pg_toplevels;        // scrolled list box cell instance list
    sAsmTf *pg_tx;                  // cell instance transform widgets
    tlinfo **pg_cellinfo;           // per instance transform info
    unsigned int pg_infosize;       // size of cellinfo
    unsigned int pg_numtlcells;     // number of instances
    int pg_curtlcell;               // index of selected instance, or -1
    GTKspinBtn sb_scale;            // scale spin button
};

// The main widget container.
//
struct sAsm : public gtk_bag
{
    friend void cConvert::PopUpAssemble(GRobject, ShowMode);

    sAsm(GRobject);
    ~sAsm();

    bool scanning()                 { return (asm_doing_scan); }
    void set_scanning(bool b)       { asm_doing_scan = b; }

    void update();
    bool run();
    void reset();
    bool dump_file(FILE*, bool);
    bool read_file(FILE*);

    void notebook_append();
    void notebook_remove(int);
    int current_page_index();
    void output_page_setup();
    void store_tx_params();
    void show_tx_params(unsigned int);
    const char *top_level_cell();
    static void set_status_message(const char*);
    static void pop_up_monitor(int, const char*, ASMcode);

    static void asm_drag_data_received(GtkWidget*, GdkDragContext*,
        gint, gint, GtkSelectionData*, guint, guint);

private:
    static void asm_cancel_proc(GtkWidget*, void*);
    static void asm_go_proc(GtkWidget*, void*);
    static void asm_page_change_proc(GtkWidget*, void*, int, void*);
    static void asm_action_proc(GtkWidget*, void*, unsigned int);
    static void asm_save_cb(const char*, void*);
    static void asm_recall_cb(const char*, void*);
    static void asm_fsel_open(const char*, void*);
    static void asm_fsel_cancel(GRfilePopup*, void*);
    static void asm_tladd_cb(const char*, void*);

    static int asm_timer_callback(void*);
    static void asm_setup_monitor(bool);
    static bool asm_do_run(const char*);

    GRobject asm_caller;                // calling button
    GtkItemFactory *asm_item_factory;   // menu item factory
    GtkWidget *asm_notebook;            // pages, one per source file
    GtkWidget *asm_outfile;             // output file name entry
    GtkWidget *asm_topcell;             // new top level cell name entry
    GtkWidget *asm_status;              // status label
    GRfilePopup *asm_fsel;              // file selection
    cvofmt_t *asm_fmt;                  // output fromat selection

    sAsmPage **asm_sources;             // notebook pages
    unsigned int asm_srcsize;           // size of sources
    unsigned int asm_pages;             // number of pages in use
    GRlistPopup *asm_listobj;           // layer list pop-up object
    int asm_timer_id;                   // status message timer id
    void **asm_refptr;
    bool asm_doing_scan;                // true while scanning
    bool asm_abort;                     // abort operation flag

public:
    static GtkTargetEntry target_table[];
    static const char *path_to_source_string;
    static const char *path_to_new_string;
    static guint n_targets;
    static int asm_fmt_type;
};

// Progrss monitor pop-up.
//
struct sAsmPrg
{
    sAsmPrg();
    ~sAsmPrg();

    void update(const char*, ASMcode);

    void set_refptr(void **ptr)     { prg_refptr = ptr; }
    bool aborted()                  { return (prg_abort); }
    GtkWidget *shell()              { return (prg_shell); }

private:
    static void prg_cancel_proc(GtkWidget*, void*);
    static void prg_abort_proc(GtkWidget*, void*);

    GtkWidget *prg_shell;
    GtkWidget *prg_inp_label;
    GtkWidget *prg_out_label;
    GtkWidget *prg_info_label;
    GtkWidget *prg_cname_label;
    GtkWidget *prg_pbar;
    void **prg_refptr;
    bool prg_abort;                     // abort operation flag
};

#endif


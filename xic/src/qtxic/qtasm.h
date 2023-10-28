
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

#ifndef QTASM_H
#define QTASM_H

#include "main.h"
#include "fio.h"
#include "qtmain.h"

#include <QDialog>
#include <QTabWidget>


//-----------------------------------------------------------------------------
// Pop-up to merge layout sources into a single file.
//

enum ASMcode { ASM_INIT, ASM_INFO, ASM_READ, ASM_WRITE, ASM_CNAME };

// Scale spin button digits and size
#define ASM_NUMD 5
#define ASM_NFW 100

// Position spin button size
#define ASM_NFWP 100

// Token shown in Top-Level Cells listing for default cell.
#define ASM_TOPC "<default>"

class QTasmDlg;
class QTasmPage;
class QTconvOutFmt;
namespace qtinterf { class QTactivity; }

class QLabel;
class QDoubleSpinBox;
class QCheckBox;
class QTreeWidget;


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
class QTasmTf : public QTabWidget
{
    Q_OBJECT

public:
    QTasmTf(QTasmPage*);
    ~QTasmTf();

    void set_sens(bool, bool);
    void reset();
    void get_tx_params(tlinfo*);
    void set_tx_params(tlinfo*);

private slots:
    void angle_changed_slot(int);
    void usew_btn_slot(int);

private:
    QTasmPage   *tf_owner;              // container
    // "basic" page
    QLabel      *tf_pxy_label;          // placement label
    QDoubleSpinBox *tf_sb_placement_x;  // placement x spin button
    QDoubleSpinBox *tf_sb_placement_y;  // placement y spin button
    QLabel      *tf_ang_label;          // angle label
    QComboBox   *tf_angle;              // angle option menu 0-315 in 45 incr.
    QCheckBox   *tf_mirror;             // reflection button
    QLabel      *tf_mag_label;          // magnification label
    QDoubleSpinBox *tf_sb_magnification;// magnification spin button
    QLabel      *tf_name_label;         // placement name label
    QLineEdit   *tf_name;               // placement name
    // "advanced" page
    QCheckBox   *tf_use_win;            // use window check box
    QCheckBox   *tf_do_clip;            // clip check box
    QCheckBox   *tf_do_flatn;           // flatten check box
    QLabel      *tf_ecf_label;          // empty cell filtering label
    QCheckBox   *tf_ecf_pre;            // empty cell pre-filtering
    QCheckBox   *tf_ecf_post;           // empty cell post-filtering
    QLabel      *tf_lb_label;           // l,b label
    QDoubleSpinBox *tf_sb_win_l;        // window left spin button
    QDoubleSpinBox *tf_sb_win_b;        // window bottom spin button
    QLabel      *tf_rt_label;           // r,t label
    QDoubleSpinBox *tf_sb_win_r;        // window right spin button
    QDoubleSpinBox *tf_sb_win_t;        // window top spin button
    QLabel      *tf_sc_label;           // scale factor label
    QDoubleSpinBox *tf_sb_scale;        // scale spin button
    QCheckBox   *tf_no_hier;            // no hier check box
    int         tf_angle_ix;            // current angle selection index
};

// Container for notebook page widgets.
//
class QTasmPage : public QWidget
{
    Q_OBJECT

public:
    friend class QTasmDlg;

    QTasmPage(QTasmDlg*);
    ~QTasmPage();

    void upd_sens();
    void reset();
    tlinfo *add_instance(const char*);

private slots:
    void toplev_selection_changed_slot();
    void font_changed_slot(int);

private:
    QTasmDlg    *pg_owner;          // container
    QLineEdit   *pg_path;           // archive path entry
    QCheckBox   *pg_layers_only;    // use layer list layers only button
    QCheckBox   *pg_skip_layers;    // skip layer list layers button
    QLineEdit   *pg_layer_list;     // layer list entry
    QLineEdit   *pg_layer_aliases;  // layer aliases entry
    QDoubleSpinBox *pg_sb_scale;    // scale spin button
    QLineEdit   *pg_prefix;         // cell name modification prefix entry
    QLineEdit   *pg_suffix;         // cell name modification suffix entry
    QLabel      *pg_prefix_lab;     // prefix label
    QLabel      *pg_suffix_lab;     // suffix label
    QCheckBox   *pg_to_lower;       // cell name case change
    QCheckBox   *pg_to_upper;       // cell name case change
    QTreeWidget *pg_toplevels;      // scrolled list box cell instance list
    QTasmTf     *pg_tx;             // cell instance transform widgets
    tlinfo      **pg_cellinfo;      // per instance transform info
    unsigned int pg_infosize;       // size of cellinfo
    unsigned int pg_numtlcells;     // number of instances
    int         pg_curtlcell;       // index of selected instance, or -1
};

// The main widget container.
//
class QTasmDlg : public QDialog, public QTbag
{
    Q_OBJECT

public:
    // Main menu function codes.
    enum
    {
        NoCode,
        CancelCode,
        OpenCode,
        SaveCode,
        RecallCode,
        ResetCode,
        NewCode,
        DelCode,
        NewTlCode,
        DelTlCode,
        HelpCode
    };

    QTasmDlg(GRobject);
    ~QTasmDlg();

    void set_transient_for(QWidget *prnt)
    {
        Qt::WindowFlags f = windowFlags();
        setParent(prnt);
        setWindowFlags(f);
    }

    static QTasmDlg *self()         { return (instPtr); }
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

private slots:
    void main_menu_slot(QAction*);
    void tab_changed_slot(int);
    void crlayout_btn_slot();
    void dismiss_btn_slot();
    void help_slot();

private:
    static void asm_save_cb(const char*, void*);
    static void asm_recall_cb(const char*, void*);
    static void asm_fsel_open(const char*, void*);
    static void asm_fsel_cancel(GRfilePopup*, void*);
    static void asm_tladd_cb(const char*, void*);
    static int asm_timer_callback(void*);
    static void asm_setup_monitor(bool);
    static bool asm_do_run(const char*);

    GRobject    asm_caller;             // calling button
    QAction     *asm_filesel_btn;       // file select menu button
    QTabWidget  *asm_notebook;          // pages, one per source file
    QLineEdit   *asm_outfile;           // output file name entry
    QLineEdit   *asm_topcell;           // new top level cell name entry
    QLabel      *asm_status;            // status label
    GRfilePopup *asm_fsel;              // file selection
    QTconvOutFmt *asm_fmt;              // output fromat selection

    QTasmPage   **asm_sources;          // notebook pages
    unsigned int asm_srcsize;           // size of sources
    unsigned int asm_pages;             // number of pages in use
    GRlistPopup *asm_listobj;           // layer list pop-up object
    int         asm_timer_id;           // status message timer id
    void        **asm_refptr;
    bool        asm_doing_scan;         // true while scanning
    bool        asm_abort;              // abort operation flag

    static QTasmDlg *instPtr;

public:
    static const char *path_to_source_string;
    static const char *path_to_new_string;
    static int asm_fmt_type;
};

// Progress monitor pop-up.
//
class QTasmPrgDlg : public QDialog
{
    Q_OBJECT

public:
    QTasmPrgDlg();
    ~QTasmPrgDlg();

    void update(const char*, ASMcode);

    void set_refptr(void **ptr)     { prg_refptr = ptr; }
    bool aborted()                  { return (prg_abort); }
    static QTasmPrgDlg *self()      { return (instPtr); }

private slots:
    void abort_btn_slot();
    void dismiss_btn_slot();

private:
    QLabel  *prg_inp_label;
    QLabel  *prg_out_label;
    QLabel  *prg_info_label;
    QLabel  *prg_cname_label;
    QTactivity *prg_pbar;
    void    **prg_refptr;
    bool    prg_abort;              // abort operation flag

    static QTasmPrgDlg *instPtr;
};

#endif


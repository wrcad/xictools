
/*========================================================================
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
 * Qt MOZY help viewer.
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#ifndef QTHELP_H
#define QTHELP_H

#include "qtinterf/qtinterf.h"
#include "help/help_defs.h"
#include "help/help_context.h"
#include "htm/htm_widget.h"

#include <QVariant>
#include <QDialog>
#include <QKeyEvent>


// This implements a help/html viewer dialog, providing menus and
// interfaces to the QTviewer viewing area.  This derives from QDialog
// and the HelpWidget and QTbag interfaces.

class QLabel;
class QMenu;
class QMenuBar;
class QResizeEvent;
class QTmozyClrDlg;
namespace qtinterf {
    class QTsearchDlg;
}

struct htmAnchorCallbackStruct;
struct htmFormCallbackStruct;
struct htmFrameCallbackStruct;
struct htmImageInfo;
struct htmPLCStream;
struct htmRect;
struct htm_if;
struct HLPtopic;
struct HLPparams;
class Transaction;

namespace qtinterf {
    class QTviewer;
    class QThelpDlg;
}

#define HLP_DEF_WIDTH 730
#define HLP_DEF_HEIGHT 400

class qtinterf::QThelpDlg : public QDialog, public HelpWidget,
    public htmDataInterface, public QTbag
{
    Q_OBJECT

public:
    // Return from newtopic().
    enum NTtype { NTnone, NTnew, NThandled };

    QThelpDlg(bool, QWidget*);
    ~QThelpDlg();

    void set_transient_for(QWidget *prnt)
        {
            setParent(prnt);
        }

    // Don't pop down from Esc press.
    void keyPressEvent(QKeyEvent *ev)
        {
            if (ev->key() != Qt::Key_Escape)
                QDialog::keyPressEvent(ev);
        }

    void menu_sens_set(bool);

    // ViewWidget and HelpWidget interface
    void freeze();
    void thaw();
    void set_transaction(Transaction*, const char*);
    Transaction *get_transaction();
    bool check_halt_processing(bool);
    void set_halt_proc_sens(bool);
    void set_status_line(const char*);
    htmImageInfo *new_image_info(const char*, bool);
    bool call_plc(const char*);
    htmImageInfo *image_procedure(const char*);
    void image_replace(htmImageInfo*, htmImageInfo*);
    bool is_body_image(const char*);
    const char *get_url();
    bool no_url_cache();
    int image_load_mode();
    int image_debug_mode();
    GRwbag *get_widget_bag();
    void link_new(HLPtopic*);
    void reuse(HLPtopic*, bool);
    void redisplay();
    HLPtopic *get_topic();
    void unset_halt_flag();
    void halt_images();
    void show_cache(int);

    // htmDataInterface functions
    void emit_signal(SignalID, void*);
    void *event_proc(const char*);
    void panic_callback(const char*);
    htmImageInfo *image_resolve(const char*);
    int get_image_data(htmPLCStream*, void*);
    void end_image_data(htmPLCStream*, void*, int, bool);  
    void frame_rendering_area(htmRect*);
    const char *get_frame_name();
    void get_topic_keys(char**, char**);
    void scroll_visible(int, int, int, int);

    // QTbag functions
    char *GetPostscriptText(int, const char*, const char*, bool, bool);
    char *GetPlainText();
    char *GetHtmlText();

    // misc. functions
    int get_scroll_pos(bool = false);
    void set_scroll_pos(int, bool = false);

    QSize sizeHint() const
        { return (QSize(HLP_DEF_WIDTH, HLP_DEF_HEIGHT)); }
    QSize minimumSizeHint() const { return (QSize(260, 175)); }

    QTviewer *viewer()      { return (h_viewer); }

private slots:
    void backward_slot();
    void forward_slot();
    void stop_slot();
    void open_slot();
    void open_file_slot();
    void save_slot();
    void print_slot();
    void reload_slot();
    void make_fifo_slot(bool);
    void quit_slot();
    void config_slot();
    void proxy_slot();
    void search_slot();
    void find_text_slot();
    void colors_slot(bool);
    void set_font_slot(bool);
    void dont_cache_slot(bool);
    void clear_cache_slot();
    void reload_cache_slot();
    void show_cache_slot();
    void no_cookies_slot(bool);
    void no_images_slot(bool);
    void sync_images_slot(bool);
    void delayed_images_slot(bool);
    void progressive_images_slot(bool);
    void anchor_plain_slot(bool);
    void anchor_buttons_slot(bool);
    void anchor_underline_slot(bool);
    void anchor_highlight_slot(bool);
    void bad_html_slot(bool);
    void freeze_animations_slot(bool);
    void log_transactions_slot(bool);
    void add_slot();
    void delete_slot();
    void bookmark_slot(QAction*);
    void help_slot();
    void cache_choice_slot(const char*);

    void do_open_slot(const char*, void*);
    void do_save_slot(const char*, void*);
    void do_search_slot(const char*, void*);
    void search_down_slot();
    void search_up_slot();
    void ignore_case_slot(bool);

private:
    void set_frame_parent(QThelpDlg *p) { h_frame_parent = p; }
    void set_frame_name(const char *n) { h_frame_name = strdup(n); }

    void htm_activate_proc(htmAnchorCallbackStruct*);
    void htm_frame_proc(htmFrameCallbackStruct*);
    NTtype newtopic(const char*, bool, bool, bool);
    NTtype newtopic(const char*, FILE*, bool);
    void stop_image_download();
    static void h_proxy_proc(const char*, void*);
    bool register_fifo(const char*);
    void unregister_fifo();
#ifdef WIN32
    static void pipe_thread_proc(void*);
#endif
    static int fifo_check_proc(void*);

    QTviewer    *h_viewer;

    QMenu       *h_main_menus[4];
    QLabel      *h_status_bar;

    HLPparams   *h_params;          // default parameters
    HLPtopic    *h_root_topic;      // root (original) topic
    HLPtopic    *h_cur_topic;       // current topic
    bool        h_stop_btn_pressed; // stop download flag
    bool        h_is_frame;         // true for frames
    const char  *h_fifo_name;       // FIFO name
    int         h_fifo_fd;          // FIFO file id
    int         h_fifo_tid;         // FIFO poll timer id
#ifdef WIN32
    void        *h_fifo_pipe;       // pipe to FIFO for MSW
    stringlist  *h_fifo_tfiles;     // list of temp files
#endif
    GRlistPopup *h_cache_list;      // hook for pop-up cache list
    QTmozyClrDlg *h_clrdlg;         // hook for colors listing

    QThelpDlg   **h_frame_array;    // array of frame children
    int         h_frame_array_size;
    bool        h_ign_case;
    QThelpDlg   *h_frame_parent;    // pointer to frame parent
    char        *h_frame_name;      // frame name if frame

    QTsearchDlg *h_searcher;
    char        *h_last_search;

    // menu actions
    QAction     *h_Backward;
    QAction     *h_Forward;
    QAction     *h_Stop;
    QAction     *h_Open;
    QAction     *h_OpenFile;
    QAction     *h_Save;
    QAction     *h_Print;
    QAction     *h_Reload;
    QAction     *h_MkFIFO;
    QAction     *h_Quit;
    QAction     *h_Config;
    QAction     *h_Proxy;
    QAction     *h_Search;
    QAction     *h_FindText;
    QAction     *h_DefaultColors;
    QAction     *h_SetFont;
    QAction     *h_DontCache;
    QAction     *h_ClearCache;
    QAction     *h_ReloadCache;
    QAction     *h_ShowCache;
    QAction     *h_NoCookies;
    QAction     *h_NoImages;
    QAction     *h_SyncImages;
    QAction     *h_DelayedImages;
    QAction     *h_ProgressiveImages;
    QAction     *h_AnchorPlain;
    QAction     *h_AnchorButtons;
    QAction     *h_AnchorUnderline;
    QAction     *h_AnchorHighlight;
    QAction     *h_BadHTML;
    QAction     *h_FreezeAnimations;
    QAction     *h_LogTransactions;
    QAction     *h_AddBookmark;
    QAction     *h_DeleteBookmark;
    QAction     *h_Help;
};

#endif


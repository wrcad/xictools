
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

#ifndef HTMLVIEW_D_H
#define HTMLVIEW_D_H

#include <QVariant>
//#include <QWidget>
#include <QMainWindow>
#include "qtinterf/qtinterf.h"

#include "help/help_defs.h"
#include "help/help_context.h"
//#include "help/help_topic.h"
#include "htm/htm_widget.h"

// This implements a help/html viewer dialog, providing menus and
// interfaces to the viewer_wv iewing area.  This derives from QDialog
// and the HelpWidget and QTbag interfaces.

class QStatusBar;
class QMenu;
class QMenuBar;
class QResizeEvent;

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

namespace qtinterf
{
    class viewer_w;

    class QThelpPopup : public QMainWindow, public HelpWidget,
        public htmDataInterface, public QTbag
    {
        Q_OBJECT

    public:
        QThelpPopup(bool, QWidget*);
        ~QThelpPopup();
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

        QSize sizeHint() const { return (QSize(520, 350)); }
        QSize minimumSizeHint() const { return (QSize(260, 175)); }

    private slots:
        void backward_slot();
        void forward_slot();
        void stop_slot();
        void open_slot();
        void open_file_slot();
        void save_slot();
        void print_slot();
        void reload_slot();
        void quit_slot();
        void search_slot();
        void find_slot();
        void set_font_slot(bool);
        void font_selected_slot(int, const char*);
        void font_down_slot();
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

        void anchor_track_slot(htmAnchorCallbackStruct*);
        void newtopic_slot(htmAnchorCallbackStruct*);
        void form_slot(htmFormCallbackStruct*);
        void frame_slot(htmFrameCallbackStruct*);

        void do_open_slot(const char*, void*);
        void do_save_slot(const char*, void*);
        void do_search_slot(const char*, void*);
        void do_find_text_slot(const char*, void*);

    signals:
        void dismiss();

    private:
        void set_frame_parent(QThelpPopup *p) { frame_parent = p; }
        void set_frame_name(const char *n) { frame_name = strdup(n); }

        void newtopic(const char*, bool, bool, bool);
        void stop_image_download();

        viewer_w *html_viewer;

        QMenuBar *menubar;
        QMenu *main_menus[4];
        QStatusBar *status_bar;

        // menu actions
        QAction *a_Backward;
        QAction *a_Forward;
        QAction *a_Stop;
        QAction *a_Open;
        QAction *a_OpenFile;
        QAction *a_Save;
        QAction *a_Print;
        QAction *a_Reload;
        QAction *a_Quit;
        QAction *a_Search;
        QAction *a_FindText;
        QAction *a_SetFont;
        QAction *a_DontCache;
        QAction *a_ClearCache;
        QAction *a_ReloadCache;
        QAction *a_ShowCache;
        QAction *a_NoCookies;
        QAction *a_NoImages;
        QAction *a_SyncImages;
        QAction *a_DelayedImages;
        QAction *a_ProgressiveImages;
        QAction *a_AnchorPlain;
        QAction *a_AnchorButtons;
        QAction *a_AnchorUnderline;
        QAction *a_AnchorHighlight;
        QAction *a_BadHTML;
        QAction *a_FreezeAnimations;
        QAction *a_LogTransactions;
        QAction *a_AddBookmark;
        QAction *a_DeleteBookmark;
        QAction *a_Help;

        HLPparams *params;          // default parameters
        HLPtopic *root_topic;       // root (original) topic
        HLPtopic *cur_topic;        // current topic
        bool stop_btn_pressed;      // stop download flag
        bool is_frame;              // true for frames
        GRlistPopup *cache_list;    // hook for pop-up cache list

        QThelpPopup **frame_array;  // array of frame children
        int frame_array_size;
        QThelpPopup *frame_parent;  // pointer to frame parent
        char *frame_name;           // frame name if frame
    };
}

#endif

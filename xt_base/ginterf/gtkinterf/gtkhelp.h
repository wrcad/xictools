
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2005 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *                                                                        *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,      *
 *   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES      *
 *   OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-        *
 *   INFRINGEMENT.  IN NO EVENT SHALL STEPHEN R. WHITELEY BE LIABLE       *
 *   FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION      *
 *   OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN           *
 *   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN         *
 *   THE SOFTWARE.                                                        *
 *                                                                        *
 * This library is available for non-commercial use under the terms of    *
 * the GNU Library General Public License as published by the Free        *
 * Software Foundation; either version 2 of the License, or (at your      *
 * option) any later version.                                             *
 *                                                                        *
 * A commercial license is available to those wishing to use this         *
 * library or a derivative work for commercial purposes.  Contact         *
 * Whiteley Research Inc., 456 Flora Vista Ave., Sunnyvale, CA 94086.     *
 * This provision will be enforced to the fullest extent possible under   *
 * law.                                                                   *
 *                                                                        *
 * This library is distributed in the hope that it will be useful,        *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU      *
 * Library General Public License for more details.                       *
 *                                                                        *
 * You should have received a copy of the GNU Library General Public      *
 * License along with this library; if not, write to the Free             *
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.     *
 *========================================================================*
 *                                                                        *
 * GtkInterf Graphical Interface Library                                  *
 *                                                                        *
 *========================================================================*
 $Id: gtkhelp.h,v 2.26 2014/03/17 05:35:36 stevew Exp $
 *========================================================================*/

#ifndef GTKHELP_H
#define GTKHELP_H

#include "gtklist.h"
#include "gtksearch.h"
#include "help/help_defs.h"
#include "help/help_context.h"
#include "htm/htm_widget.h"

//
// The pop-up HTML help viewer/url browser.
//

struct htmAnchorCallbackStruct;
struct htmAnchorCallbackStruct;
struct htmFrameCallbackStruct;
struct htmFormCallbackStruct;

namespace gtkinterf {
    class gtk_viewer;

    // Basic graphical data subclass
    //
    struct GTKhelpPopup : public HelpWidget, public htmDataInterface,
        public gtk_bag
    {
        // Return from newtopic().
        enum NTtype { NTnone, NTnew, NThandled };

        GTKhelpPopup(bool, int, int, GtkWidget*);
        ~GTKhelpPopup();

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
        void reuse_display();
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

        gtk_viewer *viewer() { return (h_viewer); }

        // gtk_bag functions
        char *GetPostscriptText(int, const char*, const char*, bool, bool);
        char *GetPlainText();
        char *GetHtmlText();

        bool register_fifo(const char*);
        void unregister_fifo();

        // This can be set to provide a default transient-for window.
        static void set_transient_for(GtkWindow *win)
            {
                h_transient_for = win;
            }

        static GtkWindow *transient_for()       { return (h_transient_for); }

    private:
#ifdef WIN32
        static void pipe_thread_proc(void*);
#endif
        static int fifo_check_proc(void*);

        // misc. functions
        NTtype newtopic(const char*, bool, bool, bool);
        NTtype newtopic(const char*, FILE*, bool);
        void set_defaults();
        void stop_image_download();
        int scroll_position(bool);
        void set_scroll_position(int, bool);
        void activate_signal_handler(htmAnchorCallbackStruct*);
        void anchor_track_signal_handler(htmAnchorCallbackStruct*);
        void frame_signal_handler(htmFrameCallbackStruct*);
        void form_signal_handler(htmFormCallbackStruct*);

        void set_frame_parent(GTKhelpPopup *p) { h_frame_parent = p; }
        void set_frame_name(const char *n) { h_frame_name = strdup(n); }

        // GTK signal handlers
        static void h_drag_data_received(GtkWidget*, GdkDragContext*,
            gint, gint, GtkSelectionData*, guint, guint, void*);
        static void h_back_proc(GtkWidget*, void*);
        static void h_forw_proc(GtkWidget*, void*);
        static void h_stop_proc(GtkWidget*, void*);
        static void h_fontsel(gtk_bag*, GtkWidget*);
        static void h_menu_hdlr(GtkWidget*, void*, unsigned);
        static void h_bm_handler(GtkWidget*, void*);
        static void h_cancel_proc(GtkWidget*, void*);
        static int h_destroy_hdlr(GtkWidget*, GdkEvent*, void*);

        // dialog callbacks
        static void h_list_cb(const char*, void*);
        static void h_open_cb(const char*, void*);
        static void h_do_search_proc(const char*, void*);
        static bool h_find_text_proc(const char*, bool, bool, void*);
        static void h_do_save_proc(const char*, void*);

        // misc
        static void h_sens_set(gtk_bag*, bool, int);
        static void h_font_cb(const char*, const char*, void*);
        static void h_bm_dest(void*);
        static int h_ntop_timeout(void*);

        GtkWidget *h_parent;            // transient for...
        GtkItemFactory *h_item_factory; // menu item factory
        gtk_viewer *h_viewer;           // viewer class
        HLPparams *h_params;            // default parameters
        HLPtopic *h_root_topic;         // root (original) topic
        HLPtopic *h_cur_topic;          // current topic
        bool h_stop_btn_pressed;        // stop download flag
        bool h_is_frame;                // true for frames
        GRlistPopup *h_cache_list;      // hook for pop-up cache list
        GRfilePopup *h_fsels[4];        // file selection widgets
        GTKsearchPopup *h_find_text;

        GTKhelpPopup **h_frame_array;   // array of frame children
        int h_frame_array_size;
        GTKhelpPopup *h_frame_parent;   // pointer to frame parent
        char *h_frame_name;             // frame name if frame

        const char *h_fifo_name;        // pipe name
        int h_fifo_fd;                  // pipe fd
        int h_fifo_tid;                 // poll timer id
#ifdef WIN32
        void *h_fifo_pipe;              // pipe to fifo for MSW
        stringlist *h_fifo_tfiles;      // list of temp files
#endif

        static GtkWindow *h_transient_for;
        static GtkItemFactoryEntry h_help_menu_items[];
    };
}

#endif


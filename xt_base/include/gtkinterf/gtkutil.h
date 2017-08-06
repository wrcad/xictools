
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
 * GtkInterf Graphical Interface Library                                  *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#ifndef GTKUTIL_H
#define GTKUTIL_H


//
//  Utility Widgets
//

struct gtkDataInterface;

namespace gtkinterf {
    // Supplemental base class for the popup widget classes.
    //
    // Have to be careful about popdown.  It is possible for the
    // application to call the popdown callback twice, which is bad
    // news.  Problems are avoided by checking for the existence
    // of the widget in the container's GRmonList at the top of
    // GRxxx-visible functions.
    //
    struct base_w
    {
        base_w()
            {
                pw_shell = 0;
                pw_cancel = 0;
            }

        GtkWidget *pw_shell;        // popup shell
        GtkWidget *pw_cancel;       // cancel button
    };

    // Simple yes/no popup.
    //
    struct GTKaffirmPopup : public GRaffirmPopup, public base_w
    {
        GTKaffirmPopup(gtk_bag*, const char*, void*);
        ~GTKaffirmPopup();

        // GRpopup overrides
        void set_visible(bool visib)
            {
                if (visib)
                    gtk_widget_show(pw_shell);
                else
                    gtk_widget_hide(pw_shell);
            }
        void register_caller(GRobject, bool = false, bool = false);
        void popdown();

    private:
        // GTK signal handlers
        static int pw_affirm_key(GtkWidget*, GdkEvent*, void*);
        static void pw_affirm_button(GtkWidget*, void*);
        static void pw_affirm_popdown(GtkWidget*, void*);
        static int pw_attach_idle_proc(void*);

        GtkWidget *pw_yes;
        GtkWidget *pw_label;
        bool pw_affirmed;
    };

    // Numerical entry.
    struct GTKnumPopup : GRnumPopup, public base_w
    {
        GTKnumPopup(gtk_bag*, const char*, double, double, double,
            double, int, void*);
        ~GTKnumPopup();

        // GRpopup overrides
        void set_visible(bool visib)
            {
                if (visib)
                    gtk_widget_show(pw_shell);
                else
                    gtk_widget_hide(pw_shell);
            }
        void register_caller(GRobject, bool = false, bool = false);
        void popdown();

    private:
        // GTK signal handlers
        static void pw_numer_val_changed(GtkWidget*, void*);
        static int pw_numer_key_hdlr(GtkWidget*, GdkEvent*, void*);
        static void pw_numer_button(GtkWidget*, void*);
        static void pw_numer_popdown(GtkWidget*, void*);
        static int pw_attach_idle_proc(void*);

        GtkWidget *pw_yes;
        GtkWidget *pw_label;
        GtkWidget *pw_text;
        double pw_value, pw_tmp_value;
        double pw_mind, pw_maxd;
        bool pw_setentr;
        bool pw_affirmed;
    };

    // Line editor.
    //
    struct GTKledPopup : public GRledPopup, public base_w
    {
        GTKledPopup(gtk_bag*, const char*, const char*, int, bool,
            const char*, void*);
        ~GTKledPopup();

        // GRpopup overrides
        void set_visible(bool visib)
            {
                if (visib)
                    gtk_widget_show(pw_shell);
                else
                    gtk_widget_hide(pw_shell);
            }
        void register_caller(GRobject, bool = false, bool = false);
        void popdown();

        // GRledPopup overrides
        void update(const char*, const char*);

        void set_ignore_return()    { pw_ign_ret = true; }
        void set_no_drops(bool set) { pw_no_drops = set; }
        bool no_drops()             { return (pw_no_drops); }

    private:
        void button_hdlr(GtkWidget*);
        // GTK signal handlers
        static int pw_editstr_key(GtkWidget*, GdkEvent*, void*);
        static void pw_editstr_button(GtkWidget*, void*);
        static void pw_editstr_popdown(GtkWidget*, void*);
        static void pw_editstr_drag_data_received(GtkWidget*, GdkDragContext*,
            gint, gint, GtkSelectionData*, guint, guint);
        static int pw_attach_idle_proc(void*);

        GtkWidget *pw_yes;
        GtkWidget *pw_label;
        GtkWidget *pw_text;
        bool pw_applied;            // apply button pressed
        bool pw_ign_ret;            // PopUpInput mode
        bool pw_no_drops;           // don't accept drops
    };

    // Simple message box.
    //
    struct GTKmsgPopup : public GRmsgPopup, public base_w
    {
        GTKmsgPopup(gtk_bag*, const char*, bool);
        ~GTKmsgPopup();

        // GRpopup overrides
        void set_visible(bool visib)
            {
                if (visib)
                    gtk_widget_show(pw_shell);
                else
                    gtk_widget_hide(pw_shell);
            }
        void popdown();

        void set_desens()   { pw_desens = true; }
        bool is_desens()    { return (pw_desens); }

    private:
        // GTK signal handlers
        static void pw_message_popdown(GtkWidget*, void*);
        static int pw_message_popdown_ev(GtkWidget*, GdkEvent*, void*);

        GtkWidget *pw_label;
        bool pw_desens;         // if true, parent->input desensitized
    };

    // Fancy message box types.
    enum {PuWarn, PuErr, PuErrAlso, PuInfo, PuInfo2, PuHTML};

    class gtk_viewer;

    // Fancy message box.
    struct GTKtextPopup : public GRtextPopup, public base_w
    {
        GTKtextPopup(gtk_bag*, const char*, int, STYtype, void*);
        ~GTKtextPopup();

        // GRpopup overrides
        void set_visible(bool visib)
            {
                if (visib)
                    gtk_widget_show(pw_shell);
                else
                    gtk_widget_hide(pw_shell);
            }
        void popdown();

        // GRtextPopup overrides
        bool get_btn2_state();
        void set_btn2_state(bool);

        bool update(const char*);

        // When set, error pop-ups have a "Show Error Log" button that
        // pops up a file browser on this file.
        //
        static void set_error_log(const char *s)
            {
                char *t = strdup(s);
                delete [] pw_errlog;
                pw_errlog = t;
            }

    private:
        bool textbox(const char*, int*, int*);
        // GTK signal handlers
        static void pw_text_popdown(GtkWidget*, void*);
        static void pw_text_action(GtkWidget*, void*);
        static void pw_btn_hdlr(GtkWidget*, void*);
        static ESret pw_save_cb(const char*, void*);
        static int pw_timeout(void*);
        static int pw_text_upd_idle(void*);

        int pw_which;
        STYtype pw_style;
        gtk_viewer *pw_viewer;
        gtkDataInterface *pw_if;
        GtkWidget *pw_text;
        GtkWidget *pw_btn;
        char *pw_upd_text;
        GTKledPopup *pw_save_pop;
        GTKmsgPopup *pw_msg_pop;
        int pw_idle_id;
        bool pw_defer;

        static char *pw_errlog;
    };
}

#endif


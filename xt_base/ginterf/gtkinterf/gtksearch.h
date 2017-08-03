
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
 * GtkInterf Graphical Interface Library                                  *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#ifndef GTKSEARCH_H
#define GTKSEARCH_H


//
// A text search pop-up, used with the text editor and help viewer.
//

typedef bool(*GtkSearchCb)(const char*, bool, bool, void*);

namespace gtkinterf {
    struct GTKsearchPopup
    {
        GTKsearchPopup(GRobject, GtkWidget*, GtkSearchCb, void*);
        ~GTKsearchPopup();

        void pop_up_search(int);

    private:
        static void search_cancel(GtkWidget*, void*);
        static void search_action(GtkWidget*, void*);
        static int fix_label_timeout(void*);
        static int scan_text(char*, char*, bool, char**, int*);

        GRobject s_caller;
        GtkWidget *s_searchwin;
        GtkSearchCb s_cb;
        void *s_arg;
        GtkWidget *s_popup;
        GtkWidget *s_text;
        GtkWidget *s_label;
        GtkWidget *s_dn;
        GtkWidget *s_up;
        GtkWidget *s_igncase;

        char *s_last_search;
        int s_timer_id;
    };
}

#endif



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
 $Id: gtksearch.h,v 2.2 2011/01/16 21:52:25 stevew Exp $
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


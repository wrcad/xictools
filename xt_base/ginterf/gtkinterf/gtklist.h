
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
 $Id: gtklist.h,v 2.15 2016/02/05 03:50:31 stevew Exp $
 *========================================================================*/

#ifndef GTKLIST_H
#define GTKLIST_H

namespace gtkinterf {
    struct GTKlistPopup : public GRlistPopup, public gtk_bag
    {
        GTKlistPopup(gtk_bag*, stringlist*, const char*, const char*,
            bool, bool, void*);
        ~GTKlistPopup();

        GtkWidget *list()           { return (ls_list); }

        // GRpopup overrides
        void set_visible(bool visib)
            {
                if (visib)
                    gtk_widget_show(wb_shell);
                else
                    gtk_widget_hide(wb_shell);
            }
        void popdown();

        // GRlistPopup override
        void update(stringlist*, const char*, const char*);
        void update(bool(*)(const char*));
        void unselect_all();

    private:
        void selection_handler(int);

        // GTK signal handlers
        static void ls_quit_proc(GtkWidget*, void*);
        static void ls_apply_proc(GtkWidget*, void*);
        static bool ls_selection_proc(GtkTreeSelection*, GtkTreeModel*,
            GtkTreePath*, bool, void*);
        static bool ls_focus_proc(GtkWidget*, GdkEvent*, void*);

        GtkWidget *ls_list;         // list widget
        GdkPixbuf *ls_open_pb;      // GRonecolPx pixmaps
        GdkPixbuf *ls_close_pb;

        int ls_row;                 // selected row;
        bool ls_no_select;          // treeview focus hack
        bool ls_use_pix;            // display using pixmaps
        bool ls_use_apply;          // use Apply button
    };
}

#endif


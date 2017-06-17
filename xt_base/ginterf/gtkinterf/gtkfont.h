
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
 $Id: gtkfont.h,v 2.19 2015/07/31 22:37:01 stevew Exp $
 *========================================================================*/

#ifndef GTKFONT_H
#define GTKFONT_H

#include "fontutil.h"

//
// Font handling
//

namespace gtkinterf {
    struct GTKfont : public GRfont
    {
        GTKfont() { last_index = 0; }

        GRfontType getType() { return (GRfontP); }
        void setName(const char*, int);
        const char *getName(int);
        char *getFamilyName(int);
        bool getFont(void*, int);
        void registerCallback(void*, int);
        void unregisterCallback(void*, int);

        static void trackFontChange(GtkWidget*, int);
        static void setupFont(GtkWidget*, int, bool);
        static char *fontNameFromFont(GdkFont*);
        static GdkFont *getWidgetFont(GtkWidget*);
        static bool stringBounds(int, const char*, int*, int*);
        static int stringWidth(GtkWidget*, const char*);
        static int stringHeight(GtkWidget*, const char*);

        // Note that this can be called once only, it resets itself to
        // avoid signal loops.
        int last_index_for_update()
            {
                int i = last_index;
                last_index = 0;
                return (i);
            }

    private:
        void refresh(int);

        struct FcbRec
        {
            FcbRec(GtkWidget *w, FcbRec *n) { widget = w; next = n; }

            GtkWidget *widget;
            FcbRec *next;
        };

        struct sFrec
        {
            sFrec() { name = 0; font = 0; cbs = 0; }

            const char *name;
            GdkFont *font;  // gtk1 only
            FcbRec *cbs;
        } fonts[MAX_NUM_APP_FONTS];

        int last_index;
    };

    // The font selection pop-up
    //
    struct GTKfontPopup : public GRfontPopup, public gtk_bag
    {
        GTKfontPopup(gtk_bag*, int, void*, const char**, const char*);
        ~GTKfontPopup();

        // GRpopup overrides
        void set_visible(bool visib)
            {
                if (visib)
                    gtk_widget_show(wb_shell);
                else
                    gtk_widget_hide(wb_shell);
            }
        void popdown();

        // GRfontPopup overrides
        void set_font_name(const char*);
        void update_label(const char*);

        void set_index(int);
        static void update_all(int);

    private:
        // GTK signal handlers
        static void ft_quit_proc(GtkWidget*, void*);
        static void ft_button_proc(GtkWidget*, void*);
        static void ft_apply_proc(GtkWidget*, void*);
        static void ft_opt_menu_proc(GtkWidget*, void*);
        static int index_idle(void*);
        void show_available_fonts(bool);

        GtkWidget *ft_fsel;
        GtkWidget *ft_label;
        int ft_index;

        static GTKfontPopup *activeFontSels[4];
    };
}

#endif


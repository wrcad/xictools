
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

#ifndef GTKFONT_H
#define GTKFONT_H

#include "ginterf/fontutil.h"

//
// Font handling
//

namespace gtkinterf {
    struct GTKfont : public GRfont
    {
        GTKfont() { last_index = 0; }

        void initFonts();
        GRfontType getType() { return (GRfontP); }
        void setName(const char*, int);
        const char *getName(int);
        char *getFamilyName(int);
        bool getFont(void*, int);
        void registerCallback(void*, int);
        void unregisterCallback(void*, int);

        static void trackFontChange(GtkWidget*, int);
        static void setupFont(GtkWidget*, int, bool);
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
            sFrec() { name = 0; cbs = 0; }

            const char *name;
            FcbRec *cbs;
        } fonts[MAX_NUM_APP_FONTS];

        int last_index;
    };

    // The font selection pop-up
    //
    struct GTKfontPopup : public GRfontPopup, public GTKbag
    {
        GTKfontPopup(GTKbag*, int, void*, const char**, const char*);
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


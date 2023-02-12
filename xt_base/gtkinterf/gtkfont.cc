
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

//#define XXX_OPT

#include <ctype.h>
#include <stdint.h>
#include "gtkinterf.h"
#include "gtkfont.h"
#include "miscutil/lstring.h"

#include <gdk/gdkprivate.h>
#include <gdk/gdkkeysyms.h>

//
// Font setting and selection utilities
//


// Exported font control class
//
namespace { GTKfont gtk_font; }

GRfont &FC = gtk_font;

GRfont::fnt_t GRfont::app_fonts[] =
{
    fnt_t( 0, 0, false, false ),  // not used
    fnt_t( "Fixed Pitch Text Window Font",    0, true, false ),
    fnt_t( "Proportional Text Window Font",   0, false, false ),
    fnt_t( "Fixed Pitch Drawing Window Font", 0, true, false ),
    fnt_t( "Text Editor Font",                0, true, false ),
    fnt_t( "HTML Viewer Proportional Family", 0, false, true ),
    fnt_t( "HTML Viewer Fixed Pitch Family",  0, true, true )
};

int GRfont::num_app_fonts =
    sizeof(GRfont::app_fonts)/sizeof(GRfont::app_fonts[0]);


// This sets the default font names and sizes.  It nust be called as
// soon as GTK is initialized, and before any calls to the other font
// functions.
//
void
GTKfont::initFonts()
{
    // Just call this once.
    if (app_fonts[0].default_fontname != 0)
        return;

    // Figure out the present font size, this should take into account
    // settings in .gtkrc-2.0 and elsewhere.
    GtkWidget *button = gtk_button_new();
    int def_size = stringWidth(button, 0);
    if (def_size <= 0)
        def_size = 9;

//    printf("Default font size %d.\n", def_size);

    char buf[80];
    sprintf(buf, "Monospace %d", def_size);
    char *def_fx_font_name = lstring::copy(buf);
    sprintf(buf, "Sans %d", def_size);
    char *def_pr_font_name = lstring::copy(buf);

    // Poke in the names.
    for (int i = 0; i < num_app_fonts; i++) {
        if (app_fonts[i].fixed)
            app_fonts[i].default_fontname = def_fx_font_name;
        else
            app_fonts[i].default_fontname = def_pr_font_name;
    }
}


namespace {
    // Test if the spacing for 'i' and 'M' are the same, return true
    // if equal.  Note that this function requires a font name, a
    // family name alone will not work in RHEL3 Pango.
    //
    bool
    is_fixed(const char *font_name)
    {
        PangoContext *pc = gdk_pango_context_get();
        if (!pc)
            return (false);
        PangoLayout *pl = pango_layout_new(pc);
        if (!pl)
            return (false);
        PangoFontDescription *pfd =
            pango_font_description_from_string(font_name);
        if (!pfd)
            return (false);

        pango_layout_set_font_description(pl, pfd);
        pango_font_description_free(pfd);

        pango_layout_set_text(pl, "i", -1);
        int tw1, th1;
        pango_layout_get_pixel_size(pl, &tw1, &th1);

        pango_layout_set_text(pl, "M", -1);
        int tw2, th2;
        pango_layout_get_pixel_size(pl, &tw2, &th2);

        g_object_unref(pl);
        g_object_unref(pc);

        return (tw1 == tw2);
    }
}


// Set the name for the font fnum, update the font.
//
void
GTKfont::setName(const char *name, int fnum)
{
    if (fnum > 0 && fnum < num_app_fonts) {
        if (!name || !*name) {
            fonts[fnum].font = 0;
            getFont(0, fnum);
        }
        else if (!fonts[fnum].name || strcmp(fonts[fnum].name, name)) {
            // This handles both XFD and free form names.
            int sz;
            char *family;
            stringlist *style;
            parse_freeform_font_string(name, &family, &style, &sz, 0);
            if (!family)
                return;
            // throw out styles if family only
            if (isFamilyOnly(fnum)) {
                stringlist::destroy(style);
                style = 0;
            }
            char buf[256];
            char *s = lstring::stpcpy(buf, family);
            for (stringlist *sl = style; sl; sl = sl->next) {
                *s++ = ' ';
                s = lstring::stpcpy(s, sl->string);
            }
            sprintf(s, " %d", sz);
            delete family;
            stringlist::destroy(style);
            name = buf;

            delete [] fonts[fnum].name;
            // Don't allow a proportional font in a fixed entry.
            if (isFixed(fnum) && !is_fixed(name))
                return;
            fonts[fnum].name = lstring::copy(name);
        }
        last_index = fnum;  // stupid thing for gtkviewer.cc
        refresh(fnum);
    }
}


// Return the saved name of the font associated with fnum.
//
const char *
GTKfont::getName(int fnum)
{
    if (fnum > 0 && fnum < num_app_fonts) {
        if (fonts[fnum].name)
            return (fonts[fnum].name);
        return (app_fonts[fnum].default_fontname);
    }
    return (0);
}


// Return the font family name, as used in the help viewer.
//
char *
GTKfont::getFamilyName(int fnum)
{
    if (fnum > 0 && fnum < num_app_fonts) {
        char *family;
        int sz;
        const char *fname = fonts[fnum].name;
        if (!fname)
            fname = app_fonts[fnum].default_fontname;
        if (!fname)
            return (0);
        parse_freeform_font_string(fname, &family, 0, &sz, 0);
        if (family) {
            char *str = new char[strlen(family) + 8];
            sprintf(str, "%s %d", family, sz);
            delete [] family;
            return (str);
        }
    }
    return (0);
}


// Return the font associated with fnum, creating it if necessary.
// Note that gtk-2 doesn't need this.
//
bool
GTKfont::getFont(void *fontp, int)
{
    if (fontp)
        *(void**)fontp = 0;
    return (true);
}


// Register a font-change callback.
//
void
GTKfont::registerCallback(void *pwidget, int fnum)
{
    GtkWidget *widget = (GtkWidget*)pwidget;
    if (fnum < 1 || fnum >= num_app_fonts)
        return;
    fonts[fnum].cbs = new FcbRec(widget, fonts[fnum].cbs);
}


// Unregister a font-change callback.
//
void
GTKfont::unregisterCallback(void *pwidget, int fnum)
{
    GtkWidget *widget = (GtkWidget*)pwidget;
    if (fnum < 1 || fnum >= num_app_fonts)
        return;
    FcbRec *rp = 0, *rn;
    for (FcbRec *r = fonts[fnum].cbs; r; r = rn) {
        rn = r->next;
        if (r->widget == widget) {
            if (rp)
                rp->next = rn;
            else
                fonts[fnum].cbs = rn;
            delete r;
            continue;
        }
        rp = r;
    }
}


namespace {
    void
    unreg_hdlr(GtkWidget *caller, void *arg)
    {
        gtk_font.unregisterCallback(caller, (int)(intptr_t)arg);
    }
}


// Static function.
// Call this to track font changes.
//
void
GTKfont::trackFontChange(GtkWidget *widget, int fnum)
{
    gtk_font.registerCallback(widget, fnum);
    g_signal_connect(G_OBJECT(widget), "destroy",
        G_CALLBACK(unreg_hdlr), (void*)(long)fnum);
}


// Static function.
// Set the default font for the widget to the index (FNT_???).  If
// track is true, the widget font can be updated from the font
// selection pop-up.
//
void
GTKfont::setupFont(GtkWidget *widget, int font_index, bool track)
{
    const char *fname = FC.getName(font_index);
    PangoFontDescription *pfd = pango_font_description_from_string(fname);
    gtk_widget_modify_font(widget, pfd);
    pango_font_description_free(pfd);
    if (track)
        GTKfont::trackFontChange(widget, font_index);
}


// Static function.
// Return the string width/height for the font index provided.
//
bool
GTKfont::stringBounds(int indx, const char *string, int *w, int *h)
{
    if (!string)
        string = "X";
    PangoContext *pc = gdk_pango_context_get();
    PangoLayout *pl = pango_layout_new(pc);
    PangoFontDescription *pfd =
        pango_font_description_from_string(FC.getName(indx));
    pango_layout_set_font_description(pl, pfd);
    pango_font_description_free(pfd);
    pango_layout_set_text(pl, string, -1);

    int x, y;
    pango_layout_get_pixel_size(pl, &x, &y);
    g_object_unref(pl);
    g_object_unref(pc);
    if (w)
        *w = x;
    if (h)
        *h = y;
    return (true);
}


// Static function.
// Return the pixel width of the string.
//
int
GTKfont::stringWidth(GtkWidget *widget, const char *string)
{
    if (!string)
        string = "X";
    PangoLayout *pl;
    PangoContext *pc = 0;
    if (GTK_IS_WIDGET(widget)) {
        pl = gtk_widget_create_pango_layout(widget, string);
        GtkRcStyle *rc = gtk_widget_get_modifier_style(widget);
        if (rc->font_desc)
            pango_layout_set_font_description(pl, rc->font_desc);
    }
    else {
        pc = gdk_pango_context_get();
        pl = pango_layout_new(pc);
        PangoFontDescription *pfd =
            pango_font_description_from_string(FC.getName(FNT_FIXED));
        pango_layout_set_font_description(pl, pfd);
        pango_font_description_free(pfd);
        pango_layout_set_text(pl, string, -1);
    }
    int x, y;
    pango_layout_get_pixel_size(pl, &x, &y);
    g_object_unref(pl);
    if (pc)
        g_object_unref(pc);
    return (x);
}


// Static function.
// Return the pixel height of the string.
//
int
GTKfont::stringHeight(GtkWidget *widget, const char *string)
{
    if (!string)
        string = "X";
    PangoLayout *pl;
    PangoContext *pc = 0;
    if (GTK_IS_WIDGET(widget)) {
        pl = gtk_widget_create_pango_layout(widget, string);
        GtkRcStyle *rc = gtk_widget_get_modifier_style(widget);
        if (rc->font_desc)
            pango_layout_set_font_description(pl, rc->font_desc);
    }
    else {
        pc = gdk_pango_context_get();
        pl = pango_layout_new(pc);
        PangoFontDescription *pfd =
            pango_font_description_from_string(FC.getName(FNT_FIXED));
        pango_layout_set_font_description(pl, pfd);
        pango_font_description_free(pfd);
        pango_layout_set_text(pl, string, -1);
    }
    int x, y;
    pango_layout_get_pixel_size(pl, &x, &y);
    g_object_unref(pl);
    if (pc)
        g_object_unref(pc);
    return (y);
}


// Private function to refresh the text widgets.
//
void
GTKfont::refresh(int fnum)
{
    for (FcbRec *r = fonts[fnum].cbs; r; r = r->next) {
        PangoFontDescription *pfd =
            pango_font_description_from_string(fonts[fnum].name);
        gtk_widget_modify_font(r->widget, pfd);
        pango_font_description_free(pfd);
    }
}
// End of GTKfont functions.


//-----------------------------------------------------------------------------
// Font Selection Pop-Up

// Pop up the font selector.
//  caller      initiating button
//  loc         positioning
//  mode        MODE_ON or MODE_OFF
//  cb          Callback function for btns
//  arg         Callback function user arg
//  indx        If >= 1, update the GTKfont for this index
//  btns        0-terminated list of button names, may be 0
//  labeltext   Alternative text for label
//
// If the first string in the btns array is "+", the remaining buttons
// will be added to the left of the internal Apply/option menu/Dismiss
// buttons.  Otherwise, the btns will appear instead of these.
//
void
gtk_bag::PopUpFontSel(GRobject caller, GRloc loc, ShowMode mode,
    void(*cb)(const char*, const char*, void*), void *arg,
    int indx, const char **btns, const char *labeltext)
{
    if (mode == MODE_OFF) {
        if (wb_fontsel)
            wb_fontsel->popdown();
        return;
    }
    if (mode == MODE_UPD) {
        if (wb_fontsel)
            wb_fontsel->set_index(indx);
        return;
    }
    if (wb_fontsel)
        return;

    wb_fontsel = new GTKfontPopup(this, indx, arg, btns, labeltext);
    wb_fontsel->register_caller(caller);
    wb_fontsel->register_callback(cb);

    gtk_window_set_transient_for(GTK_WINDOW(wb_fontsel->wb_shell),
        GTK_WINDOW(wb_shell));
    GRX->SetPopupLocation(loc, wb_fontsel->wb_shell, PositionReferenceWidget());

    wb_fontsel->set_visible(true);
}
// End of gtk_bag functions


GTKfontPopup *GTKfontPopup::activeFontSels[4];


GTKfontPopup::GTKfontPopup(gtk_bag *owner, int indx, void *arg,
    const char **btns, const char *labeltext)
{
    p_parent = owner;
    p_cb_arg = arg;
    ft_fsel = 0;
    ft_label = 0;
    ft_index = indx;

    for (int i = 0; i < 4; i++) {
        if (!activeFontSels[i]) {
            activeFontSels[i] = this;
            break;
        }
    }

    if (owner)
        owner->MonitorAdd(this);

    wb_shell = gtk_NewPopup(owner, "Font Selection", ft_quit_proc, this);
    if (!wb_shell)
        return;
    gtk_window_set_resizable(GTK_WINDOW(wb_shell), false);

    GtkWidget *form = gtk_vbox_new(false, 2);
    gtk_container_add(GTK_CONTAINER(wb_shell), form);
    gtk_container_set_border_width(GTK_CONTAINER(wb_shell), 2);
    gtk_widget_show(form);
    ft_fsel = gtk_font_selection_new();
    gtk_widget_show(ft_fsel);
    gtk_box_pack_start(GTK_BOX(form), ft_fsel, true, true, 0);

    GtkWidget *hbox = gtk_hbox_new(false, 2);
    gtk_widget_show(hbox);
    gtk_box_pack_start(GTK_BOX(form), hbox, false, false, 0);

    bool use_def = !btns;
    if (btns) {
        for (int i = 0; btns[i]; i++) {
            if (i == 0 && *btns[i] == '+') {
                use_def = true;
                continue;
            }
            GtkWidget *button = gtk_button_new_with_label(btns[i]);
            gtk_widget_set_name(button, btns[i]);
            gtk_widget_show(button);
            gtk_box_pack_start(GTK_BOX(hbox), button, true, true, 0);
            g_signal_connect(G_OBJECT(button), "clicked",
                G_CALLBACK(ft_button_proc), this);
        }
    }
    if (use_def) {
        GtkWidget *button = gtk_button_new_with_label("Apply");
        gtk_widget_set_name(button, "Apply");
        gtk_widget_show(button);
        gtk_box_pack_start(GTK_BOX(hbox), button, true, true, 0);
        g_signal_connect(G_OBJECT(button), "clicked",
            G_CALLBACK(ft_apply_proc), this);
        if (indx > 0) {
#ifdef XXX_OPT
            GtkWidget *opt = gtk_option_menu_new();
            gtk_widget_show(opt);
            GtkWidget *menu = gtk_menu_new();
            gtk_widget_show(menu);
            for (int i = 1; i < gtk_font.num_app_fonts; i++) {
                GtkWidget *mi =
                    gtk_menu_item_new_with_label(gtk_font.getLabel(i));
                gtk_widget_show(mi);
                gtk_widget_set_name(mi, gtk_font.getLabel(i));
                g_object_set_data(G_OBJECT(mi), "index", (void*)(long)i);
                gtk_menu_shell_append(GTK_MENU_SHELL(menu), mi);
                g_signal_connect(G_OBJECT(mi), "activate",
                    G_CALLBACK(ft_opt_menu_proc), this);
            }
            gtk_option_menu_set_menu(GTK_OPTION_MENU(opt), menu);
            gtk_option_menu_set_history(GTK_OPTION_MENU(opt), indx-1);
#else
            GtkWidget *opt = gtk_combo_box_text_new();
            gtk_widget_show(opt);
            for (int i = 1; i < gtk_font.num_app_fonts; i++) {
                gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(opt),
                    gtk_font.getLabel(i));
            }
            //XXX Connect signals.
            g_signal_connect(G_OBJECT(opt), "changed",
                G_CALLBACK(ft_opt_menu_proc), this);
            gtk_combo_box_set_active(GTK_COMBO_BOX(opt), indx-1);
#endif
            gtk_box_pack_start(GTK_BOX(hbox), opt, true, true, 0);
        }
    }

    GtkWidget *button = gtk_button_new_with_label("Dismiss");
    gtk_widget_set_name(button, "Dismiss");
    gtk_widget_show(button);
    gtk_box_pack_start(GTK_BOX(hbox), button, true, true, 0);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(ft_quit_proc), this);
    gtk_window_set_focus(GTK_WINDOW(wb_shell), button);

    if (labeltext) {
        ft_label = gtk_label_new(labeltext);
        gtk_widget_show(ft_label);
        gtk_box_pack_start(GTK_BOX(form), ft_label, true, true, 0);
    }

    g_idle_add((GSourceFunc)index_idle, this);
}


GTKfontPopup::~GTKfontPopup()
{
    for (int i = 0; i < 4; i++) {
        if (activeFontSels[i] == this) {
            activeFontSels[i] = 0;
            break;
        }
    }
    if (p_parent) {
        gtk_bag *owner = dynamic_cast<gtk_bag*>(p_parent);
        if (owner)
            owner->ClearPopup(this);
    }
    if (p_callback)
        (*p_callback)(0, 0, p_cb_arg);
    if (p_usrptr)
        *p_usrptr = 0;
    if (p_caller)
        GRX->Deselect(p_caller);
    g_signal_handlers_disconnect_by_func(G_OBJECT(wb_shell),
        (gpointer)ft_quit_proc, this);
}


// GRpopup override
//
void
GTKfontPopup::popdown()
{
    if (p_parent) {
        gtk_bag *owner = dynamic_cast<gtk_bag*>(p_parent);
        if (!owner || !owner->MonitorActive(this))
            return;
    }
    delete this;
}


// GRfontPopup override
// Set the font.
void
GTKfontPopup::set_font_name(const char *fontname)
{
    if (p_parent) {
        gtk_bag *owner = dynamic_cast<gtk_bag*>(p_parent);
        if (!owner || !owner->MonitorActive(this))
            return;
    }
    if (!fontname || !*fontname)
        return;

    if (!gtk_font_selection_set_font_name(GTK_FONT_SELECTION(ft_fsel),
            fontname)) {
        sLstr lstr;
        lstr.add("Warning\nUnknown or invalid font name\n");
        lstr.add(fontname);
        PopUpMessage(lstr.string(), true);
    }
}


// GRfontPopup override
// Update the label text (if a label is being used).
//
void
GTKfontPopup::update_label(const char *text)
{
    if (p_parent) {
        gtk_bag *owner = dynamic_cast<gtk_bag*>(p_parent);
        if (!owner || !owner->MonitorActive(this))
            return;
    }
    if (ft_label)
        gtk_label_set_text(GTK_LABEL(ft_label), text);
}


// Switch the font selector context to the new index.
//
void
GTKfontPopup::set_index(int ix)
{
    ft_index = ix;
    show_available_fonts(gtk_font.isFixed(ft_index));
    if (ix > 0) {
        if (gtk_font.getFont(0, ft_index))
            set_font_name(gtk_font.getName(ft_index));
    }
    else
        set_font_name(gtk_font.getName(FNT_FIXED));
    GtkFontSelection *fsel = GTK_FONT_SELECTION(ft_fsel);
//XXX    gtk_widget_set_sensitive(fsel->face_list, !gtk_font.isFamilyOnly(ix));
    gtk_widget_set_sensitive(gtk_font_selection_get_face_list(fsel),
        !gtk_font.isFamilyOnly(ix));
}


// Static function.
// Update all font pop-ups for fnum.
//
void
GTKfontPopup::update_all(int fnum)
{
    for (int i = 0; i < 4; i++) {
        if (!activeFontSels[i])
            continue;
        if (activeFontSels[i]->ft_index == fnum) {
            gtk_font_selection_set_font_name(
                GTK_FONT_SELECTION(activeFontSels[i]->ft_fsel),
                gtk_font.getName(fnum));
        }
    }
}


// Private static GTK signal handler.
// Destroy callback.
//
void
GTKfontPopup::ft_quit_proc(GtkWidget*, void *client_data)
{
    GTKfontPopup *sel = static_cast<GTKfontPopup*>(client_data);
    if (sel)
        sel->popdown();
}


// Private static GTK signal handler.
// Button press callback.
//
void
GTKfontPopup::ft_button_proc(GtkWidget *widget, void *client_data)
{
    GTKfontPopup *sel = static_cast<GTKfontPopup*>(client_data);
    if (sel && sel->p_callback) {
        const char *btname = gtk_widget_get_name(widget);
        const char *fontname =
            gtk_font_selection_get_font_name(GTK_FONT_SELECTION(sel->ft_fsel));
        if (!fontname)
            fontname = "";
        (*sel->p_callback)(btname, fontname, sel->p_cb_arg);
    }
}


// Private static GTK signal handler.
// Apply button callback.
//
void
GTKfontPopup::ft_apply_proc(GtkWidget*, void *client_data)
{
    GTKfontPopup *sel = static_cast<GTKfontPopup*>(client_data);
    if (sel) {
        const char *fontname =
            gtk_font_selection_get_font_name(GTK_FONT_SELECTION(sel->ft_fsel));
        if (!fontname || !*fontname)
            return;

        if (sel->ft_index >= 1)
            gtk_font.setName(fontname, sel->ft_index);
        if (sel->p_callback) {
            char buf[32];
            sprintf(buf, "%d", sel->ft_index);
            (*sel->p_callback)(buf, gtk_font.getName(sel->ft_index),
                sel->p_cb_arg);
        }
        for (int i = 0; i < 4; i++) {
            if (!activeFontSels[i] || activeFontSels[i] == sel)
                continue;
            if (activeFontSels[i]->ft_index == sel->ft_index) {
                gtk_font_selection_set_font_name(
                    GTK_FONT_SELECTION(activeFontSels[i]->ft_fsel),
                    gtk_font.getName(sel->ft_index));
            }
        }
    }
}


// Private static GTK signal handler.
// Handler for the option menu, switches the active application font.
//
void
GTKfontPopup::ft_opt_menu_proc(GtkWidget *caller, void *client_data)
{
    GTKfontPopup *sel = static_cast<GTKfontPopup*>(client_data);
    if (sel) {
#ifdef XXX_OPT
        int ix = (intptr_t)g_object_get_data(G_OBJECT(caller), "index");
        sel->set_index(ix);
#else
        int ix = gtk_combo_box_get_active(GTK_COMBO_BOX(caller));
        sel->set_index(ix + 1);
#endif
    }
}


// The gtk2 font selection does not (yet) have filtering capability.
// The following code will replace the family list with fixed fonts
// only.  Based on widget code from gtk source.

namespace {
    int cmp_families(const void *a, const void *b)
    {
        const char *a_name = pango_font_family_get_name(*(PangoFontFamily**)a);
        const char *b_name = pango_font_family_get_name(*(PangoFontFamily**)b);
        return g_utf8_collate(a_name, b_name);
    }
}


// Static function.
// GTK-2,20,1 (at least) requires using an idle proc for the initial index
// setting, otherwise there is trouble.
//
int
GTKfontPopup::index_idle(void *arg)
{
    GTKfontPopup *ft = (GTKfontPopup*)arg;
    ft->set_index(ft->ft_index);
    return (0);
}


void
GTKfontPopup::show_available_fonts(bool fixed)
{
    GtkFontSelection *fsel = GTK_FONT_SELECTION(ft_fsel);
    GtkListStore *model = GTK_LIST_STORE(
//XXX        gtk_tree_view_get_model(GTK_TREE_VIEW(fsel->family_list)));
        gtk_tree_view_get_model(
            GTK_TREE_VIEW(gtk_font_selection_get_family_list(fsel))));

    PangoFontFamily **families;
    int n_families;
    pango_context_list_families(
        gtk_widget_get_pango_context(GTK_WIDGET(fsel)),
        &families, &n_families);
    qsort(families, n_families, sizeof(PangoFontFamily*), cmp_families);

    gtk_list_store_clear(model);

    for (int i = 0; i < n_families; i++) {
        const char *name = pango_font_family_get_name(families[i]);

        if (fixed) {
            if (!pango_font_family_is_monospace(families[i]))
                continue;
            // Bah! The function above lies sometimes.
            char buf[256];
            sprintf(buf, "%s 10", name);
            if (!is_fixed(buf))
                continue;
        }

        GtkTreeIter iter;
        gtk_list_store_append(model, &iter);

        // GTK-2.20.1 will crash without this.
        g_object_ref(families[i]);

        gtk_list_store_set(model, &iter, 0, families[i], 1, name, -1);
    }
//XXX Can't access family element, so probably can't free families.
//    fsel->family = 0;
//    g_free(families);
}
// End of GTKfontPopup functions


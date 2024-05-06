
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
 * Xic Integrated Circuit Layout and Schematic Editor                     *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#include "main.h"
#include "dsp_inlines.h"
#include "fio.h"
#include "keymap.h"
#include "keymacro.h"
#include "promptline.h"
#include "tech.h"
#include "errorlog.h"
#include "gtkmain.h"

#include <gdk/gdkkeysyms.h>
#ifdef WITH_X11
#include "gtkinterf/gtkx11.h"
#else
#ifdef WIN32
#include <windows.h>
#endif
#endif


/*========================================================================
  Keyboard Macros
 ========================================================================*/

namespace gtk_keyb {

    // List element for widget name matches
    //
    struct wlist
    {
        wlist(GtkWidget *w, wlist *n) { widget = w; next = n; }

        GtkWidget *widget;
        wlist *next;
    };

    void macro_event_handler(GdkEvent*, void*);
    GtkWidget *find_wname(GdkWindow*, char**);
    char *widget_path(GtkWidget*);
    wlist *find_widget(GtkWidget*, const char*);
    GtkWidget *name_to_widget(const char*);
}


// Global interface for the script reader.
//
bool
cMain::SendKeyEvent(const char *widget_name, int keysym, int state,
    bool up)
{
    if (!GTKdev::exists())
        return (false);
    sKeyEvent ev(lstring::copy(widget_name), state,
        up ? KEY_RELEASE : KEY_PRESS, keysym);
    if (!up)
        ev.set_text();
    return (ev.exec());
}


// Global interface for the script reader.
//
bool
cMain::SendButtonEvent(const char *widget_name, int state, int num, int x,
    int y, bool up)
{
    if (!GTKdev::exists())
        return (false);
    sBtnEvent ev(lstring::copy(widget_name), state,
        up ? BUTTON_RELEASE : BUTTON_PRESS, num, x, y);
    return (ev.exec());
}


namespace {
    // Convert the state flags to the Xic representation.
    //
    inline unsigned
    convert_state(unsigned state)
    {
        // This is a no-op since defines should be identical
        unsigned st = 0;
        if (state & GDK_CONTROL_MASK)
            st |= GR_CONTROL_MASK;
        if (state & GDK_SHIFT_MASK)
            st |= GR_SHIFT_MASK;
        if (state & GDK_MOD1_MASK)
            st |= GR_ALT_MASK;
        return (st);
    }
}


// Main function to obtain a macro mapping for a keypress.
//
sKeyMap *
cKbMacro::getKeyToMap()
{
    bool done = false;
    sKeyMap *nkey = 0;
    int prcnt = 0;
    PL()->ShowPrompt("Enter keypress to map: ");
    for (;;) {
        GdkEvent *ev = gdk_event_get();
        if (!ev)
            continue;
        if (ev->type == GDK_KEY_PRESS) {
            prcnt++;
            if (ev->key.keyval == 0) {
                // may happen if key is unknown, e.g. the Microsoft Windows
                // key
                gdk_event_free(ev);
                continue;
            }
            if (isModifier(ev->key.keyval)) {
                gdk_event_free(ev);
                continue;
            }
            if ((ev->key.string && ev->key.string[1] == 0 &&
                    (*ev->key.string == 27 || *ev->key.string == 13)) &&
                    !(ev->key.state & (GDK_CONTROL_MASK | GDK_MOD1_MASK))) {
                // Esc or CR
                PL()->ErasePrompt();
                gdk_event_free(ev);
                return (0);
            }
            if (!done) {
                if (notMappable(ev->key.keyval, ev->key.state)) {
                    PL()->ShowPrompt("Not mappable, try another: ");
                    gdk_event_free(ev);
                    continue;
                }
                nkey = AlreadyMapped(ev->key.keyval,
                    convert_state(ev->key.state));
                if (!nkey)
                    nkey = new sKeyMap(ev->key.keyval,
                        convert_state(ev->key.state), ev->key.string);
                done = true;
            }
            gdk_event_free(ev);
            continue;
        }
        if (ev->type == GDK_KEY_RELEASE) {
            if (prcnt)
                prcnt--;
            if (!prcnt && done)
                break;
            gdk_event_free(ev);
            continue;
        }
        if (ev->type == GDK_BUTTON_PRESS ||
                ev->type == GDK_2BUTTON_PRESS ||
                ev->type == GDK_3BUTTON_PRESS) {
            gdk_event_free(ev);
            return (0);
        }
        gtk_main_do_event(ev);
        gdk_event_free(ev);
    }
    return (nkey);
}


// Static function.
bool
cKbMacro::isModifier(unsigned key)
{
    return ((key >= GDK_KEY_Shift_L && key <= GDK_KEY_Hyper_R)
           || key == GDK_KEY_Mode_switch || key == GDK_KEY_Num_Lock);
}


// Static function.
bool
cKbMacro::isControl(unsigned key)
{
    return (key == GDK_KEY_Control_L || key == GDK_KEY_Control_R);
}


// Static function.
bool
cKbMacro::isShift(unsigned key)
{
    return (key == GDK_KEY_Shift_L || key == GDK_KEY_Shift_R);
}


// Static function.
bool
cKbMacro::isAlt(unsigned key)
{
    return (key == GDK_KEY_Alt_L || key == GDK_KEY_Alt_R);
}


// Static function.
char *
cKbMacro::keyText(unsigned key, unsigned state)
{
    static char text[8];
#ifdef WITH_X11
    if (gr_x_display()) {
        XKeyEvent kev;
        memset(&kev, 0, sizeof(XKeyEvent));
        kev.type = KeyPress;
        kev.display = gr_x_display();
        kev.keycode = XKeysymToKeycode(gr_x_display(), key);
        kev.state = state;
        unsigned long dummy;
        int n = XLookupString(&kev, text, 4, &dummy, 0);
        text[n] = 0;
    }
#else
#ifdef WIN32
    unsigned char kstate[256];
    memset(kstate, 0, 256);
    if (state & GR_CONTROL_MASK)
        kstate[VK_CONTROL] = 0x80;
    if (state & GR_SHIFT_MASK)
        kstate[VK_SHIFT] = 0x80;
    if (state & GR_ALT_MASK)
        kstate[VK_MENU] = 0x80;
    WORD c;
    int ret = ToAscii(key, 0, kstate, &c, 0);
    if (ret && c && isascii(c)) {
        text[0] = c;
        text[1] = 0;
    }
    else
        text[0] = 0;
#else
#ifdef WITH_QUARTZ
//XXX Need better code here, the 'state' isn't used.
    (void)state;
    memset(text, 0, sizeof(text));
    unsigned int c = 0;
    if (key != GDK_VoidSymbol)
        c = gdk_keyval_to_unicode(key);
    if (c) {
        char buf[8];
        int len = g_unichar_to_utf8(c, buf);
        buf[len] = 0;

        unsigned long bytes;
        char *string = g_locale_from_utf8(buf, len, 0, &bytes, 0);
        memcpy(text, string, bytes);
        delete [] string;
    }
    else if (key == GDK_Escape)
        text[0] = '\033';
    else if (key == GDK_Return || key == GDK_KP_Enter)
        text[0] = '\n';

#endif
#endif
#endif
    return (text);
}


// Static function.
void
cKbMacro::keyName(unsigned key, char *buf)
{
    char *t = gdk_keyval_name(key);
    strncpy(buf, t, 32);
}


// Static function.
bool
cKbMacro::isModifierDown()
{
    if (GTKmainwin::self()) {
        int x, y, state;
        gdk_window_get_pointer(gtk_widget_get_window(
            GTKmainwin::self()->Viewport()),
            &x, &y, (GdkModifierType*)&state);
        if (state & (GDK_SHIFT_MASK | GDK_CONTROL_MASK | GDK_MOD1_MASK))
            // modifier down
            return (true);
    }
    return (false);
}


// Static function.
// Return true if the given key event can not be mapped.
//
bool
cKbMacro::notMappable(unsigned key, unsigned state)
{
    if (isModifier(key))
        return (true);
    if (state)
        return (false);
    switch (key) {
    case GDK_KEY_Return:
    case GDK_KEY_Escape:
        return (true);
    }
    return (false);
}


// Send a key event, return false if the window can't be found.
//
bool
cKbMacro::execKey(sKeyEvent *k)
{
    if (!GTKmainwin::self())
        return (false);
    if (!k->widget_name) {
        GTKmainwin::self()->send_key_event(k);
        return (true);
    }

    GTKpkg::self()->CheckForInterrupt();
    GtkWidget *w = gtk_keyb::name_to_widget(k->widget_name);
    if (w && !gtk_widget_get_window(w))
        gtk_widget_realize(w);
    if (!w || !gtk_widget_get_window(w))
        return (false);

    GdkEventKey event;
    memset(&event, 0, sizeof(GdkEventKey));
    event.window = gtk_widget_get_window(w);
    event.send_event = true;
    event.time = GDK_CURRENT_TIME;
    event.state = k->state;
    event.keyval = k->key;
    event.string = k->text;
    event.length = strlen(k->text);
    event.type = (k->type == KEY_PRESS ? GDK_KEY_PRESS : GDK_KEY_RELEASE);

    gtk_propagate_event(w, (GdkEvent*)&event);
    return (true);
}


// Send a button event, return false if the window can't be found.
//
bool
cKbMacro::execBtn(sBtnEvent *b)
{
    if (b->type == BUTTON_PRESS)
        GTKpkg::self()->CheckForInterrupt();
    GtkWidget *w = gtk_keyb::name_to_widget(b->widget_name);
    if (w && !gtk_widget_get_window(w))
        gtk_widget_realize(w);
    if (!w || !gtk_widget_get_window(w))
        return (false);

    if (GTK_IS_BUTTON(w)) {
        if (b->type == BUTTON_RELEASE) {
            if (w == (GtkWidget*)km_last_btn)
                gtk_button_clicked(GTK_BUTTON(w));
            km_last_btn = 0;
        }
        else
            km_last_btn = w;
        km_last_menu = 0;
    }
    else if (GTK_IS_MENU_ITEM(w)) {
        if (b->type == BUTTON_RELEASE) {
            if (km_last_menu) {
                gtk_menu_item_activate(GTK_MENU_ITEM(w));
                gtk_menu_shell_deactivate(
                    GTK_MENU_SHELL(gtk_widget_get_parent(w)));
            }
            km_last_menu = 0;
        }
        else
            km_last_menu = w;
        km_last_btn = 0;
    }
    else {
        km_last_menu = w;
        km_last_btn = 0;
        int xo, yo;
        gdk_window_get_root_origin(gtk_widget_get_window(w), &xo, &yo);

        GdkEventButton event;
        memset(&event, 0, sizeof(GdkEventButton));
        event.window = gtk_widget_get_window(w);
        event.send_event = true;
        event.time = GDK_CURRENT_TIME;
        event.x = b->x;
        event.y = b->y;
        event.x_root = xo + b->x;
        event.y_root = yo + b->y;
        event.state = b->state;
        event.button = b->button;

        event.type = (b->type == BUTTON_PRESS ?
            GDK_BUTTON_PRESS : GDK_BUTTON_RELEASE);
        gtk_propagate_event(w, (GdkEvent*)&event);
    }
    return (true);
}
// End of cKbMacro functions


// Start recording events for macro definition.
//
void
sKeyMap::begin_recording(char *str)
{
    forstr = lstring::copy(str);
    for (end = response; end && end->next; end = end->next) ;
    show();

    lastkey = 0;
    grabber = 0;

    GTKpkg::self()->RegisterEventHandler(gtk_keyb::macro_event_handler, this);
}
// End of sKeyMap functions.


// Event handler in effect while processing macro definition input.
//
void
gtk_keyb::macro_event_handler(GdkEvent *ev, void *arg)
{
    sKeyMap *km = (sKeyMap*)arg;
    if (ev->type == GDK_KEY_RELEASE) {
        // the string field of the key up event is empty
        if (km->lastkey == 13 &&
                !(ev->key.state & (GDK_CONTROL_MASK | GDK_MOD1_MASK))) {
            km->fix_modif();
            GTKpkg::self()->RegisterEventHandler(0, 0);
            KbMac()->SaveMacro(km, true);
            return;
        }
        if (ev->key.keyval == GDK_KEY_BackSpace &&
                !(ev->key.state & (GDK_CONTROL_MASK | GDK_MOD1_MASK)))
            return;

        char *wname;
        if (gtk_keyb::find_wname(ev->key.window, &wname)) {
            sEvent *nev = new sKeyEvent(wname,
                convert_state(ev->key.state), KEY_RELEASE, ev->key.keyval);
            if (ev->key.string && ev->key.string[1] == 0)
                *((sKeyEvent*)nev)->text = *ev->key.string;
            km->add_response(nev);
        }
        return;
    }
    if (ev->type == GDK_KEY_PRESS) {
        km->lastkey = (ev->key.string && ev->key.string[1] == 0) ?
            *ev->key.string : 0;
        if (km->lastkey == 13 &&
                !(ev->key.state & (GDK_CONTROL_MASK | GDK_MOD1_MASK)))
            return;
        if (ev->key.keyval == GDK_KEY_Escape &&
                !(ev->key.state & (GDK_CONTROL_MASK | GDK_MOD1_MASK))) {
            km->clear_response();
            GTKpkg::self()->RegisterEventHandler(0, 0);
            KbMac()->SaveMacro(km, false);
            return;
        }
        if (ev->key.keyval == GDK_KEY_BackSpace &&
                !(ev->key.state & (GDK_CONTROL_MASK | GDK_MOD1_MASK))) {
            if (!km->response)
                return;
            km->clear_last_event();
            km->show();
            return;
        }

        char *wname;
        if (gtk_keyb::find_wname(ev->key.window, &wname)) {
            sEvent *nev = new sKeyEvent(wname,
                convert_state(ev->key.state), KEY_PRESS, ev->key.keyval);
            if (ev->key.string && ev->key.string[1] == 0)
                *((sKeyEvent*)nev)->text = *ev->key.string;
            km->add_response(nev);
            km->show();
        }
        return;
    }
    if (ev->type == GDK_BUTTON_PRESS || ev->type == GDK_BUTTON_RELEASE) {
        if (ev->type == GDK_BUTTON_RELEASE && km->grabber) {
            km->grabber = 0;
            gtk_main_do_event(ev);
        }

        char *wname;
        GtkWidget *widg = gtk_keyb::find_wname(ev->button.window, &wname);
        if (widg) {
            sEvent *nev = new sBtnEvent(wname,
                convert_state(ev->button.state),
                ev->button.type == GDK_BUTTON_PRESS ?
                BUTTON_PRESS : BUTTON_RELEASE, ev->button.button,
                (int)ev->button.x, (int)ev->button.y);
            if (ev->type == GDK_BUTTON_PRESS && GTK_IS_MENU_ITEM(widg) &&
                    gtk_menu_item_get_submenu(GTK_MENU_ITEM(widg))) {
                // show the menu
                km->grabber = widg;
                gtk_main_do_event(ev);
            }
            km->add_response(nev);
            if (ev->type == GDK_BUTTON_PRESS)
                km->show();
        }
        return;
    }
    if (ev->type == GDK_2BUTTON_PRESS || ev->type == GDK_3BUTTON_PRESS)
        return;
    gtk_main_do_event(ev);
}


// Return the widget and its name from the window.  Return 0 if the widget
// isn't recognized.
//
GtkWidget *
gtk_keyb::find_wname(GdkWindow *window, char **wname)
{
    // this is the magic to get a widget from a window
    GtkWidget *widget = 0;
    gdk_window_get_user_data(window, (void**)&widget);

    if (widget) {
        char *name = widget_path(widget);
        if (name) {
            *wname = name;
            return (widget);
        }
    }
    return (0);
}


// Return the path name for the widget.  Yes, there is a gtk function that
// does the same thing, but it has a massive core leak (2.4.6).
//
char *
gtk_keyb::widget_path(GtkWidget *widget)
{
    if (!widget || !GTK_IS_WIDGET(widget))
        return (0);
    char *aa[256];
    int cnt = 0, len = 0;
    while (widget) {
        aa[cnt] = (char*)gtk_widget_get_name(widget);
        len += strlen(aa[cnt]) + 1;
        widget = gtk_widget_get_parent(widget);
        cnt++;
    }
    cnt--;
    char *path = new char[len+1];
    char *t = path;
    while (cnt >= 0) {
        strcpy(t, aa[cnt]);
        while (*t)
            t++;
        if (cnt)
            *t++ = '.';
        cnt--;
    }
    *t = 0;
    return (path);
}


// Return a list of widgets that match the path given, relative to w.
//
gtk_keyb::wlist *
gtk_keyb::find_widget(GtkWidget *w, const char *path)
{
    wlist *w0 = 0;
    const char *t = strchr(path, '.');
    if (!t)
        t = path + strlen(path);
    char name[128];
    strncpy(name, path, t-path);
    name[t-path] = 0;
    GList *g = gtk_container_get_children(GTK_CONTAINER(w));
    for (GList *gt = g; gt; gt = gt->next) {
        GtkWidget *ww = GTK_WIDGET(gt->data);
        if (!strcmp(name, gtk_widget_get_name(ww))) {
            if (*t == '.') {
                wlist *wx = find_widget(ww, t+1);
                if (wx) {
                    wlist *wn = wx;
                    while (wn->next)
                        wn = wn->next;
                    wn->next = w0;
                    w0 = wx;
                }
            }
            else
                w0 = new wlist(ww, w0);
        }
    }
    if (g)
        g_list_free(g);
    return (w0);
}


// If the path identifies a widget uniquely, return it.  The path is rooted
// in a top level (GtkWindow) widget.
//
GtkWidget *
gtk_keyb::name_to_widget(const char *path)
{
    if (!path)
        return (0);
    wlist *w0 = 0;
    const char *t = strchr(path, '.');
    if (!t)
        t = path + strlen(path);
    char name[128];
    strncpy(name, path, t-path);
    name[t-path] = 0;
    GList *g = gtk_window_list_toplevels();
    for ( ; g; g = g->next) {
        GtkWidget *ww = GTK_WIDGET(g->data);
        if (!ww)
            continue;
        const char *wnm = gtk_widget_get_name(ww);
        if (!wnm)
            continue;
        if (!strcmp(name, wnm)) {
            if (*t == '.') {
                wlist *wx = find_widget(ww, t+1);
                if (wx) {
                    wlist *wn = wx;
                    while (wn->next)
                        wn = wn->next;
                    wn->next = w0;
                    w0 = wx;
                }
            }
            else
                w0 = new wlist(ww, w0);
        }
    }
    g_list_free(g);
    GtkWidget *wfound = 0;
    if (w0 && !w0->next)
        wfound = w0->widget;
    while (w0) {
        wlist *wx = w0->next;
        delete w0;
        w0 = wx;
    }
    return (wfound);
}


/*========================================================================
  Event History Logging
 ========================================================================*/

namespace {
    // Print the first few lines of the run log file.
    //
    void
    log_init(FILE *fp)
    {
        fprintf(fp, "# %s\n", XM()->IdString());

        char buf[256];
        if (FIO()->PGetCWD(buf, 256))
            fprintf(fp, "Cwd(\"%s\")\n", buf);

        fprintf(fp, "Techfile(\"%s\")\n",
            Tech()->TechFilename() ? Tech()->TechFilename() : "<none>");
        fprintf(fp, "Edit(\"%s\", 0)\n",
            DSP()->CurCellName() ?
                Tstring(DSP()->CurCellName()) : "<none>");
        fprintf(fp, "Mode(%d)\n", DSP()->CurMode());
        if (DSP()->CurMode() == Physical) {
            fprintf(fp, "Window(%g, %g, %g, 0)\n",
                (double)(DSP()->MainWdesc()->Window()->left +
                    DSP()->MainWdesc()->Window()->right)/(2*CDphysResolution),
                (double)(DSP()->MainWdesc()->Window()->bottom +
                    DSP()->MainWdesc()->Window()->top)/(2*CDphysResolution),
                (double)DSP()->MainWdesc()->Window()->width()/
                    CDphysResolution);
        }
        else {
            fprintf(fp, "Window(%g, %g, %g, 0)\n",
                (double)(DSP()->MainWdesc()->Window()->left +
                    DSP()->MainWdesc()->Window()->right)/(2*CDelecResolution),
                (double)(DSP()->MainWdesc()->Window()->bottom +
                    DSP()->MainWdesc()->Window()->top)/(2*CDelecResolution),
                (double)DSP()->MainWdesc()->Window()->width()/
                    CDelecResolution);
        }
    }
}


// Print a line in the run log file describing the event.  The print format
// is readable by the function parser.
//
void
LogEvent(GdkEvent *ev)
{
    static int ccnt;
    const char *msg = "Can't open %s file, history won't be logged.";
    if (ccnt < 0)
        return;

    if (ev->type == GDK_KEY_PRESS || ev->type == GDK_KEY_RELEASE) {
        FILE *fp = Log()->OpenLog(XM()->LogName(), ccnt ? "a" : "w", true);
        if (fp) {
            if (!ccnt)
                log_init(fp);
            if (ev->key.string && isprint(*ev->key.string))
                fprintf(fp, "# %s\n", ev->key.string);
            else
                fprintf(fp, "# %s\n", gdk_keyval_name(ev->key.keyval));
            if (ev->key.send_event) {
                fprintf(fp, "# XSendEvent\n");
                fprintf(fp, "# ");
            }

            GtkWidget *widget = gtk_get_event_widget(ev);
            char *wname = gtk_keyb::widget_path(widget);
            if (!wname)
                wname = lstring::copy("unknown");
            fprintf(fp, "%s(%d, %d, \"%s\")\n",
                ev->type == GDK_KEY_PRESS ? "KeyDown" : "KeyUp",
                ev->key.keyval, ev->key.state, wname);
            delete [] wname;
            fclose(fp);
        }
        else if (!ccnt) {
            Log()->WarningLogV(mh::Initialization, msg, XM()->LogName());
            ccnt = -1;
            return;
        }
        ccnt++;
    }
    else if (ev->type == GDK_BUTTON_PRESS || ev->type == GDK_BUTTON_RELEASE) {
        FILE *fp = Log()->OpenLog(XM()->LogName(), ccnt ? "a" : "w", true);
        if (fp) {
            if (!ccnt)
                log_init(fp);
            if (ev->button.send_event)
                // suppress all send events
                fprintf(fp, "# XSendEvent\n# ");
            GtkWidget *widget = gtk_get_event_widget(ev);
            char *wname = gtk_keyb::widget_path(widget);
            if (!wname)
                wname = lstring::copy("unknown");
            fprintf(fp, "%s(%d, %d, %d, %d, \"%s\")\n",
                ev->type == GDK_BUTTON_PRESS ? "BtnDown" : "BtnUp",
                ev->button.button, ev->button.state, (int)ev->button.x,
                (int)ev->button.y, wname);
            delete [] wname;
            fclose(fp);
        }
        else if (!ccnt) {
            Log()->WarningLogV(mh::Initialization, msg, XM()->LogName());
            ccnt = -1;
            return;
        }
        ccnt++;
    }
}


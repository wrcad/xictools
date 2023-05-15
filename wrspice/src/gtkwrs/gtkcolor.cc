
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
 * WRspice Circuit Simulation and Analysis Tool                           *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

/**************************************************************************
 *
 * Popup to set plotting colors
 *
 **************************************************************************/

#include "config.h"
#include "graph.h"
#include "simulator.h"
#include "cshell.h"
#include "commands.h"
#include "gtktoolb.h"
#ifdef HAVE_MOZY
#include "help/help_defs.h"
#endif

// This is for the Xrm stuff
#include <gdk/gdkprivate.h>
#ifdef WITH_X11
#include "gtkinterf/gtkx11.h"
#include <X11/Xresource.h>
#ifdef HAVE_PWD_H
#include <pwd.h>
#endif
#endif

// Keywords referenced in help database:
//  color

namespace {
    void clr_cancel_proc(GtkWidget*, void*);
    void clr_help_proc(GtkWidget*, void*);
}


void
GTKtoolbar::PopUpColors(int x, int y)
{
    if (co_shell)
        return;
    co_shell = gtk_NewPopup(0, "Plot Colors", clr_cancel_proc, 0);
    if (x || y) {
        FixLoc(&x, &y);
        gtk_window_move(GTK_WINDOW(co_shell), x, y);
    }

    GtkWidget *form = gtk_table_new(2, 1, false);
    gtk_widget_show(form);
    gtk_container_add(GTK_CONTAINER(co_shell), form);

    GtkWidget *hbox = gtk_hbox_new(false, 0);
    gtk_widget_show(hbox);

    GtkWidget *label = gtk_label_new("Plotting Colors");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 0);

    GtkWidget *frame = gtk_frame_new(0);
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), label);
    gtk_box_pack_start(GTK_BOX(hbox), frame, false, false, 0);

    GtkWidget *button = gtk_button_new_with_label("Dismiss");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(clr_cancel_proc), co_shell);
    gtk_box_pack_start(GTK_BOX(hbox), button, true, true, 2);

    button = gtk_button_new_with_label("Help");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(clr_help_proc), 0);
    gtk_box_pack_start(GTK_BOX(hbox), button, true, true, 2);

    gtk_table_attach(GTK_TABLE(form), hbox, 0, 2, 0, 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    for (int i = 0; i < NUMPLOTCOLORS; i++) {
        char buf[64];
        xKWent *entry = static_cast<xKWent*>(KW.color(i));
        if (entry) {
            snprintf(buf, sizeof(buf), "color%d", i);
            VTvalue vv;
            if (!Sp.GetVar(buf, VTYP_STRING, &vv)) {
                const char *s = TB()->XRMgetFromDb(buf);
                if (s && SpGrPkg::DefColorNames[i] != s)
                    SpGrPkg::DefColorNames[i] = s;
            }
            entry->ent = new xEnt(kw_string_func);
            entry->ent->create_widgets(entry, SpGrPkg::DefColorNames[i]);

            gtk_table_attach(GTK_TABLE(form), entry->ent->frame, i&1, (i&1) + 1,
                i/2 + 1, i/2 + 2,
                (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
                (GtkAttachOptions)0, 2, 2);
        }
    }
    gtk_window_set_transient_for(GTK_WINDOW(co_shell),
        GTK_WINDOW(context->Shell()));
    gtk_widget_show(co_shell);
    SetActive(ntb_colors, true);
}


void
GTKtoolbar::PopDownColors()
{
    if (!co_shell)
        return;
    SetLoc(ntb_colors, co_shell);

    GTKdev::Deselect(tb_colors);
    g_signal_handlers_disconnect_by_func(G_OBJECT(co_shell),
        (gpointer)clr_cancel_proc, co_shell);

    for (int i = 0; KW.color(i)->word; i++) {
        xKWent *ent = static_cast<xKWent*>(KW.color(i));
        if (ent->ent) {
            if (ent->ent->entry) {
                const char *str =
                    gtk_entry_get_text(GTK_ENTRY(ent->ent->entry));
                delete [] ent->lastv1;
                ent->lastv1 = lstring::copy(str);
            }
            if (ent->ent->entry2) {
                const char *str =
                    gtk_entry_get_text(GTK_ENTRY(ent->ent->entry2));
                delete [] ent->lastv2;
                ent->lastv2 = lstring::copy(str);
            }
            delete ent->ent;
            ent->ent = 0;
        }
    }
    gtk_widget_destroy(co_shell);
    co_shell = 0;
    SetActive(ntb_colors, false);
}


void
GTKtoolbar::UpdateColors(const char *s)
{
    if (!co_shell)
        return;
    const char *sub = "olor";
    for (const char *t = s+1; *t; t++) {
        if (*t == *sub && (*(t-1) == 'c' || *(t-1) == 'C')) {
            const char *u;
            for (u = sub; *u; u++)
                if (!*t || (*u != *t++))
                    break;
            if (!*u) {
                if (!isdigit(*t))
                    continue;
                int i;
                for (i = 0; isdigit(*t); t++)
                    i = 10*i + *t - '0';
                while (isspace(*t))
                    t++;
                if (*t != ':')
                    continue;
                t++;
                while (isspace(*t))
                    t++;
                char buf[128];
                char *v = buf;
                while (isalpha(*t) || isdigit(*t) || *t == '_')
                    *v++ = *t++;
                *v = '\0';
                if (i >= 0 || i < NUMPLOTCOLORS) {
                    xKWent *entry = static_cast<xKWent*>(KW.color(i));
                    if (entry->ent && entry->ent->active) {
                        bool state =
                            GTKdev::GetStatus(entry->ent->active);;
                        if (!state)
                            gtk_entry_set_text(GTK_ENTRY(entry->ent->entry),
                                buf);
                    }
                }
            }
        }
    }
}


namespace {
    void
    clr_cancel_proc(GtkWidget*, void*)
    {
        TB()->PopDownColors();
    }


    void
    clr_help_proc(GtkWidget*, void*)
    {
#ifdef HAVE_MOZY
        HLP()->word("color");
#endif
    }
}


//=========================================================================
//
// Xrm interface functions - deprecated

// Called from SetDefaultColors() in grsetup.cc.
//
void
GTKtoolbar::LoadResourceColors()
{
#ifdef WITH_X11
    // load resource values into DefColorNames
    static bool doneit;
    if (!doneit) {
        // obtain the database
        doneit = true;
        passwd *pw = getpwuid(getuid());
        if (pw) {
            char buf[512];
            snprintf(buf, sizeof(buf), "%s/%s", pw->pw_dir, "WRspice");
            if (access(buf, R_OK)) {
                snprintf(buf, sizeof(buf), "%s/%s", pw->pw_dir, "Wrspice");
                if (access(buf, R_OK))
                    return;
            }
            XrmDatabase rdb = XrmGetFileDatabase(buf);
            XrmSetDatabase(gr_x_display(), rdb);
        }
    }

    char name[64], clss[64];
    const char *s = strrchr(CP.Program(), '/');
    if (s)
        s++;
    else
        s = CP.Program();
    snprintf(name, sizeof(name), "%s.color", s);
    snprintf(clss, sizeof(clss), "%s.Color", s);
    if (islower(*clss))
        *clss = toupper(*clss);
    char *n = name + strlen(name);
    char *c = clss + strlen(clss);
    XrmDatabase db = XrmGetDatabase(gr_x_display());
    for (int i = 0; i < NUMPLOTCOLORS; i++) {
        snprintf(n, 4, "%d", i);
        snprintf(c, 4, "%d", i);
        char *ss;
        XrmValue v;
        if (XrmGetResource(db, name, clss, &ss, &v))
            SpGrPkg::DefColorNames[i] = v.addr;
    }
    if (clss[0] == 'W' && clss[1] == 'r') {
        // might as well let "WRspice" work, too
        clss[1] = 'R';
        for (int i = 0; i < NUMPLOTCOLORS; i++) {
            snprintf(n, 4, "%d", i);
            snprintf(c, 4, "%d", i);
            char *ss;
            XrmValue v;
            if (XrmGetResource(db, name, clss, &ss, &v))
                SpGrPkg::DefColorNames[i] = v.addr;
        }
    }
#endif
}


const char*
GTKtoolbar::XRMgetFromDb(const char *rname)
{
#ifdef WITH_X11
    XrmDatabase database = XrmGetDatabase(gr_x_display());
    if (!database)
        return (0);
    const char *s = strrchr(CP.Program(), '/');
    if (s)
        s++;
    else
        s = CP.Program();
    char name[64], clss[64];
    snprintf(name, sizeof(name), "%s.%s", s, rname);
    snprintf(clss, sizeof(clss), "%c%s.%c%s", toupper(*s), s+1,
        toupper(*rname), rname+1);
    char *type;
    XrmValue value;
    if (XrmGetResource(database, name, clss, &type, &value))
        return ((const char*)value.addr);
    if (clss[0] == 'W' && clss[1] == 'r') {
        // might as well let "WRspice" work, too
        clss[1] = 'R';
        if (XrmGetResource(database, name, clss, &type, &value))
            return ((const char*)value.addr);
    }
#else
    (void)rname;
#endif
    return (0);
}
// End of GTKtoolbar functions.


// The setrdb command, allows setting X resources from within running
// program.
//
void
CommandTab::com_setrdb(wordlist *wl)
{
#ifdef WITH_X11
    if (!CP.Display()) {
        GRpkg::self()->ErrPrintf(ET_ERROR, "X system not available.\n");
        return;
    }
    char *str = wordlist::flatten(wl);
    XrmDatabase db = XrmGetDatabase(gr_x_display());
    CP.Unquote(str);
    XrmPutLineResource(&db, str);
    TB()->UpdateColors(str);
    delete [] str;
#else
    GRpkg::self()->ErrPrintf(ET_ERROR, "X system not available.\n");
    (void)wl;
#endif
}


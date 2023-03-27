
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

#include "gtkinterf.h"
#include "gtkspinbtn.h"
#include "gtkfont.h"
#include <gdk/gdkkeysyms.h>
#include <locale.h>


// A class to implement a decent spin button.  In GTK-2, this just
// wraps the gtkspinbutton.  The native gtkspinbutton in GTK-1 uses
// float values, which cause roundoff problems, so we don't use it. 
// The same functionality is implemented here.
//
// We also provide an exponential-notation spin-button mode.

GTKspinBtn::GTKspinBtn()
{
    sb_cont = 0;
    sb_entry = 0;
    sb_up = 0;
    sb_dn = 0;
    sb_class = 0;
    sb_value = 0.0;
    sb_minv = 0.0;
    sb_maxv = 0.0;
    sb_del = 0.0;
    sb_pgsize = 0.0;
    sb_rate = 0.0;
    sb_incr = 0.0;
    sb_numd = 0;
    sb_dly_timer = 0;
    sb_timer = 0;
    sb_timer_calls = 0;
    sb_ev_time = 0;
    sb_wrap = false;
    sb_widget = 0;
    sb_func = 0;
    sb_arg = 0;
    sb_snap = false;
    sb_mode = sbModePass;
}


// Count delay, milliseconds.
#define SB_TIME_DLY 200

// Count repeat, milliseconds.
#define SB_TIME_TICK 100

namespace {
    const char *uparr_xpm[] = {
        // width height ncolors chars_per_pixel
        "8 5 2 1",
        // colors
        "   c none",
        ".  c black",
        // pixels
        "    .   ",
        "   ...  ",
        "  ..... ",
        " .......",
        "        "
    };

    const char *dnarr_xpm[] = {
        // width height ncolors chars_per_pixel
        "8 5 2 1",
        // colors
        "   c none",
        ".  c black",
        // pixels
        " .......",
        "  ..... ",
        "   ...  ",
        "    .   ",
        "        "
    };
}


GtkWidget *
GTKspinBtn::init(double val, double minv, double maxv, int numd)
{
    if (numd < 0 )
        numd = 0;
    if (numd > 6)
        numd = 6;
    if (val < minv)
        val = minv;
    else if (val > maxv)
        val = maxv;

    double del = 1.001;  // button 1 click increment
    switch (numd) {
    case 1:
        del = 0.1001;
        break;
    case 2:
        del = 0.01001;
        break;
    case 3:
        del = 0.001001;
        break;
    case 4:
        del = 0.0001001;
        break;
    case 5:
        del = 0.00001001;
        break;
    case 6:
        del = 0.000001001;
        break;
    }
    double pgsz = 1.0;              // button 2 click increment
    double climb_rate = 10.0*del;

    sb_value = val;
    sb_minv = minv;
    sb_maxv = maxv;
    sb_del = del;
    sb_pgsize = pgsz;
    sb_rate = climb_rate;
    sb_numd = numd;

    if (sb_mode != sbModePass) {
        sb_cont = gtk_hbox_new(false, 0);
        gtk_widget_show(sb_cont);

        sb_entry = gtk_entry_new();
        gtk_widget_show(sb_entry);
        gtk_widget_add_events(sb_entry, GDK_KEY_RELEASE_MASK);
        g_signal_connect(G_OBJECT(sb_entry), "focus-out-event",
            G_CALLBACK(sb_focus_out_proc), this);
        g_signal_connect(G_OBJECT(sb_entry), "key-press-event",
            G_CALLBACK(sb_key_press_proc), this);
        g_signal_connect(G_OBJECT(sb_entry), "key-release-event",
            G_CALLBACK(sb_key_release_proc), this);
        gtk_box_pack_start(GTK_BOX(sb_cont), sb_entry, true, true, 0);

        char buf[64];
        printnum(buf, sb_numd, sb_value);

        GtkWidget *vbox = gtk_vbox_new(true, 0);
        gtk_widget_show(vbox);

        sb_up = new_pixmap_button(uparr_xpm, 0, false);  
        gtk_widget_set_name(sb_up, "up");
        gtk_widget_show(sb_up);
        g_signal_connect(G_OBJECT(sb_up), "button-press-event",
            G_CALLBACK(sb_btndn_proc), this);
        g_signal_connect(G_OBJECT(sb_up), "button-release-event",
            G_CALLBACK(sb_btnup_proc), this);
        gtk_box_pack_start(GTK_BOX(vbox), sb_up, true, true, 0);

        sb_dn = new_pixmap_button(dnarr_xpm, 0, false);
        gtk_widget_set_name(sb_dn, "dn");
        gtk_widget_show(sb_dn);
        g_signal_connect(G_OBJECT(sb_dn), "button-press-event",
            G_CALLBACK(sb_btndn_proc), this);
        g_signal_connect(G_OBJECT(sb_dn), "button-release-event",
            G_CALLBACK(sb_btnup_proc), this);
        gtk_box_pack_start(GTK_BOX(vbox), sb_dn, true, true, 0);

        gtk_box_pack_start(GTK_BOX(sb_cont), vbox, false, false, 0);
        set_value(sb_value);
        return (sb_cont);
    }
#if GTK_CHECK_VERSION(3,0,0)
    GtkAdjustment *adj = gtk_adjustment_new(val, minv, maxv, del, pgsz, 0);
#else
    GtkAdjustment *adj = (GtkAdjustment*)gtk_adjustment_new(val, minv, maxv,
        del, pgsz, 0);
#endif
    sb_widget = gtk_spin_button_new(adj, climb_rate, numd);
    gtk_widget_show(sb_widget);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(sb_widget), true);
    return (sb_widget);
}


// Return the total widget width needed to display str.
//
int
GTKspinBtn::width_for_string(const char *str)
{
    if (sb_mode != sbModePass)
        return (GTKfont::stringWidth(sb_entry, str) + 22);
    return (GTKfont::stringWidth(sb_widget, str) + 22);
}


// Set up a text-changed handler.
//
void
GTKspinBtn::connect_changed(GCallback func, void *arg, const char *id)
{
    if (sb_mode != sbModePass) {
        if (id)
            gtk_widget_set_name(sb_entry, id);
        sb_arg = arg;
        sb_func = (SbSignalFunc)func;
    }
    else {
        if (id)
            gtk_widget_set_name(sb_widget, id);
        sb_arg = arg;
        sb_func = (SbSignalFunc)func;
        GtkAdjustment *adj = gtk_spin_button_get_adjustment(
            GTK_SPIN_BUTTON(sb_widget));
        g_signal_connect(G_OBJECT(adj), "value-changed",
            G_CALLBACK(sb_val_changed), this);
    }
}


void
GTKspinBtn::set_min(double min)
{
    if (sb_mode != sbModePass) {
        if (sb_minv != min && min < sb_maxv) {
            sb_minv = min;
            set_value(atof(gtk_entry_get_text(GTK_ENTRY(sb_entry))));
        }
    }
    else {
        GtkAdjustment *adj =
            gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(sb_widget));
        if (gtk_adjustment_get_lower(adj) != min &&
                min < gtk_adjustment_get_upper(adj)) {
            gtk_adjustment_set_lower(adj, min);
            set_value(atof(gtk_entry_get_text(GTK_ENTRY(sb_widget))));
        }
    }
}


void
GTKspinBtn::set_max(double max)
{
    if (sb_mode != sbModePass) {
        if (sb_maxv != max && max > sb_minv) {
            sb_maxv = max;
            set_value(atof(gtk_entry_get_text(GTK_ENTRY(sb_entry))));
        }
    }
    else {
        GtkAdjustment *adj =
            gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(sb_widget));
        if (gtk_adjustment_get_upper(adj) != max &&
                max > gtk_adjustment_get_lower(adj)) {
            gtk_adjustment_set_upper(adj, max);
            set_value(atof(gtk_entry_get_text(GTK_ENTRY(sb_widget))));
        }
    }
}


void
GTKspinBtn::set_delta(double del)
{
    if (sb_mode != sbModePass) {
        sb_del = del;
        sb_pgsize = 10.0*del;
        sb_rate = 10.0*del;
        set_value(atof(gtk_entry_get_text(GTK_ENTRY(sb_entry))));
    }
    else {
        gtk_spin_button_set_increments(GTK_SPIN_BUTTON(sb_widget), del,
            10.0*del);
    }
}


// When set, the numbers are forced to be an integer number of deltas
// from the minimum.
//
void
GTKspinBtn::set_snap(bool snap)
{
    sb_snap = snap;
    if (sb_mode != sbModePass)
        set_value(atof(gtk_entry_get_text(GTK_ENTRY(sb_entry))));
    else {
        // Alas, the stock gtk in Red Hat EL5 doesn't have this.
        // gtk_spin_button_set_snap_to_tics(GTK_SPIN_BUTTON(sb_widget), snap);
        set_value(atof(gtk_entry_get_text(GTK_ENTRY(sb_widget))));
    }
}


void
GTKspinBtn::set_digits(int numd)
{
    if (sb_mode != sbModePass) {
        if (numd >= 0 && numd <= 6 && numd != sb_numd) {
            sb_numd = numd;
            set_value(atof(gtk_entry_get_text(GTK_ENTRY(sb_entry))));
        }
    }
    else {
        if (numd >= 0 && numd <= 6)
            gtk_spin_button_set_digits(GTK_SPIN_BUTTON(sb_widget), numd);
    }
}


void
GTKspinBtn::set_wrap(bool wrap)
{
    if (sb_mode != sbModePass)
        sb_wrap = wrap;
    else
        gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(sb_widget), wrap);
}


void
GTKspinBtn::set_editable(bool ed)
{
    if (sb_mode != sbModePass)
        gtk_editable_set_editable(GTK_EDITABLE(sb_entry), ed);
    else
        gtk_editable_set_editable(GTK_EDITABLE(sb_widget), ed);
}


void
GTKspinBtn::set_sensitive(bool sens, bool clear)
{
    if (sb_mode != sbModePass) {
        if (!sens) {
            // May not get up event, so stop timers.
            if (sb_dly_timer) {
                g_source_remove(sb_dly_timer);
                sb_dly_timer = 0;
            }
            if (sb_timer) {
                g_source_remove(sb_timer);
                sb_timer = 0;
            }
        }
        gtk_widget_set_sensitive(sb_cont, sens);
        if (!sens && clear)
            gtk_entry_set_text(GTK_ENTRY(sb_entry), "");
    }
    else {
        gtk_widget_set_sensitive(sb_widget, sens);
        if (!sens && clear)
            gtk_entry_set_text(GTK_ENTRY(sb_widget), "");
    }
}


// Return the text from the entry area.  Internal data, don't free!
//
const char *
GTKspinBtn::get_string()
{
    if (sb_mode != sbModePass)
        return (gtk_entry_get_text(GTK_ENTRY(sb_entry)));
    else
        return (gtk_entry_get_text(GTK_ENTRY(sb_widget)));
}


int
GTKspinBtn::get_value_as_int()
{
    if (sb_mode != sbModePass) {
        const char *val = gtk_entry_get_text(GTK_ENTRY(sb_entry));
        double nv = atoi(val);
        if (nv >= 0.0)
            return ((int)(nv + 0.5));
        return ((int)(nv - 0.5));
    }
    else
        return (gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(sb_widget)));
}


double
GTKspinBtn::get_value()
{
    if (sb_mode != sbModePass) {
        const char *val = gtk_entry_get_text(GTK_ENTRY(sb_entry));
        return (atof(val));
    }
    else {
        const char *val = gtk_entry_get_text(GTK_ENTRY(sb_widget));
        return (atof(val));
    }
}


void
GTKspinBtn::set_value(double val)
{
    if (sb_mode != sbModePass) {
        if (val < sb_minv)
            val = sb_minv;
        else if (val > sb_maxv)
            val = sb_maxv;

        char buf1[32], buf2[32];
        if (sb_snap) {
            double dval = val - sb_minv;
            int n;
            if (dval >= 0.0)
                n = (int)((dval + sb_del/2.0)/sb_del);
            else
                n = (int)((dval - sb_del/2.0)/sb_del);
            double nval = sb_minv + n*sb_del;
            printnum(buf1, sb_numd, val);
            printnum(buf2, sb_numd, nval);
            if (strcmp(buf1, buf2)) {
                val = nval;
                if (val > sb_maxv)
                    val -= sb_del;
            }
        }

        printnum(buf1, sb_numd, val);
        double nv = atof(buf1);
        const char *t = gtk_entry_get_text(GTK_ENTRY(sb_entry));
        if (strcmp(t, buf1) || nv != sb_value) {
            sb_value = nv;
            gtk_entry_set_text(GTK_ENTRY(sb_entry), buf1);
            if (sb_func)
                (*sb_func)(sb_entry, sb_arg);
        }
    }
    else {
        const char *t = gtk_entry_get_text(GTK_ENTRY(sb_widget));
        char buf[64];
        printnum(buf, gtk_spin_button_get_digits(GTK_SPIN_BUTTON(sb_widget)),
            val);
        if (strcmp(t, buf)) {
            GtkAdjustment *adj =
                gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(sb_widget));
            if (gtk_adjustment_get_value(adj) != val)
                gtk_spin_button_set_value(GTK_SPIN_BUTTON(sb_widget), val);
            else {
                gtk_entry_set_text(GTK_ENTRY(sb_widget), buf);
                if (sb_func)
                    (*sb_func)(sb_widget, sb_arg);
            }
        }
    }
}


// Return true and the value if str is an appropriate string for the
// spin button.
//
bool
GTKspinBtn::is_valid(const char *str, double *pval)
{
    if (!str)
        return (false);
    if (sb_mode != sbModePass) {
        double val;
        if (sscanf(str, "%lf", &val) == 1 &&
                val >= sb_minv && val <= sb_maxv) {
            if (pval)
                *pval = val;
            return (true);
        }
        return (false);
    }
    else {
        GtkAdjustment *adj = gtk_spin_button_get_adjustment(
            GTK_SPIN_BUTTON(sb_widget));
        if (!adj)
            return (false);
        double val;
        if (sscanf(str, "%lf", &val) == 1 &&
                val >= gtk_adjustment_get_lower(adj) &&
                val <= gtk_adjustment_get_upper(adj)) {
            if (pval)
                *pval = val;
            return (true);
        }
        return (false);
    }
}


void
GTKspinBtn::printnum(char *buf, int numd, double value)
{
    if (sb_mode == sbModeE)
        sprintf(buf, "%.*e", numd, value);
    else
        sprintf(buf, "%.*f", numd, value);
}


// Compute the new value after an event.
//
double
GTKspinBtn::bump()
{
    if (sb_mode != sbModeE)
        return (sb_value + sb_incr);
    double d = sb_value;
    bool neg = false;
    if (d < 0) {
        neg = true;
        d = -d;
    }
    double logd = log10(d);
    int ex = (int)floor(logd);
    logd -= ex;
    double mant = pow(10.0, logd);

    if (!neg)
        mant += sb_incr;
    else {
        if (mant - sb_incr < 1.0)
            mant = 1.0 - (1.0 - mant + sb_incr)/10;
        else
            mant -= sb_incr;
    }
    d = mant * pow(10.0, ex);
    if (neg)
        d = -d;
    if (d < sb_minv)
        d = sb_minv;
    else if (d > sb_maxv)
        d = sb_maxv;
    return (d);
}


int
GTKspinBtn::sb_timeout_proc(void *arg)
{
    GTKspinBtn *sb = (GTKspinBtn*)arg;
    if (sb->sb_dly_timer) {
        sb->sb_dly_timer = 0;
        sb->sb_timer = g_timeout_add(SB_TIME_TICK, sb_timeout_proc, arg);
        sb->sb_timer_calls = 0;
        return (0);
    }
    sb->set_value(sb->bump());
    if (sb->sb_rate > 0.0 && fabs(sb->sb_incr) < sb->sb_pgsize) {
        if (sb->sb_timer_calls < 5)
            sb->sb_timer_calls++;
        else {
            sb->sb_timer_calls = 0;
            if (sb->sb_incr > 0)
                sb->sb_incr += sb->sb_rate;
            else
                sb->sb_incr -= sb->sb_rate;
        }
    }
    return (1);
}


int
GTKspinBtn::sb_btndn_proc(GtkWidget *widget, GdkEventButton *event, void *arg)
{
    GTKspinBtn *sb = (GTKspinBtn*)arg;

    gtk_widget_grab_focus(widget);
    const char *name = gtk_widget_get_name(widget);
    if (!strcmp(name, "up")) {
        if (!sb->sb_dly_timer)
            sb->sb_dly_timer =
                g_timeout_add(SB_TIME_DLY, sb_timeout_proc, arg);
        if (event->button == 1)
            sb->sb_incr = sb->sb_del;
        else if (event->button == 2)
            sb->sb_incr = sb->sb_pgsize;
        if (sb->sb_wrap && sb->sb_value == sb->sb_maxv)
            sb->set_value(sb->sb_minv);
        else
            sb->set_value(sb->bump());
    }
    else {
        if (!sb->sb_dly_timer)
            sb->sb_dly_timer =
                g_timeout_add(SB_TIME_DLY, sb_timeout_proc, arg);
        if (event->button == 1)
            sb->sb_incr = -sb->sb_del;
        else if (event->button == 2)
            sb->sb_incr = -sb->sb_pgsize;
        if (sb->sb_wrap && sb->sb_value == sb->sb_minv)
            sb->set_value(sb->sb_maxv);
        else
            sb->set_value(sb->bump());
    }
    return (1);
}


int
GTKspinBtn::sb_btnup_proc(GtkWidget*, GdkEventButton*, void *arg)
{
    GTKspinBtn *sb = (GTKspinBtn*)arg;
    if (sb->sb_dly_timer) {
        g_source_remove(sb->sb_dly_timer);
        sb->sb_dly_timer = 0;
    }
    if (sb->sb_timer) {
        g_source_remove(sb->sb_timer);
        sb->sb_timer = 0;
    }
    return (1);
}


int
GTKspinBtn::sb_key_press_proc(GtkWidget *widget, GdkEventKey *event, void *arg)
{
    GTKspinBtn *sb = (GTKspinBtn*)arg;
    int key = event->keyval;

    bool key_repeat = (event->time == sb->sb_ev_time);

    if (key == GDK_KEY_Up || key == GDK_KEY_Down ||
            key == GDK_KEY_Page_Up || key == GDK_KEY_Page_Down)
        sb->set_value(atof(gtk_entry_get_text(GTK_ENTRY(sb->sb_entry))));

    switch (key) {
    case GDK_KEY_Up:
        if (gtk_widget_has_focus(widget)) {
            g_signal_stop_emission_by_name(G_OBJECT(widget),
                "key_press_event");
            if (!key_repeat)
                sb->sb_incr = sb->sb_del;
            if (sb->sb_wrap && sb->sb_value == sb->sb_maxv)
                sb->set_value(sb->sb_minv);
            else
                sb->set_value(sb->sb_value + sb->sb_incr);
            if (key_repeat) {
                if (sb->sb_rate > 0.0 && sb->sb_incr < sb->sb_pgsize) {
                    if (sb->sb_timer_calls < 5)
                        sb->sb_timer_calls++;
                    else {
                        sb->sb_timer_calls = 0;
                        sb->sb_incr += sb->sb_rate;
                    }
                }
            }
            return (1);
        }
        return (0);

    case GDK_KEY_Down:
        if (gtk_widget_has_focus(widget)) {
            g_signal_stop_emission_by_name(G_OBJECT(widget),
                "key_press_event");
            if (!key_repeat)
                sb->sb_incr = sb->sb_del;
            if (sb->sb_wrap && sb->sb_value == sb->sb_minv)
                sb->set_value(sb->sb_maxv);
            else
                sb->set_value(sb->sb_value - sb->sb_incr);
            if (key_repeat) {
                if (sb->sb_rate > 0.0 && sb->sb_incr < sb->sb_pgsize) {
                    if (sb->sb_timer_calls < 5)
                        sb->sb_timer_calls++;
                    else {
                        sb->sb_timer_calls = 0;
                        sb->sb_incr += sb->sb_rate;
                    }
                }
            }
            return (1);
        }
        return (0);

    case GDK_KEY_Page_Up:
        if (event->state & GDK_CONTROL_MASK) {
            double diff = sb->sb_maxv - sb->sb_value;
            if (diff > 1e-5)
                sb->set_value(sb->sb_value + diff);
        }
        else {
            if (sb->sb_wrap && sb->sb_value == sb->sb_maxv)
                sb->set_value(sb->sb_minv);
            else
                sb->set_value(sb->sb_value + sb->sb_pgsize);
        }
        return (1);

    case GDK_KEY_Page_Down:
        if (event->state & GDK_CONTROL_MASK) {
            double diff = sb->sb_value - sb->sb_minv;
            if (diff > 1e-5)
                sb->set_value(sb->sb_value - diff);
        }
        else {
            if (sb->sb_wrap && sb->sb_value == sb->sb_minv)
                sb->set_value(sb->sb_maxv);
            else
                sb->set_value(sb->sb_value - sb->sb_pgsize);
        }
        return (1);

    default:
        break;
    }
    return (0);
}


int
GTKspinBtn::sb_key_release_proc(GtkWidget*, GdkEventKey *event, void *arg)
{
    GTKspinBtn *sb = (GTKspinBtn*)arg;
    sb->sb_ev_time = event->time;
    return (1);
}


int
GTKspinBtn::sb_focus_out_proc(GtkWidget*, GdkEvent*, void *arg)
{
    GTKspinBtn *sb = (GTKspinBtn*)arg;
    sb->set_value(atof(gtk_entry_get_text(GTK_ENTRY(sb->sb_entry))));
    return (0);
}


void
GTKspinBtn::sb_val_changed(GtkAdjustment*, void *arg)
{
    GTKspinBtn *sb = (GTKspinBtn*)arg;

    if (sb->sb_snap) {
        double val = gtk_spin_button_get_value(GTK_SPIN_BUTTON(sb->sb_widget));
        double min, max;
        gtk_spin_button_get_range(GTK_SPIN_BUTTON(sb->sb_widget), &min, &max);
        double del, pg;
        gtk_spin_button_get_increments(GTK_SPIN_BUTTON(sb->sb_widget),
            &del, &pg);
        int nd = gtk_spin_button_get_digits(GTK_SPIN_BUTTON(sb->sb_widget));

        double dval = val - min;
        int n;
        if (dval >= 0.0)
            n = (int)((dval + del/2.0)/del);
        else
            n = (int)((dval - del/2.0)/del);
        double nval = min + n*del;
        char buf1[32], buf2[32];
        sb->printnum(buf1, nd, val);
        sb->printnum(buf2, nd, nval);
        if (strcmp(buf1, buf2)) {
            if (nval > max)
                nval -= del;
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(sb->sb_widget), nval);
            return;
        }
    }

    if (sb->sb_func)
        (*sb->sb_func)(sb->sb_widget, sb->sb_arg);
}


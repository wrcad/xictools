
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2010 Whiteley Research Inc, all rights reserved.        *
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
 $Id: gtkspinbtn.h,v 2.9 2015/07/31 22:37:01 stevew Exp $
 *========================================================================*/

#ifndef GTKSPINBTN_H
#define GTKSPINBTN_H


// A class to implement a decent spin button.  In GTK-2, this just
// wraps the gtkspinbutton.  The native gtkspinbutton in GTK-1 uses
// float values, which cause roundoff problems, so we don't use it. 
// The same functionality is implemented here.
//
// We also provide an exponential-notation spin-button mode.

typedef void(*SbSignalFunc)(GtkWidget*, void*);

// Instantiation type:
// sbModePass
//   Just pass to the library spin button.  Used for GTK-2,
//   floating/integer format.
// sbModeF
//   Use our local spin button code, float/integer format, for GTK-1.
// sbModeE
//   Use our local spin button code, exponential format.
//
enum sbMode { sbModePass, sbModeF, sbModeE };

// Instantiate for float/integer format, this will automatically use Pass
// mode in GTK2, local mode in GTK1.
//
struct GTKspinBtn
{
    GTKspinBtn();
    // No destructor needed.

    GtkWidget *init(double, double, double, int);
    int width_for_string(const char*);
    void connect_changed(GtkSignalFunc, void*, const char* = 0);
    void set_min(double);
    void set_max(double);
    void set_delta(double);
    void set_snap(bool);
    void set_digits(int);
    void set_wrap(bool);
    void set_editable(bool);
    void set_sensitive(bool, bool = false);
    const char *get_string();
    int get_value_as_int();
    double get_value();
    void set_value(double);
    bool is_valid(const char*, double* = 0);
    void printnum(char*, int, double);

protected:
    double bump();
    static int sb_timeout_proc(void*);
    static int sb_btndn_proc(GtkWidget*, GdkEventButton*, void*);
    static int sb_btnup_proc(GtkWidget*, GdkEventButton*, void*);
    static int sb_key_press_proc(GtkWidget*, GdkEventKey*, void*);
    static int sb_key_release_proc(GtkWidget*, GdkEventKey*, void*);
    static int sb_focus_out_proc(GtkWidget*, GdkEvent*, void*);

    GtkWidget *sb_cont;
    GtkWidget *sb_entry;
    GtkWidget *sb_up;
    GtkWidget *sb_dn;
    GtkEntryClass *sb_class;

    double sb_value;
    double sb_minv;
    double sb_maxv;
    double sb_del;
    double sb_pgsize;
    double sb_rate;
    double sb_incr;
    int sb_numd;
    int sb_dly_timer;
    int sb_timer;
    int sb_timer_calls;
    unsigned int sb_ev_time;
    bool sb_wrap;

    static void sb_val_changed(GtkAdjustment*, void*);

    GtkWidget *sb_widget;
    SbSignalFunc sb_func;
    void *sb_arg;
    bool sb_snap;
    sbMode sb_mode;
};

// Instantiate this to get an exponential-notation spin button.
//
struct GTKspinBtnExp : public GTKspinBtn
{
    GTKspinBtnExp()
        {
            sb_mode = sbModeE;
        }
};

#endif


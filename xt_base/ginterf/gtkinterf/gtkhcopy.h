
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
 $Id: gtkhcopy.h,v 2.12 2015/10/13 00:18:26 stevew Exp $
 *========================================================================*/

#ifndef GTKHCOPY_H
#define GTKHCOPY_H

namespace gtkinterf {
    // values for textfmt
    enum HCtextType { PlainText, PrettyText, HtmlText, PStimes, PShelv,
        PScentury, PSlucida };

    // Struct to record parameters for hard copy generation
    struct GTKprintPopup
    {
        friend void gtk_bag::HCupdate(HCcb*, GRobject);

        GTKprintPopup();
        ~GTKprintPopup();

        void destroy_widgets();

        // called fron gtk_bag destructor.
        void pop_down_print(gtk_bag *w)
            {
                hc_active = true;
                w->PopUpPrint(hc_caller, 0, HCgraphical);
            }

        static void hc_hcpopup(GRobject, gtk_bag*, HCcb*, HCmode, GRdraw*);
        static void hc_pop_up_text(gtk_bag*, const char*, bool);
#ifdef WIN32
        static void hc_set_printer(gtk_bag*);
#endif
        static void hc_set_format(gtk_bag*, int, bool);

    private:
        static void hc_update_menu(GTKprintPopup*);
        static void hc_menu_proc(GtkWidget*, void*);
        static void hc_formenu_proc(GtkWidget*, void*);
#ifdef WIN32
        static void hc_prntmenu_proc(GtkWidget*, void*);
#endif
        static void hc_pagesize_proc(GtkWidget*, void*);
        static void hc_metric_proc(GtkWidget*, void*);
        static void hc_cancel_proc(GtkWidget*, void*);
        static void hc_frame_proc(GtkWidget*, void*);
        static void hc_port_proc(GtkWidget*, void*);
        static void hc_fit_proc(GtkWidget*, void*);
        static void hc_tofile_proc(GtkWidget*, void*);
        static void hc_legend_proc(GtkWidget*, void*);
        static void hc_auto_proc(GtkWidget*, void*);
        static int hc_key_hdlr(GtkWidget*, GdkEvent*, void*);
        static void hc_go_proc(GtkWidget*, void*);
        static void hc_do_go(gtk_bag*);
        static int hc_printit(const char*, const char*, gtk_bag*);
        static int hc_msg_idle_proc(void*);
        static void hc_proc_hdlr(int, int, void*);
        static void hc_resol_proc(GtkWidget*, void*);
        static void hc_help_proc(GtkWidget*, void*);
        static void hc_mkargv(int*, char**, char*);
        static void hc_checklims(HCdesc*);
        static void hc_set_sens(GTKprintPopup*, unsigned);

        // message pop-up
        static void hc_pop_message(gtk_bag*);
        static int hc_go_idle_proc(void*);
        static void hc_go_cancel_proc(GtkWidget*, void*);
        static void hc_go_abort_proc(GtkWidget*, void*);

        GRobject hc_caller;  // launching button
        GtkWidget *hc_popup;
        GtkWidget *hc_cmdlab;
        GtkWidget *hc_cmdtxtbox;
#ifdef WIN32
        GtkWidget *hc_prntmenu;
#endif
        GtkWidget *hc_wlabel;
        GtkWidget *hc_wid;
        GtkWidget *hc_hlabel;
        GtkWidget *hc_hei;
        GtkWidget *hc_xlabel;
        GtkWidget *hc_left;
        GtkWidget *hc_ylabel;
        GtkWidget *hc_top;
        GtkWidget *hc_portbtn;
        GtkWidget *hc_landsbtn;
        GtkWidget *hc_fitbtn;
        GtkWidget *hc_legbtn;
        GtkWidget *hc_tofbtn;
        GtkWidget *hc_metbtn;
        GtkWidget *hc_fontmenu;
        GtkWidget *hc_fmtmenu;
        GtkWidget *hc_resmenu;
        GtkWidget *hc_pgsmenu;

        HCcb *hc_cb;
        GRdraw *hc_context;
        const char *hc_cmdtext;
        const char *hc_tofilename;
        char *hc_clobber_str;
        int hc_resol;
        int hc_fmt;
        HCmode hc_textmode;
        HCtextType hc_textfmt;
        HClegType hc_legend;
        HCorientFlags hc_orient;
        bool hc_tofile;
        char hc_tofbak;
        bool hc_active;
        bool hc_metric;
        int hc_drvrmask;
        int hc_pgsindex;
        float hc_wid_val;
        float hc_hei_val;
        float hc_lft_val;
        float hc_top_val;

#ifdef WIN32
        char **hc_printers;
        int hc_numprinters;

        static int curprinter;
#endif
    };
}

#endif


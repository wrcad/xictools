
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

#ifndef QTTOOLB_H
#define QTTOOLB_H

#include "qtinterf/qtinterf.h"
#include "toolbar.h"


enum tid_id { tid_toolbar, tid_bug, tid_font, tid_files, tid_circuits,
    tid_plots, tid_plotdefs, tid_colors, tid_vectors, tid_variables,
    tid_shell, tid_simdefs, tid_commands, tid_runops, tid_debug, tid_END };

class QTtbHelpDlg;
class QAction;
class QDialog;

extern inline class QTtoolbar *TB();

// Keep track of the active popup widgets
//
class QTtoolbar : public sToolbar, public QTbag
{
    static QTtoolbar *ptr()
        {
            if (!instancePtr)
                on_null_ptr();
            return (instancePtr);
        }

    static void on_null_ptr();

public:
    friend inline QTtoolbar *TB() { return (QTtoolbar::ptr()); }

    // save a tool state
    struct tbent_t
    {
        tbent_t(const char *n = 0, int i = -1)
        {
            te_action = 0;
            te_dialog = 0;
            te_name = n;
            te_id = i;
            te_x = te_y = 0;
        }

        QAction *action()       const { return (te_action); }
        const char *name()      const { return (te_name); }
        int id()                const { return (te_id); }
        QDialog *dialog()       const { return (te_dialog); }
        int x()                 const { return (te_x); }
        int y()                 const { return (te_y); }
        bool active()           const { return (te_active); }

        void set_action(QAction *a)     { te_action = a; }
        void set_name(const char *n)    { te_name = n; }
        void set_id(int i)              { te_id = i; }
        void set_dialog(QDialog *d)     { te_dialog = d; }
        void set_xy(int ix, int iy)     { te_x = ix; te_y = iy; }
        void set_active(bool b)         { te_active = b; }

    private:
        QAction     *te_action;     // menu button
        QDialog     *te_dialog;     // dialog when active
        const char  *te_name;       // dialog name
        int         te_id;          // dialog id
        bool        te_active;      // show on startup
        int         te_x, te_y;     // dialog last location
    };

    struct tbpoint_t
    {
        int x, y;
    };

    // gtktoolb.cc
    QTtoolbar();

    // Toolbar virtual interface.
    // -------------------------

    // timer/idle utilities
    int RegisterIdleProc(int(*)(void*), void*);
    bool RemoveIdleProc(int);
    int RegisterTimeoutProc(int, int(*)(void*), void*);
    bool RemoveTimeoutProc(int);
    void RegisterBigForeignWindow(unsigned int);

    // command defaults dialog
    // gtkcmds.cc
    void PopUpCmdConfig(ShowMode, int, int);

    // color control dialog
    // gtkcolor.cc
    void PopUpColors(ShowMode, int, int);
    void UpdateColors(const char*);
    void LoadResourceColors();

    // debugging dialog
    // gtkdebug.cc
    void PopUpDebugDefs(ShowMode, int, int);

    // plot defaults dialog
    // gtkpldef.cc
    void PopUpPlotDefs(ShowMode, int, int);

    // shell defaults dialog
    // gtkshell.cc
    void PopUpShellDefs(ShowMode, int, int);

    // simulation defaults dialog
    // gtksim.cc
    void PopUpSimDefs(ShowMode, int, int);

    // misc listing panels and dialogs
    // gtkfte.cc
    void SuppressUpdate(bool);
    void PopUpPlots(ShowMode, int, int);
    void UpdatePlots(int);
    void PopUpVectors(ShowMode, int, int);
    void UpdateVectors(int);
    void PopUpCircuits(ShowMode, int, int);
    void UpdateCircuits();
    void PopUpFiles(ShowMode, int, int);
    void UpdateFiles();
    void PopUpRunops(ShowMode, int, int);
    void UpdateRunops();
    void PopUpVariables(ShowMode, int, int);
    void UpdateVariables();

    // main window and a few more
    void PopUpToolbar(ShowMode, int, int);
    void PopUpBugRpt(ShowMode, int, int);
    void PopUpFont(ShowMode, int, int);
    void PopUpTBhelp(ShowMode, GRobject, GRobject, TBH_type);
    void PopUpNotes();
    void PopUpSpiceErr(bool, const char*);
    void PopUpSpiceMessage(const char*, int, int);
    void PopUpSpiceInfo(const char*);
    void UpdateMain(ResUpdType);
    void CloseGraphicsConnection();

    // --------------------------------
    // End of Toolbar virtual overrides.

    // qttbhelp.cc
    char *KeywordsText(GRobject);
    void KeywordsCleanup(QTtbHelpDlg*);

    void Toolbar();
//XXX    void RevertFocus(GtkWidget*);

    // Return the entry of the named tool, if found.
    //
    static tbent_t *FindEnt(const char *str)
    {
        if (str) {
            // Handle old "trace" keyword, now called "runop".
            if (!strcmp(str, "trace"))
                str = "runop";
            for (tbent_t *tb = tb_entries; tb->name(); tb++) {
                if (!strcmp(str, tb->name()))
                    return (tb);
            }
        }
        return (0);
    }

    // Register that a tool is active, or not.
    //
    static void SetActiveDlg(tid_id id, QDialog *d)
    {
        tb_entries[id].set_dialog(d);
    }

    static void SetLoc(tid_id, QDialog*);
    static void FixLoc(int*, int*);
    static char *ConfigString();

    // gtkcolor.cc
    const char *XRMgetFromDb(const char*);

    bool Saved()            { return (tb_saved); }
    void SetSaved(bool b)   { tb_saved = b; }

    static tbent_t *entries(int ix)
    {
        if (ix >= 0 && ix < tid_END)
            return (tb_entries + ix);
        return (0);
    }

private:
    static void tb_mail_destroy_cb(GReditPopup*);
    static void tb_font_cb(const char*, const char*, void*);

    GReditPopup *tb_mailer;
    QTfontDlg   *tb_fontsel;

    QWidget     *tb_kw_help[TBH_end];
    tbpoint_t   tb_kw_help_pos[TBH_end];

    static tbent_t tb_entries[];

    bool        tb_suppress_update;
    bool        tb_saved;

    static QTtoolbar *instancePtr;
};

#endif


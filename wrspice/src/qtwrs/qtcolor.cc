
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
 * Dialog to set plotting colors
 **************************************************************************/

#include "config.h"
#include "qtcolor.h"
#include "graph.h"
#include "simulator.h"
#include "cshell.h"
#include "commands.h"
#include "qttoolb.h"
#ifdef HAVE_MOZY
#include "help/help_defs.h"
#endif

// This is for the Xrm stuff
#ifdef WITH_X11
#include <gdk/gdkprivate.h>
#include "gtkinterf/gtkx11.h"
#include <X11/Xresource.h>
#ifdef HAVE_PWD_H
#include <pwd.h>
#endif
#endif

#include <QLayout>
#include <QLabel>
#include <QToolButton>
#include <QPushButton>
#include <QLineEdit>
#include <QAction>


/**************************************************************************
 * Color parameter setting dialog.
 Keywords referenced in help database:
   color
 **************************************************************************/

void
QTtoolbar::PopUpColors(ShowMode mode, int x, int y)
{
    if (mode == MODE_OFF) {
        delete QTcolorParamDlg::self();
        return;
    }
    if (QTcolorParamDlg::self())
        return;

    new QTcolorParamDlg(x, y);
    QTcolorParamDlg::self()->show();
}


void
QTtoolbar::UpdateColors(const char *s)
{
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
                    if (entry->qtent() && entry->qtent()->active()) {
                        bool state =
                            QTdev::GetStatus(entry->qtent()->active());
                        if (!state)
                            entry->qtent()->entry()->setText(buf);
                    }
                }
            }
        }
    }
}


//=========================================================================
//
// Xrm interface functions - deprecated

// Called from SetDefaultColors() in grsetup.cc.
//
void
QTtoolbar::LoadResourceColors()
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
QTtoolbar::XRMgetFromDb(const char *rname)
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
// End of XRM support.


QTcolorParamDlg *QTcolorParamDlg::instPtr;

QTcolorParamDlg::QTcolorParamDlg(int xx, int yy)
{
    instPtr = this;

    setWindowTitle(tr("Plot Colors"));
    setAttribute(Qt::WA_DeleteOnClose);

    QMargins qmtop(2, 2, 2, 2);
    QMargins qm;
    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(qmtop);
    vbox->setSpacing(2);

    QHBoxLayout *hbox = new QHBoxLayout();
    vbox->addLayout(hbox);
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);
    
    QGroupBox *gb = new QGroupBox();
    hbox->addWidget(gb);
    QHBoxLayout *hb = new QHBoxLayout(gb);
    hb->setContentsMargins(qmtop);
    hb->setSpacing(2);
    QLabel *label = new QLabel(tr("Plotting Colors"));
    hb->addWidget(label);

    QToolButton *tbtn = new QToolButton();
    tbtn->setText(tr("Help"));
    hbox->addWidget(tbtn);
    connect(tbtn, &QAbstractButton::clicked,
        this, &QTcolorParamDlg::help_btn_slot);

    QGridLayout *grid = new QGridLayout();
    grid->setContentsMargins(qmtop);
    grid->setSpacing(2);
    vbox->addLayout(grid);

    int nc2 = NUMPLOTCOLORS/2;
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
            entry->ent = new QTkwent(KW_NO_SPIN, QTkwent::ke_string_func,
                entry, SpGrPkg::DefColorNames[i]);
            if (i < nc2)
                grid->addWidget(entry->qtent(), i, 0);
            else
                grid->addWidget(entry->qtent(), i-nc2, 1);
        }
    }

    QPushButton *btn = new QPushButton(tr("Dismiss"));
    vbox->addWidget(btn);
    connect(btn, &QAbstractButton::clicked,
        this, &QTcolorParamDlg::dismiss_btn_slot);


    if (xx || yy) {
        TB()->FixLoc(&xx, &yy);
        move(xx, yy);
    }
    TB()->SetActiveDlg(tid_colors, this);
}


QTcolorParamDlg::~QTcolorParamDlg()
{
    instPtr = 0;
    TB()->SetLoc(tid_colors, this);
    TB()->SetActiveDlg(tid_colors, 0);
}


void
QTcolorParamDlg::dismiss_btn_slot()
{
    TB()->PopUpColors(MODE_OFF, 0, 0);
}


void
QTcolorParamDlg::help_btn_slot()
{
#ifdef HAVE_MOZY
    HLP()->word("color");
#endif
}


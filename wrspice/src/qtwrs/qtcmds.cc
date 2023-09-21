
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

#include "config.h"
#include "qtcmds.h"
#include "simulator.h"
#include "spglobal.h"
#include "graph.h"
#include "output.h"
#include "cshell.h"
#include "kwords_fte.h"
#include "kwords_analysis.h"
#include "qttoolb.h"
#include "miscutil/pathlist.h"
#ifdef HAVE_MOZY
#include "help/help_defs.h"
#endif

#include <QLayout>
#include <QTabWidget>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QAction>


/**************************************************************************
 * Command configuration dialog
 **************************************************************************/

// The command parameter setting dialog, initiated from the Tools menu.
//
void
QTtoolbar::PopUpCmdConfig(ShowMode mode, int x, int y)
{
    if (mode == MODE_OFF) {
        if (QTcmdParamDlg::self())
            QTcmdParamDlg::self()->deleteLater();
        return;
    }
    if (QTcmdParamDlg::self())
        return;

    new QTcmdParamDlg(x, y);
    QTcmdParamDlg::self()->show();
}
// End of QTtoolbar functions.


#define STR(x) #x
#define STRINGIFY(x) STR(x)
#define KWGET(string) (xKWent*)sHtab::get(Sp.Options(), string)

namespace {
    inline void dblpr(char *buf, int n, double d, bool ex)
    {
        snprintf(buf, 32, ex ? "%.*e" : "%.*f", n, d);
    }
}


QTcmdParamDlg *QTcmdParamDlg::instPtr;

QTcmdParamDlg::QTcmdParamDlg(int x, int y)
{
    instPtr = this;

    setWindowTitle(tr("Command Options"));
    setAttribute(Qt::WA_DeleteOnClose);

    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setMargin(2);
    vbox->setSpacing(2);

    QHBoxLayout *hbox = new QHBoxLayout();
    vbox->addLayout(hbox);
    hbox->setMargin(0);
    hbox->setSpacing(2);
    
    QGroupBox *gb = new QGroupBox();
    hbox->addWidget(gb);
    QHBoxLayout *hb = new QHBoxLayout(gb);
    hb->setMargin(2);
    hb->setSpacing(2);
    QLabel *label = new QLabel(tr("Command Defaults"));
    hb->addWidget(label);

    QPushButton *btn = new QPushButton(tr("Dismiss"));
    hbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(dismiss_btn_slot()));

    btn = new QPushButton(tr("Help"));
    btn->setCheckable(true);
    hbox->addWidget(btn);
    connect(btn, SIGNAL(toggled(bool)), this, SLOT(help_btn_slot(bool)));

    QTabWidget *notebook = new QTabWidget();
    vbox->addWidget(notebook);

    // General page
    //
    QWidget *page = new QWidget();
    notebook->addTab(page, tr("General"));

    QGridLayout *grid = new QGridLayout(page);
    grid->setMargin(2);
    grid->setSpacing(2);

    gb = new QGroupBox();
    grid->addWidget(gb, 0, 0, 1, 2);
    hb = new QHBoxLayout(gb);
    label = new QLabel(tr("General Parameters"));
    hb->addWidget(label);
    label->setAlignment(Qt::AlignCenter);

    xKWent *entry = KWGET(kw_numdgt);
    if (entry) {
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_int_func, entry,
            STRINGIFY(DEF_numdgt));
        grid->addWidget(entry->qtent(), 1, 0);
        entry->qtent()->setup(DEF_numdgt, 1.0, 0.0, 0.0, 0);
    }

    entry = KWGET(kw_units);
    if (entry) {
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_string_func, entry,
            KW.units(0)->word, KW.KWunits);
        grid->addWidget(entry->qtent(), 1, 1);
    }

    hbox = new QHBoxLayout();
    grid->addLayout(hbox, 2, 0);
    entry = KWGET(kw_dpolydegree);
    if (entry) {
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_int_func, entry,
            STRINGIFY(DEF_dpolydegree));
        hbox->addWidget(entry->qtent());
        entry->qtent()->setup(DEF_dpolydegree, 1.0, 0.0, 0.0, 0);
    }

    entry = KWGET(spkw_dcoddstep);
    if (entry) {
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_bool_func, entry, 0);
        hbox->addWidget(entry->qtent());
    }

    hbox = new QHBoxLayout();
    grid->addLayout(hbox, 2, 1);
    entry = KWGET(kw_dollarcmt);
    if (entry) {
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_bool_func, entry, 0);
        hbox->addWidget(entry->qtent());
    }

    entry = KWGET(kw_nopage);
    if (entry) {
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_bool_func, entry, 0);
        hbox->addWidget(entry->qtent());
    }

    entry = KWGET(kw_random);
    if (entry) {
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_bool_func, entry, 0);
        hbox->addWidget(entry->qtent());
    }

    entry = KWGET(kw_errorlog);
    if (entry) {
        entry->ent = new QTkwent(KW_NO_SPIN, QTkwent::ke_string_func, entry,
            "");
        grid->addWidget(entry->qtent(), 3, 0, 1, 2);
    }

    // aspice page
    //
    page = new QWidget();
    notebook->addTab(page, "aspice");

    grid = new QGridLayout(page);
    grid->setMargin(2);
    grid->setSpacing(2);

    gb = new QGroupBox();
    grid->addWidget(gb, 0, 0);
    hb = new QHBoxLayout(gb);
    label = new QLabel(tr("The aspice Command"));
    hb->addWidget(label);
    label->setAlignment(Qt::AlignCenter);

    entry = KWGET(kw_spicepath);
    if (entry) {
        entry->ent = new QTkwent(KW_NO_SPIN, QTkwent::ke_string_func, entry,
            Global.ExecProg());
        grid->addWidget(entry->qtent(), 1, 0);
    }
    grid->setRowStretch(2, 1);

    // check page
    //
    page = new QWidget();
    notebook->addTab(page, "check");

    grid = new QGridLayout(page);
    grid->setMargin(2);
    grid->setSpacing(2);

    gb = new QGroupBox();
    grid->addWidget(gb, 0, 0);
    hb = new QHBoxLayout(gb);
    label = new QLabel(tr("The check Command"));
    hb->addWidget(label);
    label->setAlignment(Qt::AlignCenter);

    entry = KWGET(kw_checkiterate);
    if (entry) {
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_int_func, entry,
            STRINGIFY(DEF_checkiterate));
        grid->addWidget(entry->qtent(), 1, 0);
        entry->qtent()->setup(DEF_checkiterate, 1.0, 0.0, 0.0, 0);
//XXX        grid->setStretch(1, 1, 1);
    }

    entry = KWGET(kw_mplot_cur);
    if (entry) {
        entry->ent = new QTkwent(KW_NO_SPIN, QTkwent::ke_string_func, entry,
            "");
        grid->addWidget(entry->qtent(), 2, 0);
    }
    grid->setRowStretch(3, 1);

    // diff page
    //
    page = new QWidget();
    notebook->addTab(page, "diff");

    grid = new QGridLayout(page);
    grid->setMargin(2);
    grid->setSpacing(2);

    gb = new QGroupBox();
    grid->addWidget(gb, 0, 0, 1, 2);
    hb = new QHBoxLayout(gb);
    label = new QLabel(tr("The diff Command"));
    hb->addWidget(label);
    label->setAlignment(Qt::AlignCenter);

    char tbuf[64];
    entry = KWGET(kw_diff_abstol);
    if (entry) {
        dblpr(tbuf, 2, DEF_diff_abstol, true);
        entry->ent = new QTkwent(KW_FLOAT, QTkwent::ke_real_func, entry, tbuf);
        grid->addWidget(entry->qtent(), 1, 0);
        entry->qtent()->setup(DEF_diff_abstol, 0.1, 0.0, 0.0, 2);
    }

    entry = KWGET(kw_diff_reltol);
    if (entry) {
        dblpr(tbuf, 2, DEF_diff_reltol, true);
        entry->ent = new QTkwent(KW_FLOAT, QTkwent::ke_real_func, entry, tbuf);
        grid->addWidget(entry->qtent(), 1, 1);
        entry->qtent()->setup(DEF_diff_reltol, 0.1, 0.0, 0.0, 2);
    }

    entry = KWGET(kw_diff_vntol);
    if (entry) {
        dblpr(tbuf, 2, DEF_diff_vntol, true);
        entry->ent = new QTkwent(KW_FLOAT, QTkwent::ke_real_func, entry, tbuf);
        grid->addWidget(entry->qtent(), 2, 0);
        entry->qtent()->setup(DEF_diff_vntol, 0.1, 0.0, 0.0, 2);
    }
    grid->setRowStretch(3, 1);

    // edit page
    //
    page = new QWidget();
    notebook->addTab(page, "edit");

    grid = new QGridLayout(page);
    grid->setMargin(2);
    grid->setSpacing(2);

    gb = new QGroupBox();
    grid->addWidget(gb, 0, 0);
    hb = new QHBoxLayout(gb);
    label = new QLabel(tr("The edit Command"));
    hb->addWidget(label);
    label->setAlignment(Qt::AlignCenter);

    entry = KWGET(kw_editor);
    if (entry) {
        entry->ent = new QTkwent(KW_NO_SPIN, QTkwent::ke_string_func, entry,
            "xeditor");
        grid->addWidget(entry->qtent(), 1, 0);
    }

    entry = KWGET(kw_xicpath);
    if (entry) {
        char *path;
        const char *progname = Global.GfxProgName() && *Global.GfxProgName() ?
            Global.GfxProgName() : "xic";
        const char *prefix = getenv("XT_PREFIX");
        if (!prefix || !lstring::is_rooted(prefix))
            prefix = "/usr/local";
#ifdef WIN32
        char *tpath = pathlist::mk_path("xictools/bin", progname);
#else
        char *tpath = pathlist::mk_path("xictools/xic/bin", progname);
#endif
        path = pathlist::mk_path(prefix, tpath);
        delete [] tpath;
        entry->ent = new QTkwent(KW_NO_SPIN, QTkwent::ke_string_func, entry,
            path);
        delete [] path;
        grid->addWidget(entry->qtent(), 2, 0);
    }
    grid->setRowStretch(3, 1);

    // fourier page
    //
    page = new QWidget();
    notebook->addTab(page, "fourier");

    grid = new QGridLayout(page);
    grid->setMargin(2);
    grid->setSpacing(2);

    gb = new QGroupBox();
    grid->addWidget(gb, 0, 0, 1, 2);
    hb = new QHBoxLayout(gb);
    label = new QLabel(tr("The fourier and spec Commands"));
    hb->addWidget(label);
    label->setAlignment(Qt::AlignCenter);

    entry = KWGET(kw_fourgridsize);
    if (entry) {
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_int_func, entry,
            STRINGIFY(DEF_fourgridsize));
        grid->addWidget(entry->qtent(), 1, 0);
        entry->qtent()->setup(DEF_fourgridsize, 1.0, 0.0, 0.0, 0);
    }

    entry = KWGET(kw_nfreqs);
    if (entry) {
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_int_func, entry,
            STRINGIFY(DEF_nfreqs));
        grid->addWidget(entry->qtent(), 1, 1);
        entry->qtent()->setup(DEF_nfreqs, 1.0, 0.0, 0.0, 0);
    }

    // spec command
    entry = KWGET(kw_specwindow);
    if (entry) {
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_string_func, entry,
            kw_hanning, KW.KWspec);
        grid->addWidget(entry->qtent(), 2, 0);
    }

    entry = KWGET(kw_specwindoworder);
    if (entry) {
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_int_func, entry,
            STRINGIFY(DEF_specwindoworder));
        grid->addWidget(entry->qtent(), 2, 1);
        entry->qtent()->setup(DEF_specwindoworder, 1.0, 0.0, 0.0, 0);
    }

    entry = KWGET(kw_spectrace);
    if (entry) {
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_bool_func, entry, 0);
        grid->addWidget(entry->qtent(), 3, 0);
    }


#ifdef HAVE_MOZY
    // help page
    //
    page = new QWidget();
    notebook->addTab(page, "help");

    grid = new QGridLayout(page);
    grid->setMargin(2);
    grid->setSpacing(2);

    gb = new QGroupBox();
    grid->addWidget(gb, 0, 0, 1, 2);
    hb = new QHBoxLayout(gb);
    label = new QLabel(tr("The help Command"));
    hb->addWidget(label);
    label->setAlignment(Qt::AlignCenter);

    entry = KWGET(kw_helpinitxpos);
    if (entry) {
        snprintf(tbuf, sizeof(tbuf), "%d", HLP()->get_init_x());
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_int_func, entry, tbuf);
        grid->addWidget(entry->qtent(), 1, 0);
        entry->qtent()->setup(HLP()->get_init_x(), 1.0, 0.0, 0.0, 0);
    }

    entry = KWGET(kw_helpinitypos);
    if (entry) {
        snprintf(tbuf, sizeof(tbuf), "%d", HLP()->get_init_y());
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_int_func, entry, tbuf);
        grid->addWidget(entry->qtent(), 1, 1);
        entry->qtent()->setup(HLP()->get_init_y(), 1.0, 0.0, 0.0, 0);
    }

    entry = KWGET(kw_level);
    if (entry) {
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_string_func, entry,
            KW.level(0)->word, KW.KWlevel);
        grid->addWidget(entry->qtent(), 2, 0);
    }

    entry = KWGET(kw_helppath);
    if (entry) {
        char *s;
        get_helppath(&s);
        entry->ent = new QTkwent(KW_NO_SPIN, helppath_func, entry, s);
        delete [] s;
        grid->addWidget(entry->qtent(), 3, 0);
    }
#endif  // HAVE_MOZY

    // print page
    //
    page = new QWidget();
    notebook->addTab(page, "print");

    grid = new QGridLayout(page);
    grid->setMargin(2);
    grid->setSpacing(2);

    gb = new QGroupBox();
    grid->addWidget(gb, 0, 0, 1, 2);
    hb = new QHBoxLayout(gb);
    label = new QLabel(tr("The print Command"));
    hb->addWidget(label);
    label->setAlignment(Qt::AlignCenter);

    entry = KWGET(kw_printautowidth);
    if (entry) {
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_bool_func, entry, 0);
        grid->addWidget(entry->qtent(), 1, 0);
    }

    entry = KWGET(kw_printnoindex);
    if (entry) {
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_bool_func, entry, 0);
        grid->addWidget(entry->qtent(), 2, 0);
    }

    entry = KWGET(kw_printnoscale);
    if (entry) {
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_bool_func, entry, 0);
        grid->addWidget(entry->qtent(), 2, 1);
    }

    entry = KWGET(kw_printnoheader);
    if (entry) {
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_bool_func, entry, 0);
        grid->addWidget(entry->qtent(), 3, 0);
    }

    entry = KWGET(kw_printnopageheader);
    if (entry) {
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_bool_func, entry, 0);
        grid->addWidget(entry->qtent(), 3, 1);
    }

    // rawfile page
    //
    page = new QWidget();
    notebook->addTab(page, "rawfile");

    grid = new QGridLayout(page);
    grid->setMargin(2);
    grid->setSpacing(2);

    gb = new QGroupBox();
    grid->addWidget(gb, 0, 0, 1, 2);
    hb = new QHBoxLayout(gb);
    label = new QLabel(tr("The rawfile Command"));
    hb->addWidget(label);
    label->setAlignment(Qt::AlignCenter);

    entry = KWGET(kw_filetype);
    if (entry) {
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_string_func, entry,
            KW.ft(0)->word, KW.KWft);
        grid->addWidget(entry->qtent(), 1, 0);
    }

    entry = KWGET(kw_rawfileprec);
    if (entry) {
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_int_func, entry,
            STRINGIFY(DEF_rawfileprec));
        entry->qtent()->setup(DEF_rawfileprec, 1.0, 0.0, 0.0, 0);
        grid->addWidget(entry->qtent(), 1, 1);
    }

    entry = KWGET(kw_nopadding);
    if (entry) {
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_bool_func, entry, 0);
        grid->addWidget(entry->qtent(), 2, 0);
    }

    entry = KWGET(kw_rawfile);
    if (entry) {
        entry->ent = new QTkwent(KW_NO_SPIN, QTkwent::ke_string_func, entry,
            OP.getOutDesc()->outFile() ? OP.getOutDesc()->outFile() : "");
        grid->addWidget(entry->qtent(), 3, 0);
    }

    // rspice page
    //
    page = new QWidget();
    notebook->addTab(page, "rspice");

    grid = new QGridLayout(page);
    grid->setMargin(2);
    grid->setSpacing(2);

    gb = new QGroupBox();
    grid->addWidget(gb, 0, 0);
    hb = new QHBoxLayout(gb);
    label = new QLabel(tr("The rspice Command"));
    hb->addWidget(label);
    label->setAlignment(Qt::AlignCenter);

    entry = KWGET(kw_rhost);
    if (entry) {
        entry->ent = new QTkwent(KW_NO_SPIN, QTkwent::ke_string_func, entry,
            "");
        grid->addWidget(entry->qtent(), 1, 0);
    }

    entry = KWGET(kw_rprogram);
    if (entry) {
        entry->ent = new QTkwent(KW_NO_SPIN, QTkwent::ke_string_func, entry,
            CP.Program());
        grid->addWidget(entry->qtent(), 2, 0);
    }

    // source page
    //
    page = new QWidget();
    notebook->addTab(page, "source");

    grid = new QGridLayout(page);
    grid->setMargin(2);
    grid->setSpacing(2);

    gb = new QGroupBox();
    grid->addWidget(gb, 0, 0, 1, 2);
    hb = new QHBoxLayout(gb);
    label = new QLabel(tr("The source Command"));
    hb->addWidget(label);
    label->setAlignment(Qt::AlignCenter);

    entry = KWGET(kw_noprtitle);
    if (entry) {
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_bool_func, entry, 0);
        grid->addWidget(entry->qtent(), 1, 0);
    }
    grid->setRowStretch(2, 1);

    // write page
    //
    page = new QWidget();
    notebook->addTab(page, "write");

    grid = new QGridLayout(page);
    grid->setMargin(2);
    grid->setSpacing(2);

    gb = new QGroupBox();
    grid->addWidget(gb, 0, 0, 1, 2);
    hb = new QHBoxLayout(gb);
    label = new QLabel(tr("The write Command"));
    hb->addWidget(label);
    label->setAlignment(Qt::AlignCenter);

    entry = KWGET(kw_appendwrite);
    if (entry) {
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_bool_func, entry, 0);
        grid->addWidget(entry->qtent(), 1, 0);
    }
    grid->setRowStretch(2, 1);

    if (x || y) {
        TB()->FixLoc(&x, &y);
        move(x, y);
    }
    TB()->SetActive(tid_commands, true);
}


QTcmdParamDlg::~QTcmdParamDlg()
{
    TB()->PopUpTBhelp(MODE_OFF, 0, 0, TBH_CM);
    instPtr = 0;
    TB()->SetLoc(tid_commands, this);
    TB()->SetActive(tid_commands, false);
    QTtoolbar::entries(tid_commands)->action()->setChecked(false);
}


// Static function.
// Return true and set the path if the help path is currently set as a
// variable.  Set path to the default help path in use if the help
// path is not set as a variable, and return false.
//
bool
QTcmdParamDlg::get_helppath(char **path)
{
    VTvalue vv;
    if (Sp.GetVar(kw_helppath, VTYP_STRING, &vv)) {
        *path = lstring::copy(vv.get_string());
        return (true);
    }
    // helppath not found in variable database.  find out what is really
    // being used from the help system.
    //
    *path = 0;
    HLP()->get_path(path);
    if (!*path)
        *path = lstring::copy("");
    return (false);
}


// Static function.
void
QTcmdParamDlg::helppath_func(bool isset, variable*, void *entp)
{
    QTkwent *ent = static_cast<QTkwent*>(entp);
    if (ent->active()) {
        QTdev::SetStatus(ent->active(), isset);
        if (isset) {
            char *s;
            get_helppath(&s);
            ent->entry()->setText(s);
            ent->entry()->setReadOnly(true);
            delete [] s;
        }
        else
            ent->entry()->setReadOnly(false);
    }
}


void
QTcmdParamDlg::dismiss_btn_slot()
{
    TB()->PopUpCmdConfig(MODE_OFF, 0, 0);
}


void
QTcmdParamDlg::help_btn_slot(bool state)
{
    if (state)
        TB()->PopUpTBhelp(MODE_ON, this, sender(), TBH_CM);
    else
        TB()->PopUpTBhelp(MODE_OFF, 0, 0, TBH_CM);
}

//XXX
#ifdef notdef

namespace {
    int cmd_choice_hdlr(GtkWidget *caller, GdkEvent*, void *client_data)
    {
        xKWent *entry = static_cast<xKWent*>(client_data);
        if (GTKdev::GetStatus(entry->ent->active))
            return (true);
        int i;
        if (!strcmp(entry->word, kw_filetype)) {
            const char *string =
                gtk_entry_get_text(GTK_ENTRY(entry->ent->entry));
            for (i = 0; KW.ft(i)->word; i++)
                if (!strcmp(string, KW.ft(i)->word))
                    break;
            if (!KW.ft(i)->word) {
                GRpkg::self()->ErrPrintf(ET_ERROR,
                    "bad filetype found: %s.\n", string);
                i = 0;
            }
            else {
                if (g_object_get_data(G_OBJECT(caller), "down")) {
                    i--;
                    if (i < 0) {
                        i = 0;
                        while (KW.ft(i)->word && KW.ft(i+1)->word)
                            i++;
                    }
                }
                else {
                    i++;
                    if (!KW.ft(i)->word)
                        i = 0;
                }
            }
            gtk_entry_set_text(GTK_ENTRY(entry->ent->entry), KW.ft(i)->word);
        }
        else if (!strcmp(entry->word, kw_level)) {
            const char *string =
                gtk_entry_get_text(GTK_ENTRY(entry->ent->entry));
            for (i = 0; KW.level(i)->word; i++)
                if (!strcmp(string, KW.level(i)->word))
                    break;
            if (!KW.level(i)->word) {
                GRpkg::self()->ErrPrintf(ET_ERROR,
                    "bad level found: %s.\n", string);
                i = 0;
            }
            else {
                if (g_object_get_data(G_OBJECT(caller), "down")) {
                    i--;
                    if (i < 0) {
                        i = 0;
                        while (KW.level(i)->word && KW.level(i+1)->word)
                            i++;
                    }
                }
                else {
                    i++;
                    if (!KW.level(i)->word)
                        i = 0;
                }
            }
            gtk_entry_set_text(GTK_ENTRY(entry->ent->entry),KW.level(i)->word);
        }
        else if (!strcmp(entry->word, kw_specwindow)) {
            const char *string =
                gtk_entry_get_text(GTK_ENTRY(entry->ent->entry));
            for (i = 0; KW.spec(i)->word; i++)
                if (!strcmp(string, KW.spec(i)->word))
                    break;
            if (!KW.spec(i)->word) {
                GRpkg::self()->ErrPrintf(ET_ERROR,
                    "bad specwindow found: %s.\n", string);
                i = 0;
            }
            else {
                if (g_object_get_data(G_OBJECT(caller), "down")) {
                    i--;
                    if (i < 0) {
                        i = 0;
                        while (KW.spec(i)->word && KW.spec(i+1)->word)
                            i++;
                    }
                }
                else {
                    i++;
                    if (!KW.spec(i)->word)
                        i = 0;
                }
            }
            gtk_entry_set_text(GTK_ENTRY(entry->ent->entry), KW.spec(i)->word);
        }
        else if (!strcmp(entry->word, kw_units)) {
            const char *string =
                gtk_entry_get_text(GTK_ENTRY(entry->ent->entry));
            for (i = 0; KW.units(i)->word; i++)
                if (!strcmp(string, KW.units(i)->word))
                    break;
            if (!KW.units(i)->word) {
                GRpkg::self()->ErrPrintf(ET_ERROR,
                    "bad units found: %s.\n", string);
                i = 0;
            }
            else {
                if (g_object_get_data(G_OBJECT(caller), "down")) {
                    i--;
                    if (i < 0) {
                        i = 0;
                        while (KW.units(i)->word && KW.units(i+1)->word)
                            i++;
                    }
                }
                else {
                    i++;
                    if (!KW.units(i)->word)
                        i = 0;
                }
            }
            gtk_entry_set_text(GTK_ENTRY(entry->ent->entry),KW.units(i)->word);
        }
        return (true);
    }
}

#endif

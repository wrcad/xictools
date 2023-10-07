
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
 * Plot defaults popup
 *
 **************************************************************************/

#include "config.h"
#include "qtpldef.h"
#include "graph.h"
#include "cshell.h"
#include "kwords_fte.h"
#include "simulator.h"
#include "qttoolb.h"
#include "spnumber/spnumber.h"

#include <QLayout>
#include <QTabWidget>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QAction>


/**************************************************************************
 * Plot parameter setting dialog.
 **************************************************************************/

// The plot defaults popup, initiated from the toolbar.
//
void
QTtoolbar::PopUpPlotDefs(ShowMode mode, int x, int y)
{
    if (mode == MODE_OFF) {
        if (QTplotParamDlg::self())
            QTplotParamDlg::self()->deleteLater();
        return;
    }
    if (QTplotParamDlg::self())
        return;

    new QTplotParamDlg(x, y);
    QTplotParamDlg::self()->show();
}
// End of QTtoolbar functions.


#define KWGET(string) (xKWent*)sHtab::get(Sp.Options(), string)

Kword **QTplotParamDlg::KWdrvrs;
QTplotParamDlg *QTplotParamDlg::instPtr;

QTplotParamDlg::QTplotParamDlg(int x, int y)
{
    instPtr = this;
    pd_notebook = 0;

    setWindowTitle(tr("Plot Options"));
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
    QLabel *label = new QLabel(tr("Set plot options"));
    hb->addWidget(label);

    QPushButton *btn = new QPushButton(tr("Dismiss"));
    hbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(dismiss_btn_slot()));

    btn = new QPushButton(tr("Help"));
    btn->setCheckable(true);
    hbox->addWidget(btn);
    connect(btn, SIGNAL(toggled(bool)), this, SLOT(help_btn_slot(bool)));

    pd_notebook = new QTabWidget();
    vbox->addWidget(pd_notebook);

    // plot 1 page
    //
    QWidget *page = new QWidget();
    pd_notebook->addTab(page, tr("plot 1"));

    QGridLayout *grid = new QGridLayout(page);
    grid->setMargin(2);
    grid->setSpacing(2);

    gb = new QGroupBox();
    grid->addWidget(gb, 0, 0, 1, 4);
    hb = new QHBoxLayout(gb);
    label = new QLabel(tr("Plot Variables, Page 1"));
    hb->addWidget(label);
    label->setAlignment(Qt::AlignCenter);

    xKWent *entry = KWGET(kw_plotgeom);
    if (entry) {
        entry->ent = new QTkwent(KW_NO_SPIN, QTkwent::ke_string_func, entry, 0);
        grid->addWidget(entry->qtent(), 1, 0, 1, 4);
    }

    entry = KWGET(kw_title);
    if (entry) {
        entry->ent = new QTkwent(KW_NO_SPIN, QTkwent::ke_string_func, entry, 0);
        grid->addWidget(entry->qtent(), 2, 0, 1, 4);
    }

    entry = KWGET(kw_xlabel);
    if (entry) {
        entry->ent = new QTkwent(KW_NO_SPIN, QTkwent::ke_string_func, entry, 0);
        grid->addWidget(entry->qtent(), 3, 0, 1, 4);
    }

    entry = KWGET(kw_ylabel);
    if (entry) {
        entry->ent = new QTkwent(KW_NO_SPIN, QTkwent::ke_string_func, entry, 0);
        grid->addWidget(entry->qtent(), 4, 0, 1, 4);
    }

    entry = KWGET(kw_plotstyle);
    if (entry) {
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_string_func, entry,
            KW.pstyles(0)->word, KW.KWpstyles);
        grid->addWidget(entry->qtent(), 5, 0, 1, 2);
    }

    entry = KWGET(kw_ysep);
    if (entry) {
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_bool_func, entry, 0);
        grid->addWidget(entry->qtent(), 5, 2);
    }

    entry = KWGET(kw_noplotlogo);
    if (entry) {
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_bool_func, entry, 0);
        grid->addWidget(entry->qtent(), 5, 3);
    }

    entry = KWGET(kw_gridstyle);
    if (entry) {
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_string_func, entry,
            KW.gstyles(0)->word, KW.KWgstyles);
        grid->addWidget(entry->qtent(), 6, 0, 1, 2);
    }

    entry = KWGET(kw_nogrid);
    if (entry) {
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_bool_func, entry, 0);
        grid->addWidget(entry->qtent(), 6, 2);
    }

    entry = KWGET(kw_present);
    if (entry) {
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_bool_func, entry, 0);
        grid->addWidget(entry->qtent(), 6, 3);
    }

    entry = KWGET(kw_scaletype);
    if (entry) {
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_string_func, entry,
            KW.scale(0)->word, KW.KWscale);
        grid->addWidget(entry->qtent(), 7, 0, 1, 2);
    }

    entry = KWGET(kw_ticmarks);
    if (entry) {
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_int_func, entry, "10");
        grid->addWidget(entry->qtent(), 7, 2, 1, 2);
        entry->qtent()->setup(10.0, 1.0, 0.0, 0.0, 0);
    }


    // plot 2 page
    //
    page = new QWidget();
    pd_notebook->addTab(page, tr("plot 2"));

    grid = new QGridLayout(page);
    grid->setMargin(2);
    grid->setSpacing(2);

    gb = new QGroupBox();
    grid->addWidget(gb, 0, 0, 1, 2);
    hb = new QHBoxLayout(gb);
    label = new QLabel(tr("Plot Variables, Page 2"));
    hb->addWidget(label);
    label->setAlignment(Qt::AlignCenter);

    entry = KWGET(kw_xlimit);
    if (entry) {
        entry->ent = new QTkwent(KW_REAL_2, QTkwent::ke_real_func, entry, 0);
        grid->addWidget(entry->qtent(), 1, 0);
        entry->qtent()->setup(0.0, 0.0, 0.0, 0.0, 2);
    }

    entry = KWGET(kw_ylimit);
    if (entry) {
        entry->ent = new QTkwent(KW_REAL_2, QTkwent::ke_real_func, entry, 0);
        grid->addWidget(entry->qtent(), 1, 1);
        entry->qtent()->setup(0.0, 0.0, 0.0, 0.0, 2);
    }

    entry = KWGET(kw_xcompress);
    if (entry) {
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_int_func, entry, "2");
        grid->addWidget(entry->qtent(), 2, 0);
        entry->qtent()->setup(2.0, 1.0, 0.0, 0.0, 0);
    }

    entry = KWGET(kw_xindices);
    if (entry) {
        entry->ent = new QTkwent(KW_INT_2, QTkwent::ke_int_func, entry, "0");
        grid->addWidget(entry->qtent(), 2, 1);
        entry->qtent()->setup(0.0, 1.0, 0.0, 0.0, 0);
    }

    entry = KWGET(kw_xdelta);
    if (entry) {
        entry->ent = new QTkwent(KW_NO_SPIN, QTkwent::ke_real_func, entry, 0);
        grid->addWidget(entry->qtent(), 3, 0);
    }

    entry = KWGET(kw_ydelta);
    if (entry) {
        entry->ent = new QTkwent(KW_NO_SPIN, QTkwent::ke_real_func, entry, 0);
        grid->addWidget(entry->qtent(), 3, 1);
    }

    entry = KWGET(kw_polydegree);
    if (entry) {
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_int_func, entry, "1");
        grid->addWidget(entry->qtent(), 4, 0);
        entry->qtent()->setup(1.0, 1.0, 0.0, 0.0, 0);
    }

    entry = KWGET(kw_polysteps);
    if (entry) {
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_int_func, entry, "10");
        grid->addWidget(entry->qtent(), 4, 1);
        entry->qtent()->setup(10.0, 1.0, 0.0, 0.0, 0);
    }

    entry = KWGET(kw_gridsize);
    if (entry) {
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_int_func, entry, "0");
        grid->addWidget(entry->qtent(), 5, 0);
        entry->qtent()->setup(0.0, 1.0, 0.0, 0.0, 0);
    }

    entry = KWGET(kw_pointchars);
    if (entry) {
        entry->ent = new QTkwent(KW_NO_SPIN, QTkwent::ke_string_func, entry,
            SpGrPkg::DefPointchars);
        grid->addWidget(entry->qtent(), 6, 0, 1, 2);
    }
    grid->setRowStretch(7, 1);

    // asciiplot page
    //
    page = new QWidget();
    pd_notebook->addTab(page, tr("asciiplot"));

    grid = new QGridLayout(page);
    grid->setMargin(2);
    grid->setSpacing(2);

    gb = new QGroupBox();
    grid->addWidget(gb, 0, 0, 1, 3);
    hb = new QHBoxLayout(gb);
    label = new QLabel(tr("Asciiplot Variables"));
    hb->addWidget(label);
    label->setAlignment(Qt::AlignCenter);

    entry = KWGET(kw_noasciiplotvalue);
    if (entry) {
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_bool_func, entry, 0);
        grid->addWidget(entry->qtent(), 1, 0);
    }

    entry = KWGET(kw_nobreak);
    if (entry) {
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_bool_func, entry, 0);
        grid->addWidget(entry->qtent(), 1, 1);
    }

    entry = KWGET(kw_nointerp);
    if (entry) {
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_bool_func, entry, 0);
        grid->addWidget(entry->qtent(), 1, 2);
    }
    grid->setRowStretch(2, 1);

    // hardcopy page
    //
    page = new QWidget();
    pd_notebook->addTab(page, tr("hardcopy"));

    grid = new QGridLayout(page);
    grid->setMargin(2);
    grid->setSpacing(2);

    gb = new QGroupBox();
    grid->addWidget(gb, 0, 0, 1, 2);
    hb = new QHBoxLayout(gb);
    label = new QLabel(tr("Hardcopy Variables"));
    hb->addWidget(label);
    label->setAlignment(Qt::AlignCenter);

    entry = KWGET(kw_hcopydriver);
    if (entry) {
        if (!KWdrvrs) {
            int k = 0;
            for (int i = 0; ; i++) {
                if (GRpkg::self()->HCof(i) &&
                        GRpkg::self()->HCof(i)->keyword) {
                    k++;
                    continue;
                }
                k++;
                break;
            }
            KWdrvrs = new Kword*[k];

            for (int i = 0; ; i++) {
                if (GRpkg::self()->HCof(i) &&
                        GRpkg::self()->HCof(i)->keyword) {
                    KWdrvrs[i] = new Kword(GRpkg::self()->HCof(i)->keyword,
                        GRpkg::self()->HCof(i)->descr);
                    continue;
                }
                KWdrvrs[i] = new Kword(0, 0);
                break;
            }
        }
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_string_func, entry,
            KWdrvrs[0]->word, KWdrvrs);
        grid->addWidget(entry->qtent(), 1, 0, 1, 2);
    }

    entry = KWGET(kw_hcopycommand);
    if (entry) {
        entry->ent = new QTkwent(KW_NO_SPIN, QTkwent::ke_string_func, entry, 0);
        grid->addWidget(entry->qtent(), 2, 0, 1, 2);
    }

    entry = KWGET(kw_hcopyresol);
    if (entry) {
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_int_func, entry, "0");
        grid->addWidget(entry->qtent(), 3, 0);
        entry->qtent()->setup(0.0, 1.0, 0.0, 0.0, 0);
    }

    entry = KWGET(kw_hcopylandscape);
    if (entry) {
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_bool_func, entry, 0);
        grid->addWidget(entry->qtent(), 3, 1);
    }

    char tbuf[32];
    entry = KWGET(kw_hcopywidth);
    if (entry) {
        snprintf(tbuf, sizeof(tbuf), "%g", wrsHCcb.width);
        entry->ent = new QTkwent(KW_NO_SPIN, QTkwent::ke_string_func, entry,
            tbuf);
        grid->addWidget(entry->qtent(), 4, 0);
    }

    entry = KWGET(kw_hcopyheight);
    if (entry) {
        snprintf(tbuf, sizeof(tbuf), "%g", wrsHCcb.height);
        entry->ent = new QTkwent(KW_NO_SPIN, QTkwent::ke_string_func, entry,
            tbuf);
        grid->addWidget(entry->qtent(), 4, 1);
    }

    entry = KWGET(kw_hcopyxoff);
    if (entry) {
        snprintf(tbuf, sizeof(tbuf), "%g", wrsHCcb.left);
        entry->ent = new QTkwent(KW_NO_SPIN, QTkwent::ke_string_func, entry,
            tbuf);
        grid->addWidget(entry->qtent(), 5, 0);
    }

    entry = KWGET(kw_hcopyyoff);
    if (entry) {
        snprintf(tbuf, sizeof(tbuf), "%g", wrsHCcb.top);
        entry->ent = new QTkwent(KW_NO_SPIN, QTkwent::ke_string_func, entry,
            tbuf);
        grid->addWidget(entry->qtent(), 5, 1);
    }

    entry = KWGET(kw_hcopyrmdelay);
    if (entry) {
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_int_func, entry, "0");
        grid->addWidget(entry->qtent(), 6, 0);
        entry->qtent()->setup(0.0, 1.0, 0.0, 0.0, 0);
    }
    grid->setRowStretch(7, 1);

    // xgraph page
    //
    page = new QWidget();
    pd_notebook->addTab(page, tr("xgraph"));

    grid = new QGridLayout(page);
    grid->setMargin(2);
    grid->setSpacing(2);

    gb = new QGroupBox();
    grid->addWidget(gb, 0, 0, 1, 2);
    hb = new QHBoxLayout(gb);
    label = new QLabel(tr("Xgraph Variables"));
    hb->addWidget(label);
    label->setAlignment(Qt::AlignCenter);

    entry = KWGET(kw_xgmarkers);
    if (entry) {
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_bool_func, entry, 0);
        grid->addWidget(entry->qtent(), 1, 0);
    }

    entry = KWGET(kw_xglinewidth);
    if (entry) {
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_int_func, entry, "1");
        grid->addWidget(entry->qtent(), 1, 1);
        entry->qtent()->setup(1.0, 1.0, 0.0, 0.0, 0);
    }
    grid->setRowStretch(2, 1);

    if (x || y) {
        TB()->FixLoc(&x, &y);
        move(x, y);
    }
    TB()->SetActiveDlg(tid_plotdefs, this);
}


QTplotParamDlg::~QTplotParamDlg()
{
    TB()->PopUpTBhelp(MODE_OFF, 0, 0, TBH_PD);
    instPtr = 0;
    TB()->SetLoc(tid_plotdefs, this);
    TB()->SetActiveDlg(tid_plotdefs, 0);
    QTtoolbar::entries(tid_plotdefs)->action()->setChecked(false);
}


void
QTplotParamDlg::dismiss_btn_slot()
{
    TB()->PopUpPlotDefs(MODE_OFF, 0, 0);
}


void
QTplotParamDlg::help_btn_slot(bool state)
{
    if (state)
        TB()->PopUpTBhelp(MODE_ON, this, sender(), TBH_PD);
    else
        TB()->PopUpTBhelp(MODE_OFF, 0, 0, TBH_PD);
}

#ifdef notdef


namespace {
    //
    // Callbacks to process the button selections
    //

    int
    pl_choice_hdlr(GtkWidget *caller, GdkEvent*, void *client_data)
    {
        xKWent *entry = static_cast<xKWent*>(client_data);
        if (GTKdev::GetStatus(entry->ent->active))
            return (true);
        int i;
        if (!strcmp(entry->word, kw_plotstyle)) {
            const char *string =
                gtk_entry_get_text(GTK_ENTRY(entry->ent->entry));
            for (i = 0; KW.pstyles(i)->word; i++)
                if (!strcmp(string, KW.pstyles(i)->word))
                    break;
            if (!KW.pstyles(i)->word) {
                GRpkg::self()->ErrPrintf(ET_ERROR,
                    "bad plotstyle found: %s.\n", string);
                i = 0;
            }
            else {
                if (g_object_get_data(G_OBJECT(caller), "down")) {
                    i--;
                    if (i < 0) {
                        i = 0;
                        while (KW.pstyles(i)->word && KW.pstyles(i+1)->word)
                            i++;
                    }
                }
                else {
                    i++;
                    if (!KW.pstyles(i)->word)
                        i = 0;
                }
            }
            gtk_entry_set_text(GTK_ENTRY(entry->ent->entry),
                KW.pstyles(i)->word);
        }
        else if (!strcmp(entry->word, kw_gridstyle)) {
            const char *string =
                gtk_entry_get_text(GTK_ENTRY(entry->ent->entry));
            for (i = 0; KW.gstyles(i)->word; i++)
                if (!strcmp(string, KW.gstyles(i)->word))
                    break;
            if (!KW.gstyles(i)->word) {
                GRpkg::self()->ErrPrintf(ET_ERROR,
                    "bad gridstyle found: %s.\n", string);
                i = 0;
            }
            else {
                if (g_object_get_data(G_OBJECT(caller), "down")) {
                    i--;
                    if (i < 0) {
                        i = 0;
                        while (KW.gstyles(i)->word && KW.gstyles(i+1)->word)
                            i++;
                    }
                }
                else {
                    i++;
                    if (!KW.gstyles(i)->word)
                        i = 0;
                }
            }
            gtk_entry_set_text(GTK_ENTRY(entry->ent->entry),
                KW.gstyles(i)->word);
        }
        else if (!strcmp(entry->word, kw_scaletype)) {
            const char *string =
                gtk_entry_get_text(GTK_ENTRY(entry->ent->entry));
            for (i = 0; KW.scale(i)->word; i++)
                if (!strcmp(string, KW.scale(i)->word))
                    break;
            if (!KW.scale(i)->word) {
                GRpkg::self()->ErrPrintf(ET_ERROR,
                    "bad scaletype found: %s.\n", string);
                i = 0;
            }
            else {
                if (g_object_get_data(G_OBJECT(caller), "down")) {
                    i--;
                    if (i < 0) {
                        i = 0;
                        while (KW.scale(i)->word && KW.scale(i+1)->word)
                            i++;
                    }
                }
                else {
                    i++;
                    if (!KW.scale(i)->word)
                        i = 0;
                }
            }
            gtk_entry_set_text(GTK_ENTRY(entry->ent->entry),KW.scale(i)->word);
        }
        else if (!strcmp(entry->word, kw_hcopydriver)) {
            const char *string =
                gtk_entry_get_text(GTK_ENTRY(entry->ent->entry));
            i = GRpkg::self()->FindHCindex(string);
            if (i < 0)
                i = wrsHCcb.format;
            else {
                if (g_object_get_data(G_OBJECT(caller), "down")) {
                    i--;
                    if (i < 0) {
                        i = 0;
                        while (GRpkg::self()->HCof(i) &&
                                GRpkg::self()->HCof(i+1))
                            i++;
                    }
                }
                else {
                    i++;
                    if (!GRpkg::self()->HCof(i))
                       i = 0;
                }
            }
            gtk_entry_set_text(GTK_ENTRY(entry->ent->entry),
                GRpkg::self()->HCof(i)->keyword);
        }
        return (true);
    }

}

#endif

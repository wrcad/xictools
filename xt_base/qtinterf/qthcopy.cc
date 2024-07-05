
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
 * QtInterf Graphical Interface Library                                   *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#include "qthcopy.h"
#include "qtprogress.h"
#include "miscutil/lstring.h"
#include "miscutil/filestat.h"

#include <unistd.h>

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QRadioButton>
#include <QDoubleSpinBox>
#include <QGroupBox>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QToolButton>
#include <QPushButton>


using namespace qtinterf;

// Help keywords used in this file:
// hcopypanel


// Global abort function exported to the Microsoft native print driver.
//
int HCabort_callabck()
{
    if (QTdev::self()->CheckForEvents()) {
        GRpkg::self()->HCabort("User aborted");
        return (0);
    }
    return (1);
}


namespace qtinterf
{
    struct sFonts
    {
        HCtextType type;
        const char *descr;
    };

    struct sMedia
    {
        const char *name;
        int width, height;
    };
}

namespace {
    // Text-mode fonts, for menu.
    //
    sFonts textfonts[] =
    {
        { PStimes,      "PostScript Times" },
        { PShelv,       "PostScript Helvetica" },
        { PScentury,    "PostScript Century" },
        { PSlucida,     "PostScript Lucida" },
        { PlainText,    "Plain Text" },
        { HtmlText,     "HTML Text"}
    };

    // List of standard paper sizes, units are points (1/72 inch).
    //
    sMedia pagesizes[] =
    {
        { "Letter",       612,  792 },
        { "Legal",        612,  1008 },
        { "Tabloid",      792,  1224 },
        { "Ledger",       1224, 792 },
        { "10x14",        720,  1008 },
        { "11x17",        792,  1224 },
        { "12x18",        864,  1296 },
        { "17x22 \"C\"",  1224, 1584 },
        { "18x24",        1296, 1728 },
        { "22x34 \"D\"",  1584, 2448 },
        { "24x36",        1728, 2592 },
        { "30x42",        2160, 3024 },
        { "34x44 \"E\"",  2448, 3168 },
        { "36x48",        2592, 3456 },
        { "Statement",    396,  612 },
        { "Executive",    540,  720 },
        { "Folio",        612,  936 },
        { "Quarto",       610,  780 },
        { "A0",           2384, 3370 },
        { "A1",           1684, 2384 },
        { "A2",           1190, 1684 },
        { "A3",           842,  1190 },
        { "A4",           595,  842 },
        { "A5",           420,  595 },
        { "A6",           298,  420 },
        { "B0",           2835, 4008 },
        { "B1",           2004, 2835 },
        { "B2",           1417, 2004 },
        { "B3",           1001, 1417 },
        { "B4",           729,  1032 },
        { "B5",           516,  729 },
        { NULL,           0,    0 },
    };
}

#define MMPI 25.4
#define MM(x) (pd_metric ? (x)*MMPI : (x))


QTprintDlg::QTprintDlg(GRobject caller, HCcb *cb, HCmode textmode, QTbag *wbag) :
    QDialog(wbag ? wbag->Shell() : 0)
{
    pd_caller = caller;
    pd_owner = wbag;
    pd_cb = cb;
    pd_cmdtext = 0;
    pd_tofilename = 0;
    pd_resol = 0;
    pd_fmt = 0;
    pd_textmode = textmode;
    pd_textfmt = PStimes;
    pd_legend = HClegOff;
    pd_orient = 0;
    pd_tofile = false;
    pd_tofbak = 0;
    pd_active = false;
    pd_metric = false;
    pd_drvrmask = 0;
    pd_pgsindex = 0;
    pd_wid_val = 0.0;
    pd_hei_val = 0.0;
    pd_lft_val = 0.0;
    pd_top_val = 0.0;

    pd_cmdtxtbox = 0;
    pd_cmdlab = 0;
    pd_wlabel = 0;
    pd_hlabel = 0;
    pd_xlabel = 0;
    pd_ylabel = 0;
    pd_wid = 0;
    pd_hei = 0;
    pd_left = 0;
    pd_top = 0;
    pd_portbtn = 0;
    pd_landsbtn = 0;
    pd_nobg = 0;
    pd_fitbtn = 0;
    pd_legbtn = 0;
    pd_tofbtn = 0;
    pd_metbtn = 0;
    pd_a4btn = 0;
    pd_ltrbtn = 0;
    pd_fontmenu = 0;
    pd_fmtmenu = 0;
    pd_resmenu = 0;
    pd_pgsmenu = 0;
    pd_frmbtn = 0;
    pd_hlpbtn = 0;
    pd_process = 0;
    pd_progress = 0;
    pd_printer_busy = false;

    setWindowTitle(QString(tr("Print Control")));
    setAttribute(Qt::WA_DeleteOnClose);

    if (pd_cb) {
        pd_drvrmask = cb->drvrmask;
        pd_fmt = cb->format;
        int i;
        for (i = 0; GRpkg::self()->HCof(i); i++) ;
        if (pd_fmt >= i || (pd_drvrmask & (1 << pd_fmt))) {
            for (i = 0; GRpkg::self()->HCof(i); i++) {
                if (!(pd_drvrmask & (1 << i)))
                    break;
            }
            if (GRpkg::self()->HCof(i))
                pd_fmt = i;
            else if (pd_textmode == HCgraphical) {
                if (pd_owner)
                    pd_owner->PopUpMessage(
                        "No hardcopy drivers available.", true);
                else if (GRpkg::self()->MainWbag())
                    GRpkg::self()->MainWbag()->PopUpMessage(
                        "No hardcopy drivers available.", true);
                return;
            }
        }
        HCdesc *hcdesc = GRpkg::self()->HCof(pd_fmt);
        if (hcdesc) {
            pd_resol = hcdesc->defaults.defresol;
            if (hcdesc->limits.resols)
                for (i = 0; hcdesc->limits.resols[i]; i++) ;
            else
                i = 0;
            if (pd_resol >= i)
                pd_resol = i-1;
            if (pd_resol < 0)
                pd_resol = 0;

            const char *c = hcdesc->defaults.command;
            if (!c || !*c)
                c = cb->command;
            if (!c || !*c)
                c = hcdesc->defaults.default_print_cmd;
            if (!c || !*c)
                c = GRappIf()->GetPrintCmd();
            if (!c)
                c = "";
            pd_cmdtext = lstring::copy(c);

            pd_legend = hcdesc->defaults.legend;
            pd_orient = hcdesc->defaults.orient;
        }
        else {
            pd_resol = 0;
            pd_cmdtext = lstring::copy(GRappIf()->GetPrintCmd());
            pd_legend = cb->legend;
            pd_orient = cb->orient;
        }
        pd_tofilename = lstring::copy(cb->tofilename ? cb->tofilename : "");
        pd_tofile = cb->tofile;
    }
    else {
        pd_cmdtext = lstring::copy(GRappIf()->GetPrintCmd());
        pd_tofilename = lstring::copy("");
        pd_resol = 0;
        pd_fmt = 0;
        pd_orient = HCbest;
        pd_legend = HClegOn;
        pd_tofile = false;
        pd_drvrmask = 0;
    }
    if (pd_textmode == HCgraphical) {
        HCdesc *desc = GRpkg::self()->HCof(pd_fmt);
        pd_wid_val = MM(desc->defaults.defwidth);
        pd_hei_val = MM(desc->defaults.defheight);
        pd_lft_val = MM(desc->defaults.defxoff);
        pd_top_val = MM(desc->defaults.defyoff);
    }
    pd_active = true;

    QMargins qmtop(2, 2, 2, 2);
    QMargins qm;
    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(qmtop);
    vbox->setSpacing(2);

    QHBoxLayout *hbox = new QHBoxLayout(0);
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);

    int row1cnt = 0;
    if (pd_textmode == HCgraphical) {
        if (cb && cb->hcframe) {
            pd_frmbtn = new QToolButton();
            pd_frmbtn->setText(tr("Frame"));
            hbox->addWidget(pd_frmbtn);
            connect(pd_frmbtn, &QAbstractButton::toggled,
                this, &QTprintDlg::frame_slot);
            row1cnt++;
            hbox->addSpacing(20);
        }
        pd_fitbtn = new QCheckBox(tr("Best Fit"));
        hbox->addWidget(pd_fitbtn);
        connect(pd_fitbtn, &QCheckBox::toggled,
            this, &QTprintDlg::best_fit_slot);
        row1cnt++;
        if (!cb || cb->legend != HClegNone) {
            pd_legbtn = new QCheckBox(tr("Legend"));
            hbox->addWidget(pd_legbtn);
            connect(pd_legbtn, &QCheckBox::toggled,
                this, &QTprintDlg::legend_slot);
            row1cnt++;
        }
    }
    else {
        if (!cb || cb->legend != HClegNone) {
            pd_legbtn = new QCheckBox(tr("Legend"));
            hbox->addWidget(pd_legbtn);
            if (pd_legend == HClegNone)
                pd_legbtn->setEnabled(false);
            else
                pd_legbtn->setChecked(pd_legend == HClegOn);
            connect(pd_legbtn, &QCheckBox::toggled,
                this, &QTprintDlg::legend_slot);
            row1cnt++;
        }
        if (pd_textmode == HCtextPS) {
            pd_a4btn = new QCheckBox(tr("A4"));
            hbox->addWidget(pd_a4btn);
            connect(pd_a4btn, &QCheckBox::toggled,
                this, &QTprintDlg::a4_slot);
            pd_ltrbtn = new QCheckBox(tr("Letter"));
            hbox->addWidget(pd_ltrbtn);
            connect(pd_ltrbtn, &QCheckBox::toggled,
                this, &QTprintDlg::letter_slot);
            hbox->addSpacing(20);
            if (pd_metric)
                pd_a4btn->setChecked(true);
            else
                pd_ltrbtn->setChecked(true);
            row1cnt++;

            pd_textfmt = PStimes;  // default postscript times

            pd_fontmenu = new QComboBox();
            hbox->addWidget(pd_fontmenu);
            int entries = sizeof(textfonts)/sizeof(sFonts);
            int hist = -1;
            for (int i = 0; i < entries; i++) {
                pd_fontmenu->addItem(textfonts[i].descr);
                if (pd_textfmt == textfonts[i].type)
                    hist = i;
            }
            if (hist < 0) {
                hist = 0;
                pd_textfmt = textfonts[0].type;
            }
            pd_fontmenu->setCurrentIndex(hist);
            connect(pd_fontmenu, QOverload<int>::of(&QComboBox::activated),
                this, &QTprintDlg::font_slot);
            row1cnt++;
        }
    }
    // Don't leave a lonely help button in the top row, move it to the
    // second row.
    if (row1cnt) {
        pd_hlpbtn = new QToolButton();
        pd_hlpbtn->setText(tr("Help"));
        hbox->addWidget(pd_hlpbtn);
        vbox->addLayout(hbox);
    }
    else
        delete hbox;

    QGroupBox *gb = new QGroupBox();
    hbox = new QHBoxLayout(gb);
    hbox->setContentsMargins(qmtop);
    hbox->setSpacing(2);
    pd_cmdlab = new QLabel(tr(pd_tofile ? "File Name" : "Print Command"));
    hbox->addWidget(pd_cmdlab);
    pd_tofbtn = new QCheckBox(tr("To File"));
    pd_tofbtn->setChecked(pd_tofile);
    connect(pd_tofbtn, &QCheckBox::toggled, this, &QTprintDlg::tofile_slot);
    hbox->addWidget(pd_tofbtn);
    if (row1cnt)
        vbox->addWidget(gb);
    else {
        hbox = new QHBoxLayout();
        hbox->setContentsMargins(qm);
        hbox->setSpacing(2);
        hbox->addWidget(gb);
        pd_hlpbtn = new QToolButton();
        pd_hlpbtn->setText(tr("Help"));
        hbox->addWidget(pd_hlpbtn);
        vbox->addLayout(hbox);
    }
    connect(pd_hlpbtn, &QAbstractButton::clicked,
        this, &QTprintDlg::help_slot);

    pd_cmdtxtbox = new QLineEdit();
    pd_cmdtxtbox->setText(pd_tofile ? pd_tofilename : pd_cmdtext);
    vbox->addWidget(pd_cmdtxtbox);

    if (pd_textmode == HCgraphical) {
        hbox = new QHBoxLayout(0);
        hbox->setContentsMargins(qm);
        hbox->setSpacing(2);
        QVBoxLayout *vb = new QVBoxLayout();

        gb = new QGroupBox();
        QHBoxLayout *hb = new QHBoxLayout(gb);
        hb->setContentsMargins(qmtop);
        hb->setSpacing(2);
        pd_portbtn = new QRadioButton(tr("Portrait"));
        hb->addWidget(pd_portbtn);
        connect(pd_portbtn, &QRadioButton::toggled,
            this, &QTprintDlg::portrait_slot);
        pd_landsbtn = new QRadioButton(tr("Landscape"));
        hb->addWidget(pd_landsbtn);
        connect(pd_landsbtn, &QRadioButton::toggled,
            this, &QTprintDlg::landscape_slot);
        pd_nobg = new QCheckBox(tr("No Backg"));
        hb->addWidget(pd_nobg);
        connect(pd_nobg, &QCheckBox::toggled,
            this, &QTprintDlg::nobg_slot);
        vb->addWidget(gb);

        pd_fmtmenu = new QComboBox();
        for (int i = 0; GRpkg::self()->HCof(i); i++) {
            if (pd_drvrmask & (1 << i))
                continue;
            pd_fmtmenu->addItem(QString(GRpkg::self()->HCof(i)->descr));
        }
        pd_fmtmenu->setCurrentIndex(pd_fmt);
        vb->addWidget(pd_fmtmenu);
        connect(pd_fmtmenu,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &QTprintDlg::format_slot);
        hbox->addLayout(vb);

        vb = new QVBoxLayout();

        gb = new QGroupBox();
        vb->addWidget(gb);
        hb = new QHBoxLayout(gb);
        hb->setContentsMargins(qmtop);
        hb->setSpacing(2);
        QLabel *res = new QLabel(tr("Resolution"));
        hb->addWidget(res);

        pd_resmenu = new QComboBox();
        const char **s = GRpkg::self()->HCof(pd_fmt)->limits.resols;
        if (s && *s) {
            for (int i = 0; s[i]; i++)
                pd_resmenu->addItem(s[i]);
        }
        else
            pd_resmenu->addItem("fixed");
        pd_resmenu->setCurrentIndex(pd_resol);
        connect(pd_resmenu, QOverload<int>::of(&QComboBox::activated),
            this, &QTprintDlg::resol_slot);
        vb->addWidget(pd_resmenu);
        hbox->addLayout(vb);
        vbox->addLayout(hbox);

        hbox = new QHBoxLayout(0);
        vbox->addLayout(hbox);
        hbox->setContentsMargins(qm);
        hbox->setSpacing(2);

        HCdesc *desc = GRpkg::self()->HCof(pd_fmt);
        vb = new QVBoxLayout();
        hbox->addLayout(vb);
        gb = new QGroupBox();
        vb->addWidget(gb);
        hb = new QHBoxLayout(gb);
        hb->setContentsMargins(qmtop);
        pd_wlabel = new QCheckBox(tr("Width"));
        connect(pd_wlabel, &QCheckBox::toggled,
            this, &QTprintDlg::auto_width_slot);
        hb->addWidget(pd_wlabel);
        pd_wid = new QDoubleSpinBox();
        pd_wid->setDecimals(2);
        pd_wid->setRange(MM(desc->limits.minwidth), MM(desc->limits.maxwidth));
        pd_wid->setValue(pd_wid_val);
        pd_wid->setSingleStep(0.1);
        vb->addWidget(pd_wid);

        vb = new QVBoxLayout();
        hbox->addLayout(vb);
        gb = new QGroupBox();
        vb->addWidget(gb);
        hb = new QHBoxLayout(gb);
        hb->setContentsMargins(qmtop);
        pd_hlabel = new QCheckBox(tr("Height"));
        connect(pd_hlabel, &QCheckBox::toggled,
            this, &QTprintDlg::auto_height_slot);
        hb->addWidget(pd_hlabel);
        pd_hei = new QDoubleSpinBox();
        pd_hei->setDecimals(2);
        pd_hei->setRange(MM(desc->limits.minheight),MM(desc->limits.maxheight));
        pd_hei->setValue(pd_hei_val);
        pd_hei->setSingleStep(0.1);
        vb->addWidget(pd_hei);

        vb = new QVBoxLayout();
        hbox->addLayout(vb);
        gb = new QGroupBox();
        vb->addWidget(gb);
        hb = new QHBoxLayout(gb);
        hb->setContentsMargins(qmtop);
        pd_xlabel = new QLabel(tr("Left"));
        hb->addWidget(pd_xlabel);
        pd_left = new QDoubleSpinBox();
        pd_left->setDecimals(2);
        pd_left->setRange(MM(desc->limits.minxoff), MM(desc->limits.maxxoff));
        pd_left->setValue(pd_lft_val);
        pd_left->setSingleStep(0.1);
        vb->addWidget(pd_left);

        vb = new QVBoxLayout();
        hbox->addLayout(vb);
        gb = new QGroupBox();
        vb->addWidget(gb);
        hb = new QHBoxLayout(gb);
        hb->setContentsMargins(qmtop);
        pd_ylabel = new QLabel(tr("Bottom"));
        hb->addWidget(pd_ylabel);
        pd_top = new QDoubleSpinBox();
        pd_top->setDecimals(2);
        pd_top->setRange(MM(desc->limits.minyoff), MM(desc->limits.maxyoff));
        pd_top->setValue(pd_top_val);
        pd_top->setSingleStep(0.1);
        vb->addWidget(pd_top);

        hbox = new QHBoxLayout(0);
        vbox->addLayout(hbox);
        hbox->setContentsMargins(qm);
        hbox->setSpacing(2);

        pd_pgsmenu = new QComboBox();
        hbox->addWidget(pd_pgsmenu);
        int i = 0;
        for (sMedia *m = pagesizes; m->name; m++, i++)
            pd_pgsmenu->addItem(m->name);
        connect(pd_pgsmenu, QOverload<int>::of(&QComboBox::activated),
            this, &QTprintDlg::pagesize_slot);
        pd_metbtn = new QCheckBox(tr("Metric (mm)"));
        pd_metbtn->setChecked(pd_metric);
        connect(pd_metbtn, &QCheckBox::toggled,
            this, &QTprintDlg::metric_slot);
        hbox->addSpacing(20);
        hbox->addWidget(pd_metbtn);
    }

    hbox = new QHBoxLayout(0);
    vbox->addLayout(hbox);
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);

    QToolButton *tbtn = new QToolButton();
    tbtn->setText(tr("Print"));
    hbox->addWidget(tbtn);
    connect(tbtn, &QAbstractButton::clicked, this, &QTprintDlg::print_slot);

    QPushButton *btn = new QPushButton(tr("Dismiss"));
    btn->setObjectName("Default");
    hbox->addWidget(btn);
    connect(btn, &QAbstractButton::clicked, this, &QTprintDlg::quit_slot);

    if (pd_cb && pd_cb->hcsetup)
        (*pd_cb->hcsetup)(true, pd_fmt, false, 0);

    if (pd_textmode == HCgraphical) {
        HCdesc *desc = GRpkg::self()->HCof(pd_fmt);
        set_sens(desc->limits.flags);
        if (!(desc->limits.flags & HCdontCareWidth) && pd_wid_val == 0.0 &&
                desc->limits.minwidth > 0.0) {
            if (desc->limits.flags & HCnoAutoWid) {
                pd_wid_val = MM(desc->limits.minwidth);
                desc->defaults.defwidth = desc->limits.minwidth;
            }
            else {
                float tmp = desc->last_w;
                pd_wlabel->setChecked(true);
                desc->last_w = tmp;
            }
        }
        if (!(desc->limits.flags & HCdontCareHeight) && pd_hei_val == 0.0 &&
                desc->limits.minheight > 0.0) {
            if (desc->limits.flags & HCnoAutoHei) {
                pd_hei_val = MM(desc->limits.minheight);
                desc->defaults.defheight = desc->limits.minheight;
            }
            else {
                float tmp = desc->last_h;
                pd_hlabel->setChecked(true);
                desc->last_h = tmp;
            }
        }
    }

    // Success, link into owning QTbag.
    if (pd_owner) {
        if (pd_owner->HC())
            delete pd_owner->HC();
        pd_owner->SetHC(this);
    }
}


QTprintDlg::~QTprintDlg()
{
    if (pd_owner)
        pd_owner->SetHC(0);
    if (pd_caller)
        QTdev::self()->SetStatus(pd_caller, false);
}


#ifdef Q_OS_MACOS
#define DLGTYPE QTprintDlg
#include "qtmacos_event.h"
#endif


// Function to query values for command text, resolution, etc.
// The HCcb struct is filled in with the present values.
//
void
QTprintDlg::update(HCcb *cb)
{
    if (!cb)
        return;
    cb->hcsetup = pd_cb ? pd_cb->hcsetup : 0;
    cb->hcgo = pd_cb ? pd_cb->hcgo : 0;
    cb->hcframe = pd_cb ? pd_cb->hcframe : 0;

    char *s = lstring::copy(pd_cmdtxtbox->text().toLatin1().constData());
    if (pd_tofile) {
        cb->tofilename = s;
        cb->command = pd_cmdtext;
    }
    else {
        cb->command = s;
        cb->tofilename = pd_tofilename;
    }
    cb->resolution = pd_resol;
    cb->format = pd_fmt;
    cb->drvrmask = pd_drvrmask;
    cb->legend = pd_legend;
    cb->orient = pd_orient;
    cb->tofile = pd_tofile;

    double d;
    d = pd_left->value();
    if (pd_metric)
        d /= MMPI;
    cb->left = d;
    d = pd_top->value();
    if (pd_metric)
        d /= MMPI;
    cb->top = d;
    d = pd_wid->value();
    if (pd_metric)
        d /= MMPI;
    cb->width = d;
    d = pd_hei->value();
    if (pd_metric)
        d /= MMPI;
    cb->height = d;
}


// Print a message to the progress monitor.
//
void
QTprintDlg::set_message(const char *msg)
{
    if (pd_progress)
        pd_progress->set_etc(msg);
}


void
QTprintDlg::disable_progress()
{
    delete pd_progress;
}


void
QTprintDlg::set_format(int fmt)
{
    if (fmt >= 0 && fmt < 32 && !(pd_drvrmask & (1 << fmt)))
        pd_fmtmenu->setCurrentIndex(fmt);
}


void
QTprintDlg::set_active(bool set)
{
    if (set) {
        if (!pd_active) {
            pd_active = true;
            show();
            raise();
            activateWindow();
        }
        if (pd_caller)
            QTdev::self()->SetStatus(pd_caller, true);
    }
    else {
        if (pd_active) {
            pd_active = false;
            hide();
        }
        if (pd_caller)
            QTdev::self()->SetStatus(pd_caller, false);
    }
}


// Font change handler.
//
void
QTprintDlg::format_slot(int indx)
{
    if (pd_wlabel->isChecked())
        pd_wlabel->setChecked(false);
    if (pd_hlabel->isChecked())
        pd_hlabel->setChecked(false);

    if (pd_drvrmask & (1 << indx))
        return;;
    int i = pd_fmt;
    pd_fmt = indx;
    // set the current defaults to the current values
    if (GRpkg::self()->HCof(i)->defaults.command)
        delete [] GRpkg::self()->HCof(i)->defaults.command;
    if (!pd_tofile)
        GRpkg::self()->HCof(i)->defaults.command =
            lstring::copy(pd_cmdtxtbox->text().toLatin1().constData());
    else
        GRpkg::self()->HCof(i)->defaults.command = lstring::copy(pd_cmdtext);
    GRpkg::self()->HCof(i)->defaults.defresol = pd_resol;

    GRpkg::self()->HCof(i)->defaults.legend = pd_legend;
    pd_legend = GRpkg::self()->HCof(pd_fmt)->defaults.legend;
    GRpkg::self()->HCof(i)->defaults.orient = pd_orient;
    pd_orient = GRpkg::self()->HCof(pd_fmt)->defaults.orient;

    double w = pd_wid->value();
    if (pd_metric)
        w /= MMPI;
    GRpkg::self()->HCof(i)->defaults.defwidth = w;
    double h = pd_hei->value();
    if (pd_metric)
        h /= MMPI;
    GRpkg::self()->HCof(i)->defaults.defheight = h;
    double xx = pd_left->value();
    if (pd_metric)
        xx /= MMPI;
    GRpkg::self()->HCof(i)->defaults.defxoff = xx;
    double yy = pd_top->value();
    if (pd_metric)
        yy /= MMPI;
    GRpkg::self()->HCof(i)->defaults.defyoff = yy;

    HCdesc *desc = GRpkg::self()->HCof(pd_fmt);
    checklims(desc);

    // set the new values
    delete [] pd_cmdtext;
    if (desc->defaults.command)
        pd_cmdtext = lstring::copy(desc->defaults.command);
    else if (desc->defaults.default_print_cmd)
        pd_cmdtext = lstring::copy(desc->defaults.default_print_cmd);
    else
        pd_cmdtext = lstring::copy("");
    if (!pd_tofile)
        pd_cmdtxtbox->setText(QString(pd_cmdtext));

    pd_resol = desc->defaults.defresol;

    pd_wid->setMinimum(MM(desc->limits.minwidth));
    pd_wid->setMaximum(MM(desc->limits.maxwidth));
    pd_wid->setValue(MM(desc->defaults.defwidth));

    pd_hei->setMinimum(MM(desc->limits.minheight));
    pd_hei->setMaximum(MM(desc->limits.maxheight));
    pd_hei->setValue(MM(desc->defaults.defheight));

    pd_left->setMinimum(MM(desc->limits.minxoff));
    pd_left->setMaximum(MM(desc->limits.maxxoff));
    pd_left->setValue(MM(desc->defaults.defxoff));

    pd_top->setMinimum(MM(desc->limits.minyoff));
    pd_top->setMaximum(MM(desc->limits.maxyoff));
    pd_top->setValue(MM(desc->defaults.defyoff));

    if (desc->limits.flags & HCtopMargin)
        pd_ylabel->setText(QString(tr("Top")));
    else
        pd_ylabel->setText(QString(tr("Bottom")));

    set_sens(desc->limits.flags);
    if (!(desc->limits.flags & HCdontCareWidth) &&
            desc->defaults.defwidth == 0.0 && desc->limits.minwidth > 0.0) {
        if (desc->limits.flags & HCnoAutoWid)
            desc->defaults.defwidth = desc->limits.minwidth;
        else
            pd_wlabel->setChecked(true);
    }
    if (!(desc->limits.flags & HCdontCareHeight) &&
            desc->defaults.defheight == 0.0 && desc->limits.minheight > 0.0) {
        if (desc->limits.flags & HCnoAutoHei)
            desc->defaults.defheight = desc->limits.minheight;
        else
            pd_hlabel->setChecked(true);
    }

    pd_resmenu->clear();
    const char **s = desc->limits.resols;
    if (s && *s) {
        for (int j = 0; s[j]; j++)
            pd_resmenu->addItem(QString(s[j]));
    }
    else
        pd_resmenu->addItem(QString("fixed"));
    pd_resmenu->setCurrentIndex(pd_resol);

    if (pd_cb && pd_cb->hcsetup)
        (*pd_cb->hcsetup)(true, pd_fmt, false, 0);
}


// Set the printer resolution.
//
void
QTprintDlg::resol_slot(int indx)
{
    pd_resol = indx;
}


// Handler for the "font" menu.
//
void
QTprintDlg::font_slot(int indx)
{
    pd_textfmt = textfonts[indx].type;
}


// Handler for the page size menu.
//
void
QTprintDlg::pagesize_slot(int indx)
{
    double shrink = 0.375 * 72;
    double w = pagesizes[indx].width - 2*shrink;
    double h = pagesizes[indx].height - 2*shrink;
    pd_pgsindex = indx;
    if (pd_metric) {
        w *= MMPI;
        h *= MMPI;
        shrink *= MMPI;
    }
    pd_wid->setValue(w/72);
    pd_hei->setValue(h/72);
    pd_left->setValue(shrink/72);
    pd_top->setValue(shrink/72);
}


void
QTprintDlg::a4_slot(bool set)
{
    if (set) {
        if (pd_ltrbtn->isChecked())
            pd_ltrbtn->setChecked(false);
    }
    else if (!pd_ltrbtn->isChecked())
        pd_a4btn->setChecked(true);
    // This is text-only mode, set the metric field for A4.
    pd_metric = pd_a4btn->isChecked();
}


void
QTprintDlg::letter_slot(bool set)
{
    if (set) {
        if (pd_a4btn->isChecked())
            pd_a4btn->setChecked(false);
    }
    else if (!pd_a4btn->isChecked())
        pd_ltrbtn->setChecked(true);
    pd_metric = pd_a4btn->isChecked();
}


void
QTprintDlg::metric_slot(bool set)
{
    bool wasmetric = pd_metric;
    pd_metric = set;

    if (wasmetric != pd_metric) {
        double d;

        d = pd_wid->value();
        if (wasmetric) {
            d /= MMPI;
            pd_wid->setMinimum(pd_wid->minimum()/MMPI);
            pd_wid->setMaximum(pd_wid->maximum()/MMPI);
            pd_wid->setValue(d);
        }
        else {
            d *= MMPI;
            pd_wid->setMinimum(pd_wid->minimum()*MMPI);
            pd_wid->setMaximum(pd_wid->maximum()*MMPI);
            pd_wid->setValue(d);
        }

        d = pd_hei->value();
        if (wasmetric) {
            d /= MMPI;
            pd_hei->setMinimum(pd_hei->minimum()/MMPI);
            pd_hei->setMaximum(pd_hei->maximum()/MMPI);
            pd_hei->setValue(d);
        }
        else {
            d *= MMPI;
            pd_hei->setMinimum(pd_hei->minimum()*MMPI);
            pd_hei->setMaximum(pd_hei->maximum()*MMPI);
            pd_hei->setValue(d);
        }

        d = pd_left->value();
        if (wasmetric) {
            d /= MMPI;
            pd_left->setMinimum(pd_left->minimum()/MMPI);
            pd_left->setMaximum(pd_left->maximum()/MMPI);
            pd_left->setValue(d);
        }
        else {
            d *= MMPI;
            pd_left->setMinimum(pd_left->minimum()*MMPI);
            pd_left->setMaximum(pd_left->maximum()*MMPI);
            pd_left->setValue(d);
        }

        d = pd_top->value();
        if (wasmetric) {
            d /= MMPI;
            pd_top->setMinimum(pd_top->minimum()/MMPI);
            pd_top->setMaximum(pd_top->maximum()/MMPI);
            pd_top->setValue(d);
        }
        else {
            d *= MMPI;
            pd_top->setMinimum(pd_top->minimum()*MMPI);
            pd_top->setMaximum(pd_top->maximum()*MMPI);
            pd_top->setValue(d);
        }
    }
}


// Frame callback.  This allows the user to define the area  of
// graphics to be printed.
//
void
QTprintDlg::frame_slot(bool)
{
    if (pd_cb && pd_cb->hcframe)
        (*pd_cb->hcframe)(HCframeCmd, 0, 0, 0, 0, 0, 0);
}


// Switch between portrait and landscape orientation.
//
void
QTprintDlg::portrait_slot(bool set)
{
    if (set) {
        if (pd_landsbtn && pd_landsbtn->isChecked())
            pd_landsbtn->setChecked(false);
        if (!(pd_orient & HClandscape))
            return;
        pd_orient &= ~HClandscape;
        // See if we should swap the margin label
        if (GRpkg::self()->HCof(pd_fmt)->limits.flags & HClandsSwpYmarg) {
            QByteArray ba = pd_ylabel->text().toLatin1();
            const char *str = ba.constData();
            if (!strcmp(str, "Top"))
                pd_ylabel->setText("Bottom");
            else
                pd_ylabel->setText("Top");
        }
    }
    else if (!pd_landsbtn->isChecked())
        pd_portbtn->setChecked(true);
}


void
QTprintDlg::landscape_slot(bool set)
{
    if (set) {
        if (pd_portbtn->isChecked())
            pd_portbtn->setChecked(false);
        if (pd_orient & HClandscape)
            return;
        pd_orient |= HClandscape;
        // See if we should swap the margin label
        if (GRpkg::self()->HCof(pd_fmt)->limits.flags & HClandsSwpYmarg) {
            QByteArray ba = pd_ylabel->text().toLatin1();
            const char *str = ba.constData();
            if (!strcmp(str, "Top"))
                pd_ylabel->setText("Bottom");
            else
                pd_ylabel->setText("Top");
        }
    }
    else if (!pd_portbtn->isChecked())
        pd_landsbtn->setChecked(true);
}


void
QTprintDlg::nobg_slot(bool set)
{
    if (set) {
    }
}


// If the "best fit" button is active, allow rotation of the image.
//
void
QTprintDlg::best_fit_slot(bool set)
{
    if (set)
        pd_orient |= HCbest;
    else
        pd_orient &= ~HCbest;
}


// Send the output to a file, rather than a printer.
//
void
QTprintDlg::tofile_slot(bool set)
{
    pd_tofile = set;
    QByteArray ba = pd_cmdtxtbox->text().toLatin1();
    const char *s = ba.constData();
    if (set) {
        pd_cmdlab->setText("File Name");
        delete [] pd_cmdtext;
        pd_cmdtext = lstring::copy(s);
        pd_cmdtxtbox->setText(pd_tofilename);
    }
    else {
        pd_cmdlab->setText("Print Command");
        delete [] pd_tofilename;
        pd_tofilename = lstring::copy(s);
        pd_cmdtxtbox->setText(pd_cmdtext);
    }
}


// Toggle display of the legend associated with the plot.
//
void
QTprintDlg::legend_slot(bool set)
{
    pd_legend = (set ? HClegOn : HClegOff);
}


// Handle the auto-height and auto-width buttons
//
void
QTprintDlg::auto_width_slot(bool set)
{
    if (set) {
        GRpkg::self()->HCof(pd_fmt)->last_w = pd_wid->value();
        if (pd_metric)
            GRpkg::self()->HCof(pd_fmt)->last_w /= MMPI;
        GRpkg::self()->HCof(pd_fmt)->defaults.defwidth = 0;
        pd_wid->setEnabled(false);
        pd_wid->setPrefix(QString("Auto"));
        pd_wid->clear();
        if (pd_hlabel->isChecked()) {
            pd_hlabel->setChecked(false);
            pd_hei->setEnabled(true);
            double h = GRpkg::self()->HCof(pd_fmt)->last_h;
            if (h == 0.0)
                h = GRpkg::self()->HCof(pd_fmt)->limits.minheight;
            pd_hei->setValue(MM(h));
        }
    }
    else {
        pd_wid->setPrefix(QString());
        pd_wid->setEnabled(true);
        double w = GRpkg::self()->HCof(pd_fmt)->last_w;
        if (w == 0.0)
            w = GRpkg::self()->HCof(pd_fmt)->limits.minwidth;
        pd_wid->setValue(MM(w));
    }
}


void
QTprintDlg::auto_height_slot(bool set)
{
    if (set) {
        GRpkg::self()->HCof(pd_fmt)->last_h = pd_hei->value();
        if (pd_metric)
            GRpkg::self()->HCof(pd_fmt)->last_h /= MMPI;
        GRpkg::self()->HCof(pd_fmt)->defaults.defheight = 0;
        pd_hei->setEnabled(false);
        pd_hei->setPrefix(QString("Auto"));
        pd_hei->clear();
        if (pd_wlabel->isChecked()) {
            pd_wlabel->setChecked(false);
            pd_wid->setEnabled(true);
            double w = GRpkg::self()->HCof(pd_fmt)->last_w;
            if (w == 0.0)
                w = GRpkg::self()->HCof(pd_fmt)->limits.minwidth;
            pd_wid->setValue(MM(w));
        }
    }
    else {
        pd_hei->setPrefix(QString());
        pd_hei->setEnabled(true);
        double h = GRpkg::self()->HCof(pd_fmt)->last_h;
        if (h == 0.0)
            h = GRpkg::self()->HCof(pd_fmt)->limits.minheight;
        pd_hei->setValue(MM(h));
    }
}


// Handler for button presses within the widget not caught by subwidgets,
// for help mode.
//
void
QTprintDlg::help_slot()
{
    if (pd_owner)
        pd_owner->PopUpHelp("hcopypanel");
    else if (GRpkg::self()->MainWbag())
        GRpkg::self()->MainWbag()->PopUpHelp("hcopypanel");
}


#define MAX_ARGS 20

// Callback to actually generate the hardcopy.
//
void
QTprintDlg::print_slot()
{
    GRpkg::self()->HCabort(0);
    if (!pd_owner)
        return;

    QByteArray ba = pd_cmdtxtbox->text().toLatin1();
    const char *str = ba.constData();
    if (!str || !*str) {
        if (pd_tofile)
            pd_owner->PopUpMessage("No filename given!", true);
        else
            pd_owner->PopUpMessage("No command given!", true);
        return;
    }

    char *filename, buf[512];
    if (pd_textmode != HCgraphical) {
        if (!pd_tofile)
            filename = filestat::make_temp("hc");
        else
            filename = lstring::copy(str);

        if (pd_textmode == HCtext) {
            bool err = false;
            FILE *fp = fopen(filename, "w");
            if (fp) {
                char *all_text = pd_owner->GetPlainText();
                if (all_text) {
                    size_t length = strlen(all_text);
                    size_t start = 0;
                    for (;;) {
                        size_t end = start + 1024;
                        if (end > length)
                            end = length;
                        if (end == start)
                            break;
                        if (fwrite(all_text + start, 1, end - start, fp) <
                                (unsigned)(end - start)) {
                            err = true;
                            break;
                        }
                        if (end - start < 1024)
                            break;
                        start = end;
                    }
                }
                delete [] all_text;
                fclose(fp);
                if (!pd_tofile) {
                    QByteArray tba = pd_cmdtxtbox->text().toLatin1();
                    const char *st = tba.constData();
                    fork_and_submit(st, filename);
                    // note that the file is NOT unlinked
                }
                else
                    pd_owner->PopUpMessage("Text saved", false);
            }
            if (!fp || err)
                pd_owner->PopUpMessage("write error: text not saved", true);
        }
        else if (pd_textmode == HCtextPS) {
            char *text = 0;
            switch (pd_textfmt) {
            case PlainText:
            case PrettyText:
                text = pd_owner->GetPlainText();
                break;
            case PSlucida:
                text = pd_owner->GetPostscriptText(3, 0, 0, true, pd_metric);
                break;
            case PScentury:
                text = pd_owner->GetPostscriptText(2, 0, 0, true, pd_metric);
                break;
            case PShelv:
                text = pd_owner->GetPostscriptText(1, 0, 0, true, pd_metric);
                break;
            case PStimes:
                text = pd_owner->GetPostscriptText(0, 0, 0, true, pd_metric);
                break;
            case HtmlText:
                text = pd_owner->GetHtmlText();
                // returns pointer to internal data
                break;
            }

            if (!text) {
                pd_owner->PopUpMessage("Internal error: no text found", true);
                delete [] filename;
                return;
            }
            FILE *fp = fopen(filename, "w");
            if (!fp) {
                snprintf(buf, sizeof(buf), "Error: can't open file %s",
                    filename);
                pd_owner->PopUpMessage(buf, true);
                delete [] filename;
                if (pd_textfmt != HtmlText)
                    delete [] text;
                return;
            }
            if (fputs(text, fp) == EOF) {
                pd_owner->PopUpMessage("Internal error: block write error",
                    true);
                delete [] filename;
                if (pd_textfmt != HtmlText)
                    delete [] text;
                fclose(fp);
                return;
            }
            fclose(fp);
            if (pd_textfmt != HtmlText)
                delete [] text;
            if (!pd_tofile) {
                QByteArray tba = pd_cmdtxtbox->text().toLatin1();
                const char *st = tba.constData();
                fork_and_submit(st, filename);
                // note that the file is NOT unlinked
            }
            else
                pd_owner->PopUpMessage("Text saved", false);
        }
        delete [] filename;
        return;
    }

    if (pd_printer_busy) {
        pd_owner->PopUpMessage("I'm busy, please wait.", true);
        return;
    }
    pd_printer_busy = true;

    double w = 0.0;
    if (!(GRpkg::self()->HCof(pd_fmt)->limits.flags & HCdontCareWidth)) {
        if (!pd_wlabel->isChecked()) {
            w = pd_wid->value();
            if (pd_metric)
                w /= MMPI;
        }
    }
    double h = 0.0;
    if (!(GRpkg::self()->HCof(pd_fmt)->limits.flags & HCdontCareHeight)) {
        if (!pd_hlabel->isChecked()) {
            h = pd_hei->value();
            if (pd_metric)
                h /= MMPI;
        }
    }
    double xx = 0.0;
    if (!(GRpkg::self()->HCof(pd_fmt)->limits.flags & HCdontCareXoff)) {
        xx = pd_left->value();
        if (pd_metric)
            xx /= MMPI;
    }
    double yy = 0.0;
    if (!(GRpkg::self()->HCof(pd_fmt)->limits.flags & HCdontCareYoff)) {
        yy = pd_top->value();
        if (pd_metric)
            yy /= MMPI;
    }
    if (!pd_tofile)
        filename = filestat::make_temp("hc");
    else
        filename = lstring::copy(str);
    int resol = 0;
    if (GRpkg::self()->HCof(pd_fmt)->limits.resols)
        sscanf(GRpkg::self()->HCof(pd_fmt)->limits.resols[pd_resol], "%d",
            &resol);
    snprintf(buf, sizeof(buf), GRpkg::self()->HCof(pd_fmt)->fmtstring,
        filename, resol, w, h, xx, yy);
    if (pd_orient & HClandscape)
        strcat(buf, " -l");

    if (pd_nobg->isVisible() && pd_nobg->isChecked())
        strcat(buf, " -b");

    char *cmdstr = lstring::copy(buf);
    char *argv[MAX_ARGS];
    int argc;
    mkargv(&argc, argv, cmdstr);

    HCswitchErr err =
        GRpkg::self()->SwitchDev(GRpkg::self()->HCof(pd_fmt)->drname,
        &argc, argv);
    if (err == HCSinhc)
        pd_owner->PopUpMessage("Internal error - aborted", true);
    else if (err == HCSnotfnd) {
        snprintf(buf, sizeof(buf), "No hardcopy driver named %s available",
            GRpkg::self()->HCof(pd_fmt)->drname);
        pd_owner->PopUpMessage(buf, true);
    }
    else if (err == HCSinit)
        pd_owner->PopUpMessage("Init callback failed - aborted", true);
    else {
        bool ok = true;
        // hc might be freed during hcgo
        bool tofile = pd_tofile;
        char *cmd = lstring::copy(pd_cmdtxtbox->text().toLatin1().constData());

        if (pd_cb && pd_cb->hcgo) {
            HCorientFlags ot = pd_orient & HCbest;
            // pass the landscape flag only if the driver can't rotate
            if ((pd_orient & HClandscape) &&
                    (GRpkg::self()->HCof(pd_fmt)->limits.flags & HCnoCanRotate))
                ot |= HClandscape;
            if ((*pd_cb->hcgo)(ot, pd_legend, 0)) {
                if (GRpkg::self()->HCaborted()) {
                    snprintf(buf, sizeof(buf), "Terminated: %s.",
                        GRpkg::self()->HCabortMsg());
                    pd_owner->PopUpMessage(buf, true);
                }
                else
                    pd_owner->PopUpMessage(
                        "Error encountered in hardcopy generation - aborted",
                        true);
                ok = false;
                unlink(filename);
            }
        }
        GRpkg::self()->SwitchDev(0, 0, 0);
        if (ok) {
            if (!tofile)
                fork_and_submit(cmd, filename);
            else
                pd_owner->PopUpMessage("Command executed successfully",
                    false);
        }
        delete [] cmd;
    }
    delete [] cmdstr;
    delete [] filename;
    pd_printer_busy = false;
}


void
QTprintDlg::quit_slot()
{
    set_active(false);  // keep the pop-up alive
    emit dismiss();
}


void
QTprintDlg::process_error_slot(QProcess::ProcessError err)
{
    if (pd_progress) {
        const char *msg;
        switch (err) {
        case QProcess::FailedToStart:
            msg = "failed to start";
            break;
        case QProcess::Crashed:
            msg = "crashed";
            break;
        case QProcess::Timedout:
            msg = "timed out";
            break;
        case QProcess::WriteError:
            msg = "write error";
            break;
        case QProcess::ReadError:
            msg = "read error";
            break;
        default:
        case QProcess::UnknownError:
            msg = "unknown error";
            break;
        }
        
        char buf[256];
        snprintf(buf, sizeof(buf), "Error reported: code=%d (%s)", err, msg);
        pd_progress->set_etc(buf);
    }
}


void
QTprintDlg::process_finished_slot(int code, QProcess::ExitStatus)
{
    if (pd_progress) {
        char buf[256];
        if (code == 0)
            strcpy(buf, "Job completed successfully.");
        else {
            snprintf(buf, sizeof(buf), "Job completed with exit status %d.",
                code);
        }
        pd_progress->set_etc(buf);
        pd_progress->finished();
    }
}


void
QTprintDlg::set_sens(unsigned int word)
{
    pd_resmenu->setEnabled(!(word & HCfixedResol));
    pd_xlabel->setEnabled(!(word & HCdontCareXoff));
    pd_left->setEnabled(!(word & HCdontCareXoff));
    pd_ylabel->setEnabled(!(word & HCdontCareYoff));
    pd_top->setEnabled(!(word & HCdontCareYoff));

    pd_wlabel->setEnabled(!(word & HCdontCareWidth));
    pd_wid->setEnabled(!(word & HCdontCareWidth));

    pd_hlabel->setEnabled(!(word & HCdontCareHeight));
    pd_hei->setEnabled(!(word & HCdontCareHeight));

    if ((word & HCdontCareXoff) && (word & HCdontCareYoff) &&
            (word & HCdontCareWidth) && (word & HCdontCareHeight)) {
        pd_pgsmenu->setEnabled(false);
        pd_metbtn->setEnabled(false);
    }
    else {
        pd_pgsmenu->setEnabled(true);
        pd_metbtn->setEnabled(true);
        pd_metbtn->setChecked(pd_metric);
        pd_pgsmenu->setCurrentIndex(pd_pgsindex);
    }

    if (word & HCnoLandscape) {
        pd_portbtn->setChecked(true);
        pd_landsbtn->setEnabled(false);
    }
    else {
        pd_landsbtn->setEnabled(true);
        if (pd_orient & HClandscape) {
            pd_portbtn->setChecked(false);
            pd_landsbtn->setChecked(true);
        }
        else {
            pd_portbtn->setChecked(true);
            pd_landsbtn->setChecked(false);
        }
    }
    if (word & HCnoBackground)
        pd_nobg->show();
    else
        pd_nobg->hide();

    if (word & HCnoBestOrient) {
        pd_fitbtn->setChecked(false);
        pd_fitbtn->setEnabled(false);
    }
    else {
        pd_fitbtn->setEnabled(true);
        pd_fitbtn->setChecked(pd_orient & HCbest);
    }
    if (pd_legbtn) {
        if (pd_legend == HClegNone) {
            pd_legbtn->setChecked(false);
            pd_legbtn->setEnabled(false);
            pd_legend = HClegNone;
        }
        else {
            pd_legbtn->setEnabled(true);
            pd_legbtn->setChecked(pd_legend != HClegOff);
        }
    }
    if (word & HCfileOnly) {
        if (!(pd_tofbak & 1))
            pd_tofbak = pd_tofile ? 3 : 1;
        pd_tofbtn->setChecked(true);
        // hc->tofile is now true
        pd_tofbtn->setEnabled(false);
    }
    else {
        if (pd_tofbak & 1) {
            pd_tofile = (pd_tofbak & 2);
            pd_tofbak = 0;
        }
        pd_tofbtn->setEnabled(true);
        pd_tofbtn->setChecked(pd_tofile);
    }
}


void
QTprintDlg::fork_and_submit(const char *str, const char *filename)
{
    // Check for '%s' and substitute filename, otherwise
    // cat the filename.
    bool submade = false;
    const char *s = str;
    char buf[256];
    char *t = buf;
    while (*s) {
        if (*s == '%' && *(s+1) == 's') {
            strcpy(t, filename);
            while (*t)
                t++;
            s += 2;
            submade = true;
        }
        else
            *t++ = *s++;
    }
    if (!submade) {
        *t++ = ' ';
        strcpy(t, filename);
    }
    else
        *t = '\0';

    pd_progress = new QTprogressDlg(pd_owner, QTprogressDlg::prgPrint);
    pd_progress->register_usrptr((void**)&pd_progress);
    pd_progress->set_visible(true);

    if (!pd_process) {
        pd_process = new QProcess(this);
        connect(pd_process, &QProcess::errorOccurred,
            this, &QTprintDlg::process_error_slot);
        connect(pd_process,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &QTprintDlg::process_finished_slot);
    }
    QStringList sl;
    sl << "sh" << "-c" << buf;
    pd_process->start(QString("/bin/sh"), sl);
}


// Static function.
// Sanity check limits/defaults
//
void
QTprintDlg::checklims(HCdesc *desc)
{
    if (desc->limits.minwidth > desc->limits.maxwidth) {
        double tmp = desc->limits.minwidth;
        desc->limits.minwidth = desc->limits.maxwidth;
        desc->limits.maxwidth = tmp;
    }
    if (desc->defaults.defwidth != 0.0 &&
            desc->defaults.defwidth < desc->limits.minwidth)
        desc->defaults.defwidth = desc->limits.minwidth;
    if (desc->defaults.defwidth > desc->limits.maxwidth)
        desc->defaults.defwidth = desc->limits.maxwidth;

    if (desc->limits.minheight > desc->limits.maxheight) {
        double tmp = desc->limits.minheight;
        desc->limits.minheight = desc->limits.maxheight;
        desc->limits.maxheight = tmp;
    }
    if (desc->defaults.defheight != 0.0 &&
            desc->defaults.defheight < desc->limits.minheight)
        desc->defaults.defheight = desc->limits.minheight;
    if (desc->defaults.defheight > desc->limits.maxheight)
        desc->defaults.defheight = desc->limits.maxheight;

    if (desc->defaults.defwidth == 0.0 && desc->defaults.defheight == 0.0)
        desc->defaults.defwidth = desc->limits.minwidth;

    if (desc->limits.minxoff > desc->limits.maxxoff) {
        double tmp = desc->limits.minxoff;
        desc->limits.minxoff = desc->limits.maxxoff;
        desc->limits.maxxoff = tmp;
    }
    if (desc->limits.minxoff > desc->limits.maxwidth)
        desc->limits.minxoff = 0.0;
    if (desc->limits.maxxoff > desc->limits.maxwidth)
        desc->limits.minxoff = desc->limits.maxwidth;
    if (desc->defaults.defxoff < desc->limits.minxoff)
        desc->defaults.defxoff = desc->limits.minxoff;
    if (desc->defaults.defxoff > desc->limits.maxxoff)
        desc->defaults.defxoff = desc->limits.maxxoff;

    if (desc->limits.minyoff > desc->limits.maxyoff) {
        double tmp = desc->limits.minyoff;
        desc->limits.minyoff = desc->limits.maxyoff;
        desc->limits.maxyoff = tmp;
    }
    if (desc->limits.minyoff > desc->limits.maxheight)
        desc->limits.minyoff = 0.0;
    if (desc->limits.maxyoff > desc->limits.maxheight)
        desc->limits.minyoff = desc->limits.maxheight;
    if (desc->defaults.defyoff < desc->limits.minyoff)
        desc->defaults.defyoff = desc->limits.minyoff;
    if (desc->defaults.defyoff > desc->limits.maxyoff)
        desc->defaults.defyoff = desc->limits.maxyoff;
}


// Static function.
// Make an argv-type string array from string str.
//
void
QTprintDlg::mkargv(int *acp, char **av, char *str)
{
    char *s = str;
    int j = 0;
    for (;;) {
        while (isspace(*s)) s++;
        if (!*s) {
            *acp = j;
            return;
        }
        char *t = s;
        while (*t && !isspace(*t)) t++;
        if (*t)
            *t++ = '\0';
        av[j++] = s;
        s = t;
    }
}


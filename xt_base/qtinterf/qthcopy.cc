
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
#include "progress_d.h"
#include "miscutil/lstring.h"
#include "miscutil/filestat.h"

#include <unistd.h>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QGroupBox>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QPushButton>


using namespace qtinterf;

static void checklims(HCdesc*);
static void mkargv(int*, char**, char*);

// Help keywords used in this file:
// hcopypanel

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


QTprintPopup::QTprintPopup(HCcb *cb, HCmode textmode, QTbag *wbag) :
    QDialog(wbag ? wbag->shell : 0)
{
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

    cmdtxtbox = 0;
    cmdlab = 0;
    wlabel = 0;
    hlabel = 0;
    xlabel = 0;
    ylabel = 0;
    wid = 0;
    hei = 0;
    left = 0;
    top = 0;
    portbtn = 0;
    landsbtn = 0;
    fitbtn = 0;
    legbtn = 0;
    tofbtn = 0;
    metbtn = 0;
    a4btn = 0;
    ltrbtn = 0;
    fontmenu = 0;
    fmtmenu = 0;
    resmenu = 0;
    pgsmenu = 0;
    frmbtn = 0;
    hlpbtn = 0;
    printbtn = 0;
    dismissbtn = 0;
    process = 0;
    progress = 0;
    printer_busy = false;

    setWindowTitle(QString(tr("Print Control")));

    if (pd_cb) {
        pd_drvrmask = cb->drvrmask;
        pd_fmt = cb->format;
        int i;
        for (i = 0; GRpkgIf()->HCof(i); i++) ;
        if (pd_fmt >= i || (pd_drvrmask & (1 << pd_fmt))) {
            for (i = 0; GRpkgIf()->HCof(i); i++) {
                if (!(pd_drvrmask & (1 << i)))
                    break;
            }
            if (GRpkgIf()->HCof(i))
                pd_fmt = i;
            else if (pd_textmode == HCgraphical) {
                if (pd_owner)
                    pd_owner->PopUpMessage(
                        "No hardcopy drivers available.", true);
                else if (GRpkgIf()->MainWbag())
                    GRpkgIf()->MainWbag()->PopUpMessage(
                        "No hardcopy drivers available.", true);
                return;
            }
        }
        HCdesc *hcdesc = GRpkgIf()->HCof(pd_fmt);
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
        HCdesc *desc = GRpkgIf()->HCof(pd_fmt);
        pd_wid_val = MM(desc->defaults.defwidth);
        pd_hei_val = MM(desc->defaults.defheight);
        pd_lft_val = MM(desc->defaults.defxoff);
        pd_top_val = MM(desc->defaults.defyoff);
    }
    pd_active = true;

    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setMargin(4);
    vbox->setSpacing(2);

    QHBoxLayout *hbox = new QHBoxLayout(0);
    hbox->setMargin(0);
    hbox->setSpacing(2);

    int row1cnt = 0;
    if (pd_textmode == HCgraphical) {
        if (cb && cb->hcframe) {
            frmbtn = new QPushButton(this);
            frmbtn->setText(QString(tr("Frame")));
            frmbtn->setAutoDefault(false);
            connect(frmbtn, SIGNAL(toggled(bool)),
                this, SLOT(frame_slot(bool)));
            hbox->addWidget(frmbtn);
            row1cnt++;
        }
        fitbtn = new QCheckBox(this);
        fitbtn->setText(QString(tr("Best Fit")));
        connect(fitbtn, SIGNAL(toggled(bool)),
            this, SLOT(best_fit_slot(bool)));
        hbox->addWidget(fitbtn);
        row1cnt++;
        if (!cb || cb->legend != HClegNone) {
            legbtn = new QCheckBox(this);
            legbtn->setText(QString(tr("Legend")));
            connect(legbtn, SIGNAL(toggled(bool)),
                this, SLOT(legend_slot(bool)));
            hbox->addWidget(legbtn);
            row1cnt++;
        }
    }
    else {
        if (!cb || cb->legend != HClegNone) {
            legbtn = new QCheckBox(this);
            legbtn->setText(QString(tr("Legend")));
            hbox->addWidget(legbtn);
            if (pd_legend == HClegNone)
                legbtn->setEnabled(false);
            else
                legbtn->setChecked(pd_legend == HClegOn);
            connect(legbtn, SIGNAL(toggled(bool)),
                this, SLOT(legend_slot(bool)));
            row1cnt++;
        }
        if (pd_textmode == HCtextPS) {
            a4btn = new QCheckBox(this);
            a4btn->setText(QString(tr("A4")));
            connect(a4btn, SIGNAL(toggled(bool)),
                this, SLOT(a4_slot(bool)));
            hbox->addWidget(a4btn);
            ltrbtn = new QCheckBox(this);
            ltrbtn->setText(QString(tr("Letter")));
            connect(ltrbtn, SIGNAL(toggled(bool)),
                this, SLOT(letter_slot(bool)));
            hbox->addWidget(ltrbtn);
            hbox->addSpacing(20);
            if (pd_metric)
                a4btn->setChecked(true);
            else
                ltrbtn->setChecked(true);
            row1cnt++;

            pd_textfmt = PStimes;  // default postscript times

            fontmenu = new QComboBox(this);
            int entries = sizeof(textfonts)/sizeof(sFonts);
            int hist = -1;
            for (int i = 0; i < entries; i++) {
                fontmenu->addItem(QString(textfonts[i].descr));
                if (pd_textfmt == textfonts[i].type)
                    hist = i;
            }
            if (hist < 0) {
                hist = 0;
                pd_textfmt = textfonts[0].type;
            }
            fontmenu->setCurrentIndex(hist);
            connect(fontmenu, SIGNAL(activated(int)),
                this, SLOT(font_slot(int)));
            hbox->addWidget(fontmenu);
            row1cnt++;
        }
    }
    // Don't leave a lonely help button in the top row, move it to the
    // second row.
    if (row1cnt) {
        hlpbtn = new QPushButton(this);
        hlpbtn->setText(QString(tr("Help")));
        hlpbtn->setAutoDefault(false);
        hbox->addWidget(hlpbtn);
        vbox->addLayout(hbox);
    }
    else
        delete hbox;

    QGroupBox *gb = new QGroupBox(this);
    hbox = new QHBoxLayout(gb);
    hbox->setMargin(4);
    hbox->setSpacing(2);
    cmdlab = new QLabel(gb);
    cmdlab->setText(QString(tr(pd_tofile ? "File Name" : "Print Command")));
    hbox->addWidget(cmdlab);
    tofbtn = new QCheckBox(gb);
    tofbtn->setText(QString(tr("To File")));
    tofbtn->setChecked(pd_tofile);
    connect(tofbtn, SIGNAL(toggled(bool)), this, SLOT(tofile_slot(bool)));
    hbox->addWidget(tofbtn);
    if (row1cnt)
        vbox->addWidget(gb);
    else {
        hbox = new QHBoxLayout(0);
        hbox->setMargin(0);
        hbox->setSpacing(2);
        hbox->addWidget(gb);
        hlpbtn = new QPushButton(this);
        hlpbtn->setText(QString(tr("Help")));
        hlpbtn->setAutoDefault(false);
        hbox->addWidget(hlpbtn);
        vbox->addLayout(hbox);
    }
    connect(hlpbtn, SIGNAL(clicked()), this, SLOT(help_slot()));

    cmdtxtbox = new QLineEdit(this);
    cmdtxtbox->setText(QString(pd_tofile ? pd_tofilename : pd_cmdtext));
    vbox->addWidget(cmdtxtbox);

    if (pd_textmode == HCgraphical) {
        hbox = new QHBoxLayout(0);
        hbox->setMargin(0);
        hbox->setSpacing(2);
        QVBoxLayout *vb = new QVBoxLayout(0);

        gb = new QGroupBox(this);
        QHBoxLayout *hb = new QHBoxLayout(gb);
        hb->setMargin(2);
        hb->setSpacing(2);
        portbtn = new QCheckBox(gb);
        portbtn->setText(QString(tr("Portrait")));
        hb->addWidget(portbtn);
        connect(portbtn, SIGNAL(toggled(bool)),
            this, SLOT(portrait_slot(bool)));
        landsbtn = new QCheckBox(gb);
        landsbtn->setText(QString(tr("Landscape")));
        hb->addWidget(landsbtn);
        connect(landsbtn, SIGNAL(toggled(bool)),
            this, SLOT(landscape_slot(bool)));
        vb->addWidget(gb);

        fmtmenu = new QComboBox(this);
        for (int i = 0; GRpkgIf()->HCof(i); i++) {
            if (pd_drvrmask & (1 << i))
                continue;
            fmtmenu->addItem(QString(GRpkgIf()->HCof(i)->descr));
        }
        fmtmenu->setCurrentIndex(pd_fmt);
        connect(fmtmenu, SIGNAL(activated(int)), this, SLOT(format_slot(int)));
        vb->addWidget(fmtmenu);
        hbox->addLayout(vb);

        vb = new QVBoxLayout(0);

        gb = new QGroupBox(this);
        hb = new QHBoxLayout(gb);
        hb->setMargin(2);
        hb->setSpacing(2);
        QLabel *res = new QLabel(gb);
        res->setText(QString(tr("Resolution")));
        hb->addWidget(res);
        vb->addWidget(gb);

        resmenu = new QComboBox(this);
        const char **s = GRpkgIf()->HCof(pd_fmt)->limits.resols;
        if (s && *s) {
            for (int i = 0; s[i]; i++)
                resmenu->addItem(QString(s[i]));
        }
        else
            resmenu->addItem(QString("fixed"));
        resmenu->setCurrentIndex(pd_resol);
        connect(resmenu, SIGNAL(activated(int)), this, SLOT(resol_slot(int)));
        vb->addWidget(resmenu);
        hbox->addLayout(vb);
        vbox->addLayout(hbox);

        hbox = new QHBoxLayout(0);
        hbox->setMargin(0);
        hbox->setSpacing(2);

        HCdesc *desc = GRpkgIf()->HCof(pd_fmt);
        vb = new QVBoxLayout(0);
        gb = new QGroupBox(this);
        hb = new QHBoxLayout(gb);
        hb->setMargin(2);
        wlabel = new QCheckBox(gb);
        wlabel->setText(QString(tr("Width")));
        connect(wlabel, SIGNAL(toggled(bool)),
            this, SLOT(auto_width_slot(bool)));
        hb->addWidget(wlabel);
        vb->addWidget(gb);
        wid = new QDoubleSpinBox(this);
        wid->setDecimals(2);
        wid->setMinimum(MM(desc->limits.minwidth));
        wid->setMaximum(MM(desc->limits.maxwidth));
        wid->setValue(pd_wid_val);
        wid->setSingleStep(0.1);
        vb->addWidget(wid);
        hbox->addLayout(vb);

        vb = new QVBoxLayout(0);
        gb = new QGroupBox(this);
        hb = new QHBoxLayout(gb);
        hb->setMargin(2);
        hlabel = new QCheckBox(gb);
        hlabel->setText(QString(tr("Height")));
        connect(hlabel, SIGNAL(toggled(bool)),
            this, SLOT(auto_height_slot(bool)));
        hb->addWidget(hlabel);
        vb->addWidget(gb);
        hei = new QDoubleSpinBox(this);
        hei->setDecimals(2);
        hei->setMinimum(MM(desc->limits.minheight));
        hei->setMaximum(MM(desc->limits.maxheight));
        hei->setValue(pd_hei_val);
        hei->setSingleStep(0.1);
        vb->addWidget(hei);
        hbox->addLayout(vb);

        vb = new QVBoxLayout(0);
        gb = new QGroupBox(this);
        hb = new QHBoxLayout(gb);
        hb->setMargin(2);
        xlabel = new QLabel(gb);
        xlabel->setText(QString(tr("Left")));
        hb->addWidget(xlabel);
        vb->addWidget(gb);
        left = new QDoubleSpinBox(this);
        left->setDecimals(2);
        left->setMinimum(MM(desc->limits.minxoff));
        left->setMaximum(MM(desc->limits.maxxoff));
        left->setValue(pd_lft_val);
        left->setSingleStep(0.1);
        vb->addWidget(left);
        hbox->addLayout(vb);

        vb = new QVBoxLayout(0);
        gb = new QGroupBox(this);
        hb = new QHBoxLayout(gb);
        hb->setMargin(2);
        ylabel = new QLabel(gb);
        ylabel->setText(QString(tr("Bottom")));
        hb->addWidget(ylabel);
        vb->addWidget(gb);
        top = new QDoubleSpinBox(this);
        top->setDecimals(2);
        top->setMinimum(MM(desc->limits.minyoff));
        top->setMaximum(MM(desc->limits.maxyoff));
        top->setValue(pd_top_val);
        top->setSingleStep(0.1);
        vb->addWidget(top);
        hbox->addLayout(vb);

        vbox->addLayout(hbox);

        hbox = new QHBoxLayout(0);
        hbox->setMargin(0);
        hbox->setSpacing(2);

        pgsmenu = new QComboBox(this);
        hbox->addWidget(pgsmenu);
        int i = 0;
        for (sMedia *m = pagesizes; m->name; m++, i++)
            pgsmenu->addItem(QString(m->name));
        connect(pgsmenu, SIGNAL(activated(int)),
            this, SLOT(pagesize_slot(int)));
        metbtn = new QCheckBox(this);
        metbtn->setText(QString(tr("Metric (mm)")));
        metbtn->setChecked(pd_metric);
        connect(metbtn, SIGNAL(toggled(bool)), this, SLOT(metric_slot(bool)));
        hbox->addSpacing(20);
        hbox->addWidget(metbtn);
        vbox->addLayout(hbox);
    }

    hbox = new QHBoxLayout(0);
    hbox->setMargin(0);
    hbox->setSpacing(2);

    printbtn = new QPushButton(this);
    printbtn->setText(QString(tr("Print")));
    printbtn->setAutoDefault(false);
    connect(printbtn, SIGNAL(clicked()), this, SLOT(print_slot()));
    hbox->addWidget(printbtn);
    dismissbtn = new QPushButton(this);
    dismissbtn->setText(QString(tr("Dismiss")));
    connect(dismissbtn, SIGNAL(clicked()), this, SLOT(quit_slot()));
    hbox->addWidget(dismissbtn);
    vbox->addLayout(hbox);
    dismissbtn->setFocus(Qt::ActiveWindowFocusReason);

    if (pd_cb && pd_cb->hcsetup)
        (*pd_cb->hcsetup)(true, pd_fmt, false, 0);

    if (pd_textmode == HCgraphical) {
        HCdesc *desc = GRpkgIf()->HCof(pd_fmt);
        set_sens(desc->limits.flags);
        if (!(desc->limits.flags & HCdontCareWidth) && pd_wid_val == 0.0 &&
                desc->limits.minwidth > 0.0) {
            if (desc->limits.flags & HCnoAutoWid) {
                pd_wid_val = MM(desc->limits.minwidth);
                desc->defaults.defwidth = desc->limits.minwidth;
            }
            else {
                float tmp = desc->last_w;
                wlabel->setChecked(true);
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
                hlabel->setChecked(true);
                desc->last_h = tmp;
            }
        }
    }

    // Success, link into owning QTbag.
    if (pd_owner) {
        if (pd_owner->hc)
            delete pd_owner->hc;
        pd_owner->hc = this;
    }
}


QTprintPopup::~QTprintPopup()
{
    if (pd_owner)
        pd_owner->hc = 0;
}


// Function to query values for command text, resolution, etc.
// The HCcb struct is filled in with the present values.
//
void
QTprintPopup::update(HCcb *cb)
{
    if (!cb)
        return;
    cb->hcsetup = pd_cb ? pd_cb->hcsetup : 0;
    cb->hcgo = pd_cb ? pd_cb->hcgo : 0;
    cb->hcframe = pd_cb ? pd_cb->hcframe : 0;

    char *s = lstring::copy(cmdtxtbox->text().toLatin1().constData());
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
    d = left->value();
    if (pd_metric)
        d /= MMPI;
    cb->left = d;
    d = top->value();
    if (pd_metric)
        d /= MMPI;
    cb->top = d;
    d = wid->value();
    if (pd_metric)
        d /= MMPI;
    cb->width = d;
    d = hei->value();
    if (pd_metric)
        d /= MMPI;
    cb->height = d;
}


// Print a message to the progress monitor.
//
void
QTprintPopup::set_message(const char *msg)
{
    if (progress)
        progress->set_etc(msg);
}


void
QTprintPopup::disable_progress()
{
    delete progress;
}


void
QTprintPopup::set_active(bool set)
{
    if (set) {
        if (!pd_active) {
            pd_active = true;
            show();
            raise();
            activateWindow();
        }
    }
    else {
        if (pd_active) {
            pd_active = false;
            hide();
        }
    }
}


// Font change handler.
//
void
QTprintPopup::format_slot(int indx)
{
    if (wlabel->isChecked())
        wlabel->setChecked(false);
    if (hlabel->isChecked())
        hlabel->setChecked(false);

    if (pd_drvrmask & (1 << indx))
        return;;
    int i = pd_fmt;
    pd_fmt = indx;
    // set the current defaults to the current values
    if (GRpkgIf()->HCof(i)->defaults.command)
        delete [] GRpkgIf()->HCof(i)->defaults.command;
    if (!pd_tofile)
        GRpkgIf()->HCof(i)->defaults.command =
            lstring::copy(cmdtxtbox->text().toLatin1().constData());
    else
        GRpkgIf()->HCof(i)->defaults.command = lstring::copy(pd_cmdtext);
    GRpkgIf()->HCof(i)->defaults.defresol = pd_resol;

    GRpkgIf()->HCof(i)->defaults.legend = pd_legend;
    pd_legend = GRpkgIf()->HCof(pd_fmt)->defaults.legend;
    GRpkgIf()->HCof(i)->defaults.orient = pd_orient;
    pd_orient = GRpkgIf()->HCof(pd_fmt)->defaults.orient;

    double w = wid->value();
    if (pd_metric)
        w /= MMPI;
    GRpkgIf()->HCof(i)->defaults.defwidth = w;
    double h = hei->value();
    if (pd_metric)
        h /= MMPI;
    GRpkgIf()->HCof(i)->defaults.defheight = h;
    double xx = left->value();
    if (pd_metric)
        xx /= MMPI;
    GRpkgIf()->HCof(i)->defaults.defxoff = xx;
    double yy = top->value();
    if (pd_metric)
        yy /= MMPI;
    GRpkgIf()->HCof(i)->defaults.defyoff = yy;

    HCdesc *desc = GRpkgIf()->HCof(pd_fmt);
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
        cmdtxtbox->setText(QString(pd_cmdtext));

    pd_resol = desc->defaults.defresol;

    wid->setMinimum(MM(desc->limits.minwidth));
    wid->setMaximum(MM(desc->limits.maxwidth));
    wid->setValue(MM(desc->defaults.defwidth));

    hei->setMinimum(MM(desc->limits.minheight));
    hei->setMaximum(MM(desc->limits.maxheight));
    hei->setValue(MM(desc->defaults.defheight));

    left->setMinimum(MM(desc->limits.minxoff));
    left->setMaximum(MM(desc->limits.maxxoff));
    left->setValue(MM(desc->defaults.defxoff));

    top->setMinimum(MM(desc->limits.minyoff));
    top->setMaximum(MM(desc->limits.maxyoff));
    top->setValue(MM(desc->defaults.defyoff));

    if (desc->limits.flags & HCtopMargin)
        ylabel->setText(QString(tr("Top")));
    else
        ylabel->setText(QString(tr("Bottom")));

    set_sens(desc->limits.flags);
    if (!(desc->limits.flags & HCdontCareWidth) &&
            desc->defaults.defwidth == 0.0 && desc->limits.minwidth > 0.0) {
        if (desc->limits.flags & HCnoAutoWid)
            desc->defaults.defwidth = desc->limits.minwidth;
        else
            wlabel->setChecked(true);
    }
    if (!(desc->limits.flags & HCdontCareHeight) &&
            desc->defaults.defheight == 0.0 && desc->limits.minheight > 0.0) {
        if (desc->limits.flags & HCnoAutoHei)
            desc->defaults.defheight = desc->limits.minheight;
        else
            hlabel->setChecked(true);
    }

    resmenu->clear();
    const char **s = desc->limits.resols;
    if (s && *s) {
        for (int j = 0; s[j]; j++)
            resmenu->addItem(QString(s[j]));
    }
    else
        resmenu->addItem(QString("fixed"));
    resmenu->setCurrentIndex(pd_resol);

    if (pd_cb && pd_cb->hcsetup)
        (*pd_cb->hcsetup)(true, pd_fmt, false, 0);
}


// Set the printer resolution.
//
void
QTprintPopup::resol_slot(int indx)
{
    pd_resol = indx;
}


// Handler for the "font" menu.
//
void
QTprintPopup::font_slot(int indx)
{
    pd_textfmt = textfonts[indx].type;
}


// Handler for the page size menu.
//
void
QTprintPopup::pagesize_slot(int indx)
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
    wid->setValue(w/72);
    hei->setValue(h/72);
    left->setValue(shrink/72);
    top->setValue(shrink/72);
}


void
QTprintPopup::a4_slot(bool set)
{
    if (set) {
        if (ltrbtn->isChecked())
            ltrbtn->setChecked(false);
    }
    else if (!ltrbtn->isChecked())
        a4btn->setChecked(true);
    // This is text-only mode, set the metric field for A4.
    pd_metric = a4btn->isChecked();
}


void
QTprintPopup::letter_slot(bool set)
{
    if (set) {
        if (a4btn->isChecked())
            a4btn->setChecked(false);
    }
    else if (!a4btn->isChecked())
        ltrbtn->setChecked(true);
    pd_metric = a4btn->isChecked();
}


void
QTprintPopup::metric_slot(bool set)
{
    bool wasmetric = pd_metric;
    pd_metric = set;

    if (wasmetric != pd_metric) {
        double d;

        d = wid->value();
        if (wasmetric) {
            d /= MMPI;
            wid->setMinimum(wid->minimum()/MMPI);
            wid->setMaximum(wid->maximum()/MMPI);
            wid->setValue(d);
        }
        else {
            d *= MMPI;
            wid->setMinimum(wid->minimum()*MMPI);
            wid->setMaximum(wid->maximum()*MMPI);
            wid->setValue(d);
        }

        d = hei->value();
        if (wasmetric) {
            d /= MMPI;
            hei->setMinimum(hei->minimum()/MMPI);
            hei->setMaximum(hei->maximum()/MMPI);
            hei->setValue(d);
        }
        else {
            d *= MMPI;
            hei->setMinimum(hei->minimum()*MMPI);
            hei->setMaximum(hei->maximum()*MMPI);
            hei->setValue(d);
        }

        d = left->value();
        if (wasmetric) {
            d /= MMPI;
            left->setMinimum(left->minimum()/MMPI);
            left->setMaximum(left->maximum()/MMPI);
            left->setValue(d);
        }
        else {
            d *= MMPI;
            left->setMinimum(left->minimum()*MMPI);
            left->setMaximum(left->maximum()*MMPI);
            left->setValue(d);
        }

        d = top->value();
        if (wasmetric) {
            d /= MMPI;
            top->setMinimum(top->minimum()/MMPI);
            top->setMaximum(top->maximum()/MMPI);
            top->setValue(d);
        }
        else {
            d *= MMPI;
            top->setMinimum(top->minimum()*MMPI);
            top->setMaximum(top->maximum()*MMPI);
            top->setValue(d);
        }
    }
}


// Frame callback.  This allows the user to define the area  of
// graphics to be printed.
//
void
QTprintPopup::frame_slot(bool)
{
    if (pd_cb && pd_cb->hcframe)
        (*pd_cb->hcframe)(HCframeCmd, 0, 0, 0, 0, 0, 0);
}


// Switch between portrait and landscape orientation.
//
void
QTprintPopup::portrait_slot(bool set)
{
    if (set) {
        if (landsbtn && landsbtn->isChecked())
            landsbtn->setChecked(false);
        if (!(pd_orient & HClandscape))
            return;
        pd_orient &= ~HClandscape;
        // See if we should swap the margin label
        if (GRpkgIf()->HCof(pd_fmt)->limits.flags & HClandsSwpYmarg) {
            const char *str = ylabel->text().toLatin1().constData();
            if (!strcmp(str, "Top"))
                ylabel->setText(QString("Bottom"));
            else
                ylabel->setText(QString("Top"));
        }
    }
    else if (!landsbtn->isChecked())
        portbtn->setChecked(true);
}


void
QTprintPopup::landscape_slot(bool set)
{
    if (set) {
        if (portbtn->isChecked())
            portbtn->setChecked(false);
        if (pd_orient & HClandscape)
            return;
        pd_orient |= HClandscape;
        // See if we should swap the margin label
        if (GRpkgIf()->HCof(pd_fmt)->limits.flags & HClandsSwpYmarg) {
            const char *str = ylabel->text().toLatin1().constData();
            if (!strcmp(str, "Top"))
                ylabel->setText(QString("Bottom"));
            else
                ylabel->setText(QString("Top"));
        }
    }
    else if (!portbtn->isChecked())
        landsbtn->setChecked(true);
}


// If the "best fit" button is active, allow rotation of the image.
//
void
QTprintPopup::best_fit_slot(bool set)
{
    if (set)
        pd_orient |= HCbest;
    else
        pd_orient &= ~HCbest;
}


// Send the output to a file, rather than a printer.
//
void
QTprintPopup::tofile_slot(bool set)
{
    pd_tofile = set;
    const char *s = cmdtxtbox->text().toLatin1().constData();
    if (set) {
        cmdlab->setText(QString("File Name"));
        delete [] pd_cmdtext;
        pd_cmdtext = lstring::copy(s);
        cmdtxtbox->setText(QString(pd_tofilename));
    }
    else {
        cmdlab->setText(QString("Print Command"));
        delete [] pd_tofilename;
        pd_tofilename = lstring::copy(s);
        cmdtxtbox->setText(QString(pd_cmdtext));
    }
}


// Toggle display of the legend associated with the plot.
//
void
QTprintPopup::legend_slot(bool set)
{
    pd_legend = (set ? HClegOn : HClegOff);
}


// Handle the auto-height and auto-width buttons
//
void
QTprintPopup::auto_width_slot(bool set)
{
    if (set) {
        GRpkgIf()->HCof(pd_fmt)->last_w = wid->value();
        if (pd_metric)
            GRpkgIf()->HCof(pd_fmt)->last_w /= MMPI;
        GRpkgIf()->HCof(pd_fmt)->defaults.defwidth = 0;
        wid->setEnabled(false);
        wid->setPrefix(QString("Auto"));
        wid->clear();
        if (hlabel->isChecked()) {
            hlabel->setChecked(false);
            hei->setEnabled(true);
            double h = GRpkgIf()->HCof(pd_fmt)->last_h;
            if (h == 0.0)
                h = GRpkgIf()->HCof(pd_fmt)->limits.minheight;
            hei->setValue(MM(h));
        }
    }
    else {
        wid->setPrefix(QString());
        wid->setEnabled(true);
        double w = GRpkgIf()->HCof(pd_fmt)->last_w;
        if (w == 0.0)
            w = GRpkgIf()->HCof(pd_fmt)->limits.minwidth;
        wid->setValue(MM(w));
    }
}


void
QTprintPopup::auto_height_slot(bool set)
{
    if (set) {
        GRpkgIf()->HCof(pd_fmt)->last_h = hei->value();
        if (pd_metric)
            GRpkgIf()->HCof(pd_fmt)->last_h /= MMPI;
        GRpkgIf()->HCof(pd_fmt)->defaults.defheight = 0;
        hei->setEnabled(false);
        hei->setPrefix(QString("Auto"));
        hei->clear();
        if (wlabel->isChecked()) {
            wlabel->setChecked(false);
            wid->setEnabled(true);
            double w = GRpkgIf()->HCof(pd_fmt)->last_w;
            if (w == 0.0)
                w = GRpkgIf()->HCof(pd_fmt)->limits.minwidth;
            wid->setValue(MM(w));
        }
    }
    else {
        hei->setPrefix(QString());
        hei->setEnabled(true);
        double h = GRpkgIf()->HCof(pd_fmt)->last_h;
        if (h == 0.0)
            h = GRpkgIf()->HCof(pd_fmt)->limits.minheight;
        hei->setValue(MM(h));
    }
}


// Handler for button presses within the widget not caught by subwidgets,
// for help mode.
//
void
QTprintPopup::help_slot()
{
    if (pd_owner)
        pd_owner->PopUpHelp("hcopypanel");
    else if (GRpkgIf()->MainWbag())
        GRpkgIf()->MainWbag()->PopUpHelp("hcopypanel");
}


#define MAX_ARGS 20

// Callback to actually generate the hardcopy.
//
void
QTprintPopup::print_slot()
{
    GRpkgIf()->HCabort(0);
    if (!pd_owner)
        return;

    const char *str = cmdtxtbox->text().toLatin1().constData();
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
                    const char *st = cmdtxtbox->text().toLatin1().constData();
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
            int fontcode = 0;
            switch (pd_textfmt) {
            case PlainText:
            case PrettyText:
                text = pd_owner->GetPlainText();
                break;
            case PSlucida:
                fontcode++;
            case PScentury:
                fontcode++;
            case PShelv:
                fontcode++;
            case PStimes:
                text = pd_owner->GetPostscriptText(fontcode, 0, 0, true,
                    pd_metric);
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
                const char *st = cmdtxtbox->text().toLatin1().constData();
                fork_and_submit(st, filename);
                // note that the file is NOT unlinked
            }
            else
                pd_owner->PopUpMessage("Text saved", false);
        }
        delete [] filename;
        return;
    }

    if (printer_busy) {
        pd_owner->PopUpMessage("I'm busy, please wait.", true);
        return;
    }
    printer_busy = true;

    double w = 0.0;
    if (!(GRpkgIf()->HCof(pd_fmt)->limits.flags & HCdontCareWidth)) {
        if (!wlabel->isChecked()) {
            w = wid->value();
            if (pd_metric)
                w /= MMPI;
        }
    }
    double h = 0.0;
    if (!(GRpkgIf()->HCof(pd_fmt)->limits.flags & HCdontCareHeight)) {
        if (!hlabel->isChecked()) {
            h = hei->value();
            if (pd_metric)
                h /= MMPI;
        }
    }
    double xx = 0.0;
    if (!(GRpkgIf()->HCof(pd_fmt)->limits.flags & HCdontCareXoff)) {
        xx = left->value();
        if (pd_metric)
            xx /= MMPI;
    }
    double yy = 0.0;
    if (!(GRpkgIf()->HCof(pd_fmt)->limits.flags & HCdontCareYoff)) {
        yy = top->value();
        if (pd_metric)
            yy /= MMPI;
    }
    if (!pd_tofile)
        filename = filestat::make_temp("hc");
    else
        filename = lstring::copy(str);
    int resol = 0;
    if (GRpkgIf()->HCof(pd_fmt)->limits.resols)
        sscanf(GRpkgIf()->HCof(pd_fmt)->limits.resols[pd_resol], "%d",
            &resol);
    snprintf(buf, sizeof(buf), GRpkgIf()->HCof(pd_fmt)->fmtstring, filename,
        resol, w, h, xx, yy);
    if (pd_orient & HClandscape)
        strcat(buf, " -l");
    char *cmdstr = lstring::copy(buf);
    char *argv[MAX_ARGS];
    int argc;
    mkargv(&argc, argv, cmdstr);

    HCswitchErr err =
        GRpkgIf()->SwitchDev(GRpkgIf()->HCof(pd_fmt)->drname, &argc, argv);
    if (err == HCSinhc)
        pd_owner->PopUpMessage("Internal error - aborted", true);
    else if (err == HCSnotfnd) {
        snprintf(buf, sizeof(buf), "No hardcopy driver named %s available",
            GRpkgIf()->HCof(pd_fmt)->drname);
        pd_owner->PopUpMessage(buf, true);
    }
    else if (err == HCSinit)
        pd_owner->PopUpMessage("Init callback failed - aborted", true);
    else {
        bool ok = true;
        // hc might be freed during hcgo
        bool tofile = pd_tofile;
        char *cmd = lstring::copy(cmdtxtbox->text().toLatin1().constData());

        if (pd_cb && pd_cb->hcgo) {
            HCorientFlags ot = pd_orient & HCbest;
            // pass the landscape flag only if the driver can't rotate
            if ((pd_orient & HClandscape) &&
                    (GRpkgIf()->HCof(pd_fmt)->limits.flags & HCnoCanRotate))
                ot |= HClandscape;
            if ((*pd_cb->hcgo)(ot, pd_legend, 0)) {
                if (GRpkgIf()->HCaborted()) {
                    snprintf(buf, sizeof(buf), "Terminated: %s.",
                        GRpkgIf()->HCabortMsg());
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
        GRpkgIf()->SwitchDev(0, 0, 0);
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
    printer_busy = false;
}


void
QTprintPopup::quit_slot()
{
    set_active(false);  // keep the pop-up alive
    emit dismiss();
}


void
QTprintPopup::process_error_slot(QProcess::ProcessError err)
{
    if (progress) {
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
        progress->set_etc(buf);
    }
}


void
QTprintPopup::process_finished_slot(int code)
{
    if (progress) {
        char buf[256];
        if (code == 0)
            strcpy(buf, "Job completed successfully.");
        else {
            snprintf(buf, sizeof(buf), "Job completed with exit status %d.",
                code);
        }
        progress->set_etc(buf);
        progress->finished();
    }
}


void
QTprintPopup::set_sens(unsigned int word)
{
    resmenu->setEnabled(!(word & HCfixedResol));
    xlabel->setEnabled(!(word & HCdontCareXoff));
    left->setEnabled(!(word & HCdontCareXoff));
    ylabel->setEnabled(!(word & HCdontCareYoff));
    top->setEnabled(!(word & HCdontCareYoff));

    wlabel->setEnabled(!(word & HCdontCareWidth));
    wid->setEnabled(!(word & HCdontCareWidth));

    hlabel->setEnabled(!(word & HCdontCareHeight));
    hei->setEnabled(!(word & HCdontCareHeight));

    if ((word & HCdontCareXoff) && (word & HCdontCareYoff) &&
            (word & HCdontCareWidth) && (word & HCdontCareHeight)) {
        pgsmenu->setEnabled(false);
        metbtn->setEnabled(false);
    }
    else {
        pgsmenu->setEnabled(true);
        metbtn->setEnabled(true);
        metbtn->setChecked(pd_metric);
        pgsmenu->setCurrentIndex(pd_pgsindex);
    }

    if (word & HCnoLandscape) {
        portbtn->setChecked(true);
        landsbtn->setEnabled(false);
    }
    else {
        landsbtn->setEnabled(true);
        if (pd_orient & HClandscape) {
            portbtn->setChecked(false);
            landsbtn->setChecked(true);
        }
        else {
            portbtn->setChecked(true);
            landsbtn->setChecked(false);
        }
    }

    if (word & HCnoBestOrient) {
        fitbtn->setChecked(false);
        fitbtn->setEnabled(false);
    }
    else {
        fitbtn->setEnabled(true);
        fitbtn->setChecked(pd_orient & HCbest);
    }
    if (legbtn) {
        if (pd_legend == HClegNone) {
            legbtn->setChecked(false);
            legbtn->setEnabled(false);
            pd_legend = HClegNone;
        }
        else {
            legbtn->setEnabled(true);
            legbtn->setChecked(pd_legend != HClegOff);
        }
    }
    if (word & HCfileOnly) {
        if (!(pd_tofbak & 1))
            pd_tofbak = pd_tofile ? 3 : 1;
        tofbtn->setChecked(true);
        // hc->tofile is now true
        tofbtn->setEnabled(false);
    }
    else {
        if (pd_tofbak & 1) {
            pd_tofile = (pd_tofbak & 2);
            pd_tofbak = 0;
        }
        tofbtn->setEnabled(true);
        tofbtn->setChecked(pd_tofile);
    }
}


void
QTprintPopup::fork_and_submit(const char *str, const char *filename)
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

    progress = new progress_d(pd_owner, progress_d::prgPrint);
    progress->register_usrptr((void**)&progress);
    progress->set_visible(true);

    if (!process) {
        process = new QProcess(this);
        connect(process, SIGNAL(error(QProcess::ProcessError)),
            this, SLOT(process_error_slot(QProcess::ProcessError)));
        connect(process, SIGNAL(finished(int)),
            this, SLOT(process_finished_slot(int)));
    }
    QStringList sl;
    sl << "sh" << "-c" << buf;
    process->start(QString("/bin/sh"), sl);
}


// Sanity check limits/defaults
//
static void
checklims(HCdesc *desc)
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


// Make an argv-type string array from string str.
//
static void
mkargv(int *acp, char **av, char *str)
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


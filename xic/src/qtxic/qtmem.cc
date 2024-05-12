
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
 * Xic Integrated Circuit Layout and Schematic Editor                     *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#include "config.h"
#include "qtmem.h"
#include "cd_celldb.h"
#include "dsp_color.h"
#include "menu.h"
#include "view_menu.h"
#include "qtinterf/qtfont.h"
#ifdef HAVE_LOCAL_ALLOCATOR
#include "malloc/local_malloc.h"
#else
#include "miscutil/coresize.h"
#endif

#include <sys/types.h>
#include <sys/time.h>
#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#else
#ifdef Q_OS_WIN
#include <windows.h>
#endif
#endif

#include <QApplication>
#include <QLayout>
#include <QPushButton>
#include "qtinterf/qtcanvas.h"


//-----------------------------------------------------------------------------
// QTmemMonDlg:  Memory Monitor dialog. Prints current memory use.
// Called from main menu: View/Allocation.

void
cMain::PopUpMemory(ShowMode mode)
{
    if (!QTdev::exists() || !QTmainwin::exists())
        return;
    if (mode == MODE_OFF) {
        delete QTmemMonDlg::self();
        return;
    }
    if (QTmemMonDlg::self()) {
        QTmemMonDlg::self()->update();
        return;
    }
    if (mode == MODE_UPD)
        return;

    new QTmemMonDlg;

    QTmemMonDlg::self()->set_transient_for(QTmainwin::self());
    QTdev::self()->SetPopupLocation(GRloc(), QTmemMonDlg::self(),
        QTmainwin::self()->Viewport());
    QTmemMonDlg::self()->show();
}
// End of cMain functions.


// Minimum widget width so that title text isn't truncated.
#define MEM_MINWIDTH 240

QTmemMonDlg *QTmemMonDlg::instPtr;

QTmemMonDlg::QTmemMonDlg() : QTbag(this), QTdraw(XW_TEXT)
{
    instPtr = this;

    setWindowTitle(tr("Memory Monitor"));
    setAttribute(Qt::WA_DeleteOnClose);

    QMargins qmtop(2, 2, 2, 2);
    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(qmtop);
    vbox->setSpacing(2);

    // The drawing area.
    //
    gd_viewport = new QTcanvas();
    vbox->addWidget(gd_viewport);

    // The dismiss button.
    //
    QPushButton *btn = new QPushButton(tr("Dismiss"));
    btn->setObjectName("Default");
    vbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(dismiss_btn_slot()));

    connect(gd_viewport, SIGNAL(resize_event(QResizeEvent*)),
        this, SLOT(resize_slot(QResizeEvent*)));

    // Font setup.
    //
    QFont *fnt;
    if (Fnt()->getFont(&fnt, FNT_FIXED)) {
        gd_viewport->setFont(*fnt);
    }
    setMinimumWidth(sizeHint().width());
    setMinimumHeight(sizeHint().height());
    connect(QTfont::self(), SIGNAL(fontChanged(int)),
        this, SLOT(font_changed_slot(int)), Qt::QueuedConnection);

    QTpkg::self()->RegisterTimeoutProc(5000, mem_proc, 0);
}


QTmemMonDlg::~QTmemMonDlg()
{
    instPtr = 0;
    MainMenu()->MenuButtonSet(0, MenuALLOC, false);
}


#ifdef Q_OS_MACOS
#define DLGTYPE QTmemMonDlg
#include "qtinterf/qtmacos_event.h"
#endif


QSize
QTmemMonDlg::sizeHint() const
{
    int fw = QTfont::stringWidth(0, gd_viewport);
    int fh = QTfont::lineHeight(gd_viewport);
    int ww = 40*fw + 4;
    if (ww < MEM_MINWIDTH)
        ww = MEM_MINWIDTH;
    int wh = 5*fh + 50;
    return (QSize(ww, wh));
}


void
QTmemMonDlg::update()
{
    unsigned long c1 = DSP()->Color(PromptTextColor);
    unsigned long c2 = DSP()->Color(PromptEditTextColor);
    int fwid, fhei;
    TextExtent(0, &fwid, &fhei);
    SetWindowBackground(QTdev::self()->NameColor("white"));
    SetFillpattern(0);
    SetLinestyle(0);
    Clear();
    char buf[128];

    int xx = 2;
    int yy = fhei;
    int spw = fwid;

    SetColor(c1);
    const char *str = "Cells:";
    Text(str, xx, yy, 0);
    xx += 7 * spw;
    snprintf(buf, sizeof(buf), "phys=%d elec=%d", CDcdb()->cellCount(Physical),
        CDcdb()->cellCount(Electrical));
    str = buf;
    SetColor(c2);
    Text(str, xx, yy, 0);

    xx = 2;
    yy += fhei;
    SetColor(c1);
    char b;
#ifdef HAVE_LOCAL_ALLOCATOR
    double v = chk_val(Memory()->coresize(), &b);
#else
    double v = chk_val(coresize(), &b);
#endif
    snprintf(buf, sizeof(buf), "Current Datasize (%cB):", b);
    Text(buf, xx, yy, 0);
    xx += 24 * spw;
    snprintf(buf, sizeof(buf), "%.3f", v);
    SetColor(c2);
    Text(buf, xx, yy, 0);

#ifdef HAVE_SYS_RESOURCE_H
    rlimit rl;
    if (getrlimit(RLIMIT_DATA, &rl)) {      // data segment limit
        Update();
        return;
    }

    rlimit rl2;
    if (getrlimit(RLIMIT_AS, &rl2)) {       // mmap limit
        Update();
        return;
    }

    xx = 2;
    yy += fhei;
    str = "System Datasize Limits";
    SetColor(c1);
    Text(str, xx, yy, 0);
    xx = 2;
    yy += fhei;
    if (rl.rlim_cur != RLIM_INFINITY && rl2.rlim_cur != RLIM_INFINITY &&
            (v = chk_val(0.001 * (rl.rlim_cur + rl2.rlim_cur), &b)) > 0) {
        snprintf(buf, sizeof(buf), "Soft (%cB):", b);
        Text(buf, xx, yy, 0);
        xx += 24*spw;
        snprintf(buf, sizeof(buf), "%.1f", v);
        SetColor(c2);
        Text(buf, xx, yy, 0);
    }
    else {
        str = "Soft:";
        Text(str, xx, yy, 0);
        xx += 24*spw;
        str = "none set";
        SetColor(c2);
        Text(str, xx, yy, 0);
    }

    SetColor(c1);
    xx = 2;
    yy += fhei;

    if (rl.rlim_max != RLIM_INFINITY && rl2.rlim_max != RLIM_INFINITY &&
            (v = chk_val(0.001 * (rl.rlim_max + rl2.rlim_max), &b)) > 0) {
        snprintf(buf, sizeof(buf), "Hard (%cB):", b);
        Text(buf, xx, yy, 0);
        xx += 24*spw;
        snprintf(buf, sizeof(buf), "%.1f", v);
        SetColor(c2);
        Text(buf, xx, yy, 0);
    }
    else {
        str = "Hard:";
        Text(str, xx, yy, 0);
        xx += 24*spw;
        str = "none set";
        SetColor(c2);
        Text(str, xx, yy, 0);
    }
#else
#ifdef Q_OS_WIN
    MEMORYSTATUS mem;
    memset(&mem, 0, sizeof(MEMORYSTATUS));
    mem.dwLength = sizeof(MEMORYSTATUS);
    GlobalMemoryStatus(&mem);
    unsigned avail = mem.dwAvailPhys + mem.dwAvailPageFile;
    unsigned total = mem.dwTotalPhys + mem.dwTotalPageFile;

    xx = 2;
    yy += fhei;
    v = chk_val(0.001 * avail, &b);
    snprintf(buf, sizeof(buf), "Available Memory (%cB):", b);
    SetColor(c1);
    Text(buf, xx, yy, 0);
    xx += 24 * spw;
    snprintf(buf, sizeof(buf), "%.1f", v);
    SetColor(c2);
    Text(buf, xx, yy, 0);

    xx = 2;
    yy += fhei;
    v = chk_val(0.001 * total, &b);
    snprintf(buf, sizeof(buf), "Total Memory (%cB):", b);
    SetColor(c1);
    Text(buf, xx, yy, 0);
    xx += 24 * spw;
    snprintf(buf, sizeof(buf), "%.1f", v);
    SetColor(c2);
    Text(buf, xx, yy, 0);
#endif
#endif
    Update();
}


void
QTmemMonDlg::dismiss_btn_slot()
{
    XM()->PopUpMemory(MODE_OFF);
}


void
QTmemMonDlg::resize_slot(QResizeEvent*)
{
    update();
}


void
QTmemMonDlg::font_changed_slot(int fnum)
{
    if (fnum == FNT_FIXED) {
        QFont *fnt;
        if (Fnt()->getFont(&fnt, FNT_FIXED))
            gd_viewport->setFont(*fnt);
        XM()->PopUpMemory(MODE_UPD);
    }
}


// Static function.
double
QTmemMonDlg::chk_val(double val, char *m)
{
    *m = 'K';
    if (val >= 1e9) {
        val *= 1e-9;
        *m = 'T';
    }
    else if (val >= 1e6) {
        val *= 1e-6;
        *m = 'G';
    }
    else if (val >= 1e3) {
        val *= 1e-3;
        *m = 'M';
    }
    return (val);
}


int
QTmemMonDlg::mem_proc(void*)
{
    XM()->PopUpMemory(MODE_UPD);
    return (instPtr != 0);
}


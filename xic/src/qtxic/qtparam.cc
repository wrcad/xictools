
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

#include "qtmain.h"
#include "qtparam.h"
#include "qtinlines.h"
#include "dsp_color.h"
#include "dsp_inlines.h"
#include "editif.h"
#include "select.h"
#include "events.h"

#include <QHBoxLayout>


void
cMain::ShowParameters(const char*)
{
    if (mainBag())
        mainBag()->show_parameters();
}


param_w::param_w(mainwin *prnt) : QWidget(prnt)
{
    viewport = draw_if::new_draw_interface(DrawNative, false, this);
    QHBoxLayout *hbox = new QHBoxLayout(this);
    hbox->setMargin(0);
    hbox->setSpacing(0);
    hbox->addWidget(viewport->widget());

    QFont *fnt;
    if (FC.getFont(&fnt, FNT_SCREEN))
        viewport->set_font(fnt);
}


void
param_w::print()
{
    unsigned long c1 = DSP()->Color(PromptTextColor);
    unsigned long c2 = DSP()->Color(PromptEditTextColor);
    SetWindowBackground(DSP()->Color(PromptBackgroundColor));
    int fwid, fhei;
    TextExtent(0, &fwid, &fhei);
    SetFillpattern(0);
    SetLinestyle(0);
    Clear();

    QSize qs = viewport->widget()->size();
    int spw = any_string_width(viewport->widget(), " ");

    int xx = 2;
    int yy = fhei - 4;
    int selectno;
    Selections.countQueue(&selectno, 0);
    char textbuf[256];

    const char *str;
    if (DSP()->MainWdesc()->DbType() == WDchd) {
        SetColor(c1);
        str = "Hier:";
        Text(str, xx, yy, 0);
        xx += any_string_width(viewport->widget(), str) + spw;
        SetColor(c2);
        str = DSP()->MainWdesc()->DbName();
        Text(str, xx, yy, 0);
        xx += any_string_width(viewport->widget(), str) + 2*spw;
    }
    else {
        SetColor(c1);
        str = "Cell:";
        Text(str, xx, yy, 0);
        xx += any_string_width(viewport->widget(), str) + spw;
        SetColor(c2);
        str = (DSP()->CurSymname() ? NameString(DSP()->CurSymname()) : "none");
        Text(str, xx, yy, 0);
        xx += any_string_width(viewport->widget(), str) + 2*spw;

        CDs *cursd = CurCell();
        if (cursd) {
            if (cursd->isImmutable()) {
                SetColor(DSP()->Color(PromptHighlightColor));
                str = "RO";
                Text(str, xx, yy, 0);
                xx += any_string_width(viewport->widget(), str) + 2*spw;
            }
            else if (cursd->isModified()) {
                SetColor(DSP()->Color(PromptHighlightColor));
                str = "Mod";
                Text(str, xx, yy, 0);
                xx += any_string_width(viewport->widget(), str) + 2*spw;
            }
        }
    }

    SetColor(c1);
    str = "Grid:";
    Text(str, xx, yy, 0);
    xx += any_string_width(viewport->widget(), str) + spw;
    SetColor(c2);
    Attributes *a = DSP()->MainWdesc()->Attrib();
    sprintf(textbuf, "%g",
        a ? MICRONS(a->grid(DSP()->CurMode())->resol()) : 1.0);
    str = textbuf;
    Text(str, xx, yy, 0);
    xx += any_string_width(viewport->widget(), str) + 2*spw;

    int snap = (a ? a->grid(DSP()->CurMode())->snap() : 1);
    if (snap != 1) {
        SetColor(c1);
        str = "Snap:";
        Text(str, xx, yy, 0);
        xx += any_string_width(viewport->widget(), str) + spw;
        SetColor(c2);
        sprintf(textbuf, "%d", snap);
        str = textbuf;
        Text(str, xx, yy, 0);
        xx += any_string_width(viewport->widget(), str) + 2*spw;
    }
    if (selectno) {
        SetColor(c1);
        str = "Select:";
        Text(str, xx, yy, 0);
        xx += any_string_width(viewport->widget(), str) + spw;
        SetColor(c2);
        sprintf(textbuf, "%d", selectno);
        str = textbuf;
        Text(str, xx, yy, 0);
        xx += any_string_width(viewport->widget(), str) + spw;
        CDc *cd = Selections.firstCall();
        if (cd) {
            sprintf(textbuf, "(%s)", NameString(cd->cellname()));
            str = textbuf;
            Text(str, xx, yy, 0);
            xx += any_string_width(viewport->widget(), str) + spw;
        }
        xx += spw;
    }
    int clev = EditIf()->cxLevel();
    if (clev) {
        SetColor(c1);
        str = "Push:";
        Text(str, xx, yy, 0);
        xx += any_string_width(viewport->widget(), str) + spw;
        SetColor(c2);
        sprintf(textbuf, "%d", clev);
        str = textbuf;
        Text(str, xx, yy, 0);
        xx += any_string_width(viewport->widget(), str) + 2*spw;
    }

    textbuf[0] = 0;
    if (GEO()->curTx()->angle != 0)
        sprintf(textbuf + strlen(textbuf), "R%d", GEO()->curTx()->angle);
    if (GEO()->curTx()->reflectY)
        strcat(textbuf, "MY");
    if (GEO()->curTx()->reflectX)
        strcat(textbuf, "MX");
    if (GEO()->curTx()->magn != 1.0) {
        sprintf(textbuf + strlen(textbuf), "M%.8f", GEO()->curTx()->magn);
        char *t = textbuf + strlen(textbuf) - 1;
        int i = 0;
        while (*t == '0' && i < 7) {
            *t-- = 0;
            i++;
        }
    }
    if (textbuf[0]) {
        SetColor(c1);
        str = "Transf:";
        Text(str, xx, yy, 0);
        xx += any_string_width(viewport->widget(), str) + spw;
        SetColor(c2);
        str = textbuf;
        Text(str, xx, yy, 0);
        xx += any_string_width(viewport->widget(), str) + 2*spw;
    }

    str = EV()->CurCmd() ? EV()->CurCmd()->StateName : 0;
    if (str) {
        SetColor(c1);
        str = "Mode:";
        Text(str, xx, yy, 0);
        xx += any_string_width(viewport->widget(), str) + spw;
        SetColor(c2);
        str = EV()->CurCmd()->StateName;
        Text(str, xx, yy, 0);
        xx += any_string_width(viewport->widget(), str) + 2*spw;
    }
    if (DSP()->CurMode() == Physical) {
        double dx = DSP()->MainWdesc()->ViewportWidth() /
            DSP()->MainWdesc()->Ratio()/CDphysResolution;
        double dy = DSP()->MainWdesc()->ViewportHeight() /
            DSP()->MainWdesc()->Ratio()/CDphysResolution;
        SetColor(c1);
        str = "Window:";
        Text(str, xx, yy, 0);
        xx += any_string_width(viewport->widget(), str) + spw;
        SetColor(c2);
        char buf[64];
        sprintf(buf, "%.1f X %.1f", dx, dy);
        Text(buf, xx, yy, 0);
        xx += any_string_width(viewport->widget(), buf) + 2*spw;
    }
    Update();
}



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
 * GtkInterf Graphical Interface Library                                  *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#include "gtkinterf.h"
#ifndef WIN32
// Dummy symbol to avoid linker warning.
bool NO_MSWPDEV = true;

#else
// Windows only code.

#include "miscutil/lstring.h"
#include "ginterf/xdraw.h"
#include "mswdraw.h"
#include "mswpdev.h"


//
// Hard-copy driver for Windows native service.
//

namespace mswinterf {
    HCdesc MSPdesc(
        "MSP",                  // drname
        "Windows Native",       // descr
        "windows_native",       // keyword
        0,                      // alias
        "-f %s -r %d -w %f -h %f -x %f -y %f", // format
        HClimits(
            0.0, 15.0,  // minxoff, maxxoff
            0.0, 15.0,  // minyoff, maxyoff
            1.0, 16.5,  // minwidth, maxwidth
            1.0, 16.5,  // minheight, maxheight
            0,          // flags
            0           // resols
        ),
        HCdefaults(
            0.25, 0.25, // defxoff, defyoff
            7.8, 10.3,  // defwidth, defheight
            0,          // command string to print file
            0,          // defresol (index into resols)
            HClegOn,    // initially use legend
            HCbest      // initially set best orientation
        ),
        false);     // line_draw
}

using namespace mswinterf;


bool
MSPdev::Init(int *ac, char **av)
{
    if (!ac || !av)
        return (true);

    // Pull printer name out of arg list.  This is how printer name
    // and media index are passed to this driver
    if (*ac >= 3 && !strcmp(av[*ac-3], "-nat")) {
        delete [] printer;
        printer = lstring::copy(av[*ac-2]);
        media = (int)av[*ac-1];
        *ac -= 3;
    }

    HCdata *hd = new HCdata;
    hd->filename = 0;
    hd->resol = 0;
    hd->width = 8.0;
    hd->height = 10.5;
    hd->xoff = .25;
    hd->yoff = .25;
    hd->landscape = false;
    if (HCdevParse(hd, ac, av))
        return (true);

    if (!hd->filename || !*hd->filename)
        return (true);
    HCcheckEntries(hd, &MSPdesc);

    numlinestyles = 16;
    numcolors = 32;

    delete data;
    data = hd;
    return (false);
}


#define MMPI 25.4

namespace {
    struct sMedia
    {
        const char *name;
        int width, height;
        int msw_code;
    };


    // list of standard paper sizes, units are points (1/72 inch)
    //
    sMedia pagesizes[] =
    {
        { "Letter",       612,  792,    DMPAPER_LETTER },
        { "Legal",        612,  1008,   DMPAPER_LEGAL },
        { "Tabloid",      792,  1224,   DMPAPER_TABLOID },
        { "Ledger",       1224, 792,    DMPAPER_LEDGER },
        { "10x14",        720,  1008,   DMPAPER_10X14 },
        { "11x17 \"B\"",  792,  1224,   DMPAPER_11X17 },
        { "12x18",        864,  1296,   -1 },
        { "17x22 \"C\"",  1224, 1584,   DMPAPER_CSHEET },
        { "18x24",        1296, 1728,   -1 },
        { "22x34 \"D\"",  1584, 2448,   DMPAPER_DSHEET },
        { "24x36",        1728, 2592,   -1 },
        { "30x42",        2160, 3024,   -1 },
        { "34x44 \"E\"",  2448, 3168,   DMPAPER_ESHEET },
        { "36x48",        2592, 3456,   -1 },
        { "Statement",    396,  612,    DMPAPER_STATEMENT },
        { "Executive",    540,  720,    DMPAPER_EXECUTIVE },
        { "Folio",        612,  936,    DMPAPER_FOLIO },
        { "Quarto",       610,  780,    DMPAPER_QUARTO },
        { "A0",           2384, 3370,   -1 },
        { "A1",           1684, 2384,   -1 },
        { "A2",           1190, 1684,   DMPAPER_A2 },
        { "A3",           842,  1190,   DMPAPER_A3 },
        { "A4",           595,  842,    DMPAPER_A4 },
        { "A5",           420,  595,    DMPAPER_A5 },
        { "A6",           298,  420,    -1 },
        { "B0",           2835, 4008,   -1 },
        { "B1",           2004, 2835,   -1 },
        { "B2",           1417, 2004,   -1 },
        { "B3",           1001, 1417,   -1 },
        { "B4",           729,  1032,   DMPAPER_B4 },
        { "B5",           516,  729,    DMPAPER_B5 },
        { 0,              0,    0,      -1 }
    };

    // Export media parameters, used by MSP (native Windows) driver.
    //
    void HCmedia(int media, char **namep, int *widp, int *heip, int *codep)
    {
        if (media < 0 || media >= (int)(sizeof(pagesizes)/sizeof(sMedia)))
            media = 0;
        sMedia *m = &pagesizes[media];
        if (namep)
            *namep = (char*)m->name;
        if (widp)
            *widp = (int)(m->width*MMPI/72.0);   // return millimeters
        if (heip)
            *heip = (int)(m->height*MMPI/72.0);;
        if (codep)
            *codep = m->msw_code;
    }
}


// New viewport function.
//
GRdraw *
MSPdev::NewDraw(int)
{
    if (!data)
        return (0);

    MSPparams *px = new MSPparams;
    px->dev = this;
    px->lcx = GRappIf()->SetupLayers(0, px, 0);

    HANDLE hpr;
    if (!OpenPrinter(printer, &hpr, 0)) {
        delete px;
        return (0);
    }
    int sz = DocumentProperties(0, hpr, printer, 0, 0, 0);
    DEVMODE *dv = (DEVMODE*)malloc(sz);
    if (!dv) {
        delete px;
        ClosePrinter(hpr);
        return (0);
    }
    memset(dv, 0, sz);
    if (DocumentProperties(0, hpr, printer, dv, 0, DM_OUT_BUFFER)
            != IDOK) {
        delete px;
        free(dv);
        ClosePrinter(hpr);
        return (0);
    }
    if (data->resol) {
        dv->dmYResolution = data->resol;
        dv->dmPrintQuality = data->resol;
    }
    if (data->landscape)
        dv->dmOrientation = DMORIENT_LANDSCAPE;
    else
        dv->dmOrientation = DMORIENT_PORTRAIT;

    int pagew, pageh, pcode;
    HCmedia(media, 0, &pagew, &pageh, &pcode);
    if (pcode == -1) {
        dv->dmPaperWidth = 10*pagew;
        dv->dmPaperLength = 10*pageh;
    }
    else
        dv->dmPaperSize = pcode;
    if (DocumentProperties(0, hpr, printer, dv, dv,
            DM_IN_BUFFER | DM_OUT_BUFFER) != IDOK) {
        delete px;
        free(dv);
        ClosePrinter(hpr);
        return (0);
    }
    ClosePrinter(hpr);

    HDC dcp = CreateDC(0, printer, 0, dv);
    free(dv);
    if (!dcp) {
        delete px;
        return (0);
    }
    data->resol = GetDeviceCaps(dcp, LOGPIXELSX);

    SetBkColor(dcp, RGB(255, 255, 255));
    SetTextColor(dcp, RGB(0, 0, 0));
    SetBkMode(dcp, TRANSPARENT);
    SetTextAlign(dcp, TA_LEFT | TA_BOTTOM | TA_NOUPDATECP);

    px->SetMemDC(dcp);
    int nc = GetDeviceCaps(dcp, NUMCOLORS);
    if (nc == 2) {
        numcolors = 2;
        px->SetColor(0);
        px->set_monochrome(true);
    }

    // In auto-height, the application will rotate the cell.  The legend
    // is always on the bottom.  Nothing to do here
    if (data->height == 0)
        data->landscape = false;

    // Typical Microsoft; the positioning of the origin on the page is not
    // quite where it should be.  Further, the image is clipped outside
    // Microsoft's boundaries, so we can't get as close to the page edge
    // as with the internal drivers.  The devx/y subtractions reasonably
    // center the page for one sample printer.

    int devx = GetDeviceCaps(dcp, PHYSICALOFFSETX);
    int devy = GetDeviceCaps(dcp, PHYSICALOFFSETY);

    // drawable area
    if (data->landscape) {

        height = (int)(data->resol*data->width*0.99);
        width = (int)(data->resol*data->height*0.99);
        xoff = (int)(data->resol*data->yoff) - devx;
        yoff = (int)(data->resol*data->xoff) - devy;
    }
    else {
        width = (int)(data->resol*data->width*0.99);
        height = (int)(data->resol*data->height*0.99);
        xoff = (int)(data->resol*data->xoff) - devx;
        yoff = (int)(data->resol*data->yoff);
    }
    if (xoff < 0)
        xoff = 0;
    if (yoff < 0)
        yoff = 0;

    char buf[64];
    static int jnum = 1;
    sprintf(buf, "WR-MSP-%d", jnum++);
    DOCINFO di;
    memset(&di, 0, sizeof(DOCINFO));
    di.cbSize = sizeof(DOCINFO);
    di.lpszDocName = buf;
    di.lpszOutput = data->filename;
    if (StartDoc(dcp, &di) <= 0) {
        delete px;
        DeleteDC(dcp);
        return (0);
    }
    if (StartPage(dcp) <= 0) {
        delete px;
        DeleteDC(dcp);
        return (0);
    }
    return (px);
}


namespace {
    BOOL CALLBACK AbortFunc(HDC, int)
    {
        if (GRX->CheckForEvents()) {
            GRpkgIf()->HCabort("User aborted");
            return (FALSE);
        }
        return (TRUE);
    }
}


// Halt driver, dump image.
//
void
MSPparams::Halt()
{
    HDC dcp = SetMemDC(0);
    if (dcp) {
        SetAbortProc(dcp, AbortFunc);
        int ok = EndPage(dcp);
        if (ok <= 0 || GRpkgIf()->HCaborted())
            AbortDoc(dcp);
        else
            ok = EndDoc(dcp);

        HPEN pen = (HPEN)GetCurrentObject(dcp, OBJ_PEN);
        HBRUSH brush = (HBRUSH)GetCurrentObject(dcp, OBJ_BRUSH);
        DeleteDC(dcp);
        DeletePen(pen);
        DeleteBrush(brush);
        if (ok <= 0)
            GRpkgIf()->HCabort("Print driver returned failure status.");
    }
    else
        GRpkgIf()->HCabort("User aborted");
    GRappIf()->SetupLayers(0, this, lcx);
    delete this;
}


int
MSPparams::SwathHeight(int *special)
{
    if (special)
        *special = false;
    return (dev->height);
}


void
MSPparams::ResetViewport(int wid, int hei)
{
    if (wid == dev->width && hei == dev->height)
        return;
    if (wid <= 0 || hei <= 0)
        return;
    dev->width = wid;
    dev->data->width = ((double)dev->width)/dev->data->resol;
    dev->height = hei;
    dev->data->height = ((double)dev->height)/dev->data->resol;
}

#endif


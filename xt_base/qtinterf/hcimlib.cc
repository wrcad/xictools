
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
 * Ginterf Graphical Interface Library                                    *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#include "config.h"
#include "qtdraw.h"
#include "qtcanvas.h"
#include "ginterf/grfont.h"
#include "ginterf/hcimlib.h"

#include <QResizeEvent>
#include <stdio.h>


//
// Hard-copy driver which creates multiple format image files based
// on the file suffix.  This operates like the old Imlib print driver
// but is self-contained within Qt.  This version of the driver is used
// whenever Qt is linked.
//

//XXX ToDo:  At least in Windows, implement a "clipboard" target like
// the existing Windows Imlib driver.

namespace {
    const char *IMresols[] = { "100", "200", 0 };
}

namespace ginterf
{
    // Hard-copy driver - uses imsave to dump to a supported file
    // type.  The file type is determined from the suffix of the given
    // file name.
    //
    HCdesc IMdesc(
        "IM",                // drname
        "Image: jpeg, tiff, png, etc", // descr
        "image",            // keyword
        "imlib",            // alias
        "-f %s -r %d -w %f -h %f", // format
        HClimits(
            0.0, 15.0,  // minxoff, maxxoff
            0.0, 15.0,  // minyoff, maxyoff
            1.0, 16.5,  // minwidth, maxwidth
            1.0, 16.5,  // minheight, maxheight
                        // flags
            HCdontCareXoff | HCdontCareYoff | HCfileOnly | HCnoBackground,
            IMresols    // resolutions
        ),
        HCdefaults(
            0.0, 0.0,   // defxoff, defyoff
            4.0, 4.0,   // defwidth, defheight
            0,          // command string to print file
            0,          // defresol (index into resols)
            HClegOff,   // initially no legend
            HCbest      // initially set best orientation
        ),
        false);     // line_draw
}


bool
IMdev::Init(int *ac, char **av)
{
    if (!ac || !av)
        return (true);
    HCdata *hd = new HCdata;
    hd->filename = 0;
    hd->resol = 100;
    hd->width = 8.0;
    hd->height = 10.5;
    hd->xoff = .25;
    hd->yoff = .25;
    hd->landscape = false;
    hd->nobackg = false;
    if (HCdevParse(hd, ac, av)) {
        delete hd;
        return (true);
    }

    if (!hd->filename || !*hd->filename) {
        delete hd;
        return (true);
    }
    HCcheckEntries(hd, &IMdesc);

    width = (int)(hd->width * hd->resol);
    height = (int)(hd->height * hd->resol);
    numlinestyles = 16;
    numcolors = 32;

    delete im_data;
    im_data = hd;
    return (false);
}


namespace ginterf
{
    struct IMparams : public QTdraw
    {
        IMparams();
        virtual ~IMparams() { }

        void Halt();
        void ResetViewport(int, int);
        void DefineViewport();

        void set_dev(IMdev *d)      { pm_dev = d; }
        void set_lcx(void *p)       { pm_lcx = p; }

private:
        IMdev *pm_dev;
        void *pm_lcx;
    };
}


// New viewport function.
//
GRdraw *
IMdev::NewDraw(int)
{
    if (!im_data)
        return (0);
    IMparams *px = new IMparams();
    px->set_dev(this);
    return (px);
}


IMparams::IMparams() : QTdraw(0)
{
    pm_dev = 0;
    pm_lcx = 0; 

    gd_viewport = new QTcanvas();
    QFont *fnt;
    if (Fnt()->getFont(&fnt, FNT_FIXED)) {
        gd_viewport->setFont(*fnt);
    }
}


// Halt driver, dump image.
//
void
IMparams::Halt()
{
    QPixmap *pmap = Viewport()->pixmap();
    if (pmap) {
        bool transp = pm_dev->data()->nobackg;;
        bool ret_ok = false;
        if (transp) {
            QColor backg(GRappIf()->BackgroundPixel());
            QPixmap tpmap(*pmap);
            QBitmap bmap = pmap->createMaskFromColor(backg);
            tpmap.setMask(bmap);
            ret_ok = tpmap.save(pm_dev->data()->filename);
        }
        else
            ret_ok = pmap->save(pm_dev->data()->filename);
        if (!ret_ok) {
            GRpkg::self()->HCabort("Image creation error");
        }
    }
    delete this;
}


void
IMparams::ResetViewport(int wid, int hei)
{
    if (wid == pm_dev->width && hei == pm_dev->height)
        return;
    if (wid <= 0 || hei <= 0)
        return;
    pm_dev->width = wid;
    pm_dev->data()->width = ((double)pm_dev->width)/pm_dev->data()->resol;
    pm_dev->height = hei;
    pm_dev->data()->height = ((double)pm_dev->height)/pm_dev->data()->resol;
}


void
IMparams::DefineViewport()
{
    QSize oldsz(gd_viewport->width(), gd_viewport->height());
    QSize newsz(pm_dev->width, pm_dev->height);
    if (newsz != oldsz) {
        QResizeEvent rev(newsz, oldsz);
        gd_viewport->event(&rev);
    }
    SetFillpattern(0);
    SetColor(GRappIf()->BackgroundPixel());
    Box(0, 0, pm_dev->width, pm_dev->height);
}
// End of IMparams functions


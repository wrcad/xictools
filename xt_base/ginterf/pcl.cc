
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

// Hardcopy driver for HP Laserjet and compatible
// This file contains rendering routines for an in-core single-plane
// image which is subsequently dumped to the printer.

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <sys/types.h>
#include <time.h>
#include "pcl.h"
#include "miscutil/lstring.h"
#include "miscutil/texttf.h"


// default Courrier 12 point font
#define FONTPTS 12

namespace {
    // Limits and specified parameters
    const char *PCLresols[] = {"75", "100", "150", "300", 0};
}

namespace ginterf
{
    HCdesc PCLdesc(
        "PCL",                  // drname
        "HP laser PCL",         // descr
        "hp_laser_pcl",         // keyword
        "pcl",                  // alias
        "-f %s -r %d -w %f -h %f -x %f -y %f", // format
        HClimits(
            0.0, 15.0,  // minxoff, maxxoff
            0.0, 15.0,  // minyoff, maxyoff
            1.0, 16.5,  // minwidth, maxwidth
            1.0, 16.5,  // minheight, maxheight
            HCtopMargin | HClandsSwpYmarg,  // flags
            PCLresols   // resols
        ),
        HCdefaults(
            0.25, 0.25, // defxoff, defyoff
            7.8, 10.3,  // defwidth, defheight
            0,          // command string to print file
            2,          // defresol (index into resols)
            HClegOn,    // initially use legend
            HCbest      // initially set best orientation
        ),
        false);     // line_draw
}


// Initialization routine.
//
bool
PCLdev::Init(int *ac, char **av)
{
    if (!ac || !av)
        return (true);
    HCdata *hd = new HCdata;
    hd->filename = 0;
    hd->resol = 150;
    hd->width = 8.0;
    hd->height = 10.5;
    hd->xoff = .25;
    hd->yoff = .25;
    hd->landscape = false;
    if (HCdevParse(hd, ac, av)) {
        delete hd;
        return (true);
    }

    if (!hd->filename || !*hd->filename) {
        delete hd;
        return (true);
    }
    HCcheckEntries(hd, &PCLdesc);

    delete data;
    data = hd;
    numlinestyles = 0;
    numcolors = 2;
    return (false);
}


// New viewport function.
//
GRdraw *
PCLdev::NewDraw(int)
{
    if (!data)
        return (0);
    FILE *plotfp;
    if (data->filename && *data->filename)
        plotfp = fopen(data->filename, "wb");
    else
        return (0);
    if (!plotfp) {
        GRpkgIf()->Perror(data->filename);
        return (0);
    }
    PCLparams *pcl = new PCLparams;
    pcl->fp = plotfp;
    pcl->dev = this;

    pcl->linestyle = 0;
    pcl->curfillpatt = 0;
    pcl->textlist = 0;

    // In auto-height, the application will rotate the cell.  The legend
    // is always on the bottom.  Nothing to do here
    if (data->height == 0)
        data->landscape = false;

    // drawable area
    if (data->landscape) {
        height = (int)(data->resol*data->width);
        width = (int)(data->resol*data->height);
        xoff = (int)(data->resol*data->yoff);
        yoff = (int)(data->resol*data->xoff);
    }
    else {
        width = (int)(data->resol*data->width);
        height = (int)(data->resol*data->height);
        xoff = (int)(data->resol*data->xoff);
        yoff = (int)(data->resol*data->yoff);
    }

    // bytes per line
    pcl->bytpline = (width - 1)/8 + 1;

    // Use malloc/free to detect out of memory without a fatal error
    int size = height*pcl->bytpline;
    if (!size)
        size = pcl->bytpline;
    pcl->base = (unsigned char*)malloc(size);
    if (!pcl->base) {
        GRpkgIf()->ErrPrintf(ET_WARN, "Insufficient memory.");
        return (0);
    }
    memset(pcl->base, 0, size);
    return (pcl);
}


// Dump bitmap in HP Laserjet compatible format.
//
void
PCLparams::dump()
{
    // reset printer
    // if landscape, set landscape mode
    // top margin 0
    // resolution dev->data->resol dpi
    // X cursor position xoff
    // Y cursor position yoff
    // start raster graphics at current position
    // .25 * 300dpi = 75
    //
    char *buf;
    int xoff, yoff;
    if (!base)
        return;
    if (devdata()->landscape) {
        xoff = (int)(devdata()->yoff*300) - 75;
        yoff = (int)(devdata()->xoff*300) - 25;
        int sz = (int)(devdata()->height*300);
        fprintf(fp,
            "\033E\033&l1O\033&l0E\033*t%dR\033*p%dX\033*p%dY\033*r1A",
            devdata()->resol, xoff + sz, yoff);
        buf = new char [(dev->height - 1)/8 + 9];
        sprintf(buf, "\033*b%dW", (dev->height - 1)/8 + 1);
    }
    else {
        xoff = (int)(devdata()->xoff*300) - 75;
        yoff = (int)(devdata()->yoff*300);
        fprintf(fp,
            "\033E\033&l0E\033*t%dR\033*p%dX\033*p%dY\033*r1A",
            devdata()->resol, xoff, yoff);
        buf = new char [bytpline + 8];
        sprintf(buf, "\033*b%dW", bytpline);
    }

    int len = strlen(buf);
    char *c = buf + len;

    unsigned char *bs = base;
    if (devdata()->landscape) {
        // Arggh, this is ghastly
        for (int i = dev->width - 1; i >= 0; i--) {
            int xbyte = i/8;
            int xbit = 7 - i%8;
            unsigned byte = 0;
            int ybit = 7;
            int ybyte = 0;
            for (int j = 0; j < dev->height; j++) {
                unsigned a = *(bs + xbyte + j*bytpline);
                if (a & (1 << xbit))
                    byte |= (1 << ybit);
                ybit--;
                if (ybit < 0) {
                    ybit = 7;
                    c[ybyte] = byte;
                    ybyte++;
                    byte = 0;
                }
            }
            if (ybit != 7) {
                c[ybyte] = byte;
                ybyte++;
            }
            if ((int)fwrite(buf, 1, ybyte+len, fp) != ybyte + len) {
                GRpkgIf()->HCabort("File write error");
                break;
            }
        }
    }
    else {
        for (int i = 0; i < dev->height; i++) {
            memcpy(c, bs, bytpline);
            bs += bytpline;

            if ((int)fwrite(buf, 1, bytpline+len, fp)
                    != bytpline + len) {
                GRpkgIf()->HCabort("File write error");
                break;
            }
        }
    }
    // end raster graphics
    fprintf(fp, "\033*rB");

    // now for the text
    for (PCLtext *t = textlist; t; t = t->next) {
        int x = (t->x + dev->xoff)*(300/devdata()->resol) - 75;
        if (t->xform & TXTF_HJR)
            // assume 6x10 aspect ratio for char
            x -= (strlen(t->text)*FONTPTS*300*6)/(10*72);
        else if (t->xform & TXTF_HJC)
            x -= (strlen(t->text)*FONTPTS*300*6)/(2*10*72);

        int y = (t->y + dev->yoff)*(300/devdata()->resol);
        if (devdata()->landscape)
            y -= 25;
        if (t->xform & TXTF_VJT)
            y += (FONTPTS*300)/72;
        else if (t->xform & TXTF_VJC)
            y += (FONTPTS*300)/(2*72);
        // x position, y position, text
        fprintf(fp, "\033*p%dX\033*p%dY%s", x, y, t->text);
    }

    // form feed
    // reset printer
    fprintf(fp, "\033*rB\014\033E");

    delete [] buf;
}


// Halt driver, dump bitmap.
//
void
PCLparams::Halt()
{
    dump();
    fclose(fp);
    PCLtext *t, *tn;
    for (t = textlist; t; t = tn) {
        tn = t->next;
        delete [] t->text;
        delete t;
    }
    free(base);
    delete this;
}


void
PCLparams::ResetViewport(int wid, int hei)
{
    if (wid == dev->width && hei == dev->height)
        return;
    if (wid <= 0 || hei <= 0)
        return;
    dev->width = wid;
    devdata()->width = ((double)dev->width)/devdata()->resol;
    dev->height = hei;
    devdata()->height = ((double)dev->height)/devdata()->resol;
    bytpline = (dev->width - 1)/8 + 1;
    if (base)
        free(base);
    base = (unsigned char*)malloc(dev->height*bytpline);
    if (!base) {
        GRpkgIf()->HCabort("Insufficient memory");
        return;
    }
    memset(base, 0, dev->height*bytpline);
}


// Add a text string to the text list.  The text list is processed
// after the bitmap is dumped.
//
void
PCLparams::Text(const char *text, int x, int y, int xform, int width,
    int height)
{
    if (!width || !height)
        return;
    PCLtext *pclt = new PCLtext;
    pclt->x = x;
    pclt->y = y;
    pclt->xform = xform;
    pclt->text = lstring::copy(text);
    pclt->next = textlist;
    textlist = pclt;
}


// Get the text extent of string.  Note that this is inaccurate
// for proportional fonts.
//
void
PCLparams::TextExtent(const char *string, int *x, int *y)
{
    *y = FONTPTS*devdata()->resol/72;
    *x = (6*(*y))/10;
    if (string)
        *x *= strlen(string);
}


// Return the resolution, relative to the on-screen resolution, needed
// for size thresholding.
//
double
PCLparams::Resolution()
{
    return (((PCLdev*)dev)->data->resol/GR_SCREEN_RESOL);
}


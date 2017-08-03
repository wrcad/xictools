
/*========================================================================*
 *                                                                        *
 *  Whiteley Research Inc, Sunnyvale, CA USA, http://wrcad.com            *
 *  Copyright (C) 2017 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *  Licensed under the Apache License, Version 2.0 (the "License");       *
 *  you may not use this file except in compliance with the License.      *
 *  You may obtain a copy of the License at                               *
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

// Hardcopy driver for Postscript bitmap monochrome
// This file contains rendering routines for an in-core single-plane
// image which is subsequently dumped to the printer.

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <sys/types.h>
#include <time.h>
#include "psbc.h"
#include "lstring.h"
#include "texttf.h"


// font height in points=1/72 inch
#define FONTPTS 10

namespace {
    // Limits and specified parameters
    const char *PSBCresols[] =
        {"72", "75", "100", "150", "200", "300", "400", 0};
}

namespace ginterf
{
    HCdesc PSBCdesc(
        "PSBC",                     // drname
        "PostScript bitmap color",  // descr
        "postscript_bitmap_color",  // keyword
        "psbc",                     // alias
        "-f %s -r %d -w %f -h %f -x %f -y %f", // format
        HClimits(
            0.0, 22.0,  // minxoff, maxxoff
            0.0, 22.0,  // minyoff, maxyoff
            1.0, 23.5,  // minwidth, maxwidth
            1.0, 23.5,  // minheight, maxheight
            0,          // flags
            PSBCresols  // resols
        ),
        HCdefaults(
            0.25, 0.25, // defxoff, defyoff
            7.9, 10.3,  // defwidth, defheight
            0,          // command string to print file
            0,          // defresol (index into resols)
            HClegOn,    // initially use legend
            HCbest      // initially set best orientation
        ),
        false);     // line_draw

    HCdesc PSBCdesc_e(
        "PSBC",                             // drname
        "PostScript bitmap color, encoded", // descr
        "postscript_bitmap_color_encoded",  // keyword
        "psbce",                            // alias
        "-f %s -r %d -e -w %f -h %f -x %f -y %f", // format
        HClimits(
            0.0, 22.0,  // minxoff, maxxoff
            0.0, 22.0,  // minyoff, maxyoff
            1.0, 23.5,  // minwidth, maxwidth
            1.0, 23.5,  // minheight, maxheight
            0,          // flags
            PSBCresols  // resols
        ),
        HCdefaults(
            0.25, 0.25, // defxoff, defyoff
            7.9, 10.3,  // defwidth, defheight
            0,          // command string to print file
            0,          // defresol (index into resols)
            HClegOn,    // initially use legend
            HCbest      // initially set best orientation
        ),
        false);     // line_draw
}


// Initialization routine.
//
bool
PSBCdev::Init(int *ac, char **av)
{
    if (!ac || !av)
        return (true);
    HCdata *hd = new HCdata;
    hd->filename = 0;
    hd->resol = 150;
    // When encode is true, postscript uses ASCII85 encoding.  If
    // doRLE is true (hard coded below) run length encoding is done
    // as well.
    hd->encode = false;
    hd->doRLE = true;
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
    HCcheckEntries(hd, hd->encode ? &PSBCdesc : &PSBCdesc_e);

    delete data;
    data = hd;
    numlinestyles = 0;
    numcolors = 32;
    return (false);
}


// New viewport function.
//
GRdraw *
PSBCdev::NewDraw(int)
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
    PSBCparams *psbc = new PSBCparams;
    psbc->fp = plotfp;
    psbc->dev = this;
    psbc->linestyle = 0;
    psbc->curfillpatt = 0;
    psbc->textlist = 0;

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
    psbc->bytpline = 3*width;

    // Use malloc/free to detect out of memory without a fatal error
    int size = height*psbc->bytpline;
    if (!size)
        size = psbc->bytpline;
    psbc->base = (unsigned char*)malloc(size);
    if (!psbc->base) {
        GRpkgIf()->ErrPrintf(ET_WARN, "Insufficient memory.");
        return (0);
    }
    memset(psbc->base, 0, size);
    return (psbc);
}


namespace {
    char hexc[16] =
        { '0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'};
}

// Dump bitmap in postscript format.  Sets GRpkgIf()->HCabort if a write fails.
//
void
PSBCparams::dump()
{
    if (!base)
        return;
    time_t secs;
    time(&secs);
    tm *tm = localtime(&secs);
    fprintf(fp, "%%!PS-Adobe-3.0 EPSF-3.0\n");

#ifndef PS_DEBUG
    fprintf(fp, "%%%%BoundingBox: %d %d %d %d\n",
        (int)(72*devdata()->xoff)-1,
        (int)(72*devdata()->yoff)-1,
        (int)(72*(devdata()->xoff + devdata()->width))+1,
        (int)(72*(devdata()->yoff + devdata()->height))+1);
#endif

    fprintf(fp, "%%!Xic-alt %02d-%02d-%02d %02d:%02d\n", tm->tm_mon+1,
        tm->tm_mday, tm->tm_year - 100, tm->tm_hour, tm->tm_min);
    fprintf(fp, "/Helvetica findfont %g scalefont setfont\n",
        FONTPTS*devdata()->resol/72.0);

#ifdef PS_DEBUG
    int ww = (int)(8.5*72);
    int hh = 11*72;
    fprintf(fp, "%d %d moveto\n", 0, 0);
    fprintf(fp, "%d %d lineto\n", 0, hh);
    fprintf(fp, "%d %d lineto\n", ww, hh);
    fprintf(fp, "%d %d lineto\n", ww, 0);
    fprintf(fp, "%d %d lineto\n", 0, 0);
#endif

    if (!devdata()->encode)
        // set up pixel buffer
        fprintf(fp, "/pixbuf %d string def\n", bytpline);
    // scale to native coordinates
    fprintf(fp, "%g %g scale\n", 72.0/devdata()->resol, 72.0/devdata()->resol);
    if (devdata()->landscape) {
        fprintf(fp, "%d %d translate\n", dev->yoff + dev->height, dev->xoff);
        fprintf(fp, "90 rotate\n");
    }
    else
        fprintf(fp, "%d %d translate\n", dev->xoff, dev->yoff);
    // Note:  The bitmap is color inverted by the settransfer command.
    // Alternatively, the stored bits could be inverted, but this makes
    // the file much larger if encoded and there is a lot of white space.
    fprintf(fp, "{1 exch sub} settransfer\n");
    // push wid, hei, bits/pixel per color
    fprintf(fp, "%d %d 8\n", dev->width, dev->height);
    // set up data stream
    fprintf(fp, "matrix\n");
    if (devdata()->encode) {
        fprintf(fp, "currentfile\n");
        fprintf(fp, "/ASCII85Decode filter\n");
        if (devdata()->doRLE)
            fprintf(fp, "/RunLengthDecode filter\n");
        fprintf(fp, "false 3\ncolorimage\n");
    }
    else
        fprintf(fp,
            "{currentfile pixbuf readhexstring pop}\nfalse 3\ncolorimage\n");
    if (ferror(fp)) {
        GRpkgIf()->HCabort("File write error");
        return;
    }
    // write data
    if (devdata()->encode) {
        if (PS_rll85dump(fp, base, bytpline, dev->height - 1,
                devdata()->doRLE)) {
            GRpkgIf()->HCabort("File write error");
            return;
        }
        fprintf(fp, "\n~>\n");
    }
    else {
        char *ptr = (char*)base + (dev->height - 1)*bytpline;
        int cnt = 0;
        for (int j = dev->height - 1; j >= 0; j--) {
            for (int i = 0; i < bytpline; i++) {
                unsigned char c = *(unsigned char*)ptr;
                putc(hexc[c>>4], fp);
                putc(hexc[c&0xf], fp);
                ptr++;
                cnt++;
                if (cnt > 38) {
                    cnt = 0;
                    putc('\n', fp);
                }
            }
            ptr -= bytpline << 1;
            if (ferror(fp)) {
                GRpkgIf()->HCabort("File write error");
                return;
            }
        }
        fprintf(fp, "\n");
    }

    // now for the text
    // unreverse black/white
    fprintf(fp, "%d %d translate\n", -dev->xoff, 0);
    fprintf(fp, "{} settransfer\n");
    for (PSBCtext *t = textlist; t; t = t->next) {
        int x = t->x + dev->xoff;
        int y = t->y + dev->yoff;

        char tbuf[512];
        char *s = tbuf;
        char *text = t->text;
        while (*text) {
            if (*text == '(' || *text == ')' || *text == '\\')
                *s++ = '\\';
            *s++ = *text++;
        }
        *s = '\0';
        fprintf(fp, "gsave\n");
        if (t->xform & TXTF_HJR)
            fprintf(fp, "(%s) stringwidth pop neg %d add %d translate\n",
                tbuf, x, dev->yoff + dev->height - 1 - y);
        else if (t->xform & TXTF_HJC)
            fprintf(fp, "(%s) stringwidth pop -2 div %d add %d translate\n",
                tbuf, x, dev->yoff + dev->height - 1 - y);
        else
            fprintf(fp, "%d %d translate\n", x,
                dev->yoff + dev->height - 1 - y);
        if (t->xform & TXTF_VJT)
            fprintf(fp, "0 %g translate\n",
                -FONTPTS*(double)devdata()->resol/72);
        else if (t->xform & TXTF_VJC)
            fprintf(fp, "0 %g translate\n",
                -FONTPTS*(double)devdata()->resol/(2*72));
        if (t->xform & (TXTF_MX | TXTF_MY))
            fprintf(fp, "%d %d scale\n",
                (t->xform & TXTF_MX) ? -1 : 1, (t->xform & TXTF_MY) ? -1 : 1);
        int deg = (t->xform & TXTF_ROT) * 90;
        if (t->xform & TXTF_45)
            deg += 45;
        if (deg)
            fprintf(fp, "%d rotate\n", deg);
        if (t->w > 0 && t->h > 0) {
            double sy = t->h/(FONTPTS*(double)devdata()->resol/72);
            fprintf(fp, "%d (%s) stringwidth pop div %g scale\n", t->w,
                tbuf, sy);
        }
        fprintf(fp, "0 0 moveto\n");
        red = 255 - t->r;
        green = 255 - t->g;
        blue = 255 - t->b;
        fprintf(fp, "%g %g %g setrgbcolor\n",
            (double)red/255, (double)green/255,
            (double)blue/255);
        fprintf(fp, "(%s) show\n", tbuf);
        fprintf(fp, "grestore\n");
        if (ferror(fp)) {
            GRpkgIf()->HCabort("File write error");
            return;
        }
    }
    fprintf(fp, "showpage\n");
}


// Halt driver, dump bitmap.
//
void
PSBCparams::Halt()
{
    dump();
    fclose(fp);
    PSBCtext *t, *tn;
    for (t = textlist; t; t = tn) {
        tn = t->next;
        delete [] t->text;
        delete t;
    }
    if (base)
        free(base);
    delete this;
}


void
PSBCparams::ResetViewport(int wid, int hei)
{
    if (wid == dev->width && hei == dev->height)
        return;
    if (wid <= 0 || hei <= 0)
        return;
    dev->width = wid;
    devdata()->width = ((double)dev->width)/devdata()->resol;
    dev->height = hei;
    devdata()->height = ((double)dev->height)/devdata()->resol;
    bytpline = 3*wid;
    if (base)
        free(base);
    int size = hei*bytpline;
    if (size == 0)
        size = bytpline;
    base = (unsigned char*)malloc(size);
    if (!base) {
        GRpkgIf()->HCabort("Insufficient memory");
        return;
    }
    memset(base, 0, size);
}


// Add a text string to the text list.  The text list is processed
// after the bitmap is dumped.
//
void
PSBCparams::Text(const char *text, int x, int y, int xform, int width,
    int height)
{
    if (!width || !height)
        return;
    PSBCtext *psbct = new PSBCtext;
    psbct->x = x;
    psbct->y = y;
    psbct->w = width;
    psbct->h = height;
    psbct->r = red;
    psbct->g = green;
    psbct->b = blue;
    psbct->xform = xform;
    psbct->text = lstring::copy(text);
    psbct->next = textlist;
    textlist = psbct;
}


// Get the text extent of string.  Note that this is inaccurate
// for proportional fonts.
//
void
PSBCparams::TextExtent(const char *string, int *x, int *y)
{
    // This will be inacurate
    *y = (FONTPTS*devdata()->resol)/72;
    *x = (6*(*y))/10;  // approximate
    if (string)
        *x *= strlen(string);
}


// Return the resolution, relative to the on-screen resolution, needed
// for size thresholding.
//
double
PSBCparams::Resolution()
{
    return (((PSBCdev*)dev)->data->resol/GR_SCREEN_RESOL);
}


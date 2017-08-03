
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

#ifndef PSLINDRW_H
#define PSLINDRW_H

#include "graphics.h"
#include <stdio.h>

// Driver for PostScript (TM) line drawing
//
// This ia a driver for producing Postscript line drawings.
//
// Call with "-f filename -w width -h height -x left_marg -y top_marg"
//
// dimensions are in inches (float format)

#define PS_MAX_FPs 64

namespace ginterf
{
    extern HCdesc PSdesc_m;
    extern HCdesc PSdesc_c;

    class PSdev : public GRdev
    {
    public:
        PSdev()
            {
                name = "PS";
                ident = _devPS_;
                devtype = GRhardcopy;
                data = 0;
            }

        friend struct PSparams;

        void RGBofPixel(int, int *r, int *g, int *b)    { *r = *g = *b = 0; }

        bool Init(int*, char**);
        GRdraw *NewDraw(int);

    private:
        HCdata *data;       // internal private data struct
    };

    struct PSparams : public HCdraw
    {
        PSparams()
            {
                dev = 0;
                fp = 0;
                strokecnt = 0;
                lastx = lasty = 0;
                red = green = blue = 0;
                curfpat = 0;
                linestyle = 0;
                for (unsigned i = 0; i < PS_MAX_FPs; i++)
                    fpats[i] = 0;
                usecolor = false;
                backg = false;
                dev_flags = GRhasOwnText;
            }

        int invert(int yy) { return (2*dev->yoff + dev->height - yy - 1); }

        void Halt();
        void ResetViewport(int, int);
        void DefineViewport();
        void Dump(int)                      { }
        void Pixel(int, int);
        void Pixels(GRmultiPt*, int);
        void Line(int, int, int, int);
        void PolyLine(GRmultiPt*, int);
        void Lines(GRmultiPt*, int);
        void Box(int, int, int, int);
        void Boxes(GRmultiPt*, int);
        void Arc(int, int, int, int, double, double);
        void Polygon(GRmultiPt*, int);
        void Zoid(int, int, int, int, int, int);
        void Text(const char*, int, int, int, int = -1, int = -1);
        void TextExtent(const char*, int*, int*);
        void SetColor(int);
        void SetLinestyle(const GRlineType*);
        void SetFillpattern(const GRfillType*);
        void DisplayImage(const GRimage*, int, int, int, int);
        double Resolution();
        void setclr(int);

        PSdev *dev;
        FILE *fp;
        int strokecnt;          // count line elements, avoid overflow
        int lastx;
        int lasty;
        int red, green, blue;
        int curfpat;
        const GRlineType *linestyle;
        const GRfillType *fpats[PS_MAX_FPs];      // fillpatterns
        bool usecolor;
        bool backg;
    };
}

#endif



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

#ifndef HPGL_H
#define HPGL_H

#include "ginterf/graphics.h"
#include "miscutil/texttf.h"
#include <stdio.h>

// Driver for HPGL plotters
//
// Call with "-f filename -w width -h height -x left_marg -y top_marg"
//
// dimensions are in inches (float format)

namespace ginterf
{
    extern HCdesc HPdesc;

    class HPdev : public GRdev
    {
    public:
        HPdev()
            {
                name = "HP";
                ident = _devHP_;
                devtype = GRhardcopy;
                data = 0;
            }

        friend struct HPparams;

        void RGBofPixel(int, int *r, int *g, int *b)    { *r = *g = *b = 0; }

        bool Init(int*, char**);
        GRdraw *NewDraw(int);

    private:
        HCdata *data;       // internal private data struct
    };

    struct HPparams : public HCdraw
    {
        HPparams()
            {
                dev = 0;
                fp = 0;
                lastx = lasty = -1;
                curpen = curline = 0;
                nofill = false;
                landscape = false;
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

        HPdev *dev;
        FILE *fp;
        int lastx;
        int lasty;
        int curpen;
        int curline;
        bool nofill;
        bool landscape;
    };
}

#endif


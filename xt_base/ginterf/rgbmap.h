
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2005 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *                                                                        *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,      *
 *   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES      *
 *   OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-        *
 *   INFRINGEMENT.  IN NO EVENT SHALL STEPHEN R. WHITELEY BE LIABLE       *
 *   FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION      *
 *   OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN           *
 *   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN         *
 *   THE SOFTWARE.                                                        *
 *                                                                        *
 * This library is available for non-commercial use under the terms of    *
 * the GNU Library General Public License as published by the Free        *
 * Software Foundation; either version 2 of the License, or (at your      *
 * option) any later version.                                             *
 *                                                                        *
 * A commercial license is available to those wishing to use this         *
 * library or a derivative work for commercial purposes.  Contact         *
 * Whiteley Research Inc., 456 Flora Vista Ave., Sunnyvale, CA 94086.     *
 * This provision will be enforced to the fullest extent possible under   *
 * law.                                                                   *
 *                                                                        *
 * This library is distributed in the hope that it will be useful,        *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU      *
 * Library General Public License for more details.                       *
 *                                                                        *
 * You should have received a copy of the GNU Library General Public      *
 * License along with this library; if not, write to the Free             *
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.     *
 *========================================================================*
 *                                                                        *
 * Ginterf Graphical Interface Library                                    *
 *                                                                        *
 *========================================================================*
 $Id: rgbmap.h,v 2.13 2010/02/25 22:46:45 stevew Exp $
 *========================================================================*/

// Rendering functions for in-core bitmap generation

#ifndef RGBMAP_H_INCLUDED
#define RGBMAP_H_INCLUDED

#include "graphics.h"
#include <stdio.h>

namespace ginterf
{
    // This can be used by other drivers to add fields to the parameter
    // structs.
    struct RGBparams : public HCdraw
    {
        RGBparams(bool ps)
            {
                dev = 0;
                bytpline = 0;
                red = green = blue = 0;
                base = 0;
                curfillpatt = 0;
                text_scale = 0.0;
                fp = 0;
                linestyle = 0;
                invert_colors = ps;
            }

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
        void SetLinestyle(const GRlineType*);
        void SetFillpattern(const GRfillType*);
        void SetColor(int);
        void DisplayImage(const GRimage*, int, int, int, int);

        void put_pixel(unsigned int, int, int);

        GRdev *dev;                // pointer to device struct
        int bytpline;              // bytes per line of image
        int red, green, blue;      // current color
        unsigned char *base;       // base of image in core (0 <-> miny)
        const GRfillType *curfillpatt; // current fillpattern, 0 solid
        double text_scale;         // scale size of text
        FILE *fp;                  // file pointer for output
        unsigned linestyle;        // linestyle storage, 0 solid
        bool invert_colors;        // true for Postscript.
    };
}

#endif



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
 $Id: psbc.h,v 2.13 2010/07/09 02:41:26 stevew Exp $
 *========================================================================*/

#ifndef PSBC_H
#define PSBC_H

#include "rgbmap.h"

// Driver for 24-bit color in-core bitmap generation with output
// in PostScript (TM) format.
//
// Call with: "-f filename -r resolution -w width -h height
//            -x left_marg -y top_marg -e "
//
// width, height, left_marg, and top_marg in inches (float format)
// resolution: 72, 75, 100, 150, 200, 300 or 400 (pixels/inch)
// -e signifies use of ascii85/rll encoding

namespace ginterf
{
    extern HCdesc PSBCdesc;
    extern HCdesc PSBCdesc_e;

    extern bool PS_rll85dump(FILE*, unsigned char*, int, int, int);

    class PSBCdev : public GRdev
    {
    public:
        PSBCdev()
            {
                name = "PSBC";
                ident = _devPSBC_;
                devtype = GRhardcopy;
                data = 0;
            }

        friend struct PSBCparams;

        void RGBofPixel(int, int *r, int *g, int *b)    { *r = *g = *b = 0; }

        bool Init(int*, char**);
        GRdraw *NewDraw(int);

    private:
        HCdata *data;       // internal private data struct
    };

    struct PSBCtext
    {
        PSBCtext() { x = y = 0; w = h = 0; xform = 0; text = 0; next = 0;
            r = g = b = 0; }

        int x, y, w, h;
        int r, g, b;
        int xform;
        char *text;
        struct PSBCtext *next;
    };

    struct PSBCparams : public RGBparams
    {
        PSBCparams() : RGBparams(true)
            { dev = 0; textlist = 0; dev_flags = GRhasOwnText; }

        HCdata *devdata() { return (((PSBCdev*)dev)->data); }
        void dump();

        void Halt();
        void ResetViewport(int, int);
        void DefineViewport()               { }
        void Dump(int)                      { }
        void Text(const char*, int, int, int, int = -1, int = -1);
        void TextExtent(const char*, int*, int*);
        double Resolution();

        PSBCtext *textlist;         // linked list head or text segs
    };
}

#endif


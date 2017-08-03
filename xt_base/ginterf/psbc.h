
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


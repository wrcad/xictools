
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

#ifndef PCL_H
#define PCL_H

#include "raster.h"

// Driver for in-core bitmap generation with output in HP PCL
// format.
//
// Call with: "-f filename -r resolution -w width -h height
//            -x left_marg -y top_marg"
//
// width, height, left_marg, and top_marg in inches (float format)
// resolution: 75, 100, 150, or 300 (pixels/inch)

namespace ginterf
{
    extern HCdesc PCLdesc;

    class PCLdev : public GRdev
    {
    public:
        PCLdev()
            {
                name = "PCL";
                ident = _devPCL_;
                devtype = GRhardcopy;
                data = 0;
            }

        friend struct PCLparams;

        void RGBofPixel(int, int *r, int *g, int *b)    { *r = *g = *b = 0; }

        bool Init(int*, char**);
        GRdraw *NewDraw(int);

    private:
        HCdata *data;       // internal private data struct
    };

    struct PCLtext
    {
        PCLtext() { x = y = xform = 0; text = 0; next = 0; }

        int x;
        int y;
        int xform;
        char *text;
        PCLtext *next;
    };

    struct PCLparams : public RASparams
    {
        PCLparams() { dev = 0; textlist = 0; }

        HCdata *devdata() { return (((PCLdev*)dev)->data); }
        void dump();

        void Halt();
        void ResetViewport(int, int);
        void DefineViewport()               { }
        void Dump(int)                      { }
        void Text(const char*, int, int, int, int = -1, int = -1);
        void TextExtent(const char*, int*, int*);
        double Resolution();

        PCLtext *textlist;          // linked list head or text segs
    };
}

#endif


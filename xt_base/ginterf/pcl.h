
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
 $Id: pcl.h,v 2.11 2010/07/09 02:41:26 stevew Exp $
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


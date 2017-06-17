
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
 $Id: xdraw.h,v 2.73 2009/12/09 04:41:23 stevew Exp $
 *========================================================================*/

#ifndef XDRAW_H
#define XDRAW_H

#define XD_NUM_LINESTYLES 16
#define XD_NUM_FILLPATTS  16

namespace ginterf
{
    struct Xparams;

    struct Xdraw
    {
        Xdraw(const char*, unsigned long);
        ~Xdraw();

        static bool check_error();
        unsigned long create_pixmap(int, int);
        bool destroy_pixmap(unsigned long);
        bool copy_drawable(unsigned long, unsigned long, int, int, int, int,
            int, int);
        bool draw(int, int, int, int);
        bool get_drawable_size(unsigned long, int*, int*);
        unsigned long reset_drawable(unsigned long);

        void Clear();
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
        void Text(const char*, int, int, int);
        void TextExtent(const char*, int*, int*);
        void DefineColor(int*, int, int, int);
        void SetBackground(int);
        void SetWindowBackground(int);
        void SetColor(int);
        void DefineLinestyle(int, int);
        void SetLinestyle(int);
        void DefineFillpattern(int, int, int, unsigned char*);
        void SetFillpattern(int);
        void Update();
        void SetXOR(int);

    private:
        Xparams *xp;
    };

    extern HCdesc Xdesc;

    class Xdev : public GRdev
    {
    public:
        Xdev()
            {
                name = "X";
                ident = _devX_;
                devtype = GRhardcopy;
                data = 0;
            }

        friend struct Xparams;

        void RGBofPixel(int, int *r, int *g, int *b)    { *r = *g = *b = 0; }

        bool Init(int*, char**);
        GRdraw *NewDraw(int);

    private:
        HCdata *data;
    };
}

#endif


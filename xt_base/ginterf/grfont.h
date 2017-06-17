
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
 $Id: grfont.h,v 2.8 2013/09/23 21:59:46 stevew Exp $
 *========================================================================*/

#ifndef GRFONT_H
#define GRFONT_H

#include <stdio.h>

//
// Scalable vector text generation
//

namespace ginterf
{

    struct GRvecFont
    {
        struct Cpair
        {
            char x;
            char y;
        };

        struct Cstroke
        {
            int numpts;
            Cpair *cp;
        };

        struct Character
        {
            int numstroke;
            Cstroke *stroke;
            short ofst;
            short width;
        };

    public:
        GRvecFont();

        int charWidth(int c) const
            {
                if (c >= vf_startChar && c <= vf_endChar)
                    return (vf_charset[c - vf_startChar]->width);
                else if (c == '\n')
                    return (0);
                else
                    return (vf_cellWidth);
            }

        int charHeight(int) const
            {
                return (vf_cellHeight);
            }

        Character *entry(int c) const
            {
                if (c >= vf_startChar && c <= vf_endChar)
                    return (vf_charset[c - vf_startChar]);
                return (0);
            }

        int lineExtent(const char *string) const
            {
                int xw = 0;
                for (const char *s = string; *s; s++) {
                    int w = charWidth(*s);
                    if (!w)
                        // newline
                        break;
                    xw += w;
                }
                return (xw);
            }

        int cellWidth()             const { return (vf_cellWidth); }
        int cellHeight()            const { return (vf_cellHeight); }
        int startChar()             const { return (vf_startChar); }
        int endChar()               const { return (vf_endChar); }

        void textExtent(const char*, int*, int*, int*, int = 0) const;
        int xoffset(const char*, int, int, int) const;
        void renderText(GRdraw*, const char*, int, int, int, int, int,
            int = 0) const;
        void parseChars(FILE*);
        void dumpFont(FILE*) const;

    private:
        Character **vf_charset;
        int vf_cellWidth;
        int vf_cellHeight;
        int vf_startChar;
        int vf_endChar;
    };
}

extern ginterf::GRvecFont FT;

#endif


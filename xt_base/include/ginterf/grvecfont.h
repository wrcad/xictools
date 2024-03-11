
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

#ifndef GRVECFONT_H
#define GRVECFONT_H

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


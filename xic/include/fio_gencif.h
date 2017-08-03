
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
 * Xic Integrated Circuit Layout and Schematic Editor                     *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#ifndef FIO_GENCIF_H
#define FIO_GENCIF_H

#include <ctype.h>

//
// A class to write CIF to a file.
//

class sCifGen
{
public:
    void End(FILE *fp)
        {
            fprintf(fp, "E\n");
        }

    void BeginSymbol(FILE *fp, int sym_num, int A, int B)
        {
            fprintf(fp, "DS %d %d %d;\n", sym_num, A, B);
        }

    void EndSymbol(FILE *fp)
        {
            fprintf(fp, "DF;\n");
        }

    void BeginCall(FILE *fp, int num)
        {
            fprintf(fp, "C %d", num);
        }

    void EndCall(FILE *fp)
        {
            fprintf(fp, ";\n");
        }

    void Translation(FILE *fp, int x, int y)
        {
            if (x != 0 || y != 0)
                fprintf(fp, " T %d %d", x, y);
        }

    void Rotation(FILE *fp, int x, int y)
        {
            if (x != 1 || y != 0)
                fprintf(fp, " R %d %d", x, y);
        }

    void MirrorX(FILE *fp, bool mx)
        {
            if (mx)
                fprintf(fp, " MX");
        }

    void MirrorY(FILE *fp, bool my)
        {
            if (my)
                fprintf(fp, " MY");
        }

    void Layer(FILE *fp, const char *mask)
        {
            fprintf(fp, "L %s;\n", mask);
        }

    void UserExtension(FILE *fp, char digit, const char *string)
        {
            fprintf(fp, "%c ", digit);
            if (string) {
                sLstr lstr;
                fix_sc(lstr, string);
                fputs(lstr.string(), fp);
            }
            fputs(";\n", fp);
        }

    void UserExtension(FILE *fp, char digit1, char digit2, const char *string)
        {
            fprintf(fp, "%c%c ", digit1, digit2);
            if (string) {
                sLstr lstr;
                fix_sc(lstr, string);
                fputs(lstr.string(), fp);
            }
            fputs(";\n", fp);
        }

    void Comment(FILE *fp, const char *text)
        {
            fprintf(fp, "(%s);\n", text);
        }

    void Box(FILE *fp, const BBox &BB)
        {
            fprintf(fp, "B %d %d %d %d;\n", BB.right-BB.left,
                BB.top-BB.bottom, (BB.left + BB.right)/2,
                (BB.bottom + BB.top)/2);
        }

    void BoxWH(FILE *fp, int width, int height, int x, int y)
        {
            fprintf(fp, "B %d %d %d %d;\n", width, height, x, y);
        }

    void Polygon(FILE *fp, const Point *points, int numpts,
        int x = 0, int y = 0)
        {
            char buf[80];
            *buf = 'P';
            *(buf+1) = '\0';
            out_path(fp, points, numpts, buf, x, y);
        }

    // Extension: W0 -> pathtype 0, etc.  Pass style = -1 for regular CIF
    void Wire(FILE *fp, int width, int style, const Point *points, int numpts,
        int x = 0, int y = 0)
        {
            char buf[80];
            if (style > 2)
                style = 2;
            buf[0] = 'W';
            if (style >= 0) {
                buf[1] = '0' + style;
                buf[2] = ' ';
                mmItoA(buf + 3, width);
            }
            else {
                buf[1] = ' ';
                mmItoA(buf + 2, width);
            }
            out_path(fp, points, numpts, buf, x, y);
        }

    // Label and property strings can be long, try to avoid problems with
    // overflow in fprintf.

    void Label(FILE *fp, const char *label, int x, int y, int xform, int width,
            int height)
        {
            fputs("94 ", fp);
            fputs(label, fp);
            fprintf(fp, " %d %d %d %d %d;\n", x, y, xform, width, height);
        }

    void Property(FILE *fp, int value, const char *string)
        {
            fprintf(fp, "5 %d%s", value, string ? " " : "");
            if (string) {
                sLstr lstr;
                fix_sc(lstr, string);
                fputs(lstr.string(), fp);
            }
            fputs(";\n", fp);
        }

    void BoundBox(FILE *fp, const BBox &BB)
        {
            fprintf(fp, "(BBox %d,%d %d %d);\n", BB.left, BB.top,
                BB.right - BB.left, BB.top - BB.bottom);
        }

    //
    //
    //

    void End(sLstr &lstr)
        {
            lstr.add("E\n");
        }

    void BeginSymbol(sLstr &lstr, int sym_num, int A, int B)
        {
            lstr.add("DS ");
            lstr.add_i(sym_num);
            lstr.add_c(' ');
            lstr.add_i(A);
            lstr.add_c(' ');
            lstr.add_i(B);
            lstr.add(";\n");
        }

    void EndSymbol(sLstr &lstr)
        {
            lstr.add("DF;\n");
        }

    void BeginCall(sLstr &lstr, int num)
        {
            lstr.add("C ");
            lstr.add_i(num);
        }

    void EndCall(sLstr &lstr)
        {
            lstr.add(";\n");
        }

    void Translation(sLstr &lstr, int x, int y)
        {
            if (x != 0 || y != 0) {
                lstr.add(" T ");
                lstr.add_i(x);
                lstr.add_c(' ');
                lstr.add_i(y);
            }
        }

    void Rotation(sLstr &lstr, int x, int y)
        {
            if (x != 1 || y != 0) {
                lstr.add(" R ");
                lstr.add_i(x);
                lstr.add_c(' ');
                lstr.add_i(y);
            }
        }

    void MirrorX(sLstr &lstr, bool mx)
        {
            if (mx)
                lstr.add(" MX");
        }

    void MirrorY(sLstr &lstr, bool my)
        {
            if (my)
                lstr.add(" MY");
        }

    void Layer(sLstr &lstr, const char *mask)
        {
            lstr.add("L ");
            lstr.add(mask);
            lstr.add(";\n");
        }

    void UserExtension(sLstr &lstr, char digit, const char *string)
        {
            lstr.add_c(digit);
            lstr.add_c(' ');
            if (string) {
                sLstr tstr;
                fix_sc(tstr, string);
                lstr.add(tstr.string());
            }
            lstr.add(";\n");
        }

    void UserExtension(sLstr &lstr, char digit1, char digit2, const char *string)
        {
            lstr.add_c(digit1);
            lstr.add_c(digit2);
            lstr.add_c(' ');
            if (string) {
                sLstr tstr;
                fix_sc(tstr, string);
                lstr.add(tstr.string());
            }
            lstr.add(";\n");
        }

    void Comment(sLstr &lstr, const char *text)
        {
            lstr.add_c('(');
            lstr.add(text);
            lstr.add_c(')');
            lstr.add(";\n");
        }

    void Box(sLstr &lstr, const BBox &BB)
        {
            lstr.add("B ");
            lstr.add_i(BB.right - BB.left);
            lstr.add_c(' ');
            lstr.add_i(BB.top - BB.bottom);
            lstr.add_c(' ');
            lstr.add_i((BB.left + BB.right)/2);
            lstr.add_c(' ');
            lstr.add_i((BB.bottom + BB.top)/2);
            lstr.add(";\n");
        }

    void BoxWH(sLstr &lstr, int width, int height, int x, int y)
        {
            lstr.add("B ");
            lstr.add_i(width);
            lstr.add_c(' ');
            lstr.add_i(height);
            lstr.add_c(' ');
            lstr.add_i(x);
            lstr.add_c(' ');
            lstr.add_i(y);
            lstr.add(";\n");
        }

    void Polygon(sLstr &lstr, const Point *points, int numpts,
        int x = 0, int y = 0)
        {
            lstr.add_c('P');
            out_path(lstr, points, numpts, 1, x, y);
        }

    // Extension: W0 -> pathtype 0, etc.  Pass style = -1 for regular CIF
    void Wire(sLstr &lstr, int width, int style, const Point *points,
        int numpts, int x = 0, int y = 0)
        {
            if (style > 2)
                style = 2;
            lstr.add_c('W');
            if (style >= 0)
                lstr.add_c('0' + style);
            lstr.add_c(' ');
            lstr.add_i(width);
            out_path(lstr, points, numpts, strlen(lstr.string()), x, y);
        }

    void Label(sLstr &lstr, const char *label, int x, int y, int xform,
        int width, int height)
        {
            lstr.add("94 ");
            lstr.add(label);
            lstr.add_c(' ');
            lstr.add_i(x);
            lstr.add_c(' ');
            lstr.add_i(y);
            lstr.add_c(' ');
            lstr.add_i(xform);
            lstr.add_c(' ');
            lstr.add_i(width);
            lstr.add_c(' ');
            lstr.add_i(height);
            lstr.add(";\n");
        }

    void Property(sLstr &lstr, int value, const char *string)
        {
            lstr.add("5 ");
            lstr.add_i(value);
            if (string) {
                lstr.add_c(' ');
                sLstr tstr;
                fix_sc(tstr, string);
                lstr.add(tstr.string());
            }
            lstr.add(";\n");
        }

    void BoundBox(sLstr &lstr, const BBox &BB)
        {
            lstr.add("(BBox ");
            lstr.add_i(BB.left);
            lstr.add_c(',');
            lstr.add_i(BB.top);
            lstr.add_c(' ');
            lstr.add_i(BB.right - BB.left);
            lstr.add_c(' ');
            lstr.add_i(BB.top - BB.bottom);
            lstr.add(");\n");
        }

private:
    void out_path(FILE*, const Point*, int, char*, int, int);
    void out_path(sLstr &lstr, const Point*, int, int, int, int);
    void fix_sc(sLstr&, const char*);
};
extern sCifGen Gen;

#endif


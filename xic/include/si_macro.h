
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
 * Xic Integrated Circuit Layout and Schematic Editor                     *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#ifndef SI_MACRO_H
#define SI_MACRO_H

#include "miscutil/lstring.h"

#define MACRO_BUFSIZE 1024

typedef void(*MacroPredefFunc)(MacroHandler*);

struct SImacroHandler : public MacroHandler
{
    SImacroHandler()
        {
            line_cnt        = 0;
            if_else_lev     = 0;
            define_kw       = "define";
            eval_kw         = "eval";
            if_kw           = "if";
            ifdef_kw        = "ifdef";
            ifndef_kw       = "ifndef";
            else_kw         = "else";
            endif_kw        = "endif";

            if (predef_func)
                (*predef_func)(this);
        }

    void setup_keywords(const char *dkw, const char *ikw, const char *idkw,
        const char *indkw, const char *elkw, const char *enkw)
        {
            if (dkw)
                define_kw = dkw;
            if (ikw)
                if_kw = ikw;
            if (idkw)
                ifdef_kw = idkw;
            if (indkw)
                ifndef_kw = indkw;
            if (elkw)
                else_kw = elkw;
            if (enkw)
                endif_kw = enkw;
        }

    int line_number()               const { return (line_cnt); }
    void inc_line_number()          { line_cnt++; }
    void set_line_number(int n)     { line_cnt = n; }

    bool handle_keyword(FILE*, char*, const char*, const char*, char**);

    static MacroPredefFunc register_predef_func(MacroPredefFunc f)
        {
            MacroPredefFunc tmp = predef_func;
            predef_func = f;
            return (tmp);
        }

private:
    void skip_block(FILE*, char*);

    int line_cnt;
    int if_else_lev;
    const char *define_kw;
    const char *eval_kw;
    const char *if_kw;
    const char *ifdef_kw;
    const char *ifndef_kw;
    const char *else_kw;
    const char *endif_kw;

    static MacroPredefFunc predef_func;
};

#endif



/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2011 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *  WHITELEY RESEARCH INCORPORATED PROPRIETARY SOFTWARE                   *
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
 *                                                                        *
 * XIC Integrated Circuit Layout and Schematic Editor                     *
 *                                                                        *
 *========================================================================*
 $Id: si_macro.h,v 5.2 2011/02/07 01:40:38 stevew Exp $
 *========================================================================*/

#ifndef SI_MACRO_H
#define SI_MACRO_H

#include "lstring.h"

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


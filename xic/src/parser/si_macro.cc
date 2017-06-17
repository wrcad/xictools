
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2007 Whiteley Research Inc, all rights reserved.        *
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
 $Id: si_macro.cc,v 5.3 2015/03/12 17:30:00 stevew Exp $
 *========================================================================*/

#include "cd.h"
#include "si_macro.h"
#include "si_parsenode.h"
#include "si_parser.h"


MacroPredefFunc SImacroHandler::predef_func = 0;

namespace {
    char *compose_msg(const char *kw, const char *what, const char *fname,
        int line)
    {
        sLstr lstr;
        lstr.add_c('\'');
        lstr.add(kw);
        lstr.add_c('\'');
        lstr.add_c(' ');
        lstr.add(what);
        if (fname && *fname) {
            lstr.add(" in file ");
            lstr.add(fname);
        }
        lstr.add(" at line ");
        lstr.add_i(line);
        lstr.add_c('.');
        return (lstr.string_trim());
    }
}


// Handle the macro keywords, return true if one seen.  If name is not
// null, the buf should contain the rest of the line, i.e., not
// including the keyword.  If name is null, the buf should lead with
// the keyword.  The fname is used in the error message, if not null. 
// On error, a message is returned in errmsg, if not null.
//
bool
SImacroHandler::handle_keyword(FILE *fp, char *buf, const char *name,
    const char *fname, char **errmsg)
{
    if (errmsg)
        *errmsg = 0;
    bool skip_kw = false;
    if (!name) {
        name = buf;
        skip_kw = true;
    }

    sLstr lstr;
    if (lstring::cimatch(define_kw, name)) {
        char *t = buf;
        if (skip_kw)
            lstring::advtok(&t);
        if (lstring::cimatch(eval_kw, t)) {
            lstring::advtok(&t);
            char tbf[64];
            char *sm = macro_expand(t);
            if (!sm) {
                if (errmsg) {
                    sprintf(tbf, "%s %s", define_kw, eval_kw);
                    *errmsg = compose_msg(tbf, "expansion failed", fname,
                        line_cnt);
                }
                return (true);
            }
            t = sm;
            char *dname = lstring::getqtok(&t);
            if (!dname) {
                if (errmsg) {
                    sprintf(tbf, "%s %s", define_kw, eval_kw);
                    *errmsg = compose_msg(tbf, "syntax error", fname,
                        line_cnt);
                }
                delete [] sm;
                return (true);
            }
            char *p = lstring::stpcpy(buf, dname);
            *p++ = ' ';
            if (SIparse()->evaluate(t, p, MACRO_BUFSIZE - (p-buf)) != OK) {
                if (errmsg) {
                    sprintf(tbf, "%s %s", define_kw, eval_kw);
                    *errmsg = compose_msg(tbf, "evaluation failed",
                        fname, line_cnt);
                }
            }
            else if (!parse_macro(buf, false)) {
                if (errmsg) {
                    sprintf(tbf, "%s %s", define_kw, eval_kw);
                    *errmsg = compose_msg(tbf, "post-eval parse failed",
                        fname, line_cnt);
                }
            }
            delete [] dname;
            delete [] sm;
        }
        else {
            char *sm = macro_expand(t);
            if (!sm) {
                if (errmsg) {
                    *errmsg = compose_msg(define_kw, "expansion failed", fname,
                        line_cnt);
                }
                return (true);
            }
            if (!parse_macro(sm, false)) {
                if (errmsg) {
                    *errmsg = compose_msg(define_kw, "parse failed", fname,
                        line_cnt);
                }
            }
            delete [] sm;
        }
        return (true);
    }
    if (lstring::cimatch(if_kw, name)) {
        char *t = buf;
        if (skip_kw)
            lstring::advtok(&t);
        if_else_lev++;
        double d = 0.0;
        char *sm = macro_expand(t);
        if (!sm) {
            if (errmsg) {
                *errmsg = compose_msg(if_kw, "expansion failed", fname,
                    line_cnt);
            }
            return (true);
        }
        char obf[64];
        if (SIparse()->evaluate(sm, obf, 64) == OK) {
            if (sscanf(obf, "%lf", &d) != 1)
                // Non-null string, take as true.
                d = 1.0;
        }
        else {
            if (errmsg) {
                *errmsg = compose_msg(if_kw, "evaluation failed", fname,
                    line_cnt);
                return (true);
            }
        }
        delete [] sm;
        if ((int)d == 0)
            skip_block(fp, buf);
        return (true);
    }
    if (lstring::cimatch(ifdef_kw, name)) {
        char *t = buf;
        if (skip_kw)
            lstring::advtok(&t);
        char *cp = lstring::gettok(&t);
        if (!cp) {
            if (errmsg) {
                *errmsg = compose_msg(ifdef_kw, "missing identifier", fname,
                    line_cnt);
                return (true);
            }
        }
        if_else_lev++;
        if (!find_text(cp))
            skip_block(fp, buf);
        delete [] cp;
        return (true);
    }
    if (lstring::cimatch(ifndef_kw, name)) {
        char *t = buf;
        if (skip_kw)
            lstring::advtok(&t);
        char *cp = lstring::gettok(&t);
        if (!cp) {
            if (errmsg) {
                *errmsg = compose_msg(ifndef_kw, "missing identifier", fname,
                    line_cnt);
                return (true);
            }
        }
        if_else_lev++;
        if (find_text(cp))
            skip_block(fp, buf);
        delete [] cp;
        return (true);
    }
    if (lstring::cimatch(else_kw, name)) {
        if (if_else_lev)
            skip_block(fp, buf);
        return (true);
    }
    if (lstring::cimatch(endif_kw, name)) {
        if (if_else_lev)
            if_else_lev--;
        return (true);
    }
    return (false);
}


void
SImacroHandler::skip_block(FILE *fp, char *inbuf)
{
    int level = 1;
    char *s;
    char kwbuf[64];
    while ((s = fgets(inbuf, MACRO_BUFSIZE, fp)) != 0) {
        line_cnt++;
        while (isspace(*s))
            s++;
        char *t = kwbuf;
        while (*s && !isspace(*s) && t - kwbuf < 63)
            *t++ = *s++;
        *t = 0;

        if (lstring::cieq(kwbuf, if_kw) ||
                lstring::cieq(kwbuf, ifdef_kw) ||
                lstring::cieq(kwbuf, ifndef_kw))
            level++;
        else if (lstring::cieq(kwbuf, endif_kw)) {
            level--;
            if (!level) {
                if_else_lev--;
                break;
            }
        }
        else if (lstring::cieq(kwbuf, else_kw)) {
            if (level == 1)
                break;
        }
    }
}



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

#include "cd.h"
#include "geo_zlist.h"
#include "si_parsenode.h"
#include "si_parser.h"
#include "si_lexpr.h"
#include "si_handle.h"
#include "si_interp.h"
#include "si_macro.h"
#include "si_lspec.h"
#include "pcell_params.h"

#include <ctype.h>


//
// The front-end command loop.
//


sCx::sCx(sCx *nx)
{
    cx_sfunc        = SI()->siMain;
    cx_line_cnt     = SI()->siLineCount;
    cx_ex_line      = SI()->siExecLine;
    cx_level        = SI()->siLevel;
    cx_stack        = SI()->siStack;
    cx_variables    = SIparse()->getVariables();
    cx_defines      = SI()->siDefines;
    cx_next         = nx;
}
// End of sCx functions.


sLCx::sLCx(sLCx *n)
{
    next = n;
    lcx = new SIlexprCx;
    zlbak = 0;
}

sLCx::~sLCx()
{
    delete lcx;
    Zlist::destroy(zlbak);
}
// End of sLCx functions.


SIinterp *SIinterp::instancePtr = 0;

SIinterp::SIinterp()
{
    if (instancePtr) {
        fprintf(stderr, "Singleton class SIinterp already instantiated.\n");
        exit(1);
    }
    instancePtr = this;

    siContext = 0;
    siLCx = 0;
    siStack = 0;
    siCurFunc = 0;
    siFuncTab = 0;

    siDefines = 0;
    siMacrosPreset = 0;

    siStackPtr = 0;
    siBlockLineref = 0;
    siLineCount = 0;
    siExecLine = 0;
    siLevel = 0;
    siBlockStepping = false;
    siSkipJunk = false;
    siError = false;
    siInterrupt = false;
    siHalt = false;

    cryptKey = 0;
    siLocalContext = 0;
}


// Private static error exit.
//
void
SIinterp::on_null_ptr()
{
    fprintf(stderr, "Singleton class SIinterp used before instantiated.\n");
    exit(1);
}


namespace {
    // Strip out Microsoft crap.
    //
    inline int
    my_getc(SIfile *sfp)
    {
        int c;
        do { c = sfp->sif_getc(); } while (c == '\r');
        return (c);
    }
}


// Static function.
// Grab a line of input, take care of continuations, only one of the
// args should be nonzero.
//
char *
SIinterp::NextLine(int *linecnt, SIfile *sfp, stringlist **wl,
    const char **line)
{
    sLstr lstr;
    if (sfp) {
        for (;;) {
            int c = my_getc(sfp);
            if (c == EOF)
                return (0);
            if (c == '\\') {
                int d = my_getc(sfp);
                if (d == '\n') {
                    (*linecnt)++;
                    continue;
                }
                if (d == EOF)
                    return (0);
                lstr.add_c(c);
                lstr.add_c(d);
                continue;
            }
            lstr.add_c(c);
            if (c == '\n')
                break;
        }
        (*linecnt)++;
        return (lstr.string_trim());
    }
    else if (wl && *wl) {
        char *t = (*wl)->string;
        while (t && *t && *t != '\n') {
            if (*t == '\\' && *(t+1) == '\n') {
                *wl = (*wl)->next;
                if (!*wl)
                    break;
                t = (*wl)->string;
                (*linecnt)++;
                continue;
            }
            lstr.add_c(*t++);
        }
        lstr.add_c('\n');
        (*linecnt)++;
        if (*wl)
            *wl = (*wl)->next;
        return (lstr.string_trim());
    }
    else if (line) {
        const char *t = *line;
        if (!t || !*t || !*(t+1) || !*(t+2))
            return (0);
        // skip margin
        // From the debugger, the first two columns are used to indicate
        // the current line ('>') and breakpoints ('B').
        if (*(t+1) == ' ' && (*t == ' ' || *t == '>' || *t == 'B'))
            t += 2;
        while (*t && *t != '\n') {
            if (*t == '\\' && *(t+1) == '\n') {
                t += 2;   // continuation
                if (!*t || !*(t+1) || !*(t+2))
                    break;
                if (*(t+1) == ' ' && (*t == ' ' || *t == '>' || *t == 'B'))
                    t += 2;
                (*linecnt)++;
                continue;
            }
            lstr.add_c(*t++);
        }
        lstr.add_c('\n');
        if (*t == '\n')
            t++;
        *line = t;
        (*linecnt)++;
        return (lstr.string_trim());
    }
    return (0);
}


namespace {
    // Return a copy of the parameter assignment string.
    //
    char *assign_string(const char *name, const char *value)
    {
        if (name && *name) {
            if (value && *value) {
                char *str = new char[strlen(name) + strlen(value) + 4];
                sprintf(str, "%s = %s", name, value);
                return (str);
            }
            return (lstring::copy(name));
        }
        return (0);
    }

    // Used for single-stepping and in server mode.
    SIlexprCx staticCx;
}


// Main method to interpret a script file.  This includes
// "preprocessing" of #ifdef, etc.  Only one of fp, wl, line should be
// nonzero, and this represents the input stream.  If step is true,
// only one valid statement is executed.  Returns the number of the
// next line to execute.
//
// *** Step only works with string input (line != 0)
//
int
SIinterp::Interpret(SIfile *sfp, stringlist *wl, const char **line,
    siVariable *result, bool step, const PCellParam *params)
{
    // Function is not reentrant when single-stepping.

    bool skip_leading = false;
    ClearInterrupt();
    GetError();
    if (sfp || wl || siContext)
        step = false;

step_again:

    if (!step) {
        push_cx();
        // set the variables
        for (const PCellParam *p = params; p; p = p->next()) {
            char *s = p->getAssignment();
            if (s) {
                const char *t = s;
                set_block(&t, 0);
                delete [] s;
            }
        }
        if (!siDefines)
            siDefines = new SImacroHandler;
        if (sfp && sfp->filename) {
            // add:  #define THIS_SCRIPT filename
            char *s = new char[strlen(sfp->filename) + 20];
            sprintf(s, "THIS_SCRIPT %s", sfp->filename);
            siDefines->parse_macro(s, true);
            delete [] s;
        }
    }
    else if (siBlockStepping) {
        eval_stmt(0, 0, &staticCx);
        if (IsHalted() || SIparse()->ifCheckInterrupt())
            clear();
        if (siStack)
            return (siStack->bcur->co_lineno);
        else {
            // done
            siBlockStepping = false;
            SIcontrol::destroy(siMain.sf_text);
            siMain.sf_text = 0;
            siMain.sf_end = 0;
            skip_leading = true;
            if (!*line || !**line)
                return (siLineCount);
        }
    }
    else if (siSkipJunk) {
        staticCx.reset();
        skip_leading = true;
        siSkipJunk = false;
    }

    if (!siDefines)
        siDefines = new SImacroHandler;

    const char *last = line ? *line : 0;
    int lastlc = siLineCount;
    bool infunc = false;
    char *string;
    for ( ; (string = NextLine(&siLineCount, sfp, &wl, line)) != 0;
            last = line ? *line : 0) {
        const char *s = string;
        while (isspace(*s))
            s++;
        if (*s) {
            if (siDefines && *s != '#') {
                char *ms = siDefines->macro_expand(s);
                delete [] string;
                s = string = ms;
            }
            if (s && *s) {
                if (*s == '#') {
                    if (isalpha(*(s+1))) {
                        s++;
                        handle_keyword(sfp, &wl, line, s);

                        /* debugging
                        else if (lstring::cimatch("print", s))
                            siDefines->print(stdout, "#define", true);
                        */
                    }
                }
                else {
                    if (step && skip_leading) {
                        // We want to skip over comments, white space,
                        // and function definitions.  Returned line
                        // should be the first executable statement in
                        // main function.

                        bool xnow = true;
                        if (!infunc) {
                            if (lstring::prefix("function ", s)) {
                                infunc = true;
                                xnow = false;
                            }
                        }
                        else {
                            if (lstring::prefix("endfunc", s))
                                infunc = false;
                            xnow = false;
                        }
                        if (xnow) {
                            siLineCount = lastlc;
                            if (line)
                                *line = last;
                            delete [] string;
                            return (siLineCount);
                        }
                    }
                    set_block(&s, lastlc);
                    if (SIparse()->hasError())
                        // shouldn't be any
                        LineError("error building parse tree");
                }
            }
        }
        lastlc = siLineCount;
        delete [] string;
    }

    if (!skip_leading) {
        if (step) {
            if (!IsHalted()) {
                siBlockStepping = true;
                push(siMain.sf_text);
                goto step_again;
            }
        }
        else {
            if (siMain.sf_text && !IsHalted()) {
                SIlexprCx cx;
                push(siMain.sf_text);
                while (siStack) {
                    eval_stmt(0, result, &cx);
                    if (IsHalted() || SIparse()->ifCheckInterrupt())
                        clear();
                }
            }
            pop_cx();
        }
    }
    return (siLineCount);
}


// This variation is for server mode.  The lines are provided one at a time,
// through the following calling sequence:
//   siLineInterp(0, 0, true, params)  // this initializes everything
//   siLineInterp(string, &r)          // call for each line
//   ...
//   Clear()                           // done, clean up
// The return value is one of:
//   0   statement executed successfully
//   1   "end" needed, block not complete
//  -1   error
//
int
SIinterp::LineInterp(const char *str, siVariable *res,
    bool init, const PCellParam *params)
{
    ClearInterrupt();
    if (init) {
        Init();
        staticCx.reset();
        for (const PCellParam *p = params; p; p = p->next()) {
            char *s = p->getAssignment();
            if (s) {
                const char *t = s;
                set_block(&t, 0);
                delete [] s;
            }
        }
    }
    if (!str)
        return (0);

    while (isspace(*str))
        str++;
    while (*str) {
        set_block(&str, 0);
        if (SIparse()->hasError()) {
            // shouldn't be any
            LineError("error building parse tree");
            Clear();
            return (-1);
        }
        if (IsHalted()) {
            ClearInterrupt();
            Clear();
            return (-1);
        }
    }
    if (siMain.sf_text) {
        if (siMain.sf_end->co_parent)
            // in a block
            return (1);
        if (siCurFunc != &siMain)
            // in a function definition
            return (1);

        push(siMain.sf_text);
        while (siStack) {
            eval_stmt(res, res, &staticCx);
            if (IsHalted() || SIparse()->ifCheckInterrupt())
                clear();
        }
        SIcontrol::destroy(siMain.sf_text);
        siMain.sf_text = 0;
        siMain.sf_end = 0;
    }
    if (IsHalted()) {
        Clear();
        return (-1);
    }
    return (0);
}


// Return a SIfunc which can be stored away and executed later.  Exactly
// one of the first three args should be nonzero.  The vars, if nonzero,
// will be used during the parse (new variables added to end).
//
SIfunc *
SIinterp::GetBlock(SIfile *sfp, stringlist *wl, const char **line,
    siVariable **vp)
{
    push_cx();
    if (vp)
        SIparse()->setVariables(*vp);

    siLineCount = 0;
    ClearInterrupt();
    siLevel = 0;
    int lastlc = siLineCount;
    char *s;
    for ( ; (s = NextLine(&siLineCount, sfp, &wl, line)) != 0; ) {
        const char *t = s;
        while (isspace(*t))
            t++;
        set_block(&t, lastlc);
        lastlc = siLineCount;
        delete [] s;
    }
    SIfunc *sf = 0;
    if (siMain.sf_text) {
        sf = new SIfunc();
        sf->sf_text = siMain.sf_text;
        sf->sf_variables = SIparse()->getVariables();
    }
    siMain.sf_text = 0;
    siMain.sf_end = 0;
    if (vp) {
        *vp = SIparse()->getVariables();
        SIparse()->setVariables(0);
    }
    pop_cx();
    return (sf);
}


// Evaluate a control block.  This is used in the DRC user functions to
// evaluate a rule block, and in normal functions to call a subfunction.
// In the latter case, the args and res field are set, otherwise not.
// When evaluating a drc rule, we want to switch the context of the
// layer expressions, otherwise the layer expression context is global.
//
// The parse tree contains pointers to the variables in the sf_variables
// list, thus for variables to be automatic, the content of the variables
// is copied and restored.
//
XIrt
SIinterp::EvalFunc(SIfunc *sf, void *datap, siVariable *args, siVariable *res)
{
    if (!sf)
        return (XIbad);
    if (siStackPtr > CALLSTACKSIZE - 2) {
        LineError("call stack overflow -- max depth %d", CALLSTACKSIZE);
        return (XIbad);
    }
    siStackPtr++;
    siCallStack[siStackPtr].sf = sf;
    siCallStack[siStackPtr].vars = SIparse()->getVariables();
    siCallStack[siStackPtr].funcvars = 0;
    siCallStack[siStackPtr].stack = siStack;
    siStack = 0;

    XIrt ret = XIok;
    SIlexp_list *tmp_expr = 0;
    if (!args && !res)
        tmp_expr = SIparse()->setExprs(sf->sf_exprs);
    else {
        sf->init_local_vars();
        if (!sf->save_vars(&siCallStack[siStackPtr].funcvars))
            ret = XIbad;

        // Initialize arguments (already checked that argc is ok)
        SIarg *p = sf->sf_args;
        for (Variable *vals = args; p; p = p->next, vals++) {
            if (p->var) {
                p->var->type = vals->type;
                p->var->content = vals->content;
                p->var->flags = 0;
            }
        }
    }
    SIparse()->setVariables(sf->sf_variables);

    if (ret == XIok) {
        push(sf->sf_text);
        while (siStack) {
            eval_stmt(0, res, datap);
            if (args && (IsHalted() || SIparse()->ifCheckInterrupt())) {
                clear();
                ret = XIintr;
            }
        }

        if (!args && !res)
            sf->sf_exprs = SIparse()->setExprs(tmp_expr);
        else if (!sf->restore_vars(siCallStack[siStackPtr].funcvars, res))
            ret = XIbad;
    }
    SIparse()->setVariables(siCallStack[siStackPtr].vars);
    siStack = siCallStack[siStackPtr].stack;
    siStackPtr--;
    siRetinfo.type = CS_NORMAL;
    siRetinfo.rt.value = 0;
    return (ret);
}


// Initialize the interpreter context.
//
void
SIinterp::Init()
{
    Clear();
    siLineCount = 0;
    siExecLine = -1;
    siLevel = 0;
    siSkipJunk = true;

    ClearInterrupt();
}


// Clear the interpreter context.
//
void
SIinterp::Clear()
{
    siBlockStepping = false;
    SIcontrol::destroy(siMain.sf_text);
    delete siDefines;
    siDefines = siMacrosPreset;
    siMacrosPreset = 0;
    clear();

    if (siCurFunc && siCurFunc != &siMain) {
        // We get here on a parse error in a sub-function
        siCurFunc->sf_variables = SIparse()->getVariables();
        FreeSubfunc(siCurFunc->sf_name);
        siCurFunc = &siMain;
        SIparse()->setVariables(siMain.sf_variables);
    }
    SIparse()->clearVariables();
    SIparse()->clearGlobals();
    memset(&siMain, 0, sizeof(SIfunc));
    if (!siContext || !siContext->next())
        // context shared with called scripts, only clear at top level
        Cleanup();
}


//
// The following functions deal with the sub-functions, which are kept
// in the siFuncTab symbol table.
//

// Pop up a listing of the sub-functions.
//
bool
SIinterp::ShowSubfuncs()
{
    stringlist *sl = GetSubfuncList();
    if (sl) {
        SIparse()->ifShowFunctionList(sl);
        stringlist::destroy(sl);
        return (true);
    }
    return (false);
}


// Find and return the sub-function and arg count.
//
SIfunc *
SIinterp::GetSubfunc(const char *name, int *argc)
{
    if (!name || !*name)
        return (0);
    if (!siFuncTab)
        return (0);
    SIfunc *sf = (SIfunc*)SymTab::get(siFuncTab, name);
    if (!sf || sf == (SIfunc*)ST_NIL) {
        // not found
        LineError("no such function %s", name);
        return (0);
    }

    int cnt = 0;
    for (SIarg *a = sf->sf_args; a; a = a->next, cnt++) ;
    *argc = cnt;
    return (sf);
}


// Get a SIfunc to fill in.  If the named SIfunc alread exists, it is
// cleared, otherwise a new SIfunc is added to the table.
//
SIfunc *
SIinterp::NewSubfunc(const char *name)
{
    if (!name || !*name)
        return (0);
    SIfunc *s0 = 0;
    if (!siFuncTab)
        siFuncTab = new SymTab(false, false);
    else {
        s0 = (SIfunc*)SymTab::get(siFuncTab, name);
        if (s0 == (SIfunc*)ST_NIL)
            s0 = 0;
    }
    if (s0)
        s0->clear();
    else {
        s0 = new SIfunc(lstring::copy(name));
        siFuncTab->add(s0->sf_name, s0, false);
        SIparse()->ifShowFunctionList(0);
    }
    return (s0);
}


// Return a sorted list of the names of saved sub-functions.
//
stringlist *
SIinterp::GetSubfuncList()
{
    if (!siFuncTab)
        return (0);
    stringlist *s0 = 0;
    sLstr lstr;
    SymTabGen gen(siFuncTab);
    SymTabEnt *h;
    while ((h = gen.next()) != 0) {
        SIfunc *sf = (SIfunc*)h->stData;
        lstr.add(sf->sf_name ? sf->sf_name : "<unnamed>");
        lstr.add_c('(');
        for (SIarg *a = sf->sf_args; a; a = a->next) {
            lstr.add(a->var->name);
            if (a->next)
                lstr.add(", ");
        }
        lstr.add_c(')');
        s0 = new stringlist(lstr.string_trim(), s0);
        lstr.clear();
    }
    stringlist::sort(s0);
    return (s0);
}


namespace {
    struct sflist
    {
        sflist(SIfunc *s, sflist *n) { sf = s; next = n; }
        ~sflist() { delete sf; }

        SIfunc *sf;
        sflist *next;
    };
}


// Delete the named sub-function from the symbol table.
//
void
SIinterp::FreeSubfunc(const char *name)
{
    if (!siFuncTab)
        return;
    // If the SIfunc is still referenced, it is cleared and put in
    // the dead_list.  It will be freed when the refcnt is zero.
    static sflist *dead_list;
    sflist *sfp = 0, *sfn;
    for (sflist *sfl = dead_list; sfl; sfl = sfn) {
        sfn = sfl->next;
        if (sfl->sf->sf_refcnt == 0) {
            if (sfp)
                sfp->next = sfn;
            else
                dead_list = sfn;
            delete sfl;
            continue;
        }
        sfp = sfl;
    }

    if (!name || !*name)
        return;
    SIfunc *sf = (SIfunc*)SymTab::get(siFuncTab, name);
    if (!sf || sf == (SIfunc*)ST_NIL)
        return;
    siFuncTab->remove(name);

    if (sf->sf_refcnt == 0)
        delete sf;
    else {
        sf->clear();
        dead_list = new sflist(sf, dead_list);
    }
    SIparse()->ifShowFunctionList(0);
}


// Free and clear the sub-function table.
//
void
SIinterp::ClearSubfuncs()
{
    if (!siFuncTab)
        return;
    SymTabGen gen(siFuncTab, true);
    SymTabEnt *h;
    while ((h = gen.next()) != 0) {
        SIfunc *sf = (SIfunc*)h->stData;
        delete sf;
        delete h;
    }
    SIparse()->ifShowFunctionList(0);
}


// Print error message, including line number, and the detail message
// list.  The detail list is cleared.
//
void
SIinterp::LineError(const char *fmt, ...)
{
    siError = true;

    sLstr lstr;
    lstr.add("Line ");
    lstr.add_i(siExecLine > 0 ? siExecLine : siLineCount);
    lstr.add(": ");

    va_list args;
    char buf[1024];
    va_start(args, fmt);
    vsnprintf(buf, 1024, fmt, args);
    va_end(args);  

    lstr.add(buf);
    lstr.add(".\n");

    char *s = SIparse()->errMessage();
    if (s) {
        lstr.add(s);
        delete [] s;
    }
    CD()->ifInfoMessage(IFMSG_LOG_ERR, lstr.string());
    Halt();
}


//
// Remaining functions are private.
//

// Entry point to evaluate a statement block.
//
void
SIinterp::eval_stmt(siVariable *stmt_res, siVariable *ret, void *datap)
{
    if (!siStack)
        return;
    SIcontrol *cur = siStack->bcur;
    siRetinfo.type = CS_NORMAL;
    eval(stmt_res, ret, datap);

    switch (siRetinfo.type) {
    case CS_NORMAL:
        if (SIparse()->hasError())
            LineError("statement execution error");
        break;
    case CS_BROKEN:
        LineError("break not in loop or too many break levels given");
        break;
    case CS_CONTINUED:
        LineError(
            "continue not in loop or too many continue levels given");
        break;
    case CS_LABEL:
        cur = CurBlock()->findlabel(siRetinfo.rt.label);
        if (!cur)
            LineError("label %s not found", siRetinfo.rt.label);
        delete [] siRetinfo.rt.label;
        siRetinfo.rt.label = 0;
        break;
    case CS_RETURN:
        clear();
        return;
    }
    if (cur && !siStack) {
        SIcontrol *ncur = 0;
        if (cur->co_type == CO_BREAK) {
            while (cur->co_parent)
                cur = cur->co_parent;
            ncur = cur->co_next;
        }
        else if (cur->co_next)
            ncur = cur->co_next;
        else {
            while (cur->co_parent) {
                if (cur->co_parent->co_next) {
                    ncur = cur->co_parent->co_next;
                    break;
                }
                cur = cur->co_parent;
            }
        }
        push(ncur);
    }
}


// Evaluate the stack block element.
//
void
SIinterp::eval(siVariable *stmt_result, siVariable *ret_result, void *datap)
{
    sBlk *blk = siStack;
    siExecLine = blk->bcur->co_lineno;
//printf("line %d, type %d\n", blk->bcur->co_lineno, blk->bcur->co_type);

    if (blk->bcur->co_type == CO_UNFILLED) {
        // There was probably an error here...
        LineError("empty block, aborting");
        pop();
        postfunc();
    }
    else if (blk->bcur->co_type == CO_LABEL) {
        pop();
        postfunc();
    }
    else if (blk->bcur->co_type == CO_GOTO)
        eval_goto(blk->bcur->co_content.label);
    else if (blk->bcur->co_type == CO_STATEMENT) {
        if (blk->bcur->co_content.text) {
            siVariable res;
            if ((*blk->bcur->co_content.text->evfunc)
                    (blk->bcur->co_content.text, &res, datap)
                    != OK) {
                if (!IsHalted())
                    LineError("statement execution error");
            }
            if (stmt_result) {
                stmt_result->gc_result();
                *stmt_result = res;
                res.flags = 0;
            }
            else
                res.gc_result();
        }
        pop();
        postfunc();
    }
    else if (blk->bcur->co_type == CO_DELETE) {
        ParseNode *p = blk->bcur->co_content.text;
        if (p && p->type == PT_VAR) {
            siVariable *v = p->data.v;
            if (v)
                v->safe_delete();
        }
        else
            LineError("invalid or missing delete argument");
        pop();
        postfunc();
    }
    else if (blk->bcur->co_type == CO_REPEAT) {
        if (blk->bcount == 0) {
            // This is true only when entering the scope of the
            // repeat block.
            if (blk->bcur->co_content.text) {
                blk->bcount = -2;  // flag to indicate we've been here
                ParseNode *p = blk->bcur->co_content.text;
                siVariable res;
                if ((*p->evfunc)(p, &res, datap) != OK) {
                    if (!IsHalted())
                        LineError("repeat statement execution error");
                }
                else if (res.type == TYP_SCALAR && res.content.value >= 0.0)
                    blk->bcount =  1 + (int)res.content.value;
                else
                    LineError("repeat statement bad value");
            }
            else
                blk->bcount = -1;
        }
        if (blk->bcount > 1 || blk->bcount == -1) {
            if (blk->bcount != -1)
                blk->bcount--;
            blk->bchp = blk->bcur->co_children;
            push(blk->bchp);
        }
        else {
            pop();
            postfunc();
        }
    }
    else if (blk->bcur->co_type == CO_WHILE) {
        if (blk->bcur->co_content.text->istrue(datap)) {
            blk->bchp = blk->bcur->co_children;
            push(blk->bchp);
        }
        else {
            pop();
            postfunc();
        }
    }
    else if (blk->bcur->co_type == CO_DOWHILE) {
        if (!blk->bcount ||
                blk->bcur->co_content.text->istrue(datap)) {
            blk->bcount = 1;
            blk->bchp = blk->bcur->co_children;
            push(blk->bchp);
        }
        else {
            pop();
            postfunc();
        }
    }
    else if (blk->bcur->co_type == CO_IF) {
        bool found = false;
        for (SIifel *el = blk->bcur->co_content.ifcond; el; el = el->next) {
            if (el->text->istrue(datap)) {
                blk->bchp = el->children;
                found = true;
                break;
            }
        }
        if (!found)
            blk->bchp = blk->bcur->co_elseblock;
        if (blk->bchp)
            push(blk->bchp);
        else {
            pop();
            postfunc();
        }
    }
    else if (blk->bcur->co_type == CO_END) {
        pop();
        postfunc();
    }
    else if (blk->bcur->co_type == CO_BREAK)
        eval_break(blk->bcur->co_content.count);
    else if (blk->bcur->co_type == CO_CONTINUE)
        eval_continue(blk->bcur->co_content.count);
    else if (blk->bcur->co_type == CO_RETURN) {
        siRetinfo.type = CS_RETURN;
        if (ret_result) {
            ret_result->type = TYP_SCALAR;
            ret_result->content.value = 1.0;  // default return value
            if (blk->bcur->co_content.text) {
                ParseNode *p = blk->bcur->co_content.text;
                if ((*p->evfunc)(p, ret_result, datap) != OK) {
                    if (!IsHalted())
                        LineError("return statement execution error");
                }
            }
        }
    }
    else if (blk->bcur->co_type == CO_STATIC) {
        pop();
        postfunc();
    }
    else if (blk->bcur->co_type == CO_GLOBAL) {
        pop();
        postfunc();
    }
    else {
        LineError("bad block type %d", blk->bcur->co_type);
        pop();
        postfunc();
    }
}


// Function to evaluate "break n" line.
//
void
SIinterp::eval_break(int n)
{
    if (n == 0)
        n = 1;
    if (n < 0) {
        LineError("break %d, no operation", n);
        return;
    }
    pop();
    while (n) {
        if (!siStack) {
            siRetinfo.rt.num = n;
            siRetinfo.type = CS_BROKEN;
            break;
        }
        switch (siStack->bcur->co_type) {
        case CO_IF:
            pop();
            break;
        case CO_WHILE:
        case CO_DOWHILE:
        case CO_REPEAT:
            n--;
            pop();
            break;
        default:
            LineError("break, internal stack error");
        }
    }
    postfunc();
}


// Function to evaluate "continue n" line.
//
void
SIinterp::eval_continue(int n)
{
    if (n == 0)
        n = 1;
    if (n < 0) {
        LineError("continue %d, no operation", n);
        return;
    }
    do {
        pop();
        if (!siStack) {
            siRetinfo.rt.num = n;
            siRetinfo.type = CS_CONTINUED;
            break;
        }
        switch (siStack->bcur->co_type) {
        case CO_IF:
            break;
        case CO_WHILE:
        case CO_DOWHILE:
        case CO_REPEAT:
            n--;
            siStack->bchp = siStack->bcur->co_children;
            break;
        default:
            LineError("continue, internal stack error");
        }
    } while (n);
}


// Function to evaluate "goto label" line.
//
void
SIinterp::eval_goto(const char *ltext)
{
    pop();
    SIcontrol *targ = siStack->bcur->co_children->findlabel(ltext);
    while (!targ) {
        pop();
        if (!siStack) {
            siRetinfo.type = CS_LABEL;
            siRetinfo.rt.label = lstring::copy(ltext);
            return;
        }
        targ = siStack->bcur->co_children->findlabel(ltext);
    }
    siStack->bchp = targ->co_next;
    if (targ->co_next)
        push(targ->co_next);
}


// Function to advance context to next executable statement in a
// block.
//
void
SIinterp::postfunc()
{
    while (siStack) {
        if (siStack->bcur->co_type == CO_WHILE ||
                siStack->bcur->co_type == CO_DOWHILE ||
                siStack->bcur->co_type == CO_REPEAT) {
            siStack->bchp = siStack->bchp->co_next;
            push(siStack->bchp);
        }
        else if (siStack->bcur->co_type == CO_IF) {
            siStack->bchp = siStack->bchp->co_next;
            if (siStack->bchp)
                push(siStack->bchp);
            else {
                pop();
                continue;
            }
        }
        break;
    }
}


void
SIinterp::push_cx()
{
    siContext = new sCx(siContext);
    memset(&siMain, 0, sizeof(SIfunc));
    siLineCount = 0;
    siExecLine = -1;
    siLevel = 0;
    siStack = 0;
    siDefines = 0;

    SIparse()->setVariables(0);
    Init();
}


void
SIinterp::pop_cx()
{
    Clear();
    siMain = *siContext->sfunc();
    memset(siContext->sfunc(), 0, sizeof(SIfunc));
    siLineCount = siContext->line_cnt();
    siExecLine = siContext->ex_line();
    siLevel = siContext->level();
    siStack = siContext->stack();
    siDefines = siContext->defines();

    SIparse()->setVariables(siContext->variables());
    sCx *x = siContext;
    siContext = siContext->next();
    delete x;
}


// Macro keyword handler for Interpret.
//
void
SIinterp::handle_keyword(SIfile *sfp, stringlist **wl, const char **line,
    const char *s)
{
    if (lstring::cimatch("macro", s))
        SIparse()->ifMacroParse(sfp, wl, line, &siLineCount);
    else if (lstring::cimatch("define", s)) {
        lstring::advtok(&s);
        if (lstring::cimatch("eval", s)) {
            // '#define eval name value', value is evaluated as an
            // expression before macro parse.

            const char *t = s;
            lstring::advtok(&t);
            char *sm = siDefines->macro_expand(t);
            if (!sm) {
                LineError("expansion failed for #define eval");
                return;
            }
            t = sm;
            char *dname = lstring::getqtok(&t);
            if (!dname) {
                LineError("syntax error for #define eval");
                delete [] sm;
                return;
            }
            char *pbuf = new char[strlen(dname) + 100];
            char *p = lstring::stpcpy(pbuf, dname);
            *p++ = ' ';
            if (SIparse()->evaluate(t, p, 100 - (p-pbuf)) != OK)
                LineError("evaluation error for #define eval");
            else if (!siDefines->parse_macro(pbuf, false))
                LineError("post-eval parse error for #define eval");
            delete [] pbuf;
            delete [] dname;
            delete [] sm;
        }
        else {
            char *sm = siDefines->macro_expand(s);
            if (!sm) {
                LineError("expansion failed for #define");
                return;
            }
            if (!siDefines->parse_macro(sm, false))
                LineError("parse failed for #define");
            delete [] sm;
        }
    }
    else if (lstring::cimatch("if", s)) {
        while (*s && !isspace(*s))
            s++;
        siLevel++;
        double d = 0.0;
        char *sm = siDefines->macro_expand(s);
        if (!sm) {
            LineError("expansion failed for #if");
            return;
        }
        char obf[64];
        if (SIparse()->evaluate(sm, obf, 64) == OK) {
            if (sscanf(obf, "%lf", &d) != 1)
                // Non-null string, take as true.
                d = 1.0;
        }
        else
            LineError("#if evaluation failed");
        delete [] sm;
        if ((int)d == 0)
            skip_block(sfp, wl, line);
    }
    else if (lstring::cimatch("ifdef", s)) {
        while (*s && !isspace(*s))
            s++;
        char *t = lstring::gettok(&s);
        if (!t) {
            LineError("identifier missing in #ifdef");
            return;
        }
        siLevel++;
        if (!siDefines->find_text(t))
            skip_block(sfp, wl, line);
        delete [] t;
    }
    else if (lstring::cimatch("ifndef", s)) {
        while (*s && !isspace(*s))
            s++;
        char *t = lstring::gettok(&s);
        if (!t) {
            LineError("identifier missing in #ifndef");
            return;
        }
        siLevel++;
        if (siDefines->find_text(t))
            skip_block(sfp, wl, line);
        delete [] t;
    }
    else if (lstring::cimatch("else", s)) {
        if (siLevel)
            skip_block(sfp, wl, line);
    }
    else if (lstring::cimatch("endif", s)) {
        if (siLevel)
            siLevel--;
    }
}


// Skip lines until matching else or endif.
//
void
SIinterp::skip_block(SIfile *sfp, stringlist **wl, const char **line)
{
    int level = 1;
    char *nl;
    while ((nl = NextLine(&siLineCount, sfp, wl, line)) != 0) {
        char *s = nl;
        while (isspace(*s))
            s++;
        if (lstring::cimatch("#if", s) ||
                lstring::cimatch("#ifdef", s) ||
                lstring::cimatch("#ifndef", s))
            level++;
        else if (lstring::cimatch("#endif", s)) {
            level--;
            if (!level) {
                siLevel--;
                delete [] nl;
                break;
            }
        }
        else if (lstring::cimatch("#else", s)) {
            if (level == 1) {
                delete [] nl;
                break;
            }
        }
        delete [] nl;
    }
}


namespace {
    // Advance past "name[x,y,z]", with '=', ';', ',' or white space
    // terminating.  White space is allowed within the brackets.
    //
    bool
    get_past_name(const char **s)
    {
        bool sqbr = false;
        for ( ; **s; (*s)++) {
            if (**s == '[') {
                if (sqbr) {
                    SI()->LineError("syntax error, unexpected '['");
                    return (false);
                }
                sqbr = true;
            }
            else if (**s == ']') {
                if (!sqbr) {
                    SI()->LineError("syntax error, unexpected ']'");
                    return (false);
                }
                sqbr = false;
            }
            else if (isspace(**s) || **s == ',') {
                if (!sqbr)
                    break;
            }
            else if (**s == '=' || **s == ';') {
                if (sqbr) {
                    SI()->LineError("syntax error, missing ']'");
                    return (false);
                }
                break;
            }
        }
        return (true);
    }


    // Tokenizer for a list of variable names or assignments.  The names
    // or assignments can be space or comma separated, and values can be
    // double quoted (quotes are retained).  A semicolon (';') or end
    // of line terminates the list.
    //
    bool
    get_decl(const char **s, char **lhs, char **rhs, bool *term)
    {
        *lhs = 0;
        *rhs = 0;
        if (s == 0 || *s == 0 || *term)
            return (false);

        // Advance past leading junk, look for semicolon.
        while (isspace(**s) || **s == ',')
            (*s)++;
        if (!**s || **s == ';')
            return (false);

        // Grab the variable name, including dimensions in square brackets.
        // White space is allowed within the brackets (it will be stripped),
        // but not elsewhere.
        const char *st = *s;
        if (!get_past_name(s))
            return (false);
        char *cbuf = new char[*s - st + 1];
        char *c = cbuf;
        while (st < *s) {
            char ch = *st++;
            if (!isspace(ch))
                *c++ = ch;
        }
        *c = 0;
        *lhs = cbuf;

        // Advance to next token, or '=' or ';'.
        while (isspace(**s) || **s == ',')
            (*s)++;
        if (**s != '=') {
            if (**s == ';')
                *term = true;
            return (true);
        }
        (*s)++;

        // Grab the value, which can be double-quoted (quotes are retained).
        // Allow square-bracket indices.
        while (isspace(**s) || **s == ',')
            (*s)++;
        if (!**s)
            return (true);
        if (**s == ';') {
            *term = true;
            return (true);
        }
        bool strip_space = false;
        st = *s;
        if (**s == '"') {
            (*s)++;
            while (**s && **s != '"')
                (*s)++;
            if (**s == '"')
                (*s)++;
        }
        else {
            if (!get_past_name(s)) {
                delete [] *lhs;
                *lhs = 0;
                return (false);
            }
            strip_space = true;
        }
        cbuf = new char[*s - st + 1];
        c = cbuf;
        while (st < *s) {
            char ch = *st++;
            if (!strip_space || !isspace(ch))
                *c++ = ch;
        }
        *c = 0;
        *rhs = cbuf;

        while (isspace(**s) || **s == ',')
            (*s)++;
        if (**s == ';')
            *term = true;
        return (true);
    }


#define STOPCHARS ";:(+-*/=!~&|#^"

    char *
    token(const char **s)
    {
        if (s == 0 || *s == 0)
            return (0);
        while (isspace(**s))
            (*s)++;
        if (!**s)
            return (0);
        const char *st = *s;
        while (**s && !isspace(**s) && !strchr(STOPCHARS, **s))
            (*s)++;
        char *cbuf = new char[*s - st + 1];
        char *c = cbuf;
        while (st < *s)
            *c++ = *st++;
        *c = 0;
        while (isspace(**s))
            (*s)++;
        return (cbuf);
    }


    // Grab an argument token.  It is assumed that ')' or ';' terminate the
    // list, and *term is set true if seen.
    // WARNING: this can return empty strings.
    //
    char *
    argtok(const char **s, bool *term)
    {
        if (s == 0 || *s == 0 || *term)
            return (0);
        while (isspace(**s))
            (*s)++;
        if (!**s)
            return (0);
        const char *st = *s;
        while (**s && !isspace(**s) && **s != '(' && **s != ',' &&
                **s != ')' && **s != ';')
            (*s)++;
        char *cbuf = new char[*s - st + 1];
        char *c = cbuf;
        while (st < *s)
            *c++ = *st++;
        *c = 0;
        while (isspace(**s) || **s == '(' || **s == ',' ||
                **s == ')' || **s == ';') {
            if (**s == ')' || **s == ';') {
                *term = true;
                if (**s == ';')
                    break;
            }
            (*s)++;
        }
        return (cbuf);
    }
}


// Add line to the control structure list. If siCurFunc->co_type
// is CO_UNFILLED, the last line was the beginning of a
// block, and this is the unfilled first statement.
//
void
SIinterp::set_block(const char **line, int lineno)
{
    while (*line && !IsHalted()) {
        int word = gettokval(line);
        if (word == CO_UNFILLED)
            return;

        SIcontrol *cur;
        if (!siCurFunc || !siCurFunc->sf_end) {
            siMain.sf_text = cur = new SIcontrol(0);
            siMain.sf_end = cur;
            siCurFunc = &siMain;
        }
        else {
            cur = siCurFunc->sf_end;
            if (cur->co_type != CO_UNFILLED &&
                    (word != CO_FUNCTION && word != CO_ENDFUNC)) {
                cur->co_next = new SIcontrol(cur->co_parent);
                cur->co_next->co_prev = cur;
                cur = cur->co_next;
            }
        }

        if (word == CO_LABEL) {
            cur->co_type = CO_LABEL;
            cur->co_lineno = lineno;
            cur->co_content.label = token(line);
            if (!cur->co_content.label)
                LineError("missing label");
        }
        else if (word == CO_GOTO) {
            cur->co_type = CO_GOTO;
            cur->co_lineno = lineno;
            cur->co_content.label = token(line);
            if (!cur->co_content.label)
                LineError("missing label");
        }
        else if (word == CO_STATEMENT) {
            cur->co_content.text = SIparse()->getTree(line, false);
            cur->co_lineno = lineno;
            if (!cur->co_content.text)
                LineError("statement parse returned nothing");
            else
                cur->co_type = CO_STATEMENT;
        }
        else if (word == CO_DELETE) {
            cur->co_type = CO_DELETE;
            cur->co_lineno = lineno;
            cur->co_content.text = SIparse()->getTree(line, false);
            if (!cur->co_content.text)
                LineError("missing variable name");
            else if (cur->co_content.text->type != PT_VAR)
                LineError("incorrect variable name");
        }
        else if (word == CO_REPEAT) {
            cur->co_type = CO_REPEAT;
            cur->co_lineno = lineno;
            cur->co_content.text = SIparse()->getTree(line, false);
            if (!cur->co_content.text)
                LineError("missing repeat initialization expression");
            cur->co_children = new SIcontrol(cur);
            cur = cur->co_children;
        }
        else if (word == CO_WHILE) {
            cur->co_type = CO_WHILE;
            cur->co_lineno = lineno;
            cur->co_content.text = SIparse()->getTree(line, false);
            if (!cur->co_content.text)
                LineError("missing while condition");
            cur->co_children = new SIcontrol(cur);
            cur = cur->co_children;
        }
        else if (word == CO_DOWHILE) {
            cur->co_type = CO_DOWHILE;
            cur->co_lineno = lineno;
            cur->co_content.text = SIparse()->getTree(line, false);
            if (!cur->co_content.text)
                LineError("missing dowhile condition");
            cur->co_children = new SIcontrol(cur);
            cur = cur->co_children;
        }
        else if (word == CO_IF) {
            cur->co_type = CO_IF;
            cur->co_lineno = lineno;
            SIifel *el = new SIifel(cur);
            cur->co_content.ifcond = el;
            el->text = SIparse()->getTree(line, false);
            if (!el->text)
                LineError("missing if condition");
            el->children = new SIcontrol(cur);
            cur = el->children;
        }
        else if (word == CO_ELIF) {
            if (!cur->co_parent || (cur->co_parent->co_type != CO_IF) ||
                    !cur->co_parent->co_content.ifcond) {
                LineError("misplaced elif");
                cur->co_type = CO_UNFILLED;
            }
            else {
                SIifel *pel = cur->co_parent->co_content.ifcond;
                while (pel->next)
                    pel = pel->next;
                if (cur->co_prev) {
                    cur->co_prev->co_next = 0;
                    cur->co_prev = 0;
                }
                else
                    pel->children = 0;
                SIifel *el = new SIifel(cur->co_parent);
                pel->next = el;
                el->text = SIparse()->getTree(line, false);
                if (!el->text)
                    LineError("missing elif condition");
                el->children = new SIcontrol(cur->co_parent);
                cur = el->children;
            }
        }
        else if (word == CO_ELSE) {
            if (!cur->co_parent || (cur->co_parent->co_type != CO_IF) ||
                    !cur->co_parent->co_content.ifcond) {
                LineError("misplaced else");
                cur->co_type = CO_UNFILLED;
            }
            else {
                if (cur->co_prev) {
                    cur->co_prev->co_next = 0;
                    cur->co_prev = 0;
                }
                else {
                    SIifel *pel = cur->co_parent->co_content.ifcond;
                    while (pel->next)
                        pel = pel->next;
                    pel->children = 0;
                }
                cur->co_parent->co_elseblock = cur;
            }
        }
        else if (word == CO_END) {
            // Throw away this thing
            if (!cur->co_parent) {
                LineError("no block to end");
                cur->co_type = CO_UNFILLED;
            }
            else if (cur->co_prev) {
                SIcontrol *x = cur;
                cur->co_prev->co_next = 0;
                cur = cur->co_parent;
                delete x;
            }
            else {
                SIcontrol *x = cur;
                cur = cur->co_parent;
                // empty block
                if (x == cur->co_children)
                    cur->co_children = 0;
                else if (x == cur->co_elseblock)
                    cur->co_elseblock = 0;
                else if (cur->co_type == CO_IF) {
                    SIifel *pel = cur->co_content.ifcond;
                    if (pel) {
                        while (pel->next)
                            pel = pel->next;
                        if (x == pel->children)
                            pel->children = 0;
                    }
                }
                delete x;
            }
        }
        else if (word == CO_BREAK) {
            cur->co_type = CO_BREAK;
            cur->co_lineno = lineno;
            cur->co_content.count = 1;
            const char *tmp = *line;
            bool err;
            int cnt = (int)SIparse()->numberParse(line, &err);
            if (!err) {
                if (cnt <= 0)
                    LineError("break argument value not positive");
                else
                    cur->co_content.count = cnt;
            }
            else
                *line = tmp;
        }
        else if (word == CO_CONTINUE) {
            cur->co_type = CO_CONTINUE;
            cur->co_lineno = lineno;
            cur->co_content.count = 1;
            const char *tmp = *line;
            bool err;
            int cnt = (int)SIparse()->numberParse(line, &err);
            if (!err) {
                if (cnt <= 0)
                    LineError("continue argument value not positive");
                else
                    cur->co_content.count = cnt;
            }
            else
                *line = tmp;
        }
        else if (word == CO_RETURN) {
            cur->co_type = CO_RETURN;
            cur->co_lineno = lineno;
            cur->co_content.text = SIparse()->getTree(line, true);
        }
        else if (word == CO_FUNCTION) {
            if (siCurFunc != &siMain) {
                LineError("endfunc keyword missing");
                siCurFunc->sf_variables = SIparse()->getVariables();
                siCurFunc = &siMain;
                cur = siCurFunc->sf_end;
                SIparse()->setVariables(siCurFunc->sf_variables);
            }
            bool term = false;
            char *fname = argtok(line, &term);
            if (!fname)
                LineError("function name missing");
            else if (!*fname) {
                delete [] fname;
                LineError("function name missing");
            }
            else {
                SIfunc *sf = NewSubfunc(fname);
                delete [] fname;
                sf->parse_args(line, &term);
                siCurFunc->sf_variables = SIparse()->getVariables();
                siCurFunc = sf;
                cur = siCurFunc->sf_end;
                SIparse()->setVariables(siCurFunc->sf_variables);
            }
        }
        else if (word == CO_ENDFUNC) {
            if (cur->co_parent)
                LineError("block imbalance, end statement missing");
            if (siCurFunc == &siMain)
                LineError("extra \"endfunc\", not in function scope");
            else {
                siCurFunc->sf_variables = SIparse()->getVariables();
                siCurFunc->init_local_vars();
                siCurFunc = &siMain;
                cur = siCurFunc->sf_end;
                SIparse()->setVariables(siCurFunc->sf_variables);
                if (cur->co_type == CO_UNFILLED && cur == siCurFunc->sf_text) {
                    siCurFunc->sf_text = siCurFunc->sf_end = 0;
                    delete cur;
                    cur = 0;
                }
            }
        }
        else if (word == CO_STATIC) {
            char *lhs, *rhs;
            const char *lstart = *line;
            bool term = false;
            SIlexprCx cx;
            while (get_decl(line, &lhs, &rhs, &term)) {
                char *str = assign_string(lhs, rhs);
                char *b = strchr(lhs, '[');
                if (b)
                    *b = 0;
                const char *s = str;
                set_block(&s, lineno);
                delete [] str;
                push(siCurFunc->sf_text);
                // The cx is not used, can probably safely pass null.
                while (siStack)
                    eval_stmt(0, 0, &cx);
                SIcontrol::destroy(siCurFunc->sf_text);
                siCurFunc->sf_text = new SIcontrol(0);
                siCurFunc->sf_end = siCurFunc->sf_text;
                cur = siCurFunc->sf_end;
                siExecLine = 0;
                Variable *v = SIparse()->findVariable(lhs);
                if (v) {
                    if (v->type == TYP_STRING) {
                        v->content.string = lstring::copy(v->content.string);
                        v->flags |= VF_ORIGINAL;
                    }
                    v->flags |= VF_STATIC;
                }
                delete [] lhs;
                delete [] rhs;
            }
            cur->co_type = CO_STATIC;
            cur->co_lineno = lineno;
            cur->co_content.label = new char[*line - lstart + 1];
            strncpy(cur->co_content.label, lstart, *line - lstart);
            cur->co_content.label[*line - lstart] = 0;
        }
        else if (word == CO_GLOBAL) {
            char *lhs, *rhs;
            const char *lstart = *line;
            bool term = false;
            SIlexprCx cx;
            while (get_decl(line, &lhs, &rhs, &term)) {
                if (rhs && siCurFunc != &siMain) {
                    delete [] rhs;
                    rhs = 0;
                    LineError(
                        "globals can't be initialized in functions");
                }
                char *str = assign_string(lhs, rhs);
                char *b = strchr(lhs, '[');
                if (b)
                    *b = 0;
                const char *s = str;
                set_block(&s, lineno);
                delete [] str;
                push(siCurFunc->sf_text);
                // The cx is not used, can probably safely pass null.
                while (siStack)
                    eval_stmt(0, 0, &cx);
                SIcontrol::destroy(siCurFunc->sf_text);
                siCurFunc->sf_text = new SIcontrol(0);
                siCurFunc->sf_end = siCurFunc->sf_text;
                cur = siCurFunc->sf_end;
                siExecLine = 0;
                Variable *v = SIparse()->findVariable(lhs);
                if (v) {
                    if (v->type == TYP_STRING) {
                        v->content.string = lstring::copy(v->content.string);
                        v->flags |= VF_ORIGINAL;
                    }
                    v->flags |= VF_GLOBAL;
                    if (SIparse()->registerGlobal(v) == BAD)
                        LineError("global variable already initialized");
                }
                delete [] lhs;
                delete [] rhs;
            }
            cur->co_type = CO_GLOBAL;
            cur->co_lineno = lineno;
            cur->co_content.label = new char[*line - lstart + 1];
            strncpy(cur->co_content.label, lstart, *line - lstart);
            cur->co_content.label[*line - lstart] = 0;
        }
        else if (word == CO_EXEC) {
            ParseNode *pn = SIparse()->getTree(line, false);
            if (pn) {
                // Evaluating a constant expression at "compile" time.
                SIlexprCx cx;
                siVariable r;
                if ((*pn->evfunc)(pn, &r, &cx) != OK)
                    LineError("exec failed");
                ParseNode::destroy(pn);
            }
            else
                LineError("parse failed");
        }

        siCurFunc->sf_end = cur;
        if (**line == ';') {
            while (**line == ';' || isspace(**line))
                (*line)++;
        }
    }
}


namespace {
    // Hash table for script keywords.
    SymTab *token_tab;

    void
    tt_init()
    {
        token_tab = new SymTab(false, false);
        token_tab->add( "label",    (void*)CO_LABEL,        false);
        token_tab->add( "goto",     (void*)CO_GOTO,         false);
        token_tab->add( "delete",   (void*)CO_DELETE,       false);
        token_tab->add( "repeat",   (void*)CO_REPEAT,       false);
        token_tab->add( "while",    (void*)CO_WHILE,        false);
        token_tab->add( "dowhile",  (void*)CO_DOWHILE,      false);
        token_tab->add( "if",       (void*)CO_IF,           false);
        token_tab->add( "elif",     (void*)CO_ELIF,         false);
        token_tab->add( "else",     (void*)CO_ELSE,         false);
        token_tab->add( "end",      (void*)CO_END,          false);
        token_tab->add( "break",    (void*)CO_BREAK,        false);
        token_tab->add( "continue", (void*)CO_CONTINUE,     false);
        token_tab->add( "return",   (void*)CO_RETURN,       false);
        token_tab->add( "function", (void*)CO_FUNCTION,     false);
        token_tab->add( "endfunc",  (void*)CO_ENDFUNC,      false);
        token_tab->add( "static",   (void*)CO_STATIC,       false);
        token_tab->add( "global",   (void*)CO_GLOBAL,       false);
        token_tab->add( "exec",     (void*)CO_EXEC,         false);
    }
}


int
SIinterp::gettokval(const char **line)
{
    if (!line || !*line)
        return (CO_UNFILLED);
    const char *oline = *line;
    char *tok = token(line);
    if (!tok || !*tok) {
        // A line beginning with any of the token STOPCHARS is taken as
        // a comment.
        delete [] tok;
        return (CO_UNFILLED);
    }
    if (!token_tab)
        tt_init();
    long val = (long)SymTab::get(token_tab, tok);
    delete [] tok;
    if (val != (long)ST_NIL)
        return ((int)val);
    *line = oline;
    return (CO_STATEMENT);
}
// End of SIinterp functions


SIfunc::~SIfunc()
{
    delete [] sf_name;
    SIarg::destroy(sf_args);
    siVariable::destroy(sf_variables);
    SIcontrol::destroy(sf_text);
    delete sf_exprs;
    while (sf_varinit) {
        siVariable *v = (siVariable*)sf_varinit->next;
        delete sf_varinit;  // doesn't touch contents
        sf_varinit = v;
    }
}


void
SIfunc::clear()
{
    SIarg::destroy(sf_args);
    sf_args = 0;
    siVariable::destroy(sf_variables);
    sf_variables = 0;
    SIcontrol::destroy(sf_text);
    sf_text = new SIcontrol(0);
    sf_end = sf_text;
    delete sf_exprs;
    sf_exprs = 0;
    while (sf_varinit) {
        siVariable *v = (siVariable*)sf_varinit->next;
        delete sf_varinit;  // doesn't touch contents
        sf_varinit = v;
    }
}


// Parse the arguments of the function call, setting up the internal
// structures.
//
void
SIfunc::parse_args(const char **line, bool *term)
{
    siVariable *v0 = 0, *v = 0;
    SIarg *a0 = 0, *a = 0;
    char *tok;
    while ((tok = argtok(line, term)) != 0) {
        if (!v0) {
            v0 = v = new siVariable;
            a0 = a = new SIarg(v);
        }
        else {
            v->next = new siVariable;
            v = (siVariable*)v->next;
            a->next = new SIarg(v);
            a = a->next;
        }
        v->name = tok;
    }
    sf_args = a0;
    sf_variables = v0;
}


// Set the initial values for local variables.  The sf_varinit is a
// copy of the variable list before the function is ever called, used
// for initialization.  This should be called before the function is
// used.
//
void
SIfunc::init_local_vars()
{
    if (!sf_varinit) {
        siVariable *v0 = 0, *ve = 0;
        for (siVariable *v = sf_variables; v; v = (siVariable*)v->next) {
            if (v->flags & (VF_STATIC | VF_GLOBAL))
                continue;
            if (!v0)
                v0 = ve = new siVariable;
            else {
                ve->next = new siVariable;
                ve = (siVariable*)ve->next;
            }
            if (v->type == TYP_ARRAY) {
                // don't need this, allocated later
                v->content.a->allocate(0);
            }
            ve->set(v);
        }
        sf_varinit = v0;
    }
}


// Save the present content of the variables, and initialize from
// sf_varinit.  Call this before each execution.
//
bool
SIfunc::save_vars(siVariable **pv)
{
    *pv = 0;
    siVariable *v0 = 0, *ve = 0;
    siVariable *vi = sf_varinit;
    for (siVariable *v = sf_variables; v; v = (siVariable*)v->next) {
        if (v->flags & (VF_STATIC | VF_GLOBAL))
            continue;
        if (!v0)
            v0 = ve = new siVariable;
        else {
            ve->next = new siVariable;
            ve = (siVariable*)ve->next;
        }
        ve->set(v);

        if (!vi) {
            // this "can't happen"
            SI()->LineError("save_vars: strange internal error");
            restore_vars(v0, 0);
            return (false);
        }
        v->set(vi);
        if (v->type == TYP_STRING) {
            // This is a string constant, copy the text so that if
            // the string is changed, the changes don't persist
            v->content.string = lstring::copy(v->content.string);
            v->flags |= VF_ORIGINAL;
        }
        else if (v->type == TYP_ARRAY) {
            // Allocate storage for local array
            siAryData *a = new siAryData;
            if (a->init(v->content.a->dims()) == BAD) {
                SI()->LineError("out of memory");
                restore_vars(v0, 0);
                delete a;
                return (false);
            }
            v->content.a = a;
            v->flags |= VF_ORIGINAL;
        }
        vi = (siVariable*)vi->next;
    }
    *pv = v0;
    return (true);
}


// Delete any new content, and reset variables to prior values from
// the values saved with save_vars().  Call this upon return from
// execution.
//
bool
SIfunc::restore_vars(Variable *vi, Variable *res)
{
    bool retval = true;
    if (res) {
        if (res->type == TYP_STRING) {
            res->content.string = lstring::copy(res->content.string);
            res->flags |= VF_ORIGINAL;
        }
        if (res->type == TYP_ARRAY) {
            // An array return can point to one of the arguments, or
            // to a static array variable
            bool found = false;
            for (SIarg *p = sf_args; p; p = p->next) {
                if (p->var->type != TYP_ARRAY)
                    continue;
                int len = p->var->content.a->length();
                if (p->var->content.a->values() <= res->content.a->values() &&
                        res->content.a->values() <
                        p->var->content.a->values() + len)
                    // ok, points to argument
                    found = true;
            }
            if (!found) {
                for (Variable *v = sf_variables; v; v = v->next) {
                    if (!(v->flags & (VF_STATIC | VF_GLOBAL)) ||
                            v->type != TYP_ARRAY)
                        continue;
                    int len = v->content.a->length();
                    if (v->content.a->values() <= res->content.a->values() &&
                            res->content.a->values() <
                            v->content.a->values() + len)
                        // ok, points to statis/global var
                        found = true;
                }
            }
            if (!found) {
                SI()->LineError("can't return pointer to local array");
                retval = false;
            }
        }
    }

    for (Variable *v = sf_variables; v; v = v->next) {
        if (v->flags & (VF_STATIC | VF_GLOBAL))
            continue;
        if (!vi)
            break;

        if (v->type == TYP_STRING) {
            if (v->flags & VF_ORIGINAL)
                delete [] v->content.string;
        }
        if (v->type == TYP_ARRAY) {
            if (v->flags & VF_ORIGINAL)
                delete v->content.a;
        }
        if (v->type == TYP_ZLIST) {
            bool found = false;
            if (res && res->type == TYP_ZLIST &&
                    res->content.zlist == v->content.zlist)
                found = true;
            if (!found) {
                for (SIarg *p = sf_args; p; p = p->next) {
                    if (p->var->type != TYP_ZLIST)
                        continue;
                    if (p->var->content.zlist == v->content.zlist) {
                        found = true;
                        break;
                    }
                }
            }
            if (!found)
                // Found a zlist not passed or returned, delete it.
                Zlist::destroy(v->content.zlist);
        }
        if (v->type == TYP_LEXPR) {
            bool found = false;
            if (res && res->type == TYP_LEXPR &&
                    res->content.lspec == v->content.lspec)
                found = true;
            if (!found) {
                for (SIarg *p = sf_args; p; p = p->next) {
                    if (p->var->type != TYP_LEXPR)
                        continue;
                    if (p->var->content.lspec == v->content.lspec) {
                        found = true;
                        break;
                    }
                }
            }
            if (!found)
                // Found a layer expr not passed or returned, delete it.
                delete v->content.lspec;
        }
        if (v->type == TYP_HANDLE) {
            bool found = false;
            if (res && res->type == TYP_HANDLE &&
                    res->content.value == v->content.value)
                found = true;
            if (!found) {
                for (SIarg *p = sf_args; p; p = p->next) {
                    if (p->var->type != TYP_HANDLE)
                        continue;
                    if (p->var->content.value == v->content.value) {
                        found = true;
                        break;
                    }
                }
            }
            if (!found) {
                // Found a handle not passed or returned, make sure
                // it is closed
                int id = (int)v->content.value;
                sHdl *hdl = sHdl::get(id);
                if (hdl)
                    hdl->close(id);
            }
        }

        v->set(vi);
        Variable *vn = vi->next;
        delete vi;  // doesn't touch content
        vi = vn;
    }
    return (retval);
}
// End of SIfunc functions;


// Return the control struct matching the label.
//
SIcontrol *
SIcontrol::findlabel(const char *s)
{
    for (SIcontrol *ct = this; ct; ct = ct->co_next)
        if ((ct->co_type == CO_LABEL) && !strcmp(s, ct->co_content.label))
            return (ct);
    return (0);
}


// Print a control block.
//
#define tabf(num)    for (i = 0; i < num; i++) putc(' ', fp)
#define INDNT 4
//
void
SIcontrol::print(int indent, FILE *fp)
{
    int i;
    SIcontrol *tc;
    switch (co_type) {
    case CO_UNFILLED:
        tabf(indent);
        fprintf(fp, "(unfilled)\n");
        break;
    case CO_LABEL:
        tabf(indent);
        fprintf(fp, "label %s\n", co_content.label);
        break;
    case CO_GOTO:
        tabf(indent);
        fprintf(fp, "goto %s\n", co_content.label);
        break;
    case CO_STATEMENT:
        tabf(indent);
        co_content.text->print(fp);
        putc('\n', fp);
        break;
    case CO_DELETE:
        tabf(indent);
        fprintf(fp, "delete ");
        co_content.text->print(fp);
        putc('\n', fp);
        break;
    case CO_REPEAT:
        tabf(indent);
        fprintf(fp, "repeat ");
        if (co_content.text)
            co_content.text->print(fp);
        else
            putc('\n', fp);
        indent += INDNT;
        for (tc = co_children; tc; tc = tc->co_next)
            tc->print(indent, fp);
        indent -= INDNT;
        tabf(indent);
        fprintf(fp, "end\n");
        break;
    case CO_WHILE:
        tabf(indent);
        fprintf(fp, "while ");
        co_content.text->print(fp);
        putc('\n', fp);
        indent += INDNT;
        for (tc = co_children; tc; tc = tc->co_next)
            tc->print(indent, fp);
        indent -= INDNT;
        tabf(indent);
        fprintf(fp, "end\n");
        break;
    case CO_DOWHILE:
        tabf(indent);
        fprintf(fp, "dowhile ");
        co_content.text->print(fp);
        putc('\n', fp);
        indent += INDNT;
        for (tc = co_children; tc; tc = tc->co_next)
            tc->print(indent, fp);
        indent -= INDNT;
        tabf(indent);
        fprintf(fp, "end\n");
        break;
    case CO_IF:
        for (SIifel *el = co_content.ifcond; el; el = el->next) {
            tabf(indent);
            if (el == co_content.ifcond)
                fprintf(fp, "if ");
            else
                fprintf(fp, "elif ");
            el->text->print(fp);
            putc('\n', fp);
            indent += INDNT;
            for (tc = el->children; tc; tc = tc->co_next)
                tc->print(indent, fp);
            indent -= INDNT;
        }
        if (co_elseblock) {
            tabf(indent);
            fprintf(fp, "else\n");
            indent += INDNT;
            for (tc = co_elseblock; tc; tc = tc->co_next)
                tc->print(indent, fp);
            indent -= INDNT;
        }
        tabf(indent);
        fprintf(fp, "end\n");
        break;
    case CO_BREAK:
        tabf(indent);
        if (co_content.count != 1)
            fprintf(fp, "break %d\n", co_content.count);
        else
            fprintf(fp, "break\n");
        break;
    case CO_CONTINUE:
        tabf(indent);
        if (co_content.count != 1)
            fprintf(fp, "continue %d\n", co_content.count);
        else
            fprintf(fp, "continue\n");
        break;
    case CO_RETURN:
        tabf(indent);
        fprintf(fp, "return ");
        if (co_content.text)
            co_content.text->print(fp);
        else
            putc('\n', fp);
        break;
    case CO_STATIC:
        tabf(indent);
        fputs(co_content.label, fp);
        putc('\n', fp);
        break;
    case CO_GLOBAL:
        tabf(indent);
        fputs(co_content.label, fp);
        putc('\n', fp);
        break;
    default:
        tabf(indent);
        fprintf(fp, "bad type %d\n", co_type);
        break;
    }
}


// Static function.
// Free the control structure.
//
void
SIcontrol::destroy(SIcontrol *thiscc)
{
    SIcontrol *cc, *cd;
    for (cc = thiscc; cc; cc = cd) {
        switch (cc->co_type) {
        case CO_LABEL:
        case CO_GOTO:
        case CO_STATIC:
        case CO_GLOBAL:
            delete [] cc->co_content.label;
            break;
        case CO_STATEMENT:
        case CO_DELETE:
        case CO_REPEAT:
        case CO_WHILE:
        case CO_DOWHILE:
        case CO_RETURN:
            ParseNode::destroy(cc->co_content.text);
            break;
        case CO_IF:
            SIifel::destroy(cc->co_content.ifcond);
            break;
        default:
            break;
        }
        SIcontrol(cc->co_children);
        SIcontrol(cc->co_elseblock);
        cd = cc->co_next;
        delete cc;
    }
}
// End of SIcontrol functions


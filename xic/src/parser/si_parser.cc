
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
#include "si_parsenode.h"
#include "si_parser.h"
#include "si_lexpr.h"
#include "si_interp.h"

// If defined, use SPICE-style numbers, i.e., recognize multiplier
// suffix 1K = 1e3, etc.
#define SP_NUMBER

#ifdef SP_NUMBER
#include "spnumber.h"
#endif


#ifndef M_PI
#define	M_PI        3.14159265358979323846  // pi
#define M_PI_2      1.57079632679489661923  // pi/2
#define M_PI_4      0.78539816339744830962  // pi/4
#define	M_E         2.7182818284590452354   // e
#define M_SQRT2     1.41421356237309504880
#endif

// Preassigned constants.
//
PTconstant SIparser::spConstants[] = {
    { "e",          M_E },
    { "pi",         M_PI },
    { "PI",         M_PI },
    { "PI_2",       M_PI_2 },
    { "PI_4",       M_PI_4 },
    { "NULL",       0.0 },
    { "INFINITY",   1e6 },  // 1 meter
    { "TRUE",       1.0 },
    { "FALSE",      0.0 },
    { "EOF",        -1.0},
    { "CHARGE",     1.60217646e-19 },
    { "CTOK",       273.15 },
    { "BOLTZ",      1.3806226e-23 },
    { "ROOT2",      M_SQRT2 },
    { "PHI0",       2.0679e-15 },
    { 0,            0.0 }
};

SIparser *SIparser::instancePtr;

SIparser::SIparser()
{
    if (instancePtr) {
        fprintf(stderr, "Singleton class SIparser already instantiated.\n");
        exit(1);
    }
    instancePtr = this;

    spErrMsgs = 0;
    spGlobals = 0;
    spVariables = 0;
    spVariablesPreset = 0;
    spLexpVars = 0;

    spConstTab = 0;
    spIFfuncs = 0;
    spALTfuncs = 0;
    spSuppressErrInitFunc = 0;

    spExprs = 0;

    spTriNest = 0;
    spLastToken = 0;
    spLastType = ElemNumber;
    spUseAltFuncs = false;
    spGlobalOnly = false;

    funcs_lexpr_init();
    funcs_math_init();
}


// Private static error exit.
//
void
SIparser::on_null_ptr()
{
    fprintf(stderr, "Singleton class SIinterp used before instantiated.\n");
    exit(1);
}


// This is a hack to change the predefined constant INFINITY to the
// correct value after a database resolution change.
//
void
SIparser::updateInfinity()
{
    for (int i = 0; spConstants[i].name; i++) {
        if (!strcmp(spConstants[i].name, "INFINITY")) {
            spConstants[i].value = CDinfinity/CDphysResolution;
            return;
        }
    }
}


// Add or set the global variable.  A variable with the same name as
// the global variable is kept in the local context, which has the
// VF_GLOBAL flag set.  The local copy is used only for indirection.
//
bool
SIparser::registerGlobal(Variable *v0)
{
    siVariable *v = findGlobal(v0->name);
    if (v) {
        if (v0->type == TYP_NOTYPE)
            return (OK);
        if (v->type != TYP_NOTYPE)
            return (BAD);  // already set, not good
        v->type = v0->type;
        v->content = v0->content;
        if (v0->flags & VF_ORIGINAL) {
            v->flags |= VF_ORIGINAL;
            v0->flags &= ~VF_ORIGINAL;
        }
        v0->type = TYP_NOTYPE;
        v0->content.string = v0->name;
        return (OK);
    }

    v = new siVariable;
    v->name = lstring::copy(v0->name);
    v->type = v0->type;
    if (v0->type == TYP_NOTYPE)
        v->content.string = v->name;
    else {
        v->content = v0->content;
        if (v0->flags & VF_ORIGINAL) {
            v->flags |= VF_ORIGINAL;
            v0->flags &= ~VF_ORIGINAL;
        }
    }
    v0->type = TYP_NOTYPE;
    v0->content.string = v0->name;
    addGlobal(v);
    return (OK);
}


// Register an internal function.
//
void
SIparser::registerFunc(const char *name, int numargs, SIscriptFunc func)
{
    if (!name || !func)
        return;
    if (!spIFfuncs)
        spIFfuncs = new table_t<SIptfunc>;

    SIptfunc *ptf = spIFfuncs->find(name);
    if (ptf) {
        fprintf(stderr,
            "SIparser::registerFunc: Duplicate function name %s.\n", name);
        return;
    }

    // The element does not copy name, which is assumed to be a constant
    // string.
    ptf = new SIptfunc(name, numargs, 0, func);
    spIFfuncs->link(ptf, false);
    spIFfuncs = spIFfuncs->check_rehash();
}


// Remove the definition from the table.
//
void
SIparser::unRegisterFunc(const char *name)
{
    if (!name || !spIFfuncs)
        return;
    SIptfunc *ptf = spIFfuncs->find(name);
    if (ptf) {
        spIFfuncs->unlink(ptf);
        delete ptf;
    }
}


// Register an alternate (layer expression) function.
//
void
SIparser::registerAltFunc(const char *name, int numargs, unsigned int saflgs,
    SIscriptFunc func)
{
    if (!spALTfuncs)
        spALTfuncs = new table_t<SIptfunc>;

    SIptfunc *ptf = spALTfuncs->find(name);
    if (ptf) {
        fprintf(stderr,
            "SIparser::registerAltFunc: Duplicate function name %s.\n", name);
        return;
    }

    // The element does not copy name, which is assumed to be a constant
    // string.
    ptf = new SIptfunc(name, numargs, saflgs, func);
    spALTfuncs->link(ptf, false);
    spALTfuncs = spALTfuncs->check_rehash();
}


stringlist *
SIparser::funcList()
{
    stringlist *s0 = 0;
    tgen_t<SIptfunc> tgen(spIFfuncs);
    SIptfunc *ptf;
    while ((ptf = tgen.next()) != 0)
        s0 = new stringlist(lstring::copy(ptf->tab_name()), s0);
    stringlist::sort(s0);
    return (s0);
}


stringlist *
SIparser::altFuncList()
{
    stringlist *s0 = 0;
    tgen_t<SIptfunc> tgen(spALTfuncs);
    SIptfunc *ptf;
    while ((ptf = tgen.next()) != 0)
        s0 = new stringlist(lstring::copy(ptf->tab_name()), s0);
    stringlist::sort(s0);
    return (s0);
}


// Find an interface function by name.
//
SIptfunc *
SIparser::function(const char *name)
{
    if (!spIFfuncs)
        return (0);
    return (spIFfuncs->find(name));
}


// Find an alternate function by name.
//
SIptfunc *
SIparser::altFunction(const char *name)
{
    if (!spALTfuncs)
        return (0);
    return (spALTfuncs->find(name));
}


void
SIparser::clearExprs()
{
    delete spExprs;
    spExprs = 0;
}


// Add a message to the message queue.
//
void
SIparser::pushError(const char *fmt, ...)
{
    va_list args;
    char buf[1024];

    va_start(args, fmt);
    vsnprintf(buf, 1024, fmt, args);
    va_end(args);

    spErrMsgs = new stringlist(lstring::copy(buf), spErrMsgs);
}


// Pop off and return a message from the queue.
//
char *
SIparser::popError()
{
    stringlist *sl = spErrMsgs;
    if (sl) {
       spErrMsgs = sl->next;
       char *s = sl->string;
       sl->string = 0;
       delete sl;
       return (s);
    }
    return (0);
}


// Print out the error messages.
// leading "-p" replaced by "Parse error: "
// leading "-e" replaced by "Execute error: "
//
char *
SIparser::errMessage()
{
    sLstr lstr;

    char *msg;
    while ((msg = popError()) != 0) {
        if (*msg == '-' && *(msg + 1) == 'p') {
            lstr.add("  Parse error: ");
            lstr.add(msg+2);
            lstr.add(".\n");
        }
        else if (*msg == '-' && *(msg + 1) == 'e') {
            lstr.add("  Execute error: ");
            lstr.add(msg+2);
            lstr.add(".\n");
        }
        else {
            lstr.add("  ");
            lstr.add(msg);
            lstr.add(".\n");
        }
        delete [] msg;
    }
    return (lstr.string_trim());
}


// Parse the expression in *line as far as possible, and return the
// parse tree obtained.
//
ParseNode *
SIparser::getTree(const char **line, bool nohalt)
{
    spLastToken = TOK_END;
    spLastType = ElemNumber;
    ParseNode *p = parse(line);
    if (!p || p->check() != OK) {
        ParseNode::allocate_pnode(P_CLEAR);
        if (nohalt)
            SI()->ClearInterrupt();
        else
            SI()->Halt();
        return (0);
    }
    ParseNode::allocate_pnode(P_RESET);
    return (p);
}


// Return a parse tree for a layer expression.  Layer expressions have
// their own variable list and function table.
//
// Returns null on error, be sure to call errMessage or popError to
// drain the message list.
//
// If noexp is false, derived layer expressions will be parsed
// recursively, providing a tree that has regular layers only. 
// Otherwise, derived layers will appear (by name) the same as regular
// layers.  This option may apply when the component derived layers
// have been pre-evaluated, so that evaluation requires only grabbing
// geometry from the cell database, as for normal layers.
//
ParseNode *
SIparser::getLexprTree(const char **string, bool noexp)
{
    siVariable *tmpv = spVariables;
    spVariables = spLexpVars;
    bool tmp = setUseAltFuncs(true);
    ParseNode *p = getTree(string, true);
    setUseAltFuncs(tmp);
    spLexpVars = spVariables;
    spVariables = tmpv;
    if (p) {
        if (!noexp) {
            ParseNode *px;
            if (p->checkExpandTree(&px) != OK) {
                ParseNode::destroy(p);
                p = 0;
            }
            else if (px) {
                ParseNode::destroy(p);
                p = px;
            }
        }
        else if (p->checkTree() != OK) {
            ParseNode::destroy(p);
            p = 0;
        }
    }
    return (p);
}


bool
SIparser::hasGlobalVariable(const char *varstr)
{
    const char *st = varstr;
    bool tmpg = spGlobalOnly;
    spGlobalOnly = true;
    ParseNode *p = getTree(&st, true);
    spGlobalOnly = tmpg;
    if (!p)
        return (false);
    if (p->type != PT_VAR) {
        ParseNode::destroy(p);
        return (false);
    }
    // If an array, check if subscripting is in bounds.
    if (p->data.v->type == TYP_ARRAY && p->left)
        return (ADATA(p->data.v->content.a)->check(p->left) == OK);
    return (true);
}


bool
SIparser::getGlobalVariable(const char *varstr, siVariable *v)
{
    const char *st = varstr;
    bool tmpg = spGlobalOnly;
    spGlobalOnly = true;
    ParseNode *p = getTree(&st, true);
    spGlobalOnly = tmpg;
    if (!p)
        return (BAD);
    if (p->type != PT_VAR) {
        ParseNode::destroy(p);
        return (BAD);
    }

    ParseNode *pl = ParseNode::allocate_pnode(P_ALLOC);
    pl->data.v = v;
    pl->type = PT_VAR;
    pl->evfunc = &pt_var;

    ParseNode *pa = mkbnode(TOK_ASSIGN, pl, p);

    siVariable res;
    SIlexprCx cx;
    bool ret = (*pa->evfunc)(pa, &res, &cx);
    ParseNode::destroy(pa);
    res.clear();
    return (ret);
}


// This is primarily for setting global variables while executing a
// script, from Python or Tcl.  It is very much like an assignment.
//
bool
SIparser::setGlobalVariable(const char *varstr, siVariable *v)
{
    const char *st = varstr;
    bool tmpg = spGlobalOnly;
    spGlobalOnly = true;
    ParseNode *p = getTree(&st, true);
    spGlobalOnly = tmpg;
    if (!p)
        return (BAD);
    if (p->type != PT_VAR) {
        ParseNode::destroy(p);
        return (BAD);
    }

    ParseNode *pr = ParseNode::allocate_pnode(P_ALLOC);
    pr->data.v = v;
    pr->type = PT_VAR;
    pr->evfunc = &pt_var;

    ParseNode *pa = mkbnode(TOK_ASSIGN, p, pr);

    siVariable res;
    SIlexprCx cx;
    bool ret = (*pa->evfunc)(pa, &res, &cx);
    ParseNode::destroy(pa);
    res.clear();
    return (ret);
}


// Evaluate the expression in line, and print the result in buf.
// Return true if error.
//
bool
SIparser::evaluate(const char *line, char *buf, int nchars)
{
    ParseNode *p;
    siVariable v, *tmpv;
    bool err = OK;

    SI()->ClearInterrupt();
    tmpv = spVariables;
    spVariables = 0;
    p = getTree(&line, true);
    if (p) {
        SIlexprCx cx;
        if ((*p->evfunc)(p, &v, &cx) == OK) {
            switch (v.type) {
            case TYP_SCALAR:
                sprintf(buf, "%g", v.content.value);
                break;
            case TYP_ARRAY:
                if (!v.content.a || !v.content.a->values())
                    err = BAD;
                else
                    sprintf(buf, "%g", v.content.a->values()[0]);
                break;
            case TYP_NOTYPE:
            case TYP_STRING:
                if (!v.content.string) {
                    buf[0] = '0';
                    buf[1] = 0;
                }
                else {
                    strncpy(buf, v.content.string, nchars);
                    buf[nchars-1] = '\0';
                }
                break;
            default:
                err = BAD;
            }
        }
        else
            err = BAD;

        ParseNode::destroy(p);
    }
    else
        err = BAD;
    clearVariables();
    spVariables = tmpv;
    return (err);
}


#ifndef SP_NUMBER
// This serves two purposes:  1) faster than calling pow(), and 2)
// more accurate than some crappy pow() implementations.  In
// particular, (1.00e+00==1) fails (appears unequal) in GLIBC-2.2.4
// (Red Hat 7.2).
//
namespace {
    double powers[] = {
        1.0e+0,
        1.0e+1,
        1.0e+2,
        1.0e+3,
        1.0e+4,
        1.0e+5,
        1.0e+6,
        1.0e+7,
        1.0e+8,
        1.0e+9,
        1.0e+10,
        1.0e+11,
        1.0e+12,
        1.0e+13,
        1.0e+14,
        1.0e+15,
        1.0e+16,
        1.0e+17,
        1.0e+18,
        1.0e+19,
        1.0e+20,
        1.0e+21,
        1.0e+22,
        1.0e+23
    };
}
#define ETABSIZE 24
#endif
        

// Parse a number, advance the pointer.
//
double
SIparser::numberParse(const char **line, bool *error)
{
    if (error)
        *error = false;

    // loop through all of the input token
    const char *str = *line;

    if (*str == '\'') {
        // character constant
        unsigned s = 0;
        str++;
        if (*str == '\\') {
            str++;
            switch (*str) {
            case 'a':
                s = '\a'; // bell
                break;
            case 'b':
                s = '\b'; // bsp
                break;
            case 'f':
                s = '\f'; // form feed
                break;
            case 'n':
                s = '\n'; // newline
                break;
            case 'r':
                s = '\r'; // return
                break;
            case 't':
                s = '\t'; // tab
                break;
            case 'v':
                s = '\v'; // vert tab
                break;
            case '\'':
                s = '\''; // " ' "
                break;
            case '\\':
                s = '\\'; // " \ "
                break;
            default:
                if (isdigit(*str) && *str < '8') {
                    sscanf(str, "%o", &s);
                    break;
                }
                *error = true;
                return (0.0);
            }
        }
        else
            s = *str;
        while (*str && *str != '\'')
            str++;
        if (*str)
            str++;
        *line = str;
        return ((double)s);
    }

    // recognize 0xHEXNUM
    if (*str == '0' && (*(str+1) == 'x' || *(str+1) == 'X')) {
        double mantis = 0.0;
        str += 2;
        while (isxdigit(*str)) {
            // digit, so accumulate it
            int ii;
            if (isdigit(*str))
                ii = *str - '0';
            else
                ii = (isupper(*str) ? tolower(*str) : *str) - 'a' + 10;
            mantis = 16*mantis + ii;
            str++;
        }
        *line = str;
        return (mantis);
    }

#ifdef SP_NUMBER
    double *pd = SPnum.parse(&str, false);
    if (!pd) {
        if (error)
            *error = true;
        return (0.0);
    }
    *line = str;
    return (*pd);

#else
    double mantis = 0.0;
    int expo1 = 0;
    int expo2 = 0;
    int sign = 1;
    int expsgn = 1;

    if (*str == '+')
        // plus, so do nothing except skip it
        str++;
    if (*str == '-') {
        // minus, so skip it, and change sign
        str++;
        sign = -1;
    }
    if  (*str == '\0' || (!isdigit(*str) && *str != '.')) {
        // number looks like just a sign!
        *error = true;
        return (0.0);
    }
    while (isdigit(*str)) {
        // digit, so accumulate it
        mantis = 10*mantis + (*str - '0');
        str++;
    }
    if (*str == '\0') {
        // reached the end of token - done
        *line = str;
        return ((double)mantis*sign);
    }
    // after decimal point!
    if (*str == '.') {
        // found a decimal point!
        str++; // skip to next character
        if (*str == '\0') {
            // number ends in the decimal point
            *line = str;
            return ((double)mantis*sign);
        }
        while (isdigit(*str)) {
            // digit, so accumulate it
            mantis = 10*mantis + (*str - '0');
            expo1--;
            if (*str == '\0') {
                // reached the end of token - done
                *line = str;
                if (-expo1 < ETABSIZE)
                    return (sign*mantis/powers[-expo1]);
                return (sign*mantis*pow(10.0, (double)expo1));
            }
            str++;
        }
    }
    // now look for "E","e",etc to indicate an exponent
    if ((*str == 'E') || (*str == 'e') ||
            (*str == 'D') || (*str == 'd')) {
        // have an exponent, so skip the e
        str++;
        // now look for exponent sign
        if (*str == '+')
            // just skip +
            str++;
        if (*str == '-') {
            // skip over minus sign
            str++;
            // and make a negative exponent
            expsgn = -1;
        }
        // now look for the digits of the exponent
        while (isdigit(*str)) {
            expo2 = 10*expo2 + (*str - '0');
            str++;
        }
    }
    *line = str;
    expo1 += expsgn*expo2;
    if (expo1 >= 0) {
        if (expo1 < ETABSIZE)
            return (sign*mantis*powers[expo1]);
    }
    else {
        if (-expo1 < ETABSIZE)
            return (sign*mantis/powers[-expo1]);
    }
    return (sign*mantis*pow(10.0, (double)expo1));
#endif
}


// The operator-precedence table for the parser.

#define G 1
#define L 2
#define E 3
#define R 4

// G  Greater than, => reduce
// L  Less than, => push
// E  Equal
// R  Error

//    [] ()
//    ++ --
//    ! u-
//    ^
//    * / %
//    + -
//    < <= > >=
//    == !=
//    &
//    |
//    =
//    ,

namespace {
char prectable[30][30] =
  {
     /* $  +  -  *  %  /  ^  == >  <  >= <= != &  |  ,  ?  :  =  u- !  +L -L +R -R v  (  )  [  ]*/
/*$ */ {R, L, L, L, L, L, L, L, L, L, L, L, L, L, L, L, L, L, L, L, L, L, L, L, L, L, L, R, L, R},
/*+ */ {G, G, G, L, L, L, L, G, G, G, G, G, G, G, G, G, G, G, R, L, L, L, L, L, L, L, L, G, L, G},
/*- */ {G, G, G, L, L, L, L, G, G, G, G, G, G, G, G, G, G, G, R, L, L, L, L, L, L, L, L, G, L, G},
/** */ {G, G, G, G, G, G, L, G, G, G, G, G, G, G, G, G, G, G, R, L, L, L, L, L, L, L, L, G, L, G},
/*% */ {G, G, G, G, G, G, L, G, G, G, G, G, G, G, G, G, G, G, R, L, L, L, L, L, L, L, L, G, L, G},
/*/ */ {G, G, G, G, G, G, L, G, G, G, G, G, G, G, G, G, G, G, R, L, L, L, L, L, L, L, L, G, L, G},
/*^ */ {G, G, G, G, G, G, G, G, G, G, G, G, G, G, G, G, G, G, R, L, L, L, L, L, L, L, L, G, L, G},
/*==*/ {G, L, L, L, L, L, L, G, G, G, G, G, G, G, G, G, G, G, R, L, L, L, L, L, L, L, L, G, L, G},
/*> */ {G, L, L, L, L, L, L, G, G, G, G, G, G, G, G, G, G, G, R, L, L, L, L, L, L, L, L, G, L, G},
/*< */ {G, L, L, L, L, L, L, G, G, G, G, G, G, G, G, G, G, G, R, L, L, L, L, L, L, L, L, G, L, G},
/*>=*/ {G, L, L, L, L, L, L, G, G, G, G, G, G, G, G, G, G, G, R, L, L, L, L, L, L, L, L, G, L, G},
/*<=*/ {G, L, L, L, L, L, L, G, G, G, G, G, G, G, G, G, G, G, R, L, L, L, L, L, L, L, L, G, L, G},
/*!=*/ {G, L, L, L, L, L, L, G, G, G, G, G, G, G, G, G, G, G, R, L, L, L, L, L, L, L, L, G, L, G},
/*& */ {G, L, L, L, L, L, L, L, L, L, L, L, L, G, G, G, G, G, R, L, L, L, L, L, L, L, L, G, L, G},
/*| */ {G, L, L, L, L, L, L, L, L, L, L, L, L, L, G, G, G, G, R, L, L, L, L, L, L, L, L, G, L, G},
/*, */ {G, L, L, L, L, L, L, L, L, L, L, L, L, L, L, G, L, L, R, L, L, L, L, L, L, L, L, G, L, G},
/*? */ {G, L, L, L, L, L, L, L, L, L, L, L, L, L, L, L, R, L, L, L, L, L, L, L, L, L, L, G, L, L},
/*: */ {G, L, L, L, L, L, L, L, L, L, L, L, L, L, L, L, L, R, L, L, L, L, L, L, L, L, L, G, L, L},
/*= */ {G, L, L, L, L, L, L, L, L, L, L, L, L, L, L, R, G, G, L, L, L, L, L, L, L, L, L, G, L, L},
/*u-*/ {G, G, G, G, G, G, G, G, G, G, G, G, G, G, G, G, G, G, R, L, L, L, L, L, L, L, L, G, L, G},
/*! */ {G, G, G, G, G, G, G, G, G, G, G, G, G, G, G, G, G, G, R, L, L, L, L, L, L, L, L, G, L, G},
/*+L*/ {G, G, G, G, G, G, G, G, G, G, G, G, G, G, G, G, G, G, G, G, G, R, R, R, R, L, R, R, R, R},
/*-L*/ {G, G, G, G, G, G, G, G, G, G, G, G, G, G, G, G, G, G, G, G, G, R, R, R, R, L, R, R, R, R},
/*+R*/ {G, G, G, G, G, G, G, G, G, G, G, G, G, G, G, G, G, G, G, G, G, R, R, R, R, R, R, G, R, G},
/*-R*/ {G, G, G, G, G, G, G, G, G, G, G, G, G, G, G, G, G, G, G, G, G, R, R, R, R, R, R, G, R, G},
/*v */ {G, G, G, G, G, G, G, G, G, G, G, G, G, G, G, G, G, G, L, G, G, R, R, G, G, R, G, G, G, G},
/*( */ {R, L, L, L, L, L, L, L, L, L, L, L, L, L, L, L, L, L, L, L, L, L, L, L, L, L, L, E, L, L},
/*) */ {G, G, G, G, G, G, G, G, G, G, G, G, G, G, G, G, G, G, G, G, G, R, R, G, G, R, R, G, R, G},
/*[ */ {R, L, L, L, L, L, L, L, L, L, L, L, L, L, L, L, G, G, L, L, L, L, L, L, L, L, L, L, L, E},
/*] */ {G, G, G, G, G, G, G, G, G, G, G, G, G, G, G, G, G, G, G, G, G, R, R, G, G, R, R, G, R, G},
  } ;
}


namespace {
    const char *
    token_str(int i)
    {
        switch (i) {
        case TOK_END:         return ("end");
        case TOK_PLUS:        return ("+");
        case TOK_MINUS:       return ("-");
        case TOK_TIMES:       return ("*");
        case TOK_MOD:         return ("%");
        case TOK_DIVIDE:      return ("/");
        case TOK_POWER:       return ("^");
        case TOK_EQ:          return ("==");
        case TOK_GT:          return (">");
        case TOK_LT:          return ("<");
        case TOK_GE:          return (">=");
        case TOK_LE:          return ("<=");
        case TOK_NE:          return ("!=");
        case TOK_AND:         return ("&");
        case TOK_OR:          return ("|");
        case TOK_COMMA:       return (",");
        case TOK_COND:        return ("?");
        case TOK_COLON:       return (":");
        case TOK_ASSIGN:      return ("=");
        case TOK_UMINUS:      return ("-");
        case TOK_NOT:         return ("!");
        case TOK_LINCR:       return ("++");
        case TOK_LDECR:       return ("--");
        case TOK_RINCR:       return ("++");
        case TOK_RDECR:       return ("--");
        case TOK_VALUE:       return ("val");
        case TOK_LPAREN:      return ("(");
        case TOK_RPAREN:      return (")");
        case TOK_LBRAC:       return ("[");
        case TOK_RBRAC:       return ("]");
        }
        return ("bad token");
    }
}


ParseNode *
SIparser::parse(const char **line)
{
    enum ERR_Type {
        ERR_None,
        ERR_Overflow,
        ERR_Prec,
        ERR_Node,
        ERR_OpNode,
        ERR_NodeOp,
        ERR_ParNodePar,
        ERR_FuncName,
        ERR_FuncCall,
        ERR_Brac,
        ERR_ArrayName,
        ERR_BracNodeBrac,
        ERR_NodeOpNode
    };
    ERR_Type error = ERR_None;

    SIelement stack[PT_STACKSIZE];

    spTriNest = 0;
    SIelement *top=0;
    stack[0].token = TOK_END;
    SIelement *next = lexer(line);
    if (!next)
        return (0);

    ParseNode *pn, *lpn, *rpn;
    char *s;
    int sp = 0, st = 0;
    while (sp > 1 || next->token != TOK_END) {
        // Find the top-most terminal
        int i = sp;
        do {
            top = &stack[i--];
        } while (top->token == TOK_VALUE);


        switch (prectable[top->token][next->token]) {
        case L:
        case E:
            // Push the token read
            if (sp == PT_STACKSIZE - 1) {
                error = ERR_Overflow;
                goto err;
            }
            stack[++sp] = *next;
            next = lexer(line);
            continue;

        case R:
            error = ERR_Prec;
            goto err;

        case G:
            // Reduce. Make st and sp point to the elts on the
            // stack at the end and beginning of the junk to
            // reduce, then try and do some stuff. When scanning
            // back for a <, ignore VALUES.
            //
            st = sp;
            if (stack[sp].token == TOK_VALUE)
                sp--;
            while (sp > 0) {
                if (stack[sp - 1].token == TOK_VALUE)
                    i = 2;  // No 2 pnodes together...
                else
                    i = 1;
                if (prectable[stack[sp - i].token][stack[sp].token] == L)
                    break;
                else
                    sp = sp - i;
            }
            if (stack[sp - 1].token == TOK_VALUE)
                sp--;
            // Now try and see what we can make of this.
            // The possibilities are:
            //    node
            //    op node
            //    node op
            //    node op node
            //    ( node )
            //    node ( node )
            //    node [ node ]
            //
            if (st == sp) {
                // node
                pn = makepnode(&stack[st]);
                if (!pn) {
                    error = ERR_Node;
                    goto err;
                }
            }
            else if (st == sp + 1 && (stack[sp].token == TOK_UMINUS ||
                    stack[sp].token == TOK_NOT ||
                    stack[sp].token == TOK_LINCR ||
                    stack[sp].token == TOK_LDECR)) {
                // op node
                lpn = makepnode(&stack[st]);
                if (!lpn) {
                    error = ERR_OpNode;
                    goto err;
                }
                pn = mkunode(stack[sp].token, lpn);
            }
            else if (st == sp + 1 && (stack[st].token == TOK_RINCR ||
                    stack[st].token == TOK_RDECR)) {
                // node op
                lpn = makepnode(&stack[sp]);
                if (!lpn) {
                    error = ERR_NodeOp;
                    goto err;
                }
                pn = mkunode(stack[st].token, lpn);
            }
            else if (stack[sp].token == TOK_LPAREN &&
                       stack[st].token == TOK_RPAREN) {
                // ( node )
                pn = makepnode(&stack[sp + 1]);
                if (!pn) {
                    error = ERR_ParNodePar;
                    goto err;
                }
            }
            else if (stack[sp + 1].token == TOK_LPAREN &&
                       stack[st].token == TOK_RPAREN) {
                // func ( node )
                lpn = makepnode(&stack[sp + 2]);
                // lpn is null if no args
                if (stack[sp].type != ElemString) {
                    error = ERR_FuncName;
                    goto err;
                }
                s = stack[sp].value.string;
                stack[sp].value.string = 0;
                pn = mkfnode(s, lpn);
                if (!pn) {
                    error = ERR_FuncCall;
                    goto err;
                }
            }
            else if (stack[sp + 1].token == TOK_LBRAC &&
                       stack[st].token == TOK_RBRAC) {
                // node [ node ]
                lpn = makepnode(&stack[sp + 2]);
                if (!lpn || stack[sp].type != ElemString) {
                    error = ERR_Brac;
                    goto err;
                }
                s = stack[sp].value.string;
                stack[sp].value.string = 0;
                pn = mksnode(s, lpn);
                if (!pn) {
                    error = ERR_ArrayName;
                    goto err;
                }
            }
            else if (stack[sp].token == TOK_LBRAC &&
                       stack[st].token == TOK_RBRAC) {
                // [ node ]
                pn = makepnode(&stack[sp + 1]);
                if (!pn) {
                    error = ERR_BracNodeBrac;
                    goto err;
                }
            }
            else { // node op node
                lpn = makepnode(&stack[sp]);
                rpn = makepnode(&stack[st]);
                if (!lpn || !rpn) {
                    error = ERR_NodeOpNode;
                    goto err;
                }
                pn = mkbnode(stack[sp + 1].token, lpn, rpn);
            }
            stack[sp].token = TOK_VALUE;
            stack[sp].type = ElemNode;
            stack[sp].value.pnode = pn;
            continue;
        }
    }
    pn = makepnode(&stack[1]);
    if (pn)
        return (pn);
err:
    switch (error) {
    case ERR_None:
        break;
    case ERR_Overflow:
        pushError("-pparser stack overflow");
        break;
    case ERR_Prec:
        pushError("-pbad precedence, top=\'%s\' next=\'%s\'",
            token_str(top->token), token_str(next->token));
        break;
    case ERR_Node:
        pushError("-pbad node");
        break;
    case ERR_OpNode:
        pushError("-pbad node in (op)(node)");
        break;
    case ERR_NodeOp:
        pushError("-pbad node in (node)(op)");
        break;
    case ERR_ParNodePar:
        pushError("-pbad node in ( node )");
        break;
    case ERR_FuncName:
        pushError("-pfunction name not a string token");
        break;
    case ERR_FuncCall:
        pushError("-pbad function call");
        break;
    case ERR_Brac:
        pushError("-p[ ] after non-string token");
        break;
    case ERR_ArrayName:
        pushError("-pbad array name");
        break;
    case ERR_BracNodeBrac:
        pushError("-pbad node in [ node ]");
        break;
    case ERR_NodeOpNode:
        pushError("-pbad node in (node)(op)(node)");
        break;
    }
    while (st > 0) {
        if (stack[st].token == TOK_VALUE && stack[st].type == ElemString)
            delete [] stack[st].value.string;
        st--;
    }
    SI()->Halt();
    return (0);
}


namespace {
    // Return true if the characters in str to end match a layer name in
    // the current mode.  Also assume a layer name if the string is a
    // 4 or 8-digit hex number.
    //
    bool
    is_layer(const char *str, const char *end)
    {
        while (isspace(*str))
            str++;
        if (end > str && *(end-1) == '.')
            // Trailing '.' will be included in number token.
            end--;
        int l = end - str;
        if (l <= 0 || l >= 63)
            return (false);
        if (l == 4 || l == 8) {
            bool isx = true;
            for (int i = 0; i < l; i++) {
                if (!isxdigit(str[i])) {
                    isx = false;
                    break;
                }
            }
            if (isx)
                return (true);
        }
        char buf[64];
        char *s = buf;
        while (str != end)
            *s++ = *str++;
        *s = 0;
        return (CDldb()->findLayer(buf, SIparse()->ifGetCurMode()));
    }
}


namespace {
    const char *specials = " \t\n%()[]-^+*,;/|&<>~!=";

    // Return true and advance pstr if *pstr leads with word followed by
    // a member of specials, case insensitive.
    //
    bool chkword(const char **pstr, const char *word)
    {
        const char *s = word;
        const char *t = *pstr;
        while (*s && *t) {
            if ((isupper(*s) ? tolower(*s) : *s) !=
                    (isupper(*t) ? tolower(*t) : *t))
                return (false);
            s++;
            t++;
        }
        if (*s)
            return (false);
        if (!*t)
            return (false);  // Don't match end of line.
        if (strchr(specials, *t)) {
            *pstr = t;
            return (true);
        }
        return (false);
    }
}


SIelement*
SIparser::lexer(const char **line)
{
    static SIelement el;
    el.token = TOK_END;
    int j = 0;

    if (!line || !*line)
        return (0);
    const char *sbuf = *line;
    while (isspace(*sbuf))
        sbuf++;

    switch (*sbuf) {

    case 0:
    case '#':
        // comment
        goto done;

    case ';':
        // end of expr
        goto done;

    case '-':
        sbuf++;
        if (*sbuf == '-') {
            sbuf++;
            if (spLastToken == TOK_VALUE || spLastToken == TOK_RPAREN ||
                    spLastToken == TOK_RBRAC)
                el.token = TOK_RDECR;
            else
                el.token = TOK_LDECR;
        }
        else if (spLastToken == TOK_VALUE || spLastToken == TOK_RPAREN ||
                spLastToken == TOK_RBRAC)
            el.token = TOK_MINUS;
        else
            el.token = TOK_UMINUS;
        break;

    case '+':
        sbuf++;
        if (*sbuf == '+') {
            sbuf++;
            if (spLastToken == TOK_VALUE || spLastToken == TOK_RPAREN ||
                    spLastToken == TOK_RBRAC)
                el.token = TOK_RINCR;
            else
                el.token = TOK_LINCR;
        }
        else
            el.token = TOK_PLUS;
        break;

    case ',':
        el.token = TOK_COMMA;
        sbuf++;
        break;

    case '*':
        el.token = TOK_TIMES;
        sbuf++;
        break;

    case '%':
        el.token = TOK_MOD;
        sbuf++;
        break;

    case '/':
        if (*(sbuf+1) == '/' || *(sbuf+1) == '*')
            // comment - end of input
            goto done;
        el.token = TOK_DIVIDE;
        sbuf++;
        break;

    case '^':
        el.token = TOK_POWER;
        sbuf++;
        break;

    case '(':
        if ((spLastToken == TOK_VALUE && spLastType == ElemNumber) ||
                spLastToken == TOK_RPAREN || spLastToken == TOK_RBRAC)
            goto done;
        el.token = TOK_LPAREN;
        sbuf++;
        break;

    case ')':
        el.token = TOK_RPAREN;
        sbuf++;
        break;

    case '[':
        el.token = TOK_LBRAC;
        sbuf++;
        break;

    case ']':
        el.token = TOK_RBRAC;
        sbuf++;
        break;

    case '=':
        for (j = 1; isspace(sbuf[j]); j++) ;
        if (sbuf[j] == '=') {
            el.token = TOK_EQ;
            sbuf += 1 + j;
        }
        else {
            el.token = TOK_ASSIGN;
            sbuf++;
        }
        break;

    case '!':
        for (j = 1; isspace(sbuf[j]); j++) ;
        if (sbuf[j] == '=') {
            el.token = TOK_NE;
            sbuf += 1 + j;
        }
        else {
            el.token = TOK_NOT;
            sbuf++;
        }
        break;

    case '>':
    case '<':
        for (j = 1; isspace(sbuf[j]); j++) ;
        // The lexer makes <> into < >
        if ((sbuf[j] == '<' || sbuf[j] == '>') && sbuf[0] != sbuf[j]) {
            // Allow both <> and >< for NE
            el.token = TOK_NE;
            sbuf += 1 + j;
        }
        else if (sbuf[j] == '=') {
            if (sbuf[0] == '>')
                el.token = TOK_GE;
            else
                el.token = TOK_LE;
            sbuf += 1 + j;
        }
        else {
            if (sbuf[0] == '>')
                el.token = TOK_GT;
            else
                el.token = TOK_LT;
            sbuf++;
        }
        break;

    case '&':
        el.token = TOK_AND;
        sbuf++;
        break;

    case '|':
        el.token = TOK_OR;
        sbuf++;
        break;

    case '~':
        el.token = TOK_NOT;
        sbuf++;
        break;

    case '?':
        spTriNest++;
        el.token = TOK_COND;
        sbuf++;
        break;

    case ':':
        if (spTriNest)
            spTriNest--;
        el.token = TOK_COLON;
        sbuf++;
        break;

    case '"':
        if (spLastToken != TOK_VALUE && spLastToken != TOK_RPAREN &&
                spLastToken != TOK_RBRAC) {
            el.value.string = grab_string(&sbuf);
            el.type = ElemString;
            el.token = TOK_VALUE;
        }
        else
            goto done;
        break;
    }

    if (el.token == TOK_END) {
        const char *ss = sbuf;
        bool notanum;
        double td = numberParse(&ss, &notanum);
        if (!notanum) {

            // The token looks like a number, note that if we are
            // parsing a layer expression and the text matches a layer
            // name, this will override.  Note also that numbers may
            // not have trailing alpha characters, e.g., no SPICE-type
            // "1.0pF" units allowed.
            //
            // We also need to recognize layer[.stname][.cellname] as
            // a layer name.
        
            if (isalpha(*ss) || *ss == '_' || *ss == '$' || *ss == '.')
                notanum = true;
            else if (*ss == '?') {
                // This might be a conditional, or part of the
                // cellname that follows a layername.  The character
                // is not allowed in a layer name.

                const char *p = strchr(sbuf, '.');
                if (p && is_layer(sbuf, p))
                    notanum = true;
            }
            else if (spUseAltFuncs && is_layer(sbuf, ss))
                notanum = true;
        }
        if (!notanum) {
            if (spLastToken != TOK_VALUE && spLastToken != TOK_RPAREN &&
                    spLastToken != TOK_RBRAC) {
                el.value.real = td;
                el.type = ElemNumber;
                el.token = TOK_VALUE;
                sbuf = ss;
            }
        }
        else {
            if (chkword(&sbuf, "or"))
                el.token = TOK_OR;
            else if (chkword(&sbuf, "and"))
                el.token = TOK_AND;
            else if (chkword(&sbuf, "not")) {
                // In layer expresions, A not B <=> A - B <=> A &! B.
                if (spUseAltFuncs && (spLastToken == TOK_VALUE ||
                        spLastToken == TOK_RPAREN || spLastToken == TOK_RBRAC))
                    el.token = TOK_MINUS;
                else
                    el.token = TOK_NOT;
            }
            else if (spUseAltFuncs && chkword(&sbuf, "xor"))
                el.token = TOK_POWER;
            else if (chkword(&sbuf, "gt"))
                el.token = TOK_GT;
            else if (chkword(&sbuf, "lt"))
                el.token = TOK_LT;
            else if (chkword(&sbuf, "ge"))
                el.token = TOK_GE;
            else if (chkword(&sbuf, "le"))
                el.token = TOK_LE;
            else if (chkword(&sbuf, "ne"))
                el.token = TOK_NE;
            else if (chkword(&sbuf, "eq"))
                el.token = TOK_EQ;
            else if (spLastToken != TOK_VALUE && spLastToken != TOK_RPAREN &&
                    spLastToken != TOK_RBRAC) {
                el.value.string = grab_string(&sbuf);
                el.type = ElemString;
                el.token = TOK_VALUE;
            }
        }
    }
done:
    spLastToken = el.token;
    spLastType = el.type;
    *line = sbuf;
    return (&el);
}


// Grab a string token.  This is a little tricky, to cover all bases.
// A string token consists of a run of characters that are not in the
// "specials", however specials will be included if found within
// double quotes.  Quote chars are retained, and don't necessarily
// delimit the string.  Within quotes, ANSI C X3J11 backslash
// expansions are evaluated.  Also, the backslash is stripped from
// '\"', which is maybe not a good thing to do, but may make sense in
// some situations.
//
char *
SIparser::grab_string(const char **pstr)
{
    bool inq = false;
    const char *s = *pstr;
    while (*s) {
        if (*s == '"') {
            if (s == *pstr || s[-1] != '\\')
                inq = !inq;
        }
        else if (!inq && (strchr(specials, *s) || (*s == ':' && spTriNest)))
            break;
        s++;
    }
    const char *se = s;
    char *str = new char[se - *pstr + 1];
    char *e = str;

    inq = false;
    s = *pstr;
    while (*s) {
        if (*s == '"') {
            if (s == *pstr || s[-1] != '\\')
                inq = !inq;
        }
        else if (!inq && (strchr(specials, *s) || (*s == ':' && spTriNest)))
            break;

        if (inq && s != *pstr && s[-1] == '\\') {
            if (*s == '"') { e--; *e++ = '"'; s++; continue; }

            if (*s == 'a') { e--; *e++ = '\a'; s++; continue; }
            if (*s == 'b') { e--; *e++ = '\b'; s++; continue; }
            if (*s == 'f') { e--; *e++ = '\f'; s++; continue; }
            if (*s == 'n') { e--; *e++ = '\n'; s++; continue; }
            if (*s == 'r') { e--; *e++ = '\r'; s++; continue; }
            if (*s == 't') { e--; *e++ = '\t'; s++; continue; }
            if (*s == 'v') { e--; *e++ = '\v'; s++; continue; }
            if (*s == '\'') { e--; *e++ = '\''; s++; continue; }
            if (*s == '\\') { e--; *e++ = '\\'; s++; continue; }
            if (isdigit(*s)) {
                const char *stmp = s;
                int i = *s - '0';
                if (isdigit(s[1])) {
                    s++;
                    i <<= 3;
                    i += *s - '0';
                    if (isdigit(s[1])) {
                        s++;
                        i <<= 3;
                        i += *s - '0';
                    }
                }
                if (i < 256) {
                    e--;
                    *e++ = i;
                    s++;
                    continue;
                }
                s = stmp;
            }
        }
        *e++ = *s++;
    }
    *e = 0;
    *pstr = se;
    return (str);
}


// Given a pointer to an element, make a pnode out of it (if it already
// is one, return a pointer to it). If it isn't of type VALUE, then return
// 0.
//
ParseNode *
SIparser::makepnode(SIelement *elem)
{
    if (elem->token != TOK_VALUE)
        return (0);

    ParseNode *ret = 0;
    if (elem->type == ElemString) {
        ret = mksnode(elem->value.string, 0);
        elem->value.string = 0;
    }
    else if (elem->type == ElemNumber)
        ret = mknnode(elem->value.real);
    else if (elem->type == ElemNode)
        ret = elem->value.pnode;
    else
        pushError("-pbad token type");
    return (ret);
}


// Binop node.
//
ParseNode *
SIparser::mkbnode(PTokenType opnum, ParseNode *arg1, ParseNode *arg2)
{
    if (!IS_BOP(opnum)) {
        pushError("-pno such binary op num %d", opnum);
        return (0);
    }
    if (opnum == TOK_COMMA) {
        if (arg1->next) {
            ParseNode *p;
            for (p = arg1->next; p->next; p = p->next) ;
            p->next = arg2;
        }
        else
            arg1->next = arg2;
        return (arg1);
    }
    ParseNode *p = ParseNode::allocate_pnode(P_ALLOC);
    p->type = PT_BINOP;
    p->optype = opnum;
    p->data.f.funcname = spPTops[opnum].name;
    p->data.f.function = spPTops[opnum].funcptr;
    if (opnum == TOK_ASSIGN) {
        if (arg1->type != PT_VAR)
            return (0);
        p->evfunc = &pt_assign;
    }
    else if (opnum == TOK_COND)
        p->evfunc = &pt_cond;
    else
        p->evfunc = &pt_bop;
    p->left = arg1;
    p->right = arg2;
    return (p);
}


// Unop node.
//
ParseNode *
SIparser::mkunode(PTokenType opnum, ParseNode *arg1)
{
    if (!IS_UOP(opnum)) {
        pushError("-pno such unary op num %d", opnum);
        return (0);
    }
    ParseNode *p = ParseNode::allocate_pnode(P_ALLOC);
    p->type = PT_UNOP;
    p->optype = opnum;
    p->data.f.funcname = spPTops[opnum].name;
    p->data.f.function = spPTops[opnum].funcptr;
    p->evfunc = &pt_uop;
    p->left = arg1;
    return (p);
}


namespace {
    inline int nargs(ParseNode *p)
    {
        int i;
        for (i = 0; p; i++, p = p->next) ;
        return (i);
    }
}


// function node (fname freed)
//
ParseNode *
SIparser::mkfnode(char *fname, ParseNode *arg)
{
    const char *msg = "-pwrong arg count to %s";

    if (spUseAltFuncs) {
        // installable function table
        SIptfunc *ptf = altFunction(fname);

        if (ptf) {
            if (ptf->argc() != VARARGS && nargs(arg) != ptf->argc()) {
                pushError(msg, fname);
                delete [] fname;
                return (0);
            }
            ParseNode *p = ParseNode::allocate_pnode(P_ALLOC);
            p->type = PT_FUNCTION;
            p->left = arg;
            p->data.f.funcname = ptf->tab_name();
            p->data.f.function = ptf->func();
            if (ptf->argc() == VARARGS)
                p->data.f.numargs = nargs(arg);
            else
                p->data.f.numargs = ptf->argc();
            p->evfunc = &pt_fcn;
            delete [] fname;
            return (p);
        }
    }
    else {
        SIptfunc *ptf = function(fname);
        if (ptf) {
            if (ptf->argc() != VARARGS && nargs(arg) != ptf->argc()) {
                pushError(msg, fname);
                delete [] fname;
                return (0);
            }
            ParseNode *p = ParseNode::allocate_pnode(P_ALLOC);
            p->type = PT_FUNCTION;
            p->left = arg;
            p->data.f.funcname = ptf->tab_name();
            p->data.f.function = ptf->func();
            if (ptf->argc() == VARARGS)
                p->data.f.numargs = nargs(arg);
            else
                p->data.f.numargs = ptf->argc();
            p->evfunc = &pt_fcn;
            delete [] fname;
            return (p);
        }
        SIfunc *block;
        int argc;
        if ((block = SI()->GetSubfunc(fname, &argc)) != 0) {
            if (nargs(arg) != argc) {
                pushError(msg, fname);
                delete [] fname;
                return (0);
            }
            if (argc > MAXARGC) {
                pushError("-ptoo many args to %s, maximum %d",
                    fname, MAXARGC);
                delete [] fname;
                return (0);
            }
            ParseNode *p = ParseNode::allocate_pnode(P_ALLOC);
            p->type = PT_FUNCTION;
            p->left = arg;
            p->data.f.funcname = block->sf_name;
            p->data.f.userfunc = block;
            p->data.f.numargs = argc;
            p->evfunc = &pt_switch;
            // Bad news if block is deleted, so keep a reference count
            block->sf_refcnt++;
            delete [] fname;
            return (p);
        }
    }
    pushError("-pfunction %s not found", fname);
    delete [] fname;
    return (0);
}


// Number node.
//
ParseNode *
SIparser::mknnode(double number)
{
    ParseNode *p = ParseNode::allocate_pnode(P_ALLOC);
    p->type = PT_CONSTANT;
    p->data.constant.value = number;
    p->data.constant.name = 0;
    p->evfunc = &pt_const;
    return (p);
}


namespace {
    // If p is a reasonable dimension spec for an array (MAXDIMS or
    // fewer constant non-negative values) fill in d[MAXDIMS] and
    // return OK.  The dimensions are one greater than the numbers
    // passed.  Return BAD if too many dimensions or a value is
    // negative
    //
    bool
    checkdims(int *d, ParseNode *p)
    {
        memset(d, 0, MAXDIMS*sizeof(int));
        int i = 0;
        while (p) {
            if (i == MAXDIMS) {
                SIparse()->pushError(
                    "-etoo many dimensions in array subscript");
                return (BAD);
            }
            if (p->type == PT_CONSTANT) {
                d[i] = (int)p->data.constant.value + 1;
                if (d[i] <= 0) {
                    SIparse()->pushError(
                        "-enegative index in array subscript");
                    return (BAD);
                }
            }
            else {
                d[0] = 0;
                return (OK);
            }
            p = p->next;
            i++;
        }
        return (OK);
    }
}


// String node (string is freed).
//
ParseNode *
SIparser::mksnode(char *string, ParseNode *subscr)
{
    // is it a named constant?
    if (!spConstTab) {
        spConstTab = new SymTab(false, false);
        for (int i = 0; spConstants[i].name; i++)
            spConstTab->add(spConstants[i].name, &spConstants[i], false);
    }
    PTconstant *pt = (PTconstant*)SymTab::get(spConstTab, string);
    if (pt != (PTconstant*)ST_NIL) {
        ParseNode *p = ParseNode::allocate_pnode(P_ALLOC);
        p->type = PT_CONSTANT;
        p->data.constant = *pt;
        p->evfunc = &pt_const;
        delete [] string;
        return (p);
    }

    // assign the variable
    int d[MAXDIMS];
    if (checkdims(d, subscr) == BAD)
        return (0);
    siVariable *v;
    if (spGlobalOnly) {
        v = findGlobal(string);
        if (!v)
            return (0);
    }
    else
        v = mkvar(string, d);
    ParseNode *p = ParseNode::allocate_pnode(P_ALLOC);
    p->data.v = v;
    p->type = PT_VAR;
    p->evfunc = &pt_var;
    p->left = subscr;
    delete [] string;
    return (p);
}


// Create a new variable, or return an existing variable of the same name.
//
siVariable *
SIparser::mkvar(char *name, int *d)
{
    siVariable *v = findVariable(name);
    if (v)
        // We don't know much about v at this point, though it might
        // have been defined as an array.  The bounds checking and type
        // conversion testing will be done at run time
        return (v);

    v = new siVariable(lstring::copy(name), d);
    addVariable(v);
    return (v);
}


//
// Evaluation functions
//

// Static function.
bool
SIparser::pt_const(ParseNode *p, siVariable *res, void*)
{
    res->name = (char*)p->data.constant.name;
    res->content.value = p->data.constant.value;
    res->type = TYP_SCALAR;
    res->flags = VF_NAMED;
    res->next = 0;
    return (OK);
}


// Static function.
bool
SIparser::pt_var(ParseNode *p, siVariable *res, void*)
{
    siVariable *v = p->getVar();
    if (!v)
        return (BAD);
    return (v->get_var(p, res));
}


// Static function.
bool
SIparser::pt_fcn(ParseNode *p, siVariable *res, void *datap)
{
    siVariable v[MAXARGC + 1];
    if (p->data.f.numargs > MAXARGC) {
        SIparse()->pushError("-etoo many arguments to %s, maximum %d",
            p->data.f.funcname, MAXARGC);
        return (BAD);
    }
    ParseNode *arg = p->left;
    if (p->data.f.function == zlist_funcs::SIlexp_sqz_func) {
        if (!arg)
            return (BAD);
        // This is a layer expression calling the sqz
        // pseudo-function.  This merely evaluates the (Zlist*)
        // argument, after setting a flag to switch the geometry
        // source to the selection queue.
        if (!datap)
            return (BAD);
        SIlexprCx *cx = (SIlexprCx*)datap;
        bool tmp = cx->setSourceSelQueue(true);
        bool err = (*arg->evfunc)(arg, v, datap);
        cx->setSourceSelQueue(tmp);
        if (err != OK)
            return (err);
        res->type = v->type;
        res->content = v->content;
        res->flags = v->flags;
        v->content.string = 0;
        return (OK);
    }

    int types[MAXARGC];
    int i;
    for (i = 0; i < p->data.f.numargs; i++) {
        if (!arg)
            return (BAD);
        bool err = (*arg->evfunc)(arg, v+i, datap);
        if (err != OK)
            return (err);
        types[i] = arg->type;
        arg = arg->next;
    }
    v[i].type = TYP_ENDARG;

    // Initialize error recording, so we can call the GetError
    // function safely anywhere.  This is not done for the script
    // function registered with SIinterp::siSetSuppressErrInitFunc,
    // which is an "AddErrorMessage" function, which will add a
    // message to the existing list.
    //
    if (p->data.f.function != SIparse()->getSuppressErrInitFunc())
        Errs()->init_error();

    // Don't assume that p is valid if evaluation fails.
    const char *funcname = p->data.f.funcname;
    int nargs = p->data.f.numargs;

    bool err = (*p->data.f.function)(res, v, datap);

    for (i = 0; i < nargs; i++)
        v[i].gc_argv(res, types[i]);

    if (err != OK) {
        // Grab the message from Errs(), if any.  Have to strip
        // off trailing newline and period.
        char *em = 0;
        if (Errs()->has_error()) {
            em = lstring::copy(Errs()->get_error());
            if (em) {
                char *t = em + strlen(em) - 1;
                while (t >= em && (isspace(*t) || *t == '.'))
                    *t-- = 0;
                if (!*em) {
                    delete [] em;
                    em = 0;
                }
            }
        }
        if (!em)
            em = lstring::copy("unknown error");
        SIparse()->pushError("-efunction call to %s failed:\n  %s",
            funcname, em);
        delete [] em;
        return (BAD);
    }
    return (OK);
}


// Static function.
bool
SIparser::pt_switch(ParseNode *p, siVariable *res, void *datap)
{
    siVariable v[MAXARGC + 1];
    if (p->data.f.numargs > MAXARGC) {
        SIparse()->pushError("-etoo many arguments to %s, maximum %d",
            p->data.f.funcname, MAXARGC);
        return (BAD);
    }
    ParseNode *arg = p->left;
    int i;
    for (i = 0; i < p->data.f.numargs; i++) {
        if (!arg)
            return (BAD);
        bool err = (*arg->evfunc)(arg, v+i, datap);
        if (err != OK)
            return (err);
        arg = arg->next;
    }
    v[i].type = TYP_ENDARG;
    XIrt ret = SI()->EvalFunc(p->data.f.userfunc, datap, v, res);

    arg = p->left;
    for (i = 0; i < p->data.f.numargs; i++) {
        v[i].gc_argv(res, arg->type);
        arg = arg->next;
    }
    if (ret != XIok)
        return (BAD);
    return (OK);
}


// Static function.
bool
SIparser::pt_assign(ParseNode *p, siVariable *res, void *datap)
{
    siVariable *v = p->left->data.v;
    if (!v)
        return (BAD);
    if (v->flags & VF_GLOBAL) {
        v = SIparse()->findGlobal(v->name);
        if (!v) {
            SIparse()->pushError("-eglobal variable not found");
            return (BAD);
        }
    }
    return (v->assign(p, res, datap));
}


// Static function.
bool
SIparser::pt_bop(ParseNode *p, siVariable *res, void *datap)
{
    siVariable v[2];
    bool err = (*p->left->evfunc)(p->left, &v[0], datap);
    if (err != OK)
        return (err);

    if (v->bop_look_ahead(p, res))
        return (OK);

    err = (*p->right->evfunc)(p->right, &v[1], datap);
    if (err != OK)
        return (err);
    err = (*p->data.f.function)(res, v, datap);
    v[0].gc_argv(res, p->left->type);
    v[1].gc_argv(res, p->right->type);

    if (err != OK)
        return (err);
    return (OK);
}


// Static function.
bool
SIparser::pt_cond(ParseNode *p, siVariable *res, void *datap)
{
    ParseNode *pr = p->right;
    if (!pr || pr->type != PT_BINOP || pr->optype != TOK_COLON)
        return (BAD);
    if (p->left->istrue(datap)) {
        bool err = (*pr->left->evfunc)(pr->left, res, datap);
        if (err != OK)
            return (err);
    }
    else {
        bool err = (*pr->right->evfunc)(pr->right, res, datap);
        if (err != OK)
            return (err);
    }
    return (OK);
}


// Static function.
bool
SIparser::pt_uop(ParseNode *p, siVariable *res, void *datap)
{
    // The special case here is that the increment/decrement operators
    // must operate on the source variable
    if (p->optype == TOK_UMINUS) {
        siVariable r1;
        bool err = (*p->left->evfunc)(p->left, &r1, datap);
        if (err != OK)
            return (err);
        err = (*p->data.f.function)(res, &r1, datap);
        r1.gc_argv(res, p->left->type);
        if (err != OK)
            return (err);
        return (OK);
    }
    else if (p->optype == TOK_NOT) {
        siVariable r1;
        bool err = (*p->left->evfunc)(p->left, &r1, datap);
        if (err != OK)
            return (err);
        err = (*p->data.f.function)(res, &r1, datap);
        r1.gc_argv(res, p->left->type);
        if (err != OK)
            return (err);
        return (OK);
    }
    else if (p->optype == TOK_LINCR) {
        Variable *v = p->left->getVar();
        if (!v)
            return (BAD);
        if (v->type == TYP_SCALAR) {
            v->content.value += 1.0;
            res->content.value = v->content.value;
            res->type = TYP_SCALAR;
            return (OK);
        }
        if (v->type == TYP_CMPLX) {
            v->content.cx.real += 1.0;
            res->content.cx = v->content.cx;
            res->type = TYP_CMPLX;
            return (OK);
        }
        if (v->type == TYP_HANDLE) {
            v->incr__handle(res);
            return (OK);
        }
    }
    else if (p->optype == TOK_LDECR) {
        Variable *v = p->left->getVar();
        if (!v)
            return (BAD);
        if (v->type == TYP_SCALAR) {
            v->content.value -= 1.0;
            res->content.value = v->content.value;
            res->type = TYP_SCALAR;
            return (OK);
        }
        if (v->type == TYP_CMPLX) {
            v->content.cx.real -= 1.0;
            res->content.cx = v->content.cx;
            res->type = TYP_CMPLX;
            return (OK);
        }
    }
    else if (p->optype == TOK_RINCR) {
        Variable *v = p->left->getVar();
        if (!v)
            return (BAD);
        if (v->type == TYP_SCALAR) {
            res->content.value = v->content.value;
            v->content.value += 1.0;
            res->type = TYP_SCALAR;
            return (OK);
        }
        if (v->type == TYP_CMPLX) {
            res->content.cx = v->content.cx;
            v->content.cx.real += 1.0;
            res->type = TYP_CMPLX;
            return (OK);
        }
        if (v->type == TYP_HANDLE) {
            v->incr__handle(res);
            return (OK);
        }
    }
    else if (p->optype == TOK_RDECR) {
        Variable *v = p->left->getVar();
        if (!v)
            return (BAD);
        if (v->type == TYP_SCALAR) {
            res->content.value = v->content.value;
            v->content.value -= 1.0;
            res->type = TYP_SCALAR;
            return (OK);
        }
        if (v->type == TYP_CMPLX) {
            res->content.cx = v->content.cx;
            v->content.cx.real -= 1.0;
            res->type = TYP_CMPLX;
            return (OK);
        }
    }
    return (BAD);
}


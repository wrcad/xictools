
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
 * WRspice Circuit Simulation and Analysis Tool                           *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#include "spnumber.h"
struct ParseNode;
#include "spparse.h"
#include "lstring.h"
#include "graphics.h"

#ifdef WRSPICE
#include "frontend.h"
#include "ttyio.h"
#else
// Xic
#include "cd.h"
#endif


// Return true if the string matches a source function name: v, vm, vp,
// vr, vi, vdb and similar for i.
//
namespace {
    bool
    is_sourcename(const char *s)
    {
        if (!s || (*s != 'v' && *s != 'i' && *s != 'V' && *s != 'I'))
            return (false);
        s++;
        if (!*s)
            return (true);
        if (strchr("mpriMPRI", s[0]) && !s[1])
            return (true);
        if ((s[0] == 'd' || s[0] == 'D') && (s[1] == 'b' || s[1] == 'B') &&
                !s[2])
            return (true);
        return (false);
    }
}


bool Parser::Debug;

// Note:  the character used to separate fields in subcircuit
// expansion should not appear here, or there may be trouble.
//
const char *Parser::def_specials = " \t%()-^+*,/|&<>~=?!:";


Parser::Parser(Element *elem, unsigned int flags)
{
    prsr_string = 0;
    prsr_arg = 0;
    prsr_lastToken = TT_END;
    prsr_lastType = DT_BOGUS;
    prsr_bracFlag = false;
    prsr_specials = def_specials;
    prsr_el = elem;  // an array of size STACKSIZE
    prsr_stack = elem + 1;
    prsr_sptr = 0;
    prsr_errorCode = ER_OK;
    prsr_flags = flags;
    prsr_trinest = 0;
    prsr_in_source = false;
}


Parser::~Parser()
{
    delete [] prsr_el;
}


void
Parser::init(const char *str, void *arg)
{
    prsr_string = str;
    prsr_arg = arg;
    prsr_sptr = str;
    prsr_lastToken = TT_END;
    prsr_lastType = DT_BOGUS;
    prsr_bracFlag = false;
}

// The operator-precedence table.

#define G PRECgt    // Greater than
#define L PREClt    // Less than
#define E PRECeq    // Equal
#define R PRECerr   // Error

#define PREC_TABLE_SIZE 25

namespace {
const char prectable[PREC_TABLE_SIZE][PREC_TABLE_SIZE] = {
       /* $  +  -  *  %  /  ^  u- (  )  ,  :  ?  v  =  >  <  >= <= <> &  |  ~ IDX R */
/* $ */ { R, L, L, L, L, L, L, L, L, R, L, L, L, L, L, L, L, L, L, L, L, L, L, L, L },
/* + */ { G, G, G, L, L, L, L, L, L, G, G, G, G, L, G, G, G, G, G, G, G, G, G, L, L },
/* - */ { G, G, G, L, L, L, L, L, L, G, G, G, G, L, G, G, G, G, G, G, G, G, G, L, L },
/* * */ { G, G, G, G, G, G, L, L, L, G, G, G, G, L, G, G, G, G, G, G, G, G, G, L, L },
/* % */ { G, G, G, G, G, G, L, L, L, G, G, G, G, L, G, G, G, G, G, G, G, G, G, L, L },
/* / */ { G, G, G, G, G, G, L, L, L, G, G, G, G, L, G, G, G, G, G, G, G, G, G, L, L },
/* ^ */ { G, G, G, G, G, G, L, L, L, G, G, G, G, L, G, G, G, G, G, G, G, G, G, L, L },
/* u-*/ { G, G, G, G, G, G, G, L, L, G, G, G, G, L, G, G, G, G, G, G, G, G, G, L, L },
/* ( */ { R, L, L, L, L, L, L, L, L, E, L, L, L, L, L, L, L, L, L, L, L, L, L, L, L },
/* ) */ { G, G, G, G, G, G, G, G, R, G, G, G, G, R, G, G, G, G, G, G, G, G, G, G, G },
/* , */ { G, L, L, L, L, L, L, L, L, G, L, L, L, L, G, G, G, G, G, G, G, G, G, L, L },
/* : */ { G, L, L, L, L, L, L, L, L, G, L, R, L, L, L, L, L, L, L, L, L, L, L, L, L },
/* ? */ { G, L, L, L, L, L, L, L, L, G, L, L, R, L, L, L, L, L, L, L, L, L, L, L, L },
/* v */ { G, G, G, G, G, G, G, G, G, G, G, G, G, R, G, G, G, G, G, G, G, G, G, G, G },
/* = */ { G, L, L, L, L, L, L, L, L, G, L, G, G, L, G, G, G, G, G, G, G, G, L, L, L },
/* > */ { G, L, L, L, L, L, L, L, L, G, L, G, G, L, G, G, G, G, G, G, G, G, L, L, L },
/* < */ { G, L, L, L, L, L, L, L, L, G, L, G, G, L, G, G, G, G, G, G, G, G, L, L, L },
/* >=*/ { G, L, L, L, L, L, L, L, L, G, L, G, G, L, G, G, G, G, G, G, G, G, L, L, L },
/* <=*/ { G, L, L, L, L, L, L, L, L, G, L, G, G, L, G, G, G, G, G, G, G, G, L, L, L },
/* <>*/ { G, L, L, L, L, L, L, L, L, G, L, G, G, L, G, G, G, G, G, G, G, G, L, L, L },
/* & */ { G, L, L, L, L, L, L, L, L, G, L, G, G, L, L, L, L, L, L, L, G, G, L, L, L },
/* | */ { G, L, L, L, L, L, L, L, L, G, L, G, G, L, L, L, L, L, L, L, L, G, L, L, L },
/* ~ */ { G, L, L, L, L, L, L, L, L, G, L, G, G, L, G, G, G, G, G, G, G, G, G, L, L },
/*INDX*/{ G, G, G, G, G, G, G, G, L, G, G, G, G, L, G, G, G, G, G, G, G, G, G, G, L },
/*RAN*/ { G, G, G, G, G, G, G, G, L, G, G, G, G, L, G, G, G, G, G, G, G, G, G, G, G }
};

// The following two tables are not used by the parser, but are
// exported for use when printing a parse tree, for parentheses
// suppression.  When printing an expression from a parse tree, the
// safest thing to do is surround all binary operator nodes with
// parentheses, as (a binop b).  This will enforce the presedence
// build into the tree, however it produces very messy-looking
// expressions that are hard to read.  Thus, we try to reduce
// parentheses usage.
//
// The parenTable method, and the two tables below, are designed to
// accomplish this.  Printing of a binop node would have the following
// logic:
//
//  node::print(NodeType parent_type, bool LorR)
//  {
//      if (node is a binop) {
//          int prn = LorR == LEFT ? left_ptable[type()][parent_type] :
//              right_table[type()][parent_type];
//          if (prn == L)
//              printf("(");
//          left_node->print(type(), LEFT);
//          printf("%s", token());
//          right_node->print(type(), RIGHT);
//          if (prn == L)
//              printf(")");
//      }
//      ...
//  }

// The parent_type indexes the columns, the child type indexes the rows.
// In the "left" table, the entries are:
//
//   L if '(a type b) parent_type c' != 'a type b parent_type c', or to
//   enforce parens as in the LHS, G if equal or to suppress parens, R
//   if syntax bad or non-binop.
//
const char left_ptable[PREC_TABLE_SIZE][PREC_TABLE_SIZE] = {
       /* $  +  -  *  %  /  ^  u- (  )  ,  :  ?  v  =  >  <  >= <= <> &  |  ~ IDX R */
/* $ */ { R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R },
/* + */ { R, G, G, L, L, L, L, L, R, R, G, G, G, R, G, G, G, G, G, G, G, G, L, L, L },
/* - */ { R, G, G, L, L, L, L, L, R, R, G, G, G, R, G, G, G, G, G, G, G, G, L, L, L },
/* * */ { R, G, G, G, G, G, L, L, R, R, G, G, G, R, G, G, G, G, G, G, G, G, L, L, L },
/* % */ { R, G, G, G, G, G, L, L, R, R, G, G, G, R, G, G, G, G, G, G, G, G, L, L, L },
/* / */ { R, G, G, G, G, G, L, L, R, R, G, G, G, R, G, G, G, G, G, G, G, G, L, L, L },
/* ^ */ { R, G, G, G, G, G, L, L, R, R, G, G, G, R, G, G, G, G, G, G, G, G, L, L, L },
/* u-*/ { R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R },
/* ( */ { R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R },
/* ) */ { R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R },
/* , */ { R, L, L, L, L, L, L, L, R, R, G, L, L, R, G, G, G, G, G, G, G, G, R, G, G },
/* : */ { R, L, L, L, L, L, L, R, R, R, L, R, L, R, L, L, L, L, L, L, L, L, R, L, L },
/* ? */ { R, L, L, L, L, L, L, R, R, R, L, L, R, R, L, L, L, L, L, L, L, L, R, L, L },
/* v */ { R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R },
/* = */ { R, L, L, L, L, L, L, L, R, R, L, G, G, R, L, L, L, L, L, L, G, G, L, L, L },
/* > */ { R, L, L, L, L, L, L, L, R, R, L, G, G, R, L, L, L, L, L, L, G, G, L, L, L },
/* < */ { R, L, L, L, L, L, L, L, R, R, L, G, G, R, L, L, L, L, L, L, G, G, L, L, L },
/* >=*/ { R, L, L, L, L, L, L, L, R, R, L, G, G, R, L, L, L, L, L, L, G, G, L, L, L },
/* <=*/ { R, L, L, L, L, L, L, L, R, R, L, G, G, R, L, L, L, L, L, L, G, G, L, L, L },
/* <>*/ { R, L, L, L, L, L, L, L, R, R, L, G, G, R, L, L, L, L, L, L, G, G, L, L, L },
/* & */ { R, L, L, L, L, L, L, L, R, R, L, G, G, R, L, L, L, L, L, L, G, G, L, L, L },
/* | */ { R, L, L, L, L, L, L, L, R, R, L, G, G, R, L, L, L, L, L, L, L, G, L, L, L },
/* ~ */ { R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, L, R, R },
/*INDX*/{ R, G, G, G, G, G, G, R, R, R, G, G, G, R, G, G, G, G, G, G, G, G, R, G, L },
/*RAN*/ { R, G, G, G, G, G, G, R, R, R, G, G, G, R, G, G, G, G, G, G, G, G, R, G, G },
};

// In the "right" table, the entries are:
//
//   L if 'a parent_type (b type c)' != 'a parent_type b type c', or to
//   enforce parens as in the LHS, G if equal or to suppress parens, R
//   if syntax bad or non-binop.
//
const char right_ptable[PREC_TABLE_SIZE][PREC_TABLE_SIZE] = {
       /* $  +  -  *  %  /  ^  u- (  )  ,  :  ?  v  =  >  <  >= <= <> &  |  ~ IDX R */
/* $ */ { R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R },
/* + */ { R, G, L, L, L, L, L, L, R, R, G, G, G, R, G, G, G, G, G, G, G, G, L, L, L },
/* - */ { R, G, L, L, L, L, L, L, R, R, G, G, G, R, G, G, G, G, G, G, G, G, L, L, L },
/* * */ { R, G, G, G, L, L, L, L, R, R, G, G, G, R, G, G, G, G, G, G, G, G, L, L, L },
/* % */ { R, G, G, L, L, L, L, L, R, R, G, G, G, R, G, G, G, G, G, G, G, G, L, L, L },
/* / */ { R, G, G, L, L, L, L, L, R, R, G, G, G, R, G, G, G, G, G, G, G, G, L, L, L },
/* ^ */ { R, G, G, G, G, G, L, L, R, R, G, G, G, R, G, G, G, G, G, G, G, G, L, L, L },
/* u-*/ { R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R },
/* ( */ { R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R },
/* ) */ { R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R },
/* , */ { R, L, L, L, L, L, L, L, R, R, G, G, G, R, G, G, G, G, G, G, G, G, R, G, G },
/* : */ { R, L, L, L, L, L, L, R, R, R, L, R, L, R, L, L, L, L, L, L, L, L, R, L, L },
/* ? */ { R, L, L, L, L, L, L, R, R, R, L, L, R, R, L, L, L, L, L, L, L, L, R, L, L },
/* v */ { R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R },
/* = */ { R, L, L, L, L, L, L, L, R, R, L, G, G, R, L, L, L, L, L, L, G, G, L, L, L },
/* > */ { R, L, L, L, L, L, L, L, R, R, L, G, G, R, L, L, L, L, L, L, G, G, L, L, L },
/* < */ { R, L, L, L, L, L, L, L, R, R, L, G, G, R, L, L, L, L, L, L, G, G, L, L, L },
/* >=*/ { R, L, L, L, L, L, L, L, R, R, L, G, G, R, L, L, L, L, L, L, G, G, L, L, L },
/* <=*/ { R, L, L, L, L, L, L, L, R, R, L, G, G, R, L, L, L, L, L, L, G, G, L, L, L },
/* <>*/ { R, L, L, L, L, L, L, L, R, R, L, G, G, R, L, L, L, L, L, L, G, G, L, L, L },
/* & */ { R, L, L, L, L, L, L, L, R, R, L, G, G, R, L, L, L, L, L, L, G, G, L, L, L },
/* | */ { R, L, L, L, L, L, L, L, R, R, L, G, G, R, L, L, L, L, L, L, L, G, L, L, L },
/* ~ */ { R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R },
/*INDX*/{ R, G, G, G, G, G, G, R, R, R, G, G, G, R, G, G, G, G, G, G, G, G, R, G, L },
/*RAN*/ { R, G, G, G, G, G, G, R, R, R, G, G, G, R, G, G, G, G, G, G, G, G, R, G, G },
};
}


// Static functon.
//
// This is an exported helper function used for determining when to
// print binop parentheses when printing parse trees.  If true is
// returned, then the parentheses should be printed.  See the tables
// and comments above.
//
bool
Parser::parenTable(TokenType type, TokenType parent_type, bool right)
{
    if (type == TT_PLACEHOLDER || parent_type == TT_PLACEHOLDER)
        return (false);
    if (right)
        return (right_ptable[type][parent_type] == PREClt);
    return (left_ptable[type][parent_type] == PREClt);
}


#ifdef WRSPICE
namespace {
    const char *token_string(TokenType token)
    {
        switch (token) {
        case TT_END:
            return ("end");
        case TT_PLUS:
            return ("plus");
        case TT_MINUS:
            return ("minus");
        case TT_TIMES:
            return ("times");
        case TT_MOD:
            return ("mod");
        case TT_DIVIDE:
            return ("divide");
        case TT_POWER:
            return ("power");
        case TT_UMINUS:
            return ("unary minus");
        case TT_LPAREN:
            return ("left paren");
        case TT_RPAREN:
            return ("right paren");
        case TT_COMMA:
            return ("comma");
        case TT_COLON:
            return ("colon");
        case TT_COND:
            return ("cond");
        case TT_VALUE:
            return ("value");
        case TT_EQ:
            return ("equal");
        case TT_GT:
            return ("greater than");
        case TT_LT:
            return ("less than");
        case TT_GE:
            return ("greater or equal");
        case TT_LE:
            return ("less or equal");
        case TT_NE:
            return ("not equal");
        case TT_AND:
            return ("and");
        case TT_OR:
            return ("or");
        case TT_NOT:
            return ("not");
        case TT_INDX:
            return ("indx");
        case TT_RANGE:
            return ("range");
        case TT_PLACEHOLDER:
            return ("place holder");
        }
        return ("unknown");
    }
}
#endif


ParseNode *
Parser::parse()
{
    if (!prsr_sptr)
        return (0);
    prsr_trinest = 0;
    prsr_in_source = false;
    prsr_errorCode = ER_OK;
    prsr_stack[0].token = TT_END;
    Element *next = lexer();
    if (next->token == TT_END) {
        prsr_errorCode = ER_BADNOD;
        return (0);
    }
    ParseNode *pn;
    int sp = 0;
    while (sp > 1 || next->token != TT_END) {
        // Find the top-most terminal
        int i = sp, st;
        Element *top;
        do {
            top = &prsr_stack[i--];
        } while (top->token == TT_VALUE);
        char rel = prectable[top->token][next->token];
        switch (rel) {
        case L:
        case E:
            // Push the token read
            if (sp == STACKSIZE - 2) {
                // stack overflow
                prsr_errorCode = ER_OVERF;
                clear_stack(sp);
                return (0);
            }
            prsr_stack[++sp] = *next;
            next = lexer();
            continue;

        case R:
            if (next->token == TT_RPAREN && sp == 1 &&
                    prsr_stack[sp].token == TT_VALUE) {
                // accept "number )" as number
                prsr_sptr--;  // push back ')'
                pn = prsr_stack[1].makeNode(prsr_arg);
                if (pn)
                    return (pn);
            }
            prsr_errorCode = ER_SYNTAX;
            clear_stack(sp);
            return (0);

        case G:
            // Reduce. Make st and sp point to the elts on the
            // stack at the end and beginning of the junk to
            // reduce, then try and do some stuff. When scanning
            // back for a <, ignore VALUES.
            //
            st = sp;
            if (prsr_stack[sp].token == TT_VALUE)
                sp--;
            while (sp > 0) {
                if (prsr_stack[sp - 1].token == TT_VALUE)
                    i = 2;  // No 2 ParseNode together...
                else
                    i = 1;
                if (prectable[prsr_stack[sp-i].token]
                        [prsr_stack[sp].token] == L)
                    break;
                else
                    sp = sp - i;
            }
            if (prsr_stack[sp - 1].token == TT_VALUE)
                sp--;
            // Now try and see what we can make of this.
            // The possibilities are: unop node
            //            node op node
            //            ( node )
            //            func ( node )
            //            node
            //  node [ node ] is considered node op node.
            //
            if (st == sp) {
                pn = prsr_stack[st].makeNode(prsr_arg);
                if (pn == 0) {
                    prsr_errorCode = ER_BADNOD;
                    clear_stack(sp);
                    return (0);
                }
            }
            else if ((prsr_stack[sp].token == TT_UMINUS ||
                    prsr_stack[sp].token == TT_NOT) && st == sp + 1) {
                ParseNode *lpn = prsr_stack[st].makeNode(prsr_arg);
                if (lpn == 0) {
                    prsr_errorCode = ER_BADNOD;
                    clear_stack(sp);
                    return (0);
                }
                pn = prsr_stack[sp].makeUnode(lpn, prsr_arg);
            }
            else if (prsr_stack[sp].token == TT_LPAREN &&
                    prsr_stack[st].token == TT_RPAREN) {
                pn = prsr_stack[sp + 1].makeNode(prsr_arg);
                if (pn == 0) {
                    prsr_errorCode = ER_BADNOD;
                    clear_stack(sp);
                    return (0);
                }
            }
            else if (prsr_stack[sp+1].token == TT_LPAREN &&
                    prsr_stack[st].token == TT_RPAREN) {
                ParseNode *lpn = prsr_stack[sp + 2].makeNode(prsr_arg);
                if ((lpn == 0 && prsr_stack[sp + 2].token != TT_RPAREN) ||
                        (prsr_stack[sp].type != DT_STRING &&
                            prsr_stack[sp].type != DT_USTRING)) {
                    prsr_errorCode = ER_BADNOD;
                    clear_stack(sp);
                    return (0);
                }
                pn = prsr_stack[sp].makeFnode(lpn, prsr_arg);
                if (pn == 0) {
                    prsr_errorCode = ER_BADNOD;
                    clear_stack(sp);
                    return (0);
                }
            }
            else { // node op node
                ParseNode *lpn = prsr_stack[sp].makeNode(prsr_arg);
                ParseNode *rpn = prsr_stack[st].makeNode(prsr_arg);
                if (lpn == 0 || rpn == 0) {
                    prsr_errorCode = ER_BADNOD;
                    clear_stack(sp);
                    return (0);
                }
                pn = prsr_stack[sp + 1].makeBnode(lpn, rpn, prsr_arg);
                if (pn == 0) {
                    prsr_errorCode = ER_BADNOD;
                    clear_stack(sp);
                    return (0);
                }
            }
            prsr_stack[sp].token = TT_VALUE;
            prsr_stack[sp].type = DT_PNODE;
            prsr_stack[sp].vu.node = pn;
            continue;
        }
    }
    pn = prsr_stack[1].makeNode(prsr_arg);
    if (pn)
        return (pn);
    prsr_errorCode = ER_BADNOD;
    return (0);
}


#ifdef WRSPICE
#define SUBC_CATCHAR() Sp.SubcCatchar()
#define SPEC_CATCHAR() Sp.SpecCatchar()
#define PLOT_CATCHAR() Sp.PlotCatchar()
#else
#define SUBC_CATCHAR() CD()->GetSubcCatchar()
#define SPEC_CATCHAR() '@'
#define PLOT_CATCHAR() '.'
#endif

// Everything else is a string or a number. Quoted strings are kept in 
// the form "string", and the lexer strips off the quotes...
//
Element *
Parser::lexer()
{
    prsr_el->clear();
    if (prsr_bracFlag) {
        prsr_bracFlag = false;
        prsr_el->token = TT_LPAREN;
        finish();
        return (prsr_el);
    }

    while (*prsr_sptr == ' ' || *prsr_sptr == '\t')
        prsr_sptr++;

    switch (*prsr_sptr) {

    case 0:
        // done
        finish();
        return (prsr_el);

    case ';':
        // done, advance
        prsr_sptr++;
        finish();
        return (prsr_el);

    case '-':
        if (prsr_lastToken == TT_VALUE || prsr_lastToken == TT_RPAREN)
            prsr_el->token = TT_MINUS;
        else
            prsr_el->token = TT_UMINUS;
        prsr_sptr++;
        break;

    case '+':
        prsr_el->token = TT_PLUS; 
        prsr_sptr++;
        break;

    case ',':
        prsr_el->token = TT_COMMA;
        prsr_sptr++;
        break;

    case '*':
        // accept '**' as power (same as '^')
        if (prsr_sptr[1] == '*') {
            prsr_el->token = TT_POWER; 
            prsr_sptr++;
        }
        else
            prsr_el->token = TT_TIMES; 
        prsr_sptr++;
        break;

    case '%':
        prsr_el->token = TT_MOD; 
        prsr_sptr++;
        break;

    case '/':
        prsr_el->token = TT_DIVIDE; 
        prsr_sptr++;
        break;

    case '^':
        prsr_el->token = TT_POWER; 
        prsr_sptr++;
        break;

    case '[':
        if (prsr_sptr[1] == '[') {
            prsr_el->token = TT_RANGE;
            prsr_sptr += 2;
        }
        else {
            prsr_el->token = TT_INDX;
            prsr_sptr++;
        }
        prsr_bracFlag = true;
        break;

    case '(':
        if ((prsr_lastToken == TT_VALUE && prsr_lastType == DT_NUM) ||
                prsr_lastToken == TT_RPAREN) {
            prsr_el->clear();
            finish();
            return (prsr_el);
        }
        else {
            prsr_el->token = TT_LPAREN; 
            prsr_sptr++;
            break;
        }

    case ']':
        prsr_el->token = TT_RPAREN; 
        if (prsr_sptr[1] == ']')
            prsr_sptr += 2;
        else
            prsr_sptr++;
        break;

    case ')':
        // The "source" functions v(), i(), etc.  take node/source
        // names which can be really strange (at the least, they
        // contain ':'s from subcircuit expansion), so we set a flag
        // when parsing arguments to these for special treatment.  We
        // know that the call is over when we get a ')' here;
        //
        prsr_in_source = false;

        prsr_el->token = TT_RPAREN; 
        prsr_sptr++;
        break;

    case '=':
        prsr_el->token = TT_EQ;
        // Support '==', same as '='.
        if (*(prsr_sptr+1) == '=')
            prsr_sptr++;
        prsr_sptr++;
        break;

    // Support ! (same as ~) and !=.
    case '!':
        if (*(prsr_sptr+1) == '=') {
            prsr_el->token = TT_NE;
            prsr_sptr += 2;
        }
        else {
            prsr_el->token = TT_NOT;
            prsr_sptr++;
        }
        break;

    case '>':
    case '<':
        {
            int j;
            for (j = 1; isspace(prsr_sptr[j]); j++) ;
            // The lexer makes <> into < >
            if ((prsr_sptr[j] == '<' || prsr_sptr[j] == '>') &&
                    prsr_sptr[0] != prsr_sptr[j]) {
                // Allow both <> and >< for TT_NE
                prsr_el->token = TT_NE;
                prsr_sptr += 2 + j;
            }
            else if (prsr_sptr[1] == '=') {
                if (prsr_sptr[0] == '>')
                    prsr_el->token = TT_GE;
                else
                    prsr_el->token = TT_LE;
                prsr_sptr += 2;
            }
            else {
                if (prsr_sptr[0] == '>')
                    prsr_el->token = TT_GT;
                else
                    prsr_el->token = TT_LT;
                prsr_sptr++;
            }
        }
        break;

    case '&':
        prsr_el->token = TT_AND;
        // Support &&, same as &.
        if (*(prsr_sptr+1) == '&')
            prsr_sptr++;
        prsr_sptr++;
        break;

    case '|':
        prsr_el->token = TT_OR;
        // Support ||, same as |.
        if (*(prsr_sptr+1) == '|')
            prsr_sptr++;
        prsr_sptr++;
        break;

    case '~':
        prsr_el->token = TT_NOT;
        prsr_sptr++;
        break;

    case '?':
        prsr_trinest++;
        prsr_el->token = TT_COND;
        prsr_sptr++;
        break;

    case ':':
        if (prsr_trinest)
            prsr_trinest--;
        prsr_el->token = TT_COLON;
        prsr_sptr++;
        break;

    case '"':
        if (prsr_lastToken == TT_VALUE || prsr_lastToken == TT_RPAREN) {
            prsr_el->clear();
            finish();
            return (prsr_el);
        }
        prsr_el->token = TT_VALUE;
        prsr_el->vu.string = lstring::copy(++prsr_sptr);
        {
            char *s;
            for (s = prsr_el->vu.string; *s && (*s != '"'); s++, prsr_sptr++) ;
            *s = '\0';
        }
        prsr_sptr++;
        {
            const char *s = prsr_el->vu.string;
            char *t = (prsr_flags & PRSR_USRSTR) ?
                prsr_el->userString(&s, (prsr_flags & PRSR_SOURCE)) : 0;
            if (t) {
                prsr_el->type = DT_USTRING;
                delete [] t;
            }
            else
                prsr_el->type = DT_STRING;
        }
        break;

    case '\'':
        // delimiter for expression
        prsr_sptr++;
        prsr_el->clear();
        finish();
        return (prsr_el);
    }

    if (prsr_el->token != TT_END) {
        finish();
        return (prsr_el);
    }
    
    if (prsr_in_source) {
        // We're parsing the arguments of a source function.  The
        // tokens can be almost anything (node names), so we take
        // whatever we find, possibly separated with commas, as
        // strings.

        handle_string();
        prsr_el->token = TT_VALUE;
#ifdef WRSPICE
        if (Debug)
            GRpkgIf()->ErrPrintf(ET_MSGS, "lexer: string %s\n",
                prsr_el->vu.string);
#endif
        finish();
        return (prsr_el);
    }

    if ((prsr_flags & PRSR_NODEHACK) && SUBC_CATCHAR() != '.') {
        // If the token is an integer followed by Sp.SubcCatchar(), we
        // assume that this is the start of a string token for a
        // node name.  Can't do this if the catchar is '.' for obvious
        // reasons.
        //
        // This allows node names to be recognized without the enclosing
        // v( ).

        // This should go away, as it is unreliable.  We can't really
        // know where the token ends, it will be broken at any of the
        // specials.  Operations like "plot node node ..." will work
        // with arbitrary non-numeric node names (VecGet() succeeds)
        // anyway.

        const char *ss = prsr_sptr;
        for ( ; *ss; ss++) {
            if (!isdigit(*ss))
                break;
        }
        if (*ss == SUBC_CATCHAR() && ss - prsr_sptr > 0) {
            handle_string();
            prsr_el->token = TT_VALUE;
#ifdef WRSPICE
            if (Debug)
                GRpkgIf()->ErrPrintf(ET_MSGS, "lexer: string %s\n",
                    prsr_el->vu.string);
#endif
            finish();
            return (prsr_el);
        }
    }

    const char *ss = prsr_sptr;
    double *td = SPnum.parse(&ss, false, true,
        (prsr_flags & PRSR_UNITS) ? &prsr_el->units : 0);
    if (td) {
        if (prsr_lastToken == TT_VALUE || prsr_lastToken == TT_RPAREN)
            prsr_el->clear();
        else {
            prsr_el->vu.real = *td;
            prsr_el->type = DT_NUM;
            prsr_el->token = TT_VALUE;
            prsr_sptr = (char*)ss;
#ifdef WRSPICE
            if (Debug)
                GRpkgIf()->ErrPrintf(ET_MSGS,
                    "lexer: double %G\n", prsr_el->vu.real);
#endif
        }
        finish();
        return (prsr_el);
    }

    if (prsr_lastToken == TT_VALUE || prsr_lastToken == TT_RPAREN) {

        // Check for relational and logical equivalents, these can
        // only occur after an lvalue.

        if (prsr_sptr[0] == 'g' && prsr_sptr[1] == 't' &&
                is_special(prsr_sptr[2])) {
            prsr_el->token = TT_GT;
            prsr_sptr += 2;
        }
        else if (prsr_sptr[0] == 'l' && prsr_sptr[1] == 't' &&
                is_special(prsr_sptr[2])) {
            prsr_el->token = TT_LT;
            prsr_sptr += 2;
        }
        else if (prsr_sptr[0] == 'g' && prsr_sptr[1] == 'e' &&
                is_special(prsr_sptr[2])) {
            prsr_el->token = TT_GE;
            prsr_sptr += 2;
        }
        else if (prsr_sptr[0] == 'l' && prsr_sptr[1] == 'e' &&
                is_special(prsr_sptr[2])) {
            prsr_el->token = TT_LE;
            prsr_sptr += 2;
        }
        else if (prsr_sptr[0] == 'n' && prsr_sptr[1] == 'e' &&
                is_special(prsr_sptr[2])) {
            prsr_el->token = TT_NE;
            prsr_sptr += 2;
        }
        else if (prsr_sptr[0] == 'e' && prsr_sptr[1] == 'q' &&
                is_special(prsr_sptr[2])) {
            prsr_el->token = TT_EQ;
            prsr_sptr += 2;
        }
        else if (prsr_sptr[0] == 'o' && prsr_sptr[1] == 'r' &&
                is_special(prsr_sptr[2])) {
            prsr_el->token = TT_OR;
            prsr_sptr += 2;
        }
        else if (prsr_sptr[0] == 'a' && prsr_sptr[1] == 'n' &&
                prsr_sptr[2] == 'd' && is_special(prsr_sptr[3])) {
            prsr_el->token = TT_AND;
            prsr_sptr += 3;
        }
        else {
            prsr_el->clear();
            finish();
            return (prsr_el);
        }
    }
    else {
        if (prsr_sptr[0] == 'n' && prsr_sptr[1] == 'o' &&
                prsr_sptr[2] == 't' && is_special(prsr_sptr[3])) {
            prsr_el->token = TT_NOT;
            prsr_sptr += 3;
        }

        // This enables catching oddball things, like 'tran'
        // functions.
        char *s = (prsr_flags & PRSR_USRSTR) ?
            prsr_el->userString(&prsr_sptr, (prsr_flags & PRSR_SOURCE)) : 0;
        if (s) {
            prsr_el->vu.string = s;
            prsr_el->type = DT_USTRING;
        }
        else
            handle_string();
        prsr_el->token = TT_VALUE;
#ifdef WRSPICE
        if (Debug)
            GRpkgIf()->ErrPrintf(ET_MSGS, "lexer: string %s\n",
                prsr_el->vu.string);
#endif

        // Set a flag if the is the start of a "source function" call.
        if (is_sourcename(prsr_el->vu.string)) {
            const char *ts = prsr_sptr;
            while (isspace(*ts))
                ts++;
            if (*ts == '(')
                prsr_in_source = true;
        }
    }
    finish();
    return (prsr_el);
}


const char *
Parser::getErrMesg()
{
    if (prsr_errorCode == ER_OVERF)
        return ("stack overflow");
    if (prsr_errorCode == ER_SYNTAX)
        return ("bad syntax");
    if (prsr_errorCode == ER_BADNOD)
        return ("bad node");
    return ("ok");
}


void
Parser::finish()
{
    prsr_lastToken = prsr_el->token;
    prsr_lastType = prsr_el->type;
#ifdef WRSPICE
    if (Debug)
        GRpkgIf()->ErrPrintf(ET_MSGS, "lexer: token %d (%s)\n",
            prsr_el->token, token_string(prsr_el->token));
#endif
}


void
Parser::handle_string()
{
    const char *s = prsr_sptr;
    if (prsr_in_source) {
        // We are parsing args to a "source function" (v(), i(), etc.)
        // call.  The tokens are node or source names, which can
        // have special characters.

        for ( ; *s && !isspace(*s) && *s != ',' && *s != ')'; s++) ;
    }
    else {
        // It is bad how we have to recognise '[' -- sometimes it is
        // part of a word, when it defines a parameter name, and
        // otherwise it isn't.  If AMPR_HACK is not set, then include
        // [stuff] with the string token and let the caller deal with
        // it.  Otherwise, need to catch @xxx[yyy], zzz.@xxx[yyy], where
        // @ = Sp.SpecCatchar(), . = Sp.PlotCatchar().

        const char *first_colon = 0;
        bool ampr = (*s == SPEC_CATCHAR());
        for ( ; ; s++) {
            if (!*s)
                break;
            if (*s == ']') {
                s++;
                break;
            }
            if (*s == ':') {
                if (prsr_trinest <= 0)
                    continue;
                // Lots of trouble here - this could be part of a name,
                // or the operator.  The user will need to use double
                // quoting.
                // It he token ends with "#branch", keep the colon.
                // Otherwise, assume it is the operator.
                if (!first_colon)
                    first_colon = s;
                continue;
            }
            if (is_special(*s))
                break;
            if (*s == '#') {
                if (lstring::ciprefix("branch", s+1)) {
                    s += 7;
                    if (!*s || is_special(*s) || *s == ':' || *s == '[') {
                        // If the token ends this way, the embedded
                        // colon is taken as part of the name.
                        first_colon = 0;
                        break;
                    }
                }
                continue;
            }
            if (prsr_flags & PRSR_AMPHACK) {
                if (!ampr && *s == SPEC_CATCHAR() && *(s-1) == PLOT_CATCHAR())
                    ampr = true;
                else if (*s == '[' && !ampr)
                    break;
            }
        }

        // If we have a colon here, take is as the operator.
        if (first_colon)
            s = first_colon;
    }
    int nc = s - prsr_sptr;
    prsr_el->vu.string = new char[nc+1];
    strncpy(prsr_el->vu.string, prsr_sptr, nc);
    prsr_el->vu.string[nc] = 0;
    prsr_el->type = DT_STRING;
    prsr_sptr += nc;
}


void
Parser::clear_stack(int sp)
{
    stringlist *sl0 = 0;
    while (sp) {
        if (prsr_stack[sp].type == DT_STRING ||
                prsr_stack[sp].type == DT_USTRING) {
            stringlist *sl;
            for (sl = sl0; sl; sl = sl->next) {
                if (sl->string == prsr_stack[sp].vu.string)
                    break;
            }
            if (!sl)
                sl0 = new stringlist(prsr_stack[sp].vu.string, sl0);
        }
        sp--;
    }
    stringlist::destroy(sl0);
}


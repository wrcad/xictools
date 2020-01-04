
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
 * vl -- Verilog Simulator and Verilog support library.                   *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

/*========================================================================*
  Copyright (c) 1992, 1993
        Regents of the University of California
  All rights reserved.

  Use and copying of this software and preparation of derivative works
  based upon this software are permitted.  However, any distribution of
  this software or derivative works must include the above copyright 
  notice.

  This software is made available AS IS, and neither the Electronics
  Research Laboratory or the Universify of California make any
  warranty about the software, its performance or its conformity to
  any specification.

  Author: Szu-Tsung Cheng, stcheng@ic.Berkeley.EDU
          10/92
          10/93
 *========================================================================*/

#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>
#include <stdarg.h>
#include "vl_st.h"
#include "vl_list.h"
#include "vl_defs.h"
#include "vl_types.h"

#ifndef VL_VERSION
#define VL_VERSION "0.0"
#endif

#define VL_REVISION \
"vl-" VL_VERSION " Verilog Parser-Simulator, Whiteley Research Inc. (C) 2020."


//---------------------------------------------------------------------------
//  Exports
//---------------------------------------------------------------------------

namespace vl {

void vl_error(const char *fmt, ...)
{
    fflush(stdout);
    va_list args;
    char buf[MAXSTRLEN];
    va_start(args, fmt);
    vsnprintf(buf, MAXSTRLEN, fmt, args);
    va_end(args);

    cerr << "* Error: " << buf << ".\n";
}


void vl_warn(const char *fmt, ...)
{
    fflush(stdout);
    va_list args;
    char buf[MAXSTRLEN];
    va_start(args, fmt);
    vsnprintf(buf, MAXSTRLEN, fmt, args);
    va_end(args);

    cerr << "* Warning: " << buf << ".\n";
}


// Return the date. Return value is static data.
//
const char *vl_datestring()
{
    time_t tloc;
    time(&tloc);
    struct tm *tp = localtime(&tloc);
    char *ap = asctime(tp);

    static char tbuf[40];
    strcpy(tbuf,ap);
    int i = strlen(tbuf);
    tbuf[i - 1] = '\0';
    return (tbuf);
}


const char * vl_version()
{
    return (VL_REVISION);
}


// Strip quotes, substitute for escapes in string.  Returns a copy of
// the string.
//
char *vl_fix_str(const char *str)
{
    if (!str)
        str = "(nil)";
    if (*str == '"')
         str++;
    char *nstr = new char[strlen(str) + 1];
    const char *s = str;
    char *t = nstr;
    while (*s) {
        if (*s == '\\') {
            if (*(s+1) == 'n') {
                *t++ = '\n';
                s += 2;
                continue;
            }
            if (*(s+1) == 't') {
                *t++ = '\t';
                s += 2;
                continue;
            }
            if (*(s+1) == '\\') {
                *t++ = '\\';
                s += 2;
                continue;
            }
        }
        *t++ = *s++;
    }
    *t = 0;
    if (*(t-1) == '"')
        *(t-1) = 0;
    return (nstr);
}


// Global string copy function.
//
char *vl_strdup(const char *str)
{
    if (!str)
        return (0);
    char *retval = new char[strlen(str) + 1];
    strcpy(retval, str);
    return (retval);
}

} // namespace vl


//---------------------------------------------------------------------------
//  Local
//---------------------------------------------------------------------------

template<class T> ostream &
operator<<(ostream &outs, lsList<T> *exprs)
{
    lsGen<T> gen(exprs);
    T expr;
    if (gen.next(&expr)) {
        outs << expr;
        while(gen.next(&expr))
            outs << ", " << expr;
    }
    return (outs);
}


template<class T> void
vl_print_items(ostream &outs, lsList<T> *exprs)
{
    lsGen<T> gen(exprs);
    T expr;
    while (gen.next(&expr))
        outs << expr << expr->lterm();
}


namespace {
    int IndentLevel;
    bool IndentSkip;

    void Indent(ostream &outs)
    {
        if (!IndentSkip) {
            for (int i = 0; i < 4*IndentLevel; i++)
                outs << ' ';
        }
        else
            IndentSkip = false;
    }


    char hexc[] = {
        '0', '1', '2', '3', '4', '5', '6', '7',
        '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
    };

    void printbits(ostream &outs, char *bits, int width, DSPtype dtype = DSPh)
    {
        if (width == 1)
            dtype = DSPb;
        char buf[256], tb[4];
        int num = 0;
        char cfmt;
        if (dtype == DSPb) {
            // binary format;
            cfmt = 'b';
            int i = 0;
            while (i < width) {
                if (bits[i] == BitZ)
                    buf[num++] = 'z';
                else if (bits[i] == BitDC)
                    buf[num++] = 'x';
                else if (bits[i] == BitH)
                    buf[num++] = '1';
                else if (bits[i] == BitL)
                    buf[num++] = '0';
                i++;
            }
        }
        else if (dtype == DSPh || dtype == DSPall) {
            // hex format;
            cfmt = 'h';
            int i = 0;
            while (i < width) {
                int j;
                for (j = 0; j < 4; j++, i++)
                    if (i < width)
                        tb[j] = bits[i];
                    else
                        tb[j] = BitL;
                if (tb[0] == BitZ || tb[1] == BitZ ||
                        tb[2] == BitZ || tb[3] == BitZ) {
                    if (tb[0] != tb[1] || tb[1] != tb[2] || tb[2] != tb[3]) {
                        printbits(outs, bits, width, DSPb);
                        return;
                    }
                    else
                        buf[num++] = 'z';
                }
                else if (tb[0] == BitDC || tb[1] == BitDC ||
                        tb[2] == BitDC || tb[3] == BitDC) {
                    if (tb[0] != tb[1] || tb[1] != tb[2] || tb[2] != tb[3]) {
                        printbits(outs, bits, width, DSPb);
                        return;
                    }
                    else
                        buf[num++] = 'x';
                }
                else {
                    int x = 0;
                    unsigned mask = 1;
                    for (j = 0; j < 4; j++) {
                        if (tb[j] == BitH)
                            x |= mask;
                        mask <<= 1;
                    }
                    buf[num++] = hexc[x];
                }
            }
        }
        else if (dtype == DSPo) {
            // octal format
            cfmt = 'o';
            int i = 0;
            while (i < width) {
                int j;
                for (j = 0; j < 3; j++, i++)
                    if (i < width)
                        tb[j] = bits[i];
                    else
                        tb[j] = BitL;
                if (tb[0] == BitZ || tb[1] == BitZ || tb[2] == BitZ) {
                    if (tb[0] != tb[1] || tb[1] != tb[2]) {
                        printbits(outs, bits, width, DSPb);
                        return;
                    }
                    else
                        buf[num++] = 'z';
                }
                else if (tb[0] == BitDC || tb[1] == BitDC || tb[2] == BitDC) {
                    if (tb[0] != tb[1] || tb[1] != tb[2]) {
                        printbits(outs, bits, width, DSPb);
                        return;
                    }
                    else
                        buf[num++] = 'x';
                }
                int x = 0;
                unsigned mask = 1;
                for (j = 0; j < 3; j++) {
                    if (tb[j] == BitH)
                        x |= mask;
                    mask <<= 1;
                }
                buf[num++] = hexc[x];
            }
        }
        else
            return;
        outs << (int)width << '\'' << cfmt;
        while (num--)
            outs << buf[num];
    }
}


//---------------------------------------------------------------------------
//  Object overrides and functions
//---------------------------------------------------------------------------
//
// Data variables and expressions
//

ostream &
operator<<(ostream &outs, vl_var *s)
{
    s->print(outs);
    return (outs);
}


void
vl_strength::print()
{
    cout << '(';
    switch (str0()) {
    case STRnone:
        cout << "s0";
        break;
    case STRhiZ:
        cout << "z0";
        break;
    case STRsmall:
        cout << "s";
        break;
    case STRmed:
        cout << "m";
        break;
    case STRweak:
        cout << "w0";
        break;
    case STRlarge:
        cout << "l";
        break;
    case STRpull:
        cout << "p0";
        break;
    case STRstrong:
        cout << "s0";
        break;
    case STRsupply:
        cout << "sp0";
        break;
    }
    switch (str1()) {
    case STRnone:
        cout << ",s1";
        break;
    case STRhiZ:
        cout << ",z1";
        break;
    case STRsmall:
        cout << "s";
        break;
    case STRmed:
        cout << "m";
        break;
    case STRweak:
        cout << ",w1";
        break;
    case STRlarge:
        cout << "l";
        break;
    case STRpull:
        cout << ",p1";
        break;
    case STRstrong:
        cout << ",s1";
        break;
    case STRsupply:
        cout << ",sp1";
        break;
    }
    cout << ')';
}
// End of vl_strength functions.


void
vl_var::print(ostream &outs)
{
    if (v_name)
        outs << v_name;
    else if (data_type() == Dconcat)
        outs << '{' << v_data.c << '}';        
    else
        outs << '?';
    if (v_range)
        outs << v_range;
}


void
vl_var::print_value(ostream &outs, DSPtype dtype)
{
    if (data_type() == Dbit) {
        if (v_array.size()) {
            char **ss = (char**)v_data.d;
            for (int i = 0; i < v_array.size(); i++) {
                if (i)
                    outs << ' ';
                printbits(outs, ss[i], v_bits.size(), dtype);
            }
        }
        else
            printbits(outs, v_data.s, v_bits.size(), dtype);
    }
    else if (data_type() == Dint) {
        if (v_array.size()) {
            int *ii = (int*)v_data.d;
            outs << ii[0];
            for (int i = 1; i < v_array.size(); i++)
                outs << ' ' << ii[i];
        }
        else
            outs << v_data.i;
    }
    else if (data_type() == Dreal) {
        if (v_array.size()) {
            double *dd = (double*)v_data.d;
            outs << dd[0];
            for (int i = 1; i < v_array.size(); i++)
                outs << ' ' << dd[i];
        }
        else
            outs << v_data.r;
    }
    else if (data_type() == Dstring) {
        if (v_array.size()) {
            char **ss = (char**)v_data.d;
            char *s = vl_fix_str(ss[0]);
            outs << s;
            delete [] s;
            for (int i = 1; i < v_array.size(); i++) {
                s = vl_fix_str(ss[i]);
                outs << ' ' << s;
                delete [] s;
            }
        }
        else {
            char *s = vl_fix_str(v_data.s);
            outs << v_data.s;
            delete [] s;
        }
    }
    else if (data_type() == Dtime) {
        if (v_array.size()) {
            vl_time_t *tt = (vl_time_t*)v_data.d;
            outs << tt[0];
            for (int i = 1; i < v_array.size(); i++)
                outs << ' ' << tt[i];
        }
        else
            outs << v_data.t;
    }
    else
        outs << "bad data type\n";
}


const char *
vl_var::decl_type()
{
    switch (v_net_type) {
    case REGnone:
        if (data_type() == Dint)
            return ("integer");
        else if (data_type() == Dtime)
            return ("time");
        else if (data_type() == Dreal)
            return ("real");
        else if (data_type() == Dstring)
            return ("string");
        return ("unknown");
    case REGparam:
        return ("parameter");
    case REGreg:
        return ("reg");
    case REGevent:
        return ("event");
    case REGwire:
        return ("wire");
    case REGtri:
        return ("tri");
    case REGtri0:
        return ("tri0");
    case REGtri1:
        return ("tri1");
    case REGsupply0:
        return ("supply0");
    case REGsupply1:
        return ("supply1");
    case REGwand:
        return ("wand");
    case REGtriand:
        return ("triand");
    case REGwor:
        return ("wor");
    case REGtrior:
        return ("trior");
    case REGtrireg:
        return ("trireg");
    }
    return ("unknown");
}


// Return print field width for columns.
//
int
vl_var::pwidth(char fmt)
{
    if (data_type() == Dbit) {
        switch (fmt) {
        case 'b':
            return (v_bits.size());
        case 'h':
            return (v_bits.size()/4 + v_bits.size()%4);
        case 'o':
            return (v_bits.size()/3 + v_bits.size()%3);
        case 'd': 
            return ((int)ceil(v_bits.size()*log10(2.0)));
        }
    }
    else if (data_type() == Dint)
        return ((int)ceil(8*sizeof(int)*log10(2.0)));
    else if (data_type() == Dtime)
        return ((int)ceil(8*sizeof(vl_time_t)*log10(2.0)));
    else if (data_type() == Dreal)
        return (12);
    else if (data_type() == Dstring) {
        if (!v_data.s)
            return (8);
        return (8*(strlen(v_data.s)/8) + 8);
    }
    return (1);
}


char *
vl_var::bitstr()
{
    if (data_type() == Dbit) {
        if (!v_array.size()) {
            char *s = new char[v_bits.size() + 1];
            for (int i = v_bits.size()-1, j = 0; i >= 0; i--, j++) {
                if (v_data.s[i] == BitL)
                    s[j] = '0';
                else if (v_data.s[i] == BitH)
                    s[j] = '1';
                else if (v_data.s[i] == BitZ)
                    s[j] = 'z';
                else
                    s[j] = 'x';
            }
            s[v_bits.size()] = 0;
            return (s);
        }
    }
    else if (data_type() == Dint) {
        if (!v_array.size()) {
            unsigned x = (unsigned)v_data.i;
            int isz = 8*(int)sizeof(int);
            char *s = new char[isz + 1];
            for (int i = 0; i < isz; i++) {
                if (x & 1)
                    s[isz - i - 1] = '1';
                else
                    s[isz - i - 1] = '0';
                x >>= 1;
            }
            s[isz] = 0;
            return (s);
        }
    }
    else if (data_type() == Dtime) {
        if (!v_array.size()) {
            vl_time_t x = v_data.t;
            int isz = 8*(int)sizeof(vl_time_t);
            char *s = new char[isz + 1];
            for (int i = 0; i < isz; i++) {
                if (x & 1)
                    s[isz - i - 1] = '1';
                else
                    s[isz - i - 1] = '0';
                x >>= 1;
            }
            s[isz] = 0;
            return (s);
        }
    }
    return (0);
}
// End of vl_var functions.


ostream &
operator<<(ostream &outs, vl_expr *s)
{
    if (s)
        s->print(outs);
    return (outs);
}


void
vl_expr::print(ostream &outs)
{
    switch (e_type) {
    case BitExpr:
        print_value(outs);
        break;
    case IntExpr:
        outs << data().i;
        break;
    case RealExpr:
        outs << data().r;
        break;
    case IDExpr:
        if (e_data.ide.name)
            outs << e_data.ide.name;
        break;
    case BitSelExpr:
    case PartSelExpr:
        if (e_data.ide.name)
            outs << e_data.ide.name;
        if (e_data.ide.range)
            outs << e_data.ide.range;
        break;
    case ConcatExpr: {
        if (e_data.mcat.var && e_data.mcat.var->data().c) {
            if (e_data.mcat.rep)
                outs << "{" << e_data.mcat.rep << "{" <<
                    e_data.mcat.var->data().c << "}}";
            else
                outs << "{" << e_data.mcat.var->data().c << "}";
        }
        break;                        
    }
    case MinTypMaxExpr: {
        if (e_data.exprs.e1) {
            outs << e_data.exprs.e1;
            if (e_data.exprs.e2)
                outs << ":" << e_data.exprs.e2;
            if (e_data.exprs.e3)
                outs << ":" << e_data.exprs.e3;
        }
        break;                        
    }
    case StringExpr:
        if (data().s)
            outs << data().s;
        break;
    case FuncExpr:
        if (e_data.func_call.name) {
            outs << e_data.func_call.name;
            if (e_data.func_call.args)
                outs << "(" << e_data.func_call.args << ")";
        }
        break;
    case UplusExpr:
    case UminusExpr:
    case UnotExpr:
    case UcomplExpr:
    case UandExpr:
    case UnandExpr:
    case UorExpr:
    case UnorExpr:
    case UxorExpr:
    case UxnorExpr:
        if (e_data.exprs.e1)
            outs << symbol() << e_data.exprs.e1;
        break;
    
    case BplusExpr:
    case BminusExpr:
    case BtimesExpr:
    case BdivExpr:
    case BremExpr:
    case Beq2Expr:
    case Bneq2Expr:
    case Beq3Expr:
    case Bneq3Expr:
    case BlandExpr:
    case BlorExpr:
    case BltExpr:
    case BleExpr:
    case BgtExpr:
    case BgeExpr:
    case BandExpr:
    case BorExpr:
    case BxorExpr:
    case BxnorExpr:
    case BlshiftExpr:
    case BrshiftExpr:
        if (e_data.exprs.e1 && e_data.exprs.e2) {
            outs << e_data.exprs.e1 << ' ' << symbol() << ' ' <<
                e_data.exprs.e2;
        }
        break;
    case TcondExpr:
        if (e_data.exprs.e1 && e_data.exprs.e2 && e_data.exprs.e3)
            outs << e_data.exprs.e1 << " ? " << e_data.exprs.e2 << " : "
                << e_data.exprs.e3;
        break;
    case SysExpr:
        outs << e_data.systask;
        break;
    }
}


const char *
vl_expr::symbol()
{
    switch (e_type) {
    case IDExpr:
    case BitSelExpr:
    case PartSelExpr:
    case ConcatExpr:
    case MinTypMaxExpr:
    case IntExpr:
    case RealExpr:
    case StringExpr:
    case FuncExpr:      return("");    
    case UplusExpr:     return("+");        
    case UminusExpr:    return("-");        
    case UnotExpr:      return("!");        
    case UcomplExpr:    return("~");        
    case UandExpr:      return("&");        
    case UnandExpr:     return("~&");        
    case UorExpr:       return("|");        
    case UnorExpr:      return("~|");        
    case UxorExpr:      return("^");        
    case UxnorExpr:     return("~^");        
    case BplusExpr:     return("+");        
    case BminusExpr:    return("-");        
    case BtimesExpr:    return("*");        
    case BdivExpr:      return("/");        
    case BremExpr:      return("%");        
    case Beq2Expr:      return("==");        
    case Bneq2Expr:     return("!=");        
    case Beq3Expr:      return("===");        
    case Bneq3Expr:     return("!==");        
    case BlandExpr:     return("&&");        
    case BlorExpr:      return("||");        
    case BltExpr:       return("<");        
    case BleExpr:       return("<=");        
    case BgtExpr:       return(">");        
    case BgeExpr:       return(">=");        
    case BandExpr:      return("&");        
    case BorExpr:       return("|");        
    case BxorExpr:      return("^");        
    case BxnorExpr:     return("~^");        
    case BlshiftExpr:   return("<<");        
    case BrshiftExpr:   return(">>");        
    case TcondExpr:     return("?");        
    default:
        VP()->error(ERR_INTERNAL, "Unexpected expression type");
        break;
    }
    return (0);
}


ostream &
operator<<(ostream &outs, vl_strength s)
{
    if (s.str0() == STRnone)
        return (outs);
    outs << '(';
    switch (s.str0()) {
    case STRsupply:
        outs << "supply0";
        break;
    case STRstrong:
        outs << "strong0";
        break;
    case STRpull:
        outs << "pull0";
        break;
    case STRlarge:
        outs << "large)";
        return (outs);
    case STRweak:
        outs << "weak0";
        break;
    case STRmed:
        outs << "medium)";
        return (outs);
    case STRsmall:
        outs << "small)";
        return (outs);
    case STRhiZ:
        outs << "highz0";
        break;
    default:
        break;
    }

    switch (s.str1()) {
    case STRsupply:
        outs << ", supply1)";
        break;
    case STRstrong:
        outs << ", strong1)";
        break;
    case STRpull:
        outs << ", pull1)";
        break;
    case STRweak:
        outs << ", weak1)";
        break;
    case STRhiZ:
        outs << ", highz1)";
        break;
    default:
        break;
    }
    return (outs);
}


ostream &
operator<<(ostream &outs, vl_range *r)
{
    if (r->left()) {
        outs << "[" << r->left();
        if (r->right())
            outs << ":" << r->right();
        outs << "]";
    }
    return (outs);
}


ostream &
operator<<(ostream &outs, vl_delay *d)
{
    if (d->list) {
        if (d->list->length() > 1)
            outs << "#(" << d->list << ") ";
        else
            outs << "#" << d->list << " ";
    }
    else
        outs << "#" << d->delay1 << " ";
    return (outs);
}


ostream &
operator<<(ostream &outs, vl_event_expr *e)
{
    bool nop = false;
    switch (e->type()) {
    case NegedgeEventExpr:
        outs << "(negedge ";
        break;
    case PosedgeEventExpr:
        outs << "(posedge ";
        break;
    case EdgeEventExpr:
        if (e->expr() && e->expr()->etype() != IDExpr)
            outs << "(";
        else
            nop = true;
        break;
    case OrEventExpr:
        outs << "(";
        break;
    default:
        VP()->error(ERR_INTERNAL, "Unexpected EventExpr Type");        
        break;
    }
    if (e->expr())
        outs << e->expr();
    else if (e->list()) {
        vl_event_expr *ex;
        lsGen<vl_event_expr*> gen(e->list());
        if (gen.next(&ex)) {
            outs << ex;
            while(gen.next(&ex)) {
                outs << " or ";
                outs << ex;
            }
        }
    }
    if (!nop)
        outs << ')';
    return (outs);
}


//
// Parser objects
//

void
vl_parser::print(ostream &outs)
{
    if (p_description)
        outs << p_description;
}


//
// Simulator objects
//

bool
vl_simulator::monitor_change(lsList<vl_expr*> *args)
{
    if (s_first_point)
        return (true);
    if (s_stop)
        return (false);
    vl_expr *e;
    lsGen<vl_expr*> gen(args);
    // see if anything has changed
    while (gen.next(&e)) {
        if (e->etype() == StringExpr)
            continue;
        if (e->etype() == SysExpr)
            continue;
        vl_var od = *e;
        vl_var &nd = e->eval();
        if (od.data_type() != nd.data_type())
            continue;
        vl_var &z = case_neq(od, nd);
        if (z.data().s[0] == BitH)
            return (true);
    }
    return (false);
}


namespace {
    inline void wformat(ostream &outs, const char *str, char fill, int wid)
    {
        if (wid <= 0) {
            outs << str;
            return;
        }
        int w = strlen(str);
        if (w <= wid) {
            int i = wid - w;
            while (i--)
                outs << fill;
        }
        outs << str;
    }
}


void
vl_simulator::display_print(lsList<vl_expr*> *args, ostream &outs,
    DSPtype dtype, unsigned int flags)
{
    bool first = true;
    vl_expr *e;
    lsGen<vl_expr*> gen(args);
    bool hadnl = false;
    while (gen.next(&e)) {
        if (!first)
            outs << ' ';
        first = false;
        hadnl = false;
        if (e->etype() == StringExpr) {
            char *string = e->data().s;
            if (!string)
                continue;
            string = vl_fix_str(string);
            char *s = string + strlen(string) - 1;
            if (*s == '\n')
                hadnl = true;
            char *tstr = string;
            while (*string) {
                char buf[256];
                if (*string != '%') {
                    outs << *string;
                    string++;
                    continue;
                }
                char *xxstr = string;
                int fw = 0;  // field width
                while (isdigit(*(string+1))) {
                    fw = 10*fw + (*(string+1) - '0');
                    string++;
                }

                switch (*(string+1)) {
                case 'b':
                case 'B':
                    gen.next(&e);
                    {
                        char *ss = e->eval().bitstr();
                        wformat(outs, ss, ' ', fw);
                        delete [] ss;
                    }
                    break;
                case 'c':
                case 'C':
                    gen.next(&e);
                    sprintf(buf, "%c", (char)(int)(e->eval()));
                    wformat(outs, buf, ' ', fw);
                    break;
                case 'd':
                case 'D':
                    gen.next(&e);
                    {
                        vl_var &d = e->eval();
                        if (d.data_type() == Dbit) {
                            if (d.is_x()) {
                                strcpy(buf, "x");
                                fw = 0;
                            }
                            else
                                sprintf(buf, "%u", (unsigned)d);
                        }
                        else
                            sprintf(buf, "%d", (int)d);
                        wformat(outs, buf, ' ', fw);
                    }
                    break;
                case 'e':
                case 'E':
                    gen.next(&e);
                    sprintf(buf, "%e", (double)(e->eval()));
                    wformat(outs, buf, ' ', fw);
                    break;
                case 'f':
                case 'F':
                    gen.next(&e);
                    sprintf(buf, "%f", (double)(e->eval()));
                    wformat(outs, buf, ' ', fw);
                    break;
                case 'g':
                case 'G':
                    gen.next(&e);
                    sprintf(buf, "%g", (double)(e->eval()));
                    wformat(outs, buf, ' ', fw);
                    break;
                case 'h':
                case 'H':
                case 'x':
                case 'X':
                    gen.next(&e);
                    {
                        vl_var &d = e->eval();
                        if (d.data_type() == Dbit) {
                            int w = (d.bits().size() + 3)/4;
                            if (d.is_x()) {
                                int i = 0;
                                for ( ; i < w; i++)
                                    buf[i] =  'x';
                                buf[i] = 0;
                                fw = w;
                            }
                            else
                                sprintf(buf, "%0*x", w, (unsigned)d);
                        }
                        else
                            sprintf(buf, "%x", (unsigned)d);
                        wformat(outs, buf, ' ', fw);
                    }
                    break;
                case 'm':
                case 'M':
                    {
                        char *ss = s_context->hiername();
                        if (ss) {
                            outs << ss;
                            delete [] ss;
                        }
                    }
                    break;
                case 'o':
                case 'O':
                    gen.next(&e);
                    {
                        vl_var &d = e->eval();
                        if (d.data_type() == Dbit) {
                            int w = (d.bits().size() + 2)/3;
                            if (d.data_type() == Dbit && d.is_x()) {
                                int i = 0;
                                for ( ; i < w; i++)
                                    buf[i] =  'x';
                                buf[i] = 0;
                                fw = w;
                            }
                            else
                                sprintf(buf, "%0*o", w, (unsigned)d);
                        }
                        else
                            sprintf(buf, "%o", (unsigned)d);
                        wformat(outs, buf, ' ', fw);
                    }
                    break;
                case 's':
                case 'S':
                    gen.next(&e);
                    {
                        char *tt = (char*)(e->eval());
                        tt = vl_fix_str(tt);
                        sprintf(buf, "%s", tt);
                        delete [] tt;
                        wformat(outs, buf, ' ', fw);
                    }
                    break;
                case 't':
                case 'T':
                    gen.next(&e);
                    {
                        vl_var &d = e->eval();
                        if (d.data_type() == Dbit && d.is_x())
                            strcpy(buf, "x");
                        else {
                            double t = (double) d;
                            t *= (s_description->tstep()/pow(10.0, s_tfunit));
                            if (s_tfsuffix && *s_tfsuffix)
                                sprintf(buf, "%.*f%s", s_tfprec, t, s_tfsuffix);
                            else
                                sprintf(buf, "%.*f", s_tfprec, t);
                        }
                        wformat(outs, buf, ' ', s_tfwidth);
                    }
                    break;
                case 'v':
                case 'V':
                    outs << "???";
                    break;
                case '%':
                    outs << "%";
                    break;
                default:
                    while (xxstr <= string+1) {
                        outs << *xxstr;
                        xxstr++;
                    }
                }
                string++;
                if (*string)
                    string++;
            }
            delete [] tstr;
        }
        else
            e->eval().print_value(outs, dtype);
    }
    if (!hadnl && !(flags & SYSno_nl))
        outs << '\n';
}


void
vl_simulator::fdisplay_print(lsList<vl_expr*> *args, DSPtype dtype,
    unsigned int flags)
{
    if (!args)
        return;
    lsGen<vl_expr*> gen(args);
    vl_expr *e;
    if (!gen.next(&e))
        return;
    int fh = (int)e->eval();
    lsList<vl_expr*> pargs = *args;
    pargs.next();
    for (int i = 0; i < 32; i++) {
        if ((fh & 1) && s_channels[i])
            display_print(&pargs, *s_channels[i], dtype, flags);
        fh >>= 1;
    }
    pargs.clear();
}


void
vl_context::print(ostream &outs)
{
    vl_context *cx = this;
    while (cx) {
        if (cx->c_module) {
            if (cx->c_module->instance())
                outs << (cx->c_module->instance()->name() ?
                    cx->c_module->instance()->name() : "_mod");
            else
                outs << cx->c_module->name();
        }
        else if (cx->c_primitive) {
            outs << (cx->c_primitive->name() ?
                cx->c_primitive->name() : "_prim");
        }
        else if (cx->c_task) {
            outs << (cx->c_task->name() ?
                cx->c_task->name() : "_task");
        }
        else if (cx->c_function) {
            outs << (cx->c_function->name() ?
                cx->c_function->name() : "_func");
        }
        else if (cx->c_block) {
            outs << (cx->c_block->name() ?
                cx->c_block->name() : "_block");
        }
        else if (cx->c_fjblk) {
            outs << (cx->c_fjblk->name() ?
                cx->c_fjblk->name() : "_fjblk");
        }
        else
            outs << "blank";
        cx = cx->c_parent;
        if (cx)
            outs << ".";
    }
}


// Return the name hierarchy for context
//
char *
vl_context::hiername()
{
    char *string = 0;
    for (vl_context *cx = this; cx; cx = cx->c_parent) {
        const char *nm;
        if (cx->c_module) {
            if (cx->c_module->instance())
                nm = cx->c_module->instance()->name();
            else
                nm = cx->c_module->name();
        }
        else if (cx->c_primitive)
            nm = cx->c_primitive->name();
        else if (cx->c_task)
            nm = cx->c_task->name();
        else if (cx->c_function)
            nm = cx->c_function->name();
        else
            nm = 0;
        if (nm) {
            if (!string)
                string = vl_strdup(nm);
            else {
                char *xx = new char[strlen(nm) + strlen(string) + 2];
                char *s = xx;
                while (*nm)
                    *s++ = *nm++;
                *s++ = '.';
                char *t = string;
                while (*t)
                    *s++ = *t++;
                *s = 0;
                delete [] string;
                string = xx;
            }
        }
    }
    return (string);
}


//
// Verilog description objects
//

ostream &
operator<<(ostream &outs, vl_desc *d)
{
    vl_module *mod;
    lsGen<vl_module*> mgen(d->modules());
    while (mgen.next(&mod))
        outs << mod;
    vl_primitive *prim;
    lsGen<vl_primitive*> pgen(d->primitives());
    while (pgen.next(&prim))
        outs << prim;
    return (outs);
}


ostream &
operator<<(ostream &outs, vl_module *m)
{
    outs << "module ";
    IndentLevel = 0;
    if (m->name())
        outs << m->name();
    outs << " (";
    if (m->ports())
        outs << m->ports();
    outs << ");\n";
    if (m->mod_items())
        vl_print_items(outs, m->mod_items());
    outs << "endmodule\n\n";
    return (outs);
}


ostream &
operator<<(ostream &outs, vl_primitive *p)
{
    outs << "primitive ";
    IndentLevel = 0;
    if (p->name())
        outs << p->name();
    outs << " (";
    if (p->ports())
        outs << p->ports();
    outs << ");\n";
    if (p->decls())
        vl_print_items(outs, p->decls());
    if (p->initial()) {
        outs << "initial\n";
        IndentLevel++;
        outs << p->initial() << ";\n";
        IndentLevel--;
    }
    if (p->ptable()) {
        outs << "table\n";
        IndentLevel++;
        int os = p->type() == SeqPrimDecl ? 2 : 1;
        unsigned char *row = p->ptable();
        for (int i = 0; i < p->rows(); i++) {
            unsigned char *col = row + os;
            Indent(outs);
            for (int j = 0; j < MAXPRIMLEN - os; j++) {
                if (col[j] == PrimNone)
                    break;
                outs << p->symbol(col[j]) << ' ';
            }
            if (p->type() == SeqPrimDecl) 
                outs << ": " << p->symbol(row[1]) << ' ';
            
            outs << ": " << p->symbol(row[0]) << ";\n";
            row += MAXPRIMLEN;
        }
        IndentLevel--;
        outs << "endtable\n";
    }
    outs << "endprimitive\n\n";
    return (outs);
}


const char *
vl_primitive::symbol(unsigned char sym)
{
    switch (sym) {
    case PrimNone: return("");
    case Prim0: return("0");
    case Prim1: return("1");        
    case PrimX: return("X");        
    case PrimB: return("B");
    case PrimQ: return("?");
    case PrimR: return("R");
    case PrimF: return("F");
    case PrimP: return("P");
    case PrimN: return("N");
    case PrimS: return("*");
    case PrimM: return("-");
    case Prim0X: return("(0X)");
    case Prim1X: return("(1X)");
    case PrimX0: return("(X0)");
    case PrimX1: return("(X1)");       
    case PrimXB: return("(XB)");
    case PrimBX: return("(BX)");
    case PrimBB: return("(BB)");
    case PrimQ0: return("(?0)");
    case PrimQ1: return("(?1)");
    case PrimQB: return("(?B)");
    case Prim0Q: return("(0?)");
    case Prim1Q: return("(1?)");
    case PrimBQ: return("(B?)");
    default:
        char msg[MAXSTRLEN];
        sprintf(msg, "Unexpected primitive symbol type %d", sym);
        VP()->error(ERR_INTERNAL, msg);
        break;
    }
    return (0);
}


ostream &
operator<<(ostream &outs, vl_port *p)
{
    if (p->type() == NamedPort && p->name())
        outs << "." << p->name() << "(";
    if (p->port_exp()) {
        if (p->port_exp()->length() == 1)
            outs << p->port_exp();
        else
            outs << '{' << p->port_exp() << '}';
    }
    if (p->type() == NamedPort && p->name())
        outs << ")";
    return (outs);
}


ostream &
operator<<(ostream &outs, vl_port_connect *pc)
{
    if (pc->type() == NamedConnect && pc->name())
        outs << "." << pc->name() << "(";
    if (pc->expr())
        outs << pc->expr();
    if (pc->type() == NamedConnect && pc->name())
        outs << ")";
    return (outs);
}


//
// Module items
//

ostream &
operator<<(ostream &outs, vl_stmt *s)
{
    s->print(outs);
    return (outs);
}


ostream &
operator<<(ostream &outs, vl_decl *n)
{
    Indent(outs);
    outs << n->decl_type();
    if (n->strength().str0() != STRnone) {
        if (n->type() == WireDecl ||
                n->type() == TriDecl ||
                n->type() == WandDecl ||
                n->type() == TriandDecl ||
                n->type() == WorDecl ||
                n->type() == TriorDecl ||
                n->type() == TriregDecl)
            outs << ' ' << n->strength();
    }
    if (n->range())
        outs << ' ' << n->range();
    if (n->delay())
        outs << ' ' << n->delay();
    if (n->list())
        outs << ' ' << n->list();
    else
        outs << ' ' << n->ids();
    return (outs);
}


void
vl_decl::print(ostream &outs)
{
    outs << this;
}


const char *
vl_decl::decl_type()
{
    switch (st_type) {
    case RealDecl:
        return ("real");
    case EventDecl:
        return ("event");
    case IntDecl:
        return ("integer");
    case TimeDecl:
        return ("time");
    case InputDecl:
        return ("input");
    case OutputDecl:
        return ("output");
    case InoutDecl:
        return ("inout");
    case RegDecl:
        return ("reg");
    case WireDecl:
        return ("wire");
    case TriDecl:
        return ("tri");
    case Tri0Decl:
        return ("tri0");
    case Tri1Decl:
        return ("tri1");
    case Supply0Decl:
        return ("supply0");
    case Supply1Decl:
        return ("supply1");
    case WandDecl:
        return ("wand");
    case TriandDecl:
        return ("triand");
    case WorDecl:
        return ("wor");
    case TriorDecl:
        return ("trior");
    case TriregDecl:
        return ("trireg");
    case ParamDecl:
        return ("parameter");
    case DefparamDecl:
        return ("defparam");
    }
    return ("");
}


ostream &
operator<<(ostream &outs, vl_procstmt *p)
{
    switch (p->type()) {
    case AlwaysStmt:
        outs << "always\n";  
        break;
    case InitialStmt:
        outs << "initial\n"; 
        break;
    default:
        VP()->error(ERR_INTERNAL, "Unexpected Process Statement Type");
        break;
    }
    IndentLevel++;
    if (p->stmt())
        outs << p->stmt() << p->stmt()->lterm();
    IndentLevel--;
    return (outs);
}


void
vl_procstmt::print(ostream &outs)
{
    outs << this;
}


ostream &
operator<<(ostream &outs, vl_cont_assign *a)
{
    outs << "assign ";
    if (a->strength().str0() != STRnone)
        outs << a->strength() << ' ';
    if (a->delay())
        outs << a->delay() << ' ';
    if (a->assigns())
        outs << a->assigns();
    return (outs);
}


void
vl_cont_assign::print(ostream &outs)
{
    outs << this;
}


ostream &
operator<<(ostream &outs, vl_specify_block *s)
{
    outs << "specify\n";
    if (s->items())
        vl_print_items(outs, s->items());
    outs << "endspecify\n";
    return (outs);
}


void
vl_specify_block::print(ostream &outs)
{
    outs << this;
}


namespace {
    const char *pathstr(int p, bool all)
    {
        if (p == 0) {
            if (all)
                return (" *> ");
            return (" => ");
        }
        if (p == '+') {
            if (all)
                return (" +*> ");
            return (" +=> ");
        }
        if (p == '-') {
            if (all)
                return (" -*> ");
            return (" -=> ");
        }
        if (all)
            return (" ?*> ");
        return (" ?=> ");
    }


    const char *cndstr(int p)
    {
        if (p == 0)
            return (" : ");
        if (p == '+')
            return (" +: ");
        if (p == '-')
            return (" -: ");
        return (" ?: ");
    }
}


ostream &
operator<<(ostream &outs, vl_specify_item *s)
{
    if (s->type() == SpecParamDecl) {
        if (s->params())
            outs << "specparam " << s->params();
    }
    else if (s->type() == SpecPathDecl) {
        if (s->lhs() && s->rhs())
            outs << s->lhs() << " = " << s->rhs();
    }
    else if (s->type() == SpecLSPathDecl1) {
        if (s->expr() && s->list1() && s->list2() && s->rhs()) {
            outs << "if (" << s->expr() << ") (";
            outs << s->list1() << pathstr(s->pol(), false) << s->list2();
            outs << ") = " << s->rhs();
        }
    }
    else if (s->type() == SpecLSPathDecl2) {
        if (s->expr() && s->list1() && s->list2() && s->rhs()) {
            outs << "if (" << s->expr() << ") (";
            outs << s->list1() << pathstr(s->pol(), true) << s->list2();
            outs << ") = " << s->rhs();
        }
    }
    else if (s->type() == SpecESPathDecl1) {
        if (s->list1() && s->list2() && s->expr() && s->rhs()) {
            if (s->ifex())
                outs << "if (" << s->ifex() << ") ";
            if (s->edge_id() == PosedgeEventExpr)
                outs << "(posedge ";
            else if (s->edge_id() == NegedgeEventExpr)
                outs << "(negedge ";
            else
                outs << '(';
            outs << s->list1() << " => (" << s->list2() << cndstr(s->pol());
            outs << s->expr() << ")) = " << s->rhs();
        }
    }
    else if (s->type() == SpecESPathDecl2) {
        if (s->list1() && s->list2() && s->expr() && s->rhs()) {
            if (s->ifex())
                outs << "if (" << s->ifex() << ") ";
            if (s->edge_id() == PosedgeEventExpr)
                outs << "(posedge ";
            else if (s->edge_id() == NegedgeEventExpr)
                outs << "(negedge ";
            else
                outs << '(';
            outs << s->list1() << " => (" << s->list2() << cndstr(s->pol());
            outs << s->expr() << ")) = " << s->rhs();
        }
    }
    else if (s->type() == SpecTiming)
        outs << "// $setup()";
    return (outs);
}


void
vl_specify_item::print(ostream &outs)
{
    outs << this;
}


ostream &
operator<<(ostream &outs, vl_spec_term_desc *s)
{
    if (s->pol() == 0) {
        if (s->name() && s->exp1() && s->exp2())
            outs << s->name() << '[' << s->exp1() << " : " << s->exp2() << ']';
        else if (s->name() && s->exp1())
            outs << s->name() << '[' << s->exp1() << ']';
        else if (s->name())
            outs << s->name();
    }
    else if (s->pol() == '+' && s->exp1())
        outs << "+ " << s->exp1();
    else if (s->pol() == '-' && s->exp1())
        outs << "- " << s->exp1();
    return (outs);
}


void
vl_spec_term_desc::print(ostream &outs)
{
    outs << this;
}


ostream &
operator<<(ostream &outs, vl_path_desc *s)
{
    if (s->list1() && s->list2()) {
        if (s->type() == PathLeadTo)
            outs << '(' << s->list1() << " => " << s->list2() << ')';
        else if (s->type() == PathAll)
            outs << '(' << s->list1() << " *> " << s->list2() << ')';
    }
    return (outs);
}


void
vl_path_desc::print(ostream &outs)
{
    outs << this;
}


ostream &
operator<<(ostream &outs, vl_task *t)
{
    Indent(outs);
    outs << "task ";
    if (t->name())
        outs << t->name();
    outs << ";\n";
    IndentLevel++;
    if (t->decls())
        vl_print_items(outs, t->decls());
    if (t->stmts())
        vl_print_items(outs, t->stmts());
    IndentLevel--;
    Indent(outs);
    outs << "endtask";
    return (outs);
}


void
vl_task::print(ostream &outs)
{
    outs << this;
}


ostream &
operator<<(ostream &outs, vl_function *f)
{
    Indent(outs);
    outs << "function ";
    switch (f->type()) {
    case IntFuncDecl:
        outs << "integer ";
        break;
    case RealFuncDecl:
         outs << "real ";
         break;
    case RangeFuncDecl:
        outs << f->range() << ' ';
        break;
    default:
        VP()->error(ERR_INTERNAL, "Unexpected Function Type");
    }
    if (f->name())
        outs << f->name();
    outs << ";\n";

    IndentLevel++;
    if (f->decls())
        vl_print_items(outs, f->decls());
    if (f->stmts())
        vl_print_items(outs, f->stmts());
    IndentLevel--;
    Indent(outs);
    outs << "endfunction";
    return (outs);
}


void
vl_function::print(ostream &outs)
{
    outs << this;
}


ostream &
operator<<(ostream &outs, vl_gate_inst_list *l)
{
    Indent(outs);
    switch (l->type()) {
    case AndGate:
        outs << "and";
        break;
    case NandGate:
        outs << "nand";
        break;
    case OrGate:
        outs << "or";
        break;
    case NorGate:
        outs << "nor";
        break;
    case XorGate:
        outs << "xor";
        break;
    case XnorGate:
        outs << "xnor";
        break;
    case BufGate:
        outs << "buf";
        break;
    case Bufif0Gate:
        outs << "bufif0";
        break;
    case Bufif1Gate:
        outs << "bufif1";
        break;
    case NotGate:
        outs << "not";
        break;
    case Notif0Gate:
        outs << "notif0";
        break;
    case Notif1Gate:
        outs << "notif1";
        break;
    case PulldownGate:
        outs << "pulldown";
        break;
    case PullupGate:
        outs << "pullup";
        break;
    case NmosGate:
        outs << "nmos";
        break;
    case RnmosGate:
        outs << "rnmos";
        break;
    case PmosGate:
        outs << "pmos";
        break;
    case RpmosGate:
        outs << "rpmos";
        break;
    case CmosGate:
        outs << "cmos";
        break;
    case RcmosGate:
        outs << "rcmos";
        break;
    case TranGate:
        outs << "tran";
        break;
    case RtranGate:
        outs << "rtran";
        break;
    case Tranif0Gate:
        outs << "tranif0";
        break;
    case Rtranif0Gate:
        outs << "rtranif0";
        break;
    case Tranif1Gate:
        outs << "tranif1";
        break;
    case Rtranif1Gate:
        outs << "rtranif1";
        break;
    default:
        VP()->error(ERR_INTERNAL, "Unexpected Gate Type");
        break;
    }
    outs << ' ';
    if (l->strength().str0() != STRnone)
        outs << l->strength() << ' ';;
    if (l->delays())
        outs << l->delays();
    if (l->gates())
        outs << l->gates();
    return (outs);
}


void
vl_gate_inst_list::print(ostream &outs)
{
    outs << this;
}


ostream &
operator<<(ostream &outs, vl_mp_inst_list *m)
{
    Indent(outs);
    if (m->name())
        outs << m->name() << ' ';
    if (m->strength().str0() != STRnone)
        outs << m->strength() << ' ';
    if (m->prms_or_dlys())
        outs << m->prms_or_dlys() << ' ';
    if (m->mps())
        outs << m->mps();
    return (outs);
}


void
vl_mp_inst_list::print(ostream &outs)
{
    outs << this;
}


//
// Statements
//

ostream &
operator<<(ostream &outs, vl_bassign_stmt *b)
{
    Indent(outs);
    if (b->type() == AssignStmt)
        outs << "assign ";
    else if (b->type() == ForceStmt)
        outs << "force ";
    if (b->lhs())
        outs << b->lhs();

    switch (b->type()) {
    case AssignStmt:
    case ForceStmt:
    case BassignStmt:
        outs << " = ";        
        break;
    case NbassignStmt:
        outs << " <= "; 
        break;
    case DelayBassignStmt:
        outs << " = ";        
        break;
    case DelayNbassignStmt:
        outs << " <= ";        
        break;
    case EventBassignStmt:
        outs << " = @";
        break;
    case EventNbassignStmt:
        outs << " <= @";        
        break;
    default:
        VP()->error(ERR_INTERNAL, "Unexpected Assign Type");
        break;
    }
    if (b->event())
        outs << b->event() << " ";
    else if (b->delay())
        outs << ' ' << b->delay();
    int tmp = IndentLevel;
    IndentLevel = 0;
    outs << b->rhs();
    IndentLevel = tmp;
    return (outs);
}


void
vl_bassign_stmt::print(ostream &outs)
{
    outs << this;
}


ostream &
operator<<(ostream &outs, vl_sys_task_stmt *s)
{
    Indent(outs);
    if (s->name())
        outs << s->name();
    int tmp = IndentLevel;
    IndentLevel = 0;
    if (s->args())
        outs << " (" << s->args() << ')';        
    IndentLevel = tmp;
    return (outs);
}


void
vl_sys_task_stmt::print(ostream &outs)
{
    outs << this;
}


ostream &
operator<<(ostream &outs, vl_begin_end_stmt *b)
{
    Indent(outs);
    outs << "begin";
    if (b->name())
        outs << ": " << b->name() << '\n';
    else
        outs << '\n';
    IndentLevel++;
    if (b->decls())
        vl_print_items(outs, b->decls());
    if (b->stmts())
        vl_print_items(outs, b->stmts());
    IndentLevel--;
    Indent(outs);
    outs << "end";
    return (outs);
}


void
vl_begin_end_stmt::print(ostream &outs)
{
    outs << this;
}


ostream &
operator<<(ostream &outs, vl_if_else_stmt *i)
{
    Indent(outs);
    outs << "if (";
    if (i->cond())
        outs << i->cond();
    outs << ")\n";
    if (i->if_stmt()) {
        IndentLevel++;
        outs << i->if_stmt() << i->if_stmt()->lterm();
        IndentLevel--;
    }
    if (i->else_stmt()) {
        Indent(outs);
        outs << "else\n";
        IndentLevel++;
        outs << i->else_stmt() << i->else_stmt()->lterm();
        IndentLevel--;
    }
    return (outs);
}


void
vl_if_else_stmt::print(ostream &outs)
{
    outs << this;
}


ostream &
operator<<(ostream &outs, vl_case_stmt *c)
{
    Indent(outs);
    switch (c->type()) {
    case CaseStmt:
        outs << "case (";
        break;
    case CasexStmt:
        outs << "casex (";
        break;
    case CasezStmt:
        outs << "casez (";
        break;
    default:
        VP()->error(ERR_INTERNAL, "Unexpected Case Type");
        break;
    }
    if (c->cond())
        outs << c->cond();
    outs << ")\n";
    lsGen<vl_case_item*> gen(c->case_items());
    vl_case_item *item;
    while (gen.next(&item))
        outs << item;
    Indent(outs);
    outs << "endcase";
    return (outs);
}


void
vl_case_stmt::print(ostream &outs)
{
    outs << this;
}


ostream &
operator<<(ostream &outs, vl_case_item *c)
{
    Indent(outs);
    switch (c->type()) {
    case CaseItem:
        if (c->exprs())
            outs << c->exprs() << ": ";
        break;
    case DefaultItem:
        outs << "default: ";
        break;
    default:
        VP()->error(ERR_INTERNAL, "Unexpected CaseItem Type");
        break;
    }
    IndentSkip = true;
    if (c->stmt())
        outs << c->stmt() << c->stmt()->lterm();
    else
        outs << " ;\n";
    return (outs);
}


void
vl_case_item::print(ostream &outs)
{
    outs << this;
}


ostream &
operator<<(ostream &outs, vl_forever_stmt *f)
{
    Indent(outs);
    outs << "forever\n";
    IndentLevel++;
    if (f->stmt())
        outs << f->stmt() << f->stmt()->lterm();
    IndentLevel--;
    return (outs);
}


void
vl_forever_stmt::print(ostream &outs)
{
    outs << this;
}


ostream &
operator<<(ostream &outs, vl_repeat_stmt *r)
{
    Indent(outs);
    outs << "repeat (";
    if (r->count())
        outs << r->count();
    outs << ")\n";
    IndentLevel++;
    if (r->stmt())
        outs << r->stmt() << r->stmt()->lterm();
    IndentLevel--;
    return (outs);
}


void
vl_repeat_stmt::print(ostream &outs)
{
    outs << this;
}


ostream &
operator<<(ostream &outs, vl_while_stmt *w)
{
    Indent(outs);
    outs << "while (";
    if (w->cond())
        outs << w->cond();
    outs << ")\n";
    IndentLevel++;
    if (w->stmt())
        outs << w->stmt() << w->stmt()->lterm();
    IndentLevel--;
    return (outs);
}


void
vl_while_stmt::print(ostream &outs)
{
    outs << this;
}


ostream &
operator<<(ostream &outs, vl_for_stmt *f)
{
    Indent(outs);
    outs << "for (";
    IndentSkip = true;
    if (f->initial())
        outs << f->initial();
    outs << "; ";
    if (f->cond())
        outs << f->cond();
    outs << "; ";
    IndentSkip = true;
    if (f->end())
        outs << f->end();
    IndentSkip = false;
    outs << ")\n";
    IndentLevel++;
    if (f->stmt())
        outs << f->stmt() << f->stmt()->lterm();
    IndentLevel--;
    return (outs);
}


void
vl_for_stmt::print(ostream &outs)
{
    outs << this;
}


ostream &
operator<<(ostream &outs, vl_delay_control_stmt *c)
{
    Indent(outs);
    if (c->delay())
        outs << c->delay();
    if (c->stmt()) {
        IndentSkip = true;
        outs << c->stmt() << c->stmt()->lterm();
    }
    else
        outs << ";\n";
    return (outs);
}


void
vl_delay_control_stmt::print(ostream &outs)
{
    outs << this;
}


ostream &
operator<<(ostream &outs, vl_event_control_stmt *c)
{
    Indent(outs);
    outs << "@";
    if (c->event())
        outs << c->event() << '\n';
    if (c->stmt())
        outs << c->stmt() << c->stmt()->lterm();
    return (outs);
}


void
vl_event_control_stmt::print(ostream &outs)
{
    outs << this;
}


ostream &
operator<<(ostream &outs, vl_wait_stmt *w)
{
    Indent(outs);
    outs << "wait (";
    if (w->cond())
        outs << w->cond();
    outs << ")\n";
    IndentLevel++;
    if (w->stmt())
        outs << w->stmt() << w->stmt()->lterm();
    IndentLevel--;
    return (outs);
}


void
vl_wait_stmt::print(ostream &outs)
{
    outs << this;
}


ostream &
operator<<(ostream &outs, vl_send_event_stmt *s)
{
    if (s->name()) {
        Indent(outs);
        outs << "->" << s->name();
    }
    return (outs);
}


void
vl_send_event_stmt::print(ostream &outs)
{
    outs << this;
}


ostream &
operator<<(ostream &outs, vl_fork_join_stmt *f)
{
    Indent(outs);
    outs << "fork";
    if (f->name())
        outs << ": " << f->name() << '\n';
    else
        outs << '\n';
    IndentLevel++;
    if (f->decls())
        vl_print_items(outs, f->decls());
    if (f->stmts())
        vl_print_items(outs, f->stmts());
    IndentLevel--;
    Indent(outs);
    outs << "join";
    return (outs);
}


void
vl_fork_join_stmt::print(ostream &outs)
{
    outs << this;
}


ostream &
operator<<(ostream &outs, vl_fj_break *f)
{
    (void)f;
    Indent(outs);
    outs << "fork/join thread notify complete";
    return (outs);
}


void
vl_fj_break::print(ostream &outs)
{
    outs << this;
}


ostream &
operator<<(ostream &outs, vl_task_enable_stmt *t)
{
    Indent(outs);
    if (t->name())
        outs << t->name();
    if (t->args())
        outs << "(" << t->args() << ")";
    return (outs);
}


void
vl_task_enable_stmt::print(ostream &outs)
{
    outs << this;
}


ostream &
operator<<(ostream &outs, vl_disable_stmt *d)
{
    if (d->name()) {
        Indent(outs);
        outs << "disable " << d->name();
    }
    return (outs);
}


void
vl_disable_stmt::print(ostream &outs)
{
    outs << this;
}


ostream &
operator<<(ostream &outs, vl_deassign_stmt *d)
{
    if (d->lhs()) {
        Indent(outs);
        if (d->type() == DeassignStmt)
            outs << "deassign " << d->lhs();
        else if (d->type() == ReleaseStmt)
            outs << "release " << d->lhs();
    }
    return (outs);
}


void
vl_deassign_stmt::print(ostream &outs)
{
    outs << this;
}


//
// Instances
//

ostream &
operator<<(ostream &outs, vl_gate_inst *g)
{
    if (g->name())
        outs << g->name();
    if (g->array())
        outs << g->array() << ' ';
    outs << "(";
    if (g->terms())
        outs << g->terms();
    outs << ")";
    return (outs);
}


ostream &
operator<<(ostream &outs, vl_mp_inst *m)
{
    if (m->name())
        outs << m->name();
    outs << "(";
    if (m->ports())
        outs << m->ports();
    outs << ")";
    return (outs);
}


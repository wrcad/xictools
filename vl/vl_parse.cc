
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

#include <stdarg.h>
#include "vl_st.h"
#include "vl_list.h"
#include "vl_defs.h"
#include "vl_types.h"

extern void yyrestart(FILE*);

//---------------------------------------------------------------------------
//  Parser objects
//---------------------------------------------------------------------------

vl_parser *vl_parser::p_parser = 0;

vl_parser::vl_parser()
{
    if (p_parser) {
        fprintf(stderr, "Singleton vl_parser is already instantiated.\n");
        exit(1);
    }
    p_parser = this;

    p_tunit = 1.0;
    p_tprec = 1.0;
    p_filename = vl_strdup("<stdin>");
    p_module_stack = new vl_stack_t<vl_module*>;
    p_file_stack = new vl_stack_t<FILE*>;
    p_fname_stack = new vl_stack_t<const char*>;
    p_lineno_stack = new vl_stack_t<int>;
    p_dir_stack = new vl_stack_t<const char*>;
    p_macros = new table<const char*>;
    p_no_go = false;
    p_verbose = false;
}


void
vl_parser::on_null_ptr()
{
    fprintf(stderr, "Singleton vl_parser used before instantiated.\n");
    exit(1);
}


vl_parser::~vl_parser()
{
    delete p_module_stack;
    delete p_file_stack;
    delete p_fname_stack;
    delete p_lineno_stack;
    delete p_dir_stack;
    delete p_macros;
}


bool
vl_parser::parse(int ac, char **av)
{
    clear();
    int i;
    if (setjmp(p_jbuf) == 1) {
        p_no_go = true;
        fclose(yyin);
        return (true);
    }
    for (i = 1; i < ac; i++) {
        if (av[i][0] == '-') {
            if (av[i][1] == 'v' && !av[i][2])
                p_verbose = true;
            continue;
        }

        p_filename = vl_strdup(av[i]);
        yyin = vl_file_open(p_filename, "r");
        if (!yyin)
            yyin = fopen(p_filename, "r");
        cout << p_filename << '\n';
        if (!yyin) {
            cerr << "failed to open file " << p_filename << '\n';
            return (true);
        }
        if (yyparse()) {
            cerr << "-- parsing failure\n";
            return (true);
        }
        fclose(yyin);
    }
    return (p_no_go);
}


bool
vl_parser::parse(FILE *fp)
{
    clear();
    yyin = fp;
    if (setjmp(p_jbuf) == 1) {
        p_no_go = true;
        return (true);
    }
    yyrestart(yyin);
    if (yyparse())
        return (true);
    return (p_no_go);
}


void
vl_parser::clear()
{
    p_filename = vl_strdup("<stdin>");
    delete p_module_stack;
    p_module_stack = new vl_stack_t<vl_module*>;
    delete p_file_stack;
    p_file_stack = new vl_stack_t<FILE*>;
    delete p_fname_stack;
    p_fname_stack = new vl_stack_t<const char*>;
    delete p_lineno_stack;
    p_lineno_stack = new vl_stack_t<int>;
    delete p_macros;
    p_macros = new table<const char*>;

    p_description = 0;
    p_context = 0;
    p_tunit = 1.0;
    p_tprec = 1.0;

    p_no_go = false;
}


void
vl_parser::error(vlERRtype err, const char *fmt, ...)
{
    fflush(stdout);
    va_list args;
    char buf[MAXSTRLEN];
    va_start(args, fmt);
    vsnprintf(buf, MAXSTRLEN, fmt, args);
    va_end(args);

    switch (err) {
    case ERR_OK:
        return;
    case ERR_WARN:
        if (yylineno >= 0)
            vl_warn("on line %d, %s", yylineno, buf);
        else
            vl_warn("%s", buf);
        return;
    case ERR_COMPILE:
        if (yylineno >= 0)
            vl_error("on line %d, %s", yylineno, buf);
        else
            vl_error("%s", buf);
        break;
    case ERR_INTERNAL:
        vl_error("(internal) %s", buf);
        break;
    }
    p_no_go = true;
}


// Parse a timescale string of the form `timescale t1 xx / t2 xx
//
bool
vl_parser::parse_timescale(const char *str)
{
    double t1 = 1.0, t2 = 1.0;
    const char *s = str;

    // skip leading space and `timescale
    while (isspace(*s))
        s++;
    while (*s && !isspace(*s))
        s++;
    while (isspace(*s))
        s++;

    // get t1 integer, must be 1, 10, 100
    if (*s != '1')
        return (false);
    s++;
    if (*s == '0') {
        t1 *= 10.0;
        s++;
        if (*s == '0') {
            t1 *= 10.0;
            s++;
        }
    }

    while (isspace(*s))
        s++;

    // get t1 scale
    char c = isupper(*s) ? tolower(*s) : *s;
    if (c == 'm')
        t1 *= 1e-3;
    else if (c == 'u')
        t1 *= 1e-6;
    else if (c == 'n')
        t1 *= 1e-9;
    else if (c == 'p')
        t1 *= 1e-12;
    else if (c == 'f')
        t1 *= 1e-15;
    else if (c != 's' && c != '/')
        return (false);

    // look for '/' separator
    while (*s && *s != '/')
        s++;
    if (*s != '/')
        return (false);
    s++;
    while (isspace(*s))
        s++;

    // get t2 integer, must be 1, 10, 100
    if (*s != '1')
        return (false);
    s++;
    if (*s == '0') {
        t2 *= 10.0;
        s++;
        if (*s == '0') {
            t2 *= 10.0;
            s++;
        }
    }

    while (isspace(*s))
        s++;

    // get t2 scale
    c = isupper(*s) ? tolower(*s) : *s;
    if (c == 'm')
        t2 *= 1e-3;
    else if (c == 'u')
        t2 *= 1e-6;
    else if (c == 'n')
        t2 *= 1e-9;
    else if (c == 'p')
        t2 *= 1e-12;
    else if (c == 'f')
        t2 *= 1e-15;
    else if (c != 's')
        return (false);

    if (t2 > t1)
        return (false);
    if (t2 < p_description->tstep)
        p_description->tstep = t2;
    p_tunit = t1;
    p_tprec = t2;
    return (true);
}
// End of vl_parser functions


namespace {
    char * utol(char *str)
    {
        for (char *cp = str; *cp!='\0'; cp++) {
            if (isupper(*cp))
                *cp = tolower(*cp);
        }
        return (str);
    }


    const char *hexnum[16] = {
        "0000","0001","0010","0011",
        "0100","0101","0110","0111",
        "1000","1001","1010","1011",
        "1100","1101","1110","1111"
    };
}


void
bitexp_parse::bin(char *instr)
{
    int size = atoi(instr);
    if (size == 0 || size > MAXBITNUM)
        size = MAXBITNUM;
    bits().set(size);
    char *firstcp = strpbrk(instr, "bB")+1;
    char *cp = instr + strlen(instr) - 1;
    int bpos = 0;
    while (bpos < bits().size() && cp >= firstcp) {
        if (*cp != '_' && *cp != ' ') {
            switch (*cp) {
            case '0':
                brep[bpos] = BitL;
                break;
            case '1':
                brep[bpos] = BitH;
                break;
            case 'z':
            case 'Z':
            case '?':
                brep[bpos] = BitZ;
                break;
            case 'x':
            case 'X':
                brep[bpos] = BitDC;
                break;
            }
            bpos++;
        }
        cp--;
    }
    for (; bpos < bits().size(); bpos++)
        if (*firstcp == 'x')
            brep[bpos] = BitDC;
        else if (*firstcp == 'z' || *firstcp == '?')
            brep[bpos] = BitZ;
        else
            brep[bpos] = BitL;
}


void
bitexp_parse::dec(char *instr)
{
    utol(instr);
    int size = atoi(instr);
    if (size == 0 || size > MAXBITNUM)
        size = MAXBITNUM;
    bits().set(size);
    char *firstcp = strpbrk(instr, "dD")+1;
    while (isspace(*firstcp))
        firstcp++;
    int num = atoi(firstcp); // don't put x, z, ? in decimal string
    char buf[MAXSTRLEN];
    sprintf(buf, "%d'h%x", bits().size(), num);
    hex(buf);
}


void
bitexp_parse::oct(char *instr)
{
    utol(instr);
    int size = atoi(instr);
    if (size == 0 || size > MAXBITNUM)
        size = MAXBITNUM;
    bits().set(size);
    char *firstcp = strpbrk(instr, "oO")+1;
    char *cp = instr + strlen(instr) - 1;
    int bpos = 0;
    while (bpos < bits().size() && cp >= firstcp) {
        if (*cp != '_' && !isspace(*cp)) {
            for (int i = 0; i < 3; i++) {
                if (bpos < bits().size()) {
                    if (*cp == 'x')
                        brep[bpos++] = BitDC;
                    else if (*cp == 'z' || *cp == '?')
                        brep[bpos++] = BitZ;
                    else if (isdigit(*cp) && *cp < '8')
                        brep[bpos++] = (hexnum[*cp - '0'][3-i] == '1' ?
                            BitH : BitL);
                }
            }
        }
        cp--;
    }
    for (; bpos < bits().size(); bpos++)
        if (*firstcp == 'x')
            brep[bpos] = BitDC;
        else if (*firstcp == 'z' || *firstcp == '?')
            brep[bpos] = BitZ;
        else
            brep[bpos] = BitL;
}


void
bitexp_parse::hex(char *instr)
{
    utol(instr);
    int size = atoi(instr);
    if (size == 0 || size > MAXBITNUM)
        size = MAXBITNUM;
    bits().set(size);
    char *firstcp = strpbrk(instr, "hH")+1;
    char *cp = instr + strlen(instr) - 1;
    int bpos = 0;
    while (bpos < bits().size() && cp >= firstcp) {
        if (*cp != '_' && !isspace(*cp)) {
            for (int i = 0; i < 4; i++) {
                if (bpos < bits().size()) {
                    if (*cp == 'x')
                        brep[bpos++] = BitDC;
                    else if (*cp == 'z' || *cp == '?')
                        brep[bpos++] = BitZ;
                    else if (isdigit(*cp))
                        brep[bpos++] = (hexnum[*cp - '0'][3-i] == '1' ?
                            BitH : BitL);
                    else if (isxdigit(*cp))
                        brep[bpos++] = (hexnum[*cp - 'a' + 10][3-i] == '1' ?
                            BitH : BitL);
                }
            }
        }
        cp--;
    }
    for (; bpos < bits().size(); bpos++)
        if (*firstcp == 'x')
            brep[bpos] = BitDC;
        else if (*firstcp == 'z' || *firstcp == '?')
            brep[bpos] = BitZ;
        else
            brep[bpos] = BitL;
}
// End of bitexp_parse functions.


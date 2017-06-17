
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 1996 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *                                                                        *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,      *
 *   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES      *
 *   OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-        *
 *   INFRINGEMENT.  IN NO EVENT SHALL STEPHEN R. WHITELEY BE LIABLE       *
 *   FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION      *
 *   OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN           *
 *   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN         *
 *   THE SOFTWARE.                                                        *
 *                                                                        *
 * This software is available for non-commercial use under the terms of   *
 * the GNU General Public License as published by the Free Software       *
 * Foundation; either version 2 of the License, or (at your option) any   *
 * later version.                                                         *
 *                                                                        *
 * A commercial license is available to those wishing to use this         *
 * library or a derivative work for commercial purposes.  Contact         *
 * Whiteley Research Inc., 456 Flora Vista Ave., Sunnyvale, CA 94086.     *
 * This provision will be enforced to the fullest extent possible under   *
 * law.                                                                   *
 *                                                                        *
 * You should have received a copy of the GNU General Public License      *
 * along with this program; if not, write to the Free Software            *
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.              *
 *========================================================================*
 *                                                                        *
 * Verilog Support Files                                                  *
 *                                                                        *
 *========================================================================*
 $Id: vl_parse.cc,v 1.8 2015/06/17 18:36:11 stevew Exp $
 *========================================================================*/

#include <stdarg.h>
#include "vl_st.h"
#include "vl_list.h"
#include "vl_defs.h"
#include "vl_types.h"

vl_parser VP;
extern void yyrestart(FILE*);

//---------------------------------------------------------------------------
//  Parser objects
//---------------------------------------------------------------------------

vl_parser::vl_parser()
{
    tunit = tprec = 1.0;
    filename = vl_strdup("<stdin>");
    module_stack = new vl_stack_t<vl_module*>;
    file_stack = new vl_stack_t<FILE*>;
    fname_stack = new vl_stack_t<char*>;
    lineno_stack = new vl_stack_t<int>;
    dir_stack = new vl_stack_t<char*>;
    macros = new table<char*>;
    no_go = false;
    verbose = false;
}


vl_parser::~vl_parser()
{
    delete module_stack;
    delete file_stack;
    delete fname_stack;
    delete lineno_stack;
    delete dir_stack;
    delete macros;
}


bool
vl_parser::parse(int ac, char **av)
{
    clear();
    int i;
    if (setjmp(jbuf) == 1) {
        no_go = true;
        fclose(yyin);
        return (true);
    }
    for (i = 1; i < ac; i++) {
        if (av[i][0] == '-') {
            if (av[i][1] == 'v' && !av[i][2])
                verbose = true;
            continue;
        }

        filename = vl_strdup(av[i]);
        yyin = vl_file_open(filename, "r");
        if (!yyin)
            yyin = fopen(filename, "r");
        cout << filename << '\n';
        if (!yyin) {
            cerr << "failed to open file " << filename << '\n';
            return (true);
        }
        if (yyparse()) {
            cerr << "-- parsing failure\n";
            return (true);
        }
        fclose(yyin);
    }
    return (no_go);
}


bool
vl_parser::parse(FILE *fp)
{
    clear();
    yyin = fp;
    if (setjmp(jbuf) == 1) {
        no_go = true;
        return (true);
    }
    yyrestart(yyin);
    if (yyparse())
        return (true);
    return (no_go);
}


void
vl_parser::clear()
{
    filename = vl_strdup("<stdin>");
    delete module_stack;
    module_stack = new vl_stack_t<vl_module*>;
    delete file_stack;
    file_stack = new vl_stack_t<FILE*>;
    delete fname_stack;
    fname_stack = new vl_stack_t<char*>;
    delete lineno_stack;
    lineno_stack = new vl_stack_t<int>;
    delete macros;
    macros = new table<char*>;

    description = 0;
    context = 0;
    tunit = tprec = 1.0;

    no_go = false;
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
    no_go = true;
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
    if (t2 < description->tstep)
        description->tstep = t2;
    tunit = t1;
    tprec = t2;
    return (true);
}
// End of vl_parser functions


static char *
utol(char *str)
{
    for (char *cp = str; *cp!='\0'; cp++)
        if (isupper(*cp)) *cp = tolower(*cp);
    return (str);
}


static const char *hexnum[16] = {
    "0000","0001","0010","0011",
    "0100","0101","0110","0111",
    "1000","1001","1010","1011",
    "1100","1101","1110","1111"
};


void
bitexp_parse::bin(char *instr)
{
    bits.size = atoi(instr);
    if (bits.size == 0 || bits.size > MAXBITNUM)
        bits.size = MAXBITNUM;
    bits.lo_index = 0;
    bits.hi_index = bits.size - 1;
    char *firstcp = strpbrk(instr, "bB")+1;
    char *cp = instr + strlen(instr) - 1;
    int bpos = 0;
    while (bpos < bits.size && cp >= firstcp) {
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
    for (; bpos < bits.size; bpos++)
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
    bits.size = atoi(instr);
    if (bits.size == 0 || bits.size > MAXBITNUM)
        bits.size = MAXBITNUM;
    char *firstcp = strpbrk(instr, "dD")+1;
    while (isspace(*firstcp))
        firstcp++;
    int num = atoi(firstcp); // don't put x, z, ? in decimal string
    char buf[MAXSTRLEN];
    sprintf(buf, "%d'h%x", bits.size, num);
    hex(buf);
}


void
bitexp_parse::oct(char *instr)
{
    utol(instr);
    bits.size = atoi(instr);
    if (bits.size == 0 || bits.size > MAXBITNUM)
        bits.size = MAXBITNUM;
    bits.lo_index = 0;
    bits.hi_index = bits.size - 1;
    char *firstcp = strpbrk(instr, "oO")+1;
    char *cp = instr + strlen(instr) - 1;
    int bpos = 0;
    while (bpos < bits.size && cp >= firstcp) {
        if (*cp != '_' && !isspace(*cp)) {
            for (int i = 0; i < 3; i++) {
                if (bpos < bits.size) {
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
    for (; bpos < bits.size; bpos++)
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
    bits.size = atoi(instr);
    if (bits.size == 0 || bits.size > MAXBITNUM)
        bits.size = MAXBITNUM;
    bits.lo_index = 0;
    bits.hi_index = bits.size - 1;
    char *firstcp = strpbrk(instr, "hH")+1;
    char *cp = instr + strlen(instr) - 1;
    int bpos = 0;
    while (bpos < bits.size && cp >= firstcp) {
        if (*cp != '_' && !isspace(*cp)) {
            for (int i = 0; i < 4; i++) {
                if (bpos < bits.size) {
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
    for (; bpos < bits.size; bpos++)
        if (*firstcp == 'x')
            brep[bpos] = BitDC;
        else if (*firstcp == 'z' || *firstcp == '?')
            brep[bpos] = BitZ;
        else
            brep[bpos] = BitL;
}
// End of bitexp_parse functions


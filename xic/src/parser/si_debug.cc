
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

#include "cd.h"
#include "si_parsenode.h"
#include "si_parser.h"

#include <ctype.h>


//
// Debugging support functions
//

// Parse a range spec of the form [min-max d1 d2].  The "-max" is
// optional, as are the dimension values.
//
void
rvals::parse_range(const char *str)
{
    while (*str && !isdigit(*str) && *str != ']' && *str != '-')
        str++;
    if (isdigit(*str)) {
        rmin = atoi(str);
        while (isdigit(*str))
            str++;
    }
    while (*str && !isdigit(*str) && *str != ']' && *str != '-')
        str++;
    if (*str == '-') {
        minus = true;
        while (*str && !isdigit(*str) && *str != ']' && *str != ',')
            str++;
        if (isdigit(*str)) {
            rmax = atoi(str);
            while (isdigit(*str))
                str++;
        }
    }
    while (*str && !isdigit(*str) && *str != ']')
        str++;
    if (isdigit(*str)) {
        d1 = atoi(str);
        while (isdigit(*str))
            str++;
    }
    while (*str && !isdigit(*str) && *str != ']')
        str++;
    if (isdigit(*str)) {
        d2 = atoi(str);
        while (isdigit(*str))
            str++;
    }
}
// End of rvals functions


// Print the value of the named variable in the supplied buffer
//
void
SIparser::printVar(const char *vname, char *buf)
{
    *buf = 0;
    if (!vname)
        return;
    char tbuf[64];
    rvals rv;
    const char *range = strchr(vname, '[');
    if (range) {
        strcpy(tbuf, vname);
        tbuf[range - vname] = 0;
        vname = tbuf;
        rv.parse_range(range);
    }
    for (siVariable *v = spVariables; v; v = (siVariable*)v->next) {
        if (v->type == TYP_ENDARG)
            break;
        if (!strcmp(vname, v->name)) {
            v->print_var(range ? &rv : 0, buf);
            break;
        }
    }
    if (!*buf)
        strcpy(buf, "???");  // out of scope
}


// Set the named variable from the value in val.  If there is a problem,
// return an error message
//
char *
SIparser::setVar(const char *vname, const char *val)
{
    if (!val)
        val = "";
    char tbuf[128];
    rvals rv;
    const char *range = strchr(vname, '[');
    if (range) {
        strcpy(tbuf, vname);
        tbuf[range - vname] = 0;
        vname = tbuf;
        rv.parse_range(range);
    }
    for (siVariable *v = spVariables; v; v = (siVariable*)v->next) {
        if (v->type == TYP_ENDARG)
            break;
        if (!strcmp(vname, v->name))
            return (v->set_var(range ? &rv : 0, val));
    }
    sprintf(tbuf, "Variable %s is not in scope, can't set.", vname);
    return (lstring::copy(tbuf));
}


// A couple of static functions to convert to/from string
// representations using ANSI C X3J11 backslash expansions.

// Static function.
//
char *
SIparser::fromPrintable(const char *str)
{
    if (!str)
        return (0);
    if (!*str)
        return (lstring::copy(""));
    sLstr lstr;
    for (const char *c = str; *c; c++) {
        if (*c == '\\') {
            c++;
            char n = *c;
            if (n == 'a')
                lstr.add_c('\a');
            else if (n == 'b')
                lstr.add_c('\b');
            else if (n == 'f')
                lstr.add_c('\f');
            else if (n == 'n')
                lstr.add_c('\n');
            else if (n == 'r')
                lstr.add_c('\r');
            else if (n == 't')
                lstr.add_c('\t');
            else if (n == 'v')
                lstr.add_c('\v');
            else if (n == '\'')
                lstr.add_c('\'');
            else if (n == '\\')
                lstr.add_c('\\');
            else if (isdigit(n)) {
                int i = n - '0';
                c++;
                n = *c;
                if (isdigit(n)) {
                    i <<= 3;
                    i += n - '0';
                    c++;
                    n = *c;
                    if (isdigit(n)) {
                        i <<= 3;
                        i += n - '0';
                        c++;
                    }
                }
                c--;
                if (i < 256)
                    lstr.add_c(i);
            }
            else {
                lstr.add_c('\\');
                lstr.add_c(n);
            }
        }
        else
            lstr.add_c(*c);
    }
    return (lstr.string_trim());
}


// Static function.
char *
SIparser::toPrintable(const char *str)
{
    if (!str)
        return (0);
    if (!*str)
        return (lstring::copy(""));
    sLstr lstr;
    for (const char *c = str; *c; c++) {
        if (*c == '\a') {
            lstr.add_c('\\');
            lstr.add_c('a');
        }
        else if (*c == '\b') {
            lstr.add_c('\\');
            lstr.add_c('b');
        }
        else if (*c == '\f') {
            lstr.add_c('\\');
            lstr.add_c('f');
        }
        else if (*c == '\n') {
            lstr.add_c('\\');
            lstr.add_c('n');
        }
        else if (*c == '\r') {
            lstr.add_c('\\');
            lstr.add_c('r');
        }
        else if (*c == '\t') {
            lstr.add_c('\\');
            lstr.add_c('t');
        }
        else if (*c == '\v') {
            lstr.add_c('\\');
            lstr.add_c('v');
        }
        else if (*c == '\'') {
            lstr.add_c('\\');
            lstr.add_c('\'');
        }
        else if (*c == '\\') {
            lstr.add_c('\\');
            lstr.add_c('\\');
        }
        else if (!isprint(*c)) {
            unsigned int i = (unsigned char)*c;
            lstr.add_c('\\');
            unsigned int i1 = i >> 6;
            if (i1)
                lstr.add_c('0' + i1);
            unsigned int i2 = (i - (i1 << 6)) >> 3;
            if (i1 || i2)
                lstr.add_c('0' + i2);
            unsigned int i3 = i % 8;
            lstr.add_c('0' + i3);
        }
        else
            lstr.add_c(*c);
    }
    return (lstr.string_trim());
}


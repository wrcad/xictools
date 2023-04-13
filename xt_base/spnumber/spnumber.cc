
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

/***************************************************************************
JSPICE3 adaptation of Spice3e2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1987 Wayne A. Christopher
         1992 Stephen R. Whiteley
****************************************************************************/

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <ctype.h>

#ifdef WRSPICE
#include "spnumber/spnumber.h"
typedef void ParseNode;
#include "spnumber/spparse.h"
#include "graph.h"
#include "cshell.h"
#include "simulator.h"
#else
#ifdef SPEXPORT
#include "spnumber.h"
#else
#include "spnumber/spnumber.h"
#endif
#endif


//
// WRspice number formatting, printing, parsing.
//

// Instantiate global class.
sSPnumber SPnum;

#ifdef WRSPICE
#define UNITS_CATCHAR() Sp.UnitsCatchar()
#define UNITS_SEPCHAR() Sp.UnitsSepchar()
#else
#define UNITS_CATCHAR() '#'
#define UNITS_SEPCHAR() '_'
#endif


// Return a string containing text of the number.  If the units arg is
// given and not nil, the units are printed, and the Spice magnitude
// abbreviations are used.  If fix is true, an attempt is made to keep
// the numeric part of the field width constant.
//
// The returned string memory is managed here in a rotating pool.
//
const char *
sSPnumber::printnum(double num, sUnits *units, bool fix, int numd)
{
#ifdef WRSPICE
    char *ustr = units ? units->unitstr() : 0;
    const char *ret = printnum(num, ustr, fix, numd);
    delete [] ustr;
    return (ret);
#else
    (void)units;
    return (printnum(num, (const char*)0, fix, numd));
#endif
}


// Return a string containing text of the number.  If the unitstr arg
// is given, the string is printed following the number, and the Spice
// magnitude abbreviations are used.  If fix is true, an attempt is
// made to keep the numeric part of the field width constant.
//
// The returned string memory is managed here in a rotating pool.
//
const char *
sSPnumber::printnum(double num, const char *unitstr, bool fix, int numd)
{
#ifdef WRSPICE
    if (numd < 2)
        numd = CP.NumDigits();
#endif
    if (numd < 2)
        numd = 6;
    if (numd > 15)
        numd = 15;

    if (unitstr) {
        char *fltbuf = newbuf();

        int mag;
        bool neg = false;
        if (num == 0.0)
            mag = 0;
        else {
            if (num < 0) {
                num = -num;
                neg = true;
            }
            mag = (int)floor(log10(num));
            double tenpm = pow(10.0, (double)mag);
            num /= tenpm;
        }

        if ((mag % 3) && !((mag - 1) % 3)) {
            num *= 10.0; 
            mag--;
        }
        else if ((mag % 3) && !((mag + 1) % 3)) {
            num *= 100.0;
            mag -= 2;
        }

        char buf[32];
        const char* c = suffix(mag);
        if (c)
            strcpy(buf, c);
        else {
            snprintf(buf, sizeof(buf), "e%d", mag);

            // Add a units "cat char" if the units string conflicts
            // with scale factors.
            char u = *unitstr;
            if (islower(u))
                u = toupper(u);
            if (strchr("AFM", u)) {
                char *t = buf + strlen(buf);
                *t++ = UNITS_CATCHAR();
                *t = 0;
            }
        }
        strcat(buf, unitstr);

        char *t = fltbuf;
        if (neg)
            *t++ = '-';
        else if (fix)
            *t++ = ' ';
        if (fix) {
            double tmp = num >= 0.0 ? num : -num;
            if (tmp >= 10.0) {
                numd--;
                if (tmp >= 100.0)
                    numd--;
            }
            snprintf(t, SPN_BUFSIZ-2, "%.*f%s", numd, num, buf);
        }
        else
            snprintf(t, SPN_BUFSIZ-2, "%.*g%s", numd+1, num, buf);
        return (fltbuf);
    }

    const char *nm = print_exp(num, numd);
    if (!fix && *nm == ' ')
        return (nm+1);
    return (nm);
}

// Format: %.*ne, with 2 exponent digits.
//
// The returned string memory is managed here in a rotating pool.
//
const char *
sSPnumber::print_exp(double d, int n)
{
    char *fltbuf = newbuf();
    snprintf(fltbuf, SPN_BUFSIZ, "%+.*e", n, d);
    return (fixxp2(fltbuf));
}


// Static Function.
// Use two digit exponent if possible.
//
const char *
sSPnumber::fixxp2(char *str)
{
    char *ss = strchr(str, 'e');
    if (!ss)
        ss = strchr(str, 'E');
    if (!ss)
        return (str);
    ss++;
    int xp = atoi(ss);

    if (xp > 99)
        return (str);
    if (xp < -99)
        return (str);

    if (xp < 0)    {
        *ss = '-';
        xp = -xp;
    }
    else
        *ss = '+';
    *(ss+1) = '0' + (xp/10);
    *(ss+2) = '0' + (xp%10);
    *(ss+3) = '\0';
    if (*str == '+')
        *str = ' ';
    return (str);
}


// Static function.
// Our convention is to use lower case for multipliers, upper case for
// the first char of a units prefix.  Return a multiplier string, if
// applicable, 0 otherwise.  The special case mag == 0 returns an
// empty string.
//
const char *
sSPnumber::suffix(int mag)
{
    const char *c = 0;
    switch (mag) {   
    case -18:   // atto
        c = "a";
        break;  
    case -15:   // femto
        c = "f";
        break;
    case -12:   // pico
        c = "p";
        break;
    case -9:    // nano
        c = "n";
        break;
    case -6:    // micro
        c = "u";
        break;
    case -3:    // milli
        c = "m";
        break;
    case 0:
        c = "";
        break;
    case 3:     // kilo
        c = "k";
        break;
    case 6:     // mega
        c = "meg";
        break;
    case 9:     // giga
        c = "g";
        break;
    case 12:    // tera
        c = "t";
        break;
    }
    return (c);
}


//
// This function parses a number.
//

// This serves two purposes:  1) faster than calling pow(), and 2)
// more accurate than some crappy pow() implementations.
//
double sSPnumber::np_powers[] = {
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
#define ETABSIZE 24


// Parse a number.  This will handle things like 10M, etc.  If the
// number should not have any unrelated trailing text, then whole is
// true.  The string pointer is advanced past the number.  If whole is
// false the argument is advanced to the end of the word if gobble is
// true.
//
// Returns 0 if no number can be found or if there are unrecognized
// trailing characters when whole is true, otherwise returns a pointer
// to static data.
//
// If ft_strictnumparse is true, and whole is false, the first of the
// trailing characters must be a Sp.UnitsCatchar().
//
// If units is given, set according to any trailing characters, before
// checking for whole/strictnumparse.
//
double *
sSPnumber::parse(const char **line, bool whole, bool gobble, sUnits **units)
{
    double mantis = 0;
    int expo1 = 0;
    int expo2 = 0;
    int sign = 1;
    int expsgn = 1;

    if (units)
        *units = 0;
    if (!line || !*line)
        return (0);
    const char *here = *line;
    if (*here == '+')
        // Plus, so do nothing except skip it.
        here++;
    if (*here == '-') {
        // Minus, so skip it, and change sign.
        here++;
        sign = -1;
    }

    // We don't want to recognise "P" as 0P, or .P as 0.0P...
    if  (*here == '\0' || (!isdigit(*here) && *here != '.') ||
            (*here == '.' && !isdigit(*(here+1)))) {
        // Number looks like just a sign!
        return (0);
    }
    while (isdigit(*here)) {
        // Digit, so accumulate it.
        mantis = 10*mantis + (*here - '0');
        here++;
    }
    if (*here == '\0') {
        // Reached the end of token - done.
        *line = here;
        np_num = mantis*sign;
        return (&np_num);
    }
    if (*here == '.') {
        // Found a decimal point!
        here++; // skip to next character
        if (*here == '\0') {
            // Number ends in the decimal point.
            *line = here;
            np_num = mantis*sign;
            return (&np_num);
        }
        while (isdigit(*here)) {
            // Digit, so accumulate it.
            mantis = 10*mantis + (*here - '0');
            expo1--;
            if (*here == '\0') {
                // Reached the end of token - done.
                *line = here;
                if (-expo1 < ETABSIZE)
                    np_num = sign*mantis/np_powers[-expo1];
                else
                    np_num = sign*mantis*pow(10.0, (double)expo1);
                return (&np_num);
            }
            here++;
        }
    }

    // Now look for "E","e",etc to indicate an exponent.
    if (*here == 'e' || *here == 'E' || *here == 'd' || *here == 'D') {
        // Have an exponent, so skip the e.
        here++;
        // Now look for exponent sign.
        if (*here == '+')
            // just skip +
            here++;
        if (*here == '-') {
            // Skip over minus sign.
            here++;
            // Make a negative exponent.
            expsgn = -1;
        }
        // Now look for the digits of the exponent.
        while (isdigit(*here)) {
            expo2 = 10*expo2 + (*here - '0');
            here++;
        }
    }

    // Now we have all of the numeric part of the number, time to
    // look for the scale factor (alphabetic).
    //
    char ch = isupper(*here) ? tolower(*here) : *here;
    switch (ch) {
    case 'a':
        expo1 -= 18;
        here++;
        break;
    case 'f':
        expo1 -= 15;
        here++;
        break;
    case 'p':
        expo1 -= 12;
        here++;
        break;
    case 'n':
        expo1 -= 9;
        here++;
        break;
    case 'u':
        expo1 -= 6;
        here++;
        break;
    case 'm':
        // Special case for m, may be m or mil or meg.
        if ((*(here+1) == 'E' || *(here+1) == 'e') &&
                (*(here+2) == 'G' || *(here+2) == 'g')) {
            expo1 += 6;
            here += 3;
        }
        else if ((*(here+1) == 'I' || *(here+1) == 'i') &&
                (*(here+2) == 'L' || *(here+2) == 'l')) {
            expo1 -= 6;
            here += 3;
            mantis = mantis*25.4;
        }
        else {
            // Not either special case, so just m => 1e-3.
            expo1 -= 3;
            here++;
        }
        break;
    case 'k':
        expo1 += 3;
        here++;
        break;
    case 'g':
        expo1 += 9;
        here++;
        break;
    case 't':
        expo1 += 12;
        here++;
        break;
    default:
        break;
    }

    char buf[32];
#ifdef WRSPICE
    // Pick up any training units.
    if (units) {
        // The number can optionally be followed by a units string,
        // optionally separated from the main number string with a
        // Sp.UnitsCatchar().  The second instance of a
        // Sp.UnitsCatchar() is replaced with Sp.UnitsSepchar().

        if (get_unitstr(&here, buf)) {
            *units = new sUnits;
            (*units)->set(buf);
        }
    }
#else
    (void)units;
    (void)gobble;
#endif

    if (whole && *here != '\0')
        return (0);

#ifdef WRSPICE
    if (Sp.GetFlag(FT_STRICTNUM) && *here) {
        if (!units && *here == Sp.UnitsCatchar() && gobble)
            get_unitstr(&here, buf);
        if (*here)
            return (0);
    }
    else if (gobble) {
        if (!units)
            get_unitstr(&here, buf);
    }
#else
    get_unitstr(&here, buf);
#endif

    expo1 += expsgn*expo2;
    if (expo1 >= 0) {
        if (expo1 < ETABSIZE)
            np_num = sign*mantis*np_powers[expo1];
        else
            np_num = sign*mantis*pow(10.0, (double)expo1);
    }
    else {
        if (-expo1 < ETABSIZE)
            np_num = sign*mantis/np_powers[-expo1];
        else
            np_num = sign*mantis*pow(10.0, (double)expo1);
    }
    *line = here;
#ifdef WRSPICE
    if (Parser::Debug)
        GRpkgIf()->ErrPrintf(ET_MSGS, "numparse: got %e, left = %s\n", np_num,
            here);
#endif
    return (&np_num);
}


// Static function.
// Grab a units string into buf, advance the pointer.  The buf length
// is limited to 32 chars including the null byte.
//
bool
sSPnumber::get_unitstr(const char **s, char *buf)
{
    int i = 0;
    const char *t = *s;
    bool had_alpha = false;
    bool had_cat = false;
    if (isalpha(*t) || *t == UNITS_CATCHAR() || *t == UNITS_SEPCHAR()) {
        if (*t != UNITS_CATCHAR()) {
            buf[i++] = *t;
            if (*t != UNITS_SEPCHAR())
                had_alpha = true;
        }
        for (t++; i < 31; t++) {
            if (isalpha(*t)) {
                had_alpha = true;
                buf[i++] = *t;
                continue;
            }
            if (*t == UNITS_SEPCHAR()) {
                had_alpha = false;
                buf[i++] = *t;
                continue;
            }
            if (had_alpha && isdigit(*t)) {
                buf[i++] = *t;
                continue;
            }
            if (!had_cat && *t == UNITS_CATCHAR()) {
                had_alpha = false;
                buf[i++] = UNITS_SEPCHAR();
                continue;
            }
            break;
        }
        buf[i] = 0;
        *s = t;
        return (true);
    }
    return (false);
}


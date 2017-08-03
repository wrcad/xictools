
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
 * Misc. Utilities Library                                                *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#include "lstring.h"
#include "texttf.h"

//
// Functions to convert an xform text label flags word to a string
// representation, and back.
//
// NOTE:  The keywords must not begin with a-f so we can differentiate
// a hex number from a text token.


// Return a string consisting of comma-separated text tokens which
// represent the xform bits.  A zero xform returns a null string.
//
char *xform_to_string(unsigned int xform)
{
    const char *sep = ",";
    sLstr lstr;
    switch (TXTF_ROT & xform) {
    case 0:
        if (xform & TXTF_45)
            lstr.append(sep, "R45");
        break;
    case 1:
        if (xform & TXTF_45)
            lstr.append(sep, "R135");
        else
            lstr.append(sep, "R90");
        break;
    case 2:
        if (xform & TXTF_45)
            lstr.append(sep, "R225");
        else
            lstr.append(sep, "R180");
        break;
    case 3:
        if (xform & TXTF_45)
            lstr.append(sep, "R315");
        else
            lstr.append(sep, "R270");
        break;
    }
    if (xform & TXTF_MY)
        lstr.append(sep, "MY");
    if (xform & TXTF_MX)
        lstr.append(sep, "MX");
    if (xform & TXTF_HJC)
        lstr.append(sep, "HJC");
    if (xform & TXTF_HJR)
        lstr.append(sep, "HJR");
    if (xform & TXTF_VJC)
        lstr.append(sep, "VJC");
    if (xform & TXTF_VJT)
        lstr.append(sep, "VJT");
    switch (TXTF_FONT_INDEX(xform)) {
    case 0:
        break;
    case 1:
        lstr.append(sep, "T1");
        break;
    case 2:
        lstr.append(sep, "T2");
        break;
    case 3:
        lstr.append(sep, "T3");
        break;
    }
    if (xform & TXTF_SHOW)
        lstr.append(sep, "SHOW");
    if (xform & TXTF_HIDE)
        lstr.append(sep, "HIDE");
    if (xform & TXTF_TLEV)
        lstr.append(sep, "TLEV");
    if (xform & TXTF_LIML)
        lstr.append(sep, "LIML");

    if (!lstr.string())
        lstr.add("0");
    return (lstr.string_trim());
}


// Given a string in the format produced by the function above, parse
// the tokens and return an xform.  Hex numbers are also accepted. 
// Unrecognized tokens are silently ignored.
//
unsigned int string_to_xform(const char *str)
{
    if (!str || !*str)
        return (0);
    unsigned int xform = 0;
    unsigned rmask = TXTF_ROT | TXTF_45;
    char *tok;
    while ((tok = lstring::gettok(&str, ",")) != 0) {
        if (!strcasecmp(tok, "R0")) {
            xform &= ~rmask;
        }
        else if (!strcasecmp(tok, "R45")) {
            xform &= ~rmask;
            xform |= TXTF_45;
        }
        else if (!strcasecmp(tok, "R90")) {
            xform &= ~rmask;
            xform |= 1;
        }
        else if (!strcasecmp(tok, "R135")) {
            xform &= ~rmask;
            xform |= 1 | TXTF_45;
        }
        else if (!strcasecmp(tok, "R180")) {
            xform &= ~rmask;
            xform |= 2;
        }
        else if (!strcasecmp(tok, "R225")) {
            xform &= ~rmask;
            xform |= 2 | TXTF_45;
        }
        else if (!strcasecmp(tok, "R270")) {
            xform &= ~rmask;
            xform |= 3;
        }
        else if (!strcasecmp(tok, "R315")) {
            xform &= ~rmask;
            xform |= 3 | TXTF_45;
        }
        else if (!strcasecmp(tok, "MY"))
            xform |= TXTF_MY;
        else if (!strcasecmp(tok, "MX"))
            xform |= TXTF_MX;
        else if (!strcasecmp(tok, "HJL"))
            xform &= ~(TXTF_HJC | TXTF_HJR);
        else if (!strcasecmp(tok, "HJC")) {
            xform |= TXTF_HJC;
            xform &= ~TXTF_HJR;
        }
        else if (!strcasecmp(tok, "HJR")) {
            xform &= ~TXTF_HJC;
            xform |= TXTF_HJR;
        }
        else if (!strcasecmp(tok, "VJB"))
            xform &= ~(TXTF_VJC | TXTF_VJT);
        else if (!strcasecmp(tok, "VJC")) {
            xform |= TXTF_VJC;
            xform &= ~TXTF_VJT;
        }
        else if (!strcasecmp(tok, "VJT")) {
            xform &= ~TXTF_VJC;
            xform |= TXTF_VJT;
        }
        else if (!strcasecmp(tok, "T0")) {
            xform &= ~TXTF_FNT;
        }
        else if (!strcasecmp(tok, "T1")) {
            xform &= ~TXTF_FNT;
            xform |= 0x200;
        }
        else if (!strcasecmp(tok, "T2")) {
            xform &= ~TXTF_FNT;
            xform |= 0x400;
        }
        else if (!strcasecmp(tok, "T3")) {
            xform &= ~TXTF_FNT;
            xform |= 0x600;
        }
        else if (!strcasecmp(tok, "SHOW")) {
            xform |= TXTF_SHOW;
            xform &= ~TXTF_HIDE;
        }
        else if (!strcasecmp(tok, "HIDE")) {
            xform &= ~TXTF_SHOW;
            xform |= TXTF_HIDE;
        }
        else if (!strcasecmp(tok, "TLEV"))
            xform |= TXTF_TLEV;
        else if (!strcasecmp(tok, "LIML"))
            xform |= TXTF_LIML;

        else if (isxdigit(*tok)) {
            // We also accept a hex integer token for backwards
            // compatibility.

            char *t = tok;
            if (t[0] == '0' && (t[1] == 'x' || t[1] == 'X'))
                t += 2;
            unsigned int d;
            sscanf(t, "%x", &d);
            xform |= d;
        }
        delete [] tok;
    }
    return (xform);
}


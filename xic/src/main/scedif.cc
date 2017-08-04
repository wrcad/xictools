
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

#include "main.h"
#include "scedif.h"


cScedIf *cScedIf::instancePtr = 0;

cScedIf::cScedIf()
{
    if (instancePtr) {
        fprintf(stderr, "Singleton class cScedIf already instantiated.\n");
        exit(1);
    }
    instancePtr = this;
}


// Private static error exit.
//
void
cScedIf::on_null_ptr()
{
    fprintf(stderr, "Singleton class cScedIf used before instantiated.\n");
    exit(1);
}


// Parse the string and return the indicated electrical property
// number.  If doall is not null, also look for "all".
//
int
cScedIf::whichPrpty(const char *s, bool *doall)
{
    int val = 0;
    if (doall)
        *doall = false;
    if (s) {
        // 1 model   m
        // 2 value   v
        // 3 param   p
        // 4 other   o
        // 11 name   n
        // 5 nophys  y, nop
        // 6 virtual t, vi
        // 7 flatten f
        // 8 range   r
        // 10 node   nod
        // 18 symblc s, nos
        // 20 macro  c, ma
        // 21 devref d

        if (lstring::ciprefix(s, KW_MODEL))
            val = P_MODEL;
        else if (lstring::ciprefix(s, KW_VALUE))
            val = P_VALUE;
        else if (lstring::ciprefix(s, KW_PARAM) ||
                lstring::ciprefix(s, KW_INITC))
            val = P_PARAM;
        else if (lstring::ciprefix(s, KW_OTHER))
            val = P_OTHER;
        else if (lstring::ciprefix(s, KW_NAME))
            val = P_NAME;
        else if (lstring::ciprefix(s, KW_NOPHYS) || *s == 'y' || *s == 'Y')
            val = P_NOPHYS;
        else if (lstring::ciprefix(s, KW_VIRTUAL) || *s == 't' || *s == 'T')
            val = P_VIRTUAL;
        else if (lstring::ciprefix(s, KW_FLATTEN))
            val = P_FLATTEN;
        else if (lstring::ciprefix(s, KW_RANGE))
            val = P_RANGE;
        else if (lstring::ciprefix(s, KW_NODE))
            val = P_NODE;
        else if (lstring::ciprefix(s, KW_NOSYMB) || *s == 's' || *s == 'S')
            val = P_SYMBLC;
        else if (lstring::ciprefix(s, KW_MACRO) || *s == 'c' || *s == 'C')
            val = P_MACRO;
        else if (lstring::ciprefix(s, KW_DEVREF))
            val = P_DEVREF;
        else if (doall && lstring::ciprefix(s, "all"))
            *doall = true;
    }
    return (val);
}


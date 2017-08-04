
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

#include "ext_fxunits.h"
#include "lstring.h"


//
// Units selection for the FastCap/FastHenry interface.
//

namespace {
    unit_t Units[] =
    {
        unit_t("m",     "%10.3e", 10, 1e-9,  1e-6,  1.0),
        unit_t("cm",    "%10.3e", 10, 1e-7,  1e-4,  1e-2),
        unit_t("mm",    "%10.3e", 10, 1e-6,  1e-3,  1e-3),
        unit_t("um",    "%-8.3f", 8,  1e-3,  1.0,   1e-6),
        unit_t("in",    "%10.3e", 10, 1e-6/25.4,  1e-3/25.4,  1e-3*25.4),
        unit_t("mils",  "%10.3e", 10, 1e-3/25.4,  1.0/25.4,  1e-6*25.4)
    };
}


// Static function.
int
unit_t::find_unit(const char *str)
{
    if (str) {
        if (lstring::ciprefix("me", str))
            str = "m";
        else if (lstring::ciprefix("c", str))
            str = "cm";
        else if (lstring::ciprefix("millim", str))
            str = "mm";
        else if (lstring::ciprefix("mic", str))
            str = "um";
        else if (lstring::ciprefix("i", str))
            str = "in";
        else if (lstring::ciprefix("mil", str))
            str = "mils";
        unsigned int sz = sizeof(Units)/sizeof(unit_t);
        for (unsigned int i = 0; i < sz; i++) {
            if (!strcasecmp(Units[i].name(), str))
                return (i);
        }
    }
    return (-1);
}


// Static function.
//
const unit_t *
unit_t::units(e_unit u)
{
    return (Units + u);
}


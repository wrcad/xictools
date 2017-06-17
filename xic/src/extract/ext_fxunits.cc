
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2007 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *  WHITELEY RESEARCH INCORPORATED PROPRIETARY SOFTWARE                   *
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
 *                                                                        *
 * XIC Integrated Circuit Layout and Schematic Editor                     *
 *                                                                        *
 *========================================================================*
 $Id: ext_fxunits.cc,v 5.2 2014/06/12 04:28:39 stevew Exp $
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


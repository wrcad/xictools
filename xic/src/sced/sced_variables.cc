
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
#include "sced.h"
#include "menu.h"
#include "ebtn_menu.h"
#include "cd_netname.h"
#include "tech.h"
#include "errorlog.h"


//-----------------------------------------------------------------------------
// Variables

namespace {
    void
    devs_postset(const char*)
    {
        SCD()->PopUpDevs(0, MODE_UPD);
    }


    void
    dots_postset(const char*)
    {
        SCD()->PopUpDots(0, MODE_UPD);
    }


    void
    sced_postset(const char*)
    {
        SCD()->PopUpSpiceIf(0, MODE_UPD);
    }


    bool
    evDevMenuStyle(const char *vstring, bool set)
    {
        if (set) {
            if (!vstring || (strcmp(vstring, "0") && strcmp(vstring, "1") &&
                    strcmp(vstring, "2"))) {
                Log()->ErrorLog(mh::Variables,
                    "Incorrect DevMenuStyle: must be 0-2.");
                return (false);
            }
        }
        CDvdb()->registerPostFunc(devs_postset);
        return (true);
    }


    bool
    evShowDots(const char *vstring, bool set)
    {
        if (set) {
            if (vstring && (*vstring == 'a' || *vstring == 'A'))
                SCD()->setShowDots(DotsAll);
            else
                SCD()->setShowDots(DotsSome);
            SCD()->recomputeDots();
        }
        else {
            SCD()->setShowDots(DotsNone);
            SCD()->clearDots();
        }
        DSP()->RedisplayAll(Electrical);
        CDvdb()->registerPostFunc(dots_postset);
        return (true);
    }


    bool
    evSpiceSubcCatchar(const char *vstring, bool set)
    {
        if (set) {
            if (vstring && *vstring && !isspace(*vstring) && !vstring[1])
                CD()->SetSubcCatchar(*vstring);
            else {
                Log()->ErrorLog(mh::Variables,
            "Incorrect SpiceSubcCatchar: must be single printing character.");
                return (false);
            }
        }
        else
            CD()->SetSubcCatchar(DEF_SUBC_CATCHAR);
        CDvdb()->registerPostFunc(sced_postset);
        return (true);
    }


    bool
    evSpiceSubcCatmode(const char *vstring, bool set)
    {
        if (set) {
            if (vstring && lstring::cieq(vstring, "wrspice"))
                CD()->SetSubcCatmode(cCD::SUBC_CATMODE_WR);
            else if (vstring && lstring::cieq(vstring, "spice3"))
                CD()->SetSubcCatmode(cCD::SUBC_CATMODE_SPICE3);
            else {
                Log()->ErrorLog(mh::Variables,
            "Incorrect SpiceSubcCatmode: must be \"wrspice\" or \"spice3\".");
                return (false);
            }
        }
        else
            CD()->SetSubcCatmode(cCD::SUBC_CATMODE_WR);
        CDvdb()->registerPostFunc(sced_postset);
        return (true);
    }


    bool
    evSpice(const char*, bool)
    {
        CDvdb()->registerPostFunc(sced_postset);
        return (true);
    }


    bool
    evSubscripting(const char *vstring, bool set)
    {
        if (XM()->IsAppInitDone() || Tech()->HaveTechfile()) {
            Log()->ErrorLog(mh::Variables,
                "Subscripting can not be changed after startup.");
            return (false);
        }
        if (set) {
            if (vstring) {
                char c = isupper(*vstring) ? tolower(*vstring) : *vstring;
                if (c == 'a')
                    cTnameTab::subscr_set('<', '>');
                else if (c == 's')
                    cTnameTab::subscr_set('[', ']');
                else if (c == 'c')
                    cTnameTab::subscr_set('{', '}');
                else
                    vstring = 0;
            }
            if (!vstring) {
                Log()->ErrorLog(mh::Variables,
            "Incorrect Subscripting: must be a[ngle], s[quare], or c[urly].");
                return (false);
            }
        }
        else
            cTnameTab::subscr_set(DEF_SUBSCR_OPEN, DEF_SUBSCR_CLOSE);
        return (true);
    }


    bool
    evNetNamesCaseSens(const char*, bool set)
    {
        if (XM()->IsAppInitDone() || Tech()->HaveTechfile()) {
            Log()->ErrorLog(mh::Variables,
                "NetNamesCaseSens can not be changed after startup.");
            return (false);
        }
        cTnameTab::set_case_insensitive_mode(!set);
        return (true);
    }
}


#define B 'b'
#define S 's'

namespace {
    void vsetup(const char *vname, char c, bool(*fn)(const char*, bool))
    {
        CDvdb()->registerInternal(vname,  fn);
        if (c == B)
            Tech()->RegisterBooleanAttribute(vname);
        else if (c == S)
            Tech()->RegisterStringAttribute(vname);
    }
}


void
cSced::setupVariables()
{
    // Side Menu Commands
    vsetup(VA_DevMenuStyle,        S,   evDevMenuStyle);

    // Attributes Menu
    vsetup(VA_ShowDots,            S,   evShowDots);

    // SPICE Interface
    vsetup(VA_SpiceListAll,        B,   evSpice);
    vsetup(VA_SpiceAlias,          S,   evSpice);
    vsetup(VA_SpiceHost,           S,   evSpice);
    vsetup(VA_SpiceHostDisplay,    S,   evSpice);
    vsetup(VA_SpiceProg,           S,   evSpice);
    vsetup(VA_SpiceExecDir,        S,   evSpice);
    vsetup(VA_SpiceExecName,       S,   evSpice);
    vsetup(VA_SpiceSubcCatchar,    S,   evSpiceSubcCatchar);
    vsetup(VA_SpiceSubcCatmode,    S,   evSpiceSubcCatmode);
    vsetup(VA_CheckSolitary,       B,   evSpice);
    vsetup(VA_NoSpiceTools,        B,   evSpice);

    // Misc.
    vsetup(VA_Subscripting,        0,   evSubscripting);
    vsetup(VA_NetNamesCaseSens,    0,   evNetNamesCaseSens);
}



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
 * Xic Integrated Circuit Layout and Schematic Editor                     *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#include "main.h"
#include "drc.h"
#include "errorlog.h"
#include "tech.h"
#include "menu.h"
#include "drc_menu.h"


//-----------------------------------------------------------------------------
// Setup variables for DRC system.

namespace {
    // A smarter atoi().
    //
    inline bool
    str_to_int(int *iret, const char *s)
    {
        if (!s)
            return (false);
        char *e;
        *iret = strtol(s, &e, 0);
        return (e != s);
    }

    // A smarter atof()
    //
    inline bool
    str_to_dbl(double *dret, const char *s)
    {
        if (!s)
            return (false);
        return (sscanf(s, "%lf", dret) == 1);
    }

    void
    postset(const char*)
    {
        DRC()->PopUpDrcLimits(0, MODE_UPD);
    }

    void
    postrun(const char*)
    {
        DRC()->PopUpDrcRun(0, MODE_UPD);
    }

    bool
    evDrc(const char*, bool set)
    {
        DRC()->setInteractive(set);
        Menu()->MenuButtonSet(MenuDRC, MenuINTR, DRC()->isInteractive());
        return (true);
    }

    bool
    evDrcLevel(const char *vstring, bool set)
    {
        if (set) {
            int i;
            if (str_to_int(&i, vstring) && i >= 0 && i <= 2)
                DRC()->setErrorLevel((DRClevelType)i);
            else {
                Log()->ErrorLogV(mh::Variables,
                    "Incorrect DrcLevel, range %d - %d.", 0, 2);
                return (false);
            }
        }
        else
            DRC()->setErrorLevel(DRC_ERR_LEVEL_DEF);
        CDvdb()->registerPostFunc(postset);
        return (true);
    }

    bool
    evDrcNoPopup(const char*, bool set)
    {
        DRC()->setIntrNoErrMsg(set);
        CDvdb()->registerPostFunc(postset);
        Menu()->MenuButtonSet(MenuDRC, MenuNOPOP, DRC()->isIntrNoErrMsg());
        return (true);
    }

    bool
    evDrcMaxErrors(const char *vstring, bool set)
    {
        if (set) {
            int i;
            if (str_to_int(&i, vstring) && i >= DRC_MAX_ERRS_MIN &&
                    i <= DRC_MAX_ERRS_MAX)
                DRC()->setMaxErrors(i);
            else {
                Log()->ErrorLogV(mh::Variables,
                    "Incorrect DrcMaxErrors, range %d - %d.",
                    DRC_MAX_ERRS_MIN, DRC_MAX_ERRS_MAX);
                return (false);
            }
        }
        else
            DRC()->setMaxErrors(DRC_MAX_ERRS_DEF);
        CDvdb()->registerPostFunc(postset);
        return (true);
    }

    bool
    evDrcInterMaxObjs(const char *vstring, bool set)
    {
        if (set) {
            int i;
            if (str_to_int(&i, vstring) && i >= DRC_INTR_MAX_OBJS_MIN &&
                    i <= DRC_INTR_MAX_OBJS_MAX)
                DRC()->setIntrMaxObjs(i);
            else {
                Log()->ErrorLogV(mh::Variables,
                    "Incorrect DrcInterMaxObjs, range %d - %d.",
                    DRC_INTR_MAX_OBJS_MIN, DRC_INTR_MAX_OBJS_MAX);
                return (false);
            }
        }
        else
            DRC()->setIntrMaxObjs(DRC_INTR_MAX_OBJS_DEF);
        CDvdb()->registerPostFunc(postset);
        return (true);
    }

    bool
    evDrcInterMaxTime(const char *vstring, bool set)
    {
        if (set) {
            int i;
            if (str_to_int(&i, vstring) && i >= DRC_INTR_MAX_TIME_MIN &&
                    i <= DRC_INTR_MAX_TIME_MAX)
                DRC()->setIntrMaxTime(i);
            else {
                Log()->ErrorLogV(mh::Variables,
                    "Incorrect DrcInterMaxTime, range %d - %d.",
                    DRC_INTR_MAX_TIME_MIN, DRC_INTR_MAX_TIME_MAX);
                return (false);
            }
        }
        else
            DRC()->setIntrMaxTime(DRC_INTR_MAX_TIME_DEF);
        CDvdb()->registerPostFunc(postset);
        return (true);
    }

    bool
    evDrcInterMaxErrors(const char *vstring, bool set)
    {
        if (set) {
            int i;
            if (str_to_int(&i, vstring) && i >= DRC_INTR_MAX_ERRS_MIN &&
                    i <= DRC_INTR_MAX_ERRS_MAX)
                DRC()->setIntrMaxErrors(i);
            else {
                Log()->ErrorLogV(mh::Variables,
                    "Incorrect DrcInterMaxErrors, range %d - %d.",
                    DRC_INTR_MAX_ERRS_MIN, DRC_INTR_MAX_ERRS_MAX);
                return (false);
            }
        }
        else
            DRC()->setIntrMaxErrors(DRC_INTR_MAX_ERRS_DEF);
        CDvdb()->registerPostFunc(postset);
        return (true);
    }

    bool
    evDrcInterSkipInst(const char*, bool set)
    {
        DRC()->setIntrSkipInst(set);
        CDvdb()->registerPostFunc(postset);
        return (true);
    }

    bool
    evDrcLayerList(const char *vstring, bool set)
    {
        DRC()->setLayerList(set ? vstring : 0);
        CDvdb()->registerPostFunc(postset);
        return (true);
    }

    bool
    evDrcUseLayerList(const char *vstring, bool set)
    {
        if (set) {
            if (*vstring == 'n' || *vstring == 'N')
                DRC()->setUseLayerList(DrcListSkip);
            else
                DRC()->setUseLayerList(DrcListOnly);
        }
        else
            DRC()->setUseLayerList(DrcListNone);
        CDvdb()->registerPostFunc(postset);
        return (true);
    }

    bool
    evDrcRuleList(const char *vstring, bool set)
    {
        DRC()->setRuleList(set ? vstring : 0);
        CDvdb()->registerPostFunc(postset);
        return (true);
    }

    bool
    evDrcUseRuleList(const char *vstring, bool set)
    {
        if (set) {
            if (*vstring == 'n' || *vstring == 'N')
                DRC()->setUseRuleList(DrcListSkip);
            else
                DRC()->setUseRuleList(DrcListOnly);
        }
        else
            DRC()->setUseRuleList(DrcListNone);
        CDvdb()->registerPostFunc(postset);
        return (true);
    }

    bool
    evDrcChd(const char*, bool)
    {
        CDvdb()->registerPostFunc(postrun);
        return (true);
    }

    bool
    evDrcPartitionSize(const char *vstring, bool set)
    {
        if (set) {
            double d;
            if (str_to_dbl(&d, vstring) &&
                    INTERNAL_UNITS(d) >= INTERNAL_UNITS(DRC_PART_MIN) &&
                    INTERNAL_UNITS(d) <= INTERNAL_UNITS(DRC_PART_MAX))
                DRC()->setGridSize(INTERNAL_UNITS(d));
            else {
                Log()->ErrorLogV(mh::Variables,
                    "Incorrect DrcPartitionSize: must be %.2f-%.2f.",
                    DRC_PART_MIN, DRC_PART_MAX);
                return (false);
            }
        }
        else
            DRC()->setGridSize(0);
        CDvdb()->registerPostFunc(postrun);
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


// Called on program startup.
//
void
cDRC::setupVariables()
{
    vsetup(VA_Drc,                  B,  evDrc);
    vsetup(VA_DrcLevel,             S,  evDrcLevel);
    vsetup(VA_DrcNoPopup,           B,  evDrcNoPopup);
    vsetup(VA_DrcMaxErrors,         S,  evDrcMaxErrors);
    vsetup(VA_DrcInterMaxObjs,      S,  evDrcInterMaxObjs);
    vsetup(VA_DrcInterMaxTime,      S,  evDrcInterMaxTime);
    vsetup(VA_DrcInterMaxErrors,    S,  evDrcInterMaxErrors);
    vsetup(VA_DrcInterSkipInst,     B,  evDrcInterSkipInst);
    vsetup(VA_DrcLayerList,         S,  evDrcLayerList);
    vsetup(VA_DrcUseLayerList,      S,  evDrcUseLayerList);
    vsetup(VA_DrcRuleList,          S,  evDrcRuleList);
    vsetup(VA_DrcUseRuleList,       S,  evDrcUseRuleList);
    vsetup(VA_DrcChdName,           S,  evDrcChd);
    vsetup(VA_DrcChdCell,           S,  evDrcChd);
    vsetup(VA_DrcPartitionSize,     S,  evDrcPartitionSize);
}


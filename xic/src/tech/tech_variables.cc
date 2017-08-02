
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

#include "dsp.h"
#include "dsp_window.h"
#include "dsp_inlines.h"
#include "geo.h"
#include "tech.h"
#include "tech_extract.h"
#include "tech_ldb3d.h"
#include "errorlog.h"
#include "main_variables.h"


//
// Set up and handle Xic and technology variables.
//


// Set a variable in the tech database.
//
void
cTech::SetTechVariable(const char *name, const char *val)
{
    if (!name || !*name)
        return;
    if (!val)
        val = "";

    if (!tc_variable_tab)
        tc_variable_tab = new SymTab(true, true);
    else {
        SymTabEnt *h = SymTab::get_ent(tc_variable_tab, name);
        if (h) {
            delete [] (char*)h->stData;
            h->stData = lstring::copy(val);
            return;
        }
    }
    tc_variable_tab->add(lstring::copy(name), lstring::copy(val), false);
}


void
cTech::ClearTechVariable(const char *name)
{
    if (name && tc_variable_tab)
        tc_variable_tab->remove(name);
}


// Return a list of Xic variables that are set, but filter out those
// that would be set by a keyword.
//
stringlist *
cTech::VarList()
{
    stringlist *sl0 = CDvdb()->listVariables();
    stringlist *sp = 0, *sn;
    for (stringlist *sl = sl0; sl; sl = sn) {
        sn = sl->next;
        char *s = sl->string;
        char *name = lstring::gettok(&s);
        if (
                (tc_battr_tab && tc_battr_tab->find(name)) ||
                (tc_sattr_tab && tc_sattr_tab->find(name)) ||
                !strcasecmp(name, VA_Path) ||
                !strcasecmp(name, VA_LibPath) ||
                !strcasecmp(name, VA_ScriptPath) ||
                !strcasecmp(name, VA_HelpPath)) {

            if (sp)
                sp->next = sn;
            else
                sl0 = sn;
            delete [] sl->string;
            delete sl;
        }
        else
            sp = sl;
        delete [] name;
    }
    return (sl0);
}


// Substitute recursively in place for $(name) constructs, whcih can
// be tech or environment variables.  Return false and set emesg if
// error, scnt is the number of substitutions performed.
//
bool
cTech::VarSubst(char *line, char **emesg, int *scnt)
{
    char buf[TECH_BUFSIZE];
    char *s = line, *t;
    int cnt = 0;
    while ((t = strchr(s, '$')) != 0 && *(t+1) == '(') {
        char *end;
        for (end = t+2; *end && *end != ')'; end++) ;
        if (!*end) {
            sprintf(buf, "Substitution syntax error");
            if (emesg)
                *emesg = lstring::copy(buf);
            if (scnt)
                *scnt = 0;
            return (false);
        }
        *end = '\0';

        const char *sub = 0;
        if (tc_variable_tab) {
            sub = (const char*)SymTab::get(tc_variable_tab, t+2);
            if (sub == (const char*)ST_NIL)
                sub = 0;
        }
        if (!sub) {
            // If the variable is found in the environment, add it to
            // the list.
            sub = getenv(t+2);
            if (!sub) {
                sprintf(buf, "Unresolved name %s", t+2);
                *end = ')';
                if (emesg)
                    *emesg = lstring::copy(buf);
                if (scnt)
                    *scnt = 0;
                return (false);
            }
        }
        s = t;  // check the new text
        strcpy(buf, end+1);
        strncpy(t, sub, TECH_BUFSIZE - (t - line));
        line[TECH_BUFSIZE-1] = '\0';
        while (*t)
            t++;
        strncpy(t, buf, TECH_BUFSIZE - (t - line));
        line[TECH_BUFSIZE-1] = '\0';
        cnt++;
    }
    if (emesg)
        *emesg = 0;
    if (scnt)
        *scnt = cnt;
    return (true);
}


// Substitute in place for eval(....) constructs.  Return false if
// error, scnt is the number of substitutions performed.
//
bool
cTech::EvaluateEval(char *line, int *scnt)
{
    if (!tc_parse_eval) {
        Log()->ErrorLogV(mh::Techfile,
            "Tech parser not configured for script evaluation.\n");
        return (false);
    }

    char outbuf[512];
    char tbuf[TECH_BUFSIZE];
    char *s = line;
    char *t = tbuf;
    int cnt = 0;
    bool ok = true;
    while (*s) {
        if (lstring::prefix("eval(", s) && (s == line || !isalnum(*(s-1)))) {
            s += 4;
            if (!(*tc_parse_eval)(&s, outbuf)) {
                ok = false;
                cnt = 0;
                break;
            }
            else {
                char *c = outbuf;
                while (*c)
                    *t++ = *c++;
                cnt++;
            }
        }
        else
            *t++ = *s++;
    }
    *t = 0;
    if (ok && cnt)
        strcpy(line, tbuf);
    if (scnt)
        *scnt = cnt;
    return (ok);
}


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


    // A smarter atof().
    //
    inline bool
    str_to_dbl(double *dret, const char *s)
    {
        if (!s)
            return (false);
        return (sscanf(s, "%lf", dret) == 1);
    }
}


//--------------------------------------------------------------------------
// Extraction Tech Variables
//
// These provide some basic tech compatibility when the extraction
// system is not present.  Most of these actions are overridden in the
// extraction system.
//
namespace {
    bool
    evAntennaTotal(const char *vstring, bool set)
    {
        if (set) {
            double d;
            if (str_to_dbl(&d, vstring) && d >= 0.0)
                Tech()->SetAntennaTotal(d);
            else {
                Log()->ErrorLog(mh::Variables,
                    "Incorrect AntennaTotal, real >= 0.0.");
                return (false);
            }
        }
        else
            Tech()->SetAntennaTotal(0.0);
        return (true);
    }

    bool
    evDb3ZoidLimit(const char *vstring, bool set)
    {
        if (set) {
            int i;
            if (str_to_int(&i, vstring) && i >= 1000)
                Ldb3d::set_zoid_limit(i);
            else {
                Log()->ErrorLog(mh::Variables,
                    "Incorrect Db3ZoidLimit, integer >= 1000.");
                return (false);
            }
        }
        else
            Ldb3d::set_zoid_limit(DB3_DEF_ZLIMIT);
        return (true);
    }

    bool
    evLayerReorderMode(const char *str, bool set)
    {
        if (set) {
            int i;
            if (str_to_int(&i, str) && i >= 0 && i <= 2)
                Tech()->SetReorderMode((tReorderMode)i);
            else {
                Log()->ErrorLog(mh::Variables,
                    "Incorrect ReorderMode: range 0-2.");
                return (false);
            }
        }
        else
            Tech()->SetReorderMode(tReorderNone);
        return (true);
    }

    bool
    evNoPlanarize(const char*, bool set)
    {
        Tech()->SetNoPlanarize(set);
        return (true);
    }

    bool
    evSubstrateEps(const char *vstring, bool set)
    {
        if (set) {
            double d;
            if (str_to_dbl(&d, vstring) && d >= SUBSTRATE_EPS_MIN &&
                    d <= SUBSTRATE_EPS_MAX )
                Tech()->SetSubstrateEps(d);
            else {
                Log()->ErrorLogV(mh::Variables,
                    "Incorrect SubstrateEps, real %.3f - %.3f.",
                    SUBSTRATE_EPS_MIN, SUBSTRATE_EPS_MAX);
                return (false);
            }
        }
        else
            Tech()->SetSubstrateEps(SUBSTRATE_EPS);
        return (true);
    }

    bool
    evSubstrateThickness(const char *vstring, bool set)
    {
        if (set) {
            double d;
            if (str_to_dbl(&d, vstring) && d >= SUBSTRATE_THICKNESS_MIN &&
                    d <= SUBSTRATE_THICKNESS_MAX )
                Tech()->SetSubstrateThickness(d);
            else {
                Log()->ErrorLogV(mh::Variables, 
                    "Incorrect SubstrateThickness, real %.3f - %.3f.",
                    SUBSTRATE_THICKNESS_MIN, SUBSTRATE_THICKNESS_MAX);
                return (false);
            }
        }
        else
            Tech()->SetSubstrateThickness(SUBSTRATE_THICKNESS);
        return (true);
    }
}


//--------------------------------------------------------------------------
// Extraction Support
//
namespace {
    bool
    evGroundPlaneGlobal(const char*, bool set)
    {
        Tech()->SetGroundPlaneGlobal(set);
        return (true);
    }

    bool
    evGroundPlaneMulti(const char*, bool set)
    {
        Tech()->SetInvertGroundPlane(set);
        return (true);
    }

    bool
    evGroundPlaneMethod(const char *vstring, bool set)
    {
        if (set) {
            int i;
            if (str_to_int(&i, vstring) && i >= 0 && i <= 2)
                Tech()->SetGroundPlaneMode((GPItype)i);
            else {
                Log()->ErrorLog(mh::Variables,
                    "Incorrect GroundPlaneMethod: range 0-2.");
                return (false);
            }
        }
        else
            Tech()->SetGroundPlaneMode(GPI_PLACE);
        return (true);
    }


    bool
    evBoxLinestyle(const char *vstring, bool set)
    {
        // Set the linestyle used to draw boxes in electrical mode,
        // also used in some commands in physical mode.

        if (set) {
            int i;
            if (str_to_int(&i, vstring)) {
                DSP()->BoxLinestyle()->mask = i;
                DSPmainDraw(defineLinestyle(DSP()->BoxLinestyle(),
                    DSP()->BoxLinestyle()->mask))
            }
            else {
                Log()->ErrorLogV(mh::Variables,
                    "Incorrect BoxLinestyle: must be an integer.");
                return (false);
            }
        }
        else {
            DSP()->BoxLinestyle()->mask = DEF_BoxLineStyle;
            DSPmainDraw(defineLinestyle(DSP()->BoxLinestyle(),
                DSP()->BoxLinestyle()->mask))
        }
        return (true);
    }

    bool
    evGridNoCoarseOnly(const char*, bool set)
    {
        DSP()->SetGridNoCoarseOnly(set);
        return (true);
    }

    bool
    evGridThreshold(const char *vstring, bool set)
    {
        if (set) {
            int i;
            if (str_to_int(&i, vstring) &&
                    i >= DSP_MIN_GRID_THRESHOLD &&
                    i <= DSP_MAX_GRID_THRESHOLD) {
                DSP()->SetGridThreshold(i);
           }
           else {
                Log()->ErrorLogV(mh::Variables,
                    "Incorrect GridThreshold: range is %d - %d.",
                    DSP_MIN_GRID_THRESHOLD, DSP_MAX_GRID_THRESHOLD);
                return (false);
           }
        }
        else {
            DSP()->SetGridThreshold(DSP_DEF_GRID_THRESHOLD);
        }
        return (true);
    }

    bool
    evRoundFlashSides(const char *vstring, bool set)
    {
        if (set) {
            int i;
            if (str_to_int(&i, vstring) && i >= MIN_RoundFlashSides &&
                    i <= MAX_RoundFlashSides) {
                GEO()->setPhysRoundFlashSides(i);
            }
            else {
                Log()->ErrorLogV(mh::Variables,
                    "Incorrect RoundFlashSides: range is %d - %d.",
                    MIN_RoundFlashSides, MAX_RoundFlashSides);
                return (false);
            }
        }
        else {
            GEO()->setPhysRoundFlashSides(DEF_RoundFlashSides);
        }
        return (true);
    }

    bool
    evElecRoundFlashSides(const char *vstring, bool set)
    {
        if (set) {
            int i;
            if (str_to_int(&i, vstring) && i >= MIN_RoundFlashSides &&
                    i <= MAX_RoundFlashSides) {
                GEO()->setElecRoundFlashSides(i);
            }
            else {
                Log()->ErrorLogV(mh::Variables,
                    "Incorrect ElecRoundFlashSides: range is %d - %d.",
                    MIN_RoundFlashSides, MAX_RoundFlashSides);
                return (false);
            }
        }
        else {
            GEO()->setElecRoundFlashSides(DEF_RoundFlashSides);
        }
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


// Register the internal variables and callbacks.  Called from
// constructor.
//
void
cTech::setupVariables()
{
    // Extraction Tech Variables
    vsetup(VA_AntennaTotal,         S,  evAntennaTotal);
    vsetup(VA_Db3ZoidLimit,         S,  evDb3ZoidLimit);
    vsetup(VA_LayerReorderMode,     S,  evLayerReorderMode);
    vsetup(VA_NoPlanarize,          B,  evNoPlanarize);
    vsetup(VA_SubstrateEps,         S,  evSubstrateEps);
    vsetup(VA_SubstrateThickness,   S,  evSubstrateThickness);

    // Extraction Support
    vsetup(VA_GroundPlaneGlobal,    B,  evGroundPlaneGlobal);
    vsetup(VA_GroundPlaneMulti,     B,  evGroundPlaneMulti);
    vsetup(VA_GroundPlaneMethod,    S,  evGroundPlaneMethod);

    // Don't bother hooking the Constrain45 variable, it is only used
    // when editing is available so is set up in edit_variables.cc.

    vsetup(VA_BoxLineStyle,         S,  evBoxLinestyle);
    vsetup(VA_GridNoCoarseOnly,     B,  evGridNoCoarseOnly);
    vsetup(VA_GridThreshold,        S,  evGridThreshold);

    vsetup(VA_RoundFlashSides,      S,  evRoundFlashSides);
    vsetup(VA_ElecRoundFlashSides,  S,  evElecRoundFlashSides);
}


// Parse name = value lines, set techfile variable.  Called from the
// tech file parser.
//
bool
cTech::vset(const char *line)
{
    const char *t = strchr(line, '=');
    if (!t)
        return (false);
    while (isspace(*line) && line < t)
        line++;
    if (line == t)
        return (false);
    const char *end = t-1;
    while (isspace(*end))
       end--;
    end++;
    const char *vstart = t+1;
    while (isspace(*vstart))
        vstart++;
    const char *vend = vstart + strlen(vstart) - 1;
    while (isspace(*vend) && vend >= vstart)
        vend--;
    vend++;

    int nlen = end - line;
    char *name = new char[nlen+1];
    strncpy(name, line, nlen);
    name[nlen] = 0;

    int vlen = vend - vstart;
    char *vsub = new char[vlen+1];
    strncpy(vsub, vstart, vlen);
    vsub[vlen] = 0;

    if (!tc_variable_tab)
        tc_variable_tab = new SymTab(true, true);
    else {
        SymTabEnt *h = SymTab::get_ent(tc_variable_tab, name);
        if (h) {
            delete [] (char*)h->stData;
            h->stData = vsub;
            return (true);
        }
    }
    tc_variable_tab->add(name, vsub, false);
    return (true);
}


// Set an Xic internal variable.  Called from the tech file parser. 
// We keep trailing white space, any newline must be stripped by
// caller.
//
bool
cTech::bangvset(const char *line)
{
    const char *s = line;
    while (isspace(*s))
        s++;
    if (!*s)
        return (false);
    char *tok = lstring::gettok(&s);
    while (isspace(*s))
        s++;
    CDvdb()->setVariable(tok, s);
    delete [] tok;
    return (true);
}



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
Authors: 1987 Thomas L. Quarles
         1993 Stephen R. Whiteley
****************************************************************************/

#include "frontend.h"
#include "input.h"
#include "inpmtab.h"
#include "misc.h"
#include "device.h"
#include "spnumber/spnumber.h"


sModTab::~sModTab()
{
    sHgen gen(this, true);
    sHent *h;
    while ((h = gen.next()) != 0) {
        delete (sINPmodel*)h->data();
        delete h;
    }
}
// End of sModTab functions.


namespace {
    // Return true if p matches s or p is a prefix of s followed by
    // '.', case insensitive.
    //
    inline bool
    mosmatch(const char *p, const char *s)
    {
        while (*p) {
            if ((isupper(*p) ? tolower(*p) : *p) !=
                    (isupper(*s) ? tolower(*s) : *s))
                return (false);
            p++;
            s++;
        }
        return (!*s || *s == '.');
    }
}


// Find and instantiate the model.  Return true if the model is found,
// or if we can use a default.  False is returned if the model is not
// found, and this should be considered fatal.
//
bool
SPinput::getMod(sLine *curline, sCKT *ckt, const char *name, const char *line,
    sINPmodel **model)
{
    if (model)
        *model = 0;
    if (!name || !*name)
        return (true);
    sINPmodel *m = findMod(name);
    if (m) {
        // found the model in question - now instantiate if necessary
        // and return an appropriate pointer to it
        if (instantiateMod(curline, m, ckt) && model) {
            *model = m;
            return (true);
        }
        // Failed to instantiate the model, fatal.
        return (false);
    }

    if (ip_badmodtab) {
        char *lcname = lstring::copy(name);
        lstring::strtolower(lcname);
        if (sHtab::get_ent(ip_badmodtab, lcname)) {
            delete [] lcname;
            return (false);
        }
        delete [] lcname;
    }

    if (curline && curline->line() && line) {
        const char *c = curline->line();
        while (isspace(*c))
            c++;
        if (*c == 'm' || *c == 'M') {
            // Look for binned model (MOS only)

            double l = -1;
            char *tok1 = lookParam("L", line);
            if (tok1) {
                const char *t = tok1;
                int err;
                l = getFloat(&t, &err, true);
                delete [] tok1;
                if (err) {
                    logError(curline, "Non-numeric value for \"L\"");
                    l = ckt ? ckt->mos_default_l() : CKT_DEF_MOS_L;
                }
            }
            else
                l = ckt ? ckt->mos_default_l() : CKT_DEF_MOS_L;

            double w = -1;
            char *tok2 = lookParam("W", line);
            if (tok2) {
                const char *t = tok2;
                int err;
                w = getFloat(&t, &err, true);
                delete [] tok2;
                if (err) {
                    logError(curline, "Non-numeric value for \"W\"");
                    w = ckt ? ckt->mos_default_w() : CKT_DEF_MOS_W;
                }
            }
            else
                w = ckt ? ckt->mos_default_w() : CKT_DEF_MOS_W;

            sINPmodel *mm = mosFind(curline, name, l, w);
            if (mm) {
                // Found the model in question - now instantiate
                // if necessary and return an appropriate pointer
                // to it.
                //
                if (instantiateMod(curline, mm, ckt) && model)
                    *model = mm;
                return (true);
            }
        }
    }

    // If the user gave a model name and it can't be resolved, it is a
    // fatal error.  Spice3 would allow a default model in this case.
    logError(curline, "Unable to find definition of model %s", name);
    return (false);
}


// Return true if the name matches a model name.  This is used to
// recognize model names when parsing a device line.  Return true even
// if the model is found in the "bad model" table.  We flag this as
// a fatal error later.
//
bool
SPinput::lookMod(const char *name)
{
    if (!name || !*name)
        return (false);
    if (findMod(name))
        return (true);

    if (ip_badmodtab) {
        char *lcname = lstring::copy(name);
        lstring::strtolower(lcname);
        if (sHtab::get_ent(ip_badmodtab, lcname)) {
            delete [] lcname;
            return (true);
        }
        delete [] lcname;
    }

    // Can't find a name match, try matching the base of the model name
    // for MOS model selection.

    sHent *h;
    sHgen gen(ip_modtab);
    while ((h = gen.next()) != 0) {
        sINPmodel *m = (sINPmodel*)h->data();
        if (mosmatch(name, m->modName))
            return (true);
    }
    gen = sHgen(ip_modcache);
    while ((h = gen.next()) != 0) {
        sINPmodel *m = (sINPmodel*)h->data();
        if (mosmatch(name, m->modName))
            return (true);
    }
    return (false);
}


// Parse a .model line, save the model.
//
void
SPinput::parseMod(sLine *curline)
{
    const char *line = curline->line();
    char *token = getTok(&line, true);  // throw away '.model'
    delete [] token;

    char *modname = getTok(&line, true);
    token = getTok(&line, true);
    if (!token) {
        delete [] modname;
        logError(curline, ".model syntax error (ignored)");
        return;
    }

    int lev = 0;
    bool gotlev = false;
    for (int i = 0; i < DEV.numdevs(); i++) {
        IFdevice *dev = DEV.device(i);
        if (dev && dev->modkeyMatch(token)) {
            if (!gotlev) {
                if (!findLev(line, &lev, dev->isMOS())) {
                    logError(curline,
                    "Illegal argument to level parameter - level=1 assumed");
                }
                gotlev = true;
            }
            if (dev->levelMatch(lev)) {
                // The hash key is always lower case, model name
                // resolution is case insensitive.

                if (lev > 1 && !dev->level(0)) {
                    logError(curline,
                        "Given level value ignored in this model");
                }

                char *lcname = lstring::copy(modname);
                lstring::strtolower(lcname);

                if (!ip_modtab)
                    ip_modtab = new sModTab;
                else if (sHtab::get(ip_modtab, lcname)) {
                    // Uh-oh, a model with this name has already been seen.
                    logError(curline,
                        ".model %s duplicate definition (ignored)", modname);
                    curline->comment_out();

                    delete [] lcname;
                    delete [] modname;
                    delete [] token;
                    return;
                }

                ip_modtab->add(lcname, new sINPmodel(lstring::copy(modname), i,
                    lstring::copy(curline->line())));
                delete [] lcname;
                delete [] modname;
                delete [] token;
                return;
            }
        }
        if (dev && dev->moduleNameMatch(token)) {
            // Match to the module name, ignore level.

            char *lcname = lstring::copy(modname);
            lstring::strtolower(lcname);

            if (!ip_modtab)
                ip_modtab = new sModTab;
            else if (sHtab::get(ip_modtab, lcname)) {
                // Uh-oh, a model with this name has already been seen.
                logError(curline,
                    ".model %s duplicate definition (ignored)", modname);
                curline->comment_out();

                delete [] lcname;
                delete [] modname;
                delete [] token;
                return;
            }

            ip_modtab->add(lcname, new sINPmodel(lstring::copy(modname), i,
                lstring::copy(curline->line())));
            delete [] lcname;
            delete [] modname;
            delete [] token;
            return;
        }
    }

    // Save the model name in a bad model table.  It will be a fatal
    // error if a device uses this model.
    if (!ip_badmodtab)
        ip_badmodtab = new sModTab;
    char *lcname = lstring::copy(modname);
    lstring::strtolower(lcname);
    ip_badmodtab->add(lcname, 0);
    delete [] lcname;

    if (lev > 1) {
        logError(curline, "Unknown model type %s with level=%d - ignored",
            token, lev);
    }
    else
        logError(curline, "Unknown model type %s - ignored", token);
    delete [] token;
    delete [] modname;
}


// Return true if the device type has a key that matches the arg.
//
bool
SPinput::checkKey(char key, int type)
{
    if (type >= 0 && type < DEV.numdevs()) {
        IFdevice *dv = DEV.device(type);
        if (dv)
            return (dv->keyMatch(key));
    }
    return (false);
}


//
// Private
//

// Find a model.  First look in the temporary table for the present
// circuit, then the persistent model cache.
//
sINPmodel *
SPinput::findMod(const char *name)
{
    // case insensitive reference
    char *lcname = lstring::copy(name);
    lstring::strtolower(lcname);
    sINPmodel *mod = (sINPmodel*)sHtab::get(ip_modtab, lcname);
    if (!mod)
        mod = (sINPmodel*)sHtab::get(ip_modcache, lcname);
    delete [] lcname;

    return (mod);
}


// Create/lookup a temporary model entry.
// 
int
SPinput::addMod(const char *token, int type, const char *text)
{
    if (!token || !*token)
        return (OK);

    // The hash key is always lower case, model name resolution is
    // case insensitive.
    char *lcname = lstring::copy(token);
    lstring::strtolower(lcname);

    if (sHtab::get(ip_modtab, lcname)) {
        delete [] lcname;
        return (OK);
    }

    if (!ip_modtab)
        ip_modtab = new sModTab;
    ip_modtab->add(lcname,
        new sINPmodel(lstring::copy(token), type, lstring::copy(text)));
    delete [] lcname;

    return (OK);
}


void
SPinput::killMods()
{
    delete ip_modtab;
    ip_modtab = 0;
    delete ip_badmodtab;
    ip_badmodtab = 0;
}


// Scan the line for a token matching pname, and return the following
// token.
//
char *
SPinput::lookParam(const char *pname, const char *line)
{
    if (line && pname) {
        while (*line) {
            char *tok = getTok(&line, true);
            if (!tok)
                break;
            if (lstring::cieq(pname, tok)) {
                delete [] tok;
                tok = getTok(&line, true);
                return (tok);
            }
            delete [] tok;
        }
    }
    return (0);
}


// The largest model level possible, levels are stored as unsigned
// chars in the IFdevice struct.
//
#define LEV_MAX 255

// A little class for mapping an arbitrary number to a number in the
// range 1-255.  This is for mapping a "foreign" MOS level to a known
// WRspice MOS level, which makes it possible to, e.g., use design kit
// models intended for another simulator, with different MOS levels,
// without having to modify the files.
//
class cMosLevMap
{
public:
    struct levmap_t
    {
        levmap_t(int w, int x, levmap_t *n)
            {
                wrs_lev = w;
                ext_lev = x;
                next = n;
            }

        static void destroy(levmap_t *l)
            {
                while (l) {
                    levmap_t *lx = l;
                    l = l->next;
                    delete lx;
                }
            }

        int wrs_lev;        // WRspice mos level.
        int ext_lev;        // Level to map to wrs_lev.
        levmap_t *next;
    };

    cMosLevMap()
        {
            mlm_list = 0;
        }

    ~cMosLevMap()
        {
            levmap_t::destroy(mlm_list);
        }

    void add_map(int, int);
    void delete_map(int);
    void clear();
    int get_map(int);

private:
    levmap_t *mlm_list;
};

namespace { cMosLevMap mosLevelMap; }


// Add or remmap ext_lev -> wrs_lev.
//
void
cMosLevMap::add_map(int wrs_lev, int ext_lev)
{
    if (wrs_lev < 1 || wrs_lev > LEV_MAX)
        return;
    for (levmap_t *m = mlm_list; m; m = m->next) {
        if (m->ext_lev == ext_lev) {
            m->wrs_lev = wrs_lev;
            return;
        }
    }
    mlm_list = new levmap_t(wrs_lev, ext_lev, mlm_list);
}


// Delete mapping ext->lev, if any.
//
void
cMosLevMap::delete_map(int ext_lev)
{
    levmap_t *lp = 0;
    for (levmap_t *m = mlm_list; m; m = m->next) {
        if (m->ext_lev == ext_lev) {
            if (lp)
                lp->next = m->next;
            else
                mlm_list = m->next;
            delete m;
            return;
        }
        lp = m;
    }
}


// Clear all maps;
void
cMosLevMap::clear()
{
    levmap_t::destroy(mlm_list);
    mlm_list = 0;
}


// Return the mapped WRspice level for ext_level, or 0 if mapping is
// impossible.
int
cMosLevMap::get_map(int ext_lev)
{
    for (levmap_t *m = mlm_list; m; m = m->next) {
        if (m->ext_lev == ext_lev)
            return (m->wrs_lev);
    }
    if (ext_lev >= 1 && ext_lev <= LEV_MAX)
        return (ext_lev);
    return (0);
}
// End of cModLevMap functions.


void
SPinput::addMosLevelMapping(int wrs_lev, int ext_lev)
{
    mosLevelMap.add_map(wrs_lev, ext_lev);
}


void
SPinput::delMosLevelMapping(int ext_lev)
{
    mosLevelMap.delete_map(ext_lev);
}


void
SPinput::clearMosLevelMaps()
{
    mosLevelMap.clear();
}


int
SPinput::getMosLevelMapping(int ext_lev)
{
    return (mosLevelMap.get_map(ext_lev));
}


// Find the 'level' parameter on the given line and return its
// value.
//
bool
SPinput::findLev(const char *line, int *level, bool is_mos)
{
    *level = 1;
    char *tok = lookParam("LEVEL", line);
    if (tok) {
        const char *t = tok;
        int err;
        double lev = getFloat(&t, &err, true);
        delete [] tok;
        if (err)
            return (false);
        int ext_lev = lev > 0.0 ? (int)(lev + 0.5) : (int)(lev - 0.5);
        int wrs_lev;
        if (is_mos)
            wrs_lev = mosLevelMap.get_map(ext_lev);
        else
            wrs_lev = ext_lev;
        if (wrs_lev < 1 || wrs_lev > LEV_MAX)
            return (false);
        *level = wrs_lev;
    }
    return (true);
}


// Find a model with matching base name with l,w matching the selection
// parameters in the model
//
sINPmodel *
SPinput::mosFind(sLine *curline, const char *name, double l, double w)
{
    int lastcnds = -1;
    sINPmodel *lastent = 0;

    sHgen gen(ip_modtab);
    sHent *h;
    while ((h = gen.next()) != 0) {
        sINPmodel *m = (sINPmodel*)h->data();
        if (mosmatch(name, m->modName)) {
            int cnds = mosLWcnds(curline, m, l, w);
            if (cnds > lastcnds) {
                lastent = m;
                lastcnds = cnds;
            }
        }
    }
    gen = sHgen(ip_modcache);
    while ((h = gen.next()) != 0) {
        sINPmodel *m = (sINPmodel*)h->data();
        if (mosmatch(name, m->modName)) {
            int cnds = mosLWcnds(curline, m, l, w);
            if (cnds > lastcnds) {
                lastent = m;
                lastcnds = cnds;
            }
        }
    }
    return (lastent);
}


// Count and return the conditions matched.
//
int
SPinput::mosLWcnds(sLine *curline, sINPmodel *m, double l, double w)
{
    int cnds = 0;
    if (l > 0) {
        char *tok1 = lookParam("LMIN", m->modLine);
        if (tok1) {
            const char *t = tok1;
            int err;
            double lmin = getFloat(&t, &err, true);
            delete [] tok1;
            if (err) {
                logError(curline, "Non-numeric value for \"LMIN\" in model %s",
                    m->modName);
                return (-1);
            }
            if (l < lmin)
                return (-1);
            cnds++;
        }
        char *tok2 = lookParam("LMAX", m->modLine);
        if (tok2) {
            const char *t = tok2;
            int err;
            double lmax = getFloat(&t, &err, true);
            delete [] tok2;
            if (err) {
                logError(curline, "Non-numeric value for \"LMAX\" in model %s",
                    m->modName);
                return (-1);
            }
            if (l > lmax)
                return (-1);
            cnds++;
        }
    }
    if (w > 0) {
        char *tok1 = lookParam("WMIN", m->modLine);
        if (tok1) {
            const char *t = tok1;
            int err;
            double wmin = getFloat(&t, &err, true);
            delete [] tok1;
            if (err) {
                logError(curline, "Non-numeric value for \"WMIN\" in model %s",
                    m->modName);
                return (-1);
            }
            if (w < wmin)
                return (-1);
            cnds++;
        }
        char *tok2 = lookParam("WMAX", m->modLine);
        if (tok2) {
            const char *t = tok2;
            int err;
            double wmax = getFloat(&t, &err, true);
            delete [] tok2;
            if (err) {
                logError(curline, "Non-numeric value for \"WMAX\" in model %s",
                    m->modName);
                return (-1);
            }
            if (w > wmax)
                return (-1);
            cnds++;
        }
    }
    return (cnds);
}


namespace {
    // These are BJT model parameters used in HSPICE extensions, that
    // have no equivalence here.
    //
    const char *hspice_bjt_unused[] = {
        "iss",
        "ns",
        "tlev",
        "tlevc",
        "update",
        0
    };

    // These are MOS model parameters used in HSPICE extensions, that
    // have no equivalence here.
    //
    const char *hspice_mos_unused[] = {
        "binflag",
        "scalm",
        "scale",
        "lref",
        "wref",
        "xw",
        "xl",
        "acm",
        "calcacm",
        "cjgate",
        "hdif",
        "ldif",
        "rdc",
        "rd",
        "rsc",
        "rs",
        "wmlt",
        "lmlt",
        "capop",
        "cta",
        "ctp",
        "pta",
        "ptp",
        "tlev",
        "tlevc",
        "alpha",
        "lalpha",
        "walpha",
        "vcr",
        "lvcr",
        "wvcr",
        "iirat",
        "dtemp",
        "nds",
        "vnds",
        "sfvtflag",
        0
    };
}


// Create an instance of the model m in the circuit.
//
bool
SPinput::instantiateMod(sLine *curline, sINPmodel *m, sCKT *ckt)
{
    if (m->modType < 0 || m->modType >= DEV.numdevs() ||
            !DEV.device(m->modType)) {
        logError(curline, "Unknown device type for model %s", m->modName);
        return (false);
    }

    int type = m->modType;
    char *modname = lstring::copy(m->modName);
    ckt->insert(&modname);
    int error = ckt->findModl(&type, 0, modname);
    if (error != 0) {
        // not already defined, so create & give parameters
        sGENmodel *modfast;
        error = ckt->newModl(m->modType, &modfast, modname);
        if (error) {
            logError(curline, error, m->modName);
            return (false);
        }
        IFdevice *dev = DEV.device(m->modType);

        // Since the .model line may or may not be in the current deck,
        // instantiation error messages appear in the current line,
        // following a separator line.

        const char *msgfmt = "Messages from model %s instantiation follow";
        bool didsep = false;

        bool hs_silent = Sp.HspiceFriendly();

        // parameter isolation, identification, binding
        const char *line = m->modLine;
        char *parm = getTok(&line, true);   // throw away '.model'
        delete [] parm;
        parm = getTok(&line, true);         // throw away 'modname'
        delete [] parm;
        parm = getTok(&line, true);
        bool lpnf = false;

        // The first "parameter" is the built-in model name.  This is
        // always an IF_FLAG, and is a no-op/consistency check except
        // for npn/pnp, nmos/pmos, etc.
        //
        // Here we enforce a flag value, which is a hack for adms
        // compiled Verilog which does not have a flag datatype and
        // sets the model name parameter to IF_INTEGER.

        bool first = true;
        while (parm) {
            IFparm *p = dev->findModelParm(parm,
                    first ? IF_ASK | IF_SET : IF_SET);
            if (p) {
                IFdata data;
                data.type = first ? IF_FLAG : p->dataType;
                if (!getValue(&line, &data, ckt)) {
                    if (!didsep) {
                        didsep = true;
                        logError(curline, msgfmt, m->modName);
                    }
                    logError(curline, E_PARMVAL, p->keyword);
                    delete [] parm;
                    return (false);
                }

                error  = modfast->setParam(p->id, &data);
                if (error) {
                    if (!didsep) {
                        didsep = true;
                        logError(curline, msgfmt, m->modName);
                    }
                    logError(curline, error, p->keyword);
                    delete [] parm;
                    return (false);
                }
                lpnf = false;
            } 
            else
                lpnf = true;
            first = false;
            if (p) {
                delete [] parm;
                parm = getTok(&line, true);
                continue;
            }

            const char *ptmp = parm;
            if (lpnf && SPnum.parse(&ptmp, false)) {
                // If the last param wasn't found, don't whine about
                // the following number.

                lpnf = false;
                delete [] parm;
                parm = getTok(&line, true);
                continue;
            }

            if (lstring::cieq(parm, "level")) {
                // Just grab the number and throw away
                // since we already have that info from pass1.

                IFdata data;
                data.type = IF_REAL;
                if (!getValue(&line, &data, ckt)) {
                    if (!didsep) {
                        didsep = true;
                        logError(curline, msgfmt, m->modName);
                    }
                    logError(curline, E_PARMVAL, parm);
                    delete [] parm;
                    return (false);
                }
                lpnf = false;
                delete [] parm;
                parm = getTok(&line, true);
                continue;
            }

            bool found = false;
            if (dev->isMOS()) {
                if (lstring::cieq(parm, "lmin") ||
                        lstring::cieq(parm, "lmax") ||
                        lstring::cieq(parm, "wmin") ||
                        lstring::cieq(parm, "wmax")) {
                    // Just grab the number and throw away
                    // since we already have that info from pass1.
                    IFdata data;
                    data.type = IF_REAL;
                    if (!getValue(&line, &data, ckt)) {
                        if (!didsep) {
                            didsep = true;
                            logError(curline, msgfmt, m->modName);
                        }
                        logError(curline, E_PARMVAL, parm);
                        delete [] parm;
                        return (false);
                    }
                    lpnf = false;
                    delete [] parm;
                    parm = getTok(&line, true);
                    continue;
                }

                // Check if this is an HSPICE parameter that we don't
                // handle.
                for (const char **s = hspice_mos_unused; *s; s++) {
                    if (lstring::cieq(parm, *s)) {
                        if (!hs_silent) {
                            if (!didsep) {
                                didsep = true;
                                logError(curline, msgfmt, m->modName);
                            }
                            logError(curline,
                            "HSPICE MOS parameter %s not handled, ignored",
                                parm);
                        }
                        found = true;
                        break;
                    }
                }
            }
            else if (dev->isBJT()) {

                // Check if this is an HSPICE parameter that we don't
                // handle.
                for (const char **s = hspice_bjt_unused; *s; s++) {
                    if (lstring::cieq(parm, *s)) {
                        if (!hs_silent) {
                            if (!didsep) {
                                didsep = true;
                                logError(curline, msgfmt, m->modName);
                            }
                            logError(curline,
                            "HSPICE BJT parameter %s not handled, ignored",
                                parm);
                        }
                        found = true;
                        break;
                    }
                }
            }

            if (!found) {
                // For Verilog, the first token can be the module name.
                // Just ignore if there is no parameter.

                if (!dev->moduleNameMatch(parm)) {
                    if (!didsep) {
                        didsep = true;
                        logError(curline, msgfmt, m->modName);
                    }
                    logError(curline,
                        "Unrecognized parameter %s (ignored)", parm);
                }
            }
            lpnf = true;
            delete [] parm;
            parm = getTok(&line, true);
        }
    }
    return (true);
}


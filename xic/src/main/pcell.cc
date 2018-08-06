
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
#include "oa_if.h"
#include "pcell.h"
#include "pcell_params.h"
#include "editif.h"
#include "drcif.h"
#include "dsp_inlines.h"
#include "cd_propnum.h"
#include "cd_celldb.h"
#include "cd_hypertext.h"
#include "fio.h"
#include "fio_library.h"
#include "si_parsenode.h"
#include "si_parser.h"
#include "si_interp.h"
#include "pcell_params.h"
#include "python_if.h"
#include "tcltk_if.h"
#include "cvrt_variables.h"
#include "main_scriptif.h"
#include "miscutil/pathlist.h"
#include "miscutil/filestat.h"
#include "miscutil/encode.h"


//-----------------------------------------------------------------------------
// Basic PCell support

// Properties for PCells (parameterized cells).
// Property        Contents    Applies to: super sub inst objects
// XICP_PC         name of super                 x   x
// XICP_PC_SCRIPT  script text             x
// XICP_PC_PARAMS  parameters              x     x   x
// XICP_GRIP       stretch handle desc                    x
//
// The super-master PCell contains a script, default parameter
// values optionally with parameter constraint strings.  When the
// super-master is instantiated, the script is executed producing
// a sub-master under a modified name, plus an instance of the
// sub-master.  The instance contains the name of the super-master
// and a copy of the parameters.

//#define PC_DEBUG

// Instantiate pcell support.
namespace { cPCellDb _pc_; }

cPCellDb *cPCellDb::instancePtr = 0;


cPCellDb::cPCellDb()
{
    if (instancePtr) {
        fprintf(stderr, "Singleton class cPCellDb already instantiated.\n");
        exit(1);
    }
    instancePtr = this;

    pc_param_tab = 0;
    pc_script_param_tab = 0;
    pc_master_tab = 0;
}


// Private static error exit.
//
void
cPCellDb::on_null_ptr()
{
    fprintf(stderr, "Singleton class cPCellDb used before instantiated.\n");
    exit(1);
}


// Set the parameter list used when instantiating the named PCell. 
// The last parameter set passed for each pcell is saved in a hash
// table.  If inscr is true, we are executing a script, in which case
// we will want to be able to revert to the pre-script state.
//
// If dbname and prms arguments are null, destroy the table
// instead.  If the prms arg is null, remove dbname from the
// table.  These operations are NOT reverted if done in a script.
//
bool
cPCellDb::setPCinstParams(const char *dbname, const PCellParam *prms,
    bool inscr)
{
    // Used for all PCells.

    if (!dbname && !prms) {
        // Clear and destroy table.
        if (pc_param_tab) {
            SymTabGen gen(pc_param_tab, true);
            SymTabEnt *ent;
            while ((ent = gen.next()) != 0) {
                PCellParam::destroy((PCellParam*)ent->stData);
                delete [] ent->stTag;
                delete ent;
            }
            delete pc_param_tab;
            pc_param_tab = 0;
        }
        return (true);
    }
    if (!prms) {
        // Remove dbname from table.
        if (pc_param_tab) {
            PCellParam *p = (PCellParam*)SymTab::get(pc_param_tab, dbname);
            if (p != (PCellParam*)ST_NIL) {
                pc_param_tab->remove(dbname);
                PCellParam::destroy(p);
            }
        }
        return (true);
    }

    if (inscr) {
        // Save the existing entry if we haven't yet, or a null if
        // none exists.

        if (!pc_script_param_tab)
            pc_script_param_tab = new SymTab(true, false);
        if (SymTab::get(pc_script_param_tab, dbname) == ST_NIL) {
            PCellParam *px = 0;
            if (pc_param_tab) {
                PCellParam *p = (PCellParam*)SymTab::get(pc_param_tab, dbname);
                if (p != (PCellParam*)ST_NIL)
                    px = PCellParam::dup(p);
            }
            pc_script_param_tab->add(lstring::copy(dbname), px, false);
        }
    }

    if (!pc_param_tab)
        pc_param_tab = new SymTab(true, false);
    SymTabEnt *ent = SymTab::get_ent(pc_param_tab, dbname);
    if (ent) {
        PCellParam::destroy((PCellParam*)ent->stData);
        ent->stData = PCellParam::dup(prms);
    }
    else
        pc_param_tab->add(lstring::copy(dbname), PCellParam::dup(prms), false);
    return (true);
}


// This is called after executing a script, and reverts the parameter
// strings that were modified in the script to the pre-execution
// values.  It does not, however, put back entries that were removed.
//
// The script parameter table is freed here.
//
void
cPCellDb::revertPCinstParams()
{
    // Used for all PCells.

    if (pc_script_param_tab && pc_param_tab) {
        SymTabGen gen(pc_script_param_tab, true);
        SymTabEnt *ent;
        while ((ent = gen.next()) != 0) {
            PCellParam *p = (PCellParam*)SymTab::get(pc_param_tab, ent->stTag);
            if (p != (PCellParam*)ST_NIL) {
                pc_param_tab->remove(ent->stTag);
                PCellParam::destroy(p);
                if (ent->stData) {
                    pc_param_tab->add(lstring::copy(ent->stTag), ent->stData,
                        false);
                    ent->stData = 0;
                }
            }
            PCellParam::destroy((PCellParam*)ent->stData);
            delete [] ent->stTag;
            delete ent;
        }
    }
    delete pc_script_param_tab;
    pc_script_param_tab = 0;
}


// Return the current parameter set for dbname.
//
const PCellParam *
cPCellDb::curPCinstParams(const char *dbname)
{
    // Used for all PCells.

    if (!pc_param_tab)
        return (0);
    PCellParam *pm = (PCellParam*)SymTab::get(pc_param_tab, dbname);
    if (pm == (PCellParam*)ST_NIL)
        return (0);
    return (pm);
}


// Add a sub-master specification to the global pcell table.  The
// returned string is the unique Xic database name for the sub-master
// cell.  If xicname is given, use it for the Xic cell name.  This is
// done when pre-loading Xic cells that represent pcell sub-masters,
// which can be done before reading a Virtuoso design that references
// Skill pcells, which won't be resolved as-is.  The sub-master Xic
// cells must have been obtained previously by some means.
//
char *
cPCellDb::addSubMaster(const char *libname, const char *cellname,
    const char *viewname, const PCellParam *pm, const char *xicname)
{
    // Used for all PCells.

    char *dbname = PCellDesc::mk_dbname(libname, cellname, viewname);
    if (!pc_master_tab)
        pc_master_tab = new SymTab(false, false);
    SymTabEnt *ent = SymTab::get_ent(pc_master_tab, dbname);
    if (!ent) {
        PCellDesc *pd = new PCellDesc(dbname, 0);
        PCellItem *pi = new PCellItem(pm);
        pd->addItem(pi);
        pc_master_tab->add(pd->dbname(), pd, false);
        delete [] dbname;
        if (xicname && *xicname)
            pi->setCellname(xicname);
#ifdef PC_DEBUG
        char *nm = pd->cellname(cellname, pi);
        printf("addSubMaster: initial %s %s %s %s\n", nm, libname, cellname,
            viewname);
        pm->print();
        return (nm);
#else
        return (pd->cellname(cellname, pi));
#endif
    }
    delete [] dbname;
    PCellDesc *pd = (PCellDesc*)ent->stData;
    PCellItem *pi = pd->findItem(pm);
    if (!pi) {
        pi = new PCellItem(pm);
        pd->addItem(pi);
        if (xicname && *xicname)
            pi->setCellname(xicname);
    }
#ifdef PC_DEBUG
    char *nm = pd->cellname(cellname, pi);
    printf("addSubMaster: adding %s %s %s %s\n", nm, libname, cellname,
        viewname);
    pm->print();
    return (nm);
#else
    return (pd->cellname(cellname, pi));
#endif
}


// Add a super-master to the table, if not already there.
//
char *
cPCellDb::addSuperMaster(const char *libname, const char *cellname,
    const char *viewname, const PCellParam *defprms)
{
    // Used for all PCells.

    char *dbname = PCellDesc::mk_dbname(libname, cellname, viewname);
    if (!pc_master_tab)
        pc_master_tab = new SymTab(false, false);
    SymTabEnt *ent = SymTab::get_ent(pc_master_tab, dbname);
    if (!ent) {
        PCellDesc *pd = new PCellDesc(dbname, PCellParam::dup(defprms));
        pc_master_tab->add(pd->dbname(), pd, false);
    }
    return (dbname);
}


// Find a PCellDesc in the table, by the database name.
//
PCellDesc *
cPCellDb::findSuperMaster(const char *dbname)
{
    // Used for all PCells.

    if (!pc_master_tab)
        return (0);
    PCellDesc *pd = (PCellDesc*)SymTab::get(pc_master_tab, dbname);
    if (pd == (PCellDesc*)ST_NIL)
        pd = 0;
    return (pd);
}


// Return a complete parameter list as specified by a user-generated
// input string, for the named cell.
//
bool
cPCellDb::getParams(const char *dbname, const char *input, PCellParam **pret)
{
    // Used for all PCells.

    *pret = 0;

    if (pc_master_tab) {
        PCellDesc *pd = (PCellDesc*)SymTab::get(pc_master_tab, dbname);
        if (pd != (PCellDesc*)ST_NIL && pd->defaultParams())
            return (parseParams(input, pret, pd->defaultParams()));
    }

    // Not in master tab, so we have to open the pcell to get the
    // default parameter set.

    char *libname, *cellname, *viewname;
    if (!PCellDesc::split_dbname(dbname, &libname, &cellname, &viewname)) {
        Errs()->add_error("getParams: bad database name.");
        return (false);
    }
    PCellDesc::LCVcleanup lcv(libname, cellname, viewname);

    PCellParam *pdef = 0;
    if (!strcmp(libname, XIC_NATIVE_LIBNAME)) {
        // Xic native PCell.

        // Open super-master.
        CDs *sdsup;
        OItype oiret = openMaster(cellname, Physical, &sdsup);
        if (oiret != OIok) {
            Errs()->add_error("getParams: failed to open master %s.",
                cellname);
            return (false);
        }
        if (!isSuperMaster(sdsup)) {
            Errs()->add_error("getParams: %s is not a valid super-master.",
                cellname);
            return (false);
        }
        CDp *pp = sdsup->prpty(XICP_PC_PARAMS);  // always exists
        if (!PCellParam::parseParams(pp->string(), &pdef))
            return (false);
    }
    else {
        // PCell from OA.
        // Since the dbname was not in the master tab, it needs to be
        // opened.

        if (!OAif()->load_cell(libname, cellname, viewname, CDMAXCALLDEPTH,
                false, &pdef, 0)) {
            Errs()->add_error(
                "getParams: failed to open OA super-master %s/%s/%s.",
                libname, cellname, viewname);
            return (false);
        }
    }
    if (pdef) {
        char *ndbname = addSuperMaster(libname, cellname, viewname, pdef);
        delete [] ndbname;
        bool ret = parseParams(input, pret, pdef);
        PCellParam::destroy(pdef);
        return (ret);
    }
    Errs()->add_error(
        "getParams: failed to find any default parameters for %s/%s/%s.",
        libname, cellname, viewname);

    return (false);
}


// Return a copy of the default parameter set, opening the super
// master if necessary.
//
bool
cPCellDb::getDefaultParams(const char *dbname, PCellParam **pret)
{
    // Used for all PCells.

    *pret = 0;

    if (pc_master_tab) {
        PCellDesc *pd = (PCellDesc*)SymTab::get(pc_master_tab, dbname);
        if (pd != (PCellDesc*)ST_NIL && pd->defaultParams()) {
            *pret = PCellParam::dup(pd->defaultParams());
            return (true);
        }
    }

    // Not in master tab, so we have to open the pcell to get the
    // default parameter set.

    char *libname, *cellname, *viewname;
    if (!PCellDesc::split_dbname(dbname, &libname, &cellname, &viewname)) {
        Errs()->add_error("getDefaultParams: bad database name.");
        return (false);
    }
    PCellDesc::LCVcleanup lcv(libname, cellname, viewname);

    PCellParam *pdef = 0;
    if (!strcmp(libname, XIC_NATIVE_LIBNAME)) {
        // Xic native PCell.

        // Open super-master.
        CDs *sdsup;
        OItype oiret = openMaster(cellname, Physical, &sdsup);
        if (oiret != OIok) {
            Errs()->add_error("getDefaultParams: failed to open master %s.",
                cellname);
            return (false);
        }
        if (!isSuperMaster(sdsup)) {
            Errs()->add_error(
                "getDefaultParams: %s is not a valid super-master.",
                cellname);
            return (false);
        }
        CDp *pp = sdsup->prpty(XICP_PC_PARAMS);  // always exists
        if (!pp) {
            Errs()->add_error(
                "getDefaultParams: missing pc_params property in %s.",
                cellname);
            return (false);
        }
        if (!PCellParam::parseParams(pp->string(), &pdef))
            return (false);
    }
    else {
        // PCell from OA.
        // Since the dbname was not in the master tab, it needs to be
        // opened.

        if (!OAif()->load_cell(libname, cellname, viewname, CDMAXCALLDEPTH,
                false, &pdef, 0)) {
            Errs()->add_error(
                "getDefaultParams: failed to open OA super-master %s/%s/%s.",
                libname, cellname, viewname);
            return (false);
        }
    }
    if (pdef) {
        char *ndbname = addSuperMaster(libname, cellname, viewname, pdef);
        delete [] ndbname;
        *pret = pdef;
        return (true);
    }
    Errs()->add_error(
        "getDefaultParams: failed to find any default parameters for %s/%s/%s.",
        libname, cellname, viewname);

    return (false);
}


// Take the user-supplied input string and return in pret a PCellParam
// list.  The user input is assumed to consist of pairs of name/value
// tokens, separated by white space, commas, or equal signs.  It
// supports the [typechar:] prefixing on parameter names.  I.e.,
//
//   [typechar:]name [=] value[:constraint] [,]
//
// If pdef is not null, use this as a template.  The values are placed
// in the order of the template.  Missing input values are inserted
// into the output with the template values.
//
// If a name is given that is not found in a non-null template, or
// there is a syntax error, false is returned and the output parameter
// list is null.  A message will have been added to the errors system.
//
bool
cPCellDb::parseParams(const char *input, PCellParam **pret,
    const PCellParam *pdef)
{
    // Used for all PCells.

    PCellParam *p0;
    if (!PCellParam::parseParams(input, &p0))
        return (false);

    if (pdef) {
        PCellParam *pref = PCellParam::dup(pdef);
        while (p0) {
            bool found = false;
            for (PCellParam *p = pref; p; p = p->next()) {
                if (!strcmp(p0->name(), p->name())) {
                    found = true;
                    if (!p->set(p0)) {
                        Errs()->add_error(
                            "parseParams: incompatible data types for %s, "
                            "expecting %s got %s.", p0->name(),
                            p->typestr(), p0->typestr());
                        PCellParam::destroy(p0);
                        PCellParam::destroy(pref);
                        return (false);
                    }
                    break;
                }
            }
            if (found) {
                PCellParam *px = p0;
                p0 = p0->next();
                delete px;
            }
            else {
                // bad arg name
                Errs()->add_error(
                    "parseParams: unknown parameter %s.", p0->name());
                PCellParam::destroy(p0);
                PCellParam::destroy(pref);
                return (false);
            }
        }
        p0 = pref;
    }
    *pret = p0;
    return (true);
}


// Take the user-supplied input string and return in output a
// formatted parameter list string for use in the XICP_PC_PARAM
// property.  The user input is assumed to consist of pairs of
// name/value tokens, separated by white space, commas, or equal
// signs.  It supports the [typechar:] prefixing on parameter names. 
// I.e.,
//
//   [typechar:]name [=] value [,]
//
// In Xic native PCells, the parameter type is always "string", so
// that the typechar prefix should/will never occur. 
//
// If refprms, a default parameter list, is not null, use this as a
// template.  The values are placed in the order of the template. 
// Missing input values are inserted into the output with the template
// values.
//
// If a name is given that is not found in a non-null template, or
// there is a syntax error, false is returned and the output parameter
// list is null.  A message will have been added to the errors system.
//
bool
cPCellDb::formatParams(const char *input, char **output,
    const char *refprms)
{
    // Used for Xic native PCells.

    *output = 0;
    PCellParam *pdef;
    if (!PCellParam::parseParams(refprms, &pdef)) {
        Errs()->add_error(
            "formatParams: parse error in default parameters:\n%s",
            Errs()->get_error());
        return (false);
    }

    PCellParam *prms;
    if (!parseParams(input, &prms, pdef)) {
        PCellParam::destroy(pdef);
        return (false);
    }
    *output = prms->string(true);  // no constraints
    PCellParam::destroy(prms);
    return (true);
}


// Open a cell, which is already in memory, or is a native or library
// cell.
//
OItype
cPCellDb::openMaster(const char *cname, DisplayMode mode, CDs **sdret)
{
    // Used for Xic native PCells.

    CDcbin cbin;
    if (!CDcdb()->findSymbol(lstring::strip_path(cname), &cbin)) {
        OItype oitmp = FIO()->FromNative(cname, &cbin, 1.0);
        if (oitmp == OIaborted)
            return (oitmp);
        if (oitmp != OIok) {
            oitmp = FIO()->OpenLibCell(0, cname, LIBuser, &cbin);
            if (OIfailed(oitmp))
                return (oitmp);
        }
    }

    CDs *sd = cbin.celldesc(mode);
    if (!sd)
        return (OIerror);
    *sdret = sd;
    return (OIok);
}


// Open a sub-master for the super-master and params.
//
OItype
cPCellDb::openSubMaster(const CDs *sdsup, const char *params, CDs **sdret)
{
    // Used for Xic native PCells.

    *sdret = 0;
    if (!sdsup || !sdsup->isPCellSuperMaster()) {
        Errs()->add_error("openSubMaster: null or invalid super-master.");
        return (OIerror);
    }

    if (sdsup->pcType() != CDpcXic)
        return (OIerror);

    PCellParam *pcprms;
    if (!PCellParam::parseParams(params, &pcprms))
        return (OIerror);
    const char *nm = addSubMaster(XIC_NATIVE_LIBNAME,
        Tstring(sdsup->cellname()),
        sdsup->isElectrical() ? "schematic" : "layout", pcprms);

    CDcbin cbin;
    CDcdb()->findSymbol(nm, &cbin);
    PCellParam::destroy(pcprms);

#ifdef PC_DEBUG
    printf("openSubMaster: name=%s %s\n", nm, cbin.cellname() ? "exists" : "");
#endif

    CDs *sdsub = 0;
    if (!cbin.cellname()) {
        // Sub-master does not yet exist, create it.
        if (sdsup->isElectrical()) {
            cbin.setElec(CDcdb()->insertCell(nm, Electrical));
            sdsub = cbin.elec();
        }
        else {
            cbin.setPhys(CDcdb()->insertCell(nm, Physical));
            sdsub = cbin.phys();
        }
        if (!cbin.cellname()) {
            return (OIerror);
        }
        if (sdsub) {
            sdsup->cloneCell(sdsub);
            sdsub->prptyRemove(XICP_PC_PARAMS);
            sdsub->prptyAdd(XICP_PC_PARAMS, params);
            if (!evalScript(sdsub, Tstring(sdsup->cellname()))) {
                Errs()->add_error("Error: pcell evaluation failed.");
                delete sdsub;
                return (OIerror);
            }
        }
    }
    else
        sdsub = sdsup->isElectrical() ? cbin.elec() : cbin.phys();
    *sdret = sdsub;
    return (OIok);
}


// Open a pcell sub-master that corresponds to the name and property
// strings given.  These are PC and PC_PARAMS property strings as
// found in a pcell instance.  If sdesc is not null, it will be used
// as the sub-master, otherwise a new sub-master will be created if it
// doesn't already exist.
//
bool
cPCellDb::openSubMaster(const char *nmstr, const char *prpstr, CDs **psd,
    CDs *sdesc)
{
    if (psd)
        *psd = 0;

    // We can determine from the name whether the super-master is from
    // OA or not.
    char *libname, *cellname, *viewname;
    if (!PCellDesc::split_dbname(nmstr, &libname, &cellname, &viewname)) {
        // Must be Xic native.

        char *dbname = PCellDesc::mk_native_dbname(lstring::strip_path(nmstr));
        if (!PCellDesc::split_dbname(dbname, &libname, &cellname, &viewname)) {
            // can't happen
            delete [] dbname;
            return (false);
        }
        PCellDesc::LCVcleanup lcv(libname, cellname, viewname);

        // This will add a super-master entry to the master tab, open
        // the super-master in Xic and obtain the default params, and
        // create the sub-master param list.

        PCellParam *prms;
        if (!getParams(dbname, prpstr, &prms)) {
            delete [] dbname;
            return (false);
        }

        // Add an instance entry to the master_tab for this parameter
        // set.  A "suggested" Xic sub-master cellname is returned. 
        // If this differs from the sdesc cellname, reset the
        // sub-master cellname in the table.

        char *newcname = addSubMaster(libname, cellname, viewname, prms);
        if (sdesc && strcmp(newcname, Tstring(sdesc->cellname()))) {
            PCellDesc *pd = findSuperMaster(dbname);
            if (!pd) {
                Errs()->add_error(
                    "openSubMaster: super-master not in table.");
                delete [] newcname;
                delete [] dbname;
                return (false);
            }
            PCellItem *pi = pd->findItem(prms);
            if (!pi) {
                Errs()->add_error(
                    "openSubMaster: sub-master not in item list.");
                delete [] newcname;
                delete [] dbname;
                return (false);
            }
            pi->setCellname(Tstring(sdesc->cellname()));
        }
        if (sdesc)
            delete [] newcname;
        delete [] dbname;

        // Open the super-master.
        CDs *sdsup;
        OItype oiret = openMaster(nmstr, Physical, &sdsup);
        if (oiret != OIok) {
            Errs()->add_error("openSubMaster: failed to open super-master %s.",
                nmstr);
            return (false);
        }
        if (!isSuperMaster(sdsup)) {
            Errs()->add_error("openSubMaster: invalid super-master %s.",
                nmstr);
            return (false);
        }

        if (!sdesc) {
            sdesc = new CDs(CD()->CellNameTableAdd(newcname), Physical);
            CDcdb()->linkCell(sdesc);
            delete [] newcname;
        }
        sdsup->cloneCell(sdesc);
        sdesc->prptyAdd(XICP_PC_PARAMS, prpstr);
        if (!evalScript(sdesc, Tstring(sdsup->cellname()))) {
            Errs()->add_error("openSubMaster: pcell evaluation failed.");
            return (false);
        }
        if (psd)
            *psd = sdesc;
    }
    else {
        // Get a sub-master from OA.

        PCellDesc::LCVcleanup lcv(libname, cellname, viewname);

        // This will add a super-master entry to the master tab, open
        // the super-master in OA and obtain the default params, and
        // create the sub-master param list.

        PCellParam *prms;
        if (!getParams(nmstr, prpstr, &prms))
            return (false);

        // Add an instance entry to the master_tab for this parameter
        // set.  A "suggested" Xic sub-master cellname is returned. 
        // If this differs from the sdesc cellname, reset the
        // sub-master cellname in the table.

        char *newcname = addSubMaster(libname, cellname, viewname, prms);
        if (sdesc && strcmp(newcname, Tstring(sdesc->cellname()))) {
            PCellDesc *pd = findSuperMaster(nmstr);
            if (!pd) {
                Errs()->add_error("openSubMaster: missing pc_name property.");
                return (false);
            }
            PCellItem *pi = pd->findItem(prms);
            if (!pi) {
                Errs()->add_error(
                    "openSubMaster: sub-master not in item list.");
                return (false);
            }
            pi->setCellname(Tstring(sdesc->cellname()));
        }
        delete [] newcname;

        // Now call back to OA to fill in the cell.

        char *subm_name;
        if (!OAif()->load_cell(libname, cellname, viewname, CDMAXCALLDEPTH,
                false, &prms, &subm_name)) {
            Errs()->add_error("openSubMaster: OA cell load failed.");
            return (false);
        }
        if (psd)
            *psd = CDcdb()->findCell(subm_name, Physical);
        delete [] subm_name;
        PCellParam::destroy(prms);
    }
    return (true);
}


// This does post-processing after a hierarchy is read.  It is called
// for each cell created during the read.  First, it will ensure that
// pcell instances have the pcell properties.  Older Xic releases did
// not apply the instance properties.  Second, if pcell sub-masters
// are missing, they will be recreated, assuming that the pcells can
// be found.
//
bool
cPCellDb::reopenSubMaster(CDs *sdesc)
{
    // Used for all PCells.

    if (!sdesc)
        return (true);
    if (sdesc->isPCellSubMaster()) {
        // Make sure all instances have pcell properties.
        CDp *pn = sdesc->prpty(XICP_PC);
        CDp *pp = sdesc->prpty(XICP_PC_PARAMS);
        CDm_gen mgen(sdesc, GEN_MASTER_REFS);
        CDm *md;
        while ((md = mgen.next()) != 0) {
            CDc_gen cgen(md);
            for (CDc *cdesc = cgen.c_first(); cdesc;
                    cdesc = cgen.c_next()) {
                if (!cdesc->prpty(XICP_PC))
                    cdesc->prptyAddCopy(pn);
                if (!cdesc->prpty(XICP_PC_PARAMS))
                    cdesc->prptyAddCopy(pp);
            }
        }
    }

    if (!sdesc->isUnread())
        return (true);

    // The sdesc was not found in the archive.  Look at the first
    // instance for pcell properties (all instances will have the same
    // properties).  If found, open the pcell and evaluate it into
    // sdesc.

    CDm_gen mgen(sdesc, GEN_MASTER_REFS);
    CDm *md = mgen.next();
    if (!md)
        return (true);  // no instances
    CDc_gen cgen(md);
    CDc *cdesc = cgen.next();
    if (!cdesc)
        return (true);  // no instances
    CDp *pn = cdesc->prpty(XICP_PC);
    if (!pn || !pn->string() || !*pn->string())
        return (true);  // name missing, not a pcell
    CDp *pp = cdesc->prpty(XICP_PC_PARAMS);
    if (!pp)
        return (true);  // property missing, not a pcell

    sdesc->setUnread(false);
    return (openSubMaster(pn->string(), pp->string(), 0, sdesc));
}


// If the cell is a pcell sub-master, add it to the table so it will
// resolve calls to instantiate with its parameter set.  Suppose that
// we have a translation of an OpenAccess foreign design containing
// pcells, which includes the pcell masters.  We can gather the pcell
// masters and pre-load them with this function, then read the
// openAccess top-level and get the complete desigh even though we
// can't evaluate the pcells.
//
bool
cPCellDb::recordIfSubMaster(CDs *sdesc)
{
    if (!sdesc || sdesc->isElectrical())
        return (false);

    CDp *pa = sdesc->prpty(XICP_PC);
    if (!pa)
        return (false);
    CDp *pm = sdesc->prpty(XICP_PC_PARAMS);
    if (!pm)
        return (false);
    PCellParam *prm;
    if (PCellParam::parseParams(pm->string(), &prm)) {
        char *lname, *cname, *vname;
        if (PCellDesc::split_dbname(pa->string(), &lname, &cname, &vname)) {
            addSubMaster(lname, cname, vname, prm, Tstring(sdesc->cellname()));
            PCellParam::destroy(prm);
            delete [] lname;
            delete [] cname;
            delete [] vname;
            return (true);
        }
    }
    return (false);
}


// Return true if sd is a valid super-master.
//
bool
cPCellDb::isSuperMaster(const CDs *sd)
{
    // Used for Xic native PCells.

    if (!sd)
        return (false);
    if (!sd->isPCellSuperMaster())
        return (false);
    if (!sd->prpty(XICP_PC_SCRIPT))
        return (false);
    if (!sd->prpty(XICP_PC_PARAMS))
        return (false);
    return (true);
}


namespace {
    struct reverter
    {
        reverter()
            {
                state = EditIf()->saveState();
                SIlcx()->setDoingPCell(true);
            }

        ~reverter()
            {
                EditIf()->revertState(state);
                SIlcx()->setDoingPCell(false);
            }

        void *state;
    };
}


// Switch editing context to sdesc, which is a dup() of a super-master
// cell, execute the script, and fix the properties (remove
// XICP_PC_SCRIPT, add XICP_PC).  Restore editing context and return
// true if succesful.
//
bool
cPCellDb::evalScript(CDs *sdesc, const char *pcname)
{
    // Used for Xic non-OA PCells.

    if (!sdesc)
        return (false);

    PClangType lang;
    SIfile *sfp;
    char *scr;
    if (!openScript(sdesc, &sfp, &scr, &lang))
        return (false);

    reverter rev;
    CDcbin cbin(sdesc);
    DisplayMode tmpmode = DSP()->CurMode();
    DSP()->SetCurMode(sdesc->displayMode());
    CDcellName tmp_cellname = DSP()->CurCellName();
    CDcellName tmp_topname = DSP()->TopCellName();
    DSP()->SetCurCellName(cbin.cellname());
    DSP()->SetTopCellName(cbin.cellname());

    // Some of the Ciranova cells generate coincident boxes and the
    // error message is annoying.
    cCD::DupCheck tmp_dupchk = CD()->DupCheckMode();
    if (!FIO()->IsShowAllPCellWarnings())
        CD()->SetDupCheckMode(cCD::DupNoTest);

    bool ibak = DrcIf()->isInteractive();
    DrcIf()->setInteractive(false);
    bool trd = DSP()->NoRedisplay();
    DSP()->SetNoRedisplay(true);
    BBox BB = *DSP()->MainWdesc()->Window();

    ulPCstate pcstate;
    EditIf()->ulPCevalSet(sdesc, &pcstate);
    CDp *pp = sdesc->prpty(XICP_PC_PARAMS);
    PCellParam *pars = 0;
    if (pp && pp->string() && !PCellParam::parseParams(pp->string(), &pars)) {
        Errs()->add_error("Parameters parse failed.");
        return (false);
    }

    bool err = false;
    CDpcType pctype = sdesc->pcType();
    if (pctype == CDpcXic) {
        if (lang == PCnative) {
            const char *s = scr;
            siVariable result;
            SI()->Interpret(sfp, 0, &s, &result, false, pars);

            // The result will be 0, unless "return" is used.  Return
            // without a value is 1.  If the script reaches the end,
            // or Halt/Exit is called, the value is 0.  If the script
            // crashes, GetError returns 1.  If the script was stopped
            // by an interrupt signal, GetError returns -1.

            // We take a nonzero result as an error.  The PC script
            // author should use this to signal failure.

            err = (result.content.value != 0.0) || SI()->GetError();
        }
        else if (lang == PCpython) {
            if (!PyIf()->hasPy()) {
                Errs()->add_error("Python runtime support not available.");
                err = true;
            }
            else {
                sLstr lstr;
                for (PCellParam *p = pars; p; p = p->next()) {
                    char *a = p->getAssignment();
                    if (a) {
                        lstr.add(a);
                        lstr.add_c('\n');
                        delete [] a;
                    }
                }
                if (scr) {
                    if (!PyIf()->runString(lstr.string(), scr)) {
                        Errs()->add_error("Python returned error.");
                        err = true;
                    }
                }
                else if (sfp) {
                    sLstr sclstr;
                    int c;
                    while ((c = sfp->sif_getc()) != EOF)
                        sclstr.add_c(c);
                    if (!PyIf()->runString(lstr.string(), sclstr.string())) {
                        Errs()->add_error("Python returned error.");
                        err = true;
                    }
                }
            }
        }
        else if (lang == PCtcl) {
            if (!TclIf()->hasTcl()) {
                Errs()->add_error("Tcl runtime support not available.");
                err = true;
            }
            else {
                sLstr lstr;
                for (PCellParam *p = pars; p; p = p->next()) {
                    char *a = p->getTCLassignment();
                    if (a) {
                        lstr.add(a);
                        lstr.add_c('\n');
                        delete [] a;
                    }
                }
                if (scr) {
                    if (!TclIf()->runString(lstr.string(), scr)) {
                        Errs()->add_error("Tcl returned error.");
                        err = true;
                    }
                }
                else if (sfp) {
                    sLstr sclstr;
                    int c;
                    while ((c = sfp->sif_getc()) != EOF)
                        sclstr.add_c(c);
                    if (!TclIf()->runString(lstr.string(), sclstr.string())) {
                        Errs()->add_error("Tcl returned error.");
                        err = true;
                    }
                }
            }
        }
    }
    else {
        // Call foreign evaluation method.
        err = true;
    }
    delete [] scr;
    delete sfp;
    PCellParam::destroy(pars);

    sdesc->prptyRemove(XICP_PC_SCRIPT);
    sdesc->prptyAdd(XICP_PC, pcname);
    sdesc->setPCell(true, false, false);

    EditIf()->ulPCevalReset(&pcstate);

    DrcIf()->setInteractive(ibak);
    DSP()->SetCurMode(tmpmode);
    DSP()->SetCurCellName(tmp_cellname);
    DSP()->SetTopCellName(tmp_topname);
    DSP()->SetNoRedisplay(trd);
    CD()->SetDupCheckMode(tmp_dupchk);
    DSP()->MainWdesc()->InitWindow(&BB);
    return (!err);
}


// Open the XICP_PC_SCRIPT property.  If there is indirection to a
// file, open the file and return a rewound pointer in sfpp. 
// Otherwise, return the property text in strp.  Only one of these
// will be returned non-null.  It is the caller's responsibility to
// delete the returns.  The language pointer is set to the language
// found.
//
// Returns false if error.
//
bool
cPCellDb::openScript(const CDs *sdesc, SIfile **sfpp, char **strp,
    PClangType *lang)
{
    // Used for Xic native PCells.
    // The XIC_PC_SCRIPT property text is in the form
    //
    //    [@LANG langtok] [@READ path] [@MD5 digest] [script text]
    //
    // where langtok may be one of (case insensitive)
    //   n[ative] (native sript, the default)
    //   p[ython] (python script)
    //   t[cl]    (tcl script)
    //
    // The path if given is to the script text, otherwise the script
    // text follows.  The path should be quoted if it contains white
    // space.
    //
    // If an MD5 digest is given, the path file must match.

    *sfpp = 0;
    *strp = 0;
    *lang = PCnative;
    CDp *pscr = sdesc->prpty(XICP_PC_SCRIPT);
    if (!pscr)
        return (false);
    // Strip/expand any hypertext.
    char *scr = hyList::hy_strip(pscr->string());
    if (!scr)
        return (false);
    char *s = scr;
    while (isspace(*s))
        s++;

    char *filename = 0;
    char *digest = 0;

    for (;;) {

        if (lstring::ciprefix(PCELL_LANG_TOK, s)) {
            while (*s && !isspace(*s))
                s++;
            while (isspace(*s))
                s++;
            if (lstring::ciprefix("n", s))
                *lang = PCnative;
            else if (lstring::ciprefix("p", s))
                *lang = PCpython;
            else if (lstring::ciprefix("t", s))
                *lang = PCtcl;
            // If language is not recognized, just ignore here and
            // take as native.  We'll handle the error during
            // evaluation.

            while (*s && !isspace(*s))
                s++;
            while (isspace(*s))
                s++;
        }
        else if (lstring::ciprefix(PCELL_READ_TOK, s)) {
            while (*s && !isspace(*s))
                s++;
            while (isspace(*s))
                s++;
            filename = lstring::getqtok(&s);
            if (!filename) {
                delete [] scr;
                delete [] digest;
                return (false);
            }
            char *tfn = filename;
            filename = pathlist::expand_path(filename, false, true);
            delete [] tfn;
        }
        else if (lstring::ciprefix(PCELL_MD5_TOK, s)) {
            while (*s && !isspace(*s))
                s++;
            while (isspace(*s))
                s++;
            digest = lstring::getqtok(&s);
            if (!digest) {
                delete [] scr;
                delete [] filename;
                return (false);
            }
        }
        else
            break;
    }
    char *scrtmp = scr;
    scr = lstring::copy(s);
    delete [] scrtmp;

    if (filename) {
        FILE *fp;
        if (lstring::is_rooted(filename))
            fp = fopen(filename, "rb");
        else {
            // Open file through a search path.
            char *realfn;
            fp = pathlist::open_path_file(filename,
                CDvdb()->getVariable(VA_PCellScriptPath), "rb", &realfn, true);
            if (fp) {
                delete [] filename;
                filename = realfn;
            }
        }
        if (!fp) {
            Errs()->add_error("Can't open %s for reading.", filename);
            delete [] filename;
            delete [] digest;
            delete [] scr;
            return (false);
        }
        if (digest) {
            char *tdg = md5Digest(filename);
            if (!tdg || !lstring::cieq(tdg, digest)) {
                Errs()->add_error("Checksum mismatch to script file.");
                delete [] filename;
                delete [] digest;
                delete [] scr;
                fclose(fp);
                return (false);
            }
            delete [] tdg;
            delete [] digest;
        }

        *sfpp = SIfile::create(filename, fp, 0);
        if (!*sfpp) {
            Errs()->add_error("Decryption failed for %s.", filename);
            delete [] filename;
            delete [] scr;
            return (false);
        }

        delete [] filename;
        delete [] scr;
        return (true);
    }
    *strp = scr;
    return (true);
}


// Return a string containing an MD5 digest for the file whose path is
// passed as the argument.  This is the same digest as returned from
// the !md5 command, the Md5Digest script function, and from "openssl
// dgst -md5 filepath" on many Linux-like systems.
//
char *
cPCellDb::md5Digest(const char *path)
{
    if (!path || !*path) {
        Errs()->add_error("md5Digest: null or empty path.");
        return (0);
    }
    FILE *fp = filestat::open_file(path, "rb");
    if (!fp) {
        Errs()->add_error(filestat::error_msg());
        return (0);
    }

    unsigned char buf[1000];
    long length = 0;
    MD5cx context;
    int nbytes;
    while ((nbytes = fread(buf, 1, sizeof(buf), fp)) != 0) {
        length += nbytes;
        context.update(buf, nbytes);
    }
    fclose(fp);
    unsigned char digest[16];
    context.final(digest);
    char tbf[4];
    sLstr lstr;
    for (int i = 0; i < 16; i++) {
        sprintf(tbf, "%02x", digest[i]);
        lstr.add(tbf);
    }
    return (lstr.string_trim());
}


void
cPCellDb::dump(FILE *fp)
{
    if (!pc_master_tab)
        return;
    if (!fp)
        fp = stdout;
    SymTabGen gen(pc_master_tab);
    SymTabEnt *ent;
    while ((ent = gen.next()) != 0) {
        PCellDesc *pd = (PCellDesc*)ent->stData;
        fprintf(fp, "%s\n", pd->dbname());
        for (const PCellParam *p = pd->defaultParams(); p; p = p->next()) {
            char *s = p->this_string(false);
            fprintf(fp, "  %s\n", s);
            delete [] s;
        }
    }
}


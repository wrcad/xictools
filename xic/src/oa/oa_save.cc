
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
#include "cd_hypertext.h"
#include "cd_terminal.h"
#include "cd_netname.h"
#include "cd_propnum.h"
#include "cd_lgen.h"
#include "cd_celldb.h"
#include "dsp_tkif.h"
#include "dsp_inlines.h"
#include "fio.h"
#include "fio_library.h"
#include "pcell_params.h"
#include "promptline.h"
#include "errorlog.h"
#include "si_parsenode.h"
#include "tech.h"
#include "tech_cds_out.h"
#include "oa.h"
#include "oa_prop.h"
#include "oa_via.h"
#include "miscutil/texttf.h"


class oa_save
{
public:
    oa_save(cOA *oa)
        {
            out_owner = oa;
            out_warnings = 0;
            out_devsused = 0;
            out_mode = Physical;
            out_defer_tech = 0;
            out_virtuoso = false;
        }

    ~oa_save()
        {
            stringlist::destroy(out_warnings);
            delete out_devsused;
        }

    stringlist *warnings()  { return (out_warnings); }

    bool save_cell(CDs*, const char*, const char* = 0);
    bool update_tech(const char*);
    bool save_devices();
    bool save_device_lib();

    enum ViewType { VTphysical, VTelectrical, VTsymbolic };

    void push_defer_tech_upd()
        {
            out_defer_tech++;
        }

    void pop_defer_tech_upd()
        {
            if (out_defer_tech > 0)
                out_defer_tech--;
        }

private:
    bool save_phys_cell(CDs*, const char*, const char*);
    bool save_phys_inst(CDs*, const char*, oaBlock*);
    bool save_phys_geom(CDs*, const char*, oaBlock*, oaTech*);
    bool save_elec_cell(CDs*, const char*, const char*);
    bool save_elec_inst(CDs*, const char*, oaBlock*);
    bool save_elec_geom(CDs*, const char*, oaBlock*, oaTech*);
    bool save_symb_cell(CDs*, CDs*, const char*, const char*, bool, bool);
    bool save_symb_geom(CDs*, const char*, oaBlock*, oaTech*);
    oaDesign *open_design(const char*, const char*, ViewType, bool);
    void save_cell_properties(CDs*, oaObject*);
    void save_obj_properties(const CDo*, oaObject*);
    oaOrient orient_from_tx(const CDtx&);
    oaOrient orient_from_xform(int);
    oaTextAlign align_from_xform(int);

    cOA *out_owner;
    stringlist *out_warnings;
    SymTab *out_devsused;
    oaNativeNS out_ns;
    DisplayMode out_mode;
    int out_defer_tech;
    bool out_virtuoso;
};


// Save pcbin to libname.  If allhier, save the entire hierarchy,
// otherwise just the named cell.  When saving just the cell, altname
// can be used to save the cell under a different name,
//
bool
cOA::save(const CDcbin *pcbin, const char *libname, bool allhier,
    const char *altname)
{
    const char *s = CDvdb()->getVariable(VA_OaUseOnly);
    if (s && ((s[0] == '1' && s[1] == 0) || s[0] == 'p' || s[0] == 'P')) {
        if (!pcbin->phys())
            return (true);
        if (allhier)
            return (save_cell_hier(pcbin->phys(), libname));
        return (save_cell(pcbin->phys(), libname, altname));
    }
    if (s && ((s[0] == '2' && s[1] == 0) || s[0] == 'e' || s[0] == 'E')) {
        if (!pcbin->elec())
            return (true);
        if (allhier)
            return (save_cell_hier(pcbin->elec(), libname));
        return (save_cell(pcbin->elec(), libname, altname));
    }
    if (allhier)
        return (save_cbin_hier(pcbin, libname));
    return (save_cbin(pcbin, libname, altname));
}


// Save to libname pcbin and all of its hierarchy.
//
bool
cOA::save_cbin_hier(const CDcbin *pcbin, const char *libname)
{
    if (!pcbin)
        return (true);

    bool branded;
    if (!is_lib_branded(libname, &branded))
        return (false);
    if (!branded) {
        Errs()->add_error("Library not writable from Xic.");
        return (false);
    }

    oa_save out(this);
    out.push_defer_tech_upd();
    dspPkgIf()->SetWorking(true);
    CDgenHierDn_cbin gen(pcbin);
    bool err = false;
    CDcbin cbin;
    while (gen.next(&cbin, &err) != 0) {
        if (dspPkgIf()->CheckForInterrupt()) {
            if (XM()->ConfirmAbort("Interrupt received, abort save? "))
                break;
        }
        try {
            if (cbin.phys()) {
                if (!out.save_cell(cbin.phys(), libname)) {
                    err = true;
                    break;
                }
            }
            if (cbin.elec()) {
                if (!out.save_cell(cbin.elec(), libname)) {
                    err = true;
                    break;
                }
            }
        }
        catch (oaCompatibilityError &ex) {
            cOA::handleFBCError(ex);
            err = true;
            break;
        }
        catch (oaException &excp) {
            Errs()->add_error((const char*)excp.getMsg());
            err = true;
            break;
        }
    }

    out.pop_defer_tech_upd();
    if (!err && !out.update_tech(libname))
        err = true;
    if (!err && !out.save_devices())
        err = true;
    dspPkgIf()->SetWorking(false);

    if (!err && out.warnings()) {
        sLstr lstr;
        for (stringlist *s = out.warnings(); s; s = s->next) {
            lstr.add(s->string);
            lstr.add_c('\n');
        }
        Log()->WarningLog(mh::OpenAccess, lstr.string());
    }
    return (!err);
}


// Save to libname sdesc and all of its hierarchy.
//
bool
cOA::save_cell_hier(CDs *sdesc, const char *libname)
{
    if (!sdesc)
        return (false);

    bool branded;
    if (!is_lib_branded(libname, &branded))
        return (false);
    if (!branded) {
        Errs()->add_error("Library not writable from Xic.");
        return (false);
    }

    oa_save out(this);
    out.push_defer_tech_upd();
    dspPkgIf()->SetWorking(true);
    CDgenHierDn_s gen(sdesc);
    bool err = false;
    CDs *sd;
    while ((sd = gen.next(&err)) != 0) {
        if (dspPkgIf()->CheckForInterrupt()) {
            if (XM()->ConfirmAbort("Interrupt received, abort save? "))
                break;
        }
        try {
            if (!out.save_cell(sd, libname)) {
                err = true;
                break;
            }
        }
        catch (oaCompatibilityError &ex) {
            cOA::handleFBCError(ex);
            err = true;
            break;
        }
        catch (oaException &excp) {
            Errs()->add_error((const char*)excp.getMsg());
            err = true;
            break;
        }
    }
    out.pop_defer_tech_upd();

    if (!err && !out.update_tech(libname))
        err = true;
    dspPkgIf()->SetWorking(false);

    if (!err && out.warnings()) {
        sLstr lstr;
        for (stringlist *s = out.warnings(); s; s = s->next) {
            lstr.add(s->string);
            lstr.add_c('\n');
        }
        Log()->WarningLog(mh::OpenAccess, lstr.string());
    }
    return (!err);
}


// Save to libname cbin only.  If altcname is given, it will be used to
// name the OA cell.
//
bool
cOA::save_cbin(const CDcbin *cbin, const char *libname, const char *altcname)
{
    if (!cbin)
        return (false);

    bool branded;
    if (!is_lib_branded(libname, &branded))
        return (false);
    if (!branded) {
        Errs()->add_error("Library not writable from Xic.");
        return (false);
    }

    oa_save out(this);
    out.push_defer_tech_upd();
    dspPkgIf()->SetWorking(true);
    bool ret = true;
    try {
        if (cbin->phys())
            ret = out.save_cell(cbin->phys(), libname, altcname);
        if (ret && cbin->elec())
            ret = out.save_cell(cbin->elec(), libname, altcname);
    }
    catch (oaCompatibilityError &ex) {
        cOA::handleFBCError(ex);
        ret = false;
    }
    catch (oaException &excp) {
        Errs()->add_error((const char*)excp.getMsg());
        ret = false;
    }

    out.pop_defer_tech_upd();
    if (ret && !out.update_tech(libname))
        ret = false;
    if (ret && !out.save_devices())
        ret = false;
    dspPkgIf()->SetWorking(false);

    if (ret && out.warnings()) {
        sLstr lstr;
        for (stringlist *s = out.warnings(); s; s = s->next) {
            lstr.add(s->string);
            lstr.add_c('\n');
        }
        Log()->WarningLog(mh::OpenAccess, lstr.string());
    }
    return (ret);
}


// Save to libname sdesc only.  If altcname is given, it will be used to
// name the OA cell.
//
bool
cOA::save_cell(CDs *sdesc, const char *libname, const char *altcname)
{
    if (!sdesc)
        return (false);

    bool branded;
    if (!is_lib_branded(libname, &branded))
        return (false);
    if (!branded) {
        Errs()->add_error("Library not writable from Xic.");
        return (false);
    }

    bool ret = true;
    dspPkgIf()->SetWorking(true);
    oa_save out(this);
    try {
        ret = out.save_cell(sdesc, libname, altcname);
    }
    catch (oaCompatibilityError &ex) {
        cOA::handleFBCError(ex);
        ret = false;
    }
    catch (oaException &excp) {
        Errs()->add_error((const char*)excp.getMsg());
        ret = false;
    }
    if (ret && !out.update_tech(libname))
        ret = false;

    dspPkgIf()->SetWorking(false);
    if (ret && out.warnings()) {
        sLstr lstr;
        for (stringlist *s = out.warnings(); s; s = s->next) {
            lstr.add(s->string);
            lstr.add_c('\n');
        }
        Log()->WarningLog(mh::OpenAccess, lstr.string());
    }
    return (ret);
}
// End of cOA functions.


bool
oa_save::save_cell(CDs *sd, const char *libname, const char *altcname)
{
    if (!sd || !libname)
        return (false);

    // The oldlibname is the originating library name, if different
    // than libname.
    const char *oldlibname = 0;
    if (sd->fileType() == Foa) {
        oldlibname = sd->fileName();
        if (oldlibname && !strcmp(oldlibname, libname))
            oldlibname = 0;
    }
    if (!out_owner->create_lib(libname, oldlibname))
        return (false);
    oaScalarName libName(oaNativeNS(), libname);

    PL()->ShowPromptV("Saving: %s", Tstring(sd->cellname()));
    bool ret = sd->isElectrical() ?
        save_elec_cell(sd, libname, altcname) :
        save_phys_cell(sd, libname, altcname);
    if (ret)
        sd->clearModified();
    return (ret);
}


// This updates the tech.db file, call when done writing cell data.
//
bool
oa_save::update_tech(const char *libname)
{
    if (out_defer_tech > 0)
        return (true);
    try {
        oaScalarName libraryName(out_ns, libname);
        oaLib *lib = oaLib::find(libraryName);

        // Only update if tech is local to this library.
        if (oaTech::hasAttachment(lib))
            return (true);

        oaTech *tech = oaTech::find(lib);
        if (tech) {
            tech->reopen('a');
            tech->save();
            tech->purge();
        }
    }
    catch (oaCompatibilityError &ex) {
        cOA::handleFBCError(ex);
        return (false);
    }
    catch (oaException &excp) {
        Errs()->add_error((const char*)excp.getMsg());
        return (false);
    }
    return (true);
}


// Save devices used by cells written.  A device will be saved only if
// it doesn't already exist in the OA database.
//
bool
oa_save::save_devices()
{
    if (!out_devsused)
        return (true);
    SymTabGen gen(out_devsused);
    SymTabEnt *h;
    while ((h = gen.next()) != 0) {
        CDs *sd = (CDs*)h->stData;
        if (!sd)
            continue;
        try {
            oaScalarName libName(oaNativeNS(), XIC_DEVICES);
            oaLib *lib = oaLib::find(libName);
            if (!lib) {
                if (!out_owner->create_lib(XIC_DEVICES, 0))
                    return (false);
            }
            if (!save_symb_cell(sd, 0, XIC_DEVICES, 0, true, true)) {
                Log()->ErrorLog(mh::OpenAccess, Errs()->get_error());
                return (false);
            }
        }
        catch (oaCompatibilityError &ex) {
            cOA::handleFBCError(ex);
            return (false);
        }
        catch (oaException &excp) {
            Errs()->add_error((const char*)excp.getMsg());
            return (false);
        }
    }
    return (update_tech(XIC_DEVICES));
}


// Save all devices from the device library, overwriting any existing
// cells.
//
bool
oa_save::save_device_lib()
{
    stringlist *s0 = FIO()->GetLibNamelist(XM()->DeviceLibName(), LIBdevice);
    if (!s0)
        return (true);

    try {
        oaScalarName libName(oaNativeNS(), XIC_DEVICES);
        oaLib *lib = oaLib::find(libName);
        if (!lib) {
            if (!out_owner->create_lib(XIC_DEVICES, 0))
                return (false);
        }
        for (stringlist *s = s0; s; s = s->next) {
            CDs *sd = CDcdb()->findCell(s->string, Electrical);
            if (!sd)
                continue;
            if (!save_symb_cell(sd, 0, XIC_DEVICES, 0, false, true)) {
                Log()->ErrorLog(mh::OpenAccess, Errs()->get_error());
                stringlist::destroy(s0);
                return (false);
            }
        }
    }
    catch (oaCompatibilityError &ex) {
        cOA::handleFBCError(ex);
        stringlist::destroy(s0);
        return (false);
    }
    catch (oaException &excp) {
        Errs()->add_error((const char*)excp.getMsg());
        stringlist::destroy(s0);
        return (false);
    }
    if (s0) {
        stringlist::destroy(s0);
        return (update_tech(XIC_DEVICES));
    }
    return (true);
}


// Remaining methods are private.
//

bool
oa_save::save_phys_cell(CDs *sdesc, const char *libname, const char *altcname)
{
    if (!sdesc || !libname)
        return (false);

    if (sdesc->isPCellSubMaster()) {
        // If this not an Xic-native sub-master, skip writing to OA,
        // as the pcell is available from OA.
        //
        if (sdesc->pcType() == CDpcOA)
            return (true);

        // Don't save master unless we're explicitly saving.
        //
        if (!FIO()->IsKeepPCellSubMasters() && !sdesc->isPCellReadFromFile())
            return (true);
    }

    out_mode = Physical;

    oaScalarName libName(out_ns, libname);

    CDp *pd = sdesc->prpty(XICP_STDVIA);
    if (pd) {
        // This cell was created to implement a standard via from OA. 
        // If we find the standard via in the present tech database,
        // skip writing this cell.

        const char *s = pd->string();
        char *vname = lstring::gettok(&s);
        if (vname) {
            oaTech *tech = oaTech::find(libName);
            if (!tech) {
                if (oaTech::exists(libName))
                    tech = oaTech::open(libName, 'r');
                else
                    tech = oaTech::create(libName);
            }
            oaViaDef *vdef = oaViaDef::find(tech, vname);
            bool stdv = (vdef && vdef->getType() == oacStdViaDefType);
            delete [] vname;
            if (stdv)
                return (true);
        }
    }
    else {
        pd = sdesc->prpty(XICP_CSTMVIA);
        if (pd) {
            // This cell was created to implement a custom via from OA. 
            // If we find the custom via in the present tech database,
            // skip writing this cell.

            const char *s = pd->string();
            char *vname = lstring::gettok(&s);
            if (vname) {
                oaTech *tech = oaTech::find(libName);
                if (!tech) {
                    if (oaTech::exists(libName))
                        tech = oaTech::open(libName, 'r');
                    else
                        tech = oaTech::create(libName);
                }
                oaViaDef *vdef = oaViaDef::find(tech, vname);
                bool cstv = (vdef && vdef->getType() == oacCustomViaDefType);
                delete [] vname;
                if (cstv)
                    return (true);
            }
        }
    }

    const char *cellname = Tstring(sdesc->cellname());
    if (altcname && strcmp(cellname, altcname))
        cellname = altcname;

    oaDesign *design = open_design(libname, cellname, VTphysical, false);
    if (!design)
        return (false);
    save_cell_properties(sdesc, design);

    oaBlock *block = oaBlock::create(design);

    if (!save_phys_inst(sdesc, cellname, block)) {
        design->close();
        return (false);
    }

    oaTech *tech = oaTech::find(libName);
    if (!tech) {
        if (oaTech::exists(libName))
            tech = oaTech::open(libName, 'r');
        else
            tech = oaTech::create(libName);
    }
    bool ret = save_phys_geom(sdesc, cellname, block, tech);
    if (ret)
        design->save();
    design->close();
    return (ret);
}


namespace {
    bool handleVia(const CDc *cdesc, const oaTransform &tform, oaBlock *block,
        const oaScalarName &libName, const char *cellname, bool *via_created)
    {
        *via_created = false;
        CDp *pd = cdesc->prpty(XICP_STDVIA);
        if (pd) {
            // This instance was created to represent an OA standard
            // via.  Attempt to save it as such.  If the standard via
            // name is unknown in the present technology, save as a
            // regular instance.

            const char *s = pd->string();
            char *vianame = lstring::gettok(&s);
            if (!vianame) {
                Errs()->add_error(
                    "Fatal error: In cell %s, instance of %s has\n"
                    "standard via property with no via name.",
                    cellname, Tstring(cdesc->cellname()));
                return (false);
            }
            oaViaParam vparam;
            if (!cOAvia::parseStdViaString(s, vparam)) {
                Errs()->add_error(
                    "Fatal error: In cell %s, instance of %s has\n"
                    "syntax error in standard via property.",
                    cellname, Tstring(cdesc->cellname()));
                delete [] vianame;
                return (false);
            }
            oaTech *tech = oaTech::find(libName);
            if (!tech) {
                if (oaTech::exists(libName))
                    tech = oaTech::open(libName, 'r');
            }
            if (!tech) {
                delete [] vianame;
                return (true);
            }
            oaViaDef *vdef = oaViaDef::find(tech, vianame);
            delete [] vianame;
            if (vdef) {
                if (vdef->getType() != oacStdViaDefType) {
                    Errs()->add_error(
                        "Fatal error: In cell %s, instance of %s has\n"
                        "incorrect standard via name.",
                        cellname, Tstring(cdesc->cellname()));
                    return (false);
                }
                oaStdVia::create(block, (oaStdViaDef*)vdef, tform, &vparam);
                *via_created = true;
            }
            return (true);
        }

        pd = cdesc->prpty(XICP_CSTMVIA);
        if (pd) {
            // This instance was created to represent an OA custom
            // via.  Attempt to save it as such.  If the custom via
            // name is unknown in the present technology, save as a
            // regular instance.

            const char *s = pd->string();
            char *vianame = lstring::gettok(&s);
            if (!vianame) {
                Errs()->add_error(
                    "Fatal error: In cell %s, instance of %s has\n"
                    "custom via property with no via name.",
                    cellname, Tstring(cdesc->cellname()));
                return (false);
            }
            PCellParam *prms;
            if (!PCellParam::parseParams(s, &prms)) {
                Errs()->add_error(
                    "Fatal error: In cell %s, instance of %s has\n"
                    "custom via property syntax error.",
                    cellname, Tstring(cdesc->cellname()));
                delete [] vianame;
                return (false);
            }

            oaTech *tech = oaTech::find(libName);
            if (!tech) {
                if (oaTech::exists(libName))
                    tech = oaTech::open(libName, 'r');
            }
            if (!tech) {
                delete [] vianame;
                PCellParam::destroy(prms);
                return (true);
            }
            oaViaDef *vdef = oaViaDef::find(tech, vianame);
            delete [] vianame;
            if (vdef) {
                if (vdef->getType() != oacCustomViaDefType) {
                    Errs()->add_error(
                        "Fatal error: In cell %s, instance of %s has\n"
                        "incorrect custom via name.",
                        cellname, Tstring(cdesc->cellname()));
                    PCellParam::destroy(prms);
                    return (false);
                }
                oaParamArray parray;
                if (prms)
                    cOAprop::savePcParameters(prms, parray);
                oaCustomVia::create(block, (oaCustomViaDef*)vdef, tform,
                    prms ? &parray : 0);
                *via_created = true;
            }
            PCellParam::destroy(prms);
        }
        return (true);
    }
}


bool
oa_save::save_phys_inst(CDs *sdesc, const char *cellname, oaBlock *block)
{
    oaDesign *design = block->getDesign();
    oaScalarName libName, cellName, viewName;
    design->getLibName(libName);
    design->getCellName(cellName);
    design->getViewName(viewName);

    // Loop through physical cell instances.
    CDm_gen mgen(sdesc, GEN_MASTERS);
    for (CDm *mdesc = mgen.m_first(); mdesc; mdesc = mgen.m_next()) {
        CDs *msdesc = mdesc->celldesc();
        if (!msdesc)
            continue;
        CDp *pc_name = msdesc->prpty(XICP_PC);
        CDp *pc_parm = msdesc->prpty(XICP_PC_PARAMS);
        CDc_gen cgen(mdesc);
        for (CDc *cdesc = cgen.c_first(); cdesc; cdesc = cgen.c_next()) {
            if (!cdesc->is_normal())
                continue;

            CDap ap(cdesc);
            CDtx tx(cdesc);
            if (tx.ax && tx.ay) {
                // Error, OA can't handle 45's.
                Errs()->add_error(
                    "Fatal error: In cell %s, instance of %s has\n"
                    "nonorthogonal rotation, can't represent this "
                    "in OpenAccess.",
                    cellname, Tstring(cdesc->cellname()));
                return (false);
            }
            oaTransform tform(tx.tx, tx.ty, orient_from_tx(tx));

            bool via_created;
            if (!handleVia(cdesc, tform, block, libName, cellname,
                    &via_created))
                return (false);
            if (via_created)
                continue;

            if (msdesc->isPCellSubMaster() && msdesc->pcType() == CDpcOA) {
                // The cdesc is an instance of an OA pcell.

                if (!pc_name)
                    pc_name = cdesc->prpty(XICP_PC);
                if (!pc_name) {
                    // No pc name property, error!
                    Errs()->add_error(
                        "Fatal error: In cell %s, instance of %s has\n"
                        "no pc_name property.",
                        cellname, Tstring(cdesc->cellname()));
                    return (false);
                }
                if (!pc_parm)
                    pc_parm = cdesc->prpty(XICP_PC_PARAMS);
                if (!pc_parm) {
                    // No params property, error!
                    Errs()->add_error(
                        "Fatal error: In cell %s, instance of %s has\n"
                        "no pc_params property.",
                        cellname, Tstring(cdesc->cellname()));
                    return (false);
                }

                char *lname, *cname, *vname;
                const char *dbname = pc_name->string();
                if (!PCellDesc::split_dbname(dbname, &lname, &cname,
                        &vname)) {
                    Errs()->add_error(
                        "Fatal error: In cell %s, instance of %s has\n"
                        "bad pc_name property.",
                        cellname, Tstring(cdesc->cellname()));
                    return (false);
                }
                PCellDesc::LCVcleanup lcv(lname, cname, vname);

                PCellParam *prms;
                if (!PCellParam::parseParams(pc_parm->string(), &prms)) {
                    // Parse error.
                    Errs()->add_error(
                        "Fatal error: In cell %s, instance of %s has\n"
                        "parse error in pc_params property.",
                        cellname, Tstring(cdesc->cellname()));
                    return (false);
                }
                oaParamArray parray;
                cOAprop::savePcParameters(prms, parray);

                oaScalarName pcLibName(out_ns, lname);
                oaScalarName pcCellName(out_ns, cname);
                oaScalarName pcViewName(out_ns, vname);

                if (ap.nx <= 1 && ap.ny <= 1) {
                    oaInst *inst = oaScalarInst::create(block, pcLibName,
                        pcCellName, pcViewName, tform, &parray);
                    save_obj_properties(cdesc, inst);
                }
                else {
                    oaInst *inst = oaArrayInst::create(block, pcLibName,
                        pcCellName, pcViewName, tform, ap.dx, ap.dy,
                        ap.ny, ap.nx, &parray);
                    save_obj_properties(cdesc, inst);
                }
                continue;
            }
            oaScalarName cellName(out_ns, Tstring(cdesc->cellname()));
            if (ap.nx <= 1 && ap.ny <= 1) {
                oaScalarInst *inst = oaScalarInst::create(block, libName,
                    cellName, viewName, tform);
                save_obj_properties(cdesc, inst);
            }
            else {
                oaArrayInst *inst = oaArrayInst::create(block, libName,
                    cellName, viewName, tform, ap.dx, ap.dy, ap.ny, ap.nx);
                save_obj_properties(cdesc, inst);
            }
        }
    }
    return (true);
}


bool
oa_save::save_phys_geom(CDs *sdesc, const char *cellname, oaBlock *block,
    oaTech *tech)
{
    oaDesign *design = block->getDesign();
    oaScalarName libName, cellName, viewName;
    design->getLibName(libName);
    design->getCellName(cellName);
    design->getViewName(viewName);

    bool lwarn = false;
    bool wwarn1 = false;
    bool wwarn2 = false;
    CDsLgen lgen(sdesc);
    CDl *ld;
    while ((ld = lgen.next()) != 0) {
        unsigned int layernum = ld->oaLayerNum();
        unsigned int purposenum = ld->oaPurposeNum();
        const char *layername = CDldb()->getOAlayerName(layernum);
        const char *purposename = CDldb()->getOApurposeName(purposenum);
        if (!purposename) {
            purposename = CDL_PRP_DRAWING;
            purposenum = oavPurposeNumberDrawing;
        }

        CDg gdesc;
        gdesc.init_gen(sdesc, ld);
        bool first = true;
        CDo *odesc;
        while ((odesc = gdesc.next()) != 0) {
            if (!odesc->is_normal())
                continue;
            if (first) {
                if (!oaPhysicalLayer::find(tech, layernum) &&
                        !oaPhysicalLayer::find(tech, layername))
                    oaPhysicalLayer::create(tech, layername, layernum);
                if (!oaPurpose::find(tech, purposename) &&
                        !oaPurpose::find(tech, purposenum))
                    oaPurpose::create(tech, purposename, purposenum);
                first = false;
            }

            if (odesc->type() == CDBOX) {
                oaBox box(odesc->oBB().left, odesc->oBB().bottom,
                    odesc->oBB().right, odesc->oBB().top);
                oaRect *rect = oaRect::create(block, layernum, purposenum,
                    box);
                save_obj_properties(odesc, rect);
            }
            else if (odesc->type() == CDPOLYGON) {
                oaPointArray pArray;
                int num = ((CDpo*)odesc)->numpts();
                const Point *pts = ((CDpo*)odesc)->points();
                num--;  // closure not included in OA
                for (int i = 0; i < num; i++)
                    pArray.append(oaPoint(pts[i].x, pts[i].y));

                oaPolygon *poly = oaPolygon::create(block, layernum,
                    purposenum, pArray);
                save_obj_properties(odesc, poly);
            }
            else if (odesc->type() == CDWIRE) {
                CDw *wd = (CDw*)odesc;
                int num = wd->numpts();
                const Point *pts = wd->points();
                oaPointArray pArray;
                for (int i = 0; i < num; i++)
                    pArray.append(oaPoint(pts[i].x, pts[i].y));

                if (pArray.getNumElements() > 1) {
                    oaPathStyle style = oacTruncatePathStyle;
                    if (wd->wire_style() == CDWIRE_EXTEND)
                        style = oacExtendPathStyle;
                    else if (wd->wire_style() == CDWIRE_ROUND)
                        style = oacRoundPathStyle;

                    oaPath *path = oaPath::create(block, layernum, purposenum,
                        ((CDw*)odesc)->wire_width(), pArray, style);
                    save_obj_properties(odesc, path);
                }
                else {
                    // Deal with single vertex wire, OA can't handle.
                    sLstr lstr;
                    Poly po;
                    if (!((CDw*)odesc)->w_toPoly(&po.points, &po.numpts)) {
                        if (!wwarn1) {
                            lstr.add("Warning, in physical cell ");
                            lstr.add(cellname);
                            lstr.add(", one-vertex wire ignored.");
                            out_warnings = new stringlist(lstr.string_trim(),
                                out_warnings);
                            wwarn1 = true;
                        }
                    }
                    else {
                        oaPointArray pArray;
                        po.numpts--;  // closure not included in OA
                        for (int i = 0; i < po.numpts; i++)
                            pArray.append(oaPoint(po.points[i].x,
                                po.points[i].y));
                        delete [] po.points;
                        oaPolygon *poly = oaPolygon::create(block, layernum,
                            purposenum, pArray);
                        save_obj_properties(odesc, poly);

                        if (!wwarn2) {
                            lstr.add("Warning, in physical cell ");
                            lstr.add(cellname);
                            lstr.add(
                                ", one-vertex wire converted to polygon.");
                            out_warnings = new stringlist(lstr.string_trim(),
                                out_warnings);
                            wwarn2 = true;
                        }
                    }
                }
            }
            else if (odesc->type() == CDLABEL) {
                CDla *la = (CDla*)odesc;
                int xform = la->xform() & ~TXTF_LIML;

                oaTextAlign align = align_from_xform(xform);

                if (xform & TXTF_45) {
                    // OA can't handle 45's
                    if (!lwarn) {
                        sLstr lstr;
                        lstr.add("Warning: In physical cell ");
                        lstr.add(cellname);
                        lstr.add(
                        ", nonorthogonal text rotation set\nto orthogonal.");
                        out_warnings = new stringlist(lstr.string_trim(),
                            out_warnings);
                        lwarn = true;
                    }
                    xform &= ~TXTF_45;
                }
                oaOrient orient = orient_from_xform(xform);

                char *string = hyList::string(la->label(), HYcvAscii, true);
                int nlines = DSP()->LabelExtent(string, 0, 0);

                int lhei = (int)(la->height()/nlines/CDS_TEXT_SCALE);
                oaText *text = oaText::create(block, layernum, purposenum,
                    string, oaPoint(la->xpos(), la->ypos()), align, orient,
                    oacEuroStyleFont, lhei);
                save_obj_properties(odesc, text);

                // Save a property for this attribute (label shown in
                // top-level only, not in instances).
                if (la->no_inst_view())
                    oaStringProp::create(text, "XICP_NO_INST_VIEW", "t");

                delete [] string;
            }
        }
    }
    return (true);
}


bool
oa_save::save_elec_cell(CDs *sdesc, const char *libname, const char *altcname)
{
    if (!sdesc || !libname)
        return (false);
    if (sdesc->isLibrary() && sdesc->isDevice())
        return (true);

    if (out_virtuoso) {
        // If the cell has an XICP_PC property, it may be the
        // electrical/symbol from an OA pcell which should not be
        // written back.

        CDp *pd = sdesc->prpty(XICP_PC);
        if (pd) {
            char *lname, *cname, *vname;
            if (!PCellDesc::split_dbname(pd->string(), &lname, &cname,
                    &vname)) {
                Errs()->add_error(
                    "In electrical cell %s, pc_name property has "
                    "syntax error.",
                    Tstring(sdesc->cellname()));
                return (false);
            }
            delete [] cname;
            delete [] vname;
            if (strcmp(lname, XIC_NATIVE_LIBNAME)) {
                delete [] lname;
                return (true);
            }
            delete [] lname;
            // Cell is from a native Xic PCell, which is written.
        }
    }

    out_mode = Electrical;

    if (!ScedIf()->connect(sdesc))
        return (false);

    const char *cellname = Tstring(sdesc->cellname());
    if (altcname && strcmp(cellname, altcname))
        cellname = altcname;

    oaDesign *design = open_design(libname, cellname, VTelectrical, false);
    if (!design)
        return (false);
    save_cell_properties(sdesc, design);

    oaBlock *block = oaBlock::create(design);

    if (!save_elec_inst(sdesc, cellname, block)) {
        design->save();
        return (false);
    }

    oaScalarName libName(out_ns, libname);
    oaTech *tech = oaTech::find(libName);
    if (!tech) {
        if (oaTech::exists(libName))
            tech = oaTech::open(libName, 'r');
        else
            tech = oaTech::create(libName);
    }
    bool ret = save_elec_geom(sdesc, cellname, block, tech);
    if (ret)
        design->save();
    design->close();
    if (!ret)
        return (false);

    // Save symbolic representation, if any.
    CDs *srep = sdesc->symbolicRep(0);
    if (srep)
        return (save_symb_cell(srep, sdesc, libname, cellname, false, false));
    return (true);
}


bool
oa_save::save_elec_inst(CDs *sdesc, const char *cellname, oaBlock *block)
{
    oaDesign *design = block->getDesign();
    oaScalarName libName, cellName, viewName;
    design->getLibName(libName);
    design->getCellName(cellName);
    design->getViewName(viewName);

    oaScalarName devLibName(out_ns, XIC_DEVICES);
    const char *vname = CDvdb()->getVariable(VA_OaDefSymbolView);
    if (!vname)
        vname = OA_DEF_SYMBOL;
    oaScalarName symViewName(out_ns, vname);
    char buf[256];

    // Loop through electrical cell instances.
    CDm_gen mgen(sdesc, GEN_MASTERS);
    for (CDm *mdesc = mgen.m_first(); mdesc; mdesc = mgen.m_next()) {
        CDs *msdesc = mdesc->celldesc();
        if (!msdesc)
            continue;

        char *lname = 0, *cname = 0, *vname = 0;
        CDp *pc_name = msdesc->prpty(XICP_PC);
        if (out_virtuoso && pc_name) {
            // This master maybe be the symbol part of an OA PCell.
            // If so, the master is not written to OA.
            if (!PCellDesc::split_dbname(pc_name->string(), &lname,
                    &cname, &vname)) {
                Errs()->add_error(
                    "Fatal error: In cell %s, master %s has\n"
                    "bad pc_name property.",
                    cellname, Tstring(msdesc->cellname()));
                return (false);
            }
            if (!strcmp(lname, XIC_NATIVE_LIBNAME))
                pc_name = 0;
        }

        CDc_gen cgen(mdesc);
        for (CDc *cdesc = cgen.c_first(); cdesc; cdesc = cgen.c_next()) {
            if (!cdesc->is_normal())
                continue;

            CDtx tx(cdesc);
            oaTransform tform(tx.tx, tx.ty, orient_from_tx(tx));

            if (out_virtuoso && pc_name) {
                oaScalarName pcLibName(out_ns, lname);
                oaScalarName pcCellName(out_ns, cname);
                oaScalarName pcViewName(out_ns, vname);
                oaInst *inst = oaScalarInst::create(block, pcLibName,
                    pcCellName, pcViewName, tform);
                save_obj_properties(cdesc, inst);
                continue;
            }

            oaScalarName cellName(out_ns, Tstring(cdesc->cellname()));
            oaScalarInst *inst;
            if (msdesc->isLibrary() && msdesc->isDevice()) {
                if (!out_devsused)
                    out_devsused = new SymTab(false, false);
                const char *nm = Tstring(msdesc->cellname());
                if (SymTab::get(out_devsused, nm) == ST_NIL)
                    out_devsused->add(nm, msdesc, false);

                inst = oaScalarInst::create(block, devLibName, cellName,
                    symViewName, tform);
            }
            else if (msdesc->symbolicRep(0)) {
                inst = oaScalarInst::create(block, libName, cellName,
                    symViewName, tform);
            }
            else {
                inst = oaScalarInst::create(block, libName, cellName,
                    viewName, tform);
            }
            save_obj_properties(cdesc, inst);

            // Add the instance terminals, add nets if needed.
            CDp_cnode *pn = (CDp_cnode*)cdesc->prpty(P_NODE);
            for ( ; pn; pn = pn->next()) {
                // Skip ground and unassigned nodes.
                if (pn->enode() < 1)
                    continue;

                bool isglobal;
                const char *nodename = CD()->ifNodeName(sdesc, pn->enode(),
                    &isglobal);
                if (!nodename || !*nodename) {
                    mmItoA(buf, pn->enode());
                    nodename = buf;
                }
                oaScalarName nodeName(out_ns, nodename);
                oaNet *net = oaNet::find(block, nodeName);
                if (!net)
                    net = oaNet::create(block, nodeName);
                net->setGlobal(isglobal);

                CDp_snode *psn = (CDp_snode*)msdesc->prpty(P_NODE);
                for ( ; psn; psn = psn->next()) {
                    if (psn->index() == pn->index())
                        break;
                }
                if (!psn)
                    continue;

                const char *tname = Tstring(psn->term_name());
                oaScalarName termName(out_ns, tname);;
                oaInstTerm *term = oaInstTerm::create(net, inst, termName);
                (void)term;
            }
        }
        delete [] lname;
        delete [] cname;
        delete [] vname;
    }

    // Add terminals.
    CDp_snode *pn = (CDp_snode*)sdesc->prpty(P_NODE);
    for ( ; pn; pn = pn->next()) {
        // Skip ground and unassigned nodes.
        if (pn->enode() < 1)
            continue;

        bool isglobal;
        const char *nodename = CD()->ifNodeName(sdesc, pn->enode(), &isglobal);
        if (!nodename || !*nodename) {
            mmItoA(buf, pn->enode());
            nodename = buf;
        }
        oaScalarName nodeName(out_ns, nodename);
        oaNet *net = oaNet::find(block, nodeName);
        if (!net)
            net = oaNet::create(block, nodeName);
        net->setGlobal(isglobal);

        const char *tname = Tstring(pn->term_name());
        oaScalarName termName(out_ns, tname);;
        oaTerm *term = oaTerm::create(net, termName);
        term->setPosition(pn->index());
        oaPin *pin = oaPin::create(term, tname);
        (void)pin;

    }
    return (true);
}


bool
oa_save::save_elec_geom(CDs *sdesc, const char *cellname, oaBlock *block,
    oaTech *tech)
{
    oaDesign *design = block->getDesign();
    oaScalarName libName, cellName, viewName;
    design->getLibName(libName);
    design->getCellName(cellName);
    design->getViewName(viewName);

    bool lwarn = false;
    bool wwarn = false;
    CDsLgen lgen(sdesc);
    CDl *ld;
    while ((ld = lgen.next()) != 0) {
        unsigned int layernum = ld->oaLayerNum();
        unsigned int purposenum = ld->oaPurposeNum();
        const char *layername = CDldb()->getOAlayerName(layernum);
        const char *purposename = CDldb()->getOApurposeName(purposenum);
        if (!purposename) {
            purposename = CDL_PRP_DRAWING;
            purposenum = oavPurposeNumberDrawing;
        }

        CDg gdesc;
        gdesc.init_gen(sdesc, ld);
        bool first = true;
        CDo *odesc;
        while ((odesc = gdesc.next()) != 0) {
            if (!odesc->is_normal())
                continue;

            if (first) {
                if (!oaPhysicalLayer::find(tech, layernum) &&
                        !oaPhysicalLayer::find(tech, layername))
                    oaPhysicalLayer::create(tech, layername, layernum);
                if (!oaPurpose::find(tech, purposename) &&
                        !oaPurpose::find(tech, purposenum))
                    oaPurpose::create(tech, purposename, purposenum);
                first = false;
            }

            if (odesc->type() == CDBOX) {
                oaBox box(odesc->oBB().left, odesc->oBB().bottom,
                    odesc->oBB().right, odesc->oBB().top);
                oaRect *rect = oaRect::create(block, layernum, purposenum,
                    box);
                save_obj_properties(odesc, rect);
            }
            else if (odesc->type() == CDPOLYGON) {
                oaPointArray pArray;
                int num = ((CDpo*)odesc)->numpts();
                const Point *pts = ((CDpo*)odesc)->points();
                num--;  // closure not included in OA
                for (int i = 0; i < num; i++)
                    pArray.append(oaPoint(pts[i].x, pts[i].y));

                oaPolygon *poly = oaPolygon::create(block, layernum,
                    purposenum, pArray);
                save_obj_properties(odesc, poly);
            }
            else if (odesc->type() == CDWIRE) {
                CDw *wd = (CDw*)odesc;
                // OA doesn't accept 0-width wires, so create a line.
                oaPointArray pArray;
                int num = wd->numpts();
                const Point *pts = wd->points();
                for (int i = 0; i < num; i++)
                    pArray.append(oaPoint(pts[i].x, pts[i].y));
                if (pArray.getNumElements() > 1) {

                    oaLine *line = oaLine::create(block, layernum, purposenum,
                        pArray);
                    save_obj_properties(odesc, line);

                    // Add wire to net, create net if not found.
                    if (wd->ldesc()->isWireActive()) {
                        CDp_node *pn = (CDp_node*)wd->prpty(P_NODE);
                        if (pn && pn->enode() >= 0) {
                            bool isglobal;
                            const char *nn = CD()->ifNodeName(sdesc,
                                pn->enode(), &isglobal);
                            oaScalarName nodeName(out_ns, nn);
                            oaNet *net = oaNet::find(block, nodeName);
                            if (!net)
                                net = oaNet::create(block, nodeName);
                            net->setGlobal(isglobal);
                            line->addToNet(net);
                        }
                    }
                }
                else {
                    // Deal with single vertex wire, OA can't handle.
                    if (!wwarn) {
                        sLstr lstr;
                        lstr.add("Warning, in electrical cell ");
                        lstr.add(cellname);
                        lstr.add(", one-vertex wire ignored.");
                        out_warnings = new stringlist(lstr.string_trim(),
                            out_warnings);
                        wwarn = true;
                    }
                }
            }
            else if (odesc->type() == CDLABEL) {
                CDla *la = (CDla*)odesc;
                int xform = la->xform() & ~TXTF_LIML;

                oaTextAlign align = align_from_xform(xform);

                if (xform & TXTF_45) {
                    // OA can't handle 45's
                    if (!lwarn) {
                        sLstr lstr;
                        lstr.add("Warning: In electrical cell ");
                        lstr.add(cellname);
                        lstr.add(
                        ", nonorthogonal text rotation set\nto orthogonal.");
                        out_warnings = new stringlist(lstr.string_trim(),
                            out_warnings);
                        lwarn = true;
                    }
                    xform &= ~TXTF_45;
                }
                oaOrient orient = orient_from_xform(xform);

                char *string = hyList::string(la->label(), HYcvAscii, true);
                int nlines = DSP()->LabelExtent(string, 0, 0);

                int lhei = (int)(la->height()/nlines/CDS_TEXT_SCALE);
                oaText *text = oaText::create(block, layernum, purposenum,
                    string, oaPoint(la->xpos(), la->ypos()), align, orient,
                    oacEuroStyleFont, lhei);
                save_obj_properties(odesc, text);

                // Save a property for this attribute (label shown in
                // top-level only, not in instances).
                if (la->no_inst_view())
                    oaStringProp::create(text, "XICP_NO_INST_VIEW", "t");
                // Likewise for the line number limiting.
                if (la->use_line_limit())
                    oaStringProp::create(text, "XICP_USE_LINE_LIMIT", "t");

                delete [] string;
            }
        }
    }
    return (true);
}


// Write a symbolic cell to OA, this is used for library devices and
// the symbolic representation of electrical cells.  If altcname is
// given, it will be taken as the cell name.  If chk is true, the cell
// is written only if it doesn't previously exist.
//
bool
oa_save::save_symb_cell(CDs *srep, CDs *sprnt, const char *libname,
    const char *altcname, bool chk, bool device)
{
    if (!srep || !libname)
        return (false);
    out_mode = Electrical;

    // The sprnt is the schematic cell for srep if not a device library
    // cell, 0 otherwise.
    if (!sprnt)
        sprnt = srep;

    const char *cellname = Tstring(sprnt->cellname());
    if (altcname && strcmp(cellname, altcname))
        cellname = altcname;

    oaDesign *design = open_design(libname, cellname, VTsymbolic, chk);
    if (!design)
        return (chk ? true : false);

    if (device)
        save_cell_properties(sprnt, design);

    oaBlock *block = oaBlock::create(design);

    // Add terminals.
    char buf[256];
    CDp_snode *pn = (CDp_snode*)sprnt->prpty(P_NODE);
    for ( ; pn; pn = pn->next()) {
        // Skip ground and unassigned nodes.
        if (pn->enode() < 1)
            continue;

        bool isglobal;
        const char *nodename = CD()->ifNodeName(sprnt, pn->enode(), &isglobal);
        if (!nodename || !*nodename) {
            mmItoA(buf, pn->enode());
            nodename = buf;
        }
        oaScalarName nodeName(out_ns, nodename);
        oaNet *net = oaNet::find(block, nodeName);
        if (!net)
            net = oaNet::create(block, nodeName);
        net->setGlobal(isglobal);

        const char *tname = Tstring(pn->term_name());
        oaScalarName termName(out_ns, tname);;
        oaTerm::create(net, termName);
    }

    oaScalarName libName(out_ns, libname);
    oaTech *tech = oaTech::find(libName);
    if (!tech) {
        if (oaTech::exists(libName))
            tech = oaTech::open(libName, 'r');
        else
            tech = oaTech::create(libName);
    }
    bool ret = save_symb_geom(srep, cellname, block, tech);
    if (ret)
        design->save();
    design->close();
    return (ret);
}


bool
oa_save::save_symb_geom(CDs *srep, const char *cellname, oaBlock *block,
    oaTech *tech)
{
    oaDesign *design = block->getDesign();
    oaScalarName libName, cellName, viewName;
    design->getLibName(libName);
    design->getCellName(cellName);
    design->getViewName(viewName);

    bool lwarn = false;
    CDg gdesc;
    CDsLgen lgen(srep);
    CDl *ld;
    while ((ld = lgen.next()) != 0) {
        unsigned int layernum = ld->oaLayerNum();
        unsigned int purposenum = ld->oaPurposeNum();
        const char *layername = CDldb()->getOAlayerName(layernum);
        const char *purposename = CDldb()->getOApurposeName(purposenum);
        if (!purposename) {
            purposename = CDL_PRP_DRAWING;
            purposenum = oavPurposeNumberDrawing;
        }

        gdesc.init_gen(srep, ld);
        CDo *odesc;
        bool first = true;
        while ((odesc = gdesc.next()) != 0) {
            if (!odesc->is_normal())
                continue;
            if (first) {
                if (!oaPhysicalLayer::find(tech, layernum) &&
                        !oaPhysicalLayer::find(tech, layername))
                    oaPhysicalLayer::create(tech, layername, layernum);
                if (!oaPurpose::find(tech, purposename) &&
                        !oaPurpose::find(tech, purposenum))
                    oaPurpose::create(tech, purposename, purposenum);
                first = false;
            }

            if (odesc->type() == CDBOX) {
                oaBox box(odesc->oBB().left, odesc->oBB().bottom,
                    odesc->oBB().right, odesc->oBB().top);
                oaRect::create(block, layernum, purposenum, box);
            }
            else if (odesc->type() == CDPOLYGON) {
                oaPointArray pArray;
                int num = ((CDpo*)odesc)->numpts();
                const Point *pts = ((CDpo*)odesc)->points();
                num--;  // closure not included in OA
                for (int i = 0; i < num; i++)
                    pArray.append(oaPoint(pts[i].x, pts[i].y));
                oaPolygon::create(block, layernum, purposenum, pArray);
            }
            else if (odesc->type() == CDWIRE) {
                CDw *wd = (CDw*)odesc;
                // OA doesn't accept 0-width wires, so create a line.
                oaPointArray pArray;
                int num = wd->numpts();
                const Point *pts = wd->points();
                for (int i = 0; i < num; i++)
                    pArray.append(oaPoint(pts[i].x, pts[i].y));
                if (pArray.getNumElements() > 1)
                    oaLine::create(block, layernum, purposenum, pArray);
                // Just ignore single-vertex wires here.
            }
            else if (odesc->type() == CDLABEL) {
                CDla *la = (CDla*)odesc;
                int xform = la->xform() & ~TXTF_LIML;

                oaTextAlign align = align_from_xform(xform);

                if (xform & TXTF_45) {
                    // OA can't handle 45's
                    if (!lwarn) {
                        sLstr lstr;
                        lstr.add("Warning: In symbolic cell ");
                        lstr.add(cellname);
                        lstr.add(
                        ", nonorthogonal text rotation set\nto orthogonal.");
                        out_warnings = new stringlist(lstr.string_trim(),
                            out_warnings);
                        lwarn = true;
                    }
                    xform &= ~TXTF_45;
                }
                oaOrient orient = orient_from_xform(xform);

                char *string = hyList::string(la->label(), HYcvAscii, true);
                int nlines = DSP()->LabelExtent(string, 0, 0);

                int lhei = (int)(la->height()/nlines/CDS_TEXT_SCALE);
                oaText *text = oaText::create(block, layernum, purposenum,
                    string, oaPoint(la->xpos(), la->ypos()), align, orient,
                    oacEuroStyleFont, lhei);

                // Save a property for this attribute (label shown in
                // top-level only, not in instances).
                if (la->no_inst_view())
                    oaStringProp::create(text, "XICP_NO_INST_VIEW", "t");
                // Likewise for the line number limiting.
                if (la->use_line_limit())
                    oaStringProp::create(text, "XICP_USE_LINE_LIMIT", "t");

                delete [] string;
            }
        }
    }
    return (true);
}


// Open or create a new design, and return a pointer to it.  If chk is
// set, only create and return a design if it does not already exist.
//
oaDesign *
oa_save::open_design(const char *libname, const char *cellname, ViewType vt,
    bool chk)
{
    if (!libname || !cellname)
        return (0);
    oaScalarName libName(out_ns, libname);

    oaLib *lib = oaLib::find(libName);
    if (!lib) {
        // Library does not currently exist in lib.defs.
        // "can't happen"
        return (0);
    }

    oaScalarName cellName(out_ns, cellname);
    if (vt == VTphysical) {
        const char *vname = CDvdb()->getVariable(VA_OaDefLayoutView);
        if (!vname)
            vname = OA_DEF_LAYOUT;
        oaScalarName viewName(out_ns, vname);
        oaViewType *viewType = oaViewType::get(oacMaskLayout);
        if (!chk || !oaDesign::find(libName, cellName, viewName))
            return (oaDesign::open(libName, cellName, viewName, viewType, 'w'));
    }
    else if (vt == VTelectrical) {
        const char *vname = CDvdb()->getVariable(VA_OaDefSchematicView);
        if (!vname)
            vname = OA_DEF_SCHEMATIC;
        oaScalarName viewName(out_ns, vname);
        oaViewType *viewType = oaViewType::get(oacSchematic);
        if (!chk || !oaDesign::find(libName, cellName, viewName))
            return (oaDesign::open(libName, cellName, viewName, viewType, 'w'));
    }
    else if (vt == VTsymbolic) {
        const char *vname = CDvdb()->getVariable(VA_OaDefSymbolView);
        if (!vname)
            vname = OA_DEF_SYMBOL;
        oaScalarName viewName(out_ns, vname);
        oaViewType *viewType = oaViewType::get(oacSchematicSymbol);
        if (!chk || !oaDesign::find(libName, cellName, viewName))
            return (oaDesign::open(libName, cellName, viewName, viewType, 'w'));
    }
    return (0);
}


// Save properties.  Xic are number/string pairs, where the number
// need not be unique.  All are saved as oaStringProp, with the name
// formatted so as to be unique:  "xic_num[_index]".
//
void
oa_save::save_cell_properties(CDs *sdesc, oaObject *object)
{
    if (!sdesc || !object)
        return;

    if (out_virtuoso) {
        CDp_name *pn = (CDp_name*)sdesc->prpty(P_NAME);
        if (pn) {
            // Add the instNamePrefix property to devices.

            CDelecCellType tp = sdesc->elecCellType();
            switch (tp) {
            case CDelecDev:
                oaStringProp::create(object, "instNamePrefix",
                    Tstring(pn->name_string()));
                break;
            default:
                break;
            }
        }
        return;
    }

    // Add a property to mark cell as coming from Xic.
    oaStringProp::create(object, OA_XICP_PFX"version", XM()->VersionString());

    CDp *prpty_list = sdesc->prptyList();
    if (!prpty_list)
        return;
    SymTab *tab = new SymTab(false, false);
    char buf[64];
    for (CDp *p = prpty_list; p; p = p->next_prp()) {
        char *string;
        if (p->string(&string)) {
            int num = p->value();
            int ix = 0;
            SymTabEnt *h = SymTab::get_ent(tab, num);
            if (!h)
                tab->add(num, (void*)(long)1, false);
            else {
                ix = (long)h->stData;
                h->stData = (void*)(long)(ix+1);
            }
            if (ix == 0)
                sprintf(buf, "%s%d", OA_XICP_PFX, num);
            else
                sprintf(buf, "%s%d_%d", OA_XICP_PFX, num, ix);

            if (out_mode == Electrical) {
                if (num == P_SYMBLC) {
                    // We don't need the rep, just keep the activity
                    // flag.
                    char *s = string;
                    while (isspace(*s))
                        s++;
                    if (s != string)
                        *string = *s;
                    string[1] = 0;
                }
            }

            oaStringProp::create(object, buf, string);
            delete [] string;
        }
    }
    delete tab;
}


void
oa_save::save_obj_properties(const CDo *odesc, oaObject *object)
{
    if (!odesc || !object)
        return;
    CDp *prpty_list = odesc->prpty_list();
    if (!prpty_list)
        return;

    SymTab *tab = new SymTab(false, false);
    char buf[64];
    for (CDp *p = prpty_list; p; p = p->next_prp()) {
        if (prpty_reserved(p->value()) || prpty_pseudo(p->value()))
            continue;

        /*** Do we ever need this?
        // Map the Ciranove stretch handle and abutment properties.
        //
        if (p->value() == XICP_GRIP) {
            oaStringProp::create(object, "pycStretch", p->string());
            continue;
        }
        if (p->value() == XICP_AB_CLASS) {
            oaStringProp::create(object, "pycAbutClass", p->string());
            continue;
        }
        if (p->value() == XICP_AB_RULES) {
            oaStringProp::create(object, "pycAbutRules", p->string());
            continue;
        }
        if (p->value() == XICP_AB_DIRECTS) {
            oaStringProp::create(object, "pycAbutDirections", p->string());
            continue;
        }
        if (p->value() == XICP_AB_SHAPENAME) {
            oaStringProp::create(object, "pycShapeName", p->string());
            continue;
        }
        if (p->value() == XICP_AB_PINSIZE) {
            oaStringProp::create(object, "pycPinSize", p->string());
            continue;
        }
        ***/

        if (!out_virtuoso) {
            char *string;
            if (p->string(&string)) {
                int num = p->value();
                int ix = 0;
                SymTabEnt *h = SymTab::get_ent(tab, num);
                if (!h)
                    tab->add(num, (void*)(long)1, false);
                else {
                    ix = (long)h->stData;
                    h->stData = (void*)(long)(ix+1);
                }
                if (ix == 0)
                    sprintf(buf, "%s%d", OA_XICP_PFX, num);
                else
                    sprintf(buf, "%s%d_%d", OA_XICP_PFX, num, ix);

                oaStringProp::create(object, buf, string);
                delete [] string;
            }
        }
    }

    if (!out_virtuoso) {
        // Add the NODRC property if necessary
        if (out_mode == Physical && odesc->type() != CDINSTANCE &&
                odesc->type() != CDLABEL && odesc->has_flag(CDnoDRC)) {
            sprintf(buf, "%s%d", OA_XICP_PFX, XICP_NODRC);
            oaStringProp::create(object, buf, "NODRC");
        }
    }

    delete tab;
}


oaOrient
oa_save::orient_from_tx(const CDtx &tx)
{
    oaOrient orient;
    if (tx.ax > 0) {
        if (tx.refly)
            orient = oacMX;
        else
            orient = oacR0;
    }
    else if (tx.ax < 0) {
        if (tx.refly)
            orient = oacMY;
        else
            orient = oacR180;
    }
    else if (tx.ay > 0) {
        if (tx.refly)
            orient = oacMXR90;
        else
            orient = oacR90;
    }
    else {
        if (tx.refly)
            orient = oacMYR90;
        else
            orient = oacR270;
    }
    return (orient);
}


oaOrient
oa_save::orient_from_xform(int xform)
{
    oaOrient orient;
    if (xform & 0x1) {
        if (xform & 0x2) {
            if (xform & TXTF_MY) {
                if (xform & TXTF_MX)
                    orient = oacR90;
                else
                    orient = oacMXR90;
            }
            else {
                if (xform & TXTF_MX)
                    orient = oacMYR90;
                else
                    orient = oacR270;
            }
        }
        else {
            if (xform & TXTF_MY) {
                if (xform & TXTF_MX)
                    orient = oacR270;
                else
                    orient = oacMYR90;
            }
            else {
                if (xform & TXTF_MX)
                    orient = oacMXR90;
                else
                    orient = oacR90;
            }
        }
    }
    else {
        if (xform & 0x2) {
            if (xform & TXTF_MY) {
                if (xform & TXTF_MX)
                    orient = oacR0;
                else
                    orient = oacMY;
            }
            else {
                if (xform & TXTF_MX)
                    orient = oacMX;
                else
                    orient = oacR180;
            }
        }
        else {
            if (xform & TXTF_MY) {
                if (xform & TXTF_MX)
                    orient = oacR180;
                else
                    orient = oacMX;
            }
            else {
                if (xform & TXTF_MX)
                    orient = oacMY;
                else
                    orient = oacR0;
            }
        }
    }
    return (orient);
}


oaTextAlign
oa_save::align_from_xform(int xform)
{
    oaTextAlign align = oacLowerLeftTextAlign;
    if (xform & TXTF_HJR) {
        if (xform & TXTF_VJT)
            align = oacUpperRightTextAlign;
        else if (xform & TXTF_VJC)
            align = oacCenterRightTextAlign;
        else 
            align = oacLowerRightTextAlign;
    }
    else if (xform & TXTF_HJC) {
        if (xform & TXTF_VJT)
            align = oacUpperCenterTextAlign;
        else if (xform & TXTF_VJC)
            align = oacCenterCenterTextAlign;
        else 
            align = oacLowerCenterTextAlign;
    }
    else {
        if (xform & TXTF_VJT)
            align = oacUpperLeftTextAlign;
        else if (xform & TXTF_VJC)
            align = oacCenterLeftTextAlign;
        else 
            align = oacLowerLeftTextAlign;
    }
    return (align);
}
// End of oa_save functions.


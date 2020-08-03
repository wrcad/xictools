
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
#include "editif.h"
#include "scedif.h"
#include "cvrt.h"
#include "pcell.h"
#include "dsp.h"
#include "dsp_inlines.h"
#include "dsp_tkif.h"
#include "fio.h"
#include "fio_cvt_base.h"
#include "fio_library.h"
#include "cd_hypertext.h"
#include "cd_types.h"
#include "cd_objects.h"
#include "cd_strmdata.h"
#include "cd_propnum.h"
#include "cd_celldb.h"
#include "cd_terminal.h"
#include "cd_lgen.h"
#include "tech_cds_out.h"
#include "promptline.h"
#include "errorlog.h"
#include "tech.h"
#include "oa_if.h"
#include "pcell_params.h"
#include "oa.h"
#include "oa_prop.h"
#include "oa_via.h"
#include "oa_net.h"
#include "oa_nametab.h"
#include "oa_errlog.h"
#include "miscutil/texttf.h"


// Schematics from Virtuoso are very different from Xic.  For
// starters, we need to scale all coordintes by 1000 when reading
// Virtuoso schematics, since the electrically active grid is 1000
// units in Xic, but unit in Virtuoso.  Schematics originating from
// Xic don't require scaling.
//
#define XIC_ELEC_SCALE  1
#define CDS_ELEC_SCALE  100


namespace {
    // Instantiate the name table, which is used to record names of
    // cells read into Xic from OA, and aliasing.  This is persistent
    // between calls to the interface, to avoid database cell merging
    // queries due to common subcells.  The name table context can be
    // cleared when appropriate through a separate operation.
    //
    cOAnameTab NameTab;

    // Print "libname/cellname/viewname" to lstr.
    //
    void lcv(const oaDesign *design, sLstr &lstr)
    {
        oaString lname, cname, vname;
        design->getLibName(oaNativeNS(), lname);
        design->getCellName(oaNativeNS(), cname);
        design->getViewName(oaNativeNS(), vname);
        lstr.add((const char*)lname);
        lstr.add_c('/');
        lstr.add((const char*)cname);
        lstr.add_c('/');
        lstr.add((const char*)vname);
    }
}

class oa_in
{
public:
    oa_in(int);
    ~oa_in();

    enum ReadMode { ReadAll, ReadPhys, ReadElec };

    stringlist *undefPhys()     { return (in_undef_phys); }
    stringlist *undefElec()     { return (in_undef_elec); }
    stringlist *topPhys()       { return (in_top_phys); }
    stringlist *topElec()       { return (in_top_elec); }
    stringlist *warnings()      { return (in_warnings); }

    OItype loadLibrary(const char*);
    OItype loadCell(const char*, const char*, const char*, int,
        PCellParam**, const char**);

private:
    // Argument to newCell.
    enum ncType { ncError, ncOK, ncSkip };

    OItype loadCellRec(const oaScalarName&, const oaScalarName&,
        const char*, oaInt4, const char**);
    OItype loadCellRec(oaLib*, oaCell*, oaView*, oaInt4, const char**);
    bool checkMasterXicProps(CDs*);
    bool markReferences();
    oaDesign *openDesign(const oaScalarName&, const oaScalarName&,
        const oaScalarName&, const oaViewType*);
    void getSuperMasterParams(oaDesign*);
    oaDesign *handleSuperMaster(oaDesign*, char**);
    OItype loadPhysicalDesign(const oaDesign*, const char*, CDs**, oaInt4);
    OItype loadElectricalDesign(const oaDesign*, const oaDesign*,
        const char*, CDs**, oaInt4);
    OItype loadVia(const oaViaHeader*, oaUInt4);
    char *getViaName(const oaViaHeader*);
    bool writeOaStdViaCutStruct(oaStdViaHeader*, bool*);
    bool getStdViaCutName(oaStdViaHeader*, oaString&);
    bool writeOaStdViaCutAref(oaStdViaHeader*, const char*);
    OItype loadMaster(const oaInstHeader*, oaInt4);
    CDs *newCell(const char*, ncType*, bool = false);
    CDcellName checkSubMaster(CDcellName, CDp*);

    bool readPhysicalDesign(const oaDesign*, const oaString&, CDs**);
    bool readElectricalDesign(const oaDesign*, const oaDesign*,
        const oaString&, CDs**);
    bool readInstances(const oaBlock*, CDs*);
    bool readVias(const oaBlock*, CDs*);
    bool readGeometry(const oaBlock*, CDs*);
    bool readTerms(const oaBlock*, CDs*sdesc, bool);
    bool readPhysicalProperties(const oaDesign*, CDs*);
    bool readElectricalProperties(const oaDesign*, const oaDesign*, CDs*);
    bool readOaLPPHeader(oaLPPHeader*, CDs*);
    bool readOaDot(oaDot*, CDs*, CDl*);
    bool readOaEllipse(oaEllipse*, CDs*, CDl*);
    bool readOaArc(oaArc*, CDs*, CDl*);
    bool readOaDonut(oaDonut*, CDs*, CDl*);
    bool readOaLine(oaLine*, CDs*, CDl*);
    bool readOaPath(oaPath*, CDs*, CDl*);
    bool readOaPathSeg(oaPathSeg*, CDs*, CDl*);
    bool readOaPolygon(oaPolygon*, CDs*, CDl*);
    bool readOaRect(oaRect*, CDs*, CDl*);
    bool readOaText(oaText*, CDs*, CDl*);
    bool readOaAttrDisplay(oaAttrDisplay*, CDs*, CDl*);
    bool readOaPropDisplay(oaPropDisplay*, CDs*, CDl*);
    bool readOaTextOverride(oaTextOverride*, CDs*, CDl*);
    bool readOaEvalText(oaEvalText*, CDs*, CDl*);
    bool readOaInstHeader(oaInstHeader*, CDs*, const char*);
    bool readOaScalarInst(oaScalarInst*, const char*, const oaInstHeader*,
        CDs*);
    bool readOaVectorInstBit(oaVectorInstBit*, const char*, const oaInstHeader*,
        CDs*);
    bool readOaVectorInst(oaVectorInst*, const char*, const oaInstHeader*,
        CDs*);
    bool readOaArrayInst(oaArrayInst*, const char*, const oaInstHeader*, CDs*);
    bool readOaViaHeader(oaViaHeader*, CDs*, const char*);
    bool readOaVia(oaVia*, const char*, CDs*);
    CDl *mapLayer(oaScalarName&, unsigned int, unsigned int, DisplayMode);
    CDp *readProperties(const oaObject*);

    oaNativeNS in_ns;
    SymTab *in_skip_tab;
    SymTab *in_via_tab;
    stringlist *in_undef_phys;
    stringlist *in_undef_elec;
    stringlist *in_top_phys;
    stringlist *in_top_elec;
    stringlist *in_warnings;
    PCellParam *in_pc_params;
    SymTab *in_submaster_tab;
    const char *in_def_layout;
    const char *in_def_schematic;
    const char *in_def_symbol;
    const char *in_def_dev_prop;
    DisplayMode in_mode;
    int in_elec_scale;
    int in_sub_level;
    ReadMode in_read_mode;
    bool in_skip_supermaster;
    bool in_from_xic;
    int in_api_major;

    static bool in_mark_references;
};

bool oa_in::in_mark_references = false;


// Load all cells in libname into the main database.  We do NOT change
// the current cell.
//
bool
cOA::load_library(const char *libname)
{
    cOAerrLogWrap logger("load_library", OA_ERRLOG, OA_DBGLOG);

    if (!initialize()) {
        Errs()->add_error("OpenAccess initialization failed.");
        return (false);
    }
    oa_in in(oa_api_major);
    OItype oiret = in.loadLibrary(libname);
    if (oiret == OIerror)
        return (false);
    if (oiret == OIaborted)
        Log()->WarningLog(mh::OpenAccess, "Load aborted at user request.");

    sLstr lstr;
    if (in.undefPhys()) {
        lstr.add("Physical unresolved references:\n");
        for (stringlist *s = in.undefPhys(); s; s = s->next) {
            lstr.add(s->string);
            lstr.add_c('\n');
        }
    }
    if (in.undefElec()) {
        lstr.add("Electrical unresolved references:\n");
        for (stringlist *s = in.undefElec(); s; s = s->next) {
            lstr.add(s->string);
            lstr.add_c('\n');
        }
    }
    if (in.warnings() || lstr.string()) {
        sLstr wstr;
        for (stringlist *s = in.warnings(); s; s = s->next) {
            wstr.add(s->string);
            wstr.add_c('\n');
        }
        if (lstr.string()) {
            wstr.add(lstr.string());
            wstr.add_c('\n');
        }
        Log()->WarningLog(mh::OpenAccess, wstr.string());
    }
    return (true);
}


// Load the named cell and its hierarchy from libname into the main
// database.  If setcur, make the new cell the current cell.  If the
// cell is a PCell, on the first call no cells are created and the
// default parameter set is returned.  This can be modified and repeat
// calls made, which creates the sub-masters.  On these calls, the
// sub-master name is returned in new_cell_name.  If paramp is null, PCells
// will not be processed at all.  The user should free the paramp list
// when done.
//
bool
cOA::load_cell(const char *libname, const char *cellname, const char *viewname,
    int depth, bool setcur, const char **new_cell_name, PCellParam **paramp)
{
    if (new_cell_name)
        *new_cell_name = 0;

    if (!initialize()) {
        Errs()->add_error("OpenAccess initialization failed.");
        return (false);
    }

    const char *ncell = 0;
    {
        cOAerrLogWrap logger("load_cell", OA_ERRLOG, OA_DBGLOG);

        oa_in in(oa_api_major);
        OItype oiret = in.loadCell(libname, cellname, viewname, depth, paramp,
            &ncell);
        OAerrLog.set_return(oiret);

        if (oiret == OIerror) {
            OAerrLog.add_err(IFLOG_INFO, "OpenAccess read failed.");
            OAerrLog.add_err(IFLOG_INFO, Errs()->get_error());
            OAerrLog.set_show_log(true);
        }
        else if (oiret == OIaborted) {
            OAerrLog.add_err(IFLOG_INFO, 
                "OpenAccess read aborted at user request.");
        }
        else {
            sLstr lstr;
            if (in.undefPhys()) {
                lstr.add("Physical unresolved references:\n");
                for (stringlist *s = in.undefPhys(); s; s = s->next) {
                    lstr.add("  ");
                    lstr.add(s->string);
                    lstr.add_c('\n');
                }
            }
            if (in.undefElec()) {
                lstr.add("Electrical unresolved references:\n");
                for (stringlist *s = in.undefElec(); s; s = s->next) {
                    lstr.add("  ");
                    lstr.add(s->string);
                    lstr.add_c('\n');
                }
            }
            if (in.warnings() || lstr.string()) {
                sLstr wstr;
                for (stringlist *s = in.warnings(); s; s = s->next) {
                    wstr.add(s->string);
                    wstr.add_c('\n');
                }
                if (lstr.string()) {
                    wstr.add(lstr.string());
                    wstr.add_c('\n');
                }
                OAerrLog.add_err(IFLOG_INFO, wstr.string());
                OAerrLog.set_show_log(true);
            }
        }

        sLstr lstr;
        lstr.add("Libraries referenced:\n");
        stringlist *sl0 = NameTab.listLibNames();
        for (stringlist *sl = sl0; sl; sl = sl->next) {
            lstr.add("  ");
            lstr.add(sl->string);
            lstr.add_c('\n');
        }
        stringlist::destroy(sl0);
        OAerrLog.add_err(IFLOG_INFO, lstr.string());
    }
    if (new_cell_name)
        *new_cell_name = ncell;

    // If we have created any P_NAME properties, the indices are all 0. 
    // This will assign proper values, which is essential for linking
    // labels when the cell is saved to a file.
    //
    CDs *sd = CDcdb()->findCell(ncell, Electrical);
    if (sd)
        ScedIf()->connectAll(false, sd);

    if (setcur)
        XM()->Load(DSP()->MainWdesc(), ncell, 0);

    return (true);
}


// Open cellname, if found in the open OA libraries.  Return values
// are:
//
//   OIok       new cell opened successfully
//   OIerror    cell found, error on opening
//   OInew      cell was not found.
//
OItype
cOA::open_lib_cell(const char *cellname, CDcbin *cbin)
{
    if (!cellname)
        return (OInew);
    stringlist *liblist;
    if (!list_libraries(&liblist))
        return (OIerror);

    cOAerrLogWrap logger("open_lib_cell", OA_ERRLOG, OA_DBGLOG);

    bool ret = false;
    bool found = false;
    for (stringlist *s = liblist; s; s = s->next) {
        bool isopen;
        if (!is_lib_open(s->string, &isopen)) {
            stringlist::destroy(liblist);
            return (OIerror);
        }
        if (!isopen)
            continue;
        bool in_lib;
        if (!is_cell_in_lib(s->string, cellname, &in_lib)) {
            stringlist::destroy(liblist);
            return (OIerror);
        }
        if (in_lib) {
            ret = load_cell(s->string, cellname, 0, CDMAXCALLDEPTH, false);
            found = true;
            break;
        }
    }
    stringlist::destroy(liblist);
    if (ret) {
        CDcbin ctmp(CD()->CellNameTableAdd(cellname));

        // Set Immutable, Library flags.
        ctmp.setLibrary(true);
        ctmp.setImmutable(true);
        *cbin = ctmp;
        return (OIok);
    }
    if (found)
        return (OIerror);
    return (OInew);
}


// The name table, which records which cells have been read into Xic,
// is persistent between calls to the interface functions above.  This
// avoids the "merge control" pop-up which appears if a common subcell
// was previously read and is already in memory.  This function clears
// the table, and should be called if the cells loaded into Xic from
// OA have been modified or cleared.
//
void
cOA::clear_name_table()
{
    NameTab.clearCnameTab();
}
// End of cOA functions.


oa_in::oa_in(int api_major)
{
    in_skip_tab = 0;
    in_via_tab = 0;
    in_undef_phys = 0;
    in_undef_elec = 0;
    in_top_phys = 0;
    in_top_elec = 0;
    in_warnings = 0;
    in_pc_params = 0;
    in_submaster_tab = 0;

    const char *vn = CDvdb()->getVariable(VA_OaDefLayoutView);
    if (!vn)
        vn = OA_DEF_LAYOUT;
    in_def_layout = lstring::copy(vn);

    vn = CDvdb()->getVariable(VA_OaDefSchematicView);
    if (!vn)
        vn = OA_DEF_SCHEMATIC;
    in_def_schematic = lstring::copy(vn);

    vn = CDvdb()->getVariable(VA_OaDefSymbolView);
    if (!vn)
        vn = OA_DEF_SYMBOL;
    in_def_symbol = lstring::copy(vn);

    vn = CDvdb()->getVariable(VA_OaDefDevPropView);
    if (!vn)
        vn = OA_DEF_DEV_PROP;
    in_def_dev_prop = lstring::copy(vn);

    in_mode = Physical;
    in_elec_scale = XIC_ELEC_SCALE;
    in_sub_level = 0;
    in_read_mode = ReadAll;
    const char *s = CDvdb()->getVariable(VA_OaUseOnly);
    if (s && ((s[0] == '1' && s[1] == 0) || s[0] == 'p' || s[0] == 'P'))
        in_read_mode = ReadPhys;
    else if (s && ((s[0] == '2' && s[1] == 0) || s[0] == 'e' || s[0] == 'E'))
        in_read_mode = ReadElec;
    in_skip_supermaster = false;
    in_from_xic = false;
    in_api_major = api_major;
}


oa_in::~oa_in()
{
    delete in_skip_tab;

    stringlist::destroy(in_undef_phys);
    stringlist::destroy(in_undef_elec);
    stringlist::destroy(in_top_phys);
    stringlist::destroy(in_top_elec);
    stringlist::destroy(in_warnings);

    if (in_via_tab) {
        SymTabGen gen(in_via_tab, true);
        SymTabEnt *ent;
        while ((ent = gen.next()) != 0) {
            ViaDesc *vd = (ViaDesc*)ent->stData;
            delete vd;
            delete ent;
        }
        delete in_via_tab;
    }
    PCellParam::destroy(in_pc_params);
    delete [] in_def_layout;
    delete [] in_def_schematic;
    delete [] in_def_symbol;
    delete [] in_def_dev_prop;
}


// Public method to load an entire OA library.
//
OItype
oa_in::loadLibrary(const char *libname)
{
    if (!libname || !*libname) {
        Errs()->add_error("Null or empty library name encountered.");
        return (OIerror);
    }

    oaScalarName libName(in_ns, libname);
    oaLib *lib = oaLib::find(libName);
    if (!lib) {
        Errs()->add_error("Library %s was not found in lib.defs.", libname);
        return (OIerror);
    }

    // We don't want to bring in super-masters as top-level cells.
    in_skip_supermaster = true;

    // Enable merge control.
    sMCenable mc_enable;

    OItype oiret = OIok;
    try {

        lib->getAccess(oacReadLibAccess);
        dspPkgIf()->SetWorking(true);

        CD()->SetDeferInst(true);
        bool lpc = CD()->EnableLabelPatchCache(true);
        oaIter<oaCell> cIter(lib->getCells());
        while (oaCell *cell = cIter.getNext()) {

            if (dspPkgIf()->CheckForInterrupt() &&
                    XM()->ConfirmAbort("Interrupt received, abort load? ")) {
                oiret = OIaborted;
                break;
            }

            oaScalarName cellName;
            cell->getName(cellName);
            oaString cellname;
            cellName.get(in_ns, cellname);

            oiret = loadCellRec(lib, cell, 0, 0, 0);
            if (oiret == OIerror) {
                Errs()->add_error("Failed to load cell %s from library %s.",
                    (const char*)cellname, libname);
                break;
            }
            if (oiret == OIaborted)
                break;
        }
        CD()->EnableLabelPatchCache(lpc);
        CD()->SetDeferInst(false);
        if (!markReferences())
            oiret = OIerror;

        lib->releaseAccess();
        lib = 0;
    }
    catch (oaCompatibilityError &ex) {
        cOA::handleFBCError(ex);
        oiret = OIerror;
    }
    catch (oaException &excp) {
        Errs()->add_error((const char*)excp.getMsg());
        oiret = OIerror;
    }
    dspPkgIf()->SetWorking(false);

    if (lib) {
        try {
            lib->releaseAccess();
        }
        catch (...) {
            ;
        }
    }
    return (oiret);
}


// Public method to load a cell or cell hierarchy from OA.  If the
// cell is a PCell, on the first call no cells are created and the
// default parameter set is returned.  This can be modified and repeat
// calls made, which creates the sub-masters.  On these calls, the
// sub-master name is returned in new_cell_name.  If paramp is null,
// PCells will not be processed at all.  The user should free the
// paramp list when done.
//
OItype
oa_in::loadCell(const char *libname, const char *cellname,
    const char *viewname, int depth, PCellParam **paramp,
    const char **new_cell_name)
{
    if (new_cell_name)
        *new_cell_name = 0;
    if (!libname || !*libname) {
        Errs()->add_error("Null or empty library name encountered.");
        return (OIerror);
    }
    if (!cellname || !*cellname) {
        Errs()->add_error("Null or empty cell name encountered.");
        return (OIerror);
    }

    if (paramp)
        in_pc_params = *paramp;
    else
        in_skip_supermaster = true;

    // Enable merge control.
    sMCenable mc_enable;

    dspPkgIf()->SetWorking(true);
    OItype oiret = OIok;
    try {
        oaScalarName libName(in_ns, libname);
        oaScalarName cellName(in_ns, cellname);

        CD()->SetDeferInst(true);
        bool lpc = CD()->EnableLabelPatchCache(true);
        oiret = loadCellRec(libName, cellName, viewname, depth, new_cell_name);
        CD()->EnableLabelPatchCache(lpc);
        CD()->SetDeferInst(false);
        if (depth > 0 && !markReferences())
             oiret = OIerror;
        if (paramp) {
            *paramp = in_pc_params;
            in_pc_params = 0;
        }
    }
    catch (oaCompatibilityError &ex) {
        cOA::handleFBCError(ex);
        oiret = OIerror;

    }
    catch (oaException &excp) {
        Errs()->add_error((const char*)excp.getMsg());
        oiret = OIerror;
    }
    dspPkgIf()->SetWorking(false);

    if (oiret == OIerror) {
        Errs()->add_error(
            "Failed to load cell %s from library %s to depth %d.",
            (const char*)cellname, libname, depth);
    }
    return (oiret);
}


// Recursive call to load a cell.  If viewName is null, load all parts
// (physical and electrical) of the cell, otherwise load only the part
// corresponding to the view.  The viewname, if given, must name a
// layout, schematic, or schematicSymbol view.
//
OItype
oa_in::loadCellRec(const oaScalarName &libName, const oaScalarName &cellName,
    const char *viewname, oaInt4 depth, const char **new_cell_name)
{
    if (new_cell_name)
        *new_cell_name = 0;
    oaLib *lib = oaLib::find(libName);
    if (!lib) {
        oaString libname;
        libName.get(in_ns, libname);
        Errs()->add_error("Library %s was not found in lib.defs.",
            (const char*)libname);
        return (OIerror);
    }

    CDcellName cname = NameTab.cellNameAlias(libName, cellName, in_from_xic);

    OItype oiret = OIok;
    if (viewname) {
        // Given a view.  The lib/cell/view must exist, and must be a
        // known view type.

        oaScalarName viewName(in_ns, viewname);
        lib->getAccess(oacReadLibAccess);
        oaCellView *cv = oaCellView::find(lib, cellName, viewName);
        if (!cv) {
            oaString libname, cellname;
            libName.get(in_ns, libname);
            cellName.get(in_ns, cellname);
            Errs()->add_error("Lib/cel/view %s/%s/%s not found.",
                (const char*)libname, (const char*)cellname, viewname);
            lib->releaseAccess();
            return (OIerror);
        }

        oaCell *cell = cv->getCell();
        oaView *view = cv->getView();

        if (view->getViewType() == oaViewType::get(oacMaskLayout)) {
            unsigned long f = NameTab.findCname(cname);
            if (f & OAL_READP) {
                lib->releaseAccess();
                if (new_cell_name)
                    *new_cell_name = Tstring(cname);
                return (OIok);
            }
        }
        else if (view->getViewType() == oaViewType::get(oacSchematic) ||
                view->getViewType() == oaViewType::get(oacSchematicSymbol)) {
            unsigned long f = NameTab.findCname(cname);
            if (f & (OAL_READE | OAL_READS)) {
                lib->releaseAccess();
                if (new_cell_name)
                    *new_cell_name = Tstring(cname);
                return (OIok);
            }
        }
        else {
            oaString viewtype;
            view->getViewType()->getName(viewtype);
            Errs()->add_error("Unknown view name %s with type %s.",
                viewname, (const char*)viewtype);
            lib->releaseAccess();
            return (OIerror);
        }
        oiret = loadCellRec(lib, cell, view, depth, new_cell_name);
        lib->releaseAccess();
    }
    else {
        // Check if the cell has already been translated.  Here it is
        // assumed that if any part of the cell has been translated,
        // we are through translating this cell.

        unsigned long f = NameTab.findCname(cname);
        if (f & (OAL_READP | OAL_READE | OAL_READS)) {
            if (new_cell_name)
                *new_cell_name = Tstring(cname);
            return (OIok);
        }

        if (dspPkgIf()->CheckForInterrupt()) {
            if (XM()->ConfirmAbort("Interrupt received, abort load? "))
                return (OIaborted);
        }

        lib->getAccess(oacReadLibAccess);

        oaCell *cell = oaCell::find(lib, cellName);
        // If the cell is not found, there should be a warning at the top
        // level, however unresolved instances may be ok.
        if (!cell) {
            NameTab.updateCname(cname, OAL_NOOA);
            lib->releaseAccess();
            if (new_cell_name)
                *new_cell_name = Tstring(cname);
            return (OIok);
        }
        oiret = loadCellRec(lib, cell, 0, depth, new_cell_name);
        lib->releaseAccess();
    }
    return (oiret);
}


// Open a cell recursively to depth.  The view can be 0, or a pointer
// to a view.  The name of the Xic cell opened (if any) is returned
// in new_cell_name.
//
OItype
oa_in::loadCellRec(oaLib *lib, oaCell *cell, oaView *view, oaInt4 depth,
    const char **new_cell_name)
{
    if (new_cell_name)
        *new_cell_name = 0;
    if (depth < 0)
        return (OIok);

    oaScalarName libName;
    lib->getName(libName);
    oaScalarName cellName;
    cell->getName(cellName);

    oaString libname, cellname;
    libName.get(in_ns, libname);
    cellName.get(in_ns, cellname);
   
    PL()->ShowPromptV("Loading: %s/%s", (const char*)libname,
        (const char*)cellname);

    oaString lognam("loadCellRec[" + cellname + "]");
    cOAerrLogWrap logger(lognam, OA_ERRLOG, OA_DBGLOG);

    oaViewType *maskLayoutVT = oaViewType::get(oacMaskLayout);
    oaViewType *schematicVT = oaViewType::get(oacSchematic);
    oaViewType *symbolicVT = oaViewType::get(oacSchematicSymbol);

    CDs *sd_phys = 0;
    CDs *sd_elec = 0;
    CDs *sd_symb = 0;
    OItype oiret = OIok;

    // Make sure that we always reset the mode on exit.
    struct modefix_t
    {
        modefix_t(DisplayMode *ptr)     { mptr = ptr; mbak = *ptr; }
        ~modefix_t()                    { *mptr = mbak; }

    private:
        DisplayMode *mptr;
        DisplayMode mbak;
    };
    modefix_t _mf(&in_mode);

    // Set pointers to the views of interest.
    //
    oaView *maskLayoutView = 0;
    oaView *schematicView = 0;
    oaView *schematicSymbolView = 0;

    if (view) {
        // We know that the view is ok.

        if (in_read_mode != ReadElec) {
            if (view->getViewType() == maskLayoutVT)
                maskLayoutView = view;
        }
        if (in_read_mode != ReadPhys) {
            if (view->getViewType() == schematicVT)
                schematicView = view;
            else if (view->getViewType() == symbolicVT)
                schematicSymbolView = view;
        }
    }
    else {
        if (in_read_mode != ReadElec) {
            // Find layout view.
            oaScalarName viewName(in_ns, in_def_layout);
            oaCellView *cv = oaCellView::find(lib, cellName, viewName);
            if (cv) {
                if (cv->getView()->getViewType() != maskLayoutVT) {
                    Errs()->add_error(
                        "Layout view name %s does not have maskLayout type.",
                        in_def_layout);
                    return (OIerror);
                }
                maskLayoutView = cv->getView();
            }
        }
        if (in_read_mode != ReadPhys) {
            // Find schematic/symbol views.
            oaScalarName viewName(in_ns, in_def_schematic);
            oaCellView *cv = oaCellView::find(lib, cellName, viewName);
            if (cv) {
                if (cv->getView()->getViewType() != schematicVT) {
                    Errs()->add_error(
                        "Schematic view name %s does not have schematic type.",
                        in_def_schematic);
                    return (OIerror);
                }
                schematicView = cv->getView();
            }

            viewName.init(in_ns, in_def_dev_prop);
            cv = oaCellView::find(lib, cellName, viewName);
            if (cv) {
                if (cv->getView()->getViewType() != symbolicVT) {
                    Errs()->add_error(
                    "Symbol view name %s does not have schematicSymbol type.",
                        in_def_dev_prop);
                    return (OIerror);
                }
                schematicSymbolView = cv->getView();
            }
            if (!schematicSymbolView) {
                viewName.init(in_ns, in_def_symbol);
                cv = oaCellView::find(lib, cellName, viewName);
                if (cv) {
                    if (cv->getView()->getViewType() != symbolicVT) {
                        Errs()->add_error(
                    "Symbol view name %s does not have schematicSymbol type.",
                            in_def_symbol);
                        return (OIerror);
                    }
                    schematicSymbolView = cv->getView();
                }
            }
        }
    }

    if (!maskLayoutView && !schematicView && !schematicSymbolView) {
        Errs()->add_error("No views match given criteria.");
        return (OIerror);
    }

    // 1 if checked for supermaster status, 2 if is super master.
    int superMasterStatus = 0;

    // Take care of mapping to a new cellname if there is a conflict.
    const char *alt_cellname = Tstring(NameTab.cellNameAlias(libName,
        cellName, in_from_xic));

    if (maskLayoutView) {
        oaScalarName viewName;
        maskLayoutView->getName(viewName);
        oaString viewname;
        viewName.get(in_ns, viewname);
        in_mode = Physical;

        oaDesign *design = openDesign(libName, cellName, viewName,
            maskLayoutVT);
        if (design) {
            superMasterStatus = 1;
            if (!design->isSuperMaster()) {
                oiret = loadPhysicalDesign(design, alt_cellname, &sd_phys,
                    depth);
                if (oiret == OIok && sd_phys) {
                    if (sd_phys->prpty(XICP_PC_SCRIPT) &&
                            sd_phys->prpty(XICP_PC_PARAMS)) {
                        // The cell is a Native PCell!
                        sd_phys->setPCell(true, true, false);
                    }
                }
            }
            else {
                superMasterStatus = 2;
                if (in_sub_level == 0) {
                    if (in_skip_supermaster) {
                        // clean up and return ok.
                        design->close();
                        return (OIok);
                    }
                    if (!in_pc_params) {
                        // This is the initial call, go grab the
                        // parameters.
                        getSuperMasterParams(design);
                        if (!in_pc_params) {
                            // Can't evaluate super-master, so treat
                            // it like a normal cell.
                            oiret = loadPhysicalDesign(design, alt_cellname,
                                &sd_phys, depth);
                            if (oiret == OIok && sd_phys) {
                                if (new_cell_name && !*new_cell_name) {
                                    *new_cell_name =
                                        Tstring(sd_phys->cellname());
                                }
                            }
                        }
                        else {
                            // clean up and return ok.
                            design->close();
                            return (OIok);
                        }
                    }
                    else {
                        // We have a parameter set in in_pc_params. 
                        // We will create a sub-master for this param
                        // set if necessary, and add the set to the
                        // table.

                        char *cname;
                        design = handleSuperMaster(design, &cname);
                        oiret = loadPhysicalDesign(design, cname, &sd_phys,
                            depth);
                        delete [] cname;
                    }
                }
            }
            if (design)
                design->close();
        }
        if (oiret == OIok && sd_phys) {
            if (new_cell_name && !*new_cell_name)
                *new_cell_name = Tstring(sd_phys->cellname());
        }
        if (oiret == OIerror) {
            Errs()->add_error("Physical translation failed, "
                "library=%s, cell=%s view=%s.\n", (const char*)libname,
                (const char*)cellname, (const char*)viewname);
            return (oiret);
        }
        if (oiret == OIaborted)
            return (oiret);
    }

    // The elec_scale is reset if we find a property indicating that
    // the cell originated in Xic.  Note that the scale will hold-over
    // from the schematic read when reading a symbol.
    //
    in_elec_scale = CDS_ELEC_SCALE;

    if (schematicView) {
        oaScalarName viewName;
        schematicView->getName(viewName);
        oaString viewname;
        viewName.get(in_ns, viewname);
        in_mode = Electrical;

        oaDesign *design = openDesign(libName, cellName, viewName,
            schematicVT);
        if (design) {
            if (!design->isSuperMaster()) {
                oaDesign *symdesign = 0;
                if (schematicSymbolView) {
                    oaScalarName sViewName;
                    schematicSymbolView->getName(sViewName);
                    symdesign = openDesign(libName, cellName, sViewName,
                        symbolicVT);
                }
                oiret = loadElectricalDesign(design, symdesign, alt_cellname,
                    &sd_elec, depth);
                if (symdesign)
                    symdesign->close();
            }
            else if (in_sub_level == 0) {
                // The schematic cell must be a regular cell.
                Errs()->add_error("Can't accept schematic super-master, "
                    "library=%s, cell=%s view=%s.\n",
                    (const char*)libname, (const char*)cellname,
                    (const char*)viewname);
                oiret = OIerror;
            }
            design->close();
        }
        if (oiret == OIok && sd_elec) {
            if (new_cell_name && !*new_cell_name)
                *new_cell_name = Tstring(sd_elec->cellname());
        }
        if (oiret == OIerror) {
            Errs()->add_error("Electrical translation failed, "
                "library=%s, cell=%s view=%s.\n",
                (const char*)libname, (const char*)cellname,
                (const char*)viewname);
            return (oiret);
        }
        if (oiret == OIaborted)
            return (oiret);
    }

    if (schematicSymbolView) {
        oaScalarName viewName;
        schematicSymbolView->getName(viewName);
        oaString viewname;
        viewName.get(in_ns, viewname);
        in_mode = Electrical;

        oaDesign *design = openDesign(libName, cellName, viewName,
            symbolicVT);
        if (design) {
            if (!design->isSuperMaster()) {

                sd_symb = new CDs(0, Electrical);
                if (sd_elec && sd_elec->prptyList()) {
                    // We read all properties in the schematic pass,
                    // temporarily hang them on the symbol since the
                    // pass-thru links aren't set yet.

                    sd_symb->setPrptyList(sd_elec->prptyList());
                    sd_elec->setPrptyList(0);
                }

                // Give the cell a name during data entry, for
                // warning/error messages, and "[@cellName]" labels.
                //
                sd_symb->setName(CD()->CellNameTableAdd(cellname));
                oiret = loadElectricalDesign(design, 0, alt_cellname,
                    &sd_symb, depth);
                sd_symb->setName(0);
            }
            else if (in_sub_level == 0) {
                // The symbolic cell must be a regular cell.
                Errs()->add_error("Can't accept symbolic super-master, "
                    "library=%s, cell=%s view=%s.\n",
                    (const char*)libname, (const char*)cellname,
                    (const char*)viewname);
                oiret = OIerror;
            }
            design->close();
        }
        if (oiret == OIerror) {
            Errs()->add_error("Electrical translation failed, "
                "library=%s, cell=%s view=%s.\n",
                (const char*)libname, (const char*)cellname,
                (const char*)viewname);
            return (oiret);
        }
        if (oiret == OIaborted)
            return (oiret);
    }

    if (sd_symb) {
        // Link in the symbolic rep.
        if (!sd_elec) {
            // OA cell is purely symbolic, probably a device symbol.

            if (in_from_xic && FIO()->LookupLibCell(0, (const char*)cellname,
                    LIBdevice, 0)) {
                // The local device library cell has precedence, we never
                // overwrite cells from the device library.

                delete sd_symb;
                return (oiret);
            }

            // If there was a name clash, it gets fixed here, by using
            // alt_cellname

            // Create an empty cell to hang the symbolic rep on.  It
            // may already exist.
            ncType ncret = ncOK;
            in_mode = Electrical;
            sd_elec = newCell(alt_cellname, &ncret);
            if (ncret == ncError) {
                Errs()->add_error("Failed to open electrical cell %s.",
                    alt_cellname);
                delete sd_symb;
                return (OIerror);
            }
            if (oiret == OIok && sd_elec) {
                if (new_cell_name && !*new_cell_name)
                    *new_cell_name = Tstring(sd_elec->cellname());
            }
        }

        // The symbolic cell has all of the properties.  Remove them
        // here before the symbolic cell is linked, after which they
        // will be inaccessible!

        CDp *prps = sd_symb->prptyList();
        sd_symb->setPrptyList(0);

        // Put the properties back into the electrical cell.
        if (prps) {
            // The electrical cell should have no properties at present.
            CDp *plst = prps;
            while (plst->next_prp())
                plst = plst->next_prp();
            plst->set_next_prp(sd_elec->prptyList());
            sd_elec->setPrptyList(prps);
        }

        CDp_sym *ps = (CDp_sym*)sd_elec->prpty(P_SYMBLC);
        if (!ps) {
            ps = new CDp_sym;
            ps->set_active(true);
            ps->set_symrep(sd_symb);
            sd_symb->setSymbolic(true);
            sd_symb->setOwner(sd_elec);
            ps->set_next_prp(sd_elec->prptyList());
            sd_elec->setPrptyList(ps);
        }
        else {
            ps->set_active(true);
            ps->set_symrep(sd_symb);
            sd_symb->setSymbolic(true);
            sd_symb->setOwner(sd_elec);
        }

        // If the physical cell is a PCell, save a XICP_PC property
        // in the electrical cell too.
        if (superMasterStatus == 2) {
            oaScalarName viewName;
            schematicSymbolView->getName(viewName);
            oaString viewname;
            viewName.get(in_ns, viewname);
            char *dbname = PCellDesc::mk_dbname(libname, cellname, viewname);
            sd_elec->prptyRemove(XICP_PC);
            sd_elec->prptyAdd(XICP_PC, dbname);
            OAerrLog.add_log(OAlogPCell, "marking elec part of pcell %s %s",
                Tstring(sd_elec->cellname()), dbname);
            delete [] dbname;
        }
    }

    // Fix up the label reference pointers, etc.
    if (sd_elec) {
        sd_elec->prptyPatchAll();

        // Set the Device flag if the master is not a subcircuit.
        CDelecCellType tp = sd_elec->elecCellType();
        sd_elec->setDevice(tp != CDelecSubc);
    }

    CDm_gen mgen(sd_elec, GEN_MASTERS);
    for (CDm *mdesc = mgen.m_first(); mdesc; mdesc = mgen.m_next()) {
        CDs *msdesc = mdesc->celldesc();
        if (!msdesc)
            continue;

        if (!checkMasterXicProps(msdesc)) {
            Errs()->add_error(
                "Failed to set properties for electrical cell %s.",
                Tstring(msdesc->cellname()));
            return (OIerror);
        }
    }

    return (oiret);
}


// If sdesc doesn't have P_NAME and P_NODE properties, set these from
// OA if possible.  We should only need to do this here if sdesc was
// not translated because it would exceed the translation depth.  The
// sdesc is empty in that case, but we need to set the properties so
// they can be translated to instances.
//
bool
oa_in::checkMasterXicProps(CDs *sdesc)
{
    if (sdesc->prpty(P_NAME))
        return (true);
    if (sdesc->prpty(P_NODE))
        return (true);

    // This should have been set when the first instance was created,
    // and sdesc was created with the Unread flag set.
    //
    CDp *pd = sdesc->prpty(XICP_OA_ORIG);
    if (!pd)
        return (false);
    const char *s = pd->string();
    char *libname = lstring::gettok(&s, "/");
    char *cellname = lstring::gettok(&s);
    if (!cellname) {
        delete [] libname;
        return (false);
    }
    oaScalarName libName(in_ns, libname);
    oaScalarName cellName(in_ns, cellname);
    delete [] libname;

    oaLib *lib = oaLib::find(libName);
    if (!lib)
        return (false);

    lib->getAccess(oacReadLibAccess);
    oaCell *cell = oaCell::find(lib, cellName);
    if (!cell)
        return (false);

    oaView *schematicView = 0;
    oaView *schematicSymbolView = 0;
    oaViewType *schematicVT = oaViewType::get(oacSchematic);
    oaViewType *symbolicVT = oaViewType::get(oacSchematicSymbol);

    oaScalarName viewName(in_ns, in_def_schematic);
    oaCellView *cv = oaCellView::find(lib, cellName, viewName);
    if (cv && cv->getView()->getViewType() == schematicVT)
        schematicView = cv->getView();

    viewName.init(in_ns, in_def_dev_prop);
    cv = oaCellView::find(lib, cellName, viewName);
    if (cv && cv->getView()->getViewType() == symbolicVT)
        schematicSymbolView = cv->getView();
    if (!schematicSymbolView) {
        viewName.init(in_ns, in_def_symbol);
        cv = oaCellView::find(lib, cellName, viewName);
        if (cv && cv->getView()->getViewType() == symbolicVT)
            schematicSymbolView = cv->getView();
    }

    bool ret = true;
    if (schematicView) {
        oaScalarName viewName;
        schematicView->getName(viewName);

        oaDesign *design = openDesign(libName, cellName, viewName, schematicVT);
        if (design) {
            oaBlock *blk = design->getTopBlock();
            ret = blk && readTerms(blk, sdesc, false);
            design->close();
        }
    }
    if (ret && schematicSymbolView) {
        oaScalarName viewName;
        schematicSymbolView->getName(viewName);

        oaDesign *design = openDesign(libName, cellName, viewName, symbolicVT);
        if (design) {
            oaBlock *blk = design->getTopBlock();
            ret = blk && readTerms(blk, sdesc, true);
            design->close();
        }
    }
    lib->releaseAccess();
    sdesc->prptyRemove(XICP_OA_ORIG);
    return (ret);
}


// This function will
//  1) Generate lists of top-level cells, and cells that were never
//     defined.
//  2) Take care of resolving library cells.
//     All referenced or defined cells are in the database, and pointer
//     linkage is set (in CDs::makeCall).  If a cell still has the unread
//     flag set, it was never actually defined.
//  3) Compute the bounding boxes of all cells.  This will cause
//     insertion/reinsertion of instances as their BB's become valid,
//     in CDs::fixBBs.  This will also check for coincident instances.
//
bool
oa_in::markReferences()
{
    // This can't be reentrered, can cause infinite loop.
    if (in_mark_references)
        return (true);
    in_mark_references = true;

    stringlist::destroy(in_undef_phys);
    in_undef_phys = 0;
    stringlist::destroy(in_undef_elec);
    in_undef_elec = 0;
    stringlist::destroy(in_top_phys);
    in_top_phys = 0;
    stringlist::destroy(in_top_elec);
    in_top_elec = 0;

    SymTabGen gen(NameTab.cnameTab());
    SymTabEnt *h;
    while ((h = gen.next()) != 0) {
        const char *cname = h->stTag;
        CDcbin cbin;
        CDcdb()->findSymbol(cname, &cbin);

        long l = (long)h->stData;
        bool readp = (l & OAL_READP);
        bool reade = (l & OAL_READE);
        bool refp = (l & OAL_REFP);
        bool refe = (l & OAL_REFE);

        if ((cbin.phys() && refp && cbin.phys()->isUnread()) ||
                (cbin.elec() && refe && cbin.elec()->isUnread())) {
            CDcbin tcbin;
            FIO()->SetSkipFixBB(true);
            OItype oiret = FIO()->OpenLibCell(0, cname, LIBuser, &tcbin);
            FIO()->SetSkipFixBB(false);
            if (oiret != OIok) {
                // An error or user abort.  OIok is returned whether
                // or not the cell was actually found.
                Errs()->add_error("mark_references: %s.",
                    oiret == OIaborted ? "user abort" : "internal error");
                in_mark_references = false;
                return (false);
            }
            if (cbin.phys()) {
                if (!tcbin.phys()) {
                    // The cell was not resolved.
                    in_undef_phys = new stringlist(lstring::copy(h->stTag),
                        in_undef_phys);
                }
                cbin.phys()->setUnread(false);
            }
            if (cbin.elec()) {
                if (!tcbin.elec()) {
                    // The cell was not resolved.
                    in_undef_elec = new stringlist(lstring::copy(h->stTag),
                        in_undef_elec);
                }
                cbin.elec()->setUnread(false);
            }
        }

        if (readp && !refp && cbin.phys())
            in_top_phys = new stringlist(lstring::copy(h->stTag),
                in_top_phys);
        if (reade && !refe && cbin.elec())
            in_top_elec = new stringlist(lstring::copy(h->stTag),
                in_top_elec);
    }

    if (!FIO()->IsSkipFixBB()) {
        // Set the bounding boxes.
        if (in_top_phys) {
            ptrtab_t *stab = new ptrtab_t;
            for (stringlist *sl = in_top_phys; sl; sl = sl->next) {
                CDs *sdesc = CDcdb()->findCell(sl->string, Physical);
                if (!sdesc) {
                    delete stab;
                    Errs()->add_error("Physical cell %s not in database.",
                        sl->string);
                    in_mark_references = false;
                    return (false);
                }
                else if (!sdesc->fixBBs(stab)) {
                    delete stab;
                    Errs()->add_error(
                        "Failed to set bounding box of physical cell %s.",
                        sl->string);
                    in_mark_references = false;
                    return (false);
                }
            }
            delete stab;
        }
        if (in_top_elec) {
            ptrtab_t *stab = new ptrtab_t;
            for (stringlist *sl = in_top_elec; sl; sl = sl->next) {
                CDs *sdesc = CDcdb()->findCell(sl->string, Electrical);
                if (!sdesc) {
                    delete stab;
                    Errs()->add_error("Electrical cell %s not in database.",
                        sl->string);
                    in_mark_references = false;
                    return (false);
                }
                else if (!sdesc->fixBBs(stab)) {
                    delete stab;
                    Errs()->add_error(
                    "Failed to set bounding box of electrical cell %s.",
                        sl->string);
                    in_mark_references = false;
                    return (false);
                }
            }
            delete stab;
        }
    }

    // Now that we've set the cell BBs, go through and add any missing
    // property labels.  The BBs must be computed first so that labels
    // are placed in correct locations.

    gen = SymTabGen(NameTab.cnameTab());
    while ((h = gen.next()) != 0) {
        const char *cname = h->stTag;
        CDcbin cbin;
        CDcdb()->findSymbol(cname, &cbin);
        if (!cbin.elec())
            continue;
        CDm_gen mgen(cbin.elec(), GEN_MASTERS);
        for (CDm *mdesc = mgen.m_first(); mdesc; mdesc = mgen.m_next()) {
            CDc_gen cgen(mdesc);
            for (CDc *cdesc = cgen.c_first(); cdesc; cdesc = cgen.c_next()) {
                if (!cdesc->is_normal())
                    continue;

                // Create basic Xic properties if not present, and create
                // missing property labels.

                cOAprop::checkFixProperties(cdesc);
            }
        }
    }

    in_mark_references = false;
    return (true);
}


oaDesign *
oa_in::openDesign(const oaScalarName &libName,
    const oaScalarName &cellName, const oaScalarName &viewName,
    const oaViewType *viewType)
{
    oaDesign *design;
    try {
        if (!oaDesign::exists(libName, cellName, viewName))
            return (0);
        design = oaDesign::open(libName, cellName, viewName, viewType, 'r');
        if (OAerrLog.debug_load() && design && design->isSuperMaster()) {
            oaString evname, cellname;
            design->getPcellEvaluatorName(evname);
            cellName.get(cellname);
            OAerrLog.add_log(OAlogLoad, "SuperMaster %s requires evaluator %s.",
                (const char*)cellname, (const char*)evname);
        }
    }
    catch (oaCompatibilityError &ex) {
        cOA::handleFBCError(ex);
        oaString libname;
        oaString cellname;
        oaString viewname;
        libName.get(in_ns, libname);
        cellName.get(in_ns, cellname);
        viewName.get(in_ns, viewname);
        Errs()->add_error("Can't open design for library=%s cell=%s view=%s.",
            (const char*)libname, (const char*)cellname,
            (const char*)viewname);
        return (0);
    }
    catch (oaException &excp) {
        Errs()->add_error((const char*)excp.getMsg());
        oaString libname;
        oaString cellname;
        oaString viewname;
        libName.get(in_ns, libname);
        cellName.get(in_ns, cellname);
        viewName.get(in_ns, viewname);
        Errs()->add_error("Can't open design for library=%s cell=%s view=%s.",
            (const char*)libname, (const char*)cellname,
            (const char*)viewname);
        return (0);
    }
    return (design);
}


// If the design is a super-master, extract the default parameters and
// constraints.  Leave the parameter list in in_pc_params.
//
void
oa_in::getSuperMasterParams(oaDesign *design)
{
    if (!design->isSuperMaster())
        return;
    if (!design->getPcellDef()) {
        // We can't evaluate this super-master, so no placements
        // are possible except for default parameters.

        sLstr lstr;
        lcv(design, lstr);
        lstr.add(":\n");
        lstr.add(
        "    This cell is a pcell that lacks an OpenAccess evaluation\n"
        "    plug-in.  Instantiations will use default parameters only.\n");

        OAerrLog.add_err(IFLOG_WARN, lstr.string());
        return;
    }
    // Grab the parameter constraints.  Ciranova pcells have the
    // constraint definitions as sub-properties keyed by the parameter
    // name, on an oaHierProp named "cnParamConstraints.

    SymTab *pctab = cOAprop::getPropTab(design);

    oaParamArray parray;
    design->getParams(parray);

    in_pc_params = cOAprop::getPcParameters(parray, pctab);
    delete pctab;
}


// A parameters set exists in in_pc_params.  Return the design for
// this set, and a sub-master cellname.
//
oaDesign *
oa_in::handleSuperMaster(oaDesign *design, char **pcname)
{
    *pcname = 0;
    if (!design->isSuperMaster())
        return (0);
    oaParamArray parray;
    design->getParams(parray);

    int i = 0;
    for (PCellParam *p = in_pc_params; p; p = p->next()) {
        if (p->changed()) {
            switch (p->type()) {
            case PCPbool:
                parray[i].setBooleanVal(p->boolVal());
                break;
            case PCPint:
                parray[i].setIntVal(p->intVal());
                break;
            case PCPtime:
                parray[i].setTimeVal(p->timeVal());
                break;
            case PCPfloat:
                parray[i].setFloatVal(p->floatVal());
                break;
            case PCPdouble:
                parray[i].setDoubleVal(p->doubleVal());
                break;
            case PCPstring:
                parray[i].setStringVal(p->stringVal());
                break;
            case PCPappType:
                break;
            }
        }
        i++;
    }

    oaScalarName libName, cellName, viewName;
    design->getLibName(libName);
    design->getCellName(cellName);
    design->getViewName(viewName);

    oaDesign *sub = oaDesign::open(libName, cellName, viewName, parray);
    if (sub) {
        PCellParam *pm = cOAprop::getPcParameters(parray, 0);

        oaString libname, cellname, viewname;
        libName.get(in_ns, libname);
        cellName.get(in_ns, cellname);
        viewName.get(in_ns, viewname);

        *pcname = PC()->addSubMaster(libname, cellname, viewname, pm);
        PCellParam::destroy(pm);
        design->close();
        return (sub);
    }
    // The observer will print an error message.
    design->close();
    return (0);
}


OItype
oa_in::loadPhysicalDesign(const oaDesign *design, const char *cname, CDs **sdp,
    oaInt4 depth)
{
    if (depth < 0)
        return (OIok);
    if (!design)
        return (OIerror);

    // We never open a super-master directly.  We get here with a
    // super-master from loadMaster, when the master is not a sub
    // master, but loadCellRec opens all views and another view may be
    // a pcell and we skip processing that design here.
    if (design->isSuperMaster())
        return (OIok);   

    if (design->getViewType() != oaViewType::get(oacMaskLayout))
        return (OIerror);

    if (OAerrLog.debug_load()) {
        sLstr lstr;
        lstr.add("Loading ");
        lcv(design, lstr);
        OAerrLog.add_log(OAlogLoad, "%s\n", lstr.string());
    }
    oaString cellname;
    if (cname && *cname)
        cellname = cname;
    else
        design->getCellName(in_ns, cellname);

    // Read in the masters first, these are needed for finding
    // terminal locations.

    OItype oiret = OIok;
    if (depth > 0) {
        // Translate instance masters.
        // The blk will be null if a PCell can't be evaluated.
        oaBlock *blk = design->getTopBlock();
        if (!blk)
            return (OIerror);
        oaIter<oaInstHeader> iter(blk->getInstHeaders());
        while (oaInstHeader *iHdr = iter.getNext()) {
            // Try to load all masters, but return an error if one
            // fails.

            OItype ot = loadMaster(iHdr, depth - 1);
            if (ot == OIaborted)
                return (OIaborted);
            if (ot != OIok && oiret != OIok)
                oiret = OIerror;
        }
        oaIter<oaViaHeader> viter(blk->getViaHeaders());
        while (oaViaHeader *vHdr = viter.getNext()) {
            OItype ot = loadVia(vHdr, depth - 1);
            if (ot == OIaborted)
                return (OIaborted);
            if (ot != OIok) {
                oiret = ot;
                break;
            }
        }
    }
    if (!readPhysicalDesign(design, cellname, sdp))
        return (OIerror);
    return (oiret);
}


OItype
oa_in::loadElectricalDesign(const oaDesign *design, const oaDesign *symdesign,
    const char *cname, CDs **sdp, oaInt4 depth)
{
    if (depth < 0)
        return (OIok);
    if (!design) {
        design = symdesign;
        symdesign = 0;
    }
    if (!design)
        return (OIerror);

    // We never open a super-master directly.  We get here with a
    // super-master from loadMaster, when the master is not a sub
    // master, but loadCellRec opens all views and another view may be
    // a pcell and we skip processing that design here.
    if (design->isSuperMaster())
        return (OIok);   

    if (design->getViewType() == oaViewType::get(oacMaskLayout))
        return (OIerror);

    if (OAerrLog.debug_load()) {
        sLstr lstr;
        lstr.add("Loading ");
        lcv(design, lstr);
        OAerrLog.add_log(OAlogLoad, "%s\n", lstr.string());
    }
    oaString cellname;
    if (cname && *cname)
        cellname = cname;
    else
        design->getCellName(in_ns, cellname);

    // Read in the masters first, these are needed for finding
    // terminal locations.

    OItype oiret = OIok;
    if (design->getViewType() == oaViewType::get(oacSchematic)) {
        if (depth > 0) {
            // Translate instance masters.
            oaBlock *blk = design->getTopBlock();
            if (!blk)
                return (OIok);  // Just an empty cell?
            oaIter<oaInstHeader> iter(blk->getInstHeaders());
            while (oaInstHeader *iHdr = iter.getNext()) {
                // Try to load all masters, but return an error if one
                // fails.

                OItype ot = loadMaster(iHdr, depth - 1);
                if (ot == OIaborted)
                    return (OIaborted);
                if (ot != OIok && oiret != OIok)
                    oiret = OIerror;
            }
        }
        if (!readElectricalDesign(design, symdesign, cellname, sdp))
            return (OIerror);
    }
    else if (design->getViewType() == oaViewType::get(oacSchematicSymbol)) {
        if (!readElectricalDesign(design, 0, cellname, sdp))
            return (OIerror);
        // Xic symbols can't have instance placements.
    }
    return (oiret);
}


namespace {
    // Add the XICP_STDVIA property.  The marks the cell as a standard
    // via sub-master.
    void
    add_std_via_prop(const oaStdViaHeader *stdViaHeader, CDs *sd, CDo *od = 0)
    {
        char *str = cOAvia::getStdViaString(stdViaHeader);
        oaString vname;
        stdViaHeader->getViaDefName(vname);
        sLstr lstr;
        lstr.add(vname);
        lstr.add_c(' ');
        lstr.add(str);
        delete [] str;
        if (sd)
            sd->prptyAdd(XICP_STDVIA, lstr.string());
        if (od)
            od->prptyAdd(XICP_STDVIA, lstr.string(), Physical);
    }
}


OItype
oa_in::loadVia(const oaViaHeader *viaHeader, oaUInt4  depth)
{
    if (viaHeader->isSuperHeader() || !viaHeader->getVias().getCount())
        return (OIok);

    oaScalarName libName;
    oaScalarName cellName;
    oaScalarName viewName;

    // Output custom via.
    if (viaHeader->getType() == oacCustomViaHeaderType) {
        oaCustomViaHeader *header = (oaCustomViaHeader*)viaHeader;
        oaString defName;
        header->getViaDefName(defName);
        header->getLibName(libName);
        header->getCellName(cellName);
        header->getViewName(viewName);

        oaParamArray allParams;
        header->getAllParams(allParams);
        if (allParams.getNumElements() == 0) {
            // Non-parameterized custom via.
            oaString viewname;
            viewName.get(in_ns, viewname);
            const char *new_cell_name;
            OItype oiret = loadCellRec(libName, cellName,
                (const char*)viewname, depth, &new_cell_name);
            if (oiret == OIok) {
                // Add XICP_CSTMVIA property to cell.
                CDs *sd = CDcdb()->findCell(new_cell_name, in_mode);
                if (sd)
                    sd->prptyAdd(XICP_CSTMVIA, defName);
            }
            return (oiret);
        }

        oaLib *lib = oaLib::find(libName);
        if (!lib) {
            oaString libname;
            libName.get(in_ns, libname);
            Errs()->add_error("Library %s was not found in lib.defs.",
                (const char*)libname);
            return (OIerror);
        }

        oaParamArray params;
        header->getParams(params);
        CDcellName cname = NameTab.getMasterName(libName, cellName, viewName,
            params, true, in_from_xic);

        // Check if the cell has already been translated.
        //
        unsigned long f = NameTab.findCname(cname);
        if (in_mode == Physical && (f & OAL_READP))
            return (OIok);
        if (in_mode == Electrical && (f & (OAL_READE | OAL_READS)))
            return (OIok);

        if (dspPkgIf()->CheckForInterrupt()) {
            if (XM()->ConfirmAbort("Interrupt received, abort load? "))
                return (OIaborted);
        }

        lib->getAccess(oacReadLibAccess);

        oaDesign *design;

        try {
            design = oaDesign::open(libName, cellName, viewName, params);
        }
        catch (oaCompatibilityError &ex) {
            cOA::handleFBCError(ex);
            oaString libname, cellname, viewname;
            libName.get(in_ns, libname);
            cellName.get(in_ns, cellname);
            viewName.get(in_ns, viewname);
            Errs()->add_error(
                "Can't open design for library=%s cell=%s view=%s.",
                (const char*)libname, (const char*)cellname,
                (const char*)viewname);
            lib->releaseAccess();
            return (OIerror);
        }
        catch (oaException &excp) {
            Errs()->add_error((const char*)excp.getMsg());
            oaString libname, cellname, viewname;
            libName.get(in_ns, libname);
            cellName.get(in_ns, cellname);
            viewName.get(in_ns, viewname);
            Errs()->add_error(
                "Can't open design for library=%s cell=%s view=%s.",
                (const char*)libname, (const char*)cellname,
                (const char*)viewname);
            lib->releaseAccess();
            return (OIerror);
        }

        CDs *sd = 0;
        OItype oiret = loadPhysicalDesign(design, Tstring(cname), &sd, depth);

        design->close();
        lib->releaseAccess();
        if (oiret == OIok) {
            // Add XICP_CSTMVIA property to sd.
            sLstr lstr;
            lstr.add(defName);
            lstr.add_c(' ');
            PCellParam *pm = cOAprop::getPcParameters(params, 0);
            char *str = pm->string(true);
            lstr.add(str);
            delete [] str;
            PCellParam::destroy(pm);
            sd->prptyAdd(XICP_CSTMVIA, lstr.string());
        }
        return (oiret);
    }

    // Output stdVia.
    if (viaHeader->getType() == oacStdViaHeaderType) {
        if (!viaHeader->getViaDef()) {
            oaString viaDefName;
            viaHeader->getViaDefName(viaDefName);
            Errs()->add_error("Can't resolve via with name %s.",
                (const char*)viaDefName);
            return (OIerror);
        }

        oaStdViaHeader *stdViaHeader = (oaStdViaHeader*)viaHeader;
        char *viaCellname = getViaName(stdViaHeader);
        if (viaCellname) {

            // Check if the cell has already been translated.
            //
            unsigned long f = NameTab.findCname(viaCellname);
            if (in_mode == Physical && (f & OAL_READP)) {
                delete [] viaCellname;
                return (OIok);
            }
            if (in_mode == Electrical && (f & (OAL_READE | OAL_READS))) {
                delete [] viaCellname;
                return (OIok);
            }

            if (dspPkgIf()->CheckForInterrupt()) {
                if (XM()->ConfirmAbort("Interrupt received, abort load? ")) {
                    delete [] viaCellname;
                    return (OIaborted);
                }
            }

            oaDesign *design = viaHeader->getMaster();
            if (!design) {
                oaString viaDefName;
                viaHeader->getViaDefName(viaDefName);
                Errs()->add_error("No design found for via %s.\n",
                    (const char*)viaDefName );
                delete [] viaCellname;
                return (OIerror);
            }

            CDs *sd = 0;
            bool ret = readPhysicalDesign(design, viaCellname, &sd);
            if (ret && sd)
                add_std_via_prop(stdViaHeader, sd);

            design->close();
            delete [] viaCellname;
            return (ret ? OIok : OIerror);
        }
    }
    return (OIerror);
}


// Return a std via cell name to correspond to hdr.
//
char *
oa_in::getViaName(const oaViaHeader *hdr)
{
    if (hdr->getType() == oacCustomViaHeaderType) {
        oaCustomViaHeader *header = (oaCustomViaHeader*)hdr;
        oaString cellname;
        header->getCellName(in_ns, cellname);
        return (lstring::copy(cellname));
    }

    if (hdr->getType() == oacStdViaHeaderType) {
        oaStdViaHeader *header = (oaStdViaHeader*)hdr;

        oaString vianame;
        header->getViaDefName(vianame);
        oaViaDef *def = header->getViaDef();
        if (!def)
            return (0);
        oaString libname;
        def->getTech()->getLibName(in_ns, libname);
         
        oaViaParam params;
        header->getParams(params);

        char buf[256];
        sprintf(buf, "%s/%s", (const char*)libname, (const char*)vianame);

        if (!in_via_tab)
            in_via_tab = new SymTab(false, false);
        SymTabEnt *ent = SymTab::get_ent(in_via_tab, buf);
        if (!ent) {
            ViaDesc *vd = new ViaDesc(buf);
            ViaItem *vi = new ViaItem(params);
            vd->addItem(vi);
            in_via_tab->add(vd->dbname(), vd, false);
            return (vd->cellname(vianame, vi));
        }
        ViaDesc *vd = (ViaDesc*)ent->stData;
        ViaItem *vi = vd->findItem(params);
        if (!vi) {
            vi = new ViaItem(params);
            vd->addItem(vi);
        }
        return (vd->cellname(vianame, vi));
    }
    return (0);
}


// Load the instance master, if it hasn't been loaded already.  This
// handles PCells.
//
OItype
oa_in::loadMaster(const oaInstHeader *hdr, oaInt4 depth)
{
    if (hdr->isSuperHeader())
        return (OIok);

    oaScalarName libName, cellName, viewName;
    hdr->getLibName(libName);
    hdr->getCellName(cellName);
    hdr->getViewName(viewName);

    if (!hdr->isSubHeader()) {

        // When in_sub_level > 0 the Parameters popup is innhibited
        // when opening a pcell super-master.  This can happen since
        // loadCellRec opens all cell views.  Here, the view we need
        // is not a pcell, but another view may well be.  It will be
        // opened but not read.
        //
        // If a cell has a view name that is not the default, we
        // assume that it is something special like a via.  In this
        // case the actual view name is passed.  The electrical part
        // of this cell won't be opened here (there probably isn't
        // any).

        in_sub_level++;
        oaString viewname;
        viewName.get(viewname);
        OItype oiret;
        if (in_mode == Physical && viewname != in_def_layout)
            oiret = loadCellRec(libName, cellName, viewname, depth, 0);
        else
            oiret = loadCellRec(libName, cellName, 0, depth, 0);
        in_sub_level--;
        return (oiret);
    }

    oaLib *lib = oaLib::find(libName);
    if (!lib) {
        oaString libname;
        libName.get(in_ns, libname);
        Errs()->add_error("Library %s was not found in lib.defs.",
            (const char*)libname);
        return (OIerror);
    }

    oaParamArray allParams;
    hdr->getAllParams(allParams);
    bool has_params = (allParams.getNumElements() > 0);

    oaParamArray params;
    hdr->getParams(params);
    CDcellName cname = NameTab.getMasterName(libName, cellName, viewName,
        params, has_params, in_from_xic);

    // Check if the cell has already been translated.
    //
    unsigned long f = NameTab.findCname(cname);
    if (in_mode == Physical && (f & OAL_READP))
        return (OIok);
    if (in_mode == Electrical && (f & (OAL_READE | OAL_READS)))
        return (OIok);

    if (dspPkgIf()->CheckForInterrupt()) {
        if (XM()->ConfirmAbort("Interrupt received, abort load? "))
            return (OIaborted);
    }

    lib->getAccess(oacReadLibAccess);

    oaDesign *design;

    try {
        design = oaDesign::open(libName, cellName, viewName, params);
    }
    catch (oaCompatibilityError &ex) {
        cOA::handleFBCError(ex);
        oaString libname, cellname, viewname;
        libName.get(in_ns, libname);
        cellName.get(in_ns, cellname);
        viewName.get(in_ns, viewname);
        Errs()->add_error("Can't open design for library=%s cell=%s view=%s.",
            (const char*)libname, (const char*)cellname,
            (const char*)viewname);
        lib->releaseAccess();
        return (OIerror);
    }
    catch (oaException &excp) {
        Errs()->add_error((const char*)excp.getMsg());
        oaString libname, cellname, viewname;
        libName.get(in_ns, libname);
        cellName.get(in_ns, cellname);
        viewName.get(in_ns, viewname);
        Errs()->add_error("Can't open design for library=%s cell=%s view=%s.",
            (const char*)libname, (const char*)cellname,
            (const char*)viewname);
        lib->releaseAccess();
        return (OIerror);
    }

    CDs *sd = 0;
    // This is a sub-master and must be physical?
    OItype oiret;
    if (in_mode == Physical)
        oiret = loadPhysicalDesign(design, Tstring(cname), &sd, depth);
    else
        oiret = loadElectricalDesign(design, 0, Tstring(cname), &sd, depth);

    design->close();
    lib->releaseAccess();
    return (oiret);
}


// Find or open the Xic cell.  This observes non-clobbering of library
// cells and the overwrite mode set by the user.
//
// Sub-masters and via cells will have is_submaster true.  In this
// case, avoid merge control and always skip once created, and set the
// modified flag.  These cells will not have their origin set to OA.
//
CDs *
oa_in::newCell(const char *cellname, ncType *ncret, bool is_submaster)
{
    *ncret = ncError;

    CDs *sdesc = CDcdb()->findCell(cellname, in_mode);
    bool cell_exists = (sdesc != 0);
    OAerrLog.add_log(OAlogLoad, "called newCell %s exists=%d mode=%d",
        cellname, cell_exists, in_mode);
    if (!sdesc)
        sdesc = CDcdb()->insertCell(cellname, in_mode);
    else if (sdesc->isLibrary() && (FIO()->IsReadingLibrary() ||
            FIO()->IsNoOverwriteLibCells())) {
        // Skip reading into library cells.
        if (!FIO()->IsReadingLibrary()) {

            // Never overwrite a library cell already in memory, unless
            // we are actually reading in a library.

            *ncret = ncSkip;
            return (sdesc);
        }
    }
    else if (sdesc->isLibrary() && sdesc->isDevice()) {

        // Never overwrite device library cells.

        *ncret = ncSkip;
        return (sdesc);
    }

    // Note:  If the cell in memory is empty, we allow it to be
    // overwritten without confirmation.

    if (in_mode == Physical) {
        if (sdesc->isUnread())
            sdesc->setUnread(false);
        else if (!sdesc->isEmpty()) {
            long f = 0;
            if (cell_exists) {
                if (!in_skip_tab)
                    in_skip_tab = new SymTab(false, false);
                f = (long)SymTab::get(in_skip_tab, cellname);
                if (f == (long)ST_NIL) {
                    if (is_submaster) {
                        // Sub-master already exists, skip it.
                        in_skip_tab->add(Tstring(sdesc->cellname()), 0,
                            false);
                    }
                    else {
                        mitem_t mi(cellname);
                        if (!FIO()->IsNoOverwritePhys())
                            mi.overwrite_phys = true;
                        if (!FIO()->IsNoOverwriteElec())
                            mi.overwrite_elec = true;
                        FIO()->ifMergeControl(&mi);
                        f = mi.overwrite_phys | (mi.overwrite_elec << 1);
                        in_skip_tab->add(Tstring(sdesc->cellname()),
                            (void*)f, false);
                    }
                }
            }

            if (f & 1) {
                sdesc->setImmutable(false);
                sdesc->setLibrary(false);
                sdesc->clear(true);
            }
            else if (cell_exists) {
                *ncret = ncSkip;
                return (sdesc);
            }
        }
        else {
            // Empty cell, do this anyway to clear properties.
            sdesc->setImmutable(false);
            sdesc->setLibrary(false);
            sdesc->clear(true);
        }
    }
    else {
        if (sdesc->isUnread()) {
            sdesc->setUnread(false);
            sdesc->prptyRemove(XICP_OA_ORIG);
        }
        else if (!sdesc->isEmpty()) {
            long f = 0;
            if (cell_exists) {
                if (!in_skip_tab)
                    in_skip_tab = new SymTab(false, false);
                f = (long)SymTab::get(in_skip_tab, cellname);
                if (f == (long)ST_NIL) {
                    if (is_submaster) {
                        // Sub-master already exists, skip it.
                        in_skip_tab->add(Tstring(sdesc->cellname()), 0,
                            false);
                    }
                    else {
                        mitem_t mi(cellname);
                        if (!FIO()->IsNoOverwritePhys())
                            mi.overwrite_phys = true;
                        if (!FIO()->IsNoOverwriteElec())
                            mi.overwrite_elec = true;
                        FIO()->ifMergeControl(&mi);
                        f = mi.overwrite_phys | (mi.overwrite_elec << 1);
                        in_skip_tab->add(Tstring(sdesc->cellname()),
                            (void*)f, false);
                    }
                }
            }

            if (f & 2) {
                // overwrite_elec was set
                sdesc->setImmutable(false);
                sdesc->setLibrary(false);
                sdesc->clear(true);
            }
            else if (cell_exists) {
                *ncret = ncSkip;
                return (sdesc);
            }
        }
        else {
            // Empty cell, do this anyway to clear properties.
            sdesc->setImmutable(false);
            sdesc->setLibrary(false);
            sdesc->clear(true);
        }
    }
    *ncret = ncOK;
    if (is_submaster)
        sdesc->incModified();
    return (sdesc);
}


CDcellName
oa_in::checkSubMaster(CDcellName cname, CDp *plist) 
{
    if (in_mode == Physical) {
        CDcellName cn = (CDcellName)SymTab::get(in_submaster_tab,
            (unsigned long)cname);
        if (cn != (CDcellName)ST_NIL)
            cname = cn;
        else {
            CDs *sdnew;
            cn = cname;
            if (FIO()->ifOpenSubMaster(plist, &sdnew) && sdnew) {
                cname = sdnew->cellname();
                if (!in_submaster_tab)
                    in_submaster_tab = new SymTab(false, false);
                in_submaster_tab->add((unsigned long)cn, cname, false);
            }
        }
    }
    return (cname);
}


bool 
oa_in::readPhysicalDesign(const oaDesign *design, const oaString &xic_cname,
    CDs **sdp)
{
    if (!design) {
        Errs()->add_error("Null design handle encountered.");
        return (false);
    }
    if (design->getViewType() != oaViewType::get(oacMaskLayout)) {
        Errs()->add_error("readPhysicalDesign:  non-layout data passed.");
        return (false);
    }
    if (in_mode != Physical) {
        Errs()->add_error("readPhysicalDesign:  not in physical mode.");
        return (false);
    }

    oaString libname;
    design->getLibName(in_ns, libname);
    oaBlock *blk = design->getTopBlock();
    if (!blk) {
        oaString cellname;
        oaString viewname;
        design->getCellName(in_ns, cellname);
        design->getViewName(in_ns, viewname);

        Errs()->add_error(
            "No top block in design library=%s cell=%s view=%s.",
            (const char*)libname, (const char*)cellname,
            (const char*)viewname);
        return (false);
    }

    // Open design in the database.
    ncType ncret = ncOK;
    CDs *sdesc = sdp ? *sdp : 0;
    if (!sdesc)
        sdesc = newCell(xic_cname, &ncret, design->isSubMaster());

    if (ncret == ncError) {
        Errs()->add_error("Failed to open/reopen %s cell %s.",
            DisplayModeNameLC(in_mode), (const char*)xic_cname);
        return (false);
    }
    if (sdp)
        *sdp = sdesc;

    // Set a flag indicating that a cell has been read, of the
    // corresponding type.
    //
    NameTab.updateCname(xic_cname, OAL_READP);

    if (ncret == ncSkip)
        return (true);

    if (design->isSubMaster()) {
        // Note that sub-masters aren't OA cells.  Set the properties
        // and flags for Xic.

        sdesc->prptyRemove(XICP_PC);
        sdesc->prptyRemove(XICP_PC_PARAMS);
        oaString cellname;
        oaString viewname;
        design->getCellName(in_ns, cellname);
        design->getViewName(in_ns, viewname);
        char *dbname = PCellDesc::mk_dbname(libname, cellname, viewname);
        sdesc->prptyAdd(XICP_PC, dbname);
        delete [] dbname;

        oaParamArray parray;
        design->getParams(parray, false);  // Non-default params only.
        PCellParam *pm = cOAprop::getPcParameters(parray, 0);
        if (pm) {
            char *pmstr = pm->string(true);
            sdesc->prptyAdd(XICP_PC_PARAMS, pmstr);
            delete [] pmstr;
            PCellParam::destroy(pm);
        }
        sdesc->setPCell(true, false, true);
    }
    else {
        sdesc->setFileName(libname);
        sdesc->setFileType(Foa);
    }
    if (!readPhysicalProperties(design, sdesc))
        return (false);

    // Read Instances.  This must be done before reading text labels
    // for bound labels to work.
    //
    if (!readInstances(blk, sdesc))
        return (false);

    if (!readVias(blk, sdesc))
        return (false);

    // Read shapes.
    if (!readGeometry(blk, sdesc))
        return (false);

    return (true);
}


bool 
oa_in::readElectricalDesign(const oaDesign *design, const oaDesign *symdesign,
    const oaString &xic_cname, CDs **sdp)
{
    if (!design) {
        design = symdesign;
        symdesign = 0;
    }
    if (!design) {
        Errs()->add_error("Null design handle encountered.");
        return (false);
    }
    if (design->getViewType() == oaViewType::get(oacMaskLayout)) {
        Errs()->add_error("readElectricalDesign:  layout data passed.");
        return (false);
    }
    if (in_mode == Physical) {
        Errs()->add_error("readElectricalDesign:  in physical mode.");
        return (false);
    }

    oaString libname;
    design->getLibName(in_ns, libname);
    oaBlock *blk = design->getTopBlock();
    if (!blk) {
        oaString cellname;
        oaString viewname;
        design->getCellName(in_ns, cellname);
        design->getViewName(in_ns, viewname);

        Errs()->add_error(
            "No top block in design library=%s cell=%s view=%s.",
            (const char*)libname, (const char*)cellname,
            (const char*)viewname);
        return (false);
    }

    bool symbolic =
        design->getViewType() == oaViewType::get(oacSchematicSymbol);

    // Open design in the database.
    ncType ncret = ncOK;
    CDs *sdesc = sdp ? *sdp : 0;
    if (!sdesc) {
        if (symbolic)
            sdesc = new CDs(0, Electrical);
        else
            sdesc = newCell(xic_cname, &ncret, design->isSubMaster());
    }

    if (ncret == ncError) {
        Errs()->add_error("Failed to open/reopen %s cell %s.",
            DisplayModeNameLC(in_mode), (const char*)xic_cname);
        return (false);
    }
    if (sdp)
        *sdp = sdesc;

    // Set a flag indicating that a cell has been read, of the
    // corresponding type.
    //
    long f = 0;
    if (in_mode == Electrical) {
        if (symbolic)
            f = OAL_READS;
        else
            f = OAL_READE;
    }
    NameTab.updateCname(xic_cname, f);

    if (ncret == ncSkip)
        return (true);

    // Read properties.  Read properties from schematic and symbol in
    // the schematic pass, skip reading properties in the symbol pass
    // unless there was no schematic.  In Xic, there is no distinction
    // between schematic and symbol properties.
    //
    if (!symbolic) {
        if (!readElectricalProperties(design, symdesign, sdesc))
            return (false);
    }
    else {
        long f = NameTab.findCname(xic_cname);
        if (!(f & OAL_READE)) {
            if (!readElectricalProperties(design, 0, sdesc))
                return (false);
        }
    }

    // Read Instances.  This must be done before reading text labels
    // for bound labels to work.  If symbolic, we can't handle
    // instances, so instances read here will cause an error.
    //
    if (!readInstances(blk, sdesc))
        return (false);

    // Read shapes.
    if (!readGeometry(blk, sdesc))
        return (false);

    if (!readTerms(blk, sdesc, symbolic))
        return (false);

    return (true);
}


bool
oa_in::readInstances(const oaBlock *blk, CDs *sdesc)
{
    bool ret = true;
    try {
        oaIter<oaInstHeader> headers(blk->getInstHeaders());
        while (oaInstHeader *header = headers.getNext()) {
            if (header->isSuperHeader())
                continue;

            oaScalarName libName, cellName, viewName;
            header->getLibName(libName);
            header->getCellName(cellName);
            header->getViewName(viewName);

            oaParamArray allParams;
            header->getAllParams(allParams);
            bool has_params = (allParams.getNumElements() > 0);

            oaParamArray params;
            header->getParams(params);
            CDcellName cname = NameTab.getMasterName(libName, cellName,
                viewName, params, has_params, in_from_xic);

            long f = in_mode == Physical ? OAL_REFP : OAL_REFE;
            NameTab.updateCname(cname, f);

            ret = readOaInstHeader(header, sdesc, Tstring(cname));
            if (!ret)
                break;
        }
    }
    catch (oaCompatibilityError &ex) {
        cOA::handleFBCError(ex);
        ret = false;
    }
    catch (oaException &excp) {
        Errs()->add_error((const char*)excp.getMsg());
        ret = false;
    }
    if (!ret)
        Errs()->add_error("Failed reading instances.");
    return (ret);
}


bool
oa_in::readVias(const oaBlock *blk, CDs *sdesc)
{
    bool ret = true;
    try {
        oaIter<oaViaHeader> viaHeaders(blk->getViaHeaders());
        while (oaViaHeader *viaHeader = viaHeaders.getNext()) {
            if (viaHeader->isSuperHeader())
                continue;

            char *vianame = getViaName(viaHeader);
            long f = in_mode == Physical ? OAL_REFP : OAL_REFE;
            NameTab.updateCname(vianame, f);

            ret = readOaViaHeader(viaHeader, sdesc, vianame);
            delete [] vianame;
            if (!ret)
                break;
        }
    }
    catch (oaCompatibilityError &ex) {
        cOA::handleFBCError(ex);
        ret = false;
    }
    catch (oaException &excp) {
        Errs()->add_error((const char*)excp.getMsg());
        ret = false;
    }
    if (!ret)
        Errs()->add_error("Failed reading vias.");
    return (ret);
}


bool
oa_in::readGeometry(const oaBlock *blk, CDs *sdesc)
{
    bool ret = true;
    try {
        oaIter<oaLPPHeader> lpps(blk->getLPPHeaders());
        while (oaLPPHeader *lpp = lpps.getNext()) {
            ret = readOaLPPHeader(lpp, sdesc);
            if (!ret)
                break;
        }
    }
    catch (oaCompatibilityError &ex) {
        cOA::handleFBCError(ex);
        ret = false;
    }
    catch (oaException &excp) {
        Errs()->add_error((const char*)excp.getMsg());
        ret = false;
    }
    if (!ret)
        Errs()->add_error("Failed reading LPP objects.");
    return (ret);
}


// Set up the P_NODE and P_NAME properties of electrical cells, needed
// for Virtuoso conversion only.
//
bool
oa_in::readTerms(const oaBlock *blk, CDs *sdesc, bool symbolic)
{
    if (in_from_xic)
        return (true);
    if (!sdesc->isElectrical())
        return (true);

    sLstr lstr;
    oaString cname, vname;
    oaDesign *design = blk->getDesign();
    design->getCellName(oaNativeNS(), cname);
    design->getViewName(oaNativeNS(), vname);
    oaString tmp("setupNets[" + cname + "/" + vname + "]");

    cOAerrLogWrap logger(tmp, OA_ERRLOG, OA_DBGLOG);

    cOAnetHandler nh(blk, sdesc, in_elec_scale, in_def_symbol, in_def_dev_prop);
    bool ret = nh.setupNets(symbolic);
    return (ret);
}


bool
oa_in::readPhysicalProperties(const oaDesign *design, CDs *sdesc)
{
    bool ret = true;
    try {
        in_from_xic = false;
        CDp *p0 = readProperties(design);
        if (p0) {
            stringlist *s0 = sdesc->prptyApplyList(0, &p0);
            CDp::destroy(p0);
            if (s0) {
                stringlist *s = s0;
                while (s->next)
                    s = s->next;
                s->next = in_warnings;
                in_warnings = s0;
            }
        }
    }
    catch (oaCompatibilityError &ex) {
        cOA::handleFBCError(ex);
        ret = false;
    }
    catch (oaException &excp) {
        Errs()->add_error((const char*)excp.getMsg());
        ret = false;
    }
    if (!ret)
        Errs()->add_error("Failed reading cell (design) properties.");
    return (ret);
}


bool
oa_in::readElectricalProperties(const oaDesign *design,
    const oaDesign *symdesign, CDs *sdesc)
{
    bool ret = true;
    try {
        in_from_xic = false;
        CDp *p0 = 0;
        if (symdesign)
            p0 = readProperties(symdesign);
        CDp *px = p0;
        if (px) {
            while (px->next_prp())
                px = px->next_prp();
            px->set_next_prp(readProperties(design));
        }
        else
            p0 = readProperties(design);
        if (p0) {
            stringlist *s0 = sdesc->prptyApplyList(0, &p0);
            CDp::destroy(p0);
            if (s0) {
                stringlist *s = s0;
                while (s->next)
                    s = s->next;
                s->next = in_warnings;
                in_warnings = s0;
            }
        }

        // Look up the CDF info for this device (if any).
        oaLib *lib = design->getLib();
        oaScalarName cellName;
        design->getCellName(cellName);
        oaCell *cell = oaCell::find(lib, cellName);
        const cOAelecInfo *cdf = cOAprop::getCDFinfo(cell, in_def_symbol,
            in_def_dev_prop);

        // First set a P_NAME property if needed.
        CDp_sname *pname = (CDp_sname*)sdesc->prpty(P_NAME);
        if (!pname) {
            // The device lacks an instNamePrefix property.

            // This could be a "gnd" device, from Xic.  If so, it will
            // have exactly one node property and no subcells.
            CDp_node *pnd = (CDp_node*)sdesc->prpty(P_NODE);
            if (pnd && !pnd->next() && !sdesc->masters()) {
                // Is a gnd device, nothing more to do.
                return (true);
            }
            if (cdf && cdf->prefix() && *cdf->prefix()) {
                const char *pfx = cdf->prefix();
                pname = new CDp_sname;
                pname->set_name_string(pfx);
                pname->set_next_prp(sdesc->prptyList());
                sdesc->setPrptyList(pname);
            }
            else {
                // Assume a subcircuit here.
                pname = new CDp_sname;
                pname->set_name_string("X");
                pname->set_next_prp(sdesc->prptyList());
                sdesc->setPrptyList(pname);
            }
        }
        if (!pname->name_string() || pname->key() == P_NAME_NULL) {
            // A "null" device, no nodes.
            return (true);
        }
        int key = pname->key();

        // If the "subcircuit" has exactly one pin, and no instances,
        // assume that it is a terminal device.  It is probably one of
        // the analogLib terminals, which on import will behave like
        // an Xic terminal.  Note that the "gnd" device from analogLib
        // is actually a terminal connected to "gnd!", and NOT an Xic
        // gnd device.

        if (key == 'x' || key == 'X') {
            int numpins = 0;
            oaBlock *block = design->getTopBlock();
            oaIter<oaPin> pin_iter(block->getPins());
            while (pin_iter.getNext() != 0)
                numpins++;
            if (numpins == 1) {
                oaScalarName cellName;
                design->getCellName(cellName);
                oaString cn;
                cellName.get(cn);

                // Here's a hack, make the "noConn" device not
                // electrically active.

                bool nulldev = (cn == "noConn");

                // If there is no schematic view, the cell is a
                // terminal.  Otherwise, the schematic would have
                // been checked already as below (schematic view
                // is read first).

                oaScalarName libName;
                oaScalarName viewName(oaNativeNS(), "schematic");
                design->getLibName(libName);
                if (!oaDesign::exists(libName, cellName, viewName)) {
                    if (nulldev) {
                        pname->set_name_string(P_NAME_NULL_STR);
                        key = P_NAME_NULL;
                    }
                    else {
                        pname->set_name_string(P_NAME_TERM_STR);
                        key = P_NAME_TERM;
                    }
                }
                else {
                    int instcnt = 0;
                    oaIter<oaInst> inst_iter(block->getInsts());
                    while (inst_iter.getNext() != 0)
                        instcnt++;

                    if (instcnt == 0) {
                        if (nulldev) {
                            pname->set_name_string(P_NAME_NULL_STR);
                            key = P_NAME_NULL;
                        }
                        else {
                            pname->set_name_string(P_NAME_TERM_STR);
                            key = P_NAME_TERM;
                        }
                    }
                }
            }
        }
    }
    catch (oaCompatibilityError &ex) {
        cOA::handleFBCError(ex);
        ret = false;
    }
    catch (oaException &excp) {
        Errs()->add_error((const char*)excp.getMsg());
        ret = false;
    }
    if (!ret)
        Errs()->add_error("Failed reading cell (design) properties.");
    return (ret);
}


bool
oa_in::readOaLPPHeader(oaLPPHeader *header, CDs *sdesc)
{
    if (!header) {
        Errs()->add_error("Null LPP header encountered.");
        return (false);
    }
    if (!sdesc) {
        Errs()->add_error("Null cell pointer encountered.");
        return (false);
    }

    oaScalarName libName;
    header->getDesign()->getLibName(libName);
    CDl *ldesc = mapLayer(libName, header->getLayerNum(),
        header->getPurposeNum(), sdesc->displayMode());
    if (!ldesc) {
        Errs()->add_error("Failed to map LPP %d,%d to Xic layer.",
            header->getLayerNum(), header->getPurposeNum());
        return (false);
    }

    bool ret = true;
    oaIter<oaShape> shapes(header->getShapes());
    while (oaShape *shape = shapes.getNext()) {
        switch (shape->getType()) {
        case oacArcType:
            ret = readOaArc((oaArc*)shape, sdesc, ldesc);
            break;

        case oacDonutType:
            ret = readOaDonut((oaDonut*)shape, sdesc, ldesc);
            break;

        case oacDotType:
            ret = readOaDot((oaDot*)shape, sdesc, ldesc);
            break;

        case oacEllipseType:
            ret = readOaEllipse((oaEllipse*)shape, sdesc, ldesc);
            break;

        case oacLineType:
            ret = readOaLine((oaLine*)shape, sdesc, ldesc);
            break;

        case oacRectType:
            ret = readOaRect((oaRect*)shape, sdesc, ldesc);
            break;

        case oacPathType:
            ret = readOaPath((oaPath*)shape, sdesc, ldesc);
            break;

        case oacPathSegType:
            ret = readOaPathSeg((oaPathSeg*)shape, sdesc, ldesc);
            break;

        case oacPolygonType:
            ret = readOaPolygon((oaPolygon*)shape, sdesc, ldesc);
            break;

        case oacTextType:
            ret = readOaText((oaText*)shape, sdesc, ldesc);
            break;

        case oacAttrDisplayType:
            ret = readOaAttrDisplay((oaAttrDisplay*)shape, sdesc, ldesc);
            break;

        case oacPropDisplayType:
            ret = readOaPropDisplay((oaPropDisplay*)shape, sdesc, ldesc);
            break;

        case oacTextOverrideType:
            ret = readOaTextOverride((oaTextOverride*)shape, sdesc, ldesc);
            break;

        case oacEvalTextType:
            ret = readOaEvalText((oaEvalText*)shape, sdesc, ldesc);
            break;

        default:
            OAerrLog.add_log(OAlogLoad, "unhandled shape %s",
                (const char*)shape->getType().getName());
            break;
        }
        if (!ret)
            break;
    }
    return (ret);
}


bool
oa_in::readOaDot(oaDot *dot, CDs*, CDl*)
{
    bool ret = true;
    try {
        oaPoint p;
        dot->getLocation(p);
        // Ignore these for now.
    }
    catch (oaCompatibilityError &ex) {
        cOA::handleFBCError(ex);
        ret = false;
    }
    catch (oaException &excp) {
        Errs()->add_error((const char*)excp.getMsg());
        ret = false;
    }
    if (!ret)
        Errs()->add_error("Failed reading Dot.");
    return (ret);
}


bool
oa_in::readOaEllipse(oaEllipse *ellipse, CDs *sdesc, CDl *ldesc)
{
    bool ret = true;
    Poly po;
    try {
        oaBox box;
        ellipse->getBBox(box);
        bool is_elec = sdesc->isElectrical();
        if (is_elec && in_elec_scale != 1) {
            box.set(in_elec_scale*box.left(), in_elec_scale*box.bottom(),
                in_elec_scale*box.right(), in_elec_scale*box.top());
        }
        oaPointArray boundary;
        ellipse->genBoundary(box, GEO()->roundFlashSides(is_elec), boundary);

        po.numpts = boundary.getNumElements();
        po.points = new Point[po.numpts + 1];
        for (int i = 0; i < po.numpts; i++) {
            po.points[i].x = boundary[i].x();
            po.points[i].y = boundary[i].y();
        }
        po.points[po.numpts] = po.points[0];
        po.numpts++;
    }
    catch (oaCompatibilityError &ex) {
        cOA::handleFBCError(ex);
        ret = false;
    }
    catch (oaException &excp) {
        Errs()->add_error((const char*)excp.getMsg());
        ret = false;
    }
    if (!ret) {
        Errs()->add_error("Failed reading Ellipse.");
        return (false);
    }

    CDpo *newo;
    int pchk_flags;
    CDerrType err = sdesc->makePolygon(ldesc, &po, &newo, &pchk_flags);
    if (err != CDok) {
        Errs()->add_error("Failed to create database polygon.");
        return (false);
    }
    if (newo) {
        try {
            CDp *p0 = readProperties(ellipse);
            if (p0) {
                stringlist *s0 = sdesc->prptyApplyList(newo, &p0);
                CDp::destroy(p0);
                if (s0) {
                    stringlist *s = s0;
                    while (s->next)
                        s = s->next;
                    s->next = in_warnings;
                    in_warnings = s0;
                }
            }
        }
        catch (oaCompatibilityError &ex) {
            cOA::handleFBCError(ex);
            ret = false;
        }
        catch (oaException &excp) {
            Errs()->add_error((const char*)excp.getMsg());
            ret = false;
        }
        if (!ret) {
            Errs()->add_error("Failed reading Ellipse properties.");
            return (false);
        }
    }
    return (ret);
}


bool
oa_in::readOaArc(oaArc *arc, CDs *sdesc, CDl *ldesc)
{
    // Arc is a path using 0-width wires.
    bool ret = true;
    Wire w;
    try {
        oaBox box;
        arc->getEllipseBBox(box);
        bool is_elec = sdesc->isElectrical();
        if (is_elec && in_elec_scale != 1) {
            box.set(in_elec_scale*box.left(), in_elec_scale*box.bottom(),
                in_elec_scale*box.right(), in_elec_scale*box.top());
        }

        double as = arc->getStartAngle();
        double ae = arc->getStopAngle();
        int np;
        if (ae > as)
            np = (int)(0.5*GEO()->roundFlashSides(is_elec)*(ae - as)/M_PI) + 1;
        else
            np = (int)(GEO()->roundFlashSides(is_elec)*
                (1.0 - 0.5*(as - ae)/M_PI)) + 1;
        oaPointArray points;
        arc->genPoints(box, as, ae, np, points);

        w.numpts = points.getNumElements();
        w.points = new Point[w.numpts];
        for (int i = 0; i < w.numpts; i++) {
            w.points[i].x = points[i].x();
            w.points[i].y = points[i].y();
        }
        w.set_wire_width(0);
        w.set_wire_style(CDWIRE_FLUSH);
    }
    catch (oaCompatibilityError &ex) {
        cOA::handleFBCError(ex);
        ret = false;
    }
    catch (oaException &excp) {
        Errs()->add_error((const char*)excp.getMsg());
        ret = false;
    }
    if (!ret)
        Errs()->add_error("Failed reading Arc.");

    CDw *newo;
    CDerrType err = sdesc->makeWire(ldesc, &w, &newo, 0);
    if (err != CDok) {
        Errs()->add_error("Failed creating database wire for Line.");
        return (false);
    }
    if (newo) {
        try {
            CDp *p0 = readProperties(arc);
            if (p0) {
                stringlist *s0 = sdesc->prptyApplyList(newo, &p0);
                CDp::destroy(p0);
                if (s0) {
                    stringlist *s = s0;
                    while (s->next)
                        s = s->next;
                    s->next = in_warnings;
                    in_warnings = s0;
                }
            }
        }
        catch (oaCompatibilityError &ex) {
            cOA::handleFBCError(ex);
            ret = false;
        }
        catch (oaException &excp) {
            Errs()->add_error((const char*)excp.getMsg());
            ret = false;
        }
        if (!ret) {
            Errs()->add_error("Failed reading Arc properties.");
            return (false);
        }
    }

    return (true);
}


bool
oa_in::readOaDonut(oaDonut *donut, CDs *sdesc, CDl *ldesc)
{
    bool ret = true;
    Poly po;
    try {
        oaPoint center;
        donut->getCenter(center);
        int rad, hrad;
        bool is_elec = sdesc->isElectrical();
        if (is_elec && in_elec_scale != 1) {
            rad = in_elec_scale*donut->getRadius();
            hrad = in_elec_scale*donut->getHoleRadius();
            center.set(in_elec_scale*center.x(), in_elec_scale*center.y());
        }
        else {
            rad = donut->getRadius();
            hrad = donut->getHoleRadius();
        }
        oaPointArray boundary;
        const int num_sides = 2*GEO()->roundFlashSides(is_elec);
        donut->genBoundary(center, rad, hrad, num_sides, boundary);

        po.numpts = boundary.getNumElements();
        po.points = new Point[po.numpts + 1];
        for (int i = 0; i < po.numpts; i++) {
            po.points[i].x = boundary[i].x();
            po.points[i].y = boundary[i].y();
        }
        po.points[po.numpts] = po.points[0];
        po.numpts++;
    }
    catch (oaCompatibilityError &ex) {
        cOA::handleFBCError(ex);
        ret = false;
    }
    catch (oaException &excp) {
        Errs()->add_error((const char*)excp.getMsg());
        ret = false;
    }
    if (!ret)
        Errs()->add_error("Failed reading Donut.");

    CDpo *newo;
    int pchk_flags;
    CDerrType err = sdesc->makePolygon(ldesc, &po, &newo, &pchk_flags);
    if (err != CDok) {
        Errs()->add_error("Failed to create database polygon.");
        return (false);
    }
    if (newo) {
        try {
            CDp *p0 = readProperties(donut);
            if (p0) {
                stringlist *s0 = sdesc->prptyApplyList(newo, &p0);
                CDp::destroy(p0);
                if (s0) {
                    stringlist *s = s0;
                    while (s->next)
                        s = s->next;
                    s->next = in_warnings;
                    in_warnings = s0;
                }
            }
        }
        catch (oaCompatibilityError &ex) {
            cOA::handleFBCError(ex);
            ret = false;
        }
        catch (oaException &excp) {
            Errs()->add_error((const char*)excp.getMsg());
            ret = false;
        }
        if (!ret) {
            Errs()->add_error("Failed reading Donut properties.");
            return (false);
        }
    }
    return (ret);
}


bool
oa_in::readOaLine(oaLine *line, CDs *sdesc, CDl *ldesc)
{
    // Lines are 0-width wires.
    bool ret = true;
    Wire w;
    try {
        oaPointArray points;
        line->getPoints(points);
        w.numpts = points.getNumElements();
        w.points = new Point[w.numpts];
        if (sdesc->isElectrical() && in_elec_scale != 1) {
            for (int i = 0; i < w.numpts; i++) {
                w.points[i].x = in_elec_scale*points[i].x();
                w.points[i].y = in_elec_scale*points[i].y();
            }
        }
        else {
            for (int i = 0; i < w.numpts; i++) {
                w.points[i].x = points[i].x();
                w.points[i].y = points[i].y();
            }
        }
        w.set_wire_width(0);
        w.set_wire_style(CDWIRE_FLUSH);
    }
    catch (oaCompatibilityError &ex) {
        cOA::handleFBCError(ex);
        ret = false;
    }
    catch (oaException &excp) {
        Errs()->add_error((const char*)excp.getMsg());
        ret = false;
    }
    if (!ret) {
        Errs()->add_error("Failed reading Line.");
        return (false);
    }

    CDw *newo;
    CDerrType err = sdesc->makeWire(ldesc, &w, &newo, 0);
    if (err != CDok) {
        Errs()->add_error("Failed creating database wire for Line.");
        return (false);
    }
    if (newo) {
        try {
            CDp *p0 = readProperties(line);
            if (p0) {
                stringlist *s0 = sdesc->prptyApplyList(newo, &p0);
                CDp::destroy(p0);
                if (s0) {
                    stringlist *s = s0;
                    while (s->next)
                        s = s->next;
                    s->next = in_warnings;
                    in_warnings = s0;
                }
            }
        }
        catch (oaCompatibilityError &ex) {
            cOA::handleFBCError(ex);
            ret = false;
        }
        catch (oaException &excp) {
            Errs()->add_error((const char*)excp.getMsg());
            ret = false;
        }
        if (!ret) {
            Errs()->add_error("Failed reading Line properties.");
            return (false);
        }
    }

    return (true);
}


namespace {
    void convert_4to0(oaPoint &pend, oaPoint &pn, int extn)
    {
        if (!extn)
            return;
        if (pend.x() == pn.x()) {
            if (pend.y() > pn.y())
                pend.set(pend.x(), pend.y() + extn);
            else
                pend.set(pend.x(), pend.y() - extn);
        }
        else if (pend.y() == pn.y()) {
            if (pend.x() > pn.x())
                pend.set(pend.x() + extn, pend.y());
            else
                pend.set(pend.x() - extn, pend.y());
        }
        else {
            double dx = (double)(pend.x() - pn.x());
            double dy = (double)(pend.y() - pn.y());
            double d = sqrt(dx*dx + dy*dy);
            d = extn/d;
            pend.set(mmRnd(pend.x() + dx*d), mmRnd(pend.y() + dy*d));
        }
    }
}


bool
oa_in::readOaPath(oaPath *path, CDs *sdesc, CDl *ldesc)
{
    bool ret = true;
    Wire w;
    try {
        oaPointArray points;
        path->getPoints(points);
        int np = points.getNumElements();

        WireStyle style = CDWIRE_FLUSH;
        if (path->getStyle() == oacExtendPathStyle)
            style = CDWIRE_EXTEND;
        else if (path->getStyle() == oacRoundPathStyle)
            style = CDWIRE_ROUND;
        else if (path->getStyle() == oacVariablePathStyle) {
            // Convert to flush ends.
            oaInt4 bgnExt = path->getBeginExt();
            oaInt4 endExt = path->getEndExt();
            convert_4to0(points[0], points[1], bgnExt);
            convert_4to0(points[np-1], points[np-2], endExt);
        }

        w.numpts = np;
        w.points = new Point[w.numpts];
        if (sdesc->isElectrical() && in_elec_scale != 1) {
            for (int i = 0; i < w.numpts; i++) {
                w.points[i].x = in_elec_scale*points[i].x();
                w.points[i].y = in_elec_scale*points[i].y();
            }
            // This should always be zero?
            // "Fat" wires?
            w.set_wire_width(in_elec_scale*path->getWidth());
        }
        else {
            for (int i = 0; i < w.numpts; i++) {
                w.points[i].x = points[i].x();
                w.points[i].y = points[i].y();
            }
            w.set_wire_width(path->getWidth());
        }
        w.set_wire_style(style);
    }
    catch (oaCompatibilityError &ex) {
        cOA::handleFBCError(ex);
        ret = false;
    }
    catch (oaException &excp) {
        Errs()->add_error((const char*)excp.getMsg());
        ret = false;
    }
    if (!ret) {
        Errs()->add_error("Failed reading Path.");
        return (false);
    }

    CDw *newo;
    int wchk_flags;
    CDerrType err = sdesc->makeWire(ldesc, &w, &newo, &wchk_flags);
    if (err != CDok) {
        Errs()->add_error("Failed creating database wire for Path.");
        return (false);
    }
    if (newo) {
        try {
            CDp *p0 = readProperties(path);
            if (p0) {
                stringlist *s0 = sdesc->prptyApplyList(newo, &p0);
                CDp::destroy(p0);
                if (s0) {
                    stringlist *s = s0;
                    while (s->next)
                        s = s->next;
                    s->next = in_warnings;
                    in_warnings = s0;
                }
            }
        }
        catch (oaCompatibilityError &ex) {
            cOA::handleFBCError(ex);
            ret = false;
        }
        catch (oaException &excp) {
            Errs()->add_error((const char*)excp.getMsg());
            ret = false;
        }
        if (!ret) {
            Errs()->add_error("Failed reading Path properties.");
            return (false);
        }
    }

    return (true);
}


bool
oa_in::readOaPathSeg(oaPathSeg *seg, CDs *sdesc, CDl *ldesc)
{
    bool ret = true;
    bool custom = false;
    oaSegStyle style;
    Poly po;
    Wire w;

    try {
        seg->getStyle(style);

        oaEndStyle bgn = style.getBeginStyle();
        oaEndStyle end = style.getEndStyle();

        // Convert custom end segments to polygons.
        custom = (bgn == oacCustomEndStyle || end == oacCustomEndStyle
            || bgn == oacChamferEndStyle || end == oacChamferEndStyle);

        if (custom) {
            // If the beginStyle or endStyle is custom or chamfer,
            // output the pathSeg as a boundary object.
            oaPointArray points;
            seg->getBoundary(points);

            po.numpts = points.getNumElements();
            po.points = new Point[po.numpts];
            if (sdesc->isElectrical() && in_elec_scale != 1) {
                for (int i = 0; i < po.numpts; i++) {
                    po.points[i].x = in_elec_scale*points[i].x();
                    po.points[i].y = in_elec_scale*points[i].y();
                }
            }
            else {
                for (int i = 0; i < po.numpts; i++) {
                    po.points[i].x = points[i].x();
                    po.points[i].y = points[i].y();
                }
            }
        }
        else {
            // All other pathSegs will be output as a 2 point path with flush
            // ends.
            oaPoint p[2];
            seg->getPoints(p[0], p[1]);
            oaDist width = style.getWidth();

            if (bgn == oacExtendEndStyle)
                convert_4to0(p[0], p[1], width/2);
            else if (bgn == oacVariableEndStyle)
                convert_4to0(p[0], p[1], style.getBeginExt());

            if (end == oacExtendEndStyle)
                convert_4to0(p[1], p[0], width/2);
            else if (end == oacVariableEndStyle)
                convert_4to0(p[1], p[0], style.getEndExt());

            w.numpts = 2;
            w.points = new Point[w.numpts];
            if (sdesc->isElectrical() && in_elec_scale != 1) {
                for (int i = 0; i < w.numpts; i++) {
                    w.points[i].x = in_elec_scale*p[i].x();
                    w.points[i].y = in_elec_scale*p[i].y();
                }
                // This should always be zero?
                // "Fat" wires?
                w.set_wire_width(in_elec_scale*width);
            }
            else {
                for (int i = 0; i < w.numpts; i++) {
                    w.points[i].x = p[i].x();
                    w.points[i].y = p[i].y();
                }
                w.set_wire_width(width);
            }
            w.set_wire_style(CDWIRE_FLUSH);
        }
    }
    catch (oaCompatibilityError &ex) {
        cOA::handleFBCError(ex);
        ret = false;
    }
    catch (oaException &excp) {
        Errs()->add_error((const char*)excp.getMsg());
        ret = false;
    }
    if (!ret) {
        Errs()->add_error("Failed reading PathSeg.");
        return (false);
    }

    CDo *newo;
    if (custom) {
        int pchk_flags;
        CDerrType err = sdesc->makePolygon(ldesc, &po, (CDpo**)&newo,
            &pchk_flags);
        if (err != CDok) {
            Errs()->add_error(
                "Failed to create database polygon for PathSeg.");
            return (false);
        }
    }
    else {
        int wchk_flags;
        CDerrType err = sdesc->makeWire(ldesc, &w, (CDw**)&newo, &wchk_flags,
            false);
        if (err != CDok) {
            Errs()->add_error("Failed to create database wire for PathSeg.");
            return (false);
        }
    }
    if (newo) {
        try {
            CDp *p0 = readProperties(seg);
            if (p0) {
                stringlist *s0 = sdesc->prptyApplyList(newo, &p0);
                CDp::destroy(p0);
                if (s0) {
                    stringlist *s = s0;
                    while (s->next)
                        s = s->next;
                    s->next = in_warnings;
                    in_warnings = s0;
                }
            }
        }
        catch (oaCompatibilityError &ex) {
            cOA::handleFBCError(ex);
            ret = false;
        }
        catch (oaException &excp) {
            Errs()->add_error((const char*)excp.getMsg());
            ret = false;
        }
        if (!ret) {
            Errs()->add_error("Failed reading Path properties.");
            return (false);
        }
    }

    return (true);
}


bool
oa_in::readOaPolygon(oaPolygon *polygon, CDs *sdesc, CDl *ldesc)
{
    bool ret = true;
    Poly po;
    try {
        oaPointArray points;
        polygon->getPoints(points);

        po.numpts = points.getNumElements();
        po.points = new Point[po.numpts + 1];
        if (sdesc->isElectrical() && in_elec_scale != 1) {
            for (int i = 0; i < po.numpts; i++) {
                po.points[i].x = in_elec_scale*points[i].x();
                po.points[i].y = in_elec_scale*points[i].y();
            }
        }
        else {
            for (int i = 0; i < po.numpts; i++) {
                po.points[i].x = points[i].x();
                po.points[i].y = points[i].y();
            }
        }
        po.points[po.numpts] = po.points[0];
        po.numpts++;
    }
    catch (oaCompatibilityError &ex) {
        cOA::handleFBCError(ex);
        ret = false;
    }
    catch (oaException &excp) {
        Errs()->add_error((const char*)excp.getMsg());
        ret = false;
    }
    if (!ret) {
        Errs()->add_error("Failed reading Polygon.");
        return (false);
    }

    CDpo *newo;
    int pchk_flags;
    CDerrType err = sdesc->makePolygon(ldesc, &po, &newo, &pchk_flags);
    if (err != CDok) {
        Errs()->add_error("Failed to create database polygon.");
        return (false);
    }
    if (newo) {
        try {
            CDp *p0 = readProperties(polygon);
            if (p0) {
                stringlist *s0 = sdesc->prptyApplyList(newo, &p0);
                CDp::destroy(p0);
                if (s0) {
                    stringlist *s = s0;
                    while (s->next)
                        s = s->next;
                    s->next = in_warnings;
                    in_warnings = s0;
                }
            }
        }
        catch (oaCompatibilityError &ex) {
            cOA::handleFBCError(ex);
            ret = false;
        }
        catch (oaException &excp) {
            Errs()->add_error((const char*)excp.getMsg());
            ret = false;
        }
        if (!ret) {
            Errs()->add_error("Failed reading Polygon properties.");
            return (false);
        }
    }

    return (true);
}


bool
oa_in::readOaRect(oaRect *rect, CDs *sdesc, CDl *ldesc)
{
    bool ret = true;
    oaBox box;
    rect->getBBox(box);

    BBox BB(box.left(), box.bottom(), box.right(), box.top());

    if (sdesc->isElectrical() && in_elec_scale != 1) {
        BB = BBox(in_elec_scale*box.left(), in_elec_scale*box.bottom(),
            in_elec_scale*box.right(), in_elec_scale*box.top());
    }

    CDo *newo;
    CDerrType err = sdesc->makeBox(ldesc, &BB, &newo);
    if (err != CDok) {
        Errs()->add_error("Failed to create database rectangle.");
        return (false);
    }
    if (newo) {
        try {
            CDp *p0 = readProperties(rect);
            if (p0) {
                stringlist *s0 = sdesc->prptyApplyList(newo, &p0);
                CDp::destroy(p0);
                if (s0) {
                    stringlist *s = s0;
                    while (s->next)
                        s = s->next;
                    s->next = in_warnings;
                    in_warnings = s0;
                }
            }
        }
        catch (oaCompatibilityError &ex) {
            cOA::handleFBCError(ex);
            ret = false;
        }
        catch (oaException &excp) {
            Errs()->add_error((const char*)excp.getMsg());
            ret = false;
        }
        if (!ret) {
            Errs()->add_error("Failed reading Rect properties.");
            return (false);
        }
    }

    return (true);
}


namespace {
    void set_orient(int *xfp, oaOrient orient)
    {
        int xf = 0;
        if (orient == oacR90)
            xf = 1;
        else if (orient == oacR180)
            xf = 2;
        else if (orient == oacR270)
            xf = 3;
        else if (orient == oacMY)
            xf = TXTF_MX;
        else if (orient == oacMYR90)
            xf = 1 | TXTF_MY;
        else if (orient == oacMX)
            xf = TXTF_MY;
        else if (orient == oacMXR90)
            xf = 1 | TXTF_MX;
        *xfp |= xf;
    }


    void set_alignment(int *ap, oaTextAlign align)
    {
        int a = 0;
        if (align == oacUpperLeftTextAlign)
            a = TXTF_VJT;
        else if (align == oacCenterLeftTextAlign)
            a = TXTF_VJC;
        else if (align == oacLowerLeftTextAlign)
            a = 0;
        else if (align == oacUpperCenterTextAlign)
            a = TXTF_HJC | TXTF_VJT;
        else if (align == oacCenterCenterTextAlign)
            a = TXTF_HJC | TXTF_VJC;
        else if (align == oacLowerCenterTextAlign)
            a = TXTF_HJC;
        else if (align == oacUpperRightTextAlign)
            a = TXTF_HJR | TXTF_VJT;
        else if (align == oacCenterRightTextAlign)
            a = TXTF_HJR | TXTF_VJC;
        else if (align == oacLowerRightTextAlign)
            a = TXTF_HJR;
        *ap |= a;
    }
}


bool
oa_in::readOaText(oaText *text, CDs *sdesc, CDl *ldesc)
{
    // Ignore invisible text.
    if (!text->isVisible())
        return (true);

    // Ignore the overbar flag, may wnat to implement this some day.
    // if (text->hasOverbar()) {
    // }

    bool ret = true;
    Label la;
    try {
        oaString data;
        text->getText(data);
        if (data.isEmpty()) {
            // Empty label, ignore.
            return (true);
        }

        oaPoint origin;
        text->getOrigin(origin);
        la.x = origin.x();
        la.y = origin.y();
        la.height = text->getHeight();
        if (la.height <= 0) {
            // Label has no size, ignore these.
            return (true);
        }
        if (sdesc->isElectrical() && in_elec_scale != 1) {
            la.x *= in_elec_scale;
            la.y *= in_elec_scale;
            la.height *= in_elec_scale;
        }
        la.height = (int)(la.height * CDS_TEXT_SCALE);

        la.label = new hyList(sdesc, data, HYcvAscii);

        double tw, th;
        int nl = CD()->DefaultLabelSize(data, in_mode, &tw, &th);
        la.height *= nl;
        la.width = (int)((la.height * tw)/th);

        int xform = 0;
        set_orient(&xform, text->getOrient());
        set_alignment(&xform, text->getAlignment());
        la.xform = xform;
    }
    catch (oaCompatibilityError &ex) {
        cOA::handleFBCError(ex);
        ret = false;
    }
    catch (oaException &excp) {
        Errs()->add_error((const char*)excp.getMsg());
        ret = false;
    }
    if (!ret) {
        Errs()->add_error("Failed reading Text.");
        return (false);
    }

    CDla *newo;
    CDerrType err = sdesc->makeLabel(ldesc, &la, &newo);
    if (err != CDok) {
        Errs()->add_error("Failed to create database label.");
        return (false);
    }
    if (newo) {
        if (oaProp::find(text, "XICP_NO_INST_VIEW")) {
            // The label will only be visible when its containing cell
            // is top-level.
            newo->set_no_inst_view(true);
        }
        if (oaProp::find(text, "XICP_USE_LINE_LIMIT")) {
            // The label will allow limiting the number of lines shown.
            newo->set_use_line_limit(true);
        }

        try {
            CDp *p0 = readProperties(text);
            if (p0) {
                stringlist *s0 = sdesc->prptyApplyList(newo, &p0);
                CDp::destroy(p0);
                if (s0) {
                    stringlist *s = s0;
                    while (s->next)
                        s = s->next;
                    s->next = in_warnings;
                    in_warnings = s0;
                }
            }
        }
        catch (oaCompatibilityError &ex) {
            cOA::handleFBCError(ex);
            ret = false;
        }
        catch (oaException &excp) {
            Errs()->add_error((const char*)excp.getMsg());
            ret = false;
        }
        if (!ret) {
            Errs()->add_error("Failed reading Text properties.");
            return (false);
        }
    }

    return (true);
}


bool
oa_in::readOaAttrDisplay(oaAttrDisplay *attrDisplay, CDs *sdesc, CDl *ldesc)
{
    // Ignore invisible text.
    if (!attrDisplay->isVisible())
        return (true);

    // Ignore the overbar flag, may wnat to implement this some day.
    // if (attrDisplay->hasOverbar()) {
    // }

    bool ret = true;
    Label la;
    try {
        oaString data;
        attrDisplay->getText(in_ns, data);
        if (data.isEmpty()) {
            // Empty label, ignore.
            return (true);
        }

        oaPoint origin;
        attrDisplay->getOrigin(origin);
        la.x = origin.x();
        la.y = origin.y();
        la.height = attrDisplay->getHeight();
        if (la.height <= 0) {
            // Label has no size, ignore these.
            return (true);
        }
        if (sdesc->isElectrical() && in_elec_scale != 1) {
            la.x *= in_elec_scale;
            la.y *= in_elec_scale;
            la.height *= in_elec_scale;
        }
        la.height = (int)(la.height * CDS_TEXT_SCALE);

        la.label = new hyList(sdesc, data, HYcvAscii);

        if (OAerrLog.debug_load()) {
            oaObject *obj = attrDisplay->getObject();
            oaType ot = obj->getType();
            oaString tn = ot.getName();
            oaString aname;

            switch (ot) {
            case oacDesignType:
                {
                    oaDesignAttrType at =
                        (oaDesignAttrType)attrDisplay->getAttribute();
                    aname = at.getName();
                }
                break;
            case oacAssignAssignmentType:
            case oacAssignValueType:
                {
                    oaAssignmentAttrType at =
                        (oaAssignmentAttrType)attrDisplay->getAttribute();
                    aname = at.getName();
                }
                break;
            case oacVectorInstType:
            case oacArrayInstType:
            case oacScalarInstType:
            case oacVectorInstBitType:
                {
                    oaInstAttrType at =
                        (oaInstAttrType)attrDisplay->getAttribute();
                    aname = at.getName();
                }
                break;
            case oacInstTermType:
                {
                    oaInstTermAttrType at =
                        (oaInstTermAttrType)attrDisplay->getAttribute();
                    aname = at.getName();
                }
                break;
            case oacBundleNetType:
            case oacBusNetType:
            case oacBusNetBitType:
            case oacScalarNetType:
                {
                    oaNetAttrType at =
                        (oaNetAttrType)attrDisplay->getAttribute();
                    aname = at.getName();
                }
                break;
            case oacBundleTermType:
            case oacBusTermType:
            case oacBusTermBitType:
            case oacScalarTermType:
                {
                    oaTermAttrType at =
                        (oaTermAttrType)attrDisplay->getAttribute();
                    aname = at.getName();
                }
                break;
            default:
                aname = "???";
                break;
            }
            OAerrLog.add_log(OAlogLoad, "attrDisplay %s : %s %s",
                (const char*)data, (const char*)tn, (const char*)aname);
        }

        double tw, th;
        int nl = CD()->DefaultLabelSize(data, in_mode, &tw, &th);
        la.height *= nl;
        la.width = (int)((la.height * tw)/th);

        int xform = 0;
        set_orient(&xform, attrDisplay->getOrient());
        set_alignment(&xform, attrDisplay->getAlignment());
        la.xform = xform;
    }
    catch (oaCompatibilityError &ex) {
        cOA::handleFBCError(ex);
        ret = false;
    }
    catch (oaException &excp) {
        Errs()->add_error((const char*)excp.getMsg());
        ret = false;
    }
    if (!ret) {
        Errs()->add_error("Failed reading AttrDisplay.");
        return (false);
    }

    CDla *newo;
    CDerrType err = sdesc->makeLabel(ldesc, &la, &newo);
    if (err != CDok) {
        Errs()->add_error("Failed to create database label for AttrDisplay.");
        return (false);
    }
    if (newo) {
        // The label will only be visible when its containing cell
        // is top-level.
        newo->set_no_inst_view(true);

        try {
            CDp *p0 = readProperties(attrDisplay);
            if (p0) {
                stringlist *s0 = sdesc->prptyApplyList(newo, &p0);
                CDp::destroy(p0);
                if (s0) {
                    stringlist *s = s0;
                    while (s->next)
                        s = s->next;
                    s->next = in_warnings;
                    in_warnings = s0;
                }
            }
        }
        catch (oaCompatibilityError &ex) {
            cOA::handleFBCError(ex);
            ret = false;
        }
        catch (oaException &excp) {
            Errs()->add_error((const char*)excp.getMsg());
            ret = false;
        }
        if (!ret) {
            Errs()->add_error("Failed reading Text properties.");
            return (false);
        }
    }

    return (true);
}


bool
oa_in::readOaPropDisplay(oaPropDisplay *propDisplay, CDs *sdesc, CDl *ldesc)
{
    // Ignore invisible text.
    if (!propDisplay->isVisible())
        return (true);

    // Ignore the overbar flag, may wnat to implement this some day.
    // if (propDisplay->hasOverbar()) {
    // }

    bool ret = true;
    Label la;
    try {
        oaString data;
        propDisplay->getText(data);
        if (data.isEmpty()) {
            // Empty label, ignore.
            return (true);
        }

        oaPoint origin;
        propDisplay->getOrigin(origin);
        la.x = origin.x();
        la.y = origin.y();
        la.height = propDisplay->getHeight();
        if (la.height <= 0) {
            // Label has no size, ignore these.
            return (true);
        }
        if (sdesc->isElectrical() && in_elec_scale != 1) {
            la.x *= in_elec_scale;
            la.y *= in_elec_scale;
            la.height *= in_elec_scale;
        }
        la.height = (int)(la.height * CDS_TEXT_SCALE);

        la.label = new hyList(sdesc, data, HYcvAscii);

        if (OAerrLog.debug_load()) {
            oaProp *prop = propDisplay->getProp();
            oaString pn, pv;
            prop->getName(pn);
            prop->getValue(pv);
            OAerrLog.add_log(OAlogLoad, "propDisplay %s : %s %s",
                (const char*)data, (const char*)pn, (const char*)pv);
        }

        double tw, th;
        int nl = CD()->DefaultLabelSize(data, in_mode, &tw, &th);
        la.height *= nl;
        la.width = (int)((la.height * tw)/th);

        int xform = 0;
        set_orient(&xform, propDisplay->getOrient());
        set_alignment(&xform, propDisplay->getAlignment());
        la.xform = xform;
    }
    catch (oaCompatibilityError &ex) {
        cOA::handleFBCError(ex);
        ret = false;
    }
    catch (oaException &excp) {
        Errs()->add_error((const char*)excp.getMsg());
        ret = false;
    }
    if (!ret) {
        Errs()->add_error("Failed reading PropDisplay.");
        return (false);
    }

    CDla *newo;
    CDerrType err = sdesc->makeLabel(ldesc, &la, &newo);
    if (err != CDok) {
        Errs()->add_error("Failed to create database label for PropDisplay.");
        return (false);
    }
    if (newo) {
        // The label will only be visible when its containing cell
        // is top-level.
        newo->set_no_inst_view(true);

        try {
            CDp *p0 = readProperties(propDisplay);
            if (p0) {
                stringlist *s0 = sdesc->prptyApplyList(newo, &p0);
                CDp::destroy(p0);
                if (s0) {
                    stringlist *s = s0;
                    while (s->next)
                        s = s->next;
                    s->next = in_warnings;
                    in_warnings = s0;
                }
            }
        }
        catch (oaCompatibilityError &ex) {
            cOA::handleFBCError(ex);
            ret = false;
        }
        catch (oaException &excp) {
            Errs()->add_error((const char*)excp.getMsg());
            ret = false;
        }
        if (!ret) {
            Errs()->add_error("Failed reading Text properties.");
            return (false);
        }
    }

    return (true);
}


bool
oa_in::readOaTextOverride(oaTextOverride *textOverride, CDs *sdesc, CDl *ldesc)
{
    // This overrides text presentation attributes of an existing
    // label in an instance master.  This isn't currently supported,
    // so just return.
    //
    return (true);

    // Ignore invisible text.
    if (!textOverride->isVisible())
        return (true);

    // Ignore the overbar flag, may wnat to implement this some day.
    // if (textOverride->hasOverbar()) {
    // }

    bool ret = true;
    Label la;
    try {
        oaString data;
        textOverride->getText(data);
        if (data.isEmpty()) {
            // Empty label, ignore.
            return (true);
        }

        oaPoint origin;
        textOverride->getOrigin(origin);
        la.x = origin.x();
        la.y = origin.y();
        la.height = textOverride->getHeight();
        if (la.height <= 0) {
            // Label has no size, ignore these.
            return (true);
        }
        if (sdesc->isElectrical() && in_elec_scale != 1) {
            la.x *= in_elec_scale;
            la.y *= in_elec_scale;
            la.height *= in_elec_scale;
        }
        la.height = (int)(la.height * CDS_TEXT_SCALE);

        la.label = new hyList(sdesc, data, HYcvAscii);

        OAerrLog.add_log(OAlogLoad, "textOverride %s", (const char*)data);

        double tw, th;
        int nl = CD()->DefaultLabelSize(data, in_mode, &tw, &th);
        la.height *= nl;
        la.width = (int)((la.height * tw)/th);

        int xform = 0;
        set_orient(&xform, textOverride->getOrient());
        set_alignment(&xform, textOverride->getAlignment());
        la.xform = xform;
    }
    catch (oaCompatibilityError &ex) {
        cOA::handleFBCError(ex);
        ret = false;
    }
    catch (oaException &excp) {
        Errs()->add_error((const char*)excp.getMsg());
        ret = false;
    }
    if (!ret) {
        Errs()->add_error("Failed reading TextOverride.");
        return (false);
    }

    CDla *newo;
    CDerrType err = sdesc->makeLabel(ldesc, &la, &newo);
    if (err != CDok) {
        Errs()->add_error("Failed to create database label for TextOverride.");
        return (false);
    }
    if (newo) {
        // The label will only be visible when its containing cell
        // is top-level.
        newo->set_no_inst_view(true);

        try {
            CDp *p0 = readProperties(textOverride);
            if (p0) {
                stringlist *s0 = sdesc->prptyApplyList(newo, &p0);
                CDp::destroy(p0);
                if (s0) {
                    stringlist *s = s0;
                    while (s->next)
                        s = s->next;
                    s->next = in_warnings;
                    in_warnings = s0;
                }
            }
        }
        catch (oaCompatibilityError &ex) {
            cOA::handleFBCError(ex);
            ret = false;
        }
        catch (oaException &excp) {
            Errs()->add_error((const char*)excp.getMsg());
            ret = false;
        }
        if (!ret) {
            Errs()->add_error("Failed reading Text properties.");
            return (false);
        }
    }

    return (true);
}


// Here begins some pretty ugly stuff.  The api-major 5 is not drop-in
// compatible with api-major 4, which is what we have access to. 
// Virtuoso 6.1.7 uses api-major 5, which is presently a coalition
// release, requiring a hefty payment to join the coalition to gain
// access to the source and headers.  This would still be ugly, as we
// would need separate plugins for api-major 4 and 5.
//
// One can call the initialization function patching in the "5", and
// things almost work.  There is a seg fault when handling oaEvalText,
// however, but if this is commented out I can read the memory chip
// databases apparently without error.
//
// I found the oa-22.50 docs on-line (supposedly only available to
// coalition members).  This explains the problem:
//
// ---- from docs ----
// The IBase class now defines a pure virtual getRefCount function and
// a pure virtual destructor.  Applications that create their own
// plugins and which derive from the IBase class must declare and
// define concrete implementations of these functions.

// Past OpenAccess releases (since the first OA 2.2 release) have
// suffered from memory leaks due to the missing pure virtual
// destructor in the oaPlugIn::IBase class definition.  This could not
// be added earlier as it would break our drop-in compatibility
// guarantee.  Without this destructor definition the order of the
// destructor calls in derived classes was not done in a strict most
// derived to least derived order.  In fact, some derived classes
// would not have their destructors called at all.

// With this change a number of previously existing memory leaks in
// OpenAccess have been fixed.  The new pure virtual getRefCount
// interface is intended to help derived class destructors determine
// their correct course of action.
// ----
//
// So the IBase class, parent of IEvalText, has changed.  Here. we
// set up an equivalent, which we can use to support api-major 5.

// This is the same is IBase in api-major 5.
class IBase5 {
public:
    virtual                 ~IBase5() = 0;
    virtual long            queryInterface(const Guid   &id,
                                           void         **iPtr) = 0;
    virtual unsigned long   addRef() = 0;
    virtual unsigned long   release() = 0;
    virtual unsigned long   getRefCount() = 0;
    static const Guid       &getId();
    
    static bool             s_refCountDisabled;
};
          
// A special IEvalText.
class IEvalText5 : public IBase5 {
public:
    virtual                 ~IEvalText5() { }
                                     
    virtual void            getName(oaString &name) = 0;
                                    
    virtual void            onEval(const oaString   &textIn,
                                   oaString         &textOut) = 0;
  
    static const Guid       &getId() { return (IEvalText::getId()); }
};


namespace {
    oaEvalTextLink *evTextLink;
    const char *part_name;
    const char *cell_name;

    void on_eval(const oaString &textIn, oaString &textOut)
    {
        if (textIn == "[@partName]") {
            if (part_name)
                textOut = part_name;
            else
                return;
        }
        else if (textIn == "[@instanceName]") {
        }
        else if (textIn == "[@cellName]") {
            if (cell_name)
                textOut = cell_name;
            else
                return;
        }
        else if (lstring::prefix("cdsTerm(", textIn)) {
        }
        else if (lstring::prefix("cdsParam(", textIn)) {
        }
        else if (lstring::prefix("cdsName(", textIn)) {
        }
    }


    // For api-major 4.
    class myIEvalText4 : public IEvalText
    {
    public:
        unsigned long addRef()
            {
                return (0);
            }

        long queryInterface(const Guid&, void **ptr)
            {
                *ptr = 0;
                return (IBase::cNoInterface);
            }

        unsigned long release()
            {
                return (0);
            }

        void getName(oaString &name)
            {
                name = "XicEvalText";
            }

        void onEval(const oaString &textIn, oaString &textOut)
            {
                on_eval(textIn, textOut);
            }

#if oacAPIMajorRevNumber >= 5
        // Building on app-major 5, can we provide service to
        // app-major 4 OA installations?  This entry shouldn't exist
        // in that case, hopefully its presence would be benign.

        unsigned long getRefCount()
            {
                return (0);
            }
#endif
    };

    // For api-major 5.
    class myIEvalText5 : public IEvalText5
    {
    public:
        ~myIEvalText5() { }

        unsigned long addRef()
            {
                return (0);
            }

        long queryInterface(const Guid&, void **ptr)
            {
                *ptr = 0;
                return (IBase::cNoInterface);
            }

        unsigned long release()
            {
                return (0);
            }

        unsigned long getRefCount()
            {
                return (0);
            }

        void getName(oaString &name)
            {
                name = "XicEvalText";
            }

        void onEval(const oaString &textIn, oaString &textOut)
            {
                on_eval(textIn, textOut);
            }
    };
}


bool
oa_in::readOaEvalText(oaEvalText *evalText, CDs *sdesc, CDl *ldesc)
{
    // Ignore invisible text.
    if (!evalText->isVisible())
        return (true);

    CDp *prp = sdesc->prpty(XICP_PARTNAME);
    part_name = prp ? prp->string() : 0;

    // The seems to be redundant, same as model property string.
    cell_name = Tstring(sdesc->cellname());

    if (!evTextLink) {
        if (in_api_major <= 4)
            evTextLink = oaEvalTextLink::create((IEvalText*)new myIEvalText4);
        else
            evTextLink = oaEvalTextLink::create((IEvalText*)new myIEvalText5);
    }
    evalText->setLink(evTextLink);

    // Ignore the overbar flag, may want to implement this some day.
    // if (evalText->hasOverbar()) {
    // }

    bool ret = true;
    Label la;
    try {
        oaString data;
        evalText->getText(data);
        if (data.isEmpty()) {
            // Empty label, ignore.
            return (true);
        }

        oaPoint origin;
        evalText->getOrigin(origin);
        la.x = origin.x();
        la.y = origin.y();
        la.height = evalText->getHeight();
        if (la.height <= 0) {
            // Label has no size, ignore these.
            return (true);
        }
        if (sdesc->isElectrical() && in_elec_scale != 1) {
            la.x *= in_elec_scale;
            la.y *= in_elec_scale;
            la.height *= in_elec_scale;
        }
        la.height = (int)(la.height * CDS_TEXT_SCALE);

        la.label = new hyList(sdesc, data, HYcvAscii);

        OAerrLog.add_log(OAlogLoad, "evalText %s", (const char*)data);

        double tw, th;
        int nl = CD()->DefaultLabelSize(data, in_mode, &tw, &th);
        la.height *= nl;
        la.width = (int)((la.height * tw)/th);

        int xform = 0;
        set_orient(&xform, evalText->getOrient());
        set_alignment(&xform, evalText->getAlignment());
        la.xform = xform;
    }
    catch (oaCompatibilityError &ex) {
        cOA::handleFBCError(ex);
        ret = false;
    }
    catch (oaException &excp) {
        Errs()->add_error((const char*)excp.getMsg());
        ret = false;
    }
    if (!ret) {
        Errs()->add_error("Failed reading TextOverride.");
        return (false);
    }

    CDla *newo;
    CDerrType err = sdesc->makeLabel(ldesc, &la, &newo);
    if (err != CDok) {
        Errs()->add_error("Failed to create database label for TextOverride.");
        return (false);
    }
    if (newo) {
        try {
            CDp *p0 = readProperties(evalText);
            if (p0) {
                stringlist *s0 = sdesc->prptyApplyList(newo, &p0);
                CDp::destroy(p0);
                if (s0) {
                    stringlist *s = s0;
                    while (s->next)
                        s = s->next;
                    s->next = in_warnings;
                    in_warnings = s0;
                }
            }
        }
        catch (oaCompatibilityError &ex) {
            cOA::handleFBCError(ex);
            ret = false;
        }
        catch (oaException &excp) {
            Errs()->add_error((const char*)excp.getMsg());
            ret = false;
        }
        if (!ret) {
            Errs()->add_error("Failed reading Text properties.");
            return (false);
        }
    }

    return (true);
}


bool
oa_in::readOaInstHeader(oaInstHeader *header, CDs *sdesc, const char *cname)
{
    bool ret = true;
    oaIter<oaInst> insts(header->getInsts(0));
    while (oaInst *inst = insts.getNext()) {
        switch (inst->getType()) {
        case oacScalarInstType:
            ret = readOaScalarInst((oaScalarInst*)inst, cname, header, sdesc);
            break;

        case oacVectorInstBitType:
            ret = readOaVectorInstBit((oaVectorInstBit*)inst, cname, header,
                sdesc);
            break;

        case oacArrayInstType:
            ret = readOaArrayInst((oaArrayInst*)inst, cname, header, sdesc);
            break;

        case oacVectorInstType:
            ret = readOaVectorInst((oaVectorInst*)inst, cname, header, sdesc);
            break;

        default:
            break;
        }
        if (!ret)
            break;
    }
    return (ret);
}


namespace {
    void tx_from_orient(CDtx &tx, oaOrient orient)
    {
        if (orient == oacR90) {
            tx.ax = 0;
            tx.ay = 1;
        }
        else if (orient == oacR180) {
            tx.ax = -1;
            tx.ay = 0;
        }
        else if (orient == oacR270) {
            tx.ax = 0;
            tx.ay = -1;
        }
        else if (orient == oacMY) {
            tx.ax = -1;
            tx.ay = 0;
            tx.refly = true;
        }
        else if (orient == oacMYR90) {
            tx.ax = 0;
            tx.ay = -1;
            tx.refly = true;
        }
        else if (orient == oacMX)
            tx.refly = true;
        else if (orient == oacMXR90) {
            tx.ax = 0;
            tx.ay = 1;
            tx.refly = true;
        }
    }


    void print_prm(const oaParam &p, sLstr &lstr)
    {
        switch (p.getType()) {

        case oacIntParamType:
            lstr.add_i(p.getIntVal());
            break;
        case oacFloatParamType:
            lstr.add_g(p.getFloatVal());
            break;
        case oacStringParamType:
            lstr.add_c('|');
            lstr.add(p.getStringVal());
            lstr.add_c('|');
            break;
        case oacAppParamType:
            lstr.add("appType");
            break;
        case oacDoubleParamType:
            lstr.add_g(p.getDoubleVal());
            break;
        case oacBooleanParamType:
            lstr.add_i(p.getBooleanVal());
            break;
        case oacTimeParamType:
            lstr.add_i(p.getTimeVal());
            break;
        }
    }


    void check_params(oaParamArray &pa1)
    {
        if (pa1.getNumElements() < 1)
            return;
        PCellParam *prms1 = cOAprop::getPcParameters(pa1, 0);
        char *pstr1 = prms1->string(true);
        PCellParam *prms2;
        if (!PCellParam::parseParams(pstr1, &prms2)) {
            OAerrLog.add_log(OAlogPCell, "param check, parse error.\n%s",
                pstr1);
            return;
        }
        oaParamArray pa2;
        cOAprop::savePcParameters(prms2, pa2);
        if (pa1 == pa2)
            OAerrLog.add_log(OAlogPCell, "param check, equal.");
        else {
            int n1 = pa1.getNumElements();
            int n2 = pa2.getNumElements();
            if (n1 != n2)
                OAerrLog.add_log(OAlogPCell,
                    "param check, size difference %d %d.",
                    n1, n2);
            int n = mmMin(n1, n2);
            for (int i = 0; i < n; i++) {
                oaString nm1, nm2;
                pa1[i].getName(nm1);
                pa2[i].getName(nm2);
                if (pa1[i] == pa2[i])
                    OAerrLog.add_log(OAlogPCell, "%s %s ok.", (const char*)nm1,
                        (const char*)nm2);
                else {
                    oaString tp1, tp2;
                    tp1 = pa1[i].getType().getName();
                    tp2 = pa2[i].getType().getName();
                    sLstr lstr;
                    print_prm(pa1[i], lstr);
                    lstr.add_c(' ');
                    print_prm(pa2[i], lstr);
                    OAerrLog.add_log(OAlogPCell, "|%s| %s |%s| %s differ %s.",
                        (const char*)nm1, (const char*)tp1,
                        (const char*)nm2, (const char*)tp2),
                        lstr.string();
                }
            }
        }
    }


    bool makeInst(const oaInst *inst, CallDesc *calldesc, const CDtx *tx,
        const CDap *ap, const oaInstHeader *header, CDs *sdesc, CDp **p0,
        stringlist **warnings, CDc **pnewinst = 0)
    {
        if (pnewinst)
            *pnewinst = 0;
        CDc *newo;
        OItype oiret = sdesc->makeCall(calldesc, tx, ap, CDcallDb, &newo);
        if (oiret != OIok) {
            Errs()->add_error(
                "Failed to create database instance of %s.",
                Tstring(calldesc->name()));
            return (false);
        }
        if (!newo)
            return (false);

        if (p0) {
            stringlist *s0 = sdesc->prptyApplyList(newo, p0);
            if (s0) {
                if (warnings) {
                    stringlist *s = s0;
                    while (s->next)
                        s = s->next;
                    s->next = *warnings;
                    *warnings = s0;
                }
                else
                    stringlist::destroy(s0);
            }
        }

        oaParamArray allParams, params;
        header->getAllParams(allParams);
        header->getParams(params);

        // The presence of a XICP_PC_PARAMS property marks a PCell. 
        // The property string contains only non-default property
        // settings, and can therefor be empty.

        if (allParams.getNumElements() > 0) {
            // Instance of an OpenAccess PCell.
            newo->prptyRemove(XICP_PC);
            newo->prptyRemove(XICP_PC_PARAMS);

            if (OAerrLog.debug_pcell())
                check_params(allParams);
            oaString libname, cellname, viewname;
            inst->getLibName(oaNativeNS(), libname);
            inst->getCellName(oaNativeNS(), cellname);
            inst->getViewName(oaNativeNS(), viewname);

            char *dbname = PCellDesc::mk_dbname(libname, cellname, viewname);
            newo->prptyAdd(XICP_PC, dbname, Physical);
            delete [] dbname;

            PCellParam *prms = cOAprop::getPcParameters(params, 0);
            char *prmstr = prms->string(true);
            if (!prmstr)
                prmstr = lstring::copy("");
            newo->prptyAdd(XICP_PC_PARAMS, prmstr, Physical);
            delete [] prmstr;
            PCellParam::destroy(prms);
        }

        CDs *msdesc = newo->masterCell();
        if (msdesc && msdesc->isElectrical()) {
            // If the electrical master has the unread flag set, save
            // the library name, we may need it later.

            oaString cellname;
            inst->getCellName(oaNativeNS(), cellname);
            if (msdesc->isUnread()) {
                oaString libname;
                inst->getLibName(oaNativeNS(), libname);
                sLstr lstr;
                lstr.add(libname);
                lstr.add_c('/');
                lstr.add(cellname);
                msdesc->prptyAdd(XICP_OA_ORIG, lstr.string());
            }
        }
        if (pnewinst)
            *pnewinst = newo;
        return (true);
    }
}


bool
oa_in::readOaScalarInst(oaScalarInst *inst, const char *cname,
    const oaInstHeader *header, CDs *sdesc)
{
    bool ret = true;
    CDtx tx;

    try {
        oaPoint origin(0,0);
        inst->getOrigin(origin);
        if (sdesc->isElectrical() && in_elec_scale != 1) {
            tx.tx = in_elec_scale*origin.x();
            tx.ty = in_elec_scale*origin.y();
        }
        else {
            tx.tx = origin.x();
            tx.ty = origin.y();
        }
        tx_from_orient(tx, inst->getOrient());
    }
    catch (oaCompatibilityError &ex) {
        cOA::handleFBCError(ex);
        ret = false;
    }
    catch (oaException &excp) {
        Errs()->add_error((const char*)excp.getMsg());
        ret = false;
    }
    if (!ret) {
        Errs()->add_error("Failed reading ScalarInst.");
        return (false);
    }

    // Read the properties.
    CDp *p0 = 0;
    try {
        p0 = readProperties(inst);
    }
    catch (oaCompatibilityError &ex) {
        cOA::handleFBCError(ex);
        ret = false;
    }
    catch (oaException &excp) {
        Errs()->add_error((const char*)excp.getMsg());
        ret = false;
    }
    if (!ret) {
        Errs()->add_error("Failed reading ScalarInst properties.");
        return (false);
    }

    CDcellName cn = CD()->CellNameTableAdd(cname);
    cn = checkSubMaster(cn, p0);
    CallDesc calldesc(cn, 0);

    CDc *cnew;
    CDap ap;
    ret = makeInst(inst, &calldesc, &tx, &ap, header, sdesc, &p0,
        &in_warnings, &cnew);
    CDp::destroy(p0);

    // For electrical instances, if the instance name has the same
    // device prefix, use it as an assigned name.
    if (ret && sdesc->isElectrical()) {
        CDp_cname *pn = (CDp_cname*)cnew->prpty(P_NAME);
        if (!pn) {
            // No instance name property, if the master has one,
            // create one for the instance.
            CDp_sname *ps = (CDp_sname*)cnew->masterCell()->prpty(P_NAME);
            if (ps) {
                pn = new CDp_cname(*ps);
                cnew->link_prpty_list(pn);
            }
        }
        if (pn && !pn->assigned_name() &&
                isalpha(*Tstring(pn->name_string()))) {
            oaString instName;
            inst->getName(in_ns, instName);
            if (lstring::ciprefix(Tstring(pn->name_string()), instName))
                 pn->set_assigned_name(instName);
        }
    }
    return (ret);
}


bool
oa_in::readOaVectorInstBit(oaVectorInstBit *inst, const char *cname,
    const oaInstHeader *header, CDs *sdesc)
{
    // The "bits" are ignored, all info is in the corresponding
    // oaVectorInst.

    (void)inst;
    (void)cname;
    (void)header;
    (void)sdesc;
    return (true);
}


bool
oa_in::readOaVectorInst(oaVectorInst *inst, const char *cname,
    const oaInstHeader *header, CDs *sdesc)
{
    bool ret = true;
    CDtx tx;

    try {
        oaPoint origin(0,0);
        inst->getOrigin(origin);
        if (sdesc->isElectrical() && in_elec_scale != 1) {
            tx.tx = in_elec_scale*origin.x();
            tx.ty = in_elec_scale*origin.y();
        }
        else {
            tx.tx = origin.x();
            tx.ty = origin.y();
        }
        tx_from_orient(tx, inst->getOrient());
    }
    catch (oaCompatibilityError &ex) {
        cOA::handleFBCError(ex);
        ret = false;
    }
    catch (oaException &excp) {
        Errs()->add_error((const char*)excp.getMsg());
        ret = false;
    }
    if (!ret) {
        Errs()->add_error("Failed reading ScalarInst.");
        return (false);
    }

    // Read the properties.
    CDp *p0 = 0;
    try {
        p0 = readProperties(inst);
    }
    catch (oaCompatibilityError &ex) {
        cOA::handleFBCError(ex);
        ret = false;
    }
    catch (oaException &excp) {
        Errs()->add_error((const char*)excp.getMsg());
        ret = false;
    }
    if (!ret) {
        Errs()->add_error("Failed reading ScalarInst properties.");
        return (false);
    }

    CDcellName cn = CD()->CellNameTableAdd(cname);
    cn = checkSubMaster(cn, p0);
    CallDesc calldesc(cn, 0);

    CDap ap;
    CDc *newinst;
    ret = makeInst(inst, &calldesc, &tx, &ap, header, sdesc, &p0,
        &in_warnings, &newinst);
    CDp::destroy(p0);

    // Add the P_RANGE property.
    if (newinst) {
        CDp_range *pr = new CDp_range;
        pr->set_range(inst->getStart(), inst->getStop());
        newinst->link_prpty_list(pr);
    }

    return (true);
}


bool
oa_in::readOaArrayInst(oaArrayInst *inst, const char *cname,
    const oaInstHeader *header, CDs *sdesc)
{
    bool ret = true;
    CDtx tx;
    CDap ap;

    try {
        oaPoint origin(0,0);
        inst->getOrigin(origin);
        if (sdesc->isElectrical() && in_elec_scale != 1) {
            tx.tx = in_elec_scale*origin.x();
            tx.ty = in_elec_scale*origin.y();
        }
        else {
            tx.tx = origin.x();
            tx.ty = origin.y();
        }
        tx_from_orient(tx, inst->getOrient());

        ap.nx = inst->getNumCols();
        ap.ny = inst->getNumRows();
        if (sdesc->isElectrical() && in_elec_scale != 1) {
            ap.dx = in_elec_scale*inst->getDX();
            ap.dy = in_elec_scale*inst->getDY();
        }
        else {
            ap.dx = inst->getDX();
            ap.dy = inst->getDY();
        }
    }
    catch (oaCompatibilityError &ex) {
        cOA::handleFBCError(ex);
        ret = false;
    }
    catch (oaException &excp) {
        Errs()->add_error((const char*)excp.getMsg());
        ret = false;
    }
    if (!ret) {
        Errs()->add_error("Failed reading ArrayInst.");
        return (false);
    }

    // Read the properties.
    CDp *p0 = 0;
    try {
        p0 = readProperties(inst);
    }
    catch (oaCompatibilityError &ex) {
        cOA::handleFBCError(ex);
        ret = false;
    }
    catch (oaException &excp) {
        Errs()->add_error((const char*)excp.getMsg());
        ret = false;
    }
    if (!ret) {
        Errs()->add_error("Failed reading ArrayInst properties.");
        return (false);
    }

    CDcellName cn = CD()->CellNameTableAdd(cname);
    cn = checkSubMaster(cn, p0);
    CallDesc calldesc(cn, 0);

    if (sdesc->isElectrical() && (ap.nx > 1 || ap.ny > 1)) {
        // Xic doesn't support electrical arrays, need to scalarize.

        oaDesign *design = inst->getDesign();
        sLstr lstr;
        lcv(design, lstr);
        lstr.add(":\n");
        lstr.add("scalarizing arrayed electrical instance of ");
        lstr.add(cname);
        lstr.add_c('.');
        OAerrLog.add_err(IFLOG_WARN, lstr.string());

        cTfmStack stk;
        stk.TPush();
        stk.TLoadCurrent(&tx);
        int x, y;
        stk.TGetTrans(&x, &y);
        CDap tap;
        for (unsigned int i = 0; i < ap.ny; i++) {
            for (unsigned int j = 0; j < ap.nx; j++) {
                stk.TTransMult(j*ap.dx, i*ap.dy);
                CDtx ttx;
                stk.TCurrent(&ttx);
                stk.TSetTrans(x, y);

                if (!makeInst(inst, &calldesc, &ttx, &tap, header,
                        sdesc, &p0, &in_warnings)) {
                    CDp::destroy(p0);
                    return (false);
                }
            }
        }
        stk.TPop();
    }
    else if (!makeInst(inst, &calldesc, &tx, &ap, header, sdesc, &p0,
            &in_warnings)) {
        CDp::destroy(p0);
        return (false);
    }
    CDp::destroy(p0);
    return (true);
}


bool
oa_in::readOaViaHeader(oaViaHeader *header, CDs *sdesc, const char *cname)
{
    oaIter<oaVia> vias(header->getVias());
    while (oaVia *via = vias.getNext()) {
        if (!readOaVia(via, cname, sdesc))
            return (false);
    }
    return (true);
}



bool
oa_in::readOaVia(oaVia *via, const char *cname, CDs *sdesc)
{
    bool ret = true;
    CDtx tx;

    try {
        oaPoint origin(0,0);
        via->getOrigin(origin);
        tx.tx = origin.x();
        tx.ty = origin.y();
        tx_from_orient(tx, via->getOrient());
    }
    catch (oaCompatibilityError &ex) {
        cOA::handleFBCError(ex);
        ret = false;
    }
    catch (oaException &excp) {
        Errs()->add_error((const char*)excp.getMsg());
        ret = false;
    }
    if (!ret) {
        Errs()->add_error("Failed reading Via.");
        return (false);
    }

    CallDesc calldesc(CD()->CellNameTableAdd(cname), 0);
    CDc *newo;
    CDap ap;
    OItype oiret = sdesc->makeCall(&calldesc, &tx, &ap, CDcallDb, &newo);
    if (oiret != OIok) {
        Errs()->add_error("Failed to create database instance of %s.", cname);
        return (false);
    }

    // Add a property indicating standard or custom via and params.
    oaViaHeader *hdr = via->getHeader();
    if (hdr->getType() == oacStdViaHeaderType) {
        oaStdViaHeader *stdvhdr = (oaStdViaHeader*)hdr;
        add_std_via_prop(stdvhdr, 0, newo);
    }
    else if (hdr->getType() == oacCustomViaHeaderType) {
        oaCustomViaHeader *cstmvhdr = (oaCustomViaHeader*)hdr;
        oaString libname, cellname, viewname;
        cstmvhdr->getLibName(in_ns, libname);
        cstmvhdr->getCellName(in_ns, cellname);
        cstmvhdr->getViewName(in_ns, viewname);
        oaParamArray vparams;
        cstmvhdr->getParams(vparams);
        PCellParam *prms = cOAprop::getPcParameters(vparams, 0);
        char *str = prms->string(true);
        PCellParam::destroy(prms);
        char *dbname = PCellDesc::mk_dbname(libname, cellname, viewname);
        sLstr lstr;
        lstr.add(dbname);
        lstr.add_c(' ');
        lstr.add(str);
        delete [] dbname;
        delete [] str;
        newo->prptyAdd(XICP_CSTMVIA, lstr.string(), Physical);
    }
    return (true);
}


CDl *
oa_in::mapLayer(oaScalarName &libName, unsigned int layernum,
    unsigned int purposenum, DisplayMode mode)
{
    CDl *ld = CDldb()->findLayer(layernum, purposenum, mode);
    if (ld)
        return (ld);

    char buf[32];
    if (purposenum != oavPurposeNumberDrawing) {
        const char *pname = CDldb()->getOApurposeName(purposenum);
        if (!pname) {
            oaTech *tech = oaTech::find(libName);
            if (!tech)
                tech = oaTech::open(libName, 'r');
            oaPurpose *purpose = oaPurpose::find(tech, purposenum);
            if (purpose) {
                oaString prpname;
                purpose->getName(prpname);
                CDldb()->saveOApurpose(prpname, purposenum);
                pname = CDldb()->getOApurposeName(purposenum);
            }
        }
        if (!pname) {
            snprintf(buf, 32, "purposeX%x", purposenum);
            if (!CDldb()->saveOApurpose(buf, purposenum)) {
                Errs()->add_error(
                    "Failed to save purpose/num %s/%d.", buf, purposenum);
                return (0);
            }
        }
    }
    // The purposenum is now known-good in Xic.

    const char *lname = CDldb()->getOAlayerName(layernum);
    if (lname) {
        // The layernum is already mapped to a name, use it.  This
        // really can't fail.

        ld = CDldb()->newLayer(layernum, purposenum, mode);
        if (!ld) {
            Errs()->add_error(
                "Failed to create layer for %d/%d.", layernum, purposenum);
        }
        return (ld);
    }

    oaTech *tech = oaTech::find(libName);
    if (!tech)
        tech = oaTech::open(libName, 'r');
    oaLayer *layer = oaPhysicalLayer::find(tech, layernum);
    if (layer) {
        oaString lyrname;
        layer->getName(lyrname);
        unsigned int lnum = CDldb()->getOAlayerNum(lyrname);
        if (lnum == CDL_NO_LAYER) {
            if (!CDldb()->saveOAlayer(lyrname, layernum)) {
                Errs()->add_error(
                    "Failed to save layer/num %s/%d.", (const char*)lyrname,
                    layernum);
                return (0);
            }
        }
        ld = CDldb()->newLayer(layernum, purposenum, mode);
        if (!ld) {
            if (lnum != CDL_NO_LAYER && lnum != layernum) {
                Errs()->add_error(
        "Layer %s is already mapped to %d, probably this layer is defined\n"
        "explicitly in the technology, but the name clashes with an internal\n"
        "reserved layer name mapped to %d.", (const char*)lyrname, lnum,
        layernum);
            }
            Errs()->add_error(
                "Failed to create layer for %d/%d.", layernum, purposenum);
        }
        return (ld);
    }

    // Invent a new name.
    snprintf(buf, 32, "layerX%x", layernum);
    if (CDldb()->getOAlayerNum(buf) != CDL_NO_LAYER) {
        // Really unlikely.
        Errs()->add_error("Layer name %s is already in use.", buf);
        return (0);
    }
    if (!CDldb()->saveOAlayer(buf, layernum)) {
        Errs()->add_error("Failed to save layer/num %s/%d.", buf, layernum);
        return (0);
    }
    ld = CDldb()->newLayer(layernum, purposenum, mode);
    if (!ld) {
        Errs()->add_error(
            "Failed to create layer for %d/%d.", layernum, purposenum);
    }
    return (ld);
}


CDp *
oa_in::readProperties(const oaObject *object)
{
    cOAprop prop(in_def_layout, in_def_schematic, in_def_symbol,
        in_def_dev_prop, in_from_xic);
    CDp *p0 = prop.handleProperties(object, in_mode);
    in_from_xic = prop.fromXic();
    if (in_from_xic)
        in_elec_scale = XIC_ELEC_SCALE;
    return (p0);
}


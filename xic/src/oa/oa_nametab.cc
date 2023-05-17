
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

#include "cd.h"
#include "fio.h"
#include "fio_library.h"
#include "oa.h"
#include "oa_nametab.h"
#include "oa_prop.h"
#include "oa_alib.h"
#include "oa_errlog.h"
#include "pcell.h"
#include "pcell_params.h"


cOAnameTab::cOAnameTab()
{
    nt_cname_tab = 0;
    nt_libtab_tab = 0;
}


cOAnameTab::~cOAnameTab()
{
    delete nt_cname_tab;
    if (nt_libtab_tab) {
        SymTabGen gen(nt_libtab_tab, true);
        SymTabEnt *h;
        while ((h = gen.next()) != 0) {
            delete (SymTab*)h->stData;
            delete h;
        }
        delete nt_libtab_tab;
    }
}


// The cname is the name of the Xic cell, which may be different from
// the OA cell name.  The mapping is done in getMasterName.

unsigned long
cOAnameTab::findCname(CDcellName nm)
{
    if (!nt_cname_tab)
        return (0);
    unsigned long f = (unsigned long)SymTab::get(nt_cname_tab,
        (unsigned long)nm);
    if (f == (unsigned long)ST_NIL)
        return (0);
    return (f);
}


void
cOAnameTab::updateCname(CDcellName nm, unsigned long f)
{
    if (!nt_cname_tab)
        nt_cname_tab = new SymTab(false, false);
    SymTabEnt *h = SymTab::get_ent(nt_cname_tab, (unsigned long)nm);
    if (!h) {
        OAerrLog.add_log(OAlogLoad, "adding to cell name table: %s.",
            Tstring(nm));
        nt_cname_tab->add((unsigned long)nm, (void*)f, false);
    }
    else {
        unsigned long ll = (unsigned long)h->stData;
        h->stData = (void*)(ll | f);
    }
}


void
cOAnameTab::clearCnameTab()
{
    delete nt_cname_tab;
    nt_cname_tab = 0;
}


// Return a cell name to correspond to L/C/V.  This takes care of
// mapping pcell names (has_params is true for pcells).
//
CDcellName
cOAnameTab::getMasterName(const oaScalarName &libName,
    const oaScalarName &cellName, const oaScalarName &viewName,
    const oaParamArray &params, bool has_params, bool from_xic)
{
    if (has_params) {
        oaString libname, cellname, viewname;
        libName.get(oaNativeNS(), libname);
        cellName.get(oaNativeNS(), cellname);
        viewName.get(oaNativeNS(), viewname);

        PCellParam *pm = cOAprop::getPcParameters(params, 0);
        char *cname = PC()->addSubMaster(libname, cellname, viewname, pm);
        CDcellName nn = CD()->CellNameTableAdd(cname);
        delete [] cname;
        PCellParam::destroy(pm);
        return (nn);
    }
    return (cellNameAlias(libName, cellName, from_xic));
}


// This modifies the cell name if there would be a clash due to the
// same cell name already being in use from a different OA library. 
// We also modify cell names that clash with the Xic device library.
//
CDcellName
cOAnameTab::cellNameAlias(const oaScalarName &libName,
    const oaScalarName &cellName, bool from_xic)
{
    oaString libname, cellname;
    libName.get(oaNativeNS(), libname);
    cellName.get(oaNativeNS(), cellname);
    CDcellName lname = CD()->CellNameTableAdd(libname);
    CDcellName cname = CD()->CellNameTableAdd(cellname);
    SymTab *stab = (SymTab*)SymTab::get(nt_libtab_tab, (unsigned long)lname);
    if (stab != (SymTab*)ST_NIL) {
        CDcellName nn = (CDcellName)SymTab::get(stab, (unsigned long)cname);
        if (nn != (CDcellName)ST_NIL)
            return (nn);
    }

    // We haven't seen this lib/cell before.  See if the cell name is
    // in use.
    CDcellName newcname = cname;

    // Remap cell names from the Virtuoso analogLib, which contains
    // schematic symbols that conflict with the Xic device library.

    if (AlibFixup.is_alib_device((const char*)cellname)) {
        const char *pfx = ANALOG_LIB_PFX;
        char *cn = new char[strlen(cellname) + strlen(pfx) + 1];
        char *e = lstring::stpcpy(cn, pfx);
        strcpy(e, cellname);
        newcname = CD()->CellNameTableAdd(cn);
        delete [] cn;
    }

    bool clash = findCname(newcname);
    if (!clash && !from_xic)
        clash = FIO()->LookupLibCell(0, Tstring(newcname), LIBdevice, 0);
    if (clash)
        newcname = getNewName(newcname);

    if (!nt_libtab_tab)
        nt_libtab_tab = new SymTab(false, false);
    if (stab == (SymTab*)ST_NIL) {
        stab = new SymTab(false, false);
        nt_libtab_tab->add((unsigned long)lname, stab, false);
    }
    stab->add((unsigned long)cname, newcname, false);
    return (newcname);
}


// Return a modified name that does not clash with an existing Xic
// cell name.
//
CDcellName
cOAnameTab::getNewName(CDcellName name)
{
    // We assume here that the "_N" suffix automatically prevents
    // a clash with an Xic library device name.

    const char *oldname = Tstring(name);
    int len = strlen(oldname) + 20;
    char *tbuf = new char[len];
    char *e = lstring::stpcpy(tbuf, oldname);
    *e++ = '_';
    len -= (e - tbuf);
    for (int i = 1; ; i++) {
        snprintf(e, len, "%d", i);
        CDcellName nn = CD()->CellNameTableAdd(tbuf);
        if (SymTab::get(nt_cname_tab, (unsigned long)nn) == ST_NIL) {
            delete [] tbuf;
            return (nn);
        }
    }
    // unreached
    return (0);
}


// Return an alpha-sorted list of the library names.
//
stringlist *
cOAnameTab::listLibNames()
{
    if (!nt_libtab_tab)
        return (0);
    stringlist *sl0 = 0;
    SymTabGen gen(nt_libtab_tab);
    SymTabEnt *h;
    while ((h = gen.next()) != 0)
        sl0 = new stringlist(lstring::copy(h->stTag), sl0);
    stringlist::sort(sl0);
    return (sl0);
}


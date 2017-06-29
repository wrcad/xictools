
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
 $Id: ext_device.cc,v 5.172 2017/03/19 01:58:39 stevew Exp $
 *========================================================================*/

#include "config.h"
#include "main.h"
#include "ext.h"
#include "ext_extract.h"
#include "ext_antenna.h"
#include "ext_errlog.h"
#include "ext_rlsolver.h"
#include "ext_grpgen.h"
#include "cd_celldb.h"
#include "cd_terminal.h"
#include "cd_netname.h"
#include "cd_strmdata.h"
#include "cd_propnum.h"
#include "edit.h"
#include "sced.h"
#include "sced_param.h"
#include "paramsub.h"
#include "dsp_inlines.h"
#include "dsp_tkif.h"
#include "dsp_layer.h"
#include "dsp_color.h"
#include "geo_zgroup.h"
#include "geo_ylist.h"
#include "si_parsenode.h"
#include "si_parser.h"
#include "si_lexpr.h"
#include "promptline.h"
#include "errorlog.h"
#include "layertab.h"
#include "tech.h"
#include "tech_extract.h"
#include "tech_layer.h"
#include "spnumber.h"
#include "timer.h"
#include "rangetest.h"
#include <algorithm>


/*========================================================================*
 *
 *  Functions for device extraction from physical layout
 *
 *========================================================================*/

namespace {
    // Measure primitive keyword bases.
    //
    struct sMkw
    {
        bool checkname(const char*, const char*) const;
        bool is_prim(const char*) const;

        static const char *nsect;
        static const char *bArea;
        static const char *bPerim;
        static const char *bMinDim;
        static const char *cArea;
        static const char *cPerim;
        static const char *cWidth;
        static const char *cnWidth;
        static const char *cbWidth;
        static const char *cbnWidth;
        static const char *resistance;
        static const char *inductance;
        static const char *mutual;
        static const char *capacitance;
    };

    sMkw Mkw;
}

const char *sMkw::nsect       = "Sections";
const char *sMkw::bArea       = "BodyArea";
const char *sMkw::bPerim      = "BodyPerim";
const char *sMkw::bMinDim     = "BodyMinDimen";
const char *sMkw::cArea       = "CArea";
const char *sMkw::cPerim      = "CPerim";
const char *sMkw::cWidth      = "CWidth";
const char *sMkw::cnWidth     = "CNWidth";
const char *sMkw::cbWidth     = "CBWidth";
const char *sMkw::cbnWidth    = "CBNWidth";
const char *sMkw::resistance  = "Resistance";
const char *sMkw::inductance  = "Inductance";
const char *sMkw::mutual      = "Mutual_Inductance";
const char *sMkw::capacitance = "Capacitance";



// Return true if the base name in str matches name.
//
bool
sMkw::checkname(const char *str, const char *name) const
{
    char buf[64];
    strcpy(buf, str);
    char *s = strchr(buf, '.');
    if (s)
        *s = 0;
    if (lstring::cieq(buf, name))
        return (true);
    return (false);
}


// Return true if the base name is a primitive name.
//
bool
sMkw::is_prim(const char *str) const
{
    if (checkname(str, nsect)           ||
            checkname(str, bArea)       ||
            checkname(str, bPerim)      ||
            checkname(str, bMinDim)     ||
            checkname(str, cArea)       ||
            checkname(str, cPerim)      ||
            checkname(str, cWidth)      ||
            checkname(str, cnWidth)     ||
            checkname(str, cbWidth)     ||
            checkname(str, cbnWidth)    ||
            checkname(str, resistance)  ||
            checkname(str, inductance)  ||
            checkname(str, mutual)      ||
            checkname(str, capacitance))
        return (true);
    return (false);
}

#define checkname(x, y) Mkw.checkname(x, sMkw::y)


namespace {
    // Set the contact search area.  If q < 0, the search area is the entire
    // BB.  Otherwise, set to one of the four quadrants of BB.
    //
    void
    setqz(Zoid &Z, const BBox &BB, int q)
    {
        Z.set(&BB);
        if (q < 0)
            return;
        q %= 4;
        switch (q) {
        case 0:
            // upper left
            Z.yl = (Z.yl + Z.yu)/2;
            Z.xur = Z.xlr = (Z.xll + Z.xlr)/2;
            break;
        case 1:
            // lower right
            Z.yu = (Z.yl + Z.yu)/2;
            Z.xul = Z.xll = (Z.xll + Z.xlr)/2;
            break;
        case 2:
            // lower left
            Z.yu = (Z.yl + Z.yu)/2;
            Z.xur = Z.xlr = (Z.xll + Z.xlr)/2;
            break;
        case 3:
            // upper right
            Z.yl = (Z.yl + Z.yu)/2;
            Z.xul = Z.xll = (Z.xll + Z.xlr)/2;
            break;
        }
    }

    int ncomp(const char *s1, const char *s2)
    {
        if (!s1 && !s2)
            return (0);
        if (!s1)
            return (-1);
        if (!s2)
            return (1);
        return (strcmp(s1, s2));
    }
}


// Add the subcircuit descriptor to the global table.
//
void
cExt::addSubcircuit(sSubcDesc *sd)
{
    if (!sd || !sd->master())
        return;
    if (!ext_subckt_tab)
        ext_subckt_tab = new SymTab(false, false);
    sSubcDesc *osd =
        (sSubcDesc*)ext_subckt_tab->get((unsigned long)sd->master());
    if (osd != (sSubcDesc*)ST_NIL) {
        ext_subckt_tab->remove((unsigned long)sd->master());
        delete osd;
    }
    ext_subckt_tab->add((unsigned long)sd->master(), sd, false);
}


// Find the subcircuit descriptor in the global table.
//
sSubcDesc *
cExt::findSubcircuit(const CDs *sdesc)
{
    if (!ext_subckt_tab || !sdesc)
        return (0);
    sSubcDesc *sd = (sSubcDesc*)ext_subckt_tab->get((unsigned long)sdesc);
    if (sd == (sSubcDesc*)ST_NIL)
        return (0);
    return (sd);
}


// Add a device description to the global table.  Return an error
// message if there is a problem, in which case the device is not
// added to the table.
//
char * 
cExt::addDevice(sDevDesc *dd, sDevDesc **pold)
{
    if (pold)
        *pold = 0;
    if (!dd || !dd->name())
        return (0);
    if (!ext_device_tab)
        ext_device_tab = new SymTab(false, false);
    // Recall that sDevDesc::name() is from the cell name string table.
    SymTabEnt *ent = ext_device_tab->get_ent(dd->name()->string());
    if (ent) {
        char *s = dd->checkEquiv((sDevDesc*)ent->stData);
        if (s)
            return (s);

        // Keep the prefix list sorted, null prefix first then others
        // in alpha order.

        sDevDesc *dp = 0, *dn;
        for (sDevDesc *d = (sDevDesc*)ent->stData; d; d = dn) {
            dn = d->next();
            int n = ncomp(dd->prefix(), d->prefix());
            if (n == 0) {
                if (dd == d)
                    return (0);
                // Replace matching existing entry.
                if (dp)
                    dp->set_next(dd);
                else
                    ent->stData = dd;
                dd->set_next(dn);
                if (pold)
                    *pold = d;
                else
                    delete d;
                return (0);
            }
            if (n < 0) {
                if (dp)
                    dp->set_next(dd);
                else
                    ent->stData = dd;
                dd->set_next(d);
                return (0);
            }
            dp = d;
        }
        dp->set_next(dd);
    }
    else
        ext_device_tab->add(dd->name()->string(), dd, false);
    return (0);
}


sDevDesc *
cExt::removeDevice(const char *name, const char *prefix)
{
    if (!ext_device_tab)
        return (0);
    SymTabEnt *ent = ext_device_tab->get_ent(name);
    if (ent) {
        sDevDesc *dp = 0, *dn;
        for (sDevDesc *d = (sDevDesc*)ent->stData; d; d = dn) {
            dn = d->next();
            int n = ncomp(prefix, d->prefix());
            if (!n) {
                if (dp)
                    dp->set_next(dn);
                else
                    ent->stData = dn;
                if (!ent->stData)
                    ext_device_tab->remove(d->name()->string());
                return (d);
            }
            if (n > 0)
                break;
        }
    }
    return (0);
}


// Find a device description in the global table.
//
sDevDesc *
cExt::findDevice(const char *name, const char *prefix)
{
    if (!ext_device_tab)
        return (0);
    sDevDesc *desc = (sDevDesc*)ext_device_tab->get(name);
    if (desc != (sDevDesc*)ST_NIL) {
        for (sDevDesc *d = desc; d; d = d->next()) {
            int n = ncomp(prefix, d->prefix());
            if (!n)
                return (d);
            if (n < 0)
                break;
        }
    }
    return (0);
}


// Return the list of descriptors corresponding to the ordered
// prefixes of the named device.
//
sDevDesc *
cExt::findDevices(const char *name)
{
    if (!ext_device_tab)
        return (0);
    sDevDesc *desc = (sDevDesc*)ext_device_tab->get(name);
    if (desc != (sDevDesc*)ST_NIL)
        return (desc);
    return (0);
}


// Return a sorted list of all device (and prefix) names.
//
stringlist *
cExt::listDevices()
{
    if (!ext_device_tab)
        return (0);
    SymTabGen gen(ext_device_tab);
    SymTabEnt *ent;
    stringlist *s0 = 0;
    while ((ent = gen.next()) != 0) {
        for (sDevDesc *d = (sDevDesc*)ent->stData; d; d = d->next()) {
            sLstr lstr;
            lstr.add(d->name()->string());
            if (d->prefix()) {
                lstr.add_c(' ');
                lstr.add(d->prefix());
            }
            s0 = new stringlist(lstr.string_trim(), s0);
        }
    }
    s0->sort();
    return (s0);
}


// Traverse the device list and call the init function.  This is deferred
// until the tech file is read, so all layers are defined.
//
void
cExt::initDevs()
{
    SymTabGen gen(ext_device_tab);
    SymTabEnt *ent;
    while ((ent = gen.next()) != 0) {
        sDevDesc *dp = 0, *dn;
        for (sDevDesc *d = (sDevDesc*)ent->stData; d; d = dn) {
            dn = d->next();
            if (!d->init()) {
                if (!dp)
                    ent->stData = dn;
                else
                    dp->set_next(d->next());
                if (!ent->stData)
                    ext_device_tab->remove(d->name()->string());
                Log()->ErrorLogV(mh::Techfile,
                    "Init failed for device %s, prefix %s, device will "
                    "be ignored.\n%s",
                    d->name()->string(), d->prefix() ? d->prefix() : "none",
                    Errs()->get_error());
                delete d;
                continue;
            }
            dp = d;
        }
    }
}


// Return a list of structs containing physical device instances.
//
XIrt
cExt::getDevlist(CDcbin *cbin, sDevList **dvl)
{
    if (!dvl)
        return (XIbad);
    *dvl = 0;
    sDevList *dv0 = 0, *dve = 0;

    SymTabGen gen(ext_device_tab);
    SymTabEnt *ent;
    while ((ent = gen.next()) != 0) {
        for (sDevDesc *d = (sDevDesc*)ent->stData; d; d = d->next()) {
            sDevInst *di;
            XIrt ret = d->find(cbin->phys(), &di, 0);
            if (ret != XIok) {
                di->free();
                dv0->free();
                return (ret);
            }
            if (di) {
                sDevList *dx;
                for (dx = dv0; dx; dx = dx->next()) {
                    if (dx->devname() == d->name())
                        break;
                }
                if (dx)
                    dx->add(di);
                else {
                    if (!dv0)
                        dv0 = dve = new sDevList(di, 0);
                    else {
                        dve->set_next(new sDevList(di, 0));
                        dve = dve->next();
                    }
                }
            }
        }
    }
        
    *dvl = dv0;
    return (XIok);
}


// For each physical cell in the current hierarchy, save the measure
// results into the "data box" property.
//
bool
cExt::saveMeasuresInProp()
{
    CDs *sd = CurCell(Physical);
    if (!sd)
        return (false);
    if (!associate(sd))
        return (false);

    CDgenHierDn_s gen(sd);
    CDs *tsd;
    bool err;
    bool ret = true;
    while ((tsd = gen.next(&err)) != 0) {
        cGroupDesc *gd = tsd->groups();
        if (!gd)
            continue;
        if (!gd->setup_dev_layer() || !gd->update_measure_prpty())
            ret = false;
    }
    if (err)
        ret = false;
    if (!ret && Errs()->has_error())
        Log()->ErrorLog(mh::Initialization, Errs()->get_error());
    return (ret);
}
// End of cExt functions.


// Return a list of instances, physical only, one "real" instance per
// sDevInstList link.  Instances match device name, etc.  if given,
// and overlap AOI, if given.  Returned instances are from the group
// struct and should not be deleted.
//
sDevInstList *
cGroupDesc::find_dev(const char *name, const char *pref, const char *inds,
    const BBox *AOI)
{
    BBox aoiBB;
    if (AOI) {
        aoiBB = *AOI;
        if (aoiBB.left == aoiBB.right) {
            aoiBB.left -= 10;
            aoiBB.right += 10;
        }
        if (aoiBB.bottom == aoiBB.top) {
            aoiBB.bottom -= 10;
            aoiBB.top += 10;
        }
    }

    cRangeTest rtest(inds);

    sDevInstList *d0 = 0;
    for (sDevList *dv = gd_devices; dv; dv = dv->next()) {
        if (name && strcmp(name, dv->devname()->string()))
            continue;
        for (sDevPrefixList *p = dv->prefixes(); p; p = p->next()) {
            if (pref && p->devs()->desc()->prefix() &&
                    strcmp(pref, p->devs()->desc()->prefix()))
                continue;
            for (sDevInst *di = p->devs(); di; di = di->next()) {
                if (rtest.check(di->index())) {
                    if (AOI) {
                        bool err = false;
                        sDevInstList *dx = di->getdev(&aoiBB, &err);
                        if (err) {
                            d0->free();
                            return (0);
                        }
                        if (dx) {
                            dx->next = d0;
                            d0 = dx;
                        }
                    }
                    else
                        d0 = new sDevInstList(di, d0);
                }
            }
        }
    }
    return (d0);
}


// Search for devices as above, but set the displayed flag to state
// on matches.  Return the number of matches.
//
int
cGroupDesc::find_dev_set(const char *name, const char *pref, const char *inds,
    const BBox *AOI, bool state)
{
    BBox aoiBB;
    if (AOI) {
        aoiBB = *AOI;
        if (aoiBB.left == aoiBB.right) {
            aoiBB.left -= 10;
            aoiBB.right += 10;
        }
        if (aoiBB.bottom == aoiBB.top) {
            aoiBB.bottom -= 10;
            aoiBB.top += 10;
        }
    }

    cRangeTest rtest(inds);

    int found = 0;
    for (sDevList *dv = gd_devices; dv; dv = dv->next()) {
        if (!name || !*name || !strcmp(name, dv->devname()->string())) {
            for (sDevPrefixList *p = dv->prefixes(); p; p = p->next()) {
                if (!pref || !*pref || (p->devs()->desc()->prefix() &&
                        !strcmp(pref, p->devs()->desc()->prefix()))) {
                    for (sDevInst *di = p->devs(); di;
                            di = di->next()) {
                        if (rtest.check(di->index())) {
                            if (AOI) {
                                if (aoiBB.intersect(di->bBB(), false)) {
                                    SIlexprCx cx(gd_celldesc,
                                        di->desc()->depth() + di->bdepth(),
                                        &aoiBB);
                                    Zlist *zret = 0;
                                    if (di->desc()->body()->getZlist(&cx,
                                            &zret) != XIok)
                                        return (found);
                                    if (zret) {
                                        Zlist::free(zret);
                                        di->set_displayed(state);
                                        found++;
                                    }
                                }
                            }
                            else {
                                di->set_displayed(state);
                                found++;
                            }
                        }
                    }
                }
            }
        }
    }
    return (found);
}


// Parse a string of the form name.prefix.index, set the matching device
// display flag, and redisplay.  If the string is null or blank, clear
// all displayed devices.  Fields are optional, index is a number, range,
// or comma-separated list of numbers and ranges.
//
void
cGroupDesc::parse_find_dev(const char *str, bool show)
{
    char *t = lstring::copy(str);
    char *t0 = t;

    char *name = 0;
    char *pref = 0;
    char *inds = 0;

    if (t) {
        while (isspace(*t))
            t++;
        char *s = strchr(t, '.');
        if (s)
            *s++ = 0;
        if (*t) {
            name = lstring::copy(t);
            t = name + strlen(name) - 1;
            while (t > name && isspace(*t))
                *t-- = 0;
            t = s;
        }
    }
    if (t) {
        while (isspace(*t))
            t++;
        char *s = strchr(t, '.');
        if (s)
            *s++ = 0;
        if (*t) {
            pref = lstring::copy(t);
            t = pref + strlen(pref) - 1;
            while (t > pref && isspace(*t))
                *t-- = 0;
            t = s;
        }
    }
    if (t) {
        while (isspace(*t))
            t++;
        if (*t)
            inds = lstring::copy(t);
    }
    delete [] t0;

    if (name && lstring::cieq(name, "all")) {
        delete [] name;
        name = 0;
    }
    if (pref && lstring::cieq(pref, "all")) {
        delete [] pref;
        pref = 0;
    }

    if (!name && !pref && !inds) {
        WindowDesc *wd;
        WDgen wgen(WDgen::MAIN, WDgen::CDDB);
        while ((wd = wgen.next()) != 0)
            show_devs(wd, ERASE);
        set_devs_display(show);
        EX()->setShowingDevs(show);
        if (show) {
            wgen = WDgen(WDgen::MAIN, WDgen::CDDB);
            while ((wd = wgen.next()) != 0)
                show_devs(wd, DISPLAY);
        }
    }
    else {
        WindowDesc *wd;
        WDgen wgen(WDgen::MAIN, WDgen::CDDB);
        while ((wd = wgen.next()) != 0)
            show_devs(wd, ERASE);

        find_dev_set(name, pref, inds, 0, show);
        wgen = WDgen(WDgen::MAIN, WDgen::CDDB);
        int cnt = 0;
        while ((wd = wgen.next()) != 0)
            cnt += show_devs(wd, DISPLAY);
        EX()->setShowingDevs(cnt != 0);
        delete [] name;
        delete [] pref;
        delete [] inds;
    }
}

 
// Return a sorted list of all devices extracted.
//
stringlist *
cGroupDesc::list_devs()
{
    {
        cGroupDesc *gdt = this;
        if (!gdt)
            return (0);
    }
    stringlist *s0 = 0;
    char buf[256];
    for (sDevList *dv = gd_devices; dv; dv = dv->next()) {
        for (sDevPrefixList *p = dv->prefixes(); p; p = p->next()) {
            int maxidx = 0;
            for (sDevInst *di = p->devs(); di; di = di->next()) {
                if (di->index() > maxidx)
                    maxidx = di->index();
            }
            sprintf(buf, "%s %s %d-%d", p->devs()->desc()->name()->string(), 
                p->devs()->desc()->prefix() ?
                    p->devs()->desc()->prefix() : EXT_NONE_TOK, 0, maxidx);
            s0 = new stringlist(lstring::copy(buf), s0);
        }
    }
    s0->sort();
    return (s0);
}


// Set or clear the display flag for all devices.
//
void
cGroupDesc::set_devs_display(bool state)
{
    for (sDevList *dv = gd_devices; dv; dv = dv->next()) {
        for (sDevPrefixList *p = dv->prefixes(); p; p = p->next()) {
            for (sDevInst *di = p->devs(); di; di = di->next())
                di->set_displayed(state);
        }
    }
}


// Highlight/erase highlighting of all devices with the display flag set.
//
int
cGroupDesc::show_devs(WindowDesc *wdesc, bool d_or_e)
{
    {
        cGroupDesc *gdt = this;
        if (!gdt)
            return (0);
    }

    static bool skipit;  // prevent reentrancy
    if (skipit)
        return (0);
    if (!wdesc->Wdraw())
        return (0);
    if (!wdesc->IsSimilar(DSP()->MainWdesc(), WDsimXmode))
        return (0);

    int cnt = 0;
    if (dspPkgIf()->IsDualPlane())
        wdesc->Wdraw()->SetXOR(d_or_e == DISPLAY ? GRxHlite : GRxUnhlite);
    else {
        if (d_or_e == DISPLAY)
            wdesc->Wdraw()->SetColor(
                DSP()->Color(HighlightingColor, wdesc->Mode()));
        else {
            BBox BB(CDnullBB);
            for (sDevList *dv = gd_devices; dv; dv = dv->next()) {
                for (sDevPrefixList *p = dv->prefixes(); p; p = p->next()) {
                    for (sDevInst *di = p->devs(); di; di = di->next()) {
                        if (di->displayed()) {
                            di->show(wdesc, &BB);
                            cnt++;
                        }
                    }
                }
            }
            skipit = true;
            wdesc->Redisplay(&BB);
            skipit = false;
            return (cnt);
        }
    }
    for (sDevList *dv = gd_devices; dv; dv = dv->next()) {
        for (sDevPrefixList *p = dv->prefixes(); p; p = p->next()) {
            for (sDevInst *di = p->devs(); di; di = di->next()) {
                if (di->displayed()) {
                    di->show(wdesc);
                    cnt++;
                }
            }
        }
    }
    if (dspPkgIf()->IsDualPlane())
        wdesc->Wdraw()->SetXOR(GRxNone);
    else if (LT()->CurLayer())
        wdesc->Wdraw()->SetColor(dsp_prm(LT()->CurLayer())->pixel());
    return (cnt);
}


// Link the device contact into the group.
//
bool
cGroupDesc::link_contact(sDevContactInst *ci)
{
    if (ci->group() >= 0 && ci->group() <= gd_asize) {
        gd_groups[ci->group()].set_device_contacts(
            new sDevContactList(ci, gd_groups[ci->group()].device_contacts()));
        return (true);
    }
    return (false);
}


// If the contact group has not been assigned, look for the contact
// metal overlapping the contact area.  If found, obtain and return
// the group number.  If check_hier, look for a virtual connection
// within subcells.  The return value is negative if the contact group
// is not set here.
//
int
cGroupDesc::find_contact_group(sDevContactInst *ci, bool check_hier)
{
    if (ci->group() >= 0 || !ci->cont_ok())
        return (-1);

    // Find the contact metal layer.
    CDl *ld = CDldb()->findLayer(ci->desc()->lname(), Physical);
    if (!ld || !ld->isConductor()) {
        sDevContactDesc *d = ci->desc();
        sLstr lstr;
        lstr.add(d->lname());
        lstr.add(" for ");
        lstr.add(d->name()->string());
        const char *msg = "conductor layer %s not found.";
        ExtErrLog.add_dev_err(gd_celldesc, ci->dev(), msg, lstr.string());
        return (-1);
    }

    // Check the containing cell metal.  This may fail unless called
    // after flatten.
    sGrpGen grg;
    grg.init_gen(this, ld, ci->cBB());
    CDo *odesc;
    while ((odesc = grg.next()) != 0) {
        if (odesc->intersect(ci->cBB(), false) && odesc->group() >= 0) {
            ci->set_group(odesc->group());
            link_contact(ci);
            return (odesc->group());
        }
    }

    if (!check_hier)
        return (-1);

    // Descend through the hierarchy looking for overlapping metal. 
    // If found, recurse the group number up if possible.

    cTfmStack stk;
    sSubcLink *cl = build_links(&stk, ci->cBB());
    sSubcGen cg;
    cg.set(cl);

    bool skipsubs = false;
    CDc *cd;
    for ( ; (cd = cg.current()) != 0; cg.advance(skipsubs)) {

        skipsubs = false;
        if (!cd->intersect(ci->cBB(), false)) {
            skipsubs = true;
            continue;
        }

        CDs *sd = cd->masterCell(true);
        cGroupDesc *gd = sd->groups();
        if (!gd)
            continue;

        // AOI is the inverse transform of the contact area.
        CDtf *tf = cg.transform();
        stk.TPush();
        stk.TLoadCurrent(tf);
        stk.TInverse();
        BBox AOI(*ci->cBB());
        stk.TInverseBB(&AOI, 0);
        stk.TPop();

        // look through the subcell groups for metal on ld that
        // overlaps AOI.  Nonzero overlap area indicates "contact".
        for (int i = 0; i < gd->gd_asize; i++) {
            sGroupObjs *gobj = gd->gd_groups[i].net();
            if (!gobj)
                continue;
            for (CDol *ol = gobj->objlist(); ol; ol = ol->next) {
                if (ol->odesc->ldesc() != ld)
                    continue;
                if (ol->odesc->intersect(&AOI, false)) {
                    int subc_group = ol->odesc->group();
                    // Found a candidate object, now walk back up the
                    // hierarchy to find the group number at the top
                    // level.

                    int top_group = -1;
                    int sp = 0;
                    for (;;) {
                        const sSubcLink *tcl = cg.getlink(sp);
                        if (!tcl)
                            break;
                        CDc *tcd = tcl->cdesc;
                        if (!tcd)
                            break;
                        CDs *parent = tcd->parent();
                        if (!parent)
                            break;
                        cGroupDesc *gdp = parent->groups();
                        if (!gdp)
                            break;
                        sSubcInst *s = gdp->find_subc(tcd, tcl->ix, tcl->iy);
                        if (!s)
                            break;
                        int par_grp = -1;
                        for (sSubcContactInst *c = s->contacts(); c;
                                c = c->next()) {
                            if (c->subc_group() == subc_group) {
                                par_grp = c->parent_group();
                                break;
                            }
                        }
                        if (gdp == this) {
                            top_group = par_grp;
                            break;
                        }
                        subc_group = par_grp;
                        if (subc_group < 0)
                            break;
                        sp++;
                    }
                    if (top_group >= 0) {
                        ci->set_group(top_group);
                        link_contact(ci);
                        cl->free();
                        return (top_group);
                    }
                }
            }
        }
    }
    cl->free();
    return (-1);
}


// Return true if a bulk contact is correctly resolved.  This the top
// level descriptor.  the bBB is the device body of the device d
// containing the terminal, translated to top level coordinates.  The
// next argument is the descriptor of the bulk contact, and the final
// argument is an error return.
//
// This is used to check the BC_defer contacts.
//
bool
cGroupDesc::has_bulk_contact(const sDevDesc *d, const BBox *bBB,
    sDevContactDesc *c, XIrt *xrt)
{
    *xrt = XIok;
    if (!c || c->level() == BC_immed)
        return (false);
    if (c->level() == BC_skip)
        return (true);
    BBox bcBB(*bBB);
    bcBB.bloat(INTERNAL_UNITS(c->bulk_bloat()));
    SIlexprCx cx(gd_celldesc, CDMAXCALLDEPTH, &bcBB);
    Zlist *zret = 0;
    XIrt ret = c->lspec()->getZlist(&cx, &zret);
    if (ret != XIok) {
        const char *msg = "contact expression for %s%s, evaluation %s.";
        ExtErrLog.add_dev_err(gd_celldesc, 0, msg, d->name(), c->name(),
            ret == XIintr ? "interrupted" : "failed");
        Zlist::free(zret);
        *xrt = ret;
        return (false);
    }
    if (!zret)
        return (false);

    BBox cBB;
    Zlist::BB(zret, cBB);
    Zlist::free(zret);
    return (check_bulk_contact_group(&cBB, c));
}


// Return true if region cBB connects to group metal that has the same
// netname as c, somewhere in the hierarchy.  The netname is assumed
// to be a global.
//
bool
cGroupDesc::check_bulk_contact_group(const BBox *cBB, const sDevContactDesc *c)
{
    if (!c || !c->netname())
        return (false);

    // Find the contact metal layer.
    CDl *ld = CDldb()->findLayer(c->lname(), Physical);
    if (!ld || !ld->isConductor())
        return (false);

    // Check the containing cell metal.
    sGrpGen grg;
    grg.init_gen(this, ld, cBB);
    CDo *odesc;
    while ((odesc = grg.next()) != 0) {
        if (odesc->intersect(cBB, false)) {
            sGroup *g = group_for(odesc->group());
            if (g && g->netname() == c->netname())
                return (true);
        }
    }

    // Descend through the hierarchy looking for overlapping metal. 

    cTfmStack stk;
    sSubcLink *cl = build_links(&stk, cBB);
    sSubcGen cg;
    cg.set(cl);

    bool skipsubs = false;
    CDc *cd;
    for ( ; (cd = cg.current()) != 0; cg.advance(skipsubs)) {

        skipsubs = false;
        if (!cd->intersect(cBB, false)) {
            skipsubs = true;
            continue;
        }

        CDs *sd = cd->masterCell(true);
        cGroupDesc *gd = sd->groups();
        if (!gd)
            continue;

        // AOI is the inverse transform of the contact area.
        CDtf *tf = cg.transform();
        stk.TPush();
        stk.TLoadCurrent(tf);
        stk.TInverse();
        BBox AOI(*cBB);
        stk.TInverseBB(&AOI, 0);
        stk.TPop();

        // look through the subcell groups for metal on ld that
        // overlaps AOI.  Nonzero overlap area indicates "contact".
        for (int i = 0; i < gd->gd_asize; i++) {
            sGroupObjs *gobj = gd->gd_groups[i].net();
            if (!gobj)
                continue;
            for (CDol *ol = gobj->objlist(); ol; ol = ol->next) {
                if (ol->odesc->ldesc() != ld)
                    continue;
                if (ol->odesc->intersect(&AOI, false)) {
                    sGroup *g = gd->group_for(ol->odesc->group());
                    if (g && g->netname() == c->netname())
                        return (true);
                }
            }
        }
    }
    cl->free();
    return (false);
}


namespace {
    double
    findmax(float *g, int nc)
    {
        if (!g)
            return (0.0);
        double gmax = -1e15;
        for (int i = 1; i < nc; i++) {
            for (int j = i+1; j < nc; j++) {
                double gx = -g[i*nc + j];
                if (gx > gmax)
                    gmax = gx;
            }
        }
        return (gmax);
    }
}


// Split multi-contact devices into a network of two-terminal devices
// applies to resistor-like devices with "..." terminating CONTACT line.
//
void
cGroupDesc::split_devices()
{
    const char *msg = "RLsolver (split_devices) error: %s";
    for (sDevList *d = gd_devices; d; d = d->next()) {
        for (sDevPrefixList *p = d->prefixes(); p; p = p->next()) {
            if (!p->devs()->desc()->variable_conts())
                continue;
            int ixc = 0;
            sDevInst *dp = 0, *dn;
            for (sDevInst *di = p->devs(); di; di = dn) {
                dn = di->next();
                int nc = di->count_contacts();
                if (nc > 2) {
                    float *rvals = 0, *lvals = 0;
                    if (di->desc()->find_prim(Mkw.resistance)) {
                        RLsolver *r = di->setup_squares(false);
                        if (r) {
                            int rvsize;
                            if (!r->solve_multi(&rvsize, &rvals)) {
                                ExtErrLog.add_dev_err(
                                    gd_celldesc, di, msg,
                                    Errs()->get_error());
                            }
                            delete r;
                        }
                    }
                    double rgmaxval = findmax(rvals, nc);
                    if (di->desc()->find_prim(Mkw.inductance)) {
                        RLsolver *r = di->setup_squares(true);
                        if (r) {
                            int lvsize;
                            if (!r->solve_multi(&lvsize, &lvals)) {
                                ExtErrLog.add_dev_err(
                                    gd_celldesc, di, msg,
                                    Errs()->get_error());
                            }
                            delete r;
                        }
                    }
                    double lgmaxval = findmax(lvals, nc);
                    di->set_index(-1);
                    di->set_next(0);
                    unlink_contacts(di);
                    sDevInst *d0 = 0, *de = 0;
                    int i = 0;
                    for (sDevContactInst *c1 = di->contacts(); c1;
                            i++, c1 = c1->next()) {
                        int j = i+1;
                        for (sDevContactInst *c2 = c1->next(); c2;
                                j++, c2 = c2->next()) {
                            // don't add ridiculously high values
                            if (rvals) {
                                double tmp = -rvals[i*nc + j];
                                if (tmp * 100.0 < rgmaxval)
                                    continue;
                            }
                            if (lvals) {
                                double tmp = -lvals[i*nc + j];
                                if (tmp * 100.0 < lgmaxval)
                                    continue;
                            }
                            sDevInst *dx = new sDevInst(ixc++, di->desc(),
                                gd_celldesc);
                            dx->set_contacts(new sDevContactInst(*c1));
                            dx->contacts()->set_desc(dx->desc()->contacts());
                            dx->contacts()->set_dev(dx);
                            link_contact(dx->contacts());
                            dx->contacts()->set_next(
                                new sDevContactInst(*c2));
                            dx->contacts()->next()->set_desc(
                                dx->desc()->contacts()->next());
                            dx->contacts()->next()->set_dev(dx);
                            link_contact(dx->contacts()->next());
                            if (!d0)
                                d0 = de = dx;
                            else {
                                de->set_next(dx);
                                de = de->next();
                            }
                            dx->set_multi_devs(di);
                            if (dx == d0)
                                dx->set_mstatus(MS_SPLIT);
                            else
                                dx->set_mstatus(MS_SPLIT_NOFREE);
                            dx->set_BB(dx->contacts()->cBB());
                            dx->add_BB(dx->contacts()->next()->cBB());
                            dx->set_bBB(dx->BB());
                            int spc = (rvals ? 1 : 0) + (lvals ? 1 : 0) + 1;
                            dx->set_precmp(new sPreCmp[spc]);
                            spc = 0;
                            if (rvals) {
                                dx->precmp()[spc].which = Mkw.resistance;
                                dx->precmp()[spc].val = -1.0/rvals[i*nc + j];
                                spc++;
                            }
                            if (lvals) {
                                dx->precmp()[spc].which = Mkw.inductance;
                                dx->precmp()[spc].val = -1.0/lvals[i*nc + j];
                                spc++;
                            }
                        }
                    }
                    if (!dp)
                        p->set_devs(d0);
                    else
                        dp->set_next(d0);
                    de->set_next(dn);
                    dp = de;
                    delete [] rvals;
                    delete [] lvals;
                    continue;
                }
                di->set_index(ixc++);
                dp = di;
            }
        }
    }
}


// Do the post-extraction device merging.
//
void
cGroupDesc::combine_devices()
{
    for (sDevList *d = gd_devices; d; d = d->next()) {
        for (sDevPrefixList *p = d->prefixes(); p; p = p->next()) {
            combine_parallel(p);
            for (;;) {
                if (!combine_series(p))
                    break;
                if (!combine_parallel(p))
                    break;
            }
        }
    }
    // We assert here that devices that have been removed from the
    // main list due to combining have their contacts deleted from the
    // ge_groups[].device_contacts list.
}


// Ensure consistency between the device:xicdata LPP and the extracted
// devices.  Called after device extraction.
//
bool
cGroupDesc::setup_dev_layer()
{
    CDl *ld = CDldb()->newLayer(EXT_DEV_LPP, Physical);

    {
        // Loop through boxes and set the group number to 0.

        CDg gdesc;
        gdesc.init_gen(gd_celldesc, ld);
        CDo *od;
        while ((od = gdesc.next()) != 0)
            od->set_group(0);
    }

    // Go through the devices, check for a corresponding box, create
    // as needed.  Give the box a nonzero group number.

    bool ret = true;
    for (sDevList *dv = gd_devices; dv; dv = dv->next()) {
        for (sDevPrefixList *p = dv->prefixes(); p; p = p->next()) {
            for (sDevInst *di = p->devs(); di; di = di->next()) {
                if (!di->setup_dev_layer(ld))
                    ret = false;
            }
        }
    }

    {
        // Loop through the boxes, destroy any with group number 0.

        CDol *ol = 0;
        CDg gdesc;
        gdesc.init_gen(gd_celldesc, ld);
        CDo *od;
        while ((od = gdesc.next()) != 0) {
            if (od->group() == 0)
                ol = new CDol(od, ol);
        }
        while (ol) {
            CDol *ox = ol;
            ol = ol->next;
            gd_celldesc->unlink(ox->odesc, false);
            delete ox;
        }
    }
    return (ret);
}


// For each device instance, save the measure results in a
// XICP_MEASURES property.
//
bool
cGroupDesc::update_measure_prpty()
{
    bool ret = true;
    for (sDevList *dv = gd_devices; dv; dv = dv->next()) {
        for (sDevPrefixList *p = dv->prefixes(); p; p = p->next()) {
            for (sDevInst *di = p->devs(); di; di = di->next()) {
                if (!di->save_measures_in_prpty())
                    ret = false;
            }
        }
    }
    return (ret);
}


// Build the device list and set contact groups.
//
XIrt
cGroupDesc::add_devs()
{
    if (EX()->isVerbosePromptline()) {
        PL()->ShowPromptV("Extracting devices in %s...",
            gd_celldesc->cellname()->string());
    }

    sDevList *dv;
    CDcbin cbin(gd_celldesc);
    XIrt ret = EX()->getDevlist(&cbin, &dv);
    if (ret != XIok)
        return (ret);
    gd_devices = dv;

    for (sDevList *d = gd_devices; d; d = d->next()) {
        for (sDevPrefixList *p = d->prefixes(); p; p = p->next()) {

            // set the contact groups
            for (sDevInst *di = p->devs(); di; di = di->next()) {
                sDevContactInst *ci = di->contacts();
                for ( ; ci; ci = ci->next())
                    find_contact_group(ci, false);
            }

            // Replace the finds with a pointer to the instance in the list.
            //
            for (sDevInst *di = p->devs(); di; di = di->next()) {
                if (!di->fnum())
                   break;
                for (int i = 0; i < di->fnum(); i++) {
                    sDevInst *df = gd_devices->find_in_list(di->fdevs()[i]);
                    delete di->fdevs()[i];
                    di->fdevs()[i] = df;
                    if (!df) {
                        ExtErrLog.add_err(
                            "In %s, add_devs: can't find reference %d in %s %d",
                            gd_celldesc->cellname()->string(),
                            i, di->desc()->name()->string(), di->index());
                    }
                }
            }
        }
    }
    return (XIok);
}


// Look through the device instances and link together those that are
// connected in parallel.  The parallel devices are linked to the
// di_multi_devs field and removed from the main list.  It is
// important to also remove the contacts from the device_contacts list
// (unlink_contacts() does this).
//
bool
cGroupDesc::combine_parallel(sDevPrefixList *p)
{
    if (EX()->isNoMergeParallel())
        return (false);
    if (!p->devs()->desc()->merge_parallel())
        return (false);
    bool didone = false;
    int cnt = 0;  // renumber devices
    for (sDevInst *di = p->devs(); di; di = di->next()) {
        di->set_index(cnt++);
        if (EX()->isNoMergeShorted() && di->is_shorted())
            continue;
        if (di->no_merge())
            continue;
        sDevInst *dp = di, *dn;
        for (sDevInst *dj = di->next(); dj; dj = dn) {
            dn = dj->next();
            if (di->is_parallel(dj)) {
                ExtErrLog.add_log(ExtLogExt, "Merge parallel %s %d %s %d.",
                    di->desc()->name(), di->index(),
                    dj->desc()->name(), dj->index());
                // dj has been permuted if necessary
                dp->set_next(dn);
                dj->set_next(0);
                di->insert_parallel(dj);
                unlink_contacts(dj);
                didone = true;
                continue;
            }
            dp = dj;
        }
    }
    return (didone);
}


// Remove linkage of device contacts into groups.
//
void
cGroupDesc::unlink_contacts(sDevInst *di)
{
    for (sDevContactInst *ci = di->contacts(); ci; ci = ci->next()) {
        int grp = ci->group();
        if (grp >= 0 && grp < gd_asize) {
            sDevContactList *cp = 0;
            for (sDevContactList *dc = gd_groups[grp].device_contacts(); dc;
                    dc = dc->next()) {
                if (dc->contact() == ci) {
                    if (!cp)
                        gd_groups[grp].set_device_contacts(dc->next());
                    else
                        cp->set_next(dc->next());
                    delete dc;
                    break;
                }
                cp = dc;
            }
        }
    }
}


// Combine series devices.  This can't be called until the entire
// hierarchy is initially extracted, since the node between series
// devices may be a contact point.  This also removes/updates the
// contacts in the device_contacts list.
//
bool
cGroupDesc::combine_series(sDevPrefixList *p)
{
    if (EX()->isNoMergeSeries())
        return (false);
    if (!p->devs()->desc()->merge_series())
        return (false);
    bool didone = false;
    for (int i = 0; i < gd_asize; i++) {
        if (gd_groups[i].termlist() || gd_groups[i].subc_contacts())
            continue;
        sDevContactList *dc = gd_groups[i].device_contacts();
        if (!dc || !dc->next() || dc->next()->next())
            continue;
        // exactly 2 contacts
        sDevContactInst *c1 = dc->contact();
        sDevContactInst *c2 = dc->next()->contact();
        if (c1->dev()->desc() != p->devs()->desc())
            continue;
        if (c2->dev()->desc() != p->devs()->desc())
            continue;
        if (c1->dev() == c2->dev())
            continue;
        if (c1->dev()->no_merge() || c2->dev()->no_merge())
            continue;

        sDevContactInst *c1x;
        if (c1 == c1->dev()->contacts())
            c1x = c1->dev()->contacts()->next();
        else
            c1x = c1->dev()->contacts();
        sDevContactInst *c2x;
        if (c2 == c2->dev()->contacts())
            c2x = c2->dev()->contacts()->next();
        else
            c2x = c2->dev()->contacts();
        if (c1x->group() < 0 || c2x->group() < 0)
            continue;
        if (c1x->group() == c2x->group())
            continue;

        int cnt = 0;  // renumber devices
        bool done = false;
        for (sDevInst *di = p->devs(); di; di = di->next()) {
            di->set_index(cnt++);
            if (done)
                continue;
            if (di == c1->dev() || di == c2->dev()) {
                if (di == c2->dev()) {
                    sDevContactInst *ctmp = c2;
                    c2 = c1;
                    c1 = ctmp;
                    ctmp = c2x;
                    c2x = c1x;
                    c1x = ctmp;
                }
                sDevInst *dp = di;
                for (sDevInst *dj = di->next(); dj; dj = dj->next()) {
                    if (dj == c2->dev()) {
                        dp->set_next(dj->next());
                        dj->set_next(0);
                        break;
                    }
                    dp = dj;
                }
                c1->dev()->insert_series(c2->dev());
                done = true;
            }
        }

        // fix the contact links
        delete dc->next();
        delete dc;
        gd_groups[i].set_device_contacts(0);

        if (c2x->group() >= 0 && c2x->group() < gd_asize) {
            for (sDevContactList *cd =
                    gd_groups[c2x->group()].device_contacts();
                    cd; cd = cd->next()) {
                if (cd->contact() == c2x) {
                    cd->set_contact(c1);
                    c1->set_group(c2x->group());
                    c1->set_pnode(c2x->pnode());
                    break;
                }
            }
        }
        didone = true;
    }
    return (didone);
}
// End of cGroupDesc functions


char *
sEinstList::instance_name() const
{
    return (el_cdesc->getInstName(el_vec_ix));
}


// Return true if el is the same type of device or subcircuit,
// connected in parallel.  This does not check permutations.
//
bool
sEinstList::is_parallel(const sEinstList *el) const
{
    if (!el || el->cdesc()->masterCell() != el_cdesc->masterCell())
        return (false);

    unsigned int maxix = 0;
    CDp_cnode *pc0 = (CDp_cnode*)el_cdesc->prpty(P_NODE);
    for (CDp_cnode *pc = pc0; pc; pc = pc->next()) {
        if (pc->index() > maxix)
            maxix = pc->index();
    }
    CDp_cnode **ary = new CDp_cnode*[maxix + 1];
    memset(ary, 0, (maxix+1)*sizeof(CDp_cnode*));
    CDp_range *pr = 0;
    if (el_vec_ix > 0) {
        pr = (CDp_range*)el_cdesc->prpty(P_RANGE);
        if (!pr) {
            delete [] ary;
            return (false);
        }
    }
    for ( ; pc0; pc0 = pc0->next()) {
        CDp_cnode *pc;
        if (el_vec_ix > 0)
            pc = pr->node(0, el_vec_ix, pc0->index());
        else
            pc = pc0;
        if (!pc)
            return (false);
        ary[pc->index()] = pc;
    }

    pc0 = (CDp_cnode*)el->cdesc()->prpty(P_NODE);
    pr = 0;
    if (el->cdesc_index() > 0) {
        pr = (CDp_range*)el->cdesc()->prpty(P_RANGE);
        if (!pr) {
            delete [] ary;
            return (false);
        }
    }
    bool ret = true;
    for ( ; pc0; pc0 = pc0->next()) {
        CDp_cnode *pc;
        if (el->cdesc_index() > 0)
            pc = pr->node(0, el->cdesc_index(), pc0->index());
        else
            pc = pc0;
        if (!pc || ary[pc->index()]->enode() != pc->enode()) {
            ret = false;
            break;
        }
    }
    delete [] ary;
    return (ret);
}


namespace {
    int count_parallel(const sEinstList *el)
    {
        int cnt = 1;
        for (const sEinstList *e = el->sections(); e; e = e->next())
            cnt += count_parallel(e);
        return (cnt);
    }
}


// Compose a SPICE-like value/parameter line from the properties.  The
// strings used to create a local parameter table which can be used to
// evaluate expressions containing the parameters.  The "leadval" is
// an unlabeled leading number (as in SPICE).
//
void
sEinstList::setup_eval(sParamTab **tret, double **dret) const
{
    if (tret)
        *tret = 0;
    if (dret)
        *dret = 0;

    // Count the number of "sections" which are parallel devices.
    //
    // ASSUME for now THAT ALL SECTIONS ARE IDENTICAL.

    int esects = count_parallel(this);

    sLstr lstr;
    CDp_user *pv = (CDp_user*)el_cdesc->prpty(P_VALUE);
    if (pv) {
        char *string = pv->data()->string(HYcvPlain, true);
        if (EX()->paramCx())
            EX()->paramCx()->update(&string);
        lstr.add(string);
        delete [] string;
    }

    CDp_user *pp = (CDp_user*)el_cdesc->prpty(P_PARAM);
    if (pp) {
        char *string = pp->data()->string(HYcvPlain, true);
        if (EX()->paramCx())
            EX()->paramCx()->update(&string);
        if (lstr.string())
            lstr.add_c(' ');
        lstr.add(string);
        delete [] string;
    }

    // Strip off the leading number, if any.
    double *leadval = 0;
    sParamTab *ptab = 0;
    const char *t = lstr.string();
    if (t) {
        leadval = SPnum.parse(&t, false);
        while (isspace(*t))
            t++;

        // Build a parameter table from the string.
        if (*t) {
            ptab = new sParamTab;
            ptab->extract_params(t);
        }
    }

    if (esects > 1) {
        // For now, we will assume that the "M" parameter applies to
        // the device, and add/modify this to account for the
        // sections.

        char buf[128];
        if (ptab) {
            if (ptab->get("M")) {
                sprintf(buf, "'%d*M'", esects);
                char *str = lstring::copy(buf);
                ptab->param_subst_all_collapse(&str);
                sprintf(buf, "M=%s", str);
                delete [] str;
                ptab->extract_params(buf);
            }
            else if (EX()->paramCx() && EX()->paramCx()->has_param("M")) {
                sprintf(buf, "'%d*M'", esects);
                char *str = lstring::copy(buf);
                EX()->paramCx()->update(&str);
                sprintf(buf, "M=%s", str);
                delete [] str;
                ptab->extract_params(buf);
            }
            else {
                sprintf(buf, "M=%d", esects);
                ptab->extract_params(buf);
            }
        }
        else {
            ptab = new sParamTab;
            sprintf(buf, "M=%d", esects);
            ptab->extract_params(buf);
        }
    }
    if (tret)
        *tret = ptab;
    else
        delete ptab;
    if (dret)
        *dret = leadval;
}


namespace {
    bool et_comp(const sEinstList *e1, const sEinstList *e2)
    {
        const char *n1 = e1->cdesc()->getBaseName();
        const char *n2 = e2->cdesc()->getBaseName();
        int r = strcmp(n1, n2);
        if (r < 0)
            return (true);
        if (r > 0)
            return (false);
        CDp_name *pn1 = (CDp_name*)e1->cdesc()->prpty(P_NAME);
        CDp_name *pn2 = (CDp_name*)e2->cdesc()->prpty(P_NAME);
        unsigned i1 = pn1 ? pn1->number() : 0;
        unsigned i2 = pn2 ? pn2->number() : 0;
        if (i1 != i2)
            return (i1 < i2);
        return (e1->cdesc_index() < e2->cdesc_index());
    }
}


// Sort the list by name, index, and vec-index.
//
sEinstList *
sEinstList::sort()
{
    int cnt = 0;
    for (sEinstList *e = this; e; e = e->next(), cnt++) ;
    if (cnt < 2)
        return (this);
    sEinstList **ary = new sEinstList*[cnt];
    cnt = 0;
    for (sEinstList *e = this; e; e = e->next())
        ary[cnt++] = e;

    std::sort(ary, ary + cnt, et_comp);
    cnt--;
    for (int i = 0; i < cnt; i++)
        ary[i]->set_next(ary[i+1]);
    ary[cnt]->set_next(0);
    sEinstList *eret = ary[0];
    delete [] ary;
    return (eret);
}


// Return an array containing the node properties in index order, and
// the array size in the argument.
//
const CDp_cnode * const*
sEinstList::nodes(unsigned int *psize)
{
    *psize = 0;
    CDp_cnode *pc0 = (CDp_cnode*)el_cdesc->prpty(P_NODE);
    if (!pc0)
        return (0);
    unsigned int imx = 0;
    for (CDp_cnode *pc = pc0; pc; pc = pc->next()) {
        if (pc->index() > imx)
            imx = pc->index();
    }
    imx++;

    CDp_range *pr = 0;
    if (el_vec_ix > 0) {
        pr = (CDp_range*)el_cdesc->prpty(P_RANGE);
        if (!pr)
            return (0);
        if (!pr->node(0, el_vec_ix, 0))
            return (0);
    }

    CDp_cnode **ary = new CDp_cnode*[imx];
    memset(ary, 0, imx*sizeof(CDp_cnode*));
    for (CDp_cnode *pc = pc0; pc; pc = pc->next()) {
        if (el_vec_ix > 0)
            ary[pc->index()] = pr->node(0, el_vec_ix, pc->index());
        else
            ary[pc->index()] = pc;
    }
    *psize = imx;
    return (ary);
}


// If dd and this provide two permutable contacts, load the indices
// into the buffer and pass back a pointer to it.
//
const int *
sEinstList::permutes(const sDevDesc *dd, int *bf)
{
    if (!dd || !bf)
        return (0);
    bf[0] = -1;
    bf[1] = -1;
    if (!dd->permute_cont1())
        return (0);
    CDp_cnode *pc = (CDp_cnode*)el_cdesc->prpty(P_NODE);
    for ( ; pc; pc = pc->next()) {
        if (pc->get_term_name() == dd->permute_cont1()) {
            bf[0] = pc->index();
            if (bf[0] >= 0 && bf[1] >= 0)
                return (bf);
        }
        if (pc->get_term_name() == dd->permute_cont2()) {
            bf[1] = pc->index();
            if (bf[0] >= 0 && bf[1] >= 0)
                return (bf);
        }
    }
    return (0);
}
// End of sEinstList functions.


// Return null if this has the same signature as cref, otherwise
// return a message describing the difference.  The signiture is the
// contact name and and a couple of attributes.  Contacts for devices
// of the same name must have the same order and signature.
//
char *
sDevContactDesc::checkEquiv(const sDevContactDesc *cref)
{
    if (!cref)
        return (0);
    char buf[256];
    if (c_name != cref->c_name) {
        snprintf(buf, 256, "contact name is %s, expecting %s",
            c_name ? c_name->string() : "null",
            cref->c_name ? cref->c_name->string() : "null");
        return (lstring::copy(buf));
    }
    if (c_bulk != cref->c_bulk) {
        snprintf(buf, 256, "%s contact %s, expecting %s",
            c_bulk ? "bulk" : "non-bulk", c_name ? c_name->string() : "null",
            cref->c_bulk ? "bulk" : "non-bulk");
        return (lstring::copy(buf));
    }
    if (c_multiple != cref->c_multiple) {
        snprintf(buf, 256, "%s contact %s, expecting %s",
            c_multiple ? "multiple" : "non-multiple",
            c_name ? c_name->string() : "null",
            cref->c_multiple ? "multiple" : "non-multiple");
        return (lstring::copy(buf));
    }
    return (0);
}


// Initialization for contact specification, check the given values.
//
bool
sDevContactDesc::init()
{
    const char *what = c_bulk ? "BulkContact" : "Contact";
    if (!c_name) {
        if (!c_name_gvn || !*c_name_gvn) {
            Errs()->add_error("%s with no name.", what);
            return (false);
        }
        c_name = CDnetex::name_tab_add(c_name_gvn);
    }
    if (!c_netname && c_bulk) {
        if (!c_netname_gvn || !*c_netname_gvn) {
            if (c_level != BC_immed) {
                Errs()->add_error("%s %s, no bulk net name given.",
                    what, c_name_gvn);
                return (false);
            }
        }
        else
            c_netname = CDnetex::name_tab_add(c_netname_gvn);
    }

    if (!c_lspec.setup()) {
        Errs()->add_error("%s %s: %s.", what, c_name_gvn, Errs()->get_error());
        return (false);
    }
    return (true);
}


// Cache the property index numbers in the contact descriptors.
//
bool
sDevContactDesc::setup_contact_indices(const sDevDesc *dd)
{
    CDs *esd = CDcdb()->findCell(dd->name(), Electrical);
    CDp_snode *ps0 = esd ? (CDp_snode*)esd->prpty(P_NODE) : 0;
    if (!ps0) {
        if (!esd)
            Errs()->add_error("Device %s, can't open electrical cell,",
                dd->name()->string());
        else
            Errs()->add_error("Device %s, electrical cell has no nodes,",
                dd->name()->string());
        for (sDevContactDesc *c = this; c; c = c->next())
            c->c_el_index = -1;
        return (false);
    }

    for (sDevContactDesc *c = this; c; c = c->next()) {
        c->c_el_index = -1;
        // This block used to be done only if !dd->variable_conts().
        for (CDp_snode *ps = ps0; ps; ps = ps->next()) {
            if (ps->term_name() == c->name()) {
                c->c_el_index = ps->index();
                break;
            }
        }
        if (c->c_el_index < 0) {
            Errs()->add_error(
                "Physical device %s contact %s not found in "
                "electrical cell.", dd->name()->string(),
                c->name()->string());
            for (c = this; c; c = c->next())
                c->c_el_index = -1;
            return (false);
        }
    }
    return (true);
}


// Static function.
// <tname> <layername> <expression> <...>
// Create a new contact from the string and link it into d, returning 0.  On
// error, return an error message (needs to be freed).
//
char *
sDevContactDesc::parse_contact(const char **line, sDevDesc *d)
{
    char buf[256];
    sDevContactDesc *c = new sDevContactDesc;
    c->c_name_gvn = lstring::gettok(line);
    c->c_lname = lstring::gettok(line);
    if (c->c_lname) {
        char *t = strmdata::dec_to_hex(c->c_lname);
        // handle "layer,datatype"
        if (t) {
            delete [] c->c_lname;
            c->c_lname = t;
        }
    }
    if (!c->c_name_gvn || !c->c_lname) {
        sprintf(buf,
            "Syntax error, no contact name, contact spec in Device %s "
            "block,\nline %d.\n", d->name()->stringNN(), Tech()->LineCount());
        delete c;
        return (lstring::copy(buf));
    }
    if (!c->c_lspec.parseExpr(line)) {
        sLstr lstr;
        sprintf(buf,
            "Layer expression parse error, contact spec in Device %s "
            "block,\nline %d:\n", d->name()->stringNN(),
            Tech()->LineCount());
        lstr.add(buf);
        lstr.add(Errs()->get_error());
        lstr.add_c('\n');
        delete c;
        return (lstr.string_trim());
    }

    if (lstring::prefix("...", *line)) {
        if (!d->contacts()) {
            sprintf(buf,
                "Error, contact spec in Device %s block, \"...\" "
                "found in first contact,\nline %d.\n",
                d->name()->stringNN(), Tech()->LineCount());
            delete c;
            return (lstring::copy(buf));
        }
        c->c_multiple = true;
        d->set_variable_conts(true);
    }
    if (!d->contacts())
        d->set_contacts(c);
    else {
        sDevContactDesc *ctmp = d->contacts();
        while (ctmp->c_next)
            ctmp = ctmp->c_next;
        ctmp->c_next = c;
    }
    return (0);
}


// Static function.
// Create a new bulk contact from the string and link it into d,
// returning 0.  On error, return an error message (needs to be
// freed).  There are presently three forms:
//
// BulkContact <tname> [immed] <bloat> <layername> <expression>]
//   level = 0
//   Check in cell, requires bloat, layername, and expression.
//   The body BB is bloated.  A region where expression is
//   true that is connected to the layername must exist.  The
//   area closest to the center is taken as the contact.  If
//   no such area, the device will not be recognized.  The
//   search depth for the contact is all levels.
//
// BulkContact <tname> skip <globname>
//   level = 1
//   Ignore in extraction, assume connected to <globname>, <globname>
//   is a required global scalar net name in the schematic.
//
// BulkContact <tname> defer <globname> <bloat> <layername> <expression>
//   level = 2
//   Check, requires globname, bloat, layername, and expression.
//   A second pass is made in extraction.  On the first pass, contacts
//   are assumed connected to globname.  In the second pass, contacts
//   are sought from the top level.  It is an error if no contact is
//   found.
//
char *
sDevContactDesc::parse_bulk_contact(const char **line, sDevDesc *d)
{
    char buf[256];
    sDevContactDesc *c = new sDevContactDesc;
    c->c_name_gvn = lstring::gettok(line);
    if (!c->c_name_gvn) {
        sprintf(buf,
            "Error, missing terminal name in Device %s block, line %d.\n",
            d->name()->stringNN(), Tech()->LineCount());
        delete c;
        return (lstring::copy(buf));
    }
    c->c_bulk = true;

    char *tok = lstring::gettok(line);
    if (!tok) {
        sprintf(buf,
            "Error, missing text in bulk contact spec in "
            "Device %s block, line %d.\n",
            d->name()->stringNN(), Tech()->LineCount());
        delete c;
        return (lstring::copy(buf));
    }

    bool gotlev = true;
    if (lstring::cieq(tok, Ekw.skip()))
        c->c_level = BC_skip;
    else if (lstring::cieq(tok, Ekw.defer()))
        c->c_level = BC_defer;
    else if (lstring::cieq(tok, Ekw.immed()))
        c->c_level = BC_immed;
    else
        gotlev = false;
    if (gotlev) {
        delete [] tok;
        if (c->c_level == BC_skip || c->c_level == BC_defer) {
            tok = lstring::gettok(line);
            if (!tok) {
                sprintf(buf,
                    "Error, missing net name in bulk contact spec in "
                    "Device %s block, line %d.\n",
                    d->name()->stringNN(), Tech()->LineCount());
                delete c;
                return (lstring::copy(buf));
            }
            c->c_netname_gvn = tok;
        }
        if (c->c_level == BC_skip)
            goto done;

        tok = lstring::gettok(line);
        if (!tok) {
            sprintf(buf,
                "Error, missing bloat value in bulk contact spec in "
                "Device %s block, line %d.\n",
                d->name()->stringNN(), Tech()->LineCount());
            delete c;
            return (lstring::copy(buf));
        }
    }

    // The tok should be a bloat number.
    if (sscanf(tok, "%f", &c->c_bulk_bloat) != 1 ||
            c->c_bulk_bloat < 0.0) {
        sprintf(buf,
            "Error, bad bloat value in bulk contact spec in "
            "Device %s block, line %d.\n",
            d->name()->stringNN(), Tech()->LineCount());
        delete c;
        delete [] tok;
        return (lstring::copy(buf));
    }
    delete [] tok;

    c->c_lname = lstring::gettok(line);
    if (c->c_lname) {
        char *t = strmdata::dec_to_hex(c->c_lname);
        // handle "layer,datatype"
        if (t) {
            delete [] c->c_lname;
            c->c_lname = t;
        }
    }
    if (!c->c_lname) {
        sprintf(buf,
            "Syntax error, no layer name, contact spec in Device %s block,\n"
            "line %d.\n", d->name()->stringNN(), Tech()->LineCount());
        delete c;
        return (lstring::copy(buf));
    }
    if (!c->c_lspec.parseExpr(line)) {
        sLstr lstr;
        sprintf(buf,
            "Syntax error, no layer name, contact spec in Device %s block,\n"
            "line %d:\n", d->name()->stringNN(), Tech()->LineCount());
        lstr.add(buf);
        lstr.add(Errs()->get_error());
        lstr.add_c('\n');
        delete c;
        return (lstr.string_trim());
    }

done:
    if (!d->contacts())
        d->set_contacts(c);
    else {
        sDevContactDesc *ctmp = d->contacts();
        while (ctmp->c_next)
            ctmp = ctmp->c_next;
        ctmp->c_next = c;
    }
    return (0);
}
// End of sDevContactDesc functions.


// Return the node corresponding to this contact, or -1 if unknown. 
// The optional argument can be used to fake a dual if needed.
//
int
sDevContactInst::node(const sEinstList *el) const
{
    if (!el)
        el = dev()->dual();
    if (!el)
        return (-1);
    CDc *cd = el->cdesc();
    int index = desc()->elec_index();
    if (index < 0) {
        // Somehow the elec_index wasn't set, hunt for it the hard
        // way.

        CDs *sd = cd->masterCell(true);
        if (!sd)
            return (-1);
        CDp_snode *ps = (CDp_snode*)sd->prpty(P_NODE);
        for ( ; ps; ps = ps->next()) {
            if (ps->term_name() == desc()->name()) {
                index = ps->index();
                break;
            }
        }
        if (index < 0)
            return (-1);
    }

    unsigned int vix = el->cdesc_index();
    if (vix > 0) {
        CDp_range *pr = (CDp_range*)cd->prpty(P_RANGE);
        if (pr) {
            CDp_cnode *pc = pr->node(0, vix, index);
            if (pc)
                return (pc->enode());
        }
    }
    else {
        CDp_cnode *pc = (CDp_cnode*)cd->prpty(P_NODE);
        for ( ; pc; pc = pc->next()) {
            if (pc->index() == (unsigned int)index)
                return (pc->enode());
        }
    }
    return (-1);
}


// Return the node property corresponding to this contact, or null if
// unknown.  The optional argument can be used to fake a dual if
// needed.
//
CDp_cnode *
sDevContactInst::node_prpty(const sEinstList *el) const
{
    if (!el)
        el = dev()->dual();
    if (!el)
        return (0);
    CDc *cd = el->cdesc();
    int index = desc()->elec_index();
    if (index < 0) {
        // Somehow the elec_index wasn't set, hunt for it the hard
        // way.

        CDs *sd = cd->masterCell(true);
        if (!sd)
            return (0);
        CDp_snode *ps = (CDp_snode*)sd->prpty(P_NODE);
        for ( ; ps; ps = ps->next()) {
            if (ps->term_name() == desc()->name()) {
                index = ps->index();
                break;
            }
        }
        if (index < 0)
            return (0);
    }

    unsigned int vix = el->cdesc_index();
    if (vix > 0) {
        CDp_range *pr = (CDp_range*)cd->prpty(P_RANGE);
        if (pr)
            return (pr->node(0, vix, index));
    }
    else {
        CDp_cnode *pc = (CDp_cnode*)cd->prpty(P_NODE);
        for ( ; pc; pc = pc->next()) {
            if (pc->index() == (unsigned int)index)
                return (pc);
        }
    }
    return (0);
}


// Copy the contact list, providing the device pointer passed.
//
sDevContactInst *
sDevContactInst::dup_list(sDevInst *di) const
{
    sDevContactInst *c0 = 0, *ce = 0;
    for (const sDevContactInst *c = this; c; c = c->next()) {
        sDevContactInst *cnew = new sDevContactInst(*c);
        cnew->ci_dev = di;
        if (!c0)
            c0 = ce = cnew;
        else {
            ce->set_next(cnew);
            ce = ce->next();
        }
    }
    return (c0);
}


// Draw a box around the contact BB, or add to the eBB.
//
void
sDevContactInst::show(WindowDesc *wdesc, BBox *eBB) const
{
    if (!cont_ok())
        return;
    if (eBB)
        eBB->add(&ci_BB);
    else
        wdesc->ShowBox(&ci_BB, 0, 0);

    if (!ci_dev->desc()->overlap_conts()) {
        int delta = (int)(0.5*DSP_DEF_PTRM_TXTHT/wdesc->Ratio());
        // log scale size
        if (wdesc->Ratio() < .78) {
            double t1 = -5.0/log(wdesc->Ratio());
            delta = (int)(delta*t1);
        }
        else
            delta *= 20;

        int dim = ci_dev->BB()->height();
        if (ci_dev->BB()->width() < dim)
            dim = ci_dev->BB()->width();
        dim /= 2;
        if (dim > 0 && dim < delta)
            delta = dim;

        int x, y, w, h;
        DSP()->DefaultLabelSize(cont_name()->string(), wdesc->Mode(), &w, &h);
        w = (w*delta)/h;
        h = delta;
        x = (ci_BB.left + ci_BB.right - w)/2;
        y = (ci_BB.bottom + ci_BB.top - h)/2;
        if (eBB) {
            BBox tBB(x, y, x+w, y+h);
            eBB->add(&tBB);
        }
        else
            wdesc->ShowLabel(cont_name()->string(), x, y, w, h, 0);
    }
}


// Set the terminal location for the contact, if the terminal is not
// TE_FIXED.  The dual must be set, which should always be true if
// ci_pnode is set.  Set the reference object.
//
void
sDevContactInst::set_term_loc(CDs *sdesc, CDl *ld) const
{
    if (!ci_pnode)
        return;
    if (!cont_ok())
        return;
    CDcterm *term = ci_pnode->inst_terminal();
    if (!term)
        return;

    CDo *odesc = is_set(sdesc, ld);
    if (odesc && odesc->group() == ci_group) {
        term->set_ref(odesc);
        return;
    }
    if (!term->is_fixed()) {
        term->set_loc((ci_BB.left + ci_BB.right)/2,
            (ci_BB.bottom + ci_BB.top)/2);
        odesc = is_set(sdesc, ld);
        if (odesc && odesc->group() == ci_group)
            term->set_ref(odesc);
    }
}


// Return an object on ld under the pn marker.
//
CDo *
sDevContactInst::is_set(CDs *sdesc, CDl *ld) const
{
    if (ld && ci_pnode) {
        if (!cont_ok())
            return (0);
        CDcterm *term = ci_pnode->inst_terminal();
        if (term) {
            int x = term->lx();
            int y = term->ly();
            BBox tBB(x-5, y-5, x+5, y+5);
            cGroupDesc *gd = sdesc->groups();
            if (gd) {
                sGrpGen grg;
                grg.init_gen(gd, ld, &tBB);
                CDo *odesc;
                while ((odesc = grg.next()) != 0) {
                    if (odesc->intersect(&tBB, true))
                        return (odesc);
                }
            }
        }
    }
    return (0);
}
// End of sDevContactInst functions.


//
// Parameter measurement for extracted devices.
//

// Return the requested result.
//
double
sMeasure::measure()
{
    return ((m_result && m_result->type == TYP_SCALAR) ?
        m_result->content.value : 0.0);
}


// Print the result.
//
void
sMeasure::print_result(FILE *fp)
{
    if (m_result)
        fprintf(fp, " %s: %g\n", m_name, m_result->content.value);
}


namespace {
    // Return true if a and b a close to equal.
    //
    inline bool
    tcmp(double a, double b, int dp)
    {
        double u = fabs(a - b);
        if (u == 0)
            return (true);
        double v = 0.5*(fabs(a) + fabs(b));
        for (int i = 0; i < dp; i++)
            v /= 10.0;
        if (u <= v)
            return (true);
        return (false);
    }
}


// Print the result in the form of a comparison.  Errors and ambiguous
// results will increment the counts whose addreses are provided by
// the arguments.  Return the number of comparison errors.
//
int
sMeasure::print_compare(sLstr *lstr, const double *leadval, sParamTab *ptab,
    int *bad, int *amb)
{
    int ecnt = 0;
    if (m_lvsword) {
        const char *msg = "    %-12s:   physical %1.6e, electrical %s";
        const char *nf = "<not found>";
        double a = measure();
        const double *d = 0;

        if (!*m_lvsword) {
            // Empty lvsword, this will match the leadval.
            d = leadval;
        }
        else {
            // The lvsword is either a parameter name, or a
            // single-quoted expression involving constants and
            // parameter names.

            char *string = lstring::copy(m_lvsword);
            if (ptab)
                ptab->param_subst_all_collapse(&string);
            if (EX()->paramCx())
                EX()->paramCx()->update(&string);

            // If substitution and evaluation succeeded, the
            // string should now consist of a single number.
            const char *t = string;
            d = SPnum.parse(&t, true);
            delete [] string;
            if (!d && amb)
                (*amb)++;
        }

        char tbuf[256], nbuf[64];
        if (d)
            sprintf(nbuf, "%1.6e", *d);
        sprintf(tbuf, msg, m_name, a, d ? nbuf : nf);
        lstr->add(tbuf);
        if (d && !tcmp(a, *d, m_precision)) {
            lstr->add("  ** differ\n");
            if (bad)
                (*bad)++;
            ecnt++;
        }
        else
            lstr->add_c('\n');
    }
    return (ecnt);
}


// Print the specification.
//
void
sMeasure::print(FILE *fp)
{
    sLstr lstr;
    m_tree->string(lstr);
    if (m_precision != DEF_PRECISION)
        fprintf(fp, "Measure %s %s %d\n", m_name, lstr.string(), m_precision);
    else
        fprintf(fp, "Measure %s %s\n", m_name, lstr.string());
}
// End of sMeasure functions.


sMprim::sMprim(siVariable *v1, siVariable *v2, sMprim *n)
{
    mp_next = n;
    mp_variable = v1;
    mp_extravar = v2;
    if (v1 && checkname(v1->name, nsect))
        evfunc = &sMprim::sections;
    else if ((v1 && checkname(v1->name, bArea)) ||
            (v2 && checkname(v2->name, bPerim)))
        evfunc = &sMprim::bodyAP;
    else if (v1 && checkname(v1->name, bMinDim))
        evfunc = &sMprim::bodyMinDim;
    else if ((v1 && checkname(v1->name, cArea)) ||
            (v2 && checkname(v2->name, cPerim)))
        evfunc = &sMprim::cAP;
    else if (v1 && checkname(v1->name, cWidth))
        evfunc = &sMprim::cWidth;
    else if (v1 && checkname(v1->name, cnWidth))
        evfunc = &sMprim::cnWidth;
    else if (v1 && checkname(v1->name, cbWidth))
        evfunc = &sMprim::cbWidth;
    else if (v1 && checkname(v1->name, cbnWidth))
        evfunc = &sMprim::cbnWidth;
    else if (v1 && checkname(v1->name, resistance))
        evfunc = &sMprim::resistance;
    else if (v1 && checkname(v1->name, inductance))
        evfunc = &sMprim::inductance;
    else if (v1 && checkname(v1->name, mutual))
        evfunc = &sMprim::mutual;
    else if (v1 && checkname(v1->name, capacitance))
        evfunc = &sMprim::capacitance;
    else
        evfunc = &sMprim::nop;
}


// Parse name, which has the form token.token.token.
//
char *
sMprim::split(const char *name, char **a1, char **a2) const
{
    *a1 = 0;
    *a2 = 0;
    char *str = lstring::copy(name);
    for (char *s = str; *s; s++) {
        if (*s == '.') {
            *s = 0;
            if (!*a1)
                *a1 = s+1;
            else if (!*a2)
                *a2 = s+1;
            else {
                *a1 = 0;
                *a2 = 0;
                delete [] str;
                return (0);
            }
        }
    }
    return (str);
}


// Split name, returning the integer values for the args, or -1 if the arg
// is not an integer.
//
void
sMprim::setnums(const char *name, int *n1, int *n2) const
{
    *n1 = -1;
    *n2 = -1;
    char *a1, *a2;
    char *str = split(name, &a1, &a2);
    if (a1 && isdigit(*a1))
        *n1 = *a1 - '0';
    if (a2 && isdigit(*a2))
        *n2 = *a2 - '0';
    delete [] str;
}


bool
sMprim::sections(sDevInst *di)
{
    if (mp_variable) {
        mp_variable->type = TYP_SCALAR;
        if (!di->find_precmp(Mkw.nsect, &mp_variable->content.value))
            mp_variable->content.value = di->count_sections();
    }
    return (true);
}


bool
sMprim::bodyAP(sDevInst *di)
{
    // BodyArea (no args)
    // BodyPerim (no args)
    // mp_variable takes the area, mp_extravar takes the perimeter

    bool do_area = (mp_variable != 0);
    if (do_area && di->find_precmp(Mkw.bArea, &mp_variable->content.value)) {
        mp_variable->type = TYP_SCALAR;
        do_area = false;
    }
    bool do_perim = (mp_extravar != 0);
    if (do_perim && di->find_precmp(Mkw.bPerim, &mp_extravar->content.value)) {
        mp_extravar->type = TYP_SCALAR;
        do_perim = false;
    }
    if (!do_area && !do_perim)
        return (true);

    if (di->mstatus() == MS_PARALLEL || di->mstatus() == MS_SERIES ) {
        // sum areas and perimeters
        double area = 0.0, perim = 0.0;
        for (sDevInst *dx = di->multi_devs(); dx; dx = dx->next()) {
            if (!bodyAP(dx))
                return (false);
            if (do_area)
                area += mp_variable->content.value;
            if (do_perim)
                perim += mp_extravar->content.value;
        }
        if (do_area) {
            mp_variable->content.value = area;
            mp_variable->type = TYP_SCALAR;
        }
        if (do_perim) {
            mp_extravar->content.value = perim;
            mp_extravar->type = TYP_SCALAR;
        }
        return (true);
    }

    if (do_area) {
        mp_variable->content.value = di->barea();
        mp_variable->type = TYP_SCALAR;
    }
    if (do_perim) {
        mp_extravar->content.value = MICRONS(di->bperim());
        mp_extravar->type = TYP_SCALAR;
    }
    return (true);
}


bool
sMprim::bodyMinDim(sDevInst *di)
{
    // BodyMinDimen (no args)
    bool do_it = (mp_variable != 0);
    if (do_it && di->find_precmp(Mkw.bMinDim, &mp_variable->content.value)) {
        mp_variable->type = TYP_SCALAR;
        do_it = false;
    }
    if (!do_it)
        return (true);

    if (di->mstatus() == MS_PARALLEL || di->mstatus() == MS_SERIES ) {
        // find smallest mindim
        double mindim = CDinfinity;
        for (sDevInst *dx = di->multi_devs(); dx; dx = dx->next()) {
            if (!bodyMinDim(dx))
                return (false);
            double a = mp_variable->content.value;
            if (a < mindim)
                mindim = a;
        }
        mp_variable->content.value = mindim;
        mp_variable->type = TYP_SCALAR;
        return (true);
    }

    mp_variable->content.value = MICRONS(di->bmindim());
    mp_variable->type = TYP_SCALAR;
    return (true);
}


namespace {
    // Recursively increment the counts of groups where a contact
    // overlap is found.
    //
    void check_contacts_in_groups(const CDs *sd, const CDl *ld,
        const sDevContactInst *ci, int *carray)
    {
        if (ci->dev()->mstatus() == MS_PARALLEL) {
            for (sDevInst *dx = ci->dev()->multi_devs(); dx;
                    dx = dx->next()) {
                sDevContactInst *cx =
                    dx->find_contact(ci->desc()->name());
                if (cx)
                    check_contacts_in_groups(sd, ld, cx, carray);
            }
        }
        else {
            // The contact is weighted by the maximum dimension of
            // the contact BB.  This just happens to be the finger
            // width for MOS devices.

            CDg gdesc;
            gdesc.init_gen(sd, ld, ci->cBB());
            CDo *od;
            while ((od = gdesc.next()) != 0) {
                if (od->intersect(ci->cBB(), false))
                    carray[od->group()] += ci->cBB()->max_wh();
            }
        }
    }
}


// Find the area and perimeter length of the collection of objects
// in the contact group which form a single group which overlaps
// the contact BB.
//
bool
sMprim::cAP(sDevInst *di)
{
    // CArea.contact_num.layer_name
    // CPerim.contact_num.layer_name
    // variable takes the area, extravar takes the perimeter

    bool do_area = (mp_variable != 0);
    if (do_area && di->find_precmp(Mkw.cArea, &mp_variable->content.value)) {
        mp_variable->type = TYP_SCALAR;
        do_area = false;
    }
    bool do_perim = (mp_extravar != 0);
    if (do_perim && di->find_precmp(Mkw.cPerim, &mp_extravar->content.value)) {
        mp_extravar->type = TYP_SCALAR;
        do_perim = false;
    }
    if (!do_area && !do_perim)
        return (true);

    if (do_area) {
        mp_variable->content.value = 0.0;
        mp_variable->type = TYP_SCALAR;
    }
    if (do_perim) {
        mp_extravar->content.value = 0.0;
        mp_extravar->type = TYP_SCALAR;
    }

    if (di->mstatus() == MS_SERIES)
        // meaningless, return
        return (true);

    int n1 = 0;
    CDl *ld = 0;
    char *s;
    if (mp_variable)
        s = mp_variable->name;
    else if (mp_extravar)
        s = mp_extravar->name;
    else {
        const char *msg = "no variable name given";
        ExtErrLog.add_dev_err(di->celldesc(), di, msg);
        return (false);
    }
    while (isalnum(*s))
        s++;
    if (*s) {
        s++;
        if (*s >= '0' && *s <= '9') {
            n1 = *s - '0';
            s++;
            if (*s && !isalnum(*s)) {
                s++;
                ld = CDldb()->findLayer(s, Physical);
            }
        }
    }
    if (!ld) {
        const char *msg = "no/unknown layer name given";
        ExtErrLog.add_dev_err(di->celldesc(), di, msg);
        return (false);
    }

    sDevContactInst *c1 = di->find_contact(n1);
    if (!c1) {
        const char *msg = "no/unknown contact %d";
        ExtErrLog.add_dev_err(di->celldesc(), di, msg, n1);
        return (false);
    }

    cGroupDesc *gd = di->celldesc()->groups();
    sGroup *g = gd ? gd->group_for(c1->group()) : 0;
    if (!g || !g->net()) {
        const char *msg = "contact %d in virtual group";
        ExtErrLog.add_dev_err(di->celldesc(), di, msg, n1);
        return (false);
    }

    sGroupXf *gx = new sGroupXf(di->celldesc(), g->net(), ld);

    // The list contains all objects on ld that connect to the
    // contact.
    CDol *ol0 = gx->find_objects(c1, ld);

    // Convert to a trapezoid list.
    Zlist *z0 = 0, *ze = 0;
    while (ol0) {
        Zlist *zl = ol0->odesc->toZlist();
        if (!z0)
            z0 = ze = zl;
        else {
            while (ze->next)
                ze = ze->next;
            ze->next = zl;
        }
        CDol *ox = ol0;
        ol0 = ol0->next;
        delete ox;
    }
    // Note that the objects in ol0 may be copies owned by gx.
    delete gx;

    if (!z0)
        return (true);

    // Clip/merge the list, so no trapezoids intersect.
    z0 = Zlist::repartition_ni(z0);

    // Clip the list to the actual contact layer expression.
    SIlexprCx cx(di->celldesc(), CDMAXCALLDEPTH, z0);
    Zlist *zret = 0;
    XIrt xrt = c1->desc()->lspec()->getZlist(&cx, &zret);
    Zlist::free(z0);
    if (xrt != XIok)
        return (false);
    z0 = Zlist::repartition_ni(zret);

    bool perm = di->desc()->is_permute(c1->desc()->name());
    int nterms = 0;
    for (sDevContactList *c = g->device_contacts(); c; c = c->next()) {
        sDevContactInst *ci = c->contact();
        if (ci->dev()->desc() != c1->dev()->desc())
            continue;
        if (c1->desc()->name() != ci->desc()->name() && !(perm &&
                di->desc()->is_permute(ci->desc()->name())))
            continue;
        nterms++;
    }
    if (nterms <= 1) {
        // There are no similar device terminals connected to this
        // terminal, do the calculation and begone.

        if (do_area)
            mp_variable->content.value = Zlist::area(z0);
        if (do_perim) {
            int perim = 0;
            PolyList *pl = Zlist::to_poly_list(z0);
            for (PolyList *p = pl; p; p = p->next)
                perim += p->po.perim();
            pl->free();
            mp_extravar->content.value = MICRONS(perim);
        }
        else
            Zlist::free(z0);
        return (true);
    }

    // The contact may be shared.  Look for similar contacts in the
    // same group for devices of the same type.

    // Separate the trapezoids into mutually-connected groups.
    Zgroup *zg = Zlist::group(z0);

    // We're going to count the contact areas of the present contact,
    // and other contacts, that intersect each group.
    //
    int *counts = new int[zg->num];
    memset(counts, 0, zg->num*sizeof(int));
    int *xcounts = new int[zg->num];
    memset(xcounts, 0, zg->num*sizeof(int));

    CDs *tmpsd = zg->mk_cell(ld);

    for (sDevContactList *c = g->device_contacts(); c; c = c->next()) {
        sDevContactInst *ci = c->contact();
        if (ci->dev()->desc() != c1->dev()->desc())
            continue;
        if (c1->desc()->name() != ci->desc()->name() && !(perm &&
                di->desc()->is_permute(ci->desc()->name())))
            continue;

        if (ci == c1)
            check_contacts_in_groups(tmpsd, ld, ci, counts);
        else
            check_contacts_in_groups(tmpsd, ld, ci, xcounts);
    }

    // We split according to the contact count and width.
    //
    if (do_area) {
        double atot = 0.0;
        CDg gdesc;
        gdesc.init_gen(tmpsd, ld);
        CDo *od;
        while ((od = gdesc.next()) != 0) {
            int n = od->group();
            double a = od->area();
            if (!xcounts[n])
                atot += a;
            else
                atot += counts[n]*a/(counts[n] + xcounts[n]);
        }
        mp_variable->content.value = atot;
    }
    if (do_perim) {
        double ptot = 0.0;
        CDg gdesc;
        gdesc.init_gen(tmpsd, ld);
        CDo *od;
        while ((od = gdesc.next()) != 0) {
            int n = od->group();
            if (od->type() != CDPOLYGON)
                continue;  // "can't happen"
            int p = ((CDpo*)od)->po_perim();
            if (!xcounts[n])
                ptot += MICRONS(p);
            else
                ptot += counts[n]*MICRONS(p)/(counts[n] + xcounts[n]);
        }
        mp_extravar->content.value = ptot;
    }
    delete tmpsd;
    delete [] counts;
    delete [] xcounts;
    return (true);
}


// Measure the length of a line in the direction between contacts, clipped
// by the contact bounding box.
//
bool
sMprim::cWidth(sDevInst *di)
{
    // CWidth[.n1[.n2]]
    bool do_it = (mp_variable != 0);
    if (do_it && di->find_precmp(Mkw.cWidth, &mp_variable->content.value)) {
        mp_variable->type = TYP_SCALAR;
        do_it = false;
    }
    if (!do_it)
        return (true);

    if (di->mstatus() == MS_PARALLEL || di->mstatus() == MS_SERIES) {
        // meaningless, return 0
        mp_variable->content.value = 0.0;
        mp_variable->type = TYP_SCALAR;
        return (true);
    }

    int n1, n2;
    setnums(mp_variable->name, &n1, &n2);
    if (n1 < 0)
        n1 = 0;
    if (n2 < 0)
        n2 = 0;
    if (n1 == n2)
        n2++;

    sDevContactInst *ci = di->find_contact(n1);
    if (!ci) {
        const char *msg = "no/unknown contact %d";
        ExtErrLog.add_dev_err(di->celldesc(), di, msg, n1);
        return (false);
    }
    sDevContactInst *rf = di->find_contact(n2);
    if (!rf) {
        const char *msg = "no/unknown contact %d";
        ExtErrLog.add_dev_err(di->celldesc(), di, msg, n2);
        return (false);
    }
    int x1 = (ci->cBB()->left + ci->cBB()->right)/2;
    int y1 = (ci->cBB()->bottom + ci->cBB()->top)/2;
    int x2 = (rf->cBB()->left + rf->cBB()->right)/2;
    int y2 = (rf->cBB()->bottom + rf->cBB()->top)/2;
    int dx = x2 - x1;
    int dy = y2 - y1;
    x1 -= dx;
    y1 -= dy;
    if (cGEO::line_clip(&x1, &y1, &x2, &y2, ci->cBB())) {
        const char *msg = "contact %d area clipping failed";
        ExtErrLog.add_dev_err(di->celldesc(), di, msg, n1);
        return (false);
    }
    if (x1 == x2)
        mp_variable->content.value = MICRONS(abs(y1 - y2));
    else if (y1 == y2)
        mp_variable->content.value = MICRONS(abs(x1 - x2));
    else
        mp_variable->content.value =
            MICRONS(mmRnd(sqrt((x1-x2)*(double)(x1-x2) +
            (y1-y2)*(double)(y1-y2))));
    mp_variable->type = TYP_SCALAR;
    return (true);
}


// Measure the length of a line normal to the direction between contacts,
// clipped to the contact bounding box.
//
bool
sMprim::cnWidth(sDevInst *di)
{
    // CNWidth[.n1[.n2]]
    bool do_it = (mp_variable != 0);
    if (do_it && di->find_precmp(Mkw.cnWidth, &mp_variable->content.value)) {
        mp_variable->type = TYP_SCALAR;
        do_it = false;
    }
    if (!do_it)
        return (true);

    if (di->mstatus() == MS_PARALLEL || di->mstatus() == MS_SERIES) {
        // meaningless, return 0
        mp_variable->content.value = 0.0;
        mp_variable->type = TYP_SCALAR;
        return (true);
    }

    int n1, n2;
    setnums(mp_variable->name, &n1, &n2);
    if (n1 < 0)
        n1 = 0;
    if (n2 < 0)
        n2 = 0;
    if (n1 == n2)
        n2++;

    sDevContactInst *ci = di->find_contact(n1);
    if (!ci) {
        const char *msg = "no/unknown contact %d";
        ExtErrLog.add_dev_err(di->celldesc(), di, msg, n1);
        return (false);
    }
    sDevContactInst *rf = di->find_contact(n2);
    if (!rf) {
        const char *msg = "no/unknown contact %d";
        ExtErrLog.add_dev_err(di->celldesc(), di, msg, n2);
        return (false);
    }
    int x1 = (ci->cBB()->left + ci->cBB()->right)/2;
    int y1 = (ci->cBB()->bottom + ci->cBB()->top)/2;
    int x2 = (rf->cBB()->left + rf->cBB()->right)/2;
    int y2 = (rf->cBB()->bottom + rf->cBB()->top)/2;
    int dx = x2 - x1;
    int dy = y2 - y1;
    int tx = x1;
    int ty = y1;
    int r = ci->cBB()->width() + ci->cBB()->height();
    if (dx == 0) {
        x1 = tx + r;
        y1 = ty;
        x2 = tx - r;
        y2 = ty;
    }
    else if (dy == 0) {
        x1 = tx;
        y1 = ty - r;
        x2 = tx;
        y2 = ty + r;
    }
    else {
        double s = sqrt(dx*(double)dx + dy*(double)dy);
        x1 = tx + (int)(dy*(double)r/s);
        y1 = ty - (int)(dx*(double)r/s);
        x2 = tx - (int)(dy*(double)r/s);
        y2 = ty + (int)(dx*(double)r/s);
    }
    if (cGEO::line_clip(&x1, &y1, &x2, &y2, di->bBB())) {
        const char *msg = "body area clipping failed";
        ExtErrLog.add_dev_err(di->celldesc(), di, msg);
        return (false);
    }
    if (x1 == x2)
        mp_variable->content.value = MICRONS(abs(y1 - y2));
    else if (y1 == y2)
        mp_variable->content.value = MICRONS(abs(x1 - x2));
    else
        mp_variable->content.value =
            MICRONS(mmRnd(sqrt((x1-x2)*(double)(x1-x2) +
            (y1-y2)*(double)(y1-y2))));
    mp_variable->type = TYP_SCALAR;
    return (true);
}


// Measure the length of a line between the centers of the two contacts,
// clipped to the body bounding box.
//
bool
sMprim::cbWidth(sDevInst *di)
{
    // CBWidth[.n1[.n2]]
    bool do_it = (mp_variable != 0);
    if (do_it && di->find_precmp(Mkw.cbWidth, &mp_variable->content.value)) {
        mp_variable->type = TYP_SCALAR;
        do_it = false;
    }
    if (!do_it)
        return (true);

    if (di->mstatus() == MS_PARALLEL) {
        // return the average
        double val = 0.0;
        int cnt = 0;
        for (sDevInst *dx = di->multi_devs(); dx; dx = dx->next()) {
            if (!cbWidth(dx))
                return (false);
            val += mp_variable->content.value;
            cnt++;
        }
        mp_variable->content.value = val/(cnt ? cnt : 1);
        mp_variable->type = TYP_SCALAR;
        return (true);
    }
    if (di->mstatus() == MS_SERIES) {
        // return the sum
        double val = 0.0;
        for (sDevInst *dx = di->multi_devs(); dx; dx = dx->next()) {
            if (!cbWidth(dx))
                return (false);
            val += mp_variable->content.value;
        }
        mp_variable->content.value = val;
        mp_variable->type = TYP_SCALAR;
        return (true);
    }

    int n1, n2;
    setnums(mp_variable->name, &n1, &n2);
    if (n1 < 0)
        n1 = 0;
    if (n2 < 0)
        n2 = 0;
    if (n1 == n2)
        n2++;

    sDevContactInst *ci = di->find_contact(n1);
    if (!ci) {
        const char *msg = "no/unknown contact %d";
        ExtErrLog.add_dev_err(di->celldesc(), di, msg, n1);
        return (false);
    }
    sDevContactInst *rf = di->find_contact(n2);
    if (!rf) {
        const char *msg = "no/unknown contact %d";
        ExtErrLog.add_dev_err(di->celldesc(), di, msg, n2);
        return (false);
    }
    int x1 = (ci->cBB()->left + ci->cBB()->right)/2;
    int y1 = (ci->cBB()->bottom + ci->cBB()->top)/2;
    int x2 = (rf->cBB()->left + rf->cBB()->right)/2;
    int y2 = (rf->cBB()->bottom + rf->cBB()->top)/2;
    if (cGEO::line_clip(&x1, &y1, &x2, &y2, di->bBB())) {
        const char *msg = "body area clipping failed";
        ExtErrLog.add_dev_err(di->celldesc(), di, msg);
        return (false);
    }
    if (x1 == x2)
        mp_variable->content.value = MICRONS(abs(y1 - y2));
    else if (y1 == y2)
        mp_variable->content.value = MICRONS(abs(x1 - x2));
    else
        mp_variable->content.value =
            MICRONS(mmRnd(sqrt((x1-x2)*(double)(x1-x2) +
            (y1-y2)*(double)(y1-y2))));
    mp_variable->type = TYP_SCALAR;
    return (true);
}


// Measure the length of a line normal to the line between contact centers,
// centered, clipped to the body bounding box.
//
bool
sMprim::cbnWidth(sDevInst *di)
{
    // CBNWidth[.n1[.n2]]
    bool do_it = (mp_variable != 0);
    if (do_it && di->find_precmp(Mkw.cbnWidth, &mp_variable->content.value)) {
        mp_variable->type = TYP_SCALAR;
        do_it = false;
    }
    if (!do_it)
        return (true);

    if (di->mstatus() == MS_PARALLEL) {
        // return the sum
        double val = 0.0;
        for (sDevInst *dx = di->multi_devs(); dx; dx = dx->next()) {
            if (!cbnWidth(dx))
                return (false);
            val += mp_variable->content.value;
        }
        mp_variable->content.value = val;
        mp_variable->type = TYP_SCALAR;
        return (true);
    }
    if (di->mstatus() == MS_SERIES) {
        // return the average
        double val = 0.0;
        int cnt = 0;
        for (sDevInst *dx = di->multi_devs(); dx; dx = dx->next()) {
            if (!cbnWidth(dx))
                return (false);
            val += mp_variable->content.value;
            cnt++;
        }
        mp_variable->content.value = val/(cnt ? cnt : 1);
        mp_variable->type = TYP_SCALAR;
        return (true);
    }

    int n1, n2;
    setnums(mp_variable->name, &n1, &n2);
    if (n1 < 0)
        n1 = 0;
    if (n2 < 0)
        n2 = 0;
    if (n1 == n2)
        n2++;

    sDevContactInst *ci = di->find_contact(n1);
    if (!ci) {
        const char *msg = "no/unknown contact %d";
        ExtErrLog.add_dev_err(di->celldesc(), di, msg, n1);
        return (false);
    }
    sDevContactInst *rf = di->find_contact(n2);
    if (!rf) {
        const char *msg = "no/unknown contact %d";
        ExtErrLog.add_dev_err(di->celldesc(), di, msg, n2);
        return (false);
    }
    int x1 = (ci->cBB()->left + ci->cBB()->right)/2;
    int y1 = (ci->cBB()->bottom + ci->cBB()->top)/2;
    int x2 = (rf->cBB()->left + rf->cBB()->right)/2;
    int y2 = (rf->cBB()->bottom + rf->cBB()->top)/2;
    int tx = (x1 + x2)/2;
    int ty = (y1 + y2)/2;
    int dx = x2 - x1;
    int dy = y2 - y1;
    int r = di->bBB()->width() + di->bBB()->height();
    if (dx == 0) {
        x1 = tx + r;
        y1 = ty;
        x2 = tx - r;
        y2 = ty;
    }
    else if (dy == 0) {
        x1 = tx;
        y1 = ty - r;
        x2 = tx;
        y2 = ty + r;
    }
    else {
        double s = sqrt(dx*(double)dx + dy*(double)dy);
        x1 = tx + (int)(dy*(double)r/s);
        y1 = ty - (int)(dx*(double)r/s);
        x2 = tx - (int)(dy*(double)r/s);
        y2 = ty + (int)(dx*(double)r/s);
    }
    if (cGEO::line_clip(&x1, &y1, &x2, &y2, di->bBB())) {
        const char *msg = "body area clipping failed";
        ExtErrLog.add_dev_err(di->celldesc(), di, msg);
        return (false);
    }
    if (x1 == x2)
        mp_variable->content.value = MICRONS(abs(y1 - y2));
    else if (y1 == y2)
        mp_variable->content.value = MICRONS(abs(x1 - x2));
    else
        mp_variable->content.value =
            MICRONS(mmRnd(sqrt((x1-x2)*(double)(x1-x2) +
            (y1-y2)*(double)(y1-y2))));
    mp_variable->type = TYP_SCALAR;
    return (true);
}


bool
sMprim::resistance(sDevInst *di)
{
    bool do_it = (mp_variable != 0);
    if (do_it && di->find_precmp(Mkw.resistance,
            &mp_variable->content.value)) {
        mp_variable->type = TYP_SCALAR;
        do_it = false;
    }
    if (!do_it)
        return (true);

    if (di->mstatus() == MS_PARALLEL) {
        // return the parallel value
        double val = 0.0;
        for (sDevInst *dx = di->multi_devs(); dx; dx = dx->next()) {
            if (!resistance(dx))
                return (false);
            if (mp_variable->content.value != 0.0)
                val += 1.0/mp_variable->content.value;
        }
        mp_variable->content.value = (val != 0.0 ? 1.0/val : 0.0);
        mp_variable->type = TYP_SCALAR;
        return (true);
    }
    if (di->mstatus() == MS_SERIES) {
        // return the sum
        double val = 0.0;
        for (sDevInst *dx = di->multi_devs(); dx; dx = dx->next()) {
            if (!resistance(dx))
                return (false);
            val += mp_variable->content.value;
        }
        mp_variable->content.value = val;
        mp_variable->type = TYP_SCALAR;
        return (true);
    }

    mp_variable->type = TYP_SCALAR;
    mp_variable->content.value = 0.0;

    bool retval = false;
    RLsolver *r = di->setup_squares(false);
    if (r) {
        retval = true;
        double squares;
        if (r->solve_two(&squares))
            mp_variable->content.value = squares;
        else {
            const char *msg = "RLsolver error: %s";
            ExtErrLog.add_dev_err(di->celldesc(), di, msg,
                Errs()->get_error());
            retval = false;
        }
        delete r;
    }
    return (retval);
}


bool
sMprim::inductance(sDevInst *di)
{
    bool do_it = (mp_variable != 0);
    if (do_it && di->find_precmp(Mkw.inductance,
            &mp_variable->content.value)) {
        mp_variable->type = TYP_SCALAR;
        do_it = false;
    }
    if (!do_it)
        return (true);

    if (di->mstatus() == MS_PARALLEL) {
        // return the parallel value
        double val = 0.0;
        for (sDevInst *dx = di->multi_devs(); dx; dx = dx->next()) {
            if (!inductance(dx))
                return (false);
            if (mp_variable->content.value != 0.0)
                val += 1.0/mp_variable->content.value;
        }
        mp_variable->content.value = (val != 0.0 ? 1.0/val : 0.0);
        mp_variable->type = TYP_SCALAR;
        return (true);
    }
    if (di->mstatus() == MS_SERIES) {
        // return the sum
        double val = 0.0;
        for (sDevInst *dx = di->multi_devs(); dx; dx = dx->next()) {
            if (!inductance(dx))
                return (false);
            val += mp_variable->content.value;
        }
        mp_variable->content.value = val;
        mp_variable->type = TYP_SCALAR;
        return (true);
    }

    mp_variable->type = TYP_SCALAR;
    mp_variable->content.value = 0.0;

    RLsolver *r = di->setup_squares(true);
    if (r) {
        double squares;
        if (r->solve_two(&squares))
            mp_variable->content.value = squares;
        else {
            const char *msg = "RLsolver error: %s";
            ExtErrLog.add_dev_err(di->celldesc(), di, msg,
                Errs()->get_error());
        }
        delete r;
    }
    return (true);
}


bool
sMprim::mutual(sDevInst*)
{
    return (false);
}


bool
sMprim::capacitance(sDevInst *di)
{
    bool do_it = (mp_variable != 0);
    if (do_it && di->find_precmp(Mkw.capacitance,
            &mp_variable->content.value)) {
        mp_variable->type = TYP_SCALAR;
        do_it = false;
    }
    if (!do_it)
        return (true);

    if (di->mstatus() == MS_PARALLEL) {
        // return the sum
        double val = 0.0;
        for (sDevInst *dx = di->multi_devs(); dx; dx = dx->next()) {
            if (!capacitance(dx))
                return (false);
            val += mp_variable->content.value;
        }
        mp_variable->content.value = val;
        mp_variable->type = TYP_SCALAR;
        return (true);
    }
    if (di->mstatus() == MS_SERIES) {
        // return the parallel value
        double val = 0.0;
        for (sDevInst *dx = di->multi_devs(); dx; dx = dx->next()) {
            if (!capacitance(dx))
                return (false);
            if (mp_variable->content.value != 0.0)
                val += 1.0/mp_variable->content.value;
        }
        mp_variable->content.value = (val != 0.0 ? 1.0/val : 0.0);
        mp_variable->type = TYP_SCALAR;
        return (true);
    }

    mp_variable->content.value = 0.0;
    CDl *ld = di->body_layer(sDevInst::bl_cap);
    if (ld)
        mp_variable->content.value = di->barea()*tech_prm(ld)->cap_per_area() +
            MICRONS(di->bperim())*tech_prm(ld)->cap_per_perim();
    else {
        const char *msg = "no Capacitance specified in body layer(s)";
        ExtErrLog.add_dev_err(di->celldesc(), di, msg);
    }
    mp_variable->type = TYP_SCALAR;
    return (true);
}
// End of sMprim functions.


namespace {
    inline bool seq(const char *s1, const char *s2)
    {
        return ((!s1 && !s2) || (s1 && s2 && !strcasecmp(s1, s2)));
    }
}


/*****
 * UNUSED, see commented code below
namespace {
    const char *merge_name(int f)
    {
        if (f & EXT_DEV_MERGE_PARALLEL) {
            if (f & EXT_DEV_MERGE_SERIES)
                return ("parallel and series");
            return ("parallel");
        }
        if (f & EXT_DEV_MERGE_SERIES)
            return ("series");
        return ("no merging");
    }
}
*****/


// We support devices with the same name but differing prefixes. 
// However, these all map to a common electrical cell, which requires
// that all of the physical cells have the same contact signatures and
// permutes, in accord with the electrical cell.
//
// The first device of a given name becomes the reference device. 
// Here we check the present device against the reference.  We allow
// inheritance of permutes, meaning that if the reference has
// permutes, and the second cell has no permutes, the second cell will
// be taken to have the same permutes as the reference.
//
// If the devices are incompatible, an error message will be returned,
// and the device will not be accepted.
//
char *
sDevDesc::checkEquiv(const sDevDesc *dref)
{
    if (!dref)
        return (0);
    char buf[256];

    // Check the contact lists.  The contacts must be identical.
    sDevContactDesc *c = d_contacts;
    sDevContactDesc *cr = dref->d_contacts;
    while (c && cr) {
        char *s = c->checkEquiv(cr);
        if (s) {
            snprintf(buf, 256, "Device %s, prefix %s:  %s",
            d_name->string(), d_prefix ? d_prefix : "null", s);
            delete [] s;
            return (lstring::copy(buf));
        }
        c = c->next();
        cr = cr->next();
    }
    if (c) {
        snprintf(buf, 256,
            "Device %s, prefix %s:  extra contact %s not found in reference\n"
            "device with the same name.", d_name->string(),
            d_prefix ? d_prefix : "null",
            c->name()->string());
        return (lstring::copy(buf));
    }
    if (cr) {
        snprintf(buf, 256,
            "Device %s, prefix %s:  missing contact %s found in reference\n"
            "devices with the same name.", d_name->string(),
            d_prefix ? d_prefix : "null",
            cr->name()->string());
        return (lstring::copy(buf));
    }

    // Make sure that the permutes are identical.
    if (dref->d_prmconts) {
        if (!d_prmconts) {
            // Silently inherit the reference permutes in this case.
            d_prmconts = dref->d_prmconts->dup();
        }
        else {
            // We accept different ordering, but the names must match.
            const char *p1 = 0;
            const char *p2 = 0;
            for (stringlist *sl = d_prmconts; sl; sl = sl->next) {
                if (!p1)
                    p1 = sl->string;
                else if (!p2)
                    p2 = sl->string;
            }
            const char *pr1 = 0;
            const char *pr2 = 0;
            for (stringlist *sl = dref->d_prmconts; sl; sl = sl->next) {
                if (!pr1)
                    pr1 = sl->string;
                else if (!pr2)
                    pr2 = sl->string;
            }
            if ((seq(p1, pr1) && seq(p2, pr2)) ||
                    (seq(p1, pr2) && seq(p2, pr1)))
                ;
            else {
                snprintf(buf, 256,
                    "Device %s, prefix %s:  permutes %s, %s differ "
                    "from %s, %s\n"
                    "found in reference device with the same name.",
                    d_name->string(), d_prefix ? d_prefix : "null",
                    p1 ? p1 : "null", p2 ? p2 : "null",
                    pr1 ? pr1 : "null", pr2 ? pr2 : "null");
                return (lstring::copy(buf));
            }
        }
    }
    else if (d_prmconts) {
        snprintf(buf, 256,
            "Device %s, prefix %s:  permutes are inconsistent with\n"
            "reference device with the same name.",
            d_name->string(), d_prefix ? d_prefix : "null");
        return (lstring::copy(buf));
    }

    /*****
    // I don't think this is necessary, need to skip this to correctly
    // LVS George Chen's asic2.

    // Check the merge flags, these must be identical.
    if (dref->d_flags && !d_flags) {
        // Silently fix this.
        d_flags = dref->d_flags;
    }
    if (dref->d_flags != d_flags) {
        snprintf(buf, 256,
            "Device %s, prefix %s:  merging (%s} differs from \n"
            "(%s) found in reference device with the same name.",
            d_name->string(), d_prefix ? d_prefix : "null",
            merge_name(d_flags), merge_name(dref->d_flags));
        return (lstring::copy(buf));
    }
    *****/

    return (0);
}


// Initialization for device specification, initialize contacts and check
// other parameters.
//
bool
sDevDesc::init()
{
    // check/initialize contact layer specs
    int numc = 0;
    for (sDevContactDesc *c = d_contacts; c; c = c->next()) {
        if (!c->init()) {
            Errs()->add_error("Device %s, error in contact initialization.",
                name());
            return (false);
        }
        numc++;
    }
    if (variable_conts())
        d_num_contacts = 0;
    else
        d_num_contacts = numc;

    // check/initialize body layer spec
    if (d_body.tree()) {
        char *s = d_body.tree()->checkLayersInTree();
        if (s) {
            char *ss = s;
            int n = 0;
            while (lstring::advtok(&ss))
                n++;
            if (n == 0)
                Errs()->add_error(
                    "Device %s, body expression initialization failed.",
                    name());
            else
                Errs()->add_error(
                    "Device %s, unknown %s in body expression: %s.", name(),
                    n > 1 ? "layers" : "layer", s); 
            delete [] s;
            return (false);
        }
    }
    else {
        if (!d_body.ldesc())
            d_body.set_ldesc(CDldb()->findLayer(d_body.lname(), Physical));
        if (!d_body.ldesc())
            d_body.set_ldesc(CDldb()->findDerivedLayer(d_body.lname()));
        if (!d_body.ldesc()) {
            Errs()->add_error("Device %s, unknown body layer: %s.", name(),
                d_body.lname()); 
            return (false);
        }
    }

    // Check that permute names match contact names.
    if (d_prmconts && !d_prm1) {
        char *nm1 = d_prmconts->string;
        if (!nm1 || !*nm1) {
            Errs()->add_error("Device %s, permute name null or empty.",
                d_name->stringNN());
            return (false);
        }
        if (!d_prmconts->next) {
            Errs()->add_error("Device %s, permute name missing.",
                d_name->stringNN());
            return (false);
        }
        char *nm2 = d_prmconts->next->string;
        if (!nm2 || !*nm2) {
            Errs()->add_error("Device %s, second permute name null or empty.",
                d_name->stringNN());
            return (false);
        }
        if (d_prmconts->next->next) {
            Errs()->add_error("Device %s, too many permute names, maximum 2.",
                d_name->stringNN());
            return (false);
        }

        // Note:  use case-insensitive matching even if net names are
        // case sensitive.

        sDevContactDesc *c1 = find_contact(nm1);
        if (!c1) {
            Errs()->add_error(
                "Device %s, can't find permutable contact named %s.",
                d_name->stringNN(), nm1);
            return (false);
        }

        sDevContactDesc *c2 = find_contact(nm2);
        if (!c2) {
            Errs()->add_error(
                "Device %s, can't find permutable contact named %s.",
                d_name->stringNN(), nm2);
            return (false);
        }
        d_prm1 = CDnetex::name_tab_add(nm1);
        d_prm2 = CDnetex::name_tab_add(nm2);
    }

    // Extract measurement primitives, and look for unreferenced variables.
    add_prims();
    check_vars();
    return (true);
}


// Parse and set up the measurement directive from the Device block.
//
bool
sDevDesc::parse_measure(const char *line)
{
    char *tok = lstring::gettok(&line);
    sMeasure *m = new sMeasure(tok);

    siVariable *tv = SIparse()->getVariables();
    SIparse()->setVariables(d_variables);
    m->set_tree(SIparse()->getTree(&line, true));
    if (!m->tree()) {
        delete m;
        SIparse()->setVariables(tv);
        return (false);
    }

    // The initial parse of the expression allocates the variables
    // used (if they are not already allocated).  Allocate a variable
    // for the result, so it is available in other measure expressions.
    //
    // Note: this may cause duplicate variable names, as in
    //  Measure Resistance Resistance
    //
    d_variables = SIparse()->getVariables();
    m->set_result(new siVariable());
    m->result()->name = lstring::copy(m->name());
    m->result()->type = TYP_SCALAR;
    m->result()->next = d_variables;
    d_variables = m->result();
    SIparse()->setVariables(tv);

    // An optional precision follows the expression.
    m->set_precision(DEF_PRECISION);
    if (*line) {
        while (isspace(*line))
            line++;
        if (isdigit(*line))
            m->set_precision(*line - '0');
    }

    if (!d_measures)
        d_measures = m;
    else {
        sMeasure *mm = d_measures;
        while (mm->next())
            mm = mm->next();
        mm->set_next(m);
    }
    return (true);
}


// Return the named measure struct.
//
sMeasure *
sDevDesc::find_measure(const char *nm)
{
    for (sMeasure *m = d_measures; m; m = m->next())
        if (!strcmp(nm, m->name()))
            return (m);
    return (0);
}


// Return the primitive with the matching name.
//
sMprim *
sDevDesc::find_prim(const char *kw)
{
    if (kw) {
        for (sMprim *mp = d_mprims; mp; mp = mp->next()) {
            if (mp->variable() && lstring::cieq(mp->variable()->name, kw))
                return (mp);
            if (mp->extravar() && lstring::cieq(mp->extravar()->name, kw))
                return (mp);
        }
    }
    return (0);
}


// Return the first variable with matching name.
//
siVariable *
sDevDesc::find_variable(const char *nm)
{
    for (siVariable *v = d_variables; v; v = (siVariable*)v->next)
        if (lstring::cieq(v->name, nm))
            return (v);
    return (0);
}


// Return the last variable with matching name.
//
siVariable *
sDevDesc::find_variable_last(const char *nm)
{
    siVariable *vl = 0;
    for (siVariable *v = d_variables; v; v = (siVariable*)v->next)
        if (lstring::cieq(v->name, nm))
            vl = v;
    return (vl);
}


// Find a contact by text name, case-insensitive whether or not
// CDnetName is case-sensitive.
//
sDevContactDesc *
sDevDesc::find_contact(const char *cname)
{
    if (cname) {
        for (sDevContactDesc *c = d_contacts; c; c = c->next())
            if (!lstring::cieq(c->name()->stringNN(), cname))
                return (c);
    }
    return (0);
}


namespace {
    // The device may be compound, and we will possibly match one of
    // the components.  The multi_devs() list consists if a copy of
    // the parent with original BB, followed by the parallel sections,
    // which themselves may be compound.
    //
    bool find_match(sDevInst *d, sDevInst *di, cTfmStack *stk)
    {
        if (di->multi_devs()) {
            for (di = di->multi_devs(); di; di = di->next()) {
                if (find_match(d, di, stk))
                    return (true);
            }
        }
        else {
            BBox tBB(*di->bBB());
            stk->TBB(&tBB, 0);
            if (tBB == *d->bBB())
                return (true);
        }
        return (false);
    }


    // Return true if gd contains a device identical to d when
    // transformed.
    //
    bool find_match(sDevInst *d, cGroupDesc *gd, cTfmStack *stk)
    {
        if (!gd)
            return (false);
        if (!d || !d->desc())
            return (false);
        for (sDevList *dv = gd->devices(); dv; dv = dv->next()) {
            if (d->desc()->name() != dv->devname())
                continue;
            for (sDevPrefixList *p = dv->prefixes(); p; p = p->next()) {
                if (p->devs()->desc() != d->desc())
                    continue;
                for (sDevInst *di = p->devs(); di; di = di->next()) {
                    if (find_match(d, di, stk))
                        return (true);
                }
            }
        }
        return (false);
    }


    // Return true of there is a device identical to d in the
    // hierarchy under sdesc.
    //
    bool find_hier_match(CDs *sdesc, sDevInst *d, int depth, cTfmStack *stk)
    {
        if (depth <= 0)
            return (false);
        CDl *ld = CellLayer();
        CDg gdesc;
        gdesc.init_gen(sdesc, ld, d->bBB());
        CDc *cd;
        while ((cd = (CDc*)gdesc.next()) != 0) {
            CDs *msdesc = cd->masterCell();
            if (!msdesc)
                continue;
            cGroupDesc *gd = msdesc->groups();
            stk->TPush();
            unsigned int x1, x2, y1, y2;
            if (stk->TOverlapInst(cd, d->bBB(), &x1, &x2, &y1, &y2)) {
                CDap ap(cd);
                int tx, ty;
                stk->TGetTrans(&tx, &ty);
                xyg_t xyg(x1, x2, y1, y2);
                do {
                    stk->TTransMult(xyg.x*ap.dx, xyg.y*ap.dy);

                    if (find_match(d, gd, stk)) {
                        stk->TPop();
                        return (true);
                    }
                    if (find_hier_match(msdesc, d, depth-1, stk)) {
                        stk->TPop();
                        return (true);
                    }

                    stk->TSetTrans(tx, ty);
                } while (xyg.advance());
            }
            stk->TPop();
        }
        return (false);
    }


    // Return false if all contact groups can be immediately determined
    // and all are the same.  In this case, the device can be ignored. 
    // This helps eliminate spurious device matches from test structures,
    // etc.
    //
    bool check_contacts_not_shorted(const cGroupDesc *gd, const sDevInst *di)
    {
        if (!gd)
            return (true);
        bool first = true;
        int last_grp = 0;
        for (sDevContactInst *ci = di->contacts(); ci; ci = ci->next()) {
            CDl *ld = CDldb()->findLayer(ci->desc()->lname(), Physical);
            if (!ld || !ld->isConductor())
                // Bad layer, punt.
                return (true);
            sGrpGen grg;
            grg.init_gen(gd, ld, ci->cBB());
            CDo *odesc;
            while ((odesc = grg.next()) != 0) {
                if (odesc->intersect(ci->cBB(), false))
                    break;
            }
            if (!odesc)
                return (true);
            if (first) {
                first = false;
                last_grp = odesc->group();
            }
            else if (last_grp != odesc->group())
                return (true);
        }
        return (false);
    }
}


// Return a list of instances of the device, or the first instance
// that overlaps AOI if given.  This is the main function for device
// recognition.
//
XIrt
sDevDesc::find(CDs *sdesc, sDevInst **dlist, const BBox *AOI, bool findall,
    bool deptest)
{
    // User needs to give us this.
    if (!dlist)
        return (XIbad);
    *dlist = 0;

    // Get a list of device body areas, and group them.  Each
    // spatially distinct area is a potential device.

    SIlexprCx cx(sdesc, d_depth, AOI && deptest ? AOI : sdesc->BB());
    Zlist *zret = 0;
    XIrt ret = d_body.getZlist(&cx, &zret);
    if (ret == XIbad) {
        const char *msg = "body expression for %s, evaluation failed.";
        ExtErrLog.add_dev_err(sdesc, 0, msg, name());
        return (ret);
    }
    if (ret == XIintr)
        return (ret);
    if (!zret)
        return (XIok);
    Zgroup *g = Zlist::group(zret);

    // Look for those that overlap AOI, if AOI given.
    if (AOI) {
        int j = 0;
        for (int i = 0; i < g->num; i++) {
            if (!Zlist::intersect(g->list[i], AOI, false)) {
                Zlist::free(g->list[i]);
                g->list[i] = 0;
            }
            else {
                j++;
                if (!findall)
                    break;
            }
        }
        if (!j) {
            delete g;
            return (XIok);
        }
        Zlist **zz = new Zlist*[j];
        j = 0;
        for (int i = 0; i < g->num; i++) {
            if (g->list[i]) {
                if (!findall && j)
                    Zlist::free(g->list[i]);
                else
                    zz[j++] = g->list[i];
                g->list[i] = 0;
            }
        }
        delete [] g->list;
        g->list = zz;
        g->num = j;
    }

    // Throw out any that overlap the GlobalExclude layer expression
    // dark areas, if any.
    //
    // Warning: must call globex.clear before exit, copy constructor
    // does a shallow copy.
    sLspec globex(*EX()->globalExclude());
    if (globex.tree() || globex.ldesc()) {
        int j = 0;
        for (int i = 0; i < g->num; i++) {
            cx = SIlexprCx(sdesc, CDMAXCALLDEPTH, g->list[i]);
            CovType ct;
            if (globex.testZlistCovNone(&cx, &ct, 2*Tech()->AngleSupport())
                    == XIok && ct != CovNone) {
                Zlist::free(g->list[i]);
                g->list[i] = 0;
                continue;
            }
            j++;
        }
        if (!j) {
            delete g;
            return (XIok);
        }
        Zlist **zz = new Zlist*[j];
        j = 0;
        for (int i = 0; i < g->num; i++) {
            if (g->list[i]) {
                zz[j++] = g->list[i];
                g->list[i] = 0;
            }
        }
        delete [] g->list;
        g->list = zz;
        g->num = j;
    }
    globex.clear();

    // Flag for MOS contact fix.
    bool contact_fix;
    if (contacts_min_dim_given() && !contacts_min_dim())
       contact_fix = false;
    else if (!contacts_min_dim_given())
        contact_fix = is_mos();
    else
        contact_fix = true;
    if (contact_fix && !d_prm1)
        contact_fix = false;

//#define DEVXHIER_DEBUG
#ifdef DEVXHIER_DEBUG
    int skip_cnt = 0;
#endif

    // Loop through the device body areas, and construct the device
    // structures if possible.
    int cnt = 0;
    sDevInst *d0 = 0;
    unsigned long check_time = 0;
    for (int i = 0; i < g->num; i++) {
        if (Timer()->check_interval(check_time)) {
            if (DSP()->MainWdesc() && DSP()->MainWdesc()->Wdraw())
                dspPkgIf()->CheckForInterrupt();
            if (XM()->ConfirmAbort()) {
                delete g;
                *dlist = d0;  // partial list
                return (XIintr);
            }
        }
        sDevInst *d = new sDevInst(g->num - 1 - i, this, sdesc);
        BBox tBB;
        Zlist::BB(g->list[i], tBB);
        d->set_bBB(&tBB);
        d->set_BB(&tBB);
        if (d_bloat != 0.0)
            d->bloat_BB(INTERNAL_UNITS(d_bloat));

        if (!identify_contacts(sdesc, d, g->list[i], &ret)) {
            delete d;
            if (ret != XIok) {
                delete g;
                *dlist = d0;  // partial list
                return (ret);
            }
            continue;
        }

        if (!find_finds(sdesc, d, &ret)) {
            delete d;
            if (ret != XIok) {
                delete g;
                *dlist = d0;  // partial list
                return (ret);
            }
            continue;
        }

        // When depth is > 0, subcells containing an entire device
        // cause trouble, since the device is recognized in the parent
        // cell, yet remains in the subcell and is recognized there,
        // too.  We will look in the instances for an identical
        // device, and if found, we abort the present one here.  Note
        // that the subcells have already been extracted so the
        // devices have been listed in the group desc.
        //
        if (d_depth > 0) {
            cTfmStack stk;
            if (find_hier_match(sdesc, d, d_depth, &stk)) {
                delete d;
                d = 0;
#ifdef DEVXHIER_DEBUG
                skip_cnt++;
#endif
                continue;
            }
        }

        // Compute area, mindim, and perim.
        try {
            g->list[i] = Zlist::repartition(g->list[i]);
        }
        catch (XIrt tmpret) {
            delete d;
            delete g;
            return (tmpret);
        }

        double area;
        int mindim;
        if (simple_min_dim() || !g->list[i]->next) {
            area = 0.0;
            mindim = CDinfinity;
            for (Zlist *z = g->list[i]; z; z = z->next) {
                int w = ((z->Z.xur + z->Z.xlr) - (z->Z.xul + z->Z.xll))/2;
                int h = z->Z.yu - z->Z.yl;
                int dim = w < h ? w : h;
                if (dim < mindim)
                    mindim = dim;
                area += w*(double)h;
            }
            area /= (CDphysResolution*CDphysResolution);
        }
        else {
            mindim = Zlist::linewidth(g->list[i]);
            area = Zlist::area(g->list[i]);
        }
        d->set_bmindim(mindim);
        d->set_barea(area);

        // Clip out contact overlap areas.
        d->clip_contacts();

        if (contact_fix) {
            // This is a hack for extracting MOS devices with
            // width less than length.  If the device is
            // rectangular, the BodyMinDimen is explicitly set to
            // the S/D distance.
            //
            // This is extended to other devices, the contacts are
            // those in the permutes list.
            //
            // This applies only to rectangular devices.
            //

            if (g->list[i]->next == 0 && g->list[i]->Z.is_rect()) {
                sDevContactInst *c1 = d->find_contact(d_prm1);
                sDevContactInst *c2 = d->find_contact(d_prm2);
                if (c1 && c2) {
                    int x1 = (c1->cBB()->left + c1->cBB()->right)/2;
                    int y1 = (c1->cBB()->bottom + c1->cBB()->top)/2;
                    int x2 = (c2->cBB()->left + c2->cBB()->right)/2;
                    int y2 = (c2->cBB()->bottom + c2->cBB()->top)/2;
                    Zlist *z = g->list[i];
                    int cbw = 0;
                    if (x1 == x2)
                        cbw = z->Z.yu - z->Z.yl; 
                    else if (y1 == y2)
                        cbw = z->Z.xlr - z->Z.xll;
                    if (cbw > d->bmindim())
                        d->set_bmindim(cbw);
                }
            }
        }

        Poly po;
        g->list[i] = Zlist::to_poly(g->list[i], &po.points, &po.numpts);
        // g->list[i] should be 0, any return is ignored
        d->set_bperim(po.perim());
        delete [] po.points;

        if (!EX()->isKeepShortedDevs() &&
                !check_contacts_not_shorted(sdesc->groups(), d))
            delete d;
        else {
            d->set_next(d0);
            d0 = d;
            cnt++;
        }
    }
    delete g;

    if (EX()->isVerbosePromptline()) {
#ifdef DEVXHIER_DEBUG
        if (skip_cnt == 1)
            PL()->ShowPrompt("Skipping %s on lower hierarchy", name());
        else if (skip_cnt > 1)
            PL()->ShowPrompt("Skipped %s on lower hierarchy %d times", name(),
                skip_cnt);
#endif
        if (cnt && !deptest) {
            if (d_prefix)
                PL()->ShowPromptV("Found %d: %s (%s)", cnt, name(), d_prefix);
            else
                PL()->ShowPromptV("Found %d: %s", cnt, name());
        }
    }

    *dlist = d0;
    return (XIok);
}


// Identify and set up the device contacts.  If contacts aren't found,
// return false with xrt returning XIok, which indicates that d is no
// good and should be deleted.  Otherwise, xrt will indicate an error
// condition.
//
bool
sDevDesc::identify_contacts(CDs *sdesc, sDevInst *d, const Zlist *zbody,
    XIrt *xrt)
{
    *xrt = XIok;
    Zlist *zbb = new Zlist;
    int quad = -1;
    if (overlap_conts())
        quad = 0;
    setqz(zbb->Z, *d->BB(), quad);

    sDevContactInst *cend = 0;
    for (sDevContactDesc *c = d_contacts; c; c = c->next()) {

        XIrt ret;
        sDevContactInst *cx = identify_contact(sdesc, d, &zbb, zbody, c,
            quad, &ret);
        if (cx) {
            // The cx may be a list of contact structs, if the device
            // supports variable contact counts.

            if (!cend) {
                d->set_contacts(cx);
                cend = cx;
            }
            else {
                while (cend->next())
                    cend = cend->next();
                cend->set_next(cx);
            }
        }
        else {
            Zlist::free(zbb);
            *xrt = ret;
            return (false);
        }
    }
    Zlist::free(zbb);
    return (true);
}


// Identify and create a device contact instance, or list of
// instances, if possible.  A list of contacts may be extracted if the
// c->multiple() flag is set, which applies to devices with variable
// numbers of contacts.
//
// The zbbp points to a Zlist representing the device body, with the
// contacts clipped out when identified.  The zbody is the initial
// device body.  The quad value is for overlapping contact support.
//
sDevContactInst *
sDevDesc::identify_contact(CDs *sdesc, sDevInst *d, Zlist **zbbp,
    const Zlist *zbody, sDevContactDesc *c, int &quad, XIrt *xrt)
{
    if (c->is_bulk())
        return (identify_bulk_contact(sdesc, d, c, xrt));

    *xrt = XIok;
    Zlist *zbb = *zbbp;
    if (!zbb)
        return (0);

    // Find and group the possible contact areas.
    Zlist *zret = 0;
    SIlexprCx cx(sdesc, d_depth, zbb);
    XIrt ret = c->lspec()->getZlist(&cx, &zret);
    if (ret != XIok) {
        const char *msg = "contact expression for %s%s, evaluation %s.";
        ExtErrLog.add_dev_err(sdesc, 0, msg, name(), c->name(),
            ret == XIintr ? "interrupted" : "failed");
        *xrt = ret;
        return (0);
    }
    if (!zret)
        return (0);
    Zgroup *gc = Zlist::group(zret);

    // Bloat the device body if bloating.  Device contacts must
    // touch or intersect the (possibly bloated) body.
    const Zlist *zb = zbody;
    if (d_bloat != 0.0) {
        try {
            zb = Zlist::bloat(zbody, INTERNAL_UNITS(d_bloat), 0);
        }
        catch (XIrt zbret) {
            const char *msg = "contact expression for %s%s, body "
                "bloat evaluation %s.";
            ExtErrLog.add_dev_err(sdesc, 0, msg, name(), c->name(),
                ret == XIintr ? "interrupted" : "failed");
            delete gc;
            *xrt = zbret;
            return (0);
        }
    }

    int nc = 0;
    sDevContactInst *ci0 = 0, *ciend = 0;
    for (int j = 0; j < gc->num; j++) {
        if (Zlist::intersect(gc->list[j], zb, true)) {
            sDevContactInst *ci = new sDevContactInst(d, c);
            Zlist::BB(gc->list[j], *ci->BB());
            ci->set_fillfct(Zlist::area(gc->list[j])/ci->cBB()->area());

            if (!overlap_conts()) {
                for (Zlist *z = gc->list[j]; z; z = z->next)
                    Zlist::zl_andnot(&zbb, &z->Z);
            }
            else {
                quad++;
                setqz(zbb->Z, *d->BB(), quad);
            }
            if (c->multiple()) {
                if (!nc)
                    ci0 = ciend = ci;
                else {
                    char buf[64];
                    char *s = lstring::stpcpy(buf,
                        ci->desc()->name()->stringNN());
                    *s++ = 'a' + nc - 1;
                    *s = 0;
                    ci->set_name(CDnetex::name_tab_add(buf));
                    ciend->set_next(ci);
                    ciend = ciend->next();
                }
            }
            else {
                ci0 = ci;
                break;
            }
            nc++;
        }
    }
    delete gc;
    if (d_bloat != 0.0)
        Zlist::free(zb);
    *zbbp = zbb;
    return (ci0);
}


sDevContactInst *
sDevDesc::identify_bulk_contact(CDs *sdesc, sDevInst *d, sDevContactDesc *c,
    XIrt *xrt)
{
    *xrt = XIok;
    if (c->level() == BC_skip) {
        // Make an empty contact struct, and pretend that the
        // contact actually exists.

        return (new sDevContactInst(d, c));
    }
    BBox bcBB(*d->bBB());
    bcBB.bloat(INTERNAL_UNITS(c->bulk_bloat()));
    SIlexprCx cx(sdesc, CDMAXCALLDEPTH, &bcBB);
    Zlist *zret = 0;
    XIrt ret = c->lspec()->getZlist(&cx, &zret);
    if (ret != XIok) {
        const char *msg = "contact expression for %s%s, evaluation %s.";
        ExtErrLog.add_dev_err(sdesc, 0, msg, name(), c->name(),
            ret == XIintr ? "interrupted" : "failed");
        Zlist::free(zret);
        *xrt = ret;
        return (0);
    }
    if (!zret) {
        if (c->level() == BC_defer) {
            // No contact found at this level, proceed with a
            // dummy contact as for BC_skip.

            return (new sDevContactInst(d, c));
        }
        return (0);
    }
    Zgroup *gc = Zlist::group(zret);

    // Find the "closest" contact area.
    double dst = -1;
    int ix = -1;
    int xo = (bcBB.left + bcBB.right)/2;
    int yo = (bcBB.bottom + bcBB.top)/2;

    for (int j = 0; j < gc->num; j++) {
        BBox tBB;
        Zlist::BB(gc->list[j], tBB);
        int x = (tBB.left + tBB.right)/2;
        int y = (tBB.bottom + tBB.top)/2;
        double dd = (x-xo)*(x-xo) + (y-yo)*(y-yo);
        if (dst < 0.0 || dd < dst) {
            dst = dd;
            ix = j;
        }
    }
    sDevContactInst *ci = new sDevContactInst(d, c);
    Zlist::BB(gc->list[ix], *ci->BB());
    ci->set_fillfct(Zlist::area(gc->list[ix])/ci->cBB()->area());
    delete gc;
    return (ci);
}


bool
sDevDesc::find_finds(CDs *sdesc, sDevInst *d, XIrt *xrt)
{
    *xrt = XIok;
    stringlist *f;
    for (f = d_finds; f; f = f->next)
        d->set_fnum(d->fnum() + 1);
    if (d->fnum()) {
        d->set_fdevs(new sDevInst*[d->fnum()]);
        int j;
        for (j = 0; j < d->fnum(); j++)
            d->fdevs()[j] = 0;
        j = 0;
        for (f = d_finds; f; f = f->next) {
            char tbuf[64];
            strcpy(tbuf, f->string);
            char *nm = tbuf;
            char *p = strchr(tbuf, '.');
            if (p)
                *p++ = 0;
            sDevDesc *td = EX()->findDevice(nm, p);
            if (!td)
                return (false);
            sDevInst *di;
            XIrt ret = td->find(sdesc, &di, d->BB(), true);
            if (ret != XIok) {
                *xrt = ret;
                return (false);
            }
            if (!di)
                return (false);
            while (di->next()) {
                // If the find returns more than one device, keep
                // only devices not already referenced.  The
                // selections may be ambiguous in this case, but
                // at least not duplicated in the list.
                int k;
                for (k = 0; k < j; k++) {
                    if (d->fdevs()[k]->desc() == di->desc() &&
                            *d->fdevs()[k]->bBB() == *di->bBB())
                        break;
                }
                if (k < j) {
                    sDevInst *dt = di;
                    di = di->next();
                    dt->set_next(0);
                    delete dt;
                    continue;
                }
                di->next()->free();
                di->set_next(0);
            }
            d->fdevs()[j++] = di;
        }
    }
    return (true);
}


// In the measure matching name, set the lvsword to word.  This is the
// token in a SPICE element line which gives the matching parameter.  If
// there is no such token and the value is a leading number, word should
// be an empty string.
//
bool
sDevDesc::set_lvs_word(const char *nm, char *word)
{
    for (sMeasure *m = d_measures; m; m = m->next()) {
        if (lstring::cieq(nm, m->name())) {
            m->set_lvsword(word);
            return (true);
        }
    }
    return (false);
}


//
// Start of private functions.
//

// Return false if a variable is not a primitive or measure name.
//
bool
sDevDesc::check_vars()
{
    for (Variable *v = d_variables; v; v = v->next) {
        if (!Mkw.is_prim(v->name) && !find_measure(v->name)) {
            Log()->WarningLogV(mh::Techfile,
                "Device %s, unknown measure variable %s.",
                name(), v->name);
            return (false);
        }
    }
    return (true);
}


// Return the sMprim struct that contains the variable.  The mprims are the
// primitive measurements te be performed, in support of the perhaps more
// complex extractions.  The primitives are filtered out so we only do them
// once.
//
sMprim *
sDevDesc::find_prim(siVariable *v)
{
    if (v) {
        for (sMprim *mp = d_mprims; mp; mp = mp->next()) {
            if (mp->variable() == v || mp->extravar() == v)
                return (mp);
        }
    }
    return (0);
}


// Look through the variables list for primitive variables, and add them
// to the mprims list if not already there.  There may be duplicate names
// as for "Measure Resistance Resistance", in which case the last variable
// in the list is the primitive, the others are not.
//
void
sDevDesc::add_prims()
{
    for (Variable *v = d_variables; v; v = v->next) {

        if (checkname(v->name, bArea) || checkname(v->name, bPerim)) {
            // These are computed together
            // BodyArea
            // BodyPerim
            siVariable *v1 = find_variable_last(Mkw.bArea);
            siVariable *v2 = find_variable_last(Mkw.bPerim);
            if (!find_prim(v1) && !find_prim(v2))
                d_mprims = new sMprim(v1, v2, d_mprims);
        }
        else if (checkname(v->name, cArea) || checkname(v->name, cPerim)) {
            // These are computed together
            // CArea.cont.layer
            // CPerim.cont.layer
            char buf[64];
            char *s = strchr(v->name, '.');
            strcpy(buf, Mkw.cArea);
            strcat(buf, s);
            siVariable *v1 = find_variable_last(buf);
            strcpy(buf, Mkw.cPerim);
            strcat(buf, s);
            siVariable *v2 = find_variable_last(buf);
            if (!find_prim(v1) && !find_prim(v2))
                d_mprims = new sMprim(v1, v2, d_mprims);
        }
        else if (Mkw.is_prim(v->name)) {
            siVariable *vl = find_variable_last(v->name);
            if (!find_prim(vl))
                d_mprims = new sMprim(vl, 0, d_mprims);
        }
    }
}
// End sDevDesc functions.


// Highlight this device, unless eBB is given in which case add the
// drawing BB.
//
void
sDevInst::show(WindowDesc *wdesc, BBox *eBB) const
{
    if (!wdesc->IsSimilar(DSP()->MainWdesc(), WDsimXmode))
        return;
    if (wdesc->Mode() == Physical) {
        outline(wdesc, eBB);
        int delta = (int)(1.5*DSP_DEF_PTRM_TXTHT/wdesc->Ratio());
        // log scale size
        if (wdesc->Ratio() < .78) {
            double t1 = -5.0/log(wdesc->Ratio());
            delta = (int)(delta*t1);
        }
        else
            delta *= 20;

        int dim = di_BB.height();
        if (di_BB.width() < dim)
            dim = di_BB.width();
        dim /= 2;
        if (dim > 0 && dim < delta)
            delta = dim;

        int x, y, w, h;
        char buf[128];
        if (di_desc->prefix()) {
            char *s = lstring::stpcpy(buf, di_desc->prefix());
            mmItoA(s, di_index);
        }
        else {
            char *s = lstring::stpcpy(buf, di_desc->name()->stringNN());
            *s++ = ' ';
            mmItoA(s, di_index);
        }
        DSP()->DefaultLabelSize(buf, wdesc->Mode(), &w, &h);
        w = (w*delta)/h;
        h = delta;

        int xf;
        if (di_BB.height() >= 2*di_BB.width()) {
            x = (di_BB.left + di_BB.right + h)/2;
            y = di_BB.bottom + delta/2;
            xf = 1;  // rotate 90 degrees
            if (eBB) {
                BBox tBB(x-h, y, x, y+w);
                eBB->add(&tBB);
            }
        }
        else {
            x = di_BB.left + delta/2;
            y = (di_BB.bottom + di_BB.top - h)/2;
            xf = 0;
            if (eBB) {
                BBox tBB(x, y, x+w, y+h);
                eBB->add(&tBB);
            }
        }
        if (!eBB)
            wdesc->ShowLabel(buf, x, y, w, h, xf);
    }
    else if (di_dual) {
        // If the top cell in the window is symbolic, don't show.
        CDs *tcd = wdesc->TopCellDesc(Electrical);
        if (!tcd || tcd->isSymbolic())
            return;

        BBox dBB = di_dual->cdesc()->oBB();
        dBB.bloat(CDphysResolution/2);
        if (eBB)
            eBB->add(&dBB);
        else
            wdesc->ShowBox(&dBB, CDL_OUTLINED, 0);
    }
}


namespace {
    // Print the contact areas, descending into compound devices to
    // print individual elements.
    //
    void pr_conts(FILE *fp, const sDevInst *di, int dp, int *pcnt)
    {
        int ndgt = CD()->numDigits();
        for ( ; di; di = di->next()) {
            if (di->multi_devs()) {
                fprintf(fp, "%*s---\n", 2*dp, "");
                pr_conts(fp, di->multi_devs(), dp+1, pcnt);
                continue;
            }
            fprintf(fp, "%*ssegment %d:\n", 2*dp, "", *pcnt);
            for (sDevContactInst *ci = di->contacts(); ci; ci = ci->next()) {
                fprintf(fp, "  %*sContact %s, ", 2*dp, "",
                    ci->cont_name()->stringNN());
                fprintf(fp, "   Area %.*f,%.*f %.*f,%.*f\n",
                    ndgt, MICRONS(ci->BB()->left),
                    ndgt, MICRONS(ci->BB()->bottom),
                    ndgt, MICRONS(ci->BB()->right),
                    ndgt, MICRONS(ci->BB()->top));
            }
            (*pcnt)++;
        }
    }
}


// Print the device info.
//
void
sDevInst::print(FILE *fp, bool verbose)
{
    int ndgt = CD()->numDigits();
    fprintf(fp, "Instance %d of %s:\n", di_index, di_desc->name()->stringNN());
    int cnt = count_sections();
    if (cnt > 1) {
        fprintf(fp, "  merged - %d components\n", cnt);
        cnt = 0;
        if (verbose)
            pr_conts(fp, di_multi_devs, 0, &cnt);
    }
    for (sDevContactInst *ci = contacts(); ci; ci = ci->next()) {
        fprintf(fp, " Contact %s, ", ci->cont_name()->stringNN());
        fprintf(fp, " Group %d, ", ci->group());
        fprintf(fp, " Area %.*f,%.*f %.*f,%.*f\n",
            ndgt, MICRONS(ci->cBB()->left), ndgt, MICRONS(ci->cBB()->bottom),
            ndgt, MICRONS(ci->cBB()->right), ndgt, MICRONS(ci->cBB()->top));
    }
    measure();
    for (sMeasure *m = di_desc->measures(); m; m = m->next())
        m->print_result(fp);
}


// Print the device info in SPICE format.
//
bool
sDevInst::print_net(FILE *fp, const sDumpOpts *opts)
{
    char *s;
    if (!net_line(&s, di_desc->netline(), opts->spice_print_mode()))
        return (false);
    if (s) {
        if (opts->spice_print_mode() != PSPM_physical) {
            if (di_dual) {
                char *instname = di_dual->instance_name();
                char *t = s;
                while (*t && !isspace(*t))
                    t++;
                fprintf(fp, "%s%s\n", instname, t);
                delete [] instname;
            }
            else if (opts->spice_print_mode() == PSPM_mixed)
                fprintf(fp, "%s\n", s);
        }
        else
            fprintf(fp, "%s\n", s);
        delete [] s;
    }
    return (true);
}


// Return a string containing formatted text.
//
bool
sDevInst::net_line(char **ret, const char *format, PhysSpicePrintMode pmode)
{
    *ret = 0;
    const char *s = format;
    sLstr lstr;
    if (!s)
        return (true);
    if (!measure())
        return (false);
    while (*s) {
        if (*s == '\\') {
            // handle \n, \t
            if (*(s+1) == 'n') {
                lstr.add_c('\n');
                s += 2;
            }
            else if (*(s+1) == 't') {
                lstr.add_c('\t');
                s += 2;
            }
            else {
                lstr.add_c(*s);
                s++;
            }
            continue;
        }
        if (*s != '%') {
            lstr.add_c(*s);
            s++;
            continue;
        }

        // Find the leading digit, if any.  The diigit indicates that
        // the the target is one of devices in the di_fdevs list rather
        // than the containing device.
        int digit = 0;
        if (isdigit(*(s+1))) {
            sscanf(s+1, "%d", &digit);
            while (isdigit(*(s+1)))
                s++;
        }
        sDevInst *targ = this;
        if (digit > 0 && di_fnum) {
            for (int i = 0; i < di_fnum; i++) {
                if (digit == i + 1) {
                    if (di_fdevs[i])
                        targ = di_fdevs[i];
                    break;
                }
            }
        }

        if (format == di_desc->netline() || format == di_desc->netline1()) {

            // %model%, replaced by Model string
            if (*(s+1) == 'm' && *(s+2) == 'o' && *(s+3) == 'd' &&
                    *(s+4) == 'e' && *(s+5) == 'l' && *(s+6) == '%') {
                s += 7;
                char *c;
                if (!net_line(&c, di_desc->model(), pmode))
                    return (false);
                lstr.add(c);
                delete [] c;
                continue;
            }

            // %value%, replaced by Value string
            if (*(s+1) == 'v' && *(s+2) == 'a' && *(s+3) == 'l' &&
                    *(s+4) == 'u' && *(s+5) == 'e' && *(s+6) == '%') {
                s += 7;
                char *c;
                if (!net_line(&c, di_desc->value(), pmode))
                    return (false);
                lstr.add(c);
                delete [] c;
                continue;
            }

            // %param%, replaced by Param string
            if (*(s+1) == 'p' && *(s+2) == 'a' && *(s+3) == 'r' &&
                    *(s+4) == 'a' && *(s+5) == 'm' && *(s+6) == '%') {
                s += 7;
                char *c;
                if (!net_line(&c, di_desc->param(), pmode))
                    return (false);
                lstr.add(c);
                delete [] c;
                continue;
            }

            // %initc%, replaced by Param string (back compatability)
            if (*(s+1) == 'i' && *(s+2) == 'n' && *(s+3) == 'i' &&
                    *(s+4) == 't' && *(s+5) == 'c' && *(s+6) == '%') {
                s += 7;
                char *c;
                if (!net_line(&c, di_desc->param(), pmode))
                    return (false);
                lstr.add(c);
                delete [] c;
                continue;
            }
        }

        // %c%name, replaced by group number or name
        if (*(s+1) == 'c' && *(s+2) == '%') {
            const char *p = s + 3;
            char *tok = lstring::gettok(&p);
            if (tok) {
                char *q = strchr(tok, '%');
                if (q) {
                    *q = 0;
                    s++;
                }
                p = s + 3 + strlen(tok);
                sDevContactInst *ci = targ->find_contact(tok);
                delete [] tok;
                const char *nm = 0;
                if (ci) {
                    int grp = ci->group();
                    cGroupDesc *gd = di_sdesc->groups();
                    if (gd)
                        nm = gd->group_name(grp);
                }
                if (nm)
                    lstr.add(nm);
                else
                    lstr.add("???");
            }
            s = p;
            continue;
        }

        // %m%mname, replaced by measure result
        // %m[ g | s[d] | f[d] | e[d] ]%
        if (*(s+1) == 'm') {
            int ix = 0;
            int dp = 5;
            int pt = 0;
            if (*(s+2) == '%') {
                ix = 3;
                pt = 'g';
            }
            else if (*(s+2) == 'g' && *(s+3) == '%') {
                ix = 4;
                pt = 'g';
            }
            else if (*(s+2) == 's' && *(s+3) == '%') {
                ix = 4;
                pt = 's';
            }
            else if (*(s+2) == 's' && isdigit(*(s+3)) && *(s+4) == '%') {
                ix = 5;
                dp = *(s+3) - '0';
                pt = 's';
            }
            else if (*(s+2) == 'f' && *(s+3) == '%') {
                ix = 4;
                pt = 'f';
            }
            else if (*(s+2) == 'f' && isdigit(*(s+3)) && *(s+4) == '%') {
                ix = 5;
                dp = *(s+3) - '0';
                pt = 'f';
            }
            else if (*(s+2) == 'e' && *(s+3) == '%') {
                ix = 4;
                pt = 'e';
            }
            else if (*(s+2) == 'e' && isdigit(*(s+3)) && *(s+4) == '%') {
                ix = 5;
                dp = *(s+3) - '0';
                pt = 'e';
            }
            if (ix) {
                const char *p = s + ix;
                char *tok = lstring::gettok(&p);
                if (tok) {
                    char *q = tok;
                    while (isalnum(*q))
                        q++;
                    if (*q) {
                        if (*q == '%')
                            s++;
                        *q = 0;
                    }
                    p = s + ix + strlen(tok);
                    sMeasure *m = targ->di_desc->find_measure(tok);
                    delete [] tok;
                    if (m) {
                        double val = m->measure();
                        if (pt == 's')
                            lstr.add(SPnum.printnum(val, "", false, dp));
                        else if (pt == 'e')
                            lstr.add_e(val, dp > 0 ? dp : 5);
                        else if (pt == 'f')
                            lstr.add_d(val, dp > 0 ? dp : 5, false);
                        else
                            lstr.add_g(val);
                    }
                    else
                        lstr.add("???");
                }
                s = p;
                continue;
            }
        }

        // %n% replaced by prefix and SPICE index number
        if (*(s+1) == 'n' && *(s+2) == '%') {

            if (pmode == PSPM_duals) {
                if (!di_dual)
                    return (true);
                char *instname = di_dual->instance_name();
                lstr.add(instname);
                delete [] instname;
            }
            else if (pmode == PSPM_mixed) {
                if (di_dual) {
                    char *instname = di_dual->instance_name();
                    lstr.add(instname);
                    delete [] instname;
                }
                else {
                    // If no prefix (shouldn't happen), suppress the line.
                    const char *pf = targ->di_desc->prefix();
                    if (!pf || !*pf)
                        return (true);
                    lstr.add(pf);
                    lstr.add_i(targ->di_spindex);
                }
            }
            else {
                // PSPM_physical
                // If no prefix (shouldn't happen), suppress the line.
                const char *pf = targ->di_desc->prefix();
                if (!pf || !*pf)
                    return (true);
                lstr.add(pf);
                lstr.add_i(targ->di_spindex);
            }
            s += 3;
            continue;
        }

        // %e%, same as above, except that if the corresponding electrical
        // device can be identified, that name will replace the token
        if (*(s+1) == 'e' && *(s+2) == '%') {
            if (targ->di_dual) {
                char *instname = targ->di_dual->instance_name();
                lstr.add(instname);
                delete [] instname;
            }
            else {
                if (pmode == PSPM_duals)
                    return (true);
                lstr.add(targ->di_desc->prefix() ?
                    targ->di_desc->prefix() : "");
                lstr.add_i(targ->di_spindex);
            }
            s += 3;
            continue;
        }

        // %f%, same as above, except that if the corresponding electrical
        // device can be identified, that name will replace the token,
        // but the token is blanked otherwise
        if (*(s+1) == 'f' && *(s+2) == '%') {
            if (targ->di_dual) {
                char *instname = targ->di_dual->instance_name();
                lstr.add(instname);
                delete [] instname;
            }
            s += 3;
            continue;
        }

        // %p lname pnum%, replace with the string from property pnum
        // found on an object in the cell on lname which overlaps the
        // BB
        if (*(s+1) == 'p') {
            char *p0 = lstring::copy(s+2);
            char *p = p0;
            char *q = strchr(p, '%');
            char *lname = 0;
            char *pnum = 0;
            if (q) {
                *q = 0;
                lname = lstring::gettok(&p);
                pnum = lstring::gettok(&p);
            }
            delete [] p0;

            if (lname && pnum && isdigit(*pnum)) {
                bool found = false;
                s = strchr(s+2, '%') + 1;
                CDl *ld = CDldb()->findLayer(lname, Physical);
                int num = atoi(pnum);
                delete [] lname;
                delete [] pnum;
                if (ld) {
                    CDg gdesc;
                    gdesc.init_gen(di_sdesc, ld, &di_BB);
                    CDo *odesc;
                    while ((odesc = gdesc.next()) != 0) {
                        if (odesc->intersect(&di_BB, false)) {
                            CDp *pdesc = odesc->prpty(num);
                            if (pdesc && pdesc->string()) {
                                lstr.add(pdesc->string());
                                found = true;
                                break;
                            }
                        }
                    }
                }
                if (!found)
                    lstr.add("???");
                continue;
            }
            delete [] lname;
            delete [] pnum;
        }

        lstr.add_c(*s);
        s++;
    }
    char *t = lstr.string_trim();
    if (!t)
        t = lstring::copy("");
    if (!Tech()->EvaluateEval(t)) {
        ExtErrLog.add_err("In %s, netLine: evaluation error in %s.",
            di_sdesc->cellname()->string(), t);
    }

    *ret = t;
    return (true);
}


// Print a comparison between the extracted device parameters and those
// associated with the dual.
//
int
sDevInst::print_compare(sLstr *plstr, int *bad, int *amb)
{
    double *leadval = 0;
    sParamTab *ptab = 0;
    if (di_dual)
        di_dual->setup_eval(&ptab, &leadval);
    int ecnt = 0;
    measure();
    for (sMeasure *m = di_desc->measures(); m; m = m->next())
        ecnt += m->print_compare(plstr, leadval, ptab, bad, amb);
    delete ptab;
    return (ecnt);
}


// Set the properties in the electrical device from the parameters
// extracted from the physical equivalent (sdesc is physical).
//
bool
sDevInst::set_properties()
{
    bool didit = false;
    if (di_dual) {
        if (di_desc->model()) {
            char *s;
            net_line(&s, di_desc->model(), PSPM_duals);
            if (s) {
                CDp *p = di_dual->cdesc()->prpty(P_MODEL);
                SCD()->prptyModify(di_dual->cdesc(), p, P_MODEL, s, 0);
                delete [] s;
                didit = true;
            }
        }
        if (di_desc->value()) {
            char *s;
            net_line(&s, di_desc->value(), PSPM_duals);
            if (s) {
                CDp *p = di_dual->cdesc()->prpty(P_VALUE);
                SCD()->prptyModify(di_dual->cdesc(), p, P_VALUE, s, 0);
                delete [] s;
                didit = true;
            }
        }
        if (di_desc->param()) {
            char *s;
            net_line(&s, di_desc->param(), PSPM_duals);
            if (s) {
                CDp *p = di_dual->cdesc()->prpty(P_PARAM);
                SCD()->prptyModify(di_dual->cdesc(), p, P_PARAM, s, 0);
                delete [] s;
                didit = true;
            }
        }
    }
    return (didit);
}


// Return the layer associated with the body specification that has the
// necessary parameters set.
//
CDl *
sDevInst::body_layer(sDevInst::bl_type type) const
{
    double params[8];
    if (di_desc->body()->ldesc()) {
        CDl *ld = di_desc->body()->ldesc();
        if (type == bl_resis && (tech_prm(ld)->rho() > 0.0 ||
                tech_prm(ld)->ohms_per_sq() > 0.0))
            return (ld);
        else if (type == bl_cap && (tech_prm(ld)->cap_per_area() > 0.0 ||
                tech_prm(ld)->cap_per_perim() > 0.0))
            return (ld);
        else if (type == bl_induct && cTech::GetLayerTline(ld, params))
            return (ld);
        return (0);
    }
    CDll *l0 = di_desc->body()->tree()->findLayersInTree();
    for (CDll *l = l0; l; l = l->next) {
        CDl *ld = l->ldesc;
        if (type == bl_resis && (tech_prm(ld)->rho() > 0.0 ||
                tech_prm(ld)->ohms_per_sq() > 0.0)) {
            l0->free();
            return (ld);
        }
        else if (type == bl_cap && (tech_prm(ld)->cap_per_area() > 0.0 ||
                tech_prm(ld)->cap_per_perim() > 0.0)) {
            l0->free();
            return (ld);
        }
        else if (type == bl_induct && cTech::GetLayerTline(ld, params)) {
            l0->free();
            return (ld);
        }
    }
    l0->free();
    return (0);
}


// Return the named contact.
//
sDevContactInst *
sDevInst::find_contact(const char *name) const
{
    if (name) {
        for (sDevContactInst *ci = di_contacts; ci; ci = ci->next()) {
            if (lstring::cieq(name, ci->cont_name()->stringNN()))
                return (ci);
        }
    }
    return (0);
}


// Clip out from each contact box the parts that overlap other
// contacts.  Take that largest box that results as a new contact
// area.  If the contact is entirely covered by other contacts,
// keep its original area.
//
void
sDevInst::clip_contacts() const
{
    int numc = count_contacts();
    if (numc <= 1)
        return;
    Blist **bla = new Blist*[numc];
    int cnt = 0;
    for (sDevContactInst *c = di_contacts; c; c = c->next()) {
        if (c->desc()->is_bulk())
            continue;
        bla[cnt] = new Blist(c->cBB(), 0);
        cnt++;
    }
    numc = cnt;

    for (int i = 0; i < numc; i++) {
        cnt = 0;
        for (sDevContactInst *c = di_contacts; c; c = c->next()) {
            if (c->desc()->is_bulk())
                continue;
            if (cnt != i) {
                Blist bx = Blist(c->cBB(), 0);
                bla[i] = bla[i]->clip_out(&bx);
            }
            cnt++;
        }
    }
    int i = 0;
    for (sDevContactInst *c = di_contacts; c; c = c->next()) {
        if (c->desc()->is_bulk())
            continue;
        if (bla[i]) {
            if (!bla[i]->next)
                c->set_BB(&bla[i]->BB);
            else {
                // find largest
                BBox tBB;
                double area = 0.0;
                for (Blist *b = bla[i]; b; b = b->next) {
                    double a = b->BB.area();
                    if (a > area) {
                        area = a;
                        tBB = b->BB;
                    }
                }
                c->set_BB(&tBB);
            }
            bla[i]->free();
        }
        i++;
    }
    delete [] bla;
}


// Permute the permutable contacts, and null the measurements.
//
void
sDevInst::permute()
{
    if (!di_desc->permute_cont1())
        return;
    sDevContactInst *c1 = find_contact(di_desc->permute_cont1());
    sDevContactInst *c2 = find_contact(di_desc->permute_cont2());
    if (!c1 || !c2 || c1 == c2)
        return;
    sDevContactDesc *ctmp = c1->desc();
    c1->set_desc(c2->desc());
    c2->set_desc(ctmp);

    float *mtmp = di_mvalues;
    di_mvalues = di_mvalues_permuted;
    di_mvalues_permuted = mtmp;

    // permute the parallel devices
    if (di_mstatus == MS_PARALLEL) {
        for (sDevInst *d = di_multi_devs; d; d = d->next())
            d->permute();
    }
}


// Look through the primitives, if one depends asymetrically on a
// permutable contact, permute the device and return true.
//
bool
sDevInst::permute_params()
{
    if (!di_desc->permute_cont1())
        return (false);
    sDevContactDesc *c;
    int n1 = 0;
    for (c = di_desc->contacts(); c; c = c->next(), n1++) {
        if (c->name() == di_desc->permute_cont1())
            break;
    }
    if (!c)
        return (false);
    int n2 = 0;
    for (c = di_desc->contacts(); c; c = c->next(), n2++) {
        if (c->name() == di_desc->permute_cont2())
            break;
    }
    if (!c)
        return (false);
    for (sMprim *mp = di_desc->mprims(); mp; mp = mp->next()) {
        if (mp->variable()) {
            int x1, x2;
            mp->setnums(mp->variable()->name, &x1, &x2);
            if ((x1 == n1 || x1 == n2) && x2 != n1 && x2 != n2) {
                permute();
                return (true);
            }
            if ((x2 == n1 || x2 == n2) && x1 != n1 && x1 != n2) {
                permute();
                return (true);
            }
        }
        if (mp->extravar()) {
            int x1, x2;
            mp->setnums(mp->extravar()->name, &x1, &x2);
            if ((x1 == n1 || x1 == n2) && x2 != n1 && x2 != n2) {
                permute();
                return (true);
            }
            if ((x2 == n1 || x2 == n2) && x1 != n1 && x1 != n2) {
                permute();
                return (true);
            }
        }
    }
    return (false);
}


// Return true if di is connected in parallel, and permute di if necessary
// if it is found to be in parallel.
//
bool
sDevInst::is_parallel(sDevInst *di) const
{
    if (di_desc != di->di_desc)
        return (false);
    bool needs_permute = false;
    if (di_desc->permute_cont1()) {
        sDevContactInst *c1 = find_contact(di_desc->permute_cont1());
        sDevContactInst *c2 = find_contact(di_desc->permute_cont2());
        sDevContactInst *cx1 = di->find_contact(di_desc->permute_cont1());
        sDevContactInst *cx2 = di->find_contact(di_desc->permute_cont2());
        if (!c1 || !c2 || !cx1 || !cx2)
            return (false);
        if (c1->group() == cx1->group() && c2->group() == cx2->group())
            ;
        else if (c1->group() == cx2->group() && c2->group() == cx1->group())
            needs_permute = true;
        else
            return (false);
    }
    for (sDevContactInst *c = di_contacts; c; c = c->next()) {
        if (!c->cont_ok())
            continue;
        if (di_desc->is_permute(c->desc()->name()))
            continue;
        if (c->desc()->level() != BC_immed)
            continue;
        if (c->group() < 0)
            return (false);
        sDevContactInst *x = di->find_contact(c->desc()->name());
        if (c->group() != x->group())
            return (false);
    }
    if (needs_permute)
        di->permute();
    return (true);
}


// Return true to prevent device merging if the NoMerge property is
// found in a body object.
//
bool
sDevInst::no_merge()
{
    if (di_nomerge)
        return (true);
    if (!di_nm_checked) {
        CDll *l0 = di_desc->body()->findLayers();
        for (CDll *l = l0; l; l = l->next) {
            sPF gen(di_sdesc, &di_bBB, l->ldesc, di_desc->depth());
            CDo *odesc;
            while ((odesc = gen.next(true, false)) != 0) {
                if (odesc->prpty(XICP_NOMERGE)) {
                    l0->free();
                    di_nomerge = true;
                    return (true);
                }
            }
        }
        l0->free();
        di_nm_checked = true;
    }
    return (false);
}


// Copy Function for sDevInst structs.  Use the group mapping if given.
//
sDevInst *
sDevInst::copy(const cTfmStack *tstk, int *map) const
{
    sDevInst *di = new sDevInst(di_index, di_desc, di_sdesc);
    di->di_bBB = di_bBB;
    di->di_BB = di_BB;
    tstk->TBB(&di->di_bBB, 0);
    tstk->TBB(&di->di_BB, 0);
    sDevContactInst *cp = 0;
    for (sDevContactInst *c = di_contacts; c; c = c->next()) {
        sDevContactInst *ci = new sDevContactInst(di, c->desc());
        ci->set_BB(c->cBB());
        ci->set_fillfct(c->fillfct());
        tstk->TBB(ci->BB(), 0);
        if (map) {
            if (c->group() >= 0)
                ci->set_group(map[c->group()]);
            else
                ci->set_group(-1);
        }
        else
            ci->set_group(c->group());
        if (!cp)
            cp = di->di_contacts = ci;
        else {
            cp->set_next(ci);
            cp = cp->next();
        }
    }
    di->di_barea = di_barea;
    di->di_bperim = di_bperim;
    di->di_bmindim = di_bmindim;
    di->di_bdepth = di_bdepth;
    di->di_displayed = false;
    di->di_mstatus = di_mstatus;
    if (di_multi_devs) {
        sDevInst *de = 0;
        for (sDevInst *dx = di_multi_devs; dx; dx = dx->next()) {
            sDevInst *dxc = dx->copy(tstk, map);
            if (de) {
                de->set_next(dxc);
                de = de->next();
            }
            else {
                di->di_multi_devs = dxc;
                de = di->di_multi_devs;
            }
        }
    }

    // fdevs
    // fnum;
    // mvalues
    return (di);
}


// Actually perform the measurements.  For parallel devices, do the
// combining.
//
bool
sDevInst::measure()
{
    if (di_mvalues) {
        int i = 0;
        for (sMeasure *m = di_desc->measures(); m; m = m->next(), i++)
            m->result()->content.value = di_mvalues[i];
        return (true);
    }
    if (CDvdb()->getVariable(VA_NoMeasure))
        return (true);

    // If we find results here, use 'em.
    if (get_measures_from_property())
        return (true);

    if (di_index >= 0) {
        if (EX()->isVerbosePromptline()) {
            PL()->ShowPromptV("Measuring %s %d in %s ...",
                di_desc->name(), di_index, di_sdesc->cellname());
        }
    }

    int i = 0;
    for (sMeasure *m = di_desc->measures(); m; m = m->next(), i++) ;
    if (!i)
        i++;
    di_mvalues = new float[i];
    while (i--)
        di_mvalues[i] = 0.0;

    siVariable *tv = SIparse()->getVariables();
    SIparse()->setVariables(di_desc->variables());

    bool nogo = false;
    for (sMprim *mp = di_desc->mprims(); mp; mp = mp->next()) {
        if (!(mp->*mp->evfunc)(this)) {
            nogo = true;
            break;
        }
    }
    if (nogo) {
        SIparse()->setVariables(tv);
        return (false);
    }

    i = 0;
    for (sMeasure *m = di_desc->measures(); m; m = m->next(), i++) {
        di_mvalues[i] = 0.0;
        if (m->tree()) {
            siVariable v;
            SIlexprCx cx(di_sdesc, CDMAXCALLDEPTH, (const Zlist*)0);
            if (m->tree()->evfunc(m->tree(), &v, &cx) != OK) {
                ExtErrLog.add_err(
                    "In %s, measure: measure %s evaluation failed for %s %d.",
                    di_sdesc->cellname()->string(), m->name(),
                    di_desc->name()->stringNN(), di_index);
            }
            m->result()->content.value =
                (v.type == TYP_SCALAR ? v.content.value : 0.0);
            di_mvalues[i] = (float)m->result()->content.value;
        }
    }
    SIparse()->setVariables(tv);

    return (true);
}


// Perform the measures if not done, save the results to a
// XICP_MEASURES property on the device:data box.
//
bool
sDevInst::save_measures_in_prpty()
{
    bool ret = true;
    for (sDevInst *d = di_multi_devs; d; d = d->next()) {
        if (!d->save_measures_in_prpty())
            ret = false;
    }
    CDo *databox = get_my_data_box();
    if (!databox)
        return (false);
    if (!di_mvalues) {
        if (!measure())
            return (false);
    }
    if (!di_mvalues)
        return (ret);

    int cnt = 0;
    for (sMeasure *m = di_desc->measures(); m; m = m->next(), cnt++) ;
    if (!cnt)
        cnt++;
    sLstr lstr;

    for (int i = 0; i < cnt; i++) {
        if (lstr.string())
            lstr.add_c(' ');
        lstr.add_e(di_mvalues[i], 6);
        if (di_mvalues_permuted && di_mvalues_permuted[i] != di_mvalues[i]) {
            lstr.add_c(':');
            lstr.add_e(di_mvalues_permuted[i], 6);
        }
    }

    CDp *prp = databox->prpty(XICP_MEASURES);
    if (!prp) {
        prp = new CDp(XICP_MEASURES);
        databox->link_prpty_list(prp);
    }
    prp->set_string(lstr.string());
    return (ret);
}


// If an XICP_MEASURES property is found, set the measure results from
// it and return true.
//
bool
sDevInst::get_measures_from_property()
{
    if (!EX()->isUseMeasurePrpty() || EX()->isNoReadMeasurePrpty())
        return (false);

    bool ret = true;
    for (sDevInst *d = di_multi_devs; d; d = d->next()) {
        if (!d->get_measures_from_property())
            ret = false;
    }
    CDo *databox = get_my_data_box();
    if (!databox)
        return (false);
    CDp *prp = databox->prpty(XICP_MEASURES);
    if (!prp)
        return (false);

    int cnt = 0;
    for (sMeasure *m = di_desc->measures(); m; m = m->next(), cnt++) ;
    if (!cnt)
        cnt++;
    float *vals = new float[cnt];
    float *pvals = 0;
    vals[0] = 0;
    const char *s = prp->string();
    int i = 0;
    for (sMeasure *m = di_desc->measures(); m; m = m->next(), i++) {
        char *tok = lstring::gettok(&s);
        if (!tok) {
            // Wrong token count.
            delete [] vals;
            return (false);
        }
        char *e;
        vals[i] = strtof(tok, &e);
        if (e == tok) {
            // parse error
            delete [] vals;
            delete [] pvals;
            delete [] tok;
            return (false);
        }
        if (*e == ':') {
            if (!pvals) {
                pvals = new float[cnt];
                for (int j = 0; j < i; j++)
                    pvals[j] = vals[j];
            }
            const char *t = e+1;
            pvals[i] = strtof(t, &e);
            if (e == t) {
                // parse error
                delete [] vals;
                delete [] pvals;
                delete [] tok;
                return (false);
            }
        }
        else if (pvals)
            pvals[i] = vals[i];
        delete [] tok;
    }
    if (cnt > 1) {
        while (isspace(*s))
            s++;
        if (*s) {
            // Wrong token count.
            delete [] vals;
            delete [] pvals;
            return (false);
        }
    }
    delete [] di_mvalues;
    di_mvalues = vals;
    delete [] di_mvalues_permuted;
    di_mvalues_permuted = pvals;
    i = 0;
    for (sMeasure *m = di_desc->measures(); m; m = m->next(), i++)
        m->result()->content.value = di_mvalues[i];
    return (ret);
}


// If an XICP_MEASURES property is found, destroy it.
//
void
sDevInst::destroy_measures_property() const
{
    CDo *databox = get_my_data_box();
    if (databox)
        databox->prptyRemove(XICP_MEASURES);
}


// Check for a corresponding box on ld, create as needed.  Give the
// box a nonzero group number.
//
bool
sDevInst::setup_dev_layer(CDl *ld) const
{
    for (sDevInst *d = di_multi_devs; d; d = d->next()) {
        if (!d->setup_dev_layer(ld))
            return (false);
    }

    bool ret = true;
    CDg gdesc;
    gdesc.init_gen(di_sdesc, ld, bBB());
    CDo *od;
    bool found = false;
    while ((od = gdesc.next()) != 0) {
        if (od->type() == CDBOX && od->oBB() == *bBB()) {
            od->set_group(1);
            found = true;
        }
    }
    if (!found) {
        if (di_sdesc->makeBox(ld, bBB(), &od, false) == CDok && od)
            od->set_group(1);
        else
            ret = false;
    }
    return (ret);
}


// If this or any of its components intersects AOI, return a single
// sDevInstList link containing this.
//
sDevInstList *
sDevInst::getdev(BBox *AOI, bool *err)
{
    *err = false;
    if (!AOI)
        return (0);

    if (di_mstatus != MS_PARALLEL && di_mstatus != MS_SERIES) {
        if (AOI->intersect(&di_bBB, false)) {
            // Post-flattening, so we can't use a depth limit here.
            SIlexprCx cx(di_sdesc, CDMAXCALLDEPTH, AOI);
            Zlist *zret = 0;
            if (di_desc->body()->getZlist(&cx, &zret) != XIok) {
                *err = true;
                return (0);
            }
            if (zret) {
                Zlist::free(zret);
                return (new sDevInstList(this, 0));
            }
        }
        return (0);
    }
    else {
        for (sDevInst *dx = di_multi_devs; dx; dx = dx->next()) {
            sDevInstList *d0 = dx->getdev(AOI, err);
            if (*err)
                return (0);
            if (d0) {
                d0->dev = this;
                return (d0);
            }
        }
    }
    return (0);
}


// Place di in parallel with this.  It is assumed that di has been
// unlinked from the device list.
//
void
sDevInst::insert_parallel(sDevInst *di)
{
    if (!di)
        return;
    if (di_mstatus == MS_PARALLEL) {
        sDevInst *d = di_multi_devs;
        while (d->next())
            d = d->next();
        d->set_next(di);
        di->di_merged = true;
    }
    else {
        sDevInst *dx = new sDevInst(-1, di_desc, di_sdesc);
        dx->di_next = di;
        dx->di_contacts = di_contacts->dup_list(dx);
        dx->di_fdevs = di_fdevs;
        di_fdevs = 0;
        dx->di_fnum = di_fnum;
        di_fnum = 0;
        dx->di_bBB = di_bBB;
        dx->di_BB = di_BB;
        dx->di_barea = di_barea;
        dx->di_bperim = di_bperim;
        dx->di_bmindim = di_bmindim;
        dx->di_mvalues = di_mvalues;
        dx->di_mvalues_permuted = di_mvalues_permuted;
        di_mvalues = 0;
        di_mvalues_permuted = 0;
        dx->di_precmp = di_precmp;
        di_precmp = 0;
        dx->di_multi_devs = di_multi_devs;
        di_multi_devs = dx;
        dx->di_mstatus = di_mstatus;
        di_mstatus = MS_PARALLEL;
        di->di_merged = true;
        dx->di_merged = true;
    }
    for (sDevInst *d = di; d; d = d->next()) {
        d->di_index = -1;
        di_BB.add(&d->di_BB);
    }
}


// Place di in series with this.  It is assumed that di has been
// unlinked from the device list.
//
void
sDevInst::insert_series(sDevInst *di)
{
    if (!di)
        return;
    if (di_mstatus == MS_SERIES) {
        sDevInst *d = di_multi_devs;
        while (d->next())
            d = d->next();
        d->set_next(di);
    }
    else {
        sDevInst *dx = new sDevInst(-1, di_desc, di_sdesc);
        dx->di_next = di;
        dx->di_contacts = di_contacts->dup_list(dx);
        dx->di_fdevs = di_fdevs;
        di_fdevs = 0;
        dx->di_fnum = di_fnum;
        di_fnum = 0;
        dx->di_bBB = di_bBB;
        dx->di_BB = di_BB;
        dx->di_barea = di_barea;
        dx->di_bperim = di_bperim;
        dx->di_bmindim = di_bmindim;
        dx->di_mvalues = di_mvalues;
        dx->di_mvalues_permuted = di_mvalues_permuted;
        di_mvalues = 0;
        di_mvalues_permuted = 0;
        dx->di_precmp = di_precmp;
        di_precmp = 0;
        dx->di_multi_devs = di_multi_devs;
        di_multi_devs = dx;
        dx->di_mstatus = di_mstatus;
        di_mstatus = MS_SERIES;
    }
    for (sDevInst *d = di; d; d = d->next()) {
        d->di_index = -1;
        di_BB.add(&d->di_BB);
    }
}


// Set up and return a context for solving for resistance, or inductance
// if lmode is true.
//
RLsolver *
sDevInst::setup_squares(bool lmode) const
{
    if (!di_contacts || !di_contacts->next()) {
        const char *msg = "no contacts to device";
        ExtErrLog.add_dev_err(di_sdesc, this, msg);
        return (0);
    }
    CDl *bld = body_layer(lmode ? bl_induct : bl_resis);
    if (!bld) {
        const char *msgl = "bad or missing Tline,Thickness in body layer(s)";
        const char *msgr =
            "bad or missing Rsh or Rho,Thickness in body layer(s)";
        ExtErrLog.add_dev_err(di_sdesc, this, lmode ? msgl : msgr);
        return (0);
    }

    Zlist *zlist;
    SIlexprCx cx(di_sdesc, di_desc->depth() + di_bdepth, &di_bBB);
    if (di_desc->body()->getZlist(&cx, &zlist) == XIok && zlist) {
        // If the body list contains disjoint areas, find the piece that
        // intersects the first contact, and throw away the others.  This
        // can happen with odd shaped devices whose BB covers part of
        // another device.
        //
        Zgroup *g = Zlist::group(zlist);
        zlist = 0;
        if (g->num > 1) {
            for (int i = 0; i < g->num; i++) {
                if (Zlist::intersect(g->list[i], di_contacts->cBB(), true)) {
                    zlist = g->list[i];
                    g->list[i] = 0;
                    break;
                }
            }
        }
        else {
            zlist = g->list[0];
            g->list[0] = 0;
        }
        delete g;
    }
    if (!zlist) {
        const char *msg = "failed to extract body area";
        ExtErrLog.add_dev_err(di_sdesc, this, msg);
        return (0);
    }

    int numcont = count_contacts();
    Zgroup *cg = new Zgroup;
    cg->num = numcont;
    cg->list = new Zlist*[numcont];
    memset(cg->list, 0, numcont*sizeof(Zlist*));
    int i = 0;
    for (sDevContactInst *ci = di_contacts; ci;
            ci = ci->next(), i++) {
        SIlexprCx cx1(di_sdesc, di_desc->depth() + di_bdepth, ci->cBB());
        if (ci->desc()->lspec()->getZlist(&cx1, &cg->list[i]) != XIok ||
                !cg->list[i]) {
            const char *msg = "contact %s, failed to extract area";
            ExtErrLog.add_dev_err(di_sdesc, this, msg, ci->desc()->name());
            Zlist::free(zlist);
            delete cg;
            return (0);
        }
    }

    RLsolver *r = new RLsolver;
    bool ret = r->setup(zlist, bld, cg);
    Zlist::free(zlist);
    delete cg;

    const char *msg = "RLsolver error: %s";
    if (!ret) {
        ExtErrLog.add_dev_err(di_sdesc, this, msg, Errs()->get_error());
        delete r;
        return (0);
    }

    ret = lmode ? r->setupL() : r->setupR();
    if (!ret) {
        ExtErrLog.add_dev_err(di_sdesc, this, msg, Errs()->get_error());
        delete r;
        return (0);
    }
    return (r);
}


//
// Private sDevInst functions.
//

// Outline the device and its components, or, if eBB is given, find the
// total bounding box.
//
void
sDevInst::outline(WindowDesc *wdesc, BBox *eBB) const
{
    if (di_mstatus != MS_PARALLEL && di_mstatus != MS_SERIES) {
        if (eBB)
            eBB->add(&di_BB);
        else if (wdesc)
            wdesc->ShowBox(&di_BB, CDL_OUTLINED, 0);
        for (sDevContactInst *cx = di_contacts; cx; cx = cx->next())
            cx->show(wdesc, eBB);
    }
    else {
        for (sDevInst *dx = di_multi_devs; dx; dx = dx->next())
            dx->outline(wdesc, eBB);
    }
}


// Evaluate the measure expressions and place the results into the
// cache.  This assumes that the variable structs for the primitives
// hold the correct values.
//
void
sDevInst::cache_measures()
{
    if (!di_mvalues) {
        int i = 0;
        for (sMeasure *m = di_desc->measures(); m; m = m->next(),
            i++) ;
        if (!i)
            i++;
        di_mvalues = new float[i];
        while (i--)
            di_mvalues[i] = 0.0;
    }
    if (di_mstatus != MS_PARALLEL && di_mstatus != MS_SERIES) {
        siVariable *tv = SIparse()->getVariables();
        SIparse()->setVariables(di_desc->variables());
        int i = 0;
        for (sMeasure *m = di_desc->measures(); m; m = m->next(),i++) {
            di_mvalues[i] = 0.0;
            if (m->tree()) {
                siVariable v;
                SIlexprCx cx(di_sdesc, CDMAXCALLDEPTH, (const Zlist*)0);
                if (m->tree()->evfunc(m->tree(), &v, &cx) != OK) {
                    ExtErrLog.add_err(
                        "cacheMeasures: measure %s evaluation "
                        "failed for %s %d.", m->name(),
                        di_desc->name()->stringNN(), di_index);
                }
                m->result()->content.value =
                    (v.type == TYP_SCALAR ? v.content.value : 0.0);
                di_mvalues[i] = (float)m->result()->content.value;
            }
        }
        SIparse()->setVariables(tv);
    }
}


// Return the "data box" for this device.  The data box is a thing to
// hang properties on.
//
CDo *
sDevInst::get_my_data_box() const
{
    CDl *ld = CDldb()->findLayer(EXT_DEV_LPP, Physical);
    if (!ld)
        return (0);
    CDg gdesc;
    gdesc.init_gen(di_sdesc, ld, bBB());
    CDo *od;
    while ((od = gdesc.next()) != 0) {
        if (od->type() == CDBOX && od->oBB() == *bBB())
            return (od);
    }
    return (0);
}
// End sDevInst functions.


// Add di to the list(s) of instances for this device name.  These are
// kept in separate lists for each prefix.
//
void
sDevList::add(sDevInst *di)
{
    if (di) {
        if (!dl_prefixes) {
            dl_prefixes = new sDevPrefixList(di, 0);
            return;
        }
        for (sDevPrefixList *p = dl_prefixes; p; p = p->next()) {
            if ((di->desc()->prefix() && p->devs()->desc()->prefix() &&
                    !strcmp(di->desc()->prefix(),
                        p->devs()->desc()->prefix())) ||
                    (!di->desc()->prefix() && !p->devs()->desc()->prefix())) {
                sDevInst *dx = p->devs();
                while (dx->next())
                    dx = dx->next();
                dx->set_next(di);
                return;
            }
        }
        sDevPrefixList *p = dl_prefixes;
        while (p->next())
            p = p->next();
        p->set_next(new sDevPrefixList(di, 0));
    }
}


// Return the device instance from the list that matches the given
// device instance (identical bounding box).
//
sDevInst *
sDevList::find_in_list(sDevInst *dref)
{
    for (sDevList *d = this; d; d = d->next()) {
        for (sDevPrefixList *p = d->prefixes(); p; p = p->next()) {
            if (p->devs()->desc() == dref->desc()) {
                for (sDevInst *di = p->devs(); di; di = di->next()) {
                    if (*di->BB() == *dref->BB())
                        return (di);
                }
            }
            return (0);
        }
    }
    return (0);
}
// End of sDevList functions.


// Return a list containing all of the objects in the contact groups
// of c1.  These are objects from a single group and layer, but the
// list may be long if the device is compound.
//
CDol *
sGroupXf::find_objects(const sDevContactInst *c1, const CDl *ld)
{
    // First, recursively grab an object that overlaps every contact
    // BB.

    SymTab *tab = 0;
    if (gx_objs)
        tab = new SymTab(false, false);
    CDol *ol0 = 0;
    if (c1->dev()->mstatus() == MS_PARALLEL) {
        for (sDevInst *dx = c1->dev()->multi_devs(); dx; dx = dx->next()) {
            sDevContactInst *cx = dx->find_contact(c1->desc()->name());
            if (cx) {
                CDol *ol = find_objects_rc(cx, ld, tab);
                if (ol) {
                    if (ol0) {
                        CDol *ox = ol;
                        while (ox->next)
                            ox = ox->next;
                        ox->next = ol0;
                    }
                    ol0 = ol;
                }
            }
        }
    }
    else if (gx_sdesc) {
        CDg gdesc;
        gdesc.init_gen(gx_sdesc, ld, c1->cBB());
        CDo *od;
        while ((od = gdesc.next()) != 0) {
            if (od->has_flag(CDoMark2))
                continue;
            if (od->intersect(c1->cBB(), false)) {
                od->set_flag(CDoMark2);
                ol0 = new CDol(od, ol0);
            }
        }
    }
    else if (gx_objs) {
        CDol *ol = gx_objs->find_object(ld, c1->cBB());
        if (ol) {
            if (tab->get((unsigned long)ol->odesc) != ST_NIL) {
                ol->free();
                return (0);
            }
            tab->add((unsigned long)ol->odesc, 0, false);
            ol->next = ol0;
            ol0 = ol;
        }
    }

    // Now accumulate other objects that touch the objects already
    // gathered.

    if (gx_sdesc) {
        if (ol0) {
            CDol *oe = ol0;
            while (oe->next)
                oe = oe->next;
            for (CDol *o = ol0; o; o = o->next) {
                CDg gdesc;
                gdesc.init_gen(gx_sdesc, ld, &o->odesc->oBB());
                CDo *od;
                while ((od = gdesc.next()) != 0) {
                    if (od->has_flag(CDoMark2))
                        continue;
                    if (od->intersect(o->odesc, false)) {
                        od->set_flag(CDoMark2);
                        oe->next = new CDol(od, 0);
                        oe = oe->next;
                    }
                }
            }
        }
    }
    else if (gx_objs) {
        gx_objs->accumulate(ol0, tab);
        delete tab;
    }
    return (ol0);
}


// Private recursive tail for find_objects.
//
CDol *
sGroupXf::find_objects_rc(const sDevContactInst *c1, const CDl *ld,
    SymTab *tab)
{
    CDol *ol0 = 0;
    if (c1->dev()->mstatus() == MS_PARALLEL) {
        for (sDevInst *dx = c1->dev()->multi_devs(); dx; dx = dx->next()) {
            sDevContactInst *cx = dx->find_contact(c1->desc()->name());
            if (cx) {
                CDol *ol = find_objects_rc(cx, ld, tab);
                if (ol) {
                    if (ol0) {
                        CDol *ox = ol;
                        while (ox->next)
                            ox = ox->next;
                        ox->next = ol0;
                    }
                    ol0 = ol;
                }
            }
        }
    }
    else if (gx_sdesc) {
        CDg gdesc;
        gdesc.init_gen(gx_sdesc, ld, c1->cBB());
        CDo *od;
        while ((od = gdesc.next()) != 0) {
            if (od->has_flag(CDoMark2))
                continue;
            od->set_flag(CDoMark2);
            ol0 = new CDol(od, ol0);
        }
    }
    else if (gx_objs) {
        CDol *ol = gx_objs->find_object(ld, c1->cBB());
        if (ol) {
            if (tab->get((unsigned long)ol->odesc) != ST_NIL) {
                ol->free();
                return (0);
            }
            tab->add((unsigned long)ol->odesc, 0, false);
            ol->next = ol0;
            ol0 = ol;
        }
    }
    return (ol0);
}


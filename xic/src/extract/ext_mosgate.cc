
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
#include "ext.h"
#include "ext_extract.h"
#include "cd_terminal.h"
#include "cd_netname.h"


//========================================================================
//
// This does some circuit tracing to recognoze subcircuit terminals
// that can be considered equivalent, as they go to NAND or NOR gate
// structure inputs.  Note that such inputs are not topologically
// equivalent, but are generally considered to be electrically
// equivalent.  This applies to CMOS technology only.  Identified
// equivalent inputs are put into permutation groups that are used
// during association and LVS.
//
// This code works when the transistors are vectored, which
// complicates recognition.  In this case, it is MANDATORY that
// parallel merging be applied.
//
//========================================================================


// Convenience structure to extract and provide the contact group
// numbers.
//
struct sMOSgrps
{
    sMOSgrps(const sDevInst *di)
        {
            mg_g = -1;
            mg_d = -1;
            mg_s = -1;
            if (!di || (!di->is_nmos() && !di->is_pmos()))
                return;
            for (sDevContactInst *ci = di->contacts(); ci; ci = ci->next()) {
                const char *cn = Tstring(ci->cont_name());
                if (!cn)
                    continue;
                switch (*cn) {
                    case 'g':
                    case 'G':
                        mg_g = ci->group();
                        continue;
                    case 'd':
                    case 'D':
                        mg_d = ci->group();
                        continue;
                    case 's':
                    case 'S':
                        mg_s = ci->group();
                        continue;
                    default:
                        break;
                }
            }
        }

    // Return true if the two device channels are connected in parallel.
    //
    bool parallel_channels(const sMOSgrps &m) const
        {
            return ((d() == m.d() && s() == m.s()) ||
                (d() == m.s() && s() == m.d()));
        }

    int g()     const { return (mg_g); }
    int d()     const { return (mg_d); }
    int s()     const { return (mg_s); }
    bool ok()   const { return (mg_g >= 0 && mg_d >= 0 && mg_s >= 0); }

private:
    int mg_g;
    int mg_d;
    int mg_s;
};


namespace {
    // Return true of ci is an NMOS source or drain.
    //
    bool is_nmos_sd(const sDevContactInst *ci)
    {
        if (!ci)
            return (false);
        const char *cn = Tstring(ci->cont_name());
        if (!cn)
            return (false);
        switch (*cn) {
            case 'd':
            case 'D':
            case 's':
            case 'S':
                break;
            default:
                return (false);
        }
        return (ci->dev() && ci->dev()->is_nmos());
    }


    // Return true of ci is a PMOS source or drain.
    //
    bool is_pmos_sd(const sDevContactInst *ci)
    {
        if (!ci)
            return (false);
        const char *cn = Tstring(ci->cont_name());
        if (!cn)
            return (false);
        switch (*cn) {
            case 'd':
            case 'D':
            case 's':
            case 'S':
                break;
            default:
                return (false);
        }
        return (ci->dev() && ci->dev()->is_pmos());
    }


    int gate_group(const sDevInst *di)
    {
        for (sDevContactInst *ci = di->contacts(); ci; ci = ci->next()) {
            const char *n = Tstring(ci->cont_name());
            if (n && (*n == 'g' || *n == 'G'))
                return (ci->group());
        }
        return (-1);
    }


    /***
    int drain_group(const sDevInst *di)
    {
        for (sDevContactInst *ci = di->contacts(); ci; ci = ci->next()) {
            const char *n = ci->cont_name()->string();
            if (n && (*n == 'd' || *n == 'D'))
                return (ci->group());
        }
        return (-1);
    }


    int source_group(const sDevInst *di)
    {
        for (sDevContactInst *ci = di->contacts(); ci; ci = ci->next()) {
            const char *n = ci->cont_name()->string();
            if (n && (*n == 's' || *n == 'S'))
                return (ci->group());
        }
        return (-1);
    }
    ***/
}


// Return true if the two devices are permutable totem-pole ends. 
// These would be the end devices connected to the gate output, of
// parallel-connected totem poles.  The parallel-connected totem poles
// are permutable.  This function detects this case in device
// identification, avoiding time-consuming symmetry trials.
// *** FUNCTION NOT USED ***
//
bool
cGroupDesc::is_mos_tpeq(const sDevInst *d1, const sDevInst *d2) const
{
    return (false);
    if (!d1 || !d2)
        return (false);
    sDevDesc *dd1 = d1->desc();
    sDevDesc *dd2 = d2->desc();
    if (!dd1->is_mos() || !dd2->is_mos())
        return (false);
     sMOSgrps mg1(d1);
     sMOSgrps mg2(d2);
     if (!mg1.ok() || !mg2.ok())
         return (false);

    // Gate groups the same?
    if (mg1.g() != mg2.g())
        return (false);

    // One common permutable group connection?
    int grp1, grp2;
    if (mg1.d() == mg2.d()) {
        grp1 = mg1.s();
        grp2 = mg2.s();
    }
    else if (mg1.d() == mg2.s()) {
        grp1 = mg1.s();
        grp2 = mg2.d();
    }
    else if (mg1.s() == mg2.d()) {
        grp1 = mg1.d();
        grp2 = mg2.s();
    }
    else if (mg1.s() == mg2.s()) {
        grp1 = mg1.d();
        grp2 = mg2.d();
    }
    else
        return (false);

    if (grp1 == grp2)
        return (true);  // parallel device

    // Walk the totem poles to the end.  The end is indicated when we
    // reach a group that has other than exactly two connections, the
    // second one being a permutable contact to the same type of
    // device, with the gate connected to the same group.

    for (;;) {
        sDevContactList *cl1 = gd_groups[grp1].device_contacts();
        sDevContactList *cl2 = gd_groups[grp2].device_contacts();
        if (!cl1 || !cl1->next() || cl1->next()->next())
            break;
        if (!cl2 || !cl2->next() || cl2->next()->next())
            break;
        // Exactly one other device contact in this group.

        sDevContactInst *c1;
        if (cl1->contact()->dev() == d1)
            c1 = cl1->next()->contact();
        else if (cl1->next()->contact()->dev() == d1)
            c1 = cl1->contact();
        else
            return (false);
        sDevContactInst *c2;
        if (cl2->contact()->dev() == d2)
            c2 = cl2->next()->contact();
        else if (cl2->next()->contact()->dev() == d2)
            c2 = cl2->contact();
        else
            return (false);

        // Find the devices owning the second contacts.  They should
        // be the same type of device as the provious devices, and the
        // contact should be permutable, and the gate connection
        // groups of the new devices should be the same.

        sDevInst *di_c1 = c1->dev();
        if (di_c1->desc()->name() != d1->desc()->name())
            return (false);
        if (!dd1->is_permute(c1->desc()->name()))
            return (false);
        sDevInst *di_c2 = c2->dev();
        if (di_c1->desc()->name() != d1->desc()->name())
            return (false);
        if (!dd2->is_permute(c2->desc()->name()))
            return (false);

        // The gate groups of the connected devices are the same?
        if (gate_group(di_c1) != gate_group(di_c2))
            return (false);

        grp1 = c1->group();
        grp2 = c2->group();
        d1 = di_c1;
        d2 = di_c2;
    }
    // Found end of totem pole for at least one.  If the groups are
    // the same, the totem poles are parallel and can be permuted, so
    // return true.
    return (grp1 == grp2);
}


// Return true if the contact is a permutation candidate.  Such a
// contact connects only to gates, with at least one of each type
// (NMOS or PMOS), and exactly one of one type.  This *REQUIRES*
// that device merging be enabled.  If multi-component devices are
// used, the PMOS will merge to a single transistor in NAND
// structures, and NMOS will merge to a single transistor in NOR
// structures.
//
// The pointers are set to a device found if there is only one of
// this type.  Otherwise the pcount indicates the number of devices.
//
bool
cGroupDesc::mos_np_input(const CDsterm *term, sDevInst **ptndev,
    sDevInst **ptpdev, int *pcount) const
{
    if (ptndev)
        *ptndev = 0;
    if (ptpdev)
        *ptpdev = 0;
    if (pcount)
        *pcount = 1;
    int grp = term->group();
    if (grp < 0)
        return (false);
    sGroup *g = group_for(grp);
    if (!g)
        return (false);
    if (g->subc_contacts())
        return (false);
    int ncnt = 0;
    int pcnt = 0;
    sDevInst *ndev = 0;
    sDevInst *pdev = 0;
    for (sDevContactList *cl = g->device_contacts(); cl; cl = cl->next()) {
        sDevContactInst *ci = cl->contact();
        if (ci->dev()->is_nmos()) {
            ncnt++;
            ndev = ci->dev();
        }
        else if (ci->dev()->is_pmos()) {
            pcnt++;
            pdev = ci->dev();
        }
        else
            return (false);
        const char *n = Tstring(ci->cont_name());
        if (!n)
            return (false);
        if (*n != 'g' && *n != 'G')
            return (false);
    }
    if (!ncnt || !pcnt)
        return (false);

    // Merging must be enabled.  If the transistors are
    // multi-component, the PMOS of a NAND or ther NMOS of a NOR
    // will be merged to a single device.  The other transistors
    // won't be merged.
    if (ncnt != 1 && pcnt != 1)
        return (false);
    if (pcnt == 1 && ptpdev)
        *ptpdev = pdev;
    if (ncnt == 1 && ptndev)
        *ptndev = ndev;
    if (pcnt > 1 && pcount)
        *pcount = pcnt;
    else if (ncnt > 1 && pcount)
        *pcount = ncnt;

    return (true);
}


// Return true if the devices are connected as part of a NAND
// gate, so that the inputs can be permuted.
//
bool
cGroupDesc::nand_match(int group1, int group2, const sDevInst *pmos1,
    const sDevInst *pmos2, int dcnt) const
{
    if (!pmos1 || !pmos2)
        return (false);
    sMOSgrps pm1(pmos1);
    if (!pm1.ok())
        return (false);
    sMOSgrps pm2(pmos2);
    if (!pm2.ok())
        return (false);

    // First test:  the PMOS channels are in parallel.
    if (!pm1.parallel_channels(pm2))
        return (false);

    // Second test:  the NMOS channels are in the same totem-pole. 
    // If dcnt > 1, the devices are compound.  We find the top row
    // of NMOS devices, and walk down each component, finding our
    // gate nodes along the way.

    int dc;
    int ttop = find_nmos_totem_top(pm1.s(), pm1.d(), &dc);
    if (ttop < 0)
        return (false);
    if (dcnt != dc)
        return (false);

    sGroup *g = group_for(ttop);
    for (sDevContactList *cl = g->device_contacts(); cl; cl = cl->next()) {
        sDevContactInst *ci = cl->contact();
        if (!is_nmos_sd(ci))
            continue;
        sDevInst *ntop = ci->dev();

        bool found1 = false, found2 = false;
        int tgrp = ttop;
        for (;;) {
            if (!found1 && group1 == gate_group(ntop))
                found1 = true;
            if (!found2 && group2 == gate_group(ntop))
                found2 = true;
            if (found1 && found2)
                break;
            sDevInst *tnew;
            tgrp = find_nmos_totem_next(tgrp, ntop, &tnew);
            if (tgrp < 0)
                break;
            ntop = tnew;
        }
        if (!found1 || !found2)
            return (false);
    }
    return (true);
}


// Return true if the devices are connected as part of a NOR
// gate, so that the inputs can be permuted.
//
bool
cGroupDesc::nor_match(int group1, int group2, const sDevInst *nmos1,
    const sDevInst *nmos2, int dcnt) const
{
    if (!nmos1 || !nmos2)
        return (false);
    sMOSgrps nm1(nmos1);
    if (!nm1.ok())
        return (false);
    sMOSgrps nm2(nmos2);
    if (!nm2.ok())
        return (false);

    // First test:  the NMOS channels are in parallel.
    if (!nm1.parallel_channels(nm2))
        return (false);

    // Second test:  the PMOS channels are in the same totem-pole. 
    // If dcnt > 1, the devices are compound.  We find the bottom
    // row of PMOS devices, and walk up each component, finding
    // our gate nodes along the way.

    int dc;
    int tbot = find_pmos_totem_bot(nm1.s(), nm1.d(), &dc);
    if (tbot < 0)
        return (false);
    if (dcnt != dc)
        return (false);

    sGroup *g = group_for(tbot);
    for (sDevContactList *cl = g->device_contacts(); cl; cl = cl->next()) {
        sDevContactInst *ci = cl->contact();
        if (!is_pmos_sd(ci))
            continue;
        sDevInst *pbot = ci->dev();

        bool found1 = false, found2 = false;
        int tgrp = tbot;
        for (;;) {
            if (!found1 && group1 == gate_group(pbot))
                found1 = true;
            if (!found2 && group2 == gate_group(pbot))
                found2 = true;
            if (found1 && found2)
                break;
            sDevInst *tnew;
            tgrp = find_pmos_totem_next(tgrp, pbot, &tnew);
            if (tbot < 0)
                break;
            pbot = tnew;
        }
        if (!found1 || !found2)
            return (false);
    }
    return (true);
}


// We guess that either sgrp or dgrp is the top group of the NMOS
// totem pole in a NAND structure.  Identify and return the
// correct group, also set pcount to the number of NMOS components
// found.  Each component must have the same gate group.
//
int
cGroupDesc::find_nmos_totem_top(int sgrp, int dgrp, int *pcount) const
{
    if (pcount)
        *pcount = 1;

    // The "top of totem pole" group contains one or more NMOS
    // source or drain, and is probably not a global net, being
    // the gate output net.  The "wrong" choice is probably vdd
    // which is likely global.  If there is more than one NMOS,
    // all their gates should be connected mutually.

    int dc1;
    bool r1 = find_nmos_sd(sgrp, &dc1);
    int dc2;
    bool r2 = find_nmos_sd(dgrp, &dc2);
    if (!r1 && !r2)
        return (-1);
    if (r1 && r2) {
        // Hmmmm, both match.  Choose the one that is not global
        // if this case, if one is global.

        bool g1 = group_for(sgrp)->global();
        bool g2 = group_for(dgrp)->global();
        if (g1 && !g2) {
            if (pcount)
                *pcount = dc2;
            return (dgrp);
        }
        if (!g1 && g2) {
            if (pcount)
                *pcount = dc1;
            return (sgrp);
        }
        return (-1);
    }
    if (r1) {
        if (pcount)
            *pcount = dc1;
        return (sgrp);
    }
    if (pcount)
        *pcount = dc2;
    return (dgrp);
}


// We guess that either sgrp or dgrp is the bottom group of the
// PMOS totem pole in a NOR structure.  Identify and return the
// correct group, also set ptdev to the bottom PMOS.
//
int
cGroupDesc::find_pmos_totem_bot(int sgrp, int dgrp, int *pcount) const
{
    if (pcount)
        *pcount = 1;

    // The "bottom of totem pole" group contains one or more PMOS
    // source or drain, and is probably not a global net, being
    // the gate output net.  The "wrong" choice is probably gnd
    // which is likely global.  If there is more than one NMOS,
    // all their gates should be connected mutually.

    int dc1;
    bool r1 = find_pmos_sd(sgrp, &dc1);
    int dc2;
    bool r2 = find_pmos_sd(dgrp, &dc2);
    if (!r1 && !r2)
        return (-1);
    if (r1 && r2) {
        // Hmmmm, both match.  Coose the one that is not global
        // if this case, if one is global.

        bool g1 = group_for(sgrp)->global();
        bool g2 = group_for(dgrp)->global();
        if (g1 && !g2) {
            if (pcount)
                *pcount = dc2;
            return (dgrp);
        }
        if (!g1 && g2) {
            if (pcount)
                *pcount = dc1;
            return (sgrp);
        }
        return (-1);
    }
    if (r1) {
        if (pcount)
            *pcount = dc1;
        return (sgrp);
    }
    if (pcount)
        *pcount = dc2;
    return (dgrp);
}


// Identify and return the group and NMOS device below the device
// and top group passed, or -1 if we are already at the bottom.
//
int
cGroupDesc::find_nmos_totem_next(int top, const sDevInst *dev,
    sDevInst **ptdev) const
{
    if (ptdev)
        *ptdev = 0;

    sMOSgrps nm(dev);
    int nextgrp = -1;
    if (nm.d() == top)
        nextgrp = nm.s();
    else if (nm.s() == top)
        nextgrp = nm.d();
    else
        return (-1);

    sGroup *g = group_for(nextgrp);
    if (!g)
        return (-1);
    if (g->subc_contacts())
        return (-1);

    // This group should have exactly two connections, one to the
    // source or drain of dev, the other to the source or drain of
    // the NMOS we're looking for.

    sDevContactList *cl = g->device_contacts();
    if (!cl || !cl->next() || cl->next()->next())
        return (-1);
    sDevContactInst *c1 = cl->contact();
    if (!is_nmos_sd(c1))
        return (-1);
    sDevContactInst *c2 = cl->next()->contact();
    if (!is_nmos_sd(c2))
        return (-1);
    if (c1->dev() == c2->dev())
        return (-1);
    if (c1->dev() == dev) {
        if (ptdev)
            *ptdev = c2->dev();
        return (nextgrp);
    }
    if (c2->dev() == dev) {
        if (ptdev)
            *ptdev = c1->dev();
        return (nextgrp);
    }
    return (-1);
}


// Identify and return the group and PMOS device above the device
// and bottom group passed, or -1 if we are already at the top.
//
int
cGroupDesc::find_pmos_totem_next(int bot, const sDevInst *dev,
    sDevInst **ptdev) const
{
    if (ptdev)
        *ptdev = 0;

    sMOSgrps pm(dev);
    int nextgrp = -1;
    if (pm.d() == bot)
        nextgrp = pm.s();
    else if (pm.s() == bot)
        nextgrp = pm.d();
    else
        return (-1);

    sGroup *g = group_for(nextgrp);
    if (!g)
        return (-1);
    if (g->subc_contacts())
        return (-1);

    // This group should have exactly two connections, one to the
    // source or drain of dev, the other to the source or drain of
    // the PMOS we're looking for.

    sDevContactList *cl = g->device_contacts();
    if (!cl || !cl->next() || cl->next()->next())
        return (-1);
    sDevContactInst *c1 = cl->contact();
    if (!is_pmos_sd(c1))
        return (-1);
    sDevContactInst *c2 = cl->next()->contact();
    if (!is_pmos_sd(c2))
        return (-1);
    if (c1->dev() == c2->dev())
        return (-1);
    if (c1->dev() == dev) {
        if (ptdev)
            *ptdev = c2->dev();
        return (nextgrp);
    }
    if (c2->dev() == dev) {
        if (ptdev)
            *ptdev = c1->dev();
        return (nextgrp);
    }
    return (-1);
}


// Return true if grp has NMOS source or drain connections, at least
// one.  Set pcount to the number of NMOS devices.
//
bool
cGroupDesc::find_nmos_sd(int grp, int *pcount) const
{
    if (pcount)
        *pcount = 1;
    sGroup *g = group_for(grp);
    if (!g)
        return (false);
    int dcnt = 0;
    for (sDevContactList *cl = g->device_contacts(); cl; cl = cl->next()) {
        sDevContactInst *ci = cl->contact();
        if (is_nmos_sd(ci))
            dcnt++;
    }
    if (!dcnt)
        return (false);
    if (dcnt == 1)
        return (true);

    // Make sure that the NMOS gates are connected.
    int ggrp = -1;
    for (sDevContactList *cl = g->device_contacts(); cl; cl = cl->next()) {
        sDevContactInst *ci = cl->contact();
        if (is_nmos_sd(ci)) {
            int gg = gate_group(ci->dev());
            if (ggrp < 0)
                ggrp = gg;
            else if (ggrp != gg)
                return (false);
        }
    }
    *pcount = dcnt;
    return (true);
}


// Return true if grp has PMOS source or drain connections, at least
// one.  Set pcount to the number of PMOS devices.
//
bool
cGroupDesc::find_pmos_sd(int grp, int *pcount) const
{
    if (pcount)
        *pcount = 1;
    sGroup *g = group_for(grp);
    if (!g)
        return (false);
    int dcnt = 0;
    for (sDevContactList *cl = g->device_contacts(); cl; cl = cl->next()) {
        sDevContactInst *ci = cl->contact();
        if (is_pmos_sd(ci))
            dcnt++;
    }
    if (!dcnt)
        return (false);
    if (dcnt == 1)
        return (true);

    // Make sure that the PMOS gates are connected.
    int ggrp = -1;
    for (sDevContactList *cl = g->device_contacts(); cl; cl = cl->next()) {
        sDevContactInst *ci = cl->contact();
        if (is_pmos_sd(ci)) {
            int gg = gate_group(ci->dev());
            if (ggrp < 0)
                ggrp = gg;
            else if (ggrp != gg)
                return (false);
        }
    }
    *pcount = dcnt;
    return (true);
}
// End of cGroupDesc functions.


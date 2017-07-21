
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 1996 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *                                                                        *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,      *
 *   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES      *
 *   OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-        *
 *   INFRINGEMENT.  IN NO EVENT SHALL STEPHEN R. WHITELEY BE LIABLE       *
 *   FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION      *
 *   OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN           *
 *   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN         *
 *   THE SOFTWARE.                                                        *
 *                                                                        *
 *========================================================================*
 *                                                                        *
 * Device Library                                                         *
 *                                                                        *
 *========================================================================*
 $Id: urcset.cc,v 2.14 2016/05/12 16:21:46 stevew Exp $
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3f2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 Thomas L. Quarles
         1993 Stephen R. Whiteley
****************************************************************************/

#include <stdio.h>
#include "urcdefs.h"


int
URCdev::setup(sGENmodel *genmod, sCKT *ckt, int *state)
{
    (void)state;

    int rtype = ckt->typelook("Resistor");
    int ctype = ckt->typelook("Capacitor");
    int dtype = ckt->typelook("Diode");
    sURCmodel *model = static_cast<sURCmodel*>(genmod);
    for ( ; model; model = model->next()) {
        if (!model->URCkGiven)
             model->URCk = 1.5;
        if (!model->URCfmaxGiven)
             model->URCfmax = 1e9;
        if (!model->URCrPerLGiven)
             model->URCrPerL = 1000;
        if (!model->URCcPerLGiven)
             model->URCcPerL = 1e-12;

// may need to put in limits:  k>=1.1, freq <=1e9, rperl >=.1

        sURCinstance *inst;
        for (inst = model->inst(); inst; inst = inst->next()) {
            if (inst->URCsetupDone)
                continue;
            inst->URCsetupDone = 1;
            double p = model->URCk;
            double r0 = inst->URClength * model->URCrPerL;
            double c0 = inst->URClength * model->URCcPerL;
            double i0 = inst->URClength * model->URCisPerL;
            if (!inst->URClumpsGiven) {
                double wnorm = model->URCfmax * r0 * c0 * 2.0 * M_PI;
                inst->URClumps = (int)
                    SPMAX(3.0, log(wnorm*(((p-1)/p)*((p-1)/p)))/log(p));
                if (wnorm < 35) inst->URClumps = 3;
                // may want to limit lumps to <= 100 or something like that
            }
            double r1 = (r0*(p-1))/((2*(pow(p, (double)inst->URClumps)))-2);
            double c1 =
                (c0*(p-1))/((pow(p, (double)(inst->URClumps-1)))*(p+1)-2);
            double i1 =
                (i0*(p-1))/((pow(p, (double)(inst->URClumps-1)))*(p+1)-2);
            double rd = inst->URClength * inst->URClumps * model->URCrsPerL;
            // may want to check that c1 > 0
            double prop = 1;
            char *nameelt;
            char *namehi;
            char *namelo;
            sCKTnode *nodehi;
            sCKTnode *nodelo;
            IFdata data;
            data.type = IF_REAL;
            int i;
            sGENinstance *fast;
            sGENmodel *modfast;  // capacitor or diode model
            int error;
            IFuid resUid;
            IFuid capUid;
            IFuid dioUid;
            IFuid eltUid;
            if (model->URCisPerLGiven) {
                error = ckt->newUid(&dioUid, inst->GENname, "diodemod",
                    UID_MODEL);
                if (error)
                    return (error);
                modfast = 0;
                error = ckt->newModl(dtype, &modfast, dioUid);
                if (error)
                    return (error);
                data.v.rValue = c1;
                error = modfast->setParam("cjo", &data);
                if (error)
                    return (error);
                data.v.rValue = rd;
                error = modfast->setParam("rs", &data);
                if (error)
                    return (error);
                data.v.rValue = i1;
                error = modfast->setParam("is", &data);
                if (error)
                    return (error);
            }
            else {
                error = ckt->newUid(&capUid, inst->GENname, "capmod",
                    UID_MODEL);
                if (error)
                    return (error);
                modfast = 0;
                error = ckt->newModl(ctype, &modfast, capUid);
                if (error)
                    return (error);
            }
            error = ckt->newUid(&resUid, inst->GENname, "resmod",
                UID_MODEL);
            if (error)
                return (error);
            sGENmodel *rmodfast = 0; // resistor model
            error = ckt->newModl(rtype, &rmodfast, resUid);
            if (error)
                return (error);
            sCKTnode *lowl = ckt->num2node(inst->URCposNode);
            sCKTnode *hir = ckt->num2node(inst->URCnegNode);
            for (i = 1; i <= inst->URClumps; i++) {
                char namebf[16];
                sprintf(namebf, "hi%d", i);
                namehi = new char[strlen(namebf)+1];
                strcpy(namehi, namebf);
                error = ckt->mkVolt(&nodehi, inst->GENname, namehi);
                if (error)
                    return (error);
                sCKTnode *hil = nodehi;
                sCKTnode *lowr;
                if (i == inst->URClumps)
                    lowr = hil;
                else {
                    sprintf(namebf, "lo%d", i);
                    namelo = new char[strlen(namebf)+1];
                    strcpy(namelo, namebf);
                    error = ckt->mkVolt(&nodelo, inst->GENname, namelo);
                    if (error)
                        return (error);
                    lowr = nodelo;
                }
                double r = prop*r1;
                double c = prop*c1;

                sprintf(namebf, "rlo%d",i);
                nameelt =  new char[strlen(namebf)+1];
                strcpy(nameelt, namebf);
                error = ckt->newUid(&eltUid, inst->GENname, nameelt,
                    UID_INSTANCE);
                if (error)
                    return (error);
                error = ckt->newInst(rmodfast, &fast, eltUid);
                if (error)
                    return (error);
                error = lowl->bind(fast, 1);
                if (error)
                    return (error);
                error = lowr->bind(fast, 2);
                if (error)
                    return (error);
                data.v.rValue = r;
                error = fast->setParam("resistance", &data);
                if (error)
                    return (error);

                sprintf(namebf, "rhi%d", i);
                nameelt = new char[strlen(namebf)+1];
                strcpy(nameelt, namebf);
                error = ckt->newUid(&eltUid, inst->GENname, nameelt,
                    UID_INSTANCE);
                if (error)
                    return (error);
                error = ckt->newInst(rmodfast, &fast, eltUid);
                if (error)
                    return (error);
                error = hil->bind(fast, 1);
                if (error) return (error);
                error = hir->bind(fast, 2);
                if (error)
                    return (error);
                data.v.rValue = r;
                error = fast->setParam("resistance", &data);
                if (error)
                    return (error);

                if (model->URCisPerLGiven) {
                    // use diode
                    sprintf(namebf, "dlo%d", i);
                    nameelt = new char[strlen(namebf)+1];
                    strcpy(nameelt, namebf);
                    error = ckt->newUid(&eltUid, inst->GENname, nameelt,
                        UID_INSTANCE);
                    if (error)
                        return (error);
                    error = ckt->newInst(modfast, &fast, eltUid);
                    if (error)
                        return (error);
                    error = lowr->bind(fast, 1);
                    if (error)
                        return (error);
                    error = ckt->num2node(inst->URCgndNode)->bind(fast, 2);
                    if (error)
                        return (error);
                    data.v.rValue = prop;
                    error = fast->setParam("area", &data);
                    if (error)
                        return (error);
                }
                else {
                    // use simple capacitor
                    sprintf(namebf, "clo%d", i);
                    nameelt = new char[strlen(namebf)+1];
                    strcpy(nameelt, namebf);
                    error = ckt->newUid(&eltUid, inst->GENname, nameelt,
                        UID_INSTANCE);
                    if (error)
                        return (error);
                    error = ckt->newInst(modfast, &fast, eltUid);
                    if (error)
                        return (error);
                    error = lowr->bind(fast, 1);
                    if (error)
                        return (error);
                    error = ckt->num2node(inst->URCgndNode)->bind(fast, 2);
                    if (error)
                        return (error);
                    data.v.rValue = c;
                    error = fast->setParam("capacitance", &data);
                    if (error)
                        return (error);
                }

                if (i != inst->URClumps){
                    if (model->URCisPerLGiven) {
                        // use diode
                        sprintf(namebf, "dhi%d", i);
                        nameelt = new char[strlen(namebf)+1];
                        strcpy(nameelt, namebf);
                        error = ckt->newUid(&eltUid, inst->GENname, nameelt,
                            UID_INSTANCE);
                        if (error)
                            return (error);
                        error = ckt->newInst(modfast, &fast,eltUid);
                        if (error)
                            return (error);
                        error = hil->bind(fast, 1);
                        if (error)
                            return (error);
                        error = ckt->num2node(inst->URCgndNode)->bind(fast, 2);
                        if (error)
                            return (error);
                        data.v.rValue = prop;
                        error = fast->setParam("area", &data);
                        if (error)
                            return (error);
                    }
                    else {
                        // use simple capacitor
                        sprintf(namebf, "chi%d", i);
                        nameelt = new char[strlen(namebf)+1];
                        strcpy(nameelt, namebf);
                        error = ckt->newUid(&eltUid, inst->GENname, nameelt,
                            UID_INSTANCE);
                        if (error)
                            return (error);
                        error = ckt->newInst(modfast, &fast, eltUid);
                        if (error)
                            return (error);
                        error = hil->bind(fast, 1);
                        if (error)
                            return (error);
                        error = ckt->num2node(inst->URCgndNode)->bind(fast, 2);
                        if (error)
                            return (error);
                        data.v.rValue = c;
                        error = fast->setParam("capacitance", &data);
                        if (error)
                            return (error);
                    }
                }
                prop *= p;
                lowl = lowr;
                hir = hil;
            }
        }
    }
    return (OK);
}


int
URCdev::unsetup(sGENmodel *genmod, sCKT *ckt)
{
    // Delete models, devices, and intermediate nodes;

    sURCmodel *model = static_cast<sURCmodel*>(genmod);
    for ( ; model; model = model->next()) {
        sURCinstance *inst;
        for (inst = model->inst(); inst; inst = inst->next()) {
            inst->URCsetupDone = 0;

            int error;
            IFuid varUid;
            if (model->URCisPerLGiven)
                // Diodes
                error = ckt->newUid(&varUid, inst->GENname, "diodemod",
                    UID_MODEL);
            else
                // Capacitors
                error = ckt->newUid(&varUid, inst->GENname, "capmod",
                    UID_MODEL);

            if (error && error != E_EXISTS)
                return (error);

            sGENmodel *modfast = 0;
            int type = -1;
            error = ckt->findModl(&type, &modfast, varUid);
            if (error)
                return (error);

            ckt->delModl(0, 0, modfast);    // Does the elements too

            // Resistors
            error = ckt->newUid(&varUid, inst->GENname, "resmod",
                UID_MODEL);
            if (error && error != E_EXISTS)
                return (error);

            modfast = 0;
            type = -1;
            error = ckt->findModl(&type, &modfast, varUid);
            if (error)
                return (error);

            ckt->delModl(0, 0, modfast);
        }
    }
    return (OK);
}


// SRW - reset the matrix element pointers.
//
int
URCdev::resetup(sGENmodel*, sCKT*)
{
    // Nothing to do here, the devices will be updated with their
    // respective types.
    return (OK);
}


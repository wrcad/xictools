
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
 * WRspice Circuit Simulation and Analysis Tool:  Device Library          *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3f2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 Thomas L. Quarles
         1989 Takayasu Sakurai
         1993 Stephen R. Whiteley
****************************************************************************/

#include "mosdefs.h"
#include "errors.h"

// assuming silicon - make definition for epsilon of silicon
#define EPSSIL (11.7 * 8.854214871e-12)

// some constants to avoid slow divides
#define P66 .66666666666667
#define P33 .33333333333333
#define P1X3 1.3333333333333

// (cm**2/m**2)
#define CM2PM2 1e4

// (m**2/cm**2)
#define M2PCM2 1e-4


static void mos2_vsat(double*, double*);


//     This code evaluates the drain current and its 
//     derivatives using the shichman-hodges model and the 
//     charges associated with the gate, channel and bulk for 
//     mosfets
//
double
mosstuff::eq1(sMOSmodel *model, sMOSinstance *inst)
{
    double vbx = ((inst->MOSmode == 1) ? ms_vbs : ms_vbd);
    double vgx = ((inst->MOSmode == 1) ? ms_vgs : ms_vgd);

    double sarg;
    if (vbx <= 0)
        sarg = sqrt(inst->MOStPhi - vbx);
    else {
        sarg = sqrt(inst->MOStPhi);
        sarg = sarg - vbx/(sarg+sarg);
        sarg = SPMAX(0,sarg);
    }
    if (model->MOStype > 0)
        ms_von = inst->MOStVbi + model->MOSgamma*sarg;
    else
        ms_von = -inst->MOStVbi + model->MOSgamma*sarg;

    double vgst = vgx - ms_von;
    ms_vdsat = SPMAX(vgst,0);

    double cdrain;
    if (vgst <= 0) {
        // cutoff region
        cdrain = 0;
        inst->MOSgm = 0;
        inst->MOSgds = 0;
        inst->MOSgmbs = 0;
    }
    else {
        double vds;
        if (inst->MOSmode > 0)
            vds = ms_vds;
        else
            vds = -ms_vds;

        double arg;
        if (sarg <= 0)
            arg = 0;
        else
            arg = model->MOSgamma/(sarg+sarg);
        double betap = inst->MOSbeta * (1 + model->MOSlambda*vds);

        if (vgst <= vds) {
            // saturation region

            inst->MOSgm   = betap*vgst;
            vgst *= vgst*.5;
            cdrain        = betap*vgst;
            inst->MOSgds  = model->MOSlambda*inst->MOSbeta*vgst;
            inst->MOSgmbs = inst->MOSgm*arg;
        }
        else {
            // linear region

            inst->MOSgm   = betap*vds;
            cdrain        = inst->MOSgm*(vgst - .5*vds);
            inst->MOSgds  = betap*(vgst - vds) +
                        model->MOSlambda*inst->MOSbeta*vds*(vgst - .5*vds);
            inst->MOSgmbs = inst->MOSgm*arg;
        }
    }
    return (cdrain);
}


//     this routine evaluates the drain current, its derivatives and
//     the charges associated with the gate, channel and bulk
//     for mosfets (level 2 model)
//
double
mosstuff::eq2(sMOSmodel *model, sMOSinstance *inst)
{
    // 'local' variables - these switch d & s around appropriately
    // so that we don't have to worry about vds < 0
    //
    double lvbs;
    double lvds;
    double lvgs;
    if (inst->MOSmode > 0) {
        lvbs = ms_vbs;
        lvds = ms_vds;
        lvgs = ms_vgs;
    }
    else {
        lvbs = ms_vbd;
        lvds = -ms_vds;
        lvgs = ms_vgd;
    }
    double xlamda = model->MOSlambda;
    double phiMinVbs = inst->MOStPhi - lvbs;
    double oneoverl = 1.0/inst->MOSeffectiveLength;

    //
    //  compute some useful quantities
    //

    double sarg;
    double dsrgdb;
    double d2sdb2;
    double sphi  = sqrt(inst->MOStPhi);
    double sphi3 = inst->MOStPhi*sphi;
    if (lvbs <= 0.0) {
        sarg   = sqrt(phiMinVbs);
        dsrgdb = -0.5/sarg;
        d2sdb2 = 0.5*dsrgdb/phiMinVbs;
    }
    else {
        sarg   = sphi/(1.0 + 0.5*lvbs/inst->MOStPhi);
        double tmp = sarg/sphi3;
        dsrgdb = -0.5*sarg*tmp;
        d2sdb2 = -dsrgdb*tmp;
    }

    double barg;
    double dbrgdb;
    double d2bdb2;
    if ((lvds - lvbs) >= 0) {
        barg   = sqrt(phiMinVbs + lvds);
        dbrgdb = -0.5/barg;
        d2bdb2 = 0.5*dbrgdb/(phiMinVbs + lvds);
    }
    else {
        barg   = sphi/(1.0 + 0.5*(lvbs - lvds)/inst->MOStPhi);
        double tmp = barg/sphi3;
        dbrgdb = -0.5*barg*tmp;
        d2bdb2 = -dbrgdb*tmp;
    }
    int fssandcap = (model->MOSfastSurfaceStateDensity != 0.0 &&
        inst->MOSoxideCap != 0.0);

    //
    //  calculate threshold voltage (von)
    //     narrow-channel effect
    //
    double factor   = 0.125*model->MOSnarrowFactor*2.0*M_PI*EPSSIL/
                    inst->MOSoxideCap*inst->MOSeffectiveLength;
    double eta      = 1.0 + factor;
    double oneovere = 1.0/eta;
    double vbin     = inst->MOStVbi*model->MOStype + factor*phiMinVbs;

    double gamasd   = model->MOSgamma;
    double gammad   = model->MOSgamma;
    double dgddvb   = 0.0;
    double dgdvds   = 0.0;
    double dgddb2   = 0.0;
    if ((model->MOSgamma > 0.0) || 
            (model->MOSsubstrateDoping > 0.0)) {

        //
        //     short-channel effect with vds != 0.0
        //

        if (model->MOSjunctionDepth > 0) {

            double tbxwd = model->MOSxd*dbrgdb;
            double tbxws = model->MOSxd*dsrgdb;

            double tmp = 2.0/model->MOSjunctionDepth;
            double targxs = 1.0 + tmp*model->MOSxd*sarg;
            double targxd = 1.0 + tmp*model->MOSxd*barg;
            double targs = sqrt(targxs);
            double targd = sqrt(targxd);
            tmp = .5*model->MOSjunctionDepth*oneoverl;
            gamasd *= (1.0 - tmp*(targs + targd - 2.0));

            targs = oneoverl/targs;
            targd = oneoverl/targd;
            dgddvb = -.5*model->MOSgamma*(tbxws*targs + tbxwd*targd);
            dgdvds = model->MOSgamma*0.5*tbxwd*targd;

            tmp = -model->MOSxd*(d2sdb2 + dsrgdb*dsrgdb*
                        model->MOSxd/(model->MOSjunctionDepth*targxs))*targs;
            double tmp1 = -model->MOSxd*(d2bdb2 + dbrgdb*dbrgdb*
                        model->MOSxd/(model->MOSjunctionDepth*targxd))*targd;
            dgddb2 = -0.5*model->MOSgamma*(tmp + tmp1);
        }
    }

    ms_von   = vbin + gamasd*sarg;
    double vth   = ms_von;
    ms_vdsat = 0.0;

    double argg;
    double xn = 1.0;
    if (fssandcap) {
        double tmp  = CHARGE*model->MOSfastSurfaceStateDensity*CM2PM2;
        double tmp1 = -(gamasd*dsrgdb + dgddvb*sarg) + factor;
        xn  +=
            tmp/inst->MOSoxideCap*inst->MOSw*inst->MOSeffectiveLength + tmp1;
        tmp  = ms_vt*xn;
        ms_von += tmp;
        argg = 1.0/tmp;
    }
    else {
        argg = 0; // not used

// NGspice
        if (lvgs <= vbin) {
/*
        if (lvgs <= ms_von) {
*/
            //
            //  cutoff region
            //
            inst->MOSgds = 0.0;
        }
    }

    double cdrain = 0;
// NGspice
    if (fssandcap || (lvgs > vbin)) {
/*
    if (fssandcap || (lvgs > ms_von)) {
*/
        double vgst = lvgs - ms_von;
        //
        //  compute some more useful quantities
        //
        double sarg3  = sarg*sarg*sarg;
        double sbiarg = sqrt(inst->MOStBulkPot);
        gammad = gamasd;
        double dgdvbs = dgddvb;
        double body   = barg*barg*barg - sarg3;
        double gdbdv  = 2.0*gammad*(barg*barg*dbrgdb - sarg*sarg*dsrgdb);
        double dodvbs = -factor + dgdvbs*sarg + gammad*dsrgdb;

        double dxndvb;
        double dxndvd;
        double dodvds;
        if (fssandcap) {
            dxndvb = 2.0*dgdvbs*dsrgdb + gammad*d2sdb2 + dgddb2*sarg;
            dodvbs = dodvbs + ms_vt*dxndvb;
            dxndvd = dgdvds*dsrgdb;
            dodvds = dgdvds*sarg + ms_vt*dxndvd;
        }
        else {
            // not used
            dxndvb = 0;
            dxndvd = 0;
            dodvds = 0;
        }

        //
        //  evaluate effective mobility and its derivatives
        //
        double ufact;
        double ueff;
        double dudvgs;
        double dudvds;
        double dudvbs;
        if (inst->MOSoxideCap <= 0.0) {
            ufact  = 1.0;
            ueff   = model->MOSsurfaceMobility * M2PCM2;
            dudvgs = 0.0;
            dudvds = 0.0;
            dudvbs = 0.0;
        }
        else {
            double tmp = model->MOScritField * 100 /* cm/m */ * EPSSIL/
                model->MOSoxideCapFactor;
            if (vgst <= tmp) {
                ufact  = 1.0;
                ueff   = model->MOSsurfaceMobility * M2PCM2;
                dudvgs = 0.0;
                dudvds = 0.0;
                dudvbs = 0.0;
            }
            else {
                ufact  = exp(model->MOScritFieldExp*log(tmp/vgst));
                ueff   = model->MOSsurfaceMobility * M2PCM2 *ufact;
                dudvgs = -ufact*model->MOScritFieldExp/vgst;
                dudvds = 0.0;
                dudvbs = model->MOScritFieldExp*ufact*dodvbs/vgst;
            }
        }

        //
        //     evaluate saturation voltage and its derivatives according to
        //     grove-frohman equation
        //
        gammad = gamasd*oneovere;
        dgdvbs = dgddvb;
        double vgsx = lvgs;
        if (fssandcap)
            vgsx = SPMAX(lvgs,ms_von);
        double dsdvgs;
        double dsdvbs;
        if (gammad > 0) {
            double tmp = gammad*gammad;
            double tmp1 = (vgsx - vbin)*oneovere + phiMinVbs;
            if (tmp1 <= 0.0) {
                ms_vdsat = 0.0;
                dsdvgs = 0.0;
                dsdvbs = 0.0;
            }
            else {
                double arg = sqrt(1.0 + 4.0*tmp1/tmp);
                ms_vdsat = (vgsx - vbin)*oneovere + .5*tmp*(1.0 - arg);
                ms_vdsat = SPMAX(ms_vdsat,0.0);  
                dsdvgs = (1.0 - 1.0/arg)*oneovere;
                dsdvbs = (gammad*(1.0 - arg) + 2.0*tmp1/(gammad*arg))*oneovere
                    * dgdvbs + 1.0/arg + factor*dsdvgs;
            }
        }
        else {
            ms_vdsat = (vgsx - vbin)*oneovere;
            ms_vdsat = SPMAX(ms_vdsat,0.0);
            dsdvgs = 1.0;
            dsdvbs = 0.0;
        }
        if (model->MOSmaxDriftVel > 0) {

            double tmp = model->MOSmaxDriftVel*inst->MOSeffectiveLength/ueff;
            double tmp1 = (vgsx - vbin)*oneovere + phiMinVbs;
            double param[5];
            param[0] = P1X3*gammad;
            param[1] = -2.0*(tmp1 + tmp);
            param[2] = -2.0*gammad*tmp;
            param[3] = 2.0*tmp1*(phiMinVbs + tmp) - phiMinVbs*phiMinVbs -
                        P1X3*gammad*sarg3;
            param[4] = phiMinVbs;

            mos2_vsat(&ms_vdsat, param);
        }
        //
        //  evaluate effective channel length and its derivatives
        //
        double dldvgs = 0.0;
        double dldvds = 0.0;
        double dldvbs = 0.0;
        double bsarg;
        double bodys;
        double gdbdvs;
        double dbsrdb;
        if (lvds == 0.0) {
            // not used
            bsarg = 0;
            bodys = 0;
            gdbdvs = 0;
        }
        else {
            gammad = gamasd;
            if ((lvbs - ms_vdsat) <= 0) {
                bsarg = sqrt(ms_vdsat + phiMinVbs);
                dbsrdb = -0.5/bsarg;
            }
            else {
                bsarg = sphi/(1.0 + 0.5*(lvbs - ms_vdsat)/inst->MOStPhi);
                dbsrdb = -0.5*bsarg*bsarg/sphi3;
            }
            bodys = bsarg*bsarg*bsarg - sarg3;
            gdbdvs = 2.0*gammad*(bsarg*bsarg*dbsrdb - sarg*sarg*dsrgdb);

            if (model->MOSmaxDriftVel <= 0) {
                if (model->MOSsubstrateDoping != 0.0 && xlamda <= 0.0) {
                    double tmp = .25*(lvds - ms_vdsat);
                    double tmp1 = sqrt(1.0 + tmp*tmp);
                    tmp    = sqrt(tmp + tmp1);
                    xlamda = model->MOSxd*tmp*oneoverl/lvds;
                    tmp    = lvds*xlamda/(8.0*tmp1);
                    dldvgs = tmp*dsdvgs;
                    dldvds = -xlamda + tmp;
                    dldvbs = tmp*dsdvbs;
                }
            }
            else {
                double txdv   = model->MOSxd/sqrt(model->MOSchannelCharge);
                double txlv   = model->MOSmaxDriftVel*txdv/(2.0*ueff);
                double tqdsat = -1.0 + gammad*dbsrdb;
                double tmp    = model->MOSmaxDriftVel*inst->MOSeffectiveLength;
                double tmp1   = (vgsx - vbin)*oneovere - ms_vdsat -
                    gammad*bsarg;
                tmp1   = 1.0/(tmp*tqdsat - ueff*tmp1);
                dsdvgs = -(tmp - ueff*ms_vdsat)*oneovere*tmp1;
                dsdvbs = -(-tmp*(1.0 + tqdsat - factor*oneovere) +
                        ueff*(gdbdvs - P66*dgdvbs*bodys)*oneovere)*tmp1;

                if (model->MOSsubstrateDoping != 0.0 && xlamda <= 0.0) {
                    tmp    = lvds - ms_vdsat;
                    tmp    = SPMAX(tmp, 0.0);
                    tmp1   = sqrt(txlv*txlv + tmp);
                    tmp    = txdv*oneoverl/(2.0*tmp1);
                    xlamda = txdv*(tmp1 - txlv)*oneoverl/lvds;
                    dldvgs = tmp*dsdvgs;
                    dldvds = -xlamda + tmp;
                    dldvbs = tmp*dsdvbs;
                }
            }
        }
        //
        //     limit channel shortening at punch-through
        //
        double xwb    = model->MOSxd*sbiarg;
        double clfact = 1.0 - xlamda*lvds;

        dldvds = -xlamda - dldvds;
        double tmp1 = inst->MOSeffectiveLength*clfact;
        double tmp  = xlamda*lvds*inst->MOSeffectiveLength;
        if (model->MOSsubstrateDoping == 0.0) xwb = 0.25e-6;
        if (tmp1 < xwb) {
            tmp1 = xwb/(1.0 + (tmp - inst->MOSeffectiveLength - xwb)/xwb);
            clfact = tmp1*oneoverl;

            double dfact = tmp1/xwb;
            dfact  *= dfact;
            dldvgs *= dfact;
            dldvds *= dfact;
            dldvbs *= dfact;
        }
        //
        //  evaluate effective beta (effective kp)
        //
        clfact = 1.0/clfact;
        double beta1 = inst->MOSbeta*ufact*clfact;
        ufact  = 1.0/ufact;
        //
        //  test for mode of operation and branch appropriately
        //
        gammad = gamasd;
        dgdvbs = dgddvb;

        if (lvds > 1.0e-10) {
            if (lvgs > ms_von) {
                if (lvds <= ms_vdsat) {
                    //
                    //  linear region
                    //
                    cdrain = beta1*((lvgs - vbin - .5*eta*lvds)*lvds -
                        P66*gammad*body);

                    tmp = cdrain*(dudvgs*ufact - dldvgs*clfact);
                    inst->MOSgm = tmp + beta1*lvds;

                    tmp = cdrain*(dudvds*ufact - dldvds*clfact);
                    inst->MOSgds = tmp + beta1*(lvgs - vbin - eta*lvds -
                        gammad*barg - P66*dgdvds*body);

                    tmp = cdrain*(dudvbs*ufact - dldvbs*clfact);
                    inst->MOSgmbs = tmp - beta1*(gdbdv + P66*dgdvbs*body -
                        factor*lvds);
                }
                else {
                    // 
                    //  saturation region
                    //
                    cdrain = beta1*((lvgs - vbin - eta*
                        .5*ms_vdsat)*ms_vdsat - P66*gammad*bodys);

                    tmp = cdrain*(dudvgs*ufact - dldvgs*clfact);
                    inst->MOSgm = tmp + beta1*ms_vdsat + beta1*(lvgs -
                        vbin - eta*ms_vdsat - gammad*bsarg)*dsdvgs;
                    inst->MOSgds =
                        -cdrain*dldvds*clfact - P66*beta1*dgdvds*bodys;
                    tmp = cdrain*(dudvbs*ufact - dldvbs*clfact);
                    inst->MOSgmbs = tmp - beta1*(gdbdvs + P66*dgdvbs*bodys -
                        factor*ms_vdsat) + beta1*
                        (lvgs - vbin - eta*ms_vdsat - gammad*bsarg)*dsdvbs;
                }
            }
            else {
                //
                //  subthreshold region
                //
                if (ms_vdsat <= 0) {
                    inst->MOSgds = 0.0;
                    if (lvgs > vth)
                        return (cdrain);
                    inst->MOSgm = 0.0;
                    inst->MOSgmbs = 0.0;
                    return (0.0);
                } 
                double vdson = SPMIN(ms_vdsat,lvds);
                if (lvds > ms_vdsat) {
                    barg = bsarg;
                    dbrgdb = dbsrdb;
                    body = bodys;
                    gdbdv = gdbdvs;
                }
                double cdson = beta1*((ms_von - vbin - .5*eta*vdson)*vdson
                    - P66*gammad*body);
                double didvds = beta1*(ms_von - vbin - eta*vdson -
                    gammad*barg);
                double gdson = -cdson*dldvds*clfact - P66*beta1*dgdvds*body;
                if (lvds < ms_vdsat)
                    gdson += didvds;
                double gbson = -cdson*dldvbs*clfact + beta1*
                    (dodvbs*vdson + factor*vdson - P66*dgdvbs*body - gdbdv);
                if (lvds > ms_vdsat)
                    gbson += didvds*dsdvbs;
                tmp = exp(argg*(lvgs - ms_von));
                cdrain = cdson*tmp;
                double gmw = cdrain*argg;
                inst->MOSgm = gmw;
                if (lvds > ms_vdsat) inst->MOSgm = gmw + didvds*dsdvgs*tmp;
                tmp1 = gmw*(lvgs - ms_von)/xn;
                inst->MOSgds = gdson*tmp - inst->MOSgm*dodvds - tmp1*dxndvd;
                inst->MOSgmbs = gbson*tmp - inst->MOSgm*dodvbs - tmp1*dxndvb;
            }
            return (cdrain);
        }
        else {
            if (lvgs <= ms_von) {
                if (!fssandcap)
                    inst->MOSgds = 0.0;
                else
                    inst->MOSgds = beta1*(ms_von - vbin - gammad*sarg)*
                        exp(argg*(lvgs-ms_von));
            }
            else
                inst->MOSgds = beta1*(lvgs-vbin-gammad*sarg);
        }
    }
    //
    //  finish special cases
    //
    inst->MOSgm = 0.0;
    inst->MOSgmbs = 0.0;
    return (0.0);
}


//     Evaluate saturation voltage and its derivatives 
//     according to baum's theory of scattering velocity 
//     saturation
//
static void
mos2_vsat(double *vsat, double *baum)
{
    double a1 = baum[0];
    double b1 = baum[1];
    double c1 = baum[2];
    double d1 = baum[3];

    double tmp  = a1*c1 - 4.0*d1;
    double tmp1 = -P33*b1*b1 + tmp;
    double s    = -2.0*b1*b1*b1/27.0 + P33*b1*tmp;
    tmp  = -d1*(a1*a1 - 4.0*b1) - c1*c1;
    s   += tmp;
    double s2   = s*s;

    tmp  = P33*tmp1;
    tmp *= tmp*tmp;
    double p    = .25*s2 + tmp;
    double p0   = FABS(p);
    double p2   = sqrt(p0);

    double y3;
    if (p < 0) {
        tmp  = sqrt(.25*s2 + p0);
        tmp  = exp(P33*log(tmp));
        y3   = 2.0*tmp*cos(P33*atan(-2.0*p2/s)) + P33*b1;
    }
    else {
        tmp  = (-.5*s + p2);
        tmp  = exp(P33*log(FABS(tmp)));
        tmp1 = (-.5*s - p2);
        tmp1 = exp(P33*log(FABS(tmp1)));
        y3   = tmp + tmp1 + P33*b1;
    }

    int jknt = 0;
    double a3  = sqrt(.25*a1*a1 - b1 + y3);
    double b3  = sqrt(.25*y3*y3 - d1);
    double b00 = .5*y3 + b3;
    double b01 = .5*y3 - b3;
    double a00 = .5*a1 + a3;
    double a01 = .5*a1 - a3;
    double a02 = .25*a00*a00;
    double a03 = .25*a01*a01;
    double a04 = -.5*a00;
    double a05 = -.5*a01;
    double az[4];
    az[0] = a02 - b00;
    az[1] = a03 - b00;
    az[2] = a02 - b01;
    az[3] = a03 - b01;
    double bz[4];
    bz[0] = a04;
    bz[1] = a05;
    bz[2] = a04;
    bz[3] = a05;

    for (int i = 0; i < 4; i++) {
        tmp = az[i];
        if (tmp >= 0) {
            tmp = sqrt(tmp);
            double ptmp = bz[i] + tmp;
            if (ptmp > 0) {
                double poly4 = d1 + ptmp*(c1 + ptmp*(b1 + ptmp*(a1 + ptmp)));
                if (FABS(poly4) <= 1.0e-6) {
                    if (!jknt || ptmp <= tmp1)
                        tmp1 = ptmp;
                    jknt++;
                }
            }
            ptmp = bz[i] - tmp;
            if (ptmp > 0) {
                double poly4 = d1 + ptmp*(c1 + ptmp*(b1 + ptmp*(a1 + ptmp)));
                if (FABS(poly4) <= 1.0e-6) {
                    if (!jknt || ptmp <= tmp1)
                        tmp1 = ptmp;
                    jknt++;
                }
            }
        }
    }
    if (jknt > 0)
        *vsat = tmp1*tmp1 - baum[4];
}


//     This routine evaluates the drain current, its derivatives and
//     the charges associated with the gate, channel and bulk
//     for mosfets based on semi-empirical equations (level 3 model)
//
double
mosstuff::eq3(sMOSmodel *model, sMOSinstance *inst)
{
    // 'local' variables - these switch d & s around appropriately
    // so that we don't have to worry about vds < 0
    //
    double lvbs;
    double lvds;
    double lvgs;
    if (inst->MOSmode > 0) {
        lvbs = ms_vbs;
        lvds = ms_vds;
        lvgs = ms_vgs;
    }
    else {
        lvbs = ms_vbd;
        lvds = -ms_vds;
        lvgs = ms_vgd;
    }
    //
    //     bypasses the computation of charges
    //

    //
    //     reference cdrain equations to source and
    //     charge equations to bulk
    //
    ms_vdsat = 0.0;
    // 1/effective length
    double oneoverxl = 1.0/inst->MOSeffectiveLength;

    // eta from model after length factor
    double eta = model->MOSeta * 8.15e-22/(model->MOSoxideCapFactor*
            inst->MOSeffectiveLength*
            inst->MOSeffectiveLength*inst->MOSeffectiveLength);
    //
    //.....square root term
    //
    double phibs;       // phi - vbs
    double sqphbs;      // square root of phibs
    double dsqdvb;
    if (lvbs <=  0.0 ) {
        phibs  =  inst->MOStPhi - lvbs;
        sqphbs =  sqrt(phibs);
        dsqdvb =  -0.5/sqphbs;
    }
    else {
        double sqphis = sqrt(inst->MOStPhi);
        double sqphs3 = inst->MOStPhi*sqphis;
        sqphbs = sqphis/(1.0 + lvbs/(inst->MOStPhi + inst->MOStPhi));
        phibs  = sqphbs*sqphbs;
        dsqdvb = -phibs/(sqphs3+sqphs3);
    }
    //
    //.....short channel effect factor
    //
    double fshort;
    double dfsdvb;
    if (model->MOSjunctionDepth != 0.0 && model->MOSxd != 0.0) {
        double coeff0 = 0.0631353e0;
        double coeff1 = 0.8013292e0;
        double coeff2 = -0.01110777e0;

        double wps = model->MOSxd*sqphbs;
        double oneoverxj = 1.0/model->MOSjunctionDepth;
        double xjonxl = model->MOSjunctionDepth*oneoverxl;
        double djonxj = model->MOSlatDiff*oneoverxj;
        double wponxj = wps*oneoverxj;
        double wconxj = coeff0 + coeff1*wponxj + coeff2*wponxj*wponxj;
        double arga   = wconxj + djonxj;
        double argc   = wponxj/(1.0 + wponxj);
        double argb   = sqrt(1.0 - argc*argc);
        fshort = 1.0 - xjonxl*(arga*argb - djonxj);

        double dwpdvb = model->MOSxd*dsqdvb;
        double dadvb  = (coeff1 + coeff2*(wponxj+wponxj))*dwpdvb*oneoverxj;
        double dbdvb  = -argc*argc*(1.0 - argc)*dwpdvb/(argb*wps);
        dfsdvb = -xjonxl*(dadvb*argb + arga*dbdvb);
    }
    else {
        fshort = 1.0;
        dfsdvb = 0.0;
    }
    //
    //.....body effect
    //
    double gammas = model->MOSgamma*fshort;
    double fbodys = 0.5*gammas/(sqphbs+sqphbs);
    double fbody  = fbodys+model->MOSnarrowFactor/inst->MOSw;
    double onfbdy = 1.0/(1.0 + fbody);
    double dfbdvb = -fbodys*dsqdvb/sqphbs + fbodys*dfsdvb/fshort;
    double qbonco = gammas*sqphbs + model->MOSnarrowFactor*phibs/inst->MOSw;
    double dqbdvb = gammas*dsqdvb + model->MOSgamma*dfsdvb*sqphbs -
        model->MOSnarrowFactor/inst->MOSw;
    //
    //.....threshold voltage
    //
    double vth    = qbonco + inst->MOStVbi*model->MOStype - eta*lvds;
    double dvtdvd = -eta;
    double dvtdvb = dqbdvb;
    //
    //.....joint weak inversion and strong inversion
    //
    ms_von = vth;
    double cdrain;
    double xn = 1.0;
    double dxndvb;
    double dvodvd;
    double dvodvb;
    if (model->MOSfastSurfaceStateDensity != 0.0) {
        double csonco = CHARGE*model->MOSfastSurfaceStateDensity * CM2PM2 *
            inst->MOSeffectiveLength*inst->MOSw/inst->MOSoxideCap;
        double cdonco = qbonco/(phibs + phibs);
        xn    += csonco + cdonco;
        ms_von += ms_vt*xn;
        dxndvb = dqbdvb/(phibs + phibs) - qbonco*dsqdvb/(phibs*sqphbs);
        dvodvd = dvtdvd;
        dvodvb = dvtdvb + ms_vt*dxndvb;
    }
    else {
        //
        //.....cutoff region
        //
        dxndvb = 0; /* not used */
        dvodvd = 0; /* not used */
        dvodvb = 0; /* not used */
        if (lvgs <= ms_von ) {
            cdrain = 0.0;
            inst->MOSgm = 0.0;
            inst->MOSgds = 0.0;
            inst->MOSgmbs = 0.0;
            return (cdrain);
        }
    }
    //
    //.....device is on
    //
    double vgsx = SPMAX(lvgs,ms_von);
    //
    //.....mobility modulation by gate voltage
    //
    double onfg   = 1.0 + model->MOStheta*(vgsx - vth);
    double fgate  = 1.0/onfg;
    double us     = inst->MOStSurfMob * M2PCM2 * fgate;
    double dfgdvg = -model->MOStheta*fgate*fgate;
    double dfgdvd = -dfgdvg*dvtdvd;
    double dfgdvb = -dfgdvg*dvtdvb;
    //
    //.....saturation voltage
    //
    double dvsdvg;
    double dvsdvd;
    double dvsdvb;
    double onvdsc;
    ms_vdsat = (vgsx - vth)*onfbdy;
    if ( model->MOSmaxDriftVel <= 0.0 ) {
        dvsdvg = onfbdy;
        dvsdvd = -dvsdvg*dvtdvd;
        dvsdvb = -dvsdvg*dvtdvb-ms_vdsat*dfbdvb*onfbdy;
        onvdsc = 0; // not used
    }
    else {
        double vdsc   = inst->MOSeffectiveLength*model->MOSmaxDriftVel/us;
        onvdsc = 1.0/vdsc;
        double arga   = (vgsx - vth)*onfbdy;
        double argb   = sqrt(arga*arga + vdsc*vdsc);
        ms_vdsat  = arga+vdsc - argb;
        double dvsdga = (1.0 - arga/argb)*onfbdy;
        dvsdvg = dvsdga - (1.0 - vdsc/argb)*vdsc*dfgdvg*onfg;
        dvsdvd = -dvsdvg*dvtdvd;
        dvsdvb = -dvsdvg*dvtdvb - arga*dvsdga*dfbdvb;
    }
    //
    //.....current factors in linear region
    //
    double beta = inst->MOSbeta;
    double vdsx = SPMIN(lvds,ms_vdsat);
    if (vdsx == 0.0) {
        beta *= fgate;
        cdrain = 0.0;
        inst->MOSgm = 0.0;
        inst->MOSgds = beta*(vgsx-vth);
        inst->MOSgmbs = 0.0;
        if ( (model->MOSfastSurfaceStateDensity != 0.0) && 
                (lvgs < ms_von) ) {
            inst->MOSgds *= exp( (lvgs - ms_von)/(ms_vt*xn) );
        }
        return (cdrain);
    }

    double cdo = vgsx - vth - 0.5*(1.0 + fbody)*vdsx;
    double dcodvb = -dvtdvb - 0.5*dfbdvb*vdsx;
    // 
    //.....normalized drain current
    //
    double cdnorm = cdo*vdsx;
    inst->MOSgm = vdsx;
// NGspice
    if ((inst->MOSmode*ms_vds) > ms_vdsat)
        inst->MOSgds = -dvtdvd*vdsx;
    else
        inst->MOSgds = vgsx - vth - (1.0 + fbody + dvtdvd)*vdsx;
/*
    inst->MOSgds = vgsx - vth - (1.0 + fbody + dvtdvd)*vdsx;
*/
    inst->MOSgmbs = dcodvb*vdsx;
    // 
    //.....drain current without velocity saturation effect
    //
    double cd1 = beta*cdnorm;
    beta *= fgate;
    cdrain = beta*cdnorm;
    inst->MOSgm = beta*inst->MOSgm + dfgdvg*cd1;
    inst->MOSgds = beta*inst->MOSgds + dfgdvd*cd1;
// NGspice
    inst->MOSgmbs = beta*inst->MOSgmbs + dfgdvb*cd1;
/*
    inst->MOSgmbs = beta*inst->MOSgmbs;
*/
    //
    //.....velocity saturation factor
    //
    double fdrain;
    double dfddvg;
    double dfddvb;
    double dfddvd;
// NGspice
    if (model->MOSmaxDriftVel > 0.0) {
        fdrain = 1.0/(1.0+vdsx*onvdsc);
        double fd2 = fdrain*fdrain;
        double arga = fd2*vdsx*onvdsc*onfg;
        dfddvg = -dfgdvg*arga;
        if (inst->MOSmode*ms_vds > ms_vdsat)
            dfddvd = -dfgdvd*arga;
        else
            dfddvd = -dfgdvd*arga - fd2*onvdsc;
        dfddvb = -dfgdvb*arga;
/*
    if (model->MOSmaxDriftVel != 0.0) {
        fdrain = 1.0/(1.0+vdsx*onvdsc);
        double fd2 = fdrain*fdrain;
        double arga = fd2*vdsx*onvdsc*onfg;
        dfddvg = -dfgdvg*arga;
        dfddvd = -dfgdvd*arga - fd2*onvdsc;
        dfddvb = -dfgdvb*arga;
*/
        //
        //.....drain current
        //
        inst->MOSgm = fdrain*inst->MOSgm + dfddvg*cdrain;
        inst->MOSgds = fdrain*inst->MOSgds + dfddvd*cdrain;
        inst->MOSgmbs = fdrain*inst->MOSgmbs + dfddvb*cdrain;
        cdrain = fdrain*cdrain;
        beta *= fdrain;
    }
    else {
        // not used
        fdrain = 0;
        dfddvg = 0;
        dfddvd = 0;
        dfddvb = 0;
    }
    //
    //.....channel length modulation
    //
    double gds0 = 0;
//    if (lvds <= ms_vdsat)
//        goto invers;

    if (lvds > ms_vdsat

        && !(model->MOSmaxDriftVel != 0.0 && model->MOSalpha == 0)) {
        // avoid goto below

        double delxl;
        double dldvd;
        double ddldvg;
        double ddldvd;
        double ddldvb;
        if (model->MOSmaxDriftVel == 0.0) {
            delxl =
                sqrt(model->MOSkappa*(lvds - ms_vdsat)*model->MOSalpha);
            dldvd  = 0.5*delxl/(lvds - ms_vdsat);
            ddldvg = 0.0;
            ddldvd = -dldvd;
            ddldvb = 0.0;
        }
        else {

//            if (model->MOSalpha == 0.0)
//                goto invers;

            double cdsat  = cdrain;
            double gdsat  = cdsat*(1.0 - fdrain)*onvdsc;
            gdsat  = SPMAX(1.0e-12, gdsat);
            double gdoncd = gdsat/cdsat;
            double gdonfd = gdsat/(1.0 - fdrain);
            double gdonfg = gdsat*onfg;
            double dgdvg  = gdoncd*inst->MOSgm - gdonfd*dfddvg+gdonfg*dfgdvg;
            double dgdvd  = gdoncd*inst->MOSgds - gdonfd*dfddvd+gdonfg*dfgdvd;
            double dgdvb  = gdoncd*inst->MOSgmbs - gdonfd*dfddvb+gdonfg*dfgdvb;

            double emax = model->MOSkappa * cdsat*oneoverxl/gdsat;

            double emoncd = emax/cdsat;
            double emongd = emax/gdsat;
            double demdvg = emoncd*inst->MOSgm - emongd*dgdvg;
            double demdvd = emoncd*inst->MOSgds - emongd*dgdvd;
            double demdvb = emoncd*inst->MOSgmbs - emongd*dgdvb;

            double arga   = 0.5*emax*model->MOSalpha;
            double argc   = model->MOSkappa*model->MOSalpha;
            double argb   = sqrt(arga*arga + argc*(lvds - ms_vdsat));

            delxl  = argb - arga;
            dldvd  = argc/(argb+argb);

            double dldem  = 0.5*(arga/argb - 1.0)*model->MOSalpha;

            ddldvg = dldem*demdvg;
            ddldvd = dldem*demdvd-dldvd;
            ddldvb = dldem*demdvb;
        }
        //
        //.....punch through approximation
        //
        if (delxl > (0.5*inst->MOSeffectiveLength)) {
            delxl  = inst->MOSeffectiveLength -
                    (inst->MOSeffectiveLength*inst->MOSeffectiveLength/
                    (4.0*delxl));
            double arga = 4.0*(inst->MOSeffectiveLength - delxl)*
                    (inst->MOSeffectiveLength - delxl)/
                    (inst->MOSeffectiveLength*inst->MOSeffectiveLength);
            ddldvg = ddldvg*arga;
            ddldvd = ddldvd*arga;
            ddldvb = ddldvb*arga;
            dldvd  = dldvd*arga;
        }
        //
        //.....saturation region
        //
        double dlonxl = delxl*oneoverxl;
        double xlfact = 1.0/(1.0 - dlonxl);

// NGspice
        cd1 = cdrain;
        cdrain = cdrain*xlfact;
        double diddl = cdrain/(inst->MOSeffectiveLength - delxl);
        inst->MOSgm = inst->MOSgm*xlfact + diddl*ddldvg;
        inst->MOSgmbs = inst->MOSgmbs*xlfact + diddl*ddldvb;
        gds0 = diddl*ddldvd;
        inst->MOSgm = inst->MOSgm + gds0*dvsdvg;
        inst->MOSgmbs = inst->MOSgmbs + gds0*dvsdvb;
        inst->MOSgds = inst->MOSgds*xlfact + diddl*dldvd + gds0*dvsdvd;
/*
        cdrain = cdrain*xlfact;
        double diddl = cdrain/(inst->MOSeffectiveLength - delxl);
        inst->MOSgm= inst->MOSgm*xlfact + diddl*ddldvg;
        gds0 = inst->MOSgds*xlfact + diddl*ddldvd;
        inst->MOSgmbs = inst->MOSgmbs*xlfact + diddl*ddldvb;
        inst->MOSgm = inst->MOSgm + gds0*dvsdvg;
        inst->MOSgmbs = inst->MOSgmbs + gds0*dvsdvb;
        inst->MOSgds = gds0*dvsdvd + diddl*dldvd;
*/
    }

// invers:
    //
    //.....finish strong inversion case
    //
    if (lvgs < ms_von) {
        //
        //.....weak inversion
        //
        double onxn   = 1.0/xn;
        double ondvt  = onxn/ms_vt;
        double wfact  = exp((lvgs - ms_von)*ondvt);
        cdrain = cdrain*wfact;

        double gms    = inst->MOSgm*wfact;
        double gmw    = cdrain*ondvt;
        inst->MOSgm = gmw;
        if (lvds > ms_vdsat)
            inst->MOSgm = inst->MOSgm+gds0*dvsdvg*wfact;
        inst->MOSgds = inst->MOSgds*wfact+(gms-gmw)*dvodvd;
        inst->MOSgmbs = inst->MOSgmbs*wfact + (gms-gmw)*dvodvb - gmw*
            (lvgs - ms_von)*onxn*dxndvb;
    }

    return (cdrain);
}


//     This code evaluates the drain current and its 
//     derivatives using the n-th power MOS model and the 
//     charges associated with the gate, channel and bulk for 
//     mosfets
//
double
mosstuff::eq6(sMOSmodel *model, sMOSinstance *inst)
{
    double lvds, lvbs, lvgs;
    if (inst->MOSmode > 0) {
        lvbs = ms_vbs;
        lvds = ms_vds;
        lvgs = ms_vgs;
    }
    else {
        lvbs = ms_vbd;
        lvds = -ms_vds;
        lvgs = ms_vgd;
    }

    double sarg;
    if (lvbs <= 0)
        sarg = sqrt(inst->MOStPhi - lvbs);
    else {
        sarg = sqrt(inst->MOStPhi);
        sarg = sarg - lvbs/(sarg+sarg);
        sarg = SPMAX(0,sarg);
    }
    ms_von = (inst->MOStVbi*model->MOStype)+model->MOSgamma*sarg
        - model->MOSgamma1 * lvbs - model->MOSsigma  * lvds;
    double vgon = lvgs - ms_von;

    double cdrain;
    if (vgon <= 0) {
        //
        //     cutoff region
        //
        ms_vdsat  = 0;
        inst->MOSgm   = 0;
        inst->MOSgds  = 0;
        inst->MOSgmbs = 0;
        cdrain        = 0;

    }
    else {
        double vonbm;
        if (sarg <= 0)
            vonbm = 0;
        else {
            if (lvbs <= 0 ) {
                vonbm = model->MOSgamma1
                    + model->MOSgamma/(sarg + sarg);
            }
            else {
                vonbm = model->MOSgamma1
                    + .5*model->MOSgamma/sqrt(inst->MOStPhi);
            }
        }
        sarg = log(vgon);
        ms_vdsat = model->MOSkv * exp(sarg * model->MOSnv);
        double idsat = inst->MOSbeta * exp(sarg * model->MOSnc);
        double lambda = model->MOSlamda0 - model->MOSlamda1*lvbs;
        //
        //     saturation region
        //
        cdrain = idsat*(1 + lambda*lvds);
        inst->MOSgm   = cdrain*model->MOSnc/vgon;
        inst->MOSgds  = inst->MOSgm*model->MOSsigma + idsat*lambda;
        inst->MOSgmbs = inst->MOSgm*vonbm - idsat*model->MOSlamda1*lvds;

        if (ms_vdsat > lvds) {
            //
            //     linear region
            //
            double vdst   = lvds/ms_vdsat;
            double vdst2  = (2 - vdst)*vdst;
            double vdstg  = -vdst*model->MOSnv/vgon;
            double ivdst1 = cdrain*(2 - vdst - vdst);
            cdrain = cdrain*vdst2;
            inst->MOSgm = inst->MOSgm*vdst2 + ivdst1*vdstg;
            inst->MOSgds = inst->MOSgds*vdst2 +
                ivdst1*(1/ms_vdsat + vdstg*model->MOSsigma);
            inst->MOSgmbs = inst->MOSgmbs*vdst2 +
                ivdst1*vdstg*vonbm;
        }
    }
    return (cdrain);
}


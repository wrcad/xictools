
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
 $Id: jfetseti.cc,v 1.3 2015/07/29 04:50:20 stevew Exp $
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3f2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 Thomas L. Quarles
         1993 Stephen R. Whiteley
****************************************************************************/
/********** new in 3f2
Sydney University mods Copyright(c) 1989 Anthony E. Parker, David J. Skellern
    Laboratory for Communication Science Engineering
    Sydney University Department of Electrical Engineering, Australia
**********/

#include "jfetdefs.h"


int
JFETdev::setInst(int param, IFdata *data, sGENinstance *geninst)
{
    sJFETinstance *inst = static_cast<sJFETinstance*>(geninst);
    IFvalue *value = &data->v;

    switch (param) {
    case JFET_AREA:
        inst->JFETarea = value->rValue;
        inst->JFETareaGiven = true;
        break;
    case JFET_IC_VDS:
        inst->JFETicVDS = value->rValue;
        inst->JFETicVDSGiven = true;
        break;
    case JFET_IC_VGS:
        inst->JFETicVGS = value->rValue;
        inst->JFETicVGSGiven = true;
        break;
    case JFET_IC:
        switch (value->v.numValue) {
        case 2:
            inst->JFETicVGS = *(value->v.vec.rVec+1);
            inst->JFETicVGSGiven = true;
        case 1:
            inst->JFETicVDS = *(value->v.vec.rVec);
            inst->JFETicVDSGiven = true;
            data->cleanup();
            break;
        default:
            data->cleanup();
            return (E_BADPARM);
        }
        break;
    case JFET_OFF:
        inst->JFEToff = value->iValue;
        break;
    case JFET_TEMP:
        inst->JFETtemp = value->rValue+CONSTCtoK;
        inst->JFETtempGiven = true;
        break;
    default:
        return (E_BADPARM);
    }
    return (OK);
}

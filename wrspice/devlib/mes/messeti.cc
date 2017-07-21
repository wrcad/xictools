
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
 $Id: messeti.cc,v 1.3 2015/07/29 04:50:20 stevew Exp $
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3f2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 S. Hwang
         1993 Stephen R. Whiteley
****************************************************************************/

#include "mesdefs.h"


int
MESdev::setInst(int param, IFdata *data, sGENinstance *geninst)
{
    sMESinstance *inst = static_cast<sMESinstance*>(geninst);
    IFvalue *value = &data->v;

    switch (param) {
    case MES_AREA:
        inst->MESarea = value->rValue;
        inst->MESareaGiven = true;
        break;
    case MES_IC_VDS:
        inst->MESicVDS = value->rValue;
        inst->MESicVDSGiven = true;
        break;
    case MES_IC_VGS:
        inst->MESicVGS = value->rValue;
        inst->MESicVGSGiven = true;
        break;
    case MES_IC:
        switch (value->v.numValue) {
        case 2:
            inst->MESicVGS = *(value->v.vec.rVec+1);
            inst->MESicVGSGiven = true;
            // fallthrough
        case 1:
            inst->MESicVDS = *(value->v.vec.rVec);
            inst->MESicVDSGiven = true;
            data->cleanup();
            break;
        default:
            data->cleanup();
            return(E_BADPARM);
        }
        break;
    case MES_OFF:
        inst->MESoff = value->iValue;
        break;
    default:
        return (E_BADPARM);
    }
    return (OK);
}

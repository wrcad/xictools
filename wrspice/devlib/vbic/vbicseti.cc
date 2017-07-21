
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
 $Id: vbicseti.cc,v 1.3 2015/07/29 04:50:20 stevew Exp $
 *========================================================================*/

/**********
Copyright 1990 Regents of the University of California.  All rights reserved.
Author: 1985 Thomas L. Quarles
Model Author: 1995 Colin McAndrew Motorola
Spice3 Implementation: 2003 Dietmar Warning DAnalyse GmbH
**********/

#include "vbicdefs.h"

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif


// This routine sets instance parameters for
// VBICs in the circuit.
//
int
VBICdev::setInst(int param, IFdata *data, sGENinstance *geninst)
{
    sVBICinstance *here = static_cast<sVBICinstance*>(geninst);
    IFvalue *value = &data->v;

    switch(param) {
    case VBIC_AREA:
        here->VBICarea = value->rValue;
        here->VBICareaGiven = TRUE;
        break;
    case VBIC_OFF:
        here->VBICoff = value->iValue;
        break;
    case VBIC_IC_VBE:
        here->VBICicVBE = value->rValue;
        here->VBICicVBEGiven = TRUE;
        break;
    case VBIC_IC_VCE:
        here->VBICicVCE = value->rValue;
        here->VBICicVCEGiven = TRUE;
        break;
    case VBIC_TEMP:
        here->VBICtemp = value->rValue+CONSTCtoK;
        here->VBICtempGiven = TRUE;
        break;
    case VBIC_DTEMP:
        here->VBICdtemp = value->rValue;
        here->VBICdtempGiven = TRUE;
        break;
    case VBIC_M:
        here->VBICm = value->rValue;
        here->VBICmGiven = TRUE;
        break;
    case VBIC_IC :
        switch(value->v.numValue) {
        case 2:
            here->VBICicVCE = *(value->v.vec.rVec+1);
            here->VBICicVCEGiven = TRUE;
            // fallthrough
        case 1:
            here->VBICicVBE = *(value->v.vec.rVec);
            here->VBICicVBEGiven = TRUE;
            data->cleanup();
            break;
        default:
            data->cleanup();
            return(E_BADPARM);
        }
        break;
    default:
        return(E_BADPARM);
    }
    return(OK);
}

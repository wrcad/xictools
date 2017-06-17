
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
 $Id: dioseti.cc,v 1.5 2015/11/22 01:20:25 stevew Exp $
 *========================================================================*/

/**********
Copyright 1990 Regents of the University of California.  All rights reserved.
Author: 1985 Thomas L. Quarles
Modified by Dietmar Warning 2003 and Paolo Nenzi 2003
**********/

#include "diodefs.h"

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif


int
DIOdev::setInst(int param, IFdata *data, sGENinstance *geninst)
{
#ifdef WITH_CMP_GOTO
    // Order MUST match parameter definition enum!
    static void *array[] = {
        0, // notused
        &&L_DIO_AREA, 
        &&L_DIO_IC,
        &&L_DIO_OFF,
        &&L_DIO_TEMP,
        &&L_DIO_DTEMP,
        &&L_DIO_PJ,
        &&L_DIO_M};
        // &&L_DIO_CAP,
        // &&L_DIO_CURRENT,
        // &&L_DIO_VOLTAGE,
        // &&L_DIO_CHARGE,
        // &&L_DIO_CAPCUR,
        // &&L_DIO_CONDUCT,
        // &&L_DIO_POWER,

        // &&L_DIO_POSNODE,
        // &&L_DIO_NEGNODE,
        // &&L_DIO_INTNODE};

    if ((unsigned int)param > DIO_M)
        return (E_BADPARM);
#endif

    sDIOinstance *here = static_cast<sDIOinstance*>(geninst);
    IFvalue *value = &data->v;

#ifdef WITH_CMP_GOTO
    void *jmp = array[param];
    if (!jmp)
        return (E_BADPARM);
    goto *jmp;
    L_DIO_AREA:
        here->DIOarea = value->rValue;
        here->DIOareaGiven = TRUE;
        return (OK);
    L_DIO_IC:
        here->DIOinitCond = value->rValue;
        return (OK);
    L_DIO_OFF:
        here->DIOoff = value->iValue;
        return (OK);
    L_DIO_TEMP:
        here->DIOtemp = value->rValue+CONSTCtoK;
        here->DIOtempGiven = TRUE;
        return (OK);
    L_DIO_DTEMP:
        here->DIOdtemp = value->rValue;
        here->DIOdtempGiven = TRUE;
        return (OK);    
    L_DIO_PJ:
        here->DIOpj = value->rValue;
        here->DIOpjGiven = TRUE;
        return (OK);
    L_DIO_M:
        here->DIOm = value->rValue;
        here->DIOmGiven = TRUE;
        return (OK);
#else
    switch(param) {
    case DIO_AREA:
        here->DIOarea = value->rValue;
        here->DIOareaGiven = TRUE;
        break;
    case DIO_IC:
        here->DIOinitCond = value->rValue;
        break;
    case DIO_OFF:
        here->DIOoff = value->iValue;
        break;
    case DIO_TEMP:
        here->DIOtemp = value->rValue+CONSTCtoK;
        here->DIOtempGiven = TRUE;
        break;
    case DIO_DTEMP:
        here->DIOdtemp = value->rValue;
        here->DIOdtempGiven = TRUE;
        break;    
    case DIO_PJ:
        here->DIOpj = value->rValue;
        here->DIOpjGiven = TRUE;
        break;
    case DIO_M:
        here->DIOm = value->rValue;
        here->DIOmGiven = TRUE;
        break;
    default:
        return(E_BADPARM);
    }
#endif
    return(OK);
}

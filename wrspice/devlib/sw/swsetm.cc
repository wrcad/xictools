
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
 $Id: swsetm.cc,v 1.2 2015/07/26 01:09:13 stevew Exp $
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3e2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 Gordon M. Jacobs
         1992 Stephen R. Whiteley
****************************************************************************/

#include "swdefs.h"


int
SWdev::setModl(int param, IFdata *data, sGENmodel *genmod)
{
    sSWmodel *model = static_cast<sSWmodel*>(genmod);
    IFvalue *value = &data->v;

    switch (param) {

    case SW_MOD_SW:
    case SW_MOD_CSW:
        // just says that this is a switch
        break;
    case SW_MOD_RON:
        model->SWonResistance = value->rValue;
        model->SWonConduct = 1.0/(value->rValue);
        model->SWonGiven = true;
        break;
    case SW_MOD_ROFF:
        model->SWoffResistance = value->rValue;
        model->SWoffConduct = 1.0/(value->rValue);
        model->SWoffGiven = true;
        break;
    case SW_MOD_VTH:
        model->SWvThreshold = value->rValue;
        model->SWvThreshGiven = true;
        break;
    case SW_MOD_VHYS:
        // take absolute value of hysteresis voltage
        model->SWvHysteresis = FABS(value->rValue);
        model->SWvHystGiven = true;
        break;
    case SW_MOD_ITH:
        model->SWiThreshold = value->rValue;
        model->SWiThreshGiven = true;
        break;
    case SW_MOD_IHYS:
        // take absolute value of hysteresis current
        model->SWiHysteresis = FABS(value->rValue);
        model->SWiHystGiven = true;
        break;
    default:
        return (E_BADPARM);
    }
    return (OK);
}

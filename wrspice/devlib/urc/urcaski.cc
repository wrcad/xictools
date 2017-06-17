
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
 $Id: urcaski.cc,v 1.2 2015/07/26 01:09:14 stevew Exp $
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3f2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 Thomas L. Quarles
         1993 Stephen R. Whiteley
****************************************************************************/

#include "urcdefs.h"


int
URCdev::askInst(const sCKT*, const sGENinstance *geninst, int which,
    IFdata *data)
{
    const sURCinstance *inst = static_cast<const sURCinstance*>(geninst);

    switch (which) {
    case URC_POS_NODE:
        data->type = IF_INTEGER;
        data->v.iValue = inst->URCposNode;
        break;
    case URC_NEG_NODE:
        data->type = IF_INTEGER;
        data->v.iValue = inst->URCnegNode;
        break;
    case URC_GND_NODE:
        data->type = IF_INTEGER;
        data->v.iValue = inst->URCgndNode;
        break;
    case URC_LEN:
        data->type = IF_REAL;
        data->v.rValue = inst->URClength;
        break;
    case URC_LUMPS:
        data->type = IF_INTEGER;
        data->v.iValue = inst->URClumps;
        break;
    default:
        return (E_BADPARM);
    }
    return (OK);
}


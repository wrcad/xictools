
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
 $Id: mutaski.cc,v 1.3 2015/09/09 18:22:21 stevew Exp $
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3f2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 Thomas L. Quarles
         1992 Stephen R. Whiteley
****************************************************************************/

#include "inddefs.h"


int
MUTdev::askInst(const sCKT*, const sGENinstance *geninst, int which,
    IFdata *data)
{
    const sMUTinstance *inst = static_cast<const sMUTinstance*>(geninst);

    switch (which) {
    case MUT_COEFF:
        data->type = IF_REAL;
        data->v.rValue = inst->MUTcoupling;
        break;
    case MUT_FACTOR:
        data->type = IF_REAL;
        data->v.rValue = inst->MUTfactor;
        break;
    case MUT_IND1:
        data->type = IF_INSTANCE;
        data->v.uValue = inst->MUTindName1;
        break;
    case MUT_IND2:
        data->type = IF_INSTANCE;
        data->v.uValue = inst->MUTindName2;
        break;
    default:
        return (E_BADPARM);
    }
    return (OK);
}


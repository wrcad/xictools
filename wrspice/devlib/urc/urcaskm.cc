
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
 $Id: urcaskm.cc,v 1.1 2015/07/26 01:09:14 stevew Exp $
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3f2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 Thomas L. Quarles
         1993 Stephen R. Whiteley
****************************************************************************/

#include "urcdefs.h"


int
URCdev::askModl(const sGENmodel *genmod, int which, IFdata *data)
{
    const sURCmodel *model = static_cast<const sURCmodel*>(genmod);
    IFvalue *value = &data->v;
    // Need to override this for non-real returns.
    data->type = IF_REAL;

    switch (which) {
    case URC_MOD_K:
        value->rValue = model->URCk;
        break;
    case URC_MOD_FMAX:
        value->rValue = model->URCfmax;
        break;
    case URC_MOD_RPERL:
        value->rValue = model->URCrPerL;
        break;
    case URC_MOD_CPERL:
        value->rValue = model->URCcPerL;
        break;
    case URC_MOD_ISPERL:
        value->rValue = model->URCisPerL;
        break;
    case URC_MOD_RSPERL:
        value->rValue = model->URCrsPerL;
        break;
    default:
        return (E_BADPARM);
    }
    return (OK);
}

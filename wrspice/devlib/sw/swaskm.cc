
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
 $Id: swaskm.cc,v 1.1 2015/07/26 01:09:13 stevew Exp $
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3e2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 Gordon M. Jacobs
         1992 Stephen R. Whiteley
****************************************************************************/

#include "swdefs.h"


int
SWdev::askModl(const sGENmodel *genmod, int which, IFdata *data)
{
    const sSWmodel *model = static_cast<const sSWmodel*>(genmod);
    IFvalue *value = &data->v;
    // Need to override this for non-real returns.
    data->type = IF_REAL;

    switch (which) {
    case SW_MOD_RON:
        value->rValue = model->SWonResistance;
        break;
    case SW_MOD_ROFF:
        value->rValue = model->SWoffResistance;
        break;
    case SW_MOD_VTH:
        value->rValue = model->SWvThreshold;
        break;
    case SW_MOD_VHYS:
        value->rValue = model->SWvHysteresis;
        break;
    case SW_MOD_ITH:
        value->rValue = model->SWiThreshold;
        break;
    case SW_MOD_IHYS:
        value->rValue = model->SWiHysteresis;
        break;
    case SW_MOD_GON:
        value->rValue = model->SWonConduct;
        break;
    case SW_MOD_GOFF:
        value->rValue = model->SWoffConduct;
        break;
    default:
        return (E_BADPARM);
    }
    return (OK);
}

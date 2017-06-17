
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
 $Id: capaskm.cc,v 1.3 2015/07/26 01:09:11 stevew Exp $
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3f2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 Thomas L. Quarles
         1992 Stephen R. Whiteley
****************************************************************************/

#include "capdefs.h"


int
CAPdev::askModl(const sGENmodel *genmod, int which, IFdata *data)
{
    const sCAPmodel *model = static_cast<const sCAPmodel*>(genmod);
    IFvalue *value = &data->v;
    // Need to override this for non-real returns.
    data->type = IF_REAL;

    switch (which) {
    case CAP_MOD_CJ:
        value->rValue = model->CAPcj;
        break;
    case CAP_MOD_CJSW:
        value->rValue = model->CAPcjsw;
        break;
    case CAP_MOD_DEFWIDTH:
        value->rValue = model->CAPdefWidth;
        break;
    case CAP_MOD_NARROW:
        value->rValue = model->CAPnarrow;
        break;
    case CAP_MOD_TNOM:
        value->rValue = model->CAPtnom - CONSTCtoK;
        break;
    case CAP_MOD_TC1:
        value->rValue = model->CAPtempCoeff1;
        break;
    case CAP_MOD_TC2:
        value->rValue = model->CAPtempCoeff2;
        break;
    default:  
        return (E_BADPARM);
    }
    return (OK);
}


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
 $Id: resaskm.cc,v 1.5 2015/07/26 01:09:13 stevew Exp $
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3f2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 Thomas L. Quarles
         1993 Stephen R. Whiteley
****************************************************************************/

#include "resdefs.h"


int 
RESdev::askModl(const sGENmodel *genmod, int which, IFdata *data)
{
    const sRESmodel *model = static_cast<const sRESmodel*>(genmod);
    IFvalue *value = &data->v;
    // Need to override this for non-real returns.
    data->type = IF_REAL;

    switch (which) {
    case RES_MOD_RSH:
        value->rValue = model->RESsheetRes;
        break;
    case RES_MOD_NARROW: 
        value->rValue = model->RESnarrow;
        break;
    case RES_MOD_DL:
        value->rValue = model->RESshorten;
        break;
    case RES_MOD_TC1:
        value->rValue = model->REStempCoeff1;
        break;
    case RES_MOD_TC2:
        value->rValue = model->REStempCoeff2;
        break;
    case RES_MOD_DEFWIDTH:
        value->rValue = model->RESdefWidth;
        break;
    case RES_MOD_DEFLENGTH:
        value->rValue = model->RESdefLength;
        break;
    case RES_MOD_TNOM:
        value->rValue = model->REStnom-CONSTCtoK;
        break;
    case RES_MOD_TEMP:
        value->rValue = model->REStemp-CONSTCtoK;
        break;
    case RES_MOD_NOISE:
        value->rValue = model->RESnoise;
        break;
    case RES_MOD_KF:
        value->rValue = model->RESkf;
        break;
    case RES_MOD_AF:
        value->rValue = model->RESaf;
        break;
    case RES_MOD_EF:
        value->rValue = model->RESef;
        break;
    case RES_MOD_WF:
        value->rValue = model->RESwf;
        break;
    case RES_MOD_LF:
        value->rValue = model->RESlf;
        break;
    default:
        return (E_BADPARM);
    }
    return (OK);
}


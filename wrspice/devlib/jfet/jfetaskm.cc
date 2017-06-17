
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
 $Id: jfetaskm.cc,v 1.2 2015/07/26 01:09:12 stevew Exp $
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3f2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1987 Mathew Lew and Thomas L. Quarles
         1993 Stephen R. Whiteley
****************************************************************************/
/********** new in 3f2
Sydney University mods Copyright(c) 1989 Anthony E. Parker, David J. Skellern
    Laboratory for Communication Science Engineering
    Sydney University Department of Electrical Engineering, Australia
**********/

#include "jfetdefs.h"


int
JFETdev::askModl(const sGENmodel *genmod, int which, IFdata *data)
{
    const sJFETmodel *model = static_cast<const sJFETmodel*>(genmod);
    IFvalue *value = &data->v;
    // Need to override this for non-real returns.
    data->type = IF_REAL;

    switch (which) {
    case JFET_MOD_VTO:
        value->rValue = model->JFETthreshold;
        break;
    case JFET_MOD_BETA:
        value->rValue = model->JFETbeta;
        break;
    case JFET_MOD_LAMBDA:
        value->rValue = model->JFETlModulation;
        break;
    case JFET_MOD_RD:
        value->rValue = model->JFETdrainResist;
        break;
    case JFET_MOD_RS:
        value->rValue = model->JFETsourceResist;
        break;
    case JFET_MOD_CGS:
        value->rValue = model->JFETcapGS;
        break;
    case JFET_MOD_CGD:
        value->rValue = model->JFETcapGD;
        break;
    case JFET_MOD_PB:
        value->rValue = model->JFETgatePotential;
        break;
    case JFET_MOD_IS:
        value->rValue = model->JFETgateSatCurrent;
        break;
    case JFET_MOD_FC:
        value->rValue = model->JFETdepletionCapCoeff;
        break;
    case JFET_MOD_B:
        value->rValue = model->JFETb;
        break;
    case JFET_MOD_DRAINCOND:
        value->rValue = model->JFETdrainConduct;
        break;
    case JFET_MOD_SOURCECOND:
        value->rValue = model->JFETsourceConduct;
        break;
    case JFET_MOD_TYPE:
        if (model->JFETtype == NJF)
            value->sValue = "njf";
        else
            value->sValue = "pjf";
        data->type = IF_STRING;
        break;
    case JFET_MOD_TNOM:
        value->rValue = model->JFETtnom-CONSTCtoK;
        break;
    default:
        return (E_BADPARM);
    }
    return (OK);
}


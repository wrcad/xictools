
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
 $Id: mestemp.cc,v 1.0 1998/01/30 05:31:38 stevew Exp $
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3f2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 S. Hwang
         1993 Stephen R. Whiteley
****************************************************************************/

#include "mesdefs.h"


int
MESdev::temperature(sGENmodel *genmod, sCKT *ckt)
{
    (void)ckt;
    sMESmodel *model = static_cast<sMESmodel*>(genmod);
    for ( ; model; model = model->next()) {

        if (model->MESdrainResist != 0)
            model->MESdrainConduct = 1/model->MESdrainResist;
        else
            model->MESdrainConduct = 0;
        if (model->MESsourceResist != 0)
            model->MESsourceConduct = 1/model->MESsourceResist;
        else
            model->MESsourceConduct = 0;

        model->MESdepletionCap = model->MESdepletionCapCoeff *
                model->MESgatePotential;
        double xfc = (1 - model->MESdepletionCapCoeff);
        double temp = sqrt(xfc);
        model->MESf1 = model->MESgatePotential * (1 - temp)/(1-.5);
        model->MESf2 = temp * temp * temp;
        model->MESf3 = 1 - model->MESdepletionCapCoeff * (1 + .5);
        model->MESvcrit = CONSTvt0 * log(CONSTvt0/
                (CONSTroot2 * model->MESgateSatCurrent));
    }
    return(OK);
}

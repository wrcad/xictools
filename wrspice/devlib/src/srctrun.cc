
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
 $Id: srctrun.cc,v 2.9 2011/05/26 17:15:18 stevew Exp $
 *========================================================================*/

#include "srcdefs.h"


int
SRCdev::trunc(sGENmodel *genmod, sCKT *ckt, double *timeStep)
{
    double time_step = HUGE_VAL;
    sSRCmodel *model = static_cast<sSRCmodel*>(genmod);
    for ( ; model; model = model->next()) {
        sSRCinstance *inst;
        for (inst = model->inst(); inst; inst = inst->next()) {
            if (inst->SRCtree)
                time_step = inst->SRCtree->timeLim(time_step);
        }
    }
    if (time_step < HUGE_VAL) {
        // The timeStep is un-normalized!
        double tmp = time_step;
        for (int i = 1; i < ckt->CKTorder; i++)
            time_step *= tmp;
    }
    if (*timeStep > time_step)
        *timeStep = time_step;
    return (OK);
}

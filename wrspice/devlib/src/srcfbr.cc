
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
 $Id: srcfbr.cc,v 1.2 2015/07/24 02:51:30 stevew Exp $
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3e2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 Thomas L. Quarles
         1987 Kanwar Jit Singh
         1992 Stephen R. Whiteley
****************************************************************************/

#include "srcdefs.h"


int
SRCdev::findBranch(sCKT *ckt, sGENmodel *genmod, IFuid sname)
{
    sSRCmodel *model = static_cast<sSRCmodel*>(genmod);
    for ( ; model; model = model->next()) {
        sSRCinstance *inst;
        for (inst = model->inst(); inst; inst = inst->next()) {

            if (inst->GENname == sname && inst->SRCtype == SRC_V) {
                if (inst->SRCbranch == 0) {
                    sCKTnode *tmp;
                    int error = ckt->mkCur(&tmp, inst->GENname, "branch");
                    if (error)
                        return (error);
                    inst->SRCbranch = tmp->number();
                }
                return (inst->SRCbranch);
            }
        }
    }
    return (0);
}

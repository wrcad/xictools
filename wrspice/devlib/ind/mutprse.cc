
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
 $Id: mutprse.cc,v 2.16 2015/08/08 01:50:56 stevew Exp $
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3e2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1987 Thomas L. Quarles
         1993 Stephen R. Whiteley
****************************************************************************/

#include <ctype.h>
#include "inddefs.h"
#include "input.h"


// mutual inductance parser
// Kname Lname Lname <val>
//
void
MUTdev::parse(int type, sCKT *ckt, sLine *current)
{
    const char *line = current->line();
    char *dname = IP.getTok(&line, true);
    if (!dname)
        return;
    ckt->insert(&dname);
    char dvkey[2];
    dvkey[0] = islower(*dname) ? toupper(*dname) : *dname;
    dvkey[1] = 0;

    int error;
    sGENmodel *mdfast;
    sCKTmodItem *mx = ckt->CKTmodels.find(type);
    if (mx && mx->default_model)
        mdfast = mx->default_model;
    else {
        // create default model
        IFuid uid;
        ckt->newUid(&uid, 0, dvkey, UID_MODEL);
        sGENmodel *m;
        error = ckt->newModl(type, &m, uid);
        IP.logError(current, error);
        if (!mx) {
            // newModl should have created this
            mx = ckt->CKTmodels.find(type);
        }
        if (mx)
            mx->default_model = m;
        mdfast = m;
    }
    sGENinstance *fast; // pointer to the actual instance
    error = ckt->newInst(mdfast, &fast, dname);
    IP.logError(current, error);

    IFdata data;
    data.type = IF_INSTANCE;
    IP.getValue(&line, &data, ckt);
    error = fast->setParam("inductor1", &data);
    IP.logError(current, error);
    IP.getValue(&line, &data, ckt);
    error = fast->setParam("inductor2", &data);
    IP.logError(current, error);

    IP.devParse(current, &line, ckt, fast, "coefficient");
}


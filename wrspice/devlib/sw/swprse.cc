
/*========================================================================*
 *                                                                        *
 *  Distributed by Whiteley Research Inc., Sunnyvale, California, USA     *
 *                       http://wrcad.com                                 *
 *  Copyright (C) 2017 Whiteley Research Inc., all rights reserved.       *
 *  Author: Stephen R. Whiteley, except as indicated.                     *
 *                                                                        *
 *  As fully as possible recognizing licensing terms and conditions       *
 *  imposed by earlier work from which this work was derived, if any,     *
 *  this work is released under the Apache License, Version 2.0 (the      *
 *  "License").  You may not use this file except in compliance with      *
 *  the License, and compliance with inherited licenses which are         *
 *  specified in a sub-header below this one if applicable.  A copy       *
 *  of the License is provided with this distribution, or you may         *
 *  obtain a copy of the License at                                       *
 *                                                                        *
 *        http://www.apache.org/licenses/LICENSE-2.0                      *
 *                                                                        *
 *  See the License for the specific language governing permissions       *
 *  and limitations under the License.                                    *
 *                                                                        *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,      *
 *   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES      *
 *   OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-        *
 *   INFRINGEMENT.  IN NO EVENT SHALL WHITELEY RESEARCH INCORPORATED      *
 *   OR STEPHEN R. WHITELEY BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER     *
 *   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,      *
 *   ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE       *
 *   USE OR OTHER DEALINGS IN THE SOFTWARE.                               *
 *                                                                        *
 *========================================================================*
 *               XicTools Integrated Circuit Design System                *
 *                                                                        *
 * WRspice Circuit Simulation and Analysis Tool:  Device Library          *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3e2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1987 Thomas L. Quarles
         1993 Stephen R. Whiteley
****************************************************************************/

#include <ctype.h>
#include "swdefs.h"
#include "input.h"


// switch parser
// VOLTAGE CONTROLLED SWITCH
//   Sname <node> <node> <node> <node> [<modname>] [IC]
// CURRENT CONTROLLED SWITCH
//   Wname <node> <node> <vctrl> [<modname>] [IC]
//
void
SWdev::parse(int type, sCKT *ckt, sLine *current)
{
    const char *toofewmsg = "Error: Too few nodes given for device.";

    const char *line = current->line();
    char *dname = IP.getTok(&line, true);
    if (!dname)
        return;
    ckt->insert(&dname);
    char dvkey[2];
    dvkey[0] = islower(*dname) ? toupper(*dname) : *dname;
    dvkey[1] = 0;

    char *nname1 = IP.getTok(&line, true);
    if (!nname1) {
        IP.logError(current, toofewmsg);
        ckt->CKTnogo = true;
        return;
    }
    sCKTnode *node1;     // the first node's node pointer
    ckt->termInsert(&nname1, &node1);

    char *nname2 = IP.getTok(&line, true);
    if (!nname2) {
        IP.logError(current, toofewmsg);
        ckt->CKTnogo = true;
        return;
    }
    sCKTnode *node2;     // the second node's node pointer
    ckt->termInsert(&nname2, &node2);

    sCKTnode *node3;
    sCKTnode *node4;
    IFdata data;
    if (*dvkey == 'S') {
        // voltage controlled switch
        char *nname3 = IP.getTok(&line, true);
        if (!nname3) {
            IP.logError(current, toofewmsg);
            ckt->CKTnogo = true;
            return;
        }
        ckt->termInsert(&nname3, &node3);

        char *nname4 = IP.getTok(&line, true);
        if (!nname4) {
            IP.logError(current, toofewmsg);
            ckt->CKTnogo = true;
            return;
        }
        ckt->termInsert(&nname4, &node4);
    }
    else {
        // current controlled switch
        data.type = IF_INSTANCE;
        if (!IP.getValue(&line, &data, ckt)) {
            IP.logError(current, "Error: failed to find controlling source");
            ckt->CKTnogo = true;
            return;
        }
    }
    char *model = IP.getTok(&line, true);
    if (model)
        ckt->insert(&model);

    sINPmodel *thismodel;
    IP.getMod(current, ckt, model, 0, &thismodel);
    int error;
    sGENmodel *mdfast;   // pointer to the actual model
    if (thismodel != 0) {
        int ntype = thismodel->modType;
        if (ntype != type && !IP.checkKey(*dv_name, ntype)) {
            current->errcat("Incorrect model type");
            return;
        }
        char *mname = new char[strlen(thismodel->modName) + 1];
        strcpy(mname, thismodel->modName);
        ckt->insert(&mname);
        sGENmodel *mdf = 0;
        error = ckt->findModl(&ntype, &mdf, mname);
        if (error == 0)
            mdfast = mdf;
        else {
            IP.logError(current, error);
            mdfast = 0;
        }
    }
    else {
        sCKTmodItem *mx = ckt->CKTmodels.find(type);
        if (mx && mx->default_model)
            mdfast = mx->default_model;
        else {
            // create deafult model
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
    }
    sGENinstance *fast;  // pointer to the actual instance
    error = ckt->newInst(mdfast, &fast, dname);
    IP.logError(current, error);
    error = node1->bind(fast, 1);
    IP.logError(current, error);
    error = node2->bind(fast, 2);
    IP.logError(current, error);

    if (*dvkey == 'S') {
        error = node3->bind(fast, 3);
        IP.logError(current, error);
        error = node4->bind(fast, 4);
        IP.logError(current, error);
    }
    else {
        error = fast->setParam("control", &data);
        IP.logError(current, error);
    }

    IP.devParse(current, &line, ckt, fast, 0);
}


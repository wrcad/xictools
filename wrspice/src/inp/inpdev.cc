
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
 * WRspice Circuit Simulation and Analysis Tool                           *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3e2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1987 Thomas L. Quarles
         1992 Stephen R. Whiteley
****************************************************************************/

#include "input.h"
#include "simulator.h"
#include "device.h"
#include "misc.h"


namespace {
    // These are MOS instance parameters used in HSPICE, that have no
    // equivalence here.
    struct hs_param
    {
        hs_param(const char *n, int dt)
            {
                name = n;
                dtype = dt;
            }

        const char *name;
        int dtype;
    };

    hs_param hspice_unused[] = {
        hs_param("dtemp", IF_REAL),
        hs_param("sigma", IF_REAL),
        hs_param("mismatchflag", IF_INTEGER),
        hs_param(0, 0)
    };
}


//  Parse a given input according to the standard rules - look
//  for the parameters given in the parmlists, In addition, 
//  an optional leading numeric parameter is handled.
//
void
SPinput::devParse(
    sLine *curline,     // the line for error messages
    const char **line,  // the string to parse
    sCKT *ckt,          // the circuit being built
    sGENinstance *fast, // direct pointer to device being parsed
    const char *leadname) // leading param name that might be omitted
{
    int type = fast->GENmodPtr->GENmodType;
    IFdevice *device = DEV.device(type);

    // Grab the first token, and see if it is a parameter name.
    const char *tline = *line;
    char *parm = getTok(line, true);
    if (!parm)
        return;

    bool found = false;
    IFparm *p = device->findInstanceParm(parm, IF_SET);
    if (p) {
        found = true;

        // First token is a normal parameter, grab value.
        IFdata data;
        data.type = p->dataType;
        if (!getValue(line, &data, ckt)) {
            logError(curline, E_PARMVAL, p->keyword);
            delete [] parm;
            return;
        }
        int error = fast->setParam(p->id, &data);
        if (error) {
            logError(curline, error, p->keyword);
            delete [] parm;
            return;
        }
    }
    delete [] parm;
    if (!found)
        *line = tline;

    if (!found && leadname && *leadname) {
        // Not a parameter name, so it might be a leading expression
        // or number.

        p = device->findInstanceParm(leadname, IF_SET);
        if (p) {
            IFdata data;
            data.type = p->dataType;

            if (IP.getValue(line, &data, ckt)) {
                int error = fast->setParam(p->id, &data);
                if (error) {
                    logError(curline, error, p->keyword);
                    return;
                }
            }
        }
    }

    while (**line) {
        parm = getTok(line, true);
        if (!parm)
            break;

        p = device->findInstanceParm(parm, IF_SET);
        if (p) {
            delete [] parm;
            IFdata data;
            data.type = p->dataType;
            if (!getValue(line, &data, ckt)) {
                logError(curline, E_PARMVAL, p->keyword);
                return;
            }
            int error = fast->setParam(p->id, &data);
            if (error) {
                logError(curline, error, p->keyword);
                return;
            }
        }
        else {

            found = false;
            if (device->isMOS()) {
                // MOS device.  Check if this is an HSPICE
                // parameter that we don't handle.
                for (hs_param *h = hspice_unused; h->name; h++) {
                    if (lstring::cieq(parm, h->name)) {

                        // Read the value.
                        IFdata data;
                        data.type = h->dtype;
                        if (!getValue(line, &data, ckt)) {
                            logError(curline, E_PARMVAL, parm);
                            delete [] parm;
                            return;
                        }

                        // If the value is zero, just silently ignore the
                        // parameter assignment.
                        bool is_zero = false;
                        if (data.type == IF_REAL) {
                            if (data.v.rValue == 0.0)
                                is_zero = true;
                        }
                        else if (data.type == IF_INTEGER) {
                            if (data.v.iValue == 0)
                                is_zero = true;
                        }
                        if (!is_zero && !Sp.HspiceFriendly()) {
                            logError(curline,
                                "HSPICE parameter %s not handled, ignored",
                                parm);
                        }
                        found = true;
                        break;
                    }
                }
            }
            if (!found) {
                if (lstring::cieq(parm, "model")) {
                    // Get these in SPICE output from Assura, read the
                    // text token that follows and silently ignore.

                    delete [] parm;
                    parm = getTok(line, true);
                    delete [] parm;
                    continue;
                }
                logError(curline, E_BADPARM, parm);
            }
            delete [] parm;
        }
    }
}


// Find the range of nodes used by the device in str and the number of
// devices referenced, and whether the device has a model.  This is
// called during subcircuit expansion, i.e., before parse.  We don't
// know about the models yet.  The min and max returns represent all
// matches.
//
int
SPinput::numRefs(const char *str, int *minterms, int *maxterms, int *devs,
    int *hasmod)
{
    // Note that the returns here are wrong for e,f,g,h sources with the
    // function or poly constructs.

    int nmin = 1000;
    int nmax = 0;
    bool mod = false;
    int ndevs = 0;
    int c = *str;
    if (isalpha(c)) {
        for (int i = 0; i < DEV.numdevs(); i++) {
            IFdevice *dev = DEV.device(i);
            if (!dev)
                continue;
            IFkeys *k = dev->keyMatch(c);
            if (k) {
                if (k->minTerms < nmin)
                    nmin = k->minTerms;
                if (k->maxTerms > nmax)
                    nmax = k->maxTerms;
                if (k->numDevs > ndevs)
                    ndevs = k->numDevs;
                if (dev->modelKey(0))
                    mod = true;
            }
        }
    }
    if (minterms)
        *minterms = (nmin <= nmax ? nmin : 0);
    if (maxterms)
        *maxterms = nmax;
    if (devs)
        *devs = ndevs;
    if (hasmod)
        *hasmod = mod;
    return (OK);
}


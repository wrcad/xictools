
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
JSPICE3 adaptation of Spice3f2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1987 Thomas L. Quarles
         1992 Stephen R. Whiteley
****************************************************************************/

#include "simulator.h"
#include "input.h"
#include "circuit.h"
#include "misc.h"
#include "subexpand.h"
#include "optdefs.h"
#include "spnumber/hash.h"


namespace {
    sHtab *unh_tab;

    const char *unh_words[] = {
        "alias",
        "alter",
        "connect",
        "data",
        "dellib",
        "dout",
        "graph",
        "hdl",
        "lin",
        "malias",
        "probe",
        "protect",
        "stim",
        "unprotect",
        0
    };
}


// The token is a word starting with '.', return nonzero if the line
// starting with the token should be ignored.  The return value if
// nonzero can indicate a class of words to ignore, presently the only
// class is unhandled HSPICE keywords.
//
int
SPinput::checkDotCard(const char *token)
{
    if (!token || *token != '.' || !isalpha(token[1]))
        return (0);
    if (!unh_tab) {
        unh_tab = new sHtab(true);
        for (const char **s = unh_words; *s; s++)
            unh_tab->add(*s, (void*)1L);
    }
    return ((int)(intptr_t)sHtab::get(unh_tab, token + 1));
}


// Parse and process only the .options lines, anything else is ignored.
//
void
SPinput::parseOptions(sLine *opts, sJOB *job)
{
    for (sLine *dd = opts; dd; dd = dd->next()) {      
        const char *line = dd->line();
        char *token = getTok(&line, true);
        if (!token)
            continue;
        for (char *t = token+1; *t; t++) {
            if (isupper(*t))
                *t = tolower(*t);
        }
        if (lstring::eq(token, OPTIONS_KW) || lstring::eq(token, OPTION_KW) ||
                lstring::eq(token, OPT_KW)) {
            // A hack - pass the job in the sTASK arg.
            OPTinfo.parse(dd, 0, 0, &line, (sTASK*)job);
        }
        delete [] token;
    }
}


IFanalysis *
SPinput::getAnalysis(const char *name, int *which)
{
    for (int i = 0; ; i++) {
        IFanalysis *a = IFanalysis::analysis(i);
        if (!a)
            break;
        if (lstring::cieq(a->name, name)) {
            *which = i;
            return (a);
        }
    }
    *which = -1;
    return (0);
}


// Parse .<something>, many possibilities.  Return true only if this
// is a .end card.
//
bool
SPinput::parseDot(sCKT *ckt, sTASK *task, sLine *curline)
{
    const char *line = curline->line();
    char *token = getTok(&line, true);
    if (!token)
        return (false);

    // Convert token to lower case, match is case insensitive.
    for (char *t = token+1; *t; t++) {
        if (isupper(*t))
            *t = tolower(*t);
    }

    int which;
    // We know that IP.kwMatchInit was called recently.
    if (SPcx.kwMatchModel(token)) {
        // Don't have to do anything, since models were all done in
        // pass 1 
        //
        delete [] token;
        return (false);
    }
    if (lstring::eq(token, MOSMAP_KW) || lstring::eq(token, DEFMOD_KW)) {
        // Handled in pass 1.
        delete [] token;
        return (false);
    }

    if (lstring::eq(token, WIDTH_KW) || lstring::eq(token, PRINT_KW) ||
            lstring::eq(token, PLOT_KW)) {
        // obsolete - ignore
        delete [] token;
        logError(curline, "Obsolete control card - ignored.");
        return (false);
    }

    if (lstring::eq(token, TEMP_KW)) {
        // .temp temp1 temp2 temp3 temp4 .....
        delete [] token;

        // We'll save the temperature list in CKTtemps, but this is
        // not further used at present.  The first listed temperature
        // will also be extracted with the .options, and used to set a
        // dummy option specifying the temperature.

        if (ckt->CKTtemps) {
            logError(curline,
                ".TEMP already given, ignoring subsequent .TEMP line");
            return (false);
        }
        const char *ltmp = line;
        int cnt = 0;
        int error = 0;
        while (!error) {
            getFloat(&line, &error, true);
            if (!error)
                cnt++;
        }
        if (cnt) {
            double *tvals = new double[cnt];
            ckt->CKTtemps = tvals;
            ckt->CKTnumTemps = cnt;
            line = ltmp;
            for (cnt = 0; cnt < ckt->CKTnumTemps; ckt++)
                tvals[cnt] = getFloat(&line, &error, true);
        }
        else
            logError(curline, ".TEMP card has no temperatures given");

        if (cnt > 1)
            logError(curline, ".TEMP card uses only first listed temperature");
        return (false);
    }

    if (lstring::eq(token, FOUR_KW) || lstring::eq(token, FOURIER_KW)) {
        // not implemented - warn & ignore
        delete [] token;
        logError(curline, "Use fourier command to obtain fourier analysis");
        return (false);
    }

    if (lstring::eq(token, NODESET_KW)) {
        delete [] token;
        which = -1;
        for (int i = 0; ; i++) {
            IFparm *prm = sCKTnode::nodeParam(i);
            if (!prm)
                break;
            if (strcmp(prm->keyword, "nodeset") == 0) {
                which = prm->id;
                break;
            }
        }
        if (which == -1)
            logError(curline, "Nodeset unknown to simulator");
        else {
            for (;;) {
                char *name = getTok(&line, true);
                if (!name)
                    break;
                // check to see if in the form V(xxx) and grab the xxx
                if ((*name == 'V' || *name == 'v') && name[1] == 0) {
                    // looks like V - must be V(xx) - get xx now
                    delete [] name;
                    name = getTok(&line, true);
                }
                if (name) {
                    sCKTnode *node;
                    ckt->termInsert(&name, &node);
                    int error;
                    IFdata data;
                    data.type = IF_REAL;
                    data.v.rValue = getFloat(&line, &error, true);
                    node->set(which, &data);
                    logError(curline, error);
                    continue;
                }
                logError(curline, ".NODESET syntax error");
                break;
            }
        }
        return (false);
    }

    if (lstring::eq(token, IC_KW)) {
        delete [] token;
        which = -1;
        for (int i = 0; ; i++) {
            IFparm *prm = sCKTnode::nodeParam(i);
            if (!prm)
                break;
            if (strcmp(prm->keyword, "ic") == 0) {
                which = prm->id;
                break;
            }
        }
        if (which == -1)
            logError(curline, "IC unknown to simulator");
        else {
            for (;;) {
                char *name = getTok(&line, true);
                if (!name)
                    break;
                // check to see if in the form V(xxx) and grab the xxx
                if ((*name == 'V' || *name == 'v') && name[1] == 0) {
                    // looks like V - must be V(xx) - get xx now
                    delete [] name;
                    name = getTok(&line, true);
                }
                if (name) {
                    sCKTnode *node;
                    ckt->termInsert(&name, &node);
                    int error;
                    IFdata data;
                    data.type = IF_REAL;
                    data.v.rValue = getFloat(&line, &error, true);
                    node->set(which, &data);
                    logError(curline, error);
                    continue;
                }
                logError(curline, ".IC syntax error");
                break;
            }
        }
        return (false);
    }

    if (lstring::eq(token, OP_KW)) {
        if (task) {
            IFanalysis *an = getAnalysis(token+1, &which);
            delete [] token;
            if (an)
                an->parse(curline, ckt, which, &line, task);
            else
                logError(curline, "DC operating point analysis unsupported");
        }
        return (false);
    }

    if (lstring::eq(token, DC_KW)) {
        // .dc SRC1NAME Vstart1 Vstop1 Vinc1 [SRC2NAME Vstart2 Vstop2 Vinc2
        if (task) {
            IFanalysis *an = getAnalysis(token+1, &which);
            delete [] token;
            if (an)
                an->parse(curline, ckt, which, &line, task);
            else
                logError(curline, "DC transfer curve analysis unsupported");
        }
        return (false);
    }

    if (lstring::eq(token, AC_KW)) {
        // .ac {DEC OCT LIN} NP FSTART FSTOP
        if (task) {
            IFanalysis *an = getAnalysis(token+1, &which);
            delete [] token;
            if (an)
                an->parse(curline, ckt, which, &line, task);
            else
                logError(curline, "AC small signal analysis unsupported");
        }
        return (false);
    }

    if (lstring::eq(token, TRAN_KW)) {
        // .tran Tstep Tstop <Tstart <Tmax> > <UIC>
        if (task) {
            IFanalysis *an = getAnalysis(token+1, &which);
            delete [] token;
            if (an)
                an->parse(curline, ckt, which, &line, task);
            else
                logError(curline, "Transient analysis unsupported");
        }
        return (false);
    }

    if (lstring::eq(token, TF_KW)) {
        // .tf v( node1, node2 ) src
        // .tf vsrc2             src
        if (task) {
            IFanalysis *an = getAnalysis(token+1, &which);
            delete [] token;
            if (an)
                an->parse(curline, ckt, which, &line, task);
            else
                logError(curline, "DC Transfer function analysis unsupported");
        }
        return (false);
    }

    if (lstring::eq(token, NOISE_KW)) {
        // .noise V(OUTPUT) SRC {DEC OCT LIN} NP FSTART FSTOP <PTSPRSUM>
        if (task) {
            IFanalysis *an = getAnalysis(token+1, &which);
            delete [] token;
            if (an)
                an->parse(curline, ckt, which, &line, task);
            else
                logError(curline, "Noise analysis unsupported");
        }
        return (false);
    }

    if (lstring::eq(token, SENS_KW)) {
        // .sens <output> [ac [dec|lin|oct] <pts> <low freq> <high freq> | dc ]
        if (task) {
            IFanalysis *an = getAnalysis(token+1, &which);
            delete [] token;
            if (an)
                an->parse(curline, ckt, which, &line, task);
            else
                logError(curline, "Sensitivity unsupported");
        }
        return(false);
    }

    if (lstring::eq(token, PZ_KW)) {
        // .pz nodeI nodeG nodeJ nodeK {V I} {POL ZER PZ}
        if (task) {
            IFanalysis *an = getAnalysis(token+1, &which);
            delete [] token;
            if (an)
                an->parse(curline, ckt, which, &line, task);
            else
                logError(curline, "Pole-zero analysis unsupported.");
        }
        return (false);
    }

    if (lstring::eq(token, DISTO_KW)) {
        // .disto {DEC OCT LIN} NP FSTART FSTOP <F2OVERF1>
        if (task) {
            IFanalysis *an = getAnalysis(token+1, &which);
            delete [] token;
            if (an)
                an->parse(curline, ckt, which, &line, task);
            else
                logError(curline, "Small signal distortion analysis unsupported");
        }
        return (false);
    }

    if (lstring::eq(token, OPTIONS_KW) || lstring::eq(token, OPTION_KW) ||
            lstring::eq(token, OPT_KW)) {
        // .option - specify program options
        delete [] token;
        OPTinfo.parse(curline, ckt, 0, &line, 0);
        return (false);
    }

    if (lstring::eq(token, END_KW)) {
        // .end - end of input
        delete [] token;
        // not allowed to pay attention to additional input - return true
        return (true);
    }

    if (lstring::eq(token, TABLE_KW)) {
        delete [] token;
        tablCheck(curline, ckt);
        return (false);
    }

    if (lstring::eq(token, PARAM_KW)) {
        delete [] token;
        return (false);
    }

    if (lstring::eq(token, TITLE_KW)) {
        delete [] token;
        return (false);
    }

    if (lstring::eq(token, MEAS_KW) || lstring::eq(token, MEASURE_KW)) {
        delete [] token;
        return (false);
    }

    if (lstring::eq(token, STOP_KW)) {
        delete [] token;
        return (false);
    }

    if (lstring::eq(token, CACHE_KW) || lstring::eq(token, ENDCACHE_KW)) {
        delete [] token;
        return (false);
    }

    if (checkDotCard(token) == 1) {
        // HSPICE unhandled

        if (Sp.HspiceFriendly()) {
            delete [] token;
            return (false);
        }
    }

    logError(curline, "Unimplemented control card");
    delete [] token;
    return (false);
}


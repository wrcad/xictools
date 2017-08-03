
/*========================================================================*
 *                                                                        *
 *  Whiteley Research Inc, Sunnyvale, CA USA, http://wrcad.com            *
 *  Copyright (C) 2017 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *  Licensed under the Apache License, Version 2.0 (the "License");       *
 *  you may not use this file except in compliance with the License.      *
 *  You may obtain a copy of the License at                               *
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

#include "frontend.h"
#include "input.h"
#include "device.h"
#include "misc.h"
#include "subexpand.h"


void
SPinput::parseDeck(sCKT *ckt, sLine *deck, sTASK *task, bool pass2_only)
{
    if (ckt && deck) {
        if (pass2_only)
            // parsing an analysis string
            pass2(ckt, deck, task);
        else {
            // parsing the whole bloody thing
            pass1(ckt, deck);
            pass2(ckt, deck, task);
            setModCache(0);
            killMods();
            tablFix(ckt);

            // Provide a default "op" analysis if no other analysis
            // was specified in the deck.
            if (task && !task->TSKjobs) {
                int which;
                if (getAnalysis("op", &which))
                    task->newAnal(which);
            }
        }
    }
    clearMosLevelMaps();
}


// The first pass of the circuit parser just looks
// for '.model' and '.table' lines.
//
void
SPinput::pass1(sCKT *ckt, sLine *deck)
{
    for (sLine *l = deck; l; l = l->next()) {
        // key off of the first character of the line
        const char *thisline = l->line();

        // white space should already be stripped
        while (isspace(*thisline))
            thisline++;

        // export the current line
        ip_current_line = l;

        if (*thisline == '.') {
            // We know that IP.kwMatchInit was called recently.
            if (lstring::cimatch(TABLE_KW, thisline))
                tablParse(l, ckt);
            else if (SPcx.kwMatchModel(thisline))
                parseMod(l);
            if (lstring::cimatch(MOSMAP_KW, thisline)) {

                // Handle .mosmap here.  This is a WRspice feature that
                // allows MOS model level mapping.  Syntax is
                //   .mosmap [ext_level] [wrspice_level]

                char *token = getTok(&thisline, true);  // throw away '.mosmap'
                delete [] token;
                int err;
                double lev = getFloat(&thisline, &err, true);
                if (err != OK)
                    clearMosLevelMaps();
                else {
                    int ext_lev =
                        lev > 0.0 ? (int)(lev + 0.5) : (int)(lev - 0.5);
                    lev = getFloat(&thisline, &err, true);
                    if (err)
                        delMosLevelMapping(ext_lev);
                    else {
                        int wrs_lev =
                            lev > 0.0 ? (int)(lev + 0.5) : (int)(lev - 0.5);
                        addMosLevelMapping(wrs_lev, ext_lev);
                    }
                }
            }
        }
        // for now, we do nothing with the other cards - just 
        // keep them in the list for pass 2 
    }
    ip_current_line = 0;
}


// pass 2 - Scan through the lines.  ".model" cards have been processed
// in pass1 and are ignored here.
//
void
SPinput::pass2(sCKT *ckt, sLine *data, sTASK *task)
{
    char *gname = lstring::copy("0");;
    int error = ckt->gndInsert(&gname, 0);
    if (error && error != E_EXISTS) {
        data->errcat(
    "Internal error: can't insert internal ground node in symbol table!\n");
        ckt->CKTnogo = true;
        ip_current_line = 0;
        return;
    }

    for (sLine *l = data; l; l = l->next()) {
        const char *thisline = l->line();

        // white space should already be stripped
        while (isspace(*thisline))
            thisline++;

        // export the current line
        ip_current_line = l;

        char c = *thisline;

        if (isalpha(c)) {
            int ix = findDev(thisline);
            if (ix >= 0)
                DEV.device(ix)->parse(ix, ckt, l);
            else if (c != 'n' && c != 'N') {
                // feature: the 'n' devices are "null"
                logError(l, "Error: Unknown device type.");
                ckt->CKTnogo = true;
                ip_current_line = 0;
                return;
            }
        }
        else if (c == '.') {
            // .<something> many possibilities
            if (parseDot(ckt, task, l))
                break; // .end found
        }
        else if (c && c != '*')
            // something unexpected
            logError(l, "Syntax error.");
    }
    ip_current_line = 0;
}


// Return the index of the device in line, or -1 if not found.
//
int
SPinput::findDev(const char *line)
{
    int min_terms, max_terms, has_mod;
    numRefs(line, &min_terms, &max_terms, 0, &has_mod);

    // Now find the model name, this will give the correct device index.
    for (int i = 0; i < DEV.numdevs(); i++) {
        IFdevice *dev = DEV.device(i);
        if (!dev)
            continue;
        IFkeys *k = dev->keyMatch(*line);
        if (!k)
            continue;
        if (!dev->modelKey(0)) {
            if (!has_mod)
                return (i);
            continue;
        }

        int tchk = max_terms - min_terms + 1;
        if (tchk < 1)
            tchk = 1;
        int tmin = min_terms + k->numDevs;
        const char *s = line;
        char *tok = IP.getTok(&s, true);
        delete [] tok;  // device name
        for (int j = 0; j < tmin; j++) {
            tok = IP.getTok(&s, true);
            delete [] tok;
        }

        // Now start looking for the model.
        for (int j = 0; j < tchk; j++) {
            tok = IP.getTok(&s, true);
            if (!tok)
                break;
            sINPmodel *m = findMod(tok);
            delete [] tok;
            if (m && m->modType == i)
                return (i);
        }
    }

    // The model name must be bad, return the default instance and
    // deal with the problem later by creating a default model.

    for (int i = 0; i < DEV.numdevs(); i++) {
        IFdevice *dev = DEV.device(i);
        if (!dev || dev->level(0) > 1)
            continue;
        if (dev->keyMatch(*line))
            return (i);
    }
    return (-1);
}
// End of SOinput functions.


// Static function.
// Copy a deck, including the actual lines.
//
sLine *
sLine::copy(const sLine *deck)
{
    sLine *d = 0, *nd = 0;
    while (deck) {
        if (nd) {
            d->li_next = new sLine;
            d = d->li_next;
        }
        else
            nd = d = new sLine;
        d->li_linenum = deck->li_linenum;
        d->li_line = lstring::copy(deck->li_line);
        d->li_error = lstring::copy(deck->li_error);
        d->li_actual = copy(deck->li_actual);
        deck = deck->li_next;
    }
    return (nd);
}


void
sLine::set_line(const char *t)
{
    char *s = lstring::copy(t);
    delete [] li_line;
    li_line = s;
}


void
sLine::append_line(const char *t, bool add_sp)
{
    if (!t)
        return;
    if (!li_line)
        li_line = lstring::copy(t);
    else {
        int len = strlen(li_line) + strlen(t) + 1 + add_sp;
        char *s = new char[len];
        char *e = lstring::stpcpy(s, li_line);
        delete [] li_line;
        li_line = s;
        if (add_sp)
            *e++ = ' ';
        strcpy(e, t);
    }
}


void
sLine::fix_line()
{
    if (li_line) {
        for (char *s = li_line; *s; s++) {
            *s &= 0x7f;
            if (!isspace(*s) && !isprint(*s))
                *s = '_';
        }
    }
}


// Remove (...) around node lists.  This is for convenience in later
// processing.
//
void
sLine::elide_parens()
{
    if (SPcx.kwMatchSubstart(li_line)) {
        char *s = li_line;
        lstring::advtok(&s);
        lstring::advtok(&s);
        while (isspace(*s))
            s++;
        if (*s == '(') {
            while (s[0] && (s[1] != ')')) {
                s[0] = s[1];
                s++;
            }
            while (s[1]) {
                s[0] = s[2];
                s++;
            }
        }
    }
    else if (isalpha(*li_line)) {
        char *s = li_line;
        lstring::advtok(&s);
        while (isspace(*s))
            s++;
        if (*s == '(') {
            while (s[0] && (s[1] != ')')) {
                s[0] = s[1];
                s++;
            }
            while (s[1]) {
                s[0] = s[2];
                s++;
            }
        }
    }
}


void
sLine::set_error(const char *t)
{
    char *s = lstring::copy(t);
    delete [] li_error;
    li_error = s;
}


// Add text to the error line.
//
void
sLine::errcat(const char *text)
{
    if (li_error != 0) {
        if (text == 0 || *text == 0)
            return;
        char *errtmp = new char[strlen(li_error) + strlen(text) + 2];
        char *e = lstring::stpcpy(errtmp, li_error);
        if (*(e-1) != '\n')
            *e++ = '\n';
        e = lstring::stpcpy(e, text);
        if (*(e-1) == '\n')
            *--e = 0;
        delete [] li_error;
        li_error = errtmp;
    }
    else
        li_error = lstring::copy(text);
}


// As above, but assume text is dynamic storage and delete it
// after use.
//
void
sLine::errcat_free(char *text)
{
    if (li_error != 0) {
        if (text == 0)
            return;
        char *errtmp = new char[strlen(li_error) + strlen(text) + 2];
        strcpy(errtmp, li_error);
        strcat(errtmp, "\n");
        strcat(errtmp, text);
        delete [] li_error;
        delete [] text;
        li_error = errtmp;
    }
    else
        li_error = text;
}
// End of sLine functions.


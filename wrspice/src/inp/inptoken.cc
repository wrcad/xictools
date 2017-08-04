
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
Authors: 1985 Thomas L. Quarles
         1992 Stephen R. Whiteley
****************************************************************************/

#include <math.h>
#include "input.h"
#include "circuit.h"
#include "misc.h"
#include "spnumber.h"
#include "errorrec.h"


namespace {
    inline bool issep(int c)
    {
        return (isspace(c) || c == '=' || c == '(' || c == ')' || c == ',');
    }
}


// Get input token from 'line', and return a pointer to it.  If
// gobble, eat non-whitespace trash AFTER token.
//
char *
SPinput::getTok(const char **line, bool gobble)
{
    if (!line || !*line)
        return (0);

    // Scan along throwing away garbage characters.
    const char *point;
    for (point = *line; *point != '\0'; point++) {
        if (!issep(*point))
            break;
    }

    // Mark beginning of token.
    *line = point;

    // Keep quoted entities intact.
    if (*point == '\'' || *point == '"') {
        char q = *point;
        for (point++; *point != '\0'; point++) {
            if (*point == q) {
                point++;
                break;
            }
        }
    }
    else {
        // Find all good characters.
        for (point = *line; *point != '\0'; point++) {
            if (issep(*point))
                break;
        }
    }

    char *token = 0;
    if (point > *line) {
        token = new char[1 + point - *line];
        strncpy(token, *line, point - *line);
        *(token + (point - *line)) = '\0';
    }

    *line = point;

    // Gobble garbage to next token.
    for ( ; **line != '\0'; (*line)++) {
        if (**line == ' ')
            continue;
        if (**line == '\t')
            continue;
        if ((**line == '=') && gobble)
            continue;
        if ((**line == ',') && gobble)
            continue;
        break;
    }
    return (token);
}


// As above, but simply advance the line pointer past the token,
// without saving the token.
//
void
SPinput::advTok(const char **line, bool gobble)
{
    if (!line || !*line)
        return;

    // Scan along throwing away garbage characters.
    const char *point;
    for (point = *line; *point != '\0'; point++) {
        if (!issep(*point))
            break;
    }

    // Mark beginning of token.
    *line = point;

    // Keep quoted entities intact.
    if (*point == '\'' || *point == '"') {
        char q = *point;
        for (point++; *point != '\0'; point++) {
            if (*point == q) {
                point++;
                break;
            }
        }
    }
    else {
        // Find all good characters.
        for (point = *line; *point != '\0'; point++) {
            if (issep(*point))
                break;
        }
    }

    *line = point;

    // Gobble garbage to next token.
    for ( ; **line != '\0'; (*line)++) {
        if (**line == ' ')
            continue;
        if (**line == '\t')
            continue;
        if ((**line == '=') && gobble)
            continue;
        if ((**line == ',') && gobble)
            continue;
        break;
    }
}


// #define DEBUG

// Pass the required data type in data->type, returns true with data->v
// filled in on success, returns false on error.
//
bool
SPinput::getValue(const char **line, IFdata *data, sCKT *ckt,
    const char *xalias)
{
    // Make sure we get rid of extra bits in type.
    int type = data->type & IF_VARTYPES;
    data->v.rValue = 0.0;  // initialize clear

    if (type == IF_INTEGER) {
        int error;
        data->v.iValue = (int)getFloat(line, &error, true);
        if (error)
            return (false);
#ifdef DEBUG
        printf(" returning integer value %d\n", data->v.iValue);
#endif
    }
    else if (type == IF_REAL) {
        int error;
        data->v.rValue = getFloat(line, &error, true);
        if (error)
            return (false);
#ifdef DEBUG
        printf(" returning real value %e\n", data->v.rValue);
#endif
    }
    else if (type == IF_REALVEC) {
        int error;
        const char *tline = *line;
        int cnt = 0;
        for (;;) {
            getFloat(line, &error, true);
            if (error)
                break;
            cnt++;
        }
        if (cnt == 0)
            return (false);
        data->v.v.numValue = cnt;
        *line = tline;
        double *list = new double[cnt];
        data->v.v.vec.rVec = list;
        data->v.v.freeVec = true;
        cnt = 0;
        for (;;) {
            double tmp = getFloat(line, &error, true);
            if (error)
                break;
            list[cnt++] = tmp;
#ifdef DEBUG
            printf(" returning vector value %g\n", tmp);
#endif
        }
    }
    else if (type == IF_FLAG)
        data->v.iValue = 1;
    else if (type == IF_NODE) {
        if (!ckt)
            return (false);
        char *word = getTok(line, true);
        if (!word)
            return (false);
        ckt->termInsert(&word, &data->v.nValue);
    }
    else if (type == IF_INSTANCE) {
        if (!ckt)
            return (false);
        char *word = getTok(line, true);
        if (!word)
            return (false);
        ckt->insert(&word);
        data->v.uValue = word;
    }
    else if (type == IF_STRING) {
        char *word = getTok(line, true);
        if (!word)
            return (false);
        data->v.sValue = word;
    }
    else if (type == IF_PARSETREE) {
        if (!ckt)
            return (false);
        Errs()->init_error();
        data->v.tValue = IFparseTree::getTree(line, ckt, xalias);
        if (!data->v.tValue)
            Errs()->add_error("Expression parse failed.");
        if (Errs()->has_error())
            logError(IP_CUR_LINE, Errs()->get_error());
        if (!data->v.tValue)
            return (false);
    }
    else if (type == IF_TABLEVEC) {
        // This can either be a list of numbers, or a reference to
        // a table.
        const char *tline = *line;
        char *word = getTok(line, true);
        if (!word) {
            // Allow this.
            data->type &= ~IF_VARTYPES;
            data->type |= IF_FLAG;
            data->v.iValue = 1;
            return (true);
        }
        if (lstring::cieq("table", word)) {
            delete [] word;
            word = getTok(line, true);
            if (!word)
                return (false);
            while (**line == ')' || **line == ' ' || **line == '\t')
                (*line)++;
            data->type &= ~IF_VARTYPES;
            data->type |= IF_STRING;
            data->v.sValue = word;
        }
        else {
            delete [] word;
            *line = tline;
            data->type &= ~IF_VARTYPES;
            data->type |= IF_REALVEC;

            int error;
            int cnt = 0;
            for (;;) {
                getFloat(line, &error, true);
                if (error)
                    break;
                cnt++;
            }
            if (cnt == 0)
                return (false);
            data->v.v.numValue = cnt;
            *line = tline;
            double *list = new double[cnt];
            data->v.v.vec.rVec = list;
            data->v.v.freeVec = true;
            cnt = 0;
            for (;;) {
                double tmp = getFloat(line, &error, true);
                if (error)
                    break;
                list[cnt++] = tmp;
#ifdef DEBUG
                printf(" returning vector value %g\n", tmp);
#endif
            }
        }
    }
    else {
        // Don't know what type of parameter caller is talking about!
        return (false);
    }
    return (true);
}


// Set gobble non-zero to gobble rest of token, zero to leave it alone.
// The first character following a string of integers can not be ':',
// since this indicates a subcircuit note.
//
double
SPinput::getFloat(const char **line, int *error, bool gobble)
{
    const char *tmp = *line;
    double *d;
    *error = 0;
    if (gobble) {
        char *token = getTok(&tmp, true);
        if (!token) {
            *error = E_BADPARM;
            return (0.0);
        }
        const char *t = token;
        d = SPnum.parse(&t, false);
        delete [] token;
    }
    else
        d = SPnum.parse(&tmp, false, false);
    if (d) {
        *line = tmp;
        return (*d);
    }
    *error = E_BADPARM;
    return (0.0);
}


// Grab a name and value.  The supported syntax is:
//
//   [ ,(]name[ ][=[ ]value][ ,)]
//
// Either token can be quoted (single or double), quote chars are
// retained.
//
// Return true if a name was obtained.  This is for parsing a .options
// line.
//
bool
SPinput::getOption(const char **line, char **name, char **value)
{
    const char *s = *line;
    if (!s)
        return (false);
    for (;;) {
        while (isspace(*s) || *s == ',' || *s == '(')
            s++;
        if (!*s)
            return (false);
        const char *t = s;
        if (*t == '"' || *t == '\'') {
            char q = *t;
            char *tok = lstring::getqtok(&t);
            char *tmp = new char[3 + (tok ? strlen(tok) : 0)];
            *name = tmp;
            *tmp++ = q;
            if (tok) {
                tmp = lstring::stpcpy(tmp, tok);
                delete [] tok;
            }
            *tmp++ = q;
            *tmp = 0;
        }
        else {
            while (*t && !isspace(*t) && *t != '=' && *t != ',' && *t != ')')
                t++;
            if (t > s) {
                *name = new char[t-s + 1];
                strncpy(*name, s, t-s);
                (*name)[t-s] = 0;
            }
            else {
                // Stray '=' or ')', skip over and ignore.
                s = t+1;
                continue;
            }
        }
        s = t;
        while (isspace(*s))
            s++;
        if (*s != '=') {
            // Single flag token.
            *value = 0;
            while (isspace(*s) || *s == ',' || *s == ')')
                s++;
            *line = s;
            return (true);
        }
        s++;  // Advance over '='.
        while (isspace(*s))
            s++;
        if (!*s) {
            // Single flag token, with stray trailing '='.
            *value = 0;
            *line = s;
            return (true);
        }
        t = s;
        if (*t == '"' || *t == '\'') {
            char q = *t;
            char *tok = lstring::getqtok(&t);
            char *tmp = new char[3 + (tok ? strlen(tok) : 0)];
            *value = tmp;
            *tmp++ = q;
            if (tok) {
                tmp = lstring::stpcpy(tmp, tok);
                delete [] tok;
            }
            *tmp++ = q;
            *tmp = 0;
        }
        else {
            while (*t && !isspace(*t)) {
                // If the first character is ',' or ')' and is
                // followed by another of these chars or white space,
                // accept the single character as the value.  This is
                // a hack for subc_catchar.

                if (*t == ',' || *t == ')') {
                    if (t != s)
                        break;
                    if (!t[1] || isspace(t[1]) || t[1] == ',' || t[1] == ')')
                        t++;
                    break;
                }
                t++;
            }
            if (t > s) {
                *value = new char[t-s + 1];
                strncpy(*value, s, t-s);
                (*value)[t-s] = 0;
            }
            else {
                // Single flag token, with stray trailing '='.
                *value = 0;
            }
        }
        s = t;
        while (isspace(*s) || *s == ',' || *s == ')')
            s++;
        *line = s;
        return (true);
    }
}


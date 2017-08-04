
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
Authors: 1992 Stephen R. Whiteley
****************************************************************************/

#include "input.h"
#include "circuit.h"
#include "misc.h"

// 
// Routines for parsing and checking data tables input by the user.
//


// #define DEBUG

#ifdef DEBUG

namespace {
    void testprnt(sCKT *ckt)
    {
        sCKTtable *table;
        IP.tablFind(0, &table, ckt);

        for (sCKTtable *t = table; t; t = t->next()) {
            printf("%s\n", t->name());
            for (sCKTentry *e = t->entries(); e; e = e->ent_next) {
                switch(e->ent_type) {
                case ENT_REAL:
                    printf("  type=%d val=%g real=%g\n", e->ent_type,
                        e->ent_val, e->ent_data.real);
                    continue;
                case ENT_CPLX:
                    printf("  type=%d val=%g real=%g imag=%g\n", e->ent_type,
                        e->ent_val, e->ent_data.real, e->ent_data.imag);
                    continue;
                case ENT_TABLE:
                    printf("  type=%d val=%g real=%g (table %s)\n",
                        e->ent_type, e->ent_val, e->ent_data.real,
                        e->table() ? e->table()->name() : "(null)");
                    continue;
                case ENT_OMITTED:
                    printf("  type=%d val=%g omitted\n", e->ent_type,
                        e->ent_val);
                    continue;
                default:
                    printf("  type=%d error\n", e->ent_type);
                }
            }
        }
    }
}

#endif


void
sCKTentry::set_table_name(const char *n)
{
    char *s = lstring::copy(n);
    delete [] ent_table_name;
    ent_table_name = s;
}


void
sCKTentry::set_table(sCKTtable *t)
{
    delete [] ent_table_name;
    ent_table_name = 0;
    ent_table = t;
}
// End of sCKTentry functions.


sCKTtable::sCKTtable(const char *n)
{
    tab_name = lstring::copy(n);
    tab_type = CTAB_TRAN;
    tab_entry = 0;
    tab_next = 0;
}


// Free the storage used for tables.
//
sCKTtable::~sCKTtable()
{
    delete [] tab_name;
    sCKTentry *e, *en;
    for (e = tab_entry; e; e = en) {
        en = e->ent_next;
        delete e;
    }
}


// Find the list head of the table named in str which is kept in ckt
// return true if found, false if not found.
//
bool
SPinput::tablFind(const char *str, sCKTtable **table, sCKT *ckt)
{
    if (!ckt)
        return (false);
    if (!str) {
        if (table)
            *table = ckt->CKTtableHead;
        return (true);
    }
    sCKTtable *t;
    for (t = ckt->CKTtableHead; t; t = t->next())
        if (!strcmp(str, t->name()))
            break;

    if (!t)
        return (false);
    if (table)
        *table = t;
    return (true);
}


//
// Remaining functions are private
//

// Parse a line containing a table description of the form
// .table tabname [x1] {v1} x2 {v2} ... xN [{vN}]
// where {vI} can be:
//  a real number, or
//  table table_name.
// If the first x is omitted the first {v} must be a table.
// the last {v} can be omitted.
//
void
SPinput::tablParse(sLine *curline, sCKT *ckt)
{
    const char *line = curline->line();

    // throw out .table
    char *t = getTok(&line, true);
    delete [] t;

    if (!line || !*line)
        return;

    t = getTok(&line, true);
    if (!t)
        return;
    if (tablFind(t, 0, ckt)) {
        // already there - error
        logError(curline, "Table %s already defined (ignored)", t);
        delete [] t;
        return;
    }

    sCKTtable *table = new sCKTtable(t);
    delete [] t;

    const char *zz = line;
    t = getTok(&line, true);
    if (!t) {
        logError(curline, "Table %s s empty", table->name());
        return;
    }
    if (lstring::cieq(t, "dc"))
        table->set_type(CTAB_DC);
    else if (lstring::cieq(t, "ac"))
        table->set_type(CTAB_AC);
    else if (lstring::cieq(t, "tran"))
        table->set_type(CTAB_TRAN);
    else
        line = zz;
    delete [] t;

    sCKTentry *entry = 0;
    while (*line) {
        int error;
        sCKTentry *etmp = new sCKTentry;
        if (table->entries() == 0) {
            entry = etmp;
            table->set_entries(etmp);
            double ord = getFloat(&line, &error, true);
            if (error == 0)
                entry->ent_val = ord;
            else
                entry->ent_val = 0;
        }
        else {
            entry->ent_next = etmp;
            entry = entry->ent_next;
            double ord = getFloat(&line, &error, true);
            if (error == 0)
                entry->ent_val = ord;
            else {
                t = lstring::gettok(&line);
                logError(curline, "Syntax error: expected value, got %s", t);
                delete [] t;
                return;
            }
        }
        if (*line == 0) {
            // last entry omitted
            entry->ent_type = ENT_OMITTED;
            break;
        }

        double val = getFloat(&line, &error, true);
        if (error == 0) {
            // entry is a number
            entry->ent_data.real = val;
            if (table->type() == CTAB_AC) {
                val = getFloat(&line, &error, true);
                if (error) {
                    logError(curline,
                        "Syntax error: expected complex value, got real");
                    return;
                }
                entry->ent_data.imag = val;
                entry->ent_type = ENT_CPLX;
            }
            else
                entry->ent_type = ENT_REAL;
            continue;
        }
        t = getTok(&line, true);
        if (!t) {
            logError(curline, "Syntax error: missing entry");
            return;
        }

        if (lstring::cieq(t, "table")) {
            // table specified
            delete [] t;
            entry->ent_type = ENT_TABLE;
            t = getTok(&line, true);
            if (!t) {
                logError(curline, "Syntax error: missing table name");
                return;
            }
            entry->set_table_name(t);
            delete [] t;
            continue;
        }
        // uh-oh
        logError(curline, "Syntax error: unknown keyword %s", t);
        delete [] t;
        return;
    }
    table->set_next(ckt->CKTtableHead);
    ckt->CKTtableHead = table;
}


// Check a .table line for sub-table existence.  Add an error message if
// an undefined table reference is found.  A syntax error will return
// immediately, such errors are already flagged.
//
// If no syntax errors, fix up the corresponding table structure so that
// sub-table references are pointers to the sub-table, not to the name.
// char *line; the .table line to check (".table" stripped)
//
void
SPinput::tablCheck(sLine *curline, sCKT *ckt)
{
    const char *line = curline->line();

    // throw out .table
    char *t = getTok(&line, true);
    delete [] t;

    t = getTok(&line, true); // table name
    if (!t)
        return;

    sCKTtable *table = 0;
    tablFind(0, &table, ckt);
    if (table == 0) {
        // no tables saved (syntax error)
        delete [] t;
        return;
    }

    sCKTtable *tb;
    for (tb = table; tb; tb = tb->next())
        if (lstring::eq(t, tb->name()))
            break;

    delete [] t;
    if (tb == 0) // can't find own symbol (syntax error)
        return;

    const char *zz = line;
    t = getTok(&line, true);
    if (!t)
        return;
    if (!lstring::cieq(t, "dc") && !lstring::cieq(t, "ac") &&
            !lstring::cieq(t, "tran"))
        line = zz;
    delete [] t;

    while (*line) {
        int error;
        getFloat(&line, &error, true);
        if (*line == '\0')  // last entry was omitted
            break;
        getFloat(&line, &error, true);
        if (error == 0) {
            if (tb->type() == CTAB_AC) {
                getFloat(&line, &error, true);
                if (error)
                    return;
            }
            continue; // entry is a real
        }
        t = getTok(&line, true);
        if (!t)
            return;
        if (lstring::cieq(t, "table")) {
            // table specified
            delete [] t;
            t = getTok(&line, true); // table reference
            if (!t)
                return;
            sCKTtable *stab;
            bool bad = false;
            if (!tablFind(t, &stab, ckt)) {
                logError(curline, "Unresolved table reference: %s", t);
                bad = true;
            }
            else if (stab->type() != tb->type()) {
                logError(curline, "Table reference to different type: %s", t);
                bad = true;
            }
            if (bad) {
                // find the reference and null it
                for (sCKTentry *e = tb->entries(); e; e = e->ent_next) {
                    if (e->ent_type != ENT_TABLE)
                        continue;
                    if (e->table_name() && lstring::eq(e->table_name(), t)) {
                        e->set_table(0);
                        break;
                    }
                }
            }
            delete [] t;
            continue;
        }
        return;  // syntax error
    }
    // No syntax errors.
    // Replace known table name references with pointers to tables.

    for (sCKTentry *e = tb->entries(); e; e = e->ent_next) {
        if (e->ent_type == ENT_TABLE && e->table_name() != 0) {
            for (sCKTtable *tt = table; tt; tt = tt->next()) {
                if (lstring::eq(e->table_name(), tt->name())) {
                    // Replace with pointer to table.
                    e->set_table(tt);
                    break;
                }
            }
        }
    }
}


// Fix the actual values for the omitted final entries, and
// set the ent_data components to the function or table evaluated
// with the ent_val component.
//
void
SPinput::tablFix(sCKT *ckt)
{
    if (ckt) {
        for (sCKTtable *t = ckt->CKTtableHead; t; t = t->next())
            t->tablfix();
    }

#ifdef DEBUG
    testprnt(ckt);
#endif
}


IFcomplex
sCKTtable::tablEval(double x)
{
    sCKTentry *e = tab_entry, *ee;
    if (x <= e->ent_val)
        return (e->ent_data);
    for ( ; e; e = ee) {
        ee = e->ent_next;
        if (ee == 0) break;
        if (x >= e->ent_val && x < ee->ent_val) {
            if (e->ent_type == ENT_TABLE) {
                if (e->table())
                    return (e->table()->tablEval(x));
                return (e->ent_data);
            }
            else {
                IFcomplex c;
                c.real = e->ent_data.real + (x - e->ent_val)*
                    (ee->ent_data.real - e->ent_data.real)/
                    (ee->ent_val - e->ent_val);
                if (tab_type == CTAB_AC)
                    c.imag = e->ent_data.imag + (x - e->ent_val)*
                        (ee->ent_data.imag - e->ent_data.imag)/
                        (ee->ent_val - e->ent_val);
                else
                    c.imag = 0.0;
                return (c);
            }
        }
    }
    if (e->ent_type == ENT_TABLE && e->table())
        return (e->table()->tablEval(x));
    return (e->ent_data);
}


double
sCKTtable::tablEvalDeriv(double x)
{
    if (tab_type == CTAB_AC)
        return (0.0);
    sCKTentry *e = tab_entry, *ee;
    if (x <= e->ent_val)
        return (0);
    for ( ; e; e = ee) {
        ee = e->ent_next;
        if (ee == 0)
            break;
        if (x >= e->ent_val && x < ee->ent_val) {
            if (e->ent_type == ENT_TABLE) {
                if (e->table())
                    return (e->table()->tablEvalDeriv(x));
                return (0.0);
            }
            else
                return ((ee->ent_data.real - e->ent_data.real)/
                    (ee->ent_val - e->ent_val));
        }
    }
    return (0);
}


// If the final entry argument is omitted, the final entry will be
// taken as a number equal to the previous entry evaluated at the
// final value.  The ent_data.real component of each function or
// table entry is set to the function or table evaluated at the
// entry value.
//
void
sCKTtable::tablfix()
{
    sCKTentry *e, *ee;
    for (e = tab_entry; e; e = ee) {
        ee = e->ent_next;

        if (ee && ee->ent_type == ENT_OMITTED) {
            if (e->ent_type == ENT_REAL || e->ent_type == ENT_CPLX)
                ee->ent_data = e->ent_data;
            else if (e->ent_type == ENT_TABLE) {
                if (e->table()) {
                    e->table()->tablfix();
                    ee->ent_data = e->table()->tablEval(ee->ent_val);
                }
            }
            ee->ent_type = (tab_type == CTAB_AC ? ENT_CPLX : ENT_REAL);
            break;
        }

        if (e->ent_type == ENT_REAL || e->ent_type == ENT_CPLX)
            continue;
        if (e->ent_type == ENT_TABLE) {
            if (e->table()) {
                e->table()->tablfix();
                e->ent_data = e->table()->tablEval(ee->ent_val);
            }
        }
    }
}


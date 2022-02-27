
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
Authors: 1986 Wayne A. Christopher
         1993 Stephen R. Whiteley
****************************************************************************/

#include "simulator.h"
#include "commands.h"
#include "datavec.h"
#include "output.h"
#include "ttyio.h"
#include "errors.h"
#include "wlist.h"
#include "ginterf/graphics.h"


//
// Stuff to deal with "types" for vectors and plots.
//

// Abbreviations for vector types.
struct typeab
{
    typeab()
        {
            t_type = UU_NOTYPE;
            t_name = 0;
            t_abbrev = 0;
        }

    Utype t_type;
    const char *t_name;
    const char *t_abbrev;
    sUnits t_units;
};

// Abbreviations for plot names.
struct plotab
{
    plotab()
        {
            p_name = 0;
            p_pattern = 0;
        }

    const char *p_name;
    const char *p_pattern;
};

struct Abbrev
{
    Abbrev();
    void listTypes();
    void addPlotAb(const char*, const char*);
    void addTypeAb(int, const char*, const char*);
    typeab *findTypeAb(const char*);
    typeab *findTypeAb(int);
    typeab *findTypeAbMatch(const char*);
    plotab *findPlotAb(const char*);
    plotab *findPlotAbMatch(const char*);

    friend char *sUnits::unitstr();

private:
    plotab *plotAbs;
    typeab *typeAbs;
    int numPlotAbs;
    int numTypeAbs;
    int tsize;
    int psize;
};

namespace { Abbrev Ab; }


// A command to define types for vectors and plots.  This will generally
// be used in the Command: field of the rawfile.
// The syntax is "deftype v typename abbrev", where abbrev will be used to
// parse things like abbrev(name) and to label axes with M<abbrev>, instead
// of numbers. It may be ommitted.
// Also, the command "deftype p plottype pattern ..." will assign plottype as
// the name to any plot with one of the patterns in its Name: field.
//
void
CommandTab::com_deftype(wordlist *wl)
{
    static int utypes = 100;
    char c = *wl->wl_word;
    if ((c == 'v' || c == 'V') &&
            (!wl->wl_word[1] || isspace(wl->wl_word[1]))) {
        wl = wl->wl_next;
        if (!wl) {
            GRpkgIf()->ErrPrintf(ET_ERROR, "missing name argument.\n");
            return;
        }
        char *name = wl->wl_word;
        wl = wl->wl_next;
        char *abb;
        if (!wl)
            abb = 0;
        else
            abb = wl->wl_word;
        typeab *t = Ab.findTypeAb(name);
        if (t) {
            delete [] t->t_abbrev;
            t->t_abbrev = lstring::copy(abb);
        }
        else
            Ab.addTypeAb(utypes++, name, abb);
    }
    else if ((c == 'p' || c == 'P') &&
            (!wl->wl_word[1] || isspace(wl->wl_word[1]))) {
        wl = wl->wl_next;
        if (!wl) {
            GRpkgIf()->ErrPrintf(ET_ERROR, "missing name argument.\n");
            return;
        }
        char *name = wl->wl_word;
        wl = wl->wl_next;
        if (!wl) {
            GRpkgIf()->ErrPrintf(ET_ERROR, "missing pattern argument.\n");
            return;
        }
        while (wl) {
            plotab *p = Ab.findPlotAb(wl->wl_word);
            if (p) {
                delete [] p->p_name;
                p->p_name = lstring::copy(name);
            }
            else
                Ab.addPlotAb(name, wl->wl_word);
        }
    }
    else
        GRpkgIf()->ErrPrintf(ET_ERROR, "missing 'p' or 'v' argument.\n");
}


// Change the type of a vector.
//
void
CommandTab::com_settype(wordlist *wl)
{
    if (!wl) {
        Ab.listTypes();
        return;
    }
    char *tpname = wl->wl_word;
    sUnits u;
    typeab *t = Ab.findTypeAb(tpname);
    if (t)
        u.set(t->t_type);
    else {
        if (!u.set(tpname)) {
            GRpkgIf()->ErrPrintf(ET_ERROR,
                "could not recognize type or units in '%s'.\n", tpname);
            return;
        }
    }
    for (wl = wl->wl_next; wl; wl = wl->wl_next) {
        // We only want permanent vectors.
        sDataVec *v = OP.vecGet(wl->wl_word, 0);
        if (!v || !(v->flags() & VF_PERMANENT))
            Sp.Error(E_NOVEC, 0, wl->wl_word);
        else {
            if (v->link())
                for (sDvList *dl = v->link(); dl; dl = dl->dl_next)
                    *dl->dl_dvec->units() = u;
            else
                *v->units() = u;
        }
    }
}
// End of CommandTab functions.


// Return the abbreviation associated with a number.
//
const char *
IFsimulator::TypeAbbrev(int typenum)
{
    typeab *t = Ab.findTypeAb(typenum);
    if (t)
        return (t->t_abbrev);
    return (0);
}


// Return the typename associated with a number.
//
const char *
IFsimulator::TypeNames(int typenum)
{
    typeab *t = Ab.findTypeAb(typenum);
    if (t)
        return (t->t_name);
    return (0);
}


// Return the type number associated with the name.
//
int
IFsimulator::TypeNum(const char *name)
{
    if (lstring::eq(name, "none"))
        name = "notype";
    typeab *t = Ab.findTypeAb(name);
    if (t)
        return (t->t_type);
    return (0);
}


// For plots...
//
const char *
IFsimulator::PlotAbbrev(const char *string)
{
    plotab *p = Ab.findPlotAbMatch(string);
    if (p)
        return (p->p_name);
    return (0);
}


Abbrev::Abbrev()
{
    plotAbs = 0;
    typeAbs = 0;
    numPlotAbs = 0;
    numTypeAbs = 0;
    tsize = 0;
    psize = 0;

    // default type abbreviations
    addTypeAb( UU_NOTYPE, "notype", 0 );
    addTypeAb( UU_TIME, "time", "S" );
    addTypeAb( UU_FREQUENCY, "frequency", "Hz" );
    addTypeAb( UU_VOLTAGE, "voltage", "V" );
    addTypeAb( UU_CURRENT, "current", "A" );
    addTypeAb( UU_CHARGE, "charge", "Cl" );
    addTypeAb( UU_FLUX, "flux", "Wb" );
    addTypeAb( UU_CAP, "capacitance", "F" );
    addTypeAb( UU_IND, "inductance", "H" );
    addTypeAb( UU_RES, "resistance", "O" );
    addTypeAb( UU_COND, "conductance", "Si" );
    addTypeAb( UU_LEN, "length", "M" );
    addTypeAb( UU_AREA, "area", "M2" );
    addTypeAb( UU_TEMP, "temperature", "C" );
    addTypeAb( UU_POWER, "power", "W" );

    // The above are the basic types, which must come first.  The
    // define below is the number of basic types.  The "notype" entry
    // must be first.
#define NUM_BTYPES 15

    addTypeAb( UU_POLE, "pole", 0 );
    addTypeAb( UU_ZERO, "zero", 0 );
    addTypeAb( UU_SPARAM, "s-param", 0 );

    // default plot abbreviations
    addPlotAb( "tran", "transient" );
    addPlotAb( "tran", "tran" );
    addPlotAb( "op", "op" );
    addPlotAb( "exec", "exec" );
    addPlotAb( "tf", "function" );
    addPlotAb( "tf", "tf" );
    addPlotAb( "dc", "d.c." );
    addPlotAb( "dc", "dc" );
    addPlotAb( "dc", "transfer" );
    addPlotAb( "ac", "a.c." );
    addPlotAb( "ac", "ac" );
    addPlotAb( "pz", "pz" );
    addPlotAb( "pz", "p.z." );
    addPlotAb( "pz", "pole-zero");
    addPlotAb( "disto", "disto" );
    addPlotAb( "dist", "dist" );
    addPlotAb( "noise", "noise" );
    addPlotAb( "sens", "sens" );
    addPlotAb( "sens", "sensitivity" );
    addPlotAb( "sp", "s.p." );
    addPlotAb( "sp", "sp" );
    addPlotAb( "harm", "harm" );
    addPlotAb( "spect", "spect" );
    addPlotAb( "range", "range" );
    addPlotAb( "test", "testrun" );
}


void
Abbrev::listTypes()
{
    const char *format = "  %-28s%s\n";
    TTY.init_more();
    TTY.printf(format+2, "Type", "Abbreviation");
    for (int i = 0; i < numTypeAbs; i++)
        TTY.printf(format, typeAbs[i].t_name, typeAbs[i].t_abbrev ?
            typeAbs[i].t_abbrev : "");
}


void
Abbrev::addTypeAb(int type, const char *name, const char *abbrev)
{
    if (!typeAbs) {
        typeAbs = new typeab[8];
        tsize = 8;
        numTypeAbs = 0;
    }
    if (numTypeAbs >= tsize) {
        Realloc(&typeAbs, tsize*2, tsize);
        tsize *= 2;
    }
    typeAbs[numTypeAbs].t_type = (Utype)type;
    typeAbs[numTypeAbs].t_name = lstring::copy(name);
    typeAbs[numTypeAbs].t_abbrev = lstring::copy(abbrev);
    typeAbs[numTypeAbs].t_units.set(type);
    numTypeAbs++;
}


void
Abbrev::addPlotAb(const char *name, const char *pattern)
{
    if (!plotAbs) {
        plotAbs = new plotab[8];
        psize = 8;
        numPlotAbs = 0;
    }
    if (numPlotAbs >= psize) {
        Realloc(&plotAbs, psize*2, psize);
        psize *= 2;
    }
    plotAbs[numPlotAbs].p_name = lstring::copy(name);
    plotAbs[numPlotAbs].p_pattern = lstring::copy(pattern);
    numPlotAbs++;
}


typeab *
Abbrev::findTypeAb(const char *name)
{
    for (typeab *t = typeAbs; t->t_name; t++) {
        if (lstring::cieq(t->t_name, name))
            return (t);
    }
    return (0);
}


typeab *
Abbrev::findTypeAb(int type)
{
    for (typeab *t = typeAbs; t->t_name; t++) {
        if (t->t_type == type)
            return (t);
    }
    return (0);
}


typeab *
Abbrev::findTypeAbMatch(const char *ab)
{
    for (typeab *t = typeAbs; t->t_name; t++) {
        if (lstring::cieq(t->t_abbrev, ab))
            return (t);
    }
    return (0);
}


plotab *
Abbrev::findPlotAb(const char *pattern)
{
    for (plotab *p = plotAbs; p->p_pattern; p++) {
        if(lstring::cieq(p->p_pattern, pattern))
            return (p);
    }
    return (0);
}


plotab *
Abbrev::findPlotAbMatch(const char *cstring)
{
    if (!cstring)
        return (0);
    char *string = new char[strlen(cstring) + 1];
    char *s = string;
    for (const char *t = cstring; *t; t++)
        *s++ = isupper(*t) ? tolower(*t) : *t;
    *s = 0;

    s = string;
    char *tok;
    while ((tok = lstring::gettok(&s)) != 0) {
        for (plotab *p = plotAbs; p->p_pattern; p++) {
            if (lstring::substring(p->p_pattern, tok)) {
                delete [] string;
                delete [] tok;
                return (p);
            }
        }
        delete [] tok;
    }
    delete [] string;
    return (0);
}


//
// Functions to keep track of vector units.
//

// Set fundamental units.
//
void
sUnits::set(int type)
{
    for (int i = 0; i < 8; i++)
        units[i] = 0;
    switch (type) {
    default:
    case UU_NOTYPE:
        return;
    case UU_TIME:
        units[uuTime] = 1;
        return;
    case UU_FREQUENCY:
        units[uuTime] = -1;
        return;
    case UU_VOLTAGE:
        units[uuVoltage] = 1;
        return;
    case UU_CURRENT:
        units[uuCurrent] = 1;
        return;
    case UU_CHARGE:
        units[uuCurrent] = 1;
        units[uuTime] = 1;
        return;
    case UU_FLUX:
        units[uuVoltage] = 1;
        units[uuTime] = 1;
        return;
    case UU_CAP:
        units[uuCurrent] = 1;
        units[uuTime] = 1;
        units[uuVoltage] = -1;
        return;
    case UU_IND:
        units[uuVoltage] = 1;
        units[uuTime] = 1;
        units[uuCurrent] = -1;
        return;
    case UU_RES:
        units[uuVoltage] = 1;
        units[uuCurrent] = -1;
        return;
    case UU_COND:
        units[uuCurrent] = 1;
        units[uuVoltage] = -1;
        return;
    case UU_LEN:
        units[uuLength] = 1;
        return;
    case UU_AREA:
        units[uuLength] = 2;
        return;
    case UU_TEMP:
        units[uuTemper] = 1;
        return;
    case UU_POWER:
        units[uuVoltage] = 1;
        units[uuCurrent] = 1;
        return;
    }
}


// Set fundamental units.
//
void
sUnits::set(IFparm *p)
{
    for (int i = 0; i < 8; i++)
        units[i] = 0;
    if (!p)
        return;
    switch (p->dataType & IF_UNITS) {
    default:
    case 0:
        return;
    case IF_TIME:
        units[uuTime] = 1;
        return;
    case IF_FREQ:
        units[uuTime] = -1;
        return;
    case IF_VOLT:
        units[uuVoltage] = 1;
        return;
    case IF_AMP:
        units[uuCurrent] = 1;
        return;
    case IF_CHARGE:
        units[uuCurrent] = 1;
        units[uuTime] = 1;
        return;
    case IF_FLUX:
        units[uuVoltage] = 1;
        units[uuTime] = 1;
        return;
    case IF_CAP:
        units[uuCurrent] = 1;
        units[uuTime] = 1;
        units[uuVoltage] = -1;
        return;
    case IF_IND:
        units[uuVoltage] = 1;
        units[uuTime] = 1;
        units[uuCurrent] = -1;
        return;
    case IF_RES:
        units[uuVoltage] = 1;
        units[uuCurrent] = -1;
        return;
    case IF_COND:
        units[uuCurrent] = 1;
        units[uuVoltage] = -1;
        return;
    case IF_LEN:
        units[uuLength] = 1;
        return;
    case IF_AREA:
        units[uuLength] = 2;
        return;
    case IF_TEMP:
        units[uuTemper] = 1;
        return;
    case IF_POWR:
        units[uuVoltage] = 1;
        units[uuCurrent] = 1;
        return;
    }
}


// Parse the type abbreviations in string and set the units accordingly.
// Return false if error.
//
bool
sUnits::set(const char *string)
{
    for (int i = 0; i < 8; i++)
        units[i] = 0;
    if (!string || !*string)
        return (true);  // ok, notype
    sUnits u = *this;
    char buf[128];
    strcpy(buf, string);
    char *s1 = buf;
    char *s2 = strchr(buf, Sp.UnitsSepchar());
    if (s2)
        *s2++ = 0;
    char ab[8];

#define MAXABLEN 2

    while (*s1) {
        int n;
        for (n = MAXABLEN; n > 0; n--) {
            strncpy(ab, s1, n);
            ab[n] = 0;
            typeab *t = Ab.findTypeAbMatch(ab);
            if (t) {
                s1 += strlen(ab);
                int nn = 1;
                if (isdigit(*s1)) {
                    nn = *s1 - '0';
                    s1++;
                }
                while (nn--)
                    u*t->t_units;
                break;
            }
        }
        if (n == 0)
            return (false);
    }
    if (s2) {
        while (*s2) {
            int n;
            for (n = MAXABLEN; n > 0; n--) {
                strncpy(ab, s2, n);
                ab[n] = 0;
                typeab *t = Ab.findTypeAbMatch(ab);
                if (t) {
                    s2 += strlen(ab);
                    int nn = 1;
                    if (isdigit(*s2)) {
                        nn = *s2 - '0';
                        s2++;
                    }
                    while (nn--)
                        u/t->t_units;
                    break;
                }
            }
            if (n == 0)
                return (false);
        }
    }
    *this = u;
    return (true);
}


bool
sUnits::operator==(Utype type)
{
    typeab *t = Ab.findTypeAb(type);
    if (!t)
        return (false);
    for (int i = 0; i < 8; i++)
        if (units[i] != t->t_units.units[i])
            return (false);
    return (true);
}


// Return a string containing the units.  First simplify by using the
// less fundamental types.
//
char *
sUnits::unitstr()
{
    sUnits u = *this;
    char xx[NUM_BTYPES+1];
    int i;
    for (i = 0; i <= NUM_BTYPES; i++)
        xx[i] = 0;

    // The strategy is to find the basic type which removes the largest
    // number of fundamental types.  Then, pull it out, save it, and
    // repeat until no fundamental types remain.
    //
    while (!u.isnotype()) {
        int ind = 0, cnt0 = 0, sign = 0;
        for (i = 1; i < NUM_BTYPES; i++) {
            // iterating over basic types (includes fundamental types)
            int cnt1 = 0, cnt2 = 0;
            u.trial(Ab.typeAbs[i].t_units, &cnt1, false);
            u.trial(Ab.typeAbs[i].t_units, &cnt2, true);
            if (!cnt1 && !cnt2)
                continue;
            if (cnt1 >= cnt2) {
                if (cnt1 > cnt0) {
                    cnt0 = cnt1;
                    sign = 1;
                    ind = i;
                }
            }
            else {
                if (cnt2 > cnt0) {
                    cnt0 = cnt2;
                    sign = -1;
                    ind = i;
                }
            }
        }
        if (!ind)
            // 'can't happen'
            return (0);

        // ind is the basic type index which removes most fundamental
        // types, now reduce u and inc the counter for ind.
        if (sign > 0) {
            for (int j = 0; j < 8; j++)
                u.units[j] -= Ab.typeAbs[ind].t_units.units[j];
            xx[ind]++;
        }
        else {
            for (int j = 0; j < 8; j++)
                u.units[j] += Ab.typeAbs[ind].t_units.units[j];
            xx[ind]--;
        }
    }
    // now create the string
    char buf[128];
    char *s = buf;
    *s = 0;
    bool isdenom = false;
    for (i = 1; i < NUM_BTYPES; i++) {
        if (xx[i] > 0) {
            strcpy(s, Ab.typeAbs[i].t_abbrev);
            while (*s)
                s++;
            if (xx[i] > 1) {
                // add exponent
                int j = xx[i];
                while (j) {
                    *s++ = '0' + j%10;
                    j /= 10;
                }
                *s = 0;
            }
        }
        else if (xx[i] < 0)
            isdenom = true;
    }
    if (isdenom) {
        *s++ = Sp.UnitsSepchar();
        *s = 0;
        for (i = 1; i < NUM_BTYPES; i++) {
            if (xx[i] < 0) {
                strcpy(s, Ab.typeAbs[i].t_abbrev);
                while (*s)
                    s++;
                if (xx[i] < -1) {
                    // add exponent
                    int j = -xx[i];
                    while (j) {
                        *s++ = '0' + j%10;
                        j /= 10;
                    }
                    *s = 0;
                }
            }
        }
    }
    if (*buf) {
        if (buf[0] == Sp.UnitsSepchar() && buf[1] == 'S')
            return (lstring::copy("Hz"));
        if (buf[0] == Sp.UnitsSepchar() && buf[1] == 'O')
            return (lstring::copy("Si"));
        return (lstring::copy(buf));
    }
    return (lstring::copy(""));
}


// See if we can remove datatype u.  If s0, return true, and set cnt to
// the net change in fundamental type population that would occur.
// If denom is false, remove from numerator, else remove from denominator.
//
void
sUnits::trial(sUnits &u, int *cnt, bool denom)
{
    int tcnt = 0;
    for (int i = 0; i < 8; i++) {
        if (abs(u.units[i]) > abs(units[i]))
            return;
        if (!denom) {
            if ((u.units[i] < 0 && units[i] > 0) ||
                    (u.units[i] > 0 && units[i] < 0))
                return;
        }
        else {
            if ((u.units[i] > 0 && units[i] > 0) ||
                    (u.units[i] < 0 && units[i] < 0))
                return;
        }
        tcnt += abs(u.units[i]);
    }
    *cnt = tcnt;
}


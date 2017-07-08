
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2010 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *  WHITELEY RESEARCH INCORPORATED PROPRIETARY SOFTWARE                   *
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
 *                                                                        *
 * XIC Integrated Circuit Layout and Schematic Editor                     *
 *                                                                        *
 *========================================================================*
 $Id: pcell_params.cc,v 1.7 2015/06/11 05:54:07 stevew Exp $
 *========================================================================*/

#include "cd.h"
#include "pcell_params.h"
#include "spnumber.h"
#include "si_parsenode.h"
#include "si_parser.h"
#include "si_interp.h"
#include "encode.h"
#include "errorlog.h"
#include <algorithm>


// Equality operator for PCellParam lists.  Lists are equal if there
// is a 1-1 correspondence between properties, and all values match.
//
bool operator==(const PCellParam &p1, const PCellParam &p2)
{
    int n1 = 0;
    for (const PCellParam *p = &p1; p; p = p->next())
        n1++;
    int n2 = 0;
    for (const PCellParam *p = &p2; p; p = p->next())
        n2++;
    if (n1 != n2)
        return (false);

    const PCellParam *p2last = &p2;
    for (const PCellParam *p = &p1; p; p = p->next()) {
        if (!p2last || strcmp(p->name(), p2last->name())) {
            for (p2last = &p2; p2last; p2last->next()) {
                if (!strcmp(p->name(), p2last->name()))
                    break;
            }
            if (!p2last)
                return (false);
        }
        if (p->type() != p2last->type())
            return (false);

        switch (p->type()) {
        case PCPbool:
            if (p->boolVal() != p2last->boolVal())
                return (false);
            break;
        case PCPint:
            if (p->intVal() != p2last->intVal())
                return (false);
            break;
        case PCPtime:
            if (p->timeVal() != p2last->timeVal())
                return (false);
            break;
        case PCPfloat:
            if (p->floatVal() != p2last->floatVal())
                return (false);
            break;
        case PCPdouble:
            if (p->doubleVal() != p2last->doubleVal())
                return (false);
            break;
        case PCPstring:
            {
                const char *s1 = p->stringVal();
                if (!s1)
                    s1 = "";
                const char *s2 = p2last->stringVal();
                if (!s2)
                    s2 = "";
                if (strcmp(s1, s2))
                    return (false);
            }
            break;
        case PCPappType:
            {
                if (p->appValSize() != p2last->appValSize())
                    return (false);
                unsigned int sz = p->appValSize();
                if (sz && memcmp(p->appVal(), p2last->appVal(), sz))
                    return (false);
            }
            break;
        }
        p2last = p2last->next();
    }
    return (true);
}


namespace {
    bool boolval(const char *str)
    {
        if (isdigit(*str))
            return (atoi(str));
        return (lstring::cieq(str, "true") || lstring::cieq(str, "on"));
    }

    long longval(const char *str)
    {
        return (strtol(str, 0, 10));
    }

    float floatval(const char *str)
    {
        return (strtof(str, 0));
    }

    double doubleval(const char *str)
    {
        return (strtod(str, 0));
    }
}


PCellParam::PCellParam(char typ, const char *nam, const char *val,
     const char *constr)
{
    switch (typ) {
    case 'b':
    case 'B':
        init(PCPbool, nam, constr);
        u.ival = boolval(val);
        break;
    case 'i':
    case 'I':
        init(PCPint, nam, constr);
        u.ival = longval(val);
        break;
    case 't':
    case 'T':
        init(PCPtime, nam, constr);
        u.ival = longval(val);
        break;
    case 'f':
    case 'F':
        init(PCPfloat, nam, constr);
        u.fval = floatval(val);
        break;
    case 'd':
    case 'D':
        init(PCPdouble, nam, constr);
        u.dval = doubleval(val);
        break;
    case 'a':
    case 'A':
        init(PCPappType, nam, constr);
        p_appsize = 0;
        u.aval = 0;
        break;
    default:
        init(PCPstring, nam, constr);
        u.sval = lstring::copy(val);
        break;
    }
}


// Return a text token naming the type.
//
const char *
PCellParam::typestr()
{
    switch (type()) {
    case PCPbool:
        return ("bool");
    case PCPint:
        return ("int");
    case PCPtime:
        return ("time");
    case PCPfloat:
        return ("float");
    case PCPdouble:
        return ("double");
    case PCPstring:
        return ("string");
    case PCPappType:
        return ("apptype");
        break;
    }
    // not reached
    return (0);
}


// Set a single parameter from p.  Try to coerce a type if it seems
// reasonable.  Only set the variable if the new value differs, to
// avoid unnecessarily setting the changed flag.
//
// Return true if the data types are compatible, false otherwise.
//
bool
PCellParam::set(const PCellParam *p)
{
    const char *s;
    switch (type()) {
    case PCPbool:
        {
            bool bv;
            switch (p->type()) {
            case PCPbool:
                if (boolVal() != p->boolVal())
                    setBoolVal(p->boolVal());
                return (true);
            case PCPint:
                bv = (p->intVal() != 0);
                if (boolVal() != bv)
                    setBoolVal(bv);
                return (true);
            case PCPtime:
                bv = (p->timeVal() != 0);
                if (boolVal() != bv)
                    setBoolVal(bv);
                return (true);
            case PCPfloat:
                bv = (p->floatVal() != 0.0);
                if (boolVal() != bv)
                    setBoolVal(bv);
                return (true);
            case PCPdouble:
                bv = (p->doubleVal() != 0.0);
                if (boolVal() != bv)
                    setBoolVal(bv);
                return (true);
            case PCPstring:
                s = p->stringVal();
                if (s) {
                    int v = -1;
                    if  (!strcasecmp(s, "1") || !strcasecmp(s, "t") ||
                            !strcasecmp(s, "true") || !strcasecmp(s, "y") ||
                            !strcasecmp(s, "yes") || !strcasecmp(s, "on"))
                        v = 1;
                    else if (!strcasecmp(s, "0") || !strcasecmp(s, "n") ||
                            !strcasecmp(s, "no") || !strcasecmp(s, "nil") ||
                            !strcasecmp(s, "f") || !strcasecmp(s, "false") ||
                            !strcasecmp(s, "off"))
                        v = 0;
                    if (v >= 0) {
                        bv = v;
                        if (boolVal() != bv)
                            setBoolVal(bv);
                        return (true);
                    }
                }
                break;
            case PCPappType:
                break;
            }
        }
        return (false);

    case PCPint:
        {
            long iv;
            switch (p->type()) {
            case PCPbool:
                iv = p->boolVal();
                if (intVal() != iv)
                    setIntVal(iv);
                return (true);
            case PCPint:
                if (intVal() != p->intVal())
                    setIntVal(p->intVal());
                return (true);
            case PCPtime:
                iv = p->timeVal();
                if (intVal() != iv)
                    setIntVal(iv);
                return (true);
            case PCPfloat:
                iv = mmRndL(p->floatVal());
                if (intVal() != iv)
                    setIntVal(iv);
                return (true);
            case PCPdouble:
                iv = mmRndL(p->doubleVal());
                if (intVal() != iv)
                    setIntVal(iv);
                return (true);
            case PCPstring:
                s = p->stringVal();
                if (s) {
                    double *d = SPnum.parse(&s, false);
                    if (d) {
                        iv = mmRndL(*d);
                        if (intVal() != iv)
                            setIntVal(iv);
                        return (true);
                    }
                }
                break;
            case PCPappType:
                break;
            }
        }
        return (false);

    case PCPtime:
        {
            long tv;
            switch (p->type()) {
            case PCPbool:
                tv = p->boolVal();
                if (timeVal() != tv)
                    setTimeVal(tv);
                return (true);
            case PCPint:
                tv = p->intVal();
                if (timeVal() != tv)
                    setTimeVal(tv);
                return (true);
            case PCPtime:
                if (timeVal() != p->timeVal())
                    setTimeVal(p->timeVal());
                return (true);
            case PCPfloat:
                tv = mmRndL(p->floatVal());
                if (timeVal() != tv)
                    setTimeVal(tv);
                return (true);
            case PCPdouble:
                tv = mmRndL(p->doubleVal());
                if (timeVal() != tv)
                    setTimeVal(tv);
                return (true);
            case PCPstring:
                s = p->stringVal();
                if (s) {
                    double *d = SPnum.parse(&s, false);
                    if (d) {
                        tv = mmRndL(*d);
                        if (timeVal() != tv)
                            setTimeVal(tv);
                        return (true);
                    }
                }
                break;
            case PCPappType:
                break;
            }
        }
        return (false);

    case PCPfloat:
        {
            float fv;
            switch (p->type()) {
            case PCPbool:
                fv = p->boolVal();
                if (floatVal() != fv)
                    setFloatVal(fv);
                return (true);
            case PCPint:
                fv = p->intVal();
                if (floatVal() != fv)
                    setFloatVal(fv);
                return (true);
            case PCPtime:
                fv = p->timeVal();
                if (floatVal() != fv)
                    setFloatVal(fv);
                return (true);
            case PCPfloat:
                if (floatVal() != p->floatVal())
                    setFloatVal(p->floatVal());
                return (true);
            case PCPdouble:
                fv = p->doubleVal();
                if (floatVal() != fv)
                    setFloatVal(fv);
                return (true);
            case PCPstring:
                s = p->stringVal();
                if (s) {
                    double *d = SPnum.parse(&s, false);
                    if (d) {
                        fv = *d;
                        if (floatVal() != fv)
                            setFloatVal(fv);
                        return (true);
                    }
                }
                break;
            case PCPappType:
                break;
            }
        }
        return (false);

    case PCPdouble:
        {
            double dv;
            switch (p->type()) {
            case PCPbool:
                dv = p->boolVal();
                if (doubleVal() != dv)
                    setDoubleVal(dv);
                return (true);
            case PCPint:
                dv = p->intVal();
                if (doubleVal() != dv)
                    setDoubleVal(dv);
                return (true);
            case PCPtime:
                dv = p->timeVal();
                if (doubleVal() != dv)
                    setDoubleVal(dv);
                return (true);
            case PCPfloat:
                dv = p->floatVal();
                if (doubleVal() != dv)
                    setDoubleVal(dv);
                return (true);
            case PCPdouble:
                if (doubleVal() != p->doubleVal())
                    setDoubleVal(p->doubleVal());
                return (true);
            case PCPstring:
                s = p->stringVal();
                if (s) {
                    double *d = SPnum.parse(&s, false);
                    if (d) {
                        dv = *d;
                        if (doubleVal() != dv)
                            setDoubleVal(dv);
                        return (true);
                    }
                }
                break;
            case PCPappType:
                break;
            }
        }
        return (false);

    case PCPstring:
        {
            char buf[64];
            switch (p->type()) {
            case PCPbool:
                sprintf(buf, "%d", p->boolVal());
                if (!stringVal() || strcmp(stringVal(), buf))
                    setStringVal(buf);
                return (true);
            case PCPint:
                sprintf(buf, "%ld", p->intVal());
                if (!stringVal() || strcmp(stringVal(), buf))
                    setStringVal(buf);
                return (true);
            case PCPtime:
                sprintf(buf, "%ld", (long)p->timeVal());
                if (!stringVal() || strcmp(stringVal(), buf))
                    setStringVal(buf);
                return (true);
            case PCPfloat:
                sprintf(buf, "%.9g", p->floatVal());
                if (!stringVal() || strcmp(stringVal(), buf))
                    setStringVal(buf);
                return (true);
            case PCPdouble:
                sprintf(buf, "%.15lg", p->doubleVal());
                if (!stringVal() || strcmp(stringVal(), buf))
                    setStringVal(buf);
                return (true);
            case PCPstring:
                if ((!stringVal() && p->stringVal()) ||
                        (stringVal() && !p->stringVal()) ||
                        (stringVal() && p->stringVal() &&
                        strcmp(stringVal(), p->stringVal())))
                    setStringVal(p->stringVal());
                return (true);
            case PCPappType:
                break;
            }
        }
        return (false);

    case PCPappType:
        {
            switch (p->type()) {
            case PCPbool:
                break;
            case PCPint:
                break;
            case PCPtime:
                break;
            case PCPfloat:
                break;
            case PCPdouble:
                break;
            case PCPstring:
                break;
            case PCPappType:
                if ((appValSize() != p->appValSize()) ||
                        memcmp(appVal(), p->appVal(),
                        p->appValSize()))
                    setAppVal(p->appVal(), p->appValSize());
                return (true);
            }
        }
        break;
    }
    return (false);
}


// Static function.
// Duplicate a parameter list.
//
PCellParam *
PCellParam::dup(const PCellParam *thispc)
{
    PCellParam *p0 = 0, *pe = 0;
    for (const PCellParam *p = thispc; p; p = p->next()) {
        PCellParam *pn = 0;
        switch (p->type()) {
        case PCPbool:
            pn = new PCellParam(p->type(), p->name(), p->constraint_string(),
                p->boolVal());
            break;
        case PCPint:
            pn = new PCellParam(p->type(), p->name(), p->constraint_string(),
                (long)p->intVal());
            break;
        case PCPtime:
            pn = new PCellParam(p->type(), p->name(), p->constraint_string(),
                (long)p->timeVal());
            break;
        case PCPfloat:
            pn = new PCellParam(p->type(), p->name(), p->constraint_string(),
                p->floatVal());
            break;
        case PCPdouble:
            pn = new PCellParam(p->type(), p->name(), p->constraint_string(),
                p->doubleVal());
            break;
        case PCPstring:
            pn = new PCellParam(p->type(), p->name(), p->constraint_string(),
                p->stringVal());
            break;
        case PCPappType:
            pn = new PCellParam(p->type(), p->name(), p->constraint_string(),
                p->appName(), p->appVal(), p->appValSize());
            break;
        }
        if (!p0)
            p0 = pe = pn;
        else {
            pe->setNext(pn);
            pe = pn;
        }
    }
    return (p0);
}


// Set the parameters in this from p0.  The p0 should be a subset of
// the params in this, params in p0 and not this are ignored.
//
void
PCellParam::setup(const PCellParam *p0)
{
    {
        PCellParam *ppt = this;
        if (!ppt)
            return;
    }
    if (!p0)
        return;
    SymTab tab(false, false);
    for (PCellParam *p = this; p; p = p->next())
        tab.add(p->name(), p, false);
    for (const PCellParam *q = p0; q; q = q->next()) {
        PCellParam *p = (PCellParam*)SymTab::get(&tab, q->name());
        if (p == (PCellParam*)ST_NIL)
            continue;
        p->set(q);
    }
}


// All elements of this are set to corresponding p0 values, all
// changed flags are cleared.
//
void
PCellParam::reset(const PCellParam *p0)
{
    {
        PCellParam *ppt = this;
        if (!ppt)
            return;
    }
    if (!p0)
        return;
    SymTab tab(false, false);
    for (PCellParam *p = this; p; p = p->next())
        tab.add(p->name(), p, false);
    for (const PCellParam *q = p0; q; q = q->next()) {
        PCellParam *p = (PCellParam*)SymTab::get(&tab, q->name());
        if (p == (PCellParam*)ST_NIL)
            continue;
        p->set(q);
        p->p_changed = false;
    }
}


// Update the single element that matches prm_name with the value
// given.  It the constraint string is non-null, update the constraint
// as well.  Return true if a change was made.
// 
bool
PCellParam::setValue(const char *prm_name, double prm_value,
    const char *constr)
{
    PCellParam *p = find(this, prm_name);
    if (!p)
        return (false);

    if (constr) {
        while (isspace(*constr))
            constr++;
        if (*constr) {
            PConstraint *pc = new PConstraint();
            const char *c = constr;
            if (!pc->parseConstraint(&c)) {
                Log()->WarningLogV(mh::Initialization,
                    "Failed to parse new constraint string\n  %s\n"
                    "for parameter %s. constraint unchanged.",
                    constr, p_name);
                delete pc;
            }
            else {
                delete [] p->p_constr_str;
                p->p_constr_str = lstring::copy(constr);
                delete p->p_constr;
                p->p_constr = pc;
            }
        }
    }
    if (p->type() == PCPbool) {
        bool b = (prm_value != 0.0);
        if (b != p->boolVal()) {
            p->setBoolVal(b);
            return (true);
        }
    }
    else if (p->type() == PCPint) {
        long i = mmRndL(prm_value);
        if (i != p->intVal()) {
            p->setIntVal(i);
            return (true);
        }
    }
    else if (p->type() == PCPtime) {
        long i = mmRndL(prm_value);
        if (i != p->timeVal()) {
            p->setTimeVal(i);
            return (true);
        }
    }
    else if (p->type() == PCPfloat) {
        float f = prm_value;
        if (f != p->floatVal()) {
            p->setFloatVal(f);
            return (true);
        }
    }
    else if (p->type() == PCPdouble) {
            if (prm_value != p->doubleVal()) {
                p->setDoubleVal(prm_value);
            return (true);
        }
    }
    else if (p->type() == PCPstring) {
        char buf[128];
        sprintf(buf, "%.15g", prm_value);
        if (strcmp(buf, p->stringVal() ? p->stringVal() : "")) {
            p->setStringVal(buf);
            return (true);
        }
    }
    return (false);
}


// Update the single element that matches prm_name with the value
// given.  It the constraint string is non-null, update the constraint
// as well.  Return true if a change was made.
// 
bool
PCellParam::setValue(const char *prm_name, const char *prm_value,
    const char *constr)
{
    PCellParam *p = find(this, prm_name);
    if (!p)
        return (false);

    if (constr) {
        while (isspace(*constr))
            constr++;
        if (*constr) {
            PConstraint *pc = new PConstraint();
            const char *c = constr;
            if (!pc->parseConstraint(&c)) {
                Log()->WarningLogV(mh::Initialization,
                    "Failed to parse new constraint string\n  %s\n"
                    "for parameter %s. constraint unchanged.",
                    constr, p_name);
                delete pc;
            }
            else {
                delete [] p->p_constr_str;
                p->p_constr_str = lstring::copy(constr);
                delete p->p_constr;
                p->p_constr = pc;
            }
        }
    }
    if (p->type() == PCPbool) {
        bool b = boolval(prm_value);
        if (b != p->boolVal()) {
            p->setBoolVal(b);
            return (true);
        }
    }
    else if (p->type() == PCPint) {
        long i = longval(prm_value);
        if (i != p->intVal()) {
            p->setIntVal(i);
            return (true);
        }
    }
    else if (p->type() == PCPtime) {
        long i = longval(prm_value);
        if (i != p->timeVal()) {
            p->setTimeVal(i);
            return (true);
        }
    }
    else if (p->type() == PCPfloat) {
        float f = floatval(prm_value);
        if (f != p->floatVal()) {
            p->setFloatVal(f);
            return (true);
        }
    }
    else if (p->type() == PCPdouble) {
        double d = doubleval(prm_value);
        if (d != p->doubleVal()) {
            p->setDoubleVal(d);
            return (true);
        }
    }
    else if (p->type() == PCPstring) {
        if (strcmp(prm_value, p->stringVal() ? p->stringVal() : "")) {
            p->setStringVal(prm_value);
            return (true);
        }
    }
    return (false);
}


char *
PCellParam::getValue() const
{
    if (type() == PCPstring)
        return (lstring::copy(stringVal()));

    char buf[64];
    if (type() == PCPbool) {
        sprintf(buf, "%s", boolVal() ? "1" : "0");
        return (lstring::copy(buf));
    }
    if (type() == PCPint) {
        sprintf(buf, "%ld", intVal());
        return (lstring::copy(buf));
    }
    if (type() == PCPtime) {
        sprintf(buf, "%ld", intVal());
        return (lstring::copy(buf));
    }
    if (type() == PCPfloat) {
        sprintf(buf, "%.9g", floatVal());
        return (lstring::copy(buf));
    }
    if (type() == PCPdouble) {
        sprintf(buf, "%.15lg", doubleVal());
        return (lstring::copy(buf));
    }
    return (0);
}


char *
PCellParam::getValueByName(const char *prm_name) const
{
    const PCellParam *p = find_c(this, prm_name);
    if (p)
        return (p->getValue());
    return (0);
}


// Return an assignment string, which will be parsed by the Xic script
// interpreter for parameter initialization of native pcells.
//
char *
PCellParam::getAssignment() const
{
    sLstr lstr;
    lstr.add(name());
    lstr.add(" = ");
    char *v = getValue();
    lstr.add(v);
    delete [] v;
    return (lstr.string_trim());
}


// Return a string containing a TCL parameter assignment in the form
//   set name [expr "val"]           (numeric values)
//   set name "val"                  (pre-quoted strings)
//
// Note that any param names in the value string MUST have a $
// prepending, this is not added here.
//
char *
PCellParam::getTCLassignment() const
{
    sLstr lstr;
    lstr.add("set ");
    lstr.add(name());
    char *v = getValue();
    if (!v)
        v = lstring::copy("\"\"");
    if (*v == '"') {
        lstr.add_c(' ');
        lstr.add(v);
    }
    else {
        lstr.add(" [expr \"");
        lstr.add(v);
        lstr.add("\"]");
    }
    delete [] v;
    return (lstr.string_trim());
}


// This will update the existing elements from the passed string,
// which is in the format described for the parse() function.  No new
// elements are created, names not found and syntax errors are
// ignored.  True is returned if a change was made.
//
bool
PCellParam::update(const char *str)
{
    if (!str)
        return (false);

    bool chgd = false;
    const char *s = str;
    while (*s) {
        char typ, *nam, *val, *cns;
        if (!getPair(&s, &typ, &nam, &val, &cns))
            break;
        if (nam) {
            if (setValue(nam, val, cns))
                chgd = true;
        }
        delete [] nam;
        delete [] val;
        delete [] cns;
    }
    return (chgd);
}


namespace {
    inline bool pccmp(const PCellParam *p1, const PCellParam *p2)
    {
        return (strcmp(p1->name(), p2->name()) > 0);
    }
}


// Print the element to a string.  The format is
//
//    type:name=value:constraint
//
// type:
// Single character type specifier, for all but string type.  In
// strings that are used with Xic native PCells, all parameters are
// "string" type, so the type character and following colon will not
// appear.
//
// name=value
// The parameter name and value.
// 
// :constraint
// Optional colon-separated constraint string.  This is skipped if
// noconstr is true.
//
char *
PCellParam::this_string(bool noconstr) const
{
    sLstr lstr;

    // New type prefix.
    switch (type()) {
    case PCPbool:
        lstr.add_c('b');
        lstr.add_c(PCP_TYPE_SEP);
        break;
    case PCPint:
        lstr.add_c('i');
        lstr.add_c(PCP_TYPE_SEP);
        break;
    case PCPtime:
        lstr.add_c('t');
        lstr.add_c(PCP_TYPE_SEP);
        break;
    case PCPfloat:
        lstr.add_c('f');
        lstr.add_c(PCP_TYPE_SEP);
        break;
    case PCPdouble:
        lstr.add_c('d');
        lstr.add_c(PCP_TYPE_SEP);
        break;
    case PCPstring:
        // No prefix in this case.
        break;
    case PCPappType:
        lstr.add_c('a');
        lstr.add_c(PCP_TYPE_SEP);
        break;
    }
    lstr.add(name());
    lstr.add_c('=');

    switch (type()) {
    case PCPbool:
        lstr.add(boolVal() ? "1" : "0");
        break;
    case PCPint:
        lstr.add_i(intVal());
        break;
    case PCPtime:
        lstr.add_i(timeVal());
        break;
    case PCPfloat:
        lstr.add_d(floatVal(), 9, true);
        break;
    case PCPdouble:
        lstr.add_d(doubleVal(), 9, true);
        break;
    case PCPstring:
        {
            const char *str = stringVal();
            if (str) {
                bool quotme = false;
                if (*str != '\'' && *str != '"') {
                    for (const char *s = str; *s; s++) {
                        if (isspace(*s) || *s == PCP_WORD_SEP ||
                                *s == PCP_CONS_SEP) {
                            quotme = true;
                            break;
                        }
                    }
                }
                if (quotme)
                    lstr.add_c('"');

                // If the string is already double-quoted, add a
                // leading backslash which forces preservation of
                // the quotes in the string.
                else if (*str == '"')
                    lstr.add_c('\\');

                lstr.add(str);
                if (quotme)
                    lstr.add_c('"');
            }
            else {
                lstr.add_c('"');
                lstr.add_c('"');
            }
        }
        break;
    case PCPappType:
        lstr.add("0");
        break;
    }

    if (!noconstr) {
        // Add the constraint if there is one.
        const char *cns = constraint_string();
        if (cns && *cns) {
            lstr.add_c(PCP_CONS_SEP);
            lstr.add(cns);
        }
    }
    return (lstr.string_trim());
}


// Print the list to a string.  The format is
//
//    type:name=value:constraint,type:name=value,...
//
// Elements are separated by commas.  There is no white space, except
// possibly enclosed in strings.  If a string token contains commas,
// colons, or white space, it will be double quoted.
//
// If noconstr, the constraints will be skipped over and ignored.
//
char *
PCellParam::string(bool noconstr) const
{
    sLstr lstr;
    for (const PCellParam *p = this; p; p = p->next()) {
        char *s = p->this_string(noconstr);
        lstr.add(s);
        delete [] s;
        if (p->next())
            lstr.add_c(PCP_WORD_SEP);
    }
    return (lstr.string_trim());
}


// Return a digest string that is unique for the parameter set and values
// independent of order.
//
char *
PCellParam::digest() const
{
    int n = 0;
    for (const PCellParam *p = this; p; p = p->next())
        n++;
    if (n == 0)
        return (lstring::copy("noparams"));
    const PCellParam **ary = new const PCellParam*[n];
    n = 0;
    for (const PCellParam *p = this; p; p = p->next())
        ary[n++] = p;
    if (n > 1)
        std::sort(ary, ary + n, pccmp);
    sLstr lstr;
    for (int i = 0; i < n; i++) {
        const PCellParam *p = ary[i];
        if (p->type() == PCPappType) {
            lstr.add_i(p->appValSize());
            for (unsigned int k = 0; k < p->appValSize(); k++) {
                char c = p->appVal()[k];
                lstr.add_c(c ? c : '0');
            }
        }
        else {
            char *s = ary[i]->this_string(true);
            lstr.add(s);
            delete [] s;
        }
    }
    delete [] ary;

    // MD5 likes 4-byte boundaries.
    while (lstr.length() % 4)
        lstr.add_c('$');
    n = lstr.length();

    // Compute the MD5 digest.
    MD5cx ctx;
    ctx.update((unsigned char*)lstr.string(), n);
    unsigned char *fstr = new unsigned char[17];
    memset(fstr, 0, 17);
    ctx.final(fstr);

    // Remap the chars so that they are all valid in GDSII cell names.
    // These are 0-9a-zA-Z?$_
    const char *good =
        "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ$?_";
    int mod = strlen(good);

    for (n = 0; n < 16; n++)
        fstr[n] = good[fstr[n] % mod];
    return ((char*)fstr);
}


// Print the list (debugging).
//
void
PCellParam::print() const
{
    for (const PCellParam *p = this; p; p = p->next()) {
        char *s = p->this_string(false);
        printf("%s\n", s);
        delete [] s;
    }
}


// Static function.
// Get one name = value pair and advance the pointer.  The expected
// element syntax is
//
//   [typechar:]name [=] value[:constraint] [,]
//
// Return false on a fatal syntax error, else return true.  The
// pointers should be checked on a true return, a null name indicates
// end of data.
//
bool
PCellParam::getPair(const char **pstr, char *ptype, char **pname,
    char **pvalue, char **pconstraint)
{
    *ptype = 0;
    *pname = 0;
    *pvalue = 0;
    if (pconstraint)
        *pconstraint = 0;
    char *nam = 0;
    char *val = 0;
    char *constr = 0;

    // Skip leading space and word separation characters.
    const char *s = *pstr;
    while (*s == PCP_WORD_SEP || isspace(*s))
        s++;
    if (!*s) {
        *pstr = s;
        return (true);
    }

    // s now points to first char of [type:]name, parse this.
    char typ = 0;
    if (*(s+1) == PCP_TYPE_SEP) {
        typ = *s;
        s += 2;
    }
    *ptype = typ;

    // Copy the parameter name, which is terminated by '=' or white
    // space.
    const char *eq = s;
    while (*eq && *eq != '=' && !isspace(*eq))
        eq++;
    int len = eq - s;
    nam = new char[len + 1];
    strncpy(nam, s, len);
    nam[len] = 0;
    // Note that nam can be empty, which is not good, but we'll look
    // at the rest of the token before dealing with it.

    // Skip to start of value.
    s = eq + 1;
    while (*s == '=' || isspace(*s))
        s++;

    const char *sep = s;
    if (*sep) {
        // This is a little tricky.  It the value starts out with a
        // double quote character, read up to the matching quote
        // verbatim, but strip the quotes.  If the value starts with a
        // single quote, also read in verbatim but retain the quotes
        // (for Python) unless the second character is a double quote
        // mark.  In this case, strip the single quotes, the double
        // quotes will remain.  This is for backward compatibility
        // with '"a string"' as a way to preserve the double quotes
        // for native string assignments.
        //
        // If the value starts with backslash-double quote, strip the
        // backslash and retain the double quotes.  When copying and
        // searching for the ending quote, if an ending quote
        // character is preceded by a backslash, copy both characters
        // verbatim and continue searching.

        const char *q2 = 0;
        if (*sep == '"' || *sep == '\'') {
            char q = *sep++;
            bool killit = false;
            if (q == '"' || *sep == '"') {
                s++;  // Skip leading quote.
                killit = true;
            }
            while (*sep && (*sep != q || *(sep-1) == '\\'))
                sep++;
            if (*sep && killit)
                q2 = sep;  // Location of ending quote.
        }
        else if (*sep == '\\' && *(sep+1) == '"') {
            char q = *(sep+1);
            sep++;
            sep++;
            s++;  // Skip backslash, but retain quote.
            while (*sep && (*sep != q || *(sep-1) == '\\'))
                sep++;
        }
        while (*sep && *sep != PCP_WORD_SEP && *sep != PCP_CONS_SEP &&
                !isspace(*sep))
            sep++;
        len = sep - s;
        val = new char[len + 1];
        if (q2) {
            // Don't copy the ending quote char.
            strncpy(val, s, q2 - s);
            strncpy(val + (q2 - s), q2 + 1, sep - q2 - 1);
            val[len - 1] = 0;
        }
        else {
            strncpy(val, s, len);
            val[len] = 0;
        }

        // Look for a constraint string.
        if (*sep == PCP_CONS_SEP) {
            sep++;
            s = sep;
            q2 = 0;
            if (*sep == '"' || *sep == '\'') {
                char q = *sep++;
                s++;  // will skip leading quote
                while (*sep && (*sep != q || *(sep-1) == '\\'))
                    sep++;
                if (*sep)
                    q2 = sep;  // location of ending quote
                while (*sep && *sep != PCP_WORD_SEP && !isspace(*sep))
                    sep++;
                len = sep - s;
                constr = new char[len + 1];
                if (q2) {
                    // Don't copy the ending quote char.
                    strncpy(constr, s, q2 - s);
                    strncpy(constr + (q2 - s), q2 + 1, sep - q2 - 1);
                    constr[len - 1] = 0;
                }
                else {
                    strncpy(constr, s, len);
                    constr[len] = 0;
                }
                // Make sure that it is valid.
                PConstraint pc;
                s = constr;
                if (!pc.parseConstraint(&s)) {
                    // Syntax error.
                    delete [] nam;
                    delete [] val;
                    Errs()->add_error(
                        "Syntax error in constraint %s.", constr);
                    delete [] constr;
                    return (false);
                }
            }
            else {
                // Not quoted, the constraint parser determines the
                // length.
                PConstraint pc;
                if (!pc.parseConstraint(&sep)) {
                    // Syntax error.
                    delete [] val;
                    Errs()->add_error(
                        "Syntax error in parameter %s constraint.", nam);
                    delete [] nam;
                    return (false);
                }
                constr = new char[sep - s + 1];
                strncpy(constr, s, sep - s);
                constr[sep - s] = 0;
            }
        }
    }

    // Now see if what we saved is ok.
    if (*nam && val) {
        if (!*val && typ) {
            // Empty value accepted for string type only.
            delete [] val;
            delete [] constr;
            Errs()->add_error("Empty value in parameter %s string.", nam);
            delete [] nam;
            return (false);
        }

        *pname = nam;
        *pvalue = val;
        if (pconstraint)
            *pconstraint = constr;
        else
            delete [] constr;
    }
    else {
        if (!*nam && (!val || !*val)) {
            // No name or value, ignore.
            delete [] nam;
            delete [] val;
            delete [] constr;
        }
        else {
            // Syntax error.
            delete [] val;
            delete [] constr;
            Errs()->add_error("Syntax error in parameter %s string.",
                nam ? nam : "");
            delete [] nam;
            return (false);
        }
    }
    *pstr = sep;
    return (true);
}


// Static function.
// Take the user-supplied input string and return in pret a PCellParam
// list.  The input format is as described for the string() method. 
// It is assumed to consist of pairs of name/value tokens, separated
// by white space, commas, or equal signs.  It supports the
// [typechar:] prefixing on parameter names, and Ciranova-style
// constraint strings which can follow the value separated by a colon. 
// I.e.,
//
//   [typechar:]name [=] value[:constraint] [,]
//
// Note that white space is a delimiter and is otherwise ignored, and
// the '=' and ',' characters are optional.  If the leading character
// of a value is a single or double quote, all characters up to the
// matching quote will be read verbatim, with the quotes stripped.
//
bool
PCellParam::parseParams(const char *str, PCellParam **pret)
{
    *pret = 0;
    if (!str)
        return (true);

    PCellParam *p0 = 0, *pe = 0;
    const char *s = str;

    while (*s) {
        char typ, *nam, *val, *cns;
        if (!getPair(&s, &typ, &nam, &val, &cns)) {
            PCellParam::destroy(p0);
            return (false);
        }
        if (!nam)
            break;
        PCellParam *px = new PCellParam(typ, nam, val, cns);
        if (!p0)
            p0 = pe = px;
        else {
            pe->setNext(px);
            pe = px;
        }
        delete [] nam;
        delete [] val;
        delete [] cns;
    }
    *pret = p0;
    return (true);
}


// Private function to initialize the constraint.
//
void
PCellParam::init_constraint()
{
    p_constr = 0;
    if (p_constr_str) {
        p_constr = new PConstraint();
        const char *c = p_constr_str;
        if (!p_constr->parseConstraint(&c)) {
            Log()->WarningLogV(mh::Initialization,
                "Failed to parse constraint string\n  %s\n"
                "for parameter %s. constraint ignored.",
                p_constr_str, p_name);
            delete p_constr;
            p_constr = 0;
            delete [] p_constr_str;
            p_constr_str = 0;
        }
    }
}
// End of PCellParam functions.


//
// Parse methods for Ciranova-style parameter constraint strings,
// which are in the form of Python function calls.
//

PConstraint::PConstraint()
{
    pc_type = PCunknown;
    pc_action = PCAreject;
    pc_choice_list = 0;
    pc_scale = lstring::copy("u");  // default value

    pc_low = 0.0;
    pc_high = 0.0;
    pc_resol = 0.0;
    pc_step = 0.0;
    pc_start = 0.0;
    pc_limit = 0.0;
    pc_sfact = factor(pc_scale);

    pc_low_none = true;
    pc_high_none = true;
    pc_resol_none = true;
    pc_step_none = true;
    pc_start_none = false;  // defaults to 0
    pc_limit_none = true;
}


namespace {
    // Return true and set pd if the param is numeric, including a
    // string value that can be parsed as a number.
    //
    bool numeric_val(const PCellParam *p, double *pd)
    {
        if (p->type() == PCPstring) {
            const char *t = p->stringVal();
            double *d = SPnum.parse(&t, false);
            if (d) {
                *pd = *d;
                return (true);
            }
            return (false);
        }
        if (p->type() == PCPint) {
            *pd = p->intVal();
            return (true);
        }
        if (p->type() == PCPtime) {
            *pd = p->timeVal();
            return (true);
        }
        if (p->type() == PCPfloat) {
            *pd = p->floatVal();
            return (true);
        }
        if (p->type() == PCPdouble) {
            *pd = p->doubleVal();
            return (true);
        }
        return (false);
    }


    bool numeric_val(const char *str, double *pd)
    {
        double *d = SPnum.parse(&str, false);
        if (d) {
            *pd = *d;
            return (true);
        }
        return (false);
    }
}


// Return false if the constraint forbids the value in the argument,
// true if ok or doesn't apply.
//
bool
PConstraint::checkConstraint(const PCellParam *p) const
{
    if (!p || p->type() == PCPbool || p->type() == PCPappType)
        return (true);
    if (pc_action == PCAaccept)
        return (true);

    if (pc_type == PCchoice) {
        if (!pc_choice_list)
            return (true);
        if (p->type() == PCPstring) {
            const char *str = p->stringVal();
            if (!str)
                str = "";
            for (stringlist *sl = pc_choice_list; sl; sl = sl->next) {
                if (lstring::cieq(str, sl->string))
                    return (true);
            }
            return (false);
        }
        double val = 0;
        if (numeric_val(p, &val)) {
            for (stringlist *sl = pc_choice_list; sl; sl = sl->next) {
                const char *s = sl->string;
                double *d = SPnum.parse(&s, false);
                if (d) {
                    if (*d == val)
                        return (true);
                    if (fabs(val - *d) < 1e-9*(fabs(val) + fabs(*d)))
                        return (true);
                }
            }
            return (false);
        }
        return (true);
    }
    if (pc_type == PCrange) {
        double val;
        if (numeric_val(p, &val)) {
            if (!pc_low_none && val < pc_low)
                return (false);
            if (!pc_high_none && val > pc_high)
                return (false);
        }
        return (true);
    }
    if (pc_type == PCstep || pc_type == PCnumStep) {
        // PCnumStep is like PCstep except that
        // 1.  The SPICE number parser is used to convert the string
        //     value, allowing use of suffix multipliers (e.g., 1k = 1e3).
        // 2.  The start, step, and limit values are internally multiplied
        //     by the scaleFactor before use (when parsed).

        double val;
        if (numeric_val(p, &val)) {
            if (pc_step == 0.0) {
                // In this case, treat like range.
                if (!pc_start_none && val < pc_start)
                    return (false);
                if (!pc_limit_none && val > pc_limit)
                    return (false);
                return (true);
            }
            int n = mmRnd((val - pc_start)/pc_step);
            if (n < 0)
                return (false);
            if (!pc_limit_none) {
                if (pc_step > 0.0 && val > pc_limit)
                    return (false);
                if (pc_step < 0.0 && val < pc_limit)
                    return (false);
            }
            double d = pc_start + n*pc_step;
            if (d == val)
                return (true);
            if (fabs(d - val) < 1e-9*(fabs(d) + fabs(val)))
                return (true);
            return (false);
        }
        return (true);
    }
    return (true);
}


// Return false if the constraint forbids the value in the argument,
// true if ok or doesn't apply.
//
bool
PConstraint::checkConstraint(double val) const
{
    if (pc_action == PCAaccept)
        return (true);

    if (pc_type == PCchoice) {
        if (!pc_choice_list)
            return (true);
        for (stringlist *sl = pc_choice_list; sl; sl = sl->next) {
            const char *s = sl->string;
            double *d = SPnum.parse(&s, false);
            if (d) {
                if (*d == val)
                    return (true);
                if (fabs(val - *d) < 1e-9*(fabs(val) + fabs(*d)))
                    return (true);
            }
        }
        return (false);
    }
    if (pc_type == PCrange) {
        if (!pc_low_none && val < pc_low)
            return (false);
        if (!pc_high_none && val > pc_high)
            return (false);
        return (true);
    }
    if (pc_type == PCstep || pc_type == PCnumStep) {
        // PCnumStep is like PCstep except that
        // 1.  The SPICE number parser is used to convert the string
        //     value, allowing use of suffix multipliers (e.g., 1k = 1e3).
        // 2.  The start, step, and limit values are internally multiplied
        //     by the scaleFactor before use (when parsed).

        if (pc_step == 0.0) {
            // In this case, treat like range.
            if (!pc_start_none && val < pc_start)
                return (false);
            if (!pc_limit_none && val > pc_limit)
                return (false);
            return (true);
        }
        int n = mmRnd((val - pc_start)/pc_step);
        if (n < 0)
            return (false);
        if (!pc_limit_none) {
            if (pc_step > 0.0 && val > pc_limit)
                return (false);
            if (pc_step < 0.0 && val < pc_limit)
                return (false);
        }
        double d = pc_start + n*pc_step;
        if (d == val)
            return (true);
        if (fabs(d - val) < 1e-9*(fabs(d) + fabs(val)))
            return (true);
        return (false);
    }
    return (true);
}


// Return false if the constraint forbids the value in the argument,
// true if ok or doesn't apply.
//
bool
PConstraint::checkConstraint(const char *string) const
{
    if (pc_action == PCAaccept)
        return (true);
    if (!string)
        string = "";

    if (pc_type == PCchoice) {
        if (!pc_choice_list)
            return (true);
        double val = 0;
        if (numeric_val(string, &val)) {
            for (stringlist *sl = pc_choice_list; sl; sl = sl->next) {
                const char *s = sl->string;
                double *d = SPnum.parse(&s, false);
                if (d) {
                    if (*d == val)
                        return (true);
                    if (fabs(val - *d) < 1e-9*(fabs(val) + fabs(*d)))
                        return (true);
                }
            }
        }
        else {
            for (stringlist *sl = pc_choice_list; sl; sl = sl->next) {
                if (lstring::cieq(string, sl->string))
                    return (true);
            }
        }
        return (false);
    }
    if (pc_type == PCrange) {
        double val;
        if (numeric_val(string, &val)) {
            if (!pc_low_none && val < pc_low)
                return (false);
            if (!pc_high_none && val > pc_high)
                return (false);
        }
        return (true);
    }
    if (pc_type == PCstep || pc_type == PCnumStep) {
        // PCnumStep is like PCstep except that
        // 1.  The SPICE number parser is used to convert the string
        //     value, allowing use of suffix multipliers (e.g., 1k = 1e3).
        // 2.  The start, step, and limit values are internally multiplied
        //     by the scaleFactor before use (when parsed).

        double val;
        if (numeric_val(string, &val)) {
            if (pc_step == 0.0) {
                // In this case, treat like range.
                if (!pc_start_none && val < pc_start)
                    return (false);
                if (!pc_limit_none && val > pc_limit)
                    return (false);
                return (true);
            }
            int n = mmRnd((val - pc_start)/pc_step);
            if (n < 0)
                return (false);
            if (!pc_limit_none) {
                if (pc_step > 0.0 && val > pc_limit)
                    return (false);
                if (pc_step < 0.0 && val < pc_limit)
                    return (false);
            }
            double d = pc_start + n*pc_step;
            if (d == val)
                return (true);
            if (fabs(d - val) < 1e-9*(fabs(d) + fabs(val)))
                return (true);
            return (false);
        }
        return (true);
    }
    return (true);
}


// Parse a Python constraint specification, setting the appropriate
// fields in this.  Returns true if success, false otherwise with a
// message fragment in the error system.
//
bool
PConstraint::parseConstraint(const char **pstr)
{
    if (!pstr)
        return (true);
    const char *c = *pstr;
    if (!c || !*c)
        return (true);
    char *cname = lstring::gettok(&c, "(");
    if (!cname) {
        Errs()->add_error("no constraint typename given");
        return (false);
    }
    if (lstring::ciprefix("choice", cname)) {
        // ChoiceConstraint(choices, action=REJECT)
        // choices: ['string', Layer('string') ... ]

        delete [] cname;
        if (!parse_argument(&c, "choices"))
            goto bad;
        if (!parse_argument(&c, "action"))
            goto bad;
        pc_type = PCchoice;
    }
    else if (lstring::ciprefix("range", cname)) {
        // RangeConstraint(low, high, resolution=None, action=REJECT)

        delete [] cname;
        if (!parse_argument(&c, "low"))
            goto bad;
        if (!parse_argument(&c, "high"))
            goto bad;
        if (!parse_argument(&c, "resolution"))
            goto bad;
        if (!parse_argument(&c, "action"))
            goto bad;
        if (!pc_low_none && !pc_high_none && pc_low > pc_high) {
            double tmp = pc_low;
            pc_low = pc_high;
            pc_high = tmp;
        }
        pc_type = PCrange;
    }
    else if (lstring::ciprefix("step", cname)) {
        // StepConstraint(step, start=0, limit=None, resolution=None,
        //   action=REJECT)

        delete [] cname;
        if (!parse_argument(&c, "step"))
            goto bad;
        if (!parse_argument(&c, "start"))
            goto bad;
        if (!parse_argument(&c, "limit"))
            goto bad;
        if (!parse_argument(&c, "resolution"))
            goto bad;
        if (!parse_argument(&c, "action"))
            goto bad;
        pc_type = PCstep;
    }
    else if (lstring::ciprefix("numericstep", cname)) {
        // NumericStepConstraint(step, start=0, limit=None, resolution=None,
        //   scaleFactor='u', action=REJECT)

        // This is like "step" except that
        // 1.  The SPICE number parser is used to convert the string
        //     value, allowing use of suffix multipliers (e.g., 1k = 1e3).
        // 2.  The start, step, and limit values are internally multiplied
        //     by the scaleFactor before use.

        delete [] cname;
        if (!parse_argument(&c, "step"))
            goto bad;
        if (!parse_argument(&c, "start"))
            goto bad;
        if (!parse_argument(&c, "limit"))
            goto bad;
        if (!parse_argument(&c, "resolution"))
            goto bad;
        if (!parse_argument(&c, "scaleFactor"))
            goto bad;
        if (!parse_argument(&c, "action"))
            goto bad;
        double f = factor(pc_scale);
        if (f != 0.0) {
            pc_start *= f;
            pc_step *= f;
            pc_limit *= f;
        }
        pc_type = PCnumStep;
    }
    else {
        Errs()->add_error("unknown constraint name: %s", cname);
        delete [] cname;
        goto bad;
    }

    // Get past the trailing ')', we'll accept white space here for no
    // particularly good reason.
    while (isspace(*c))
        c++;
    if (*c == ')')
        c++;
    *pstr = c;
    return (true);

bad:
    // Skip to the end of the token, presumably terminated by ')'.
    while (*c && *c != ')')
        c++;
    if (*c == ')')
        c++;
    *pstr = c;
    return (false);
}


// Static function.
// Return the numerical scale factor implied by the suffix string,
// or 0 if not recognized.
//
double
PConstraint::factor(const char *s)
{
    if (s && *s) {
        char c = isupper(*s) ? tolower(*s) : *s;
        switch (c) {
        case 'a':   return (1e-18);
        case 'f':   return (1e-15);
        case 'p':   return (1e-12);
        case 'n':
            {
                if (lstring::ciprefix("none", s))
                    return (0.0);
                return (1e-9);
            }
        case 'u':   return (1e-6);
        case 'm':
            {
                if (lstring::ciprefix("meg", s))
                    return (1e6);
                if (lstring::ciprefix("mil", s))
                    return (1e-3/39.3700787402);
                return (1e-3);
            }
        case 'k':   return (1e3);
        case 'g':   return (1e9);
        case 't':   return (1e12);
        default:
            break;
        }
    }
    return (0.0);
}


// Parse an argument.  The argname is the positional name, which is
// ignored if a keyword is found with the argument.
//
bool
PConstraint::parse_argument(const char **pstr, const char *argname)
{
    const char *s = *pstr;
    while (isspace(*s) || *s == ',')
        s++;
    if (!*s || *s == ')')
        return (true);

    char *keyword = 0;
    if (isalpha(*s)) {
        const char *t = s;
        while (*t && !isspace(*t) && *t != '=' && *t != ',' && *t != ')')
            t++;
        while (isspace(*t))
            t++;
        if (*t == '=') {
            // a keyword=value argument
            keyword = new char[t - s + 1];
            char *se = keyword;
            while (!isspace(*s) && *s != '=')
                *se++ = *s++;
            *se = 0;
            s = t+1;
        }
    }
    if (!keyword)
        keyword = lstring::copy(argname);

    bool ret = true;
    if (lstring::cieq(keyword, "choices"))
        ret = parse_list(&s);
    else if (lstring::cieq(keyword, "action"))
        ret = parse_action(&s);
    else if (lstring::cieq(keyword, "low"))
        ret = parse_number(&s, &pc_low, &pc_low_none);
    else if (lstring::cieq(keyword, "high"))
        ret = parse_number(&s, &pc_high, &pc_high_none);
    else if (lstring::cieq(keyword, "resolution"))
        ret = parse_number(&s, &pc_resol, &pc_resol_none);
    else if (lstring::cieq(keyword, "step"))
        ret = parse_number(&s, &pc_step, &pc_step_none);
    else if (lstring::cieq(keyword, "start"))
        ret = parse_number(&s, &pc_start, &pc_start_none);
    else if (lstring::cieq(keyword, "limit"))
        ret = parse_number(&s, &pc_limit, &pc_limit_none);
    else if (lstring::cieq(keyword, "scaleFactor")) {
        delete [] pc_scale;
        if (*s == '\'' || *s == '"')
            pc_scale = parse_quoted(&s);
        else {
            const char *t = s;
            while (*t && !isspace(*t) && *t != ',' && *t != ')')
                t++;
            int len = t - s;
            pc_scale = new char[len + 1];
            strncpy(pc_scale, s, len);
            pc_scale[len] = 0;
            s = t;
        }
        pc_sfact = factor(pc_scale);
    }
    else {
        ret = false;
        Errs()->add_error("unknown argument: %s", keyword);
    }
    delete [] keyword;
    *pstr = s;
    return (ret);
}


// Parse the list that is proveded for the "choices" value.  A list
// consists of comma-separated tokens inside of square brackets. 
// Elements can be either single-quoted strings or forms like
// "Layer('name')".  In the latter case, we simply strip the Layer( )
// part.  If success, the pc_choice_list is set to a list of unquoted
// tokens, the plst is advanced, and true is returned.
//
bool
PConstraint::parse_list(const char **plst)
{
    const char *terms = ",()]";
    const char *s = *plst;
    while (isspace(*s) || *s == ',')
        s++;
    if (*s != '[') {
        // Can't be a list!
        return (false);
    }
    s++;

    stringlist *sle = 0;
    for (;;) {
        while (isspace(*s) || *s == ',' || *s == ')')
            s++;
        if (*s == ']')
            break;
        if (*s == '\'' || *s == '"') {
            // 'foo' or "foo"
            char *str = parse_quoted(&s);
            stringlist *sl = new stringlist(str, 0);
            if (!sle)
                sle = pc_choice_list = sl;
            else {
                sle->next = sl;
                sle = sl;
            }
        }
        else {
            const char *t = s;
            while (*t && !isspace(*t) && !strchr(terms, *t))
                t++;
            const char *e = t;
            while (isspace(*e))
                e++;
            if (*e == '(') {
                // Weird Ciranova thing, form like Layer(something), we
                // keep only the something.
                s = e + 1;
                continue;
            }
            else {
                // Unquoted word, just grab it verbatim.
                int len = t - s;
                if (len) {
                    char *str = new char[len + 1];
                    strncpy(str, s, len);
                    str[len] = 0;
                    stringlist *sl = new stringlist(str, 0);
                    if (!sle)
                        sle = pc_choice_list = sl;
                    else {
                        sle->next = sl;
                        sle = sl;
                    }
                }
                s = t;
            }
        }
    }
    *plst = s + 1;
    return (true);
}


// Check the value of the action token.  Set the pc_action if the
// token is recognized, advance pstr, and return true.
//
bool
PConstraint::parse_action(const char **pstr)
{
    // Look for REJECT, ACCEPT, USE_DEFAULT.
    const char *s = *pstr;
    while (isspace(*s) || *s == ',')
        s++;
    if (lstring::ciprefix("REJECT", s)) {
        s += 6;
        pc_action = PCAreject;
    }
    else if (lstring::ciprefix("ACCEPT", s)) {
        s += 6;
        pc_action = PCAaccept;
    }
    else if (lstring::ciprefix("USE_DEFAULT", s)) {
        s += 11;
        pc_action = PCAdefault;
    }
    else
        return (false);
    *pstr = s;
    return (true);
}


// Parse a token known to start with a single or double quote
// character.  This returns a copy of the token with the quotes
// stripped, and advances pstr.  We skip over the initial quote,
// copying characters until the final quote or a null byte.
//
char *
PConstraint::parse_quoted(const char **pstr)
{
    const char *s = *pstr;
    char q =  *s++;
    if (q != '\'' && q != '"')
        return (0);  // error
    const char *t = s;
    while (*t && *t != q)
        t++;
    char *str = new char[t - s + 1];
    char *se = str;
    while (s < t)
        *se++ = *s++;
    *se = 0;
    *pstr = *s ? s+1 : s;
    return (str);
}


// Parse a number token, which can be a raw number or a single-quoted
// string.  We use the SPICE parser to handle the scale factors.  If
// the parse succeeds, true is returned, and the result is in res. 
// The pstr is advanced past the number token.
//
// Giving a value "None" will instead set the flag.
//
bool
PConstraint::parse_number(const char **pstr, double *res, bool *none)
{
    const char *s = *pstr;
    while (isspace(*s) || *s == ',')
        s++;
    if (*s == '\'' || *s == '"') {
        char *str = parse_quoted(&s);
        if (lstring::ciprefix("str", s)) {
            *none = true;
            *pstr = s;
            return (true);
        }
        const char *t = str;
        double *dp = SPnum.parse(&t, false);
        delete [] str;
        if (!dp) {
            Errs()->add_error("number parse failure");
            return (false);
        }
        *res = *dp;
    }
    else {
        if (lstring::ciprefix("None", s)) {
            *none = true;
            *pstr = s + 4;
            return (true);
        }
        double *dp = SPnum.parse(&s, false);
        if (!dp) {
            Errs()->add_error("number parse failure");
            return (false);
        }
        *res = *dp;
    }
    *pstr = s;
    *none = false;
    return (true);
}
// End of PConstraint functions.


// Originally, Xic pcell names were required to have the form
// "basenameXXX", but any name is now accepted.  The XXX, if present,
// will be stripped.
//
#define OLD_PCELL_SUFFIX    "XXX"

// Return a unique sub-master cell name, used to name the Xic cells
// for the sub-master.
//
// There is no clobber-testing here.  The user can set the cellname
// with PCellItem::setCellname, otherwise a "suggested" name is
// generated here.
//
char *
PCellDesc::cellname(const char *basename, const PCellItem *pi) const
{
    if (pi->cellname())
        return (lstring::copy(pi->cellname()));

    int len = strlen(basename);
    char *cname = new char[len + 18];
    strcpy(cname, basename);
    char *s = cname + len - strlen(OLD_PCELL_SUFFIX);
    if (!strcmp(s, OLD_PCELL_SUFFIX))
        *s = 0;
    else
        s = cname + len;

#define use_digest 0
    if (use_digest) {
        // Using the digest will give a repeatably unique name
        // for a given parameter set.

        char *dgn = pi->params()->digest();
        // The digest is 16 chars plus a null terminator, enforce
        // this anyway.
        if (strlen(dgn) > 16)
            dgn[16] = 0;
        *s++ = '$';
        strcpy(s, dgn);
        delete [] dgn;
    }
    else
        sprintf(s, "$$%u", pi->index());
    return (cname);
}


// Static function.
// Encode the names into a single "dbname" token.
// Warning: CDs::setPCellFlags assumes this format.
//
char *
PCellDesc::mk_dbname(const char *libname, const char *cellname,
    const char *viewname)
{
    int len = 0;
    len += libname ? strlen(libname) : 0;
    len += cellname ? strlen(cellname) : 0;
    len += viewname ? strlen(viewname) : 0;
    len += 7;
    char *dbname = new char[len];
    char *t = dbname;
    *t++ = '<';
    if (libname)
        t = lstring::stpcpy(t, libname);
    *t++ = '>';
    *t++ = '<';
    if (cellname)
        t = lstring::stpcpy(t, cellname);
    *t++ = '>';
    *t++ = '<';
    if (viewname)
        t = lstring::stpcpy(t, viewname);
    *t++ = '>';
    *t = 0;
    return (dbname);
}


// Static function.
// Encode the Xic cellname into a single "dbname" token.
//
char *
PCellDesc::mk_native_dbname(const char *cellname)
{
    return (mk_dbname(XIC_NATIVE_LIBNAME, cellname, "layout"));
}


// Static function.
// Split a "dbname" string into its components.
//
bool
PCellDesc::split_dbname(const char *dbname, char **libname, char **cellname,
    char **viewname)
{
    if (libname)
        *libname = 0;
    if (cellname)
        *cellname = 0;
    if (viewname)
        *viewname = 0;

    const char *t = dbname;
    if (!t || *t != '<')
        return (false);
    t++;
    const char *e = strchr(t, '>');
    if (!e)
        return (false);
    char *lname = new char[e - t + 1];
    strncpy(lname, t, e - t);
    lname[e - t] = 0;

    t = e + 1;
    if (*t != '<') {
        delete [] lname;
        return (false);
    }
    t++;
    e = strchr(t, '>');
    if (!e) {
        delete [] lname;
        return (false);
    }
    char *cname = new char[e - t + 1];
    strncpy(cname, t, e - t);
    cname[e - t] = 0;

    t = e + 1;
    if (*t != '<') {
        delete [] lname;
        delete [] cname;
        return (false);
    }
    t++;
    e = strchr(t, '>');
    if (!e) {
        delete [] lname;
        delete [] cname;
        return (false);
    }
    char *vname = new char[e - t + 1];
    strncpy(vname, t, e - t);
    vname[e - t] = 0;

    if (libname)
        *libname = lname;
    else
        delete [] lname;
    if (cellname)
        *cellname = cname;
    else
        delete [] cname;
    if (viewname)
        *viewname = vname;
    else
        delete [] vname;
    return (true);
}
// End of PCellDesc functions.


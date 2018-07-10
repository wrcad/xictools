
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

#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "srcdefs.h"
#include "input.h"

namespace {
    void parsepoly(sLine*, const char**, int, const char*);
    char *polystring(char**, int, char**, int);
    int  nxtpwr(int*, int*);


    // Case insensitive str eq.
    //
    int cieq(const char *p, const char *s)
    {
        if (p == 0 || s == 0)
            return (0);
        while (*p) {
            if ((isupper(*p) ? tolower(*p) : *p) !=
                (isupper(*s) ? tolower(*s) : *s))
                return (0);
            p++;
            s++;
        }
        return (*s ? 0 : 1);
    }


    // Create a copy of a string.
    //
    char *copy(const char *str)
    {
        if (str == 0)
            return (0);
        char *p = new char[strlen(str) + 1];
        strcpy(p, str);
        return (p);
    }


    // Append s2 to s1, and return the new string.  --> s1 is freed. <--
    //
    char *build_str(char *s1, const char *s2) 
    {
        if (!s2 || !*s2)
            return (s1);
        if (!s1 || !*s1)
            return (copy(s2));

        char *str = new char[strlen(s1) + strlen(s2) + 1];
        char *s = str;
        char *t = s1;
        while (*s1) *s++ = *s1++;
        while (*s2) *s++ = *s2++;
        *s = '\0';
        delete [] t;
        return (str);
    } 
}


// parser for all voltage/current sources
//
//   'arbitrary' source:
//   Aname <n+> <n-> [{"v" | "i"}=<expr>] [[DC] <val>] [AC [<val> [<val>]]]
//
//   'independent' voltage source:
//   Vname <n+> <n-> [<expr>] [[DC] <val>] [AC [<val> [<val>]]]
//
//   'independent' current source:
//   Iname <n+> <n-> [<expr>] [[DC] <val>] [AC [<val> [<val>]]]
//
//   'vcvs'
//   Ename <n+> <n-> <nc+> <nc-> <expr> [[DC] <val>] [AC [<val> [<val>]]]
//   Ename <n+> <n-> function <expr> [[DC] <val>] [AC [<val> [<val>]]]
//   Ename <n+> <n-> vol= <expr> [[DC] <val>] [AC [<val> [<val>]]]
//   Ename <n+> <n-> poly(n) polyprms  [[DC] <val>] [AC [<val> [<val>]]]
//
//   'vccs'
//   Gname <n+> <n-> <nc+> <nc-> <expr> [[DC] <val>] [AC [<val> [<val>]]]
//   Gname <n+> <n-> function <expr> [[DC] <val>] [AC [<val> [<val>]]]
//   Gname <n+> <n-> cur= <expr> [[DC] <val>] [AC [<val> [<val>]]]
//   Gname <n+> <n-> poly(n) polyprms [[DC] <val>] [AC [<val> [<val>]]]
//
//   'ccvs'
//   Hname <n+> <n-> <vname> <expr> [[DC] <val>] [AC [<val> [<val>]]]
//   Hname <n+> <n-> function <expr> [[DC] <val>] [AC [<val> [<val>]]]
//   Hname <n+> <n-> vol= <expr> [[DC] <val>] [AC [<val> [<val>]]]
//   Hname <n+> <n-> poly(n) polyprms [[DC] <val>] [AC [<val> [<val>]]]
//
//   'cccs'
//   Fname <n+> <n-> <vname> <expr> [[DC] <val>] [AC [<val> [<val>]]]
//   Fname <n+> <n-> function <expr> [[DC] <val>] [AC [<val> [<val>]]]
//   Fname <n+> <n-> cur= <expr> [[DC] <val>] [AC [<val> [<val>]]]
//   Fname <n+> <n-> poly(n) polyprms [[DC] <val>] [AC [<val> [<val>]]]
//
// General rules:
//   For G,E,F,H the DC/AC values must appear after the expr (which
//   is a number for linear dependent sources).
//   Otherwise, the DC/AC specs can appear before or after the expr.
//   If before, the numerical parameters must be given explicitly
//   unless the following token prefix is manifestly not a number.
//   The AC spec must be followed by something that is numerically
//   not a number (vector parse not smart enough to stop).
//   The expr must appear immediately after "v", "i", or "function".
//   If "DC" is omitted, and an expr is given, the dc value must
//   follow the expr.
//
void
SRCdev::parse(int type, sCKT *ckt, sLine *current)
{
    const char *mesg = "Too few parameters in device line";
    const char *fvmsg = "Found \"vol\", accepting as \"cur\"";
    const char *fcmsg = "Found \"cur\", accepting as \"vol\"";

    const char *line = current->line();
    const char *lptr = line;
    char *dname = IP.getTok(&line, true);
    if (!dname) {
        IP.logError(current, mesg);
        ckt->CKTnogo = true;
        return;
    }
    ckt->insert(&dname);
    char dvkey[2];
    dvkey[0] = islower(*dname) ? toupper(*dname) : *dname;
    dvkey[1] = 0;

    char *nname1 = IP.getTok(&line, true);
    if (!nname1) {
        IP.logError(current, mesg);
        ckt->CKTnogo = true;
        return;
    }
    sCKTnode *node1;     // the first node's node pointer
    ckt->termInsert(&nname1, &node1);

    char *nname2 = IP.getTok(&line, true);
    if (!nname2) {
        IP.logError(current, mesg);
        ckt->CKTnogo = true;
        return;
    }
    sCKTnode *node2;     // the second node's node pointer
    ckt->termInsert(&nname2, &node2);

    sGENmodel *mdfast;
    int error;
    sCKTmodItem *mx = ckt->CKTmodels.find(type);
    if (mx && mx->default_model)
        mdfast = mx->default_model;
    else {
        // create default source model
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
    sGENinstance *fast;  // pointer to the actual instance
    error = ckt->newInst(mdfast, &fast, dname);
    IP.logError(current, error);
    error = node1->bind(fast, 1);
    IP.logError(current, error);
    error = node2->bind(fast, 2);
    IP.logError(current, error);

    char buf[128];
    const char *polyline = 0;
    const char *xalias = "time";
    const char *leadname = "dc";
    const char *groundname = "0";
    if (*dvkey == 'E' || *dvkey == 'G') {
        // control nodes optional with function
        lptr = line;
        char *nname3 = IP.getTok(&line, true);
        if (!nname3) {
            IP.logError(current, mesg);
            ckt->CKTnogo = true;
            return;
        }
        if (cieq(nname3, "function")) {
            line = lptr;
            delete [] nname3;
        }
        else if (cieq(nname3, "vol")) {
            if (*dvkey == 'G')
                IP.logError(current, fvmsg);
            line = lptr;
            delete [] nname3;
        }
        else if (cieq(nname3, "cur")) {
            if (*dvkey == 'E')
                IP.logError(current, fcmsg);
            line = lptr;
            delete [] nname3;
        }
        else if (cieq(nname3,  "poly")) {
            parsepoly(current, &line, 0, groundname);
            delete [] nname3;
            polyline = line;
        }
        else {
            sCKTnode *node3;
            ckt->termInsert(&nname3, &node3);
            char *nname4 = IP.getTok(&line, true);
            if (!nname4) {
                IP.logError(current, mesg);
                ckt->CKTnogo = true;
                return;
            }
            sCKTnode *node4;
            ckt->termInsert(&nname4, &node4);
            error = node3->bind(fast, 3);
            IP.logError(current, error);
            error = node4->bind(fast, 4);
            IP.logError(current, error);

            // set alias for "x" in expr
            *buf = '\0';
            // makeSnode() in inpptree.cc will not try to evaluate strings
            // starting with '\\', so if we have a node named "c" it won't
            // be replaced with "2299792500", for example.
            if (!cieq(nname3, groundname) && !cieq(nname4, groundname))
                sprintf(buf, "v(\"\\%s\", \"\\%s\")", nname3, nname4);
            else if (!cieq(nname3, groundname))
                sprintf(buf, "v(\"\\%s\")", nname3);
            else if (!cieq(nname4, groundname))
                sprintf(buf, "(-v(\"\\%s\"))", nname4);

            if (strlen(buf))
                xalias = buf;
        }
        leadname = "gain";
    }
    else if (*dvkey == 'F' || *dvkey == 'H') {
        // control device is optional with function
        lptr = line;
        char *nm = IP.getTok(&line, true);
        if (!nm) {
            IP.logError(current, mesg);
            ckt->CKTnogo = true;
            return;
        }
        if (cieq(nm, "poly")) {
            parsepoly(current, &line, 1, groundname);
            polyline = line;
        }
        else if (cieq(nm, "function"))
            line = lptr;
        else if (cieq(nm, "vol")) {
            if (*dvkey == 'F')
                IP.logError(current, fvmsg);
            line = lptr;
        }
        else if (cieq(nm, "cur")) {
            if (*dvkey == 'H')
                IP.logError(current, fcmsg);
            line = lptr;
        }
        else {
            line = lptr;
            IFdata data;
            data.type = IF_INSTANCE;
            if (!IP.getValue(&line, &data, ckt)) {
                IP.logError(current,
                    "Error: failed to find controlling device");
                ckt->CKTnogo = true;
                return;
            }
            error = fast->setParam("control", &data);
            IP.logError(current, error);

            // set alias for "x" in function
            *buf = '\0';
            if (error == OK)
                sprintf(buf, "i(\"\\%s\")", (char*)data.v.uValue);
            xalias = buf;
        }
        delete [] nm;
        leadname = "gain";
    }
    IFdata data;
    data.type = IF_INTEGER;
    data.v.iValue = *dvkey;
    error = fast->setParam("type", &data);
    IP.logError(current, error);

    if (!polyline) {
        char *s = IP.fixParentheses(line, ckt, xalias);
        if (s) {

            // Replace the line in the deck with the fixed version.
            int len = line - current->line();
            char *tmpl = new char[len + strlen(s) + 1];
            strncpy(tmpl, current->line(), len);
            strcpy(tmpl + len, s);
            delete [] s;

            sLine *a = new sLine;
            a->set_line(current->line());
            current->set_line(tmpl);
            delete [] tmpl;
            a->set_line_num(current->line_num());
            delete current->actual();
            current->set_actual(a);

            line = current->line() + len;
        }
    }
    src_parse(current, &line, ckt, fast, leadname, xalias);
    if (polyline) {
        // Replace the line in the deck with the polynomial version.

        int len = lptr - current->line();
        char *tmpl = new char[len + strlen(polyline) + 1];
        strncpy(tmpl, current->line(), len);
        strcpy(tmpl + len, polyline);
        delete [] polyline;

        sLine *a = new sLine;
        a->set_line(current->line());
        current->set_line(tmpl);
        delete [] tmpl;
        a->set_line_num(current->line_num());
        delete current->actual();
        current->set_actual(a);
    }
}


// A special parser for source specifications.  The strategy is to
// first extract the recognized keywords and variables from the
// string, then examine what is left.  If only a number remains, the
// leadval is set.  Otherwise, if anything remains, a parse tree is
// built.
//
void
SRCdev::src_parse(
    sLine *current,         // deck line
    const char **line,      // the string to parse
    sCKT *ckt,              // the circuit this device is a member of
    sGENinstance *fast,     // direct pointer to device being parsed
    const char *leadname,   // parameter name for residual
    const char *xalias)     // alias for expression
{
    char *parm;
    int error;
    char *nline = new char[strlen(*line)+1];
    char *nend = nline;
    const char *end = *line;
    const char *start = end;
    int i;
    while (**line != 0) {

        // get token, make sure we move past ')'
        start = *line;
        parm = IP.getTok(line, true);
        if (!parm) {
            delete [] nline;
            return;
        }
        while (**line == ')')
            (*line)++;
        end = *line;

        if (!isalpha(*parm)) {
            // a keyword always starts with letter
            delete [] parm;
            while (start < end)
                *nend++ = *start++;
            continue;
        }

        // Have to be careful here.  If the token is "v" or "i", it
        // could be a parameter for asrc, or it could be part of the
        // function specification.

        if ((*parm == 'v' || *parm == 'V' || *parm == 'i' || *parm == 'I') &&
                strlen(parm) == 1) {
            const char *s = start;
            while (*s != *parm) s++;
            s++;
            if (*s == '(' || *s == ')') {
                // v(something) or i(something), or i(v) (source named
                // "v"). Not a parameter.
                //
                delete [] parm;
                while (start < end)
                    *nend++ = *start++;
                continue;
            }
        }

        IFparm *p = findInstanceParm(parm, IF_SET);
        if (p) {
            delete [] parm;
            IFdata data;
            double one = 1.0;
            data.type = p->dataType;
            if (!IP.getValue(line, &data, ckt, xalias)) {
                if (cieq(p->keyword, "ac")) {
                    data.v.v.numValue = 1;
                    data.v.v.vec.rVec = &one;
                }
                else {
                    IP.logError(current, E_PARMVAL);
                    delete [] nline;
                    return;
                }
            }
            error = fast->setParam(p->id, &data);
            if (error) {
                IP.logError(current, error);
                delete [] nline;
                return;
            }
        }
        else {
            delete [] parm;
            while (start < end)
                *nend++ = *start++;
        }
    }
    *nend = '\0';

    // nline contains function spec and/or leadvals
    nend = nline;

    // count tokens
    i = 0;
    while (*nend != 0) {
        parm = IP.getTok((const char**)&nend, true);
        if (!parm)
            break;
        i++;
        delete [] parm;
        parm = 0;
    }
    nend = nline;

    if (i == 0) {
        // nothing left
        delete [] nline;
        return;
    }

    int leadcnt = 0;
    double leadvals[2];
    leadvals[0] = leadvals[1] = 0.0;

    // Something parseable left?  Can have a function and 2 leading
    // numbers, in any order - the gain param can be complex for ac.

    IFparm *p = findInstanceParm("function", IF_SET);
    if (p) {
        bool foundfunc = false;
        IFdata data;
        data.type = p->dataType;
        if (!IP.getValue((const char**)&nend, &data, ckt, xalias)) {
            IP.logError(current, E_SYNTAX);
            delete [] nline;
            return;
        }
        // If the expression is constant, evaluate it as the leading number
        if (data.v.tValue->isConst()) {
            if (data.v.tValue->eval(&leadvals[0], 0, 0, &leadcnt) != OK)
                IP.logError(current, E_SYNTAX);
            else if (leadcnt == 0)
                leadcnt++;
            delete data.v.tValue;
        }
        else {
            error = fast->setParam(p->id, &data);
            if (error) {
                IP.logError(current, error);
                delete [] nline;
                return;
            }
            foundfunc = true;
        }
        if (*nend) {
            // Something remains, could be leading number or function.
            data.type = p->dataType;
            if (!IP.getValue((const char**)&nend, &data, ckt, xalias)) {
                IP.logError(current, E_SYNTAX);
                delete [] nline;
                return;
            }
            // If the expression is constant, evaluate it as the
            // leading number.
            if (data.v.tValue->isConst()) {
                if (leadcnt) {
                    if (cieq(leadname, "dc")) {
                        char tbuf[64];
                        sprintf(tbuf, "%g", leadvals[0]);
                        IP.logError(current, "Ambiguous DC value, using %s",
                            tbuf);
                    }
                    else if (cieq(leadname, "gain") && leadcnt == 1) {
                        data.v.tValue->eval(&leadvals[1], 0, 0);
                        delete data.v.tValue;
                        leadcnt++;
                    }
                    else
                        IP.logError(current,
                            "Internal: unknown default parameter %s (ignored)",
                            leadname);
                }
                else {
                    if (data.v.tValue->eval(&leadvals[0],0,0, &leadcnt) != OK)
                        IP.logError(current, E_SYNTAX);
                    else if (leadcnt == 0)
                        leadcnt++;
                    delete data.v.tValue;
                }
            }
            else {
                if (foundfunc)
                    IP.logError(current,
                        "Syntax error, extra function (ignored)");
                else {
                    error = fast->setParam(p->id, &data);
                    if (error) {
                        IP.logError(current, error);
                        delete [] nline;
                        return;
                    }
                    foundfunc = true;
                }
            }
        }
        if (*nend) {
            // Something remains, could be leading number or function.
            data.type = p->dataType;
            if (!IP.getValue((const char**)&nend, &data, ckt, xalias)) {
                IP.logError(current, E_SYNTAX);
                delete [] nline;
                return;
            }
            // If the expression is constant, evaluate it as the
            // leading number.
            if (data.v.tValue->isConst()) {
                if (leadcnt) {
                    if (cieq(leadname, "dc")) {
                        char tbuf[64];
                        sprintf(tbuf, "%g", leadvals[0]);
                        IP.logError(current, "Ambiguous DC value, using %s",
                            tbuf);
                    }
                    else if (cieq(leadname, "gain") && leadcnt == 1) {
                        data.v.tValue->eval(&leadvals[1], 0, 0);
                        delete data.v.tValue;
                        leadcnt++;
                    }
                    else
                        IP.logError(current,
                            "Internal: unknown default parameter %s (ignored)",
                            leadname);
                }
                else {
                    leadcnt++;
                    if (data.v.tValue->eval(&leadvals[0],0,0, &leadcnt) != OK)
                        IP.logError(current, E_SYNTAX);
                    delete data.v.tValue;
                }
            }
            else {
                if (foundfunc)
                    IP.logError(current,
                        "Syntax error, extra function (ignored)");
                else {
                    error = fast->setParam(p->id, &data);
                    if (error) {
                        IP.logError(current, error);
                        delete [] nline;
                        return;
                    }
                    foundfunc = true;
                }
            }
        }
        if (*nend) {
            parm = IP.getTok((const char**)&nend, true);
            if (parm) {
                IP.logError(current, "Unknown parameter %s (ignored)", parm);
                delete [] parm;
            }
        }
        if (leadcnt) {
            if (leadcnt == 1) {
                data.type = IF_REAL;
                data.v.rValue = leadvals[0];
            }
            else {
                data.type = IF_COMPLEX;
                data.v.cValue.real = leadvals[0];
                data.v.cValue.imag = leadvals[1];
            }
            error = fast->setParam(leadname, &data);
            if (error) {
                IP.logError(current, error);
                delete [] nline;
                return;
            }
        }
    }
    else {
        // Shouldn't get here, unless the "function" keyword is missing.
        while (*nend) {
            if (!leadcnt) {
                // check for leading value after function
                IFdata data;
                if (cieq(leadname, "dc"))
                    data.type = IF_REAL;
                else if (cieq(leadname, "gain"))
                    data.type = IF_REALVEC;
                else {
                    IP.logError(current,
                        "Internal: unknown default parameter %s (ignored)",
                        leadname);
                    break;
                }
                if (!IP.getValue((const char**)&nend, &data, ckt, xalias)) {
                    IP.logError(current, E_PARMVAL);
                    delete [] nline;
                    return;
                }
                error = fast->setParam(leadname, &data);
                if (error) {
                    IP.logError(current, error);
                    delete [] nline;
                    return;
                }
                leadcnt++;
                continue;
            }
            parm = IP.getTok((const char**)&nend, true);
            if (parm) {
                IP.logError(current, "Unknown parameter %s (ignored)", parm);
                delete [] parm;
                break;
            }
        }
    }
    delete [] nline;
}


namespace {
    // POLY(N) converter.  Changes the poly specification into a
    // polynomial, which is subsequently parsed in src_parse().
    //
    // int cc; true for current controlled device
    //
    void parsepoly(sLine *current, const char **line, int cc,
        const char *groundname)
    {
        const char *mesg = "Too few values in POLY";
        char buf[512];
        char *tok[20], *vals[200];
        char *node1, *node2;
        int error = 0;
        int ndims = (int)IP.getFloat(line, &error, true);
        if (error) {
            IP.logError(current, error);
            return;
        }
        if (ndims < 0 || ndims > 20) {
            IP.logError(current, "Bad POLY dimension");
            return;
        }
        int i;
        for (i = 0; i < ndims; i++) {
            if (cc) {
                node1 = IP.getTok(line, true);
                if (!node1) {
                    IP.logError(current, mesg);
                    return;
                }
                sprintf(buf, "%s#branch", node1);
                delete [] node1;
                tok[i] = copy(buf);
            }
            else {
                node1 = IP.getTok(line, true);
                if (!node1) {
                    IP.logError(current, mesg);
                    return;
                }
                node2 = IP.getTok(line, true);
                if (!node2) {
                    IP.logError(current, mesg);
                    return;
                }
                if (cieq(node2, groundname))
                    sprintf(buf, "v(%s)", node1);
                else if (cieq(node1, groundname))
                    sprintf(buf, "(-v(%s))", node2);
                else
                    sprintf(buf, "v(%s,%s)", node1, node2);
                delete [] node1;
                delete [] node2;
                tok[i] = copy(buf);
            }
        }
        i = 0;
        while (**line && !isalpha(**line)) {
            node1 = IP.getTok(line, true);
            if (node1) {
                if (i >= 200) {
                    IP.logError(current, "Too many values in POLY");
                    return;
                }
                vals[i] = node1;
                i++;
            }
        }
        char *s = polystring(vals, i, tok, ndims);
        if (**line)
            s = build_str(s, *line - 1); // include separator
        *line = s;
        while (i--)
            delete [] vals[i];
        while (ndims--)
            delete [] tok[ndims];
    }


    // Return a string built from the arguments representing a
    // polynomial.
    //
    char *polystring(char **vals, int numvals, char **args, int numargs)
    {
        char buf[512];
        char *string = 0;
        if (atof(vals[0]) != 0.0)
            string = copy(vals[0]);
        if (numvals > 1) {
            int *exp = new int[numargs];
            int i;
            for (i = 0; i < numargs; i++)
                exp[i] = 0;

            for (i = 1; i < numvals; ++i) {
                nxtpwr(exp, &numargs);
                double v = atof(vals[i]);
                if (v != 0.0) {
                    if (string) {
                        if (v > 0)
                            sprintf(buf, " + %s*", vals[i]);
                        else
                            sprintf(buf, " - %s*", vals[i]+1);
                    }
                    else
                        sprintf(buf, "%s*", vals[i]);
                    int first = 1;
                    for (int j = 0; j < numargs; ++j) {
                        if (exp[j] == 1) {
                            if (!first)
                                sprintf(buf + strlen(buf), "*");
                            first = 0;
                            sprintf(buf + strlen(buf), "%s", args[j]);
                        }
                        else if (exp[j]) {
                            if (!first)
                                sprintf(buf + strlen(buf), "*");
                            sprintf(buf + strlen(buf), "%s", args[j]);
                            for (int k = 1; k < exp[j]; k++)
                                sprintf(buf + strlen(buf), "*%s", args[j]);
                            first = 0;
                        }
                    }
                    string = build_str(string, buf);
                }
            }
            delete [] exp;
        }
        if (numvals == 1 && numargs == 1 && string) {
            // if one coeff and one dimension, assume linear coeff
            sprintf(buf, "*%s", args[0]);
            string = build_str(string, buf);
        }
        if (!string)
            string = copy("0");
        // add "function"
        char *str = copy("function ");
        str = build_str(str, string);
        delete [] string;
        return (str);
    }


    // From SPICE2
    // This function determines the 'next' set of exponents for the
    // different dimensions of a polynomial.
    //
    int nxtpwr(int *pwrseq, int *pdim)
    {
        // System generated locals
        int i__1;

        // Local variables
        int psum, i, k, km1;

        // Parameter adjustments
        --pwrseq;

        // Function Body
        if (*pdim == 1)
            goto L80;
        k = *pdim;
    L10:
        if (pwrseq[k] != 0)
            goto L20;
        --k;
        if (k != 0)
            goto L10;
        goto L80;
    L20:
        if (k == *pdim)
            goto L30;
        --pwrseq[k];
        ++pwrseq[k + 1];
        goto L100;
    L30:
        km1 = k - 1;
        i__1 = km1;
        for (i = 1; i <= i__1; ++i) {
            if (pwrseq[i] != 0)
                goto L50;
        }
        pwrseq[1] = pwrseq[*pdim] + 1;
        pwrseq[*pdim] = 0;
        goto L100;
    L50:
        psum = 1;
        k = *pdim;
    L60:
        if (pwrseq[k - 1] >= 1)
            goto L70;
        psum += pwrseq[k];
        pwrseq[k] = 0;
        --k;
        goto L60;
    L70:
        pwrseq[k] += psum;
        --pwrseq[k - 1];
        goto L100;
    L80:
        ++pwrseq[1];

    L100:
        return 0;
    }
}


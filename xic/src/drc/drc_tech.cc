
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
 * Xic Integrated Circuit Layout and Schematic Editor                     *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#include "main.h"
#include "drc.h"
#include "drc_kwords.h"
#include "dsp_layer.h"
#include "tech.h"
#include "tech_layer.h"
#include "tech_spacetab.h"
#include "tech_kwords.h"
#include "cd_lgen.h"
#include "si_parser.h"
#include "errorlog.h"
#include <algorithm>


//-----------------------------------------------------------------------------
// Technology file read/write functions

// Instantiate the keyword repository.
sDrcKW Dkw;

// These two function link and unlink a rule from its layer.  The
// order of the list is important, and is maintained by these
// functions.  These should be used exclusively to add/remove rules
// from a layer.
//
// Note also that the layer for the rule is specified when a rule is
// created.  A rule can never be re-linked to a different layer.

void
cDRC::linkRule(DRCtestDesc *rule)
{
    if (!rule)
        return;

    // Let's be paranoid and make sure that it is not already linked.
    DRCtestDesc *td = *tech_prm(rule->layer())->rules_addr();
    for ( ; td; td = td->next()) {
        if (td == rule)
            return;
    }

    // We want to link new rules at the end of a group of similar
    // rules, thus keeping file order.

    td = *tech_prm(rule->layer())->rules_addr();
    if (!td || rule->type() < td->type()) {
        rule->setNext(td);
        *tech_prm(rule->layer())->rules_addr() = rule;
        return;
    }
    DRCtestDesc *tdp = 0;
    for ( ; td; td = td->next()) {
        if (rule->type() < td->type()) {
            rule->setNext(td);
            if (tdp)
                tdp->setNext(rule);
            else
                *tech_prm(rule->layer())->rules_addr() = rule;
            return;
        }
        tdp = td;
    }
    rule->setNext(0);
    tdp->setNext(rule);
}


// If rule is found, unlink it and return it, otherwise return 0.
//
DRCtestDesc *
cDRC::unlinkRule(DRCtestDesc *rule)
{
    DRCtestDesc *td = *tech_prm(rule->layer())->rules_addr();
    DRCtestDesc *tdp = 0;
    for ( ; td; td = td->next()) {
        if (td == rule) {
            if (tdp)
                tdp->setNext(td->next());
            else
                *tech_prm(rule->layer())->rules_addr() = td->next();
            td->setNext(0);
            return (td);
        }
        tdp = td;
    }
    return (0);
}


//
// Functions to create rules from Virtuoso constraint groups.
//

bool
cDRC::addMaxWidth(CDl *ld, double val, const char *cmt)
{
    DRCtestDesc *td = DRCtestDesc::findRule(ld, drMaxWidth, 0);
    bool exists = (td != 0);
    if (!exists)
        td = new DRCtestDesc(drMaxWidth, ld);
    td->setDimen(INTERNAL_UNITS(val));
    td->setInfo(0, cmt);
    if (!exists)
        linkRule(td);
    return (true);
}


bool
cDRC::addMinWidth(CDl *ld, double val, const char *cmt)
{
    // CDS MinWidth <-> XIC MinWidth

    // Need to update the minWidth test to use diag min width.
    // done.

    DRCtestDesc *td = DRCtestDesc::findRule(ld, drMinWidth, 0);
    bool exists = (td != 0);
    if (!exists)
        td = new DRCtestDesc(drMinWidth, ld);
    td->setDimen(INTERNAL_UNITS(val));
    td->setInfo(0, cmt);
    if (!exists)
        linkRule(td);
    return (true);
}


bool
cDRC::addMinDiagonalWidth(CDl *ld, double val, const char *cmt)
{
    // Apply this when the test vector is not orthogonal.

    DRCtestDesc *td = DRCtestDesc::findRule(ld, drMinWidth, 0);
    bool exists = (td != 0);
    if (!exists)
        td = new DRCtestDesc(drMinWidth, ld);
    td->setValue(0, INTERNAL_UNITS(val));
    td->setInfo(1, cmt);
    if (!exists)
        linkRule(td);
    return (true);
}


bool
cDRC::addMinSpacing(CDl *ld, CDl *ld2, double val, const char *cmt)
{
    // CDS MinSpacing <-> XIC MinSpace  (one layer)
    // CDS MinSpacing <-> XIC MinSpaceTo  (two layers)

    // Need to update the MinSpace test to
    // 1. use tables
    // 2. use diagonal
    // 3. same net vs. not

    if (ld2) {
        DRCtestDesc *td = DRCtestDesc::findRule(ld, drMinSpaceTo, ld2);
        bool exists = (td != 0);
        if (!exists) {
            td = new DRCtestDesc(drMinSpaceTo, ld);
            td->setTargetLayer(ld2);
        }
        td->setDimen(INTERNAL_UNITS(val));
        td->setInfo(0, cmt);
        if (!exists)
            linkRule(td);
    }
    else {
        DRCtestDesc *td = DRCtestDesc::findRule(ld, drMinSpace, 0);
        bool exists = (td != 0);
        if (!exists)
            td = new DRCtestDesc(drMinSpace, ld);
        td->setDimen(INTERNAL_UNITS(val));
        td->setInfo(0, cmt);
        if (!exists)
            linkRule(td);
    }
    return (true);
}


bool
cDRC::addMinSameNetSpacing(CDl *ld, CDl *ld2, double val, const char *cmt)
{
    // For MinSpace/MinSpaceTo, value[0] is diagonal spacing, value[1]
    // is same net spacing.

    if (ld2) {
        DRCtestDesc *td = DRCtestDesc::findRule(ld, drMinSpaceTo, ld2);
        bool exists = (td != 0);
        if (!exists) {
            td = new DRCtestDesc(drMinSpaceTo, ld);
            td->setTargetLayer(ld2);
        }
        td->setValue(1, INTERNAL_UNITS(val));
        td->setInfo(2, cmt);
        if (!exists)
            linkRule(td);
    }
    else {
        DRCtestDesc *td = DRCtestDesc::findRule(ld, drMinSpace, 0);
        bool exists = (td != 0);
        if (!exists)
            td = new DRCtestDesc(drMinSpace, ld);
        td->setValue(1, INTERNAL_UNITS(val));
        td->setInfo(2, cmt);
        if (!exists)
            linkRule(td);
    }
    return (true);
}


bool
cDRC::addMinDiagonalSpacing(CDl *ld, CDl *ld2, double val, const char *cmt)
{
    // Apply this when the test vector is not orthogonal.
    //
    // For MinSpace/MinSpace2, value[0] is diagonal spacing, value[1]
    // is same net spacing.

    if (ld2) {
        DRCtestDesc *td = DRCtestDesc::findRule(ld, drMinSpaceTo, ld2);
        bool exists = (td != 0);
        if (!exists) {
            td = new DRCtestDesc(drMinSpaceTo, ld);
            td->setTargetLayer(ld2);
        }
        td->setValue(0, INTERNAL_UNITS(val));
        td->setInfo(1, cmt);
        if (!exists)
            linkRule(td);
    }
    else {
        DRCtestDesc *td = DRCtestDesc::findRule(ld, drMinSpace, 0);
        bool exists = (td != 0);
        if (!exists)
            td = new DRCtestDesc(drMinSpace, ld);
        td->setValue(0, INTERNAL_UNITS(val));
        td->setInfo(1, cmt);
        if (!exists)
            linkRule(td);
    }
    return (true);
}


bool
cDRC::addMinSpaceTable(CDl *ld, CDl *ld2, sTspaceTable *st)
{
    if (ld2) {
        DRCtestDesc *td = DRCtestDesc::findRule(ld, drMinSpaceTo, ld2);
        bool exists = (td != 0);
        if (!exists) {
            td = new DRCtestDesc(drMinSpaceTo, ld);
            td->setTargetLayer(ld2);
        }
        td->setDimen(st->dimen);
        td->setSpaceTab(st);
        if (!exists)
            linkRule(td);
    }
    else {
        DRCtestDesc *td = DRCtestDesc::findRule(ld, drMinSpace, 0);
        bool exists = (td != 0);
        if (!exists)
            td = new DRCtestDesc(drMinSpace, ld);
        td->setDimen(st->dimen);
        tech_prm(ld)->set_spacing(st->dimen);
        td->setSpaceTab(st);
        if (!exists)
            linkRule(td);
    }
    return (true);
}


bool
cDRC::addMinArea(CDl *ld, double val, const char *cmt)
{
    // CDS MinArea <-> XIC MinArea

    DRCtestDesc *td = DRCtestDesc::findRule(ld, drMinArea, 0);
    bool exists = (td != 0);
    if (!exists)
        td = new DRCtestDesc(drMinArea, ld);
    td->setArea(val);
    td->setInfo(0, cmt);
    if (!exists)
        linkRule(td);
    return (true);
}


bool
cDRC::addMinHoleArea(CDl *ld, double val, const char *cmt)
{
    DRCtestDesc *td = DRCtestDesc::findRule(ld, drNoHoles, 0);
    bool exists = (td != 0);
    if (!exists)
        td = new DRCtestDesc(drNoHoles, ld);
    td->setArea(val);
    td->setInfo(1, cmt);
    if (!exists)
        linkRule(td);
    return (true);
}


bool
cDRC::addMinHoleWidth(CDl *ld, double val, const char *cmt)
{
    DRCtestDesc *td = DRCtestDesc::findRule(ld, drNoHoles, 0);
    bool exists = (td != 0);
    if (!exists)
        td = new DRCtestDesc(drNoHoles, ld);
    td->setValue(0, INTERNAL_UNITS(val));
    td->setInfo(2, cmt);
    if (!exists)
        linkRule(td);
    return (true);
}


bool
cDRC::addMinExtension(CDl *ld, CDl *ld2, double val, const char *cmt)
{
    // CDS MinExtension <-> XIC MinOverlap

    DRCtestDesc *td = DRCtestDesc::findRule(ld, drMinOverlap, ld2);
    bool exists = (td != 0);
    if (!exists) {
        td = new DRCtestDesc(drMinOverlap, ld);
        td->setTargetLayer(ld2);
    }
    td->setDimen(INTERNAL_UNITS(val));
    td->setInfo(0, cmt);
    if (!exists)
        linkRule(td);
    return (true);
}


// Both minEnclosure is the same as minOppExtension with the same
// value given twice.  They would map to MinNoOverlap, however, it is
// better to swap ld and ld2 and map these to MinSpaceFrom.  This will
// simplify rule evaluation since we are then iterating over the edges
// of the object whose edges are of primary interest, i.e., the
// enclosed object.  So, MinSpaceFrom carries the following values:
//
//   dimen                Extension value.
//   value[0]             Overrides dimen when full enclosure.
//   value[1], value[2]   Opposite side constraint for full
//                        enclosure or overlap, overrides value[0]
//                        when given.
//


bool
cDRC::addMinEnclosure(CDl *ld, CDl *ld2, double val, const char *cmt)
{
    // CDS MinEnclosure <-> XIC MinSpaceFrom with layer swap

    CDl *ltmp = ld;
    ld = ld2;
    ld2 = ltmp;

    DRCtestDesc *td = DRCtestDesc::findRule(ld, drMinSpaceFrom, ld2);
    bool exists = (td != 0);
    if (!exists) {
        td = new DRCtestDesc(drMinSpaceFrom, ld);
        td->setTargetLayer(ld2);
    }
    td->setValue(0, INTERNAL_UNITS(val));
    td->setInfo(1, cmt);
    if (!exists)
        linkRule(td);
    return (true);
}


bool
cDRC::addMinOppExtension(CDl *ld, CDl *ld2, double val1, double val2,
    const char *cmt)
{
    // For MinSpaceFrom, value[1] and value[2] are the two opposite
    // side extensions.

    CDl *ltmp = ld;
    ld = ld2;
    ld2 = ltmp;

    DRCtestDesc *td = DRCtestDesc::findRule(ld, drMinSpaceFrom, ld2);
    bool exists = (td != 0);
    if (!exists) {
        td = new DRCtestDesc(drMinSpaceFrom, ld);
        td->setTargetLayer(ld2);
    }
    td->setValue(1, INTERNAL_UNITS(val1));
    td->setValue(2, INTERNAL_UNITS(val2));
    td->setInfo(2, cmt);
    if (!exists)
        linkRule(td);
    return (true);
}
// End of Virtuoso constraint support functions.


// Handle rule parsing.
//
TCret
cDRC::techParseLayer(CDl *ldesc)
{
    const char *kw = Tech()->KwBuf();
    const char *text = Tech()->InBuf();
    const char *otext = Tech()->OrigBuf();

    DRCtype rnum = DRCtestDesc::ruleType(kw);
    if (rnum != drNoRule) {
        TCret tcret = Tech()->CheckLD(true);
        if (tcret != TCnone)
            return (tcret);

        DRCtestDesc *td = 0;
        if (rnum == drMinSpace) {
            // Look for an existing rule, may have been created in
            // spacing table parse.
            for (DRCtestDesc *d = *tech_prm(ldesc)->rules_addr(); d;
                    d = d->next()) {
                if (d->type() == drMinSpace) {
                    td = d;
                    break;
                }
            }
        }
        bool newone = false;
        if (!td) {
            td = new DRCtestDesc(rnum, ldesc);
            newone = true;
        }

        const char *emsg = td->parse(text, otext);
        if (!emsg) {
            if (newone)
                linkRule(td);
        }
        else {
            if (newone)
                delete td;
            return (Tech()->SaveError("%s: layer %s, bad spefication, %s.",
                kw,  ldesc->name(), emsg));
        }
        return (TCmatch);
    }

    DRCtest *tst;
    for (tst = drc_user_tests; tst; tst = tst->next()) {
        if (lstring::cieq(kw, tst->name()))
            break;
    }
    if (!tst)
        return (TCnone);
    TCret tcret = Tech()->CheckLD(true);
    if (tcret != TCnone)
        return (tcret);

    DRCtestDesc *td = new DRCtestDesc(drUserDefinedRule, ldesc);
    const char *emsg = td->parse(text, otext, tst);
    if (!emsg)
        linkRule(td);
    else {
        delete td;
        return (Tech()->SaveError("%s: layer %s, bad reference, %s.",
            kw,  ldesc->name(), emsg));
    }
    return (TCmatch);
}


// Parse the provided DRC test definition, and if successful add the
// new struct to the drc_user_tests list, and return true.  The
// DrcTest line may or may not have been loaded, depending if we are
// parsing the tech file, or reading a file stand-alone (not loaded
// case).
//
bool
cDRC::techParseUserRule(FILE *fp, const char **rname)
{
    if (rname)
        *rname = 0;

    Tech()->BeginParse();
    if (!Tech()->Matching(Dkw.DrcTest()))
        Tech()->GetKeywordLine(fp);
    if (!Tech()->Matching(Dkw.DrcTest())) {
        // Get here opening a stand-alone file ony.
        Log()->WarningLogV(mh::Techfile, "Not a User Rule block.\n");

        Tech()->EndParse();
        return (false);
    }

    bool nogo = false;
    DRCtest *tst = 0;
    const char *inbuf = Tech()->InBuf();
    const char *line = inbuf;
    char *tok = lstring::gettok(&line);
    if (!tok) {
        char *e = Tech()->SaveError("%s: missing name.", Dkw.DrcTest());
        Log()->WarningLogV(mh::Techfile, "%s\n", e);
        delete [] e;
        nogo = true;
        Tech()->SetCmtType(tBlkNone, 0);
    }
    else {
        const char *name = tok;
        tst = new DRCtest(tok);
        char *tmp[DRC_MAX_USER_ARGS];
        int i = 0;
        while ((tok = lstring::gettok(&line)) != 0) {
            if (i == DRC_MAX_USER_ARGS) {
                char *e = Tech()->SaveError("%s: too many args, %d max.",
                    Dkw.DrcTest(), DRC_MAX_USER_ARGS);
                Log()->WarningLogV(mh::Techfile, "%s\n", e);
                delete [] e;
                nogo = true;
                Tech()->SetCmtType(tBlkNone, 0);
                delete [] tok;
                break;
            }
            tmp[i++] = tok;
        }
        if (!nogo) {
            char **av = new char*[i+1];
            int ac = i;
            av[i] = 0;
            while (i--)
                av[i] = tmp[i];
            tst->setArgs(ac, av);
            Tech()->SetCmtType(tBlkRule, name);
        }
    }

    struct DRCedgeCnd *edgend = 0;
    struct DRCtestCnd *tstend = 0;
    int tstcnt = 0;
    const char *kw;
    while ((kw = Tech()->GetKeywordLine(fp)) != 0) {
        line = inbuf;
        if (Tech()->Matching(Dkw.Edge())) {
            if (nogo)
                continue;
            tok = lstring::gettok(&line);
            if (!tok) {
                char *e = Tech()->SaveError("%s: missing token.", Dkw.Edge());
                Log()->WarningLogV(mh::Techfile, "%s\n", e);
                delete [] e;
                nogo = true;
                continue;
            }
            DRCwhereType where = DRCoutside;
            if (*tok == 'o' || *tok == 'O')
                where = DRCoutside;
            else if (*tok == 'i' || *tok == 'I')
                where = DRCinside;
            else {
                char *e = Tech()->SaveError("%s: unknown token %s.",
                    Dkw.Edge(), tok);
                Log()->WarningLogV(mh::Techfile, "%s\n", e);
                delete [] e;
                nogo = true;
            }
            delete [] tok;
            if (nogo)
                continue;

            if (*line) {
                if (!edgend) {
                    edgend = new DRCedgeCnd(where, line);
                    tst->setEdges(edgend);
                }
                else {
                    edgend->setNext(new DRCedgeCnd(where, line));
                    edgend = edgend->next();
                }
            }
            else {
                char *e = Tech()->SaveError("%s: missing expression.",
                    Dkw.Edge());
                Log()->WarningLogV(mh::Techfile, "%s\n", e);
                delete [] e;
                nogo = true;
            }
        }
        else if (Tech()->Matching(Dkw.Test())) {
            if (nogo)
                continue;
            if (tstcnt >= DRC_MAX_USER_TEST_SPECS) {
                char *e = Tech()->SaveError(
                    "%s: too many test specs, max %d.", Dkw.Test(),
                    DRC_MAX_USER_TEST_SPECS);
                Log()->WarningLogV(mh::Techfile, "%s\n", e);
                delete [] e;
                nogo = true;
                continue;
            }
            tok = lstring::gettok(&line);
            if (!tok) {
                char *e = Tech()->SaveError("%s: missing token.", Dkw.Test());
                Log()->WarningLogV(mh::Techfile, "%s\n", e);
                delete [] e;
                nogo = true;
                continue;
            }
            DRCwhereType where = DRCoutside;
            if (*tok == 'o' || *tok == 'O')
                where = DRCoutside;
            else if (*tok == 'i' || *tok == 'I')
                where = DRCinside;
            else {
                char *e = Tech()->SaveError("%s: unknown token %s.",
                    Dkw.Test(), tok);
                Log()->WarningLogV(mh::Techfile, "%s\n", e);
                delete [] e;
                nogo = true;
            }
            delete [] tok;
            if (nogo)
                continue;

            if (*line) {
                if (!tstend) {
                    tstend = new DRCtestCnd(where, 0, line);
                    tst->setTests(tstend);
                }
                else {
                    tstend->setNext(new DRCtestCnd(where, 0, line));
                    tstend = tstend->next();
                }
                tstcnt++;
            }
            else {
                char *e = Tech()->SaveError("%s: missing expression.",
                    Dkw.Test());
                Log()->WarningLogV(mh::Techfile, "%s\n", e);
                delete [] e;
                nogo = true;
            }
        }
        else if (Tech()->Matching(Dkw.Evaluate())) {
            if (nogo)
                continue;
            if (tst->fail()) {
                char *e = Tech()->SaveError("%s: already specified, ignored.",
                    Dkw.Evaluate());
                Log()->WarningLogV(mh::Techfile, "%s\n", e);
                delete [] e;
            }
            else {
                if (*line)
                    tst->setFail(line);
                else {
                    sLstr lstr;
                    while (Tech()->GetKeywordLine(fp, tNoKeyword) != 0) {
                        line = Tech()->InBuf();
                        if (lstring::ciprefix("ENDSCRIPT", line))
                            break;
                        lstr.add(line);
                    }
                    tst->setFail(lstr.string());
                }
            }
        }
        else if (Tech()->Matching(Dkw.MinEdge())) {
            if (nogo)
                continue;
            if (!*line) {
                char *e = Tech()->SaveError("%s: missing dimension, ignored.",
                    Dkw.MinEdge());
                Log()->WarningLogV(mh::Techfile, "%s\n", e);
                delete [] e;
            }
            else
                tst->setEminStr(line);
        }
        else if (Tech()->Matching(Dkw.MaxEdge())) {
            if (nogo)
                continue;
            if (!*line) {
                char *e = Tech()->SaveError("%s: missing dimension, ignored.",
                    Dkw.MaxEdge());
                Log()->WarningLogV(mh::Techfile, "%s\n", e);
                delete [] e;
            }
            else
                tst->setEmaxStr(line);
        }
        else if (Tech()->Matching(Dkw.TestCornerOverlap())) {
            if (nogo)
                continue;
            if (!*line) {
                char *e = Tech()->SaveError("%s: missing dimension, ignored.",
                    Dkw.TestCornerOverlap());
                Log()->WarningLogV(mh::Techfile, "%s\n", e);
                delete [] e;
            }
            else
                tst->setCotStr(line);
        }
        else if (lstring::ciprefix(Dkw.End(), kw))
            break;
        else {
            char *e = Tech()->SaveError("Unknown keyword %s, ignored.", kw);
            Log()->WarningLogV(mh::Techfile, "%s\n", e);
            delete [] e;
        }
    }
    Tech()->EndParse();

    if (nogo) {
        delete tst;
        return (false);
    }

    if (!tst->fail()) {
        Log()->WarningLogV(mh::Techfile,
            "No %s given in block ending on line %d.\n",
            Dkw.Evaluate(), Tech()->LineCount());
        delete tst;
        return (false);
    }
    if (!tst->tests()) {
        Log()->WarningLogV(mh::Techfile,
            "No %s given in block ending on line %d.\n",
            Dkw.Test(), Tech()->LineCount());
        delete tst;
        return (false);
    }

    // We're good, so return the name.
    if (rname)
        *rname = tst->name();

    if (!drc_user_tests)
        drc_user_tests = tst;
    else {
        // The drc editor allows these to be redefined.
        DRCtest *tp = 0;
        for (DRCtest *t = drc_user_tests; t; t = t->next()) {
            if (!strcmp(t->name(), tst->name())) {
                if (!tp)
                    drc_user_tests = tst;
                else
                    tp->setNext(tst);
                tst->setNext(t->next());

                // All instances of the old rule are inhibited.
                CDl *ld;
                CDlgenDrv gen;
                while ((ld = gen.next()) != 0) {
                    for (DRCtestDesc *td = *tech_prm(ld)->rules_addr(); td;
                            td = td->next()) {
                        if (td->matchUserRule(tst->name()))
                            td->setInhibited(true);
                    }
                }
                delete t;
                return (true);
            }
            tp = t;
        }
        tp->setNext(tst);
    }
    return (true);
}


// Print the rule from ld to fp or lstr.  Called when updating the
// technology file.  If cmts, print any associated comments.  If
// show_inh, print the inhibited status "I " or "  " in the first two
// columns.  If use_orig, use the original string from the tech file
// if available.
//
void
cDRC::techPrintLayer(FILE *fp, sLstr *lstr, bool cmts, const CDl *ld,
    bool show_inh, bool use_orig)
{
    if (!ld)
        return;
    for (DRCtestDesc *td = *tech_prm(ld)->rules_addr(); td; td = td->next())
        td->print(fp, lstr, cmts, ld, show_inh, use_orig);
}


// Dump the rule blocks to fp, used when updating technology file.
//
void
cDRC::techPrintUserRules(FILE *fp)
{
    const char *sep =
        "#-------------------------------------------------------------"
        "-----------------\n";

    if (drc_user_tests) {
        fputs(sep, fp);
        fputs("# User-Defined Design Rule Tests\n", fp);
        fputs("\n", fp);
    }
    for (DRCtest *tst = drc_user_tests; tst; tst = tst->next()) {
        sLstr lstr;
        tst->print(&lstr);
        fputs(lstr.string(), fp);
        fputs("\n", fp);
        lstr.free();
    }
    if (drc_user_tests) {
        fputs(sep, fp);
        fputs("\n", fp);
    }
}


namespace {
    void crstr(sLstr &lstr, const char *pre, const char *name, const CDl *ld,
        const CDl *ld2, const char *cmt, double val, bool twov = false,
        double val2 = 0.0)
    {
        char buf[256];

        lstr.add(pre);
        lstr.add("( ");
        sprintf(buf, "%-22s", name);
        lstr.add(buf);
        lstr.add_c(' ');

        if (ld) {
            char *e = buf;
            *e++ = '"';
            e = lstring::stpcpy(e, ld->name());
            *e++ = '"';
            while (e - buf < 8)
                *e++ = ' ';
            *e = 0;
            lstr.add(buf);
            lstr.add_c(' ');
        }
        if (ld2) {
            char *e = buf;
            *e++ = '"';
            e = lstring::stpcpy(e, ld2->name());
            *e++ = '"';
            while (e - buf < 8)
                *e++ = ' ';
            *e = 0;
            lstr.add(buf);
            lstr.add_c(' ');
        }

        if (twov) {
            lstr.add_c('(');
            lstr.add_g(val);
            lstr.add_c(' ');
            lstr.add_g(val2);
            lstr.add_c(')');
        }
        else
            lstr.add_g(val);
        if (cmt && *cmt) {
            lstr.add(" 'ref \"");
            lstr.add(cmt);
            lstr.add_c('"');
        }
        lstr.add(" )\n");
    }
}


// Print the spacings list, for use when writing a Cadence tech file.
//
char *
cDRC::spacings(const char *pref)
{
    sLstr lstr;
    CDl *ld;
    CDlgenDrv gen;
    while ((ld = gen.next()) != 0) {
        for (DRCtestDesc *td = *tech_prm(ld)->rules_addr(); td;
                td = td->next()) {
            DRCtype type = td->type();
            if (type == drNoHoles) {
                if (td->area() > 0.0) {
                    crstr(lstr, pref, "minHoleArea", ld, 0, td->info(1),
                        td->area());
                }
                if (td->hasValue(0)) {
                    crstr(lstr, pref, "minHoleWidth", ld, 0, td->info(2),
                        td->value(0));
                }
            }
            else if (type == drMinArea) {
                crstr(lstr, pref, "minArea", ld, 0, td->info(0), td->area());
            }
            else if (type == drMaxArea) {
            }
            else if (type == drMinEdgeLength) {
            }
            else if (type == drMaxWidth) {
                crstr(lstr, pref, "maxWidth", ld, 0, td->info(0),
                    MICRONS(td->dimen()));
            }
            else if (type == drMinWidth) {
                if (td->dimen() > 0) {
                    crstr(lstr, pref, "minWidth", ld, 0, td->info(0),
                        MICRONS(td->dimen()));
                }
                if (td->hasValue(0)) {
                    crstr(lstr, pref, "minDiagonalWidth", ld, 0, td->info(1),
                        MICRONS(td->value(0)));
                }
            }
            else if (type == drMinSpace) {
                if (td->dimen() > 0) {
                    crstr(lstr, pref, "minSpacing", ld, 0, td->info(0),
                        MICRONS(td->dimen()));
                }
                if (td->hasValue(0)) {
                    crstr(lstr, pref, "minDiagonalSpacing", ld, 0, td->info(1),
                        MICRONS(td->value(0)));
                }
                if (td->hasValue(1)) {
                    crstr(lstr, pref, "minSameNetSpacing", ld, 0, td->info(2),
                        MICRONS(td->value(1)));
                }
            }
            else if (type == drMinSpaceTo) {
                // Yes, this goes here, not in orderedSpacings.
                CDl *ld2 = td->targetLayer();
                if (!ld2)
                    continue;
                if (td->dimen() > 0) {
                    crstr(lstr, pref, "minSpacing", ld, ld2,  td->info(0),
                        MICRONS(td->dimen()));
                }
                if (td->hasValue(0)) {
                    crstr(lstr, pref, "minDiagonalSpacing", ld, ld2,
                        td->info(1), MICRONS(td->value(0)));
                }
                if (td->hasValue(1)) {
                    crstr(lstr, pref, "minSameNetSpacing", ld, ld2,
                        td->info(2), MICRONS(td->value(1)));
                }
            }
        }
    }
    return (lstr.string_trim());
}


char *
cDRC::orderedSpacings(const char *pref)
{
    sLstr lstr;
    CDl *ld;
    CDlgenDrv gen;
    while ((ld = gen.next()) != 0) {
        for (DRCtestDesc *td = *tech_prm(ld)->rules_addr(); td;
                td = td->next()) {
            CDl *ld2 = td->targetLayer();
            if (!ld2)
                continue;
            DRCtype type = td->type();
            if (type == drMinSpaceFrom) {
                if (td->hasValue(0)) {
                    crstr(lstr, pref, "minEnclosure", ld2, ld, td->info(1),
                        MICRONS(td->value(0)));
                }
                if (td->hasValue(1) || td->hasValue(2)) {
                    crstr(lstr, pref, "minOppExtension", ld2, ld, td->info(2),
                        MICRONS(td->value(1)), true, MICRONS(td->value(2)));
                }
            }
            else if (type == drMinOverlap) {
                crstr(lstr, pref, "minExtension", ld, ld2, td->info(0),
                    MICRONS(td->dimen()));
            }
        }
    }
    return (lstr.string_trim());
}


// Compose a string containing the spacing table minSpacing records.
//
char *
cDRC::minSpaceTables(const char *pref)
{
    sLstr lstr;
    CDl *ld;
    CDlgenDrv gen;
    while ((ld = gen.next()) != 0) {
        for (DRCtestDesc *td = *tech_prm(ld)->rules_addr(); td;
                td = td->next()) {
            if (td->type() == drMinSpace) {
                const sTspaceTable *tab = td->spaceTab();
                if (!tab)
                    continue;
                char *str = sTspaceTable::to_lisp_string(tab, pref,
                    ld->name(), 0);
                if (str) {
                    lstr.add(str);
                    delete [] str;
                }
            }
            else if (td->type() == drMinSpaceTo) {
                const sTspaceTable *tab = td->spaceTab();
                if (!tab)
                    continue;
                CDl *ld2 = td->targetLayer();
                char *str = sTspaceTable::to_lisp_string(tab, pref, ld->name(),
                    ld2 ? ld2->name() : "unknown");
                if (str) {
                    lstr.add(str);
                    delete [] str;
                }
            }
        }
    }
    return (lstr.string_trim());
}
// End of cDRC functions.


// Parse the specification string, the rule keyword has been stripped. 
// The DRCtest must be passed if td_type is drUserDefinedRule.  The
// return is null on success, otherwise a static error phrase is
// provided.
//
const char *
DRCtestDesc::parse(const char *text, const char *origstring, DRCtest *tst)
{
    if (origstring && *origstring) {
        char *t = lstring::copy(origstring);
        delete [] td_origstring;
        td_origstring = t;
    }

    switch (td_type) {
    case drNoRule:
        return ("incorrect rule type");

    case drConnected:
        return (parseConnected(text));
    case drNoHoles:
        return (parseNoHoles(text));

    case drExist:
        parseComment(text);
        return (0);

    case drOverlap:
    case drIfOverlap:
    case drNoOverlap:
    case drAnyOverlap:
    case drPartOverlap:
    case drAnyNoOverlap:
        return (parseXoverlap(text));

    case drMinArea:
    case drMaxArea:
        return (parseXarea(text));

    case drMinEdgeLength:
        return (parseMinEdgeLength(text));

    case drMaxWidth:
        return (parseMaxWidth(text));
    case drMinWidth:
        return (parseMinWidth(text));

    case drMinSpace:
    case drMinSpaceTo:
        return (parseMinSpaceX(text));
    case drMinSpaceFrom:
        return (parseMinSpaceFrom(text));

    case drMinOverlap:
    case drMinNoOverlap:
        return (parseMinXoverlap(text));

    case drUserDefinedRule:
        return (parseUserDefinedRule(text, tst));
    }
    return (0);
}


namespace {
    void putstr(FILE *fp, sLstr *lstr, const char *s)
    {
        if (lstr)
            lstr->add(s);
        else if (fp)
            fputs(s, fp);
    }

    void putchr(FILE *fp, sLstr *lstr, char c)
    {
        if (lstr)
            lstr->add_c(c);
        else if (fp)
            fputc(c, fp);
    }
}


// Print the rule text to fp or lstr.  If cmts, print any associated
// comments, layer is used only when printing comments.  If show_inh,
// print the inhibited status "I " or "  " in the first two columns. 
// If use_orig, use the original string from the tech file if
// available.
//
void
DRCtestDesc::print(FILE *fp, sLstr *lstr, bool cmts, const CDl *ldesc,
    bool show_inh, bool use_orig) const
{
    char *s, buf[1024];
    if (use_orig && td_origstring) {
        sprintf(buf,  "%c %s %s\n", td_inhibit ? 'I' : ' ', ruleName(),
            td_origstring);
        putstr(fp, lstr, show_inh ? buf : buf+2);
        if (cmts && ldesc)
            Tech()->CommentDump(fp, lstr, tBlkPlyr, ldesc->name(), ruleName());
        return;;
    }

    buf[0] = 0;
    if (td_type != drExist) {
        char *rstr = regionString();
        if (rstr) {
            sprintf(buf, "%c %s Region %s", td_inhibit ? 'I' : ' ', ruleName(),
                rstr);
            delete [] rstr;
        }
    }
    if (!buf[0])
        sprintf(buf, "%c %s", td_inhibit ? 'I' : ' ', ruleName());

    switch (td_type) {
    case drNoRule:
        // can't happen
        break;

    case drConnected:
        putstr(fp, lstr, show_inh ? buf : buf+2);
        printComment(fp, lstr);
        putchr(fp, lstr, '\n');
        break;

    case drNoHoles:
        putstr(fp, lstr, show_inh ? buf : buf+2);
        if (area() > 0.0) {
            sprintf(buf, " %s %.4e", Dkw.MinArea(), area());
            putstr(fp, lstr, buf);
        }
        if (hasValue(0)) {
            sprintf(buf, " %s %.4f", Dkw.MinWidth(),
                MICRONS(value(0)));
            putstr(fp, lstr, buf);
        }
        printComment(fp, lstr);
        putchr(fp, lstr, '\n');
        break;

    case drExist:
        putstr(fp, lstr, show_inh ? buf : buf+2);
        printComment(fp, lstr);
        putchr(fp, lstr, '\n');
        break;

    case drOverlap:
    case drIfOverlap:
    case drNoOverlap:
    case drAnyOverlap:
    case drPartOverlap:
    case drAnyNoOverlap:
        putstr(fp, lstr, show_inh ? buf : buf+2);
        putchr(fp, lstr, ' ');
        s = td_target.string();
        putstr(fp, lstr, s);
        delete [] s;
        printComment(fp, lstr);
        putchr(fp, lstr, '\n');
        break;

    case drMinArea:
    case drMaxArea:
        putstr(fp, lstr, show_inh ? buf : buf+2);
        sprintf(buf, " %.4e", area());
        putstr(fp, lstr, buf);
        printComment(fp, lstr);
        putchr(fp, lstr, '\n');
        break;

    case drMinEdgeLength:
        putstr(fp, lstr, show_inh ? buf : buf+2);
        s = td_inside.string(true);
        if (s) {
            putchr(fp, lstr, ' ');
            putstr(fp, lstr, Dkw.Inside());
            putchr(fp, lstr, ' ');
            putstr(fp, lstr, s);
            delete [] s;
        }
        s = td_outside.string(true);
        if (s) {
            putchr(fp, lstr, ' ');
            putstr(fp, lstr, Dkw.Outside());
            putchr(fp, lstr, ' ');
            putstr(fp, lstr, s);
            delete [] s;
        }
        putchr(fp, lstr, ' ');
        s = td_target.string();
        putstr(fp, lstr, s);
        delete [] s;
        sprintf(buf, " %.4f", MICRONS(dimen()));
        putstr(fp, lstr, buf);
        printComment(fp, lstr);
        putchr(fp, lstr, '\n');
        break;

    case drMaxWidth:
        putstr(fp, lstr, show_inh ? buf : buf+2);
        s = td_inside.string(true);
        if (s) {
            putchr(fp, lstr, ' ');
            putstr(fp, lstr, Dkw.Inside());
            putchr(fp, lstr, ' ');
            putstr(fp, lstr, s);
            delete [] s;
        }
        s = td_outside.string(true);
        if (s) {
            putchr(fp, lstr, ' ');
            putstr(fp, lstr, Dkw.Outside());
            putchr(fp, lstr, ' ');
            putstr(fp, lstr, s);
            delete [] s;
        }
        sprintf(buf, " %.4f", MICRONS(dimen()));
        putstr(fp, lstr, buf);
        printComment(fp, lstr);
        putchr(fp, lstr, '\n');
        break;

    case drMinWidth:
        putstr(fp, lstr, show_inh ? buf : buf+2);
        s = td_inside.string(true);
        if (s) {
            putchr(fp, lstr, ' ');
            putstr(fp, lstr, Dkw.Inside());
            putchr(fp, lstr, ' ');
            putstr(fp, lstr, s);
            delete [] s;
        }
        s = td_outside.string(true);
        if (s) {
            putchr(fp, lstr, ' ');
            putstr(fp, lstr, Dkw.Outside());
            putchr(fp, lstr, ' ');
            putstr(fp, lstr, s);
            delete [] s;
        }
        sprintf(buf, " %.4f", MICRONS(dimen()));
        putstr(fp, lstr, buf);
        if (hasValue(0)) {
            putchr(fp, lstr, ' ');
            putstr(fp, lstr, Dkw.Diagonal());
            sprintf(buf, " %.4f", MICRONS(value(0)));
            putstr(fp, lstr, buf);
        }
        printComment(fp, lstr);
        putchr(fp, lstr, '\n');
        break;

    case drMinSpace:
    case drMinSpaceTo:
        putstr(fp, lstr, show_inh ? buf : buf+2);
        s = td_inside.string(true);
        if (s) {
            putchr(fp, lstr, ' ');
            putstr(fp, lstr, Dkw.Inside());
            putchr(fp, lstr, ' ');
            putstr(fp, lstr, s);
            delete [] s;
        }
        s = td_outside.string(true);
        if (s) {
            putchr(fp, lstr, ' ');
            putstr(fp, lstr, Dkw.Outside());
            putchr(fp, lstr, ' ');
            putstr(fp, lstr, s);
            delete [] s;
        }
        if (td_type == drMinSpaceTo) {
            putchr(fp, lstr, ' ');
            s = td_target.string();
            putstr(fp, lstr, s);
            delete [] s;
        }
        if (spaceTab()) {
            putchr(fp, lstr, ' ');
            sTspaceTable::tech_print(spaceTab(), fp, lstr);
            putchr(fp, lstr, ' ');
        }
        else {
            sprintf(buf, " %.4f", MICRONS(dimen()));
            putstr(fp, lstr, buf);
        }
        if (hasValue(0)) {
            putchr(fp, lstr, ' ');
            putstr(fp, lstr, Dkw.Diagonal());
            sprintf(buf, " %.4f", MICRONS(value(0)));
            putstr(fp, lstr, buf);
        }
        if (hasValue(1)) {
            putchr(fp, lstr, ' ');
            putstr(fp, lstr, Dkw.SameNet());
            sprintf(buf, " %.4f", MICRONS(value(1)));
            putstr(fp, lstr, buf);
        }
        printComment(fp, lstr);
        putchr(fp, lstr, '\n');
        break;

    case drMinSpaceFrom:
        putstr(fp, lstr, show_inh ? buf : buf+2);
        s = td_inside.string(true);
        if (s) {
            putchr(fp, lstr, ' ');
            putstr(fp, lstr, Dkw.Inside());
            putchr(fp, lstr, ' ');
            putstr(fp, lstr, s);
            delete [] s;
        }
        s = td_outside.string(true);
        if (s) {
            putchr(fp, lstr, ' ');
            putstr(fp, lstr, Dkw.Outside());
            putchr(fp, lstr, ' ');
            putstr(fp, lstr, s);
            delete [] s;
        }
        putchr(fp, lstr, ' ');
        s = td_target.string();
        putstr(fp, lstr, s);
        delete [] s;
        sprintf(buf, " %.4f", MICRONS(dimen()));
        putstr(fp, lstr, buf);
        if (hasValue(0)) {
            putchr(fp, lstr, ' ');
            putstr(fp, lstr, Dkw.Enclosed());
            sprintf(buf, " %.4f", MICRONS(value(0)));
            putstr(fp, lstr, buf);
        }
        if (hasValue(1) || hasValue(2)) {
            putchr(fp, lstr, ' ');
            putstr(fp, lstr, Dkw.Opposite());
            sprintf(buf, " %.4f %.4f", MICRONS(value(1)), MICRONS(value(2)));
            putstr(fp, lstr, buf);
        }
        printComment(fp, lstr);
        putchr(fp, lstr, '\n');
        break;

    case drMinOverlap:
        putstr(fp, lstr, show_inh ? buf : buf+2);
        s = td_inside.string(true);
        if (s) {
            putchr(fp, lstr, ' ');
            putstr(fp, lstr, Dkw.Inside());
            putchr(fp, lstr, ' ');
            putstr(fp, lstr, s);
            delete [] s;
        }
        s = td_outside.string(true);
        if (s) {
            putchr(fp, lstr, ' ');
            putstr(fp, lstr, Dkw.Outside());
            putchr(fp, lstr, ' ');
            putstr(fp, lstr, s);
            delete [] s;
        }
        putchr(fp, lstr, ' ');
        s = td_target.string();
        putstr(fp, lstr, s);
        delete [] s;
        sprintf(buf, " %.4f", MICRONS(dimen()));
        putstr(fp, lstr, buf);
        printComment(fp, lstr);
        putchr(fp, lstr, '\n');
        break;

    case drMinNoOverlap:
        putstr(fp, lstr, show_inh ? buf : buf+2);
        s = td_inside.string(true);
        if (s) {
            putchr(fp, lstr, ' ');
            putstr(fp, lstr, Dkw.Inside());
            putchr(fp, lstr, ' ');
            putstr(fp, lstr, s);
            delete [] s;
        }
        s = td_outside.string(true);
        if (s) {
            putchr(fp, lstr, ' ');
            putstr(fp, lstr, Dkw.Outside());
            putchr(fp, lstr, ' ');
            putstr(fp, lstr, s);
            delete [] s;
        }
        putchr(fp, lstr, ' ');
        s = td_target.string();
        putstr(fp, lstr, s);
        delete [] s;
        sprintf(buf, " %.4f", MICRONS(dimen()));
        putstr(fp, lstr, buf);
        printComment(fp, lstr);
        putchr(fp, lstr, '\n');
        break;

    case drUserDefinedRule:
        putstr(fp, lstr, show_inh ? buf : buf+2);
        s = td_inside.string(true);
        if (s) {
            putchr(fp, lstr, ' ');
            putstr(fp, lstr, Dkw.Inside());
            putchr(fp, lstr, ' ');
            putstr(fp, lstr, s);
            delete [] s;
        }
        s = td_outside.string(true);
        if (s) {
            putchr(fp, lstr, ' ');
            putstr(fp, lstr, Dkw.Outside());
            putchr(fp, lstr, ' ');
            putstr(fp, lstr, s);
            delete [] s;
        }
        for (int i = 0; i < td_user_rule->argc(); i++) {
            putchr(fp, lstr, ' ');
            putstr(fp, lstr, td_user_rule->argv(i));
        }
        printComment(fp, lstr);
        putchr(fp, lstr, '\n');
        break;
    }
    if (cmts && ldesc)
        Tech()->CommentDump(fp, lstr, tBlkPlyr, ldesc->name(), ruleName());
}


// Print the comment field.  This is a little tricky, since there may
// actually be multiple strings.  In the latter case, the strings must
// be individually double-quoted for separation, and missing strings
// must appear as "".  The saved strings are known to be unquoted.
//
void
DRCtestDesc::printComment(FILE *fp, sLstr *lstr) const
{
    if (td_info[1] || td_info[2] || td_info[3]) {
        putchr(fp, lstr, ' ');
        putchr(fp, lstr, '"');
        if (td_info[0])
            putstr(fp, lstr, td_info[0]);
        putchr(fp, lstr, '"');
        putchr(fp, lstr, ' ');
        putchr(fp, lstr, '"');
        if (td_info[1])
            putstr(fp, lstr, td_info[1]);
        putchr(fp, lstr, '"');
        if (td_info[2] || td_info[3]) {
            putchr(fp, lstr, ' ');
            putchr(fp, lstr, '"');
            if (td_info[2])
                putstr(fp, lstr, td_info[2]);
            putchr(fp, lstr, '"');
        }
        if (td_info[3]) {
            putchr(fp, lstr, ' ');
            putchr(fp, lstr, '"');
            putstr(fp, lstr, td_info[3]);
            putchr(fp, lstr, '"');
        }
    }
    else if (td_info[0]) {
        putchr(fp, lstr, ' ');
        putstr(fp, lstr, td_info[0]);
    }
}


//
// Remaining functions are private.
//

namespace {
    const char *err_region  = "REGION parse failed";
    const char *err_number  = "bad or missing numeric value";
    const char *err_target  = "target layer expression parse failed";
    const char *err_inside  = "INSIDE expression parse failed";
    const char *err_outside = "OUTSIDE expression parse failed";
}

const char *
DRCtestDesc::parseConnected(const char *text)
{
    const char *tbak = text;
    char *tok = lstring::gettok(&text);
    if (lstring::cieq(tok, Dkw.Region())) {
        delete [] tok;
        if (!parseRegion(&text))
            return (err_region);
    }
    else {
        delete [] tok;
        text = tbak;
    }
    parseComment(text);
    return (0);
}


const char *
DRCtestDesc::parseNoHoles(const char *text)
{
    bool got_rg = false;
    bool got_ar = false;
    bool got_wd = false;

    for (;;) {
        const char *tbak = text;
        char *s = lstring::gettok(&text);
        if (!s)
            break;
        if (!got_rg && lstring::cieq(s, Dkw.Region())) {
            delete [] s;
            if (!parseRegion(&text))
                return (err_region);
            got_rg = true;
            continue;
        }
        if (!got_ar && lstring::cieq(s, Dkw.MinArea())) {
            delete [] s;
            s = lstring::gettok(&text);
            double d;
            if (!s || sscanf(s, "%lf", &d) != 1 || d < 0.0) {
                delete [] s;
                return (err_number);
            }
            delete [] s;
            setArea(d);
            got_ar = true;
            continue;
        }
        if (!got_wd && lstring::cieq(s, Dkw.MinWidth())) {
            delete [] s;
            s = lstring::gettok(&text);
            double d;
            if (!s || sscanf(s, "%lf", &d) != 1 || d < 0.0) {
                delete [] s;
                return (err_number);
            }
            delete [] s;
            setValue(0, INTERNAL_UNITS(d));
            got_wd = true;
            continue;
        }
        delete [] s;
        text = tbak;
        break;
    }
    parseComment(text);
    return (0);
}


const char *
DRCtestDesc::parseXoverlap(const char *text)
{
    const char *tbak = text;
    char *tok = lstring::gettok(&text);
    if (lstring::cieq(tok, Dkw.Region())) {
        delete [] tok;
        if (!parseRegion(&text))
            return (err_region);
    }
    else {
        delete [] tok;
        text = tbak;
    }
    if (!parseTarget(&text))
        return (err_target);
    parseComment(text);
    return (0);
}


const char *
DRCtestDesc::parseXarea(const char *text)
{
    const char *tbak = text;
    char *tok = lstring::gettok(&text);
    if (lstring::cieq(tok, Dkw.Region())) {
        delete [] tok;
        if (!parseRegion(&text))
            return (err_region);
    }
    else {
        delete [] tok;
        text = tbak;
    }
    char *s = lstring::gettok(&text);
    double d;
    if (!s || sscanf(s, "%lf", &d) != 1 || d < 0.0) {
        delete [] s;
        return (err_number);
    }
    delete [] s;
    setArea(d);
    parseComment(text);
    return (0);
}


const char *
DRCtestDesc::parseMinEdgeLength(const char *text)
{
    bool got_rg = false;
    bool got_in = false;
    bool got_ou = false;

    for (;;) {
        const char *tbak = text;
        char *s = lstring::gettok(&text);
        if (!s)
            break;
        if (!got_rg && lstring::cieq(s, Dkw.Region())) {
            delete [] s;
            if (!parseRegion(&text))
                return (err_region);
            got_rg = true;
            continue;
        }
        if (!got_in && lstring::cieq(s, Dkw.Inside())) {
            delete [] s;
            if (!parseInside(&text))
                return (err_inside);
            got_in = true;
            continue;
        }
        if (!got_ou && lstring::cieq(s, Dkw.Outside())) {
            delete [] s;
            if (!parseOutside(&text))
                return (err_outside);
            got_ou = true;
            continue;
        }
        delete [] s;
        text = tbak;
        break;
    }

    if (!parseTarget(&text))
        return (err_target);
    char *s = lstring::gettok(&text);
    double d;
    if (!s || sscanf(s, "%lf", &d) != 1 || d < 0.0) {
        delete [] s;
        return (err_number);
    }
    delete [] s;
    setDimen(INTERNAL_UNITS(d));
    parseComment(text);
    return (0);
}


const char *
DRCtestDesc::parseMaxWidth(const char *text)
{
    bool got_rg = false;
    bool got_in = false;
    bool got_ou = false;

    for (;;) {
        const char *tbak = text;
        char *s = lstring::gettok(&text);
        if (!s)
            break;
        if (!got_rg && lstring::cieq(s, Dkw.Region())) {
            delete [] s;
            if (!parseRegion(&text))
                return (err_region);
            got_rg = true;
            continue;
        }
        if (!got_in && lstring::cieq(s, Dkw.Inside())) {
            delete [] s;
            if (!parseInside(&text))
                return (err_inside);
            got_in = true;
            continue;
        }
        if (!got_ou && lstring::cieq(s, Dkw.Outside())) {
            delete [] s;
            if (!parseOutside(&text))
                return (err_outside);
            got_ou = true;
            continue;
        }
        delete [] s;
        text = tbak;
        break;
    }

    char *s = lstring::gettok(&text);
    double d;
    if (!s || sscanf(s, "%lf", &d) != 1 || d < 0.0) {
        delete [] s;
        return (err_number);
    }
    delete [] s;
    setDimen(INTERNAL_UNITS(d));
    parseComment(text);
    return (0);;
}


const char *
DRCtestDesc::parseMinWidth(const char *text)
{
    bool got_rg = false;
    bool got_in = false;
    bool got_ou = false;

    for (;;) {
        const char *tbak = text;
        char *s = lstring::gettok(&text);
        if (!s)
            break;
        if (!got_rg && lstring::cieq(s, Dkw.Region())) {
            delete [] s;
            if (!parseRegion(&text))
                return (err_region);
            got_rg = true;
            continue;
        }
        if (!got_in && lstring::cieq(s, Dkw.Inside())) {
            delete [] s;
            if (!parseInside(&text))
                return (err_inside);
            got_in = true;
            continue;
        }
        if (!got_ou && lstring::cieq(s, Dkw.Outside())) {
            delete [] s;
            if (!parseOutside(&text))
                return (err_outside);
            got_ou = true;
            continue;
        }
        delete [] s;
        text = tbak;
        break;
    }

    char *s = lstring::gettok(&text);
    double d;
    if (!s || sscanf(s, "%lf", &d) != 1 || d < 0.0) {
        delete [] s;
        return (err_number);
    }
    delete [] s;
    setDimen(INTERNAL_UNITS(d));

    // Optional Diagonal clause can follow.

    const char *tbak = text;
    s = lstring::gettok(&text);
    if (!s)
        return (0);

    if (lstring::cieq(s, Dkw.Diagonal())) {
        delete [] s;
        s = lstring::gettok(&text);
        if (!s || sscanf(s, "%lf", &d) != 1 || d < 0.0) {
            delete [] s;
            return (err_number);
        }
        delete [] s;
        setValue(0, INTERNAL_UNITS(d));
    }
    else {
        delete [] s;
        text = tbak;
    }
    parseComment(text);
    return (0);
}


const char *
DRCtestDesc::parseMinSpaceX(const char *text)
{
    bool got_rg = false;
    bool got_in = false;
    bool got_ou = false;

    for (;;) {
        const char *tbak = text;
        char *s = lstring::gettok(&text);
        if (!s)
            break;
        if (!got_rg && lstring::cieq(s, Dkw.Region())) {
            delete [] s;
            if (!parseRegion(&text))
                return (err_region);
            got_rg = true;
            continue;
        }
        if (!got_in && lstring::cieq(s, Dkw.Inside())) {
            delete [] s;
            if (!parseInside(&text))
                return (err_inside);
            got_in = true;
            continue;
        }
        if (!got_ou && lstring::cieq(s, Dkw.Outside())) {
            delete [] s;
            if (!parseOutside(&text))
                return (err_outside);
            got_ou = true;
            continue;
        }
        delete [] s;
        text = tbak;
        break;
    }

    if (td_type == drMinSpaceTo) {
        if (!parseTarget(&text))
            return (err_target);
    }
    char *s = lstring::gettok(&text);
    if (!s)
        return (err_number);

    // The first token is either the numeric dimension or the
    // SpacingTable keyword which is followed by table data. 

    double d;
    if (sscanf(s, "%lf", &d) == 1) {
        delete [] s;
        if (d < 0.0)
            return (err_number);
        setDimen(INTERNAL_UNITS(d));
    }
    else if (lstring::cieq(s, Tkw.SpacingTable())) {
        delete [] s;
        const char *err;
        sTspaceTable *st = sTspaceTable::tech_parse(&text, &err);
        if (!st) {
            if (err)
                return (err);
            return (err_number);
        }
        setSpaceTab(st);
        if (st->dimen > dimen())
            setDimen(st->dimen);
    }
    else {
        delete [] s;
        return (err_number);
    }

    // Optionally next can be Diagonal and SameNet keywords and
    // their numeric values, in either order.  Have to be careful
    // to revert the pointer location if not found as we may be
    // in comment text.

    bool got_dia = false;
    bool got_sn = false;

    for (;;) {
        const char *tbak = text;
        s = lstring::gettok(&text);
        if (!s)
            break;
        if (!got_dia && lstring::cieq(s, Dkw.Diagonal())) {
            delete [] s;
            s = lstring::gettok(&text);
            if (!s || sscanf(s, "%lf", &d) != 1 || d < 0.0) {
                delete [] s;
                return (err_number);
            }
            delete [] s;
            setValue(0, INTERNAL_UNITS(d));
            got_dia = true;
            continue;
        }
        if (!got_sn && lstring::cieq(s, Dkw.SameNet())) {
            delete [] s;
            s = lstring::gettok(&text);
            if (!s || sscanf(s, "%lf", &d) != 1 || d < 0.0) {
                delete [] s;
                return (err_number);
            }
            delete [] s;
            setValue(1, INTERNAL_UNITS(d));
            got_sn = true;
            continue;
        }
        delete [] s;
        text = tbak;
        break;
    }
    parseComment(text);
    return (0);
}


const char *
DRCtestDesc::parseMinSpaceFrom(const char *text)
{
    bool got_rg = false;
    bool got_in = false;
    bool got_ou = false;

    for (;;) {
        const char *tbak = text;
        char *s = lstring::gettok(&text);
        if (!s)
            break;
        if (!got_rg && lstring::cieq(s, Dkw.Region())) {
            delete [] s;
            if (!parseRegion(&text))
                return (err_region);
            got_rg = true;
            continue;
        }
        if (!got_in && lstring::cieq(s, Dkw.Inside())) {
            delete [] s;
            if (!parseInside(&text))
                return (err_inside);
            got_in = true;
            continue;
        }
        if (!got_ou && lstring::cieq(s, Dkw.Outside())) {
            delete [] s;
            if (!parseOutside(&text))
                return (err_outside);
            got_ou = true;
            continue;
        }
        delete [] s;
        text = tbak;
        break;
    }

    if (!parseTarget(&text))
        return (err_target);

    char *s = lstring::gettok(&text);
    double d;
    if (!s || sscanf(s, "%lf", &d) != 1 || d < 0.0) {
        delete [] s;
        return (err_number);
    }
    delete [] s;
    setDimen(INTERNAL_UNITS(d));

    // Optionally Enclosed and Opposite clauses can follow,
    // in either order.

    bool got_enc = false;
    bool got_opp = false;

    for (;;) {
        const char *tbak = text;
        s = lstring::gettok(&text);
        if (!s)
            break;
        if (!got_enc && lstring::cieq(s, Dkw.Enclosed())) {
            delete [] s;
            s = lstring::gettok(&text);
            if (!s || sscanf(s, "%lf", &d) != 1 || d < 0.0) {
                delete [] s;
                return (err_number);
            }
            delete [] s;
            setValue(0, INTERNAL_UNITS(d));
            got_enc = true;
            continue;
        }
        if (!got_opp && lstring::cieq(s, Dkw.Opposite())) {
            delete [] s;
            s = lstring::gettok(&text);
            double d1;
            if (!s || sscanf(s, "%lf", &d1) != 1 || d1 < 0.0) {
                delete [] s;
                return (err_number);
            }
            delete [] s;
            s = lstring::gettok(&text);
            double d2;
            if (!s || sscanf(s, "%lf", &d2) != 1 || d2 < 0.0) {
                delete [] s;
                return (err_number);
            }
            delete [] s;
            setValue(1, INTERNAL_UNITS(d1));
            setValue(2, INTERNAL_UNITS(d2));
            got_opp = true;
            continue;
        }
        delete [] s;
        text = tbak;
        break;
    }
    parseComment(text);
    return (0);
}


const char *
DRCtestDesc::parseMinXoverlap(const char *text)
{
    bool got_rg = false;
    bool got_in = false;
    bool got_ou = false;

    for (;;) {
        const char *tbak = text;
        char *s = lstring::gettok(&text);
        if (!s)
            break;
        if (!got_rg && lstring::cieq(s, Dkw.Region())) {
            delete [] s;
            if (!parseRegion(&text))
                return (err_region);
            got_rg = true;
            continue;
        }
        if (!got_in && lstring::cieq(s, Dkw.Inside())) {
            delete [] s;
            if (!parseInside(&text))
                return (err_inside);
            got_in = true;
            continue;
        }
        if (!got_ou && lstring::cieq(s, Dkw.Outside())) {
            delete [] s;
            if (!parseOutside(&text))
                return (err_outside);
            got_ou = true;
            continue;
        }
        delete [] s;
        text = tbak;
        break;
    }

    if (!parseTarget(&text))
        return (err_target);
    char *s = lstring::gettok(&text);
    double d;
    if (!s || sscanf(s, "%lf", &d) != 1 || d < 0.0) {
        delete [] s;
        return (err_number);
    }
    delete [] s;
    setDimen(INTERNAL_UNITS(d));
    parseComment(text);
    return (0);
}


const char *
DRCtestDesc::parseUserDefinedRule(const char *text, DRCtest *tst)
{
    if (!tst)
        return ("null test descriptor");

    bool got_rg = false;
    bool got_in = false;
    bool got_ou = false;

    for (;;) {
        const char *tbak = text;
        char *s = lstring::gettok(&text);
        if (!s)
            break;
        if (!got_rg && lstring::cieq(s, Dkw.Region())) {
            delete [] s;
            if (!parseRegion(&text))
                return (err_region);
            got_rg = true;
            continue;
        }
        if (!got_in && lstring::cieq(s, Dkw.Inside())) {
            delete [] s;
            if (!parseInside(&text))
                return (err_inside);
            got_in = true;
            continue;
        }
        if (!got_ou && lstring::cieq(s, Dkw.Outside())) {
            delete [] s;
            if (!parseOutside(&text))
                return (err_outside);
            got_ou = true;
            continue;
        }
        delete [] s;
        text = tbak;
        break;
    }

    int ac = 0;
    char **av, *tok;
    av = new char*[tst->argc() + 1];
    av[tst->argc()] = 0;
    while ((tok = lstring::gettok(&text)) != 0) {
        av[ac++] = tok;
        if (ac == tst->argc())
            break;
    }
    if (ac != tst->argc()) {
        for (int i = 0; i < ac; i++)
            delete [] av[i];
        delete [] av;
        return ("too few arguments");
    }
    DRCtest *ntst = new DRCtest(lstring::copy(tst->name()));
    ntst->setArgs(ac, av);
    if (!ntst->substitute(tst)) {
        delete ntst;
        return ("argument substitution failed");
    }
    td_user_rule = ntst;
    parseComment(text);
    return (0);
}


// Parse the "Region" specification, if any, in string.
//
bool
DRCtestDesc::parseRegion(const char **string_ptr)
{
    char buf[256];
    const char *str = *string_ptr;
    if (!td_source.parseExpr(&str, true)) {
        sLstr lstr;
        lstr.add("Region layer expression parse failed on line ");
        lstr.add_i(Tech()->LineCount());
        lstr.add(":\n");
        lstr.add(Errs()->get_error());
        Log()->ErrorLog(mh::DRC, lstr.string());
        return (false);
    }

    // We need to AND the region with the objlayer.
    if (td_source.tree()) {
        strcpy(buf, td_objlayer->name());
        const char *t = buf;
        ParseNode *pleft = SIparse()->getLexprTree(&t);
        // Above should never fail, but be safe.
        if (!pleft) {
            char *er = SIparse()->errMessage();
            if (er) {
                sLstr lstr;
                lstr.add("Internal: Region parse failed on line ");
                lstr.add_i(Tech()->LineCount());
                lstr.add(":\n");
                lstr.add(er);
                Log()->ErrorLog(mh::DRC, lstr.string());
                delete [] er;
            }
            return (false);
        }
        ParseNode *p =
            SIparse()->MakeBopNode(TOK_AND, pleft, td_source.tree());
        td_source.set_tree(p);
    }
    else {
        char *s = td_source.string(true);
        if (s) {
            if (strcmp(s, td_objlayer->name())) {
                // If region name same as source layer name, not a
                // "real" region!
                sprintf(buf, "(%s)&(%s)", td_objlayer->name(), s);
                const char *t = buf;
                ParseNode *p = SIparse()->getLexprTree(&t);
                if (!p) {
                    char *er = SIparse()->errMessage();
                    if (er) {
                        sLstr lstr;
                        lstr.add("Region parse failed on line ");
                        lstr.add_i(Tech()->LineCount());
                        lstr.add(":\n");
                        lstr.add(er);
                        Log()->ErrorLog(mh::DRC, lstr.string());
                        delete [] er;
                    }
                    return (false);
                }
                td_source.set_tree(p);
            }
            td_source.set_lname(0);
            td_source.set_ldesc(0);
            delete [] s;
        }
    }
    *string_ptr = str;
    return (true);
}


// Parse the Inside layer or expression.
//
bool
DRCtestDesc::parseInside(const char **string_ptr)
{
    if (!td_inside.parseExpr(string_ptr, true)) {
        sLstr lstr;
        lstr.add("Inside layer expression parse failed on line ");
        lstr.add_i(Tech()->LineCount());
        lstr.add(":\n");
        lstr.add(Errs()->get_error());
        Log()->ErrorLog(mh::DRC, lstr.string());
        return (false);
    }
    return (true);
}


// Parse the Outside layer or expression.
//
bool
DRCtestDesc::parseOutside(const char **string_ptr)
{
    if (!td_outside.parseExpr(string_ptr, true)) {
        sLstr lstr;
        lstr.add("Outside layer expression parse failed on line ");
        lstr.add_i(Tech()->LineCount());
        lstr.add(":\n");
        lstr.add(Errs()->get_error());
        Log()->ErrorLog(mh::DRC, lstr.string());
        return (false);
    }
    return (true);
}


// Parse the target layer or expression.
//
bool
DRCtestDesc::parseTarget(const char **string_ptr)
{
    if (!td_target.parseExpr(string_ptr, true)) {
        sLstr lstr;
        lstr.add("Target layer expression parse failed on line ");
        lstr.add_i(Tech()->LineCount());
        lstr.add(":\n");
        lstr.add(Errs()->get_error());
        Log()->ErrorLog(mh::DRC, lstr.string());
        return (false);
    }
    return (true);
}


// Parse the comment text.  This can be in the form of separate
// double-quoted strings, in which case these set the info[] fields
// progressively.  If the leading character is not a double quote, the
// remainder of the string is assumed.
//
void
DRCtestDesc::parseComment(const char *text)
{
    if (!text)
        return;
    while (isspace(*text))
        text++;
    if (!*text)
        return;
    if (*text == '"') {
        char *str = lstring::getqtok(&text);
        if (str && *str)
            setInfo(0, str);
        delete [] str;
        if (*text == '"') {
            str = lstring::getqtok(&text);
            if (str && *str)
                setInfo(1, str);
            delete [] str;
            if (*text == '"') {
                str = lstring::getqtok(&text);
                if (str && *str)
                    setInfo(2, str);
                delete [] str;
                if (*text == '"') {
                    str = lstring::getqtok(&text);
                    if (str && *str)
                        setInfo(3, str);
                    delete [] str;
                }
                else if (*text)
                    setInfo(3, text);
            }
            else if (*text)
                setInfo(2, text);
        }
        else if (*text)
            setInfo(1, text);
    }
    else
        setInfo(0, text);
}

//XXX Diva syntax
/////////////////////////////////////////////////////////////////////
#ifdef notdef

// [DRC] [(]layer1 [layer2] function [modifiers] ["string"][)]
//
// function:  lower_limit operator func_kw operator upper_limit
//            function_kw operator upper_limit
//            function_kw operator lower_limit
//
// The lower and upper dimensional limits can be integers, floating point
// numbers, expressions, or references to layer properties defined in the
// technology file.  The upper limit is required for all functions except
// area; the lower limit is optional.  Both limits are defined in user
// units (for example, microns).
//
// One can use the following function keywords:
//
//   width
//   notch
//   area
//   sep
//   enc
//   ovlp
//
// The following operators are available:
//
//   <
//   <=
//   >
//   >=
//   ==
//



void
cDRC::parseDiva(const char *string)
{
    if (!string)
        return;

    // Strip leading white space, "DRC" if found, and opening paren
    // if found.
    while (isspace(*string))
        string++;
    if (lstring::ciprefix("drc", string) && (isspace(string[3) ||
            string[3] == '(' || strting[3] == 0))
        string += 3;
    while (isspace(*string))
        string++;
    if (*string == '(')
        string++;
    while (isspace(*string))
        string++;
    if (!*string)
        return;

    // Find the source layer.
    char *tok = lstring::gettok(&string);
    CDl *ld = CDldb()->findLayer(tok, Physical);
    delete [] tok;
    if (!ld) {
        //error
    }

    // Check for a target layer.
    const char *tbak = string;
    tok = lstring::gettok(&string);
    CDl *ld2 = CDldb()->findLayer(tok, Physical);
    delete [] tok;
    if (!ld2)
        string = tbak;


}

#endif


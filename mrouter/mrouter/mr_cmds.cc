
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
 *                                                                        *
 * LEF/DEF Database and Maze Router.                                      *
 *                                                                        *
 * Stephen R. Whiteley (stevew@wrcad.com)                                 *
 * Whiteley Research Inc. (wrcad.com)                                     *
 *                                                                        *
 * Portions adapted from Qrouter by Tim Edwards,                          *
 * (www.opencircuitdesign.com) which used code by Steve Beccue.           *
 * See original headers where applicable.                                 *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#include "mrouter_prv.h"
#include "miscutil/lstring.h"
#include <stdarg.h>
#include <unistd.h>
#include <algorithm>


//
// Maze Router.
//
// This is a high-level control interface, used to implement, e.g.,
// command line handling.  The functions return true on success, with
// possibly a message in cLDDB::db_donemsg and/or cLDDB::db_warnmsg. 
// False is returned on error, with a message in cLDDB::db_errmsg.
//


// A script file consists of one or more commands, one per line.  This
// will replace the config file capability imported from Qrouter. 
//
bool
cMRouter::readScript(const char *fname)
{
    if (!fname || !*fname) {
        db->emitErrMesg("ERROR: null or empty file name.\n");
        return (LD_BAD);
    }

    FILE *fp = fopen(fname, "r");
    if (!fp) {
        db->emitErrMesg("ERROR: failed to open %s,\n", fname);
        return (LD_BAD);
    }
    bool ret = readScript(fp);

    fclose(fp);
    return (ret);
}


bool
cMRouter::readScript(FILE *fp)
{
    if (!fp)
        return (LD_BAD);
    bool istty = isatty(fileno(fp)) && isatty(fileno(stdout));
    char buf[256], *line;
    bool ret = LD_OK;
    if (istty) {
        fprintf(stdout, "? ");
        fflush(stdout);
    }
    while ((line = fgets(buf, 256, fp)) != 0) {
        while (isspace(*line))
            line++;
        char *e = line + strlen(line) - 1;
        while (e >= line && isspace(*e))
            *e-- = 0;

        // All commands start with an alpha character.  Lines that
        // don't are taken as comments and ignored.
        if (!isalpha(*line))
            continue;

        if (!strcasecmp(line, "exit") || !strcasecmp(line, "quit"))
            break;

        ret = doCmd(line);
        if (errMsg())
            db->emitErrMesg("ERROR: %s\n%s\n", line, db->errMsg());
        else {
            if (warnMsg())
                db->emitMesg("WARNING: %s\n%s\n", line, db->warnMsg());
            if (doneMsg())
                db->emitMesg("%s\n", db->doneMsg());
        }
        if (ret == LD_BAD)
            break;
        if (istty) {
            fprintf(stdout, "? ");
            fflush(stdout);
        }
    }
    clearMsgs();
    return (ret);
}


// Process a single line command.  This can be used for interactive
// command line processing, or in script processing.  This applies to
// router and database commands.  If a command is unknown here, it
// will be passed on to the LDDB command handler.  Note that we
// overload a few of the LDDB commands.
//
bool
cMRouter::doCmd(const char *cmd)
{
    clearMsgs();
    const char *s = cmd;
    char *tok = lstring::gettok(&s);
    if (!tok)
        return (LD_OK);

    bool ret = LD_BAD;
    bool unhandled = false;
    if (!strcmp(tok,        "reset")) {       // override
        delete [] tok;
        tok = lstring::gettok(&s);
        ret = reset( (tok && strchr("tTyY1aA", *tok)) );
    }
    else if (!strcmp(tok,   "set"))           // override
        ret = cmdSet(s);
    else if (!strcmp(tok,   "setcost"))
        ret = cmdSetcost(s);
    else if (!strcmp(tok,   "unset"))         // override
        ret = cmdUnset(s);
    else if (!strcmp(tok,   "read")) {
        delete [] tok;
        tok = lstring::gettok(&s);
        if (!tok) {
            setErrMsg(lstring::copy("Missing directive to read operation."));
            return (LD_BAD);
        }
        if (!strcmp(tok,        "script")) {  // override
            delete [] tok;
            tok = lstring::getqtok(&s);
            ret = readScript(tok);
        }
        if (!strcmp(tok,        "config")) {  // override
            delete [] tok;
            tok = lstring::getqtok(&s);
            ret = cmdReadCfg(tok);
        }
        else
            unhandled = true;
    }
    else if (!strcmp(tok,   "write")) {
        delete [] tok;
        tok = lstring::gettok(&s);
        if (!tok) {
            setErrMsg(lstring::copy("Missing directive to write operation."));
            return (LD_BAD);
        }
        if (!strcmp(tok,        "def")) {     // override
            delete [] tok;
            tok = lstring::getqtok(&s);
            setupRoutePaths();
            clearMsgs();
            ret = db->defWrite(tok);
            if (ret == LD_BAD)
                setErrMsg(lstring::copy("Write DEF failed."));
        }
        else
            unhandled = true;
    }
    else if (!strcmp(tok,   "append")) {      // override
        delete [] tok;
        tok = lstring::getqtok(&s);
        char *tok2 = lstring::getqtok(&s);
        setupRoutePaths();
        clearMsgs();
        ret = db->writeDefRoutes(tok, tok2);
        delete [] tok2;
        if (ret == LD_BAD)
            setErrMsg(lstring::copy("Update DEF failed."));
    }

    else if (!strcmp(tok,   "stage1"))
        ret = cmdStage1(s);
    else if (!strcmp(tok,   "stage2"))
        ret = cmdStage2(s);
    else if (!strcmp(tok,   "stage3"))
        ret = cmdStage3(s);
    else if (!strcmp(tok,   "ripup"))
        ret = cmdRipUp(s);
    else if (!strcmp(tok,   "failed"))
        ret = cmdFailed(s);
    else if (!strncmp(tok,   "congest", 7))
        ret = cmdCongested(s);
    else
        unhandled = true;
    delete [] tok;

    if (unhandled)
        ret = db->doCmd(cmd);
    return (ret);
}


namespace {
    char *write_msg(const char *fmt, ...)
    {
        char buf[256];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buf, 256, fmt, args);
        return (lstring::copy(buf));
    }
}


bool
cMRouter::reset(bool all)
{
    clearMsgs();
    resetRouter();
    if (all)
        db->reset();
    return (LD_OK);
}


bool
cMRouter::cmdSet(const char *cmd)
{
    clearMsgs();
    char buf[80];
    const char *s = cmd;
    char *tok = lstring::gettok(&s);
    if (!tok) {
        sLstr lstr;
        const char *fmt = "%-16s: ";

        db->cmdSet(0);
        lstr.add(doneMsg());
        delete [] doneMsg();
        setDoneMsg(0);

        sprintf(buf, fmt, "netorder");
        lstr.add(buf);
        sprintf(buf, "%u\n", netOrder());
        lstr.add(buf);

        sprintf(buf, fmt, "passes");
        lstr.add(buf);
        sprintf(buf, "%d\n", numPasses());
        lstr.add(buf);

        sprintf(buf, fmt, "increments");
        lstr.add(buf);
        if (!mr_rmaskIncs)
            lstr.add("1\n");
        else {
            for (u_int i = 0; i < mr_rmaskIncsSz; i++) {
                sprintf(buf, " %d", mr_rmaskIncs[i]);
                lstr.add(buf);
            }
            lstr.add_c('\n');
        }

        sprintf(buf, fmt, "via_stack");
        lstr.add(buf);
        if (stackedVias() < 0 ||
                (stackedVias() > 0 && stackedVias() >= (int)numLayers()))
            sprintf(buf, "all\n");
        else if (stackedVias() == 0 || stackedVias() == 1)
            sprintf(buf, "none\n");
        else
            sprintf(buf, "%d\n", stackedVias());
        lstr.add(buf);

        sprintf(buf, fmt, "via_pattern");
        lstr.add(buf);
        lstr.add(viaPattern() == VIA_PATTERN_NORMAL ? "normal" : "inverted");
        lstr.add_c('\n');

        setDoneMsg(lstr.string_trim());
        return (LD_OK);
    }
    if (!strcasecmp(tok, "netorder")) {
        // Set the net ordering algorithm to be used.

        delete [] tok;
        tok = lstring::gettok(&s);
        if (!tok) {
            sprintf(buf, "netorder: %u", netOrder());
            setDoneMsg(lstring::copy(buf));
        }
        else {
            if (isdigit(*tok)) {
                u_int i = atoi(tok);
                switch (i) {
                case 0:
                    setNetOrder(mrNetDefault);
                    break;
                case 1:
                    setNetOrder(mrNetAlt1);
                    break;
                case 2:
                    setNetOrder(mrNetNoSort);
                    break;
                default:
                    setErrMsg(write_msg(
                        "bad value %s, expecting integer 0-2.", tok));
                    delete [] tok;
                    return (LD_BAD);
                }
            }
            else {
                setErrMsg(write_msg(
                    "bad value %s, expecting positive integer.", tok));
                delete [] tok;
                return (LD_BAD);
            }
            delete [] tok;
        }
        return (LD_OK);
    }
    if (!strcasecmp(tok, "passes")) {
        // Set the maximum number of attempted passes of the route
        // search algorithm, where the routing constraints (maximum
        // cost and route area mask) are relaxed on each pass.  With
        // no argument, return the value for the maximum number of
        // passes.  The default number of passes is 10 and normally
        // there is no reason for the user to change it.

        delete [] tok;
        tok = lstring::gettok(&s);
        if (!tok) {
            sprintf(buf, "passes: %d", numPasses());
            setDoneMsg(lstring::copy(buf));
        }
        else {
            if (isdigit(*tok)) {
                u_int i = atoi(tok);
                if (1 > MR_MAX_PASSES) {
                    setErrMsg(write_msg("too many passes %u, limit %u.",
                        i, MR_MAX_PASSES));
                    delete [] tok;
                    return (LD_BAD);
                }
                if (i == 0) {
                    setErrMsg(write_msg(
                        "bad value %s, expecting positive integer.", tok));
                    delete [] tok;
                    return (LD_BAD);
                }
                setNumPasses(i);
            }
            else {
                setErrMsg(write_msg(
                    "bad value %s, expecting positive integer.", tok));
                delete [] tok;
                return (LD_BAD);
            }
            delete [] tok;
        }
        return (LD_OK);
    }
    if (!strcasecmp(tok, "increments")) {
        // This can be set to a sequence of small positive integers,
        // each giving the halo width for each pass level, in
        // channels.  Pass numbers larger than the list lingth take
        // the final (rightmost) value.

        delete [] tok;
        tok = lstring::gettok(&s);
        if (!tok) {
            sLstr lstr;
            lstr.add("increments: ");
            if (!rmaskIncs())
                lstr.add("1");
            else {
                for (u_int i = 0; i < rmaskIncsSz(); i++) {
                    sprintf(buf, " %d", rmaskIncs()[i]);
                    lstr.add(buf);
                }
            }
            setDoneMsg(lstr.string_trim());
        }
        else {
            const char *bak = s;
            int ntoks = 1;
            char *t;
            while ((t = lstring::gettok(&s)) != 0) {
                delete [] t;
                ntoks++;
            }
            u_char *vals = new u_char[ntoks];

            s = bak;
            ntoks = 0;
            while (tok) {
                if (!isdigit(*tok)) {
                    setErrMsg(write_msg(
                        "bad increment value %s, expecting positive integer.",
                        tok));
                    delete [] tok;
                    delete [] vals;
                    return (LD_BAD);
                }
                u_int i = atoi(tok);
                if (i > 255) {
                    setErrMsg(write_msg("bad increment value %u, limit %u.",
                        i, 255));
                    delete [] tok;
                    delete [] vals;
                    return (LD_BAD);
                }
                if (i == 0) {
                    setErrMsg(write_msg(
                        "bad increment value %s, expecting positive integer.",
                        tok));
                    delete [] tok;
                    delete [] vals;
                    return (LD_BAD);
                }
                vals[ntoks++] = i;
                delete [] tok;
                tok = lstring::gettok(&s);
            }
            setRmaskIncs(vals, ntoks);
        }
        return (LD_OK);
    }
    if (!strcasecmp(tok, "via_stack")) {
        // Value is the maximum number of vias that may be stacked
        // directly on top of each other at a single point.  Value
        // "none", "0", and "1" all mean the same thing.

        delete [] tok;
        tok = lstring::gettok(&s);
        if (!tok) {
            if (stackedVias() < 0 ||
                    (stackedVias() > 0 && stackedVias() >= (int)numLayers()))
                sprintf(buf, "via_stack: all");
            else if (stackedVias() == 0 || stackedVias() == 1)
                sprintf(buf, "via_stack: none");
            else
                sprintf(buf, "via_stack: %d", stackedVias());
            setDoneMsg(lstring::copy(buf));
        }
        else {
            int i;
            if (sscanf(tok, "%d", &i) == 1) {
                if (i < 0)
                    setStackedVias(-1);
                else if (i == 0)
                    setStackedVias(1);
                else
                    setStackedVias(i);
            }
            else if (*tok == 'n' || *tok == 'N')
                setStackedVias(1);
            else if (*tok == 'a' || *tok == 'A')
                setStackedVias(-1);
        }
        return (LD_OK);
    }
    if (!strcasecmp(tok, "via_pattern")) {
        // If vias are non-square, then they are placed in a
        // checkerboard pattern, with every other via rotated 90
        // degrees.  If inverted, the rotation is swapped relative to
        // the grid positions used in the non-inverted case.
        // Values: 0 or n/N[...], nonzero or i/I[...].

        delete [] tok;
        tok = lstring::gettok(&s);
        if (!tok) {
            sprintf(buf, "via_stack: %s",
                viaPattern() == VIA_PATTERN_NORMAL ? "normal" : "inverted");
            setDoneMsg(lstring::copy(buf));
        }
        else {
            if (isdigit(*tok)) {
                u_int i = atoi(tok);
                setViaPattern(i ? VIA_PATTERN_INVERT : VIA_PATTERN_NORMAL);
            }
            else if (*tok == 'n' || *tok == 'N')
                setViaPattern(VIA_PATTERN_NORMAL);
            else if (*tok == 'i' || *tok == 'I')
                setViaPattern(VIA_PATTERN_INVERT);
        }
        return (LD_OK);
    }
    delete [] tok;
    return (db->cmdSet(cmd));
}


// Similar to set, but applies only to the routing cost values.
//
bool
cMRouter::cmdSetcost(const char *cmd)
{
    clearMsgs();
    char buf[80];
    char *tok = lstring::gettok(&cmd);
    if (!tok) {
        sLstr lstr;
        const char *fmt = "%-16s: ";

        sprintf(buf, fmt, "segcost");
        lstr.add(buf);
        sprintf(buf, "%d\n", segCost());
        lstr.add(buf);

        sprintf(buf, fmt, "viacost");
        lstr.add(buf);
        sprintf(buf, "%d\n", viaCost());
        lstr.add(buf);

        sprintf(buf, fmt, "jogcost");
        lstr.add(buf);
        sprintf(buf, "%d\n", jogCost());
        lstr.add(buf);

        sprintf(buf, fmt, "crossovercost");
        lstr.add(buf);
        sprintf(buf, "%d\n", xverCost());
        lstr.add(buf);

        sprintf(buf, fmt, "blockcost");
        lstr.add(buf);
        sprintf(buf, "%d\n", blockCost());
        lstr.add(buf);

        sprintf(buf, fmt, "offsetcost");
        lstr.add(buf);
        sprintf(buf, "%d\n", offsetCost());
        lstr.add(buf);

        sprintf(buf, fmt, "conflictcost");
        lstr.add(buf);
        sprintf(buf, "%d\n", conflictCost());
        lstr.add(buf);

        db->setDoneMsg(lstr.string_trim());
        return (LD_OK);
    }
    char c = *tok;
    if (isupper(c))
        c = tolower(c);
    if (c == 's') {
        // segment cost
        delete [] tok;
        tok = lstring::gettok(&cmd);
        if (!tok) {
            sprintf(buf, "segment cost: %d", segCost());
            db->setDoneMsg(lstring::copy(buf));
        }
        else {
            if (isdigit(*tok)) {
                u_int i = atoi(tok);
                setSegCost(i);
            }
            delete [] tok;
        }
        return (LD_OK);
    }
    if (c == 'v') {
        // via cost
        delete [] tok;
        tok = lstring::gettok(&cmd);
        if (!tok) {
            sprintf(buf, "via cost: %d", viaCost());
            db->setDoneMsg(lstring::copy(buf));
        }
        else {
            if (isdigit(*tok)) {
                u_int i = atoi(tok);
                setViaCost(i);
            }
            delete [] tok;
        }
        return (LD_OK);
    }
    if (c == 'j') {
        // jog cost
        delete [] tok;
        tok = lstring::gettok(&cmd);
        if (!tok) {
            sprintf(buf, "jog cost: %d", jogCost());
            db->setDoneMsg(lstring::copy(buf));
        }
        else {
            if (isdigit(*tok)) {
                u_int i = atoi(tok);
                setJogCost(i);
            }
            delete [] tok;
        }
        return (LD_OK);
    }
    if (c == 'c') {
        if (tok[1] == 'r' || tok[1] == 'R')
            c = 'x';
    }
    if (c == 'x') {
        // crossover cost
        delete [] tok;
        tok = lstring::gettok(&cmd);
        if (!tok) {
            sprintf(buf, "crossover cost: %d", xverCost());
            db->setDoneMsg(lstring::copy(buf));
        }
        else {
            if (isdigit(*tok)) {
                u_int i = atoi(tok);
                setXverCost(i);
            }
            delete [] tok;
        }
        return (LD_OK);
    }
    if (c == 'b') {
        // block cost
        delete [] tok;
        tok = lstring::gettok(&cmd);
        if (!tok) {
            sprintf(buf, "block cost: %d", blockCost());
            db->setDoneMsg(lstring::copy(buf));
        }
        else {
            if (isdigit(*tok)) {
                u_int i = atoi(tok);
                setBlockCost(i);
            }
            delete [] tok;
        }
        return (LD_OK);
    }
    if (c == 'o') {
        // offset cost
        delete [] tok;
        tok = lstring::gettok(&cmd);
        if (!tok) {
            sprintf(buf, "offset cost: %d", offsetCost());
            db->setDoneMsg(lstring::copy(buf));
        }
        else {
            if (isdigit(*tok)) {
                u_int i = atoi(tok);
                setOffsetCost(i);
            }
            delete [] tok;
        }
        return (LD_OK);
    }
    if (c == 'c') {
        // conflict cost
        delete [] tok;
        tok = lstring::gettok(&cmd);
        if (!tok) {
            sprintf(buf, "conflict cost: %d", conflictCost());
            db->setDoneMsg(lstring::copy(buf));
        }
        else {
            if (isdigit(*tok)) {
                u_int i = atoi(tok);
                setConflictCost(i);
            }
            delete [] tok;
        }
        return (LD_OK);
    }

    db->setErrMsg(write_msg("Unknown keyword %s.", tok));
    delete [] tok;
    return (LD_BAD);
}


// Apply default values for (i.e., unset) each of the keywords given.
//
bool
cMRouter::cmdUnset(const char *cmd)
{
    clearMsgs();
    char *tok;
    while ((tok = lstring::gettok(&cmd)) != 0) {
        if (!strcasecmp(tok, "netorder"))
            setNetOrder(mrNetDefault);
        else if (!strcasecmp(tok, "passes"))
            setNumPasses(MR_PASSES);
        else if (!strcasecmp(tok, "increments")) {
            u_char *vals = new u_char[1];
            vals[0] = 1;
            setRmaskIncs(vals, 1);
        }
        else if (!strcasecmp(tok, "via_stack"))
            setStackedVias(-1);
        else if (!strcasecmp(tok, "via_pattern"))
            setViaPattern(VIA_PATTERN_NONE);

        else if (!strcasecmp(tok, "segcost"))
            setSegCost(MR_SEGCOST);
        else if (!strcasecmp(tok, "viacost"))
            setSegCost(MR_VIACOST);
        else if (!strcasecmp(tok, "jogcost"))
            setSegCost(MR_JOGCOST);
        else if (!strcasecmp(tok, "crossovercost") ||
                !strcasecmp(tok, "xverrcost"))
            setSegCost(MR_XVERCOST);
        else if (!strcasecmp(tok, "blockcost"))
            setSegCost(MR_BLOCKCOST);
        else if (!strcasecmp(tok, "offsetcost"))
            setSegCost(MR_OFFSETCOST);
        else if (!strcasecmp(tok, "conflictcost"))
            setSegCost(MR_CONFLICTCOST);

        else {
            if (db->cmdSet(tok) != LD_OK) {
                delete [] tok;
                return (LD_BAD);
            }
        }
        delete [] tok;
    }
    return (LD_OK);
}


bool
cMRouter::cmdReadCfg(const char *fname)
{
    clearMsgs();
    if (readConfig(fname, false, false) == LD_OK)
        return (LD_OK);
    setErrMsg(lstring::copy("Read config failed."));
    return (LD_BAD);

}


// Execute stage1 routing.  This works through the entire netlist,
// routing as much as possible but not doing any rip-up and re-route. 
// Nets that fail to route are put in the "FailedNets" list.
//
// The interpreter result is set to the number of failed routes at the
// end of the first stage.
//
// Options:
//
//  -d          Draw the area being searched in real-time.  This slows
//               down the algorithm and is intended only for diagnostic use.
//  -s          Single-step stage one.
//  -m[ ]n[one] Don't limit the search area.
//  -m[ ]a[uto] Select the mask automatically.
//  -m[ ]b[box] Use the net bbox as a mask
//  -m[ ]<value>  Set the mask size to <value>, an integer typ. 0 and up.
//  -f          Force a terminal to be routable
//  <net>       Route net named <net> only.
//
bool
cMRouter::cmdStage1(const char *cmd)
{
    clearMsgs();
    int maskV = MASK_NONE;
    bool dodebug = false;
    bool forceRt = false;
    bool dostep = false;
    stringlist *routeNames = 0;

    char *tok;
    while ((tok = lstring::gettok(&cmd)) != 0) {
        if (*tok == '-') {
            if (tok[1] == 'd') {
                dodebug = true;
                delete [] tok;
                continue;
            }
            if (tok[1] == 'f') {
                forceRt = true;
                delete [] tok;
                continue;
            }
            if (tok[1] == 's') {
                dostep = true;
                delete [] tok;
                continue;
            }
            if (tok[1] == 'm') {
                if (tok[2] == 'a') {
                    maskV = MASK_AUTO;
                    delete [] tok;
                    continue;
                }
                if (tok[2] == 'b') {
                    maskV = MASK_BBOX;
                    delete [] tok;
                    continue;
                }
                if (tok[2] == 'n') {
                    maskV = MASK_NONE;
                    delete [] tok;
                    continue;
                }
                if (isdigit(tok[2])) {
                    maskV = atoi(tok+2);
                    delete [] tok;
                    continue;
                }
                if (!tok[2]) {
                    delete tok;
                    tok = lstring::gettok(&cmd);
                    if (!tok) {
                        db->setErrMsg(
                            lstring::copy("stage1: missing -m value."));
                        delete [] tok;
                        stringlist::destroy(routeNames);
                        return (LD_BAD);
                    }
                    if (*tok == 'a') {
                        maskV = MASK_AUTO;
                        delete [] tok;
                        continue;
                    }
                    if (*tok == 'b') {
                        maskV = MASK_BBOX;
                        delete [] tok;
                        continue;
                    }
                    if (*tok == 'n') {
                        maskV = MASK_NONE;
                        delete [] tok;
                        continue;
                    }
                    if (isdigit(*tok)) {
                        maskV = atoi(tok);
                        delete [] tok;
                        continue;
                    }
                    db->setErrMsg(lstring::copy("stage1: bad -m value."));
                    delete [] tok;
                    stringlist::destroy(routeNames);
                    return (LD_BAD);
                }
            }
            db->setErrMsg(write_msg("stage1: unknown option %s.", tok));
            delete [] tok;
            stringlist::destroy(routeNames);
            return (LD_BAD);
        }
        else {
            // Anything not an option is a route name.
            routeNames = new stringlist(tok, routeNames);
        }
    }

    if (!dostep)
        mr_stepnet = -1;
    else
        mr_stepnet++;

    int omask = maskVal();
    bool oforce = forceRoutable();
    setMaskVal(maskV);
    setForceRoutable(forceRt);

    int failcount = 0;
    if (!routeNames)
        failcount = doFirstStage(dodebug, mr_stepnet);
    else {
        for (stringlist *sl = routeNames; sl; sl = sl->next) {
            dbNet *net = getNet(sl->string);
            if (!net) {
                // No such net, issue a warning.
                db->setWarnMsg(
                    write_msg("stage1: no such net %s.", sl->string));
                continue;
            }
            if (net->netnodes) {
                int result = doRoute(net, mrStage1, dodebug);
                if (result)
                    failcount++;
                else {
                    // Remove from FailedNets list if routing
                    // was successful.

                    mr_failedNets.remove(net);
                }
            }
        }
        stringlist::destroy(routeNames);
    }
    setMaskVal(omask);
    setForceRoutable(oforce);
    if (mr_stepnet >= ((int)numNets() - 1))
        mr_stepnet = -1;
    if (failcount)
        db->setDoneMsg(write_msg("stage1:  %d failed nets.", failcount));
    return (LD_OK);
}


// Execute stage2 routing.  This stage works through the "FailedNets"
// list, routing with collisions, and then ripping up the colliding
// nets and appending them to the "FailedNets" list.
//
// The interpreter result is set to the number of failed routes at the
// end of the second stage.
//
// Options:
//
//  -d          Draw the area being searched in real-time.  This slows
//               down the algorithm and is intended only for diagnostic use.
//  -s          Single-step stage two
//  -m[ ]n[one] Don't limit the search area
//  -m[ ]a[uto] Select the mask automatically
//  -m[ ]b[box] Use the net bbox as a mask
//  -m[ ]<value> Set the mask size to <value>, an integer typ. 0 and up.
//  -f          Force a terminal to be routable.
//  -l <n>      Fail route if solution collides with more than <n> nets.
//  -t <n>      Keep trying n additional times.
//  <net>       Route net named <net> only.
//
bool
cMRouter::cmdStage2(const char *cmd)
{
    clearMsgs();
    int maskV = MASK_NONE;
    bool dodebug = false;
    bool forceRt = false;
    bool dostep = false;
    stringlist *routeNames = 0;
    int limit = 0;
    int tries = 0;

    char *tok;
    while ((tok = lstring::gettok(&cmd)) != 0) {
        if (*tok == '-') {
            if (tok[1] == 'd') {
                dodebug = true;
                delete [] tok;
                continue;
            }
            if (tok[1] == 'f') {
                forceRt = true;
                delete [] tok;
                continue;
            }
            if (tok[1] == 's') {
                dostep = true;
                delete [] tok;
                continue;
            }
            if (tok[1] == 'l') {
                delete [] tok;
                tok = lstring::gettok(&cmd);
                if (!tok || !isdigit(*tok)) {
                    db->setErrMsg(
                        lstring::copy("stage2: missing or bad -l value."));
                    delete [] tok;
                    stringlist::destroy(routeNames);
                    return (LD_BAD);
                }
                limit = atoi(tok);
                delete [] tok;
                continue;
            }
            if (tok[1] == 't') {
                delete [] tok;
                tok = lstring::gettok(&cmd);
                if (!tok || !isdigit(*tok)) {
                    db->setErrMsg(
                        lstring::copy("stage2: missing or bad -t value."));
                    delete [] tok;
                    stringlist::destroy(routeNames);
                    return (LD_BAD);
                }
                tries = atoi(tok);
                delete [] tok;
                continue;
            }
            if (tok[1] == 'm') {
                if (tok[2] == 'a') {
                    maskV = MASK_AUTO;
                    delete [] tok;
                    continue;
                }
                if (tok[2] == 'b') {
                    maskV = MASK_BBOX;
                    delete [] tok;
                    continue;
                }
                if (tok[2] == 'n') {
                    maskV = MASK_NONE;
                    delete [] tok;
                    continue;
                }
                if (isdigit(tok[2])) {
                    maskV = atoi(tok+2);
                    delete [] tok;
                    continue;
                }
                if (!tok[2]) {
                    delete tok;
                    tok = lstring::gettok(&cmd);
                    if (!tok) {
                        db->setErrMsg(
                            lstring::copy("stage2: missing -m value."));
                        delete [] tok;
                        stringlist::destroy(routeNames);
                        return (LD_BAD);
                    }
                    if (*tok == 'a') {
                        maskV = MASK_AUTO;
                        delete [] tok;
                        continue;
                    }
                    if (*tok == 'b') {
                        maskV = MASK_BBOX;
                        delete [] tok;
                        continue;
                    }
                    if (*tok == 'n') {
                        maskV = MASK_NONE;
                        delete [] tok;
                        continue;
                    }
                    if (isdigit(*tok)) {
                        maskV = atoi(tok);
                        delete [] tok;
                        continue;
                    }
                    db->setErrMsg(
                        lstring::copy("stage2: bad -m value."));
                    delete [] tok;
                    stringlist::destroy(routeNames);
                    return (LD_BAD);
                }
            }
            db->setErrMsg(write_msg("stage2: unknown option %s.", tok));
            delete [] tok;
            stringlist::destroy(routeNames);
            return (LD_BAD);
        }
        else {
            // Anything not an option is a route name.
            routeNames = new stringlist(tok, routeNames);
        }
    }

    int omask = maskVal();
    bool oforce = forceRoutable();
    setMaskVal(maskV);
    setForceRoutable(forceRt);

    int otries = keepTrying();
    int olimit = ripLimit();
    if (tries > 0)
        setKeepTrying(tries);
    if (limit > 0)
        setRipLimit(limit);

    int failcount = 0;
    if (!routeNames)
        failcount = doSecondStage(dodebug, dostep);
    else {
        for (stringlist *sl = routeNames; sl; sl = sl->next) {
            dbNet *net = getNet(sl->string);
            if (!net) {
                // No such net, issue a warning.
                db->setWarnMsg(
                    write_msg("stage2: no such net %s.", sl->string));
                continue;
            }
            failcount += routeNetRipup(net, dodebug);
        }
        stringlist::destroy(routeNames);
    }
    setKeepTrying(otries);
    setRipLimit(olimit);
    setMaskVal(omask);
    setForceRoutable(oforce);

    if (failcount)
        db->setDoneMsg(write_msg("stage2:  %d failed nets.", failcount));
    return (LD_OK);
}


// Execute stage3 routing.  This works through the
// entire netlist, ripping up each route in turn and
// re-routing it.
//
// The interpreter result is set to the number of
// failed routes at the end of the first stage.
//
// Options:
//
//  -d          Draw the area being searched in real-time.  This slows
//               down the algorithm and is intended only for diagnostic use.
//  -s          Single-step stage three.
//  -m[ ]n[one] Don't limit the search area
//  -m[ ]a[uto] Select the mask automatically
//  -m[ ]b[box] Use the net bbox as a mask
//  -m[ ]<value> Set the mask size to <value>, an integer typ. 0 and up.
//  -f          Force a terminal to be routable
//  -l <n>      Fail route if solution collides with more than <n> nets.
//  -t <n>      Keep trying n additional times.
//  <net>       Route net named <net> only.
//
bool
cMRouter::cmdStage3(const char *cmd)
{
    clearMsgs();
    int maskV = MASK_NONE;
    bool dodebug = false;
    bool forceRt = false;
    bool dostep = false;
    stringlist *routeNames = 0;
    int limit = 0;
    int tries = 0;

    char *tok;
    while ((tok = lstring::gettok(&cmd)) != 0) {
        if (*tok == '-') {
            if (tok[1] == 'd') {
                dodebug = true;
                delete [] tok;
                continue;
            }
            if (tok[1] == 'f') {
                forceRt = true;
                delete [] tok;
                continue;
            }
            if (tok[1] == 's') {
                dostep = true;
                delete [] tok;
                continue;
            }
            if (tok[1] == 'l') {
                delete [] tok;
                tok = lstring::gettok(&cmd);
                if (!tok || !isdigit(*tok)) {
                    db->setErrMsg(
                        lstring::copy("stage3: missing or bad -l value."));
                    delete [] tok;
                    stringlist::destroy(routeNames);
                    return (LD_BAD);
                }
                limit = atoi(tok);
                delete [] tok;
                continue;
            }
            if (tok[1] == 't') {
                delete [] tok;
                tok = lstring::gettok(&cmd);
                if (!tok || !isdigit(*tok)) {
                    db->setErrMsg(
                        lstring::copy("stage3: missing or bad -t value."));
                    delete [] tok;
                    stringlist::destroy(routeNames);
                    return (LD_BAD);
                }
                tries = atoi(tok);
                delete [] tok;
                continue;
            }
            if (tok[1] == 'm') {
                if (tok[2] == 'a') {
                    maskV = MASK_AUTO;
                    delete [] tok;
                    continue;
                }
                if (tok[2] == 'b') {
                    maskV = MASK_BBOX;
                    delete [] tok;
                    continue;
                }
                if (tok[2] == 'n') {
                    maskV = MASK_NONE;
                    delete [] tok;
                    continue;
                }
                if (isdigit(tok[2])) {
                    maskV = atoi(tok+2);
                    delete [] tok;
                    continue;
                }
                if (!tok[2]) {
                    delete tok;
                    tok = lstring::gettok(&cmd);
                    if (!tok) {
                        db->setErrMsg(
                            lstring::copy("stage3: missing -m value."));
                        delete [] tok;
                        stringlist::destroy(routeNames);
                        return (LD_BAD);
                    }
                    if (*tok == 'a') {
                        maskV = MASK_AUTO;
                        delete [] tok;
                        continue;
                    }
                    if (*tok == 'b') {
                        maskV = MASK_BBOX;
                        delete [] tok;
                        continue;
                    }
                    if (*tok == 'n') {
                        maskV = MASK_NONE;
                        delete [] tok;
                        continue;
                    }
                    if (isdigit(*tok)) {
                        maskV = atoi(tok);
                        delete [] tok;
                        continue;
                    }
                    db->setErrMsg(lstring::copy("stage3: bad -m value."));
                    delete [] tok;
                    stringlist::destroy(routeNames);
                    return (LD_BAD);
                }
            }
            db->setErrMsg(write_msg("stage3: unknown option %s.", tok));
            delete [] tok;
            stringlist::destroy(routeNames);
            return (LD_BAD);
        }
        else {
            // Anything not an option is a route name.
            routeNames = new stringlist(tok, routeNames);
        }
    }

    if (!dostep)
        mr_stepnet = -1;
    else
        mr_stepnet++;

    int omask = maskVal();
    bool oforce = forceRoutable();
    setMaskVal(maskV);
    setForceRoutable(forceRt);

    int otries = keepTrying();
    int olimit = ripLimit();
    if (tries > 0)
        setKeepTrying(tries);
    if (limit > 0)
        setRipLimit(limit);

    int failcount = 0;
    if (!routeNames)
        failcount = doThirdStage(dodebug, mr_stepnet);
    else {
        for (stringlist *sl = routeNames; sl; sl = sl->next) {
            dbNet *net = getNet(sl->string);
            if (!net) {
                // No such net, issue a warning.
                db->setWarnMsg(
                    write_msg("stage3: no such net %s.", sl->string));
                continue;
            }
            if (net->netnodes) {
                int result = doRoute(net, mrStage1, dodebug);
                if (result)
                    failcount++;
                else {
                    // Remove from FailedNets list if routing was
                    // successful.

                    mr_failedNets.remove(net);
                }
            }
        }
    }
    setKeepTrying(otries);
    setRipLimit(olimit);
    setMaskVal(omask);
    setForceRoutable(oforce);

    if (mr_stepnet >= ((int)numNets() - 1))
        mr_stepnet = -1;
    if (failcount)
        db->setErrMsg(write_msg("stage3:  %d failed nets.", failcount));
    return (LD_OK);
}


// Rip up (destroy routing of) a net or nets, or all nets.
//
// Options:
//   -a             Remove all nets from the design
//   <name> [...]   Remove the named net(s) from the design.
//
bool
cMRouter::cmdRipUp(const char *cmd)
{
    clearMsgs();
    stringlist *netnames = 0;
    char *tok;
    while ((tok = lstring::gettok(&cmd)) != 0)
        netnames = new stringlist(tok, netnames);

    if (netnames) {
        if (!strcmp(netnames->string, "-a")) {
            // Remove all nets.
            for (u_int i = 0; i < numNets(); i++) {
               dbNet *net = nlNet(i);
               ripupNet(net, true);
            }
            db->setDoneMsg(lstring::copy("All nets ripped up."));
        }
        else {
            int cnt = 0;
            for (stringlist *s = netnames; s; s = s->next) {
                dbNet *net = getNet(s->string);
                if (!net) {
                    // No such net, issue a warning.
                    db->setWarnMsg(
                        write_msg("ripup: no such net %s.", s->string));
                    continue;
                }
                ripupNet(net, true);
                cnt++;
            }
            char buf[64];
            sprintf(buf, "%d nets ripped up.", cnt);
            db->setDoneMsg(lstring::copy(buf));
        }
        stringlist::destroy(netnames);
    }
    return (LD_OK);
}


// Default operation is to simply print the number of failed routes.
//
// Options:
//
//   -a     Move all nets to FailedNets ordered by the standard metric.
//   -u     Move all nets to FailedNets, as originally ordered.
//   -p     Print a list of failed net names.
//
bool
cMRouter::cmdFailed(const char *s)
{
    clearMsgs();
    bool all = false;
    bool unordered = false;
    bool print = false;

    char *tok;
    while ((tok = lstring::gettok(&s)) != 0) {
        if (!strcmp(tok, "-a"))
            all = true;
        else if (!strcmp(tok, "-u"))
            unordered = true;
        else if (!strcmp(tok, "-p"))
            print = true;
        delete [] tok;
    }

    if (unordered) {
        // Free up FailedNets list and then move all nets to
        // FailedNets.

        mr_failedNets.clear();
        for (u_int i = 0; i < numNets(); i++) {
            dbNet *net = nlNet(i);
            mr_failedNets.append(net);
        }
    }
    else if (all) {
        create_net_order();

        mr_failedNets.clear();
        for (u_int i = 0; i < numNets(); i++) {
            dbNet *net = mr_nets[i];
            mr_failedNets.append(net);
        }
    }

    if (print) {
        int cnt = mr_failedNets.num_elements();
        if (!cnt)
            db->setDoneMsg(lstring::copy("There are no failed nets."));
        else {
            // Print names of failed nets.
            const char *t = "Failed Nets:\n";
            int len = strlen(t);
            for (dbNetList *nl = mr_failedNets.list(); nl; nl = nl->next)
                len += strlen(nl->net->netname) + 3;
            len++;
            char *dmsg = new char[len];
            char *e = lstring::stpcpy(dmsg, t);
            for (dbNetList *nl = mr_failedNets.list(); nl; nl = nl->next) {
                *e++ = ' ';
                *e++ = ' ';
                e = lstring::stpcpy(e, nl->net->netname);
                *e++ = '\n';
            }
            *e = 0;
            db->setDoneMsg(dmsg);
        }
    }
    else {
        // Print number of failed nets, number of nets.

        int cnt = mr_failedNets.num_elements();
        char buf[80];
        snprintf(buf, 80, "There are %d failed nets out of %d total.",
            cnt, numNets());
        db->setDoneMsg(lstring::copy(buf));
    }
    return (LD_OK);
}


namespace {
    struct dbCongestion
    {
        dbGate  *gate;
        double  congestion;
    };


    bool congcmp(const dbCongestion *c1, const dbCongestion *c2)
    {
        return (c2->congestion < c1->congestion);
    }
}


// Command to compute and return congestion for each cell instance. 
// This list can be passed back to a placement tool to direct it to
// add padding around these cells to ease congestion and make the
// layout more routable.
//
// congested [-n N] [filename]
//
// Options:
//   -n N   Print the top N congested cell instances in the design.
//
// If filename is given, output goes to that file, otherwise output
// goes to the standard output.
//
bool
cMRouter::cmdCongested(const char *s)
{
    clearMsgs();
    int entries = 0;
    char *fname = 0;
    char buf[256];

    if (!db->numGates()) {
        db->setErrMsg(lstring::copy("congestion: no gates in design."));
        return (LD_BAD);
    }

    char *tok;
    while ((tok = lstring::gettok(&s)) != 0) {
        if (!strcmp(tok, "-n")) {
            delete [] tok;
            tok = lstring::gettok(&s);
            if (!tok)
                break;
            if (sscanf(tok, "%d", &entries) != 1 || entries < 0) {
                db->setErrMsg(
                    lstring::copy("congestion: syntax error, bad -n value."));
                delete [] fname;
                return (LD_BAD);
            }
            delete [] tok;
            continue;
        }
        if (!fname)
            fname = tok;
        else {
            snprintf(buf, 256, "congestion: unknown argument %s.", tok);
            db->setErrMsg(lstring::copy(buf));
            delete [] fname;
            delete [] tok;
            return (LD_BAD);
        }
    }

    u_int sz = numChannelsX(0) * numChannelsY(0);
    float *cgst = new float[sz];
    memset(cgst, 0, sz*sizeof(float));
#define CONGEST(x, y) cgst[x + y*numChannelsX(0)]

    // Use net bounding boxes to estimate congestion.

    for (u_int i = 0; i < numNets(); i++) {
        dbNet *net = nlNet(i);
        int nwidth = (net->xmax - net->xmin + 1);
        int nheight = (net->ymax - net->ymin + 1);
        int area = nwidth * nheight;
        int length;
        if (nwidth > nheight) {
            length = nwidth + (nheight >> 1) * net->numnodes;
        }
        else {
            length = nheight + (nwidth >> 1) * net->numnodes;
        }
        float density = (float)length / area;

        for (int x = net->xmin; x < net->xmax; x++) {
            for (int y = net->ymin; y < net->ymax; y++) {
                if (x >= 0 && x < numChannelsX(0) &&
                        y >= 0 && y < numChannelsY(0))
                    CONGEST(x, y) += density;
            }
        }
    }

    // Use instance bounding boxes to estimate average congestion
    // in the area of an instance.

    dbCongestion **cgates = new dbCongestion*[numGates()];

    for (u_int i = 0; i < numGates(); i++) {
        dbGate *gsrch = nlGate(i);

        dbSeg bbox;
        cgates[i] = new dbCongestion;
        int dx = gsrch->placedX;
        int dy = gsrch->placedY;
        bbox.x1 = (int)((dx - xLower()) / pitchX(0)) - 1;
        bbox.y1 = (int)((dy - yLower()) / pitchY(0)) - 1;
        dx = gsrch->placedX + gsrch->width;
        dy = gsrch->placedY + gsrch->height;
        bbox.x2 = (int)((dx - xLower()) / pitchX(0)) - 1;
        bbox.y2 = (int)((dy - yLower()) / pitchY(0)) - 1;

        float cavg = 0.0;
        for (int x = bbox.x1; x <= bbox.x2; x++) {
            for (int y = bbox.y1; y <= bbox.y2; y++) {
                cavg += CONGEST(x, y);
            }
        }
        cavg /= (bbox.x2 - bbox.x1 + 1);
        cavg /= (bbox.y2 - bbox.y1 + 1);

        cgates[i]->gate = gsrch;
        cgates[i]->congestion = cavg / numLayers();
    }
    delete [] cgst;

    std::sort(cgates, cgates + numGates(), congcmp);

    if (entries <= 0)
        entries = numGates();
    else if (entries > (int)numGates())
        entries = numGates();

    sLstr lstr;
    if (fname) {
        FILE *fp = fopen(fname, "w");
        if (!fp) {
            snprintf(buf, 256, "congestion: can't open %s for output.", fname);
            db->setErrMsg(lstring::copy(buf));
            delete [] fname;
            for (u_int i = 0; i < numGates(); i++)
                delete cgates[i];
            delete [] cgates;
            return (LD_BAD);
        }
        // Add a brief header as in Qrouter.
        int nfailed = 0;
        for (dbNetList *n = failedNets(); n; n = n->next)
            nfailed++;
        fprintf(fp, "MRouter congestion summary\n");
        fprintf(fp, "--------------------------\n");
        fprintf(fp, "Failures: %u %u\n", nfailed, numNets());
        fprintf(fp, "--------------------------\n");
        for (int i = 0; i < entries; i++) {
            fprintf(fp, "%-20s %g\n", cgates[i]->gate->gatename,
                cgates[i]->congestion);
        }
        fclose(fp);
        lstr.add("Wrote congestion information into ");
        lstr.add(fname);
        lstr.add(".\n");
        delete [] fname;
    }
    else {
        for (int i = 0; i < entries; i++) {
            snprintf(buf, 256, "%-20s %g\n", cgates[i]->gate->gatename,
                cgates[i]->congestion);
            lstr.add(buf);
        }
    }
    db->setDoneMsg(lstr.string_trim());

    // Cleanup
    for (u_int i = 0; i < numGates(); i++)
        delete cgates[i];
    delete [] cgates;

    return (LD_OK);
}


#ifdef notdef
// Import from qrouter-1.3.62, work in progress.

/*--------------------------------------------------------------*/
/* Find a node in the node list.  If qrouter is compiled with   */
/* Tcl support, then this routine uses hash tables, greatly     */
/* speeding up the output of large delay files.             */
/*--------------------------------------------------------------*/

#ifndef TCL_QROUTER

dbGate *
FindGateNode(NODE node, int *ridx)
{
    // This is a slow lookup, hashing is preferred.

    for (dbGate *g = nlGates(); g; g = g->next) {
        for (u_int i = 0; i < g->nodes; i++) {
            if (g->noderec[i] == node) {
                *ridx = i;
                return (g);
            }
        }
    }
    return (0);
}

#else /* The version using TCL hash tables */

/* Define record holding information pointing to a gate and the */
/* index into a specific node of that gate.         */

typedef struct gatenode_ *GATENODE;

struct gatenode_ {
    GATE gate;
    int idx;
};

GATE
FindGateNode(Tcl_HashTable *NodeTable, NODE node, int *ridx)
{
    GATENODE gn;
    GATE g;
    Tcl_HashEntry *entry;

    entry = Tcl_FindHashEntry(NodeTable, node);
    gn = (entry) ? (GATENODE)Tcl_GetHashValue(entry) : NULL;
    *ridx = gn->idx;
    return gn->gate;
}

#endif  /* TCL_QROUTER */

/*--------------------------------------------------------------*/
/* Write an output file of the calculated R, C for every route  */
/* branch.                          */
/*--------------------------------------------------------------*/
//int write_delays(char *filename)

bool
cMRouter::write_delays(const char *s)
{
    NET net;
    ROUTE rt;
    SEG seg;
    int i, new;

    char *filename = 0;

    char *tok;
    while ((tok = lstring::gettok(&s)) != 0) {
        filename = tok;
        break;
    }

    FILE *delayFile;
    if (!filename)
        delayFile = stdout;
    else {
        delayFile = fopen(filename, "w");
        if (!delayFile) {
            db->setErrMsg(lstring::copy(
                "write_delays:  Couldn't open output delay file.\n");
            delete [] filename;
            return (LD_BAD);
        }
    }

#ifdef TCL_QROUTER    
    /* Build a hash table of nodes;  key = node record address,     */
    /* record = pointer to gate and index of the node in its noderec.   */

    Tcl_InitHashTable(&NodeTable, TCL_ONE_WORD_KEYS);

    for (g = Nlgates; g; g = g->next) {
    for (i = 0; i < g->nodes; i++) {
        GATENODE gn;
        gn = (GATENODE)malloc(sizeof(struct gatenode_));
        gn->idx = i;
        gn->gate = g;
        entry = Tcl_CreateHashEntry(&NodeTable, g->noderec + i, &new);
        Tcl_SetHashValue(entry, gn);
    }
    }
#endif

    for (u_int i = 0; i < numNets(); i++) {
        dbNet *net = nlNet(i);
        fprintf(delayFile, "%s", net->netname);

        // For each route, determine if endpoints land on another
        // route, mark each such point.  If the point is in the middle
        // of a segment, then break the segment at that point.

        // At the same time, determine the driver node, as determined
        // by the node with LEF direction 'OUTPUT'.  (For now, if a
        // net has multiple tristate drivers, just use the first one
        // and treat the rest as receivers.)

        for (dbRoute *rt = net->routes; rt; rt = rt->next) {
            for (dbDseg *seg = rt->segments; seg; seg = seg->next) {
                // WIP
            }
        }

        // Determine the driver

        // Walk the routes from the driver to all receivers recursively.
    }

    fclose(delayFile);

    return (LD_OK);
}

#endif


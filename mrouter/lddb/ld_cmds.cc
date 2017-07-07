
/*========================================================================*
 *                                                                        *
 *  Whiteley Research Inc, Sunnyvale, CA USA 2016, http://wrcad.com       *
 *                                                                        *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,      *
 *   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES      *
 *   OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-        *
 *   INFRINGEMENT.  IN NO EVENT SHALL STEPHEN R. WHITELEY OR WHITELEY     *
 *   RESEARCH INC. BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,   *
 *   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,   *
 *   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER        *
 *   DEALINGS IN THE SOFTWARE.                                            *
 *                                                                        *
 *   Licensed under the Apache License, Version 2.0 (the "License");      *
 *   you may not use this file except in compliance with the License.     *
 *   You may obtain a copy of the License at                              *
 *                                                                        *
 *        http://www.apache.org/licenses/LICENSE-2.0                      *
 *                                                                        *
 *   Unless required by applicable law or agreed to in writing, software  *
 *   distributed under the License is distributed on an "AS IS" BASIS,    *
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or      *
 *   implied. See the License for the specific language governing         *
 *   permissions and limitations under the License.                       *
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
 $Id: ld_cmds.cc,v 1.11 2017/02/17 05:44:38 stevew Exp $
 *========================================================================*/

#include "lddb_prv.h"
#include <stdarg.h>
#include <unistd.h>


//
// LEF/DEF Database.
//
// This is a high-level control interface, used to implement, e.g.,
// command line handling.  The functions return true on success, with
// possibly a message in db_donemsg and/or db_warnmsg.  False is
// returned on error, with a message in db_errmsg.
//


// A script file consists of one or more commands, one per line.  This
// function applies only to the database, however, and should be
// called only if a stand-alone database is being used.  The router
// version, which chains to commands provided here, is appropriate to
// call for a complete router.
//
bool
cLDDB::readScript(const char *fname)
{
    if (!fname || !*fname) {
        emitErrMesg("ERROR: null or empty file name.\n");
        return (LD_BAD);
    }

    FILE *fp = fopen(fname, "r");
    if (!fp) {
        emitErrMesg("ERROR: failed to open %s,\n", fname);
        return (LD_BAD);
    }
    bool ret = readScript(fp);

    fclose(fp);
    return (ret);
}


bool
cLDDB::readScript(FILE *fp)
{
    if (!fp)
        return (LD_BAD);
#ifdef WIN32
    bool istty = _isatty(_fileno(fp));
#else
    bool istty = isatty(fileno(fp));
#endif
    char buf[256], *line;
    bool ret = LD_OK;
    if (istty) {
        fprintf(fp, "? ");
        fflush(fp);
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
        if (db_errmsg)
            emitErrMesg("ERROR: %s\n%s\n", line, db_errmsg);
        else {
            if (db_warnmsg)
                emitMesg("WARNING: %s\n%s\n", line, db_warnmsg);
            if (db_donemsg)
                emitMesg("%s\n", db_donemsg);
        }
        if (ret == LD_BAD)
            break;
        if (istty) {
            fprintf(fp, "? ");
            fflush(fp);
        }
    }
    clearMsgs();
    return (ret);
}


// Process a single line command.  This can be used for interactive
// command line processing, or in script processing.  This applies
// only to database commands.
//
bool
cLDDB::doCmd(const char *cmd)
{
    clearMsgs();
    const char *s = cmd;
    char *tok = lddb::gettok(&s);
    if (!tok)
        return (LD_OK);

    bool ret = LD_BAD;
    bool unhandled = false;
    if (!strcmp(tok,        "version")) {
        char buf[64];
        sprintf(buf, "LDDB release: %s", LD_VERSION);
        db_donemsg = lddb::copy(buf);
        ret = LD_OK;
    }
    else if (!strcmp(tok,   "reset"))
        ret = reset();
    else if (!strcmp(tok,   "set"))
        ret = cmdSet(s);
    else if (!strcmp(tok,   "unset"))
        ret = cmdUnset(s);
    else if (!strcmp(tok,   "ignore"))
        ret = cmdIgnore(s);
    else if (!strcmp(tok,   "critical"))
        ret = cmdCritical(s);
    else if (!strcmp(tok,   "obstruction"))
        ret = cmdObstruction(s);
    else if (!strcmp(tok,   "layer"))
        ret = cmdLayer(s);
    else if (!strcmp(tok,   "newlayer"))
        ret = cmdNewLayer(s);
    else if (!strcmp(tok,   "boundary"))
        ret = cmdBoundary(s);

    else if (!strcmp(tok,   "read")) {
        delete [] tok;
        tok = lddb::gettok(&s);
        if (!strcmp(tok,        "script")) {
            delete [] tok;
            tok = lddb::getqtok(&s);
            ret = readScript(tok);
        }
        else if (!strcmp(tok,   "lef")) {
            delete [] tok;
            tok = lddb::getqtok(&s);
            ret = cmdReadLef(tok);
        }
        else if (!strcmp(tok,   "def")) {
            delete [] tok;
            tok = lddb::getqtok(&s);
            ret = cmdReadDef(tok);
        }
        else
            unhandled = true;
    }
    else if (!strcmp(tok,   "write")) {
        delete [] tok;
        tok = lddb::gettok(&s);
        if (!strcmp(tok,        "lef")) {
            delete [] tok;
            tok = lddb::getqtok(&s);
            ret = cmdWriteLef(tok);
        }
        else if (!strcmp(tok,   "def")) {
            delete [] tok;
            tok = lddb::getqtok(&s);
            ret = cmdWriteDef(tok);
        }
        else
            unhandled = true;
    }
    else if (!strcmp(tok,   "append")) {
        delete [] tok;
        tok = lddb::getqtok(&s);
        char *t2 = lddb::getqtok(&s);
        ret = cmdUpdateDef(tok, t2);
        delete [] t2;
    }
    else
        unhandled = true;
    delete [] tok;

    if (unhandled) {
        db_errmsg = lddb::copy("unknown command");
        return (LD_BAD);
    }
    return (ret);
}


namespace {
    char *write_msg(const char *fmt, ...)
    {
        char buf[256];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buf, 256, fmt, args);
        return (lddb::copy(buf));
    }
}


bool
cLDDB::cmdSet(const char *cmd)
{
    clearMsgs();
    char buf[80];
    char *tok = lddb::gettok(&cmd);
    if (!tok) {
        dbLstr lstr;
        const char *fmt = "%-16s: ";

        sprintf(buf, fmt, "debug");
        lstr.add(buf);
        sprintf(buf, "0x%x\n", debug());
        lstr.add(buf);

        sprintf(buf, fmt, "verbose");
        lstr.add(buf);
        sprintf(buf, "%d\n", verbose());
        lstr.add(buf);

        sprintf(buf, fmt, "global");
        lstr.add(buf);
        lstr.add("global:");
        for (int i = 0; i < LD_MAX_GLOBALS; i++) {
            if (global(i)) {
                lstr.add_c(' ');
                lstr.add(global(i));
            }
        }
        lstr.add_c('\n');

        sprintf(buf, fmt, "layers");
        lstr.add(buf);
        sprintf(buf, "%u\n", numLayers());
        lstr.add(buf);

        sprintf(buf, fmt, "maxnets");
        lstr.add(buf);
        sprintf(buf, "%u\n", maxNets());
        lstr.add(buf);

        sprintf(buf, fmt, "lefresol");
        lstr.add(buf);
        sprintf(buf, "%d\n", lefResol());
        lstr.add(buf);

        sprintf(buf, fmt, "mfggrid");
        lstr.add(buf);
        sprintf(buf, "%g\n", lefToMic(manufacturingGrid()));
        lstr.add(buf);

        sprintf(buf, fmt, "definresol");
        lstr.add(buf);
        sprintf(buf, "%d\n", defInResol());
        lstr.add(buf);

        sprintf(buf, fmt, "defoutresol");
        lstr.add(buf);
        sprintf(buf, "%d\n", defOutResol());
        lstr.add(buf);

        db_donemsg = lstr.string_trim();
        return (LD_OK);
    }
    if (!strcasecmp(tok, "debug")) {
        delete [] tok;
        tok = lddb::gettok(&cmd);
        if (!tok) {
            sprintf(buf, "debug: 0x%x", debug());
            db_donemsg = lddb::copy(buf);
        }
        else {
            setDebug(strtol(tok, 0, 0));
            delete [] tok;
        }
        return (LD_OK);
    }
    if (!strcasecmp(tok, "verbose")) {
        delete [] tok;
        tok = lddb::gettok(&cmd);
        if (!tok) {
            sprintf(buf, "verbose: %d", verbose());
            db_donemsg = lddb::copy(buf);
        }
        else {
            if (isdigit(*tok)) {
                u_int i = atoi(tok);
                setVerbose(i);
            }
            delete [] tok;
        }
        return (LD_OK);
    }
    if (!strcasecmp(tok, "global") || !strcasecmp(tok, "gnd") ||
            !strcasecmp(tok, "vdd")) {
        // The 'gnd' and 'vdd' keywords are obsolete, but still
        // semi-supported here, as aliases for 'global'.

        delete [] tok;
        tok = lddb::gettok(&cmd);
        if (!tok) {
            dbLstr lstr;
            lstr.add("global:");
            for (int i = 0; i < LD_MAX_GLOBALS; i++) {
                if (global(i)) {
                    lstr.add_c(' ');
                    lstr.add(global(i));
                }
            }
            db_donemsg = lstr.string_trim();
        }
        else {
            do {
                if (addGlobal(tok) == LD_BAD) {
                    db_errmsg = write_msg("too many global nets, limit %u.",
                        LD_MAX_GLOBALS);
                    delete [] tok;
                    break;
                }
                delete [] tok;
            } while ((tok = lddb::gettok(&cmd)) != 0);
        }
        return (LD_OK);
    }
    if (!strcasecmp(tok, "layers")) {
        // Set the number of layers assumed.

        delete [] tok;
        tok = lddb::gettok(&cmd);
        if (!tok) {
            sprintf(buf, "layers: %u", numLayers());
            db_donemsg = lddb::copy(buf);
        }
        else {
            if (isdigit(*tok)) {
                u_int i = atoi(tok);
                if (1 > db_allocLyrs) {
                    db_errmsg = write_msg("too many layers %u, available %u.",
                        i, db_allocLyrs);
                    delete [] tok;
                    return (LD_BAD);
                }
                if (i == 0) {
                    db_errmsg = write_msg("at least one layer required.");
                    delete [] tok;
                    return (LD_BAD);
                }
                setNumLayers(i);
            }
            else {
                db_errmsg = write_msg(
                    "bad value %s, expecting positive integer.", tok);
                delete [] tok;
                return (LD_BAD);
            }
            delete [] tok;
        }
        return (LD_OK);
    }
    if (!strcasecmp(tok, "maxnets")) {
        // Set the maximum number of nets that the database will
        // accept.

        delete [] tok;
        tok = lddb::gettok(&cmd);
        if (!tok) {
            sprintf(buf, "maxnets: %u", maxNets());
            db_donemsg = lddb::copy(buf);
        }
        else {
            if (isdigit(*tok)) {
                u_int i = atoi(tok);
                if (i == 0) {
                    db_errmsg = write_msg(
                        "bad value %s, expecting positive integer.", tok);
                    delete [] tok;
                    return (LD_BAD);
                }
                if (i > dfMaxNets()) {
                    db_errmsg = write_msg(
                        "bad value %u, maximum is %u.", i, dfMaxNets());
                    delete [] tok;
                    return (LD_BAD);
                }
                setMaxNets(i);
            }
            else {
                db_errmsg = write_msg(
                    "bad value %s, expecting positive integer.", tok);
                delete [] tok;
                return (LD_BAD);
            }
            delete [] tok;
        }
        return (LD_OK);
    }
    if (!strcasecmp(tok, "lefresol")) {
        // Set the LEF resolution as if read from a LEF file.  The
        // given resolution applies to all length/position values
        // subsequently stored in the database, and to LEF output.

        delete [] tok;
        tok = lddb::gettok(&cmd);
        if (!tok) {
            sprintf(buf, "lefresol: %d", lefResol());
            db_donemsg = lddb::copy(buf);
        }
        else {
            if (isdigit(*tok)) {
                u_int i = atoi(tok);
                if (setLefResol(i) != LD_OK) {
                    delete [] tok;
                    return (LD_BAD);
                }
            }
            else {
                db_errmsg = write_msg(
                    "bad value %s, expecting positive integer.", tok);
                delete [] tok;
                return (LD_BAD);
            }
            delete [] tok;
        }
        return (LD_OK);
    }
    if (!strcasecmp(tok, "mfggrid")) {
        // Set the ManufacturingGrid as if read from a LEF file.

        delete [] tok;
        tok = lddb::gettok(&cmd);
        if (!tok) {
            sprintf(buf, "mfggrid: %g", lefToMic(manufacturingGrid()));
            db_donemsg = lddb::copy(buf);
        }
        else {
            if (isdigit(*tok)) {
                double d = atof(tok);
                if (setManufacturingGrid(micToLef(d)) != LD_OK) {
                    delete [] tok;
                    return (LD_BAD);
                }
            }
            else {
                db_errmsg = write_msg(
                    "bad value %s, expecting positive real number.", tok);
                delete [] tok;
                return (LD_BAD);
            }
            delete [] tok;
        }
        return (LD_OK);
    }
    if (!strcasecmp(tok, "definresol")) {
        // Set the DEF resolution as if read from a DEF file.

        delete [] tok;
        tok = lddb::gettok(&cmd);
        if (!tok) {
            sprintf(buf, "definresol: %d", defInResol());
            db_donemsg = lddb::copy(buf);
        }
        else {
            if (isdigit(*tok)) {
                int i = atoi(tok);
                if (setDefInResol(i) != LD_OK) {
                    delete [] tok;
                    return (LD_BAD);
                }
            }
            else {
                db_errmsg = write_msg(
                    "bad value %s, expecting positive integer.", tok);
                delete [] tok;
                return (LD_BAD);
            }
            delete [] tok;
        }
        return (LD_OK);
    }
    if (!strcasecmp(tok, "defoutresol")) {
        // Set the DEF resolution to use when writing output.

        delete [] tok;
        tok = lddb::gettok(&cmd);
        if (!tok) {
            sprintf(buf, "defoutresol: %d", defOutResol());
            db_donemsg = lddb::copy(buf);
        }
        else {
            if (isdigit(*tok)) {
                int i = atoi(tok);
                if (setDefOutResol(i) != LD_OK) {
                    delete [] tok;
                    return (LD_BAD);
                }
            }
            else {
                db_errmsg = write_msg(
                    "bad value %s, expecting positive integer.", tok);
                delete [] tok;
                return (LD_BAD);
            }
            delete [] tok;
        }
        return (LD_OK);
    }
//XXX add stuff here, e.g. from config file parser
// comment layer name

    db_errmsg = write_msg("Unknown keyword %s.", tok);
    delete [] tok;
    return (LD_BAD);
}


// Unset, i.e., revert to default, each "set" keyword biven.
//
bool
cLDDB::cmdUnset(const char *cmd)
{
    clearMsgs();
    char *tok;
    while ((tok = lddb::gettok(&cmd)) != 0) {
        if (!strcasecmp(tok, "debug"))
            setDebug(0);
        else if (!strcasecmp(tok, "verbose"))
            setVerbose(0);
        else if (!strcasecmp(tok, "global") || !strcasecmp(tok, "gnd") ||
                !strcasecmp(tok, "vdd"))
            clearGlobal(0);
        else if (!strcasecmp(tok, "layers"))
            setNumLayers(allocLayers());
        else if (!strcasecmp(tok, "maxnets"))
            setMaxNets(dfMaxNets());
        else if (!strcasecmp(tok, "lefresol"))
            ;  // Can't do it, ignore.
        else if (!strcasecmp(tok, "mfggrid"))
            setManufacturingGrid(0);
        else if (!strcasecmp(tok, "definresol"))
            ;  // Can't do it, ignore.
        else if (!strcasecmp(tok, "defoutresol"))
            setDefOutResol(0);
        else {
            db_errmsg = write_msg("Unknown keyword %s.", tok);
            delete [] tok;
            return (LD_BAD);
        }
        delete [] tok;
    }
    return (LD_OK);
}


// Specify one or more net names to be ignored by the router.  With no
// options, prints a list of nets being ignored by the router.
// 
// Options:
//    -u            Listed nets that follow are un-ignored.
//    -u all        All ignored nets are un-ignored.
//    [<net> ...]   Net names.
//
bool
cLDDB::cmdIgnore(const char *s)
{
    clearMsgs();
    dbStringList *netnames = 0;
    dbStringList *ne = 0;
    char *tok;
    while ((tok = lddb::gettok(&s)) != 0) {
        if (!netnames)
            ne = netnames = new dbStringList(tok, 0);
        else {
            ne->next = new dbStringList(tok, 0);
            ne = ne->next;
        }
    }

    if (!netnames) {
        int cnt = 0;
        for (u_int i = 0; i < numNets(); i++) {
            dbNet *net = nlNet(i);
            if (net->flags & NET_IGNORED)
                cnt++;
        }
        if (!cnt)
            db_donemsg = lddb::copy("No nets being ignored.");
        else {
            // Compose a listing.
            const char *t = "Ignored nets:\n";
            int len = strlen(t);
            for (u_int i = 0; i < numNets(); i++) {
                dbNet *net = nlNet(i);
                if (net->flags & NET_IGNORED)
                    len += strlen(net->netname) + 3;
            }
            len++;
            db_donemsg = new char[len];
            char *e = lddb::stpcpy(db_donemsg, t);
            for (u_int i = 0; i < numNets(); i++) {
                dbNet *net = nlNet(i);
                if (net->flags & NET_IGNORED) {
                    *e++ = ' ';
                    *e++ = ' ';
                    e = lddb::stpcpy(e, net->netname);
                    *e++ = '\n';
                }
            }
            *e = 0;
        }
    }
    else {
        bool unignore = false;
        for (dbStringList *sl = netnames; sl; sl = sl->next) {
            if (!strcmp(sl->string, "-u")) {
                unignore = true;
                if (sl->next && !strcasecmp(sl->next->string, "all")) {
                    for (u_int i = 0; i < numNets(); i++) {
                        dbNet *net = nlNet(i);
                        if (net->flags & NET_IGNORED)
                            net->flags &= ~NET_IGNORED;
                    }
                    dbStringList *cnets = noRouteList();
                    dbStringList::destroy(cnets);
                    setNoRouteList(0);
                    dbStringList::destroy(netnames);
                    db_donemsg = lddb::copy("Ignored net list cleared.");
                    return (LD_OK);
                }
                continue;
            }
            dbNet *net = getNet(sl->string);
            if (!net) {
                char *t = write_msg("ignore: no such net %s.", sl->string);
                if (!db_warnmsg)
                    db_warnmsg = t;
                else {
                    int len = strlen(db_warnmsg) + strlen(t) + 2;
                    char *tt = new char[len];
                    sprintf(tt, "%s\n%s", db_warnmsg, t);
                    delete [] t;
                    delete [] db_warnmsg;
                    db_warnmsg = tt;
                }
                continue;
            }
            if (unignore) {
                net->flags &= ~NET_IGNORED;
                dbStringList *cnets = noRouteList();
                dbStringList *cp = 0;
                for (dbStringList *cl = cnets; cl; cl = cl->next) {
                    if (!strcmp(cl->string, sl->string)) {
                        if (cp)
                            cp->next = cl->next;
                        else
                            setNoRouteList(cl->next);
                        delete [] cl->string;
                        delete cl;
                        break;
                    }
                    cp = cl;
                }
            }
            else {
                net->flags |= NET_IGNORED;
                dbStringList *cnets = noRouteList();
                bool found = false;
                dbStringList *cp = 0;
                for (dbStringList *cl = cnets; cl; cl = cl->next) {
                    if (!strcmp(cl->string, sl->string)) {
                        found = true;
                        // Move existing entry to front.
                        if (cp) {
                            cp->next = cl->next;
                            cl->next = cnets;
                            setNoRouteList(cl);
                        }
                        break;
                    }
                    cp = cl;
                }
                if (!found) {
                    setNoRouteList(
                        new dbStringList(lddb::copy(sl->string), cnets));
                }
            }
        }
        dbStringList::destroy(netnames);
    }
    return (LD_OK);
}


// Specify one or more net names to be routed first by
// the router.
//
// Options:
//    -u            Listed nets that follow are made un-critical.
//    -u all        Critical net list is cleared.
//    [<net> ...]   Net names.
//
bool
cLDDB::cmdCritical(const char *s)
{
    clearMsgs();
    dbStringList *netnames = 0;
    dbStringList *ne = 0;
    char *tok;
    while ((tok = lddb::gettok(&s)) != 0) {
        if (!netnames)
            ne = netnames = new dbStringList(tok, 0);
        else {
            ne->next = new dbStringList(tok, 0);
            ne = ne->next;
        }
    }

    if (!netnames) {
        int cnt = 0;
        for (u_int i = 0; i < numNets(); i++) {
            dbNet *net = nlNet(i);
            if (net->flags & NET_CRITICAL)
                cnt++;
        }
        if (!cnt)
            db_donemsg = lddb::copy("No critical nets.");
        else {
            // Compose a listing.
            const char *t = "Critical nets:\n";
            int len = strlen(t);
            for (u_int i = 0; i < numNets(); i++) {
                dbNet *net = nlNet(i);
                if (net->flags & NET_CRITICAL)
                    len += strlen(net->netname) + 3;
            }
            len++;
            db_donemsg = new char[len];
            char *e = lddb::stpcpy(db_donemsg, t);
            for (u_int i = 0; i < numNets(); i++) {
                dbNet *net = nlNet(i);
                if (net->flags & NET_CRITICAL) {
                    *e++ = ' ';
                    *e++ = ' ';
                    e = lddb::stpcpy(e, net->netname);
                    *e++ = '\n';
                }
            }
            *e = 0;
        }
    }
    else {
        bool uncritical = false;
        for (dbStringList *sl = netnames; sl; sl = sl->next) {
            if (!strcmp(sl->string, "-u")) {
                uncritical = true;
                if (sl->next && !strcasecmp(sl->next->string, "all")) {
                    for (u_int i = 0; i < numNets(); i++) {
                        dbNet *net = nlNet(i);
                        if (net->flags & NET_CRITICAL)
                            net->flags &= ~NET_CRITICAL;
                    }
                    dbStringList *cnets = criticalNetList();
                    dbStringList::destroy(cnets);
                    setCriticalNetList(0);
                    dbStringList::destroy(netnames);
                    db_donemsg = lddb::copy("Critical net list cleared.");
                    return (LD_OK);
                }
                continue;
            }
            dbNet *net = getNet(sl->string);
            if (!net) {
                char *t = write_msg("critical: no such net %s.", sl->string);
                if (!db_warnmsg)
                    db_warnmsg = t;
                else {
                    int len = strlen(db_warnmsg) + strlen(t) + 2;
                    char *tt = new char[len];
                    sprintf(tt, "%s\n%s", db_warnmsg, t);
                    delete [] t;
                    delete [] db_warnmsg;
                    db_warnmsg = tt;
                }
                continue;
            }
            if (uncritical) {
                net->flags &= ~NET_CRITICAL;
                dbStringList *cnets = criticalNetList();
                dbStringList *cp = 0;
                for (dbStringList *cl = cnets; cl; cl = cl->next) {
                    if (!strcmp(cl->string, sl->string)) {
                        if (cp)
                            cp->next = cl->next;
                        else
                            setCriticalNetList(cl->next);
                        delete [] cl->string;
                        delete cl;
                        break;
                    }
                    cp = cl;
                }
            }
            else {
                net->flags |= NET_CRITICAL;
                dbStringList *cnets = criticalNetList();
                bool found = false;
                dbStringList *cp = 0;
                for (dbStringList *cl = cnets; cl; cl = cl->next) {
                    if (!strcmp(cl->string, sl->string)) {
                        found = true;
                        // Move existing entry to front.
                        if (cp) {
                            cp->next = cl->next;
                            cl->next = cnets;
                            setCriticalNetList(cl);
                        }
                        break;
                    }
                    cp = cl;
                }
                if (!found) {
                    setCriticalNetList(
                        new dbStringList(lddb::copy(sl->string), cnets));
                }
            }
        }
        dbStringList::destroy(netnames);
    }
    return (LD_OK);
}


// Set or clear obstructions.
//   obstruction [-u] [layer] [all] [L B R T]
//
// If -u is given, any current obstruction matching the given values is
// removed.  Otherwise, the obstruction area is added.  If no arguments,
// all user obstructions are printed.
//
bool
cLDDB::cmdObstruction(const char *cmd)
{
    struct obs_t
    {
        obs_t()
            {
                lname = 0;
                L = 0;
                B = 0;
                R = 0;
                T = 0;
            }

        ~obs_t()
            {
                delete [] lname;
                delete [] L;
                delete [] B;
                delete [] R;
                delete [] T;
            }

        char *lname;
        char *L;
        char *B;
        char *R;
        char *T;
    };

    clearMsgs();
    obs_t obs;
    bool clear = false;
    bool all = false;
    char *tok;
    while ((tok = lddb::gettok(&cmd)) != 0) {
        if (!strcmp(tok, "-u")) {
            clear = true;
            delete [] tok;
            continue;
        }
        if (!strcasecmp(tok, "all")) {
            all = true;
            delete [] tok;
            continue;
        }
        if (!obs.lname && getLayer(tok) >= 0) {
            obs.lname = tok;
            continue;
        }
        if (!obs.L) {
            obs.L = tok;
            continue;
        }
        if (!obs.B) {
            obs.B = tok;
            continue;
        }
        if (!obs.R) {
            obs.R = tok;
            continue;
        }
        if (!obs.T) {
            obs.T = tok;
            continue;
        }

        db_errmsg = write_msg("unknown token %s, unrecognized layer?.", tok);
        delete [] tok;
        return (LD_BAD);
    }

    int lnum = -1;
    if (obs.lname)
        lnum = getLayer(obs.lname);

    lefu_t left = 0, bottom = 0, right = 0, top = 0;
    double dval;
    if (obs.L) {
        if (!obs.T) {
            db_errmsg = write_msg("too few tokens");
            return (LD_BAD);
        }
        if (sscanf(obs.L, "%lf", &dval) != 1) {
            db_errmsg = write_msg("non-numeric token %s.", obs.L);
            return (LD_BAD);
        }
        left = micToLefGrid(dval);
        if (sscanf(obs.B, "%lf", &dval) != 1) {
            db_errmsg = write_msg("non-numeric token %s.", obs.B);
            return (LD_BAD);
        }
        bottom = micToLefGrid(dval);
        if (sscanf(obs.R, "%lf", &dval) != 1) {
            db_errmsg = write_msg("non-numeric token %s.", obs.R);
            return (LD_BAD);
        }
        right = micToLefGrid(dval);
        if (sscanf(obs.T, "%lf", &dval) != 1) {
            db_errmsg = write_msg("non-numeric token %s.", obs.T);
            return (LD_BAD);
        }
        top = micToLefGrid(dval);
        if (left > right) { lefu_t t = left; left = right; right = t; }
        if (bottom > top) { lefu_t t = bottom; bottom = top; top = t; }
    }

    if (clear) {
        if (all) {
            // Clear all user obstructions.

            dbDseg *ds = userObs();
            dbDseg::destroy(ds);
            setUserObs(0);
            db_donemsg = lddb::copy("User obstruction list cleared.");
            return (LD_OK);
        }
        if (lnum >= 0) {
            if (obs.L) {
                dbDseg *ob = findObstruction(left, bottom, right, top, lnum);
                if (ob) {
                    dbDseg *sp = 0, *sn;
                    for (dbDseg *ds = userObs(); ds; ds = sn) {
                        sn = ds->next;
                        if (ds == ob) {
                            if (sp)
                                sp->next = sn;
                            else
                                setUserObs(sn);
                            delete ds;
                            break;
                        }
                        sp = ds;
                    }
                    db_donemsg = lddb::copy("Obstruction removed.");
                }
                else
                    db_donemsg = lddb::copy("Obstruction not found.");
            }
            else {
                // Clear all obstructions on given layer.

                dbDseg *sp = 0, *sn;
                for (dbDseg *ds = userObs(); ds; ds = sn) {
                    sn = ds->next;
                    if (ds->layer == lnum) {
                        if (sp)
                            sp->next = sn;
                        else
                            setUserObs(sn);
                        delete ds;
                        continue;
                    }
                    sp = ds;
                }
                db_donemsg = write_msg("Cleared all obstructions on layer %d.",
                    lnum+1);
            }
        }
        else
            db_donemsg = lddb::copy("No layer given, no obstructions removed.");
        return (LD_OK);
    }

    char buf[80];
    if (lnum < 0) {
        // Format a listing of all user obstructions.

        if (userObs()) {
            dbLstr lstr;
            for (dbDseg *ds = userObs(); ds; ds = ds->next) {
                sprintf(buf, "%-3d %.4f,%.4f  %.4f,%.4f\n", ds->layer,
                    lefToMic(ds->x1), lefToMic(ds->y1),
                    lefToMic(ds->x2), lefToMic(ds->y2));
                lstr.add(buf);
            }
            db_donemsg = lstr.string_trim();
        }
        else
            db_donemsg = lddb::copy("No obstructions in list.");
        return (LD_OK);
    }

    if (!obs.L) {
        // Format a list of obstructions on given layer.

        int cnt = 0;
        for (dbDseg *ds = userObs(); ds; ds = ds->next) {
            if (ds->layer == lnum)
                cnt++;
        }
        if (cnt) {
            dbLstr lstr;
            for (dbDseg *ds = userObs(); ds; ds = ds->next) {
                if (ds->layer != lnum)
                    continue;
                sprintf(buf, "%-3d %.4f,%.4f  %.4f,%.4f\n", ds->layer,
                    lefToMic(ds->x1), lefToMic(ds->y1),
                    lefToMic(ds->x2), lefToMic(ds->y2));
                lstr.add(buf);
            }
            db_donemsg = lstr.string_trim();
        }
        else
            db_donemsg = lddb::copy("No obstructions in list.");
        return (LD_OK);
    }

    dbDseg *ob = findObstruction(left, bottom, right, top, lnum);
    if (ob)
        db_donemsg = lddb::copy("Obstruction already in list.");
    else {
        addObstruction(left, bottom, right, top, lnum);
        db_donemsg = lddb::copy("Obstruction added.");
    }
    return (LD_OK);
}


// Set or query layer information.
//   layer N n[ame]     [name]
//   layer N l[ayernum] [GDSII layer]
//   layer N t[ypenum]  [GDSII datatype]
//   layer N w[idth]    [width]
//   layer N p[itch]    [pitch]
//   layer N d[irection] [h or v]
//
bool
cLDDB::cmdLayer(const char *cmd)
{
    clearMsgs();
    char *tok = lddb::gettok(&cmd);
    int lnum = -1;
    if (tok) {
        lnum = getLayer(tok);
        if (lnum < 0) {
            db_errmsg = write_msg("can't resolve layer %s.", tok);
            delete [] tok;
            return (LD_BAD);
        }
        delete [] tok;
        tok = lddb::gettok(&cmd);
    }

    char buf[80];
    if (lnum < 0 || !tok) {
        if (numLayers() == 0)
            db_donemsg = lddb::copy("No routing layers defined.");
        else {
            dbLstr lstr;
            for (u_int i = 0; i < numLayers(); i++) {
                if (lnum >= 0 && (int)i != lnum)
                    continue;
                lefRouteLayer *lefr = getLefRouteLayer(i);
                sprintf(buf, "layer %d\n", i+1);
                lstr.add(buf);
                sprintf(buf, "  %-10s: %s\n", "name", layerName(i));
                lstr.add(buf);
                sprintf(buf, "  %-10s: %d\n", "layernum", layerNumber(i));
                lstr.add(buf);
                sprintf(buf, "  %-10s: %d\n", "typenum", purposeNumber(i));
                lstr.add(buf);
                sprintf(buf, "  %-10s: %.4f\n", "width",
                    lefToMic(lefr->route.width));
                lstr.add(buf);
                if (lefr->route.pitchX == lefr->route.pitchY ||
                        lefr->route.pitchY == 0) {
                    sprintf(buf, "  %-10s: %.4f\n", "pitch",
                        lefToMic(lefr->route.pitchX));
                }
                else {
                    sprintf(buf, "  %-10s: %.4f %.4f\n", "pitch",
                        lefToMic(lefr->route.pitchX),
                        lefToMic(lefr->route.pitchY));
                }
                lstr.add(buf);
                sprintf(buf, "  %-10s: %s\n", "direction",
                    lefr->route.direction == DIR_VERT ? "vert" : "horiz");
                lstr.add(buf);
            }
            db_donemsg = lstr.string_trim();
        }
        return (LD_OK);
    }

    char c = isupper(*tok) ? tolower(*tok) : *tok;;
    if (c == 'n') {
        // name
        delete [] tok;
        tok = lddb::gettok(&cmd);
        if (!tok) {
            sprintf(buf, "layer %d name: %s", lnum+1, layerName(lnum));
            db_donemsg = lddb::copy(buf);
        }
        else {
            setLayerName(lnum, tok);
            delete [] tok;
        }
        return (LD_OK);
    }
    if (c == 'l') {
        // GDSII layer number
        delete [] tok;
        tok = lddb::gettok(&cmd);
        if (!tok) {
            sprintf(buf, "layer %d GDSII layernum: %d", lnum+1,
                layerNumber(lnum));
            db_donemsg = lddb::copy(buf);
        }
        else {
            u_int n;
            if (sscanf(tok, "%u", &n) != 1) {
                db_errmsg = write_msg("expecting integer value for %s.", tok);
                delete [] tok;
                return (LD_BAD);
            }
            setLayerNumber(lnum, n);
            delete [] tok;
        }
        return (LD_OK);
    }
    if (c == 't') {
        // GDSII datatype number
        delete [] tok;
        tok = lddb::gettok(&cmd);
        if (!tok) {
            sprintf(buf, "layer %d GDSII datatype: %d", lnum+1,
                purposeNumber(lnum));
            db_donemsg = lddb::copy(buf);
        }
        else {
            u_int n;
            if (sscanf(tok, "%u", &n) != 1) {
                db_errmsg = write_msg("expecting integer value for %s.", tok);
                delete [] tok;
                return (LD_BAD);
            }
            setPurposeNumber(lnum, n);
            delete [] tok;
        }
        return (LD_OK);
    }
    if (c == 'w') {
        // width
        lefRouteLayer *lefr = getLefRouteLayer(lnum);
        delete [] tok;
        tok = lddb::gettok(&cmd);
        if (!tok) {
            sprintf(buf, "layer %d path width: %g", lnum+1,
                lefToMic(lefr->route.width));
            db_donemsg = lddb::copy(buf);
        }
        else {
            double w;
            if (sscanf(tok, "%lf", &w) != 1) {
                db_errmsg = write_msg("expecting real number for %s.", tok);
                delete [] tok;
                return (LD_BAD);
            }
            lefr->route.width = micToLefGrid(w);
            delete [] tok;
        }
        return (LD_OK);
    }
    if (c == 'p') {
        // pitch
        lefRouteLayer *lefr = getLefRouteLayer(lnum);
        delete [] tok;
        tok = lddb::gettok(&cmd);
        if (!tok) {
            if (lefr->route.pitchX == lefr->route.pitchY ||
                    lefr->route.pitchY == 0) {
                sprintf(buf, "layer %d pitch: %.4f\n", lnum+1,
                    lefToMic(lefr->route.pitchX));
            }
            else {
                sprintf(buf, "layer %d pitch: %.4f %.4f", lnum+1,
                    lefToMic(lefr->route.pitchX),
                    lefToMic(lefr->route.pitchY));
            }
            db_donemsg = lddb::copy(buf);
        }
        else {
            double p;
            if (sscanf(tok, "%lf", &p) != 1) {
                db_errmsg = write_msg("expecting real number for %s.", tok);
                delete [] tok;
                return (LD_BAD);
            }
            lefr->route.pitchX = micToLefGrid(p);
            lefr->route.pitchY = lefr->route.pitchX;
            delete [] tok;
            tok = lddb::gettok(&cmd);
            if (tok) {
                if (sscanf(tok, "%lf", &p) != 1) {
                    db_errmsg = write_msg("expecting real number for %s.", tok);
                    delete [] tok;
                    return (LD_BAD);
                }
                lefr->route.pitchY = micToLefGrid(p);
                delete [] tok;
            }
        }
        return (LD_OK);
    }
    if (c == 'd') {
        // horiz or vert
        lefRouteLayer *lefr = getLefRouteLayer(lnum);
        delete [] tok;
        tok = lddb::gettok(&cmd);
        if (!tok) {
            sprintf(buf, "layer %d direction: %s", lnum+1,
                lefr->route.direction == DIR_VERT ? "vert" : "horiz");
            db_donemsg = lddb::copy(buf);
        }
        else {
            if (*tok == 'v' || *tok == 'V')
                lefr->route.direction = DIR_VERT;
            else
                lefr->route.direction = DIR_HORIZ;
            delete [] tok;
        }
        return (LD_OK);
    }

    db_errmsg = write_msg("Unknown keyword %s.", tok);
    delete [] tok;
    return (LD_BAD);
}


// Add a new routing layer.
//   newlayer name
//
bool
cLDDB::cmdNewLayer(const char *cmd)
{
    clearMsgs();
    char *name = lddb::gettok(&cmd);
    if (!name) {
        db_errmsg = write_msg("newlayer: no layer name given.");
        return (LD_BAD);
    }
    lefRouteLayer *lefl = new lefRouteLayer(lddb::copy(name));
    lefAddObject(lefl);
    return (LD_OK);
}


// Set the bounding rectangle of the routing area.
//   boundary L B R T
//
bool
cLDDB::cmdBoundary(const char *cmd)
{
    clearMsgs();
    char *tok1 = lddb::gettok(&cmd);
    char *tok2 = lddb::gettok(&cmd);
    char *tok3 = lddb::gettok(&cmd);
    char *tok4 = lddb::gettok(&cmd);

    if (!tok1) {
        char buf[80];
        sprintf(buf, "boundary:  %.4f,%.4f  %.4f,%.4f",
            lefToMic(xLower()), lefToMic(yLower()),
            lefToMic(xUpper()), lefToMic(yUpper()));
        db_donemsg = lddb::copy(buf);
        return (LD_OK);
    }
    if (tok4) {
        double d1, d2, d3, d4;
        if (sscanf(tok1, "%lf", &d1) != 1) {
            db_errmsg = write_msg("non-numeric token %s.", tok1);
            goto bummer;
        }
        if (sscanf(tok2, "%lf", &d2) != 1) {
            db_errmsg = write_msg("non-numeric token %s.", tok2);
            goto bummer;
        }
        if (sscanf(tok3, "%lf", &d3) != 1) {
            db_errmsg = write_msg("non-numeric token %s.", tok3);
            goto bummer;
        }
        if (sscanf(tok4, "%lf", &d4) != 1) {
            db_errmsg = write_msg("non-numeric token %s.", tok4);
            goto bummer;
        }
        if (d1 > d3) { double t = d1; d1 = d3; d3 = t; }
        if (d2 > d4) { double t = d2; d2 = d4; d4 = t; }
        setXlower(micToLefGrid(d1));
        setYlower(micToLefGrid(d2));
        setXupper(micToLefGrid(d3));
        setYupper(micToLefGrid(d4));
        return (LD_OK);
    }

    db_errmsg = write_msg("wrong argument count.");

bummer:
    delete [] tok1;
    delete [] tok2;
    delete [] tok3;
    delete [] tok4;
    return (LD_BAD);
}


bool
cLDDB::cmdReadLef(const char *fname)
{
    clearMsgs();
    if (lefRead(fname, false) == LD_OK)
        return (LD_OK);
    db_errmsg = lddb::copy("Read LEF failed.");
    return (LD_BAD);
}


bool
cLDDB::cmdReadDef(const char *fname)
{
    clearMsgs();
    if (defRead(fname) == LD_OK)
        return (LD_OK);
    db_errmsg = lddb::copy("Read DEF failed.");
    return (LD_BAD);
}


bool
cLDDB::cmdWriteLef(const char *fname)
{
    clearMsgs();
    if (lefWrite(fname, LEF_OUT_ALL) == LD_OK)
        return (LD_OK);
    db_errmsg = lddb::copy("Write LEF failed.");
    return (LD_BAD);
}


bool
cLDDB::cmdWriteDef(const char *fname)
{
    clearMsgs();
    if (defWrite(fname) == LD_OK)
        return (LD_OK);
    db_errmsg = lddb::copy("Write DEF failed.");
    return (LD_BAD);
}


bool
cLDDB::cmdUpdateDef(const char *fname, const char *newfn)
{
    clearMsgs();
    if (writeDefRoutes(fname, newfn) == LD_OK)
        return (LD_OK);
    db_errmsg = lddb::copy("Update DEF failed.");
    return (LD_BAD);
}


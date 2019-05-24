
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
Authors: 1985 Wayne A. Christopher
         1992 Stephen R. Whiteley
****************************************************************************/

#include "spglobal.h"
#include <errno.h>
#include "simulator.h"
#include "runop.h"
#include "circuit.h"
#include "graph.h"
#include "output.h"
#include "cshell.h"
#include "kwords_fte.h"
#include "kwords_analysis.h"
#include "commands.h"
#include "input.h"
#include "toolbar.h"
#include "parser.h"
#include "subexpand.h"
#include "verilog.h"
#include "spnumber/paramsub.h"
#include "miscutil/lstring.h"
#include "miscutil/filestat.h"
#include "miscutil/tvals.h"
#ifdef WIN32
#include <winsock2.h>
#else
#include <sys/socket.h>
#endif

// Core-leak testing.
//#define SOURCE_DEBUG

// Time profiling
//#define TIME_DEBUG

#ifdef SOURCE_DEBUG
#include "../../malloc/local_malloc.h"
#endif


//
// Functions for sourcing of input.
//


// A table of tables of library tag offsets.
//
struct sLibMap
{
    sLibMap()
        {
            lib_tab = 0;
        }

    ~sLibMap();

    long find(const char*, const char*);

private:
    sHtab *map_lib(FILE*);

    sHtab *lib_tab;
};

#define LM_NO_FILE -1
#define LM_NO_NAME -2

namespace { sLibMap *LibMap; }


namespace {
    // Return true if the file looks like valid data.  Check for
    // binary, and csdf/rawfile which happens when the user forgets
    // "load".
    //
    bool check_file(FILE *fp)
    {
        char *s, buf[256];
        int csdf = 0;
        int raw = 0;
        int line = 0;
        int nchk = 0;
        for (;;) {
            if (csdf >= 10 || raw >= 10)
                return (false);
            line++;
            int nlo = 0, nhi = 0, nc = 0;
            int c;
            while ((c = getc(fp)) != EOF) {
                if (c == '\n') {
                    if (nc <= 255)
                        buf[nc] = 0;
                    break;
                }
                if (c < 0x8 || (c < 0x20 && c > 0xa))
                    nlo++;
                if (c > 0x7e)
                    nhi++;
                if (nc < 255)
                    buf[nc] = c;
                nc++;
            }
            buf[255] = 0;
            if (c == EOF) {
                if (line == 1)
                    return (true);
                break;
            }

            s = buf;
            while (isspace(*s))
                s++;

            if (*s == '*')
                continue;

            if (nlo > 2)
                return (false);
            if (nc > 32 && nlo + nhi > nc/4)
                return (false);

            nchk++;
            if (nchk > 10)
                break;
            if (lstring::cieq("#H", s)) {
                csdf += 5;
                continue;
            }
            if (lstring::ciprefix("source", s)) {
                csdf += 1;
                continue;
            }
            if (lstring::ciprefix("title", s)) {
                csdf += 1;
                raw += 1;
                continue;
            }
            if (lstring::ciprefix("serialno", s)) {
                csdf += 2;
                continue;
            }
            if (lstring::ciprefix("time", s)) {
                csdf += 1;
                continue;
            }
            if (lstring::ciprefix("analysis", s)) {
                csdf += 1;
                continue;
            }
            if (lstring::ciprefix("sweepmode", s)) {
                csdf += 2;
                continue;
            }
            if (lstring::ciprefix("xbegin", s)) {
                csdf += 2;
                continue;
            }
            if (lstring::ciprefix("date", s)) {
                raw += 1;
                continue;
            }
            if (lstring::ciprefix("plotname", s)) {
                raw += 2;
                continue;
            }
            if (lstring::ciprefix("flags", s)) {
                raw += 1;
                continue;
            }
            if (lstring::ciprefix("no. variables", s)) {
                raw += 4;
                continue;
            }
            if (lstring::ciprefix("no. points", s)) {
                raw += 4;
                continue;
            }
            if (lstring::ciprefix("command", s)) {
                raw += 1;
                continue;
            }
        }
        rewind(fp);
        return (true);
    }
}


// Static function.
// Convert a wordlist of file names to a list of files, each returned
// file contains a single circuit.  The will split input files
// according to ".newjob" cards.
//
// The returned file list may contain regular and temporary files, the
// temporary files have the lineval flag set to 1.  If there is an
// error, the return is 0.
//
file_elt *
file_elt::wl_to_fl(const wordlist *wl, wordlist **tmpfiles)
{
    if (tmpfiles)
        *tmpfiles = 0;
    if (!wl)
        return (0);

    // Create a list of the files, and line numbers/offsets for
    // .newjob lines.
    //
    file_elt *f0 = 0, *fe = 0;
    for (const wordlist *ww = wl; ww; ww = ww->wl_next) {
        bool isfifo;
        FILE *fp = Sp.PathOpen(ww->wl_word, "r", &isfifo);
        if (!fp) {
            GRpkgIf()->Perror(ww->wl_word);
            file_elt::destroy(f0);
            return (0);
        }
        char *tempfile = 0;
        if (isfifo) {
            // Have to use temp file.
            tempfile = filestat::make_temp("sp");
            FILE *outfp = Sp.PathOpen(tempfile, "w");
            if (!outfp) {
                GRpkgIf()->Perror(tempfile);
                delete [] tempfile;
                file_elt::destroy(f0);
                return (0);
            }
            if (tmpfiles) {
                wordlist *wt = new wordlist(tempfile, 0);
                wt->wl_next = *tmpfiles;
                if (wt->wl_next)
                    wt->wl_next->wl_prev = wt;
                *tmpfiles = wt;
            }
            int c;
            while ((c = getc(fp)) != EOF)
                putc(c, outfp);
            fclose(fp);
            fclose(outfp);
            fp = Sp.PathOpen(tempfile, "r");
            if (!fp) {
                GRpkgIf()->Perror(tempfile);
                delete [] tempfile;
                file_elt::destroy(f0);
                return (0);
            }
        }
        if (!check_file(fp)) {
            GRpkgIf()->ErrPrintf(ET_ERROR, "file %s is not valid input.\n",
                ww->wl_word);
            delete [] tempfile;
            fclose(fp);
            file_elt::destroy(f0);
            return (0);
        }
        const char *fname = tempfile ? tempfile : wl->wl_word;

        // The -1 lineval is a placeholder for new file start.
        file_elt *newfile = new file_elt(fname, -1, 0);
        if (!f0)
            f0 = fe = newfile;
        else {
            fe->fe_next = newfile;
            fe = fe->fe_next;
        }
        char *s, buf[BSIZE_SP];
        int lcnt = 0;
        int offs = 0;
        while ((s = fgets(buf, BSIZE_SP, fp)) != 0) {
            while (isspace(*s))
                s++;
            if (lstring::cimatch(NEWJOB_KW, s)) {
                if (lcnt == 0)
                    newfile->fe_lineval = 0;
                else if (!f0)
                    f0 = fe = new file_elt(fname, lcnt, offs);
                else {
                    fe->fe_next = new file_elt(fname, lcnt, offs);
                    fe = fe->fe_next;
                }
            }
            offs = ftell(fp);
            lcnt++;
        }
        delete [] tempfile;
        fclose(fp);
    }

    // Now split/combine files into temp files as necessary.  The file
    // list returned is an ordered list of files to read, each element
    // is an independent "source".  The lineval element is true if the
    // file is temporary.

    char buf[BSIZE_SP];
    file_elt *f1 = 0;
    fe = 0;
    file_elt *fstart = f0;
    while (fstart) {
        file_elt *fend = fstart->get_end();
        if (fstart->fe_lineval <= 0 && (!fstart->fe_next ||
                (fend == fstart->fe_next && fend->fe_lineval == 0))) {
            // The whole file is read, no temp file needed.
            if (!f1)
                f1 = fe = new file_elt(fstart->fe_filename, 0, 0);
            else {
                fe->fe_next = new file_elt(fstart->fe_filename, 0, 0);
                fe = fe->fe_next;
            }
            fstart = fend;
            continue;
        }

        // Have to use temp file.
        char *tempfile = filestat::make_temp("sp");
        FILE *outfp = Sp.PathOpen(tempfile, "w");
        if (!outfp) {
            GRpkgIf()->Perror(tempfile);
            file_elt::destroy(f0);
            file_elt::destroy(f1);
            return (0);
        }
        if (tmpfiles) {
            wordlist *wt = new wordlist(tempfile, 0);
            wt->wl_next = *tmpfiles;
            if (wt->wl_next)
                wt->wl_next->wl_prev = wt;
            *tmpfiles = wt;
        }

        // Open (first) source file, set offset.
        FILE *infp = Sp.PathOpen(fstart->fe_filename, "r");
        if (!infp) {
            GRpkgIf()->Perror(fstart->fe_filename);
            file_elt::destroy(f0);
            file_elt::destroy(f1);
            fclose(outfp);
            return (0);
        }
        if (fstart->fe_offset)
            fseek(infp, fstart->fe_offset, SEEK_SET);

        file_elt *fp = fstart;
        file_elt *fx = fstart->fe_next;
        while (fx) {
            if (fx->fe_lineval > 0) {
                int nchars = fx->fe_offset - fp->fe_offset;
                while (nchars--) {
                    int c = getc(infp);
                    putc(c, outfp);
                }
                fclose(infp);
                infp = 0;
                break;
            }

            // Copy to end of current file, close infp.
            int i;
            while ((i = fread(buf, 1, BSIZE_SP, infp)) > 0)
                fwrite(buf, 1, i, outfp);
            fclose(infp);
            infp = 0;

            if (fx->fe_lineval == 0) {
                // Start of new file, done with this temp file.
                break;
            }
            // Reopen infp.
            infp = Sp.PathOpen(fx->fe_filename, "r");
            if (!infp) {
                GRpkgIf()->Perror(fx->fe_filename);
                file_elt::destroy(f0);
                file_elt::destroy(f1);
                fclose(outfp);
                return (0);
            }
            if (fx->fe_offset)
                fseek(infp, fx->fe_offset, SEEK_SET);
            fp = fx;
            fx = fx->fe_next;
        }
        if (infp) {
            int i;
            while ((i = fread(buf, 1, BSIZE_SP, infp)) > 0)
                fwrite(buf, 1, i, outfp);
            fclose(infp);
            infp = 0;
        }
        fclose(outfp);
        outfp = 0;
        if (!f1)
            f1 = fe = new file_elt(tempfile, 0, 0);
        else {
            fe->fe_next = new file_elt(tempfile, 0, 0);
            fe = fe->fe_next;
        }
        delete [] tempfile;
        tempfile = 0;
        fstart = fx;
    }

    file_elt::destroy(f0);
    return (f1);
}
// End of file_elt functions.


void
CommandTab::com_source(wordlist *wl)
{
    bool nospice = false, nocmds = false, reuse = false;
    wl = wordlist::copy(wl);
    wl->wl_prev = 0;
    wordlist *ww = wl, *wn;
    for ( ; wl; wl = wn) {
        wn = wl->wl_next;
        if (*wl->wl_word != '-')
            continue;
        if (!wl->wl_word[1]) {
            GRpkgIf()->ErrPrintf(ET_ERROR, "syntax, missing options.\n");
            wordlist::destroy(ww);
            return;
        }
        for (char *s = wl->wl_word + 1; *s; s++) {
            if (*s == 'n')
                nospice = true;
            else if (*s == 'c')
                nocmds = true;
            else if (*s == 'r')
                reuse = true;
            else {
                GRpkgIf()->ErrPrintf(ET_ERROR,
                    "unknown option character %c.\n", *s);
                wordlist::destroy(ww);
                return;
            }
        }
        if (wl->wl_prev)
            wl->wl_prev->wl_next = wl->wl_next;
        if (wl->wl_next)
            wl->wl_next->wl_prev = wl->wl_prev;
        delete [] wl->wl_word;
        if (ww == wl)
            ww = wl->wl_next;
        delete wl;
    }
    wl = ww;

    if (!wl) {
        GRpkgIf()->ErrPrintf(ET_ERROR, "no files given!\n");
        return;
    }

    wordlist *tmpfiles;
    file_elt *f0 = file_elt::wl_to_fl(wl, &tmpfiles);
    wordlist::destroy(wl);

    if (f0) {
#ifdef SOURCE_DEBUG
        if (Memory()->mon_start(10))
            printf("mmon started\n");
#endif
        Sp.Source(f0, nospice, nocmds, reuse);
        file_elt::destroy(f0);
#ifdef SOURCE_DEBUG
        printf("Allocation table contains %d entries.\n", Memory()->mon_count());

        delete Sp.CurCircuit();
        Sp.VecGc();

        if (Memory()->mon_stop() && Memory()->mon_dump((char*)"mon.out"))
            printf(
                "Allocation monitor stopped, data in file \"mon.out\".\n");
#endif
    }
    for (wordlist *w = tmpfiles; w; w = w->wl_next)
        unlink(w->wl_word);
    wordlist::destroy(tmpfiles);
}
// End of CommandTab functions.


bool
IFsimulator::Block(const char *s)
{
    FILE *fp = PathOpen(s, "r");
    if (fp) {
        fclose(fp);
        wordlist tl1, tl2;
        tl1.wl_word = (char*)s;
        tl1.wl_next = &tl2;
        tl2.wl_word = (char*)"-n";
        tl2.wl_prev = &tl1;
        CommandTab::com_source(&tl1);
        return (true);
    }
    return (false);
}


void
IFsimulator::Source(file_elt *f0, bool nospice, bool nocmds, bool reuse)
{
    if (!f0)
        return;
    if (f0->next())
        reuse = false;
    for (file_elt *f = f0; f; f = f->next()) {

        FILE *fp = PathOpen(f->filename(), "r");
        if (!fp) {
            GRpkgIf()->Perror(f->filename());
            return;
        }
        bool inter = CP.GetFlag(CP_INTERACTIVE);
        CP.SetFlag(CP_INTERACTIVE, false);
        if (reuse && !nospice)
            delete ft_curckt;

        char buf[BSIZE_SP];
        bool sourced = false;
        // Don't print the title if this is an init file.
        SYS_STARTUP(buf);
        if (lstring::eq(buf, lstring::strip_path(f->filename()))) {
            SpSource(fp, true, true, f->filename());
            sourced = true;
        }
        if (!sourced) {
            USR_STARTUP(buf);
            if (lstring::eq(buf, lstring::strip_path(f->filename()))) {
                SpSource(fp, true, true, f->filename());
                sourced = true;
            }
        }
        if (!sourced) {
            SpSource(fp, nospice, nocmds, f->filename());
            sourced = true;
        }
        CP.SetFlag(CP_INTERACTIVE, inter);
        fclose(fp);
    }
}


// Source the named file.  This is primarily used in the callback
// for the source button in the editor.
//
void
IFsimulator::EditSource(const char *filename, bool permfile, bool reuse)
{
    bool inter = CP.GetFlag(CP_INTERACTIVE);
    CP.SetFlag(CP_INTERACTIVE, false);
    FILE *fp = PathOpen(filename, "r");
    if (!fp)
        GRpkgIf()->Perror(filename);
    else {
        if (reuse)
            delete ft_curckt;
        SpSource(fp, false, false, permfile ? filename : 0);
        fclose(fp);

        // Check for other loaded circuits from the same file.  If found, 
        // free and null the filename entry, as the file has been edited
        // and therefore does not correspond to the saved circuit struct.
        //
        if (permfile) {
            for (sFtCirc *old = ft_circuits; old; old = old->next()) {
                if (old != ft_curckt && old->filename() &&
                        lstring::eq(old->filename(), filename))
                    old->clear_filename();
            }
        }
    }
    CP.SetFlag(CP_INTERACTIVE, inter);
}


// This checks the first line of input for special directives, then creates
// a deck and passes it to DeckSource().  For a startup script, nospice and
// nocmds should be true.  For ordinary spice, both args should be false.
// If fp is null, the text will be read from the message socket.
//
bool
IFsimulator::SpSource(FILE *fp, bool nospice, bool nocmds,
    const char *filename)
{
    bool err;
    char *title = 0;
    for (;;) {
        title = ReadLine(fp, &err);
        if (err)
            return (false);
        if (!title)
            return (true);
        if (lstring::cimatch(NEWJOB_KW, title)) {
            delete [] title;
            title = 0;
            continue;
        }
        break;
    }

    // Check for Xic stuff, skip if found.
    if (lstring::prefix(GFX_PREFIX, title)) {
        // Input contains symbolic layout information.
        delete [] title;
        while ((title = ReadLine(fp, &err)) != 0) {
            // look for "Generated by" or "* Generated by"
            char *t = title;
            if (t[0] == '*' && t[1] == ' ')
                t += 2;
            if (lstring::prefix(GFX_TITLE, t))
                break;
            delete [] title;
        }
        if (err)
            return (false);
        if (!title)
            return (true);
    }

    // Check if binary.
    for (char *t = title; *t; t++) {
        if (*t & 0x80) {
            delete [] title;
            GRpkgIf()->ErrPrintf(ET_ERROR,
                "source is not valid input - binary?\n");
            return (true);
        }
    }

    sLine *deck = 0;

    // Check if user forgot the title line.
    char *s = title;
    while (isspace(*s))
        s++;
    if (*s == '.') {
        if (lstring::cimatch(CONT_KW, s) ||
                lstring::cimatch(EXEC_KW, s) ||
                lstring::cimatch(INCL_L_KW, s) ||
                lstring::cimatch(INCL_S_KW, s) ||
                lstring::cimatch(SPINCL_KW, s) ||
                lstring::cimatch(LIB_KW, s) ||
                lstring::cimatch(SPLIB_KW, s) ||
                lstring::cimatch(CHECK_KW, s) ||
                lstring::cimatch(CHECKALL_KW, s) ||
                lstring::cimatch(MONTE_KW, s) ||
                lstring::cimatch(NOEXEC_KW, s) ||
                lstring::cimatch(PARAM_KW, s) ||
                lstring::cimatch(CACHE_KW, s) ||
                lstring::cimatch(TITLE_KW, s)) {
            deck = new sLine;
            deck->set_line("");
        }
    }
    else if (title[0] == '#' && title[1] == '!') {
        // If passed a title line of the form "#! path/wrspice" then this
        // is a "shell" script, so strip the title.
        const char *ss = strrchr(title, '/');
        if (ss)
            ss++;
        else {
            ss = title + 2;
            while (isspace(*ss))
                ss++;
        }
        if (!strncmp(s, CP.Program(), strlen(CP.Program())))
            title = 0;
    }

    // The parameters are read here for processing .if, etc.  Functions
    // will be defined as a by-product.  These will be thrown away.  The
    // "real" parse occurs later.
    //
#ifdef TIME_DEBUG
    double tstart = OP.seconds();
#endif
    PushUserFuncs(0);
    sParamTab *tab = new sParamTab();
    LibMap = new sLibMap;
    if (deck)
        deck->set_next(ReadDeck(fp, title, &err, &tab, filename));
    else
        deck = ReadDeck(fp, title, &err, &tab, filename);
    delete LibMap;
    LibMap = 0;
    delete tab;
    cUdf *tmpudf = PopUserFuncs();
    delete tmpudf;
#ifdef TIME_DEBUG
    double tend = OP.seconds();
    printf("file read: %g\n", tend - tstart);
#endif

    delete [] title;
    if (err)
        return (false);
    ContinueLines(deck);
    DeckSource(deck, nospice, nocmds, filename, (fp == 0));
    return (true);
}


// Source a spice deck.  If nospice is true, execute the commands in
// the deck without saving the spice text, if any.  If nocmds is true,
// save the spice text but discard the commands.  If both flags are
// true, then execute all commands up until a .endc line, the deck is
// assumed to come from an init file.  If neither flag is true, source
// the spice text and execute the commands.  The exec block is
// executed before the source, the controls after the source. 
// Execution of the controls is skipped if get_controls() finds a
// margin analysis specifier, the execs are always executed, but
// created vectors are not saved if noexec is true.
//
// The .if/etc. lines have already been processed!
//
void
IFsimulator::DeckSource(sLine *deck, bool nospice, bool nocmds,
    const char *filename, bool quiet)
{
    if (!deck)
        return;

#ifdef TIME_DEBUG
    double tstart = OP.seconds();
#endif
    // Expand "\n" and "\t" in title line, thus the title can print
    // as multiple lines.  It counts as a single line for numbering,
    // though.
    //
    char *title = new char[strlen(deck->line()) + 1];
    char *t = title;
    const char *s = deck->line();
    while (*s) {
        if (*s == '\\') {
            if (s[1] == 'n' || s[1] == 'N') {
                *t++ = '\n';
                s += 2;
                continue;
            }
            if (s[1] == 't' || s[1] == 'T') {
                *t++ = '\t';
                s += 2;
                continue;
            }
        }
        *t++ = *s++;
    }
    *t = 0;
    if (!lstring::eq(title, deck->line()))
        deck->set_line(title);
    delete [] title;

    // grab any Verilog block
    sLine *vblk = deck->extract_verilog();

    // "old" margin analysis format keywords
    bool check = false;
    bool checkall = false;
    bool monte = false;
    bool noexec = false;
    if (lstring::cimatch(CHECK_KW, deck->line()))
        check = true;
    else if (lstring::cimatch(CHECKALL_KW, deck->line()))
        checkall = true;
    else if (lstring::cimatch(MONTE_KW, deck->line()))
        monte = true;
    else if (lstring::cimatch(NOEXEC_KW, deck->line()))
        noexec = true;
    if (check || checkall || monte)
        noexec = true;
    
    // If noexec is true here, old format is indicated.  The first line
    // is .whatever, and will be cut below.

    int kwfound;
    deck->get_keywords(&kwfound);

    wordlist *execs = 0;
    wordlist *controls = 0;
    wordlist *postrun = 0;

    if (noexec || (nospice && nocmds) || (kwfound & LI_EXEC_FOUND))
        execs = deck->get_controls(nospice && nocmds, CBLK_EXEC, noexec);
    if (kwfound & LI_CONT_FOUND)
        controls = deck->get_controls(nospice && nocmds, CBLK_CTRL);
    if (kwfound & LI_POST_FOUND)
        postrun = deck->get_controls(nospice && nocmds, CBLK_POST);

    if (!noexec) {
        check = ((kwfound & LI_CHECK_FOUND) ? true : false);
        checkall = ((kwfound & LI_CHECKALL_FOUND) ? true : false);
        monte = ((kwfound & LI_MONTE_FOUND) ? true : false);
        noexec = ((kwfound & LI_NOEXEC_FOUND) ? true : false);
        if (check || checkall || monte)
            noexec = true;
    }
    else {
        // old format, rid first line
        sLine *ld = deck;
        deck = deck->next();
        delete ld;
    }

    // Display circuit name
    TTY.init_more();
    if (!GetVar(kw_noprtitle, VTYP_BOOL, 0)) {
        if (!nospice && deck->is_ckt()) {
            if (!CP.GetFlag(CP_MESSAGE) && !quiet &&
                    !GetFlag(FT_SERVERMODE) && deck->line()) {
                if (*deck->line())
                    TTY.printf("\nCircuit: %s\n\n", deck->line());
                else
                    TTY.printf("\nCircuit: <untitled>\n\n");
            }
        }
    }
#ifdef TIME_DEBUG
    double tend = OP.seconds();
    printf("init 1: %g\n", tend - tstart);
#endif

    // Source the deck
    Tvals tv;
    tv.start();
    bool bad_deck = false;
    sFtCirc *ct = SpDeck(deck, filename, execs, wordlist::copy(controls), vblk,
        nospice, false, &bad_deck);
    tv.stop();
    if (ft_flags[FT_SIMDB])
        GRpkgIf()->ErrPrintf(ET_MSGS, "Total time to source input: %g.\n",
            tv.realTime());
    if (!bad_deck && ct) {
        if (monte)
            ct->set_runtype(MONTE_GIVEN);
        else if (checkall)
            ct->set_runtype(CHECKALL_GIVEN);
    }
    if (bad_deck) {
        wordlist::destroy(controls);
        wordlist::destroy(postrun);
        return;
    }

    if (postrun) {
        // Parameter expand and apply the post-run block.
        for (wordlist *wl = postrun; wl; wl = wl->wl_next)
            ct->params()->param_subst_all(&wl->wl_word);
        ct->set_postrun(postrun);
    }

    // Now do the commands
    if (controls && !noexec) {
        if (ct && ct->params()) {
            // Parameter expand the .control lines.
            for (wordlist *wl = controls; wl; wl = wl->wl_next)
                ct->params()->param_subst_all(&wl->wl_word);
        }
        if (!nocmds || nospice)
            ExecCmds(controls);
    }
    wordlist::destroy(controls);

    if (GetFlag(FT_BATCHMODE) && (check || checkall || monte))
        // do margin analysis if in batch mode
        MargAnalysis(0);
}


// Low level operations for input deck processing.  This takes care of
// options, shell variable expansion, etc.
//
sFtCirc *
IFsimulator::SpDeck(sLine *deck, const char *filename, wordlist *execs,
    wordlist *controls, sLine *verilog, bool nospice, bool noexec, bool *err)
{
    if (err)
        *err = false;

#ifdef TIME_DEBUG
    double tstart = OP.seconds();
#endif
    sLine *realdeck = sLine::copy(deck);

    sFtCirc *oldckt = ft_curckt;

    // If ft_curckt has a null ci_descr field, it has been cleared for
    // reuse.  Otherwise, create a new circuit for use when evaluating
    // execs and for UDFs, and for things like "random" in the
    // options.  SetCircuit (called later) will update the UI after
    // options have been parsed.  If there is no circuit parsed, this
    // will be silently reverted.
    //
    if (!ft_curckt || ft_curckt->descr())
        ft_curckt = new sFtCirc();

    // Add the user-defined function context.
    ft_curckt->setDefines(new cUdf);

    // This is probably redundant with the call in ReadDeck.  As there,
    // the settings will not reflect values defined in the .options
    // lines.
    SPcx.kwMatchInit();

    // The conditionals have already been processed and removed.  This
    // builds a param table from the existing lines.
    sParamTab *ptab = new sParamTab;
    ptab = deck->process_conditionals(ptab);
    ptab->define_macros(true);

    // If execs, run them now, after parameter expanding.
    sPlot *pl_ex = 0;
    char tpname[64];
    if (execs && !noexec) {
        wordlist *wx = wordlist::copy(execs);
        if (ptab) {
            // Parameter expand the .exec lines.
            for (wordlist *wl = wx; wl; wl = wl->wl_next)
                ptab->param_subst_all(&wl->wl_word);
        }
        strcpy(tpname, OP.curPlot()->type_name());
        // Run the execs (before source).
        // Set up a temporary plot for vectors defined in the exec
        // block that might be needed in the circuit.
        pl_ex = new sPlot("exec");
        pl_ex->new_plot();
        OP.setCurPlot(pl_ex);
        pl_ex->set_circuit(ft_curckt->name());
        pl_ex->set_title(ft_curckt->name());
        ExecCmds(wx);
        wordlist::destroy(wx);

        // Still in list? may have been destroyed.
        sPlot *px = OP.plotList();
        for ( ; px; px = px->next_plot()) {
            if (px == pl_ex)
                break;
        }
        if (!px)
            pl_ex = 0;
    }
#ifdef TIME_DEBUG
    double tend = OP.seconds();
    printf("init 2: %g\n", tend - tstart);
#endif

    sFtCirc *ct = ft_curckt;
    if (nospice || !deck->is_ckt()) {
        if (ft_curckt == oldckt) {
            delete ft_curckt;
            oldckt = 0;
            ct = 0;
        }
        else {
            delete ft_curckt;
            ft_curckt = oldckt;
            ct = 0;
        }

        sLine::destroy(realdeck);
        delete ptab;
        wordlist::destroy(execs);
        wordlist::destroy(controls);
        sLine::destroy(verilog);
    }
    else {
        ft_curckt->setup(deck, filename, execs, controls, verilog, ptab);

        if (!ft_curckt->expand(realdeck, err)) {
            if (ft_curckt == oldckt) {
                delete ft_curckt;
                // Destructor will reset ft_curckt to something ok.
                oldckt = 0;
            }
            else {
                delete ft_curckt;
                ft_curckt = oldckt;
            }
            ct = 0;
        }
    }

    // 4.2.5, retain if any vectors.
    if (pl_ex && !pl_ex->num_perm_vecs()) {
        // Clean up special plot created for execs.
        ToolBar()->SuppressUpdate(true);
        pl_ex->destroy();
        ToolBar()->SuppressUpdate(false);
        // current plot might be deleted in the execs
        for (sPlot *pl = OP.plotList(); pl; pl = pl->next_plot()) {
            if (IFoutput::plotPrefix(tpname, pl->type_name())) {
                OP.setCurPlot(pl);
                break;
            }
        }
    }
    return (ct);
}


// Execute the commands given, used to execute the .control/.exec blocks
//
void
IFsimulator::ExecCmds(wordlist *wl)
{
    if (wl) {
        bool intr = CP.GetFlag(CP_INTERACTIVE);
        CP.SetFlag(CP_INTERACTIVE, false);
        OP.pushPlot();
        TTY.ioPush();
        CP.PushControl();
        CP.SetReturnVal(0.0);
        for ( ; wl; wl = wl->wl_next)
            CP.EvLoop(wl->wl_word);
        CP.PopControl();
        TTY.ioPop();
        OP.popPlot();
        CP.SetFlag(CP_INTERACTIVE, intr);
    }
}


// Return a line of input text of arbitrary length.  If the file
// pointer is null, read from the message socket.
//
char *
IFsimulator::ReadLine(FILE *fp, bool *err)
{
    *err = false;
    sLstr lstr;
    if (!fp) {
        if (CP.MesgSocket() >= 0) {
            for (;;) {
                char c;
                int i = recv(CP.MesgSocket(), &c, 1, 0);
                if (i <= 0) {
                    // EOF or error
                    if (i < 0) {
                        if (errno == EINTR)
                            continue;
#ifdef WIN32
                        GRpkgIf()->ErrPrintf(ET_ERROR, "recv: WSA code %d.",
                            WSAGetLastError());
#else
                        GRpkgIf()->Perror("recv");
#endif
                    }
                    *err = true;
                    return (0);
                }
                if (c == '\n' || c == '\0') {
                    if (c)
                        lstr.add_c(c);
                    else if (!lstr.length())
                        // empty line
                        return (lstring::copy(""));
                    break;
                }
                else
                    lstr.add_c(c);
            }
        }
    }
    else {
        int c;
        while ((c = getc(fp)) != EOF) {
            if (c == '\r')
                continue;  // file may come from DOS
            lstr.add_c(c);
            if (c == '\n')
                break;
        }
    }
    if (!lstr.length())
        return (0);

    bool force = GetVar(kw_dollarcmt, VTYP_BOOL, 0);

    // The dollarcmt variable is for HSPICE compatibility.  Look for
    // '$' or ';' characters.  These may indicate comment text.

    char *line = lstr.string_trim();
    char *l = line;
    while (isspace(*l))
        l++;

    if (*l == '$' || *l ==';') {
        if (force || isspace(*(l+1)))
            *l = '*';
    }
    else if (*l) {
        bool insq = false, indq = false;
        for (char *s = l+1; *s; s++) {
            if (*s == '\'' && s[-1] != '\\') {
                insq = !insq;
                continue;
            }
            if (*s == '"' && s[-1] != '\\') {
                indq = !indq;
                continue;
            }
            if (insq || indq)
                continue;

            if ((*s == '$' || *s == ';') && s[-1] != '\\' &&
                    (isspace(s[-1]) || force)) {
                if (isspace(s[1]) || force) {
                    s[0] = '\n';
                    s[1] = 0;
                    break;
                }
            }
        }
    }
    return (line);
}


namespace {
    // Return separated path and file. The passed *path may be deleted.
    //
    void
    parse_path(char **path, char **file)
    {
        char *p = *path;
        char *f = lstring::strrdirsep(p);
        if (!f) {
            // no directory
            *file = p;
            *path = 0;
            return;
        }
        if (f != lstring::strdirsep(p)) {
            // two or more dir separators
            *f++ = 0;
            *file = lstring::copy(f);
            return;
        }
        *f++ = 0;
        *file = lstring::copy(f);
        if (*p == 0) {
            // /file
            delete [] p;
            *path = lstring::copy("/");
            return;
        }
        if (strlen(p) == 2 && isalpha(*p) && p[1] == ':') {
            //  x:/file (Windows)
            char *t = new char[4];
            t[0] = p[0];
            t[1] = p[1];
            t[2] = '/';
            t[3] = 0;
            delete [] p;
            *path = t;
            return;
        }
        // xxx/file
        *path = lstring::copy(p);
        delete [] p;
    }
}


namespace {
    // Return true if this line ends with '\', and null out the
    // backslash.
    //
    bool check_continued(char *str)
    {
        if (!str || !*str)
            return (false);
        bool contd = false;
        char *s = str + strlen(str) - 1;
        while (isspace(*s) && s != str)
            s--;
        if (*s == '\\') {
            contd = true;
            // Allow multiple backslashes, some programs require two.
            while (*s == '\\' && s >= str)
                *s-- = 0;
        }
        return (contd);
    }
}


// Return a raw SPICE deck.  If fp is null, obtain text from the
// message socket.  The first line has been read, and is passed as the
// title argument.
//
// This handles conditionals, out-of-scope lines are elided.  The ptab
// points to a parameter table, or 0 to create one.  This should be
// deleted after top-level return (this function is recursive).
//
sLine *
IFsimulator::ReadDeck(FILE *fp, char *title, bool *err, sParamTab **ptab,
    const char *filename)
{
    sLine *deck = 0, *end = 0;
    int line = 1;
    bool contlast = false;

    // Set up the initial values for the subcircuit expansion
    // keywords, which are used in the initial parse for conditional
    // testing (we skip parameters defined in subcircuits).  This will
    // NOT reflect definitions supplied from .options in the circuit
    // file.
    //
    SPcx.kwMatchInit();

    // Read in all lines from the file.  Backslash line continuation is
    // handled here.

    for (;;) {
        char *buffer;
        bool title_line = false;
        if (title) {
            buffer = lstring::copy(title);
            title = 0;
            title_line = true;
        }
        else if ((buffer = ReadLine(fp, err)) == 0) {
            if (*err) {
                if (filename) {
                    GRpkgIf()->ErrPrintf(ET_MSG,
                        "Error while reading file %s.\n", filename);
                }
                sLine::destroy(deck);
                return (0);
            }
            break;
        }

        // Remove trailing white space.
        char *s = buffer + strlen(buffer) - 1;
        while (s >= buffer && isspace(*s))
            *s-- = 0;

        // Empty line, but allow empty title.
        if (!title_line && !buffer[0]) {
            delete [] buffer;
            continue;
        }

        // End of transmission.
        if (buffer[0] == '@' && buffer[1] == 0) {
            delete [] buffer;
            break;
        }

        s = buffer;
        if (!contlast) {
            // Strip any leading white space.
            while (isspace(*s))
                s++;
            if (s != buffer) {
                char *t = buffer;
                while (*s)
                    *t++ = *s++;
                *t = '\0';
            }
        }

        if (lstring::cimatch(ENDL_KW, buffer) ||
                lstring::cimatch(ENDL_KW, buffer)) {
            delete [] buffer;
            break;
        }

        if (contlast) {
            contlast = check_continued(buffer);
            end->append_line(buffer, false);
        }
        else {
            if (end) {
                end->set_next(new sLine);
                end = end->next();
            }
            else
                end = deck = new sLine;
            contlast = check_continued(buffer);
            end->set_line(buffer);
            end->set_line_num(line++);
        }
        delete [] buffer;
    }
    if (!deck)
        return (0);

    // Process .param, .title, .if/.elif/.else/.endif
    // This returns a parameter table udated or created from the
    // .param lines, which is used for conditional evaluation.  It
    // also shell expands .if/.elif lines and evaluates these, pruning
    // the deck accordingly.  The conditional lines are removed.

    *ptab = deck->process_conditionals(*ptab);

    // Recursively expand the includes and library references.

    for (sLine *dd = deck; dd; dd = dd->next()) {

        if (lstring::cimatch(INCL_L_KW, dd->line()) ||
                lstring::cimatch(INCL_S_KW, dd->line()) ||
                lstring::cimatch(SPINCL_KW, dd->line())) {
            const char *s = dd->line();
            char *init_tok = lstring::gettok(&s);

            char *stmp = lstring::copy(s);
            // Apply variable substitution to line.
            CP.VarSubst(&stmp);
            s = stmp;
            char *path = lstring::getqtok(&s);
            bool compat_mode = false;
            bool dc_bak = GetVar(kw_dollarcmt, VTYP_BOOL, 0);
            if (path) {
                // If the first token is "H", use HSPICE compatibility
                // mode during the read.
                if (lstring::cieq(path, "h")) {
                    compat_mode = true;
                    delete [] path;
                    path = lstring::getqtok(&s);
                }
            }
            delete [] stmp;

            if (path) {
                char *t = CP.TildeExpand(path);
                if (t) {
                    delete [] path;
                    path = t;
                }

                // Strip off the path, we're going to chdir there for the
                // duration of the read.
                char *file;
                parse_path(&path, &file);

                char *cwd = getcwd(0, 0);
                if (path) {
                    if (chdir(path) < 0) {
                        GRpkgIf()->Perror(path);
                        GRpkgIf()->ErrPrintf(ET_ERROR,
                            "Failed to chdir to %s.\n", path);
                        if (filename) {
                            GRpkgIf()->ErrPrintf(ET_MSG,
                                "Error while reading file %s.\n", filename);
                        }
                        *err = true;
                        sLine::destroy(deck);
                        delete [] path;
                        delete [] init_tok;
                        delete [] file;
                        delete [] cwd;
                        return (0);
                    }
                }

                FILE *newfp = fopen(file, "r");

                if (newfp) {
                    if (compat_mode)
                        SetVar(kw_dollarcmt);
                    sLine *newcard = ReadDeck(newfp, 0, err, ptab, file);
                    if (compat_mode && !dc_bak)
                        RemVar(kw_dollarcmt);
                    fclose(newfp);
                    if (*err) {
                        if (filename) {
                            GRpkgIf()->ErrPrintf(ET_MSG,
                                "From file %s.\n", filename);
                        }
                        sLine::destroy(deck);
                        if (path)
                            chdir(cwd);
                        delete [] path;
                        delete [] init_tok;
                        delete [] file;
                        delete [] cwd;
                        return (0);
                    }

                    // Make the INCL_KW (".include") a comment.
                    dd->comment_out();

                    line = dd->line_num() + 1;

                    // Link in the new lines.
                    sLine *tl = dd->next();
                    dd->set_next(newcard);
                    while (dd->next())
                        dd = dd->next();

                    // Add end comment.
                    dd->set_next(new sLine);
                    dd = dd->next();
                    dd->set_line("*end ");
                    dd->append_line(file, false);
                    dd->set_next(tl);

                    // Renumber the cards
                    for (end = newcard; end; end = end->next())
                        end->set_line_num(line++);
                }
                else {
                    GRpkgIf()->Perror(file);
                    GRpkgIf()->ErrPrintf(ET_ERROR,
                        "Failed to open included file %s.\n", file);
                    if (filename) {
                        GRpkgIf()->ErrPrintf(ET_MSG,
                            "Error while reading file %s.\n", filename);
                    }
                    *err = true;
                    sLine::destroy(deck);
                    if (path)
                        chdir(cwd);
                    delete [] path;
                    delete [] init_tok;
                    delete [] file;
                    delete [] cwd;
                    return (0);
                }

                if (path)
                    chdir(cwd);
                delete [] file;
                delete [] cwd;
            }
            else {
                GRpkgIf()->ErrPrintf(ET_ERROR, "filename missing from %s.\n",
                    init_tok);
                if (filename) {
                    GRpkgIf()->ErrPrintf(ET_MSG,
                        "Error while reading file %s.\n", filename);
                }
                *err = true;
                sLine::destroy(deck);
                delete [] init_tok;
                return (0);
            }
            delete [] path;
            delete [] init_tok;
        }
        else if (lstring::cimatch(LIB_KW, dd->line()) ||
                lstring::cimatch(SPLIB_KW, dd->line())) {
            const char *s = dd->line();
            char *init_tok = lstring::gettok(&s);

            char *stmp = lstring::copy(s);
            // Apply variable substitutuin to line.
            CP.VarSubst(&stmp);
            s = stmp;
            char *path = lstring::getqtok(&s);
            bool compat_mode = false;
            bool dc_bak = GetVar(kw_dollarcmt, VTYP_BOOL, 0);
            if (path) {
                // If the first token is "H", use HSPICE compatibility
                // mode during the read.
                if (lstring::cieq(path, "h")) {
                    compat_mode = true;
                    delete [] path;
                    path = lstring::getqtok(&s);
                }
            }
            char *name = lstring::gettok(&s);
            delete [] stmp;

            if (name) {
                char *t = CP.TildeExpand(path);
                if (t) {
                    delete [] path;
                    path = t;
                }

                // Strip off the path, we're going to chdir there for the
                // duration of the read.
                char *file;
                parse_path(&path, &file);

                char *cwd = getcwd(0, 0);
                if (path) {
                    if (chdir(path) < 0) {
                        GRpkgIf()->Perror(path);
                        GRpkgIf()->ErrPrintf(ET_ERROR,
                            "Failed to chdir to %s.\n", path);
                        if (filename) {
                            GRpkgIf()->ErrPrintf(ET_MSG,
                                "Error while reading file %s.\n", filename);
                        }
                        *err = true;
                        sLine::destroy(deck);
                        delete [] path;
                        delete [] name;
                        delete [] init_tok;
                        delete [] file;
                        delete [] cwd;
                        return (0);
                    }
                }
                long offs = LibMap->find(file, name);
                if (offs == LM_NO_NAME) {
                    GRpkgIf()->ErrPrintf(ET_ERROR,
                        "block %s not found in library %s.\n", name, file);
                    if (filename) {
                        GRpkgIf()->ErrPrintf(ET_MSG,
                            "Error while reading file %s.\n", filename);
                    }
                    *err = true;
                    sLine::destroy(deck);
                    if (path)
                        chdir(cwd);
                    delete [] path;
                    delete [] name;
                    delete [] init_tok;
                    delete [] file;
                    delete [] cwd;
                    return (0);
                }

                FILE *newfp;
                if (offs == LM_NO_FILE || (newfp = fopen(file, "r")) == 0) {
                    GRpkgIf()->Perror(file);
                    GRpkgIf()->ErrPrintf(ET_ERROR,
                        "Failed to open library file %s.\n", file);
                    if (filename) {
                        GRpkgIf()->ErrPrintf(ET_MSG,
                            "Error while reading file %s.\n", filename);
                    }
                    *err = true;
                    sLine::destroy(deck);
                    if (path)
                        chdir(cwd);
                    delete [] path;
                    delete [] name;
                    delete [] init_tok;
                    delete [] file;
                    delete [] cwd;
                    return (0);
                }

                if (fseek(newfp, offs, SEEK_SET) < 0) {
                    GRpkgIf()->Perror(file);
                    GRpkgIf()->ErrPrintf(ET_ERROR,
                        "Seek failed in library file %s.\n", file);
                    if (filename) {
                        GRpkgIf()->ErrPrintf(ET_MSG,
                            "Error while reading file %s.\n", filename);
                    }
                    *err = true;
                    sLine::destroy(deck);
                    if (path)
                        chdir(cwd);
                    fclose(newfp);
                    delete [] path;
                    delete [] name;
                    delete [] init_tok;
                    delete [] file;
                    delete [] cwd;
                    return (0);
                }
                if (compat_mode)
                    SetVar(kw_dollarcmt);
                sLine *newcard = ReadDeck(newfp, 0, err, ptab, file);
                if (compat_mode && !dc_bak)
                    RemVar(kw_dollarcmt);
                fclose(newfp);
                if (*err) {
                    if (filename) {
                        GRpkgIf()->ErrPrintf(ET_MSG,
                            "From file %s.\n", filename);
                    }
                    sLine::destroy(deck);
                    if (path)
                        chdir(cwd);
                    delete [] path;
                    delete [] name;
                    delete [] init_tok;
                    delete [] file;
                    delete [] cwd;
                    return (0);
                }

                // Make the .lib a comment.
                dd->comment_out();

                line = dd->line_num() + 1;

                // Link in the new lines.
                sLine *tl = dd->next();
                dd->set_next(newcard);
                while (dd->next())
                    dd = dd->next();

                // Add end comment.
                dd->set_next(new sLine);
                dd = dd->next();
                dd->set_line("*end ");
                dd->append_line(file, false);
                dd->append_line(name, true);
                dd->set_next(tl);

                // Renumber the cards
                for (end = newcard; end; end = end->next())
                    end->set_line_num(line++);

                if (path)
                    chdir(cwd);
                delete [] file;
                delete [] cwd;
            }
            else {
                if (!path) {
                    GRpkgIf()->ErrPrintf(ET_ERROR,
                        "file name missing from %s call.\n", init_tok);
                    if (filename) {
                        GRpkgIf()->ErrPrintf(ET_MSG,
                            "Error while reading file %s.\n", filename);
                    }
                }
                else {
                    GRpkgIf()->ErrPrintf(ET_ERROR,
                        "file or block name missing from %s call.\n",
                        init_tok);
                    if (filename) {
                        GRpkgIf()->ErrPrintf(ET_MSG,
                            "Error while reading file %s.\n", filename);
                    }
                    if (path == lstring::strip_path(path))
                        GRpkgIf()->ErrPrintf(ET_MSG,
    "This appears to be library block definition for \"%s\".\n"
    "Files containing library definitions can not be read as WRspice input\n"
    "except through .lib calls.\n", path);
                }

                *err = true;
                sLine::destroy(deck);
                delete [] path;
                delete [] name;
                delete [] init_tok;
                return (0);
            }
            delete [] path;
            delete [] name;
            delete [] init_tok;
        }
    }
    return (deck);
}


// Join SPICE continuation lines in the deck.
//
void
IFsimulator::ContinueLines(sLine *deck)
{
    if (!deck)
        return;
    sLine *working = deck->next();      // Skip title
    sLine *prev = 0;
    while (working) {
        const char *s = working->line();
        while (isspace(*s))
            s++;
        switch (*s) {
        case '*':
        case '\0':
            working = working->next();
            break;
        case '+':
            if (!prev) {
                working->set_error("Illegal continuation card: ignored.");
                working = working->next();
                break;
            }
            {
                char *lst = prev->actual() ? 0 : lstring::copy(prev->line());
                prev->append_line(s+1, true);
                prev->set_next(working->next());
                working->set_next(0);
                if (prev->actual()) {
                    sLine *end = prev->actual();
                    while (end->next())
                        end = end->next();
                    end->set_next(working);
                }
                else {
                    sLine *newcard = new sLine;
                    newcard->set_line_num(prev->line_num());
                    newcard->set_line(lst);
                    delete [] lst;
                    newcard->set_next(working);
                    prev->set_actual(newcard);
                }
                working = prev->next();
            }
            break;
        default:
            prev = working;
            working = working->next();
            break;
        }
    }
}


void
IFsimulator::CaseFix(char *string)
{
    if (string) {
        while (*string) {
            *string = STRIP(*string);
            if (!isspace(*string) && !isprint(*string))
                *string = '_';
            string++;
        }
    }
}
// End of IFsimulator functions.


// Update the UDFs.
//
void
sFtCirc::setDefines(cUdf *udfs)
{
    delete ci_defines;
    ci_defines = udfs;
}


namespace {
    // Copy str, if it has multiple tokens return a quoted copy.
    //
    char *quote(char *str)
    {
        if (!str)
            return (0);
        char *s = str;
        if (*s != '"') {
            int n = 0;
            while (lstring::advtok(&s))
                n++;
            if (n > 1) {
                char *tmp = new char[strlen(str) + 3];
                s = tmp;
                *s++ = '"';
                s = lstring::stpcpy(s, str);
                *s++ = '"';
                *s = 0;
                return (tmp);
            }
        }
        return (lstring::copy(str));
    }

    // Remove, in-place, double quotes.
    //
    void dequote(char *str)
    {
        if (str && *str == '"') {
            char *s = str + strlen(str) - 1;
            if (s > str && *s == '"') {
                *s = 0;
                s = str+1;
                for ( ; *s; s++)
                    s[-1] = s[0];
                s[-1] = 0;
            }
        }
    }
}


// Set up a new circuit.  First phase, parse the deck and set up various
// data areas.
//
void
sFtCirc::setup(sLine *spdeck, const char *fname, wordlist *exec,
    wordlist *ctrl, sLine *vlog, sParamTab *ptab)
{
    ci_descr = lstring::copy(spdeck->line());
    ci_verilog = vlog;
    ci_params = ptab;
    ci_execBlk.set_text(exec);
    ci_controlBlk.set_text(ctrl);
    if (fname)
        ci_filename = lstring::copy(fname);

    // Grab options lines, in order.
    ci_options = Sp.GetDotOpts(spdeck);

    // Now parse the options, shell expanding when '$' found along
    // the way.  The options are parsed top-to-bottom and left-to-right,
    // and the result of each option setting is available immediately.
    // Thus, forms like ".options aaa=1 bbb=$aaa" will work.
    //
    variable *vv = 0;
    for (sLine *dd = ci_options; dd; dd = dd->next()) {
        const char *s = dd->line();
        char *optok = lstring::gettok(&s);
        sLstr lstr;
        lstr.add(optok);
        delete [] optok;

        char *pname, *value;
        while (IP.getOption(&s, &pname, &value)) {
            CP.VarSubst(&pname);
            dequote(pname);
            if (value) {
                CP.VarSubst(&value);
                if (ptab) {
                    ptab->param_subst_options(&value);
                    if (ptab->errString) {
                        GRpkgIf()->ErrPrintf(ET_WARN, "line %d : %s\n\%s\n",
                            dd->line_num(), dd->line(), ptab->errString);
                        delete [] ptab->errString;
                        ptab->errString = 0;
                    }
                }
                dequote(value);
            }

            char *qpname = quote(pname);
            char *qvalue = quote(value);
            lstr.add_c(' ');
            lstr.add(qpname);
            if (qvalue) {
                lstr.add_c('=');
                lstr.add(qvalue);
            }
            delete [] qpname;
            delete [] qvalue;

            wordlist *wl = new wordlist(pname, 0);
            if (value) {
                wl->wl_next = new wordlist("=", wl);
                wl->wl_next->wl_next = new wordlist(value, wl->wl_next);
            }

            variable *v = CP.ParseSet(wl);
            wordlist::destroy(wl);
            delete [] pname;
            delete [] value;
            if (!vv) {
                vv = v;
                ci_vars = v;
            }
            else {
                while (vv->next())
                    vv = vv->next();
                vv->set_next(v);
            }
        }
        dd->set_line(lstr.string());
    }

    if (Sp.GetFlag(FT_BATCHMODE)) {
        // Set variables for batch mode.
        for (variable *v = ci_vars; v; v = v->next()) {
            // This is where these batch mode only options are
            // "officially" set.  They are also caught in ci_setOption(),
            // so that they can be set through the shell as well.
            //
            if (lstring::cieq(v->name(), spkw_acct))
                Sp.SetFlag(FT_ACCTPRNT, true);
            else if (lstring::cieq(v->name(), spkw_dev))
                Sp.SetFlag(FT_DEVSPRNT, true);
            else if (lstring::cieq(v->name(), spkw_list))
                Sp.SetFlag(FT_LISTPRNT, true);
            else if (lstring::cieq(v->name(), spkw_mod))
                Sp.SetFlag(FT_MODSPRNT, true);
            else if (lstring::cieq(v->name(), spkw_node))
                Sp.SetFlag(FT_NODESPRNT, true);
            else if (lstring::cieq(v->name(), spkw_opts))
                Sp.SetFlag(FT_OPTSPRNT, true);
            else if (lstring::cieq(v->name(), spkw_nopage))
                Sp.SetFlag(FT_NOPAGE, true);
            else if (lstring::cieq(v->name(), spkw_numdgt)) {
                if (v->type() == VTYP_NUM)
                    CP.SetNumDigits(v->integer());
                else if (v->type() == VTYP_REAL)
                    CP.SetNumDigits((int)v->real());
            }
            else if (lstring::cieq(v->name(), spkw_post)) {
                if (!Sp.GetFlag(FT_RAWFGIVEN) && v->type() == VTYP_STRING) {
                    if (lstring::cieq(v->string(), "csdf") ||
                            lstring::cieq(v->string(), "raw")) {
                        const char *tfn = filename();
                        if (!tfn)
                            tfn = "unknown";
                        char *fn = new char[strlen(filename()) + 10];
                        strcpy(fn, tfn);
                        char *t = strrchr(fn, '.');
                        if (t && (t > fn)) {
                            if (lstring::cieq(t, ".sp") ||
                                    lstring::cieq(t, ".cir"))
                                *t = 0;
                        }
                        t = fn + strlen(fn);
                        *t = '.';
                        strcpy(t+1, v->string());
                        while (*t) {
                            if (isupper(*t))
                                *t = tolower(*t);
                            t++;
                        }
                        OP.getOutDesc()->set_outFile(fn);
                        delete [] fn;
                        Sp.SetFlag(FT_RAWFGIVEN, true);
                    }
                }
            }
        }
    }

    Sp.SetCircuit(ci_name);

    // Set up the ci_defOpts from the options list.

    ci_defOpt = new sOPTIONS;
    IP.parseOptions(ci_options, ci_defOpt);
    for (sLine *dd = ci_options; dd; dd = dd->next()) {
        if (dd->error()) {
            GRpkgIf()->ErrPrintf(ET_WARN, "line %d : %s\n%s\n", 
                dd->line_num(), dd->line(), dd->error());
        }
    }

    // Substitute for shell variables in the spice deck as it is read. 
    // The variables must have been defined before the deck is sourced
    // (in the EXEC_KW (".exec") block, NOT in the CONT_KW
    // (".control") block), or in the .options line.

    // The .options have been parsed, and are now available in the
    // current circuit.

    Tvals tv;
    tv.start();

    int lcnt = 0;
    for (sLine *dd = spdeck->next(); dd; dd = dd->next()) {
        if (*dd->line() == '*')
            continue;
        if (strchr(dd->line(), '$'))
            dd->var_subst();
    }
    tv.stop();

    if (Sp.GetFlag(FT_SIMDB)) {
        GRpkgIf()->ErrPrintf(ET_MSGS,
        "Parameter and variable substitution: params=%d lines=%d time=%g.\n",
            ptab ? ptab->allocated() : 0, lcnt, tv.realTime());
    }

    // Remove (...) around node lists.  This is for convenience in
    // later processing.
    //
    for (sLine *c = spdeck; c; c = c->next())
        c->elide_parens();

    ci_deck = spdeck;
}


// Second phase, perform subcircuit expansion.
//
bool
sFtCirc::expand(sLine *realdeck, bool *err)
{
    if (err)
        *err = false;
#ifdef TIME_DEBUG
    double tstart = OP.seconds();
#endif
    ci_commands = ci_deck->get_speccmds();
    if (!ci_deck->next()) {
        if (err)
            *err = true;
        GRpkgIf()->ErrPrintf(ET_WARN, "no lines in input.\n");
        sLine::destroy(realdeck);
        return (false);
    }

    // Subcircuit expansion also performs parameter substitution on
    // the lines in the deck.

    if (!Sp.GetVar(kw_nosubckt, VTYP_BOOL, 0)) {

        // Expand the subcircuits in the deck, resulting in a flat
        // representation of the circuit.
        Tvals tv;
        tv.start();
        expandSubckts();
        tv.stop();
        if (Sp.GetFlag(FT_SIMDB))
            GRpkgIf()->ErrPrintf(ET_MSGS,
                "Subcircuit expansion time: %g.\n", tv.realTime());

        if (!ci_deck->next()) {
            if (err)
                *err = true;
            GRpkgIf()->ErrPrintf(ET_ERROR,
                "subcircuit expansion failed, circuit not loaded.\n");
            sLine::destroy(realdeck);
            return (false);
        }
    }

    for (sLine *dd = ci_deck->next(); dd; dd = dd->next()) {
        // Set up any .measures or .stops.
        //
        if (lstring::cimatch(MEAS_KW, dd->line()) ||
                lstring::cimatch(MEASURE_KW, dd->line())) {

            char *er = 0;
            const char *line = dd->line();
            lstring::advtok(&line);
            sRunopMeas *m = new sRunopMeas(line, &er);
            if (er) {
                dd->set_error(er);
                delete [] er;
                delete m;
            }
            else {
                m->set_number(OP.runops()->new_count());
                m->set_active(true);
                
                if (!ci_runops.measures())
                    ci_runops.set_measures(m);
                else {
                    sRunopMeas *mx = ci_runops.measures();
                    while (mx->next())
                        mx = mx->next();
                    mx->set_next(m);
                }
            }
        }
        else if (lstring::cimatch(STOP_KW, dd->line())) {

            char *er = 0;
            const char *line = dd->line();
            lstring::advtok(&line);
            sRunopStop *m = new sRunopStop(line, &er);
            if (er) {
                dd->set_error(er);
                delete [] er;
                delete m;
            }
            else {
                m->set_number(OP.runops()->new_count());
                m->set_active(true);
                
                if (!ci_runops.stops())
                    ci_runops.set_stops(m);
                else {
                    sRunopStop *mx = ci_runops.stops();
                    while (mx->next())
                        mx = mx->next();
                    mx->set_next(m);
                }
            }
        }
        // Maybe add .trace, .iplot?
    }

    ci_deck->set_actual(realdeck);
    ci_origdeck = realdeck;
    ci_inprogress = false;
    ci_runonce = false;

    CP.AddKeyword(CT_CKTNAMES, ci_descr);
    for (sLine *dd = ci_deck->next(); dd; dd = dd->next())
        Sp.AddNodeNames(dd->line());

    // Print any errors, and free them.
    for (sLine *dd = ci_deck; dd; dd = dd->next()) {
        if (dd->error()) {
            // If a commented line has an error, it must have been
            // commented out in subcircuit expansion.  Skip the
            // comment character here.

            int xx = (*dd->line() == '*');
            GRpkgIf()->ErrPrintf(ET_WARN, "line %d : %s\n%s\n", 
                dd->line_num(), dd->line() + xx, dd->error());
            dd->set_error(0);
        }
    }

#ifdef TIME_DEBUG
    double tend = OP.seconds();
    printf("subc expand: %g\n", tend - tstart);
#endif

    return (true);
}


int
sFtCirc::newCKT(sCKT **cktp, sTASK **taskp)
{
#ifdef TIME_DEBUG
    double tstart = OP.seconds();
#endif
    *cktp = 0;
    if (taskp)
        *taskp = 0;

    if (!ci_deck) {
        // We get here from .exec blocks, haven't processed the deck
        // yet.

        GRpkgIf()->ErrPrintf(ET_ERROR,
            "newCKT: can't build circuit object before text expansion.\n");
        return (E_FAILED);
    }
    sCKT *sckt = new sCKT;
    sckt->CKTbackPtr = this;

    IFuid taskUid;
    int ret = sckt->newUid(&taskUid, 0, "default", UID_TASK);
    if (ret) {
        Sp.Error(ret, "newUid");
        delete sckt;
        return (ret);
    }
    sTASK *task = taskp ? new sTASK(taskUid) : 0;
    if (ci_verilog) {
        // this must be done before parse
        sckt->CKTvblk = new VerilogBlock(ci_verilog);
    }
    IP.parseDeck(sckt, ci_deck->next(), task, false);
    *cktp = sckt;
    if (taskp)
        *taskp = task;
    if (ret == OK && sckt->CKTnogo)
        ret = E_FAILED;

    // Print any errors, and free them.
#define MAX_LINES 50
    int ercnt = 0;
    for (sLine *dd = ci_deck; dd; dd = dd->next()) {
        if (dd->error()) {
            // If a commented line has an error, it must have been
            // commented out in subcircuit expansion.  Skip the
            // comment character here.

            int xx = (*dd->line() == '*');
            if (ercnt < MAX_LINES) {
                GRpkgIf()->ErrPrintf(ET_MSG, "Line %d : %s\n%s\n", 
                    dd->line_num(), dd->line() + xx, dd->error());
            }
            ercnt++;
            dd->set_error(0);
        }
    }
    if (ercnt >=  MAX_LINES)
        GRpkgIf()->ErrPrintf(ET_MSG, "...  More errors were detected.\n");

#ifdef TIME_DEBUG
    tend = OP.seconds();
    printf("parse: %g\n", tend - tstart);
#endif
    return (ret);
}
// End of sFtCirc functions.


// Return true if there is a circuit.
//
bool
sLine::is_ckt()
{
    for (sLine *l = next(); l; l = l->next()) {
        const char *s = l->line();
        if (!s)
            continue;
        while (isspace(*s))
            s++;
        if (!*s || *s == '*' || *s == '#')
            continue;
        if (*s == '.' && lstring::ciprefix("end", s+1))
            continue;
        // found something real
        return (true);
    }
    return (false);
}


wordlist *
sLine::get_speccmds()
{
    wordlist *wl = 0, *end = 0;
    sLine *ld = this;
    for (sLine *dd = next(); dd; dd = ld->next()) {
        if (dd->line()[0] == '*') {
            ld = dd;
            continue;
        }
        const char *s = dd->line();
        while (isspace(*s)) s++;
        dd->fix_line();

        if (lstring::cimatch(WIDTH_KW, s) ||
                lstring::cimatch(FOUR_KW, s) ||
                lstring::cimatch(PLOT_KW, s) ||
                lstring::cimatch(PRINT_KW, s) ||
                lstring::cimatch(SAVE_KW, s) ||
                lstring::cimatch(PROBE_KW, s)) {
            if (end) {
                end->wl_next = new wordlist(dd->line(), end);
                end->wl_next->wl_prev = end;
                end = end->wl_next;
            }
            else
                wl = end = new wordlist(dd->line(), 0);
            ld->set_next(dd->next());
            delete dd;
        }
        else
            ld = dd;
    }
    return (wl);
}


// Set flags if certain keywords are found in the deck.  The "flag"
// keywords are removed from the deck, those that start a block are
// retained.
//
void
sLine::get_keywords(int *kwfound)
{
    if (!kwfound)
        return;
    *kwfound = 0;
    sLine *ld = this;
    for (sLine *dd = li_next; dd; dd = ld->li_next) {
        if (lstring::cimatch(CHECKALL_KW, dd->li_line)) {
            *kwfound |= LI_CHECKALL_FOUND;
            ld->li_next = dd->li_next;
            delete dd;
            continue;
        }
        if (lstring::cimatch(CHECK_KW, dd->li_line)) {
            *kwfound |= LI_CHECK_FOUND;
            ld->li_next = dd->li_next;
            delete dd;
            continue;
        }
        if (lstring::cimatch(MONTE_KW, dd->li_line)) {
            *kwfound |= LI_MONTE_FOUND;
            ld->li_next = dd->li_next;
            delete dd;
            continue;
        }
        if (lstring::cimatch(NOEXEC_KW, dd->li_line)) {
            *kwfound |= LI_NOEXEC_FOUND;
            ld->li_next = dd->li_next;
            delete dd;
            continue;
        }

        if (lstring::cimatch(CONT_KW, dd->li_line) ||
                lstring::cimatch(CONT_PREFX, dd->li_line))
            *kwfound |= LI_CONT_FOUND;
        else if (lstring::cimatch(EXEC_KW, dd->li_line) ||
                lstring::cimatch(EXEC_PREFX, dd->li_line))
            *kwfound |= LI_EXEC_FOUND;
        else if (lstring::cimatch(POST_KW, dd->li_line))
            *kwfound |= LI_POST_FOUND;
        ld = dd;
    }
}


namespace {
    // Handle in-line '#' comments.  We would like to use this as an
    // in-line comment character, but this can be part of a vector
    // name, etc., so we need to be careful.  We'll take it as a
    // comment delimeter if it appears at the line start or after
    // white space.  We've already dealt with the case where the first
    // line char is '#'.
    //
    // If the # is preceded by a backslash, the user is telling us
    // that the # is explicitly not a comment.  Strip the backslash.
    //
    void clip_comment(char *str)
    {
        char *t = strchr(str, '#');
        while (t) {
            if (isspace(*(t-1))) {
                // Don't leave trailing space.
                t--;
                while (t >= str && isspace(*t))
                    *t-- = 0;
                break;
            }
            if (*(t-1) == '\\') {
                for (char *c = t-1; *c; c++)
                    *c = *(c+1);
                t = strchr(t, '#');
            }
            else
                t = strchr(t+1, '#');
        }
    }
}


// Strip the control lines out of the deck and return as a wordlist. 
// If allcmds is true, assume that all lines are commands, up to the
// block terminator.  If not allcmds, the contblk sets whether we are
// saving control commands (true) or exec commands (false). 
// Additionally, lines starting with "*#" are taken as control
// commands, "*@" are taken as exec commands, if outside of blocks. 
// They are taken as comments (killed) if inside blocks.  Blocks
// should not overlap.  If kwfound is not 0, check for margin analysis
// keywords and set *kwfound code if found.  If oldformat is true,
// start out saving exec block, up until CONT_KW.
//
wordlist *
sLine::get_controls(bool allcmds, CBLK_TYPE type, bool oldformat)
{
    const char *cmdkey, *altkey1, *altkey2, *prefx;
    if (type == CBLK_EXEC) {
        cmdkey = EXEC_KW;
        altkey1 = CONT_KW;
        altkey2 = POST_KW;
        prefx = EXEC_PREFX;
    }
    else if (type == CBLK_CTRL) {
        cmdkey = CONT_KW;
        altkey1 = EXEC_KW;
        altkey2 = POST_KW;
        prefx = CONT_PREFX;
        // exec block only
        oldformat = false;
    }
    else {
        cmdkey = POST_KW;
        altkey1 = EXEC_KW;
        altkey2 = CONT_KW;
        prefx = 0;
        // exec block only
        oldformat = false;
    }
    wordlist *unnamed_blk = 0, *ub_end = 0;
    wordlist *named_blk = 0, *nb_end = 0;;
    bool commands = oldformat;
    bool altcmds1 = false;
    bool altcmds2 = false;
    char *blkname = 0;

    sLine *ld = this;
    for (sLine *dd = li_next; dd; dd = ld->li_next) {
        if (lstring::cimatch(cmdkey, dd->li_line)) {
            const char *str = dd->li_line;
            lstring::advtok(&str);
            blkname = lstring::gettok(&str);

            if (allcmds || commands)
                GRpkgIf()->ErrPrintf(ET_WARN, "redundant %s card.\n", cmdkey);
            ld->li_next = dd->li_next;
            delete dd;
            commands = true;
        }
        else if (lstring::cimatch(altkey1, dd->li_line)) {
            if (oldformat)
                break;
            if (allcmds || altcmds1)
                GRpkgIf()->ErrPrintf(ET_WARN, "redundant %s card.\n", altkey1);
            if (allcmds) {
                ld->li_next = dd->li_next;
                delete dd;
            }
            else
                ld = dd;
            altcmds1 = true;
        }
        else if (lstring::cimatch(altkey2, dd->li_line)) {
            if (oldformat)
                break;
            if (allcmds || altcmds2)
                GRpkgIf()->ErrPrintf(ET_WARN, "redundant %s card.\n", altkey2);
            if (allcmds) {
                ld->li_next = dd->li_next;
                delete dd;
            }
            else
                ld = dd;
            altcmds2 = true;
        }
        else if (lstring::cimatch(ENDC_KW, dd->li_line)) {
            if (allcmds)
                break;
            if (commands) {
                ld->li_next = dd->li_next;
                delete dd;
                commands = false;
            }
            else if (altcmds1) {
                ld = dd;
                altcmds1 = false;
            }
            else if (altcmds2) {
                ld = dd;
                altcmds2 = false;
            }
            else {
                GRpkgIf()->ErrPrintf(ET_WARN, "misplaced %s card.\n",
                    ENDC_KW);
                ld->li_next = dd->li_next;
                delete dd;
            }
            if (blkname) {
                // Save as named codeblock and clear state, there may
                // be arbitrarily many of these.

                CP.AddBlock(blkname, named_blk);

                delete [] blkname;
                blkname = 0;
                wordlist::destroy(named_blk);
                named_blk = 0;
            }
        }
        else if (!*dd->li_line || ((allcmds ||
                (commands && !altcmds1 && !altcmds2)) &&
                (*dd->li_line == '#' || *dd->li_line == '*'))) {
            // So blank lines in com files don't get considered as
            // circuits.  Also kill comments in commands.  Note that
            // "*#" doesn't work in command files.

            ld->li_next = dd->li_next;
            delete dd;
        }
        else if (allcmds || (commands && !altcmds1 && !altcmds2 && !blkname) ||
                (!altcmds1 && !altcmds2 && prefx &&
                lstring::prefix(prefx, dd->li_line))) {
            // element of an unnamed block

            const char *c;
            if (prefx && lstring::prefix(prefx, dd->li_line)) {
                c = dd->li_line + 2;
                while (isspace(*c))
                    c++;
            }
            else
                c = dd->li_line;

            if (!unnamed_blk)
                ub_end = unnamed_blk = new wordlist(c, 0);
            else {
                ub_end->wl_next = new wordlist(c, ub_end);
                ub_end->wl_next->wl_prev = ub_end;
                ub_end = ub_end->wl_next;
            }
            clip_comment(ub_end->wl_word);

            ld->li_next = dd->li_next;
            delete dd;
        }
        else if (commands && !altcmds1 && !altcmds2) {
            // element of a named block

            if (!named_blk)
                named_blk = nb_end = new wordlist(dd->li_line, 0);
            else {
                nb_end->wl_next = new wordlist(dd->li_line, nb_end);
                nb_end = nb_end->wl_next;
            }
            clip_comment(nb_end->wl_word);

            ld->li_next = dd->li_next;
            delete dd;
        }
        else
            ld = dd;
    }
    return (unnamed_blk);
}


// Remove the .verilog lines, which terminate with .end, and the .adc
// lines.  It is assumed that only one such block can exist in the
// deck.  The .adc lines (if any) are stitched ahead of the verilog
// block.
//
sLine *
sLine::extract_verilog()
{
    sLine *dp = this;
    sLine *start = 0, *vl = 0, *vl0 = 0;;
    sLine *ad0 = 0, *ad = 0;
    bool inblk = false;
    for (sLine *d = li_next; d; dp = d, d = d->li_next) {
        if (d->li_line && *d->li_line == '.') {
            if (!inblk && lstring::cimatch(VERILOG_KW, d->li_line)) {
                inblk = true;
                start = dp;
                vl = d;
                continue;
            }
            if (!inblk && lstring::cimatch(ADC_KW, d->li_line)) {
                dp->li_next = d->li_next;
                d->li_next = 0;
                if (!ad0)
                    ad0 = ad = d;
                else {
                    ad->li_next = d;
                    ad = ad->li_next;
                }
                d = dp;
                continue;
            }
            if (lstring::cimatch(ENDV_KW, d->li_line)) {
                if (!inblk) {
                    d->set_error(
                        "Warning: misplaced .endv, not in block (ignored).");
                    d->comment_out();
                    continue;
                }
                inblk = false;
                start->li_next = d->li_next;
                d->li_next = 0;

                if (!vl0)
                    vl0 = vl;
                else {
                    // A block was already read, merge in the new lines.
                    sLine *tmp = vl0;
                    while (tmp->li_next && tmp->li_next->li_next)
                        tmp = tmp->li_next;

                    sLine::destroy(tmp->li_next);  // .endv
                    tmp->li_next = vl->li_next;
                    delete vl;
                }
                vl = 0;

                d = start->li_next;
                if (!d)
                    break;
            }
        }
    }
    if (inblk) {
        // No .endv seen.
        start->li_next = 0;
        if (!vl0)
            vl0 = vl;
        else {
            // A block was already read, merge in the new lines.
            sLine *tmp = vl0;
            while (tmp->li_next && tmp->li_next->li_next)
                tmp = tmp->li_next;

            sLine::destroy(tmp->li_next);  // .endv
            tmp->li_next = vl->li_next;
            delete vl;
        }
    }
    if (ad0) {
        ad = ad0;
        while (ad->li_next)
            ad = ad->li_next;
        ad->li_next = vl0;
    }
    else
        ad0 = vl0;
    return (ad0);
}


namespace {
    inline void err_msg(sLine *dd, const char *what)
    {
        dd->comment_out();
        char *err = new char[strlen(what) + 32];
        sprintf(err, "Warning: misplaced \"%s\".", what);
        dd->set_error(err);
        delete [] err;
    }


    // Determine whether ".if x [= y]" is true, or nonzero.  This
    // relies on the parameter substitution not touching the left side
    // of "assignments".
    //
    bool istrue(const char *cstr)
    {
        lstring::advtok(&cstr);  // skip ".if"

        pnode *pn = Sp.GetPnode(&cstr, true);
        if (!pn)
            return (false);
        sDataVec *dv = Sp.Evaluate(pn);
        delete pn;
        if (dv)
            return (dv->realval(0) != 0.0);
        return (false);
    }
}


void
sLine::var_subst()
{
    CP.VarSubst(&li_line);
}


// Process the .if/.else/.endif lines, removing unused lines from the
// deck.  The .param and .title lines are also handled here.  This is
// to ensure that .param/.title lines in unused blocks are ignored.
//
sParamTab *
sLine::process_conditionals(sParamTab *ptab)
{
    // Stack element.
    struct ifel
    {
        ifel(ifel *n, int l)
            {
                bnext = n;
                blev = l;
                btaken = false;
            }

        ifel *bnext;
        int  blev;      // level of .if
        bool btaken;    // one of the if/elif/else clauses was selected
    };

    ifel *ifcx = 0;     // the stack
    sLine *blhead = 0;  // statement just before block to be deleted

    const char *sbstart = SUBCKT_KW;
    VTvalue vv1;
    if (Sp.GetVar(kw_substart, VTYP_STRING, &vv1))
        sbstart = vv1.get_string();

    const char *sbend = ENDS_KW;
    VTvalue vv2;
    if (Sp.GetVar(kw_subend, VTYP_STRING, &vv2))
        sbend = vv2.get_string();

    int inif = 0, insc = 0, incb = 0;
    sLine *dp = this, *dn;
    for (sLine *dd = li_next; dd; dd = dn) {
        dn = dd->li_next;

        if (lstring::cimatch(CACHE_KW, dd->li_line)) {
            if (incb) {
                err_msg(dd, CACHE_KW);
                *dd->li_line = '*';
            }
            else {
                incb = 1;
                char *t = dd->li_line + strlen(CACHE_KW);
                char *cn = lstring::gettok(&t);
                if (SPcache.inCache(cn)) {
                    // This block is cached, internal lines are
                    // stripped.
                    incb = 2;
                }
                delete [] cn;
            }
        }
        else if (lstring::cimatch(ENDCACHE_KW, dd->li_line)) {
            if (!incb) {
                err_msg(dd, ENDCACHE_KW);
                *dd->li_line = '*';
            }
            incb = 0;
        }
        else if (incb == 2) {
            // In a cached block, strip line.
            dp->li_next = dd->li_next;
            dd->li_next = 0;
            sLine::destroy(dd);
            continue;
        }

        // .param and .title lines are ignored if in subcircuit
        else if (lstring::cimatch(sbstart, dd->li_line) ||
                lstring::cimatch(MACRO_KW, dd->li_line)) {
            insc++;
        }
        else if (lstring::cimatch(sbend, dd->li_line) ||
                lstring::cimatch(EOM_KW, dd->li_line)) {
            if (insc)
                insc--;
        }
        else if (lstring::cimatch(PARAM_KW, dd->li_line)) {
            if (!blhead && !insc) {
                ptab = sParamTab::extract_params(ptab, dd->li_line);
                if (ptab->errString) {
                    dd->li_error = ptab->errString;
                    ptab->errString = 0;
                }
            }
        }
        else if (lstring::cimatch(TITLE_KW, dd->li_line)) {
            if (!blhead && !insc) {
                // replace the title line
                char *t = dd->li_line + strlen(TITLE_KW);
                while (isspace(*t))
                    t++;
                delete [] li_line;
                li_line = lstring::copy(t);
            }
        }

        else if (lstring::cimatch(IF_KW, dd->li_line)) {
            if (ptab)
                ptab->param_subst_all(&dd->li_line);
            if (strchr(dd->li_line, '$'))
                dd->var_subst();
            inif++;

            if (!blhead) {
                ifcx = new ifel(ifcx, inif);
                if (istrue(dd->li_line)) {
                    ifcx->btaken = true;
                    dp->li_next = dn;
                    delete dd;
                    continue;
                }
                blhead = dp;
            }
        }
        else if (lstring::cimatch(ELIF_KW, dd->li_line) ||
                lstring::cimatch(ELSEIF_KW, dd->li_line)) {
            if (ptab)
                ptab->param_subst_all(&dd->li_line);
            if (strchr(dd->li_line, '$'))
                dd->var_subst();
            if (!inif)
                err_msg(dd, ELIF_KW " or " ELSEIF_KW);
            else if (inif == ifcx->blev) {
                if (!blhead)
                    blhead = dp;
                else if (!ifcx->btaken && istrue(dd->li_line)) {
                    ifcx->btaken = true;
                    sLine *tmp = blhead->li_next;
                    blhead->li_next = dn;
                    dd->li_next = 0;
                    sLine::destroy(tmp);
                    dp = blhead;
                    blhead = 0;
                    continue;
                }
            }
        }
        else if (lstring::cimatch(ELSE_KW, dd->li_line)) {
            if (!inif)
                err_msg(dd, ELSE_KW);
            else if (inif == ifcx->blev) {
                if (!blhead)
                    blhead = dp;
                else if (!ifcx->btaken) {
                    ifcx->btaken = true;
                    sLine *tmp = blhead->li_next;
                    blhead->li_next = dn;
                    dd->li_next = 0;
                    sLine::destroy(tmp);
                    dp = blhead;
                    blhead = 0;
                    continue;
                }
            }
        }
        else if (lstring::cimatch(ENDIF_KW, dd->li_line)) {
            if (!inif)
                err_msg(dd, ENDIF_KW);
            else if (inif == ifcx->blev) {
                inif--;
                ifel *x = ifcx;
                ifcx = ifcx->bnext;
                delete x;

                if (!blhead) {
                    dp->li_next = dn;
                    delete dd;
                }
                else {
                    sLine *tmp = blhead->li_next;
                    blhead->li_next = dn;
                    dd->li_next = 0;
                    sLine::destroy(tmp);
                    dp = blhead;
                    blhead = 0;
                }
                continue;
            }
            else
                inif--;
        }
        dp = dd;
    }
    if (inif)
        dp->li_error = lstring::copy("Warning: missing \"" ENDIF_KW "\".");
    while (ifcx) {
        ifel *x = ifcx;
        ifcx = ifcx->bnext;
        delete x;
    }
    return (ptab);
}
// End of sLine functions.


sLibMap::~sLibMap()
{
    sHgen gen(lib_tab, true);
    sHent *ent;
    while ((ent = gen.next()) != 0) {
        delete (sHtab*)ent->data();
        delete ent;
    }
    delete lib_tab;
}


// Return an offset to the start of the tag text in the passed file
// and tag name.  This will map the file if not found in the table,
// for fast subsequent access.
//
// The file is a bare file name, and we have chdir'ed to its directory.
//
long
sLibMap::find(const char *file, const char *name)
{
    if (!file)
        return (LM_NO_FILE);
    if (!lib_tab)
        lib_tab = new sHtab(false);

    char *path = getcwd(0, 0);
    char *fullpath = new char[strlen(path) + strlen(file) + 2];
    char *t = lstring::stpcpy(fullpath, path);
    *t++ = '/';
    strcpy(t, file);
    delete [] path;

    sHtab *h = 0;
    sHent *ent = sHtab::get_ent(lib_tab, fullpath);
    if (!ent) {
        FILE *fp = fopen(file, "r");
        if (!fp) {
            delete [] fullpath;
            return (LM_NO_FILE);
        }
        h = map_lib(fp);
        fclose(fp);
        lib_tab->add(fullpath, h);
    }
    else
        h = (sHtab*)ent->data();

    delete [] fullpath;
    ent = sHtab::get_ent(h, name);
    if (!ent)
        return (LM_NO_NAME);
    return ((long)ent->data());
}


// Private function to map the .lib offsets in a file, returning a
// hash table listing offsets by name.
//
sHtab *
sLibMap::map_lib(FILE *fp)
{
    sHtab *h = new sHtab(true);  // tags are case-insensitive

    char buf[256];
    int c, state = 1;
    while ((c = getc(fp)) != EOF) {
        if (state == 1) {
            if (isspace(c))
                continue;
            if (c != '.') {
                state = 0;
                continue;
            }
            c = getc(fp);
            if (c != 'l' && c != 'L') {
                state = 0;
                continue;
            }
            c = getc(fp);
            if (c != 'i' && c != 'I') {
                state = 0;
                continue;
            }
            c = getc(fp);
            if (c != 'b' && c != 'B') {
                state = 0;
                continue;
            }
            c = getc(fp);
            if (!isspace(c)) {
                state = 0;
                continue;
            }

            char *s = fgets(buf, 256, fp);
            if (s) {
                while (isspace(*s))
                    s++;
                char *t = s;
                while (*t && !isspace(*t))
                    t++;
                *t = 0;
                if (t > s) {
                    unsigned long offs = ftell(fp);
                    h->add(s, (void*)offs);
                }
            }

            state = 1;
            continue;
        }

        if (c == '\n') {
            state = 1;
            continue;
        }
    }
    return (h);
}


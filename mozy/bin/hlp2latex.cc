
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
 * hlp2latex -- A utility to extract latex text blocks stored in the      *
 * help system and create latex files from .tex.in template files.        *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#include "config.h"
#include "miscutil/childproc.h"
#include "miscutil/filestat.h"
#include "help/help_defs.h"
#include "help/help_context.h"
#include "help/help_topic.h"

#include <algorithm>
#include <stdio.h>
#include <signal.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>
#include <dirent.h>
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif
#ifdef WIN32
#include "miscutil/msw.h"
#endif

// Usage: hlp2latex [-p helppath] [-d var ...]  latexdir | file.tex.in ...
//
// This utility creates latex files from latex templates and blocks
// of latex saved in the help system.  By saving both html and latex
// in the same place in the help system, it should be easier to keep
// these in sync.
//
// The argument following literal "-p" is a path to a directory
// containing help files.  It is assumed that these files contain
// latex blocks.  These follow HTML or TEXT blocks:
//     !!HTML or !!TEXT
//     (html or plain text)
//     !!LATEX keyword latexfile
//     (latex text)
// The latexfile can be the name of the intended latex output file.
// This is unused but a word must be present.
//
// Arguments following literal "-d" are variables which become
// defined and can be tested with the !!IFDEF and similar constructs
// in the help text.
//
// Other arguments can be tokenized latex files with .tex.in extensions,
// or a directory containing such files.  In
// these files, lines with a token starting in the first column in
// the form
//     <<keyword helpfile>>
// will be replaced with the latex block tagged with the same keyword
// when the latex files are generated.  The latex file name is the
// same as the source file name with the trailing ".in" stripped.
// Other than the tokens described, text is copied verbatim.  The
// helpfile can be the name of the originating .hlp file.  This is
// not used but a word must be present.
//
// The processing is recursive, so that !!LATEX blocks can contain
// <<key filename>> tokens.  These are expanded in place recursively
// when found.
//
// The !!IFDEF/!!IFNDEF/!!ELSE//!!ENDIF keywords are recognized in
// the !!LATEX blocks and operate the same as in !!HTML blocks, i.e.,
// allowing conditional inclusions.
//
// !!LATEX blocks are terminated by any line that starts with "!!"
// that is not one of the conditionals mentioned above.  It is usually
// not necessary to explicitly terminate as termination occurs naturally
// from following !!SEEALO, !!KEYWORD or similar lines.  Termination
// also occurs naturally at the end of the file.


// The following topic methods override those in help/help_topic.cc,
// which prevents topic.cc and most of the rest of ginterf from being
// linked.  The only file needed from the help system is
// help/help_read.cc.

void HLPtopic::load_text() { }
bool HLPtopic::show_in_window() { return (true); }
HLPtopList *HLPtopList::sort(HLPtopList *thisp) { return(thisp); }
// End of topic functions.


// We also need to fake some HLPcontext things.

HLPcontext::HLPcontext() { memset(this, 0, sizeof(HLPcontext)); }
void HLPcontext::clearVisited() { }


namespace {
    GRpkg _gr_;

    void usage()
    {
        printf("%s\n",
"Usage: hlp2latex help_path outputs\n\n"
"  Outputs are tokenized latex files with .tex.in extension, or a\n"
"  directory containing such files.  The .tex.in files contain tokens\n"
"  in the form <keyword helpfilename> that match !!LATEX blocks found\n"
"  in files from the help_path.  The tokens are replaced by the latex\n"
"  in the .tex output files.\n");
    }

    void init_signals();
}


#define PT_BSIZE 256

class cProcTmpl
{
public:
    struct scope
    {
        scope() { reading = false; }

        bool reading;
    };
    int setup(const char*);
    int do_subst(const char*);

private:
    void do_block(FILE*);
    FILE *is_token();

    FILE    *pt_in_fp;
    FILE    *pt_out_fp;
    scope   pt_scopes[SCOPEDEPTH];
    int     pt_scopedepth;
    char    pt_buf[PT_BSIZE];
};


#define INFILE_EXT ".tex.in"

int
main(int argc, char **argv)
{
    if (argc < 2) {
        GRpkg::self()->ErrPrintf(ET_ERROR, "too few arguments.\n");
        usage();
        return (1);
    }
    init_signals();

    bool want_path = false;
    bool want_define = false;
    for (int ac = 1; ac < argc; ac++) {
        const char *in = argv[ac];
        if (want_path) {
            HLP()->set_path(in, false);
            want_path = false;
            if (HLP()->error_msg()) {
                GRpkg::self()->ErrPrintf(ET_ERROR, "%s", HLP()->error_msg());
                return (1);
            }
            continue;
        }
        if (want_define) {
            HLP()->define(in);
            want_define = false;
            continue;
        }
        if (!strcmp(in, "-p")) {
            want_path = true;
            continue;
        }
        if (!strcmp(in, "-d")) {
            want_define = true;
            continue;
        }

        if (filestat::is_directory(in)) {
            // Process all of the .tex.in files found in the directory,
            // other files are ignored.
            DIR *wdir;
            if ((wdir = opendir(in)) != 0) {
                struct dirent *de;
                while ((de = readdir(wdir)) != 0) {
                    const char *name = de->d_name;
                    int n = strlen(name) - strlen(INFILE_EXT);
                    if (n > 0 && !strcmp(name + n, INFILE_EXT)) {
                        cProcTmpl pt;
                        int r = pt.setup(name);
                        if (!r)
                            r = pt.do_subst(name);
                        if (r != 0)
                            return (r);
                    }
                }
            }
        }
        else {
            // Process the ordinary file.  This will fail if the file
            // does not have a .tex.in extension.
            cProcTmpl pt;
            int r = pt.setup(in);
            if (!r)
                r = pt.do_subst(in);
            if (r != 0)
                return (r);
        }
    }
    return (0);
}


// Open the input latex template file and the output file.  The input
// file name must have a .tex.in extension.  The output file name is the
// infile name with the ".in" is stripped.  That is, if infile is
// myfile.tex.in, the creatred file will be myfile.tex.  Zero is
// returned on success.
//
int
cProcTmpl::setup(const char *infile)
{
    if (!infile) {
        GRpkg::self()->ErrPrintf(ET_ERROR,
            "null filename passed to do_subst.\n");
        return (101);
    }
    int n = strlen(infile) - strlen(INFILE_EXT);
    if (n < 1 || strcmp(infile + n, INFILE_EXT)) {
        GRpkg::self()->ErrPrintf(ET_ERROR,
            "input file %s does not have %s extension.\n",
            infile, INFILE_EXT);
        return (101);
    }
    pt_in_fp = fopen(infile, "r");
    if (!pt_in_fp) {
        GRpkg::self()->ErrPrintf(ET_ERROR,
            "can't open input %s.\n", infile);
        return (102);
    }
    char *out = lstring::copy(infile);
    *strrchr(out, '.') = 0;  // strip ".in"
    pt_out_fp = fopen(out, "w");
    if (!pt_out_fp) {
        fclose(pt_in_fp);
        GRpkg::self()->ErrPrintf(ET_ERROR, "can't open output %s.\n", out);
        delete [] out;
        return (103);
    }

    for (int i = 0; i < SCOPEDEPTH; i++)
        pt_scopes[i].reading = false;
    pt_scopedepth = -1;
    return (0);
}


// Read the template infile and copy to output, expanding any
// <<kw fname>> tokens found, recursively.
int
cProcTmpl::do_subst(const char *infile)
{
    printf("Processing %s\n", infile);
    while (const char *s = fgets(pt_buf, PT_BSIZE, pt_in_fp)) {
        // The substittution token starts with "<<" in the first
        // column.  This may be followed by space, then the keyword.
        // The keyword must start with a letter, number, or period.
        FILE *xp = is_token();
        if (xp) {
            do_block(xp);
            fclose(xp);
            continue;
        }
        fputs(pt_buf, pt_out_fp);
    }
    fclose(pt_out_fp);
    fclose(pt_in_fp);
    // Error return not used.  An unresolved keyword is not an error,
    // a warning message is output.
    return (0);
}


// Copy the latex from the passed file pointer to the output file.  This
// takes into account conditional directives, and the block will
// terminate on a leading "!!" or end of file.  The block can contain
// <<key filename>> directives which are processed recursively.  The
// filename is ignored but must be present.
//
void
cProcTmpl::do_block(FILE *xp)
{
    if (!xp)
        return;
    int pmode = 0;;
    while (const char *s = fgets(pt_buf, PT_BSIZE, xp)) {
        // Do not break out of the processing when !!PROTECT is active.
        if (lstring::ciprefix("!!PROTECT", s)) {
            pmode++;
            continue;
        }
        if (lstring::ciprefix("!!UNPROTECT", s) && pmode) {
            pmode--;
            continue;
        }
        if (lstring::ciprefix(HLP_IFDEF, s)) {
            if (pt_scopedepth < SCOPEDEPTH-1)
                pt_scopedepth++;
            lstring::advtok(&s);
            char *def = lstring::gettok(&s);
            pt_scopes[pt_scopedepth].reading = HLP()->isdef(def);
            delete [] def;
            continue;
        }
        if (lstring::ciprefix(HLP_IFNDEF, s)) {
            if (pt_scopedepth < SCOPEDEPTH-1)
                pt_scopedepth++;
            lstring::advtok(&s);
            char *def = lstring::gettok(&s);
            pt_scopes[pt_scopedepth].reading = !HLP()->isdef(def);
            delete [] def;
            continue;
        }
        if (lstring::ciprefix(HLP_ELSE, s)) {
            if (pt_scopedepth >= 0) {
                if (!pt_scopes[pt_scopedepth].reading)
                    pt_scopes[pt_scopedepth].reading = true;
                else
                    pt_scopes[pt_scopedepth].reading = false;
            }
            continue;
        }
        if (lstring::ciprefix(HLP_ENDIF, s)) {
            if (pt_scopedepth >= 0)
                pt_scopedepth--;
            continue;
        }
        bool reading = true;
        for (int j = 0; j <= pt_scopedepth; j++) {
            if (!pt_scopes[j].reading) {
                reading = false;
                break;
            }
        }
        if (!reading)
            continue;

        if (!pmode) {
            if (s[0] == '!' && s[1] == '!')
                break;  // End of a block.
        }
        FILE *yp = is_token();
        if (yp) {
            do_block(yp);
            fclose(yp);
            continue;
        }
        fputs(s, pt_out_fp);
    }
}


// Return a file pointer if we find a <<key filename>> token starting at
// the first character of the buffer and the key is resolved.  The
// filename is ignored but must be present.
//
FILE *
cProcTmpl::is_token()
{
    const char *s = pt_buf;
    if (s[0] == '<' && s[1] == '<') {
        s += 2;
        while (isspace(*s))
            s++;
        char *e = strchr(pt_buf, '>');
        if (e && e[1] == '>') {
            *e = 0;
            char *kw = lstring::gettok(&s);
            char *fname = lstring::gettok(&s);
            if (fname && kw) {
                delete [] fname;
                FILE *xp = HLP()->open_latex(kw, 0);
                if (!xp) {
                    GRpkg::self()->ErrPrintf(ET_WARN,
                        "unresolved keyword %s.\n", kw);
                    delete [] kw;
                    return (0);
                }
                delete [] kw;
                return (xp);
            }
            delete [] kw;
            delete [] fname;
        }
        GRpkg::self()->ErrPrintf(ET_WARN,
            "token syntax error in %s.\n", pt_buf);
    }
    return (0);
}


namespace {
    // The signal handler.  We probably don't need this.
    //
    void
    sig_hdlr(int sig)
    {
        signal(sig, sig_hdlr);  // reset for SysV

        if (sig == SIGSEGV) {
            fprintf(stderr, "Fatal internal error: segmentation violation.\n");
            signal(SIGINT, SIG_DFL);
            raise(SIGINT);
        }
#ifdef SIGBUS
        else if (sig == SIGBUS) {
            fprintf(stderr, "Fatal internal error: bus error.\n");
            signal(SIGINT, SIG_DFL);
            raise(SIGINT);
        }
#endif
        else if (sig == SIGILL) {
            fprintf(stderr, "Fatal internal error: illegal instruction.\n");
            signal(SIGINT, SIG_DFL);
            raise(SIGINT);
        }
        else if (sig == SIGFPE) {
            fprintf(stderr, "Warning: floating point exception.\n");
            return;
        }
#ifdef SIGCHLD
        else if (sig == SIGCHLD) {
            Proc()->SigchldHandler();
            return;
        }
#endif
    }


    void
    init_signals()
    {
        signal(SIGINT, SIG_IGN);
        signal(SIGSEGV, sig_hdlr);
#ifdef SIGBUS
        signal(SIGBUS, sig_hdlr);
#endif
        signal(SIGILL, sig_hdlr);
        signal(SIGFPE, sig_hdlr);
#ifdef SIGCHLD
        signal(SIGCHLD, sig_hdlr);
#endif
    }
}


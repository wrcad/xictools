
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
 * Help System Files                                                      *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#include "config.h"
#include "help_defs.h"
#include "help_context.h"
#include "help_topic.h"
#include "miscutil/pathlist.h"
#include "miscutil/filestat.h"
#include "miscutil/hashfunc.h"

#include <string.h>
#include <sys/types.h>
#include <dirent.h>

#ifdef HAVE_REGEX_H
#include <regex.h>
#else
#include "regex/regex.h"
#endif


namespace {

    // The help system main class
    cHelp _hlp_;

    char *my_fgets(char*, int, FILE*);

    // error strings
    const char *msg_nodb  = "no path to database.\n";
    const char *msg_notop = "no top level topic.\n";
    const char *msg_nox   = "can't open display.\n";
    char msg_noent[128];

    // Used in cHelp::hlp_read.
    struct scope
    {
        scope() { reading = false; }

        bool reading;
    };


    // Hashing function.
    //
#define NUMHASH 32
    //
    inline int
    h_hash(const char *c)
    {
        return (string_hash(c, NUMHASH - 1));
    }
}


cHelp *cHelp::instancePtr = 0;

cHelp::cHelp()
{
    if (instancePtr) {
        fprintf(stderr, "Singleton class cHelp already instantiated.\n");
        exit(1);
    }
    instancePtr = this;

    hlp_context = 0;
    hlp_dirlist = 0;
    hlp_errmsg = 0;
    hlp_initxpos = START_XPOS;
    hlp_initypos = START_YPOS;
    hlp_title = 0;
    hlp_defines = 0;
    hlp_header = 0;
    hlp_footer = 0;
    hlp_main_tag = 0;
    hlp_tag_text = 0;
    hlp_text_hdlr = 0;

    hlp_usex = true;
    hlp_multiwin = false;
    hlp_no_file_fonts = false;
    hlp_fifo_start = false;
    hlp_debug = false;

    hlp_callback = 0;

    // Constructor will call back and initialize hlp_callback.
    hlp_context = new HLPcontext;
}


// Private static error exit.
//
void
cHelp::on_null_ptr()
{
    fprintf(stderr, "Singleton class cHelp used before instantiated.\n");
    exit(1);
}


// Set the path of the directory containing database files.
// This must be called before accessing the database.  Directories not
// in the current path are marked inactive, unless merge is true.
//
void
cHelp::set_path(const char *path, bool merge)
{
    HLPdirList *dl = 0, *dl0 = 0;
    if (pathlist::is_empty_path(path))
        path = ".";
    pathgen pg(path);
    char *p;
    while ((p = pg.nextpath(false)) != 0) {
        if (!dl0)
            dl0 = dl = recycle_dir(p);
        else {
            dl->next = recycle_dir(p);
            dl = dl->next;
        }
        delete [] p;
    }
    if (!merge) {
        for (HLPdirList *dd = hlp_dirlist; dd; dd = dd->next)
            dd->hd_inactive = true;
    }
    if (dl)
        dl->next = hlp_dirlist;
    hlp_dirlist = dl0;
}


// Return true if the help path has been set, with the path in *path if
// path is not 0.  Otherwise return false.
//
bool
cHelp::get_path(char **path)
{
    if (!hlp_dirlist)
        return (false);
    if (!path)
        return (true);
    int len = 0;
    for (HLPdirList *dd = hlp_dirlist; dd; dd = dd->next) {
        if (dd->hd_inactive)
            continue;
        len += strlen(dd->hd_dir) + 1;
    }
    if (!len)
        return (false);
    *path = new char[len + 1];
    char *t = *path;
    for (HLPdirList *dd = hlp_dirlist; dd; dd = dd->next) {
        if (dd->hd_inactive)
            continue;
        if (t != *path)
            *t++ = PATH_SEP;
        strcpy(t, dd->hd_dir);
        while (*t)
            t++;
    }
    return (true);
}


// Throw out all of the cached file entries.
//
void
cHelp::rehash()
{
    for (HLPdirList *dd = hlp_dirlist; dd; dd = dd->next)
        dd->reset();
    context()->clearVisited();
}


// Display the help text for each of the words in the list.
//
void
cHelp::list(HLPwords *hw)
{
    if (!hlp_dirlist) {
        hlp_errmsg = msg_nodb;
        return;
    }
    if (hw == 0)
        word(0);
    else {
        while (hw) {
            word(hw->word);
            hw = hw->next;
        }
    }
}


// Display the help text for word.
//
void
cHelp::word(const char *keyword_in)
{
    char buf[256];
    buf[0] = 0;
    char *keyword = buf;
    hlp_errmsg = 0;
    if (keyword_in) {
        strcpy(buf, keyword_in);
        char *s = buf + strlen(buf) - 1;
        while (isspace(*s))
            *s-- = '\0';
    }
    const char *a = alias(keyword);
    if (a)
        strcpy(buf, a);

    HLPtopic *top = 0;
    if (hlp_callback) {
        // Application callback, this can modify the word, or supply
        // a topic, or abort if true is returned.  The callback should
        // call read().
        //
        if ((*hlp_callback)(&keyword, &top))
            return;
    }
    if (!top)
        top = read(keyword);
    if (!top) {
        if (keyword && *keyword) {
            snprintf(msg_noent, sizeof(msg_noent),
                "topic not found: %s.\n", keyword);
            hlp_errmsg = msg_noent;
        }
        else
            hlp_errmsg = msg_notop;
        return;
    }
    if (hlp_usex) {
        if (!top->show_in_window())
            hlp_errmsg = msg_nox;
    }
    else if (hlp_text_hdlr)
        hlp_text_hdlr->display(top);
}


// Main access code for the help database.  Returns the topic struct
// associated with word, or 0 if not found in the database.
//
HLPtopic *
cHelp::read(const char *keyword)
{
    hlp_errmsg = 0;
    if (!hlp_dirlist) {
        hlp_errmsg = msg_nodb;
        return (0);
    }
    HLPent *bb;
    FILE *fp;
    fp = open(keyword, &bb);
    if (!fp)
        return (0);
    HLPtopic *top = new HLPtopic(keyword, bb->title, bb->tag, true);
    top->set_is_html(bb->is_html);
    bool latex = false;
    bool seealso = false;
    bool subtopics = false;

    scope scopes[SCOPEDEPTH];
    int scopedepth = -1;

    char buf[256], tbuf[64], *s, *t;
    HLPwords *hw = 0;
    bool prot = false;
    while ((s = my_fgets(buf, 256, fp)) != 0) {

        if (prot) {
            if (lstring::ciprefix(HLP_UNPROTECT, s))
                prot = false;
            continue;
        }
        if (lstring::ciprefix(HLP_PROTECT, s)) {
            prot = true;
            continue;
        }

        if (lstring::ciprefix(HLP_COMMENT, s))
            continue;

        if (lstring::ciprefix(HLP_IFDEF, s)) {
            if (scopedepth < SCOPEDEPTH-1)
                scopedepth++;
            lstring::advtok(&s);
            char *def = lstring::gettok(&s);
            scopes[scopedepth].reading = isdef(def);
            delete [] def;
            continue;
        }
        if (lstring::ciprefix(HLP_IFNDEF, s)) {
            if (scopedepth < SCOPEDEPTH-1)
                scopedepth++;
            lstring::advtok(&s);
            char *def = lstring::gettok(&s);
            scopes[scopedepth].reading = !isdef(def);
            delete [] def;
            continue;
        }
        if (lstring::ciprefix(HLP_ELSE, s)) {
            if (scopedepth >= 0) {
                if (!scopes[scopedepth].reading)
                    scopes[scopedepth].reading = true;
                else
                    scopes[scopedepth].reading = false;
            }
            continue;
        }
        if (lstring::ciprefix(HLP_ENDIF, s)) {
            if (scopedepth >= 0)
                scopedepth--;
            continue;
        }
        if (scopedepth >= 0 && !scopes[scopedepth].reading)
            continue;

        if (lstring::ciprefix(HLP_KEYWORD, s) ||
                lstring::ciprefix(HLP_HEADER, s) ||
                lstring::ciprefix(HLP_FOOTER, s))
            break;
        if (lstring::ciprefix(HLP_MAINTAG, s) ||
                lstring::ciprefix(HLP_TAG, s) ||
                lstring::ciprefix(HLP_TAGTEXT, s))
            break;

        if (lstring::ciprefix(HLP_INCLUDE, s)) {
            if (hw) {
                hw->next = get_include(s);
                while (hw->next) {
                    top->register_word(hw->word);
                    hw = hw->next;
                }
            }
            else {
                hw = get_include(s);
                if (hw) {
                    top->set_words(hw);
                    while (hw->next) {
                        top->register_word(hw->word);
                        hw = hw->next;
                    }
                }
            }
            continue;
        }
        if (lstring::ciprefix(HLP_LATEX, s)) {
            latex = true;
            seealso = false;
            subtopics = false;
            continue;
        }
        if (lstring::ciprefix(HLP_SEEALSO, s)) {
            latex = false;
            seealso = true;
            subtopics = false;
            continue;
        }
        if (lstring::ciprefix(HLP_SUBTOPICS, s)) {
            latex = false;
            subtopics = true;
            seealso = false;
            continue;
        }
        if (lstring::ciprefix(HLP_REDIR, s))
            continue;

        if (seealso || subtopics) {

            if (*s == '#' || *s == '*')
                continue;
            while (isspace(*s))
                s++;
            if (!*s)
                continue;

            t = s + strlen(s)-1;
            while (*t <= ' ')
                *t-- = '\0';

            t = s;
            while ((s = lstring::gettok(&t)) != 0) {
                bb = get_entry(s);
                if (bb) {
                    if (seealso)
                        top->add_seealso(bb->title, s);
                    else if (subtopics)
                        top->add_subtopic(bb->title, s);
                    delete [] s;
                    continue;
                }
                const char *a = alias(s);
                if (a) {
                    bb = get_entry(a);
                    if (bb) {
                        if (seealso)
                            top->add_seealso(bb->title, s);
                        else if (subtopics)
                            top->add_subtopic(bb->title, s);
                        delete [] s;
                        continue;
                    }
                }
                else
                    a = s;

                const char *anc = strchr(a, '#');
                if (anc) {
                    char *tmp = lstring::copy(a);
                    tmp[anc - a] = 0;
                    bb = get_entry(tmp);
                    delete [] tmp;
                    if (bb) {
                        snprintf(tbuf, sizeof(tbuf), "%s: %s", bb->title, s);
                        if (seealso)
                            top->add_seealso(tbuf, s);
                        else if (subtopics)
                            top->add_subtopic(tbuf, s);
                        delete [] s;
                        continue;
                    }
                }
                if (hlp_debug) {
                    snprintf(tbuf, sizeof(tbuf), "unknown: %s", s);
                    if (seealso)
                        top->add_seealso(tbuf, s);
                    else if (subtopics)
                        top->add_subtopic(tbuf, s);
                }
                delete [] s;
            }
            continue;
        }
        if (!latex) {
            if ((s = strchr(buf,'\n')) != 0)
                *s = '\0';
            if (hw) {
                hw->next = new HLPwords(buf, hw);
                hw = hw->next;
            }
            else {
                hw = new HLPwords(buf, 0);
                top->set_words(hw);
            }
            top->register_word(buf);
        }
    }
    fclose(fp);

    // look for <body> tag
    if (top->is_html()) {
        top->set_need_body(true);
        int i = 0;
        for (hw = top->get_words(); hw && i < 50; i++, hw = hw->next) {
            char *stmp = strchr(hw->word, '<');
            while (stmp) {
                stmp++;
                if (lstring::ciprefix("body", stmp) || is_html_frame(stmp)) {
                    top->set_need_body(false);
                    break;
                }
                else if (is_html_body(stmp))
                    break;
                stmp = strchr(stmp, '<');
            }
        }
    }

    top->sort_subtopics();
    top->sort_seealso();

    return (top);
}


// Return true if the word is a database keyword, false otherwise.
//
bool
cHelp::is_keyword(const char *keyword)
{
    if (!hlp_dirlist)
        return (false);
    if (get_entry(keyword))
        return (true);
    if (alias(keyword))
        return (true);
    return (false);
}


// Return a redirection for keyword.
//
const char*
cHelp::alias(const char *keyword)
{
    if (!keyword || !*keyword)
        return (0);
    int j = h_hash(keyword);
    for (HLPdirList *dl = hlp_dirlist; dl; dl = dl->next) {
        if (dl->hd_inactive || dl->hd_empty)
            continue;
        if (!dl->hd_base)
            dl->init();
        if (!dl->hd_base) {
            dl->hd_empty = true;
            continue;
        }
        if (!dl->hd_rdbase)
            continue;
        for (HLPrdir *h = dl->hd_rdbase[j]; h; h = h->next) {
            if (!strcmp(keyword, h->key))
                return (h->alias);
        }
    }
    return (0);
}


// Return a topic struct in response to a keyword search for regular
// expression target.  Matches are listed in the subtopics.
//
HLPtopic *
cHelp::search(const char *target)
{
    char buf[256];
    HLPtopic *top = new HLPtopic(target, "search");
    HLPwords *hw = scan(target);
    for (HLPwords *ww = hw; ww; ww = ww->next) {
        HLPent *bb = get_entry(ww->word);
        if (bb)
            top->add_seealso(bb->title, ww->word);
    }
    HLPwords::destroy(hw);
    top->sort_seealso();
    int i;
    HLPtopList *tl;
    for (i = 0, tl = top->seealso(); tl; i++, tl = tl->next()) ;
    snprintf(buf, sizeof(buf), "Keyword search for %s : %d entries found.",
        target, i);
    top->set_words(new HLPwords(buf, 0));
    top->register_word(buf);
    return (top);
}


// Define token for conditional text read (!!IFDEF).
//
void
cHelp::define(const char *token)
{
    if (!isdef(token)) {
        HLPwords *h = new HLPwords(token, 0);
        h->next = hlp_defines;
        hlp_defines = h;
    }
}


// Undefine token for conditional text read (!!IFDEF).
//
void
cHelp::undef(const char *token)
{
    if (isdef(token)) {
        HLPwords *hp = 0;
        for (HLPwords *h = hlp_defines; h; h = h->next) {
            if (!strcmp(h->word, token)) {
                if (!hp)
                    hlp_defines = h->next;
                else
                    hp->next = h->next;
                delete h;
                return;
            }
            hp = h;
        }
    }
}


// Return true if token has been defined.
//
bool
cHelp::isdef(const char *token)
{
    if (token) {
        for (HLPwords *h = hlp_defines; h; h = h->next)
            if (!strcmp(h->word, token))
                return (true);
    }
    return (false);
}


// Access the database for keyword.  Return the offset file pointer,
// and the database structure.
//
FILE *
cHelp::open(const char *keyword, HLPent **pb)
{
    if (pb)
        *pb = 0;
    HLPent *bb = get_entry(keyword);
    FILE *fp = 0;
    if (bb) {
        fp = fopen(bb->filename, "rb");
        if (!fp) {
            GRpkg::self()->Perror(bb->filename);
            return (0);
        }
        if (pb)
            *pb = bb;
        fseek(fp, bb->offset, 0);
    }
    return (fp);
}


// Return the internal database struct for keyword.  No file access.
//
HLPent *
cHelp::get_entry(const char *keyword)
{
    if (!keyword)
        return (0);
    int j = h_hash(keyword);
    for (HLPdirList *dl = hlp_dirlist; dl; dl = dl->next) {
        if (dl->hd_inactive || dl->hd_empty)
            continue;
        if (!dl->hd_base)
            dl->init();
        if (!dl->hd_base) {
            dl->hd_empty = true;
            continue;
        }
        for (HLPent *bb = dl->hd_base[j]; bb; bb = bb->next) {
            if (!strcmp(keyword, bb->keyword))
                return (bb);
        }
    }
    return (0);
}


// Access the latex block for keyword.  Return the offset file pointer,
// and the database structure.
//
FILE *
cHelp::open_latex(const char *keyword, HLPent **pb)
{
    if (pb)
        *pb = 0;
    HLPent *bb = get_latex_entry(keyword);
    FILE *fp = 0;
    if (bb) {
        fp = fopen(bb->filename, "rb");
        if (!fp) {
            GRpkg::self()->Perror(bb->filename);
            return (0);
        }
        if (pb)
            *pb = bb;
        fseek(fp, bb->offset, 0);
    }
    return (fp);
}


// Return the internal latex block struct for keyword.  No file access.
//
HLPent *
cHelp::get_latex_entry(const char *keyword)
{
    if (!keyword)
        return (0);
    int j = h_hash(keyword);
    for (HLPdirList *dl = hlp_dirlist; dl; dl = dl->next) {
        if (dl->hd_inactive || dl->hd_empty)
            continue;
        if (!dl->hd_base)
            dl->init();
        if (!dl->hd_base) {
            dl->hd_empty = true;
            continue;
        }
        if (!dl->hd_latex)
            continue;
        for (HLPent *bb = dl->hd_latex[j]; bb; bb = bb->next) {
            if (!strcmp(keyword, bb->keyword))
                return (bb);
        }
    }
    return (0);
}


// Return a list of topic keywords of entries with a line that matches
// the target regular expression.  The entire database is searched.
//
HLPwords *
cHelp::scan(const char *target)
{
    HLPwords **tbase = new HLPwords*[NUMHASH];
    int i;
    // use hashing to check for duplicates efficiently
    for (i = 0; i < NUMHASH; i++)
        tbase[i] = 0;
    for (struct HLPdirList *dd = hlp_dirlist; dd; dd = dd->next) {
        if (dd->hd_inactive)
            continue;
        DIR *wdir;
        if (!(wdir = opendir(dd->hd_dir)))
            continue;
        dirent *de;
        while ((de = readdir(wdir)) != 0) {
            char *s;
            if ((s = strrchr(de->d_name, '.')) && !strcmp(s+1, "hlp")) {
                char *path = pathlist::mk_path(dd->hd_dir, de->d_name);
                FILE *fp = fopen(path, "rb");
                delete [] path;
                if (!fp)
                    continue;
                HLPwords *hx = scan_file(fp, target);
                fclose(fp);

                // store words in the hash table
                HLPwords *hn;
                for (HLPwords *h = hx; h; h = hn) {
                    hn = h->next;
                    i = h_hash(h->word);
                    HLPwords *hh;
                    for (hh = tbase[i]; hh; hh = hh->next) {
                        if (!strcmp(h->word, hh->word))
                            // duplicate
                            break;
                    }
                    if (!hh) {
                        h->next = tbase[i];
                        if (tbase[i])
                            tbase[i]->prev = h;
                        h->prev = 0;
                        tbase[i] = h;
                    }
                    else
                        delete h;
                }
            }
        }
        closedir(wdir);
    }
    HLPwords *h0 = 0;
    for (i = 0; i < NUMHASH; i++) {
        if (tbase[i]) {
            HLPwords *h;
            for (h = tbase[i]; h->next; h = h->next) ;
            h->next = h0;
            if (h0)
                h0->prev = h;
            h0 = tbase[i];
        }
    }
    delete [] tbase;
    return (h0);
}


// Return a list of topic keywords which have a line that matches
// the target regular expression.
//
HLPwords *
cHelp::scan_file(FILE *fp, const char *target)
{
    char *s, *kw = 0, buf[256];

#define FLAGS  REG_EXTENDED | REG_ICASE | REG_NOSUB
    regex_t preg;
    if (regcomp(&preg, target, FLAGS)) {
        fprintf(stderr, "regcomp error\n");
        return (0);
    }

    bool found = true;
    bool keyw = false;
    bool prot = false;
    HLPwords *h0 = 0, *hw = 0;
    while ((s = my_fgets(buf, 256, fp)) != 0) {
        if (prot) {
            if (lstring::ciprefix(HLP_UNPROTECT, s))
                prot = false;
            continue;
        }
        if (lstring::ciprefix(HLP_PROTECT, s)) {
            prot = true;
            continue;
        }
        if (lstring::prefix(HLP_COMMENT, s))
            continue;
        if (lstring::prefix(HLP_REDIR, s))
            continue;
        if (lstring::ciprefix(HLP_KEYWORD, s)) {
            keyw = true;
            continue;
        }
        if (lstring::ciprefix(HLP_HEADER, s) ||
                lstring::ciprefix(HLP_FOOTER, s) ||
                lstring::ciprefix(HLP_MAINTAG, s) ||
                lstring::ciprefix(HLP_TAG, s) ||
                lstring::ciprefix(HLP_TAGTEXT, s)) {
            delete [] kw;
            kw = 0;
            found = true;
            keyw = false;
            continue;
        }
        if (keyw) {
            delete [] kw;
            kw = lstring::gettok(&s);
            found = false;
            keyw = false;
            continue;
        }

        if (!found && !regexec(&preg, s, 0, 0, 0)) {
            if (hw) {
                hw->next = new HLPwords(kw, hw);
                hw = hw->next;
            }
            else
                h0 = hw = new HLPwords(kw, 0);
            found = true;
        }
    }
    delete [] kw;

    regfree(&preg);
    return (h0);
}


// Either unlink and return the list element for dir if it exists,
// or create a new one.
//
HLPdirList *
cHelp::recycle_dir(const char *dir)
{
    HLPdirList *dp = 0, *dl;
    for (dl = hlp_dirlist; dl; dl = dl->next) {
        if (!strcmp(dl->hd_dir, dir)) {
            if (!dp)
                hlp_dirlist = dl->next;
            else
                dp->next = dl->next;
            dl->hd_inactive = false;
            return (dl);
        }
        dp = dl;
    }
    dl = new HLPdirList(dir);
    return (dl);
}


// Return true if the file is found in the active directory list.  We
// don't allow more that one file of the same name to be read.  The
// reason for this is that if merging the help for two applications
// where some of the files are identical, we would get a lot of
// duplicate tag errors.
//
bool
cHelp::lookup_file(const char *fname)
{
    for (HLPdirList *dd = hlp_dirlist; dd; dd = dd->next) {
        if (dd->hd_inactive)
            continue;
        for (stringlist *ff = dd->hd_files; ff; ff = ff->next) {
            if (!strcmp(lstring::strip_path(ff->string), fname))
                return (true);
        }
    }
    return (false);
}


// Open the file named in the second token of str, and read the contents
// into the returned list of words.
//
HLPwords *
cHelp::get_include(const char *str)
{
    char *tok = lstring::gettok(&str);
    delete [] tok;  // skip "include"
    tok = lstring::getqtok(&str);
    str = pathlist::expand_path(tok, false, false);
    delete [] tok;
    if (!str)
        return (0);

    FILE *fp = 0;
    HLPdirList *d = 0;
    char *path = 0;
    if (lstring::is_rooted(str))
        fp = fopen(str, "r");
    else {
        for (d = hlp_dirlist; d; d = d->next) {
            path = pathlist::mk_path(d->hd_dir, str);
            fp = fopen(path, "r");
            if (fp)
                break;
            delete [] path;
            path = 0;
        }
    }
    HLPwords *hw = 0, *he = 0;
    if (fp) {
        // if there was relative path passed, add it to the directory
        // search list so we can find the gif's
        if (d && lstring::strdirsep(str)) {
            char *s = lstring::strrdirsep(path);
            *s = 0;
            d = recycle_dir(path);
            d->next = hlp_dirlist;
            hlp_dirlist = d;
        }
        char buf[512];
        while (fgets(buf, 512, fp) != 0) {
            if (!he)
               hw = he = new HLPwords(buf, hw);
            else {
               he->next = new HLPwords(buf, hw);
               he = he->next;
            }
        }
        fclose(fp);
    }
    delete [] path;
    delete [] str;
    return (hw);
}
// End of cHelp functions.


HLPdirList::~HLPdirList()
{
    delete [] hd_dir;
    stringlist::destroy(hd_files);
    stringlist::destroy(hd_tags);
    if (hd_base) {
        for (int i = 0; i < NUMHASH; i++)
            HLPent::destroy(hd_base[i]);
        delete [] hd_base;
    }
    if (hd_latex) {
        for (int i = 0; i < NUMHASH; i++)
            HLPent::destroy(hd_latex[i]);
        delete [] hd_latex;
    }
}


// Create the internal database from the xxx.hlp files found in
// the directory.  Structures containing the title, offsets, and
// filenames of the entries are kept in a hash indexed array of
// linked lists.  If a file of the same name has already been
// read, skip it.
//
void
HLPdirList::init()
{
    DIR *wdir;
    if (!(wdir = opendir(hd_dir))) {
        GRpkg::self()->ErrPrintf(ET_ERROR,
            "can't open help database directory %s.\n", hd_dir);
        return;
    }
    dirent *de;
    while ((de = readdir(wdir)) != 0) {
        char *s;
        if ((s = strrchr(de->d_name, '.')) && !strcmp(s+1, "hlp")) {
            if (HLP()->lookup_file(de->d_name))
                continue;
            read_file(de->d_name);
        }
    }
    closedir(wdir);
}


// Undo the initialization.
//
void
HLPdirList::reset()
{
    stringlist::destroy(hd_files);
    hd_files = 0;
    if (hd_base) {
        for (int i = 0; i < NUMHASH; i++)
            HLPent::destroy(hd_base[i]);
        delete [] hd_base;
        hd_base = 0;
    }
    if (hd_latex) {
        for (int i = 0; i < NUMHASH; i++)
            HLPent::destroy(hd_latex[i]);
        delete [] hd_latex;
        hd_latex = 0;
    }
    if (hd_rdbase) {
        for (int i = 0; i < NUMHASH; i++)
            HLPrdir::destroy(hd_rdbase[i]);
        delete [] hd_rdbase;
        hd_rdbase = 0;
    }
    hd_empty = false;
}


// Read the database file, fill in the data structures, and link them
// into the database.
//
void
HLPdirList::read_file(const char *fnamein)
{
    hd_files = new stringlist(pathlist::mk_path(hd_dir, fnamein), hd_files);
    char *fname = hd_files->string;
    FILE *fp = fopen(fname, "rb");
    if (!fp) {
        GRpkg::self()->Perror(fnamein);
        return;
    }
    if (!hd_base) {
        hd_base = new HLPent*[NUMHASH];
        for (int i = 0; i < NUMHASH; i++)
            hd_base[i] = 0;
    }

    scope scopes[SCOPEDEPTH];
    int scopedepth = -1;

    char buf[256];
    char *s, *kw = 0, *ti = 0;
    char *hstr = 0, *fstr = 0;
    char *maintag = 0, *tag = 0;;
    char *ttext = 0;
    bool keyword = false, title = false;
    bool header = false, footer = false;
    bool tagtext = false;
    bool prot = false;
    while ((s = my_fgets(buf, 256, fp)) != 0) {
        if (prot) {
            if (lstring::ciprefix(HLP_UNPROTECT, s))
                prot = false;
            continue;
        }
        if (lstring::ciprefix(HLP_PROTECT, s)) {
            prot = true;
            continue;
        }

        if (lstring::prefix(HLP_COMMENT, s))
            continue;

        if (lstring::ciprefix(HLP_IFDEF, s)) {
            if (scopedepth < SCOPEDEPTH-1)
                scopedepth++;
            lstring::advtok(&s);
            char *def = lstring::gettok(&s);
            scopes[scopedepth].reading = HLP()->isdef(def);
            delete [] def;
            continue;
        }
        if (lstring::ciprefix(HLP_IFNDEF, s)) {
            if (scopedepth < SCOPEDEPTH-1)
                scopedepth++;
            lstring::advtok(&s);
            char *def = lstring::gettok(&s);
            scopes[scopedepth].reading = !HLP()->isdef(def);
            delete [] def;
            continue;
        }
        if (lstring::ciprefix(HLP_ELSE, s)) {
            if (scopedepth >= 0) {
                if (!scopes[scopedepth].reading)
                    scopes[scopedepth].reading = true;
                else
                    scopes[scopedepth].reading = false;
            }
            continue;
        }
        if (lstring::ciprefix(HLP_ENDIF, s)) {
            if (scopedepth >= 0)
                scopedepth--;
            continue;
        }
        if (scopedepth >= 0 && !scopes[scopedepth].reading)
            continue;

        if (lstring::ciprefix(HLP_MAINTAG, s)) {
            lstring::advtok(&s);
            if (!maintag)
                maintag = lstring::gettok(&s);
            continue;
        }
        if (lstring::ciprefix(HLP_TAG, s) &&
                !lstring::ciprefix(HLP_TAGTEXT, s)) {
            lstring::advtok(&s);
            tag = lstring::gettok(&s);
            if (tag) {
                bool found = false;
                for (stringlist *sl = hd_tags; sl; sl = sl->next) {
                    if (!strcmp(tag, sl->string)) {
                        delete [] tag;
                        tag = sl->string;
                        found = true;
                        break;
                    }
                }
                if (!found)
                    hd_tags = new stringlist(tag, hd_tags);
            }
            continue;
        }

        if (lstring::ciprefix(HLP_REDIR, s)) {
            lstring::advtok(&s);
            char *okw = lstring::gettok(&s);
            char *nkw = lstring::gettok(&s);
            if (okw && nkw) {
                int j = h_hash(okw);
                HLPent *bb = 0;
                for (bb = hd_base[j]; bb; bb = bb->next) {
                    if (!strcmp(okw, bb->keyword))
                        break;
                }
                if (bb) {
                    GRpkg::self()->ErrPrintf(ET_WARN,
                        "help database keyword clash for %s.\n", okw);
                    delete [] okw;
                    delete [] nkw;
                    continue;
                }
                if (!hd_rdbase) {
                    hd_rdbase = new HLPrdir*[NUMHASH];
                    for (int i = 0; i < NUMHASH; i++)
                        hd_rdbase[i] = 0;
                }
                HLPrdir *h = 0;
                for (h = hd_rdbase[j]; h; h = h->next)
                    if (!strcmp(okw, h->key))
                        break;
                if (h) {
                    GRpkg::self()->ErrPrintf(ET_WARN,
                        "help database keyword clash for %s.\n", okw);
                    delete [] okw;
                    delete [] nkw;
                    continue;
                }
                hd_rdbase[j] = new HLPrdir(okw, nkw, hd_rdbase[j]);
            }
            else {
                delete [] okw;
                delete [] nkw;
            }
            continue;
        }
        if (lstring::ciprefix(HLP_KEYWORD, s)) {
            keyword = true;
            title = false;
            header = false;
            footer = false;
            tagtext = false;
            kw = 0;
            continue;
        }
        if (lstring::ciprefix(HLP_HEADER, s)) {
            keyword = false;
            title = false;
            header = true;
            footer = false;
            tagtext = false;
            continue;
        }
        if (lstring::ciprefix(HLP_FOOTER, s)) {
            keyword = false;
            title = false;
            header = false;
            footer = true;
            tagtext = false;
            continue;
        }
        if (lstring::ciprefix(HLP_TITLE, s)) {
            keyword = false;
            title = true;
            header = false;
            footer = false;
            tagtext = false;
            ti = 0;
            continue;
        }
        if (lstring::ciprefix(HLP_TAGTEXT, s)) {
            keyword = false;
            title = false;
            header = false;
            footer = false;
            tagtext = true;
            continue;
        }
        if (lstring::ciprefix(HLP_TEXT, s) ||
                lstring::ciprefix(HLP_HTML, s)) {
            if (ti && kw) {
                char *t = kw;
                while ((s = lstring::gettok(&t)) != 0) {
                    int j = h_hash(s);
                    HLPent *bb = 0;
                    for (bb = hd_base[j]; bb; bb = bb->next) {
                        if (!strcmp(s, bb->keyword))
                            break;
                    }
                    if (bb) {
                        GRpkg::self()->ErrPrintf(ET_WARN,
                            "help database keyword clash for %s.\n", s);
                        delete [] s;
                        continue;
                    }
                    hd_base[j] = new HLPent(s, lstring::copy(ti), fname, tag,
                        ftell(fp), hd_base[j]);
                    if (lstring::ciprefix(HLP_HTML, buf))
                        hd_base[j]->is_html = true;
                }
                delete [] kw;
                kw = 0;
                keyword = false;
                delete [] ti;
                ti = 0;
                title = false;
            }
            continue;
        }
        if (lstring::ciprefix(HLP_LATEX, s)) {
            if (keyword || title) {
                GRpkg::self()->ErrPrintf(ET_WARN,
                    "LATEX block out of order.\n");
            }
            else if (header || footer || tagtext) {
                GRpkg::self()->ErrPrintf(ET_WARN,
                    "LATEX block out of order.\n");
            }
            lstring::advtok(&s);
            char *lkw = lstring::gettok(&s);
            if (lkw) {
                if (!hd_latex) {
                    hd_latex = new HLPent*[NUMHASH];
                    for (int i = 0; i < NUMHASH; i++)
                        hd_latex[i] = 0;
                }
                int j = h_hash(lkw);
                HLPent *bb = 0;
                for (bb = hd_latex[j]; bb; bb = bb->next) {
                    if (!strcmp(lkw, bb->keyword))
                        break;
                }
                if (bb) {
                    GRpkg::self()->ErrPrintf(ET_WARN,
                        "LATEX block keyword clash for %s.\n", lkw);
                    delete [] lkw;
                    continue;
                }
                hd_latex[j] = new HLPent(lkw, 0, fname, tag,
                    ftell(fp), hd_latex[j]);
            }
            continue;
        }

        if (keyword || title) {

            while (isspace(*s))
                s++;
            if (!*s)
                continue;

            char *t = s + strlen(s)-1;
            while (*t <= ' ')
                *t-- = '\0';

            if (keyword)
                kw = lstring::copy(s);
            else
                ti = lstring::copy(s);

        }
        else if (header) {
            while (isspace(*s))
                s++;
            if (!*s)
                continue;
            if (!hstr)
                hstr = lstring::copy(s);
            else {
                char *t = new char[strlen(hstr) + strlen(s) + 1];
                strcpy(t, hstr);
                strcat(t, s);
                delete [] hstr;
                hstr = t;
            }
        }
        else if (footer) {
            while (isspace(*s))
                s++;
            if (!*s)
                continue;
            if (!fstr)
                fstr = lstring::copy(s);
            else {
                char *t = new char[strlen(fstr) + strlen(s) + 1];
                strcpy(t, fstr);
                strcat(t, s);
                delete [] fstr;
                fstr = t;
            }
        }
        else if (tagtext) {
            while (isspace(*s))
                s++;
            if (!*s)
                continue;
            if (!ttext)
                ttext = lstring::copy(s);
            else {
                char *t = new char[strlen(ttext) + strlen(s) + 1];
                strcpy(t, ttext);
                strcat(t, s);
                delete [] ttext;
                ttext = t;
            }
        }
    }
    fclose(fp);
    if (hstr) {
        if (!HLP()->get_header())
            HLP()->set_header(hstr);
        else
            delete [] hstr;
    }
    if (fstr) {
        if (!HLP()->get_footer())
            HLP()->set_footer(fstr);
        else
            delete [] fstr;
    }
    if (maintag) {
        if (!HLP()->get_main_tag())
            HLP()->set_main_tag(maintag);
        else
            delete [] maintag;
    }
    if (ttext) {
        if (!HLP()->get_tag_text())
            HLP()->set_tag_text(ttext);
        else
            delete [] ttext;
    }
}


namespace {
    // An fgets that deals with Microsoft line termination.
    //
    char *
    my_fgets(char *buf, int size, FILE *fp)
    {
        char *s;
        int i;
        for (s = buf, i = size; i; s++, i--) {
            int c = getc(fp);
            if (c == '\r')
                c = getc(fp);
            if (c == EOF) {
                *s = '\0';
                if (s == buf)
                    return (0);
                return (buf);
            }
            *s = c;
            if (c == '\n') {
                *++s = '\0';
                return (buf);
            }
        }
        buf[size-1] = '\0';
        return (buf);
    }
}


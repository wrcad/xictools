
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
 * hlpsrv -- Help server, for www access to help system pages.
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#include "config.h"
#include "miscutil/childproc.h"
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
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif
#ifdef WIN32
#include "miscutil/msw.h"
#endif

// This is a server for the help database, which, through use of a simple cgi
// script, allows the help system to be accessed through a web server.
//
// There are three environment variables that must be set:
// HLPSRV_PATH      Full real path to help database files
// HLPSRV_CGIPATH   Server path to cgi script, with argument prefix.  This is
//                  prepended to the anchor text for all help keyword anchors.
//                  For example, the .hlp file contains <a href="keyword">,
//                  and HLPSRV_CGIPATH = "/cgi-bin/hlpsrv.cgi?h=", then the
//                  tag would be converted to
//                  <a href="/cgi-bin/hlpsrv.cgi?h=keyword">.
// HLPSRV_IMPATH    Server path to images called from help database files.
//                  This is prepended to the path which follows src= in img
//                  tags.  Images should be linked or moved to this location.
//
// The invocation arguments are database keywords.  The program dumps the
// Content-type header and topic text to the standard output.
//
// Here is an example Apache cgi script, which makes available the Xic
// help database.  This would be installed in the server's cgi-bin
// as (e.g.) xichelp.cgi.
//--------------------
// #! /bin/sh
// 
// IFS='='
// set $QUERY_STRING
// 
// export HLPSRV_PATH="/usr/local/xictools/xic/help"
// export HLPSRV_CGIPATH="/cgi-bin/xichelp.cgi?h="
// export HLPSRV_IMPATH="/help-images/"
// 
// /usr/local/xictools/bin/hlpsrv $2
//--------------------
//
// In web pages, the help system can then be accessed using code like
//--------------------
// <a href="/cgi-bin/xichelp.cgi?h=xic">
// Click here</a> to enter the on-line <b><i>Xic</i></b> help system.<br>
//--------------------


#ifndef DEF_TOPIC
#define DEF_TOPIC "xicinfo"
#endif

// #define HTML_BG_COLOR "#e8e8f0"
#define HTML_BG_COLOR "#f5f5f5"

namespace {
    GRpkg _gr_;

    // Default colors used for help text
    //
    char *BGimage;
    char *BGcolor;
    char *FGcolor;
    char *LNcolor;

    // CGI reference
    char *CgiPath;

    // Image path
    char *ImagePath;

    char *Anchor;
}

#ifndef HAVE_STRERROR
#ifndef SYS_ERRLIST_DEF
extern char *sys_errlist[];
#endif
#endif


// The following topic methods override those in help/help_topic.cc,
// which prevents topic.cc and most of the rest of ginterf from being
// linked.  The only file needed from the help system is
// help/help_read.cc.

void
HLPtopic::load_text()
{
    delete [] tp_chartext;
    sLstr lstr;
    char tbuf[256];
    bool body_added = false;
    if (!tp_show_plain && (!tp_is_html || tp_need_body)) {
        body_added = true;
        lstr.add("<body ");
        if (BGimage && *BGimage) {
            sprintf(tbuf, "background=\"%s\" ", BGimage);
            lstr.add(tbuf);
        }
        else if (BGcolor && *BGcolor) {
            sprintf(tbuf, "bgcolor=\"%s\" ", BGcolor);
            lstr.add(tbuf);
        }
        if (FGcolor && *FGcolor) {
            sprintf(tbuf, "text=\"%s\" ", FGcolor);
            lstr.add(tbuf);
        }
        if (LNcolor && *LNcolor) {
            sprintf(tbuf, "link=\"%s\" ", LNcolor);
            lstr.add(tbuf);
        }
        lstr.add(">\n");
    }
    if (!tp_show_plain && tp_from_db) {
        bool did_title = false;
        const char *hdr = HLP()->get_header();
        if (hdr) {
            const char *ts = strstr(hdr, "%TITLE%");
            if (ts) {
                const char *t = hdr;
                while (t < ts)
                    lstr.add_c(*t++);
                lstr.add(tp_title);
                t += 7;
                while (*t)
                    lstr.add_c(*t++);
                did_title = true;
            }
            else
                lstr.add(hdr);
        }
        if (tp_title && *tp_title && !did_title) {
            sprintf(tbuf, "<H1>%s</H1>\n", tp_title);
            lstr.add(tbuf);
        }
        if (tp_tag && HLP()->get_main_tag() &&
                strcmp(tp_tag, HLP()->get_main_tag())) {
            const char *ttext = HLP()->get_tag_text();
            if (ttext) {
                const char *ts = strstr(ttext, "%TAG%");
                if (ts) {
                    const char *t = ttext;
                    while (t < ts)
                        lstr.add_c(*t++);
                    lstr.add(tp_tag);
                    t += 5;
                    while (*t)
                        lstr.add_c(*t++);
                }
                else
                    lstr.add(ttext);
            }
        }
    }
    for (HLPwords *hw = tp_text; hw; hw = hw->next) {
        char *s = hw->word;
        if (!tp_show_plain && tp_from_db) {
            while (isspace(*s))
                s++;
        }
        lstr.add(s);
        if (!tp_show_plain && !tp_is_html) {
            if (*s)
                lstr.add("<BR>");
            else
                lstr.add("<P>");
        }
        lstr.add_c('\n');
    }
    if (!tp_show_plain) {
        if (tp_subtopics) {
            lstr.add("<H3>Subtopics</H3>\n");
            for (HLPtopList *tl = tp_subtopics; tl; tl = tl->next()) {
                tl->set_buttontext(tl->description());
                if (!tl->buttontext())
                    tl->set_buttontext("<unknown>");
                sprintf(tbuf, "<A HREF=\"%s\">%s</A><BR>\n",
                    tl->keyword(), tl->buttontext());
                lstr.add(tbuf);
            }
        }
        if (tp_seealso) {
            lstr.add("<H3>References</H3>\n");
            for (HLPtopList *tl = tp_seealso; tl; tl = tl->next()) {
                tl->set_buttontext(tl->description());
                if (!tl->buttontext())
                    tl->set_buttontext("<unknown>");
                sprintf(tbuf, "<A HREF=\"%s\">%s</A><BR>\n",
                    tl->keyword(), tl->buttontext());
                lstr.add(tbuf);
            }
        }
        if (tp_from_db)
            lstr.add(HLP()->get_footer());
        if (body_added)
            lstr.add("</body>\n");

        if (Anchor) {
            lstr.add("<script type=\"text/javascript\">\n");
            lstr.add("document.location.hash=\'");
            lstr.add(Anchor);
            lstr.add("\'\n</script>\n");
        }
    }

    // Fix the references
    if (CgiPath || ImagePath) {
        const char *s = lstr.string();
        lstr.clear();
        bool in_a = false;
        bool in_img = false;
        const char *t = s;
        while (*t) {
            if (in_a && !strncasecmp(t, "href=", 5)) {
                lstr.add_c(*t++);
                lstr.add_c(*t++);
                lstr.add_c(*t++);
                lstr.add_c(*t++);
                lstr.add_c(*t++);
                if (*t == '"')
                    lstr.add_c(*t++);
                // We don't touch the reference if it has a protocol specifier
                if (strncasecmp(t, "http:", 5) && strncasecmp(t, "ftp:", 4) &&
                        strncasecmp(t, "mailto:", 7))
                    lstr.add(CgiPath);
                while (*t && *t != '>')
                    lstr.add_c(*t++);
            }
            else if (in_img && !strncasecmp(t, "src=", 4)) {
                lstr.add_c(*t++);
                lstr.add_c(*t++);
                lstr.add_c(*t++);
                lstr.add_c(*t++);
                if (*t == '"')
                    lstr.add_c(*t++);
                // We don't touch the reference if it has a protocol specifier
                if (strncasecmp(t, "http:", 5) && strncasecmp(t, "ftp:", 4))
                    lstr.add(ImagePath);
                while(*t && *t != '>')
                    lstr.add_c(*t++);
            }
            if (*t == '>') {
                in_a = false;
                in_img = false;
            }
            else if (*t == '<') {
                lstr.add_c(*t++);
                while (isspace(*t))
                    lstr.add_c(*t++);
                if ((*t == 'a' || *t == 'A') && isspace(t[1]))
                    in_a = true;
                else if (!strncasecmp(t, "img", 3) && isspace(t[3]))
                    in_img = true;
            }
            lstr.add_c(*t++);
        }
        tp_chartext = lstr.string_clear();
    }
    else
        tp_chartext = lstr.string_trim();
}


// Emit the html to stdout.
//
bool
HLPtopic::show_in_window()
{
    printf("Content-type: text/html\n\n");
    printf("<html>\n");
    printf("<head><title>%s</title></head>\n", tp_title);
    puts(get_text());
    printf("</html>\n\n");
    return (true);
}
// End of topic functions.


// We also need to define HLPtopList::sort.

namespace {
    // Strip HTML tags from the beginning of the string, return the first
    // text.
    //
    inline const char *
    striptag(const char *s)
    {
        while (*s == '<') {
            const char *t = s + 1;
            while (*t && *t != '>')
                t++;
            if (!*t)
                break;
            s = t+1;
        }
        return (s);
    }

    inline bool
    sortcmp(const HLPtopList *tlp1, const HLPtopList *tlp2)
    {
        const char *s1 = tlp1->description();
        const char *s2 = tlp2->description();
        s1 = striptag(s1);
        s2 = striptag(s2);
        while (*s1 && *s2) {
            if (isdigit(*s1) && isdigit(*s2)) {
                int d = atoi(s1) - atoi(s2);
                if (d != 0)
                    return (d < 0);
                // same number, but may be leading 0's
                while (isdigit(*s1)) {
                    d = *s1 - *s2;
                    if (d)
                        return (d < 0);
                    s1++;
                    s2++;
                }
                continue;
            }
            int d = *s1 - *s2;
            if (d)
                return (d < 0);
            s1++;
            s2++;
        }
        return (*s1 < *s2);
    }
}

// Static function.
HLPtopList *
HLPtopList::sort(HLPtopList *thisp)
{
    int num;
    HLPtopList *tl;
    for (num = 0, tl = thisp; tl; num++, tl = tl->tl_next) ;
    if (num < 2)
        return (thisp);

    HLPtopList **vec = new HLPtopList*[num];
    int i;
    for (tl = thisp, i = 0; tl; tl = tl->tl_next, i++)
        vec[i] = tl;
    std::sort(vec, vec + num, sortcmp);
    tl = vec[0];
    for (i = 0; i < num - 1; i++)
        vec[i]->tl_next = vec[i + 1];
    vec[i]->tl_next = 0;
    delete [] vec;
    return (tl);
}
// End of HLPtopList functions.


// Finally, we need some HLPcontext things.

HLPcontext::HLPcontext()
{
    memset(this, 0, sizeof(HLPcontext));
}

void
HLPcontext::clearVisited()
{
}
// End of HLPcontext functions.


namespace {
    void init_signals();

    // This intercepts "special" keywords in hlp_word.
    //
    bool
    special_callback(char **word, HLPtopic **ptop)
    {
        char buf[256];
        strcpy(buf, *word);

        delete [] Anchor;
        Anchor = 0;

        char *anc = strchr(buf, '#');
        if (anc) {
            *anc++ = 0;
            Anchor = lstring::copy(anc);
        }

        const char *a = HLP()->alias(buf);
        if (a)
            strcpy(buf, a);

        *ptop = HLP()->read(buf);

        return (false);
    }
}


int
main(int argc, char **argv)
{
    BGcolor = lstring::copy(HTML_BG_COLOR);
    FGcolor = lstring::copy("black");
    LNcolor = lstring::copy("blue");

    HLP()->register_callback(special_callback);

    CgiPath = getenv("HLPSRV_CGIPATH");
    ImagePath = getenv("HLPSRV_IMPATH");
    char *path = getenv("HLPSRV_PATH");
    if (!path) {
        fprintf(stderr, "Error: HLPSRV_PATH not set.\n");
        return (1);
    }
    HLP()->set_path(path, false);

    init_signals();

    char *url = 0;
    for (int ac = 1; ac < argc; ac++) {
        const char *t = argv[ac];
        if (t[0] == '-' && t[1] == 'D') {
            if (t[2])
                HLP()->define(t+2);
            else if (ac+1 < argc) {
                ac++;
                HLP()->define(argv[ac]);
            }
            continue;
        }
        if (url)
            break;

        // If this is an existing html or image file, pass the full
        // path name.
        const char *p = strrchr(t, '.');
        if (p) {
            p++;
            if (lstring::cieq(p, "html") || lstring::cieq(p, "htm") ||
                    lstring::cieq(p, "jpg") || lstring::cieq(p, "gif") ||
                    lstring::cieq(p, "png") || lstring::cieq(p, "xpm")) {
                if (!lstring::is_rooted(t)) {
                    char *cwd = getcwd(0, 256);
                    if (cwd) {
                        url = new char[strlen(cwd) + strlen(t) + 2];
                        sprintf(url, "%s/%s", cwd, t);
                        delete [] cwd;
                        if (access(url, R_OK)) {
                            // no such file
                            delete [] url;
                            url = 0;
                        }
                    }
                }
            }
        }
        if (!url)
            url = lstring::copy(t);
    }
    if (!url)
        url = lstring::copy(DEF_TOPIC);
    HLP()->word(url);
    delete [] url;

    char *err = 0;
    if (HLP()->error_msg())
        err = lstring::copy(HLP()->error_msg());
    if (err) {
        HLP()->word(".");
        if (HLP()->error_msg()) {
            GRpkgIf()->ErrPrintf(ET_ERROR, "%s", HLP()->error_msg());
            exit (1);
        }
        GRpkgIf()->ErrPrintf(ET_WARN, "%s", err);
        return (1);
    }
    return (0);
}


namespace {
    // The signal handler
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


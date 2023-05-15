
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
 * hlp2html -- XicTools help database to HTML converter                   *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#include "config.h"
#include "help/help_defs.h"
#include "help/help_context.h"
#include "help/help_topic.h"

#include <algorithm>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/stat.h>
#ifdef HAVE_PWD_H
#include <pwd.h>
#endif
#include <unistd.h>

// hlp2html path [path...]
//
// This program takes as argements paths to .hlp files, or paths to
// directories containing .hlp files, and converts the help text to
// ordinary HTML files in the current directory.  The resulting set of
// files can be used to read the help text using an ordinary web
// browser.
//
// WARNING:  This program is ancient and rather obsolete, and does not
// handle the newer help database features.


#ifndef HAVE_STRERROR
#ifndef SYS_ERRLIST_DEF
extern char *sys_errlist[];
#endif
#endif

namespace {
    GRpkg gr;

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

    bool next_kw(char**, char**);
    char *my_fgets(char*, int, FILE*);
    void pathfix(char *string);
}



// The following topic methods override those in help/topic.cc, which
// prevents topic.cc and most of the rest of ginterf from being linked.
// The only file needed from the help system is help/help_read.cc.

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
            snprintf(tbuf, sizeof(tbuf), "background=\"%s\" ", BGimage);
            lstr.add(tbuf);
        }
        else if (BGcolor && *BGcolor) {
            snprintf(tbuf, sizeof(tbuf), "bgcolor=\"%s\" ", BGcolor);
            lstr.add(tbuf);
        }
        if (FGcolor && *FGcolor) {
            snprintf(tbuf, sizeof(tbuf), "text=\"%s\" ", FGcolor);
            lstr.add(tbuf);
        }
        if (LNcolor && *LNcolor) {
            snprintf(tbuf, sizeof(tbuf), "link=\"%s\" ", LNcolor);
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
            snprintf(tbuf, sizeof(tbuf), "<H1>%s</H1>\n", tp_title);
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
                snprintf(tbuf, sizeof(tbuf), "<A HREF=\"%s\">%s</A><BR>\n",
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
                snprintf(tbuf, sizeof(tbuf), "<A HREF=\"%s\">%s</A><BR>\n",
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



struct sHelp2html
{
    sHelp2html() { HlpPath = 0; }
    void process_path();
    void process_dir(char*);
    void process_file(char*);

    char *HlpPath;
};


// The main function takes a list of paths to files or directories.  The
// directory components are merged into a help database, which is then
// dumped as html files in the current directory.  Files given are simply
// converted, directories given convert all .hlp files in the directory.
//
int
main(int argc, char **argv)
{
    sHelp2html H;
    struct stat st;
    BGcolor = lstring::copy(HLP_DEF_BG_COLOR);
    FGcolor = lstring::copy(HLP_DEF_FG_COLOR);
    LNcolor = lstring::copy(HLP_DEF_LN_COLOR);
    
    if (argc == 1) {
        fprintf(stderr, "You need to give a file or directory path.\n");
        return (1);
    }

    // first read in the directories, so we have the references
    int i;
    for (i = 1; i < argc; i++) {
        char *path = argv[i];
        if (stat(path, &st)) {
            perror("stat");
            return (1);
        }
        if (st.st_mode & S_IFDIR)
            HLP()->set_path(path, 1);
        else {
            char *s = strrchr(path, '/');
            if (s) {
                *s = '\0';
                HLP()->set_path(path, 0);
                *s = '/';
            }
            else
                HLP()->set_path(".", 0);
        }
    }
    // now dump the converted files
    for (i = 1; i < argc; i++) {
        char *path = argv[i];
        if (stat(path, &st)) {
            perror("stat");
            return (1);
        }
        if (st.st_mode & S_IFDIR)
            H.process_dir(path);
        else
            H.process_file(path);
    }
    return (0);
}


// Process all files in all directories in the search path
//
void
sHelp2html::process_path()
{
    char dir[512];
    char *path = HlpPath;
    while (isspace(*path) || *path == '(')
        path++;
    while (path && *path) {
        int i = 0;
        while (*path && !isspace(*path) && *path != ':')
            dir[i++] = *path++;  
        while (isspace(*path) || *path == ':' || *path == ')')
            path++;
        dir[i] = '\0';
        pathfix(dir);
        process_dir(dir);
    }
}


// Process all files in the given directory
//
void
sHelp2html::process_dir(char *dir)
{

    DIR *wdir;
    if (!(wdir = opendir(dir))) {
        fprintf(stderr, "can't open help database directory %s.\n", dir);
        return;
    }
    dirent *de;
    char buf[512];
    while ((de = readdir(wdir)) != 0) {
        char *s;
        if ((s = strrchr(de->d_name, '.')) && !strcmp(s+1, "hlp")) {
            snprintf(buf, sizeof(buf), "%s/%s", dir, de->d_name);
            process_file(buf);
        }
    }
    closedir(wdir);
}


// Process the given file, converting to an html file in the current
// directory
//
void
sHelp2html::process_file(char *fname)
{
    char buf[256];
    char *s = strrchr(fname, '/');
    if (s)
        s++;
    else
        s = fname;
    strcpy(buf, s);
    s = strrchr(buf, '.');
    if (s)
        strcpy(s, ".html");
    else
        strcat(buf, ".html");
        
    FILE *op = fopen(buf, "w");
    if (!op) {
        perror(buf);
        return;
    }
    int keyword = 0;
    int title = 0;
    int text = 0;
    int subtopics = 0;
    int seealso = 0;

    FILE *ip = fopen(fname, "r");
    if (!ip) {
        perror(buf);
        fclose(op);
        return;
    }

    while ((s = my_fgets(buf, 256, ip)) != 0) {

        if (lstring::ciprefix(HLP_KEYWORD, buf)) {
            keyword = 1;
            title = 0;
            text = 0;
            subtopics = 0;
            seealso = 0;
            continue;
        }
        if (lstring::ciprefix(HLP_TITLE, buf)) {
            keyword = 0;
            title = 1;
            text = 0;
            subtopics = 0;
            seealso = 0;
            continue;
        }
        if (lstring::ciprefix(HLP_TEXT, buf) ||
                lstring::ciprefix(HLP_HTML, buf)) {
            keyword = 0;
            title = 0;
            text = 1;
            subtopics = 0;
            seealso = 0;
            continue;
        }
        if (lstring::ciprefix(HLP_SUBTOPICS, buf)) {
            keyword = 0;
            title = 0;
            text = 0;
            subtopics = 1;
            seealso = 0;
            continue;
        }
        if (lstring::ciprefix(HLP_SEEALSO, buf)) {
            keyword = 0;
            title = 0;
            text = 0;
            subtopics = 0;
            seealso = 1;
            continue;
        }
        if (keyword) {
            s = buf;
            char *t;
            while (next_kw(&t, &s)) {
                fprintf(op, "<A NAME=\"%s\"> \n", t);
                delete [] t;
            }
            continue;
        }
        if (title) {
            s = strchr(buf, '\n');
            if (s)
                *s = '\0';
            fprintf(op, "<H1>%s</H1>\n", buf);
            continue;
        }
        if (text) {
            s = strchr(buf, '\n');
            if (s)
                *s = '\0';
            s = buf;
            while (isspace(*s))
                s++;
            if (!*s) {
                if (text == 2) {
                    fprintf(op, "<P>\n");
                    text = 3;
                }
                continue;
            }
            text = 2;
            fprintf(op, "%s<BR>\n", s);
            continue;
        }
        if (subtopics) {
            s = strchr(buf, '\n');
            if (s)
                *s = '\0';
            HLPent *bb = HLP()->get_entry(buf);
            if (bb) {
                if (subtopics == 1) {
                    fprintf(op, "<H3>Subtopics</H3>\n");
                    subtopics = 2;
                }
                char tbuf[512];
                strcpy(tbuf, bb->filename);
                s = strrchr(tbuf, '/');
                if (s)
                    s++;
                else
                    s = tbuf;
                char *t = strrchr(tbuf, '.');
                if (t)
                    strcpy(t, ".html");
                else
                    strcat(s, ".html");
                fprintf(op, "<A HREF=\"%s#%s\">%s</A><BR>\n", s,
                    buf, bb->title);
            }
            continue;
        }
        if (seealso) {
            s = strchr(buf, '\n');
            if (s)
                *s = '\0';
            HLPent *bb = HLP()->get_entry(buf);
            if (bb) {
                if (seealso == 1) {
                    fprintf(op, "<H3>See Also</H3>\n");
                    seealso = 2;
                }
                char tbuf[512];
                strcpy(tbuf, bb->filename);
                s = strrchr(tbuf, '/');
                if (s)
                    s++;
                else
                    s = tbuf;
                char *t = strrchr(tbuf, '.');
                if (t)
                    strcpy(t, ".html");
                else
                    strcat(s, ".html");
                fprintf(op, "<A HREF=\"%s#%s\">%s</A><BR>\n", s,
                    buf, bb->title);
            }
            continue;
        }
    }
    fclose(op);
    fclose(ip);
}


namespace {
    // Grab a keyword, return false if there are no more keywords.
    //
    bool next_kw(char **kw, char **buf)
    {
        char *s, *t, kbuf[64];;

        s = *buf;
        while (isspace(*s))
            s++;
        if (!*s)
            return (false);
        t = kbuf;
        while (*s && !isspace(*s))
            *t++ = *s++;
        *t = '\0';
        if (kw)
            *kw = lstring::copy(kbuf);
        *buf = s;
        return (true);
    }


    // An fgets that works with DOS and UNIX.
    //
    char *my_fgets(char *buf, int size, FILE *fp)
    {
        char *s;
        int i, c;

        for (s = buf, i = size; i; s++, i--) {
            c = getc(fp);
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


    // In MSW, replace '/' with DOS directory separator, and truncate
    // names.  In UNIX, expande tildes.
    // Note that this is done in-place, string must be large enough
    // for expansion.
    //
    void pathfix(char *string)
    {
#ifdef WIN32
        char *s, *t;
        int bcnt = 0, ecnt = 0;

        s = t = string;
        while (*t != '\0') {
            if (*t == '/' || *t == '\\') {
                *s++ = '\\';
                t++;
                bcnt = 0;
                ecnt = 0;
            }
            else if (*t == '.') {
                *s++ = *t++;
                ecnt = 1;
            }
            else if (!ecnt) {
                if (bcnt++ < 8) *s++ = *t++;
                else t++;
            }
            else {
                if (ecnt++ < 4) *s++ = *t++;
                else t++;
            }
        }
        *s = '\0';

#else
#ifdef HAVE_GETPWUID
        passwd *pw;
        char *s, *t, buf[512];
        int i;

        if ((t = strchr(string, '~')) != 0) {
            if (t == string || isspace(*(t-1))) {
                s = t;
                t++;
                i = 0;
                while (*t && !isspace(*t) && *t != '/')
                    buf[i++] = *t++;
                buf[i] = '\0';
                pw = 0;
                if (!i)
                    pw = getpwuid(getuid());
                else
                    pw = getpwnam(buf);
                if (pw) {
                    strcpy(buf, t);
                    strcpy(s, pw->pw_dir);
                    strcat(s, buf);
                }
            }
        }
#endif
#endif
    }
}

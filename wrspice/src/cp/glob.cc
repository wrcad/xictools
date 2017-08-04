
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

//
// Expand global characters.
//

#include "config.h"
#include "cshell.h"
#include "lstring.h"
#include "graphics.h"

#include <sys/types.h>
#ifdef HAVE_DIRENT_H
#include <dirent.h>
#ifndef direct
#define direct dirent
#endif
#else
#ifdef HAVE_SYS_DIR_H
#include <sys/dir.h>
#endif
#endif
#ifdef HAVE_GETPWUID
#include <pwd.h>
#endif


namespace {
    char *pcanon(const char*);
    int sortcmp(char**, char**);
}


// For each word, go through two steps: expand the {}'s, and then do ?*[]
// globbing in them. Sort after the second phase but not the first...
//
void
CshPar::DoGlob(wordlist **list)
{
    // Expand {a, b, c}
    if (list == 0 || *list == 0)
        return;
    wordlist *wlist = *list, *wl;

    // These do their own glob matching
    if (lstring::eq(wlist->wl_word, "show") ||
            lstring::eq(wlist->wl_word, "alter"))
        return;

    for (wl = wlist; wl; wl = wl->wl_next) {
        wordlist *w = BracExpand(wl->wl_word);
        if (!w) {
            wordlist::destroy(wlist);
            *list = 0;
            return;
        }
        wordlist *nwl = wl->splice(w);
        if (wlist == wl)
            wlist = w;
        wl = nwl;
    }

    // Do tilde expansion

    for (wl = wlist; wl; wl = wl->wl_next)
        if (*wl->wl_word == cp_til) {
            char *s = TildeExpand(wl->wl_word);
            delete wl->wl_word;
            if (!s)
                wl->wl_word = lstring::copy("");
            else
                wl->wl_word = s;
        }

    // Now, expand *?[] for each word. unset * and unalias * mean 
    // something special

    if ((cp_flags[CP_NOGLOB] == true) || lstring::eq(wlist->wl_word, "unset") ||
            lstring::eq(wlist->wl_word, "unalias")) {
        *list = wlist;
        return;
    }

    for (wl = wlist; wl; wl = wl->wl_next) {
        if (noglobs(wl->wl_word))
            continue;
        wordlist *w = GlobExpand(wl->wl_word);
        if (w == 0)
            continue;
        wordlist *nwl = wl->splice(w);
        if (wlist == wl)
            wlist = w;
        wl = nwl;
    }
    *list = wlist;
}


// Say whether the pattern p can match the string s.
//
bool
CshPar::GlobMatch(const char *p, const char *s)
{
    if ((*s == '.') && ((*p == cp_huh) || (*p == cp_star)))
        return (false);

    for (;;) {
        char schar = STRIP(*s++);
        char pchar = *p++;
        if (pchar == cp_star) {
            if (*p == '\0')
                return (true);
            for (s--; *s != '\0'; s++)
                if (GlobMatch(p, s))
                    return (true);
            return (false);
        }
        else if (pchar == cp_huh) {
            if (schar == '\0')
                return (false);
            continue;
        }
        else if (pchar == cp_obrac) {
            bool bchar = false;
            bool except = false;
            if (*p == '^') {
                except = true;
                p++;
            }
            char fc = -1, bc;
            while ((bc = *p++) != 0) {
                if (bc == cp_cbrac) {
                    if ((bchar && !except) || 
                        (!bchar && except))
                        break;
                    else
                        return (false);
                }
                if (bc == '-') {
                    if (fc <= schar && schar <= *p++)
                        bchar = true;
                }
                else {
                    fc = bc;
                    if (fc == schar)
                        bchar = true;
                }
            }
            if (bc == '\0') {
                GRpkgIf()->ErrPrintf(ET_ERROR, "missing ].\n");
                return (false);
            }
            continue;
        }
        else if (pchar == '\0') {
            if (schar == '\0')
                return (true);
            else
                return (false);
        }
        else {
            if (STRIP(pchar) != schar)
                return (false);
            continue;
        }
    }
}


namespace {
    // Do tilde expansion. If the expansion fails, return 0.
    //
    char *tilde_expand(const char *string)
    {
        char *ret = 0;
        if (!string)
            return (0);
        const char *s0 = string;
        while (isspace(*string))
            string++;
        if (!*string || *string != '~')
            return (lstring::copy(s0));

        const char *t = string + 1;
        const char *t0 = t;
        while (*t && !isspace(*t) && *t != '/'
#ifdef WIN32
                && *t != '\\'
#endif
                )
            t++;

#ifdef HAVE_GETPWUID
        passwd *pw = 0;
        if (t == t0)
            pw = getpwuid(getuid());
        else {  
            char *ctmp = new char[t - t0 + 1];
            char *c = ctmp;
            while (t0 < t)
                *c++ = *t0++;
            *c = 0;
            pw = getpwnam(ctmp);
            delete [] ctmp;
        }
        if (pw) {
            t0--;
            ret = new char[t0 - s0 + strlen(pw->pw_dir) + strlen(t) + 1];
            strncpy(ret, s0, t0 - s0);
            strcpy(ret + (t0 - s0), pw->pw_dir);
            strcat(ret, t);
        }
#else
        if (t == t0) {
            char *home = getenv("HOME");
            if (home) {
                t0--;
                ret = new char[t0 - s0 + strlen(home) + strlen(t) + 1];
                strncpy(ret, s0, t0 - s0);
                strcpy(ret + (t0 - s0), home);
                strcat(ret, t);
            }
        }
#endif
        return (ret);
    }
}


// Expand tildes
//
char *
CshPar::TildeExpand(const char *string)
{
    char *result = tilde_expand(string);
    if (!result) {
        if (cp_flags[CP_NONOMATCH])
            return (lstring::copy(string));
        else
            return (0);
    }
    return (result);
}


// Return a wordlist, with *?[] expanded and sorted. This is the idea: set
// up an array with possible matches, and go through each path component
// and search the appropriate directories for things that match, and add
// those that do to the array.
//
wordlist *
CshPar::GlobExpand(const char *string)
{
    char buf[BSIZE_SP];
    char *poss[MAXWORDS];
    memset(poss, 0, MAXWORDS*sizeof(char *));
    string = pcanon(string);
    const char *point = string;

    if (lstring::is_dirsep(*point)) {
        point++;
        poss[0] = lstring::copy("/");
    }
    else if (point[0] == '.' && point[1] == '.' &&
            lstring::is_dirsep(point[2])) {
        poss[0] = lstring::copy("..");
        point += 3;
    }
#ifdef WIN32
    else if (point[1] == ':') {
        point += 2;
        if (lstring::is_dirsep(point[0])) {
            point++;
            poss[0] = lstring::copy("?:/");
        }
        else
            poss[0] = lstring::copy("?:.");
        *poss[0] = *string;
    }
#endif
    else
        poss[0] = lstring::copy(".");

    int level = 0;
nextcomp:
    level++;
    strcpy(buf, point);
    char *s = lstring::strdirsep(buf);
    if (s)
        *s = '\0';
    int i;
#if defined(HAVE_DIRENT_H) || defined(HAVE_SYS_DIR_H)
    for (i = 0; i < MAXWORDS; i++) {
        if (!poss[i] || (poss[i][0] == '\0'))
            continue;
        bool found = false;
        DIR *wdir = opendir(poss[i]);
        if (wdir == 0) {
            if (level > 1) {
                delete [] poss[i];
                poss[i] = 0;
                continue;
            }
            wordlist *wl = 0;
            if (cp_flags[CP_NONOMATCH])
                wl  = new wordlist(string, 0);
            else
                GRpkgIf()->ErrPrintf(ET_MSG, "%s: no match.\n", string);
            delete [] string;
            return (wl);
        }
        struct direct *de;
        while ((de = readdir(wdir)) != 0)
            if (GlobMatch(buf, de->d_name)) {
                found = true;
                int j;
                for (j = 0; j < MAXWORDS; j++)
                    if (!poss[j])
                        break;
                if (j == MAXWORDS) {
                    GRpkgIf()->ErrPrintf(ET_ERROR, "too many arguments.\n");
                    for (i = 0; i < MAXWORDS; i++)
                        delete [] poss[i];
                    delete [] string;
                    return (0);
                }
                poss[j] = new char [BSIZE_SP];
                // Hide the newly found words from the globbing process
                // by making the first byte a '\0'.
                //
                *poss[j] = '\0';
                strcpy(poss[j] + 1, poss[i]);
                strcat(poss[j] + 1, "/");
                strcat(poss[j] + 1, de->d_name);
            }
        delete [] poss[i];
        poss[i] = 0;
        closedir(wdir);
        if (!found) {
            if (level > 1)
                continue;
            wordlist *wl = 0;
            if (cp_flags[CP_NONOMATCH])
                wl  = new wordlist(string, 0);
            else
                GRpkgIf()->ErrPrintf(ET_MSG, "%s: no match.\n", string);
            delete [] string;
            return (wl);
        }
    }
    // copy down over loading 0
    for (i = 0; i < MAXWORDS; i++)
        if (poss[i])
            for (int j = 0; (poss[i][j] = poss[i][j+1]) != '\0'; j++) ;
    if (lstring::strdirsep(point)) {
        point = lstring::strdirsep(point) + 1;
        goto nextcomp;
    }
#endif

    // Compact everything properly
    int j;
    for (i = j = 0; i < MAXWORDS; i++) {
        if (!poss[i])
            continue;
        if (i != j) {
            poss[j] = poss[i];
            poss[i] = 0;
        }
        j++;
    }

    wordlist *wlist = 0;
    if (j == 0) {
        if (cp_flags[CP_NONOMATCH])
            wlist = new wordlist(string, 0);
        else
            GRpkgIf()->ErrPrintf(ET_MSG, "%s: no match.\n", string);
    }
    else {
        // Now, sort the stuff and make it into wordlists
        qsort((char*)poss, j, sizeof(char*), 
            (int(*)(const void*, const void*))sortcmp);

        wordlist *wl = 0;
        for (i = 0; i < j; i++) {
            if (wlist == 0)
                wlist = wl = new wordlist(pcanon(poss[i]), 0);
            else {
                wl->wl_next = new wordlist(pcanon(poss[i]), wl);
                wl = wl->wl_next;
            }
            delete [] poss[i];
            poss[i] = 0;
        }
    }
    delete [] string;
    return (wlist);
}


// Expand {...}
//
wordlist *
CshPar::BracExpand(const char *string)
{
    if (!string)
        return (0);
    wordlist *wl = brac1(string);
    if (!wl)
        return (0);
    for (wordlist *w = wl; w; w = w->wl_next) {
        char *s = w->wl_word;
        w->wl_word = lstring::copy(s);
        delete [] s;
    }
    return (wl);
}


// Given a string, returns a wordlist of all the {} expansions. This is
// called recursively by cp_brac2(). All the words here will be of size
// BSIZE_SP, so it is a good idea to copy() and free() the old words.
//
wordlist *
CshPar::brac1(const char *string)
{
    wordlist *words = new wordlist;
    words->wl_word = new char[BSIZE_SP];
    words->wl_word[0] = 0;

    for (const char *s = string; *s; s++) {
        if (*s == cp_ocurl) {
            wordlist *nwl = brac2(s);
            int nb = 0;
            for (;;) {
                if (*s == cp_ocurl)
                    nb++;
                if (*s == cp_ccurl)
                    nb--;
                if (*s == '\0') {   // {
                    GRpkgIf()->ErrPrintf(ET_ERROR, "missing }.\n");
                    wordlist::destroy(words);
                    return (0);
                }
                if (nb == 0)
                    break;
                s++;
            }
            // Add nwl to the rest of the strings in words
            wordlist *newwl = 0;
            for (wordlist *wl = words; wl; wl = wl->wl_next) {
                for (wordlist *w = nwl; w; w = w->wl_next) {
                    wordlist *nw = new wordlist;
                    nw->wl_word = new char[BSIZE_SP];
                    strcpy(nw->wl_word, wl->wl_word);
                    strcat(nw->wl_word, w->wl_word);
                    newwl = wordlist::append(newwl, nw);
                }
            }
            wordlist::destroy(words);
            words = newwl;
        }
        else
            for (wordlist *wl = words; wl; wl = wl->wl_next)
                lstring::appendc(wl->wl_word, *s);
    }
    return (words);
}


// Given a string starting with a {, return a wordlist of the expansions
// for the text until the matching }.
//
wordlist *
CshPar::brac2(const char *string)
{
    string++;   // Get past the first open brace...
    wordlist *wlist = 0;
    bool eflag = false;
    for (;;) {
        char *buf = lstring::copy(string);
        int nb = 0;
        char *s = buf;
        for (;;) {
            if ((*s == cp_ccurl) && (nb == 0)) {
                eflag = true;
                break;
            }
            if ((*s == cp_comma) && (nb == 0))
                break;
            if (*s == cp_ocurl)
                nb++;
            if (*s == cp_ccurl)
                nb--;
            if (*s == '\0') {       // {
                GRpkgIf()->ErrPrintf(ET_ERROR, "missing }.\n");
                wordlist::destroy(wlist);
                delete [] buf;
                return (0);
            }
            s++;
        }
        *s = '\0';
        wordlist *nwl = brac1(buf);
        wlist = wordlist::append(wlist, nwl);
        string += s - buf + 1;
        delete [] buf;
        if (eflag)
            return (wlist);
    }
}


bool
CshPar::noglobs(const char *string)
{
    if (strchr(string, cp_star) || strchr(string, cp_huh) || 
            strchr(string, cp_obrac))
        return (false);
    else
        return (true);
}


namespace {
    // Normalize filenames (get rid of extra ///, .///... etc.. )
    //
    char *pcanon(const char *string)
    {
        char *p = new char[strlen(string) + 1];
        char *s = p;
        for (;;) {
            int n = sizeof(".") - 1;
            if (!strncmp(string, ".", n) &&
                    lstring::is_dirsep(*(string + n))) {
                string += n+1;
                continue;
            }
            for (;;) {
                if (lstring::is_dirsep(*string)) {
                    *s++ = *string;
                    while (lstring::is_dirsep(*++string)) ;
                    break;
                }
                if (!*string) {
                    if (lstring::is_dirsep(*(s - 1)))
                        s--;
                    *s = '\0';
                    return (p);
                }
                *s++ = *string++;
            }
        }
    }


    int sortcmp(char **s1, char **s2)
    {
        char *a = *s1;
        char *b = *s2;
        for (;;) {
            if (*a > *b)
                return (1);
            if (*a < *b)
                return (-1);
            if (*a == '\0')
                return (0);
            a++;
            b++;
        }
    }
}


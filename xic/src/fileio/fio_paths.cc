
/*========================================================================*
 *                                                                        *
 *  Whiteley Research Inc, Sunnyvale, CA USA, http://wrcad.com            *
 *  Copyright (C) 2017 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *  Licensed under the Apache License, Version 2.0 (the "License");       *
 *  you may not use this file except in compliance with the License.      *
 *  You may obtain a copy of the License at                               *
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

#include "config.h"
#include "fio.h"
#include "fio_library.h"
#include "pathlist.h"
#include "filestat.h"
#include <ctype.h>
#ifdef HAVE_PWD_H
#include <pwd.h>
#endif


// Set the current search path, discarding the previous path.
//
void
cFIO::PSetPath(const char *string)
{
    char *s = lstring::copy(string);
    delete [] fioSymSearchPath;
    fioSymSearchPath = s;
}


const char *
cFIO::PGetPath()
{
    return (fioSymSearchPath);
}


#define noReadExclusive 0x2
#define addToBack       0x4
// #define PATH_DEBUG

// Set/unset the path used during read of native cells.
// Warning: this may change the search path.  The application may need
// notification if this occurs.
//
int
cFIO::PSetReadPath(const char *cpath, int flags)
{
    const char *s = lstring::strip_path(cpath);
    if (s == cpath)
        return (0);
    s--;
    char *pbuf = lstring::copy(cpath);
    char *t = pbuf + (s - cpath);
    *t++ = 0;

    if (!flags) {
        flags = 1;
        // set up the search path
        if (IsNoReadExclusive())
            flags |= noReadExclusive;
        if (IsAddToBack())
            flags |= addToBack;
        char *pth = lstring::copy(PGetPath());
        if (flags & noReadExclusive) {
            // don't change path position if dir is in path
            if (flags & addToBack)
                pth = PAppendPath(pth, pbuf, false);
            else
                pth = PPrependPath(pth, pbuf, false);
        }
        else
            // put dir at front of path for open
            pth = PPrependPath(pth, pbuf, true);
        delete [] fioSymSearchPath;
        fioSymSearchPath = pth;
#ifdef PATH_DEBUG
        printf("path= %s\n", fioSymSearchPath);
#endif
    }
    else {
        // fix the path
        if (!(flags & noReadExclusive) || !(flags & addToBack)) {
            // move "." to front
            char *pth = lstring::copy(PGetPath());
            if (PInPath(pth, ".")) {
                pth = PPrependPath(pth, ".", true);
                delete [] fioSymSearchPath;
                fioSymSearchPath = pth;
            }
            else
                delete [] pth;
        }
        if (!(flags & noReadExclusive) && (flags & addToBack)) {
            // move dir to back
            char *pth = lstring::copy(PGetPath());
            pth = PAppendPath(pth, pbuf, true);
            delete [] fioSymSearchPath;
            fioSymSearchPath = pth;
        }
#ifdef PATH_DEBUG
        printf("path= %s\n", fioSymSearchPath);
#endif
    }
    delete [] pbuf;
    return (flags);
}


// char *filein;        Name of the file to be opened.
// char *mode;          The file mode, as given to fopen.
// char **prealname;    Pointer to a location that will be filled
//                      in with the address of the real name of
//                      the file that was successfully opened.
//                      If 0, then nothing is stored.
// LibType type;        Library search mode.
// sLibRef **pref;      Library reference info return, if not null.
// sLib **plib          Library description, if not null.
//
FILE *
cFIO::POpen(const char *filein, const char *mode, char **prealname,
    int type, sLibRef **pref, sLib **plib)
{
    if (prealname)
        *prealname = 0;
    if (pref)
        *pref = 0;
    if (plib)
        *plib = 0;

    if (!filein || !*filein)
        return (0);
    // Expand tildes and strip white space, no dot expand.
    char *file = pathlist::expand_path(filein, false, true);
    FILE *fp;

    if (lstring::ciprefix("http://", file) ||
            lstring::ciprefix("ftp://", file)) {
        if (strchr(mode, 'w') || strchr(mode, 'a') || strchr(mode, '+')) {
            delete [] file;
            return (0);
        }
        fp = NetOpen(file, prealname);
        delete [] file;
        return (fp);
    }

    bool local = file[0] == '.' && (lstring::is_dirsep(file[1]) ||
        (file[1] == '.' && lstring::is_dirsep(file[2])));
    if (local || lstring::is_rooted(file) || *mode == 'w' ||
            pathlist::is_empty_path(fioSymSearchPath)) {
        fp = large_fopen(file, mode);
        if (!fp) {
            delete [] file;
            return (0);
        }
        if (prealname)
            // Dot expand returned path.
            *prealname = pathlist::expand_path(file, true, false);
        delete [] file;
        return (fp);
    }

    // Check if match found in library
    if (type != 0 && *mode == 'r' && !strchr(mode, '+')) {
        sLib *lib;
        sLibRef *ref;
        fp = OpenLibFile(0, file, type, &ref, &lib);
        if (fp) {
            if (prealname) {
                if (ref->dir())
                    *prealname = pathlist::mk_path(ref->dir(), ref->file());
                else
                    *prealname = lstring::copy(lib->filename());
                pathlist::path_canon(*prealname);
            }
            if (pref)
                *pref = ref;
            if (plib)
                *plib = lib;
            delete [] file;
            return (fp);
        }
    }

    // Now try going through the path.
    const char *path = fioSymSearchPath;
    if (path) {
        pathgen pg(path);
        char *p;
        while ((p = pg.nextpath((prealname != 0))) != 0) {
            char *ptmp = pathlist::mk_path(p, file);
            delete [] p;
            p = ptmp;
            fp = large_fopen(p, mode);
            if (fp != 0) {
                if (prealname) {
                    *prealname = lstring::copy(p);
                    pathlist::path_canon(*prealname);
                }
                delete [] p;
                delete [] file;
                return (fp);
            }
            delete [] p;
        }
    }
    delete [] file;
    return (0);
}


// Static function.
// This is like getcwd() except that under Windows, the path will contain
// forward slashes.
//
char *
cFIO::PGetCWD(char *buf, size_t size)
{
    char *cwd = getcwd(buf, size);
    if (!buf)
        cwd = lstring::tocpp(cwd);
#ifdef WIN32
    if (cwd) {
        for (char *s = cwd; *s; s++) {
            if (*s == '\\')
                *s = '/';
        }
    }
#endif
    return (cwd);
}


namespace {
    // Double quote the path if it contains a separation character.
    //
    char *
    quote_maybe(const char *path, bool mode)
    {
        if (!path)
            return (0);
        bool needsit = false;
        if (mode) {
            for (const char *p = path; *p; p++) {
                if (isspace(*p)) {
                    needsit = true;
                    break;
                }
            }
        }
        else {
            for (const char *p = path; *p; p++) {
                if (*p == PATH_SEP) {
                    needsit = true;
                    break;
                }
            }
        }
        if (needsit) {
            char *s = new char[strlen(path) + 3];
            s[0] = '"';
            strcpy(s+1, path);
            strcat(s, "\"");
            return (s);
        }
        return (lstring::copy(path));
    }
}


// Static function.
// If dir is not already in the search path, add it to the end.
// If it already is in the path and move is true, move to end.
// Returns the modified path string.  The old path is deleted
// if changed.
//
char *
cFIO::PAppendPath(char *spath, const char *dir, bool move)
{
    if (!spath)
        return (lstring::copy(dir));
    char *dirpth = pathlist::expand_path(dir, true, true);
    if (!dirpth)
        return (spath);

    pathgen pg(spath);
    int osthis, osnext;
    char *p;
    while ((p = pg.nextpath(true, &osthis, &osnext)) != 0) {
#ifdef WIN32
        if (!strcasecmp(dirpth, p)) {
#else
        if (!strcmp(dirpth, p)) {
#endif
            if (move) {
                // already there, remove
                char *s = spath + osthis;
                char *t = spath + osnext;
                while (*t)
                    *s++ = *t++;
                *s = 0;
                delete [] p;
                break;
            }
            else {
                // already there, return
                delete [] p;
                delete [] dirpth;
                return (spath);
            }
        }
        delete [] p;
    }

    delete [] dirpth;
    dirpth = quote_maybe(dir, pg.style());

    // Add the new element to the end of the path.  The path elements
    // are all unexpanded

    char *newp = new char[strlen(spath) + strlen(dirpth) + 4];
    strcpy(newp, spath);
    delete [] spath;

    int tl = strlen(newp);
    char *t = newp;
    if (tl) {
        t += tl - 1;
        if (pg.style()) {
            while ((isspace(*t) || *t == ')') && t >= newp)
                t--;
        }
        else {
            while (*t == PATH_SEP && t >= newp)
                t--;
        }
        t++;
        // t points to just beyond the last char of the last token
    }
    char *end = lstring::copy(t);

    if (t > newp)
        *t++ = (pg.style() ? ' ' : PATH_SEP);
    strcpy(t, dirpth);
    if (pg.style()) {
        char *ep = end;
        while (isspace(ep[0]) && isspace(ep[1]))
            ep++;
        strcat(t, ep);
    }
    else if (isspace(*(dirpth + strlen(dirpth) - 1))) {
        char ec[2];
        ec[0] = PATH_SEP;
        ec[1] = 0;
        strcat(t, ec);
    }
    delete [] end;
    delete [] dirpth;
    return (newp);
}


// Static function.
// If string is not already in the search path, add it to the beginning.
// If it is in the path, and move is true, move to the beginning of the path.
// "Beginning" means the first entry after ".", if present.
// Return the new path string, delete the old if changed.
//
char *
cFIO::PPrependPath(char *spath, const char *dir, bool move)
{
    if (!spath)
        return (lstring::copy(dir));
    char *dirpth = pathlist::expand_path(dir, true, true);
    if (!dirpth)
        return (spath);

    pathgen pg(spath);
    int osthis, osnext;
    char *p;
    while ((p = pg.nextpath(true, &osthis, &osnext)) != 0) {
#ifdef WIN32
        if (!strcasecmp(dirpth, p)) {
#else
        if (!strcmp(dirpth, p)) {
#endif
            if (move) {
                // already there, remove
                char *s = spath + osthis;
                char *t = spath + osnext;
                while (*t)
                    *s++ = *t++;
                *s = 0;
                delete [] p;
                break;
            }
            else {
                // already there, return
                delete [] p;
                delete [] dirpth;
                return (spath);
            }
        }
        delete [] p;
    }

    delete [] dirpth;
    dirpth = quote_maybe(dir, pg.style());

    // Add the new element to the front of the path.  The path elements
    // are all unexpanded.

    char *newp = new char[strlen(spath) + strlen(dirpth) + 4];
    char *s = newp;
    char *t = spath;
    if (pg.style()) {
        int spcnt = 0;
        for (;;) {
            if (*t == '(')
                *s++ = *t++;
            else if (isspace(*t)) {
                if (!spcnt)
                    *s++ = *t;
                t++;
                spcnt++;
            }
            else
                break;
        }
    }
    else if (isspace(*dirpth))
        *s++ = PATH_SEP;
    strcpy(s, dirpth);
    while (*s)
        s++;

    if (*t)
        *s++ = (pg.style() ? ' ' : PATH_SEP);
    strcpy(s, t);

    delete [] spath;
    delete [] dirpth;
    return (newp);
}


// Static function.
// Return true if dir is in spath.
//
bool
cFIO::PInPath(const char *spath, const char *dir)
{
    if (!spath)
        return (false);
    char *dirpth = pathlist::expand_path(dir, true, true);
    if (!dirpth)
        return (false);

    pathgen pg(spath);
    char *p;
    while ((p = pg.nextpath(true)) != 0) {
#ifdef WIN32
        if (!strcasecmp(dirpth, p)) {
#else
        if (!strcmp(dirpth, p)) {
#endif
            // already there, return
            delete [] p;
            delete [] dirpth;
            return (true);
        }
        delete [] p;
    }
    delete [] dirpth;
    return (false);
}


// Static function.
// Remove the directory dir if found in the path.  If found and removed,
// return true.  The path is edited in-place
//
bool
cFIO::PRemovePath(char *spath, const char *dir)
{
    if (!spath)
        return (false);
    char *dirpth = pathlist::expand_path(dir, true, true);
    if (!dirpth)
        return (false);

    pathgen pg(spath);
    int osthis, osnext;
    char *p;
    while ((p = pg.nextpath(true, &osthis, &osnext)) != 0) {
#ifdef WIN32
        if (!strcasecmp(dirpth, p)) {
#else
        if (!strcmp(dirpth, p)) {
#endif
            char *s = spath + osthis;
            char *t = spath + osnext;
            while (*t)
                *s++ = *t++;
            *s = 0;
            delete [] p;
            if (!pg.style()) {
                int len = strlen(spath);
                if (len) {
                    t = spath + len - 1;
                    if (*t == PATH_SEP && (t == spath || !isspace(*(t-1))))
                        *t = 0;
                }
            }
            delete [] dirpth;
            return (true);
        }
        delete [] p;
    }
    delete [] dirpth;
    return (false);
}



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
 * Misc. Utilities Library                                                *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#include "config.h"
#include "largefile.h"
#ifdef WIN32
#include "windows.h"
#include <limits.h>
#endif
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <time.h>
#ifdef HAVE_PWD_H
#include <pwd.h>
#include <grp.h>
#include <langinfo.h>
#endif
#include <algorithm>
#include "pathlist.h"
#include "filestat.h"

#ifdef __FreeBSD__
#include <limits.h>
#include <sys/sysctl.h>
#endif
#ifdef __APPLE__
#include <limits.h>
#include <mach-o/dyld.h>
#endif

#ifdef WIN32
#define fopen fopen64
#endif


sPathList::sPathList(const char *path, CheckFuncType chkfunc,
    const char *nf_msg, const char *t_head, const char *d_head, int cols,
    bool dirstoo)
{
    pl_path_string = lstring::copy(path);
    pl_checkfunc = chkfunc;
    pl_no_files_msg = nf_msg;
    pl_top_header = t_head;
    pl_dir_header = d_head;
    pl_columns = cols;
    pl_dirs = 0;
    pl_incldirs = dirstoo;
    sDirList *dend = 0;

    if (pathlist::is_empty_path(path))
        path = ".";
    pathgen pg(path);
    char *p;
    while ((p = pg.nextpath(true)) != 0) {
        sDirList *d = new sDirList(p);
        sFileList fl(p);
        fl.read_list(pl_checkfunc, pl_incldirs);
        int *colwid;
        char *txt = fl.get_formatted_list(pl_columns, false,
            pl_no_files_msg, &colwid);
        d->set_dirfiles(txt, colwid);
        if (!dend)
            dend = pl_dirs = d;
        else {
            dend->set_next(d);
            dend = d;
        }
        delete [] p;
    }
}


sPathList::~sPathList()
{
    delete [] pl_path_string;
    for (sDirList *d = pl_dirs; d; d = pl_dirs) {
        pl_dirs = d->next();
        delete d;
    }
}


// Update the directories with the dirty flag set.
//
void
sPathList::update()
{
    for (sDirList *dl = pl_dirs; dl; dl = dl->next()) {
        if (dl->dirty()) {
            sFileList fl(dl->dirname());
            fl.read_list(pl_checkfunc, pl_incldirs);
            int *colwid;
            char *txt = fl.get_formatted_list(pl_columns, false,
                pl_no_files_msg, &colwid);
            dl->set_dirfiles(txt, colwid);
            dl->set_dirty(false);
        }
    }
}
// End of sPathList functions.


sFileList::sFileList(const char *dir)
{
    if (!dir || !*dir)
        dir = ".";
    fl_directory = lstring::copy(dir);
    fl_file_list = 0;
}


sFileList::~sFileList()
{
    delete [] fl_directory;
    stringlist::destroy(fl_file_list);
}


// Add a file name to the list.
//
void
sFileList::add_file(const char *fn)
{
    if (fn)
        fl_file_list = new stringlist(lstring::copy(fn), fl_file_list);
}


// Read the directory and build the file list.  The checkfunc, if
// given, should return a heap-allocated string to use as the name of
// the passed file, or null to skip it.  If incldirs is true, include
// directories in the list.
//
bool
sFileList::read_list(CheckFuncType checkfunc, bool incldirs)
{
    DIR *wdir;
    if (!(wdir = opendir(fl_directory)))
        return (false);

    stringlist::destroy(fl_file_list);
    fl_file_list = 0;

    char *path = new char[strlen(fl_directory) + 256];
    char *dp = lstring::stpcpy(path, fl_directory);
    if (dp > path && !lstring::is_dirsep(*(dp-1)))
        *dp++ = '/';

    struct dirent *de;
    while ((de = readdir(wdir)) != 0) {
        char *name = de->d_name;
        if (name[0] == '.') {
            if (!name[1] || (name[1] == '.' && !name[2]))
                continue;
        }
        strcpy(dp, name);
#ifdef DT_DIR
        // Linux glibc returns DT_UNKNOWN for everything
        if (de->d_type != DT_UNKNOWN) {
            if (de->d_type != DT_REG && de->d_type != DT_DIR
#ifdef DT_LNK
                && de->d_type != DT_LNK
#endif
            ) continue;
            if (de->d_type == DT_DIR && !incldirs)
                continue;
        }
        else {
            GFTtype rt = filestat::get_file_type(path);
            if (rt != GFT_FILE && rt != GFT_DIR)
                continue;
            if (rt == GFT_DIR && !incldirs)
                continue;
        }
#else
        GFTtype rt = filestat::get_file_type(path);
        if (rt != GFT_FILE && rt != GFT_DIR)
            continue;
        if (rt == GFT_DIR && !incldirs)
            continue;
#endif

        char *string = 0;
        if (checkfunc)
            string = (*checkfunc)(path);
        else
            string = lstring::copy(name);
        if (string)
            fl_file_list = new stringlist(string, fl_file_list);
    }
    delete [] path;
    closedir(wdir);
    return (true);
}


// Return a string containing a sorted, column-formatted list of the
// file names.  The cols is the number of columns of space available. 
// If sort_newest_first, the files are listed in reverse order of last
// modified time, otherwise the listing is alphabetic.  If the list is
// empty, the no_files_msg is printed.
//
char *
sFileList::get_formatted_list(int cols, bool sort_newest_first,
    const char *no_files_msg, int **col_widp)
{
    if (!fl_file_list) {
        if (col_widp)
            *col_widp = 0;
        return (lstring::copy(no_files_msg ? no_files_msg : ""));
    }
    sort_list(sort_newest_first);
    return (stringlist::col_format(fl_file_list, cols, col_widp));
}


namespace {
    struct mt { char *name; time_t mtime; };

    // Sort comparison function, reverse order in mod time.
    inline bool mcmp(const mt &mt1, const mt &mt2)
    {
        return (mt2.mtime < mt1.mtime);
    }


    // Sort comparison function.
    // Cell names may have initial '*', '+', file names have type
    // character prepended (with space).  Don't let these mess up sort.
    //
    bool
    lcomp(const char *a, const char *b)
    {
        if (*a == '+')
            a++;
        if (*a == '*')
            a++;
        else if (*(a+1) == ' ' && *a && *(a+2))
            a += 2;
        while (*a == ' ') a++;
        if (*b == '+')
            b++;
        if (*b == '*')
            b++;
        else if (*(b+1) == ' ' && *b && *(b+2))
            b += 2;
        while (*b == ' ') b++;
        return ((strcmp(a, b) < 0));
    }
}


// Function to sort the file list, lexically or in reverse of the last
// modified time.
//
void
sFileList::sort_list(bool by_mtime)
{
    if (!fl_file_list || !fl_file_list->next)
        return;
    if (!by_mtime) {
        stringlist::sort(fl_file_list, &lcomp);
        return;
    }

    int n = stringlist::length(fl_file_list);
    mt *ary = new mt[n];
    char *path = new char[256 + (fl_directory ? strlen(fl_directory) : 0)];
    char *p = path;
    if (fl_directory) {
        p = lstring::stpcpy(path, fl_directory);
        if (p > path && !lstring::is_dirsep(*(p-1)))
            *p++ = '/';
    }

    int cnt = 0;
    stringlist *sp = 0, *sn;
    for (stringlist *sl = fl_file_list; sl; sl = sn) {
        sn = sl->next;
        ary[cnt].name = sl->string;
        strcpy(p, sl->string);
        struct stat st;
        if (stat(path, &st) < 0) {
            // Can't stat this file, throw it away.
            if (!sp)
                fl_file_list = sn;
            else
                sp->next = sn;
            n--;
            delete [] sl->string;
            delete sl;
            continue;
        }
        ary[cnt].mtime = st.st_mtime;
        cnt++;
        sp = sl;
    }

    if (n > 1) {
        std::sort(ary, ary + n, mcmp);
        cnt = 0;
        for (stringlist *sl = fl_file_list; sl; sl = sl->next)
            sl->string = ary[cnt++].name;
    }
    delete [] ary;
    delete [] path;
}
// End of sFileList functions.


//
// Utility functions.
//

// Return a formatted listing of files found along the path.
//
char *
pathlist::pathexp(const char *path, CheckFuncType checkfunc, int cols,
    bool dirstoo)
{
    sPathList *pl = new sPathList(path, checkfunc, 0, 0, 0, cols, dirstoo);
    sLstr lstr;
    for (sDirList *d = pl->dirs(); d; d = d->next()) {
        if (d->dirfiles()) {
            lstr.add(pathexpDirPrefix);
            lstr.add_c(' ');
            lstr.add(d->dirname());
            lstr.add_c('\n');
            lstr.add(d->dirfiles());
            lstr.add_c('\n');
        }
    }
    delete pl;
    return (lstr.string_trim());
}


// Return a formatted listing of files found in the given directory. 
// This is the same as above for a single-directory path.
//
char *
pathlist::dir_listing(const char *dir, CheckFuncType checkfunc, int cols,
    bool dirstoo, int **col_widp)
{
    if (!dir || !*dir)
        dir = ".";
    sPathList *pl = new sPathList(dir, checkfunc, 0, 0, 0, cols, dirstoo);
    sLstr lstr;
    sDirList *d = pl->dirs();
    if (d && d->dirfiles()) {
        lstr.add(pathexpDirPrefix);
        lstr.add_c(' ');
        lstr.add(d->dirname());
        lstr.add_c('\n');
        lstr.add(d->dirfiles());
        lstr.add_c('\n');
    }
    if (col_widp) {
        const int *w = d ? d->col_width() : 0;
        if (!w)
            *col_widp = 0;
        else {
            int i = 0;
            for ( ; w[i]; i++) ;
            int *n = new int[i+1];
            memcpy(n, w, (i+1)*sizeof(int));
            *col_widp = n;
        }
    }
    delete pl;
    return (lstr.string_trim());
}


namespace {
    char *
    cat(const char *pre, int npre, const char *s1, const char *s2)
    {
        int len = 0;
        if (npre)
            len += npre;
        if (s1)
            len += strlen(s1);
        if (s2)
            len += strlen(s2);
        char *s = new char[len + 1];
        s[0] = 0;
        if (npre) {
            strncpy(s, pre, npre);
            s[npre] = 0;
        }
        if (s1)
            strcat(s, s1);
        if (s2)
            strcat(s, s2);
        return (s);
    }
}


// Recursively substitute environment variables in "$variable" form in
// the string.  The PASSED STRING MUST HAVE BEEN HEAP-ALLOCATED as it
// will be freed if an expansion is done.  True is returned in this
// case, otherwise false.
//
// The variable must start with an alpha and contain only alpha,
// digits, and '_'.  a '\' ahead of the '$' prevents substitution. 
// Unresolved names are kept as-is.
//
bool
pathlist::env_subst(char **p)
{
    bool ret = false;
    if (!p || !*p)
        return (ret);
    for (char *s = *p; *s; s++) {
        if (*s == '$' && (s == *p || *(s-1) != '\\') && isalpha(*(s+1))) {
            const char *start = s+1;
            const char *end = start;
            while (isalnum(*end) || *end == '_')
                end++;
            int l = end - start;
            char *tstr = new char[l+1];
            strncpy(tstr, start, l);
            tstr[l] = 0;
            const char *env = getenv(tstr);
            delete [] tstr;
            if (env) {
                ret = true;
                sLstr lstr;
                *s = 0;
                lstr.add(*p);
                lstr.add(env);
                lstr.add(end);
                char *nstr = lstr.string_trim();
                s = nstr + strlen(*p) - 1;
                delete [] *p;
                *p = nstr;
            }
        }
    }
    return (ret);
}


// Perform tilde and optionally dot expansion of the directory path in
// string.  Optionally clip leading and trailing space.  In Windows,
// convert all directory separators to forward slashes.  Remove
// outermost quoting if stripping space.
//
char *
pathlist::expand_path(const char *string, bool expdot, bool strip_space)
{
    char *ret = 0;
    if (!string)
        return (0);
    const char *start = string;
    while (isspace(*string))
        string++;
    if (strip_space)
        start = string;
    if (!*string)
        return (lstring::copy(string));
    if (*string == '"' || *string == '\'')
        string++;
    int nstart = string - start;
    if (*string == '~') {
        const char *t = string + 1;
        const char *t0 = t;
        while (*t && !isspace(*t) && !lstring::is_dirsep(*t))
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
        if (pw)
            ret = cat(start, nstart, pw->pw_dir, t);
#else
        if (t == t0) {
            char *home = get_home();
            if (home) {
                ret = cat(start, nstart, home, t);
                delete [] home;
            }
        }
#endif
    }
    else if (expdot && *string == '.') {
        if (!*(string+1) || lstring::is_dirsep(*(string+1))) {
            char *cwd = getcwd(0, 0);
            if (cwd) {
                char *s = cwd + strlen(cwd) - 1;
                if (lstring::is_dirsep(*s) && lstring::is_dirsep(*(string+1)))
                    *s = 0;
                ret = cat(start, nstart, cwd, string+1);
                free(cwd);
            }
        }
        else if (*(string+1) == '.' &&
                (!*(string+2) || lstring::is_dirsep(*(string+2)))) {
            char *cwd = getcwd(0, 0);
            if (cwd) {
                char *s = lstring::strrdirsep(cwd);
                if (s && *(s+1)) {
                    *s = 0;
                    ret = cat(start, nstart, cwd, string+2);
                }
                free(cwd);
            }
        }
    }
    if (!ret)
        ret = cat(start, nstart, string, 0);
    if (strip_space) {
        char *t = ret + strlen(ret) - 1;
        while (isspace(*t) && t >= ret)
            *t-- = 0;
    }
#ifdef WIN32
    for (char *t = ret; *t; t++) {
        if (*t == '\\')
            *t = '/';
    }
#endif
    // Note that quoting is preserved up to here.  We can't use getqtok()
    // since we may have multiple unquoted tokens.  We want to remove the
    // top quoting level, if any.
    if (strip_space)
        lstring::unquote_in_place(ret);
    path_canon(ret);
    return (ret);
}


// Return a copy of the path: d/f.
//
char *
pathlist::mk_path(const char *d, const char *f)
{
    if (!d || !*d)
        return (lstring::copy(f));
    if (!f || !*f)
        return (lstring::copy(d));
    char *path = new char[strlen(d) + strlen(f) + 2];
    char *s = lstring::stpcpy(path, d);
    if (!lstring::is_dirsep(s[-1]))
        *s++ = '/';
    if (lstring::is_dirsep(*f))
        f++;
    strcpy(s, f);
    return (path);
}


// Return true if a path is empty.  This is complicated due to the
// various separation and quoting forms.
//
bool
pathlist::is_empty_path(const char *path)
{
    pathgen pg(path);
    char *s = pg.nextpath(false);
    if (s) {
        delete [] s;
        return (false);
    }
    return (true);
}


// Remove, in place, extra directory separators and forms like /./ and
// /../ from full path.
//
void
pathlist::path_canon(char *p)
{
    if (!p)
        return;
    while (isspace(*p))
        p++;
    // Eliminate redundant dirseps.
    char *t, *p0 = p;
    while ((t = lstring::strdirsep(p)) != 0) {
        if (lstring::is_dirsep(*(t+1))) {
#ifdef WIN32
            // Keep a leading '//' which indicates a share path.
            if (t == p0) {
                p++;
                continue;
            }
#endif
            p = t;
            for ( ; (*t = *(t+1)); t++) ;
            continue;
        }
        p = t+1;
    }

    // eliminate /./ and /../
#ifdef WIN32
    // skip c:
    if (isalpha(p0[0]) && p0[1] == ':' && lstring::is_dirsep(p0[2]))
        p0 += 2;
#endif
    p = p0;
    if (lstring::is_dirsep(*p)) {
        while ((t = strchr(p, '.')) != 0) {
            if (lstring::is_dirsep(*(t-1))) {
                if (lstring::is_dirsep(*(t+1))) {
                    // xxx/./yyy --> xxx/yyy
                    p = t;
                    for ( ; (*t = *(t+2)); t++) ;
                    continue;
                }
                else if (*(t+1) == '.' && lstring::is_dirsep(*(t+2))) {
                    // xxx/yyy/../zzz --> xxx/zzz
                    char *s = t+3;  // start of 'zzz'
                    char *q = t-2;  // end of 'yyy'
                    if (q > p0) {
                        while (q >= p0 && !lstring::is_dirsep(*q))
                            q--;
                        if (q >= p0) {
                            q++;
                            p = q;
                            while ((*q++ = *s++)) ;
                            continue;
                        }
                    }
                    break;
                }
            }
            p = t+1;
        }
    }

    // eliminate trailing dirsep
    t = p0 + strlen(p0) - 1;
    if (t > p0 && lstring::is_dirsep(*t))
        *t = 0;
}


// Return the process owner's real name or user name (malloc'ed).
//
char *
pathlist::get_user_name(bool realname)
{
    char buf[256];
#ifdef HAVE_GETPWUID
    struct passwd *pw = getpwuid(getuid());
    if (pw) {
        if (realname) {
            strcpy(buf, pw->pw_gecos);
            char *s = strchr(buf, ',');
            if (s)
                *s = 0;
            if (*buf)
                return (lstring::copy(buf));
        }
        return (lstring::copy(pw->pw_name));
    }
#else
#ifdef WIN32
    (void)realname;
    DWORD len = 255;
    if (GetUserName(buf, &len))
        return (lstring::copy(buf));
#endif
#endif
    return (lstring::copy("unknown_user"));
}


// Return the home directory (copied), or null if home can't be
// determined.
//
char *
pathlist::get_home()
{
#ifdef HAVE_GETPWUID
    // Unix/Linux
    passwd *pw = getpwuid(getuid());
    if (pw && pw->pw_dir && *pw->pw_dir)
        return (lstring::copy(pw->pw_dir));
    const char *homedir = getenv("HOME");
    if (homedir && *homedir)
        return (lstring::copy(homedir));
#endif

#ifdef WIN32
    // Ignore "HOME" as it is probably relative to a Cygwin or MSYS2
    // environment.  When using such an environment, the XT_HOMEDIR
    // variable should be set to the full Windows path of the
    // environment's home directory, e.g., "c:/msys64/home/yourname".

    const char *homedir = getenv("XT_HOMEDIR");
    if (homedir && *homedir) {
        char *home = lstring::copy(homedir);
        lstring::unix_path(home);
        return (home);
    }
    // This is deprecated.
    homedir = getenv("XIC_START_DIR");
    if (homedir && *homedir) {
        char *home = lstring::copy(homedir);
        lstring::unix_path(home);
        return (home);
    }

    // These may lead to the user's Windows home directory under
    // (e.g.) c:/Users.

    const char *homedrive = getenv("HOMEDRIVE");
    const char *homepath = getenv("HOMEPATH");
    if (homedrive && *homedrive && homepath && *homepath) {
        char *home = new char[strlen(homedrive) + strlen(homepath) + 1];
        strcpy(home, homedrive);
        strcat(home, homepath);
        lstring::unix_path(home);
        return (home);
    }
    const char *userprofile = getenv("USERPROFILE");
    if (userprofile && *userprofile) {
        char *home = lstring::copy(userprofile);
        lstring::unix_path(home);
        return (home);
    }
#endif

    // Can't find home.
    return (0);
}


// Return a rooted path to the binary of this running program.
//
// Mac OS X: _NSGetExecutablePath() (man 3 dyld)
// Linux: readlink /proc/self/exe
// Solaris: getexecname()
// FreeBSD: sysctl CTL_KERN KERN_PROC KERN_PROC_PATHNAME -1
// FreeBSD if it has procfs: readlink /proc/curproc/file (FreeBSD doesn't
//   have procfs by default)
// NetBSD: readlink /proc/curproc/exe
// DragonFly BSD: readlink /proc/curproc/file
// Windows: GetModuleFileName() with hModule = NULL
//
char *
pathlist::get_bin_path(const char *argv0)
{
    if (argv0 && lstring::is_rooted(argv0))
        return (lstring::copy(argv0));
#ifdef __linux
    char pthbuf[PATH_MAX];
    int pthlen = readlink("/proc/self/exe", pthbuf, PATH_MAX - 1);
    if (pthlen > 0) {
        pthbuf[pthlen] = 0;
        return (lstring::copy(pthbuf));
    }
#else
#ifdef WIN32
    char pthbuf[PATH_MAX];
    int pthlen = GetModuleFileName(0, pthbuf, PATH_MAX);
    if (pthlen > 0 && GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        return (lstring::copy(pthbuf));
#else
#ifdef __APPLE__
    char pthbuf[PATH_MAX];
    uint32_t pthlen = PATH_MAX;
    if (_NSGetExecutablePath(pthbuf, &pthlen) == 0)
        return (lstring::copy(pthbuf));
#else
#ifdef __FreeBSD__
    size_t pthlen = PATH_MAX;
    char pthbuf[pthlen];
    int mib[4];
    mib[0] = CTL_KERN;
    mib[1] = KERN_PROC;
    mib[2] = KERN_PROC_PATHNAME;
    mib[3] = -1;
    if (sysctl(mib, 4, pthbuf, &pthlen, 0, 0) == 0)
        return (lstring::copy(pthbuf));
#else
#ifdef __CYGWIN__
//XXX
#else
    FIXME DAMMIT!
#endif  // __CYGWIN__
#endif  // __FreeBSD__
#endif  // __APPLE__
#endif  // WIN32
#endif  // __linux
    return (0);
}


// Function to open a file from a search path.  If realname is not 0,
// it will contain the malloc'd full path name if the open is
// successful.  If cwdfirst, try to open in the current directory
// first.
//
FILE *
pathlist::open_path_file(const char *namein, const char *path,
    const char *mode, char **realname, bool cwdfirst)
{
    if (realname)
        *realname = 0;
    char *name = expand_path(namein, false, true);
    if (!name)
        return (0);

    bool local = name[0] == '.' &&
        (lstring::is_dirsep(name[1]) ||
            (name[1] == '.' && lstring::is_dirsep(name[2])));
    if (local || lstring::is_rooted(name) || !path || !*path || *mode == 'w') {
        FILE *fp = fopen(name, mode);
        if (fp && realname) {
            if (local)
                *realname = expand_path(name, true, false);
            else if (lstring::is_rooted(name))
                *realname = lstring::copy(name);
            else {
                char *cwd = getcwd(0, 0);
                *realname = mk_path(cwd, name);
                free(cwd);
            }
        }
        delete [] name;
        return (fp);
    }

    if (cwdfirst) {
        FILE *fp = fopen(name, mode);
        if (fp) {
            if (realname) {
                char *cwd = getcwd(0, 0);
                *realname = mk_path(cwd, name);
                free(cwd);
            }
            delete [] name;
            return (fp);
        }
    }

    pathgen pg(path);
    char *p;
    FILE *fp = 0;
    while ((p = pg.nextpath(realname ? true : false)) != 0) {
        char *ptmp = mk_path(p, name);
        delete [] p;
        p = ptmp;
        fp = fopen(p, mode);
        if (!fp) {
            delete [] p;
            continue;
        }
        if (realname)
            *realname = p;
        else
            delete [] p;
        break;
    }
    delete [] name;
    return (fp);
}


// Return true if the named file exists, using the given search path. 
// If realname is not null, return the full path to the file.  If
// cwdfirst, check the current directory before checking the search
// path.
//
bool
pathlist::find_path_file(const char *namein, const char *path,
    char **realname, bool cwdfirst)
{
    if (realname)
        *realname = 0;
    char *name = expand_path(namein, false, true);
    if (!name)
        return (false);

    bool local = name[0] == '.' &&
        (lstring::is_dirsep(name[1]) ||
            (name[1] == '.' && lstring::is_dirsep(name[2])));
    if (local || lstring::is_rooted(name) || !path || !*path) {
        bool found = (access(name, F_OK) == 0);
        if (found && realname) {
            if (local)
                *realname = expand_path(name, true, false);
            else if (lstring::is_rooted(name))
                *realname = lstring::copy(name);
            else {
                char *cwd = getcwd(0, 0);
                *realname = mk_path(cwd, name);
                free(cwd);
            }
        }
        delete [] name;
        return (found);
    }

    if (cwdfirst) {
        bool found = (access(name, F_OK) == 0);
        if (found) {
            if (realname) {
                char *cwd = getcwd(0, 0);
                *realname = mk_path(cwd, name);
                free(cwd);
            }
            delete [] name;
            return (true);
        }
    }

    pathgen pg(path);
    char *p;
    while ((p = pg.nextpath(realname ? true : false)) != 0) {
        char *ptmp = mk_path(p, name);
        delete [] p;
        p = ptmp;
        bool found = (access(p, F_OK) == 0);
        if (found) {
            if (realname)
                *realname = lstring::copy(p);
            delete [] p;
            delete [] name;
            return (true);
        }
        delete [] p;
    }
    delete [] name;
    return (false);
}


// Return a string that is very similar to the format of "ls -l", for
// file listings.  If nl is true, put the file name on a separate
// line.

//
char *
pathlist::ls_longform(const char *path, bool nl)
{
    const char *fname = lstring::strrdirsep(path);
    if (fname)
        fname++;
    else
        fname = path;
    sLstr lstr;
    struct stat st;
    if (stat(path, &st)) {
        lstr.add("cannot stat ");
        lstr.add(fname);
        return (lstr.string_trim());
    }

#ifdef WIN32
    time_t ftime = st.st_mtime;
    char tbuf[64];
    snprintf(tbuf, sizeof(tbuf), "%-12ld ", st.st_size);
    lstr.add(tbuf);
    strftime(tbuf, sizeof(tbuf), "%c", localtime(&ftime));
    lstr.add(tbuf);
#else
    // permission bits
    char mds[12];
    mds[0] = '-';
    if (S_ISFIFO(st.st_mode & S_IFMT))
        mds[0] = 'p';
    else if (S_ISCHR(st.st_mode & S_IFMT))
        mds[0] = 'c';
    else if (S_ISDIR(st.st_mode & S_IFMT))
        mds[0] = 'd';
    else if (S_ISBLK(st.st_mode & S_IFMT))
        mds[0] = 'b';
    else if (S_ISLNK(st.st_mode & S_IFMT))
        mds[0] = 'l';
#ifdef S_IF_SOCK
    else if (S_ISSOCK(st.st_mode))
        mds[0] = 's';
#endif
    mds[1] = (st.st_mode & S_IRUSR) ? 'r' : '-';
    mds[2] = (st.st_mode & S_IWUSR) ? 'w' : '-';
    if (st.st_mode & S_ISUID) {
        if (st.st_mode & S_IXUSR)
            mds[3] = 's';
        else
            mds[3] = 'S';
    }
    else
        mds[3] = (st.st_mode & S_IXUSR) ? 'x' : '-';
    mds[4] = (st.st_mode & S_IRGRP) ? 'r' : '-';
    mds[5] = (st.st_mode & S_IWGRP) ? 'w' : '-';
    if (st.st_mode & S_ISGID) {
        if (st.st_mode & S_IXGRP)
            mds[6] = 's';
        else
            mds[6] = 'S';
    }
    else
        mds[6] = (st.st_mode & S_IXGRP) ? 'x' : '-';
    mds[7] = (st.st_mode & S_IROTH) ? 'r' : '-';
    mds[8] = (st.st_mode & S_IWOTH) ? 'w' : '-';
#ifdef S_ISTXT
    if (st.st_mode & S_ISTXT) {
        if (st.st_mode & S_IXOTH)
            mds[9] = 't';
        else
            mds[9] = 'T';
    }
    else
#endif
        mds[9] = (st.st_mode & S_IXOTH) ? 'x' : '-';
    mds[10] = 0;

    // owner, group
    char obuf[16], gbuf[16];
    char *owner, *group;
    struct passwd *pw = getpwuid(st.st_uid);
    if (pw)
        owner = pw->pw_name;
    else {
        snprintf(obuf, sizeof(obuf), "%u", (unsigned int)st.st_uid);
        owner = obuf;
    }
    struct group *gp = getgrgid(st.st_gid);
    if (gp)
        group = gp->gr_name;
    else {
        snprintf(gbuf, sizeof(gbuf), "%u", (unsigned int)st.st_gid);
        group = gbuf;
    }

#ifdef D_MD_ORDER
    static int d_first = -1;
    if (d_first < 0)
        d_first = (*nl_langinfo(D_MD_ORDER) == 'd');
#else
#define d_first 0
#endif

    // date/time last modified
    time_t ftime = st.st_mtime;

    static time_t now = 0;
    if (now == 0)
        now = time(0);

#define SIXMONTHS   ((365 / 2) * 86400)
    bool f_sectime = true;
    const char *format;
    if (f_sectime)
        // mmm dd hh:mm:ss yyyy || dd mmm hh:mm:ss yyyy
        format = d_first ? "%e %b %T %Y " : "%b %e %T %Y ";
    else if (ftime + SIXMONTHS > now && ftime < now + SIXMONTHS)
        // mmm dd hh:mm || dd mmm hh:mm
        format = d_first ? "%e %b %R " : "%b %e %R ";
    else
        // mmm dd  yyyy || dd mmm  yyyy
        format = d_first ? "%e %b  %Y " : "%b %e  %Y ";

    // compose string
    lstr.add(mds);
    lstr.add_c(' ');
    lstr.add(owner);
    lstr.add_c(' ');
    lstr.add(group);
    lstr.add_c(' ');
    char tbuf[64];
    snprintf(tbuf, sizeof(tbuf), "%-12lld ", (long long)st.st_size);
    lstr.add(tbuf);
    strftime(tbuf, sizeof(tbuf), format, localtime(&ftime));
    lstr.add(tbuf);
#endif
    lstr.add_c(nl ? '\n' : ' ');
    lstr.add(fname);
    return (lstr.string_trim());
}


//
// pathgen: search path tokenizer
//

pathgen::pathgen(const char *path)
{
    pg_string = 0;
    pg_style = false;
    pg_offset = 0;
    if (path) {
        const char *p = path;
        while (isspace(*p))
            p++;
        if (*p == '(') {
            pg_style = true;
            while (*p == '(' || isspace(*p))
                p++;
        }
        pg_string = lstring::copy(p);
        pg_offset = p - path;
        if (pg_style) {
            char *s = pg_string + strlen(pg_string) - 1;
            while (s >= pg_string && (isspace(*s) || *s == ')'))
                *s-- = 0;
        }
        else {
            char *s = pg_string + strlen(pg_string) - 1;
            while (s >= pg_string && (isspace(*s)))
                *s-- = 0;
            // What if the user has forgotton the parens?  Look for a
            // space, and if the following token is rooted, assume
            // style is true
            s = pg_string;
            while (*s && !isspace(*s))
                s++;
            if (isspace(*s)) {
                while (isspace(*s))
                    s++;
                if (*s == '"')
                    s++;
#ifndef WIN32
                if (*s == '\'')
                    s++;
#endif
                if (lstring::is_rooted(s) || *s == '.')
                    pg_style = true;
            }
        }
    }
    pg_sptr = pg_string;
    pg_rdlist = 0;
}


// Return successive path components, 0 is returned when done.  The
// components returned are unquoted, tilde expanded and optionally dot
// expanded.  The two optional pointer arguemnts return the character
// offset of this token and the next token in the original string.
// This function never returns an empty pointer.  White space found in
// path components is retained.
//
// This handles redirect files, see note in pathlist.h.  The redirect
// processing is done only if the int pointers are null - these are
// used for path appending/prepending and not for traversal.
//
//
char *
pathgen::nextpath(bool expdot, int *osthis, int *osnext)
{
    char *pexp = 0;
    if (pg_rdlist && !osthis && !osnext) {
        stringlist *sl = pg_rdlist;
        pg_rdlist = pg_rdlist->next;
        pexp = pathlist::expand_path(sl->string, expdot, false);
        delete [] sl->string;
        delete sl;
    }
    else {
        if (!pg_sptr)
            return (0);
        for (;;) {
            char *dir = 0;
            if (pg_style) {
                // ( xxx xxx xxx )
                if (osthis)
                    *osthis = pg_offset + (pg_sptr - pg_string);
                dir = lstring::getqtok(&pg_sptr);
                if (osnext)
                    *osnext = pg_offset + (pg_sptr - pg_string);
                if (!dir) {
                    delete [] pg_string;
                    pg_sptr = pg_string = 0;
                    return (0);
                }
            }
            else {
                // xxx:xxx:xxx
                while (*pg_sptr == PATH_SEP)
                    pg_sptr++;
                if (osthis)
                    *osthis = pg_offset + (pg_sptr - pg_string);
                if (!*pg_sptr) {
                    delete [] pg_string;
                    pg_sptr = pg_string = 0;
                    return (0);
                }
                char *t = pg_sptr;
                while (*t) {
                    if (*t == '"' || *t == '\'')
                        lstring::advq(&t, 0, false);
                    else {
                        if (*t == PATH_SEP)
                            break;
                        t++;
                    }
                }
                dir = new char[t - pg_sptr + 1];
                char *np = dir;
                while (pg_sptr < t)
                    *np++ = *pg_sptr++;
                *np = 0;
                if (osnext) {
                    while (*t == PATH_SEP)
                        t++;
                    *osnext = pg_offset + (t - pg_string);
                }
                lstring::unquote_in_place(dir);
            }
            pexp = pathlist::expand_path(dir, expdot, false);
            delete [] dir;
            break;
        }
    }
    if (pexp && *pexp) {
        if (!osthis && !osnext)
            read_redirect(pexp);
        return (pexp);
    }
    delete [] pexp;
    return (0);
}


// Read and process the redirect file in dir, if any.
//
void
pathgen::read_redirect(const char *dir)
{
    char *rpath = pathlist::mk_path(dir, REDIRECT_FNAME);
    FILE *fp = fopen(rpath, "r");
    delete [] rpath;
    if (!fp)
        return;
    char buf[2048];
    for (;;) {
        char *s = fgets(buf, 2048, fp);
        if (!s)
            break;
        while (isspace(*s))
            s++;
        if (!*s || *s == '#')
            continue;
        char *d;
        while ((d = lstring::getqtok(&s)) != 0) {
            if (!lstring::is_rooted(d)) {
                char *t = pathlist::mk_path(dir, d);
                delete [] d;
                d = t;
            }
            struct stat st;
            if (!stat(d, &st) && S_ISDIR(st.st_mode))
                pg_rdlist = new stringlist(d, pg_rdlist);
            else
                delete [] d;
        }
    }
    fclose(fp);
}


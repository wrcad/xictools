
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

#include "largefile.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdarg.h>
#include <errno.h>
#include <stdlib.h>
#include <ctype.h>

#include "config.h"
#include "filestat.h"
#include "pathlist.h"
#ifdef WIN32
#include "windows.h"
#endif

//
// The stat function and struct differ when using 64-bits.  The
// filestat functions should be used instead of calling stat() when a
// large file might be encountered.
//

char *filestat::errmsg = 0;
char *filestat::mkt_def_id = 0;
char *filestat::mkt_env_var = 0;
stringlist *filestat::tmp_deletes = 0;
int filestat::rm_file_minutes = 0;  // default: disabled

// Instantiate an object, so destructor is called (and tmpfiles freed)
// on program exit.
namespace { filestat _fs_; }


// Return a code indicating whether to object is a regular file
// or directory.
//
GFTtype
filestat::get_file_type(const char *path)
{
    if (!path)
        return (GFT_NONE);
#ifdef WIN32
    // In Win32, stat fails for shares, i.e., "//machine/name".
    char *wpath = lstring::copy(path);
    lstring::dos_path(wpath);
    WIN32_FILE_ATTRIBUTE_DATA info;
    int ret = GetFileAttributesEx(wpath, GetFileExInfoStandard, &info);
    delete [] wpath;
    if (!ret) {
        errno = ENOENT;
        return (GFT_NONE);
    }
    if (info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        return (GFT_DIR);
    return (GFT_FILE);
#else
    struct stat st;
    if (!stat(path, &st)) {
        if (S_ISDIR(st.st_mode))
            return (GFT_DIR);
        if (S_ISREG(st.st_mode))
            return (GFT_FILE);
        return (GFT_OTHER);
    }
    return (GFT_NONE);
#endif
}


// Return true if regular file with read permission.
//
bool
filestat::is_readable(const char *path)
{
    if (!path)
        return (false);
#ifdef WIN32
    // In Win32, stat fails for shares, i.e., "//machine/name".
    char *wpath = lstring::copy(path);
    lstring::dos_path(wpath);
    WIN32_FILE_ATTRIBUTE_DATA info;
    int ret = GetFileAttributesEx(wpath, GetFileExInfoStandard, &info);
    delete [] wpath;
    if (!ret)
        return (false);
    return ((info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ||
        !(info.dwFileAttributes & FILE_ATTRIBUTE_READONLY));
#else
    struct stat st;
    return (!stat(path, &st) && (st.st_mode & S_IFREG) &&
        ((st.st_mode & S_IROTH) ||
            ((st.st_mode & S_IRGRP) && st.st_gid == getgid()) ||
            ((st.st_mode & S_IRUSR) && st.st_uid == getuid())));
#endif
}


// Return true if path is directory.
//
bool
filestat::is_directory(const char *path)
{
    if (!path)
        return (false);
#ifdef WIN32
    // In Win32, stat fails for shares, i.e., "//machine/name".
    char *wpath =lstring::copy(path);
    lstring::dos_path(wpath);
    WIN32_FILE_ATTRIBUTE_DATA info;
    int ret = GetFileAttributesEx(wpath, GetFileExInfoStandard, &info);
    delete [] wpath;
    if (!ret)
        return (false);
    return (info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
#else
    struct stat st;
    return (!stat(path, &st) && (S_ISDIR(st.st_mode)));
#endif
}


// Return true if path1 and path2 designate the same file.
//
bool
filestat::is_same_file(const char *path1, const char *path2)
{
    if (!path1 || !path2)
        return (false);
    struct stat st1, st2;
    return (!stat(path1, &st1) && !stat(path2, &st2) &&
#ifdef WIN32
    st1.st_size == st2.st_size && st1.st_mtime == st2.st_mtime);
#else
    st1.st_ino == st2.st_ino);
#endif
}


// Return the status of the file fname according to mode.
//
check_t
filestat::check_file(const char *fname, int mode)
{
    // check filename
    if (!fname) {
        save_err("Null filename.");
        return (NOGO);
    }
    while (isspace(*fname)) fname++;
    if (!*fname) {
        save_err("Empty filename.");
        return (NOGO);
    }

    GFTtype rt = filestat::get_file_type(fname);
    if (rt == GFT_NONE) {
        if (errno == ENOENT) {
            // file doesn't exist
            if (mode == R_OK) {
                save_err("File %s does not exist.", fname);
                return (NO_EXIST);
            }
            FILE *fp = fopen(fname, "w");
            if (fp) {
                fclose(fp);
                return (WRITE_OK);
            }
        }
    }
    if (rt == GFT_FILE) {
        if (mode == R_OK) {
            if (!access(fname, R_OK))
                return (READ_OK);
        }
        else {
            if (!access(fname, W_OK))
                return (WRITE_OK);
        }
    }
    const char *msg = "Error: can't %s file %s.";
    save_err(msg, (mode == R_OK) ? "read" : "write", fname);
    return (NOGO);
}


// An fopen replacement that provides an error message on failure.
//
FILE *
filestat::open_file(const char *fname, const char *mode)
{
    if (!fname || !*fname) {
        save_err("Null or empty filename.");
        return (0);
    }
    FILE *fp = fopen(fname, mode);
    if (!fp) {
        sLstr lstr;
        lstr.add("Can't open ");
        lstr.add(fname);
        lstr.add(":\n");
#ifdef HAVE_STRERROR
        lstr.add(strerror(errno));
#else
        lstr.add(sys_errlist[errno]);
#endif
        lstr.add_c('.');
        delete [] errmsg;
        errmsg = lstr.string_trim();
    }
    return (fp);
}


// This function returns true and creates a .bak file if fname exists
// and it is ok to write to fname.  Set *bakname to the backup name if
// an existing file was moved.
//
bool
filestat::create_bak(const char *fname, char **bakname)
{
    if (!fname || !*fname) {
        save_err("Null or empty filename.");
        return (false);
    }

    if (bakname)
        *bakname = 0;
    const char *msg1 = "Can't write %s, not a regular file.";
    const char *msg2 = "Can't write %s, permission denied.";

    GFTtype rt = get_file_type(fname);
    if (rt == GFT_NONE) {
        if (errno == ENOENT) {
            FILE *fp = fopen(fname, "w");
            if (fp) {
                fclose(fp);
                return (true);
            }
        }
        save_sys_err(fname);
        return (false);
    }
    if (rt != GFT_FILE) {
        save_err(msg1, fname);
        return (false);
    }
    if (access(fname, W_OK)) {
        save_err(msg2, fname);
        return (false);
    }

    char *backup = new char[strlen(fname) + 5];
    strcpy(backup, fname);
    strcat(backup, ".bak");
    rt = get_file_type(backup);
    if (rt == GFT_NONE) {
        if (errno == ENOENT) {
            if (!move_file_local(backup, fname)) {
                delete [] backup;
                return (false);
            }
            if (bakname)
                *bakname = backup;
            else
                delete [] backup;
            return (true);
        }
        save_sys_err(backup);
        delete [] backup;
        return (false);
    }
    if (rt != GFT_FILE) {
        save_err(msg1, backup);
        delete [] backup;
        return (false);
    }
    if (access(backup, W_OK)) {
        save_err(msg2, backup);
        delete [] backup;
        return (false);
    }
    if (unlink(backup)) {
        save_sys_err(backup);
        delete [] backup;
        return (false);
    }
    if (!move_file_local(backup, fname)) {
        delete [] backup;
        return (false);
    }
    if (bakname)
        *bakname = backup;
    else
        delete [] backup;
    return (true);
}


// Rename a file.  Files must be in the same file system.
//
bool
filestat::move_file_local(const char *newname, const char *oldname)
{
    if (!access(newname, F_OK))
        unlink(newname);
#ifdef WIN32
    // Bah! this needs to be DOS style
    char *s_newname = lstring::copy(newname);
    char *s_oldname = lstring::copy(oldname);
    lstring::dos_path(s_newname);
    lstring::dos_path(s_oldname);
    int r = rename(s_oldname, s_newname);
    delete [] s_newname;
    delete [] s_oldname;
    if (r) {
        save_sys_err("rename");
        return (false);
    }
#else
    if (link(oldname, newname)) {
        save_sys_err("link");
        return (false);
    }
    if (unlink(oldname)) {
        save_sys_err("unlink");
        return (false);
    }
#endif
    return (true);
}


// A more portable version of the standard "mktemp( )" function.
//
// Configure the default name prefix and the environment variable name
// for temp files.
//
void
filestat::make_temp_conf(const char *defid, const char *envvar)
{
    delete [] mkt_def_id;
    mkt_def_id = lstring::copy(defid);
    delete [] mkt_env_var;
    mkt_env_var = lstring::copy(envvar);
}


char *
filestat::make_temp(const char *id)
{
    static int num;
    static unsigned int pid;
    if (pid == 0)
        pid = getpid();
    else
        num++;

    if (!id)
        id = mkt_def_id;
    if (!id)
        id = "xx";

    const char *path = mkt_env_var ? getenv(mkt_env_var) : 0;
    if (!path)
        path = getenv("TMPDIR");
    if (!path)
        path = "/tmp";
    path = pathlist::expand_path(path, true, true);

#ifdef WIN32
    if (lstring::is_dirsep(path[0]) && !lstring::is_dirsep(path[1])) {
        // Add a drive specifier.  If the CWD is a share, use "C:".
        char *cwd = getcwd(0, 0);
        if (!isalpha(cwd[0]) || cwd[1] != ':') {
            cwd[0] = 'c';
            cwd[1] = ':';
        }
        cwd[2] = 0;
        char *t = new char[strlen(path) + 4];
        strcpy(t, cwd);
        free(cwd);
        strcpy(t+2, path);
        delete [] path;
        path = t;
    }
    mkdir(path);
#endif

    char buf[64], *fpath;
    snprintf(buf, sizeof(buf), "%s%u-%d", id, pid, num);
    if (!access(path, W_OK))
        fpath = pathlist::mk_path(path, buf);
    else
        fpath = lstring::copy(buf);
    delete [] path;
    return (fpath);
}


// Save a full path to a file for deletion.
//
void
filestat::queue_deletion(const char *fname)
{
    if (!fname)
        return;
    for (stringlist *c = tmp_deletes; c; c = c->next) {
        if (!strcmp(fname, c->string))
            return;
    }
    tmp_deletes = new stringlist(lstring::copy(fname), tmp_deletes);
}


// Unlink the files saved for deletion.
//
void
filestat::delete_deletions()
{
    for (stringlist *s = tmp_deletes; s; s = s->next)
        unlink(s->string);
    stringlist::destroy(tmp_deletes);
    tmp_deletes = 0;
}


// The following is an interface to the UNIX at command for deleting
// files at some time in the future, whether or not the application is
// still running.  This is used to clean up after printing temporary
// files.

void
filestat::set_rm_minutes(int m)
{
    rm_file_minutes = m;
}

int
filestat::rm_minutes()
{
    return (rm_file_minutes);
}

// Schedule the file for deletion.  If the delete is scheduled, a
// message is returned (this must be freed).
//
char *
filestat::schedule_rm_file(const char *filename)
{
#ifdef WIN32
    (void)filename;
    return (0);
#else
    if (rm_file_minutes <= 0)
        return (0);
    if (!filename || !*filename)
        return (0);
    int len = strlen(filename) + 80;
    char *buf = new char[len];
    sLstr lstr;
    snprintf(buf, len, "Scheduling %s for deletion in %d %s.\n", filename,
        rm_file_minutes, rm_file_minutes > 1 ? "minutes" : "minute");
    lstr.add(buf);

    snprintf(buf, len, "echo rm %s | at now + %d %s 2>&1\n", filename,
        rm_file_minutes, rm_file_minutes > 1 ? "minutes" : "minute");

    FILE *fp = popen(buf, "r");
    if (fp) {
        int c;
        while ((c = getc(fp)) != EOF)
            lstr.add_c(c);
        pclose(fp);
    }
    delete [] buf;
    return (lstr.string_trim());
#endif
}


void
filestat::save_sys_err(const char *str)
{
    sLstr lstr;
    if (str && *str) {
        lstr.add(str);
        lstr.add(": ");
    }
#ifdef HAVE_STRERROR
    lstr.add(strerror(errno));
#else
    lstr.add(sys_errlist[errno]);
#endif
    delete [] errmsg;
    errmsg = lstr.string_trim();
}


void
filestat::save_err(const char *fmt, ...)
{
    if (!fmt) {
        delete [] errmsg;
        errmsg = 0;
    }
    else {
        va_list args;
        char buf[BUFSIZ];
        va_start(args, fmt);
        vsnprintf(buf, BUFSIZ, fmt, args);
        va_end(args);
        delete [] errmsg;
        errmsg = lstring::copy(buf);
    }
}
// End of filestat functions



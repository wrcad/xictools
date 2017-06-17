
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2005 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *                                                                        *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,      *
 *   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES      *
 *   OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-        *
 *   INFRINGEMENT.  IN NO EVENT SHALL STEPHEN R. WHITELEY BE LIABLE       *
 *   FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION      *
 *   OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN           *
 *   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN         *
 *   THE SOFTWARE.                                                        *
 *                                                                        *
 * This library is available for non-commercial use under the terms of    *
 * the GNU Library General Public License as published by the Free        *
 * Software Foundation; either version 2 of the License, or (at your      *
 * option) any later version.                                             *
 *                                                                        *
 * A commercial license is available to those wishing to use this         *
 * library or a derivative work for commercial purposes.  Contact         *
 * Whiteley Research Inc., 456 Flora Vista Ave., Sunnyvale, CA 94086.     *
 * This provision will be enforced to the fullest extent possible under   *
 * law.                                                                   *
 *                                                                        *
 * This library is distributed in the hope that it will be useful,        *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU      *
 * Library General Public License for more details.                       *
 *                                                                        *
 * You should have received a copy of the GNU Library General Public      *
 * License along with this library; if not, write to the Free             *
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.     *
 *========================================================================*
 *                                                                        *
 * Misc. Utilities Library                                                *
 *                                                                        *
 *========================================================================*
 $Id: pathlist.h,v 1.13 2014/04/22 01:47:29 stevew Exp $
 *========================================================================*/

#ifndef PATHLIST_H
#define PATHLIST_H

#include "lstring.h"

#define FL_HASHLEN 31
#define FL_HASH(tag, k) {char *c=tag; for(k=0; *c; k+= *c++) ; k%=FL_HASHLEN;}

// Prefix for directory in listing.
#define pathexpDirPrefix "[directory]"

typedef char*(*CheckFuncType)(const char*);

// Utilities.
//
namespace pathlist {
    char *pathexp(const char*, CheckFuncType, int, bool);
    char *dir_listing(const char*, CheckFuncType, int, bool, int** = 0);
        // Listing functions.

    char *expand_path(const char*, bool, bool);
        // Tilde and dot expansion utility function.

    char *mk_path(const char*, const char*);
        // Create a path.

    bool is_empty_path(const char*);
        // Check if path is empty.

    void path_canon(char*);
        // Remove /./ and /../ in-place.

    char *get_user_name(bool);
        // Return user's name.

    char *get_home(const char* = 0);
        // Return user's home directory.

    char *get_bin_path(const char* = 0);
        // Return full path to running program binary.

    FILE *open_path_file(const char*, const char*, const char*,
        char**, bool);
        // Open a file from a search path.

    bool find_path_file(const char*, const char*, char**, bool);
        // Find a file using a search path.

    char *ls_longform(const char*, bool);
        // List a file in format similar to "ls -l".
};


//
// A container for a list of files.
//

struct sFileList
{
    sFileList(const char*);
    ~sFileList();

    void add_file(const char*);
    bool read_list(CheckFuncType, bool);
    char *get_formatted_list(int, bool, const char*, int** = 0);
    void sort_list(bool);

    stringlist *file_list()     const { return (fl_file_list); }

private:
    char *fl_directory;
    stringlist *fl_file_list;
};


//
// A list element for directories.
//

struct sDirList
{
    sDirList(const char *n)
        {
            dl_dirname = lstring::copy(n);
            dl_dirfiles = 0;
            dl_colwid = 0;
            dl_dirty = false;
            dl_mtime = 0;
            dl_dataptr = 0;
            dl_next = 0;
        }

    ~sDirList()
        {
            delete [] dl_dirname;
            delete [] dl_dirfiles;
            delete [] dl_colwid;
        }

    const char *dirname()       const { return (dl_dirname); }

    const char *dirfiles()      const { return (dl_dirfiles); }
    void set_dirfiles(char *s, int *cw)
        {
            delete [] dl_dirfiles;
            dl_dirfiles = s;
            delete [] dl_colwid;
            dl_colwid = cw;
        }

    const int *col_width()      const { return (dl_colwid); }

    bool dirty()                const { return (dl_dirty); }
    void set_dirty(bool b)      { dl_dirty = b; }

    time_t mtime()              const { return (dl_mtime); }
    void set_mtime(time_t t)    { dl_mtime = t; }

    void *dataptr()             const { return (dl_dataptr); }
    void set_dataptr(void *d)   { dl_dataptr = d; }

    sDirList *next()            { return (dl_next); }
    void set_next(sDirList *n)  { dl_next = n; }

private:
    char *dl_dirname;           // directory path
    char *dl_dirfiles;          // formatted file listing
    int *dl_colwid;             // column widths array
    bool dl_dirty;              // directory changed (external use)
    time_t dl_mtime;            // last modified time (external use)
    void *dl_dataptr;           // (external use)
    sDirList *dl_next;          // link
};


//
// The main structure for the path data.
//

struct sPathList
{
    sPathList(const char*, char*(*)(const char*), const char*,
        const char*, const char*, int, bool);
    ~sPathList();

    void update();

    int columns()                   const { return (pl_columns); }
    void set_columns(int c)         { pl_columns = c; }

    sDirList *dirs()                const { return (pl_dirs); }
    void set_dirs(sDirList *d)      { pl_dirs = d; }

    const char *path_string()       const { return (pl_path_string); }
    CheckFuncType checkfunc()       const { return (pl_checkfunc); }
    bool incldirs()                 const { return (pl_incldirs); }
    const char *no_files_msg()      const { return (pl_no_files_msg); }

private:
    char *pl_path_string;           // copy of original path string
    CheckFuncType pl_checkfunc;     // screening function for files
    const char *pl_no_files_msg;    // file list text when no files found
    const char *pl_top_header;      // header text at top of listing
    const char *pl_dir_header;      // first line of file listing
    int pl_columns;                 // columns in display window
    sDirList *pl_dirs;              // list of directories
    bool pl_incldirs;               // include subdirectories in file list
};


//
// A path parser, generator.
//

#ifdef WIN32
#define PATH_SEP ';'
#else
#define PATH_SEP ':'
#endif

// Redirect Files for pathgen
//
// A file by the name defined below found in a path directory
// (recursively) will be read.  It contains directories which are
// effectively added to the search path.  Unrooted directories are
// relative to the directory containing the file.  Lines that start
// with '#' and lines that contain only white space are ignored. 
// E.g., if dir is a directory from the search path, and that
// directory contains the following file:
//
//    # I'm a redirect file
//    subdir anothersubdir
//    /usr/local/foo
//
// The dir element of the path becomes effectively
//     dir dir/subdir dir/anothersubdir /usr/local/foo
//
// Note that directories can be placed on the same line, or different
// lines.  Directories that contain white space should be
// double-quoted.  Only paths that actually point to an existing
// directory will be used.  The new directories can in turn contain
// redirect files.
//
#define REDIRECT_FNAME "xt_redirect"


// Generator for search path decomposition.  This handles paths in the
// form ( xxx xxx ) as well as xxx:xxx.  In the first form, the tokens
// can be single or double quoted if they contain white space.  In the
// second form, no quoting is allowed - the separation character
// serves this purpose.  These can optionally lead or trail the path
// if the first or last component contain white space.
//
// Call nextpath to get each component (which should be freed), until
// 0 is returned.
//
struct pathgen
{
    pathgen(const char*);
    ~pathgen()
        {
            delete [] pg_string;
        }

    char *nextpath(bool, int* = 0, int* = 0);

    bool style()            const { return (pg_style); }

private:
    void read_redirect(const char*);

    char *pg_string;        // copy of full path
    char *pg_sptr;          // current location
    stringlist *pg_rdlist;  // redirection list
    int pg_offset;          // char counting
    bool pg_style;          // true when space used as delimiter
};

#endif


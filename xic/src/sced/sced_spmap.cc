
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

#include "main.h"
#include "sced_spiceipc.h"
#include "sced_spmap.h"
#include "pathlist.h"


// A table of tables of library tag offsets.
//

#define LM_NO_FILE -1
#define LM_NO_NAME -2

sLibMap::~sLibMap()
{
    SymTabGen gen(lib_tab, true);
    SymTabEnt *ent;
    while ((ent = gen.next()) != 0) {
        delete (SymTab*)ent->stData;
        delete ent;
    }
    delete lib_tab;
}


// If word is non-null, prepend it to the deck.  Expand all .inc/.lib
// type lines.  On success, a new expanded deck is returned (the
// original deck is freed).  On error, false is returned, and the
// original deck is retained.
//
bool
sLibMap::expand_includes(stringlist **deck, const char *word)
{
    stringlist *s0 = 0, *se = 0;
    s0 = new stringlist(lstring::copy(word), s0);
    se = s0;

    Errs()->init_error();
    bool err = false;
    for (stringlist *dd = *deck; dd; dd = dd->next) {
        char *s = dd->string;
        while (isspace(*s))
            s++;
        if (*s) {
            // Forms like ".inc h" will turn on HSPICE comments forms
            // for the included file and below.  One potential issue
            // is when the present deck and includes have HSPICE
            // comments, and don't use the WRspice 'h' forms.  In this
            // case, WRspice should take care of things, if ".options
            // hspice" is given.

            if (lstring::cimatch(".inc", s) || lstring::cimatch(".include", s))
                se = append_file_rc(se, s, false, &err);
            else if (lstring::cimatch(".spinclude", s)) {
                // change to ".include"
                se->next = new stringlist(lstring::copy(s+2), 0);
                se = se->next;
                *se->string = '.';
            }
            else if (lstring::cimatch(".lib", s))
                se = append_file_rc(se, s, false, &err);
            else if (lstring::cimatch(".splib", s)) {
                // change to ".lib"
                se->next = new stringlist(lstring::copy(s+2), 0);
                se = se->next;
                *se->string = '.';
            }
            else {
                se->next = new stringlist(lstring::copy(s), 0);
                se = se->next;
            }
        }
        if (err) {
            stringlist::destroy(s0);
            return (false);
        }
    }
    stringlist::destroy(*deck);
    if (word)
        *deck = s0;
    else {
        *deck = s0->next;
        delete s0;
    }
    return (true);
}


// Append, recursively, the file mentioned in the .include or block
// given in a .lib line.  This is expanded here, rather than in
// WRspice, since WRspice may be on a remote system.
//
stringlist *
sLibMap::append_file_rc(stringlist *se, const char *line, bool hs_compat,
    bool *error)
{
    *error = false;

    // line has no leading white space
    if (lstring::cimatch(".inc", line) || lstring::cimatch(".include", line)) {
        char *tbf = new char[strlen(line) + 8];
        sprintf(tbf, "* %s", line);
        se->next = new stringlist(tbf, 0);
        se = se->next;

        const char *lp = line;
        lstring::advtok(&lp);
        char *path = lstring::getqtok(&lp);
        if (path && lstring::cieq(path, "h")) {
            hs_compat = true;
            delete [] path;
            path = lstring::getqtok(&lp);
        }
        if (path) {
            char *ptmp = pathlist::expand_path(path, false, false);
            if (ptmp) {
                delete [] path;
                path = ptmp;
            }

            // Strip off the path, we're going to chdir there for the
            // duration of the read.
            char *file = lstring::strrdirsep(path);
            if (file)
                *file++ = 0;
            else {
                file = path;
                path = 0;
            }
            char *cwd = getcwd(0, 0);
            if (path)
                chdir(path);

            FILE *fp = fopen(file, "r");
            if (fp) {
                for (;;) {
                    char *buf = cSpiceIPC::ReadLine(fp, hs_compat);
                    if (!buf)
                        break;
                    char *s = buf;
                    while (isspace(*s))
                        s++;

                    if (lstring::cimatch(".inc", s) ||
                            lstring::cimatch(".include", s))
                        se = append_file_rc(se, s, hs_compat, error);
                    else if (lstring::cimatch(".spinclude", s)) {
                        // change to ".include"
                        se->next = new stringlist(lstring::copy(s+2), 0);
                        se = se->next;
                        *se->string = '.';
                    }
                    else if (lstring::cimatch(".lib", s))
                        se = append_file_rc(se, s, hs_compat, error);
                    else if (lstring::cimatch(".splib", s)) {
                        // change to ".lib"
                        se->next = new stringlist(lstring::copy(s+2), 0);
                        se = se->next;
                        *se->string = '.';
                    }
                    else {
                        se->next = new stringlist(lstring::copy(s), 0);
                        se = se->next;
                    }
                    delete [] buf;
                    if (*error)
                        break;
                }
                fclose(fp);
            }
            else {
                Errs()->add_error("Couldn't open include file %s.", file);
                *error = true;
            }

            if (path) {
                chdir(cwd);
                delete [] path;
            }
            else
                delete [] file;
            free(cwd);
        }
        else {
            Errs()->add_error("Bad syntax: %s\nNo file specified.", line);
            *error = true;
        }

        se->next = new stringlist(lstring::copy("* end of inclusion"), 0);
        se = se->next;
    }
    else if (lstring::cimatch(".lib", line)) {
        char *tbf = new char[strlen(line) + 8];
        sprintf(tbf, "* %s", line);
        se->next = new stringlist(tbf, 0);
        se = se->next;

        const char *lp = line;
        lstring::advtok(&lp);
        char *path = lstring::getqtok(&lp);
        if (path && lstring::cieq(path, "h")) {
            hs_compat = true;
            delete [] path;
            path = lstring::getqtok(&lp);
        }
        char *name = lstring::gettok(&lp);

        if (name) {
            char *ptmp = pathlist::expand_path(path, false, false);
            if (ptmp) {
                delete [] path;
                path = ptmp;
            }

            // Strip off the path, we're going to chdir there for the
            // duration of the read.
            char *file = lstring::strrdirsep(path);
            if (file)
                *file++ = 0;
            else {
                file = path;
                path = 0;
            }
            char *cwd = getcwd(0, 0);
            if (path && chdir(path) < 0) {
                Errs()->sys_error("chdir");
                Errs()->add_error(
                    "Failed to change directory to %s.\n", path);
                *error = true;
            }

            long offs = 0;
            if (!*error) {
                offs = find(file, name);
                if (offs == LM_NO_NAME) {
                    Errs()->add_error(
                        "Block %s not found in library %s.\n", name, file);
                    *error = true;
                }
            }

            FILE *fp = 0;
            if (!*error) {
                if (offs == LM_NO_FILE || (fp = fopen(file, "r")) == 0) {
                    Errs()->sys_error(file);
                    Errs()->add_error(
                        "Failed to open library file %s.\n", file);
                    *error = true;
                }
            }

            if (!*error) {
                if (fseek(fp, offs, SEEK_SET) < 0) {
                    Errs()->sys_error(file);
                    Errs()->add_error(
                        "Seek failed in library file %s.\n", file);
                    *error = true;
                }
            }
            while (!*error) {
                char *buf = cSpiceIPC::ReadLine(fp, hs_compat);
                if (!buf)
                    break;
                char *s = buf;
                while (isspace(*s))
                    s++;
                if (lstring::cimatch(".inc", s) ||
                        lstring::cimatch(".include", s))
                    se = append_file_rc(se, s, hs_compat, error);
                else if (lstring::cimatch(".spinclude", s)) {
                    // change to ".include"
                    se->next = new stringlist(lstring::copy(s+2), 0);
                    se = se->next;
                    *se->string = '.';
                }
                else if (lstring::cimatch(".lib", s))
                    se = append_file_rc(se, s, hs_compat, error);
                else if (lstring::cimatch(".splib", s)) {
                    // change to ".lib"
                    se->next = new stringlist(lstring::copy(s+2), 0);
                    se = se->next;
                    *se->string = '.';
                }
                else if (lstring::cimatch(".endl", s)) {
                    delete [] buf;
                    break;
                }
                else {
                    se->next = new stringlist(lstring::copy(s), 0);
                    se = se->next;
                }
                delete [] buf;
            }
            if (fp)
                fclose(fp);

            if (path) {
                chdir(cwd);
                delete [] path;
            }
            else
                delete [] file;
            free(cwd);
        }
        else {
            Errs()->add_error("Bad syntax: %s\nPath or block name missing.",
                line);
            *error = true;
        }

        se->next = new stringlist(lstring::copy("* end of .lib inclusion"), 0);
        se = se->next;
    }
    return (se);
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
        lib_tab = new SymTab(true, false);

    char *cwd = getcwd(0, 0);
    char *fullpath = new char[strlen(cwd) + strlen(file) + 2];
    char *t = lstring::stpcpy(fullpath, cwd);
    *t++ = '/';
    strcpy(t, file);
    free(cwd);

    SymTab *tab = 0;
    SymTabEnt *ent = SymTab::get_ent(lib_tab, fullpath);
    if (!ent) {
        FILE *fp = fopen(file, "r");
        if (!fp) {
            delete [] fullpath;
            return (LM_NO_FILE);
        }
        tab = map_lib(fp);
        fclose(fp);
        lib_tab->add(fullpath, tab, false);
    }
    else {
        tab = (SymTab*)ent->stData;
        delete [] fullpath;
    }

    // Tags are case-insensitive, lower case in the table.
    char *lcname = lstring::copy(name);
    for (char *tt = lcname; *tt; tt++) {
        if (isupper(*tt))
            *tt = tolower(*tt);
    }

    ent = SymTab::get_ent(tab, lcname);
    delete [] lcname;
    if (!ent)
        return (LM_NO_NAME);
    return ((long)ent->stData);
}


// Private function to map the .lib offsets in a file, returning a
// hash table listing offsets by name.
//
SymTab *
sLibMap::map_lib(FILE *fp)
{
    // Tags are case-insensitive, the tab key is lower-cased.
    SymTab *tab = new SymTab(true, false);

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
            if (c != 'i' && c != 'B') {
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
                    char *tag = lstring::copy(s);
                    for (char *tt = tag; *tt; tt++) {
                        if (isupper(*tt))
                            *tt = tolower(*tt);
                    }
                    unsigned long offs = ftell(fp);
                    tab->add(tag, (void*)offs, false);
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
    return (tab);
}



/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2007 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *  WHITELEY RESEARCH INCORPORATED PROPRIETARY SOFTWARE                   *
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
 *                                                                        *
 * XIC Integrated Circuit Layout and Schematic Editor                     *
 *                                                                        *
 *========================================================================*
 $Id: fio_library.h,v 5.17 2015/04/18 16:44:56 stevew Exp $
 *========================================================================*/

#ifndef CD_LIBRARY_H
#define CD_LIBRARY_H

class cCHD;

// Defines for library search mode, and to specify a library type.
//
#define LIBdevice       1
#define LIBuser         2
#define LIBnativeOnly   4
//
//  LIBdevice       Search device library (device.lib)
//  LIBuser         Search user libraries
//  LIBnativeOnly   Restrict search to inline and first-level native
//                  cells.

// Symbol table element for library references and cells.  Strings are
// pointers into name table, don't free!
//
struct sLibRef
{
    const char *tab_name()    const { return (lr_name); }

    // We need to hide the alias flag somewhere, adding a bool member
    // would be costly in space.  The only field that is not
    // byte-aligned is the next element pointer, so we can use that
    // lsb.

    sLibRef *tab_next()       const {
        return ((sLibRef*)(lr_next_and_flag & ~1)); }
    void set_tab_next(sLibRef *n) {
        lr_next_and_flag = (unsigned long)n | (lr_next_and_flag & 1); }

    void set_alias_flag(bool b)
        {
            if (b)
                lr_next_and_flag |= 1;
            else
                lr_next_and_flag &= ~1;
        }

    bool alias_flag()         const { return (lr_next_and_flag & 1); }

    sLibRef *tgen_next(bool)  const {
            return ((sLibRef*)(lr_next_and_flag & ~1)); }

    void clear()
        {
            lr_next_and_flag = 0;
            lr_name = 0;
            lr_dir = 0;
            lr_file = 0;
            lr_u.offset = 0;
        }

    const char *name()        const { return (lr_name); }
    void set_name(const char *n)    { lr_name = n; }
    const char *dir()         const { return (lr_dir); }
    void set_dir(const char *d)     { lr_dir = d; }
    const char *file()        const { return (lr_file); }
    void set_file(const char *f)    { lr_file = f; }
    long offset()             const { return (lr_u.offset); }
    void set_offset(long o)         { lr_u.offset = o; }
    const char *cellname()    const { return (lr_u.cellname); }
    void set_cellname(const char *s) { lr_u.cellname = s; }

private:
    unsigned long lr_next_and_flag; // table link pointer and alias flag
    const char *lr_name;        // key name for reference

    // The path to the file is saved as a (rooted) directory path and
    // a file name (without path).  Both are saved in the string
    // table.  If there are a large number of different files in the
    // same directory, we avoid saving the possibly lengthly directory
    // path multiple times.

    const char *lr_dir;         // directory path to target file
    const char *lr_file;        // target file name
    union {
        long offset;            // offset to cell defined in library
        const char *cellname;   // name of reference if cellfile is
                                //  archive or library
    } lr_u;
};

// Symbol table for library references.
//
struct libtab_t
{
    libtab_t() { lb_tab = new table_t<sLibRef>; }
    ~libtab_t() { delete lb_tab; }

    sLibRef *find(const char *string)
        {
            if (!string || !lb_tab)
                return (0);
            return (lb_tab->find(string));
        }

    sLibRef *add(sLibRef *ref)
        {
            if (!ref)
                return (0);
            lb_tab->link(ref, false);
            lb_tab = lb_tab->check_rehash();
            return (ref);
        }

    table_t<sLibRef> *table() { return (lb_tab); }

private:
    table_t<sLibRef> *lb_tab;       // offset hash table
};

// List element for open libraries
//
struct sLib
{
    sLib(const char*, int);
    ~sLib();

    const char *filename()          { return (l_libfilename); }
    libtab_t &symtab()              { return (l_symtab); }
    int lib_type()                  { return (l_type); }

    static sLib *open_library(sLib*, const char*, const char*, bool*);
    static sLib *find(sLib*, const char*);
    static FILE *open_file(sLib*, const char*, const char*, int, sLibRef**,
        sLib**);
    static OItype open_cell(sLib*, const char*, const char*, int, CDcbin*);
    static sLib *lookup(sLib*, const char*, const char*, int, sLibRef**);
    static stringlist *namelist(sLib*, const char*, int);
    static const stringlist *properties(const sLib*, const char*);
    static stringlist *list(const sLib*, int);
    static sLib *close_library(sLib*, const char*, int);

    cCHD *get_chd(const sLibRef*);
    void set_chd(const sLibRef*, cCHD*);

private:
    bool match_name(const char *libname) const
        {
            if (libname) {
                return (!strcmp(libname, l_libfilename) ||
                    !strcmp(libname, lstring::strip_path(l_libfilename)));
            }
            return (false);
        }

    bool match_type_and_name(int type, const char *libname) const
        {
            return ((l_type & type) &&
                (!libname || !strcmp(libname, l_libfilename) ||
                !strcmp(libname, lstring::strip_path(l_libfilename))));
        }

    sLibRef *new_libref(const char*, const char*, const char*);
    sLibRef *new_libref(const char*, long);

    sLib *l_nextlib;                // next in list
    const char *l_libfilename;      // full path to library file
    stringlist *l_prpty_strings;    // properties for LIBdevice
    SymTab *l_chdtab;               // CHDs for archive access
    strtab_t l_nametab;             // string table
    libtab_t l_symtab;              // table of references and cells
    eltab_t<sLibRef> l_lref_elts;   // table element factory
    int l_type;                     // LIBdevice or LIBuser
};

#endif


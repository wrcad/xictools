
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

#include "fio.h"
#include "fio_library.h"
#include "fio_alias.h"
#include "fio_chd.h"
#include "si_parsenode.h"
#include "si_parser.h"
#include "si_interp.h"
#include "si_macro.h"
#include "cd_celldb.h"
#include "pathlist.h"
#include "filestat.h"

#include <ctype.h>
#include <unistd.h>
#include <dirent.h>


//
// Library access functions.
//

// Return true if fp points to a library file
//
bool
cFIO::IsLibrary(FILE *fp)
{
    char buf[16];
    memset(buf, 0, 16);  // for valgrind
    fread(buf, 1, 16, fp);
    rewind(fp);
    if (!strncmp(buf, "(Library ", 9))
        return (true);
    return (false);
}


// Return a list of the libraries currently open, according to type.
//
stringlist *
cFIO::ListLibraries(int type)
{
    return (sLib::list(fioLibraries, type));
}


// Create an sLib struct containing list of cell names, etc., and
// add to the Libraries list.
//
bool
cFIO::OpenLibrary(const char *path, const char *name)
{
    return (sLib::open_library(&fioLibraries, path, name));
}


// Return the sLib* if libname is in the open list.
//
sLib *
cFIO::FindLibrary(const char *libname)
{
    return (sLib::find(fioLibraries, libname));
}


// Return the property strings list for the library named.  Only LIBdevice
// libraries have properties.
//
const stringlist *
cFIO::GetLibraryProperties(const char *libname)
{
    return (sLib::properties(fioLibraries, libname));
}


// Close and free the named library, and delete the cells from the
// database if lib is LIBdevice.  If name is null, clear all
// libraries and library cells.
//
void
cFIO::CloseLibrary(const char *libname, int type)
{
    sLib::close_library(&fioLibraries, libname, type);
}


// Return a list of library access names from the named library, or
// from all libraries is name is null.  Use type to filter the search.
//
stringlist *
cFIO::GetLibNamelist(const char *libname, int type)
{
    return (sLib::namelist(fioLibraries, libname, type));
}


// Return a FILE pointer to the named object, searching libraries
// according to type.  If a match is found, return a file pointer to
// the object, and return the sLibRef and sLib.
//
FILE *
cFIO::OpenLibFile(const char *libname, const char *name, int type,
    sLibRef **ref, sLib **plib)
{
    return (sLib::open_file(fioLibraries, libname, name, type, ref, plib));
}


// Open a cell indirected from name in libname.
//
OItype
cFIO::OpenLibCell(const char *libname, const char *name, int type,
    CDcbin *cbin)
{
    return (sLib::open_cell(fioLibraries, libname, name, type, cbin));
}


// Similar to OpenLibFile(), but the reference is not opened.  If the
// reference is found, return the sLib*, and set the ref pointer.
//
sLib *
cFIO::LookupLibCell(const char *libname, const char *name, int type,
    sLibRef **ref)
{
    return (sLib::lookup(fioLibraries, libname, name, type, ref));
}


// Try to resolve a symref with the defseen flag not set through the
// library mechanism.  If the library contains an archive that
// resolves the name, a CHD is returned.
//
// Returns:
//  RESOLVerror     error
//  RESOLVnone      no resolving cell found in library
//  RESOLVok        cell resolved
//
RESOLVtype
cFIO::ResolveLibSymref(symref_t *p, cCHD **chdptr, bool *is_devlib)
{
    if (chdptr)
        *chdptr = 0;
    if (is_devlib)
        *is_devlib = false;
    if (!p) {
        Errs()->add_error("ResolveLibrarySymref: null symref pointer.");
        return (RESOLVerror);
    }

    sLibRef *libref;
    sLib *lib = LookupLibCell(0, Tstring(p->get_name()),
        LIBdevice | LIBuser, &libref);
    if (!lib || !libref)
        return (RESOLVnone);

    cCHD *chd = lib->get_chd(libref);
    if (chd) {
        if (chdptr)
            *chdptr = chd;
        return (RESOLVok);
    }

    if (!libref->dir()) {
        // inline cell
        if (is_devlib && lib->lib_type() == LIBdevice)
            *is_devlib = true;
        return (RESOLVok);
    }

    char *fullpath = pathlist::mk_path(libref->dir(), libref->file());
    FILE *fp = POpen(fullpath, "r");
    if (!fp) {
        Errs()->add_error("Bad library reference:\n can't open %s.",
            fullpath);
        delete [] fullpath;
        return (RESOLVerror);
    }
    FileType ft = GetFileType(fp);
    fclose(fp);

    if (ft == Fgds || ft == Foas || ft == Fcgx || ft == Fcif) {
        FIOaliasTab *atab = new FIOaliasTab(false, false);
        atab->add_lib_alias(libref, lib);
        atab->set_auto_rename(IsAutoRename());
        // Create with ful info counts for empty cell pre-filtering.
        chd = NewCHD(fullpath, ft, Electrical, atab, cvINFOplpc);
        delete atab;
        delete [] fullpath;
        if (chd) {
            lib->set_chd(libref, chd);
            *chdptr = chd;
            return (RESOLVok);
        }
        Errs()->add_error("ResolveLibrarySymref: CHD creation failed.");
        return (RESOLVerror);
    }
    if (ft == Fnative) {
        if (is_devlib && lib->lib_type() == LIBdevice)
            *is_devlib = true;
        delete [] fullpath;
        return (RESOLVok);
    }
    Errs()->add_error("Bad library reference:\n unknown format %s.",
        fullpath);
    delete [] fullpath;
    return (RESOLVerror);
}


// Function to attempt to resolve a symref with the defseen flag
// unset, which means that the cell was referenced but not defined in
// the source file for the CHD.
//
RESOLVtype
cFIO::ResolveUnseen(cCHD **pchd, symref_t **ppx)
{
    symref_t *cp = *ppx;

    DisplayMode mode = cp->mode();
    cCHD *rchd;
    bool isdev;
    RESOLVtype rval = ResolveLibSymref(cp, &rchd, &isdev);
    if (rval == RESOLVerror)
        return (RESOLVerror);

    bool inlib = false;
    if (rval == RESOLVok) {
        // Found the cell in a library.  If it came from an archive
        // file, we'll get a CHD.
        if (rchd) {
            symref_t *px = rchd->findSymref(cp->get_name(), mode);
            if (!px) {
                Errs()->add_error(
                    "ResolveUnseen: symref %s not found in CHD.",
                    Tstring(cp->get_name()));
                return (RESOLVerror);
            }
            *pchd = rchd;
            *ppx = px;
            return (RESOLVok);
        }
        inlib = true;

        // The cell was native, perhaps inlined.  We'll try and open
        // it below.
    }

    // If we find the cell in memory, set up the BB which is probably
    // needed later for the tables.

    CDs *sd = CDcdb()->findCell(cp->get_name(), mode);
    if (sd && (sd->isViaSubMaster() || sd->isPCellSubMaster())) {
        // Cell is a via or pcell, we keep these.

        cp->set_bb(sd->BB());
        cp->set_bbok(true);
        return (RESOLVmem);
    }

    if (inlib) {
        // Open library cell, if successful we keep these.

        OItype oiret = OpenLibCell(0, Tstring(cp->get_name()),
            LIBdevice | LIBuser, 0);
        if (oiret == OIok || oiret == OIold) {
            CDs *tsd = CDcdb()->findCell(cp->get_name(), mode);
            if (tsd) {
                cp->set_bb(tsd->BB());
                cp->set_bbok(true);
                return (RESOLVmem);
            }
        }
    }

    // May want to add an option to let existing cells in memory
    // resolve symref.  Presently they are ignored unless they are a
    // via/pcell sub-master or come from a library.

    // Note that memory cells that we resolve here are included in the
    // tables, and will be processed similar to Override cells when
    // reading cell data, i.e., data from the memory cell will be
    // entered into output as if read through the CHD.

    if (IsChdFailOnUnresolved()) {
        Errs()->add_error("ResolveUnseen: unresolved symref %s%s",
            Tstring(cp->get_name()), sd ? ", but found in memory." : ".");
        return (RESOLVerror);
    }
    return (RESOLVnone);
}
// End of cFIO functions


sLib::sLib(const char *lfn, int t)
{
    l_nextlib = 0;
    l_libfilename = l_nametab.add(lfn);
    l_prpty_strings = 0;
    l_chdtab = 0;
    l_type = t;
}


sLib::~sLib()
{
    stringlist::destroy(l_prpty_strings);

    SymTabGen gen(l_chdtab, true);
    SymTabEnt *h;
    while ((h = gen.next()) != 0) {
        delete (cCHD*)h->stData;
        delete h;
    }
    delete l_chdtab;
}


// Static function.
// Open a new library.  The searchpath is a search path, used to find
// fname, the file name.  Return false on error, with a message in
// Errs.  The list of libraries in *plib is updated.
//
bool
sLib::open_library(sLib **plib, const char *searchpath, const char *fname)
{
    if (!plib || !fname) {
        Errs()->add_error("open_library: null argument.");
        return (false);
    }
    char *fullname;
    FILE *fp = pathlist::open_path_file(fname, searchpath, "rb", &fullname,
        false);
    if (!fp) {
        Errs()->add_error("Unable to open %s.", fname);
        return (false);
    }
    for (sLib *lib = *plib; lib; lib = lib->l_nextlib) {
        if (!strcmp(fullname, lib->l_libfilename)) {
            // already open
            delete [] fullname;
            fclose(fp);
            return (true);
        }
    }
    if (!FIO()->IsLibrary(fp)) {
        delete [] fullname;
        fclose(fp);
        Errs()->add_error("Not a library: %s.", fname);
        return (false);
    }

    const char *devlib_name =  FIO()->ifDeviceLibName();
    if (!devlib_name)
        devlib_name = "device.lib";

    // LIBdevice designates the "device.lib" library.
    int type = (!strcmp(lstring::strip_path(fullname), devlib_name) ?
        LIBdevice : LIBuser);
    sLib *lib = new sLib(fullname, type);
    delete [] fullname;

    char buf[MACRO_BUFSIZE];
    SImacroHandler macros;

    stringlist *aliases = 0;
    bool in_symbol = false;
    stringlist *se = 0;
    long ftold = 0, start = 0;
    for ( ; fgets(buf, MACRO_BUFSIZE, fp) != 0; ftold = ftell(fp)) {
        NTstrip(buf);
        macros.inc_line_number();
        char *c = buf;
        while (isspace(*c))
            c++;
        if (!*c)
            continue;
        if (!in_symbol) {
            if (*c == '#')
                // comment line, these can appear outside of cell defs
                continue;
            if (lstring::ciprefix("property ", c)) {
                c += 9;
                while (isspace(*c))
                    c++;
                if (!lib->l_prpty_strings) {
                    se = new stringlist(lstring::copy(c), 0);
                    lib->l_prpty_strings = se;
                }
                else {
                    se->next = new stringlist(lstring::copy(c), 0);
                    se = se->next;
                }
                continue;
            }
            if (lstring::ciprefix("alias ", c)) {
                lstring::advtok(&c);
                char *alias = lstring::gettok(&c);  // name given
                char *name = lstring::gettok(&c);   // mapped to name
                if (name) {
                    stringlist *s0 = new stringlist(alias, aliases);
                    aliases = new stringlist(name, s0);
                }
                else
                    delete [] alias;
                continue;
            }
            if (lstring::ciprefix("reference ", c)) {
                c += 10;
                while (isspace(*c))
                    c++;
                char *n = lstring::gettok(&c);
                char *p = lstring::getqtok(&c);
                if (p) {
                    char *cp = macros.macro_expand(p);
                    delete [] p;
                    p = cp;
                }
                char *s = lstring::gettok(&c);
                if (n && p)
                    lib->new_libref(n, p, s);
                delete [] n;
                delete [] p;
                delete [] s;
                continue;
            }
            if (lstring::ciprefix("directory ", c)) {
                c += 10;
                while (isspace(*c))
                    c++;
                char *p = lstring::getqtok(&c);
                if (p) {
                    char *cp = macros.macro_expand(p);
                    delete [] p;
                    p = cp;
                    DIR *wdir = opendir(p);
                    if (wdir) {
                        char *path = new char[strlen(p) + 256];
                        strcpy(path, p);
                        char *t = path + strlen(path) - 1;
                        if (!lstring::is_dirsep(*t)) {
                            t++;
                            *t++ = '/';
                            *t = 0; 
                        }
                        else
                            t++;

                        struct dirent *de;
                        while ((de = readdir(wdir)) != 0) {
                            if (!strcmp(de->d_name, "."))
                                continue;
                            if (!strcmp(de->d_name, ".."))
                                continue;
                            const char *tmp = strrchr(de->d_name, '.');
                            if (tmp && lstring::cieq(tmp, ".bak"))
                                continue;
                            strcpy(t, de->d_name);
                            if (filestat::get_file_type(path) != GFT_FILE)
                                continue;

                            FILE *tp = fopen(path, "r");
                            if (!tp)
                                continue;
                            bool goodone = (FIO()->IsCIF(tp) ||
                                FIO()->IsGDSII(tp) || FIO()->IsOASIS(tp) ||
                                FIO()->IsCGX(tp) || FIO()->IsLibrary(tp));
                            fclose(tp);
                            if (!goodone)
                                continue;
                            lib->new_libref(de->d_name, path, 0);
                        }
                        delete [] path;
                        closedir(wdir);
                    }
                }
                continue;
            }

            char *emsg;
            if (macros.handle_keyword(fp, c, 0, fname, &emsg)) {
                if (emsg) {
                    FIO()->ifInfoMessage(IFMSG_POP_WARN, "%s\n", emsg);
                    delete [] emsg;
                }
                continue;
            }

            if (lstring::ciprefix("(Symbol ", c) ||
                    lstring::ciprefix("(PHYSICAL", c)) {
                in_symbol = true;
                start = ftold;
                continue;
            }
            if (lstring::ciprefix("(ELECTRICAL", c)) {
                in_symbol = true;
                continue;
            }
        }
        else {
            if (*c == '9' && isspace(*(c+1))) {
                c = buf + 1;
                while (isspace(*c))
                    c++;
                char *d;
                for (d = c; d; d++) {
                    if (isspace(*d) || *d == ';') {
                        *d = '\0';
                        break;
                    }
                }
                if (d == c)
                    continue;
                lib->new_libref(c, start);
            }
            else if (*c == 'E' && (isspace(*(c+1)) || *(c+1) == '\0'))
                in_symbol = false;
        }
    }
    fclose(fp);

    if (aliases) {
        for (stringlist *sl = aliases; sl; sl = sl->next) {
            const char *name = sl->string;
            sl = sl->next;
            const char *alias = sl->string;

            sLibRef *r = lib->l_symtab.find(name);
            if (r) {
                sLibRef *rnew = lib->l_lref_elts.new_element();
                *rnew = *r;
                rnew->set_tab_next(0);
                rnew->set_alias_flag(true);
                rnew->set_name(lib->l_nametab.add(alias));
                lib->l_symtab.add(rnew);
            }
        }
        stringlist::destroy(aliases);
    }

    // Link the new library into the list.  The LIBdevice libraries
    // are listed first in order of opening, followed by the LIBuser
    // libraries.
    if (!*plib)
        *plib = lib;
    else if (lib->l_type == LIBdevice) {
        sLib *lp = 0;
        for (sLib *l = *plib; l; l = l->l_nextlib) {
            if (l->l_type == LIBuser) {
                lib->l_nextlib = l;
                if (!lp)
                    *plib = lib;
                else
                    lp->l_nextlib = lib;
                break;
            }
            else if (!l->l_nextlib) {
                l->l_nextlib = lib;
                break;
            }
            lp = l;
        }
    }
    else {
        sLib *l = *plib;
        while (l->l_nextlib)
            l = l->l_nextlib;
        l->l_nextlib = lib;
    }
    if (lib->l_type == LIBuser)
        FIO()->ifLibraryChange();
    return (true);
}


// Static function.
// Close and free the named library, and delete the cells from the
// database if the named library has type LIBdevice.  If name is null,
// clear all libraries and library cells.  Return true if something
// was changed.
//
bool
sLib::close_library(sLib **plib, const char *libname, int type)
{
    bool changed = false;
    sLib *lp = 0, *ln;
    for (sLib *lib = *plib; lib; lib = ln) {
        ln = lib->l_nextlib;
        if (lib->match_type_and_name(type, libname)) {
            if (!lp)
                *plib = ln;
            else
                lp->l_nextlib = ln;
            if (lib->l_type == LIBdevice) {
                // close all cells
                tgen_t<sLibRef> gen(lib->l_symtab.table());
                sLibRef *r;
                while ((r = gen.next()) != 0)
                    CD()->Close(CD()->CellNameTableFind(r->name()));
            }
            delete lib;
            changed = true;
            continue;
        }
        lp = lib;
    }
    if (changed)
        FIO()->ifLibraryChange();
    return (changed);
}


// Static function.
// Return the library matching the file or path name given.
//
sLib *
sLib::find(sLib *thislib, const char *libname)
{
    for (sLib *lib = thislib; lib; lib = lib->l_nextlib) {
        if (lib->match_name(libname))
            return (lib);
    }
    return (0);
}


// Static function.
// Open the file indirected from the name in libname according to
// type.  The reference and library pointers are returned in the
// arguments, on success.
//
FILE *
sLib::open_file(sLib *thislib, const char *libname, const char *name, int type,
    sLibRef **ref, sLib **plib)
{
    if (ref)
        *ref = 0;
    if (plib)
        *plib = 0;
    for (sLib *lib = thislib; lib; lib = lib->l_nextlib) {
        if (lib->match_type_and_name(type, libname)) {
            sLibRef *r = lib->l_symtab.find(name);
            if (r) {
                FILE *fp = 0;
                if (r->dir()) {
                    char *p = pathlist::mk_path(r->dir(), r->file());
                    fp = fopen(p, "r");
                    delete [] p;
                    if (!fp)
                        return (0);
                    if (type & LIBnativeOnly) {
                        bool issced = false;
                        CFtype ciftype = CFnone;
                        if (!FIO()->IsCIF(fp, &ciftype, &issced) ||
                                ciftype != CFnative) {
                            fclose(fp);
                            return (0);
                        }
                        rewind(fp);
                    }
                }
                else {
                    fp = fopen(lib->l_libfilename, "r");
                    if (fp == 0)
                        continue;
                    fseek(fp, r->offset(), 0);
                }
                if (ref)
                    *ref = r;
                if (plib)
                    *plib = lib;
                return (fp);
            }
        }
    }
    return (0);
}


// Static function.
// Open a cell through library references, if possible.  This returns ok
// if the cell is not found, one should examine cbin to determine if a
// cell was actually read.
//
OItype
sLib::open_cell(sLib *thislib, const char *libname, const char *name,
    int type, CDcbin *cbin)
{
    OItype oiret = OIok;
    sLibRef *libref;
    sLib *lib = lookup(thislib, libname, name, type, &libref);
    if (lib) {
        if ((type & LIBnativeOnly) || !libref->dir()) {
            // inline cell
            oiret = FIO()->OpenNative(name, cbin, 1.0);
        }
        else {
            oiret = FIO()->OpenImport(lib->filename(), FIO()->DefReadPrms(),
                libref->name(), 0, cbin, 0, 0, 0);
        }
        if (oiret == OIok) {
            cbin->setLibrary(true);
            cbin->setImmutable(true);
            if (lib->lib_type() == LIBdevice)
                cbin->setDevice(true);
        }
    }
    else if ((type & LIBuser) && !(type & LIBnativeOnly))
        FIO()->ifOpenOA(libname, name, cbin);
    return (oiret);
}


// Static function.
// If name exists in libname, return the lib pointer and a pointer to
// the reference struct.
//
sLib *
sLib::lookup(sLib *thislib, const char *libname, const char *name, int type,
    sLibRef **ref)
{
    if (ref)
        *ref = 0;
    for (sLib *lib = thislib; lib; lib = lib->l_nextlib) {
        if (lib->match_type_and_name(type, libname)) {
            sLibRef *r = lib->l_symtab.find(name);
            if (r) {
                if (r->dir() && (type & LIBnativeOnly)) {
                    char *p = pathlist::mk_path(r->dir(), r->file());
                    FILE *fp = fopen(p, "r");
                    delete [] p;
                    if (!fp)
                        return (0);
                    bool issced = false;
                    CFtype ciftype = CFnone;
                    if (!FIO()->IsCIF(fp, &ciftype, &issced) ||
                            ciftype != CFnative) {
                        fclose (fp);
                        return (0);
                    }
                    fclose (fp);
                }
                if (ref)
                    *ref = r;
                return (lib);
            }
        }
    }
    return (0);
}


// Static function.
// Return a list of library access names from the named library, or
// from all libraries is name is null.  Use type to filter the search.
//
// Aliases are not included.
//
stringlist *
sLib::namelist(sLib *thislib, const char *libname, int type)
{
    stringlist *s0 = 0;
    for (sLib *lib = thislib; lib; lib = lib->l_nextlib) {
        if (lib->match_type_and_name(type, libname)) {
            tgen_t<sLibRef> gen(lib->l_symtab.table());
            sLibRef *r;
            while ((r = gen.next()) != 0) {
                if (!r->alias_flag())
                    s0 = new stringlist(lstring::copy(r->name()), s0);
            }
        }
    }
    stringlist::sort(s0);
    return (s0);
}


// Static function.
// Return the property strings list for the library named.  Only LIBdevice
// libraries have properties.
//
const stringlist *
sLib::properties(const sLib *thislib, const char *libname)
{
    for (const sLib *lib = thislib; lib; lib = lib->l_nextlib) {
        if (lib->match_name(libname)) {
            if (lib->l_type == LIBdevice)
                return (lib->l_prpty_strings);
        }
    }
    return (0);
}


// Static function.
// Return a list of the libraries currently open, according to type.
//
stringlist *
sLib::list(const sLib *thislib, int type)
{
    stringlist *s0 = 0, *se = 0;
    for (const sLib *lib = thislib; lib; lib = lib->l_nextlib) {
        if (lib->l_type & type) {
            if (!s0)
                se = s0 = new stringlist(
                    lstring::copy(lib->l_libfilename), 0);
            else {
                se->next = new stringlist(
                    lstring::copy(lib->l_libfilename), 0);
                se = se->next;
            }
        }
    }
    return (s0);
}


// A CHD is saved in a library that accesses an archive, to facilitate
// access of additional cells.  This returns the appropriate CHD, if
// it exists.
//
cCHD *
sLib::get_chd(const sLibRef *ref)
{
    if (!l_chdtab || !ref)
        return (0);
    char *path = pathlist::mk_path(ref->dir(), ref->file());
    cCHD *chd = (cCHD*)SymTab::get(l_chdtab, path);
    delete [] path;
    if (chd != (cCHD*)ST_NIL)
        return (chd);
    return (0);
}


// Save a CHD used to access ref.  This should be called once only for
// each archive file.
//
void
sLib::set_chd(const sLibRef *ref, cCHD *chd)
{
    if (ref && chd) {
        if (!l_chdtab)
            l_chdtab = new SymTab(false, false);
        l_chdtab->add(chd->filename(), chd, false);
    }
}


// Private function to create a new library reference, for an archive.
//
sLibRef *
sLib::new_libref(const char *ref, const char *pth, const char *name)
{
    sLibRef *r = l_lref_elts.new_element();
    r->clear();
    r->set_name(l_nametab.add(ref));

    if (pth) {
        const char *s = lstring::strrdirsep(pth);
        if (s) {
            char *t = lstring::copy(pth);
            t[s-pth] = 0;
            r->set_dir(l_nametab.add(t));
            delete [] t;
            r->set_file(l_nametab.add(s+1));
        }
        else {
            char *ss = getcwd(0, 0);
            r->set_dir(l_nametab.add(ss));
            delete [] ss;
            r->set_file(l_nametab.add(pth));
        }
        r->set_cellname(l_nametab.add(name));
    }
    l_symtab.add(r);
    return (r);
}


// Private function to create a new library reference, for inline
// native cells.
//
sLibRef *
sLib::new_libref(const char *ref, long ofs)
{
    sLibRef *r = l_lref_elts.new_element();
    r->clear();
    r->set_name(l_nametab.add(ref));
    r->set_offset(ofs);
    l_symtab.add(r);
    return (r);
}
// End of sLib functions


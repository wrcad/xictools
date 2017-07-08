
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
 $Id: sced_modlib.cc,v 5.12 2017/04/12 05:03:00 stevew Exp $
 *========================================================================*/

#include "config.h"
#include "main.h"
#include "fio.h"
#include "sced.h"
#include "sced_modlib.h"
#include "spnumber.h"
#include "pathlist.h"
#include "filestat.h"

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


// Search the library path and scan the files, adding entries to the
// symbol tables maintained for subroutines and models.  The model
// files are found in a subdirectory of the path directories with
// name in XM()->ModelSubdirName.
//
void
cModLib::Open(const char *name)
{
    const char *path = CDvdb()->getVariable(VA_LibPath);
    if (pathlist::is_empty_path(path)) {
        fprintf(stderr, "Warning: no path to libraries.\n");
        return;
    }
    ModSymTab = new SymTab(false, false);
    SubcSymTab = new SymTab(false, false);
    if (!name)
        name = XM()->ModelLibName();
    bool found_def = false;

    // First check the CWD for model.lib
    char *fname = pathlist::mk_path("./", XM()->ModelLibName());
    if (filestat::is_readable(fname)) {
        char *s = getcwd(0, 0);
        scan_file(s, name);
        free(s);
        found_def = true;
    }
    delete [] fname;

    pathgen pg(path);
    char *p;
    while ((p = pg.nextpath(false)) != 0) {
        // first look for the "model.lib" file, if not seen yet
        if (!found_def) {
            fname = pathlist::mk_path(p, XM()->ModelLibName());
            if (filestat::is_readable(fname)) {
                scan_file(p, name);
                found_def = true;
            }
            delete [] fname;
        }
        // now look in the "models" subdirectory
        char *ptmp = pathlist::mk_path(p, XM()->ModelSubdirName());
        delete [] p;
        p = ptmp;

        DIR *wdir;
        if (!(wdir = opendir(p))) {
            delete [] p;
            continue;
        }
        struct direct *de;
        while ((de = readdir(wdir)) != 0) {
            fname = pathlist::mk_path(p, de->d_name);
            if (filestat::is_readable(fname))
                scan_file(p, de->d_name);
            delete [] fname;
        }
        closedir(wdir);
        delete [] p;
    }
}


// Put model in referenced list if it is not there already.
//
void
cModLib::QueueModel(const char *name, const char *device, const char *params)
{
    if (name) {
        while (isspace(*name)) name++;
        if (!*name)
            return;
        if (device && (*device == 'm' || *device == 'M')) {
            // MOS device, check for L/W dependent models.
            libent_t *ent = mosFind(name, params);
            if (ent)
                name = ent->token;
        }
        for (mlib_t *m = Models; m; m = m->ml_next)
            if (lstring::cieq(name, m->ml_name))
                return;
        Models = new mlib_t(name, ContextLevel, Models);
    }
}


// Put subckt in referenced list if it is not there already.
//
void
cModLib::QueueSubckt(const char *name)
{
    if (name) {
        while (isspace(*name)) name++;
        if (!*name)
            return;
        for (mlib_t *m = Subckts; m; m = m->ml_next)
            if (lstring::cieq(name, m->ml_name))
                return;
        Subckts = new mlib_t(name, ContextLevel, Subckts);
    }
}


// Return the spice lines for the models referenced in the current
// context.  Free the entries from the referenced list.
//
sp_line_t *
cModLib::PrintModels()
{
    sp_line_t *d0 = 0, *d = 0;
    while (Models && Models->ml_level == ContextLevel) {
        sp_line_t *d1 = ModelText(Models->ml_name);
        if (d1 != 0) {
            if (d0 == 0)
                d = d0 = d1;
            else
                d->li_next = d1;
            for (; d->li_next; d = d->li_next) ;
        }
        mlib_t *m = Models;
        Models = Models->ml_next;
        delete m;
    }
    return (d0);
}


// Return the spice lines for the subckts referenced in the current
// context.  Free the entries from the referenced list.
//
sp_line_t *
cModLib::PrintSubckts()
{
    sp_line_t *d0 = 0, *d = 0;
    while (Subckts && Subckts->ml_level == ContextLevel) {
        sp_line_t *d1 = SubcktText(Subckts->ml_name);
        if (d1 != 0) {
            if (d0 == 0)
                d = d0 = d1;
            else
                d->li_next = d1;
            for (; d->li_next; d = d->li_next) ;
        }
        mlib_t *m = Subckts;
        Subckts = Subckts->ml_next;
        delete m;
    }
    return (d0);
}


// Return true if name is the name of a model in the database.
//
bool
cModLib::IsModel(const char *name)
{
    char *buf = lstring::copy(name);
    lstring::strtolower(buf);
    void *l = SymTab::get(ModSymTab, buf);
    delete [] buf;
    if (l == ST_NIL)
        return (false);
    return (true);
}


// Return the text for the named model.
//
sp_line_t *
cModLib::ModelText(const char *name)
{
    char *buf = lstring::copy(name);
    lstring::strtolower(buf);
    libent_t *l = (libent_t*)SymTab::get(ModSymTab, buf);
    delete [] buf;
    if (l == (libent_t*)ST_NIL)
        return (0);
    return (read_entry(l));
}


// Return the text for the named subcircuit.
//
sp_line_t *
cModLib::SubcktText(const char *name)
{
    char *buf = lstring::copy(name);
    lstring::strtolower(buf);
    libent_t *l = (libent_t*)SymTab::get(SubcSymTab, buf);
    delete [] buf;
    if (l == (libent_t*)ST_NIL)
        return (0);
    return (read_entry(l));
}


void
cModLib::Close()
{
    SymTabGen mgen(ModSymTab, true);
    SymTabEnt *h;
    while ((h = mgen.next()) != 0) {
        libent_t *l = (libent_t*)h->stData;
        delete l;
        delete h;
    }
    delete ModSymTab;
    ModSymTab = 0;

    SymTabGen sgen(SubcSymTab, true);
    while ((h = sgen.next()) != 0) {
        libent_t *l = (libent_t*)h->stData;
        delete l;
        delete h;
    }
    delete SubcSymTab;
    SubcSymTab = 0;

    delete WordTab;
    WordTab = 0;

    // Free the referenced lists
    while (Models) {
        mlib_t *m = Models;
        Models = Models->ml_next;
        delete m;
    }
    while (Subckts) {
        mlib_t *m = Subckts;
        Subckts = Subckts->ml_next;
        delete m;
    }
    ContextLevel = 0;
}


// Return the unique path string from the table.
//
char *
cModLib::word_tab(const char *word)
{
    if (!WordTab)
        WordTab = new SymTab(true, false);
    char *w = (char*)SymTab::get(WordTab, word);
    if (w == (char*)ST_NIL) {
        w = lstring::copy(word);
        WordTab->add(w, w, false);
    }
    return (w);
}


// For each .model or .subckt entry, record the offset and name in the
// appropriate symbol table.  Note that only top-level subcircuits and
// models outside of subcircuits are recorded.  Return false if error.
//
bool
cModLib::scan_file(const char *dir, const char *name)
{
    const char *msg =
        "Name clash in %s library for %s, first instance used.\n";

    char *p = pathlist::mk_path(dir, name);
    FILE *fp = fopen(p, "rb");
    delete [] p;
    if (!fp) {
        perror("fopen");
        return (false);
    }

    long offset = 0;
    int insub = 0;
    char *s, buf[512];
    while ((s = fgets(buf, 512, fp)) != 0) {
        NTstrip(buf);
        char wbuf[64];
        char *word = wbuf;
        while (isspace(*s))
            s++;
        while (*s && !isspace(*s) && s - buf < 7)
           *word++ = *s++;
        *word = '\0';
        word = wbuf;
        bool addent = false;
        if (*word == '.') {
            if ((*(word+1) == 's' || *(word+1) == 'S') &&
                    (*(word+2) == 'u' || *(word+2) == 'U') &&
                    (*(word+3) == 'b' || *(word+3) == 'B') &&
                    (*(word+4) == 'c' || *(word+4) == 'C') &&
                    (*(word+5) == 'k' || *(word+5) == 'K') &&
                    (*(word+6) == 't' || *(word+6) == 'T') &&
                    !*(word+7)) {
                insub++;
                if (insub == 1)
                    addent = true;
            }
            else if ((*(word+1) == 'e' || *(word+1) == 'E') &&
                    (*(word+2) == 'n' || *(word+2) == 'N') &&
                    (*(word+3) == 'd' || *(word+3) == 'D') &&
                    (*(word+4) == 's' || *(word+4) == 'S') &&
                    !*(word+5)) {
                insub--;
            }
            else if ((*(word+1) == 'm' || *(word+1) == 'M') &&
                    (*(word+2) == 'o' || *(word+2) == 'O') &&
                    (*(word+3) == 'd' || *(word+3) == 'D') &&
                    (*(word+4) == 'e' || *(word+4) == 'E') &&
                    (*(word+5) == 'l' || *(word+5) == 'L') &&
                    !*(word+6)) {
                if (!insub)
                    addent = true;
            }
        }
        if (addent) {
            while (isspace(*s))
                s++;
            word = wbuf;
            while (*s && !isspace(*s) && *s != ',' && *s != '(')
                *word++ = *s++;
            *word = '\0';
            libent_t *ent = new libent_t(word_tab(dir), word_tab(name),
                wbuf, offset);
            if (!insub) {
                if (!ModSymTab->add(ent->token, (void*)ent, true)) {
                    fprintf(stderr, msg, "model", ent->token);
                    delete ent;
                }
            }
            else {
                if (!SubcSymTab->add(ent->token, (void*)ent, true)) {
                    fprintf(stderr, msg, "subckt", ent->token);
                    delete ent;
                }
            }
        }
        offset = ftell(fp);
    }
    fclose(fp);
    return (true);
}


// Return a listing of the text associated with ent.
//
sp_line_t *
cModLib::read_entry(libent_t *ent)
{
    char buf[512];
    strcpy(buf, ent->dir);
    strcat(buf, "/");
    strcat(buf, ent->fname);
    FILE *fp = fopen(buf, "rb");
    if (fp == 0)
        return (0);
    fseek(fp, ent->offset, 0);
    int lcnt = 0;
    int insub = 0;
    bool inmod = false;
    char *s;
    sp_line_t *w = 0, *w0 = 0;
    while ((s = fgets(buf, 512, fp)) != 0) {
        NTstrip(buf);
        char wbuf[16];
        char *word = wbuf;
        while (isspace(*s))
            s++;
        while (*s && !isspace(*s) && s - buf < 7)
           *word++ = *s++;
        *word = '\0';
        word = wbuf;
        if (*word == '.') {
            if ((*(word+1) == 's' || *(word+1) == 'S') &&
                    (*(word+2) == 'u' || *(word+2) == 'U') &&
                    (*(word+3) == 'b' || *(word+3) == 'B') &&
                    (*(word+4) == 'c' || *(word+4) == 'C') &&
                    (*(word+5) == 'k' || *(word+5) == 'K') &&
                    (*(word+6) == 't' || *(word+6) == 'T') &&
                    !*(word+7)) {
                insub++;
            }
            else if ((*(word+1) == 'e' || *(word+1) == 'E') &&
                    (*(word+2) == 'n' || *(word+2) == 'N') &&
                    (*(word+3) == 'd' || *(word+3) == 'D') &&
                    (*(word+4) == 's' || *(word+4) == 'S') &&
                    !*(word+5)) {
                insub--;
            }
            else if ((*(word+1) == 'm' || *(word+1) == 'M') &&
                    (*(word+2) == 'o' || *(word+2) == 'O') &&
                    (*(word+3) == 'd' || *(word+3) == 'D') &&
                    (*(word+4) == 'e' || *(word+4) == 'E') &&
                    (*(word+5) == 'l' || *(word+5) == 'L') &&
                    !*(word+6)) {
                if (!insub) {
                    if (lcnt)
                        break;
                    inmod = true;
                }
            }
        }
        else if (*word == '#')
            continue;

        if (!lcnt && !inmod && !insub)
            // oops, pointer is wrong
            break;
        if (!w0)
            w = w0 = new sp_line_t;
        else {
            w->li_next = new sp_line_t;
            w = w->li_next;
        }
        s = buf + strlen(buf) - 1;
        while (isspace(*s) && s >= buf)
            *s-- = '\0';
        w->li_line = lstring::copy(buf);
        if (!insub && !inmod)
            break;
        lcnt++;
    }
    fclose(fp);
    return (w0);
}


namespace {
    void getLW(const char*, double*, double*);
    char *getParam(const char*, sp_line_t*);
}

// Find a model with matching base name with l,w matching the selection
// parameters in the model
//
libent_t *
cModLib::mosFind(const char *name, const char *params)
{
    double l, w;
    getLW(params, &l, &w);

    int lastcnds = -1;
    libent_t *lastent = 0;

    SymTabGen gen(ModSymTab);
    SymTabEnt *h;
    while ((h = gen.next()) != 0) {
        libent_t *ent = (libent_t*)h->stData;
        const char *n = h->stTag;

        if (lstring::cieq(name, n) ||
                (lstring::ciprefix(name, n) && *(n + strlen(name)) == '.')) {

            // Count the conditions matched.  The entry with the highest
            // condition count is returned.
            int cnds = 0;

            sp_line_t *mlines = 0;
            if (l > 0) {
                mlines = read_entry(ent);

                char *tok1 = getParam("LMIN", mlines);
                char *tok2 = getParam("LMAX", mlines);
                if (tok1) {
                    const char *t = tok1;
                    double *d = SPnum.parse(&t, false);
                    delete [] tok1;
                    if (d) {
                        if (l < *d)
                            continue;
                        cnds++;
                    }
                    else {
                        fprintf(stderr,
                            "Non-numeric value for \"LMIN\" in model %s", n);
                        continue;
                    }
                }
                if (tok2) {
                    const char *t = tok2;
                    double *d = SPnum.parse(&t, false);
                    delete [] tok2;
                    if (d) {
                        if (l > *d)
                            continue;
                        cnds++;
                    }
                    else {
                        fprintf(stderr,
                            "Non-numeric value for \"LMAX\" in model %s", n);
                        continue;
                    }
                }
            }
            if (w > 0) {
                if (!mlines)
                    mlines = read_entry(ent);
                char *tok1 = getParam("WMIN", mlines);
                char *tok2 = getParam("WMAX", mlines);
                if (tok1) {
                    const char *t = tok1;
                    double *d = SPnum.parse(&t, false);
                    delete [] tok1;
                    if (d) {
                        if (w < *d)
                            continue;
                        cnds++;
                    }
                    else {
                        fprintf(stderr,
                            "Non-numeric value for \"WMIN\" in model %s", n);
                        continue;
                    }
                }
                if (tok2) {
                    const char *t = tok2;
                    double *d = SPnum.parse(&t, false);
                    delete [] tok2;
                    if (d) {
                        if (w > *d)
                            continue;
                        cnds++;
                    }
                    else {
                        fprintf(stderr,
                            "Non-numeric value for \"WMAX\" in model %s", n);
                        continue;
                    }
                }
            }
            if (cnds > lastcnds) {
                lastent = ent;
                lastcnds = cnds;
            }
        }
    }
    return (lastent);
}
// End of cModLib functions.


namespace {
    void
    getLW(const char *params, double *l, double *w)
    {
        *l = 0.0;
        *w = 0.0;
        char *tok;
        const char *p = params;
        while ((tok = cSced::sp_gettok(&p)) != 0) {
            if (lstring::cieq(tok, "L")) {
                delete [] tok;
                tok = cSced::sp_gettok(&p);
                if (tok) {
                    const char *t = tok;
                    double *d = SPnum.parse(&t, false);
                    delete [] tok;
                    if (d)
                        *l = *d;
                }
                continue;
            }
            if (lstring::cieq(tok, "W")) {
                delete [] tok;
                tok = cSced::sp_gettok(&p);
                if (tok) {
                    const char *t = tok;
                    double *d = SPnum.parse(&t, false);
                    delete [] tok;
                    if (d)
                        *w = *d;
                }
                continue;
            }
            delete [] tok;
        }
    }


    char *
    getParam(const char *token, sp_line_t *lines)
    {
        for (sp_line_t *l = lines; l; l = l->li_next) {
            const char *s = l->li_line;
            char *tok;
            while ((tok = cSced::sp_gettok(&s)) != 0) {
                if (lstring::cieq(tok, token)) {
                    delete [] tok;
                    return (cSced::sp_gettok(&s));
                }
                delete [] tok;
            }
        }
        return (0);
    }
}



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
 $Id: fio_alias.cc,v 1.34 2015/06/11 05:54:06 stevew Exp $
 *========================================================================*/

#include "fio.h"
#include "fio_alias.h"
#include "fio_library.h"
#include "cd_celldb.h"
#include <ctype.h>


// The FIOaliasTab controls cell name changing while reading and
// writing archives.  If aliasing is to be applied, an FIOaliasTab can
// be bound into cv_in/cv_out derivatives.  The aliasing modes are set
// at the time of FIOaliasTab creation.
//
// The table below summarizes the modes that apply under various
// operations.
//
//             Store  Front        Back
// Translation L R    CC,PS,AF*,GD -            Format translation
// Edit File   X R    CC,AF,AR     -            Read to memory, Edit cmd
// Read File   X R    CC,PS,AF,AR  -            Read to memory, panel
// Read Cx     X R    CC,PS,AF,AR  -            Read to context
// Read Lib    X R    LB,AR        -            Read from library reference
// Write Mem   L W    -            CC,PS,AF,GD  Write archive from memory
// Write Cx    L W    -            CC,PS,AF,GD  Write from context
// Assemble    L R    -            CC,PS,GD     Assemble multiple archives
// Split       -      -            -            Split and flaten archive
//
// Notes:
//  There is never both front end and back end tables used.
//
// Store
// L:  aliases use local storage in al_buf
// X:  aliases use system storage (cCD::CellNameTable)
// R:  open for reading
// W:  open for writing
//
// Alias modes
// CC:  Case conversion
// PS:  Prefix/Suffix change
// AF:  Use alias file (*reading only for translation, assemble)
// AR:  Auto Rename
// LB:  Use library aliases
// GD:  GDS cell name checks, name length and characters


// Create a new FIOaliasTab used for reading into memory or a context
// struct, and for the assemble function.
//
FIOaliasTab *
cFIO::NewReadingAlias(unsigned int mask)
{
    // When reading into memory, always use an alias table to handle
    // library cell clashes.
    FIOaliasTab *tab = new FIOaliasTab(false, false);

    bool ar = false;
    if (mask & CVAL_AUTO_NAME) {
        if (IsAutoRename()) {
            tab->set_auto_rename(true);
            ar = true;
        }
    }
    if (mask & CVAL_TO_LOWER) {
        if (IsInToLower())
            tab->set_to_lower(true);
    }
    if (mask & CVAL_TO_UPPER) {
        if (IsInToUpper())
            tab->set_to_upper(true);
    }
    if (mask & CVAL_PREFIX) {
        const char *pre = InCellNamePrefix();
        if (pre && *pre)
            tab->set_prefix(pre);
    }
    if (mask & CVAL_SUFFIX) {
        const char *suf = InCellNameSuffix();
        if (suf && *suf)
            tab->set_suffix(suf);
    }
    if (mask & CVAL_READ_FILE) {
        UAtype t = InUseAlias();
        tab->set_rd_file(t == UAupdate || t == UAread);
    }
    if ((mask & CVAL_WRITE_FILE) && !ar) {
        // No update when translating or auto-rename.
        UAtype t = InUseAlias();
        tab->set_wr_file(t == UAupdate || t == UAwrite);
    }
    if (mask & CVAL_GDS_CHECK)
        tab->set_gds_check(true);
    if (mask & CVAL_LIMIT32)
        tab->set_limit32(GdsOutLevel());
    return (tab);
}


FIOaliasTab *
cFIO::NewTranslatingAlias(unsigned int mask)
{
    FIOaliasTab *tab = 0;
    if (mask & CVAL_TO_LOWER) {
        if (IsInToLower()) {
            if (!tab)
                tab = new FIOaliasTab(true, false);
            tab->set_to_lower(true);
        }
    }
    if (mask & CVAL_TO_UPPER) {
        if (IsInToUpper()) {
            if (!tab)
                tab = new FIOaliasTab(true, false);
            tab->set_to_upper(true);
        }
    }
    if (mask & CVAL_PREFIX) {
        const char *pre = InCellNamePrefix();
        if (pre && *pre) {
            if (!tab)
                tab = new FIOaliasTab(true, false);
            tab->set_prefix(pre);
        }
    }
    if (mask & CVAL_SUFFIX) {
        const char *suf = InCellNameSuffix();
        if (suf && *suf) {
            if (!tab)
                tab = new FIOaliasTab(true, false);
            tab->set_suffix(suf);
        }
    }
    if (mask & CVAL_READ_FILE) {
        UAtype t = InUseAlias();
        if (t == UAupdate || t == UAread) {
            if (!tab)
                tab = new FIOaliasTab(true, false);
            tab->set_rd_file(true);
        }
    }
    if (mask & CVAL_GDS_CHECK) {
        if (!tab)
            tab = new FIOaliasTab(true, false);
        tab->set_gds_check(true);
    }
    if (mask & CVAL_LIMIT32) {
        if (!tab)
            tab = new FIOaliasTab(true, false);
        tab->set_limit32(GdsOutLevel());
    }
    return (tab);
}


// Create a new FIOaliasTab for writing, used when writing from memory
// or a constxt struct.  Unless the flags indicate that aliasing will
// be done, a tab is not created, unless forcenew if true.
//
FIOaliasTab *
cFIO::NewWritingAlias(unsigned int mask, bool forcenew)
{
    FIOaliasTab *tab = 0;
    if (mask & CVAL_TO_LOWER) {
        if (IsOutToLower()) {
            if (!tab)
                tab = new FIOaliasTab(true, true);
            tab->set_to_lower(true);
        }
    }
    if (mask & CVAL_TO_UPPER) {
        if (IsOutToUpper()) {
            if (!tab)
                tab = new FIOaliasTab(true, true);
            tab->set_to_upper(true);
        }
    }
    if (mask & CVAL_PREFIX) {
        const char *pre = OutCellNamePrefix();
        if (pre && *pre) {
            if (!tab)
                tab = new FIOaliasTab(true, true);
            tab->set_prefix(pre);
        }
    }
    if (mask & CVAL_SUFFIX) {
        const char *suf = OutCellNameSuffix();
        if (suf && *suf) {
            if (!tab)
                tab = new FIOaliasTab(true, true);
            tab->set_suffix(suf);
        }
    }
    if (mask & CVAL_READ_FILE) {
        UAtype t  = OutUseAlias();
        if (t == UAupdate || t == UAread) {
            if (!tab)
                tab = new FIOaliasTab(true, true);
            tab->set_rd_file(true);
        }
    }
    if (mask & CVAL_WRITE_FILE) {
        UAtype t = OutUseAlias();
        if (t == UAupdate || t == UAwrite) {
            if (!tab)
                tab = new FIOaliasTab(true, true);
            tab->set_wr_file(true);
        }
    }
    if (mask & CVAL_GDS_CHECK) {
        if (!tab)
            tab = new FIOaliasTab(true, true);
        tab->set_gds_check(true);
    }
    if (mask & CVAL_LIMIT32) {
        if (!tab)
            tab = new FIOaliasTab(true, true);
        tab->set_limit32(GdsOutLevel());
    }
    if (!tab && forcenew)
        tab = new FIOaliasTab(true, true);
    return (tab);
}


// Return true if a conversion from filetype ft using prms would
// change anything physical.
//
bool
cFIO::FileWillChange(FileType ft, const FIOcvtPrms *prms)
{
    if (!prms)
        return (false);
    if (ft != prms->filetype())
        return (true);
    if (prms->scale() != 1.0)
        return (true);
    if (prms->use_window() || prms->flatten() ||
            (prms->ecf_level() != ECFnone))
        return (true);
    if (prms->allow_layer_mapping()) {
        if (UseLayerList()) {
            const char *ll = LayerList();
            if (ll && *ll)
                return (true);
        }
        if (IsUseLayerAlias()) {
            const char *la = LayerAlias();
            if (la && *la)
                return (true);
        }
    }
    if (prms->alias_mask() & CVAL_PREFIX) {
        const char *pre = InCellNamePrefix();
        if (pre && *pre)
            return (true);
    }
    if (prms->alias_mask() & CVAL_SUFFIX) {
        const char *suf = InCellNameSuffix();
        if (suf && *suf)
            return (true);
    }
    if (prms->alias_mask() & CVAL_TO_LOWER) {
        if (IsInToLower())
            return (true);
    }
    if (prms->alias_mask() & CVAL_TO_UPPER) {
        if (IsInToUpper())
            return (true);
    }
    if (prms->alias_mask() & CVAL_READ_FILE) {
        UAtype t = InUseAlias();
        if (t == UAupdate || t == UAread)
            return (true);
    }
    return (false);
}
// End of cFIO functions.


// alias_tab  name, alias  (names not in seen only)
// aliases    alias        (aliases also in alias_tab)
// names      name         (names also in alias_tab)
// seen       name         (names not in alias_tab only)
//
// The "name" strings are always allocated locally, in the seen or
// names tables.  The "alias" strings are allocated either locally
// or from the system string table.
//
FIOaliasTab::FIOaliasTab(bool local, bool out, const cv_alias_info *aif)
{
    at_alias_tab = new ATalTab;
    at_aliases = new ATstrTab(local);
    at_names = new strtab_t;
    at_seen = new strtab_t;
    at_accum = 0;
    at_dirty = false;
    at_frozen = false;
    at_use_local = local;
    set_output(out);

    if (aif) {
        ai_pfx = lstring::copy(aif->prefix());
        ai_sfx = lstring::copy(aif->suffix());
        ai_flags = aif->flags();
    }

    CD()->RegisterCreate("FIOaliasTab");
}


FIOaliasTab::~FIOaliasTab()
{
    delete at_alias_tab;
    delete at_aliases;
    delete at_names;
    delete at_seen;
    delete at_accum;

    CD()->RegisterDestroy("FIOaliasTab");
}


// This is called after reading each archive when reusing the same
// aliasing for multiple archives.  If rem_name in not null, it will
// be removed from the accumulated seen list.  This prevents aliasing
// of a name that has already been referenced, but is to be defined in
// the file about to be read.
//
void
FIOaliasTab::reinit(const char *rem_name)
{
    // Clear alias_tab, the aliases table is left as-is.
    at_alias_tab->clear();
    delete at_names;
    at_names = new strtab_t;

    // Put the seen elements into accum, clear seen
    if (!at_accum)
        at_accum = new strtab_t;
    at_accum->merge(at_seen);
    delete at_seen;
    at_seen = new strtab_t;
    if (rem_name)
        at_accum->remove(rem_name);
}


// This can be called when all cells have been read.  It frees tables
// no longer needed and prevents updating.
//
void
FIOaliasTab::set_frozen()
{
    at_aliases->clear();  // keeps alias string table
    delete at_seen;
    at_seen = 0;
    delete at_accum;
    at_accum = 0;
    at_frozen = true;
}


// Perform an alias substitution, if necessary.
//
const char *
FIOaliasTab::alias(const char *name)
{
    const char *als = at_alias_tab->find_alias(name);
    if (als)
        return (als);

    if (at_frozen)
        return (name);

    als = at_seen->find(name);
    if (als)
        return (als);

    // Never change library cell names
    if (FIO()->LookupLibCell(0, name, LIBdevice, 0))
        return (at_seen->add(name));

    bool force_change = false;
    if (at_aliases->find(name))
        // already using this name as an alias
        force_change = true;
    else if (at_accum && at_accum->find(name))
        // saw this name before in previous file
        force_change = true;
    else if (auto_rename()) {
        if (CDcdb()->findSymbol(name))
            force_change = true;
    }
    return (new_name(name, force_change));
}


// Obtain a new cell name.  If force_change, always change the name.
//
const char *
FIOaliasTab::new_name(const char *name, bool force_change)
{
    char scratch[256];
    int sf_offs = 0;
    for (int i = 0; ; i++) {
        if (alt_name(scratch, name, &sf_offs, i)) {
            if (!at_seen->find(scratch) && !at_aliases->find(scratch)) {
                if (at_accum && at_accum->find(scratch))
                    continue;
                CDs *sd = CDcdb()->findCell(scratch, Physical);
                if (sd && (auto_rename() || sd->isDevice()))
                    continue;
                sd = CDcdb()->findCell(scratch, Electrical);
                if (sd && (auto_rename() || sd->isDevice()))
                    continue;
                const char *als = at_aliases->add(scratch);
                const char *cname = at_names->add(name);
                at_alias_tab->add(cname, als);
                at_dirty = true;
                sprintf(scratch, "Using alias %s for name %s.", als, cname);
                FIO()->ifPrintCvLog(IFLOG_INFO, scratch);
                return (als);
            }
        }
        else if (!force_change)
            break;
    }
    return (at_seen->add(name));
}


// Set up an alias for name, if it has not already been aliased.
//
void
FIOaliasTab::set_alias(const char *name, const char *als)
{
    if (!at_alias_tab->find_alias(name)) {
        const char *cname = at_names->add(name);
        const char *calias = at_aliases->add(als);
        at_alias_tab->add(cname, calias);
    }
}


// Read in a set of aliases from the named file.  Each line of the file
// contains one alias of the form <GDS_name> <internal_name>.
//
void
FIOaliasTab::read_alias(const char *filename)
{
    if (at_frozen || !rd_file())
        return;

    // First look for the name without "gz", and if not found try it
    // with "gz".  The "gz" version is no longer generated, but we
    // look for it for backward compatibility.
    //
    char buf[256];
    strcpy(buf, filename);
    char *t = buf + strlen(buf) - 3;
    bool was_gz = false;
    if (!strcasecmp(t, ".gz")) {
        *t = 0;
        was_gz = true;
    }
    strcat(buf, AliasExt);
    FILE *fp = fopen(buf, "r");
    if (!fp && was_gz) {
        strcpy(buf, filename);
        strcat(buf, AliasExt);
        fp = fopen(buf, "r");
        if (fp) {
            // Old bug, name includes 'gz', make sure to write a new file.
            at_dirty = true;
        }
    }
    if (!fp)
        return;  // not an error

    while (fgets(buf, 256, fp) != 0) {
        char name[128], gdsname[128];
        // <gds_name> <internal_name>
        if (sscanf(buf, "%s %s", gdsname, name) != 2)
            continue;
        if (output())
            set_alias(name, gdsname);
        else
            set_alias(gdsname, name);
    }
    fclose(fp);
}


// Read in the aliases from the library pointers.  This is done when
// reading from an archive referenced through a library.  The normal
// aliasing is suppressed in this case, but the aliasing implied
// within the library (established here) is applied.
//
void
FIOaliasTab::add_lib_alias(sLibRef *ref, sLib *lib)
{
    if (at_frozen)
        return;
    if (!ref || !ref->dir() || !lib)
        return;

    tgen_t<sLibRef> gen(lib->symtab().table());
    sLibRef *r;
    while ((r = gen.next()) != 0) {
        if (r->dir() == ref->dir() && r->file() == ref->file()) {
            if (!r->cellname() || !strcmp(r->name(), r->cellname()))
                continue;
            set_alias(r->cellname(), r->name());
        }
    }
}


// Dump the current set of aliases into the named file.
//
void
FIOaliasTab::dump_alias(const char *filename)
{
    if (!wr_file() || !at_alias_tab || !at_dirty)
        return;
    if (!filename || !*filename)
        return;
    char buf[256];
    strcpy(buf, filename);

    // Strip ".gz"
    char *t = buf + strlen(buf) - 3;
    if (!strcasecmp(t, ".gz"))
        *t = 0;

    strcat(buf, AliasExt);
    FILE *fp = fopen(buf, "w");
    if (!fp) {
        FIO()->ifPrintCvLog(IFLOG_WARN, "could not write alias file for %s.",
            lstring::strip_path(filename));
        return;
    }

    tgen_t<bd_t> gen(at_alias_tab->bd_tab);
    bd_t *e;
    while ((e = gen.next()) != 0) {
        if (output())
            fprintf(fp, "%-33s %s\n", e->alias, e->name);
        else
            fprintf(fp, "%-33s %s\n", e->name, e->alias);
    }

    fclose(fp);
    at_dirty = false;
}


void
FIOaliasTab::set_substitutions(const char *pref, const char *sufx)
{
    if (at_frozen)
        return;
    set_prefix(pref);
    set_suffix(sufx);
}


// Private function to alter the given name in accord with the
// directive flags.
//
bool
FIOaliasTab::alt_name(char *cbuf, const char *name, int *sf_offs, int ix)
{
    if (ix == 0) {
        strcpy(cbuf, name);
        *sf_offs = strlen(cbuf);
        bool chg = false;

        if (to_lower()) {
            // Map upper-case names to lower case, mixed-case names
            // are not changed.
            bool found_lower = false;
            bool found_upper = false;
            for (char *s = cbuf; *s; s++) {
                if (isupper(*s))
                    found_upper = true;
                else if (islower(*s)) {
                    found_lower = true;
                    break;
                }
            }
            if (!found_lower && found_upper) {
                for (char *s = cbuf; *s; s++) {
                    if (isupper(*s))
                        *s = tolower(*s);
                }
                chg = true;
            }
        }
        if (to_upper()) {
            // Map lower-case names to upper case, mixed-case names
            // are not changed.
            bool found_upper = false;
            bool found_lower = false;
            for (char *t = cbuf; *t; t++) {
                if (islower(*t))
                    found_lower = true;
                else if (isupper(*t)) {
                    found_upper = true;
                    break;
                }
            }
            if (!found_upper && found_lower) {
                for (char *t = cbuf; *t; t++) {
                    if (islower(*t))
                        *t = toupper(*t);
                }
                chg = true;
            }
        }
        if (prefix() || suffix()) {
            char *tname = cCD::AlterName(cbuf, prefix(), suffix());
            if (strcmp(tname, cbuf)) {
                strcpy(cbuf, tname);
                *sf_offs = strlen(cbuf);
                chg = true;
            }
            delete [] tname;
        }
        if (limit32()) {
            // Truncate name if necessary (GDSII level 3).
            if (*sf_offs > 32) {
                cbuf[32] = 0;
                *sf_offs = 32;
                chg = true;
            }
        }
        if (gds_check()) {
            // Replace any unacceptable chars.  Legal chars in GDSII
            // structure names are A-Z, a-z, 0-9, '_', '?', '$'.
            for (char *t = cbuf; *t; t++) {
                if (isalpha(*t) || isdigit(*t) || *t == '_' || *t == '?' ||
                        *t == '$')
                    continue;
                if (*t == '.')
                    *t = '_';
                else
                    *t = '$';
                chg = true;
            }
        }
        if (!FIO()->IsNoStrictCellnames()) {
            // Unless cell name testing is disabled, never allow white
            // space in cell names.
            for (char *t = cbuf; *t; t++) {
                if (*t > ' ')
                    continue;
                *t = '_';
                chg = true;
            }
        }
        return (chg);
    }
    if (limit32()) {
        char tbuf[32];
        tbuf[0] = '$';
        mmItoA(tbuf + 1, ix);
        if (*sf_offs + strlen(tbuf) <= 32)
            strcpy(cbuf + *sf_offs, tbuf);
        else
            strcpy(cbuf + 32 - strlen(tbuf), tbuf);
    }
    else {
        char *s = cbuf + *sf_offs;
        *s++ = '$';
        mmItoA(s, ix);
    }
    return (true);
}
// End of FIOaliasTab functions


const char *
bdtable_t::find_alias(const char *name)
{
    unsigned int i = string_hash(name, hashmask);
    for (bd_t *e = tab[i]; e; e = e->n_next) {
        if (str_compare(name, e->name))
            return (e->alias);
    }
    return (0);
}


const char *
bdtable_t::find_name(const char *alias)
{
    unsigned int i = string_hash(alias, hashmask) + hashmask + 1;
    for (bd_t *e = tab[i]; e; e = e->n_next) {
        if (str_compare(alias, e->alias))
            return (e->name);
    }
    return (0);
}


// The name and alias are known to not already be in the tables.
// These are pointers obtained from an external string table.
//
void
bdtable_t::add(const char *name, const char *alias, eltab_t<bd_t> *fct)
{
    bd_t *e = fct->new_element();
    e->name = name;
    e->alias = alias;
    unsigned int i = string_hash(name, hashmask);
    e->n_next = tab[i];
    tab[i] = e;
    i = string_hash(alias, hashmask) + hashmask + 1;
    e->a_next = tab[i];
    tab[i] = e;
    count++;
}


bool
bdtable_t::remove(const char *name)
{
    unsigned int i = string_hash(name, hashmask);
    bd_t *e, *ep = 0;
    for (e = tab[i]; e; e = e->n_next) {
        if (str_compare(name, e->name)) {
            if (ep)
                ep->n_next = e->n_next;
            else
                tab[i] = e->n_next;
            break;
        }
        ep = e;
    }
    if (!e)
        return (false);
    i = string_hash(e->alias, hashmask) + hashmask + 1;
    for (bd_t *ee = tab[i]; ee; ee = ee->a_next) {
        if (ee == e) {
            if (ep)
                ep->n_next = ee->a_next;
            else
                tab[i] = ee->a_next;
            break;
        }
        ep = ee;
    }
    return (true);
}


bdtable_t *
bdtable_t::check_rehash()
{
    if (count/(hashmask+1) <= ST_MAX_DENS)
        return (this);

    unsigned int newmask = (hashmask << 1) | 1;
    bdtable_t *st =
        (bdtable_t*)new char[sizeof(bdtable_t) + 2*newmask*sizeof(bd_t*)];
    st->count = count;
    st->hashmask = newmask;
    for (unsigned int i = 0; i <= 2*newmask; i++)
        st->tab[i] = 0;
    for (unsigned int i = 0;  i <= hashmask; i++) {
        bd_t *en;
        for (bd_t *e = tab[i]; e; e = en) {
            en = e->n_next;
            unsigned int j = string_hash(e->name, newmask);
            e->n_next = st->tab[j];
            st->tab[j] = e;
        }
        tab[i] = 0;
    }
    for (unsigned int i = hashmask + 1;  i <= 2*hashmask; i++) {
        bd_t *en;
        for (bd_t *e = tab[i]; e; e = en) {
            en = e->n_next;
            unsigned int j = string_hash(e->name, newmask) + newmask + 1;
            e->a_next = st->tab[j];
            st->tab[j] = e;
        }
        tab[i] = 0;
    }

    delete this;
    return (st);
}


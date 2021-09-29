
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
 * Xic Integrated Circuit Layout and Schematic Editor                     *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#ifndef FIO_ALIAS_H
#define FIO_ALIAS_H


// Extension used for alias files
#define AliasExt ".alias"

struct bd_t
{
    const char *name;
    const char *alias;
    bd_t *n_next;
    bd_t *a_next;
};


struct bdtable_t
{
    bdtable_t()
        {
            count = 0;
            hashmask = 0;
            tab[0] = 0;
            tab[1] = 0;
        }

    const char *find_alias(const char *name);
    const char *find_name(const char *alias);
    void add(const char *name, const char *alias, eltab_t<bd_t>*);
    void add(bd_t*);
    bool remove(const char *name);
    bdtable_t *check_rehash();

    unsigned int count;     // Number of elements in table.
    unsigned int hashmask;  // Hash table size - 1, size is power 2.
    bd_t *tab[2];           // Start of the hash table (extended as necessary).
};

// The bi-directional table used to hold name/alias pairs.
//
struct ATalTab
{
    ATalTab() { bd_tab = new bdtable_t; }
    ~ATalTab() { delete bd_tab; }

    void clear()
        {
            delete bd_tab;
            bd_tab = new bdtable_t;
            bd_elts.clear();
        }

    const char *find_alias(const char *name)
        { return (bd_tab->find_alias(name)); }
    const char *find_name(const char *alias)
        { return (bd_tab->find_name(alias)); }
    void add(const char *name, const char *alias)
        {
            bd_tab->add(name, alias, &bd_elts);
            bd_tab = bd_tab->check_rehash();
        }
    bool remove(const char *name)
        {
            bool ok = bd_tab->remove(name);
            bd_tab = bd_tab->check_rehash();
            return (ok);
        }

    bdtable_t *bd_tab;
    eltab_t<bd_t> bd_elts;
};

// A string table that allows allocation either locally or from the
// system string table.
//
struct ATstrTab
{
    ATstrTab(bool local) { st_tab = 0; st_local = local; }
    ~ATstrTab() { delete st_tab; }

    // Keep the string table, elements may still be in use.
    //
    void clear()
        {
            delete st_tab;
            st_tab = 0;
            st_elts.clear();
        }

    // Return the matching entry for string, if it exists, null
    // otherwise.
    //
    const char *find(const char *string)
        {
            if (!string || !st_tab)
                return (0);
            sl_t *e = st_tab->find(string);
            return (e ? e->tab_name() : 0);
        }


    // Add string to string table if not already there, and return a
    // pointer to the table entry.
    //
    const char *add(const char *string)
        {
            if (!string)
                return (0);
            if (!st_tab)
                st_tab = new table_t<sl_t>;
            sl_t *e = st_tab->find(string);
            if (!e) {
                e = st_elts.new_element();
                e->set_tab_next(0);
                if (st_local)
                    e->set_tab_name(st_buf.new_string(string));
                else
                    e->set_tab_name(Tstring(CD()->CellNameTableAdd(string)));
                st_tab->link(e, false);
                st_tab = st_tab->check_rehash();
            }
            return (e->tab_name());
        }

private:
    table_t<sl_t> *st_tab;      // offset hash table
    eltab_t<sl_t> st_elts;      // table element factory
    stbuf_t st_buf;             // local storage for strings
    bool st_local;              // local string storage when set
};

// Flags for alias mode mask.
//
#define CVAL_PREFIX     0x1
#define CVAL_SUFFIX     0x2
#define CVAL_OUTPUT     0x4
#define CVAL_AUTO_NAME  0x8
#define CVAL_TO_LOWER   0x10
#define CVAL_TO_UPPER   0x20
#define CVAL_READ_FILE  0x40
#define CVAL_WRITE_FILE 0x80
#define CVAL_GDS_CHECK  0x100
#define CVAL_LIMIT32    0x200

// Convenience defines for the enabling mask.
//
#define CVAL_CASE  CVAL_TO_UPPER | CVAL_TO_LOWER
#define CVAL_PFSF  CVAL_PREFIX | CVAL_SUFFIX
#define CVAL_FILE  CVAL_READ_FILE | CVAL_WRITE_FILE
#define CVAL_GDS   CVAL_GDS_CHECK | CVAL_LIMIT32


// Keep track of aliasing setup parameters.
//
struct cv_alias_info
{
    cv_alias_info()
        {
            ai_pfx = ai_sfx = 0;
            ai_flags = 0;
        }

    ~cv_alias_info()
        {
            delete [] ai_pfx;
            delete [] ai_sfx;
        }

    // set substitution prefix
    void set_prefix(const char *str)
        {
            if (str && *str) {
                ai_flags |= CVAL_PREFIX;
                char *s = lstring::copy(str);
                delete [] ai_pfx;
                ai_pfx = s;
            }
            else {
                ai_flags &= ~CVAL_PREFIX;
                delete [] ai_pfx;
                ai_pfx = 0;
            }
        }

    // set substitution suffix
    void set_suffix(const char *str)
        {
            if (str && *str) {
                ai_flags |= CVAL_SUFFIX;
                char *s = lstring::copy(str);
                delete [] ai_sfx;
                ai_sfx = s;
            }
            else {
                ai_flags &= ~CVAL_SUFFIX;
                delete [] ai_sfx;
                ai_sfx = 0;
            }
        }

    // mark writing output
    void set_output(bool b)         { setflg(b, CVAL_OUTPUT); }

    // mark auto-rename mode
    void set_auto_rename(bool b)    { setflg(b, CVAL_AUTO_NAME); }

    // mark case change to lower
    void set_to_lower(bool b)       { setflg(b, CVAL_TO_LOWER); }

    // mark case change to upper
    void set_to_upper(bool b)       { setflg(b, CVAL_TO_UPPER); }

    // mark reading of existing alias file when reading
    void set_rd_file(bool b)        { setflg(b, CVAL_READ_FILE); }

    // mark update of alias file when writing
    void set_wr_file(bool b)        { setflg(b, CVAL_WRITE_FILE); }

    // mark enforcement of gds cell name char list
    void set_gds_check(bool b)      { setflg(b, CVAL_GDS_CHECK); }

    // mark enforcement of 32-char cellname limit
    void set_limit32(bool b)        { setflg(b, CVAL_LIMIT32); }

    void setup(const cv_alias_info *a)
        {
            if (!a)
                return;
            ai_pfx = lstring::copy(a->ai_pfx);
            ai_sfx = lstring::copy(a->ai_sfx);
            ai_flags = a->ai_flags;
        }

    void setup(unsigned int f, const char *pfx, const char *sfx)
        {
            ai_pfx = lstring::copy(pfx);
            ai_sfx = lstring::copy(sfx);
            ai_flags = f;
        }


    void set_flags(unsigned int f)  { ai_flags = f; }

    const char *prefix() const  { return (ai_pfx); }
    const char *suffix() const  { return (ai_sfx); }
    bool output()        const  { return (ai_flags & CVAL_OUTPUT); }
    bool auto_rename()   const  { return (ai_flags & CVAL_AUTO_NAME); }
    bool to_lower()      const  { return (ai_flags & CVAL_TO_LOWER); }
    bool to_upper()      const  { return (ai_flags & CVAL_TO_UPPER); }
    bool rd_file()       const  { return (ai_flags & CVAL_READ_FILE); }
    bool wr_file()       const  { return (ai_flags & CVAL_WRITE_FILE); }
    bool gds_check()     const  { return (ai_flags & CVAL_GDS_CHECK); }
    bool limit32()       const  { return (ai_flags & CVAL_LIMIT32); }

    unsigned int flags() const  { return (ai_flags); }

    // If equal, name tables will be the same.
    bool operator==(const cv_alias_info &a) const
        {
            if (!str_compare(ai_pfx, a.ai_pfx))
                return (false);
            if (!str_compare(ai_sfx, a.ai_sfx))
                return (false);
            if (auto_rename() || a.auto_rename())
                return (false);
            if (rd_file() || a.rd_file())
                return (false);
            return (
                output() == a.output() &&
                to_lower() == a.to_lower() &&
                to_upper() == a.to_upper() &&
                gds_check() == a.gds_check() &&
                limit32() == a.limit32());
        }

protected:
    void setflg(bool b, unsigned int flag)
        {
            if (b)
                ai_flags |= flag;
            else
                ai_flags &= ~flag;
        }

    char *ai_pfx, *ai_sfx;      // substitution strings
    unsigned int ai_flags;      // mode flags, CVAL_?
};


// The main alias table object, for use reading/writing layout files.
//
struct FIOaliasTab : public cv_alias_info
{
    FIOaliasTab(bool, bool, const cv_alias_info* = 0);
    ~FIOaliasTab();

    void reinit(const char* = 0);
    void set_frozen();
    const char *alias(const char*);
    const char *new_name(const char*, bool);
    void set_alias(const char*, const char*);
    void read_alias(const char*);
    void add_lib_alias(sLibRef*, sLib*);
    void dump_alias(const char*);
    void set_substitutions(const char*, const char*);

    const char *unalias(const char *als)
        {
            if (at_alias_tab)
                return (at_alias_tab->find_name(als));
            return (0);
        }

    void unseen(const char *n)
        {
            if (at_seen)
                 at_seen->remove(n);
        }

private:
    bool alt_name(char*, const char*, int*, int ix);

    ATalTab *at_alias_tab;      // name <--> alias bidirectional table
    ATstrTab *at_aliases;       // aliases generated
    strtab_t *at_names;         // names used in alias table
    strtab_t *at_seen;          // names seen but not aliased
    strtab_t *at_accum;         // previously seen but not aliased names
    bool at_dirty;              // alias_tab has changed
    bool at_frozen;             // all cells have been seen
    bool at_use_local;          // use local string storage for aliases
};

#endif


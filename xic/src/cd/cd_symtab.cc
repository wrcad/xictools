
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
 $Id: cd_symtab.cc,v 5.46 2015/07/13 04:48:10 stevew Exp $
 *========================================================================*/

#include "cd.h"
#include <algorithm>
#include <string.h>


//-------------------------------------------------------------------------
// General purpose symbol table
//-------------------------------------------------------------------------

// Constructor
//  free_tag:  tag will be freed (as a char*) when table destroyed.
//  free_data: data will be freed (as a char*) when table destroyed.
//  hashwidth: log of initial hash size.
//
// The initial hash size can be made large to minimize indirection on
// access, for speed in small tables.  For example, suppose the table
// will contain about 64 entries.  Give hashwidth = 6, so that the
// average list length of each bin is 1.
//
SymTab::SymTab(bool free_tag, bool free_data, int hashwidth)
{
    tNumAllocated = 0;
    if (hashwidth > 0 && hashwidth < 18)
        tMask = ~((unsigned int)-1 << hashwidth);
    else
        tMask = ST_START_MASK;

    tEnt = new SymTabEnt*[tMask + 1];
    for (unsigned int i = 0; i <= tMask; i++)
        tEnt[i] = 0;
    tFlags = 0;
    if (free_tag)
        tFlags |= ST_FREE_TAG;
    if (free_data)
        tFlags |= ST_FREE_DATA;
    tMode = STnone;
}


SymTab::~SymTab()
{
    for (unsigned int i = 0; i <= tMask; i++) {
        SymTabEnt *hn;
        for (SymTabEnt *h = tEnt[i]; h; h = hn) {
            hn = h->stNext;
            if (tFlags & ST_FREE_TAG)
                delete [] h->stTag;
            if (tFlags & ST_FREE_DATA)
                delete [] (char*)h->stData;
            delete h;
        }
    }
    delete [] tEnt;
}


void
SymTab::clear()
{
    // Note that the case-sensitivity flag is retained.
    if (tNumAllocated) {
        for (unsigned int i = 0; i <= tMask; i++) {
            SymTabEnt *hn;
            for (SymTabEnt *h = tEnt[i]; h; h = hn) {
                hn = h->stNext;
                if (tFlags & ST_FREE_TAG)
                    delete [] h->stTag;
                if (tFlags & ST_FREE_DATA)
                    delete [] (char*)h->stData;
                delete h;
            }
            tEnt[i] = 0;
        }
        tNumAllocated = 0;
    }
    tMode = STnone;
}


namespace {
    unsigned int str_hash(const char *str, unsigned int mask, bool ci)
    {
        return (ci ? string_hash_ci(str, mask) : string_hash(str, mask));
    }


    bool str_comp(const char *s1, const char *s2, bool ci)
    {
        return (ci ? str_compare_ci(s1, s2) : str_compare(s1, s2));
    }
}


// Add the data to the symbol table, keyed by character
// string tag.  Check for existing tag if check_unique
// is set;
//
bool
SymTab::add(const char *tag, const void *data, bool check_unique)
{
    if (tMode == STint)
        return (false);
    tMode = STchar;
    bool ci = case_insens();
    unsigned int i = str_hash(tag, tMask, ci);
    SymTabEnt *h;
    if (check_unique) {
        for (h = tEnt[i]; h; h = h->stNext) {
            if (str_comp(tag, h->stTag, ci))
                return (false);
        }
    }
    h = new SymTabEnt(tag, data);
    h->stNext = tEnt[i];
    tEnt[i] = h;
    tNumAllocated++;
    if (tNumAllocated/(tMask + 1) > ST_MAX_DENS)
        rehash();
    return (true);
}


// Add the data to the symbol table, keyed by integer
// itag.
//
bool
SymTab::add(unsigned long itag, const void *data, bool check_unique)
{
    if (tMode == STchar)
        return (false);
    tMode = STint;
    unsigned int i = number_hash(itag, tMask);
    SymTabEnt *h;
    if (check_unique) {
        for (h = tEnt[i]; h; h = h->stNext) {
            if (h->stTag == (char*)itag)
                return (false);
        }
    }
    h = new SymTabEnt((char*)itag, data);
    h->stNext = tEnt[i];
    tEnt[i] = h;
    tNumAllocated++;
    if (tNumAllocated/(tMask + 1) > ST_MAX_DENS)
        rehash();
    return (true);
}


// If tag is found, return false and update the data (maybe free old).
// If tag is not found, add an entry using a copy of tag and return
// true.
//
bool
SymTab::replace(const char *tag, const void *data)
{
    if (tMode == STint)
        return (false);
    tMode = STchar;
    bool ci = case_insens();
    unsigned int i = str_hash(tag, tMask, ci);
    SymTabEnt *h;
    for (h = tEnt[i]; h; h = h->stNext) {
        if (str_comp(tag, h->stTag, ci)) {
            if (tFlags & ST_FREE_DATA)
                delete [] (char*)h->stData;
            h->stData = data;
            return (false);
        }
    }
    h = new SymTabEnt(lstring::copy(tag), data);
    h->stNext = tEnt[i];
    tEnt[i] = h;
    tNumAllocated++;
    if (tNumAllocated/(tMask + 1) > ST_MAX_DENS)
        rehash();
    return (true);
}


// If itag is found, return false and update the data (maybe free
// old).  If itag is not found, add an entry and return true.
//
bool
SymTab::replace(unsigned long itag, const void *data)
{
    if (tMode == STchar)
        return (false);
    tMode = STint;
    unsigned int i = number_hash(itag, tMask);
    SymTabEnt *h;
    for (h = tEnt[i]; h; h = h->stNext) {
        if (h->stTag == (char*)itag) {
            if (tFlags & ST_FREE_DATA)
                delete [] (char*)h->stData;
            h->stData = data;
            return (false);
        }
    }
    h = new SymTabEnt((char*)itag, data);
    h->stNext = tEnt[i];
    tEnt[i] = h;
    tNumAllocated++;
    if (tNumAllocated/(tMask + 1) > ST_MAX_DENS)
        rehash();
    return (true);
}


// Remove and possibly delete the data item keyed by string tag from the
// database.
//
bool
SymTab::remove(const char *tag)
{
    if (!tag)
        return (false);
    if (tMode == STint)
        return (false);
    bool ci = case_insens();
    unsigned int i = str_hash(tag, tMask, ci);
    SymTabEnt *hp = 0;
    for (SymTabEnt *h = tEnt[i]; h; h = h->stNext) {
        if (str_comp(tag, h->stTag, ci)) {
            if (!hp)
                tEnt[i] = h->stNext;
            else
                hp->stNext = h->stNext;
            if (tFlags & ST_FREE_TAG)
                delete [] h->stTag;
            if (tFlags & ST_FREE_DATA)
                delete [] (char*)h->stData;
            delete h;
            tNumAllocated--;
            break;
        }
        hp = h;
    }
    return (true);
}


// Remove and possibly delete the data item keyed by integer itag from the
// database.
//
bool
SymTab::remove(unsigned long itag)
{
    if (tMode == STchar)
        return (false);
    unsigned int i = number_hash(itag, tMask);
    SymTabEnt *hp = 0;
    for (SymTabEnt *h = tEnt[i]; h; h = h->stNext) {
        if (h->stTag == (char*)itag ) {
            if (!hp)
                tEnt[i] = h->stNext;
            else
                hp->stNext = h->stNext;
            if (tFlags & ST_FREE_TAG)
                delete [] h->stTag;
            if (tFlags & ST_FREE_DATA)
                delete [] (char*)h->stData;
            delete h;
            tNumAllocated--;
            break;
        }
        hp = h;
    }
    return (true);
}


//
// The remaining functions are private, and are called through static
// inline wrappers so we can query a null table without adverse
// consequences.
//

// Return the data keyed by string tag.  If not found,
// return the datum ST_NIL.
//
void *
SymTab::get_prv(const char *tag)
{
    if (tMode != STchar)
        return (ST_NIL);
    bool ci = case_insens();
    unsigned int i = str_hash(tag, tMask, ci);
    for (SymTabEnt *h = tEnt[i]; h; h = h->stNext) {
        if (str_comp(tag, h->stTag, ci))
            return ((void*)h->stData);
    }
    return (ST_NIL);
}


// Return the data keyed by integer itag.  If not found,
// return the datum ST_NIL.
//
void *
SymTab::get_prv(unsigned long itag)
{
    if (tMode != STint)
        return (ST_NIL);
    unsigned int i = number_hash(itag, tMask);
    for (SymTabEnt *h = tEnt[i]; h; h = h->stNext) {
        if (h->stTag == (char*)itag)
            return ((void*)h->stData);
    }
    return (ST_NIL);
}


// Return a pointer to the internal entry struct for tag, or 0 if not
// in table.  The data pointer can be altered, otherwise this should
// not be messed with.
//
SymTabEnt *
SymTab::get_ent_prv(const char *tag)
{
    if (tMode != STchar)
        return (0);
    bool ci = case_insens();
    unsigned int i = str_hash(tag, tMask, ci);
    for (SymTabEnt *h = tEnt[i]; h; h = h->stNext) {
        if (str_comp(tag, h->stTag, ci))
            return (h);
    }
    return (0);
}


// Return a pointer to the internal entry struct for itag, or 0 if not
// in table.  The data pointer can be altered, otherwise this should
// not be messed with.
//
SymTabEnt *
SymTab::get_ent_prv(unsigned long itag)
{
    if (tMode != STint)
        return (0);
    unsigned int i = number_hash(itag, tMask);
    for (SymTabEnt *h = tEnt[i]; h; h = h->stNext) {
        if (h->stTag == (char*)itag)
            return (h);
    }
    return (0);
}


// Return a list of the names
//
stringlist *
SymTab::names_prv()
{
    if (tMode != STchar)
        return (0);
    stringlist *s0 = 0;
    for (unsigned int i = 0; i <= tMask; i++) {
        for (SymTabEnt *h = tEnt[i]; h; h = h->stNext)
            s0 = new stringlist(lstring::copy(h->stTag), s0);
    }
    return (s0);
}


// Grow the hash table width.  This is called after adding elements if
// the element count to width ratio exceeds ST_MAX_DENS.
//
void
SymTab::rehash()
{
    unsigned int oldmask = tMask;
    tMask = (oldmask << 1) | 1;
    SymTabEnt **oldent = tEnt;
    tEnt = new SymTabEnt*[tMask + 1];
    for (unsigned int i = 0; i <= tMask; i++)
        tEnt[i] = 0;
    if (tMode == STchar) {
        bool ci = case_insens();
        for (unsigned int i = 0; i <= oldmask; i++) {
            SymTabEnt *hn;
            for (SymTabEnt *h = oldent[i]; h; h = hn) {
                hn = h->stNext;
                unsigned int j = str_hash(h->stTag, tMask, ci);
                h->stNext = tEnt[j];
                tEnt[j] = h;
            }
        }
    }
    else if (tMode == STint) {
        for (unsigned int i = 0; i <= oldmask; i++) {
            SymTabEnt *hn;
            for (SymTabEnt *h = oldent[i]; h; h = hn) {
                hn = h->stNext;
                unsigned int j = number_hash((unsigned long)h->stTag, tMask);
                h->stNext = tEnt[j];
                tEnt[j] = h;
            }
        }
    }
    delete [] oldent;
}
// End SymTab functions


//
// A generator for iterating through the table.  This should not
// be used with multi-thread write access to the SymTab.
//

// Constructor, if remove is true, returned elements are removed.
//
SymTabGen::SymTabGen(SymTab *st, bool remove_element)
{
    tab = st;
    ix = 0;
    ent = st ? st->tEnt[0] : 0;
    remove = remove_element;
}


// Iterator, the returned value is a pointer to the internal storage,
// so be careful if modifying (unless remove is true, in which case it
// should be freed).
//
SymTabEnt *
SymTabGen::next()
{
    if (!tab)
        return (0);
    for (;;) {
        if (!ent) {
            ix++;
            if (ix > tab->tMask) {
                if (remove) {
                    tab->tNumAllocated = 0;
                    tab->tMode = STnone;
                }
                return (0);
            }
            ent = tab->tEnt[ix];
            continue;
        }
        SymTabEnt *e = ent;
        ent = ent->stNext;
        if (remove) {
            tab->tEnt[ix] = ent;
            tab->tNumAllocated--;
        }
        return (e);
    }
}
// End of SymTabGen functions


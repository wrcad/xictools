
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 1996 Whiteley Research Inc, all rights reserved.        *
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
 *========================================================================*
 *                                                                        *
 * Whiteley Research Circuit Simulation and Analysis Tool                 *
 *                                                                        *
 *========================================================================*
 $Id: hash.cc,v 1.3 2015/06/20 03:31:34 stevew Exp $
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3e2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1993 Stephen R. Whiteley
****************************************************************************/

#include "hash.h"
#ifdef WRSPICE
#include "wlist.h"
#endif


sHtab::sHtab(bool case_insens)
{
    ht_allocated = 0;
    ht_hashmask = HTAB_START_MASK;
    ht_base = new sHent*[ht_hashmask + 1];
    memset(ht_base, 0, (ht_hashmask + 1)*sizeof(sHent*));
    ht_ci = case_insens;
}


// Destructor, does NOT free the data.
//
sHtab::~sHtab()
{
    for (unsigned int i = 0; i <= ht_hashmask; i++) {
        sHent *h, *hh;
        for (h = ht_base[i]; h; h = hh) {
            hh = h->h_next;
            delete h;
        }
    }
    delete [] ht_base;
}


// A string hashing function (Bernstein, comp.lang.c).
//
inline unsigned int
sHtab::ht_hash(const char *str) const
{
    if (!ht_hashmask || !str)
        return (0);
    unsigned int hash = 5381;
    if (ht_ci) {
        for ( ; *str; str++) {
            unsigned char c = isupper(*str) ? tolower(*str) : *str;
            hash = ((hash << 5) + hash) ^ c;
        }
    }
    else {
        for ( ; *str; str++)
            hash = ((hash << 5) + hash) ^ *(unsigned char*)str;
    }
    return (hash & ht_hashmask);
}


// String comparison function.
//
inline int
sHtab::ht_comp(const char *s, const char *t) const
{
    if (ht_ci) {
        char c1 = isupper(*s) ? tolower(*s) : *s;
        char c2 = isupper(*t) ? tolower(*t) : *t;
        while (c1 && c1 == c2) {
            s++;
            t++;
            c1 = isupper(*s) ? tolower(*s) : *s;
            c2 = isupper(*t) ? tolower(*t) : *t;
        }
        return (c1 - c2);
    }
    else {
        while (*s && *s == *t) {
            s++;
            t++;
        }
        return (*s - *t);
    }
}


// Change the case-sensitivity status of the table.
//
void
sHtab::chg_ciflag(bool ci)
{
    if (ht_ci == ci)
        return;
    ht_ci = ci;
    sHent **oldent = ht_base;
    ht_base = new sHent*[ht_hashmask + 1];
    for (unsigned int i = 0; i <= ht_hashmask; i++)
        ht_base[i] = 0;   
    for (unsigned int i = 0; i <= ht_hashmask; i++) {
        sHent *hn;
        for (sHent *h = oldent[i]; h; h = hn) {
            hn = h->h_next;
            h->h_next = 0;
            unsigned int n = ht_hash(h->h_name);

            sHent *hx = ht_base[n];
            if (hx == 0) {
                ht_base[n] = h;
                continue;
            }

            sHent *hprv = 0;
            for ( ; hx; hprv = hx, hx = hx->h_next) {
                if (ht_comp(h->h_name, hx->h_name) <= 0)
                    continue;
                if (hprv) {
                    hprv->h_next = h;
                    hprv = hprv->h_next;
                }
                else {
                    hprv = h;
                    ht_base[n] = hprv;
                }
                hprv->h_next = hx;
                hprv = 0;
                break;
            }
            if (hprv)
                hprv->h_next = h;
        }
    }
    delete [] oldent;
}


// Add data and associated name to database.  If name is already in
// database, just return, data field is not changed.
//
void
sHtab::add(const char *name, void *data)
{
    {
        sHtab *ht = this;
        if (!ht)
            return;
    }
    if (!name)
        return;
    unsigned int n = ht_hash(name);
    sHent *h = ht_base[n];
    if (h == 0) {
        ht_base[n] = new sHent(name, data);
        ht_allocated++;
        if (ht_allocated/(ht_hashmask + 1) > HTAB_MAX_DENS)
            ht_rehash();
        return;
    }

    sHent *hprv = 0;
    for ( ; h; hprv = h, h = h->h_next) {
        int i = ht_comp(name, h->h_name);
        if (i < 0)
            continue;
        if (i == 0)
            return;
        if (hprv) {
            hprv->h_next = new sHent(name, data);
            hprv = hprv->h_next;
        }
        else {
            hprv = new sHent(name, data);
            ht_base[n] = hprv;
        }
        hprv->h_next = h;
        ht_allocated++;
        if (ht_allocated/(ht_hashmask + 1) > HTAB_MAX_DENS)
            ht_rehash();
        return;
    }
    hprv->h_next = new sHent(name, data);
    ht_allocated++;
    if (ht_allocated/(ht_hashmask + 1) > HTAB_MAX_DENS)
        ht_rehash();
}


// Remove name from the database.  The data entry is not freed, and is
// returned.
//
void *
sHtab::remove(const char *name)
{
    {
        sHtab *ht = this;
        if (!ht)
            return (0);
    }
    if (!name)
        return (0);
    unsigned int n = ht_hash(name);
    sHent *h = ht_base[n];
    for (sHent *hprv = 0; h; hprv = h, h = h->h_next) {
        int i = ht_comp(name, h->h_name);
        if (i < 0)
            continue;
        if (i == 0) {
            if (hprv)
                hprv->h_next = h->h_next;
            else
                ht_base[n] = h->h_next;
            void *dat = h->h_data;
            delete h;
            ht_allocated--;
            return (dat);
        }
        break;
    }
    return (0);
}


// Return the data associatd with name.  O is returned if not found.
//
void *
sHtab::get(const char *name) const
{
    {
        const sHtab *ht = this;
        if (!ht)
            return (0);
    }
    if (!name)
        return (0);
    unsigned int n = ht_hash(name);
    for (sHent *h = ht_base[n]; h; h = h->h_next) {
        int i = ht_comp(name, h->h_name);
        if (i < 0)
            continue;
        if (i == 0)
            return (h->h_data);
        break;
    }
    return (0);
}


// Return the entry associatd with name.  O is returned if not found.
// The data can be changed.
//
sHent *
sHtab::get_ent(const char *name) const
{
    {
        const sHtab *ht = this;
        if (!ht)
            return (0);
    }
    if (!name)
        return (0);
    unsigned int n = ht_hash(name);
    for (sHent *h = ht_base[n]; h; h = h->h_next) {
        int i = ht_comp(name, h->h_name);
        if (i < 0)
            continue;
        if (i == 0)
            return (h);
        break;
    }
    return (0);
}


// Print the entries to stderr, according to datafmt, which should have
// "%d (for hash index), %s (for name), and data format, in that order.
//
void
sHtab::print(const char *datafmt) const
{
    {
        const sHtab *ht = this;
        if (!ht)
            return;
    }
    if (datafmt == 0)
        datafmt = "hash=%d name=%s data=%x\n";
    for (unsigned int i = 0; i <= ht_hashmask; i++) {
        for (sHent *h = ht_base[i]; h; h = h->h_next)
            fprintf(stderr, datafmt, i, h->h_name, h->h_data);
    }
}


// Return a wordlist of the names in the database.
//
wordlist *
sHtab::wl() const
{
#ifdef WRSPICE
    {
        const sHtab *ht = this;
        if (!ht)
            return (0);
    }
    wordlist *twl = 0, *wl0 = 0;
    for (unsigned int i = 0; i <= ht_hashmask; i++) {
        for (sHent *h = ht_base[i]; h; h = h->h_next) {
            if (wl0 == 0)
                wl0 = twl = new wordlist(h->h_name, 0);
            else {
                twl->wl_next = new wordlist(h->h_name, twl);
                twl = twl->wl_next;
            }
        }
    }
    return (wl0);
#else
    return (0);
#endif
}


// Return true if the database contains no data.
//
bool
sHtab::empty() const
{
    {
        const sHtab *ht = this;
        if (!ht)
            return (true);
    }
    return (ht_allocated == 0);
}


// Clear the data, this is not done in the destructor.  If a callback
// is not given, the data pointer is destroyed as a char*.
//
void
sHtab::clear_data(void(*cb)(void*, void*), void *user_arg)
{
    {
        sHtab *ht = this;
        if (!ht)
            return;
    }
    for (unsigned int i = 0; i <= ht_hashmask; i++) {
        for (sHent *h = ht_base[i]; h; h = h->h_next) {
            if (cb)
                (*cb)(h->h_data, user_arg);
            else
                delete [] (char*)h->h_data;
            h->h_data = 0;
        }
    }
}


// Private function to adjust hash width when necessary, called after
// additions.
//
void
sHtab::ht_rehash()
{
    unsigned int oldmask = ht_hashmask;
    ht_hashmask = (oldmask << 1) | 1;
    sHent **oldent = ht_base;
    ht_base = new sHent*[ht_hashmask + 1];
    for (unsigned int i = 0; i <= ht_hashmask; i++)
        ht_base[i] = 0;   
    for (unsigned int i = 0; i <= oldmask; i++) {
        sHent *hn;
        for (sHent *h = oldent[i]; h; h = hn) {
            hn = h->h_next;
            h->h_next = 0;
            unsigned int n = ht_hash(h->h_name);

            sHent *hx = ht_base[n];
            if (hx == 0) {
                ht_base[n] = h;
                continue;
            }

            sHent *hprv = 0;
            for ( ; hx; hprv = hx, hx = hx->h_next) {
                if (ht_comp(h->h_name, hx->h_name) <= 0)
                    continue;
                if (hprv) {
                    hprv->h_next = h;
                    hprv = hprv->h_next;
                }
                else {
                    hprv = h;
                    ht_base[n] = hprv;
                }
                hprv->h_next = hx;
                hprv = 0;
                break;
            }
            if (hprv)
                hprv->h_next = h;
        }
    }
    delete [] oldent;
}


bool sHtab::ht_ciflags[CSE_NUMTYPES] = {
    true,       // functions, case-insensitive
    true,       // user-defined functions, case insensitive
    true,       // vectors, case-sensitive
    true,       // .param names, case insensitive
    true,       // codeblock names, case sensitive
    true        // node and device names, case sensitive
};


// Static function.
// Parse a string that sets the case sensitivity flags for the various
// classes.  This takes to form of a sequence of characters, from
// among those listed below (unknown characters are ignored).  If
// lower case, the associated class is case-sensitive.  If upper case,
// the associated class is case-insensitive.
//
//  f,F  functions
//  u,U  user-defined functions
//  v,V  vectors
//  p,P  .param parameter names
//  c,C  codeblock names
//  n,N  node and device names
//
// Example:  "FPv" will set functions and parameters to be
// case-insensitive, vectors will be case sensitive.
//
void
sHtab::parse_ciflags(const char *s)
{
    while (*s) {
        switch (*s) {
        case 'f':
            set_ciflag(CSE_FUNC, false);
            break;
        case 'F':
            set_ciflag(CSE_FUNC, true);
            break;
        case 'u':
            set_ciflag(CSE_UDF, false);
            break;
        case 'U':
            set_ciflag(CSE_UDF, true);
            break;
        case 'v':
            set_ciflag(CSE_VEC, false);
            break;
        case 'V':
            set_ciflag(CSE_VEC, true);
            break;
        case 'p':
            set_ciflag(CSE_PARAM, false);
            break;
        case 'P':
            set_ciflag(CSE_PARAM, true);
            break;
        case 'c':
            set_ciflag(CSE_CBLK, false);
            break;
        case 'C':
            set_ciflag(CSE_CBLK, true);
            break;
        case 'n':
            set_ciflag(CSE_NODE, false);
            break;
        case 'N':
            set_ciflag(CSE_NODE, true);
            break;
        }
        s++;
    }
}
// End of sHtab functions


// Generator, constructor.
//
sHgen::sHgen(const sHtab *st)
{
    tab = st;
    ix = 0;
    ent = st ? st->ht_base[0] : 0;
    remove = false;
}


// Generator, constructor.  If remove is true, returned elements are
sHgen::sHgen(sHtab *st, bool remove_element)
{
    tab = st;
    ix = 0;
    ent = st ? st->ht_base[0] : 0;
    remove = remove_element;
}

         
// Generator, iterator.  The returned value is a pointer to the internal
// storage, so be careful if modifying (unless remove is true, in which
// case it should be freed)
//
sHent *
sHgen::next()
{
    if (!tab)
        return (0);
    for (;;) {
        if (!ent) {
            ix++;
            if (ix > tab->ht_hashmask) {
                if (remove)
                    ((sHtab*)tab)->ht_allocated = 0;
                return (0);
            }
            ent = tab->ht_base[ix];
            continue;
        }
        sHent *e = ent;
        ent = ent->next();
        if (remove) {
            ((sHtab*)tab)->ht_base[ix] = ent;
            ((sHtab*)tab)->ht_allocated--;
        }
        return (e);
    }
}
// End of sHgen functions


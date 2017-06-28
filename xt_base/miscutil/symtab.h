
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
 $Id: symtab.h,v 1.27 2017/04/16 20:27:53 stevew Exp $
 *========================================================================*/

#ifndef SYMTAB_H
#define SYMTAB_H

#include "lstring.h"
#include "hashfunc.h"
#include <stdlib.h>


//
// Hash table templates for various purposes.
//

//-----------------------------------------------------------------------------
// stab_t:  Table base

// Table growth threshhold
#define ST_MAX_DENS     5

// Base class for low-overhead symbol tables
//
template <class T>
struct stab_t
{
    stab_t()
        {
            count = 0;
            hashmask = 0;
            tab[0] = 0;
        }

    void *operator new(size_t sz)       { return (malloc(sz)); }
    void operator delete(void *p)       { free(p); }

    int allocated()                     { return (count); }
    int hashwidth()                     { return (hashmask+1); }
    T **array()                         { return (tab); }

    void dump(bool verbose)
    {
        unsigned int mx = 0;
        unsigned int mn = 0xffffffff;
        for (unsigned int i = 0; i <= this->hashmask; i++) {
            unsigned int cnt = 0;
            for (T *e = tab[i]; e; e = e->tab_next())
                cnt++;
            if (cnt > mx)
                mx = cnt;
            if (cnt < mn)
                mn = cnt;
            if (verbose)
                printf("%-6d %d\n", i, cnt);
        }
        printf("alloc=%d mask=%d min=%d max=%d\n", this->allocated(),
            this->hashmask, mn, mx);
    }

protected:
    unsigned int count;     // Number of elements in table.
    unsigned int hashmask;  // Hash table size - 1, size is power 2.
    T *tab[1];          // Start of the hash table (extended as necessary).
};


//-----------------------------------------------------------------------------
// table_t<>: String-keyed table

// Template for a low-overhead string-keyed symbol table.  The
// elements must provide the following public methods:
//
//    const char *tab_name();
                            // Publicly accessible tag name.  Be sure to
//                          // remove/reinsert if the name changes!
//    T *tab_next();
//    void set_tab_next(T*);
                            // Functions to set/access pointer to next
                            // element.
//

// The table itself.  This is not fixed size - size will grow with hash
// size, in partular it starts out as a linked list (hashsize = 1)
//
template <class T>
struct table_t : public stab_t<T>
{
    // Destructor does *not* clear content.
    void clear()
        {
            for (unsigned int i = 0; i <= this->hashmask; i++) {
                while (this->tab[i]) {
                    T *n = this->tab[i]->tab_next();
                    delete this->tab[i];
                    this->tab[i] = n;
                }
                this->tab[i] = 0;
            }
            this->count = 0;
        }

    T *find(const char*);
    T *remove(const char*);
    T *link(T*, bool = true);
    T *unlink(T*);
    table_t<T> *check_rehash();
};


// Return the named element, or 0 if not found.
//
template <class T> T *
table_t<T>::find(const char *tag)
{
    unsigned int i = string_hash(tag, this->hashmask);
    for (T *e = this->tab[i]; e; e = e->tab_next()) {
        if (str_compare(tag, e->tab_name()))
            return (e);
    }
    return (0);
}


// If an element matches the name passed, unlink and return it,
// otherwise return 0.
//
template <class T> T *
table_t<T>::remove(const char *tag)
{
    unsigned int i = string_hash(tag, this->hashmask);
    T *ep = 0;
    for (T *e = this->tab[i]; e; e = e->tab_next()) {
        if (str_compare(tag, e->tab_name())) {
            if (!ep)
                this->tab[i] = e->tab_next();
            else
                ep->set_tab_next(e->tab_next());
            e->set_tab_next(0);
            this->count--;
            return (e);
        }
        ep = e;
    }
    return (0);
}


// If check and an element of the same name is already in the table,
// return it.  Otherwise, link in the element passed and return it.
// Return 0 only if the table or passed element is 0.
//
template <class T> T *
table_t<T>::link(T *el, bool check)
{
    if (el) {
        unsigned int i = string_hash(el->tab_name(), this->hashmask);
        if (check) {
            for (T *e = this->tab[i]; e; e = e->tab_next()) {
                if (str_compare(el->tab_name(), e->tab_name()))
                    return (e);
            }
        }
        el->set_tab_next(this->tab[i]);
        this->tab[i] = el;
        this->count++;
        return (el);
    }
    return (0);
}


// If the element is in the table, unlink it and return it.  If not
// found return 0.  Don't mess with the element name string and expect
// this to work.
//
template <class T> T *
table_t<T>::unlink(T *el)
{
    if (el) {
        unsigned int i = string_hash(el->tab_name(), this->hashmask);
        T *ep = 0;
        for (T *e = this->tab[i]; e; e = e->tab_next()) {
            if (e == el) {
                if (!ep)
                    this->tab[i] = e->tab_next();
                else
                    ep->set_tab_next(e->tab_next());
                e->set_tab_next(0);
                this->count--;
                return (e);
            }
            ep = e;
        }
    }
    return (0);
}


// If the density exceeds the limit, rebuild the table, returning the
// new one, or the old one if not rebuilt.  Should be called
// periodically when adding elements.
//
template <class T> table_t<T> *
table_t<T>::check_rehash()
{
    if (this->count/(this->hashmask+1) <= ST_MAX_DENS)
        return (this);

    unsigned int newmask = (this->hashmask << 1) | 1;
    table_t<T> *st =
        (table_t<T>*)malloc(sizeof(table_t<T>) + newmask*sizeof(T*));
    st->count = this->count;
    st->hashmask = newmask;
    for (unsigned int i = 0; i <= newmask; i++)
        st->tab[i] = 0;
    for (unsigned int i = 0;  i <= this->hashmask; i++) {
        T *en;
        for (T *e = this->tab[i]; e; e = en) {
            en = e->tab_next();
            unsigned int j = string_hash(e->tab_name(), newmask);
            e->set_tab_next(st->tab[j]);
            st->tab[j] = e;
        }
        this->tab[i] = 0;
    }
    delete this;
    return (st);
}
// End of table_t template functions.


//-----------------------------------------------------------------------------
// ctable_t<>: String-keyed table, case-insensitive

// Template for a low-overhead string-keyed symbol table.  The
// elements must provide the following public methods:
//
//    const char *tab_name();
                            // Publicly accessible tag name.  Be sure to
//                          // remove/reinsert if the name changes!
//    T *tab_next();
//    void set_tab_next(T*);
                            // Functions to set/access pointer to next
                            // element.
//

// The table itself.  This is not fixed size - size will grow with hash
// size, in partular it starts out as a linked list (hashsize = 1)
//
template <class T>
struct ctable_t : public stab_t<T>
{
    // Destructor does *not* clear content.
    void clear()
        {
            for (unsigned int i = 0; i <= this->hashmask; i++) {
                while (this->tab[i]) {
                    T *n = this->tab[i]->tab_next();
                    delete this->tab[i];
                    this->tab[i] = n;
                }
                this->tab[i] = 0;
            }
            this->count = 0;
        }

    T *find(const char*);
    T *remove(const char*);
    T *link(T*, bool = true);
    T *unlink(T*);
    ctable_t<T> *check_rehash();
};


// Return the named element, or 0 if not found.
//
template <class T> T *
ctable_t<T>::find(const char *tag)
{
    unsigned int i = string_hash_ci(tag, this->hashmask);
    for (T *e = this->tab[i]; e; e = e->tab_next()) {
        if (str_compare_ci(tag, e->tab_name()))
            return (e);
    }
    return (0);
}


// If an element matches the name passed, unlink and return it,
// otherwise return 0.
//
template <class T> T *
ctable_t<T>::remove(const char *tag)
{
    unsigned int i = string_hash_ci(tag, this->hashmask);
    T *ep = 0;
    for (T *e = this->tab[i]; e; e = e->tab_next()) {
        if (str_compare_ci(tag, e->tab_name())) {
            if (!ep)
                this->tab[i] = e->tab_next();
            else
                ep->set_tab_next(e->tab_next());
            e->set_tab_next(0);
            this->count--;
            return (e);
        }
        ep = e;
    }
    return (0);
}


// If check and an element of the same name is already in the table,
// return it.  Otherwise, link in the element passed and return it.
// Return 0 only if the table or passed element is 0.
//
template <class T> T *
ctable_t<T>::link(T *el, bool check)
{
    if (el) {
        unsigned int i = string_hash_ci(el->tab_name(), this->hashmask);
        if (check) {
            for (T *e = this->tab[i]; e; e = e->tab_next()) {
                if (str_compare_ci(el->tab_name(), e->tab_name()))
                    return (e);
            }
        }
        el->set_tab_next(this->tab[i]);
        this->tab[i] = el;
        this->count++;
        return (el);
    }
    return (0);
}


// If the element is in the table, unlink it and return it.  If not
// found return 0.  Don't mess with the element name string and expect
// this to work.
//
template <class T> T *
ctable_t<T>::unlink(T *el)
{
    if (el) {
        unsigned int i = string_hash_ci(el->tab_name(), this->hashmask);
        T *ep = 0;
        for (T *e = this->tab[i]; e; e = e->tab_next()) {
            if (e == el) {
                if (!ep)
                    this->tab[i] = e->tab_next();
                else
                    ep->set_tab_next(e->tab_next());
                e->set_tab_next(0);
                this->count--;
                return (e);
            }
            ep = e;
        }
    }
    return (0);
}


// If the density exceeds the limit, rebuild the table, returning the
// new one, or the old one if not rebuilt.  Should be called
// periodically when adding elements.
//
template <class T> ctable_t<T> *
ctable_t<T>::check_rehash()
{
    if (this->count/(this->hashmask+1) <= ST_MAX_DENS)
        return (this);

    unsigned int newmask = (this->hashmask << 1) | 1;
    ctable_t<T> *st =
        (ctable_t<T>*)malloc(sizeof(table_t<T>) + newmask*sizeof(T*));
    st->count = this->count;
    st->hashmask = newmask;
    for (unsigned int i = 0; i <= newmask; i++)
        st->tab[i] = 0;
    for (unsigned int i = 0;  i <= this->hashmask; i++) {
        T *en;
        for (T *e = this->tab[i]; e; e = en) {
            en = e->tab_next();
            unsigned int j = string_hash_ci(e->tab_name(), newmask);
            e->set_tab_next(st->tab[j]);
            st->tab[j] = e;
        }
        this->tab[i] = 0;
    }
    delete this;
    return (st);
}
// End of ctable_t template functions.


//-----------------------------------------------------------------------------
// sntable_t<>: String and number-keyed table

// Template for a low-overhead string and number-keyed symbol table. 
// String comparison is case-sensitive.  All negative integer values
// are mapped to -1.
// The elements must provide the following public methods:
//
//    const char *tab_name();
//    int tab_indx();
//
//    T *tab_next();
//    void set_tab_next(T*);
//

// The table itself.  This is not fixed size - size will grow with hash
// size, in partular it starts out as a linked list (hashsize = 1).
//
template <class T>
struct sntable_t : public stab_t<T>
{
    // Destructor does *not* clear content.
    void clear()
        {
            for (unsigned int i = 0; i <= this->hashmask; i++) {
                while (this->tab[i]) {
                    T *n = this->tab[i]->tab_next();
                    delete this->tab[i];
                    this->tab[i] = n;
                }
                this->tab[i] = 0;
            }
            this->count = 0;
        }

    T *find(const char*, int);
    T *remove(const char*, int);
    T *link(T*, bool = true);
    T *unlink(T*);
    sntable_t<T> *check_rehash();
};


// Return the specified element, or 0 if not found.
//
template <class T> T *
sntable_t<T>::find(const char *tag, int indx)
{
    unsigned int i = string_num_hash(tag, indx, this->hashmask);
    for (T *e = this->tab[i]; e; e = e->tab_next()) {
        if (string_num_compare(tag, indx, e->tab_name(), e->tab_indx()))
            return (e);
    }
    return (0);
}


// If an element matches the name passed, unlink and return it,
// otherwise return 0.
//
template <class T> T *
sntable_t<T>::remove(const char *tag, int indx)
{
    unsigned int i = string_num_hash(tag, indx, this->hashmask);
    T *ep = 0;
    for (T *e = this->tab[i]; e; e = e->tab_next()) {
        if (string_num_compare(tag, indx, e->tab_name(), e->tab_indx())) {
            if (!ep)
                this->tab[i] = e->tab_next();
            else
                ep->set_tab_next(e->tab_next());
            e->set_tab_next(0);
            this->count--;
            return (e);
        }
        ep = e;
    }
    return (0);
}


// If check and an element of the same name is already in the table,
// return it.  Otherwise, link in the element passed and return it.
// Return 0 only if the table or passed element is 0.
//
template <class T> T *
sntable_t<T>::link(T *el, bool check)
{
    if (el) {
        unsigned int i = string_num_hash(el->tab_name(), el->tab_indx(),
            this->hashmask);
        if (check) {
            for (T *e = this->tab[i]; e; e = e->tab_next()) {
                if (string_num_compare(el->tab_name(), el->tab_indx(),
                        e->tab_name(), e->tab_indx()))
                    return (e);
            }
        }
        el->set_tab_next(this->tab[i]);
        this->tab[i] = el;
        this->count++;
        return (el);
    }
    return (0);
}


// If the element is in the table, unlink it and return it.  If not
// found return 0.  Don't mess with the element name string and expect
// this to work.
//
template <class T> T *
sntable_t<T>::unlink(T *el)
{
    if (el) {
        unsigned int i = string_num_hash(el->tab_name(), el->tab_indx(),
            this->hashmask);
        T *ep = 0;
        for (T *e = this->tab[i]; e; e = e->tab_next()) {
            if (e == el) {
                if (!ep)
                    this->tab[i] = e->tab_next();
                else
                    ep->set_tab_next(e->tab_next());
                e->set_tab_next(0);
                this->count--;
                return (e);
            }
            ep = e;
        }
    }
    return (0);
}


// If the density exceeds the limit, rebuild the table, returning the
// new one, or the old one if not rebuilt.  Should be called
// periodically when adding elements.
//
template <class T> sntable_t<T> *
sntable_t<T>::check_rehash()
{
    if (this->count/(this->hashmask+1) <= ST_MAX_DENS)
        return (this);

    unsigned int newmask = (this->hashmask << 1) | 1;
    sntable_t<T> *st =
        (sntable_t<T>*)malloc(sizeof(sntable_t<T>) + newmask*sizeof(T*));
    st->count = this->count;
    st->hashmask = newmask;
    for (unsigned int i = 0; i <= newmask; i++)
        st->tab[i] = 0;
    for (unsigned int i = 0;  i <= this->hashmask; i++) {
        T *en;
        for (T *e = this->tab[i]; e; e = en) {
            en = e->tab_next();
            unsigned int j = string_num_hash(e->tab_name(), e->tab_indx(),
                newmask);
            e->set_tab_next(st->tab[j]);
            st->tab[j] = e;
        }
        this->tab[i] = 0;
    }
    delete this;
    return (st);
}
// End of sntable_t template functions.


//-----------------------------------------------------------------------------
// csntable_t<>: Case-insensitive string and number-keyed table

// Template for a low-overhead string and number-keyed symbol table. 
// String comparison is case-sensitive.  All negative integer values
// are mapped to -1.
// The elements must provide the following public methods:
//
//    const char *tab_name();
//    int tab_indx();
//
//    T *tab_next();
//    void set_tab_next(T*);
//

// The table itself.  This is not fixed size - size will grow with hash
// size, in partular it starts out as a linked list (hashsize = 1).
//
template <class T>
struct csntable_t : public stab_t<T>
{
    // Destructor does *not* clear content.
    void clear()
        {
            for (unsigned int i = 0; i <= this->hashmask; i++) {
                while (this->tab[i]) {
                    T *n = this->tab[i]->tab_next();
                    delete this->tab[i];
                    this->tab[i] = n;
                }
                this->tab[i] = 0;
            }
            this->count = 0;
        }

    T *find(const char*, int);
    T *remove(const char*, int);
    T *link(T*, bool = true);
    T *unlink(T*);
    csntable_t<T> *check_rehash();
};


// Return the specified element, or 0 if not found.
//
template <class T> T *
csntable_t<T>::find(const char *tag, int indx)
{
    unsigned int i = string_num_hash_ci(tag, indx, this->hashmask);
    for (T *e = this->tab[i]; e; e = e->tab_next()) {
        if (string_num_compare_ci(tag, indx, e->tab_name(), e->tab_indx()))
            return (e);
    }
    return (0);
}


// If an element matches the name passed, unlink and return it,
// otherwise return 0.
//
template <class T> T *
csntable_t<T>::remove(const char *tag, int indx)
{
    unsigned int i = string_num_hash_ci(tag, indx, this->hashmask);
    T *ep = 0;
    for (T *e = this->tab[i]; e; e = e->tab_next()) {
        if (string_num_compare_ci(tag, indx, e->tab_name(),
                e->tab_indx())) {
            if (!ep)
                this->tab[i] = e->tab_next();
            else
                ep->set_tab_next(e->tab_next());
            e->set_tab_next(0);
            this->count--;
            return (e);
        }
        ep = e;
    }
    return (0);
}


// If check and an element of the same name is already in the table,
// return it.  Otherwise, link in the element passed and return it.
// Return 0 only if the table or passed element is 0.
//
template <class T> T *
csntable_t<T>::link(T *el, bool check)
{
    if (el) {
        unsigned int i = string_num_hash_ci(el->tab_name(), el->tab_indx(),
            this->hashmask);
        if (check) {
            for (T *e = this->tab[i]; e; e = e->tab_next()) {
                if (string_num_compare_ci(el->tab_name(), el->tab_indx(),
                        e->tab_name(), e->tab_indx()))
                    return (e);
            }
        }
        el->set_tab_next(this->tab[i]);
        this->tab[i] = el;
        this->count++;
        return (el);
    }
    return (0);
}


// If the element is in the table, unlink it and return it.  If not
// found return 0.  Don't mess with the element name string and expect
// this to work.
//
template <class T> T *
csntable_t<T>::unlink(T *el)
{
    if (el) {
        unsigned int i = string_num_hash_ci(el->tab_name(), el->tab_indx(),
            this->hashmask);
        T *ep = 0;
        for (T *e = this->tab[i]; e; e = e->tab_next()) {
            if (e == el) {
                if (!ep)
                    this->tab[i] = e->tab_next();
                else
                    ep->set_tab_next(e->tab_next());
                e->set_tab_next(0);
                this->count--;
                return (e);
            }
            ep = e;
        }
    }
    return (0);
}


// If the density exceeds the limit, rebuild the table, returning the
// new one, or the old one if not rebuilt.  Should be called
// periodically when adding elements.
//
template <class T> csntable_t<T> *
csntable_t<T>::check_rehash()
{
    if (this->count/(this->hashmask+1) <= ST_MAX_DENS)
        return (this);

    unsigned int newmask = (this->hashmask << 1) | 1;
    csntable_t<T> *st =
        (csntable_t<T>*)malloc(sizeof(csntable_t<T>) + newmask*sizeof(T*));
    st->count = this->count;
    st->hashmask = newmask;
    for (unsigned int i = 0; i <= newmask; i++)
        st->tab[i] = 0;
    for (unsigned int i = 0;  i <= this->hashmask; i++) {
        T *en;
        for (T *e = this->tab[i]; e; e = en) {
            en = e->tab_next();
            unsigned int j = string_num_hash_ci(e->tab_name(), e->tab_indx(),
                newmask);
            e->set_tab_next(st->tab[j]);
            st->tab[j] = e;
        }
        this->tab[i] = 0;
    }
    delete this;
    return (st);
}
// End of csntable_t template functions.


//-----------------------------------------------------------------------------
// itable_t<>: Integer/pointer-keyed table

// Template for a low-overhead integer-keyed symbol table.  The
// elements must provide the following public methods:
//
//    unsigned long tab_key();
                            // Key value.
//    T *tab_next();
//    void set_tab_next(T*);
                            // Functions to set/access pointer to next
                            // element.
//

// The table itself.  This is not fixed size - size will grow with hash
// size, in partular it starts out as a linked list (hashsize = 1)
//
template <class T>
struct itable_t : public stab_t<T>
{
    T *find(unsigned long);
    T *remove(unsigned long);

    // These provide an interface for arbitrary pointers.
    T *find(const void *n) { return (find((unsigned long)n)); }
    T *remove(const void *n) { return (remove((unsigned long)n)); }

    T *link(T*, bool = true);
    T *unlink(T*);
    itable_t<T> *check_rehash();
};


// Return the element, or 0 if not found.
//
template <class T> T *
itable_t<T>::find(unsigned long tag)
{
    unsigned int i = number_hash(tag, this->hashmask);
    for (T *e = this->tab[i]; e; e = e->tab_next()) {
        if (tag == e->tab_key())
            return (e);
    }
    return (0);
}


// If an element matches the tag passed, unlink and return it,
// otherwise return 0.
//
template <class T> T *
itable_t<T>::remove(unsigned long tag)
{
    unsigned int i = number_hash(tag, this->hashmask);
    T *ep = 0;
    for (T *e = this->tab[i]; e; e = e->tab_next()) {
        if (tag == e->tab_key()) {
            if (!ep)
                this->tab[i] = e->tab_next();
            else
                ep->set_tab_next(e->tab_next());
            e->set_tab_next(0);
            this->count--;
            return (e);
        }
        ep = e;
    }
    return (0);
}


// If check and an element of the same key is already in the table,
// return it.  Otherwise, link in the element passed and return it.
// Return 0 only if the table or passed element is 0.
//
template <class T> T *
itable_t<T>::link(T *el, bool check)
{
    if (el) {
        unsigned int i = number_hash(el->tab_key(), this->hashmask);
        if (check) {
            for (T *e = this->tab[i]; e; e = e->tab_next()) {
                if (el->tab_key() == e->tab_key())
                    return (e);
            }
        }
        el->set_tab_next(this->tab[i]);
        this->tab[i] = el;
        this->count++;
        return (el);
    }
    return (0);
}


// If the element is in the table, unlink it and return it.  If not
// found return 0.
//
template <class T> T *
itable_t<T>::unlink(T *el)
{
    if (el) {
        unsigned int i = number_hash(el->tab_key(), this->hashmask);
        T *ep = 0;
        for (T *e = this->tab[i]; e; e = e->tab_next()) {
            if (e == el) {
                if (!ep)
                    this->tab[i] = e->tab_next();
                else
                    ep->set_tab_next(e->tab_next());
                e->set_tab_next(0);
                this->count--;
                return (e);
            }
            ep = e;
        }
    }
    return (0);
}


// If the density exceeds the limit, rebuild the table, returning the
// new one, or the old one if not rebuilt.  Should be called
// periodically when adding elements.
//
template <class T> itable_t<T> *
itable_t<T>::check_rehash()
{
    if (this->count/(this->hashmask+1) <= ST_MAX_DENS)
        return (this);

    unsigned int newmask = (this->hashmask << 1) | 1;
    itable_t<T> *st =
        (itable_t<T>*)malloc(sizeof(itable_t<T>) + newmask*sizeof(T*));
    st->count = this->count;
    st->hashmask = newmask;
    for (unsigned int i = 0; i <= newmask; i++)
        st->tab[i] = 0;
    for (unsigned int i = 0;  i <= this->hashmask; i++) {
        T *en;
        for (T *e = this->tab[i]; e; e = en) {
            en = e->tab_next();
            unsigned int j = number_hash(e->tab_key(), newmask);
            e->set_tab_next(st->tab[j]);
            st->tab[j] = e;
        }
        this->tab[i] = 0;
    }
    delete this;
    return (st);
}
// End of itable_t template functions


//-----------------------------------------------------------------------------
// xytable_t<>: X-Y integer-keyed table

// Template for a low-overhead xy-keyed symbol table.  The
// elements must provide the following definitions
//
//    int tab_x();
//    int tab_y();
//    T *tab_next();
//    void set_tab_next(T*);
//
//    For generator:
//    T *tgen_next(bool) { return (tab_next()); }
//

// The table itself.  This is not fixed size - size will grow with hash
// size, in partular it starts out as a linked list (hashsize = 1)
//
template <class T>
struct xytable_t : public stab_t<T>
{
    T *find(int, int);
    T *remove(int, int);
    T *link(T*, bool = true);
    T *unlink(T*);
    xytable_t<T> *check_rehash();
};


inline unsigned long
xy_hash(int x, int y, unsigned long hashmask)
{
    return (number_hash(x + y, hashmask));
}


// Return the element, or 0 if not found.
//
template <class T> T *
xytable_t<T>::find(int x, int y)
{
    unsigned int i = xy_hash(x, y, this->hashmask);
    for (T *e = this->tab[i]; e; e = e->tab_next()) {
        if (x == e->tab_x() && y == e->tab_y())
            return (e);
    }
    return (0);
}


// If an element matches the tag passed, unlink and return it,
// otherwise return 0.
//
template <class T> T *
xytable_t<T>::remove(int x, int y)
{
    unsigned int i = xy_hash(x, y, this->hashmask);
    T *ep = 0;
    for (T *e = this->tab[i]; e; e = e->tab_next()) {
        if (x == e->tab_x() && y == e->tab_y()) {
            if (!ep)
                this->tab[i] = e->tab_next();
            else
                ep->set_tab_next(e->tab_next());
            e->set_tab_next(0);
            this->count--;
            return (e);
        }
        ep = e;
    }
    return (0);
}


// If check and an element of the same key is already in the table,
// return it.  Otherwise, link in the element passed and return it.
// Return 0 only if the table or passed element is 0.
//
template <class T> T *
xytable_t<T>::link(T *el, bool check)
{
    if (el) {
        unsigned int i = xy_hash(el->tab_x(), el->tab_y(), this->hashmask);
        if (check) {
            for (T *e = this->tab[i]; e; e = e->tab_next()) {
                if (el->tab_x() == e->tab_x() && el->tab_y() == e->tab_y())
                    return (e);
            }
        }
        el->set_tab_next(this->tab[i]);
        this->tab[i] = el;
        this->count++;
        return (el);
    }
    return (0);
}


// If the element is in the table, unlink it and return it.  If not
// found return 0.
//
template <class T> T *
xytable_t<T>::unlink(T *el)
{
    if (el) {
        unsigned int i = xy_hash(el->tab_x(), el->tab_y(), this->hashmask);
        T *ep = 0;
        for (T *e = this->tab[i]; e; e = e->tab_next()) {
            if (e == el) {
                if (!ep)
                    this->tab[i] = e->tab_next();
                else
                    ep->set_tab_next(e->tab_next());
                e->set_tab_next(0);
                this->count--;
                return (e);
            }
            ep = e;
        }
    }
    return (0);
}


// If the density exceeds the limit, rebuild the table, returning the
// new one, or the old one if not rebuilt.  Should be called
// periodically when adding elements.
//
template <class T> xytable_t<T> *
xytable_t<T>::check_rehash()
{
    if (this->count/(this->hashmask+1) <= ST_MAX_DENS)
        return (this);

    unsigned int newmask = (this->hashmask << 1) | 1;
    xytable_t<T> *st =
        (xytable_t<T>*)malloc(sizeof(xytable_t<T>) + newmask*sizeof(T*));
    st->count = this->count;
    st->hashmask = newmask;
    for (unsigned int i = 0; i <= newmask; i++)
        st->tab[i] = 0;
    for (unsigned int i = 0;  i <= this->hashmask; i++) {
        T *en;
        for (T *e = this->tab[i]; e; e = en) {
            en = e->tab_next();
            unsigned int j = xy_hash(e->tab_x(), e->tab_y(), newmask);
            e->set_tab_next(st->tab[j]);
            st->tab[j] = e;
        }
        this->tab[i] = 0;
    }
    delete this;
    return (st);
}
// End of xytable_t template functions


//-----------------------------------------------------------------------------
// ptable_t: Self-keying variation

// This keys the elements by the element address.  The elements must
// provide the following definitions.  Note that elements can be used
// in both types of table simultaneously (CDm does so).
//
//    T *ptab_next();           // Link pointer.
//    void set_ptab_next(T*);
//

// The table itself.  This is not fixed size - size will grow with hash
// size, in partular it starts out as a linked list (hashsize = 1)
//
template <class T>
struct ptable_t : public stab_t<T>
{
    T *find(T*);
    T *add(T*);
    T *remove(T*);
    ptable_t<T> *check_rehash();
};


// Return the element if it is in the table, 0 otherwise.
//
template <class T> T *
ptable_t<T>::find(T *el)
{
    unsigned int i = number_hash((unsigned long)el, this->hashmask);
    for (T *e = this->tab[i]; e; e = e->ptab_next()) {
        if (e == el)
            return (e);
    }
    return (0);
}


// Add the element to the table, if it is not already there.  The
// element is returned on success, 0 otherwise.
//
template <class T> T *
ptable_t<T>::add(T *el)
{
    if (el) {
        unsigned int i = number_hash((unsigned long)el, this->hashmask);
        for (T *e = this->tab[i]; e; e = e->ptab_next()) {
            if (e == el)
                return (e);
        }
        el->set_ptab_next(this->tab[i]);
        this->tab[i] = el;
        this->count++;
        return (el);
    }
    return (0);
}


// Remove the element from the table, returning the element if found,
// or 0 otherwise.
//
template <class T> T *
ptable_t<T>::remove(T *el)
{
    unsigned int i = number_hash((unsigned long)el, this->hashmask);
    T *ep = 0;
    for (T *e = this->tab[i]; e; e = e->ptab_next()) {
        if (e == el) {
            if (!ep)
                this->tab[i] = e->ptab_next();
            else
                ep->set_ptab_next(e->ptab_next());
            e->set_ptab_next(0);
            this->count--;
            return (e);
        }
        ep = e;
    }
    return (0);
}


// If the density exceeds the limit, rebuild the table, returning the
// new one, or the old one if not rebuilt.  Should be called
// periodically when adding elements.
//
template <class T> ptable_t<T> *
ptable_t<T>::check_rehash()
{
    if (this->count/(this->hashmask+1) <= ST_MAX_DENS)
        return (this);

    unsigned int newmask = (this->hashmask << 1) | 1;
    ptable_t<T> *st =
        (ptable_t<T>*)malloc(sizeof(ptable_t<T>) + newmask*sizeof(T*));
    st->count = this->count;
    st->hashmask = newmask;
    for (unsigned int i = 0; i <= newmask; i++)
        st->tab[i] = 0;
    for (unsigned int i = 0;  i <= this->hashmask; i++) {
        T *en;
        for (T *e = this->tab[i]; e; e = en) {
            en = e->ptab_next();
            unsigned int j = number_hash((unsigned long)e, newmask);
            e->set_ptab_next(st->tab[j]);
            st->tab[j] = e;
        }
        this->tab[i] = 0;
    }
    delete this;
    return (st);
}
// End of ptable_t template functions


//-----------------------------------------------------------------------------
// eltab_t<>:  Pool allocator for table elements

// This can be used to allocate any small data object.  Note that new
// objects are not initialized.

#define ST_BLOCKSIZE  (8192 - sizeof(void*))

template <class T>
struct eltab_t
{
    // Storage buffer for list elements.
    struct elbf_t
    {
        elbf_t(elbf_t *n) { next = n; }

        elbf_t *next;
        T elts[ST_BLOCKSIZE/sizeof(T)];
    };

    eltab_t()
        {
            et_elts = 0;
            et_eltcnt = 0;
        }

    ~eltab_t() { clear(); }

    T *new_element()
        {
            if (!et_elts || et_eltcnt >= ST_BLOCKSIZE/sizeof(T)) {
                et_elts = new elbf_t(et_elts);
                et_eltcnt = 0;
            }
            return (et_elts->elts + et_eltcnt++);
        }

    void clear()
        {
            while (et_elts) {
                elbf_t *b = et_elts;
                et_elts = et_elts->next;
                delete b;
            }
            et_eltcnt = 0;
        }

    void zero()
        {
            et_elts = 0;
            et_eltcnt = 0;
        }

    // Return size of data allocated.
    unsigned int memuse()
        {
            unsigned int cnt = 0;
            for (elbf_t *b = et_elts; b; b = b->next, cnt++) ;
            return (cnt * sizeof(elbf_t));
        }

    unsigned int block_size() { return (ST_BLOCKSIZE/sizeof(T)); }
    elbf_t *block_list() { return (et_elts); }

protected:
    elbf_t *et_elts;         // list of list element buffers
    unsigned int et_eltcnt;  // elements used
};


//-----------------------------------------------------------------------------
// stbuf_t:  String pool

// This allocates new strings from a pool.  It does not check for
// duplication.

struct stbuf_t
{
    // String buffer list element
    struct bf_t
    {
        bf_t(bf_t *n) { next = n; }

        bf_t *next;
        char buffer[ST_BLOCKSIZE];
    };

    stbuf_t() { st_blocks = 0; st_offset = 0; }

    ~stbuf_t() { clear(); }

    char *new_string(const char *string)
        {
            int len = strlen(string) + 1;
            if (st_offset + len > ST_BLOCKSIZE || !st_blocks) {
                st_blocks = new bf_t(st_blocks);
                st_offset = 0;
            }
            char *s = st_blocks->buffer + st_offset;
            strcpy(s, string);
            st_offset += len;
            return (s);
        }

    void clear()
        {
            while (st_blocks) {
                bf_t *b = st_blocks;
                st_blocks = st_blocks->next;
                delete b;
            }
            st_offset = 0;
        }

    // Return size of data allocated.
    unsigned int memuse()
        {
            unsigned int cnt = 0;
            for (bf_t *b = st_blocks; b; b = b->next, cnt++) ;
            return (cnt * sizeof(bf_t));
        }

protected:
    bf_t *st_blocks;            // list of string buffers
    unsigned int st_offset;     // current offset into buffer
};


//-----------------------------------------------------------------------------
// strtab_t:  Self-contained string table

// List element for strings, locally allocated.
struct sl_t
{
    const char *tab_name()      { return (name); }
    void set_tab_name(const char *n) { name = n; }
    sl_t *tab_next()            { return (next); }
    void set_tab_next(sl_t *t)  { next = t; }
    sl_t *tgen_next(bool)       { return (next); }

private:
    const char *name;
    sl_t *next;
};

// String table, with local string buffer.
//
struct strtab_t
{
    strtab_t() { st_tab = 0; }
    ~strtab_t() { delete st_tab; }

    // Clear all entries.
    //
    void clear()
        {
            delete st_tab;
            st_tab = 0;
            st_buf.clear();
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
                e->set_tab_name(st_buf.new_string(string));
                st_tab->link(e, false);
                st_tab = st_tab->check_rehash();
            }
            return (e->tab_name());
        }

    bool remove(const char *string)
        {
            if (!string)
                return (false);
            if (!st_tab)
                return (false);
            if (st_tab->remove(string)) {
                st_tab = st_tab->check_rehash();
                return (true);
            }
            return (false);
        }

    inline void merge(strtab_t*);
    inline stringlist *strings();

    // Return heap allocation total.
    unsigned int memuse()
        {
            unsigned int bytes = st_buf.memuse() + st_elts.memuse();
            if (st_tab) {
                bytes += sizeof(table_t<sl_t>) +
                    (st_tab->hashwidth() - 1)*sizeof(void*);
            }
            return (bytes);
        }

private:
    table_t<sl_t> *st_tab;      // offset hash table
    eltab_t<sl_t> st_elts;      // table element factory
    stbuf_t       st_buf;       // string pool
};


// Case-insensitive string table, with local string buffer.
//
struct cstrtab_t
{
    enum SaveMode { SaveFirst, SaveLower, SaveUpper };
    // When we save an entry, it will be in one or these forms:
    // SaveFirst        Save the entry as given the first time.
    // SaveLower        Save in lower-case.
    // SaveUpper        Save in upper-case.

    cstrtab_t(SaveMode sm)  { st_tab = 0; st_mode = sm; }
    ~cstrtab_t()            { delete st_tab; }

    // Clear all entries.
    //
    void clear()
        {
            delete st_tab;
            st_tab = 0;
            st_buf.clear();
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
    // pointer to the table entry.  Note that we don't change case
    // when a string is saved, whatever form is initially given will
    // be used for all future matches.
    //
    const char *add(const char *string)
        {
            if (!string)
                return (0);
            if (!st_tab)
                st_tab = new ctable_t<sl_t>;
            sl_t *e = st_tab->find(string);
            if (!e) {
                e = st_elts.new_element();
                e->set_tab_next(0);
                char *n = st_buf.new_string(string);
                if (st_mode == SaveUpper) {
                    for (char *s = n; *s; s++) {
                        if (islower(*s))
                            *s = toupper(*s);
                    }
                }
                else if (st_mode == SaveLower) {
                    for (char *s = n; *s; s++) {
                        if (isupper(*s))
                            *s = tolower(*s);
                    }
                }
                e->set_tab_name(n);
                st_tab->link(e, false);
                st_tab = st_tab->check_rehash();
            }
            return (e->tab_name());
        }

    bool remove(const char *string)
        {
            if (!string)
                return (false);
            if (!st_tab)
                return (false);
            if (st_tab->remove(string)) {
                st_tab = st_tab->check_rehash();
                return (true);
            }
            return (false);
        }

    inline void merge(cstrtab_t*);
    inline stringlist *strings();

    // Return heap allocation total.
    unsigned int memuse()
        {
            unsigned int bytes = st_buf.memuse() + st_elts.memuse();
            if (st_tab) {
                bytes += sizeof(table_t<sl_t>) +
                    (st_tab->hashwidth() - 1)*sizeof(void*);
            }
            return (bytes);
        }

private:
    ctable_t<sl_t>  *st_tab;      // offset hash table
    eltab_t<sl_t>   st_elts;      // table element factory
    stbuf_t         st_buf;       // string pool
    SaveMode        st_mode;      // save mode: as-is, lower, upper case
};


//-----------------------------------------------------------------------------
// tgen_t:  Element generator for stab_t derivatives

// Class for iteration through table_t, etc.  This can be constructed
// directly from a pointer to a table, or an unsigned long with the 1
// bit set to indicate a table pointer, or a linked list otherwise.
//
// In order to use this the T must have a tgen_next function:
//   T *T::tgen_next(bool b)
//      { return (b ? ptab_next() : tab_next()); }
//
template <class T>
struct tgen_t
{

    tgen_t()
        {
            init(0, false);
        }

    tgen_t(stab_t<T> *t)
        {
            tinit(t);
        }

    tgen_t(unsigned long p, bool ptype)
        {
            init(p, ptype);
        }

    void tinit(stab_t<T> *t)
        {
            hashmask = t ? t->hashwidth() - 1 : 0;
            indx = 0;
            array = t ? t->array() : 0;
            elt = array ? array[0] : 0;
            use_p = false;
        }

    void init(unsigned long p, bool ptype)
        {
            if (p & 1) {
                stab_t<T> *t = (stab_t<T>*)(p & ~1);
                hashmask = t->hashwidth() - 1;
                indx = 0;
                array = t->array();
                elt = array[0];
            }
            else {
                array = 0;
                elt = (T*)p;
                hashmask = 0;
                indx = 0;
            }
            use_p = ptype;
        }

    T *next();

private:
    unsigned int hashmask;
    unsigned int indx;
    T **array;
    T *elt;
    bool use_p;  // use ptab_next if set (for ptable_t)
};


template <class T> T *
tgen_t<T>::next()
{
    for (;;) {
        if (!elt) {
            if (!array)
                return (0);
            indx++;
            if (indx > hashmask)
                return (0);
            elt = array[indx];
            continue;
        }
        T *e = elt;
        elt = elt->tgen_next(use_p);
        return (e);
    }
}


//-----------------------------------------------------------------------------
// ptrtab_t:  Self-contained pointer table

// This saves pointer values, can be used to ensure uniqueness among a
// list of objects by recording and testing addresses.

// List element, locally allocated.
struct pl_t
{
    unsigned long tab_key()         { return (key); }
    void set_tab_key(const void *p) { key = (unsigned long)p; }
    pl_t *tab_next()                { return (next); }
    void set_tab_next(pl_t *t)      { next = t; }
    pl_t *tgen_next(bool)           { return (next); }

private:
    unsigned long key;
    pl_t *next;
};

// Pointer table.
//
struct ptrtab_t
{

    ptrtab_t() { pt_tab = 0; }
    ~ptrtab_t() { delete pt_tab; }

    // Return the matching entry for p, if it exists, null
    // otherwise.
    //
    void *find(const void *p)
        {
            if (!p || !pt_tab)
                return (0);
            pl_t *e = pt_tab->find(p);
            return (e ? (void*)e->tab_key() : 0);
        }

    // Add p to table if not already there.
    //
    void *add(const void *p)
        {
            if (!p)
                return (0);
            if (!pt_tab)
                pt_tab = new itable_t<pl_t>;
            pl_t *e = pt_tab->find(p);
            if (!e) {
                e = pt_elts.new_element();
                e->set_tab_next(0);
                e->set_tab_key(p);
                pt_tab->link(e, false);
                pt_tab = pt_tab->check_rehash();
            }
            return ((void*)e->tab_key());
        }

    // Remove p and return true if found.
    //
    bool remove(const void *p)
        {
            if (!p || !pt_tab)
                return (false);
            return (pt_tab->remove(p) != 0);
        }

    unsigned int allocated()    { return (pt_tab ? pt_tab->allocated() : 0); }

    itable_t<pl_t> *tab()       { return (pt_tab); }

private:
    itable_t<pl_t> *pt_tab;     // offset hash table
    eltab_t<pl_t> pt_elts;      // table element factory
};

// Generator.
struct ptrgen_t
{
    ptrgen_t(ptrtab_t *t) : pg_gen(t ? t->tab() : 0) { }

    void *next()
        {
            pl_t *pl = pg_gen.next();
            if (pl)
                return ((void*)pl->tab_key());
            return (0);
        }

private:
    tgen_t<pl_t> pg_gen;
};


//-----------------------------------------------------------------------------
// Deferred inlines

// Add the strings in tab to this.
//
inline void
strtab_t::merge(strtab_t *tab)
{
    tgen_t<sl_t> gen(tab->st_tab);
    sl_t *sl;
    while ((sl = gen.next()) != 0)
        add(sl->tab_name());
}


// Return a list of the strings.
//
inline stringlist *
strtab_t::strings()
{
    stringlist *s0 = 0;
    tgen_t<sl_t> gen(st_tab);
    sl_t *sl;
    while ((sl = gen.next()) != 0)
        s0 = new stringlist(lstring::copy(sl->tab_name()), s0);
    s0->sort();
    return (s0);
}


// Add the strings in tab to this.
//
inline void
cstrtab_t::merge(cstrtab_t *tab)
{
    tgen_t<sl_t> gen(tab->st_tab);
    sl_t *sl;
    while ((sl = gen.next()) != 0)
        add(sl->tab_name());
}


// Return a list of the strings.
//
inline stringlist *
cstrtab_t::strings()
{
    stringlist *s0 = 0;
    tgen_t<sl_t> gen(st_tab);
    sl_t *sl;
    while ((sl = gen.next()) != 0)
        s0 = new stringlist(lstring::copy(sl->tab_name()), s0);
    s0->sort();
    return (s0);
}

#endif


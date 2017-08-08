
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

#ifndef MEMMGR_TMPL_H
#define MEMMGR_TMPL_H

#include <stdio.h>
#include <unistd.h>

// Define this for multi-threaded safety.
#define MM_PTHREAD

#ifdef WIN32
typedef unsigned long long u_int64_t;
#endif

#ifdef MM_PTHREAD
#include "miscutil/atomic_stack.h"
#include <pthread.h>
#endif


//
// Memory management system for frequently used data structs.
//

#define MMGR_FULL (u_int64_t)-1

// Bits in an u_int64_t.
#define MM_HBITS 64

// Two to this power is MM_HBITS.
#define MM_HBITS_SHIFT 6

// Number of MM_HBITS blocks in bank.
#define MM_BANKSZ 8

// Hash table depth.
#define MMGR_HASH_DEPTH 5

// Error callback.
extern void mm_err_hook(const char*, const char*);


// Return the position and mask of the least significant zero bit, so
// if v is 01001011 (base 2), the result will be 2 and the mask will
// be 00000100.
//
inline unsigned int
least_unset(u_int64_t v, u_int64_t *mask)
{
    // we know that v has an unset bit
    v = ~v;
    v &= -v;  // only the least one is set
    *mask = v;
    unsigned int c = 0;
    if (v & 0xffffffff00000000ull)
        c |= 0x20;
    if (v & 0xffff0000ffff0000ull)
        c |= 0x10;
    if (v & 0xff00ff00ff00ff00ull)
        c |= 0x8;
    if (v & 0xf0f0f0f0f0f0f0f0ull)
        c |= 0x4;
    if (v & 0xccccccccccccccccull)
        c |= 0x2;
    if (v & 0xaaaaaaaaaaaaaaaaull)
        c |= 0x1;
    return (c);
}


// Struct to hold statistics.
//
struct mmgrstat_t
{
    mmgrstat_t()
        {
            strsize = 0;
            full_len = 0;
            full_hash = 0;
            not_full_len = 0;
            not_full_hash = 0;
            inuse = 0;
            not_inuse = 0;
        }

    void print(const char *nm, FILE *fp)
        {
            fprintf(fp, "%-9s sz=%-3u fl=%-7u fh=%-5u nfl=%-7u nfh=%-5u "
                "u=%-10u nu=%u\n",
                nm, strsize, full_len, full_hash, not_full_len,
                not_full_hash, inuse*strsize, not_inuse*strsize);
        }

    unsigned int strsize;           // size of struct
    unsigned int full_len;          // full bank count
    unsigned int full_hash;         // full hash width
    unsigned int not_full_len;      // not-full bank count
    unsigned int not_full_hash;     // not-full hash size
    unsigned int inuse;             // allocations in use
    unsigned int not_inuse;         // allocations not in use
};


// Bank of elements to allocate, containing twice the number of
// elements as bits in an int.  Alignment to 8 bits if element size is
// multiple of 8 bits.
//
template <class T> struct ebank_t
{
    ebank_t(const char *tn)
        {
            u.next = 0;
            for (int i = 0; i < MM_BANKSZ; i++)
                inuse_flags[i] = 0;
            type_name = tn;
            num_used = 0;
        }

    // Return true if p is in bank.
    //
    bool inbank(T *p)
        {
            return (p >= (T*)data && p < (T*)data + MM_BANKSZ*MM_HBITS);
        }

    // Return offset of p in bank, -1 if pointer bogus.
    //
    int bank_offset(T *p)
        {
            unsigned long d = p - (T*)data;
            if (data + d*sizeof(T) != (char*)p) {
                mm_err_hook("bad dealloc pointer!", type_name);
                return (-1);
            }
            return (d);
        }

    // Return true if no elements are in use.
    //
    bool is_empty()
        {
            return (num_used == 0);
        }

    // Return true if all elements are in use.
    //
    bool is_full()
        {
            return (num_used == MM_BANKSZ*MM_HBITS);
        }

    // Return the offset of the first unused element, and set its used
    // flag.
    //
    int first_unused()
        {
            for (int i = 0; i < MM_BANKSZ; i++) {
                if (inuse_flags[i] != MMGR_FULL) {
                    u_int64_t mask;
                    unsigned int os = least_unset(inuse_flags[i], &mask);
                    inuse_flags[i] |= mask;
                    num_used++;
                    return (os + i*MM_HBITS);
                }
            }
            return (-1);
        }

    // Clear the used flag for the given offset.
    //
    void set_unused(unsigned int os)
        {
            int sb = os >> MM_HBITS_SHIFT;
            u_int64_t f = 1ull << (os & (MM_HBITS-1));
            if (inuse_flags[sb] & f) {
                inuse_flags[sb] &= ~f;
                num_used--;
            }
            else
                mm_err_hook("element already free.", type_name);
        }

    union {
        ebank_t<T> *next;                       // link for table
        u_int64_t spacer;                       // not used, alignment only
    } u;
    u_int64_t inuse_flags[MM_BANKSZ];           // usage bits, 1 => in use
    char data[MM_BANKSZ*MM_HBITS*sizeof(T)];    // element store
    const char *type_name;                      // for error messages
    unsigned int num_used;                      // number of elements used
};


// A page table where the banks are stored.
//
template <class T>
struct mttab_t
{
    mttab_t()
        {
            page_shift = 0;
            while ((1 << page_shift) < (int)sizeof(ebank_t<T>))
                page_shift++;
            tab = 0;
            hashmask = 0;
            count = 0;
            first_ix = -1;
        }

    /*
     *  Containing class instance is statically allocated, this
     *  is not needed and dangerous.
    ~mttab_t()
        {
            // free allocated banks, does not call object destructors
            if (tab) {
                for (unsigned int i = 0; i <= hashmask; i++) {
                    while (tab[i]) {
                        ebank_t<T> *x = tab[i];
                        tab[i] = tab[i]->u.next;
                        delete x;
                    }
                }
                delete [] tab;
            }
        }
    */

    unsigned int hash(void *addr)
        {
            unsigned long l = (unsigned long)addr;
            return (hashmask & (unsigned int)(l >> page_shift));
        }

    ebank_t<T> *first()
        {
            if (tab) {
                if (first_ix >= 0)
                    return (tab[first_ix]);
                for (unsigned int i = 0; i <= hashmask; i++) {
                    if (tab[i]) {
                        first_ix = i;
                        return (tab[i]);
                    }
                }
            }
            return (0);
        }

    void insert(ebank_t<T>*);
    ebank_t<T> *remove(ebank_t<T>*);
    ebank_t<T> *find(T*);
    void check_rehash();

    ebank_t<T> **tab;           // hashed bank lists
    unsigned int hashmask;      // hashing mask
    unsigned int count;         // bank in table
    unsigned int page_shift;    // hashing size
    int first_ix;               // index into tab of first entry
};


// Insert bank into the table.
//
template <class T>
void
mttab_t<T>::insert(ebank_t<T> *blk)
{
    if (blk->u.next) {
        mm_err_hook("bank to insert already linked.", blk->type_name);
        return;
    }
    // Don't bother existence checking.

    if (!tab) {
        tab = new ebank_t<T>*[1];
        *tab = 0;
    }
    unsigned int i = hash(blk);
    blk->u.next = this->tab[i];
    this->tab[i] = blk;
    this->count++;
    if ((int)i < this->first_ix)
        this->first_ix = i;
    check_rehash();
}


// Remove that bank from the table, return a pointer to it if removed.
//
template <class T>
ebank_t<T> *
mttab_t<T>::remove(ebank_t<T> *blk)
{
    if (tab) {
        unsigned int i = hash(blk);
        ebank_t<T> *bp = 0;
        for (ebank_t<T> *b = this->tab[i]; b; b = b->u.next) {
            if (b == blk) {
                if (bp)
                    bp->u.next = b->u.next;
                else
                    this->tab[i] = b->u.next;
                b->u.next = 0;
                this->count--;
                if ((int)i == this->first_ix && !this->tab[i]) {
                    unsigned int t = this->first_ix;
                    this->first_ix = -1;
                    for (unsigned int j = t; j <= this->hashmask; j++) {
                        if (this->tab[j]) {
                            this->first_ix = j;
                            break;
                        }
                    }
                }
                check_rehash();
                return (b);
            }
            bp = b;
        }
    }
    return (0);
}


// Return the bank containing data.  The data location is not necessarily
// in the same page as the bank start, so check lower table bin.
//
template <class T>
ebank_t<T> *
mttab_t<T>::find(T *data)
{
    if (tab) {
        unsigned int i = hash(data);
        for (ebank_t<T> *b = this->tab[i]; b; b = b->u.next) {
            if (b->inbank(data))
                return (b);
        }
        if (i)
            i--;
        else
            i = hashmask;

        for (ebank_t<T> *b = this->tab[i]; b; b = b->u.next) {
            if (b->inbank(data))
                return (b);
        }
    }
    return (0);
}


// Reconfigure hashing width as element count changes.
//
template <class T>
void
mttab_t<T>::check_rehash()
{
    unsigned int newmask, ratio = this->count/(this->hashmask+1);
    if (ratio > MMGR_HASH_DEPTH)
        newmask = (this->hashmask << 1) | 1;
    else if (!ratio && this->hashmask > 3)
        newmask = (this->hashmask >> 1);
    else
        return;

    first_ix = -1;
    ebank_t<T> **ntab = new ebank_t<T>*[newmask+1];
    for (unsigned int i = 0; i <= newmask; i++)
        ntab[i] = 0;
    unsigned int oldmask = this->hashmask;
    this->hashmask = newmask;
    for (unsigned int i = 0; i <= oldmask; i++) {
        ebank_t<T> *bn;
        for (ebank_t<T> *b = this->tab[i]; b; b = bn) {
            bn = b->u.next;
            unsigned int j = hash(b);
            b->u.next = ntab[j];
            ntab[j] = b;
        }
    }
    delete [] this->tab;
    this->tab = ntab;
}

#define MM_MAX_FREE_LEN     64

// The "recycling mode" is now always on, but the free list length is
// limited to MM_MAX_FREE_LEN.  Switching recycling mode on/off is not
// possible with multi-threads.  One can call collectTrash() to clear
// the free list if desired.


// The main allocator class.
//
template <class T> struct MemMgr
{
#ifndef HAS_ATOMIC_STACK
    struct freelist_t { freelist_t *next; };
#endif

    MemMgr(const char *tname)
        {
            first_not_full = 0;
            type_name = tname;
#ifndef HAS_ATOMIC_STACK
            free_list = 0;
            free_count = 0;
#endif
#ifdef MM_PTHREAD
            pthread_mutex_init(&mtx, 0);
#endif
        }

    void *newElem()
        {
#ifdef HAS_ATOMIC_STACK
            as_elt_t *e = free_stack.pop();
            if (e)
                return (e);
#else
#ifdef MM_PTHREAD
            pthread_mutex_lock(&mtx);
#endif
            if (free_list) {
                void *e = free_list;
                free_list = free_list->next;
                free_count--;
#ifdef MM_PTHREAD  
                pthread_mutex_unlock(&mtx);
#endif
                return (e);
            }
#ifdef MM_PTHREAD  
            pthread_mutex_unlock(&mtx);
#endif
#endif
            return (newElemPrv());
        }

    void delElem(void *pp)
        {
            if (!pp)
                return;
#ifdef HAS_ATOMIC_STACK
            if (free_stack.push((as_elt_t*)pp, MM_MAX_FREE_LEN))
                return;
#else
#ifdef MM_PTHREAD
            pthread_mutex_lock(&mtx);
#endif
            if (free_count < MM_MAX_FREE_LEN) {
                freelist_t *fl = (freelist_t*)pp;
                fl->next = free_list;
                free_list = fl;
                free_count++;
#ifdef MM_PTHREAD
                pthread_mutex_unlock(&mtx);
#endif
                return;
            }
#ifdef MM_PTHREAD
            pthread_mutex_unlock(&mtx);
#endif
#endif
            delElemPrv(pp);
        }

    void collectTrash();        // hard-free the free list
    void *newElemPrv();         // creator
    void delElemPrv(void*);     // destructor
    void stats(mmgrstat_t*);    // statistics
    bool isAllocated(T*);       // is element allocated in this manager?

private:
    ebank_t<T> *first_not_full; // "current" block for allocation
    const char *type_name;      // type name of objects
#ifdef HAS_ATOMIC_STACK
    as_stack_t free_stack;      // "freed" objects for reuse
#else
    freelist_t *free_list;      // "freed" objects for reuse
#endif
    mttab_t<T> full_tab;        // table for full banks
    mttab_t<T> not_full_tab;    // table for not full banks
#ifndef HAS_ATOMIC_STACK
    int free_count;             // number of elements in free list
#endif
#ifdef MM_PTHREAD
    pthread_mutex_t mtx;
#endif
};


// Hard-free the free list.
//
template <class T> void
MemMgr<T>::collectTrash()
{
#ifdef HAS_ATOMIC_STACK
    as_elt_t *e0 = free_stack.list_head();
    while (e0) {
        as_elt_t *e = e0;
        e0 = e0->next;
        delElemPrv(e);
    }
    free_stack.clear();
#else
    freelist_t *fl = free_list;
    free_list = 0;
    free_count = 0;
    while (fl) {
        void* e = fl;
        fl = fl->next;
        delElemPrv(e);
    }
#endif
}


// Element allocator.
//
template <class T> void *
MemMgr<T>::newElemPrv()
{
#ifdef MM_PTHREAD
    pthread_mutex_lock(&mtx);
#endif
    ebank_t<T> *bnk = first_not_full;
    if (!bnk) {
        bnk = new ebank_t<T>(type_name);
        not_full_tab.insert(bnk);
        first_not_full = bnk;
    }
    int os = bnk->first_unused();
    if (os >= 0) {
        if (bnk->is_full()) {
            not_full_tab.remove(bnk);
            full_tab.insert(bnk);
            first_not_full = not_full_tab.first();
        }
#ifdef MM_PTHREAD
        pthread_mutex_unlock(&mtx);
#endif
        return ((T*)bnk->data + os);
    }
#ifdef MM_PTHREAD
    pthread_mutex_unlock(&mtx);
#endif
    mm_err_hook("full bank in not-full table, constructor failed.", type_name);
    return (0);
}


// Element destructor.
//
template <class T> void
MemMgr<T>::delElemPrv(void *pp)
{
#ifdef MM_PTHREAD
    pthread_mutex_lock(&mtx);
#endif
    T *p = (T*)pp;
    ebank_t<T> *bnk = not_full_tab.find(p);
    if (bnk) {
        int os = bnk->bank_offset(p);
        if (os >= 0) {
            bnk->set_unused(os);
            if (bnk != first_not_full && bnk->is_empty()) {
                bnk = not_full_tab.remove(bnk);
                delete bnk;
            }
#ifdef MM_PTHREAD
            pthread_mutex_unlock(&mtx);
#endif
            return;
        }
    }
    else {
        bnk = full_tab.find(p);
        if (bnk) {
            int os = bnk->bank_offset(p);
            if (os >= 0) {
                bnk->set_unused(os);
                full_tab.remove(bnk);
                not_full_tab.insert(bnk);
                ebank_t<T> *nf = not_full_tab.first();
                if (nf != first_not_full) {
                    bnk = first_not_full;
                    if (bnk && bnk->is_empty()) {
                        bnk = not_full_tab.remove(bnk);
                        delete bnk;
                    }
                    first_not_full = nf;
                }
#ifdef MM_PTHREAD
                pthread_mutex_unlock(&mtx);
#endif
                return;
            }
        }
    }
#ifdef MM_PTHREAD
    pthread_mutex_unlock(&mtx);
#endif
    mm_err_hook("bad pointer passed to delElem (not in banks)!", type_name);
}


// Statistics.
//
template <class T> void
MemMgr<T>::stats(mmgrstat_t *st)
{
    st->strsize = sizeof(T);
    if (full_tab.tab) {
        for (unsigned int i = 0; i <= full_tab.hashmask; i++) {
            for (ebank_t<T> *e = full_tab.tab[i]; e; e = e->u.next) {
                if (!e->is_full())
                    mm_err_hook("not full in full table!", type_name);
            }
        }
    }
    st->full_len = full_tab.count;
    st->full_hash = full_tab.hashmask + 1;
    st->not_full_len = not_full_tab.count;
    st->not_full_hash = not_full_tab.hashmask + 1;
    st->inuse = 2*MM_HBITS*st->full_len;
    st->not_inuse = 0;

    if (not_full_tab.tab) {
        for (unsigned int i = 0; i <= not_full_tab.hashmask; i++) {
            for (ebank_t<T> *e = not_full_tab.tab[i]; e; e = e->u.next) {
                if (e->is_full())
                    mm_err_hook("full in not-full table!", type_name);
                if (e->is_empty() && e != first_not_full)
                    mm_err_hook("empty in not-full table!", type_name);
                int cnt = e->num_used;
                st->inuse += cnt;
                st->not_inuse += MM_BANKSZ*MM_HBITS - cnt;
            }
        }
    }
}


template <class T> bool
MemMgr<T>::isAllocated(T *x)
{
    return (not_full_tab.find(x) || full_tab.find(x));
}

#endif


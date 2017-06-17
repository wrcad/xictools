
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
 * Memory Management System                                               *
 *                                                                        *
 *========================================================================*
 $Id: monitor.cc,v 2.17 2015/06/20 03:31:09 stevew Exp $
 *========================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#if defined(__linux) || defined(__APPLE__)
#ifdef __x86_64
#include <execinfo.h>
#endif
#endif
#include "local_malloc.h"


//-----------------------------------------------------------------------
//
// Memory Allocation Monitor (core leak detection)
//
//-----------------------------------------------------------------------

// defines (from Makefile)
// CPP_ONLY                   Monitor new/delete calls only

// environment
// MMON_STARTUP [=N]          Start monitor on application startup,
//                            with optional saved stack depth.  When this
//                            is used, freed objects are tested against
//                            the allocation table.

//-----------------------------------------------------------------------
// Symbol table definition
//-----------------------------------------------------------------------

// Table element, variable size
//
struct melem_t
{
    void *key;          // memory block address
    melem_t *next;      // link
    void *data[1];      // stack
};
 
// The table, variable size
//
struct mtable_t
{
    mtable_t() { count = 0; hashsize = 1; tab[0] = 0; }
    ~mtable_t();

    melem_t *find(void*);
    melem_t *remove(void*);
    melem_t *add(void*);
    melem_t *new_element(void*);
    mtable_t *check_rehash();

    unsigned count;     // number of elements in table
    int hashsize;       // hashing width
    melem_t *tab[1];    // element heads (hashsize)
};


// Destructor, free table and entries
//
mtable_t::~mtable_t()
{
    for (int i = 0; i < hashsize; i++) {
        melem_t *e = tab[i];
        tab[i] = 0;
        while (e) {
            melem_t *ex = e;
            e = e->next;
            delete ex;
        }
    }
}


// Return the element saved for k, if any.
//
melem_t *
mtable_t::find(void *k)
{
    int j = ((unsigned long)k) % hashsize;
    for (melem_t *e = tab[j]; e; e = e->next) {
        if (e->key == k)
            return (e);
    }
    return (0);
}


// Unlink and return the element saved for k, if any.
//
melem_t *
mtable_t::remove(void *k)
{
    int j = ((unsigned long)k) % hashsize;
    melem_t *ep = 0;
    for (melem_t *e = tab[j]; e; e = e->next) {
        if (e->key == k) {
            if (!ep)
                tab[j] = e->next;
            else
                ep->next = e->next;
            count--;
            return (e);
        }
        ep = e;
    }
    return (0);
}


// Create a new element for k and return it.  This never fails, no
// checking for previous existence in table.
//
melem_t *
mtable_t::add(void *k)
{
    int j = ((unsigned long)k) % hashsize;
    melem_t *e = new_element(k);
    e->next = tab[j];
    tab[j] = e;
    count++;
    return (e);
}


melem_t *
mtable_t::new_element(void *k)
{
    melem_t *e = (melem_t*)
        (new char[sizeof(melem_t) + (Memory()->mon_depth()-1)*sizeof(void*)]);
    e->key = k;
    e->next = 0;
    memset(e->data, 0, Memory()->mon_depth()*sizeof(void*));
    return (e);
}


#define MAX_DENS 5

// Enlarge the table if necessary, call periodically after adding elements.
//
mtable_t *
mtable_t::check_rehash()
{
    if (!(void*)this)
        return (0);
    if (count/hashsize <= MAX_DENS)
        return (this);

    int newsize = 2*hashsize + 1;
    mtable_t *st =
        (mtable_t*)new char[sizeof(mtable_t) + (newsize-1)*sizeof(melem_t*)];
    st->count = count;
    st->hashsize = newsize;
    for (int i = 0; i < newsize; i++)
        st->tab[i] = 0;
    for (int i = 0; i < hashsize; i++) {
        melem_t *en;
        for (melem_t *e = tab[i]; e; e = en) {
            en = e->next;
            int j = ((unsigned long)e->key) % newsize;
            e->next = st->tab[j];
            st->tab[j] = e;
        }
        tab[i] = 0;
    }
    delete this;
    return (st);
}
// End of table definitions


// Start the memory monitor.  The depth is the number of stack
// backtrace entries to save with each allocation record.  This
// frees the allocation tables from any previous run.
//
bool
sMemory::mon_start(int depth)
{
    if (!mem_mon_on) {
        if (depth < 1)
            depth = 1;
        else if (depth > 15)
            depth = 15;
        mem_mon_depth = depth;
        delete mem_mon_tab;
        mem_mon_tab = new mtable_t;
        mem_mon_enable(true);
    }
    return (true);
}


// Suspend memory allocation logging.
//
bool
sMemory::mon_stop()
{
    if (!mem_mon_on)
        return (false);
    mem_mon_enable(false);
    return (true);
}


// Return the number of allocation records in the table, i.e., the
// number of blocks allocated but not freed.
//
int
sMemory::mon_count()
{
    return (mem_mon_tab ? (int)mem_mon_tab->count : -1);
}


namespace {
    // Sorting function for allocation records, sort by caller
    //
    int
    mcomp(const void *a, const void *b)
    {
        melem_t *e1 = *(melem_t**)a;
        melem_t *e2 = *(melem_t**)b;
        unsigned long n1 = (unsigned long)e1->data[0];
        unsigned long n2 = (unsigned long)e2->data[0];
        if (n1 > n2)
            return (1);
        if (n2 > n1)
            return (-1);
        return (0);
    }
}


// Dump the records to file fname.
//
bool
sMemory::mon_dump(const char *fname)
{
    bool tmon = mem_mon_on;
    mem_mon_on = false;
    if (mem_mon_tab->count <= 0) {
        FILE *fp = fopen(fname, "w");
        if (fp) {
            fprintf(fp, "No allocation records in table.\n");
            fclose(fp);
        }
        mem_mon_on = tmon;
        return (fp ? true : false);
    }

    melem_t **bf = new melem_t*[mem_mon_tab->count];
    int cnt = 0;
    for (int i = 0; i < mem_mon_tab->hashsize; i++) {
        for (melem_t *e = mem_mon_tab->tab[i]; e; e = e->next)
            bf[cnt++] = e;
    }
    if (cnt > 1)
        qsort(bf, cnt, sizeof(melem_t*), mcomp);

    FILE *fp = fopen(fname, "w");
    if (fp) {
        for (int i = 0; i < cnt; i++) {
            melem_t *e = bf[i];
            if (mem_mon_depth == 1)
                fprintf(fp, "chunk=0x%lx  caller=0x%lx\n",
                    (unsigned long)e->key,
                    (unsigned long)e->data[0]);
            else {
                fprintf(fp, "chunk=0x%lx  caller=", (unsigned long)e->key);
                for (int j = 0; j < mem_mon_depth; j++) {
                    fprintf(fp, "0x%lx", (long)e->data[j]);
                    if (j == mem_mon_depth - 1)
                        fputc('\n', fp);
                    else
                        fputc(',', fp);
                }
#ifdef __linux
                // The backtrace_symbols call seems to return an array
                // of null strings in Red Hat EL5, i686, x86_64 is ok.
#ifdef __x86_64
                char **strings = backtrace_symbols(e->data, mem_mon_depth);
                for (int k = 0; k < mem_mon_depth; k++) {
                    fprintf(fp, "%s\n", strings[k]);
                }
                free(strings);
#endif
#endif
            }
        }
        fclose(fp);
    }

    delete [] bf;
    mem_mon_on = tmon;
    return (fp ? true : false);
}


void
sMemory::mem_mon_enable(bool enable)
{
    if (enable) {
        mem_mon_on = true;
    }
    else {
        mem_mon_on = false;
        mem_mon_check_free = false;
    }
}


#ifdef __x86_64
#else
extern "C" { int main(int, char**); }
#endif

// Private work function.
//
void
sMemory::mem_mon_alloc_hook_prv(void *v)
{
#ifdef OLD_BSD_SIGS
    int omsk = sigsetmask(sigmask(SIGALRM));
#else
    sigset_t sset, oset;;
    sigemptyset(&sset);
    sigaddset(&sset, SIGALRM);
    sigprocmask(SIG_BLOCK, &sset, &oset);
#endif
    mem_mon_on = false;
    melem_t *e = mem_mon_tab->add(v);

    for (int i = 0; i < mem_mon_depth; i++)
        e->data[i] = 0;

#ifdef __x86_64
    void *vtmp[16];
    int i = backtrace(vtmp, mem_mon_depth + 1) - 1;
    memcpy(e->data, vtmp + 1, i*sizeof(void*));
#else
    // Stack walk, gcc-specific.
    int i = 0;
    switch (1) {
    default:
        e->data[i++] = __builtin_return_address(1);
        if (i >= mem_mon_depth || e->data[i-1] <= (void*)&main) break;
        e->data[i++] = __builtin_return_address(2);
        if (i >= mem_mon_depth || e->data[i-1] <= (void*)&main) break;
        e->data[i++] = __builtin_return_address(3);
        if (i >= mem_mon_depth || e->data[i-1] <= (void*)&main) break;
        e->data[i++] = __builtin_return_address(4);
        if (i >= mem_mon_depth || e->data[i-1] <= (void*)&main) break;
        e->data[i++] = __builtin_return_address(5);
        if (i >= mem_mon_depth || e->data[i-1] <= (void*)&main) break;
        e->data[i++] = __builtin_return_address(6);
        if (i >= mem_mon_depth || e->data[i-1] <= (void*)&main) break;
        e->data[i++] = __builtin_return_address(7);
        if (i >= mem_mon_depth || e->data[i-1] <= (void*)&main) break;
        e->data[i++] = __builtin_return_address(8);
        if (i >= mem_mon_depth || e->data[i-1] <= (void*)&main) break;
        e->data[i++] = __builtin_return_address(9);
        if (i >= mem_mon_depth || e->data[i-1] <= (void*)&main) break;
        e->data[i++] = __builtin_return_address(10);
        if (i >= mem_mon_depth || e->data[i-1] <= (void*)&main) break;
        e->data[i++] = __builtin_return_address(11);
        if (i >= mem_mon_depth || e->data[i-1] <= (void*)&main) break;
        e->data[i++] = __builtin_return_address(12);
        if (i >= mem_mon_depth || e->data[i-1] <= (void*)&main) break;
        e->data[i++] = __builtin_return_address(13);
        if (i >= mem_mon_depth || e->data[i-1] <= (void*)&main) break;
        e->data[i++] = __builtin_return_address(14);
        if (i >= mem_mon_depth || e->data[i-1] <= (void*)&main) break;
        e->data[i++] = __builtin_return_address(15);
    }
#endif
    mem_mon_tab = mem_mon_tab->check_rehash();
    mem_mon_on = true;
#ifdef OLD_BSD_SIGS
    sigsetmask(omsk);
#else
    sigprocmask(SIG_SETMASK, &oset, 0);
#endif
}


// Private work function.
//
void
sMemory::mem_mon_free_hook_prv(void *v)
{
#ifdef OLD_BSD_SIGS
    int omsk = sigsetmask(sigmask(SIGALRM));
#else
    sigset_t sset, oset;;
    sigemptyset(&sset);
    sigaddset(&sset, SIGALRM);
    sigprocmask(SIG_BLOCK, &sset, &oset);
#endif
    mem_mon_on = false;
    melem_t *e = mem_mon_tab->remove(v);
    if (!e && mem_mon_check_free) {
        fprintf(stderr, "MMON ERROR: delete, %lx not allocated.\n",
            (unsigned long)v);
    }
    delete e;
    mem_mon_on = true;
#ifdef OLD_BSD_SIGS
    sigsetmask(omsk);
#else
    sigprocmask(SIG_SETMASK, &oset, 0);
#endif
}


#ifdef CPP_ONLY

void *
operator new(size_t size)
{
    void *v = malloc(size);
    if (mem_mon_on && v)
        mem_mon_alloc_hook(v);
    return (v);
}


void *
operator new[](size_t size)
{
    void *v = malloc(size);
    if (mem_mon_on && v)
        mem_mon_alloc_hook(v);
    return (v);
}


void
operator delete(void *v)
{
    if (v) {
        if (mem_mon_on)
            mem_mon_free_hook(v);
        free(v);
    }
}


void
operator delete[](void *v)
{
    if (v) {
        if (mem_mon_on)
            mem_mon_free_hook(v);
        free(v);
    }
}

#endif

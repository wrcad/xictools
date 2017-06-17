
/*========================================================================*
 *                                                                        *
 *  Whiteley Research Inc, Sunnyvale, CA USA 2016, http://wrcad.com       *
 *                                                                        *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,      *
 *   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES      *
 *   OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-        *
 *   INFRINGEMENT.  IN NO EVENT SHALL STEPHEN R. WHITELEY OR WHITELEY     *
 *   RESEARCH INC. BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,   *
 *   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,   *
 *   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER        *
 *   DEALINGS IN THE SOFTWARE.                                            *
 *                                                                        *
 *   Licensed under the Apache License, Version 2.0 (the "License");      *
 *   you may not use this file except in compliance with the License.     *
 *   You may obtain a copy of the License at                              *
 *                                                                        *
 *        http://www.apache.org/licenses/LICENSE-2.0                      *
 *                                                                        *
 *   Unless required by applicable law or agreed to in writing, software  *
 *   distributed under the License is distributed on an "AS IS" BASIS,    *
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or      *
 *   implied. See the License for the specific language governing         *
 *   permissions and limitations under the License.                       *
 *                                                                        *
 *========================================================================*
 *                                                                        *
 * LEF/DEF Database and Maze Router.                                      *
 *                                                                        *
 * Stephen R. Whiteley (stevew@wrcad.com)                                 *
 * Whiteley Research Inc. (wrcad.com)                                     *
 *                                                                        *
 * Portions adapted from Qrouter by Tim Edwards,                          *
 * (www.opencircuitdesign.com) which used code by Steve Beccue.           *
 * See original headers where applicable.                                 *
 *                                                                        *
 *========================================================================*
 $Id: ld_hash.cc,v 1.1 2017/02/07 04:19:44 stevew Exp $
 *========================================================================*/

#include "lddb_prv.h"
#include "ld_hash.h"


//
// ld_hash.cc
//
// Hash table implementation for LDDB.
//
//
// The hash table is specialized to our needs in LDDB:
// 1.  The tab strings are not copied, they must remain valid in the
//     application as long as the table is in use.
// 2.  The payload is an integer (which will be an array index).
//     This is a long to make use of space with 8-byte alignment.

// Table growth threshhold
#define ST_MAX_DENS     5

//#define DEBUG_HT


// Constructor
//
// Args:
//   cinsens    Sets case-insensitive when true;
//   numelts    The initial number of elements expected, hard limit but
//              table can be expanded.
//
dbHtab::dbHtab(bool cinsens, int numents)
{
    tNumAllocated = 0;

    int hashwidth = 2;
    while ((1 << (hashwidth + 1)) < numents)
        hashwidth++;
    tMask = ~((u_int)-1 << hashwidth);
#ifdef DEBUG_HT
    printf("HT NEW TAB %d %d %x\n", numents, tMask+1, tMask);
#endif

    tEnt = new dbHtabEnt*[tMask + 1];
    for (unsigned int i = 0; i <= tMask; i++)
        tEnt[i] = 0;
    tFlags = 0;
    if (cinsens)
        tFlags |= ST_CASE_INSENS;
    tSize = numents;
    tPool = new dbHtabEnt[numents + 1];

    // The first pool element is used as a link for possible expansion.
    tPool->stNext = 0;
    tPool->stTag = 0;
    tPool->stData = numents;
}


dbHtab::~dbHtab()
{
    delete [] tEnt;
    while (tPool) {
        dbHtabEnt *x = tPool;
        tPool = tPool->stNext;
        delete [] x;
    }
}


// Increase the available entries in the table by increment.
//
void
dbHtab::incsize(int increment)
{
    if (increment < 1)
        return;
    tSize += increment;
    rehash();

    // Link just behind existing pool.
    dbHtabEnt *h = new dbHtabEnt[increment + 1];
    h->stNext = tPool->stNext;
    h->stTag = 0;
    h->stData = increment;
    tPool->stNext = h;
}


// Add the data to the symbol table, keyed by character string tag. 
// Return false if we're out of elements.  Note that we don't check
// for duplicate strings.
//
bool
dbHtab::add(const char *tag, unsigned long data)
{
    unsigned int i = str_hash(tag, tMask);
    dbHtabEnt *h = newent();
    if (!h)
        return (false);
    h->stTag = tag;
    h->stData = data;
    h->stNext = tEnt[i];
    tEnt[i] = h;
    tNumAllocated++;
#ifdef DEBUG_HT
    printf("HT ADD %s %d %d\n", tag, i, tNumAllocated);
#endif
    return (true);
}


// Return the data keyed by string tag.  If not found,
// return the value ST_NIL.
//
unsigned long
dbHtab::get(const char *tag)
{
    u_int i = str_hash(tag, tMask);
    for (dbHtabEnt *h = tEnt[i]; h; h = h->stNext) {
        if (str_comp(tag, h->stTag))
            return (h->stData);
    }
    return (ST_NIL);
}


// Return a fresh dbHtabEnt*, or 0 if we are out of allocations.
//
dbHtabEnt *
dbHtab::newent()
{
    if (tPool->stData == 0) {
        // Pool is empty, move it to end of list if a full one follows.
        if (!tPool->stNext || tPool->stNext->stData == 0) {
#ifdef DEBUG_HT
            printf("HT ERROR newent failed!\n");
#endif
            return (0);
        }
        dbHtabEnt *h = tPool;
        tPool = tPool->stNext;
        h->stNext = 0;
        dbHtabEnt *e = tPool;
        while (e->stNext)
            e = e->stNext;
        e->stNext = h;
    }
    return (tPool + tPool->stData--);
}


// Grow the hash table width if necessary.
//
void
dbHtab::rehash()
{
    int hashwidth = 2;
    while ((u_int)(1 << (hashwidth + 1)) < tSize)
        hashwidth++;
    u_int nmask = ~((u_int)-1 << hashwidth);
    if (nmask == tMask)
        return;
#ifdef DEBUG_HT
    printf("HT REHASH to %x from %x\n", nmask, tMask);
#endif
    unsigned int oldmask = tMask;
    tMask = nmask;

    dbHtabEnt **oldent = tEnt;
    tEnt = new dbHtabEnt*[tMask + 1];
    for (u_int i = 0; i <= tMask; i++)
        tEnt[i] = 0;
    for (u_int i = 0; i <= oldmask; i++) {
        dbHtabEnt *hn;
        for (dbHtabEnt *h = oldent[i]; h; h = hn) {
            hn = h->stNext;
            u_int j = str_hash(h->stTag, tMask);
            h->stNext = tEnt[j];
            tEnt[j] = h;
        }
    }
    delete [] oldent;
}
// End of dbHtab functions


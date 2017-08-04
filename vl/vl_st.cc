
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
 * vl -- Verilog Simulator and Verilog support library.                   *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#include <string.h>
#include "vl_st.h"

extern char *vl_strdup(const char*);

#define ST_DEFAULT_MAX_DENSITY 5
#define ST_DEFAULT_INIT_TABLE_SIZE 11
#define ST_DEFAULT_GROW_FACTOR 2.0
#define ST_DEFAULT_REORDER_FLAG 0


inline int
do_hash(const char *string, int modulus)
{
    int c, val = 0;
    while ((c = *string++) != '\0')
        val = val*997 + c;
    return ((val < 0) ? -val : val)%modulus;
}


st_table::st_table(int sz, int dens, double gf, int rf)
{
    num_entries = 0;
    max_density = dens;
    grow_factor = gf;
    reorder_flag = rf;
    if (sz <= 0)
        sz = 1;
    num_bins = sz;
    bins = new st_entry*[num_bins];
    for (int i = 0; i < num_bins; i++)
        bins[i] = 0;
}


st_table::st_table()
{
    num_entries = 0;
    max_density = ST_DEFAULT_MAX_DENSITY;
    grow_factor = ST_DEFAULT_GROW_FACTOR;
    reorder_flag = ST_DEFAULT_REORDER_FLAG;
    num_bins = ST_DEFAULT_INIT_TABLE_SIZE;
    bins = new st_entry*[num_bins];
    for (int i = 0; i < num_bins; i++)
        bins[i] = 0;
}


st_table::~st_table()
{
    for (int i = 0; i < num_bins ; i++) {
        st_entry *ptr = bins[i];
        while (ptr) {
            st_entry *next = ptr->next;
            delete ptr;
            ptr = next;
        }
    }
    delete [] bins;
}


bool
st_table::lookup(const char *key, void *value)
{
    int hash_val = do_hash(key, num_bins);
    st_entry **last = &bins[hash_val];
    st_entry *ptr = *last;
    while (ptr && strcmp(ptr->key, key)) {
        last = &ptr->next;
        ptr = *last;
    }
    if (ptr && reorder_flag) {
        *last = ptr->next;
        ptr->next = bins[hash_val];
        bins[hash_val] = ptr;
    }
    if (!ptr)
        return (false);
    else {
        if (value)
            *(void**)value = ptr->record; 
        return (true);
    }
}


bool
st_table::insert(const char *key, void *value)
{
    int hash_val = do_hash(key, num_bins);
    st_entry **last = &bins[hash_val];
    st_entry *ptr = *last;
    while (ptr && strcmp(ptr->key, key)) {
        last = &ptr->next;
        ptr = *last;
    }
    if (ptr && reorder_flag) {
        *last = ptr->next;
        ptr->next = bins[hash_val];
        bins[hash_val] = ptr;
    }
    if (!ptr) {
        if (num_entries/num_bins >= max_density) {
            rehash();
            hash_val = do_hash(key, num_bins);
        }
        st_entry *enew = new st_entry;
        enew->key = key;
        enew->record = value;
        enew->next = bins[hash_val];
        bins[hash_val] = enew;
        num_entries++;
        return (false);
    }
    else {
        ptr->record = value;
        return (true);
    }
}


bool
st_table::remove(const char **keyp, void *value)
{
    const char *key = *keyp;
    int hash_val = do_hash(key, num_bins);
    st_entry **last = &bins[hash_val];
    st_entry *ptr = *last;
    while (ptr && strcmp(ptr->key, key)) {
        last = &ptr->next;
        ptr = *last;
    }
    if (ptr && reorder_flag) {
        *last = ptr->next;
        ptr->next = bins[hash_val];
        bins[hash_val] = ptr;
    }
    if (!ptr)
        return (false);

    *last = ptr->next;
    if (value)
        *(void**)value = ptr->record;
    *keyp = ptr->key;
    delete ptr;
    num_entries--;
    return (true);
}


void
st_table::rehash()
{
    st_entry **old_bins = bins;
    int old_num_bins = num_bins;

    num_bins = (int)(grow_factor*old_num_bins);
    if (num_bins%2 == 0)
        num_bins++;
    
    num_entries = 0;
    bins = new st_entry*[num_bins];
    for (int i = 0; i < num_bins; i++)
        bins[i] = 0;

    for (int i = 0; i < old_num_bins ; i++) {
        st_entry *ptr = old_bins[i];
        while (ptr) {
            st_entry *next = ptr->next;
            int hash_val = do_hash(ptr->key, num_bins);
            ptr->next = bins[hash_val];
            bins[hash_val] = ptr;
            num_entries++;
            ptr = next;
        }
    }
    delete [] old_bins;
}


st_generator::st_generator(st_table *t)
{
    table = t;
    entry = 0;
    index = 0;
}


bool 
st_generator::next(const char **key_p, void *value_p)
{
    if (!entry) {
        // try to find next entry
        if (table) {
            for (int i = index; i < table->num_bins; i++) {
                if (table->bins[i]) {
                    index = i+1;
                    entry = table->bins[i];
                    break;
                }
            }
        }
        if (!entry)
            return (false);  // done
    }
    *key_p = entry->key;
    if (value_p)
        *(void**)value_p = entry->record;
    entry = entry->next;
    return (true);
}


//
// The set manipulation functions, used to be defined in set.cc
//

void
st_table::set_destroy()
{

    st_generator gen(this);
    const char *key;
    void *data;
    while (gen.next(&key, &data)) {
        if (key != (char*)data)
            delete [] key;
        delete [] (char*)data;
    }
    delete [] bins;
    delete [] (char*)this;
}


set_t
st_table::set_add(const char *a)
{
    void *dummy;
    if (!lookup(a, &dummy))
        insert(vl_strdup(a), vl_strdup(a));
    return (this);
}


set_t
st_table::set_eliminate(const char *a)
{
    void *dummy;
    if (lookup(a, &dummy)) {
        const char *key = a;
        remove(&key, &dummy);
        delete [] key;
        delete [] (char*)dummy;
    }
    return (this);
}


bool
st_table::set_find(const char *a)
{
    void *dummy;
    return (lookup(a, &dummy));
}


set_t
st_table::set_union(set_t s2)
{

    set_t retval = new st_table;
    st_generator gen(this);
    const char *key;
    void *data;
    while (gen.next(&key, &data))
        retval->insert(vl_strdup(key), vl_strdup((char*)data));
    gen = st_generator(s2);
    while (gen.next(&key, &data)) {
        void *dummy;
        if (!lookup(key, &dummy))
            retval->insert(vl_strdup(key), vl_strdup((char*)data));
    }
    return (retval);
}


set_t
st_table::set_intersect(set_t s2)
{
    set_t retval = new st_table;
    st_generator gen(this);
    const char *key;
    void *data;
    while (gen.next(&key, &data)) {
        void *dummy;
        if (s2->lookup(key, &dummy))
            retval->insert(vl_strdup(key), vl_strdup(key));
    }
    return (retval);
}


set_t
st_table::set_dup()
{
    set_t retval = new st_table;
    st_generator gen(this);
    const char *key;
    void *data;
    while (gen.next(&key, &data))
        retval->insert(vl_strdup(key), vl_strdup((char*)data));
    return (retval);
}


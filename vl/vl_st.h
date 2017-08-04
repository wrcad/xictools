
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

#define set_empty() new st_table

struct st_entry
{
    const char *key;
    void *record;
    st_entry *next;
};

class st_table
{
public:
    st_table();
    st_table(int, int, double, int);
    ~st_table();
    bool lookup(const char*, void*);
    bool insert(const char*, void*);
    bool remove(const char**, void*);
    int count() { return (num_entries); }
    friend class st_generator;

    void set_destroy();
    st_table *set_add(const char*);
    st_table *set_eliminate(const char*);
    bool set_find(const char*);
    st_table *set_union(st_table*);
    st_table *set_intersect(st_table*);
    st_table *set_dup();

private:
    void rehash();

    int num_bins;
    int num_entries;
    int max_density;
    int reorder_flag;
    double grow_factor;
    st_entry **bins;
};
typedef st_table *set_t;

class st_generator
{
public:
    st_generator() { }
    st_generator(st_table*);
    bool next(const char**, void*);

protected:
    st_table *table;
    st_entry *entry;
    int index;
};



template<class T> class table_gen : public st_generator
{
public:
    table_gen(void *t) { table = (st_table*)t; entry = 0; index = 0; }
    bool next(const char **sp, T *d)
        { return (st_generator::next(sp, (void*)d)); }
};


template<class T> class table : public st_table
{
public:
    bool lookup(const char *s, T *dp)
        { return (st_table::lookup(s, (void*)dp)); }
    bool insert(const char *s, T d)
        { return (st_table::insert(s, (void*)d)); }
    bool remove(const char **sp, T *dp)
        { return (st_table::remove(sp, (void*)dp)); }
    friend class table_gen<T>;
};


template<class T> void
delete_table(table<T> *tab)
{
    if (!tab)
        return;
    table_gen<T> gen(tab);
    T item;
    const char *key;
    while (gen.next(&key, &item))
        delete item;
    delete tab;
}


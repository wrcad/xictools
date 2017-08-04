
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

#ifndef CD_SORTER_H
#define CD_SORTER_H

#include "randval.h"
#include <algorithm>

//
// A class to encapsulate various sorting algorithms to make use of
// modality in OASIS output.
//

template <class T>
class sorter
{
public:
    sorter(T **a, unsigned int sz, bool qo)
        {
            array = a;
            diffs = 0;
            asize = sz;
            quicksort_only = qo;
        }

    ~sorter() { delete [] diffs; }

    void sort()
        {
            if (quicksort_only || asize > 10000)
                quick_sort();
            else
                search_sort();
        }


    static inline bool
    cmp(const T *t1, const T *t2)
        {
            return (*t2 < *t1);
        }

    void quick_sort() { std::sort(array, array + asize, sorter<T>::cmp); }
    void search_sort();

    void set_diffs()
        {
            if (!diffs)
                diffs = new unsigned int[asize+1];
            for (unsigned int i = 0; i < asize; i++) {
                if (i)
                    diffs[i] = diff(array[i-1], array[i]);
                else
                    diffs[i] = 0;
            }
            diffs[asize] = 0;
        }

    unsigned int total_cost()
        {
            unsigned int cost = 0;
            for (unsigned int i = 1; i < asize; i++)
                cost += diffs[i];
            return (cost);
        }

    unsigned int size() { return (asize); }

private:

    int cost_of_swap(unsigned int, unsigned int);
    void swap(unsigned int, unsigned int);

    unsigned int diff(const T *a, const T *b) { return (T::diff(a, b)); }

    T **array;
    unsigned int *diffs;
    unsigned int asize;
    bool quicksort_only;
};


template <class T>
void
sorter<T>::search_sort()
{
    unsigned int last = 0;
    unsigned int min = 0xffffffff;
    for (unsigned int i = 0; i < asize; i++) {
        unsigned df = diff(array[i], 0);
        if (df < min) {
            min = df;
            last = i;
        }
    }
    if (last) {
        T *tmp = array[0];
        array[0] = array[last];
        array[last] = tmp;
    }

    for (unsigned int i = 0; i < asize; i++) {
        last = 0;
        min = 0xffffffff;
        for (unsigned int j = i+1; j < asize; j++) {
            unsigned df = diff(array[i], array[j]);
            if (df < min) {
                min = df;
                last = j;
            }
        }
        if (last > i+1) {
            T *tmp = array[i+1];
            array[i+1] = array[last];
            array[last] = tmp;
        }
    }

    // This can save a few bytes, but does not seem worth the expense.
    /*
    set_diffs();
    for (unsigned int i = 1; i < asize; i++) {
        for (unsigned int j = i+1; j < asize; j++) {
            if (cost_of_swap(i, j) < 0)
                swap(i, j);
        }
    }
    */
}


template <class T>
int
sorter<T>::cost_of_swap(unsigned int i, unsigned int j)
{
    if (abs((int)i - (int)j) == 1) {
        if (j < i) {
            unsigned int tmp = j;
            j = i;
            i = tmp;
        }
        int a1 = diffs[i] + diffs[j] + diffs[j+1];
        int a2 = 0;
        if (j < asize - 1)
            a2 += diff(array[j+1], array[i]);
        a2 += diff(array[j], array[i]);
        if (i)
            a2 += diff(array[j], array[i-1]);
        return (a2 - a1);
    }
    int a1 = diffs[i] + diffs[i+1] + diffs[j] + diffs[j+1];

    int a2 = 0;
    if (i)
        a2 += diff(array[j], array[i-1]);
    if (i < asize - 1)
        a2 += diff(array[i+1], array[j]);
    if (j)
        a2 += diff(array[i], array[j-1]);
    if (j < asize - 1)
        a2 += diff(array[j+1], array[i]);
    return (a2 - a1);
}


template <class T>
void
sorter<T>::swap(unsigned int i, unsigned int j)
{
    T *tmp = array[i];
    array[i] = array[j];
    array[j] = tmp;

    if (abs((int)i - (int)j) == 1) {
        if (j < i) {
            unsigned int itmp = j;
            j = i;
            i = itmp;
        }
        if (i)
            diffs[i] = diff(array[i], array[i-1]);
        diffs[j] = diff(array[j], array[i]);
        if (j < asize - 1)
            diffs[j+1] = diff(array[j+1], array[j]);
    }
    else {
        if (i)
            diffs[i] = diff(array[i], array[i-1]);
        if (i < asize - 1)
            diffs[i+1] = diff(array[i+1], array[i]);
        if (j)
            diffs[j] = diff(array[j], array[j-1]);
        if (j < asize - 1)
            diffs[j+1] = diff(array[j+1], array[j]);
    }
}

#endif


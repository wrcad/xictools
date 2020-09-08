
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
 * WRspice Circuit Simulation and Analysis Tool                           *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#ifndef STAB_H
#define STAB_H


//
// The sTab struct is defined in circuit.h.  These are the generic
// implementation functions, this must be included where there are
// called.
//

// A string hashing function (Bernstein, comp.lang.c).
//
template <class T>
unsigned int
sTab<T>::hash(const char *str) const
{
    if (!t_hashmask || !str)
        return (0);
    unsigned int hval = 5381;
    if (t_ci) {
        for ( ; *str; str++) {
            unsigned char c = isupper(*str) ? tolower(*str) : *str;
            hval = ((hval << 5) + hval) ^ c;
        }
    }
    else {
        for ( ; *str; str++)
            hval = ((hval << 5) + hval) ^ *(unsigned char*)str;
    }
    return (hval & t_hashmask);
}


// String comparison function.
//
template <class T>
int
sTab<T>::comp(const char *s, const char *t) const
{
    if (t_ci) {
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


template <class T>
T *
sTab<T>::find(const char *name) const
{
    if (name == 0)
        return (0);
    unsigned int n = hash(name);
    for (T *t = t_tab[n]; t; t = t->next()) {
        int i = comp(name, t->name());
        if (i < 0)
            continue;
        if (i == 0)
            return (t);
        break;
    }
    return (0);
}


template <class T>
void
sTab<T>::add(T *ent)
{
    unsigned int n = hash(ent->name());
    if (t_tab[n] == 0) {
        t_tab[n] = ent;
        ent->set_next(0);
        t_allocated++;
        check_rehash();
        return;
    }

    T *prv = 0;
    for (T *t = t_tab[n]; t; prv = t, t = t->next()) {
        int i = comp(ent->name(), t->name());
        if (i < 0)
            continue;
        ent->set_next(t);
        if (prv)
            prv->set_next(ent);
        else
            t_tab[n] = ent;
        t_allocated++;
        check_rehash();
        return;
    }
    prv->set_next(ent);
    t_allocated++;
    check_rehash();
}


template <class T>
void
sTab<T>::remove(const char *name)
{
    if (name == 0)
        return;
    unsigned int n = hash(name);
    T *t = t_tab[n];
    for (T *tprv = 0; t; tprv = t, t = t->next()) {
        int i = comp(name, t->name());
        if (i < 0)
            continue;
        if (i == 0) {
            if (tprv)
                tprv->set_next(t->next());
            else
                t_tab[n] = t->next();
            delete t;
            t_allocated--;
            return;
        }
        break;
    }
}


#define STAB_MAX_DENS   5

template <class T>
void
sTab<T>::check_rehash()
{
    if (t_allocated/(t_hashmask + 1) > STAB_MAX_DENS) {
        unsigned int oldmask = t_hashmask;
        t_hashmask = (oldmask << 1) | 1;
        T **oldent = t_tab;
        t_tab = new T*[t_hashmask + 1];
        for (unsigned int i = 0; i <= t_hashmask; i++)
            t_tab[i] = 0;
        for (unsigned int i = 0; i <= oldmask; i++) {
            T *tn;
            for (T *t = oldent[i]; t; t = tn) {
                tn = t->next();
                t->set_next(0);
                unsigned int n = hash(t->name());

                T *tx = t_tab[n];
                if (tx == 0) {
                    t_tab[n] = t;
                    continue;
                }

                T *tprv = 0;
                for ( ; tx; tprv = tx, tx = tx->next()) {
                    if (comp(t->name(), tx->name()) <= 0)
                        continue;
                    if (tprv) {
                        tprv->set_next(t);
                        tprv = tprv->next();
                    }
                    else {
                        tprv = t;
                        t_tab[n] = tprv;
                    }
                    tprv->set_next(tx);
                    tprv = 0;
                    break;
                }
                if (tprv)
                    tprv->set_next(t);
            }
        }
        delete [] oldent;
    }
}
// End of sTab<> functions.

#endif


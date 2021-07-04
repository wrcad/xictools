
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

template <class T> class lsList;
template <class T> class lsGen;

template <class T> class lsElem
{
public:
    lsElem(T d, lsElem<T> *p, lsElem<T> *n) { data = d; prev = p; next = n; }
    friend class lsList<T>;
    friend class lsGen<T>;

private:
    T data;
    lsElem<T> *next;
    lsElem<T> *prev;
};

template <class T> class lsList
{
public:
    friend class lsGen<T>;

    lsList(bool z=false) { first = last = 0; zok = z; }
    ~lsList()
        { lsElem<T> *f = first;
        while (f) { lsElem<T> *tmp = f->next; delete f; f = tmp; } }
    bool del(T);
    bool replace(T, T);
    void newEnd(T);
    void newBeg(T);
    void firstItem(T *d) { *d = first ? first->data : 0; } 
    void lastItem(T *d) { *d = last ? last->data : 0; }
    void delEnd(T *);
    void next() { first = first->next; }
    void clear() { first = last = 0; }
    int length();

private:
    lsElem<T> *last;
    lsElem<T> *first;
    bool zok;
};

template <class T> class lsGen
{
public:
    lsGen(lsList<T> *l = 0, bool rev = false)
        { ptr = l ? (rev ? l->last : l->first) : 0; }
    bool next(T*);
    bool prev(T*);
    void reset(lsList<T> *l, bool rev = false)
        { ptr = l ? (rev ? l->last : l->first) : 0; }

private:
    lsElem<T> *ptr;
};


template<class T> inline bool
lsList<T>::del(T t) 
{
    for (lsElem<T> *f = first; f; f = f->next) {
        if (f->data == t) {
            if (f->prev)
                f->prev->next = f->next;
            else
                first = f->next;
            if (f->next)
                f->next->prev = f->prev;
            else
                last = f->prev;
            delete f;
            return (true);
        }
    }
    return (false);
}


template<class T> inline bool
lsList<T>::replace(T oldt, T newt)
{
    if (newt || this->zok) {
        for (lsElem<T> *f = first; f; f = f->next) {
            if (f->data == oldt) {
                f->data = newt;
                return (true);
            }
        }
    }
    return (false);
}


template<class T> inline void
lsList<T>::newEnd(T data)
{
    if (data || this->zok) {
        lsElem<T> *n = new lsElem<T>(data, last, 0);
        if (last)
            last->next = n;
        last = n;
        if (!first)
            first = n;
    }
}


template<class T> inline void
lsList<T>::newBeg(T data)
{
    if (data || this->zok) {
        lsElem<T> *n = new lsElem<T>(data, 0, first);
        if (first)
            first->prev = n;
        first = n;
        if (!last)
            last = n;
    }
}


template<class T> inline void
lsList<T>::delEnd(T *d) {
    if (last) {
        *d = last->data;
        lsElem<T> *tmp = last;
        if (first == last)
            first = last = 0;
        else
            last = last->prev;
        if (last)
            last->next = 0;
        delete tmp;
    }
    else
        *d = 0;
}


template<class T> inline int
lsList<T>::length()
{
    int len = 0;
    for (lsElem<T> *e = first; e; e = e->next, len++) ;
    return (len);
}


template<class T> inline bool
lsGen<T>::next(T *dataptr)
{
    if (!ptr)
        return (false);
    *dataptr = ptr->data;
    ptr = ptr->next;
    return (true);
}


template<class T> inline bool
lsGen<T>::prev(T *dataptr)
{
    if (!ptr)
        return (false);
    *dataptr = ptr->data;
    ptr = ptr->prev;
    return (true);
}


template<class T> class vl_stack_t : public lsList<T>
{
public:
	void push(T itm) { lsList<T>::newEnd(itm); }
	void pop(T *itmp) { lsList<T>::delEnd(itmp); }
	void top(T *itmp) { lsList<T>::lastItem(itmp); }
};


template<class T> lsList<T> *
copy_list(lsList<T> *list)
{
    if (!list)
        return (0);
    lsList<T> *retval = new lsList<T>;
    lsGen<T> gen(list);
    T stmt;
    while (gen.next(&stmt))
        retval->newEnd(chk_copy(stmt));
    return (retval);
}


template<class T> void
init_list(lsList<T> *list)
{
    if (!list)
        return;
    lsGen<T> gen(list);
    T stmt;
    while (gen.next(&stmt))
        stmt->init();
}


template<class T> void
delete_list(lsList<T> *list)
{
    if (!list)
        return;
    lsGen<T> gen(list);
    T stmt;
    while (gen.next(&stmt))
        delete stmt;
    delete list;
}


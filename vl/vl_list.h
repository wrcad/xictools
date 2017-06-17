
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
 * This software is available for non-commercial use under the terms of   *
 * the GNU General Public License as published by the Free Software       *
 * Foundation; either version 2 of the License, or (at your option) any   *
 * later version.                                                         *
 *                                                                        *
 * A commercial license is available to those wishing to use this         *
 * library or a derivative work for commercial purposes.  Contact         *
 * Whiteley Research Inc., 456 Flora Vista Ave., Sunnyvale, CA 94086.     *
 * This provision will be enforced to the fullest extent possible under   *
 * law.                                                                   *
 *                                                                        *
 * You should have received a copy of the GNU General Public License      *
 * along with this program; if not, write to the Free Software            *
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.              *
 *========================================================================*
 *                                                                        *
 * Verilog Support Files                                                  *
 *                                                                        *
 *========================================================================*
 $Id: vl_list.h,v 1.6 2013/11/10 21:30:15 stevew Exp $
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
        retval->newEnd(stmt->copy());
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


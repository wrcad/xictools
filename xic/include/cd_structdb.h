
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

#ifndef CD_STRUCTDB_H
#define CD_STRUCTDB_H

// This defines a template class for data structures.
//
// The structure must have:
//   1) operator== specified.
//   2) a hashing function for contents.
//
// The objective is to keep exactly one copy or each different definition
// of the structure in this database.  The structure is referenced by a
// "ticket" returned from the database (also stored in structure).
//
// These are the main exports:
//
//     T *find(ticket_t)    Return a pointer to the structure corresponding
//                          to the ticket passed.
//
//     ticket_t record(T*)  Obtain a ticket for the passed structure, which
//                          is added to the database if necessary.
//     ticket_t check(T*)   Returns a ticket to a matching struct, or a null
//                          ticket if none exists.

#define STRDB_INITSZ 64
#define STRDB_FACTOR 5

template<class T> struct item_t
{
    T item;
    ticket_t link;
};

template<class T> struct sDBase
{
    sDBase();
    ~sDBase();
    ticket_t record(T*);
    ticket_t check(T*);

    T *find(ticket_t t)
    {
        if (t == NULL_TICKET || t >= DBdataEnd)
            return (0);
        return (&DBdata[t].item);
    }

    ticket_t end_ticket() { return (DBdataEnd); }

    void print_stats()
    {
        int tcnt = 0;
        for (ticket_t i = 0; i <= DBhashmask; i++) {
            int cnt = 0;
            for (item_t<T> *t = link(DBtab[i]); t; t = link(t->link))
                cnt++;
            printf("%d %d\n", i, cnt);
            tcnt += cnt;
        }
        printf("allocated: %d\n", tcnt);
    }

private:
    item_t<T> *link(ticket_t t)
    {
        if (t == NULL_TICKET || t >= DBdataEnd)
            return (0);
        return (DBdata + t);
    }

    ticket_t hash(T*);

    item_t<T> *DBdata;
    ticket_t *DBtab;
    ticket_t DBdataEnd;
    ticket_t DBdataSize;
    ticket_t DBhashmask;
};


template <class T> sDBase<T>::sDBase()
{
    DBdataSize = STRDB_INITSZ;
    DBdata = new item_t<T>[DBdataSize];
    DBdataEnd = 0;
    DBhashmask = 0;
    DBtab = new ticket_t[DBhashmask + 1];
    for (ticket_t i = 0; i <= DBhashmask; i++)
        DBtab[i] = NULL_TICKET;
}


template <class T> sDBase<T>::~sDBase()
{
    delete [] DBdata;
    delete [] DBtab;
}


template <class T> ticket_t
sDBase<T>::record(T *c)
{
    ticket_t tkt = hash(c);
    for (item_t<T> *xx = link(DBtab[tkt]); xx; xx = link(xx->link)) {
        if (*c == xx->item)
            return ((ticket_t)(xx - DBdata));
    }
    if (DBdataEnd >= DBdataSize) {
        item_t<T> *nd = new item_t<T>[DBdataSize + DBdataSize];
        memcpy(nd, DBdata, DBdataSize*sizeof(item_t<T>));
        // zero pointers for destructor
        memset(DBdata, 0, DBdataSize*sizeof(item_t<T>));
        DBdataSize += DBdataSize;
        delete [] DBdata;
        DBdata = nd;
    }
    DBdata[DBdataEnd].item = *c;
    DBdata[DBdataEnd].link = DBtab[tkt];
    DBtab[tkt] = DBdataEnd;
    DBdataEnd++;

    if (DBdataEnd/(DBhashmask+1) > STRDB_FACTOR) {

        ticket_t newmask = (DBhashmask << 1) | 1;
        ticket_t *tmp = new ticket_t[newmask + 1];
        for (ticket_t i = 0; i <= newmask; i++)
            tmp[i] = NULL_TICKET;

        ticket_t oldhmask = DBhashmask;
        DBhashmask = newmask;
        for (ticket_t i = 0; i <= oldhmask; i++) {
            item_t<T> *xn;
            for (item_t<T> *xx = link(DBtab[i]); xx; xx = xn) {
                xn = link(xx->link);
                ticket_t j = hash(&xx->item);
                xx->link = tmp[j];
                tmp[j] = (ticket_t)(xx - DBdata);
            }
        }
        delete [] DBtab;
        DBtab = tmp;
    }
    return (DBdataEnd-1);
}


template <class T> ticket_t
sDBase<T>::check(T *c)
{
    ticket_t i = hash(c);
    for (item_t<T> *xx = link(DBtab[i]); xx; xx = link(xx->link)) {
        if (*c == xx->item)
            return ((ticket_t)(xx - DBdata));
    }
    return (NULL_TICKET);
}

#endif


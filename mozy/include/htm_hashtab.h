
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
 * MOZY html help viewer files                                            *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#ifndef HTM_HASHTAB_H
#define HTM_HASHTAB_H


// Initial width - 1 (width MUST BE POWER OF 2)
#define ST_START_MASK   7

// Density threshold for resizing

#define ST_MAX_DENS 5

struct htmHashEnt
{
    htmHashEnt(const char *nm)
        {
            h_name = nm;
            h_next = 0;
        }

    virtual ~htmHashEnt()   { }

    const char *name()      const { return (h_name); }
    htmHashEnt *next()      const { return (h_next); }
    void set_next(htmHashEnt *h)  { h_next = h; }

protected:
    const char *h_name;
    htmHashEnt *h_next;
};


struct htmHashTab
{
    // A string hashing function (Bernstein, comp.lang.c)
    //
    static unsigned int string_hash(const char *str, unsigned int hashmask)
    {
        if (!hashmask || !str)
            return (0);
        unsigned int hash = 5381;
        for ( ; *str; str++)
            hash = ((hash << 5) + hash) ^ *(unsigned char*)str;
        return (hash & hashmask);
    }


    // A string comparison function that deals with nulls, return true if
    // match.
    //
    static bool str_compare(const char *s, const char *t)
    {
        if (s == t)
            return (true);
        if (!s || !t)
            return (false);
        while (*s && *t) {
            if (*s++ != *t++)
                return (false);
        }
        return (*s == *t);
    }

    // Constructor
    //
    htmHashTab()
    {
        tNumAllocated = 0;
        tEnt = new htmHashEnt*[ST_START_MASK + 1];
        for (unsigned int i = 0; i <= ST_START_MASK; i++)
            tEnt[i] = 0;
        tMask = ST_START_MASK;
    }


    virtual ~htmHashTab()
    {
        clear();
        delete [] tEnt;
    }


    void clear()
    {
        if (tNumAllocated) {
            for (unsigned int i = 0; i <= tMask; i++) {
                htmHashEnt *hn;
                for (htmHashEnt *h = tEnt[i]; h; h = hn) {
                    hn = h->next();
                    delete h;
                }
                tEnt[i] = 0;
            }
            tNumAllocated = 0;
        }
    }


    // Add the data to the hash table, keyed by character string name.
    //
    bool add(htmHashEnt *ent)
    {
        if (!ent)
            return (false);
        unsigned int i = string_hash(ent->name(), tMask);
        ent->set_next(tEnt[i]);
        tEnt[i] = ent;
        tNumAllocated++;
        if (tNumAllocated/(tMask + 1) > ST_MAX_DENS)
            rehash();
        return (true);
    }


    // Return the data keyed by string tag.  If not found,
    // return 0.
    //
    htmHashEnt *get(const char *tag)
    {
        if (!tag)
            return (0);
        unsigned int i = string_hash(tag, tMask);
        for (htmHashEnt *h = tEnt[i]; h; h = h->next()) {
            if (str_compare(tag, h->name()))
                return (h);
        }
        return (0);
    }


    // Grow the hash table width, and reinsert the entries.
    //
    void rehash()
    {
        htmHashEnt **oldent = tEnt;
        unsigned int oldmask = tMask;
        tMask = (oldmask << 1) | 1;
        tEnt = new htmHashEnt*[tMask + 1];
        for (unsigned int i = 0; i <= tMask; i++)
            tEnt[i] = 0;
        for (unsigned int i = 0; i <= oldmask; i++) {
            htmHashEnt *hn;
            for (htmHashEnt *h = oldent[i]; h; h = hn) {
                hn = h->next();
                unsigned int j = string_hash(h->name(), tMask);
                h->set_next(tEnt[j]);
                tEnt[j] = h;
            }
        }
        delete [] oldent;
    }

protected:
    htmHashEnt **tEnt;              // element list heads
    unsigned int tNumAllocated;     // element count
    unsigned int tMask;             // hashsize -1, hashsize is power 2

};

#endif


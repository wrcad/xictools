
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

#ifndef CD_SYMTAB_H
#define CD_SYMTAB_H

#include "cd_memmgr_cfg.h"


//-------------------------------------------------------------------------
// General purpose symbol table
//-------------------------------------------------------------------------

// Symbol table entry
struct SymTabEnt
{
#ifdef CD_USE_MANAGER
    void *operator new(size_t);
    void operator delete(void*, size_t);
#endif
    SymTabEnt()
        {
            stNext = 0;
            stTag = 0;
            stData = 0;
        }

    SymTabEnt(const char *t, const void *d)
        {
            stNext = 0;
            stTag = t;
            stData = d;
        }

    SymTabEnt *stNext;
    const char *stTag;
    const void *stData;
};

// Type of key: char*, int, or undefined
enum { STnone, STchar, STint };
typedef unsigned char STtype;

// "not found" return value
#define ST_NIL          (void*)(-1)

// Flags
#define ST_FREE_TAG     0x1
#define ST_FREE_DATA    0x2
#define ST_CASE_INSENS  0x4

// Initial width - 1 (width MUST BE POWER OF 2)
#define ST_START_MASK   31

struct stringlist;

// Symbol table
//
struct SymTab
{
    friend struct SymTabGen;

    SymTab(bool, bool, int = 0);
    ~SymTab();
    void clear();
    bool add(const char*, const void*, bool);
    bool add(uintptr_t, const void*, bool);
    bool replace(const char*, const void*);
    bool replace(uintptr_t, const void*);
    bool remove(const char*);
    bool remove(uintptr_t);

    static void *get(SymTab *tab, const char *tag)
        {
            if (!tab)
                return (ST_NIL);
            return (tab->get_prv(tag));
        }

    static void *get(SymTab *tab, uintptr_t itag)
        {
            if (!tab)
                return (ST_NIL);
            return (tab->get_prv(itag));
        }

    static SymTabEnt *get_ent(SymTab *tab, const char *tag)
        {
            if (!tab)
                return (0);
            return (tab->get_ent_prv(tag));
        }

    static SymTabEnt *get_ent(SymTab *tab, uintptr_t itag)
        {
            if (!tab)
                return (0);
            return (tab->get_ent_prv(itag));
        }

    static stringlist *names(SymTab *tab)
        {
            if (!tab)
                return (0);
            return (tab->names_prv());
        }

    // Default is case-sensitive.  This can be called before any
    // additions, or after calling clear, to set case-insensitivity. 
    // Obviously, applies to string tags only.
    //
    void set_case_insens(bool b)
        {
            if (!tNumAllocated) {
                if (b)
                    tFlags |= ST_CASE_INSENS;
                else
                    tFlags &= ~ST_CASE_INSENS;
            }
        }
    bool case_insens()          const { return (tFlags & ST_CASE_INSENS); }

    unsigned int allocated()    const { return (tNumAllocated); }

    // Change the free-on-destroy flag for the data, since it is
    // possible to change the data type.
    //
    void set_data_free(bool b)
        {
            if (b)
                tFlags |= ST_FREE_DATA;
            else
                tFlags &= ~ST_FREE_DATA;
        }

protected:
    void *get_prv(const char*);
    void *get_prv(uintptr_t);
    SymTabEnt *get_ent_prv(const char*);
    SymTabEnt *get_ent_prv(uintptr_t);
    stringlist *names_prv();

private:
    void rehash();

    SymTabEnt **tEnt;               // element list heads
    unsigned int tNumAllocated;     // element count
    unsigned int tMask;             // hashsize -1, hashsize is power 2
    unsigned char tFlags;
    STtype tMode;
};

// Generator, recurse through entries
//
struct SymTabGen
{
    SymTabGen(SymTab*, bool = false);
    SymTabEnt *next();

private:
    SymTab *tab;
    SymTabEnt *ent;
    unsigned int ix;
    bool remove;
};

#endif


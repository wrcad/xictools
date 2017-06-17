
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2007 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *  WHITELEY RESEARCH INCORPORATED PROPRIETARY SOFTWARE                   *
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
 *                                                                        *
 * XIC Integrated Circuit Layout and Schematic Editor                     *
 *                                                                        *
 *========================================================================*
 $Id: cd_symtab.h,v 5.39 2015/07/13 04:48:10 stevew Exp $
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
    bool add(unsigned long, const void*, bool);
    bool replace(const char*, const void*);
    bool replace(unsigned long, const void*);
    bool remove(const char*);
    bool remove(unsigned long);
    void *get(const char*);
    void *get(unsigned long);
    SymTabEnt *get_ent(const char*);
    SymTabEnt *get_ent(unsigned long);
    stringlist *names();

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


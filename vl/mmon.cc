
#include <stdlib.h>
#include <stdio.h>

//=========================================================================
//  Memory Core-Leak Debugging Tool
//=========================================================================

// Symbol table entry
struct SymTabEnt
{
    SymTabEnt() { stNext = 0; stTag = 0; stData = 0; }
    SymTabEnt(const char *t, void *d) { stNext = 0; stTag = t; stData = d; }

    SymTabEnt *stNext;
    const char *stTag;
    void *stData;
};

enum boolean { False, True };

// Type of key: char*, int, or undefined
char enum STtype { STnone, STchar, STint };

// "not found" return value
#define ST_NIL          (void*)(-1)

// Flags
#define ST_FREE_TAG     0x1
#define ST_FREE_DATA    0x2

// Initial width
#define ST_MIN_SIZE     11

// Growth threshhold
#define ST_MAX_DENS     5

// Symbol table
//
struct SymTab
{
    friend class CDstGen;

    SymTab(boolean, boolean);
    ~SymTab();
    void clear();
    boolean add(const char*, void*, boolean);
    boolean add(unsigned long, void*, boolean);
    boolean remove(const char*);
    boolean remove(unsigned long);
    void *get(const char*);
    void *get(unsigned long);
    unsigned int allocated() { return (tNumAllocated); }
private:
    void rehash();
    inline int chash(const char*);
    inline int ihash(unsigned long);

    unsigned int tNumAllocated;
    SymTabEnt **tEnt;
    unsigned short tSize;
    unsigned char tFlags;
    STtype tMode;
};

// Generator, recurse through entries
//
struct CDstGen
{
    CDstGen(SymTab*, bool = false);
    SymTabEnt *next();

private:
    SymTab *tab;
    int ix;
    SymTabEnt *ent;
    bool remove;
};

// The routines in this section implement a simple hashing symbol
// table, keyed either by a character string or an integer.
// The keying strings are NOT copied.

// Constructor
//  free_tag:  tag will be freed (as a char*) when table destroyed
//  free_data: data will be freed (as a char*) when table destroyed
//
SymTab::SymTab(boolean free_tag, boolean free_data)
{
    tNumAllocated = 0;
    tEnt = new SymTabEnt*[ST_MIN_SIZE];
    for (int i = 0; i < ST_MIN_SIZE; i++)
        tEnt[i] = 0;
    tSize = ST_MIN_SIZE;
    tFlags = 0;
    if (free_tag)
        tFlags |= ST_FREE_TAG;
    if (free_data)
        tFlags |= ST_FREE_DATA;
    tMode = STnone;
}


SymTab::~SymTab()
{
    if (this) {
        for (int i = 0; i < tSize; i++) {
            SymTabEnt *hn;
            for (SymTabEnt *h = tEnt[i]; h; h = hn) {
                hn = h->stNext;
                if (tFlags & ST_FREE_TAG)
                    delete [] h->stTag;
                if (tFlags & ST_FREE_DATA)
                    delete [] (char*)h->stData;
                delete h;
            }
        }
        delete [] tEnt;
    }
}


void
SymTab::clear()
{
    if (this) {
        if (tNumAllocated) {
            for (int i = 0; i < tSize; i++) {
                SymTabEnt *hn;
                for (SymTabEnt *h = tEnt[i]; h; h = hn) {
                    hn = h->stNext;
                    if (tFlags & ST_FREE_TAG)
                        delete [] h->stTag;
                    if (tFlags & ST_FREE_DATA)
                        delete [] (char*)h->stData;
                    delete h;
                }
                tEnt[i] = 0;
            }
            tNumAllocated = 0;
        }
        tMode = STnone;
    }
}


inline int
SymTab::chash(const char *tag)
{
    int k;
    for (k = 0; *tag; k += *tag++);
    return (k % tSize);
}


inline int
SymTab::ihash(unsigned long itag)
{
    return (itag % tSize);
}


// Add the data to the symbol table, keyed by character
// string tag.  Check for existing tag if check_unique
// is set;
//
boolean
SymTab::add(const char *tag, void *data, boolean check_unique)
{
    if (!this)
        return (False);
    if (tMode == STint)
        return (False);
    tMode = STchar;
    int i = chash(tag);
    SymTabEnt *h;
    if (check_unique) {
        for (h = tEnt[i]; h; h = h->stNext)
            if (!strcmp(tag, h->stTag))
                return (False);
    }
    h = new SymTabEnt(tag, data);
    h->stNext = tEnt[i];
    tEnt[i] = h;
    tNumAllocated++;
    if (tNumAllocated/tSize > ST_MAX_DENS)
        rehash();
    return (True);
}


// Add the data to the symbol table, keyed by integer
// itag.
//
boolean
SymTab::add(unsigned long itag, void *data, boolean check_unique)
{
    if (!this)
        return (False);
    if (tMode == STchar)
        return (False);
    tMode = STint;
    int i = ihash(itag);
    SymTabEnt *h;
    if (check_unique) {
        for (h = tEnt[i]; h; h = h->stNext)
            if (h->stTag == (char*)itag)
                return (False);
    }
    h = new SymTabEnt((char*)itag, data);
    h->stNext = tEnt[i];
    tEnt[i] = h;
    tNumAllocated++;
    if (tNumAllocated/tSize > ST_MAX_DENS)
        rehash();
    return (True);
}


// Remove and possibly delete the data item keyed by string tag from the
// database.
//
boolean
SymTab::remove(const char *tag)
{
    if (!this || !tag)
        return (False);
    if (tMode == STint)
        return (False);
    int i = chash(tag);
    SymTabEnt *hp = 0;
    for (SymTabEnt *h = tEnt[i]; h; h = h->stNext) {
        if (!strcmp(tag, h->stTag)) {
            if (!hp)
                tEnt[i] = h->stNext;
            else
                hp->stNext = h->stNext;
            if (tFlags & ST_FREE_TAG)
                delete [] h->stTag;
            if (tFlags & ST_FREE_DATA)
                delete [] (char*)h->stData;
            delete h;
            tNumAllocated--;
            break;
        }
        hp = h;
    }
    return (True);
}


// Remove and possibly delete the data item keyed by integer itag from the
// database.
//
boolean
SymTab::remove(unsigned long itag)
{
    if (!this)
        return (False);
    if (tMode == STchar)
        return (False);
    int i = ihash(itag);
    SymTabEnt *hp = 0;
    for (SymTabEnt *h = tEnt[i]; h; h = h->stNext) {
        if (h->stTag == (char*)itag ) {
            if (!hp)
                tEnt[i] = h->stNext;
            else
                hp->stNext = h->stNext;
            if (tFlags & ST_FREE_TAG)
                delete [] h->stTag;
            if (tFlags & ST_FREE_DATA)
                delete [] (char*)h->stData;
            delete h;
            tNumAllocated--;
            break;
        }
        hp = h;
    }
    return (True);
}


// Return the data keyed by string tag.  If not found,
// return the datum ST_NIL.
//
void *
SymTab::get(const char *tag)
{
    if (!this)
        return (ST_NIL);
    if (tMode != STchar)
        return (ST_NIL);
    int i = chash(tag);
    for (SymTabEnt *h = tEnt[i]; h; h = h->stNext) {
        if (!strcmp(tag, h->stTag))
            return (h->stData);
    }
    return (ST_NIL);
}


// Return the data keyed by integer itag.  If not found,
// return the datum ST_NIL.
//
void *
SymTab::get(unsigned long itag)
{
    if (!this)
        return (ST_NIL);
    if (tMode != STint)
        return (ST_NIL);
    int i = ihash(itag);
    for (SymTabEnt *h = tEnt[i]; h; h = h->stNext) {
        if (h->stTag == (char*)itag)
            return (h->stData);
    }
    return (ST_NIL);
}


// Grow the hash table width, and reinsert the entries
//
void
SymTab::rehash()
{
    SymTabEnt **oldent = tEnt;
    int oldsize = tSize;
    tSize = 2*oldsize + 1;
    tEnt = new SymTabEnt*[tSize];
    for (int i = 0; i < tSize; i++)
        tEnt[i] = 0;
    if (tMode == STchar) {
        for (int i = 0; i < oldsize; i++) {
            SymTabEnt *hn;
            for (SymTabEnt *h = oldent[i]; h; h = hn) {
                hn = h->stNext;
                int j = chash(h->stTag);
                h->stNext = tEnt[j];
                tEnt[j] = h;
            }
        }
    }
    else if (tMode == STint) {
        for (int i = 0; i < oldsize; i++) {
            SymTabEnt *hn;
            for (SymTabEnt *h = oldent[i]; h; h = hn) {
                hn = h->stNext;
                int j = ihash((unsigned long)h->stTag);
                h->stNext = tEnt[j];
                tEnt[j] = h;
            }
        }
    }
    delete [] oldent;
}
// End SymTab functions


// Generator, constructor.  If remove is true, returned elements are
// removed
//
CDstGen::CDstGen(SymTab *st, bool remove_element = false)
{
    tab = st;
    ix = 0;
    ent = st ? st->tEnt[0] : 0;
    remove = remove_element;
}


// Generator, iterator.  The returned value is a pointer to the internal
// storage, so be careful if modifying (unless remove is true, in which
// case it should be freed)
//
SymTabEnt *
CDstGen::next()
{
    if (!tab)
        return (0);
    for (;;) {
        if (!ent) {
            ix++;
            if (ix == tab->tSize) {
                if (remove) {
                    tab->tNumAllocated = 0;
                    tab->tMode = STnone;
                }
                return (0);
            }
            ent = tab->tEnt[ix];
            continue;
        }
        SymTabEnt *e = ent;
        ent = ent->stNext;
        if (remove) {
            tab->tEnt[ix] = ent;
            tab->tNumAllocated--;
        }
        return (e);
    }
}
// End of CDstGen functions


//=======================================================================
// Memory Allocation Monitor (core leak detection)
//
#define DEBUG_ALLOC

#ifdef DEBUG_ALLOC
static SymTab *mmon_tab;
static boolean mmon_on;
static int mmon_depth = 1;
#endif


bool
mmon_start(int depth)
{
#ifdef DEBUG_ALLOC
    if (!mmon_on) {
        if (depth < 1)
            depth = 1;
        else if (depth > 15)
            depth = 15;
        mmon_depth = depth;
        delete mmon_tab;
        mmon_tab = new SymTab(False, mmon_depth > 1 ? True : False);
        mmon_on = True;
    }
    return (true);
#else
    return (false);
#endif
}


bool
mmon_stop()
{
#ifdef DEBUG_ALLOC
    if (!mmon_on)
        return (false);
    mmon_on = False;
    return (true);
#else
    return (false);
#endif
}


int
mmon_count()
{
#ifdef DEBUG_ALLOC
    return (mmon_tab ? (int)mmon_tab->allocated() : -1);
#else
    return (-1);
#endif
}


bool
mmon_dump(char *fname)
{
#ifdef DEBUG_ALLOC
    boolean tmon = mmon_on;
    mmon_on = False;
    CDstGen gen(mmon_tab);
    SymTabEnt *h;
    FILE *fp = fopen(fname, "w");
    if (!fp)
        return (false);
    while ((h = gen.next()) != 0) {
        if (mmon_depth == 1)
            fprintf(fp, "chunk=0x%lx  caller=0x%x\n", (unsigned long)h->stTag,
                (unsigned int)h->stData);
        else {
            fprintf(fp, "chunk=0x%lx  caller=", (unsigned long)h->stTag);
            for (int i = 0; i < mmon_depth; i++) {
                fprintf(fp, "0x%x", ((int*)h->stData)[i]);
                if (i == mmon_depth - 1)
                    fputc('\n', fp);
                else
                    fputc(',', fp);
            }
        }
    }
    fclose(fp);
    mmon_on = tmon;
    return (true);
#else
    return (false);
#endif
}

#ifdef DEBUG_ALLOC

#define  stackwalk(x) \
    int *b = ((int*)(&x)) - 2; \
    int *stk; \
    if (mmon_depth == 1) \
        stk = (int*)b[1]; \
    else { \
        stk = (int*)malloc(mmon_depth*sizeof(int)); \
        for (int i = 0; i < mmon_depth; i++) { \
            if (b == 0) \
                stk[i] = 0; \
            else { \
                stk[i] = b[1]; \
                b = (int*)b[0]; \
            } \
        } \
    }


void *
operator new(size_t size)
{
    void *v = malloc(size);
    if (mmon_on && v) {
        stackwalk(size)
        mmon_on = False;
        mmon_tab->add((unsigned long)v, stk, False);
        mmon_on = True;
    }
    return (v);
}


void *
operator new[](size_t size)
{
    void *v = malloc(size);
    if (mmon_on && v) {
        stackwalk(size)
        mmon_on = False;
        mmon_tab->add((unsigned long)v, stk, False);
        mmon_on = True;
    }
    return (v);
}


void
operator delete(void *v)
{
    if (v) {
        if (mmon_on) {
            mmon_on = False;
            mmon_tab->remove((unsigned long)v);
            mmon_on = True;
        }
        free(v);
    }
}


void
operator delete[](void *v)
{
    if (v) {
        if (mmon_on) {
            mmon_on = False;
            mmon_tab->remove((unsigned long)v);
            mmon_on = True;
        }
        free(v);
    }
}

#endif
//=======================================================================


#ifndef HTM_HASHTABPH
#define HTM_HASHTABPH


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
        {
            htmHashTab *ht = this;
            if (!ht)
                return;
        }
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
        {
            htmHashTab *ht = this;
            if (!ht)
                return (false);
        }
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
        {
            htmHashTab *ht = this;
            if (!ht)
                return (0);
        }
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


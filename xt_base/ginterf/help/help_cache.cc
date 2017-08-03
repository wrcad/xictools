
/*========================================================================*
 *                                                                        *
 *  Whiteley Research Inc, Sunnyvale, CA USA, http://wrcad.com            *
 *  Copyright (C) 2017 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *  Licensed under the Apache License, Version 2.0 (the "License");       *
 *  you may not use this file except in compliance with the License.      *
 *  You may obtain a copy of the License at                               *
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
 * Help System Files                                                      *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#include "config.h"
#include "help_cache.h"
#include "filestat.h"
#include "pathlist.h"

#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>

// default cache size (number of saved files)
int HLPcache::cache_size = CACHE_NUMFILES;

/*=======================================================================
 =
 =  The URL Cache Functions
 =
 =======================================================================*/

HLPcache::HLPcache()
{
    dirname = 0;
    tagcnt = 0;
    nocache_ent = 0;
    entries = new HLPcacheEnt* [cache_size];
    memset(entries, 0, cache_size*sizeof(HLPcacheEnt*));
    memset(table, 0, sizeof(table));
    use_ext = true;

    set_dir();
    load();
}


// The "front end" functions get(), add(), set_complete(), and
// remove() constitute the basic interface to the application.  The
// dump()/load() functions are used to write/read the directory file
// directly, and should be used with care if called by the
// application.  The external functions (suffixed with "_ext") are
// similar, but maintain the external directory file.  These are
// called from the non-suffixed functions if "use_ext" is set, in
// which case we have an internal "local" cache, and an external cache
// with state maintained in the directory file.


// Get the entry for url from the cache, or return 0 if not found.  If
// the entry is found, the directory is consulted to make sure that it
// is still current, if use_ext is set.  Note that this function always
// fails if nocache is set.
//
HLPcacheEnt *
HLPcache::get(const char *url, bool nocache)
{
    if (!nocache) {
        int ix = tab_get(url);
        if (ix >= 0) {
            if (!entries[ix])
                // shouldn't happen
                tab_remove(url);
            else {
                if (use_ext) {
                    HLPcache *c = new HLPcache;
                    c->load();
                    HLPcacheEnt *ce = c->get_ext(url, nocache);
                    if (!ce || c->entries[ix] != ce) {
                        // Not found in ext cache, or has a different
                        // index from the ext cache, not acceptable so
                        // delete entry
                        tab_remove(url);
                        delete entries[ix];
                        entries[ix] = 0;
//                        pop_up_cache(0, MODE_UPD);
                    }
                    delete c;
                }
                return (entries[ix]);
            }
        }
    }
    return (0);
}


// Get the entry for url from the cache, or return 0 if not found.
// Note that this function always fails if nocache is set.
//
HLPcacheEnt *
HLPcache::get_ext(const char *url, bool nocache)
{
    if (!nocache) {
        int ix = tab_get(url);
        if (ix >= 0) {
            if (!entries[ix])
                // shouldn't happen
                tab_remove(url);
            else
                return (entries[ix]);
        }
    }
    return (0);
}


// Add en entry for url into the cache, and return the new entry.  The
// set_complete() function should be called once the file status is
// known.  The HLPcacheEnt pointer is returned, which provides the name
// of the file associated with this url.  If nocache is set, the
// return value points to a single struct which is used for all
// accesses.
//
HLPcacheEnt *
HLPcache::add(const char *url, bool nocache)
{
    set_dir();
    if (!nocache) {
        if (dirname) {
            int ix = tab_get(url);
            if (ix >= 0) {
                // entry exists, delete old one
                tab_remove(url);
                delete entries[ix];
                entries[ix] = 0;
            }
            if (use_ext) {
                HLPcache *c = new HLPcache;
                c->load();
                c->add_ext(url, nocache);
                tagcnt = c->tagcnt - 1;
                delete c;
            }
            const char *fn = CACHE_DIR + 1;
            ix = tagcnt % cache_size;
            char buf[256];
            sprintf(buf, "%s/%s%d", dirname, fn, ix);
            if (entries[ix]) {
                tab_remove(entries[ix]->url);
                delete entries[ix];
            }
            entries[ix] = new HLPcacheEnt(url, buf);
            tab_add(entries[ix]->url, ix);
            if (!use_ext)
                tagcnt++;
//            pop_up_cache(0, MODE_UPD);
            return (entries[ix]);
        }
    }

    // If we get here, we aren't caching.  Create a single entry with
    // a temp file to use.
    //
    if (!nocache_ent)
        nocache_ent = new HLPcacheEnt(url, 0);
    else {
        delete [] nocache_ent->url;
        nocache_ent->url = lstring::copy(url);
        delete [] nocache_ent->filename;
    }
    nocache_ent->filename = filestat::make_temp("htp");
    filestat::queue_deletion(nocache_ent->filename);
    return (nocache_ent);
}


// Add en entry for url into the cache, update the directory, and
// return the new entry.  The set_complete_ext() function should be
// called once the file status is known.  The HLPcacheEnt pointer is
// returned, which provides the name of the file associated with this
// url.  If nocache is set, the return value points to a single struct
// which is used for all accesses.
//
HLPcacheEnt *
HLPcache::add_ext(const char *url, bool nocache)
{
    set_dir();
    if (!nocache) {
        if (dirname) {
            HLPcacheEnt *ctmp = 0;
            int ix = tab_get(url);
            if (ix >= 0) {
                // old version exists, delete it and save file for
                // deletion *after* the directory is updated
                tab_remove(url);
                ctmp = entries[ix];
                entries[ix] = 0;
                int iy = tagcnt % cache_size;
                if ((ix == cache_size-1 && iy == 0) || ix == iy - 1)
                    // slot was at top, so it can be reused
                    tagcnt--;
            }
            const char *fn = CACHE_DIR + 1;
            ix = tagcnt % cache_size;
            char buf[256];
            sprintf(buf, "%s/%s%d", dirname, fn, ix);
            if (entries[ix]) {
                // slot has prior entry, delete it, no need to delete
                // file since it is done anyway
                tab_remove(entries[ix]->url);
                delete entries[ix];
            }
            entries[ix] = new HLPcacheEnt(url, buf);
            tab_add(entries[ix]->url, ix);
            tagcnt++;
            dump();

            unlink(buf);
            if (ctmp) {
                unlink(ctmp->filename);
                delete ctmp;
            }
            return (entries[ix]);
        }
    }

    // If we get here, we aren't caching.  Create a single entry with
    // a temp file to use.
    //
    if (!nocache_ent)
        nocache_ent = new HLPcacheEnt(url, 0);
    else {
        delete [] nocache_ent->url;
        nocache_ent->url = lstring::copy(url);
        delete [] nocache_ent->filename;
    }
    nocache_ent->filename = filestat::make_temp("htp");
    filestat::queue_deletion(nocache_ent->filename);
    return (nocache_ent);
}


// After calling HLPcache::add(), one must supply the file named in the
// struct returned.  When this is done (ok is true), or if it can't be
// done (ok is false) this function completes the add process
//
void
HLPcache::set_complete(HLPcacheEnt *cent, DLstatus status)
{
    if (cent) {
        DLstatus old_status = cent->get_status();
        cent->set_status(status);
        if (status == DLnogo)
            // The external cache doesn't use nogo, so we can try download
            // again after reloading external cache
            status = DLincomplete;
        if (use_ext && old_status != status) {
            HLPcache *c = new HLPcache;
            c->load();
            int ix = c->tab_get(cent->url);
            if (ix >= 0)
                c->set_complete_ext(c->entries[ix], status);
            delete c;
        }
    }
}


// After calling HLPcache::add_ext(), one must supply the file named in the
// struct returned.  When this is done (ok is true), or if it can't be
// done (ok is false) this function completes the add process, updating
// the directory
//
void
HLPcache::set_complete_ext(HLPcacheEnt *cent, DLstatus status)
{
    if (cent) {
        cent->set_status(status);
        dump();
    }
}


// Remove the entry for url from the cache, return true if the object was
// deleted.  The directory is modified only if the entry existed in the
// local cache, and use_ext is set.  If nocache is set, nothing is done.
//
bool
HLPcache::remove(const char *url, bool nocache)
{
    if (!nocache) {
        int ix = tab_get(url);
        if (ix >= 0) {
            tab_remove(url);
            if (use_ext) {
                HLPcache *c = new HLPcache;
                c->load();
                c->remove_ext(url, nocache);
                delete c;
            }
            delete entries[ix];
            entries[ix] = 0;
//            pop_up_cache(0, MODE_UPD);
            return (true);
        }
    }
    return (false);
}


// Remove the entry for url from the cache, and update the directory.
// If an entry was removed, true is returned.  This is a no-op if
// nocache is set.
//
bool
HLPcache::remove_ext(const char *url, bool nocache)
{
    if (!nocache) {
        int ix = tab_get(url);
        if (ix >= 0) {
            tab_remove(url);
            HLPcacheEnt *ctmp = entries[ix];
            entries[ix] = 0;
            dump();
            unlink(ctmp->filename);
            delete ctmp;
            return (true);
        }
    }
    return (false);
}


// Clear the cache.  This clears only the cache struct and does not touch
// the directory.
//
void
HLPcache::clear()
{
    for (int i = 0; i < CACHE_NUMHASH; i++) {
        HLPcacheTabEnt *t = table[i];
        while (t) {
            HLPcacheTabEnt *tn = t->next;
            delete t;
            t = tn;
        }
        table[i] = 0;
    }
    for (int i = 0; i < cache_size; i++) {
        delete entries[i];
        entries[i] = 0;
    }
    tagcnt = 0;
//    pop_up_cache(0, MODE_UPD);
}


// Create a file named "directory" in the cache directory, and dump a
// listing of the current cache contents.  The first line of the file
// contains the file prefix, and the current integer index (two tokens).
// Additional lines describe the entries (three tokens): the first token
// is the index, the second token is '0' if the file is ok, '1' otherwise,
// and the third token is the url.
//
// Note that calling this function overwrites the directory, so the
// previous contents should be read and merged first.
//
void
HLPcache::dump()
{
    char buf[256];
    if (dirname) {
        sprintf(buf, "%s/%s", dirname, "directory");
        FILE *fp = fopen(buf, "w");
        if (fp) {
            fprintf(fp, "%s %d\n", CACHE_DIR + 1, tagcnt % cache_size);
            for (int i = 0; i < cache_size; i++) {
                if (entries[i])
                    fprintf(fp, "%-4d %d %s\n", i,
                        entries[i]->get_status(), entries[i]->url);
            }
            fclose (fp);
        }
    }
}


// Initialize the cache from the "directory" file.  The cache is cleared
// before reading the directory.
//
void
HLPcache::load()
{
    char buf[256];
    set_dir();
    if (dirname) {
        const char *fn = CACHE_DIR + 1;
        sprintf(buf, "%s/%s", dirname, "directory");
        FILE *fp = fopen(buf, "r");
        if (fp) {
            int tc = 0;
            if (fgets(buf, 256, fp) != 0) {
                if (!lstring::prefix(fn, buf)) {
                    fclose (fp);
                    return;
                }
                if (sscanf(buf, "%*s %d", &tc) < 1 || tc < 0) {
                    fclose (fp);
                    return;
                }
                if (tc >= cache_size)
                    tc = 0;
            }
            clear();
            tagcnt = tc;
            char tbuf[256];
            sprintf(tbuf, "%s/%s", dirname, CACHE_DIR + 1);
            int fend = strlen(tbuf);
            while (fgets(buf, 256, fp) != 0) {
                char *s = buf;
                char *fnum = lstring::gettok(&s);
                char *inc = lstring::gettok(&s);
                char *url = lstring::gettok(&s);
                if (!url) {
                    delete [] fnum;
                    delete [] inc;
                    continue;
                }
                int ix;
                if (sscanf(fnum, "%d", &ix) < 1 || ix < 0 ||
                        ix >= cache_size) {
                    delete [] url;
                    delete [] inc;
                    delete [] fnum;
                    break;
                }
                strcpy(tbuf + fend, fnum);
                entries[ix] = new HLPcacheEnt(url, tbuf);
                if (*inc == '0')
                    entries[ix]->set_status(DLok);
                tab_add(entries[ix]->url, ix);
                delete [] url;
                delete [] inc;
                delete [] fnum;
            }
//                pop_up_cache(0, MODE_UPD);
            fclose(fp);
        }
    }
}


// Resize the cache size to newsize
//
void
HLPcache::resize(int newsize)
{
    if (newsize == cache_size)
        return;
    HLPcacheEnt **tmp = new HLPcacheEnt* [newsize];
    if (newsize > cache_size) {
        int i;
        for (i = 0; i < cache_size; i++)
            tmp[i] = entries[i];
        for ( ; i < newsize; i++)
            tmp[i] = 0;
        delete [] entries;
        entries = tmp;
    }
    else {
        int i;
        for (i = 0; i < newsize; i++)
            tmp[i] = entries[i];
        for ( ; i < cache_size; i++) {
            tab_remove(entries[i]->url);
            delete entries[i];
        }
        delete [] entries;
        entries = tmp;
        if (tagcnt >= newsize)
            tagcnt = 0;
    }
    cache_size = newsize;
//    pop_up_cache(0, MODE_UPD);
}


// Private function to remove the url from the hash table
//
void
HLPcache::tab_remove(const char *url)
{
    int i = hash(url);
    HLPcacheTabEnt *ctep = 0;
    for (HLPcacheTabEnt *cte = table[i]; cte; cte = cte->next) {
        if (!strcmp(url, cte->url)) {
            if (ctep)
                ctep->next = cte->next;
            else
                table[i] = cte->next;
            delete cte;
            return;
        }
        ctep = cte;
    }
}


// Private function to obtain the directory where cache files are
// stored
//
void
HLPcache::set_dir()
{
    if (!dirname) {
        char *home = pathlist::get_home(0);
        if (home) {
            char *pbuf = pathlist::mk_path(home, CACHE_DIR);
            delete [] home;
            struct stat st;
            if (stat(pbuf, &st)) {
                if (errno != ENOENT ||
#ifdef WIN32
                mkdir(pbuf)) {
#else
                mkdir(pbuf, 0755)) {
#endif
                    delete [] pbuf;
                    return;
                }
            }
            else if (!S_ISDIR(st.st_mode)) {
                delete [] pbuf;
                return;
            }
            dirname = pbuf;
        }
    }
}


// Return a string list of the current valid entries.
//
stringlist *
HLPcache::list_entries()
{
    stringlist *s0 = 0, *se = 0;
    int c1 = 2;
    if (HLPcache::cache_size > 99)
        c1 = 3;
    if (HLPcache::cache_size > 999)
        c1 = 4;
    for (int i = 0; i < cache_size; i++) {
        if (entries[i] && entries[i]->get_status() == DLok) {
            int len = strlen(entries[i]->url);
            char *buf = new char[len + 12];
            sprintf(buf, "%-*d %s", c1, i, entries[i]->url);
            stringlist *s = new stringlist(buf, 0);
            if (s0) {
                se->next = s;
                se = se->next;
            }
            else
                s0 = se = s;
        }
    }
    return (s0);
}
// End of HLPcache functions



/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2005 Whiteley Research Inc, all rights reserved.        *
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
 * This library is available for non-commercial use under the terms of    *
 * the GNU Library General Public License as published by the Free        *
 * Software Foundation; either version 2 of the License, or (at your      *
 * option) any later version.                                             *
 *                                                                        *
 * A commercial license is available to those wishing to use this         *
 * library or a derivative work for commercial purposes.  Contact         *
 * Whiteley Research Inc., 456 Flora Vista Ave., Sunnyvale, CA 94086.     *
 * This provision will be enforced to the fullest extent possible under   *
 * law.                                                                   *
 *                                                                        *
 * This library is distributed in the hope that it will be useful,        *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU      *
 * Library General Public License for more details.                       *
 *                                                                        *
 * You should have received a copy of the GNU Library General Public      *
 * License along with this library; if not, write to the Free             *
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.     *
 *========================================================================*
 *                                                                        *
 * Help System Files                                                      *
 *                                                                        *
 *========================================================================*
 $Id: help_cache.h,v 1.2 2017/04/19 12:56:36 stevew Exp $
 *========================================================================*/

#ifndef URLCACHE_H
#define URLCACHE_H

#include "lstring.h"
#include <string.h>


// Keep a list of user/passwds for the url's that need them
//
struct HLPauthList
{
    HLPauthList(char *up, char *u, char *p, HLPauthList *n)
        {
            url_prefix = lstring::copy(up);
            char *t = strrchr(url_prefix, '/');
            if (t)
                *t = 0;
            user = lstring::copy(u);
            passwd = lstring::copy(p);
            next = n;
        }

    HLPauthList *next;
    char *url_prefix;
    char *user;
    char *passwd;
};


// Defines for the URL cache
//
// The cache is placed in a directory $HOME/CACHE_DIR
// The cache map is in $HOME/CACHE_DIR/directory
// The files are named CACHE_DIR+1 followed by an integer
//
#define CACHE_DIR ".wr_cache"
#define CACHE_NUMFILES 64
#define CACHE_NUMHASH 31

// Status of transferred file
//
enum DLstatus { DLok, DLincomplete, DLnogo };

struct HLPcacheEnt
{
    HLPcacheEnt() { url = 0; filename = 0; status = DLincomplete; }
    HLPcacheEnt(const char *u, const char *f)
        {
            url = lstring::copy(u);
            filename = lstring::copy(f);
            status = DLincomplete;
        }
    ~HLPcacheEnt() { delete [] url; delete [] filename; }
    DLstatus get_status() { return (status); }

    friend struct HLPcache;

    char *url;
    char *filename;
private:
    void set_status(DLstatus s) { status = s; }

    DLstatus status;  // transfer status
};

struct HLPcacheTabEnt
{
    HLPcacheTabEnt(char *u, int e, HLPcacheTabEnt *n)
        { url = u; entry = e; next = n; }

    HLPcacheTabEnt *next;
    char *url;
    int entry;  // offset into entries array in HLPcache
};

struct HLPcache
{
    HLPcache();
    ~HLPcache()
        {
            clear();
            delete [] entries;
            delete [] dirname;
            delete nocache_ent;
        }

    HLPcacheEnt *get(const char*, bool);
    HLPcacheEnt *get_ext(const char*, bool);
    HLPcacheEnt *add(const char*, bool);
    HLPcacheEnt *add_ext(const char*, bool);
    void set_complete(HLPcacheEnt*, DLstatus);
    void set_complete_ext(HLPcacheEnt*, DLstatus);
    bool remove(const char*, bool);
    bool remove_ext(const char*, bool);
    void clear();
    void dump();
    void load();
    void resize(int);
    char *dir_name() { return (dirname); }
    stringlist *list_entries();

    static int cache_size;

private:
    char *dirname;
    int hash(const char *tag) {
        int k = 0;
        for ( ; *tag; k += *tag++) ;
        k %= CACHE_NUMHASH;
        return (k);
    }
    void tab_add(char *url, int ix) {
        int i = hash(url);
        table[i] = new HLPcacheTabEnt(url, ix, table[i]);
    }
    int tab_get(const char *url) {
        int i = hash(url);
        for (HLPcacheTabEnt *cte = table[i]; cte; cte = cte->next)
            if (!strcmp(url, cte->url))
                return (cte->entry);
        return (-1);
    }
    void tab_remove(const char*);
    void set_dir();

    int tagcnt;
    HLPcacheEnt *nocache_ent;
    HLPcacheEnt **entries;
    HLPcacheTabEnt *table[CACHE_NUMHASH];
    bool use_ext;
};

#endif



/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2014 Whiteley Research Inc, all rights reserved.        *
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
 * LSTPACK.CC -- Fast[er]Cap list file packer.                            *
 *                                                                        *
 *========================================================================*
 $Id: lstpack.cc,v 1.2 2014/07/19 03:44:24 stevew Exp $
 *========================================================================*/

#include "lstring.h"
#include "symtab.h"
#include "pathlist.h"


// This program will assemble a unified list file from separate files. 
// The unified format is supported by FasterCap from
// FastFieldSolvers.com, and by the Whiteley Research version of
// FastCap.  It is not supported in the original MIT FastCap, or in
// FastCap2 from FastFieldSolvers.com. 
//
// The argument is a path to a non-unified list file.  The unified
// file is created in the current directory.  If the original list
// file is named filename.lst, the new packed list file is named
// filename_p.lst.  All referenced files are expected to be found in
// the same directory as the original list file.

namespace {
    struct elt_t
    {
        const char *tab_name()          { return (e_name); }
        elt_t *tab_next()               { return (e_next); }
        void set_tab_next(elt_t *n)     { e_next = n; }
        elt_t *tgen_next(bool)          { return (e_next); }

        elt_t *e_next;
        char *e_name;
    };

    table_t<elt_t> *hash_tab;

    void hash(const char *str)
    {
        if (!hash_tab)
            hash_tab = new table_t<elt_t>;
        if (hash_tab->find(str))
            return;
        elt_t *e = new elt_t;
        e->e_next = 0;
        e->e_name = lstring::copy(str);
        hash_tab->link(e);
        hash_tab = hash_tab->check_rehash();
    }

    FILE *myopen(const char *dir, const char *fn)
    {
        if (!dir)
            return (fopen(fn, "r"));
        char *p = pathlist::mk_path(dir, fn);
        FILE *fp = fopen(p, "r");
        delete [] p;
        return (fp);
    }
}


int main(int argc, char **argv)
{
    if (argc != 2) {
        printf("Usage:  %s filename\n", argv[0]);
        printf(
        "This creates a FasterCap unified list file from old-style separate\n"
        "FastCap files, created in the current directory.\n\n");
        return (1);
    }
    FILE *fpin = fopen(argv[1], "r");
    if (!fpin) {
        printf("Unable to open %s.\n\n", argv[1]);
        return (2);
    }

    char buf[256];
    strcpy(buf, lstring::strip_path(argv[1]));
    char *e = strrchr(buf, '.');
    if (e)
        *e = 0;
    strcat(buf, "_p.lst");

    FILE *fpout =fopen(buf, "w");
    if (!fpout) {
        printf("Unable to open %s.\n\n", buf);
        return (3);
    }

    // Path assumed for all input.
    char *srcdir = 0;
    if (lstring::strrdirsep(argv[1])) {
        srcdir = lstring::copy(argv[1]);
        *lstring::strrdirsep(srcdir) = 0;
    }

    // Hash the file names.
    int lcnt = 0;
    while (fgets(buf, 256, fpin) != 0) {
        lcnt++;
        if (buf[0] == 'C' || buf[0] == 'c') {
            char *s = buf;
            lstring::advtok(&s);
            char *tok = lstring::getqtok(&s);
            if (!tok) {
                printf("Warning: C line without file name on line %d.\n",
                    lcnt);
                continue;
            }
            hash(tok);
            delete [] tok;
        }
        else if (buf[0] == 'D' || buf[0] == 'd') {
            char *s = buf;
            lstring::advtok(&s);
            char *tok = lstring::getqtok(&s);
            if (!tok) {
                printf("Warning: D line without file name on line %d.\n",
                    lcnt);
                continue;
            }
            hash(tok);
            delete [] tok;
        }
        fputs(buf, fpout);
    }
    fclose(fpin);
    fputs("End\n\n", fpout);

    // Open and add the files we've hashed.
    tgen_t<elt_t> gen(hash_tab);
    elt_t *elt;
    while ((elt = gen.next()) != 0) {
        fpin = myopen(srcdir, elt->tab_name());
        if (!fpin) {
            printf("Warning: can't open %s, not packed.\n", elt->tab_name());
            continue;
        }
        sprintf(buf, "File %s\n", elt->tab_name());
        fputs(buf, fpout);
        while (fgets(buf, 256, fpin) != 0)
            fputs(buf, fpout);
        fputs("End\n", fpout);
        fclose(fpin);
    }
    return (0);
}


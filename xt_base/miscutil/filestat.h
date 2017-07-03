
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
 * Misc. Utilities Library                                                *
 *                                                                        *
 *========================================================================*
 $Id: filestat.h,v 1.11 2014/10/04 22:18:35 stevew Exp $
 *========================================================================*/

#ifndef FILESTAT_H
#define FILESTAT_H

#include "lstring.h"
#include <unistd.h>
#include <string.h>

// Return from get_file_type()
enum GFTtype { GFT_NONE, GFT_FILE, GFT_DIR, GFT_OTHER };

// Return from check_file()
enum check_t { NOGO, NO_EXIST, READ_OK, WRITE_OK };

class filestat
{
public:
    ~filestat()
        {
            delete_files();
        }

    static GFTtype get_file_type(const char *path);
    static bool is_readable(const char*);
    static bool is_directory(const char*);
    static bool is_same_file(const char*, const char*);
    static check_t check_file(const char*, int);
    static FILE *open_file(const char*, const char*);
    static bool create_bak(const char*, char** = 0);
    static bool move_file_local(const char*, const char*);

    static void make_temp_conf(const char*, const char*);
    static char *make_temp(const char*);

    static const char *error_msg() { return (errmsg); }
    static void save_perror(const char *s) { save_sys_err(s); }

    static void set_rm_minutes(int);
    static int rm_minutes();
    static char *schedule_rm_file(const char*);

    static void queue_deletion(const char *fname)
        {
            if (!fname)
                return;
            for (stringlist *c = tmp_deletes; c; c = c->next)
                if (!strcmp(fname, c->string))
                    return;
            tmp_deletes = new stringlist(lstring::copy(fname), tmp_deletes);
        }

    static void delete_files()
        {
            for (stringlist *s = tmp_deletes; s; s = s->next)
                unlink(s->string);
            stringlist::destroy(tmp_deletes);
            tmp_deletes = 0;
        }

private:
    static void save_sys_err(const char*);
    static void save_err(const char*, ...);

    static char *errmsg;
    static char *mkt_def_id;
    static char *mkt_env_var;
    static stringlist *tmp_deletes;
    static int rm_file_minutes;
};

#endif


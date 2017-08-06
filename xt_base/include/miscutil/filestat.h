
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
 * Misc. Utilities Library                                                *
 *                                                                        *
 *========================================================================*
 $Id:$
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

    static const char *error_msg()          { return (errmsg); }
    static void save_perror(const char *s)  { save_sys_err(s); }
    static void clear_error()               { save_err(0); }

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


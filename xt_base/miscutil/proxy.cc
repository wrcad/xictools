
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

#include "proxy.h"
#include "pathlist.h"
#include "filestat.h"


#define PXFILE  ".wrproxy"

// Return the first line of the $HOME/.wrproxy file if found.
//
char *
proxy::get_proxy()
{
    char *home = pathlist::get_home("XIC_START_DIR");
    if (!home)
        return (0);

    char *pwpath = pathlist::mk_path(home, PXFILE);
    delete [] home;

    FILE *fp = fopen(pwpath, "r");
    delete [] pwpath;
    if (fp) {
        char buf[256];
        const char *s = fgets(buf, 256, fp);
        fclose(fp);
        return (lstring::getqtok(&s));
    }
    return (0);
}


// Static function.
// Write a $HOME/.wrproxy file.  The addr may already have a :port
// field, in which case port should be null.  The port will be parsed
// as an integer and if valid added to the addr following a colon.
//
const char *
proxy::set_proxy(const char *addr, const char *port)
{
    int p = 0;
    if (port && *port) {
        int tmp;
        if (sscanf(port, "%d", &tmp) == 1 && tmp > 0)
            p = tmp;
        else
            return ("bad port number");
    }
    char *a = lstring::gettok(&addr);
    if (!a)
        return ("bad host address");

    char *home = pathlist::get_home("XIC_START_DIR");
    if (!home) {
        delete [] a;
        return ("can't determine home directory");
    }

    char *pwpath = pathlist::mk_path(home, PXFILE);
    delete [] home;

    FILE *fp = fopen(pwpath, "w");
    delete [] pwpath;
    if (fp) {
        if (p > 0)
            fprintf(fp, "http://%s:%d\n", a, p);
        else
            fprintf(fp, "http://%s\n", a);
        delete [] a;
        fclose (fp);
        return (0);
    }
    delete [] a;
    return ("can't open " PXFILE " file for writing");
}


// Move the .wrproxy file according to the token:
//    "-"     .wrproxy -> .wrproxy.bak
//    "-abc"  .wrproxy -> .wrproxy.abc
//    "+"     .wrproxy.bak -> .wrproxy
//    "+abc"  .wrproxy.abc -> .wrproxy.
//
// Just return 0 if token doesn't start with - or +.
// Return a message on error.
//
const char *
proxy::move_proxy(const char *token)
{
    int c = *token++;
    if (c != '-' && c != '+')
        return (0);
    char *home = pathlist::get_home(0);
    if (!home)
        return ("can't determine home directory.");
    char *f1 = pathlist::mk_path(home, PXFILE);
    char *f2;
    if (!*token)
        f2 = pathlist::mk_path(home, PXFILE".bak");
    else {
        f2 = new char[strlen(f1) + strlen(token) + 2];
        sprintf(f2, "%s.%s", f1, token);
    }

    bool ret;
    if (c == '-')
        ret = filestat::move_file_local(f2, f1);  // f1 -> f2
    else
        ret = filestat::move_file_local(f1, f2);  // f2 -> f1
    delete [] home;
    delete [] f1;
    delete [] f2;
    return (ret ? 0 : filestat::error_msg());
}


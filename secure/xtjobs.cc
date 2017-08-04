
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
 * License server and authentication, and related utilities               *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include "secure.h"
#include "secure_prv.h"

#ifdef WIN32
#include "msw.h"
#include "lstring.h"
// Tell the msw interface that we're Generation 4.
const char *msw::MSWpkgSuffix = "-4";
#endif

#ifndef HAVE_STRERROR
#ifndef SYS_ERRLIST_DEF
char *sys_errlist[];
#endif
#endif

int BoxFilled;


// Arguments:
//  [ hostname ]    server host, defaults to "localhost".
//  [ -kill ]       kill the server.
//  [ -p host pid]  send XTV_CLOSE for host/pid.
//
int
main(int argc, char **argv)
{
    const char *usage =
        "Usage: xtjobs [serverhost[:port]] [-kill | -p jobhost jobpid]";
    bool kill = false;
    const char *job_host = 0;
    int job_pid = 0;
    const char *hostname = 0;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-kill"))
            kill = true;
        else if (!strcmp(argv[i], "-p")) {
            i++;
            if (i == argc) {
                puts(usage);
                return (1);
            }
            job_host = argv[i];
            i++;
            if (i == argc) {
                puts(usage);
                return (1);
            }
            if (sscanf(argv[i], "%d", &job_pid) != 1) {
                puts("Error parsing expected integer pid.");
                puts(usage);
                return (1);
            }
        }
        else if (*argv[i] == '-' || *argv[i] == '?') {
            puts("");
            puts(usage);
            puts(
                "List managed processes, or kill license server process, "
                "or remove\nprocess from management.\n");
            return (0);
        }
        else
            hostname = argv[i];
    }
    if (!hostname)
        hostname = getenv("XTLSERVER");
    if (!hostname)
        hostname = "localhost";
#ifdef WIN32
    // initialize winsock
    { WSADATA wsadata;
      if (WSAStartup(MAKEWORD(2, 2), &wsadata) != 0) {
        puts(
        "Error: WSA initialization failed, no interprocess communication.\n");
        exit(1);
      }
    }
#endif
    printf("Server: %s\n", hostname);
    sAuthChk a(false);
    if (kill)
        a.serverCmd(hostname, XTV_KILL);
    else if (job_host)
        a.serverCmd(hostname, XTV_CLOSE, job_host, job_pid);
    else
        a.serverCmd(hostname, XTV_DUMP);
    return (0);
}


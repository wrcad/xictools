
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

#include "config.h"
#include "errorrec.h"
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#ifdef WIN32
#include <winsock2.h>
#else
#include <netdb.h>
#endif


//-----------------------------------------------------------------------
// Error Handling Rules:
//
// 1) If a function calls get_err outside of push_error/pop_error, it
//    should also call init_error at the top, init_error should not be
//    called otherwise.
//
// 2) The add_error method is called only just before an immediate
//    return of an error condition (same for sys_error).
//
// 3) If a function calls a sub-function, and the sub-function error
//    return is not a fatal error, use push_error/pop_error around the
//    sub-function call.

ErrRec *ErrRec::instancePtr = 0;


// Push an error string into the error recorder.
//
void
ErrRec::add_error(const char *fmt, ...)
{
    va_list args;
    char buf[BUFSIZ];
    va_start(args, fmt);
    vsnprintf(buf, BUFSIZ, fmt, args);
    va_end(args);

    erMsgs = new stringlist(lstring::copy(buf), erMsgs);
}


// Add a system error string, as from the perror() call.
//
void
ErrRec::sys_error(const char *str)
{
    if (str && *str) {
#ifdef WIN32
        // Microsoft foulness.  The socket functions don't set errno,
        // so we attempt to intercept these.  Assume that str is
        // either the function name, or some text followed by the
        // function name, space separated.

        const char *t = strrchr(str, ' ');
        if (t)
            t++;
        else
            t = str;

        char buf[256];
        if (    !strcmp(t, "send")   || !strcmp(t, "recv")    ||
                !strcmp(t, "socket") || !strcmp(t, "connect") ||
                !strcmp(t, "accept") || !strcmp(t, "bind")    ||
                !strcmp(t, "listen") || !strcmp(t, "select")  ||
                lstring::prefix("gethost", t)) {

            sprintf(buf, "%s: WSA error code %d.\n", str, WSAGetLastError());
            add_error(buf);
            return;
        }
#endif
#ifdef HAVE_STRERROR
        add_error("%s: %s", str, strerror(errno));
    }
    else
        add_error("%s", strerror(errno));
#else
        add_error("%s: %s", str, sys_errlist[errno]);
    }
    else
        add_error("%s", sys_errlist[errno]);
#endif
}


// Add a system error string, as from the herror() call, for
// gethostbyxxx functions only.
//
void
ErrRec::sys_herror(const char *str)
{
#ifdef WIN32
    char buf[256];
    if (str && *str)
        sprintf(buf, "%s: WSA error code %d.\n", str, WSAGetLastError());
    else
        sprintf(buf, "WSA error code %d.\n", WSAGetLastError());
    add_error(buf);
#else

    if (str && *str) {
#ifdef HAVE_HSTRERROR
        add_error("%s: %s", str, hstrerror(h_errno));
    }
    else
        add_error("%s", hstrerror(h_errno));
#else
        add_error("%s: %s", str, "unknown error");
    }
    else
        add_error("%s", "unknown error");
#endif
#endif
}


// Return the error message text, clear error records.  The returned
// string is saved and freed on the next call.
//
const char *
ErrRec::get_error()
{
    delete [] lastMsg;
    if (erMsgs) {
        lastMsg = getstr();
        stringlist::destroy(erMsgs);
        erMsgs = 0;
    }
    else
        lastMsg = lstring::copy("No error messages in queue.");
    return (lastMsg);
}


// Private function to concatenate the error messages into a string.
//
char *
ErrRec::getstr()
{
    if (!erMsgs)
        return (0);
    int len = 0;
    for (stringlist *l = erMsgs; l; l = l->next)
        len += strlen(l->string) + 1;
    len++;
    char *mbuf = new char[len];
    char *t = mbuf;
    for (stringlist *l = erMsgs; l; l = l->next) {
        strcpy(t, l->string);
        while (*t)
            t++;
        if (*(t-1) != '\n')
            *t++ = '\n';
    }
    *t = 0;
    return (mbuf);
}


// Add a message to the warnings list.  This is a similar but separate
// capability.  Warnings will be saved if arm_warnings(true) has been
// called.  Use get_warnings() to get the current list, and
// clear_warnings() to clear the list (arm_warnings callse
// clear_warnings).  The warning list is unaffected by the error
// message calls.
//
void
ErrRec::add_warning(const char *fmt, ...)
{
    if (!warnings_flag)
        return;
    va_list args;
    char buf[BUFSIZ];
    va_start(args, fmt);
    vsnprintf(buf, BUFSIZ, fmt, args);
    va_end(args);

    // Don't save redundant messages.
    for (stringlist *s = warnings; s; s = s->next) {
        if (!strcmp(buf, s->string))
            return;
    }
    warnings = new stringlist(lstring::copy(buf), warnings);
}


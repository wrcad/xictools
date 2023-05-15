
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
 * WRspice Circuit Simulation and Analysis Tool                           *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3e2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 Thomas L. Quarles
         1992 Stephen R. Whiteley
****************************************************************************/

#include <stdarg.h>
#include "input.h"
#include "errors.h"
#include "misc.h"
#include "ginterf/graphics.h"

// 
//  Error handling functions
//


#define unknownError  "Unknown error code %d"
#define pausereq      "Pause requested"
#define nomem         "Unexpected null pointer, out of memory?"
#define panic         "Impossible error - this can't happen!"
#define failed        "Operation failed"
#define nockt         "Null circuit reference"
#define nodev         "No such device %s"
#define nomod         "No such model %s"
#define noanal        "No such analysis %s"
#define noterm        "No such terminal on this device"
#define novec         "No such vector %s"
#define notfound      "Not found"
#define badparm       "Parameter %s is unknown or invalid"
#define parmval       "Value of parameter %s is invalid"
#define unsupp        "Action unsupported by this simulator"
#define exists        "Name is already in use"
#define notempty      "Can't delete - still referenced"
#define nochange      "Sorry, simulator can't handle that now"
#define nodecon       "Bad node"
#define syntax        "Parse error"
#define toomuch       "Resource limit exceeded"

#define s_nodev       "(no device)"
#define s_nomod       "(no model)"
#define s_noanal      "(no analysis)"
#define s_noterm      "(no terminal)"
#define s_novec       "(no vector)"
#define s_notfound    "(not found)"
#define s_badparm     "(bad parameter)"
#define s_parmval     "(bad value)"
#define s_unsupp      "(unsupported)"
#define s_exists      "(exists)"
#define s_notempty    "(bad reference)"
#define s_nochange    "(no change)"
#define s_nodecon     "(bad node)"


void
SPinput::logError(sLine *l, const char *fmt, ...)
{
    if (l == IP_CUR_LINE)
        l = ip_current_line;

    va_list args;
    char buf[BUFSIZ];
    va_start(args, fmt);
    vsnprintf(buf, BUFSIZ, fmt, args);
    va_end(args);

    if (l)
        l->errcat(buf);
    else
        GRpkg::self()->ErrPrintf(ET_ERROR, "%s\n", buf);
}


void
SPinput::logError(sLine *l, int type, const char *badone)
{
    if (type == OK)
        return;
    if (l == IP_CUR_LINE)
        l = ip_current_line;

    char *s = errMesg(type, badone);
    if (s) {
        if (l)
            l->errcat(s);
        else
            GRpkg::self()->ErrPrintf(ET_ERROR, "%s\n", s);
        delete [] s;
        return;
    }
    char buf[BSIZE_SP];
    switch (type) {
    default:
        snprintf(buf, sizeof(buf), unknownError, type);
        break;
    case E_SYNTAX:
        if (badone)
            snprintf(buf, sizeof(buf), "%s: %s", syntax, badone);
        else
            strcpy(buf, syntax);
        break;
    }
    if (l)
        l->errcat(buf);
    else
        GRpkg::self()->ErrPrintf(ET_ERROR, "%s\n", buf);
}


char *
SPinput::errMesg(int code, const char *badone)
{
    char buf[BSIZE_SP];
    switch (code) {
    case E_PAUSE:
    case E_INTRPT:
        strcpy(buf, pausereq);
        break;
    case E_NOMEM:
        strcpy(buf, nomem);
        break;
    case E_PANIC:
        strcpy(buf, panic);
        break;
    case E_FAILED:
        strcpy(buf, failed);
        break;
    case E_NOCKT:
        strcpy(buf, nockt);
        break;
    case E_NODEV:
        snprintf(buf, sizeof(buf), nodev, badone ? badone : "");
        break;
    case E_NOMOD:
        snprintf(buf, sizeof(buf), nomod, badone ? badone : "");
        break;
    case E_NOANAL:
        snprintf(buf, sizeof(buf), noanal, badone ? badone : "");
        break;
    case E_NOTERM:
        strcpy(buf, noterm);
        break;
    case E_NOVEC:
        snprintf(buf, sizeof(buf), novec, badone ? badone : "found");
        break;
    case E_NOTFOUND:
        strcpy(buf, notfound);
        break;
    case E_BADPARM:
        snprintf(buf, sizeof(buf), badparm, badone ? badone : "given");
        break;
    case E_PARMVAL:
        snprintf(buf, sizeof(buf), parmval, badone ? badone : "");
        break;
    case E_UNSUPP:
        strcpy(buf, unsupp);
        break;
    case E_EXISTS:
        strcpy(buf, exists);
        break;
    case E_NOTEMPTY:
        strcpy(buf, notempty);
        break;
    case E_NOCHANGE:
        strcpy(buf, nochange);
        break;
    case E_NODECON:
        strcpy(buf, nodecon);
        break;
    case E_TOOMUCH:
        strcpy(buf, toomuch);
        break;
    default:
        return (0);
    }
    return (lstring::copy(buf));
}


const char *
SPinput::errMesgShort(int code)
{
    switch (code) {
    case E_NODEV:
        return (s_nodev);
    case E_NOMOD:
        return (s_nomod);
    case E_NOANAL:
        return (s_noanal);
    case E_NOTERM:
        return (s_noterm);
    case E_NOVEC:
        return (s_novec);
    case E_NOTFOUND:
        return (s_notfound);
    case E_BADPARM:
        return (s_badparm);
    case E_PARMVAL:
        return (s_parmval);
    case E_UNSUPP:
        return (s_unsupp);
    case E_EXISTS:
        return (s_exists);
    case E_NOTEMPTY:
        return (s_notempty);
    case E_NOCHANGE:
        return (s_nochange);
    case E_NODECON:
        return (s_nodecon);
    }
    return (0);
}


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
Authors: UCB CAD Group
         1992 Stephen R. Whiteley
****************************************************************************/

//
// Date and time utility functions
//

#include "config.h"
#include "misc.h"

#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#ifdef HAVE_FTIME
#include <sys/timeb.h>
#endif

// Return the date. Return value is static data.
//
char *
datestring()
{
#ifdef HAVE_GETTIMEOFDAY
    struct timeval tv;
    gettimeofday(&tv, 0);
    struct tm *tp = localtime((time_t *) &tv.tv_sec);
    char *ap = asctime(tp);
#else
    time_t tloc;
    time(&tloc);
    struct tm *tp = localtime(&tloc);
    char *ap = asctime(tp);
#endif

    static char tbuf[40];
    strcpy(tbuf, ap ? ap : "");
    int i = strlen(tbuf);
    tbuf[i - 1] = '\0';
    return (tbuf);
}


// Return the elapsed time in milliseconds from the first call.
//
unsigned long
millisec()
{
    static long long init_time;
    unsigned long elapsed = 0;
#ifdef HAVE_GETTIMEOFDAY
    struct timeval tv;
    gettimeofday(&tv, 0);
    if (!init_time)
        init_time = ((long long)tv.tv_sec)*1000 + tv.tv_usec/1000;
    else
        elapsed = ((long long)tv.tv_sec)*1000 + tv.tv_usec/1000 - init_time;
#else
#ifdef HAVE_FTIME
    struct timeb tb;
    ftime(&tb);
    if (!init_time)
        init_time = ((long long)tb.time)*1000 + tb.millitm;
    else
        elapsed = ((long long)tb.time)*1000 + tb.millitm - init_time;
#else
    NO TIMER PACKAGE
#endif
#endif
    return (elapsed);
}


// How many seconds have elapsed since epoch.
//
double
seconds()
{
#ifdef HAVE_GETTIMEOFDAY
    struct timeval tv;
    gettimeofday(&tv, 0);
    return (tv.tv_sec + tv.tv_usec/1000000.0);
#else
#ifdef HAVE_FTIME
    struct timeb timenow;
    ftime(&timenow);
    return (timenow.time + timenow.millitm/1000.0);
#else
    // don't know how to do this in general.
    return (-1.0); // Obvious error condition
#endif
#endif
}

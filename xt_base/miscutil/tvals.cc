
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
 * Misc. Utilities Library                                                *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

//
// A simple class for time measurement.
//

#include "config.h"
#include "tvals.h"

#include <sys/time.h>
#include <sys/types.h>
#ifndef HAVE_GETTIMEOFDAY
#ifdef HAVE_FTIME
#include <sys/timeb.h>
#endif
#endif

#ifdef HAVE_GETRUSAGE
#include <sys/resource.h>
#else
#ifdef HAVE_TIMES
#include <sys/times.h>
#include <sys/param.h>
#endif
#endif


// Begin a timing measurement.
//
void
Tvals::start()
{
    real_sec = 0;
    user_sec = 0;
    system_sec = 0;
#ifdef HAVE_GETTIMEOFDAY
    struct timeval tv;
    gettimeofday(&tv, 0);
    real_sec = tv.tv_sec + tv.tv_usec*1e-6;
#else
#ifdef HAVE_FTIME
    timeb timenow;
    ftime(&timenow);
    real_sec = timenow.time + timenow.millitm*1e-3;
#endif
#endif

#ifdef HAVE_GETRUSAGE
    struct rusage ruse;
    getrusage(RUSAGE_SELF, &ruse);
    user_sec = ruse.ru_utime.tv_sec + ruse.ru_utime.tv_usec*1e-6;
    system_sec = ruse.ru_stime.tv_sec + ruse.ru_stime.tv_usec*1e-6;
#else
#ifdef HAVE_TIMES
    struct tms ruse;
    time_t realt = times(&ruse);
#if !defined(HAVE_GETTIMEOFDAY) && !defined(HAVE_FTIME)
    real_sec = realt/HZ + (double)(realt % HZ)/HZ;
#endif
    user_sec = ruse.tms_utime/HZ + (double)(ruse.tms_utime % HZ)/HZ;
    system_sec = ruse.tms_stime/HZ + (double)(ruse.tms_stime % HZ)/HZ;
#endif
#endif
}


// Terminate a timing measurement.
//
void
Tvals::stop()
{
#ifdef HAVE_GETTIMEOFDAY
    struct timeval tv;
    gettimeofday(&tv, 0);
    real_sec = tv.tv_sec + tv.tv_usec*1e-6 - real_sec;
#else
#ifdef HAVE_FTIME
    timeb timenow;
    ftime(&timenow);
    real_sec = timenow.time + timenow.millitm*1e-3 - real_sec;
#endif
#endif

#ifdef HAVE_GETRUSAGE
    struct rusage ruse;
    getrusage(RUSAGE_SELF, &ruse);
    user_sec =
        ruse.ru_utime.tv_sec + ruse.ru_utime.tv_usec*1e-6 - user_sec;
    system_sec =
        ruse.ru_stime.tv_sec + ruse.ru_stime.tv_usec*1e-6 - system_sec;
#else
#ifdef HAVE_TIMES
    struct tms ruse;
    time_t realt = times(&ruse);
#if !defined(HAVE_GETTIMEOFDAY) && !defined(HAVE_FTIME)
    real_sec = realt/HZ + (double)(realt % HZ)/HZ - real_sec;
#endif
    user_sec =
        ruse.tms_utime/HZ + (double)(ruse.tms_utime % HZ)/HZ - user_sec;
    system_sec =
        ruse.tms_stime/HZ + (double)(ruse.tms_stime % HZ)/HZ - system_sec;
#endif
#endif
}


// Static function.
// Return the elapsed time in milliseconds from the first call.
//
unsigned long
Tvals::millisec()
{
    unsigned long elapsed = 0;
#ifdef HAVE_GETTIMEOFDAY
    static struct timeval tv0;
    struct timeval tv;
    if (tv0.tv_sec == 0)
        gettimeofday(&tv0, 0);
    else {
        gettimeofday(&tv, 0);
        elapsed = (tv.tv_sec - tv0.tv_sec)*1000 +
            (tv.tv_usec - tv0.tv_usec)/1000;
    }
#else
#ifdef HAVE_FTIME
    static struct timeb tb0;
    struct timeb tb;
    if (tb0.time == 0)
        ftime(&tb0);
    else {
        ftime(&tb);
        elapsed = (tb.time - tb0.time)*1000 + (tb.millitm - tb0.millitm);
    }
#endif
#endif
    return (elapsed);
}


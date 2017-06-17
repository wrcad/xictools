
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
 $Id: tvals.cc,v 1.5 2009/05/29 21:31:20 stevew Exp $
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


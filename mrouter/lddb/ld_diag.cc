
/*========================================================================*
 *                                                                        *
 *  Whiteley Research Inc, Sunnyvale, CA USA 2016, http://wrcad.com       *
 *                                                                        *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,      *
 *   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES      *
 *   OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-        *
 *   INFRINGEMENT.  IN NO EVENT SHALL STEPHEN R. WHITELEY OR WHITELEY     *
 *   RESEARCH INC. BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,   *
 *   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,   *
 *   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER        *
 *   DEALINGS IN THE SOFTWARE.                                            *
 *                                                                        *
 *   Licensed under the Apache License, Version 2.0 (the "License");      *
 *   you may not use this file except in compliance with the License.     *
 *   You may obtain a copy of the License at                              *
 *                                                                        *
 *        http://www.apache.org/licenses/LICENSE-2.0                      *
 *                                                                        *
 *   Unless required by applicable law or agreed to in writing, software  *
 *   distributed under the License is distributed on an "AS IS" BASIS,    *
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or      *
 *   implied. See the License for the specific language governing         *
 *   permissions and limitations under the License.                       *
 *                                                                        *
 *========================================================================*
 *                                                                        *
 * LEF/DEF Database and Maze Router.                                      *
 *                                                                        *
 * Stephen R. Whiteley (stevew@wrcad.com)                                 *
 * Whiteley Research Inc. (wrcad.com)                                     *
 *                                                                        *
 * Portions adapted from Qrouter by Tim Edwards,                          *
 * (www.opencircuitdesign.com) which used code by Steve Beccue.           *
 * See original headers where applicable.                                 *
 *                                                                        *
 *========================================================================*
 $Id: ld_diag.cc,v 1.3 2017/02/18 19:48:38 stevew Exp $
 *========================================================================*/

#include "config.h"
#include "lddb_prv.h"

#include <sys/time.h>
#include <sys/types.h>
#ifndef HAVE_GETTIMEOFDAY
#ifdef HAVE_FTIME
#include <sys/timeb.h>
#endif
#endif

#include <unistd.h>
#ifdef WIN32
#include <windows.h>
#endif
#ifdef __linux
#include <malloc.h>
#endif
#ifdef __APPLE__
#include <malloc/malloc.h>
#endif


//
// Some diagnostic utilities.
//


// Return the elapsed time in milliseconds from the initial call.
//
long
cLDDB::millisec()
{
    long elapsed = 0;
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


// Return the allocated data size in KB.
//
double
cLDDB::coresize()
{
#ifdef WIN32
    unsigned int size = 0;
    MEMORY_BASIC_INFORMATION m;
    for (char *ptr = 0; ptr < (char*)0x7ff00000; ptr += m.RegionSize) {
        VirtualQuery(ptr, &m, sizeof(m));
        if (m.AllocationProtect == PAGE_READWRITE &&
                m.State == MEM_COMMIT &&
                m.Type == MEM_PRIVATE)
            size += m.RegionSize;
    }
    return (.001*size);
#else

    size_t sz = 0;
#ifdef __APPLE__
    malloc_statistics_t st;
    malloc_zone_statistics(0, &st);
    sz = st.size_in_use;
#else
#ifdef __linux
    struct mallinfo m = mallinfo();
    sz = m.arena + m.hblkhd;
#endif
#endif
    return (.001*sz);
#endif
}


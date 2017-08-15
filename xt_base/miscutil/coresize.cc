
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

#include "coresize.h"
#include <sys/types.h>
#include <unistd.h>
#ifdef WIN32
#include "msw.h"
#else
#ifdef __linux
#include <malloc.h>
#else
#ifdef __APPLE__
#include <malloc/malloc.h>
#endif
#endif
#endif


// Return the allocated data size in KB.  This should only be used
// when not using the local allocator, use sMemory::coresize()
// instead, which will be far more accurate.
//
double coresize()
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


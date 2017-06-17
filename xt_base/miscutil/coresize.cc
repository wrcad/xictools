
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2010 Whiteley Research Inc, all rights reserved.        *
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
 $Id: coresize.cc,v 1.5 2015/12/14 03:42:22 stevew Exp $
 *========================================================================*/

#include "config.h"  // for HAVE_LOCAL_ALLOCATOR
#include "coresize.h"

#include <sys/types.h>
#include <unistd.h>

#ifdef HAVE_LOCAL_ALLOCATOR
#include "../malloc/local_malloc.h"
#endif

#ifdef WIN32
#include "msw.h"
#endif

#ifdef __linux
#include <malloc.h>
#endif


// Return true if the coresize function ia accurate.
//
bool have_coresize_metric()
{
#ifdef HAVE_LOCAL_ALLOCATOR
    return (Memory()->in_use() != 0);
#else
    return (false);
#endif
}


// Return the allocated data size in KB.  If not using the local
// malloc, this returns the break value change, which is a poor
// indicator.
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
#ifdef HAVE_LOCAL_ALLOCATOR
    size_t sz = Memory()->in_use();
    if (sz)
        return (.001*sz);
#else
    size_t sz = 0;
#endif

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


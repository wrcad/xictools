
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
 $Id: smartarray.h,v 1.2 2009/10/17 04:36:09 stevew Exp $
 *========================================================================*/

#ifndef SMARTARRAY_H
#define SMARTARRAY_H


// An array that will grow its size to cover the index given.
//
template <class T>   
struct SmartArray  
{
    // The constructor argument is the suggested initial size.
    SmartArray(int init_sz)
        {
            sa_list = 0;
            sa_size = 0;
            if (init_sz < 20)
                init_sz = 20;
            sa_init_sz = init_sz;
        }        

    ~SmartArray()
        {
            delete [] sa_list; 
        }

    T& operator[] (const int ix)
        {
            if (ix >= sa_size) {
                if (sa_size == 0) {
                    sa_size = ix + 1;   
                    if (sa_size < sa_init_sz)
                        sa_size = sa_init_sz;
                    sa_list = new T[sa_size];
                }
                else {
                    int nsz = sa_size + sa_size;
                    if (ix >= nsz)
                        nsz = ix + 1;
                    T *tmp = new T[nsz];
                    memcpy(tmp, sa_list, sa_size * sizeof(T));
                    sa_size = nsz;
                    delete [] sa_list;
                    sa_list = tmp;
                }
            }
            return (sa_list[ix]);
        }

    void clear(T **ptr)
        {
            if (ptr)
                *ptr = sa_list;
            else
                delete [] sa_list;
            sa_list = 0;
            sa_size = 0;
        }

protected:
    T *sa_list;
    int sa_size;
    int sa_init_sz;
};

#endif


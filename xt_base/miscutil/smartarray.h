
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
                    memcpy(tmp, sa_list, (int)(sa_size * sizeof(T)));
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


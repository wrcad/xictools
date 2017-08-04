
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

#ifndef ATOMIC_STACK_H
#define ATOMIC_STACK_H

//
// An atomic (thread-safe) stack, for use as a free list.  This builds
// only in x86_64 with GNU compilers or compatible.
//

#if (defined __x86_64__ && __GNUC__)
#define HAS_ATOMIC_STACK

// Data items are cast to this.
//
struct as_elt_t
{
    as_elt_t *next;
    u_int64_t tag;
};


// An aligned container for a pointer and tag number.  This is
// instantiated in the stack and contains the head pointer,
//
struct as_aligner_t
{
    // BEWARE:  This probably won't work with -fPIC, some register
    // backup is needed.
    //
    bool cas(as_aligner_t const &cmp, as_aligner_t const &exc)
        {
            bool result;
            __asm__ __volatile__ (
                "lock cmpxchg16b %1\n\t"
                "setz %0\n"
                : "=q" ( result )
                 ,"+m" ( ui )
                : "a" ( cmp.ptr ), "d" ( cmp.tag )
                 ,"b" ( exc.ptr ), "c" ( exc.tag )
                : "cc"
            );
            return result;
        }

    union {
        u_int64_t ui[2];
        struct {
            as_elt_t *ptr;
            u_int64_t tag;
        } __attribute__ (( __aligned__(16) ));
    };
};


// The main struct.  The stack is limited to stk_max entries.
//
struct as_stack_t
{
    as_stack_t()                { clear(); }
    as_elt_t *list_head()       { return (head.ptr); }

    void clear()
        {
            head.ptr = 0;
            head.tag = 0;
            elt_count = 0;
            tag_count = 0;
        }

    bool push(as_elt_t *elt, int stk_max)
        {
            if (elt_count >= stk_max)
                return (false);

            as_aligner_t s1, s2;
            do {
                s1 = head;
                elt->next = s1.ptr;
                elt->tag = __sync_add_and_fetch(&tag_count, 1);
                s2.ptr = elt;
                s2.tag = elt->tag;
            } while (!head.cas(s1, s2));

            __sync_fetch_and_add(&elt_count, 1);
            return (true);
        }

    as_elt_t *pop()
        {
            as_aligner_t s1, s2;
            do {
                s1 = head;
                if (!s1.ptr)
                    return (0);
                s2.ptr = s1.ptr->next;
                s2.tag = s2.ptr ? s2.ptr->tag : 0;

            } while (!head.cas(s1, s2));

            __sync_fetch_and_add(&elt_count, -1);
            return (s1.ptr);
        }

protected:
    as_aligner_t head;
    int elt_count;
    unsigned int tag_count;
};

#endif

#endif


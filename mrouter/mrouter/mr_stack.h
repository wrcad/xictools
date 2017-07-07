
/*========================================================================*
 *                                                                        *
 *  Whiteley Research Inc, Sunnyvale, CA USA 2017, http://wrcad.com       *
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
 $Id: mr_stack.h,v 1.3 2017/02/10 23:10:35 stevew Exp $
 *========================================================================*/

#ifndef MR_STACK_H
#define MR_STACK_H


//
// A stack for use in doRoute.  This replaces the linked list of
// mrPoint (equivalent) structs used in Qrouter, along with the
// "gunproc" list.  By using bulk allocation and arrays instead of
// linked lists, memory use is minimized.
//

// stkEltBlk struct is a little less than 8192 bytes.
// (8192-16)/sizeof(stkElt)
//
// #define STK_BLKSIZE 681  // ints
// #define STK_BLKSIZE 1362 // shorts
#define STK_BLKSIZE ((8192-16)/sizeof(stkElt))

// Provision for saving unprocessed elements in reverse order.  This
// was experimental and appears to provide no benefit.  Keep the code
// for now, it might be useful sometime.
//
// #define STK_REVERSE

// The stack.  This contains two lists:  the stack itself, and a
// similar structure for the "saved" elements.
//
struct mrStack
{
    // Element, block allocated.
    struct stkElt
    {
        grdu_t  x, y;
        short   l;
    };

    // Memory allocation block.
    struct stkEltBlk
    {
        stkEltBlk(stkEltBlk *n)
            {
                next = n;
#ifdef LD_MEMDBG
                mrStack::mrstack_cnt++;
                if (mrStack::mrstack_hw < mrStack::mrstack_cnt)
                    mrStack::mrstack_hw = mrStack::mrstack_cnt;
#endif
            }

#ifdef LD_MEMDBG
        ~stkEltBlk()
            {
                mrStack::mrstack_cnt--;
            }
#endif

        stkEltBlk *next;
        stkElt  blk[STK_BLKSIZE];
    };

#ifdef STK_REVERSE
    enum stkMode { svUnset, svNormal, svReverse };
#endif

    mrStack()
        {
            s_alloc     = 0;
            s_svalloc   = 0;
            s_blocks    = 0;
            s_svblocks  = 0;
#ifdef STK_REVERSE
            s_terminal  = 0;
            s_svmode    = svUnset;
#endif
        }

    ~mrStack()
        {
            while (s_blocks) {
                stkEltBlk *x = s_blocks;
                s_blocks = s_blocks->next;
                delete x;
            }
            while (s_svblocks) {
                stkEltBlk *x = s_svblocks;
                s_svblocks = s_svblocks->next;
                delete x;
            }
        }

    // Push an (x,y,l) triple on to the stack.
    //
    void push(int x, int y, int l)
        {
            if (!s_blocks || s_alloc == STK_BLKSIZE) {
                s_blocks = new stkEltBlk(s_blocks);
                s_alloc = 0;
            }
            stkElt *s = &s_blocks->blk[s_alloc++];
            s->x = x;
            s->y = y;
            s->l = l;
        }

    // Pop off an (x,y,l) triple from the stack, return false if the
    // stack is empty.
    //
    bool pop(int *px, int *py, int *pl)
        {
            if (!s_blocks)
                return (false);
            s_alloc--;
#ifdef STK_REVERSE
            if (!s_blocks->next && s_alloc < s_terminal) {
                delete s_blocks;
                s_blocks = 0;
                s_terminal = 0;
                return (false);
            }
#endif
            if (s_alloc < 0) {
                if (!s_blocks->next) {
                    delete s_blocks;
                    s_blocks = 0;
                    return (false);
                }
                stkEltBlk *b = s_blocks;
                s_blocks = b->next;
                delete b;
                s_alloc = STK_BLKSIZE-1;
            }
            stkElt *s = &s_blocks->blk[s_alloc];
            *px = s->x;
            *py = s->y;
            *pl = s->l;
            return (true);
        }

#ifdef STK_REVERSE
    // Push a triple to the front of the saved list.
    void push_save(int x, int y, int l)
        {
            if (s_svmode != svNormal) {
                if (s_svmode == svUnset)
                    s_svmode = svNormal;
                else {
                    printf("Stupid: can't mix norm/rev save\n");
                    return;
                }
            }
            if (!s_svblocks || s_svalloc == STK_BLKSIZE) {
                s_svblocks = new stkEltBlk(s_svblocks);
                s_svalloc = 0;
            }
            stkElt *s = &s_svblocks->blk[s_svalloc++];
            s->x = x;
            s->y = y;
            s->l = l;
        }

    // Append a triple to the saved list.
    void append_save(int x, int y, int l)
        {
            if (s_svmode != svReverse) {
                if (s_svmode == svUnset)
                    s_svmode = svReverse;
                else {
                    printf("Stupid: can't mix norm/rev save\n");
                    return;
                }
            }
            if (!s_svblocks || s_svalloc < 0) {
                s_svblocks = new stkEltBlk(s_svblocks);
                s_svalloc = STK_BLKSIZE - 1;
            }
            stkElt *s = &s_svblocks->blk[s_svalloc--];
            s->x = x;
            s->y = y;
            s->l = l;
        }
#else
    // Push a triple to the front of the saved list.
    void push_save(int x, int y, int l)
        {
            if (!s_svblocks || s_svalloc == STK_BLKSIZE) {
                s_svblocks = new stkEltBlk(s_svblocks);
                s_svalloc = 0;
            }
            stkElt *s = &s_svblocks->blk[s_svalloc++];
            s->x = x;
            s->y = y;
            s->l = l;
        }
#endif

    // Return true if any points have been saved.
    //
    bool has_saved()    { return (s_svblocks != 0); }

    // Clear the stack, then set up the stack to use the saved list.
    //
    void clear_to_saved()
        {
            clear();
#ifdef STK_REVERSE
            if (s_svmode == svReverse) {
                while (s_svblocks) {
                    stkEltBlk *b = s_svblocks;
                    s_svblocks = b->next;
                    b->next = s_blocks;
                    s_blocks = b;
                }
                s_terminal = s_svalloc + 1;
                s_svalloc = 0;
                s_alloc = STK_BLKSIZE;
            }
            else {
#endif
                s_blocks = s_svblocks;
                s_svblocks = 0;
                s_alloc = s_svalloc;
                s_svalloc = 0;
#ifdef STK_REVERSE
                s_terminal = 0;
            }
            s_svmode = svUnset;
#endif
        }

    // Clear the stack (saved list is not touched).
    //
    void clear()
        {
            while (s_blocks) {
                stkEltBlk *x = s_blocks;
                s_blocks = s_blocks->next;
                delete x;
            }
            s_alloc = 0;
#ifdef STK_REVERSE
            s_terminal = 0;
#endif
        }

private:
    int     s_alloc;        // Stack current element offset.
    int     s_svalloc;      // Saved stack current element offset.
    stkEltBlk *s_blocks;    // Linked list of stack element blocks.
    stkEltBlk *s_svblocks;  // Linked list of saved stack element blocks.
#ifdef STK_REVERSE
    int     s_terminal;     // Last valid index.
    stkMode s_svmode;       // Saving mode.
#endif
#ifdef LD_MEMDBG
public:
    static int mrstack_cnt; // Total blocks currently allocated.
    static int mrstack_hw;  // Allocated blocks high-water count.
#endif
};


// A container for dbNetList, used for the failed route list.
//
struct mrNetList
{
    mrNetList()
        {
            nl_head = 0;
            nl_tail = 0;
        }

    ~mrNetList()
        {
            dbNetList::destroy(nl_head);
        }

    void clear()
        {
            dbNetList::destroy(nl_head);
            nl_head = 0;
            nl_tail = 0;
        }

    // Put a new element at the front.
    void push(dbNet *net)
        {
            nl_head = new dbNetList(net, nl_head);
            if (!nl_tail)
                nl_tail = nl_head;
        }

    // Append a new element to the list.
    void append(dbNet *net)
        {
            if (nl_tail) {
                nl_tail->next = new dbNetList(net, 0);
                nl_tail = nl_tail->next;
            }
            else
                push(net);
        }

    // Append the list of elements.
    void append(dbNetList *lst)
        {
            if (!lst)
                return;
            if (nl_tail)
                nl_tail->next = lst;
            else
                nl_head = nl_tail = lst;
            while (nl_tail->next)
                nl_tail = nl_tail->next;
        }

    // Remove the initial element from the list, and return it.
    dbNet *pop()
        {
            if (!nl_head)
                return (0);
            dbNetList *nl = nl_head;
            nl_head = nl_head->next;
            if (!nl_head)
                nl_tail = 0;
            dbNet *net = nl->net;
            delete nl;
            return (net);
        }

    // Remove 'net' from the list, return true if found.
    //
    bool remove(dbNet *net)
        {
            if (!net)
                return (true);
            bool found = false;
            dbNetList *pv = 0, *nx;
            for (dbNetList *nl = nl_head; nl; nl = nx) {
                nx = nl->next;
                if (nl->net == net) {
                    // Assume that 'net' may appear multiple times, so
                    // search to the end.

                    found = true;
                    if (pv)
                        pv->next = nx;
                    else
                        nl_head = nx;
                    if (nl_tail == nl)
                        nl_tail = pv;
                    delete nl;
                    continue;
                }
                pv = nl;
            }
            return (found);
        }

    u_int num_elements()    { return (dbNetList::countlist(nl_head)); }

    bool is_empty()         { return (nl_head == 0); }

    dbNetList *list()       { return (nl_head); }

private:
    dbNetList *nl_head;
    dbNetList *nl_tail;
};

#endif


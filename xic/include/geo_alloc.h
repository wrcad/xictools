
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
 * Xic Integrated Circuit Layout and Schematic Editor                     *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#ifndef GEO_ALIAS_H
#define GEO_ALIAS_H


// Local memory management.

#define GF_BLKSIZE 256

// Factory.
//
template <class T>
struct tGEOfact
{
    // Allocation block.
    //
    struct gf_blk
    {
        gf_blk() { next = 0; }

        gf_blk *next;
        T array[GF_BLKSIZE];
    };

    // Generator for data element traversal.  This returns elements in
    // order of allocation.
    //
    struct gen
    {
        gen(tGEOfact<T> *gf)
            {
                g_cnt = 0;
                g_off = gf->gf_count;
                g_blk = &gf->gf_block0;
            }

        T *next()
            {
                if (g_blk->next) {
                    if (g_cnt < GF_BLKSIZE)
                        return (g_blk->array + g_cnt++);
                }
                else {
                    if (g_cnt < g_off)
                        return (g_blk->array + g_cnt++);
                }
                g_blk = g_blk->next;
                if (!g_blk)
                    return (0);
                g_cnt = 0;
                return (g_blk->array + g_cnt++);
            }

    private:
        int     g_cnt;
        int     g_off;
        gf_blk  *g_blk;
    };

    tGEOfact()
        {
            gf_cur_block = &gf_block0;
            gf_count = 0;
        }

    ~tGEOfact()
        {
            clear();
        }

    void clear()
        {
            gf_blk *b = gf_block0.next;
            gf_block0.next = 0;
            gf_count = 0;
            while (b) {
                gf_blk *bx = b;
                b = b->next;
                delete bx;
            }
        }

    T *new_obj()
        {
            if (gf_count == GF_BLKSIZE) {
                gf_cur_block->next = new gf_blk;
                gf_cur_block = gf_cur_block->next;
                gf_count = 0;
            }
            return (gf_cur_block->array + gf_count++);
        }

    unsigned int allocated()
        {
            unsigned int sz = 0;
            gf_blk *b = &gf_block0;
            while (b->next) {
                sz += GF_BLKSIZE;
                b = b->next;
            }
            sz += gf_count;
            return (sz);
        }

    unsigned long memuse()
        { 
            unsigned long sz = allocated();
            return (sz*sizeof(T));
        }

    bool is_empty()     { return (!gf_count && !gf_block0.next); }

//    gf_blk *blk_head()  { return (&gf_block0); }
//    int blk_offset()    { return (gf_count); }

private:
    gf_blk          *gf_cur_block;      // current block
    unsigned int    gf_count;           // offset into current block
    gf_blk          gf_block0;          // base block
};

#endif


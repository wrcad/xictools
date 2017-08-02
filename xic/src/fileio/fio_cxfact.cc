
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

#include "fio.h"
#include "fio_cxfact.h"
#include "fio_crgen.h"
#include "fio_zio.h"


// Allocate size bytes of storage, and return its ticket.  When a new
// allocation would overflow a block, the block is resized to include
// the new allocation, a subsequent allocation will begin a new block.
// Thus the data pointer returned from find is volatile, but the
// allocations exactly fit the data (except for the current block
// being filled).
//
ticket_t
bytefact_t::get_space_volatile(unsigned long size)
{
    if (size == 0)
        return (0);
    b_allocations++;
    b_inuse += size;

    if (!b_blocks) {
        b_blocks = new unsigned char*[BDB_INIT];
        b_allocated = BDB_INIT;
        b_blocks[0] = 0;
    }
    else if (b_used >= b_allocated) {

        if (b_used > BDB_MAX) {
            // out of tickets
            b_full = true;
            return (0);
        }

        unsigned char **tmp = new unsigned char*[b_allocated + b_allocated];
        memcpy(tmp, b_blocks, b_allocated*sizeof(char*));
        delete [] b_blocks;
        b_blocks = tmp;
        b_blocks[b_used] = 0;
        b_allocated += b_allocated;
    }

    ticket_t t;
    if (!b_blocks[b_used]) {
        if (size >= BDB_BSZ) {
            b_blocks[b_used] = new unsigned char[size];
            t = b_used*BDB_BSZ;
            b_used++;
            if (b_used < b_allocated)
                b_blocks[b_used] = 0;
            b_blkused = 0;
        }
        else {
            b_blocks[b_used] = new unsigned char[BDB_BSZ];
            t = b_used*BDB_BSZ;
            b_blkused = size;
        }
    }
    else {
        if (b_blkused + size >= BDB_BSZ) {
            unsigned char *tmp = new unsigned char[b_blkused + size];
            memcpy(tmp, b_blocks[b_used], b_blkused);
            delete [] b_blocks[b_used];
            b_blocks[b_used] = tmp;
            t = b_blkused + b_used*BDB_BSZ;
            b_blkused = 0;
            b_used++;
            if (b_used < b_allocated)
                b_blocks[b_used] = 0;
        }
        else {
            t = b_blkused + b_used*BDB_BSZ;
            b_blkused += size;
        }
    }
    return (t+1);
}


// This variation will start a new block if an allocation would
// overflow the current block.  Thus, bytes at the end of the block
// will be unused, but the pointer corresponding to a ticket, as
// returned with find, is stable.
//
ticket_t
bytefact_t::get_space_nonvolatile(unsigned long size)
{
    if (size == 0)
        return (0);

    b_allocations++;
    b_inuse += size;

again:
    if (!b_blocks) {
        b_blocks = new unsigned char*[BDB_INIT];
        b_allocated = BDB_INIT;
        b_blocks[0] = 0;
    }
    else if (b_used >= b_allocated) {

        if (b_used > BDB_MAX) {
            // out of tickets
            b_full = true;
            return (0);
        }

        unsigned char **tmp = new unsigned char*[b_allocated + b_allocated];
        memcpy(tmp, b_blocks, b_allocated*sizeof(char*));
        delete [] b_blocks;
        b_blocks = tmp;
        b_blocks[b_used] = 0;
        b_allocated += b_allocated;
    }

    ticket_t t;
    if (!b_blocks[b_used]) {
        if (size > BDB_BSZ) {
            b_blocks[b_used] = new unsigned char[size];
            t = b_used*BDB_BSZ;
            b_used++;
            if (b_used < b_allocated)
                b_blocks[b_used] = 0;
            b_blkused = 0;
        }
        else {
            b_blocks[b_used] = new unsigned char[BDB_BSZ];
            t = b_used*BDB_BSZ;
            b_blkused = size;
        }
    }
    else {
        if (b_blkused + size >= BDB_BSZ) {
            b_blkused = 0;
            b_used++;
            if (b_used < b_allocated)
                b_blocks[b_used] = 0;
            goto again;
        }
        else {
            t = b_blkused + b_used*BDB_BSZ;
            b_blkused += size;
        }
    }
    return (t+1);
}
// End of bytefact_t functions.


#ifdef WITH_CMPRS

// Allocations this size and larger will be compressed.  If less than
// CMP_MIN_THRESHOLD, no compression is used.  Compression of short
// strings is ineffective and may actually increase the size.
//
unsigned int cmp_bytefact_t::b_cmp_threshold = CMP_DEF_THRESHOLD;

ticket_t
cmp_bytefact_t::get_space(unsigned int size)
{
    ticket_t tkt;
    if (b_cmp_threshold >= CMP_MIN_THRESHOLD && size >= b_cmp_threshold) {
        tkt = get_space_volatile(size+5, true);
        unsigned char *s = find(tkt);
        if (!s)
            return (0);
        *s++ = CMP_HDR_U;
        unsigned int i = size;
        memcpy(s, &i, 4);
    }
    else {
        tkt = get_space_volatile(size + 1, false);
        unsigned char *s = find(tkt);
        *s = CMP_HDR;
    }
    return (tkt);
}


// If tkt specifies a block that was allocated with a compression
// header and is uncompressed, replace the block with a compressed
// block.
//
bool
cmp_bytefact_t::save_space(ticket_t tkt)
{
    if (b_cmp_threshold < CMP_MIN_THRESHOLD)
        return (true);
    if (!tkt)
        return (false);

    unsigned char *s = find(tkt);
    if (!s)
        return (false);
    if (*s == CMP_HDR_U) {
        unsigned int i;
        memcpy(&i, s+1, 4);
        unsigned char *c = zio_stream::zio_compress(s+5, &i, true, 5);
        if (c) {
            *c = CMP_HDR_C;
            memcpy(c+1, &i, 4);
            reassign_space(tkt, c);
        }
        // If compression failed, leave block as-is.
    }
    return (true);
}


// This takes the cs_ticket and sets the cs_stream and cs_size fields. 
// If the string was decompressed, then cs_size is the uncompressed
// size, and cs_stream must be freed by the user.  If not compressed,
// cs_size is 0.
//
void
cmp_bytefact_t::find_space(cmp_stream *cs)
{
    unsigned char *s = find(cs->cs_ticket);
    if (s) {
        if (*s == CMP_HDR_U) {
            // String hasn't been compressed yet, save_space not called.
            cs->cs_stream = s + 5;
            cs->cs_size = 0;
            return;
        }
        if (*s == CMP_HDR_C) {
            // Decompress.
            unsigned int i;
            memcpy(&i, s+1, 4);
            cs->cs_stream = zio_stream::zio_uncompress(s+5, &i);
            if (!cs->cs_stream) {
                // Hmmm, uncompression failed.  Caller needs to
                // check for null return.
                cs->cs_size = 0;
                return;
            }
            cs->cs_size = i;
            return;
        }
        s++;  // skip CMP_HDR byte
    }
    cs->cs_stream = s;
    cs->cs_size = 0;
}


// Assume that the string has been modified (cref flag changed),
// recompress and replace.
//
void
cmp_bytefact_t::update(cmp_stream *cs)
{
    if (!cs->cs_size)
        return;
    unsigned int i = cs->cs_size;
    unsigned char *c = zio_stream::zio_compress(cs->cs_stream, &i, true, 5);
    if (c) {
        *c = CMP_HDR_C;
        memcpy(c+1, &i, 4);
        reassign_space(cs->cs_ticket, c);
    }
}


// Allocate size bytes of storage, and return its ticket.  When a new
// allocation would overflow a block, the block is resized to include
// the new allocation, a subsequent allocation will begin a new block. 
// Thus the data pointer returned from find is volatile, but the
// allocations exactly fit the data (except for the current block
// being filled).
//
// If unique is true, the bytes are allocated in a single block, for
// easy replacement.  Otherwise, allocations generally share blocks.
//
//
ticket_t
cmp_bytefact_t::get_space_volatile(unsigned int size, bool unique)
{
    if (size == 0)
        return (0);
    b_allocations++;
    b_inuse += size;

    if (!check_blocks())
        return (0);

    // Blocks larger than CBDB_BSZ will be uniquely allocated.

    ticket_t t;
    if (!b_blocks[b_used]) {
        if (size >= CBDB_BSZ || unique) {
            b_blocks[b_used] = new unsigned char[size];
            t = b_used*CBDB_BSZ;
            b_used++;
            if (b_used < b_allocated)
                b_blocks[b_used] = 0;
            b_blkused = 0;
        }
        else {
            b_blocks[b_used] = new unsigned char[CBDB_BSZ];
            t = b_used*CBDB_BSZ;
            b_blkused = size;
        }
    }
    else {
        if (unique) {
            unsigned char *tmp = new unsigned char[b_blkused];
            memcpy(tmp, b_blocks[b_used], b_blkused);
            delete [] b_blocks[b_used];
            b_blocks[b_used] = tmp;
            b_used++;
            if (!check_blocks())
                return (0);
            b_blocks[b_used] = new unsigned char[size];
            t = b_used*CBDB_BSZ;
            b_used++;
            if (b_used < b_allocated)
                b_blocks[b_used] = 0;
            b_blkused = 0;
        }
        else if (b_blkused + size >= CBDB_BSZ) {
            unsigned char *tmp = new unsigned char[b_blkused + size];
            memcpy(tmp, b_blocks[b_used], b_blkused);
            delete [] b_blocks[b_used];
            b_blocks[b_used] = tmp;
            t = b_blkused + b_used*CBDB_BSZ;
            b_blkused = 0;
            b_used++;
            if (b_used < b_allocated)
                b_blocks[b_used] = 0;
        }
        else {
            t = b_blkused + b_used*CBDB_BSZ;
            b_blkused += size;
        }
    }
    return (t+1);
}


// If tkt specifies a uniquely allocated block, replace it with s,
// freeing the original.
//
bool
cmp_bytefact_t::reassign_space(ticket_t tkt, unsigned char *s)
{
    if (!tkt)
        return (false);
    tkt--;
    unsigned int blk = tkt/CBDB_BSZ;
    if (blk > b_used)
        return (false);
    if (tkt & (CBDB_BSZ - 1))
        return (false);
    delete [] b_blocks[blk];
    b_blocks[blk] = s;
    return (true);
}
// End of cmp_bytefact_t functions.

#endif


// Compression of cref_t lists.
//
// The function below is the interface to the readers.  Compression is
// used to save space, as the compressed list is typically less than
// half the size of the equivalent fixed-size cref_t structs. 
// Further, the compressed list is chained, so that for very long
// instance lists, the required contiguous memory does not become
// unreasonable.  In this case, compression occurs while reading the
// list, limiting total memory use.
//
// The order of the crefs must match the order found in the file.  The
// compression is applied as follows:
//
// If more than one cref:
//  1) We keep track of the diffs between the x/y variables to the
//     last cref.  These are typically smaller numbers than the
//     absolutes.
//  2) Create a table of numbers that are larger than one byte to
//     encode and are used more than once.  These numbers will be
//     accessed by an indirection index.  Used for x/y values only.
//  3) The size of the number table followed by the table is written.
//  4) We need eight flag bits for each cref:  four for modal skip
//     flags (we don't write a value if it was the same as previous),
//     two indirection flags, one for the (unused) set_area flag and
//     one for the end termination.  The value 0xff indicates end of
//     a chain.
//  5) The flags byte, followed by the values, is written for each
//     cref.
//  6) If chained, 0xff is written, followed by four bytes giving
//     the ticket of the next segment.  If not chained, the termination
//     byte is written.
//
// If one cref:
//  1) Write a zero size integer.
//  2) Write the flags and data.
//  3) Write the terminator byte.

// This is the maximum instance count per segment.
#define REC_MAX 10000

// Change REC_MAX to a small value for testing.
//#define CR_DEBUG

// The readers call this after calling new_cref.  The curtkt is the
// return from new_cref.  The symref is assumed to contain the first
// ticket in the current string segment.
//
// If curtkt exceeds the segmentation threshold, or is marked as the
// end, compression takes place.  If the end is seen, p is set to
// reference the comperssed stream.
//
bool
cxfact_t::cref_cmp_test(symref_t *p, ticket_t curtkt, CRCMPtype state)
{
    if (!cmp.allow_compress)
        return (true);

    ticket_t t0 = p->get_crefs();
    if (!t0) {
        Errs()->add_error("cref_cmp_test: error, no ticket in symref.");
        return (false);
    }
    if (state == CRCMP_start) {
        // Initialize and return.
        cmp.head = 0;
        cmp.last = 0;
        cmp.offset = 0;
        return (true);
    }
    if (state == CRCMP_end ||
            (curtkt - t0 > REC_MAX && is_block_start(curtkt))) {
        cr_writer crw(this);
        unsigned int num_values;
        unsigned int rec_cnt;
        SymTab *tab = crw.create_table(p, &num_values, &rec_cnt, curtkt);
        if (!tab && !rec_cnt && state == CRCMP_end) {
            // No more crefs were added, the symref ticket is bogus. 
            // See note below by set_crefs.  Change the termination of
            // the last link to end.
            if (!cmp.last) {
                // "Can't happen."
                Errs()->add_error("cref_cmp_test: error, bad ticket.");
                return (false);
            }
            unsigned char *offs = find_space(cmp.last) + cmp.offset - 1;
            *offs = CREF_END;
#ifdef CR_DEBUG
            printf("done (oddball case)\n");
#endif
            p->set_crefs(cmp.head);
            p->set_cmpr(true);
            cmp.head = 0;
            cmp.last = 0;
            cmp.offset = 0;
            cmp.has_compress = true;
            cref_clear();
            return (true);
        }
        ticket_t tkt = crw.build_list(p, tab, num_values, curtkt, true);
        if (!tkt) {
            delete tab;
            return (false);
        }
        crw.build_list(p, tab, num_values, curtkt, false);
        delete tab;

        if (!cmp.head)
            cmp.head = tkt;
        else {
#ifdef CR_DEBUG
            printf("new link %x\n", tkt);
#endif
            unsigned char *offs = find_space(cmp.last) + cmp.offset;
            offs[0] = tkt & 0xff;
            offs[1] = (tkt >> 8) & 0xff;
            offs[2] = (tkt >> 16) & 0xff;
            offs[3] = (tkt >> 24) & 0xff;
            // We're through with the last string for now, maybe
            // compress it.
            save_space(cmp.last);
        }
        cmp.offset = crw.bytes_used() - 4;
        cmp.last = tkt;

        if (state == CRCMP_end) {
#ifdef CR_DEBUG
            if (cmp.last != cmp.head)
                printf("done\n");
#endif
            p->set_crefs(cmp.head);
            p->set_cmpr(true);
            cmp.head = 0;
            cmp.last = 0;
            cmp.offset = 0;
            cmp.has_compress = true;
            // Maybe compress the string.
            save_space(tkt);
        }
        else {
#ifdef CR_DEBUG
#else
            cref_clear(p->get_crefs(), curtkt);
#endif
            // Careful!  The symref will point to a cgen_t that does
            // not exist.  If the cref is subsequently allocated, all
            // is well, however, it is possible that no nore crefs are
            // allocated before CRCMP_end.
            p->set_crefs(curtkt+1);
        }
#ifdef CR_DEBUG
        // Compare the compressed and uncompressed data, print
        // differences.  There shouldn't be any.

        // Temporarily swap in compressed stream.
        ticket_t tbak = p->get_crefs();
        p->set_crefs(tkt);

        ticket_t txx = t0;
        crgen_t gen(this, p);
        const cref_o_t *c;

        ticket_t next_tkt;
        while ((c = gen.next(&next_tkt)) != 0) {
            // Since the next_tkt arg is passed, this loop will terminate
            // at the end of the segment.
            cref_t *cc = find_cref(txx);
            if (!cc)
                break;
            txx++;

            if (c->srfptr != cc->refptr() || c->attr != cc->get_tkt() ||
                    c->tx != cc->pos_x() || c->ty != cc->pos_y() ||
                    c->flag != cc->get_flg()) {

                printf("< s=%u t=%u x=%d y=%d f=%d\n", cc->refptr(),
                    cc->get_tkt(), cc->pos_x(), cc->pos_y(), cc->get_flg());
                printf("> s=%u t=%u x=%d y=%d f=%d\n", c->srfptr, c->attr,
                    c->tx, c->ty, c->flag);
            }
        }
        p->set_crefs(tbak);
#endif
    }
    if (state == CRCMP_end)
        cref_clear();
    return (true);
}


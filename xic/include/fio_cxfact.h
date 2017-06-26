
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2007 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *  WHITELEY RESEARCH INCORPORATED PROPRIETARY SOFTWARE                   *
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
 *                                                                        *
 * XIC Integrated Circuit Layout and Schematic Editor                     *
 *                                                                        *
 *========================================================================*
 $Id: fio_cxfact.h,v 5.17 2015/06/11 05:54:06 stevew Exp $
 *========================================================================*/

#ifndef FIO_CXFACT_H
#define FIO_CXFACT_H

#include "fio_symref.h"


// If defined, use internal compression of CHD instance lists.
#define WITH_CMPRS

//-------------------------------------------------------------------------
// Allocator for symref_t and cref_t.
//-------------------------------------------------------------------------

#define BDB_INIT 32
#define BDB_BSZ  2048
#define BDB_MAX ((~(ticket_t)0)/BDB_BSZ)

// A factory to allocate random-sized chunks of memory.
//
struct bytefact_t
{
    bytefact_t()
        {
            b_blocks = 0;
            b_used = 0;
            b_allocated = 0;
            b_blkused = 0;
            b_allocations = 0;
            b_inuse = 0;
            b_records = 0;
            b_full = false;
        }

    ~bytefact_t()
        {
            if (b_blocks) {
                for (unsigned int i = 0; i <= b_used && i < b_allocated; i++)
                    delete [] b_blocks[i];
                delete [] b_blocks;
            }
        }

    void clear()
        {
            if (b_blocks) {
                for (unsigned int i = 0; i <= b_used && i < b_allocated; i++)
                    delete [] b_blocks[i];
                delete [] b_blocks;
            }
            b_blocks = 0;
            b_used = 0;
            b_allocated = 0;
            b_blkused = 0;
            b_allocations = 0;
            b_inuse = 0;
            b_records = 0;
            b_full = false;
        }

    unsigned char *find(ticket_t t)
        {
            if (!t)
                return (0);
            t--;
            unsigned int blk = t/BDB_BSZ;
            if (blk > b_used)
                return (0);
            unsigned int ix = t & (BDB_BSZ - 1);
            unsigned char *p = b_blocks[blk];
            if (!p)
                return (0);
            return (p + ix);
        }

    ticket_t get_space_volatile(unsigned long);
    ticket_t get_space_nonvolatile(unsigned long);

    size_t memuse()
        {
            return (sizeof(bytefact_t) + b_inuse + b_allocated*sizeof(char*) +
                (b_blkused ? BDB_BSZ - b_blkused : 0));
        }

    bool is_full() { return (b_full); }

    unsigned int add_count(unsigned int cnt)
        {
            b_records += cnt;
            return (b_records);
        }

    unsigned long bytes_inuse() { return (b_inuse); }
    unsigned int allocations() { return (b_allocations); }

protected:
    unsigned char **b_blocks;   // array of data blocks
    unsigned int b_used;        // current index into array
    unsigned int b_allocated;   // size of array
    unsigned int b_blkused;     // current offset into block
    unsigned int b_allocations; // number of allocations
    unsigned long b_inuse;      // bytes in use
    unsigned int b_records;     // user's record count
    bool b_full;                // out of tickets
};

#ifdef WITH_CMPRS

#define CBDB_INIT 32
#define CBDB_BSZ  2048
#define CBDB_MAX ((~(ticket_t)0)/CBDB_BSZ)

// Each block has a header byte:
#define CMP_HDR   0x0
#define CMP_HDR_U 0x1
#define CMP_HDR_C 0x2
// CMP_HDR
//   Ordinary small block.
// CMP_HDR_U
//   4-byte size followed by uncompressed data.
// CMP_HDR_C
//   4-byte size followed by compressed data.

// Default size threshold for compression.
#define CMP_DEF_THRESHOLD 256

// Minimum uncompressed length for compression.
#define CMP_MIN_THRESHOLD 100

// Struct used to pass a data string out of the cmp_bytefact_t
// database.  cmp_bytfact_t::find_space initializes the fields.  If
// cs_size is nonzero, then the cs_stream should be freed by the user,
// it has been decompressed.  If the cs_stream is changed, then
// cmp_bytefact::update can be called before the user frees the
// stream, to update the compressed block in the database.  This is
// only necessary if cs_size > 0.
//
struct cmp_stream
{
    cmp_stream(ticket_t t = 0)
        {
            cs_stream = 0;
            cs_size = 0;
            cs_ticket = t;
        }

    unsigned char *cs_stream;
    unsigned int cs_size;
    ticket_t cs_ticket;
};

// A factory to allocate small random-sized chunks of memory.  This
// variation gzips long blocks to save space in memory.
//
struct cmp_bytefact_t
{
    cmp_bytefact_t()
        {
            b_blocks = 0;
            b_used = 0;
            b_allocated = 0;
            b_blkused = 0;
            b_allocations = 0;
            b_inuse = 0;
            b_records = 0;
            b_full = false;
        }

    ~cmp_bytefact_t()
        {
            if (b_blocks) {
                for (unsigned int i = 0; i <= b_used && i < b_allocated; i++)
                    delete [] b_blocks[i];
                delete [] b_blocks;
            }
        }

    ticket_t get_space(unsigned int);
    bool save_space(ticket_t);
    void find_space(cmp_stream*);
    void update(cmp_stream*);

    size_t memuse()
        {
            return (sizeof(cmp_bytefact_t) + b_inuse +
                b_allocated*sizeof(char*) +
                (b_blkused ? CBDB_BSZ - b_blkused : 0));
        }

    bool is_full() { return (b_full); }

    unsigned int add_count(unsigned int cnt)
        {
            b_records += cnt;
            return (b_records);
        }

    unsigned long bytes_inuse() { return (b_inuse); }
    unsigned int allocations() { return (b_allocations); }

    static void set_cmp_threshold(unsigned int t) { b_cmp_threshold = t; }

private:
    unsigned char *find(ticket_t t)
        {
            if (!t)
                return (0);
            t--;
            unsigned int blk = t/CBDB_BSZ;
            if (blk > b_used)
                return (0);
            unsigned int ix = t & (CBDB_BSZ - 1);
            unsigned char *p = b_blocks[blk];
            if (!p)
                return (0);
            return (p + ix);
        }

    bool check_blocks()
        {
            if (!b_blocks) {
                b_blocks = new unsigned char*[CBDB_INIT];
                b_allocated = CBDB_INIT;
                b_blocks[0] = 0;
            }
            else if (b_used >= b_allocated) {
                if (b_used > CBDB_MAX) {
                    // out of tickets
                    b_full = true;
                    return (false);
                }
                unsigned char **tmp =
                    new unsigned char*[b_allocated + b_allocated];
                memcpy(tmp, b_blocks, b_allocated*sizeof(char*));
                delete [] b_blocks;
                b_blocks = tmp;
                b_blocks[b_used] = 0;
                b_allocated += b_allocated;
            }
            return (true);
        }

    ticket_t get_space_volatile(unsigned int, bool);
    bool reassign_space(ticket_t, unsigned char*);

    unsigned char **b_blocks;   // array of data blocks
    unsigned int b_used;        // current index into array
    unsigned int b_allocated;   // size of array
    unsigned int b_blkused;     // current offset into block
    unsigned int b_allocations; // number of allocations
    unsigned long b_inuse;      // bytes in use
    unsigned int b_records;     // user's record count
    bool b_full;                // out of tickets

    static unsigned int b_cmp_threshold;
};
#endif


//
// Memory management for symref_t and cref_t.
//
// These are allocated from the factory, and never freed, except by
// deleting the factory.  The structures can be referenced by a
// ticket_t as well as through the memory pointer.
//

#define FCT_BLOCKSIZE 256

struct fact_t
{
    fact_t(int d)
        {
            blocks = 0;
            indx = 0;
            datasize = d;
            bsize = 0;
            bused = 0;
        }

    ~fact_t()
        {
            clear();
        }

    // Return true if ix starts a new block.
    bool is_blstart(ticket_t ix)
        {
            fact_t *ft = this;
            if (!ix || !ft)
                return (false);
            ix--;  
            if (ix >= indx)
                return (false);
            return (!(ix & (FCT_BLOCKSIZE-1)));
        }

    // Delete all full blocks between i1 and i2.
    void clear(ticket_t i1, ticket_t i2)
        {
            if (i2 < i1) {
                ticket_t t = i2;
                i2 = i1;
                i1 = t;
            }

            fact_t *ft = this;
            if (!i1 || !ft)
                return;
            i1--;
            if (i1 >= indx)
                return;
            unsigned int bel1 = i1/FCT_BLOCKSIZE;
            unsigned int bix = i1 & (FCT_BLOCKSIZE-1);
            if (bix) {
                if (bel1 == 0xffffffff)
                    return;
                bel1++;
                if (bel1 >= bused)
                    return;
            }

            if (!i2)
                return;
            i2--;
            if (i2 >= indx)
                return;
            unsigned int bel2 = i2/FCT_BLOCKSIZE;
            bix = i2 & (FCT_BLOCKSIZE-1);
            if (bix != (FCT_BLOCKSIZE-1)) {
                if (!bel2)
                    return;
                bel2--;
            }
            for (unsigned int i = bel1; i <= bel2; i++) {
                delete [] blocks[i];
                blocks[i] = 0;
            }
        }

    void clear()
        {
            for (unsigned int i = 0; i < bused; i++)
                delete [] blocks[i];
            delete [] blocks;
            blocks = 0;
            indx = 0;
            bsize = 0;
            bused = 0;
        }

    ticket_t allocated()        const { return (indx); }
    unsigned int data_size()    const { return (datasize); }

    ticket_t new_item()
        {
            unsigned bel = indx/FCT_BLOCKSIZE;
            unsigned bix = indx & (FCT_BLOCKSIZE-1);
            if (!bix) {
                if (bused >= bsize) {
                    char **tb = new char*[bsize + FCT_BLOCKSIZE];
                    if (blocks) {
                        memcpy(tb, blocks, bsize*sizeof(char*));
                        delete [] blocks;
                    }
                    blocks = tb;
                    bsize += FCT_BLOCKSIZE;
                }
                blocks[bused] = new char[FCT_BLOCKSIZE*datasize];
                bused++;
            }
            indx++;
            char *elt = blocks[bel] + bix*datasize;
            memset(elt, 0, datasize);
            return (indx);
        }

    char *find_item(ticket_t ix)
        {
            fact_t *ft = this;
            if (!ix || !ft)
                return (0);
            ix--;
            if (ix >= indx)
                return (0);
            unsigned bel = ix/FCT_BLOCKSIZE;
            unsigned bix = ix & (FCT_BLOCKSIZE-1);
            return (blocks[bel] + bix*datasize);
        }

/*
    // It would be possible to eliminate the ticket_t stored in the
    // symref_t, however 1) this saves no space in 64-bit builds due
    // to alignment and 2) this function is called for each instance
    // wher reading in the file, and may have too much overhead.

    ticket_t find_ticket(char *p)
        {
            if (!p || !this)
                return (0);
            unsigned int bsz = FCT_BLOCKSIZE * datasize;
            for (unsigned int i = 0; i < bsize; i++) {
                if (p >= blocks[i] && p < blocks[i] + bsz) {
                    unsigned int bix = (p - blocks[i])/datasize;
                    return (i*FCT_BLOCKSIZE + bix + 1);
                }
            }
            return (0);
        }
*/

    size_t memuse()
        {
            // Return bytes in use by saved data container, plus overhead.
            size_t base = sizeof(fact_t);
            if (blocks) {
                base += bsize * sizeof(char*);
                base += bused * FCT_BLOCKSIZE * datasize;
            }
            return (base);
        }

private:
    char **blocks;          // array of blocks of FCT_BLOCKSIZE elements
    ticket_t indx;          // access code for elements
    unsigned int datasize;  // size of an element
    unsigned int bsize;     // size of blocks array
    unsigned int bused;     // existing blocks in array
};


// Keep track of cref compression state.
//
struct cr_cmpr_t
{
    cr_cmpr_t()
        {
            head = 0;
            last = 0;
            offset = 0;
            allow_compress = !FIO()->IsNoCompressContext();
            has_compress = false;
        }

    ticket_t head;
    ticket_t last;
    unsigned int offset;
    bool allow_compress;
    bool has_compress;
};


// Compression state arg for cxfact_t::cref_cmp_test.
enum CRCMPtype { CRCMP_start, CRCMP_check, CRCMP_end };

// Main factory, instantiate this in the container.
//
struct cxfact_t
{
    cxfact_t()
        {
            sytab = 0;
            crtab = 0;
            crstab = 0;
            flags = 0;
        }

    ~cxfact_t()
        {
            delete sytab;
            delete crtab;
            delete crstab;
        }

    //
    // General.
    //

    size_t data_memuse()
        {
            // Return size of data, excluding sizeof(this).
            size_t base = 0;
            if (sytab)
                base += sytab->memuse();
            if (crtab)
                base += crtab->memuse();
            if (crstab)
                base += crstab->memuse();
            return (base);
        }

    //
    // Symref allocation.
    //

    ticket_t new_symref(const char *name, DisplayMode mode, symref_t **ptr,
        bool is_cif = false)
        {
            // Note: CIF symrefs are allocated if and only if the first
            // allocation asks for CIF.
            if (!sytab)
                sytab = new fact_t(
                    is_cif ? sizeof(symref_cif_t) : sizeof(symref_t));
            ticket_t t = sytab->new_item();
            symref_t *p = (symref_t*)sytab->find_item(t);
            p->set_name(CD()->CellNameTableAdd(name));
            if (mode == Electrical)
                p->set_elec(true);

            // Only flag a "real" CIF symref.
            if (sytab->data_size() == sizeof(symref_cif_t))
                p->set_cif(true);
            p->set_ticket(t);
            if (ptr)
                *ptr = p;
            return (t);
        }

    symref_t *find_symref(ticket_t t)
        {
            return ((symref_t*)sytab->find_item(t));
        }

/*
    // See comment above.
    ticket_t find_symref_ticket(const symref_t *s)
        {
            return (sytab->find_ticket((char*)s));
        }
*/

    //
    // Cref allocation.
    //

    ticket_t new_cref(cref_t **ptr)
        {
            if (!crtab)
                crtab = new fact_t(sizeof(cref_t));
            ticket_t t = crtab->new_item();
            if (ptr)
                *ptr = (cref_t*)crtab->find_item(t);
            return (t);
        }

    cref_t *find_cref(ticket_t t)
        {
            return ((cref_t*)crtab->find_item(t));
        }

    void cref_clear()
        {
            if (crtab) crtab->clear();
        }

    void cref_clear(ticket_t t1, ticket_t t2)
        {
            if (crtab)
                crtab->clear(t1, t2);
        }

    bool is_block_start(ticket_t t)
        {
            return (crtab && crtab->is_blstart(t));
        }

    //
    // Stream allocation (compressed cref list).
    //
#ifdef WITH_CMPRS
    ticket_t get_space(int s, int rcnt = 0)
        {
            if (!crstab)
                crstab = new cmp_bytefact_t;
            crstab->add_count(rcnt);
            return (crstab->get_space(s));
        }

    void find_space(cmp_stream *cs)
        {
            if (crstab)
                crstab->find_space(cs);
        }

    // Call this ONLY for data known to be uncompressed.
    unsigned char *find_space(ticket_t t)
        {
            if (crstab) {
                cmp_stream cs(t);
                crstab->find_space(&cs);
                return (cs.cs_stream);
            }
            return (0);
        }

    bool save_space(ticket_t t)
        {
            if (!crstab)
                return (false);
            return (crstab->save_space(t));
        }

    void update(cmp_stream *cs)
        {
            if (crstab)
                crstab->update(cs);
        }
#else


    ticket_t get_space(int s, int rcnt = 0)
        {
            if (!crstab)
                crstab = new bytefact_t;
            crstab->add_count(rcnt);
            return (crstab->get_space_volatile(s));
        }

    unsigned char *find_space(ticket_t t)
        {
            if (!crstab)
                return (0);
            return (crstab->find(t));
        }

    bool save_space(ticket_t) { }
#endif

    void cref_count(unsigned int *ncrefs, unsigned long *bytes_inuse)
        {
            *ncrefs = crstab->add_count(0);
            *bytes_inuse = crstab->bytes_inuse();
        }

    bool is_full()
        {
            return (crstab && crstab->is_full());
        }

    //
    // Compression.
    //

    bool compression_allowed()
        {
            return (cmp.allow_compress);
        }

    bool has_compression()
        {
            return (cmp.has_compress);
        }

    bool cref_cmp_test(symref_t*, ticket_t, CRCMPtype);

private:
    fact_t *sytab;          // symref_t factory
    fact_t *crtab;          // cref_t factory
#ifdef WITH_CMPRS
    cmp_bytefact_t *crstab; // cref stream allocator
#else
    bytefact_t *crstab;     // cref stream allocator
#endif
    cr_cmpr_t cmp;          // compression state
    unsigned short flags;   // flags passed to symref_t
};

#endif


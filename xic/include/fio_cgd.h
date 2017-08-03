
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

#ifndef FIO_CGD_H
#define FIO_CGD_H


// The compression buffer saves bytes for compression.  If it
// overflows, the contents are passed to the compressor.  Otherwise, it
// is assumed that the "compressed" size will actually be larger than
// the uncompressed data for the small block, so the buffer will be
// dumped as-is.
//
#define CGD_COMPR_BUFSIZE 128

// Defines for CGD file header and record types.
//
#define CGD_MAGIC "Xic-Geom-1.0"
#define CGD_MAGIC1 "Xic-Geom-1.1"
// MAGIC1 includes a sourceName field.
//
#define CGD_CNAME_REC   20
#define CGD_LNAME_REC   21
#define CGD_END_REC     1


// Byte accumulator.
//
struct accum_t
{
    accum_t()
        {
            ac_data = 0;
            ac_count = 0;
            ac_csize = 0;
            ac_tempfile = 0;
            ac_fp = 0;
            ac_zfp = 0;
        }

    ~accum_t();

    bool put_byte(int);
    bool finalize();

    const unsigned char *data()
        {
            if (ac_csize) {
                const unsigned char *cc = ac_data;
                ac_data = 0;
                return (cc);
            }
            if (ac_count <= CGD_COMPR_BUFSIZE) {
                unsigned char *str = new unsigned char[ac_count];
                memcpy(str, ac_buf, ac_count);
                return (str);
            }
            return (0);
        }
    size_t compr_size()             const { return (ac_csize); }
    size_t uncompr_size()           const { return (ac_count); }

private:
    unsigned char *ac_data;         // data block accumulated
    size_t ac_count;                // bytes saved (uncompressed)
    size_t ac_csize;                // size of compressed block
    char *ac_tempfile;              // tempfile name for compression
    FILE *ac_fp;                    // tempfile file pointer
    zio_stream *ac_zfp;             // tempfile zlib stream
    unsigned char ac_buf[CGD_COMPR_BUFSIZE]; // buffer
};

// Geometry table element, holds geometry data for a layer.
//
struct cgd_lyr_t
{
    const char *tab_name()          { return (lname); }
    void set_tab_name(const char *n) { lname = n; }
    cgd_lyr_t *tab_next()           { return (next); }
    void set_tab_next(cgd_lyr_t *t) { next = t; }
    cgd_lyr_t *tgen_next(bool)      { return (next); }

    void set_data(const unsigned char *d)
        {
            u.data = d;
            csize_f |= 1;
        }
    const unsigned char *get_data()
        {
            if (csize_f & 1)
                return (u.data);
            return (0);
        }
    void free_data()
        {
            if (csize_f & 1) {
                delete [] u.data;
                csize_f--;
                u.offset = 0;
            }
        }

    void set_offset(uint64_t o)
        {
            u.offset = o;
            if (csize_f & 1)
                csize_f--;
        }
    uint64_t get_offset()
        {
            if (!(csize_f & 1))
                return (u.offset);
            return (0);
        }

    bool has_local_data()
        {
            return (csize_f & 1);
        }

    void set_csize(size_t sz)
        {
            csize_f = (sz << 1) | (csize_f & 1);
        }
    size_t get_csize()
        {
            return (csize_f >> 1);
        }

    void set_usize(size_t sz)
        {
            usize = sz;
        }
    size_t get_usize()
        {
            return (usize);
        }

    cgd_lyr_t *next;
    const char *lname;          // layer name
private:
    union {
        uint64_t offset;            // file offset of data stream
        const unsigned char *data;  // geometrical data stream
    } u;
    size_t csize_f;             // size of compressed data and flag
    size_t usize;               // size of uncompressed data
};

// Table element, holds a geometry table for a cell.
//
struct cgd_cn_t
{
    const char *tab_name()          { return (name); }
    void set_tab_name(const char *n) { name = n; }
    cgd_cn_t *tab_next()            { return (next); }
    void set_tab_next(cgd_cn_t *t)  { next = t; }
    cgd_cn_t *tgen_next(bool)       { return (next); }

    cgd_cn_t *next;
    const char *name;           // cell name
    table_t<cgd_lyr_t> *table;  // table for layer data
};

struct sCHDout;
struct oas_byte_stream;

// The database, holds geometry info for cells.
//
class cCGD
{
public:
    cCGD(const char*);
    cCGD(const char*, int);
    ~cCGD();

    // CHD linking interface.
    stringlist *cells_list();
    bool cell_test(const char*);
    stringlist *layer_list(const char*);
    bool layer_test(const char*, const char*);
    bool remove_cell(const char*);
    stringlist *unlisted_list();
    bool unlisted_test(const char*);
    bool get_byte_stream(const char*, const char*, oas_byte_stream**);

    char *info(bool);
    bool find_block(const char*, const char*, size_t*, size_t*,
        const unsigned char**);

    // Remote mode functions.
    bool set_remote_cgd_name(const char*);
    stringlist *remote_cgd_name_list();

    // Local mode functions.
    bool load_cells(cCHD*, stringlist*, double, bool);
    bool add_data(const char*, const char*, const unsigned char*,
        size_t, size_t);
    bool remove_cell_layer(const char*, const char*);
    stringlist *layer_info_list(const char*);
    bool get_cur_stream(uint64_t, size_t);
    void dump();

    bool write(const char*);
    bool write(FILE*, bool);
    bool read(const char*, CgdType);
    bool read(FILE*, CgdType, bool);

    static void set_chd_out(sCHDout *out) { cg_chd_out_s = out; }

    const char *sourceName()      const { return (cg_sourcename); }

    int connection_socket()       const { return (cg_skt); }

    // The refcnt is increment when this is linked to a CHD, and when
    // used by an oas_in.  Never delete this if refcnt is nonzero.
    //
    void inc_refcnt()                   { cg_refcnt++; }
    void dec_refcnt()                   { if (cg_refcnt) cg_refcnt--; }
    int refcnt()                  const { return (cg_refcnt); }

    // The id name is set while this is in CD table.  The char* is
    // owned by CD.  Never call set_id_name (only for CD), and never
    // free the id_string.
    //
    const char *id_name()         const { return (cg_dbname); }
    void set_id_name(char *nm)          { cg_dbname = nm; }

    // When set, this will be freed when unlinked from a CHD and
    // refcnt is zero.
    //
    bool free_on_unlink()         const { return (cg_free_on_unlink); }
    void set_free_on_unlink(bool b)     { cg_free_on_unlink = b; }

private:
    // Local mode functions.
    cgd_cn_t *add_cn(const char*);
    cgd_lyr_t *add_lyr(cgd_cn_t*, const char*);

    // Local mode data.
    char *cg_sourcename;            // Source file name or hostname:port/id.
    table_t<cgd_cn_t> *cg_table;
    sCHDout *cg_chd_out;            // Input channel. for building from
                                    // unified context/phys data file.
    unsigned char *cg_cur_stream;   // Current geom string, read from file.
    FILE *cg_fp;                    // Geometry file, closed in destructor.
    table_t<cgd_cn_t> *cg_unlisted; // Removed cells.

    // Remote mode data.
    char *cg_hostname;              // Name of remote host.
    char *cg_cgdname;               // Name if CGD on remote system.
    int cg_port;                    // Port number of remote server.
    int cg_skt;                     // Socket number of remote connection.
    bool cg_remote;                 // True in remote mode.

    // General.
    bool cg_free_on_unlink;         // Destroy this when unlinked from CHD.
    int cg_refcnt;                  // CHD link count.
    char *cg_dbname;                // Access name in CD CGD table.

    // Local mode support.
    eltab_t<cgd_lyr_t> cg_eltab_lyr;
    eltab_t<cgd_cn_t> cg_eltab_cn;
    strtab_t cg_string_tab;

    static sCHDout *cg_chd_out_s;   // Registered input channel.
};

#endif


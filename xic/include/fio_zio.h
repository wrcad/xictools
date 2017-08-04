
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
 * Xic Integrated Circuit Layout and Schematic Editor                     *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#ifndef FIO_ZIO_H
#define FIO_ZIO_H

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <zlib.h>


// Buffer size for compressed bytes.
#define Z_INSIZE    16384U

// Buffer size for uncompressed bytes.  This should NOT be smaller than
// Z_WINSIZE. 
#define Z_OUTSIZE   32768U

// Size of sliding data window for random access.
#define Z_WINSIZE   32768U

struct zio_index;
struct sFilePtr;

// Low-level zlib interface
//
struct zio_stream
{
    zio_stream(FILE*);
    ~zio_stream();

    static unsigned char *zio_compress(const unsigned char*,
        unsigned int*, bool = false, unsigned int = 0);
    static unsigned char *zio_uncompress(const unsigned char*,
        unsigned int*, bool = false, unsigned int = 0);
    static zio_stream *zio_open(FILE*, const char*);
    static zio_stream *zio_open(sFilePtr*, const char*, int64_t = 0);

    bool zio_gzinit();

    int zio_getc()
        {
            unsigned char c;
            if (z_avail > 0) {
                c = (unsigned char)*z_ptr++;
                z_avail--;
                return (c);
            }
            return (zio_read(&c, 1) == 1 ? c : -1);
        }

    int zio_putc(int c)
        { unsigned char cc = c; return (zio_write(&cc, 1) == 1 ? cc : -1); }

    int zio_read(void*, unsigned int);
    int zio_write(const void*, unsigned int);

    int zio_flush() { return (flush(Z_SYNC_FLUSH)); }

    int zio_rewind();
    int zio_seek(int64_t, int);
    int64_t zio_tell()
        { return (z_mode == 'r' ? z_total_out - z_avail : z_total_in); }
    int zio_eof()           { return (z_eof); }
    FILE *zio_fp()          { return (z_fp); }
    z_stream &stream()      { return (z_strm); }
    unsigned int zio_crc()  { return (z_file_crc); }
    void zio_set_crc(unsigned int c) { z_file_crc = c; }

private:
    int seek_to(int64_t, zio_index*);
    int read_core(void*, unsigned int);
    int flush(int);
    int get_byte();
    unsigned int get_unsigned();
    void put_unsigned(unsigned int);
    bool check_gz_header();

    z_stream        z_strm;
    FILE            *z_fp;          // .gz file
    sFilePtr        *z_zfp;         // input stream
    Byte            *z_inbuf;       // input buffer
    Byte            *z_outbuf;      // output buffer
    char            *z_ptr;         // output buffer pointer for reading
    int64_t         z_startpos;     // initial offset
    int64_t         z_total_in;     // 64-bit shadow for stream.total_in
    int64_t         z_total_out;    // 64-bit shadow for stream.total_out
    int64_t         z_in_limit;     // limit number of source bytes read
    unsigned int    z_crc;          // running crc32 of uncompressed data
    unsigned int    z_file_crc;     // crc32 of uncompressed data from file
    int             z_avail;        // output buffer count for reading
    int             z_err;          // error code for last stream operation
    bool            z_eof;          // set if end of input file
    bool            z_header;       // if true, using gzip header
    char            z_mode;         // 'w' or 'r'
    bool            z_had_header;   // true if header was checked
    bool            z_no_checksum;  // skip checksum test (seek called)
};


// Random access point list and interface.
//
struct zio_index
{
    // Access point entry.
    struct point_t
    {
        uint64_t out;       // corresponding offset in uncompressed data
        uint64_t in;        // offset in input file of first full byte
        int bits;           // number of bits (1-7) from byte at in - 1, or 0
        unsigned char *window;  // Z_WINSIZE buffer for uncompressed data
    };

#define PT_BLKSIZE 10
    // Bulk-allocated data blocks.
    struct pt_data_t
    {
        pt_data_t(pt_data_t *n)
            {
                d_next = n;
                d_blk = 0;
            }

        pt_data_t *d_next;
        unsigned int d_blk;
        unsigned char d_data[PT_BLKSIZE*Z_WINSIZE];
    };

    zio_index()
        {
            zi_list = 0;
            zi_data = 0;
            zi_have = 0;
            zi_size = 0;
            zi_crc = 0;
            zi_refcnt = 0;
        }

    ~zio_index()
        {
            delete [] zi_list;
            while (zi_data) {
                pt_data_t *dp = zi_data;
                zi_data = zi_data->d_next;
                delete dp;
            }
        }

    // Find where in stream to start.
    point_t *get_point(uint64_t offset)
        {
            /* linear search
            point_t *here = zi_list;
            int cnt = zi_have;
            while (--cnt && here[1].out <= offset)
                here++;
            return (here);
            */

            // binary search
            int low = 0, mid = 0, high = zi_have - 1;
            while (low <= high) {
                mid = (low + high)/2;
                if (offset < zi_list[mid].out) {
                    if (mid == 0)
                        break;
                    high = mid - 1;
                }
                else if (mid+1 < zi_have && offset > zi_list[mid+1].out)
                    low = mid + 1;
                else
                    break;
            }
            return &zi_list[mid];
        }

    int build_index(FILE*, unsigned int);
    bool to_file(const char*);
    bool from_file(const char*);

    unsigned int crc()      { return (zi_crc); }
    void ref()              { zi_refcnt++; }
    void unref()            { zi_refcnt--; }
    int refcnt()            { return (zi_refcnt); }

private:
    bool addpoint(int, uint64_t, uint64_t, unsigned int, unsigned char*);

    point_t *zi_list;       // allocated list
    pt_data_t *zi_data;     // list of data blocks
    int zi_have;            // number of list entries filled in
    int zi_size;            // number of list entries allocated
    int zi_crc;             // crc checksum of file that was indexed
    int zi_refcnt;          // references to this index
};


// "FILE" pointer: use stdio for uncompressed files, zio_stream for
// compressed files.
//
struct sFilePtr
{
    sFilePtr(zio_stream *f, FILE *p)
        {
            file = f;
            fp = p;
            keep_fp = false;
        }

    sFilePtr(zio_stream *f, FILE *p, bool k)
        {
            file = f;
            fp = p;
            keep_fp = k;
        }

    ~sFilePtr()
        {
            if (file) {
                fp = file->zio_fp();
                delete file;
            }
            if (fp) {
                if (keep_fp)
                    rewind(fp);
                else
                    fclose(fp);
            }
        }

    static sFilePtr *newFilePtr(const char*, const char*);
    static sFilePtr *newFilePtr(FILE*);

    inline int z_puts(const char*);
    inline int z_putc(int);
    inline int z_getc();
    inline int z_flush();
    inline void z_rewind();
    inline int z_eof();
    inline void z_clearerr();
    inline int z_read(void*, size_t, size_t);
    inline int z_write(const void*, size_t, size_t);
    inline char *z_gets(char*, int);
    inline unsigned int z_crc();
    inline void z_set_crc(unsigned int);

    int z_seek(int64_t, int);
    int64_t z_tell();

    zio_stream *file;
    FILE *fp;
    bool keep_fp;
};
typedef sFilePtr* FilePtr;


inline int
sFilePtr::z_puts(const char *s)
{
    if (file)
        return (file->zio_write(s, strlen(s)));
    else if (fp) {
        int r = fputs(s, fp);
#ifndef WIN32
        if (r == EOF) {
            // See note in z_putc
            clearerr(fp);
            fsync(fileno(fp));
            return (fputs(s, fp));
        }
#endif
        return (r);
    }
    return (EOF);
}

inline int
sFilePtr::z_putc(int c)
{
    if (file)
        return (file->zio_putc(c));
    else if (fp) {
        int r = putc(c, fp);
#ifndef WIN32
        if (r == EOF) {
            // This fixes a problem seen under Linux, when low on memory.
            // Might be due to writing to remote file system.  Some buffer
            // can't resize, so write fails, but will succeed after fsync.
            clearerr(fp);
            fsync(fileno(fp));
            return (putc(c, fp));
        }
#endif
        return (r);
    }
    return (EOF);
}

inline int
sFilePtr::z_getc()
{
    if (file)
        return (file->zio_getc());
    else if (fp)
#ifndef WIN32
        // The unlocked variation is faster but is not thread-safe.
        // Function getc_unlocked is available in Linux, Solaris, AIX.
        return (getc_unlocked(fp));
#else
        return (getc(fp));
#endif
    return (EOF);
}

inline int
sFilePtr::z_flush()
{
    if (file)
        return (file->zio_flush());
    else if (fp)
        return (fflush(fp));
    return (EOF);
}

inline void
sFilePtr::z_rewind()
{
    if (file)
        file->zio_rewind();
    else if (fp)
        rewind(fp);
}

inline int
sFilePtr::z_eof()
{
    if (file)
        return (file->zio_eof());
    else if (fp)
        return (feof(fp));
    return (-1);
}

inline void
sFilePtr::z_clearerr()
{
    if (file)
        // this is a no-op, EOF is reset by seek
        return;
    else if (fp)
        clearerr(fp);
}

inline int
sFilePtr::z_read(void *buf, size_t size, size_t nmemb)
{
    if (file) {
        int n = file->zio_read(buf, size*nmemb);
        if (n <= 0)
            return (n);
        return (n/size);
    }
    else if (fp)
#ifdef __linux
        // The unlocked variation is faster but is not thread-safe.
        // Function fread_unlocked is in Linux (glibc).
        return (fread_unlocked(buf, size, nmemb, fp));
#else
        return (fread(buf, size, nmemb, fp));
#endif
    return (0);
}

inline int
sFilePtr::z_write(const void *buf, size_t size, size_t nmemb)
{
    if (file) {
        int n = file->zio_write(buf, size*nmemb);
        if (n <= 0)
            return (n);
        return (n/size);
    }
    else if (fp)
        return (fwrite(buf, size, nmemb, fp));
    return (0);
}

inline char *
sFilePtr::z_gets(char *str, int len)
{
    if (file) {
        if (!str || len <= 0)
            return (0);
        char *tstr = str;
        while (--len > 0 && file->zio_read(str, 1) == 1 && *str++ != '\n') ;
        *str = 0;
        return (tstr == str && len > 0 ? 0 : tstr);
    }
    else if (fp)
        return (fgets(str, len, fp));
    return (0);
}


// Get or set the crc value for gzipped files.  This is used for
// random access.  If the file is read to the end, the crc value will
// be available.  It can also be set if the value is known, allowing
// use of the access tables.  There is no consistency checking, so the
// value had better be right when setting.

inline unsigned int
sFilePtr::z_crc()
{
    return (file ? file->zio_crc() : 0);
}


inline void
sFilePtr::z_set_crc(unsigned int c)
{
    if (file)
        file->zio_set_crc(c);
}

#endif



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

#include "largefile.h"
#include "config.h"
#include "fio.h"
#include "fio_zio.h"
#include "fio_chd.h"
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#ifdef WIN32
// The regular fseeko/ftello are not available, but the 64-bit versions
// are.
#define fopen fopen64
#define fseeko fseeko64
#define ftello ftello64
#endif

#ifdef ZLIB_VERNUM
#if (ZLIB_VERNUM >= 0x1230)
#define Z_HAS_RANDOM_MAP
#endif
#endif


//
// Supporting functions for 64-bit file offset and zlib.
//

// Uncomment to turn on random access index debugging.
// #define MAP_DEBUG

// First two bytes of a gzip file
//
#define GZMAGIC_0 0x1f
#define GZMAGIC_1 0x8b

// gzip flag byte
//
#define ASCII_FLAG   0x01 // bit 0 set: file probably ascii text
#define HEAD_CRC     0x02 // bit 1 set: header CRC present
#define EXTRA_FIELD  0x04 // bit 2 set: extra field present
#define ORIG_NAME    0x08 // bit 3 set: original file name present
#define COMMENT      0x10 // bit 4 set: file comment present
#define RESERVED     0xE0 // bits 5..7: reserved

// Desired distance between random access points.
#define SPAN 1048576L


// Static Function.
//
sFilePtr *
sFilePtr::newFilePtr(const char *path, const char *mode)
{
    if (!mode)
        return (0);

    if (*mode == 'r') {
        FILE *fp = FIO()->POpen(path, "rb", 0);
        if (!fp)
            return (0);
        if (getc(fp) != GZMAGIC_0 || getc(fp) != GZMAGIC_1) {
            rewind(fp);
            return (new sFilePtr(0, fp));
        }
        rewind(fp);
        zio_stream *file = zio_stream::zio_open(fp, "r");
        if (!file)
            return (new sFilePtr(0, fp));
        if (!file->zio_gzinit()) {
            delete file;
            rewind(fp);
            return (new sFilePtr(0, fp));
        }
        return (new sFilePtr(file, 0));
    }
    if (*mode == 'w') {
        FILE *fp = fopen(path, "wb");
        if (!fp)
            return (0);
        const char *s = strrchr(path, '.');
        if (s && !strcmp(s, ".gz")) {
            zio_stream *file = zio_stream::zio_open(fp, "w");
            if (file) {
                if (file->zio_gzinit())
                    return (new sFilePtr(file, 0));
                delete file;
                rewind(fp);
            }
        }
        return (new sFilePtr(0, fp));
    }
    return (0);
}


// Static Function.
//
// Reading only!
//
sFilePtr *
sFilePtr::newFilePtr(FILE *fp)
{
    if (!fp)
        return (0);
    rewind(fp);
    if (getc(fp) != GZMAGIC_0 || getc(fp) != GZMAGIC_1) {
        rewind(fp);
        return (new sFilePtr(0, fp, true));
    }
    rewind(fp);
    zio_stream *file = zio_stream::zio_open(fp, "r");
    if (!file)
        return (new sFilePtr(0, fp, true));
    if (!file->zio_gzinit()) {
        delete file;
        rewind(fp);
        return (new sFilePtr(0, fp, true));
    }
    return (new sFilePtr(file, 0, true));
}


int
sFilePtr::z_seek(int64_t offset, int whence)
{
    if (file)
        return (file->zio_seek(offset, whence));
    else if (fp)
        return (fseeko(fp, offset, whence));
    return (-1);
}


int64_t
sFilePtr::z_tell()
{
    if (file)
        return (file->zio_tell());
    else if (fp)
        return (ftello(fp));
    return (-1);
}
// End of sFilePtr functions


// This should be used instead of fopen when support for large files
// is needed.
//
FILE *
large_fopen(const char *path, const char *mode)
{
    return (fopen(path, mode));
}


// The following can be used with any file pointer, but a file larger
// than 2Gb must be opened with large_fopen.

// 64-bit seek function.
//
int64_t
large_fseek(FILE *fp, int64_t offset, int whence)
{
    return (fseeko(fp, offset, whence));
}


// 64-bit tell function.
//
int64_t
large_ftell(FILE *fp)
{
    return (ftello(fp));
}


// Compress/uncompress function.
//
char *
gzip(const char *infile, const char *outfile, GZIPmode mode)
{
    const char *msg1 = "can't open %s";
    const char *msg2 = "gz open failed on %s";
    const char *msg3 = "gz init failed on %s";
    const char *msg4 = "write error - aborted";
    if (mode == GZIPcompress) {
        FILE *fp = fopen(outfile, "wb");
        if (!fp) {
            char *str = new char[strlen(msg1) + strlen(outfile)];
            sprintf(str, msg1, outfile);
            return (str);
        }
        zio_stream *out = zio_stream::zio_open(fp, "w");
        if (!out) {
            fclose(fp);
            char *str = new char[strlen(msg2) + strlen(outfile)];
            sprintf(str, msg2, outfile);
            return (str);
        }
        if (!out->zio_gzinit()) {
            delete out;
            fclose(fp);
            char *str = new char[strlen(msg3) + strlen(outfile)];
            sprintf(str, msg3, outfile);
            return (str);
        }
        FILE *in = fopen(infile, "rb");
        if (!in) {
            delete out;
            fclose(fp);
            char *str = new char[strlen(msg1) + strlen(infile)];
            sprintf(str, msg1, infile);
            return (str);
        }
        int c;
        while ((c = getc(in)) != EOF) {
            if (out->zio_putc(c) < 0) {
                delete out;
                fclose(fp);
                fclose(in);
                unlink(outfile);
                return (lstring::copy(msg4));
            }
        }
        delete out;
        fclose(fp);
        fclose(in);
    }
    else if (mode == GZIPuncompress) {
        FILE *out = fopen(outfile, "wb");
        if (!out) {
            char *str = new char[strlen(msg1) + strlen(outfile)];
            sprintf(str, msg1, outfile);
            return (str);
        }
        FILE *fp = fopen(infile, "rb");
        if (!fp) {
            fclose(out);
            char *str = new char[strlen(msg1) + strlen(infile)];
            sprintf(str, msg1, infile);
            return (str);
        }
        zio_stream *in = zio_stream::zio_open(fp, "r");
        if (!in) {
            fclose(out);
            fclose(fp);
            char *str = new char[strlen(msg2) + strlen(infile)];
            sprintf(str, msg2, infile);
            return (str);
        }
        if (!in->zio_gzinit()) {
            fclose(out);
            delete in;
            fclose(fp);
            char *str = new char[strlen(msg3) + strlen(infile)];
            sprintf(str, msg3, infile);
            return (str);
        }
        int c;
        while ((c = in->zio_getc()) != EOF) {
            if (putc(c, out) < 0) {
                fclose(out);
                delete in;
                fclose(fp);
                unlink(outfile);
                return (lstring::copy(msg4));
            }
        }
        fclose(out);
        delete in;
        fclose(fp);
    }
    else {
        FILE *out = fopen(outfile, "wb");
        if (!out) {
            char *str = new char[strlen(msg1) + strlen(outfile)];
            sprintf(str, msg1, outfile);
            return (str);
        }
        FILE *in = fopen(infile, "rb");
        if (!in) {
            fclose(out);
            char *str = new char[strlen(msg1) + strlen(infile)];
            sprintf(str, msg1, infile);
            return (str);
        }
        int c;
        while ((c = getc(in)) != EOF) {
            if (putc(c, out) < 0) {
                fclose(out);
                fclose(in);
                unlink(outfile);
                return (lstring::copy(msg4));
            }
        }
        fclose(out);
        fclose(in);
    }
    return (0);
}


//-----------------------------------------------------------------------------
// cCHD interface to table of maps for random access.

SymTab *cCHD::c_index_tab = 0;

namespace {
    // Return true if file is gzipped.  Assumes fp is rewound, does not
    // restore position.
    //
    bool is_gzipped(FILE *fp)
    {
        unsigned char c[4];
        int n = fread(c, 1, 4, fp);
        if (n != 4)
            return (0);
        if (c[0] != GZMAGIC_0 || c[1] != GZMAGIC_1)
            return (false);
        if (c[2] != Z_DEFLATED || (c[3] & RESERVED) != 0)
            return (false);
        return (true);
    }
}


// Build and store a zio_index map for the file associated with the
// CHD, if it is gzipped.  Return true if a map was created, already
// exists, or is not needed, false on error.
//
bool
cCHD::registerRandomMap()
{
#ifdef Z_HAS_RANDOM_MAP
    if (c_filetype != Fgds && c_filetype != Fcgx) {
        // File can't be in gzip format.
        return (true);
    }
    if (c_crc) {
        // We've already called registerRandomMap with this CHD, and a
        // map was associated.
        return (true);
    }

    FILE *fp = large_fopen(c_filename, "rb");
    if (!fp)
        return (false);
    if (!is_gzipped(fp)) {
        // File is not gzipped.
        fclose(fp);
        return (true);
    }

    if (c_index_tab) {
        // Look at the CRC ar the end of the file, and see if there is
        // a match, to avoid building an index.  If the gzipped file
        // has multiple sections, this will fail.  However, this type
        // of gzipped file seems uncommon.

        if (fseeko(fp, -8, SEEK_END) < 0) {
            fclose(fp);
            return (false);
        }
        unsigned char c[4];
        int n = fread(c, 1, 4, fp);
        if (n != 4) {
            fclose(fp);
            return (false);
        }
        unsigned int fcrc = c[0];
        fcrc += (unsigned int)c[1] << 8;
        fcrc += (unsigned int)c[2] << 16;
        fcrc += (unsigned int)c[3] << 24;
#ifdef MAP_DEBUG
printf("file end crc: %x\n", fcrc);
#endif
        zio_index *zi = (zio_index*)SymTab::get(c_index_tab, fcrc);
        if (zi != (zio_index*)ST_NIL) {
            // Already have an index for this file.
            c_crc = zi->crc();
            zi->ref();
            fclose(fp);
#ifdef MAP_DEBUG
printf("assigned an existing map using file end crc\n");
#endif
            return (true);
        }
    }
    rewind(fp);

    zio_index *zi = new zio_index;
    unsigned int span = SPAN;
    if (FIO()->ChdRandomGzip() > 1)
        span *= FIO()->ChdRandomGzip();
    int ret = zi->build_index(fp, span);
    fclose(fp);
    if (ret < 0) {
        delete zi;
        return (false);
    }
    if (!zi->crc()) {
        // Something's foobar, shouldn't happen.
        delete zi;
        return (false);
    }

    if (!c_index_tab)
        c_index_tab = new SymTab(false, false);
    zio_index *ozi = (zio_index*)SymTab::get(c_index_tab, zi->crc());
    if (ozi != (zio_index*)ST_NIL) {
        // Hmmm, already a map for this file, use it.  Our test above
        // failed.
        delete zi;
        c_crc = ozi->crc();
        ozi->ref();
#ifdef MAP_DEBUG
printf("assigned an existing map using real crc\n");
#endif
        return (true);
    }
    c_index_tab->add(zi->crc(), zi, false);
    c_crc = zi->crc();
#ifdef MAP_DEBUG
printf("assigned a new map, crc= %x\n", zi->crc());
#endif

#endif  // Z_HAS_RANDOM_MAP
    return (true);
}


// Destroy a map for the file associated with the CHD, if any.  Return
// true if a map was deleted, false otherwise.
//
bool
cCHD::unregisterRandomMap()
{
#ifdef Z_HAS_RANDOM_MAP
    if (c_filetype != Fgds && c_filetype != Fcgx) {
        // File can't be in gzip format.
        return (false);
    }
    if (!c_index_tab) {
        // No maps exist at all.
        return (false);
    }
    if (!c_crc) {
        // We never assigned a map for this CHD.
        return (false);
    }
    zio_index *zi = (zio_index*)SymTab::get(c_index_tab, c_crc);
    if (zi != (zio_index*)ST_NIL) {
        zi->unref();
        if (zi->refcnt() < 0) {
            c_index_tab->remove(c_crc);
            delete zi;
#ifdef MAP_DEBUG
printf("destroyed a map for crc= %x\n", c_crc);
#endif
            c_crc = 0;
            return (true);
        }
    }
    c_crc = 0;
#endif  // Z_HAS_RANDOM_MAP

    return (false);
}


// Static function.
// Return the map keyed by the crc checksum, if any.
//
zio_index *
cCHD::getRandomMap(unsigned int crc)
{
    if (!c_index_tab)
        return (0);
    zio_index *zi = (zio_index*)SymTab::get(c_index_tab, crc);
    if (zi == (zio_index*)ST_NIL)
        return (0);
    return (zi);
}
// End of cCHD functions


//------------------------------------------------------------------------
// zio_stream methods
//
// Low-level compressed i/o, similar to the gzio interface in zlib.
// Most of this is taken from zlib-1.1.4
//------------------------------------------------------------------------
// interface of the 'zlib' general purpose compression library
// version 1.1.4, March 11th, 2002
//
// Copyright (C) 1995-2002 Jean-loup Gailly and Mark Adler
//
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//
// Jean-loup Gailly        Mark Adler
// jloup@gzip.org          madler@alumni.caltech.edu
//
//
// The data format used by the zlib library is described by RFCs (Request for
// Comments) 1950 to 1952 in the files ftp://ds.internic.net/rfc/rfc1950.txt
// (zlib format), rfc1951.txt (deflate format) and rfc1952.txt (gzip format).
//------------------------------------------------------------------------

#ifdef WIN32
#define OS_CODE  0x0b
#endif
#ifndef OS_CODE
#define OS_CODE  0x03  // assume Unix
#endif


zio_stream::zio_stream(FILE *fp)
{
    z_strm.zalloc = 0;
    z_strm.zfree = 0;
    z_strm.opaque = 0;
    z_strm.next_in = z_inbuf = 0;
    z_strm.next_out = z_outbuf = 0;
    z_strm.avail_in = z_strm.avail_out = 0;
    z_fp = fp;
    z_zfp = 0;
    z_ptr = 0;
    z_startpos = fp ? large_ftell(fp) : 0;
    z_total_in = 0;
    z_total_out = 0;
    z_in_limit = 0;
    z_crc = crc32(0, 0, 0);
    z_file_crc = 0;
    z_avail = 0;
    z_err = Z_OK;
    z_eof = false;
    z_header = false;
    z_mode = 0;
    z_had_header = false;
    z_no_checksum = false;
}


zio_stream::~zio_stream()
{
    if (z_strm.state) {
        if (z_mode == 'w') {
            if (flush(Z_FINISH) == Z_OK && z_header) {
                put_unsigned(z_crc);
                put_unsigned(z_total_in);  // 32 bits!
            }
            deflateEnd(&z_strm);
        }
        else
            inflateEnd(&z_strm);
    }
    delete [] z_inbuf;
    delete [] z_outbuf;
}


// #define CMP_DBG

// Static function.
// In-memory block compression function.
// Args:
//   b      Data to compress.
//   psz    (In) size of uncompressed data (out) compressed block size,
//          not including ofs.
//   trim   If true, reallocate returned buffer to actual size.
//   offs   Leave this many bytes in out buffer before data.
//
unsigned char *
zio_stream::zio_compress(const unsigned char *b, unsigned int *psz, bool trim,
    unsigned int ofs)
{
    z_stream z_strm;
    z_strm.zalloc = 0;
    z_strm.zfree = 0;
    z_strm.opaque = 0;

    int mode = 1 ? Z_BEST_SPEED : Z_DEFAULT_COMPRESSION;
    int err = deflateInit2(&z_strm, mode, Z_DEFLATED,
        -MAX_WBITS, MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY);
    if (err != Z_OK)
        return (0);

#ifdef CMP_DBG
    unsigned int usz = *psz;
#endif
    unsigned int sz = Z_OUTSIZE;
    Byte *z_outbuf = new Byte[sz];
    z_strm.next_out = z_outbuf + ofs;
    z_strm.avail_out = sz - ofs;

    z_strm.next_in = (Bytef*)b;
    z_strm.avail_in = *psz;

    z_strm.total_in = 0;
    z_strm.total_out = 0;
    int flush = Z_NO_FLUSH;
    for (;;) {
        if (z_strm.avail_out == 0) {
            Byte *tmp = new Byte[sz*2];
            unsigned int nb = z_strm.next_out - z_outbuf;
            memcpy(tmp, z_outbuf, nb);
            delete [] z_outbuf;
            z_outbuf = tmp;
            z_strm.next_out = z_outbuf + nb;
            sz *= 2;;
            z_strm.avail_out = sz - nb;
        }
        int z_err = deflate(&z_strm, flush);
        if (z_err == Z_STREAM_END)
            break;
        if (z_strm.avail_out == 0)
            continue;
        if (z_err != Z_OK) {
            delete [] z_outbuf;
            deflateEnd(&z_strm);
#ifdef CMP_DBG
            printf("COMPRESSION FAILED!\n");
#endif
            return (0);
        }
        if (z_strm.avail_in == 0)
            flush = Z_FINISH;
    }
    deflateEnd(&z_strm);
    *psz = z_strm.total_out;

    if (trim) {
        unsigned char *tmp = new unsigned char[z_strm.total_out + ofs];
        memcpy(tmp, z_outbuf, z_strm.total_out + ofs);
        delete [] z_outbuf;
        z_outbuf = tmp;
    }
#ifdef CMP_DBG
    unsigned int csz = z_strm.total_out;
//    printf("comp %d -> %d\n", usz, csz);
    unsigned char *xx = zio_stream::zio_uncompress(z_outbuf + ofs, &csz);
    if (!xx)
        printf("TEST UNCOMPRESS FAILED\n");
    if (usz != csz)
        printf("TEST UNCOMPRESS SIZES DIFFER %d %d\n", usz, csz);
    else if (memcmp(b, xx, usz))
        printf("TEST UNCOMPRESS DATA DIFFER\n");
    delete [] xx;
#endif
    return (z_outbuf);
}


// Static function.
// In-memory block compression function.
// Args:
//   b      Data to uncompress.
//   psz    (In) size of compressed data (out) uncompressed block size,
//          not including ofs.
//   trim   If true, reallocate returned buffer to actual size.
//   offs   Leave this many bytes in out buffer before data.
//
unsigned char *
zio_stream::zio_uncompress(const unsigned char *b, unsigned int *psz,
    bool trim, unsigned int ofs)
{
    z_stream z_strm;
    z_strm.zalloc = 0;
    z_strm.zfree = 0;
    z_strm.opaque = 0;

    static unsigned char z_foo[4];

    int err = inflateInit2(&z_strm, -MAX_WBITS);
    if (err != Z_OK)
        return (0);

    unsigned int sz = Z_OUTSIZE;
    Byte *z_outbuf = new Byte[sz];
    z_strm.next_out = z_outbuf + ofs;
    z_strm.avail_out = sz - ofs;

    z_strm.next_in = (Byte*)b;
    z_strm.avail_in = *psz;

    z_strm.total_in = 0;
    z_strm.total_out = 0;
    for (;;) {
        if (z_strm.avail_out == 0) {
            Byte *tmp = new Byte[sz*2];
            unsigned int nb = z_strm.next_out - z_outbuf;
            memcpy(tmp, z_outbuf, nb);
            delete [] z_outbuf;
            z_outbuf = tmp;
            z_strm.next_out = z_outbuf + nb;
            sz *= 2;;
            z_strm.avail_out = sz - nb;
        }
        int z_err = inflate(&z_strm, Z_NO_FLUSH);
        if (z_err == Z_STREAM_END)
            break;
        if (z_strm.avail_out == 0)
            continue;
        if (z_err != Z_OK) {
            if (z_err == Z_BUF_ERROR && z_strm.avail_in == 0) {
                // Sometimes this happens, inflate needs to read another
                // byte (or two?) to recognize the stream end.
                z_strm.next_in = z_foo;
                z_strm.avail_in = 2;
                continue;
            }
            delete [] z_outbuf;
            inflateEnd(&z_strm);
            return (0);
        }
    }
    inflateEnd(&z_strm);
    *psz = z_strm.total_out;
    if (trim) {
        unsigned char *tmp = new unsigned char[z_strm.total_out + ofs];
        memcpy(tmp, z_outbuf, z_strm.total_out + ofs);
        delete [] z_outbuf;
        return (tmp);
    }
    return (z_outbuf);
}


// Static function.
// Open a compression stream - must use same mode as fp!  When
// creating or reading .gz (gzipped) files, zio_stream::zio_gzinit()
// must be called.  Otherwise, it is assumed that an embedded
// compressed block, such as used in OASIS files, is being opened.
// This has no gz header.
//
zio_stream *
zio_stream::zio_open(FILE *fp, const char *mode)
{
    if (!mode || !fp)
        return (0);

    zio_stream *s = new zio_stream(fp);

    if (*mode == 'r')
        s->z_mode = 'r';
    else if (*mode == 'w' || *mode == 'a')
        s->z_mode = 'w';
    else {
        delete s;
        return (0);
    }

    if (s->z_mode == 'w') {
        // For gzip, windowBits is passed < 0 to suppress zlib header.
        int err = deflateInit2(&s->z_strm, Z_DEFAULT_COMPRESSION, Z_DEFLATED,
            -MAX_WBITS, MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY);

        if (err != Z_OK) {
            delete s;
            return (0);
        }
        s->z_strm.next_out = s->z_outbuf = new Byte[Z_OUTSIZE];
    }
    else {
        s->z_strm.next_in  = s->z_inbuf = new Byte[Z_INSIZE];

        // For gzip, windowBits is passed < 0 to tell that there is no
        // zlib header.  Note that in this case inflate *requires* an
        // extra "dummy" byte after the compressed stream in order to
        // complete decompression and return Z_STREAM_END.  Here the
        // gzip CRC32 ensures that 4 bytes are present after the
        // compressed stream.
        int err = inflateInit2(&s->z_strm, -MAX_WBITS);

        if (err != Z_OK) {
            delete s;
            return (0);
        }
    }
    s->z_strm.avail_out = Z_OUTSIZE;
    return (s);
}


// As above, but takes an sFilePtr as an input source.  If the source
// is a zstream, only reading is possible.
// 
// The inbytes, if nonzero, will limit the number of bytes read from
// the input stream to the given value.  This prevents reading too
// many bytes and having to back-track, which can be a performance
// disaster if the source is a zstream.
//
zio_stream *
zio_stream::zio_open(sFilePtr *zfp, const char *mode, int64_t inbytes)
{
    if (!mode || !zfp)
        return (0);
    if (zfp->fp) {
        zio_stream *s = zio_open(zfp->fp, mode);
        if (s)
            s->z_in_limit = inbytes;
        return (s);
    }
 
    // This is a hack for gzipped OASIS files that contain CBLOCK
    // records.  This applies only while reading.
    
    if (strcmp(mode, "r") && strcmp(mode, "rb"))
        return (0);
    
    zio_stream *s = new zio_stream(0);
    s->z_zfp = zfp;
    s->z_startpos = zfp->z_tell();
    s->z_mode = 'r';
    s->z_strm.next_in  = s->z_inbuf = new Byte[Z_INSIZE];
    s->z_in_limit = inbytes;
    
    // windowBits is passed < 0 to tell that there is no zlib
    // header.  Note that in this case inflate *requires* an extra
    // "dummy" byte after the compressed stream in order to
    // complete decompression and return Z_STREAM_END.  Here the
    // gzip CRC32 ensures that 4 bytes are present after the
    // compressed stream.
  
    int err = inflateInit2(&s->z_strm, -MAX_WBITS);
    if (err != Z_OK) {   
        delete s;
        return (0);
    }
    s->z_strm.avail_out = Z_OUTSIZE;
    return (s);
}


// Call this after zio_open to use with gzip files.
//
bool
zio_stream::zio_gzinit()
{
    if (z_mode == 'w') {
        // Write a very simple .gz header:
        fprintf(z_fp, "%c%c%c%c%c%c%c%c%c%c", GZMAGIC_0, GZMAGIC_1,
             Z_DEFLATED, 0, 0,0,0,0, 0, OS_CODE);
        z_startpos = 10;
        z_header = true;
    }
    else {
        if (!check_gz_header()) {
            rewind(z_fp);
            return (false);
        }
        z_startpos = large_ftell(z_fp) - z_strm.avail_in;
    }
    return (true);
}


int
zio_stream::zio_read(void *buf, unsigned int len)
{
    if (z_avail < 0 || !buf)
        return (-1);
    unsigned int req = len;
    char *cp = (char*)buf;
    while (len) {
        if (z_avail == 0) {
            if (!z_outbuf)
                z_outbuf = new Byte[Z_OUTSIZE];
            z_avail = read_core(z_outbuf, Z_OUTSIZE);
            if (z_avail < 0)
                return (-1);
            if (z_avail == 0)
                break;
            z_ptr = (char*)z_outbuf;
        }
        *cp++ = *z_ptr++;
        z_avail--;
        len--;
    }
    return (req - len);
}


int
zio_stream::zio_write(const void *buf, unsigned int len)
{
    if (z_mode != 'w')
        return (Z_STREAM_ERROR);

    z_strm.next_in = (Bytef*)buf;
    z_strm.avail_in = len;

    z_strm.total_in = 0;
    z_strm.total_out = 0;
    while (z_strm.avail_in != 0) {

        if (z_strm.avail_out == 0) {

            z_strm.next_out = z_outbuf;
            if (fwrite(z_outbuf, 1, Z_OUTSIZE, z_fp) != Z_OUTSIZE) {
                z_err = Z_ERRNO;
                break;
            }
            z_strm.avail_out = Z_OUTSIZE;
        }
        z_err = deflate(&z_strm, Z_NO_FLUSH);
        if (z_err != Z_OK)
            break;
    }
    z_crc = crc32(z_crc, (const Bytef*)buf, len);
    z_total_in += z_strm.total_in;
    z_total_out += z_strm.total_out;
    return (len - z_strm.avail_in);
}


int
zio_stream::zio_rewind()
{
    if (z_mode != 'r')
        return (-1);

    z_err = Z_OK;
    z_eof = 0;
    z_strm.avail_in = 0;
    z_strm.next_in = z_inbuf;
    z_crc = crc32(0, 0, 0);

    inflateReset(&z_strm);
    z_total_in = 0;
    z_total_out = 0;
    z_avail = 0;
    z_ptr = 0;
    if (z_zfp)
        return (z_zfp->z_seek(z_startpos, SEEK_SET));
    return (fseeko(z_fp, z_startpos, SEEK_SET));
}


int
zio_stream::zio_seek(int64_t offset, int whence)
{
    if (whence == SEEK_END || z_err == Z_ERRNO || z_err == Z_DATA_ERROR)
        return (-1);

    z_eof = false;
    z_err = Z_OK;

    if (z_mode == 'w') {
        if (whence == SEEK_SET)
            offset -= z_total_in;
        if (offset < 0)
            return (-1);

        // At this point, offset is the number of zero bytes to write.
        if (!z_inbuf) {
            z_inbuf = new Byte[Z_INSIZE]; // for seeking
            memset(z_inbuf, 0, Z_INSIZE);
        }

        while (offset > 0)  {
            int size = Z_INSIZE;
            if (offset < Z_INSIZE)
                size = offset;

            size = zio_write(z_inbuf, size);
            if (size <= 0)
                return -1;
            offset -= size;
        }
        return (0);
    }

    // Rest of function is for reading only.

    // Present absolute position.
    int64_t posn = z_total_out - z_avail;
    if (whence == SEEK_CUR)
        offset += posn;
    if (offset < 0)
        return (-1);
    if (offset == posn)
        return (0);

    // If we jump, the running checksum becomes bogus.
    z_no_checksum = true;

#ifdef MAP_DEBUG
printf("seeking %lld %d, file crc %x\n", offset, whence, z_file_crc);
#endif
    if (z_file_crc) {
        zio_index *zi = cCHD::getRandomMap(z_file_crc);
#ifdef MAP_DEBUG
if (zi)
printf("found map\n");
#endif
        if (zi)
            return (seek_to(offset, zi));
    }

    // Offset of start of current in-memory block.
    int64_t bposn = z_total_out - z_strm.total_out;
    if (offset < bposn) {
        // Reposition before current block, have to read from the
        // beginning.
        if (zio_rewind() < 0)
            return (-1);
    }
    else if (offset < z_total_out) {
        // Reposition in current block.
        z_avail -= (offset - posn);
        z_ptr += (offset - posn);
        return (0);
    }
    else {
        // Reposition after current block.
        offset -= z_total_out;
    }

    if (offset && !z_outbuf)
        z_outbuf =  new Byte[Z_OUTSIZE];
    while (offset > 0)  {
        int size = Z_OUTSIZE;
        if (offset < Z_OUTSIZE)
            size = offset;

        size = read_core(z_outbuf, size);
        if (size <= 0)
            return (-1);
        offset -= size;
    }
    z_avail = 0;
    z_ptr = 0;
    return (0);
}


//
// Remaining functions are private.
//

// Use the index (possibly) to reposition the stream read point to
// offset.  Return value is 0, or negative on error.  (Z_DATA_ERROR or
// Z_MEM_ERROR).  This function should not return a data error unless
// the file was modified since the index was generated.  This may also
// return Z_ERRNO if there is an error on reading or seeking the input
// file.
//
int
zio_stream::seek_to(int64_t offset, zio_index *index)
{
    z_eof = false;
    z_err = Z_OK;

    if (offset < 0)
        return (-1);

    // Find current position.
    int64_t cur_posn = z_total_out - z_avail;
    if (offset == cur_posn)
        return (0);

    // Offset of start of current in-memory block.
    int64_t bposn = z_total_out - z_strm.total_out;

    bool backup = (offset < bposn);
    if (!backup && offset < z_total_out) {
        // Reposition in current block, the happy case.
        z_avail -= (offset - cur_posn);
        z_ptr += (offset - cur_posn);
#ifdef MAP_DEBUG
printf("REPOSITION %lld\n", offset - cur_posn);
#endif
        return (0);
    }

    // Find where in stream to start.
    zio_index::point_t *here = index ? index->get_point(offset) : 0;

// Inflating to this many bytes is time equivalent to the overhead of
// resetting inflate from a point (a guess).
#define OVERHEAD 65536

    if (backup && (!here || here->out == 0)) {
        // Rewind stream and read to new position.
        if (zio_rewind() < 0)
            return (-1);
        cur_posn = 0;
    }
    else if (!backup &&
            (!here || (z_total_out + OVERHEAD >= (int64_t)here->out))) {
        // We're already closer than the index (or no index was
        // passed), so just read ahead.
#ifdef MAP_DEBUG
printf("SEEKING from current\n");
#endif
        offset -= z_total_out;
    }
#ifdef Z_HAS_RANDOM_MAP
    else {
        // Do the map thing.
#ifdef MAP_DEBUG
printf("SEEKING from table %lld %lld %d\n", offset, here->out, here->bits);
#endif
        z_total_in = here->in;
        z_total_out = here->out;

        z_strm.zalloc = 0;
        z_strm.zfree = 0;
        z_strm.opaque = 0;
        z_strm.avail_in = 0;
        z_strm.next_in = 0;
        int ret = inflateInit2(&z_strm, -MAX_WBITS);
        if (ret != Z_OK)
            return (ret);

        // If we ever seek randomly, we'll ignore the checksum, since the
        // calculation is almost certainly bogus.
        z_had_header = false;

        if (z_zfp)
            ret = z_zfp->z_seek(here->in - (here->bits ? 1 : 0), SEEK_SET);
        else
            ret = fseeko(z_fp, here->in - (here->bits ? 1 : 0), SEEK_SET);
        if (ret == -1)
            return (ret);
        if (here->bits) {
            if (z_zfp)
                ret = z_zfp->z_getc();
            else
                ret = getc(z_fp);
            if (ret == -1) {
                ret = ferror(z_fp) ? Z_ERRNO : Z_DATA_ERROR;
                return (ret);
            }
            inflatePrime(&z_strm, here->bits, ret >> (8 - here->bits));
        }
        inflateSetDictionary(&z_strm, here->window, Z_WINSIZE);

        offset -= here->out;
    }
#endif  // Z_HAS_RANDOM_MAP

    if (offset && !z_outbuf)
        z_outbuf =  new Byte[Z_OUTSIZE];
    while (offset > 0)  {
        int size = Z_OUTSIZE;
        if (offset < Z_OUTSIZE)
            size = offset;

        size = read_core(z_outbuf, size);
        if (size <= 0)
            return (-1);
        offset -= size;
    }
    z_avail = 0;
    z_ptr = 0;
    return (0);
}


int
zio_stream::read_core(void *buf, unsigned int len)
{
    if (z_mode != 'r')
        return (Z_STREAM_ERROR);
    if (z_err == Z_DATA_ERROR || z_err == Z_ERRNO)
        return (-1);
    if (z_err == Z_STREAM_END)
        return (0);  // EOF

    Bytef *start = (Bytef*)buf; // starting point for crc computation
    z_strm.next_out = (Bytef*)buf;
    z_strm.avail_out = len;

    z_strm.total_in = 0;
    z_strm.total_out = 0;
    while (z_strm.avail_out != 0) {

        if (z_strm.avail_in == 0 && !z_eof) {
            // Note that z_in_limit does not really limit the input
            // bytes read.  If the value is too small, normal buffering
            // is resumed. 
        
            if (z_in_limit > 0 && z_in_limit < Z_INSIZE) {
                if (z_zfp)
                    z_strm.avail_in = z_zfp->z_read(z_inbuf, 1, z_in_limit);
                else
                    z_strm.avail_in = fread(z_inbuf, 1, z_in_limit, z_fp);
            }
            else {
                if (z_zfp)
                    z_strm.avail_in = z_zfp->z_read(z_inbuf, 1, Z_INSIZE); 
                else
                    z_strm.avail_in = fread(z_inbuf, 1, Z_INSIZE, z_fp);  
                if (z_in_limit > 0)
                    z_in_limit -= Z_INSIZE;
            }

            if (z_strm.avail_in == 0) {
                z_eof = true;
                if (ferror(z_fp)) {
                    z_err = Z_ERRNO;
                    break;
                }
            }
            z_strm.next_in = z_inbuf;
        }
        z_err = inflate(&z_strm, Z_NO_FLUSH);
        if (z_err == Z_STREAM_END) {
            z_eof = false;
#ifdef MAP_DEBUG
printf("read: end of stream\n");
#endif
            if (z_had_header && !z_no_checksum) {
                // Check CRC
                z_crc = crc32(z_crc, start, z_strm.next_out - start);
                start = z_strm.next_out;
                unsigned int crc = get_unsigned();
                if (crc != z_crc) {
#ifdef MAP_DEBUG
printf("read: checksum error\n");
#endif
                    z_err = Z_DATA_ERROR;
                }
                else if (!z_file_crc) {
#ifdef MAP_DEBUG
printf("read: assigned file crc\n");
#endif
                    z_file_crc = crc;
                }
            }
            if (z_err == Z_STREAM_END) {
                z_total_in += z_strm.total_in;
                z_total_out += z_strm.total_out;
                return (len - z_strm.avail_out);
            }
        }
        if (z_err != Z_OK || z_eof) {
            z_total_in += z_strm.total_in;
            z_total_out += z_strm.total_out;
            return (-1);
        }
    }
    z_crc = crc32(z_crc, start, z_strm.next_out - start);
    z_total_in += z_strm.total_in;
    z_total_out += z_strm.total_out;
    return (len - z_strm.avail_out);
}


int
zio_stream::flush(int flsh)
{
    if (z_mode != 'w')
        return (Z_STREAM_ERROR);

    z_strm.avail_in = 0; // should be zero already

    z_strm.total_in = 0;
    z_strm.total_out = 0;
    int done = 0;
    for (;;) {
        uInt len = Z_OUTSIZE - z_strm.avail_out;

        if (len != 0) {
            if ((uInt)fwrite(z_outbuf, 1, len, z_fp) != len) {
                z_err = Z_ERRNO;
                return (Z_ERRNO);
            }
            z_strm.next_out = z_outbuf;
            z_strm.avail_out = Z_OUTSIZE;
        }
        if (done)
            break;
        z_err = deflate(&z_strm, flsh);

        // Ignore the second of two consecutive flushes
        if (len == 0 && z_err == Z_BUF_ERROR)
            z_err = Z_OK;

        // deflate has finished flushing only when it hasn't used up
        // all the available space in the output buffer:
        //
        done = (z_strm.avail_out != 0 || z_err == Z_STREAM_END);

        if (z_err != Z_OK && z_err != Z_STREAM_END)
            break;
    }
    z_total_in += z_strm.total_in;
    z_total_out += z_strm.total_out;
    return (z_err == Z_STREAM_END ? Z_OK : z_err);
}


int
zio_stream::get_byte()
{
    if (z_eof)
        return (EOF);
    if (z_strm.avail_in == 0) {
        if (z_zfp)
            z_strm.avail_in = z_zfp->z_read(z_inbuf, 1, Z_INSIZE);
        else
            z_strm.avail_in = fread(z_inbuf, 1, Z_INSIZE, z_fp);
        if (z_strm.avail_in == 0) {
            z_eof = true;
            if (ferror(z_fp))
                z_err = Z_ERRNO;
            return (EOF);
        }
        z_strm.next_in = z_inbuf;
    }
    z_strm.avail_in--;
    return (*(z_strm.next_in)++);
}


unsigned int
zio_stream::get_unsigned()
{
    unsigned int x = (unsigned int)get_byte();
    x += ((unsigned int)get_byte()) << 8;
    x += ((unsigned int)get_byte()) << 16;
    int c = get_byte();
    if (c == EOF)
        z_err = Z_DATA_ERROR;
    x += ((unsigned int)c) << 24;
    return (x);
}


void
zio_stream::put_unsigned(unsigned int x)
{
    for (int n = 0; n < 4; n++) {
        putc((int)(x & 0xff), z_fp);
        x >>= 8;
    }
}


bool
zio_stream::check_gz_header()
{
    // Check the gzip magic header
    if (get_byte() != GZMAGIC_0 || get_byte() != GZMAGIC_1)
        return (false);

    int method = get_byte();
    int flags = get_byte();
    if (method != Z_DEFLATED || (flags & RESERVED) != 0) {
        z_err = Z_DATA_ERROR;
        return (false);
    }

    // Indicate that the header has been read, so we know to
    // compare checksum at end of read.
    z_had_header = true;

    // Discard time, xflags and OS code:
    for (int i = 0; i < 6; i++)
        get_byte();

    if ((flags & EXTRA_FIELD) != 0) { // skip the extra field
        int len  =  get_byte();
        len += get_byte() << 8;
        // len is garbage if EOF but the loop below will quit anyway
        while (len-- && get_byte() != EOF) ;
    }
    if ((flags & ORIG_NAME) != 0) { // skip the original file name
        int c;
        while ((c = get_byte()) != 0 && c != EOF) ;
    }
    if ((flags & COMMENT) != 0) {   // skip the .gz file comment
        int c;
        while ((c = get_byte()) != 0 && c != EOF) ;
    }
    if ((flags & HEAD_CRC) != 0) {  // skip the header crc
        get_byte();
        get_byte();
    }
    z_err = z_eof ? Z_DATA_ERROR : Z_OK;
    return (z_err == Z_OK);
}
// End of zio_stream functions.


//-----------------------------------------------------------------------------
// zio_index -- Table for random access into a gzipped file.
//
// This is based on logic illustrated in the zran.c file included in
// tht examples of the zlib-1.2.5 distibution. 

/* zran.c -- example of zlib/gzip stream indexing and random access
 * Copyright (C) 2005 Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h
   Version 1.0  29 May 2005  Mark Adler */

/* Illustrate the use of Z_BLOCK, inflatePrime(), and inflateSetDictionary()
   for random access of a compressed file.  A file containing a zlib or gzip
   stream is provided on the command line.  The compressed stream is decoded in
   its entirety, and an index built with access points about every SPAN bytes
   in the uncompressed output.  The compressed file is left open, and can then
   be read randomly, having to decompress on the average SPAN/2 uncompressed
   bytes before getting to the desired block of data.

   An access point can be created at the start of any deflate block, by saving
   the starting file offset and bit of that block, and the 32K bytes of
   uncompressed data that precede that block.  Also the uncompressed offset of
   that block is saved to provide a referece for locating a desired starting
   point in the uncompressed stream.  build_index() works by decompressing the
   input zlib or gzip stream a block at a time, and at the end of each block
   deciding if enough uncompressed data has gone by to justify the creation of
   a new access point.  If so, that point is saved in a data structure that
   grows as needed to accommodate the points.

   To use the index, an offset in the uncompressed data is provided, for which
   the latest accees point at or preceding that offset is located in the index.
   The input file is positioned to the specified location in the index, and if
   necessary the first few bits of the compressed data is read from the file.
   inflate is initialized with those bits and the 32K of uncompressed data, and
   the decompression then proceeds until the desired offset in the file is
   reached.  Then the decompression continues to read the desired uncompressed
   data from the file.

   Another approach would be to generate the index on demand.  In that case,
   requests for random access reads from the compressed data would try to use
   the index, but if a read far enough past the end of the index is required,
   then further index entries would be generated and added.

   There is some fair bit of overhead to starting inflation for the random
   access, mainly copying the 32K byte dictionary.  So if small pieces of the
   file are being accessed, it would make sense to implement a cache to hold
   some lookahead and avoid many calls to extract() for small lengths.

   Another way to build an index would be to use inflateCopy().  That would
   not be constrained to have access points at block boundaries, but requires
   more memory per access point, and also cannot be saved to file due to the
   use of pointers in the state.  The approach here allows for storage of the
   index in a file.
 */


// Make one entire pass through the compressed stream and build the
// index, with access points about every span bytes of uncompressed
// output -- span is chosen to balance the speed of random access
// against the memory requirements of the list, about 32K bytes per
// access point.  Note that data after the end of the first zlib or
// gzip stream in the file is ignored.  This returns the number of
// access points on success (>= 1), Z_MEM_ERROR for out of memory,
// Z_DATA_ERROR for an error in the input file, or Z_ERRNO for a file
// read error.
//
int
zio_index::build_index(FILE *in, unsigned int span)
{
#ifdef Z_HAS_RANDOM_MAP
    // Initialize inflate.
    z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;
    int ret = inflateInit2(&strm, 47);  // Automatic zlib or gzip decoding.
    if (ret != Z_OK)
        return (ret);

    // Inflate the input, maintain a sliding window, and build the
    // index -- this also validates the integrity of the compressed
    // data using the check information at the end of the gzip or zlib
    // stream.

    // Our own total counters to avoid 4GB limit.
    uint64_t totin = 0, totout = 0;

    // Last access point totout value.
    uint64_t last = 0;

    unsigned char input[Z_INSIZE];
    unsigned char window[Z_WINSIZE];

    strm.avail_out = 0;
    do {
        // Get some compressed data from input file.
        strm.avail_in = fread(input, 1, Z_INSIZE, in);
        if (ferror(in)) {
            inflateEnd(&strm);
            return (Z_ERRNO);
        }
        if (strm.avail_in == 0) {
            inflateEnd(&strm);
            return (Z_DATA_ERROR);
        }
        strm.next_in = input;

        // Process all of that, or until end of stream.
        do {
            // Reset sliding window if necessary.
            if (strm.avail_out == 0) {
                strm.avail_out = Z_WINSIZE;
                strm.next_out = window;
            }

            // Inflate until out of input, output, or at end of block,
            // update the total input and output counters.
            totin += strm.avail_in;
            totout += strm.avail_out;
            ret = inflate(&strm, Z_BLOCK);      // Return at end of block.
            totin -= strm.avail_in;
            totout -= strm.avail_out;
            if (ret == Z_NEED_DICT)
                ret = Z_DATA_ERROR;
            if (ret == Z_MEM_ERROR || ret == Z_DATA_ERROR) {
                inflateEnd(&strm);
                return (ret);
            }
            if (ret == Z_STREAM_END) {
                // Done, grab the checksum.  In this mode, we're done
                // with the input buffer, and the file ptr has been
                // advanced past the two trailing ints.
                unsigned char c[4];
                if (fseeko(in, -8, SEEK_CUR) >= 0 && fread(c, 1, 4, in) == 4) {
                    zi_crc = c[0];
                    zi_crc += (unsigned int)c[1] << 8;
                    zi_crc += (unsigned int)c[2] << 16;
                    zi_crc += (unsigned int)c[3] << 24;
                }
                break;
            }

            // If at end of block, consider adding an index entry
            // (note that if data_type indicates an end-of-block, then
            // all of the uncompressed data from that block has been
            // delivered, and none of the compressed data after that
            // block has been consumed, except for up to seven bits)
            // -- the totout == 0 provides an entry point after the
            // zlib or gzip header, and assures that the index always
            // has at least one access point; we avoid creating an
            // access point after the last block by checking bit 6 of
            // data_type.

            if ((strm.data_type & 128) && !(strm.data_type & 64) &&
                    (totout == 0 || totout - last > span)) {
                if (!addpoint(strm.data_type & 7, totin, totout,
                        strm.avail_out, window)) {
                    inflateEnd(&strm);
                    return (Z_MEM_ERROR);
                }
                last = totout;
            }
        } while (strm.avail_in != 0);
    } while (ret != Z_STREAM_END);

    // Clean up and return (release unused entries in list).
    inflateEnd(&strm);

    point_t *tmp = new point_t[zi_have];
    memcpy(tmp, zi_list, zi_have*sizeof(point_t));
    delete [] zi_list;
    zi_list = tmp;
    zi_size = zi_have;
    return (zi_size);
#else
    (void)in;
    (void)span;
    return (-1);
#endif  // Z_HAS_RANDOM_MAP
}


#define ZI_MAGIC_STRING "Xic gzip file map 1.0"

// Write a file representing the zio_index struct.  Format is
// ZI_MAGIC_STRING
// crc checksum
// {
// out (8 bytes)
// in  (8 buytes)
// bits (4 bytes)
// window (32768 bytes)
// } repeat 
//
bool
zio_index::to_file(const char *filename)
{
    if (!zi_list)
        return (false);

    FILE *fp = fopen(filename, "wb");
    if (!fp)
        return (false);
    unsigned int n = strlen(ZI_MAGIC_STRING);
    if (fwrite(ZI_MAGIC_STRING, 1, n, fp) != n) {
        fclose(fp);
        return (false);
    }
    fputc(zi_crc, fp);
    fputc(zi_crc >> 8, fp);
    fputc(zi_crc >> 16, fp);
    fputc(zi_crc >> 24, fp);
    for (int i = 0; i < zi_have; i++) {
        point_t &p = zi_list[i];
        fputc(p.out, fp);
        fputc(p.out >> 8, fp);
        fputc(p.out >> 16, fp);
        fputc(p.out >> 24, fp);
        fputc(p.out >> 32, fp);
        fputc(p.out >> 40, fp);
        fputc(p.out >> 48, fp);
        fputc(p.out >> 56, fp);
        fputc(p.in, fp);
        fputc(p.in >> 8, fp);
        fputc(p.in >> 16, fp);
        fputc(p.in >> 24, fp);
        fputc(p.in >> 32, fp);
        fputc(p.in >> 40, fp);
        fputc(p.in >> 48, fp);
        fputc(p.in >> 56, fp);
        fputc(p.bits, fp);
        fputc(p.bits >> 8, fp);
        fputc(p.bits >> 16, fp);
        fputc(p.bits >> 24, fp);
        if (fwrite(p.window, 1, Z_WINSIZE, fp) != Z_WINSIZE) {
            fclose(fp);
            return (false);
        }
    }
    fclose (fp);
    return (true);
}


// Read in a file representing the zio_index.  Any existing list is
// cleared.
//
bool
zio_index::from_file(const char *filename)
{
    FILE *fp = fopen(filename, "rb");
    if (!fp)
        return (false);

    unsigned char buf[32];
    unsigned int n = strlen(ZI_MAGIC_STRING);
    if (fread(buf, 1, n, fp) != n) {
        fclose(fp);
        return (false);
    }
    if (memcmp(ZI_MAGIC_STRING, buf, n)) {
        fclose(fp);
        return (false);
    }

    if (fread(buf, 1, 4, fp) != 4) {
        fclose(fp);
        return (false);
    }
    zi_crc = buf[0];
    zi_crc += (unsigned int)buf[1] << 8;
    zi_crc += (unsigned int)buf[2] << 16;
    zi_crc += (unsigned int)buf[3] << 25;
    delete [] zi_list;
    zi_list = 0;
    zi_have = 0;
    zi_size = 0;

    // How many records?
    int64_t posn = ftello(fp);
    if (fseeko(fp, 0, SEEK_END) < 0) {
        fclose(fp);
        return (false);
    }
    int64_t epos = ftello(fp);
    int rsz = 8 + 8 + 4 + Z_WINSIZE;
    int nrec = (epos - posn)/rsz;
    if (nrec <= 0) {
        fclose(fp);
        return (false);
    }
    if (fseeko(fp, posn, SEEK_SET) < 0) {
        fclose(fp);
        return (false);
    }
    zi_list = new point_t[nrec];
    zi_size = nrec;
    zi_have = nrec;
    for (int i = 0; i < nrec; i++) {
        if (fread(buf, 1, 8, fp) != 8) {
            fclose(fp);
            return (false);
        }
        zi_list[i].out = buf[0];
        zi_list[i].out += (uint64_t)buf[1] << 8;
        zi_list[i].out += (uint64_t)buf[2] << 16;
        zi_list[i].out += (uint64_t)buf[3] << 24;
        zi_list[i].out += (uint64_t)buf[4] << 32;
        zi_list[i].out += (uint64_t)buf[5] << 40;
        zi_list[i].out += (uint64_t)buf[6] << 48;
        zi_list[i].out += (uint64_t)buf[7] << 56;
        if (fread(buf, 1, 8, fp) != 8) {
            fclose(fp);
            return (false);
        }
        zi_list[i].in = buf[0];
        zi_list[i].in += (uint64_t)buf[1] << 8;
        zi_list[i].in += (uint64_t)buf[2] << 16;
        zi_list[i].in += (uint64_t)buf[3] << 24;
        zi_list[i].in += (uint64_t)buf[4] << 32;
        zi_list[i].in += (uint64_t)buf[5] << 40;
        zi_list[i].in += (uint64_t)buf[6] << 48;
        zi_list[i].in += (uint64_t)buf[7] << 56;
        if (fread(buf, 1, 4, fp) != 4) {
            fclose(fp);
            return (false);
        }
        zi_list[i].bits = buf[0];
        zi_list[i].bits += (unsigned int)buf[1] << 8;
        zi_list[i].bits += (unsigned int)buf[2] << 16;
        zi_list[i].bits += (unsigned int)buf[3] << 24;
        if (fread(zi_list[i].window, 1, Z_WINSIZE, fp) != Z_WINSIZE) {
            fclose(fp);
            return (false);
        }
    }
    fclose(fp);
    return (true);
}


// Add an entry to the access point list.
//
bool
zio_index::addpoint(int bits, uint64_t in, uint64_t out, unsigned left,
    unsigned char *window)
{
    // If list is empty, create it (start with PT_BLKSIZE points).
    if (!zi_list) {
        zi_list = new point_t[PT_BLKSIZE];
        zi_size = PT_BLKSIZE;
        zi_have = 0;
        zi_data = new pt_data_t(0);
    }
    else if (zi_have == zi_size) {
        // List is full, reallocate.
        zi_size += zi_size;
        point_t *tmp = new point_t[zi_size];
        memcpy(tmp, zi_list, zi_have*sizeof(point_t));
        delete [] zi_list;
        zi_list = tmp;
    }

    // Fill in entry and increment count.
    point_t *next = zi_list + zi_have;
    next->out = out;
    next->in = in;
    next->bits = bits;
    if (zi_data->d_blk == PT_BLKSIZE)
        zi_data = new pt_data_t(zi_data);
    next->window = zi_data->d_data + Z_WINSIZE * zi_data->d_blk++;
    if (left)
        memcpy(next->window, window + Z_WINSIZE - left, left);
    if (left < Z_WINSIZE)
        memcpy(next->window + left, window, Z_WINSIZE - left);
    zi_have++;

    return (true);
}


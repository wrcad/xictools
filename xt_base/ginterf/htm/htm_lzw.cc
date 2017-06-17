
/*=======================================================================*
 *                                                                       *
 *  XICTOOLS Integrated Circuit Design System                            *
 *  Copyright (c) 2005 Whiteley Research Inc, all rights reserved.       *
 *                                                                       *
 * MOZY html viewer application files                                    *
 *                                                                       *
 * Based on previous work identified below.                              *
 *-----------------------------------------------------------------------*
 * This file is part of the gtkhtm widget library.  The gtkhtm library
 * was derived from the gtk-xmhtml library by:
 *
 *   Stephen R. Whiteley  <stevew@wrcad.com>
 *   Whiteley Research Inc.
 *-----------------------------------------------------------------------*
 *  The gtk-xmhtml widget was derived from the XmHTML library by
 *  Miguel de Icaza  <miguel@nuclecu.unam.mx> and others from the GNOME
 *  project.
 *  11/97 - 2/98
 *-----------------------------------------------------------------------*
 * Author:  newt
 * (C)Copyright 1995-1996 Ripley Software Development
 * All Rights Reserved
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *-----------------------------------------------------------------------*
 * $Id: htm_lzw.cc,v 1.8 2014/02/15 23:14:17 stevew Exp $
 *-----------------------------------------------------------------------*/

#include "htm_lzw.h"
#include "htm_string.h"
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define INIT_BITS       9                   // minimum compress codeSize
#define MAX_LZW_BITS    12                  // maximum GIF LZW codeSize
#define MAX_BITS        13                  // maximum compress codeSize
#define MAX_LZW_CODE    (1<<MAX_LZW_BITS)   // maximum code GIF signal value
#define MAX_CODE        (1<<MAX_BITS)       // maximum code signal value
#define FIRST           257                 // first free slot in compress
#define CLEAR           256                 // compress clearcode signal

// Note:  we set the maximum bits available for compress to 13 instead
// of 12.  This ensures that the compress code storage keeps in sync
// with the GIF LZW codes when the gif code size is less than 8.
// Using 12 bits for compress would overflow the compress tables
// before the GIF clearCode signal is received.

#define MAXCODE(n_bits) ((1L << (n_bits)) - 1)

namespace {
    // bit packing masks
    const unsigned char lmask[9] =
        {0xff, 0xfe, 0xfc, 0xf8, 0xf0, 0xe0, 0xc0, 0x80, 0x00};
    const unsigned char rmask[9] =
        {0x00, 0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xff};
}


// Allocate a new stream.
//
lzwStream::lzwStream(ImageBuffer *ib, const char *zCmd)
{
    lz_zPipe                = 0;
    lz_f                    = 0;
    strcpy(lz_zCmd, zCmd != 0 ? zCmd : "uncompress");
    strcat(lz_zCmd, "  ");
    lz_zName                = lz_zCmd + strlen(lz_zCmd);
    lz_error                = 0;
    lz_uncompressed         = 0;
    lz_ib                   = ib;

    memset(lz_accum, 0, sizeof(lz_accum));
    lz_acount               = 0;

    memset(lz_buf, 0, sizeof(lz_buf));
    lz_curBit               = 0;
    lz_lastBit              = 0;
    lz_lastByte             = 0;
    lz_done                 = 0;
    lz_nextCode             = 0;

    lz_codeSize             = 0;
    lz_codeBits             = 0;
    lz_clearCode            = 0;
    lz_endCode              = 0;
    lz_maxCode              = 0;
    lz_maxCodeSize          = 0;
    memset(lz_outBuf, 0, sizeof(lz_outBuf));

    lz_offset               = 0;
    lz_freeEntry            = 0;
    lz_n_bits               = 0;
    lz_maxcode8             = 0;
    lz_clearFlag            = 0;

    readOK                  = 0;
    getData                 = 0;

    lz_err_msg              = 0;
}


lzwStream::~lzwStream()
{
    if (lz_zPipe)
        fclose(lz_zPipe);
    if (lz_f)
        fclose(lz_f);
    if (lz_err_msg)
        delete [] lz_err_msg;
    unlink(lz_zName);
}


// LZW decoder driver.
//
void
lzwStream::convert()
{
    if (lz_codeSize >= 8)
        convert8orAbove();
    else
        convertBelow8();
}


namespace {
    // Create a temporary file name.
    //
    void
    lzw_tempfile(char *buf)
    {
        static int num;
        const char *id = "htm-lzw";

        const char *path = getenv("TMPDIR");
        if (!path) {
            path = "/tmp";
#ifdef WIN32
            mkdir(path);
#endif
        }

        if (!access(path, W_OK))
            sprintf(buf, "%s/%s%u-%d.Z", path, id, (unsigned int)getpid(), num);
        else
            sprintf(buf, "%s%u-%d.Z", id, (unsigned int)getpid(), num);
        num++;
    }
}


// Initialize the given stream.   Possible return codes:
// -2: the read functions haven't been set;
// -1: temp file couldn't be opened;
//  0: gif code size is invalid/couldn't be read;
//  1: success.
//
int
lzwStream::init()
{
    // no error
    lz_err_msg = 0;

    // check if we have the read functions
    char msg_buf[256];
    if (readOK == 0 || getData == 0) {
        sprintf(msg_buf, "lzwStream Error: no read functions attached!");
        lz_err_msg = lstring::copy(msg_buf);
        return (-2);
    }

    // init input side
    lz_done = 0;
    lz_curBit = 0;
    lz_lastBit = 0;
    lz_lastByte = 2;

    for (int i = 0; i < 280; i++)
        lz_buf[i] = 0;
    for (int i = 0; i < 16; i++)
        lz_outBuf[i] = 0;

    // initialize buffered output
    memset(lz_accum, 0, BUFFERSIZE);
    lz_acount = 0;

    // close any open files
    if (lz_zPipe) {
        fclose(lz_zPipe);
        lz_zPipe = 0;
    }
    if (lz_f) {
        fclose(lz_f);
        lz_f = 0;
        unlink(lz_zName);
    }

    // no data to uncompress yet
    lz_error = false;
    lz_uncompressed = false;

    // temporary output file
    lzw_tempfile(lz_zName);
    if (!(lz_f = fopen(lz_zName, "w"))) {
        sprintf(msg_buf, "lzwStream Error: couldn't open temporary file "
            "'%s'.", lz_zName);
        lz_err_msg = lstring::copy(msg_buf);
        return (-1);
    }

    // get codeSize (= how many bits each pixel takes) from ImageBuffer.
    unsigned char c;
    if ((*readOK)(lz_ib, &c, 1) == 0) {
        sprintf(msg_buf, "lzwStream Error: couldn't read GIF codesize.");
        lz_err_msg = lstring::copy(msg_buf);
        return (0);
    }

    // GIF lzw codes
    lz_codeSize = (int)((unsigned char)c);  // initial bits per pixel

    // initialize input side
    lz_codeBits    = lz_codeSize + 1;
    lz_clearCode   = 1 << lz_codeSize;
    lz_endCode     = lz_clearCode + 1;
    lz_maxCodeSize = 2*lz_clearCode;
    lz_maxCode     = lz_clearCode + 2;
    lz_nextCode    = lz_maxCode;

    // initialize codeSize < 8 output side
    lz_offset    = 0;
    lz_clearFlag = false;
    lz_n_bits    = INIT_BITS;
    lz_maxcode8  = MAXCODE(lz_n_bits);
    lz_freeEntry = FIRST;

    // check clearCode value
    if (lz_clearCode >= MAX_LZW_CODE) {
        sprintf(msg_buf, "lzwStream Error: corrupt raster data: bad "
            "GIF codesize (%i).", lz_codeSize);
        lz_err_msg = lstring::copy(msg_buf);
        return (0);
    }

    // Write compress header
    char_out(0x1f);
    char_out(0x9d);

    // gif max code length + 1, block mode flag
    char_out((MAX_BITS & 0x1f) | 0x80);

    return (1);
}


// Read uncompressed data from the stream.
//
int
lzwStream::fillBuffer(unsigned char *data, int size)
{
    if (lz_error)
        return (0);

    // uncompress data if not yet done
    if (!lz_uncompressed || lz_zPipe == 0) {
        if (!uncompressData())
            return (0);
    }
    return (fread(data, 1, size, lz_zPipe));
}


// Return an allocated buffer with uncompressed stream data.
//
unsigned char *
lzwStream::uncompress(int *size)
{
    *size = 0;

    if (lz_error)
        return (0);

    // no error
    lz_err_msg = 0;

    // uncompress data if not yet done
    if (!lz_uncompressed || lz_zPipe == 0) {
        if (!uncompressData())
            return (0);
    }

    fseek(lz_zPipe, 0L, SEEK_END);
    *size = ftell(lz_zPipe);

    // sanity check
    char msg_buf[256];
    if (*size == 0) {
        sprintf(msg_buf, "lzwStream Error: zero-length data file.");
        lz_err_msg = lstring::copy(msg_buf);
        return (0);
    }

    rewind(lz_zPipe);

    unsigned char *data = new unsigned char[*size];

    // read it all
    fread(data, *size, 1, lz_zPipe);

    // close and remove file
    if (lz_zPipe) {
        fclose(lz_zPipe);
        lz_zPipe = 0;
    }
    if (lz_f) {
        fclose(lz_f);
        lz_f = 0;
        unlink(lz_zName);
    }
    // and return it
    return (data);
}


// Get next code signal from the LZW stream.  Returns code signal or
// -1 on end of data.
//
int
lzwStream::getCode()
{
    if (lz_error) {
        lz_error = 0;
        return (lz_clearCode);
    }

    // about to overflow the input buffer, get a new one
    int code = 0;
    if ((lz_curBit + lz_codeBits) >= lz_lastBit) {
        int count;

        if (lz_done) {
            // When lzw->curBit >= lzw->lastBit there was less data
            // than required.  This is a common occurance for gif
            // images.
            return (-1);
        }

        // Keep two last codes of current buffer, they haven't been
        // processed yet.

        lz_buf[0] = (int)((unsigned char)lz_buf[lz_lastByte-2]);
        lz_buf[1] = (int)((unsigned char)lz_buf[lz_lastByte-1]);

        // get next data
        if ((count = (*getData)(lz_ib, (unsigned char*)&(lz_buf[2]))) == 0)
            lz_done = 1;

        lz_lastByte = 2 + count;
        lz_curBit = (lz_curBit - lz_lastBit) + 16;
        lz_lastBit = (2+count)*8 ;
    }

    // compute LZW code
    for (int i = lz_curBit, j = 0; j < lz_codeBits; ++i, ++j)
        code |= (((int)((unsigned char)
            lz_buf[i / 8]) & (1 << (i % 8))) != 0) << j;

    lz_curBit += lz_codeBits;

    return (code);
}


// Call "uncompress" to uncompress the LZW-compressed GIF raster data.
//
int
lzwStream::uncompressData()
{
    char msg_buf[256];
    lz_err_msg = 0;

    if (lz_zPipe == 0) {
        // flush output file
        fflush(lz_f);

        // call uncompress on our converted GIF lzw data
        if (system(lz_zCmd)) {
            sprintf(msg_buf, "lzwStream Error: Couldn't execute '%s'.",
                lz_zCmd);
            lz_err_msg = lstring::copy(msg_buf);
            unlink(lz_zName);
            lz_error = true;
            return (false);
        }
        // open the output file
        lz_zName[strlen(lz_zName) - 2] = '\0';
        if ((lz_zPipe = fopen(lz_zName, "r")) == 0) {
            sprintf(msg_buf, "lzwStream Error: Couldn't open uncompress "
                "file '%s'. Corrupt data?", lz_zName);
            lz_err_msg = lstring::copy(msg_buf);
            unlink(lz_zName);
            lz_error = true;
            return (false);
        }
    }
    lz_uncompressed = true;
    return (true);
}


// Buffers LZW codes and writes them out when the buffer is full.
// This function is for gif images with less than 8 bits per pixel (=
// codeSize) and packs the LZW codes the same way compress does.  This
// is ABSOLUTELY REQUIRED since compress only deals with codes between
// 8 and 15 bits.  This piece of code has been lifted almost literally
// from the public domain compress.
//
void
lzwStream::packBits(int code)
{
    int r_off = lz_offset;   // current offset in output buffer
    int bits = lz_n_bits;    // bits per pixel
    char *bp = lz_outBuf;    // ptr to current pos in buffer

    if (code >= 0) {
        // get to the first byte
        bp += (r_off >> 3);
        r_off &= 7;

        // Since code is always >= 8 bits, only need to mask the first
        // hunk on the left.

        *bp = (*bp & rmask[r_off]) | ((code << r_off) & lmask[r_off]);
        bp++;
        bits -= (8 - r_off);
        code >>= 8 - r_off;

        // Get any 8 bit parts in the middle (<=1 for up to 16 bits).
        if (bits >= 8) {
            *bp++ = code;
            code >>= 8;
            bits -= 8;
        }
        // Last bits.
        if (bits)
            *bp = code;

        lz_offset += lz_n_bits;
        if (lz_offset == (lz_n_bits << 3)) {
            bp = lz_outBuf;
            bits = lz_n_bits;
            do {
                char_out(*bp++);
            }
            while (--bits);
            lz_offset = 0;
        }

        // If the next entry is going to be too big for the code size,
        // then increase it, if possible.

        if (lz_freeEntry > lz_maxcode8 || lz_clearFlag) {
            // flush contents of output buffer
            flush_buf();

            // Write the whole buffer, because the input side won't
            // discover the size increase until after it has read it.

            if (lz_offset > 0)
                fwrite(lz_outBuf, 1, lz_n_bits, lz_f);
            lz_offset = 0;

            if (lz_clearFlag) {
                lz_maxcode8 = MAXCODE(lz_n_bits = INIT_BITS);
                lz_clearFlag = false;
            }
            else {
                lz_n_bits++;
                if (lz_n_bits == MAX_BITS)
                    lz_maxcode8 = MAX_CODE;
                else
                    lz_maxcode8 = MAXCODE(lz_n_bits);
            }
        }
    }
    // flush rest of buffer at EOF
    else {
        // flush contents of output buffer
        flush_buf();

        if (lz_offset > 0) {
            // flush out remaining chars
            fwrite(lz_outBuf, 1, (lz_offset+7)/8, lz_f);

            lz_offset = 0;
            fflush(lz_f);
        }
    }
}


// Convert the GIF LZW compressed raster data to the "compress"
// compressed data format, codeSize < 8.
//
void
lzwStream::convertBelow8()
{
    bool first = true;
    int offset = 255 - lz_clearCode;
    lz_error = true;      // skips first clearCode in raster data
    lz_nextCode = lz_maxCode;
    lz_freeEntry = FIRST;

    // Mimic compress behaviour but get LZW code instead of computing
    // it.  GIF LZW code needs to be corrected to match fixed compress
    // codes so we can properly reset the tables if required.  We also
    // must keep track of the LZW codesize since it influences the
    // values of the LZW codes.

    bool eod = false;       // set when end-of-data is reached
    int inCode;             // input code
    while ((inCode = getCode()) != -1 && !eod) {
        // First part: verify and adjust GIF LZW code signal.

        // ClearCode sets everything back to their initial values and then
        // reads the next code.

        if (inCode == lz_clearCode) {
            // reset code table
            lz_codeBits    = lz_codeSize + 1;
            lz_clearCode   = 1 << lz_codeSize;
            lz_endCode     = lz_clearCode + 1;
            lz_maxCodeSize = 2*lz_clearCode;
            lz_maxCode     = lz_clearCode + 2;
            lz_nextCode    = lz_maxCode - 1;
            offset = 255 - lz_clearCode;

            // keep output side in sync
            if (first)
                first = false;
            else {
                lz_freeEntry = FIRST;
                lz_clearFlag = true;
                packBits(CLEAR);
            }
            // get next code
            do {
                inCode = getCode();
                if (inCode == -1) {
                    // end-of-data, break out of it
                    eod = true;
                    inCode = lz_endCode;
                    break;
                }
            }
            while (inCode == lz_clearCode);
        }

        // End-of-data, compress doesn't have this , so just break
        if (inCode == lz_endCode) {
            eod = true;
            break;
        }

        // Check input code size and adjust if the next code would
        // overflow the LZW tables.  Compressed GIF raster data uses a
        // range starting at codeSize + 1 up to 12 bits per code.

        if ((++lz_nextCode >= lz_maxCodeSize) &&
                (lz_maxCodeSize < MAX_LZW_CODE)) {
            lz_maxCodeSize *= 2;
            lz_codeBits++;
        }

        // Second part: write corrected LZW code
        //
        // Write output block:  the output buffer contains a series of
        // bytes with lengths anywhere between 3 and 12.  Compress can
        // only handle codeSize 8 or up, anything below must be packed
        // according to the format compress expects.  This was the
        // most difficult part to figure out, but its as simple as
        // this:  the GIF lzw codes are just increased with the
        // difference of compress clearCode (fixed value of 256) and
        // the current GIF clearCode...

        // correct and flush
        packBits(inCode < lz_clearCode ? inCode : inCode + offset);

        if (lz_freeEntry < MAX_CODE)
            lz_freeEntry++;
    }
    // flush output buffer
    packBits(-1);

    // and close output file
    fflush(lz_f);
    fclose(lz_f);
    lz_f = 0;
}


// Convert the GIF LZW compressed raster data to the "compress"
// compressed data format, codeSize >= 8.
//
void
lzwStream::convert8orAbove()
{
    // init output side
    int outCodeBits = lz_codeBits;
    int maxOutSize  = 2*lz_clearCode;

    // clear table
    bool first = true;
    lz_error = true;      // skips first clearCode in raster data
    lz_nextCode = lz_maxCode - 1;
    bool clear = false;

    bool eod = false;
    int inCode, outCode;
    int outBuf[8];
    do {
        // create output buffer
        int i;
        for (i = 0; i < 8; ++i) {
            // check for table overflow
            if (lz_nextCode + 1 > 0x1001)
                // clear string table
                inCode = 256;
            // read input code
            else {
                do {
                    inCode = getCode();
                    if (inCode == -1) {
                        eod = true;
                        inCode = 0;
                    }
                }
                while (first && inCode == lz_clearCode);
                first = false;
            }

            // Code signal less than clear code signal:  compressed
            // data, store it.

            if (inCode < lz_clearCode)
                outCode = inCode;

            // Code signal equals clear code.  Set flag so string
            // table will be cleared.  GIF clearCode signal value is
            // dynamic, whereas compress uses a fixed value of 256.

            else if (inCode == lz_clearCode) {
                outCode = 256;
                clear = true;
                first = false;
            }

            // End code signal.  Compress does not have this signal so
            // just set the end-of-data flag.

            else if (inCode == lz_endCode) {
                outCode = 0;
                eod = true;
            }
            else

                // Code signal higher than end code signal:
                // compressed data, store it.

                outCode = inCode - 1;

            outBuf[i] = outCode;

            // Check input code size.  Compressed GIF raster data uses
            // a range starting at codeSize + 1 up to 12 bits per
            // code.

            if ((++lz_nextCode >= lz_maxCodeSize) &&
                    (lz_maxCodeSize < MAX_LZW_CODE)) {
                lz_maxCodeSize *= 2;
                ++lz_codeBits;
            }

            // check for eod/clear
            if (eod)
                break;
            if (clear) {
                i = 8;
                break;
            }
        }

        // Write output block:  the output buffer contains a series of
        // bytes with lengths anywhere between 8 and 12.  The output
        // buffer is just packed in the same way an LZW gif *writer*
        // does it.

        int outBits = 0;    // max output code
        int outData = 0;    // temporary output buffer
        int j = 0;
        while (j < i || outBits > 0) {
            // package all codes
            if (outBits < 8 && j < i) {
                outData = outData | (outBuf[j++] << outBits);
                outBits += outCodeBits;
            }
            char_out(outData & 0xff);
            outData >>= 8;
            outBits -= 8;
        }

        if (lz_nextCode - 1 == maxOutSize) {
            outCodeBits = lz_codeBits;
            maxOutSize *= 2;
        }

        // clear table if necessary
        if (clear) {
            lz_codeBits    = lz_codeSize + 1;
            lz_clearCode   = 1 << lz_codeSize;
            lz_endCode     = lz_clearCode + 1;
            lz_maxCodeSize = 2*lz_clearCode;
            lz_maxCode     = lz_clearCode + 2;
            lz_nextCode    = lz_maxCode - 1;
            maxOutSize     = 2*lz_clearCode;
            outCodeBits    = lz_codeBits;
            clear = false;
        }
    }
    while (!eod);

    // flush any chars left
    flush_buf();

    // and close output file
    fflush(lz_f);
    fclose(lz_f);
    lz_f = 0;
}


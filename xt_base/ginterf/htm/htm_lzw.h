
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
 * MOZY html help viewer files                                            *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

/*------------------------------------------------------------------------*
 * This file is part of the gtkhtm widget library.  The gtkhtm library
 * was derived from the gtk-xmhtml library by:
 *
 *   Stephen R. Whiteley  <srw@wrcad.com>
 *   Whiteley Research Inc.
 *------------------------------------------------------------------------*
 *  The gtk-xmhtml widget was derived from the XmHTML library by
 *  Miguel de Icaza  <miguel@nuclecu.unam.mx> and others from the GNOME
 *  project.
 *  11/97 - 2/98
 *------------------------------------------------------------------------*
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
 *------------------------------------------------------------------------*/

#ifndef HTM_LZW_H
#define HTM_LZW_H

#include "htm_widget.h"
#include "htm_image.h"
#include <stdio.h>
#include <zlib.h>

#define BUFFERSIZE  512

namespace htm
{
    // LZW stream definition
    struct lzwStream
    {
        lzwStream(ImageBuffer*, const char*);
        ~lzwStream();

        // flush output buffer if it contains something
        void flush_buf()
            {
                if (lz_acount > 0) {
                    fwrite(lz_accum, 1, lz_acount, lz_f);
                    lz_acount = 0;
                }
            }

        // buffer output and flush when full
        void char_out(unsigned char c)
            {
                lz_accum[lz_acount++] = c;
                if (lz_acount >= BUFFERSIZE-1)
                    flush_buf();
            }

        int getCodeSize() { return (lz_codeSize); }
        const char *getError() { return (lz_err_msg); }
        void setReadProc(size_t(*r)(ImageBuffer*, unsigned char*, int))
            { readOK = r; }
        void setDataProc(size_t(*d)(ImageBuffer*, unsigned char*))
            { getData = d; }

        void convert();
        int init();
        int fillBuffer(unsigned char*, int);
        unsigned char *uncompress(int*);
        int getCode();
        int uncompressData();
        void packBits(int);
        void convertBelow8();
        void convert8orAbove();

private:
        FILE *lz_zPipe;             // uncompress file handle
        FILE *lz_f;                 // compress file handle
        char lz_zCmd[256];          // uncompress command
        char *lz_zName;             // compress file name, indexes in zCmd
        int lz_error;               // uncompress error flag
        int lz_uncompressed;        // uncompress finished flag
        ImageBuffer *lz_ib;         // master input buffer

        unsigned char lz_accum[BUFFERSIZE];    // buffered output
        int lz_acount;              // current char count

        // LZW code computation variables
        char lz_buf[280];           // input buffer
        int lz_curBit;              // no of bits processed so far
        int lz_lastBit;             // bitcount of last bit in input buffer
        int lz_lastByte;            // last known processed byte
        int lz_done;                // input done flag
        int lz_nextCode;            // LZW code counter

        // global raster data variables
        int lz_codeSize;            // bits per pixel
        int lz_codeBits;            // bits used by each LZW code
        int lz_clearCode;           // reset signal table signal
        int lz_endCode;             // end-of-data signal
        int lz_maxCode;             // start code signal
        int lz_maxCodeSize;         // maximum signal table size
        char lz_outBuf[16];         // compress output buffer

        // variables for images with 7 or less bits per pixel
        int lz_offset;              // current bit offset
        int lz_freeEntry;           // compress code counter
        int lz_n_bits;              // output code size
        int lz_maxcode8;            // maximum output signal
        int lz_clearFlag;           // clear signal table flag

        // data readers
        size_t (*readOK)(ImageBuffer*, unsigned char*, int);
        size_t (*getData)(ImageBuffer*, unsigned char*);

        char *lz_err_msg;           // error description
    };
}

#endif


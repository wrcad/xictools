
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

//
// External gif decoder
//
// NOT copyrighted, May 1997.
// Provided by an anonymous contributor.
//
// You are totally free to do everything you want with it.

#include "htm_widget.h"
#include "htm_image.h"
#include <string.h>
#include <stdio.h>

//  Pulled out of nextCode
#define MAX_LWZ_BITS 12
#define MAX_LWZ_SIZE (1 << MAX_LWZ_BITS)

namespace {
    const int maskTbl[16] =
    {
        0x0000, 0x0001, 0x0003, 0x0007,
        0x000f, 0x001f, 0x003f, 0x007f,
        0x00ff, 0x01ff, 0x03ff, 0x07ff,
        0x0fff, 0x1fff, 0x3fff, 0x7fff,
    };
}

struct LZW
{
    LZW(int);

    int nextCode();
    int nextLZW();

    int curbit;
    int lastbit;
    int last_byte;
    bool get_done;
    bool return_clear;
    int stack[MAX_LWZ_SIZE*2];
    int *sp;
    int code_size;
    int set_code_size;
    int max_code;
    int clear_code;
    int max_code_size;
    int end_code;
    unsigned char buf[280];
    int table[2][MAX_LWZ_SIZE];
    int firstcode;
    int oldcode;
};


LZW::LZW(int input_code_size)
{
    curbit              = 0;
    lastbit             = 0;
    last_byte           = 2;
    get_done            = false;
    return_clear        = true;
    memset(stack, 0, sizeof(stack));
    sp                  = stack;
    code_size           = input_code_size + 1;
    set_code_size       = input_code_size;
    clear_code          = 1 << set_code_size;
    max_code            = clear_code + 2;
    max_code_size       = 2 * clear_code;
    end_code            = clear_code + 1;
    memset(buf, 0, sizeof(buf));
    memset(table, 0, sizeof(table));
    firstcode           = 0;
    oldcode             = 0;
}


int
LZW::nextCode()
{
    if (return_clear) {
        return_clear = false;
        return (clear_code);
    }

    int end = curbit + code_size;

    // out of data for this call
    if (end >= lastbit)
        return (-1);

    int j = end / 8;
    int i = curbit / 8;

    unsigned int ret;
    if (i == j)
        ret = (unsigned int)buf[i];
    else if (i + 1 == j)
        ret = (unsigned int)buf[i] | ((unsigned int)buf[i+1] << 8);
    else
        ret = (unsigned int)buf[i] | ((unsigned int)buf[i+1] << 8) |
            ((unsigned int)buf[i+2] << 16);

    ret = (ret >> (curbit % 8)) & maskTbl[code_size];

    curbit += code_size;

    return ((int)ret);
}


int
LZW::nextLZW()
{
    if (sp > stack)
        return (*--sp);

    int code;
    if ((code = nextCode()) < 0)
        return (code);

    if (firstcode < 0) {
        firstcode = oldcode = code;
        return (code);
    }

    if (code == clear_code) {
        // corrupt GIFs can make this happen
        if (clear_code >= MAX_LWZ_SIZE)
            return (-2);

        int i;
        for (i = 0 ; i < clear_code; ++i) {
            table[0][i] = 0;
            table[1][i] = i;
        }
        for ( ; i < MAX_LWZ_SIZE; ++i)
            table[0][i] = table[1][i] = 0;

        code_size = set_code_size + 1;
        max_code_size = 2*clear_code;
        max_code = clear_code + 2;
        do {
            firstcode = oldcode = nextCode();
        } while (firstcode == clear_code);
        return (firstcode);
    }
    if (code == end_code)
        return (-2);

    int incode = code;

    if (code >= max_code) {
        *sp++ = firstcode;
        code = oldcode;
    }

    while (code >= clear_code) {
        *sp++ = table[1][code];

        // circular table entry BIG ERROR
        if (code == table[0][code])
            return (code);

        // circular table STACK OVERFLOW!
        if (sp >= (stack + sizeof(stack)))
            return (code);

        code = table[0][code];
    }

    *sp++ = firstcode = table[1][code];

    if ((code = max_code) < MAX_LWZ_SIZE) {
        table[0][code] = oldcode;
        table[1][code] = firstcode;
        ++max_code;
        if (max_code >= max_code_size &&
                max_code_size < MAX_LWZ_SIZE) {
            max_code_size *= 2;
            ++code_size;
        }
    }
    oldcode = incode;
    return (*--sp);
}


// gstream contains the incoming data and should contain the decoded
// data upon return.  The number of bytes offered for decoding will
// never exceed 256 (value of the avail_in field in the gstream
// structure) as this is the maximum value a block of GIF data can
// possibly have.
//
int
decodeGIFImage(htmGIFStream *gstream)
{
    // we are being told to initialize ourselves
    if (gstream->state == GIF_STREAM_INIT) {
        // Initialize the decompression routines
        gstream->external_state = new LZW(gstream->codesize);
        return (GIF_STREAM_OK);
    }

    // pick up our LZW state
    LZW *lzw = (LZW*)gstream->external_state;

    // We are being told to destruct ourselves.  Note that for
    // GIF_STREAM_END gstream->next_in will contain the value
    // \000\001; (zero-length data block and the GIF terminator code)
    // and gstream->avail_in will be equal to 3.

    if (gstream->state == GIF_STREAM_FINAL ||
            gstream->state == GIF_STREAM_END) {
        // free everything and return
        if (lzw) {
            delete lzw;
            gstream->external_state = NULL;
        }
        return (GIF_STREAM_END);
    }

    // store new data
    lzw->buf[0] = lzw->buf[lzw->last_byte-2];
    lzw->buf[1] = lzw->buf[lzw->last_byte-1];

    memcpy(&lzw->buf[2], gstream->next_in, gstream->avail_in);

    lzw->last_byte = 2 + gstream->avail_in;
    lzw->curbit = (lzw->curbit - lzw->lastbit) + 16;
    lzw->lastbit = 8*lzw->last_byte;

    // current position in decoded output buffer
    unsigned char *dp = gstream->next_out;

    for ( ; gstream->avail_out; gstream->total_out++, gstream->avail_out--) {
        int v;
        if ((v = lzw->nextLZW()) < 0) {
            // saw end_code
            if (v == -2)
                return (GIF_STREAM_END);
            // processed all current data
            return (GIF_STREAM_OK);
        }
        *dp++ = v;
    }
    return (GIF_STREAM_END);
}


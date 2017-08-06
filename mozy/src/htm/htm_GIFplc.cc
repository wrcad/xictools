
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
 * MOZY html help viewer files                                            *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

/*------------------------------------------------------------------------*
 * This file is part of the gtkhtm widget library.  The gtkhtm library
 * was derived from the gtk-xmhtml library by:
 *
 *   Stephen R. Whiteley  <stevew@wrcad.com>
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

#include "htm_widget.h"
#include "htm_plc.h"
#include <unistd.h>

#define INTERLACE       0x40
#define LOCALCOLORMAP   0x80

#define BitSet(byte, bit)       (((byte) & (bit)) == (bit))
#define LM_to_uint(a, b)        ((((b)&0xFF) << 8) | ((a)&0xFF))


PLCImageGIF::~PLCImageGIF()
{
#ifdef GIF_DECODE
    // we have an external decoder, tell it to destroy itself
    o_gstream->state     = GIF_STREAM_END;
    o_gstream->next_out  = 0;
    o_gstream->avail_out = 0;
    o_gstream->next_in   = 0;
    o_gstream->avail_in  = 0;

    // call external decoder
    decodeGIFImage(o_gstream);

    // destroy the stream object
    delete o_gstream;
#else
    // destroy the stream object
    delete o_lstream;
#endif
}


// Image initializer for GIF images.  As this function must be fully
// re-entrant, it needs a lot of checks to make sure we have the data
// we want fully available.  The drawback is that if we are being
// suspended while doing this initialization, everything must be reset
// and repeated the next time this routine is called.
//
void
PLCImageGIF::Init()
{
    // this plc is active
    o_plc->plc_status = PLC_ACTIVE;

    // When this function is called, the init method of this PLC has
    // already been called to determine the type of this PLC Image
    // object.  Therefore we already have data available and we need
    // to rewind the input buffer back to the beginning.

    o_plc->RewindInputBuffer();

    // we know this is a gif image, so skip magic
    o_info->type = IMAGE_GIF;
    unsigned char buf[16];
    o_plc->ReadOK(buf, 6);

    // read logical screen descriptor
    o_plc->ReadOK(buf, 7);

    // image dimensions
    o_width   = LM_to_uint(buf[0], buf[1]);
    o_height  = LM_to_uint(buf[2], buf[3]);

    // set colorspace and allocate a colormap
    o_colorclass = IMAGE_COLORSPACE_INDEXED;
    o_cmapsize   = 2 << (buf[4] & 0x07);

    // We may have been called before (but returned 'cause not enough
    // data was available).

    if (o_cmap == 0)
        o_cmap = new htmColor[o_cmapsize];

    // image is initially fully opaque
    o_transparency = IMAGE_NONE;
    o_bg_pixel = (unsigned int)-1;

    // Incoming data buffer.  This is way too much as the incoming
    // data will be compressed (but it does make sure there is enough
    // room).

    o_buf_size   = o_width*o_height;
    o_buf_pos    = 0;    // current pos in data received so far
    o_byte_count = 0;    // size of data received so far
    if (o_buffer == 0) {
        o_buffer = new unsigned char[o_buf_size + 1];
        memset(o_buffer, 0, o_buf_size + 1);
    }

    // check if a global colormap is available
    if (BitSet(buf[4], LOCALCOLORMAP)) {
        if (!ReadColormap()) {
            // premature end of data
            if (o_plc->plc_data_status == STREAM_END) {
                o_plc->plc_owner->im_html->warning("GIF Init",
                    "%s: bad image\n    Could not read global colormap.",
                    o_plc->plc_url);
                o_plc->plc_status = PLC_ABORT;
            }
            return;  // no global colormap!
        }
    }

    // process all extensions
    unsigned char c = 0;
    while (c != ',') {
        if (!o_plc->ReadOK(&c, 1))
            return;

        if (c == ';') {
            // GIF terminator
            o_plc->plc_owner->im_html->warning("GIF Init",
                "%s: corrupt image\n    Image contains no data.",
                o_plc->plc_url);
            o_plc->plc_status = PLC_ABORT;
            return;
        }

        if (c == '!') {
            // Extension
            if (!o_plc->ReadOK(&c, 1)) {
                if (o_plc->plc_data_status == STREAM_END) {
                    o_plc->plc_owner->im_html->warning("GIF Init",
                        "%s: corrupt image\n    Could not"
                        "read extension block type.", o_plc->plc_url);
                    o_plc->plc_status = PLC_ABORT;
                }
                return;
            }
            if (!DoExtension(c)) {
                if (o_plc->plc_data_status == STREAM_END) {
                    o_plc->plc_owner->im_html->warning("GIF Init",
                        "%s: corrupt image\n    Empty extension block.",
                        o_plc->plc_url);
                    o_plc->plc_status = PLC_ABORT;
                }
                return;
            }
            continue;
        }
        if (c != ',')
            continue;  // Not a valid start character
    }
    // get image descriptor
    if (!o_plc->ReadOK(buf, 9))
        return;

    // see if we are to use a local colormap
    if (BitSet(buf[8], LOCALCOLORMAP)) {
        // local colormap size
        o_ncolors = 1<<((buf[8]&0x07)+1);

        // do we also have a glocal colormap?
        delete [] o_cmap;

        o_cmapsize = o_ncolors;
        o_cmap = new htmColor[o_cmapsize];

        if (!ReadColormap()) {
            // premature end of data
            if (o_plc->plc_data_status == STREAM_END) {
                o_plc->plc_owner->im_html->warning("GIF Init",
                    "%s: bad image\n    Could not read local colormap.",
                    o_plc->plc_url);
                o_plc->plc_status = PLC_ABORT;
            }
            return;  // no global colormap!
        }
    }
    o_ncolors = o_cmapsize;

    // sanity check: image must have a colormap
    if (o_cmap == 0) {
        o_plc->plc_owner->im_html->warning("GIF Init",
            "%s: bad image\n    Image has no colormap (global or local)!",
            o_plc->plc_url);
        o_plc->plc_status = PLC_ABORT;
        return;  // no global colormap!
    }

    // image depth (= codeSize in GIF images, unused in GZF images)
    if (!(o_plc->ReadOK(&c, 1)))
        return;

    o_depth = (int)(c & 0xff);

    // check interlacing
    if (BitSet(buf[8], INTERLACE)) {
        // interlaced gifs require 4 passes and use an initial rowstride of 8
        o_npasses = 4;
        o_stride  = 8;
    }
    else {
        // regular gif, 1 pass will get us the entire image
        o_npasses = 1;
        o_stride  = 0;
    }
    o_curr_pass = 0;
    o_curr_scanline = 0;

    // This routine is also used for GZF images, so before
    // initializing the lzwStream object we need to make sure we have
    // been called for a true GIF image.

    if (o_type == plcGIF) {

#ifdef GIF_DECODE
        o_gstream = new htmGIFStream(c, o_buffer, o_buf_size+1);
        o_gstream->is_progressive = true;

        // And call external decoder so it can initialize its own
        // data structures

        if ((decodeGIFImage(o_gstream)) != GIF_STREAM_OK) {
            if (o_gstream->msg != 0) {
                o_plc->plc_owner->im_html->warning("GIF Init",
                    "Initalization failed for image\n    %s:\n    %s.",
                    o_plc->plc_url, o_gstream->msg ? o_gstream->msg :
                    "(unknown error)");
            }
            // external decoder initalization failed, abort and return
            o_plc->plc_status = PLC_ABORT;
            return;
        }
        o_gstream->state = GIF_STREAM_OK;
#else
        // initialize local data buffer
        o_ib.file   = o_plc->plc_url;
        o_ib.buffer = o_buffer;
        o_ib.size   = 0;
        o_ib.next   = 0;
        o_ib.type   = IMAGE_GIF;
        o_ib.depth  = o_depth;
        o_ib.may_free = false;

        // initialize lzwStream object
        if (o_lstream == 0) {
            o_lstream = new lzwStream(&o_ib,
                o_plc->plc_owner->im_html->htm_zCmd);
            // set read functions
            o_lstream->setReadProc(htmGifReadOK);
            o_lstream->setDataProc(htmGifGetDataBlock);
        }
        // first byte in buffer is gif codesize
        o_ib.buffer[0] = c;
        o_ib.size = 1;
#endif
        // allocate room for final image data
        if (o_data == 0) {
            o_data = new unsigned char[o_buf_size + 1];
            memset(o_data, 0, o_buf_size + 1);

            // don't allocate clipmask yet, it's done in the plc code
        }
        o_data_size = o_buf_size;
        o_data_pos  = 0;
    }

    // object has been initialized
    o_plc->plc_initialized = true;
}


// GZF scanline processor (decompresses data and orders it).
//
void
PLCImageGIF::ScanlineProc()
{
    // We can choose between two types of LZW decoders:  the internal
    // one or an externally installed progressive decoder.  The
    // internal decoder is fairly efficient for direct gif loading,
    // but is HUGELY slow for progressive gif loading:  it forks of
    // uncompress (or gzip) each time lzwStreamUncompress is called.
    // Therefore we collect all data and call the internal decoder
    // when all image data has been received.  When an external gif
    // decoder has been installed, we just proceed in the same way as
    // with the GZF scanline procedure:  decode a chunk of data each
    // time and allow it to be transfered to the display.

#ifdef GIF_DECODE
    // External GIF decoder installed, proceeds in the same way as
    // the GZF decoder, except that now the external decoder is
    // called instead of the zlib routines.

    htmGIFStream *gifstream = o_gstream;
    int err;

    int bytes_avail = o_plc->plc_left;

    // Get a new block of compressed data.  This will
    // automatically make a new request for input data when
    // there isn't enough data left in the current input
    // buffer.

    if ((gifstream->avail_in = o_plc->GetDataBlock(o_gbuf)) == 0) {
        if (o_plc->plc_status == PLC_SUSPEND ||
                o_plc->plc_status == PLC_ABORT)
            return;

        // If plc_status == PLC_ACTIVE, we have a zero data
        // block which indicates the raster data end.  Put in
        // a 0 length data block, the gif terminator, set the
        // gifstream state so the caller knows to wrap things
        // up and proceed.

        o_gbuf[0] = 0;
        o_gbuf[1] = 1;
        o_gbuf[2] = ';';
        gifstream->avail_in = 3;
        gifstream->state = GIF_STREAM_FINAL;
    }

    gifstream->next_in = o_gbuf;

    // bytes left in current buffer (+1 for block header)
    bytes_avail -= (gifstream->avail_in + 1);

    // adjust output buffer
    gifstream->next_out  = o_buffer + gifstream->total_out;
    gifstream->avail_out = o_buf_size - gifstream->total_out;

    // uncompress it
    err = decodeGIFImage(gifstream);

    // check return value
    if (err != GIF_STREAM_OK && err != GIF_STREAM_END) {
        o_plc->plc_owner->im_html->warning("GIF ScanlineProc",
            "Error while decoding GIF image %s:\n    external decoder "
            "failed: %s.", o_plc->plc_url,
            gifstream->msg ? gifstream->msg : "(unknown error)");
        o_plc->plc_status = PLC_ABORT;
        return;
    }

    // this many uncompressed bytes available now
    o_byte_count = gifstream->total_out;

    // convert input data to raw image data
    bool done = DoImage(o_buffer);

    // and break if inflate has finished uncompressing our data
    if (err == GIF_STREAM_END || done == true)
        o_plc->plc_obj_funcs_complete = true;

#else

    bool have_all = false;
    int bytes_avail = o_plc->plc_left;

    // Get a new block of compressed data.  This will
    // automatically make a new request for input data when
    // there isn't enough data left in the current input
    // buffer.

    int len;
    if ((len = o_plc->GetDataBlock(o_gbuf)) != 0) {
        // store block size
        o_ib.buffer[o_ib.size++] = (unsigned char)len;

        // append newly received data
        memcpy(o_ib.buffer + o_ib.size, o_gbuf, len);

        // new buffer size
        o_ib.size += len;

        // bytes left in current buffer (+1 for block header)
        bytes_avail -= (len + 1);

        // prevent image transfer function from doing anything
        o_prev_pos = o_data_pos = 0;
    }
    else {
        if (o_plc->plc_status == PLC_SUSPEND ||
                o_plc->plc_status == PLC_ABORT)
            return;

        // If plc_status == PLC_ACTIVE, we have a zero data
        // block which indicates the raster data end.  For now
        // we just consider this as the end signal and start
        // decoding the raster data.  For gif animations this
        // would be the place to move to the next frame.

        have_all = true;

        // plug in a zero length data block and GIF terminator
        o_ib.buffer[o_ib.size++] = 0;
        o_ib.buffer[o_ib.size++] = 1;
        o_ib.buffer[o_ib.size++] = ';';

    }

    // we have got all image data, now decode it
    if (have_all) {
        // rewind image buffer
        o_ib.next = 0;

        // now (re-)initialize the stream object
        if ((o_lstream->init()) <= 0) {
            // this is an error
            o_plc->plc_owner->im_html->warning("GIF ScanlineProc",
                o_lstream->getError());
            o_plc->plc_status = PLC_ABORT;
            return;
        }

        // convert data
        o_lstream->convert();

        // get uncompressed data
        unsigned char *input;
        if ((input = o_lstream->uncompress((int*)&o_byte_count)) == 0) {
            o_plc->plc_owner->im_html->warning("GIF ScanlineProc",
                o_lstream->getError());
            // rather fatal...
            o_plc->plc_owner->im_html->warning("GIF ScanlineProc",
                "lzwStreamUncompress failed for image %s.",
                o_plc->plc_url);
            o_plc->plc_status = PLC_ABORT;
            return;
        }

        // convert input data to raw image data
        DoImage(input);

        // free input buffer
        delete [] input;

        // we are finished here
        o_plc->plc_obj_funcs_complete = true;
    }
#endif
}


// Process a gif extension block.
//
bool
PLCImageGIF::DoExtension(int label)
{
    char buf[256];

    switch(label) {
    case 0x01:      // Plain Text Extension
        break;
    case 0xff:      // Application Extension

        // Netscape Looping extension
        // Get first block

        o_plc->GetDataBlock((unsigned char*)buf);
        if (!(strncmp(buf, "NETSCAPE2.0", 11))) {
            if ((o_plc->GetDataBlock((unsigned char*)buf)) <= 0)
                return (false);  // corrupt animation/end of data
        }
        break;
    case 0xfe:      // Comment Extension
        while (o_plc->GetDataBlock((unsigned char*)buf) > 0) ;
        return (true);
    case 0xf9:      // Graphic Control Extension
        o_plc->GetDataBlock((unsigned char*)buf);
        if ((buf[0] & 0x1) != 0) {
            o_bg_pixel = (unsigned int)buf[3];
            o_transparency = IMAGE_TRANSPARENCY_BG;
        }

        while (o_plc->GetDataBlock((unsigned char*)buf) > 0) ;
        return (true);
    default:
        break;
    }
    while (o_plc->GetDataBlock((unsigned char*)buf) > 0) ;

    return (true);
}


// Read the colormap of a GIF/GZF image.
//
bool
PLCImageGIF::ReadColormap()
{
    // read it
    for (int i = 0; i < o_cmapsize; ++i) {
        unsigned char rgb[3];
        if (!o_plc->ReadOK(rgb, sizeof(rgb)))
            return (false);

        o_cmap[i].red   = rgb[0];
        o_cmap[i].green = rgb[1];
        o_cmap[i].blue  = rgb[2];
    }
    return (true);
}


// Transforms raw gif raster data to final gif image data.
//
bool
PLCImageGIF::DoImage(unsigned char *input)
{
    bool done = false;

    // convert data to actual image data
    if (o_npasses > 1) {
        // image is interlaced

        // last known position in uncompressed data stream
        unsigned char *inPtr = input;

        // Interlaced images are always fully recomputed
        unsigned char *data = o_data;     // image data
        int ypos   = 0;             // current scanline
        int pass   = 0;             // current interlacing pass
        int stride = 8;             // interlacing data stride
        int skip   = o_width;       // initial data index
        int xpos   = 0;
        int jlast  = 0;

        {
            int i, j;
            for (i = j = 0; i < (int)o_height && j < (int)o_byte_count;
                    i++, j += skip) {
                if (ypos < (int)o_height) {
                    unsigned char *outPtr = &data[skip * ypos];
                    for (xpos = 0; xpos < skip; xpos++)
                        *outPtr++ = *inPtr++;
                }
                ypos += stride;
                if (ypos >= (int)o_height) {
                    if (pass)
                        stride /= 2;
                    pass++;
                    ypos = stride/2;
                }
            }
            jlast = j;
        }

        // Tidy things up by filling in empty spaces between the
        // interlacing passes.  When we have completed a pass we have
        // data available for expanding the entire image.

        if (pass) {
            o_prev_pos = 0;
            o_data_pos = o_data_size;
            ypos = o_height;
        }
        else
            // current image data ends at this position
            o_data_pos = ypos * o_width;

        // This piece of code will copy the current line to a number
        // of following, unfilled lines.  For pass 0, it will copy a
        // line 7 times (stride = 8), for pass 1 it will copy a line 3
        // times (stride = 4), and for pass 2 it will copy a line only
        // once (stride = 2).  The last pass is a no-op.

        if (stride != 1) {
            for (int i = 0; i < ypos; i += stride) {
                unsigned char *src = &data[skip * i];
                for (int nfill = 1; nfill < stride &&
                        i + nfill < (int)o_height; nfill++) {
                    unsigned char *dest = &data[skip * (i + nfill)];
                    memmove(dest, src, skip);
                }
            }
        }
        // Break out if we have performed all passes and processed all data
        done = (pass == (int)o_npasses && jlast >= (int)o_data_size);
    }
    else {
        // last known position in uncompressed data stream
        unsigned char *inPtr = input + o_prev_pos;
        // last known position in image data stream
        unsigned char *outPtr= o_data + o_prev_pos;

        // last byte to use
        int max_byte = (int)(o_byte_count/o_width)*o_width;

        // direct data copy
        for (int i = o_prev_pos; i < max_byte; i++)
            *outPtr++ = *inPtr++;

        // last full scanline
        o_data_pos = max_byte;

        // Break out if we have performed all passes and processed all data
        done = (o_data_pos >= o_data_size);
    }
    return (done);
}



PLCImageGZF::~PLCImageGZF()
{
#if defined(HAVE_LIBPNG) || defined(HAVE_LIBZ)
    // clean up zlib
    if ((inflateEnd(&o_zstream)) != Z_OK)
        o_plc->plc_owner->im_html->warning("GZF Destructor",
            "inflateEnd failed: %s.", o_zstream.msg);
#endif
}


// Image initializer for GZF images.  As GZF images are GIF images in
// which only the compressed raster data differs, we can simply use
// the gif initializer for this.
//
void
PLCImageGZF::Init()
{
#if defined(HAVE_LIBPNG) || defined(HAVE_LIBZ)
    PLCImageGIF::Init();

    if (o_plc->plc_status == PLC_ACTIVE) {

        // this is a GZF image
        o_info->type = IMAGE_GZF;

        // initialize z_stream
        o_zstream.zalloc = Z_NULL;
        o_zstream.zfree  = Z_NULL;
        o_zstream.opaque = Z_NULL;

        // abort if this fails: incorrect library version/out of memory
        if ((inflateInit(&o_zstream)) != Z_OK) {
            o_plc->plc_owner->im_html->warning("GZF Init",
                "inflateInit failed: %s.",
                o_zstream.msg ? o_zstream.msg :"(unknown zlib error)");
            o_plc->plc_status = PLC_ABORT;
            return;
        }
        // allocate room for uncompressed data
        o_data = new unsigned char[o_buf_size];
        memset(o_data, 0, o_buf_size);
        o_data_size = o_buf_size;
        o_data_pos  = 0;

        // don't allocate clipmask yet, it's done in the plc code
    }
#else
    o_plc->plc_status = PLC_ABORT;
#endif
}


// GZF scanline processor (decompresses data and orders it).
//
void
PLCImageGZF::ScanlineProc()
{
#if defined(HAVE_LIBPNG) || defined(HAVE_LIBZ)
    bool done = false;

    // bytes left in current input buffer
    int bytes_avail = o_plc->plc_left;

    // Get a new block of compressed data.  This will
    // automatically make a new request for input data when there
    // isn't enough data left in the current input buffer.

    if ((o_zstream.avail_in = o_plc->GetDataBlock(o_zbuf)) == 0) {

        // If plc_status == PLC_ACTIVE, we have a zero data block
        // which indicates the raster data end.  For now we just
        // consider this as the end signal and return without
        // further processing.  For gif animations this would be
        // the place to move to the next frame.

        return;  // aborted, date ended or plc suspended
    }
    o_zstream.next_in = o_zbuf;

    // bytes left in current buffer (+1 for block header)
    bytes_avail -= (o_zstream.avail_in + 1);

    // adjust output buffer
    o_zstream.next_out  = o_buffer + o_zstream.total_out;
    o_zstream.avail_out = o_buf_size - o_zstream.total_out;

    // uncompress it
    int err = inflate(&o_zstream, Z_PARTIAL_FLUSH);

    // check return value
    if (err != Z_OK && err != Z_STREAM_END) {
        o_plc->plc_owner->im_html->warning("GZF ScanlineProc",
            "Error while decoding image %s:\n    inflate failed: %s.",
            o_plc->plc_url, o_zstream.msg);
        o_plc->plc_status = PLC_ABORT;
        return;
    }

    // this many uncompressed bytes available now
    o_byte_count = o_zstream.total_out;

    // convert input data to raw image data
    done = DoImage(o_buffer);

    // and break if inflate has finished uncompressing our data
    if (err == Z_STREAM_END || done == true)
        o_plc->plc_obj_funcs_complete = true;
#else
    o_plc->plc_status = PLC_ABORT;
#endif
}


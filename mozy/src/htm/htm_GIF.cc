
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
#include "htm_lzw.h"
#include "htm_image.h"
#include <stdlib.h>

#ifdef HAVE_LIBZ
#include <unistd.h>
#include <zlib.h>
#endif

#define INTERLACE       0x40
#define LOCALCOLORMAP   0x80

#define BitSet(byte, bit)       (((byte) & (bit)) == (bit))
#define LM_to_uint(a, b)        ((((b)&0xFF) << 8) | ((a)&0xFF))

#define WriteOK(FILE, BUF, LEN) fwrite(BUF, LEN, 1, FILE)

namespace htm
{
    enum
    {
        CM_RED,
        CM_GREEN,
        CM_BLUE
    };

    // gif logical screen descriptor
    struct GIFscreen
    {
        GIFscreen(htmWidget *w, unsigned char *buf)
            {
                Width               = LM_to_uint(buf[0], buf[1]);
                Height              = LM_to_uint(buf[2], buf[3]);
                memset(ColorMap, 0, sizeof(ColorMap));
                BitPixel            = 2 << (buf[4] & 0x07);
                ColorResolution     = (((buf[4] & 0x70) >> 3) + 1);
                Background          = buf[5];
                AspectRatio         = buf[6];
                html                = w;
            }

        unsigned int    Width;
        unsigned int    Height;
        unsigned char   ColorMap[3][MAX_IMAGE_COLORS];
        unsigned int    BitPixel;
        unsigned int    ColorResolution;
        unsigned int    Background;
        unsigned int    AspectRatio;
        htmWidget       *html;
    };

    // gif extension data
    struct GIF89
    {
        GIF89()
            {
                transparent     = -1;
                delayTime       = -1;
                inputFlag       = -1;
                disposal        = 0;
                loopCount       = 0;
            }

        int transparent;
        int delayTime;
        int inputFlag;
        int disposal;
        int loopCount;
    };


    // Copy len bytes to buf from an ImageBuffer.
    //
    size_t
    htmGifReadOK(ImageBuffer *ib, unsigned char *buf, int len)
    {
        if (ib->size > ib->next) {
            if (ib->next + len > ib->size)
                len = ib->size - ib->next;
            memcpy(buf, ib->buffer + ib->next, len);
            ib->next += len;
            return (len);
        }
        return (0);
    }


    // Get the next amount of data from the input buffer.
    //
    size_t
    htmGifGetDataBlock(ImageBuffer *ib, unsigned char *buf)
    {
        size_t count = 0;
        if (ib->next + 1 >= ib->size)
            return (0);
        count = ib->buffer[ib->next++];
        if (count != 0 && !htmGifReadOK(ib, buf, (int)count))
            return (0);
        return (count);
    }
}

namespace {
    bool ReadColorMap(ImageBuffer*, int,
        unsigned char[3][MAX_IMAGE_COLORS], int* = 0);
    void CopyColormap(htmColor*, int, unsigned char[3][MAX_IMAGE_COLORS]);
    int DoExtension(ImageBuffer*, int, GIF89*);
    unsigned char *DoImage(unsigned char*, int, int);
    void SkipImage(ImageBuffer*);
}


// Check whether a file is a GIF or animated GIF image.
// Returns:
//   IMAGE_UNKNOWN, IMAGE_GIF, IMAGE_GIFANIM or IMAGE_GIFANIMLOOP
int
htmImageManager::isGifAnimated(ImageBuffer *ib)
{
    // When we get here, we have already know this is a gif image so
    // don't test again.

    // gif magic
    unsigned char buf[16];
    htmGifReadOK(ib, buf, 6);

    // logical screen descriptor
    htmGifReadOK(ib, buf, 7);

    GIFscreen GifAnimScreen(im_html, buf);

    // skip global colormap
    if (BitSet(buf[4], LOCALCOLORMAP)) {
        if (!ReadColorMap(ib, GifAnimScreen.BitPixel, GifAnimScreen.ColorMap))
            return (IMAGE_UNKNOWN);  // can't read colormap
    }

    // We know a gif image is an animation if either the Netscape2.0
    // loop extension is present or if we have at least two images.
    // The first case is pretty easy to detect, the second one
    // requires us to process the giffile up to the second image.
    // Although we do not actually decode the image itself, there is a
    // performance loss here.

    GIF89 Gif89;
    int imageCount = 0;
    while (imageCount < 2) {
        // read block identifier
        unsigned char c;
        if (!htmGifReadOK(ib, &c, 1))
            break;

        if (c == ';')
            break;  // gif terminator

        if (c == '!') {
            if (!htmGifReadOK(ib, &c, 1))
                return (IMAGE_UNKNOWN);
            if ((DoExtension(ib, c, &Gif89)) == IMAGE_GIFANIMLOOP)
                return (IMAGE_GIFANIMLOOP);
            continue;
        }
        if (c != ',')
            continue;   // invalid start character

        // get next block
        if (!htmGifReadOK(ib, buf, 9))
            break;

        if (BitSet(buf[8], LOCALCOLORMAP)) {
            if (!ReadColorMap(ib, GifAnimScreen.BitPixel,
                    GifAnimScreen.ColorMap))
                return (IMAGE_UNKNOWN);  // can't read colormap
        }
        // skip this image
        SkipImage(ib);

        imageCount++;
    }
    return (imageCount >= 2 ? IMAGE_GIFANIM : IMAGE_GIF);
}


void
htmImageManager::gifAnimTerminate(ImageBuffer *ib)
{
    // rewind image buffer
    ib->rewind();
}


htmRawImageData *
htmImageManager::gifAnimInit(ImageBuffer *ib, int *loop_cnt_ret,
    GIFscreen **screen_ret)
{
    *loop_cnt_ret = -1;
    *screen_ret = 0;
#ifndef HAVE_LIBZ
    if (ib->type == IMAGE_GZF || ib->type == IMAGE_GZFANIM ||
            ib->type == IMAGE_GZFANIMLOOP)
        return (0);
#endif

    // rewind imagebuffer
    ib->rewind();

    // at this point, we know this is a valid gif or gzf image
    // skip GIF/GZF magic
    ib->next = 6;

    // read logical screen descriptor
    unsigned char buf[16];
    htmGifReadOK(ib, buf, 7);
    GIFscreen *GifAnimScreen = new GIFscreen(im_html, buf);

    // store logical screen dimensions
    htmRawImageData *img_data = new htmRawImageData();
    img_data->width  = GifAnimScreen->Width;
    img_data->height = GifAnimScreen->Height;

    // A global colormap
    if (BitSet(buf[4], LOCALCOLORMAP)) {
        if (!ReadColorMap(ib, GifAnimScreen->BitPixel,
                GifAnimScreen->ColorMap)) {
            // error reading global colormap, too bad
            im_html->warning("gifAnimInit", "Error "
                "reading global colormap in GIF animation %s, animation "
                "ignored.", ib->file);
            delete GifAnimScreen;
            delete img_data;
            return (0);
        }
        // store global colormap
        img_data->allocCmap(GifAnimScreen->BitPixel);
        CopyColormap(img_data->cmap, GifAnimScreen->BitPixel,
            GifAnimScreen->ColorMap);
    }
    else {
        // no global colormap, too bad
        im_html->warning("gifAnimInit", "No global "
            "colormap found in GIF animation %s, animation ignored.",
            ib->file);
        delete GifAnimScreen;
        delete img_data;
        return (0);
    }

    // save current position in block
    size_t curr_pos = ib->next;

    // move past all initial headers

    // block descriptor
    unsigned char c;
    if (!htmGifReadOK(ib, &c, 1)) {
        delete GifAnimScreen;
        delete img_data;
        return (0);
    }

    // loop thru all extensions and global colormap
    GIF89 Gif89;
    bool netscape = false;
    while (c == '!') {
        // read extension type
        if (!htmGifReadOK(ib, &c, 1)) {
            delete GifAnimScreen;
            delete img_data;
            return (0);
        }

        // Problem:  we know this gif is an animation, but it may be a
        // NETSCAPE2.0 loop extension or a series of images.  The
        // first has a loop count in it, but the second one doesn't.
        // DoExtension returns IMAGE_GIFANIM if this is a NETSCAPE2.0
        // or IMAGE_GIF if it's a series of images.  So we need to see
        // what we get returned:  if it's GIFANIM we get the
        // loop_count out of the Gif89 struct.  If it's just GIF we
        // set it to 1.

        if ((DoExtension(ib, c, &Gif89)) == IMAGE_GIFANIMLOOP)
            netscape = true;

        // get next block
        if (!htmGifReadOK(ib, &c, 1)) {
            delete GifAnimScreen;
            delete img_data;
            return (0);
        }
    }

    // save background pixel
    img_data->bg = Gif89.transparent;

    // and reset file pointer
    ib->next = curr_pos;

    *screen_ret = GifAnimScreen;
    *loop_cnt_ret = (netscape ? Gif89.loopCount : 1);
    return (img_data);
}


// Read the next frame of a gif animation.
//
htmRawImageData *
htmImageManager::gifAnimNextFrame(ImageBuffer *ib, int *x, int *y,
    int *timeout, int *dispose, GIFscreen *GifAnimScreen)
{
    if (!GifAnimScreen)
        return (0);
    unsigned char c;
    if (!htmGifReadOK(ib, &c, 1)) {
        delete GifAnimScreen;
        return (0);
    }

    // run until we get to the image data start character
    GIF89 Gif89;
    while (c != ',') {
        // GIF terminator
        if (c == ';') {
            delete GifAnimScreen;
            return (0);
        }

        // loop thru all extensions
        if (c == '!') {
            // read extension type
            if (!htmGifReadOK(ib, &c, 1)) {
                delete GifAnimScreen;
                return (0);
            }
            DoExtension(ib, c, &Gif89);
        }
        if (!htmGifReadOK(ib, &c, 1)) {
            delete GifAnimScreen;
            return (0);
        }
    }

    // image descriptor
    unsigned char buf[16];
    if (!htmGifReadOK(ib, buf, 9)) {
        delete GifAnimScreen;
        return (0);
    }

    // offsets in the logical screen
    *x = LM_to_uint(buf[0], buf[1]);
    *y = LM_to_uint(buf[2], buf[3]);

    htmRawImageData *img_data = new htmRawImageData;
    // width and height for this particular frame
    img_data->width  = LM_to_uint(buf[4], buf[5]);
    img_data->height = LM_to_uint(buf[6], buf[7]);

    bool useGlobalColormap = ! BitSet(buf[8], LOCALCOLORMAP);

    int bitPixel = 1 << ((buf[8] & 0x07) + 1);

    // read local colormap. Global colormap stuff is handled by caller
    unsigned char localColorMap[3][MAX_IMAGE_COLORS];
    if (!useGlobalColormap) {
        if (!ReadColorMap(ib, bitPixel, localColorMap)) {
            im_html->warning("gifAnimNextFrame",
                "Error reading local colormap, ignoring remaining frames.");
            delete GifAnimScreen;
            delete img_data;
            return (0);
        }
        // compare with global colormap and check if it differs
        unsigned int i;
        if ((i = bitPixel) == GifAnimScreen->BitPixel) {
            // compare. Will only be equal to bitPixel if the maps are equal
            i = memcmp(localColorMap, GifAnimScreen->ColorMap,
                3*MAX_IMAGE_COLORS) + bitPixel;
        }
        // local colormap differs from global one, use it
        if (i != GifAnimScreen->BitPixel) {
            // allocate colormap for this frame
            img_data->allocCmap(bitPixel);
            // fill it
            CopyColormap(img_data->cmap, bitPixel, localColorMap);
        }
    }

    // get image depth (= codeSize in GIF images)
    htmGifReadOK(ib, &c, 1);
    ib->depth = c;
    ib->next--;

    // decompress raster data
    int nread;
    unsigned char *image = inflateRaster(this, ib, img_data->width,
        img_data->height, &nread);

    if (image == 0 || (!BitSet(buf[8], INTERLACE) &&
            nread != img_data->width * img_data->height)) {
        im_html->warning("gifAnimNextFrame",
            "Read Error on image %s:\n    uncompress returned %i bytes "
            "while %i bytes are expected.", ib->file, nread,
            img_data->width * img_data->height);
        delete [] image;
        delete GifAnimScreen;
        delete img_data;
        return (0);
    }

    // convert interlaced image data to normal image data
    if (BitSet(buf[8], INTERLACE))
        image = DoImage(image, img_data->width, img_data->height);

    // save ptr
    img_data->data = image;

    // our timeouts are in milliseconds
    *timeout = Gif89.delayTime*10;
    // and they are less than zero sometimes!
    *timeout = abs(*timeout);
    *dispose = Gif89.disposal;
    img_data->bg = Gif89.transparent;

    if (img_data->data == 0) {
        delete GifAnimScreen;
        delete img_data;
        return (0);
    }
    return (img_data);
}


// Read a gif file.
//
htmRawImageData*
htmImageManager::readGIF(ImageBuffer *ib)
{
#ifndef HAVE_LIBZ
    if (ib->type == IMAGE_GZF || ib->type == IMAGE_GZFANIM ||
            ib->type == IMAGE_GZFANIMLOOP)
        return (0);
#endif

    // When we get here we already know this is a gif image, so we can
    // safely skip the magic check.

    ib->next = 6;

    // read logical screen descriptor
    unsigned char buf[16];
    htmGifReadOK(ib, buf, 7);

    GIFscreen GifScreen(im_html, buf);

    htmRawImageData *img_data = new htmRawImageData;
    img_data->width  = GifScreen.Width;
    img_data->height = GifScreen.Height;

    // allocate colormap
    img_data->allocCmap(GifScreen.BitPixel);

    int grayscale = 0;
    if (BitSet(buf[4], LOCALCOLORMAP)) {
        // Global Colormap
        if (!ReadColorMap(ib, GifScreen.BitPixel, GifScreen.ColorMap,
                &grayscale)) {
            im_html->warning("readGIF",
                "%s: bad gif\n    Image contains no global colormap.",
                ib->file);
            delete img_data;
            return (0);   // can't read colormap
        }

        for (unsigned int i = 0; i < GifScreen.BitPixel; i++) {
            // upscale to 0-2^16
            img_data->cmap[i].red   = GifScreen.ColorMap[0][i] << 8;
            img_data->cmap[i].green = GifScreen.ColorMap[1][i] << 8;
            img_data->cmap[i].blue  = GifScreen.ColorMap[2][i] << 8;
        }
    }

    unsigned char *image = 0;
    int imageCount = 0;
    int imageNumber = 1;

    GIF89 Gif89;
    while (image == 0) {
        unsigned char c;
        if (!htmGifReadOK(ib, &c, 1)) {
            delete img_data;
            return (0);
        }

        if (c == ';') {
            // GIF terminator
            if (imageCount < imageNumber) {
                im_html->warning("readGIF",
                    "%s: corrupt gif\n    Image contains no data.", ib->file);
                delete img_data;
                return (0);
            }
            break;
        }

        if (c == '!') {
            // Extension
            if (!htmGifReadOK(ib, &c, 1)) {
                im_html->warning("readGIF",
                    "%s: corrupt gif\n    Bad extension block.", ib->file);
                delete img_data;
                return (0);
            }
            DoExtension(ib, c, &Gif89);
            continue;
        }

        if (c != ',')
            continue;  // Not a valid start character

        ++imageCount;

        if (!htmGifReadOK(ib, buf, 9)) {
            im_html->warning("readGIF",
                "%s: corrupt gif\n    Bad image descriptor.", ib->file);
            delete img_data;
            return (0);
        }

        bool useGlobalColormap = ! BitSet(buf[8], LOCALCOLORMAP);

        int bitPixel = 1 << ((buf[8] & 0x07) + 1);

        // We only want to set width and height for the imageNumber we
        // are requesting.

        if (imageCount == imageNumber) {
            img_data->width  = LM_to_uint(buf[4], buf[5]);
            img_data->height = LM_to_uint(buf[6], buf[7]);
        }

        // read and fill a local or glocal colormap
        unsigned char localColorMap[3][MAX_IMAGE_COLORS];
        if (!useGlobalColormap) {
            if (!ReadColorMap(ib, bitPixel, localColorMap, &grayscale)) {
                im_html->warning("readGIF",
                    "%s: corrupt gif\n    Image contains no local colormap "
                    "while there should be one.", ib->file);
                delete img_data;
                return (0);
            }

            // We only want to set the data for the
            // imageNumber we are requesting.

            if (imageCount == imageNumber) {
                // see if we already have a colormap
                if (img_data->cmap) {
                    // image has both global and local colormap
                    if (img_data->cmapsize != bitPixel) {
                        // free current colormap
                        delete img_data->cmap;
                        // add a new one
                        img_data->allocCmap(bitPixel);
                    }
                }
                else
                    img_data->allocCmap(bitPixel);
                CopyColormap(img_data->cmap, bitPixel, localColorMap);
            }
        }
        else {
            if (imageCount == imageNumber)
                CopyColormap(img_data->cmap, GifScreen.BitPixel,
                    GifScreen.ColorMap);
        }

        // get image data for this image
        if (imageCount == imageNumber) {
            int nread;
            // get image depth (= codeSize in GIF images)
            htmGifReadOK(ib, &c, 1);
            ib->depth = c;
            ib->next--;

            // decompress raster data
            image = inflateRaster(this, ib, img_data->width,
                img_data->height, &nread);

            if (image == 0 || (!BitSet(buf[8], INTERLACE) &&
                    nread != img_data->width * img_data->height)) {
                im_html->warning("readGIF",
                    "Read Error on image %s:\n    uncompress returned %i bytes "
                    "while %i bytes are expected.", ib->file, nread,
                    img_data->width * img_data->height);
                delete [] image;
                delete img_data;
                return (0);
            }

            // convert interlaced image data to normal image data
            if (BitSet(buf[8], INTERLACE))
                image = DoImage(image, img_data->width, img_data->height);
        }
        else
            SkipImage(ib);  // skip it

    }
    img_data->bg = Gif89.transparent;
    img_data->data = image;
    img_data->color_class = (grayscale != 0 ?
        IMAGE_COLORSPACE_GRAYSCALE : IMAGE_COLORSPACE_INDEXED);
    return (img_data);
}


// Decode LZW compressed raster data without using an LZW decoder by
// using the "compress" utilitity.
//
unsigned char*
htmImageManager::inflateGIFInternal(ImageBuffer *ib, int, int *nread)
{
    *nread = 0;

    // create a new stream object
    lzwStream *lzw = new lzwStream(ib, im_html->htm_zCmd);

    // set read functions
    lzw->setReadProc(htmGifReadOK);
    lzw->setDataProc(htmGifGetDataBlock);

    // initialize uncompression
    if ((lzw->init()) <= 0) {
        im_html->warning("inflateGIFInternal", lzw->getError());
        delete lzw;
        return (0);
    }

    // convert data
    lzw->convert();

    // get uncompressed data
    unsigned char *data;
    if ((data = lzw->uncompress(nread)) == 0)
        im_html->warning("inflateGIFInternal", lzw->getError());

    // destroy stream
    delete lzw;

    // and return uncompressed data
    return (data);
}


unsigned char*
htmImageManager::inflateGIFExternal(ImageBuffer *ib, int dsize, int *nread)
{
    *nread = 0;
#ifdef GIF_DECODE
    // get code size
    unsigned char c;
    htmGifReadOK(ib, &c, 1);

    // allocate output buffer
    unsigned char *buffer = new unsigned char[dsize + 1];
    memset(buffer, 0, dsize + 1);

    // initialize GIFStream object
    htmGIFStream gstream(c, buffer, dsize+1);

    // and call external decoder so it can initialize its own data
    // structures

    if ((decodeGIFImage(&gstream)) != GIF_STREAM_OK) {
        if (gstream.msg != 0) {
            im_html->warning("inflateGIFExternal",
                "decodeGIF initalization failed for image\n    %s:\n"
                "    %s.", ib->file,
                gstream.msg ? gstream.msg : "(unknown error)");
        }
        delete [] buffer;
        return (0);
    }
    gstream.state = GIF_STREAM_OK;

    // keep uncompressing until we reach the end of the compressed stream
    unsigned char buf[256];
    while (true) {
        // next compressed block of data
        gstream.avail_in  = htmGifGetDataBlock(ib, buf);
        if (!gstream.avail_in) {
            if (ib->next + 1 >= ib->size) {
                im_html->warning("inflateGIFExternal",
                    "external GIF decoder failed: %s.",
                    "(corrupt GIF file)");
                break;
            }
            continue;
        }

        gstream.next_in   = buf;
        // update output buffer
        gstream.next_out  = buffer + gstream.total_out;
        gstream.avail_out = dsize - gstream.total_out;

        // uncompress it
        int err = decodeGIFImage(&gstream);

        // check return value
        if (err != GIF_STREAM_OK && err != GIF_STREAM_END) {
            im_html->warning("inflateGIFExternal",
                "external GIF decoder failed: %s.",
                gstream.msg ? gstream.msg : "(unknown error)");
            break;
        }
        // and break if inflate has finished uncompressing our data
        if (err == GIF_STREAM_END || (int)gstream.total_out == dsize)
            break;
    }
    // total size of uncompressed data
    *nread = gstream.total_out;

    // skip remaining data
    while ((htmGifGetDataBlock(ib, buf)) > 0) ;

    // we have an external decoder, tell it to destroy itself
    gstream.state     = GIF_STREAM_END;
    gstream.next_out  = 0;
    gstream.avail_out = 0;
    gstream.next_in   = 0;
    gstream.avail_in  = 0;

    // call it
    decodeGIFImage(&gstream);

    // successfull uncompress, return uncompressed image data
    return (buffer);
#else
    (void)ib;
    (void)dsize;
    return (0);
#endif
}


// GIF/GZF raster data decompressor driver.  Selects the appropriate
// decompressor to use for decompressing the GIF/GZF LZW/deflate
// compressed raster data.
//
unsigned char*
htmImageManager::inflateRaster(htmImageManager*, ImageBuffer *ib,
    int width, int height, int *nread)
{
    *nread = 0;

    // Uncompress image data according to image type.  GZF images
    // always use the internal decoder, no overriding possible.  GIF
    // images can choose between the external or internal decoder.

    int dsize = width*height;   // estimated decompressed data size
    unsigned char *data;
    if (ib->type == IMAGE_GZF || ib->type == IMAGE_GZFANIM ||
            ib->type == IMAGE_GZFANIMLOOP)
        data = inflateGZFInternal(ib, dsize, nread);
    else {
#ifdef GIF_DECODE
            data = inflateGIFExternal(ib, dsize, nread);
#else
            // uncompress command to use for lzwStream
            data = inflateGIFInternal(ib, dsize, nread);
#endif
    }
    return (data);
}


// Uncompress deflated raster data.
//
unsigned char*
htmImageManager::inflateGZFInternal(ImageBuffer *ib, int dsize, int *nread)
{
    *nread = 0;
#ifndef HAVE_LIBZ
    (void)ib;
    (void)dsize;
    return (0);
#else

    // Skip code size, its never used
    unsigned char c;
    htmGifReadOK(ib, &c, 1);

    // allocate output buffer
    unsigned char *output_buf = new unsigned char[dsize + 1];
    memset(output_buf, 0, dsize + 1);

    // initialize inflate stuff
    z_stream stream;
    stream.zalloc = Z_NULL;
    stream.zfree  = Z_NULL;

    int err;
    if ((err = inflateInit(&stream)) != Z_OK) {
        im_html->warning("inflateGZFInternal",
            "inflateInit failed: %s.", stream.msg);
        delete [] output_buf;
        return (0);
    }

    // keep uncompressing until we reach the end of the compressed stream
    unsigned char buf[256];
    while (true) {
        // next compressed block of data
        stream.avail_in = htmGifGetDataBlock(ib, buf);
        stream.next_in  = buf;
        // update output buffer
        stream.next_out = output_buf + stream.total_out;
        stream.avail_out = dsize - stream.total_out;

        // uncompress it
        err = inflate(&stream, Z_PARTIAL_FLUSH);

        // check return value
        if (err != Z_OK && err != Z_STREAM_END) {
            im_html->warning("inflateGZFInternal",
                "inflate failed: %s.", stream.msg);
            delete [] output_buf;
            return (0);
        }
        // and break if inflate has finished uncompressing our data
        if (err == Z_STREAM_END)
            break;
    }

    // Skip remaining data.  The deflate format signals the end of
    // compressed data itself, so we never reach the zero-data block
    // in the above loop.

    while ((htmGifGetDataBlock(ib, buf)) > 0) ;

    // deallocate zstream data
    if ((inflateEnd(&stream)) != Z_OK) {
        im_html->warning("inflateGZFInternal",
            "inflateEnd failed: %s.",
            stream.msg ? stream.msg : "(unknown zlib error)");
    }
    *nread = stream.total_out;

    // successfull uncompress, return uncompressed image data
    return (output_buf);
#endif
}


namespace {
    // Skip past an image.
    //
    void
    SkipImage(ImageBuffer *ib)
    {
        // Initialize the Compression functions
        unsigned char c;
        if (!htmGifReadOK(ib, &c, 1))
            return;

        // skip image
        unsigned char buf[256];
        while ((htmGifGetDataBlock(ib, buf)) > 0) ;
    }


    // Read a GIF colormap.
    //
    bool
    ReadColorMap(ImageBuffer *ib, int number,
        unsigned char buffer[3][MAX_IMAGE_COLORS], int *gray)
    {
        int i, is_gray = 0;
        unsigned char rgb[3];
        for (i = 0; i < number; ++i) {
            if (!htmGifReadOK(ib, rgb, sizeof(rgb)))
                return (false);

            buffer[CM_RED][i]   = rgb[0];
            buffer[CM_GREEN][i] = rgb[1];
            buffer[CM_BLUE][i]  = rgb[2];
            is_gray &= (rgb[0] == rgb[1] && rgb[1] == rgb[2]);
        }
        // zero remainder
        for ( ; i < MAX_IMAGE_COLORS; i++) {
            buffer[CM_RED][i]   = 0;
            buffer[CM_GREEN][i] = 0;
            buffer[CM_BLUE][i]  = 0;
        }
        if (gray)
            *gray = is_gray;
        return (true);
    }


    // Copy the colormap from a gif image to a private colormap.
    //
    void
    CopyColormap(htmColor *colrs, int cmapSize,
        unsigned char cmap[3][MAX_IMAGE_COLORS])
    {
        // copy colormap
        for (int i = 0; i < cmapSize; i++) {
            colrs[i].red   = cmap[CM_RED][i];
            colrs[i].green = cmap[CM_GREEN][i];
            colrs[i].blue  = cmap[CM_BLUE][i];
        }
    }


    // Process a gif extension block.  Return gif type code upon success
    // or IMAGE_UNKNOWN upon error.
    //
    int
    DoExtension(ImageBuffer *ib, int label, GIF89 *gf)
    {
        unsigned char buf[256];
        int ret_val = IMAGE_GIF;

        switch (label) {
        case 0x01:      // Plain Text Extension
            break;
        case 0xff:      // Application Extension
            // Netscape Looping extension
            // Get first block

            htmGifGetDataBlock(ib, buf);
            if (!(strncmp((char*)buf, "NETSCAPE2.0", 11))) {
                ret_val = IMAGE_GIFANIMLOOP;
                if ((htmGifGetDataBlock(ib, buf)) <= 0)
                    ret_val = IMAGE_UNKNOWN;   // corrupt animation
                else
                    gf->loopCount = LM_to_uint(buf[1], buf[2]);
            }
            break;
        case 0xfe:      // Comment Extension
            while (htmGifGetDataBlock(ib, buf) > 0) ;
            return (ret_val);
        case 0xf9:      // Graphic Control Extension
            htmGifGetDataBlock(ib, buf);
            gf->disposal    = (buf[0] >> 2) & 0x7;
            gf->inputFlag   = (buf[0] >> 1) & 0x1;
            gf->delayTime   = LM_to_uint(buf[1], buf[2]);
            if ((buf[0] & 0x1) != 0)
                gf->transparent = buf[3];

            while (htmGifGetDataBlock(ib, buf) > 0) ;
            return (ret_val);
        default:
            break;
        }
        while (htmGifGetDataBlock(ib, buf) > 0) ;

        return (ret_val);
    }


    // Interlaced image.  Need to alternate uncompressed data to create
    // the actual image.
    //
    unsigned char *
    DoImage(unsigned char *data, int len, int height)
    {
        if (!data)
            return (0);

        unsigned char *dPtr = data;

        // allocate image storage
        unsigned char *image = new unsigned char[len*height];
        memset(image, 0, len*height);

        int ypos = 0;
        int pass = 0;
        int step = 8;
        for (int i = 0; i < height; i++) {
            if (ypos < height) {
                unsigned char *dp = &image[len * ypos];
                for (int xpos = 0; xpos < len; xpos++)
                    *dp++ = *dPtr++;
            }
            if ((ypos += step) >= height) {
                if (pass++ > 0)
                    step /= 2;
                ypos = step / 2;
            }
        }

        if (step != 1) {
            for (int i = 0; i < ypos; i += step) {
                unsigned char *src = &image[len * i];
                for (int nfill = 1;
                        nfill < step && i + nfill < height; nfill++) {
                    unsigned char *dest = &image[len * (i + nfill)];
                    memmove(dest, src, len);
                }
            }
        }

        // no longer needed
        delete [] data;
        return (image);
    }
}


//
// GIF to GZF converter functions
//

#ifdef HAVE_LIBZ

// Convert a CompuServe gif image to a GZF image.
//
bool
htmImageManager::GIFtoGZF(const char *infile, unsigned char *buf, int size,
    const char *outfile)
{
    bool ret_val = false;
    if (size == 0 && infile == 0)
        return (ret_val);

    ImageBuffer data, *ib = 0;
    if (size == 0) {
        if ((ib = imageFileToBuffer(infile)) == 0)
            return (ret_val);
    }
    else {
        if (buf != 0) {
            data.file = "<internally buffered image>";
            data.buffer = buf;
            data.size = size;
            data.next = 0;
            data.may_free = false;
            ib = &data;
        }
        else
            return (ret_val);
    }

    // and convert it
    ret_val = cvtGifToGzf(ib, outfile);

    // release buffer
    if (ib->may_free)
        delete ib;

    return (ret_val);
}


namespace {  void writeColormap(ImageBuffer*, FILE*, int); }

// Converts a Gif87a, Gif89a, to Gzf87a, Gzf89a.
//
bool
htmImageManager::cvtGifToGzf(ImageBuffer *ib, const char *file)
{
    FILE *fp;
    if ((fp = fopen(file, "w")) == 0) {
        perror(file);
        return (false);
    }

    // gif magic
    unsigned char buf[256];
    htmGifReadOK(ib, buf, 6);
    if (!(strncmp((char*)buf, "GIF87a", 6))) {
        strcpy((char*)buf, "GZF87a");
        buf[6] = '\0';
        WriteOK(fp, buf, 6);
    }
    else if (!(strncmp((char*)buf, "GIF89a", 6))) {
        strcpy((char*)buf, "GZF89a");
        buf[6] = '\0';
        WriteOK(fp, buf, 6);
    }
    else {
        fclose(fp);
        unlink(file);
        return (false);
    }

    // logical screen descriptor
    htmGifReadOK(ib, buf, 7);
    WriteOK(fp, buf, 7);

    if (BitSet(buf[4], LOCALCOLORMAP)) {
        // colormap has this many entries of 3 bytes
        writeColormap(ib, fp, 2 << (buf[4] & 0x07));
    }

    int done = 0;
    while (!done) {
        // block identifier
        unsigned char c;
        if (!htmGifReadOK(ib, &c, 1)) {
            done = -1;
            break;
        }
        // save it
        WriteOK(fp, &c, 1);

        // GIF terminator
        if (c == ';') {
            done = 1;
            break;
        }

        // Extension
        if (c == '!') {
            // error
            if (!htmGifReadOK(ib,&c,1)) {
                done = -1;
                break;
            }
            WriteOK(fp, &c, 1);

            int i;
            while ((i = htmGifGetDataBlock(ib, (unsigned char*)buf)) > 0) {
                WriteOK(fp, &i, 1);
                WriteOK(fp, buf, i);
            }
            // and write zero block terminator
            c = 0;
            WriteOK(fp, &c, 1);
            continue;
        }

        if (c != ',')
            continue;  // Not a valid start character

        // image descriptor
        if (!htmGifReadOK(ib, buf, 9)) {
            // error
            done = -1;
            break;
        }
        WriteOK(fp, buf, 9);

        // we have a local colormap
        if (BitSet(buf[8], LOCALCOLORMAP))
            writeColormap(ib, fp, 1 << ((buf[8] & 0x07) + 1));

        // width and height for this particular frame
        int w = LM_to_uint(buf[4], buf[5]);
        int h = LM_to_uint(buf[6], buf[7]);

        // get image codeSize
        htmGifReadOK(ib, &c, 1);
        int codeSize = (int)c;
        ib->next--;

        // and convert the image data
        unsigned char *image;
        int size;
        if ((image = inflateGIFInternal(ib, w*h, &size)) != 0) {

            // save codeSize as well
            WriteOK(fp, &codeSize, 1);

            // compress image data in one go

            // first allocate destination buffer
            unsigned long csize = size + (int)(0.15*size) + 12;
            unsigned char *compressed = new unsigned char[csize];

            if ((compress(compressed, &csize, image, size)) != Z_OK) {
                im_html->warning("cvtGifToGzf", "Error: compress failed!");
                delete [] compressed;
                // put block terminator
                int j = 0;
                WriteOK(fp, &j, 1);
                delete image;
                done = -1;
                break;
            }

            unsigned char *inPtr = compressed;

            // save image data in chunks of 255 bytes max.
            int j = 0;
            for (int i = 0; i < (int)csize; i++) {
                buf[j++] = *inPtr++;
                if (j == 254) {
                    putc(j & 0xff, fp);
                    WriteOK(fp, buf, 254);
                    j = 0;
                }
            }
            // flush out remaining data
            if (j) {
                WriteOK(fp, &j, 1);
                WriteOK(fp, buf, j);
            }

            // and write the block terminator
            j = 0;
            WriteOK(fp, &j, 1);

            delete [] compressed;

            // and free image data
            delete [] image;
        }
        else {
            done = -1;
            break;
        }
    }
    // close output file
    fclose(fp);

    // and remove if we had an error
    if (done == -1) {
        im_html->warning("cvtGifToGzf",
            "Error: %s is a corrupt GIF file.\n"
            "    Cannot convert to GZF format.", ib->file);
        unlink(file);
        return (false);
    }
    return (true);
}


namespace {
    void
    writeColormap(ImageBuffer *ib, FILE *fp, int nentries)
    {
        unsigned char rgb[3];
        for (int i = 0; i < nentries; i++) {
            htmGifReadOK(ib, (unsigned char*)rgb, sizeof(rgb));
            WriteOK(fp, rgb, sizeof(rgb));
        }
    }
}

#else

bool
htmImageManager::GIFtoGZF(const char*, unsigned char*, int, const char*)
{
    return (false);
}

#endif



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
#include "htm_image.h"
#include "htm_string.h"
#include <stdio.h>

#if defined(HAVE_LIBPNG) || defined(HAVE_LIBZ)
#include <zlib.h>
#endif

#define readByte(VAL) { \
    VAL = (*ib->curr_pos++ & 0xff); \
    ib->next++; }

#define readDimension(VAL) { \
    VAL = (*ib->curr_pos++ & 0xff); \
    VAL |= ((*ib->curr_pos++ & 0xff) << 8); \
    ib->next += 2; }

#define readCardinal(VAL) { \
    VAL = (*ib->curr_pos++ & 0xff); \
    VAL |= ((*ib->curr_pos++ & 0xff) << 8); \
    VAL |= ((*ib->curr_pos++ & 0xff) << 16); \
    VAL |= ((*ib->curr_pos++ & 0xff) << 24); \
    ib->next += 4; }

#define readInt(VAL) { \
    VAL = (*ib->curr_pos++ & 0xff); \
    VAL |= ((*ib->curr_pos++ & 0xff) << 8); \
    VAL |= ((*ib->curr_pos++ & 0xff) << 16); \
    VAL |= ((*ib->curr_pos++ & 0xff) << 24); \
    ib->next += 4; }


// Read a Fast Loadable Graphic directly into an htmImage.
//
htmImageInfo*
htmImageManager::readFLG(ImageBuffer *ib)
{
#if defined(HAVE_LIBPNG) || defined(HAVE_LIBZ)
    ImageBuffer data;
#endif
    htmImageInfo *image = 0;

    // sanity
    ib->rewind();

    // skip magic & version number
    ib->curr_pos += 7;
    ib->next += 7;

    // check if data is compressed
    unsigned char c;
    readByte(c);

    bool err = false;
    ImageBuffer *dp;
    unsigned char *buffer = 0;
    if (c == 1) {
#if !defined(HAVE_LIBPNG) && !defined(HAVE_LIBZ)
        // compressed FLG requires zlib support to be present
        im_html->warning("readFLG",
            "%s: can't uncompress:\n    Reason: zlib support not present.",
            ib->file);
        return (image);
#else

        // get size of uncompressed data
        unsigned int tmp;
        readCardinal(tmp);
        unsigned long dsize = (unsigned long)tmp;

        // size of compressed data
        unsigned long csize = (unsigned long)(ib->size - ib->next);

        // allocate uncompress buffer
        buffer = new unsigned char[dsize + 1];

        int zlib_err;
        if ((zlib_err = uncompress(buffer, &dsize, ib->curr_pos, csize))
                != Z_OK) {
            im_html->warning("readFLG",
                "%s: uncompress failed\n    Reason: %s.", ib->file,
                zlib_err == Z_MEM_ERROR ? "out of memory" :
                zlib_err == Z_BUF_ERROR ? "not enough output room" :
                "data corrupted");
            err = true;
        }
        else if (dsize != (unsigned long)tmp) {
            im_html->warning("readFLG",
                "%s: uncompress failed\n    %li bytes returned while %i "
                "are expected. Data corrupt?", ib->file,
                "%s: to uncompress data.", dsize, tmp);
            err = true;
        }
        // compose local image buffer
        data.buffer = buffer;
        data.curr_pos = data.buffer;
        data.size   = (size_t)dsize;
        data.next   = 0;
        data.may_free = false;
        dp = &data;
#endif
    }
    else
        dp = ib;

    if (err) {
        // free up compressed buffer
        if (c == 1)
            delete [] buffer;
        return (image);
    }

    // now go and process the actual image
    dp->rewind();

    image = readFLGprivate(dp);

    // store name of image
    image->url = lstring::copy(ib->file);  // image location

    // free up compressed buffer
    if (c == 1)
        delete [] buffer;

    return (image);
}


htmImageInfo*
htmImageManager::readFLGprivate(ImageBuffer *ib)
{
    // bogus frame leader
    htmImageInfo *all_frames = new htmImageInfo;
    all_frames->frame = 0;
    htmImageInfo *current = all_frames;

    // skip magic
    ib->next += 6;
    ib->curr_pos += 6;

    // FLG revision
    unsigned char c;
    readByte(c);

    // compress byte
    readByte(c);

    // original image type
    readByte(c);

    // no of frames in this file
    unsigned int nframes;
    readCardinal(nframes);

    // no of times to repeat animation
    unsigned int loop_count;
    readCardinal(loop_count);

    // read all frames
    int frameCount = 0;

    do {
        htmImageInfo *frame = new htmImageInfo;

        // default options
        frame->options =
            IMAGE_DEFERRED_FREE | IMAGE_RGB_SINGLE | IMAGE_ALLOW_SCALE;

        frame->type = IMAGE_FLG;

        // frames remaining
        frame->nframes = nframes - frameCount;
        frame->loop_count = loop_count;

        readDimension(frame->width);
        readDimension(frame->height);
        frame->swidth  = frame->width;
        frame->sheight = frame->height;

        // logical screen offsets (these can be negative)
        readInt(frame->x);
        readInt(frame->y);

        // frame timeout
        readCardinal(frame->timeout);

        // frame disposal method
        readByte(frame->dispose);

        // frame depth (original bits per pixel)
        readByte(frame->depth);

        // transparent pixel index
        readInt(frame->bg);

        // transparency type
        readByte(frame->transparency);

        // colorspace
        readByte(frame->colorspace);

        // size of colormap
        readCardinal(frame->ncolors);

        if (frame->ncolors) {
            // add one, it's zero based
            frame->ncolors++;
            frame->scolors = frame->ncolors;

            // alloc colormap
            frame->reds = new unsigned short[3*frame->ncolors];
            memset(frame->reds, 0, 3*frame->ncolors*sizeof(unsigned short));

            // read it
            unsigned short *color = frame->reds;

            // red color components
            for (unsigned int i = 0; i < frame->ncolors; i++, color++)
                readDimension(*color);

            // green color components
            frame->greens = color;
            for (unsigned int i = 0; i < frame->ncolors; i++, color++)
                readDimension(*color);

            // blue color components
            frame->blues = color;
            for (unsigned int i = 0; i < frame->ncolors; i++, color++)
                readDimension(*color);
        }

        // frame data
        frame->data = new unsigned char[frame->width * frame->height];
        unsigned char *data = frame->data;
        for (unsigned int i = 0; i < frame->width * frame->height; i++)
            readByte(*data++);

        // clipmask data (if we have got one)
        if (frame->bg != -1 && frame->transparency == IMAGE_TRANSPARENCY_BG) {

            int wd = frame->width;
            // pad so it will be a multiple of 8
            while ((wd % 8))
                wd++;

            // this many bytes on a row
            wd /= 8;
            // this many bytes in the clipmask
            wd *= frame->height;

            frame->clip = new unsigned char[wd];
            unsigned char *clip = frame->clip;
            for (int i = 0; i < wd; i++)
                readByte(*clip++);

            // we have a clipmask
            frame->options |= IMAGE_CLIPMASK;
        }

        // alpha channel (if any)
        if (frame->transparency == IMAGE_TRANSPARENCY_ALPHA) {

            // frame gamma
            unsigned gamma;
            readCardinal(gamma);
            frame->fg_gamma = (float)gamma/100000.0;

            // write out alpha channel
            frame->alpha = new unsigned char[frame->width * frame->height];
            unsigned char *alpha = frame->alpha;
            for (unsigned int i = 0; i < frame->width * frame->height; i++)
                readByte(*alpha++);
        }
        // terminator
        readByte(c);

        if (c != 0)
            fprintf(stderr, "readFLG: missing separator bit on frame %i!\n",
                frameCount);

        current->frame = frame;
        current = current->frame;

        frameCount++;   // done with this frame
    } while (frameCount < (int)nframes);

    current = all_frames->frame;
    delete all_frames;
    return (current);
}


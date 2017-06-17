
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
 * $Id: htm_BM.cc,v 1.4 2014/02/15 23:14:17 stevew Exp $
 *-----------------------------------------------------------------------*/

#include "htm_widget.h"
#include "htm_plc.h"

unsigned char htmImageManager::im_bitmap_bits[8] =
    { 1, 2, 4, 8, 16, 32, 64, 128 };

#define MAX_LINE    81


namespace {
    // Memory buffer version of fgets.  Returns buf filled with at most
    // max_len - 1 characters or 0 on end-of-buffer.
    //
    char *
    bgets(char *buf, int max_len, ImageBuffer *ib)
    {
        int len = 0;
        if (ib->size > ib->next) {
            unsigned char *chPtr = ib->buffer + ib->next;

            while (true) {
                if (len < max_len-1 && *chPtr != 0 && *chPtr != '\n' &&
                        ib->next + len < ib->size) {
                    len++;
                    chPtr++;
                }
                else
                    break;
            }
            // must include terminating character as well
            if (*chPtr == '\n' || *chPtr == 0)
                len++;
            memcpy(buf, ib->buffer + ib->next, len);
            buf[len] = 0;    // 0 terminate
            ib->next += len;
            return (buf);
        }
        return (0);
    }
}


// X11 Bitmap reading function.  Can't use any of the X[]Bitmap[]
// functions since we require the image data to be in ZPixmap format.
//
htmRawImageData*
htmImageManager::readBitmap(ImageBuffer *ib)
{
    // Initialize image data
    ib->depth = 2;

    // check bitmap header
    char line[MAX_LINE], name_and_type[MAX_LINE];
    int width = 0, height = 0;
    for ( ; ; ) {
        if (!(bgets(line, MAX_LINE, ib)))
            break;

        // see if we have successfully read a line
        if (strlen(line) == (MAX_LINE - 1))
            // probably not an xbm image, just return
            return (0);

        // check for xpm
        if (!(strcmp(line, "//* XPM *//"))) {
            // an xpm pixmap, return
            return (0);
        }

        // get width and height for this bitmap
        int value;
        if (sscanf(line, "#define %s %d", name_and_type, &value) == 2) {
            char *t;
            if (!(t = strrchr(name_and_type, '_')))
                t = name_and_type;
            else
                t++;
            if (!strcmp("width", t))
                width  = value;
            if (!strcmp("height", t))
                height = value;
            continue;
        }
        if (((sscanf(line, "static short %s = {", name_and_type)) == 1) ||
                ((sscanf(line,"static char * %s = {", name_and_type)) == 1)) {
            // not a bitmap, return
            return (0);
        }

        // last line of the bitmap header
        if (sscanf(line,"static char %s = [", name_and_type) == 1)
            break;
    }

    // bitmap has invalid dimension(s)
    if (width == 0 || height == 0)
        return (0);

    // allocate image
    htmRawImageData *img_data = new htmRawImageData(width, height, 2);

    img_data->color_class = IMAGE_COLORSPACE_GRAYSCALE;
    img_data->bg = -1;

    unsigned int fg_pixel = im_html->htm_cm.cm_body_fg;
    unsigned int bg_pixel = im_html->htm_cm.cm_body_bg;

    htmColor fg_color, bg_color;
    fg_color.pixel = fg_pixel;
    bg_color.pixel = bg_pixel;
    im_html->htm_tk->tk_query_colors(&fg_color, 1);
    im_html->htm_tk->tk_query_colors(&bg_color, 1);

    int blackbit = 0; // fg_color.pixel
    int whitebit = 1; // bg_color.pixel
    img_data->cmap[blackbit] = fg_color;
    img_data->cmap[whitebit] = bg_color;

    int bytes_per_line = ((img_data->width + 7) / 8);

    // size of a scan line
    int raster_length = bytes_per_line * img_data->height;

    // read bitmap data
    int cnt = 0;
    unsigned char *ptr = img_data->data;
    int lim = bytes_per_line * 8;
    for (int bytes = 0; bytes < raster_length; bytes++) {
        if (!(bgets(line, MAX_LINE, ib)))
            break;
        char *elePtr = line, *chPtr;;
        while ((chPtr = strstr(elePtr, ",")) != 0) {
            int value;
            if (sscanf(elePtr, " 0x%x%*[,}]%*[ \r\n]", &value) != 1) {
                delete img_data;
                return (0);
            }
            for (int i = 0; i < 8; i++) {
                if (cnt < (img_data->width)) {
                    if (value & im_bitmap_bits[i])
                        *ptr++ = blackbit;
                    else
                        *ptr++ = whitebit;
                }
                if (++cnt >= lim)
                    cnt = 0;
            }
            elePtr = chPtr + 1;
        }
    }
    return (img_data);
}


void
PLCImageXBM::Init()
{
    // this plc is active
    o_plc->plc_status = PLC_ACTIVE;

    // rewind input buffers, it may not be the first time we are doing this
    o_plc->RewindInputBuffer();

    // We don't know how large the raw data will be at this point,
    // allocate a default buffer so we can get at least the initial
    // image data.

    if (o_buf_size == 0) {
        o_buf_size = PLC_MAX_BUFFER_SIZE;
        o_buffer = new unsigned char[o_buf_size];
        memset(o_buffer, 0, o_buf_size);
    }

    // read some more data, appending at data we have already read
    if (o_buf_pos >= o_byte_count) {
        // read data from input but not more than we can take
        int value;
        if ((value = o_plc->plc_left) > (int)(o_buf_size - o_byte_count))
            value = o_buf_size - o_byte_count;

        int len;
        if ((len = o_plc->ReadOK(o_buffer + o_byte_count, value)) == 0)
            // end of data, suspended or aborted
            return;

        o_byte_count += len;
    }
    // rewind input buffer
    o_buf_pos = 0;

    // check bitmap header
    int width = 0, height = 0;
    char line[MAX_LINE], name_and_type[MAX_LINE];
    for ( ; ; ) {
        int len;
        if ((len = bgets(line, MAX_LINE)) == 0)
            return;

        // see if we have successfully read a line
        if (len == (MAX_LINE - 1)) {
            // probably not an xbm image, just return
            o_plc->plc_status = PLC_ABORT;
            return;
        }

        // check for xpm
        if (!(strcmp(line, "//* XPM *//"))) {
            // an xpm pixmap, return
            o_plc->plc_status = PLC_ABORT;
            return;
        }

        // get width and height for this bitmap
        int value;
        if (sscanf(line, "#define %s %d", name_and_type, &value) == 2) {
            char *t;
            if (!(t = strrchr(name_and_type, '_')))
                t = name_and_type;
            else
                t++;
            if (!strcmp("width", t))
                width  = value;
            if (!strcmp("height", t))
                height = value;
            continue;
        }
        if (((sscanf(line, "static short %s = {", name_and_type)) == 1) ||
                ((sscanf(line,"static char * %s = {", name_and_type)) == 1)) {
            // not a bitmap, return
            o_plc->plc_status = PLC_ABORT;
            return;
        }

        // last line of the bitmap header
        if (sscanf(line,"static char %s = [",name_and_type) == 1)
            break;
    }
    // start of real image data
    o_data_start = o_buf_pos;

    // bitmap has invalid dimension(s)
    if (width == 0 || height == 0) {
        o_plc->plc_status = PLC_ABORT;
        return;
    }

    o_width  = width;
    o_height = height;

    // two-color grayscale image
    o_colorclass = IMAGE_COLORSPACE_GRAYSCALE;
    o_cmapsize = 2;
    o_cmap = new htmColor[o_cmapsize];

    // image is initially fully opaque
    o_transparency = IMAGE_NONE;
    o_bg_pixel = (unsigned int)-1;

    // Resize incoming data buffer but don't touch buf_pos and
    // byte_count as the buffer already contains data.

    unsigned int ts = o_buf_size;
    unsigned char *tb = o_buffer;
    o_buf_size = o_width*o_height;
    o_buffer = new unsigned char[o_buf_size];
    memcpy(o_buffer, tb, ts < o_buf_size ? ts : o_buf_size);
    delete [] tb;

    // image data buffer
    o_data_size   = o_width*o_height;
    o_data_pos    = 0;
    o_prev_pos    = 0;
    o_data = new unsigned char[o_data_size + 1];
    memset(o_data, 0, o_data_size + 1);

    unsigned int fg_pixel = o_plc->plc_owner->im_html->htm_cm.cm_body_fg;
    unsigned int bg_pixel = o_plc->plc_owner->im_html->htm_cm.cm_body_bg;

    htmColor fg_color, bg_color;
    fg_color.pixel = fg_pixel;
    bg_color.pixel = bg_pixel;

    o_plc->plc_owner->im_html->htm_tk->tk_query_colors(&fg_color, 1);
    o_plc->plc_owner->im_html->htm_tk->tk_query_colors(&bg_color, 1);

    o_cmap[0] = fg_color;
    o_cmap[1] = bg_color;

    // no of bytes per line for this bitmap
    o_stride = ((o_width + 7) / 8);

    // full raw data size
    o_raster_length = o_stride * o_height;

    // object has been initialized
    o_plc->plc_initialized = true;
}


void
PLCImageXBM::ScanlineProc()
{
    // all data in buffer has been consumed, get some more
    if (o_buf_pos >= o_byte_count) {
        int value;
        if ((value = o_plc->plc_left) > (int)(o_buf_size - o_byte_count))
            value = o_buf_size - o_byte_count;

        // append at current position
        int len;
        if ((len = o_plc->ReadOK(o_buffer + o_byte_count, value)) == 0)
            // end of data, suspended or aborted
            return;

        // new data length
        o_byte_count += len;
    }
    // raw image data start
    o_buf_pos = o_data_start;

    // decoded image data start
    unsigned char *ptr = o_data;
    o_data_pos = 0;
    int lim = o_stride * 8;

    int cnt = 0;
    char line[MAX_LINE];
    for (int bytes = 0; bytes < o_raster_length &&
            o_data_pos < o_data_size; bytes++) {

        // read a new line of data
        int len;
        if ((len = bgets(line, MAX_LINE)) == 0) {
            // last known full scanline
            o_data_pos = (o_data_pos > o_data_size ?
                o_data_size : o_data_pos);

            // all data processed
            if (o_plc->plc_data_status == STREAM_END)
                break;

            // suspended or aborted
            return;
        }

        char *elePtr = line, *chPtr;
        while ((chPtr = strstr(elePtr, ",")) != 0) {
            int value;
            if (sscanf(elePtr, " 0x%x%*[,}]%*[ \r\n]", &value) != 1) {
                o_plc->plc_status = PLC_ABORT;
                return;
            }
            for (int i = 0; i < 8; i++) {
                if (cnt < (int)o_width) {
                    if (value & o_plc->plc_owner->im_bitmap_bits[i])
                        *ptr++ = 0; // blackbit
                    else
                        *ptr++ = 1; // whitebit
                    o_data_pos++;
                }
                // processed a full scanline
                if (++cnt >= lim)
                    cnt = 0;
            }
            elePtr = chPtr + 1;
        }
    }
    // done with this image
    o_plc->plc_obj_funcs_complete = true;
}


// Progressive Bitmap loading routines.
//
int
PLCImageXBM::bgets(char *buf, int max_len)
{
    int len = 0;
    if (o_buf_pos < o_byte_count) {
        unsigned char *chPtr = o_buffer + o_buf_pos;

        while (true) {
            if (len < max_len-1 && *chPtr && *chPtr != '\n' &&
                    *chPtr != '}' && o_buf_pos + len < o_byte_count) {
                len++;
                chPtr++;
            }
            else
                break;
        }
        // must include terminating character as well
        if (*chPtr == '\n' || *chPtr == '}' || *chPtr == '\0')
            len++;

        memcpy(buf, o_buffer + o_buf_pos, len);
        buf[len] = 0;    // 0 terminate
        o_buf_pos += len;
        return (len);
    }

    // not enough data, make a new request
    o_plc->plc_min_in = 0;
    o_plc->plc_max_in = o_plc->plc_input_size;
    o_plc->DataRequest();

    return (0);
}


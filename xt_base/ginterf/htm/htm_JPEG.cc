
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
 * $Id: htm_JPEG.cc,v 1.5 2014/02/15 23:14:17 stevew Exp $
 *-----------------------------------------------------------------------*/

#include "htm_widget.h"
#include "htm_image.h"
#include <stdio.h>

#ifdef HAVE_LIBJPEG
extern "C" {
#include <jpeglib.h>
}
#include <setjmp.h>

namespace htm
{
    struct my_error_mgr
    {
        struct jpeg_error_mgr pub;  // "public" fields
        jmp_buf setjmp_buffer;      // for return to caller
    };
    typedef my_error_mgr *my_error_ptr;

    struct buffer_source_mgr
    {
        jpeg_source_mgr pub;  // public fields
        JOCTET *buffer;
    };
    typedef buffer_source_mgr *buffer_src_ptr;
}

namespace {
    JOCTET jpeg_EOI_buffer[2];

    void
    my_error_exit(j_common_ptr cinfo)
    {
        my_error_ptr myerr = (my_error_ptr)cinfo->err;
        longjmp(myerr->setjmp_buffer, 1);
    }


    void
    jpeg_buffer_init_source(j_decompress_ptr)
    {
        // This should really only be done once
        jpeg_EOI_buffer[0]=(JOCTET)0xFF;
        jpeg_EOI_buffer[1]=(JOCTET)JPEG_EOI;
    }


    // We cant give any more data, because all we know about the image is
    // already in the buffer!
    //
    // Therefore, if fill_input_buffer is called, we have a bogus JPEG
    // image, and all we can do is try and fake some EOIs.
    //
    boolean
    jpeg_buffer_fill_input_buffer(j_decompress_ptr cinfo)
    {
        buffer_src_ptr src=(buffer_src_ptr)cinfo->src;
        src->buffer=jpeg_EOI_buffer;
        src->pub.next_input_byte=src->buffer;
        src->pub.bytes_in_buffer=2;
        return (true);
    }


    // Skip over uninteresting data in the JPEG stream.  If we have to
    // seek past the end of our buffer, then we have a bogus image.  The
    // call to jpeg_buffer_fill_input_buffer handles this case for us.
    //
    void
    jpeg_buffer_skip_input_data(j_decompress_ptr cinfo, long num_bytes)
    {
        buffer_src_ptr src = (buffer_src_ptr)cinfo->src;
        if (num_bytes > 0) {
            // we have been told to seek off the end of the image!
            if (num_bytes>(long)src->pub.bytes_in_buffer)
                jpeg_buffer_fill_input_buffer(cinfo);
            else {
                src->pub.next_input_byte+=(size_t)num_bytes;
                src->pub.bytes_in_buffer-=(size_t)num_bytes;
            }
        }
    }


    void
    jpeg_buffer_term_source(j_decompress_ptr)
    {
        // no-op
    }


    // Set up input from a memory buffer.
    //
    void
    jpeg_buffer_src(j_decompress_ptr cinfo, unsigned char *data,
        unsigned int len)
    {

        if (cinfo->src == 0) {
            // first time for this JPEG object
            cinfo->src = (struct jpeg_source_mgr*)
                (*cinfo->mem->alloc_small) ((j_common_ptr)cinfo,
                    JPOOL_PERMANENT, sizeof(buffer_source_mgr));
        }

        buffer_src_ptr src = (buffer_src_ptr)cinfo->src;
        src->buffer = (JOCTET*)data;
        src->pub.init_source = jpeg_buffer_init_source;
        src->pub.fill_input_buffer = jpeg_buffer_fill_input_buffer;
        src->pub.skip_input_data = jpeg_buffer_skip_input_data;
        src->pub.resync_to_restart = jpeg_resync_to_restart;  // default
        src->pub.term_source = jpeg_buffer_term_source;
        src->pub.bytes_in_buffer = len;
        src->pub.next_input_byte = data;
    }
}


// Read a JPEG buffer and returns image data.
//
htmRawImageData*
htmImageManager::readJPEG(ImageBuffer *ib)
{
    // We set up the normal JPEG error routines, then override error_exit.
    jpeg_decompress_struct cinfo;
    my_error_mgr jerr;
    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = my_error_exit;

    // Establish the initial setjmp return context for my_error_exit
    // to use.
    if (setjmp(jerr.setjmp_buffer)) {

        // JPEG signalled an error.  Free any allocated buffers and
        // return 0.

        jpeg_destroy_decompress(&cinfo);
        return (0);
    }

    jpeg_create_decompress(&cinfo);

    jpeg_buffer_src(&cinfo, ib->buffer, ib->size);

    jpeg_read_header(&cinfo, true);

    cinfo.quantize_colors = true;
    cinfo.two_pass_quantize = true;

    // Get configuration defaults: color quantization and gamma correction.
    // color quantization
    cinfo.desired_number_of_colors = im_html->htm_max_image_colors;

    // gamma correction
    cinfo.output_gamma = im_html->htm_screen_gamma;

    // no dither selection for gtkhtm itself. Always use FS
    cinfo.dither_mode = JDITHER_FS;

    jpeg_start_decompress(&cinfo);

    int row_stride = cinfo.output_width * cinfo.output_components;

    // fix 03/24/97-01, rr
    // allocate raw image
    htmRawImageData *img_data = new htmRawImageData(cinfo.output_height,
        row_stride);

    // Establish a new setjmp return context for my_error_exit to use
    // which destroys img_data.
    if (setjmp(jerr.setjmp_buffer)) {

        // JPEG signalled an error.  Destroy image data, free any
        // allocated buffers and return 0.

        jpeg_destroy_decompress(&cinfo);
        delete img_data;
        return (0);
    }

    unsigned char *r = img_data->data;

    JSAMPROW buffer[1];         // row pointer array for read_scanlines
    while (cinfo.output_scanline < cinfo.output_height) {
        buffer[0] = r;
        jpeg_read_scanlines(&cinfo, buffer, 1);
        r += row_stride;
    }

    // update raw image data
    img_data->width  = cinfo.output_width;
    img_data->height = cinfo.output_height;
    ib->depth = cinfo.data_precision;

    // add colormap
    img_data->allocCmap(cinfo.actual_number_of_colors);

    // set up X colormap. Upscale RGB to 16bits precision
    if (cinfo.out_color_components == 3) {
        int cshift = 16 - cinfo.data_precision;
        img_data->color_class = IMAGE_COLORSPACE_RGB;
        for (int i = 0; i < img_data->cmapsize; i++) {
            img_data->cmap[i].red   = cinfo.colormap[0][i] << cshift;
            img_data->cmap[i].green = cinfo.colormap[1][i] << cshift;
            img_data->cmap[i].blue  = cinfo.colormap[2][i] << cshift;
        }
    }
    else {
        int cshift = 16 - cinfo.data_precision;
        img_data->color_class = IMAGE_COLORSPACE_GRAYSCALE;
        for (int i = 0; i < img_data->cmapsize; i++) {
            img_data->cmap[i].red = img_data->cmap[i].green =
                img_data->cmap[i].blue = cinfo.colormap[0][i] << cshift;
        }
    }

    // as we are now sure every color values lies within the range
    // 0-2^16, we can downscale to the range 0-255

    for (int i = 0; i < img_data->cmapsize; i++) {
        img_data->cmap[i].red   >>= 8;
        img_data->cmap[i].green >>= 8;
        img_data->cmap[i].blue  >>= 8;
    }

    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);

    return (img_data);
}

#else


htmRawImageData*
htmImageManager::readJPEG(ImageBuffer*)
{
    return (0);
}

#endif


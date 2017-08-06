
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
#include "htm_image.h"

#ifdef _AIX
#define jmpbuf __jmpbuf
#endif

#ifdef HAVE_LIBPNG
#include <png.h>
#include <setjmp.h>
#include <math.h>
#include <stdlib.h>


#define STORE_COLOR(R,G,B,P) { \
    img_data->cmap[P].red = (R); img_data->cmap[P].green = (G); \
    img_data->cmap[P].blue = (B); img_data->cmap[P].pixel = P; }

// background gamma correction used in alpha channel processing
#define BG_GAMMA_CORRECTION     2.2222222222

// the maximum value a color component can have
#define MAX_RGB_VAL 255


namespace {
    // png error function.  This routine displays a warning message and
    // terminates PNG reading by jumping to the point where the error
    // function was set.
    //
    void
    my_png_error(png_structp png_ptr, const char *msg)
    {
        ImageBuffer *ib = (ImageBuffer*)png_get_io_ptr(png_ptr);
        if (ib->buffer)
            strncpy((char*)ib->buffer, msg, ib->size);
        longjmp(png_jmpbuf(png_ptr), 1);
    }


    // Function called by png when it needs another chunk of data.  This
    // function is used instead of the default png reader (which uses
    // fread) as we have the file data in memory.
    //
    void
    my_png_read(png_structp png_ptr, png_bytep data, png_size_t len)
    {
        ImageBuffer *ib = (ImageBuffer*)png_get_io_ptr(png_ptr);
        int size = (int)len;
        if (ib->size > ib->next) {
            if (ib->next + size > ib->size)
                size = ib->size - ib->next;
            memcpy(data, ib->buffer + ib->next, size);
            ib->next += size;
            return;
        }
        my_png_error(png_ptr, "Read Error");
    }
}


// Read a PNG (Portable Network Graphics) image.
//
htmRawImageData*
htmImageManager::readPNG(ImageBuffer *ib)
{
    char msg[128];

    // get configuration defaults
    volatile double gamma = im_html->htm_screen_gamma;

    // We set up the normal PNG error routines, then override with longjmp.
    png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
        0, 0, 0);

    // create and initialize the info structure
    png_infop info_ptr;
    if ((info_ptr = png_create_info_struct(png_ptr)) == 0) {
        // failed, too bad
        png_destroy_read_struct(&png_ptr, 0, 0);
        return (0);
    }

    // set initial error handler
    if (setjmp(png_jmpbuf(png_ptr))) {

        // PNG signaled an error.  Destroy image data, free any
        // allocated buffers and return 0.

        ImageBuffer *ibtmp = (ImageBuffer*)png_get_io_ptr(png_ptr);
        im_html->warning("png_error",
            "libpng error on image %s: %s.", ibtmp->file,
            ibtmp->buffer ? (char*)ibtmp->buffer : (char*)"unknown error");

        png_destroy_read_struct(&png_ptr, &info_ptr, 0);
        return (0);
    }

     // We have the entire image in memory, so we need to set our own
     // ``fread'' function for libpng to use.

    png_set_read_fn(png_ptr, ib, &my_png_read);

    // by now we already know this is a png
    ib->next = 8;

    png_set_sig_bytes(png_ptr, 8);

    // get png info
    png_read_info(png_ptr, info_ptr);

    // allocate raw image data
    htmRawImageData *img_data = new htmRawImageData();

    // set error handler to destroy img_data.
    if (setjmp(png_jmpbuf(png_ptr))) {

        // PNG signaled an error.  Destroy image data, free any
        // allocated buffers and return 0.

        ImageBuffer *ibtmp = (ImageBuffer*)png_get_io_ptr(png_ptr);
        im_html->warning("png_error",
            "libpng error on image %s: %s.", ibtmp->file,
            ibtmp->buffer ? (char*)ibtmp->buffer : (char*)"unknown error");

        png_destroy_read_struct(&png_ptr, &info_ptr, 0);
        delete img_data;
        return (0);
    }

    // save width & height
    int width  = img_data->width  = png_get_image_width(png_ptr, info_ptr);
    int height = img_data->height = png_get_image_height(png_ptr, info_ptr);

    // image depth
    ib->depth = png_get_bit_depth(png_ptr, info_ptr);

    // no of colors
    png_colorp palette;
    int ncolors;
    png_get_PLTE(png_ptr, info_ptr, &palette, &ncolors);

    // type of image
    int color_type = png_get_color_type(png_ptr, info_ptr);

    // The fun stuff.  This is based on readPNG by Greg Roelofs as
    // found in xpaint 2.4.8, which falls under the same distribution
    // note as readGIF by David Koblas (which is the original author
    // of xpaint).  It has been quite heavily modified but it was an
    // invaluable starting point!

    struct pdata
    {
        unsigned char *data;
        bool has_alpha;
        bool has_cmap;
        bool do_gamma;
    };
    pdata pd = pdata();

    switch (color_type) {
    case PNG_COLOR_TYPE_PALETTE:
        img_data->color_class = IMAGE_COLORSPACE_INDEXED;

        // Paletted images never contain more than 256 colors but
        // check anyway.

        if (ncolors > 256) {
            sprintf(msg, "PNG_COLOR_TYPE_PALETTE: %i colors reported "
                "while max is 256.", ncolors);
            my_png_error(png_ptr, msg);
        }

        // Paletted images with transparency info are expanded to RGB
        // with alpha channel.  Actual image creation is postponed
        // until the image is needed.

        if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) {
            png_set_expand(png_ptr);
            pd.has_alpha = true;
            pd.do_gamma = false;
        }
        else {
            // store colormap and allocate buffer for read image data
            img_data->allocCmap(ncolors);
            for (int i = 0; i < ncolors; i++) {
                img_data->cmap[i].red   = palette[i].red;
                img_data->cmap[i].green = palette[i].green;
                img_data->cmap[i].blue  = palette[i].blue;
            }
            pd.has_cmap = true;
            pd.data = new unsigned char[width*height];
        }
        break;

    case PNG_COLOR_TYPE_RGB:
        img_data->color_class = IMAGE_COLORSPACE_RGB;
        if (ib->depth == 16) {
            png_set_strip_16(png_ptr);
            ib->depth = 8;
        }
        // image data
        pd.data = new unsigned char[width*height*3];
        break;

    case PNG_COLOR_TYPE_GRAY:
        img_data->color_class = IMAGE_COLORSPACE_GRAYSCALE;
        if (ib->depth == 16) {
            png_set_strip_16(png_ptr);
            ib->depth = 8;
        }

        // Grayscale with transparency is expanded to RGB with alpha
        // channel.

        if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) {
            png_set_gray_to_rgb(png_ptr);
            png_set_expand(png_ptr);
            pd.has_alpha = true;
            pd.do_gamma = false;
            break;
        }

        // fill in appropriate grayramp
        switch (ib->depth) {
        case 1:
            // allocate colormap
            ncolors = 2;
            img_data->allocCmap(ncolors);
            // fill it
            STORE_COLOR(0, 0, 0, 0);
            STORE_COLOR(255, 255, 255, 1);
            break;
        case 2:
            // allocate colormap
            ncolors = 4;
            img_data->allocCmap(ncolors);
            // fill it
            STORE_COLOR(0, 0, 0, 0);
            STORE_COLOR(85,  85,  85,  1);  // 255/3
            STORE_COLOR(170, 170, 170, 2);
            STORE_COLOR(255, 255, 255, 3);
            break;
        case 4:
            // allocate colormap
            ncolors = 16;
            img_data->allocCmap(ncolors);
            // fill it
            for (int i = 0;  i < 16;  ++i) {
                int idx = i * 17;  // 255/15
                STORE_COLOR(idx, idx, idx, i);
            }
            break;
        case 8:
            // allocate colormap
            ncolors = 256;
            img_data->allocCmap(ncolors);
            // fill it
            for (int i = 0;  i < 256;  ++i)
                STORE_COLOR(i, i, i, i);
            break;
        }
        // valid colormap
        pd.has_cmap = true;

        // image data
        pd.data = new unsigned char[width*height];
        break;

    case PNG_COLOR_TYPE_RGB_ALPHA:
        img_data->color_class = IMAGE_COLORSPACE_RGB;
        pd.do_gamma = false;
        pd.has_alpha = true;

        // strip 16bit down to 8bits
        if (ib->depth == 16)
            png_set_strip_16(png_ptr);
        break;

    case PNG_COLOR_TYPE_GRAY_ALPHA:
        img_data->color_class = IMAGE_COLORSPACE_GRAYSCALE;
        pd.do_gamma = false;
        pd.has_alpha = true;

        // expand to rgb
        png_set_gray_to_rgb(png_ptr);
        break;

    default:
        sprintf(msg, "bad PNG image: unknown color type (%d)",
            png_get_color_type(png_ptr, info_ptr));
        my_png_error(png_ptr, msg);
        break;
    }

    // set error handler to destroy img_data and pd.data.
    if (setjmp(png_jmpbuf(png_ptr))) {

        // PNG signaled an error.  Destroy image data, free any
        // allocated buffers and return 0.

        ImageBuffer *ibtmp = (ImageBuffer*)png_get_io_ptr(png_ptr);
        im_html->warning("png_error",
            "libpng error on image %s: %s.", ibtmp->file,
            ibtmp->buffer ? (char*)ibtmp->buffer : (char*)"unknown error");

        png_destroy_read_struct(&png_ptr, &info_ptr, 0);
        delete img_data;
        delete [] pd.data;
        return (0);
    }

    // We only substitute background pixel if we don't have an alpha
    // channel Doing that for alpha channel images would change the
    // colortype of the current image, leading to weird results.

    if (!pd.has_alpha && png_get_valid(png_ptr, info_ptr, PNG_INFO_bKGD)) {
        png_color_16p background;
        png_get_bKGD(png_ptr, info_ptr, &background);
        png_set_background(png_ptr, background, PNG_BACKGROUND_GAMMA_FILE,
            1, 1.0);
        // not sure that this is right
        if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
            img_data->bg = background->index;
    }

    // handle gamma correction
    double fg_gamma;
    if (png_get_valid(png_ptr, info_ptr, PNG_INFO_gAMA))
        png_get_gAMA(png_ptr, info_ptr, &fg_gamma);
    else
        fg_gamma = 0.45;

    // set it
    if (pd.do_gamma)
        png_set_gamma(png_ptr, gamma, fg_gamma);

    // dithering gets handled by caller

    // one byte per pixel
    if (png_get_bit_depth(png_ptr, info_ptr) < 8)
        png_set_packing(png_ptr);

    // no tRNS chunk handling, we've expanded it to an alpha channel

    // handle interlacing
    if (png_get_interlace_type(png_ptr, info_ptr))
        png_set_interlace_handling(png_ptr);

    // and now update everything
    png_read_update_info(png_ptr, info_ptr);

    // has possibly changed if we have promoted GrayScale or tRNS chunks
    color_type = png_get_color_type(png_ptr, info_ptr);

    // new color_type?
    if (color_type == PNG_COLOR_TYPE_RGB_ALPHA) {
        png_bytep png_data;

        // RGB image with Alpha channel.  Actual decoding of this
        // image is delayed until it's required.
        //
        // This approach is needed as at this point we don't know the
        // position of the image in the document and hence we can't
        // correctly handle proper alpha channel processing.
        //
        // Setting the delayed_creation flag instructs gtkhtm to
        // create an empty htmImage structure and delays the actual
        // image composition until the position of this image is
        // known.  At that point the painter will call doAlphaChannel
        // to do the actual image creation.

        png_bytep *row_ptrs = new png_bytep[height];
        png_data = new png_byte[height*png_get_rowbytes(png_ptr, info_ptr)];

        for (int i = 0; i < height; i++)
            row_ptrs[i] = (png_bytep)png_data + i*png_get_rowbytes(png_ptr,
                info_ptr);

        // read it
        png_read_image(png_ptr, row_ptrs);

        img_data->data = png_data;          // raw image data

        // no longer needed
        delete [] row_ptrs;

        // set flag so gtkhtm will know what to do with this image
        img_data->delayed_creation = true;

        // PNG cleanup
        png_read_end(png_ptr, info_ptr);
        png_destroy_read_struct(&png_ptr, &info_ptr, 0);

        // store image gamma value. We need it later on
        img_data->fg_gamma = fg_gamma;
        img_data->type = IMAGE_PNG;
        return (img_data);
    }

    // no alpha channel
    png_bytep *row_ptrs = new png_bytep[height];

    for (int i = 0; i < height; ++i)
        row_ptrs[i] = (png_bytep)pd.data + i*png_get_rowbytes(png_ptr, info_ptr);

    // read it
    png_read_image(png_ptr, row_ptrs);

    // no longer needed
    delete [] row_ptrs;

    // We're lucky: having a colormap means we have an indexed image.

    if (pd.has_cmap) {
        img_data->data = pd.data;
        pd.data = 0;
    }
    else {
        // RGB image.  Convert to paletted image.  First allocate a
        // buffer which will receive the final image data.
        img_data->data = new unsigned char[width*height];
        convert24to8(pd.data, img_data,
            im_html->htm_max_image_colors, im_html->htm_rgb_conv_mode);

        // and we no longer need the image data itself
        delete [] pd.data;
        pd.data = 0;
    }

    // PNG cleanup
    png_read_end(png_ptr, info_ptr);
    png_destroy_read_struct(&png_ptr, &info_ptr, 0);

    return (img_data);
}


// Rereads a PNG (Portable Network Graphics) image.  This function is
// only called for RGB images with a tRNS chunk or alpha channel
// support.  It's purpose is to reprocess the raw image data so we can
// properly deal with the alpha channel.  For overall PNG comments see
// the above routine.
//
htmRawImageData*
htmImageManager::reReadPNG(htmRawImageData *raw_data, int x, int y,
    bool is_body_image)
{
    // various alpha channel processing variables

    // background alpha channel stuff
    AlphaChannelInfo *alpha_buffer = im_alpha_buffer;

    // display gamma
    double gamma = im_html->htm_screen_gamma;
    // file gamma
    double fg_gamma = raw_data->fg_gamma;

    // If we do not have a body image (or this image *is* the body
    // image), use the background color.  Else we need to get the RGB
    // components of the colors used by the background image.

    int background[3];              // background pixel: R, G, B
    htmRawImageData *img_data;  // return data
    htmImage *bg_image = 0;
    int bg_width = 0;
    int bg_height = 0;
    unsigned char *bg_data = 0;
    if (!is_body_image && alpha_buffer->ncolors) {
        bg_image  = im_body_image;
        bg_width  = bg_image->width;
        bg_height = bg_image->height;
        bg_data   = bg_image->html_image->data;
    }
    else {
        background[0] = alpha_buffer->background[0];
        background[1] = alpha_buffer->background[1];
        background[2] = alpha_buffer->background[2];
    }

    // get image width
    int width  = raw_data->width;
    int height = raw_data->height;

    img_data = new htmRawImageData(width, height);

    // raw png image data (source buffer)
    unsigned char *png = raw_data->data;
    // intermediate destination buffer, contains alpha-corrected values
    unsigned char *currLine = new unsigned char[width*height*3];
    unsigned char *rgb = currLine;

    // Actual alpha channel processing.  This code is based on the
    // alpha channel example in the official W3C PNG Recommendation
    // which uses floating point all over the place.
    //
    // Performance increases can be gained by:
    // - using lookup tables;
    // - handling body-image/body-color separate;
    // - integrate dithering.

    int foreground[4];              // image pixel: R, G, B, A
    int fbpix[3];                   // final image pixel: R, G, B
    for (int i = 0; i < height; i++) {
        // do a single scanline
        for (int j = 0; j < width; ++j) {
            // tile position
            if (bg_data) {
                // compute correct tile offset
                int dx = (j+x) % bg_width;
                int dy = (i+y) % bg_height;
                int id = dy*bg_width + dx;
                int idx = (int)bg_data[id];
                background[0] = alpha_buffer->bg_cmap[idx].red;
                background[1] = alpha_buffer->bg_cmap[idx].green;
                background[2] = alpha_buffer->bg_cmap[idx].blue;
            }

            // get foreground color
            foreground[0] = *png++;     // red
            foreground[1] = *png++;     // green
            foreground[2] = *png++;     // blue
            foreground[3] = *png++;     // alpha

            // Get integer version of alpha.  Check for opaque and
            // transparent special cases; no compositing needed if so.
            //
            // We show the whole gamma decode/correct process in
            // floating point, but it would more likely be done with
            // lookup tables.

            double alpha, compalpha;
            double gamfg, linfg, gambg, linbg, comppix, gcvideo;
            int ialpha = foreground[3];
            if (ialpha == 0) {
                // Foreground is transparent, replace with background.
                fbpix[0] = background[0];
                fbpix[1] = background[1];
                fbpix[2] = background[2];
            }
            else if (ialpha == MAX_RGB_VAL) {
                // Copy foreground pixel to frame buffer.
                for (int k = 0; k < 3; k++) {
                    gamfg    = (double)foreground[k] / MAX_RGB_VAL;
                    linfg    = pow(gamfg, 1.0/fg_gamma);
                    comppix  = linfg;
                    gcvideo  = pow(comppix, 1.2/gamma);
                    fbpix[k] = (int) (gcvideo * MAX_RGB_VAL + 0.5);
                }
            }
            else {
                // Compositing is necessary.  Get floating-point alpha
                // and its complement.  Note:  alpha is always linear;
                // gamma does not affect it.

                alpha = (double)ialpha / MAX_RGB_VAL;
                compalpha = 1.0 - alpha;

                for (int k = 0; k < 3; k++) {
                    // Convert foreground and background to floating
                    // point, then linearize (undo gamma encoding).

                    gamfg = (double)foreground[k] / MAX_RGB_VAL;
                    linfg = pow(gamfg, 1.0/fg_gamma);
                    gambg = (double)background[k] / MAX_RGB_VAL;
                    linbg = pow(gambg, BG_GAMMA_CORRECTION);

                    // Composite
                    comppix = linfg * alpha + linbg * compalpha;

                    // Gamma correct for display.  Convert to integer
                    // frame buffer pixel.  We assume a viewing gamma
                    // of 1.2

                    gcvideo = pow(comppix, 1.2/gamma);
                    fbpix[k] = (int) (gcvideo * MAX_RGB_VAL + 0.5);
                }
            }
            // and store it
            *rgb++ = (unsigned char)fbpix[0];
            *rgb++ = (unsigned char)fbpix[1];
            *rgb++ = (unsigned char)fbpix[2];
        }
    }
    // and reduce to 8bit paletted image
    convert24to8(currLine, img_data, im_html->htm_max_image_colors,
        im_html->htm_rgb_conv_mode);
    delete [] currLine;

    // copy untouched fields
    img_data->bg   = raw_data->bg;
    img_data->type = raw_data->type;
    img_data->color_class = raw_data->color_class;

    return (img_data);
}

#else


htmRawImageData*
htmImageManager::readPNG(ImageBuffer*)
{
    return (0);
}


htmRawImageData*
htmImageManager::reReadPNG(htmRawImageData*, int, int, bool)
{
    return (0);
}

#endif


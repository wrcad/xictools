
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
 * IMSAVE Image Dump Facility
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

/*------------------------------------------------------------------------*
 *
 * IMSAVE -- Screen Dump Utility
 *
 * S. R. Whiteley (stevew@wrcad.com)
 *------------------------------------------------------------------------*
 * Borrowed extensively from Imlib-1.9.8
 *
 * This software is Copyright (C) 1998 By The Rasterman (Carsten
 * Haitzler).  I accept no responsability for anything this software
 * may or may not do to your system - you use it completely at your
 * own risk.  This software comes under the LGPL and GPL licences.
 * The library itself is LGPL (all software in Imlib and gdk_imlib
 * directories) see the COPYING and COPYING.LIB files for full legal
 * details.
 *
 * (Rasterdude :-) <raster@redhat.com>)
 *------------------------------------------------------------------------*/

#include "imsave.h"

#ifdef HAVE_LIBPNG
#include <stdio.h>
#include <stdlib.h>
#include <png.h>


bool
Image::savePNG(const char *file, SaveInfo*)
{
    FILE *f = fopen(file, "wb");
    if (!f)
        return (false);

    png_structp png_ptr =
        png_create_write_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
    if (!png_ptr) {
        fclose(f);
        return (false);
    }
    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == NULL) {
        fclose(f);
        png_destroy_write_struct(&png_ptr, 0);
        return (false);
    }
    if (setjmp(png_jmpbuf(png_ptr))) {
        fclose(f);
        png_destroy_write_struct(&png_ptr, (png_infopp) NULL);
        return (false);
    }
    png_init_io(png_ptr, f);
    png_set_IHDR(png_ptr, info_ptr, rgb_width, rgb_height, 8,
        PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
    png_color_8 sig_bit;
    sig_bit.red = 8;
    sig_bit.green = 8;
    sig_bit.blue = 8;
    sig_bit.alpha = 8;
    png_set_sBIT(png_ptr, info_ptr, &sig_bit);
    png_write_info(png_ptr, info_ptr);
    png_set_shift(png_ptr, &sig_bit);
    png_set_packing(png_ptr);

    // use malloc to deal with memory error here
    unsigned char *data = (unsigned char*)malloc(rgb_width * 4);
    if (!data) {
        fclose(f);
        png_destroy_write_struct(&png_ptr, 0);
        return (false);
    }
    for (int y = 0; y < rgb_height; y++) {
        unsigned char *ptr = rgb_data + (y * rgb_width * 3);
        for (int x = 0; x < rgb_width; x++) {
            data[(x << 2) + 0] = *ptr++;
            data[(x << 2) + 1] = *ptr++;
            data[(x << 2) + 2] = *ptr++;
            data[(x << 2) + 3] = 255;
          }
          png_bytep row_ptr = (png_bytep)data;
          png_write_rows(png_ptr, &row_ptr, 1);
    }
    free(data);
    png_write_end(png_ptr, info_ptr);
    png_destroy_write_struct(&png_ptr, 0);

    fclose(f);
    return (true);
}

#else

bool
Image::savePNG(const char*, SaveInfo*)
{
    return (false);
}

#endif


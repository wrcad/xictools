
/*========================================================================*
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
 *========================================================================*
 * $Id: jpeg.cc,v 1.3 2008/07/13 06:17:47 stevew Exp $
 *========================================================================*/

#include "imsave.h"

#ifdef HAVE_LIBJPEG
#include <stdio.h>
extern "C" {
#include <jpeglib.h>
}


bool
Image::saveJPEG(const char *file, SaveInfo *info)
{

    FILE *f = fopen(file, "wb");
    if (!f)
        return (false);

    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);
    jpeg_stdio_dest(&cinfo, f);
    cinfo.image_width = rgb_width;
    cinfo.image_height = rgb_height;
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_RGB;
    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, (100 * info->quality) >> 8, TRUE);
    jpeg_start_compress(&cinfo, TRUE);
    int row_stride = cinfo.image_width * 3;
    while (cinfo.next_scanline < cinfo.image_height) {
        JSAMPROW row_pointer[1];
        row_pointer[0] = rgb_data + (cinfo.next_scanline * row_stride);
        jpeg_write_scanlines(&cinfo, row_pointer, 1);
    }
    jpeg_finish_compress(&cinfo);
    fclose(f);
    return (true);
}

#else

bool
Image::saveJPEG(const char*, SaveInfo*)
{
    return (false);
}

#endif


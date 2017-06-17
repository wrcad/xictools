
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
 * $Id: tiff.cc,v 1.7 2008/07/13 06:17:47 stevew Exp $
 *========================================================================*/

#include "imsave.h"

#ifdef HAVE_LIBTIFF

#ifdef _AIX
// Godawful hack to get around sys/inttypes.h clash with tiff.h.
#define  _TIFF_DATA_TYPEDEFS_
typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef int int32;
#endif

#include <tiffio.h>


bool
Image::saveTIFF(const char *file, SaveInfo*)
{
    TIFF *tif = TIFFOpen(file, "w");
    if (!tif)
        return (false);

    TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, rgb_width);
    TIFFSetField(tif, TIFFTAG_IMAGELENGTH, rgb_height);
    TIFFSetField(tif, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
    TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, 8);
    TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
//    TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_LZW);
TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_DEFLATE);
    TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, 3);
    TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
    TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP,
        TIFFDefaultStripSize(tif, (unsigned int)-1));
    for (int y = 0; y < rgb_height; y++) {
        unsigned char *data = rgb_data + (y * rgb_width * 3);
        TIFFWriteScanline(tif, data, y, 0);
    }
    TIFFClose(tif);
    return (true);
}

#else

bool
Image::saveTIFF(const char*, SaveInfo*)
{
    return (false);
}

#endif


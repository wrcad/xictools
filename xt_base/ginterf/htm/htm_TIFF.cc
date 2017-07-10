
/*=======================================================================*
 *                                                                       *
 *  XICTOOLS Integrated Circuit Design System                            *
 *  Copyright (c) 2014 Whiteley Research Inc, all rights reserved.       *
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
 * The present file is heavily based on
 * tiff2png.c - converts Tagged Image File Format to Portable Network Graphics
 *
 * Copyright 1996,2000 Willem van Schaik, Calgary (willem@schaik.com)
 * Copyright 1999-2002 Greg Roelofs (newt@pobox.com)
 *
 * [see VERSION macro below for version and date]
 *
 * Lots of material was stolen from libtiff, tifftopnm, pnmtopng, which
 * programs had also done a fair amount of "borrowing", so the credit for
 * this program goes besides the author also to:
 *         Sam Leffler
 *         Jef Poskanzer
 *         Alexander Lehmann
 *         Patrick Naughton
 *         Marcel Wijkstra
 *
 *-----------------------------------------------------------------------*
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
 * $Id: htm_TIFF.cc,v 1.2 2014/02/15 23:14:17 stevew Exp $
 *-----------------------------------------------------------------------*/

#include "htm_widget.h"
#include "htm_image.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#if defined(HAVE_LIBTIFF) && defined(HAVE_LIBPNG)
#include <tiff.h>
#include <tiffio.h>
#include <png.h>

namespace tiffns {
    char *tiff_to_png_err();
    bool tiff_to_png(const char*, FILE*);
}


// We handle TIFF by converting to PNG.
//
htmRawImageData*
htmImageManager::readTIFF(ImageBuffer *ib)
{
    const char *path = getenv("TMPDIR");
    if (!path)
        path = "/tmp";
    char buf[256];
    sprintf(buf, "%s/moz%d-%s.tmp", path, getpid(), "tiff");
    FILE *fp = fopen(buf, "wb");
    if (!fp)
        return (0);

    fwrite(ib->buffer, 1, ib->size, fp);
    fclose(fp);
    char *tiffile = strdup(buf);

    sprintf(buf, "%s/moz%d-%s.tmp", path, getpid(), "png");
    fp = fopen(buf, "wb");
    if (fp) {
        bool ret = tiffns::tiff_to_png(tiffile, fp);
        fclose (fp);
        if (ret) {
            fp = fopen(buf, "rb");
            fseek(fp, 0, SEEK_END);
            long len = ftell(fp);
            rewind(fp);
            char *sbuf = new char[len];
            long sz = fread(sbuf, 1, len, fp);
            if (sz == len) {
                delete [] ib->buffer;
                ib->buffer = (unsigned char*)sbuf;
                ib->size = sz;
                ib->type = IMAGE_PNG;
            }
            else
                delete [] sbuf;
        }
        unlink(buf);
    }
    unlink(tiffile);
    delete [] tiffile;

    if (ib->type == IMAGE_PNG)
        return (readPNG(ib));
    return (0);
}


#ifdef _AIX
#define jmpbuf __jmpbuf
#endif

#define MAXCOLORS 256

#ifndef PHOTOMETRIC_DEPTH
#define PHOTOMETRIC_DEPTH 32768
#endif

// It appears that PHOTOMETRIC_MINISWHITE should always be inverted
// (which makes sense), but if you find a class of TIFFs or a version
// of libtiff for which that is *not* the case, try not defining
// INVERT_MINISWHITE.

#define INVERT_MINISWHITE

// Macros to get and put bits out of the bytes.

#define GET_LINE_SAMPLE \
  { \
    if (bitsleft == 0) \
    { \
      p_line++; \
      bitsleft = 8; \
    } \
    bitsleft -= (bps >= 8) ? 8 : bps; \
    sample = (*p_line >> bitsleft) & maxval; \
  }

#define GET_STRIP_SAMPLE \
  { \
    if (getbitsleft == 0) \
    { \
      p_strip++; \
      getbitsleft = 8; \
    } \
    getbitsleft -= (bps >= 8) ? 8 : bps; \
    sample = (*p_strip >> getbitsleft) & maxval; \
  }

#define PUT_LINE_SAMPLE \
  { \
    if (putbitsleft == 0) \
    { \
      p_line++; \
      putbitsleft = 8; \
    } \
    putbitsleft -= (bps >= 8) ? 8 : bps; \
    *p_line |= ((sample & maxval) << putbitsleft); \
  }

namespace {
    inline bool test_bigendian()
    {
        union { int32 i; char c[4]; } endian_tester;
        endian_tester.i = 1;
        return (endian_tester.c[0]);
    }


    struct jmpbuf_wrapper
    {
        jmp_buf jmpbuf;
    };
    jmpbuf_wrapper jmpbuf_struct;

    char *err_msg;

    void err_printf(const char *fmt, ...)
    {
        va_list args;
        delete [] err_msg;
        vasprintf(&err_msg, fmt, args);
    }


    void error_handler(png_structp png_ptr, png_const_charp msg)
    {
        err_printf("fatal libpng error: %s", msg);
        jmpbuf_wrapper *jmpbuf_ptr =
            (jmpbuf_wrapper*)png_get_error_ptr(png_ptr);
        longjmp(jmpbuf_ptr->jmpbuf, 1);
    }
}


// Static function.
// Return the error message generated by a call to tiff_to_png.  The
// returned string should be deleted after use.  This clears the error
// message saved from the function call.
//
char *
tiffns::tiff_to_png_err()
{
    char *er = err_msg;
    err_msg = 0;
    return (er);
}


// Static function.
// Read a TIFF file in tiffname, and output a PNG translation to
// pngfp.  True is returned on success.  On error, false is returned,
// and an error message is available from tiff_to_png_err.
//
bool
tiffns::tiff_to_png(const char *tiffname, FILE *pngfp)
{
    bool bigendian = test_bigendian();

    TIFF *tif = TIFFOpen(tiffname, "r");
    if (!tif) {
        err_printf("Failed to open %s.", tiffname);
        return (false);
    }

    png_struct *png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,
        &jmpbuf_struct, error_handler, 0);
    if (!png_ptr) {
        TIFFClose(tif);
        return (false);
    }

    png_info *info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        png_destroy_write_struct(&png_ptr, 0);
        TIFFClose(tif);
        return (false);
    }

    if (setjmp(jmpbuf_struct.jmpbuf)) {
        png_destroy_write_struct(&png_ptr, 0);
        TIFFClose(tif);
        return (false);
    }

    png_init_io(png_ptr, pngfp);

#ifdef DEBUG
    if (verbose) {
        int byteswapped = TIFFIsByteSwapped(tif);

        fprintf(stderr, "tiff_to_png:  ");
        TIFFPrintDirectory(tif, stderr, TIFFPRINT_NONE);
        fprintf(stderr, "tiff_to_png:  byte order = %s\n",
            ((bigendian && byteswapped) || (!bigendian && !byteswapped))?
            "little-endian (Intel)" : "big-endian (Motorola)");
        fprintf(stderr, "tiff_to_png:  this machine is %s-endian\n",
            bigendian? "big" : "little");
    }
#endif

    unsigned short photometric;
    if (!TIFFGetField(tif, TIFFTAG_PHOTOMETRIC, &photometric)) {
        err_printf("TIFF photometric could not be retrieved (%s)", tiffname);
        png_destroy_write_struct(&png_ptr, &info_ptr);
        TIFFClose(tif);
        return (false);
    }
    unsigned short bps;
    if (!TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &bps))
        bps = 1;
    unsigned short spp;
    if (!TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &spp))
        spp = 1;
    unsigned short planar;
    if (!TIFFGetField(tif, TIFFTAG_PLANARCONFIG, &planar))
        planar = 1;

    unsigned short tiled = TIFFIsTiled(tif);

    int cols, rows;
    TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &cols);
    TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &rows);
    png_uint_32 width = cols;

    bool have_res = false;
    float xres, yres;
    png_uint_32 res_x=0, res_y=0;
    int unit_type = 0;
    if (TIFFGetField(tif, TIFFTAG_XRESOLUTION, &xres) &&
            TIFFGetField(tif, TIFFTAG_YRESOLUTION, &yres) && 
            (xres != 0.0) && (yres != 0.0)) {
        uint16 resunit;
        have_res = true;
#ifdef DEBUG
        if (verbose) {
            float ratio = xres / yres;
            fprintf(stderr,
                "tiff_to_png:  aspect ratio (hor/vert) = %g (%g / %g)\n",
                ratio, xres, yres);
            if (0.95 < ratio && ratio < 1.05)
                fprintf(stderr, "tiff2png:  near-unity aspect ratio\n");
            else if (1.90 < ratio && ratio < 2.10)
                fprintf(stderr, "tiff2png:  near-2X aspect ratio\n");
            else
                fprintf(stderr, "tiff2png:  non-square, non-2X pixels\n");
        }
#endif

        if (!TIFFGetField(tif, TIFFTAG_RESOLUTIONUNIT, &resunit))
            resunit = RESUNIT_INCH;

        // Convert from TIFF data (floats) to PNG data (unsigned longs).
        switch (resunit) {
        case RESUNIT_CENTIMETER:
            res_x = (png_uint_32)(100.0*xres + 0.5);
            res_y = (png_uint_32)(100.0*yres + 0.5);
            unit_type = PNG_RESOLUTION_METER;
            break;
        case RESUNIT_INCH:
            res_x = (png_uint_32)(39.37*xres + 0.5);
            res_y = (png_uint_32)(39.37*yres + 0.5);
            unit_type = PNG_RESOLUTION_METER;
            break;
        case RESUNIT_NONE:
        default:
            res_x = (png_uint_32)(100.0*xres + 0.5);
            res_y = (png_uint_32)(100.0*yres + 0.5);
            unit_type = PNG_RESOLUTION_UNKNOWN;
            break;
        }
    }

#ifdef DEBUG
    if (verbose) {
        fprintf(stderr, "tiff_to_png:  %dx%dx%d image\n", cols, rows,
            bps * spp);
        fprintf(stderr, "tiff_to_png:  %d bit%s/sample, %d sample%s/pixel\n",
            bps, bps == 1? "" : "s", spp, spp == 1? "" : "s");
    }
#endif

    // Detect tiff filetype.

    int maxval = (1 << bps) - 1;
#ifdef DEBUG
    if (verbose)
        fprintf(stderr, "tiff_to_png:  maxval=%d\n", maxval);
#endif

    int color_type = -1;
    int bit_depth = 0;
    int colors = 0;
    png_color palette[MAXCOLORS];
    unsigned short tiff_compression_method;

    switch (photometric) {
    case PHOTOMETRIC_MINISWHITE:
    case PHOTOMETRIC_MINISBLACK:
#ifdef DEBUG
        if (verbose) {
            fprintf(stderr,
                "tiff_to_png:  %d graylevels (min = %s)\n", maxval + 1,
                photometric == PHOTOMETRIC_MINISBLACK? "black" : "white");
        }
#endif
        if (spp == 1) {
            // no alpha
            color_type = PNG_COLOR_TYPE_GRAY;
#ifdef DEBUG
            if (verbose)
                fprintf(stderr, "tiff_to_png:  color type = grayscale\n");
#endif
            bit_depth = bps;
        }
        else {
            // must be alpha
            color_type = PNG_COLOR_TYPE_GRAY_ALPHA;
#ifdef DEBUG
            if (verbose)
                fprintf(stderr,
                    "tiff_to_png:  color type = grayscale + alpha\n");
#endif
            if (bps <= 8)
                bit_depth = 8;
            else
                bit_depth = bps;
        }
        break;

    case PHOTOMETRIC_PALETTE:
        {
            int palette_8bit; // Set iff all color values in TIFF palette
                              // are < 256.

            color_type = PNG_COLOR_TYPE_PALETTE;
#ifdef DEBUG
            if (verbose)
                fprintf(stderr, "tiff_to_png:  color type = paletted\n");
#endif

            unsigned short *redcolormap;
            unsigned short *greencolormap;
            unsigned short *bluecolormap;
            if (!TIFFGetField(tif, TIFFTAG_COLORMAP, &redcolormap,
                    &greencolormap, &bluecolormap)) {
                err_printf("Cannot retrieve TIFF colormaps (%s)\n", tiffname);
                png_destroy_write_struct(&png_ptr, &info_ptr);
                TIFFClose(tif);
                return (false);
            }
            colors = maxval + 1;
            if (colors > MAXCOLORS) {
                err_printf("Palette too large (%d colors) (%s)\n",
                    colors, tiffname);
                png_destroy_write_struct(&png_ptr, &info_ptr);
                TIFFClose(tif);
                return (false);
            }
            // Max PNG palette-size is 8 bits, you could convert to
            // full-color.
            if (bps >= 8) 
                bit_depth = 8;
            else
                bit_depth = bps;

            // PLTE chunk
            // TIFF palettes contain 16-bit shorts, while PNG palettes
            // are 8-bit.  Some broken (??) software puts 8-bit values
            // in the shorts, which would make the palette come out
            // all zeros, which isn't good.  We check...

            palette_8bit = 1;
            for (int i = 0 ; i < colors ; i++) {
                if (redcolormap[i] > 255 || greencolormap[i] > 255 ||
                         bluecolormap[i] > 255) {
                     palette_8bit = 0;
                     break;
                }
            } 
#ifdef DEBUG
            if (palette_8bit && verbose)
                fprintf(stderr,
                    "tiff_to_png warning:  assuming 8-bit palette values.\n");
#endif

            for (int i = 0 ; i < colors ; i++) {
                if (palette_8bit) {
                    palette[i].red   = (png_byte)redcolormap[i];
                    palette[i].green = (png_byte)greencolormap[i];
                    palette[i].blue  = (png_byte)bluecolormap[i];
                }
                else {
                    palette[i].red   = (png_byte)(redcolormap[i] >> 8);
                    palette[i].green = (png_byte)(greencolormap[i] >> 8);
                    palette[i].blue  = (png_byte)(bluecolormap[i] >> 8);
                }
            }
            break;
        }

    case PHOTOMETRIC_YCBCR:
        TIFFGetField(tif, TIFFTAG_COMPRESSION, &tiff_compression_method);
        if (tiff_compression_method == COMPRESSION_JPEG &&
                planar == PLANARCONFIG_CONTIG) {
            // Can rely on libjpeg to convert to RGB.
            TIFFSetField(tif, TIFFTAG_JPEGCOLORMODE, JPEGCOLORMODE_RGB);
            photometric = PHOTOMETRIC_RGB;
#ifdef DEBUG
            if (verbose)
                fprintf(stderr, "tiff2png:  original color type = YCbCr "
                    "with JPEG compression.\n");
#endif
        }
        else {
            err_printf(
                "Don't know how to handle PHOTOMETRIC_YCBCR with "
                "compression %d\n"
                "  (%sJPEG) and planar config %d (%scontiguous)\n"
                "  (%s)\n", tiff_compression_method,
                tiff_compression_method == COMPRESSION_JPEG? "" : "not ",
                planar, planar == PLANARCONFIG_CONTIG? "" : "not ", tiffname);
            png_destroy_write_struct(&png_ptr, &info_ptr);
            TIFFClose(tif);
            return (false);
        }
        // fall thru... 

    case PHOTOMETRIC_RGB:
        if (spp == 3) {
            color_type = PNG_COLOR_TYPE_RGB;
#ifdef DEBUG
            if (verbose)
                fprintf(stderr, "tiff_to_png:  color type = truecolor\n");
#endif
        }
        else {
            color_type = PNG_COLOR_TYPE_RGB_ALPHA;
#ifdef DEBUG
            if (verbose)
                fprintf(stderr,
                    "tiff_to_png:  color type = truecolor + alpha\n");
#endif
        }
        if (bps <= 8)
            bit_depth = 8;
        else
            bit_depth = bps;
        break;

    case PHOTOMETRIC_LOGL:
    case PHOTOMETRIC_LOGLUV:
        TIFFGetField(tif, TIFFTAG_COMPRESSION, &tiff_compression_method);
        if (tiff_compression_method != COMPRESSION_SGILOG &&
                tiff_compression_method != COMPRESSION_SGILOG24) {
            err_printf("Don't know how to handle PHOTOMETRIC_LOGL%s with\n"
                "  compression %d (not SGILOG) (%s)\n",
                photometric == PHOTOMETRIC_LOGLUV? "UV" : "",
                tiff_compression_method, tiffname);
            png_destroy_write_struct(&png_ptr, &info_ptr);
            TIFFClose(tif);
            return (false);
        }
        // rely on library to convert to RGB/greyscale.
#ifdef LIBTIFF_HAS_16BIT_INTEGER_FORMAT
        if (bps > 8) {
            // SGILOGDATAFMT_16BIT converts to a floating-point
            // luminance value; U,V are left as such. 
            // SGILOGDATAFMT_16BIT_INT doesn't exist.

            TIFFSetField(tif, TIFFTAG_SGILOGDATAFMT, SGILOGDATAFMT_16BIT_INT);
            bit_depth = bps = 16;
        }
        else
#endif
        {
            // SGILOGDATAFMT_8BIT converts to normal grayscale or RGB format.
            TIFFSetField(tif, TIFFTAG_SGILOGDATAFMT, SGILOGDATAFMT_8BIT);
            bit_depth = bps = 8;
        }
        if (photometric == PHOTOMETRIC_LOGL) {
            photometric = PHOTOMETRIC_MINISBLACK;
            color_type = PNG_COLOR_TYPE_GRAY;
#ifdef DEBUG
            if (verbose) {
                fprintf(stderr, "tiff_to_png:  original color type = "
                    "logL with SGILOG compression.\n");
                fprintf(stderr, "tiff_to_png:  color type = grayscale.\n");
            }
#endif
        }
        else {
            photometric = PHOTOMETRIC_RGB;
            color_type = PNG_COLOR_TYPE_RGB;
#ifdef DEBUG
            if (verbose) {
                fprintf(stderr, "tiff_to_png:  original color type = "
                    "logLUV with SGILOG compression.\n");
                fprintf(stderr, "tiff_to_png:  color type = truecolor.\n");
            }
#endif
        }
        break;

    case PHOTOMETRIC_MASK:
    case PHOTOMETRIC_SEPARATED:
    case PHOTOMETRIC_CIELAB:
    case PHOTOMETRIC_DEPTH:
        err_printf("Don't know how to handle %s (%s)\n",
            photometric == PHOTOMETRIC_MASK?      "PHOTOMETRIC_MASK" :
            photometric == PHOTOMETRIC_SEPARATED? "PHOTOMETRIC_SEPARATED" :
            photometric == PHOTOMETRIC_CIELAB?    "PHOTOMETRIC_CIELAB" :
            photometric == PHOTOMETRIC_DEPTH?     "PHOTOMETRIC_DEPTH" :
                                                  "unknown photometric",
            tiffname);
        png_destroy_write_struct(&png_ptr, &info_ptr);
        TIFFClose(tif);
        return (false);

    default:
        err_printf("Unknown photometric (%d) (%s)\n",
            photometric, tiffname);
        png_destroy_write_struct(&png_ptr, &info_ptr);
        TIFFClose(tif);
        return (false);
    }
    int tiff_color_type = color_type;

#ifdef DEBUG
    if (verbose)
        fprintf(stderr, "tiff_to_png:  bit depth = %d\n", bit_depth);
#endif

    // Put parameter info in png-chunks.

    png_set_IHDR(png_ptr, info_ptr, width, rows, bit_depth, color_type,
        PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
        PNG_FILTER_TYPE_DEFAULT);

//    if (png_compression_level != -1)
//        png_set_compression_level(png_ptr, png_compression_level);

    if (color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_PLTE(png_ptr, info_ptr, palette, colors);

    // gAMA chunk
//    if (gamma != -1.0) {
#ifdef DEBUG
//        if (verbose)
//            fprintf(stderr, "tiff_to_png:  gamma = %f\n", gamma);
#endif
//        png_set_gAMA(png_ptr, info_ptr, gamma);
//    }

    // pHYs chunk
    if (have_res)
        png_set_pHYs(png_ptr, info_ptr, res_x, res_y, unit_type);

    png_write_info(png_ptr, info_ptr);
    png_set_packing(png_ptr);

    // Allocate space for one line (or row of tiles) of TIFF image.

    unsigned char *tiffline = 0;
    unsigned char *tifftile = 0;
    unsigned char *tiffstrip = 0;
    size_t tilesz = 0L;
    int num_tilesX = 0;
    uint32 tile_width = 0, tile_height = 0;

    if (!tiled) {
        // strip-based TIFF
        if (planar == 1) {
            // contiguous picture
            tiffline = (unsigned char*)malloc(TIFFScanlineSize(tif));
        }
        else {
            // separated planes
            tiffline = (unsigned char*)malloc(TIFFScanlineSize(tif) * spp);
        }
    }
    else {
        // Allocate space for one "row" of tiles.

        TIFFGetField(tif, TIFFTAG_TILEWIDTH, &tile_width);
        TIFFGetField(tif, TIFFTAG_TILELENGTH, &tile_height);

        num_tilesX = (width+tile_width-1)/tile_width;

        if (planar == 1) {
            tilesz = TIFFTileSize(tif);
            tifftile = new unsigned char[tilesz];
            size_t stripsz = (tile_width*num_tilesX) * tile_height * spp;
            tiffstrip = new unsigned char[stripsz];
            tiffline = tiffstrip;
            // Just set the line to the top of the strip, we'll move it
            // through below.
        }
        else {
            err_printf(
                "Can't handle tiled separated-plane TIFF format (%s).\n",
                tiffname);
            png_destroy_write_struct (&png_ptr, &info_ptr);
            TIFFClose(tif);
            return (false);
        }
    }

    if (!tiffline) {
        err_printf("Can't allocate memory for TIFF scanline buffer (%s).\n",
            tiffname);
        png_destroy_write_struct(&png_ptr, &info_ptr);
        TIFFClose(tif);
        if (tiled && planar == 1)
            delete [] tifftile;
        return (false);
    }

    if (planar != 1) {
        // In case we must combine more planes into one.
        tiffstrip = new unsigned char[TIFFScanlineSize(tif)];
        if (!tiffstrip) {
            err_printf("Can't allocate memory for TIFF strip buffer (%s).\n",
                tiffname);
            png_destroy_write_struct(&png_ptr, &info_ptr);
            TIFFClose(tif);
            free(tiffline);
            return (false);
        }
    }

    // Allocate space for one line of PNG image.
    // max: 3 color channels plus one alpha channel, 16 bit => 8 bytes/pixel

    png_byte *pngline = new unsigned char[cols * 8];
    for (int pass = 0 ; pass < png_set_interlace_handling(png_ptr); pass++) {
        for (int row = 0; row < rows; row++) {
            if (planar == 1) {
                // contiguous picture
                if (!tiled) {
                    if (TIFFReadScanline(tif, tiffline, row, 0) < 0) {
                        err_printf("Bad data read on line %d (%s).\n",
                            row, tiffname);
                        png_destroy_write_struct(&png_ptr, &info_ptr);
                        TIFFClose(tif);
                        free(tiffline);
                        return (false);
                    }
                }
                else {
                    // tiled
                    int col, ok=1, r;
                    int tileno;
                    // Read in one row of tiles and hand out the data
                    // one scanline at a time so the code below
                    // doesn't need to change.

                    // Is it time for a new strip?
                    if ((row % tile_height) == 0) {
                        for (col = 0; ok && col < num_tilesX; col += 1 ) {
                            tileno = col+(row/tile_height)*num_tilesX;
                            // Read the tile into an RGB array.
                            if (!TIFFReadEncodedTile(tif, tileno, tifftile,
                                    tilesz)) {
                                ok = 0;
                                break;
                            }

                            // Copy this tile into the row buffer.
                            for (r = 0; r < (int) tile_height; r++) {
                                void *dest = tiffstrip + (r * tile_width *
                                    num_tilesX * spp) +
                                    (col * tile_width * spp);
                                void *src  = tifftile + (r * tile_width * spp);
                                memcpy(dest, src, (tile_width * spp));
                            }
                        }
                        tiffline = tiffstrip; // Set tileline to top of strip.
                    }
                    else {
                        tiffline = tiffstrip + ((row % tile_height) *
                            ((tile_width * num_tilesX) * spp));
                    }
                }
            }
            else {
                // Separated planes, then combine more strips into one line.
                unsigned short s;

                // XXX:  this assumes strips; are separated-plane
                // tiles possible?

                unsigned char *p_line = tiffline;
                for (int n = 0; n < (cols/8 * bps*spp); n++)
                    *p_line++ = '\0';

                for (s = 0; s < spp; s++) {
                    unsigned char *p_strip = tiffstrip;
                    int getbitsleft = 8;
                    p_line = tiffline;
                    int putbitsleft = 8;

                    if (TIFFReadScanline(tif, tiffstrip, row, s) < 0) {
                        err_printf("Bad data read on line %d (%s).\n",
                            row, tiffname);
                        png_destroy_write_struct(&png_ptr, &info_ptr);
                        TIFFClose(tif);
                        delete [] tiffline;
                        delete [] tiffstrip;
                        return (false);
                    }

                    p_strip = (unsigned char *)tiffstrip;
                    unsigned char sample = '\0';
                    for (int i = 0 ; i < s ; i++)
                        PUT_LINE_SAMPLE
                    for (int n = 0; n < cols; n++) {
                        GET_STRIP_SAMPLE
                        PUT_LINE_SAMPLE
                        sample = '\0';
                        for (int i = 0 ; i < (spp-1) ; i++)
                            PUT_LINE_SAMPLE
                    }
                }
            }

            unsigned char *p_line = tiffline;
            int bitsleft = 8;
            png_byte *p_png = pngline;

            // Convert from tiff-line to png-line.

            switch (tiff_color_type) {
            case PNG_COLOR_TYPE_GRAY:       // we know spp == 1
                for (int col = cols; col > 0; --col) {
                    switch (bps) {
                    case 16:
#ifdef INVERT_MINISWHITE
                        if (photometric == PHOTOMETRIC_MINISWHITE) {
                            unsigned char sample;
                            int sample16;
                            if (bigendian) {
                                // same as PNG order
                                GET_LINE_SAMPLE
                                sample16 = sample;
                                sample16 <<= 8;
                                GET_LINE_SAMPLE
                                sample16 |= sample;
                            }
                            else {
                                // reverse of PNG
                                GET_LINE_SAMPLE
                                sample16 = sample;
                                GET_LINE_SAMPLE
                                sample16 |= (((int)sample) << 8);
                            }
                            sample16 = maxval - sample16;
                            *p_png++ = (unsigned char)((sample16 >> 8) & 0xff);
                            *p_png++ = (unsigned char)(sample16 & 0xff);
                        }
                        else // not PHOTOMETRIC_MINISWHITE
#endif
                        {
                            unsigned char sample;
                            if (bigendian) {
                                GET_LINE_SAMPLE
                                *p_png++ = sample;
                                GET_LINE_SAMPLE
                                *p_png++ = sample;
                            }
                            else {
                                GET_LINE_SAMPLE
                                p_png[1] = sample;
                                GET_LINE_SAMPLE
                                *p_png = sample;
                                p_png += 2;
                            }
                        }
                        break;

                    case 8:
                    case 4:
                    case 2:
                    case 1:
                        {
                            unsigned char sample;
                            GET_LINE_SAMPLE
#ifdef INVERT_MINISWHITE
                            if (photometric == PHOTOMETRIC_MINISWHITE)
                                sample = maxval - sample;
#endif
                            *p_png++ = sample;
                        }
                        break;

                    }
                }
                break;

            case PNG_COLOR_TYPE_GRAY_ALPHA:
                for (int col = 0; col < cols; col++) {
                    for (int i = 0 ; i < spp ; i++) {
                        switch (bps) {
                        case 16:
#ifdef INVERT_MINISWHITE
                            if (photometric == PHOTOMETRIC_MINISWHITE &&
                                    i == 0) {
                                unsigned char sample;
                                int sample16;
                                if (bigendian) {
                                    GET_LINE_SAMPLE
                                    sample16 = (sample << 8);
                                    GET_LINE_SAMPLE
                                    sample16 |= sample;
                                }
                                else {
                                    GET_LINE_SAMPLE
                                    sample16 = sample;
                                    GET_LINE_SAMPLE
                                    sample16 |= (((int)sample) << 8);
                                }
                                sample16 = maxval - sample16;
                                *p_png++ = (unsigned char)(
                                    (sample16 >> 8) & 0xff);
                                *p_png++ = (unsigned char)(sample16 & 0xff);
                            }
                            else
#endif
                            {
                                unsigned char sample;
                                if (bigendian) {
                                    GET_LINE_SAMPLE
                                    *p_png++ = sample;
                                    GET_LINE_SAMPLE
                                    *p_png++ = sample;
                                }
                                else {
                                    GET_LINE_SAMPLE
                                    p_png[1] = sample;
                                    GET_LINE_SAMPLE
                                    *p_png = sample;
                                    p_png += 2;
                                }
                            }
                            break;

                        case 8:
                            {
                                unsigned char sample;
                                GET_LINE_SAMPLE
#ifdef INVERT_MINISWHITE
                                if (photometric == PHOTOMETRIC_MINISWHITE &&
                                        i == 0)
                                    sample = maxval - sample;
#endif
                                *p_png++ = sample;
                            }
                            break;

                        case 4:
                            {
                                unsigned char sample;
                                GET_LINE_SAMPLE
#ifdef INVERT_MINISWHITE
                                if (photometric == PHOTOMETRIC_MINISWHITE &&
                                        i == 0)
                                    sample = maxval - sample;
#endif
                                *p_png++ = sample * 17;   /* was 16 */
                            }
                            break;

                        case 2:
                            {
                                unsigned char sample;
                                GET_LINE_SAMPLE
#ifdef INVERT_MINISWHITE
                                if (photometric == PHOTOMETRIC_MINISWHITE &&
                                        i == 0)
                                    sample = maxval - sample;
#endif
                                *p_png++ = sample * 85;   /* was 64 */
                            }
                            break;

                        case 1:
                            {
                                unsigned char sample;
                                GET_LINE_SAMPLE
#ifdef INVERT_MINISWHITE
                                if (photometric == PHOTOMETRIC_MINISWHITE &&
                                        i == 0)
                                    sample = maxval - sample;
#endif
                                *p_png++ = sample * 255;  /* was 128...oops */
                            }
                            break;

                        }
                    }
                }
                break;

            case PNG_COLOR_TYPE_RGB:
            case PNG_COLOR_TYPE_RGB_ALPHA:
                for (int col = 0; col < cols; col++) {
                    // Process for red, green and blue (and when
                    // applicable alpha).

                    for (int i = 0 ; i < spp ; i++) {
                        switch (bps) {
                        case 16:
                            // XXX:  do we need INVERT_MINISWHITE
                            // support here, too, or is that only for
                            // grayscale?

                            unsigned char sample;
                            if (bigendian) {
                                GET_LINE_SAMPLE
                                *p_png++ = sample;
                                GET_LINE_SAMPLE
                                *p_png++ = sample;
                            }
                            else {
                                GET_LINE_SAMPLE
                                p_png[1] = sample;
                                GET_LINE_SAMPLE
                                *p_png = sample;
                                p_png += 2;
                            }
                            break;

                        case 8:
                            GET_LINE_SAMPLE
                            *p_png++ = sample;
                            break;

                        case 4:
                            GET_LINE_SAMPLE
                            *p_png++ = sample * 17;
                            break;

                        case 2:
                            GET_LINE_SAMPLE
                            *p_png++ = sample * 85;
                            break;

                        case 1:
                            GET_LINE_SAMPLE
                            *p_png++ = sample * 255;
                           break;

                        }
                    }
                }
                break;
  
            case PNG_COLOR_TYPE_PALETTE:
                for (int col = 0; col < cols; col++) {
                    unsigned char sample;
                    GET_LINE_SAMPLE
                    *p_png++ = sample;
                }
                break;
  
            default:
                err_printf("Unknown photometric (%d) (%s).\n",
                    photometric, tiffname);
                png_destroy_write_struct(&png_ptr, &info_ptr);
                TIFFClose(tif);
                delete [] tiffline;
                if (tiled && planar == 1)
                    delete [] tifftile;
                else if (planar != 1)
                    delete [] tiffstrip;
                return (false);

            }
            png_write_row(png_ptr, pngline);
        }
    }

    TIFFClose(tif);

    png_write_end(png_ptr, info_ptr);

    png_destroy_write_struct(&png_ptr, &info_ptr);

    free(tiffline);
    if (tiled && planar == 1)
        delete [] tifftile;
    else if (planar != 1)
        delete [] tiffstrip;
    return (true);
}


#else

htmRawImageData*
htmImageManager::readTIFF(ImageBuffer*)
{
    return (0);
}

#endif


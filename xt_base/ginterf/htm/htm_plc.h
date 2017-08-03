
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
 *   Stephen R. Whiteley  <srw@wrcad.com>
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

#ifndef HTM_PLC_H
#define HTM_PLC_H

#ifndef HTM_WIDGET_H
#include "config.h"
#endif
#include <stdio.h>
#ifdef HAVE_LIBPNG
#include <png.h>
#else
#ifdef HAVE_LIBZ
#include <zlib.h>
#endif
#endif

// png.h includes setjmp.h and issues a cpp error on Linux when it gets
// included more than once...

#ifdef HAVE_LIBJPEG
extern "C" {
#include <jpeglib.h>
}
#ifndef HAVE_LIBPNG
#include <setjmp.h>
#endif
#endif

// GIF decoder
#include "htm_image.h"
#include "htm_lzw.h"

// Maximum size of the PLC get_data() buffer.  This is the maximum
// amount of data that will be requested to a function installed on
// the progressiveReadProc.  Although this define can have any value,
// using a very small value will make progressive loading very slow,
// while using a large value will make the response of gtkhtm slow
// while any PLC's are active.  The first call to the get_data()
// routine will request PLC_MAX_BUFFER_SIZE bytes, while the size
// requested by any following calls will depend on the type of image
// being loaded and the amount of data left in the current input
// buffer.
//
#define PLC_MAX_BUFFER_SIZE                 2048*8

// Definition of the Progressive Loader Context.  This structure forms
// the basis of the widget's progressive object loading mechanism.
// All PLC's in use by the widget are represented by a ringbuffer with
// various function pointers.  The PLC monitoring routine will
// circulate this buffer using an adjustable interval, calling
// functions as they are necessary.

namespace htm
{
    struct PLCImage;

    // PLC status flags
    enum PLCStatus
    {
        PLC_ACTIVE,                 // PLC is active
        PLC_SUSPEND,                // PLC has been suspended
        PLC_ABORT,                  // PLC has been aborted
        PLC_COMPLETE                // PLC is done
    };

    struct PLC
    {
        PLC(htmImageManager*, const char*);
        ~PLC();

        void new_image_object();
        void transfer();
        void finalize();

        // rewind the current input buffer
        void RewindInputBuffer()
            {
                plc_left = plc_size;
                plc_next_in = plc_buffer;
            }

        bool DataRequest();
        size_t ReadOK(unsigned char*, int);
        size_t GetDataBlock(unsigned char*);
        void EndData();
        void Insert();
        void Remove();
        void Run();

        char            *plc_url;           // object identifier
        PLCImage        *plc_object;        // object-specific data
        htmImageManager *plc_owner;         // owning image manager
        unsigned char   *plc_buffer;        // current data
        unsigned int    plc_buf_size;       // size of buffer
        unsigned int    plc_size;           // size of valid data in buffer
        unsigned int    plc_left;           // bytes left in buffer
        unsigned char   *plc_next_in;       // current position in buffer

        unsigned char   *plc_input_buffer;  // input buffer
        int             plc_input_size;     // size of input buffer
        unsigned int    plc_total_in;       // total number of bytes received
        unsigned int    plc_max_in;         // get_data() maximum request size
        unsigned int    plc_min_in;         // get_data() minimum request size

        PLCStatus       plc_status;         // current PLC status
        int             plc_data_status;    // last return value from get_data
        bool            plc_initialized;    // indicates object data set
                                            // and actual processing can begin

        htmImageInfo    *plc_priv_data;     // private PLC data
        void            *plc_user_data;     // data registered for this PLC

        bool            plc_obj_funcs_complete; // obj_func calling flag

        PLC             *plc_next;          // ptr to next PLC
        PLC             *plc_prev;          // ptr to previous PLC
    };

    // Explanation of the PLCProc fields.
    //
    // init():
    //   This function is called when the object-specific data should be
    //   initialized.  When the object is initialized, the initialized field
    //   should be set to true.  The PLC cycler will call this function as
    //   long as the initialized field is false, and the plc_status field is
    //   either PLC_ACTIVE or PLC_SUSPEND.
    //
    // destructor():
    //   This function is called if the object should destroy its own data.
    //   It is called when the plc_status field reaches either PLC_COMPLETE
    //   or PLC_ABORT.
    //
    // transfer():
    //   This function is called whenever an object-specific function
    //   returns.  The purpose of this function is to signal the application
    //   that it can transfer the processed data to its final destination
    //   (for images, this should include transfering the newly decoded
    //   scanlines to the screen buffer).  It is called whenever the PLC
    //   cycler returns from any function in the obj_funcs array.
    //
    // finalize():
    //   This function is called when the plc_status field reaches
    //   PLC_COMPLETE (get_data() returned STREAM_END or processing has
    //   finished).  The application should then save all decoded data.
    //   The object may not destruct itself, the PLC cycler will call the
    //   object-specific destructor method when it has called the finalize()
    //   function.
    //
    // The PLCProc array contains object-specific functions.  For images,
    // only the first slot is used:  it is the scanline function.
    // curr_obj_func gives the index of the obj_func to call.
    // obj_funcs_complete indicates whether or not the PLC cycler should
    // continue calling any obj_func.  If it is set to true, the cycler
    // will call the finalize() PLCProc to allow final processing of the
    // received data.

    // The fun part: Object definitions.
    // Each object for which a PLC is to be used has a unique
    // definition.

    // Possible values for the o_type field.
    //
    enum PLCType
    {
        plcAnyImage = 1,    // common image object
        plcGIF,             // gif image
        plcGZF,             // gzf image
        plcPNG,             // png image
        plcJPEG,            // jpeg image
        plcXPM,             // xpm image
        plcXBM              // xbm image
    };

    // Common image object fields.
    // The image object fields are divided in two main sections:
    //
    // public fields:
    //   these fields must be set/updated by the decoder and can be used
    //   by the decoder.  The image transfer function uses the values of
    //   these fields to compose the image itself, and can modify the
    //   values of data_pos and prev_pos for backtracking purposes.
    //
    // private fields:
    //   these fields are used by the image transfer function, and may
    //   never be touched by the decoder.

    // Common image object.  This structure contains the fields that are
    // common to all image objects.

    struct PLCImage
    {
        PLCImage(PLC*, htmImageInfo*);
        virtual ~PLCImage() { }

        virtual void Init() = 0;
        virtual void ScanlineProc() = 0;
        virtual void FinalPass() = 0;

        void finalize(htmImageInfo*);

        PLCType o_type;                 // type of object
        PLC *o_plc;                     // owning PLC
        unsigned char *o_buffer;        // destination buffer
        unsigned int o_buf_size;        // size of destination buffer
        unsigned int o_byte_count;      // number of bytes received so far
        unsigned int o_buf_pos;         // current position in buffer

        // public fields
        int o_depth;                    // depth of image
        unsigned char o_colorclass;     // colorclass of image
        unsigned char o_transparency;   // transparency type of image
        htmColor *o_cmap;               // colormap for this image
        int o_cmapsize;                 // size of colormap
        int o_ncolors;                  // original no of colors in image
        unsigned int o_width;           // width in pixels
        unsigned int o_height;          // height in pixels (= no of scanlines)
        unsigned int o_npasses;         // no of passes required on image data
        unsigned int o_curr_pass;       // current pass on data
        unsigned int o_curr_scanline;   // current scanline
        unsigned int o_stride;          // scanline stride
        unsigned char *o_data;          // raw image data
        int o_data_size;                // maximum data size
        int o_data_pos;                 // current position in data
        int o_prev_pos;                 // last known position in data

        // private fields
        int o_used[MAX_IMAGE_COLORS];   // array of used colors
        int o_nused;                    // colors already used
        unsigned int o_xcolors[MAX_IMAGE_COLORS]; // array of allocated pixels
        unsigned int o_bg_pixel;        // transparent pixel index
        htmColor *o_bg_cmap;            // background colormap for this image
        int o_bg_cmapsize;              // background colormap size
        htmPixmap *o_pixmap;            // destination pixmap
        htmPixmap *o_clipmask;          // destination clipmask
        unsigned char *o_clip_data;     // raw clipmask data
        unsigned char *o_scaled_data;   // scaled image data
        int o_sc_start;                 // curr. pos in scaled data
        int o_sc_end;                   // end pos in scaled data
        bool o_is_scaled;               // True when scaling required
        htmXImage *o_ximage;            // destination image
        htmImageInfo *o_info;           // raw image information
        htmImage *o_image;              // destination image
    };

    // GIF image object
    struct PLCImageGIF : public PLCImage
    {
        PLCImageGIF(PLC *plc, htmImageInfo *info) : PLCImage(plc, info)
            {
                o_type = plcGIF;

                o_gstream = 0;
                o_lstream = 0;
            }
        ~PLCImageGIF();

        void Init();
        void ScanlineProc();
        void FinalPass() { }

        bool DoExtension(int);
        bool ReadColormap();
        bool DoImage(unsigned char*);

        unsigned char o_gbuf[256];      // block of compressed raster data

        htmGIFStream *o_gstream;        // GIFStream() stream object

        ImageBuffer o_ib;               // lzwStream data provider
        lzwStream *o_lstream;           // lzwStream() stream object
    };

    // GZF image object
    struct PLCImageGZF : public PLCImageGIF
    {
        PLCImageGZF(PLC *plc, htmImageInfo *info) : PLCImageGIF(plc, info)
            { o_type = plcGZF; }
        ~PLCImageGZF();

        void Init();
        void ScanlineProc();

#if defined(HAVE_LIBPNG) || defined(HAVE_LIBZ)
        unsigned char o_zbuf[256];      // block of compressed raster data
        z_stream o_zstream;             // zlib inflate() stream object
#endif
    };

    // JPEG image object
#ifdef HAVE_LIBJPEG
    // default libjpeg error override
    struct plc_jpeg_err_mgr
    {
        jpeg_error_mgr pub;             // jpeg public fields
        jmp_buf setjmp_buffer;          // for return to caller
    };
#endif

    struct PLCImageJPEG : public PLCImage
    {
        PLCImageJPEG(PLC *plc, htmImageInfo *info) : PLCImage(plc, info)
        {
            o_type = plcJPEG;

#ifdef HAVE_LIBJPEG
            o_init = false;
            o_do_final = false;
#endif
        }
        ~PLCImageJPEG();

        void Init();
        void ScanlineProc();
        void FinalPass();

#ifdef HAVE_LIBJPEG
        void ReadJPEGColormap(struct jpeg_decompress_struct*);

        bool                o_init;     // jpeg initialization complete?
        bool                o_do_final; // do final pass
        jpeg_decompress_struct o_cinfo; // jpeg decompressor
        plc_jpeg_err_mgr    o_jerr;     // error manager object
#endif
    };

    // PNG image object
    struct PLCImagePNG : public PLCImage
    {
        PLCImagePNG(PLC *plc, htmImageInfo *info) : PLCImage(plc, info)
            { o_type = plcPNG; }

        void Init() { o_plc->plc_status = PLC_ABORT; }
        void ScanlineProc() { o_plc->plc_status = PLC_ABORT; }
        void FinalPass() { o_plc->plc_status = PLC_ABORT; }
    };

    // XPM image object
    struct PLCImageXPM : public PLCImage
    {
        PLCImageXPM(PLC *plc, htmImageInfo *info) : PLCImage(plc, info)
            { o_type = plcXPM; }

        void Init() { o_plc->plc_status = PLC_ABORT; }
        void ScanlineProc() { o_plc->plc_status = PLC_ABORT; }
        void FinalPass() { o_plc->plc_status = PLC_ABORT; }
    };

    // XBM image object
    struct PLCImageXBM : public PLCImage
    {
        PLCImageXBM(PLC *plc, htmImageInfo *info) : PLCImage(plc, info)
            { o_type = plcXBM; }

        void Init();
        void ScanlineProc();
        void FinalPass() { }

        int bgets(char*, int);

        int o_raster_length;
        int o_data_start;
    };
}

#endif



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
#include "htm_plc.h"

#ifdef HAVE_LIBJPEG

namespace {
    // our own memory manager
    struct plc_jpeg_source_mgr
    {
        jpeg_source_mgr pub;        // public fields
        PLC     *plc;               // ptr to current PLC
        unsigned char *buffer;      // jpeg input buffer
        int     buf_size;           // size of jpeg input buffer
        int     nskip;              // bytes to be skipped in the input buf.
    };

    void ErrorExit(j_common_ptr);
    void InitSource(j_decompress_ptr);
    void TermSource(j_decompress_ptr);
    void SetSource(j_decompress_ptr, PLC*);
    void SkipInputData(j_decompress_ptr, long);
    boolean FillInputBuffer(j_decompress_ptr);
}


// JPEG PLC virtual destructor method.
//
PLCImageJPEG::~PLCImageJPEG()
{
    jpeg_decompress_struct *cinfo = &o_cinfo;
    plc_jpeg_source_mgr *src = (plc_jpeg_source_mgr*)cinfo->src;

    // wipe out source manager buffers if not already done
    if (src->buf_size)
        delete [] src->buffer;
    src->buffer   = 0;
    src->buf_size = 0;

    // and destroy the decompressor
    jpeg_destroy_decompress(cinfo);

    o_plc->plc_object = 0;
    o_plc->plc_status = PLC_COMPLETE;
}


// JPEG PLC initialization routine.  Allocates data structures, reads
// initial JPEG info.
// (public virtual)
//
void
PLCImageJPEG::Init()
{
    // this plc is active
    o_plc->plc_status = PLC_ACTIVE;
    o_plc->plc_max_in = PLC_MAX_BUFFER_SIZE;

    jpeg_decompress_struct *cinfo = &o_cinfo;
    plc_jpeg_err_mgr *jerr = &o_jerr;

    // If the jpeg decompressor hasn't been allocated yet, do it now.
    // This test may fail if we have been suspended while reading the
    // JPEG header.  Note that we only rewind the input buffer only
    // once as the jpeg decompressor keeps its own internal state when
    // I/O suspension occurs in any of libjpeg's internal routines.
    // Rewinding the input buffer every time would disrupt the
    // decompressor.

    if (o_init == false) {
        // we might have already processed some incoming data, reset
        o_plc->RewindInputBuffer();

        cinfo->err = (struct jpeg_error_mgr *)jpeg_std_error(&(jerr->pub));
        jerr->pub.error_exit = ErrorExit;

        jpeg_create_decompress(cinfo);

        SetSource(cinfo, o_plc);
        o_init = true;
    }

    // Exit override must be done for each invocation of this routine,
    // set_jmp saves the current environment, which differs each time
    // this routine is called.

    if (setjmp(jerr->setjmp_buffer)) {

        // JPEG signaled an error, abort this PLC.  Necessary cleanup
        // will be done by Destructor.

        // abort this PLC
        o_plc->plc_status = PLC_ABORT;

        return;
    }

    // Read the header.  We only want to have images (e.i.  the true).
    // libjpeg will backtrack to a suitable place in the input stream
    // when I/O suspension occurs in this call.

    int i;
    if ((i = jpeg_read_header(cinfo, true)) != JPEG_HEADER_OK) {
        // an error of some kind, abort this PLC
        if (i != JPEG_SUSPENDED)
            o_plc->plc_status = PLC_ABORT;

        // no more data, aborted or suspended
        return;
    }
    // we know this is a JPEG image
    o_info->type = IMAGE_JPEG;

    // jpeg images are always fully opaque
    o_transparency = IMAGE_NONE;
    o_bg_pixel = (unsigned int)-1;

    // Set jpeg options
    // We want to have buffered output if we want to be able to display
    // the data read so far.

    cinfo->buffered_image = true;

    // We only want colormapped output, and we only do single-pass
    // color quantization for all but the last passes (two pass
    // quantization requires the entire image data to be present,
    // making progressive loading of JPEG images useless).  A final
    // color optimization pass can be performed when the
    // progressivePerfectColor resource has been set to true, so if we
    // want to be able to allow this we must make preparations for
    // this (hence the enable_ settings).

    cinfo->quantize_colors    = true;
    cinfo->enable_1pass_quant = true;
    cinfo->enable_2pass_quant = true;
    cinfo->two_pass_quantize  = false;
    cinfo->dither_mode        = JDITHER_ORDERED;
    cinfo->colormap           = 0;
    cinfo->output_gamma       = o_plc->plc_owner->im_html->htm_screen_gamma;
    cinfo->desired_number_of_colors =
        o_plc->plc_owner->im_html->htm_max_image_colors;

    // Now initialize the decompressor using the options set above.
    // This routine will never suspend in buffered image mode.

    jpeg_start_decompress(cinfo);

    // check colorspace. We only support RGB and GRAYSCALE
    if (cinfo->out_color_space != JCS_RGB &&
            cinfo->out_color_space != JCS_GRAYSCALE) {
        J_COLOR_SPACE j_cs = cinfo->out_color_space;

        o_plc->plc_owner->im_html->warning("JPEG Init",
            "Unsupported JPEG colorspace %s on image\n    %s. Skipping.",
            (j_cs == JCS_UNKNOWN ? "unspecified" :
            (j_cs == JCS_YCbCr ? "YCbCr (also known as YUV)" :
            (j_cs == JCS_CMYK ? "CMYK" : "YCCK"))), o_plc->plc_url);

        // to bad, abort it
        o_plc->plc_status = PLC_ABORT;
        return;
    }

    // Image colorclass and colormap.  As the RGB image data will be
    // colormapped (pixelized that is), we classify this image as
    // being indexed.

    o_ncolors  = cinfo->desired_number_of_colors;

    // image dimensions
    o_width  = cinfo->output_width;
    o_height = cinfo->output_height;

    // Decoded data buffer and counters.
    // input buffer data stride; output_comp should always be 1
    o_stride = cinfo->output_width * cinfo->output_components;

    o_data_pos  = 0;    // current pos in data received so far
    o_prev_pos  = 0;    // size of data received so far
    o_data_size = o_stride*o_height;
    o_data      = new unsigned char[o_data_size];
    memset(o_data, 0, o_data_size);

    // object has been initialized
    o_plc->plc_initialized = true;
    o_do_final = false;
}


// JPEG scanline processor.
// (public virtual)
//
void
PLCImageJPEG::ScanlineProc()
{
    if (o_do_final) {
        FinalPass();
        return;
    }

    jpeg_decompress_struct *cinfo = &o_cinfo;
    plc_jpeg_err_mgr *jerr  = &o_jerr;

    // Exit override must be done for each invocation of this routine,
    // set_jmp saves the current environment, which differs each time
    // this routine is called.

    if (setjmp(jerr->setjmp_buffer)) {

        // JPEG signalled an error, abort this PLC.
        // Necessary cleanup will be done by Destructor.

        // abort this PLC
        o_plc->plc_status = PLC_ABORT;
        return;
    }

    // a new pass
    if (cinfo->input_scan_number != cinfo->output_scan_number) {
        cinfo->do_block_smoothing = true;

        // rewind output buffer
        o_prev_pos = 0;
        o_data_pos = 0;

        // will never suspend as we are only using one-pass quantization
        jpeg_start_output(cinfo, cinfo->input_scan_number);

        // Read colormap only if this is the first pass.  As we are
        // using dithered ordering, it will be the same for each pass.
        // The final colormap will be read by FinalPass.

        if (cinfo->input_scan_number == 1)
            ReadJPEGColormap(cinfo);
    }

    unsigned char *r = o_data + o_data_pos;

    // keep processing scanlines as long as we have data available
    JSAMPROW buffer[1];         // row pointer array for read_scanlines

    int cnt = 0;
    while (cinfo->output_scanline < cinfo->output_height) {
        buffer[0] = r;

        // Read a new scanline and break if jpeg_read_scanlines
        // couldn't read anymore scanlines (input suspended) or when
        // the PLC gets de-activated.

        if (jpeg_read_scanlines(cinfo, buffer, 1) != 1)
            break;
        // next data slot
        r += o_stride;
        cnt++;
    }

    // Note:  since we have set the buffered_image flag, data_pos will
    // probably become equal to data_size after the first scan or so.

    o_data_pos = cinfo->output_scanline*o_stride;

    // pass completed
    if (cinfo->output_scanline == cinfo->output_height) {
        // skip to next scan
        if ((jpeg_finish_output(cinfo)) == false)
            return;     // no more data, suspended or aborted, return
    }

    // and this PLC has finished when no more input is available
    if (jpeg_input_complete(cinfo) &&
            cinfo->input_scan_number == cinfo->output_scan_number) {

        // only move to final dithering pass if requested
        switch (o_plc->plc_owner->im_html->htm_perfect_colors) {
        case AUTOMATIC:

            // We decide by ourselves:  if the image has less than 1/3
            // of the requested image colors, we dither, else we
            // don't.

            if (3*o_nused-1 < o_cmapsize) {
                o_do_final = true;
                break;
            }
            o_plc->plc_status = PLC_COMPLETE;
            o_plc->plc_obj_funcs_complete = true;
            break;
        case ALWAYS:
            // always dither
            o_do_final = true;
            break;
        case NEVER:  // no dithering wanted at all
        default:
            o_plc->plc_status = PLC_COMPLETE;
            o_plc->plc_obj_funcs_complete = true;
            break;
        }
    }
}


// Perform last pass on the decoded image data to do proper color
// quantization.  This function is only used when the
// progressivePerfectColor resource has been set to true (it's a very
// time consuming operation and causes a colorflash while the results
// aren't always good.  It is false by default).
// (public virtual)
//
void
PLCImageJPEG::FinalPass()
{
    jpeg_decompress_struct *cinfo = &o_cinfo;
    plc_jpeg_err_mgr *jerr  = &o_jerr;
    htmImage *image = o_image;
    htmImageInfo *info  = o_info;

    // Exit override must be done for each invocation of this routine,
    // set_jmp saves the current environment, which differs each time
    // this routine is called.

    if (setjmp(jerr->setjmp_buffer)) {

        // JPEG signalled an error, abort this PLC.
        // Necessary cleanup will be done by Destructor.

        // abort this PLC
        o_plc->plc_status = PLC_ABORT;

        return;
    }

    // The decoded image is by now completely decoded and has been
    // buffered in the current decompressor.  Set proper color
    // quantization params and initialize the final pass.

    cinfo->quantize_colors    = true;
    cinfo->two_pass_quantize  = true;
    cinfo->dither_mode        = JDITHER_FS;
    cinfo->colormap           = 0;
    cinfo->desired_number_of_colors =
        o_plc->plc_owner->im_html->htm_max_image_colors;

    // sanity
    if (image->npixels == 0) {
        o_plc->plc_owner->im_html->warning("JPEG FinalPass",
        "PLC Internal Error: no colors found in final JPEG decoding pass.");
        o_plc->plc_status = PLC_ABORT;
        return;
    }

    // Will never suspend as the image is already totally decoded, so
    // it's an error if it does suspend!

    if ((jpeg_start_output(cinfo, cinfo->input_scan_number)) == false) {
        o_plc->plc_owner->im_html->warning("JPEG FinalPass",
            "JPEG Error: I/O suspension while initializing final decoding "
            "pass.\n    Final pass not performed.");

        // PLC completed
        o_plc->plc_status = PLC_COMPLETE;
        o_plc->plc_obj_funcs_complete = true;
        return;
    }

    // rewind output buffer
    o_prev_pos = 0;
    o_data_pos = 0;

    // keep processing scanlines as long as we have data available
    unsigned char *r = o_data;
    JSAMPROW buffer[1];         // row pointer array for read_scanlines
    while (cinfo->output_scanline < cinfo->output_height) {
        buffer[0] = r;
        if ((jpeg_read_scanlines(cinfo, buffer, 1)) == 0) {
            o_plc->plc_owner->im_html->warning("JPEG FinalPass",
                "Aaaaie! JPEG I/O suspension in final decoding pass!");
            o_plc->plc_status = PLC_ABORT;
            return;
        }
        r += o_stride;
    }

    // finish this final pass
    if ((jpeg_finish_output(cinfo)) == false) {
        // this is very unlikely to happen
        o_plc->plc_owner->im_html->warning("JPEG FinalPass",
            "Aaaaie! JPEG I/O suspension in final output pass! Aborting "
            "image load...");
        o_plc->plc_status = PLC_COMPLETE;
        return;
    }

    // This will equal o_data_size; it's the final image data.
    o_data_pos = cinfo->output_scanline*o_stride;

    // get rid of current htmImageInfo RGB arrays
    delete [] info->reds;
    info->reds = info->greens = info->blues = 0;

    // now reset used colors array
    for (int i = 0; i < MAX_IMAGE_COLORS; i++) {
        o_used[i] = 0;
        o_xcolors[i] = 0L;
    }

    o_nused = 1;

    // read new colormap
    ReadJPEGColormap(cinfo);

    // allocate new RGB arrays
    info->reds   = new unsigned short[3*o_cmapsize];
    info->greens = info->reds   + o_cmapsize;
    info->blues  = info->greens + o_cmapsize;
    memset(info->reds, 0, 3*o_cmapsize*sizeof(short));

    // pass completed
    o_plc->plc_obj_funcs_complete = true;
}


// Read the colormap allocated for a JPEG image.
// (Private non-virtual)
//
void
PLCImageJPEG::ReadJPEGColormap(struct jpeg_decompress_struct *cinfo)
{
    // free previous colormap
    if (o_cmap) {
        delete [] o_cmap;
        o_cmap = 0;
    }

    // allocate new colormap
    o_cmapsize = cinfo->actual_number_of_colors;
    o_cmap = new htmColor[o_cmapsize];

    // fill colormap. Upscale RGB to 16bits precision
    if (cinfo->out_color_components == 3) {
        int cshift = 16 - cinfo->data_precision;
        o_colorclass = IMAGE_COLORSPACE_RGB;

        for (int i = 0; i < o_cmapsize; i++) {
            o_cmap[i].red   = cinfo->colormap[0][i] << cshift;
            o_cmap[i].green = cinfo->colormap[1][i] << cshift;
            o_cmap[i].blue  = cinfo->colormap[2][i] << cshift;
            o_cmap[i].pixel = i;
        }
    }
    else {
        int cshift = 16 - cinfo->data_precision;
        o_colorclass = IMAGE_COLORSPACE_GRAYSCALE;
        for (int i = 0; i < o_cmapsize; i++) {
            o_cmap[i].red = o_cmap[i].green = o_cmap[i].blue =
                cinfo->colormap[0][i] << cshift;
            o_cmap[i].pixel = i;
        }
    }

    // As we are now sure every color values lies within the range
    // 0-2^16, we can downscale to the range 0-255

    for (int i = 0; i < o_cmapsize; i++) {
        o_cmap[i].red   >>= 8;
        o_cmap[i].green >>= 8;
        o_cmap[i].blue  >>= 8;
    }
    // get image depth
    o_depth = 1;
    while (o_cmapsize > (1 << o_depth))
        o_depth++;
}
// End of PLCImageJPEG functions


namespace {
    // JPEG error override. Called when libjpeg signals an error.
    //
    void
    ErrorExit(j_common_ptr cerr)
    {
        char err_msg[JMSG_LENGTH_MAX];
        j_decompress_ptr cinfo   = (j_decompress_ptr)cerr;
        plc_jpeg_source_mgr *src = (plc_jpeg_source_mgr*)cinfo->src;
        plc_jpeg_err_mgr *jerr   = (plc_jpeg_err_mgr*)cinfo->err;
        PLC *plc = src->plc;

        // create the error message
        (*jerr->pub.format_message)(cerr, err_msg);

        // show it
        src->plc->plc_owner->im_html->warning("JPEG ErrorExit",
            "libjpeg error on image %s:\n    %s.", plc->plc_url, err_msg);

        // jump to saved restart point
        longjmp(jerr->setjmp_buffer, 1);
    }


    // JPEG init_buffer method. Allocates the input buffer.
    //
    void
    InitSource(j_decompress_ptr cinfo)
    {
        plc_jpeg_source_mgr *src = (plc_jpeg_source_mgr*)cinfo->src;
        PLC *plc = src->plc;

        if (src->buf_size == 0) {

            // Private fields.
            // We use our own private jpeg input buffer.

            src->buf_size = plc->plc_input_size;
            src->buffer   = new unsigned char[src->buf_size];

            // public fields
            src->pub.next_input_byte = src->buffer;
            src->pub.bytes_in_buffer = 0;
        }
    }


    // JPEG fill_input_buffer method.  We use the Suspended I/O facility
    // of the jpeg library.  false when decompression of the current
    // decompressor should be suspended (not enough data or data is being
    // skipped), true when the decompresser should continue.
    //
    // This is the core of the JPEG PLC object.  It updates the JPEG input
    // buffer by making appropriate data requests from the current PLC,
    // inserting a fake jpeg EOI marker when all data has been read or by
    // skipping bogus input data if the jpeg skip_input_data method has
    // been called.  The hardest part to figure out for progressive JPEG
    // support was that, if input is to be suspended, the
    // pub.next_input_byte and pub.bytes_in_buffer fields MUST be set to
    // 0.  Havoc occurs otherwise.
    // (this took me two days to figure out...)
    //
    boolean
    FillInputBuffer(j_decompress_ptr cinfo)
    {
        plc_jpeg_source_mgr *src = (plc_jpeg_source_mgr*)cinfo->src;
        PLC *plc = src->plc;

        // If the current input buffer is empty, we need to suspend the
        // current JPEG stream.  If there is any data left in the jpeg
        // stream, it's saved so it can be restored upon the next
        // invocation of this routine.
        //
        // buffer is of type JOCTET, which is an unsigned char (guchar) on
        // all Unix systems.  next_input_byte points to the next byte that
        // is to be read from the buffer.  Data before this byte can be
        // discarded safely, while data after this byte must be saved.
        // bytes_in_buffer indicates how many bytes are left in the
        // current buffer.

        if (plc->plc_left == 0) {

            // Save current state by backtracking in the main PLC input
            // buffer.  This ensures we have things right when a
            // STREAM_RESIZE request was made and that we stay aligned
            // with the incoming data.

            if (src->pub.bytes_in_buffer) {
                plc->plc_left    = src->pub.bytes_in_buffer;
                plc->plc_next_in =
                    plc->plc_buffer + (plc->plc_size - plc->plc_left);
            }

            // issue a new data request
            plc->plc_min_in = 0;
            plc->plc_max_in = plc->plc_input_size;
            plc->DataRequest();

            // check end of data
            if (plc->plc_data_status == STREAM_END) {
                src->buffer[0] = (JOCTET)0xFF;
                src->buffer[1] = (JOCTET)JPEG_EOI;
                src->pub.next_input_byte = src->buffer;
                src->pub.bytes_in_buffer = 2;
                return (true);
            }

            // need to resize jpeg buffer if PLC input buffer size changed
            if (plc->plc_input_size != src->buf_size) {
                unsigned char *tb = src->buffer;
                src->buffer = new unsigned char[plc->plc_input_size];
                memcpy(src->buffer, tb, src->buf_size < plc->plc_input_size ?
                    src->buf_size : plc->plc_input_size);
                src->buf_size = plc->plc_input_size;
                delete [] tb;
            }

            // suspend decoder
            src->pub.next_input_byte = 0;
            src->pub.bytes_in_buffer = 0;
            return (false);
        }

        // move data left to the beginning of the current jpeg buffer
        if (src->pub.bytes_in_buffer) {

            // move data left to the beginning of the buffer
            memmove(src->buffer, src->pub.next_input_byte,
                src->pub.bytes_in_buffer);
            src->pub.next_input_byte = src->buffer;
        }

        // We have data available. We can do two things here:
        // 1.  skip input data if the jpeg skip_input_data method has been
        //     told to skip more data than was available (in which nskip
        //     contains the no of bytes to be skipped).
        // 2. copy data from the PLC input buffer to the JPEG input buffer.

        // Do we still have to skip input data?
        if (src->nskip) {
            // sanity
            src->pub.bytes_in_buffer = 0;
            src->pub.next_input_byte = 0;

            // maximum no of bytes we may consume
            int len = (src->buf_size > (int)plc->plc_left ?
                (int)plc->plc_left : src->buf_size);

            // don't consume more bytes than we may
            if (len > src->nskip)
                len = src->nskip;

            unsigned char *dest = src->buffer;

            // make a data request. This should never fail.
            size_t status;
            if ((status = plc->ReadOK(dest, len)) == 0) {

                if (plc->plc_data_status == STREAM_END) {
                    src->buffer[0] = (JOCTET)0xFF;
                    src->buffer[1] = (JOCTET)JPEG_EOI;
                    src->pub.next_input_byte = src->buffer;
                    src->pub.bytes_in_buffer = 2;
                    return (true);
                }

                // this should *never* happen
                src->plc->plc_owner->im_html->warning("JPEG FillInputBuffer",
                    "Read error "
                    "while skipping jpeg input data: PLCReadOK failed for %i\n"
                    "    bytes. Skipping this image.", len);

                plc->plc_status = PLC_ABORT;
                return (false);
            }

            // this amount of data to be skipped upon next call
            src->nskip -= status;

            // still skipping data or no more data
            if (src->nskip || plc->plc_left == 0) {
                return (false);
            }

            // no more data to be skipped and input available, fall through
        }

        // maximum no of bytes we can consume
        int len = src->buf_size - src->pub.bytes_in_buffer;

        // Maximum no of bytes available from input (prevents ReadOK from
        // issuing a data request by itself).

        if (len > (int)plc->plc_left)
            len = plc->plc_left;

        unsigned char *dest = src->buffer + src->pub.bytes_in_buffer;

        // This one should *never* fail, so it's an error if it does.
        // Check for STREAM_END anyway.

        size_t status;
        if ((status = plc->ReadOK(dest, len)) == 0) {

            if (plc->plc_data_status == STREAM_END) {
                src->buffer[0] = (JOCTET)0xFF;
                src->buffer[1] = (JOCTET)JPEG_EOI;
                src->pub.next_input_byte = src->buffer;
                src->pub.bytes_in_buffer = 2;
                return (true);
            }

            // we should never get here
            src->plc->plc_owner->im_html->warning("JPEG FillInputBuffer",
                "Read error while filling jpeg input buffer: PLCReadOK "
                "failed for %i\n    bytes.  Skipping this image.", len);

            // suspend decoder
            src->pub.next_input_byte = 0;
            src->pub.bytes_in_buffer = 0;

            // abort this PLC
            plc->plc_status = PLC_ABORT;
            return (false);
        }

        // update source manager fields
        src->pub.next_input_byte = (JOCTET*)src->buffer;
        src->pub.bytes_in_buffer += (size_t)status;

        // continue processing
        return (true);
    }


    // JPEG skip_input_data method.  Due to the fact that this function
    // may not be suspended, we must save the amount of data to skip to a
    // public field in the source manager
    //
    void
    SkipInputData(j_decompress_ptr cinfo, long nskip)
    {
        plc_jpeg_source_mgr *src = (plc_jpeg_source_mgr*)cinfo->src;

        if (nskip != 0) {
            if (nskip > (long)src->pub.bytes_in_buffer) {
                // this much data needs to be skipped by FillInputBuffer
                src->nskip = (nskip - src->pub.bytes_in_buffer);

                // skip input data
                src->pub.bytes_in_buffer = 0;
                src->pub.next_input_byte = 0;
            }
            else {
                // skip input data
                src->pub.next_input_byte += (size_t)nskip;
                src->pub.bytes_in_buffer -= (size_t)nskip;
            }
        }
    }


    // JPEG term_source method.
    //
    void
    TermSource(j_decompress_ptr cinfo)
    {
        plc_jpeg_source_mgr *src = (plc_jpeg_source_mgr*)cinfo->src;

        if (src->buf_size)
            delete [] src->buffer;
        src->buffer   = 0;
        src->buf_size = 0;
    }


    // jpeg_stdio_src variant for JPEG PLC.
    //
    void
    SetSource(j_decompress_ptr cinfo, PLC *plc)
    {

        if (cinfo->src == 0) {
            // first time for this JPEG object
            cinfo->src = (struct jpeg_source_mgr*)
                (*cinfo->mem->alloc_small) ((j_common_ptr)cinfo,
                    JPOOL_PERMANENT, sizeof(plc_jpeg_source_mgr));
        }

        plc_jpeg_source_mgr *src = (plc_jpeg_source_mgr*)cinfo->src;
        src->plc      = plc;
        src->buffer   = 0;
        src->buf_size = 0;
        src->nskip    = 0;
        src->pub.init_source       = InitSource;
        src->pub.fill_input_buffer = FillInputBuffer;
        src->pub.skip_input_data   = SkipInputData;
        src->pub.resync_to_restart = jpeg_resync_to_restart;    // default
        src->pub.term_source       = TermSource;
        src->pub.bytes_in_buffer   = 0;
        src->pub.next_input_byte   = 0;
    }
}


#else

PLCImageJPEG::~PLCImageJPEG()
{
}


// Dummy JPEG PLC support functions.  Only Init will be called by the
// main PLC cycler.  The other two are here to prevent linker errors.
//
void
PLCImageJPEG::Init()
{
    o_plc->plc_status = PLC_ABORT;
}


void
PLCImageJPEG::ScanlineProc()
{
    o_plc->plc_status = PLC_ABORT;
}


void
PLCImageJPEG::FinalPass()
{
    o_plc->plc_status = PLC_ABORT;
}

#endif



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
#include "htm_format.h"
#include "htm_string.h"

PLCImage::PLCImage(PLC *plc, htmImageInfo *info)
{
    o_type                  = plcAnyImage;
    o_plc                   = plc;

    o_buffer                = 0;
    o_buf_size              = 0;
    o_byte_count            = 0;
    o_buf_pos               = 0;

    // public fields
    o_depth                 = 0;
    o_colorclass            = 0;
    o_transparency          = 0;
    o_cmap                  = 0;
    o_cmapsize              = 0;
    o_ncolors               = 0;
    o_width                 = 0;
    o_height                = 0;
    o_npasses               = 0;
    o_curr_pass             = 0;
    o_curr_scanline         = 0;
    o_stride                = 0;
    o_data                  = 0;
    o_data_size             = 0;
    o_data_pos              = 0;
    o_prev_pos              = 0;

    // private fields
    memset(o_used, 0, sizeof(o_used));
    o_nused                 = 0;
    memset(o_xcolors, 0, sizeof(o_xcolors));
    o_bg_pixel              = 0;
    o_bg_cmap               = 0;
    o_bg_cmapsize           = 0;
    o_pixmap                = 0;
    o_clipmask              = 0;
    o_clip_data             = 0;
    o_scaled_data           = 0;
    o_sc_start              = 0;
    o_sc_end                = 0;
    o_is_scaled             = 0;
    o_ximage                = 0;
    o_info                  = info;
    o_image                 = 0;
}


// Clean up and transfer final parameters to info.
//
void
PLCImage::finalize(htmImageInfo *info)
{
    // this must always be destroyed
    if (o_ximage) {
        o_plc->plc_owner->im_html->htm_tk->tk_release_image(o_ximage);
        o_ximage = 0;
    }

    // no longer need the scaled data, image has been fully processed
    if (o_is_scaled) {
        delete [] o_scaled_data;
        o_scaled_data = 0;
        o_is_scaled = false;
    }

    // Transfer stuff to the info structure.
    if (info) {
        int bgp = o_bg_pixel;
        if (bgp < 0 || bgp >= o_nused)
            bgp = 0;
        info->data          = o_data;           // decoded image data
        info->clip          = o_clip_data;      // raw clipmask data
        info->bg            = o_used[bgp] - 1;
        info->colorspace    = o_colorclass;     // image colorclass
        info->transparency  = o_transparency;   // image transparency
        info->depth         = o_depth;          // image depth
        info->ncolors       = o_nused-1;        // no of colors in image
        info->scolors       = o_ncolors;        // original no of cols
        info->width         = o_width;          // reset to real width
        info->height        = o_height;         // reset to real height

        o_data              = 0;
        o_clip_data         = 0;

        // this image is no longer being loaded progressively
        info->options &= ~IMAGE_PROGRESSIVE;

        // Adjust image RGB components to only contain the number of
        // colors used in this image.  Only do it when we have
        // allocated any colors for this image (we didn't allocate
        // colors if this PLC was aborted prematurely).

        if (info->ncolors && info->reds != 0 &&
                (int)info->ncolors != o_cmapsize) {

            // save old RGB arrays
            unsigned short *reds   = info->reds;
            unsigned short *greens = info->greens;
            unsigned short *blues  = info->blues;

            // allocate new RGB arrays
            info->reds   = new unsigned short[3 * info->ncolors];
            info->greens = info->reds   + info->ncolors;
            info->blues  = info->greens + info->ncolors;

            // copy old RGB arrays
            memcpy(info->reds, reds, info->ncolors*sizeof(unsigned short));
            memcpy(info->greens, greens, info->ncolors*sizeof(unsigned short));
            memcpy(info->blues, blues, info->ncolors*sizeof(unsigned short));

            // free old RGB arrays
            delete [] reds;

            // update this as well
            info->scolors = info->ncolors;
        }
    }

    // free remaining PLC resources
    delete [] o_cmap;
    o_cmap = 0;
    delete [] o_bg_cmap;
    o_bg_cmap = 0;
    delete [] o_buffer;
    o_buffer = 0;
}


namespace {
    bool
    suffix(const char *str, const char *suf)
    {
        int n1 = strlen(str);
        int n2 = strlen(suf);
        if (n1 >= n2 && !strcmp(suf, str + n1 - n2))
            return (true);
        return (false);
    }
}


// In this version, the progressive loading is timed by the caller,
// through this function.  The cycler below returns immediately and
// disables further cycling.  It is up to the caller to call this
// function repeatedly, for each image, until 0 is returned.
//
bool
htmImageManager::callPLC(const char *url)
{
    // Return if we haven't got any PLC's installed or if progressive
    // image loading has been suspended.

    if (im_plc_buffer == 0 || im_plc_suspended)
        return (false);

    int i = 0;
    for (PLC *plc = im_plc_buffer; plc && i < im_num_plcs;
            plc = plc->plc_next, i++) {
        if (suffix(url, plc->plc_url)) {
            im_plc_buffer = plc;

            if (plc->plc_status == PLC_SUSPEND) {

               // PLCDataRequest failed on previous call, skip it this
               // time to give the connection a bit more time and set
               // plc_status so it will get activated the next time
               // this plc is called.

                plc->plc_status = PLC_ACTIVE;
                return (true);
            }
            if (plc->plc_status == PLC_ACTIVE)
                plc->Run();

            if (plc->plc_status == PLC_COMPLETE ||
                    plc->plc_status == PLC_ABORT) {
                plc->Remove();
                return (false);
            }
            return (true);
        }
    }
    return (false);
}


// Suspend progressive image loading.
//
void
htmImageManager::imageProgressiveSuspend()
{
    PLC *plc;
    if ((plc = im_plc_buffer) == 0)
        // nothing to suspend
        return;

    // first suspend all active PLC's. Don't mess with other PLC states
    for (int i = 0; i < im_num_plcs; plc = plc->plc_next, i++) {
        if (plc->plc_status == PLC_ACTIVE)
            plc->plc_status = PLC_SUSPEND;
    }

    // set global PLC suspension flag
    im_plc_suspended = true;
}


// reactivate progressive image loading
void
htmImageManager::imageProgressiveContinue()
{
    PLC *plc;
    if ((plc = im_plc_buffer) == 0)
        // nothing to do
        return;

    // first activate all suspended PLC's. Don't mess with other PLC states
    for (int i = 0; i < im_num_plcs; plc = plc->plc_next, i++) {
        if (plc->plc_status == PLC_SUSPEND)
            plc->plc_status = PLC_ACTIVE;
    }

    // reactivate cycler
    im_plc_suspended = false;
}


// Terminate progressive image loading.
//
void
htmImageManager::imageProgressiveKill()
{
    if (im_plc_buffer == 0)
        return;

    // kill the bastards!
    im_plc_suspended = true;
    killPLCCycler();
}


PLC::PLC(htmImageManager *im, const char *u)
{
    plc_url                 = lstring::copy(u);
    plc_object              = 0;
    plc_owner               = im;
    plc_buffer              = new unsigned char[PLC_MAX_BUFFER_SIZE];
    plc_buf_size            = PLC_MAX_BUFFER_SIZE;
    plc_size                = 0;
    plc_left                = 0;
    plc_next_in             = 0;

    plc_input_buffer        = new unsigned char[PLC_MAX_BUFFER_SIZE];
    plc_input_size          = PLC_MAX_BUFFER_SIZE;
    plc_total_in            = 0;
    plc_max_in              = PLC_MAX_BUFFER_SIZE;
    plc_min_in              = 0;

    plc_status              = PLC_ACTIVE;
    plc_data_status         = STREAM_OK;
    plc_initialized         = false;

    plc_priv_data           = 0;
    plc_user_data           = 0;

    plc_obj_funcs_complete  = false;

    plc_next                = 0;
    plc_prev                = 0;
};


// Create a PLC for the widget.  The priv_data argument is not the
// same as the user_data field.  It allows an application to attach
// internal data to a PLC.
//
PLC*
htmImageManager::PLCCreate(htmImageInfo *priv_data, const char *url)
{
    PLC *plc = new PLC(this, url);
    plc->plc_priv_data = priv_data;

    // insert it
    plc->Insert();

    return (plc);
}


// Kill all outstanding PLC procedures.  This function is called when
// the current document is discarded (by loading a new document or
// destroying the widget).
//
void
htmImageManager::killPLCCycler()
{
    im_plc_suspended = true;

    // now remove all outstanding plc's
    while (im_plc_buffer != 0) {
        // abort all outstanding PLC's
        PLC *plc = im_plc_buffer;
        plc->plc_status = PLC_ABORT;
        plc->Remove();
    }
    im_num_plcs = 0;
}
// End of htmImageManager functions


PLC::~PLC()
{
    delete [] plc_url;
    delete plc_object;             // all object data
    delete [] plc_buffer;          // current data buffer
    delete [] plc_input_buffer;    // current input buffer
}


// Make a get_data() request for the current PLC.  Returns true when
// request was served, false if not.  plc_status is also updated to
// reflect actual request return code.
//
bool
PLC::DataRequest()
{
    {
        PLC *pt = this;
        if (!pt)
            return (false);
    }
    if (!plc_owner->im_html->htm_if)
        return (false);

    // very useful sanity
    if (plc_max_in == 0 || plc_max_in < plc_min_in)
        plc_max_in = plc_input_size;

    // next_in is the current position in the destination buffer, so
    // we need to make sure that next and max_in do not exceed input
    // buffer size

    if (plc_left + plc_max_in > plc_buf_size)
        plc_max_in = plc_buf_size - plc_left;

    // yet another sanity
    if (plc_max_in && plc_min_in >= plc_max_in)
        plc_min_in = 0;

    // fill stream buffer
    htmPLCStream context;
    context.total_in  = plc_total_in;   // bytes received so far
    context.min_out   = plc_min_in;     // minimum no of bytes requested
    context.max_out   = plc_max_in;     // maximum no of bytes requested
    context.user_data = plc_user_data;  // user_data for this PLC

    // get data from the external data stream
    int status;
    if ((status = plc_owner->im_html->htm_if->get_image_data(&context,
            plc_input_buffer)) > 0) {
        // bad copy, issue warning but proceed
        if (status < (int)plc_min_in) {
            plc_owner->im_html->warning("PLC DataRequest",
                "Improperly served PLC get_data() request:\n"
                "    Received %i bytes while %i is minimally required.",
                status, plc_min_in);
        }
        if (status > (int)plc_max_in) {
            plc_owner->im_html->warning("PLC DataRequest",
                "Improperly served PLC get_data() request:\n"
                "    Received %i bytes while %i is maximally allowed. Excess "
                " data ignored.", status, plc_max_in);
            status = plc_max_in;
        }

        // more than min_in bytes returned, activate plc
        plc_status = PLC_ACTIVE;

        // update received byte count
        plc_total_in += status;

        // move data left to the beginning of the buffer (thereby discarding
        // already processed data)

        if (plc_left)
            memmove(plc_buffer, plc_buffer + (plc_size - plc_left),
                plc_left);

        // append newly received data
        memcpy(plc_buffer + plc_left, plc_input_buffer, status);

        // this many bytes are valid in the buffer
        plc_size = plc_left + status;
        // reset current ptr position
        plc_next_in = plc_buffer;

        // this many bytes left for reading in the buffer
        plc_left += status;

        return (true);
    }

    // check return value in most logical (?) order
    if (status == STREAM_RESIZE) {
        // we have been requested to resize the buffers
        if (context.max_out <= 0) {
            // this is definitly an error
            plc_owner->im_html->warning("PLC DataRequest",
                "Request to resize PLC buffers to zero!");
            return (false);
        }

        // resize input buffer
        unsigned char *tbf = plc_input_buffer;
        plc_input_buffer = new unsigned char[context.max_out];
        memcpy(plc_input_buffer, tbf,
            plc_input_size < (int)context.max_out ?
            plc_input_size : (int)context.max_out);
        delete [] tbf;

        plc_input_size   = context.max_out;
        plc_max_in       = context.max_out;

        // Always backtrack if we have data left in the current buffer.
        // We make it ourselves easy here and let the user worry about it.

        if (plc_left) {
            plc_total_in -= plc_left;
            plc_left      = 0;
            plc_next_in   = 0;
            plc_size      = 0;
        }

        // resize current data buffer
        tbf = plc_buffer;
        plc_buffer = new unsigned char[context.max_out];
        memcpy(plc_buffer, tbf,
            plc_buf_size < context.max_out ?
            plc_buf_size : context.max_out);
        delete [] tbf;

        plc_buf_size     = context.max_out;

        // and call ourselves again with the new buffers in place
        return (DataRequest());
    }

    if (status == STREAM_SUSPEND) {
        // not enough data available, suspend this plc
        plc_status = PLC_SUSPEND;
        plc_data_status = STREAM_SUSPEND;
    }
    else if (status == STREAM_END) {
        // all data has been received, terminate plc
        plc_status = PLC_COMPLETE;
        plc_data_status = STREAM_END;
    }
    else {
        // plc has been aborted
        plc_status = PLC_ABORT;
        plc_data_status = STREAM_ABORT;
    }
    return (false);
}


// Copy len bytes to buf from an ImageBuffer.
//
size_t
PLC::ReadOK(unsigned char *buf, int len)
{
    for (;;) {
        // plc_left is the number of bytes left in the input buffer
        if (len <= (int)plc_left) {
            memcpy(buf, plc_next_in, len);
            // new position in buffer
            plc_next_in += len;
            // this many bytes still available
            plc_left -= len;
            return (len);
        }
        // not enough data available, make a request
        plc_min_in = len - plc_left;
        plc_max_in = PLC_MAX_BUFFER_SIZE;

        if (!DataRequest())
            break;  // suspended, aborted or end of data
    }
    return (0);
}


// Get the next amount of data from the input buffer.
//
size_t
PLC::GetDataBlock(unsigned char *buf)
{
    unsigned char count = 0;
    if (!ReadOK(&count, 1))
        return (0);
    if (!count)
        return (0);

    if (!ReadOK(buf, count)) {
        // back up 1 for byte count
        plc_left++;
        plc_next_in--;
        return (0);
    }
    return (count);
}


// Call the end_data() function to signal this PLC is terminating.
//
void
PLC::EndData()
{
    if (!plc_object)
        return;
    if (!plc_owner->im_html->htm_if)
        return;
    htmPLCStream context;
    context.total_in  = plc_total_in;   // bytes received so far
    context.min_out   = 0;              // meaningless
    context.max_out   = 0;              // meaningless
    context.user_data = plc_user_data;  // user_data for this PLC

    htmImageInfo *image = plc_object->o_info;
    plc_owner->im_html->htm_if->end_image_data(&context, image, PLC_IMAGE,
        (plc_status == PLC_COMPLETE));
}


// Insert a PLC in the PLC ringbuffer.
//
void
PLC::Insert()
{
    // first element, let it point to itself
    if (plc_owner->im_plc_buffer == 0) {
        plc_next = plc_prev = this;
        plc_owner->im_plc_buffer = this;
        plc_owner->im_num_plcs++;
        return;
    }

    // We already have elements in the plc buffer.  The new plc is
    // inserted as the next element of the current PLC so it will get
    // activated immediatly.

    PLC *tmp = plc_owner->im_plc_buffer->plc_next;
    tmp->plc_prev = this;
    plc_next = tmp;
    plc_prev = plc_owner->im_plc_buffer;
    plc_owner->im_plc_buffer->plc_next = this;

    // keep up running PLC count as well
    plc_owner->im_num_plcs++;
}


// Remove a PLC from the PLC ringbuffer and calls the object
// destructor method.
//
void
PLC::Remove()
{
    if (!plc_owner->im_html->htm_if)
        return;

    // call finalize method if this plc was ended prematurely
    if (plc_obj_funcs_complete == false)
        finalize();

    // Call the end_data() function to signal the user that this PLC
    // is about to be destroyed.

    EndData();

    delete plc_object;
    plc_object = 0;

    // now remove it
    PLC *next = plc_next;
    PLC *prev = plc_prev;

    // this is the last PLC in the plc ringbuffer
    if (next == this || prev == this) {
        // kill the main plc cycler
        plc_owner->im_plc_buffer = 0;
        plc_owner->killPLCCycler();
    }
    else {
        next->plc_prev = prev;
        prev->plc_next = next;

        // if this is the current plc, advance the ring buffer
        if (plc_owner->im_plc_buffer == this)
            plc_owner->im_plc_buffer = next;
    }

    // If no more PLC's are left in the buffer, call the end_data()
    // function to signal the user that we are done loading
    // progressively.

    if (plc_owner->im_plc_buffer == 0 || plc_owner->im_num_plcs == 1)
        plc_owner->im_html->htm_if->end_image_data(0, 0, PLC_FINISHED, true);

    // keep up running PLC count as well
    if (plc_owner->im_num_plcs)
        plc_owner->im_num_plcs--;

    if (plc_owner->im_num_plcs == 0 && plc_owner->im_plc_buffer != 0)
        plc_owner->im_html->warning("PLC Remove",
            "Internal PLC Error: ringbuffer != 0 but num_plcs == 0.");

    // destroy the PLC itself
    delete this;
}


// Activate a PLC.
//
void
PLC::Run()
{
    // see if the object has been set
    if (!plc_object) {
        new_image_object();
        return;
    }

    if (plc_owner->im_plc_suspended)
        return;

    // see if we have been initialized
    if (plc_initialized == false) {
        // call object initializer
        plc_object->Init();
        return;
    }

    plc_object->ScanlineProc();

    // If the plc_status is PLC_ACTIVE or PLC_COMPLETE when the above
    // function returns, call the object transfer function as well.

    if (plc_status == PLC_ACTIVE || plc_status == PLC_COMPLETE)
        transfer();

    // If the object functions are finished, call the finalize method
    // as well.

    if (plc_obj_funcs_complete == true) {
        finalize();
        plc_status = PLC_COMPLETE;
    }
}


// Init:
//   function that performs initialization of the object for which a PLC
//   is to be used.  This may involve getting more object-specific data.
//   When this function finishes, it should set the obj_set field to
//   true.
// Transfer:
//   function that needs to perform intermediate transfer of
//   object-specific data to the final destination.
// Finalize:
//   function that needs to perform final transfer of object-specific
//   data to the final destination.

// NOTE PNG and XPM aren't supported.  No image will be loaded.

void
PLC::new_image_object()
{
    static unsigned char png_magic[8] =
        {137, 80, 78, 71, 13, 10, 26, 10};

    // get first 10 bytes to determine the image type
    plc_min_in = 10;
    plc_max_in = PLC_MAX_BUFFER_SIZE;

    // We need to know the image type before we can start doing
    // anything.

    if (!(DataRequest()))
        return;

    unsigned char magic[10];
    memcpy(magic, plc_buffer, 10);

    if (!(strncmp((char*)magic, "GIF87a", 6)) ||
            !(strncmp((char*)magic, "GIF89a", 6)))
        plc_object = new PLCImageGIF(this, plc_priv_data);
    else if (!(strncmp((char*)magic, "GZF87a", 6)) ||
            !(strncmp((char*)magic, "GZF89a", 6)))
        plc_object = new PLCImageGZF(this, plc_priv_data);
    else if (magic[0] == 0xff && magic[1] == 0xd8 && magic[2] == 0xff)
        plc_object = new PLCImageJPEG(this, plc_priv_data);
    else if (!(memcmp(magic, png_magic, 8)))
        plc_object = new PLCImagePNG(this, plc_priv_data);
    else if (!(strncmp((char*)magic, "/* XPM */", 9)))
        plc_object = new PLCImageXPM(this, plc_priv_data);
    else if (!(strncmp((char*)magic, "#define", 7)) ||
            (magic[0] == '/' && magic[1] == '*'))
        plc_object = new PLCImageXBM(this, plc_priv_data);
    else {
        plc_owner->im_html->warning("new_image_object",
            "%s: unsupported by PLC/unknown image type!", plc_url);
        plc_status = PLC_ABORT;
        return;
    }
}


// Intermediate image transfer function.  When this function is called
// for the first time, it initializes all common image fields (XImage,
// pixmap, color arrays, clipmask, etc...).
//
// The actual image composition is split in six parts:
// 1.  color counting; the numbers of colors used by the new chunk of
//     data is counted, and the RGB arrays for these colors are filled;
//
// 2.  data pixelization; the new chunk of raw data is mapped to the
//     pixel values assigned in the previous step;
//
// 3.  color allocation; if new colors are to be allocated that's done
//     now;
//
// 4.  XImage updating; scanlines represented by the new chunk of data
//     are added to the existing scanlines already present in the
//     XImage;
//
// 5.  Pixmap updating; the newly added scanlines are copied into the
//     destination drawable (a Pixmap);
//
// 6.  Display updating:  the updated portion of the pixmap is copied
//     to screen;
//
void
PLC::transfer()
{
    PLCImage *pi = plc_object;
    htmImage *image  = pi->o_image;
    htmImageInfo *info = pi->o_info;
    htmInterface *tk = plc_owner->im_html->htm_tk;

    int pixels [MAX_IMAGE_COLORS];

    if (info == 0) {
        plc_owner->im_html->warning("PLC Transfer",
            "%s: Internal PLC error\n    No htmImageInfo structure bound!",
            plc_url);
        plc_status = PLC_ABORT;
        return;
    }

    // don't do a thing if we haven't received any new data
    if (pi->o_prev_pos == pi->o_data_pos)
        return;

    // no HTML image stored yet, go pick it up
    if (image == 0) {
        bool need_redisplay = false;

        for (image = plc_owner->im_images; image &&
            image->html_image != info; image = image->next) ;
        if (image == 0) {
            plc_owner->im_html->warning("PLC Transfer",
                "%s: Internal PLC error\n    No image registered!",
                plc_url);
            plc_status = PLC_ABORT;
            return;
        }
        image->options |= IMG_PROGRESSIVE;

        pi->o_image = image;

        // store original image dimensions
        info->swidth  = pi->o_width;
        info->sheight = pi->o_height;

        // See if any dimensions were specified on the <IMG> element
        // or if we may not perform scaling.

        if (!image->HasDimensions() || !info->Scale()) {
            // store used image dimensions
            info->width  = image->width  = pi->o_width;
            info->height = image->height = pi->o_height;

            // Keep requested dimensions if we have them. Set to real
            // image dimensions otherwise.

            if (!image->HasDimensions()) {
                image->swidth  = image->width;
                image->sheight = image->height;
            }
        }
        // we have dimensions and we may scale
        else {
            // store used image dimensions
            info->width  = image->width  = image->swidth;
            info->height = image->height = image->sheight;

            // check see if we really need to scale
            if (image->swidth != (int)pi->o_width ||
                    image->sheight != (int)pi->o_height) {
                // we need to scale
                pi->o_is_scaled = true;
                pi->o_scaled_data =
                    new unsigned char[image->swidth * image->sheight];
            }
        }

        // Update imageWord dimensions as well.  owner can be 0 if
        // this is the body image.

        if (image->owner != 0 && image->owner->words != 0 &&
                image->owner->words[0].image == image) {

            // If the owning word had it's dimensions wrong (not set
            // before) we need to recompute the screen layout.

            if ((int)image->owner->words[0].area.width != image->width ||
                    (int)image->owner->words[0].area.height != image->height) {

                // store new image dimensions
                image->owner->words[0].area.width = image->width;
                image->owner->words[0].area.height = image->height;

                // redo screen layout as it will be incorrectly by now
                need_redisplay = true;
            }
        }

        // allocate pixmaps
        if ((pi->o_pixmap = tk->tk_new_pixmap(image->width,
                image->height)) == 0) {
            plc_owner->im_html->warning("PLC Transfer",
                "%s: failed to create pixmap.", plc_url);
            plc_status = PLC_ABORT;
            return;
        }

        // pre-tile the pixmap with the body image (if this isn't the
        // body image of course)

        if (plc_owner->im_body_image &&
                !plc_owner->im_body_image->IsBackground() &&
                !plc_owner->im_body_image->DelayedCreation() &&
                htmImageInfo::BodyImageLoaded(
                    plc_owner->im_body_image->html_image)) {

            int tile_width  = plc_owner->im_body_image->width;
            int tile_height = plc_owner->im_body_image->height;

            // compute correct image offsets
            int x_dist = image->owner->words[0].area.x;
            int y_dist = image->owner->words[0].area.y;

            int xs = plc_owner->im_html->viewportX(x_dist);
            int ys = plc_owner->im_html->viewportY(y_dist);

            int ntiles_x = (int)(x_dist/tile_width);
            int ntiles_y = (int)(y_dist/tile_height);

            int x_offset = x_dist - ntiles_x * tile_width;
            int y_offset = y_dist - ntiles_y * tile_height;

            int tsx = xs - x_offset;
            int tsy = ys - y_offset;

            tk->tk_set_draw_to_pixmap(pi->o_pixmap);
            tk->tk_tile_draw_pixmap(tsx, tsy, plc_owner->im_body_image->pixmap,
                0, 0, image->width, image->height);
            tk->tk_set_draw_to_pixmap(0);
        }
        else {
            tk->tk_set_foreground(plc_owner->im_html->htm_cm.cm_body_bg);
            tk->tk_set_draw_to_pixmap(pi->o_pixmap);
            tk->tk_draw_rectangle(true, 0, 0, image->width, image->height);
            tk->tk_set_draw_to_pixmap(0);
            tk->tk_set_foreground(plc_owner->im_html->htm_cm.cm_body_fg);
        }

        image->pixmap = pi->o_pixmap;

        // allocate fully transparent clipmask
        if (pi->o_transparency == IMAGE_TRANSPARENCY_BG) {
            info->options |= IMAGE_CLIPMASK;

            // compute amount of data required for this clipmask
            int i = image->width;

            // make it byte-aligned
            while ((i % 8))
                i++;

            // this many bytes on a row
            i /= 8;

            // size of clipmask
            int clipsize = i * image->height;

            // raw clipmask data
            pi->o_clip_data = new unsigned char[clipsize];
            memset(pi->o_clip_data, 0, clipsize);

            // fully transparent clipmask

            htmColor fg, bg;
            fg.pixel = 1;
            bg.pixel = 0;

            pi->o_clipmask = tk->tk_bitmap_from_data(image->width,
                image->height, pi->o_clip_data);
        }
        // destination clipmask
        image->clip = pi->o_clipmask;
        // temporary clipmask data
        info->clip  = pi->o_clip_data;

        // allocate image RGB values
        info->options |= IMAGE_RGB_SINGLE;
        info->reds   = new unsigned short[3 * pi->o_cmapsize];
        info->greens = info->reds   + pi->o_cmapsize;
        info->blues  = info->greens + pi->o_cmapsize;
        memset(info->reds, 0, 3 * pi->o_cmapsize * sizeof(short));

        // reset used colors array
        for (int i = 0; i < MAX_IMAGE_COLORS; i++) {
            pi->o_used[i] = 0;
            pi->o_xcolors[i] = 0;
        }

        pi->o_nused = 1;

        // update all copies of this image
        if (image->child)
            plc_owner->imageUpdateChildren(image);

        // create a working XImage for this plc
        if (pi->o_ximage == 0) {
            pi->o_ximage = tk->tk_new_image(image->width, image->height);
            if (pi->o_ximage == 0) {
                plc_status = PLC_ABORT;
                return;
            }
        }

        // redo screen layout as it will be incorrect by now
        if (need_redisplay) {
            plc_owner->im_html->htm_layout_needed = true;
            plc_owner->im_html->trySync();
        }
    }

    // always get used image dimensions
    int width  = image->width;
    int height = image->height;

    // Step 1: color usage

    // last known index of allocated colors
    int col_cnt = pi->o_nused;

    // store pixel indices
    {
        // last known processed data
        unsigned char *ptr = pi->o_data + pi->o_prev_pos;
        for (int i = pi->o_prev_pos; i < pi->o_data_pos; i++, ptr++) {
            if (pi->o_used[(int)*ptr] == 0) {
                pi->o_used[(int)*ptr] = col_cnt;
                col_cnt++;
            }
        }
        col_cnt--;
    }

    // Now go and fill the RGB arrays.  Only do this if the new chunk
    // of data contains new colors.

    if (pi->o_nused != col_cnt+1) {
        for (int i = 0; i < MAX_IMAGE_COLORS; i++)
            pixels[i] = 0;
        for (int i = 0; i < MAX_IMAGE_COLORS; i++) {

            if (pi->o_used[i] != 0) {
                int indx = pi->o_used[i] - 1;
                pixels[indx] = true;
                info->reds[indx]   = pi->o_cmap[i].red;
                info->greens[indx] = pi->o_cmap[i].green;
                info->blues[indx]  = pi->o_cmap[i].blue;
            }
        }
    }

    // Step 3: image pixelization.
    // Replace each pixel value in the decoded image data with the
    // indices in our own pixel array.

    // Do this before scaling, or JPEG_Finalize will screw colors up
    // (the data are recalculated), so scaled_data will be wrong.

    // last known processed data
    {
        unsigned char *ptr = pi->o_data + pi->o_prev_pos;
        for (int i = pi->o_prev_pos; i < pi->o_data_pos; i++) {
            *ptr = (unsigned char)(pi->o_used[(int)*ptr] - 1);
            ptr++;
        }
    }

    // Step 2a: image scaling (optional)

    if (pi->o_is_scaled) {

        // get ptr to scaled image data
        unsigned char *data = pi->o_scaled_data;
        unsigned char *img_data = pi->o_data;

        // get real image dimensions
        int src_w = pi->o_width;
        int src_h = pi->o_height;

        // initialize scaling
        unsigned char *elptr = data;

        // starting scanline index
        int ey = pi->o_sc_start/width;

        // scaling is done from top to bottom, left to right
        for ( ; ey < height; ey++, elptr += width) {
            // vertical pixel skip
            int iy = (src_h * ey) / height;
            unsigned char *epptr = elptr;
            unsigned char *ilptr = img_data + (iy * src_w);
            // don't overrun
            if (iy*src_w > pi->o_data_pos)
                break;
            for (int ex = 0; ex < width; ex++, epptr++) {
                // horizontal pixel skip
                int ix = (src_w * ex) / width;
                unsigned char *ipptr = ilptr + ix;
                *epptr = *ipptr;
            }
        }
        // update scaled data end position
        pi->o_sc_end = ey*width;
    }

    // Step 2b: clipmask creation (optional)
    // We just redo the entire image.

    if (pi->o_transparency == IMAGE_TRANSPARENCY_BG) {
        unsigned char *cptr = pi->o_clip_data;
        int bgp = pi->o_bg_pixel;
        if (bgp < 0 || bgp >= pi->o_nused)
            bgp = 0;
        int bg_pixel = pi->o_used[bgp] - 1;

        int lo, hi;
        unsigned char *ptr;
        if (pi->o_is_scaled) {
            ptr = pi->o_scaled_data;
            // last known processed scanline
            lo  = pi->o_sc_start/width;
            // maximum scanline on this pass
            hi  = pi->o_sc_end/width;
        }
        else {
            ptr = pi->o_data;
            hi = (pi->o_data_pos)/width;
            lo = (pi->o_prev_pos)/width;
        }

        // pick up were we left off. Saves prev_pos conditionals
        for (int i = 0; i < lo; i++) {
            for (int j = 0, bcnt = 0; j < width; j++) {
                if ((bcnt % 8) == 7 || j == (width-1))
                    cptr++;
                bcnt++;
                ptr++;
            }
        }

        // process next amount of data
        for (int i = lo; i < hi; i++) {

            for (int j = 0, bcnt = 0; j < width; j ++) {
                if (*ptr != bg_pixel)
                    *cptr |= plc_owner->im_bitmap_bits[(bcnt % 8)];
                else
                    *cptr &= ~plc_owner->im_bitmap_bits[(bcnt % 8)];
                if ((bcnt % 8) == 7 || j == (width-1))
                    cptr++;
                bcnt++;
                ptr++;
            }
        }
        // destroy existing bitmap
        if (pi->o_clipmask != 0)
            plc_owner->im_html->htm_tk->tk_release_bitmap(pi->o_clipmask);

        // create new one
        htmColor fg, bg;
        fg.pixel = 1;
        bg.pixel = 0;

        pi->o_clipmask = tk->tk_bitmap_from_data(width, height,
            pi->o_clip_data);

        // save it
        image->clip = pi->o_clipmask;

        // update child copies
        if (image->child)
            plc_owner->imageUpdateChildren(image);
    }

    // Step 4: color allocation.
    // Only allocate colors if the new chunk of data contains colors that
    // haven't been allocated yet.

    if (pi->o_nused != col_cnt+1) {
        int npixels = image->npixels;
        for (int i = 0; i < pi->o_cmapsize; i++) {
            if (pixels[i]) {
                tk->tk_get_pixels(info->reds + i, info->greens + i,
                    info->blues + i, 1, pi->o_xcolors + i);
                npixels++;
            }
        }

        // update used color count
        pi->o_nused = col_cnt+1;
        image->npixels = npixels;
    }

    // Step 5: XImage updating.
    // We have our colors, now do the ximage. If the image is being scaled,
    // use the pre-scaled image data, else use the real image data.

    int ypos;   // current scanline
    int nlines; // no of scanlines added on this pass
    if (pi->o_is_scaled) {
        tk->tk_fill_image(pi->o_ximage, pi->o_scaled_data, pi->o_xcolors,
            pi->o_sc_start, pi->o_sc_end);
        ypos = pi->o_sc_start/width;
        nlines = (pi->o_sc_end - pi->o_sc_start)/width;
    }
    else {
        tk->tk_fill_image(pi->o_ximage, pi->o_data, pi->o_xcolors,
            pi->o_prev_pos, pi->o_data_pos);
        ypos = pi->o_prev_pos/width;
        nlines = (pi->o_data_pos - pi->o_prev_pos)/width;
    }

    // XImage updated, make all positions aligned on completed
    // scanline boundaries.

    pi->o_prev_pos = (pi->o_data_pos/pi->o_width) * pi->o_width;

    // Step 6: destination updating.
    // XImage data processed, copy newly added scanlines to the
    // destination pixmap.

    // update pixmap
    tk->tk_set_draw_to_pixmap(pi->o_pixmap);
    tk->tk_draw_image(0, ypos, pi->o_ximage, 0, ypos, width, nlines);
    tk->tk_set_draw_to_pixmap(0);

    // Step 7: display update.
    // The first call updates the master image and the second call
    // updates any copies of this image.  Safety check:  only do it
    // when the image has an owner (i.e., it isn't the background
    // image).

    if (image->owner) {
        htmRect rect;

        // set bogus image height
        int tmp = image->height;
        image->height = nlines;
        plc_owner->im_html->drawImage(image->owner, ypos, false, &rect);
        image->height = tmp;
        if (rect.width && rect.height)
            tk->tk_refresh_area(rect.left(), rect.top(), rect.width,
                rect.height);

        // Walk the list of child images and update for copy.
        for (htmImage *timg = image->child; timg; timg = timg->child) {
            if (timg->owner) {
                rect.clear();
                // set bogus image height
                int tmpht = timg->height;
                timg->height = nlines;
                plc_owner->im_html->drawImage(timg->owner, ypos, false,
                    &rect);
                timg->height = tmpht;
                if (rect.width && rect.height)
                    tk->tk_refresh_area(rect.left(), rect.top(), rect.width,
                        rect.height);
            }
        }
    }
}


void
PLC::finalize()
{
    // obj_set will be false if this PLC was terminated during init phase
    if (!plc_object)
        return;

    PLCImage *pi = plc_object;
    htmImageInfo *info = pi->o_info;

    pi->finalize(info);

    // update all copies of this image
    htmImage *image  = pi->o_image;
    if (image) {
        if (image->owner) {
            image->owner->area.width = image->width;
            image->owner->area.height = image->height;
        }
        // no longer progressive
        image->options &= ~IMG_PROGRESSIVE;
        if (image->child != 0)
            plc_owner->imageUpdateChildren(image);
    }

    if (!image || !image->owner)
        // this redraws everything
        // Used to be called every time, looks bad.  Why redisplay
        // everything?  When is the image pointer 0?  (owner is 0 for
        // body bgimage)

        plc_owner->im_html->repaint(0, 0,
            plc_owner->im_html->htm_viewarea.width,
            plc_owner->im_html->htm_viewarea.height);
    else {
        // just redraw image areas
        plc_owner->im_html->repaint(image->owner->area.x,
            image->owner->area.y, image->width, image->height);
        for (htmImage *tmp = image->child; tmp; tmp = tmp->child) {
            if (tmp->owner)
                plc_owner->im_html->repaint(tmp->owner->area.x,
                    tmp->owner->area.y, tmp->width, tmp->height);
            else
printf("finalize: owner null\n");
        }
    }
}


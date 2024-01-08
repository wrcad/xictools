
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
#include "htm_parser.h"
#include "htm_plc.h"
#include "htm_tag.h"
#include "htm_format.h"
#include "htm_string.h"
#include "htm_callback.h"
#include <sys/param.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>

#include "htm_bitmaps.h"

#define DEFAULT_IMG_SUSPENDED   1   // images suspended icon
#define DEFAULT_IMG_UNSUPPORTED 2   // unsupported image icon

htmImageInfo::htmImageInfo(htmRawImageData *rd)
{
    url             = 0;
    data            = 0;
    clip            = 0;
    width           = 0;
    height          = 0;
    reds            = 0;
    greens          = 0;
    blues           = 0;
    bg              = 0;
    ncolors         = 0;
    options         = 0;

    type            = 0;
    depth           = 0;
    colorspace      = 0;
    transparency    = 0;
    swidth          = 0;
    sheight         = 0;
    scolors         = 0;

    alpha           = 0;
    fg_gamma        = 0;

    x               = 0;
    y               = 0;
    loop_count      = 0;
    dispose         = 0;
    timeout         = 0;
    nframes         = 0;
    frame           = 0;

    user_data       = 0;

    if (rd) {
        data        = rd->data;    // image data bits
        rd->data    = 0;           // reflect change of ownership
        type        = rd->type;    // image type
        width       = rd->width;   // image width
        height      = rd->height;  // image height
        swidth      = rd->width;   // original image width
        sheight     = rd->height;  // original image height
        bg          = rd->bg;      // save background pixel index
        colorspace  = rd->color_class;
        transparency = rd->bg != -1 ? IMAGE_TRANSPARENCY_BG : IMAGE_NONE;
    }
}


htmImageInfo::~htmImageInfo()
{
    delete [] url;
    delete [] data;
    if (Clipmask())
        delete [] clip;
    if (RGBSingle())
        delete [] reds;
    else {
        delete [] reds;
        delete [] greens;
        delete [] blues;
    }
    if (DelayedCreation())
        delete [] alpha;
}


htmImage::htmImage(htmImageManager *mgr)
{
    manager         = mgr;

    magic           = 0;
    url             = 0;
    html_image      = 0;
    pixmap          = 0;
    clip            = 0;
    options         = 0;
    width           = 0;
    height          = 0;
    npixels         = 0;
    pixel_map       = 0;

    swidth          = 0;
    sheight         = 0;
    alt             = 0;
    align           = HALIGN_NONE;
    map_type        = MAP_NONE;
    map_url         = 0;
    border          = 0;
    hspace          = 0;
    vspace          = 0;

    owner           = 0;
    child           = 0;
    next            = 0;

    frames          = 0;
    nframes         = 0;
    current_frame   = 0;
    current_loop    = 0;
    loop_count      = 0;
    proc_id         = 0;

    events          = 0;
}


htmImage::~htmImage()
{
    // remove the animation timeout proc
    if (proc_id)
        manager->im_html->htm_tk->tk_remove_timer(proc_id);

    // see if this is image has been copied
    if (!IsCopy() && !InfoFreed()) {

        // See if we are allowed to free the htmImageInfo structure
        // This is never the case for the internal images and can be
        // the case for external images if the user has set the
        // IMAGE_DEFERRED_FREE and/or IMAGE_IMMEDIATE_FREE bit.

        if (!IsInternal() && html_image &&
                (html_image->FreeNow() || html_image->FreeLater()))
            manager->freeImageInfo(html_image);

        // Free all pixmaps and allocated colors for this image.  Also
        // frees all htmImageFrame animation data.
        freePixmaps();

        delete [] url;
    }

    // free allocated strings
    delete [] alt;
    delete [] map_url;
}


// Allocate the colors for the given image and create an array of
// indexed pixel values (which is a sort of private colormap).
//
void
htmImage::makeColormap(htmImageInfo *info)
{
    if (pixel_map) {
        manager->im_html->htm_tk->tk_free_colors(pixel_map, npixels);
        delete [] pixel_map;
    }
    npixels = info->ncolors;
    pixel_map = new unsigned int[info->ncolors];
    manager->im_html->htm_tk->tk_get_pixels(info->reds,
        info->greens, info->blues, info->ncolors, pixel_map);
}


// Free all pixmaps and allocated colors for the given image.
//
void
htmImage::freePixmaps()
{
    // first free all previous pixmaps
    if (frames) {
        for (int i = 0; i < nframes; i++) {
            manager->im_html->htm_tk->tk_release_pixmap(frames[i].pixmap);
            manager->im_html->htm_tk->tk_release_bitmap(frames[i].clip);
            manager->im_html->htm_tk->tk_release_pixmap(frames[i].prev_state);
            if (frames[i].pixel_map) {
                manager->im_html->htm_tk->tk_free_colors(frames[i].pixel_map,
                    frames[i].npixels);
                delete [] frames[i].pixel_map;
            }
        }
        delete [] frames;
        frames = 0;
    }
    else {
        manager->im_html->htm_tk->tk_release_pixmap(pixmap);
        manager->im_html->htm_tk->tk_release_bitmap(clip);
        if (pixel_map) {
            manager->im_html->htm_tk->tk_free_colors(pixel_map, npixels);
            delete [] pixel_map;
        }
    }
    pixmap = 0;
    clip = 0;
    npixels = 0;
    pixel_map = 0;
}


void
htmImage::getSize(const char *attributes)
{
    if (!attributes) {
        if (!owner || !owner->object)
            return;
        attributes = owner->object->attributes;
    }

    // store real image dimensions
    width = html_image->width;
    height = html_image->height;

    // get specified width and height
    int wid, w_given = htmTagGetNumber(attributes, "width", -1);
    if (w_given <= 0)
        wid = 0;
    else
        wid = w_given;
    int hei, h_given = htmTagGetNumber(attributes, "height", -1);
    if (h_given <= 0)
        hei = 0;
    else
        hei = h_given;

    // Store specified width and height (if any)
    if (hei || wid) {
        if (options == IMG_ISINTERNAL) {
            // If the image is not found, use whatever dimensions were
            // given.  The internal images are never scaled.
            wid = w_given > 0 ? w_given : width;
            hei = h_given > 0 ? h_given : height;
        }
        else {
            if (w_given == 0)
                wid = width;
            else if (w_given < 0) {
                if (h_given > 0 && height)
                    wid = (width * h_given)/height;
                else
                    wid = width;
            }
            if (h_given == 0)
                hei = height;
            else if (h_given < 0) {
                if (w_given > 0 && width)
                    hei = (height * w_given)/width;
                else
                    hei = height;
            }
        }
    }
    if (hei && wid) {
        // we have got dimensions
        options |= IMG_HASDIMENSIONS;

        // store requested dimensions
        sheight = hei;
        swidth = wid;

        // sanity check:  if this is an internal image and the
        // specified dimensions are smaller than default image
        // dimensions, return the dimensions of the default image
        // instead or text layout will be really horrible....

        if (options == IMG_ISINTERNAL) {
            if (hei < height)
                hei = height;
            if (wid < width)
                wid = width;
        }

        // revert to original dimensions if we may not scale this image
        if (!html_image->Scale()) {
            sheight = height;
            swidth = width;
        }
    }
    else {
        swidth  = width;
        sheight = height;
    }
}


htmImageManager::htmImageManager(htmWidget *html)
{
    im_html                     = html;

    im_body_image               = 0;
    im_images                   = 0;
    im_delayed_creation         = false;

    // Imagemap resources
    im_image_maps               = 0;

    // Progressive Loader Context buffer and interval
    im_plc_buffer               = 0;
    im_num_plcs                 = 0;
    im_plc_suspended            = true;

    // Internal stuff for alpha channelled PNG images
    im_alpha_buffer             = 0;
}


htmImageManager::~htmImageManager()
{
    freeResources(true);
}


void
htmImageManager::initialize()
{
    // Image resources
    im_body_image       = 0;
    im_images           = 0;
    im_image_maps       = 0;
    im_delayed_creation = false;

    // PLC resources
    im_plc_buffer       = 0;
    im_num_plcs         = 0;

    // alpha channel stuff
    im_alpha_buffer     = 0;
}


void
htmImageManager::freeResources(bool free_img)
{
    freeImageMaps();
    im_image_maps = 0;

    if (free_img) {
        // Free all images (also clears xcc & alpha channel stuff)
        imageFreeAllImages();

        // must reset body_image as well since it also has been freed
        im_body_image    = 0;
        im_images        = 0;
        im_delayed_creation = false;  // no delayed image creation
        im_alpha_buffer  = 0;
    }
    else {
        // We need to orphan all images:  the formatter will be called
        // shortly after this function returns and as a result of that
        // the owner of each image will become invalid.  Not
        // orphanizing them would lead to a lot of image copying.
        // Info structures with the IMAGE_DELAYED_CREATION bit need to
        // propagate this info to their parent, or chances are that
        // alpha channeling will *not* be redone when required.  Must
        // not forget to set the global delayed_creation flag or
        // nothing will happen.

        for (htmImage *img = im_images; img; img = img->next) {
            img->owner = 0;  // safety
            img->options |= IMG_ORPHANED;
            if (!img->InfoFreed() && img->html_image->DelayedCreation()) {
                img->options |= IMG_DELAYED_CREATION;
                im_delayed_creation = true;
            }
        }
    }
}


#ifndef notused

namespace {
    // Convert the image data to another format via an external program,
    // such as one of the netpbm utilities.
    //
    bool
    convert(ImageBuffer *ib, const char *program)
    {
        if (!program || !*program)
            return (false);
        const char *path = getenv("TMPDIR");
        if (!path)
            path = "/tmp";
        int tflen = strlen(path) + strlen(program) + 32;
        char *tf = new char[tflen];
        snprintf(tf, tflen, "%s/moz%d-%s.tmp", path, getpid(), program);
        FILE *fp = fopen(tf, "w");
        if (!fp) {
            delete [] tf;
            return (false);
        }

        fwrite(ib->buffer, 1, ib->size, fp);
        fclose(fp);

        bool ret = false;
        int buflen = strlen(program) + strlen(tf) + 16;
        char *buf = new char[buflen];
        snprintf(buf, buflen, "%s %s 2>/dev/null", program, tf);
        fp = popen(buf, "r");
        delete [] buf;
        if (fp) {
            int c, cnt = 0;
            unsigned char *s0 = new unsigned char[1024];
            int sz = 1024;
            while ((c = getc(fp)) != EOF) {
                if (cnt == sz) {
                    unsigned char *tmp = new unsigned char[2*sz];
                    memcpy(tmp, s0, sz);
                    delete [] s0;
                    s0 = tmp;
                    sz += sz;
                }
                s0[cnt++] = c;
            }
            pclose(fp);
            unsigned char *tmp = new unsigned char[cnt];
            memcpy(tmp, s0, cnt);
            delete [] s0;
            delete [] ib->buffer;
            ib->buffer = tmp;
            ib->size = cnt;
            ib->curr_pos = ib->buffer;
            ret = true;
        }
        unlink(tf);
        delete [] tf;
        return (ret);
    }
}

#endif


// Default image loading procedure.
//
htmImageInfo*
htmImageManager::imageLoadProc(const char *file)
{
    // must be a valid filename
    if (!file)
        return (0);

    ImageBuffer *ib = imageFileToBuffer(file);
    if (!ib)
        return (0);
    // Assume all images have an initial depth of 8 bits.
    ib->depth = 8;

    ib->type = getImageType(ib);

    // sanity
    ib->rewind();

#ifndef notused
    // The ppm support is built in now.  Other formats might be
    // handled by the mechanism here.

    // Hack: support pnm image, thru conversion.
    //
    if (ib->type == IMAGE_UNKNOWN) {
        unsigned char *data = ib->buffer;
        if (data[0] == 'P' && data[1] >= '1' && data[1] <= '6' &&
                isspace(data[2])) {

            // A pnm file, convert to png using netpbm.
            if (convert(ib, "pnmtopng"))
                ib->type = IMAGE_PNG;
        }
    }
#endif

    if (ib->type == IMAGE_UNKNOWN || ib->type == IMAGE_ERROR) {
        delete ib;
        return (0);
    }

    htmImageInfo *image = 0;
    switch (ib->type) {
    case IMAGE_GIFANIM:
    case IMAGE_GIFANIMLOOP:
    case IMAGE_GZFANIM:
    case IMAGE_GZFANIMLOOP:
        image = readGifAnimation(ib);
        break;
    case IMAGE_FLG:
        // bypasses the readImage + defaultImage proc entirely
        image = readFLG(ib);
        break;
    case IMAGE_XPM:
    case IMAGE_XBM:
    case IMAGE_GIF:
    case IMAGE_GZF:
    case IMAGE_JPEG:
    case IMAGE_PNG:
    case IMAGE_TIFF:
    case IMAGE_PPM:
        {
            // load the given image
            htmRawImageData *img_data;
            if ((img_data = readImage(ib)) != 0) {

                // If image creation hasn't been delayed (it can be
                // delayed if for example, a png image with an alpha
                // channel or a tRNS chunk is being processed), use
                // the default image proc to create an htmImageInfo
                // structure for this image.

                if (img_data->delayed_creation == false)
                    image = imageDefaultProc(img_data, file);
                else
                    image = imageDelayedProc(img_data, ib);

                // No longer needed, destroy.  Don't free the members
                // of this structure as they are moved to the returned
                // image.
                delete img_data;
            }
        }
        break;
    default:
        break;
    }
    if (image) {
        image->type = ib->type;
        image->depth= ib->depth;
    }
    delete ib;
    return (image);
}


// Determine the type of a given image.
//
unsigned char
htmImageManager::imageGetType(const char *file)
{
    if (!file)
        return (IMAGE_ERROR);
    ImageBuffer *dp = imageFileToBuffer(file);
    if (!dp)
        return (IMAGE_ERROR);
    unsigned char ret_val = getImageType(dp);
    delete dp;
    return (ret_val);
}


// Replace the htmImageInfo structure with a new one.  Returns
// IMAGE_ALMOST if replacing this image requires a recomputation of
// the document layout, IMAGE_OK if not and some other value upon
// error.
//
ImageStatus
htmImageManager::imageReplace(htmImageInfo *image, htmImageInfo *new_image)
{
    // sanity
    if (image == 0 || new_image == 0) {
        im_html->warning("imageReplace", "Called with a NULL %s "
            "argument.", (image == 0 ? "image" : "new_image"));
        return (IMAGE_BAD);
    }

    // do we already have the body image?
    bool is_body_image = (im_body_image != 0);

    ImageStatus status;
    htmObjectTable *temp;
    if ((status = replaceOrUpdateImage(image, new_image, &temp)) != IMAGE_OK) {
        if (status == IMAGE_ALMOST)
            im_html->htm_layout_needed = true;
        return (status);
    }

    if (temp != 0) {
        htmRect win(im_html->htm_viewarea.x, im_html->htm_viewarea.y,
            im_html->htm_viewarea.width, im_html->htm_viewarea.height);
        if (win.intersect(temp->area)) {
            // put up the new image
            int xs = im_html->viewportX(temp->area.x);
            int ys = im_html->viewportY(temp->area.y);
            im_html->repaint(xs, ys, temp->area.width, temp->area.height);
        }
    }
    else if (!is_body_image && im_body_image != 0)
        // if we've replaced the body image, plug it in
        im_html->repaint(0, 0, im_html->htm_viewarea.width,
            im_html->htm_viewarea.height);

    return (IMAGE_OK);
}


// Update an image.  Return IMAGE_ALMOST if updating this image
// requires a recomputation of the document layout, IMAGE_OK if not
// and some other value upon error.
//
ImageStatus
htmImageManager::imageUpdate(htmImageInfo *image)
{
    if (image == 0) {
        im_html->warning("imageUpdate", "Called with a NULL image "
            "argument.");
        return (IMAGE_BAD);
    }

    // do we already have the body image?
    bool is_body_image = (im_body_image != 0);

    // return error code if failed
    htmObjectTable *temp;
    ImageStatus status;
    if ((status = replaceOrUpdateImage(image, 0, &temp)) != IMAGE_OK)
        return (status);

    if (temp != 0) {
        htmRect win(im_html->htm_viewarea.x, im_html->htm_viewarea.y,
            im_html->htm_viewarea.width, im_html->htm_viewarea.height);
        if (win.intersect(temp->area)) {
            // put up the new image
            int xs = im_html->viewportX(temp->area.x);
            int ys = im_html->viewportY(temp->area.y);
            im_html->repaint(xs, ys, temp->area.width, temp->area.height);
            im_html->paint(temp, temp->next);
        }
    }
    else if (!is_body_image && im_body_image != 0)
        // if we've updated the body image, plug it in
        im_html->repaint(0, 0, im_html->htm_viewarea.width,
            im_html->htm_viewarea.height);

    return (IMAGE_OK);
}


// Release all allocated images and associated structures.
//
void
htmImageManager::imageFreeAllImages()
{
    htmImage *image_list = im_images;
    im_images = 0;

    while (image_list) {
        htmImage *image = image_list->next;
        delete image_list;
        image_list = image;
    }

    // alpha channel stuff
    if (im_alpha_buffer) {
        if (im_alpha_buffer->ncolors)
            delete [] im_alpha_buffer->bg_cmap;
        delete im_alpha_buffer;
    }
    im_alpha_buffer = 0;
}


// Add the given imagemap to the widget.
//
void
htmImageManager::imageAddImageMap(const char *image_map)
{
    if (image_map == 0) {
        im_html->warning("imageAddImageMap", "%s passed.", "NULL imagemap");
        return;
    }

    // parse the imagemap
    htmObject *parsed_map = 0;
    {
        htmParser parser(im_html);
        parser.setSource(image_map);
        parsed_map = parser.parse(0);
    }
    if (!parsed_map)
        return;

    htmImageMap *imageMap = 0;
    for (htmObject *temp = parsed_map; temp; temp = temp->next) {
        switch (temp->id) {
        case HT_MAP:
            if (temp->is_end) {
                storeImagemap(imageMap);
                imageMap = 0;
            }
            else {
                char *chPtr = htmTagGetValue(temp->attributes, "name");
                if (chPtr) {
                    delete imageMap;
                    imageMap = new htmImageMap(chPtr);
                    delete [] chPtr;
                }
                else
                    im_html->warning("imageAddImagemap",
                        "Unnamed map, ignored (line %i in input).",
                        temp->line);
            }
            break;

        case HT_AREA:
            if (imageMap)
                addAreaToMap(imageMap, temp, 0);
            else
                im_html->warning("imageAddImagemap",
                    "<AREA> element outside <MAP>, ignored "
                    "(line %i in input).", temp->line);
            break;
        default:
            break;
        }
    }

    delete imageMap;
    htmObject::destroy(parsed_map);
}


// Free the given htmImageInfo structure.
//
void
htmImageManager::imageFreeImageInfo(htmImageInfo *info)
{
    if (info == 0) {
        im_html->warning("imageFreeImageInfo",
            "NULL htmImageInfo structure passed.");
        return;
    }
    freeImageInfo(info);
}


// Return the size of the given htmImageInfo structure.
//
int
htmImageManager::imageGetImageInfoSize(htmImageInfo *info)
{
    int size = 0;
    htmImageInfo *frame = info;
    while (frame != 0) {
        size += sizeof(htmImageInfo);
        size += frame->width*frame->height;     // raw image data

        // clipmask size. The clipmask is a bitmap of depth 1
        if (frame->clip) {
            int clipsize;
            clipsize = frame->width;
            // make it byte-aligned
            while ((clipsize % 8))
                clipsize++;
            // this many bytes on a row
            clipsize /= 8;
            // and this many rows
            clipsize *= frame->height;
            size += clipsize;
        }
        // reds, greens and blues
        size += 3*frame->ncolors*sizeof(unsigned);
        frame = frame->frame;   // next frame of this image (if any)
    }
    return (size);
}


// Determine the type of an image.  Return image type upon success,
// IMAGE_UNKNOWN on failure to determine the type.
//
unsigned char
htmImageManager::getImageType(ImageBuffer *ib)
{
    static unsigned char png_magic[8] = {137, 80, 78, 71, 13, 10, 26, 10};

    unsigned char magic[30];
    memcpy(magic, ib->buffer, 30);

    if (!(memcmp(magic, png_magic, 8)))
        return (IMAGE_PNG);
    if (magic[0] == 0xff && magic[1] == 0xd8 && magic[2] == 0xff)
        return (IMAGE_JPEG);
    if (magic[0] == 0x49 && magic[1] == 0x49 &&
            magic[2] == 0x2a && magic[3] == 0)
        return (IMAGE_TIFF);
    if (magic[0] == 0x4d && magic[1] == 0x4d &&
            magic[2] == 0 && magic[3] == 0x2a)
        return (IMAGE_TIFF);
    if (magic[0] == 'P' && magic[1] == '6' && isspace(magic[2]))
        return (IMAGE_PPM);
    if (!(strncmp((char*)magic, "GIF87a", 6)) ||
            !(strncmp((char*)magic, "GIF89a", 6)))
        return ((unsigned char)isGifAnimated(ib));
    if (!(strncmp((char*)magic, "GZF87a", 6)) ||
            !(strncmp((char*)magic, "GZF89a", 6))) {
        unsigned char ret_val = (unsigned char)isGifAnimated(ib);
        if (ret_val == IMAGE_GIF)
            ret_val = IMAGE_GZF;
        else if (ret_val == IMAGE_GIFANIM)
            ret_val = IMAGE_GZFANIM;
        else ret_val = IMAGE_GZFANIMLOOP;
        return (ret_val);
    }
    if (!(strncmp((char*)magic, "eX!flg", 6)))
        return (IMAGE_FLG);
    if (strstr((char*)magic, " XPM"))
        return (IMAGE_XPM);
    if (!(strncmp((char*)magic, "#define", 7)) ||
            (magic[0] == '/' && magic[1] == '*'))
        return (IMAGE_XBM);

    return (IMAGE_UNKNOWN);
}


// Load a file into a memory buffer.
//
ImageBuffer*
htmImageManager::imageFileToBuffer(const char *file)
{
    FILE *fp;
    if ((fp = fopen(file, "rb")) == 0) {
        perror(file);
        return (0);
    }

    fseek(fp, 0L, SEEK_END);
    int size = ftell(fp);
    if (size == 0) {
        fclose(fp);
        return (0);
    }
    rewind(fp);

    ImageBuffer *ib = new ImageBuffer(size);
    ib->file = lstring::copy(file);

    if ((fread(ib->buffer, ib->size, 1, fp)) != 1) {
        perror(file);
        fclose(fp);
        delete ib;
        return (0);
    }
    fclose(fp);

    return (ib);
}


// Create and fills an image structure.  The width and height returned
// are stored in the owning ObjectTable element.  The real image
// dimensions are stored in the htmImageInfo and htmImage structure.
// A resize of the image is only done if the real image dimensions
// differ from the requested dimensions and then only if the image
// isn't part of an animation or is an internal image.
//
htmImage*
htmImageManager::newImage(const char *attributes, unsigned int *width,
    unsigned int *height)
{
    // See if we have a source for this image. If not, just return
    char *src = htmTagGetValue(attributes, "src");
    if (!src)
        return (0);

    // get specified width and height
    int w_given = htmTagGetNumber(attributes, "width", -1);
    if (w_given <= 0)
        *width = 0;
    else
        *width = w_given;
    int h_given = htmTagGetNumber(attributes, "height", -1);
    if (h_given <= 0)
        *height = 0;
    else
        *height = h_given;

    unsigned char image_type = 0;
    htmImage *image;
    htmImageInfo *html_image;
    if (im_html->htm_images_enabled) {
        // We try to be a little bit careful with system resources.
        // HTML documents intend to have an increasing number of
        // images in them, so we try to reuse as much as possible.
        // lookForImage also updates width and height if width and/or
        // height is zero or one of them is zero and the other one
        // matches the real image dimension.  Images for which the
        // delayed creation bit has been set cannot be copied as they
        // will be different depending on their location on screen.

        if ((image = lookForImage(src, attributes, width, height,
                w_given, h_given)) != 0) {
            delete [] src;
            return (copyHTMLImage(image, attributes));
        }
        if (im_html->htm_if) {
            // only external loaders can enable delayed image loading
            html_image = im_html->htm_if->image_resolve(src);

            // If this image is to be loaded progressively we need
            // to have a get_data() function installed.  It's an error
            // if it's not!

            if (html_image && html_image->Progressive()) {

                // progressive images can't also be delayed
                if (html_image->Delayed())
                    html_image->options &= ~IMAGE_DELAYED;

                // no freeing of this ImageInfo
                html_image->options &= ~IMAGE_IMMEDIATE_FREE;
                html_image->options &= ~IMAGE_DEFERRED_FREE;

                // create a context for this image
                PLC *plc = PLCCreate(html_image, src);

                // Update/override PLC fields.  This can include
                // storing image-specific function pointers in the
                // obj_funcs array of the returned PLC.
                //
                // The user_data field must *always* be set or there
                // will be nothing to do.  It is used as the user_data
                // field in the stream argument to the get_data() and
                // end_data() calls.
                plc->plc_user_data = html_image->user_data;
            }

            // see if loading of this image has been suspended
            if (html_image && html_image->Delayed()) {

                // get delayed image information
                htmImageInfo *delayed = defaultImage(src,
                    DEFAULT_IMG_SUSPENDED, false, w_given, h_given,
                    &image_type);

                // substitute appropriate data fields
                html_image->data    = delayed->data;
                html_image->clip    = delayed->clip;
                html_image->ncolors = delayed->ncolors;
                html_image->reds    = delayed->reds;
                html_image->greens  = delayed->greens;
                html_image->blues   = delayed->blues;
                html_image->bg      = delayed->bg;
                html_image->depth   = delayed->depth;
                html_image->ncolors = html_image->scolors = delayed->ncolors;
                html_image->colorspace   = delayed->colorspace;
                html_image->transparency = delayed->colorspace;

                // always use default image dimensions or bad things happen
                html_image->width   = delayed->width;
                html_image->height  = delayed->height;
                html_image->swidth  = delayed->swidth;
                html_image->sheight = delayed->sheight;

                // and we don't know the image type (yet)
                html_image->type = IMAGE_UNKNOWN;

                // need to append options of the default image as well
                html_image->options |= delayed->options;
            }
        }
        else
            html_image = imageLoadProc(src);

        // This widget contains images which are to be created when
        // they are needed.

        if (html_image && html_image->DelayedCreation())
            im_delayed_creation = true;
    }
    else {
        // see if we have this image already loaded
        if ((image = lookForImage(src, attributes, width, height,
                w_given, h_given)) != 0 && image->IsInternal()) {
            delete [] src;
            return (copyHTMLImage(image, attributes));
        }

        // we haven't got it, get a default image
        html_image = defaultImage(src, DEFAULT_IMG_SUSPENDED, false,
            w_given, h_given, &image_type);
    }

    if (html_image == 0) {
        // imageProc failed, get default image
        html_image = defaultImage(src, DEFAULT_IMG_UNSUPPORTED, false,
            w_given, h_given, &image_type);

        // This is *definitly* an error
        if (html_image == 0) {
            im_html->fatalError(
                "A fatal error occured in the default image loading "
                "procedure for image\n    %s.", src);
        }
    }

    // allocate and initialize a new image
    image = new htmImage(this);
    image->magic = HTML_IMAGE_MAGIC;
    image->html_image = html_image;
    image->options = image_type;

    // store original url in private image info
    image->url = src;

    // Check if this image is an animation
    if (html_image->nframes > 1)
        image->options |= IMG_ISANIM;

    // check if we have delayed creation
    if (html_image->DelayedCreation())
        image->options |= IMG_DELAYED_CREATION;

    image->getSize(attributes);
    *width = image->swidth;
    *height = image->sheight;

    // Go and create the image.
    // We have four options here:
    //   1. the image can be an animation
    //   2. the image can be a plain image;
    //   3. the image can have the delayed creation flag set.
    //   4. we are instructed to progressively load this image.

    if (html_image->Progressive()) {
        if (image->HasDimensions()) {
            *width  = image->swidth;
            *height = image->sheight;
        }
        else
            // we must set some return dimensions
            *width = *height = 64;

        // plc has already been created
    }
    else if (image->IsAnim())
        makeAnimation(image, *width, *height);
    else {
        if (!(image->DelayedCreation())) {
            // infoToPixmap will scale the image if required

            image->makeColormap(html_image);
            htmPixmap *clip;
            htmPixmap *pixmap = infoToPixmap(image, html_image,
                *width, *height, image->pixel_map, &clip);
            if (!pixmap) {
                delete image;
                return (0);
            }
            image->pixmap = pixmap;
            image->clip = clip;
        }
        // no additional processing for delayed image creation
    }

    // pick up remaining image attributes
    getImageAttributes(image, attributes);

    // store in the image list
    addImageToList(image);

    // check if we may free the htmImageInfo right now
    if (html_image->FreeNow() && !image->IsCopy()) {
        freeImageInfo(html_image);
        image->html_image = 0;
    }
    return (image);
}


// Load the body image specified by the given url.
//
void
htmImageManager::loadBodyImage(const char *url)
{
    if (!url) {
        im_body_image = 0;
        return;
    }

    // kludge so newImage recognizes it
    int len = strlen(url) + 7;
    char *buf = new char[len];
    snprintf(buf, len, "src=\"%s\"", url);

    // load it
    htmImage *body_image;
    unsigned int width, height;
    if ((body_image = newImage(buf, &width, &height)) != 0)
        processBodyImage(body_image, width, height);

    delete [] buf;
}


// Update all children for the given parent image.  This function
// adjusts the htmImageInfo and pixmap fields of an image child.  It
// is called whenever htmUpdateImage or htmReplaceImage is called.  It
// is also called from within plc.c at PLC initialization and at PLC
// completion.
//
void
htmImageManager::imageUpdateChildren(htmImage *image)
{
    for (htmImage *tmp = image->child; tmp; tmp = tmp->child) {
        // Update all necessary fields, including width and height of
        // the word represented by this image!

        tmp->pixmap        = image->pixmap;
        tmp->clip          = image->clip;
        tmp->frames        = image->frames;
        tmp->nframes       = image->nframes;
        tmp->current_frame = image->current_frame;
        tmp->current_loop  = image->current_loop;
        tmp->loop_count    = image->loop_count;
        tmp->options       = image->options;
        tmp->html_image    = image->html_image;
        tmp->width         = image->width;
        tmp->height        = image->height;
        tmp->swidth        = image->swidth;
        tmp->sheight       = image->sheight;
        // and this is still a copy
        tmp->options |= IMG_ISCOPY;

        // update word data for this image
        if (tmp->owner && tmp->owner->words &&
                tmp->owner->words[0].image == tmp) {
            tmp->owner->words[0].area.width = tmp->width;
            tmp->owner->words[0].area.height = tmp->height;
        }
    }
}


// Walk the list of images and rereads them in order to process any
// alpha channel information.  This function unsets the
// delayed_creation flag if the current document does not have a body
// image (when alpha channel info is processed against a solid
// background color, no reprocessing is required since the position if
// the image does not matter).  It's the caller's responsibility to
// check whether or not the delayed_creation flag has been set.
//
void
htmImageManager::imageCheckDelayedCreation()
{
    // if we have a body image, but it's not yet available, do nothing
    if (im_body_image &&
            !im_body_image->DelayedCreation() &&
            !htmImageInfo::BodyImageLoaded(im_body_image->html_image))
        return;

    // First check if the body image is present.  If it is and we
    // should process it now, the alphaChannel info must be
    // initialized for background image alpha processing.

    bool for_body_image = false;
    if (im_body_image &&
            im_body_image->DelayedCreation())
        for_body_image = true;

    // Always (re-)initialize the AlphaChannel buffer.  If the body
    // image is an alpha-channeled image we must use the current
    // background color for it, and for all subsequent images we need
    // to use the colormap of the background image.

    initAlphaChannels(for_body_image);

    // now walk all images
    for (htmImage *tmp = im_images; tmp; tmp = tmp->next) {
        if (tmp->DelayedCreation()) {
            doAlphaChannel(tmp);

            // If this was the body image, we need to re-initialize
            // the alphaChannel info, or images will use the
            // background color instead of the new background image.
            // doAlphaChannel will reset the delayed creation bit for
            // alpha channelled images automatically, so we don't need
            // to do this here.

            // Note:
            // You might wonder why we don't reprocess the images when
            // we find out the background image has an alpha channel.
            // We can get away with this 'cause the background image
            // is *always* the first element in the list of images.

            if (tmp->IsBackground() && for_body_image)
                initAlphaChannels((for_body_image = false));
        }
    }
    // If we don't have a body image, all images have been processed
    // and do not need to be reprocessed when the document resizes.

    if (im_body_image == 0)
        im_delayed_creation = false;
}


// Create a series of image that form an animation.
//
void
htmImageManager::makeAnimation(htmImage *image, unsigned int width,
    unsigned int height)
{
    int nframes = image->html_image->nframes;
    image->options |= IMG_ISANIM;
    image->nframes = nframes;
    image->frames = new htmImageFrame[nframes];
    image->current_frame = 0;

    // Animations can also be scaled.  To do this, we scale each frame
    // with the scale factor that needs to be applied to the logical
    // screen area.  The logical screen area is given by the width and
    // height of the first frame.

    htmImageInfo *frame = image->html_image;
    float height_p = (float)(height/(float)frame->height);
    float width_p  = (float)(width/(float)frame->width);

    // global colormap
    image->makeColormap(frame);
    image->frames[0].pixel_map = image->pixel_map;
    image->pixel_map = 0;
    image->frames[0].npixels = image->npixels;
    image->npixels = 0;

    int i = 0;
    unsigned int w = width, h = height;
    while (frame && i < nframes) {
        // use current frame dimensions instead of global image dimensions
        if (!(frame->options & IMAGE_FRAME_IGNORE)) {
            w = (int)(width_p*frame->width + 0.5);
            h = (int)(height_p*frame->height + 0.5);

            // If the dimensions of this frame differ from the logical
            // screen dimensions or a frame has a disposal method
            // other than none, we run the animation on an internal
            // pixmap and blit this pixmap to screen when all required
            // processing for the current frame has been done.  This
            // is an enormous performance enhancment since we only
            // need to do one screen update.  Animations of which each
            // frame has the same size and a disposal method of none
            // are blit to screen directly as they require no
            // processing whatsoever.

            // The "simple" way does not redraw the image correctly on
            // exposure, since it only shows the current frame, which
            // may not show the complete image.  The rendering through
            // a common pixmap keeps the history so exposes correctly

            /* keep me!
            if ((w != width || h != height ||
                frame->dispose != IMAGE_DISPOSE_NONE) &&
                !ImageHasState(image))
             */
                image->options |= IMG_HASSTATE;

            // Only use local colormap if we've got one.
            if (i && frame->ncolors) {
                image->makeColormap(frame);
                image->frames[i].pixel_map = image->pixel_map;
                image->pixel_map = 0;
                image->frames[i].npixels = image->npixels;
                image->npixels = 0;
            }

            htmPixmap *clip;
            htmPixmap *pixmap = infoToPixmap(image, frame, w, h,
                i && frame->ncolors ?
                    image->frames[i].pixel_map : image->frames[0].pixel_map,
                &clip);
            if (!pixmap) {
                image->html_image->nframes = i;
                return;
            }
            image->frames[i].pixmap = pixmap;
            image->frames[i].clip = clip;
        }
        else
            image->frames[i].pixmap = 0;

        image->frames[i].area.x = (int)(width_p*(float)frame->x);
        image->frames[i].area.y = (int)(height_p*(float)frame->y);
        image->frames[i].area.width = w;
        image->frames[i].area.height = h;
        image->frames[i].dispose = frame->dispose;
        image->npixels = 0;

        // adjust animation timeout if it's too small
        image->frames[i].timeout = (frame->timeout ? frame->timeout : 50);

        // move to next animation frame
        frame = frame->frame;
        i++;
    }

    // Fallback images when loop_count is used or animations are frozen.
    image->pixmap = image->frames[0].pixmap;
    image->clip = image->frames[0].clip;

    image->loop_count = image->html_image->loop_count;
    image->current_loop = 0;
    image->current_frame = 0;
    // this will initialize looping in DrawImage
    image->options |= IMG_FRAMEREFRESH;
}


// Create a pixmap from the given image data.
//
htmPixmap *
htmImageManager::infoToPixmap(htmImage *image, htmImageInfo *info,
    unsigned int width, unsigned int height, unsigned int *colormap,
    htmBitmap **clip)
{
    *clip = 0;

    // external images are only scaled if the dimensions specified in
    // the <IMG> attributes differ from the real image dimensions.
    // The IMAGE_ALLOW_SCALE bit must also be set (true by default).

    if (!(image->IsInternal()) && info->Scale() &&
            (height != info->height || width != info->width)) {
        scaleImage(info, width, height);
        image->height = height;
        image->width = width;
    }

    htmPixmap *pixmap = im_html->htm_tk->tk_pixmap_from_info(
        image, info, colormap);
    if (info->Clipmask())
        *clip = im_html->htm_tk->tk_bitmap_from_data(info->width, info->height,
            info->clip);

    return (pixmap);
}


// Update an image with new image data.  Returns IMAGE_OK if no layout
// recomputation is required, IMAGE_ALMOST if it is, an error code
// otherwise.
//
ImageStatus
htmImageManager::replaceOrUpdateImage(htmImageInfo *info,
    htmImageInfo *new_info, htmObjectTable **elePtr)
{
    *elePtr = 0;

    // given image dimensions
    int img_width  = (new_info ? new_info->width  : info->width);
    int img_height = (new_info ? new_info->height : info->height);

    bool do_return = false;
    for (htmImage *image = im_images; image; image = image->next) {
        if (image->html_image == info) {
            // get/set image dimensions.  If dimensions have been
            // specified on the original <IMG> spec, use those, else
            // use the real image dimensions.

            if (image->HasDimensions() && info->Scale()) {
                image->width  = image->swidth;
                image->height = image->sheight;
                do_return = true;
            }
            else {
                // image dimensions haven't changed: a true image reload
                if (image->width == img_width && image->height == img_height)
                    do_return = true;
                image->swidth  = image->width  = img_width;
                image->sheight = image->height = img_height;
            }

            // if this is the background image, update body_image ptr
            if (image->IsBackground())
                im_body_image = image;
            else if (image->owner && image->owner->words &&
                    image->owner->words[0].image == image) {
                // This image normally should have an owning object,
                // so update the worddata for this image as well.
                image->owner->words[0].area.width = image->width;
                image->owner->words[0].area.height = image->height;
            }

            // if this image is a copy and it's no longer delayed,
            // just return.

            if (image->IsCopy() && !(info->Delayed())) {
                if (do_return && image->owner) {
                    *elePtr = image->owner;
                    return (IMAGE_OK);
                }
                return (IMAGE_ALMOST);
            }

            // check if we are replacing this image
            if (new_info != 0) {
                // release previous info
                if (!image->IsInternal() && image->html_image->FreeLater())
                    freeImageInfo(image->html_image);
                image->html_image = new_info;
            }

            // free all pixmaps and allocated colors for this image
            image->freePixmaps();

            // no longer an internal image, nor a copy
            image->options  &= ~(IMG_ISINTERNAL) & ~(IMG_ISCOPY);

            // just to be sure
            image->html_image->options &= ~(IMAGE_DELAYED);
            image->html_image->options &= ~(IMAGE_SHARED_DATA);

            // if this image has delayed creation, don't let it be freed
            if (image->html_image->DelayedCreation()) {
                // may not be freed
                image->html_image->options &= ~(IMAGE_IMMEDIATE_FREE);
                image->html_image->options &= ~(IMAGE_DEFERRED_FREE);
                image->options |= IMG_DELAYED_CREATION;
                im_delayed_creation = true;
            }

            image->getSize(0);
            if (image->HasDimensions() && info->Scale()) {
                image->width  = image->swidth;
                image->height = image->sheight;
            }
            if (image->owner && image->owner->words &&
                    image->owner->words[0].image == image) {
                // This image normally should have an owning object,
                // so update the worddata for this image as well.
                image->owner->words[0].area.width = image->width;
                image->owner->words[0].area.height = image->height;
            }

            // check if we are to create an animation or a plain image
            if (image->html_image->nframes > 1)
                makeAnimation(image, image->width, image->height);
            else {
                if (!(image->DelayedCreation())) {
                    // Create the new pixmap.  infoToPixmap will
                    // scale the image if the real image dimensions
                    // differ from the specified ones.  This is a
                    // serious error if it fails.

                    image->makeColormap(image->html_image);
                    htmPixmap *clip;
                    htmPixmap *pixmap = infoToPixmap(image, image->html_image,
                        image->width, image->height, image->pixel_map, &clip);
                    if (!pixmap)
                        return (IMAGE_ERROR_STATUS);

                    // store it
                    image->pixmap = pixmap;
                    image->clip = clip;

                    // background image processing
                    if (image->IsBackground())
                        processBodyImage(image, image->width, image->height);
                }
                else {
                    // If the image dimensions are known, we can do
                    // alpha processing right away.  Otherwise, we
                    // need to set the global delayed_creation flag
                    // which will trigger alpha processing when the
                    // user calls htmRedisplay after he has flushed
                    // all image.

                    if (do_return || image->IsBackground()) {
                        initAlphaChannels(image->IsBackground());
                        doAlphaChannel(image);
                    }
                    else
                        im_delayed_creation = true;
                }
            }

            // update all copies of this image
            imageUpdateChildren(image);

            // return owner if image dimensions haven't changed
            if (do_return && image->owner) {
                *elePtr = image->owner;
                return (IMAGE_OK);
            }
            return (IMAGE_ALMOST);
        }
    }
    im_html->warning("replaceOrUpdateImage",
        "Can't update image %s.\n    Provided htmImageInfo not bound to any "
        "image.",
        info->url);
    return (IMAGE_UNKNOWN_STATUS);
}


// Free the given htmImageInfo structure, if it is not shared.
//
void
htmImageManager::freeImageInfo(htmImageInfo *info)
{
    // Always set the info free bit, even if we are being called from
    // application code.

    for (htmImage *image = im_images; image; image = image->next) {
        if (image->html_image == info)
            image->options |= IMG_INFOFREED;
    }

    // also free all animation frames for this image
    while (info != 0) {
        htmImageInfo *tmp = info->frame;

        if (!info->Shared())
            delete info;
        else {
            delete [] info->url;
            info->url = 0;

            // This will be true only for internal images of which the
            // data may never be freed.  Internal images are created
            // only once and are used by every document that needs to
            // display an internal image.  Fix thanks to Dick Porter
            // <dick@cymru.net>.
        }
        info = tmp;
    }
}


// Release the given image and update the internal list of images.
//
void
htmImageManager::releaseImage(htmImage *image)
{
    if (!image) {
        im_html->warning("releaseImage",
            "Internal Error: attempt to release a non-existing image.");
        return;
    }

    // first remove this image from the list of images
    if (image == im_images)
        im_images = image->next;
    else {
        htmImage *tmp = im_images;

        // walk the list until we find it
        if (tmp)
            for ( ; tmp->next && tmp->next != image; tmp = tmp->next) ;

        // not found
        if (!tmp || !tmp->next) {
            im_html->warning("releaseImage",
                "Internal Error: image %s not found in allocated image list.",
                image->url);
            return;
        }

        // Disconnect this image from the list.  tmp is the image just
        // before the image to release.
        tmp->next = image->next;
    }

    // now release it
    delete image;
}


// Image loading driver function.  X11 bitmaps, pixmaps and gif are
// supported by default.  If the system we are running on has the
// jpeglib, loading of jpeg files is also supported.  Same holds for
// png.
//
htmRawImageData*
htmImageManager::readImage(ImageBuffer *ib)
{
    htmRawImageData *img_data = 0;
    ib->rewind();
    switch (ib->type) {
    case IMAGE_GIF:
    case IMAGE_GZF:     // our compatible gif format
        img_data = readGIF(ib);
        break;
    case IMAGE_XBM:
        img_data = readBitmap(ib);
        break;
    case IMAGE_XPM:
        img_data = readXPM(ib);
        break;
    case IMAGE_JPEG:
        img_data = readJPEG(ib);
        break;
    case IMAGE_PNG:
        img_data = readPNG(ib);
        break;
    case IMAGE_TIFF:
        img_data = readTIFF(ib);
        break;
    case IMAGE_PPM:
        img_data = readPPM(ib);
        break;
    case IMAGE_FLG:     // treated wholy differently
        break;
    case IMAGE_UNKNOWN:
    default:
        break;
    }
    return (img_data);
}


// Clip the given image to the given dimensions.
//
void
htmImageManager::clipImage(htmImageInfo *image, unsigned int new_w,
    unsigned int new_h)
{
    // allocate memory for clipped image data
    unsigned char *data = new unsigned char[new_w*new_h];
    unsigned char *dataPtr =  data;

    // pick up current image data
    unsigned char *imgPtr = image->data;

    // clipping is done from top to bottom, left to right
    for (int y = 0; y < (int)new_h; y++) {
        int x;
        for (x = 0; x < (int)new_w; x++)
            *(dataPtr++) = *(imgPtr++);
        // skip anything outside image width
        while (x < (int)image->width)
            imgPtr++;
    }

    // free previous (unclipped) image data
    delete [] image->data;

    // new clipped image data
    image->data = data;

    // new image dimensions
    image->width = new_w;
    image->height= new_h;
}


// Scale the given image to the given dimensions.
//
void
htmImageManager::scaleImage(htmImageInfo *image, unsigned int new_w,
    unsigned int new_h)
{
    // allocate memory for scaled image data
    unsigned char *data = new unsigned char[new_w*new_h];

    // pick up current image data
    unsigned char *img_data = image->data;

    // current image dimensions
    int src_w = image->width;
    int src_h = image->height;

    // initialize scaling
    unsigned char *elptr = data;
    unsigned char *epptr = data;

    // scaling is done from top to bottom, left to right
    for (int ey = 0 ; ey < (int)new_h; ey++, elptr += new_w) {
        // vertical pixel skip
        int iy = (src_h * ey) / new_h;
        epptr = elptr;
        unsigned char *ilptr = img_data + (iy * src_w);
        for (int ex = 0; ex < (int)new_w; ex++, epptr++) {
            // horizontal pixel skip
            int ix = (src_w * ex) / new_w;
            unsigned char *ipptr = ilptr + ix;
            *epptr = *ipptr;
        }
    }
    // free previous (unscaled) image data
    delete [] img_data;

    // scaled image data
    image->data = data;

    // Create new clipmask data if we have one instead of resizing the
    // clipmask itself.

    if (image->Clipmask()) {

        int clipsize = new_w;
        // make it byte-aligned
        while ((clipsize % 8))
            clipsize++;

        // this many bytes on a row
        clipsize /= 8;

        // size of clipmask
        clipsize *= new_h;

        // resize clipmask data
        delete [] image->clip;
        image->clip = new unsigned char[clipsize];
        memset(image->clip, 0, clipsize);
        unsigned char *ilptr = image->clip;
        elptr = image->data;

        // recreate bitmap
        for (int iy = 0; iy < (int)new_h; iy++) {
            for (int ix = 0, bcnt = 0; ix < (int)new_w; ix++) {
                if (*elptr != image->bg)
                    *ilptr += im_bitmap_bits[(bcnt % 8)];
                if ((bcnt % 8) == 7 || ix == (int)(new_w - 1))
                    ilptr++;
                bcnt++;
                elptr++;
            }
        }
    }

    // new image width and height
    image->width = new_w;
    image->height= new_h;
}


// Return maximum number of colors allowed for current display.
//
int
htmImageManager::getMaxColors(int max_colors)
{
    unsigned int planes = im_html->htm_tk->tk_visual_depth();
    if (planes > 8)
        planes = 8;
    int ncolors = 1 << planes;
    if (max_colors > ncolors) {
        im_html->warning("getMaxColors",
            "Bad value for maxImageColors: %i colors selected while "
            "display only\n    supports %i colors. Reset to %i.",
            max_colors, ncolors, ncolors);
        return (ncolors);
    }
    else if (!max_colors)
        return (ncolors);
    return (max_colors);
}


// Retrieve all possible attribute specifications for the IMG element.
//
void
htmImageManager::getImageAttributes(htmImage *image, const char *attributes)
{
    if ((image->alt = htmTagGetValue(attributes, "alt")) != 0) {
        // handle escape sequences in the alt text
        char *t = image->alt;
        image->alt = expandEscapes(t, false);
        delete [] t;
    }
    else {
        // if we have a real URL or some path to this image, strip it off
        if (strstr(image->url, "/")) {
            int i;
            for (i = strlen(image->url) - 1; i > 0 && image->url[i] != '/';
                i--) ;
            image->alt = lstring::copy(&image->url[i+1]);
        }
        else
            // fix 09/01/97-01, kdh
            image->alt = lstring::copy(image->url);
    }

    image->border = htmTagGetNumber(attributes, "border", 0);
    image->hspace = htmTagGetNumber(attributes, "hspace", 0);
    image->vspace = htmTagGetNumber(attributes, "vspace", 0);
    image->align  = htmGetImageAlignment(attributes);

    // Imagemap stuff.  First check if we have a usemap spec.  If so,
    // it's automatically a client-side imagemap.  If no usemap is
    // given but the ISMAP attribute is set this is a server-side
    // imagemap.

    if ((image->map_url = htmTagGetValue(attributes, "usemap")) != 0)
        image->map_type = MAP_CLIENT;
    else if (htmTagCheck(attributes, "ismap"))
        image->map_type = MAP_SERVER;
    else
        image->map_type = MAP_NONE;
}


// Link the most wasteful members of src to a new image.  dest has
// it's is_copy member set to true which tells the widget not to free
// the html_image, xcc and pixmap members of the copy.
//
htmImage*
htmImageManager::copyImage(htmImage *src, const char *attributes)
{
    htmImage *dest = new htmImage(this);
    dest->magic = HTML_IMAGE_MAGIC;

    // This isn't exactly a copy, but almost a full referential
    dest->url           = src->url;
    dest->height        = src->height;
    dest->width         = src->width;
    dest->sheight       = src->sheight;
    dest->swidth        = src->swidth;
    dest->pixmap        = src->pixmap;
    dest->clip          = src->clip;
    dest->frames        = src->frames;
    dest->nframes       = src->nframes;
    dest->current_frame = src->current_frame;
    dest->current_loop  = src->current_loop;
    dest->loop_count    = src->loop_count;
    dest->options       = src->options;
    dest->html_image    = src->html_image;

    // unique for each image
    getImageAttributes(dest, attributes);

    // This tells the widget which members to free and which not
    dest->options |= IMG_ISCOPY;

    return (dest);
}


// See if the image at the given url is in the current list of images.
// If a match is found, the dimensions of that image are checked
// against possible specifications.  When no specifications are given,
// we can just return the image found so a copy can be made.  If
// however specifications are given, we need an exact match since I
// don't know a way to resize pixmaps.
//
htmImage*
htmImageManager::lookForImage(const char *url, const char*,
    unsigned int *width, unsigned int *height, int w_given, int h_given)
{
    for (htmImage *image = im_images; image; image = image->next) {
        // images that are already a copy can't be copied
        if (image->url && !image->IsCopy() && !(strcmp(image->url, url))) {

            // All possible combinations if dimension specification.
            // We always need to compare against the specified
            // dimensions:  even though the image may be the same,
            // different dimensions require it to be scaled.  We want
            // to have the original data or we would have a tremendous
            // loss of information.

            int w = *width;
            int h = *height;
            if (!image->html_image->Scale() ||
                    ((!h && !w) && image->height == image->sheight
                        && image->width == image->swidth) ||
                    (h == image->sheight && w == image->swidth) ||
                    (!h && w == image->swidth && ((h_given == 0 &&
                        image->height == image->sheight) || (h_given != 0 &&
                        image->height != image->sheight))) ||
                    (!w && h == image->sheight && ((w_given == 0 &&
                        image->width == image->swidth) || (w_given != 0 &&
                        image->width != image->swidth)))) {
                *height = image->sheight;
                *width = image->swidth;
                return (image);
            }
        }
    }
    return (0);
}


// Add the given image to the list of images.
//
void
htmImageManager::addImageToList(htmImage *image)
{
    // head of the list
    if (!im_images) {
        im_images = image;
        return;
    }

    // walk to the one but last image in the list and insert the image
    htmImage *tmp;
    for (tmp = im_images; tmp && tmp->next; tmp = tmp->next) ;
    tmp->next = image;
}


// Create an internal image.
//
htmImageInfo*
htmImageManager::defaultImage(const char *src, int default_image_type,
    bool call_for_free, int width, int height, unsigned char *image_type)
{
    static htmImageInfo *suspended, *unsupported;
    const char * const *xpm_data = 0;

    // If the given size is smaller than the XPM image size, return a
    // 1-pixel transparent image.

    if ((width > 0 && width < 32) || (height > 0 && height < 32)) {
        xpm_data = onepix_xpm;
        htmRawImageData *data = createXpmFromData(xpm_data, src);
        htmImageInfo *info = imageDefaultProc(data, src);
        info->type = IMAGE_XPM;
        info->options &= ~IMAGE_DEFERRED_FREE;
        info->options |= IMAGE_SHARED_DATA;
        info->depth = 4; // 15 colors
        return (info);
    }

    *image_type = IMG_ISINTERNAL;
    if (default_image_type == DEFAULT_IMG_SUSPENDED) {
        if (call_for_free || suspended != 0)
            return (suspended);
        else
            xpm_data = img_xpm;
    }
    else if (default_image_type == DEFAULT_IMG_UNSUPPORTED) {
        if (call_for_free || unsupported != 0)
            return (unsupported);
        else
            xpm_data = noimg_xpm;
    }
    else
        im_html->fatalError(
        "Internal Error: default image requested but don't know the type!");

    htmRawImageData *data =
        createXpmFromData(xpm_data, src);

    // The default images must use a clipmask:  not doing this would
    // use the current background for the transparent pixel in every
    // document.  And since each document doesn't have the same
    // background this would render the wrong transparent color when
    // the document background color changes.  Also, these images may
    // never be freed.

    if (default_image_type == DEFAULT_IMG_SUSPENDED) {
        suspended = imageDefaultProc(data, src);
        suspended->type = IMAGE_XPM;
        suspended->options &= ~IMAGE_DEFERRED_FREE;
        suspended->options |= IMAGE_SHARED_DATA;
        suspended->depth = 4;   /* 15 colors */
        return (suspended);
    }
    else {
        unsupported = imageDefaultProc(data, src);
        unsupported->type = IMAGE_XPM;
        unsupported->options &= ~IMAGE_DEFERRED_FREE;
        unsupported->options |= IMAGE_SHARED_DATA;
        unsupported->depth = 4; /* 15 colors */
        return (unsupported);
    }
}


// Default image loading procedure.
//
htmImageInfo*
htmImageManager::imageDefaultProc(htmRawImageData *img_data, const char *url)
{
    // default htmImageInfo flags
    int options = IMAGE_DEFERRED_FREE|IMAGE_RGB_SINGLE|IMAGE_ALLOW_SCALE;

    int max_colors = im_html->htm_max_image_colors;

    // raw data size
    int size = img_data->width * img_data->height;

    // If we have a background pixel, it means we have a transparent
    // image.  To make it really transparent, pick up the background
    // pixel and corresponding RGB values so it can be substituted in
    // the image.

    bool do_clip = false;
    if (img_data->bg >= 0) {
        options |= IMAGE_CLIPMASK;
        do_clip = true;
    }
    // we've got ourselves a clipmask
    int clipsize = 0;
    unsigned char *clip = 0;
    if (do_clip) {
        int i = img_data->width;

        // make it byte-aligned
        while ((i % 8))
            i++;

        // this many bytes on a row
        i /= 8;

        // size of clipmask
        clipsize = i * img_data->height;

        clip = new unsigned char[clipsize];
        memset(clip, 0, clipsize);
    }

    // initialize used colors counter
    int used[MAX_IMAGE_COLORS];
    memset(used, 0, MAX_IMAGE_COLORS*sizeof(int));

    // allocate an image
    htmImageInfo *image = new htmImageInfo(img_data);

    // Fill in appropriate fields
    image->url     = lstring::copy(url);    // image location
    image->clip    = clip;                  // clipbitmap bits
    image->options = options;               // set htmImageInfo options

    // If the image is quantized and scaled, the background pixel will
    // be needed to create the scaled clipmask.  Since quantization
    // blows away the background pixel, we will find it again from the
    // colors.
    htmColor cbg;
    if (do_clip) {
        cbg.pixel = img_data->bg;
        cbg.red = img_data->cmap[cbg.pixel].red;
        cbg.green = img_data->cmap[cbg.pixel].green;
        cbg.blue =  img_data->cmap[cbg.pixel].blue;
    }

    int cnt = 1;
    unsigned char *cptr = image->clip;
    unsigned char *ptr = image->data;

    // Fill array of used pixel indexes.  If we have a background
    // pixel to substitute, save the pixel that should be substituted
    // and create the XYBitmap data to be used as a clip mask.

    bool clip_valid = false;
    for (unsigned int i = 0; i < image->height; i++) {
        for (unsigned int j = 0, bcnt = 0; j < image->width; j++) {
            if (used[(int)*ptr] == 0) {
                used[(int)*ptr] = cnt;
                cnt++;

                // Check for validity of clip data.  If we don't have
                // a match it means the image has a useless
                // transparency so we can safely obliterate the
                // clipmask data.

                if (*ptr == image->bg)
                    clip_valid = true;
            }
            if (do_clip) {
                if (*ptr != image->bg)
                    *cptr += im_bitmap_bits[(bcnt % 8)];
                if ((bcnt % 8) == 7 || j == image->width-1)
                    cptr++;
                bcnt++;
            }
            ptr++;
        }
    }
    cnt--;

    // sanity
    if (cnt > img_data->cmapsize) {
        im_html->warning("imageDefaultProc", "Detected %i "
            "colors in image while %i have been specified! Recovering.",
            cnt, img_data->cmapsize);
        cnt = img_data->cmapsize;
    }

    // store number of colors
    image->ncolors = image->scolors = cnt;

    // Erase clipmask if we didn't find a matching pixel while there is
    // supposed to be one.

    if (do_clip && !clip_valid) {
        image->bg = -1;
        do_clip = false;
        delete [] image->clip;
        image->clip = 0;
        image->options &= ~IMAGE_CLIPMASK;
        image->transparency = IMAGE_NONE;
    }

    // We must perform dithering (or quantization) after we've
    // created a (possible) clipmask.  Both operations will nuke the
    // transparent pixel...

    bool bg_bad = false;
    if (max_colors && cnt > max_colors) {

        // Upon return, image has been dithered/quantized and cmap
        // contains a new colormap.
        img_data->data = image->data;
        quantizeImage(img_data, max_colors);
        image->data = img_data->data;
        img_data->data = 0;
        bg_bad = true;  // The background pixel (if any) is now crap.

        // need to get a new used array
        memset(used, 0, MAX_IMAGE_COLORS*sizeof(int));

        cnt = 1;
        ptr = image->data;

        for (unsigned int i = 0; i < image->height; i++) {
            for (unsigned int j = 0; j < image->width; j++) {
                if (used[(int)*ptr] == 0) {
                    used[(int)*ptr] = cnt;
                    cnt++;
                }
                ptr++;
            }
        }
        cnt--;
        // store number of colors
        image->ncolors = cnt;
    }

    // allocate image RGB values
    image->reds   = new unsigned short[3*cnt];
    image->greens = image->reds + cnt;
    image->blues  = image->greens + cnt;

    // now go and fill the RGB arrays
    for (int i = 0; i < MAX_IMAGE_COLORS; i++) {

        if (used[i] != 0) {
            int indx = used[i] - 1;
            image->reds[indx]   = img_data->cmap[i].red;
            image->greens[indx] = img_data->cmap[i].green;
            image->blues[indx]  = img_data->cmap[i].blue;
        }
    }

    if (do_clip && bg_bad) {
        // Assume that the background pixel is the closest match to the
        // original background color.

        int maxa = 1000000;
        for (unsigned int i = 0; i < image->ncolors; i++) {
            int a = (image->reds[i] - cbg.red)*(image->reds[i] - cbg.red) +
                (image->greens[i] - cbg.green)*(image->greens[i] - cbg.green) +
                (image->blues[i] - cbg.blue)*(image->blues[i] - cbg.blue);
            if (a < maxa) {
                maxa = a;
                cbg.pixel = i;
            }
            if (!a)
                break;
        }
        image->bg = cbg.pixel;
    }
    else if (do_clip)
        image->bg = used[image->bg] - 1;

    // final transition step to ZPixmap format

    ptr = image->data;
    for (int i = 0; i < size ; i++) {
        *ptr = (unsigned char)(used[(int)*ptr] - 1);
        ptr++;
    }

    // return loaded image data
    return (image);
}


// Default animation loading procedure.
//
htmImageInfo*
htmImageManager::animDefaultProc(htmRawImageData *img_data,
    htmRawImageData *master, int *global_used, int *global_used_size,
    bool return_global_used, const char *url)
{
    // default htmImageInfo flags
    int options = IMAGE_DEFERRED_FREE|IMAGE_RGB_SINGLE|IMAGE_ALLOW_SCALE;

    int max_colors = im_html->htm_max_image_colors;

    // are we to use a local colormap ?
    bool use_local_cmap = (img_data->cmapsize != 0);

    // raw data size
    int size = img_data->width * img_data->height;

    // pick up the background pixel and corresponding RGB values
    bool do_clip = false;
    int clipsize = 0;
    unsigned char *clip = 0;
    if (img_data->bg >= 0) {
        options |= IMAGE_CLIPMASK;
        do_clip = true;

        // allocate clipmask data
        int i = img_data->width;
        while ((i % 8))
            i++;
        i /= 8;
        clipsize = i * img_data->height;
        clip = new unsigned char[clipsize];
        memset(clip, 0, clipsize);
    }

    // allocate image
    htmImageInfo *image = new htmImageInfo(img_data);

    // Fill in appropriate fields
    if (url)
        // only first frame has this
        image->url = lstring::copy(url);
    image->clip    = clip;              // clipbitmap bits
    image->options = options;           // set htmImageInfo options

    // Fill array of used pixel indices.  gcolors is a count of the
    // colors really used by this frame, whether or not we should use
    // a global colormap.  We must make the distinction if we want
    // dithering to be done correctly for each frame.

    int used[MAX_IMAGE_COLORS], cnt, gcolors;
    if (use_local_cmap || return_global_used) {
        memset(used, 0, MAX_IMAGE_COLORS*sizeof(int));
        cnt = 1;
        gcolors = 1;
        unsigned char *ptr = image->data;
        for (int i = 0; i < size; i++, ptr++) {
            if (used[(int)*ptr] == 0) {
                used[(int)*ptr] = cnt++;
                gcolors++;
            }
        }
    }
    else {
        // gused is the array of colors really being used
        int gused[MAX_IMAGE_COLORS];
        memcpy(used, global_used, MAX_IMAGE_COLORS*sizeof(int));
        memset(gused, 0, MAX_IMAGE_COLORS*sizeof(int));
        cnt = *global_used_size + 1;
        unsigned char *ptr = image->data;
        gcolors = 1;
        for (int i = 0; i < size; i++, ptr++) {
            if (used[(int)*ptr] == 0)
                used[(int)*ptr] = cnt++;
            if (gused[(int)*ptr] == 0)
                gused[(int)*ptr] = gcolors++;
        }
    }
    cnt--;
    gcolors--;

    // store number of colors if we are using a local colormap
    if (use_local_cmap) {
        if (cnt > img_data->cmapsize)
            cnt = img_data->cmapsize;
        image->ncolors = image->scolors = cnt;
    }
    // only update global array when we aren't dithering
    else if (return_global_used || *global_used_size < cnt) {
        // only update if we don't have to quantize this image
        if (return_global_used || !(max_colors && gcolors > max_colors)) {
            for (int i = 0; i < MAX_IMAGE_COLORS; i++)
                if (global_used[i] == 0)
                    global_used[i] = used[i];
        }
        if (return_global_used)
            image->ncolors = image->scolors = gcolors;
    }

    // check clipmask stuff
    if (do_clip) {
        unsigned char *cptr = image->clip;
        unsigned char *ptr = image->data;
        for (int i = 0; i < (int)image->height; i++) {
            for (int j = 0, bcnt = 0; j < (int)image->width; j++) {
                if (*ptr != image->bg)
                    *cptr += im_bitmap_bits[(bcnt % 8)];
                if ((bcnt % 8) == 7 || j == (int)(image->width-1))
                    cptr++;
                bcnt++;
                ptr++;
            }
        }
    }

    // dither/quantize if necessary
    if (max_colors && gcolors > max_colors) {
        // if we don't have a local colormap, copy the global one
        if (!use_local_cmap) {
            img_data->allocCmap(master->cmapsize);
            // copy it
            memcpy(img_data->cmap, master->cmap,
                master->cmapsize * sizeof(htmColor));
            // forces use of the local colormap
            use_local_cmap = true;
        }

        // do it, have to reset img_data->data temporarily
        img_data->data = image->data;
        quantizeImage(img_data, max_colors);
        image->data = img_data->data;
        img_data->data = 0;

        // need to get a new used array
        memset(used, 0, MAX_IMAGE_COLORS*sizeof(int));

        cnt = 1;
        unsigned char *ptr = image->data;

        // compose it
        for (int i = 0; i < (int)image->height; i++) {
            for (int j = 0; j < (int)image->width; j++) {
                if (used[(int)*ptr] == 0) {
                    used[(int)*ptr] = cnt;
                    cnt++;
                }
                ptr++;
            }
        }
        cnt--;
        // store number of colors
        image->ncolors = cnt;
    }

    // Allocate image RGB values if we have a local colormap or when
    // we have to return it (for the first frame in this animation)

    if (use_local_cmap) {
        image->reds   = new unsigned short[3*cnt];
        image->greens = image->reds + cnt;
        image->blues  = image->greens + cnt;

        // now go and fill the RGB arrays
        for (int i = 0; i < MAX_IMAGE_COLORS; i++) {
            if (used[i] != 0) {
                int indx = used[i] - 1;
                image->reds[indx]   = img_data->cmap[i].red;
                image->greens[indx] = img_data->cmap[i].green;
                image->blues[indx]  = img_data->cmap[i].blue;
            }
        }
        // can happen when we have quantized the first frame in an animation
        if (return_global_used) {
            memcpy(global_used, used, MAX_IMAGE_COLORS*sizeof(int));
            *global_used_size = cnt;
            image->ncolors = cnt;

            // Quantization made a copy of the master colormap.  As
            // this is the first frame in this animation, we must free
            // the copy here or it won't get freed at all.

            if (img_data->cmapsize) {
                delete [] img_data->cmap;
                img_data->cmap = 0;
            }
            img_data->cmapsize = 0;
        }
    }
    else if (return_global_used || *global_used_size < cnt) {
        image->reds   = new unsigned short[3*cnt];
        image->greens = image->reds + cnt;
        image->blues  = image->greens + cnt;

        // now go and fill the RGB arrays
        for (int i = 0; i < MAX_IMAGE_COLORS; i++) {
            if (global_used[i] != 0) {
                int indx = global_used[i] - 1;
                image->reds[indx]   = master->cmap[i].red;
                image->greens[indx] = master->cmap[i].green;
                image->blues[indx]  = master->cmap[i].blue;
            }
        }
        *global_used_size = cnt;
    }

    // Final transition step to ZPixmap format
    unsigned char *ptr = image->data;
    if (use_local_cmap) {
        for (int i = 0; i < size ; i++) {
            *ptr = (unsigned char)(used[(int)*ptr] - 1);
            ptr++;
        }
        // map background pixel, too
        image->bg = used[image->bg] - 1;
    }
    else {
        for (int i = 0; i < size ; i++) {
            *ptr = (unsigned char)(global_used[(int)*ptr] - 1);
            ptr++;
        }
        // map background pixel, too
        image->bg = global_used[image->bg] - 1;
    }

    // release local colormap
    if (img_data->cmapsize && !return_global_used) {
        delete [] img_data->cmap;
        img_data->cmap = 0;
        img_data->cmapsize = 0;
    }

    // return loaded image data
    return (image);
}


// Creates an empty htmImageInfo structure required for delayed image
// creation (i.e., images with an alpha channel).
//
htmImageInfo*
htmImageManager::imageDelayedProc(htmRawImageData *img_data, ImageBuffer *ib)
{
    htmImageInfo *image = new htmImageInfo;

    // store image location and type of this image
    image->url     = lstring::copy(ib->file);
    image->type    = ib->type;
    image->depth   = ib->depth;
    image->width   = img_data->width;       // image width,  REQUIRED
    image->height  = img_data->height;      // image height, REQUIRED
    image->swidth  = img_data->width;
    image->sheight = img_data->height;
    image->ncolors = image->scolors = img_data->cmapsize;

    image->bg = -1;
    image->transparency = IMAGE_TRANSPARENCY_ALPHA;
    image->colorspace   = img_data->color_class;
    image->options      = IMAGE_DELAYED_CREATION | IMAGE_ALLOW_SCALE;
    image->fg_gamma     = img_data->fg_gamma;
    image->alpha        = img_data->data;

    img_data->data = 0;  // ownership change

    // return loaded image data
    return (image);
}


htmImageInfo*
htmImageManager::readGifAnimation(ImageBuffer *ib)
{
    // fallbacks if no dispose or timeout method is specified
    int fallback_dispose = IMAGE_DISPOSE_NONE, fallback_timeout = 50;
    int global_size = 0;

    htmImageInfo *all_frames = 0;
    htmImageInfo *frame = 0;

    int global_used[MAX_IMAGE_COLORS];
    memset(global_used, 0, MAX_IMAGE_COLORS*sizeof(int));

    // initialize gif animation reading
    int loop_count;
    GIFscreen *ctx;
    htmRawImageData *master = gifAnimInit(ib, &loop_count, &ctx);
    if (!master)
        return (0);

    unsigned int screen_height = master->height;
    unsigned int screen_width  = master->width;
    int bg = master->bg;

    // keep reading frames until we run out of them
    int x, y;
    int timeout = 50, dispose;
    int nframes = 0;
    htmRawImageData *img_data;
    while ((img_data = gifAnimNextFrame(ib, &x, &y, &timeout, &dispose,
            ctx)) != 0) {
        // go and create each frame
        if (nframes) {
            int cnt = global_size;
            frame->frame = animDefaultProc(img_data, master, &global_used[0],
                &cnt, false, 0);
            frame = frame->frame;

            // Check if the global colormap has been modified.  We may
            // *never* do this for images that have been quantized!

            if (global_size < cnt && frame->ncolors == frame->scolors) {
                // release previous colormap
                delete [] all_frames->reds;

                // store new cmap
                global_size = cnt;
                all_frames->reds = frame->reds;
                all_frames->greens = frame->greens;
                all_frames->blues  = frame->blues;
                all_frames->ncolors = all_frames->scolors = global_size;

                // wipe colormap for this frame, it uses the global one
                frame->reds = frame->greens = frame->blues = 0;
                frame->ncolors = frame->scolors = 0;
            }
        }
        else {
            // this is the first frame with the master colormap
            all_frames = animDefaultProc(img_data, master, &global_used[0],
                &global_size, true, ib->file);
            frame = all_frames;
        }

        if (dispose)
            fallback_dispose = dispose;
        if (timeout)
            fallback_timeout = timeout;
        // save info for this frame
        frame->depth = ib->depth;
        frame->x = x;
        frame->y = y;
        frame->width  = img_data->width;
        frame->height = img_data->height;
        frame->dispose = fallback_dispose;
        frame->timeout = fallback_timeout;

        unsigned int width = frame->width;
        unsigned int height = frame->height;

        // Reset dispose to none if the dimensions of this frame equal
        // the screen dimensions and if this image isn't transparent.
        // This will give us a small performance enhancement since the
        // frame refreshing routines don't have to restore the
        // background or blit the previous screen state to the screen.

        if (width == screen_width && height == screen_height && x == 0 &&
                y == 0 && bg == -1)
            frame->dispose = IMAGE_DISPOSE_NONE;

        // Final check before moving to the next frame:  if this frame
        // doesn't fit on the logical screen, clip the parts that lie
        // outside the logical screen area.  Images that fall
        // completely outside the logical screen are just left the way
        // they are.

        if (x + width > screen_width || y + height > screen_height) {
            int new_w = x + width > screen_width ? screen_width - x : width;
            int new_h = y + height > screen_height ? screen_height - y : height;
            if (x < (int)screen_width && y < (int)screen_height)
                clipImage(frame, new_w, new_h);
            else
                frame->options |= IMAGE_FRAME_IGNORE;
        }
        nframes++;
        delete img_data;
    }
    delete master;

    // terminate gif animation reading
    gifAnimTerminate(ib);

    if (all_frames) {
        all_frames->loop_count = loop_count;
        // nframes is total no of frames in this animation
        all_frames->nframes = nframes;
    }
    return (all_frames);
}


htmImage*
htmImageManager::copyHTMLImage(htmImage *image, const char *attributes)
{
    // If creation for this image should be delayed until it's needed,
    // set the global creation flag.

    if (image->DelayedCreation())
        im_delayed_creation = true;

    // If this is an orphaned image, it is currently not being used
    // and can thus be used without copying it or inserting it in the
    // list (images are orphaned when a resource is changed that does
    // not require a reload of images itself).

    if (image->IsOrphaned()) {
        image->options &= ~IMG_ORPHANED;
        return (image);
    }
    htmImage *dest = copyImage(image, attributes);

    // store in the image list
    addImageToList(dest);

    // add it to the child list of the parent image
    if (image->child == 0)
        image->child = dest;
    else {
        htmImage *tmp;
        for (tmp = image->child; tmp && tmp->child; tmp = tmp->child) ;
        tmp->child = dest;
    }
    return (dest);
}


// Initialize alpha channel processing:  obtain background color/image
// information which will be merged with alpha-channeled images.
//
void
htmImageManager::initAlphaChannels(bool for_body_image)
{
    // Always (re-)initialize the AlphaChannel buffer.  If the body
    // image is an alpha-channeled image we must use the current
    // background *color* for it, and for all subsequent images we
    // need to use the colormap of the background image.

    if (!im_alpha_buffer)
        im_alpha_buffer = new AlphaChannelInfo;
    else if (im_alpha_buffer->ncolors)
        delete [] im_alpha_buffer->bg_cmap;

    AlphaChannelInfo *alpha = im_alpha_buffer;
    alpha->bg_cmap = 0;
    alpha->ncolors = 0;
    alpha->fb_maxsample = (1 << im_html->htm_tk->tk_visual_depth()) - 1;

    // no body image or this is the body image, use background color
    if (im_body_image == 0 || for_body_image) {

        // current background color
        htmColor bg_color;
        bg_color.pixel = im_html->htm_cm.cm_body_bg;

        // get rgb components
        im_html->htm_tk->tk_query_colors(&bg_color, 1);

        alpha->background[0] = bg_color.red;
        alpha->background[1] = bg_color.green;
        alpha->background[2] = bg_color.blue;
    }
    else {
        htmImageInfo *bg_image = im_body_image->html_image;

        // allocate color_map pixel entries
        unsigned int *color_map = new unsigned int[bg_image->ncolors];
        alpha->ncolors = bg_image->ncolors;
        im_html->htm_tk->tk_get_pixels(bg_image->reds,
            bg_image->greens, bg_image->blues, bg_image->ncolors, color_map);

        // initialize body image colormap
        alpha->bg_cmap = new htmColor[alpha->ncolors];
        for (int i = 0; i < alpha->ncolors; i++)
            alpha->bg_cmap[i].pixel = color_map[i];

        // no longer needed
        delete [] color_map;

        // get rgb values
        im_html->htm_tk->tk_query_colors(alpha->bg_cmap, alpha->ncolors);
    }
}


// Recreate an image specified by image_word.  This function is only
// used for PNG images with either a tRNS chunk or an alpha channel.
// As the actual pixmap contents depend on the location of the image
// in the document, we need to create a new pixmap every time the
// document is resized.
//
void
htmImageManager::doAlphaChannel(htmImage *image)
{
    // Verify that this image has an owner, e.g., is not the body
    // image.  If it has an owner, get its position.  If this *is* the
    // body image, alpha channel processing is done against the
    // current background color.  (either set thru <body bgcolor="..">
    // or the default background color)

    int x = 0, y = 0;
    if (image->owner) {
        x = image->owner->words[0].area.x;
        y = image->owner->words[0].area.y;
    }

    // we require an htmImageInfo structure
    htmImageInfo *html_image = image->html_image;
    if (image->InfoFreed() || html_image == 0) {
        im_html->warning("doAlphaChannel",
            "Alpha channel processing failed for image\n    %s:\n"
            "    No htmImageInfo attached.", image->url);
        image->options &= ~IMG_DELAYED_CREATION;
        return;
    }

    // sanity
    if (html_image->type != IMAGE_PNG) {
        im_html->warning("doAlphaChannel",
            "Alpha channel processing failed for image\n    %s:\n"
            "    Image is not PNG (id = %i).", image->url,
            (int)html_image->type);
        image->options &= ~IMG_DELAYED_CREATION;
        return;
    }

    // more sanity
    if (!(html_image->DelayedCreation())) {
        im_html->warning("doAlphaChannel",
            "Alpha channel processing failed for image\n    %s:\n"
            "    IMAGE_DELAYED_CREATION bit not set.",
            image->url);
        image->options &= ~IMG_DELAYED_CREATION;
        return;
    }

    if (im_html->htm_if) {
        htmAnchorCallbackStruct cbs;
        cbs.url_type = ANCHOR_FILE_LOCAL;   // doesn't really matter
        cbs.href     = image->url;
        im_html->htm_if->emit_signal(S_ANCHOR_TRACK, &cbs);
    }

    // fill in a rawImage structure
    htmRawImageData raw_data;
    raw_data.data     = html_image->alpha;
    raw_data.alpha    = 0;
    raw_data.width    = html_image->swidth;
    raw_data.height   = html_image->sheight;
    raw_data.bg       = -1;
    raw_data.cmap     = 0;
    raw_data.cmapsize = 0;
    raw_data.type     = html_image->type;
    raw_data.fg_gamma = html_image->fg_gamma;
    raw_data.color_class      = html_image->colorspace;
    raw_data.delayed_creation = true;

    htmRawImageData *img_data = reReadPNG(&raw_data, x, y, image->owner == 0);
    raw_data.data = 0;  // protect from destructor
    raw_data.cmap = 0;  // protect from destructor
    img_data->type = IMAGE_PNG;
    htmImageInfo *new_info = imageDefaultProc(img_data, image->url);
    delete img_data;

    // sanity
    if (new_info == 0) {
        image->options &= ~IMG_DELAYED_CREATION;
        return;
    }

    // update the htmImageInfo for this image
    if (!(html_image->Shared())) {
        delete [] html_image->data;
        if (html_image->Clipmask())
            delete [] html_image->clip;
        if (html_image->RGBSingle())
            delete [] html_image->reds;
        else {
            delete [] html_image->reds;
            delete [] html_image->greens;
            delete [] html_image->blues;
        }
    }

    // Now copy members of the new ImageInfo structure that are likely
    // to have changed.  This is required if we do not want to mess up
    // any of the user's htmImageInfo administration (caching and
    // stuff).  scolors is left untouched, unless it is zero (png has
    // a palette size for gray and paletted colorspaces).

    html_image->data    = new_info->data;   new_info->data = 0;
    html_image->clip    = new_info->clip;   new_info->clip = 0;
    html_image->reds    = new_info->reds;   new_info->reds = 0;
    html_image->greens  = new_info->greens; new_info->greens = 0;
    html_image->blues   = new_info->blues;  new_info->blues = 0;
    html_image->depth   = new_info->depth;
    html_image->ncolors = new_info->ncolors;
    html_image->depth   = 8;    // always
    if (!html_image->scolors)
        html_image->scolors = new_info->scolors;

    // Check what flags we should set.  We must check a number of
    // flags that the user might have set himself (such as keeping the
    // image alive).  We ignore the delayed, clipmask and progressive
    // flags:  when we get here, the image is fully available.  And
    // alpha channels don't even begin to think about clipmasks, it's
    // why I've got to deal with this delayed creation mess in the
    // first place!!

    unsigned int flags = 0;
    if (html_image->FreeLater())
        flags |= IMAGE_DEFERRED_FREE;     // free when doc switches?
    else if (html_image->FreeNow())
        flags |= IMAGE_IMMEDIATE_FREE;    // free when image created?
    if (html_image->Scale())
        flags |= IMAGE_ALLOW_SCALE;       // may we scale?
    if (html_image->Shared())
        flags |= IMAGE_SHARED_DATA;       // must data be kept alive?

    // flags from the imageDefaultProc and of course the delayed creation bit
    flags |= IMAGE_RGB_SINGLE|IMAGE_DELAYED_CREATION;

    html_image->options = flags;

    // and free the new info, we no longer need it
    delete new_info;

    // this image has an alpha channel
    html_image->transparency = IMAGE_TRANSPARENCY_ALPHA;

    // save image type as well
    html_image->type = raw_data.type;

    // Fourth step is to create the actual pixmap.  First destroy any
    // previous pixmaps.

    image->freePixmaps();

    image->makeColormap(html_image);
    image->html_image = html_image;
    htmPixmap *clip;
    htmPixmap *pixmap = infoToPixmap(image, html_image, image->width,
        image->height, image->pixel_map, &clip);

    image->pixmap = pixmap;
    image->clip   = clip;       // which is always None

    // Unset delayed creation if we have a solid background color:
    // the image does not have to be recreated when this doc is
    // resized.  Also unset delayed creation flag for the body image,
    // it does not need reprocessing on document resize.

    if (image->IsBackground())
        image->options &= ~IMG_DELAYED_CREATION;
    else if (im_body_image)
        image->options |= IMG_DELAYED_CREATION;
    else
        image->options &= ~IMG_DELAYED_CREATION;
}


// Final body image processing:  verifies a few things and plugs in
// the background color if the body image is transparent.  Stores the
// background image in the widget.
//
void
htmImageManager::processBodyImage(htmImage *body_image, unsigned int width,
    unsigned int height)
{
    // animations are not allowed as background images
    if (body_image->IsAnim()) {
        im_html->warning("processBodyImage",
            "animations not allowed as background images.");
        im_body_image = 0;
        return;
    }

    // mark as the background image
    body_image->options |= IMG_ISBACKGROUND;

    // See if the image was loaded succesfully, e.i.  is an external
    // image.  might seem strange, but this image *will* be freed when
    // htmFreeAllImages is called (e.i.  destroy() is invoked or new
    // text is set.

    if (body_image->IsInternal())
        im_body_image = 0;
    else {
        im_body_image = body_image;

        // We support transparent background images as well.  To do so,
        // we need to create a pixmap, fill it with the current
        // background color, set the clipmask and copy the real image
        // onto it.  Then we replace the old background image with the
        // new semi-transparent background image.

        if (!body_image->DelayedCreation() && body_image->clip != 0) {

            // create a new pixmap
            htmPixmap *pixmap = im_html->htm_tk->tk_new_pixmap(width, height);
            if (pixmap) {
                im_html->htm_tk->tk_set_foreground(im_html->htm_cm.cm_body_bg);
                im_html->htm_tk->tk_set_draw_to_pixmap(pixmap);
                im_html->htm_tk->tk_draw_rectangle(true, 0, 0, width, height);

                im_html->htm_tk->tk_set_clip_mask(body_image->pixmap,
                    body_image->clip);
                im_html->htm_tk->tk_set_clip_origin(0, 0);

                // and copy original pixmap to the new pixmap
                im_html->htm_tk->tk_draw_pixmap(0, 0, body_image->pixmap,
                    width, height, 0, 0);
                im_html->htm_tk->tk_set_draw_to_pixmap(0);

                // release original pixmap
                im_html->htm_tk->tk_release_pixmap(body_image->pixmap);
                im_html->htm_tk->tk_release_bitmap(body_image->clip);

                // save new pixmap
                body_image->pixmap = pixmap;
                body_image->clip = 0;

                // all done!
            }
        }
    }
}


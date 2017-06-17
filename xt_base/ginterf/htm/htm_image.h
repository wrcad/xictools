
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
 *   Stephen R. Whiteley  <srw@wrcad.com>
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
 * $Id: htm_image.h,v 1.10 2015/06/11 05:54:04 stevew Exp $
 *-----------------------------------------------------------------------*/

#ifndef HTM_IMAGE_H
#define HTM_IMAGE_H

#include <sys/types.h>
#include <string.h>

// The Unisys patents have expired, so this can be used royalty free.
#define GIF_DECODE

#ifdef GIF_DECODE
extern int decodeGIFImage(htmGIFStream*);
#endif

class htmWidget;
namespace htm
{
    struct AllEvents;
    struct mapArea;
}
struct htmObjectTable;
struct htmObject;
struct htmColorContext;
struct htmRawImageData;

// Absolute maximum nnumber of colors the widget may use.  It is the
// maximum value for the maxImageColors resource.  Increasing this
// number isn't a wise thing to do since all image functions are
// optimized for using palletted images.  If you want the widget to
// handle more than 256 colors, you will have to modify the code.
//
#define MAX_IMAGE_COLORS    256

// Default gamma correction value for your display.  This is only used
// for images that support gamma correction (JPEG and PNG).  2.2 is a
// good assumption for almost every X display.  For a Silicon Graphics
// displays, change this to 1.8 For Macintosh displays (MkLinux),
// change this to 1.4 (so I've been told) If you change this value, it
// must be a floating point value.
//
#define HTML_DEFAULT_GAMMA  2.2

namespace htm
{
    // GIF procs
    size_t htmGifReadOK(ImageBuffer*, unsigned char*, int);
    size_t htmGifGetDataBlock(ImageBuffer*, unsigned char*);

    // Server/client side and map type values
    enum Imagemap
    {
        MAP_NONE = 1,
        MAP_SERVER,
        MAP_CLIENT
    };

    // Codes for htmImageGetType()
    enum
    {
        IMAGE_ERROR,                // error on image loading
        IMAGE_UNKNOWN,              // unknown image
        IMAGE_XPM,                  // X11 pixmap
        IMAGE_XBM,                  // X11 bitmap
        IMAGE_GIF,                  // CompuServe(C) Gif87a or Gif89a
        IMAGE_GIFANIM,              // animated gif
        IMAGE_GIFANIMLOOP,          // animated gif with loop extension
        IMAGE_GZF,                  // compatible Gif87a or Gif89a
        IMAGE_GZFANIM,              // compatible animated gif
        IMAGE_GZFANIMLOOP,          // compatible animated gif
        IMAGE_JPEG,                 // JPEG image
        IMAGE_PNG,                  // PNG image
        IMAGE_TIFF,                 // TIFF image
        IMAGE_PPM,                  // PPM image
        IMAGE_FLG                   // Fast Loadable Graphic
    };

    // Return values for progressive loading callback
    enum
    {
        STREAM_OK           = 1,    // internally used value
        STREAM_END          = 0,    // data stream ended (no more data)
        STREAM_SUSPEND      = -1,   // data stream suspended (not enough data)
        STREAM_ABORT        = -2,   // data stream aborted
        STREAM_RESIZE       = -3    // resize input buffer
    };

    // Return values for the GIF decoder
    enum
    {
        GIF_STREAM_OK       = 2,
        GIF_STREAM_END      = 1,
        GIF_STREAM_ERR      = 0,
        GIF_STREAM_INIT     = -1,
        GIF_STREAM_FINAL    = -2
    };

    // Values for transparency (value for the "bg" field in both htmImage
    // and htmImageInfo structures).  Possible values are:
    //
    // IMAGE_NONE
    //   Indicates the image is not transparent.
    //
    // IMAGE_TRANSPARENCY_BG
    //   Indicates the image achieves transparency by substituting the
    //   current background setting (can be a single color or background
    //   image.  Internally, such transparency is achieved by using a
    //   clipmask).
    //
    // IMAGE_TRANSPARENCY_ALPHA
    //   Indicates the image achieves transparency by using an alpha
    //   channel.  This transparency is currently only used by PNG images
    //   with an alpha channel or a tRNS chunk (which is expanded to an
    //   alpha channel internally).
    //
    enum
    {
        IMAGE_NONE,
        IMAGE_TRANSPARENCY_BG,
        IMAGE_TRANSPARENCY_ALPHA
    };

    // Values for the colorspace field.
    //
    // IMAGE_COLORSPACE_GRAYSCALE
    //   Image contains only shades of gray.  The colorcube is reduced to a
    //   1D representation.  All components in a shade have the same value.
    //   The pixel values are equal to the value of a single color
    //   component.
    //
    // IMAGE_COLORSPACE_INDEXED
    //   Image uses a fixed palette.  Colorcube is mapped to a 1D
    //   lookup-table.
    //
    // IMAGE_COLORSPACE_RGB
    //   Image uses a full 3D colorcube.
    //
    enum
    {
        // IMAGE_NONE
        IMAGE_COLORSPACE_GRAYSCALE = 1,
        IMAGE_COLORSPACE_INDEXED,
        IMAGE_COLORSPACE_RGB
    };

    // Image option bits.
    // Each of these bits represents certain state information about an image.
    //
    enum
    {
        IMG_ISBACKGROUND        = 0x1,      // is a background image
        IMG_ISINTERNAL          = 0x2,      // is an internal image
        IMG_ISCOPY              = 0x4,      // is a referential copy
        IMG_ISANIM              = 0x8,      // is an animation
        IMG_FRAMEREFRESH        = 0x10,     // set when running an animation
        IMG_HASDIMENSIONS       = 0x20,     // dimensions are given in <IMG>
        IMG_HASSTATE            = 0x40,     // current state pixmap present
        IMG_INFOFREED           = 0x80,     // imageinfo has been freed
        IMG_DELAYED_CREATION    = 0x100,    // create when needed
        IMG_ORPHANED            = 0x200,    // indicates orphaned image
        IMG_PROGRESSIVE         = 0x400     // indicates image is being loaded
    };

    // Possible colorclass an image can have
    enum
    {
        COLOR_CLASS_GRAYSCALE,
        COLOR_CLASS_INDEXED,
        COLOR_CLASS_RGB
    };

    // htmImageInfo structure options field bits
    //
    // The ``Set by default'' indicates a bit set when the
    // htmImageDefaultProc is used to read an image.  The ``Read Only''
    // indicates a bit you should consider as read-only.
    //
    // IMAGE_DELAYED
    //   Indicates the image is delayed, e.i.  it will be provided at a
    //   later stage;
    //
    // IMAGE_DEFERRED_FREE
    //   Indicates the widget may free this structure when a new document is
    //   loaded.
    //
    // IMAGE_IMMEDIATE_FREE
    //   Indicates the widget may free this structure when the widget no
    //   longer needs it;
    //
    // IMAGE_RGB_SINGLE
    //   Indicates that the reds, greens and blues fields are allocated in a
    //   single memory area instead of three seperate memory arrays.
    //
    // IMAGE_ALLOW_SCALE
    //   Indicates that scaling an image is allowed.
    //
    // IMAGE_FRAME_IGNORE
    //   Use with animations:  set this bit when a frame falls outside the
    //   logical screen area.  No pixmap is created but the timeout for the
    //   frame is kept.
    //
    // IMAGE_CLIPMASK
    //   This bit is set when the returned htmImageInfo structure contains
    //   clipmask data.  The widget uses this info to create a clipping
    //   bitmap.  Changing this bit from set to unset will lead to a memory
    //   leak while changing it from unset to set *without* providing a
    //   clipmask yourself *will* cause an error to happen.  You can set
    //   this bit when you are providing your own clipmask (to provide
    //   non-rectangular images for example), PROVIDED you fill the ``clip''
    //   field with valid bitmap data (a stream of bytes in XYBitmap format
    //   and the same size of the image).
    //
    // IMAGE_SHARED_DATA
    //   This bit is set when images share data.  The widget sets this bit
    //   when the image in question is an internal image, e.i., one for
    //   which the image data may never be freed.  Be carefull setting this
    //   bit yourself, since it prevents the widget from freeing the image
    //   data present in the htmImageInfo structure.  It can easily lead to
    //   memory leaks when an image is not an internal image.
    //
    // IMAGE_PROGRESSIVE
    //   Setting this bit will enable the widget progressive image loading.
    //   A function must have been installed on the NprogressiveReadProc
    //   resource prior to setting this bit.  Installing a function on the
    //   progressiveEndProc is optional.  When this bit is set all other
    //   bits will be ignored.
    //
    // IMAGE_DELAYED_CREATION
    //   This bit is read-only.  It is used internally by the widget for
    //   images with an alpha channel.  Alpha channel processing merges the
    //   current background with the original RGB data from the image and
    //   uses the result to compose the actual on-screen image (the merged
    //   data is stored in the ``data'' field of the htmImageInfo
    //   structure).  The widget needs to store the original data somewhere,
    //   and when this bit is set it is stored in the ``rgb'' field of the
    //   htmImageInfo structure.  When this bit is set, the returned
    //   htmImageInfo may NOT BE FREED as long as the current document is
    //   alive.  You can discard it as soon as a new document is loaded.
    //
    enum
    {
        IMAGE_DELAYED          = 0x1,
        IMAGE_DEFERRED_FREE    = 0x2,       // set by default
        IMAGE_IMMEDIATE_FREE   = 0x4,
        IMAGE_RGB_SINGLE       = 0x8,       // set by default
        IMAGE_ALLOW_SCALE      = 0x10,      // set by default
        IMAGE_FRAME_IGNORE     = 0x20,
        IMAGE_CLIPMASK         = 0x40,      // Read Only
        IMAGE_SHARED_DATA      = 0x80,      // Read Only
        IMAGE_PROGRESSIVE      = 0x100,
        IMAGE_DELAYED_CREATION = 0x200      // Read Only
    };

    // htmImageInfo animation disposal values
    //
    // A disposal method specifies what should be done before the current
    // frame is rendered.  Possible values are:
    //
    // IMAGE_DISPOSE_NONE
    //   Do nothing, overlays the previous frame with the current frame.
    //
    // IMAGE_DISPOSE_BY_BACKGROUND
    //   Restore to background color.  The area used by the previous frame
    //   must be restored to the background color/image
    //
    // IMAGE_DISPOSE_BY_PREVIOUS
    //   Restore to previous. The area used by the previous frame must be
    //   restored to what was there prior to rendering the previous frame.
    //
    enum
    {
        // IMAGE_NONE
        IMAGE_DISPOSE_NONE = 1,         // default behaviour
        IMAGE_DISPOSE_BY_BACKGROUND,
        IMAGE_DISPOSE_BY_PREVIOUS
    };

    // Primary image cache actions
    enum
    {
        IMAGE_STORE,                // store an image in the cache
        IMAGE_GET,                  // retrieve an image from the cache
        IMAGE_DISCARD               // discard an image from the cache
    };

    // htmImage frame selection flags
    // Any positive number will return the requested frame.  If larger
    // than total framecount, last frame is returned.
    //
    enum
    {
        AllFrames       = -1,       // do all frames
        FirstFrame      = -2,       // only use first frame
        LastFrame       = -3        // only do last frame
    };

    // htmImage configuration flags
    enum
    {
        ImageFSDither       = 0x1,      // Floyd-Steinberg on quantized images
        ImageCreateGC       = 0x2,      // create gc for image
        ImageWorkSpace      = 0x4,      // create animation workspace
        ImageClipmask       = 0x8,      // create clipmask
        ImageBackground     = 0x10,     // substitute background pixel
        ImageQuantize       = 0x20,     // quantize image
        ImageMaxColors      = 0x40,     // sets maximum colors
        ImageGifDecodeProc  = 0x80,     // gif lzw decoder function
        ImageGifzCmd        = 0x100,    // gif lzw uncompress command
        ImageFrameSelect    = 0x200,    // frame selection
        ImageScreenGamma    = 0x400     // gamma correction. JPEG and PNG only
    };

    // The following structure is used to mimic file access in memory.
    //
    struct ImageBuffer
    {
        ImageBuffer(size_t sz = 0)
            {
                file        = 0;
                buffer      = sz ? new unsigned char[sz] : 0;
                curr_pos    = buffer;
                next        = 0;
                size        = sz;
                may_free    = (sz != 0);
                type        = 0;
                depth       = 0;
            }

        ~ImageBuffer()
            {
                if (may_free) {
                    delete [] file;
                    delete [] buffer;
                }
            }

        void rewind()
            {
                next = 0;
                curr_pos = buffer;
            }

        const char *file;           // name of file
        unsigned char *buffer;      // memory buffer
        unsigned char *curr_pos;    // current position in buffer
        size_t next;                // current block count
        size_t size;                // total size of in-memory file
        bool may_free;              // true when we must free this block
        unsigned char type;         // type of image
        int depth;                  // depth of this image
    };

    // This struct is required to properly perform alpha channel
    // processing.  It contains information about the current background
    // setting.
    //
    // Alpha channel processing is done for PNG images with (obviously) an
    // alpha channel.  The data used for creating the pixmap is a merger
    // of the original image data with the current background setting
    // (fixed color or an image).  When a document with such an image
    // contains a background image, gtkhtm needs to redo this alpha
    // processing whenever the document layout is changed:  the exact
    // result of this merger depends on the position of the image.  This
    // can be a rather slow process (alpha channels require floating point
    // ops), and by at least storing the current background info we can
    // achieve some performance increase.
    //
    struct AlphaChannelInfo
    {
        AlphaChannelInfo()
            {
                fb_maxsample    = 0;
                background[0]   = 0;
                background[1]   = 0;
                background[2]   = 0;
                ncolors         = 0;
                bg_cmap         = 0;
            }

        int fb_maxsample;           // frame buffer maximum sample value
        int background[3];          // solid background color: R, G, B
        int ncolors;                // size of background image colormap
        htmColor *bg_cmap;          // background image colormap
    };
}

// External GIF decoder stream object.  This is the only argument to
// any procedure installed on the htmNdecodeGIFProc resource.
//
// The first block is kept up to date by gtkhtm and is read-only.
// When state is GIF_STREAM_INIT, the decoder should initialize it's
// private data and store it in the external_state field so that it
// can be used for successive calls to the decoder.  When state is
// GIF_STREAM_FINAL, the decoder should wrapup and flush all pending
// data.  It can also choose to destruct it's internal data structures
// here (another call with state set to GIF_STREAM_END will be made
// when the internal loader is destroying it's internal objects as
// well).
//
// All following fields are the ``public'' fields and must be updated
// by the external decoder.  The msg field can be set to an error
// message if the external decoder fails for some reason.  gtkhtm will
// then display this error message and abort this image.
//
struct htmGIFStream
{
    htmGIFStream(int cs, unsigned char *nxto, unsigned int avo)
        {
            state           = GIF_STREAM_INIT;
            codesize        = cs;
            is_progressive  = false;
            next_in         = 0;
            avail_in        = 0;
            total_in        = 0;

            next_out        = nxto;
            avail_out       = avo;
            total_out       = 0;

            msg             = 0;
            external_state  = 0;
        }

    // read-only fields
    int state;                  // decoder state
    int codesize;               // initial LZW codesize
    bool is_progressive;        // when used by a progressive loader
    unsigned char *next_in;     // next input byte
    unsigned int avail_in;      // number of bytes available at next_in
    unsigned int total_in;      // total nb of input bytes read so far

    // fields to be updated by caller
    unsigned char *next_out;    // next output byte should be put there
    unsigned int avail_out;     // remaining free space at next_out
    unsigned int total_out;     // total nb of bytes output so far

    char *msg;                  // last error message, or 0
    void *external_state;       // room for decoder-specific data
};

// Client-side imagemap information.
//
struct htmImageMap
{
    htmImageMap(const char*);
    ~htmImageMap();
    void free();

    char                *name;      // name of map
    int                 nareas;     // no of areas
    mapArea             *areas;     // list of areas
    htmImageMap         *next;      // ptr to next imagemap
};

// Return from the imageDefaultProc function.
//
struct htmImageInfo
{
    htmImageInfo(htmRawImageData* = 0);
    ~htmImageInfo();

    bool Delayed()      { return (options & IMAGE_DELAYED); }
    bool FreeLater()    { return (options & IMAGE_DEFERRED_FREE); }
    bool FreeNow()      { return (options & IMAGE_IMMEDIATE_FREE); }
    bool Scale()        { return (options & IMAGE_ALLOW_SCALE); }
    bool RGBSingle()    { return (options & IMAGE_RGB_SINGLE); }
    bool Shared()       { return (options & IMAGE_SHARED_DATA); }
    bool Clipmask()     { return (options & IMAGE_CLIPMASK); }
    bool DelayedCreation() { return (options & IMAGE_DELAYED_CREATION); }
    bool Progressive()  { return (options & IMAGE_PROGRESSIVE); }

    // check whether the body image is fully loaded
    bool BodyImageLoaded()
        {
            return ((void*)this ? (!Delayed() && !Progressive()) : true);
        }

    // regular image fields
    char *url;                  // original location of image
    unsigned char *data;        // raw image data. ZPixmap format
    unsigned char *clip;        // raw clipmask data. XYBitmap format
    unsigned int width;         // used image width, in pixels
    unsigned int height;        // used image height, in pixels
    unsigned short *reds;       // red image pixels
    unsigned short *greens;     // green image pixels
    unsigned short *blues;      // blue image pixels
    int bg;                     // transparent pixel index/type
    unsigned int ncolors;       // Number of colors in the image
    unsigned int options;       // image option bits

    // image classification fields and original data
    unsigned char type;         // image type, see the IMAGE_ enum above
    unsigned char depth;        // bits per pixel for this image
    unsigned char colorspace;   // colorspace for this image
    unsigned char transparency; // transparency type for this image
    unsigned int swidth;        // image width as read from image
    unsigned int sheight;       // image height as read from image
    unsigned int scolors;       // Original number of colors in the image

    // special fields for images with an alpha channel
    unsigned char *alpha;       // alpha channel data
    float fg_gamma;             // image gamma

    // additional animation data
    int x;                      // logical screen x-position for this frame
    int y;                      // logical screen y-position for this frame
    int loop_count;             // animation loop count
    unsigned char dispose;      // image disposal method
    int timeout;                // frame refreshment in milliseconds
    int nframes;                // no of animation frames remaining
    htmImageInfo *frame;        // ptr to next animation frame

    void *user_data;            // any data to be stored with this image
};

// Animation frame data
struct htmImageFrame
{
    htmImageFrame()
        {
            timeout     = 0;
            dispose     = 0;
            pixmap      = 0;
            clip        = 0;
            prev_state  = 0;
            pixel_map   = 0;
            npixels     = 0;
        }

    htmRect area;               // extent of frame
    int timeout;                // timeout for the next frame
    unsigned char dispose;      // previous frame disposal method
    htmPixmap *pixmap;          // actual image
    htmBitmap *clip;            // image clipmask
    htmPixmap *prev_state;      // previous screen state
    unsigned int *pixel_map;    // local pixel map
    int npixels;                // size of local pixel map
};

struct htmRawImageData
{
    htmRawImageData(int w = 0, int h = 0, int size = 0)
        {
            data                = 0;
            if (w*h > 0) {
                data = new unsigned char[w*h];
                memset(data, 0, w*h);
            }
            alpha               = 0;
            width               = w;
            height              = h;
            bg                  = -1;
            cmap                = 0;
            cmapsize            = 0;
            if (size > 0) {
                cmap = new htmColor[size];
                for (int i = 0; i < size; i++)
                    cmap[i].pixel = i;
                cmapsize = size;
            }
            type                = 0;
            color_class         = size > 0 ? IMAGE_COLORSPACE_INDEXED : 0;
            fg_gamma            = 0;
            delayed_creation    = false;
        }

    ~htmRawImageData()
        {
            delete [] data;
            delete [] cmap;
        }

    void allocCmap(int size)
        {
            cmap = new htmColor[size];
            for (int i = 0; i < size; i++)
                cmap[i].pixel = i;
            cmapsize = size;
        }

    void clear()
        {
            delete [] data;
            delete [] cmap;
            data                = 0;
            alpha               = 0;
            width               = 0;
            height              = 0;
            bg                  = -1;
            cmap                = 0;
            cmapsize            = 0;
            type                = 0;
            color_class         = 0;
            fg_gamma            = 0;
            delayed_creation    = false;
        }

    unsigned char   *data;          // raw image data
    unsigned char   *alpha;         // alpha channel data
    int             width;          // image width in pixels
    int             height;         // image height in pixels
    int             bg;             // transparent pixel index
    htmColor        *cmap;          // colormap for this image
    int             cmapsize;       // actual no of colors in image colormap
    unsigned char   type;           // type of image
    unsigned char   color_class;    // color class for this image
    float           fg_gamma;       // image foreground gamma
    bool            delayed_creation;
};


// Magic number for the htmImage structure.  This field is used
// to verify the return value from a user-installed primary image
// cache.
//
#define HTML_IMAGE_MAGIC      0xce

// Internal image format.  One very important thing to note is that
// the meaning of the (width,height) and (swidth,sheight) members of
// this structure is exactly *OPPOSITE* to the members with the same
// name in the public structures (htmImageInfo and htmImage).
//
struct htmImage
{
    htmImage(htmImageManager*);
    ~htmImage();

    bool IsBackground()    { return (options & IMG_ISBACKGROUND); }
    bool IsInternal()      { return (options & IMG_ISINTERNAL); }
    bool IsCopy()          { return (options & IMG_ISCOPY); }
    bool IsAnim()          { return (options & IMG_ISANIM); }
    bool FrameRefresh()    { return (options & IMG_FRAMEREFRESH); }
    bool HasDimensions()   { return (options & IMG_HASDIMENSIONS); }
    bool HasState()        { return (options & IMG_HASSTATE); }
    bool InfoFreed()       { return (options & IMG_INFOFREED); }
    bool DelayedCreation() { return (options & IMG_DELAYED_CREATION); }
    bool IsOrphaned()      { return (options & IMG_ORPHANED); }
    bool IsProgressive()   { return (options & IMG_PROGRESSIVE); }

    void makeColormap(htmImageInfo*);
    void freeColormap();
    void freePixmaps();
    void getSize(const char*);

    htmImageManager *manager;       // owning image manager

    // Normal image data
    unsigned char   magic;          // structure identifier
    char            *url;           // raw src specification
    htmImageInfo    *html_image;    // local image data
    htmPixmap       *pixmap;        // actual image
    htmBitmap       *clip;          // for transparant pixmaps
    unsigned int    options;        // image option bits
    int             width;          // resulting image width
    int             height;         // resulting image height
    int             npixels;        // no of allocated pixels
    unsigned int    *pixel_map;     // color pixels used

    // Possible <IMG> attributes
    int             swidth;         // requested image width
    int             sheight;        // requested image height
    char            *alt;           // alternative image text
    Alignment       align;          // image alignment
    Imagemap        map_type;       // type of map to use
    char            *map_url;       // image map url/name
    unsigned        border;         // image border thickness
    unsigned        hspace;         // horizontal spacing
    unsigned        vspace;         // vertical spacing

    htmObjectTable  *owner;         // owner of this image
    htmImage        *child;         // ptr to copies of this image
    htmImage        *next;          // ptr to next image

    // animation data
    htmImageFrame   *frames;        // array of animation frames
    int             nframes;        // no of frames following
    int             current_frame;  // current frame count
    int             current_loop;   // current loop count
    int             loop_count;     // maximum loop count
    int             proc_id;        // timer id for animations

    // other data
    AllEvents       *events;        // events to be served
};

#endif


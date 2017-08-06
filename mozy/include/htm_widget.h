
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

#ifndef HTM_WIDGET_H
#define HTM_WIDGET_H

#include "config.h"

typedef void htmEvent;

// Default uncompress command (used to be "uncompress").
#define HTM_DEFAULT_UNCOMPRESS  "gunzip"

// HTML Elements internal id's

namespace htm
{

    // This list is alphabetically sorted to speed up the searching process.
    // DO NOT MODIFY
    //
    enum htmlEnum
    {
        HT_DOCTYPE, HT_A, HT_ADDRESS, HT_APPLET, HT_AREA, HT_B, HT_BASE,
        HT_BASEFONT, HT_BIG, HT_BLOCKQUOTE, HT_BODY, HT_BR, HT_CAPTION,
        HT_CENTER, HT_CITE, HT_CODE, HT_DD, HT_DFN, HT_DIR, HT_DIV, HT_DL,
        HT_DT, HT_EM, HT_FONT, HT_FORM, HT_FRAME, HT_FRAMESET, HT_H1, HT_H2,
        HT_H3, HT_H4, HT_H5, HT_H6, HT_HEAD, HT_HR, HT_HTML, HT_I, HT_IMG,
        HT_INPUT, HT_ISINDEX, HT_KBD, HT_LI, HT_LINK, HT_MAP, HT_MENU,
        HT_META, HT_NOFRAMES, HT_OL, HT_OPTION, HT_P, HT_PARAM, HT_PRE,
        HT_SAMP, HT_SCRIPT, HT_SELECT, HT_SMALL, HT_STRIKE, HT_STRONG,
        HT_STYLE, HT_SUB, HT_SUP, HT_TAB, HT_TABLE, HT_TD, HT_TEXTAREA, HT_TH,
        HT_TITLE, HT_TR, HT_TT, HT_U, HT_UL, HT_VAR, HT_ZTEXT
    };

    // corresponding HTML Element names.
    // This list is alphabetically sorted to speed up the searching process.
    // DO NOT MODIFY
    extern const char * const html_tokens[];

    // Line styles
    enum
    {
        NO_LINE         = 0x0,      // no lines at all
        ALT_STYLE       = 0x1,      // no lines, use alternate style (button),
                                    //  supersedes other flags
        LINE_DASHED     = 0x2,      // dashed line, else solid
        LINE_DOUBLE     = 0x4,      // double line, else single
        LINE_STRIKE     = 0x8,      // render as strikeout
        LINE_UNDER      = 0x10      // render as underline
    };

    // Possible types of HTML objects.  All text types are only used when
    // computing the screen layout.
    //
    enum htmObjectType
    {
        OBJ_NONE,
        OBJ_TEXT,           // text element
        OBJ_PRE_TEXT,       // preformatted text
        OBJ_BULLET,         // all types of markers for lists
        OBJ_HRULE,          // horizontal rule
        OBJ_TABLE,          // table elements
        OBJ_TABLE_FRAME,    // table caption, row, cell elements
        OBJ_IMG,            // image elements
        OBJ_FORM,           // form elements
        OBJ_APPLET,         // applet elements
        OBJ_BLOCK           // other block level elements
    };

    // Text alignment
    enum TextAlignment
    {
        ALIGNMENT_END,
        ALIGNMENT_CENTER,
        ALIGNMENT_BEGINNING
    };

    // Horizontal/Vertical alignment data
    enum Alignment
    {
        HALIGN_NONE,        // horizontal alignment
        HALIGN_LEFT,
        HALIGN_CENTER,
        HALIGN_RIGHT,
        HALIGN_JUSTIFY,     // extension for fully justified text
        CLEAR_LEFT,
        CLEAR_RIGHT,
        CLEAR_ALL,
        VALIGN_NONE,        // vertical alignment
        VALIGN_TOP,
        VALIGN_MIDDLE,
        VALIGN_BOTTOM,
        VALIGN_BASELINE
    };

    // HTML list marker enumeration type
    enum Marker
    {
        MARKER_NONE = 0,
        MARKER_ARABIC = 10,
        MARKER_ALPHA_LOWER,
        MARKER_ALPHA_UPPER,
        MARKER_ROMAN_LOWER,
        MARKER_ROMAN_UPPER,
        MARKER_DISC = 15,
        MARKER_SQUARE,
        MARKER_CIRCLE
    };

    // htmWidget::tk_visual_mode() return
    //
    enum CCXmode
    {
        MODE_UNDEFINED,     // unknown
        MODE_BW,            // default B/W
        MODE_STD_CMAP,      // has a standard colormap
        MODE_TRUE,          // is a TrueColor/DirectColor visual
        MODE_MY_GRAY,       // my grayramp
        MODE_PALETTE        // has a pre-allocated palette
    };

    // Mime code
    enum mimeID
    {
        MIME_PLAIN,
        MIME_HTML,
        MIME_IMAGE
    };

    // Callback reason
    enum
    {
        HTM_ACTIVATE,
        HTM_LOSING_FOCUS,
        HTM_ARM
    };

    // Signal ID's
    enum SignalID
    {
        S_ARM,
        S_ACTIVATE,
        S_ANCHOR_TRACK,
        S_ANCHOR_VISITED,
        S_DOCUMENT,
        S_LINK,
        S_FRAME,
        S_FORM,
        S_IMAGEMAP,
        S_HTML_EVENT,
        S_LAST_SIGNAL
    };

    // Anchor styles
    enum AnchorStyle
    {
        ANC_PLAIN,
        ANC_BUTTON,
        ANC_SINGLE_LINE,
        ANC_DOUBLE_LINE,
        ANC_DASHED_LINE,
        ANC_DOUBLE_DASHED_LINE
    };

    // Scrollbar (etc.) mode
    //
    enum Availability
    {
        AUTOMATIC,
        ALWAYS,
        NEVER
    };

    // What type of scrolling a frame should employ
    enum FrameScrolling
    {
        FRAME_SCROLL_NONE = 1,
        FRAME_SCROLL_AUTO,
        FRAME_SCROLL_YES
    };

    // RGB conversion values:
    //
    // QUICK
    //      first checks if the 24bit image contains less than
    //      MaxImageColors.  If not, the widget will dither to a fixed
    //      palette.  This is fast but has the disadvantage that the
    //      background color in an alpha channelled image will not be
    //      matched exactly.
    //
    // BEST
    //      first checks if the 24bit image contains less than
    //      MaxImageColors.  If it is, the actual image colors are used.
    //      If not, a histogram of the image is computed, the most used
    //      colors are selected and the resulting image is dithered to this
    //      palette.  Offers best 24 to 8bit conversion and is probably
    //      faster than SLOW as only images with more than MaxImageColors
    //      will be dithered.
    //
    // FAST
    //      Skips the check and dithers to a fixed palette right away.
    //      This is the fastest way to do 24 to 8bit conversion but has the
    //      disadvantage that every 24bit image is dithered to a fixed
    //      palette, regardless of the actual no of colors in the image.
    //
    // SLOW
    //       Skips the check and does histogram stuff right away.
    //
    enum RGBconvType
    {
        QUICK,
        BEST,
        FAST,
        SLOW
    };

    // Return values from various image-related functions
    enum ImageStatus
    {
        IMAGE_ERROR_STATUS,
        IMAGE_UNKNOWN_STATUS,
        IMAGE_BAD,          // bad function call: missing arg or so
        IMAGE_ALMOST,       // action completed, further response necessary
        IMAGE_OK            // action completed
    };
}

using namespace htm;

//-----------------------------------------------------------------------------
// Callback function prototypes

class htmWidget;
struct htmGIFStream;
struct htmImageInfo;
struct htmRect;

// -- Image data handling.

// Progressive Loader Context stream
//
struct htmPLCStream
{
    htmPLCStream()
        {
            total_in    = 0;
            min_out     = 0;
            max_out     = 0;
            user_data   = 0;
        }

    unsigned int total_in;      // no of bytes received so far
    unsigned int min_out;       // minimum number of bytes requested
    unsigned int max_out;       // maximum number of bytes requested
    void *user_data;            // any data registered on this PLC
    unsigned char pad[24];      // reserved for future use
};

// Values for the third argument of htmEndDataProc
enum
{
    PLC_IMAGE,        // PLCObject for an image
    PLC_DOCUMENT,     // PLCObject for a document
    PLC_FINISHED      // indicates all plc's have been processed
};

// A subclass of this is used to communicate image and other
// information with the application.
//
struct htmDataInterface
{
    virtual ~htmDataInterface() { }

    // Dispatch function for "signals".  The first arguemnt is the
    // signal type, the second is a pointer to a data struct which
    // depends on the signal type.
    //
    virtual void emit_signal(SignalID, void*) = 0;

    // Procedure to be when a html-4 event needs handling.  The
    // argument is the keyword or script text.  The return value is a
    // pointer to data to be stored whenever a document event should
    // be processed.  This data is unused internally and is provided
    // as the user_data argument in the htmEvent structure.  For
    // example, the return value could be a pointer into some internal
    // procedural database, a ptr to a compiled script procedure or
    // the script source text if you want to process it at some later
    // time (when the event occurs).
    //
    // If 0 is returned the event in question is disabled.
    //
    virtual void *event_proc(const char*) = 0;

    // Fatal error callback function.
    //
    virtual void panic_callback(const char*) = 0;

    // Callback function to load images.
    //
    virtual htmImageInfo *image_resolve(const char*) = 0;

    // Progressive Loading functions.
    //
    virtual int get_image_data(htmPLCStream*, void*) = 0;
    virtual void end_image_data(htmPLCStream*, void*, int, bool) = 0;

    // Obtain the area that can be used for frames.
    //
    virtual void frame_rendering_area(htmRect*) = 0;

    // If in a frame, return the frame's name.
    //
    virtual const char *get_frame_name() = 0;

    // Return the topic keyword and title if available.  Returns copies.
    //
    virtual void get_topic_keys(char**, char**) = 0;

    // Scroll to make sure that the x,y given is visible.
    //
    virtual void scroll_visible(int, int, int, int) = 0;
};

// Declare callback structures defined in htm_callback.h
struct htmAnchorCallbackStruct;
struct htmVisitedCallbackStruct;
struct htmDocumentCallbackStruct;
struct htmLinkCallbackStruct;
struct htmImagemapCallbackStruct;
struct htmEventCallbackStruct;

// Declare callback structures defined in htm_frame.h
struct htmFrameCallbackStruct;

//-----------------------------------------------------------------------------
// Color Management

struct htmColor
{
    htmColor()
        {
            pixel   = 0;
            red     = 0;
            green   = 0;
            blue    = 0;
        }

    unsigned int pixel;
    unsigned short red;
    unsigned short green;
    unsigned short blue;
};

struct htmDefColors
{
    const char *DefFgLink;
    const char *DefFgVisitedLink;
    const char *DefFgTargetLink;
    const char *DefFgActiveLink;
    const char *DefBgActiveLink;
    const char *DefBg;
    const char *DefFgText;
    const char *DefBgSelect;
    const char *DefFgImagemap;
};

// This part of the widget manages the colors and palettes.
//
struct htmColorManager
{
    htmColorManager(htmWidget*);

    // colors.cc
    void initialize();
    void reset();
    unsigned int getPixelByName(const char*, unsigned int);
    void recomputeColors(unsigned int);
    void recomputeHighlightColor(unsigned int);
    int createColormap(htmColor*);
    bool tryColor(const char*, htmColor*);
    int anchor_fg_pixel(unsigned int);
    void shade_color(htmColor*, htmColor*, double);

    void set_anchor_fg(const char *clr)
    {
        cm_anchor_fg = cm_anchor_fg_save = getPixelByName(clr, 0);
        cm_anchor_target_fg = cm_anchor_target_fg_save =
            getPixelByName(clr, 0);
    }

    void set_anchor_visited_fg(const char *clr)
    {
        cm_anchor_visited_fg = cm_anchor_visited_fg_save =
            getPixelByName(clr, 0);
    }

    void set_anchor_activated_fg(const char *clr)
    {
        cm_anchor_activated_fg = cm_anchor_activated_fg_save =
            getPixelByName(clr, 0);
    }

    void set_body_bg(const char *clr)
    {
        cm_body_bg = cm_body_bg_save = getPixelByName(clr, 0);
    }

    void set_body_fg(const char *clr)
    {
        cm_body_fg = cm_body_fg_save = getPixelByName(clr, 0);
    }

    void set_select_bg(const char *clr)
    {
        cm_select_bg = cm_select_bg_save = getPixelByName(clr, 0);
    }

    void set_imagemap_fg(const char *clr)
    {
        cm_imagemap_fg = cm_imagemap_fg_save = getPixelByName(clr, 0);
    }

    htmWidget           *cm_html;

    // anchor colors
    unsigned int        cm_anchor_fg;
    unsigned int        cm_anchor_visited_fg;
    unsigned int        cm_anchor_target_fg;
    unsigned int        cm_anchor_activated_fg;

    // other colors
    unsigned int        cm_body_bg;             // current background color
    unsigned int        cm_body_fg;             // current foreground color
    unsigned int        cm_select_bg;           // text selection bacground
    unsigned int        cm_imagemap_fg;         // bounding box color
    unsigned int        cm_shadow_top;          // computed shadow color
    unsigned int        cm_shadow_bottom;       // computed shadow color
    unsigned int        cm_highlight;           // computed highlight color

    // fallback color values
    unsigned int        cm_anchor_fg_save;
    unsigned int        cm_anchor_visited_fg_save;
    unsigned int        cm_anchor_target_fg_save;
    unsigned int        cm_anchor_activated_fg_save;
    unsigned int        cm_body_bg_save;
    unsigned int        cm_body_fg_save;
    unsigned int        cm_select_bg_save;
    unsigned int        cm_imagemap_fg_save;
    bool                cm_colors_saved;

    unsigned int        cm_fullscale;           // full scale of pixel vals
    unsigned int        cm_halfscale;           // half scale of pixel vals

    static htmDefColors cm_defcolors;
};


//-----------------------------------------------------------------------------
// Format Management

// Bounding box for displayed objects.  The coordinate system has
// origin in the upper-left corner, with y increasing downward.  For
// text elements, the baseline is top() + font->ascent.
//
//
struct htmRect
{
    htmRect(int xx, int yy, unsigned ww, unsigned int hh)
        { x = xx; y = yy; width = ww; height = hh; area_start = true; }
    htmRect() { clear(); }

    void clear() { x = y = 0; width = height = 0; area_start = true; }
    int left() { return (x); }
    int bottom() { return (y + height); }
    int right() { return (x + width); }
    int top() { return (y); }
    bool intersect(htmRect &r)
        { return (right() > r.left() && left() < r.right() &&
          top() < r.bottom() && bottom() > r.top()); }
    bool no_intersect(htmRect &r)
        { return (right() < r.left() || left() > r.right() ||
          top() > r.bottom() || bottom() < r.top()); }
    bool includes(int px, int py)
        { return (px >= left() && px <= right() &&
          py <= bottom() && py >= top()); }
    void start_area() { area_start = true; }
    void add(htmRect &r)
        {
            if (area_start) {
                area_start = false;
                x = r.x;
                y = r.y;
                width = r.width;
                height = r.height;
                return;
            }

            int rtmp = right();
            int btmp = bottom();
            if (left() > r.left())
                x = r.left();
            if (top() > r.top())
                y = r.top();
            if (rtmp < r.right())
                rtmp = r.right();
            if (btmp < r.bottom())
                btmp = r.bottom();
            width = rtmp - x;
            height = btmp - y;
        }

    int x;
    int y;
    unsigned int width;
    unsigned int height;
    bool area_start;
};

struct htmFontSizes
{
    htmFontSizes()
        {
            fixed_normal    = 12;
            tiny            = 8;
            small           = 10;
            normal          = 12;
            h4              = 14;
            h3              = 16;
            h2              = 18;
            h1              = 22;
        }

    void set_scaled(const htmFontSizes *fs, int sz)
        {
            fixed_normal    = (fs->fixed_normal*sz)/fs->normal;
            tiny            = (fs->tiny*sz)/fs->normal;
            small           = (fs->small*sz)/fs->normal;
            normal          = sz;
            h4              = (fs->h4*sz)/fs->normal;
            h3              = (fs->h3*sz)/fs->normal;
            h2              = (fs->h2*sz)/fs->normal;
            h1              = (fs->h1*sz)/fs->normal;
        }

    int font_size(int html_size, bool fixed)
        {
            if (html_size < 1)
                html_size = 1;
            else if (html_size > 7)
                html_size = 7;
            switch (html_size) {
            case 1:
                return (tiny);
            case 2:
                return (small);
            case 3:
                break;
            case 4:
                return (h4);
            case 5:
                return (h3);
            case 6:
                return (h2);
            case 7:
                return (h1);
            }
            return (fixed ? fixed_normal : normal);
        }

    unsigned char fixed_normal;
    unsigned char tiny;
    unsigned char small;
    unsigned char normal;
    unsigned char h4;
    unsigned char h3;
    unsigned char h2;
    unsigned char h1;
};


//-----------------------------------------------------------------------------
// Image Management

typedef void htmPixmap;   // toolkit-specific pixmap format
typedef void htmBitmap;   // toolkit-specific bitmap format
typedef void htmXImage;   // toolkit-specific image format

struct htmAnchor;
struct htmFont;
struct htmFormatManager;
struct htmImage;
struct htmImageMap;
struct htmObject;
struct htmObjectTable;
struct htmRawImageData;
namespace htm
{
    struct ImageBuffer;
    struct PLC;
    struct AlphaChannelInfo;
    struct GIFscreen;
}

// This part of the widget manages images.
//
struct htmImageManager
{
    htmImageManager(htmWidget*);
    ~htmImageManager();

    bool imageJPEGSupported()
        {
#ifdef HAVE_LIBJPEG
            return (true);
#else
            return (false);
#endif
        }

    bool imagePNGSupported()
        {
#ifdef HAVE_LIBPNG
            return (true);
#else
            return (false);
#endif
        }

    bool imageGZFSupported(void)
        {
#if defined(HAVE_LIBPNG) || defined(HAVE_LIBZ)
            return (true);
#else
            return (false);
#endif
        }

    // htm_image.cc
    void initialize();
    void freeResources(bool);
    htmImageInfo *imageLoadProc(const char*);
    unsigned char imageGetType(const char*);
    ImageStatus imageReplace(htmImageInfo*, htmImageInfo*);
    ImageStatus imageUpdate(htmImageInfo*);
    void imageFreeAllImages();
    void imageAddImageMap(const char*);
    void imageFreeImageInfo(htmImageInfo*);
    int imageGetImageInfoSize(htmImageInfo*);
    unsigned char getImageType(ImageBuffer*);
    ImageBuffer *imageFileToBuffer(const char*);
    htmImage *newImage(const char*, unsigned int*, unsigned int*);
    void loadBodyImage(const char*);
    void imageUpdateChildren(htmImage*);
    void imageCheckDelayedCreation();
    void makeAnimation(htmImage*, unsigned int, unsigned int);
    htmPixmap *infoToPixmap(htmImage*, htmImageInfo*,
        unsigned int, unsigned int, unsigned int*, htmBitmap**);
    ImageStatus replaceOrUpdateImage(htmImageInfo*,
        htmImageInfo*, htmObjectTable**);
    void freeImage(htmImage*);
    void freeImageInfo(htmImageInfo*);
    void releaseImage(htmImage*);
    htmRawImageData *readImage(ImageBuffer*);
    void clipImage(htmImageInfo*, unsigned int, unsigned int);
    void scaleImage(htmImageInfo*, unsigned int, unsigned int);
    int getMaxColors(int);
    void getImageAttributes(htmImage*, const char*);
    htmImage *copyImage(htmImage*, const char*);
    htmImage *lookForImage(const char*, const char*, unsigned int*,
        unsigned int*, int, int);
    void addImageToList(htmImage*);
    htmImageInfo *defaultImage(const char*, int, bool, int, int,
        unsigned char*);
    htmImageInfo *imageDefaultProc(htmRawImageData*, const char*);
    htmImageInfo *animDefaultProc(htmRawImageData*, htmRawImageData*, int*,
        int*, bool, const char*);
    htmImageInfo *imageDelayedProc(htmRawImageData*, ImageBuffer*);
    htmImageInfo *readGifAnimation(ImageBuffer*);
    htmImage *copyHTMLImage(htmImage*, const char*);
    void initAlphaChannels(bool);
    void doAlphaChannel(htmImage*);
    void processBodyImage(htmImage*, unsigned int, unsigned int);

    // htm_imagemap.cc
    void storeImagemap(htmImageMap*);
    void addAreaToMap(htmImageMap*, htmObject*, htmFormatManager*);
    htmImageMap *getImagemap(const char*);
    htmAnchor *getAnchorFromMap(int, int, htmImage*, htmImageMap*);
    void checkImagemaps();
    void freeImageMaps();
    void drawImagemapSelection(htmImage*);

    // htm_plc.cc
    bool callPLC(const char*);
    void imageProgressiveSuspend();
    void imageProgressiveContinue();
    void imageProgressiveKill();
    PLC *PLCCreate(htmImageInfo*, const char*);
    void killPLCCycler();

    // htm_quantize.cc
    void convert24to8(unsigned char*, htmRawImageData*, int, unsigned char);
    void quantizeImage(htmRawImageData*, int);
    void pixelizeRGB(unsigned char*, htmRawImageData*);

    // htm_BM.cc
    htmRawImageData *readBitmap(ImageBuffer*);

    // htm_FLG.cc
    htmImageInfo *readFLG(ImageBuffer*);
    htmImageInfo *readFLGprivate(ImageBuffer*);

    // htm_GIF.cc
    int isGifAnimated(ImageBuffer*);
    void gifAnimTerminate(ImageBuffer*);
    htmRawImageData *gifAnimInit(ImageBuffer*, int*, GIFscreen**);
    htmRawImageData *gifAnimNextFrame(ImageBuffer*, int*, int*, int*, int*,
        GIFscreen*);
    htmRawImageData *readGIF(ImageBuffer*);
    unsigned char *inflateGIFInternal(ImageBuffer*, int, int*);
    unsigned char *inflateGIFExternal(ImageBuffer*, int, int*);
    unsigned char *inflateRaster(htmImageManager*, ImageBuffer*, int, int,
        int*);
    unsigned char *inflateGZFInternal(ImageBuffer*, int, int*);
    bool GIFtoGZF(const char*, unsigned char*, int, const char*);
    bool cvtGifToGzf(ImageBuffer*, const char*);

    // htm_JPEG.cc
    htmRawImageData *readJPEG(ImageBuffer*);

    // htm_PPM.cc
    htmRawImageData *readPPM(ImageBuffer*);

    // htm_PNG.cc
    htmRawImageData *readPNG(ImageBuffer*);
    htmRawImageData *reReadPNG(htmRawImageData*, int, int, bool);

    // htm_TIFF.cc
    htmRawImageData *readTIFF(ImageBuffer*);

    // htm_XPM.cc
    htmRawImageData *readXPM(ImageBuffer*);
    htmRawImageData *createXpmFromData(const char* const*, const char*);

    htmWidget           *im_html;

    htmImage            *im_body_image;    // background image data
    htmImage            *im_images;        // list of images in current doc
    bool                im_delayed_creation; // delayed image creation

    // Imagemap resources
    htmImageMap         *im_image_maps;    // array of client-side imagemaps

    // Progressive Loader Context buffer and interval
    PLC                 *im_plc_buffer;     // PLC ringbuffer
    int                 im_num_plcs;        // no of PLC's in ringbuffer
    bool                im_plc_suspended;   // Global PLC suspension flag

    // Internal stuff for alpha channelled PNG images
    AlphaChannelInfo    *im_alpha_buffer;

    static unsigned char im_bitmap_bits[8]; // htm_BM.cc
};


//-----------------------------------------------------------------------------
// Rendering and Graphics Control

struct htmForm;
struct htmWord;

struct htmPoint
{
    htmPoint() { x = 0; y = 0; }
    htmPoint(int xx, int yy) { x = xx; y = yy; }

    int x;
    int y;
};

struct htmPoly
{
    htmPoly() { points = 0; numpts = 0; }
    ~htmPoly() { delete [] points; }

    bool intersect(int, int);

    htmPoint *points;
    int numpts;
};

// This is the base class for the external toolkit-specific functions.
//
class htmInterface
{
public:
    // For tk_set_line_style.
    enum FillMode
    {
        SOLID,
        TILED
    };

    // For tk_window_size.
    enum WinRetMode
    {
        VIEWABLE,
        DRAWABLE
    };

    virtual ~htmInterface() { }

    virtual void tk_resize_area(int, int) = 0;
    virtual void tk_refresh_area(int, int, int, int) = 0;
    virtual void tk_window_size(WinRetMode, unsigned int*, unsigned int*) = 0;
    virtual unsigned int tk_scrollbar_width() = 0;
    virtual void tk_set_anchor_cursor(bool) = 0;
    virtual unsigned int tk_add_timer(int(*)(void*), void*) = 0;
    virtual void tk_remove_timer(int) = 0;
    virtual void tk_start_timer(unsigned int, int) = 0;
    virtual void tk_claim_selection(const char*) = 0;

    virtual htmFont *tk_alloc_font(const char*, int, unsigned char) = 0;
    virtual void tk_release_font(void*) = 0;
    virtual void tk_set_font(htmFont*) = 0;
    virtual int tk_text_width(htmFont*, const char*, int) = 0;

    virtual CCXmode tk_visual_mode() = 0;
    virtual int tk_visual_depth() = 0;
    virtual htmPixmap *tk_new_pixmap(int, int) = 0;
    virtual void tk_release_pixmap(htmPixmap*) = 0;
    virtual htmPixmap *tk_pixmap_from_info(htmImage*, htmImageInfo*,
        unsigned int*) = 0;
    virtual void tk_set_draw_to_pixmap(htmPixmap*) = 0;
    virtual htmBitmap *tk_bitmap_from_data(int, int, unsigned char*) = 0;
    virtual void tk_release_bitmap(htmBitmap*) = 0;
    virtual htmXImage *tk_new_image(int, int) = 0;
    virtual void tk_fill_image(htmXImage*, unsigned char*, unsigned int*,
        int, int) = 0;
    virtual void tk_draw_image(int, int, htmXImage*, int, int, int, int) = 0;
    virtual void tk_release_image(htmXImage*) = 0;

    virtual void tk_set_foreground(unsigned int) = 0;
    virtual void tk_set_background(unsigned int) = 0;
    virtual bool tk_parse_color(const char*, htmColor*) = 0;
    virtual bool tk_alloc_color(htmColor*) = 0;
    virtual int tk_query_colors(htmColor*, unsigned int) = 0;
    virtual void tk_free_colors(unsigned int*, unsigned int) = 0;
    virtual bool tk_get_pixels(unsigned short*, unsigned short*,
        unsigned short*, unsigned int, unsigned int*) = 0;

    virtual void tk_set_clip_mask(htmPixmap*, htmBitmap*) = 0;
    virtual void tk_set_clip_origin(int, int) = 0;
    virtual void tk_set_clip_rectangle(htmRect*) = 0;
    virtual void tk_draw_pixmap(int, int, htmPixmap*, int, int, int, int) = 0;
    virtual void tk_tile_draw_pixmap(int, int, htmPixmap*, int, int, int, int)
        = 0;

    virtual void tk_draw_rectangle(bool, int, int, int, int) = 0;
    virtual void tk_set_line_style(FillMode) = 0;
    virtual void tk_draw_line(int, int, int, int) = 0;
    virtual void tk_draw_text(int, int, const char*, int) = 0;
    virtual void tk_draw_polygon(bool, htmPoint*, int) = 0;
    virtual void tk_draw_arc(bool, int, int, int, int, int, int) = 0;

    // forms handling
    virtual void tk_add_widget(htmForm*, htmForm* = 0) = 0;
    virtual void tk_select_close(htmForm*) = 0;
    virtual char *tk_get_text(htmForm*) = 0;
    virtual void tk_set_text(htmForm*, const char*) = 0;
    virtual bool tk_get_checked(htmForm*) = 0;
    virtual void tk_set_checked(htmForm*) = 0;
    virtual void tk_position_and_show(htmForm*, bool) = 0;
    virtual void tk_form_destroy(htmForm*) = 0;
};


//-----------------------------------------------------------------------------
// Widget Class

struct htmTableProperties;
struct htmFormData;
struct htmFrameWidget;
struct htmFontTab;
struct htmHeadAttributes;
struct htmInfoStructure;
struct htmTable;
struct htmTableCell;
namespace htm
{
    struct HTEvent;
    struct AllEvents;
}

// htmXYToInfo return value
//
struct htmInfoStructure
{
    htmInfoStructure()
        {
            line    = 0;
            is_map  = false;
            x = y   = 0;
            image   = 0;
            anchor  = 0;
        }

    unsigned line;              // line number at selected position
    bool is_map;                // true when clicked image is an imagemap
    int x,y;                    // position relative to image corner
    htmImageInfo *image;        // image data
    htmAnchor *anchor;          // possible anchor data
};

// Selected words
//
struct htmSelection
{
    htmSelection()
        {
            start           = 0;
            start_word      = 0;
            start_nwords    = 0;

            end             = 0;
            end_word        = 0;
            end_nwords      = 0;
        }

    htmObjectTable *start;          // selection start object
    int start_word;                 // first word index
    int start_nwords;               // word count

    htmObjectTable *end;            // ending start object (inclusive)
    int end_word;                   // first word index
    int end_nwords;                 // word count
};

// Defined in htm_text.cc.
struct htmSearch;

// Finally, the widget class.
//
class htmWidget
{
public:
    htmWidget(htmInterface*, htmDataInterface*);
    virtual ~htmWidget();

    // public methods, in public.cc

    // Anchor resources
    void setAnchorStyle(AnchorStyle);
    void setAnchorVisitedStyle(AnchorStyle);
    void setAnchorTargetStyle(AnchorStyle);
    void setHighlightOnEnter(bool);
    void setAnchorCursorDisplay(bool);

    AnchorStyle anchorStyle() { return (htm_anchor_style); }
    AnchorStyle anchorVisitedStyle() { return (htm_anchor_visited_style); }
    AnchorStyle anchorTargetStyle() { return (htm_anchor_target_style); }
    bool highlightOnEnter() { return (htm_highlight_on_enter); }
    bool anchorCursorDisplay() { return (htm_anchor_display_cursor); }

    // Color resources
    void setAllowBodyColors(bool);
    void setAllowColorSwitching(bool);

    bool allowBodyColors() { return (htm_body_colors_enabled); }
    bool allowColorSwitching() { return (htm_allow_color_switching); }

    htmImageInfo *imageLoadProc(const char*);

    // Display control resources
    void freeze();
    void thaw();
    void redisplay();
    void set_anchor_fg(const char *clr)
        { htm_cm.set_anchor_fg(clr); }
    void set_anchor_visited_fg(const char *clr)
        { htm_cm.set_anchor_visited_fg(clr); }
    void set_anchor_activated_fg(const char *clr)
        { htm_cm.set_anchor_activated_fg(clr); }
    void set_body_bg(const char *clr)
        { htm_cm.set_body_bg(clr); }
    void set_body_fg(const char *clr)
        { htm_cm.set_body_fg(clr); }
    void set_select_bg(const char *clr)
        { htm_cm.set_select_bg(clr); }
    void set_imagemap_fg(const char *clr)
        { htm_cm.set_imagemap_fg(clr); }

    // On startup, trySync will fail until this is set true. 
    // Rendering may require that the widgets be realized.  This can
    // be called from an expose event handler.
    void setReady() { if (!htm_ready) { htm_ready = true; trySync(); } }
    bool isReady() { return (htm_ready); }

    // coordinates relative to top/left of visible area
    int viewportX(int x) { return (x - htm_viewarea.x); }
    int viewportY(int y) { return (y - htm_viewarea.y); }

    // coordinates relative to document (arguments are
    // viewport coordinates)
    int windowX(int x) { return (x + htm_viewarea.x); }
    int windowY(int y) { return (y + htm_viewarea.y); }

    // Document resources
    void setMimeType(const char*);
    void setBadHtmlWarnings(bool);
    void setAlignment(TextAlignment);
    void setOutline(bool);
    void setVmargin(unsigned int);
    void setHmargin(unsigned int);
    void setSource(const char*);
    const char *getSource();
    char *getString();
    const char *getVersion();
    char *getTitle();

    const char *mimeType() { return (htm_mime_type); }
    bool badHtmlWarnings() { return (htm_bad_html_warnings); }
    TextAlignment alignment() { return ((TextAlignment)htm_alignment); }
    bool outline() { return (htm_enable_outlining); }
    unsigned int vMargin() { return (htm_margin_height); }
    unsigned int hMargin() { return (htm_margin_width); }

    // Font resources
    void setStringRtoL(bool);
    void setAllowFontSwitching(int);
    void setFontFamily(const char*, int);
    void setFixedFontFamily(const char*, int);
    void setFontSizes(htmFontSizes*);

    bool stringRtoL() { return (htm_string_r_to_l); }
    bool allowFontSwitching() { return (htm_allow_font_switching); }
    const char *fontFamily() { return (htm_font_family); }
    const htmFontSizes *fontSizes() { return (&htm_font_sizes); }
    const char *fixedFontFamily() { return (htm_font_family_fixed); }

    // Form resources
    void setAllowFormColoring(bool);

    bool allowFormColoring() { return (htm_allow_form_coloring); }

    // Frame resources

    // Image resources
    void setImagemapDraw(int);
    void setAllowImages(bool);
    void setRgbConvMode(RGBconvType);
    void setPerfectColors(Availability);
    void setAlphaProcessing(Availability);
    void setScreenGamma(float);
    void setUncompressCommand(const char*);
    void setFreezeAnimations(bool);

    bool ImagemapDraw() { return (htm_imagemap_draw); }
    bool allowImages() { return (htm_images_enabled); }
    int maxImageColors() { return (htm_max_image_colors); }
    RGBconvType rgbConvMode() { return (htm_rgb_conv_mode); }
    Availability perfectColors() { return (htm_perfect_colors); }
    Availability alphaProcessing() { return (htm_alpha_processing); }
    float screenGamma() { return (htm_screen_gamma); }
    const char *uncompressCommand() { return (htm_zCmd); }
    bool freezeAnimations() { return (htm_freeze_animations); }

    // Scrollbar resources
    int anchorPosByName(const char*);
    int anchorPosById(int);

    // Table resources

    // Text selection resources
    void selectRegion(int, int, int, int);

    // Misc.

    // error.cc
    void warning(const char*, const char*, ...);
    void fatalError(const char*, ...);

    // callback.cc
    bool getHeadAttributes(htmHeadAttributes*, unsigned char);
    void linkCallback();
    void trackCallback(htmEvent*, htmAnchor*);
    void activateCallback(htmEvent*, htmAnchor*);
    bool documentCallback(bool, bool, bool, bool, int);

    // events.cc
    AllEvents *checkCoreEvents(const char*);
    AllEvents *checkBodyEvents(const char*);
    AllEvents *checkFormEvents(const char*);
    void processEvent(htmEvent*, HTEvent*);
    void freeEventDatabase();
    HTEvent *storeEvent(int, void*);
    HTEvent *checkEvent(int, const char*);

    // fonts.cc
    htmFont *findFont(const char*, int, unsigned char);
    htmFont *loadDefaultFont();
    htmFont *loadFont(htmlEnum, int, htmFont*);
    htmFont *loadFontWithFace(int, const char*, htmFont*);

    // format.cc
    void format();
    void parseBodyTags(htmObject*);
    htmAnchor *newAnchor(htmObject*, htmFormatManager*);

    // forms.cc
    void destroyForms();
    void startForm(const char*);
    void endForm();
    htmForm *formAddInput(const char*);
    htmForm *formAddSelect(const char*);
    void formSelectAddOption(htmForm*, const char*, const char*);
    void formSelectClose(htmForm*);
    htmForm *formAddTextArea(const char*, const char*);
    void formActivate(htmEvent*, htmForm*);
    void formReset(htmForm*);
    void positionAndShowForms();

    // frames.cc
    int checkForFrames(htmObject*);
    bool configureFrames();
    void destroyFrames();
    void makeFrameSets(htmObject*);

    // htm_layout.cc
    void computeLayout();

    // htm_paint.cc
    void animTimerHandler(htmImage*);
    void paint(htmObjectTable*, htmObjectTable*);
    void setup_selection(htmObjectTable*, htmWord*, htmWord*);
    void restartAnimations();
    void drawImage(htmObjectTable*, int, bool, htmRect* = 0);
    void drawImageAnchor(htmObjectTable*);
    void drawFrame(htmImage*, htmObjectTable*, int, int);
    void drawText(htmObjectTable*, htmRect* = 0);
    htmObjectTable *drawAnchor(htmObjectTable*, htmObjectTable*, htmRect* = 0);
    void drawRule(htmObjectTable*, htmRect* = 0);
    void drawBullet(htmObjectTable*, htmRect* = 0);
    void drawShadows(int, int, unsigned int, unsigned int, bool);
    htmObjectTable *drawTable(htmObjectTable*, htmObjectTable*, htmRect* = 0);
    void drawCellContent(htmTableCell*, int, int);
    void drawCellFrame(htmTableCell*, int, int);
    void drawTableBorder(htmTable*);

    // htm_parser.cc
    void parseInput();
    void setMimeId();
    char *getObjectString();

    // htm_table.cc
    htmTableProperties *tableCheckProperties(const char*,
        htmTableProperties*, Alignment, unsigned int, htmImage*);

    // htm_text.cc
    char *getPostscriptText(int, const char*, const char*, bool, bool);
    char *getPlainText();
    bool findWords(const char*, bool, bool);
    void clearSearch();

    // htm_widget.cc
    void initialize();
    void trySync();
    void parse();
    void reformat();
    void layout();
    void resize();
    void redraw();
    void repaint(int, int, int, int);
    void paintAnchorSelected(htmObjectTable*);
    void paintAnchorUnSelected();
    void enterAnchor(htmObjectTable*);
    void leaveAnchor();
    htmInfoStructure *XYToInfo(int, int);
    htmImage* onImage(int, int);
    htmObjectTable* getLineObject(int);
    int verticalPosToLine(int);
    htmWord* getAnchor(int, int);
    htmObjectTable *getAnchorByName(const char*);
    htmObjectTable *getAnchorByValue(int);
    htmAnchor* getImageAnchor(int, int);
    int anchorGetId(const char*);
    void anchorTrack(htmEvent*, int, int);
    void extendStart(htmEvent*, int, int, int);
    void extendEnd(htmEvent*, int, bool, int, int);
    void selection();
    void selectionBB(int*, int*, int*, int*);

    // Anchor resources
    // ------------------------------------------------------------------------
    bool                htm_anchor_display_cursor;
    bool                htm_highlight_on_enter; // anchor highlighting
    int                 htm_anchor_position_x;  // for server-side imagemaps
    int                 htm_anchor_position_y;  // for server-side imagemaps
    htmObjectTable      *htm_armed_anchor;      // current anchor
    htmAnchor           *htm_anchor_current_cursor_element;

    // anchor display styles
    AnchorStyle         htm_anchor_style;
    AnchorStyle         htm_anchor_visited_style;
    AnchorStyle         htm_anchor_target_style;

    // anchor activation resources
    int                 htm_press_x;            // ptr coordinates
    int                 htm_press_y;
    htmAnchor           *htm_selected_anchor;   // selected anchor
    htmObjectTable      *htm_current_anchor;    // selected anchor object

    // Color resources
    // ------------------------------------------------------------------------
    bool                htm_body_colors_enabled;
    bool                htm_body_images_enabled;
    bool                htm_allow_color_switching;
    bool                htm_allow_form_coloring;    // body colors on HTML forms
    bool                htm_freeze_animations;
    char                *htm_body_image_url;    // background image location
    char                *htm_def_body_image_url; // default bg image location
    htmColorManager     htm_cm;

    // Document resources
    // ------------------------------------------------------------------------
    char                *htm_source;     // source text
    char                *htm_mime_type;  // mime type of this text/image
    unsigned char       htm_mime_id;     // internal mime id
    bool                htm_enable_outlining;
    unsigned char       htm_bad_html_warnings;

    // Event and callback resources
    // ------------------------------------------------------------------------
    HTEvent             *htm_events;            // HTML4.0 event data
    int                 htm_nevents;            // no of events watched

    // Formatted document resources
    // ------------------------------------------------------------------------
    unsigned int        htm_formatted_width;    // total width of document
    unsigned int        htm_formatted_height;   // total height of document
    unsigned int        htm_nlines;             // no of lines in document
    htmObjectTable      *htm_formatted;         // display object data
    htmObject           *htm_elements;          // unfiltered parser output

    int                 htm_num_anchors;        // total no of anchors in doc
    int                 htm_num_named_anchors;  // total no of named anchors
    int                 htm_anchor_words;       // total no of anchor words
    htmWord             **htm_anchors;          // for quick anchor lookup
    htmObjectTable      **htm_named_anchors;    // for named anchor lookup
    htmAnchor           *htm_anchor_data;       // parsed anchor data

    htmObjectTable      *htm_paint_start;       // first paint command
    htmObjectTable      *htm_paint_end;         // last paint command
    htmRect             htm_paint;              // paint clip area
    htmRect             htm_last_paint;         // actual painted area

    // Font resources
    // ------------------------------------------------------------------------
    char                *htm_font_family;
    char                *htm_font_family_fixed;
    htmFont             *htm_default_font;
    htmFontTab          *htm_font_cache;
    const htmFontSizes  htm_default_font_sizes;
    htmFontSizes        htm_font_sizes;
    bool                htm_string_r_to_l;
    bool                htm_allow_font_switching;
    TextAlignment       htm_alignment;
    Alignment           htm_default_halign;
    static const unsigned char htm_def_font_sizes[8];

    // Form resources
    // ------------------------------------------------------------------------
    htmFormData         *htm_form_data;     // all forms in the current document
    htmFormData         *htm_current_form;
    htmForm             *htm_current_entry;

    // Frame resources
    // ------------------------------------------------------------------------
    int                 htm_frame_border;       // add a border to the frames?
    int                 htm_nframes;            // number of frames
    htmFrameWidget      **htm_frames;           // list of frames
    htmFrameWidget      *htm_frameset;          // top frameset

    // Image resources
    // ------------------------------------------------------------------------
    bool                htm_images_enabled;     // true -> show images
    bool                htm_imagemap_draw;      // draw imagemap bounding boxes
    int                 htm_max_image_colors;   // 0 -> as much as possible
    RGBconvType         htm_rgb_conv_mode;      // 24 to 8bit conversion method
    Availability        htm_perfect_colors;     // perform final dithering
    Availability        htm_alpha_processing;   // do alpha channel stuff?
    float               htm_screen_gamma;       // gamma correction for display
    char                *htm_zCmd;              // uncompress command for LZW
    htmImageManager     htm_im;

    // Table resources
    // ------------------------------------------------------------------------
    htmTable            *htm_tables;            // list of all tables

    // Text selection and search resources
    // ------------------------------------------------------------------------
    char                *htm_text_selection;
    htmRect             htm_select;
    htmSearch           *htm_search;

    // Misc. Resources
    // ------------------------------------------------------------------------
    htmInterface        *htm_tk;                // pointer to class used
                                                // for graphics rendering
    htmDataInterface    *htm_if;                // pointer to class used
                                                // for image callbacks

    unsigned int        htm_margin_width;       // document margins
    unsigned int        htm_margin_height;
    htmRect             htm_viewarea;           // visible area

    // The "window" for all display objects is given by the formatted
    // width and height.  The viewarea can represent the currently
    // visible portion of this.  We have to support these two
    // operating strategies:
    //
    // 1) The viewarea is always the same as the formatted area, and
    // scrolling is performed outside the scope of this code.  That
    // is, the display pixmap is the same size as the formatted area.
    //
    // 2) The viewarea is the size of the actual visible screen area, and
    // scrolling is accomplihed by updating the display in this area only.

    // Rules (unless explicitly noted otherwise):
    //   All htmInterface (tk_xxx) function arguments representing
    //   coordinates are viewport coordinates.
    //
    //   All other function arguments representing coordinates are
    //   window coordinates.  All coordinates saved in data structs are
    //   window coordinates.

    bool                htm_in_layout;          // layout blocking flag.

    bool                htm_ready;
    bool                htm_initialized;        // state flags
    bool                htm_parse_needed;
    bool                htm_new_source;
    bool                htm_reformat_needed;
    bool                htm_layout_needed;
    bool                htm_redraw_needed;
    bool                htm_free_images_needed;
    int                 htm_frozen;
};

inline int
ul_type(AnchorStyle type)
{
    switch (type) {
    case ANC_PLAIN:
        return (NO_LINE);
    case ANC_BUTTON:
        return (ALT_STYLE);
    case ANC_DASHED_LINE:
        return (LINE_DASHED | LINE_UNDER);
    case ANC_DOUBLE_LINE:
        return (LINE_DOUBLE | LINE_UNDER);
        break;
    case ANC_DOUBLE_DASHED_LINE:
        return (LINE_DASHED | LINE_DOUBLE | LINE_UNDER);
    case ANC_SINGLE_LINE:
    default:
        break;
    }
    return (LINE_UNDER);
}

extern char *htmPlainTextFromHTML(const char*, int, int);

#endif


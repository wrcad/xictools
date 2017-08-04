
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
#include <algorithm>
#include <stdlib.h>

// RANGE forces a to be in the range b..c (inclusive)
#define RANGE(a, b, c) { if (a < b) a = b;  if (a > c) a = c; }

namespace htm
{
    struct pixel
    {
        unsigned char r;
        unsigned char g;
        unsigned char b;
    };
}

namespace {
    void my_bcopy(char*, char*, size_t);
    bool quick_rgb(unsigned char*, htmRawImageData*, int);
    void quick_quantize(unsigned char*, htmRawImageData*);
    int ppm_quant(unsigned char*, pixel**, htmRawImageData*, int);
}

// Defines and macros for quick 24bit (RGB) image to 8bit (paletted)
// image We use a 3/3/2 colorcube.
//
#define RMASK       0xe0
#define RSHIFT      0
#define GMASK       0xe0
#define GSHIFT      3
#define BMASK       0xc0
#define BSHIFT      6

// division speedup: multiply by this instead of dividing by 16
#define SIXTEENTH   0.0625


// Transform a 24bit RGB image to an 8bit paletted image.
//
void
htmImageManager::convert24to8(unsigned char *data, htmRawImageData *img_data,
    int max_colors, unsigned char mode)
{
    bool done = false;

    // If this image isn't RGB, there's a good chance that this image
    // has less than 256 colors in it.  So we first make a quick check
    // to see if this is indeed true.  If true, this will produce the
    // best possible results as no quantization is performed.

    if ((mode == BEST || mode == QUICK) &&
            img_data->color_class != IMAGE_COLORSPACE_RGB)
        done = quick_rgb(data, img_data, max_colors);

    if (!done) {
        if (mode == BEST || mode == SLOW)
            ppm_quant(data, 0, img_data, max_colors);
        else
            quick_quantize(data, img_data);
    }
}


// Quantizes an image to max_colors.
//
void
htmImageManager::quantizeImage(htmRawImageData *img_data, int max_colors)
{
    // reformat image data into a 2D array of pixel values
    pixel **pixels = new pixel*[img_data->height];
    unsigned char *ptr = img_data->data;

    for (int row = 0; row < img_data->height; row++) {
        pixels[row] = new pixel[img_data->width];

        int col;
        pixel *pP;
        for (col = 0, pP = pixels[row]; col < img_data->width; col++, pP++) {
            pP->r = img_data->cmap[*ptr].red;
            pP->g = img_data->cmap[*ptr].green;
            pP->b = img_data->cmap[*ptr++].blue;
        }
    }
    // quantize it
    ppm_quant(0, pixels, img_data, max_colors);

    // no need to free pixels, ppm_quant does that for us
}


// Convert RGB data to paletted data, don't do any quantizing.
// img_data contains a valid colormap (with possibly more than
// MAX_IMAGE_COLORS colors) and the data field has been pixelized.
//
void
htmImageManager::pixelizeRGB(unsigned char *rgb, htmRawImageData *img_data)
{
    int width  = img_data->width;
    int height = img_data->height;

    // initialize colors array
    unsigned int max_colors = MAX_IMAGE_COLORS;
    unsigned int *colors = new unsigned int[max_colors];

    // put the first color in the table by hand
    int num_colors = 0;
    int mid = 0;

    int i;
    unsigned char *p;
    unsigned int col;
    for (i = width*height, p = rgb; i; i--) {
        // make truecolor pixel val
        col  = (((unsigned int) *p++) << 16);
        col += (((unsigned int) *p++) << 8);
        col +=  *p++;

        // binary search the 'colors' array to see if it's in there
        int low = 0;
        int high = num_colors - 1;
        while (low <= high) {
            mid = (low+high)/2;
            if (col < colors[mid])
                high = mid - 1;
            else if (col > colors[mid])
                low = mid + 1;
            else
                break;
        }

        if (high < low) {
            // didn't find color in list, add it
            if (num_colors >= (int)max_colors) {
                // enlarge colors array
                max_colors *= 2;
                unsigned int *tc = colors;
                colors = new unsigned int[max_colors];
                memcpy(colors, tc, (max_colors/2) * sizeof(unsigned int));
                delete [] tc;
            }
            my_bcopy((char *)&colors[low], (char*)&colors[low+1],
                (num_colors - low) * sizeof(unsigned int));
            colors[low] = col;
            num_colors++;
        }
    }

    // destination buffer
    if (img_data->data == 0)
        img_data->data = new unsigned char[width*height];

    // Pixelize data: map pixel values of the RGB image in the colormap
    unsigned char *pix;
    for (i = width*height, p = rgb, pix = img_data->data; i; i--,pix++) {
        col  = (((unsigned int)*p++) << 16);
        col += (((unsigned int)*p++) << 8);
        col +=  *p++;

        // binary search the 'colors' array.  It IS in there
        int low = 0;
        int high = num_colors - 1;
        while (low <= high) {
            mid = (low+high)/2;
            if (col < colors[mid])
                high = mid - 1;
            else if (col > colors[mid])
                low = mid + 1;
            else
                break;
        }
        *pix = mid;
    }

    // allocate colormap
    img_data->allocCmap(num_colors);

    // and fill it
    for (i = 0; i < num_colors; i++) {
        img_data->cmap[i].red   = ( colors[i] >> 16);
        img_data->cmap[i].green = ((colors[i] >> 8 ) & 0xff);
        img_data->cmap[i].blue  = ( colors[i]  & 0xff);
    }
    // no longer needed
    delete [] colors;
}


namespace {
    // Safe bcopy, areas may overlap.
    //
    void
    my_bcopy(char *src, char *dst, size_t len)
    {
        // areas are the same
        if (src == dst || len <= 0)
            return;

        if (src < dst && src+len > dst) {
            // do a backward copy
            src = src + len - 1;
            dst = dst + len - 1;
            for ( ; len > 0; len--, src--, dst--)
                *dst = *src;
        }
        else {
            // do a forward copy
            for ( ; len > 0; len--, src++, dst++)
                *dst = *src;
        }
    }


    // Attemp a quick 24 to 8 bit image conversion.
    //
    bool
    quick_rgb(unsigned char *rgb, htmRawImageData *img_data, int max_colors)
    {
        int width  = img_data->width;
        int height = img_data->height;

        // put the first color in the table by hand
        int num_colors = 0;
        int mid = 0;

        int i;
        unsigned char *p;
        unsigned int colors[MAX_IMAGE_COLORS];
        for (i = width*height, p = rgb; i; i--) {
            // make truecolor pixel val
            int col  = (((unsigned int) *p++) << 16);
            col += (((unsigned int) *p++) << 8);
            col +=  *p++;

            // binary search the 'colors' array to see if it's in there
            int low = 0;
            int high = num_colors - 1;
            while (low <= high) {
                mid = (low+high)/2;
                if (col < (int)colors[mid])
                    high = mid - 1;
                else if (col > (int)colors[mid])
                    low = mid + 1;
                else
                    break;
            }

            if (high < low) {
                // didn't find color in list, add it
                if (num_colors >= max_colors)
                    return (false);  // more colors than allowed
                my_bcopy((char*)&colors[low], (char*)&colors[low+1],
                    (num_colors - low) * sizeof(unsigned int));
                colors[low] = col;
                num_colors++;
            }
        }

        // Pixelize data: map pixel values of the RGB image in the colormap.
        unsigned char *pix;
        for (i = width*height, p = rgb, pix = img_data->data; i; i--,pix++) {
            int col  = (((unsigned int)*p++) << 16);
            col += (((unsigned int)*p++) << 8);
            col +=  *p++;

            // binary search the 'colors' array.  It IS in there
            int low = 0;
            int high = num_colors - 1;
            while (low <= high) {
                mid = (low+high)/2;
                if (col < (int)colors[mid])
                    high = mid - 1;
                else if (col > (int)colors[mid])
                    low = mid + 1;
                else
                    break;
            }
            *pix = mid;
        }

        // allocate colormap
        img_data->allocCmap(num_colors);

        // and fill it
        for (i = 0; i < num_colors; i++) {
            img_data->cmap[i].red   = ( colors[i] >> 16);
            img_data->cmap[i].green = ((colors[i] >> 8 ) & 0xff);
            img_data->cmap[i].blue  = ( colors[i]  & 0xff);
        }

        return (true);
    }


    // Quick conversion of RGB image with > 256 colors to a paletted image
    // using a 256 colors imagemap.  This routine does a very quick
    // quantization using a fixed palette and is based on the routine
    // quick_quant as found in xv28to8.c from xv-3.10 by John Bradley.
    // Used by permission.
    //
    void
    quick_quantize(unsigned char *rgb, htmRawImageData *img_data)
    {
        int *thisPtr, *nextPtr;
        int width  = img_data->width;
        int height = img_data->height;

        unsigned char *pp = img_data->data;
        int size = width*3;  // a single RGB scanline
        int imax = height-1;
        int jmax = width-1;

        // allocate a full colormap for this image
        img_data->allocCmap(MAX_IMAGE_COLORS);
        htmColor *cols = img_data->cmap;

        // fill colormap
        for (int i = 0; i < MAX_IMAGE_COLORS; i++) {
            cols[i].red =
                (int)((((i << RSHIFT) & RMASK)*255 + 0.5*RMASK)/RMASK);
            cols[i].green =
                (int)((((i << GSHIFT) & GMASK)*255 + 0.5*GMASK)/GMASK);
            cols[i].blue =
                (int)((((i << BSHIFT) & BMASK)*255 + 0.5*BMASK)/BMASK);
        }

        // temporary work memory
        int *thisLine = new int[size];
        int *nextLine = new int[size];

        // get first line of picture
        for (int j = size, *tmpPtr = nextLine; j; j--)
            *tmpPtr++ = (int)*rgb++;

        for (int i = 0; i < height; i++) {
            int *tmpPtr = thisLine;
            thisLine = nextLine;
            nextLine = tmpPtr;

            // get next line
            if (i != imax) {
                int j;
                for (j = size, tmpPtr = nextLine; j; j--)
                    *tmpPtr++ = (int)*rgb++;
            }

            // convert RGB scanline to indexed scanline
            int j;
            for (j = 0, thisPtr = thisLine, nextPtr = nextLine; j < width;
                    j++, pp++) {
                // get RGB values
                int r = *thisPtr++;
                int g = *thisPtr++;
                int b = *thisPtr++;

                // check validity of component ranges
                RANGE(r, 0, 255);
                RANGE(g, 0, 255);
                RANGE(b, 0, 255);

                // choose actual pixel value
                int val = (((r & RMASK) >> RSHIFT) | ((g & GMASK) >> GSHIFT) |
                    ((b & BMASK) >> BSHIFT));
                *pp = val;

                // compute color errors
                r -= cols[val].red;
                g -= cols[val].green;
                b -= cols[val].blue;

                // Add error fractions to adjacent pixels using a 3x3 matrix.

                // adjust RIGHT pixel
                if (j != jmax) {
                    thisPtr[0] += (int)(SIXTEENTH * (r*7));
                    thisPtr[1] += (int)(SIXTEENTH * (g*7));
                    thisPtr[2] += (int)(SIXTEENTH * (b*7));
                }

                // do BOTTOM pixel
                if (i != imax) {
                    nextPtr[0] += (int)(SIXTEENTH * (r*5));
                    nextPtr[1] += (int)(SIXTEENTH * (g*5));
                    nextPtr[2] += (int)(SIXTEENTH * (b*5));

                    // do BOTTOM LEFT pixel
                    if (j > 0) {
                        nextPtr[-3] += (int)(SIXTEENTH * (r*3));
                        nextPtr[-2] += (int)(SIXTEENTH * (g*3));
                        nextPtr[-1] += (int)(SIXTEENTH * (b*3));
                    }
                    // do BOTTOM RIGHT pixel
                    if (j != jmax) {
                        nextPtr[3] += (int)(SIXTEENTH * (r));
                        nextPtr[4] += (int)(SIXTEENTH * (g));
                        nextPtr[5] += (int)(SIXTEENTH * (b));
                    }
                    nextPtr += 3;
                }
            }
        }
        // free it
        delete [] thisLine;
        delete [] nextLine;
    }
}


//-----------------------------------------------------------------------------
// The following code based on code from the 'pbmplus' package written
// by Jef Poskanzer.
//

namespace htm
{
    struct chist_item
    {
        pixel color;
        int value;
    };

    struct chist_list_item
    {
        struct chist_item ch;
        struct chist_list_item *next;
    };

    struct box
    {
        int index;
        int colors;
        int sum;
    };

    typedef chist_item* chist_vec;
    typedef chist_list_item* chist_list;
    typedef chist_list* chash_table;
    typedef struct box* box_vector;
}


#define PPM_GETR(p) ((p).r)
#define PPM_GETG(p) ((p).g)
#define PPM_GETB(p) ((p).b)

namespace {
    inline bool
    redcompare(const chist_item &p1, const chist_item &p2)
    {
        return ((int)PPM_GETR(p1.color) < (int)PPM_GETR(p2.color));
    }

    inline bool
    greencompare(const chist_item &p1, const chist_item &p2)
    {
        return ((int)PPM_GETG(p1.color) < (int)PPM_GETG(p2.color));
    }

    inline bool
    bluecompare(const chist_item &p1, const chist_item &p2)
    {
        return ((int)PPM_GETB(p1.color) < (int)PPM_GETB(p2.color));
    }

    inline bool
    sumcompare(const box &p1, const box &p2)
    {
        return (p2.sum < p1.sum);
    }

    chist_vec mediancut(chist_vec, int, int, int, int);
    chist_vec ppm_computechist(pixel**, int, int, int, int*);
    chash_table ppm_computechash(pixel**, int, int, int, int*);
    chash_table ppm_allocchash(void);
    chist_vec ppm_chashtochist(chash_table, int);
    void ppm_freechash(chash_table);
}

// Various macros used by the quantization functions.

#define PPM_ASSIGN(p,red,grn,blu) \
    { (p).r = (red); (p).g = (grn); (p).b = (blu); }

// color compare
#define PPM_EQUAL(p,q) ( (p).r == (q).r && (p).g == (q).g && (p).b == (q).b )

// Color scaling macro -- to make writing ppmtowhatever easier.
#define PPM_DEPTH(newp,p,oldmaxval,newmaxval) \
    PPM_ASSIGN( (newp), \
            (int) PPM_GETR(p) * (newmaxval) / ((int)oldmaxval), \
            (int) PPM_GETG(p) * (newmaxval) / ((int)oldmaxval), \
            (int) PPM_GETB(p) * (newmaxval) / ((int)oldmaxval) )

// Luminance macro, using only integer ops.  Returns an int (*256).
#define PPM_LUMIN(p) \
    ( 77 * PPM_GETR(p) + 150 * PPM_GETG(p) + 29 * PPM_GETB(p) )

// compute a hashvalue
#define ppm_hashpixel(p) ((((int)PPM_GETR(p) * 33023 + \
    (int)PPM_GETG(p) * 30013 + (int)PPM_GETB(p) * 27011) & 0x7fffffff) \
    % HASH_SIZE)

#define MAXCOLORS 32767     // max. no of colors to take into account
#define HASH_SIZE 6553      // color hashtable size


namespace {
    int
    ppm_quant(unsigned char *pic24, pixel **pix, htmRawImageData *img_data,
        int max_colors)
    {
        int index = 0;
        unsigned char maxval = 255;
        int cols = img_data->width;
        int rows = img_data->height;
        unsigned char *pic8 = img_data->data;

        // reformat RGB image data into a 2D array of pixel values
        pixel **pixels;
        if (pix == 0) {
            pixels = new pixel*[rows];

            for (int row = 0; row < rows; row++) {
                pixels[row] = new pixel[cols];

                int col;
                pixel *pP;
                for (col = 0, pP=pixels[row]; col<cols; col++, pP++) {
                    pP->r = *pic24++;
                    pP->g = *pic24++;
                    pP->b = *pic24++;
                }
            }
        }
        else
            pixels = pix;

        // create unclustered color histogram
        chist_vec chv;
        int colors;
        for ( ; ; ) {
            chv = ppm_computechist(pixels, cols, rows, MAXCOLORS, &colors);
            if (chv != 0)
                break;

            unsigned char newmaxval = maxval / 2;

            for (int row = 0; row < rows; ++row) {
                int col;
                pixel *pP;
                for (col = 0, pP = pixels[row]; col < cols; ++col, ++pP)
                    PPM_DEPTH( *pP, *pP, maxval, newmaxval );
            }
            maxval = newmaxval;
        }

        // Step 3: apply median-cut to histogram, making the new colormap.
        chist_vec colormap =
            mediancut(chv, colors, rows * cols, maxval, max_colors);

        // free histogram
        delete [] chv;

        // Step 4: map the colors in the image to their closest match in the
        // new colormap, and write 'em out.

        chash_table cht = ppm_allocchash();

        unsigned char *picptr = pic8;
        for (int row = 0;  row < rows;  ++row) {
            int col = 0;
            int limitcol = cols;
            pixel *pP = pixels[row];

            do {
                // Check hash table to see if we have already matched
                // this color.
                int hash = ppm_hashpixel(*pP);

                // check for collisions
                chist_list chl;
                for (chl = cht[hash];  chl; chl = chl->next) {
                    if (PPM_EQUAL(chl->ch.color, *pP)) {
                        index = chl->ch.value;
                        break;
                    }
                }

                if (!chl) {
                    // Not preset, search colormap for closest match.

                    int r1 = PPM_GETR(*pP);
                    int g1 = PPM_GETG(*pP);
                    int b1 = PPM_GETB(*pP);
                    long dist = 2000000000;

                    for (int i = 0; i < max_colors; i++) {
                        int r2 = PPM_GETR(colormap[i].color);
                        int g2 = PPM_GETG(colormap[i].color);
                        int b2 = PPM_GETB(colormap[i].color);

                        long newdist = (r1 - r2)*(r1 - r2) +
                            (g1 - g2)*(g1 - g2) + (b1 - b2)*(b1 - b2);

                        if (newdist<dist) {
                            index = i;
                            dist = newdist;
                        }
                    }
                    hash = ppm_hashpixel(*pP);
                    chl = new chist_list_item;
                    chl->ch.color = *pP;
                    chl->ch.value = index;
                    chl->next = cht[hash];
                    cht[hash] = chl;
                }
                *picptr++ = index;
                ++col;
                ++pP;
            }
            while (col != limitcol);
        }
        // free the pixels array
        for (int i = 0; i < rows; i++)
            delete [] pixels[i];
        delete [] pixels;

        // rescale colormap and save return colormap
        if (img_data->cmapsize)
            delete [] img_data->cmap;
        img_data->allocCmap(max_colors);
        for (int i = 0; i < max_colors; i++) {
            PPM_DEPTH(colormap[i].color, colormap[i].color, maxval, 255);
            img_data->cmap[i].red   = PPM_GETR(colormap[i].color);
            img_data->cmap[i].green = PPM_GETG(colormap[i].color);
            img_data->cmap[i].blue  = PPM_GETB(colormap[i].color);
            img_data->cmap[i].pixel = i;
        }

        // free cht and colormap
        delete [] colormap;
        ppm_freechash(cht);

        return (0);
    }


    // Here is the fun part, the median-cut colormap generator.  This is
    // based on Paul Heckbert's paper "Color Image Quantization for Frame
    // Buffer Display", SIGGRAPH '82 Proceedings, page 297.
    //
    chist_vec
    mediancut(chist_vec chv, int colors, int sum, int maxval, int max_colors)
    {
        box_vector bv = new box[max_colors];
        chist_vec colormap = new chist_item[max_colors];

        // reset to zero
        for (int i = 0; i < max_colors; i++)
            PPM_ASSIGN(colormap[i].color, 0, 0, 0);

        // Set up the initial box.
        bv[0].index = 0;
        bv[0].colors = colors;
        bv[0].sum = sum;
        int boxes = 1;

        // Main loop: split boxes until we have enough.
        while (boxes < max_colors) {

            // Find the first splittable box.
            int bi;
            for (bi = 0; bv[bi].colors < 2 && bi < boxes; bi++) ;

            if (bi == boxes)
                break;  // ran out of colors!

            int indx = bv[bi].index;
            int clrs = bv[bi].colors;
            int sm = bv[bi].sum;

            // Go through the box finding the minimum and maximum of each
            // component - the boundaries of the box.

            int maxr = PPM_GETR( chv[indx].color );
            int minr = maxr;
            int maxg = PPM_GETG( chv[indx].color );
            int ming = maxg;
            int maxb = PPM_GETB( chv[indx].color );
            int minb = maxb;

            for (int i = 1; i < clrs; i++) {
                int v = PPM_GETR( chv[indx + i].color );
                if (v < minr) minr = v;
                if (v > maxr) maxr = v;

                v = PPM_GETG( chv[indx + i].color );
                if (v < ming) ming = v;
                if (v > maxg) maxg = v;

                v = PPM_GETB( chv[indx + i].color );
                if (v < minb) minb = v;
                if (v > maxb) maxb = v;
            }

            // Find the largest dimension, and sort by that component.
            pixel p;
            PPM_ASSIGN(p, maxr - minr, 0, 0);
            int rl = PPM_LUMIN(p);

            PPM_ASSIGN(p, 0, maxg - ming, 0);
            int gl = PPM_LUMIN(p);

            PPM_ASSIGN(p, 0, 0, maxb - minb);
            int bl = PPM_LUMIN(p);

            if (rl >= gl && rl >= bl)
                std::sort(chv + indx, chv + indx + clrs, redcompare);
            else if (gl >= bl)
                std::sort(chv + indx, chv + indx + clrs, greencompare);
            else
                std::sort(chv + indx, chv + indx + clrs, bluecompare);

            // Now find the median based on the counts, so that about half
            // the pixels (not colors, pixels) are in each subdivision.

            int lowersum = chv[indx].value;
            int halfsum = sm / 2;
            int i;
            for (i = 1; i < clrs-1; i++) {
                if (lowersum >= halfsum)
                    break;
                lowersum += chv[indx + i].value;
            }

            // Split the box, and sort to bring the biggest boxes to the top.
            bv[bi].colors = i;
            bv[bi].sum = lowersum;
            bv[boxes].index = indx + i;
            bv[boxes].colors = clrs - i;
            bv[boxes].sum = sm - lowersum;
            ++boxes;
            std::sort(bv, bv + boxes, sumcompare);
        }

        // Now choose a representative color for each box.

        for (int bi = 0; bi < boxes; bi++) {
            int indx = bv[bi].index;
            int clrs = bv[bi].colors;
            int r = 0, g = 0, b = 0, tmpsum = 0;

            for (int i = 0; i < clrs; i++) {
                r += PPM_GETR( chv[indx + i].color ) * chv[indx + i].value;
                g += PPM_GETG( chv[indx + i].color ) * chv[indx + i].value;
                b += PPM_GETB( chv[indx + i].color ) * chv[indx + i].value;
                tmpsum += chv[indx + i].value;
            }

            r = r / tmpsum;
            if (r > maxval)
                r = maxval;  // avoid math errors
            g = g / tmpsum;
            if (g > maxval)
                g = maxval;
            b = b / tmpsum;
            if (b > maxval)
                b = maxval;

            PPM_ASSIGN( colormap[bi].color, r, g, b );
        }

        delete [] bv;
        return (colormap);
    }


    chist_vec
    ppm_computechist(pixel **pixels, int cols, int rows, int maxcolors,
        int *colorsP)
    {

        chash_table cht =
            ppm_computechash(pixels, cols, rows, maxcolors, colorsP);
        if (!cht)
            return (0);

        chist_vec chv = ppm_chashtochist(cht, maxcolors);
        ppm_freechash(cht);
        return (chv);
    }


    chash_table
    ppm_computechash(pixel **pixels, int cols, int rows, int maxcolors,
        int *colorsP )
    {
        *colorsP = 0;
        chash_table cht = ppm_allocchash();

        // Go through the entire image, building a hash table of colors.
        for (int row = 0; row < rows; row++) {
            int col;
            pixel* pP;
            for (col = 0, pP = pixels[row];  col < cols;  col++, pP++) {
                int hash = ppm_hashpixel(*pP);

                chist_list chl;
                for (chl = cht[hash]; chl; chl = chl->next) {
                    if (PPM_EQUAL(chl->ch.color, *pP))
                        break;
                }

                if (chl)
                    ++(chl->ch.value);
                else {
                    if ((*colorsP)++ > maxcolors) {
                        ppm_freechash(cht);
                        return (0);
                    }

                    chl = new chist_list_item;
                    chl->ch.color = *pP;
                    chl->ch.value = 1;
                    chl->next = cht[hash];
                    cht[hash] = chl;
                }
            }
        }
        return (cht);
    }


    chash_table
    ppm_allocchash(void)
    {
        chash_table cht = new chist_list[HASH_SIZE];
        for (int i = 0; i < HASH_SIZE; i++)
            cht[i] = 0;

        return (cht);
    }


    chist_vec
    ppm_chashtochist(chash_table cht, int maxcolors)
    {
        // Now collate the hash table into a simple chist array.
        chist_vec chv = new chist_item[maxcolors];

        // Loop through the hash table.
        int j = 0;
        for (int i = 0; i < HASH_SIZE; i++) {
            for (chist_list chl = cht[i]; chl; chl = chl->next) {
                // Add the new entry.
                chv[j] = chl->ch;
                ++j;
            }
        }
        return (chv);
    }


    void
    ppm_freechash(chash_table cht)
    {
        for (int i = 0; i < HASH_SIZE; i++) {
            chist_list chlnext;
            for (chist_list chl = cht[i]; chl; chl = chlnext) {
                chlnext = chl->next;
                delete chl;
            }
        }
        delete [] cht;
    }
}



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
 * Ginterf Graphical Interface Library                                    *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

// Rendering functions for in-core bitmap generation.

#ifndef RGBZIMG_H_INCLUDED
#define RGBZIMG_H_INCLUDED

#include "graphics.h"
#include "shmctl.h"
#include <stdio.h>
#include <string.h>


// An RGB in-core image generator that keeps track of color ordering. 
// Colors can be written in any order, but for a given image pixel,
// the actual color will be the highest numbered pixel value written
// to the location.
//
// In Xic, this allows the image to be created with one pass through
// the hierarchy, and the image will display the layers top-down.

// These are predefined priorities.  Layers are drawn with a priority
// level of the index into the layer table, i.e. general drawing uses
// levels 1-999, 0 for background, 1000 and up for annotation.
//
#define LV_UNDER        0
#define LV_OVER_LAYERS  1000
#define LV_HLITE        1001
#define LV_OVER_HLITE   1002
#define LV_OVER_ALL     1003

namespace ginterf
{

    struct RGBzimg_dev : public GRdev
    {
        GRdraw* NewDraw(int) { return (0); }
        bool Init(int*, char**) { return (false); }
    };

    struct RGBzimg : public HCdraw
    {
        RGBzimg()
            {
                rz_dev = new RGBzimg_dev;
                rz_rgbmap = 0;
                rz_levmap = 0;
                rz_text_scale = 1.0;
                rz_cur_fp = 0;
                rz_cur_level = 0;
                rz_cur_color = 0;
                rz_cur_ls = 0;
                rz_shmid = 0;
                rz_no_level = false;
            }

        virtual ~RGBzimg()
            {
                delete rz_dev;
                free_map();
                delete [] rz_levmap;
            }

        void Pixel(int, int);
        void Pixels(GRmultiPt*, int);
        void Line(int, int, int, int);
        void PolyLine(GRmultiPt*, int);
        void Lines(GRmultiPt*, int);
        void Box(int, int, int, int);
        void Boxes(GRmultiPt*, int);
        void Arc(int, int, int, int, double, double);
        void Polygon(GRmultiPt*, int);
        void Zoid(int, int, int, int, int, int);
        void Text(const char*, int, int, int, int = -1, int = -1);
        void TextExtent(const char*, int*, int*);

        void Halt()                                             { }
        void ResetViewport(int, int)                            { }
        void DefineViewport()                                   { }
        void Dump(int)                                          { }
        void DisplayImage(const GRimage*, int, int, int, int)   { }
        double Resolution()                       { return (1.0); }

        void SetLinestyle(const GRlineType *lineptr)
            {
                if (!lineptr || !lineptr->mask || lineptr->mask == -1)
                    rz_cur_ls = 0;
                else
                    rz_cur_ls = lineptr->mask;
            }

        void SetFillpattern(const GRfillType *fillp)
            {
                if (!fillp || !fillp->hasMap())
                    rz_cur_fp = 0;
                else
                    rz_cur_fp = fillp;
            }

        void SetColor(int clr)
            {
                rz_cur_color = clr;
            }

        void Init(unsigned int w, unsigned int h, bool nolev)
            {
                if (rz_dev->width != (int)w || rz_dev->height != (int)h) {
                    if (w < 1)
                        w = 1;
                    if (h < 1)
                        h = 1;
                    unsigned int sz = w*h;
                    free_map();
                    alloc_map(sz);
                    delete [] rz_levmap;
                    rz_no_level = nolev;
                    if (nolev)
                        rz_levmap = 0;
                    else {
                        rz_levmap = new unsigned short[sz];
                        memset(rz_levmap, 0, sz*sizeof(short));
                    }
                    rz_dev->width = w;
                    rz_dev->height = h;
                }
                else {
                    rz_no_level = nolev;
                    if (nolev) {
                        delete [] rz_levmap;
                        rz_levmap = 0;
                    }
                    else {
                        unsigned int sz = rz_dev->width*rz_dev->height;
                        if (!rz_levmap)
                            rz_levmap = new unsigned short[sz];
                        memset(rz_levmap, 0, sz*sizeof(short));
                    }
                }
            }

        void SetLevel(int lev)
            {
                rz_cur_level = lev;
            }

        void DrawPixel(unsigned int offs)
            {
                if (rz_no_level) {
                    rz_rgbmap[offs] = rz_cur_color;
                    return;
                }
                unsigned short *z = rz_levmap + offs;
                if (rz_cur_level >= *z) {
                    *z = rz_cur_level;
                    rz_rgbmap[offs] = rz_cur_color;
                }
            }

        unsigned int *Map()
            {
                return (rz_rgbmap);
            }

        int shmid()
            {
                return (rz_shmid);
            }

    protected:
        // Take care of allocation of the rgbmap.  We use SYSV SHM if
        // possible, for the MIT-SHM extension.
        //
        void alloc_map(unsigned int sz)
            {
                rz_rgbmap = (unsigned int*)ShmCtl::allocate(&rz_shmid,
                    sz*sizeof(unsigned int));
            }

        void free_map()
            {
                ShmCtl::deallocate(rz_rgbmap);
                rz_rgbmap = 0;
            }

        GRdev *rz_dev;                  // pointer to device struct
        unsigned int *rz_rgbmap;        // base of image in core (0 <-> miny)
        unsigned short *rz_levmap;      // Z-map
        double rz_text_scale;           // scale size of text
        const GRfillType *rz_cur_fp;    // current fillpattern, 0 solid
        unsigned int rz_cur_level;      // current colormap offset
        unsigned int rz_cur_color;      // current RGB color;
        unsigned int rz_cur_ls;         // linestyle storage, 0 solid
        int rz_shmid;                   // segment id if SHM.
        bool rz_no_level;               // ignore levels
    };
}

#endif



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
 *   Stephen R. Whiteley  <stevew@wrcad.com>
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
 * $Id: htm_XPM.cc,v 1.7 2014/02/15 23:14:17 stevew Exp $
 *-----------------------------------------------------------------------*/

#include "htm_widget.h"
#include "htm_image.h"
#include "lstring.h"
#include <stdio.h>
#include <ctype.h>

namespace htm
{
    struct xpm_info
    {
        struct xpm_plane
        {
            xpm_plane()
                {
                    c = 0;
                    colorname = 0;
                }

            ~xpm_plane()
                {
                    delete [] colorname;
                }

            unsigned int c;
            char *colorname;
        };

        xpm_info(const char *const *);
        xpm_info(const char*);
        ~xpm_info()
            {
                delete [] cmap;
                delete [] data;
            }
        int depth() { return (nc); }
        void size(int *x, int *y) { *x = nx; *y = ny; }
        bool has_data() { return (data != 0); }
        htmRawImageData *to_raw_data(htmWidget*);

    private:
        int nx, ny, nc;         // size, color count
        xpm_plane *cmap;        // color values
        unsigned char *data;    // data, indices into cmap
    };
}


// Load the info from an XPM string array.
//
htm::xpm_info::xpm_info(const char * const *xpm)
{
    nx = 0;
    ny = 0;
    nc = 0;
    cmap = 0;
    data = 0;

    if (!xpm || !*xpm)
        return;
    int cc;
    if (sscanf(xpm[0], "%d %d %d %d", &nx, &ny, &nc, &cc) != 4)
        return;
    if (nc < 1)
        return;
    if (cc > 4)
        return;
    cmap = new xpm_plane[nc];
    for (int i = 0; i < nc; i++) {
        const char *s = xpm[i+1];
        cmap[i].c = 0;
        for (int j = 0; j < cc; j++) {
            cmap[i].c |= (*s << j*8);
            s++;
        }
        while (isspace(*s))
            s++;
        while (*s && !isspace(*s))
            s++;
        while (isspace(*s))
            s++;
        cmap[i].colorname = lstring::gettok(&s);
    }
    data = new unsigned char[nx*ny];
    int lastv = 0;
    unsigned int lastc = 0;
    for (int i = 0; i < ny; i++) {
        const char *s = xpm[nc + 1 + i];
        for (int j = 0; j < nx; j++) {
            unsigned int xc = 0;
            for (int k = 0; k < cc; k++) {
                xc |= (*s << k*8);
                s++;
            }
            if (xc != lastc) {
                for (int k = 0; k < nc; k++) {
                    if (xc == cmap[k].c) {
                        lastc = xc;
                        lastv = k;
                        break;
                    }
                }
            }
            data[i*nx + j] = lastv;
        }
    }
}


namespace {
    // Grab the next quoted string from a line in the block, and advance
    // the pointer.  Return the string with quotes stripped.
    //
    char *
    getline(const char **bp)
    {
        char *line = 0;
        for (;;) {
            const char *start = *bp;
            if (!start)
                break;
            const char *end = start;
            while (*end && *end != '\n')
                end++;
            *bp = *end ? end+1 : end;
            while (isspace(*start) && start < end)
                start++;
            if (*start != '"')
                continue;
            start++;
            while (end > start && *end != '"')
                end--;
            if (*end != '"')
                continue;
            line = new char[end - start + 1];
            strncpy(line, start, end - start);
            line[end-start] = 0;
            break;
        }
        return (line);
    }
}


// Load the data from the xpm text read from a file.
//
htm::xpm_info::xpm_info(const char *buf)
{
    nx = 0;
    ny = 0;
    nc = 0;
    cmap = 0;
    data = 0;

    const char *bp = buf;

    char *str = getline(&bp);
    int cc;
    if (sscanf(str, "%d %d %d %d", &nx, &ny, &nc, &cc) != 4) {
        delete [] str;
        return;
    }
    if (nc < 1)
        return;
    if (cc > 4)
        return;
    cmap = new xpm_plane[nc];
    for (int i = 0; i < nc; i++) {
        str = getline(&bp);
        if (!str) {
            delete [] cmap;
            cmap = 0;
            return;
        }
        char *s = str;
        cmap[i].c = 0;
        for (int j = 0; j < cc; j++) {
            cmap[i].c |= (*s << j*8);
            s++;
        }
        while (isspace(*s))
            s++;
        while (*s && !isspace(*s))
            s++;
        while (isspace(*s))
            s++;
        cmap[i].colorname = lstring::gettok(&s);
        delete [] str;
    }
    data = new unsigned char[nx*ny];
    int lastv = 0;
    unsigned int lastc = 0;
    for (int i = 0; i < ny; i++) {
        str = getline(&bp);
        if (!str) {
            delete [] cmap;
            cmap = 0;
            delete [] data;
            data = 0;
            return;
        }
        char *s = str;
        for (int j = 0; j < nx; j++) {
            unsigned int xc = 0;
            for (int k = 0; k < cc; k++) {
                xc |= (*s << k*8);
                s++;
            }
            if (xc != lastc) {
                for (int k = 0; k < nc; k++) {
                    if (xc == cmap[k].c) {
                        lastc = xc;
                        lastv = k;
                        break;
                    }
                }
            }
            data[i*nx + j] = lastv;
        }
        delete [] str;
    }
}


// Create and return an htmRawIMageData struct representing the XPM
// image.
//
htmRawImageData *
htm::xpm_info::to_raw_data(htmWidget *html)
{
    int is_gray = 0;

    // allocate raw image
    htmRawImageData *img_data = new htmRawImageData(nx, ny, nc);

    // fill colormap for this image
    for (int i = 0; i < nc; i++) {
        // pick up the name of the current color
        const char *col_name = cmap[i].colorname;

        // transparancy, these can all name a transparent color
        htmColor tmpcolr;
        if (!(strcasecmp(col_name, "none")) ||
                !(strcasecmp(col_name, "mask")) ||
                !(strcasecmp(col_name, "background"))) {
            unsigned int bg_pixel;

            // Get current background index:  use the given background
            // pixel index if we have one.  Else get the current
            // background color of the given widget.

            bg_pixel = html->htm_cm.cm_body_bg;

            // get RGB components for this color
            tmpcolr.pixel = bg_pixel;
            html->htm_tk->tk_query_colors(&tmpcolr, 1);

            // store background pixel index
            img_data->bg = i;
        }
        else
            // get RGB components for this color
            html->htm_tk->tk_parse_color(col_name, &tmpcolr);

        // colorcomponents are in the range 0-255
        img_data->cmap[i].red   = tmpcolr.red;
        img_data->cmap[i].green = tmpcolr.green;
        img_data->cmap[i].blue  = tmpcolr.blue;
        is_gray &= (tmpcolr.red == tmpcolr.green) &&
                    (tmpcolr.green == tmpcolr.blue);
    }
    // no need to fill in remainder of colormap
    img_data->color_class = (is_gray != 0 ? IMAGE_COLORSPACE_INDEXED :
        IMAGE_COLORSPACE_GRAYSCALE);

    // copy data, no changes needed
    memcpy(img_data->data, data, nx*ny);

    return (img_data);
}


// Reads an xpm image of any type from xpm data read from a file.
//
htmRawImageData*
htmImageManager::readXPM(ImageBuffer *ib)
{
    htm::xpm_info info((char*)ib->buffer);
    if (!info.has_data()) {
        // spit out appropriate error message
        im_html->warning("readXPM", "failed on image %s.", ib->file);
        return (0);
    }
    // compute image depth
    ib->depth = 2;
    while ((ib->depth << 2) < info.depth() && ib->depth < 12)
        ib->depth++;
    return (info.to_raw_data(im_html));
}


// Read an xpm image of any type from raw xpm data.
//
htmRawImageData*
htmImageManager::createXpmFromData(const char * const *data, const char *src)
{
    htm::xpm_info info(data);
    if (!info.has_data()) {
        // spit out appropriate error message
        im_html->warning("createXpmFromData", "failed on image %s.", src);
        return (0);
    }
    return (info.to_raw_data(im_html));
}


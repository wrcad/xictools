
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
#include "htm_tag.h"
#include "htm_format.h"
#include "htm_string.h"
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

namespace {
    int *getCoordinates(const char*, int*);


    inline int
    isct(int y, htmPoint *pts)
    {
        if (pts->x == (pts-1)->x)
            return (pts->x);
        return ((int)((y - (pts-1)->y)*
            ((double)(pts->x - (pts-1)->x)/
            (pts->y - (pts-1)->y))) + (pts-1)->x);
    }
}


// Return true if the point is in the polygon or on the boundary.
//
bool
htmPoly::intersect(int x, int y)
{
    int i;
    for (i = numpts - 1; i >= 0; i--) {
        if (points[i].y != y)
            break;
    }
    if (i < 0)
        // strange case
        return (false);
    bool below = (points[i].y < y);

    htmPoint *pts = points;
    int num = numpts;
    int count = 0;
    // count the crossings to the left of x, and look for boundary
    // intersections
    //
    for (pts++, num--; num; pts++, num--) {
        if ((below && pts->y > y) || (!below && pts->y < y)) {
            int xtmp = isct(y, pts);
            if (xtmp < x) {
                if (below)
                    count++;
                else
                    count--;
            }
            if (xtmp == x)
                return (true);
            below = !below;
        }
        if (pts->y == y) {
            if ((pts-1)->y != pts->y) {
                if (x == pts->x)
                    return (true);
            }
            else {
                if (((pts-1)->x <= x && x <= pts->x) ||
                        (pts->x <= x && x <= (pts-1)->x))
                    return (true);
            }
        }
    }
    return (count);
}
// End of htmPoly functions.


namespace htm
{
    enum MapShape { MAP_DEFAULT, MAP_RECT, MAP_CIRCLE, MAP_POLY };

    struct mapArea
    {
        mapArea(htmObject*);
        ~mapArea();

        char *url;              // url to call when clicked
        char *alt;              // alternative text
        bool nohref;            // obvious
        MapShape shape;         // type of area
        int ncoords;            // no of coordinates
        int *coords;            // array of coordinates
        htmPoly poly;           // Region for polygons
        htmAnchor *anchor;      // anchor object
        mapArea *next;          // ptr to next area
    };

    mapArea::mapArea(htmObject *object)
    {
        url = htmTagGetValue(object->attributes, "href");
        alt = htmTagGetValue(object->attributes, "alt");
        nohref = htmTagCheck(object->attributes, "nohref");

        char *chPtr = htmTagGetValue(object->attributes, "shape");
        if (chPtr == 0) {
            // No shape given, try to figure it out using the number of
            // specified coordinates.
            switch (ncoords) {
            case 0:
                // no coords given => default area
                shape = MAP_DEFAULT;
                break;
            case 3:
                // 3 coords => circle
                shape = MAP_CIRCLE;
                break;
            case 4:
                // 4 coords => assume rectangle
                shape = MAP_RECT;
                break;
            default:
                // assume poly
                shape = MAP_POLY;
            }
        }
        else {
            switch (tolower(chPtr[0])) {
            case 'c':
                shape = MAP_CIRCLE;
                break;
            case 'r':
                shape = MAP_RECT;
                break;
            case 'p':
                shape = MAP_POLY;
                break;
            default:
                shape = MAP_DEFAULT;
            }
            delete [] chPtr;
        }

        // get specified coordinates
        coords = getCoordinates(object->attributes, &ncoords);

        anchor      = 0;
        next        = 0;
    }


    mapArea::~mapArea()
    {
        delete [] url;
        delete [] alt;
        delete [] coords;
    }
}
// End of mapArea functions

using namespace htm;

htmImageMap::htmImageMap(const char *n)
{
    name    = lstring::copy(n);
    nareas  = 0;
    areas   = 0;
    next    = 0;
}


htmImageMap::~htmImageMap()
{
    delete [] name;
    while (areas) {
        mapArea *anxt = areas->next;
        delete areas;
        areas = anxt;
    }
}
// End of htmImageMap functions


// Store the given imagemap in the given html widget.
//
void
htmImageManager::storeImagemap(htmImageMap *map)
{
    // head of the list
    if (im_image_maps == 0) {
        im_image_maps = map;
        return;
    }

    // walk to the one but last map in the list and insert it
    htmImageMap *tmp;
    for (tmp = im_image_maps; tmp && tmp->next; tmp = tmp->next) ;
    tmp->next = map;
}


// Adds the given area specification to the given imagemap.
//
void
htmImageManager::addAreaToMap(htmImageMap *map, htmObject *object,
    htmFormatManager *fmt)
{
    if (map == 0 || object->attributes == 0)
        return;

    mapArea *area = new mapArea(object);

    // check if all coordinates specs are valid for the given shape
    if (area->shape == MAP_RECT) {
        // too bad if coords are bad
        if (area->ncoords != 4) {
            im_html->warning("addAreaToImagemap",
                "Imagemap shape = RECT but "
                "I have %i coordinates instead of 4. Area ignored.",
                area->ncoords);
            delete area;
            return;
        }
    }
    else if (area->shape == MAP_CIRCLE) {
        // too bad if coords are bad
        if (area->ncoords != 3) {
            im_html->warning("addAreaToImagemap",
                "Imagemap shape = CIRCLE "
                "but I have %i coordinates instead of 3. Area ignored.",
                area->ncoords);
            delete area;
            return;
        }
    }
    else if (area->shape == MAP_POLY) {
        if (!area->coords) {
            im_html->warning("addAreaToImagemap",
                "Imagemap shape = POLY but"
                " I have no coordinates!. Area ignored.",
                area->ncoords);
            delete area;
            return;
        }
        if (area->ncoords % 2) {
            im_html->warning("addAreaToImagemap",
                "Imagemap shape = POLY "
                "but I have oddsized polygon coordinates (%i found).\n"
                "    Skipping last coordinate.", area->ncoords);
            area->ncoords--;
        }

        // create array of htmPoint's required for region generation
        int half = area->ncoords/2;
        htmPoint *xpoints = new htmPoint[half + 1];
        for (int i = 0; i < half; i++) {
            xpoints[i].x = area->coords[i*2];
            xpoints[i].y = area->coords[i*2+1];
        }
        // last point is same as first point
        xpoints[half].x = xpoints[0].x;
        xpoints[half].y = xpoints[0].y;

        area->poly.points = xpoints;
        area->poly.numpts = half + 1;
    }

    // gets automagically added to the list of anchors for this widget
    if (!area->nohref)
        area->anchor = im_html->newAnchor(object, fmt);

    // add this area to the list of areas for this imagemap
    if (map->areas == 0) {
        map->nareas = 1;
        map->areas = area;
        return;
    }
    mapArea *tmp = map->areas;
    while (tmp->next)
        tmp = tmp->next;
    tmp->next = area;
    map->nareas++;
}


// Retrieve the imagemap with the given name.
//
htmImageMap*
htmImageManager::getImagemap(const char *name)
{
    if (!name || *name == '\0')
        return (0);
    htmImageMap *tmp;
    for (tmp = im_image_maps; tmp &&
        strcasecmp(tmp->name, &name[1]); tmp = tmp->next) ;
    return (tmp);
}


// Checks whether the given coordinates lie somewhere within the given
// imagemap.
//
htmAnchor*
htmImageManager::getAnchorFromMap(int x, int y, htmImage *image,
    htmImageMap *map)
{
    // map coordinates to upperleft corner of image
    int xs = x  - image->owner->area.x;
    int ys = y  - image->owner->area.y;

    mapArea *area = map->areas;
    mapArea *def_area = 0;

    // We test against found instead of anchor becoming nonzero:
    // areas with the NOHREF attribute set don't have an anchor but
    // should be taken into account as well.

    htmAnchor *anchor = 0;
    bool found = false;
    while (area && !found) {
        switch (area->shape) {
        case MAP_RECT:
            if (xs >= area->coords[0] && xs <= area->coords[2] &&
                    ys >= area->coords[1] && ys <= area->coords[3]) {
                anchor = area->anchor;
                found = true;
            }
            break;
        case MAP_CIRCLE:
            if ((xs - area->coords[0])*(double)(xs - area->coords[0]) +
                    (ys - area->coords[1])*(double)(ys - area->coords[1]) <=
                    area->coords[2]*(double)area->coords[2]) {
                anchor = area->anchor;
                found = true;
            }
            break;
        case MAP_POLY:
            if (area->poly.intersect(xs, ys)) {
                anchor = area->anchor;
                found = true;
            }
            break;

        // Just save default area info; it's only needed if nothing
        // else matches.

        case MAP_DEFAULT:
            def_area = area;
            break;
        }
        area = area->next;
    }
    if (!found && def_area)
        anchor = def_area->anchor;

    return (anchor);
}


// Checks whether an image requires an external imagemap.  This
// function is only effective when a imagemapCallback callback is
// installed.  When an external imagemap is required, this routine
// triggers this callback and will load an imagemap when the
// map_contents field is non-null after the callback returns.  We make
// a copy of this map and use it to parse and load the imagemap.
//
void
htmImageManager::checkImagemaps()
{
    if (!im_html->htm_if)
        return;
    for (htmImage *image = im_images; image; image = image->next) {
        if (image->map_url != 0) {
            if (!getImagemap(image->map_url)) {
                htmImagemapCallbackStruct cbs;
                cbs.map_name = image->map_url;
                cbs.image_name = image->html_image->url;
                im_html->htm_if->emit_signal(S_IMAGEMAP, &cbs);

                // parse and add this imagemap
                if (cbs.map_contents != 0) {
                    char *map = lstring::copy(cbs.map_contents);
                    imageAddImageMap(map);
                    delete [] map;
                }
            }
        }
    }
}


// Free all imagemaps for the given widget.
//
void
htmImageManager::freeImageMaps()
{
    htmImageMap::destroy(im_image_maps);
    im_image_maps = 0;
}


// Draw a bounding box around each area in an imagemap.
//
void
htmImageManager::drawImagemapSelection(htmImage *image)
{
    htmImageMap *map;
    if ((map = getImagemap(image->map_url)) == 0)
        return;
    if (!image->owner)
        return;

    // map coordinates to upperleft corner of image
    int xs = im_html->htm_viewarea.x - image->owner->area.x;
    int ys = im_html->htm_viewarea.y - image->owner->area.y;

    mapArea *area = map->areas;

    while (area) {
        if (area->shape == MAP_RECT) {

            int x = area->coords[0] - xs;
            int y = area->coords[1] - ys;
            int width = area->coords[2] - area->coords[0];
            int height = area->coords[3] - area->coords[1];
            im_html->htm_tk->tk_set_foreground(im_html->htm_cm.cm_imagemap_fg);
            im_html->htm_tk->tk_draw_rectangle(false, x, y, width, height);
        }
        else if (area->shape == MAP_CIRCLE) {

            int npoints = area->ncoords/2;
            htmPoint *points = new htmPoint[npoints+1];
            for (int i = 0; i < npoints; i++) {
                points[i].x = area->coords[i*2] - xs;
                points[i].y = area->coords[i*2+1] - ys;
            }
            // last point is same as first point
            points[npoints].x = points[0].x;
            points[npoints].y = points[0].y;

            im_html->htm_tk->tk_set_foreground(im_html->htm_cm.cm_imagemap_fg);
            im_html->htm_tk->tk_draw_polygon(false, points, npoints+1);
            delete [] points;
        }
        else if (area->shape == MAP_POLY) {

            int x = area->coords[0] - xs;
            int y = area->coords[1] - ys;
            int radius = area->coords[2];

            // upper-left corner of bounding rectangle
            x -= radius;
            y -= radius;

            im_html->htm_tk->tk_set_foreground(im_html->htm_cm.cm_imagemap_fg);
            im_html->htm_tk->tk_draw_arc(false, x, y, 2*radius, 2*radius,
                0, 23040);
        }
        area = area->next;
    }
}
// End of htmImageManager functions


namespace {
    // Returns array of map coordinates.
    //
    int *
    getCoordinates(const char *attributes, int *ncoords)
    {
        *ncoords = 0;

        // get coordinates and count how many there are
        char *chPtr = htmTagGetValue(attributes, "coords");
        if (!chPtr)
            return (0);

        // count how many coordinates we have
        int num = 0;
        char *tok, *attr = chPtr;
        while ((tok = htm_csvtok(&attr)) != 0) {
            num++;
            delete [] tok;
        }
        if (!num) {
            delete [] chPtr;
            return (0);
        }

        // allocate memory for these coordinates
        int *coords = new int[num];

        // again get coordinates, but now convert to numbers
        attr = chPtr;
        num = 0;
        while ((tok = htm_csvtok(&attr)) != 0) {
            coords[num++] = atoi(tok);
            delete [] tok;
        }

        delete [] chPtr;

        *ncoords = num;
        return (coords);
    }
}


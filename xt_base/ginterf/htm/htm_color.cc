
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
#include "htm_image.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>

// RANGE forces a to be in the range b..c (inclusive)
#define RANGE(a, b, c) { if (a < b) a = b;  if (a > c) a = c; }

#define HIGHLIGHT_MULT 0.8
#define LIGHT_MULT     1.5
#define DARK_MULT      0.5

namespace {
    void rgb_to_hls(double*, double*, double*);
    void hls_to_rgb(double*, double*, double*);
}

// HTML-3.2 color names
// "black",     "#000000"
// "silver",    "#c0c0c0"
// "gray",      "#808080"
// "white",     "#ffffff"
// "maroon",    "#800000"
// "red",       "#ff0000"
// "purple",    "#800080"
// "fuchsia",   "#ff00ff"
// "green",     "#008000"
// "lime",      "#00ff00"
// "olive",     "#808000"
// "yellow",    "#ffff00"
// "navy",      "#000080"
// "blue",      "#0000ff"
// "teal",      "#008080"
// "aqua",      "#00ffff"

htmDefColors htmColorManager::cm_defcolors;

htmColorManager::htmColorManager(htmWidget *html)
{
    cm_html                     = html;

    // Warning, can't call tk functions in constructor since vtab
    // isn't initialized yet.

    cm_anchor_fg                = 0;
    cm_anchor_visited_fg        = 0;
    cm_anchor_target_fg         = 0;
    cm_anchor_activated_fg      = 0;

    cm_body_bg                  = 0;
    cm_body_fg                  = 0;
    cm_select_bg                = 0;
    cm_imagemap_fg              = 0;
    cm_shadow_top               = 0;
    cm_shadow_bottom            = 0;
    cm_highlight                = 0;

    cm_anchor_fg_save           = 0;
    cm_anchor_visited_fg_save   = 0;
    cm_anchor_target_fg_save    = 0;
    cm_anchor_activated_fg_save = 0;
    cm_body_bg_save             = 0;
    cm_body_fg_save             = 0;
    cm_select_bg_save           = 0;
    cm_imagemap_fg_save         = 0;
    cm_colors_saved             = false;

    cm_fullscale                = 0;
    cm_halfscale                = 0;
}

#define DEFCLR(x, y) cm_defcolors.x ? cm_defcolors.x : y

void
htmColorManager::initialize()
{
    if (!cm_colors_saved) {
        cm_anchor_fg_save =
            getPixelByName(DEFCLR(DefFgLink,        "blue"), 0);
        cm_anchor_visited_fg_save =
            getPixelByName(DEFCLR(DefFgVisitedLink, "steel blue"), 0);
        cm_anchor_target_fg_save =
            getPixelByName(DEFCLR(DefFgTargetLink,  "blue"), 0);
        cm_anchor_activated_fg_save =
            getPixelByName(DEFCLR(DefFgActiveLink,  "red"), 0);
        cm_body_bg_save =
            getPixelByName(DEFCLR(DefBg,            "white"), 0);
        cm_body_fg_save =
            getPixelByName(DEFCLR(DefFgText,        "black"), 0);
        cm_select_bg_save =
            getPixelByName(DEFCLR(DefBgSelect,      "pale turquoise"), 0);
        cm_imagemap_fg_save =
            getPixelByName(DEFCLR(DefFgImagemap,    "blue"), 0);
        cm_colors_saved = true;

        // Set pixel value scale, which is generally 8 or 16 bits.
        htmColor c;
        cm_html->htm_tk->tk_parse_color("white", &c);  // should never fail
        cm_fullscale = c.red;
        cm_halfscale = c.red/2 + 1;
    }

    // Original colors must be stored.  They can be altered by the
    // <BODY> element, so if we get a body without any or some of
    // these colors specified, we can use the proper default values
    // for the unspecified elements.

    cm_anchor_fg                = cm_anchor_fg_save;
    cm_anchor_visited_fg        = cm_anchor_visited_fg_save;
    cm_anchor_target_fg         = cm_anchor_target_fg_save;
    cm_anchor_activated_fg      = cm_anchor_activated_fg_save;
    cm_body_bg                  = cm_body_bg_save;
    cm_body_fg                  = cm_body_fg_save;
    cm_select_bg                = cm_select_bg_save;
    cm_imagemap_fg              = cm_imagemap_fg_save;

    recomputeColors(cm_body_bg);
}


void
htmColorManager::reset()
{
    // Reset all colors
    cm_body_fg              = cm_body_fg_save;
    cm_body_bg              = cm_body_bg_save;
    cm_anchor_fg            = cm_anchor_fg_save;
    cm_anchor_target_fg     = cm_anchor_target_fg_save;
    cm_anchor_visited_fg    = cm_anchor_visited_fg_save;
    cm_anchor_activated_fg  = cm_anchor_activated_fg_save;
    cm_select_bg            = cm_select_bg_save;
}


// Retrieve the pixel value for a color name.  Color can be given as a
// color name as well as a color value.
//
unsigned int
htmColorManager::getPixelByName(const char *color, unsigned int def_pixel)
{
    if (!color || !*color)
        return (def_pixel);

    htmColor def;
    if ((!tryColor(color, &def))) {
        // bad color spec, return
        cm_html->warning("getPixelByName", "Bad color name %s.", color);
        return (def_pixel);
    }
    return (def.pixel);
}


// Compute new values for top and bottom shadows and the highlight
// color based on the current background color.
//
void
htmColorManager::recomputeColors(unsigned int bg)
{
    htmColor cbackground;
    cbackground.pixel = bg;
    cm_html->htm_tk->tk_query_colors(&cbackground, 1);

    htmColor c;
    shade_color(&cbackground, &c, LIGHT_MULT);
    cm_html->htm_tk->tk_alloc_color(&c);
    cm_shadow_top = c.pixel;

    shade_color(&cbackground, &c, DARK_MULT);
    cm_html->htm_tk->tk_alloc_color(&c);
    cm_shadow_bottom = c.pixel;

    shade_color(&cbackground, &c, HIGHLIGHT_MULT);
    cm_html->htm_tk->tk_alloc_color(&c);
    cm_highlight = c.pixel;
}


// Compute the select color based upon the given color.
//
void
htmColorManager::recomputeHighlightColor(unsigned int bg)
{
    htmColor cbackground;
    cbackground.pixel = bg;
    cm_html->htm_tk->tk_query_colors(&cbackground, 1);

    htmColor c;
    shade_color(&cbackground, &c, HIGHLIGHT_MULT);
    cm_html->htm_tk->tk_alloc_color(&c);
    cm_highlight = c.pixel;
}


// Verify the validity of the given colorname and returns the
// corresponding RGB components and pixel value for the requested
// color.  This function tries to recover from incorrectly specified
// RGB triplets.  (not all six fields present).
//
//
bool
htmColorManager::tryColor(const char *color, htmColor *def)
{
    // Backup color for stupid html writers that don't use a leading
    // hash.  The 000000 will ensure we have a full colorspec if the
    // user didn't specify the full symbolic color name (2 red, 2
    // green and 2 blue).

    // first try original name
    if (!cm_html->htm_tk->tk_parse_color(color, def)) {

        // Failed, see if we have a leading hash.  This doesn't work
        // with symbolic color names.  Too bad then.

        char hash[8];
        strcpy(hash, "#000000");

        int j = (color[0] == '#' ? 1 : 0);
        for (int i = 1; i < 7 && color[j]; i++, j++) {
            if (isdigit(color[j]) ||
                   (color[j] >= 'a' && color[j] <= 'f') ||
                   (color[j] >= 'A' && color[j] <= 'F'))
                hash[i] = color[j];
        }

        // try again
        if (!cm_html->htm_tk->tk_parse_color(hash, def))
            return (false);
    }
    return (true);
}


// This derives a color from the passed pixel that is just slightly
// dfferent, used to set the anchor_target_fg from the anchor_fg.
//
int
htmColorManager::anchor_fg_pixel(unsigned int pix)
{
    htmColor c;
    c.pixel = pix;
    cm_html->htm_tk->tk_query_colors(&c, 1);
    int r = c.red;
    int g = c.green;
    int b = c.blue;
    int rr = r;
    int gg = g;
    int bb = b;

    rr += (int)((b + g - r)*0.2);
    gg += (int)((r + b - g)*0.2);
    bb += (int)((r + g - b)*0.2);
    rr -= (int)((r - (int)cm_halfscale)*0.2);
    gg -= (int)((g - (int)cm_halfscale)*0.2);
    bb -= (int)((b - (int)cm_halfscale)*0.2);
    RANGE(rr, 0, 255);
    RANGE(gg, 0, 255);
    RANGE(bb, 0, 255);
    c.red = rr;
    c.green = gg;
    c.blue = bb;

    unsigned int pixel = 0;
    cm_html->htm_tk->tk_get_pixels(&c.red, &c.green, &c.blue, 1, &pixel);
    return (pixel);
}


void
htmColorManager::shade_color(htmColor *a, htmColor *b, double k)
{
    // Special case for black, so that it looks pretty...

    if ((a->red == 0 && a->green == 0 && a->blue == 0) ||
            (k != HIGHLIGHT_MULT && a->red == cm_fullscale &&
            a->green == cm_fullscale && a->blue == cm_fullscale)) {
        a->red = cm_halfscale;
        a->green = cm_halfscale;
        a->blue = cm_halfscale;
    }

    double red = a->red / (double)cm_fullscale;
    double green = a->green / (double)cm_fullscale;
    double blue = a->blue / (double)cm_fullscale;

    rgb_to_hls(&red, &green, &blue);

    green *= k;

    if (green > 1.0)
        green = 1.0;
    else if (green < 0.0)
        green = 0.0;

    blue *= k;

    if (blue > 1.0)
        blue = 1.0;
    else if (blue < 0.0)
        blue = 0.0;

    hls_to_rgb(&red, &green, &blue);

    b->red = (int) (red * cm_fullscale + 0.5);
    b->green = (int) (green * cm_fullscale + 0.5);
    b->blue = (int) (blue * cm_fullscale + 0.5);
}
// End of htmColorManager functions


namespace {
    // This color shading method is taken from gtkstyle.c.  Some
    // modifications made by me - Federico.

    void
    rgb_to_hls(double *r, double *g, double *b)
    {
        double red = *r;
        double green = *g;
        double blue = *b;

        double min, max;
        if (red > green) {
            if (red > blue)
                max = red;
            else
                max = blue;

            if (green < blue)
                min = green;
            else
                min = blue;
        }
        else {
            if (green > blue)
                max = green;
            else
                max = blue;

            if (red < blue)
                min = red;
            else
                min = blue;
        }

        double l = (max + min) / 2;
        double s = 0;
        double h = 0;

        if (max != min) {
            if (l <= 0.5)
                s = (max - min) / (max + min);
            else
                s = (max - min) / (2 - max - min);

            double delta = max -min;
            if (red == max)
                h = (green - blue) / delta;
            else if (green == max)
                h = 2 + (blue - red) / delta;
            else if (blue == max)
                h = 4 + (red - green) / delta;

            h *= 60;
            if (h < 0.0)
                h += 360;
        }

        *r = h;
        *g = l;
        *b = s;
    }


    void
    hls_to_rgb(double *h, double *l, double *s)
    {
        double lightness = *l;
        double saturation = *s;

        double m2;
        if (lightness <= 0.5)
            m2 = lightness * (1 + saturation);
        else
            m2 = lightness + saturation - lightness * saturation;
        double m1 = 2 * lightness - m2;

        if (saturation == 0) {
            *h = lightness;
            *l = lightness;
            *s = lightness;
        }
        else {
            double hue = *h + 120;
            while (hue > 360)
                hue -= 360;
            while (hue < 0)
                hue += 360;

            double r, g, b;
            if (hue < 60)
                r = m1 + (m2 - m1) * hue / 60;
            else if (hue < 180)
                r = m2;
            else if (hue < 240)
                r = m1 + (m2 - m1) * (240 - hue) / 60;
            else
                r = m1;

            hue = *h;
            while (hue > 360)
                hue -= 360;
            while (hue < 0)
                hue += 360;

            if (hue < 60)
                g = m1 + (m2 - m1) * hue / 60;
            else if (hue < 180)
                g = m2;
            else if (hue < 240)
                g = m1 + (m2 - m1) * (240 - hue) / 60;
            else
                g = m1;

            hue = *h - 120;
            while (hue > 360)
                hue -= 360;
            while (hue < 0)
                hue += 360;

            if (hue < 60)
                b = m1 + (m2 - m1) * hue / 60;
            else if (hue < 180)
                b = m2;
            else if (hue < 240)
                b = m1 + (m2 - m1) * (240 - hue) / 60;
            else
                b = m1;

            *h = r;
            *l = g;
            *s = b;
        }
    }
}


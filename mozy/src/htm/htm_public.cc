
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
#include "htm_parser.h"
#include "htm_string.h"
#include "htm_image.h"
#include "htm_format.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifndef HTM_VERSION
#define HTM_VERSION "1.0"
#endif

//-----------------------------------------------------------------------------
// Anchor resources

void
htmWidget::setAnchorStyle(AnchorStyle style)
{
    if (htm_anchor_style == style)
        return;
    htm_anchor_style = style;
    if (htm_initialized) {
        htm_reformat_needed = true;
        trySync();
    }
}


void
htmWidget::setAnchorVisitedStyle(AnchorStyle style)
{
    if (htm_anchor_visited_style == style)
        return;
    htm_anchor_visited_style = style;
    if (htm_initialized) {
        htm_reformat_needed = true;
        trySync();
    }
}


void
htmWidget::setAnchorTargetStyle(AnchorStyle style)
{
    if (htm_anchor_target_style == style)
        return;
    htm_anchor_target_style = style;
    if (htm_initialized) {
        htm_reformat_needed = true;
        trySync();
    }
}


void
htmWidget::setHighlightOnEnter(bool set)
{
    htm_armed_anchor = 0;
    htm_highlight_on_enter = set;
}


void
htmWidget::setAnchorCursorDisplay(bool set)
{
    htm_anchor_display_cursor = set;
}


//-----------------------------------------------------------------------------
// Color resources

void
htmWidget::setAllowBodyColors(bool set)
{
    if (htm_body_colors_enabled == set)
        return;
    htm_body_colors_enabled = set;

    htm_cm.cm_body_fg             = htm_cm.cm_body_fg_save;
    htm_cm.cm_body_bg             = htm_cm.cm_body_bg_save;
    htm_cm.cm_anchor_fg           = htm_cm.cm_anchor_fg_save;
    htm_cm.cm_anchor_visited_fg   = htm_cm.cm_anchor_visited_fg_save;
    htm_cm.cm_anchor_activated_fg = htm_cm.cm_anchor_activated_fg_save;
    if (htm_initialized) {
        htm_reformat_needed = true;
        trySync();
    }
}


void
htmWidget::setAllowColorSwitching(bool set)
{
    if (htm_allow_color_switching == set)
        return;
    htm_allow_color_switching = set;
    if (htm_initialized) {
        htm_reformat_needed = true;
        trySync();
    }
}


//-----------------------------------------------------------------------------
// Display control resources

// Freeze the widget display (increment call counter).
//
void
htmWidget::freeze()
{
    htm_frozen++;
}

// Decrement freeze counter and redisplay if counter becomes empty.
//
void
htmWidget::thaw()
{
    if (!htm_frozen)
        return;

    htm_frozen--;
    trySync();
}


// Force a reformat and redisplay.
//
void
htmWidget::redisplay()
{
    if (htm_initialized) {
        htm_reformat_needed = true;
        trySync();
    }
}


//-----------------------------------------------------------------------------
// Document resources

void
htmWidget::setMimeType(const char *mime_type)
{
    delete [] (htm_mime_type);
    htm_mime_type = lstring::copy(mime_type ? mime_type : "text/html");
}


void
htmWidget::setBadHtmlWarnings(bool set)
{
    htm_bad_html_warnings = set;
}


void
htmWidget::setAlignment(TextAlignment align)
{
    if (htm_enable_outlining)
        htm_default_halign = HALIGN_JUSTIFY;
    else {
        // default alignment depends on string direction
        htm_alignment = align;
        if (align == ALIGNMENT_BEGINNING) {
            if (htm_string_r_to_l)
                htm_default_halign = HALIGN_RIGHT;
            else
                htm_default_halign = HALIGN_LEFT;
        }
        if (align == ALIGNMENT_END) {
            if (htm_string_r_to_l)
                htm_default_halign = HALIGN_LEFT;
            else
                htm_default_halign = HALIGN_RIGHT;
        }
        else if (align == ALIGNMENT_CENTER)
            htm_default_halign = HALIGN_CENTER;
    }
    if (htm_initialized) {
        htm_parse_needed = true;
        htm_reformat_needed = true;
        trySync();
    }
}


void
htmWidget::setOutline(bool set)
{
    htm_enable_outlining = set;
    if (htm_initialized) {
        htm_reformat_needed = true;
        trySync();
    }
}


void
htmWidget::setVmargin(unsigned int marg)
{
    if (htm_margin_height == marg)
        return;

    htm_margin_height = marg;
}


void
htmWidget::setHmargin(unsigned int marg)
{
    if (htm_margin_width == marg)
        return;

    htm_margin_width = marg;
}


// Set the raw input text and display.
//
void
htmWidget::setSource(const char *new_source)
{
    bool need_parse = false;
    if (htm_source){
        if (new_source) {
            if (strcmp(new_source, htm_source)) {
                need_parse = true;
                delete [] htm_source;
                htm_source = lstring::copy(new_source);
            }
        }
        else {
            // clear current text
            need_parse = true;
            delete [] htm_source;
            htm_source = 0;
        }
    }
    else if (new_source){
        need_parse = true;
        htm_source = lstring::copy(new_source);
    }
    htm_parse_needed = need_parse;
    trySync();
}


// Return a a pointer to the original, unmodified document.
//
const char *
htmWidget::getSource()
{
    return (htm_source);
}


// Composes a text buffer consisting of the parser output.  This
// return buffer is not necessarely equal to the original document as
// the document verification and repair routines are capable of
// modifying the original rather heavily.
//
// The return value from this function must be freed by the caller.
//
char *
htmWidget::getString()
{
    return (getObjectString());
}


// Return the version number of the widget.
//
const char *
htmWidget::getVersion()
{
    return (HTM_VERSION);
}


// Return the value of the <title></title> element.
//
char *
htmWidget::getTitle()
{
    htmObject *tmp;
    for (tmp = htm_elements;
        tmp && tmp->id != HT_TITLE && tmp->id != HT_BODY;
        tmp = tmp->next) ;

    // sanity check
    if (!tmp || !tmp->next || tmp->id == HT_BODY)
        return (0);

    // ok, we have reached the title element, pick up the text
    tmp = tmp->next;

    // another sanity check
    if (!tmp->element)
        return (0);

    // skip leading...
    char *start;
    for (start = tmp->element; *start && isspace(*start); start++) ;

    // ...and trailing whitespace
    char *end;
    for (end = &start[strlen(start)-1]; *end && isspace(*end); end--) ;

    // always backs up one to many
    end++;

    // sanity
    if (*start == 0 || (end - start) <= 0)
        return (0);

    // duplicate the title
    char *t = htm_strndup(start, end - start);

    // expand escape sequences
    char *ret_val = expandEscapes(t, htm_bad_html_warnings, this);
    delete [] t;

    // and return to caller
    return (ret_val);
}


//-----------------------------------------------------------------------------
// Event and callback resources


//-----------------------------------------------------------------------------
// Formatted document resources


//-----------------------------------------------------------------------------
// Font resources

void
htmWidget::setStringRtoL(bool r_to_l)
{
    if (htm_string_r_to_l == r_to_l)
        return;
    htm_string_r_to_l = r_to_l;
    if (htm_initialized) {
        htm_parse_needed  = true;
        htm_reformat_needed = true;
        trySync();
    }
}


void
htmWidget::setAllowFontSwitching(int set)
{
    if (htm_allow_font_switching == set)
        return;
    htm_allow_font_switching = set;
    if (htm_initialized) {
        htm_reformat_needed = true;
        trySync();
    }
}


// Set the proportional font family.  The first argument is the face
// name, e.g. "Times" or "Helvetica", or can be null to keep the
// present font.  The second argument, if larger than 0, is taken as
// the base font size and the family sizes are scaled to this.
// Otherwise, the default sizes are used.
//
// Note:  this will reset the fixed-font normal size to the standard
// size.
//
void
htmWidget::setFontFamily(const char *family, int base_size)
{
    if (family) {
        delete [] htm_font_family;
        htm_font_family = lstring::copy(family);
    }
    if (base_size <= 0)
        base_size = htm_default_font_sizes.normal;
    htm_font_sizes.set_scaled(&htm_default_font_sizes, base_size);

    loadDefaultFont();
    if (htm_initialized) {
        htm_reformat_needed = true;
        trySync();
    }
}


// Set the fixed-pitch font family.  The first argument is the face
// name, e.g. "Monospace" or "Courier", or can be null to keep the
// present font.  The second argument, if larger than 0, is the normal
// size of the fixed font.  Other sizes will be assigned from the
// standard sizes.  The normal size might be different from the
// standard size for aesthetic reasons.  If the value is passed
// negative or zero, the standard size will be used.
//
void
htmWidget::setFixedFontFamily(const char *family, int fixed_norm_size)
{
    if (family) {
        delete [] htm_font_family_fixed;
        htm_font_family_fixed = lstring::copy(family);
    }
    if (fixed_norm_size < 0)
        fixed_norm_size = htm_default_font_sizes.fixed_normal;
    htm_font_sizes.fixed_normal = fixed_norm_size;

    if (htm_initialized) {
        htm_reformat_needed = true;
        trySync();
    }
}


void
htmWidget::setFontSizes(htmFontSizes *fs)
{
    htm_font_sizes = *fs;
    loadDefaultFont();
    if (htm_initialized) {
        htm_reformat_needed = true;
        trySync();
    }
}


//-----------------------------------------------------------------------------
// Form resources

void
htmWidget::setAllowFormColoring(bool set)
{
    htm_allow_form_coloring = set;
}


//-----------------------------------------------------------------------------
// Frame resources

//-----------------------------------------------------------------------------
// Image resources


void
htmWidget::setImagemapDraw(int set)
{
    htm_imagemap_draw = set;
}


void
htmWidget::setAllowImages(bool set)
{
    if (htm_images_enabled == set)
        return;
    htm_images_enabled = set;
    if (htm_initialized) {
        htm_free_images_needed = true;
        htm_reformat_needed = true;
        trySync();
    }
}


void
htmWidget::setRgbConvMode(RGBconvType dtype)
{
    htm_rgb_conv_mode = dtype;
}


void
htmWidget::setPerfectColors(Availability mode)
{
    htm_perfect_colors = mode;
}


void
htmWidget::setAlphaProcessing(Availability mode)
{
    htm_alpha_processing = mode;
}


void
htmWidget::setScreenGamma(float screen_gamma)
{
    htm_screen_gamma = screen_gamma;
}


void
htmWidget::setUncompressCommand(const char *cmd)
{
    delete [] htm_zCmd;
    htm_zCmd = lstring::copy(cmd ? cmd : HTM_DEFAULT_UNCOMPRESS);
}


void
htmWidget::setFreezeAnimations(bool set)
{
    if (htm_freeze_animations == set)
        return;

    htm_freeze_animations = set;
    restartAnimations();
}


//-----------------------------------------------------------------------------
// Scrollbar resources

// Return the position of the named anchor, or 0 if not found.
//
int
htmWidget::anchorPosByName(const char *anchor)
{
    htmObjectTable *anchor_data;
    if ((anchor_data = getAnchorByName(anchor)) != 0)
        // position one line above
        return (anchor_data->area.y - anchor_data->area.height);
    return (0);
}


// Return the position of the anchor with given id, or 0 if not found.
//
int
htmWidget::anchorPosById(int anchor_id)
{
    if (anchor_id < 0) {
        warning("anchorScrollToId", "%s passed.", "Invalid id");
        return (0);
    }

    htmObjectTable *anchor_data;
    if ((anchor_data = getAnchorByValue(anchor_id)) != 0)
        // position one line above
        return (anchor_data->area.y - anchor_data->area.height);
    return (0);
}


//-----------------------------------------------------------------------------
// Table resources


//-----------------------------------------------------------------------------
// Text selection resources

// Establish a text selection in the region specified.
//
void
htmWidget::selectRegion(int x1, int y1, int x2, int y2)
{
    int tx, ty, tw, th;
    selectionBB(&tx, &ty, &tw, &th);
    if (htm_text_selection) {
        delete (htm_text_selection);
        htm_text_selection = 0;
    }
    htm_select.clear();
    if (tw && th)
        repaint(tx, ty, tw, th);
    if (x1 || y1 || x2 || y2) {
        htm_select.x = (x1 < x2 ? x1 : x2);
        htm_select.y = (y1 < y2 ? y1 : y2);
        htm_select.width = abs(x1 - x2);
        htm_select.height = abs(y1 - y2);
        selection();
        selectionBB(&tx, &ty, &tw, &th);
        if (tw && th)
            repaint(tx, ty, tw, th);
        else
            htm_select.clear();
    }
    htm_tk->tk_claim_selection(htm_text_selection);
}



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
 * $Id: htm_widget.cc,v 1.30 2017/04/19 12:56:38 stevew Exp $
 *-----------------------------------------------------------------------*/

#include "htm_widget.h"
#include "htm_parser.h"
#include "htm_format.h"
#include "htm_form.h"
#include "htm_font.h"
#include "htm_table.h"
#include "htm_image.h"
#include "htm_callback.h"
#include "htm_string.h"

#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>

// Default horizontal & vertical marginwidth.
//
#define HTML_DEFAULT_MARGIN 4

//-----------------------------------------------------------------------------
// Constuctor, destructor, and initialization

htmWidget::htmWidget(htmInterface *tk_ptr, htmDataInterface *if_ptr) :
    htm_cm(this), htm_im(this)
{
    htm_tk = tk_ptr;
    htm_if = if_ptr;

    // Anchor resources
    htm_anchor_display_cursor   = true;
    htm_highlight_on_enter      = true;
    htm_anchor_position_x       = 0;
    htm_anchor_position_y       = 0;
    htm_armed_anchor            = 0;
    htm_anchor_current_cursor_element   = 0;

    htm_anchor_style            = ANC_SINGLE_LINE;
    htm_anchor_visited_style    = ANC_SINGLE_LINE;
    htm_anchor_target_style     = ANC_SINGLE_LINE;

    htm_press_x                 = 0;
    htm_press_y                 = 0;
    htm_selected_anchor         = 0;
    htm_current_anchor          = 0;

    // Color resources
    htm_body_colors_enabled     = true;
    htm_body_images_enabled     = true;
    htm_allow_color_switching   = true;
    htm_allow_form_coloring     = true;
    htm_freeze_animations       = false;
    htm_body_image_url          = 0;
    htm_def_body_image_url      = 0;

    // Document resources
    htm_source                  = 0;
    htm_mime_type               = lstring::copy("text/html");
    htm_mime_id                 = 0;
    htm_enable_outlining        = false;
    htm_bad_html_warnings       = false;

    // Event and callback resources
    htm_events                  = 0;
    htm_nevents                 = 0;

    // Formatted document resources
    htm_formatted_width         = 1;
    htm_formatted_height        = 1;
    htm_nlines                  = 0;
    htm_formatted               = 0;
    htm_elements                = 0;

    htm_num_anchors             = 0;
    htm_num_named_anchors       = 0;
    htm_anchor_words            = 0;
    htm_anchors                 = 0;
    htm_named_anchors           = 0;
    htm_anchor_data             = 0;

    htm_paint_start             = 0;
    htm_paint_end               = 0;

    // Font resources
    htm_font_family             = lstring::copy("times");
    htm_font_family_fixed       = lstring::copy("courier");
    htm_default_font            = 0;
    htm_font_cache              = 0;
    htm_string_r_to_l           = false;
    htm_allow_font_switching    = true;
    htm_alignment               = ALIGNMENT_BEGINNING;
    htm_default_halign          = HALIGN_NONE;

    // Form resources
    htm_form_data               = 0;
    htm_current_form            = 0;
    htm_current_entry           = 0;

    // Frame resources
    htm_frame_border            = 0;
    htm_nframes                 = 0;
    htm_frames                  = 0;
    htm_frameset                = 0;

    // Image resources
    htm_images_enabled          = true;
    htm_imagemap_draw           = false;
    htm_max_image_colors        = MAX_IMAGE_COLORS;
    htm_rgb_conv_mode           = BEST;
    htm_perfect_colors          = AUTOMATIC;
    htm_alpha_processing        = ALWAYS;
    htm_screen_gamma            = 2.2;
    htm_zCmd                    = lstring::copy(HTM_DEFAULT_UNCOMPRESS);

    // Table resources
    htm_tables                  = 0;

    // Text selection resources
    htm_text_selection          = 0;
    htm_search                  = 0;

    // Misc. Resources
    htm_margin_width            = HTML_DEFAULT_MARGIN;
    htm_margin_height           = HTML_DEFAULT_MARGIN;
    htm_viewarea.width          = 1;
    htm_viewarea.height         = 1;

    htm_in_layout               = false;

    htm_ready                   = false;
    htm_initialized             = false;
    htm_parse_needed            = false;
    htm_new_source              = false;
    htm_reformat_needed         = false;
    htm_layout_needed           = false;
    htm_redraw_needed           = false;
    htm_free_images_needed      = false;
    htm_frozen                  = 0;
}


htmWidget::~htmWidget()
{
    htm_im.killPLCCycler();

    freeEventDatabase();

    clearSearch();
    delete [] htm_source;
    delete [] htm_mime_type;
    htmObjectTable::destroy(htm_formatted);
    htmObject::destroy(htm_elements);
    delete [] htm_anchors;
    delete [] htm_named_anchors;
    htmAnchor::destroy(htm_anchor_data);
    delete [] htm_font_family;
    delete [] htm_font_family_fixed;
    htmFormData::destroy(htm_form_data);
    delete [] htm_zCmd;
    htmTable::destroy(htm_tables);

    destroyFrames();

    // free all fonts for this widget's instance
    delete htm_font_cache;
}


htmImageInfo *
htmWidget::imageLoadProc(const char *file)
{
    return (htm_im.imageLoadProc(file));
}


void
htmWidget::initialize()
{
    // Initialize private resources

    htm_body_image_url      = 0;

    // Formatted document resources
    htm_formatted_width     = 1;
    htm_formatted_height    = 1;
    htm_elements            = 0;
    htm_formatted           = 0;
    htm_nlines              = 0;
    htm_in_layout           = false;

    // layout & paint engine resources
    htm_paint_start         = 0;
    htm_paint_end           = 0;
    htm_paint.clear();
    htm_form_data           = 0;
    htm_current_form        = 0;
    htm_current_entry       = 0;

    // anchor resources
    htm_anchor_position_x   = 0;
    htm_anchor_position_y   = 0;
    htm_armed_anchor        = 0;
    htm_anchor_current_cursor_element = 0;

    htm_press_x             = 0;
    htm_press_y             = 0;
    htm_selected_anchor     = 0;
    htm_current_anchor      = 0;

    htm_num_anchors         = 0;
    htm_num_named_anchors   = 0;
    htm_anchors             = 0;
    htm_anchor_words        = 0;
    htm_named_anchors       = 0;
    htm_anchor_data         = 0;

    // Text selection resources
    htm_text_selection      = 0;
    htm_select.clear();

    // HTML Frame resources
    htm_nframes             = 0;
    htm_frames              = 0;
    htm_frameset            = 0;

    // Table resources
    htm_tables              = 0;

    // HTML4.0 Event database
    htm_events              = 0;
    htm_nevents             = 0;

    htm_im.initialize();
    htm_cm.initialize();

    // initial mimetype
    setMimeId();

    loadDefaultFont();

    htm_tk->tk_window_size(htmInterface::DRAWABLE,
        &htm_viewarea.width, &htm_viewarea.height);

    htm_initialized = true;
    htm_parse_needed = true;
}


void
htmWidget::trySync()
{
    if (!htm_ready)
        return;
    if (!htm_initialized)
        initialize();
    if (htm_parse_needed)
        parse();

    htm_frozen++;
    if (htm_reformat_needed)
        reformat();
    if (htm_layout_needed)
        layout();
    htm_frozen--;

    if (htm_frozen)
        return;
    if (htm_redraw_needed)
        redraw();
}


void
htmWidget::parse()
{
    // new text has been set, kill of any existing PLC's
    htm_im.killPLCCycler();

    // release event database
    freeEventDatabase();

    // destroy any form data
    destroyForms();

    // destroy any existing frames
    destroyFrames();

    // Parse the raw HTML text
    parseInput();

    // keep current frame setting and check if new frames are allowed
    htm_nframes = checkForFrames(htm_elements);

    // trigger link callback
    linkCallback();

    htm_parse_needed        = false;
    htm_new_source          = true;
    htm_reformat_needed     = true;
    htm_free_images_needed  = true;
}


void
htmWidget::reformat()
{
    // Now format the list of parsed objects.  Don't do a bloody thing
    // if we are already in layout as this will cause unnecessary
    // reloading and screen flickering.
    if (htm_in_layout)
        return;

    // destroy any form data
    htmFormData::destroy(htm_form_data);
    htm_form_data = 0;

    // free anchor word data
    if (htm_anchor_words)
        delete [] htm_anchors;
    htm_anchors = 0;
    htm_anchor_words = 0;

    // free named anchor data
    if (htm_num_named_anchors)
        delete [] htm_named_anchors;
    htm_named_anchors = 0;
    htm_num_named_anchors = 0;

    // clear the images if we have to
    htm_im.freeResources(htm_free_images_needed);

    // reset some important vars
    htm_anchor_current_cursor_element = 0;
    htm_armed_anchor        = 0;
    htm_current_anchor      = 0;
    htm_selected_anchor     = 0;
    htm_text_selection      = 0;
    htm_select.clear();
    htm_viewarea.x          = 0;
    htm_viewarea.y          = 0;
    htm_formatted_width     = 1;
    htm_formatted_height    = 1;
    htm_paint_start         = 0;
    htm_paint_end           = 0;
    htm_paint.clear();

    if (htm_free_images_needed)
        htm_body_image_url = 0;

    htm_cm.reset();

    htm_tk->tk_set_background(htm_cm.cm_body_bg);

    // get new values for top, bottom and highlight
    htm_cm.recomputeColors(htm_cm.cm_body_bg);

    // formatting is done here
    format();

    // and check for possible external imagemaps
    htm_im.checkImagemaps();

    htm_reformat_needed = false;
    htm_layout_needed   = true;
}


// Main layout algorithm.  Computes text layout and configures the
// scrollbars.  Also does handles image recreation.
//
void
htmWidget::layout()
{
    if (!configureFrames()) {
        htm_frames = 0;
        htm_nframes = 0;
    }

    // clear selection, since selection area is bogus
    selectRegion(0, 0, 0, 0);

    // remember current vertical position if we have been scrolled
    htmObjectTable *curr_ele = 0;
    if (htm_viewarea.y)
        curr_ele = getLineObject(htm_viewarea.y);

    unsigned int width, height;
    htm_tk->tk_window_size(htmInterface::VIEWABLE, &width, &height);
    if (width <= 1)
        return;

    // set blocking flag
    htm_in_layout = true;

    // this is the viewable area without scrollbars
    unsigned int scrollbar_width = htm_tk->tk_scrollbar_width();
    htm_viewarea.width = width - 2*htm_margin_width;
    htm_viewarea.height = 0;
    computeLayout();

    // try to avoid vertical scrollbar if possible
    if (htm_formatted_height > height) {
        if (htm_formatted_width > width - scrollbar_width &&
                htm_formatted_width <= width) {
            htm_viewarea.width = width - scrollbar_width - 2*htm_margin_width;
            htm_viewarea.height = 0;
            computeLayout();
        }
    }

    // set new vertical scrollbar positions
    if (curr_ele != 0)
        htm_viewarea.y = curr_ele->area.y;
    else
        htm_viewarea.y = 0;

    // communicate the formatted size
    htm_tk->tk_resize_area(htm_formatted_width + 2*htm_margin_width,
        htm_formatted_height);

    // set the size of the drawing area
    htm_tk->tk_window_size(htmInterface::DRAWABLE, &htm_viewarea.width,
        &htm_viewarea.height);

    htm_in_layout       = false;
    htm_layout_needed   = false;
    htm_redraw_needed   = true;
}


void
htmWidget::resize()
{
    if (!htm_in_layout && htm_initialized)
        layout();
}


//-----------------------------------------------------------------------------
// Display control and painting

void
htmWidget::redraw()
{
    if (htm_free_images_needed) {
        for (htmImage *img = htm_im.im_images; img; img = img->next) {
            if (!img->InfoFreed() && img->html_image->DelayedCreation()) {
                img->options |= IMG_DELAYED_CREATION;
                htm_im.im_delayed_creation = true;
            }
        }
        if (htm_im.im_delayed_creation)
            htm_im.imageCheckDelayedCreation();
    }
    positionAndShowForms();
    repaint(htm_viewarea.x, htm_viewarea.y,
        htm_viewarea.width, htm_viewarea.height);

    // If the formatted area is smaller than the view area, have to
    // update the surrounding area - it hasn't been updated.

    if (htm_formatted_width < htm_viewarea.width)
        htm_tk->tk_refresh_area(htm_last_paint.right(), 0,
            htm_viewarea.width - htm_last_paint.width, htm_viewarea.height);
    if (htm_formatted_height < htm_viewarea.height)
        htm_tk->tk_refresh_area(0, htm_last_paint.height,
            htm_viewarea.width, htm_viewarea.height - htm_last_paint.height);

    // if new text has been set, fire up the PLCCycler
    if (htm_new_source)
        htm_im.im_plc_suspended = false;

    htm_redraw_needed   = false;
    htm_new_source      = false;
    htm_free_images_needed = false;
}


// Main screen refresher:  given an exposure rectangle, this function
// determines the proper paint engine start and end points and calls
// the painter.
//
void
htmWidget::repaint(int x, int y, int width, int height)
{
    if (htm_frozen > 0) {
        htm_redraw_needed = true;
        return;
    }

    // Update background with the given region.  Must check if body
    // image hasn't been delayed or is being loaded progressively or
    // we will get some funny effects...

    int xv = viewportX(x);
    int yv = viewportY(y);
    bool bg_drawn = false;
    if (htm_im.im_body_image &&
            !htm_im.im_body_image->DelayedCreation() &&
            htmImageInfo::BodyImageLoaded(htm_im.im_body_image->html_image)) {

        // We need to figure out a correct starting point for the first
        // tile to be drawn (ts_[x,y]_origin in the GC).  We know the
        // region to update.  First we need to get the number of tiles
        // drawn so far.  Since we want the total number of tiles drawn,
        // we must add the scroll offsets to the region origin.

        int tile_width  = htm_im.im_body_image->width;
        int tile_height = htm_im.im_body_image->height;
        if (tile_width && tile_height) {

            int x_offset = x % tile_width;
            int y_offset = y % tile_height;
            htm_tk->tk_tile_draw_pixmap(xv - x_offset, yv - y_offset,
                htm_im.im_body_image->pixmap, xv, yv, width, height);
            bg_drawn = true;
        }
    }
    if (!bg_drawn) {
        htm_tk->tk_set_foreground(htm_cm.cm_body_bg);
        htm_tk->tk_draw_rectangle(true, xv, yv, width, height);
    }

    // make sure margins are updated
    htm_tk->tk_refresh_area(xv, yv, width, height);

    htm_paint.x = x;
    htm_paint.width = width;
    htm_paint.y = y;
    htm_paint.height = height;

    // bump up size a little to catch boundaries
    if (htm_paint.x >= 2)
        htm_paint.x -= 2;
    else
        htm_paint.x = 0;
    if (htm_paint.right() <= htm_viewarea.right() - 2)
        htm_paint.width += 2;
    else
        htm_paint.width = htm_viewarea.right() - htm_paint.left();
    if (htm_paint.y >= 2)
        htm_paint.y -= 2;
    else
        htm_paint.y = 0;
    if (htm_paint.bottom() <= (int)htm_formatted_height - 2)
        htm_paint.height += 2;
    else
        htm_paint.height = htm_formatted_height - htm_paint.top();

    // If the offset of the top of the exposed region is higher than
    // the max height of the text to display, the exposure region is
    // empty, so we just return here and leave everything untouched.

    if (htm_formatted && htm_paint.top() <= (int)htm_formatted_height) {
        // Find the start and end positions.  This replaces a big
        // block of code that would overlook images with text wrapped,
        // probably among other things.  I don't know how to avoid
        // searching the whole list.

        htmObjectTable *start = htm_formatted;
        while (start && start->area.bottom() < htm_paint.top())
            start = start->next;
        if (start == 0)
            start = htm_formatted;
        htmObjectTable *end = start;

        for (;;) {
            if (!end) {
                // can't happen
                end = start;
                break;
            }
            // Skip over left-aligned tables.
            if (end->object_type == OBJ_TABLE) {
                htmTable *table = end->table;
                if (table && table->t_children)
                    table = table->t_children;
                if (!table) {
                    end = end->next;
                    continue;
                }
                if (table->t_properties &&
                        table->t_properties->tp_halign == HALIGN_LEFT) {
                    if (!table->t_end) {
                        end = end->next;
                        continue;
                    }
                    end = table->t_end;
                    continue;
                }
            }
            if ((end->next && end->area.top() < htm_paint.bottom()) ||
                    end->object_type == OBJ_BULLET ||
                    (end->object && end->object->id == HT_BR)) {
                // Can't quit on a bullet, might be viewable objects
                // following ditto for breaks.
                end = end->next;
                continue;
            }

            // Bah! Above isn't adequate, keep looking until the end.
            htmObjectTable *tmp = end;
            while (tmp) {
                if (tmp->area.top() < htm_paint.bottom())
                    end = tmp;
                tmp = tmp->next;
            }
            break;
        }

        // set proper paint engine start and end
        htm_paint_start = start;
        htm_paint_end = end;

        htmRect r(xv, yv, width, height);
        htm_tk->tk_set_clip_rectangle(&r);
        paint(htm_paint_start, htm_paint_end);
        htm_tk->tk_set_clip_rectangle(0);
    }
}


namespace {
    htmObjectTable *
    anchor_start(htmObjectTable *anchor)
    {
        htmObjectTable *a_start;
        for (a_start = anchor; a_start && a_start->anchor == anchor->anchor;
            a_start = a_start->prev) ;
        if (!a_start)
            return (0);
        return (a_start->next);
    }
}


// Paint the current active in an activated state.  htmPaint will do
// the proper rendering.
//
void
htmWidget::paintAnchorSelected(htmObjectTable *anchor)
{
    anchor = anchor_start(anchor);
    if (!anchor)
        return;

    // save as the current active anchor
    htmObjectTable *start = htm_current_anchor = anchor;

    // pick up anchor end
    htmRect r;
    htmObjectTable *end;
    for (end = start; end && end->anchor == start->anchor; end = end->next) {
        end->anchor_state = ANCHOR_SELECTED;
        r.add(end->area);
    }

    // Due to anti-aliased fonts, have to redisplay the area, not simply
    // repaint the text with paint(start, end).
    repaint(r.x, r.y, r.width, r.height);
}


// Paint the currently active anchor in an unactivated state.
// htmPaint will do the proper rendering.
//
void
htmWidget::paintAnchorUnSelected()
{
    htmObjectTable *start = htm_current_anchor;
    if (!start)
        return;

    // pick up the anchor end. An anchor ends when the raw worddata changes
    htmRect r;
    htmObjectTable *end;
    for (end = start; end && end->anchor == start->anchor; end = end->next) {
        end->anchor_state = ANCHOR_UNSELECTED;
        r.add(end->area);
    }

    // Due to anti-aliased fonts, have to redisplay the area, not simply
    // repaint the text with paint(start, end).
    repaint(r.x, r.y, r.width, r.height);

    htm_current_anchor = 0;
}


// Paint a highlight on the given anchor.
//
void
htmWidget::enterAnchor(htmObjectTable *anchor)
{
    anchor = anchor_start(anchor);
    if (!anchor)
        return;

    // save as the current active anchor
    htmObjectTable *start = htm_armed_anchor = anchor;

    // pick up anchor end
    htmRect r;
    htmObjectTable *end;
    for (end = start; end && end->anchor == start->anchor; end = end->next) {
        end->anchor_state = ANCHOR_INSELECT;
        r.add(end->area);
    }

    // Due to anti-aliased fonts, have to redisplay the area, not simply
    // repaint the text with paint(start, end).
    repaint(r.x, r.y, r.width, r.height);
}


// Remove anchor highlighting.
//
void
htmWidget::leaveAnchor()
{
    // save as the current active anchor
    htmObjectTable *start = htm_armed_anchor;
    if (!start)
        return;

    // pick up the anchor end
    htmRect r;
    htmObjectTable *end;
    for (end = start; end && end->anchor == start->anchor; end = end->next) {
        end->anchor_state = ANCHOR_UNSELECTED;
        r.add(end->area);
    }

    // Due to anti-aliased fonts, have to redisplay the area, not simply
    // repaint the text with paint(start, end).
    repaint(r.x, r.y, r.width, r.height);

    htm_armed_anchor = 0;
}


//-----------------------------------------------------------------------------
// Information retrieval

// Retrieve the contents of an image and/or anchor at the given cursor
// position.  Returns a filled htmInfoStructure when the pointer was
// pressed on an image and/or anchor, 0 if not.
//
htmInfoStructure *
htmWidget::XYToInfo(int x, int y)
{
    static htmInfoStructure cbs;
    static htmAnchorCallbackStruct anchor_cbs;
    static htmImageInfo info;

    long line = -1;

    // default fields
    cbs.x      = x;
    cbs.y      = y;
    cbs.is_map = MAP_NONE;
    cbs.image  = 0;
    cbs.anchor = 0;
    line = -1;

    // pick up a possible anchor or imagemap location
    htmAnchor *anchor = 0;

    htmWord *anchor_word;
    if ((anchor_word = getAnchor(x, y)) == 0)
        anchor = getImageAnchor(x, y);

    // no regular anchor, see if it's an imagemap
    if (anchor == 0 && anchor_word)
        anchor = anchor_word->owner->anchor;

    // Final check:  if this anchor is a form component it can't be
    // followed as this is an internal-only anchor.

    if (anchor && anchor->url_type == ANCHOR_FORM_IMAGE)
        anchor = 0;

    // check if we have an anchor
    if (anchor) {
        // set to zero
        memset(&anchor_cbs, 0, sizeof(htmAnchorCallbackStruct));

        // initialize callback fields
        anchor_cbs.reason   = HTM_ACTIVATE;
        anchor_cbs.event    = 0;
        anchor_cbs.url_type = anchor->url_type;
        anchor_cbs.line     = anchor->line;
        anchor_cbs.href     = anchor->href;
        anchor_cbs.target   = anchor->target;
        anchor_cbs.rel      = anchor->rel;
        anchor_cbs.rev      = anchor->rev;
        anchor_cbs.title    = anchor->title;
        anchor_cbs.doit     = false;
        anchor_cbs.visited  = anchor->visited;

        line                = anchor->line;
    }

    // check if we have an image
    htmImage *image;
    if ((image = onImage(x, y)) != 0) {
        // set map type
        cbs.is_map = (image->map_type != MAP_NONE);

        if (image->html_image != 0) {
            // no image info if this image is being loaded progressively
            if (!image->html_image->Progressive()) {
                // use real url but link all other members
                info        = *image->html_image;
                info.url    = image->url;
                // set it
                cbs.image   = &info;
            }
        }
        else {
            // htmImageInfo has been freed, construct one
            // set to zero
            memset(&info, 0, sizeof(htmImageInfo));
            // fill in the fields we know
            info.url        = image->url;
            info.type       = IMAGE_UNKNOWN;
            info.width      = image->swidth;
            info.height     = image->sheight;
            info.swidth     = image->width;
            info.sheight    = image->height;
            info.ncolors    = image->npixels;
            info.nframes    = image->nframes;
            // set it
            cbs.image       = &info;
        }
        if (line == -1)
            line = (image->owner ? image->owner->line : (unsigned int)-1);
    }
    // no line number yet, get one
    if (line == -1)
        cbs.line = verticalPosToLine(y);
    else
        cbs.line = line;
    return (&cbs);
}


// Check whether the given positions fall within an image.
//
htmImage*
htmWidget::onImage(int x, int y)
{
    htmImage *image;
    for (image = htm_im.im_images; image; image = image->next) {
        if (image->owner && image->owner->area.includes(x, y))
            return (image);
    }
    return (0);
}


// Get the object located at the given y position.
//
htmObjectTable*
htmWidget::getLineObject(int y_pos)
{
    // y_pos given must fall in the bounding box of an element.  We
    // try to be a little bit smart here:  If we have a paint engine
    // end and it's y position is below the requested position, walk
    // forwards until we find a match.  If we have a paint engine
    // start and it's y position is below the requested position, walk
    // forwards.  If it's above the requested position, walk
    // backwards.  We are always bound to find a matching element.

    htmObjectTable *tmp = 0;
    if (htm_paint_end || htm_paint_start) {
        // located above paint engine end, walk forwards
        if (htm_paint_end && htm_paint_end->area.y < y_pos) {
            for (tmp = htm_paint_end; tmp; tmp = tmp->next)
                if (y_pos >= tmp->area.top() && y_pos < tmp->area.bottom())
                    break;
        }
        else if (htm_paint_start) {
            // located above paint engine start, walk forwards
            if (htm_paint_start->area.y < y_pos) {
                for (tmp = htm_paint_start; tmp; tmp = tmp->next)
                    if (y_pos >= tmp->area.top() && y_pos < tmp->area.bottom())
                        break;
            }
            // located under paint engine start, walk backwards
            else {
                for (tmp = htm_paint_start; tmp; tmp = tmp->prev)
                    if (y_pos >= tmp->area.top() && y_pos < tmp->area.bottom())
                        break;
            }
        }
    }
    else
        for (tmp = htm_formatted; tmp; tmp = tmp->next)
            if (y_pos >= tmp->area.top() && y_pos < tmp->area.bottom())
                break;

    if (tmp) {
        // Look for other candidates, choose the one with top closest to
        // y_pos.

        htmObjectTable *dtmp = tmp;
        int dmin = y_pos - tmp->area.top();
        for (tmp = tmp->next; tmp; tmp = tmp->next) {
            if (y_pos >= tmp->area.top() && y_pos < tmp->area.bottom()) {
                if (y_pos - tmp->area.top() < dmin) {
                    dmin = y_pos - tmp->area.top();
                    dtmp = tmp;
                }
                continue;
            }
            if (y_pos < tmp->area.top())
                break;
        }
        tmp = dtmp;
    }

    // top or bottom element
    if (tmp == 0 || tmp->prev == 0) {
        // bottom element
        if (tmp == 0)
            return (htm_formatted);
        // top element otherwise
        return (0);
    }
    return (tmp);
}


// Translate a vertical position to the line number in the displayed
// document.
//
int
htmWidget::verticalPosToLine(int y)
{
    if (!htm_formatted)
        return (0);
    htmObjectTable *tmp  = getLineObject(y);
    if (tmp) {
        int top_line = tmp->line;

        // If the current element has got more than one word in it,
        // and these words span accross a number of lines, adjust the
        // linenumber.

        if (tmp->n_words > 1 && tmp->words[0].ybaseline !=
                tmp->words[tmp->n_words-1].ybaseline) {
            int i;
            for (i = 0 ; i < tmp->n_words && tmp->words[i].ybaseline < y;
                i++) ;
            if (i != tmp->n_words)
                top_line = tmp->words[i].line;
        }
        return (top_line);
    }
    return (0);
}


// Determine if the given x and y positions are within the bounding
// rectangle of an anchor.  We include the interword spacing in the
// search.
//
htmWord*
htmWidget::getAnchor(int x, int y)
{
    for (int i = 0; i < htm_anchor_words; i++) {
        htmWord *anchor = htm_anchors[i];
        htmRect r(anchor->area);
        if (i+1 < htm_anchor_words &&
                anchor->owner->anchor == htm_anchors[i+1]->owner->anchor &&
                anchor->line == htm_anchors[i+1]->line)
            r.width = htm_anchors[i+1]->area.x - anchor->area.x + 2;
        if (r.includes(x, y)) {
            // store line number
            anchor->owner->anchor->line = anchor->line;
            return (anchor);
        }
    }
    return (0);
}


// Return the named anchor data.
//
htmObjectTable *
htmWidget::getAnchorByName(const char *anchor)
{
    // see if it is indeed a named anchor
    if (!anchor || !*anchor || anchor[0] != '#')
        return (0);   // fix 02/03/97-04, kdh

    // we start right after the leading hash sign
    const char *chPtr = &anchor[1];

    for (int i = 0; i < htm_num_named_anchors; i++) {
        htmObjectTable *anchor_data = htm_named_anchors[i];
        if (anchor_data && anchor_data->anchor && anchor_data->anchor->name &&
                !strcmp(anchor_data->anchor->name, chPtr))
            return (anchor_data);
    }
    // hrumph, no exact match, find something close
    for (int i = 0; i < htm_num_named_anchors; i++) {
        htmObjectTable *anchor_data = htm_named_anchors[i];
        if (anchor_data && anchor_data->anchor && anchor_data->anchor->name &&
                strstr(anchor_data->anchor->name, chPtr))
            return (anchor_data);
    }
    return (0);
}


// Return the named anchor data.
//
htmObjectTable *
htmWidget::getAnchorByValue(int anchor_id)
{
    // this should always match
    htmObjectTable *anchor_data = htm_named_anchors[anchor_id];
    if (!anchor_data)
        return (0);
    if ((int)anchor_data->id == anchor_id)
        return (anchor_data);

    // hmm, something went wrong, search the whole list of named anchors
    warning("getAnchorByValue",
        "Misaligned anchor stack (id=%i), trying to recover.", anchor_id);

    for (int i = 0; i < htm_num_named_anchors; i++) {
        anchor_data = htm_named_anchors[i];
        if (anchor_data && (int)anchor_data->id == anchor_id)
            return (anchor_data);
    }
    return (0);
}


// Determine if the given x and y positions lie upon an image that has
// an imagemap.
//
htmAnchor*
htmWidget::getImageAnchor(int x, int y)
{
    // don't do this if we haven't got any imagemaps
    if (htm_im.im_image_maps == 0)
        return (0);

    for (htmImage *image = htm_im.im_images; image; image = image->next) {
        if (image->owner && image->owner->area.includes(x, y)) {
            if (image->map_type != MAP_NONE) {
                if (image->map_type == MAP_SERVER) {
                    warning("getImageAnchor",
                        "Server side imagemaps not supported yet.");
                    return (0);
                }
                htmImageMap *imagemap;
                if ((imagemap = htm_im.getImagemap(image->map_url)) != 0) {
                    htmAnchor *anchor;
                    if ((anchor = htm_im.getAnchorFromMap(x, y, image,
                            imagemap)) != 0)
                        return (anchor);
                }
            }
        }
    }
    return (0);
}


// Return the internal id of an anchor.
//
int
htmWidget::anchorGetId(const char *anchor)
{
    htmObjectTable *anchor_data;
    if ((anchor_data = getAnchorByName(anchor)) != 0)
        return (anchor_data->id);
    return (-1);
}


//-----------------------------------------------------------------------------
// Misc. event handling

// Motion event handler.
//
void
htmWidget::anchorTrack(htmEvent *event, int x, int y)
{
    htmAnchor *anchor = 0;
    htmWord *anchor_word = 0;

    // try to get current anchor element (if any)
    if (((anchor_word = getAnchor(x, y)) == 0) &&
            ((anchor = getImageAnchor(x, y)) == 0)) {
        // invalidate current selection if there is one
        if (htm_anchor_current_cursor_element)
            trackCallback(event, 0);

        if (htm_highlight_on_enter && htm_armed_anchor)
            leaveAnchor();

        htm_armed_anchor = 0;
        htm_anchor_current_cursor_element = 0;
        if (htm_anchor_display_cursor)
            htm_tk->tk_set_anchor_cursor(false);
        return;
    }

    if (anchor == 0)
        anchor = anchor_word->owner->anchor;

    // trigger callback and set cursor if we are entering a new element
    if (anchor != htm_anchor_current_cursor_element) {
        // remove highlight of previous anchor
        if (htm_highlight_on_enter) {
            if (anchor_word) {
                // unarm previous selection
                if (htm_armed_anchor &&
                        htm_armed_anchor != anchor_word->owner)
                    leaveAnchor();
                // highlight new selection
                enterAnchor(anchor_word->owner);
            }
            else
                // unarm previous selection
                if (htm_armed_anchor)
                    leaveAnchor();

        }

        htm_anchor_current_cursor_element = anchor;
        trackCallback(event, anchor);
        if (htm_anchor_display_cursor)
            htm_tk->tk_set_anchor_cursor(true);
    }
}


// ButtonPress event handler.  Initializes a selection when not over
// an anchor, else paints the anchor as being selected.
//
void
htmWidget::extendStart(htmEvent *event, int button, int x, int y)
{
    // try to get current anchor element
    htmAnchor *anchor = 0;
    htmWord *anchor_word = 0;
    if (button != 3 &&
            ((anchor_word = getAnchor(x, y)) != 0 ||
            (anchor = getImageAnchor(x, y)) != 0)) {

        // User has selected an anchor.  Get the text for this anchor.
        // Note:  if anchor is 0 it means the user was over a real
        // anchor (regular anchor, anchored image or a form image
        // button) and anchor_word is nonzero (this object the
        // referenced URL).  If it is nonzero the mouse was over an
        // imagemap, in which case we may not show visual feedback to
        // the user.

        if (anchor == 0) {
            // store anchor & paint as selected
            anchor = anchor_word->owner->anchor;

            // uncheck currently selected anchor if it's not the same
            // as the current anchor (mouse dragging)

            if (htm_current_anchor != 0 &&
                    htm_current_anchor != anchor_word->owner)
                paintAnchorUnSelected();

            paintAnchorSelected(anchor_word->owner);
        }
        else if (htm_selected_anchor && htm_selected_anchor != anchor)
            paintAnchorUnSelected();

        // check for the onMouseDown event
        if (anchor->events && anchor->events->onMouseDown)
            processEvent(event, anchor->events->onMouseDown);

        htm_selected_anchor = anchor;
    }
    else if (htm_current_anchor != 0) {
        // not over an anchor, unselect current anchor and reset cursor
        paintAnchorUnSelected();
        htm_tk->tk_set_anchor_cursor(false);
    }

    // remember pointer position
    htm_press_x = x;
    htm_press_y = y;

    if (!anchor_word && !anchor && htm_if) {
        htmCallbackInfo cbs(HTM_ARM, event);
        htm_if->emit_signal(S_ARM, &cbs);
    }
}


// ButtonRelease event handler.  Terminates the selection
// initiated by ExtendStart.  When over an anchor, paints the anchor
// as being deselected.  activateCallback or armCallback callback
// resources are only called if the buttonpress and release occur
// within a certain time limit HTML_MAX_BUTTON_RELEASE_TIME
//
void
htmWidget::extendEnd(htmEvent *event, int, bool clicked, int x, int y)
{
    // try to get current anchor element
    htmAnchor *anchor = 0;
    htmWord *anchor_word = 0;
    if ((anchor_word = getAnchor(x, y)) != 0 ||
            (anchor = getImageAnchor(x, y)) != 0) {

        // OK, release took place over an anchor, see if it falls
        // within the allowable time limit and we are still over the
        // anchor selected by extendStart.

        if (anchor == 0)
            anchor = anchor_word->owner->anchor;

        // If we already have an active anchor and it's different from
        // the current anchor, deselect it.

        if (htm_current_anchor &&
                htm_current_anchor != anchor_word->owner)
            paintAnchorUnSelected();

        // see if we need to serve the mouseUp event
        if (anchor->events && anchor->events->onMouseUp)
            processEvent(event, anchor->events->onMouseUp);

        // this anchor is still in selection
        if (anchor_word)
            enterAnchor(anchor_word->owner);

        // If we had a selected anchor and it's equal to the current
        // anchor and the button was released in time, trigger the
        // activation callback.

        if (htm_selected_anchor && anchor == htm_selected_anchor && clicked) {
            // check for the onClick event
            if (anchor->events && anchor->events->onClick)
                processEvent(event, anchor->events->onClick);

            if (anchor->url_type == ANCHOR_FORM_IMAGE)
                formActivate(event, anchor_word->form);
            else
                // trigger activation callback
                activateCallback(event, anchor);
            return;
        }
    }

    // unset any previously selected anchor
    if (htm_current_anchor) {
        // keep current anchor selection or unset it
        if (anchor_word)
            enterAnchor(anchor_word->owner);
        else
            paintAnchorUnSelected();
    }

    if (htm_current_anchor)
        // inhibit new selection
        selectRegion(0, 0, 0, 0);
    else
        selectRegion(htm_press_x, htm_press_y, x, y);
}


//-----------------------------------------------------------------------------
// Selections

// Identify selected text, place string in htm_text_selection.
//
void
htmWidget::selection()
{
    delete (htm_text_selection);
    htm_text_selection = 0;

    int lastline = -1, lastspa = 0;
    sLstr ls;
    htmObjectTable *tmp;
    for (tmp = htm_formatted; tmp; tmp = tmp->next) {
        if (tmp->object_type != OBJ_TEXT && tmp->object_type != OBJ_PRE_TEXT)
            continue;

        bool inpre = (tmp->object_type == OBJ_PRE_TEXT);
        int n_words = tmp->n_words;
        htmWord *words = tmp->words;
        for (int i = 0; i < n_words; i++) {
            if (words[i].type != OBJ_TEXT)
                continue;

            if (htm_select.intersect(words[i].area)) {
                if ((int)words[i].line != lastline && lastline >= 0) {
                    ls.add("\n");
                    lastline = words[i].line;
                    lastspa = 0;
                }
                else if (lastline >= 0) {
                    if (inpre || (lastspa & TEXT_SPACE_TRAIL) ||
                            (words[i].spacing & TEXT_SPACE_LEAD))
                        ls.add(" ");
                }
                else
                    lastline = words[i].line;
                ls.add(words[i].word);
                lastspa = words[i].spacing;
            }
        }
    }
    htm_text_selection = ls.string_trim();
}


// Return the bounding box of the text selection.
//
void
htmWidget::selectionBB(int *x, int *y, int *w, int *h)
{
    *x = 0;
    *y = 0;
    *w = 0;
    *h = 0;
    if (htm_select.left() == 0 && htm_select.top() == 0 &&
            htm_select.width == 0 && htm_select.height == 0)
        return;

    htmRect tw;
    bool didone = false;
    htmObjectTable *tmp;
    for (tmp = htm_formatted; tmp; tmp = tmp->next) {
        if (tmp->object_type != OBJ_TEXT && tmp->object_type != OBJ_PRE_TEXT)
            continue;

        int n_words = tmp->n_words;
        htmWord *words = tmp->words;
        for (int i = 0; i < n_words; i++) {
            if (words[i].type != OBJ_TEXT)
                continue;
            if (htm_select.intersect(words[i].area)) {
                tw.add(words[i].area);
                didone = true;
            }
        }
    }
    if (didone) {
        *x = tw.left();
        *y = tw.top();
        *w = tw.width;
        *h = tw.height;
    }
}


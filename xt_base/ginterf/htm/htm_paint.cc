
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
 * $Id: htm_paint.cc,v 1.17 2014/02/15 23:14:17 stevew Exp $
 *-----------------------------------------------------------------------*/

#include "htm_widget.h"
#include "htm_font.h"
#include "htm_format.h"
#include "htm_table.h"
#include "htm_image.h"
#include "htm_callback.h"
#include <stdio.h>  // debugging only


// Private Variable Declarations
#define DRAW_TOP        (1<<1)
#define DRAW_BOTTOM     (1<<2)
#define DRAW_LEFT       (1<<3)
#define DRAW_RIGHT      (1<<4)
#define DRAW_BOX        (DRAW_TOP|DRAW_BOTTOM|DRAW_LEFT|DRAW_RIGHT)
#define DRAW_NONE       ~(DRAW_BOX)

#define DBG_RECT(area) htm_tk->tk_draw_rectangle(false, viewportX(area.x), \
    viewportY(area.y), area.width, area.height)

namespace {
    // Animation timeout handler.
    //
    int
    TimerCB(void *data)
    {
        htmImage *image = (htmImage*)data;
        image->manager->im_html->animTimerHandler(image);
        return (false);
    }
}


// Animation timeout handler.
//
void
htmWidget::animTimerHandler(htmImage *image)
{
    if (htm_freeze_animations)
        return;

    image->options |= IMG_FRAMEREFRESH;
    drawImage(image->owner, 0, true);
}


namespace {
    // Find the correct background mode in a table cell.
    //
    void
    table_bg(htmTableCell *cell, bool *do_bg, bool *do_image, int *org_x,
        int *org_y)
    {
        htmTableRow *row = cell->tc_parent;
        htmTable *table = row->tr_parent;

        *do_bg = false;
        *do_image = false;
        if (cell->tc_properties->tp_flags & TP_IMAGE_GIVEN) {
            *org_x = cell->tc_owner->area.x;
            *org_y = cell->tc_owner->area.y;
            *do_image = true;
        }
        else if (row->tr_properties->tp_flags & TP_IMAGE_GIVEN) {
            *org_x = row->tr_owner->area.x;
            *org_y = row->tr_owner->area.y;
            *do_image = true;
        }
        else if (table->t_properties->tp_flags & TP_IMAGE_GIVEN) {
            *org_x = table->t_owner->area.x;
            *org_y = table->t_owner->area.y;
            *do_image = true;
        }
        if (cell->tc_properties->tp_flags & TP_BG_GIVEN)
            *do_bg = true;
        else if (row->tr_properties->tp_flags & TP_BG_GIVEN)
            *do_bg = true;
        else if (table->t_properties->tp_flags & TP_BG_GIVEN)
            *do_bg = true;
    }
}


// Paint the given listed objects and refresh the display.
//
void
htmWidget::paint(htmObjectTable *start, htmObjectTable *end)
{
    htm_last_paint.clear();
    for (htmObjectTable *temp = start; temp && temp != end;
            temp = temp->next) {
        switch (temp->object_type) {
        case OBJ_TEXT:
        case OBJ_PRE_TEXT:

            // First check if this is an image.  drawImage will render
            // an image as an anchor if required.

            if (temp->text_data & TEXT_IMAGE)
                drawImage(temp, 0, false, &htm_last_paint);
            else {
                if (temp->text_data & TEXT_FORM)
                    break;
                else {
                    if (temp->text_data & TEXT_ANCHOR)
                        temp = drawAnchor(temp, end, &htm_last_paint);
                    else
                        drawText(temp, &htm_last_paint);
                }
            }
            break;
        case OBJ_BULLET:
            drawBullet(temp, &htm_last_paint);
            break;
        case OBJ_HRULE:
            drawRule(temp, &htm_last_paint);
            break;
        case OBJ_IMG:
            warning("paint", "Refresh: Invalid image object.");
            break;
        case OBJ_TABLE:
        case OBJ_TABLE_FRAME:
            {
                htmObjectTable *tend = drawTable(temp, end, &htm_last_paint);
//                DBG_RECT(temp->area);
                temp = tend;
            }
            break;
        case OBJ_APPLET:
        case OBJ_BLOCK:
        case OBJ_NONE:
            break;
        default:
            warning("paint", "Unknown object type!");
        }
    }
    if (htm_last_paint.width && htm_last_paint.height)
        htm_tk->tk_refresh_area(
            viewportX(htm_last_paint.left()),
            viewportY(htm_last_paint.top()),
            htm_last_paint.width, htm_last_paint.height);
}


// Paint the background if the word is in the selection area.
//
void
htmWidget::setup_selection(htmObjectTable *data, htmWord *word, htmWord *nxt)
{
    if (htm_select.intersect(word->area)) {
        int xv = viewportX(word->area.x);
        int yv = viewportY(word->area.y);
        int spa = 0;
        if (nxt && word->line == nxt->line &&
                nxt->area.left() < htm_select.right())
            spa = 4;
        htm_tk->tk_set_foreground(htm_cm.cm_select_bg);
        htm_tk->tk_draw_rectangle(true, xv, yv, word->area.width + spa,
            word->area.height);
        htm_tk->tk_set_foreground(data->fg);
    }
}


// Restart all animations.  Called by SetValues when the value of the
// freezeAnimations resource switches from true to false.
//
void
htmWidget::restartAnimations()
{
    for (htmImage *tmp = htm_im.im_images; tmp; tmp = tmp->next) {
        if (tmp->IsAnim()) {
            tmp->options |= IMG_FRAMEREFRESH;
            drawImage(tmp->owner, 0, false, 0);
        }
    }
}


// Image refresher.  This is a funny routine:  it does plain images as
// well as animations.  Animations with a loop count of zero will loop
// forever.  Other animations will loop their counts and when that has
// been reached they are depromoted to regular images.  The only way
// to restore animations with a loop count to animations again is to
// reload them (htmImageUpdate).
//
// The y_offset is used when updating images progressively.
//
void
htmWidget::drawImage(htmObjectTable *data, int y_offset, bool from_timerCB,
    htmRect *rect)
{
    htmImage *image = data->words->image;
    if (!image)
        return;

    // check the owner box size, reset if necessary (sanity check)
    if (image->owner) {
        if ((int)image->owner->area.height < image->height)
            image->owner->area.height = image->height;
        if ((int)image->owner->area.width < image->width)
            image->owner->area.width = image->width;
    }

    // compute correct image offsets
    htmRect rect_im(data->words->area.left() + image->hspace,
        data->words->area.top(), image->width, image->height);

    // Animation frames should be repainted if the animation is
    // somewhere in the visible area.

    // Only do this when we are repainting this image as a result of
    // an exposure.

    if (!from_timerCB) {
        // anchors are always repainted if they are visible
        // fix 03/25/97-01, kdh
        if (data->text_data & TEXT_ANCHOR)
            drawImageAnchor(data);
    }

    // If this is an animation, paint next frame or restore current
    // state when we are scrolling this animation on and off screen.

    if (image->IsAnim()) {
        if (rect)
            rect->add(rect_im);
        drawFrame(image, data, rect_im.left(), rect_im.top());
    }
    else if (image->pixmap != 0) {

        rect_im.y += y_offset;
        if (htm_paint.no_intersect(rect_im))
            return;
        if (rect)
            rect->add(rect_im);

        int xv = viewportX(rect_im.left());
        int yv = viewportY(rect_im.top());

        // put in clipmask
        if (image->clip) {
            htm_tk->tk_set_clip_mask(image->pixmap, image->clip);
            htm_tk->tk_set_clip_origin(xv, yv);
        }

        // copy expose region to screen

        int iy = 0;
        int ih = image->height;
        if (rect_im.top() < htm_paint.top()) {
            iy = htm_paint.top() - rect_im.top();
            ih -= iy;
        }
        if (rect_im.bottom() > htm_paint.bottom())
            ih -= rect_im.bottom() - htm_paint.bottom();
        yv += iy;

        int ix = 0;
        int iw = image->width;
        if (rect_im.left() < htm_paint.left()) {
            ix = htm_paint.left() - rect_im.left();
            iw -= ix;
        }
        if (rect_im.right() > htm_paint.right())
            iw -= rect_im.right() - htm_paint.right();
        xv += ix;

        htm_tk->tk_draw_pixmap(xv, yv, image->pixmap, ix,
            iy + y_offset, iw, ih);
    }
    htm_tk->tk_set_clip_origin(0, 0);
    htm_tk->tk_set_clip_mask(0, 0);

    // Paint the alt text if images are disabled or when this image is
    // delayed.

    if ((!htm_images_enabled ||
            (image->html_image && image->html_image->Delayed())) &&
            !(data->text_data & TEXT_ANCHOR)) {

        htm_tk->tk_set_font(data->words->font);
        htm_tk->tk_set_foreground(htm_cm.cm_body_fg);

        // put text inside bounding rectangle (left-justified, centered
        // vertically)
        int xv = viewportX(rect_im.left());
        int yv = viewportY(rect_im.top()) + image->height/2 -
            data->words->font->height/2;
        htm_tk->tk_draw_text(xv, yv + data->words->font->ascent,
            data->words->word, data->words->len);
    }

    // check if we have to draw the imagemap bounding boxes
    if (image->map_type == MAP_CLIENT && htm_imagemap_draw)
        htm_im.drawImagemapSelection(image);
    else if ((image->border || (image->options & IMG_ISINTERNAL)) &&
            !(data->text_data & TEXT_ANCHOR)) {
        drawShadows(data->area.x, data->area.y,
            data->area.width, data->area.height, false);
    }
}


void
htmWidget::drawImageAnchor(htmObjectTable *data)
{
    // clip
    if (htm_paint.no_intersect(data->area))
        return;

    int width  = data->words->area.width;
    int height = data->words->area.height;
    int xv = viewportX(data->words->area.left());
    int yv = viewportY(data->words->area.top());

    if (data->words->image->border) {
        // add border offsets as well
        xv -= data->words->image->border;
        yv -= data->words->image->border;

        if (data->words->line_data & ALT_STYLE) {
            bool pressed;
            if (htm_highlight_on_enter) {
                if (data->anchor_state == ANCHOR_INSELECT) {
                    // paint button highlighting
                    if (htm_im.im_body_image == 0 &&
                            data->words->image->clip != 0) {
                        htm_tk->tk_set_foreground(htm_cm.cm_highlight);
                        htm_tk->tk_draw_rectangle(true, xv, yv, width, height);
                    }
                    // and draw as unselected
                    pressed = true;
                }
                else if (data->anchor_state == ANCHOR_SELECTED) {
                    // paint button highlighting
                    if (htm_im.im_body_image == 0 &&
                            data->words->image->clip != 0) {
                        htm_tk->tk_set_foreground(htm_cm.cm_highlight);
                        htm_tk->tk_draw_rectangle(true, xv, yv, width, height);
                    }

                    // and draw as selected
                    pressed = true;
                }
                else {
                    // button is unselected
                    // restore correct background
                    if (htm_im.im_body_image == 0 &&
                            data->words->image->clip != 0) {
                        htm_tk->tk_set_foreground(data->bg);
                        htm_tk->tk_draw_rectangle(true, xv, yv, width, height);
                    }
                    // and draw as unselected
                    pressed = false;
                }
            }
            else {
                // either selected or unselected
                if (data->anchor_state == ANCHOR_SELECTED)
                    pressed = true;
                else
                    pressed = false;
            }
            drawShadows(data->words->area.left(),
                data->words->area.top(), width, height, pressed);
        }
        else {
            int b = data->words->image->border;

            // set appropriate foreground color
            if (data->anchor_state == ANCHOR_INSELECT &&
                    htm_highlight_on_enter)
                htm_tk->tk_set_foreground(htm_cm.cm_anchor_target_fg);
            else if (data->anchor_state == ANCHOR_SELECTED)
                htm_tk->tk_set_foreground(htm_cm.cm_anchor_activated_fg);
            else
                htm_tk->tk_set_foreground(data->fg);

            htm_tk->tk_draw_rectangle(true, xv, yv, width, b);
            htm_tk->tk_draw_rectangle(true, xv, yv, b, height);
            htm_tk->tk_draw_rectangle(true, xv, yv+height-b, width, b);
            htm_tk->tk_draw_rectangle(true, xv+width-b, yv, b, height);
        }
    }
    // paint the alt text if images are disabled
    if (!htm_images_enabled) {
        htm_tk->tk_set_font(data->words->font);
        htm_tk->tk_set_foreground(data->anchor_state == ANCHOR_SELECTED ?
            htm_cm.cm_anchor_activated_fg : htm_cm.cm_anchor_fg);

        // put text inside bounding rectangle
        yv += data->words->image->height/2 - data->words->font->height/2;
        htm_tk->tk_draw_text(xv, yv + data->words->font->ascent,
            data->words->word, data->words->len);
    }
}


// Animation driver, does frame disposal and renders a new frame.
//
// Complex animations
// ------------------
// Instead of drawing into the window directly, we draw into an
// internal pixmap and blit this pixmap to the screen when all
// required processing has been done, which is a lot faster than
// drawing on the screen directly.  Another advantage of this approach
// is that we always have a current state available which can be used
// when an animation is scrolled on and off screen (frame dimensions
// differ from logical screen dimensions or a disposal method other
// than IMAGE_DISPOSE_NONE is to be used).
//
// Easy animations
// ---------------
// Each frame is blitted to screen directly, only processing done is
// using a possible clipmask (frame dimensions equal to logical screen
// dimensions and a disposal method of IMAGE_DISPOSE_NONE).
//
void
htmWidget::drawFrame(htmImage *image, htmObjectTable *data, int x, int y)
{
    htm_tk->tk_set_clip_origin(0, 0);
    htm_tk->tk_set_clip_mask(0, 0);

    int idx = image->current_frame ?
        image->current_frame - 1 : image->nframes - 1;

    // First check if we are running this animation internally.  If we
    // aren't we have a simple animation of which each frame has the
    // same size and no disposal method has been specified.  This type
    // of animations are blit to screen directly.

    if (!image->HasState()) {
        // index of current frame
        if (image->FrameRefresh())
            idx = image->current_frame;
        unsigned int width  = image->frames[idx].area.width;
        unsigned int height = image->frames[idx].area.height;
        int xv = viewportX(x);
        int yv = viewportY(y);

        // can happen when a frame falls outside the logical screen area
        if (image->frames[idx].pixmap != 0) {
            // plug in the clipmask
            if (image->frames[idx].clip) {
                htm_tk->tk_set_clip_mask(image->frames[idx].pixmap,
                    image->frames[idx].clip);
                htm_tk->tk_set_clip_origin(xv, yv);
            }
            // blit frame to screen
            htm_tk->tk_draw_pixmap(xv, yv, image->frames[idx].pixmap,
                0, 0, width, height);
            htm_tk->tk_refresh_area(xv, yv, image->width,
                image->height);
        }

        // Jump to frame updating when we are not triggered by an
        // exposure, otherwise just return.

        if (image->FrameRefresh())
            goto nextframe;
        return;
    }

    // If we get here we are running the animation internally.  First
    // check the disposal method and update the current state
    // accordingly before putting the next frame on the display.
    // Pixmap can be 0 if a frame falls outside the logical screen
    // area.  idx is the index of the previous frame (the frame that
    // is currently being displayed).

    if (image->frames[idx].pixmap != 0) {
        int fx = viewportX(x + image->frames[idx].area.x);
        int fy = viewportY(y + image->frames[idx].area.y);
        unsigned int width  = image->frames[idx].area.width;
        unsigned int height = image->frames[idx].area.height;

        if (image->frames[idx].dispose == IMAGE_DISPOSE_BY_BACKGROUND) {

            bool did_bg = false;
            if (data->table_cell) {
                int org_x, org_y;
                bool do_image, do_bg;
                table_bg(data->table_cell, &do_bg, &do_image, &org_x, &org_y);
                if (do_image || do_bg) {
                    htmTableProperties *props =
                        data->table_cell->tc_properties;
                    org_x -= data->area.x;
                    org_y -= data->area.y;
                    if (do_bg) {
                        htm_tk->tk_set_foreground(data->bg);
                        htm_tk->tk_draw_rectangle(true, fx, fy, width, height);
                        did_bg = true;
                    }
                    if (do_image && props->tp_bg_image != 0 &&
                            props->tp_bg_image->pixmap) {

                        htmImage *bg_image = props->tp_bg_image;
                        htm_tk->tk_set_clip_mask(bg_image->pixmap,
                            bg_image->clip);
                        htm_tk->tk_set_clip_origin(org_x, org_y);
                        htm_tk->tk_tile_draw_pixmap(org_x, org_y,
                            bg_image->pixmap, fx, fy, width, height);
                        htm_tk->tk_set_clip_mask(0, 0);
                        did_bg = true;
                    }
                }
            }
            if (!did_bg) {
                if (htm_im.im_body_image) {
                    // we have a body image, compute correct tile offsets

                    int tile_width  = htm_im.im_body_image->width;
                    int tile_height = htm_im.im_body_image->height;
                    int tsy = htm_viewarea.top() % tile_height;
                    int tsx = htm_viewarea.left() % tile_width;

                    htm_tk->tk_tile_draw_pixmap(tsx, tsy,
                        htm_im.im_body_image->pixmap, fx, fy, width, height);
                }
                else {
                    htm_tk->tk_set_foreground(data->bg);
                    htm_tk->tk_draw_rectangle(true, fx, fy, width, height);
                }
            }
        }

        // No disposal method but we have a clipmask.  Need to plug in
        // the background image or color.  As we are completely
        // overlaying the current image with the new image, we can
        // safely erase the entire contents of the current state with
        // the wanted background, after which we use the clipmask to
        // copy the requested parts of the image on screen.
        //
        // Please note that this is *only* done for the very first
        // frame of such an animation.  All other animations, whether
        // they have a clipmask or not, are put on top of this frame.
        // Doing it for other frames as well would lead to unwanted
        // results as the underlying portions of the animation would
        // be replaced with the current background, and thereby
        // violating the none disposal method logic.

        else if (image->frames[idx].dispose == IMAGE_DISPOSE_NONE &&
                idx == 0 && image->frames[idx].clip != 0) {

            bool did_bg = false;
            if (data->table_cell) {
                int org_x, org_y;
                bool do_image, do_bg;
                table_bg(data->table_cell, &do_bg, &do_image, &org_x, &org_y);
                if (do_image || do_bg) {
                    htmTableProperties *props =
                        data->table_cell->tc_properties;
                    org_x -= data->area.x;
                    org_y -= data->area.y;
                    if (do_bg) {
                        htm_tk->tk_set_foreground(data->bg);
                        htm_tk->tk_draw_rectangle(true, fx, fy, width, height);
                        did_bg = true;
                    }
                    if (do_image && props->tp_bg_image != 0 &&
                            props->tp_bg_image->pixmap) {

                        htmImage *bg_image = props->tp_bg_image;
                        htm_tk->tk_set_clip_mask(bg_image->pixmap,
                            bg_image->clip);
                        htm_tk->tk_set_clip_origin(org_x, org_y);
                        htm_tk->tk_tile_draw_pixmap(org_x, org_y,
                            bg_image->pixmap, fx, fy, width, height);
                        htm_tk->tk_set_clip_mask(0, 0);
                        did_bg = true;
                    }
                }
            }
            if (!did_bg) {
                if (htm_im.im_body_image) {

                    int tile_width  = htm_im.im_body_image->width;
                    int tile_height = htm_im.im_body_image->height;
                    int tsy = htm_viewarea.top() % tile_height;
                    int tsx = htm_viewarea.left() % tile_width;

                    htm_tk->tk_tile_draw_pixmap(tsx, tsy,
                        htm_im.im_body_image->pixmap, fx, fy, width, height);
                }
                else {
                    // do a plain fillrect in current background color
                    htm_tk->tk_set_foreground(data->bg);
                    htm_tk->tk_draw_rectangle(true, fx, fy, width, height);
                }
            }
            htm_tk->tk_set_clip_mask(image->frames[idx].pixmap,
                image->frames[idx].clip);
            htm_tk->tk_set_clip_origin(fx, fy);

            // paint it, use full image dimensions
            htm_tk->tk_draw_pixmap(fx, fy,
                image->frames[idx].pixmap, 0, 0, width, height);
        }
        // dispose by previous (the only one to have a prev_state)
        else if (image->frames[idx].prev_state != 0) {
            // plug in the clipmask
            if (image->frames[idx].clip) {
                htm_tk->tk_set_clip_mask(image->frames[idx].pixmap,
                    image->frames[idx].clip);
                htm_tk->tk_set_clip_origin(fx, fy);
            }
            // put previous screen state on current state
            htm_tk->tk_draw_pixmap(fx, fy,
                image->frames[idx].prev_state, 0, 0, width, height);
        }
    }
    htm_tk->tk_set_clip_origin(0, 0);
    htm_tk->tk_set_clip_mask(0, 0);

    // index of current frame
    idx = image->current_frame;

    // can happen when a frame falls outside the logical screen area
    if (image->frames[idx].pixmap != 0) {
        int fx = viewportX(x + image->frames[idx].area.x);
        int fy = viewportY(y + image->frames[idx].area.y);
        unsigned int width  = image->frames[idx].area.width;
        unsigned int height = image->frames[idx].area.height;

        // Get current screen state if we are to dispose of this frame
        // by the previous state.  The previous state is given by the
        // current pixmap, so we just create a new pixmap and copy the
        // current one into it.  This is about the fastest method I
        // can think of.

        if (image->frames[idx].dispose == IMAGE_DISPOSE_BY_PREVIOUS &&
                image->frames[idx].prev_state == 0) {

            // create pixmap that is to receive the image
            htmPixmap *prev_state = htm_tk->tk_new_pixmap(width, height);

            // set target to prev_state
            htm_tk->tk_set_draw_to_pixmap(prev_state);
            htm_tk->tk_draw_pixmap(0, 0, image->pixmap, 0, 0, width, height);
            htm_tk->tk_set_draw_to_pixmap(0);

            // and save it
            image->frames[idx].prev_state = prev_state;
        }
        if (image->frames[idx].clip) {
            htm_tk->tk_set_clip_mask(image->frames[idx].pixmap,
                image->frames[idx].clip);
            htm_tk->tk_set_clip_origin(fx, fy);
        }
        htm_tk->tk_draw_pixmap(fx, fy,
            image->frames[idx].pixmap, 0, 0, width, height);

        htm_tk->tk_set_clip_origin(0, 0);
        htm_tk->tk_set_clip_mask(0, 0);

        htm_tk->tk_refresh_area(viewportX(x), viewportY(y), image->width,
            image->height);
    }

    // end expose handling
    if (!image->FrameRefresh())
        return;

nextframe:
    image->current_frame++;

    // will get set again by TimerCB
    image->options &= ~(IMG_FRAMEREFRESH);

    if (image->current_frame == image->nframes) {
        image->current_frame = 0;

        // Sigh, when an animation is running forever (loop_count ==
        // 0) and some sucker is keeping gtkhtm up forever, chances
        // are that we can exceed INT_MAX.  Since some systems don't
        // wrap their integers properly when their value exceeds
        // INT_MAX, we can't keep increasing the current loop count
        // forever since this can lead to a crash (which is a
        // potential security hole).  To prevent this from happening,
        // we only increase current_loop when run this animation a
        // limited number of times.

        if (image->loop_count) {
            image->current_loop++;

            // If the current loop count matches the total loop count,
            // depromote the animation to a regular image so the next
            // time the timer callback is activated we will enter
            // normal image processing.

            if (image->current_loop == image->loop_count)
                image->options &= ~(IMG_ISANIM);
        }
    }

    unsigned int timeout = image->frames[idx].timeout;
    if (image->proc_id == 0)
        image->proc_id = htm_tk->tk_add_timer(TimerCB, image);
    htm_tk->tk_start_timer(image->proc_id, timeout);
}


// Main text rendering engine.
//
void
htmWidget::drawText(htmObjectTable *data, htmRect *rect)
{
    int nwords = data->n_words;
    htmWord *words = data->words;

    if (!nwords)
        return;

    // only need to set this once
    htm_tk->tk_set_font(words[0].font);
    htm_tk->tk_set_foreground(data->fg);

// DBG_RECT(data->area);

    for (int i = 0 ; i < nwords; i++) {
        htmWord *tmp = &words[i];

        // When any of the two cases below is true, the text at the
        // current position is outside the exposed area.  Not doing
        // this check would cause a visible flicker of the screen when
        // scrolling:  the entire line would be repainted, even the
        // invisible text.  And we sure don't want to render any
        // linebreaks, looks pretty ugly.  (this test is a lot cheaper
        // than doing ``invisible'' rendering)

        if (tmp->type == OBJ_BLOCK)
            continue;
        if (htm_paint.no_intersect(tmp->area))
            continue;

        int xv = viewportX(tmp->area.x);
        int yv = viewportY(tmp->ybaseline);
        if (rect)
            rect->add(tmp->area);

// DBG_RECT(tmp->area);

        setup_selection(data, tmp, i < nwords-1 ? &words[i+1] : 0);
        htm_tk->tk_draw_text(xv, yv, tmp->word, tmp->len);

        if (tmp->line_data & LINE_UNDER) {

            // Vertical position for underline, barely connects with
            // the underside of the ``deepest'' character.

            int dy = yv + tmp->base->font->ul_offset;

            // adjust word width
            int width = words[i].area.width;
            if (i < nwords-1 && words[i].line == words[i+1].line)
                width = words[i+1].area.left() - words[i].area.left();

            if (tmp->line_data & LINE_DASHED)
                htm_tk->tk_set_line_style(htmInterface::TILED);
            else
                htm_tk->tk_set_line_style(htmInterface::SOLID);
            htm_tk->tk_draw_line(xv, dy, xv + width, dy);
            if (tmp->line_data & LINE_DOUBLE)
                htm_tk->tk_draw_line(xv, dy+2, xv + width, dy+2);
        }
        if (tmp->line_data & LINE_STRIKE) {
            // strikeout line is somewhere near the middle of a line
            int dy = yv - tmp->base->font->st_offset;

            // adjust word width
            int width = words[i].area.width;
            if (i < nwords-1 && words[i].line == words[i+1].line)
                width = words[i+1].area.left() - words[i].area.left();

            htm_tk->tk_set_line_style(htmInterface::SOLID);
            htm_tk->tk_draw_line(xv, dy, xv + width, dy);
        }
    }
}


// Main text anchor renderer.  This function handles all textual
// anchor stuff.  It paints anchors according to the selected anchor
// style and (optionally) performs anchor highlighting.  Image anchors
// are rendered by drawImageAnchor.
//
htmObjectTable *
htmWidget::drawAnchor(htmObjectTable *data, htmObjectTable *end, htmRect *rect)
{
    // pick up the real start of this anchor
    htmObjectTable *a_start;
    for (a_start = data; a_start && a_start->anchor == data->anchor;
        a_start = a_start->prev) ;

    // sanity, should never happen
    if (a_start == 0) {
        warning("drawAnchor", "Internal Error: "
            "could not locate anchor starting point!");
        return (data);
    }

    // previous loop always walks back one too many
    a_start = a_start->next;

    // pick up the real end of this anchor and count the words
    htmObjectTable *a_end;
    int nwords = 0;
    for (a_end = a_start; a_end && a_end->anchor == a_start->anchor;
            a_end = a_end->next) {
        if (a_end->text_data & TEXT_IMAGE)
            drawImage(a_end, 0, false, &htm_last_paint);
        else if (!(a_end->text_data & TEXT_BREAK))
            nwords += a_end->n_words;
    }

    // sanity check
    if (!nwords)
        return (data);     // fix 01/30/97-01, kdh

    // Put all anchor words into an array if this anchor spans
    // multiple objects (as can be the case with font changes within
    // an anchor) If this isn't the case, just use the words of the
    // current data object.

    htmWord **all_words = 0;
    htmWord *words = 0;
    if (a_start->next != a_end) {
        all_words = new htmWord*[nwords];

        int i = 0;
        htmObjectTable *temp;
        for (temp = a_start; temp != a_end; temp = temp->next) {
            // ignore image words, they get handled by drawImageAnchor
            if (!(temp->text_data & TEXT_IMAGE) &&
                    !(temp->text_data & TEXT_BREAK)) {
                for (int j = 0 ; j < temp->n_words; j++)
                    all_words[i++] = temp->words + j;
            }
        }
    }
    else
        words = data->words;

    // This is used for drawing the bounding rectangle of an anchor.
    // When an anchor is encountered, width is used to compute the
    // total width of a rectangle surrounding all anchor words on the
    // same line.  The bounding rectangle drawn extends a little bit
    // to the left and right of the anchor.

    htmWord *tmp = (words ? &words[0] : all_words[0]);
    int xv0 = viewportX(tmp->area.x) - 2;  // extend to the left
    int yv0 = viewportY(tmp->ybaseline);
    int width = tmp->area.width;

    int i = 0;
    int start = 0;

//DBG_RECT(tmp->owner->area);

    do {
        tmp = (words ? &words[i] : all_words[i]);

// DBG_RECT(tmp->area);

        // anchors are always painted
        int xv = viewportX(tmp->area.x);
        int yv = viewportY(tmp->ybaseline);

        // baseline font
        htmFont *font = tmp->base->font;

        // compute total width of all words on this line
        if (yv == yv0)
            width = xv + tmp->area.width - xv0;
        if (tmp->line_data & ALT_STYLE)
            // extend to the right if this word has a trailing space
            width += (tmp->spacing & TEXT_SPACE_TRAIL ? 1 : 0);

        if (yv != yv0 || i == nwords-1) {

            if (tmp->line_data & ALT_STYLE) {
                if (data->anchor_state == ANCHOR_SELECTED)
                    // draw as selected
                    drawShadows(windowX(xv0), windowY(yv0 - font->ascent),
                        width, font->lineheight, true);
                else
                    // Button is unselected or being selected. In both
                    // cases draw it as unselected.
                    drawShadows(windowX(xv0), windowY(yv0 - font->ascent),
                        width, font->lineheight, false);
            }

            // set appropriate foreground color
            if (data->anchor_state == ANCHOR_INSELECT &&
                    htm_highlight_on_enter)
                htm_tk->tk_set_foreground(htm_cm.cm_anchor_target_fg);
            else if (data->anchor_state == ANCHOR_SELECTED)
                htm_tk->tk_set_foreground(htm_cm.cm_anchor_activated_fg);
            else
                htm_tk->tk_set_foreground(data->fg);

            if (words) {
                for (int j = start; j < i+1; j++) {
                    htm_tk->tk_set_font(words[j].font);
                    setup_selection(data, &words[j],
                        j < nwords-1 ? &words[j+1] : 0);
                    htm_tk->tk_draw_text(
                        viewportX(words[j].area.x),
                        viewportY(words[j].ybaseline),
                        words[j].word, words[j].len);
                    if (rect)
                        rect->add(words[j].owner->area);

// DBG_RECT(words[j].owner->area);

                }
            }
            else {
                for (int j = start; j < i+1; j++) {
                    htm_tk->tk_set_font(all_words[j]->font);
                    setup_selection(data, all_words[j],
                        j < nwords-1 ? all_words[j+1] : 0);
                    htm_tk->tk_draw_text(
                        viewportX(all_words[j]->area.x),
                        viewportY(all_words[j]->ybaseline),
                        all_words[j]->word, all_words[j]->len);
                    if (rect)
                        rect->add(all_words[j]->owner->area);

// DBG_RECT(all_words[j]->owner->area);

                }
            }

            // Anchor buttons are never underlined, it looks ugly
            if (tmp->line_data & LINE_UNDER) {
                int dy = yv0 + font->ul_offset;
                if (tmp->line_data & LINE_DASHED)
                    htm_tk->tk_set_line_style(htmInterface::TILED);
                else
                    htm_tk->tk_set_line_style(htmInterface::SOLID);
                htm_tk->tk_draw_line(xv0 + 2, dy, xv0 + width, dy);
                // draw another line if requested
                if (tmp->line_data & LINE_DOUBLE)
                    htm_tk->tk_draw_line(xv0 + 2, dy+2, xv0 + width, dy+2);
            }
            if (tmp->line_data & LINE_STRIKE) {
                int dy = yv0 - font->st_offset;
                htm_tk->tk_set_line_style(htmInterface::SOLID);
                htm_tk->tk_draw_line(xv0+2, dy, xv0 + width - 2, dy);
            }

            // stupid hack to get the last word of a broken anchor right
            if (yv != yv0 && i == nwords-1)
                i--;
            // next word starts on another line
            width = tmp->area.width;
            start = i;
            xv0 = xv - 2;
            yv0 = yv;
        }
        i++;
    }
    while (i != nwords);

    if (words == 0) {
        delete [] all_words;    // fix 05/26/97-01, kdh

        // Adjust current object data as we have now updated a number of
        // objects in one go. We must use prev as htmPaint will advance
        // to a_end itself.

        for (a_end = data; a_end && a_end != end &&
                a_end->anchor == a_start->anchor; a_end = a_end->next)
            ;
            return (a_end->prev);
    }
    return (data);
}


// Paint a horizontal rule.  Rules that had their noshade attribute
// set are identified by having a non-zero y_offset field in the data.
//
void
htmWidget::drawRule(htmObjectTable *data, htmRect *rect)
{
    if (htm_paint.no_intersect(data->area))
        return;
    if (rect)
        rect->add(data->area);

    if (data->y_offset) {
        // noshade
        htm_tk->tk_set_foreground(data->fg);
        htm_tk->tk_draw_rectangle(true, viewportX(data->area.left()),
            viewportY(data->area.top()), data->area.width, data->area.height);
    }
    else {
        if (data->fg != htm_cm.cm_body_fg)
            htm_cm.recomputeColors(data->fg);
        drawShadows(data->area.left(), data->area.top(),
            data->area.width, data->area.height, false);
        if (data->fg != htm_cm.cm_body_fg)
            htm_cm.recomputeColors(htm_cm.cm_body_bg);
    }
}


void
htmWidget::drawBullet(htmObjectTable *data, htmRect *rect)
{
    if (htm_paint.no_intersect(data->area))
        return;
    if (rect)
        rect->add(data->area);

    // reset colors, an anchor might have been drawn before
    htm_tk->tk_set_foreground(data->fg);
    htm_tk->tk_set_line_style(htmInterface::SOLID);

    unsigned int w = data->area.width;
    int xv = viewportX(data->area.x);
    int yv = viewportY(data->area.y);

    int w2 = 2*w;
    switch (data->marker) {
    case MARKER_DISC:
        htm_tk->tk_draw_arc(true, xv - w - 2, yv, w2, w2, 0, 23040);
        break;
    case MARKER_SQUARE:
        htm_tk->tk_draw_rectangle(false, xv - w - 2, yv, w2, w2);
        break;
    case MARKER_CIRCLE:
        htm_tk->tk_draw_arc(false, xv - w - 2, yv, w2, w2, 0, 23040);
        break;
    default:
        htm_tk->tk_set_font(htm_default_font);
        htm_tk->tk_draw_text(xv, yv + htm_default_font->ascent, data->text,
            data->len);
        break;
    }
}


void
htmWidget::drawShadows(int x, int y, unsigned int width, unsigned int height,
    bool shadow_in)
{
    x = viewportX(x);
    y = viewportY(y);
    if (shadow_in) {
        htm_tk->tk_set_foreground(htm_cm.cm_shadow_bottom);
        // top & left border
        htm_tk->tk_draw_line(x, y, x + width - 1, y);
        htm_tk->tk_draw_line(x, y + 1, x, y + height);

        htm_tk->tk_set_foreground(htm_cm.cm_shadow_top);
        // bottom & right border
        htm_tk->tk_draw_line(x, y + height, x + width, y + height);
        htm_tk->tk_draw_line(x + width,  y, x + width, y + height + 1);
    }
    else {
        htm_tk->tk_set_foreground(htm_cm.cm_shadow_top);
        // top & left border
        htm_tk->tk_draw_line(x, y, x + width - 1, y);
        htm_tk->tk_draw_line(x, y + 1, x, y + height);

        htm_tk->tk_set_foreground(htm_cm.cm_shadow_bottom);
        // bottom & right border
        htm_tk->tk_draw_line(x, y + height, x + width, y + height);
        htm_tk->tk_draw_line(x + width,  y, x + width, y + height + 1);
    }
}


htmObjectTable *
htmWidget::drawTable(htmObjectTable *start, htmObjectTable *data_end,
    htmRect *rect)
{
    // pick up table data
    htmTable *table = start->table;

    if (table == 0)
        return (start);

    if (rect)
        rect->add(start->area);

    // The first table in a stack of tables contains all data for all
    // table childs it contains.  The first table child is the master
    // table itself.  So when a table doesn't have a child table it
    // is a child table itself and thus we should add the left
    // offset to the initial horizontal position.

    if (table->t_children)
        table = table->t_children;

    int nrows = table->t_nrows;

    // first draw the contents
    for (int i = 0; i < nrows; i++) {
        htmTableRow *row = table->t_rows + i;

        for (int j = 0; j < row->tr_ncells; j++) {
            htmTableCell *cell = row->tr_cells + j;

            // only draw something if it falls in the exposure area
            if (htm_paint.no_intersect(cell->tc_owner->area))
                continue;

            drawCellContent(cell, j, i);
        }
    }

    // next draw the table and cell borders
    if (table->t_properties->tp_framing != TFRAME_VOID)
        drawTableBorder(table);

    for (int i = 0; i < nrows; i++) {
        htmTableRow *row = table->t_rows + i;

        for (int j = 0; j < row->tr_ncells; j++) {
            htmTableCell *cell = row->tr_cells + j;

            // only draw something if it falls in the exposure area
            if (htm_paint.no_intersect(cell->tc_owner->area))
                continue;

            if (!table->t_children ||
                    cell->tc_properties->tp_ruling != TRULE_NONE)
                drawCellFrame(cell, j, i);
        }
    }

    if (data_end) {
        htmObjectTable *temp;
        for (temp = start; temp && temp != data_end && temp != table->t_end;
                temp = temp->next)
            ;
            if (temp)
                return (temp->prev);
    }
    else if (table->t_end)
         return (table->t_end->prev);
    return (start);
}


void
htmWidget::drawCellContent(htmTableCell *cell, int xn, int yn)
{
    htmObjectTable *start = cell->tc_start;
    htmObjectTable *end = cell->tc_end;
    htmObjectTable *data = cell->tc_owner;

    htmTable *table = cell->tc_parent->tr_parent;

    unsigned char rule = DRAW_BOX;
    switch (table->t_properties->tp_ruling) {
    case TRULE_NONE:    // no rules, only bg color/image will be done
        rule = DRAW_NONE;
        break;
    case TRULE_GROUPS:  // only horizontal rules
    case TRULE_ROWS:    // only horizontal rules
        rule = DRAW_TOP|DRAW_BOTTOM;
        break;
    case TRULE_COLS:    // only vertical rules
        rule = DRAW_LEFT|DRAW_RIGHT;
        break;
    case TRULE_ALL:     // all rules
        break;
    }

    // draw the background if necessary

    int xoff   = (rule & (DRAW_LEFT|DRAW_RIGHT)) ? table->t_hmargin : 0;
    if (xn == 0)
        xoff = table->t_hmargin;
    int width  = data->area.width - xoff;
    int yoff   = (rule & (DRAW_TOP|DRAW_BOTTOM)) ? table->t_vmargin : 0;
    if (yn == 0)
        yoff = table->t_vmargin;
    int height = data->area.height - yoff;

    // initial, absolute, positions
    int xs = data->area.x + xoff;
    int ys = data->area.y + yoff;

    bool do_image, do_bg;
    int org_x, org_y;
    table_bg(cell, &do_bg, &do_image, &org_x, &org_y);
    if (do_image || do_bg) {

        // correct absolute positions so only the exposed region is updated
        if (xs < htm_paint.left()) {
            // origin too far left
            width -= (htm_paint.left() - xs);
            xs = htm_paint.left();
        }
        if (xs + width > htm_paint.right()) {
            // width too far right
            width = htm_paint.right() - xs;
        }
        if (ys < htm_paint.top()) {
            // origin too high
            height -= (htm_paint.top() - ys);
            ys = htm_paint.top();
        }
        if (ys + height > htm_paint.bottom()) {
            // height too low
            height = htm_paint.bottom() - ys;
        }

        if (width >= 0 && height >= 0) {
            int xv = viewportX(xs);
            int yv = viewportY(ys);
            int org_xv = viewportX(org_x);
            int org_yv = viewportY(org_y);

            // Do we have a unique background color?
            if (do_bg) {
                htm_tk->tk_set_foreground(data->bg);
                htm_tk->tk_draw_rectangle(true, xv, yv, width, height);
            }
            if (do_image && cell->tc_properties->tp_bg_image != 0 &&
                    cell->tc_properties->tp_bg_image->pixmap) {

                htmImage *bg_image = cell->tc_properties->tp_bg_image;
                htm_tk->tk_set_clip_mask(bg_image->pixmap,
                    bg_image->clip);
                htm_tk->tk_set_clip_origin(org_xv, org_yv);
                htm_tk->tk_tile_draw_pixmap(org_xv, org_yv,
                    bg_image->pixmap, xv, yv, width, height);
                htm_tk->tk_set_clip_mask(0, 0);
                htm_tk->tk_set_clip_origin(0, 0);
            }
        }
    }

    // now loop thru and render the contents
    for (htmObjectTable *temp = start; temp && temp != end;
            temp = temp->next) {
        switch (temp->object_type) {
        case OBJ_TEXT:
        case OBJ_PRE_TEXT:

            // First check if this is an image.  drawImage will render
            // an image as an anchor if required.

            if (temp->text_data & TEXT_IMAGE)
                drawImage(temp, 0, false);
            else {
                if (temp->text_data & TEXT_FORM)
                    break;
                else {
                    if (temp->text_data & TEXT_ANCHOR)
                        temp = drawAnchor(temp, end);
                    else
                        drawText(temp);
                }
            }
            break;
        case OBJ_BULLET:
            drawBullet(temp);
            break;
        case OBJ_HRULE:
            drawRule(temp);
            break;
        case OBJ_TABLE:
            // nested tables
            temp = drawTable(temp, 0);
            break;
        case OBJ_TABLE_FRAME:
        case OBJ_IMG:
        case OBJ_APPLET:
        case OBJ_BLOCK:
        case OBJ_NONE:
            break;
        default:
            warning("drawCellContent", "Unknown object type!");
        }
    }
}


// Render a frame around a table cell.  Optionally fills it with a
// background color or image.
//
void
htmWidget::drawCellFrame(htmTableCell *cell, int xn, int yn)
{
    htmObjectTable *data = cell->tc_owner;
    htmTable *table = cell->tc_parent->tr_parent;

    if (table->t_properties->tp_border == 0)
        return;

    // which sides do we have to render?
    unsigned char rule = DRAW_BOX;
    switch (table->t_properties->tp_ruling) {
    case TRULE_NONE:    // no rules, only bg color/image will be done
        rule = DRAW_NONE;
        break;
    case TRULE_GROUPS:  // only horizontal rules
    case TRULE_ROWS:    // only horizontal rules
        rule = DRAW_TOP|DRAW_BOTTOM;
        break;
    case TRULE_COLS:    // only vertical rules
        rule = DRAW_LEFT|DRAW_RIGHT;
        break;
    case TRULE_ALL:     // all rules
        break;
    }
    if (!xn)
        rule |= DRAW_LEFT;
    if (!yn)
        rule |= DRAW_TOP;
    int xoff   = cell->tc_parent->tr_parent->t_hmargin;
    if (!(rule & DRAW_RIGHT) && xn)
        xoff = 0;
    int width  = data->area.width - xoff;

    int yoff   = cell->tc_parent->tr_parent->t_vmargin;
    if (!(rule & DRAW_BOTTOM) && yn)
        yoff = 0;
    int height = data->area.height - yoff;

    // initial, absolute, positions
    int xs = data->area.x + xoff;
    int ys = data->area.y + yoff;

    // Correct absolute positions so only the exposed region is updated

    if (xs < htm_paint.left()) {
        // origin too far left
        width -= (htm_paint.left() - xs);
        xs = htm_paint.left();
        rule &= ~DRAW_LEFT;
    }
    if (xs + width > htm_paint.right()) {
        // width too far right
        width = htm_paint.right() - xs;
        rule &= ~DRAW_RIGHT;
    }

    if (width < 0)
        return;

    if (ys < htm_paint.top()) {
        // origin too high
        height -= (htm_paint.top() - ys);
        ys = htm_paint.top();
        rule &= ~DRAW_TOP;
    }
    if (ys + height > htm_paint.bottom()) {
        // height too low
        height = htm_paint.bottom() - ys;
        rule &= ~DRAW_BOTTOM;
    }

    if (height < 0)
        return;

    // Note: width/height = 0 is valid and in fact necessary input below.

    // Translate absolute coordinates to relative ones by substracting
    // the region that has been scrolled.

    int xv = viewportX(xs);
    int yv = viewportY(ys);

    // top & left border
    htm_tk->tk_set_foreground(htm_cm.cm_shadow_bottom);
    // top & left border
    if (rule & DRAW_TOP)
        htm_tk->tk_draw_rectangle(true, xv, yv, width, 1);
    if (rule & DRAW_LEFT)
        htm_tk->tk_draw_rectangle(true, xv, yv, 1, height);

    // bottom & right border
    htm_tk->tk_set_foreground(htm_cm.cm_shadow_top);
    if (rule & DRAW_BOTTOM)
        htm_tk->tk_draw_rectangle(true, xv, yv+height, width, 1);
    if (rule & DRAW_RIGHT)
        htm_tk->tk_draw_rectangle(true, xv+width, yv, 1, height);
}


// Render a frame around a table.
//
void
htmWidget::drawTableBorder(htmTable *table)
{
    htmObjectTable *data = table->t_owner;

    int width = data->area.width;
    int height = data->area.height;

    // which sides do we have to render?
    unsigned char rule = DRAW_BOX;
    switch (table->t_properties->tp_framing) {
    case TFRAME_VOID:
        return;
        break;
    case TFRAME_ABOVE:
        rule = DRAW_TOP;
        break;
    case TFRAME_BELOW:
        rule = DRAW_BOTTOM;
        break;
    case TFRAME_LEFT:
        rule = DRAW_LEFT;
        break;
    case TFRAME_RIGHT:
        rule = DRAW_RIGHT;
        break;
    case TFRAME_HSIDES:
        rule = DRAW_LEFT|DRAW_RIGHT;
        break;
    case TFRAME_VSIDES:
        rule = DRAW_TOP|DRAW_BOTTOM;
        break;
    case TFRAME_BOX:
    case TFRAME_BORDER:
        break;
    }

    // initial, absolute, positions
    int xs = data->area.x;
    int ys = data->area.y;

    // correct absolute positions so only the exposed region is updated

    if (xs < htm_paint.left()) {
        // origin too far left
        width -= (htm_paint.left() - xs);
        xs = htm_paint.left();
        rule &= ~DRAW_LEFT;
    }
    if (xs + width > htm_paint.right()) {
        // width too far right
        width = htm_paint.right() - xs;
        rule &= ~DRAW_RIGHT;
    }

    if (width <= 0)
        return;

    if (ys < htm_paint.top()) {
        // origin too high
        height -= (htm_paint.top() - ys);
        ys = htm_paint.top();
        rule &= ~DRAW_TOP;
    }
    if (ys + height > htm_paint.bottom()) {
        // height too low
        height = htm_paint.bottom() - ys;
        rule &= ~DRAW_BOTTOM;
    }

    if (height <= 0)
        return;

    // Translate absolute coordinates to relative ones by substracting
    // the region that has been scrolled.

    int xv = viewportX(xs);
    int yv = viewportY(ys);

    // top & left border
    htm_tk->tk_set_foreground(htm_cm.cm_shadow_top);
    // top & left border
    if (rule & DRAW_TOP)
        htm_tk->tk_draw_rectangle(true, xv, yv, width, 1);
    if (rule & DRAW_LEFT)
        htm_tk->tk_draw_rectangle(true, xv, yv, 1, height);

    // bottom & right border
    htm_tk->tk_set_foreground(htm_cm.cm_shadow_bottom);
    if (rule & DRAW_BOTTOM)
        htm_tk->tk_draw_rectangle(true, xv, yv+height, width, 1);
    if (rule & DRAW_RIGHT)
        htm_tk->tk_draw_rectangle(true, xv+width, yv, 1, height);
}



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
 * $Id: htm_format_cx.cc,v 1.3 2014/02/15 23:14:17 stevew Exp $
 *-----------------------------------------------------------------------*/

#include "htm_widget.h"
#include "htm_format.h"
#include "htm_tag.h"


htmFormatContext::htmFormatContext()
{
    fg_color_stack = &fg_color_base;
    bg_color_stack = &bg_color_base;
    image_stack = &image_base;
    align_stack = &align_base;
    font_stack = &font_base;

    head = new htmObjectTable;
    anchor_head = 0;
    init();
}


htmFormatContext::~htmFormatContext()
{
    htmObjectTable::destroy(head);
    htmAnchor::destroy(anchor_head);
    clear();
}


void
htmFormatContext::init()
{
    ident_data = 0;

    numchars = 0;
    prev_state = LF_NONE;
    have_space = false;

    num_elements = 1;
    num_anchors  = 0;
    htmObjectTable::destroy(head->next);
    head->next = 0;
    current = head;
    htmAnchor::destroy(anchor_head);
    anchor_head = 0;
    anchor_current = 0;
}


void
htmFormatContext::clear()
{
    // clear foreground colorstack
    if (fg_color_stack) {
        while (fg_color_stack->next)
            popFGColor();
    }
    // clear background colorstack
    if (bg_color_stack) {
        while (bg_color_stack->next)
            popBGColor();
    }
    // clear alignment stack
    if (align_stack) {
        while (align_stack->next)
            popAlignment();
    }
    // clear font stack
    if (font_stack) {
        while (font_stack->next) {
            int fontsize;
            popFont(&fontsize);
        }
    }
    // clear image stack
    if (image_stack) {
        while (image_stack->next)
            popBGImage();
    }
    // clear indentation stack
    while (ident_data) {
        identData *id = ident_data;
        ident_data = id->next;
        delete id;
    }
}


// Detach and return the formatted output list.
//
htmObjectTable *
htmFormatContext::getFormatted()
{
    // Since the table head is a dummy element, we return the
    // next one.
    htmObjectTable *tmp = head->next;
    if (tmp)
        tmp->prev = 0;
    head->next = 0;
    current = head;
    num_elements = 0;
    return (tmp);
}


// Detach and return the anchor list.
//
htmAnchor *
htmFormatContext::getAnchors()
{
    htmAnchor *tmp = anchor_head;
    anchor_head = 0;
    anchor_current = 0;
    num_anchors = 0;
    return (tmp);
}


void
htmFormatContext::insertElement(htmObjectTable *element)
{
    if (element) {
        element->prev = current;
        current->next = element;
        current = element;
        num_elements++;
    }
}


unsigned int
htmFormatContext::popFGColor()
{
    unsigned int color;
    if (fg_color_stack->next) {
        colorStack *tmp = fg_color_stack;
        fg_color_stack = fg_color_stack->next;
        color = tmp->color;
        delete tmp;
    }
    else
        color = fg_color_stack->color;
    return (color);
}


unsigned int
htmFormatContext::popBGColor()
{
    unsigned int color;
    if (bg_color_stack->next) {
        colorStack *tmp = bg_color_stack;
        bg_color_stack = bg_color_stack->next;
        color = tmp->color;
        delete tmp;
    }
    else
        color = bg_color_stack->color;
    return (color);
}


htmImage *
htmFormatContext::popBGImage()
{
    htmImage *image;
    if (image_stack->next) {
        imageStack *tmp = image_stack;
        image_stack = image_stack->next;
        image = tmp->image;
        delete tmp;
    }
    else
        image = image_stack->image;
    return (image);
}


Alignment
htmFormatContext::popAlignment()
{
    Alignment align;
    if (align_stack->next) {
        alignStack *tmp = align_stack;
        align_stack = align_stack->next;
        align = tmp->align;
        delete tmp;
    }
    else
        align = align_stack->align;
    return (align);
}


htmFont *
htmFormatContext::popFont(int *size)
{
    htmFont *font;
    if (font_stack->next) {
        fontStack *tmp = font_stack;
        font_stack = font_stack->next;
        font = tmp->font;
        *size = tmp->size;
        delete tmp;
    }
    else {
        font = font_stack->font;
        *size = font_stack->size;
    }
    return (font);
}


int
htmFormatContext::popIdent()
{
    int ident = 0;
    identData *id = ident_data;
    if (id) {
        ident_data = id->next;
        ident = id->level;
        delete id;
    }
    return (ident);
}


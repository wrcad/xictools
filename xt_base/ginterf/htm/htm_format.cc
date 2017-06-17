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
 * $Id: htm_format.cc,v 1.21 2017/04/13 17:06:14 stevew Exp $
 *-----------------------------------------------------------------------*/

#include "htm_widget.h"
#include "htm_format.h"
#include "htm_table.h"
#include "htm_font.h"
#include "htm_form.h"
#include "htm_parser.h"
#include "htm_string.h"
#include "htm_tag.h"
#include "htm_image.h"
#include "htm_callback.h"
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>

//-----------------------------------------------------------------------------
// Local definitions

#define IDENT_SPACES        3   // length of each indent, in no of spaces

namespace htm
{
    // Element data bits
    enum
    {
        ELE_ANCHOR              = (1<<0),
        ELE_ANCHOR_TARGET       = (1<<1),
        ELE_ANCHOR_VISITED      = (1<<2),
        ELE_ANCHOR_INTERN       = (1<<3),
        ELE_UNDERLINE           = (1<<4),
        ELE_UNDERLINE_TEXT      = (1<<5),
        ELE_STRIKEOUT           = (1<<6),
        ELE_STRIKEOUT_TEXT      = (1<<7)
    };


    struct listMarkers
    {
        const char *name;
        Marker type;
    };
}

namespace {
    // Marker information for HTML lists, ordered list.
#define OL_ARRAYSIZE    5
    const listMarkers ol_markers[OL_ARRAYSIZE] =
    {
        {"1", MARKER_ARABIC},
        {"a", MARKER_ALPHA_LOWER},
        {"A", MARKER_ALPHA_UPPER},
        {"i", MARKER_ROMAN_LOWER},
        {"I", MARKER_ROMAN_UPPER},
    };

    // Unordered list.
#define UL_ARRAYSIZE    3
    const listMarkers ul_markers[UL_ARRAYSIZE] =
    {
        {"disc", MARKER_DISC},
        {"square", MARKER_SQUARE},
        {"circle", MARKER_CIRCLE},
    };
}


//-----------------------------------------------------------------------------
// Exported utility

// Try to figure out what type of url the given href is.  This
// function is quite forgiving on typos of any url spec:  only the
// first character is checked, the remainder doesn't matter.
//
URLType
htmGetURLType(const char *href)
{
    if (!href || !*href)
        return (ANCHOR_UNKNOWN);

    // first pick up any leading url spec
    if (strchr(href, ':')) {
        // check for URL types we know of. Do in most logical order(?)
        if (!strncasecmp(href, "https", 5))  // must be before http
            return (ANCHOR_SECURE_HTTP);
        if (!strncasecmp(href, "http", 4))
            return (ANCHOR_HTTP);
        if (!strncasecmp(href, "mailto", 6))
            return (ANCHOR_MAILTO);
        if (!strncasecmp(href, "ftp", 3))
            return (ANCHOR_FTP);
        if (!strncasecmp(href, "file", 4))
            return (ANCHOR_FILE_REMOTE);
        if (!strncasecmp(href, "news", 4))
            return (ANCHOR_NEWS);
        if (!strncasecmp(href, "telnet", 6))
            return (ANCHOR_TELNET);
        if (!strncasecmp(href, "gopher", 6))
            return (ANCHOR_GOPHER);
        if (!strncasecmp(href, "wais", 4))
            return (ANCHOR_WAIS);
        if (!strncasecmp(href, "exec", 4) ||
            !strncasecmp(href, "xexec", 5))
            return (ANCHOR_EXEC);
        if (!strncasecmp(href, "pipe", 4))
            return (ANCHOR_PIPE);
        if (!strncasecmp(href, "about", 4))
            return (ANCHOR_ABOUT);
        if (!strncasecmp(href, "man", 4))
            return (ANCHOR_MAN);
        if (!strncasecmp(href, "info", 4))
            return (ANCHOR_INFO);
        return (ANCHOR_UNKNOWN);
    }
    return (href[0] == '#' ? ANCHOR_JUMP : ANCHOR_FILE_LOCAL);
}


//-----------------------------------------------------------------------------
// Widget methods

// Main function to apply formatting.
//
void
htmWidget::format()
{
    // Free any previous lists and initialize
    htm_formatted->free();
    htm_formatted = 0;
    htm_anchor_data->free();
    htm_anchor_data = 0;

    // free table data
    htm_tables->free();
    htm_tables = 0;

    htmFormatManager fm(this);
    fm.formatObjects(htm_elements);

    // Allocate memory for all anchor words in this document.
    if (fm.anchorWords()) {
        htm_anchors = new htmWord*[fm.anchorWords() + 1];
        htm_anchor_words = fm.anchorWords();
    }
    htm_num_anchors = fm.numAnchors();

    // Allocate memory for all named anchors.
    if (fm.namedAnchors()) {
        htm_named_anchors = new htmObjectTable*[fm.namedAnchors() + 1];
        htm_num_named_anchors = fm.namedAnchors();
        for (int i = 0; i <= htm_num_named_anchors; i++)
            htm_named_anchors[i] = 0;
    }

    // Store the final formatted list and anchor list.
    htm_formatted = fm.formattedOutput();
    htm_anchor_data = fm.anchorData();
}


// Check the <BODY> element for additional tags.
//
void
htmWidget::parseBodyTags(htmObject *data)
{
    bool bg_color_set = false;   // flag for bodyImage substitution

    // check all body color tags
    char *chPtr;
    if (htm_body_colors_enabled) {

        if ((chPtr = htmTagGetValue(data->attributes, "text"))) {
            htm_cm.cm_body_fg = htm_cm.getPixelByName(
                chPtr, htm_cm.cm_body_fg_save);
            delete [] chPtr;
        }
        if ((chPtr = htmTagGetValue(data->attributes, "bgcolor")) != 0) {
            bg_color_set = true;
            htm_cm.cm_body_bg = htm_cm.getPixelByName(
                chPtr, htm_cm.cm_body_bg_save);

            // get new values for top, bottom & highlight
            htm_cm.recomputeColors(htm_cm.cm_body_bg);
            delete [] chPtr;
        }
        if ((chPtr = htmTagGetValue(data->attributes, "link")) != 0) {
            htm_cm.cm_anchor_fg = htm_cm.getPixelByName(
                chPtr, htm_cm.cm_anchor_fg_save);
            htm_cm.cm_anchor_target_fg =
                htm_cm.anchor_fg_pixel(htm_cm.cm_anchor_fg);
            delete [] chPtr;
        }
        if ((chPtr = htmTagGetValue(data->attributes, "vlink")) != 0) {
            htm_cm.cm_anchor_visited_fg = htm_cm.getPixelByName(chPtr,
                htm_cm.cm_anchor_visited_fg_save);
            delete [] chPtr;
        }
        if ((chPtr = htmTagGetValue(data->attributes, "alink")) != 0) {
            htm_cm.cm_anchor_activated_fg = htm_cm.getPixelByName(chPtr,
                htm_cm.cm_anchor_activated_fg_save);
            delete [] chPtr;
        }
    }

    // Check background image spec.  First invalidate any existing body image.
    if (htm_im.im_body_image)
        htm_im.im_body_image->options |= IMG_ORPHANED;
    htm_im.im_body_image = 0;
    htm_body_image_url = 0;

    // ALWAYS load the body image if we want the SetValues method to
    // behave itself.

    // preset body_image_url, so the image resolve callback can find it
    if ((chPtr = htmTagGetValue(data->attributes, "background")) != 0) {
        // store document's body image location
        htm_body_image_url = chPtr;
        htm_im.loadBodyImage(chPtr);
        delete [] chPtr;
    }
    // Use default body image if present *and* if no background color
    // has been set.

    else if (!bg_color_set && htm_def_body_image_url) {
        htm_body_image_url = htm_def_body_image_url;
        htm_im.loadBodyImage(htm_def_body_image_url);
    }
    if (htm_im.im_body_image)
        htm_body_image_url = htm_im.im_body_image->url;
    else
        htm_body_image_url = 0;

    // Now nullify it if we aren't to show the background image.
    // makes sense huh?  (the list of images is a global resource so
    // the storage occupied by this unused image is freed when all
    // document data is freed).

    if (!htm_images_enabled || !htm_body_images_enabled) {
        if (htm_im.im_body_image)
            htm_im.im_body_image->options |= IMG_ORPHANED;
        htm_im.im_body_image = 0;
    }

    // When a body image is present it is very likely that a highlight
    // color based upon the current background actually makes an
    // anchor invisible when highlighting is selected.  Therefore we
    // base the highlight color on the activated anchor background
    // when we have a body image, and on the document background when
    // no body image is present.

    if (htm_im.im_body_image)
        htm_cm.recomputeHighlightColor(htm_cm.cm_anchor_activated_fg);
    else
        htm_cm.recomputeHighlightColor(htm_cm.cm_body_bg);
}


htmAnchor *
htmWidget::newAnchor(htmObject *object, htmFormatManager *fmt)
{
    if (!object->attributes)
        return (0);

    htmAnchor *anchor = new htmAnchor();

    // anchors can be both named and href'd at the same time
    anchor->name = htmTagGetValue(object->attributes, "name");

    // get the url specs
    anchor->parseHref(object->attributes);

    // get the url type
    anchor->url_type = htmGetURLType(anchor->href);

    // promote to named if necessary
    if (anchor->url_type == ANCHOR_UNKNOWN && anchor->name)
        anchor->url_type = ANCHOR_NAMED;

    // see if we need to watch any events for this anchor
    anchor->events = checkCoreEvents(object->attributes);

#ifdef PEDANTIC
    if (anchor->url_type == ANCHOR_UNKNOWN) {
        warning("newAnchor", "Could "
            "not determine URL type for anchor %s (line %i of input)\n",
            object->attributes, object->line);
    }
#endif

    if (htm_if) {
        // If we have a proc available for anchor testing, call it and set
        // the visited field.

        htmVisitedCallbackStruct cbs;
        cbs.url = anchor->href;
        cbs.visited = anchor->visited;
        htm_if->emit_signal(S_ANCHOR_VISITED, &cbs);
        anchor->visited = cbs.visited;
    }

    // Insert in the anchor list.  If called with fmt nonzero, we are
    // formatting so link the new anchor into the format context.
    // Otherwise, we are probably dealing with an external imagemap
    // being parsed after the main document.  Add the anchor to the
    // widget's list.

    if (fmt)
        fmt->addAnchor(anchor);
    else {
        if (htm_anchor_data) {
            htmAnchor *atmp = htm_anchor_data;
            while (atmp->next)
                atmp = atmp->next;
            atmp->next = anchor;
        }
        else
            htm_anchor_data = anchor;
    }
    return (anchor);
}
// End of htmWidget functions


namespace {
    // Collapse whitespace in the given text.
    //
    void
    CollapseWhiteSpace(char *text)
    {
        char *outPtr = text;

        // We only collapse valid text and text that contains more than
        // whitespace only.  This should never be true since copyText will
        // filter these things out.  It's just here for sanity.

        if (*text == '\0' || !strlen(text))
            return;

        // Now collapse each occurance of multiple whitespaces.  This may
        // produce different results on different systems since isspace()
        // might not produce the same on each and every platform.

        while (true) {
            switch (*text) {
            case '\f':
            case '\n':
            case '\r':
            case '\t':
            case '\v':
                *text = ' ';    // replace by a single space
                // fall through
            case ' ':
                // skip past first space
                *(outPtr++) = *(text++);
                // collapse every space following
                while (*text != '\0' && isspace(*text))
                    *text++ = '\0';
                break;
            default:
                *(outPtr++) = *(text++);
                break;
            }
            if (*text == 0) {
                *outPtr = '\0';
                return;
            }
        }
    }
}


//-----------------------------------------------------------------------------
// Struct AllEvents methods

AllEvents::AllEvents()
{
    onLoad          = 0;
    onUnload        = 0;

    onSubmit        = 0;
    onReset         = 0;
    onFocus         = 0;
    onBlur          = 0;
    onSelect        = 0;
    onChange        = 0;

    onClick         = 0;
    onDblClick      = 0;
    onMouseDown     = 0;
    onMouseUp       = 0;
    onMouseOver     = 0;
    onMouseMove     = 0;
    onMouseOut      = 0;
    onKeyPress      = 0;
    onKeyDown       = 0;
    onKeyUp         = 0;
}


//-----------------------------------------------------------------------------
// Struct htmAnchor methods

htmAnchor::htmAnchor()
{
    url_type        = ANCHOR_UNKNOWN;
    name            = 0;
    href            = 0;
    target          = 0;
    rel             = 0;
    rev             = 0;
    title           = 0;
    events          = 0;
    line            = 0;
    visited         = false;
    next            = 0;
}


htmAnchor::htmAnchor(htmAnchor &a)
{
    url_type        = a.url_type;
    name            = lstring::copy(a.name);
    href            = lstring::copy(a.href);
    target          = lstring::copy(a.target);
    rel             = lstring::copy(a.rel);
    rev             = lstring::copy(a.rev);
    title           = lstring::copy(a.title);
    events          = 0;
    line            = a.line;
    visited         = a.visited;
    next            = 0;
}


htmAnchor::~htmAnchor()
{
    delete [] name;
    delete [] href;
    delete [] target;
    delete [] rel;
    delete [] rev;
    delete [] title;
    delete events;
}


void
htmAnchor::free()
{
    htmAnchor *anchors = this;
    while (anchors) {
        htmAnchor *tmp = anchors->next;
        delete anchors;
        anchors = tmp;
    }
}


// Return the url specification found in the given anchor.
//
void
htmAnchor::parseHref(const char *text)
{
    if (text == 0 || (href = htmTagGetValue(text, "href")) == 0) {
        // allocate empty href field so later strcmps won't explode
        href = new char[1];
        href[0] = 0;  // fix 02/03/97-05, kdh

        // Could be a named anchor with a target spec.  Rather
        // impossible but allow for it anyway (I can imagine this to
        // be true for a split-screen display).

        if (!text)
            return;
    }

    // check if there is a target specification
    target = htmTagGetValue(text, "target");

    // also check for rel, rev and title
    rel = htmTagGetValue(text, "rel");
    rev = htmTagGetValue(text, "rev");
    title = htmTagGetValue(text, "title");
}


//-----------------------------------------------------------------------------
// Struct htmWord methods

htmWord::htmWord()
{
    ybaseline           = 0;
    line                = 0;
    type                = OBJ_NONE;
    word                = 0;
    len                 = 0;
    font                = 0;
    line_data           = 0;
    spacing             = 0;
    events              = 0;
    image               = 0;
    form                = 0;
    base                = 0;
    self                = 0;
    owner               = 0;
}


// Update positioning, used in layout.
//
int
htmWord::update(int lineno, int xp, int yp)
{
    line = lineno;
    area.x = xp + (owner ? owner->x_offset : 0);
    area.y = yp + (owner ? owner->y_offset : 0);
    ybaseline = area.y;
    if (type == OBJ_TEXT && font)
        area.y -= font->ascent;
    return (area.right());
}


void
htmWord::set_baseline(int ybl)
{
    ybaseline = ybl;
    area.y = ybl;
    if (type == OBJ_TEXT && font)
        area.y -= font->ascent;
}


//-----------------------------------------------------------------------------
// Struct htmObjectTable methods

htmObjectTable::~htmObjectTable()
{
    delete [] text;
    if (n_words) {
        // only the first word contains a valid ptr, all others
        // point to some char in this buffer, so freeing them
        // will cause a segmentation fault eventually.

        delete [] words[0].word;
        delete [] words;
    }
}


void
htmObjectTable::reset(htmObject *obj)
{
    line                = 0;
    id                  = 0;
    object_type         = OBJ_NONE;
    text                = 0;
    text_data           = 0;
    len                 = 0;
    y_offset            = 0;
    x_offset            = 0;
    object              = obj;
    anchor              = 0;
    words               = 0;
    form                = 0;
    table               = 0;
    table_cell          = 0;
    n_words             = 0;
    anchor_state        = 0;
    halign              = HALIGN_NONE;
    linefeed            = 0;
    ident               = 0;
    marker              = MARKER_NONE;
    list_level          = 0;
    font                = 0;
    fg                  = 0;
    bg                  = 0;
    next                = 0;
    prev                = 0;
}


void
htmObjectTable::free()
{
    htmObjectTable *list = this;
    while (list) {
        htmObjectTable *temp = list->next;
        delete list;
        list = temp;
    }
}


//-----------------------------------------------------------------------------
// Struct htmFormatManager methods

htmFormatManager::htmFormatManager(htmWidget *html)
{
    f_html                      = html;
    f_object                    = 0;
    f_cx                        = 0;

    f_text                      = 0;
    f_linefeed                  = 0;

    f_words                     = 0;
    f_n_words                   = 0;
    f_anchor_words              = 0;
    f_named_anchors             = 0;
    f_x_offset                  = 0;
    f_y_offset                  = 0;
    f_text_data                 = 0;
    f_line_data                 = 0;
    f_element_data              = 0;
    f_anchor_data               = 0;
    f_form_anchor_data          = 0;

    f_ul_level                  = 0;
    f_ol_level                  = 0;
    f_ident_level               = 0;
    f_current_list              = 0;
    f_in_dt                     = 0;

    f_width                     = 0;
    f_height                    = 0;
    f_halign                    = HALIGN_NONE;
    f_valign                    = VALIGN_NONE;
    f_object_type               = OBJ_NONE;
    f_element                   = 0;
    f_previous_element          = 0;

    f_fg                        = 0;
    f_bg                        = 0;
    f_bg_image                  = 0;
    f_fontsize                  = 3;
    f_basefontsize              = 3;
    f_font                      = 0;
    f_lastfont                  = 0;

    f_imageMap                  = 0;

    f_table                     = 0;
    f_current_table             = 0;
    f_table_cell                = 0;

    f_new_anchors               = 0;
    f_anchor_data_used          = false;

    f_ignore                    = false;
    f_pre_nl                    = false;
    f_in_pre                    = false;

    // initialize dispatch table

    dtable[HT_DOCTYPE]      = &htmFormatManager::a_nop;
    dtable[HT_A]            = &htmFormatManager::a_A;
    dtable[HT_ADDRESS]      = &htmFormatManager::a_ADDRESS;
    dtable[HT_APPLET]       = &htmFormatManager::a_APPLET;
    dtable[HT_AREA]         = &htmFormatManager::a_AREA;
    dtable[HT_B]            = &htmFormatManager::a_B_etc;
    dtable[HT_BASE]         = &htmFormatManager::a_nop;
    dtable[HT_BASEFONT]     = &htmFormatManager::a_BASEFONT;
    dtable[HT_BIG]          = &htmFormatManager::a_BIG;
    dtable[HT_BLOCKQUOTE]   = &htmFormatManager::a_BLOCKQUOTE;
    dtable[HT_BODY]         = &htmFormatManager::a_nop;
    dtable[HT_BR]           = &htmFormatManager::a_BR;
    dtable[HT_CAPTION]      = &htmFormatManager::a_CAPTION;
    dtable[HT_CENTER]       = &htmFormatManager::a_CENTER;
    dtable[HT_CITE]         = &htmFormatManager::a_B_etc;
    dtable[HT_CODE]         = &htmFormatManager::a_B_etc;
    dtable[HT_DD]           = &htmFormatManager::a_DD;
    dtable[HT_DFN]          = &htmFormatManager::a_B_etc;
    dtable[HT_DIR]          = &htmFormatManager::a_DIR_MENU_UL;
    dtable[HT_DIV]          = &htmFormatManager::a_DIV;
    dtable[HT_DL]           = &htmFormatManager::a_DL;
    dtable[HT_DT]           = &htmFormatManager::a_DT;
    dtable[HT_EM]           = &htmFormatManager::a_B_etc;
    dtable[HT_FONT]         = &htmFormatManager::a_FONT;
    dtable[HT_FORM]         = &htmFormatManager::a_FORM;
    dtable[HT_FRAME]        = &htmFormatManager::a_nop;
    dtable[HT_FRAMESET]     = &htmFormatManager::a_nop;
    dtable[HT_H1]           = &htmFormatManager::a_H1_6;
    dtable[HT_H2]           = &htmFormatManager::a_H1_6;
    dtable[HT_H3]           = &htmFormatManager::a_H1_6;
    dtable[HT_H4]           = &htmFormatManager::a_H1_6;
    dtable[HT_H5]           = &htmFormatManager::a_H1_6;
    dtable[HT_H6]           = &htmFormatManager::a_H1_6;
    dtable[HT_HEAD]         = &htmFormatManager::a_nop;
    dtable[HT_HR]           = &htmFormatManager::a_HR;
    dtable[HT_HTML]         = &htmFormatManager::a_nop;
    dtable[HT_I]            = &htmFormatManager::a_B_etc;
    dtable[HT_IMG]          = &htmFormatManager::a_IMG;
    dtable[HT_INPUT]        = &htmFormatManager::a_INPUT;
    dtable[HT_ISINDEX]      = &htmFormatManager::a_nop;
    dtable[HT_KBD]          = &htmFormatManager::a_B_etc;
    dtable[HT_LI]           = &htmFormatManager::a_LI;
    dtable[HT_LINK]         = &htmFormatManager::a_nop;
    dtable[HT_MAP]          = &htmFormatManager::a_MAP;
    dtable[HT_MENU]         = &htmFormatManager::a_DIR_MENU_UL;
    dtable[HT_META]         = &htmFormatManager::a_nop;
    dtable[HT_NOFRAMES]     = &htmFormatManager::a_nop;
    dtable[HT_OL]           = &htmFormatManager::a_OL;
    dtable[HT_OPTION]       = &htmFormatManager::a_OPTION;
    dtable[HT_P]            = &htmFormatManager::a_P;
    dtable[HT_PARAM]        = &htmFormatManager::a_PARAM;
    dtable[HT_PRE]          = &htmFormatManager::a_PRE;
    dtable[HT_SAMP]         = &htmFormatManager::a_B_etc;
    dtable[HT_SCRIPT]       = &htmFormatManager::a_SCRIPT;
    dtable[HT_SELECT]       = &htmFormatManager::a_SELECT;
    dtable[HT_SMALL]        = &htmFormatManager::a_SMALL;
    dtable[HT_STRIKE]       = &htmFormatManager::a_STRIKE;
    dtable[HT_STRONG]       = &htmFormatManager::a_B_etc;
    dtable[HT_STYLE]        = &htmFormatManager::a_STYLE;
    dtable[HT_SUB]          = &htmFormatManager::a_SUB_SUP;
    dtable[HT_SUP]          = &htmFormatManager::a_SUB_SUP;
    dtable[HT_TAB]          = &htmFormatManager::a_TAB;
    dtable[HT_TABLE]        = &htmFormatManager::a_TABLE;
    dtable[HT_TD]           = &htmFormatManager::a_TD_TH;
    dtable[HT_TEXTAREA]     = &htmFormatManager::a_TEXTAREA;
    dtable[HT_TH]           = &htmFormatManager::a_TD_TH;
    dtable[HT_TITLE]        = &htmFormatManager::a_nop;
    dtable[HT_TR]           = &htmFormatManager::a_TR;
    dtable[HT_TT]           = &htmFormatManager::a_B_etc;
    dtable[HT_U]            = &htmFormatManager::a_U;
    dtable[HT_UL]           = &htmFormatManager::a_DIR_MENU_UL;
    dtable[HT_VAR]          = &htmFormatManager::a_B_etc;
    dtable[HT_ZTEXT]        = &htmFormatManager::a_ZTEXT;
}


htmFormatManager::~htmFormatManager()
{
    delete f_cx;
}


// Create a list of formatted HTML objects.
//
void
htmFormatManager::formatObjects(htmObject *obj_list)
{
    if (!f_html)
        return;

    f_object = obj_list;
    find_first_element();
    if (!f_object)
        return;

    if (!f_cx)
        f_cx = new htmFormatContext();
    f_cx->init();

    // initialize font stack
    f_font = f_html->loadDefaultFont();
    f_lastfont = f_font;
    f_cx->setFont(f_font, f_basefontsize);

    // Reset anchor count
    f_anchor_words = 0;
    f_named_anchors = 0;
    f_anchor_data = 0;
    f_form_anchor_data = 0;

    // initialize list variables
    f_ul_level = 0;
    f_ol_level = 0;
    f_ident_level = 0;
    f_current_list = 0;

    // reset stacks
    for (int i = 0; i < MAX_NESTED_LISTS; i++) {
        f_list_stack[i].isindex = false;
        f_list_stack[i].marker  = MARKER_NONE;
        f_list_stack[i].level   = 0;
        f_list_stack[i].type    = HT_ZTEXT;
    }

    // Initialize linefeeding mechanism
    f_linefeed = checkLineFeed(LF_DOWN_2, true);

    // Initialize alignment
    f_halign = f_html->htm_default_halign;
    f_valign = VALIGN_NONE;
    f_object_type = OBJ_NONE;
    f_cx->setAlignment(f_halign);

    // check for background stuff
    f_html->parseBodyTags(f_object);
    f_object = f_object->next;

    // foreground color to use
    f_fg = f_html->htm_cm.cm_body_fg;
    f_cx->setFGColor(f_fg);

    // background color to use
    f_bg = f_html->htm_cm.cm_body_bg;
    f_cx->setBGColor(f_bg);

    // background image to use
    f_bg_image = f_html->htm_im.im_body_image;
    f_cx->setBGImage(f_bg_image);

    // Insert a dummy element at the head of the list to prevent
    // incorrect handling of the first real element to be rendered.

    f_element = new htmObjectTable(f_object);
    f_element->object_type = OBJ_NONE;
    f_element->font = f_html->htm_default_font;
    f_cx->insertElement(f_element);

    // Only elements between <BODY></BODY> elements are really
    // interesting.  BUT:  if the HTML verification/reparation
    // routines in parse.c should fail, we might have a premature
    // </body> element, so we don't check on it but walk thru every
    // item found.

    while (f_object != 0) {
        if (f_ignore)
            // Reuse current element if it wasn't needed on the
            // previous pass.
            f_element->reset(f_object);
        else
            f_element = new htmObjectTable(f_object);

        // initialize elements
        init();

        // process
        (this->*dtable[f_object->id])();

        if (!f_ignore)
            setup_element(f_object);

        // move to next element
        if (f_object)
            f_object = f_object->next;
    }

    // If there still is an open table, close it.
    if (f_table)
        f_table->close(f_html, f_element, &f_table_cell);

    // Insert a dummy element at the end of the list, saves some 0
    // tests in the layout functions.
    if (f_ignore)
        f_element->reset(0);
    else
        f_element = new htmObjectTable(0);
    f_cx->insertElement(f_element);

    // If some sucker forget to terminate a list and the parser failed
    // to repair it, spit out a warning.
    if (f_html->htm_bad_html_warnings && f_ident_level != 0)
        f_html->warning("formatObjects", "Non-zero indentation at "
            "end of input. Check your document.");

    // clear stacks
    f_cx->clear();
}


// Add the anchor to the list.
//
void
htmFormatManager::addAnchor(htmAnchor *anchor)
{
    if (f_cx->anchor_head) {
        // We can't do anything about duplicate anchors.  Removing
        // them would mess up the named anchor lookup table.
        f_cx->anchor_current->next = anchor;
        f_cx->anchor_current = anchor;
    }
    else
        f_cx->anchor_head = f_cx->anchor_current = anchor;
}


// Initialize all fields changed in dispatch loop.
void
htmFormatManager::init()
{
    f_text                  = 0;
    f_ignore                = false;
    f_object_type           = OBJ_NONE;
    f_n_words               = 0;
    f_width                 = 0;
    f_height                = 0;
    f_words                 = 0;
    f_linefeed              = LF_NONE;
    f_line_data             = NO_LINE;
    f_text_data             = TEXT_SPACE_NONE;
    f_lastfont              = f_font;
}


// Move to the first element suitable for formatting.  If f_object is
// null on return, there is no suitable text for formatting.
//
void
htmFormatManager::find_first_element()
{
    // Move to the body element
    htmObject *start_object = f_object;
    while (f_object && f_object->id != HT_BODY)
        f_object = f_object->next;

    if (!f_object) {
        // No <body> element found.  This is an error since the parser
        // will always add a <body> element if none is present in the
        // source document.

        // The only exception is a document only containing plain text
        // and no BODY tag was present in the input.  In this case,
        // check the input and start outputting at the end of the
        // first text element not belonging to the head of a document.
        // Fix 01/04/98-01, kdh

        htmObject *last_obj = 0;
        for (htmObject *tmp = start_object; tmp; tmp = tmp->next) {
            switch (tmp->id) {
            case HT_DOCTYPE:
            case HT_BASE:
                // these two have no ending tag
                last_obj = tmp;
                break;
            case HT_HTML:
                // don't use the closing tag for this, it's the end...
                if (!tmp->is_end)
                    last_obj = tmp;
                break;
            case HT_HEAD:
            case HT_TITLE:
            case HT_SCRIPT:
            case HT_STYLE:
                // pick up the last closing tag
                if (tmp->is_end)
                    last_obj = tmp;
            default:
                break;
            }
        }

        if (!last_obj || !last_obj->next)
            // nothing to display - no text loaded
            return;

        // we move to the correct object a bit further down
        f_object = last_obj;
    }
}


// Set the element fields and save the element.
//
void
htmFormatManager::setup_element(htmObject *temp)
{
    // adjust anchor count
    if (f_element_data & ELE_ANCHOR) {
        f_text_data |= TEXT_ANCHOR;
        f_anchor_words += f_n_words;
        f_anchor_data_used = true;
    }
    // mark object as internal anchor
    if (f_element_data & ELE_ANCHOR_INTERN) {
        f_text_data |= TEXT_ANCHOR_INTERN;
        f_named_anchors++;
        f_anchor_data_used = true;
    }
    f_element->text = f_text;
    f_element->text_data = f_text_data;
    f_element->words = f_words;
    f_element->n_words = f_n_words;
    f_element->area.width = f_width;
    f_element->area.height = f_height;
    f_element->fg = f_fg;
    f_element->bg = f_bg;
    f_element->font = f_font;
    f_element->marker = f_list_stack[f_current_list].marker;
    f_element->list_level = f_list_stack[f_current_list].level;
    f_element->table = f_table;
    f_element->table_cell = f_table_cell;

    // <dt> elements have an identation one less than the current.
    // All identation must use the default font (consistency).
    int id = (temp->id == HT_TABLE && !temp->is_end && f_table ?
        f_table->t_ident : f_ident_level);
    if (f_in_dt && id)
        f_element->ident = (id-1) * IDENT_SPACES *
            f_html->htm_default_font->width;
    else
        f_element->ident = id * IDENT_SPACES *
            f_html->htm_default_font->width;

    switch (f_linefeed) {
    case LF_NONE:
        f_element->linefeed = 0;
        break;
    default:
    case LF_DOWN_1:
        f_element->linefeed = temp->is_end ?
            f_lastfont->lineheight : f_font->lineheight;
        break;
    case LF_DOWN_2:
        f_element->linefeed = temp->is_end ?
            f_lastfont->lineheight : f_font->lineheight;
        if (temp->is_end)
            f_element->linefeed += f_font->lineheight;
        else
            f_element->linefeed += f_lastfont->lineheight;
    }

    // stupid hack so HT_HR won't mess up alignment and color stack
    if (temp->id != HT_HR) {
        f_element->halign = f_halign;
        f_element->y_offset = f_y_offset;
        f_element->x_offset = f_x_offset;
        if (temp->id == HT_BR && (f_halign == CLEAR_LEFT ||
                f_halign == CLEAR_RIGHT || f_halign == CLEAR_ALL))
            f_halign = f_cx->popAlignment();
    }
    else if (f_html->htm_allow_color_switching)
        f_fg = f_cx->popFGColor();

    f_element->object_type = f_object_type;

    if (f_object_type == OBJ_BULLET)
        fillBullet();

    // If we have a form component of type <input
    // type="image">, we have promoted it to an anchor.  Set
    // this anchor data as the anchor for this element and, as
    // it is used only once, reset it to 0.  In all other case
    // we have a plain anchor.
    //
    // Note:  as form components are allowed inside anchors,
    // this is the only place in which we can possibly have
    // nested anchors.  This is a problem we will have to live
    // with...

    if (f_form_anchor_data) {
        f_element->anchor = f_form_anchor_data;
        f_form_anchor_data = 0;
        f_element_data &= ~ELE_ANCHOR;
    }
    else
        f_element->anchor = f_anchor_data;

    // add an anchor id if this data belongs to a named anchor
    if (f_element_data & ELE_ANCHOR_INTERN)
        f_element->id = f_named_anchors;

    f_cx->insertElement(f_element);
    f_previous_element = f_element;
}


// Split the given text into an array of words.
//
void
htmFormatManager::textToWords()
{
    if (!f_text) {
        f_height = f_n_words = 0;
        f_words = 0;
        return;
    }

    // compute how many words we have
    int n_words = 0;
    for (char *chPtr = f_text; *chPtr; chPtr++) {
        if (*chPtr == ' ')
            n_words++;
    }
    // also pick up the last word
    n_words++;

    // copy text
    char *raw = lstring::copy(f_text);

    // note that words[0].word -> raw, which should be freed

    // allocate memory for all words
    htmWord *words = new htmWord[n_words];

    // Split the text in words and fill in the appropriate fields.
    f_height = f_font->height;
    char *chPtr = raw;
    char *start = raw;

    int i = 0;
    for (int j = 0, len = 0; ; chPtr++, len++, j++) {
        // also pick up the last word!
        if (*chPtr == ' ' || !*chPtr) {
            if (*chPtr) {
                chPtr++;            // nuke the space
                raw[j++] = 0;
            }
            // fill in required fields
            words[i].self      = &words[i];
            words[i].word      = start;
            words[i].len       = len;
            words[i].area.height = f_height;
            words[i].area.width  =
                f_html->htm_tk->tk_text_width(f_font, words[i].word, len);
            words[i].owner     = f_element;
            words[i].font      = f_font;
            words[i].spacing   = TEXT_SPACE_LEAD | TEXT_SPACE_TRAIL;
            words[i].type      = OBJ_TEXT;
            words[i].line_data = f_line_data;

            start = chPtr;
            i++;
            len = 0;
        }
        if (!*chPtr)
            break;
    }
    // When there is more than one word in this block, the first word
    // _always_ has a trailing space.  Likewise, the last word always
    // has a leading space.

    if (n_words > 1) {
        // unset nospace bit
        unsigned char spacing = f_text_data & ~TEXT_SPACE_NONE;
        words[0].spacing = spacing | TEXT_SPACE_TRAIL;
        words[n_words-1].spacing = spacing | TEXT_SPACE_LEAD;
    }
    else
        words[0].spacing = f_text_data;

    f_n_words = i;  // n_words
    f_words = words;
}


// Convert an image to a word.
//
bool
htmFormatManager::imageToWord(const char *attributes)
{
    f_n_words = 0;
    f_height = 0;
    if (!attributes)
        return (false);

    unsigned width;
    htmImage *image = f_html->htm_im.newImage(attributes, &width, &f_height);
    if (!image) {
        f_height = 0;
        return (false);
    }

    htmWord *word = new htmWord[1];

    // required for image anchoring/replace/update
    image->owner = f_element;

    // fill in required fields
    word->self   = word;
    word->word   = lstring::copy(image->alt);        // we always have this
    word->len    = strlen(image->alt);
    word->area.width  = width + 2*image->hspace + 2*image->border;
    word->area.height = f_height + 2*image->vspace + 2*image->border;
    word->owner  = f_element;
    word->font   = f_cx->font_base.font;    // always use the default font

    // If image support is disabled, add width of the alt text to the
    // image width (either from default image or specified in the
    // doc).  This is required for proper exposure handling when
    // images are disabled.

    if (!f_html->htm_images_enabled)
        word->area.width +=
            f_html->htm_tk->tk_text_width(word->font, word->word, word->len);

    // No spacing if part of a chunk of <pre></pre> text.
    // Fix 07/24/97, kdh

    word->spacing = f_in_pre ? 0 : TEXT_SPACE_LEAD | TEXT_SPACE_TRAIL;
    word->type = OBJ_IMG;
    word->line_data = (f_line_data & ALT_STYLE);  // no underlining for images
    word->image = image;

    f_n_words = 1;
    f_words = word;
    return (true);
}


// Allocate a default htmWord for use within a HTML form.  The
// formatted arg is true true when allocating a form component present
// in <pre></pre>.
//
void
htmFormatManager::allocFormWord(htmForm *form)
{
    htmWord *word = new htmWord[1];

    // fill in required fields
    word->self    = word;
    word->word    = lstring::copy(form->name);       // we always have this
    word->len     = strlen(form->name);
    word->area.height = f_height = form->height;
    word->area.width  = f_width  = form->width;
    word->owner   = f_element;
    word->font    = f_cx->font_base.font;   // always use default font
    word->spacing = f_in_pre ? 0 : TEXT_SPACE_LEAD | TEXT_SPACE_TRAIL;
    word->type    = OBJ_FORM;
    word->form    = form;

    f_words = word;
    f_n_words = 1;
}


// Convert a HTML form <input> element to a word.  The formatted arg
// is true when this form component is placed in a <pre></pre> tag.
//
bool
htmFormatManager::inputToWord(const char *attributes)
{
    f_n_words = 0;
    if (!attributes)
        return (false);
    htmForm *form_entry = f_html->formAddInput(attributes);
    if (!form_entry)
        return (false);

    // save owner, we need it in the paint routines
    form_entry->data = f_element;

    // image buttons are treated as anchored images
    if (form_entry->type == FORM_IMAGE) {
        if (!imageToWord(attributes))
            return (false);
        // remove alt text
        delete [] f_words->word;
        // use form member name instead
        f_words->word = lstring::copy(form_entry->name);
        f_words->len  = strlen(form_entry->name);
        f_words->form = form_entry;
        return (true);
    }

    // allocate new word for this form member
    allocFormWord(form_entry);
    return (true);
}


// Convert a HTML form <select></select> to a HTMLWord, also process
// any <option></option> items within this select.  The formatted arg
// is true when this form component is placed in a <pre></pre> tag.
//
bool
htmFormatManager::selectToWord(htmObject *start)
{
    f_n_words = 0;
    if (!start->attributes)
        return (false);

    htmForm *form_entry = f_html->formAddSelect(start->attributes);
    if (!form_entry)
        return (false);

    // save owner
    form_entry->data = f_element;

    // add all option tags
    htmObject *tmp = start->next;
    for ( ; tmp && tmp->id != HT_SELECT; tmp = tmp->next) {
        if (tmp->id == HT_OPTION && !tmp->is_end) {
            htmObject *sel_start = tmp;

            // The next object should be plain text, if not it's an
            // error and we should ignore it.

            tmp = tmp->next;
            if (tmp->id != HT_ZTEXT) {
                if (f_html->htm_bad_html_warnings) {
                    // empty option tag, ignore it
                    if (tmp->id == HT_OPTION)
                        f_html->warning("selectToWord",
                            "Empty <OPTION> tag, ignored (line %i in input).",
                            tmp->line);
                    else
                        f_html->warning("selectToWord",
                            "<%s> not allowed inside <OPTION> tag, ignored "
                            "(line %i in input).", html_tokens[tmp->id],
                            tmp->line);
                }
                continue;
            }
            // get text
            unsigned char foo;
            char *text = copyText(tmp->element, false, &foo, true);
            if (!text)
                continue;

            CollapseWhiteSpace(text);
            if (strlen(text)) {
                f_html->formSelectAddOption(form_entry,
                    sel_start->attributes, text);
                // no longer needed
                delete [] text;
            }
        }
    }
    // close this selection
    f_html->formSelectClose(form_entry);

    // allocate new word for this form member
    allocFormWord(form_entry);

    f_n_words = 1;
    return (true);
}


// Convert a HTML form <textarea> to a HTMLWord.  The formatted arg is
// true when this form component is placed in a <pre></pre> tag.
//
bool
htmFormatManager::textAreaToWord(htmObject *start)
{
    f_n_words = 0;
    f_height = f_width = 0;
    if (!start->attributes)
        return (false);

    // get text between opening and closing <textarea>, if any
    char *text = 0;
    if (start->next->id == HT_ZTEXT) {
        unsigned char foo;
        text = copyText(start->next->element, true, &foo, false);
    }

    // create new form entry. text will serve as the default content
    htmForm *form_entry = f_html->formAddTextArea(start->attributes, text);
    if (!form_entry) {
        delete [] text;
        return (false);
    }
    form_entry->data = f_element;

    // allocate new word for this form member
    allocFormWord(form_entry);

    f_n_words = 1;
    return (true);
}


// Create a prefix for numbered lists with the ISINDEX attribute set.
// The formatted arg is true when this form component is placed in a
// <pre></pre> tag.  This function creates the prefix based on the
// type and depth of the current list.  All types can be intermixed,
// so this routine is capable of returning something like 1.A.IV.c.iii
// for a list nested five levels, the first with type `1', second with
// type `A', third with type `I', fourth with type `a' and fifth with
// type `i'.
//
void
htmFormatManager::indexToWord()
{
    char index[128], number[42];  // enough for a zillion numbers and depths
    memset(index, 0, 128);

    for (int i = 0; i < f_current_list; i++) {
        if (f_list_stack[i].type == HT_OL) {
            switch (f_list_stack[i].marker) {
            case MARKER_ALPHA_LOWER:
                sprintf(number, "%s.", ToAsciiLower(f_list_stack[i].level));
                break;
            case MARKER_ALPHA_UPPER:
                sprintf(number, "%s.", ToAsciiUpper(f_list_stack[i].level));
                break;
            case MARKER_ROMAN_LOWER:
                sprintf(number, "%s.", ToRomanLower(f_list_stack[i].level));
                break;
            case MARKER_ROMAN_UPPER:
                sprintf(number, "%s.", ToRomanUpper(f_list_stack[i].level));
                break;
            case MARKER_ARABIC:
            default:
                sprintf(number, "%i.", f_list_stack[i].level);
                break;
            }
            // no buffer overflow
            if (strlen(index) + strlen(number) > 128)
                break;
            strcat(index, number);
        }
    }

    // fill in required fields
    htmWord *word = new htmWord[1];
    word->word      = lstring::copy(index);
    word->len       = strlen(index);
    word->self      = word;                 // unused
    word->owner     = f_element;            // unused
    word->font      = f_cx->font_base.font; // unused
    word->spacing   = f_in_pre ? 0 : TEXT_SPACE_NONE;
    word->type      = OBJ_TEXT;             // unused
    word->line_data = NO_LINE;              // unused

    f_words = word;
    f_n_words = 1;
}


// Split the given text into an array of preformatted lines.  The
// static var nchars is used to propagate the tab index to another
// chunk of preformatted text if the current text is a block of
// preformatted text with whatever formatting.  It is only reset if an
// explicit newline is encountered.
//
bool
htmFormatManager::textToPre()
{
    f_n_words = 0;
    if (!f_text || !*f_text)
        return (false);

    char *chPtr = f_text;

    // Compute how many words we have.  A preformatted word is started
    // with a printing char and is terminated by either a newline or a
    // sequence of whitespaces.  Multiple newlines are collapsed into
    // a single word where the height of the word indicates the number
    // of newlines to insert.  The in_word logic comes from GNU wc.

    bool in_word = true;
    int nwords = 1, ntabs = 1;   // fix 01/30/97-02, kdh
    while (true) {
        switch (*chPtr) {
        // tabs and single spaces are collapsed
        case '\t':  // horizontal tab
        case ' ':
            if (in_word) {
                while (*chPtr && (*chPtr == ' ' || *chPtr == '\t')) {
                    if (*chPtr == '\t')
                        ntabs++;    // need to know how many to expand
                    chPtr++;
                }
                nwords++;
                in_word = false;
            }
            else {
                // fix 03/23/97-01, kdh
                if (*chPtr == '\t')
                    ntabs++;    // need to know how many to expand
                chPtr++;
            }
            break;
        // newlines reset the tab index and are collapsed
        case '\n':
            while (*chPtr && *chPtr == '\n')
                chPtr++;
            nwords++;   // current word is terminated
            f_cx->numchars = 1;
            break;
        default:
            chPtr++;
            in_word = true;
            break;
        }
        if (!*chPtr)
            break;
    }

    // sanity check
    if (!nwords) {
        f_n_words = 0;
        return (false);
    }

    // add an extra word and tab for safety
    nwords++;   // preformatted text with other formatting needs this
    ntabs++;

    // compute amount of memory to allocate
    int size = (ntabs*8) + strlen(f_text) + 1;

    char *raw = new char[size];

    // allocate memory for all words
    htmWord *words = new htmWord[nwords];

    int len;
    chPtr = f_text;
    char *end = raw;
    // first filter out all whitespace and other non-printing characters
    while (true) {
        switch (*chPtr) {
        case '\f':  // formfeed, ignore
        case '\r':  // carriage return, ignore
        case '\v':  // vertical tab, ignore
            chPtr++;
            break;
        case '\t':  // horizontal tab
            // no of ``floating spaces'' to emulate a tab
            len = ((f_cx->numchars / 8) + 1) * 8;
            for (int j = 0; j < (len - f_cx->numchars); j++)
                *end++ = ' ';       // insert a tab
            f_cx->numchars = len;
            chPtr++;
            break;
        // newlines reset the tab index
        case '\n':
            f_cx->numchars = 0;  // reset tab spacing index
            // fall thru
        default:
            f_cx->numchars++;
            *end++ = *chPtr++;
            break;
        }
        if (!*chPtr) {
            // terminate loop
            *end = '\0';
            break;
        }
    }

    // now go and fill all words
    char *start = end = raw;
    len = 0;
    int nfeeds = 0;

    // Previously this was set up so that the words saved for a
    // sentence like "now is the time" were "now", "now is", "now is
    // the", etc.  Although it doesn't cost storage, it is still
    // inefficient wrt rendering.  So, words are now saved as for
    // normal text, and the width is tweeked to account for space.

    int sp = f_font->isp;
    int nsp = 0;

    int i = 0;
    while (true) {
        // also pick up the last word!
        if (*end == ' ' || *end == '\n' || *end == '\0') {
            char *wend = end;
            if (*end) {
                // skip past all spaces
                while (*end == ' ') {
                    end++;
                    nsp++;
                }

                // If this word is ended by a newline, remove the
                // newline.  X doesn't know how to interpret them.
                // We also want to recognize multiple newlines, so
                // we must skip past them.

                if (*end == '\n') {
                    while (*end == '\n') {
                        nfeeds++;
                        *end++ = '\0';
                    }

                    // Since the no of newlines to add is stored
                    // in a guchar, we need to limit the no of
                    // newlines to the max.  value a guchar can
                    // have:  255 (= 2^8)

                    if (nfeeds > 255)
                        nfeeds = 255;
                }
            }
            *wend = 0;

            words[i].type      = OBJ_TEXT;
            words[i].self      = &words[i];
            words[i].word      = start;
            words[i].area.height = f_font->height;
            words[i].owner     = f_element;
            words[i].spacing   = nfeeds;  // no of newlines
            words[i].font      = f_font;
            words[i].line_data = f_line_data;
            words[i].len       = len;
            words[i].area.width  = nsp*sp +
                f_html->htm_tk->tk_text_width(f_font, words[i].word, len);

            start = end;
            i++;
            len = 0;
            nfeeds = 0;
            nsp = 0;
        }
        if (!*end)   // terminate loop
            break;
        end++;  // move to the next char
        len++;
    }

    f_words = words;
    f_n_words = i;
    return (true);
}


void
htmFormatManager::makeDummyWord()
{
    htmWord *word = new htmWord[1];
    word->type      = OBJ_TEXT;
    word->self      = word;
    word->word      = new char[1];  // needs an empty word
    word->word[0]   = 0;
    word->len       = 0;
    word->area.height = f_height = f_font->height; // height of current font
    word->owner     = f_element;
    word->line_data = f_line_data;
    word->font      = f_font;
    // an empty word acts as a single space
    word->spacing   = 0;
    f_words = word;
    f_n_words = 1;
}


// Return a htmWord with spaces required for a tab.
//
void
htmFormatManager::setTab(int size)
{
    // The tab itself
    char *raw = new char[size+1];

    // fill with spaces
    memset(raw, ' ', size);
    raw[size] = 0;  // 0 terminate

    htmWord *tab = new htmWord[1];

    // Set all text fields for this tab
    tab->self      = tab;
    tab->word      = raw;
    tab->len       = size;
    tab->area.height = f_height = f_font->height;
    tab->area.width  = f_html->htm_tk->tk_text_width(f_font, raw, size);
    tab->owner     = f_element;
    tab->spacing   = TEXT_SPACE_NONE;   // a tab is already spacing
    tab->font      = f_font;
    tab->type      = OBJ_TEXT;
    tab->line_data = NO_LINE;
    f_words = tab;
    f_n_words = 1;
}


// Copy the given text to a newly malloc'd buffer.  formatted is true
// when this text occurs inside <pre></pre>.  text_data is text option
// bits, spacing and such.  expand_escapes is true -> expand escape
// sequences in text.  Only viable when copying pre-formatted text
// (plain text documents are handled internally as consisting
// completely of preformatted text for which the escapes may not be
// expanded).
//
char *
htmFormatManager::copyText(const char *text, bool formatted,
    unsigned char *text_data, bool expand_escapes)
{
    if (!text || !*text)
        return (0);

    // preformatted text, just copy and return
    if (formatted) {
        *text_data = TEXT_SPACE_NONE;
        // expand all escape sequences in this text
        char *ret_val;
        if (expand_escapes)
            ret_val = expandEscapes(text, f_html->htm_bad_html_warnings,
                f_html);
        else
            ret_val = lstring::copy(text);
        f_cx->have_space = false;
        return (ret_val);
    }

    // initial length of full text
    int len = strlen(text);

    *text_data = 0;

    // see if we have any leading/trailing spaces
    if (isspace(*text) || f_cx->have_space)
        *text_data = TEXT_SPACE_LEAD;

    if (isspace(text[len-1]))
        *text_data |= TEXT_SPACE_TRAIL;

    // Remove leading/trailing spaces
    // very special case: spaces between different text formatting
    // elements must be retained

    // remove all leading space
    const char *start = text;
    while (*start && isspace(*start))
        start++;
    // remove all trailing space
    len = strlen(start);
    while (len > 0 && isspace(start[len-1]))
        len--;

    // Spaces can appear between different text formatting elements.
    // We want to retain this spacing since the above whitespace
    // checking only yields the current element, and does not take the
    // previous text element into account.  So when the current
    // element doesn't have any leading or trailing spaces, we use the
    // spacing from the previous full whitespace element.  Obviously
    // we must reset this data if we have text to process.
    //
    // Very special case:  this text only contains whitespace and its
    // therefore most likely just spacing between formatting elements.
    // If the next text elements are in the same paragraph as this
    // single whitespace, we need to add a leading space if that text
    // doesn't have leading spaces.  That's done above.  If we have
    // plain text, we reset the prev_spacing or it will mess up the
    // layout later on.

    if (!len) {
        f_cx->have_space = true;
        return (0);
    }
    f_cx->have_space = false;

    // We are a little bit to generous here:  consecutive multiple
    // whitespace will be collapsed into a single space, so we may
    // over-allocate.  Hey, better to overdo this than to have one
    // byte to short)

    char *ret_val = new char[len + 1];
    strncpy(ret_val, start, len);   // copy it
    ret_val[len] = 0;               // 0 terminate

    // expand all escape sequences in this text
    if (expand_escapes) {
        char *t = ret_val;
        ret_val = expandEscapes(t, f_html->htm_bad_html_warnings, f_html);
        delete [] t;
    }

    return (ret_val);
}


void
htmFormatManager::fillBullet()
{
    htmFont *font = f_html->htm_default_font;

    // x-offset for any marker and radius for a bullet or length of a
    // side for a square marker.
    unsigned int radius = font->width/2;

    if (f_element->marker == MARKER_DISC ||
            f_element->marker == MARKER_SQUARE ||
            f_element->marker == MARKER_CIRCLE) {
        // y-offset for this marker
        f_element->area.width = radius;
        f_element->area.height = f_html->htm_default_font->height;
    }
    else {
        // If we have a word, this is an ordered list for which the
        // index should be propageted.

        const char *prefix;
        if (f_element->words)
            prefix = f_element->words[0].word;
        else
            prefix = "";
        char number[64];
        switch (f_element->marker) {
        case MARKER_ALPHA_LOWER:
            sprintf(number, "%s%s.", prefix,
                ToAsciiLower(f_element->list_level));
            break;
        case MARKER_ALPHA_UPPER:
            sprintf(number, "%s%s.", prefix,
                ToAsciiUpper(f_element->list_level));
            break;
        case MARKER_ROMAN_LOWER:
            sprintf(number, "%s%s.", prefix,
                ToRomanLower(f_element->list_level));
            break;
        case MARKER_ROMAN_UPPER:
            sprintf(number, "%s%s.", prefix,
                ToRomanUpper(f_element->list_level));
            break;
        case MARKER_ARABIC:
        default:
            sprintf(number, "%s%i.", prefix, f_element->list_level);
            break;
        }
        f_element->text  = lstring::copy(number);
        f_element->len   = strlen(number);
        f_element->area.width = font->width + 
            f_html->htm_tk->tk_text_width(font, f_element->text,
            f_element->len);
        f_element->area.height = f_html->htm_default_font->height;
    }
}


// Check wether the requested newline is honored.
//
int
htmFormatManager::checkLineFeed(int nl, bool force)
{
    int ret_val = nl;

    if (force) {
        f_cx->prev_state = nl;
        return (ret_val);
    }

    // multiple soft and hard returns are never honored
    switch (nl) {
    case LF_DOWN_2:
        if (f_cx->prev_state == LF_DOWN_1) {
            ret_val = LF_DOWN_1;
            f_cx->prev_state = LF_DOWN_2;
            break;
        }
        if (f_cx->prev_state == LF_DOWN_2) {
            // unchanged
            ret_val = LF_NONE;
            break;
        }
        f_cx->prev_state = ret_val = nl;
        break;
    case LF_DOWN_1:
        if (f_cx->prev_state == LF_DOWN_1) {
            ret_val = LF_NONE;
            f_cx->prev_state = LF_DOWN_1;
            break;
        }
        if (f_cx->prev_state == LF_DOWN_2) {
            // unchanged
            ret_val = LF_NONE;
            break;
        }
        ret_val = f_cx->prev_state = nl;
        break;
    case LF_NONE:
        ret_val = f_cx->prev_state = nl;
        break;
    }
    return (ret_val);
}


//----- The dispatch functions --------------------------------------------

// HT_DOCTYPE, HT_BASE, HT_BODY, HT_FRAME, HT_FRAMESET, HT_HEAD,
// HT_HTML, HT_ISINDEX, HT_LINK, HT_META, HT_NOFRAMES, HT_TITLE
//
void
htmFormatManager::a_nop()
{
    f_ignore = true;
}


void
htmFormatManager::a_A()
{
    if (f_object->is_end) {
        // This is a very sneaky hack:  since empty named
        // anchors are allowed, we must store it somehow.  And
        // this is how we do it:  insert a dummy word (to
        // prevent margin reset) and back up one element.
        if (!f_anchor_data_used && (f_element_data & ELE_ANCHOR_INTERN)) {

            // insert a dummy word to prevent margin reset
            makeDummyWord();
            f_object_type = f_in_pre ? OBJ_PRE_TEXT : OBJ_TEXT;
            f_anchor_data_used = true;
            f_object = f_object->prev;
            return;
        }
        // unset anchor bitfields
        f_element_data &= ( ~ELE_ANCHOR & ~ELE_ANCHOR_TARGET &
            ~ELE_ANCHOR_VISITED & ~ELE_ANCHOR_INTERN);
        f_fg = f_cx->popFGColor();
        f_anchor_data = 0;
        f_ignore = true;  // only need anchor data
    }
    else {
        // allocate a new anchor
        f_anchor_data = f_html->newAnchor(f_object, this);
        if (!f_anchor_data) {
            f_ignore = true;
            return;
        }

        // save current color
        f_cx->pushFGColor(f_fg);

        f_new_anchors++;
        f_anchor_data_used = false;

        // maybe it's a named one
        if (f_anchor_data->name)
            f_element_data |= ELE_ANCHOR_INTERN;

        // Maybe it's also a plain anchor.  If so, see what
        // foreground color we have to use to render this
        // anchor.

        if (f_anchor_data->href[0] != '\0') {
            f_element_data |= ELE_ANCHOR;
            f_fg = f_html->htm_cm.cm_anchor_fg;

            // maybe it's been visited
            if (f_anchor_data->visited) {
                f_element_data |= ELE_ANCHOR_VISITED;
                f_fg = f_html->htm_cm.cm_anchor_visited_fg;
            }
            // maybe it's a target
            else if (f_anchor_data->target)
                f_element_data |= ELE_ANCHOR_TARGET;
        }
        f_ignore = true;  // only need anchor data
    }
}


void
htmFormatManager::a_ADDRESS()
{
    if (f_object->is_end) {
        if (f_html->htm_allow_color_switching)
            f_fg = f_cx->popFGColor();
        f_font = f_cx->popFont(&f_fontsize);
    }
    else {
        if (f_html->htm_allow_color_switching) {
            f_cx->pushFGColor(f_fg);
            char *chPtr = htmTagGetValue(f_object->attributes, "color");
            if (chPtr) {
                f_fg = f_html->htm_cm.getPixelByName(chPtr, f_fg);
                delete [] chPtr;
            }
        }
        f_cx->pushFont(f_font, f_fontsize);
        f_font = f_html->loadFont(f_object->id, f_fontsize, f_font);
    }
    f_linefeed = checkLineFeed(LF_DOWN_1, false);
    f_object_type = OBJ_BLOCK;
}


void
htmFormatManager::a_APPLET()
{
    if (f_object->is_end) {
        // INSERT CODE
        // to end this applet
    }
    else {
        if (f_html->htm_bad_html_warnings)
            f_html->warning("a_APPLET",
                "<APPLET> element not supported yet.");
        // INSERT CODE
        // to start this applet
    }
    f_object_type = OBJ_APPLET;
    f_ignore = true;
}


void
htmFormatManager::a_AREA()
{
    if (f_imageMap)
        f_html->htm_im.addAreaToMap(f_imageMap, f_object, this);
    else if (f_html->htm_bad_html_warnings)
        f_html->warning("a_AREA", "<AREA> "
            "element outside <MAP>, ignored (line %i in input).",
            f_object->line);
    f_ignore = true;  // only need area data
}


// HT_B, HT_CITE, HT_CODE, HT_DFN, HT_EM, HT_I, HT_KBD, HT_SAMP,
// HT_STRONG, HT_TT, HT_VAR
//
void
htmFormatManager::a_B_etc()
{
    if (f_object->is_end) {
        if (f_html->htm_allow_color_switching)
            f_fg = f_cx->popFGColor();
        f_font = f_cx->popFont(&f_fontsize);
    }
    else {
        if (f_html->htm_allow_color_switching) {
            f_cx->pushFGColor(f_fg);
            char *chPtr = htmTagGetValue(f_object->attributes, "color");
            if (chPtr) {
                f_fg = f_html->htm_cm.getPixelByName(chPtr, f_fg);
                delete [] chPtr;
            }
        }
        f_cx->pushFont(f_font, f_fontsize);
        f_font = f_html->loadFont(f_object->id, f_fontsize, f_font);
    }
    f_ignore = true;  // only need font data
}


void
htmFormatManager::a_BASEFONT()
{
    f_basefontsize = htmTagGetNumber(f_object->attributes, "size", 0);
    // take absolute value
    f_basefontsize = abs(f_basefontsize);
    if (f_basefontsize < 1 || f_basefontsize > 7) {
        if (f_html->htm_bad_html_warnings)
            f_html->warning("a_BASEFONT",
                "Invalid fontsize size %i at line %i of "
                "input.", f_basefontsize, f_object->line);
        f_basefontsize = 3;
    }
    f_fontsize = f_basefontsize;
    f_ignore = true;  // only need font data
}


void
htmFormatManager::a_BIG()
{
    if (f_object->is_end)
        f_font = f_cx->popFont(&f_fontsize);
    else {
        // multiple big elements are not honoured
        f_cx->pushFont(f_font, f_fontsize);
        if (f_fontsize < 7)
            f_fontsize++;
        f_font = f_html->loadFont(HT_FONT, f_fontsize, f_font);
    }
    f_ignore = true;  // only need font data
}


void
htmFormatManager::a_BLOCKQUOTE()
{
    if (f_object->is_end) {
        f_ident_level--;
        if (f_html->htm_allow_color_switching)
            f_fg = f_cx->popFGColor();
    }
    else {
        f_ident_level++;
        if (f_html->htm_allow_color_switching) {
            f_cx->pushFGColor(f_fg);
            char *chPtr = htmTagGetValue(f_object->attributes, "color");
            if (chPtr) {
                f_fg = f_html->htm_cm.getPixelByName(chPtr, f_fg);
                delete [] chPtr;
            }
        }
    }
    f_linefeed = checkLineFeed(LF_DOWN_2, false);
    f_object_type = OBJ_BLOCK;
}


void
htmFormatManager::a_BR()
{
    char *chPtr = htmTagGetValue(f_object->attributes, "clear");
    if (chPtr) {
        if (tolower(chPtr[0]) != 'n') {
            f_cx->pushAlignment(f_halign);
            if (tolower(chPtr[0]) == 'l')         // clear = left
                f_halign = CLEAR_LEFT;
            else if (tolower(chPtr[0]) == 'r')    // clear = right
                f_halign = CLEAR_RIGHT;
            else if (tolower(chPtr[0]) == 'a')    // clear = all
                f_halign = CLEAR_ALL;
        }
        delete [] chPtr;
    }
    f_linefeed = checkLineFeed(LF_DOWN_1, true);
    f_object_type = OBJ_BLOCK;
}


void
htmFormatManager::a_CAPTION()
{
    if (f_object->is_end) {
        // close the caption
        f_table->closeCaption(f_html, f_element);

        f_halign = f_cx->popAlignment();
        f_bg = f_cx->popBGColor();
        f_bg_image = f_cx->popBGImage();
        f_object_type = OBJ_NONE;
    }
    else {
        f_cx->pushAlignment(f_halign);
        f_cx->pushBGColor(f_bg);
        f_cx->pushBGImage(f_bg_image);

        // captions are always centered
        f_halign = HALIGN_CENTER;

        // open the caption
        f_table->openCaption(f_html, f_element, f_object, &f_bg, &f_bg_image);

        f_object_type = OBJ_TABLE_FRAME;
    }
}


void
htmFormatManager::a_CENTER()
{
    if (f_object->is_end) {
        f_halign = f_cx->popAlignment();
        // do we have a color attrib?
        if (f_html->htm_allow_color_switching)
            f_fg = f_cx->popFGColor();
    }
    else {
        f_cx->pushAlignment(f_halign);
        f_halign = HALIGN_CENTER;
        // do we have a color attrib?
        if (f_html->htm_allow_color_switching) {
            f_cx->pushFGColor(f_fg);
            char *chPtr = htmTagGetValue(f_object->attributes, "color");
            if (chPtr) {
                f_fg = f_html->htm_cm.getPixelByName(chPtr, f_fg);
                delete [] chPtr;
            }
        }
    }
    f_linefeed = checkLineFeed(LF_DOWN_1, false);
    f_object_type = OBJ_BLOCK;
}


void
htmFormatManager::a_DD()
{
    f_object_type = OBJ_BLOCK;
    f_linefeed = checkLineFeed(LF_DOWN_1, false);
}


void
htmFormatManager::a_DIR_MENU_UL()
{
    if (f_object->is_end) {

        f_ul_level--;
        f_ident_level--;
        if (f_ident_level < 0) {
            if (f_html->htm_bad_html_warnings)
                f_html->warning("a_DIR_MENU_UL",
                    "Negative indentation at line %i of input. "
                    "Check your document.", f_object->line);
            f_ident_level = 0;
        }
        f_current_list = (f_ident_level ? f_ident_level - 1 : 0);
    }
    else {
        // avoid overflow of mark id array
        int mark_id = f_ul_level % UL_ARRAYSIZE;

        // set default marker & list start
        f_list_stack[f_ident_level].marker = ul_markers[mark_id].type;
        f_list_stack[f_ident_level].level = 0;
        f_list_stack[f_ident_level].type = f_object->id;

        if (f_ident_level == MAX_NESTED_LISTS) {
            f_html->warning("a_DIR_MENU_UL", "Exceeding"
                " maximum nested list depth for nested lists (%i) "
                "at line %i of input.", MAX_NESTED_LISTS,
                f_object->line);
            f_ident_level = MAX_NESTED_LISTS-1;
        }
        f_current_list = f_ident_level;

        if (f_object->id == HT_UL) {
            // check if user specified a custom marker
            char *chPtr = htmTagGetValue(f_object->attributes, "type");
            if (chPtr) {
                // Walk thru the list of possible markers.  If
                // a match is found, store it so we can switch
                // back to the correct marker once this list
                // terminates.
                for (int i = 0 ; i < UL_ARRAYSIZE; i++) {
                    if (!(strcasecmp(ul_markers[i].name, chPtr))) {
                        f_list_stack[f_ident_level].marker =
                            ul_markers[i].type;
                        break;
                    }
                }
                delete [] chPtr;
            }
        }
        f_ul_level++;
        f_ident_level++;
    }
    f_linefeed = checkLineFeed(LF_DOWN_2, false);
    f_object_type = OBJ_BLOCK;
}


void
htmFormatManager::a_DIV()
{
    if (f_object->is_end) {
        f_halign = f_cx->popAlignment();
        // do we have a color attrib?
        if (f_html->htm_allow_color_switching)
            f_fg = f_cx->popFGColor();
    }
    else {
        f_cx->pushAlignment(f_halign);
        f_halign = htmGetHorizontalAlignment(f_object->attributes, f_halign);
        // do we have a color attrib?
        if (f_html->htm_allow_color_switching) {
            f_cx->pushFGColor(f_fg);
            char *chPtr = htmTagGetValue(f_object->attributes, "color");
            if (chPtr) {
                f_fg = f_html->htm_cm.getPixelByName(chPtr, f_fg);
                delete [] chPtr;
            }
        }
    }
    f_linefeed = checkLineFeed(LF_DOWN_1, false);
    f_object_type = OBJ_BLOCK;
}


void
htmFormatManager::a_DL()
{
    if (f_object->is_end)
        f_ident_level--;
    else
        f_ident_level++;
    f_linefeed = checkLineFeed(LF_DOWN_2, false);
    f_object_type = OBJ_BLOCK;
}


void
htmFormatManager::a_DT()
{
    if (f_object->is_end)
        f_in_dt--;
    else
        f_in_dt++;
    f_object_type = OBJ_BLOCK;
    f_linefeed = checkLineFeed(LF_DOWN_1, false);
}


// <font> is a bit performance hit.  We always need to push & pop the
// font *even* if only the font color has been changed as we can't
// keep track of what has actually been changed.
//
void
htmFormatManager::a_FONT()
{
    if (f_object->is_end) {
        if (f_html->htm_allow_font_switching)
            f_font = f_cx->popFont(&f_fontsize);
        if (f_html->htm_allow_color_switching)
            f_fg = f_cx->popFGColor();
    }
    else {
        if (f_html->htm_allow_color_switching) {
            f_cx->pushFGColor(f_fg);
            char *chPtr = htmTagGetValue(f_object->attributes, "color");
            if (chPtr) {
                f_fg = f_html->htm_cm.getPixelByName(chPtr, f_fg);
                delete [] chPtr;
            }
        }

        if (f_html->htm_allow_font_switching)
            f_cx->pushFont(f_font, f_fontsize);
        else
            return;;

        // can't use TagGetNumber: fontchange can be relative
        char *chPtr = htmTagGetValue(f_object->attributes, "size");
        if (chPtr) {
            int fsz = atoi(chPtr);

            // check wether size is relative or not
            if (chPtr[0] == '-' || chPtr[0] == '+')
                fsz = f_basefontsize + fsz;
            // sanity check
            if (fsz < 1 || fsz > 7) {
                if (fsz < 1)
                    fsz = 1;
                else
                    fsz = 7;
            }
            f_fontsize = fsz;
            delete [] chPtr;  // fix 01/28/98-02, kdh
            chPtr = 0;
        }
        // Font face changes only allowed when not in
        // preformatted text.  Only check when not being
        // pedantic.
#ifndef PEDANTIC
        chPtr = 0;
        if (!f_in_pre)
#endif
            chPtr = htmTagGetValue(f_object->attributes, "face");

        if (chPtr) {
#ifdef PEDANTIC
            if (f_in_pre) {
                warning("a_FONT",
                    "<FONT FACE=\"%s\"> not allowed inside <PRE>,"
                    " ignored.\n    (line %i in input)", chPtr,
                    f_object->line);
                // Ignore face but must allow for size change.
                // (Font stack will get unbalanced otherwise!)
                f_font = f_html->loadFont(HT_FONT, f_fontsize, f_font);
            }
            else
#endif
                f_font = f_html->loadFontWithFace(f_fontsize, chPtr, f_font);

            delete [] chPtr;
        }
        else
            f_font = f_html->loadFont(HT_FONT, f_fontsize, f_font);
    }
    f_ignore = true;  // only need font data
}


void
htmFormatManager::a_FORM()
{
    if (f_object->is_end)
        f_html->endForm();
    else
        f_html->startForm(f_object->attributes);

    // only need form data
    f_ignore = true;
}


void
htmFormatManager::a_H1_6()
{
    if (f_object->is_end) {
        if (f_html->htm_allow_color_switching)
            f_fg = f_cx->popFGColor();
        f_halign = f_cx->popAlignment();
        f_font = f_cx->popFont(&f_fontsize);
    }
    else {
        if (f_html->htm_allow_color_switching) {
            f_cx->pushFGColor(f_fg);
            char *chPtr = htmTagGetValue(f_object->attributes, "color");
            if (chPtr) {
                f_fg = f_html->htm_cm.getPixelByName(chPtr, f_fg);
                delete [] chPtr;
            }
        }

        f_cx->pushAlignment(f_halign);
        f_halign = htmGetHorizontalAlignment(f_object->attributes, f_halign);
        f_cx->pushFont(f_font, f_fontsize);
        f_font = f_html->loadFont(f_object->id, f_fontsize, f_font);

        // Need to update basefont size as well so font face
        // changes inside these elements use the correct font
        // size as well.  The sizes used by the headers are in
        // reverse order.
        f_fontsize = (int)(HT_H6 - f_object->id) + 2;
    }
    f_linefeed = checkLineFeed(LF_DOWN_2, false);
    f_object_type = OBJ_BLOCK;
}


void
htmFormatManager::a_HR()
{
    // Horizontal rules don't have an ending counterpart,
    // so the alignment is never pushed.  If we should do
    // that, we would get an unbalanced stack.
    f_element->halign = htmGetHorizontalAlignment(f_object->attributes,
        f_halign);
    // see if we have a width spec
    f_element->len = htmTagCheckNumber(f_object->attributes, "width", 0);

    // check height
    f_height = htmTagGetNumber(f_object->attributes, "size", 0);
    // sanity check
    if (f_height < 2 )
        f_height = 2;
    // y_offset is used as a flag for the NOSHADE attr
    f_element->y_offset =
        (int)htmTagCheck(f_object->attributes, "noshade");

    // do we have a color attrib?
    if (f_html->htm_allow_color_switching) {
        f_cx->pushFGColor(f_fg);
        char *chPtr = htmTagGetValue(f_object->attributes, "color");
        if (chPtr) {
            f_fg = f_html->htm_cm.getPixelByName(chPtr, f_fg);
            delete [] chPtr;
        }
    }

    // horizontal rules always have a soft return
    f_linefeed = checkLineFeed(LF_DOWN_1, false);
    f_object_type = OBJ_HRULE;
}


void
htmFormatManager::a_IMG()
{
    if (!imageToWord(f_object->attributes)) {
        f_ignore = true;
        return;
    }
    f_text_data |= TEXT_IMAGE;
    f_object_type = f_in_pre ? OBJ_PRE_TEXT : OBJ_TEXT;
    // no explicit returns for images, reset
    f_linefeed = checkLineFeed(LF_NONE, false);
}


void
htmFormatManager::a_INPUT()
{
    if (!inputToWord(f_object->attributes)) {
        f_ignore = true;
        return;
    }
    // type=image is promoted to a true image
    if (f_words->form->type == FORM_IMAGE) {
        f_text_data |= TEXT_IMAGE;

        // allocate a new anchor
        if ((f_form_anchor_data = f_html->newAnchor(f_object, this)) == 0)
            return;

        // promote to internal form anchor
        f_form_anchor_data->url_type = ANCHOR_FORM_IMAGE;

        f_new_anchors++;

        // set proper element bits, we assume it's a plain one
        f_element_data |= ELE_ANCHOR;
    }
    else
        f_text_data |= TEXT_FORM;
    f_object_type = f_in_pre ? OBJ_PRE_TEXT : OBJ_TEXT;
}


void
htmFormatManager::a_LI()
{
    if (f_object->is_end)    // optional termination
        f_object_type = OBJ_BLOCK;
    else {
        // increase list counter
        f_list_stack[f_current_list].level++;

        // check if user specified a custom marker
        char *chPtr = htmTagGetValue(f_object->attributes, "type");
        if (chPtr) {
            // Depending on current list type, check and set
            // the marker.
            if (f_list_stack[f_current_list].type == HT_OL) {
                for (int i = 0 ; i < OL_ARRAYSIZE; i++) {
                    if (!(strcmp(ol_markers[i].name, chPtr))) {
                        f_list_stack[f_current_list].marker =
                            ol_markers[i].type;
                        break;
                    }
                }
            }
            else if (f_list_stack[f_current_list].type == HT_UL) {
                for (int i = 0 ; i < UL_ARRAYSIZE; i++) {
                    if (!(strcmp(ul_markers[i].name, chPtr))) {
                        f_list_stack[f_current_list].marker =
                            ul_markers[i].type;
                        break;
                    }
                }
            }
            delete [] chPtr;
        }
        // check if user specified a custom number for ol lists
        if (f_list_stack[f_current_list].type == HT_OL &&
                htmTagCheck(f_object->attributes, "value")) {
            f_list_stack[f_current_list].level =
                htmTagGetNumber(f_object->attributes, "value", 0);
        }

        // If the current list is an index, create a prefix
        // for the current item.

        if (f_list_stack[f_current_list].isindex)
            indexToWord();
        f_object_type = OBJ_BULLET;
    }
    f_linefeed = checkLineFeed(LF_DOWN_1, false);
}


void
htmFormatManager::a_MAP()
{
    if (f_object->is_end) {
        f_html->htm_im.storeImagemap(f_imageMap);
        f_imageMap = 0;
    }
    else {
        char *chPtr = htmTagGetValue(f_object->attributes, "name");
        if (chPtr) {
            f_imageMap = new htmImageMap(chPtr);
            delete [] chPtr;
        }
        else if (f_html->htm_bad_html_warnings)
            f_html->warning("a_MAP", "Unnamed "
                "map, ignored (line %i in input).", f_object->line);
    }
    f_ignore = true;  // only need imagemap name
}


void
htmFormatManager::a_OL()
{
    if (f_object->is_end) {
        // Must be reset properly, only possible for <ol> lists.
        f_list_stack[f_current_list].isindex = false;

        f_ol_level--;
        f_ident_level--;
        if (f_ident_level < 0) {
            if (f_html->htm_bad_html_warnings)
                f_html->warning("a_OL",
                    "Negative indentation at line %i of input. "
                    "Check your document.", f_object->line);
            f_ident_level = 0;
        }
        f_current_list = (f_ident_level ? f_ident_level - 1 : 0);
    }
    else {
        // avoid overflow of mark id array
        int mark_id = f_ol_level % OL_ARRAYSIZE;

        // set default marker & list start
        f_list_stack[f_ident_level].marker = ol_markers[mark_id].type;
        f_list_stack[f_ident_level].level = 0;
        f_list_stack[f_ident_level].type = f_object->id;

        if (f_ident_level == MAX_NESTED_LISTS) {
            f_html->warning("a_OL", "Exceeding"
                " maximum nested list depth for nested lists (%i) "
                "at line %i of input.", MAX_NESTED_LISTS,
                f_object->line);
            f_ident_level = MAX_NESTED_LISTS-1;
        }
        f_current_list = f_ident_level;

        // check if user specified a custom marker
        char *chPtr = htmTagGetValue(f_object->attributes, "type");
        if (chPtr != 0) {
            // Walk thru the list of possible markers.  If a
            // match is found, store it so we can switch back
            // to the correct marker once this list
            // terminates.
            for (int i = 0 ; i < OL_ARRAYSIZE; i++) {
                if (!(strcmp(ol_markers[i].name, chPtr))) {
                    f_list_stack[f_ident_level].marker =
                        ol_markers[i].type;
                    break;
                }
            }
            delete [] chPtr;
        }

        // see if a start tag exists
        if (htmTagCheck(f_object->attributes, "start")) {
            // pick up a start spec
            f_list_stack[f_ident_level].level =
                htmTagGetNumber(f_object->attributes, "start", 0);
            f_list_stack[f_ident_level].level--;
        }

        // see if we have to propage the current index number
        f_list_stack[f_ident_level].isindex =
            htmTagCheck(f_object->attributes, "isindex");
        f_ol_level++;
        f_ident_level++;
    }
    f_linefeed = checkLineFeed(LF_DOWN_2, false);
    f_object_type = OBJ_BLOCK;
}


// It's an error if we get this, SelectToWord deals with these tags.
//
void
htmFormatManager::a_OPTION()
{
    if (!f_object->is_end) {
        if (f_html->htm_bad_html_warnings)
            f_html->warning("a_OPTION",
                "Bad <OPTION> tag: outside a <SELECT> tag, "
                "ignoring (line %i in input).", f_object->line);
    }
    f_ignore = true;
}


void
htmFormatManager::a_P()
{
    if (f_object->is_end) {
        f_halign = f_cx->popAlignment();
        // do we have a color attrib?
        if (f_html->htm_allow_color_switching)
            f_fg = f_cx->popFGColor();
    }
    else {
        f_cx->pushAlignment(f_halign);
        f_halign = htmGetHorizontalAlignment(f_object->attributes, f_halign);
        // do we have a color attrib?
        if (f_html->htm_allow_color_switching) {
            f_cx->pushFGColor(f_fg);
            char *chPtr = htmTagGetValue(f_object->attributes, "color");
            if (chPtr) {
                f_fg = f_html->htm_cm.getPixelByName(chPtr, f_fg);
                delete [] chPtr;
            }
        }
    }
    f_linefeed = checkLineFeed(LF_DOWN_2, false);
    f_object_type = OBJ_BLOCK;
}


void
htmFormatManager::a_PARAM()
{
    if (f_html->htm_bad_html_warnings)
        f_html->warning("a_PARAM",
            "<PARAM> element not supported yet.");
    f_object_type = OBJ_APPLET;
    f_ignore = true;
}


void
htmFormatManager::a_PRE()
{
    if (f_object->is_end) {
        if (f_html->htm_allow_color_switching)
            f_fg = f_cx->popFGColor();
        f_font = f_cx->popFont(&f_fontsize);
        f_in_pre = false;
    }
    else {
        if (f_html->htm_allow_color_switching) {
            f_cx->pushFGColor(f_fg);
            char *chPtr = htmTagGetValue(f_object->attributes, "color");
            if (chPtr) {
                f_fg = f_html->htm_cm.getPixelByName(chPtr, f_fg);
                delete [] chPtr;
            }
        }
        f_cx->pushFont(f_font, f_fontsize);
        f_font = f_html->loadFont(f_object->id, f_fontsize, f_font);
        f_in_pre = true;
        f_pre_nl = true;
    }
    f_linefeed = checkLineFeed(LF_DOWN_1, false);
    f_object_type = OBJ_BLOCK;
}


// HT_SCRIPT and HT_STYLE
// According to HTML3.2, the following elements may not occur inside
// the body content, but a *lot* of HTML documents are in direct
// violation with this and the parser isn't always successfully in
// removing them.  So we need to handle these elements as well and
// skip all data between the opening and closing element.
//
void
htmFormatManager::a_SCRIPT()
{
    htmlEnum end_id = f_object->id;
    // move past element
    for (f_object = f_object->next; f_object; f_object = f_object->next) {
        if (f_object->id == end_id && f_object->is_end)
            break;
    }
    f_ignore = true;
}


void
htmFormatManager::a_SELECT()
{
    // this form component can only contain option tags
    if (!selectToWord(f_object)) {
        f_ignore = true;
        return;
    }
    // walk to the end of this select
    for (f_object = f_object->next; f_object && f_object->id != HT_SELECT;
        f_object = f_object->next) ;

    f_text_data |= TEXT_FORM;
    f_object_type = f_in_pre ? OBJ_PRE_TEXT : OBJ_TEXT;
}


void
htmFormatManager::a_SMALL()
{
    if (f_object->is_end)
        f_font = f_cx->popFont(&f_fontsize);
    else {
        // multiple small elements are not honoured
        f_cx->pushFont(f_font, f_fontsize);
        if (f_fontsize > 1)
            f_fontsize--;
        f_font = f_html->loadFont(HT_FONT, f_fontsize, f_font);
    }
    f_ignore = true;  // only need font data
}


void
htmFormatManager::a_STRIKE()
{
    if (f_object->is_end)  // unset strikeout bitfields
        f_element_data &= (~ELE_STRIKEOUT & ~ELE_STRIKEOUT_TEXT);
    else
        f_element_data |= ELE_STRIKEOUT;
    f_ignore = true;  // only need strikeout data
}


void
htmFormatManager::a_STYLE()
{
    htmlEnum end_id = f_object->id;
    // move past element
    for (f_object = f_object->next; f_object; f_object = f_object->next) {
        if (f_object->id == end_id && f_object->is_end)
            break;
    }
    f_ignore = true;
}


void
htmFormatManager::a_SUB_SUP()
{
    if (f_object->is_end) {
        // restore vertical offset
        f_y_offset = 0;
        f_x_offset = 0;
        f_font = f_cx->popFont(&f_fontsize);
    }
    else {
        // multiple small elements are not honoured
        int asc = f_font->ascent;
        f_cx->pushFont(f_font, f_fontsize);
        if (f_fontsize > 1)
            f_fontsize--;
        if (f_fontsize > 3)
            f_fontsize--;
        f_font = f_html->loadFont(HT_FONT, f_fontsize, f_font);
        f_y_offset = (f_object->id == HT_SUB ?
            f_font->sub_yoffset : (int)(-0.4*asc));
        f_x_offset = (f_object->id == HT_SUB ?
            f_font->sub_xoffset : 2);
    }
    f_ignore = true;  // only need font data
}


void
htmFormatManager::a_TAB()
{
    f_object_type = OBJ_TEXT;
    f_element->len = 8;  // default tabsize

    // see if we have a width spec
    char *chPtr = htmTagGetValue(f_object->attributes, "size");
    if (chPtr) {
        f_element->len = atoi(chPtr);
        delete [] chPtr;
    }
    setTab(f_element->len);
}


void
htmFormatManager::a_TABLE()
{
    if (f_object->is_end) {
        // wrapup current table
        f_table = f_table->close(f_html, f_element, &f_table_cell);

        f_halign = f_cx->popAlignment();
        f_bg = f_cx->popBGColor();
        f_bg_image = f_cx->popBGImage();
        f_ident_level = f_cx->popIdent();
        f_object_type = OBJ_NONE;
    }
    else {
        // table start always causes a linebreak
        f_linefeed = checkLineFeed(LF_DOWN_1, false);

        f_cx->pushAlignment(f_halign);
        f_cx->pushBGColor(f_bg);
        f_cx->pushBGImage(f_bg_image);
        f_cx->pushIdent(f_ident_level);

        // Open a new table. Returns a parent or a child table.
        f_table = f_table->open(f_html, f_element, f_object, &f_halign, &f_bg,
            &f_bg_image, &f_table_cell);

        // Want to reset indentation level for table contents,
        // but keep the indentation level for tables in
        // table->ident

        f_table->t_ident = f_ident_level;
        f_ident_level = 0;

        // new master table, insert
        if (f_table->t_parent == 0) {
            // insert this table in the list of tables
            if (f_html->htm_tables) {
                f_current_table->t_next = f_table;
                f_current_table = f_table;
            }
            else {
                f_html->htm_tables = f_table;
                f_current_table = f_table;
            }
        }
        f_object_type = OBJ_TABLE;
    }
}


void
htmFormatManager::a_TD_TH()
{
    if (f_object->is_end) {
        // optional termination
        // header cell used a bold font, restore
        if (f_object->id == HT_TH)
            f_font = f_cx->popFont(&f_fontsize);

        // close current cell
        f_table->closeCell(f_html, f_element);
        f_halign = f_cx->popAlignment();
        f_bg = f_cx->popBGColor();
        f_bg_image = f_cx->popBGImage();
        f_object_type = OBJ_NONE;
    }
    else {
        // header cell uses a bold font
        if (f_object->id == HT_TH) {
            f_cx->pushFont(f_font, f_fontsize);
            f_font = f_html->loadFont(HT_B, f_fontsize, f_font);
        }

        f_cx->pushAlignment(f_halign);
        f_cx->pushBGColor(f_bg);
        f_cx->pushBGImage(f_bg_image);

        // open a cell
        f_table_cell = f_table->openCell(f_html, f_element, f_object,
            &f_halign, &f_bg, &f_bg_image);

        // table cell always resets linefeeding
        checkLineFeed(LF_DOWN_1, true);

        f_object_type = OBJ_TABLE_FRAME;
    }
}


void
htmFormatManager::a_TEXTAREA()
{
    if (!textAreaToWord(f_object)) {
        f_ignore = true;
        return;
    }
    // Walk to the end of this textarea.  If there was any
    // text provided, we've already picked it up.
    for (f_object = f_object->next; f_object && f_object->id != HT_TEXTAREA;
        f_object = f_object->next) ;

    f_text_data |= TEXT_FORM;
    f_object_type = f_in_pre ? OBJ_PRE_TEXT : OBJ_TEXT;
}


void
htmFormatManager::a_TR()
{
    if (f_object->is_end) {
        // optional termination
        // close current row
        f_table->closeRow(f_html, f_element);
        f_halign = f_cx->popAlignment();
        f_bg = f_cx->popBGColor();
        f_bg_image = f_cx->popBGImage();
        f_object_type = OBJ_NONE;
    }
    else {
        // open a row
        f_cx->pushAlignment(f_halign);
        f_cx->pushBGColor(f_bg);
        f_cx->pushBGImage(f_bg_image);
        f_table->openRow(f_html, f_element, f_object, &f_halign, &f_bg,
            &f_bg_image);
        f_object_type = OBJ_TABLE_FRAME;
    }
}


void
htmFormatManager::a_U()
{
    if (f_object->is_end)  // unset underline bitfields
        f_element_data &= (~ELE_UNDERLINE & ~ELE_UNDERLINE_TEXT);
    else
        f_element_data |= ELE_UNDERLINE;
    f_ignore = true;  // only need underline data
}


void
htmFormatManager::a_ZTEXT()
{
    f_object_type = OBJ_TEXT;
    // text_data gets completely reset in copyText.  We do not
    // want escape expansion if we are loading a plain text
    // document.
    // Unless <pre> is on the same line as the leading
    // text, there is an extra newline - zap it.
    char *ttmp = f_object->element;
    if (f_in_pre && f_pre_nl) {
        f_pre_nl = false;
        if (ttmp && *ttmp == '\n')
            ttmp++;
    }
    if ((f_text = copyText(ttmp, f_in_pre, &f_text_data,
            f_html->htm_mime_id != MIME_PLAIN)) == 0) {
        f_ignore = true;  // ignore empty text fields
        return;
    }
    if (!f_in_pre) {
        CollapseWhiteSpace(f_text);

        // If this turns out to be an empty block, ignore it,
        // but only if it's not an empty named anchor.

        if (strlen(f_text) == 0) {
            f_ignore = true;
            delete [] f_text;
            f_text = 0;
            return;
        }
        // check line data
        if (f_element_data & ELE_ANCHOR) {
            if (f_element_data & ELE_ANCHOR_TARGET)
                f_line_data = ul_type(f_html->htm_anchor_target_style);
            else if (f_element_data & ELE_ANCHOR_VISITED)
                f_line_data = ul_type(f_html->htm_anchor_visited_style);
            else
                f_line_data = ul_type(f_html->htm_anchor_style);
        }
        else {
            if (f_element_data & ELE_UNDERLINE)
                f_line_data  = LINE_UNDER;
        }
        if (f_element_data & ELE_STRIKEOUT)
            f_line_data |= LINE_STRIKE;
        // convert text to a number of words
        textToWords();
    }
    else {
        f_object_type = OBJ_PRE_TEXT;
        // check line data
        if (f_element_data & ELE_ANCHOR) {
            if (f_element_data & ELE_ANCHOR_TARGET)
                f_line_data = ul_type(f_html->htm_anchor_target_style);
            else if (f_element_data & ELE_ANCHOR_VISITED)
                f_line_data = ul_type(f_html->htm_anchor_visited_style);
            else
                f_line_data = ul_type(f_html->htm_anchor_style);
        }
        else {
            if (f_element_data & ELE_UNDERLINE)
                f_line_data  = LINE_UNDER;
        }
        if (f_element_data & ELE_STRIKEOUT)
            f_line_data |= LINE_STRIKE;
        // convert text to a number of words, keep formatting
        textToPre();
    }
    f_linefeed = checkLineFeed(LF_NONE, false);
}



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
#include "htm_callback.h"
#include "htm_parser.h"
#include "htm_format.h"
#include "htm_string.h"
#include "htm_form.h"
#include "htm_frame.h"
#include "htm_table.h"
#include "htm_tag.h"
#include <ctype.h>
#include <string.h>


namespace {
    htmLinkDataRec *parse_links(htmObject*, int*);
    htmMetaDataRec *parse_meta(htmObject*, int*);
}

htmAnchorCallbackStruct::htmAnchorCallbackStruct(htmAnchor *a)
{
    reason      = HTM_ANCHORTRACK;
    event       = 0;
    url_type    = a ? a->url_type : ANCHOR_UNKNOWN;
    line        = a ? a->line : 0;
    href        = a ? a->href : 0;
    target      = a ? a->target : 0;
    rel         = a ? a->rel : 0;
    rev         = a ? a->rev : 0;
    title       = a ? a->target : 0;
    is_frame    = false;
    doit        = false;
    visited     = a ? a->visited : false;
}

namespace htm
{
    // getHeadAttributes mask bits
    enum
    {
        HeadClear       = 0x0,              // clear everything
        HeadDocType     = 0x1,              // fill doctype member
        HeadTitle       = 0x2,              // fill title member
        HeadIsIndex     = 0x4,              // fill isIndex member
        HeadBase        = 0x8,              // fill Base member
        HeadMeta        = 0x10,             // fill meta members
        HeadLink        = 0x20,             // fill link members
        HeadScript      = 0x40,             // fill script members
        HeadStyle       = 0x80,             // fill Style members
        HeadAll         = 0xff              // fill all members
    };
}

htmHeadAttributes::htmHeadAttributes()
{
    doctype         = 0;
    title           = 0;
    is_index        = 0;
    base            = 0;
    num_meta        = 0;
    meta            = 0;
    num_link        = 0;
    link            = 0;
    style_type      = 0;
    style           = 0;
    script_lang     = 0;
    script          = 0;
}


// Free the requested members of the given HeadAttributes.
//
void
htmHeadAttributes::freeHeadAttributes(unsigned char mask_bits)
{
    if (mask_bits & HeadDocType) {
        delete [] doctype;
        doctype = 0;
    }
    if (mask_bits & HeadTitle) {
        delete [] title;
        title = 0;
    }
    if (mask_bits & HeadBase) {
        delete [] base;
        base = 0;
    }
    if (mask_bits & HeadScript) {
        delete [] script;
        script = 0;
        delete [] script_lang;
        script_lang = 0;
    }
    if (mask_bits & HeadStyle) {
        delete [] style_type;
        style_type = 0;
        delete [] style;
        style = 0;
    }
    if (mask_bits & HeadMeta) {
        delete [] meta;
        meta = 0;
        num_meta = 0;
    }
    if (mask_bits & HeadLink) {
        delete [] link;
        link = 0;
        num_link = 0;
    }
}


// fill the given HeadAttributes with the requested document head
// elements.  Returns true when a <head></head> block is present,
// false if not.
//
bool
htmWidget::getHeadAttributes(htmHeadAttributes *head, unsigned char mask_bits)
{
    if (!head) {
        warning("getHeadAttributes", "NULL structure passed!");
        return (false);
    }

    // Don't bother to check a thing when we only have to clear all
    // attributes.

    if (mask_bits == HeadClear) {
        head->freeHeadAttributes(HeadAll);
        return (false);
    }

    // free requested members
    head->freeHeadAttributes(mask_bits);

    // empty document
    if (htm_elements == 0)
        return (false);

    // walk until we reach HT_HEAD or HT_BODY, whatever comes first
    htmObject *tmp;
    for (tmp = htm_elements; tmp && tmp->id != HT_HEAD
            && tmp->id != HT_BODY; tmp = tmp->next) {
        // pick up doctype if we happen to see it
        if (tmp->id == HT_DOCTYPE && tmp->attributes &&
                ((mask_bits & HeadDocType) || mask_bits == HeadAll))
            head->doctype = lstring::copy(tmp->attributes);
    }

    // no head found
    if (tmp == 0 || tmp->id == HT_BODY)
        return (false);

    // we have found the head
    tmp = tmp->next;    // move past it

    // Go and fill all members
    htmObject *link_start = 0, *meta_start = 0;
    int num_link = 0, num_meta = 0;
    for ( ; tmp != 0 && tmp->id != HT_HEAD && tmp->id != HT_BODY;
            tmp = tmp->next) {
        switch (tmp->id) {
        case HT_LINK:
            num_link++;
            link_start = (num_link == 1 ? tmp : link_start);
            break;
        case HT_META:
            num_meta++;
            meta_start = (num_meta == 1 ? tmp : meta_start);
            break;
        case HT_ISINDEX:
            if ((mask_bits & HeadIsIndex) || mask_bits == HeadAll)
                head->is_index = true;
            break;
        case HT_TITLE:
            if (((mask_bits & HeadTitle) || mask_bits == HeadAll)
                    && !tmp->is_end) {

                // pick up the text, its all in a single element
                tmp = tmp->next;

                // sanity check
                if (!tmp->element)
                    break;

                // skip leading...
                char *start;
                for (start = tmp->element; *start && isspace(*start);
                    start++) ;

                // sanity
                if (*start == 0)
                    break;

                // ...and trailing whitespace
                char *end;
                for (end = &start[strlen(start)-1]; *end && isspace(*end);
                    end--) ;

                // always backs up one too many
                end++;

                // sanity
                if (end - start <= 0)
                    break;

                // duplicate the title and expand escape sequences
                char *t = htm_strndup(start, end - start);
                head->title = expandEscapes(t, htm_bad_html_warnings, this);
                delete [] t;
            }
            break;
        case HT_BASE:
            if ((mask_bits & HeadBase) || mask_bits == HeadAll)
                head->base = htmTagGetValue(tmp->attributes, "href");
            break;
        case HT_SCRIPT:
            if (((mask_bits & HeadScript) || mask_bits == HeadAll)
                    && !tmp->is_end) {
                head->script_lang = htmTagGetValue(tmp->attributes,
                    "language");

                // pick up the text, its all in a single element
                tmp = tmp->next;

                // sanity check
                if (!tmp->element)
                    break;

                // copy contents
                head->script = lstring::copy(tmp->element);
            }
            break;
        case HT_STYLE:
            if (((mask_bits & HeadStyle) || mask_bits == HeadAll)
                    && !tmp->is_end) {
                head->style_type = htmTagGetValue(tmp->attributes,
                    "type");

                // pick up the text, its all in a single element
                tmp = tmp->next;

                // sanity check
                if (!tmp->element)
                    break;

                // copy contents
                head->style = lstring::copy(tmp->element);
            }
            break;
        default:
            break;
        }
    }
    // fill in remaining link and meta members
    if (mask_bits & HeadMeta) {
        if (num_meta)
            head->meta = parse_meta(meta_start, &num_meta);
        head->num_meta = num_meta;
    }
    if (mask_bits & HeadLink) {
        if (num_link)
            head->link = parse_links(link_start, &num_link);
        head->num_link = num_link;
    }
    // we found a head
    return (true);
}


// Call installed callback routines.
//
void
htmWidget::linkCallback()
{
    if (htm_if) {
        // initialize callback fields
        htmLinkCallbackStruct cbs;

        // count how many link elements we have
        int num_link = 0;
        htmObject *start = 0;
        for (htmObject *temp = htm_elements; temp && temp->id != HT_BODY;
                temp = temp->next) {
            if (temp->id == HT_LINK) {
                num_link++;
                start = (num_link == 1 ? temp : start);
            }
        }
        // no <LINK> found, call with a zero links field
        if (num_link == 0 || start == 0) {
            cbs.link = 0;
            htm_if->emit_signal(S_LINK, &cbs);
            return;
        }

        // parse all link elements
        cbs.link = parse_links(start, &num_link);
        cbs.num_link = num_link;
        htm_if->emit_signal(S_LINK, &cbs);
    }
}


// Function associated with the anchorTrackCallback resource, fills in
// the appropriate fields in the htmCallbackStruct.
//
void
htmWidget::trackCallback(htmEvent *event, htmAnchor *anchor)
{
    if (htm_if) {
        htmAnchorCallbackStruct cbs(anchor);
        cbs.event = event;
        htm_if->emit_signal(S_ANCHOR_TRACK, &cbs);
    }
}


// Function associated with the activateCallback resource.  fills in
// the appropriate fields in the htmCallbackStruct.
//
void
htmWidget::activateCallback(htmEvent *event, htmAnchor *anchor)
{
    if (anchor == 0)
        return;

    // The anchor passed might be freed before we're done with it, so
    // make a copy.
    htmAnchor anchor_cpy(*anchor);

    htmAnchorCallbackStruct cbs(&anchor_cpy);
    cbs.event = event;

    if (htm_if) {
        const char *name = htm_if->get_frame_name();
        if (name) {
            // this is a frame child
            cbs.is_frame = true;
            if (!cbs.target)
                // target ourself if no other target
                cbs.target = name;
        }
    }

    // this might free anchor
    if (htm_if)
        htm_if->emit_signal(S_ACTIVATE, &cbs);

    // If we have a local anchor, see if we should mark it as visited
    // and if we should jump to it.  The jumping itself is postponed
    // to the end of this function.

    htmObjectTable *jump_anchor = 0;
    if (anchor_cpy.url_type == ANCHOR_JUMP) {
        // set new foreground color
        if (cbs.visited) {
            // first check if this anchor wasn't already visited
            if (!anchor_cpy.visited) {
                unsigned char line_style;
                // mark all other anchors pointing to the same name as well
                for (int i = 0; i < htm_anchor_words; i++) {
                    // check if this anchor matches
                    htmAnchor *tmp = htm_anchors[i]->owner->anchor;
                    if (!(strcasecmp(tmp->href, anchor_cpy.href))) {
                        // a match, set the foreground of the master block
                        htm_anchors[i]->owner->fg =
                            htm_cm.cm_anchor_visited_fg;

                        // change underline style as well!
                        line_style = ul_type(htm_anchor_visited_style);
                        if (htm_anchors[i]->self->line_data & LINE_STRIKE)
                            line_style |= LINE_STRIKE;

                        // update all words for this anchor
                        for (int j = 0; j < htm_anchors[i]->owner->n_words;
                                j++) {
                            htm_anchors[i]->owner->words[j].line_data =
                                line_style;
                        }
                    }
                    // skip remaining anchor words of the master block
                    while (i < htm_anchor_words-1 &&
                            htm_anchors[i]->owner ==
                                htm_anchors[i+1]->owner)
                        i++;
                }
            }
        }
        if (cbs.doit) {
            jump_anchor = getAnchorByName(anchor_cpy.href);
            if (jump_anchor == 0)
                warning("activateCallback",
                    "Can't locate named anchor %s.", anchor_cpy.href);
        }
    }
}


// DocumentCallback driver.  Returns true when another pass on the
// current text is requested/required, false when not.
//
bool
htmWidget::documentCallback(bool html32, bool verified, bool balanced,
    bool terminated, int pass_level)
{
    if (htm_if) {
        htmDocumentCallbackStruct cbs;
        cbs.html32     = html32;
        cbs.verified   = verified;
        cbs.balanced   = balanced;
        cbs.terminated = terminated;
        cbs.pass_level = pass_level;
        cbs.redo       = false;

        htm_if->emit_signal(S_DOCUMENT, &cbs);
        return (cbs.redo);
    }
    return (false);
}
// End of htmWidget functions


namespace {
    htmLinkDataRec *
    parse_links(htmObject *list, int *num_link)
    {
        // We have got some links. Allocate memory
        htmLinkDataRec *link = new htmLinkDataRec[*num_link];

        int i = 0;
        htmObject *temp = list;
        for ( ; temp && temp->id != HT_BODY && i < *num_link;
                temp = temp->next) {
            if (temp->id == HT_LINK) {
                // get value of REL tag
                char *tmp = htmTagGetValue(temp->attributes, "rel");
                if (tmp) {
                    // make lowercase
                    lstring::strtolower(tmp);
                    link[i].rel = tmp;
                }
                else {
                    tmp = htmTagGetValue(temp->attributes, "rev");
                    // get value of REV tag
                    if (tmp) {
                        // make lowercase
                        lstring::strtolower(tmp);
                        link[i].rev = tmp;
                    }
                    else
                        // invalid link member
                        continue;
                }

                tmp = htmTagGetValue(temp->attributes, "href");
                // get value of URL tag
                if (tmp)
                    link[i].url = tmp;
                else {
                    // href is mandatory
                    delete [] link[i].rel;
                    link[i].rel = 0;
                    delete [] link[i].rev;
                    link[i].rev = 0;
                    continue;
                }

                tmp = htmTagGetValue(temp->attributes, "title");
                // get value of TITLE tag
                if (tmp)
                    link[i].title = tmp;
                i++;
            }
        }
        // adjust link count for actually allocated elements
        *num_link = i;

        return (link);
    }


    htmMetaDataRec *
    parse_meta(htmObject *list, int *num_meta)
    {
        // We have got some links. Allocate memory
        htmMetaDataRec *meta = new htmMetaDataRec[*num_meta];

        int i = 0;
        htmObject *temp = list;
        for ( ; temp != 0 && temp->id != HT_BODY && i < *num_meta;
                temp = temp->next) {
            if (temp->id == HT_META) {
                // get value of http-equiv tag
                char *tmp = htmTagGetValue(temp->attributes, "http-equiv");
                if (tmp ) {
                    // make lowercase
                    lstring::strtolower(tmp);
                    meta[i].http_equiv = tmp;
                }
                else {
                    tmp = htmTagGetValue(temp->attributes, "name");
                    // get value of name tag
                    if (tmp) {
                        // make lowercase
                        lstring::strtolower(tmp);
                        meta[i].name = tmp;
                    }
                    else
                        // invalid meta element
                        continue;
                }

                tmp = htmTagGetValue(temp->attributes, "content");
                // get value of content tag
                if (tmp)
                    meta[i].content = tmp;
                else {
                    // invalid meta element
                    delete [] meta[i].http_equiv;
                    meta[i].http_equiv = 0;
                    delete [] meta[i].name;
                    meta[i].name = 0;
                    continue;
                }
                i++;
            }
        }
        // adjust meta count for actually allocated elements
        *num_meta = i;
        return (meta);
    }
}


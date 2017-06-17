
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
 * $Id: htm_layout.cc,v 1.16 2017/04/13 17:06:14 stevew Exp $
 *-----------------------------------------------------------------------*/

#include "htm_widget.h"
#include "htm_layout.h"
#include "htm_format.h"
#include "htm_table.h"
#include "htm_font.h"
#include "htm_parser.h"
#include "htm_image.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h> // debugging

// Maximum number of iterations the text-justification routines may
// reach.  Decreasing the default value of 1500 will lead to an
// increasing amount of warnings.

#define MAX_JUSTIFY_ITERATIONS              1500

// Maximum number of iterations the table layout computation routines
// may reach.  This only occurs when the minimum suggested table width
// is smaller than the available width and the maximum suggested table
// width is larger than the available width.  In this case, the layout
// routines have to compute an optimum balance between the different
// colum widths within the available width.  The algorithm used should
// be convergent, but could be divergent for nested tables or tables
// prefixed with an extreme indentation (nested lists and such).
// Hence the safeguard.

#define MAX_TABLE_ITERATIONS                128

namespace { void expandCols(int*, int*, int, int, int, bool); }

// Characters that must be flushed against a word.  Can't use ispunct
// since that are all printable chars that are not a number or a
// letter.
//
#define IS_PUNCT(c) (c == '.' || c == ',' || c == ':' || c == ';' || \
    c == '!' || c == '?')

namespace htm
{
    // Context helper for text layout.  This splits a single ghastly
    // function into several still fairly ugly functions.
    //
    struct TextCx
    {
        TextCx(PositionBox*, bool, htmFont*);

        void setup(htmWord**, int, Margins*, bool, htmLayoutManager*);
        bool flow_text(htmWord**, int, PositionBox*, Margins*,
            htmLayoutManager*);
        void check_font(htmWord*);
        void save_position_preformat(htmWord**, int, int, PositionBox*, bool,
            htmLayoutManager*);
        void save_position(htmWord**, int, int, PositionBox*, Margins*,
            htmLayoutManager*);
        void check_lineheight(htmWord*, bool);
        int interword_spacing(htmWord**, int, int, htmLayoutManager*);
        void finalize(htmWord**, int*, int, PositionBox*, bool, bool, bool,
            htmLayoutManager*);

        // initial offsets
        int left;
        int right;
        int width;
        int lheight;
        int rheight;
        unsigned int x_start;
        unsigned int x_pos;
        unsigned int y_pos;

        // interword spacing
        htmFont *font;
        htmFont *basefont;
        int sw;
        int e_space;
        int e_marg;

        htmWord *base_obj;
        int lineheight;
        int word_start;

        // misc state
        int p_lheight;
        int p_rheight;
        int lypos;
        int rypos;
        unsigned int min_box_width;
        unsigned int max_box_width;
        unsigned int max_box_height;
        int skip_idl;
        int skip_idr;
        bool first_line;
        bool in_line;
        bool have_object;
        bool done;
    };

    // Geometric context for table layout
    //
    struct TableCx
    {
        TableCx(htmTable*, int, int);
        ~TableCx();

        void step1(htmTable*, htmLayoutManager*);
        void step2(htmTable*, htmLayoutManager*, bool);
        void step3(htmTable*, htmLayoutManager*, bool);
        void step4(htmTable*, htmLayoutManager*);
        void step5(htmTable*, htmLayoutManager*);
        void step6(htmTable*, htmLayoutManager*);
        void step7(htmTable*, htmLayoutManager*, int, int);
        void step8(htmTable*, htmLayoutManager*, int);

        PositionBox **boxes;
        int *rows;
        int *min_cols;
        int *max_cols;
        int nrows;
        int ncols;
        int x_start;
        int twidth;

        int hspace;
        int vspace;
        int hpad;
        int vpad;
        int bwidth;
        int usable_twidth;

        int full_max_twidth;
        int full_min_twidth;
        int max_twidth;
        int min_twidth;
        int max_theight;
    };
}


// Perform layout of the entire display.
//
void
htmWidget::computeLayout()
{
    htmLayoutManager lm(this);

    // work_width is core width minus one horizontal margin.
    // Maximum useable width is core width minus two times the horizontal
    // margin.
    PositionBox box;
    box.x = htm_margin_width;
    box.y = htm_margin_height;
    box.lmargin = htm_margin_width;      // absolute left margin
    box.rmargin = htm_viewarea.width;    // absolute right margin
    box.tmargin = htm_margin_height;     // top margin
    box.bmargin = htm_margin_height;     // bottom margin
    box.width  = box.rmargin - box.lmargin;     // absolute box width
    box.height = -1;
    box.min_width = -1;
    box.min_height = -1;
    box.left  = box.lmargin;             // initial left offset
    box.right = box.rmargin;             // initial right offset
    box.idx = 0;

    if (htm_formatted == 0)
        return;

    // pass through first and set absolute width
    int w = 0;
    for (htmObjectTable *tmp = htm_formatted; tmp; tmp = tmp->next) {
        switch (tmp->object_type) {
        default:
            break;
        case OBJ_HRULE:
            if (tmp->len > w)
                w = tmp->len;
            break;
        case OBJ_TABLE:
            if (tmp->table->t_width > w)
                w = tmp->table->t_width;
            break;
        }
    }
    if (w > box.width) {
        box.width = w;
        box.right = box.rmargin = box.lmargin + w;
    }

    int yend = lm.core(htm_formatted, 0, &box, 0, false, false, true, true);

    htm_formatted_height = yend + box.tmargin + htm_default_font->descent;
    htm_formatted_width = lm.max_width();
    htm_nlines = lm.line();

    // now process any images with an alpha channel (if any)
    if (htm_im.im_delayed_creation)
        htm_im.imageCheckDelayedCreation();
}
// End of htmWidget functions


htmLayoutManager::htmLayoutManager(htmWidget *w)
{
    lm_html             = w;
    lm_baseline_obj     = 0;
    lm_bullet           = 0;
    lm_line             = 0;
    lm_last_text_line   = 0;
    lm_max_width        = 0;
    lm_curr_anchor      = 0;
    lm_named_anchor     = 0;
    lm_had_break        = false;
}


int
htmLayoutManager::core(htmObjectTable *obj_start, htmObjectTable *obj_end,
    PositionBox *box, PositionBox *box_return, bool precompute,
    bool table_layout, bool no_shrink, bool store_anchors)
{
    int max_width_save = lm_max_width;
    int bw_save = box->width;

    Margins margins;

    lm_had_break = false;
    lm_baseline_obj = 0;
    int yend = box->y;
    int xpos = box->x;
    int rmrg = box->rmargin;
    int y_real_end = yend;
    int yinit = yend;

    for (htmObjectTable *tmp = obj_start; tmp && tmp != obj_end;
            tmp = tmp->next) {

        box->width = bw_save;
        if (precompute) {
            box->min_width = -1;
            box->height = -1;
        }
        int ypos = box->y;

        int maxrmarg = 0;
        htmObjectTable *end;
        switch (tmp->object_type) {
        case OBJ_TEXT:
            // collect all words
            for (end = tmp;
                end->next->object_type == OBJ_TEXT; end = end->next) ;

            setText(box, tmp, end->next, false, precompute, table_layout,
                &margins, &maxrmarg);

            if (store_anchors) {
                for ( ; tmp->object_type == OBJ_TEXT; tmp = tmp->next)
                    storeAnchor(tmp);
            }
            tmp = end;
            if (box_return && box_return->width < box->width)
                box_return->width = box->width;
            break;

        case OBJ_PRE_TEXT:
            // collect all words
            for (end = tmp; end->next->object_type == OBJ_PRE_TEXT;
                end = end->next) ;

            margins.clear();
            box->y = yend;

            setText(box, tmp, end->next, true, precompute, table_layout,
                0, &maxrmarg);

            if (store_anchors) {
                for ( ; tmp->object_type == OBJ_PRE_TEXT; tmp = tmp->next)
                    storeAnchor(tmp);
            }
            tmp = end;
            if (box_return && box_return->width < box->width)
                box_return->width = box->width;
            break;

        case OBJ_BULLET:
            box->y = yend;
            setBullet(box, tmp, &margins);
            break;

        case OBJ_HRULE:
            setRule(box, tmp, &margins);
            break;

        case OBJ_TABLE:
            if (margins.left && margins.left->margin > box->x)
                box->x = margins.left->margin;
            if (margins.right && margins.right->margin < box->rmargin)
                box->rmargin = margins.right->margin;
            box->width = box->rmargin - box->lmargin;

            if (!margins.left && !margins.right &&
                    box->x + (int)tmp->ident > box->lmargin)
                setBreak(box, tmp);

            if (tmp->table->t_properties->tp_border)
                box->y++;

            end = setTable(box, tmp, no_shrink);

            // This is a hack to keep table columns that contain
            // sub-tables from being too wide.  The table size
            // calculation really needs a complete rewrite.
            //
            if (box->width == box->min_width)
                box->min_width = box->width/2;

            if (box->y > (int)yend)
                yend = box->y;
            if (tmp->table->t_properties->tp_halign == HALIGN_LEFT &&
                    !tmp->table->t_properties->tp_noinline &&
                    box->x + box->width < box->rmargin - 50) {
                Exclude *ex = new Exclude;
                if (margins.left && box_return && box_return->width <
                        margins.left->margin + box->width - xpos)
                    box_return->width =
                        margins.left->margin + box->width - xpos;
                ex->next = margins.left;
                margins.left = ex;
                margins.left->margin = box->x + box->width;
                margins.left->bottom = box->y;
                box->y = ypos;
            }
            else if (tmp->table->t_properties->tp_halign == HALIGN_RIGHT &&
                    !tmp->table->t_properties->tp_noinline &&
                    box->x + box->width < box->rmargin - 50) {
                Exclude *ex = new Exclude;
                if (box_return &&
                        box_return->width < box->rmargin - box->lmargin)
                    box_return->width = box->rmargin - box->lmargin;
                ex->next = margins.right;
                margins.right = ex;
                margins.right->margin = box->rmargin - box->width;
                margins.right->bottom = box->y;
                box->y = ypos;
            }
            else if (tmp->table->t_properties->tp_halign == HALIGN_CENTER &&
                    box->x + box->width < box->rmargin - 50) {
                if (box_return &&
                        box_return->width < box->rmargin - box->lmargin)
                    box_return->width = box->rmargin - box->lmargin;
                margins.clear();
                box->y = yend;
            }

            box->x = box->lmargin;
            box->rmargin = rmrg;

            if (store_anchors) {
                for ( ; tmp != end; tmp = tmp->next) {
                    if (tmp->object_type == OBJ_TEXT ||
                            tmp->object_type == OBJ_PRE_TEXT)
                        storeAnchor(tmp);
                    // empty named anchors can cause this
                    else if (tmp->text_data & TEXT_ANCHOR_INTERN) {
                        // save named anchor location
                        lm_html->htm_named_anchors[lm_named_anchor] = tmp;
                        lm_named_anchor++;
                    }
                }
            }
            setBlock(box, end);
            tmp = end->prev;
            if (box_return && box_return->width < box->width)
                box_return->width = box->width;
            break;

        case OBJ_TABLE_FRAME:
            break;

        case OBJ_APPLET:
            setApplet(box, tmp);
            setBreak(box, tmp);
            break;

        case OBJ_BLOCK:
            if ((!margins.left || box->y >
                    margins.left->bottom - tmp->font->height) &&
                    (!margins.right || box->y >
                    margins.right->bottom - tmp->font->height))
                box->y = yend;
            if (tmp->halign == CLEAR_LEFT || tmp->halign == CLEAR_ALL) {
                if (margins.left) {
                    Exclude *xx = margins.left->next;
                    if (margins.left->bottom - tmp->font->height > box->y)
                        box->y = margins.left->bottom - tmp->font->height;
                    delete margins.left;
                    margins.left = xx;
                }
            }
            if (tmp->halign == CLEAR_RIGHT || tmp->halign == CLEAR_ALL) {
                if (margins.right) {
                    Exclude *xx = margins.right->next;
                    if (margins.right->bottom - tmp->font->height > box->y)
                        box->y = margins.right->bottom - tmp->font->height;
                    delete margins.right;
                    margins.right = xx;
                }
            }
            setBlock(box, tmp);
            setBreak(box, tmp);
            break;

        case OBJ_NONE:
            setBlock(box, tmp);
            // empty named anchors can cause this
            if (store_anchors) {
                if (tmp->text_data & TEXT_ANCHOR_INTERN) {
                    // save named anchor location
                    lm_html->htm_named_anchors[lm_named_anchor] = tmp;
                    lm_named_anchor++;
                }
            }
            break;

        default:
            lm_html->warning("layout core", "Unknown object type!");
        }
        if (box->y > yend)
            yend = box->y;
        if ((margins.left || margins.right) && ypos + box->height > y_real_end)
            y_real_end = ypos + box->height;

        while (margins.left && box->y >= margins.left->bottom) {
            Exclude *xx = margins.left->next;
            delete margins.left;
            margins.left = xx;
        }
        while (margins.right && box->y >= margins.right->bottom) {
            Exclude *xx = margins.right->next;
            delete margins.right;
            margins.right = xx;
        }
        if (box->x > lm_max_width)
            lm_max_width = box->x;
        if (maxrmarg > lm_max_width)
            lm_max_width = maxrmarg;
        if (box_return && box->min_width > 0 &&
                box_return->min_width < box->min_width)
            box_return->min_width = box->min_width;
    }
    if (y_real_end - yinit > box->height)
        box->height = y_real_end - yinit;
    if (yend < y_real_end)
        yend = y_real_end;
    if (!store_anchors)
        // not at top level
        lm_max_width = max_width_save;
    return (yend);
}


void
htmLayoutManager::storeAnchor(htmObjectTable *tmp)
{
    if (tmp->text_data & TEXT_ANCHOR) {
        for (int i = 0 ; i < tmp->n_words; i++) {
            if (lm_curr_anchor == lm_html->htm_anchor_words) {
                lm_html->warning("storeAnchor",
                    "I'm about to crash: exceeding anchor word count!");
                lm_curr_anchor--;
            }
            lm_html->htm_anchors[lm_curr_anchor] = tmp->words + i;
            lm_curr_anchor++;
        }
    }
    if (tmp->text_data & TEXT_ANCHOR_INTERN) {
        if (lm_named_anchor == lm_html->htm_num_named_anchors) {
            lm_html->warning("storeAnchor",
                "I'm about to crash: exceeding named anchor count!");
            lm_named_anchor--;
        }
        lm_html->htm_named_anchors[lm_named_anchor] = tmp;
        lm_named_anchor++;
    }
}


// Adjust interword spacing to produce fully justified text.
// Justification is done on basis of the longest words.  Words that
// start with a punctuation character are never adjusted, they only
// get shoved to the right.  This routine could be much more efficient
// if the text to be justified would be sorted.
//
void
htmLayoutManager::justifyText(htmWord *words[], int word_start,
    int word_end, unsigned int sw, int len, int line_len, int skip_idl,
    int skip_idr)
{
    // See how many spaces we have to add
    int nspace = (int)((line_len - len)/(sw == 0 ? (sw = 3) : sw));

    // last line of a block or no spaces to add. Don't adjust it.
    // nspace can be negative if there are words that are longer than
    // the available linewidth
    if (nspace < 1)
        return;

    // we need at least two words if we want this to work
    if ((word_end - word_start) < 2)
        return;

    // no hassling for a line with two words, fix 07/03/97-02, kdh
    if ((word_end - word_start) == 2) {
        // just flush the second word to the right margin
        words[word_start+1]->area.x += nspace*sw;
        return;
    }

    // pick up the longest word
    int longest_word = 0;
    for (int i = word_start; i < word_end; i++) {
        if (i == skip_idl || i == skip_idr)
            continue;
        if (words[i]->len > longest_word)
            longest_word = words[i]->len;
    }
    int word_len = longest_word;
    int num_iter = 0;

    // adjust interword spacing until we run out of spaces to add
    while (nspace && num_iter < MAX_JUSTIFY_ITERATIONS) {
        // walk all words in search of the longest one
        for (int i = word_start; i < word_end && nspace; i++, num_iter++) {
            if (i == skip_idl || i == skip_idr || words[i]->len == 0)
                continue;
            // Found!
            if (words[i]->len == word_len &&
                    !IS_PUNCT(*(words[i]->word)) &&
                    !(words[i]->spacing & TEXT_SPACE_NONE)) {
                // see if we are allowed to shift this word
                if (!(words[i]->spacing & TEXT_SPACE_TRAIL) ||
                    !(words[i]->spacing & TEXT_SPACE_LEAD))
                    continue;

                // Add a leading space if we may, but always shift all
                // following words to the right.
                //
                // fix 07/03/97-01, kdh

                if (words[i]->spacing & TEXT_SPACE_LEAD && i != word_start) {
                    for (int j = i; j < word_end; j++) {
                        if (j == skip_idl || j == skip_idr)
                            continue;
                        words[j]->area.x += sw;
                    }
                    nspace--;
                }
                if (nspace) {
                    int j;
                    for (j = i + 1; j < word_end; j++) {
                        if (j == skip_idl || j == skip_idr)
                            continue;
                        words[j]->area.x += sw;
                    }

                    // we have only added a space if this is true
                    if (j != i+1)
                        nspace--;
                }
            }
        }
        num_iter++;
        // move on to next set of words eligible for space adjustement
        word_len = (word_len == 0 ? longest_word : word_len - 1);
    }
    if (num_iter == MAX_JUSTIFY_ITERATIONS) {
        lm_html->warning("justifyText",
            "Text justification: bailing out after %i iterations\n"
            "    (line %i of input).", MAX_JUSTIFY_ITERATIONS,
            words[word_start]->owner->object->line);
    }
}


// Adjust x-position of every word to reflect requested alignment.
// Every word in start and end (and any object(s) in between them)
// that belongs to the same line is updated to reflect the alignment.
// This routine just returns if the current alignment matches the
// default alignment.
//
void
htmLayoutManager::checkAlignment(htmWord *words[], int word_start,
    int word_end, int sw, int line_len, bool last_line, int skip_idl,
    int skip_idr)
{
    if (word_end < 1)
        return;

    // total line width occupied by these words
    int width = words[word_end-1]->area.right() - words[word_start]->area.x;

    int offset;
    switch (words[word_start]->owner->halign) {
    case HALIGN_RIGHT:
        offset = line_len - width;
        break;
    case HALIGN_CENTER:
        offset = (line_len - width)/2;
        break;
    case HALIGN_LEFT:
        offset = 0;
        break;
    case HALIGN_JUSTIFY:
        // sw == -1 when used for <pre> text
        if (lm_html->htm_enable_outlining && !last_line && sw != -1) {
            justifyText(words, word_start, word_end, sw, width, line_len,
                (word_start < skip_idl ? skip_idl : -1),
                (word_start < skip_idr ? skip_idr : -1));
            offset = 0;
            break;
        }
        // fall thru
    case HALIGN_NONE:
    default:
        // use specified alignment
        switch (lm_html->htm_alignment) {
        case ALIGNMENT_END:
            offset = line_len - width;
            break;
        case ALIGNMENT_CENTER:
            offset = (line_len - width)/2;
            break;
        case ALIGNMENT_BEGINNING:
        default:
            offset = 0;
            break;
        }
        break;
    }

    // Only adjust with a positive offset.  A negative offset
    // indicates that the current width is larger than the available
    // width.  Will ignore alignment setting for pre text that is
    // wider than the available window width.

    if (offset <= 0)
        return;
    for (int i = word_start; i < word_end; i++)
        words[i]->area.x += offset;
}


// Main text layout driver.
//
void
htmLayoutManager::setText(PositionBox *box, htmObjectTable *start,
    htmObjectTable *end, bool in_pre, bool precompute,
    bool table_layout, Margins *margins, int *maxrmarg)
{
    // To make it ourselves _much_ easier, put all the words starting
    // from start and up to end in a single block of words.
    int nwords;
    htmWord **words = getWords(start, end, &nwords);
    if (!words)
        return;

    // Set up the initial PositionBox to be used for text layout.
    PositionBox my_box;
    my_box.x         = box->x;
    my_box.y         = box->y;
    my_box.lmargin   = box->lmargin;
    my_box.rmargin   = box->rmargin;
    my_box.left      = box->left;
    if (words[0]->type == OBJ_TEXT)
        my_box.left += words[0]->font->lbearing;
    my_box.right     = box->rmargin;
    my_box.width     = my_box.right - my_box.left;
    my_box.min_width = -1;
    my_box.height    = -1;

    // do text layout
    computeText(&my_box, words, 0, &nwords, true, precompute | table_layout,
        in_pre, margins);

    if (maxrmarg)
        *maxrmarg = box->x + my_box.width + 2;

    if (precompute) {
        // update return values
        box->x = my_box.x;
        box->y = my_box.y;
        box->width = my_box.width;
        box->min_width = my_box.min_width;
        box->height = my_box.height;

        // no longer needed
        delete [] words;

        // done precomputing
        return;
    }

    // Update all ObjectTable elements for these words
    for (int i = 0; i < nwords; ) {
        htmObjectTable *current = words[i]->owner;
        current->line  = words[i]->line;
        current->font = words[i]->base->font;

        // Set the owner bounding area.

        htmRect r;
        for (int j = 0; j < current->n_words; j++)
            r.add(words[i+j]->area);
        current->area = r;

        i += current->n_words;
    }

    // and update return values
    box->x = my_box.x;
    box->y = my_box.y;
    box->height = my_box.height;

    // free words
    delete [] words;
}


// Order the given text data into single lines, breaking and moving up
// to the next line if necessary.  This function does the layout of
// complete paragraphs at once.  A paragraph is given by all text
// elements between start and end.
//
//  This is a rather complex operation. Things done are the following:
//   - considers images, HTML form members and text as the same objects;
//   - adjusts baseline according to the highest object on a line;
//   - adjusts space width if font changes;
//   - performs horizontal alignment;
//   - performs text outlining if required;
//   - glues words together if required (interword spacing);
//
void
htmLayoutManager::computeText(PositionBox *box, htmWord **words, int nstart,
    int *nwords, bool last_line, bool table_layout, bool preformat,
    Margins *margins)
{
    lm_had_break = false;
    TextCx cx(box, preformat, words[nstart]->font);
    cx.setup(words, nstart, margins, preformat, this);

    // Text layout:  we keep walking words until we are about to
    // exceed the available linewidth.  When we are composing a line
    // in this way, we keep track of the highest word (which will
    // define the maximum lineheight).  If a linefeed needs to be
    // inserted, the lineheight is added to every word for a line.  We
    // then move to the next line (updating the vertical offset as we
    // do) and the whole process repeats itself.

    int i;
    for (i = nstart; i < *nwords && !cx.done; i++)    {
        // Note that x_pos is unsigned and right is signed.  This is
        // important for testing against right = -1, which should act
        // like infinity.
        if (words[i]->type == OBJ_BLOCK &&
                cx.x_pos >= (unsigned int)cx.right) {
            // just skip it
            cx.e_space = 0;
            cx.x_pos = words[i]->update(lm_line, cx.x_pos + cx.e_space,
                cx.y_pos);
            continue;
        }

        if (!preformat && words[i]->type == OBJ_IMG &&
                (words[i]->image->align == HALIGN_LEFT ||
                words[i]->image->align == HALIGN_RIGHT)) {
            if (cx.flow_text(words, i, box, margins, this))
                continue;
        }
        lm_had_break = false;

        cx.check_font(words[i]);
        if (preformat)
            cx.save_position_preformat(words, *nwords, i, box, table_layout,
                this);
        else
            cx.save_position(words, *nwords, i, box, margins, this);
        cx.check_lineheight(words[i], preformat);
        if (!preformat)
            i = cx.interword_spacing(words, *nwords, i, this);
    }

    cx.finalize(words, nwords, i, box, last_line, table_layout, preformat,
        this);
}


void
htmLayoutManager::setApplet(PositionBox *box, htmObjectTable *data)
{
    data->area.x = box->x;
    data->area.y = box->y;
    data->area.height = lm_html->htm_default_font->height;
    data->line   = lm_line;
}


void
htmLayoutManager::setBlock(PositionBox *box, htmObjectTable *data)
{
    htmFont *font = (data->font ? data->font : lm_html->htm_default_font);
    data->area.x = box->x;
    data->area.y = box->y;
    data->area.height = font->lineheight;    // fix 01/25/97-01; kdh
    data->line   = lm_line;
}


// Compute the offset & position of a horizontal rule.  Rules always
// start at the left margin of the current page and extend across
// the full available width.
//
void
htmLayoutManager::setRule(PositionBox *box, htmObjectTable *data,
    Margins *margins)
{
    int width = box->width;
    int left = box->lmargin;
    int right = box->rmargin;
    int linefeed = data->linefeed;

    if (margins) {
        if (margins->left)
            left = margins->left->margin + 6;
        if (margins->right)
            right = margins->right->margin;
    }
    width = right - left - data->ident;
    box->x = left + data->ident;

    // See if we have a width specification
    if (data->len != 0) {
        if (data->len < 0)   // % spec
            width = (int)(-width*data->len/100.0);
        else
            // pixel spec, cut if wider than available
            width = (data->len > width ? width : data->len);
        // alignment is only honored if there is a width spec
        switch (data->halign) {
        case HALIGN_RIGHT:
            box->x = box->rmargin - width;
            break;
        case HALIGN_CENTER:
            box->x = (box->rmargin - width)/2;
        default:
            break;
        }
    }

    // check if this is a real linebreak or just a margin reset
    if (linefeed) {
        // if we already had a linefeed, we can substract one
        if (lm_had_break && lm_baseline_obj) {
            linefeed -= lm_baseline_obj->font->lineheight;
            lm_had_break = false;
        }
        // no negative linefeeds!!
        if (linefeed > 0)
            box->y += linefeed;
    }

    // vertical offset
    int dy = (int)(.25*(lm_html->htm_default_font->height));

    // save position and width
    data->area.x = box->x;
    data->area.y = box->y + dy;
    data->line  = lm_line;
    data->area.width = width;

    box->x = box->lmargin;
    box->y += data->area.height +
        (int)(.75*(lm_html->htm_default_font->lineheight));

    // linefeed
    lm_line += 2;
}


// Compute the position & offsets for a list leader (can be a bullet
// or number).  Bullets always carry indentation within them.  This
// indentation is the total indentation from the left margin and
// therefore the x-position is computed using the left margin, and not
// the left position.  As we want all bullets to be right aligned, the
// x-position of a bullet is computed as the left margin *plus* any
// indentation.  When being rendered, the real position is the
// computed position *minus* the width of the bullet.
//
void
htmLayoutManager::setBullet(PositionBox *box, htmObjectTable *data,
    Margins *margins)
{
    // save vertical position
    data->area.y = box->y;

    // linefeed if not at left margin
    if (box->x != box->lmargin)
        lm_line++;
    box->y += data->linefeed;
    box->x = box->lmargin + data->ident;

    // we have a left offset
    box->left = box->x;

    int lmrg = margins->left ? margins->left->margin : 0;

    // the rendering area is to the left of the position box location
    data->area.x = box->x - data->area.width + lmrg;
    if (data->marker == MARKER_DISC || data->marker == MARKER_SQUARE ||
            data->marker == MARKER_CIRCLE) {
        data->area.x -= data->area.width;
        data->area.y -= data->area.width;
    }
    else
        data->area.y -= lm_html->htm_default_font->ascent;

    data->line = lm_line;
    data->area.y += lm_html->htm_default_font->ascent;
    lm_bullet = data;  // tweek positioning later
}


void
htmLayoutManager::setBreak(PositionBox *box, htmObjectTable *data)
{
    int linefeed = data->linefeed;
    if (linefeed && lm_baseline_obj &&
            lm_baseline_obj->font->lineheight > linefeed)
        linefeed = lm_baseline_obj->font->lineheight;

    // position of this break
    data->area.y = box->y;
    data->area.x = box->x;

    // check if this is a real linebreak or just a margin reset
    if (linefeed) {
        // if we already had a linefeed, we can substract one
        if (lm_had_break && lm_baseline_obj) {
            linefeed -= lm_baseline_obj->font->lineheight;
            lm_had_break = false;
        }
        // no negative linefeeds!!
        if (linefeed > 0) {
            lm_line++;
            box->y += linefeed;
            // update box height
            box->height = linefeed;
        }
    }

    // reset margin
    box->x = box->lmargin + data->ident;
    box->left = box->x;

    data->line = lm_line;
    data->area.height = box->y - data->area.y;    // height of this linefeed
}


htmObjectTable *
htmLayoutManager::setTable(PositionBox *box, htmObjectTable *data,
    bool no_shrink)
{
    // pick up table data
    htmTable *table = data->table;

    // The first table in a stack of tables contains all data for all
    // table children it contains.  The first table child is the
    // master table itself.  So when a table doesn't have a child
    // table it is a child table itself and thus we should add the
    // left offset to the initial horizontal position.

    if (table->t_children)
        table = table->t_children;

    // table position
    data->area.x = box->x;
    data->area.y = box->y;

    // set maximum table width
    int max_table_width;
    if (table->t_width) {
        // relative to current box width
        if (table->t_width < 0 && box->width > 0)
            max_table_width = (int)((-table->t_width * box->width)/100.0);
        else if (table->t_width > 0)
            max_table_width = table->t_width;
        else
            max_table_width = -1;
    }
    else {
        // if this is a child table, assume a 100% width
        if (!table->t_children)
            max_table_width = box->width;
        else {
            // parent table, use whatever we can get our hands on
            if ((max_table_width = box->width + box->lmargin - box->x) <= 0)
                max_table_width = -1;
            no_shrink = false;
        }
    }
    int align_width = box->width + box->lmargin - box->x;
    if (align_width < 0)
        align_width = -1;

    // save current line number count
    int save_line = lm_line;

    // total no of rows and columns this table is made up of
    if (table->t_nrows == 0 || table->t_ncols == 0) {
        if (lm_html->htm_bad_html_warnings)
            lm_html->warning("setTable", "Empty table, ignoring.");
        return (table->t_end);
    }

    TableCx tcx(table, box->x, max_table_width);
    if (!tcx.boxes)
        return (table->t_end);

    tcx.step1(table, this);
    tcx.step2(table, this, no_shrink);
    tcx.step3(table, this, no_shrink);
    tcx.step4(table, this);
    tcx.step5(table, this);
    tcx.step6(table, this);
    tcx.step7(table, this, align_width, box->y);
    tcx.step8(table, this, save_line);

    // store return dimensions, box->x is not touched
    box->height = tcx.max_theight;
    box->y += box->height;
    table->t_end->area.height = tcx.max_theight;
    box->width = tcx.full_max_twidth;
    box->min_width = tcx.full_min_twidth;
    if (table->t_width > box->min_width)
        box->min_width = table->t_width;

    data->area.x = tcx.x_start - tcx.bwidth;
    // final (absolute) table dimensions
    data->area.height = tcx.max_theight;
    data->area.width  = tcx.full_max_twidth;

    // adjust maximum document width
    if (box->x + tcx.full_max_twidth > lm_max_width)
        lm_max_width = box->x + tcx.full_max_twidth;

    // all done!
    return (table->t_end);
}


void
htmLayoutManager::computeTable(PositionBox *box, htmObjectTable *obj_start,
    htmObjectTable *obj_end, bool precompute, bool no_shrink)
{
    if (!obj_start)
        return;

    if (precompute) {
        int y_start = box->y;
        PositionBox box_tmp(*box);
        PositionBox box_return(*box);
        box_tmp.y = 0;
        box_tmp.x = 0;
        box_return.width = 0;
        box_return.min_width = 0;

        int y_end = core(obj_start, obj_end, &box_tmp, &box_return, true,
            true, no_shrink, false);

        if (!no_shrink || box_return.width > box->width)
            box->width = box_return.width;
        box->min_width = box_return.min_width;

        if (box_return.height != -1) {
            if (box_return.height < 0)
                box->height = y_end - (y_start + box_return.height);
            else if (y_end - y_start > box->height)
                box->height = y_end - y_start;
        }
        else
            box->height = y_end - y_start;
    }
    else
        core(obj_start, obj_end, box, 0, false, true, no_shrink, false);
}


void
htmLayoutManager::checkVerticalAlignment(int height, PositionBox *box,
    htmObjectTable *start, htmObjectTable *end, Alignment valign)
{
    // height is actual contents height, box is container
    int y_offset = 0;
    if (box->height <= height)
        return;
    switch (valign) {
    case VALIGN_TOP:
        return;
    default:
    case VALIGN_MIDDLE:
        y_offset = (box->height - height)/2;
        break;
    case VALIGN_BOTTOM:
    case VALIGN_BASELINE:
        y_offset = box->height - height;
        break;
    }
    if (y_offset && start) {
        while (start != end) {
            start->area.y += y_offset;
            if (start->words) {
                for (int i = 0; i < start->n_words; i++) {
                    start->words[i].ybaseline += y_offset;
                    start->words[i].area.y += y_offset;
                }
            }
            start = start->next;
        }
    }
}


// create an array containing all OBJ_TEXT elements between start and
// end.
//
htmWord**
htmLayoutManager::getWords(htmObjectTable *start, htmObjectTable *end,
    int *nwords)
{
    int cnt = 0;
    for (htmObjectTable *tmp = start; tmp != end ; tmp = tmp->next)
        cnt += tmp->n_words;
    if (!cnt) {
        *nwords = 0;
        return (0);
    }

    htmWord **words = new htmWord*[cnt];

    int k = 0;
    if (lm_html->htm_string_r_to_l) {
        if (end == 0)
            for (end = start; end->next; end = end->next) ;
        for (htmObjectTable *tmp = end->prev; tmp != start->prev;
                tmp = tmp->prev) {
            for (int i = 0; i < tmp->n_words; i++) {
                // store word ptr and reset position to zero
                words[k] = tmp->words + i;
                words[k]->area.x = 0;
                words[k]->area.y = 0;
                words[k]->ybaseline = 0;
                words[k++]->line = 0;
            }
        }
    }
    else {
        for (htmObjectTable *tmp = start; tmp != end; tmp = tmp->next) {
            for (int i = 0; i < tmp->n_words; i++) {
                // store word ptr and reset position to zero
                words[k] = tmp->words + i;
                words[k]->area.x = 0;
                words[k]->area.y = 0;
                words[k]->ybaseline = 0;
                words[k++]->line = 0;
            }
        }
    }

    *nwords = cnt;
    return (words);
}


// Adjust the baseline for each word between start and end.
//
void
htmLayoutManager::adjustBaseline(htmWord *base_obj, htmWord **words,
    int start, int end, int *lineheight, bool)
{
    // The base_obj is the object in the line with the largest height.
    // If the base_obj is not OBJ_TEXT, then the base_obj->font is
    // the largest font found in the line.

    int y_offset = 0;
    if (base_obj->type == OBJ_IMG) {
        int i;
        for (i = start; i < end; i++) {
            if (words[i]->type == OBJ_TEXT)
                break;
        }
        if (i != end) {
            // only do this if text follows
            switch (base_obj->image->align) {
            case VALIGN_MIDDLE:
                y_offset = (*lineheight + base_obj->font->ascent)/2;
                break;
            case VALIGN_BASELINE:
            case VALIGN_BOTTOM:
                y_offset = *lineheight;
                *lineheight += base_obj->font->descent;
                break;
            case VALIGN_TOP:
            default:
                y_offset = base_obj->font->ascent;
                break;
            }
        }
    }
    else if (base_obj->type == OBJ_FORM) {
        // fix 07/04/97-01, kdh
        // form elements are always aligned in the middle
        y_offset = (*lineheight + base_obj->font->ascent)/2;
    }
    else if (base_obj->type == OBJ_TEXT || base_obj->type == OBJ_BLOCK)
        y_offset = base_obj->font->ascent;

    // Now adjust the baseline for every word on this line.
    for (int i = start; i < end; i++) {
        // only move text objects
        if (words[i]->type == OBJ_TEXT)
            words[i]->ybaseline += y_offset;
        else if (base_obj->type == OBJ_IMG) {
            switch (base_obj->image->align) {
            case VALIGN_MIDDLE:
                words[i]->ybaseline += (*lineheight - words[i]->area.height)/2;
                break;
            case VALIGN_BASELINE:
            case VALIGN_BOTTOM:
                words[i]->ybaseline += *lineheight - words[i]->area.height;
                break;
            case VALIGN_TOP:
            default:
                break;
            }
        }
        else if (base_obj->type == OBJ_FORM)
            words[i]->ybaseline += (*lineheight - words[i]->area.height)/2;
        words[i]->base = base_obj;
        words[i]->set_baseline(words[i]->ybaseline);
    }
    if (lm_bullet) {
        lm_bullet->area.y -= lm_html->htm_default_font->ascent;
        if (lm_bullet->marker == MARKER_DISC ||
                lm_bullet->marker == MARKER_SQUARE ||
                lm_bullet->marker == MARKER_CIRCLE) {
            lm_bullet->area.y += (base_obj->font->ascent +
                base_obj->font->descent)/2;
        }
        else
            lm_bullet->area.y += y_offset + 1;
        lm_bullet = 0;
    }
}


namespace {
    // Expand the table colums.  If a column is narrow, don't expand it,
    // unless all colums are narrow.  The total is increased by width.
    //
    void
    expandCols(int *max_cols, int *min_cols, int mincol, int nextcol,
        int width, bool do_min)
    {
#define NARROW 25
        bool doall = false;
        int cnt = 0;
        for (int i = mincol; i < nextcol; i++)
            if (max_cols[i] > NARROW && max_cols[i] > min_cols[i])
                cnt++;
        if (!cnt) {
            cnt = nextcol - mincol;
            doall = true;
        }

        int md = width % cnt;
        int del = width/cnt;
        for (int j = 0, i = mincol; i < nextcol; i++) {
            if ((max_cols[i] > NARROW && max_cols[i] > min_cols[i]) || doall) {
                if (do_min) {
                    min_cols[i] += del + (j < md);
                    j++;
                    if (max_cols[i] < min_cols[i])
                        max_cols[i] = min_cols[i];
                }
                else {
                    max_cols[i] += del + (j < md);
                    j++;
                }
            }
        }
    }
}


//-----------------------------------------------------------------------------
// Text Layout Context - does the work for text layout

TextCx::TextCx(PositionBox *box, bool preformat, htmFont *fnt)
{
    left            = box->left;
    right           = preformat ? -1 : box->right;
    width           = box->width;
    lheight         = 0;
    rheight         = 0;
    x_start         = box->left;
    x_pos           = box->left;
    y_pos           = box->y;

    font            = fnt;
    basefont        = fnt;
    sw              = fnt->isp;
    e_space         = fnt->isp;
    e_marg          = fnt->isp;

    base_obj        = 0;
    lineheight      = 0;
    word_start      = 0;

    p_lheight       = 0;
    p_rheight       = 0;
    lypos           = 0;
    rypos           = 0;
    min_box_width   = 0;
    max_box_width   = 0;
    max_box_height  = 0;
    skip_idl        = -1;
    skip_idr        = -1;
    first_line      = true;
    in_line         = true;
    have_object     = false;
    done            = false;
}


// Do some setup.
//
void
TextCx::setup(htmWord **words, int nstart, Margins *margins,
    bool preformat, htmLayoutManager *lm)
{
    // Proper baseline continuation of lines consisting of words with
    // different properties (font, fontstyle, images, form members or
    // anchors) require us to check if we are still on the same line.
    // If we are, we use the baseline object of that line.  If we are
    // on a new line, we take the first word of this line as the
    // baseline object.

    if (!preformat) {
        if (!lm->lm_baseline_obj)
            base_obj = words[nstart];
        else
            base_obj = (lm->lm_last_text_line == lm->lm_line ?
                lm->lm_baseline_obj : words[nstart]);
    }
    else
        base_obj = words[nstart];

    // lineheight always comes from the current baseline object
    lineheight = base_obj->area.height;
    if (preformat && base_obj->spacing != 0)
        lineheight = basefont->height;

    word_start = nstart;

    if (!preformat) {
        // set appropriate margins
        if (margins) {
            if (margins->left) {
                if (x_pos)
                    x_pos += margins->left->margin;
                else
                    x_pos = margins->left->margin + e_marg;
                left = x_pos;
            }
            if (margins->right)
                right = margins->right->margin - e_marg;
        }
        width = right - left;
    }
}


// Flow text around a right or left-aligned image.  If true is returned,
// advance to next word.
//
bool
TextCx::flow_text(htmWord **words, int curword, PositionBox *box,
    Margins *margins, htmLayoutManager *lm)
{
    htmWord *word = words[curword];
    int skip_id = -1;
    if (word->image->align == HALIGN_RIGHT && skip_idr == -1)
        skip_idr = skip_id = curword;
    else if (word->image->align == HALIGN_LEFT && skip_idl == -1)
        skip_idl = skip_id = curword;
    if (skip_id != -1) {
        // save all info for this word
        words[skip_id]->line = lm->lm_line;
        have_object = true;
        words[skip_id]->set_baseline(y_pos + words[skip_id]->owner->y_offset);

        // this word sets the baseline for itself
        words[skip_id]->base = words[skip_id];

        // set appropriate margins
        if (words[skip_id]->image->align == HALIGN_LEFT) {
            // Flush to the left margin
            words[skip_id]->area.x = x_pos;
            x_pos = words[skip_id]->area.right();
            left = x_pos + e_space;
            if (margins) {
                Exclude *tmp = new Exclude;
                tmp->next = margins->left;
                margins->left = tmp;
                tmp->margin = x_pos;
                tmp->bottom = y_pos + words[skip_id]->area.height;
            }
            lypos = y_pos;
            lheight = words[skip_id]->area.height;
            p_lheight = 0;
            if (skip_idr != -1) {
                if (words[skip_idl]->area.width +
                        words[skip_idr]->area.width > min_box_width)
                    min_box_width = words[skip_idl]->area.width +
                        words[skip_idr]->area.width;
            }
            else if (words[skip_idl]->area.width > min_box_width)
                min_box_width = words[skip_idl]->area.width;
        }
        else {
            // flush to the right margin
            words[skip_id]->area.x = right - words[skip_id]->area.width;
            right = words[skip_id]->area.x;
            if (margins) {
                Exclude *tmp = new Exclude;
                tmp->next = margins->right;
                margins->right = tmp;
                tmp->margin = right;
                tmp->bottom = y_pos + words[skip_id]->area.height;
            }
            rypos = y_pos;
            rheight = words[skip_id]->area.height;
            p_rheight = 0;
            if (skip_idl != -1) {
                if (words[skip_idl]->area.width +
                        words[skip_idr]->area.width > min_box_width)
                    min_box_width = words[skip_idl]->area.width +
                        words[skip_idr]->area.width;
            }
            else if (words[skip_idr]->area.width > min_box_width)
                min_box_width = words[skip_idr]->area.width;
        }
        width = box->width - words[skip_id]->area.width - sw - e_space;
        lineheight = 0;

        // we are already busy with a line, finish it first
        if (in_line)
            // start of a line, just proceed
            return (true);
    }
    return (false);
}


// Get new space width if font changes.
//
void
TextCx::check_font(htmWord *word)
{
    in_line = true;  // we are busy with a line of text
    if (font != word->font) {
        font = word->font;
        sw = font->isp;     // new interword spacing

        // If this font is larger than the current font it will
        // become the baseline font for non-text objects.

        if (font->lineheight > basefont->lineheight)
            basefont = font;
    }
}


// Save line, x and y pos for this word.  We don't do any interword
// spacing for <PRE> objects, they already have it.  Images and forms
// need to have the font ascent substracted to get a proper vertical
// alignment.
//
void
TextCx::save_position_preformat(htmWord **words, int nwords,
    int curword, PositionBox *box, bool table_layout, htmLayoutManager *lm)
{
    htmWord *word = words[curword];
    word->line = lm->lm_line;   // fix 04/26/97-01, kdh
    word->area.x = x_pos;
    word->base = base_obj;
    word->set_baseline(y_pos + word->owner->y_offset);
    if (word->type != OBJ_TEXT)
        have_object = true;
    x_pos += word->area.width;

    // we must insert a newline
    if (word->spacing != 0) {
        // Adjust font of non-text objects to the largest font
        // of the text objects (required for proper anchor
        // drawing)

        if (base_obj->type != OBJ_TEXT)
            base_obj->font = basefont;

        lm->adjustBaseline(base_obj, words, word_start, curword+1,
            &lineheight, false);
        int tlh = ((int)word->spacing) * basefont->lineheight;
        if (tlh > lineheight)
            lineheight = tlh;
        if (curword || !table_layout) {
            // The initial word is a line break
            y_pos += lineheight;

            // increment height of this paragraph
            p_lheight += lineheight;
            p_rheight += lineheight;
        }

        // Adjust for alignment
        lm->checkAlignment(words, word_start, curword, -1, width, false,
            -1, -1);

        if (x_pos > max_box_width)
            max_box_width = x_pos;

        x_pos = box->left;
        lm->lm_line++;
        word_start  = curword+1;      // next word starts on a new line
        if (curword+1 < nwords)
            base_obj = words[curword+1];
        basefont    = base_obj->font;
        lineheight  = basefont->lineheight; // default lineheight
        have_object = false;
        first_line  = false;
        in_line     = false;    // done with current line

        if (box->height != -1 && (p_lheight >= box->height ||
                p_rheight >= box->height))
            done = true;
    }
}


// Need to check if we may break words before we do the check on
// current line width:  if the current word doesn't have a trailing
// space, walk all words which don't have a leading and trailing space
// as well and end if we encounter the first word which does have a
// trailing space.  We then use the total width of this word to check
// against available line width.
//
void
TextCx::save_position(htmWord **words, int nwords, int curword,
    PositionBox *box, Margins *margins, htmLayoutManager *lm)
{
    htmWord *word = words[curword];
    int word_width;
    if (!(word->spacing & TEXT_SPACE_TRAIL) &&
        curword+1 < nwords && !(words[curword+1]->spacing & TEXT_SPACE_LEAD)) {
        int j = curword+1;
        word_width = word->area.width;
        while (j < nwords) {
            if (!(words[j]->spacing & TEXT_SPACE_LEAD))
                word_width += words[j]->area.width;

            // see if this word has a trail space and the next a
            // leading
            if (!(words[j]->spacing & TEXT_SPACE_TRAIL) && j+1 < nwords &&
                    !(words[j+1]->spacing & TEXT_SPACE_LEAD))
                j++;
            else
                break;
        }
    }
    else
        word_width = word->area.width;

    // minimum box width must fit the longest non-breakable word
    if ((int)min_box_width < word_width)
        min_box_width = word_width;

    bool is_break = word->type == OBJ_BLOCK;

    // Check if we are about to exceed the viewing width Note that r =
    // -1 is like r = infinity since x_pos is unsigned, thus in this
    // case lines are never forcibly broken.
    //
    bool toolong = is_break;
    if (!toolong) {
        // If this is the first word and there are no margins, too bad
        // if it doesn't fit, not much we can do.
        if (curword || (margins && (margins->left || margins->right))) {
            if (word->type == OBJ_TEXT) {
                if (x_pos + word_width + e_space + 2 > (unsigned int)right)
                    toolong = true;
            }
            else {
                if (x_pos + word_width >= (unsigned int)right)
                    toolong = true;
            }
        }
    }

    if (toolong) {

        // If this is a forced linebreak we act as this is the
        // last line in a paragraph:  no implicit lineheight
        // adjustment and no text justification in
        // CheckAlignment.

        // set font of non-text objects to the largest font of
        // the text objects (required for proper anchor
        // drawing)

        if (base_obj->type != OBJ_TEXT)
            base_obj->font = basefont;

        // adjust baseline for all words on the current line
        lm->adjustBaseline(base_obj, words, word_start, curword, &lineheight,
            is_break);

        // adjust for alignment
        lm->checkAlignment(words, word_start, curword, sw, width, is_break,
            skip_idl, skip_idr);

        // increment absolute height
        y_pos += lineheight;

        // increment absolute box height
        max_box_height += lineheight;

        if (!curword && !is_break) {
            // Move below the next margin, we know that there is one.
            unsigned int yb;
            if (margins->left && margins->right)
                yb = margins->left->bottom > margins->right->bottom ?
                    margins->left->bottom : margins->right->bottom;
            else if (margins->left)
                yb = margins->left->bottom;
            else
                yb = margins->right->bottom;
            if (y_pos < yb) {
                y_pos = yb;
                max_box_height += (yb - y_pos);
            }
        }

        // insert linebreak
        if (is_break) {
            int h = (int) ((word->line_data)*base_obj->font->lineheight);

            // no negative linebreaks!
            if ((h -= lineheight) < 0)
                h = 0;  // no negative breaks!
            y_pos += h;
            max_box_height += h;

            // INSERT CODE
            // if we are flowing around an image, check if
            // break = clear was given.  If so, terminate flow
            // around the current image.

            // This word was a break and therefore the next
            // word can't have a leading space (if it has it
            // will mess up text justification).
            // Fix 12/15/97-02, kdh
            if (curword+1 != nwords)
                words[curword+1]->spacing &= ~TEXT_SPACE_LEAD;
        }

        // update maximum box width
        if (word->type == OBJ_TEXT) {
            if (x_pos - x_start + 2 > max_box_width)
                max_box_width = x_pos - x_start + 2;
        }
        else {
            if (x_pos - x_start > max_box_width)
                max_box_width = x_pos - x_start;
        }

        x_pos = x_start;
        lm->lm_line++;
        word_start  = curword;  // next word starts on a new line
        base_obj    = word;
        lineheight  = base_obj->area.height;
        have_object = false;    // object has been done
        first_line  = false;    // no longer the first line
        in_line     = false;    // done with current line

        // line is finished, set all margins for proper text flowing
        if (skip_idl != -1 || skip_idr != -1) {
            if (!margins) {
                p_lheight += lineheight;
                p_rheight += lineheight;
                if (skip_idl != -1)
                    x_pos = words[skip_idl]->area.right();
            }
            if (skip_idl != -1 && p_lheight >= lheight) {
                skip_idl = -1;
                lheight = -1;
                if (!margins) {
                    left  = box->left;
                    x_pos = x_start;
                }
            }
            if (skip_idr != -1 && p_rheight >= rheight) {
                skip_idr = -1;
                rheight = -1;
                if (!margins)
                    right = box->right;
            }
            if (!margins)
                width = box->width;
        }
        if (margins && (margins->left || margins->right)) {
            // set appropriate margins
            p_lheight += lineheight;
            p_rheight += lineheight;
            while (margins->left && (int)y_pos >= margins->left->bottom) {
                Exclude *tmp = margins->left;
                margins->left = margins->left->next;
                delete tmp;
            }
            while (margins->right && (int)y_pos >= margins->right->bottom) {
                Exclude *tmp = margins->right;
                margins->right = margins->right->next;
                delete tmp;
            }
            if (margins->left) {
                if (margins->left->margin + e_marg > (int)x_pos) {
                    if (x_pos)
                        x_pos += margins->left->margin;
                    else
                        x_pos = margins->left->margin + e_marg;
                    left = x_pos;
                }
            }
            else
                left  = box->left;
            if (margins->right) {
                if (right > margins->right->margin - e_marg)
                    right = margins->right->margin - e_marg;
            }
            else
                right = box->right;
            width = right - left;
        }
    }
}


// Save maximum lineheight.  Also, if the base_obj is block, switch it
// to the present object.
//
void
TextCx::check_lineheight(htmWord *word, bool preformat)
{
    if (lineheight < (int)word->area.height || base_obj->type == OBJ_BLOCK) {
        if (!preformat || !word->spacing) {
            lineheight = word->area.height;
            base_obj   = word;
        }
    }
}


// Interword Spacing.
//   1.  word starts at beginning of a line, don't space it
//      at all.  (box->lmargin includes indentation as well)
//   2. previous word does not have a trailing spacing:
//       a. current word does have leading space, space it.
//       b. current word does not have a leading space, don't
//          space it.
//   3. previous word does have a trailing space:
//       a. always space current word.
//   4. previous word does not have any spacing:
//       a. current word has leading space, space it.
//       b. current word does not have a leading space, don't
//          space it.
// Note:  if the previous word does not have a trailing
// space and the current word does not have a leading
// space, these words are ``glued'' together.
//
int
TextCx::interword_spacing(htmWord **words, int nwords, int curword,
    htmLayoutManager *lm)
{
    e_space = 0;
    if (curword != 0 && (int)x_pos != left) {
        int tsw = sw;
        // use average spacing when font changes, looks better
        if (curword && words[curword-1]->font != words[curword]->font)
            tsw = (words[curword-1]->font->isp + words[curword]->font->isp)/2;
        if (words[curword-1]->type == OBJ_IMG &&
                words[curword]->type == OBJ_IMG)
            e_space = 0;
        else if (!(words[curword-1]->spacing & TEXT_SPACE_TRAIL)) {
            if (words[curword]->spacing & TEXT_SPACE_LEAD)
                e_space = tsw;
        }
        else if (words[curword-1]->spacing & TEXT_SPACE_TRAIL)
            e_space = tsw;
        else if (words[curword]->spacing & TEXT_SPACE_LEAD)
            e_space = tsw;

        // additional end-of-line spacing?
        if (e_space && words[curword]->len > 0 &&
                words[curword]->word[words[curword]->len-1] == '.')
            e_space += font->eol_sp;
    }

    // save linenumber, x and y positions for this word or for
    // multiple words needing to be ``glued'' together.

    if (!(words[curword]->spacing & TEXT_SPACE_TRAIL) &&
        curword+1 < nwords && !(words[curword+1]->spacing & TEXT_SPACE_LEAD)) {
        // first word must take spacing into account
        x_pos = words[curword]->update(lm->lm_line, x_pos + e_space, y_pos);
        if (words[curword]->type != OBJ_TEXT &&
                words[curword]->type != OBJ_BLOCK)
            have_object = true;
        // all other words are glued, so no spacing!
        e_space = 0;
        curword++;
        while (curword < nwords) {
            // don't take left/right flushed image into account
            if (curword == skip_idl || curword == skip_idr)
                continue;
            // connected word, save line, x and y pos.
            if (!(words[curword]->spacing & TEXT_SPACE_LEAD)) {
                x_pos = words[curword]->update(lm->lm_line, x_pos + e_space,
                    y_pos);
                if (words[curword]->type != OBJ_TEXT &&
                        words[curword]->type != OBJ_BLOCK)
                    have_object = true;
            }

            // this word has a trailing and the next a leading space?
            if (!(words[curword]->spacing & TEXT_SPACE_TRAIL) &&
                    curword+1 < nwords &&
                    !(words[curword+1]->spacing & TEXT_SPACE_LEAD))
                curword++;
            else
                break;
        }
    }
    else {
        // save line, x and y pos for this word
        x_pos = words[curword]->update(lm->lm_line, x_pos + e_space, y_pos);
        if (words[curword]->type != OBJ_TEXT &&
                words[curword]->type != OBJ_BLOCK)
            have_object = true;
    }
    return (curword);
}


// Final computations, update the position box with returned values.
//
void
TextCx::finalize(htmWord **words, int *nwords, int curword, PositionBox *box,
    bool last_line, bool table_layout, bool preformat, htmLayoutManager *lm)
{
    if (table_layout && !(word_start == *nwords - 1 &&
            words[word_start]->type == OBJ_BLOCK))
        have_object = true;

    // If we've got an image left, update it.  We only have an image
    // left if it's position hasn't been updated in the above loop, it
    // will be positioned otherwise, but we ran out of text before we
    // reached the box's height.  So we need to update y_pos to move
    // the baseline properly down.  The box itself isn't restored as
    // we have to check the alignment for this last line as well.

    if (skip_idl != -1) {
        if (words[skip_idl]->area.x == 0 && words[skip_idl]->ybaseline == 0) {
            x_pos = words[skip_idl]->update(
                lm->lm_line, x_pos + e_space, y_pos);
            if (words[skip_idl]->type != OBJ_TEXT &&
                    words[skip_idl]->type != OBJ_BLOCK)
                have_object = true;
            x_pos -= words[skip_idl]->area.width;
        }
        in_line = false;
        have_object = false;
    }
    if (skip_idr != -1) {
        if (words[skip_idr]->area.x == 0 && words[skip_idr]->ybaseline == 0) {
            x_pos = words[skip_idr]->update(
                lm->lm_line, x_pos + e_space, y_pos);
            if (words[skip_idr]->type != OBJ_TEXT &&
                    words[skip_idr]->type != OBJ_BLOCK)
                have_object = true;
        }
        in_line = false;
        have_object = false;
    }

    // How do we know we are at the end of this block of text
    // objects??  If the calling routine set last_line to true, we
    // know we are done and we can consider the layout computation
    // done.  If last_line is false, we can be sure that other text is
    // coming so we must continue layout computation on the next call
    // to this routine.  If we haven't finished computing the layout
    // for all words, we were flowing text around an object (currently
    // only images), and we need to adjust the number of words done
    // and be able to restart computation on the next call to this
    // routine.

    if (curword == *nwords) {
        if (last_line)
            done = true;
        else
            done = false;
    }
    else if (done) {
        *nwords = curword;
        done = false;
    }

    // also adjust baseline for the last line
    if (base_obj->type != OBJ_TEXT)
        base_obj->font = basefont;

    if (lineheight == 0)
        lineheight = base_obj->area.height;

    if (word_start < *nwords) {
        lm->adjustBaseline(base_obj, words, word_start, curword,
            &lineheight, done);

        // also adjust alignment for the last line
        lm->checkAlignment(words, word_start, *nwords, preformat ? -1 : sw,
            width, done, skip_idl, skip_idr);
    }

    if (base_obj->type == OBJ_TEXT && skip_idl == -1 && skip_idr == -1)
        y_pos += 2;

    // save initial vertical offset
    unsigned int y_start = box->y;

    // non-text objects (images & form members) move the baseline downward
    if (have_object) {
        box->y = y_pos + lineheight;
        lm->lm_had_break = true;
    }
    else
        box->y = y_pos;
    box->x = x_pos;

    // store final box height
    if (first_line || (box->height = box->y - y_start) == 0) {
        if (lineheight > base_obj->font->height)
            box->height = lineheight;
        else
            box->height = base_obj->area.height;
    }
    else if (!preformat)
        box->height = max_box_height;

    if (skip_idl != -1 && (int)(lypos + words[skip_idl]->area.height -
            y_start) > box->height)
        box->height = lypos + words[skip_idl]->area.height - y_start;
    if (skip_idr != -1 && (int)(rypos + words[skip_idr]->area.height -
            y_start) > box->height)
        box->height = rypos + words[skip_idr]->area.height - y_start;

    // check maximum box width again
    if (words[*nwords - 1]->type == OBJ_TEXT) {
        if (x_pos - x_start + 2 > max_box_width)
            max_box_width = x_pos - x_start + 2;
    }
    else {
        if (x_pos - x_start > max_box_width)
            max_box_width = x_pos - x_start;
    }

    if (skip_idr != -1)
        max_box_width += words[skip_idr]->area.width;

    if (preformat)
        min_box_width = max_box_width;

    if (words[0]->type == OBJ_TEXT) {
         max_box_width += words[0]->font->lbearing;
         min_box_width += words[0]->font->lbearing;
    }
    if (max_box_width < min_box_width)
        max_box_width = min_box_width;

    if (min_box_width >= 0.95*max_box_width)
        min_box_width = max_box_width;

    // store minimum and maximum box width
    box->width = max_box_width;
    box->min_width = min_box_width;

    // and check against document maximum width
    if ((int)max_box_width > lm->lm_max_width)
        lm->lm_max_width = max_box_width;

    // last text line and baseline object for this piece of text
    lm->lm_last_text_line = lm->lm_line;
    lm->lm_baseline_obj   = base_obj;

    // If we haven't done a full line, we must increase linenumbering
    // anyway as we've inserted a linebreak.

    if (first_line)
        lm->lm_line++;
}


//-----------------------------------------------------------------------------
// Table Layout Context - does the work for table layout

TableCx::TableCx(htmTable *table, int start_pos, int max_table_width)
{
    // total no of rows and columns this table is made up of
    nrows = table->t_nrows;
    ncols = table->t_ncols;
    boxes = 0;
    rows = 0;
    min_cols = 0;
    max_cols = 0;
    x_start = start_pos;
    twidth = max_table_width;

    hspace = table->t_hmargin;
    vspace = table->t_vmargin;
    hpad   = table->t_hpadding;
    vpad   = table->t_vpadding;
    bwidth = table->t_properties->tp_border;
    if (!bwidth)
        bwidth = 1;
        // Nested tables with border=0 show background of outer table
        // 1 pixel wide around inner table.

    // Sanity Check:  check all cells in search of a rowspan
    // attribute.  If we detect a rowspan in the last cell of a
    // row, we must add a bogus cell to this row.  If we don't do
    // this, any cells falling in this row will be skipped,
    // causing text to disappear (at the least, in the worst case
    // it will cause a crash 'cause any in the skipped text
    // anchors are never detected).

    htmTableRow *row = 0;
    for (int i = 0; i < table->t_nrows; i++) {
        row = table->t_rows + i;
        if (!row->tr_owner) {
            nrows = i-1;
            break;
        }
        for (int j = 0; j < row->tr_ncells; j++) {
            if (row->tr_cells[j].tc_rowspan > 1 && (j+1) == ncols)
                ncols++;
        }
    }
    if (nrows <= 0)
        // Error condition, caller must check for null booxes.
        return;

    boxes = new PositionBox*[nrows];
    for (int i = 0; i < nrows; i++)
        boxes[i] = new PositionBox[ncols];

    usable_twidth = twidth - ncols*(hspace + 2*hpad) - 2*bwidth - hspace;
    if (usable_twidth < 0)
        usable_twidth = -1;
    full_max_twidth = 0;
    full_min_twidth = 0;
    max_twidth      = 0;
    min_twidth      = 0;
    max_theight     = 0;
}


TableCx::~TableCx()
{
    for (int i = 0; i < nrows; i++)
        delete [] boxes[i];
    delete [] boxes;
    delete [] rows;
    delete [] min_cols;
    delete [] max_cols;
}


// Step One:  check if we have cells spanning multiple rows or
// columns.  We always assume the table is rectangular:  each row has
// the same amount of columns.  If a cell is really being used, it
// will have a positive box index.  A box with a negative index value
// means that this is a bogus cell spanned by it's neighbouring cells
// (which can be in another row).
//
void
TableCx::step1(htmTable *table, htmLayoutManager*)
{
    for (;;) {
        for (int i = 0; i < nrows; i++) {
            htmTableRow *row = table->t_rows + i;

            int j, idx;
            for (j = 0, idx = 0; j < ncols && idx < row->tr_ncells; j++) {
                // can happen when a cell spans multiple rows
                if (boxes[i][j].idx == -1)
                    continue;

                htmTableCell *cell = row->tr_cells + idx;

                // adjust col & rowspan if not set or incorrect
                if (cell->tc_colspan <= 0 || cell->tc_colspan > ncols)
                    cell->tc_colspan = ncols;
                if (cell->tc_rowspan <= 0 || cell->tc_rowspan > nrows)
                    cell->tc_rowspan = nrows;

                boxes[i][j].idx = idx;
                if (cell->tc_colspan != 1) {
                    // subsequent cells are spanned by this cell
                    int k;
                    for (k = 1; j + k < ncols && k < cell->tc_colspan; k++)
                        boxes[i][j + k].idx = -1;
                    // update cell counter to last spanned cell
                    j += (k-1);
                }
                if (cell->tc_rowspan != 1) {
                    // subsequent rows are spanned by this cell
                    for (int k = 1; i + k < nrows && k < cell->tc_rowspan; k++)
                        boxes[i + k][j].idx = -1;
                }
                idx++;
            }
            if (j != ncols) {
                for ( ; j < ncols; j++)
                    boxes[i][j].idx = -1;
            }
        }

        // check if the last column can be eliminated
        int i;
        for (i = 0; i < nrows; i++) {
            if (boxes[i][ncols-1].idx != -1)
                break;
        }
        if (i < nrows)
            break;
        ncols--;
    }
}


// Step Two:  compute minimum and maximum width of each cell.  All
// precomputation is done without any margins, they will be added
// later.
//
void
TableCx::step2(htmTable *table, htmLayoutManager *lm, bool no_shrink)
{
    for (int i = 0; i < nrows; i++) {
        htmTableRow *row = table->t_rows + i;

        // compute desired cell dimensions
        for (int j = 0; j < ncols; j++) {

            // skip if this is a spanned cell
            int idx;
            htmTableCell *cell;
            if ((idx = boxes[i][j].idx) != -1)
                cell = row->tr_cells + idx;
            else
                continue;

            // offsets unused

            // Preset margins.  Layout precomputation *excludes* any
            // spacing, it is added later since it doesn't apply to
            // the cell contents.  Defaults are for unknown cell
            // dimensions.

            boxes[i][j].lmargin = 0;
            boxes[i][j].width = boxes[i][j].height = -1;
            boxes[i][j].min_width = -1;

            // do we have a width?
            if (cell->tc_width) {
                // is it relative to table width?
                if (cell->tc_width < 0) {
                    // yes it is, do we have a table width?
                    if (twidth != -1) {
                        boxes[i][j].width = (int)((-cell->tc_width*(twidth -
                            2*bwidth - hspace))/100.0 - 2*hpad - hspace);
                        if (boxes[i][j].width < 0)
                            boxes[i][j].width = 0;
                    }
                }
                else
                    // absolute cell width
                    boxes[i][j].width = cell->tc_width - 2*hpad - hspace;
            }
            else if (ncols == 1 && twidth > 0) {
                boxes[i][j].width = twidth - 2*(bwidth + hspace + hpad);
                if (boxes[i][j].width < 0)
                    boxes[i][j].width = -1;
            }
            boxes[i][j].rmargin = boxes[i][j].width;

            // Do we have a cell height?  If so, it must be an
            // absolute number.  Can't do anything with relative
            // heights.

            if (cell->tc_height > 0 && (cell->tc_height - 2*vpad - vspace) > 0)
                boxes[i][j].height = cell->tc_height - 2*vpad - vspace;
            else
                boxes[i][j].height = -1;    // leave open ended

            // Precompute the required dimensions for this cell.  Upon
            // return, the PositionBox will have updated values for
            // width, min_width and height.

            // if width was -1, the *maximum* width is returned
            int tempw = boxes[i][j].width;
            if (cell->tc_properties->tp_nowrap)
                boxes[i][j].width = -1;
            lm->computeTable(&boxes[i][j], cell->tc_start, cell->tc_end,
                true, false);
            if (cell->tc_properties->tp_nowrap)
                boxes[i][j].min_width = boxes[i][j].width;
            if (boxes[i][j].width < tempw && no_shrink)
                boxes[i][j].width = tempw;
            if (tempw > 0 && cell->tc_width > 0) {
                if (boxes[i][j].min_width < tempw) {
                    boxes[i][j].min_width = tempw;
                    boxes[i][j].width = tempw;
                }
            }
        }
    }
}


// Step Three:  compute minimum and maximum row widths.  The table
// layout is contained in the PositionBox matrix.
//
void
TableCx::step3(htmTable *table, htmLayoutManager *lm, bool no_shrink)
{
    // Allocate room for minimum row dimensions, initialize to unknown
    // size.
    rows = new int[nrows];
    memset(rows, -1, nrows * sizeof(int));

    // Allocate room to store min and max column dimensions, initialize
    // to unknown size.
    min_cols = new int[ncols];
    memset(min_cols, -1, ncols * sizeof(int));
    max_cols = new int[ncols];
    memset(max_cols, -1, ncols * sizeof(int));

    // compute minimum & maximum column widths and row heights
    htmTableCell *cell = 0;
    for (int i = 0; i < nrows; i++) {
        int row_max_height = 0;

        // get current row
        htmTableRow *row = table->t_rows + i;

        // walk all cells in this row
        for (int j = 0; j < ncols; j++) {
            // skip if this is a spanned cell
            int idx;
            if ((idx = boxes[i][j].idx) != -1)
                cell = row->tr_cells + idx;
            else
                continue;

            // Height & width are useless for cells spanning multiple
            // rows or cells, both will get proper values once all
            // cell widths have been set.  The one exception is when
            // the minimum width of this cell equals the maximum
            // width, in which case the cell can't possibly be broken.
            //
            // As we are computing *total* cell dimensions, we must
            // take border width & cell spacing into account as well.

            if (cell->tc_colspan == 1) {
                if (min_cols[j] < boxes[i][j].min_width)
                    min_cols[j] = boxes[i][j].min_width;

                if (max_cols[j] < boxes[i][j].width)
                    max_cols[j] = boxes[i][j].width;
            }
            if (cell->tc_rowspan == 1) {
                // get maximum row height
                if (row_max_height < boxes[i][j].height)
                    row_max_height = boxes[i][j].height;
            }
            else
                boxes[i][j].height = -1;
        }
        // store height for this row
        rows[i] = row_max_height;

        // and update table height
        max_theight += row_max_height;
    }

    // check if we have any open-ended columns
    for (int i = 0; i < ncols; i++) {
        if (max_cols[i] == -1 || min_cols[i] == -1) {
            bool have_width = false;

            for (int k = 0; k < nrows; k++) {
                htmTableRow *row = table->t_rows + k;
                int idx;
                if ((idx = boxes[k][i].idx) != -1)
                    cell = row->tr_cells + idx;
                else
                    continue;
                if (cell->tc_width) {
                    have_width = true;
                    break;
                }
            }
            if (!have_width) {
                // empty column with no width set
                if (min_cols[i] == -1)
                    min_cols[i] = 0;
                if (max_cols[i] == -1)
                    max_cols[i] = min_cols[i];
                continue;
            }
            // If here, the column is empty, but has a relative width
            // to the unknown parent width.

             if (min_cols[i] == -1)
                 min_cols[i] = 1;
             if (max_cols[i] == -1)
                 max_cols[i] = lm->lm_html->htm_viewarea.width;
             if (cell->tc_width < 0)
                 // should always be true
                 max_cols[i] = (int)(max_cols[i] * -cell->tc_width/100.0);
        }
    }

    // modify column widths to account for spanned columns
    for (int i = 0; i < nrows; i++) {
        // get current row
        htmTableRow *row = table->t_rows + i;

        // walk all cells in this row
        for (int j = 0; j < ncols; j++) {
            // skip if this is a spanned cell
            int idx;
            if ((idx = boxes[i][j].idx) != -1)
                cell = row->tr_cells + idx;
            else
                continue;

            if (cell->tc_colspan > 1) {
                int tmax = 0, tmin = 0;
                int dx = hspace + 2*hpad;

                for (int k = 0; k < cell->tc_colspan && j+k < ncols; k++)
                    tmax += max_cols[j+k] + dx;
                for (int k = 0; k < cell->tc_colspan && j+k < ncols; k++)
                    tmin += min_cols[j+k] + dx;
                if (cell->tc_width) {
                    int wid = 0;
                    if (cell->tc_width < 0) {
                        if (twidth != -1) {
                            wid = (int)((-cell->tc_width*(twidth -
                                2*bwidth - hspace))/100.0 - dx);
                            if (wid < 0)
                                wid = 0;
                        }
                    }
                    else {
                        // absolute cell width
                        if (cell->tc_width > twidth && twidth > 0)
                            wid = -1;
                        else
                            wid = cell->tc_width;
                    }
                    if (wid > tmax) {
                        int k = j + cell->tc_colspan;
                        if (k > ncols)
                            k = ncols;
                        expandCols(max_cols, min_cols, j, k, wid - tmax,
                            false);
                        tmax = wid;
                    }
                }

                if (boxes[i][j].min_width + dx > tmin) {
                    int k = j + cell->tc_colspan;
                    if (k > ncols)
                        k = ncols;
                    expandCols(max_cols, min_cols, j, k,
                        boxes[i][j].min_width + (k-1)*dx - tmin, true);
                }
                // these get reset later
                boxes[i][j].width = -1;
                boxes[i][j].min_width = -1;
            }
        }
    }

    // Compute full minimum & maximum table widths.  Full table width
    // takes all all spacing into account.  The used table width is
    // the table width minus all spacing.

    for (int i = 0; i < ncols; i++) {
        // max_ and min_twidth exclude any spacing
        max_twidth += max_cols[i];
        min_twidth += min_cols[i];

        // full_ includes all spacing
        full_max_twidth += max_cols[i] + hspace + 2*hpad;
        full_min_twidth += min_cols[i] + hspace + 2*hpad;
    }

    // full widths include left & right borders as well
    full_max_twidth += 2*bwidth + hspace;
    full_min_twidth += 2*bwidth + hspace;

    // Adjust width if necessary
    if (no_shrink && table->t_width && full_max_twidth < twidth) {
        expandCols(max_cols, min_cols, 0, ncols, twidth - full_max_twidth,
            false);
        full_max_twidth = twidth;
        max_twidth = full_max_twidth - ncols*(hspace + 2*hpad) -
            (2*bwidth + hspace);
    }
}


// Step Four: compute column widths.
//
void
TableCx::step4(htmTable *table, htmLayoutManager *lm)
{
    // case 1:  minimum width equal or wider than total width For
    // nested tables, twidth can be -1 if the table with wasn't set to
    // an absolute number.  In this case return present width and fix
    // it later.

    if (twidth == -1) {
        ;
    }
    else if (full_min_twidth >= twidth) {
        // assign each column it's minimum width
        for (int i = 0; i < ncols; i++)
            max_cols[i] = min_cols[i];
        max_twidth = min_twidth;
        full_max_twidth = full_min_twidth;
    }
    else if (full_max_twidth < twidth) {
        // case 2: maximum width less than total width

        // Re-evaluation of cell heights only required if col or row
        // spanning is in effect.  If so, the appropriate flag has
        // already been set in the above logic.

        // When a table has an absolute width (table->t_width > 0), we
        // stretch all columns so the available width is used up
        // almost entirely (roundoff errors).

        if (table->t_width > 0) {
            min_twidth = 0;
            full_min_twidth = 0;
            for (int i = 0; i < ncols; i++) {
                // compute width percentage used by this column
                float pwidth = (float)max_cols[i]/(float)max_twidth;
                max_cols[i] = (int)(pwidth*usable_twidth);
                min_twidth += max_cols[i];
                full_min_twidth += max_cols[i] + hspace + 2*hpad;
            }
            // save new table width
            max_twidth = min_twidth;
            full_min_twidth += 2*bwidth + hspace;
            full_max_twidth = full_min_twidth;
        }
    }
    else {
        // case 3: max width exceeds available width while min width fits
        int nloop = 0;

        // Loop this until the table fits the available width or we
        // exceed the allowed no of iterations.

        while (full_max_twidth > twidth && nloop < MAX_TABLE_ITERATIONS) {
            // Difference between available space and minimum table width
            float w_diff = (float)(usable_twidth - min_twidth);

            // difference between maximum and minimum table width
            float m_diff = (float)(max_twidth - min_twidth);

            // prevent divide by zero
            if (m_diff == 0.0)
                m_diff = 1.0;

            // For each column, get the difference between minimum and
            // maximum column width and scale using the above
            // differences.

            max_twidth = 0;
            full_max_twidth = 0;
            for (int i = 0; i < ncols; i++) {
                if (max_cols[i] > twidth/ncols || nloop > 2) {
                    float c_diff = max_cols[i] - min_cols[i];
                    max_cols[i]  = min_cols[i] +
                        (int)(c_diff * (w_diff/m_diff));
                }

                // update maximum width: add spacing
                max_twidth += max_cols[i];
                full_max_twidth += max_cols[i] + hspace + 2*hpad;
            }
            full_max_twidth += 2*bwidth + hspace;
            nloop++;
        }
        if (nloop == MAX_TABLE_ITERATIONS) {
            lm->lm_html->warning("setTable step 4",
                "Cell Dimension: bailing out after %i iterations\n"
                "    (near line %i in input).", MAX_TABLE_ITERATIONS,
                table->t_start->object->line);
        }
    }
}


// Step Five:  recompute row heights.  For a number of cells, the
// width will be less than the maximum cell width and thus lines will
// be broken.  If we don't recompute the required box height, the
// table will overflow vertically.  As we are recomputing the cell
// dimensions, we must substract the horizontal spacing we added
// above.
//
void
TableCx::step5(htmTable *table, htmLayoutManager *lm)
{
    for (int i = 0; i < nrows; i++) {
        int row_max_height = 0;
        int row_max_width  = 0;
        htmTableRow *row = table->t_rows + i;
        for (int j = 0; j < ncols; j++) {

            // skip if this is a spanned cell
            int idx;
            htmTableCell *cell;
            if ((idx = boxes[i][j].idx) != -1)
                cell = row->tr_cells + idx;
            else
                continue;

            // offsets unused
            boxes[i][j].x       = 0;
            boxes[i][j].y       = 0;
            boxes[i][j].left    = 0;
            boxes[i][j].right   = 0;

            // set margins
            boxes[i][j].lmargin = 0;

            // set right margin
            if (cell->tc_colspan == 1)
                boxes[i][j].rmargin = max_cols[j];
            else {
                // spans multiple columns, add up column widths
                boxes[i][j].rmargin = 0;
                for (int k = j; k < j + cell->tc_colspan && k < ncols; k++)
                    boxes[i][j].rmargin += max_cols[k] + 2*hpad + hspace;
                boxes[i][j].rmargin -= 2*hpad + hspace;
            }
            boxes[i][j].width = boxes[i][j].rmargin - boxes[i][j].lmargin;

            if (cell->tc_height > 0 && (cell->tc_height - 2*vpad) > 0)
                boxes[i][j].height = cell->tc_height - 2*vpad;
            else
                boxes[i][j].height = -1;    // leave open ended

            lm->computeTable(&boxes[i][j], cell->tc_start, cell->tc_end,
                true, true);

            // update maximum row width
            row_max_width += boxes[i][j].width;

            // update maximum row height, taking spacing into account
            if (cell->tc_rowspan == 1 && row_max_height < boxes[i][j].height)
                row_max_height = boxes[i][j].height;

            // Update table width if we're exceeding it, which should
            // not be really happening as the cell width will only
            // decrease:  each cell already has it's minimum width.

            if (max_twidth < row_max_width)
                max_twidth = row_max_width;
        }
        rows[i] = row_max_height;
    }
}


// Step Six:  adjust row heights to account for rowspan attributes.
//
// Each (filled) cell has it's dimensions set.  We now need to adjust
// the row heights to properly account for any rowspan attributes set.
// The way we do this is as follows:  for each row, check if it
// contains a cell with the rowspan attribute set and get the one
// which spans the most rows.  When found, compute the total height of
// all spanned rows and then compute the difference between this
// height and the height of the spanning cell.  If this difference is
// negative, we can keep the current row heights.  If it's positive
// however, we distribute this difference evenly accross the height of
// all spanned rows.
//
void
TableCx::step6(htmTable *table, htmLayoutManager*)
{
    for (int i = 0; i < nrows; i++) {
        int max_span = 1;
        int span_cell = -1;

        htmTableRow *row = table->t_rows + i;
        for (int j = 0; j < ncols; j++) {
            // skip if this is a spanned cell
            int idx;
            htmTableCell *cell;
            if ((idx = boxes[i][j].idx) != -1)
                cell = row->tr_cells + idx;
            else
                continue;
            if (cell->tc_rowspan > max_span) {
                max_span = cell->tc_rowspan;
                span_cell = j;
            }
        }
        if (span_cell != -1 && span_cell < ncols) {
            // height of spanning cell
            int max_h = boxes[i][span_cell].height;

            // Compute height of all spanned rows.  If spanned height
            // is greater than the occupied height, add residue to
            // last row.

            int span_h = 0, k;
            for (k = i; k < nrows && k < i + max_span; k++)
                span_h += rows[k];
            if (max_h - span_h > 0)
                rows[k-1] += max_h - span_h;
        }
    }
}


// Step Seven: assign box dimensions for each cell.
//
void
TableCx::step7(htmTable *table, htmLayoutManager *lm, int align_width,
    int box_y)
{
    // Check horizontal table alignment and compute initial horizontal
    // offset.

    if (align_width > full_max_twidth) {
        switch (table->t_properties->tp_halign) {
        case HALIGN_RIGHT:
            x_start += align_width - full_max_twidth;
            break;
        case HALIGN_CENTER:
            x_start += (align_width - full_max_twidth)/2;
            break;
        case HALIGN_LEFT:
        case HALIGN_JUSTIFY:  // useless for tables
        case HALIGN_NONE:
        default:
            break;
        }
    }

    max_theight = 0;
    max_twidth  = 0;

    // if the height was given, take this into account
    if (table->t_height > 0) {
        int maxh = 2*bwidth + vspace;
        for (int i = 0; i < nrows; i++)
            maxh += rows[i] + 2*vpad + vspace;
        if (table->t_height > maxh) {
            int dy = table->t_height - maxh;
            int n1 = dy / nrows;
            int n2 = dy % nrows;
            for (int i = 0; i < nrows; i++) {
                rows[i] += n1 + (n2 > 0 ? 1 : 0);
                n2--;
            }
        }
    }

    // adjust upper left corner of table
    x_start += bwidth;
    unsigned int y_pos = box_y + bwidth;

    for (int i = 0; i < nrows; i++) {
        int tw = 0;

        // pick up current row
        htmTableRow*row = table->t_rows + i;

        // top-left row positions
        unsigned int x_pos = x_start;
        row->tr_owner->area.x = x_pos;
        row->tr_owner->area.y = y_pos;

        for (int j = 0; j < ncols; j++) {
            int cwidth = 0, cheight = 0;

            // pick up current cell if not a spanned cell
            int idx;
            htmTableCell *cell = 0;
            if ((idx = boxes[i][j].idx) != -1)
                cell = row->tr_cells + idx;

            // initial start positions for this box
            boxes[i][j].x = x_pos + hspace;
            boxes[i][j].y = y_pos + vspace + vpad;

            // Set correct left & right margin, taking horizontal
            // spacing into account.

            boxes[i][j].lmargin = x_pos + hpad + hspace;
            if (idx == -1 || cell->tc_colspan == 1) {
                // set right margin
                boxes[i][j].rmargin = boxes[i][j].lmargin + max_cols[j];

                // total cell width
                cwidth = max_cols[j] + 2*hpad + hspace;
            }
            else {
                // spans multiple columns, add up column sizes
                boxes[i][j].rmargin = boxes[i][j].lmargin;
                for (int k = j; k < j + cell->tc_colspan && k < ncols; k++) {
                    // left & right padding
                    boxes[i][j].rmargin += (max_cols[k] + 2*hpad + hspace);

                    // total cell width
                    cwidth += max_cols[k] + 2*hpad + hspace;
                }
                // above loop adds one to many
                boxes[i][j].rmargin -= 2*hpad + hspace;
            }

            // set available cell width
            boxes[i][j].width = boxes[i][j].rmargin - boxes[i][j].lmargin;
            boxes[i][j].left  = boxes[i][j].lmargin;
            boxes[i][j].right = boxes[i][j].rmargin;

            // set correct cell height
            if (idx == -1 || cell->tc_rowspan == 1) {
                boxes[i][j].height = rows[i] + 2*vpad;
                cheight = boxes[i][j].height + vspace;
            }
            else {
                // spans multiple rows, add up the row heights it occupies
                boxes[i][j].height = 0;
                for (int k = i; k < i + cell->tc_rowspan && k < nrows; k++) {
                    boxes[i][j].height += rows[k] + 2*vpad;
                    cheight += (rows[k] + 2*vpad + vspace);
                }

            }
            // set vertical margins
            boxes[i][j].tmargin = vpad;
            boxes[i][j].bmargin = boxes[i][j].height;

            // Store bounding box dimensions for proper frame
            // rendering, but never do this if the current cell is a
            // spanned one.  If we would do this the offsets would be
            // horribly wrong...

            if (idx != -1) {
                cell->tc_owner->area.x = x_pos;
                cell->tc_owner->area.y = y_pos;
                cell->tc_owner->area.width  = cwidth;
                cell->tc_owner->area.height = cheight;
            }

            // Advance x position to next column.  Must include any
            // padding & spacing.

            x_pos += (max_cols[j] + 2*hpad + hspace);
            tw += (max_cols[j] + 2*hpad + hspace);
        }
        // update max_width if necessary
        if ((int)x_pos > lm->lm_max_width) {
            // adjust maximum document width
            lm->lm_max_width = x_pos;
        }
        if (max_twidth < tw)
            max_twidth = tw;

        // move to next row, row height already includes padding
        y_pos += rows[i] + 2*vpad + vspace;

        // save row dimensions
        row->tr_owner->area.width = x_pos - row->tr_owner->area.x;
        row->tr_owner->area.height = y_pos - row->tr_owner->area.y;
    }
    max_twidth  += 2*bwidth + hspace;

    // final table height
    max_theight = y_pos - box_y + bwidth + vspace;
    full_max_twidth = max_twidth;
}


// Step Eight:  compute real text layout using the computed box
// dimensions.
//
void
TableCx::step8(htmTable *table, htmLayoutManager *lm, int save_line)
{
    // restore line count
    lm->lm_line = save_line;

    for (int i = 0; i < table->t_nrows; i++) {
        htmTableRow *row = table->t_rows + i;

        // restore line count for each row
        lm->lm_line = save_line;

        int max_line = 0;

        // layout all cells in this row
        for (int j = 0; j < ncols; j++) {
            // skip if this is a spanned cell
            int idx;
            htmTableCell *cell;
            if ((idx = boxes[i][j].idx) != -1)
                cell = row->tr_cells + idx;
            else
                continue;

            // same line count for each cell
            lm->lm_line = save_line;

            // compute final layout for the current cell
            int bw_save = boxes[i][j].width;
            int bh_save = boxes[i][j].height;
            int height = -boxes[i][j].y;
            boxes[i][j].height = -1;
            lm->computeTable(&boxes[i][j], cell->tc_start, cell->tc_end,
                false, true);
            height += boxes[i][j].y;
            if (height < boxes[i][j].height)
                height = boxes[i][j].height;
            boxes[i][j].width = bw_save;
            boxes[i][j].height = bh_save;
            // adjust cell contents vertically
            height += 2*vpad;
            lm->checkVerticalAlignment(height, &boxes[i][j], cell->tc_start,
                cell->tc_end, cell->tc_properties->tp_valign);

            // store maximum line count in this row
            if (max_line < (lm->lm_line - save_line))
                max_line = (lm->lm_line - save_line);
        }
        // row done, adjust linecount
        save_line += max_line;
    }
    // all done, set correct linenumber
    lm->lm_line = save_line;
}


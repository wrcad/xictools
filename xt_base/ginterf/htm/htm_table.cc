
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
#include "htm_table.h"
#include "htm_string.h"
#include "htm_tag.h"
#include "htm_parser.h"
#include "htm_format.h"
#include "htm_image.h"

#include <string.h>
#include <stdio.h>


namespace htm
{
    TableFraming
    htmGetFraming(const char *attributes, TableFraming def)
    {
        TableFraming ret_val = def;

        // First check if this tag does exist
        char *buf;
        if ((buf = htmTagGetValue(attributes, "frame")) == 0)
            return (ret_val);

        // transform to lowercase
        lstring::strtolower(buf);

        if (!(strcmp(buf, "void")))
            ret_val = TFRAME_VOID;
        else if (!(strcmp(buf, "above")))
            ret_val = TFRAME_ABOVE;
        else if (!(strcmp(buf, "below")))
            ret_val = TFRAME_BELOW;
        else if (!(strcmp(buf, "hsides")))
            ret_val = TFRAME_HSIDES;
        else if (!(strcmp(buf, "lhs")))
            ret_val = TFRAME_LEFT;
        else if (!(strcmp(buf, "rhs")))
            ret_val = TFRAME_RIGHT;
        else if (!(strcmp(buf, "vsides")))
            ret_val = TFRAME_VSIDES;
        else if (!(strcmp(buf, "box")))
            ret_val = TFRAME_BOX;
        else if (!(strcmp(buf, "border")))
            ret_val = TFRAME_BORDER;

        delete [] buf;
        return (ret_val);
    }


    TableRuling
    htmGetRuling(const char *attributes, TableRuling def)
    {
        TableRuling ret_val = def;

        // First check if this tag does exist
        char *buf;
        if ((buf = htmTagGetValue(attributes, "rules")) == 0)
            return (ret_val);

        // transform to lowercase
        lstring::strtolower(buf);

        if (!(strcmp(buf, "none")))
            ret_val = TRULE_NONE;
        else if (!(strcmp(buf, "groups")))
            ret_val = TRULE_GROUPS;
        else if (!(strcmp(buf, "rows")))
            ret_val = TRULE_ROWS;
        else if (!(strcmp(buf, "cols")))
            ret_val = TRULE_COLS;
        else if (!(strcmp(buf, "all")))
            ret_val = TRULE_ALL;

        delete [] buf;
        return (ret_val);
    }
}


// Scan a table element for common properties (border, alignment,
// background and border styles).
//
htmTableProperties*
htmWidget::tableCheckProperties(const char *attributes,
    htmTableProperties *parent, Alignment halign, unsigned int bg,
    htmImage *bg_image)
{
    htmTableProperties prop;
    if (parent)
        prop = *parent;
    else
        prop.tp_halign = halign;
    prop.tp_bg       = bg;
    prop.tp_bg_image = bg_image;

    // Horizontal alignment is only inherited through the halign
    // argument:  the align attribute on the table tag applies to the
    // table in a whole, not to any of it's members.

    htmTableProperties *prop_ret = new htmTableProperties;
    prop_ret->tp_halign = htmGetHorizontalAlignment(attributes, HALIGN_NONE);
    if (prop_ret->tp_halign == HALIGN_NONE) {
        prop_ret->tp_halign = halign;
        if (halign == HALIGN_LEFT || halign == HALIGN_RIGHT)
            // don't wrap text around table
            prop_ret->tp_noinline = true;
    }
    prop_ret->tp_valign = htmGetVerticalAlignment(attributes, prop.tp_valign);
    prop_ret->tp_nowrap = htmTagCheck(attributes, "nowrap");

    // Border value.  If -1 is returned, check for the presence of the
    // word ``border'' in the attributes.  If it exists, we assume a
    // non-zero border width.

    prop_ret->tp_border = htmTagGetNumber(attributes, "border", -1);
    // if the word ``border'' is present, use default border width
    if (prop_ret->tp_border == -1) {
        if (htmTagCheck(attributes, "border"))
            prop_ret->tp_border = HTML_DEFAULT_TABLE_BORDERWIDTH;
        else
            prop_ret->tp_border = prop.tp_border;
    }

    // Framing applies per-table.  If border is non-zero, the default
    // is to render a fully framed table.

    prop_ret->tp_framing  = htmGetFraming(attributes,
        prop_ret->tp_border > 0 ? TFRAME_BOX : TFRAME_VOID);

    // Ruling applies per-cell.  If border is non-zero, the default is
    // to render full borders around this cell.

    prop_ret->tp_ruling   = htmGetRuling(attributes,
        prop_ret->tp_border > 0 ? TRULE_ALL : TRULE_NONE);

    // only pick up background color if we are allowed to honor this attrib
    char *chPtr;
    if (htm_allow_color_switching &&
            (chPtr = htmTagGetValue(attributes, "bgcolor")) != 0) {
        prop_ret->tp_bg = htm_cm.getPixelByName(chPtr, prop.tp_bg);
        prop_ret->tp_flags |= TP_BG_GIVEN;
        delete [] chPtr;
    }
    else
        prop_ret->tp_bg = prop.tp_bg;

    // table background image?
    if ((chPtr = htmTagGetValue(attributes, "background"))) {

        // kludge so htmNewImage recognizes it
        char *buf = new char[strlen(chPtr) + 7];
        sprintf(buf, "src=\"%s\"", chPtr);

        // load it
        htmImage *image;
        unsigned int width, height;
        if ((image = htm_im.newImage(buf, &width, &height)) != 0) {
            // animations are not allowed as background images
            if (image->IsAnim())
                image = 0;
            // and we sure won't have the default image as background
            else if (image->IsInternal())
                image = 0;
        }
        prop_ret->tp_bg_image = image;
        prop_ret->tp_flags |= TP_IMAGE_GIVEN;
        delete [] buf;
        delete [] chPtr;
    }
    else
        prop_ret->tp_bg_image = prop.tp_bg_image;
    return (prop_ret);
}


htmTableProperties::htmTableProperties()
{
    tp_border       = HTML_DEFAULT_TABLE_BORDERWIDTH;
    tp_halign       = HALIGN_NONE;
    tp_valign       = VALIGN_MIDDLE;
    tp_bg           = 0;
    tp_bg_image     = 0;
    tp_flags        = 0;
    tp_framing      = TFRAME_BOX;
    tp_ruling       = TRULE_ALL;
    tp_nowrap       = false;
    tp_noinline     = false;
}


htmTableCell::htmTableCell()
{
    tc_header       = false;
    tc_width        = 0;
    tc_height       = 0;
    tc_rowspan      = 1;
    tc_colspan      = 0;
    tc_properties   = 0;
    tc_start        = 0;
    tc_end          = 0;
    tc_owner        = 0;
    tc_parent       = 0;
}


htmTableCell::~htmTableCell()
{
    delete tc_properties;
}


htmTableRow::htmTableRow()
{
    tr_cells        = 0;
    tr_ncells       = 0;
    tr_lastcell     = 0;
    tr_properties   = 0;
    tr_start        = 0;
    tr_end          = 0;
    tr_owner        = 0;
    tr_parent       = 0;
}


htmTableRow::~htmTableRow()
{
    delete [] tr_cells;
    delete tr_properties;
}


htmTable::htmTable()
{
    t_width         = 0;
    t_height        = 0;
    t_hmargin       = 0;
    t_vmargin       = 0;
    t_hpadding      = 0;
    t_vpadding      = 0;
    t_ncols         = 0;
    t_ident         = 0;
    t_properties    = 0;

    t_caption       = 0;
    t_rows          = 0;
    t_nrows         = 0;
    t_lastrow       = 0;

    t_parent        = 0;
    t_parent_cell   = 0;
    t_children      = 0;
    t_nchildren     = 0;
    t_lastchild     = 0;

    t_start         = 0;
    t_end           = 0;

    t_owner         = 0;
    t_next          = 0;
}


htmTable::~htmTable()
{
    delete [] t_rows;
    delete t_properties;
    if (!t_parent && t_children) {
        t_children[0].t_children = 0;
        t_children[0].t_rows = 0;
        t_children[0].t_properties = 0;
        delete [] t_children;
    }
}


// Static function.
// Open a new table.
//
htmTable*
htmTable::open(htmTable *thistab, htmWidget *html, htmObjectTable *start,
    htmObject *obj, Alignment *halign, unsigned int *bg, htmImage **bg_image,
    htmTableCell **tcellp)
{
    htmTable *table;
    if (thistab) {
        // get to the absolute parent of this table
        htmTable *parent_table = thistab;
        while (parent_table->t_parent)
            parent_table = parent_table->t_parent;

        // get correct ptr
        parent_table = parent_table->t_children;

        // sanity check
        if (parent_table->t_lastchild + 1 == parent_table->t_nchildren)
            html->fatalError("Bad table count!!!");

        // get next available table
        parent_table->t_lastchild++;
        table = parent_table->t_children + parent_table->t_lastchild;

        // Save a pointer the the parent cell
        table->t_parent_cell = *tcellp;
        *tcellp = 0;
    }
    else
        table = new htmTable;

    // Get table attributes.

    table->t_width    = htmTagCheckNumber(obj->attributes, "width", 0);
    table->t_height   = htmTagCheckNumber(obj->attributes, "height", 0);
    table->t_hmargin  = htmTagGetNumber(obj->attributes, "cellspacing",
                        HTML_DEFAULT_CELLSPACING);
    table->t_vmargin  = htmTagGetNumber(obj->attributes, "rowspacing",
                        table->t_hmargin);
    table->t_hpadding = htmTagGetNumber(obj->attributes, "cellpadding", 0);
    table->t_vpadding = htmTagGetNumber(obj->attributes, "rowpadding",
                        table->t_hpadding);
    table->t_ncols    = htmTagGetNumber(obj->attributes, "cols", 0);
    table->t_start    = start;    // starting object
    table->t_owner    = start;    // owning object
    table->t_parent   = 0;        // parent table

    // table properties
    table->t_properties = html->tableCheckProperties(obj->attributes,
        thistab ? thistab->t_properties : 0, *halign, *bg, *bg_image);

    // set return alignment
    *halign = table->t_properties->tp_halign;

    // set return background
    *bg = table->t_properties->tp_bg;
    *bg_image = table->t_properties->tp_bg_image;

    int nrows = 0;          // no of rows in table
    int depth = 0;
    int nchildren = 0;      // no of table children in this table
    Alignment caption_position = VALIGN_TOP;  // where is the caption?
    bool have_caption = false;

    // count how many rows this table has
    for (htmObject *tmp = obj->next; tmp; tmp = tmp->next) {
        // check for end of table and child tables
        if (tmp->id == HT_TABLE) {
            if (tmp->is_end) {
                if (depth == 0)
                    break;
                else
                    depth--;
            }
            else {
                // new table opens
                depth++;
                nchildren++;
            }
        }

        // Only count a row when it belongs to the top-level table.  A
        // caption is considered a special row that spans the entire
        // table and has only one cell:  the row itself.

        if ((tmp->id == HT_TR || tmp->id == HT_CAPTION) && depth == 0 &&
                !tmp->is_end) {
            if (tmp->id == HT_CAPTION) {

                // See where the caption should be inserted:  as the
                // first or last row for this table.

                char *chPtr = htmTagGetValue(tmp->attributes, "align");
                if (chPtr == 0)
                    caption_position = VALIGN_TOP;
                else {
                    if (!(strcasecmp(chPtr, "bottom")))
                        caption_position = VALIGN_BOTTOM;
                    else
                        caption_position = VALIGN_TOP;
                    delete [] chPtr;
                }
                have_caption = true;
            }
            nrows++;
        }
    }
    // sanity, should never happen
    if (!nrows)
        nrows++;

    // allocate all rows for this table
    table->t_rows = new htmTableRow[nrows];
    table->t_nrows = nrows;
    table->t_lastrow = 0;

    // set caption ptr
    if (have_caption) {
        if (caption_position == VALIGN_TOP) {
            table->t_caption = table->t_rows;
            table->t_lastrow = 1;
        }
        else
            table->t_caption = table->t_rows + nrows - 1;
    }

    // The master table contains all tables
    if (!thistab) {
        nchildren++;
        table->t_children = new htmTable[nchildren];
        table->t_nchildren = nchildren;
        table->t_children[0] = *table;      // we are the first table
    }
    else {
        table->t_children = 0;
        table->t_nchildren = 0;
        table->t_lastchild = 0;
        // set parent table
        table->t_parent = thistab;
    }

    // and set as table in the element given to us
    start->table = table;

    return (table);
}


// Perform required table wrapup actions.
//
htmTable*
htmTable::close(htmWidget*, htmObjectTable *end, htmTableCell **tcellp)
{
    // bad hack
    htmTable *real_table = t_owner->table;
    real_table->t_start  = t_owner->next;
    real_table->t_end    = end;

    // pick up correct ptr
    htmTable *table = this;
    if (!table->t_parent)
        table = t_children;

    table->t_start = table->t_owner->next;
    table->t_end   = end;

    // Sanity Check:  check all cells in search of a rowspan
    // attribute.  If we detect a rowspan in the *last* cell of a row,
    // we must add a bogus cell to this row.  If we don't do this, any
    // cells falling in this row will be skipped, causing text to
    // disappear (at the least, in the worst case it will cause a
    // crash).

    // See how many columns we have (if not already set by the COLS attr.)
    int ncols = 0;
    for (int i = 0; i < table->t_nrows; i++) {
        if (ncols < table->t_rows[i].tr_ncells)
            ncols = table->t_rows[i].tr_ncells;
    }
    if (ncols > table->t_ncols)
        table->t_ncols = ncols;

    // move to current table
    *tcellp = table->t_parent_cell;
    return (table->t_parent);
}


// Add a caption to the given table.
//
void
htmTable::openCaption(htmWidget *html, htmObjectTable *start,
    htmObject *obj, unsigned int *bg, htmImage **bg_image)
{
    htmTable *table = this;
    if (!table->t_parent)
        table = t_children;

    // get caption
    htmTableRow *caption = table->t_caption;

    // only one caption allowed
    if (caption->tr_lastcell)
        return;

    // Get properties for this caption.  The global table properties
    // are never propagated since, officially, a Caption doesn't have
    // any attributes...

    caption->tr_properties = html->tableCheckProperties(obj->attributes,
        0, html->htm_default_halign, *bg, *bg_image);

    // starting object
    caption->tr_start = start;
    caption->tr_owner = start;

    // set parent table
    caption->tr_parent = table;

    // one cell: the caption itself
    caption->tr_cells = new htmTableCell[1];
    caption->tr_ncells   = 1;
    caption->tr_lastcell = 1;

    // fill in used fields
    htmTableCell *cell = caption->tr_cells;

    // get properties for this cell
    cell->tc_properties = html->tableCheckProperties(obj->attributes,
        0, caption->tr_properties->tp_halign, caption->tr_properties->tp_bg,
        caption->tr_properties->tp_bg_image);
    // set return background
    *bg = caption->tr_properties->tp_bg;
    *bg_image = caption->tr_properties->tp_bg_image;

    // starting object
    cell->tc_start = start;
    cell->tc_owner = start;

    // ending object unknown
    cell->tc_end = 0;

    // set parent caption
    cell->tc_parent = caption;
}


// Perform required caption wrapup actions.
//
void
htmTable::closeCaption(htmWidget*, htmObjectTable *end)
{
    htmTable *table = this;
    if (!table->t_parent)
        table = t_children;

    htmTableRow *caption = table->t_caption;

    // sanity
    if (caption->tr_ncells == 0)
        return;

    htmTableCell *cell = caption->tr_cells;

    cell->tc_start = cell->tc_start->next;
    cell->tc_end   = end;
}


// Add a row to the current table.
//
void
htmTable::openRow(htmWidget *html, htmObjectTable *start, htmObject *obj,
    Alignment *halign, unsigned int *bg, htmImage **bg_image)
{
    htmTable *table = this;
    if (!table->t_parent)
        table = t_children;

    // sanity
    if (table->t_lastrow == table->t_nrows)
        html->fatalError("Bad tablerow count!!!");

    // get next available row in this table
    htmTableRow *row = table->t_rows + table->t_lastrow;

    // get properties for this row
    row->tr_properties = html->tableCheckProperties(obj->attributes,
        table->t_properties, html->htm_default_halign, *bg, *bg_image);
    // set return alignment
    *halign = row->tr_properties->tp_halign;

    // set return background
    *bg = row->tr_properties->tp_bg;
    *bg_image = row->tr_properties->tp_bg_image;

    // starting object
    row->tr_start = start;
    row->tr_owner = start;

    // set parent table
    row->tr_parent = table;

    // count how many cells this row has
    int ncells = 0;
    int depth  = 0;
    for (htmObject *tmp = obj->next; tmp; tmp = tmp->next) {
        // check for end of row and child rows (in child tables)
        if (tmp->id == HT_TR) {
            if (tmp->is_end) {
                if (depth == 0)
                    break;
                else
                    depth--;
            }
            else    // new row opens
                depth++;
        }
        // only count a cell when it belongs to the top-level row
        if ((tmp->id == HT_TH || tmp->id == HT_TD) && depth == 0 &&
                !tmp->is_end)
            ncells++;
    }
    // empty rows don't have cells
    if (ncells)
        row->tr_cells = new htmTableCell[ncells];
    else
        // allocate an empty cell
        row->tr_cells = new htmTableCell[1];
    row->tr_ncells   = ncells;
    row->tr_lastcell = 0;

    // move to next available row
    table->t_lastrow++;
}


// Perform required row wrapup actions.
//
void
htmTable::closeRow(htmWidget*, htmObjectTable *end)
{
    htmTable *table = this;
    if (!table->t_parent)
        table = t_children;

    // get current row in this table
    htmTableRow *row = table->t_rows + table->t_lastrow - 1;

    // Count how many columns this row has (including cells spanning
    // multiple columns).

    int ncols = 0;
    for (int i = 0; i < row->tr_ncells; i++)
        ncols += row->tr_cells[i].tc_colspan;

    if (ncols > table->t_ncols)
        table->t_ncols = ncols;

    if (row->tr_start)
        row->tr_start = row->tr_start->next;
    row->tr_end = end;
}


// Add a cell to the current row in the current table.
//
htmTableCell *
htmTable::openCell(htmWidget *html, htmObjectTable *start, htmObject *obj,
    Alignment *halign, unsigned int *bg, htmImage **bg_image)
{
    htmTable *table = this;
    if (!table->t_parent)
        table = t_children;

    // get current row in this table
    htmTableRow *row = table->t_rows + table->t_lastrow - 1;

    // sanity
    if (row->tr_lastcell == row->tr_ncells)
        html->fatalError("Bad table row cell count!!!");

    // get next available cell in this row
    htmTableCell *cell = row->tr_cells + row->tr_lastcell;

    // get cell-specific properties
    cell->tc_header = (obj->id == HT_TH ? true : false);
    cell->tc_width = htmTagCheckNumber(obj->attributes, "width", 0);
    cell->tc_height = htmTagCheckNumber(obj->attributes, "height", 0);
    cell->tc_rowspan = htmTagGetNumber(obj->attributes, "rowspan", 1);
    cell->tc_colspan = htmTagGetNumber(obj->attributes, "colspan", 1);

    // [row/cell]span = 0 : span entire table in requested direction
    if (cell->tc_rowspan <= 0 || cell->tc_rowspan > table->t_nrows)
        cell->tc_rowspan = table->t_nrows;

    // colspan <= 0 gets handled in SetTable when we now how many
    // columns this table has

    // get global properties for this cell
    cell->tc_properties = html->tableCheckProperties(obj->attributes,
        row->tr_properties, row->tr_properties->tp_halign, *bg, *bg_image);
    // set return alignment
    *halign = cell->tc_properties->tp_halign;

    // set return background
    *bg = cell->tc_properties->tp_bg;
    *bg_image = cell->tc_properties->tp_bg_image;

    // starting object
    cell->tc_start = start;
    cell->tc_owner = start;

    // set parent row
    cell->tc_parent = row;

    // move to next available cell
    row->tr_lastcell++;
    return (cell);
}


// Perform required cell wrapup actions.
//
void
htmTable::closeCell(htmWidget*, htmObjectTable *end)
{
    htmTable *table = this;
    if (!table->t_parent)
        table = t_children;

    // get current row in this table
    htmTableRow *row = table->t_rows + table->t_lastrow - 1;

    // get current cell in this row
    htmTableCell *cell = row->tr_cells + row->tr_lastcell - 1;

    if (cell->tc_start)
        cell->tc_start = cell->tc_start->next;
    else
        cell->tc_start = end;
    cell->tc_end = end;
}


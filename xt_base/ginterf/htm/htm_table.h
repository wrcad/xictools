
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
 *   Stephen R. Whiteley  <srw@wrcad.com>
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
 * $Id: htm_table.h,v 1.1 2005/10/02 07:47:58 stevew Exp $
 *-----------------------------------------------------------------------*/

#ifndef HTM_TABLE_H
#define HTM_TABLE_H

struct htmTableRow;

// Values for flags below
#define TP_BG_GIVEN     0x1
#define TP_IMAGE_GIVEN  0x2

// Default Table border width, cell & row spacing.
//
#define HTML_DEFAULT_TABLE_BORDERWIDTH    0
#define HTML_DEFAULT_CELLSPACING          2
#define HTML_DEFAULT_ROWSPACING           2

namespace htm
{
    // Definition of tables
    //
    // Dimensions:
    // positive -> absolute number;
    // negative -> relative number;
    // 0        -> no dimension specified;
    //
    // Each component in a table has a set of core properties.  Properties
    // are inherited from top to bottom and can be overriden.
    //
    // Content containers render the contents of all objects between start
    // (inclusive) and end (exclusive).

    // Possible framing types
    enum TableFraming
    {
        TFRAME_VOID,                // no borders
        TFRAME_ABOVE,               // only top side
        TFRAME_BELOW,               // only bottom side
        TFRAME_LEFT,                // only left side
        TFRAME_RIGHT,               // only right side
        TFRAME_HSIDES,              // top & bottom
        TFRAME_VSIDES,              // left & right
        TFRAME_BOX,                 // all sides
        TFRAME_BORDER               // all sides
    };

    // Possible ruling types
    enum TableRuling
    {
        TRULE_NONE,                 // no rules
        TRULE_GROUPS,               // only colgroups
        TRULE_ROWS,                 // only rows
        TRULE_COLS,                 // only columns
        TRULE_ALL                   // all cells
    };

    extern TableFraming htmGetFraming(const char*, TableFraming);
    extern TableRuling htmGetRuling(const char*, TableRuling);
}

// A cell, can be a header cell or a simple cell.
//
struct htmTableCell
{
    htmTableCell();
    ~htmTableCell();

    bool            tc_header;          // true if a header cell
    int             tc_width;           // suggested cell width
    int             tc_height;          // suggested cell height
    int             tc_rowspan;         // no of rows spanned
    int             tc_colspan;         // no of cells spanned
    htmTableProperties *tc_properties;  // properties for this cell
    htmObjectTable  *tc_start;          // first object to render
    htmObjectTable  *tc_end;            // last object to render
    htmObjectTable  *tc_owner;          // owning object
    htmTableRow     *tc_parent;         // parent of this cell
};

// A row, consistinf of a number of cells.
//
struct htmTableRow
{
    htmTableRow();
    ~htmTableRow();

    htmTableCell    *tr_cells;          // all cells in this row
    int             tr_ncells;          // no of cells in row
    int             tr_lastcell;        // last used cell
    htmTableProperties *tr_properties;  // properties for this row
    htmObjectTable  *tr_start;          // first object to render
    htmObjectTable  *tr_end;            // last object to render
    htmObjectTable  *tr_owner;          // owning object
    htmTable        *tr_parent;         // parent of this row
};

// A table, consisting of a caption and a number of rows The caption
// is a special row:  it has only one cell that stretches across the
// entire table:  itself.
//
struct htmTable
{
    htmTable();
    ~htmTable();

    static void destroy(htmTable *t)
        {
            while (t) {
                htmTable *tx = t;
                t = t->t_next;
                delete tx;
            }
        }

    static htmTable *open(htmTable*, htmWidget*, htmObjectTable*, htmObject*,
        Alignment*, unsigned int*, htmImage**, htmTableCell**);

    htmTable *close(htmWidget*, htmObjectTable*, htmTableCell**);

    void openCaption(htmWidget*, htmObjectTable*, htmObject*, unsigned int*,
        htmImage**);

    void closeCaption(htmWidget*, htmObjectTable*);

    void openRow(htmWidget*, htmObjectTable*, htmObject*, Alignment*,
        unsigned int*, htmImage**);

    void closeRow(htmWidget*, htmObjectTable*);

    htmTableCell *openCell(htmWidget*, htmObjectTable*, htmObject*,
        Alignment*, unsigned int*, htmImage**);

    void closeCell(htmWidget*, htmObjectTable*);

    // overall table properties
    int             t_width;            // suggested table width
    int             t_height;           // suggested table height
    int             t_hmargin;          // horizontal cell margin
    int             t_vmargin;          // vertical cell margin
    int             t_hpadding;         // horizontal cell padding
    int             t_vpadding;         // vertical row padding
    int             t_ncols;            // no of columns
    int             t_ident;            // indentation
    htmTableProperties *t_properties;      // master table properties

    htmTableRow     *t_caption;         // table caption
    htmTableRow     *t_rows;            // all table rows
    int             t_nrows;            // no of rows in table
    int             t_lastrow;          // last used row

    htmTable        *t_parent;          // parent table (for child)
    htmTableCell    *t_parent_cell;     // parent cell (for child)
    htmTable        *t_children;        // table child
    int             t_nchildren;        // no of child tables
    int             t_lastchild;        // last used table

    htmObjectTable  *t_start;           // first object in table
    htmObjectTable  *t_end;             // last object in table

    htmObjectTable  *t_owner;           // owner of this table
    htmTable        *t_next;            // ptr to next table
};

// Properties shared by all table elements.  These are inherited from
// top to bottom and can be overriden by the appropriate tag
// attributes.
//
struct htmTableProperties
{
    htmTableProperties();

    int             tp_border;          // border width, 0 = noborder
    Alignment       tp_halign;          // content horizontal alignment
    Alignment       tp_valign;          // content vertical alignment
    unsigned int    tp_bg;              // content background color
    htmImage        *tp_bg_image;       // content background image
    int             tp_flags;           // record as given
    TableFraming    tp_framing;         // what frame should we use?
    TableRuling     tp_ruling;          // what rules should we draw?
    bool            tp_nowrap;          // don't break lines
    bool            tp_noinline;        // if left or right justification is
                                        // inherited, this flag is set.  This
                                        // prevents text wrapping around the
                                        // table, which is the norm when the
                                        // justification is given explicitly.
};

#endif


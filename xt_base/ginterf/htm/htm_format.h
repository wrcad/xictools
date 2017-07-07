
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
 * $Id: htm_format.h,v 1.2 2008/07/13 06:17:44 stevew Exp $
 *-----------------------------------------------------------------------*/

#ifndef HTM_FORMAT_H
#define HTM_FORMAT_H

#include "htm_callback.h"

#define MAX_NESTED_LISTS 26  // maximum no of nested lists

namespace htm
{
    // Spacing and anchor text data bits
    enum
    {
        TEXT_SPACE_NONE        = 0x0,   // no spacing at all
        TEXT_SPACE_LEAD        = 0x1,   // add a leading space
        TEXT_SPACE_TRAIL       = 0x2,   // add a trailing space
        TEXT_ANCHOR            = 0x4,   // a regular anchor
        TEXT_ANCHOR_INTERN     = 0x8,   // internal anchor flag
        TEXT_IMAGE             = 0x10,  // indicates an image member
        TEXT_FORM              = 0x20,  // indicates a form member
        TEXT_BREAK             = 0x40   // indicates a linebreak
    };

    // Linefeed types
    enum
    {
        LF_NONE         = -1,   // stay on the same line
        LF_DOWN_1       = 0,    // return + move single line downard
        LF_DOWN_2       = 1,    // return + move two lines downward
        LF_ALL          = 2     // return + move baseline fully down
    };

    // HTML4.0 Events
    struct HTEvent
    {
        HTEvent() { type = 0; data = 0; }
        HTEvent(int t, void *d) { type = t; data = d; }

        int         type;           // HTML4.0 event type
        void        *data;          // event user data
    };

    // All possible events that HTML4.0 defines.  Fields are ptrs into
    // HTEvent array.
    //
    struct AllEvents
    {
        AllEvents();

        // Document/Frame specific events
        HTEvent     *onLoad;
        HTEvent     *onUnload;

        // HTML Form specific events
        HTEvent     *onSubmit;
        HTEvent     *onReset;
        HTEvent     *onFocus;
        HTEvent     *onBlur;
        HTEvent     *onSelect;
        HTEvent     *onChange;

        // object events
        HTEvent     *onClick;
        HTEvent     *onDblClick;
        HTEvent     *onMouseDown;
        HTEvent     *onMouseUp;
        HTEvent     *onMouseOver;
        HTEvent     *onMouseMove;
        HTEvent     *onMouseOut;
        HTEvent     *onKeyPress;
        HTEvent     *onKeyDown;
        HTEvent     *onKeyUp;
    };
}

// Definition of an anchor.
//
struct htmAnchor
{
    htmAnchor();
    htmAnchor(htmAnchor&);
    ~htmAnchor();


    static void destroy(htmAnchor *a)
        {
            while (a) {
                htmAnchor *ax = a;
                a = a->next;
                delete ax;
            }
        }

    void parseHref(const char*);

    URLType             url_type;       // url type of anchor
    char                *name;          // name if it's a named anchor
    char                *href;          // referenced URL
    char                *target;        // target spec
    char                *rel;           // possible rel
    char                *rev;           // possible rev
    char                *title;         // possible title
    AllEvents           *events;        // events to be served
    unsigned int        line;           // location of this anchor
    bool                visited;        // true when anchor is visited
    htmAnchor           *next;          // ptr to next anchor
};

// Definition of a word (a word can be plain text, an image, a form
// member or a linebreak).
//
struct htmWord
{
    htmWord();
    int update(int, int, int);
    void set_baseline(int);

    htmRect             area;       // bounding box of word
    int                 ybaseline;  // text baseline
    unsigned int        line;       // line for this word
    htmObjectType       type;       // type of word, used by <pre>,<img>
    char                *word;      // word to display
    int                 len;        // string length of word
    htmFont             *font;      // font to use
    unsigned char       line_data;  // line data (underline/strikeout)
    unsigned char       spacing;    // leading/trailing/nospace allowed
    AllEvents           *events;    // events to be served
    htmImage            *image;     // when this is an image
    htmForm             *form;      // when this is a form element
    htmWord             *base;      // baseline word for a line
    htmWord             *self;      // ptr to itself, for anchor adjustment
    htmObjectTable      *owner;     // owner of this worddata
};

// Definition of formatted HTML elements.
//
struct htmObjectTable
{
    htmObjectTable(htmObject *obj = 0) { reset(obj); }
    ~htmObjectTable();

    static void destroy(htmObjectTable *ot)
        {
            while (ot) {
                htmObjectTable *ox = ot;
                ot = ot->next;
                delete ox;
            }
        }

    void reset(htmObject*);

    htmRect         area;           // bounding box of element
    unsigned int    line;           // starting line number of this object
    unsigned int    id;             // object identifier (anchors only)
    htmObjectType   object_type;    // element type
    char            *text;          // cleaned text
    unsigned char   text_data;      // text/image/anchor data bits
    int             len;            // length of text or width of a rule
    int             y_offset;       // offset for sub/sup, <hr> noshade flag
    int             x_offset;       // additional offset for sub/sup
    htmObject       *object;        // object data (raw text)
    htmAnchor       *anchor;        // ptr to anchor data
    htmWord         *words;         // words to be displayed
    htmForm         *form;          // form data
    htmTable        *table;         // table data
    htmTableCell    *table_cell;    // parent table cell
    int             n_words;        // number of words
    unsigned char   anchor_state;   // anchor selection state identifier
    Alignment       halign;         // horizontal line alignment
    int             linefeed;       // linebreak type
    unsigned        ident;          // xoffset for list indentation
    Marker          marker;         // marker to use in lists
    int             list_level;     // current count of list element
    htmFont         *font;          // font to be used for this object
    unsigned int    fg;             // foreground color for this object
    unsigned int    bg;             // background color for this object
    htmObjectTable  *next;
    htmObjectTable  *prev;
};

// Struct to keep track of everything while formatting.
//
struct htmFormatContext
{
    // For nesting of colors.
    struct colorStack
    {
        colorStack() { color = 0; next = 0; }
        colorStack(unsigned int c, colorStack *n) { color = c; next = n; }

        unsigned long color;
        colorStack *next;
    };

    // For nesting of table background images.
    struct imageStack
    {
        imageStack() { image = 0; next = 0; }
        imageStack(htmImage *i, imageStack *n) { image = i; next = n; }

        htmImage *image;
        imageStack *next;
    };

    // for nesting of alignments
    struct alignStack
    {
        alignStack() { align = HALIGN_NONE; next = 0; }
        alignStack(Alignment a, alignStack *n) { align = a; next = n; }

        Alignment align;
        alignStack *next;
    };

    // for nesting of fonts
    struct fontStack
    {
        fontStack() { size = 0; font = 0; next = 0; }
        fontStack(int s, htmFont *f, fontStack *n)
            { size = s; font = f; next = n; }

        int size;
        htmFont *font;   // ptr to cached font
        fontStack *next;
    };

    // Indentation level, push when entering a table.
    struct identData
    {
        identData() { level = 0; next = 0; }
        identData(int l, identData *n) { level = l; next = n; }

        int level;
        identData *next;
    };

    htmFormatContext();
    ~htmFormatContext();

    void init();
    void clear();
    htmObjectTable *getFormatted();
    htmAnchor *getAnchors();

    void insertElement(htmObjectTable*);

    void setFGColor(unsigned int fg)
        { fg_color_stack->color = fg; }
    void setBGColor(unsigned int bg)
        { bg_color_stack->color = bg; }
    void setBGImage(htmImage *image)
        { image_stack->image = image; }
    void setAlignment(Alignment align)
        { align_stack->align = align; }
    void setFont(htmFont *font, int size)
        { font_stack->font = font;
          font_stack->size = size; }

    void pushFGColor(unsigned int color)
        { fg_color_stack = new colorStack(color, fg_color_stack); }
    void pushBGColor(unsigned int color)
        { bg_color_stack = new colorStack(color, bg_color_stack); }
    void pushBGImage(htmImage *image)
        { image_stack = new imageStack(image, image_stack); }
    void pushAlignment(Alignment align)
        { align_stack = new alignStack(align, align_stack); }
    void pushFont(htmFont *font, int size)
        { font_stack = new fontStack(size, font, font_stack); }
    void pushIdent(int ident)
        { ident_data = new identData(ident, ident_data); }

    unsigned int popFGColor();
    unsigned int popBGColor();
    htmImage *popBGImage();
    Alignment popAlignment();
    htmFont *popFont(int*);
    int popIdent();

    // object list and stacks
    colorStack fg_color_base, *fg_color_stack;
    colorStack bg_color_base, *bg_color_stack;
    imageStack image_base, *image_stack;
    alignStack align_base, *align_stack;
    fontStack font_base, *font_stack;
    identData *ident_data;

    // for text formatting
    int numchars;
    int prev_state;
    bool have_space;

    unsigned int num_elements;
    unsigned int num_anchors;
    htmObjectTable *head;
    htmObjectTable *current;
    htmAnchor *anchor_head;
    htmAnchor *anchor_current;
};

// Driver for html formatting.
//
struct htmFormatManager
{
    // For nesting of ordered and unordered lists.
    struct listStack
    {
        listStack()
            { isindex = false; level = 0; type = HT_UL; marker = MARKER_NONE; }

        bool isindex;       // propagate index numbers?
        int level;          // item number
        htmlEnum type;      // ol or ul, used for custom markers
        Marker marker;      // marker to use
    };

    htmFormatManager(htmWidget*);
    ~htmFormatManager();

    // perform formatting
    void formatObjects(htmObject*);

    // output retrieval
    htmObjectTable *formattedOutput()
        { return (f_cx ? f_cx->getFormatted() : 0); }
    htmAnchor *anchorData()
        { return (f_cx ? f_cx->getAnchors() : 0); }
    int anchorWords()
        { return (f_anchor_words); }
    int numAnchors()
        { return (f_cx ? f_cx->num_anchors : 0); }
    int namedAnchors()
        { return (f_named_anchors); }

    void addAnchor(htmAnchor*);

private:
    void init();
    void find_first_element();
    void setup_element(htmObject*);
    void textToWords();
    bool imageToWord(const char*);
    void allocFormWord(htmForm*);
    bool inputToWord(const char*);
    bool selectToWord(htmObject*);
    bool textAreaToWord(htmObject*);
    void indexToWord();
    bool textToPre();
    void makeDummyWord();
    void setTab(int);
    char *copyText(const char*, bool, unsigned char*, bool);
    void fillBullet();
    int checkLineFeed(int, bool);

    // dispatch methods
    //
    void a_nop();          // HT_DOCTYPE, HT_BASE, HT_BODY, HT_FRAME,
                           // HT_FRAMESET, HT_HEAD, HT_HTML,
                           // HT_ISINDEX, HT_LINK, HT_META,
                           // HT_NOFRAMES, HT_TITLE
    void a_A();            // HT_A
    void a_ADDRESS();      // HT_ADDRESS
    void a_APPLET();       // HT_APPLET
    void a_AREA();         // HT_AREA
    void a_B_etc();        // HT_B, HT_CITE, HT_CODE, HT_DFN, HT_EM,
                           // HT_I, HT_KBD, HT_SAMP, HT_STRONG,
                           // HT_TT, HT_VAR
    void a_BASEFONT();     // HT_BASEFONT
    void a_BIG();          // HT_BIG
    void a_BLOCKQUOTE();   // HT_BLOCKQUOTE
    void a_BR();           // HT_BR
    void a_CAPTION();      // HT_CAPTION
    void a_CENTER();       // HT_CENTER
    void a_DD();           // HT_DD
    void a_DIR_MENU_UL();  // HT_DIR, HT_MENU, HT_UL
    void a_DIV();          // HT_DIV
    void a_DL();           // HT_DL
    void a_DT();           // HT_DT
    void a_FONT();         // HT_FONT
    void a_FORM();         // HT_FORM
    void a_H1_6();         // HT_H1, HT_H2, ..., HT_H6
    void a_HR();           // HT_HR
    void a_IMG();          // HT_IMG
    void a_INPUT();        // HT_INPUT
    void a_LI();           // HT_LI
    void a_MAP();          // HT_MAP
    void a_OL();           // HT_OL
    void a_OPTION();       // HT_OPTION
    void a_P();            // HT_P
    void a_PARAM();        // HT_PARAM
    void a_PRE();          // HT_PRE
    void a_SCRIPT();       // HT_SCRIPT
    void a_SELECT();       // HT_SELECT
    void a_SMALL();        // HT_SMALL
    void a_STRIKE();       // HT_STRIKE
    void a_STYLE();        // HT_STYLE
    void a_SUB_SUP();      // HT_SUB, HT_SUP
    void a_TAB();          // HT_TAB
    void a_TABLE();        // HT_TABLE
    void a_TD_TH();        // HT_TD, HT_TH
    void a_TEXTAREA();     // HT_TEXTAREA
    void a_TR();           // HT_TR
    void a_U();            // HT_U
    void a_ZTEXT();        // HT_ZTEXT

    void (htmFormatManager::*dtable[HT_ZTEXT + 1])();

    htmWidget *f_html;          // widget pointer
    htmObject *f_object;        // current element
    htmFormatContext *f_cx;     // attribute context

    char *f_text;
    int f_linefeed;

    htmWord *f_words;
    int f_n_words;
    int f_anchor_words;
    int f_named_anchors;
    int f_x_offset;
    int f_y_offset;
    unsigned char f_text_data;
    unsigned char f_line_data;
    unsigned long f_element_data;
    htmAnchor *f_anchor_data;
    htmAnchor *f_form_anchor_data;

    int f_ul_level;
    int f_ol_level;
    int f_ident_level;
    int f_current_list;
    int f_in_dt;

    unsigned int f_width;
    unsigned int f_height;
    Alignment f_halign;
    Alignment f_valign;
    htmObjectType f_object_type;
    htmObjectTable *f_element;
    htmObjectTable *f_previous_element;

    unsigned int f_fg;
    unsigned int f_bg;
    htmImage *f_bg_image;
    int f_fontsize;
    int f_basefontsize;
    htmFont *f_font;
    htmFont *f_lastfont;

    htmImageMap *f_imageMap;

    htmTable *f_table;
    htmTable *f_current_table;
    htmTableCell *f_table_cell;

    int f_new_anchors;
    bool f_anchor_data_used;

    bool f_ignore;
    bool f_pre_nl;
    bool f_in_pre;

    listStack f_list_stack[MAX_NESTED_LISTS];
};

#endif


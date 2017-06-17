
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
 * $Id: htm_form.h,v 1.1 2005/10/02 07:47:58 stevew Exp $
 *-----------------------------------------------------------------------*/

#ifndef HTM_FORM_H
#define HTM_FORM_H

#include "htm_callback.h"

namespace htm
{
    // Form method types
    enum
    {
        HTM_FORM_GET,                   // method = get
        HTM_FORM_POST,                  // method = post
        HTM_FORM_PIPE                   // method = pipe
    };

    // Form component types
    enum componentType
    {
        FORM_NONE,                      // undefined
        FORM_TEXT,                      // textfield
        FORM_PASSWD,                    // password textfield
        FORM_CHECK,                     // checkbox
        FORM_RADIO,                     // radiobox
        FORM_RESET,                     // reset button
        FORM_FILE,                      // filelisting
        FORM_SELECT,                    // select parent
        FORM_OPTION,                    // select child
        FORM_TEXTAREA,                  // multiline edit field
        FORM_IMAGE,                     // drawbutton
        FORM_HIDDEN,                    // hidden input
        FORM_SUBMIT                     // submit button
    };
}

// Definition of HTML form components.
//
struct htmForm
{
    htmForm(htmFormData*);
    ~htmForm();
    void free(htmWidget*);

    char            *name;          // name for this widget
    componentType   type;           // widget type
    unsigned int    width;          // widget width
    unsigned int    height;         // widget height
    int             size;           // cols in text(field)/items in select
    int             maxlength;      // max chars to enter/rows in textarea
    char            *value;         // default text
    char            *content;       // entered text(field) contents
    Alignment       align;          // image/file browse button position
    bool            multiple;       // multiple select flag
    bool            selected;       // default state
    bool            checked;        // check value for option/radio buttons
    bool            mapped;         // true when displayed, false otherwise
    htmForm         *options;       // option items for select
    htmObjectTable  *data;          // owning data object
    htmFormData     *parent;        // parent form
    void            *widget;        // ptr to widget
    htmForm         *prev;          // ptr to previous record
    htmForm         *next;          // ptr to next record
};

// Definition of form data.
//
struct htmFormData
{
    htmFormData(htmWidget*);
    ~htmFormData();
    void free();

    htmWidget       *html;          // owner of this form
    char            *action;        // destination url/cgi-bin
    int             method;         // HTM_FORM_GET,POST,PIPE
    char            *enctype;       // form encoding
    int             ncomponents;    // no of items in this form
    htmForm         *components;    // list of form items
    htmFormData     *prev;          // ptr to previous form
    htmFormData     *next;          // ptr to next form
};

// Form Component data
struct htmFormDataRec
{
    htmFormDataRec()
        {
            type        = FORM_NONE;
            name        = 0;
            value       = 0;
        }

    ~htmFormDataRec()
        {
            if (type == FORM_IMAGE)
                delete [] name;
            delete [] value;
        }

    componentType   type;           // Form component type
    const char      *name;          // component name
    const char      *value;         // component value
};

// Callback structure.
struct htmFormCallbackStruct
{
    htmFormCallbackStruct()
        {
            reason          = HTM_FORM;
            event           = 0;
            action          = 0;
            enctype         = 0;
            method          = 0;
            ncomponents     = 0;
            components      = 0;
        }

    int reason;             // the reason the callback was called
    htmEvent *event;        // event structure that triggered callback
    const char *action;     // URL or cgi-bin destination
    const char *enctype;    // form encoding
    int method;             // Form Method, GET, POST or PIPE
    int ncomponents;        // no of components in this form
    htmFormDataRec *components;
};

#endif


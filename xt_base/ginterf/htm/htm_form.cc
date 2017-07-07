
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
 * $Id: htm_form.cc,v 1.13 2014/02/15 23:14:17 stevew Exp $
 *-----------------------------------------------------------------------*/

#include "htm_widget.h"
#include "htm_form.h"
#include "htm_format.h"
#include "htm_string.h"
#include "htm_tag.h"
#include "htm_callback.h"
#include <stdio.h>
#include <string.h>

namespace { componentType getInputType(const char*); }

htmForm::htmForm(htmFormData *owner)
{
    name        = 0;
    type        = FORM_NONE;
    width       = 0;
    height      = 0;
    size        = 0;
    maxlength   = 0;
    value       = 0;
    content     = 0;
    align       = HALIGN_NONE;
    multiple    = false;
    selected    = false;
    checked     = false;
    mapped      = false;
    options     = 0;
    data        = 0;
    parent      = owner;
    widget      = 0;
    prev        = 0;
    next        = 0;
}


htmForm::~htmForm()
{
    delete [] name;
    delete [] value;
    delete [] content;
    htmForm::destroy(options, 0);
}
// End of htmForm functions.


htmFormData::htmFormData(htmWidget *w)
{
    html            = w;
    action          = 0;
    method          = 0;
    enctype         = 0;
    ncomponents     = 0;
    components      = 0;
    prev            = 0;
    next            = 0;
}


htmFormData::~htmFormData()
{
    delete [] action;
    delete [] enctype;
    htmForm::destroy(components, html);
}
// End of htmFormData functions.


void
htmWidget::destroyForms()
{
    // Destroy any form data and zero pointers.
    htmFormData::destroy(htm_form_data);
    htm_form_data = 0;
    htm_current_form = 0;
    htm_current_entry = 0;
}


void
htmWidget::startForm(const char *attributes)
{
    // attributets can be null!

    // allocate a new entry
    htmFormData *form = new htmFormData(this);

    // this form starts a new set of entries
    htm_current_entry = 0;

    // pick up action
    if ((form->action = htmTagGetValue(attributes, "action")) == 0)
        form->action = lstring::copy("no_action");

    // default method is get
    form->method = HTM_FORM_GET;
    {
        char *method = htmTagGetValue(attributes, "method");
        if (method != 0) {
            if (!strncasecmp(method, "get", 3))
                form->method = HTM_FORM_GET;
            else if (!strncasecmp(method, "post", 4))
                form->method = HTM_FORM_POST;
            else if (!strncasecmp(method, "pipe", 4))
                form->method = HTM_FORM_PIPE;
            delete [] method;
        }
    }

    // form encoding
    if ((form->enctype = htmTagGetValue(attributes, "enctype")) == 0)
        form->enctype = lstring::copy("application/x-www-form-urlencoded");

    if (htm_form_data) {
        form->prev = htm_current_form;
        htm_current_form->next = form;
        htm_current_form = form;
    }
    else
        htm_form_data = htm_current_form = form;
}


// Invalidates the current parent form.
//
void
htmWidget::endForm()
{
    htm_current_entry = 0;
}


htmForm*
htmWidget::formAddInput(const char *attributes)
{
    if (attributes == 0)
        return (0);

    if (htm_current_form == 0) {
        warning("formAddInput",
            "Bad HTML form: <INPUT> not within form.");
        return (0);
    }

    // Create and initialise a new entry
    htmForm *entry = new htmForm(htm_current_form);

    entry->type = getInputType(attributes);

    // get name
    const char *chPtr = 0;
    if ((entry->name = htmTagGetValue(attributes, "name")) == 0) {
        switch (entry->type) {
        case FORM_TEXT:
            chPtr = "Text";
            break;
        case FORM_PASSWD:
            chPtr = "Password";
            break;
        case FORM_CHECK:
            chPtr = "CheckBox";
            break;
        case FORM_RADIO:
            chPtr = "RadioBox";
            break;
        case FORM_RESET:
            chPtr = "Reset";
            break;
        case FORM_FILE:
            chPtr = "File";
            break;
        case FORM_IMAGE:
            chPtr = "Image";
            break;
        case FORM_HIDDEN:
            chPtr = "Hidden";
            break;
        case FORM_SUBMIT:
            chPtr = "Submit";
            break;
        default:
            delete entry;
            return (0);
        }
        entry->name = lstring::copy(chPtr);
    }

    char *t = htmTagGetValue(attributes, "value");
    entry->value = expandEscapes(t, false, 0);
    delete [] t;
    entry->checked = htmTagCheck(attributes, "checked");
    entry->selected = entry->checked;   // save default state

    if (entry->type == FORM_TEXT || entry->type == FORM_PASSWD) {
        // default to 25 columns if size hasn't been specified
        entry->size = htmTagGetNumber(attributes, "size", 25);

        // unlimited amount of text input if not specified
        entry->maxlength = htmTagGetNumber(attributes, "maxlength", -1);

        // passwd can't have a default value
        if (entry->type == FORM_PASSWD && entry->value) {
            delete [] entry->value;
            entry->value = 0;
        }
        // empty value if none given
        if (entry->value == 0) {
            entry->value = new char[1];
            entry->value[0] = 0;
        }
    }
    else if (entry->type == FORM_FILE) {
        // default to 20 columns if size hasn't been specified
        entry->size = htmTagGetNumber(attributes, "size", 20);

        // check is we are to support multiple selections
        entry->multiple = htmTagCheck(attributes, "multiple");

        // any dirmask to use?
        entry->value   = htmTagGetValue(attributes, "value");
        entry->content = htmTagGetValue(attributes, "src");
    }
    entry->align = htmGetImageAlignment(attributes);

    // go create the actual widget
    // As image buttons are promoted to image words we don't deal with
    // the FORM_IMAGE case.  For hidden form fields nothing needs to
    // be done.

    htm_tk->tk_add_widget(entry);

    if (htm_current_entry) {
        entry->prev = htm_current_entry;
        htm_current_entry->next = entry;
        htm_current_entry = entry;
    }
    else {
        htm_current_form->components = entry;
        htm_current_entry = entry;
    }
    htm_current_form->ncomponents++;

    // all done
    return (entry);
}


htmForm*
htmWidget::formAddSelect(const char *attributes)
{
    if (attributes == 0)
        return (0);

    if (htm_current_form == 0) {
        // too bad, ignore
        warning("formAddSelect",
            "Bad HTML form: <SELECT> not within form.");
        return (0);
    }

    // Create and initialise a new entry
    htmForm *entry = new htmForm(htm_current_form);

    // form type
    entry->type = FORM_SELECT;

    // get name
    if ((entry->name = htmTagGetValue(attributes, "name")) == 0)
        entry->name = lstring::copy("Select");

    // no of visible items in list
    entry->size = htmTagGetNumber(attributes, "size", 1);

    // multiple select?
    entry->multiple = htmTagCheck(attributes, "multiple");

    htm_tk->tk_add_widget(entry);

    // this will be used to keep track of inserted menu items
    entry->next = 0;

    return (entry);
}


void
htmWidget::formSelectAddOption(htmForm *entry, const char *attributes,
    const char *label)
{
    // Create and initialise a new entry
    htmForm *item = new htmForm(0);

    // form type
    item->type = FORM_OPTION;
    item->name = lstring::copy(label);

    // value, use id if none given
    if ((item->value = htmTagGetValue(attributes, "value")) == 0) {
        char dummy[32];  // 2^32 possible entries...
        sprintf(dummy, "%d", entry->maxlength);
        item->value = lstring::copy(dummy);
    }

    // initial state
    item->selected = htmTagCheck(attributes, "selected");

    // insert item, entry->next contains ptr to last inserted option
    if (entry->options) {
        htmForm *ftmp = entry->options;
        while (ftmp->next)
            ftmp = ftmp->next;
        ftmp->next = item;
    }
    else
        entry->options = item;

    htm_tk->tk_add_widget(item, entry);
    entry->maxlength++;
}


void
htmWidget::formSelectClose(htmForm *entry)
{
    htm_tk->tk_select_close(entry);

    if (htm_current_entry) {
        entry->prev = htm_current_entry;
        htm_current_entry->next = entry;
        htm_current_entry = entry;
    }
    else {
        htm_current_form->components = entry;
        htm_current_entry = entry;
    }
    htm_current_form->ncomponents++;
}


// Create a form <textarea> entry.
//
htmForm*
htmWidget::formAddTextArea(const char *attributes, const char *text)
{
    // these are required
    if (attributes == 0)
        return (0);

    // sanity, we must have a parent form
    if (htm_current_form == 0) {
        warning("formAddTextArea",
            "Bad HTML form: <TEXTAREA> not within form.");
        return (0);
    }

    // get form name. Mandatory so spit out an error if not found
    char *name;
    if ((name = htmTagGetValue(attributes, "name")) == 0) {
        warning("formAddTextArea",
            "Bad <TEXTAREA>: missing name attribute.");
        return (0);
    }

    // get form dimensions. Mandatory so spit out an error if not found
    int rows = htmTagGetNumber(attributes, "rows", 0);
    int cols = htmTagGetNumber(attributes, "cols", 0);
    if (rows <= 0 || cols <= 0)
        warning("formAddTextArea",
            "Bad <TEXTAREA>: invalid or missing ROWS and/or COLS attribute.");

    // Create and initialise a new entry
    htmForm *entry = new htmForm(htm_current_form);

    // fill in appropriate fields
    entry->name      = name;
    entry->type      = FORM_TEXTAREA;
    entry->size      = cols;
    entry->maxlength = rows;
    entry->value = lstring::copy(text);

    htm_tk->tk_add_widget(entry);

    if (htm_current_entry) {
        entry->prev = htm_current_entry;
        htm_current_entry->next = entry;
        htm_current_entry = entry;
    }
    else {
        htm_current_form->components = entry;
        htm_current_entry = entry;
    }
    htm_current_form->ncomponents++;

    // all done!
    return (entry);
}


void
htmWidget::formActivate(htmEvent *event, htmForm *comp)
{
    if (!htm_if)
        return;

    // Count the elements that might be returned, for array size.
    int count = 0;
    for (htmForm *entry = comp->parent->components; entry;
            entry = entry->next) {

        switch (entry->type) {
        case FORM_SELECT:
            if (entry->multiple || entry->size > 1) {
                for (htmForm *opt = entry->options; opt; opt = opt->next)
                    count++;
            }
            else
                count++;
            break;

        case FORM_IMAGE:
            count++;
            // fall thru
        case FORM_CHECK:
        case FORM_RADIO:
        case FORM_RESET:
        case FORM_SUBMIT:
        case FORM_PASSWD:
        case FORM_TEXT:
        case FORM_FILE:
        case FORM_TEXTAREA:
        case FORM_HIDDEN:
            count++;
            break;

        default:
            break;
        }
    }
    htmFormDataRec *components = new htmFormDataRec[count];

    int ncomp = 0;
    for (htmForm *entry = comp->parent->components; entry;
            entry = entry->next) {
        // default settings for this entry. Overridden when required below
        components[ncomp].type = entry->type;
        components[ncomp].name = entry->name;

        char *chPtr;
        switch (entry->type) {
        case FORM_SELECT:

            // Option menu, get value of selected item (size check
            // required as multiple is false for list boxes offering a
            // single entry).

            htm_tk->tk_get_checked(entry);
            if (entry->multiple || entry->size > 1) {
                for (htmForm *opt = entry->options; opt; opt = opt->next) {
                    if (opt->checked) {
                        components[ncomp].name  = entry->name;
                        components[ncomp].value = lstring::copy(opt->value);
                        ncomp++;
                    }
                }
            }
            else {
                // At most one item checked.
                htmForm *opt;
                for (opt = entry->options; opt && !opt->checked;
                    opt = opt->next) ;

                if (opt) {
                    components[ncomp].name  = entry->name;
                    components[ncomp].type  = FORM_OPTION;  // override
                    components[ncomp].value = lstring::copy(opt->value);
                    ncomp++;
                }
            }
            break;

        case FORM_PASSWD:
        case FORM_TEXT:
        case FORM_FILE:
        case FORM_TEXTAREA:
            chPtr = htm_tk->tk_get_text(entry);
            if (chPtr && *chPtr)
                components[ncomp++].value = chPtr;
            break;

        // check/radio boxes are equal in here
        case FORM_CHECK:
        case FORM_RADIO:
            if (htm_tk->tk_get_checked(entry))
                components[ncomp++].value = lstring::copy(entry->value);
            break;

        case FORM_IMAGE:
            if (comp == entry) {
                char *xname = new char[strlen(entry->name)+3];
                strcpy(xname, entry->name);
                strcat(xname,".x");
                char *yname = new char[strlen(entry->name)+3];
                strcpy(yname, entry->name);
                strcat(yname,".y");

                char *xloc = new char[16];
                char *yloc = new char[16];
                sprintf(xloc, "%d", htm_press_x - comp->data->area.x);
                sprintf(yloc, "%d", htm_press_y - comp->data->area.y);

                components[ncomp].name  = xname;    // override
                components[ncomp].value = xloc;
                ncomp++;
                components[ncomp].name  = yname;    // override
                components[ncomp].value = yloc;
                ncomp++;
            }
            break;

        // always return these
        case FORM_HIDDEN:
            components[ncomp++].value = lstring::copy(entry->value);
            break;

        // reset and submit are equal in here
        case FORM_RESET:
        case FORM_SUBMIT:
            if (comp == entry)
                components[ncomp++].value = lstring::copy(entry->value);
            break;

        case FORM_OPTION:
            // is a wrapper, so doesn't do anything
            break;

        default:
            break;
        }
    }

    htmFormCallbackStruct cbs;
    cbs.event       = event;
    cbs.action      = lstring::copy(comp->parent->action);
    cbs.method      = comp->parent->method;
    cbs.enctype     = lstring::copy(comp->parent->enctype);
    cbs.ncomponents = ncomp;
    cbs.components  = components;

    htm_if->emit_signal(S_FORM, &cbs);

    delete [] cbs.action;
    delete [] cbs.enctype;
    delete [] components;
}


void
htmWidget::formReset(htmForm *entry)
{
    htmFormData *form = entry->parent;
    for (htmForm *tmp = form->components; tmp != 0; tmp = tmp->next) {

        switch (tmp->type) {
        case FORM_PASSWD:
            // passwd doesn't have a default value, clear it
            htm_tk->tk_set_text(tmp, "");
            break;

        case FORM_TEXT:
            htm_tk->tk_set_text(tmp, tmp->value);
            break;

        case FORM_TEXTAREA:
            htm_tk->tk_set_text(tmp, tmp->value ? tmp->value : "");
            break;

        case FORM_CHECK:
        case FORM_RADIO:
            // checkbuttons, set default state
            htm_tk->tk_set_checked(tmp);
            break;

        case FORM_FILE:
            // clear selection
            htm_tk->tk_set_text(tmp, "");
            break;

        case FORM_SELECT:
            htm_tk->tk_set_checked(tmp);
            break;

        default:
            break;
        }
    }
}


void
htmWidget::positionAndShowForms()
{
    for (htmFormData *form = htm_form_data; form; form = form->next) {
        for (htmForm *entry = form->components; entry; entry = entry->next) {
            htm_tk->tk_position_and_show(entry, false);
            htm_tk->tk_position_and_show(entry, true);
        }
    }
}


namespace {
    // Retrieve the type of an <input> HTML form member.  Returns
    // componenttype if ``type'' is present in attributes.  FORM_TEXT is
    // returned if type is not present or type is invalid/misspelled.
    //
    componentType
    getInputType(const char *attributes)
    {
        // if type isn't specified we default to a textfield
        componentType ret_val = FORM_TEXT;
        char *chPtr;
        if ((chPtr = htmTagGetValue(attributes, "type")) == 0)
            return (ret_val);

        if (!(strcasecmp(chPtr, "text")))
            ret_val = FORM_TEXT;
        else if (!(strcasecmp(chPtr, "password")))
            ret_val = FORM_PASSWD;
        else if (!(strcasecmp(chPtr, "checkbox")))
            ret_val = FORM_CHECK;
        else if (!(strcasecmp(chPtr, "radio")))
            ret_val = FORM_RADIO;
        else if (!(strcasecmp(chPtr, "submit")))
            ret_val = FORM_SUBMIT;
        else if (!(strcasecmp(chPtr, "reset")))
            ret_val = FORM_RESET;
        else if (!(strcasecmp(chPtr, "file")))
            ret_val = FORM_FILE;
        else if (!(strcasecmp(chPtr, "hidden")))
            ret_val = FORM_HIDDEN;
        else if (!(strcasecmp(chPtr, "image")))
            ret_val = FORM_IMAGE;
        delete [] chPtr;
        return (ret_val);
    }
}


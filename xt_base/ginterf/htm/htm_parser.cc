
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
 * $Id: htm_parser.cc,v 1.13 2015/06/17 18:36:11 stevew Exp $
 *-----------------------------------------------------------------------*/

#include "htm_widget.h"
#include "htm_parser.h"
#include "htm_string.h"
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>

namespace htm
{
    // HTML Element names.
    // This list is alphabetically sorted to speed up the searching process.
    // DO NOT MODIFY  (must match the HT_* enum)
    //
    const char *const html_tokens[] =
    {
        "!doctype", "a", "address", "applet", "area", "b", "base",
        "basefont", "big", "blockquote", "body", "br", "caption", "center",
        "cite", "code", "dd", "dfn", "dir", "div", "dl", "dt", "em", "font",
        "form", "frame", "frameset", "h1", "h2", "h3", "h4", "h5", "h6",
        "head", "hr", "html", "i", "img", "input", "isindex", "kbd", "li",
        "link", "map", "menu", "meta", "noframes", "ol", "option", "p",
        "param", "pre", "samp", "script", "select", "small", "strike",
        "strong", "style", "sub", "sup", "tab", "table", "td", "textarea",
        "th", "title", "tr", "tt", "u", "ul", "var", "plain text"
    };

    // values for BadHTMLWarnings
    enum
    {
        HTM_NONE,                   // no warnings
        HTM_UNKNOWN_ELEMENT,
        HTM_BAD = 2,
        HTM_OPEN_BLOCK = 4,
        HTM_CLOSE_BLOCK = 8,
        HTM_OPEN_ELEMENT = 16,
        HTM_NESTED = 32,
        HTM_VIOLATION = 64,
        HTM_ALL = 127               // all warnings
    };
}


namespace {
    // Check if the given element has a terminating counterpart.
    //
    inline bool
    IsElementTerminated(htmlEnum id)
    {
        switch (id) {
        // Elements that are never terminated
        case HT_AREA:
        case HT_BASE:
        case HT_BASEFONT:
        case HT_BR:
        case HT_DOCTYPE:
        case HT_FRAME:
        case HT_HR:
        case HT_IMG:
        case HT_INPUT:
        case HT_ISINDEX:
        case HT_LINK:
        case HT_META:
        case HT_TAB:
        case HT_ZTEXT:
            return (false);
        default:
            break;
        }

        // all other elements are terminated
        return (true);
    }


    // Check whether the given id is allowed to appear inside the <BODY>
    // tag.
    //
    inline bool
    IsBodyElement(htmlEnum id)
    {
        switch (id) {
        // all but these belong inside a <body></body> tag
        case HT_DOCTYPE:
        case HT_BASE:
        case HT_HTML:
        case HT_HEAD:
        case HT_LINK:
        case HT_META:
        case HT_STYLE:
        case HT_TITLE:
        case HT_FRAMESET:
        case HT_FRAME:
        case HT_NOFRAMES:
        case HT_SCRIPT:
        case HT_ZTEXT:
            return (false);
        default:
            break;
        }
        return (true);
    }
}


//-----------------------------------------------------------------------------
// Widget methods

// Parse the text in htm_source and place the output in htm_elements.
//
void
htmWidget::parseInput()
{
    // clear existing elements
    htm_elements->free();
    htm_elements = 0;

    if (!htm_source || !*htm_source)
        return;
    const char *text = htm_source;

    int loop_count = 0;
    bool redo = false;
    do {
        bool bad_html = false;
        bool html32 = true;
        htmObject *output = 0;
        {
            htmParser parser(this);
            parser.setSource(text);
            parser.setWarnings(htm_bad_html_warnings ? HTM_ALL : HTM_NONE);
            output = parser.parse(htm_mime_type);
            bad_html = parser.get_bad_html();
            html32   = parser.get_html32();
        }
        if (!output) {
            if (loop_count)
                delete [] text;
            return;
        }
        htm_elements->free();
        htm_elements = output;

        // If the state stack was unbalanced, check if the
        // verification/repair routines produced a balanced parser
        // tree.
        //
        htmObject *checked = 0;
        if (bad_html) {
            htmParser p(this);
            checked = (p.verifyVerify(output));
        }

        // If we have a document callback, call it now.  If we don't
        // have one and verifyVerification failed, we iterate again on
        // the current document.  The verify stuff mimics the default
        // DocumentCallback behaviour, that is, advise another pass if
        // the parser tree is unbalanced.  The reason for this is
        // that, although a document may have yielded an unbalanced
        // state stack, the parser tree is properly balanced and hence
        // suitable for usage.

        if (loop_count)
            delete [] text;
        text = 0;

        redo = documentCallback(html32, !bad_html, (checked == 0), false,
            loop_count);
        if (!redo && loop_count < 2 && checked)
            redo = true;
        if (redo)
            // pull new source out of the parser
            text = getObjectString();
        loop_count++;
    }
    while (redo);
}


// Initialize the mime_id field from the mime_type.
//
void
htmWidget::setMimeId()
{
    if (!htm_mime_type)
        htm_mime_type = lstring::copy("text/html");
    if (!(strcasecmp(htm_mime_type, "text/html")))
        htm_mime_id = MIME_HTML;
    else if (!(strcasecmp(htm_mime_type, "text/html-perfect")))
        htm_mime_id = MIME_HTML;
    else if (!(strcasecmp(htm_mime_type, "text/plain")))
        htm_mime_id = MIME_PLAIN;
    else if (!(strncasecmp(htm_mime_type, "image/", 6)))
        htm_mime_id = MIME_IMAGE;
    else {
        // something strange
        delete [] htm_mime_type;
        htm_mime_type = lstring::copy("text/html");
        htm_mime_id = MIME_HTML;
    }
}


// Create an HTML source document from the parser tree.  Document
// created in a buffer.  This buffer must be freed by the caller.
//
char *
htmWidget::getObjectString()
{
    if (!htm_elements)
        return (0);

    int *sizes = new int[HT_ZTEXT*sizeof(int)];
    for (int i = 0; i < HT_ZTEXT; i++)
        sizes[i] = strlen(html_tokens[i]);

    // first pass, compute length of buffer to allocate
    int size = 0;
    for (htmObject *tmp = htm_elements; tmp; tmp = tmp->next) {
        if (tmp->id != HT_ZTEXT) {
            if (tmp->is_end)
                size++;  // a /

            // a pair of <> + element length
            size += 2 + sizes[tmp->id];

            // a space and the attributes
            if (tmp->attributes)
                size += 1 + strlen(tmp->attributes);
        }
        else
            size += strlen(tmp->element);
    }
    size++;   // terminating character

    char *buffer = new char[size * sizeof(char)];
    char *chPtr = buffer;

    // second pass, compose the text
    for (htmObject *tmp = htm_elements; tmp; tmp = tmp->next) {
        if (tmp->id != HT_ZTEXT) {
            *chPtr++ = '<';
            if (tmp->is_end)
                *chPtr++ = '/';
            strcpy(chPtr, html_tokens[tmp->id]);
            chPtr += sizes[(int)tmp->id];

            // a space and the attributes
            if (tmp->attributes) {
                *chPtr++ = ' ';
                strcpy(chPtr, tmp->attributes);
                chPtr += strlen(tmp->attributes);
            }
            *chPtr++ = '>';
        }
        else {
            strcpy(chPtr, tmp->element);
            chPtr += strlen(tmp->element);
        }
    }
    *chPtr = 0;  // 0 terminate
    delete [] sizes;
    return (buffer);
}


//-----------------------------------------------------------------------------
// Some useful inlines

namespace {
    inline bool
    IsOptionalClosure(htmlEnum id)
    {
        switch (id) {
        // these don't require a closing tab
        case HT_DD:
        case HT_DT:
        case HT_LI:
        case HT_P:
        case HT_OPTION:
        case HT_TD:
        case HT_TH:
        case HT_TR:
            return (true);
        default:
            break;
        }
        return (false);
    }


    inline bool
    IsMarkup(htmlEnum id)
    {
        switch (id) {
        // these are physical/logical markup elements
        case HT_TT:
        case HT_I:
        case HT_B:
        case HT_U:
        case HT_STRIKE:
        case HT_BIG:
        case HT_SMALL:
        case HT_SUB:
        case HT_SUP:
        case HT_EM:
        case HT_STRONG:
        case HT_DFN:
        case HT_CODE:
        case HT_SAMP:
        case HT_KBD:
        case HT_VAR:
        case HT_CITE:
        case HT_FONT:
            return (true);
        default:
            break;
        }
        return (false);
    }


    inline bool
    IsContainer(htmlEnum id)
    {
        switch (id) {
        // these are text containers
        case HT_BODY:
        case HT_DIV:
        case HT_CENTER:
        case HT_BLOCKQUOTE:
        case HT_FORM:
        case HT_TH:
        case HT_TD:
        case HT_DD:
        case HT_LI:
        case HT_NOFRAMES:
            return (true);
        default:
            break;
        }
        return (false);
    }


    inline bool
    IsNestableElement(htmlEnum id)
    {
        if (IsMarkup(id))
            return (true);

        switch (id) {
        // these are elements that may be nested
        case HT_APPLET:
        case HT_BLOCKQUOTE:
        case HT_DIV:
        case HT_CENTER:
        case HT_FRAMESET:
            return (true);
        default:
            break;
        }
        return (false);
    }


    inline bool
    IsMiscElement(htmlEnum id)
    {
        switch (id) {
        // these are other elements
        case HT_P:
        case HT_H1:
        case HT_H2:
        case HT_H3:
        case HT_H4:
        case HT_H5:
        case HT_H6:
        case HT_PRE:
        case HT_ADDRESS:
        case HT_APPLET:
        case HT_CAPTION:
        case HT_A:
        case HT_DT:
            return (true);
        default:
            break;
        }
        return (false);
    }
}


//-----------------------------------------------------------------------------
// Parser methods.

htmParser::htmParser(htmWidget*)
{
    p_html = 0;
    p_source = 0;
    p_len = 0;
    p_index = 0;
    p_num_lines = 1;
    p_line_len = 0;
    p_cnt = 0;
    p_num_elements = 1;
    p_num_text = 0;
    p_head = newObject(HT_ZTEXT, 0, 0, false, false);
    p_current = p_head;
    p_state_stack = &p_state_base;
    p_cstart = 0;
    p_cend = 0;
    p_inserted = 0;
    p_err_count = 0;
    p_loop_count = 0;
    p_have_body = 0;

    p_bad_html = false;
    p_html32 = false;

    p_warn = 0;
    p_directionRtoL = false;
}


htmParser::~htmParser()
{
    delete [] p_source;
    p_head->free();
}


// Copy the input text to an internal buffer.
//
void
htmParser::setSource(const char *input)
{
    if (!input)
        input = "";
    delete [] p_source;
    p_len    = strlen(input);
    p_source = htm_strndup(input, p_len);
}


// Parse the input, return the resulting object list.
//
htmObject *
htmParser::parse(const char *mime_type)
{
    if (!mime_type || !strcasecmp(mime_type, "text/html"))
        parseHTML();
    else if (!strcasecmp(mime_type, "text/html-perfect"))
        parsePerfectHTML();
    else if (!strcasecmp(mime_type, "text/plain"))
        parsePLAIN();
    else if (!strncasecmp(mime_type, "image/", 6))
        parseIMAGE();
    else
        parseHTML();

    // Set return list. The first element in the list is a dummy one.
    htmObject *list_return = p_head->next;
    p_head->next = 0;
    if (list_return)
        list_return->prev = 0;
    p_current = p_head;
    return (list_return);
}


// Checks whether the document verification/repair routines did a
// proper job.
//
htmObject *
htmParser::verifyVerify(htmObject *objects)
{

    // walk to the first terminated item in the list
    htmObject *tmp = objects;
    while (tmp && !tmp->terminated)
        tmp = tmp->next;

    // reset state stack
    htmlEnum curr = tmp->id;
    p_state_stack->id = curr;

    if (tmp) {
        for (tmp = tmp->next; tmp; tmp = tmp->next) {
            if (tmp->terminated) {
                if (tmp->is_end) {
                    if (curr != tmp->id)
                        break;
                    curr = popState();
                }
                else {
                    pushState(curr);
                    curr = tmp->id;
                }
            }
        }
    }
    // clear the stack
    while (p_state_stack->next != 0)
        curr = popState();

    return (tmp);
}


// Create a doubly linked list of all elements (both HTML and plain
// text).
//
void
htmParser::parseHTML()
{
    // we assume all documents are valid
    p_bad_html = false;
    // and that every document is HTML 3.2 conforming
    p_html32 = true;

    // start scanning
    char *chPtr = p_source;
    p_num_lines = 1;  // every editor starts its linecount at 1
    p_cstart    = 0;
    p_cend      = 0;
    p_line_len  = 0;
    char *start = 0;
    unsigned int line_len = 0;
    int text_len = 0;
    char *text_start = chPtr;

    int cnt = 0;
    int on_stack = 0;
    bool done = false;
    while (*chPtr) {
        if (*chPtr == '<') {
            // start of element
            // see if we have any text pending
            if (text_len && text_start != 0) {
                storeTextElement(text_start, chPtr);
                text_start = 0;
                text_len = 0;
            }
            // move past element starter
            start = chPtr+1; // element text starts here
            done = false;
            // absolute starting position for this element
            p_cstart = start - p_source;

            // Scan until the end of this tag.  Comments are removed
            // properly.  The behavior is undefined when a comment is
            // placed inside a tag.

            while (*chPtr != '\0' && !done) {
                chPtr++;
                switch (*chPtr) {
                case '!':
                    // either a comment or the !doctype
                    if ((*(chPtr+1) == '>' || *(chPtr+1) == '-')) {
                        chPtr = cutComment(chPtr);
                        // back up one element
                        chPtr--;
                        start = chPtr;
                        done = true;
                    }
                    break;

                    // Quotes should be balanced, so we look ahead and
                    // see if we can find a closing > after the
                    // closing quote.  Anything can appear within
                    // these quotes so we break out of it once we find
                    // either < or /> inside the quote or > after the
                    // closing quote.

                case '\"':
                {
                    // first look ahead for a corresponding "
                    char *tmpE, *tmpS = chPtr;

                    chPtr++;    // move past it
                    for ( ; *chPtr && *chPtr != '\"' && *chPtr != '>';
                        p_num_lines += *chPtr++ == '\n' ? 1 : 0) ;

                    // we found one
                    if (!*chPtr || *chPtr == '\"')
                        break;

                    // It's unbalanced, check if we run into one
                    // before we see a <, save position first.

                    tmpE = chPtr;
                    for (; *chPtr && *chPtr != '\"' && *chPtr != '<';
                        p_num_lines += *chPtr++ == '\n' ? 1 : 0) ;

                    // we found one
                    if (!*chPtr || *chPtr == '\"')
                        break;

                    // If we get here it means the element didn't have
                    // a closing quote.  Spit out a warning and
                    // restore saved position.

                    if (p_warn != HTM_NONE) {
                        int len = chPtr - tmpS;

                        // no overruns
                        char *msg = htm_strndup(tmpS, (len < 128 ? len : 128));

                        warning("parseHTML", "%s: badly placed or "
                            "missing quote\n    (line %i in "
                            "input).", msg, p_num_lines);
                        delete [] msg;
                    }
                    chPtr = tmpE;
                    // fall thru
                }

                case '>':
                    // go and store the element
                    chPtr = storeElement(start, chPtr);
                    done = true;
                    break;
                case '/':
                    // Only handle shorttags when requested.  We have
                    // a short tag if this / is preceeded by a valid
                    // character.

                    // SGML shorttag handling.  We use a buffer of a
                    // fixed length for storing an encountered token.
                    // We can do this safely since SGML shorttag's may
                    // never contain any attributes whatsoever.  The
                    // fixed buffer does include some room for leading
                    // whitespace though.

                    if (isalnum(*(chPtr-1))) {
                        char token[16], *ptr;
                        int id;

                        // Check if text between opening < and this
                        // first / is a valid html tag.  Opening
                        // NULL-end tags must always be attached to
                        // the tag itself.

                        if (chPtr - start > 15 || isspace(*(chPtr-1)))
                            break;
                        // copy text
                        strncpy(token, start, chPtr - start);
                        token[chPtr - start] = '\0';
                        ptr = token;
                        if (p_warn != HTM_NONE)
                            warning(0,
                                "Possible null-end token in: %s.", token);
                        // cut leading spaces
                        while (*ptr && (isspace(*ptr))) {
                            if (*ptr == '\n')
                                p_num_lines++;
                            ptr++;
                        }
                        // make lowercase
                        lstring::strtolower(ptr);
                        if (p_warn != HTM_NONE)
                            warning(0,
                                "Checking null-end token %s.", token);
                        // no warning message when ParserTokenToId fails
                        if ((id = tokenToId(token, false)) != -1) {
                            // store this element
                            storeElement(start, chPtr);
                            if (p_warn != HTM_NONE)
                                warning(0,
                                    "Stored valid null-end token %s.", token);
                            // move past the /
                            chPtr++;
                            if (*chPtr == '>') {
                                // ignore / in <tag/>
                                done = true;
                                break;
                            }
                            text_start = chPtr;
                            text_len = 0;
                            // walk up to the next / which terminates this
                            // block
                            for ( ; *chPtr && *chPtr != '/';
                                    chPtr++, cnt++, text_len++) {
                                if (*chPtr == '\n')
                                    p_num_lines++;
                            }
                            // store the text
                            if (text_len && text_start != 0)
                                storeTextElement(text_start, chPtr);
                            // starts after this slash
                            text_start = chPtr + 1;
                            text_len = 0;

                            // Store the end element. Use the empty
                            // closing element notation so storeElement
                            // knows what to do. Reset element ptrs after
                            // that.

                            storeElement((char*)"/>", chPtr);
                            start = 0;
                            // entry has been terminated
                            done = true;
                        }
                        else if (p_warn != HTM_NONE)
                            warning(0, "%s: not a token.", token);
                    }
                    break;
                case '\n':
                    p_num_lines++;
                    break;
                default:
                    break;
                }
            }
            if (done)
                text_start = chPtr+1;  // plain text starts here
            text_len = 0;
            start = 0;
        }
        else if (*chPtr == '\n') {
            p_num_lines++;
            if (cnt > (int)line_len)
                line_len = cnt;
            cnt = 0;
            text_len++;
        }
        else {
            cnt++;
            text_len++;
        }
        // Need this test, we can run out of characters at *any* time.
        if (*chPtr == '\0')
            break;
        chPtr++;
    }
    if (text_len && text_start != 0)
        storeTextElement(text_start, chPtr);

    // see if everything is balanced
    if (p_state_stack->next != 0) {
        htmlEnum state;

        // this is a bad html document
        p_bad_html = true;
        // and thus not HTML 3.2 conforming
        p_html32 = false;

        // bad hack to make sure everything gets appended at the end
        p_cstart = strlen(p_source);
        p_cend   = p_cstart + 1;
        // make all elements balanced
        while (p_state_stack->next != 0) {
            on_stack++;
            state = popState();
            insertElement(html_tokens[state], state, true);
        }
    }

    // Allow lines to have 80 chars at maximum. It's only used when
    // the resizeWidth resource is true.
    p_line_len = (line_len > 80 ? 80 : line_len);

    if (p_warn != HTM_NONE)
        warning(0, "Allocated %i HTML elements "
            "and %i text elements (%i total).", p_num_elements,
            p_num_text, p_num_elements + p_num_text);
}


// Create a doubly linked list of all elements (both HTML and plain
// text).  This parser assumes the text it is parsing is ABSOLUTELY
// PERFECT HTML3.2.  No parserstack is used or created.  It's only
// called when the mime type of a document is text/html-perfect.  No
// HTML shorttags and comments must be correct.  Elements of which the
// terminator is optional *MUST* have a terminator present or bad
// things will happen.
//
void
htmParser::parsePerfectHTML()
{
    // we assume all documents are valid
    p_bad_html = false;
    // and that every document is HTML 3.2 conforming
    p_html32 = true;

    // start scanning
    char *start = 0, *text_start = 0;
    char *chPtr = p_source;
    int text_len = 0;
    p_num_lines = 1;  // every editor starts its linecount at 1
    p_cstart    = 0;
    p_cend      = 0;
    p_line_len  = 0;
    unsigned int line_len = 0;
    int cnt = 0;
    bool done = false;

    while (*chPtr) {
        if (*chPtr == '<') {
            // start of element
            // see if we have any text pending
            if (text_len && text_start != 0) {
                storeTextElement(text_start, chPtr);
                text_start = 0;
                text_len = 0;
            }
            // move past element starter
            start = chPtr+1; // element text starts here
            done = false;
            // absolute starting position for this element
            p_cstart = start - p_source;

            // scan until end of this tag
            while (*chPtr && !done) {
                chPtr++;
                switch (*chPtr) {
                case '!':
                    // either a comment or the !doctype
                    if ((*(chPtr+1) == '>' || *(chPtr+1) == '-')) {
                        // cut comment
                        int dashes = 0;
                        bool end_comment = false;
                        chPtr++;
                        while (!end_comment && *chPtr != '\0') {
                            switch (*chPtr) {
                            case '\n':
                                p_num_lines++;
                                break;
                            case '-':
                                // comment dashes occur twice in succession
                                if (*(chPtr+1) == '-')
                                {
                                    chPtr++;
                                    dashes += 2;
                                }
                                break;
                            case '>':
                                if (*(chPtr-1) == '-' && !(dashes % 4))
                                    end_comment = true;
                                break;
                            }
                            chPtr++;
                        }

                        // back up one element
                        chPtr--;
                        start = chPtr;
                        done = true;
                    }
                    break;
                case '>':
                    // go and store the element
                    chPtr = storeElementUnconditional(start, chPtr);
                    done = true;
                    break;
                case '\n':
                    p_num_lines++;
                    break;
                default:
                    break;
                }
            }
            if (done)
                text_start = chPtr+1; // plain text starts here
            text_len = 0;
            start = 0;
        }
        else if (*chPtr == '\n') {
            p_num_lines++;
            if (cnt > (int)line_len)
                line_len = cnt;
            cnt = 0;
            text_len++;
        }
        else {
            cnt++;
            text_len++;
        }
        // need this test, we can run out of characters at *any* time.
        if (*chPtr == '\0')
            break;
        chPtr++;
    }

    // Allow lines to have 80 chars at maximum. It's only used when
    // the resizeWidth resource is true.
    p_line_len = (line_len > 80 ? 80 : line_len);

    if (p_warn != HTM_NONE)
        warning(0, "Allocated %i HTML elements "
            "and %i text elements (%i total).", p_num_elements,
            p_num_text, p_num_elements + p_num_text);
}


// Create a parser tree for plain text.  This functrion adds html and
// body tags and the full text in a <pre></pre>.  We don't parse
// anything since it can screw up the autocorrection function if the
// raw text contains html commands.
//
void
htmParser::parsePLAIN()
{
    p_num_lines = 1;  // every editor starts its linecount at 1
    p_cstart    = 0;
    p_cend      = 0;

    insertElement(html_tokens[HT_HTML], HT_HTML, false);
    insertElement(html_tokens[HT_BODY], HT_BODY, false);
    insertElement(html_tokens[HT_PRE], HT_PRE, false);

    // store the raw text
    char *chPtr = p_source + p_len;
    storeTextElement(p_source, chPtr);

    // count how many lines we have and get the longest line as well
    int i = 0;
    int line_len = 0;;
    for (chPtr = p_source; *chPtr; chPtr++) {
        switch (*chPtr) {
        case '\n':
            p_num_lines++;
            if (i > line_len)
                line_len = i;
            i = 0;
            break;
        default:
            i++;
        }
    }

    // add closing elements
    insertElement(html_tokens[HT_PRE], HT_PRE, true);
    insertElement(html_tokens[HT_BODY], HT_BODY, true);
    insertElement(html_tokens[HT_HTML], HT_HTML, true);

    // maximum line length
    p_line_len = (line_len > 80 ? 80 : line_len);
}


// Create a parser tree for the given image.
//
void
htmParser::parseIMAGE()
{
    static const char *content_image =
        "<html><body><img src=\"%s\"></body></html>";

    // save original input
    char *input = p_source;

    // create a temporary HTML source text for this image
    char *tmpPtr = new char[(strlen(content_image) + p_len + 1)*sizeof(char)];
    sprintf(tmpPtr, content_image, p_source);

    // set it
    p_source = tmpPtr;
    p_len    = strlen(tmpPtr);

    // parse it
    parseHTML();

    // free temporary source
    delete [] tmpPtr;

    // restore original source
    p_source = input;
    p_len    = strlen(input);
}


// Element verifier, the real funny part.  Returns -1 when element
// should be removed, 0 when the element is not terminated and 1 when
// it is.  This function tries to do a huge amount of damage control
// by a number of checks (and is a real mess).  This function is
// becoming increasingly complex, especially with possible iteration
// over all current parser states to find a proper insertion point
// when the new element is out of place, the checks on contents of the
// current element and appearance of the new element and the
// difference between opening and closing elements.  This function is
// far too complex to explain, I can hardly even grasp it myself.  If
// you really want to know what is happening here, read thru it and
// keep in mind that checkElementOccurance and checkElementContent
// behave very differently from each other.
//
int
htmParser::verify(htmlEnum id, bool is_end)
{
    // current parser state
    htmlEnum curr = p_state_stack->id;
    int iter = 0, new_id;

    // Note about HT_FORM:  this is treated specially, since it is
    // common that these tags are not in sync with other tags, for
    // example by spanning rows or columns of a table.  HT_FORM is
    // never pushed on the stack, nor is any repair attempted
    // involving this tag.  Ditto for HT_DIV.

    // this is so often out of place, ignore it
    if (id == HT_FONT || id == HT_DIV)
        return (1);

    // ignore this
    if (id == HT_HTML && is_end)
        return (-1);

    // ending elements are automatically terminated
    if (is_end || IsElementTerminated(id)) {
        if (!is_end) {
            // adding an opening tag
            //
            // First check:  if the new element matches the current
            // state, we first need to terminate the previous element
            // (remember the new element is a starting one).  Don't do
            // this for nested elements since that has the potential
            // of seriously messing things up.

            if (id == curr && !IsNestableElement(id)) {
                // invalid nesting if this is not an optional closure
                if (!IsOptionalClosure(curr))
                    callback(id, curr, HTML_NESTED);

                // default is to terminate current state
                insertElement(html_tokens[curr], curr, true);
                // new element matches current, so it stays on the stack
                return (1);
            }

            // Second check:  see if the new element is allowed to
            // occur inside the current element.

            new_id = checkElementOccurance(id, curr);
            if (new_id != HT_ZTEXT && new_id != -1) {
                callback(id, curr, HTML_VIOLATION);
                // throw out tags that don't belong in the body
                if ((p_have_body && id != HT_BODY &&
                        !IsBodyElement((htmlEnum)new_id))) {
                    callback(id, (htmlEnum)0, HTML_BAD);
                    return (-1);
                }
                if (checkElementOccurance((htmlEnum)new_id, curr) == HT_ZTEXT) {
                    // ok to add new element
                    insertElement(html_tokens[new_id],
                        (htmlEnum)new_id, new_id == curr);

                    // If the new element terminates it's opening
                    // counterpart, pop it from the stack.  Otherwise
                    // it adds a new parser state.

                    if (new_id == curr)
                        popState();
                    else
                        pushState((htmlEnum)new_id);
                    // new element is now allowed, push it
                    pushState(id);
                    return (1);
                }
            }

recheck:
            if (iter > 4 || (p_state_stack->next == 0 && iter)) {
                // stack restoration
                if (p_state_stack->id == HT_DOCTYPE)
                    pushState(HT_HTML);
                if (p_state_stack->id == HT_HTML)
                    pushState(HT_BODY);

                // HTML_BAD, default is to remove it
                callback(id, curr, HTML_BAD);
                return (-1);
            }
            iter++;

            // Third check:  see if the new element is allowed as
            // content of the current element.  This check will
            // iterate until it finds a matching parser state or until
            // the parser runs out of states.

            if (!checkElementContent(id, curr)) {
                if (id == HT_FORM) {
                    callback(id, curr, HTML_BAD);
                    return (-1);
                }

                // HTML_OPEN_BLOCK, default is to insert current spit
                // out a warning if it's really missing

                if (!IsOptionalClosure(curr))
                    callback(id, curr, HTML_OPEN_BLOCK);

                // patch up a missing TD or TR
                // In general, prematurely terminating a table is a
                // very bad thing

                if (id != curr && curr == HT_TR) {
                    insertElement(html_tokens[HT_TD], HT_TD, false);
                    pushState(HT_TD);
                    pushState(id);
                    return (1);
                }
                if (id != curr && curr == HT_TABLE) {
                    insertElement(html_tokens[HT_TR], HT_TR, false);
                    pushState(HT_TR);
                    if (id != HT_TD) {
                        insertElement(html_tokens[HT_TD], HT_TD, false);
                        pushState(HT_TD);
                    }
                    pushState(id);
                    return (1);
                }

                // terminate current element before adding the new one
                if (curr != HT_DOCTYPE && curr != HT_HTML && curr != HT_BODY)
                    insertElement(html_tokens[curr], curr, true);
                popState();
                curr = p_state_stack->id;
                goto recheck;
            }
            // element allowed, push it
            if (id != HT_FORM)
                pushState(id);
            return (1);
        }
        else {
            // adding a closing tag

            if (id == HT_FORM)
                return (1);

            // <curr></id> Logic:
            //  (1): if </id> invalid tag return -1
            //  (2): if <id> not in stack return -1
            //  (3): if <curr> is optional closure,
            //         close it, pop stack, and loop till id == stack
            //  (4): if <curr> is not optional closure
            //         if stack+1 == id, close, pop, return 1
            //         else return -1

            // (1):
            // See if this element has a terminating counterpart.

            if (!IsElementTerminated(id)) {

                // We do not know terminated elements that can't be
                // terminated (*very* stupid HTML).

                warning(curr, curr, HTML_UNKNOWN_ELEMENT);
                return (-1);  // obliterate it
            }

            // (2):
            // See if the counterpart of this terminating element is
            // on the stack.  If it isn't, we probably terminated it
            // ourselves to keep the document properly balanced and we
            // don't want to insert this terminator as it probably
            // will change the document substantially (a perfect
            // example is <p><form></p>..</form>, which without this
            // check would be changed to
            // <p></p><form></form><p></p>...  instead of
            // <p></p><form>...</form>.

            if (!onStack(id)) {
                warning(id, curr, HTML_CLOSE_BLOCK);
                return (-1);
            }

            // element ends, check balance
reterminate:
            // damage control
            if (iter > 4 || (p_state_stack->next == 0 && iter)) {
                // stack restoration
                if (p_state_stack->id == HT_DOCTYPE)
                    pushState(HT_HTML);
                if (p_state_stack->id == HT_HTML)
                    pushState(HT_BODY);

                // HTML_BAD, default is to remove it
                callback(id, curr, HTML_BAD);
                return (-1);
            }
            iter++;

            if (id != curr) {
                if (IsOptionalClosure(id)) {

                    // If id is an optional closure tag, just ignore
                    // it rather than closing open states.  If it is
                    // needed it will be added later automatically.

                    return (-1);
                }
                if (IsOptionalClosure(curr)) {

                    // (3):
                    // Current state is an optional closure and the
                    // new element causes it to be inserted.  Make it
                    // so and redo the entire process.  This will emit
                    // all optional closures that prevent the new
                    // element from becoming legal and will balance
                    // the stack.  Sigh.  The horror of HTML...

                    if (curr != HT_DOCTYPE && curr != HT_HTML &&
                             curr != HT_BODY)
                        insertElement(html_tokens[curr], curr, true);
                    curr = popState();
                    if (curr != id) {
                        curr = p_state_stack->id;
                        goto reterminate;
                    }
                }
                else {

                    // (4):
                    // This is something like:
                    // <a href=..>...<b>...</a>...</b>
                    // current = HT_B;
                    // id = HT_A;
                    // expected = HT_B;
                    // repair action:  insert the expected element if
                    // it's the next one on the stack, else forget it,
                    // it will be inserted automatically when the
                    // stack reaches the required depth.

                    callback(id, curr, HTML_OPEN_ELEMENT);
                    if (curr == HT_TABLE) {
                        // never terminate a table, looks horrid
                        callback(id, (htmlEnum)0, HTML_BAD);
                        return (-1);
                    }
                    if ((curr == HT_FONT || curr == HT_CENTER ||
                            curr == HT_DL || curr == HT_UL || curr == HT_OL) &&
                            p_state_stack->id == curr) {
                        // hack - if in these tags, just terminate and pop
                        insertElement(html_tokens[curr], (htmlEnum)curr, true);
                        curr = popState();
                        if (curr != id) {
                            curr = p_state_stack->id;
                            iter--;
                            goto reterminate;
                        }
                    }
                    else if (!terminateElement(html_tokens[curr], id, curr)) {
                        callback(id, (htmlEnum)0, HTML_BAD);
                        return (-1);
                    }
                }
            }

            // resync
            if (id == p_state_stack->id)
                popState();
        }
        return (1);  // element allowed
    }

    // see if the new element is allowed as content of current element
    if ((new_id = checkElementOccurance(id, curr)) != HT_ZTEXT) {
        // maybe terminate the current parser state?
        if (new_id == -1)
            new_id = curr;

        callback(id, curr, HTML_VIOLATION);

        // HTML_VIOLATION, default is to insert since new_id is valid
        insertElement(html_tokens[new_id], (htmlEnum)new_id,
            new_id == curr);
        if (new_id == curr)
            popState();
        else
            pushState((htmlEnum)new_id);
    }
    return (0);
}


// Only call the warner when we may issue warnings and this isn't a
// notification message.
//
void
htmParser::callback(htmlEnum id, htmlEnum current, parserError error)
{
    if (p_warn != HTM_NONE && error != HTML_NOTIFY)
        warning(id, current, error);
}


// Give out a warning message depending on the type of error.
//
void
htmParser::warning(htmlEnum id, htmlEnum current, parserError error)
{
    static char msg[256];

    // update error count before doing anything else
    if (error != HTML_UNKNOWN_ELEMENT)
        p_err_count++;

    // Make appropriate error message, set bad_html flag and update
    // error count when error indicates a markup error or HTML
    // violation.

    switch (error) {
    case HTML_UNKNOWN_ELEMENT:
        {
            if (!(p_warn & HTML_UNKNOWN_ELEMENT))
                return;
            int len = p_cend - p_cstart;
            if (len > 127)
                len = 127;
            strcpy(msg, "<");
            msg[1] = 0;  // nullify
            strncat(msg, &p_source[p_cstart], len);
            strcat(msg, ">: unknown HTML identifier, removed.");
        }
        break;
    case HTML_OPEN_ELEMENT:
        p_html32 = false;
        if (!(p_warn & HTML_OPEN_ELEMENT))
            return;
        sprintf(msg, "Unbalanced terminator: got %s while %s is "
            "required.", html_tokens[id], html_tokens[current]);
        break;
    case HTML_BAD:
        p_html32 = false;
        if (!(p_warn & HTML_BAD))
            return;
        sprintf(msg, "Removing element %s which is invalid.", html_tokens[id]);
        break;
    case HTML_OPEN_BLOCK:
        p_html32 = false;
        if (!(p_warn & HTML_OPEN_BLOCK))
            return;
        sprintf(msg, "A new block level element (%s) was encountered "
            "while %s is still open.", html_tokens[id],
            html_tokens[current]);
        break;
    case HTML_CLOSE_BLOCK:
        p_html32 = false;
        if (!(p_warn & HTML_CLOSE_BLOCK))
            return;
        sprintf(msg, "A closing block level element (%s) was encountered "
            "while it was\n    never opened.  This has been removed.",
            html_tokens[id]);
        break;
    case HTML_NESTED:
        p_html32 = false;
        if (!(p_warn & HTML_NESTED))
            return;
        sprintf(msg, "Improperly nested element: %s may not be nested",
            html_tokens[id]);
        break;
    case HTML_VIOLATION:
        p_html32 = false;
        if (!(p_warn & HTML_VIOLATION))
            return;
        sprintf(msg, "HTML Violation: %s may not occur inside %s",
            html_tokens[id], html_tokens[current]);
        break;
    case HTML_INTERNAL:
        sprintf(msg, "Internal parser error!");
        break;
    case HTML_NOTIFY:   // not reached
    case HTML_NONE:
        return;
    }

    warning("verify", "%s\n    "
        "(line %i in input).", msg, p_num_lines);
}


// Check whether the appearence of the current token is allowed in the
// current parser state.  When current is not allowed, the id of the
// element that should be preceeding this one.  If no suitable
// preceeding element can be deduced, it returns -1.  When the element
// is allowed, HT_ZTEXT is returned.
//
int
htmParser::checkElementOccurance(htmlEnum current, htmlEnum state)
{
    stateStack *curr;

    switch (current) {
    case HT_DOCTYPE:
        return (HT_ZTEXT); // always allowed

    case HT_HTML:
        if (state == HT_DOCTYPE)
            return (HT_ZTEXT);
        return (-1);

    case HT_BODY:
        if (state == HT_HTML || state == HT_FRAMESET || state == HT_NOFRAMES)
            return (HT_ZTEXT);
        else {
            // try and guess an appropriate return value
            if (state == HT_HEAD)
                return (HT_HEAD);
            else
                return (HT_HTML);
        }
        return (-1); // not reached

    case HT_HEAD:
    case HT_FRAMESET:
        // frames may be nested
        if (state == HT_HTML || state == HT_FRAMESET)
            return (HT_ZTEXT);
        else
            return (HT_HTML);
        break;

    case HT_NOFRAMES:
        if (state == HT_HTML || state == HT_FRAMESET)
            return (HT_ZTEXT);
        else
            return (HT_HTML);
        break;

    case HT_FRAME:
        if (state == HT_FRAMESET)
            return (HT_ZTEXT);
        else
            return (HT_FRAMESET);
        break;

    case HT_BASE:
    case HT_ISINDEX:
    case HT_META:
    case HT_LINK:
    case HT_SCRIPT:
    case HT_STYLE:
    case HT_TITLE:
        if (state == HT_HEAD)
            return (HT_ZTEXT); // only allowed in the <HEAD> section
        else
            return (HT_HEAD);
        break;

    case HT_IMG:
        if (state == HT_PRE)
            // strictly speaking, images are not allowed inside <pre>
            callback(current, state, HTML_VIOLATION);
        if (IsContainer(state) || IsMarkup(state) || IsMiscElement(state))
            return (HT_ZTEXT);
        else
            return (-1);  // too bad, obliterate it

    case HT_A:
        if (state == HT_A)
            return (-1);  // no nested anchors
        // fall thru, all these elements may occur in the given context
    case HT_FONT:
    case HT_APPLET:
    case HT_B:
    case HT_BASEFONT:
    case HT_BIG:
    case HT_BR:
    case HT_CITE:
    case HT_CODE:
    case HT_DFN:
    case HT_EM:
    case HT_I:
    case HT_INPUT:
    case HT_KBD:
    case HT_MAP:
    case HT_SMALL:
    case HT_SAMP:
    case HT_SELECT:
    case HT_STRIKE:
    case HT_STRONG:
    case HT_SUB:
    case HT_SUP:
    case HT_TAB:
    case HT_TEXTAREA:
    case HT_TT:
    case HT_U:
    case HT_VAR:
        if (IsContainer(state) || IsMarkup(state) || IsMiscElement(state))
            return (HT_ZTEXT);
        if ((current == HT_FONT || current == HT_INPUT) &&
                (state == HT_TD || state == HT_TR || state == HT_TABLE))
            return (HT_ZTEXT);
        return (-1);  // too bad, obliterate it

    case HT_ZTEXT:
            return (HT_ZTEXT);  // always allowed

    case HT_AREA:       // only allowed when inside a <MAP>
        if (state == HT_MAP)
            return (HT_ZTEXT);
        else
            return (HT_MAP);
        break;

    case HT_P:
        if (state == HT_ADDRESS || IsContainer(state))
            return (HT_ZTEXT);
        // guess a proper return value
        switch (state) {
        case HT_FONT:
            return (HT_ZTEXT);
        case HT_OL:
        case HT_UL:
        case HT_DIR:
        case HT_MENU:
            return (HT_LI);
        case HT_TABLE:
            return (HT_TD);
        case HT_DL:
            return (HT_DD);
        default:

            // Strictly speaking, <p> should not be happening, but as
            // this is one of the most abused elements, allow for it
            // if we haven't been told to be strict.

            callback(current, state, HTML_VIOLATION);
            return (HT_ZTEXT);
        }
        return (-1);  // not reached

    case HT_FORM:
        if (state == HT_FORM)
            return (-1);  // no nested forms
        return (HT_ZTEXT);

        // fall thru
    case HT_ADDRESS:
    case HT_BLOCKQUOTE:
    case HT_CENTER:
    case HT_DIV:
    case HT_H1:
    case HT_H2:
    case HT_H3:
    case HT_H4:
    case HT_H5:
    case HT_H6:
    case HT_HR:
    case HT_TABLE:
    case HT_DIR:
    case HT_MENU:
    case HT_DL:
    case HT_PRE:
    case HT_OL:
    case HT_UL:
        if (IsContainer(state))
            return (HT_ZTEXT);
        // correct for most common errors
        switch (state) {
        case HT_OL:
        case HT_UL:
        case HT_DIR:
        case HT_MENU:
            return (HT_LI);
        case HT_TABLE:
            return (HT_TD);
        case HT_DL:
            return (HT_DD);
        case HT_TR:
        case HT_HR:
            return (HT_ZTEXT);
        default:

            // Almost everyone ignores the fact that horizontal rules
            // may only occur in container elements and nowhere
            // else, so allow it.

            if (current == HT_HR)
                return (HT_ZTEXT);
            return (-1);  // too bad, obliterate it
        }
        return (-1);  // not reached

    case HT_LI:
        if (state == HT_UL || state == HT_OL || state == HT_DIR ||
                state == HT_MENU)
            return (HT_ZTEXT);

        // Guess a return value:  walk the current parser state and
        // see if a list is already present.  If it's not, return
        // HT_UL, else return -1.

        for (curr = p_state_stack; curr->next != 0; curr = curr->next) {
            if (curr->id == HT_UL || curr->id == HT_OL ||
                    curr->id == HT_DIR || curr->id == HT_MENU)
                return (-1);
        }
        return (HT_UL);  // start a new list

    case HT_DT:
    case HT_DD:
        if (state == HT_DL)
            return (HT_ZTEXT);
        return (onStack(HT_DL) ? -1 : (int)HT_DL);

    case HT_OPTION:     // only inside the SELECT element
        if (state == HT_SELECT)
            return (HT_ZTEXT);
        else
            return (HT_SELECT);
        break;

    case HT_CAPTION: // only allowed in TABLE
    case HT_TR:
        if (state == HT_TABLE || state == HT_FORM || state == HT_FONT)
            return (HT_ZTEXT);
        // no smart guessing here, it completely fucks things up
        return (-1);

    case HT_TD:
    case HT_TH:
        // only allowed when in a table row
        if (state == HT_TR)
            return (HT_ZTEXT);
        if (state == HT_FONT)
            return (HT_ZTEXT);
        // nested cells are not allowed, so insert another row
        if (state == current)
            return (HT_TR);
        // final check: insert a row when one is not present on the stack
        return (onStack(HT_TR) ? -1 : (int)HT_TR);

    case HT_PARAM:  // only allowed in applets
        if (state == HT_APPLET)
            return (HT_ZTEXT);
        else
            return (HT_APPLET);
        break;
    // no default so we'll get a warning when we miss anything
    }
    return (-1);
}


// Check whether the appearence of the current token is valid in the
// current state.  Return true if the current token is in a valid
// state, false otherwise.
//
bool
htmParser::checkElementContent(htmlEnum current, htmlEnum state)
{
    // plain text is always allowed
    if (current == HT_ZTEXT)
        return (true);

    switch (state) {
    case HT_DOCTYPE:
        return (true);

    case HT_HTML:
        if (current == HT_BODY || current == HT_HEAD ||
                current == HT_FRAMESET || current == HT_NOFRAMES)
            return (true);
        break;

    case HT_FRAMESET:
        if (current == HT_FRAME || current == HT_FRAMESET ||
                current == HT_NOFRAMES)
            return (true);
        break;

    case HT_HEAD:
        if (current == HT_TITLE || current == HT_ISINDEX ||
            current == HT_BASE || current == HT_SCRIPT ||
            current == HT_STYLE || current == HT_META ||
            current == HT_LINK)
            return (true);
        break;

    case HT_NOFRAMES:
    case HT_BODY:
        if (state == HT_NOFRAMES && current == HT_BODY)
            return (true);
        if (current == HT_A || current == HT_ADDRESS ||
            current == HT_APPLET || current == HT_B ||
            current == HT_BIG || current == HT_BLOCKQUOTE ||
            current == HT_BR || current == HT_CENTER ||
            current == HT_CITE || current == HT_CODE ||
            current == HT_DFN || current == HT_DIR ||
            current == HT_DIV || current == HT_DL ||
            current == HT_EM || current == HT_FONT ||
            current == HT_FORM || current == HT_NOFRAMES ||
            current == HT_H1 || current == HT_H2 ||
            current == HT_H3 || current == HT_H4 ||
            current == HT_H5 || current == HT_H6 ||
            current == HT_HR || current == HT_I ||
            current == HT_IMG || current == HT_INPUT ||
            current == HT_KBD || current == HT_MAP ||
            current == HT_MENU || current == HT_OL ||
            current == HT_P || current == HT_PRE ||
            current == HT_SAMP || current == HT_SELECT ||
            current == HT_SMALL || current == HT_STRIKE ||
            current == HT_STRONG || current == HT_SUB ||
            current == HT_SUP || current == HT_TABLE ||
            current == HT_TEXTAREA || current == HT_TT ||
            current == HT_U || current == HT_UL ||
            current == HT_VAR || current == HT_ZTEXT)
            return (true);
        break;

    case HT_A:
        if (current == HT_H1 || current == HT_H2 ||
            current == HT_H3 || current == HT_H4 ||
            current == HT_H5 || current == HT_H6)
            // ok in anchor
            return (true);
        // fall thru
    case HT_FONT:
    case HT_B:
    case HT_BIG:
    case HT_CODE:
    case HT_EM:
    case HT_I:
    case HT_KBD:
    case HT_SAMP:
    case HT_SMALL:
    case HT_STRIKE:
    case HT_STRONG:
    case HT_SUB:
    case HT_SUP:
    case HT_TAB:
    case HT_TT:
    case HT_U:
    case HT_VAR:
    case HT_ADDRESS:
        if (current == HT_P)
            return (true);
        // fall thru, these elements are also allowed
    case HT_CAPTION:
    case HT_CITE:
    case HT_DFN:
    case HT_DT:
    case HT_P:
        if (current == HT_A || current == HT_APPLET ||
            current == HT_B || current == HT_BIG ||
            current == HT_BR || current == HT_CITE ||
            current == HT_CODE || current == HT_DFN ||
            current == HT_CENTER ||  /* SRW */
            current == HT_TABLE || /* SRW */
            current == HT_EM || current == HT_FONT ||
            current == HT_I || current == HT_IMG ||
            current == HT_INPUT || current == HT_KBD ||
            current == HT_MAP || current == HT_NOFRAMES ||
            current == HT_SAMP || current == HT_SCRIPT ||
            current == HT_SELECT || current == HT_SMALL ||
            current == HT_STRIKE || current == HT_STRONG ||
            current == HT_SUB || current == HT_SUP ||
            current == HT_TEXTAREA || current == HT_TT ||
            current == HT_U || current == HT_VAR ||
            current == HT_FORM || current == HT_ZTEXT)
            return (true);
        if (state == HT_FONT && current == HT_TR)
            return (true);
        break;

    case HT_H1:
    case HT_H2:
    case HT_H3:
    case HT_H4:
    case HT_H5:
    case HT_H6:
        if (current == HT_A || current == HT_APPLET ||
            current == HT_B || current == HT_BIG ||
            current == HT_BR || current == HT_CITE ||
            current == HT_CODE || current == HT_DFN ||
            current == HT_EM || current == HT_FONT ||
            current == HT_I || current == HT_IMG ||
            current == HT_INPUT || current == HT_KBD ||
            current == HT_MAP || current == HT_NOFRAMES ||
            current == HT_SAMP || current == HT_SCRIPT ||
            current == HT_SELECT || current == HT_SMALL ||
            current == HT_STRIKE || current == HT_STRONG ||
            current == HT_SUB || current == HT_SUP ||
            current == HT_TEXTAREA || current == HT_TT ||
            current == HT_U || current == HT_VAR ||
            current == HT_FORM || current == HT_ZTEXT)
            return (true);

        // allow these as well...
        if (current == HT_P || current == HT_DIV) {
            // but always issue a warning
            callback(current, state, HTML_VIOLATION);
            return (true);
        }
        break;

    case HT_APPLET:
        if (current == HT_A || current == HT_APPLET ||
            current == HT_B || current == HT_BIG ||
            current == HT_BR || current == HT_CITE ||
            current == HT_CODE || current == HT_DFN ||
            current == HT_EM || current == HT_FONT ||
            current == HT_I || current == HT_IMG ||
            current == HT_INPUT || current == HT_KBD ||
            current == HT_MAP || current == HT_NOFRAMES ||
            current == HT_PARAM || current == HT_SAMP ||
            current == HT_SCRIPT || current == HT_SELECT ||
            current == HT_SMALL || current == HT_STRIKE ||
            current == HT_STRONG || current == HT_SUB ||
            current == HT_SUP || current == HT_TEXTAREA ||
            current == HT_TT || current == HT_U ||
            current == HT_VAR || current == HT_ZTEXT)
            return (true);
        break;

    case HT_MAP:
        if (current == HT_AREA)
            return (true);
        break;

    case HT_AREA:       // only allowed when inside a <MAP>
        break;

    // unterminated tags that may not contain anything
    case HT_BASE:
    case HT_BR:
    case HT_HR:
    case HT_IMG:
    case HT_INPUT:
    case HT_ISINDEX:
    case HT_LINK:
    case HT_META:
    case HT_PARAM:
        return (true);

    case HT_OPTION:
    case HT_SCRIPT:
    case HT_STYLE:
    case HT_TEXTAREA:
    case HT_TITLE:
        if (current == HT_ZTEXT)
            return (true);
        break;

    case HT_FRAME:
        if (current == HT_FRAMESET)
            return (true);
        break;
    case HT_SELECT:
        if (current == HT_OPTION)
            return (true);
        break;

    case HT_TABLE:
        if (current == HT_CAPTION || current == HT_TR || current == HT_FORM ||
                current == HT_FONT)
            return (true);
        break;

    case HT_TR:
        if (current == HT_TH || current == HT_TD || current == HT_FORM ||
                current == HT_FONT)
            return (true);
        break;

    case HT_DIR:
    case HT_MENU:
    case HT_OL:
    case HT_UL:
        if (current == HT_LI || current == HT_FORM)
            return (true);
        break;

    case HT_DL:
        if (current == HT_DT || current == HT_DD || current == HT_FORM)
            return (true);
        break;

    case HT_FORM:
        /* nested forms are not allowed */
        if (current == HT_FORM)
            return (false);
        return (true);

        // fall thru
    case HT_BLOCKQUOTE:
    case HT_CENTER:
    case HT_DIV:
    case HT_TD:
    case HT_TH:
        if (current == HT_H1 || current == HT_H2 ||
            current == HT_H3 || current == HT_H4 ||
            current == HT_H5 || current == HT_H6 ||
            current == HT_ADDRESS)
            return (true);
        // fall thru
    case HT_LI:
    case HT_DD:
        if (current == HT_A || current == HT_APPLET ||
            current == HT_B || current == HT_BIG ||
            current == HT_BLOCKQUOTE || current == HT_BR ||
            current == HT_CENTER || current == HT_CITE ||
            current == HT_CODE || current == HT_DFN ||
            current == HT_DIR || current == HT_DIV ||
            current == HT_DL || current == HT_EM ||
            current == HT_FONT || current == HT_FORM ||
            current == HT_HR || current == HT_I ||
            current == HT_IMG || current == HT_INPUT ||
            current == HT_KBD || current == HT_MAP ||
            current == HT_MENU || current == HT_NOFRAMES ||
            current == HT_OL || current == HT_P ||
            current == HT_PRE || current == HT_SAMP ||
            current == HT_SCRIPT || current == HT_SELECT ||
            current == HT_SMALL || current == HT_STRIKE ||
            current == HT_STRONG || current == HT_SUB ||
            current == HT_SUP || current == HT_TABLE ||
            current == HT_TEXTAREA || current == HT_TT ||
            current == HT_U || current == HT_UL ||
            current == HT_VAR || current == HT_ZTEXT)
            return (true);
        break;

    case HT_PRE:
        if (current == HT_A || current == HT_APPLET ||
            current == HT_B || current == HT_BR ||
            current == HT_CITE || current == HT_CODE ||
            current == HT_DFN || current == HT_EM ||
            current == HT_I || current == HT_INPUT ||
            current == HT_KBD || current == HT_MAP ||
            current == HT_NOFRAMES || current == HT_SAMP ||
            current == HT_SCRIPT || current == HT_SELECT ||
            current == HT_STRIKE || current == HT_STRONG ||
            current == HT_TEXTAREA || current == HT_TT ||
            current == HT_U || current == HT_VAR ||
            current == HT_FONT || current == HT_ZTEXT ||
            current == HT_FORM)
            return (true);
        break;

    case HT_ZTEXT:
        return (true);  // always allowed

    // elements of which we don't know any state information
    case HT_BASEFONT:
        return (true);

    // no default so we'll get a warning when we miss anything
    }

    // There are a number of semi-container elements that often
    // contain the <P>/<DIV> element.  As this isn't a really
    // dangerous element to be floating around randomly, allow it if
    // we haven't been told to be strict.  We can't do this for the
    // elements that have an optional terminator as this would mess up
    // the entire parser algorithm.

    if (current == HT_P || current == HT_DIV) {
        // h1 through h6 is handled above
        if (state == HT_UL || state == HT_OL || state == HT_DL ||
                state == HT_TABLE || state == HT_CAPTION) {
            // but always issue a warning
            callback(current, state, HTML_VIOLATION);
            return (true);
        }
    }
    return (false);
}


// Convert the html token passed to an internal id.  Return the
// internal id upon success, -1 upon failure.  This function uses a
// binary search into an array of all possible HTML 3.2 tokens.  It is
// very important that _BOTH_ the array html_tokens _AND_ the
// enumeration htmlEnum are NEVER changed.  Both arrays are
// alphabetically sorted.  Modifying any of these two arrays will have
// VERY SERIOUS CONSEQUENCES, the return value of this function
// matches a corresponding htmlEnum value.  As the table currently
// contains about 70 elements, a match will always be found in at most
// 7 iterations (2^7 = 128)
//
int
htmParser::tokenToId(const char *token, bool warn)
{
    int mid, lo = 0, hi = HT_ZTEXT-1;

    while (lo <= hi) {
        mid = (lo + hi)/2;
        int cmp;
        if ((cmp = strcmp(token, html_tokens[mid])) == 0)
            return (mid);
        else
            if (cmp < 0)            // in lower end of array
                hi = mid - 1;
            else                    // in higher end of array
                lo = mid + 1;
    }

    // Not found, invalid token passed.  We don't want always have
    // warnings.  When handleShortTags is set to true, this function
    // is used to check whether a / is right behind a token or not.

    if (warn)
        callback(HT_ZTEXT, HT_ZTEXT, HTML_UNKNOWN_ELEMENT);
    return (-1);
}


// Push the given id on the state stack.
//
void
htmParser::pushState(htmlEnum id)
{
    stateStack *tmp = new stateStack(id);
    tmp->next = p_state_stack;
    p_state_stack = tmp;
}


// Pop an element of the state stack.
//
htmlEnum
htmParser::popState()
{

    htmlEnum id;
    if (p_state_stack->next != 0) {
        stateStack *tmp = p_state_stack;
        p_state_stack = p_state_stack->next;
        id = tmp->id;
        delete tmp;
    }
    else
        id = p_state_stack->id;

    return (id);
}


// Check whether the given id is somewhere on the current state stack.
//
bool
htmParser::onStack(htmlEnum id)
{
    for (stateStack *tmp = p_state_stack; tmp != 0; tmp = tmp->next) {
        if (tmp->id == id)
            return (true);
    }
    return (false);
}


// Allocate a new htmObject structure.
//
htmObject*
htmParser::newObject(htmlEnum id, char *element, char *attributes,
    bool is_end, bool terminated)
{
    return (new htmObject(id, element, attributes, is_end, terminated,
        p_num_lines));
}


// Create and insert a new element in the parser tree.
//
void
htmParser::insertElement(const char *element, htmlEnum new_id, bool is_end)
{
    if (!p_current)
        return;

    // allocate an element
    htmObject *extra = newObject(new_id, lstring::copy(element), 0, is_end,
        true);

    // insert this element in the list
    if (extra->id == HT_BODY) {

        // Insert the BODY tag before any prior ZTEXT, otherwise
        // the text won't show.

        htmObject *tmp = p_current;
        while (tmp->prev && tmp->id == HT_ZTEXT)
            tmp = tmp->prev;
        if (tmp) {
            extra->prev = tmp;
            extra->next = tmp->next;
            tmp->next = extra;
            if (extra->next)
                extra->next->prev = extra;
            if (p_current == tmp)
                p_current = extra;
        }
    }
    else {
        extra->prev = p_current;
        extra->next = p_current->next;
        p_current->next = extra;
        if (extra->next)
            extra->next->prev = extra;
        p_current = extra;
    }
    p_num_elements++;

    if (p_warn != HTM_NONE && (!is_end || !IsOptionalClosure(new_id)))
        warning(0, "Added a missing %s %s at line %i.",
            is_end ? "closing" : "opening", element, p_num_lines);
}


// Backtrack in the list of elements to terminate the given element.
// Used for terminating an unbalanced element.
//
bool
htmParser::terminateElement(const char *element, htmlEnum current,
    htmlEnum expect)
{
    stateStack *state = p_state_stack;

    // If current element is the next one on the stack, insert the
    // expected element, restore the stack and allow the current
    // element.

    if (state->next && state->next->id == current) {

        char *tmp = lstring::copy(element);

        // insert expected element
        htmObject *extra = newObject(expect, tmp, 0, true, true);

        p_num_elements++;
        extra->prev = p_current;
        p_current->next = extra;
        p_current = extra;

        // pop expected element from the stack
        popState();

        if (p_warn != HTM_NONE)
            warning(0, "Terminated element %s at line %i.", element,
                p_num_lines);

        // current allowed now
        return (true);
    }
    // needs a backtrack, postpone until it really becomes invalid
    return (false);
}


// Copy and insert the given object.
//
void
htmParser::copyElement(htmObject *src, bool is_end)
{
    if (!src)
        return;

    // allocate element data
    int len = strlen(src->element) +
        (src->attributes ? strlen(src->attributes) : 1);
    char *str = new char[(len+2)*sizeof(char)];

    // copy element data
    strcpy(str, src->element);
    len = strlen(str);
    str[len+1] = 0;

    // copy possible attributes
    if (src->attributes)
        strcpy(&str[len+1], src->attributes);

    htmObject *copy = new htmObject(src->id, str, &str[len+1], is_end,
        src->terminated, p_num_lines);

    p_num_elements++;
    // attach prev and next ptrs to the appropriate places
    copy->prev = p_current;
    p_current->next = copy;
    p_current = copy;
}


// Allocate and store an HTML element.
//
char *
htmParser::storeElement(char *start, char *end)
{
    if (end == 0 || *end == 0)
        return (end);

    // absolute ending position for this element
    p_cend = p_cstart + (end - start);

    // If this is true, we have an empty tag or an empty closing tag,
    // the action to take depends on what type of empty tag it is.

    htmObject *element = 0;
    if (start[0] == '>' || (start[0] == '/' && start[1] == '>')) {

        // If start[0] == '>', we have an empty tag, otherwise we have
        // an empty closing tag.  In the first case, we walk backwards
        // until we reach the very first tag.  An empty tag simply
        // means:  copy the previous tag, nomatter what content it may
        // have.  In the second case, we need to pick up the last
        // recorded opening tag and copy it.

        if (start[0] == '>') {

            // Walk backwards until we reach the first non-text
            // element.  Elements with an optional terminator which
            // are not terminated are updated as well.

            for (element = p_current ; element != 0; element = element->prev) {
                if (IsOptionalClosure(element->id) && !element->is_end &&
                        element->id == p_state_stack->id) {
                    insertElement(element->element, element->id, true);
                    popState();
                    break;
                }
                else if (element->id != HT_ZTEXT)
                    break;
            }
            if (element) {
                if (p_warn != HTM_NONE)
                    warning(0, "Empty tag on line %i, inserting %s.",
                        p_num_lines, element->element);
                copyElement(element, false);
                if (element->terminated)
                    pushState(element->id);
            }
        }
        else {
            for (element = p_current; element != 0; element = element->prev) {
                if (element->terminated) {
                    if (IsOptionalClosure(element->id)) {
                        // add possible terminators for these elements
                        if (!element->is_end &&
                                element->id == p_state_stack->id) {
                            insertElement(element->element, element->id, true);
                            popState();
                        }
                    }
                    else
                        break;
                }
            }
            if (element) {
                if (p_warn != HTM_NONE)
                    warning(0,
                        "Empty closing tag on line %i, inserting %s.",
                        p_num_lines, element->element);
                copyElement(element, true);
                popState();
            }
        }
        return (end);
    }

    char *startPtr = start, *endPtr;
    // check if we have any unclosed tags
    if ((endPtr = strstr(start, "<")) == 0)
        endPtr = end;
    // check if we stay within bounds
    else if (endPtr - end > 0)
        endPtr = end;

    bool ignore = false;
    while (1) {
        bool is_end = false;

        // First skip past spaces and a possible opening /.  The
        // endPtr test is mandatory or else we would walk the entire
        // text over and over again every time this routine is called.

        char *elePtr;
        for (elePtr = startPtr; *elePtr && elePtr != endPtr; elePtr++) {
            if (*elePtr == '/') {
                is_end = true;
                elePtr++;
                break;
            }
            if (!(isspace(*elePtr)))
                break;
        }
        // useful sanity
        if (endPtr - elePtr < 1)
            break;

        // allocate and copy element
        char *content = htm_strndup(elePtr, endPtr - elePtr);

        char *chPtr = elePtr = content;

        // Move past the text to get any element attributes.  The !
        // will let us pick up the !DOCTYPE definition.  Don't put the
        // chPtr++ inside the tolower, chances are that it will be
        // evaluated multiple times if tolower is a macro.  From:
        // Danny Backx <u27113@kb.be>

        if (*chPtr == '!')
            chPtr++;
        while (*chPtr && !(isspace(*chPtr))) {
            // fix 01/17/97-01; kdh
            *chPtr = tolower(*chPtr);
            chPtr++;
        }

        // Attributes are only allowed in opening elements This is a
        // neat hack:  to reduce allocations, we do *not* copy the
        // element name into it's own buffer.  Instead we use the one
        // allocated above, and place a \0 in the space right after
        // the HTML element.  If this element has attributes, we set
        // the attribute pointer (=chPtr) to point right after this
        // \0.  This also has the advantage that no reallocation or
        // string copying is required.  Freeing the memory allocated
        // can be done in one call on the element field of the
        // htmObject struct.

        if (!is_end) {
            if (*chPtr && *(chPtr+1)) {
                content[chPtr - elePtr] = 0;
                chPtr = content + strlen(content) + 1;
            }
            else {
                // no trailing attributes for this element
                if (*chPtr)      // fix 01/17/97-01; kdh
                    content[chPtr - elePtr] = 0;
                else
                    chPtr = 0;
            }
        }
        else {
            // closing element, can't have any attributes
            if (*chPtr)
                content[chPtr - elePtr] = 0;
            chPtr = 0;
        }

        // Ignore elements we do not know
        int id;
        if ((id = tokenToId(elePtr, p_warn)) != -1) {

            // Check if this element belongs to body.  This test is as
            // best as it can get (we do not scan raw text for
            // non-whitespace chars) but will omit any text appearing
            // *before* the <body> tag is inserted.

            if (!p_have_body) {
                if (id == HT_BODY)
                    p_have_body = true;
                else if (IsBodyElement((htmlEnum)id)) {
                    insertElement("body", HT_BODY, false);
                    pushState(HT_BODY);
                    p_have_body = true;
                }
            }

            // verify presence of the new element
            int terminated = verify((htmlEnum)id, is_end);

            if (terminated == -1) {
                p_html32 = false; // not HTML32 conforming
                ignore = true;
                delete [] content;
                // remove contents of badly placed SCRIPT & STYLE elements
                if ((id == HT_SCRIPT || id == HT_STYLE || id == HT_TITLE) &&
                        !is_end)
                    goto removeData;
                return (end);
            }
            else
                ignore = false;

            // insert the current element
            element = newObject((htmlEnum)id, elePtr, chPtr, is_end,
                (terminated != 0));
            // attach prev and next ptrs to the appropriate places
            p_num_elements++;
            element->prev = p_current;
            p_current->next = element;
            p_current = element;

            // The SCRIPT & STYLE elements are a real pain in the ass
            // to deal with properly:  they are text with whatever in
            // it, and it's terminated by a closing element.  It would
            // be very easy if the tags content are enclosed within a
            // comment, but since this is *NOT* required by the HTML
            // 3.2 spec, we need to check it in here...

removeData:
            if ((id == HT_SCRIPT || id == HT_STYLE || id == HT_TITLE) &&
                    !is_end) {
                int done = 0;
                char *tmpstart = end;
                int text_len = 0;

                // move past closing >
                tmpstart++;
                while (*end != '\0' && done == 0) {
                    switch (*end) {
                    case '<':
                        // catch comments
                        if (*(end+1) == '/') {
                            if (!(strncasecmp(end+1, "/script", 7)))
                                done = 1;
                            else if (!(strncasecmp(end+1, "/style", 6)))
                                done = 2;
                            else if (!(strncasecmp(end+1, "/title", 6)))
                                done = 2;
                        }
                        break;
                    case '\n':
                        p_num_lines++;
                        // fall through
                    default:
                        text_len++;
                        break;
                    }
                    if (*end == '\0')
                        break;
                    end++;
                }
                if (done) {

                    // Only store contents if this tag was in place.
                    // This check is required as this piece of code is
                    // also used to remove the tags content when the
                    // tag is out of it's proper place (not inside the
                    // <head> section).  We must always do this when
                    // we are compiled as standalone parser.  Parser
                    // will advance by one byte upon return, so we
                    // need to decrement by two bytes

                    // store script contents
                    end--;
                    if (!ignore)
                        storeTextElement(tmpstart, end, true);
                    end--;
                }
                else
                    // restore original end position
                    end = tmpstart-1;

                // no check for unclosed tags here, just return
                return (end);
            }
        }
        else {
            // ignore
            delete [] content;  // fix 01/28/97-01, kdh
        }

        // check if we have any unclosed tags remaining
        if (endPtr - end < 0) {
            endPtr++;
            startPtr = endPtr;
            // check if we have any unclosed tags
            if ((endPtr = strstr(startPtr, "<")) == 0)
                endPtr = end;

            // check if we stay within bounds
            else if (endPtr - end > 0)
                    endPtr = end;
        }
        else
            break;
    }
    return (end);
}


// Allocate and store an HTML element *without* verifying it.
//
char *
htmParser::storeElementUnconditional(char *start, char *end)
{
    if (end == 0 || *end == '\0')
        return (end);

    // absolute ending position for this element
    p_cend = p_cstart + (end - start);

    // no null end tags here

    char *startPtr = start, *endPtr;
    // check if we have any unclosed tags
    if ((endPtr = strstr(start, "<")) == 0)
        endPtr = end;
    // check if we stay within bounds
    else if (endPtr - end > 0)
        endPtr = end;

    bool is_end = false;

    // First skip past spaces and a possible opening /.  The endPtr
    // test is mandatory or else we would walk the entire text over
    // and over again every time this routine is called.

    char *elePtr;
    for (elePtr = startPtr; *elePtr && elePtr != endPtr; elePtr++) {
        if (*elePtr == '/') {
            is_end = true;
            elePtr++;
            break;
        }
        if (!(isspace(*elePtr)))
            break;
    }
    // usefull sanity
    if (endPtr - elePtr < 1)
        return (end);

    // allocate and copy element
    char *content = htm_strndup(elePtr, endPtr - elePtr);

    char *chPtr = elePtr = content;

    // Move past the text to get any element attributes.
    if (*chPtr == '!')
        chPtr++;
    while (*chPtr && !(isspace(*chPtr))) {
        *chPtr = tolower(*chPtr);
        chPtr++;
    }

    // attributes are only allowed in opening elements
    if (!is_end) {
        if (*chPtr && *(chPtr+1)) {
            content[chPtr - elePtr] = 0;
            chPtr = content + strlen(content) + 1;
        }
        else  {
            // no trailing attributes for this element
            if (*chPtr)
                content[chPtr - elePtr] = 0;
            else
                chPtr = 0;
        }
    }
    else
        // closing element, can't have any attributes
        chPtr = 0;

    // ignore elements we do not know
    int id;
    if ((id = tokenToId(elePtr, p_warn)) != -1) {
        // see if this element has a closing counterpart
        bool terminated = IsElementTerminated((htmlEnum)id);

        // insert the current element
        htmObject *element = newObject((htmlEnum)id, elePtr, chPtr, is_end,
            terminated);

        // attach prev and next ptrs to the appropriate places
        p_num_elements++;
        element->prev = p_current;
        p_current->next = element;
        p_current = element;

        // get contents of the SCRIPT & STYLE elements
        if ((id == HT_SCRIPT || id == HT_STYLE) && !is_end) {
            int done = 0;
            char *tmpstart = end;
            int text_len = 0;

            // move past closing >
            tmpstart++;
            while (*end != '\0' && done == 0) {
                switch (*end) {
                case '<':
                    // catch comments
                    if (*(end+1) == '/') {
                        if (!(strncasecmp(end+1, "/script", 7)))
                            done = 1;
                        else if (!(strncasecmp(end+1, "/style", 6)))
                            done = 2;
                    }
                    break;
                case '\n':
                    p_num_lines++;
                    // fall through
                default:
                    text_len++;
                    break;
                }
                if (*end == '\0')
                    break;
                end++;
            }
            if (done) {
                // store script contents
                end--;
                    storeTextElement(tmpstart, end-1, true);
                end--;
            }
            else
                // restore original end position
                end = tmpstart-1;

            // no check for unclosed tags here, just return
            return (end);
        }
    }
    else
        // ignore
        delete [] content;

    return (end);
}


// Allocate and store a plain text element.
//
void
htmParser::storeTextElement(char *start, char *end, bool l_to_r)
{
    // length of this block
    int len = end - start;

    // sanity
    if (!*start || len <= 0)
        return;

    char *content = htm_strndup(start, len);

    if (p_directionRtoL && !l_to_r) {
        // copy text, reversing contents as we do
        char *inPtr = start;
        char *outPtr = &content[len-1];
        while (1) {
            switch (*inPtr) {
            case '&':
                // we don't touch escape sequences
                {
                    // set start position
                    char *ptr = inPtr;

                    // get end
                    while (ptr < end && *ptr != ';')
                        ptr++;

                    // might not be a valid escape sequence
                    if (ptr == end)
                        break;

                    // insertion position
                    outPtr -= (ptr - inPtr);

                    // copy literally
                    memcpy(outPtr, inPtr, (ptr+1) - inPtr);

                    // new start position
                    inPtr = ptr;
                }
                break;

            // All bi-directional characters need to be reversed if we
            // want them to keep their intended behaviour.

            case '`':
                *outPtr = '\'';
                break;
            case '\'':
                *outPtr = '`';
                break;
            case '<':
                *outPtr = '>';
                break;
            case '>':
                *outPtr = '<';
                break;
            case '\\':
                *outPtr = '/';
                break;
            case '/':
                *outPtr = '\\';
                break;
            case '(':
                *outPtr = ')';
                break;
            case ')':
                *outPtr = '(';
                break;
            case '[':
                *outPtr = ']';
                break;
            case ']':
                *outPtr = '[';
                break;
            case '{':
                *outPtr = '}';
                break;
            case '}':
                *outPtr = '{';
                break;
            default:
                *outPtr = *inPtr;
                break;
            }
            inPtr++;
            outPtr--;
            if (inPtr == end)
                break;
        }
        content[len] = '\0';
    }

    htmObject *element = newObject(HT_ZTEXT, content, 0, false, false);

    // store this element in the list
    p_num_text++;
    element->prev = p_current;
    p_current->next = element;
    p_current = element;
}


// Removes HTML comments from the input stream.  HTML comments are one
// of the most difficult things to deal with due to the unlucky
// definition:  a comment starts with a <!, followed by zero or more
// comments, followed by >.  A comment starts and ends with "--", and
// does not contain any occurance of "--".  This effectively means
// that dashes must occur in a multiple of four.  And this is were the
// problems lies:  _many_ people don't realize this and thus open
// their comments with <!-- and end it with --> and put everything
// they like in it, including any number of dashes and --> sequences.
// To deal with all of this as much as we can, we scan the text until
// we reach a --> sequence with a balanced number of dashes.  If we
// run into a --> and we don't have a balanced number of dashes, we
// look ahead in the buffer.  The *original* comment is then
// terminated (by rewinding to the original comment ending) if we
// encounter an element opening (can be anything *except* <-) or if we
// run out of characters.  If we encounter another --> sequence, the
// comment ends here.  This is a severe performance penalty since a
// considerable number of characters *can* be scanned in order to find
// an element start or the next comment ending.  Wouldn't life be
// *much* easier if we lived in a perfect world!
//
char *
htmParser::cutComment(char *start)
{
    int dashes = 0, nchars = 0, start_line = p_num_lines;
    bool end_comment = false, start_dashes = false;
    char *chPtr = start;

    // move past opening exclamation character
    chPtr++;
    int nlines = 0;
    while (!end_comment && *chPtr) {
        switch (*chPtr) {
        case '\n':
            nlines += 1;
            nchars++;
            break;  // fix 01/14/97-01
        case '-':
            // comment dashes occur twice in succession
            // fix 01/14/97-02; kdh
            // fix 04/30/97-1; sl
            if (*(chPtr+1) == '-' && !start_dashes) {
                chPtr++;
                nchars++;
                dashes++;
                start_dashes = true;
            }
            if (*(chPtr+1) == '-' || *(chPtr-1) == '-')
                dashes++;
            break;
        case '>':
            // Problem:  > in a comment is a valid character, so the
            // comment should only be terminated when we have a
            // multiple of four dashes.  If we haven't, we need to
            // look ahead.

            if (*(chPtr-1) == '-') {
                if (!(dashes % 4))
                    end_comment = true;
                else {
                    char *sub = chPtr;
                    bool end_sub = false;
                    int sub_lines = nlines, sub_nchars = nchars;

                    // Scan ahead until we run into another -->
                    // sequence or element opening.  If we don't,
                    // rewind and terminate the comment.

                    do {
                        chPtr++;
                        switch (*chPtr) {
                        case '\n':
                            nlines += 1;
                            nchars++;
                            break;  // fix 01/14/97-01; kdh
                        case '-':
                            if (*(chPtr+1) == '-' || *(chPtr-1) == '-')
                                dashes++;
                            break;
                        case '<':
                            // comment ended at original position
                            if (*(chPtr+1) != '-') {
                                chPtr = sub;
                                end_sub = true;
                            }
                            break;
                        case '>':
                            // comment ends here
                            if ((strncmp(chPtr-2, "--", 2) == 0) &&
                                    start_dashes) {
                                end_sub = true;
                                end_comment = true;
                                break;
                            }
                            // another nested >
                            break;
                        case '\0':
                            // comment ended at original position
                            chPtr = sub;
                            end_sub = true;
                            break;
                        }
                    }
                    while (*chPtr != '\0' && !end_sub);
                    if (chPtr == sub) {
                        // comment was ended at original position, rewind
                        end_comment = true;
                        nlines = sub_lines;
                        nchars = sub_nchars;
                    }
                }
            }
            else // special case: the empty comment
                if (*(chPtr-1) == '!' && !(dashes % 4))
                    end_comment = true;
            break;
        }
        chPtr++;
        nchars++;
    }

    /*
    if (p_warn != HTM_NONE)
        warning(0, "Removed comment spanning %i chars between "
            "line %i and %i.", nchars, start_line, start_line + nlines);
    */

    p_num_lines += nlines;

    // spit out a warning if the dash count is not multiple of four
    if (dashes %4 && p_warn != HTM_NONE)
        warning("parseHTML",
            "Bad HTML comment on line %i of input:\n    number of dashes is "
            "no multiple of four (counted %i dashes).", start_line, dashes);
    return (chPtr);
}


void
htmParser::warning(const char *fn, const char *fmt, ...)
{
    char buf[1024];
    va_list arg_list;
    va_start(arg_list, fmt);

    if (fn) {
        sprintf(buf, "Warning: in %s\n", fn);
        int len = strlen(buf);
        vsnprintf(buf + len, 1024 - len, fmt, arg_list);
    }
    else
        vsnprintf(buf, 1024, fmt, arg_list);
    va_end(arg_list);
    if (p_html)
        p_html->warning(0, buf);
    else {
        strcat(buf, "\n");
        fputs(buf, stderr);
    }
}


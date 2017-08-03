
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
 *   Stephen R. Whiteley  <srw@wrcad.com>
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

#ifndef HTM_CALLBACK_H
#define HTM_CALLBACK_H

namespace htm
{
    // URL types
    enum URLType
    {
        ANCHOR_UNKNOWN,         // unknown href
        ANCHOR_NAMED,           // name="...."
        ANCHOR_JUMP,            // href="#..."
        ANCHOR_FILE_LOCAL,      // href="file.html"
                                // href="file:/file.html" (clearly local)
                                // href="file:///file.html" (null host)
                                // href="file://localhost/file.html" (localhost)
                                //
        ANCHOR_FILE_REMOTE,     // href="file://foo.bar/file.html"
        ANCHOR_FTP,             // href="ftp://foo.bar/file"
        ANCHOR_HTTP,            // href="http://foo.bar/file.html"
        ANCHOR_SECURE_HTTP,     // href="https://foo.bar/file.html"
        ANCHOR_GOPHER,          // href="gopher://foo.bar:70"
        ANCHOR_WAIS,            // href="wais://foo.bar"
        ANCHOR_NEWS,            // href="news://foo.bar"
        ANCHOR_TELNET,          // href="telnet://foo.bar:23"
        ANCHOR_MAILTO,          // href="mailto:foo@bar"
        ANCHOR_EXEC,            // href="exec:foo_bar"
        ANCHOR_PIPE,            // href="pipe:foo_bar"
        ANCHOR_ABOUT,           // href="about:..."
        ANCHOR_INFO,            // href="info:.."
        ANCHOR_MAN,             // href="man:..."
        ANCHOR_FORM_IMAGE       // <input type=image>, only used internally
    };

    // Callback reasons
    enum
    {
        HTM_ANCHORTRACK = 16384,    // anchorTrackCallback
        HTM_DOCUMENT,               // documentCallback
        HTM_FORM,                   // formCallback
        HTM_FRAMECREATE,            // frameCallback
        HTM_FRAMERESIZE,            // frameCallback
        HTM_FRAMEDESTROY,           // frameCallback
        HTM_IMAGEMAPACTIVATE,       // imagemapCallback
        HTM_IMAGEMAP,               // imagemapCallback
        HTM_LINK,                   // linkCallback
        HTM_MODIFYING_TEXT_VALUE,   // modifyVerifyCallback
        HTM_MOTIONTRACK,            // motionTrackCallback
        HTM_PARSER,                 // parserCallback
        HTM_EVENT,                  // eventCallback
        HTM_EVENTDESTROY            // eventCallback
    };

    // The three possible anchor selection states
    enum
    {
        ANCHOR_UNSELECTED,          // default anchor state
        ANCHOR_INSELECT,            // anchor is gaining selection
        ANCHOR_SELECTED             // anchor is selected
    };

    // eventCallback sub event types
    enum
    {
        // Document/Frame specific events
        HTM_LOAD,                   // onLoad
        HTM_UNLOAD,                 // onUnLoad

        // HTML Form specific events
        HTM_SUBMIT,                 // onSubmit
        HTM_RESET,                  // onReset
        HTM_FOCUS,                  // onFocus
        HTM_BLUR,                   // onBlur
        HTM_SELECT,                 // onSelect
        HTM_CHANGE,                 // onChange

        // object events
        HTM_CLICK,                  // onClick
        HTM_DOUBLE_CLICK,           // onDblClick
        HTM_MOUSEDOWN,              // onMouseDown
        HTM_MOUSEUP,                // onMouseUp
        HTM_MOUSEOVER,              // onMouseOver
        HTM_MOUSEMOVE,              // onMouseMove
        HTM_MOUSEOUT,               // onMouseOut
        HTM_KEYPRESS,               // onKeyPress
        HTM_KEYDOWN,                // onKeyDown
        HTM_KEYUP,                  // onKeyUp
        HTM_USEREVENT               // must always be last
    };

    // Codes for the action field in the htmParserCallbackStruct
    enum
    {
        HTML_REMOVE = 1,            // remove offending element
        HTML_INSERT,                // insert missing element
        HTML_SWITCH,                // switch offending and expected element
        HTML_KEEP,                  // keep offending element
        HTML_IGNORE,                // ignore, proceed as if nothing happened
        HTML_ALIAS,                 // alias an unknown element to known one
        HTML_TERMINATE              // terminate parser
    };
}

// arm callback structure
//
struct htmCallbackInfo
{
    htmCallbackInfo(int r, htmEvent *e)
        {
            reason = r;
            event = e;
        }

    int reason;
    htmEvent *event;
};

// activate and anchorTrack callback structure
//
struct htmAnchorCallbackStruct
{
    htmAnchorCallbackStruct(htmAnchor *a = 0);

    int         reason;         // the reason the callback was called
    htmEvent    *event;         // event structure that triggered callback
    URLType     url_type;       // type of url referenced
    unsigned    line;           // line number of the selected anchor
    const char  *href;          // pointer to the anchor value
    const char  *target;        // pointer to target value
    const char  *rel;           // pointer to rel value
    const char  *rev;           // pointer to rev value
    const char  *title;         // pointer to title value
    bool        is_frame;       // true when inside a frame
    bool        doit;           // local anchor vetoing flag
    bool        visited;        // local anchor visited flag
};

// anchorVisited callback structure
//
struct htmVisitedCallbackStruct
{
    htmVisitedCallbackStruct()
        {
            url             = 0;
            visited         = false;
        }

    const char *url;
    bool visited;
};

// document callback structure
//
struct htmDocumentCallbackStruct
{
    htmDocumentCallbackStruct()
        {
            reason      = HTM_DOCUMENT;
            event       = 0;
            html32      = false;
            verified    = false;
            balanced    = false;
            terminated  = false;
            pass_level  = 0;
            redo        = false;
        }

    int reason;                 // the reason the callback was called
    htmEvent *event;            // always 0 for documentCallback
    bool html32;                // true if document was HTML 3.2 conforming
    bool verified;              // true if document has been verified
    bool balanced;              // true if parser tree is balanced
    bool terminated;            // true if parser terminated prematurely
    int pass_level;             // current parser level count. Starts at 1
    bool redo;                  // perform another pass?
};

// Link member information
struct htmLinkDataRec
{
    htmLinkDataRec()
        {
            url     = 0;
            rel     = 0;
            rev     = 0;
            title   = 0;
        }

    ~htmLinkDataRec()
        {
            delete [] url;
            delete [] rel;
            delete [] rev;
            delete [] title;
        }

    const char *url;            // value of URL tag
    const char *rel;            // value of REL tag
    const char *rev;            // value of REV tag
    const char *title;          // value of TITLE tag
};

// link callback structure
struct htmLinkCallbackStruct
{
    htmLinkCallbackStruct()
        {
            reason      = HTM_LINK;
            event       = 0;
            num_link    = 0;
            link        = 0;
        }

    ~htmLinkCallbackStruct()
        {
            delete [] link;
        }

    int reason;                 // the reason the callback was called
    htmEvent *event;            // event structure that triggered callback
    int num_link;               // number of LINK info to process
    htmLinkDataRec *link;       // array of LINK info to process
};

// imagemap callback structure.  callback reasons can be one
// of the following:
// HTM_IMAGEMAP_ACTIVATE
//   user clicked on an image.  Valid fields are x, y and image_name.  x
//   and y are relative to the upper-left corner of the image.  Only
//   invoked when an image has it's ismap attribute set and no usemap is
//   present for this image.
// HTM_IMAGEMAP
//   an image requires an external imagemap.  The only valid field is
//   map_name which contains the location of the imagemap to fetch.  If
//   the contents of this imagemap is set in the map_contents field, it
//   will be loaded by the widget.  Alternatively, one could also use
//   the htmAddImagemap convenience routine to set an imagemap into the
//   widget.
//
struct htmImagemapCallbackStruct
{
    htmImagemapCallbackStruct()
        {
            reason          = HTM_IMAGEMAP;
            event           = 0;
            x = y           = 0;
            image_name      = 0;
            map_name        = 0;
            map_contents    = 0;
            image           = 0;
        }

    int reason;         // the reason the callback was called
    htmEvent *event;    // event structure that triggered callback
    int x,y;            // position relative to the upper-left image corner
    const char *image_name;   // name of referenced image, value of src
                              // attribute
    const char *map_name;     // name of imagemap to fetch
    const char *map_contents; // contents of fetched imagemap
    htmImageInfo *image;// image data
};

// event callback structure
struct htmEventCallbackStruct
{
    htmEventCallbackStruct()
        {
            reason  = HTM_EVENT;
            event   = 0;
            type    = 0;
            data    = 0;
        }

    int         reason;         // the reason the event was called
    htmEvent    *event;         // event triggering this action
    int         type;           // HTML4.0 event type, see above
    void        *data;          // HTML4.0 event callback data
};

// End of callback structures


// Meta member information
struct htmMetaDataRec
{
    htmMetaDataRec()
        {
            http_equiv  = 0;
            name        = 0;
            content     = 0;
        }

    ~htmMetaDataRec()
        {
            delete [] http_equiv;
            delete [] name;
            delete [] content;
        }

    const char *http_equiv;     // value of HTTP-EQUIV tag
    const char *name;           // value of NAME tag
    const char *content;        // value of CONTENT tag
};

// htmHeadAttributes definition
struct htmHeadAttributes
{
    htmHeadAttributes();
    ~htmHeadAttributes();

    void freeHeadAttributes(unsigned char);

    const char *doctype;        // doctype data
    const char *title;          // document title
    bool is_index;              // true when the <isindex> element exists
    const char *base;           // value of the <base> element
    int num_meta;               // number of META info to process
    htmMetaDataRec *meta;       // array of META info to process
    int num_link;               // number of LINK info to process
    htmLinkDataRec *link;       // array of LINK info to process
    const char *style_type;     // value of the style type element tag
    const char *style;          // <style></style> contents
    const char *script_lang;    // value of the language element tag
    const char *script;         // <script></script> contents
};

#endif


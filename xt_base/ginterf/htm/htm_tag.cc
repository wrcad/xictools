
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
 * $Id: htm_tag.cc,v 1.8 2014/02/15 23:14:17 stevew Exp $
 *-----------------------------------------------------------------------*/

#include "htm_widget.h"
#include "htm_string.h"
#include "htm_tag.h"
#include <sys/types.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>

namespace htm
{

    // Check whether the given tag exists in the attributes of a HTML
    // element.  Returns true if tag is found, false otherwise.
    //
    bool
    htmTagCheck(const char *attributes, const char *tag)
    {
        if (attributes == 0)
            return (false);

        char *chPtr;
        if ((chPtr = htm_strcasestr(attributes, tag)) != 0) {
            // see if this is a valid tag: it must be preceeded
            // with whitespace
            while (*(chPtr-1) && !isspace(*(chPtr-1))) {
                // start right after this element
                char *start = chPtr + strlen(tag);
                if ((chPtr = htm_strcasestr(start, tag)) == 0)
                    return (false);
            }
            return (true);
        }
        return (false);
    }


    // Looks for the specified tag in the given list of attributes.  If
    // tag exists, the value of this tag is returned, 0 otherwise.  The
    // return value is always malloc'd; caller must free it.
    //
    char *
    htmTagGetValue(const char *attributes, const char *tag)
    {
        if (attributes == 0 || tag == 0)
            return (0);

        char *chPtr, *start, *end;
        if ((chPtr = htm_strcasestr(attributes, tag)) != 0) {

            // Check if the ptr obtained is correct, eg, no whitespace
            // before it.  If this is not the case, get the next match.
            // Need to do this since a single htm_strcasestr on, for
            // example, align will match both align _and_ valign.

            while (chPtr > attributes && *(chPtr-1) && !isspace(*(chPtr-1))) {
                start = chPtr+strlen(tag);  // start right after this element
                if ((chPtr = htm_strcasestr(start, tag)) == 0)
                    return (0);
            }

            start = chPtr+strlen(tag);  // start right after this element
            // remove leading spaces
            while (isspace(*start))
                start++;

            // if no '=', return 0
            if (*start != '=')
                return (0);

            start++;    // move past the '=' char

            // remove more spaces
            while (*start && isspace(*start))
                start++;

            if (*start == '\0') {
#ifdef PEDANTIC
                warning("tagGetValue", "tag %s has no value.", tag);
#endif
                return (0);
            }

            // unquoted tag values are treated differently
            if (*start == '\"') {
                start++;
                for (end = start; *end != '\"' && *end != '\0' ; end++) ;
            }
            else if (*start == '\'') {
                start++;
                for (end = start; *end != '\'' && *end != '\0' ; end++) ;
            }
            else
                for (end = start; !(isspace(*end)) && *end != '\0' ; end++) ;

            // empty string
            if (end == start)
                return (0);

            return (htm_strndup(start, end - start));
        }
        return (0);
    }


    // Retrieve the numerical value of the given tag.  If tag exists, the
    // value of this tag is returned, def otherwise.
    //
    int
    htmTagGetNumber(const char *attributes, const char *tag, int def)
    {
        char *chPtr;
        int ret_val = def;
        if ((chPtr = htmTagGetValue(attributes, tag)) != 0) {
            ret_val = atoi(chPtr);
            delete [] chPtr;
        }
        return (ret_val);
    }


    // Retrieve the numerical value of the given tag.  If the returned no
    // is negative, the specified value was a relative number.  Otherwise
    // it's an absolute number.  If tag exists, the value of this tag is
    // returned, def otherwise.
    //
    int
    htmTagCheckNumber(const char *attributes, const char *tag, int def)
    {
        char *chPtr;
        int ret_val = def;
        if ((chPtr = htmTagGetValue(attributes, tag)) != 0) {
            // when len is negative, a percentage has been used
            if ((strpbrk(chPtr, "%")) != 0 || (strpbrk (chPtr, "*")) != 0)
                ret_val = -1*atoi(chPtr);
            else
                ret_val = atoi(chPtr);
            delete [] chPtr;
        }
        return (ret_val);
    }


    // Check whether the specified tag in the given list of attributes has
    // a certain value.  Returns true if tag exists and has the correct
    // value, false otherwise.
    //
    bool
    htmTagCheckValue(const char *attributes, const char *tag,
        const char *check)
    {
        // no sanity check, TagGetValue returns 0 if attributes is empty
        char *buf;
        if ((buf = htmTagGetValue(attributes, tag)) == 0 ||
                strcasecmp(buf, check)) {
            if (buf != 0)
                delete [] buf;
            return (false);
        }
        delete [] buf;  // fix 12-21-96-01, kdh
        return (true);
    }


    // Return any specified image alignment.
    //
    Alignment
    htmGetImageAlignment(const char *attributes)
    {
        Alignment ret_val = VALIGN_BOTTOM;

        // First check if this tag does exist
        char *buf;
        if ((buf = htmTagGetValue(attributes, "align")) == 0)
            return (ret_val);

        // transform to lowercase
        lstring::strtolower(buf);

        if (!(strcmp(buf, "left")))
            ret_val = HALIGN_LEFT;
        else if (!(strcmp(buf, "right")))
            ret_val = HALIGN_RIGHT;
        else if (!(strcmp(buf, "top")))
            ret_val = VALIGN_TOP;
        else if (!(strcmp(buf, "middle")))
            ret_val = VALIGN_MIDDLE;
        else if (!(strcmp(buf, "bottom")))
            ret_val = VALIGN_BOTTOM;
        else if (!(strcmp(buf, "baseline")))
            ret_val = VALIGN_BASELINE;

        delete [] buf;  // fix 01/12/97-01; kdh
        return (ret_val);
    }


    // Retrieve the value of the ALIGN attribute.
    //
    Alignment
    htmGetHorizontalAlignment(const char *attributes, Alignment def_align)
    {
        Alignment ret_val = def_align;

        // First check if this tag does exist
        char *buf;
        if ((buf = htmTagGetValue(attributes, "align")) == 0)
            return (ret_val);

        // transform to lowercase
        lstring::strtolower(buf);

        if (!(strcmp(buf, "center")))
            ret_val = HALIGN_CENTER;
        if (!(strcmp(buf, "middle")))
            // Firefox accepts this, may not be standars.
            ret_val = HALIGN_CENTER;
        else if (!(strcmp(buf, "right")))
            ret_val = HALIGN_RIGHT;
        else if (!(strcmp(buf, "justify")))
            ret_val = HALIGN_JUSTIFY;
        else if (!(strcmp(buf, "left")))
            ret_val = HALIGN_LEFT;

        delete [] buf;  // fix 01/12/97-01; kdh
        return (ret_val);
    }


    // Retrieve the value of the VALIGN attribute.
    //
    Alignment
    htmGetVerticalAlignment(const char *attributes, Alignment def_align)
    {
        Alignment ret_val = def_align;

        // First check if this tag does exist
        char *buf;
        if ((buf = htmTagGetValue(attributes, "valign")) == 0)
            return (ret_val);

        // transform to lowercase
        lstring::strtolower(buf);

        if (!(strcmp(buf, "top")))
            ret_val = VALIGN_TOP;
        else if (!(strcmp(buf, "middle")))
            ret_val = VALIGN_MIDDLE;
        else if (!(strcmp(buf, "bottom")))
            ret_val = VALIGN_BOTTOM;
        else if (!(strcmp(buf, "baseline")))
            ret_val = VALIGN_BASELINE;

        delete [] buf;  // fix 01/12/97-02; kdh
        return (ret_val);
    }
}


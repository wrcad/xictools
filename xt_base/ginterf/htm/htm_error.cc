
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
 * $Id: htm_error.cc,v 1.7 2015/06/17 18:36:11 stevew Exp $
 *-----------------------------------------------------------------------*/

#include "htm_widget.h"
#include "htm_string.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Display a warning message on stderr.
//
void
htmWidget::warning(const char *fn, const char *fmt, ...)
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
    strcat(buf, "\n");
    fputs(buf, stderr);
}


// Display an error message on stderr and exit.
//
void
htmWidget::fatalError(const char *fmt, ...)
{
    char buf[1024];
    va_list arg_list;
    va_start(arg_list, fmt);

    vsnprintf(buf, 1024, fmt, arg_list);
    va_end(arg_list);
    strcat(buf, "\n");
    fputs(buf, stderr);
    if (htm_if)
        htm_if->panic_callback(lstring::copy(buf));
    exit (127);
}


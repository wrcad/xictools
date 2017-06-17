
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
 * $Id: htm_string.h,v 1.4 2013/06/11 18:24:32 stevew Exp $
 *-----------------------------------------------------------------------*/

//*********************************************************************
// Module: stringutil.cc
// Description: string manipulators
//
// Exports:
// expandEscapes            : expand all escape sequences in the given text.
// ToAsciiLower             : convert a number to all lowercase ASCII.
// ToAsciiUpper             : convert a number to all uppercase ASCII.
// ToRomanLower             : convert a number to all uppercase roman numerals.
// ToRomanUpper             : convert a number to all lowercase roman numerals.
// htm_strcasestr           : case insensitive strstr.
// htm_strndup              : strndup function
// htm_csvtok               : parse comma-separated variables
//**********************************************************************

#ifndef HTM_STRING_H
#define HTM_STRING_H

#include "lstring.h"
#include <sys/types.h>

#ifdef NEED_STRERROR
extern char *sys_errlist[];
extern int errno;
#define strerror(ERRNUM) sys_errlist[ERRNUM]
#endif

namespace htm
{
    char *expandEscapes(const char*, bool, htmWidget* = 0);
    extern const char *ToAsciiLower(int);
    extern const char *ToAsciiUpper(int);
    extern const char *ToRomanUpper(int);
    extern const char *ToRomanLower(int);
    extern char *htm_strcasestr(const char*, const char*);
    extern char *htm_strndup(const char*, size_t);
    extern char *htm_csvtok(char**);
}

#endif


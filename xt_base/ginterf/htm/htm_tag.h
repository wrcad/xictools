
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
 * $Id: htm_tag.h,v 1.2 2008/07/13 06:17:44 stevew Exp $
 *-----------------------------------------------------------------------*/

//*********************************************************************
// Module: tagutil.cc
// Description: HTML tag analyzers
//
// Exports:
// htmTagCheck              : Check the existance of a tag.
// htmTagGetValue           : Get the value of a tag.
// htmTagGetNumber          : Get the numerical value of a tag.
// htmTagCheckNumber        : Get the absolute (positive no returned) or
//                            relative (negative no returned) value of a tag.
// htmTagCheckValue         : check if the given tag exists.
// htmGetImageAlignment     : Retrieve the value of the ALIGN attribute on
//                            images.
// htmGetHorizontalAlignment: Retrieve the value of the ALIGN attribute.
// htmGetVerticalAlignment  : Retrieve the value of the VALIGN attribute.
//
//**********************************************************************

#ifndef HTM_TAG_H
#define HTM_TAG_H

namespace htm
{
    extern bool htmTagCheck(const char*, const char*);
    extern char *htmTagGetValue(const char*, const char*);
    extern int htmTagGetNumber(const char*, const char*, int);
    extern int htmTagCheckNumber(const char*, const char*, int);
    extern bool htmTagCheckValue(const char*, const char*, const char*);
    extern Alignment htmGetImageAlignment(const char*);
    extern Alignment htmGetHorizontalAlignment(const char*, Alignment);
    extern Alignment htmGetVerticalAlignment(const char*, Alignment);
}

#endif


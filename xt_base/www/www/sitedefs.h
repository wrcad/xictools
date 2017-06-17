
/*========================================================================*
 *                                                                        *
 *  Copyright (c) 2016 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *  WHITELEY RESEARCH INCORPORATED PROPRIETARY SOFTWARE                   *
 *  Author:  Stephen R. Whiteley (stevew@wrcad.com)
 *                                                                        *
 *========================================================================*
 *                                                                        *
 * Back end for forms processing.
 *                                                                        *
 *========================================================================*
 $Id: sitedefs.h,v 1.1 2016/01/16 18:37:57 stevew Exp $
 *========================================================================*/

#ifndef SITEDEFS_H
#define SITEDEFS_H

// The keys directory contains a subdirectory for each key, keys
// therefor must be unique.  Each key directory will contain a file
// named in KEYDATA which consists of name=value pairs.  If a keydata
// file is updated, the existing file is saved as keydata-time, where
// time is the time() integer at the time of the backup.

#define WWWROOT     "/home/webadmin/wrcad.com"
#define KEYDIR      WWWROOT"/keys"
#define KEYDATA     "keydata"

#define LICNOTE     WWWROOT"/www/licnote.txt"
#define DEMONOTE    WWWROOT"/www/demonote.txt"
#define PWCMD       WWWROOT"/ppw/addpw"
#define VALIDATE    WWWROOT"/lic/validate"

#define MAILADDR    "mts@wrcad.com"
#define LMAILADDR   "Whiteley Research <mts@wrcad.com>"

// Prices, should match prices.in form.
#define XIVPM       60
#define XICIIPM     120
#define XICPM       220
#define WRSPM       180
#define XTPM        340

#endif


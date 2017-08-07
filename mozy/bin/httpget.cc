
/*========================================================================*
 *                                                                        *
 *  Distributed by Whiteley Research Inc., Sunnyvale, California, USA     *
 *                       http://wrcad.com                                 *
 *  Copyright (C) 2017 Whiteley Research Inc., all rights reserved.       *
 *  Author: Stephen R. Whiteley, except as indicated.                     *
 *                                                                        *
 *  As fully as possible recognizing licensing terms and conditions       *
 *  imposed by earlier work from which this work was derived, if any,     *
 *  this work is released under the Apache License, Version 2.0 (the      *
 *  "License").  You may not use this file except in compliance with      *
 *  the License, and compliance with inherited licenses which are         *
 *  specified in a sub-header below this one if applicable.  A copy       *
 *  of the License is provided with this distribution, or you may         *
 *  obtain a copy of the License at                                       *
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
 * httpget --  HTTP file transport utility.                               *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

/************************************************************************
 *
 * httpget: program to issue http/ftp requests and retrieve responses
 *
 * S. R. Whiteley <stevew@srware.com> 2/13/00
 * Copyright (C) 2000 Whiteley Research Inc., All Rights Reserved.
 * Borrowed extensively from the XmHTML widget library
 * Copyright (C) 1994-1997 by Ripley Software Development 
 * Copyright (C) 1997-1998 Richard Offer <offer@sgi.com>
 * Borrowed extensively from the Chimera WWW browser
 * Copyright (C) 1993, 1994, 1995, John D. Kilburg (john@cs.unlv.edu)
 *
 * This is similar to the httpget program supplied with the XmHTML
 * widget by Ripley Software Development, with a few additions.
 *  1) Support for FTP file retrieval.
 *  2) Optional graphical window for status reporting.
 *  3) Support for POST queries.
 *  4) Support for HTTP basic authentication.
 *  5) Support for transaction logging.
 *  6) Conversion to C++ and (hopefully) useful classes.
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
 ************************************************************************/

#include "httpget/transact.h"
#include <stdlib.h>
#include <string.h>
#include <new>

#ifdef WIN32
#include <winsock.h>
#endif

// Uncomment to build without graphics.
// #define NO_GRAPHICS

static const char *usage =
"\nhttpget %s: retrieve a document or file via HTTP or FTP\n"
"\nUsage: httpget [options] url\n"
"Where the options are:\n"
"  url        Document to receive. Given as [http://]server/[document]\n"
"              for HTTP or ftp://host/file for FTP.\n"
"  -c [file]  Name of cookie file. If not given no cookies are sent.\n"
"  -d         HTTP debug mode.\n"
"  -e         Don't reissue for HTTP location change error.\n"
"  -f[p|h]    Output format for errors: plain or HTML.\n"
#ifndef NO_GRAPHICS
"  -g[x:y]    Use graphical window [ placed at x/y ].\n"
#endif
"  -h         Show this help.\n"
"  -i         Only get HTTP document info (HEAD).\n"
"  -l [file]  Log bytes sent and received in file.\n"
"  -n         Don't print download status indication.\n"
"  -o [file]  Name of destination file. If not given stdout is used\n"
"              for HTTP or the host file name is used for FTP.\n"
"  -p [proxy] Proxy in format http://proxyaddr.com:port.\n"
"  -q [file]  Query file for POST.\n"
"  -r [num]   Retry count, default is 0.\n"
"  -s         Save HTTP error to output on failure.\n"
"  -t [num]   Timeout in seconds, default is 5.\n"
"  -x         Use HTTP error code as exit value.\n"
;


// Called if memory allocation error.
//
static void
new_err_handler()
{
    fprintf(stderr, "Fatal error: out of memory.\n");
    exit(EXIT_FAILURE);
}


int
main(int argc, char **argv)
{
    httpget::Monitor->initialize(argc, argv);

    if (argc == 1) {
        fprintf(stderr, usage, httpVersionString);
        return (EXIT_FAILURE);
    }

#ifdef WIN32
    WSADATA wsadata;
    if (WSAStartup(MAKEWORD(2, 2), &wsadata) != 0) {
        MessageBox(0,
            "Windows Socket Architecture failed to initialize",
            "ERROR", MB_ICONSTOP);
        return (EXIT_FAILURE);
    }
#endif

    std::set_new_handler(new_err_handler);
    Transaction trn;

    // parse command line
    bool return_http_error = false;  // exit status is HTTP error
    int error = trn.parse_cmd(argc, argv, &return_http_error);
    if (error) {
        fprintf(stderr, usage, httpVersionString);
        return (EXIT_FAILURE);
    }
    error = trn.transact();
    if (error != 0) {
        if (return_http_error && error >= 100)
            return (error);
        return (EXIT_FAILURE);
    }
    if (trn.return_mode() != HTTP_Skt &&
            !trn.destination() && trn.response()->data) {
        fwrite(trn.response()->data, 1, trn.response()->bytes_read, stdout);
        fflush(stdout);
    }
    return (EXIT_SUCCESS);
}


#ifdef NO_GRAPHICS

bool
http_monitor::graphics_enabled()
{
    return (false);
}


void
http_monitor::initialize(int&, char**)
{
}


void
http_monitor::setup_comm(sComm*)
{ 
}


void
http_monitor::start(Transaction*)
{   
}

#endif


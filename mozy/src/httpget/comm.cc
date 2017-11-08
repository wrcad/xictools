
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
 * comm.cc: basic C++ class library for http/ftp communications
 *
 * S. R. Whiteley <stevew@wrcad.com> 2/13/00
 * Copyright (C) 2000 Whiteley Research Inc., All Rights Reserved.
 * Borrowed extensively from the XmHTML widget library
 * Copyright (C) 1994-1997 by Ripley Software Development 
 * Copyright (C) 1997-1998 Richard Offer <offer@sgi.com>
 * Borrowed extensively from the Chimera WWW browser
 * Copyright (C) 1993, 1994, 1995, John D. Kilburg (john@cs.unlv.edu)
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

#include "config.h"
#include "comm.h"
#include "miscutil/tvals.h"
#include "miscutil/lstring.h"
#include <algorithm>
#include <errno.h>
#include <ctype.h>

#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#ifdef WIN32
#include <winsock2.h>
#else
#include <sys/param.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <fcntl.h>
#endif


#ifndef INADDR_NONE
#define INADDR_NONE 0xffffffff
#endif

#ifndef VERSION
#define VERSION "1.0"
#endif

//
// The comm package is a set of low level routines for communications,
// with medium-level support for HTTP and FTP.
//

const char *httpVersionString = VERSION;

namespace {
    // Return a copy of the string argument
    //
    inline char *
    copy(const char *str)
    {
        if (!str)
            return (0);
        char *s = new char [strlen(str) + 1];
        strcpy(s, str);
        return (s);
    }


    // Return a copy of n chars of the string argument
    //
    inline char *
    ncopy(const char *str, int n)
    {
        char *s = new char[n+1];
        strncpy(s, str, n);
        s[n] = 0;
        return (s);
    }


    char *appendHex(char*, char*);
    char *auth(const char*, const char*);
}

#define isspace8(a) ((a) < 33 && (a) > 0)


//--------------------------------------------------------------------------
// cComm class functions
//

// Static utility function.
// Unescape HTTP escaped chars, replacement is done inline.
//
void
cComm::http_unescape(char *buf)
{
    unsigned int x, y;
    for (x = 0, y = 0; buf[y]; ++x, ++y) {
        if ((buf[x] = buf[y]) == '%') {
            char digit;
            y++;
            digit = (buf[y] >= 'A' ? ((buf[y] & 0xdf)-'A')+10 : (buf[y]-'0'));
            y++;
            digit *= 16;
            digit += (buf[y] >= 'A' ? ((buf[y] & 0xdf)-'A')+10 : (buf[y]-'0'));
            buf[x] = digit;
        }
    }
    buf[x] = '\0';
}


// Static utility function.
// Function to create a fully valid QUERY_STRING from the given
// name/value pairs.  Encoding terminates when a 0 name has been
// detected.  Returns an allocated and hex-encoded QUERY_STRING.
//
char *
cComm::http_query_string(HTTPNamedValues *formdata)
{
    if (!formdata || !formdata->name)
        return (copy(""));
    // First count how many bytes we have to allocate. Each entry gets two
    // additional bytes: the equal sign and a spacer. Each entry is also
    // multiplied by three to allow full expansion.
    // Count no of entries as well.
    //
    int len = 0;
    for (int i = 0; formdata[i].name; i++) {
        len += 3*strlen(formdata[i].name);
        if (formdata[i].value)
            len += 3*strlen(formdata[i].value);
        len += 2;   // equal sign and spacer
    }
    char *data = new char[len + 1];

    // Now compose query string: append & convert to hex at the same time.
    // We can safely do this as we've already allocated room for hexadecimal
    // expansion of the *entire* query string.
    // Room for optimisation: appendHex could be done inline
    //
    char *chPtr = data;
    for (int i = 0; formdata[i].name; i++) {
        chPtr = appendHex(chPtr, formdata[i].name);
        *chPtr++ = '=';
        if (formdata[i].value)
            chPtr = appendHex(chPtr, formdata[i].value);
        *chPtr++ = '&'; // spacer
        *chPtr = 0;
    }
    // mask off last &
    data[strlen(data)-1] = '\0';

    chPtr = copy(data);
    delete [] data;
    return (chPtr);
}


// Static utility function.
// Implement the `base64' encoding as described in RFC 1521.
//

char *
cComm::http_to_base64(const unsigned char *buf, size_t len)
{  
    static const char base64[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    char *s = new char [(4*(len + 1))/3 + 1];
    char *rv = s;
    unsigned tmp;
    while (len >= 3) {
        tmp = buf[0] << 16 | buf[1] << 8 | buf[2];
        s[0] = base64[tmp >> 18];
        s[1] = base64[(tmp >> 12) & 077];
        s[2] = base64[(tmp >> 6) & 077];
        s[3] = base64[tmp & 077];
        len -= 3;  
        buf += 3;
        s += 4;
    }

    // RFC 1521 enumerates these three possibilities...
    switch(len) {
    case 2:
        tmp = buf[0] << 16 | buf[1] << 8;
        s[0] = base64[(tmp >> 18) & 077];
        s[1] = base64[(tmp >> 12) & 077];
        s[2] = base64[(tmp >> 6) & 077];
        s[3] = '=';
        s[4] = '\0';
        break;
    case 1:
        tmp = buf[0] << 16;
        s[0] = base64[(tmp >> 18) & 077];
        s[1] = base64[(tmp >> 12) & 077];
        s[2] = s[3] = '=';
        s[4] = '\0';
        break;
    case 0:
        s[0] = '\0';
        break;
    }

    return (rv);
}


// Static utility function.
// Split the url up into its constituents, returned in uret.
//
void
cComm::http_parse_url(const char *url, const char *proxy, sURL *uret)
{
    if (!url)
        return;

    // If a proxy, prepend it to url, the parse should do the right
    // thiing, i.e., take the passed url as the filename.
    sLstr lstr;
    if (proxy) {
        lstr.add(proxy);
        lstr.add_c('/');
        lstr.add(url);
        url = lstr.string();
    }

    struct part {
        part() { start = 0; len = 0; }

        int inpart(char c) {
            for (int i = 0; i < len; i++) {
                if (start[i] == c)
                    return (i);
            }
            return (-1);
        }

        const char *start;
        int len;
    } sp, up, pwp, hp, pp, fp;

    // skip leading white-space (if any)
    const char *start = url;
    while (isspace8(*start))
        start++;

    // Look for indication of a scheme
    const char *colon = strchr(start, ':');

    // Search for characters that indicate the beginning of the
    // path/params/query/fragment part.
    //
    const char *slash = strchr(start, '/');
    const char *qs = strchr(start, ';');
    if (!qs)
        qs = strchr(start, '?');
    if (!qs)
        qs = strchr(start, '#');

    // Check to see if there is a scheme.  There is a scheme only if
    // all other separators appear after the colon.
    //
    if (colon && (!slash || colon < slash) && (!qs || colon < qs)) {
        sp.start = start;
        sp.len = colon - start;
    }

    // If there is a slash then sort out the hostname and filename.
    // If there is no slash then there is no hostname but there is a
    // filename.
    //
    if (slash) {
        // Check for leading //, if present then there is a host string
        if (*(slash+1) == '/' && ((!colon && slash == start) ||
                (colon && slash == colon + 1))) {
            // Check for filename at end of host string
            while (*slash == '/')
                slash++;
            const char *fslash = strchr(slash, '/');
            if (fslash) {
                hp.start = slash;
                hp.len = fslash - slash;
                // skip over redundant shashes
                while (*(fslash+1) == '/')
                    fslash++;
                if (proxy)
                    fslash++;
                fp.start = fslash;
                fp.len = strlen(fslash);
            }
            else {
                // there is no filename
                hp.start = slash;
                hp.len = strlen(slash);
            }
        }
        else {
            // the rest is a filename because there is no // or it appears
            // after other characters
            //
            if (colon && colon < slash) {
                fp.start = colon + 1;
                fp.len = strlen(colon + 1);
            }
            else {
                fp.start = slash;
                fp.len = strlen(slash);
            }
        }
    }
    else {
        // No slashes at all so the rest must be a filename
        if (!colon) {
            fp.start = start;
            fp.len = strlen(start);
        }
        else {
            fp.start = colon + 1;
            fp.len = strlen(colon + 1);
        }
    }

    // If there is a host string then divide it into
    // username:password@hostname:port as needed.
    //
    if (hp.len != 0) {
        // Look for username:password
        int at = hp.inpart('@');
        if (at >= 0) {
            up.start = hp.start;
            up.len = at;
            hp.start += at + 1;
            hp.len -= at + 1;
            int ucolon = up.inpart(':');
            if (ucolon >= 0) {
                pwp.start = up.start + ucolon + 1;
                pwp.len = up.len - ucolon - 1;
                up.len = ucolon;
            }
        }
        // Grab the port
        int pcolon = hp.inpart(':');
        if (pcolon >= 0) {
            pp.start = hp.start + pcolon + 1;
            pp.len = hp.len - pcolon - 1;
            hp.len = pcolon;
        }
    }

    // now have all the fragments, make them into strings
    //
    uret->scheme = (sp.len > 0 ? ncopy(sp.start, sp.len) : 0);
    uret->username = (up.len > 0 ? ncopy(up.start, up.len) : 0);
    uret->password = (pwp.len > 0 ? ncopy(pwp.start, pwp.len) : 0);
    uret->hostname = (hp.len > 0 ? ncopy(hp.start, hp.len) : 0);
    uret->filename = copy(fp.len > 0 ? fp.start : "/");
    if (pp.len > 0) {
        char buf[64];
        strncpy(buf, pp.start, pp.len);
        uret->port = atoi(buf);
    }
    else
        uret->port = -1;
}
// End of static utilities.


namespace {  bool set_nonblock(int, bool); }

// Open a communications channel and return the socket file desc.  On error,
// -1 is returned, and errcode/errmsg are set.  On abort, due to a true
// return from the status print function, -2 is returned.
//
int
cComm::comm_open(char *hostname, int port)
{
    errcode = CommOK;
    errmsg = 0;
    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));

    if ((addr.sin_addr.s_addr = inet_addr(hostname)) == INADDR_NONE) {
        hostent *hp = gethostbyname(hostname);
        if (!hp) {
            errmsg = copy("host unresolved");
            errcode = BadHost;
            return (-1);
        }
        memcpy(&addr.sin_addr, hp->h_addr, hp->h_length);
    }
    addr.sin_family = AF_INET;
    addr.sin_port = htons((unsigned short)port);

    int s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s < 0) {
        errmsg = copy(strerror(errno));
        errcode = NoSocket;
        return (-1);
    }

    if (status_printf) {
        status_active = true;
        if ((*status_printf)(printarg,
            "\rContacting host %s:%d, waiting for reply", hostname, port)) {
            errcode = Aborted;
            return (-1);
        }
    }

    set_nonblock(s, true);

    long start_time = Tvals::millisec();
    bool connected = true;
    if (connect(s, (sockaddr*)&addr, sizeof(addr)) < 0) {
        connected = false;
#ifdef WIN32
        if (WSAGetLastError() == WSAEWOULDBLOCK) {
#else
        if (errno == EINPROGRESS) {
#endif
            for (;;) {
                fd_set writefds;
                FD_ZERO(&writefds);
                FD_SET(s, &writefds);
                timeval to;
                to.tv_sec = 0;
                to.tv_usec = 100000;  // 0.1 sec
                int rval;
                if ((rval = select(s + 1, 0, &writefds, 0, &to)) > 0) {    
                    connected = true;
                    break;
                }
                if (rval < 0 && errno == EINTR)
                    continue;
                if (rval == 0) {
                    if ((long)Tvals::millisec() - start_time < timeout*1000) {
                        if (status_printf) {
                            // check for interrupt
                            if ((*status_printf)(printarg, 0)) {
                                errcode = Aborted;
                                comm_close(s);
                                return (-1);
                            }
                        }
                        continue;
                    }
                    errmsg = copy("connect timed out");
                    errcode = ConnectTimeout;
                    comm_close(s);
                    return (-1);
                }
                break;
            }
        }
    }
    if (connected) {
        int err;
        socklen_t errp = sizeof(int);
        if (getsockopt(s, SOL_SOCKET, SO_ERROR, (char*)&err,
                &errp) == 0 && !err) {
            set_nonblock(s, false);
            return (s);
        }
    }
    errmsg = copy("connect failed");
    errcode = NoConnection;
    comm_close(s);
    return (-1);
}


namespace {
    // Set the socket to/from non-blocking mode, return True on error.
    //
    bool
    set_nonblock(int sfd, bool nonblock)
    {
        if (nonblock) {
#ifdef WIN32
            unsigned long ul = 1;
            return (ioctlsocket(sfd, FIONBIO, &ul));
#else
            int flags = fcntl(sfd, F_GETFL, 0);
            if (fcntl(sfd, F_SETFL, flags | O_NONBLOCK) == -1)
                return (true);
            return (false);
#endif
        }
        else {
#ifdef WIN32
            return (ioctlsocket(sfd, FIONBIO, 0));
#else
            int flags = fcntl(sfd, F_GETFL, 0);
            if (fcntl(sfd, F_SETFL, flags&(~O_NONBLOCK)) == -1)
                return (true);
            return (false);
#endif
        }
    }
}


// Bind a socket to the port, and begin listening.  Return the socket, or
// -1 if error.  On error, errcode/errmsg are set.
//
int
cComm::comm_bind(int port)
{
    errcode = CommOK;
    errmsg = 0;
    int s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s < 0) {
        errmsg = copy(strerror(errno));
        errcode = NoSocket;
        return (-1);
    }

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons((unsigned short)port);
  
    if (bind(s, (sockaddr*)&addr, sizeof(addr))) {
        errmsg = copy(strerror(errno));
        errcode = NoBind;
        comm_close(s);
        return (-1);
    }
    if (listen(s, 1) < 0) {
        errmsg = copy(strerror(errno));
        errcode = NoListen;
        comm_close(s);
        return (-1);
    }
    return (s);
}


// Accept connections on the socket.  Return the file desc, or -1 if
// error.  If error, errcode/errmsg are set.
//
int
cComm::comm_accept(int s)
{
    errcode = CommOK;
    errmsg = 0;
    sockaddr_in sock;
    socklen_t len = sizeof(sock);
    int r = accept(s, (sockaddr*)&sock, &len);
    if (r < 0) {
        errmsg = copy(strerror(errno));
        errcode = NoAccept;
        return (-1);
    }
    return (r);
}


// Static function.
// Close the socket.  This does not touch the error returns.
//
void
cComm::comm_close(int s)
{
#ifdef WIN32
    shutdown(s, SD_SEND);
    closesocket(s);
#else
    close(s);
#endif
}


// Return the host name of the present host, or 0 if error.  This does
// not touch the error returns.
//
char *
cComm::comm_hostname()
{
    char myname[100];
    if (gethostname(myname, sizeof(myname)) < 0)
        return (0);
    hostent *hp = gethostbyname(myname);
    if (!hp)
        return (0);
    return (copy(hp->h_name));
}


// Write bufferlen bytes from buffer to the file desc, return true on
// success.  If error, errcode/errmsg are set.
//
bool
cComm::comm_write(int d, const char *buffer, int bufferlen)
{
    errcode = CommOK;
    errmsg = 0;
    int writelen;
    long start_time = Tvals::millisec();
    for (int i = 0; i < bufferlen; i += writelen) {
        fd_set writefds;
        FD_ZERO(&writefds);
        FD_SET(d, &writefds);
        timeval to;
        to.tv_sec = 0;
        to.tv_usec = 100000;  // 0.1 sec
        writelen = 0;

        int rval;
        if ((rval = select(d + 1, 0, &writefds, 0, &to)) > 0) {    
            if (FD_ISSET(d, &writefds)) {
                writelen = send(d, buffer + i, bufferlen - i, 0);
                if (writelen < 0) {
                    if (errno != EAGAIN) {
                        errmsg = copy(strerror(errno));
                        errcode = NoWrite;
                        return (false);
                    }
                    writelen = 0;
                }
                start_time = Tvals::millisec();
            }
        }
        else if (rval < 0 && errno != EINTR) {
            errmsg = copy(strerror(errno));
            errcode = NoSelect;
            return (false);
        }
        else if (rval == 0) {
            if ((long)Tvals::millisec() - start_time < timeout*1000) {
                if (status_printf) {
                    // check for interrupt
                    if ((*status_printf)(printarg, 0)) {
                        errcode = Aborted;
                        return (false);
                    }
                }
                continue;
            }
            errmsg = copy("write timed out");
            errcode = Timeout;
            return (false);
        }
    }
    comm_log_print("\nWRITE\n", buffer, bufferlen);
    return (true);
}


// Read at most bufferlen bytes, or to the first newline, or EOF, into
// buffer.  If success, true is returned, otherwise the errcode/errmsg
// are set.
//
bool
cComm::comm_read_line(int d, char *buffer, int bufferlen)
{
    errcode = CommOK;
    errmsg = 0;
    if (bufferlen <= 0) {
        errmsg = copy("no bytes in buffer");
        errcode = InvalidRequest;
        return (false);
    }
    int i = 0;
    int retry_count = 0;
    long start_time = Tvals::millisec();
    for (;;) {
        if (i == bufferlen - 1) {
            buffer[i] = '\0';
            return (true);
        }

        int nfds = d;
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(d, &readfds);
        if (gfd > 0 && gfd_callback) {
            if (gfd > nfds)
                nfds = gfd;
            FD_SET(gfd, &readfds);
        }
        timeval to;
        to.tv_sec = 0;
        to.tv_usec = 100000;  // 0.1 sec

        int rval;
        if ((rval = select(nfds + 1, &readfds, 0, 0, &to)) > 0) {    
            if (gfd > 0 && gfd_callback && FD_ISSET(gfd, &readfds)) {
                if ((*gfd_callback)(gfd)) {
                    errmsg = copy("aborted");
                    errcode = Aborted;
                    return (false);
                }
            }
            if (FD_ISSET(d, &readfds)) {
                rval = recv(d, buffer + i, 1, 0);
                if (rval < 0) {
                    errmsg = copy(strerror(errno));
                    errcode = NoRead;
                    return (false);
                }
                if (rval == 0 || buffer[i] == '\n') {
                    if (rval == 0)
                        buffer[i] = '\0';
                    else
                        buffer[i + 1] = '\0';
                    comm_log_print("\nREAD LINE\n", buffer, i+1);
                    return (true);
                }
                start_time = Tvals::millisec();
                i++;
            }
        }
        else if (rval < 0 && errno != EINTR) {
            errmsg = copy(strerror(errno));
            errcode = NoSelect;
            break;
        }
        else if (rval == 0) {
            if ((long)Tvals::millisec() - start_time < timeout*1000) {
                if (status_printf) {
                    // check for interrupt
                    if ((*status_printf)(printarg, 0)) {
                        errcode = Aborted;
                        return (false);
                    }
                }
                continue;
            }
            start_time = Tvals::millisec();
            if (debug_printf)
                (*debug_printf)(printarg,
                    "read timed out after %d seconds.\n", timeout);
            if (retry_count == retry) {
                errmsg = copy("read timed out");
                errcode = Timeout;
                break;
            }
            retry_count++;
            if (debug_printf)
                (*debug_printf)(printarg, "retrying read, count is %d.\n",
                    retry_count);
        }
    }
    return (false);
}


// Read at most max bytes, and return a pointer to the data in the
// response struct.  The actual byte count is returned in the response
// struct.  This may return a partial content if an error occurs, so
// the caller should check the error flag on return.
//
bool
cComm::comm_read_buf(int d, size_t max)
{
    errcode = CommOK;
    errmsg = 0;
    response->data = 0;
    response->bytes_read = 0;
    response->read_complete = false;
    if (status_printf) {
        status_active = true;
        if ((*status_printf)(printarg,
"\r                                                                       ")) {
            errcode = Aborted;
            return (false);
        }
        if ((*status_printf)(printarg, "\rStarting Transfer...")) {
            errcode = Aborted;
            return (false);
        }
    }
    if (max > 0)
        response->data = new char[max + 1];
    for (;;) {
        size_t blsize = chunksize;
        if (max > 0 && max - response->bytes_read < blsize)
            blsize = max - response->bytes_read;
        if (blsize <= 0)
            break;
        int blen;
        char *block =
            comm_read_block(d,
                max > 0 ? response->data + response->bytes_read : 0,
                &blen, blsize, response->bytes_read, max);
        if (!block)
            // either done, or error, caller must check error code!
            break;
        if (max == 0) {
            char *tmp = new char [response->bytes_read + blen + 1];
            if (response->bytes_read)
                memcpy(tmp, response->data, response->bytes_read);
            memcpy(tmp + response->bytes_read, block, blen);
            delete [] response->data;
            response->data = tmp;
            delete [] block;
            response->bytes_read += blen;
            response->data[response->bytes_read] = 0;
        }
    }
    response->read_complete = true;
    if (status_printf) {
        status_active = false;
        (*status_printf)(printarg, "\nDone\n");
    }
    comm_log_print("\nREAD BUFFER\n", response->data, response->bytes_read);
    return (true);
}


// Read at most max bytes into the stream fp.  The number of bytes read
// is returned in len.  True is returned on success, otherwise the
// errcode/errmsg are set.  If fname is given and fp is 0, the file
// isn't opened unless there is something to write.
//
bool
cComm::comm_read_buf_to_file(int d, size_t *len, size_t max, FILE *fp,
    const char *fname)
{
    errcode = CommOK;
    errmsg = 0;
    *len = 0;
    if (status_printf) {
        status_active = true;
        if ((*status_printf)(printarg,
"\r                                                                       ")) {
            errcode = Aborted;
            return (false);
        }
        if ((*status_printf)(printarg, "\rStarting Transfer...")) {
            errcode = Aborted;
            return (false);
        }
    }
    bool need_close = false;
    for (;;) {
        size_t blsize = chunksize;
        if (max > 0 && max - *len < blsize)
            blsize = max - *len;
        if (blsize <= 0)
            break;
        int blen;
        char *block = comm_read_block(d, 0, &blen, blsize, *len, max);
        if (!block) {
            if (errcode != CommOK) {
                if (need_close)
                    fclose(fp);
                return (false);
            }
            break;
        }
        if (!fp) {
            if ((fp = fopen(fname, "wb")) == 0) {
                errcode = (ErrCode)HTTPCannotCreateFile;
                delete [] block;
                return (false);
            }
            need_close = true;
        }
        if (fwrite(block, 1, blen, fp) < (unsigned)blen) {
            errmsg = copy(strerror(errno));
            errcode = NoWrite;
            delete [] block;
            if (need_close)
                fclose(fp);
            return (false);
        }
        fflush(fp);
        delete [] block;
        (*len) += blen;
    }
    if (need_close)
        fclose(fp);
    if (status_printf) {
        status_active = false;
        (*status_printf)(printarg, "\nDone\n");
    }

    char tbuf[256];
    sprintf(tbuf, "\nREAD BUFFER TO FILE\n%ld bytes transferred\n",
        (long)*len);
    comm_log_print(tbuf, 0, 0);
    if (errcode == CommOK)
        return (true);
    return (false);
}


// Private core function to read chunksize or fewer bytes.  The actual
// number of bytes read is returned in len.  A pointer to the data is
// returned if successful, otherwise 0 is returned and errcode/errmsg
// are set to indicate the error.
//
// If a buffer is passed, it is assumed to be response->data.
//
char *
cComm::comm_read_block(int d, char *buffer, int *len, int max, int running,
    int total)
{
    static unsigned int msgcnt;

    errcode = CommOK;
    errmsg = 0;
    if (max <= 0 || max > chunksize)
        max = chunksize;
    *len = 0;
    bool freebuf = false;
    if (!buffer) {
        buffer = new char [max];
        freebuf = true;
    }
    int blen = 0;
    int retry_count = 0;
    long start_time = Tvals::millisec();
    for (int readlen = max; readlen > 0; readlen -= blen) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(d, &readfds);
        int nfds = d;
        if (gfd > 0 && gfd_callback) {
            if (gfd > nfds)
                nfds = gfd;
            FD_SET(gfd, &readfds);
        }
        timeval to;
        to.tv_sec = 0;
        to.tv_usec = 100000;  // 0.1 sec
        blen = 0;
        int rval = select(nfds + 1, &readfds, 0, 0, &to);
        if (rval > 0) {
            if (gfd > 0 && gfd_callback && FD_ISSET(gfd, &readfds)) {
                if ((*gfd_callback)(gfd)) {
                    errmsg = copy("aborted");
                    errcode = Aborted;
                    if (freebuf)
                        delete [] buffer;
                    return (0);
                }
            }
            if (FD_ISSET(d, &readfds)) {
                blen = recv(d, buffer + *len, readlen, 0);
                if (blen == 0) {
                    if (!*len) {
                        if (freebuf)
                            delete [] buffer;
                        buffer = 0;
                    }
                    break;
                }
                if (blen < 0) {
                    if (errno != EINTR && errno != EAGAIN) {
                        errmsg = copy(strerror(errno));
                        errcode = NoRead;
                        if (freebuf)
                            delete [] buffer;
                        return (0);
                    }
                    blen = 0;
                }
                else {
                    if (!freebuf) {
                        response->bytes_read += blen;
                        response->data[response->bytes_read] = 0;
                    }
                    (*len) += blen;
                    if (!(msgcnt & 0x3f) || (*len + running == total)) {
                        if (comm_status_update(*len + running, total)) {
                            if (freebuf)
                                delete [] buffer;
                            return (0);
                        }
                    }
                    msgcnt++;
                    start_time = Tvals::millisec();
                }
            }
        }
        else if (rval < 0 && errno != EINTR) {
            errmsg = copy(strerror(errno));
            errcode = NoSelect;
            if (freebuf)
                delete [] buffer;
            return (0);
        }
        else if (rval == 0) {
            if ((long)Tvals::millisec() - start_time < timeout*1000) {
                if (status_printf) {
                    // check for interrupt
                    if ((*status_printf)(printarg, 0)) {
                        errcode = Aborted;
                        if (freebuf)
                            delete [] buffer;
                        return (0);
                    }
                }
                continue;
            }
            start_time = Tvals::millisec();
            if (status_printf) {
                status_active = true;
                if ((*status_printf)(printarg, "(stalled)")) {
                    if (freebuf)
                        delete [] buffer;
                    return (0);
                }
            }
            if (debug_printf)
                (*debug_printf)(printarg,
                    "read timed out after %d seconds.\n", timeout);
            if (retry_count == retry) {
                errmsg = copy("read timed out");
                errcode = Timeout;
                if (freebuf)
                    delete [] buffer;
                return (0);
            }
            retry_count++;
            if (debug_printf)
                (*debug_printf)(printarg, "retrying read, count is %d.\n",
                    retry_count);
        }
    }
    return (buffer);
}


// Set the name of a file to use for logging data sent and received.
// This also closes any open log file.  This should be called with an
// argument of 0 to close the log file when the channel is closed.
// This does not touch the error returns.
//
void
cComm::comm_set_log(const char *fname)
{
    if (logfp) {
        if (logfp != stdout && logfp != stderr)
            fclose(logfp);
        logfp = 0;
    }
    delete [] logfile;
    logfile = copy(fname);
}


// Private function called in various places to log the message, if the
// logfile has been set.  Note that one can set the logfile to "stdout"
// or "stderr" to enable output to these channels.  This does not touch
// the error returns.
//
void
cComm::comm_log_print(const char *string, const char *buf, int len)
{
    if (logfile) {
        if (!logfp) {
            if (!strcmp(logfile, "stdout"))
                logfp = stdout;
            else if (!strcmp(logfile, "stderr"))
                logfp = stderr;
            else
                logfp = fopen(logfile, "w");
        }
        if (logfp) {
            if (string)
                fputs(string, logfp);
            if (buf && len)
                comm_log_puts(buf, len);
        }
        else {
            delete [] logfile;
            logfile = 0;
        }
    }
}


// Copy the first 1000 bytes of string into the log file.  The line
// termination characters are written as escape codes.  This does not
// touch the error returns.
//
void
cComm::comm_log_puts(const char *string, int len)
{
    if (!string || !logfp) 
        return;
    bool more = false;
    if (len > 1000) {
        len = 1000;
        more = true;
    }
    while (*string && len) {
        if (*string == '\r') {
            putc('\\', logfp);
            putc('r', logfp);
        }
        else if (*string == '\n') {
            putc('\\', logfp);
            putc('n', logfp);
            putc('\n', logfp);
        }
        else
            putc(*string, logfp);
        string++;
        len--;
    }
    if (more)
        fputs("...\n", logfp);
}


// Private function to print a running byte transfer count to the status
// output.  This is called periodically during transfer.  If the status
// print callback returns true, then the errcode is set to abort and true
// is returned.
//
bool
cComm::comm_status_update(int len, int max)
{
    if (status_printf) {
        status_active = true;
        bool rt;
        if (max) {
            int percent = (100*len)/max;
            rt = (*status_printf)(printarg, "\r    %-12d%-12d(%2d%%)",
                max, len, percent);
        }
        else
            rt = (*status_printf)(printarg, "\r    %-16d", len);
        if (rt) {
            errcode = Aborted;
            return (true);
        }
    }
    return (false);
}


//
// HTTP Protocol support
//

namespace {
    char *build_str(char*, const char*);
    char *time_cndl_string(time_t);
}


// Function to issue an http request and accept the return.  The user/pass
// are used for basic authentication.  The ret field of the request
// contains the error return code.  If there is a comm error, then
// errcode/errmsg will also be set.  If cachetime is nonzero, the (GET)
// request header will contain "If-Modified-Since ..." and the return will
// be 304 if the server file has not been modified since that date.
//
void
cComm::comm_http_request(HTTPCookieRequest *cookieReq, time_t cachetime)
{
    errcode = CommOK;
    errmsg = 0;
    if (!request->url) {
        response->status = request->status = HTTPBadURL;
        return;
    }
    if (response->type & ~(HTTPLoadToString | HTTPLoadToFile)) {
        response->status = request->status = HTTPBadLoadType;
        return;
    }

    // resolve the url
    sURL uinfo;
    http_parse_url(request->url, request->proxy, &uinfo);
    if (!uinfo.hostname) {
        response->status = request->status = HTTPBadURL;
        return;
    }

reissue_request:

    // check protocol
    if (!uinfo.scheme || strncasecmp(uinfo.scheme, "http", 4)) {
        response->status = request->status = HTTPBadProtocol;
        return;
    }
    // check hostname
    if (!uinfo.hostname || !*uinfo.hostname) {
        response->status = request->status = HTTPBadHost;
        return;
    }
    if (uinfo.port < 0)
        uinfo.port = http_port;
    int sock = comm_open(uinfo.hostname, uinfo.port);
    if (sock < 0) {
        response->status = request->status = (HTTPRequestReturn)errcode;
        comm_set_inactive();
        return;
    }

    if (debug_printf)
        (*debug_printf)(printarg, "sending request (%i)\n", request->method);

    char *cookie = 0;
    if (cookieReq && cookieReq->cookieList) 
        cookie = cookieReq->cookieList->makeCookie();
    if (debug_printf && cookie)
        (*debug_printf)(printarg, "The server wants a cookie '%s'\n", cookie);
    
    if (request->method == HTTPGET) {
        char *formStr = (request->form_data ?
            comm_http_encode_form_data(request->form_data) : 0);
        char *authStr = auth(uinfo.username, uinfo.password);
        char *hostStr = new char[strlen(uinfo.hostname) + 10];
        sprintf(hostStr, "Host: %s\r\n", uinfo.hostname);

        char *reqStr = build_str(0, GET_METHOD);
        reqStr = build_str(reqStr, uinfo.filename);
        if (formStr) {
            reqStr = build_str(reqStr, "?");
            reqStr = build_str(reqStr, formStr);
        }
        reqStr = build_str(reqStr, HTTPVERSIONHDR);
        reqStr = build_str(reqStr, USER_AGENT_PREFIX);
        reqStr = build_str(reqStr, httpVersionString);
        reqStr = build_str(reqStr, "\r\n");
        reqStr = build_str(reqStr, hostStr);
        if (cachetime) {
            char *s = time_cndl_string(cachetime);
            reqStr = build_str(reqStr, s);
            delete [] s;
        }
        if (cookie)
            reqStr = build_str(reqStr, cookie);
        if (authStr)
            reqStr = build_str(reqStr, authStr);
        reqStr = build_str(reqStr, ACCEPT);
        reqStr = build_str(reqStr, NEWLINE);

        if (!comm_write(sock, reqStr, strlen(reqStr))) {
            response->status = request->status = (HTTPRequestReturn)errcode;
            comm_close(sock);
            comm_set_inactive();
            delete [] formStr;
            delete [] authStr;
            delete [] hostStr;
            delete [] reqStr;
            delete [] cookie;
            return;
        }
        delete [] formStr;
        delete [] authStr;
        delete [] hostStr;
        delete [] reqStr;
    }
    else if (request->method == HTTPPOST) {
        char *formStr = (request->form_data ?
            comm_http_encode_form_data(request->form_data) : 0);
        char *hostStr = new char[strlen(uinfo.hostname) + 10];
        sprintf(hostStr, "Host: %s\r\n", uinfo.hostname);

        char *reqStr = build_str(0, POST_METHOD);
        reqStr = build_str(reqStr, uinfo.filename);
        reqStr = build_str(reqStr, HTTPVERSIONHDR);
        reqStr = build_str(reqStr, USER_AGENT_PREFIX);
        reqStr = build_str(reqStr, httpVersionString);
        reqStr = build_str(reqStr, "\r\n");
        reqStr = build_str(reqStr, hostStr);
        reqStr = build_str(reqStr, ACCEPT);
        if (cookie)
            reqStr = build_str(reqStr, cookie);
        reqStr = build_str(reqStr, CONTENT_LEN);
        sprintf(hostStr, "%ld\r\n", formStr ? (long)strlen(formStr) : 0);
        reqStr = build_str(reqStr, hostStr);
        reqStr = build_str(reqStr, POST_CONTENT_TYPE);
        reqStr = build_str(reqStr, NEWLINE);
        reqStr = build_str(reqStr, NEWLINE);
        if (formStr) {
            reqStr = build_str(reqStr, formStr);
            reqStr = build_str(reqStr, NEWLINE);
        }

        if (!comm_write(sock, reqStr, strlen(reqStr))) {
            response->status = request->status = (HTTPRequestReturn)errcode;
            comm_close(sock);
            comm_set_inactive();
            delete [] formStr;
            delete [] hostStr;
            delete [] reqStr;
            return;
        }
        delete [] formStr;
        delete [] hostStr;
        delete [] reqStr;
    }
    else if (request->method == HTTPHEAD) {
        char *hostStr = new char[strlen(uinfo.hostname) + 10];
        sprintf(hostStr, "Host: %s\r\n", uinfo.hostname);

        char *reqStr = build_str(0, HEAD_METHOD);
        reqStr = build_str(reqStr, uinfo.filename);
        reqStr = build_str(reqStr, HTTPVERSIONHDR);
        reqStr = build_str(reqStr, USER_AGENT_PREFIX);
        reqStr = build_str(reqStr, httpVersionString);
        reqStr = build_str(reqStr, "\r\n");
        reqStr = build_str(reqStr, hostStr);
        reqStr = build_str(reqStr, NEWLINE);

        if (!comm_write(sock, reqStr, strlen(reqStr))) {
            response->status = request->status = (HTTPRequestReturn)errcode;
            comm_close(sock);
            comm_set_inactive();
            delete [] hostStr;
            delete [] reqStr;
            return;
        }
        delete [] hostStr;
        delete [] reqStr;
    }
    else {
        comm_close(sock);
        response->status = request->status = HTTPMethodUnsupported;
        comm_set_inactive();
        return;
    }

    // read output from remote HTTP server
    if (debug_printf)
        (*debug_printf)(printarg, "awaiting input\n");

    comm_http_read_response_head(sock);
    comm_http_read_response(sock);
    if (response->type != 0)
        comm_close(sock);

    if (response->status == HTTPInvalid) {
        if (response->type == 0) {
            comm_close(sock);
            response->socknum = -1;
        }
        comm_set_inactive();
        return;
    }

    if (debug_printf) {
        for (int i = 0; i < response->num_headers; i++) {
            (*debug_printf)(printarg, "hdr %s = %s\n",
                response->headers[i].name, response->headers[i].value);
        }
    }

    if (response->status == HTTPUnauthorised) {
        // 401 Unauthorized
        // Put a simple message in the output file that indicates we
        // have to try again with user/password
        //
        if (response->type & HTTPLoadToFile) {
            for (int i = 0; i < response->num_headers; i++) {
                if (!strcmp("WWW-Authenticate", response->headers[i].name)) {
                    if (!strncasecmp("Basic", response->headers[i].value, 5)) {
                        FILE *fp = fopen(response->destination, "wb");
                        if (!fp)
                            response->status = HTTPCannotCreateFile;
                        else {
                            fputs("401 Unauthorized\n", fp);
                            fclose(fp);
                        }
                        comm_set_inactive();
                        return;
                    }
                }
            }
        }
    }

    // valid return code?
    if (response->status > 199 && response->status < 299) {
        // get or post include data, which head does not contain
        if (request->method != HTTPHEAD) {
            if (cookieReq) {
                // parse the headers for any cookies
                for (int i = 0; i < response->num_headers; i++ ) { 
                    if (!strcasecmp(response->headers[i].name, "Set-Cookie"))
                        cookieReq->set(SetCookie, response->headers[i].value,
                            uinfo.hostname);
                    else if (!strcasecmp(response->headers[i].name,
                            "Set-Cookie2"))
                        cookieReq->set(SetCookie2, response->headers[i].value,
                            uinfo.hostname);
                }           
            }
        }
        else {
            // store data in string (most likely this was a cgi request)
            if (response->type == 0) {
                comm_close(sock);
                response->socknum = -1;
            }
            else if (response->type & HTTPLoadToFile) {
                // this was a request for a remote file. Save it
                FILE *fp = fopen(response->destination, "wb");
                if (!fp)
                    response->status = HTTPCannotCreateFile;
                else {
                    for (int i = 0; i < response->num_headers; i++) {
                        fprintf(fp, "%s = %s\n", response->headers[i].name,
                            response->headers[i].value);
                    }
                    // flush data
                    fflush(fp);
                    fclose(fp);
                }
            }
        }
    }
    else {
        // if the URL has moved (_or_ the user left off a trailing '/' from a
        // directory request), then look in the Location: header for the
        // correct URL and re-issue the request

        if (response->type == 0) {
            comm_close(sock);
            response->socknum = -1;
        }
        if (response->status == 301 || response->status == 302) {

            // parse the headers for any cookies
            if (cookieReq) {
                for (int i = 0; i < response->num_headers; i++ ) { 
                    if (!strcasecmp(response->headers[i].name, "Set-Cookie"))
                        cookieReq->set(SetCookie, response->headers[i].value,
                            uinfo.hostname);
                    else if (!strcasecmp(response->headers[i].name,
                            "Set-Cookie2"))
                        cookieReq->set(SetCookie2, response->headers[i].value,
                            uinfo.hostname);
                }           
            }

            for (int i = 0; i < response->num_headers; i++) {
                if (!strcasecmp(response->headers[i].name, "location")) {
                    if (http_no_reissue) {
                        if (response->type & HTTPLoadToFile) {
                            FILE *fp = fopen(response->destination, "wb");
                            if (!fp)
                                response->status = HTTPCannotCreateFile;
                            else {
                                fprintf(fp, "%d Location %s\n",
                                    response->status,
                                    response->headers[i].value);
                                fclose(fp);
                                comm_set_inactive();
                                return;
                            }
                        }
                    }
                    uinfo.clear();
                    http_parse_url(response->headers[i].value, 0, &uinfo);
                    // Update the URL that was requested to point to the
                    // correct one
                    delete [] request->url;
                    request->url = copy(response->headers[i].value);
                    goto reissue_request;
                }
            }
        }
    }
    comm_set_inactive();
}


namespace {
    // Append s2 to s1, free s1.
    //
    char *
    build_str(char *s1, const char *s2)
    {
        if (!s2 || !*s2)
            return (s1);
        if (!s1 || !*s1)
            return (copy(s2));

        char *s, *str;
        s = str = new char[strlen(s1) + strlen(s2) + 1];
        const char *t = s1;
        while (*s1) *s++ = *s1++;
        while (*s2) *s++ = *s2++;
        *s = '\0';
        delete [] t;
        return (str);
    }


    const char *days[] =
        { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
    const char *mons[] = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

    // Return the (copied) time conditional header for GET.
    //
    char *
    time_cndl_string(time_t tval)
    {
        // Sun, 06 Nov 1994 08:49:37 GMT    ; RFC 822, updated by RFC 1123
        char buf[256];
        struct tm *tm = gmtime(&tval);
        sprintf(buf,
            "If-Modified-Since: %s, %02d %s %4d %02d:%02d:%02d GMT\r\n",
            days[tm->tm_wday], tm->tm_mday, mons[tm->tm_mon],
            tm->tm_year + 1900, tm->tm_hour, tm->tm_min, tm->tm_sec);
        return (copy(buf));
    }
}


// Private function to read the response headers to an http request.
//
void
cComm::comm_http_read_response_head(int sock)
{
    char buf[4096];
    response->status = HTTPSuccess;
    response->http_version = 0;
    delete [] response->headers;
    response->headers = 0;
    for (;;) {
        if (!comm_read_line(sock, buf, 4096)) {
            response->status = (HTTPRequestReturn)errcode;
            return;
        }
        if (comm_http_parse_response_line(buf))
            break;
    }
    if (response->status == HTTPInvalid)
        return;
    if (debug_printf)
        (*debug_printf)(printarg, "content length %d.\n",
            response->content_length);
}


// Private function to read the response to an http request.
//
void
cComm::comm_http_read_response(int sock)
{
    if (response->status == HTTPInvalid)
        return;
    if (response->status == 301 || response->status == 302)
        // relocation, skip reading
        return;

    delete [] response->data;
    response->data = 0;
    response->bytes_read = 0;
    if (response->type == 0) {
        response->socknum = sock;
        return;
    }
    if (response->type & HTTPLoadToString) {
        if (response->type & HTTPLoadToFile)
            // reduce block size for progressive image loading
            chunksize = 2048;
        else
            chunksize = CHUNKSIZE;
        comm_read_buf(sock, response->content_length);
        if (response->data && response->bytes_read > 0 && errcode != CommOK)
            response->status = HTTPPartialContent;
        else if (!response->data)
            response->status = (HTTPRequestReturn)errcode;
        if (response->type & HTTPLoadToFile) {
            FILE *fp = fopen(response->destination, "wb");
            if (!fp)
                response->status = HTTPCannotCreateFile;
            else {
                if (response->bytes_read)
                    fwrite(response->data, 1, response->bytes_read, fp);
                fclose(fp);
            }
        }
    }
    else if (response->type & HTTPLoadToFile) {
        size_t len;
        chunksize = CHUNKSIZE;
        if (!comm_read_buf_to_file(sock, &len, response->content_length,
                0, response->destination))
            response->status = (HTTPRequestReturn)errcode;
    }
}


namespace {
    // Null trailing white space in string.
    //
    inline void
    eat_tws(char *string)
    {
        if (string) {
            char *s = string + strlen(string) - 1;
            while (s >= string && isspace(*s))
                *s-- = 0;
        }
    }
}


// Private function to parse a line from the response.  The struct has
// the status code set to invalid if there was a syntax error.  If
// true is returned, the remaining data should be read into the
// content field.
//
bool
cComm::comm_http_parse_response_line(char *string)
{
    if (!response->http_version) {
        response->http_version = HTTP_VERSION_09;
        if (strncasecmp(string, "HTTP", 4)) {
            response->status = HTTPInvalid;
            return (true);
        }
        int ver = 0, code;
        if (sscanf(string, "HTTP/1.%d %d", &ver, &code) != 2) {
            if (ver == 0)
                response->http_version = HTTP_VERSION_10;
            else if (ver == 1)
                response->http_version = HTTP_VERSION_11;
            response->status = HTTPInvalid;
            return (true);
        }

        if (debug_printf)
            (*debug_printf)(printarg, "\nHTTP 1.%d return code = %d\n",
                ver, code);
        if (ver == 0)
            response->http_version = HTTP_VERSION_10;
        else
            response->http_version = HTTP_VERSION_11;
        response->status = (HTTPRequestReturn)code;
    }
    else {
        char *str;
        char *colon = strchr(string, ':');
        if (!colon) {
            if ((string[0] == '\r' && string[1] == '\n') || string[0] == '\n')
                return (true);
            return (false);
        }

        if (!response->headers)
            response->headers = new HTTPNamedValues[1];
        else {
            HTTPNamedValues *tmp =
                new HTTPNamedValues[response->num_headers + 1];
            for (int i = 0; i < response->num_headers; i++) {
                tmp[i] = response->headers[i];
                response->headers[i].name = 0;
                response->headers[i].value = 0;
            }
            delete [] response->headers;
            response->headers = tmp;
        }
        str = ncopy(string, colon - string);
        eat_tws(str);
        response->headers[response->num_headers].name = str;
        str = colon + 1;
        while (isspace(*str))
            str++;
        str = copy(str);
        eat_tws(str);
        response->headers[response->num_headers].value = str;
        if (!strcasecmp(response->headers[response->num_headers].name,
                "Content-length"))
            response->content_length =
                atoi(response->headers[response->num_headers].value);

        response->num_headers++;
    }
    return (false);
}


// Private function to create a fully valid QUERY_STRING from the given
// name/value pairs.  Encoding terminates when a 0 name has been detected.
// Returns an allocated and hex-encoded QUERY_STRING.
//
char *
cComm::comm_http_encode_form_data(HTTPNamedValues *formdata)
{
    char *data = http_query_string(formdata);
    if (debug_printf) {
        (*debug_printf)(printarg,
            "http_encode_form_data, string length: %d\n", strlen(data));
        (*debug_printf)(printarg, "return value: %s\n", data);
    }
    return (data);
}


//
//  FTP protocol support
//

// From uinfo, retrieve filename from hostname:port, using the
// username/password for login.  The file is copied into the stream
// fp, if fp is nonzero, or destination.  If FTP_ERR is returned, the
// errcode/errmsg are set.  If FTP_DIR is returned, the filename was a
// directory on hostname, and an HTML-formatted listing of the
// directory is saved in errmsg.
//
FTP_RET
cComm::comm_ftp_request(sURL *uinfo, FILE *fp, const char *destination)
{
    errcode = CommOK;
    errmsg = 0;
    if (uinfo->port < 0)
        uinfo->port = ftp_port;
    int s = comm_open(uinfo->hostname, uinfo->port);
    if (s < 0) {
        comm_set_inactive();
        return (FTP_ERR);
    }

    // Take care of the greeting.
    //
    char *emsg;
    if (comm_ftp_soak(s, 0, &emsg) >= 400) {
        comm_close(s);
        errmsg = emsg;
        comm_set_inactive();
        return (FTP_ERR);
    }
    delete [] emsg;

    // Send the user name
    //
    char buf[256];
    if (!uinfo->username || !*uinfo->username)
        uinfo->username = copy("ftp");
    sprintf(buf, "USER %s\r\n", uinfo->username);
    if (comm_ftp_soak(s, buf, &emsg) >= 400) {
        comm_close(s);
        errmsg = emsg;
        comm_set_inactive();
        return (FTP_ERR);
    }
    delete [] emsg;

    // Send the password
    //
    if (!uinfo->password || !*uinfo->password)
        uinfo->password = copy("nobody@nowhere.com");
    sprintf(buf, "PASS %s\r\n", uinfo->password);
    if (comm_ftp_soak(s, buf, &emsg) >= 400) {
        comm_close(s);
        errmsg = emsg;
        comm_set_inactive();
        return (FTP_ERR);
    }
    delete [] emsg;

    // Set binary transfers
    //
    if (comm_ftp_soak(s, "TYPE I\r\n", &emsg) >= 400) {
        comm_close(s);
        errmsg = emsg;
        comm_set_inactive();
        return (FTP_ERR);
    }
    delete [] emsg;

    // Set passive mode and grab the port information
    //
    if (comm_ftp_soak(s, "PASV\r\n", &emsg) >= 400) {
        comm_close(s);
        errmsg = emsg;
        comm_set_inactive();
        return (FTP_ERR);
    }

    int h0, h1, h2, h3, p0, p1, reply;
    int n = sscanf(emsg, "%d %*[^(] (%d,%d,%d,%d,%d,%d)", &reply, &h0, &h1,
        &h2, &h3, &p0, &p1);
    if (n != 7 || reply != 227) {
        comm_close(s);
        errmsg = emsg;
        comm_set_inactive();
        return (FTP_ERR);
    }
    delete [] emsg;

    char data_hostname[48];
    sprintf(data_hostname, "%d.%d.%d.%d", h0, h1, h2, h3);

    // Open a data connection
    //
    int data_port = (p0 << 8) + p1;
    int d = comm_open(data_hostname, data_port);
    if (d < 0) {
        comm_close(s);
        comm_set_inactive();
        return (FTP_ERR);
    }

    sprintf(buf, "SIZE %s\r\n", uinfo->filename);
    if (comm_ftp_soak(s, buf, &emsg) == 999) {
        comm_close(s);
        comm_close(d);
        errmsg = emsg;
        comm_set_inactive();
        return (FTP_ERR);
    }
    int size = 0;
    reply = 0;
    sscanf(emsg, "%d %d", &reply, &size);
    if (reply != 213)
        size = 0;
    delete [] emsg;

    // Try to retrieve the file
    //
    sprintf(buf, "RETR %s\r\n", uinfo->filename);
    if ((reply = comm_ftp_soak(s, buf, &emsg)) == 999) {    
        comm_close(s);
        comm_close(d);
        errmsg = emsg;
        comm_set_inactive();
        return (FTP_ERR);
    }
    delete [] emsg;

    // If the retrieve fails try to treat the file as a directory.
    // If the file is a directory then ask for a listing.
    //
    if (reply >= 400) {
        // Try to read the file as a directory.
        //
        sprintf(buf, "CWD %s\r\n", uinfo->filename);
        if (comm_ftp_soak(s, buf, &emsg) >= 400) {
            comm_close(d);
            comm_close(s);
            errmsg = emsg;
            comm_set_inactive();
            return (FTP_ERR);
        }
        delete [] emsg;

        int rval;
        if ((rval = comm_ftp_soak(s, "NLST\r\n", &emsg)) >= 400) {
            comm_close(s);
            comm_close(d);
            if (rval == 550) {
                // no files found
                delete [] emsg;
                errmsg = comm_ftp_dir(0, uinfo->hostname, ftp_port,
                    uinfo->filename);
                comm_set_inactive();
                return (FTP_DIR);
            }
            errmsg = emsg;
            comm_set_inactive();
            return (FTP_ERR);
        }
        delete [] emsg;

        char *t = comm_ftp_dir(d, uinfo->hostname, ftp_port,
            uinfo->filename);
        comm_close(d);
        comm_close(s);
        if (t) {
            errmsg = t;
            comm_set_inactive();
            return (FTP_DIR);
        }
        comm_set_inactive();
        return (FTP_ERR);
    }

    // Read file from the FTP host
    //
    size_t tlen;
    if (!comm_read_buf_to_file(d, &tlen, size, fp, destination)) {
        errmsg = copy("transfer failed");
        comm_close(d);
        comm_close(s);
        comm_set_inactive();
        return (FTP_ERR);
    }
    comm_close(d);
    comm_close(s);

    comm_set_inactive();
    return (FTP_OK);
}


// Private function to issue a message (msg) to the server and read the
// response (in emsg).  The error code 999 is returned on error.
//
int
cComm::comm_ftp_soak(int s, const char *msg, char **emsg)
{
    if (msg && !comm_write(s, msg, strlen(msg)))
        return (999);

    if (emsg)
        *emsg = 0;
    char buffer[BUFSIZ];
    char *t = 0;
    int tlen = 0, btlen = 0;
    int code = -1;
    bool rval;
    while ((rval = comm_read_line(s, buffer, sizeof(buffer))) == true) {
        int blen = strlen(buffer);
        tlen += blen;
        if (t) {
            char *tmp = new char [tlen+1];
            memcpy(tmp, t, btlen);
            delete [] t;
            t = tmp;
        }
        else
            t = new char [tlen+1];
        memcpy(t + btlen, buffer, blen);
        t[tlen] = '\0';
        btlen = tlen;

        if (code == -1)
            code = atoi(buffer);
        if (buffer[3] == ' ' && buffer[0] > ' ')
            break;
    }

    if (emsg)
        *emsg = t;
    else
        delete [] t;

    if (rval == false && code == -1)
        return (999);

    return (code);
}


namespace {
    inline bool
    ftp_strcmp(const char *a, const char *b)
    {
        return (strcmp(a, b) < 0);
    }
}


// Private function to read a directory listing from the FTP server, sort,
// and return an HTML text page of the listing.
//
char *
cComm::comm_ftp_dir(int d, char *hostname, int, char *filename)
{
    const char *header =
        "<html><body bgcolor=white>"
        "<title>FTP directory %s on %s</title>\n"
        "<h1>FTP directory %s</h1>\n";
    const char *entry = "<li><a href=ftp://%s/%s/%s>%s</a>\n";
    const char *entryrt = "<li><a href=ftp://%s/%s>%s</a>\n";
    const char *footer = "</body></html>\n";
    const char *nfmsg = "Directory is empty.\n";

    int count = 0;
    int size = 16;
    char **sa = new char*[size];
    // if d == 0 we already know that there are no files
    if (d > 0) {
        char buffer[BUFSIZ];
        while (comm_read_line(d, buffer, sizeof(buffer))) {
            if (!*buffer)
                break;
            if (count >= size) {
                size *= 2;
                char **tmp = new char*[size];
                memcpy(tmp, sa, count*sizeof(char*));
                delete [] sa;
                sa = tmp;
            }
            char *cp;
            for (cp = buffer; *cp; cp++) {
                if (isspace(*cp))
                    break;
            }
            *cp = '\0';
            cp = buffer;
            if (*cp == '/')
                cp++;
            sa[count] = copy(cp);
            count++;
        }
    }

    if (count > 1)
        std::sort(sa, sa + count, ftp_strcmp);

    int flen = strlen(header) + 2*strlen(filename) + strlen(hostname) - 6;
    int entrylen = strlen(entry) + strlen(filename) + strlen(hostname) - 8;
    if (count) {
        flen += 11;
        for (int i = 0; i < count; i++)
            flen += entrylen + 2*strlen(sa[i]);
    }
    else
        flen += strlen(nfmsg);
    flen += strlen(footer) + 1;

    char *f = new char[flen];
    char *e = f;
    sprintf(e, header, filename, hostname, filename);
    while (*e) e++;

    if (*filename == '/')
        filename++;
    if (count) {
        strcpy(e, "<ul>\n");
        while (*e) e++;
        for (int i = 0; i < count; i++) {
            if (*filename)
                sprintf(e, entry, hostname, filename, sa[i], sa[i]);
            else
                sprintf(e, entryrt, hostname, sa[i], sa[i]);
            while (*e) e++;
            delete [] sa[i];
        }
        strcpy(e, "</ul>\n");
        while (*e) e++;
    }
    if (count == 0) {
        strcpy(e, nfmsg);
        while (*e) e++;
    }
    strcpy(e, footer);
    delete [] sa;

    return (f);
}
// End cComm functions


namespace {
    // Fast lookup table to determine which characters should be left
    // alone and which should be encoded.
    //
    const unsigned char allow[97] =
    {// 0 1 2 3 4 5 6 7 8 9 A B C D E F
        0,0,0,0,0,0,0,0,0,0,1,1,0,1,1,1,    // 2x   !"#$%&'()*+,-./
        1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,    // 3x  0123456789:;<=>?
        1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,    // 4x  @ABCDEFGHIJKLMNO
        1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,1,    // 5X  PQRSTUVWXYZ[\]^_
        0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,    // 6x  `abcdefghijklmno
        1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0     // 7X  pqrstuvwxyz{\}~  DEL
    };

    const char *hex = "0123456789ABCDEF";

    // Name:         appendHex
    // Return Type:  char*
    // Description:  appends src to dest, translating certain chars to their
    //               hexadecimal representation as we do;
    // In: 
    //   dest:       destination buffer. This buffer must be large enough to
    //               contain the expanded source text;
    //   src:        text to be appended;
    // Returns:
    //   a ptr pointing to the next available position in dest.
    //
    char *
    appendHex(char *dest, char *src)
    {
        char *ptr, *chPtr;
        for (ptr = dest, chPtr = src; *chPtr != '\0'; chPtr++) {
            // no negative values
            int c = (int)((unsigned char)(*chPtr));
            if (*chPtr == ' ')   // bloody exception
                *ptr++ = '+';
            else if (c >= 32 && c <= 127 && allow[c-32])
                *ptr++ = *chPtr; // acceptable char
            else {
                *ptr++ = '%';   // hex is following
                *ptr++ = hex[c >> 4];
                *ptr++ = hex[c & 15];
            }
        }
        return (ptr);
    }


    // Return the encoded user/passwd "Authorization" header.
    //
    char *
    auth(const char *user, const char *passwd)
    {
        if (!user || !passwd)
            return (0);
        char params[256];
        strcpy(params, user);
        strcat(params, ":");
        strcat(params, passwd);
        char *enc = cComm::http_to_base64((unsigned char*)params,
            strlen(params));
        char *hdr = new char [strlen(enc) + 32];
        strcpy(hdr, "Authorization: Basic ");
        strcat(hdr, enc);
        strcat(hdr, "\r\n");
        delete [] enc;
        return (hdr);
    }
}


//--------------------------------------------------------------------------
// HTTPResponse functions
//

// Return a const char error message based on the return status.
//
const char*
HTTPResponse::errorString() const
{
    switch (status) {
    // 0 and up (client messages)
    case HTTPInvalid:
        return ("Invalid request (client failure)");
    case HTTPAborted:
        return ("Transaction aborted");
    case HTTPBadProtocol:
        return ("Invalid protocol requested (client failure)");
    case HTTPBadHost:
        return ("Invalid hostname (client failure)");
    case HTTPNoSocket:
        return ("Could not open socket (client failure)");
    case HTTPNoConnection:
        return ("Not connected (client failure)");
    case HTTPNoAccept:
        return ("Accept call failed (client failure)");
    case HTTPNoBind:
        return ("Bind call failed (client failure)");
    case HTTPNoListen:
        return ("Listen call failed (client failure)");
    case HTTPNoRead:
        return ("Read call failed (client failure)");
    case HTTPNoWrite:
        return ("Write call failed (client failure)");
    case HTTPNoSelect:
        return ("Select call failed (client failure)");
    case HTTPConnectTimeout:
        return ("Could not connect: timed out (client failure)");
    case HTTPTimeout:
        return ("Connection timed out");
    case HTTPInvalidRequest:
        return ("Invalid request to operation (client failure)");
    case HTTPBadURL:
        return ("Invalid URL (client failure)");
    case HTTPBadLoadType:
        return ("Invalid load type (client failure)");
    case HTTPMethodUnsupported:
        return ("Unsupported method (client failure)");
    case HTTPBadHttp10:
        return ("Invalid HTTP/1.0 request (client failure)");
    case HTTPCannotCreateFile:
        return ("Could not create file (client failure)");

    // 100 and up (informative messages)
    case HTTPContinue:
        return ("Continue");
    case HTTPSwitchProtocols:
        return ("Bad protocol, switch required");

    // 200 and up (request succeeded)
    case HTTPSuccess:
        return ("No error");
    case HTTPCreated:
        return ("Document created");
    case HTTPAccepted:
        return ("Request accepted");
    case HTTPNonAuthoritativeInfo:
        return ("Non-authoritative information");
    case HTTPNoContent:
        return ("Document is empty");
    case HTTPResetContent:
        return ("Content has been reset");
    case HTTPPartialContent:
        return ("Partial content");

    // 300 and up (non-fatal errors, retry possible)
    case HTTPMultipleChoices:
        return ("Request not unique, multiple choices possible");
    case HTTPPermMoved:
        return ("Document has been permanently removed");
    case HTTPTempMoved:
        return ("Document has been temporarely moved");
    case HTTPSeeOther:
        return ("Site has move");
    case HTTPNotModified:
        return ("Document not modified since last access");
    case HTTPUseProxy:
        return ("Document only accessible through proxy");

    // 400 and up (fatal request errors)
    case HTTPBadRequest:
        return ("Invalid HTTP request");
    case HTTPUnauthorised:
        return ("Client not authorized");
    case HTTPPaymentReq:
        return ("Payment required");
    case HTTPForbidden:
        return ("Access forbidden");
    case HTTPNotFound:
        return ("Document not found");
    case HTTPMethodNotAllowed:
        return ("Access method not allowed");
    case HTTPNotAcceptable:
        return ("Unacceptable request");
    case HTTPProxyAuthReq:
        return ("Proxy authorization required");
    case HTTPRequestTimeOut:
        return ("Timed out");
    case HTTPConflict:
        return ("Conflict of interest");
    case HTTPGone:
        return ("Document has moved");
    case HTTPLengthReq:
        return ("Invalid request length");
    case HTTPPreCondFailed:
        return ("Condition failed");
    case HTTPReqEntityTooBig:
        return ("Request entity too large");
    case HTTPURITooBig:
        return ("URI specification too big");
    case HTTPUnsupportedMediaType:
        return ("Unsupported media type");

    // 500 and up (server errors)
    case HTTPInternalServerError:
        return ("Internal server error");
    case HTTPNotImplemented:
        return ("Method not implemented");
    case HTTPBadGateway:
        return ("Invalid gateway");
    case HTTPServiceUnavailable:
        return ("Service unavailable");
    case HTTPGatewayTimeOut:
        return ("Gateway timed out");
    case HTTPHTTPVersionNotSupported:
        return ("Unsupported HTPP version");

    default:
        return ("unknown error");
    }
}
// End HTTPResponse functions


//--------------------------------------------------------------------------
// HTTPCookieRequest functions
//

namespace { time_t parse_date(char*); }


// The "SetCookie" function.
//
void 
HTTPCookieRequest::set(int type, char *string, char *host)
{
    HTTPCookieList *cLP = setCookie;
    char *str = string;
    
    if (!cLP)
        setCookie = cLP = new HTTPCookieList;
    else if (!cLP->next) {
        cLP->next = new HTTPCookieList;
        cLP = cLP->next;
    }
    cLP->cookie = new HTTPCookie;

    int i = 0;
    while (str[i]) {
        char *name = &str[i];
        while (str[i] && str[i] != '=')
            i++;
        char *value = (char*)"";
        if (str[i]) {
            str[i] = '\0';
            i++;
            value = &str[i];
            while (str[i] && str[i] != ';')
                i++;
            if (str[i])
                str[i] = '\0';
            i++;
        }

        while (*name && *name == ' ')
            name++;
        char *t = name + strlen(name) - 1;
        while (t >= name && *t == ' ')
            *t-- = 0;
        while (*value && *value == ' ')
            value++;
        t = value + strlen(value) - 1;
        while (t >= value && *t == ' ')
            *t-- = 0;
            
        if (!cLP->cookie->cookie.name) {
            cLP->cookie->cookie.name = copy(name);
            cLP->cookie->cookie.value = copy(value);
            continue;
        }

        if (*value) {
            if (type == SetCookie2) {
                if (!strcasecmp(name,"comment")) 
                    cLP->cookie->comment = copy(value);
                else if (!strcasecmp(name,"commenturl"))
                    cLP->cookie->commentURL = copy(value);
                else if (!strcasecmp(name,"discard"))
                    cLP->cookie->discard = 1;
                else if (!strcasecmp(name,"domain")) {
                    // if first char isn't a dot add one - spec says this
                    if (value[0] != '.') { 
                        char d[128];
                        d[0] = '.';
                        strcat(d,value); 
                        cLP->cookie->domain = copy(d);
                    }
                    else
                        cLP->cookie->domain = copy(value);
                }
                else if (!strcasecmp(name,"max-age")) {
                    int j = atoi(value);
                    if (j != 0)
                        cLP->cookie->expires = time(0) + j;
                    /*
                    else {
                        // TODO dump cookie immediately
                        ;
                    }
                    */
                }
                else if (!strcasecmp(name,"path")) 
                    cLP->cookie->path = copy(value);   
                /*
                else if (!strcasecmp(name,"port")) {
                    // TODO
                    ;
                }
                */
                else if (!strcasecmp(name,"version"))
                    cLP->cookie->version = atoi(value);
            }
            else {
                // Netscape format cookie
                if (!strcasecmp(name,"domain")) {
                    // if first char isn't a dot add one - spec says this
                    if (value[0] != '.') { 
                        char d[128];
                        d[0] = '.';
                        strcat(d,value); 
                        cLP->cookie->domain = copy(d);
                    }
                    else
                        cLP->cookie->domain = copy(value);
                }
                else if (!strcasecmp(name,"path")) 
                    cLP->cookie->path = copy(value);   
                else if (!strcasecmp(name, "expires")) {
                    time_t tt = parse_date(value);
                    if (tt != -1)
                        cLP->cookie->expires = tt;
                }
            }
        }
        else {
            // a name that has no associated value
            if (!strcasecmp(name, "secure"))
                cLP->cookie->secure = 1;
            if (type == SetCookie2) {
                if (!strcasecmp(name,"discard"))
                    cLP->cookie->discard = 1;
            }           
            else {
                if (!strncasecmp(name, "expires", 7)) {
                    time_t tt = parse_date(name + 8);
                    if (tt != -1)
                        cLP->cookie->expires = tt;
                }
            }
        }
    }
    
    if (!cLP->cookie->domain)
        cLP->cookie->domain = copy(host);
    if (!cLP->cookie->path)
        cLP->cookie->path = copy("/"); 
}
// End HTTPCookieRequest functions


namespace {
    // Parse the nasty date format, return time code.
    //
    time_t
    parse_date(char *str)
    {
        // can be day-min-year or day min year, get rid of -'s and commas
        for (char *t = str; *t; t++)
            if (*t == '-' || *t == ',')
                *t = ' ';

        char day[64], month[64];
        int date, yr, hr, min, sec;
        sscanf(str, "%s %d %s %d %d:%d:%d",
            day, &date, month, &yr, &hr, &min, &sec);

        struct tm timeStruct;
        memset(&timeStruct, 0, sizeof(struct tm));
        int mon = 0;
        if (month[0] == 'J') {
            if (month[1] == 'a')
                mon = 0;
            else {
                if (month[2] == 'n')
                    mon = 5;
                else
                    mon = 6;
            }
        }
        else if (month[0] == 'F')
            mon = 1;
        else if (month[0] == 'M') {
            if (month[2] == 'r')
                mon = 2;
            else
                mon = 4;
        }
        else if (month[0] == 'A') {
            if (month[1] == 'p')
                mon = 3;
            else
                mon= 7;
        }
        else if (month[0] == 'S')
            mon = 8;
        else if (month[0] == 'O')
            mon = 9;
        else if (month[0] == 'N')
            mon = 10;
        else if (month[0] == 'D')
            mon = 11;

        timeStruct.tm_sec = sec;
        timeStruct.tm_min = min; 
        timeStruct.tm_hour = hr;
        timeStruct.tm_mday = date;
        timeStruct.tm_mon = mon;
        timeStruct.tm_year = yr - 1900;

        return (mktime(&timeStruct));
    }
}


//--------------------------------------------------------------------------
// HTTPCookieList functions
//

// Create the cookie text
//
char *
HTTPCookieList::makeCookie()
{
    char cbuf[4097]; // max length of a cookie

    // tell the server that we support RFC2109 style cookies (if the server 
    // does as well, it will reply with Set-Cookie2.
    // 
    // Note: assume that if one cookie is version 0 they all will be (for
    // this URL).  This IS an ASSUMPTION (the spec says that it is allowable
    // to send two cookies of different versions, but is this likely ?
    // 
    if (cookie->version == 0)
        sprintf(cbuf,
            "Cookie2: $Version=\"1\"\r\nCookie: $Version=\"%d\"; ",
            cookie->version);
    else
        sprintf(cbuf,
            "Cookie: $Version=\"%d\"; ", cookie->version);

    HTTPCookieList *cl = this;
    while (cl) {
        strcat(cbuf, cl->cookie->cookie.name);    
        strcat(cbuf, "=");
        strcat(cbuf, cl->cookie->cookie.value);   
        strcat(cbuf, ";");

        if (cl->cookie->type == SetCookie2) {
            if (cl->cookie->path) {
                strcat(cbuf, "$Path");
                strcat(cbuf, "=");
                strcat(cbuf, cl->cookie->path);   
                strcat(cbuf, ";");
            }
            if (cl->cookie->domain) {
                strcat(cbuf, "$Domain");
                strcat(cbuf, "=");
                strcat(cbuf, cl->cookie->domain); 
                strcat(cbuf, ";");
            }
            // ToDo: Port support
        }
        cl = cl->next;
    }
    strcat(cbuf, "\r\n");
    return (copy(cbuf));
}
// End HTTPCookieList functions


//--------------------------------------------------------------------------
// HTTPCookieCache functions
//

namespace {
    char *token(char**);


    // Cookie sorting comparison function.
    //
    inline bool
    sortCookies(const HTTPCookie *c1, const HTTPCookie *c2)
    {
        int c = 0;
        if (c1->domain && c2->domain) 
            c = strcasecmp(c1->domain, c2->domain);
        
        if (c == 0 && c2->path && c1->path) 
            // reverse sense of sort
            return (strcmp(c2->path, c1->path) < 0); 
        return (c < 0);
    }
}

#define	stringToBoolean(s) \
	((s) && *(s) ? (*(s) == 'T' || *(s) == 't' || *(s) == 'y' ? 1 : 0) : 0)


HTTPCookieCache::HTTPCookieCache(const char *fname)
{
    cookies = 0;
    ncookies = 0;
    filename = copy(fname);

    FILE *fp = fopen(filename, "rb");
    if (fp) {
        int fileType = NetscapeCookieFile;

        bool firstline = true;
        char line[4096]; // maximum length of a cookie as defined by the spec
        while (fgets(line, 4096, fp) != 0) {

            // check first line for CookieJar signature
            if (firstline) {
                firstline = false;
                if (!strncmp(line, "# CookieJar-1", 13)) {
                    fileType = CookieJar;
                    continue;
                }
            }

            // very early versions of netscape (seen on 1.12) mark the first
            // line of the cookie file with MCOM-HTTP-Cookie-file-1
            //
            if (strlen(line) == 0 || 
                    line[0] == '#' || 
                    line[0] == '\n' ||
                    !strcmp(line, "MCOM-HTTP-Cookie-file-1"))
                continue;

            char *t = line + strlen(line) - 1;
            *t = 0;  // zap newline
            t = line;

            HTTPCookie *cookie = 0;
            switch (fileType) {
            case NetscapeCookieFile:

                // allow white space in name and value items
                cookie = new HTTPCookie;
                cookie->domain = copy(token(&t));
                cookie->exactHostMatch = !stringToBoolean(token(&t));
                cookie->path = copy(token(&t));
                cookie->secure = stringToBoolean(token(&t));
                cookie->expires = atoi(token(&t));
                cookie->cookie.name = copy(token(&t));
                cookie->cookie.value = copy(token(&t));
                break;

            case CookieJar:
                // allow white space in name and value items
                cookie = new HTTPCookie;
                cookie->domain = copy(token(&t));
                cookie->version = atoi(token(&t));
                cookie->exactHostMatch = atoi(token(&t));
                cookie->path = copy(token(&t));
                cookie->port = copy(token(&t));
                cookie->secure = atoi(token(&t));
                cookie->expires = atoi(token(&t));
                cookie->comment = copy(token(&t));
                cookie->commentURL = copy(token(&t));
                cookie->cookie.name = copy(token(&t));
                cookie->cookie.value = copy(token(&t));
                break;
            }

            if (ncookies == 0)
                cookies = new HTTPCookie*[1];
            else {
                HTTPCookie **tmp = new HTTPCookie*[ncookies + 1];
                for (int i = 0; i < ncookies; i++)
                    tmp[i] = cookies[i];
                delete [] cookies;
                cookies = tmp;
            }
            cookies[ncookies] = cookie;
            ncookies++;
        }
        fclose(fp);

        // sort the cookies on domain and reversed path, makes it easier 
        // generating the cookieList.
        // 
        // According to the spec (example 2), more specific paths come before
        // the less specific one.
        //
        if (ncookies > 1)
            std::sort(cookies, cookies + ncookies, sortCookies);
    }
}


namespace {
    char *
    token(char **str)
    {
        char *s = *str;
        char *t = s;
        while (*s && *s != '\t')
            s++;
        if (*s) {
            *s = 0;
            *str = s + 1;
        }
        else
            *str = s;
        return (t);
    }
}


// The "GetCookieFromCache" function.
//
HTTPCookieRequest *
HTTPCookieCache::get(char *url)
{
    sURL uinfo;
    cComm::http_parse_url(url, 0, &uinfo);
    if (!uinfo.hostname || !uinfo.filename)
        return (0);

    HTTPCookieRequest *req = new HTTPCookieRequest;
    HTTPCookieList *cLP = req->cookieList;
    char tmpHost[128];

    for (int i = 0; i < ncookies; i++) {

        memset(tmpHost, 0, 128);
        strcat(tmpHost, uinfo.hostname);
            
        // if the cookie has expired, ignore it (it won't get written out
        // when we save the cache)
        //
        if (cookies[i]->expires < time(0))
            continue;

        char *domain;
        if ((domain = strstr(tmpHost, cookies[i]->domain)) != 0) {

            if (cookies[i]->exactHostMatch) {

                if (!strcasecmp(cookies[i]->domain, tmpHost) &&
                        !strncmp(filename, cookies[i]->path,
                        strlen(filename))) {

                    // ToDo check port numbers

                    if (!cLP)
                        req->cookieList = cLP = new HTTPCookieList;
                    else if (cLP->next == 0) {
                        cLP->next = new HTTPCookieList;
                        cLP = cLP->next;
                    }
                    cLP->cookie = cookies[i];
                }
            }
            else {
                domain[0] = '\0';

                // hostnames with embedded dots are not allowed and the
                // domain name must have  at least one dot 
                //
                if (/*strchr(tmpHost, '.') || */
                        strchr(cookies[i]->domain, '.') == 0)
                    continue;

                if (!cLP)
                    req->cookieList = cLP = new HTTPCookieList;
                else if (cLP->next == 0) {
                    cLP->next = new HTTPCookieList;
                    cLP = cLP->next;
                }
                cLP->cookie = cookies[i];
            }
        }
    }
    return (req);
}


// Add the cookie list to the cache.
//
void
HTTPCookieCache::add(HTTPCookieList *cookieList)
{
    while (cookieList) {
        int install = 1;

        // we look to see if the cookie is already in the cache, if so we
        // replace it _unless_ it has a newer VERSION 
        //     
        for (int i = 0; i < ncookies; i++) {
            
            if (!strcasecmp(cookies[i]->domain, 
                    cookieList->cookie->domain))
                if (!strcmp(cookies[i]->path, 
                        cookieList->cookie->path))
                    if (!strcmp(cookies[i]->cookie.name, 
                            cookieList->cookie->cookie.name) &&
                            cookies[i]->version >=
                            cookieList->cookie->version) {
                        delete cookies[i];
                        cookies[i] = cookieList->cookie;
                        install = 0;
                    }
        }   
        if (install) {
            HTTPCookie **tmp = new HTTPCookie* [ncookies + 1];
            for (int i = 0; i < ncookies; i++)
                tmp[i] = cookies[i];
            delete [] cookies;
            cookies = tmp;
            cookies[ncookies] = cookieList->cookie;
            ncookies++;
        }
        cookieList = cookieList->next;  
    }
    std::sort(cookies, cookies + ncookies, sortCookies);
} 


// Write a file containing the cached cookies.
//
void 
HTTPCookieCache::write()
{
    if (ncookies <= 0)
        return;
    time_t t = time(0);
    FILE *fp = fopen(filename, "wb");
    if (!fp)
        return;
        
    fprintf(fp, "# CookieJar-1\n");
    fprintf(fp, "# This file is autogenerated, don't edit it\n");
    fprintf(fp,
"# format designed by Richard Offer <offer@sgi.com> for HTTP cookies that \n");
    fprintf(fp,
"# comply with both Netscape format and the 21-Nov-97 draft of HTTP State \n");
    fprintf(fp, "# Management Mechanism (was RFC2109)\n\n");
#ifdef COOKIE_DEBUG            
    fprintf(fp, "# format:\n");
    fprintf(fp, "#   domain (String)\n");
    fprintf(fp, "#   <TAB>\n");
    fprintf(fp, "#   version (int) (0==SetCookie, 1==SetCookie2)\n");
    fprintf(fp, "#   <TAB>\n");
    fprintf(fp,
"#   exactHostMatch (int) (0=all machines in domain can access cookie\n");
    fprintf(fp, "#   <TAB>\n");
    fprintf(fp, "#   path (String)\n");
    fprintf(fp, "#   <TAB>\n");
    fprintf(fp, "#   port (String) comma-separated list of ports\n");
    fprintf(fp, "#   <TAB>\n");
    fprintf(fp, "#   secure (int)\n");
    fprintf(fp, "#   <TAB>\n");
    fprintf(fp, "#   expires (int)\n");
    fprintf(fp, "#   <TAB>\n");
    fprintf(fp, "#   comment (String)\n");
    fprintf(fp, "#   <TAB>\n");
    fprintf(fp, "#   commentURL (String)\n");
    fprintf(fp, "#   <TAB>\n");
    fprintf(fp, "#   name (String)\n");
    fprintf(fp, "#   <TAB>\n");
    fprintf(fp, "#   value (String)\n\n");
    fprintf(fp,
    "# Netscape style cookies do not include port comment or commentURL\n");
#endif

    for (int i = 0; i < ncookies; i++) {

        if (!cookies[i]->discard && 
                cookies[i]->expires > t) {

            fprintf(fp, "%s\t", cookies[i]->domain);
            fprintf(fp, "%d\t", cookies[i]->version);
            fprintf(fp, "%d\t", cookies[i]->exactHostMatch);
            fprintf(fp, "%s\t", cookies[i]->path);
            fprintf(fp, "%s\t", cookies[i]->port ? cookies[i]->port : "" );
            fprintf(fp, "%d\t", cookies[i]->secure);
            fprintf(fp, "%d\t", cookies[i]->expires);
            fprintf(fp, "%s\t", cookies[i]->comment ?
                cookies[i]->comment : "" );
            fprintf(fp, "%s\t", cookies[i]->commentURL ?
                cookies[i]->commentURL : "" );
            fprintf(fp, "%s\t", cookies[i]->cookie.name);
            fprintf(fp, "%s\n", cookies[i]->cookie.value);
        }
    }
    fclose(fp);
}


// Merge the passed cookie cache.
//
void
HTTPCookieCache::merge(HTTPCookieCache *c2)
{
    HTTPCookieCache *cct = this;
    if (cct && c2) {
        HTTPCookie **tmp = new HTTPCookie* [ncookies + c2->ncookies];
        memcpy(tmp, cookies, ncookies*sizeof(HTTPCookie*));
        memcpy(tmp + ncookies, c2->cookies,
            c2->ncookies*sizeof(HTTPCookie*));
        delete [] cookies;
        cookies = tmp;
        ncookies += c2->ncookies;   
    }
}   
// End HTTPCookieCache functions


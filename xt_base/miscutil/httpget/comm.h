
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
 * HTPGET http transport utility
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

/************************************************************************
 * comm.h: basic C++ class library for http/ftp communications
 *
 * S. R. Whiteley <stevew@srware.com> 2/13/00
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

#ifndef COMM_H
#define COMM_H

#include <stdio.h>
#include <sys/types.h>

// library revision
extern const char *httpVersionString;

// Default ports, for user convenience
//
#define DEFAULT_FTP_PORT 21
#define DEFAULT_HTTP_PORT 80

// default connect() and select() timeout
//
#define DEFAULT_TIMEOUT 15

// default buffer size
//
#define CHUNKSIZE 16384

// default retry if select() fails
//
#define DEFAULT_RETRY 4

// The return code from the cComm:comm_ftp_request() method
//
enum FTP_RET { FTP_ERR, FTP_OK, FTP_DIR };

// Error code saved in cComm::errcode after a method returns.  If the
// field is not CommOK, there is generally a message string in
// cComm::errmsg.  This string should be freed before the next method
// call.
//
enum ErrCode
{
    CommOK,
    Aborted,
    BadProtocol,
    BadHost,
    NoSocket,
    NoConnection,
    NoAccept,
    NoBind,
    NoListen,
    NoRead,
    NoWrite,
    NoSelect,
    ConnectTimeout,
    Timeout,
    InvalidRequest
};

// Forward declarations
//
struct HTTPRequest;
struct HTTPResponse;
struct HTTPCookieRequest;
struct HTTPNamedValues;
struct HTTPCookieCache;

struct sURL
{
    sURL()
        {
            scheme = 0;
            username = 0;
            password = 0;
            hostname = 0;
            filename = 0;
            port = -1;
        }

    ~sURL()
        {
            clear();
        }

    void clear()
        {
            delete [] scheme;
            delete [] username;
            delete [] password;
            delete [] hostname;
            delete [] filename;
            scheme = 0;
            username = 0;
            password = 0;
            hostname = 0;
            filename = 0;
            port = -1;
        }

    char *scheme;
    char *username;
    char *password;
    char *hostname;
    char *filename;
    int port;
};


// The main class.
//
class cComm
{
public:
    cComm(HTTPRequest *req, HTTPResponse *res)
        {
            errmsg = 0;
            errcode = CommOK;
            request = req;
            response = res;
            logfile = 0;
            logfp = 0;
            gfd = -1;
            gfd_callback = 0;
            debug_printf = 0;
            status_printf = 0;
            http_no_reissue = false;
            timeout = DEFAULT_TIMEOUT;
            retry = DEFAULT_RETRY;
            http_port = DEFAULT_HTTP_PORT;
            ftp_port = DEFAULT_FTP_PORT;
            chunksize = CHUNKSIZE;
            printarg = 0;
            status_active = false;
        }

    ~cComm()
        {
            if (logfp && logfp != stdout && logfp != stderr)
                fclose(logfp);
            delete [] logfile;
            delete [] errmsg;
        }

    void comm_set_debug_print(bool(*f)(void*, const char*, ...))
                                        { debug_printf = f; }
    void comm_set_status_print(bool(*f)(void*, const char*, ...))
                                        { status_printf = f; }
    void comm_set_no_reissue(bool b)    { http_no_reissue = b; }
    void comm_set_timeout(int t)        { timeout = t; }
    void comm_set_retry(int r)          { retry = r; }
    void comm_set_http_port(int p)      { http_port = p; }
    void comm_set_ftp_port(int p)       { ftp_port = p; }
    void comm_set_printarg(void *p)     { printarg = p; }

    // This sets up the X file desc and a callabck for servicing X events
    // while downloading.  If the cb returns true, the read is aborted
    void comm_set_gfd(int fd, bool(*cb)(int))
        {
            gfd = fd;
            gfd_callback = cb;
        }

    void comm_set_inactive()
        {
            if (status_active && status_printf) {
                status_active = false;
                (*status_printf)(printarg, "\n");
            }
        }

    // Static utilities
    static void http_unescape(char*);
    static char *http_query_string(HTTPNamedValues*);
    static char *http_to_base64(const unsigned char*, size_t);
    static void http_parse_url(const char*, const char*, sURL*);

    int comm_open(char*, int);
    int comm_bind(int);
    int comm_accept(int);
    static void comm_close(int);
    char *comm_hostname();

    bool comm_write(int, const char*, int);
    bool comm_read_line(int, char*, int);
    bool comm_read_buf(int, size_t);
    bool comm_read_buf_to_file(int, size_t*, size_t, FILE*, const char* = 0);
    void comm_set_log(const char*);

    FTP_RET comm_ftp_request(sURL*, FILE*, const char* = 0);
    void comm_http_request(HTTPCookieRequest*, time_t);

    char *errmsg;           // error message, needs to be freed!
    ErrCode errcode;        // error code

private:
    void comm_log_print(const char*, const char*, int);
    void comm_log_puts(const char*, int);
    bool comm_status_update(int, int);
    char *comm_read_block(int, char*, int*, int, int, int);

    int comm_ftp_soak(int, const char*, char**);
    char *comm_ftp_dir(int, char*, int, char*);

    void comm_http_read_response_head(int);
    void comm_http_read_response(int);
    bool comm_http_parse_response_line(char*);
    char *comm_http_encode_form_data(HTTPNamedValues*);

    HTTPRequest *request;               // request parameters
    HTTPResponse *response;             // response parameters
    char *logfile;                      // name of a file for logging
    FILE *logfp;                        // file pointer for log file
    int  gfd;                           // ConnectionNumber()
    bool (*gfd_callback)(int);          // graphics event handler
    bool (*debug_printf)(void*, const char*, ...);
                                        // debugging message printer
    bool (*status_printf)(void*, const char*, ...);
                                        // download status printer
    bool http_no_reissue;               // flag prevents http location
                                        //  change causing request reissue
    int timeout;                        // timeout, seconds
    int retry;                          // number of retries
    int http_port;                      // HTTP port number
    int ftp_port;                       // FTP port number
    int chunksize;                      // buffer size;
    void *printarg;                     // user variable for print funcs
    bool status_active;                 // printing status messages
};


//
// The remaining definitions are specific to HTTP.
//

#define HTTP_VERSION_09 900
#define HTTP_VERSION_10 1000
#define HTTP_VERSION_11 1100

#define GET_METHOD          "GET "
#define POST_METHOD         "POST "
#define HEAD_METHOD         "HEAD "
#define META_METHOD         "META "
#define HTTPVERSIONHDR " HTTP/1.0\r\n"
#define NEWLINE "\r\n"
#define CONTENT_LEN "Content-Length: "
#define CONTENT_TYPE "Content-Type: text/plain"
#define POST_CONTENT_TYPE "Content-Type: application/x-www-form-urlencoded"
#define ACCEPT "Accept: */*\r\n"
#define USER_AGENT_PREFIX "User-Agent: httpget/"

// This is the strlen of the Content-length of the form data
// 10 => forms with up to 100000000 bytes of data
#define MAX_FORM_LEN 10

// Disposition of return.  The returned data can ba dumped as a file,
// saved as in-core data, both, or neither.  If neither, the raw socket
// is returned to the user
//
#define HTTPLoadToString    0x1
#define HTTPLoadToFile      0x2
typedef int HTTPLoadType;

enum HTTPLoadMethod { HTTPGET, HTTPPOST, HTTPHEAD };

// possible HTTP return values
enum HTTPRequestReturn {
    // our own error values (should match ErrCode)
    HTTPInvalid                 = 0,
    HTTPAborted                 = 1,
    HTTPBadProtocol             = 2,
    HTTPBadHost                 = 3,
    HTTPNoSocket                = 4,
    HTTPNoConnection            = 5,
    HTTPNoAccept                = 6,
    HTTPNoBind                  = 7,
    HTTPNoListen                = 8,
    HTTPNoRead                  = 9,
    HTTPNoWrite                 = 10,
    HTTPNoSelect                = 11,
    HTTPConnectTimeout          = 12,
    HTTPTimeout                 = 13,
    HTTPInvalidRequest          = 14,
    HTTPBadURL                  = 15,
    HTTPBadLoadType             = 16,
    HTTPMethodUnsupported       = 17,
    HTTPBadHttp10               = 18,
    HTTPCannotCreateFile        = 19,

    // Now 'Real' HTTP return codes
    HTTPContinue                = 100,
    HTTPSwitchProtocols         = 101,

    HTTPSuccess                 = 200,
    HTTPCreated                 = 201,
    HTTPAccepted                = 202,
    HTTPNonAuthoritativeInfo    = 203,
    HTTPNoContent               = 204,
    HTTPResetContent            = 205,
    HTTPPartialContent          = 206,

    HTTPMultipleChoices         = 300,
    HTTPPermMoved               = 301,
    HTTPTempMoved               = 302,
    HTTPSeeOther                = 303,
    HTTPNotModified             = 304,
    HTTPUseProxy                = 305,

    HTTPBadRequest              = 400,
    HTTPUnauthorised            = 401,
    HTTPPaymentReq              = 402,
    HTTPForbidden               = 403,
    HTTPNotFound                = 404,
    HTTPMethodNotAllowed        = 405,
    HTTPNotAcceptable           = 406,
    HTTPProxyAuthReq            = 407,
    HTTPRequestTimeOut          = 408,
    HTTPConflict                = 409,
    HTTPGone                    = 410,
    HTTPLengthReq               = 411,
    HTTPPreCondFailed           = 412,
    HTTPReqEntityTooBig         = 413,
    HTTPURITooBig               = 414,
    HTTPUnsupportedMediaType    = 415,

    HTTPInternalServerError     = 500,
    HTTPNotImplemented          = 501,
    HTTPBadGateway              = 502,
    HTTPServiceUnavailable      = 503,
    HTTPGatewayTimeOut          = 504,
    HTTPHTTPVersionNotSupported = 505
};

struct HTTPNamedValues
{
    HTTPNamedValues()
        {
            name = 0;
            value = 0;
        }

    ~HTTPNamedValues()
        {
            delete [] name;
            delete [] value;
        }

    char *name;
    char *value;
};

struct HTTPRequest
{
    HTTPRequest()
        {
            url = 0;
            proxy = 0;
            method = HTTPGET;
            form_data = 0;
            status = HTTPInvalid;
        }

    ~HTTPRequest()
        {
            delete [] url;
            delete [] proxy;
            delete [] form_data;
        }

    char *url;                  // fully qualified location
    char *proxy;                // use the proxy if given
    HTTPLoadMethod method;      // get, post etc
    HTTPNamedValues *form_data; // data for form processing
    HTTPRequestReturn status;   // request error code
};

// HTTP server response definition
struct HTTPResponse
{
    HTTPResponse()
        {
            type = HTTPLoadToString;
            destination = 0;
            http_version = 0;
            headers = 0;
            num_headers = 0;
            data = 0;
            content_length = 0;
            bytes_read = 0;
            socknum = -1;
            status = HTTPSuccess;
            read_complete = false;

        }

    ~HTTPResponse()
        {
            delete [] destination;
            delete [] data;
            delete [] headers;
        }

    const char *errorString() const;

    HTTPLoadType type;              // load to string, file, or none
    char *destination;              // filename for LoadToFile
    int http_version;               // server version
    HTTPNamedValues *headers;       // array of returned headers
    int num_headers;                // number of headers
    char *data;                     // message or data dointer
    size_t content_length;          // header-supplied content len
    size_t bytes_read;              // bytes read into data buffer
    int socknum;                    // socket used for reading, for LoadNone
    HTTPRequestReturn status;       // response code (server return or error)
    bool read_complete;             // no more data to read
};


//
// Cookie support
//

// type of cookies 
//   SetCookie implies the server is using Netscape style cookies.
//   SetCookie2 means the server is using RFC2109 style cookies.
//
enum { SetCookie, SetCookie2 }; 

// type of cookie file 
//   For safety I am only going to support the writting of CookieJar files
//   however, I will allow an application to read cookie files generated by 
//   netscape.
//   
//   CookieJar (a name I've just invented) files are much richer than Netscape 
//   ones, they have to be they store the full SetCookie2 response. 
//
enum { NetscapeCookieFile = 1, CookieJar = 2 } ;  

struct HTTPCookie
{
    HTTPCookie()
        {
            cookie.name = 0;
            cookie.value = 0;
            type = 0;
            comment = 0;
            commentURL = 0;
            discard = 0;
            domain = 0;
            exactHostMatch = 0;
            secure = 0;
            path = 0;
            expires = 0;
            port = 0;
            version = 0;
        }

    ~HTTPCookie()
        {
            delete [] comment;
            delete [] commentURL;
            delete [] domain;
            delete [] path;
            delete [] port;
        }

    HTTPNamedValues cookie;
    char    type;
    char    *comment;   // this and the url are not preserved between sessions
    char    *commentURL;
    int     discard;
    char    *domain;
    int     exactHostMatch;
    int     secure;     // not supported
    char    *path;
    int     expires;
    char    *port;
    int     version;
}; 

struct HTTPCookieList
{
    HTTPCookieList()
        {
            cookie = 0;
            next = 0;

        }

    static void destroy(HTTPCookieList *l)
        {
            while (l) {
                HTTPCookieList *lx = l;
                l = l->next;
                delete lx;
            }
        }

    char *makeCookie();

    HTTPCookie  *cookie;
    HTTPCookieList *next;
};

struct HTTPCookieRequest
{
    HTTPCookieRequest()
        {
            cookieList = 0;
            setCookie = 0;
            sendCookie = 1;
        }

    ~HTTPCookieRequest()
        {
            HTTPCookieList::destroy(cookieList);
            HTTPCookieList::destroy(setCookie);
        }

    void set(int, char*, char*);

    HTTPCookieList *cookieList;
    HTTPCookieList *setCookie;
    int sendCookie;
};

struct HTTPCookieCache
{
    HTTPCookieCache()
        {
            cookies = 0;
            ncookies = 0;
            filename = 0;
        }

    HTTPCookieCache(const char*);

    ~HTTPCookieCache()
        {
            freeCookies();
            delete [] filename;
        }

    void freeCookies()
        {
            if (cookies) {
                for (int i = 0; i < ncookies; i++)
                    delete cookies[i];
                delete [] cookies;
                cookies = 0;
            }
        }

    HTTPCookieRequest *get(char*);
    void add(HTTPCookieList*);
    void write();
    void merge(HTTPCookieCache*);

    HTTPCookie  **cookies;
    int         ncookies;
    char        *filename;  
};

#endif



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
 * httpget --  HTTP file transport utility.                               *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

/************************************************************************
 * transact.h: basic C++ class library for http/ftp communications
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

#ifndef TRANSACT_H
#define TRANSACT_H

#include "comm.h"
#include "lstring.h"

struct http_monitor;
class Transaction;

// Default message output function.  The first argument is the Transaction
// class pointer.  If function returns true, transfer is aborted.
//
inline bool def_tputs(void*, const char *string)
{
    fputs(string, stderr);
    return (false);
}

typedef bool(*http_tputs_func)(void*, const char*);
typedef void(*http_err_func)(Transaction*, const char*);

enum { PLAIN, HTML };

enum http_ret_mode
{
    HTTP_Skt,           // return raw socket
    HTTP_Buf,           // return data buf
    HTTP_FileOrBuf,     // write file t_destination if given, else return
                        //  data buf
    HTTP_FileAndBuf     // write file and return data buf
};

// The transaction class performs an http/ftp transaction with a remote
// server.  The basic procedure is as follows:
//    Create a Transaction class instance
//    Fill in the public input fields needed
//    Call do_transaction()
//    Process any return fields needed
//    Destroy the instance
// The operation will generally create a file or return a buffer containing
// data.  It is also possible to obtain the raw socket so that the user
// can perform the actual read operation.
//
// The request/response structs contained in the Transaction class contain
// information about the transfer.

class Transaction
{
public:
    friend struct http_monitor;

    Transaction();
    ~Transaction();

    int parse_cmd(int, char**, bool*);  // parse options, argv array
    int parse_cmd(const char*, bool*);  // parse options, string
    int transact();                     // perform transaction
    void emit_error(const char*, int);  // error handler

    static bool http_printf(void*, const char*, ...);

    const char *url()               { return (t_url); }
    void set_url(const char *t)
        {
            char *s = lstring::copy(t);
            delete [] t_url;
            t_url = s;
        }

    const char *proxy()             { return (t_proxy); }
    void set_proxy(const char *t)
        {
            char *s = lstring::copy(t);
            delete [] t_proxy;
            t_proxy = s;
        }

    const char *destination()       { return (t_destination); }
    void set_destination(const char *t)
        {
            char *s = lstring::copy(t);
            delete [] t_destination;
            t_destination = s;
        }

    const char *post_query_file()   { return (t_post_query_file); }
    void set_post_query_file(const char *t)
        {
            char *s = lstring::copy(t);
            delete [] t_post_query_file;
            t_post_query_file = s;
        }

    const char *logfile()           { return (t_logfile); }
    void set_logfile(const char *t)
        {
            char *s = lstring::copy(t);
            delete [] t_logfile;
            t_logfile = s;
        }

    const char *cookiefile()        { return (t_cookiefile); }
    void set_cookiefile(const char *t)
        {
            char *s = lstring::copy(t);
            delete [] t_cookiefile;
            t_cookiefile = s;
        }

    int output_err_format()         { return (t_output_error_format); }
    void set_output_err_format(int f) { t_output_error_format = f; }
    int timeout()                   { return (t_timeout); }
    void set_timeout(int t)         { t_timeout = t; }
    int retries()                   { return (t_retry); }
    void set_retries(int t)         { t_retry = t; }
    int http_port()                 { return (t_http_port); }
    void set_http_port(int t)       { t_http_port = t; }
    int ftp_port()                  { return (t_ftp_port); }
    void set_ftp_port(int t)        { t_ftp_port = t; }

    http_err_func set_err_func(http_err_func f)
        {
            http_err_func o = t_err_call;
            t_err_call = f;
            return (o);
        }

    int xpos()                      { return (t_xpos); }
    void set_xpos(int x)            { t_xpos = x; }
    int ypos()                      { return (t_ypos); }
    void set_ypos(int y)            { t_ypos = y; }
    bool save_err_text()            { return (t_save_error_text); }
    void set_save_err_text(bool b)  { t_save_error_text = b; }
    bool head_only()                { return (t_head_only); }
    void set_head_only(bool b)      { t_head_only = b; }
    bool http_debug()               { return (t_http_debug); }
    void set_http_debug(bool b)     { t_http_debug = b; }
    bool http_no_reissue()          { return (t_http_no_reissue); }
    void set_http_no_reissue(bool b) { t_http_no_reissue = b; }
    bool http_silent()              { return (t_http_silent); }
    void set_http_silent(bool b)    { t_http_silent = b; }
    bool http_use_graphics()        { return (t_use_graphics); }
    void set_use_graphics(bool b)   { t_use_graphics = b; }

    http_ret_mode return_mode()     { return ((http_ret_mode)t_return_mode); }
    void set_return_mode(http_ret_mode m) { t_return_mode = m; }
    time_t cachetime()              { return (t_cachetime); }
    void set_cachetime(time_t t)    { t_cachetime = t; }

    http_tputs_func set_puts(http_tputs_func f)
        {
            http_tputs_func o = t_puts;
            t_puts = f;
            return (o);
        }


    void *user_data1()              { return (t_user_data1); }
    void set_user_data1(void *v)    { t_user_data1 = v; }
    void *user_data2()              { return (t_user_data2); }
    void set_user_data2(void *v)    { t_user_data2 = v; }

    const HTTPRequest *request()    { return (&t_request); }
    const HTTPResponse *response()  { return (&t_response); }

    // internal use for graphics functions
    http_monitor *gr()              { return (t_gcontext); }
    void set_gr(http_monitor *gc)   { t_gcontext = gc; }
    void set_abort()                { t_err_return = -2; }
    bool is_aborted()               { return (t_err_return == -2); }
    void set_err_return(int er)     { t_err_return = er; }

private:
    void do_transaction();          // main action
    void show_download();           // graphical monitor
    char *get_protocol();           // return protocol

    // The following are set by parse_cmd(), or can be set/overridden
    // explicitly
    char *t_url;                // url for transaction
    char *t_proxy;              // proxy url
    char *t_destination;        // destination file name
    char *t_post_query_file;    // query file for POST
    char *t_logfile;            // file for transaction log
    char *t_cookiefile;         // cookie file name
    int t_output_error_format;  // PLAIN or HTML, default PLAIN
    int t_timeout;              // timeout seconds for connect (has default)
    int t_retry;                // retries after timeout (has default)
    int t_http_port;            // port for http connections (has default)
    int t_ftp_port;             // port for ftp connections (has default)

    http_err_func t_err_call;
                                // If this is supplied by the caller, error
                                // messages from emit_error() will be passed
                                // to this function, if they would otherwise
                                // go to stdout

    int t_xpos, t_ypos;         // graphics, window position
    bool t_save_error_text;     // save HTTP error output on failure
    bool t_head_only;           // only get document info (HEAD)
    bool t_http_debug;          // print debugging info
    bool t_http_no_reissue;     // don't reissue on location change error
    bool t_http_silent;         // don't show download status indicator
    bool t_use_graphics;        // pop up monitor window

    // The remaining values are not set by parse_cmd()

    char t_return_mode;         // what to do with returned data
    time_t t_cachetime;         // time value for GET If-Modified-Since

    // This function can be specified to redirect messages
    http_tputs_func t_puts;     // print function for messages
    void *t_user_data1;         // hook for user data, not otherwise used
    void *t_user_data2;         // hook for user data, not otherwise used

    // These contain data transfer information during transfer and after.
    // They are filled in by the comm functions
    HTTPRequest t_request;      // request struct for comm
    HTTPResponse t_response;    // response struct for comm

    HTTPCookieCache *t_cc;      // cookie cache
    http_monitor *t_gcontext;   // graphical interface internal  
    int t_err_return;           // error status of transaction, this is
                                //  returned by transact()
};

// Interface to the graphics, if used.
//
struct http_monitor
{
    virtual ~http_monitor()                 { }
    virtual bool widget_print(const char*)  { return (false); }
    virtual void abort()                    { }
    virtual void run(Transaction *t)        { t->do_transaction(); }

    virtual bool graphics_enabled()         { return (false); }
    virtual void initialize(int&, char**)   { }
    virtual void setup_comm(cComm*)         { }
    virtual void start(Transaction*)        { }
};

namespace httpget {
    extern http_monitor *Monitor;
}

#endif


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
 * transact.cc: basic C++ class library for http/ftp communications
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

//
// The Transact package is a higher-level interface to the comm
// package.
//
#include "config.h"
#include "transact.h"
#include "lstring.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <stdarg.h>


using namespace httpget;

// The Monitor pointer is reset in the graphics library to enable
// graphics.

http_monitor *httpget::Monitor = 0;

namespace {
    http_monitor _monitor_;

    struct http_init_t
    {
        http_init_t()
            {
                if (!Monitor)
                    Monitor = &_monitor_;
            }
    };

    http_init_t _http_init_;


    // This prints the debugging messages if enabled.  These always go to
    // stderr.
    //
    bool
    http_debug_printf(void*, const char *fmt, ...)
    {
        va_list args;
        char buf[1024];

        if (!fmt)
            return (false);
        va_start(args, fmt);
#ifdef HAVE_VSNPRINTF
        vsnprintf(buf, 1024, fmt, args);
#else
        vsprintf(buf, fmt, args);
#endif
        va_end(args);  
        fputs(buf, stderr);
        return (false);
    }
}


Transaction::Transaction()
{
    t_url = 0;
    t_proxy = 0;
    t_destination = 0;
    t_post_query_file = 0;
    t_logfile = 0;
    t_cookiefile = 0;
    t_output_error_format = PLAIN;
    t_timeout = DEFAULT_TIMEOUT;
    t_retry = DEFAULT_RETRY;
    t_http_port = DEFAULT_HTTP_PORT;
    t_ftp_port = DEFAULT_FTP_PORT;
    t_err_call = 0;
    t_xpos = -1;
    t_ypos = -1;
    t_save_error_text = false;
    t_head_only = false;
    t_http_debug = false;
    t_http_no_reissue = false;
    t_http_silent = false;
    t_use_graphics = false;
    t_return_mode = 2;

    t_cachetime = 0;
    t_puts = def_tputs;
    t_user_data1 = 0;
    t_user_data2 = 0;
    t_cc = 0;
    t_gcontext = 0;
    t_err_return = 0;
}


Transaction::~Transaction()
{
    delete [] t_url;
    delete [] t_proxy;
    delete [] t_destination;
    delete [] t_post_query_file;
    delete [] t_logfile;
    delete [] t_cookiefile;
    if (t_cc) {
        t_cc->write();
        delete t_cc;
    }
    delete t_gcontext;
    if (t_response.socknum >= 0)
        cComm::comm_close(t_response.socknum);
}


// Set up the flags and options according to the argv array, interpreted
// as for the "httpget" program.  As in a normal command line array, the
// first arg is ignored.  Zero is returned on success.  The third argument
// is set by the "x" option (use http error for exit return) which is
// not used directly in the Transaction package.  This function is
// public.
// 
int
Transaction::parse_cmd(int argc, char **argv, bool *xoption)
{
    if (argc < 2)
        return (EXIT_FAILURE);
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            for (unsigned j = 1; j < strlen(argv[i]); j++) {
                switch (argv[i][j]) {
                case 'c':
                    if (i+1 != argc && argv[i+1][0] != '-') {
                        t_cookiefile = lstring::copy(argv[++i]);
                        j = strlen(argv[i]);
                    }
                    else {
                        emit_error("Missing arg for -c", 0);
                        return (EXIT_FAILURE);
                    }
                    break;
                case 'd':
                    t_http_debug = true;
                    break;
                case 'e':
                    t_http_no_reissue = true;
                    break;
                case 'f':
                    if (j+1 == strlen(argv[i]))
                        emit_error("Missing flag to -f", 0);
                    if (argv[i][j+1] == 'p')
                        t_output_error_format = PLAIN;
                    else if (argv[i][j+1] == 'h')
                        t_output_error_format = HTML;
                    else {
                        emit_error("Unknown flag to -f", 0);
                        return (EXIT_FAILURE);
                    }
                    j++;
                    break;
                case 'g':
                    if (Monitor->graphics_enabled()) {
                        t_use_graphics = true;
                        if (isdigit(argv[i][j+1])) {
                            if (sscanf(&argv[i][j+1], "%d:%d",
                                    &t_xpos, &t_ypos) != 2) {
                                t_xpos = -1;
                                t_ypos = -1;
                                emit_error("Bad coords after -g", 0);
                                return (EXIT_FAILURE);
                            }
                            while (isdigit(argv[i][j+1]) ||
                                    argv[i][j+1] == ':')
                                j++;
                        }
                    }
                    else
                        fprintf(stderr,
                        "Warning: no graphical support in this version.\n");
                    break;
                case 'h':
                    return (EXIT_FAILURE);
                case 'i':
                    t_head_only = true;
                    break;
                case 'l':
                    if (i+1 != argc && argv[i+1][0] != '-') {
                        t_logfile = lstring::copy(argv[++i]);
                        j = strlen(argv[i]);
                    }
                    else {
                        emit_error("Missing arg for -l", 0);
                        return (EXIT_FAILURE);
                    }
                    break;
                case 'n':
                    t_http_silent = true;
                    break;
                case 'o':
                    if (i+1 != argc && argv[i+1][0] != '-') {
                        t_destination = lstring::copy(argv[++i]);
                        j = strlen(argv[i]);
                    }
                    else {
                        emit_error("Missing arg for -o", 0);
                        return (EXIT_FAILURE);
                    }
                    break;
                case 'p':
                    if (i+1 != argc && argv[i+1][0] != '-') {
                        t_proxy = lstring::copy(argv[++i]);
                        j = strlen(argv[i]);
                    }
                    else {
                        emit_error("Missing arg for -p", 0);
                        return (EXIT_FAILURE);
                    }
                    break;
                case 'q':
                    if (i+1 != argc && argv[i+1][0] != '-') {
                        t_post_query_file = lstring::copy(argv[++i]);
                        j = strlen(argv[i]);
                    }
                    else {
                        emit_error("Missing flag to -q", 0);
                        return (EXIT_FAILURE);
                    }
                    break;
                case 'r':
                    if (i+1 != argc && argv[i+1][0] != '-') {
                        t_retry = atoi(argv[++i]);
                        j = strlen(argv[i]);
                    }
                    else {
                        emit_error("Missing arg for -r", 0);
                        return (EXIT_FAILURE);
                    }
                    break;
                case 's':
                    t_save_error_text = true;
                    break;
                case 't':
                    if (i+1 != argc && argv[i+1][0] != '-') {
                        t_timeout = atoi(argv[++i]);
                        j = strlen(argv[i]);
                    }
                    else {
                        emit_error("Missing arg for -t", 0);
                        return (EXIT_FAILURE);
                    }
                    break;
                case 'x':
                    *xoption = true;
                    break;
                default:
                    fprintf(stderr, "Unknown option: %c\n", argv[i][j]);
                    return (EXIT_FAILURE);
                    break;
                }
            }
        }
        else if (!t_url)
            t_url = lstring::copy(argv[i]);
    }
    return (0);
}


// Set up the Transaction struct according to the "httpget" command line
// in string.  The first token is taken as a command name and is ignored.
// Zero is returned on success.  This function is public.
//
int
Transaction::parse_cmd(const char *string, bool *xoption)
{
    char buf[2048];
    strcpy(buf, string);
    char *argv[64];
    memset(argv, 0, 64*sizeof(char*));
    int argc = 0;
    char *s = buf;
    int j = 0;
    for (;;) {
        while (isspace(*s)) s++;
        if (!*s) {
            argc = j;
            break;
        }
        char *t = s;
        while (*t && !isspace(*t)) t++;
        if (*t)
            *t++ = '\0';     
        argv[j++] = s;
        s = t;
    }
    return (parse_cmd(argc, argv, xoption));
}


// Main public function to initiate an HTTP or FTP transaction.  The
// return status is left in t_err_return and returned by this function:
//
// HTTP:
//   -1          failed (no url, etc)
//   0           success, http status 200 - 399, except for HTTPNotModified
//   pos value   HTTPRequestReturn value
//
// FTP:
//   -1          failed
//   0           success
//
int
Transaction::transact()
{
    t_err_return = 0;
    if (!t_url) {
        // This is the only thing that really must be set
        emit_error("Missing url: don't know what to fetch!", 0);
        return (-1);
    }
    if (t_use_graphics)
        show_download();
    else
        do_transaction();
    return (t_err_return);
}


// Public error handler.  The error message may or may not be HTML
// formatted, and may or may not be saved to the output file, depending
// on the flags.  The t_err_return field is set here.
//
void
Transaction::emit_error(const char *msg, int error)
{
    const char *htm1 =
        "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 3.2//EN\">\n"
        "<HTML><HEAD><TITLE>\n";
    const char *htm2 =
        "</TITLE></HEAD>\n"
        "<body text=\"#0\" bgcolor=\"#cccccc\">\n"
        "<div align=right><font face=\"arial,helvetica\" size=\"+1\">"
        "Unable to complete HTTP request&nbsp;&nbsp\n</font>\n"
        "</div><hr noshade size=\"2\"><p>\n";
    const char *htm3 =
        "<p>\n<hr noshade size=\"2\">\n<div align=\"right\">"
        "<font size=\"-1\"><i>\n"
        "Last Updated: "
        __DATE__
        "<br>\nQuestion? Mail <a href=\"mailto:stevew@wrcad.com\">"
        "httpget</a>\n</i></font></div>\n"
        "</BODY></HTML>\n";

    bool isgpr = false;
    if (t_use_graphics && t_gcontext)
        isgpr = t_gcontext->widget_print(msg);
    FILE *outfp;
    if (t_save_error_text) {
        isgpr = false;
        if (!t_destination)
            outfp = stdout;
        else if ((outfp = fopen(t_destination, "wb")) == 0)
            outfp = stdout;
    }
    else
        outfp = stdout;
    if (!isgpr) {
        if (t_output_error_format == HTML) {
            if (!t_save_error_text && t_err_call) {
                char *s = new char[strlen(htm1) + strlen(htm2) + strlen(htm3)
                    + strlen(msg) + 64];
                strcpy(s, htm1);
                char *t = s + strlen(s);
                if (error)
                    sprintf(t, "HTTP/1.X Error: %d\n", error);
                else
                    strcpy(t, "Error\n");
                t += strlen(t);
                strcpy(t, htm2);
                t += strlen(t);
                strcpy(t, msg);
                t += strlen(t);
                *t++ = '\n';
                strcpy(t, htm3);
                (*t_err_call)(this, s);
                delete [] s;
            }
            else {
                fputs(htm1, outfp);
                if (error)
                    fprintf(outfp, "HTTP/1.X Error: %d\n", error);
                else
                    fprintf(outfp, "Error\n");
                fputs(htm2, outfp);
                fputs(msg, outfp);
                fputs("\n", outfp);
                fputs(htm3, outfp);
            }
        }
        else {
            if (!t_save_error_text && t_err_call)
                (*t_err_call)(this, msg);
            else {
                fputs(msg, outfp);
                fputs("\n", outfp);
            }
        }
        fflush(outfp);
    }
    if (outfp != stdout)
        fclose(outfp);
    if (error)
        t_err_return = error;
    else
        t_err_return = -1;
}


// This handles the emitted status messages.  Print them in the widget
// if it exists, otherwise pass them to the function pointed to in the
// Transaction class t_puts field.
//
bool
Transaction::http_printf(void *arg, const char *fmt, ...)
{
    va_list args;
    char buf[1024];

    *buf = 0;
    if (fmt) {
        va_start(args, fmt);
#ifdef HAVE_VSNPRINTF
        vsnprintf(buf, 1024, fmt, args);
#else
        vsprintf(buf, fmt, args);
#endif
        va_end(args);  
    }
    Transaction *t = (Transaction*)arg;
    http_monitor *gb = t->gr();
    if (t->is_aborted()) {
        if (gb)
            gb->abort();
        return (true);
    }
    if (!gb || !gb->widget_print(buf)) {
        if (t->t_puts && (*t->t_puts)(t, buf)) {
            // abort transfer
            t->set_abort();
            if (gb)
                gb->abort();
            return (true);
        }
    }
    return (false);
}


// Private function to actually perform the transfer.
//
void
Transaction::do_transaction()
{
    cComm comm(&t_request, &t_response);
    comm.comm_set_printarg(this);
    if (t_http_debug)
        comm.comm_set_debug_print(&http_debug_printf);
    if (!t_http_silent || t_use_graphics)
        comm.comm_set_status_print(&http_printf);
    if (t_use_graphics)
        Monitor->setup_comm(&comm);
    if (t_http_no_reissue)
        comm.comm_set_no_reissue(true);
    comm.comm_set_timeout(t_timeout);
    comm.comm_set_retry(t_retry);
    if (t_http_port != DEFAULT_HTTP_PORT)
        comm.comm_set_http_port(t_http_port);
    if (t_ftp_port != DEFAULT_FTP_PORT)
        comm.comm_set_ftp_port(t_ftp_port);
    if (t_logfile)
        comm.comm_set_log(t_logfile);

    // check protocol, if none will use http
    bool do_ftp = false;
    char *proto = get_protocol();
    if (proto) {
        if (!strcmp(proto, "http")) {
            t_request.url = lstring::copy(t_url);
            t_request.proxy = lstring::copy(t_proxy);
        }
        else if (!strcmp(proto, "ftp")) {
            if (t_proxy) {
                emit_error("Unsupported proxy protocol: ftp.", 0);
                return;
            }
            do_ftp = true;
        }
        else {
            char buf[128];
            sprintf(buf, "Unsupported protocol %s requested.", proto);
            delete [] proto;
            emit_error(buf, 0);
            return;
        }
        delete [] proto;
    }
    else {
        if (t_proxy) {
            t_request.proxy = new char [8 + strlen(t_proxy)];
            strcpy(t_request.proxy, "http://");
            strcat(t_request.proxy, t_proxy);
            t_request.url = lstring::copy(t_url);
        }
        else {
            t_request.url = new char [8 + strlen(t_url)];
            strcpy(t_request.url, "http://");
            strcat(t_request.url, t_url);
        }
    }

    if (do_ftp) {
        sURL uinfo;
        cComm::http_parse_url(t_url, 0, &uinfo);
        if (!uinfo.filename || !*uinfo.filename) {
            // won't happen, filename = "/"
            emit_error("No file given to retrieve.", 0);
            return;
        }
        if (uinfo.filename[0] == '/' && uinfo.filename[1]) {
            for (char *t = uinfo.filename; *t; t++)
                *t = *(t+1);
        }
        char *t = uinfo.filename + strlen(uinfo.filename) - 1;
        if (t > uinfo.filename && *t == '/')
            *t = 0;
        if (!t_destination) {
            t = strrchr(uinfo.filename, '/');
            if (!t)
                t_destination = lstring::copy(uinfo.filename);
            else if (*(t+1))
                t_destination = lstring::copy(t+1);
            else
                t_destination = lstring::copy("httpget.return");
        }
        int rval = comm.comm_ftp_request(&uinfo, 0, t_destination);
        if (rval == FTP_ERR)
            emit_error(comm.errmsg, 0);
        else if (rval == FTP_DIR) {
            FILE *fp = fopen(t_destination, "wb");
            if (!fp)
                emit_error("Can't open file for writing.", 0);
            else {
                fputs(comm.errmsg, fp);
                fclose(fp);
            }
            delete [] comm.errmsg;
            comm.errmsg = 0;
        }
        return;
    }

    if (t_cookiefile && !t_cc) {
        if (t_http_debug)
            http_printf(this, "using '%s' as the cookie file\n", t_cookiefile);
        t_cc = new HTTPCookieCache(t_cookiefile);
    }

    if (t_post_query_file) {
        // The query file represents the output from a form, which is used
        // here to construct the "form_data" for a query using the POST
        // method.  The query file consists of name=value pairs, in the
        // forms
        //   name=single_token
        //   name="whatever"
        // There can be no white space around '='.  White space should
        // separate the pairs.
        //
        t_request.method = HTTPPOST;
        FILE *fp = fopen(t_post_query_file, "rb");
        if (!fp) {
            emit_error("Can't open query file!", 0);
            return;
        }
        
        fseek(fp, 0, SEEK_END);
        int len = ftell(fp);
        char *bf = new char [len + 1];
        rewind(fp);
        fread(bf, len, 1, fp);
        bf[len] = 0;
        fclose(fp);

        char *t = bf;
        int size = 0;
        while (*t) {
            char *name = t;
            char *value = 0;
            while (*t && *t != '=')
                t++;
            if (*t) {
                *t++ = 0;
                bool endq = false;
                if (*t == '"') {
                    t++;
                    endq = true;
                }
                value = t;
                if (endq) {
                    while (*t && !(*t == '"' && *(t-1) != '\\'))
                        t++;
                }
                else {
                    while (*t && !isspace(*t))
                        t++;
                }
            }
            if (*t)
                *t++ = 0;

            if (!t_request.form_data) {
                t_request.form_data = new HTTPNamedValues[2];
                size = 2;
                t_request.form_data[0].name = lstring::copy(name);
                t_request.form_data[0].value = lstring::copy(value);
                t_request.form_data[1].name = 0;
                t_request.form_data[1].value = 0;
            }
            else {
                HTTPNamedValues *tmp = new HTTPNamedValues[size+1];
                for (int i = 0; i < size-1; i++) {
                    tmp[i] = t_request.form_data[i];
                    t_request.form_data[i].name = 0;
                    t_request.form_data[i].value = 0;
                }
                tmp[size-1].name = lstring::copy(name);
                tmp[size-1].value = lstring::copy(value);
                tmp[size].name = 0;
                tmp[size].value = 0;
                size++;
                delete [] t_request.form_data;
                t_request.form_data = tmp;
            }
            while (isspace(*t))
                t++;
        }
        delete [] bf;
    }
    else
        t_request.method = (t_head_only ? HTTPHEAD : HTTPGET);

    if (t_return_mode == 0)
        t_response.type = 0;
    else if (t_return_mode == 1)
        t_response.type = HTTPLoadToString;
    else if (t_return_mode == 2) {
        if (t_destination) {
            t_response.type = HTTPLoadToFile;
            t_response.destination = lstring::copy(t_destination);
        }
        else
            t_response.type = HTTPLoadToString;
    }
    else if (t_return_mode == 3) {
        t_response.type = HTTPLoadToString;
        if (t_destination) {
            t_response.type |= HTTPLoadToFile;
            t_response.destination = lstring::copy(t_destination);
        }
    }
    
    HTTPCookieRequest *cookieReq = 0;
    if (t_cc)
        cookieReq = t_cc->get(t_request.url);
    comm.comm_http_request(cookieReq, t_cachetime);
    if (cookieReq) {
        t_cc->add(cookieReq->setCookie);
        delete cookieReq;
    }
    
    if (t_response.status < 200 || t_response.status >= 400) {
        if (t_return_mode == 0 && t_response.socknum >= 0) {
            comm.comm_close(t_response.socknum);
            t_response.socknum = -1;
        }
        if (t_response.status == HTTPUnauthorised)
            // If unauthorized, return normally, application should
            // re-submit with user/password
            t_err_return = HTTPUnauthorised;
        else {
            int error = (int)t_response.status;
            char msg[1024];
            if (t_use_graphics && t_gcontext)
                sprintf(msg, "%s", t_response.errorString());
            else
                sprintf(msg, "%s: %s", t_request.url,
                    t_response.errorString());
            emit_error(msg, error);
        }
        return;
    }
    if (t_response.status != HTTPSuccess) {
        if (t_return_mode == 0 && t_response.socknum >= 0) {
            comm.comm_close(t_response.socknum);
            t_response.socknum = -1;
        }
        if (t_cachetime && t_response.status == HTTPNotModified) {
            t_err_return = HTTPNotModified;
            return;
        }
        if (t_use_graphics && t_gcontext)
            http_printf(this, "%s\n", t_response.errorString());
        else
            http_printf(this, "%s: %s\n", t_request.url,
                t_response.errorString());
    }
}


// Main function to produce the pop-up monitor.
//
void
Transaction::show_download()
{
    Monitor->start(this);
}


// Return the protocol to be used for transmission, if specified in the
// proxy or main urls.
//
char *
Transaction::get_protocol()
{
    const char *s = t_proxy;
    if (!s)
        s = t_url;
    if (!s)
        return (0);
    while (isspace(*s))
        s++;
    if (!isalpha(*s))
        return (0);
    const char *s0 = s;
    for (s++; *s; s++) {
        if (isalpha(*s))
            continue;
        if (*s != ':')
            return (0);
        int len = s - s0;
        char *p = new char[len + 1];
        for (int i = 0; i < len; i++)
            p[i] = isupper(s0[i]) ? tolower(s0[i]) : s0[i];
        p[len] = 0;
        return (p);
    }
    return (0);
}


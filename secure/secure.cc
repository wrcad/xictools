
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
 * License server and authentication, and related utilities               *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

//
// Interface to license server.
//

#include "config.h"
#include "secure.h"
#include "secure_prv.h"
#include "encode.h"
#include "graphics.h"
#include "pathlist.h"
#include "miscutil.h"
#include "tvals.h"
#include "services.h"

#include <ctype.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#ifdef HAVE_PWD_H
#include <pwd.h>
#endif
#ifdef WIN32
#include "msw.h"
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#endif


// If this is defined, then if XTLTEST is defined as "host
// a0.a1.a2.a3", then those values are sent to the server.
//
//#define MASQUERADE

// Timeout for read/write.
#define IO_TIME_MS 5000

// In application.
extern int BoxFilled;

// Export to application.
int StateInitialized;


#ifdef WIN32
#define CLOSESOCKET(x) shutdown(x, SD_SEND), closesocket(x)
#else
#define CLOSESOCKET(x) close(x)
#endif


//---------------------------------------------------------------------
// The following ensures that error text always goes to stderr, rather
// than to a pop-up.  This can all be commented to use the pop-up,

#define err_printf secure_err_printf
#define out_perror secure_out_perror

namespace {
    void
    secure_err_printf(Etype type, const char *fmt, ...)
    {
        va_list args;
        switch (type) {
        default:
        case ET_MSGS:
        case ET_MSG:
            break;
        case ET_WARNS:
        case ET_WARN:
            fprintf(stderr, "Warning: ");
            break;
        case ET_ERRORS:
        case ET_ERROR:
            fprintf(stderr, "Error: ");
            break;
        case ET_INTERRS:
        case ET_INTERR:
            fprintf(stderr, "Internal error: ");
            break;
        }
        va_start(args, fmt);
        vfprintf(stderr, fmt, args);
        va_end(args);
    }


    void
    secure_out_perror(const char *str)
    {
        if (str && *str) {
#ifdef WIN32
            if (!strcmp(str, "send") || !strcmp(str, "recv") ||
                    !strcmp(str, "socket") || !strcmp(str, "connect") ||
                    !strcmp(str, "accept") || !strcmp(str, "bind") ||
                    !strcmp(str, "listen") || !strcmp(str, "select")) {

                err_printf(ET_MSG, "%s: WSA error code %d.\n", str,
                    WSAGetLastError());
                return;
            }
#endif
#ifdef HAVE_STRERROR
            err_printf(ET_MSG, "%s: %s\n", str, strerror(errno));
        }
        else
            err_printf(ET_MSG, "%s\n", strerror(errno));
#else
            err_printf(ET_MSG, "%s: %s\n", str, sys_errlist[errno]);
        }
        else
            err_printf(ET_MSG, "%s\n", sys_errlist[errno]);
#endif
    }
}

//
//---------------------------------------------------------------------


/******
 Encode the error messages: strs[i][j] ? (strs[i][j] + 13)^1 : 0);
 char *strs[] = {
     "Authorization check failed:",
     "Can't open license file - see your system administrator.\n",
     "Error reading license file.\n",
     "Checksum error in license file.\n",
     "%s\n  Host %s is not licensed to run this binary.\n",
     "%s\n  License for host %s has expired.\n",
     "%s\n  Licensed user quota exceeded.\n",
     "license verification fault.\n\
 Sorry, but this process will be KILLED IN %d MINUTES.\n\
 Please exit and restart (no further warnings).\n",
 };
******/

namespace {
    const unsigned char strs0[] =
    {0x4f, 0x83, 0x80, 0x74, 0x7d, 0x7e, 0x77, 0x86, 0x6f, 0x80, 0x77, 0x7d,
     0x7a, 0x2c, 0x71, 0x74, 0x73, 0x71, 0x79, 0x2c, 0x72, 0x6f, 0x77, 0x78,
     0x73, 0x70, 0x46, 0x0};
    const unsigned char strs1[] =
    {0x51, 0x6f, 0x7a, 0x35, 0x80, 0x2c, 0x7d, 0x7c, 0x73, 0x7a, 0x2c, 0x78,
     0x77, 0x71, 0x73, 0x7a, 0x81, 0x73, 0x2c, 0x72, 0x77, 0x78, 0x73, 0x2c,
     0x3b, 0x2c, 0x81, 0x73, 0x73, 0x2c, 0x87, 0x7d, 0x83, 0x7e, 0x2c, 0x81,
     0x87, 0x81, 0x80, 0x73, 0x7b, 0x2c, 0x6f, 0x70, 0x7b, 0x77, 0x7a, 0x77,
     0x81, 0x80, 0x7e, 0x6f, 0x80, 0x7d, 0x7e, 0x3a, 0x16, 0x0};
    const unsigned char strs2[] =
    {0x53, 0x7e, 0x7e, 0x7d, 0x7e, 0x2c, 0x7e, 0x73, 0x6f, 0x70, 0x77, 0x7a,
     0x75, 0x2c, 0x78, 0x77, 0x71, 0x73, 0x7a, 0x81, 0x73, 0x2c, 0x72, 0x77,
     0x78, 0x73, 0x3a, 0x16, 0x0};
    const unsigned char strs3[] =
    {0x51, 0x74, 0x73, 0x71, 0x79, 0x81, 0x83, 0x7b, 0x2c, 0x73, 0x7e, 0x7e,
     0x7d, 0x7e, 0x2c, 0x77, 0x7a, 0x2c, 0x78, 0x77, 0x71, 0x73, 0x7a, 0x81,
     0x73, 0x2c, 0x72, 0x77, 0x78, 0x73, 0x3a, 0x16, 0x0};
    const unsigned char strs4[] =
    {0x33, 0x81, 0x16, 0x2c, 0x2c, 0x54, 0x7d, 0x81, 0x80, 0x2c, 0x33, 0x81,
     0x2c, 0x77, 0x81, 0x2c, 0x7a, 0x7d, 0x80, 0x2c, 0x78, 0x77, 0x71, 0x73,
     0x7a, 0x81, 0x73, 0x70, 0x2c, 0x80, 0x7d, 0x2c, 0x7e, 0x83, 0x7a, 0x2c,
     0x80, 0x74, 0x77, 0x81, 0x2c, 0x6e, 0x77, 0x7a, 0x6f, 0x7e, 0x87, 0x3a,
     0x16, 0x0};
    const unsigned char strs5[] =
    {0x33, 0x81, 0x16, 0x2c, 0x2c, 0x58, 0x77, 0x71, 0x73, 0x7a, 0x81, 0x73,
     0x2c, 0x72, 0x7d, 0x7e, 0x2c, 0x74, 0x7d, 0x81, 0x80, 0x2c, 0x33, 0x81,
     0x2c, 0x74, 0x6f, 0x81, 0x2c, 0x73, 0x84, 0x7c, 0x77, 0x7e, 0x73, 0x70,
     0x3a, 0x16, 0x0};
    const unsigned char strs6[] =
    {0x33, 0x81, 0x16, 0x2c, 0x2c, 0x58, 0x77, 0x71, 0x73, 0x7a, 0x81, 0x73,
     0x70, 0x2c, 0x83, 0x81, 0x73, 0x7e, 0x2c, 0x7f, 0x83, 0x7d, 0x80, 0x6f,
     0x2c, 0x73, 0x84, 0x71, 0x73, 0x73, 0x70, 0x73, 0x70, 0x3a, 0x16, 0x0};
    const unsigned char strs7[] =
    {0x78, 0x77, 0x71, 0x73, 0x7a, 0x81, 0x73, 0x2c, 0x82, 0x73, 0x7e, 0x77,
     0x72, 0x77, 0x71, 0x6f, 0x80, 0x77, 0x7d, 0x7a, 0x2c, 0x72, 0x6f, 0x83,
     0x78, 0x80, 0x3a, 0x16, 0x61, 0x7d, 0x7e, 0x7e, 0x87, 0x38, 0x2c, 0x6e,
     0x83, 0x80, 0x2c, 0x80, 0x74, 0x77, 0x81, 0x2c, 0x7c, 0x7e, 0x7d, 0x71,
     0x73, 0x81, 0x81, 0x2c, 0x85, 0x77, 0x78, 0x78, 0x2c, 0x6e, 0x73, 0x2c,
     0x59, 0x57, 0x58, 0x58, 0x53, 0x50, 0x2c, 0x57, 0x5a, 0x2c, 0x33, 0x70,
     0x2c, 0x5b, 0x57, 0x5a, 0x63, 0x60, 0x53, 0x61, 0x3a, 0x16, 0x5c, 0x78,
     0x73, 0x6f, 0x81, 0x73, 0x2c, 0x73, 0x84, 0x77, 0x80, 0x2c, 0x6f, 0x7a,
     0x70, 0x2c, 0x7e, 0x73, 0x81, 0x80, 0x6f, 0x7e, 0x80, 0x2c, 0x34, 0x7a,
     0x7d, 0x2c, 0x72, 0x83, 0x7e, 0x80, 0x74, 0x73, 0x7e, 0x2c, 0x85, 0x6f,
     0x7e, 0x7a, 0x77, 0x7a, 0x75, 0x81, 0x37, 0x3a, 0x16, 0x0};
}


sAuthChk::sAuthChk(bool local_mode)
{
    ac_license_host     = 0;
    ac_validation_host  = 0;
    ac_job_host         = 0;
    ac_working_ip       = 0;
    ac_working_alt      = 0;
    ac_validation_port  = 0;
    ac_open_pid         = 0;
    ac_local            = local_mode;
    ac_connected        = false;
    ac_armed            = false;
    ac_lastchk          = 0;
}


void
sAuthChk::set_validation_host(const char *host)
{
    delete [] ac_validation_host;
    ac_validation_host = host ? strdup(host) : 0;
}


// Attempt to validate the application.  If validation is successful,
// return the matching code.  The path is a search path for startup
// files.
//
int
sAuthChk::validate(int code, const char *path, const char *timefile)
{
    if (ac_local)
        return (validate_int(code, path));

    // The socket-based authorization is time consuming.  For WRspice,
    // this can be a problem when doing things like CMC tests which
    // consist of a large number of simple batch jobs.  Local auth is
    // much faster.  Here we fake it.  If the user gives us a
    // filename, we save time info, and skip the auth check if the
    // current time is close enough to the saved time.

    if (code == WRSPICE_CODE && timefile && *timefile) {
        FILE *fp = fopen(timefile, "rb+");
        if (fp) {
            time_t t0;
            fread(&t0, sizeof(time_t), 1, fp);
            unsigned char *c = (unsigned char*)&t0;
            for (int i = 0; i < (int)sizeof(time_t); i++) {
                *c = (*c ^ (0x26 + i)) - (17 - i);
                c++;
            }
            rewind(fp);
            time_t t1 = time(0);
            time_t tt = t1;
            c = (unsigned char*)&tt;
            for (int i = 0; i < (int)sizeof(time_t); i++) {
                *c = ((*c + 17 - i) ^ (0x26 + i));
                c++;
            }
            fwrite(&tt, sizeof(time_t), 1, fp);
            fclose(fp);
            if (t1 - t0 < 5) {
                ac_local = true;
                StateInitialized = true;
                BoxFilled = true;
                return (WRSPICE_CODE);
            }
        }
    }

    // The original pid is saved and used for further communication with
    // the server.  If the process forks, the pid stored in the server
    // is otherwise not valid.
    ac_open_pid = getpid();

    int first_err = ERR_OK, next_err = ERR_OK;
    bool ok = validation(XTV_OPEN, code, path, &first_err);

    // New scheme:  XicII and Xiv are no longer separate applications,
    // but licensing levels of Xic.  The passed code will be XIC_CODE
    // and not one of the derivatives.  We return the highest-level
    // code that matches the license file, which is used to
    // enable/disable features.

    if (!ok && code == XIC_CODE) {
        code = XICII_CODE;
        ok = validation(XTV_OPEN, code, path, &next_err);
        if (!ok) {
            code = XIV_CODE;
            ok = validation(XTV_OPEN, code, path, &next_err);
        }
        if (!ok) {
            code = XIC_DAEMON_CODE;
            ok = validation(XTV_OPEN, code, path, &next_err);
        }
        if (!ok)
            code = 0;
    }
    if (!ok && code == WRSPICE_CODE)
        code = 0;

    if (!ok) {
        if (first_err != ERR_OK)
            print_error(first_err);
        else if (next_err != ERR_OK)
            print_error(next_err);
        exit(1);
    }
    if (ok && code == WRSPICE_CODE && timefile && *timefile) {
        FILE *fp = fopen(timefile, "wb");
        if (fp) {
            time_t t1 = time(0);
            unsigned char *c = (unsigned char*)&t1;
            for (int i = 0; i < (int)sizeof(time_t); i++) {
                *c = ((*c + 17 - i) ^ (0x26 + i));
                c++;
            }
            fwrite(&t1, sizeof(time_t), 1, fp);
            fclose(fp);
        }
    }
    return (code);
}


// Remove the application from the server list.  It doesn't hurt to call
// this more than once.
//
void
sAuthChk::closeValidation(void)
{
    if (!ac_local) {
        validation(XTV_CLOSE, 0, 0, 0);
#ifdef WIN32
        Sleep(250);
#else
        struct timespec ts;
        ts.tv_sec = 0;
        ts.tv_nsec = 250000000;
        nanosleep(&ts, 0);
#endif
    }
}


// Send a command to the server, possible commands are XTV_DUMP,
// XTV_KILL, and XTV_CLOSE.  Return false if error.
//
bool
sAuthChk::serverCmd(const char *hostname, int cmd, const char *jhost, int jpid)
{
    if (!ac_local) {
        if (cmd == XTV_DUMP || cmd == XTV_KILL)
            return (validation(cmd, 0, hostname, 0));
        if (cmd == XTV_CLOSE) {
            int tpid = ac_open_pid;
            ac_open_pid = jpid;
            ac_job_host = jhost;
            bool ret = validation(cmd, 0, hostname, 0);
            ac_job_host = 0;
            ac_open_pid = tpid;
            return (ret);
        }
    }
    return (false);
}


// Static function.
//
void
sAuthChk::decode(const unsigned char *str, char *buf)
{
    for (int j = 0; ; j++) {
        buf[j] = (str[j] ? (str[j]^1) - 13 : 0);
        if (!str[j])
            break;
    }
    BoxFilled = true;
}


// Static function.
//
void
sAuthChk::print_error(int num)
{
    char buf[256], hdr[256], host[256];
    switch (num) {
    case ERR_NOLIC:
        decode(strs1, buf);
        err_printf(ET_ERROR, buf);
        break;
    case ERR_NOMEM:
        err_printf(ET_ERROR,
        "Memory allocation failure.\n");
        break;
    case ERR_RDLIC:
        decode(strs2, buf);
        err_printf(ET_ERROR, buf);
        break;
    case ERR_CKSUM:
        decode(strs3, buf);
        err_printf(ET_ERROR, buf);
        break;
    case ERR_NOTLIC:
        decode(strs0, hdr);
        decode(strs4, buf);
        gethostname(host, 256);
        err_printf(ET_ERROR, buf, hdr, host);
        break;
    case ERR_TIMEXP:
        decode(strs0, hdr);
        decode(strs5, buf);
        gethostname(host, 256);
        err_printf(ET_ERROR, buf, hdr, host);
        break;
    case ERR_USRLIM:
        decode(strs0, hdr);
        decode(strs6, buf);
        err_printf(ET_ERROR, buf, hdr);
        break;
    default:
    case ERR_UNKNO:
        err_printf(ET_ERROR, "unknown error.\n");
        break;
    }
}


// Non-inlined core of checkPeriodic, set ac_armed and return a string
// if the XTV_CHECK fails.
//
char *
sAuthChk::periodicTestCore()
{
    if (!validation(XTV_CHECK, 0, 0, 0)) {
        char buf[256];
        decode(strs7, buf);
        ac_armed = true;
        err_printf(ET_ERROR, buf, AC_LIFETIME_MINUTES);
        char *s = new char[strlen(buf) + 10];
        sprintf(s, buf,  AC_LIFETIME_MINUTES);
        return (s);
    }
    return (0);
}


namespace {
    void
    print_msg(const char *hostname, const char *ip, const char *hw, int code,
        int d)
    {
        if (getenv("XTLDEBUG")) {
            char buf[256];
            if (!ip && !hw)
                sprintf(buf, "Checking code %d permission for %s",
                    code, hostname);
            else {
                sprintf(buf, "Checking code %d permission for %s [%s]",
                    code, hostname, ip ? ip : hw);
            }
            if (!d)
                strcat(buf, "  OK");
            puts(buf);
        }
    }


    // Return true if s points to a string of digits terminated by white   
    // space or null.
    //
    bool check_digits(const char *s)
    {
        if (!isdigit(*s))
            return (false);
        while (*++s) {
            if (isspace(*s))
                break;
            if (!isdigit(*s))
                return (false);
        }
        return (true);
    }
}


// Interface to the licensing system.  The mode parameter
// takes the following values:
//  XTV_OPEN
//  XTV_CHECK
//  XTV_CLOSE (default)
// The program code signifies the application, but need be nonzero only
// for XTV_OPEN.  The strtpath is the search path for the startup files,
// and need be non-nil only for XTV_OPEN.
// Return value is true if validation passes, false otherwise.
// If this function is called, BoxFilled is set true (in decode()).  If
// return is successful, StateInitialized is also set true (in fill_req()).
// These conditions are used for backup security tests.
//
bool
sAuthChk::validation(int mode, int progcode, const char *strtpath,
    int *errcode)
{
    if (errcode)
        *errcode = ERR_OK;

    sJobReq c;
    if (mode != XTV_DUMP && mode != XTV_KILL) {
        if (!ac_license_host) {
            ac_license_host = get_license_host(strtpath);
            if (!ac_license_host) {
                err_printf(ET_ERROR, "out of memory.\n");
                return (false);
            }
        }
    }
    servent *sp = getservbyname(XTLSERV_SERVICE, "tcp");

    protoent *pp = getprotobyname("tcp");
    if (pp == 0) {
#ifdef WIN32
        MessageBox(0, "TCP: Unknown Protocol.\n"
            "Please see your operating system manual\n"
            "to set up TCP Networking service.", "Network Error", MB_ICONSTOP);
#else
        err_printf(ET_ERROR, "tcp: unknown protocol.\n");
#endif
        if (mode == XTV_OPEN)
            exit(1);
        return (false);
    }
    int tmp_port = 0;
    hostent *hp = 0;
    if (mode == XTV_DUMP || mode == XTV_KILL) {
        // The strtpath argument is the host name.
        if (strtpath && *strtpath) {
            const char *t = strrchr(strtpath, ':');
            if (t && check_digits(t+1)) {
                if (t == strtpath) {
                    hp = gethostbyname("localhost");
                    delete [] ac_license_host;
                    ac_license_host = strdup("localhost");
                }
                else {
                    char *hn = new char[t - strtpath + 1];
                    strncpy(hn, strtpath, t - strtpath);
                    hn[t - strtpath] = 0;
                    hp = gethostbyname(hn);
                    delete [] ac_license_host;
                    ac_license_host = hn;
                }
                tmp_port = atoi(t+1);
            }
            else {
                hp = gethostbyname(strtpath);
                delete [] ac_license_host;
                ac_license_host = strdup(strtpath);;
            }
        }
        else if (ac_license_host)
            hp = gethostbyname(ac_license_host);
        else {
            hp = gethostbyname("localhost");
            delete [] ac_license_host;
            ac_license_host = strdup("localhost");
        }
    }
    else
        hp = gethostbyname(ac_license_host);
    if (hp == 0) {
#ifdef WIN32
        MessageBox(0, "Host Name/Address Resolution Failed\n"
            "Please see your operating system manual\n"
            "to set up TCP Networking service.", "Network Error", MB_ICONSTOP);
#else
        err_printf(ET_ERROR,
            "Failed to resolve %s as a network host (gethostbyname failed).\n"
            "Adding this host to /etc/hosts might fix the problem.\n",
                ac_license_host);

#endif
        if (mode == XTV_OPEN)
            exit(1);
        return (false);
    }
    sockaddr_in server;
    memset(&server, 0, sizeof(sockaddr_in));
    memcpy(&server.sin_addr, hp->h_addr, hp->h_length);
    server.sin_family = hp->h_addrtype;

    unsigned short using_port = 0;
    if (tmp_port > 0)
        using_port = tmp_port;  // Port given with DUMP.
    else if (ac_validation_port > 0)
        using_port = ac_validation_port; // User supplied a port number.
    else if (sp)
        using_port = ntohs(sp->s_port);
    else
        using_port = XTLSERV_PORT;
    server.sin_port = htons(using_port);

    int sfd = open_srv(server, pp);
    if (sfd < 0) {
        if (mode == XTV_OPEN)
            exit(1);
        return (false);
    }
    using_port = ntohs(server.sin_port);

    if (mode == XTV_OPEN && !ac_connected) {
        printf("Connected to license server on %s:%d.\n", ac_license_host,
            using_port);
        ac_connected = true;
    }

    bool onepass = false;
    char hostname[256];
    if (gethostname(hostname, 256) < 0) {
        CLOSESOCKET(sfd);
        out_perror("gethostname");
        return (false);
    }

    switch (mode) {
    case XTV_OPEN:
    case XTV_CHECK:
    case XTV_CLOSE:
    case XTV_DUMP:
    case XTV_KILL:
        break;
    default:
        mode = XTV_CLOSE;
        break;
    }

#ifdef WIN32
    // It seems that Windows DHCP clients don't handle the hostname
    // setting via server feature.  This is good, as there is no
    // trouble with dhcp/non-dhcp breaking the licensing.  It might be
    // safer to use the function below to get the host name anyway,
    // this is the "computer name" not overridden by a "cluster name".
    //
    // GetComputerNameEx(ComputerNamePhysicalNetBIOS, hostname, 256);

    char winid[128];
    if (!msw::GetProductID(winid, 0)) {
        CLOSESOCKET(sfd);
        err_printf(ET_ERROR, "can't find Product ID.\n");
        return (false);
    }
    if (!write(c, sfd, hostname, 0, winid, progcode, mode))
        return (false);
    onepass = true;
#else
#ifdef __APPLE__
    // In Apple, the host name can change with DHCP, therefor we
    // currently ignore it.  The machine ID is sufficient.

    if (!ac_working_ip && !ac_working_alt) {
        char *sn = getMacSerialNumber();
        if (sn) {
            bool ret = write(c, sfd, sn, 0, 0, progcode, mode);
            delete [] sn;
            if (!ret)
                return (false);
        }
    }
    else {
        if (!write(c, sfd, hostname, ac_working_ip, ac_working_alt,
                progcode, mode))
            return (false);
    }
#else
#ifdef MASQUERADE
    if (!ac_working_ip && !ac_working_alt) {
        char *h = getenv("XTLTEST");
        if (h) {
            int a0, a1, a2, a3;
            char ahost[64];
            if (sscanf(h, "%s %d.%d.%d.%d", ahost, &a0, &a1, &a2, &a3) == 5) {
                ac_working_ip = new char[24];
                sprintf(ac_working_ip, "%d.%d.%d.%d", a0, a1, a2, a3);
                strcpy(hostname, ahost);
                onepass = true;
            }
        }
    }
#endif
    // Hack in an alternate host name, for xtjobs -p host pid.
    if (ac_job_host)
        strcpy(hostname, ac_job_host);

    if (!ac_working_ip && !ac_working_alt) {
        hostent *he = gethostbyname(hostname);
        if (!he) {
            CLOSESOCKET(sfd);
            err_printf(ET_MSG, "gethostbyname: unknown host %s.\n", hostname);
            return (false);
        }
        unsigned char *a = (unsigned char*)he->h_addr_list[0];
        ac_working_ip = new char[24];
        sprintf(ac_working_ip, "%d.%d.%d.%d", a[0], a[1], a[2], a[3]);
    }
    if (!write(c, sfd, hostname, ac_working_ip, ac_working_alt, progcode, mode))
        return (false);
#endif
#endif

    if (mode == XTV_CLOSE || mode == XTV_KILL) {
        CLOSESOCKET(sfd);
        return (true);
    }
    if (mode == XTV_DUMP) {
        bool ret = read_dump(sfd);
        CLOSESOCKET(sfd);
        return (ret);
    }

    int rval;
#ifdef __APPLE__
    if (ac_working_ip || ac_working_alt) {
        if (!check(c, sfd, hostname, ac_working_ip, ac_working_alt, mode,
                &rval))
            return (false);
    }
    else {
        char *sn = getMacSerialNumber();
        bool ret = check(c, sfd, sn, 0, 0, mode, &rval);
        delete [] sn;
        if (!ret)
            return (false);
    }
#else
    if (!check(c, sfd, hostname, ac_working_ip, ac_working_alt, mode, &rval))
        return (false);
#endif
    if (rval == ERR_OK) {
        decode(strs0, hostname);
        return (true);
    }

    if (!onepass && mode == XTV_OPEN && rval == ERR_NOTLIC) {

#ifdef __APPLE__
        char *sn = getMacSerialNumber();
        if (sn) {
            char tbf[32];
            strncpy(tbf, sn, 32);
            delete [] sn;

            // New-style Apple licenses use the ID string as the
            // hostname, with the magic IP.  The actual host name is a
            // wild card, since it can change with DHCP.  We've
            // already checkd this on the first pass, and it failed. 
            // Here we check the old style Apple license, that uses
            // the host name and the ID as the key.

            sfd = open_srv(server, pp);
            if (sfd < 0)
                return (false);
            if (!write(c, sfd, hostname, 0, tbf, progcode, mode))
                return (false);
            if (!check(c, sfd, hostname, 0, tbf, mode, &rval))
                return (false);
            if (rval == ERR_OK) {
                decode(strs0, hostname);
                return (true);
            }
        }
#endif

        // First access failed, try the other interface addresses.
        miscutil::ifc_t *ifc = miscutil::net_if_list();
        for (miscutil::ifc_t *i = ifc; i; i = i->next()) {

            if (i->ip() &&
                    (!ac_working_ip || strcmp(i->ip(), ac_working_ip))) {
                sfd = open_srv(server, pp);
                if (sfd < 0)
                    return (false);
                if (!write(c, sfd, hostname, i->ip(), 0, progcode, mode))
                    return (false);
                if (!check(c, sfd, hostname, i->ip(), 0, mode, &rval))
                    return (false);
                if (rval == ERR_OK) {
                    decode(strs0, hostname);
                    return (true);
                }
            }
            if (i->hw()) {
                sfd = open_srv(server, pp);
                if (sfd < 0)
                    return (false);
                if (!write(c, sfd, hostname, 0, i->hw(), progcode, mode))
                    return (false);
                if (!check(c, sfd, hostname, 0, i->hw(), mode, &rval))
                    return (false);
                if (rval == ERR_OK) {
                    decode(strs0, hostname);
                    return (true);
                }
            }
        }
    }
    if (errcode)
        *errcode = rval;

    return (false);
}


// Open and return a socket to the license server, return -1 on failure.
//
int
sAuthChk::open_srv(sockaddr_in &server, protoent *pp)
{
    // Create the socket
    int sfd = socket(AF_INET, SOCK_STREAM, pp->p_proto);
    if (sfd < 0) {
        out_perror("socket");
        return (-1);
    }

    int port = ntohs(server.sin_port);

    bool connected = (connect(sfd, (sockaddr*)&server, sizeof(sockaddr)) == 0);
    if (!connected && port == XTLSERV_PORT) {
        server.sin_port = htons(3010);
        // Have to get a new socket, probably is a way to reset old one.
        CLOSESOCKET(sfd);
        sfd = socket(AF_INET, SOCK_STREAM, pp->p_proto);
        if (sfd < 0) {
            out_perror("socket");
            return (-1);
        }
        connected = (connect(sfd, (sockaddr*)&server, sizeof(sockaddr)) == 0);
        if (connected) {
            // Found a license server on the old unregisterd port
            // 3010.  Continue, but warn the user.
            printf(
                "WARNING: connected to license server on old unregistered\n"
                "port number 3010.  Please update the license server package,"
                "\nwhich will use IANA registered port number %d.\n\n",
                XTLSERV_PORT);
            ac_validation_port = 3010;
            port = ac_validation_port;
        }
    }
    if (!connected) {
#ifdef WIN32
        char buf[256];
        sprintf(buf, "Error:  can't connect to license server on %s:%d.\n",
            ac_license_host, port);
        MessageBox(0, buf, "Error", MB_ICONSTOP);
#else
        err_printf(ET_ERROR, "can't connect to license server on %s:%d.\n",
            ac_license_host, port);
#endif
        return (-1);
    }
    return (sfd);
}


// Write a test block to the server,
//
bool
sAuthChk::write(sJobReq &c, int sfd, const char *hostname, const char *ip,
    const char *hw, int progcode, int mode)
{
    if (!fill_req(&c, hostname, ip, hw)) {
        out_perror("");
        CLOSESOCKET(sfd);
        return (false);
    }

    c.code = htonl(progcode);
    c.reqtype = htonl(mode);

    if (!write_block(sfd, &c, sizeof(sJobReq))) {
        err_printf(ET_ERROR, "write error\n");
        CLOSESOCKET(sfd);
        return (false);
    }
    return (true);
}


// Read and unscramble the server response, which is returned in rval.
// This will close the socket.
//
bool
sAuthChk::check(sJobReq &c, int sfd, char *hostname, const char *ip,
    const char *hw, int mode, int *rval)
{
    sJobReq cb;
    if (!read_block(sfd, &cb, sizeof(sJobReq))) {
        err_printf(ET_ERROR, "read error\n");
        CLOSESOCKET(sfd);
        return (false);
    }
    int d = unwind(&c, &cb);
    if (mode == XTV_OPEN)
        print_msg(hostname, ip, hw, ntohl(c.code), d);
    if (d == ERR_OK) {
        if (ip) {
            if (!ac_working_ip) {
                ac_working_ip = new char[24];
                strcpy(ac_working_ip, ip);
            }
        }
        else if (hw) {
            if (!ac_working_alt) {
                ac_working_alt = new char[24];
                strcpy(ac_working_alt, hw);
            }
        }
    }
    CLOSESOCKET(sfd);
    *rval = d;
    return (true);
}


// Obtain the licence host from 1) the environment, or 2) a file in
// the application startup directory, or 3) the server host alias, or
// 4) assume it is the present machine.  This must be called before
// Validate().
//
char *
sAuthChk::get_license_host(const char *path)
{
    char buf[256];
    *buf = '\0';
    if (ac_validation_host && *ac_validation_host)
        strcpy(buf, ac_validation_host);
    if (!*buf) {
        char *s = getenv(SERVERENV);
        if (s)
            strcpy(buf, s);
    }
    if (!*buf) {
        FILE *fp = pathlist::open_path_file(SERVERFILE, path, "r", 0, true);
        if (fp) {
            fgets(buf, 256, fp);
            fclose(fp);
            char *s = buf + strlen(buf) - 1;
            while (s >= buf && isspace(*s))
               *s-- = '\0';
        }
    }
    if (!*buf) {
        hostent *he = gethostbyname(SERVERHOST);
        if (he)
            strcpy(buf, he->h_name);
    }

    // The host name can have ":port" suffix, check for this.  Strip
    // and save the port number if found.
    char *t = strrchr(buf, ':');
    if (t && check_digits(t+1)) {
        *t++ = 0;
        ac_validation_port = atoi(t);
    }

    if (!*buf)
        gethostname(buf, 256);
    return (strdup(buf));
}


// Read a block of data from fd.  Returns false if error, including time
// out after IO_TIME_MS milliseconds.
//
bool
sAuthChk::read_block(int fd, void *buf, size_t size)
{
    fd_set readfds;
    int ms = IO_TIME_MS;
    timeval to;
    to.tv_sec = ms/1000;
    to.tv_usec = (ms % 1000)*1000;;

    unsigned int t0 = Tvals::millisec();
    for (;;) {
        FD_ZERO(&readfds);
        FD_SET((unsigned)fd, &readfds);
        int i = select(fd+1, &readfds, 0, 0, &to);
        if (i < 0) {
            if (errno == EINTR) {
                unsigned int t = Tvals::millisec();
                ms -= (t - t0);
                t0 = t;
                if (ms <= 0)
                    i = 0;
                else {
                    to.tv_sec = ms/1000;
                    to.tv_usec = (ms % 1000)*1000;
                    continue;
                }
            }
            else
                return (false);
        }
        if (i == 0)
            return (false);
        break;
    }

    size_t len = 0;
    for (;;) {
        int i = recv(fd, (char*)buf + len, size - len, 0);
        if (i <= 0) {
            if (i < 0 && errno == EINTR)
                continue;
            // error or EOF
            return (false);
        }
        len += i;
        if (len == size)
            break;
    }
    return (true);
}


// Read and print the stuff returned from a job dump request.
//
bool
sAuthChk::read_dump(int fd)
{
    char buf[256];
    fd_set readfds;
    int ms = IO_TIME_MS;
    timeval to;
    to.tv_sec = ms/1000;
    to.tv_usec = (ms % 1000)*1000;;

    unsigned int t0 = Tvals::millisec();
    for (;;) {
        FD_ZERO(&readfds);
        FD_SET(fd, &readfds);
        int i = select(fd+1, &readfds, 0, 0, &to);
        if (i == -1) {
            if (errno == EINTR) {
                unsigned int t = Tvals::millisec();
                ms -= (t - t0);
                t0 = t;
                if (ms <= 0)
                    i = 0;
                else {
                    to.tv_sec = ms/1000;
                    to.tv_usec = (ms % 1000)*1000;
                    continue;
                }
            }
            else
                return (false);
        }
        if (i == 0)
            return (false);
        break;
    }
    for (;;) {
        int i = recv(fd, buf, 256, 0);
        if (i <= 0) {
            if (i < 0 && errno == EINTR)
                continue;
            // error or EOF
            return (false);
        }
        if (*buf == '@')
            break;
        send(fd, "@", 1, 0);
        fprintf(stderr, "%s", buf);
    }
    return (true);
}


// Write a block of data to fd.  Returns false if error, including time
// out after IO_TIME_MS milliseconds.
//
bool
sAuthChk::write_block(int fd, void *buf, size_t size)
{
    fd_set wfds;
    size_t len = 0;
    int ms = IO_TIME_MS;
    timeval to;
    to.tv_sec = ms/1000;
    to.tv_usec = (ms % 1000)*1000;;

    unsigned int t0 = Tvals::millisec();
    for (;;) {
        FD_ZERO(&wfds);
        FD_SET(fd, &wfds);
        int i = select(fd+1, 0, &wfds, 0, &to);
        if (i == -1) {
            if (errno == EINTR) {
                unsigned int t = Tvals::millisec();
                ms -= (t - t0);
                t0 = t;
                if (ms <= 0)
                    i = 0;
                else {
                    to.tv_sec = ms/1000;
                    to.tv_usec = (ms % 1000)*1000;
                    continue;
                }
            }
            else
                return (false);
        }
        if (i == 0)
            return (false);
        break;
    }
    for (;;) {
        int i = send(fd, (char*)buf + len, size - len, 0);
        if (i <= 0)
            return (false);
        len += i;
        if (len == size)
            break;
    }
    return (true);
}


// Fill the job request struct.
//
bool
sAuthChk::fill_req(sJobReq *c, const char *host, const char *addr,
    const char *alt)
{
    memset(c, 0, sizeof(sJobReq));

    char buf[256];
    strcpy(buf, host);
    STRIPHOST(buf);

    if (alt) {
        char *t = buf + strlen(buf);
        *t++ = ':';
        strcpy(t, alt);
#ifndef WIN32
        // Under Windows, this is the ID code, otherwise it is a HW
        // address that needs to be lower case.
        while (*t) {
            if (isupper(*t))
                *t = tolower(*t);
            t++;
        }
#endif
        strncpy(c->host, buf, 64);
        c->addr[0] = 192;
        c->addr[1] = 168;
        c->addr[2] = 0;
        c->addr[3] = 1;
    }
    else if (addr) {
        strncpy(c->host, buf, 64);
        int a0, a1, a2, a3;
        if (sscanf(addr, "%d.%d.%d.%d", &a0, &a1, &a2, &a3) != 4)
            return (false);

        c->addr[0] = a0;
        c->addr[1] = a1;
        c->addr[2] = a2;
        c->addr[3] = a3;
    }
    else {
        strncpy(c->host, buf, 64);
        c->addr[0] = 192;
        c->addr[1] = 168;
        c->addr[2] = 0;
        c->addr[3] = 1;
    }

#ifdef HAVE_GETPWUID
    {
        passwd *pw;
        pw = getpwuid(getuid());
        if (pw)
            strncpy(c->user, pw->pw_name, 64);
        else
            strncpy(c->user, "Unknown User", 64);
    }
#else
#ifdef WIN32
    {
        DWORD len = 256;
        char tbuf[256];
        if (!GetUserName(tbuf, &len))
            strcpy(tbuf, "Unknown User");
        strncpy(c->user, tbuf, 64);
    }
#else
    strncpy(c->user, "Unknown User", 64);
#endif
#endif

    StateInitialized = true;
    c->date = htonl(time(0));
    c->reqtype = htonl(XTV_NONE);
    c->pid = htonl(ac_open_pid);
    c->code = 0;
    return (true);
}


int
sAuthChk::unwind(sJobReq *cref, sJobReq *c)
{
    size_t size = sizeof(sJobReq);
    MD5cx ctx;
    ctx.update((const unsigned char*)cref, size);
    unsigned char sum[16];
    ctx.final(sum);

    unsigned char nsum[16];
    for (int i = 0; i < 16; i++) {
        if (sum[i] < 127)
            nsum[15-i] = sum[i]+1;
        else if (sum[i] == 127)
            nsum[15-i] = 0;
        else
            nsum[15-i] = sum[i];
    }
    unsigned char *cc = (unsigned char *)c;

    // use first 8 bytes to generate index to sum
    int s = 0;
    for (int i = 0; i < 8; i++)
       s += cc[i];
    s %= (size - 26);  // size - (16 + 8 + 2)
    int i = s + 8;

    // copy sum from location
    for (int j = 0; j < 16; i++, j++)
        sum[j] = cc[i];
    int retval = cc[size-2];

    if (!retval && memcmp(sum, nsum, 16))
        return (ERR_NOTLIC);
    return (retval);
}


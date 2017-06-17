
/*========================================================================*
 *                                                                        *
 *  XicTools Integrated Circuit Design System                             *
 *  Copyright (c) 2008 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *  WHITELEY RESEARCH INCORPORATED PROPRIETARY SOFTWARE                   *
 *                                                                        *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,      *
 *   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES      *
 *   OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-        *
 *   INFRINGEMENT.  IN NO EVENT SHALL STEPHEN R. WHITELEY BE LIABLE       *
 *   FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION      *
 *   OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN           *
 *   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN         *
 *   THE SOFTWARE.                                                        *
 *                                                                        *
 *========================================================================*
 *                                                                        *
 * Licensing Interface Routines                                           *
 *                                                                        *
 *========================================================================*
 $Id: secure_int.cc,v 2.45 2015/11/16 01:14:37 stevew Exp $
 *========================================================================*/

//
// Local authentication code
//

#include "config.h"
#include "secure.h"
#include "secure_prv.h"
#include "encode.h"
#include "pathlist.h"
#include "miscutil.h"
#include "key.h"

#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifdef WIN32
#include "msw.h"
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#endif

#define AUTH_FILE "LICENSE"

#ifndef HAVE_STRERROR
#ifndef SYS_ERRLIST_DEF
char *sys_errlist[];
#endif
#endif

// Booby traps for hackers.
extern int StateInitialized;
extern int BoxFilled;

namespace {
    namespace secure_prv {
        FILE *open_license(int, const char*);
        int test_me(int);
        bool parse_etc_hosts(sJobReq*, char*);
        void alter_key(bool);
        int validate(sJobReq*);
        dblk *read_license(int*);
        dblk *find_valid(dblk*, sJobReq*);
        bool check_time(dblk*);
        bool check_users(dblk*, dblk*);

        // A simple class to handle text output.  Sometimes we want to print,
        // sometimes not.
        //
        struct sOut
        {
            void cat(const char*, ...);
            void dump(int);
            int eos() { return (strlen(OutBuf)); }
            void clear(int n) { OutBuf[n] = 0; }

        private:
            char *OutBuf;   // output buffer
        };
    }

    secure_prv::sOut out;

    const char *LicenseFileName = getenv("XT_LICENSE_PATH");

    // Error: can't find LICENSE file.  No \"LICENSE\" or \"LICENSE.dat\"
    // in\n%s\n
    const unsigned char strs0[] =
    {0x53, 0x7e, 0x7e, 0x7d, 0x7e, 0x46, 0x2c, 0x71, 0x6f, 0x7a, 0x35, 0x80,
     0x2c, 0x72, 0x77, 0x7a, 0x70, 0x2c, 0x58, 0x57, 0x51, 0x53, 0x5a, 0x61,
     0x53, 0x2c, 0x72, 0x77, 0x78, 0x73, 0x3a, 0x2c, 0x2c, 0x5a, 0x7d, 0x2c,
     0x2e, 0x58, 0x57, 0x51, 0x53, 0x5a, 0x61, 0x53, 0x2e, 0x2c, 0x7d, 0x7e,
     0x2c, 0x2e, 0x58, 0x57, 0x51, 0x53, 0x5a, 0x61, 0x53, 0x3a, 0x70, 0x6f,
     0x80, 0x2e, 0x2c, 0x77, 0x7a, 0x16, 0x33, 0x81, 0x16, 0x0};

    // Error: can't open LICENSE file, %s.\n
    const unsigned char strs1[] =
    {0x53, 0x7e, 0x7e, 0x7d, 0x7e, 0x46, 0x2c, 0x71, 0x6f, 0x7a, 0x35, 0x80,
     0x2c, 0x7d, 0x7c, 0x73, 0x7a, 0x2c, 0x58, 0x57, 0x51, 0x53, 0x5a, 0x61,
     0x53, 0x2c, 0x72, 0x77, 0x78, 0x73, 0x38, 0x2c, 0x33, 0x81, 0x3a, 0x16,
     0x0};

    // License file: %s\n
    const unsigned char strs2[] =
    {0x58, 0x77, 0x71, 0x73, 0x7a, 0x81, 0x73, 0x2c, 0x72, 0x77, 0x78, 0x73,
     0x46, 0x2c, 0x33, 0x81, 0x16, 0x0};

    // Error: LICENSE file size is not 1536, file must be corrupt.\n
    const unsigned char strs3[] =
    {0x53, 0x7e, 0x7e, 0x7d, 0x7e, 0x46, 0x2c, 0x58, 0x57, 0x51, 0x53, 0x5a,
     0x61, 0x53, 0x2c, 0x72, 0x77, 0x78, 0x73, 0x2c, 0x81, 0x77, 0x86, 0x73,
     0x2c, 0x77, 0x81, 0x2c, 0x7a, 0x7d, 0x80, 0x2c, 0x3f, 0x43, 0x41, 0x42,
     0x38, 0x2c, 0x72, 0x77, 0x78, 0x73, 0x2c, 0x7b, 0x83, 0x81, 0x80, 0x2c,
     0x6e, 0x73, 0x2c, 0x71, 0x7d, 0x7e, 0x7e, 0x83, 0x7c, 0x80, 0x3a, 0x16,
     0x0};

    // You must use the binary mode of ftp and similar programs when
    // transporting\nthe LICENSE file.\n
    const unsigned char strs4[] =
    {0x67, 0x7d, 0x83, 0x2c, 0x7b, 0x83, 0x81, 0x80, 0x2c, 0x83, 0x81, 0x73,
     0x2c, 0x80, 0x74, 0x73, 0x2c, 0x6e, 0x77, 0x7a, 0x6f, 0x7e, 0x87, 0x2c,
     0x7b, 0x7d, 0x70, 0x73, 0x2c, 0x7d, 0x72, 0x2c, 0x72, 0x80, 0x7c, 0x2c,
     0x6f, 0x7a, 0x70, 0x2c, 0x81, 0x77, 0x7b, 0x77, 0x78, 0x6f, 0x7e, 0x2c,
     0x7c, 0x7e, 0x7d, 0x75, 0x7e, 0x6f, 0x7b, 0x81, 0x2c, 0x85, 0x74, 0x73,
     0x7a, 0x2c, 0x80, 0x7e, 0x6f, 0x7a, 0x81, 0x7c, 0x7d, 0x7e, 0x80, 0x77,
     0x7a, 0x75, 0x16, 0x80, 0x74, 0x73, 0x2c, 0x58, 0x57, 0x51, 0x53, 0x5a,
     0x61, 0x53, 0x2c, 0x72, 0x77, 0x78, 0x73, 0x3a, 0x16, 0x0};

    // any_host_access
    const unsigned char strs23[] =
    {0x6f, 0x7a, 0x87, 0x6d, 0x74, 0x7d, 0x81, 0x80, 0x6d, 0x6f, 0x71, 0x71,
     0x73, 0x81, 0x81, 0x0};

    // any_host_access_b
    const unsigned char strs24[] =
    {0x6f, 0x7a, 0x87, 0x6d, 0x74, 0x7d, 0x81, 0x80, 0x6d, 0x6f, 0x71, 0x71,
     0x73, 0x81, 0x81, 0x6d, 0x6e, 0x0};

    // Level %d: %s@%s.
    const unsigned char strs30[] =
    {0x58, 0x73, 0x82, 0x73, 0x78, 0x2c, 0x33, 0x70, 0x46, 0x2c, 0x33, 0x81,
     0x4c, 0x33, 0x81, 0x3a, 0x16, 0x0};
}


// Attempt to validate the application.  If validation is successful,
// return the matching code.  The path is a search path for startup
// files.
//
int
sAuthChk::validate_int(int code, const char *path)
{
    char buf[256];
    FILE *fp = secure_prv::open_license(code, path);
    if (fp)
        fclose(fp);
    else {
        decode(strs0, buf);
        out.cat(buf, "startup or license directories, or bad path.");
        out.dump(true);
        exit(1);
    }

    // check LICENSE file (size 1536 is magic)
    decode(strs2, buf);
    out.cat(buf, LicenseFileName);
    struct stat st;
    if (stat(LicenseFileName, &st) < 0) {
        decode(strs1, buf);
        out.cat(buf, strerror(errno));
        out.dump(true);
        exit(1);
    }
    if (st.st_size != 1536) {
        decode(strs3, buf);
        out.cat(buf);
        decode(strs4, buf);
        out.cat(buf);
        out.dump(true);
        exit(1);
    }

    secure_prv::alter_key(false);
    int ern = out.eos();
    int err = secure_prv::test_me(code);

    // New scheme:  XicII and Xiv are no longer separate applications,
    // but licensing levels of Xic.  The passed code will be XIC_CODE
    // and not one of the derivatives.  We return the highest-level
    // code that matches the license file, which is used to
    // enable/disable features.

    if (err && code == XIC_CODE) {
        code = XICII_CODE;
        out.clear(ern);
        err = secure_prv::test_me(code);
        if (err) {
            code = XIV_CODE;
            out.clear(ern);
            err = secure_prv::test_me(code);
        }
        if (err) {
            code = XIC_DAEMON_CODE;
            out.clear(ern);
            err = secure_prv::test_me(code);
        }
        if (err)
            code = 0;
    }
    else if (err && code == WRSPICE_CODE)
        code = 0;

    if (err) {
        out.dump(true);
        sAuthChk::print_error(err);
        exit(1);
    }
    out.dump(false);
    return (code);
}
// End of sAuthChk functions.


void
secure_prv::sOut::cat(const char *fmt, ...)
{
    va_list args;
    char buf[256];
    va_start(args, fmt);
    vsnprintf(buf, 256, fmt, args);
    va_end(args);
    if (!OutBuf) {
        OutBuf = new char[strlen(buf) + 1];
        strcpy(OutBuf, buf);
        return;
    }
    char *s = new char[strlen(OutBuf) + strlen(buf) + 1];
    strcpy(s, OutBuf);
    strcat(s, buf);
    delete [] OutBuf;
    OutBuf = s;
}


void
secure_prv::sOut::dump(int error)
{
    if (OutBuf) {
#ifdef WIN32
        if (error)
            MessageBox(0, OutBuf, "ERROR", MB_ICONSTOP);
        else
            fputs(OutBuf, stdout);
#else
        fputs(OutBuf, error ? stderr : stdout);
#endif
        delete [] OutBuf;
        OutBuf = 0;
    }
}
// End of sOut functions.


// Return a pointer to the license file.
// Search order:
//    .../xictools/xxx/startup/LICENSE
//    .../xictools/xxx/startup/LICENSE.dat
//    .../xictools/license/LICENSE
//    .../xictools/license/LICENSE.dat
//
FILE *
secure_prv::open_license(int code, const char *path)
{
    if (LicenseFileName)
        return (fopen(LicenseFileName, "rb"));

    char buf[256];

#ifdef WIN32
    const char *program = 0;
    switch (code) {
    default:
    case SERVER_CODE:
        break;
    case XIV_CODE:
        program = "Xiv";
        break;
    case XICII_CODE:
        program = "XicII";
        break;
    case XIC_CODE:
    case XIC_DAEMON_CODE:
        program = "Xic";
        break;
    case WRSPICE_CODE:
        program = "WRspice";
        break;
    }

    // In Windows, first look in the locations from the registry.
    if (program) {
        char *proot = msw::GetProgramRoot(program);
        if (!proot)
            fprintf(stderr,
                "Warning: failed to find program root in registry.\n");
        else {
            strcpy(buf, proot);
            delete [] proot;
            char *p = strrchr(buf, '/');
            if (p) {
                char *e = buf + strlen(buf);

                sprintf(e, "/startup/%s", AUTH_FILE);
                FILE *fp = fopen(buf, "rb");
                if (fp) {
                    LicenseFileName = strdup(buf);
                    return (fp);
                }

                sprintf(e, "/startup/%s%s", AUTH_FILE, ".dat");
                fp = fopen(buf, "rb");
                if (fp) {
                    LicenseFileName = strdup(buf);
                    return (fp);
                }

                sprintf(p, "/license/%s", AUTH_FILE);
                fp = fopen(buf, "rb");
                if (fp) {
                    LicenseFileName = strdup(buf);
                    return (fp);
                }

                sprintf(p, "/license/%s%s", AUTH_FILE, ".dat");
                fp = fopen(buf, "rb");
                if (fp) {
                    LicenseFileName = strdup(buf);
                    return (fp);
                }
            }
        }
    }
#endif

    (void)code;
    if (path) {
        pathgen pg(path);
        char *p;
        while ((p = pg.nextpath(true)) != 0) {
            strcpy(buf, p);
            delete [] p;
            p = strstr(buf, "/xictools/");
            if (p) {
                p += strlen("/xictools/");
                char *s = p;
                while (*s != '/')
                    s++;
                if (!strcmp(s, "/startup")) {
                    char *e = buf + strlen(buf);

                    sprintf(e, "/%s", AUTH_FILE);
                    FILE *fp = fopen(buf, "rb");
                    if (fp) {
                        LicenseFileName = strdup(buf);
                        return (fp);
                    }

                    sprintf(e, "/%s.dat", AUTH_FILE);
                    fp = fopen(buf, "rb");
                    if (fp) {
                        LicenseFileName = strdup(buf);
                        return (fp);
                    }

                    sprintf(p, "license/%s", AUTH_FILE);
                    fp = fopen(buf, "rb");
                    if (fp) {
                        LicenseFileName = strdup(buf);
                        return (fp);
                    }

                    sprintf(p, "license/%s.dat", AUTH_FILE);
                    fp = fopen(buf, "rb");
                    if (fp) {
                        LicenseFileName = strdup(buf);
                        return (fp);
                    }
                }
            }
        }
    }

    // Try the home and current directories

    char *home = pathlist::get_home();
    if (home && *home) {
        char *tmppath = pathlist::mk_path(home, AUTH_FILE);
        delete [] home;
        strcpy(buf, tmppath);
        delete [] tmppath;
        FILE *fp = fopen(buf, "rb");
        if (fp) {
            LicenseFileName = strdup(buf);
            return (fp);
        }
        strcat(buf, ".dat");
        fp = fopen(buf, "rb");
        if (fp) {
            LicenseFileName = strdup(buf);
            return (fp);
        }
    }
    strcpy(buf, AUTH_FILE);
    FILE *fp = fopen(buf, "rb");
    if (fp) {
        LicenseFileName = strdup(buf);
        return (fp);
    }
    strcat(buf, ".dat");
    fp = fopen(buf, "rb");
    if (fp) {
        LicenseFileName = strdup(buf);
        return (fp);
    }
    return (0);
}


int
secure_prv::test_me(int code)
{
    char buf[256];
    sJobReq c;
    memset(&c, 0, sizeof(sJobReq));
    if (gethostname(buf, 256) < 0) {
        out.cat("Abort: gethostname system call failed\n");
        out.dump(true);
        exit(1);
    }

#ifdef WIN32
    STRIPHOST(buf);
    char *hnend = buf + strlen(buf);
    strcpy(hnend, ":");
    if (!msw::GetProductID(hnend + 1, 0)) {
        out.cat("Abort: ProductId not found in registry\n");
        out.dump(true);
        exit(1);
    }
    strncpy(c.host, buf, 64);
    c.addr[0] = 192;
    c.addr[1] = 168;
    c.addr[2] = 0;
    c.addr[3] = 1;
    c.code = htonl(code);
    c.reqtype = htonl(XTV_NONE);

    int ret = validate(&c);
    out.cat("Checking permissions...\n");
    out.cat("host:key %s %s\n", buf, ret ? "" : "  OK");
    *hnend = 0;
#else
#ifdef __APPLE__
    char *sn = getMacSerialNumber();
    if (sn) {
        // Newer licenses ignore the host name, which can change with
        // DHCP.  These licenses have the hostname set to the ID
        // string, and the magic IP is used.

        strncpy(c.host, sn, 64);
        c.addr[0] = 192;
        c.addr[1] = 168;
        c.addr[2] = 0;
        c.addr[3] = 1;
        c.code = htonl(code);
        c.reqtype = htonl(XTV_NONE);
        int ret = validate(&c);

        out.cat("Checking permissions...\n");
        out.cat("key %s %s\n", sn, ret ? "" : "  OK");

        STRIPHOST(buf);
        if (ret) {
            // Test with the host name, old license files.

            strcat(buf, ":");
            strcat(buf, sn);
            strncpy(c.host, buf, 64);
            ret = validate(&c);
            out.cat("host:key %s %s\n", buf, ret ? "" : "  OK");
        }
        delete [] sn;
        if (!ret) {
            StateInitialized = true;
            return (0);
        }
        gethostname(buf, 256);
    }
#endif
    hostent *he = gethostbyname(buf);
    if (!he) {
        char *t = strchr(buf, '.');
        if (t) {
            *t = 0;
            he = gethostbyname(buf);
            if (!he)
                out.cat("Warning: gethostbyname system call failed\n"
                    " gethostbyname failed for %s and %s.%s\n"
                    " checking /etc/hosts directly...\n", buf, buf, t+1);
        }
        else
            out.cat("Warning: gethostbyname system call failed\n"
                " gethostbyname failed for %s\n"
                " checking /etc/hosts directly...\n", buf);
    }
    if (!he) {
        if (parse_etc_hosts(&c, buf))
            strncpy(c.host, buf, 64);
        else {
            out.cat("Abort: parse_etc_hosts failed, can't resolve %s\n", buf);
            out.dump(true);
            exit(1);
        }
    }
    else {
        STRIPHOST(buf);
        strncpy(c.host, buf, 64);
        c.addr[0] = he->h_addr_list[0][0];
        c.addr[1] = he->h_addr_list[0][1];
        c.addr[2] = he->h_addr_list[0][2];
        c.addr[3] = he->h_addr_list[0][3];
    }
    c.code = htonl(code);
    c.reqtype = htonl(XTV_NONE);

    int ret = validate(&c);
    out.cat("Checking permission for %s [%u.%u.%u.%u]%s\n", buf,
        c.addr[0], c.addr[1], c.addr[2], c.addr[3], ret ? "" : "  OK");
#endif

    if (ret == ERR_NOTLIC) {
        char ipbuf[24];
#ifdef WIN32
        *ipbuf = 0;
#else
        sprintf(ipbuf, "%d.%d.%d.%d", c.addr[0], c.addr[1], c.addr[2],
            c.addr[3]);
#endif

        // Try other addresses found from interfaces on the system.
        //
        miscutil::ifc_t *ifc = miscutil::net_if_list();
        for (miscutil::ifc_t *i = ifc; i; i = i->next()) {
            if (i->ip() && strcmp(i->ip(), ipbuf)) {
                int a0, a1, a2, a3;
                if (sscanf(i->ip(), "%d.%d.%d.%d", &a0, &a1, &a2, &a3) == 4) {
                    strncpy(c.host, buf, 64);
                    c.addr[0] = a0;
                    c.addr[1] = a1;
                    c.addr[2] = a2;
                    c.addr[3] = a3;
                    ret = validate(&c);
                    out.cat("Checking permission for %s [%s]%s\n", buf,
                        i->ip(), ret ? "" : "  OK");
                    if (ret != ERR_NOTLIC)
                        break;
                }
            }
            if (i->hw()) {
                char tbuf[128];
                strcpy(tbuf, buf);
                char *t = tbuf + strlen(tbuf);
                *t++ = ':';
                strcpy(t, i->hw());
                // Make sure hw part is lower case.
                while (*t) {
                    if (isupper(*t))
                        *t = tolower(*t);
                    t++;
                }
                strncpy(c.host, tbuf, 64);
                c.addr[0] = 192;
                c.addr[1] = 168;
                c.addr[2] = 0;
                c.addr[3] = 1;
                ret = validate(&c);
                out.cat("Checking permission for %s [%s]%s\n", buf,
                    i->hw(), ret ? "" : "  OK");
                if (ret != ERR_NOTLIC)
                    break;
            }
        }
        ifc->free();
    }

    if (!ret)
        StateInitialized = true;
    return (ret);
}


// Look for name in /etc/hosts, if found fill in the address in c and
// return true.
//
bool
secure_prv::parse_etc_hosts(sJobReq *c, char *name)
{
    char buf[256];
    FILE *fp = fopen("/etc/hosts", "r");
    if (!fp)
        return (false);
    while (fgets(buf, 256, fp) != 0) {
        char *s = buf, *ad;
        while (isspace(*s))
            s++;
        ad = s;
        if (!isdigit(*s))
            continue;
        while (*s) {
            char *t;
            while (*s && !isspace(*s))
                s++;
            while (isspace(*s))
                s++;
            t = name;
            while (*t && *s && !isspace(*s)) {
                if (*t != *s)
                    break;
                t++;
                s++;
            }
            if (!*t) {
                int i1, i2, i3, i4;
                if (sscanf(ad, "%d.%d.%d.%d", &i1, &i2, &i3, &i4) == 4) {
                    c->addr[0] = i1;
                    c->addr[1] = i2;
                    c->addr[2] = i3;
                    c->addr[3] = i4;
                    fclose(fp);
                    return (true);
                }
            }
        }
    }
    fclose(fp);
    return (false);
}


// Encode the key while not in use.
//
void
secure_prv::alter_key(bool revert)
{
    unsigned int k = 2;
    if (!revert) {
        while (k < sizeof(key) - 5) {
            if (k & 1)
                key[k] = key[k] ^ 0xf;
            else
                key[k] = key[k] ^ 0x3c;
            k++;
        }
    }
    else {
        while (k < sizeof(key) - 5) {
            if (k & 1)
                key[k] = key[k] ^ 0xf;
            else
                key[k] = key[k] ^ 0x3c;
            k++;
        }
    }
}


// Main routine for validation testing.  Returns 0 if ok, an error code
// otherwise.
//
int
secure_prv::validate(sJobReq *c)
{
    alter_key(true);
    int error = 0;
    dblk *blocks = read_license(&error);
    alter_key(false);
    char host[68];
    strncpy(host, c->host, 64);
    if (error) {
#ifdef MAIL_ADDR
        if (error == ERR_CKSUM || error == ERR_SRVREQ) {
            // send mail - this could be a security concern
            char buf[512], fbuf[128];
            sAuthChk::decode(strs30, lstring::stpcpy(fbuf, "app: "));
            sprintf(buf, fbuf, error, c->user, host);
            miscutil::send_mail(MAIL_ADDR, "SecurityReport:ValidateInt", buf);
        }
#endif
        return (error);
    }
    alter_key(true);
    dblk *myblock = find_valid(blocks, c);
    alter_key(false);
    // host name truncated
    if (!myblock)
        error = ERR_NOTLIC;
    else if (!check_time(myblock))
        error = ERR_TIMEXP;
    else if (!check_users(myblock, blocks))
        error = ERR_USRLIM;
    memset(blocks, 0, (NUMBLKS+1)*sizeof(dblk));
    delete [] blocks;
    return (error);
}


// Open the authorization file and read it in.  Perform the checksum and
// make sure it matches that in the file.  If error, 0 is returned, and
// error is set: 1 can't open file, 2 malloc failed, 3 read error, 4
// checksum error.
//
dblk *
secure_prv::read_license(int *error)
{
    FILE *fp = open_license(SERVER_CODE, 0);
    if (!fp) {
        *error = ERR_NOLIC;
        return (0);
    }
    dblk *blks = new dblk[NUMBLKS+1];
    if (!blks) {
        // never get here
        fclose(fp);
        *error = ERR_NOMEM;
        return (0);
    }
    int size = (NUMBLKS+1)*sizeof(dblk);
    int msz = fread(blks, size, 1, fp);
    fclose(fp);
    if (msz != 1) {
        *error = ERR_RDLIC;
        return (0);
    }
    // copy checksum and zero before test
    unsigned char *sump = blks[NUMBLKS].sum;
    unsigned char sum[16];
    for (int j = 0; j < 16; j++) {
        sum[j] = sump[j];
        sump[j] = 0;
    }
    MD5cx ctx;
    ctx.update((const unsigned char*)blks, size);
    ctx.update((const unsigned char*)key, sizeof(key));
    unsigned char final[16];
    ctx.final(final);
    if (memcmp(sum, final, 16)) {
        *error = ERR_CKSUM;
        return (0);
    }
    *error = ERR_OK;
    return (blks);
}


// Find the block with matching checksum.
//
dblk*
secure_prv::find_valid(dblk *blocks, sJobReq *c)
{
    block myblk;
    int i;
    for (i = 0; c->host[i] && i < HOSTNAMLEN; i++) {
        char x = c->host[i];
        // Gen3 block hostname always lower case.
        myblk.hostname[i] = isupper(x) ? tolower(x) : x;
    }
    unsigned char *s = (unsigned char*)key;
    for ( ; i < HOSTNAMLEN; i++)
        myblk.hostname[i] = *s++;
    myblk.addr[0] = c->addr[0];
    myblk.addr[1] = c->addr[1];
    myblk.addr[2] = c->addr[2];
    myblk.addr[3] = c->addr[3];
    myblk.code[0] = ntohl(c->code);
    myblk.code[1] = key[0];
    myblk.code[2] = key[16];
    myblk.code[3] = key[32];

    MD5cx ctx;
    ctx.update((const unsigned char*)&myblk, sizeof(block));
    ctx.update((const unsigned char*)key, sizeof(key));
    unsigned char final[16];
    ctx.final(final);
    for (i = 0; i < NUMBLKS; i++)
        if (!memcmp(final, blocks[i].sum, 16))
            break;
    if (i < NUMBLKS)
        return (blocks + i);

    // If the host name contained an upper-case character, try again.
    // The Gen2 licenses do not have hostnames lower-cased.
    bool ucfound = false;
    for (i = 0; c->host[i] && i < HOSTNAMLEN; i++) {
        char x = c->host[i];
        if (isupper(x))
           ucfound = true;
        myblk.hostname[i] = x;
    }
    if (ucfound) {
        s = (unsigned char*)key;
        for ( ; i < HOSTNAMLEN; i++)
            myblk.hostname[i] = *s++;
        myblk.addr[0] = c->addr[0];
        myblk.addr[1] = c->addr[1];
        myblk.addr[2] = c->addr[2];
        myblk.addr[3] = c->addr[3];
        myblk.code[0] = ntohl(c->code);
        myblk.code[1] = key[0];
        myblk.code[2] = key[16];
        myblk.code[3] = key[32];

        ctx.reinit();
        ctx.update((const unsigned char*)&myblk, sizeof(block));
        ctx.update((const unsigned char*)key, sizeof(key));
        ctx.final(final);
        for (i = 0; i < NUMBLKS; i++)
            if (!memcmp(final, blocks[i].sum, 16))
                break;
        if (i < NUMBLKS)
            return (blocks + i);
    }

    if (c->code != SERVER_CODE && !strchr(c->host, ':')) {
        // No entry for host.  Look for a site license.
        // Servers and hardware address keys are ineligible.
        //
        char buf[64];
        sAuthChk::decode(strs23, buf);  // Class C
        for (i = 0; buf[i] && i < HOSTNAMLEN; i++)
            myblk.hostname[i] = buf[i];
        s = (unsigned char*)key;
        for ( ; i < HOSTNAMLEN; i++)
            myblk.hostname[i] = *s++;
        myblk.addr[0] = c->addr[0];
        myblk.addr[1] = c->addr[1];
        myblk.addr[2] = c->addr[2];
        myblk.addr[3] = 0;
        myblk.code[0] = ntohl(c->code);
        myblk.code[1] = key[0];
        myblk.code[2] = key[16];
        myblk.code[3] = key[32];

        ctx.reinit();
        ctx.update((const unsigned char*)&myblk, sizeof(block));
        ctx.update((const unsigned char*)key, sizeof(key));
        ctx.final(final);
        for (i = 0; i < NUMBLKS; i++)
            if (!memcmp(final, blocks[i].sum, 16))
                break;
        if (i < NUMBLKS)
            return (blocks + i);

        sAuthChk::decode(strs24, buf);  // Class B
        for (i = 0; buf[i] && i < HOSTNAMLEN; i++)
            myblk.hostname[i] = buf[i];
        s = (unsigned char*)key;
        for ( ; i < HOSTNAMLEN; i++)
            myblk.hostname[i] = *s++;
        myblk.addr[0] = c->addr[0];
        myblk.addr[1] = c->addr[1];
        myblk.addr[2] = 0;
        myblk.addr[3] = 0;
        myblk.code[0] = ntohl(c->code);
        myblk.code[1] = key[0];
        myblk.code[2] = key[16];
        myblk.code[3] = key[32];

        ctx.reinit();
        ctx.update((const unsigned char*)&myblk, sizeof(block));
        ctx.update((const unsigned char*)key, sizeof(key));
        ctx.final(final);
        for (i = 0; i < NUMBLKS; i++)
            if (!memcmp(final, blocks[i].sum, 16))
                break;
        if (i < NUMBLKS)
            return (blocks + i);
    }
    return (0);
}


// Check if trying to run after the expiration date,
// if set.  Return true if ok to proceed.
//
bool
secure_prv::check_time(dblk *blk)
{
    if (blk->timelim == 0)
        return (true);
    time_t loc;
    time(&loc);
    if ((unsigned)loc < ntohl(blk->timelim))
        return (true);
    return (false);
}


// If there is a user limit larger than one, return false.  This will
// prevent validation, the license server must be used for floating
// licenses.
//
bool
secure_prv::check_users(dblk *blk, dblk *blbase)
{
    if (blk->limits[0] != SERVER_CODE && blk->limits[1] &&
            blbase[blk->limits[3]].limits[2] > 1)
        return (false);
    return (true);
}


#ifdef __APPLE__

char *
secure::getMacSerialNumber()
{
    const char *cmd =
        "/usr/sbin/ioreg -l | "
        "/usr/bin/awk '/IOPlatformSerialNumber/ { print $4; }'";
    FILE *fp = popen(cmd, "r");
    if (!fp)
        return (0);

    char buf[256];
    char *s = fgets(buf, 256, fp);
    pclose(fp);
    if (s) {
        if (*s == '"') {
            s++;
            char *e = strchr(s, '"');
            if (e)
                *e = 0;
        }
        return (strdup(s));
    }
    return (0);
}

#endif


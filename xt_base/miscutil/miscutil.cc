
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2005 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
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
 * This library is available for non-commercial use under the terms of    *
 * the GNU Library General Public License as published by the Free        *
 * Software Foundation; either version 2 of the License, or (at your      *
 * option) any later version.                                             *
 *                                                                        *
 * A commercial license is available to those wishing to use this         *
 * library or a derivative work for commercial purposes.  Contact         *
 * Whiteley Research Inc., 456 Flora Vista Ave., Sunnyvale, CA 94086.     *
 * This provision will be enforced to the fullest extent possible under   *
 * law.                                                                   *
 *                                                                        *
 * This library is distributed in the hope that it will be useful,        *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU      *
 * Library General Public License for more details.                       *
 *                                                                        *
 * You should have received a copy of the GNU Library General Public      *
 * License along with this library; if not, write to the Free             *
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.     *
 *========================================================================*
 *                                                                        *
 * Misc. Utilities Library                                                *
 *                                                                        *
 *========================================================================*
 $Id: miscutil.cc,v 1.25 2015/12/08 04:28:06 stevew Exp $
 *========================================================================*/

#include "config.h"
#include "miscutil.h"
#include "lstring.h"
#include "pathlist.h"
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <fcntl.h>
#ifdef WIN32
#include "msw.h"
// #include "stackdump.h"
#include <iptypes.h>
#include <iphlpapi.h>
#else
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <net/if.h>
#include <execinfo.h>
#if defined(__FreeBSD__) || defined(__APPLE__)
#include <net/if_dl.h>
#endif
#endif


/*
 * Don't do this here, since it brings in several dll dependencies. 
 * Put this code in the application main, when this feature is really
 * wanted.

#ifdef WIN32
namespace {
    cStackDump _stackdump_(GDB_OFILE);
}
#endif
*/


// Return the date. Return value is static data.
//
const char *
miscutil::dateString()
{
#ifdef HAVE_GETTIMEOFDAY
    struct timeval tv;
    struct timezone tz;
    gettimeofday(&tv, &tz);
    time_t tt = tv.tv_sec;
    struct tm *tp = localtime(&tt);
    char *ap = asctime(tp);
#else
    time_t tloc;
    time(&tloc);
    struct tm *tp = localtime(&tloc);
    char *ap = asctime(tp);
#endif

    static char tbuf[40];
    strcpy(tbuf,ap);
    int i = strlen(tbuf);
    tbuf[i - 1] = '\0';
    return (tbuf);
}


miscutil::ifc_t *
miscutil::net_if_list()
{
#ifdef WIN32
    unsigned long sz = 16384;
    char *bfr = new char[sz];
    GCarray<char*> gc_bfr(bfr);
    IP_ADAPTER_ADDRESSES *aa = (IP_ADAPTER_ADDRESSES*)bfr;
    unsigned long ret = GetAdaptersAddresses(AF_INET,
        GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST, 0, aa, &sz);
    if (ret != ERROR_SUCCESS)
        return (0);

    ifc_t *i0 = 0, *iend = 0;
    for (IP_ADAPTER_ADDRESSES *a = aa; a; a = a->Next) {
        char *ifname = lstring::copy(a->AdapterName);
        // The name is a long string of gibberish, not useful to
        // humans.

        char *ipaddr = 0;
        IP_ADAPTER_UNICAST_ADDRESS *ip = a->FirstUnicastAddress;
        if (ip) {
            unsigned char *ipa =
                (unsigned char*)ip->Address.lpSockaddr->sa_data + 2;
            if (ipa[0] || ipa[1] || ipa[2] || ipa[3]) {
                ipaddr = new char[24];
                sprintf(ipaddr, "%d.%d.%d.%d", ipa[0], ipa[1], ipa[2], ipa[3]);
            }
        }
        char *hwaddr = 0;
        unsigned char *h = (unsigned char*)a->PhysicalAddress;
        if (h[0] || h[1] || h[2] || h[3] || h[4] || h[5]) {
            hwaddr = new char[24];
            sprintf(hwaddr, "%02x:%02x:%02x:%02x:%02x:%02x", h[0], h[1], h[2],
                h[3], h[4], h[5]);
        }

        if (ipaddr && hwaddr) {
            // We require both addresses.  The loopback 127.0.0.1 has
            // no hw address.

            if (!i0)
                i0 = iend = new ifc_t(ifname, ipaddr, hwaddr);
            else {
                iend->set_next(new ifc_t(ifname, ipaddr, hwaddr));
                iend = iend->next();
            }
        }
        else {
            delete [] ifname;
            delete [] ipaddr;
            delete [] hwaddr;
        }
    }
    return (i0);

#else
#ifdef linux

    char buf[4096];
    int s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0) {
        perror("socket");
        return (0);
    }
    ifc_t *i0 = 0, *iend = 0;

    FILE *fp = fopen("/proc/net/dev", "r");
    if (fp) {
        // The Linux kernel should provide this service, Thr text is a
        // list of interface names with statistics.  Here we extract
        // the interface names for use in obtaining the IP and HW
        // addresses of each interface, whether active or not.
        //
        // This solves a problem, The SIOCGIFCONF ioctl only lists
        // active interfaces, i.e., those with an IP address.  If the
        // license is keyed by the HW address, and the computer is not
        // connected to a net, the interface may not be active, and
        // the resolution will fail.

        stringlist *s0 = 0, *se = 0;
        while (fgets(buf, 4096, fp) != 0) {
            const char *line = buf;
            char *tok = lstring::gettok(&line);
            // Look for first tokens in the form "eth0:xxxxx".
            char *e = strchr(tok, ':');
            if (e) {
                *e = 0;
                if (!s0)
                    s0 = se = new stringlist(tok, 0);
                else {
                    se->next = new stringlist(tok, 0);
                    se = se->next;
                }
            }
            else
                delete [] tok;
        }
        fclose(fp);

        ifreq *req = (ifreq*)buf;
        for (stringlist *sl = s0; sl; sl = sl->next) {
            strcpy(req->ifr_name, sl->string);

            // Ignore the loopback interface.
            if (ioctl(s, SIOCGIFFLAGS, req) >= 0) {
                if (req->ifr_flags & IFF_LOOPBACK)
                    continue;
            }

            // Get the IP address.
            char *ipaddr = 0;
            if (ioctl(s, SIOCGIFADDR, req) >= 0) {
                unsigned char *a = (unsigned char*)
                    &((sockaddr_in*)&req->ifr_addr)->sin_addr;
                if (a[0] || a[1] || a[2] || a[3]) {
                    ipaddr = new char[24];
                    sprintf(ipaddr, "%d.%d.%d.%d", a[0], a[1], a[2], a[3]);
                    for (ifc_t *ix = i0; ix; ix = ix->next()) {
                        if (ix->ip() && !strcmp(ix->ip(), ipaddr)) {
                            // Already have it.
                            delete ipaddr;
                            ipaddr = 0;
                            break;
                        }
                    }
                }
            }

            // Get the HW (ethernet) address.
            char *hwaddr = 0;
            if (ioctl(s, SIOCGIFHWADDR, req) >= 0) {
                unsigned char *a = (unsigned char*)req->ifr_hwaddr.sa_data;
                if (a[0] || a[1] || a[2] || a[3] || a[4] || a[5]) {
                    hwaddr = new char[24];
                    sprintf(hwaddr, "%02x:%02x:%02x:%02x:%02x:%02x",
                        a[0], a[1], a[2], a[3], a[4], a[5]);
                    for (ifc_t *ix = i0; ix; ix = ix->next()) {
                        if (ix->hw() && !strcmp(ix->hw(), hwaddr)) {
                            // Already have it.
                            delete hwaddr;
                            hwaddr = 0;
                            break;
                        }
                    }
                }
            }

            if (ipaddr || hwaddr) {
                ifc_t *ifc = new ifc_t(lstring::copy(sl->string), ipaddr,
                    hwaddr);
                if (!i0)
                    i0 = iend = ifc;
                else {
                    iend->set_next(ifc);
                    iend = iend->next();
                }
            }
        }
        stringlist::destroy(s0);
    }
    else {
        struct ifconf ifc;
        ifc.ifc_len = 4096;
        ifc.ifc_buf = buf;
        if (ioctl(s, SIOCGIFCONF, &ifc) < 0) {
            close(s);
            perror("ioctl");
            return (0);
        }

        int nif = ifc.ifc_len/sizeof(struct ifreq);
        for (int i = 0; i < nif; i++) {

            // Ignore the loopback interface.
            if (ioctl(s, SIOCGIFFLAGS, &ifc.ifc_req[i]) >= 0) {
                if (ifc.ifc_req[i].ifr_flags & IFF_LOOPBACK)
                    continue;
            }

            // Get the IP address.
            char *ipaddr = 0;
            if (ioctl(s, SIOCGIFADDR, &ifc.ifc_req[i]) >= 0) {
                unsigned char *a = (unsigned char*)
                    &((sockaddr_in*)&ifc.ifc_req[i].ifr_addr)->sin_addr;
                if (a[0] || a[1] || a[2] || a[3]) {
                    ipaddr = new char[24];
                    sprintf(ipaddr, "%d.%d.%d.%d", a[0], a[1], a[2], a[3]);
                    for (ifc_t *ix = i0; ix; ix = ix->next()) {
                        if (ix->ip() && !strcmp(ix->ip(), ipaddr)) {
                            // Already have it.
                            delete ipaddr;
                            ipaddr = 0;
                            break;
                        }
                    }
                }
            }

            // Get the HW (ethernet) address.
            char *hwaddr = 0;
            if (ioctl(s, SIOCGIFHWADDR, &ifc.ifc_req[i]) >= 0) {
                unsigned char *a =
                    (unsigned char*)ifc.ifc_req[i].ifr_hwaddr.sa_data;
                if (a[0] || a[1] || a[2] || a[3] || a[4] || a[5]) {
                    hwaddr = new char[24];
                    sprintf(hwaddr, "%02x:%02x:%02x:%02x:%02x:%02x",
                        a[0], a[1], a[2], a[3], a[4], a[5]);
                    for (ifc_t *ix = i0; ix; ix = ix->next()) {
                        if (ix->hw() && !strcmp(ix->hw(), hwaddr)) {
                            // Already have it.
                            delete hwaddr;
                            hwaddr = 0;
                            break;
                        }
                    }
                }
            }
            if (ipaddr || hwaddr) {
                char *ifname = new char[strlen(ifc.ifc_req[i].ifr_name) + 1];
                strcpy(ifname, ifc.ifc_req[i].ifr_name);
                ifc_t *ix = new ifc_t(ifname, ipaddr, hwaddr);
                if (!i0)
                    i0 = iend = ix;
                else {
                    iend->set_next(ix);
                    iend = iend->next();
                }
            }
        }
    }
    close(s);
    return (i0);

#else
#if defined(__FreeBSD__) || defined(__APPLE__)

    int s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0) {
        perror("socket");
        return (0);
    }
    struct ifconf ifc;
    char buf[4096];
    ifc.ifc_len = 4096;
    ifc.ifc_buf = buf;
    if (ioctl(s, SIOCGIFCONF, &ifc) < 0) {
        close(s);
        perror("ioctl");
        return (0);
    }

    ifc_t *i0 = 0, *iend = 0;

    // The loop below differs from the Linux equivalent in that
    // 1) The "ifreq" array is variable-length.
    // 2) The interface names can appear more than once, one for each
    //    address.

    ifreq *req = &ifc.ifc_req[0];
    for (;;) {
        char *ifname = new char[strlen(req->ifr_name) + 1];
        strcpy(ifname, req->ifr_name);

        // Get HW (ethernet) address.
        char *hwaddr = 0;
        sockaddr_dl *sdl = (sockaddr_dl*)&req->ifr_addr;
        if (sdl->sdl_family == AF_LINK && sdl->sdl_alen == 6) {
            hwaddr = new char[24];
            unsigned char *a = (unsigned char*)sdl->sdl_data + sdl->sdl_nlen;
            sprintf(hwaddr, "%02x:%02x:%02x:%02x:%02x:%02x", a[0], a[1], a[2],
                a[3], a[4], a[5]);
        }

        // Get IP address;
        char *ipaddr = 0;
        sockaddr_in *sin = (sockaddr_in*)&req->ifr_addr;
        if (sin->sin_family == AF_INET) {
            ipaddr = new char[24];
            unsigned char *a = (unsigned char*) &sin->sin_addr;
            sprintf(ipaddr, "%d.%d.%d.%d", a[0], a[1], a[2], a[3]);
        }

        // Ignore the looback interface.
        if (strcmp(ifname, "lo0") && (ipaddr || hwaddr)) {
            if (!i0)
                i0 = iend = new ifc_t(ifname, ipaddr, hwaddr);
            else {
                bool found = false;
                for (ifc_t *ic = i0; ic; ic = ic->next()) {
                    if (!strcmp(ic->name(), ifname)) {
                        if (ipaddr)
                            ic->set_ip(ipaddr);
                        if (hwaddr)
                            ic->set_hw(hwaddr);
                        found = true;
                        delete [] ifname;
                        break;
                    }
                }
                if (!found) {
                    iend->set_next(new ifc_t(ifname, ipaddr, hwaddr));
                    iend = iend->next();
                }
            }
        }
        else {
            delete [] ifname;
            delete [] ipaddr;
            delete [] hwaddr;
        }

        req = (ifreq*)(_SIZEOF_ADDR_IFREQ(req[0]) + (char*)req);
        if ((char*)req - (char*)ifc.ifc_req >= ifc.ifc_len)
            break;
    }
    close(s);
    return (i0);

#else
    return (0);
#endif
#endif
#endif
}


// Send a quickie email message to mailaddr.
//
bool
miscutil::send_mail(const char *mailaddr, const char *subject,
    const char *body, const char *file)
{
    if (!mailaddr || !*mailaddr)
        return (false);
#ifdef WIN32
    // MapiSend returns error message.
    // If given, send the file as an attachment.
    if (file && *file) {
        if (msw::MapiSend(mailaddr, subject, body, 1, &file))
            return (false);
    }
    else {
        if (msw::MapiSend(mailaddr, subject, body))
            return (false);
    }
#else
    char buf[512];
    char *s = stpcpy(buf, "mail");
    if (subject && *subject) {
        bool quoted =  (*subject == '"' || *subject == '\'');
        *s++ = ' ';
        *s++ = '-';
        *s++ = 's';
        *s++ = ' ';
        if (!quoted)
            *s++ = '"';
        s = stpcpy(s, subject);
        if (!quoted)
            *s++ = '"';
    }
    *s++ = ' ';
    s = stpcpy(s, mailaddr);

    if (file && *file) {
        // In this case, ignore the body passed here, source the file.
        *s++ = ' ';
        *s++ = '<';
        *s++ = ' ';
        strcpy(s, file);
        system(buf);
    }
    else {
        FILE *fp = popen(buf, "w");
        if (!fp)
            return (false);
        fprintf(fp, "%s\n", body);
        pclose(fp);
    }
#endif
    return (true);
}


namespace {
    // Return a full path to the xterm executable if found.
    //
    char *xterm_path()
    {
        const char *xterm = "/usr/X11R6/bin/xterm";
        if (access(xterm, X_OK) == 0)
            return (lstring::copy(xterm));
        xterm = "/usr/X11/bin/xterm";
        if (access(xterm, X_OK) == 0)
            return (lstring::copy(xterm));
        xterm = "/usr/bin/xterm";
        if (access(xterm, X_OK) == 0)
            return (lstring::copy(xterm));
        xterm = "/usr/local/bin/xterm";
        if (access(xterm, X_OK) == 0)
            return (lstring::copy(xterm));

        const char *path = getenv("PATH");
        char *xtpath;
        if (pathlist::find_path_file("xterm", path, &xtpath, false))
            return (xtpath);
        return (0);
    }
}


// Pop up a terminal and execute cmd.  Return the pid, or 0 on error.
//
int
miscutil::fork_terminal(const char *cmd)
{
#ifdef WIN32
    PROCESS_INFORMATION *info = msw::NewProcess(cmd, CREATE_NEW_CONSOLE,
        false);
    if (info) {
        int pid = info->dwProcessId;
        delete info;
        return (pid);
    }
#else
    // XTerm is not around much anymore.
    char *termpath = xterm_path();
    if (termpath) {
        int pid = fork();
        if (pid < 0)
            return (0);
        if (pid == 0) {
            char *argv[4];
            argv[0] = (char*)lstring::strip_path(termpath);
            argv[1] = (char*)"-e";
            argv[2] = (char*)cmd;
            argv[3] = 0;
            execv(termpath, argv);
            return (0);
        }
        delete [] termpath;
        return (pid);
    }
    const char *gtpath  = "/usr/bin/gnome-terminal";
    if (access(gtpath, F_OK) == 0) {
        int pid = fork();
        if (pid < 0)
            return (0);

#define NEW_GTERM
#ifdef NEW_GTERM
        stringlist *s0 = 0;
        char **argv = 0;
        if (pid == 0) {
            // This is the new preferred form:  use "--" followed by
            // the command string.  This DOES NOT work if the command
            // is passed as a single token, so we have to retokenize
            // here.

            stringlist *se = 0;
            s0 = se = new stringlist(
                lstring::copy(lstring::strip_path(gtpath)), 0);
            se->next = new stringlist(lstring::copy("--"), 0);
            se = se->next;

            int tcnt = 2;
            char *tok;
            while ((tok = lstring::getqtok(&cmd)) != 0) {
                se->next = new stringlist(tok, 0);
                se = se->next;
                tcnt++;
            }
            tcnt++;
            argv = new char*[tcnt];
            tcnt = 0;
            for (stringlist *s = s0; s; s = s->next)
                argv[tcnt++] = s->string;
            argv[tcnt] = 0;
            execv(gtpath, argv);
            return (0);
        }
        delete [] argv;
        stringlist::destroy(s0);
        return (pid);
#else
        if (pid == 0) {
            // This now generates a warning from gnome-terminal that
            // "--command" is deprecated and may be removed.

            char *argv[4];
            argv[0] = (char*)lstring::strip_path(gtpath);
            argv[1] = (char*)"--command";
            argv[2] = (char*)cmd;
            argv[3] = 0;
            execv(gtpath, argv);
            return (0);
        }
        return (pid);
#endif

    }
#endif
    return (0);
}


#ifdef WIN32
#define GDB_XFILE "gdbexec.bat"
#else
#define GDB_XFILE "gdbexec"
#endif
#define GDB_SFILE "gdbtemp"

namespace {
    inline bool write_text(int fd, const char *text)
    {
        int n = strlen(text);
        return (write(fd, text, n) == n);
    }
}


//#define USE_GDB

// Dump a backtrace in GDB_OFILE to be used for debugging after
// a crash.
//
bool
miscutil::dump_backtrace(const char *program, const char *header,
    const char *logdir, const void *death_addr)
{
    // We're not reentrant!
    static bool here;
    if (here)
        return (false);
    here = true;

    int pid = getpid();
    if (!logdir || !*logdir)
        logdir = ".";

    // This function may be called with a corrupt heap, so we avoid
    // using memory allocation.  We can't be sure about the library
    // functions, except we avoid fopen which is guaranteed to
    // allocate a buffer.

#ifdef USE_GDB
    // Use the gdb program to obtain the stack backtrace.  This has
    // the advantages of giving demangled symbol names and line
    // numbers if symbols are present.  The disadvantage is that gdb
    // might not be around.
    //
    // This does NOT work in Microsoft Windows, at least not now.  If
    // gives only the frames after the unhandled exception handler. 
    // It is also not clear which thread to look at.

    // Remove any existing output file.
    if (access(GDB_OFILE, F_OK) == 0)
        unlink(GDB_OFILE);

    bool ok = true;

    // Print the gdb commands to run.
    char fnbuf[256];
    char *e = lstring::stpcpy(fnbuf, logdir);
    *e++ = '/';
    strcpy(e, GDB_SFILE);
    int fd = open(fnbuf, O_CREAT | O_WRONLY, 0644);
    if (fd < 0)
        ok = false;
    else {
#ifdef WIN32
//        ok = write_text(fd, "thread 2\nprint DeathAddr\nbt\nquit\n");
        ok = write_text(fd, 
            "thread 1\nprint DeathAddr\nbt\n"
            "thread 2\nprint DeathAddr\nbt\n"
            "thread 3\nprint DeathAddr\nbt\n"
            "thread 4\nprint DeathAddr\nbt\n"
            "thread 5\nprint DeathAddr\nbt\n"
            "quit\n");
#else
        ok = write_text(fd, "print DeathAddr\nbt\nquit\n");
#endif
        close(fd);
    }

    // Write the script that runs gdb.
    if (ok) {
        sprintf(fnbuf, "%s/%s", logdir, GDB_XFILE);
        fd = open(fnbuf, O_CREAT | O_WRONLY, 0644);
        if (fd < 0)
            ok = false;
    }
    char buf[256];
#ifdef WIN32
    if (ok)
        ok = write_text(fd, "@echo off\n");
#endif
    if (ok) {
        sprintf(buf, "echo Starting gdb %s %d\n", program, pid);
        ok = write_text(fd, buf);
    }
    if (ok) {
#ifdef WIN32
        const char *gdb = "c:\\usr\\local\\gtk2-bundle\\bin\\gdb";
        sprintf(buf, "%s -x %s/%s -q %s %d > %s\n", gdb,
            logdir, GDB_SFILE, program, pid, GDB_OFILE);
#else
        sprintf(buf, "{ gdb -x %s/%s -q %s %d; } > %s 2>&1\n",
            logdir, GDB_SFILE, program, pid, GDB_OFILE);
#endif
        ok = write_text(fd, buf);
    }
    if (ok && header && *header) {
        sprintf(buf, "echo \"%s\" >> %s\n", header, GDB_OFILE);
        ok = write_text(fd, buf);
    }
    if (ok) {
        sprintf(buf, "echo \"%s  pid = %d\" >> %s\n", dateString(), pid,
            GDB_OFILE);
        ok = write_text(fd, buf);
    }
    if (ok) {
#ifdef WIN32
        sprintf(buf,
            "echo Wrote '%s' file.\n"
            "sleep 2\n", GDB_OFILE);
#else
        sprintf(buf,
            "echo Wrote '%s' file.\n"
            "kill -CONT %d\n"
            "sleep 2\n", GDB_OFILE, pid);
#endif
        ok = write_text(fd, buf);
    }
    if (fd >= 0)
        close(fd);

    if (ok) {
#ifdef WIN32
        sprintf(buf, "cmd /c %s", fnbuf);
#else
        sprintf(buf, "/bin/sh %s", fnbuf);
#endif
        system(buf);
        here = false;
        return (true);
    }

    // Failed to dump the gdb script, user can control gdb manually.

    fprintf(stderr, "\nStarting gdb debugger...\n");
    fprintf(stderr,
        "\nPLEASE TYPE `bt' TO GDB, AND COPY RESULT TO %s.\n",
        "Whiteley Research");
    fprintf(stderr,
        "Also include a brief description of the operation "
        "that caused\nthe fault, and any log files.\n");
    fprintf(stderr, "Type `quit' to exit gdb.\n");
    sprintf(buf, "gdb -q %s %d", program, pid);
    system(buf);

#else
    // Use a local function to get the backtrace.  This provides the
    // raw addresses and mangled names, which is actually good enough.

#ifdef WIN32
    // The gdbout file has already been written by cStackDump (we hope).
    int fd = open(GDB_OFILE, O_WRONLY | O_APPEND);
#else
    // Remove any existing output file.
    if (access(GDB_OFILE, F_OK) == 0)
        unlink(GDB_OFILE);

    int fd = open(GDB_OFILE, O_CREAT | O_WRONLY, 0644);
#endif

    if (fd < 0) {
        here = false;
        return (false);
    }
    if (!header)
        header = program;
    if (!header)
        header = "stack backtrace";
    write_text(fd, header);
    write_text(fd, "\n");
    const char *s = dateString();
    write_text(fd, s);
    write_text(fd, "  ");
    char buf[32];
    sprintf(buf, "Pid = %d", pid);
    write_text(fd, buf);
    if (death_addr) {
        write_text(fd, "  ");
        sprintf(buf, "DeathAddr = 0x%lx", (unsigned long)death_addr);
        write_text(fd, buf);
    }
#ifdef WIN32
    write_text(fd, "\n");
#else
    if (write_text(fd, "\n")) {
#define MAX_FRAMES 256
        void *addr[MAX_FRAMES];
        int nframes = backtrace(addr, MAX_FRAMES);
        backtrace_symbols_fd(addr, nframes, fd);
    }
#endif
    close(fd);
#endif

    here = false;
    return (true);
}


//-----------------------------------------------------------------------------

#ifdef TEST_HWADDR
int main()
{
    miscutil::ifc_t *ifc = miscutil::net_if_list();
    for (miscutil::ifc_t *i = ifc; i; i = i->next()) {
        printf("%s:\n", i->name());
        if (i->ip())
            printf("  IP %s\n", i->ip());
        if (i->hw())
            printf("  HW %s\n", i->hw());
    }
    return (0);
}
#endif



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
 * Xic Integrated Circuit Layout and Schematic Editor                     *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#include "config.h"
#include "fio.h"
#include "fio_oasis.h"
#include "fio_cgd.h"
#include "fio_cgd_lmux.h"
#include "geo_zlist.h"
#include "si_parsenode.h"
#include "si_daemon.h"
#include "si_parser.h"
#include "si_interp.h"
#include "si_handle.h"
#include "si_lspec.h"
#include "cd_digest.h"
#include "miscutil/miscutil.h"
#include "miscutil/crypt.h"
#include "miscutil/services.h"

#include <ctype.h>
#include <errno.h>
#ifdef HAVE_PWD_H
#include <pwd.h>
#endif
#ifdef WIN32
#include <conio.h>
#include <winsock2.h>
typedef int int32_t;
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/wait.h>
#include <fcntl.h>
#endif


#ifndef HAVE_STRERROR
#ifndef SYS_ERRLIST_DEF
extern char *sys_errlist[];
#endif
#endif

// This is the daemon code for server mode (-S option).  The daemon
// listens on the given port for commands, executes commands as it
// receives them, and returns status information.  The commands are
// statements in the scripting language, and the return is the return
// value of the statement.  In addition, there are a few global
// commands for initialization and to kill the server.

siDaemon *siDaemon::d_daemon = 0;


// Static Function
// Main function to start the daemon.  This blocks until the server is
// given a kill command or otherwise dies.
//
int
siDaemon::start(int port, siDaemonIf *dif)
{
    if (daemon())
        return (-1);
    siDaemon d(port, dif);
    if (!d.init())
        return (1);
    d.to_bg();
    return ((int)d.start_listening());
}


// Static Function
// Return the server socket, or -1.  This is needed by the application's
// signal handler.
//
int
siDaemon::server_skt()
{
    if (daemon())
        return (daemon()->d_server_skt);
    return (-1);
}


namespace {
    void
    dump_msg(const char *msg)
    {
        char tbuf[256];
        strncpy(tbuf, msg, 256);
        tbuf[255] = 0;
        printf("|%s|\n", tbuf);
        fflush(stdout);
    }
}


siDaemon::siDaemon(int p, siDaemonIf *dif)
{
    d_daemon = this;
    d_if = dif;

    d_port = p;
    if (d_port <= 0) {
        servent *sp = getservbyname(XIC_SERVICE, "tcp");
        if (sp)
            d_port = ntohs(sp->s_port);
        else
            d_port = XIC_PORT;
    }

    for (int i = 0; i < D_MAX_OPEN; i++)
        d_channels[i].socket = -1;

    d_logfp = 0;
    d_errfp = 0;
    d_msg = 0;
    d_acc_skt = -1;
    d_channel = -1;
    d_server_skt = -1;
    d_listening = false;
    d_debug = getenv("XTNETDEBUG");
    d_keepall = false;

    d_ftab = new SymTab(true, false);
    d_ftab->add(lstring::copy("close"),        (const void*)&f_close,
        false);
    d_ftab->add(lstring::copy("kill"),         (const void*)&f_kill,
        false);
    d_ftab->add(lstring::copy("reset"),        (const void*)&f_reset,
        false);
    d_ftab->add(lstring::copy("clear"),        (const void*)&f_clear,
        false);
    d_ftab->add(lstring::copy("longform"),     (const void*)&f_longform,
        false);
    d_ftab->add(lstring::copy("shortform"),    (const void*)&f_shortform,
        false);
    d_ftab->add(lstring::copy("dumpmsg"),      (const void*)&f_dumpmsg,
        false);
    d_ftab->add(lstring::copy("nodumpmsg"),    (const void*)&f_nodumpmsg,
        false);
    d_ftab->add(lstring::copy("dieonerror"),   (const void*)&f_dieonerror,
        false);
    d_ftab->add(lstring::copy("nodieonerror"), (const void*)&f_nodieonerror,
        false);
    d_ftab->add(lstring::copy("keepall"),      (const void*)&f_keepall,
        false);
    d_ftab->add(lstring::copy("nokeepall"),    (const void*)&f_nokeepall,
        false);
    d_ftab->add(lstring::copy("geom"),         (const void*)&f_geom,
        false);
}


siDaemon::~siDaemon()
{
    d_daemon = 0;
    for (int i = 0; i < D_MAX_OPEN; i++) {
        if (d_channels[i].socket >= 0)
            close_socket(d_channels[i].socket);
    }
    if (d_acc_skt > 0)
        close_socket(d_acc_skt);
    if (d_logfp)
        fclose(d_logfp);
    if (d_errfp)
        fclose(d_errfp);
    delete [] d_msg;
    delete d_ftab;
}


bool
siDaemon::init()
{
    protoent *pp = getprotobyname("tcp");
    if (pp == 0) {
        fprintf(stderr, "Error: tcp: unknown protocol\n");
        return (false);
    }
    char *idstring = 0;
    if (d_if)
        idstring = d_if->app_id_string();
    if (idstring) {
        fprintf(stderr, "%s daemon (pid %d), listening on port %d.\n",
            idstring, (int)getpid(), d_port);
        delete [] idstring;
    }
    else
        fprintf(stderr, "Starting daemon (pid %d), listening on port %d.\n",
            (int)getpid(), d_port);

    // Create the socket
    d_acc_skt = socket(AF_INET, SOCK_STREAM, pp->p_proto);
    if (d_acc_skt < 0) {
        perror("socket");
        return (false);
    }

    // This avoids system delay in rebinding
    int on = 1;
    setsockopt(d_acc_skt, SOL_SOCKET, SO_REUSEADDR, (char*)&on, sizeof(on));

    sockaddr_in sin;
    sin.sin_port = htons(d_port);
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    if (bind(d_acc_skt, (sockaddr*)&sin, sizeof(sockaddr_in)) < 0) {
        close_socket(d_acc_skt);
        perror("bind");
        return (false);
    }
    return (true);
}


void
siDaemon::to_bg()
{
#ifndef WIN32
    if (d_debug)
        return;

    if (fork()) {
        close(d_acc_skt);
        exit(0);
    }
    for (int i = 0; i < 10; i++) {
        if (i != d_acc_skt)
            close(i);
    }
    if (d_if)
        d_logfp = d_if->app_open_log("daemon_out.log", "w");
    if (!d_logfp)
        d_logfp = fopen("daemon_out.log", "w");
    if (d_logfp) {
        dup2(fileno(d_logfp), 1);
        // Get rid of buffering so output shows up in the log files
        // immediately
        setvbuf(d_logfp, 0, _IONBF, 0);
        setvbuf(stdout, 0, _IONBF, 0);
    }
    if (d_if)
        d_errfp = d_if->app_open_log("daemon_err.log", "w");
    if (!d_errfp)
        d_errfp = fopen("daemon_err.log", "w");
    if (d_errfp) {
        dup2(fileno(d_errfp), 2);
        setvbuf(d_errfp, 0, _IONBF, 0);
    }
    setsid();
#endif
}

bool
siDaemon::start_listening()
{
    if (d_if)
        d_if->app_listen_init();

    log_printf("\n-- new daemon, pid = %d, date = %s\n\n",
        (int)getpid(), miscutil::dateString());

    // Start listening for requests
    listen(d_acc_skt, D_MAX_OPEN);

    fd_set fds;
    sockaddr_in from;
    socklen_t len = sizeof(sockaddr_in);
    d_listening = true;
    while (d_listening) {

        // Initialize the active socket bit array.
        int nfds = d_acc_skt;
        int numactive = 0;
        FD_ZERO(&fds);
        FD_SET(d_acc_skt, &fds);
        for (int i = 0; i < D_MAX_OPEN; i++) {
            if (d_channels[i].socket > 0) {
                FD_SET(d_channels[i].socket, &fds);
                if (d_channels[i].socket > nfds)
                    nfds = d_channels[i].socket;
                numactive++;
            }
        }
        nfds++;

        int ret = select(nfds, &fds, 0, 0, 0);
        if (ret < 0) {
            if (errno == EINTR)
                continue;
            break;
        }
        else if (ret == 0)
            continue;
        else {
            if (FD_ISSET(d_acc_skt, &fds)) {
                // Ready to accept a new connection.
                int skt = accept(d_acc_skt, (sockaddr*)&from, &len);
                if (d_acc_skt < 0) {
                    if (errno != EINTR) {
                        log_perror(">>> accept");
                        close_socket(d_acc_skt);
                        return (false);
                    }
                    continue;
                }
                for (int i = 0; i < D_MAX_OPEN; i++) {
                    if (d_channels[i].socket < 0) {
                        d_channels[i].set(skt);
                        break;
                    }
                }
#ifdef F_SETOWN
                // Set socket pid for oob data, oob is used to pass
                // interrupts.
                fcntl(skt, F_SETOWN, getpid());
#endif
                if (d_debug)
                    fprintf(stderr, "connected\n");
                log_printf("Connected, session start %s.\n",
                    miscutil::dateString());
                numactive++;
                if (numactive == 1 && !d_keepall)
                    SI()->LineInterp(0, 0, true);

                // initial "prompt"
                int o = htonl(RSP_OK);
                if (send(skt, (char*)&o, 4, 0) < 0) {
                    log_perror(">>> send");
                    break;
                }
            }

            // Handle pending message requests.
            for (int i = 0; i < D_MAX_OPEN; i++) {
                if (d_channels[i].socket > 0 &&
                        FD_ISSET(d_channels[i].socket, &fds)) {

                    int skt = d_channels[i].socket;
                    d_channel = i;
                    d_server_skt = skt;
                    ret = transact();
                    d_server_skt = -1;
                    d_channel = -1;

                    if (ret != DMNok) {
                        numactive--;
                        d_channels[i].socket = -1;
                        close_socket(skt);
                        log_printf("Close connection %s.\n",
                            miscutil::dateString());
                        if (d_debug)
                            fprintf(stderr, "connection closed\n");

                        if (!numactive && !d_keepall) {
                            SI()->Clear();
                            if (d_if)
                                d_if->app_clear();
                        }

                        if (ret == DMNerror) {
                            if (d_channels[i].die_on_error)
                                d_listening = false;
                        }
                        else if (ret == DMNfatal || ret == DMNkill)
                            d_listening = false;
                    }
                }
            }
            if (!d_listening) {
                // Close all channels, probably redundant.
                for (int i = 0; i < D_MAX_OPEN; i++) {
                    if (d_channels[i].socket > 0) {
                        close_socket(d_channels[i].socket);
                        d_channels[i].socket = -1;
                    }
                }
            }
        }
    }
    close_socket(d_acc_skt);
    return (true);
}


DMNenum
siDaemon::transact()
{
    if (d_debug)
        fprintf(stderr, "message sent, waiting recv\n");

    Dchannel *ch = channel();
    if (!ch)
        return (DMNerror);

    if (d_if)
        d_if->app_transact_init();
    SI()->ClearInterrupt();
    d_msg = recv_msg(ch->socket);
    if (!d_msg) {
        log_perror(">>> recv");
        return (DMNerror);
    }
    char *t = d_msg + strlen(d_msg) - 1;
    while (t >= d_msg && (*t == '\n' || *t == '\r'))
        *t-- = 0;

    if (ch->dumpmsg)
        dump_msg(d_msg);

    t = d_msg;
    char *tok = lstring::gettok(&t);
    DMNfunc func = (DMNfunc)SymTab::get(d_ftab, tok);
    delete [] tok;
    if (func != (DMNfunc)ST_NIL)
        return ((*func)(t));

    siVariable v;
    int rs, ret = SI()->LineInterp(d_msg, &v);
    clearmsg();
    if (ret == 0)
        rs = respond(&v, ch->longform);
    else if (ret == 1)
        rs = respond(RSP_MORE);
    else
        rs = respond(RSP_ERR);
    v.gc_result();
    if (rs < 0) {
        log_perror(">>> send");
        return (DMNerror);
    }
    return (DMNok);
}


int
siDaemon::respond(RSPtype rsp)
{
    Dchannel *ch = channel();
    if (!ch)
        return (-1);

    int o = htonl(rsp);
    int ret = send(ch->socket, (char*)&o, 4, 0);
    return (ret);
}


int
siDaemon::respond(siVariable *v, bool longform)
{
    Dchannel *ch = channel();
    if (!ch || !v)
        return (-1);

    return (v->respond(ch->socket, longform));
}


void
siDaemon::close_socket(int skt)
{
    if (skt > 0) {
#ifdef WIN32
        shutdown(skt, SD_SEND);
        closesocket(skt);
#else
        close(skt);
#endif
        if (skt == d_acc_skt)
            d_acc_skt = -1;
    }
}


// Static function.
// Print a formatted string to the log file.
//
void
siDaemon::log_printf(const char *fmt, ...)
{
    va_list args;
    char buf[BUFSIZ];
    va_start(args, fmt);
    vsnprintf(buf, BUFSIZ, fmt, args);
    va_end(args);

    FILE *fp = 0;
    if (d_daemon->d_if)
        fp = d_daemon->d_if->app_open_log("daemon.log", "a");
    if (!fp)
        fp = fopen("daemon.log", "a");
    if (fp) {
        fputs(buf, fp);
        fclose(fp);
    }
}


// Static function.
// Replacement for perror() which supports diversion to log file.
//
void
siDaemon::log_perror(const char *str)
{
#ifdef HAVE_STRERROR
    if (str && *str)
        log_printf("%s: %s\n", str, strerror(errno));
    else
        log_printf("%s\n", strerror(errno));
#else
    if (str && *str)
        log_printf("%s: %s\n", str, sys_errlist[errno]);
    else
        log_printf("%s\n", sys_errlist[errno]);
#endif
}


// Static function.
// Wait for a message on skt, then return it.
//
char *
siDaemon::recv_msg(int skt)
{
    fd_set fd;
    timeval to;
    to.tv_sec = 0;
    to.tv_usec = 0;
    for (;;) {
        FD_ZERO(&fd);
        FD_SET(skt, &fd);
        int si = select(skt+1, &fd, 0, 0, 0);
        if (si > 0) {
            sLstr lstr;
            do {
                char c;
                int ri = recv(skt, &c, 1, 0);
                if (ri == 0)
                    return (0);
                if (ri == 1)
                    lstr.add_c(c);
                else if (ri < 0) {
                    if (errno == EINTR)
                        continue;
                    return (0);
                }
                FD_SET(skt, &fd);
                si = select(skt+1, &fd, 0, 0, &to);

            } while (si > 0);
            return (lstr.string_trim());
        }
        else if (si < 0) {
            if (errno == EINTR)
                continue;
            return (0);
        }
    }
}


// Static function
DMNenum
siDaemon::f_close(const char*)
{
    d_daemon->clearmsg();
    if (d_daemon->respond(RSP_OK) < 0) {
        log_perror(">>> send");
        return (DMNerror);
    }
    return (DMNclose);
}


// Static function
DMNenum
siDaemon::f_kill(const char*)
{
    Dchannel *ch = d_daemon->channel();
    if (!ch)
        return (DMNfatal);
    d_daemon->clearmsg();
    return (DMNkill);
}


// Static function
DMNenum
siDaemon::f_reset(const char*)
{
    d_daemon->clearmsg();
    SI()->Clear();
    SI()->LineInterp(0, 0, true);
    if (d_daemon->respond(RSP_OK) < 0) {
        log_perror(">>> send");
        return (DMNerror);
    }
    return (DMNok);
}


// Static function
DMNenum
siDaemon::f_clear(const char*)
{
    d_daemon->clearmsg();
    if (d_daemon->d_if)
        d_daemon->d_if->app_clear();
    if (d_daemon->respond(RSP_OK) < 0) {
        log_perror(">>> send");
        return (DMNerror);
    }
    return (DMNok);
}


// Static function
DMNenum
siDaemon::f_longform(const char*)
{
    Dchannel *ch = d_daemon->channel();
    if (!ch)
        return (DMNfatal);
    ch->longform = true;
    d_daemon->clearmsg();
    if (d_daemon->respond(RSP_OK) < 0) {
        log_perror(">>> send");
        return (DMNerror);
    }
    return (DMNok);
}


// Static function
DMNenum
siDaemon::f_shortform(const char*)
{
    Dchannel *ch = d_daemon->channel();
    if (!ch)
        return (DMNfatal);
    ch->longform = false;
    d_daemon->clearmsg();
    if (d_daemon->respond(RSP_OK) < 0) {
        log_perror(">>> send");
        return (DMNerror);
    }
    return (DMNok);
}


// Static function
DMNenum
siDaemon::f_dumpmsg(const char*)
{
    Dchannel *ch = d_daemon->channel();
    if (!ch)
        return (DMNfatal);
    ch->dumpmsg = true;
    d_daemon->clearmsg();
    if (d_daemon->respond(RSP_OK) < 0) {
        log_perror(">>> send");
        return (DMNerror);
    }
    return (DMNok);
}


// Static function
DMNenum
siDaemon::f_nodumpmsg(const char*)
{
    Dchannel *ch = d_daemon->channel();
    if (!ch)
        return (DMNfatal);
    ch->dumpmsg = false;
    d_daemon->clearmsg();
    if (d_daemon->respond(RSP_OK) < 0) {
        log_perror(">>> send");
        return (DMNerror);
    }
    return (DMNok);
}


// Static function
DMNenum
siDaemon::f_dieonerror(const char*)
{
    Dchannel *ch = d_daemon->channel();
    if (!ch)
        return (DMNfatal);
    ch->die_on_error = true;
    d_daemon->clearmsg();
    if (d_daemon->respond(RSP_OK) < 0) {
        log_perror(">>> send");
        return (DMNerror);
    }
    return (DMNok);
}


// Static function
DMNenum
siDaemon::f_nodieonerror(const char*)
{
    Dchannel *ch = d_daemon->channel();
    if (!ch)
        return (DMNfatal);
    ch->die_on_error = false;
    d_daemon->clearmsg();
    if (d_daemon->respond(RSP_OK) < 0) {
        log_perror(">>> send");
        return (DMNerror);
    }
    return (DMNok);
}


// Static function
DMNenum
siDaemon::f_keepall(const char*)
{
    d_daemon->d_keepall = true;
    d_daemon->clearmsg();
    if (d_daemon->respond(RSP_OK) < 0) {
        log_perror(">>> send");
        return (DMNerror);
    }
    return (DMNok);
}


// Static function
DMNenum
siDaemon::f_nokeepall(const char*)
{
    d_daemon->d_keepall = false;
    d_daemon->clearmsg();
    if (d_daemon->respond(RSP_OK) < 0) {
        log_perror(">>> send");
        return (DMNerror);
    }
    return (DMNok);
}


namespace {
    inline bool has_space(const char *str)
    {
        for (const char *s = str; *s; s++) {
            if (isspace(*s))
                return (true);
        }
        return (false);
    }
}


// Static function
DMNenum
siDaemon::f_geom(const char *args)
{
    Dchannel *ch = d_daemon->channel();
    if (!ch)
        return (DMNerror);

    siVariable v;
    v.type = TYP_STRING;
    char *cgdname = lstring::getqtok(&args);
    bool query = false;
    if (cgdname && !strcmp(cgdname, "?")) {
        query = true;
        cgdname = lstring::getqtok(&args);
    }
    if (!cgdname) {
        // query:  geom or geom ?
        // reply:  list of cgdnames
        stringlist *names = CDcgd()->cgdList();
        stringlist::sort(names);

        // The names are allowed to contain white space, so we have to
        // quote these.
        sLstr lstr;
        for (stringlist *sl = names; sl; sl = sl->next) {
            if (lstr.string())
                lstr.add_c(' ');
            if (has_space(sl->string)) {
                lstr.add_c('"');
                lstr.add(sl->string);
                lstr.add_c('"');
            }
            else
                lstr.add(sl->string);
        }
        stringlist::destroy(names);
        v.content.string = lstr.string_trim();
        if (d_daemon->respond(&v, true) < 0) {
            log_perror(">>> send");
            return (DMNerror);
        }
        return (DMNok);
    }
    cCGD *cgd = CDcgd()->cgdRecall(cgdname, false);
    delete [] cgdname;
    if (query) {
        // query:  geom ? cgdname
        // reply:  "y" or "n"

        v.content.string = cgd ? (char*)"y" : (char*)"n";
        if (d_daemon->respond(&v, true) < 0) {
            log_perror(">>> send");
            return (DMNerror);
        }
        return (DMNok);
    }
    if (!cgd) {
        if (d_daemon->respond(&v, true) < 0) {
            log_perror(">>> send");
            return (DMNerror);
        }
        return (DMNok);
    }

    bool remove = false;
    char *cellname = lstring::gettok(&args);
    if (cellname) {
        if (!strcmp(cellname, "-")) {
            remove = true;
            cellname = lstring::gettok(&args);
        }
        else if (!strcmp(cellname, "?-") || !strcmp(cellname, "-?")) {
            query = true;
            remove = true;
            cellname = lstring::gettok(&args);
        }
        else if (!strcmp(cellname, "?")) {
            query = true;
            cellname = lstring::gettok(&args);
        }
    }
    if (!cellname) {
        if (remove) {
            // query:  geom cgdname - or geom cgdname -? or geom cgdname ?-
            // reply:  list of removed cellnames
            stringlist *names = cgd->unlisted_list();
            char *s = stringlist::flatten(names, " ");
            GCarray<char*> gc_s(s);
            stringlist::destroy(names);
            v.content.string = s;
            if (d_daemon->respond(&v, true) < 0) {
                log_perror(">>> send");
                return (DMNerror);
            }
        }
        else {
            // query:  geom cgdname or geom cgdname ?
            // reply:  list of cellnames in database
            stringlist *names = cgd->cells_list();
            char *s = stringlist::flatten(names, " ");
            GCarray<char*> gc_s(s);
            stringlist::destroy(names);
            v.content.string = s;
            if (d_daemon->respond(&v, true) < 0) {
                log_perror(">>> send");
                return (DMNerror);
            }
        }
        return (DMNok);
    }
    GCarray<char*> gc_cellname(cellname);
    if (remove) {
        if (query) {
            // query:  geom cgdname ?- cellname or geom cgdname -? cellname
            // reply:  "y" or "n"
            v.content.string = cgd->unlisted_test(cellname) ?
                (char*)"y" : (char*)"n";
        }
        else {
            // query:  geom cgdname - cellname
            // reply:  "y" or "n"
            v.content.string = cgd->remove_cell(cellname) ?
                (char*)"y" : (char*)"n";
        }
        if (d_daemon->respond(&v, true) < 0) {
            log_perror(">>> send");
            return (DMNerror);
        }
        return (DMNok);
    }
    else if (query) {
        // query:  geom cgdname ? cellname
        // reply:  "y" or "n"

        v.content.string = cgd->cell_test(cellname) ? (char*)"y" : (char*)"n";
        if (d_daemon->respond(&v, true) < 0) {
            log_perror(">>> send");
            return (DMNerror);
        }
        return (DMNok);
    }

    char *layername = lstring::gettok(&args);
    if (layername && !strcmp(layername, "?")) {
        query = true;
        layername = lstring::gettok(&args);
    }
    if (!layername) {
        // query:  geom cgdname cellname
        // reply:  list of layers
        stringlist *names = cgd->layer_list(cellname);
        char *s = stringlist::flatten(names, " ");
        GCarray<char*> gc_s(s);
        stringlist::destroy(names);
        v.content.string = s;
        if (d_daemon->respond(&v, true) < 0) {
            log_perror(">>> send");
            return (DMNerror);
        }
        return (DMNok);
    }
    GCarray<char*> gc_layername(layername);
    if (query) {
        // query:  geom cgdname cellname ? layername
        // reply:  "y" or "n"

        v.content.string = cgd->layer_test(cellname, layername) ?
            (char*)"y" : (char*)"n";
        if (d_daemon->respond(&v, true) < 0) {
            log_perror(">>> send");
            return (DMNerror);
        }
        return (DMNok);
    }

    // query:  geom cgdname cellname layername
    // reply:  int int payload

    size_t csz, usz;
    const unsigned char *data;
    if (!cgd->find_block(cellname, layername, &csz, &usz, &data)) {
        log_printf("find_block failed, %s\n", Errs()->get_error());
        return (DMNerror);
    }

    size_t sz = csz ? csz : usz;
    sz += 16;
    unsigned char *tbf = new unsigned char[sz];
    int32_t *ii = (int32_t*)tbf;
    ii[0] = htonl(RSP_GEOM);
    ii[1] = htonl(sz - 8);
    ii[2] = htonl(csz);
    ii[3] = htonl(usz);
    memcpy(tbf + 16, data, sz - 16);

    int ret = send(ch->socket, (char*)tbf, sz, 0);
    delete [] tbf;
    if (ret < 0) {
        log_perror(">>> send");
        return (DMNerror);
    }

    return (DMNok);
}


//-----------------------------------------------------------------------------
// Exported Utilities
//-----------------------------------------------------------------------------

// Read nbytes bytes from skt.  This won't quit until it gets
// nbytes bytes, or the socket is closed or an error occurs.
//
bool
daemon_client::read_n_bytes(int skt, unsigned char *buf, int nbytes)
{
    unsigned char *s = buf;
    while (nbytes) {
        int ret = recv(skt, (char*)s, nbytes, 0);
        if (ret < 0) {
            if (errno != EINTR) {
                // read error
                Errs()->add_error("read_n_bytes: read error");
                return (false);
            }
            // got interrupt, retry
            continue;
        }
        if (ret == 0) {
            // server has closed connection
            Errs()->add_error("read_n_bytes: connection closed");
            return (false);
        }
        nbytes -= ret;
        s += ret;
    }
    return (true);
}


// This will read a response from the server, based on the server's
// protocol.
//
// Response messages:
//    numeric vals are MSB first, strings include trailing \0
//  (int32) 0                               ok
//  (int32) 1                               in block, waiting for "end"
//  (int32) 2                               error
//  (int32) 3 (double64) <val>              scalar data
//  (int32) 4 (int32) <strlen+1> <string>   string data
//  (int32) 5 (int32) <numelts>  <values>   array data
//  (int32) 6 (int32) <numzds>   <values>   zlist data
//  (int32) 7 (int32) <strlen+1> <string>   lexpr data
//  (int32) 8 (int32) <id>                  handle data
//  (int32) 9 (int32) <datalen>  <data>     geom data
//  (int32) 10 (2*double64) <real><imag>    complex data
//
bool
daemon_client::read_msg(int skt, int *id_ret, int *size_ret,
    unsigned char **msg_ret)
{
    *id_ret = -1;
    *size_ret = 0;
    *msg_ret = 0;

    int32_t id;
    if (!read_n_bytes(skt, (unsigned char*)&id, 4))
        return (false);
    id = ntohl(id);
    bool longform = false;
    if (id & LONGFORM_FLAG) {
        longform = true;
        id &= ~LONGFORM_FLAG;
    }
    if (id < RSP_OK || id > RSP_GEOM) {
        Errs()->add_error("read_msg: unexpected message id %d", id);
        return (false);
    }

    *id_ret = id;
    if (id >= RSP_OK && id <= RSP_ERR)
        return (true);
    if (!longform && id != RSP_GEOM)
        return (true);

    if (id == RSP_SCALAR) {
        // Scalar data.
        unsigned char *msg = new unsigned char[8];
        if (!read_n_bytes(skt, msg, 8))
            return (false);
        *size_ret = 8;
        *msg_ret = msg;
        return (true);
    }

    int32_t size;
    if (!read_n_bytes(skt, (unsigned char*)&size, 4))
        return (false);
    size = ntohl(size);

    if (id == RSP_STRING || id == RSP_LEXPR || id == RSP_GEOM) {
        // String data, size includes null terminator.
        // Lexpr data, size includes null terminator.
        // Geom data.
        if (size < 0) {
            Errs()->add_error("read_msg: inconsistent record size");
            return (false);
        }
        // We'll accept a null string, though this shouldn't happen.
        if (size > 0) {
            unsigned char *msg = new unsigned char[size];
            if (!read_n_bytes(skt, msg, size))
                return (false);
            *msg_ret = msg;
        }
        *size_ret = size;
    }
    else if (id == RSP_ARRAY) {
        // Array data, size is number of elements.
        if (size < 0) {
            Errs()->add_error("read_msg: inconsistent record size");
            return (false);
        }
        // We'll accept a null array, though this shouldn't happen.
        if (size > 0) {
            size *= 8;  // size in bytes
            unsigned char *msg = new unsigned char[size];
            if (!read_n_bytes(skt, msg, size))
                return (false);
            *msg_ret = msg;
        }
        *size_ret = size;
    }
    else if (id == RSP_ZLIST) {
        // Trapezoid data, size is number of trapezoids.
        if (size < 0) {
            Errs()->add_error("read_msg: inconsistent record size");
            return (false);
        }
        // We'll accept a null list, though this shouldn't happen.
        if (size > 0) {
            size *= 24;  // 6*4 bytes per trapezoid
            unsigned char *msg = new unsigned char[size];
            if (!read_n_bytes(skt, msg, size))
                return (false);
            *msg_ret = msg;
        }
        *size_ret = size;
    }
    else if (id == RSP_HANDLE) {
        // Handle data, size is handle id.
        *size_ret = size;
    }
    else if (id == RSP_CMPLX) {
        // Complex data.
        unsigned char *msg = new unsigned char[16];
        if (!read_n_bytes(skt, msg, 16))
            return (false);
        *size_ret = 16;
        *msg_ret = msg;
    }
    return (true);
}


// Data Conversion and Byte Order
//
// Xic uses "network byte order" for integers, which is the networking
// standard and conversion functions (man byteorder) exist for any
// sane operating system.  Floating point data doesn't seem to have
// the same level of support, so we have to roll our own (ntohd()
// below).  The user need only remember the following:
// for integers: apply ntohl()
// for doubles:  apply ntohd()

// Note: IEEE floating point is assumed here!

namespace {
    // Convert from "network byte order" which is defined here as
    // the same order Sun (sparc) uses (reverse that of I386)
    //
    double
    ntohd(double dn)
    {
        union { double d; unsigned char s[8]; } u, uh;
        u.d = 1.0;
        if (u.s[7]) {
            // This means MSB's are at top address on this host,
            // reverse bytes.
            u.d = dn;
            unsigned char *s = uh.s;
            for (int i = 7; i >= 0; i--)
                *s++ = u.s[i];
            return (uh.d);
        }
        else
            return (dn);
    }
}


// For a response code rt and message buffer buf of size nbytes, set
// up the variable res to contain the transmitted data, if any.  If
// there is data and res is set, data_ok will be set true.
//
void
daemon_client::convert_reply(int rt, unsigned char *buf, int nbytes,
    Variable *res, bool *data_ok)
{
    res->type = TYP_NOTYPE;
    res->content.string = res->name;
    *data_ok = false;

    if (RSP_OK >= 0 && rt <= RSP_ERR)
        return;

    if (rt == RSP_SCALAR) {
        // "scalar", grab the value
        if (nbytes == 8) {
            res->type = TYP_SCALAR;
            res->content.value = ntohd(*(double*)buf);
            *data_ok = true;
        }
    }
    else if (rt == RSP_STRING) {
        // "string", grab the length and the data
        if (nbytes > 0 && buf) {
            char *str = new char[nbytes];
            memcpy(str, buf, nbytes);
            res->type = TYP_STRING;
            res->content.string = str;
            res->flags |= VF_ORIGINAL;
            *data_ok = true;
        }
    }
    else if (rt == RSP_ARRAY) {
        // "array", grab the size and the data
        if (nbytes >= 8 && buf) {
            int asize = nbytes/8;
            double *arry = new double[asize];
            unsigned char *p = buf;
            for (int i = 0; i < asize; i++) {
                arry[i] = ntohd(*(double*)p);
                p += 8;
            }
            res->type = TYP_ARRAY;
            res->content.a = new siAryData;
            res->content.a->allocate(asize);
            res->content.a->dims()[0] = asize;
            memcpy(res->content.a->values(), arry, asize*sizeof(double));
            delete [] arry;
            res->flags |= VF_ORIGINAL;
            *data_ok = true;
        }
    }
    else if (rt == RSP_ZLIST) {
        // "zlist", grab the size and the data
        if (nbytes >= 24 && buf) {
            int numz = nbytes/24;
            unsigned char *p = buf;
            Zlist *z0 = 0, *ze = 0;
            Zoid z;
            for (int i = 0; i < numz; i++) {
                z.xll = ntohl(*(int32_t*)p);
                p += 4;
                z.xlr = ntohl(*(int32_t*)p);
                p += 4;
                z.yl = ntohl(*(int32_t*)p);
                p += 4;
                z.xul = ntohl(*(int32_t*)p);
                p += 4;
                z.xur = ntohl(*(int32_t*)p);
                p += 4;
                z.yu = ntohl(*(int32_t*)p);
                p += 4;

                if (!z0)
                    z0 = ze = new Zlist(&z, 0);
                else {
                    ze->next = new Zlist(&z, 0);
                    ze = ze->next;
                }
            }
            res->type = TYP_ZLIST;
            res->content.zlist = z0;
            *data_ok = true;
        }
    }
    else if (rt == RSP_LEXPR) {
        // lexpr", looks like a string
        if (nbytes > 0 && buf) {
            const char *p = (const char*)buf;
            sLspec *lspec = new sLspec;
            if (lspec->parseExpr(&p) && lspec->setup()) {
                res->type = TYP_LEXPR;
                res->content.lspec = lspec;
                *data_ok = true;
            }
            else {
                delete lspec;
                Errs()->get_error();
            }
        }
    }
    else if (rt == RSP_HANDLE) {
        // handle, grab id
        // *** What to do with this?  The handle means nothing locally.
        if (nbytes > 0) {
            res->type = TYP_HANDLE;
            res->content.value = nbytes;
            *data_ok = true;
        }
    }
    else if (rt == RSP_GEOM) {
        // compressed geometry string
        if (nbytes > 8 && buf) {
            unsigned char *p = buf;
            int32_t csz = *(int32_t*)p;
            csz = ntohl(csz);
            p += 4;
            int32_t usz = *(int32_t*)p;
            usz = ntohl(usz);
            p += 4;

            unsigned char *tbf = new unsigned char[nbytes - 8];
            memcpy(tbf, p, nbytes - 8);
            bstream_t *bs = new bstream_t(tbf, csz, usz, true);

            FIOcvtPrms prms;
            cv_incr_reader *ircr = new cv_incr_reader(bs, &prms);

            sHdl *hdltmp = new sHdlBstream(ircr);
            res->type = TYP_HANDLE;
            res->content.value = hdltmp->id;

            *data_ok = true;
        }
    }
    else if (rt == RSP_CMPLX) {
        if (nbytes == 16) {
            res->type = TYP_CMPLX;
            res->content.cx.real = ntohd(*(double*)buf);
            res->content.cx.imag = ntohd(*(double*)(buf+8));
            *data_ok = true;
        }
    }
}



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
 * Xic Integrated Circuit Layout and Schematic Editor                     *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#ifndef SI_DAEMON_H
#define SI_DAEMON_H


// Response codes for daemon mode.
enum RSPtype
{
    RSP_OK,
    RSP_MORE,
    RSP_ERR,
    RSP_SCALAR,
    RSP_STRING,
    RSP_ARRAY,
    RSP_ZLIST,
    RSP_LEXPR,
    RSP_HANDLE,
    RSP_GEOM,
    RSP_CMPLX
};


// A pointer to an instantiation of this class is passed to the daemon
// constructor to provide communication with the application.  The
// pointer must remain valid through the lifetime of the server.
//
struct siDaemonIf
{
    virtual ~siDaemonIf() { }

    virtual void app_clear() = 0;
        // Respond to the "clear" directive at the application level.

    virtual char *app_id_string() = 0;
        // Return a string that is echoed to stdout when the server
        // starts.

    virtual void app_listen_init() = 0;
        // This is called before the server begins listening, allows
        // the application to initialize itself if necessary.

    virtual void app_transact_init() = 0;
        // This is called abefore every transaction, allows the
        // application to initialize itself if necessary.

    virtual FILE *app_open_log(const char*, const char*) = 0;
        // The application can open and return a file pointer to a log
        // file.  If this returns 0, the daemon will open its own los
        // in the current directory.
};


// Transaction result
enum DMNenum  { DMNok, DMNerror, DMNfatal, DMNclose, DMNkill };

typedef DMNenum (*DMNfunc)(const char*);

#define D_MAX_OPEN 5

struct siDaemon
{
    struct Dchannel
    {
        void set(int skt)
            {
                socket = skt;
                longform = false;
                dumpmsg = false;
                die_on_error = false;
            }

        int socket;
        bool longform;
        bool dumpmsg;
        bool die_on_error;
    };

    static int start(int, siDaemonIf*);
    static int server_skt();

private:
    siDaemon(int, siDaemonIf*);
    ~siDaemon();
    void clearmsg() { delete [] d_msg; d_msg = 0; }

    static siDaemon *daemon() { return (d_daemon); }

    Dchannel *channel()
        {
            if (d_channel < 0)
                return (0);
            return (&d_channels[d_channel]);
        }

    bool init();
    void to_bg();
    bool start_listening();

    DMNenum transact();
    int respond(RSPtype);
    int respond(siVariable*, bool);
    void close_socket(int);
    static void log_printf(const char*, ...);
    static void log_perror(const char*);
    static char *recv_msg(int);

    static DMNenum f_close(const char*);
    static DMNenum f_kill(const char*);
    static DMNenum f_reset(const char*);
    static DMNenum f_clear(const char*);
    static DMNenum f_longform(const char*);
    static DMNenum f_shortform(const char*);
    static DMNenum f_dumpmsg(const char*);
    static DMNenum f_nodumpmsg(const char*);
    static DMNenum f_dieonerror(const char*);
    static DMNenum f_nodieonerror(const char*);
    static DMNenum f_keepall(const char*);
    static DMNenum f_nokeepall(const char*);
    static DMNenum f_geom(const char*);

    Dchannel d_channels[D_MAX_OPEN]; // connection channels
    SymTab *d_ftab;         // hash table for functions
    FILE *d_logfp;          // stdout log file
    FILE *d_errfp;          // stderr log file
    char *d_msg;            // received message
    int d_port;             // port number
    int d_acc_skt;          // accept socket
    int d_channel;          // current channel
    int d_server_skt;       // server socket;
    bool d_listening;       // true when daemon is active
    bool d_debug;           // true in debugging mode
    bool d_keepall;         // if true, don't reset on close
    siDaemonIf *d_if;       // interface to application

    static siDaemon *d_daemon;
};


// This is ored with the RSPtype to indicate a "longform" return.
//
#define LONGFORM_FLAG 0x80

namespace daemon_client {
    bool read_n_bytes(int, unsigned char*, int);
    bool read_msg(int, int*, int*, unsigned char**);
    void convert_reply(int, unsigned char*, int, Variable*, bool*);
}

#endif


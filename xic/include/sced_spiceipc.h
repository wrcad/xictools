
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

#ifndef SCED_SPICEIPC_H
#define SCED_SPICEIPC_H


struct hostent;
struct CmdDesc;

// Global SPICE interface
//
class cSpiceIPC
{
public:
    cSpiceIPC()
        {
            ctor_init();
        }

    ~cSpiceIPC()
        {
            CloseSpice();
        }

    bool InitSpice();
    bool RunSpice(CmdDesc*);
    bool DoCmd(const char*, char**, char**, char**, unsigned char** = 0);
    bool FileToSpice(const char*, char**);
    bool ExecPlot(const char*);
    bool SetIplot(bool = false);
    bool ClearIplot(bool = false);
    void InterruptSpice();
    void CloseSpice();
    void SigIOhdlr(int);

    bool Active()               { return (ipc_msg_skt > 0); }
    bool SimulationActive()     { return ((ipc_in_spice && ipc_msg_skt > 0)); }

    int ParentSpPort()          { return (ipc_parent_sp_port); }
    void SetParentSpPort(int p) { ipc_parent_sp_port = p; }
    void SetParentSpPID(int p)  { ipc_parent_sp_pid = p; }

    static char *ReadLine(FILE*, bool);

private:
    void ctor_init();
    int init_remote(const char*);
    int init_local();
    bool runnit(const char*);
    bool write_msg(const char*, int);
    bool isready(int, int, bool=false);
    bool read_msg(int, int, char**, bool = false, unsigned char** = 0);
    bool read_cmd_return(char**,  char**, unsigned char**);
    char *read_stdout(int);
    void dump_stdout();
    char *send_to_spice(const char*, int);
    bool deck_to_spice(stringlist*, char**);
    bool inprogress();
    bool complete_spice();
    void close_all();
    void setup_async_io(bool);
    bool set_async(int, bool);
    bool expand_includes(stringlist**, const char*);

    static void terminate_spice(int);
#ifdef WIN32
    static void remote_stdout_thread_proc(void*);
    static void local_stdout_thread_proc(void*);
    static void child_thread_proc(void*);
#else
    static void child_hdlr(int, int, void*);
#endif
    static void interrupt_hdlr(int);
    static int open_skt(hostent*, int);

    // Callback function, implemented in application.  This is called
    // from InitSpice, and initializes the ipc_spice_path,
    // ipc_spice_host, ipc_host_prog, ipc_no_graphics, ipc_no_toolbar, and
    // ipc_display_name variables through the arguments.  If false is
    // returned, the IPC setup is aborted.
    //
    static bool callback(const char**, const char**, const char**,
        bool*, bool*, const char**);

    char *ipc_analysis;     // analysis_string
    char *ipc_last_cir;     // previous circuit name
    char *ipc_last_plot;    // previous plot name
    void (*ipc_sigint_back)(int);    // signal handler backup
    void (*ipc_sigio_back)(int);     // signal handler backup
    void (*ipc_sigpipe_back)(int);   // signal handler backup
    int ipc_msg_skt;        // message socket

    // These two entries form a socket pair, need to appear in
    // sequence since they are passed as an array.
    int ipc_stdout_skt;     // stdout/stderr socket pair, main
    int ipc_stdout_skt2;    // stdout/stderr socket pair, child
#ifdef WIN32
    void *ipc_stdout_hrd;   // local-mode stdout/stderr
#endif

    int ipc_spice_port;     // initial port for simulator communications
    int ipc_child_sp_pid;   // simulator process id if spice spawned

    // Initialize these in constructor only.
    int ipc_parent_sp_port; // simulator port if WRspice spawning
    int ipc_parent_sp_pid;  // simulator proc id if WRspice spawning

#ifdef WIN32
    bool ipc_stdout_async;  // in win32. stdout is async
#endif
    bool ipc_in_spice;      // set when simulating
    bool ipc_startup;       // connection was just established
    char ipc_level;         // initialization level: 0-2

    char *ipc_spice_path;   // full path to local wrspice
    char *ipc_spice_host;   // remote spice host
    char *ipc_host_prog;    // wrspice program on remote host
    char *ipc_display_name; // X display name used on remote host
    bool ipc_no_graphics;   // don't use WRspice graphics
    bool ipc_no_toolbar;    // don't show WRspice toolbar
    int ipc_msg_pgid;       // backup group ids for async
    int ipc_stdout_pgid;
};

#endif


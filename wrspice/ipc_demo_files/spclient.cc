
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
 * spclient -- Example WRspice IPC Application                            *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#include "config.h"

#include "spclient.h"
#include "pathlist.h"
#include "tvals.h"
#include "errorrec.h"
#ifdef WIN32
#include <winsock2.h>       // for ntohl
#else
#include <netinet/in.h>     // for ntohl
#endif
#include <signal.h>


// THIS RELEASE:  Last updated for WRspice 4.1.1.

// This demo contains the interprocess communication code (IPC) from
// Xic, used to communicate with WRspice.  The code has been adapted
// into a stand-alone application, which provides a simple
// command-line interface to WRspice through IPC.  This either forks
// WRspice on the local machine, or communicates through the wrspiced
// program running on a remote (or the local) machine.

// This application is an example, and is not expected to be useful
// as-is.  However, the code here should illustrate how to build a
// custom interface to WRspice, for use in other applications.

// Basically, this file can be a starting point for a custom
// application.  The other files can be considered as "library files",
// as it is less likely that they will require changes.  They provide
// the interface which should be adequate for most applications.


//-----------------------------------------------------------------------------
// Defaults, these can be overridden on command line.

namespace {
    // Path to WRspice on local machine.
    const char *path_to_wrspice = "/usr/local/xictools/bin/wrspice";

    // Remode host running wrspiced.
    const char *wrspice_host = 0;

    // Use X graphics.
    bool use_x = false;

    // Show WRspice toolbar if using graphics.
    bool show_toolbar = false;

    // *** NEW in 3.2.11 ***
    // The X display name used on the remote machine for graphics.
    const char *wrspice_remote_display = 0;

    // Local function to process command line arguments.
    void process_args(int, char**);
}

//-----------------------------------------------------------------------------
// Cruft to instantiate and initialize the interface code.  Copy this
// block into your custom application. 

// Declare static members of support classes.
sPL *sPL::_pl;
cErrLog *cErrLog::_el;
sSCD *sSCD::_scd;
cSpiceIPC *sSCD::_ipc;
DspPkgIf *DspPkgIf::_dsp_if;
sMenu *sMenu::_menu;

// Instantiate classes.  These are all global singletons.
sPL _pl_;
cErrLog _el_;
DspPkgIf _dsp_;
sMenu _menu_;

// This is an error-reporting class.
ErrRec _err_;

// This instantiates the cSpiceIPC class, too.
sSCD _scd_;

//-----------------------------------------------------------------------------
// In binary data returns, integers and doubles are sent in "network
// byte order", which may not be the same as the machine order.  Most
// C libraries have htonl, ntohl, etc.  functions for dealing with
// this, however there does not appear to be support for float/double. 
// The function below does the trick for doubles, both to and from
// network byte order.  It simply reverses byte order if machine order
// does not match network byte order.

// If your application will always run WRspice on the same machine, or
// the same type of machine (e.g., Intel CPU), then you don't need to
// worry about byte order.  In particular, Intel x86 will reorder
// bytes, which has some overhead.  If all machines are Intel, then
// this overhead can be eliminated by skipping the byte ordering
// functions and read/write the values directly.

namespace {
    // Reverse the byte order if the MSB's are at the top address, i.e.,
    // switch to/from "network byte order".  This will reverse bytes on
    // Intel x86, but is a no-op on Sun SPARC (for example).
    // 
    // IEEE floating point is assumed here!
    //
    double net_byte_reorder(double d)
    {
        static double d1p0 = 1.0;

        if ( ((unsigned char*)&d1p0)[7] ) {
            // This means MSB's are at top address, reverse bytes.
            double dr;
            unsigned char *t = (unsigned char*)&dr;
            unsigned char *s = (unsigned char*)&d;
            t[0] = s[7];
            t[1] = s[6];
            t[2] = s[5];
            t[3] = s[4];
            t[4] = s[3];
            t[5] = s[2];
            t[6] = s[1];
            t[7] = s[0];
            return (dr);
        }
        else
            return (d);
    }
}


// *** NEW in 3.2.11 ***
//-----------------------------------------------------------------------------
// Interrupt (Ctrl-C keypress) signal handler.  Pressing Ctrl-C will
// pause a run in progress in WRspice.

namespace {
    // Interrupt signal handling for use during WRspice simulations.
    //
    void
    intr_spice_hdlr(int)
    {
        signal(SIGINT, intr_spice_hdlr);
        SCD()->spif()->InterruptSpice();
    }
}


//-----------------------------------------------------------------------------
// The main function.  The invocation is
//   spclient [-g] [-t] [-s path_to_wrspice] [[-r] host[:port]] [-d display]
//
// Arguments are:
//   -g         Use X graphics if given.
//   -t         Show WRspice toolbar if using graphics when given.
//   -s path    Path to WRspice executable for local mode.
//   -r host    Name of remote machine running wrspiced (this can be
//              the local machine, too).  The port number can follow,
//              separated by a colon.  The "-r" is optional.
// *** NEW in 3.2.11 ***
//   -d display When using a remote host, the X display to use,
//              needed when running graphics.  If not given, a
//              display name will be created as follows:
//              If the local DISPLAY variable is something like ":0.0",
//              the remote display name will be "localhostname:0.0".
//              If the local DISPLAY variable is already in the form
//              "localhostname:0.0", this is passed verbatim.
//
//              One can use ssh transport for the X connection on the
//              remote system as follows.  Use "ssh -X" to open a shell
//              on the remote machine.  Type "echo $DISPLAY" into this
//              window, it will print something like "localhost:10.0".
//              Use this value for the 'd' option.  The shell must
//              remain open while spclient is running WRspice, exiting
//              the shell will close the X connection.  See the comment
//              for "display_name" near the end of this file for more
//              info.
//
// If a hostname is given, the program will communicate with wrspiced
// running on that host.  Without a hostname, the program will
// communicate with WRspice, after forking a process on the local
// machine.
//
// The program takes command text, pretty much like WRspice in normal
// operation.  The Esc key will send an interrupt to WRspice.
//
// There are a few commands that are only available through IPC. 
// These are listed below.
//
// send filename
//   The "send filename" command will send a local file, after expanding
//   .inc/.lib lines, to WRspice, where it is sourced.  The regular
//   source command will source a file from the WRspice host.  Unlike
//   the other special IPC commands listed here, the send command is
//   implemented on the client side, the remaining commands are
//   implemented in WRspice.
//
// inprogress
//   Returns "y" or "n", depending on whether or not WRspice is currently
//   running a simulation.  This is only valuable in asynchronous mode.
//
// decksource
//   The lines that follow are a SPICE deck, terminated by a line containing
//   the single character '@'.  This is used in the FileToSpice function.
//
// curplot
//   The current plot name is returned, or "none".
//
// curckt
//   The curent circuit name is returned, or "none".
//
// freeplot name
//   This will destroy the plot with the given name.
//
// freecir name
//   This will destroy the circuit with the given name.
//
// close
//   This will terminate WRspice, but not the program.  A new channel will
//   be opened if a new command is given.
//
// winid number
//   The number is an X window ID.  This is used to register a big window
//   such as Xic's, so that WRspice can arrange for its windows to always
//   appear on top.
//
// vecget vecname
//   The return value is the value of the 0'th component of the named vector,
//   in a format ok%15e or ok%15e,%15e for complex.
//
// eval expression
//   The expression is a vector expression as understood by WRspice. 
//   The expression will be evaluated, and the result returned as binary
//   data.  A simple example:  load a deck and run it, say it produces
//   an output vector named "v(1)".  Then "eval v(1)" will return a
//   block of data containing all of the v(1) values from the
//   simulation.
//
// ping
//   The returned value is "okC", where C is the subcircuit concatenation
//   character (which presently defaults to underscore).
//
// Any other input will be passed to the WRspice command interpreter.
// 
//
int
main(int argc, char **argv)
{
    process_args(argc, argv);

    // *** NEW in 3.2.11 ***
    // Support for MS Windows, demo will build and run with Mingw
    // (I use gcc-4.5.0).  Cygwin may work, too.
    // 
#ifdef WIN32
    // initialize winsock
    WSADATA wsadata;
    if (WSAStartup(MAKEWORD(2, 2), &wsadata) != 0) {
        fprintf(stderr,
        "Error: Windows Socket Architecture initialization failed.\n");
        return (1);
    }
#endif

    // *** NEW in 3.2.11 ***
    // Setup signal handling.  A Ctrl-C press will be sent to WRspice,
    // pausing any analysis in progress.  The run can be started again
    // with the "resume" command.  Ctrl-C will not kill the main
    // (spclient) program, which would be the behavior if the line
    // below is commented out.
    //
    signal(SIGINT, intr_spice_hdlr);

    // *** NEW in 3.2.11 ***
    // The InitSpice function now returns a boolean, false on failure.
    //
    // Establish the connection, bail out on error.
    if (!SCD()->spif()->InitSpice()) {
        if (Errs()->has_error())
            fprintf(stderr, "%s\n", Errs()->get_error());
        return (1);
    }

    char buf[256];
    for (;;) {

        // Prompt the user for a command.
        fprintf(stdout, "wrspice> ");
        fflush(stdout);

        // Get a line of input from the user.
        fgets(buf, 256, stdin);

        // *** NEW in 3.2.11 ***
        // Check, bail out if WRspice connection broken (i.e., WRspice
        // is no longer running).  We don't do this here (WRspice will
        // be restarted instead) but user may want to do this check
        // periodically in custom code.
        //
        // if (!SCD()->spif()->Active())
        //     break;

        if (buf[0] == 0x1b) {  // Esc
            SCD()->spif()->InterruptSpice();
            continue;
        }

        // Strip line termination.
        char *t = buf + strlen(buf) - 1;
        while (t >= buf && (*t == '\n' || *t == '\r'))
            *t-- = 0;

        if (lstring::match("exit", buf) || lstring::match("quit", buf))
            break;

        if (!*buf)
            continue;

        // Run the command.
        char *retbuf;           // Message returned.
        char *outbuf;           // Stdout/stderr returned.
        char *errbuf;           // Error message.
        unsigned char *databuf; // Command data.
        if (!SCD()->spif()->DoCmd(buf, &retbuf, &outbuf, &errbuf, &databuf)) {
            // *** NEW in 3.2.11 ***
            // If "close" was given, just shutup error messages and
            // continue.  Giving another command should start a new
            // WRspice to execute the command.

            if (lstring::match("close", buf)) {
                fputs("WRspice closed.\n", stdout);
                delete [] retbuf;
                delete [] errbuf;
                continue;
            }
        }
        if (retbuf) {
            fputs(retbuf, stdout);
            fputc('\n', stdout);
            delete [] retbuf;
        }
        if (errbuf) {
            fputs(errbuf, stdout);
            fputc('\n', stdout);
            delete [] errbuf;
        }
        if (outbuf) {
            fputs(outbuf, stdout);
            delete [] outbuf;
        }
        if (databuf) {
            // This is returned only by the eval command at present. 
            // The format is:
            //
            // databuf[0]      'o'
            // databuf[1]      'k'
            // databuf[2]      'd'  (datatype double, other types may be
            //                       added in future)
            // databuf[3]      'r' or 'c' (real or complex)
            // databuf[4-7]    array size, network byte order
            // ...             array of data values, network byte order

            printf("\n");
            if (databuf[0] != 'o' || databuf[1] != 'k' || databuf[2] != 'd') {
                // error (shouldn't happen)
                delete [] databuf;
                continue;
            }

            // We'll just print the first 10 values of the return.
            unsigned int size = ntohl(*(int*)(databuf+4));
            double *dp = (double*)(databuf + 8);
            for (unsigned int i = 0; i < size; i++) {
                if (i == 10) {
                    printf("...\n");
                    break;
                }
                if (databuf[3] == 'r')      // real values
                    printf("%d   %g\n", i, net_byte_reorder(dp[i]));
                else if (databuf[3] == 'c') //complex values
                    printf("%d   %g,%g\n", i, net_byte_reorder(dp[2*i]),
                        net_byte_reorder(dp[2*i + 1]));
                else
                    // wacky error, can't happen
                    break;
            }
            delete [] databuf;
        }
    }

    // Graceful shutdown.
    SCD()->spif()->CloseSpice();
    return (0);
}


//-----------------------------------------------------------------------------
// Function to process command-line arguments.

namespace {
    void process_args(int argc, char **argv)
    {
        const char *usage =
        "Usage: spclient [-g] [-t] [-s path_to_wrspice] [[-r] host[:port]]\n";

        const char *env = getenv("PATH_TO_WRSPICE");
        if (env)
            path_to_wrspice = env;

        for (int i = 1; i < argc; i++) {
            char *s = argv[i];
            if (*s == '-') {
                if (s[1] == 'g')
                    // Enable graphics.
                    use_x = true;
                else if (s[1] == 't')
                    // Show WRspice toolbar if using graphics.
                    show_toolbar = true;
                else if (s[1] == 's') {
                    // Path to WRspice executable for local mode.
                    i++;
                    if (i < argc)
                        path_to_wrspice = argv[i];
                    else {
                        fputs(usage, stdout);
                        exit(1);
                    }
                }
                else if (s[1] == 'r') {
                    // Remote host running wrspiced.
                    i++;
                    if (i < argc)
                        wrspice_host = argv[i];
                    else {
                        fputs(usage, stdout);
                        exit(1);
                    }
                }
                else if (s[1] == 'd') {
                    i++;
                    if (i < argc)
                        wrspice_remote_display = argv[i];
                    else {
                        fputs(usage, stdout);
                        exit(1);
                    }
                }
                else {
                    fputs(usage, stdout);
                    exit(1);
                }
            }
            else
                wrspice_host = argv[i];
        }
    }
}


//-----------------------------------------------------------------------------
// Static callback for the cSpiceIPC class.  This is called by the
// initialization code, and obtains a few needed setup variables and
// flags.  The function should return true, if false is returned, the
// init will abort.


// Arguments:
//
// spice_path
//   The full path to WRspice on the local machine, for local mode.
//   Required if local mode is to be used.
//
// spice_host
//   The host.domain of the machine to use for remote WRspice runs, which
//   has a wrspiced daemon running.  This can be followed by a colon
//   and a port number, if a non-default port is being used by wrspiced.
//   Required if remote (wrspiced) mode is to be used.
//
// host_prog
//   If using wrspiced, this is the name of or path to the WRspice binary
//   on the remote machine.  Only needed if the WRspice to run is not
//   in the default location or has a different name from "wrspice".
//
// no_x
//   Setting true will prevent WRspice from using graphics.
//
// no_toolbar
//   Setting true will prevent the WRspice toolbar from appearing, when
//   using graphics.
//
// *** NEW in 3.2.11 ***
// display_name
//   The DISPLAY name to use for the X connection on the remote
//   machine, overrides the assumed local display name when non-null.
//   This applies to remote connections (using wrspiced) only, and
//   only when using graphics.
//
//   In legacy X-window systems, the display name would typically be
//   in the form hostname:0.0.  A remote system will draw to the
//   local display if the local hostname was used in the display
//   name, and the local X server permissions were set (with
//   xauth/xhost) to allow access.  Typically, the user would log in
//   to a remote system with telnet or ssh, set the DISPLAY variable,
//   perhaps give "xhost +" on the local machine, then run X
//   programs.
//
//   This method has been largely superseded by use of "X forwarding" in
//   ssh.  This is often automatic, or may require the '-X' option on
//   the ssh command line.  In this case, after using ssh to log in to
//   the remote machine, the DISPLAY variable is automatically set to
//   display on the local machine.  X applications "just work", with no
//   need to fool with the DISPLAY variable, or permissions.
//
//   The present Xic remote access code does not know about the ssh
//   protocol, so we have to fake it in some cases.  In most cases
//   the older method will still work.
//
//   The ssh protocol works by setting up a dummy display, with a
//   name something like "localhost:10.0", which in actuality
//   connects back to the local display.  Depending on how many ssh
//   connections are currently in force, the "10" could be "11",
//   "12", etc.
//
//   In the present case, if we want to use ssh for X transmission,
//   the display name used here must match an existing ssh display
//   name on the remote system that maps back to the local display.
//
//   If there is an existing ssh connection to the remote machine,
//   the associated DISPLAY can be used.  If there is no existing
//   ssh connection, one can be established, and that used.  E.g.,
//   from the ssh window, type "echo $DISPLAY" and use the value
//   printed.
//
//   The display name provided here will override the assumed
//   display name created internally with the local host name.
//
bool
cSpiceIPC::callback(const char **spice_path, const char **spice_host,
    const char **host_prog, bool *no_x, bool *no_toolbar,
    const char **display_name)
{
    *spice_path = path_to_wrspice;

    *spice_host = wrspice_host;

    *host_prog = "wrspice";

    *no_x = !use_x;

    *no_toolbar = !show_toolbar;

    *display_name = wrspice_remote_display;

    return (true);
}

